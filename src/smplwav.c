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

#include "smplwav/smplwav.h"

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

