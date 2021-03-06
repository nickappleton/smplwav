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

#ifndef SMPLWAV_MOUNT_H
#define SMPLWAV_MOUNT_H

#include "smplwav.h"

/* Flags
 * -------------------------------------------------------------------------*/

/* This flag will remove all known but unnecessary metadata from the waveform
 * (cue, smpl, info and adtl chunks). */
#define SMPLWAV_MOUNT_RESET                (1u)

/* The default behavior is to strip out unknown metadata (i.e. on load,
 * nb_unsupported will always be zero). If this flag is set, unknown chunks
 * will populate the unsupported array in the smplwav structure. */
#define SMPLWAV_MOUNT_PRESERVE_UNKNOWN     (2u)

/* If there are loop conflicts, use the sampler loops over the cue loops (
 * beware!). */
#define SMPLWAV_MOUNT_PREFER_SMPL_LOOPS    (4u)

/* If there are loop conflicts, use the cue loops over the sampler loops (this
 * is less risky than the above). */
#define SMPLWAV_MOUNT_PREFER_CUE_LOOPS     (8u)

/* Error Codes
 * -------------------------------------------------------------------------*/

/* The data buffer supplied could not be identified as being waveform audio.
 * The load is aborted and wav is uninitialised. */
#define SMPLWAV_ERROR_NOT_A_WAVE              (1u)

/* The format chunk in the wave was corrupt. The load is aborted and wav is
 * uninitialised. */
#define SMPLWAV_ERROR_FMT_INVALID             (2u)

/* The format chunk is valid, but does not contain waveform audio which we can
 * use. The load is aborted and wav is uninitialised. */
#define SMPLWAV_ERROR_FMT_UNSUPPORTED         (3u)

/* The data chunk is corrupt. This happens if there is not a whole number of
 * sample frames in the data chunk. The load is aborted and wav is
 * uninitialised. */
#define SMPLWAV_ERROR_DATA_INVALID            (4u)

/* The waveform contained metadata in the INFO chunk which was not defined in
 * the RIFF spec. The load is aborted and wav is uninitialised. */
#define SMPLWAV_ERROR_INFO_UNSUPPORTED        (5u)

/* The adtl metadata chunk was invalid. This can happen if the adtl chunk has
 * been truncated or if a labelled text chunk is encountered which contained
 * unsupported data. The load is aborted and wav is uninitialised. */
#define SMPLWAV_ERROR_ADTL_INVALID            (6u)

/* The adtl metadata chunk contained duplicate note or labl entries for a
 * given cue chunk ID. The load is aborted and wav is uninitialised. */
#define SMPLWAV_ERROR_ADTL_DUPLICATES         (7u)

/* The cue chunk is invalid. This can happen if the cue chunk has been
 * truncated. The load is aborted and wav is uninitialised. */
#define SMPLWAV_ERROR_CUE_INVALID             (8u)

/* The cue chunk contained cue points which shared the same identifier. The
 * load is aborted and wav is uninitialised. */
#define SMPLWAV_ERROR_CUE_DUPLICATE_IDS       (9u)

/* The smpl chunk is invalid. This can happen if the smpl chunk has been
 * truncated. The load is aborted and wav is uninitialised. */
#define SMPLWAV_ERROR_SMPL_INVALID            (10u)

/* The wave file contained too many unsupported chunks (more than
 * WAV_SAMPLE_MAX_UNSUPPORTED_CHUNKS) to store in wav. The load is aborted and
 * wav is uninitialised. */
#define SMPLWAV_ERROR_TOO_MANY_CHUNKS         (11u)

/* The wave file contained unexpected duplicate chunks (as an example, more
 * than one format or data chunk). The load is aborted and wav is
 * uninitialised. */
#define SMPLWAV_ERROR_DUPLICATE_CHUNKS        (12u)

/* The wave file contained more than WAV_SAMPLE_MAX_MARKERS positional
 * metadata items. The load is aborted and wav is uninitialised. */
#define SMPLWAV_ERROR_TOO_MANY_MARKERS        (13u)

/* The wave file contained markers which contained samples outside the range
 * of the file. The load is aborted and wav is uninitialised. */
