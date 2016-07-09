/* Copyright (c) 2016 Nick Appleton
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE. */

#ifndef SMPLWAV_H
#define SMPLWAV_H

#include <stdint.h>
#include <stddef.h>
#include <assert.h>
#include "cop/cop_attributes.h"

#define SMPLWAV_MAX_MARKERS             (64)
#define SMPLWAV_MAX_UNSUPPORTED_CHUNKS  (32)

struct smplwav_marker {
	/* id, in_cue, in_smpl and has_ltxt are used while the markers are being
	 * prepared by smplwav_mount(). They are not used by the serialisation
	 * code and are free to be read from and written to by the calling code
	 * for other purposes. */
	uint_fast32_t         id;
	int                   in_cue;
	int                   in_smpl;
	int                   has_ltxt;

	/* From labl */
	char                 *name;

	/* From note */
	char                 *desc;

	/* From ltxt or smpl. */
	uint_fast32_t         length;

	/* Sample offset this marker applies at. */
	uint_fast32_t         position;
};

#define SMPLWAV_FORMAT_PCM16   (0)
#define SMPLWAV_FORMAT_PCM24   (1)
#define SMPLWAV_FORMAT_PCM32   (2)
#define SMPLWAV_FORMAT_FLOAT32 (3)

struct smplwav_format {
	int           format;
	uint_fast32_t sample_rate;
	uint_fast16_t channels;
	uint_fast16_t bits_per_sample;
};

#define SMPLWAV_RIFF_ID(c1, c2, c3, c4) \
	(   ((uint_fast32_t)(c1)) \
	|   (((uint_fast32_t)(c2)) << 8) \
	|   (((uint_fast32_t)(c3)) << 16) \
	|   (((uint_fast32_t)(c4)) << 24) \
	)

/* See "Multimedia Programming Interface and Data Specifications 1.0" for
 * information about how these textual fields should look. */
static COP_ATTR_UNUSED uint_fast32_t SMPLWAV_INFO_TAGS[] =
{	SMPLWAV_RIFF_ID('I', 'A', 'R', 'L') /* Archival Location */
,	SMPLWAV_RIFF_ID('I', 'A', 'R', 'T') /* Artist */
,	SMPLWAV_RIFF_ID('I', 'C', 'M', 'S') /* Commissioned */
,	SMPLWAV_RIFF_ID('I', 'C', 'M', 'T') /* Comments */
,	SMPLWAV_RIFF_ID('I', 'C', 'O', 'P') /* Copyright */
,	SMPLWAV_RIFF_ID('I', 'C', 'R', 'D') /* Creation Date */
,	SMPLWAV_RIFF_ID('I', 'C', 'R', 'P') /* Cropped */
,	SMPLWAV_RIFF_ID('I', 'D', 'I', 'M') /* Dimensions */
,	SMPLWAV_RIFF_ID('I', 'D', 'P', 'I') /* Dots Per Inch */
,	SMPLWAV_RIFF_ID('I', 'E', 'N', 'G') /* Engineer */
,	SMPLWAV_RIFF_ID('I', 'G', 'N', 'R') /* Genre */
,	SMPLWAV_RIFF_ID('I', 'K', 'E', 'Y') /* Keywords */
,	SMPLWAV_RIFF_ID('I', 'L', 'G', 'T') /* Lightness */
,	SMPLWAV_RIFF_ID('I', 'M', 'E', 'D') /* Medium */
,	SMPLWAV_RIFF_ID('I', 'N', 'A', 'M') /* Name */
,	SMPLWAV_RIFF_ID('I', 'P', 'L', 'T') /* Palette Setting */
,	SMPLWAV_RIFF_ID('I', 'P', 'R', 'D') /* Product */
,	SMPLWAV_RIFF_ID('I', 'S', 'B', 'J') /* Subject */
,	SMPLWAV_RIFF_ID('I', 'S', 'F', 'T') /* Software */
,	SMPLWAV_RIFF_ID('I', 'S', 'H', 'P') /* Sharpness */
,	SMPLWAV_RIFF_ID('I', 'S', 'R', 'C') /* Source */
,	SMPLWAV_RIFF_ID('I', 'S', 'R', 'F') /* Source Form */
,	SMPLWAV_RIFF_ID('I', 'T', 'C', 'H') /* Technician */
};

#define SMPLWAV_NB_INFO_TAGS (sizeof(SMPLWAV_INFO_TAGS) / sizeof(SMPLWAV_INFO_TAGS[0]))

struct smplwav_extra_ck {
	uint_fast32_t     id;
	uint_fast32_t     size;
	unsigned char    *data;
};

struct smplwav {
	/* String metadata found in the info chunk. The purpose of each element is
	 * the element with the same index in the SMPLWAV_INFO_TAGS[] array. */
	char                      *info[SMPLWAV_NB_INFO_TAGS];

	/* If there was a smpl chunk, this will always be non-zero and pitch-info
	 * will be set to the midi pitch information. */
	int                        has_pitch_info;
	uint_fast64_t              pitch_info;

	/* Positional based metadata loaded from the waveform. */
	unsigned                   nb_marker;
	struct smplwav_marker      markers[SMPLWAV_MAX_MARKERS];

	/* The data format of the wave file. */
	struct smplwav_format      format;

	/* The number of samples in the wave file and the pointer to its data. */
	uint_fast32_t              data_frames;
	void                      *data;

	/* Chunks which were found in the wave file which cannot be handled by
	 * this implementation. This is anything other than: INFO, fmt, data, cue,
	 * smpl, adtl and fact. */
	unsigned                   nb_unsupported;
	struct smplwav_extra_ck    unsupported[SMPLWAV_MAX_UNSUPPORTED_CHUNKS];
};

static COP_ATTR_UNUSED uint_fast16_t smplwav_format_container_size(int format)
{
	switch (format) {
		case SMPLWAV_FORMAT_PCM16:
			return 2;
		case SMPLWAV_FORMAT_PCM24:
			return 3;
		default:
			assert(format == SMPLWAV_FORMAT_PCM32 || format == SMPLWAV_FORMAT_FLOAT32);
			return 4;
	}
}

void smplwav_sort_markers(struct smplwav *wav);

#endif /* SMPLWAV_H */
