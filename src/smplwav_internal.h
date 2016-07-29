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

#ifndef SMPLWAV_INTERNAL_H
#define SMPLWAV_INTERNAL_H

#include "smplwav/smplwav.h"

struct smplwav_info_item {
#ifndef NDEBUG
	int            index;
#endif
	uint_fast32_t  fourccid;
	char           fourccstr[5];
};

extern const struct smplwav_info_item SMPLWAV_INFO_ITEMS[SMPLWAV_NB_INFO_TAGS];

#define SMPLWAV_RIFF_ID(c1, c2, c3, c4) \
	(   ((uint_fast32_t)(c1)) \
	|   (((uint_fast32_t)(c2)) << 8) \
	|   (((uint_fast32_t)(c3)) << 16) \
	|   (((uint_fast32_t)(c4)) << 24) \
	)

#endif /* SMPLWAV_INTERNAL_H */
