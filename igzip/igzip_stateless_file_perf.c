/**********************************************************************
  Copyright(c) 2011-2016 Intel Corporation All rights reserved.

  Redistribution and use in source and binary forms, with or without
  modification, are permitted provided that the following conditions
  are met:
    * Redistributions of source code must retain the above copyright
      notice, this list of conditions and the following disclaimer.
    * Redistributions in binary form must reproduce the above copyright
      notice, this list of conditions and the following disclaimer in
      the documentation and/or other materials provided with the
      distribution.
    * Neither the name of Intel Corporation nor the names of its
      contributors may be used to endorse or promote products derived
      from this software without specific prior written permission.

  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
  "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
  LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
  A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
  OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
  SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
  LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
  DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
  THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
  (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
  OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
**********************************************************************/

#define _FILE_OFFSET_BITS 64
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <getopt.h>
#include "igzip_lib.h"
#include "test.h"

#define BUF_SIZE 1024
#define MIN_TEST_LOOPS   10
#ifndef RUN_MEM_SIZE
# define RUN_MEM_SIZE 500000000
#endif

int level_size_buf[10] = {
#ifdef ISAL_DEF_LVL0_DEFAULT
	ISAL_DEF_LVL0_DEFAULT,
#else
	0,
#endif
#ifdef ISAL_DEF_LVL1_DEFAULT
	ISAL_DEF_LVL1_DEFAULT,
#else
	0,
#endif
#ifdef ISAL_DEF_LVL2_DEFAULT
	ISAL_DEF_LVL2_DEFAULT,
#else
	0,
#endif
#ifdef ISAL_DEF_LVL3_DEFAULT
	ISAL_DEF_LVL3_DEFAULT,
#else
	0,
#endif
#ifdef ISAL_DEF_LVL4_DEFAULT
	ISAL_DEF_LVL4_DEFAULT,
#else
	0,
#endif
#ifdef ISAL_DEF_LVL5_DEFAULT
	ISAL_DEF_LVL5_DEFAULT,
#else
	0,
#endif
#ifdef ISAL_DEF_LVL6_DEFAULT
	ISAL_DEF_LVL6_DEFAULT,
#else
	0,
#endif
#ifdef ISAL_DEF_LVL7_DEFAULT
	ISAL_DEF_LVL7_DEFAULT,
#else
	0,
#endif
#ifdef ISAL_DEF_LVL8_DEFAULT
	ISAL_DEF_LVL8_DEFAULT,
#else
	0,
#endif
#ifdef ISAL_DEF_LVL9_DEFAULT
	ISAL_DEF_LVL9_DEFAULT,
#else
	0,
#endif
};

struct isal_zstream stream;

int usage(void)
{
	fprintf(stderr,
		"Usage: igzip_stateless_file_perf [options] <infile>\n"
		"  -h        help\n"
		"  -X        use compression level X with 0 <= X <= 2\n"
		"  -i <iter> number of iterations (at least 1)\n"
		"  -o <file> output file for compresed data\n");
	exit(0);
}

