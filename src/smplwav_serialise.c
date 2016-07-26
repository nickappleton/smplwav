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

#include "smplwav/smplwav_serialise.h"
#include "smplwav_internal.h"
#include "cop/cop_conversions.h"
#include <string.h>
#include <assert.h>

static void serialise_ltxt(unsigned char *buf, uint_fast64_t *size, uint_fast32_t id, uint_fast32_t length)
{
	if (buf != NULL) {
		buf += *size;
		cop_st_ule32(buf + 0, SMPLWAV_RIFF_ID('l', 't', 'x', 't'));
		cop_st_ule32(buf + 4, 20);
		cop_st_ule32(buf + 8, id);
		cop_st_ule32(buf + 12, length);
		cop_st_ule32(buf + 16, SMPLWAV_RIFF_ID('r', 'g', 'n', ' '));
		cop_st_ule16(buf + 20, 0);
		cop_st_ule16(buf + 22, 0);
		cop_st_ule16(buf + 24, 0);
		cop_st_ule16(buf + 26, 0);
	}
	*size += 28;
}

static int serialise_notelabl(unsigned char *buf, uint_fast64_t *size, uint_fast32_t ctyp, uint_fast32_t id, const char *s)
{
	size_t len = strlen(s);
	if (len++ > 0xFFFFFFFF - 5)
		return 1; /* can not write the chunk size. */
	if (buf != NULL) {
		buf += *size;
		cop_st_ule32(buf + 0, ctyp);
		cop_st_ule32(buf + 4, (uint_fast32_t)(4 + len)); /* cast is safe because of the previous check. */
		cop_st_ule32(buf + 8, id);
		memcpy(buf + 12, s, len);
		if (len & 1)
			(buf + 12)[len] = 0;
	}
	*size += ((uint_fast64_t)len) + 12 + (len & 1);
	return 0;
}

static int serialise_adtl(const struct smplwav *wav, unsigned char *buf, uint_fast64_t *size, int store_cue_loops)
{
	uint_fast64_t old_sz = *size;
	uint_fast64_t new_sz = old_sz + 12;
	unsigned i;

	/* Serialise any metadata that might exist. */
	for (i = 0; i < wav->nb_marker; i++) {
		if (store_cue_loops)
			serialise_ltxt(buf, &new_sz, i + 1, wav->markers[i].length);
		if (wav->markers[i].name != NULL && serialise_notelabl(buf, &new_sz, SMPLWAV_RIFF_ID('l', 'a', 'b', 'l'), i + 1, wav->markers[i].name))
			return 1;
		if (wav->markers[i].desc != NULL && serialise_notelabl(buf, &new_sz, SMPLWAV_RIFF_ID('n', 'o', 't', 'e'), i + 1, wav->markers[i].desc))
			return 1;
		if (new_sz - old_sz - 8 > 0xFFFFFFFF)
			return 1;
	}

	/* Only bother serialising if there were actually metadata items
	 * written. */
	if (new_sz != old_sz + 12) {
		if (buf != NULL) {
			buf += old_sz;
			cop_st_ule32(buf + 0, SMPLWAV_RIFF_ID('L', 'I', 'S', 'T'));
			cop_st_ule32(buf + 4, (uint_fast32_t)(new_sz - old_sz - 8));
			cop_st_ule32(buf + 8, SMPLWAV_RIFF_ID('a', 'd', 't', 'l'));
		}
		*size = new_sz;
	}

	return 0;
}

static int serialise_cue(const struct smplwav *wav, unsigned char *buf, uint_fast64_t *size, int store_cue_loops)
{
	unsigned i;
	unsigned nb_cue = 0;
	uint_fast64_t cksz;

	if (buf != NULL)
		buf += *size;

	for (i = 0; i < wav->nb_marker; i++) {
		if (store_cue_loops || wav->markers[i].length == 0) {
			if (buf != NULL) {
				cop_st_ule32(buf + 12 + nb_cue * 24, i + 1);
				cop_st_ule32(buf + 16 + nb_cue * 24, 0);
				cop_st_ule32(buf + 20 + nb_cue * 24, SMPLWAV_RIFF_ID('d', 'a', 't', 'a'));
				cop_st_ule32(buf + 24 + nb_cue * 24, 0);
				cop_st_ule32(buf + 28 + nb_cue * 24, 0);
				cop_st_ule32(buf + 32 + nb_cue * 24, wav->markers[i].position);
			}
			nb_cue++;
		}
	}

	cksz = nb_cue;
	cksz = (cksz * 24) + 4;
	if (cksz > 0xFFFFFFFF)
		return 1;

	if (nb_cue) {
		if (buf != NULL) {
			cop_st_ule32(buf + 0, SMPLWAV_RIFF_ID('c', 'u', 'e', ' '));
			cop_st_ule32(buf + 4, (uint_fast32_t)cksz); /* cast is safe due to previous check. */
			cop_st_ule32(buf + 8, nb_cue);
		}
		*size += cksz + 8;
	}

	return 0;
}

