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

/* See "Multimedia Programming Interface and Data Specifications 1.0" for
 * information about how these textual fields should look. */
#define SMPLWAV_INFO_IARL (0)  /* Archival Location */
#define SMPLWAV_INFO_IART (1)  /* Artist */
#define SMPLWAV_INFO_ICMS (2)  /* Commissioned */
#define SMPLWAV_INFO_ICMT (3)  /* Comments */
#define SMPLWAV_INFO_ICOP (4)  /* Copyright */
#define SMPLWAV_INFO_ICRD (5)  /* Creation Date */
#define SMPLWAV_INFO_ICRP (6)  /* Cropped */
#define SMPLWAV_INFO_IDIM (7)  /* Dimensions */
#define SMPLWAV_INFO_IDPI (8)  /* Dots Per Inch */
#define SMPLWAV_INFO_IENG (9)  /* Engineer */
#define SMPLWAV_INFO_IGNR (10) /* Genre */
#define SMPLWAV_INFO_IKEY (11) /* Keywords */
#define SMPLWAV_INFO_ILGT (12) /* Lightness */
#define SMPLWAV_INFO_IMED (13) /* Medium */
#define SMPLWAV_INFO_INAM (14) /* Name */
#define SMPLWAV_INFO_IPLT (15) /* Palette Setting */
#define SMPLWAV_INFO_IPRD (16) /* Product */
#define SMPLWAV_INFO_ISBJ (17) /* Subject */
#define SMPLWAV_INFO_ISFT (18) /* Software */
#define SMPLWAV_INFO_ISHP (19) /* Sharpness */
#define SMPLWAV_INFO_ISRC (20) /* Source */
#define SMPLWAV_INFO_ISRF (21) /* Source Form */
#define SMPLWAV_INFO_ITCH (22) /* Technician */

#define SMPLWAV_NB_INFO_TAGS (23)

/* This function takes an integer less than SMPLWAV_NB_INFO_TAGS and returns
 * the fourcc which corresponds to that item as a string. i.e.
 *   smplwav_info_index_to_string(SMPLWAV_INFO_ICRD) returns "ICRD"
 */
const char *smplwav_info_index_to_string(unsigned index);

/* This function takes a fourcc string and returns the index the info tag
 * index it corresponds to. i.e.
 *   smplwav_info_string_to_index("ICRD") returns SMPLWAV_INFO_ICRD
 * Unsupported values return a negative result.
 * It is undefined for the string to not be 4 characters. */
int smplwav_info_string_to_index(const char *fourcc);

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
