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

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include "cop/cop_filemap.h"
#include "smplwav/smplwav_mount.h"
#include "smplwav/smplwav_serialise.h"

#define MAX_SET_ITEMS             (32)

#define FLAG_STRIP_EVENT_METADATA (1)
#define FLAG_WRITE_CUE_LOOPS      (2)
#define FLAG_OUTPUT_METADATA      (4)
#define FLAG_INPUT_METADATA       (8)

struct wavauth_options {
	const char  *input_filename;
	const char  *output_filename;

	unsigned     flags;
	unsigned     smplwav_flags;

	unsigned     nb_set_items;
	char        *set_items[MAX_SET_ITEMS];
};

static int handle_options(struct wavauth_options *opts, char **argv, unsigned argc)
{
	int output_inplace = 0;

	opts->input_filename  = NULL;
	opts->output_filename = NULL;
	opts->flags           = 0;
	opts->smplwav_flags   = 0;
	opts->nb_set_items    = 0;

	while (argc) {

		if (!strcmp(*argv, "--reset")) {
			argv++;
			argc--;
			opts->smplwav_flags |= SMPLWAV_MOUNT_RESET;
		} else if (!strcmp(*argv, "--preserve-unknown-chunks")) {
			argv++;
			argc--;
			opts->smplwav_flags |= SMPLWAV_MOUNT_PRESERVE_UNKNOWN;
		} else if (!strcmp(*argv, "--prefer-smpl-loops")) {
			argv++;
			argc--;
			opts->smplwav_flags |= SMPLWAV_MOUNT_PREFER_SMPL_LOOPS;
		} else if (!strcmp(*argv, "--prefer-cue-loops")) {
			argv++;
			argc--;
			opts->smplwav_flags |= SMPLWAV_MOUNT_PREFER_CUE_LOOPS;
		} else if (!strcmp(*argv, "--strip-event-metadata")) {
			argv++;
			argc--;
			opts->flags |= FLAG_STRIP_EVENT_METADATA;
		} else if (!strcmp(*argv, "--write-cue-loops")) {
			argv++;
			argc--;
			opts->flags |= FLAG_WRITE_CUE_LOOPS;
		} else if (!strcmp(*argv, "--output-metadata")) {
			argv++;
			argc--;
			opts->flags |= FLAG_OUTPUT_METADATA;
		} else if (!strcmp(*argv, "--input-metadata")) {
			argv++;
			argc--;
			opts->flags |= FLAG_INPUT_METADATA;
		} else if (!strcmp(*argv, "--output-inplace")) {
			argv++;
			argc--;
			output_inplace = 1;
		} else if (!strcmp(*argv, "--set")) {
			argv++;
			argc--;
			if (!argc) {
				fprintf(stderr, "--set requires an argument.\n");
				return -1;
			}
			if (opts->nb_set_items >= MAX_SET_ITEMS) {
				fprintf(stderr, "too many --set options\n");
				return -1;
			}
			opts->set_items[opts->nb_set_items++] = *argv;
			argv++;
			argc--;
		} else if (!strcmp(*argv, "--output")) {
			argv++;
			argc--;
			if (!argc) {
				fprintf(stderr, "--output requires an argument.\n");
				return -1;
			}
			opts->output_filename = *argv;
			argv++;
			argc--;
		} else if (opts->input_filename == NULL) {
			opts->input_filename = *argv;
			argv++;
			argc--;
		} else {
			fprintf(stderr, "cannot set input file '%s'. already set to '%s'.\n", *argv, opts->input_filename);
			return -1;
		}
	}

	if ((opts->smplwav_flags & (SMPLWAV_MOUNT_PREFER_CUE_LOOPS | SMPLWAV_MOUNT_PREFER_SMPL_LOOPS)) == (SMPLWAV_MOUNT_PREFER_CUE_LOOPS | SMPLWAV_MOUNT_PREFER_SMPL_LOOPS)) {
		fprintf(stderr, "--prefer-smpl-loops and --prefer-cue-loops are exclusive options\n");
		return -1;
	}

	if (opts->input_filename == NULL) {
		fprintf(stderr, "a wave filename must be specified.\n");
		return -1;
	}

	if (output_inplace) {
		if (opts->output_filename != NULL) {
			fprintf(stderr, "--output cannot be specified with --output-inplace.\n");
			return -1;
		}
		opts->output_filename = opts->input_filename;
	}


	return 0;
}

