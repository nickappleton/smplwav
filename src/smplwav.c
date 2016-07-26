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

#include "smplwav_internal.h"

#include <string.h>

#define SMPLWAV_RIFF_IDSTR(s_) SMPLWAV_RIFF_ID(s_[0], s_[1], s_[2], s_[3])

#ifndef NDEBUG
#define MAKE_ITEM(fcc_) {SMPLWAV_INFO_ ## fcc_, #fcc_, SMPLWAV_RIFF_IDSTR(#fcc_)}
#else
#define MAKE_ITEM(fcc_) {#fcc_, SMPLWAV_RIFF_IDSTR(#fcc_)}
#endif

const struct smplwav_info_item SMPLWAV_INFO_ITEMS[SMPLWAV_NB_INFO_TAGS] =
{	MAKE_ITEM(IARL)
,	MAKE_ITEM(IART)
,	MAKE_ITEM(ICMS)
,	MAKE_ITEM(ICMT)
,	MAKE_ITEM(ICOP)
,	MAKE_ITEM(ICRD)
,	MAKE_ITEM(ICRP)
,	MAKE_ITEM(IDIM)
,	MAKE_ITEM(IDPI)
,	MAKE_ITEM(IENG)
,	MAKE_ITEM(IGNR)
,	MAKE_ITEM(IKEY)
,	MAKE_ITEM(ILGT)
,	MAKE_ITEM(IMED)
,	MAKE_ITEM(INAM)
,	MAKE_ITEM(IPLT)
,	MAKE_ITEM(IPRD)
,	MAKE_ITEM(ISBJ)
,	MAKE_ITEM(ISFT)
,	MAKE_ITEM(ISHP)
,	MAKE_ITEM(ISRC)
,	MAKE_ITEM(ISRF)
,	MAKE_ITEM(ITCH)
};

const char *smplwav_info_index_to_string(unsigned index)
{
	assert(index < SMPLWAV_NB_INFO_TAGS);
#ifndef NDEBUG
	assert(SMPLWAV_INFO_ITEMS[index].index == index);
#endif
	return SMPLWAV_INFO_ITEMS[index].fourccstr;
}

int smplwav_info_string_to_index(const char *fourcc)
{
	unsigned i;
	uint_fast32_t id;
	assert(strlen(fourcc) == 4);
	id = SMPLWAV_RIFF_IDSTR(fourcc);
	for (i = 0; i < SMPLWAV_NB_INFO_TAGS; i++) {
#ifndef NDEBUG
		assert(SMPLWAV_INFO_ITEMS[i].index == i);
#endif
		if (id == SMPLWAV_INFO_ITEMS[i].fourccid)
			return i;
	}
	return -1;
}

void smplwav_sort_markers(struct smplwav *wav)
{
	unsigned i;
	for (i = 1; i < wav->nb_marker; i++) {
		unsigned j;
		for (j = i; j < wav->nb_marker; j++) {
			int i_loop = wav->markers[i-1].length > 0;
			int j_loop = wav->markers[j].length > 0;
			uint_fast64_t i_key = (((uint_fast64_t)wav->markers[i-1].position) << 32) | (wav->markers[i-1].length ^ 0xFFFFFFFF);
			uint_fast64_t j_key = (((uint_fast64_t)wav->markers[j].position) << 32) | (wav->markers[j].length ^ 0xFFFFFFFF);
			if ((j_loop && !i_loop) || (j_loop == i_loop && j_key < i_key)) {
				struct smplwav_marker m = wav->markers[i-1];
				wav->markers[i-1] = wav->markers[j];
				wav->markers[j] = m;
			}
		}
		wav->markers[i-1].id = i;
	}
}


