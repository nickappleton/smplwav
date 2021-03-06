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

#include "smplwav/smplwav_mount.h"
#include "cop/cop_conversions.h"
#include "smplwav_internal.h"
#include <string.h>

static struct smplwav_marker *get_new_marker(struct smplwav *wav)
{
	struct smplwav_marker *m;
	if (wav->nb_marker >= SMPLWAV_MAX_MARKERS)
		return NULL;
	m = wav->markers + wav->nb_marker++;
	m->id           = 0;
	m->name         = NULL;
	m->desc         = NULL;
	m->length       = 0;
	m->has_ltxt     = 0;
	m->in_cue       = 0;
	m->in_smpl      = 0;
	m->position     = 0;
	return m;
}

static struct smplwav_marker *get_marker(struct smplwav *wav, uint_fast32_t id)
{
	unsigned i;
	struct smplwav_marker *marker;
	for (i = 0; i < wav->nb_marker; i++) {
		if (wav->markers[i].id == id)
			return &(wav->markers[i]);
	}
	if ((marker = get_new_marker(wav)) != NULL)
		marker->id = id;
	return marker;
}

static
unsigned
load_adtl
	(struct smplwav *wav
	,unsigned char     *adtl
	,size_t             adtl_len
	)
{
	unsigned warnings = 0;

	/* This condition should have been checked by the calling code. */
	assert(adtl_len >= 4);

	/* Move past "adtl" list identifier. */
	adtl_len -= 4;
	adtl     += 4;

	/* Read all of the metadata chunks in the adtl list. */
	while (adtl_len >= 8) {
		uint_fast32_t      meta_class = cop_ld_ule32(adtl);
		uint_fast32_t      meta_size  = cop_ld_ule32(adtl + 4);
		unsigned char     *meta_base  = adtl + 8;
		int                is_ltxt;
		int                is_note;
		struct smplwav_marker *marker;
		uint_fast64_t      cksz;

		/* Move the adtl pointer to the start of the next chunk.*/
		cksz = 8 + (uint_fast64_t)meta_size + (meta_size & 1);
		if (cksz > adtl_len)
			return warnings | SMPLWAV_ERROR_ADTL_INVALID;

		adtl     += cksz;
		adtl_len -= (size_t)cksz;

		/* Make sure this is a chunk we can actually understand. If there
		 * are chunks in the adtl list which are unknown to us, we could
		 * end up breaking metadata. */
		is_ltxt = (meta_class == SMPLWAV_RIFF_ID('l', 't', 'x', 't'));
		is_note = (meta_class == SMPLWAV_RIFF_ID('n', 'o', 't', 'e'));
		if  (  !(is_ltxt && meta_size == 20)
		    && !((is_note || (meta_class == SMPLWAV_RIFF_ID('l', 'a', 'b', 'l'))) && meta_size >= 4)
		    )
			return warnings | SMPLWAV_ERROR_ADTL_INVALID;

		/* The a metadata item with the given ID associated with this adtl
		 * metadata chunk. */
		marker = get_marker(wav, cop_ld_ule32(meta_base));
		if (marker == NULL)
			return warnings | SMPLWAV_ERROR_TOO_MANY_MARKERS;

		/* Skip over the metadata ID */
		meta_base += 4;
		meta_size -= 4;

		if (is_ltxt) {
			if (marker->has_ltxt)
				return SMPLWAV_ERROR_ADTL_DUPLICATES;
			marker->has_ltxt = 1;
			marker->length   = cop_ld_ule32(meta_base);
		} else {
			if (!meta_size || meta_base[meta_size-1] != 0) {
				warnings |= SMPLWAV_WARNING_ADTL_UNTERMINATED_STRINGS;
				continue;
			}

			if (is_note) {
				if (marker->desc != NULL)
					return warnings | SMPLWAV_ERROR_ADTL_DUPLICATES;
				marker->desc = (char *)meta_base;
			} else {
				if (marker->name != NULL)
					return warnings | SMPLWAV_ERROR_ADTL_DUPLICATES;
				marker->name = (char *)meta_base;
			}
		}
	}

	return warnings;
}

