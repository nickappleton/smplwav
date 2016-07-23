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

#include <stdlib.h>
#include "smplwav/smplwav_convert.h"

void smplwav_convert_deinterleave_floats(float *dest, size_t dest_stride, const void *src, unsigned length, unsigned nb_channels, int input_format)
{
	if (input_format == SMPLWAV_FORMAT_PCM16)
	{
		if (nb_channels == 2) {
			unsigned j;
			float *out1 = dest;
			float *out2 = out1 + dest_stride;
			for (j = 0; j < length; j++) {
				int_fast32_t s1;
				int_fast32_t s2;
				s1 =             ((const unsigned char *)src)[4*j+1];
				s1 = (s1 << 8) | ((const unsigned char *)src)[4*j+0];
				s2 =             ((const unsigned char *)src)[4*j+3];
				s2 = (s2 << 8) | ((const unsigned char *)src)[4*j+2];
				if (s1 >= 32768)
					s1 -= 65536;
				if (s2 >= 32768)
					s2 -= 65536;
				out1[j] = s1 * (256.0f / (float)0x800000);
				out2[j] = s2 * (256.0f / (float)0x800000);
			}
		} else {
			unsigned i;
			float *out = dest;
			for (i = 0; i < nb_channels; i++, out += dest_stride) {
				unsigned j;
				for (j = 0; j < length; j++) {
					uint_fast32_t s;
					int_fast32_t t;
					s =            ((const unsigned char *)src)[2*(i+nb_channels*j)+1];
					s = (s << 8) | ((const unsigned char *)src)[2*(i+nb_channels*j)+0];
					t = (s & 0x8000u)   ? -(int_fast32_t)(((~s) & 0x7FFFu) + 1) : (int_fast32_t)s;
					out[j] = t * (256.0f / (float)0x800000);
				}
			}
		}
	}
	else if (input_format == SMPLWAV_FORMAT_PCM24)
	{
		unsigned i;
		if (nb_channels == 2) {
			float *out1 = dest;
			float *out2 = out1 + dest_stride;
			for (i = 0; i < length; i++, src += 6) {
				int_fast32_t t1 = cop_ld_sle24(((const unsigned char *)src) + 0);
				int_fast32_t t2 = cop_ld_sle24(((const unsigned char *)src) + 3);
				out1[i]         = t1 * (1.0f / (float)0x800000);
				out2[i]         = t2 * (1.0f / (float)0x800000);
			}
		} else {
			float *out = dest;
			for (i = 0; i < nb_channels; i++, out += dest_stride) {
				unsigned j;
				for (j = 0; j < length; j++) {
					out[j] = cop_ld_sle24(((const unsigned char *)src) + 3 * (i + nb_channels * j)) * (1.0f / (float)0x800000);
				}
			}
		}
	}
	else
	{
		abort();
	}
}