#define SMPLWAV_ERROR_MARKER_RANGE            (14u)

/* If the error code is this, there were loops that were in conflict within
 * the cue and smpl chunks. This usually indicates that audio editing software
 * updated one chunk without updating another (this has been verified to
 * happen with Adobe Audition which will not update loops stored in the
 * smpl chunk). This is the only error message where the markers in the output
 * can be examined - and this is only for diagnostic purposes. If a marker
 * has:
 *   in_cue && in_smpl && (length > 0):
 *     The loop has been reconciled and is the same in both chunks.
 *   !in_cue && in_smpl && (length > 0):
 *     This loop is in the sampler chunk but not in the cue chunk.
 *   in_cue && !in_smpl && (length > 0):
 *     This loop is in the cue chunk but not in the smpl chunk.
 * All other markers should be ignored. The SMPLWAV_MOUNT_PREFER options will
 * permit the load to continue selecting which items to preserve. */
#define SMPLWAV_ERROR_SMPL_CUE_LOOP_CONFLICTS (15u)

/* Use this macro to extract the error code from a return value. */
#define SMPLWAV_ERROR_CODE(x) ((x) & 0xFFu)

/* Warning Codes
 * -------------------------------------------------------------------------*/

/* This warning happens when the RIFF chunk was shortened because the supplied
 * memory was not long enough to contain it. This can happen when a broken
 * wave writing implementation is used to create the audio data. */
#define SMPLWAV_WARNING_FILE_TRUNCATION           (0x100u)

/* The adtl chunk contained strings which were not null-terminated. The
 * strings will be ignored. */
#define SMPLWAV_WARNING_ADTL_UNTERMINATED_STRINGS (0x200u)

/* The info chunk contained strings which were not null-terminated. The
 * strings will be ignored. */
#define SMPLWAV_WARNING_INFO_UNTERMINATED_STRINGS (0x400u)

/* See the description for SMPLWAV_ERROR_SMPL_CUE_LOOP_CONFLICTS. This flag
 * is set when one of the SMPLWAV_MOUNT_PREFER_* flags is given and the sample
 * was corrected. */
#define SMPLWAV_WARNING_SMPL_CUE_LOOP_CONFLICTS   (0x800u)

/* Sample Mounting API
 * -------------------------------------------------------------------------*/

/* This function populates the given wave structure with the metadata provided
 * in the given buffer (which is assumed to be a memory view of a wave file).
 *
 * Any information in the structure before the function is called is ignored
 * and will be overwritten.
 *
 * Pointers in the wav structure (which are always string pointers) will point
 * directly into the supplied buffer argument. This means that the lifetime
 * of the pointers in wav is the same as the lifetime of the buf argument and
 * modifications to buf may modify data being pointed to. Be careful of this
 * if you intend on using the smplwav_serialise() API - you must NOT use the
 * same buffer for serialisation!
 *
 * Although the buffer is not marked const (which is deliberate), the buffer
 * will NOT be modified. The reason for this is that the pointers in the wav
 * structure are not const (as we you want to write to them).
 *
 * No memory is dynamically allocated by this function and there is nothing
 * to cleanup on failure or success.
 *
 * Flags may be any combination of the SMPLWAV_MOUNT_* values with one
 * exception. If SMPLWAV_MOUNT_PREFER_SMPL_LOOPS is specified,
 * SMPLWAV_MOUNT_PREFER_CUE_LOOPS must not be specified and vice versa.
 *
 * The return value is an error code and possibly a warning bitfield. Use
 * SMPLWAV_ERROR_CODE() to extract the error code from the return value - if
 * the value is non-zero, something went wrong and the wav structure should be
 * treated as uninitialised unless the documentation for the error code
 * explicitly states otherwise. If the extracted error code is zero but the
 * return value was non-zero, one or more compromises were made to load the
 * wav structure which may or may-not be a problem. The warnings are a mask
 * of SMPLWAV_WARNING_* flags in the returned value. */
unsigned smplwav_mount(struct smplwav *wav, unsigned char *buf, size_t bufsz, unsigned flags);

#endif /* SMPLWAV_MOUNT_H */