static
unsigned
load_cue
	(struct smplwav *wav
	,unsigned char     *cue
	,size_t             cue_len
	)
{
	uint_fast32_t  ncue;

	if (cue_len < 4 || cue_len < 4 + 24 * (ncue = cop_ld_ule32(cue)))
		return SMPLWAV_ERROR_CUE_INVALID;

	cue += 4;
	while (ncue--) {
		uint_fast32_t cue_id      = cop_ld_ule32(cue);
		struct smplwav_marker *marker = get_marker(wav, cue_id);
		if (marker == NULL)
			return SMPLWAV_ERROR_TOO_MANY_MARKERS;

		if (marker->in_cue)
			return SMPLWAV_ERROR_CUE_DUPLICATE_IDS;

		marker->position     = cop_ld_ule32(cue + 20);
		marker->in_cue = 1;
		cue += 24;
	}

	return 0;
}

static
unsigned
load_smpl
	(struct smplwav *wav
	,unsigned char     *smpl
	,size_t             smpl_len
	)
{
	uint_fast32_t nloop;
	uint_fast32_t i;

	if (smpl_len < 36 || (smpl_len < (36 + (nloop = cop_ld_ule32(smpl + 28)) * 24 + cop_ld_ule32(smpl + 32))))
		return SMPLWAV_ERROR_SMPL_INVALID;

	wav->has_pitch_info = 1;
	wav->pitch_info = (((uint_fast64_t)cop_ld_ule32(smpl + 12)) << 32) | cop_ld_ule32(smpl + 16);

	smpl += 36;
	for (i = 0; i < nloop; i++) {
		uint_fast32_t      id    = cop_ld_ule32(smpl);
		uint_fast32_t      start = cop_ld_ule32(smpl + 8);
		uint_fast32_t      end   = cop_ld_ule32(smpl + 12);
		uint_fast32_t      length;
		unsigned           j;
		struct smplwav_marker *marker;

		if (start > end)
			return SMPLWAV_ERROR_SMPL_INVALID;

		length = end - start + 1;

		for (j = 0; j < wav->nb_marker; j++) {
			/* If the loop has the same id as some metadata, but the
			 * metadata has no cue point, we can associate it with this
			 * metadata item. */
			if (id == wav->markers[j].id && !wav->markers[j].in_cue)
				break;

			/* If the loop matches an existing loop, we can associate it
			 * with this metadata item. */
			if  (   (wav->markers[j].in_cue)
			    &&  (wav->markers[j].position == start)
			    &&  (!wav->markers[j].has_ltxt || wav->markers[j].length == length)
			    )
				break;
		}

		if (j < wav->nb_marker) {
			marker = wav->markers + j;
		} else if ((marker = get_new_marker(wav)) == NULL) {
			return SMPLWAV_ERROR_TOO_MANY_MARKERS;
		}

		marker->position   = start;
		marker->in_smpl    = 1;
		marker->length     = length;

		smpl += 24;
	}

	return 0;
}