int main(int argc, char *argv[])
{
	FILE *in = NULL, *out = NULL;
	unsigned char *inbuf, *outbuf, *level_buf = NULL;
	int i, c, iterations = 0;
	uint64_t infile_size, outbuf_size;
	struct isal_huff_histogram histogram;
	struct isal_hufftables hufftables_custom;
	int level = 0, level_size = 0;
	char *in_file_name = NULL, *out_file_name = NULL;

	while ((c = getopt(argc, argv, "h0123456789i:o:")) != -1) {
		if (c >= '0' && c <= '9') {
			if (c > '0' + ISAL_DEF_MAX_LEVEL)
				usage();
			else {
				level = c - '0';
				level_size = level_size_buf[level];
			}
			continue;
		}

		switch (c) {
		case 'o':
			out_file_name = optarg;
			break;
		case 'i':
			iterations = atoi(optarg);
			if (iterations < 1)
				usage();
			break;
		case 'h':
		default:
			usage();
			break;
		}
	}

	if (optind < argc) {
		in_file_name = argv[optind];
		in = fopen(in_file_name, "rb");
	} else
		usage();

	if (!in) {
		fprintf(stderr, "Can't open %s for reading\n", in_file_name);
		exit(0);
	}
	if (out_file_name != NULL) {
		out = fopen(out_file_name, "wb");
		if (!out) {
			fprintf(stderr, "Can't open %s for writing\n", out_file_name);
			exit(0);
		}
		printf("outfile=%s\n", out_file_name);
	}

	printf("Window Size: %d K\n", IGZIP_HIST_SIZE / 1024);
	printf("igzip_file_perf: \n");
	fflush(0);
	/* Allocate space for entire input file and output
	 * (assuming some possible expansion on output size)
	 */
	infile_size = get_filesize(in);

	outbuf_size = infile_size * 1.07 + BUF_SIZE;

	if (iterations == 0) {
		iterations = infile_size ? RUN_MEM_SIZE / infile_size : MIN_TEST_LOOPS;
		if (iterations < MIN_TEST_LOOPS)
			iterations = MIN_TEST_LOOPS;
	}

	inbuf = malloc(infile_size);
	if (inbuf == NULL) {
		fprintf(stderr, "Can't allocate input buffer memory\n");
		exit(0);
	}
	outbuf = malloc(outbuf_size);
	if (outbuf == NULL) {
		fprintf(stderr, "Can't allocate output buffer memory\n");
		exit(0);
	}

	if (level_size != 0) {
		level_buf = malloc(level_size);
		if (level_buf == NULL) {
			fprintf(stderr, "Can't allocate level buffer memory\n");
			exit(0);
		}
	}

	printf("igzip_file_perf: %s %d iterations\n", in_file_name, iterations);
	/* Read complete input file into buffer */
	stream.avail_in = (uint32_t) fread(inbuf, 1, infile_size, in);
	if (stream.avail_in != infile_size) {
		fprintf(stderr, "Couldn't fit all of input file into buffer\n");
		exit(0);
	}

	struct perf start, stop;
	perf_start(&start);

	for (i = 0; i < iterations; i++) {
		isal_deflate_init(&stream);
		stream.end_of_stream = 1;	/* Do the entire file at once */
		stream.flush = NO_FLUSH;
		stream.next_in = inbuf;
		stream.avail_in = infile_size;
		stream.next_out = outbuf;
		stream.avail_out = outbuf_size;
		stream.level = level;
		stream.level_buf = level_buf;
		stream.level_buf_size = level_size;
		isal_deflate_stateless(&stream);
		if (stream.avail_in != 0)
			break;
	}
	perf_stop(&stop);

	if (stream.avail_in != 0) {
		fprintf(stderr, "Could not compress all of inbuf\n");
		exit(0);
	}

	printf("  file %s - in_size=%lu out_size=%d iter=%d ratio=%3.1f%%", in_file_name,
	       infile_size, stream.total_out, i, 100.0 * stream.total_out / infile_size);

	if (level == 0) {
		memset(&histogram, 0, sizeof(histogram));

		isal_update_histogram(inbuf, infile_size, &histogram);
		isal_create_hufftables(&hufftables_custom, &histogram);

		isal_deflate_init(&stream);
		stream.end_of_stream = 1;	/* Do the entire file at once */
		stream.flush = NO_FLUSH;
		stream.next_in = inbuf;
		stream.avail_in = infile_size;
		stream.next_out = outbuf;
		stream.avail_out = outbuf_size;
		stream.level = level;
		stream.level_buf = level_buf;
		stream.level_buf_size = level_size;
		stream.hufftables = &hufftables_custom;
		isal_deflate_stateless(&stream);

		printf(" ratio_custom=%3.1f%%", 100.0 * stream.total_out / infile_size);
	}
	printf("\n");

	printf("igzip_file: ");
	perf_print(stop, start, (long long)infile_size * i);

	if (argc > 2 && out) {
		printf("writing %s\n", out_file_name);
		fwrite(outbuf, 1, stream.total_out, out);
		fclose(out);
	}

	fclose(in);
	printf("End of igzip_file_perf\n\n");
	fflush(0);
	return 0;
}