static int serialise_smpl(const struct smplwav *wav, unsigned char *buf, uint_fast64_t *size)
{
	unsigned i;
	unsigned nb_loop = 0;
	uint_fast64_t cksz;

	if (buf != NULL)
		buf += *size;

	for (i = 0; i < wav->nb_marker; i++) {
		if (wav->markers[i].length > 0) {
			if (buf != NULL) {
				cop_st_ule32(buf + 44 + 24 * nb_loop, i + 1);
				cop_st_ule32(buf + 48 + 24 * nb_loop, 0);
				cop_st_ule32(buf + 52 + 24 * nb_loop, wav->markers[i].position);
				cop_st_ule32(buf + 56 + 24 * nb_loop, wav->markers[i].position + wav->markers[i].length - 1);
				cop_st_ule32(buf + 60 + 24 * nb_loop, 0);
				cop_st_ule32(buf + 64 + 24 * nb_loop, 0);
			}
			nb_loop++;
		}
	}

	cksz = nb_loop;
	cksz = (cksz * 24) + 36;
	if (cksz > 0xFFFFFFFF)
		return 1;

	if (nb_loop || wav->has_pitch_info) {
		if (buf != NULL) {
			cop_st_ule32(buf + 0, SMPLWAV_RIFF_ID('s', 'm', 'p', 'l'));
			cop_st_ule32(buf + 4, (uint_fast32_t)cksz); /* cast is safe due to previous check. */
			cop_st_ule32(buf + 8, 0);
			cop_st_ule32(buf + 12, 0);
			cop_st_ule32(buf + 16, 0);
			cop_st_ule32(buf + 20, ((uint_fast32_t)((wav->pitch_info) >> 32)) & 0xFFFFFFFFu);
			cop_st_ule32(buf + 24, ((uint_fast32_t)wav->pitch_info) & 0xFFFFFFFFu);
			cop_st_ule32(buf + 28, 0);
			cop_st_ule32(buf + 32, 0);
			cop_st_ule32(buf + 36, nb_loop);
			cop_st_ule32(buf + 40, 0);
		}
		*size += cksz + 8;
	}

	return 0;
}

static int serialise_format(const struct smplwav_format *fmt, unsigned char *buf, uint_fast64_t *size)
{
	static const unsigned char EXTENSIBLE_GUID_SUFFIX[14] = {/* AA, BB, */ 0x00, 0x00, 0x00, 0x00, 0x10, 0x00, 0x80, 0x00, 0x00, 0xAA, 0x00, 0x38, 0x9B, 0x71};
	int           format_code      = fmt->format;
	uint_fast16_t container_size   = smplwav_format_container_size(format_code);
	uint_fast16_t container_bits   = container_size * 8;
	uint_fast16_t bits_per_sample  = fmt->bits_per_sample;
	int           extensible       = container_bits != bits_per_sample;
	uint_fast16_t basic_format_tag = (format_code == SMPLWAV_FORMAT_FLOAT32) ? 0x0003u : 0x0001u;
	uint_fast16_t format_tag       = (extensible) ? 0xFFFEu : basic_format_tag;
	uint_fast32_t fmt_sz           = (extensible) ? 48 : ((basic_format_tag == 1) ? 24 : 26);

	if (buf != NULL) {
		uint_fast16_t channels    = fmt->channels;
		uint_fast32_t sample_rate = fmt->sample_rate;
		uint_fast16_t block_align = container_size * channels;

		buf += *size;

		cop_st_ule32(buf + 0, SMPLWAV_RIFF_ID('f', 'm', 't', ' '));
		cop_st_ule32(buf + 4, fmt_sz - 8);
		cop_st_ule16(buf + 8, format_tag);
		cop_st_ule16(buf + 10, channels);
		cop_st_ule32(buf + 12, sample_rate);
		cop_st_ule32(buf + 16, sample_rate * block_align);
		cop_st_ule16(buf + 20, block_align);
		cop_st_ule16(buf + 22, container_bits);
		if (extensible || basic_format_tag != 1) {
			cop_st_ule16(buf + 24, fmt_sz - 26);
		}
		if (extensible) {
			cop_st_ule16(buf + 26, bits_per_sample);
			cop_st_ule32(buf + 28, 0);
			cop_st_ule16(buf + 32, basic_format_tag);
			memcpy(buf + 34, EXTENSIBLE_GUID_SUFFIX, 14);
		}
	}

	*size += fmt_sz;

	return (format_tag != 1);
}