void printstr(const char *s)
{
	if (s != NULL) {
		printf("\"");
		while (*s != '\0') {
			if (*s == '\"') {
				printf("\\\"");
			} else if (*s == '\\') {
				printf("\\\\");
			} else if (*s == '\r') {
				printf("\\r");
			} else if (*s == '\n') {
				printf("\\n");
			} else {
				printf("%c", *s);
			}
			s++;
		}
		printf("\"");
	} else {
		printf("null");
	}
}

static void dump_metadata(const struct smplwav *wav)
{
	unsigned i;

	for (i = 0; i < SMPLWAV_NB_INFO_TAGS; i++) {
		if (wav->info[i] != NULL) {
			printf("info-%s ", smplwav_info_index_to_string(i));
			printstr(wav->info[i]);
			printf("\n");
		}
	}

	if (wav->has_pitch_info)
		printf("smpl-pitch %llu\n", wav->pitch_info);
	for (i = 0; i < wav->nb_marker; i++) {
		assert(wav->markers[i].in_cue || wav->markers[i].in_smpl);
		if (wav->markers[i].length > 0) {
			printf("loop %u %u ", wav->markers[i].position, wav->markers[i].length);
			printstr(wav->markers[i].name);
			printf(" ");
			printstr(wav->markers[i].desc);
		} else {
			printf("cue %u ", wav->markers[i].position);
			printstr(wav->markers[i].name);
			printf(" ");
			printstr(wav->markers[i].desc);
		}
		printf("\n");
	}
}

void *serialise_sample(const struct smplwav *wav, size_t *xsz, int store_cue_loops) {
	unsigned char *data;
	size_t         sz;
	int            err;

	/* Find size of entire wave file then allocate memory for it. */
	if (smplwav_serialise(wav, NULL, &sz, store_cue_loops)) {
		fprintf(stderr, "can not serialise the updated waveform\n");
		return NULL;
	}
	if ((data = malloc(sz)) == NULL) {
		fprintf(stderr, "out of memory\n");
		return NULL;
	}

	/* Serialise the wave file to memory. */
	err = smplwav_serialise(wav, data, xsz, store_cue_loops);

	/* Serialise should always be successful if the size query was successful
	* and the returned size should be identical to what was queried. */
	assert(err == 0); (void)err;
	assert(sz == *xsz);
	return data;
}

static int is_whitespace(char c)
{
	return (c == ' ') || (c == '\t') || (c == '\n') || (c == '\r');
}

static char *handle_identifier(char **cmd_str)
{
	char *s = *cmd_str;
	char *t = s;

	if (*t == '\0' || is_whitespace(*t))
		return NULL;

	do {
		t++;
	} while (*t != '\0' && !is_whitespace(*t));

	if (*t != '\0') {
		*t++ = '\0';
	}
	*cmd_str = t;

	return s;
}

static void eat_whitespace(char **cmd_str)
{
	char *s = *cmd_str;
	while (is_whitespace(*s))
		s++;
	*cmd_str = s;
}

static int expect_whitespace(char **cmd_str)
{
	char *s = *cmd_str;
	if (!is_whitespace(*s))
		return -1;
	while (is_whitespace(*++s));
	*cmd_str = s;
	return 0;
}

static int expect_string(char **output_str, char **cmd_str)
{
	char *s       = *cmd_str;
	char c;
	char *ret_ptr;

	if (*s != '\"')
		return -1;

	ret_ptr     = s;
	c           = *++s;
	*output_str = ret_ptr;

	while (c != '\0' && c != '\"') {
		if (c != '\\') {
			*ret_ptr++ = c;
			c = *++s;
			continue;
		}

		c = *++s;

		switch (c) {
			case '\"':
			case '\\':
				break;
			case 'n':
				c = '\n';
				break;
			case 'r':
				c = '\r';
				break;
			case '\0':
			default:
				return -1;
		}

		*ret_ptr++ = c;
		c = *++s;
	}

	if (c != '\"')
		return -1;

	s++;
	*ret_ptr = '\0';
	*cmd_str = s;
	return 0;
}

static int expect_null(char **cmd_str)
{
	char *s = *cmd_str;
	if (s[0] != 'n' || s[1] != 'u' || s[2] != 'l' || s[3] != 'l') {
		return -1;
	}
	*cmd_str = s + 4;
	return 0;
}

