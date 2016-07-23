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

#ifndef SMPLWAV_CONVERT_H
#define SMPLWAV_CONVERT_H

#include "cop/cop_conversions.h"
#include "smplwav.h"

/* This method is provided as a courtesy as it is frequently desired when
 * sample data needs to be manipulated. The function interprets the "src"
 * pointer as containing "length" frames where a frame is contains
 * "nb_channels" of audio samples with the format specified in "input_format".
 *
 * The function will take these interleaved samples and channelise them into
 * "dest". "dest" must contain at least "dest_stride" * "nb_channels" floats
 * and "dest_stride" must be at least "length". For each channel, the elements
 * of "dest" between "length" and "dest_stride" will not be initialised. */
void
smplwav_convert_deinterleave_floats
	(float               *dest
	,size_t               dest_stride
	,const unsigned char *src
	,unsigned             length
	,unsigned             nb_channels
	,int                  input_format
	);

#endif /* SMPLWAV_CONVERT_H */
