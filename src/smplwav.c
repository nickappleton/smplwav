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

#ifndef NDEBUG
#define MAKE_ITEM(fcc_, c1_, c2_, c3_, c4_) {SMPLWAV_INFO_ ## fcc_, SMPLWAV_RIFF_ID(c1_, c2_, c3_, c4_), {c1_, c2_, c3_, c4_, '\0'}}
#else
#define MAKE_ITEM(fcc_, c1_, c2_, c3_, c4_) {                       SMPLWAV_RIFF_ID(c1_, c2_, c3_, c4_), {c1_, c2_, c3_, c4_, '\0'}}
#endif

const struct smplwav_info_item SMPLWAV_INFO_ITEMS[SMPLWAV_NB_INFO_TAGS] =
{	MAKE_ITEM(IARL, 'I', 'A', 'R', 'L')
,	MAKE_ITEM(IART, 'I', 'A', 'R', 'T')
,	MAKE_ITEM(ICMS, 'I', 'C', 'M', 'S')
,	MAKE_ITEM(ICMT, 'I', 'C', 'M', 'T')
,	MAKE_ITEM(ICOP, 'I', 'C', 'O', 'P')
,	MAKE_ITEM(ICRD, 'I', 'C', 'R', 'D')
,	MAKE_ITEM(ICRP, 'I', 'C', 'R', 'P')
,	MAKE_ITEM(IDIM, 'I', 'D', 'I', 'M')
,	MAKE_ITEM(IDPI, 'I', 'D', 'P', 'I')
,	MAKE_ITEM(IENG, 'I', 'E', 'N', 'G')
,	MAKE_ITEM(IGNR, 'I', 'G', 'N', 'R')
,	MAKE_ITEM(IKEY, 'I', 'K', 'E', 'Y')
,	MAKE_ITEM(ILGT, 'I', 'L', 'G', 'T')
,	MAKE_ITEM(IMED, 'I', 'M', 'E', 'D')
,	MAKE_ITEM(INAM, 'I', 'N', 'A', 'M')
,	MAKE_ITEM(IPLT, 'I', 'P', 'L', 'T')
,	MAKE_ITEM(IPRD, 'I', 'P', 'R', 'D')
,	MAKE_ITEM(ISBJ, 'I', 'S', 'B', 'J')
,	MAKE_ITEM(ISFT, 'I', 'S', 'F', 'T')
,	MAKE_ITEM(ISHP, 'I', 'S', 'H', 'P')
,	MAKE_ITEM(ISRC, 'I', 'S', 'R', 'C')
,	MAKE_ITEM(ISRF, 'I', 'S', 'R', 'F')
,	MAKE_ITEM(ITCH, 'I', 'T', 'C', 'H')
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
	id = SMPLWAV_RIFF_ID(fourcc[0], fourcc[1], fourcc[2], fourcc[3]);
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