static int expect_null_or_str(char **str, char **cmd_str)
{
	char *s = *cmd_str;
	int err;
	*str     = NULL;
	if (*s == '\"')
		err = expect_string(str, &s);
	else
		err = expect_null(&s);
	if (err) {
		fprintf(stderr, "expected a quoted string or 'null'\n");
		return -1;
	}
	*cmd_str = s;
	return 0;
}

static int expect_int(uint_fast64_t *ival, char **cmd_str)
{
	char *s = *cmd_str;
	uint_fast64_t rv = 0;
	if (*s < '0' || *s > '9')
		return -1;
	do {
		rv = rv * 10 + (*s++ - '0');
	} while (*s >= '0' && *s <= '9');
	*ival    = rv;
	*cmd_str = s;
	return 0;
}

static int expect_end_of_args(char **cmd_str)
{
	char *s = *cmd_str;
	eat_whitespace(&s);
	if (*s != '\0')
		return -1;
	*cmd_str = s;
	return 0;
}

static int handle_loop(struct smplwav *wav, char *cmd_str)
{
	uint_fast64_t start;
	uint_fast64_t duration;
	char *name;
	char *desc;
	if  (   expect_int(&start, &cmd_str)
	    ||  expect_whitespace(&cmd_str)
	    ||  expect_int(&duration, &cmd_str)
	    ||  expect_whitespace(&cmd_str)
	    ||  expect_null_or_str(&name, &cmd_str)
	    ||  expect_whitespace(&cmd_str)
	    ||  expect_null_or_str(&desc, &cmd_str)
	    ||  expect_end_of_args(&cmd_str)
	    ) {
		fprintf(stderr, "loop command expects two integer arguments followed by two string or null arguments\n");
		return -1;
	}

	if (duration == 0) {
		fprintf(stderr, "cannot add a loop of zero duration\n");
		return -1;
	}

	if (start > wav->data_frames) {
		fprintf(stderr, "the start of the loop was beyond the end of the sample\n");
		return -1;
	}

	if (duration > 0xFFFFFFFF || start + duration > wav->data_frames) {
		fprintf(stderr, "the loop duration went beyond the end of the sample\n");
		return -1;
	}

	if (wav->nb_marker >= SMPLWAV_MAX_MARKERS) {
		fprintf(stderr, "cannot add another loop - too much marker metadata\n");
		return -1;
	}

	wav->markers[wav->nb_marker].name       = name;
	wav->markers[wav->nb_marker].desc       = desc;
	wav->markers[wav->nb_marker].length     = (uint_fast32_t)duration;
	wav->markers[wav->nb_marker].position   = (uint_fast32_t)start;
	wav->nb_marker++;

	return 0;
}

static int handle_cue(struct smplwav *wav, char *cmd_str)
{
	uint_fast64_t start;
	char *name;
	char *desc;

	if  (   expect_int(&start, &cmd_str)
	    ||  expect_whitespace(&cmd_str)
	    ||  expect_null_or_str(&name, &cmd_str)
	    ||  expect_whitespace(&cmd_str)
	    ||  expect_null_or_str(&desc, &cmd_str)
	    ||  expect_end_of_args(&cmd_str)
	    ) {
		fprintf(stderr, "cue command expects one integer arguments followed by two string or null arguments\n");
		return -1;
	}

	if (start > wav->data_frames) {
		fprintf(stderr, "the cue marker position was beyond the end of the sample\n");
		return -1;
	}

	if (wav->nb_marker >= SMPLWAV_MAX_MARKERS) {
		fprintf(stderr, "cannot add another loop - too much marker metadata\n");
		return -1;
	}

	wav->markers[wav->nb_marker].name       = name;
	wav->markers[wav->nb_marker].desc       = desc;
	wav->markers[wav->nb_marker].length     = 0;
	wav->markers[wav->nb_marker].position   = (uint_fast32_t)start;
	wav->nb_marker++;

	return 0;
}

static int handle_smplpitch(struct smplwav *wav, char *cmd_str)
{
	uint_fast64_t pitch;
	int set_pitch = expect_null(&cmd_str);
	int err       = set_pitch && expect_int(&pitch, &cmd_str);
	if (err || expect_end_of_args(&cmd_str)) {
		fprintf(stderr, "smpl-pitch command expects one integer or null argument\n");
		return -1;
	}
	wav->has_pitch_info = set_pitch;
	wav->pitch_info = (set_pitch) ? pitch : 0;
	return 0;
}

