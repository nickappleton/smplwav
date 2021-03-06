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

#ifndef SMPLWAV_SERIALISE_H
#define SMPLWAV_SERIALISE_H

#include "smplwav.h"

/* Serialises the data in wav into the given buffer. If buf is NULL, no data
 * will be written. The size argument will be updated to reflect how much
 * data will be/was written into the buffer as long as the function did not
 * fail.
 *
 * If store_cue_loops is non-zero, cue points and labeled text chunks for the
 * loops will be written.
 *
 * The unsupported chunks in the wav structure will always be written - set
 * the list to NULL to suppress writing them.
 *
 * It is undefined for any of the values in the wav_sample structure to be
 * invalid.
 *
 * The function returns zero on success or non-zero if the wave is impossible
 * to serialise (a chunk size would have exceeded the hard 32-bit limit). */
int smplwav_serialise(const struct smplwav *wav, unsigned char *buf, size_t *size, int store_cue_loops);

#endif /* SMPLWAV_SERIALISE_H */