static
unsigned
check_and_finalise_markers
	(struct smplwav *wav
	,unsigned           flags
	)
{
	unsigned i;
	unsigned dest_idx;
	unsigned nb_smpl_only_loops = 0;
	unsigned nb_cue_only_loops = 0;

	/* Remove markers which contain metadata which does not correspond to
	 * any actual data and count the number of loops which are only found
	 * in cue points and the number that are only in smpl loops. If both
	 * of these are non-zero, one set of them needs to be deleted as we
	 * cannot determine which metadata is correct. */
	for (i = 0, dest_idx = 0; i < wav->nb_marker; i++) {
		/* Metadata not corresponding to an item. Remove. */
		if (!wav->markers[i].in_smpl && !wav->markers[i].in_cue)
			continue;
		/* Check position is actually inside the audio region. */
		if (wav->markers[i].position >= wav->data_frames)
			return SMPLWAV_ERROR_MARKER_RANGE;
		if (wav->markers[i].length > 0) {
			/* Check duration does not go outside audio region. */
			if (wav->markers[i].position + wav->markers[i].length > wav->data_frames)
				return SMPLWAV_ERROR_MARKER_RANGE;
			if (wav->markers[i].in_smpl && !wav->markers[i].in_cue)
				nb_smpl_only_loops++;
			if (!wav->markers[i].in_smpl && wav->markers[i].in_cue)
				nb_cue_only_loops++;
		}
		if (i != dest_idx)
			wav->markers[dest_idx] = wav->markers[i];
		dest_idx++;
	}
	wav->nb_marker = dest_idx;

	/* Were there both independent sampler loops or independent cue
	 * loops? */
	if (!nb_smpl_only_loops || !nb_cue_only_loops)
		return 0;

	/* If the caller has not specified a flag for what do do in this
	 * situation, print some information and fail. Otherwise, delete
	 * the markers belonging to the group we do not care about. */
	if (flags & (SMPLWAV_MOUNT_PREFER_CUE_LOOPS | SMPLWAV_MOUNT_PREFER_SMPL_LOOPS)) {
		for (i = 0, dest_idx = 0; i < wav->nb_marker; i++) {
			int is_loop = wav->markers[i].length > 0;
			if (is_loop && wav->markers[i].in_smpl && !wav->markers[i].in_cue && (flags & SMPLWAV_MOUNT_PREFER_CUE_LOOPS))
				continue;
			if (is_loop && !wav->markers[i].in_smpl && wav->markers[i].in_cue && (flags & SMPLWAV_MOUNT_PREFER_SMPL_LOOPS))
				continue;
			if (i != dest_idx)
				wav->markers[dest_idx] = wav->markers[i];
			dest_idx++;
		}
		wav->nb_marker = dest_idx;
		return SMPLWAV_WARNING_SMPL_CUE_LOOP_CONFLICTS;
	}

	return SMPLWAV_ERROR_SMPL_CUE_LOOP_CONFLICTS;
}

static unsigned load_sample_format(struct smplwav_format *format, unsigned char *fmt_ptr, size_t fmt_sz)
{
	static const unsigned char EXTENSIBLE_GUID_SUFFIX[14] = {/* AA, BB, */ 0x00, 0x00, 0x00, 0x00, 0x10, 0x00, 0x80, 0x00, 0x00, 0xAA, 0x00, 0x38, 0x9B, 0x71};

	uint_fast16_t format_tag;
	uint_fast16_t container_bytes;
	uint_fast16_t block_align;
	uint_fast16_t channels;
	uint_fast16_t bits_per_sample;

	if (fmt_sz < 16)
		return SMPLWAV_ERROR_FMT_INVALID;

	format_tag              = cop_ld_ule16(fmt_ptr + 0);
	channels                = cop_ld_ule16(fmt_ptr + 2);
	format->sample_rate     = cop_ld_ule32(fmt_ptr + 4);
	block_align             = cop_ld_ule16(fmt_ptr + 12);
	bits_per_sample         = cop_ld_ule16(fmt_ptr + 14);
	container_bytes         = (bits_per_sample + 7) / 8;

	if (format_tag == 0xFFFEu) {
		uint_fast16_t cbsz;

		if  (   (bits_per_sample % 8)
			||  (fmt_sz < 18)
			||  ((cbsz = cop_ld_ule16(fmt_ptr + 16)) < 22)
			||  (fmt_sz < 18 + cbsz)
			)
			return SMPLWAV_ERROR_FMT_INVALID;

		bits_per_sample = cop_ld_ule16(fmt_ptr + 18);
		format_tag      = cop_ld_ule16(fmt_ptr + 24);

		if (memcmp(fmt_ptr + 26, EXTENSIBLE_GUID_SUFFIX, 14) != 0)
			return SMPLWAV_ERROR_FMT_UNSUPPORTED;
	}

	format->bits_per_sample = bits_per_sample;
	format->channels = channels;

	if (format_tag == 1 && container_bytes == 2)
		format->format = SMPLWAV_FORMAT_PCM16;
	else if (format_tag == 1 && container_bytes == 3)
		format->format = SMPLWAV_FORMAT_PCM24;
	else if (format_tag == 1 && container_bytes == 4)
		format->format = SMPLWAV_FORMAT_PCM32;
	else if (format_tag == 3 && container_bytes == 4)
		format->format = SMPLWAV_FORMAT_FLOAT32; 
	else
		return SMPLWAV_ERROR_FMT_UNSUPPORTED;

	if  (   (block_align != channels * container_bytes)
		||  (bits_per_sample > container_bytes * 8)
		)
		return SMPLWAV_ERROR_FMT_INVALID;

	return 0;
}