static int handle_info(struct smplwav *wav, char *ck, char *cmd_str)
{
	int idx;
	if (strlen(ck) != 4 || (idx = smplwav_info_string_to_index(ck)) < 0) {
		fprintf(stderr, "'%s' is an unsupported INFO chunk\n", ck);
		return -1;
	}
	if (expect_null_or_str(&(wav->info[idx]), &cmd_str) || expect_end_of_args(&cmd_str)) {
		fprintf(stderr, "info commands requires exactly one string or 'null' argument\n");
		return -1;
	}
	return 0;
}

static int handle_metastring(struct smplwav *wav, char *cmd_str)
{
	char *metastring = cmd_str;
	char *command;
	
	eat_whitespace(&cmd_str);

	if ((command = handle_identifier(&cmd_str)) == NULL) {
		fprintf(stderr, "could not parse meta string '%s'\n", metastring);
		return -1;
	}

	eat_whitespace(&cmd_str);

	if (!memcmp(command, "info-", 5))
		return handle_info(wav, command+5, cmd_str);
	if (!strcmp(command, "loop"))
		return handle_loop(wav, cmd_str);
	if (!strcmp(command, "cue"))
		return handle_cue(wav, cmd_str);
	if (!strcmp(command, "smpl-pitch"))
		return handle_smplpitch(wav, cmd_str);

	fprintf(stderr, "Unknown set command: '%s'\n", command);
	return -1;
}

