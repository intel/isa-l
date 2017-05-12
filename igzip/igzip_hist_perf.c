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
#include "igzip_lib.h"
#include "test.h"

#define BUF_SIZE 1024
#define MIN_TEST_LOOPS   8
#ifndef RUN_MEM_SIZE
# define RUN_MEM_SIZE 2000000000
#endif

void print_histogram(struct isal_huff_histogram *histogram)
{
	int i;
	printf("Lit Len histogram");
	for (i = 0; i < ISAL_DEF_LIT_LEN_SYMBOLS; i++) {
		if (i % 16 == 0)
			printf("\n");
		else
			printf(", ");
		printf("%4lu", histogram->lit_len_histogram[i]);
	}
	printf("\n");

	printf("Dist histogram");
	for (i = 0; i < ISAL_DEF_DIST_SYMBOLS; i++) {
		if (i % 16 == 0)
			printf("\n");
		else
			printf(", ");
		printf("%4lu", histogram->dist_histogram[i]);
	}
	printf("\n");
}

int main(int argc, char *argv[])
{
	FILE *in;
	unsigned char *inbuf, *outbuf;
	int i, iterations, avail_in;
	uint64_t infile_size, outbuf_size;
	struct isal_huff_histogram histogram1, histogram2;

	memset(&histogram1, 0, sizeof(histogram1));
	memset(&histogram2, 0, sizeof(histogram2));

	if (argc > 3 || argc < 2) {
		fprintf(stderr, "Usage: igzip_file_perf  infile [outfile]\n"
			"\t - Runs multiple iterations of igzip on a file to "
			"get more accurate time results.\n");
		exit(0);
	}
	in = fopen(argv[1], "rb");
	if (!in) {
		fprintf(stderr, "Can't open %s for reading\n", argv[1]);
		exit(0);
	}

	/* Allocate space for entire input file and output
	 * (assuming some possible expansion on output size)
	 */
	infile_size = get_filesize(in);
	outbuf_size = 2 * infile_size;

	if (infile_size != 0)
		iterations = RUN_MEM_SIZE / infile_size;
	else
		iterations = MIN_TEST_LOOPS;

	if (iterations < MIN_TEST_LOOPS)
		iterations = MIN_TEST_LOOPS;

	inbuf = malloc(infile_size);
	outbuf = malloc(outbuf_size);
	if (inbuf == NULL) {
		fprintf(stderr, "Can't allocate input buffer memory\n");
		exit(0);
	}

	if (outbuf == NULL) {
		fprintf(stderr, "Can't allocate output buffer memory\n");
		exit(0);
	}

	avail_in = fread(inbuf, 1, infile_size, in);
	if (avail_in != infile_size) {
		fprintf(stderr, "Couldn't fit all of input file into buffer\n");
		exit(0);
	}

	struct perf start, stop;
	perf_start(&start);

	for (i = 0; i < iterations; i++)
		isal_update_histogram(inbuf, infile_size, &histogram1);
	perf_stop(&stop);

	printf("  file %s - in_size=%lu iter=%d\n", argv[1], infile_size, i);
	printf("igzip_file: ");
	perf_print(stop, start, (long long)infile_size * i);

	fclose(in);
	fflush(0);
	return 0;
}