static unsigned load_info(char **infoset, unsigned char *buf, size_t sz)
{
	unsigned warnings = 0;

	assert(sz >= 4);
	sz  -= 4;
	buf += 4;

	while (sz >= 8) {
		uint_fast32_t ckid = cop_ld_ule32(buf);
		uint_fast32_t cksz = cop_ld_ule32(buf + 4);
		char *base;
		unsigned i;

		sz   -= 8;
		buf  += 8;
		base  = (char *)buf;

		if (cksz >= sz) {
			cksz = (uint_fast32_t)sz; /* cast is safe as sz is less than cksz from the conditional */
			sz   = 0;
		} else {
			buf += cksz + (cksz & 1);
			sz  -= cksz + (cksz & 1);
		}

		if (cksz == 0 || base[cksz-1] != 0) {
			warnings |= SMPLWAV_WARNING_INFO_UNTERMINATED_STRINGS;
			continue;
		}

		for (i = 0; i < SMPLWAV_NB_INFO_TAGS; i++) {
#ifndef NDEBUG
			assert(SMPLWAV_INFO_ITEMS[i].index == i);
#endif
			if (SMPLWAV_INFO_ITEMS[i].fourccid == ckid) {
				infoset[i] = base;
				break;
			}
		}

		if (i == SMPLWAV_NB_INFO_TAGS)
			return SMPLWAV_ERROR_INFO_UNSUPPORTED;
	}

	return warnings;
}