void print_usage(FILE *f, const char *pname)
{
	fprintf(f, "Usage:\n  %s\n", pname);
	fprintf(f, "    [ \"--output-inplace\" | ( \"--output\" ( filename ) ) ]\n");
	fprintf(f, "    [ \"--output-metadata\" ] [ \"--reset\" ] [ \"--write-cue-loops\" ]\n");
	fprintf(f, "    [ \"--prefer-cue-loops\" | \"--prefer-smpl-loops\" ]\n");
	fprintf(f, "    [ \"--strip-event-metadata\" ] ( sample filename )\n\n");
	fprintf(f, "This tool is used to modify or repair the metadata associated with a sample. It\n");
	fprintf(f, "operates according to the following flow:\n");
	fprintf(f, "1) The sample is loaded. If \"--reset\" is specified, all known chunks which are\n");
	fprintf(f, "   not required for the sample to be considered waveform audio will be deleted.\n");
	fprintf(f, "   Chunks which are not known are always deleted unless the\n");
	fprintf(f, "   \"--preserve-unknown-chunks\" flag is specified. The known and required chunks\n");
	fprintf(f, "   are 'fmt ', 'fact' and 'data'. The known and unrequired chunks are 'INFO',\n");
	fprintf(f, "   'adtl', 'smpl', 'cue '.\n");
	fprintf(f, "2) The 'smpl', 'cue ' and 'adtl' chunks (if any exist) will be parsed to obtain\n");
	fprintf(f, "   loop and release markers. Tools and audio editors which manipulate these\n");
	fprintf(f, "   chunks have proven to occasionally corrupt the data in them. This tool uses\n");
	fprintf(f, "   some (safe) heuristics to correct these issues. There is one issue which\n");
	fprintf(f, "   cannot be corrected automatically: when there are loops in both the cue and\n");
	fprintf(f, "   smpl chunks which do not match. When this happens, the default behavior is to\n");
	fprintf(f, "   abort the load process and terminate with an error message which details what\n");
	fprintf(f, "   the different loops are. If the \"--prefer-cue-loops\" flag is given, loops\n");
	fprintf(f, "   will be taken from the cue chunk. If the \"--prefer-smpl-loops\" flag is\n");
	fprintf(f, "   specified, loops will be taken from the smpl chunk. These two flags only have\n");
	fprintf(f, "   an effect when there is actually an unresolvable issue. i.e. specifying\n");
	fprintf(f, "   \"--prefer-cue-loops\" will not remove loops from the smpl chunk if there are\n");
	fprintf(f, "   no loops in the cue chunk.\n");
	fprintf(f, "3) If \"--strip-event-metadata\" is specified, any *textual* metadata which is\n");
	fprintf(f, "   associated with loops or cue points will be deleted.\n");
	fprintf(f, "4) If \"--input-metadata\" is specified, lines will be read from stdin and will\n");
	fprintf(f, "   be treated as if each one were passed to the \"--set\" option (see below).\n");
	fprintf(f, "5) The \"--set\" argument may be supplied multiple times to add or replace\n");
	fprintf(f, "   metadata elements in the sample. A set string is a command followed by one\n");
	fprintf(f, "   or more whitespace separated parameters. Parameters may be quoted. The\n");
	fprintf(f, "   following commands exist:\n");
	fprintf(f, "     loop ( start sample ) ( duration ) ( name ) ( description )\n");
	fprintf(f, "       Add a loop to the sample. duration must be at least 1. Name or\n");
	fprintf(f, "       description may be \"null\".\n");
	fprintf(f, "     cue ( sample ) ( name ) ( description )\n");
	fprintf(f, "       Add a cue point to the sample. Name or description may be \"null\".\n");
	fprintf(f, "     smpl-pitch ( smpl pitch )\n");
	fprintf(f, "       Store pitch information in sampler chunk. The value is the MIDI note\n");
	fprintf(f, "       multiplied by 2^32. This is to deal with the way the value is stored in\n");
	fprintf(f, "       the smpl chunk. The argument may be \"null\" to remove the pitch\n");
	fprintf(f, "       information (this has no effect if the sample contains loops).\n");
	fprintf(f, "     info-XXXX [ string ]\n");
	fprintf(f, "       Store string in the RIFF INFO chunk where XXXX is the ID of the info\n");
	fprintf(f, "       key. See the RIFF MCI spec for a list of keys. Some include:\n");
	fprintf(f, "         info-IARL   Archival location.\n");
	fprintf(f, "         info-IART   Artist.\n");
	fprintf(f, "         info-ICOP   Copyright information.\n");
	fprintf(f, "       The argument may be \"null\" to remove the metadata item.\n");
	fprintf(f, "6) If \"--output-metadata\" is specified, the metadata which has been loaded and\n");
	fprintf(f, "   potentially modified will be dumped to stdout in a format which can be used\n");
	fprintf(f, "   by \"--input-metadata\".\n");
	fprintf(f, "7) If \"--output-inplace\" is specified, the input file will be re-written with\n");
	fprintf(f, "   the updated metadata. Otherwise if \"--output\" is given, the output file will\n");
	fprintf(f, "   be written to the specified filename. These flags cannot both be specified\n");
	fprintf(f, "   simultaneously. The default behavior is that loops will only be written to\n");
	fprintf(f, "   the smpl chunk and markers will only be written to the cue chunk as this is\n");
	fprintf(f, "   the most compatible form. If \"--write-cue-loops\" is specified, loops will\n");
	fprintf(f, "   also be stored in the cue chunk. This may assist in checking them in editor\n");
	fprintf(f, "   software.\n\n");
	fprintf(f, "Examples:\n");
	fprintf(f, "   %s --reset sample.wav --output-inplace\n", pname);
	fprintf(f, "   Removes all non-essential wave chunks from sample.wav and overwrites the\n");
	fprintf(f, "   existing file.\n\n");
	fprintf(f, "   %s in.wav --output-metadata | grep '^smpl-pitch' | %s dest.wav --input-metadata --output-inplace\n", pname, pname);
	fprintf(f, "   Copy the pitch information from in.wav into dest.wav.\n\n");
}

#define STDIN_READ_BUFSZ (1024)