static void serialise_fact(uint_fast32_t data_frames, unsigned char *buf, uint_fast64_t *size)
{
	if (buf != NULL) {
		buf += *size;
		cop_st_ule32(buf,     SMPLWAV_RIFF_ID('f', 'a', 'c', 't'));
		cop_st_ule32(buf + 4, 4);
		cop_st_ule32(buf + 8, data_frames);
	}
	*size += 12;
}

static void serialise_blob(uint_fast32_t id, const unsigned char *ckdata, uint_fast64_t cksize, unsigned char *buf, uint_fast64_t *size)
{
	assert(cksize < 0xFFFFFFFF);
	if (buf != NULL) {
		buf += *size;
		cop_st_ule32(buf, id);
		cop_st_ule32(buf + 4, (uint_fast32_t)cksize);
		memcpy(buf + 8, ckdata, (size_t)cksize);
		if (cksize & 1)
			buf[8+cksize] = 0;
	}
	*size += cksize + 8 + (cksize & 1);
}

static int serialise_data(const struct smplwav_format *format, void *data, uint_fast32_t data_frames, unsigned char *buf, uint_fast64_t *size)
{
	uint_fast16_t container_size = smplwav_format_container_size(format->format);
	uint_fast16_t block_align = container_size * format->channels;
	uint_fast64_t data_size = ((uint_fast64_t)data_frames) * block_align;
	if (data_size > 0xFFFFFFFF)
		return 1;
	serialise_blob(SMPLWAV_RIFF_ID('d', 'a', 't', 'a'), data, data_size, buf, size);
	return 0;
}

static int serialise_zstrblob(uint_fast32_t id, const char *value, unsigned char *buf, uint_fast64_t *size)
{
	size_t len;
	if (value != NULL && (len = strlen(value)) > 0) {
		if (len > 0xFFFFFFFF - 1)
			return 1;
		serialise_blob(id, (const unsigned char *)value, ((uint_fast64_t)len) + 1, buf, size);
	}
	return 0;
}

static int serialise_info(char * const *infoset, unsigned char *buf, uint_fast64_t *size)
{
	uint_fast64_t old_sz = *size;
	uint_fast64_t new_sz = old_sz + 12;
	unsigned i;

	for (i = 0; i < SMPLWAV_NB_INFO_TAGS; i++) {
#ifndef NDEBUG
		assert(SMPLWAV_INFO_ITEMS[i].index == i);
#endif
		if (serialise_zstrblob(SMPLWAV_INFO_ITEMS[i].fourccid, infoset[i], buf, &new_sz))
			return 1;
		if (new_sz - old_sz - 8 > 0xFFFFFFFF)
			return 1;
	}

	/* Only bother serialising if there were actually metadata items
	 * written. */
	if (new_sz != old_sz + 12) {
		if (buf != NULL) {
			buf += old_sz;
			cop_st_ule32(buf + 0, SMPLWAV_RIFF_ID('L', 'I', 'S', 'T'));
			cop_st_ule32(buf + 4, (uint_fast32_t)(new_sz - old_sz - 8));
			cop_st_ule32(buf + 8, SMPLWAV_RIFF_ID('I', 'N', 'F', 'O'));
		}
		*size = new_sz;
	}

	return 0;
}

int smplwav_serialise(const struct smplwav *wav, unsigned char *buf, size_t *size, int store_cue_loops)
{
	unsigned i;
	uint_fast64_t sz = 12;

	if (serialise_info(wav->info, buf, &sz))
		return 1;

	if (serialise_format(&wav->format, buf, &sz))
		serialise_fact(wav->data_frames, buf, &sz);

	if  (   serialise_data(&wav->format, wav->data, wav->data_frames, buf, &sz)
	    ||  serialise_adtl(wav, buf, &sz, store_cue_loops)
	    ||  serialise_cue(wav, buf, &sz, store_cue_loops)
	    ||  serialise_smpl(wav, buf, &sz)
	    )
		return 1;

	for (i = 0; i < wav->nb_unsupported; i++) {
		if (sz - 8 > 0xFFFFFFFF || wav->unsupported[i].size > 0xFFFFFFFF)
			return 1;
		serialise_blob(wav->unsupported[i].id, wav->unsupported[i].data, wav->unsupported[i].size, buf, &sz);
	}

	if (sz - 8 > 0xFFFFFFFF || sz > SIZE_MAX)
		return 1;

	if (buf != NULL) {
		cop_st_ule32(buf + 0, SMPLWAV_RIFF_ID('R', 'I', 'F', 'F'));
		cop_st_ule32(buf + 4, (uint_fast32_t)(sz - 8));
		cop_st_ule32(buf + 8, SMPLWAV_RIFF_ID('W', 'A', 'V', 'E'));
	}

	*size = (size_t)sz;
	return 0;
}