unsigned smplwav_mount(struct smplwav *wav, unsigned char *buf, size_t bufsz, unsigned flags)
{
	uint_fast32_t riff_sz;
	unsigned warnings = 0;
	struct smplwav_extra_ck info;
	struct smplwav_extra_ck adtl;
	struct smplwav_extra_ck cue;
	struct smplwav_extra_ck smpl;
	struct smplwav_extra_ck fact;
	struct smplwav_extra_ck data;
	struct smplwav_extra_ck fmt;

	if  (   (bufsz < 12)
	    ||  (cop_ld_ule32(buf) != SMPLWAV_RIFF_ID('R', 'I', 'F', 'F'))
	    ||  ((riff_sz = cop_ld_ule32(buf + 4)) < 4)
	    ||  (cop_ld_ule32(buf + 8) != SMPLWAV_RIFF_ID('W', 'A', 'V', 'E'))
	    )
		return SMPLWAV_ERROR_NOT_A_WAVE;

	riff_sz -= 4;
	bufsz   -= 12;
	buf     += 12;

	if (riff_sz > bufsz) {
		warnings |= SMPLWAV_WARNING_FILE_TRUNCATION;
		riff_sz = (uint_fast32_t)bufsz;
	}

	wav->nb_unsupported = 0;
	wav->nb_marker = 0;
	wav->has_pitch_info = 0;
	wav->pitch_info = 0;
	memset(wav->info, 0, sizeof(wav->info));

	info.data = NULL;
	adtl.data = NULL;
	cue.data  = NULL;
	smpl.data = NULL;
	fact.data = NULL;
	data.data = NULL;
	fmt.data  = NULL;

	while (riff_sz >= 8) {
		uint_fast32_t            ckid           = cop_ld_ule32(buf);
		uint_fast32_t            cksz           = cop_ld_ule32(buf + 4);
		int                      required_chunk = 0;
		unsigned char           *ckbase         = buf + 8;
		struct smplwav_extra_ck *known_ptr      = NULL;

		/* Compute the amount of data left and move buf to point to the next
		 * chunk. We always truncate the length of the chunk if it would go
		 * beyond the length of the main RIFF body. */
		riff_sz -= 8;
		buf     += 8;
		if (cksz >= riff_sz) {
			cksz    = riff_sz;
			riff_sz = 0;
		} else {
			buf     += cksz + (cksz & 1);
			riff_sz -= cksz + (cksz & 1);
		}

		/* Figure out if this is a required chunk, a "known" chunk or if we
		 * don't know what the chunk is for. */
		if (ckid == SMPLWAV_RIFF_ID('L', 'I', 'S', 'T') && cksz >= 4) {
			switch (cop_ld_ule32(ckbase)) {
				case SMPLWAV_RIFF_ID('a', 'd', 't', 'l'): known_ptr = &adtl; break;
				case SMPLWAV_RIFF_ID('I', 'N', 'F', 'O'): known_ptr = &info; break;
				default: break;
			}
		} else {
			switch (ckid) {
				case SMPLWAV_RIFF_ID('d', 'a', 't', 'a'): known_ptr = &data; required_chunk = 1; break;
				case SMPLWAV_RIFF_ID('f', 'm', 't', ' '): known_ptr = &fmt;  required_chunk = 1; break;
				case SMPLWAV_RIFF_ID('f', 'a', 'c', 't'): known_ptr = &fact; required_chunk = 1; break;
				case SMPLWAV_RIFF_ID('c', 'u', 'e', ' '): known_ptr = &cue;  break;
				case SMPLWAV_RIFF_ID('s', 'm', 'p', 'l'): known_ptr = &smpl; break;
				default: break;
			}
		}

		/* If the chunk is required OR we know what the chunk is for and we
		 * are not resetting OR we don't know what the chunks is for but we
		 * for whatever reason want to preserve chunks we don't know how to
		 * handle: figure out where we need to place the information. Also,
		 * all the chunks which are "known" we do not support duplicates of,
		 * so figure out if that's happened here also. */ 
		if  (   required_chunk
		    ||  (known_ptr != NULL && !(flags & SMPLWAV_MOUNT_RESET))
		    ||  (known_ptr == NULL && (flags & SMPLWAV_MOUNT_PRESERVE_UNKNOWN))
		    ) {

			if (known_ptr != NULL) {
				/* There are no chunks which we know how to interpret which
				 * can occur more than once. */
				if (known_ptr->data != NULL)
					return warnings | SMPLWAV_ERROR_DUPLICATE_CHUNKS;
			} else {
				if (wav->nb_unsupported >= SMPLWAV_MAX_UNSUPPORTED_CHUNKS)
					return warnings | SMPLWAV_ERROR_TOO_MANY_CHUNKS;

				known_ptr = wav->unsupported + wav->nb_unsupported++;
			}

			known_ptr->id   = ckid;
			known_ptr->size = cksz;
			known_ptr->data = ckbase;
		}
	}

	if (fmt.data != NULL && data.data != NULL) {
		uint_fast16_t block_align;

		/* Load the format chunk into our "simple" format descriptor. */
		if (SMPLWAV_ERROR_CODE(warnings |= load_sample_format(&(wav->format), fmt.data, fmt.size)))
			return warnings;

		/* Compute the number of frames in the audio. */
		block_align      = (wav->format.channels * smplwav_format_container_size(wav->format.format));

		/* If the data chunk does not contain a whole multiple of frames, it
		 * is missing audio and we should probably (??) abort the load. */
		if (data.size % block_align)
			return warnings | SMPLWAV_ERROR_DATA_INVALID;

		wav->data        = data.data;
		wav->data_frames = data.size / block_align;
	} else {
		/* The wave file was missing the format or the data chunk. Totally
		 * broken. */
		return warnings | SMPLWAV_ERROR_NOT_A_WAVE;
	}

	if (info.data != NULL) {
		if (SMPLWAV_ERROR_CODE(warnings |= load_info(wav->info, info.data, info.size)))
			return warnings;
	}

	assert(wav->nb_marker == 0);

	if (adtl.data != NULL) {
		if (SMPLWAV_ERROR_CODE(warnings |= load_adtl(wav, adtl.data, adtl.size)))
			return warnings;
	}

	if (cue.data != NULL) {
		if (SMPLWAV_ERROR_CODE(warnings |= load_cue(wav, cue.data, cue.size)))
			return warnings;
	}

	if (smpl.data != NULL) {
		if (SMPLWAV_ERROR_CODE(warnings |= load_smpl(wav, smpl.data, smpl.size)))
			return warnings;
	}

	return warnings | check_and_finalise_markers(wav, flags);
}