int main(int argc, char *argv[])
{
	struct wavauth_options opts;
	int err;
	unsigned uerr;
	struct cop_filemap infile;
	struct smplwav wav;
	unsigned i;
	char *stdinbuf = NULL;
	size_t stdinbufsz = 0;
	size_t stdinbufpos = 0;
	unsigned char *out_data = NULL;
	size_t         out_data_sz;

	if (argc < 2) {
		print_usage(stdout, argv[0]);
		return 0;
	}

	if ((err = handle_options(&opts, argv + 1, argc - 1)) != 0)
		return err;

	if ((err = cop_filemap_open(&infile, opts.input_filename, COP_FILEMAP_FLAG_R)) != 0) {
		fprintf(stderr, "could not open %s\n", opts.input_filename);
		return err;
	}

	if (SMPLWAV_ERROR_CODE(uerr = smplwav_mount(&wav, infile.ptr, infile.size, opts.smplwav_flags))) {
		if (SMPLWAV_ERROR_CODE(uerr) == SMPLWAV_ERROR_SMPL_CUE_LOOP_CONFLICTS) {
			fprintf(stderr, "%s has sampler loops that conflict with loops in the cue chunk. you must specify --prefer-smpl-loops or --prefer-cue-loops to load it. here are the details:\n", opts.input_filename);
			fprintf(stderr, "common loops (position/duration):\n");
			for (i = 0; i < wav.nb_marker; i++)
				if (wav.markers[i].in_cue && wav.markers[i].in_smpl && wav.markers[i].length > 0)
					fprintf(stderr, "  %lu/%lu\n", (unsigned long)wav.markers[i].position, (unsigned long)wav.markers[i].length);
			fprintf(stderr, "sampler loops (position/duration):\n");
			for (i = 0; i < wav.nb_marker; i++)
				if (!wav.markers[i].in_cue && wav.markers[i].in_smpl && wav.markers[i].length > 0)
					fprintf(stderr, "  %lu/%lu\n", (unsigned long)wav.markers[i].position, (unsigned long)wav.markers[i].length);
			fprintf(stderr, "cue loops (position/duration):\n");
			for (i = 0; i < wav.nb_marker; i++)
				if (wav.markers[i].in_cue && !wav.markers[i].in_smpl && wav.markers[i].length > 0)
					fprintf(stderr, "  %lu/%lu\n", (unsigned long)wav.markers[i].position, (unsigned long)wav.markers[i].length);
		} else {
			fprintf(stderr, "failed to load '%s' sample: %u\n", opts.input_filename, uerr);
		}

		return -1;
	}

	if (opts.flags & FLAG_STRIP_EVENT_METADATA) {
		for (i = 0; i < wav.nb_marker; i++) {
			wav.markers[i].name = NULL;
			wav.markers[i].desc = NULL;
		}
	}

	if (err == 0 && (opts.flags & FLAG_INPUT_METADATA)) {
		size_t nread;
		while (err == 0 && !feof(stdin)) {
			if (stdinbufpos + STDIN_READ_BUFSZ + 1 > stdinbufsz) {
				char *nb;
				stdinbufsz = stdinbufpos + 2*STDIN_READ_BUFSZ + 1;
				nb = realloc(stdinbuf, stdinbufsz);
				if (nb == NULL) {
					fprintf(stderr, "out of memory\n");
					err = -1;
				}
				stdinbuf = nb;
			}
			if (!err) {
				nread = fread(stdinbuf + stdinbufpos, 1, STDIN_READ_BUFSZ, stdin);
				stdinbufpos += nread;
				if (ferror(stdin)) {
					fprintf(stderr, "error reading from stdin\n");
					err = -1;
				}
			}
		}

		if (err == 0 && stdinbufpos > 0) {
			stdinbuf[stdinbufpos] = '\0';
			stdinbufpos = 0;
			while (err == 0) {
				size_t epos = stdinbufpos;
				while (stdinbuf[epos] != '\0' && stdinbuf[epos] != '\r' && stdinbuf[epos] != '\n')
					epos++;
				if (stdinbuf[epos] == '\0') {
					if (epos - stdinbufpos > 0)
						err = handle_metastring(&wav, stdinbuf + stdinbufpos);
					break;
				}
				if (epos - stdinbufpos > 0) {
					stdinbuf[epos] = '\0';
					err = handle_metastring(&wav, stdinbuf + stdinbufpos);
				}
				stdinbufpos = epos + 1;
			}
		}
	}

	for (i = 0; err == 0 && i < opts.nb_set_items; i++) {
		err = handle_metastring(&wav, opts.set_items[i]);
	}

	if (err == 0) {
		smplwav_sort_markers(&wav);
		if (opts.flags & FLAG_OUTPUT_METADATA)
			dump_metadata(&wav);
		if (opts.output_filename != NULL) {
			out_data = serialise_sample(&wav, &out_data_sz, (opts.flags & FLAG_WRITE_CUE_LOOPS) == FLAG_WRITE_CUE_LOOPS);
			if (out_data == NULL)
				err = -1;
		}
	}

	if (stdinbuf != NULL)
		free(stdinbuf);

	cop_filemap_close(&infile);

	/* Must dump data after closing the filemap - this could be operating in-place. */
	if (out_data != NULL) {
		assert(opts.output_filename != NULL);
		if (err == 0 && cop_file_dump(opts.output_filename, out_data, out_data_sz)) {
			fprintf(stderr, "could not write to file %s\n", opts.output_filename);
			err = -1;
		}
		free(out_data);
	}

	return err;
}
