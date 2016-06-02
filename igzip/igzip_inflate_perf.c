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

#include <stdio.h>
#include <assert.h>
#include <zlib.h>
#include "huff_codes.h"
#include "igzip_inflate_ref.h"
#include "test.h"

#define BUF_SIZE 1024
#define MIN_TEST_LOOPS   100
#ifndef RUN_MEM_SIZE
# define RUN_MEM_SIZE 1000000000
#endif

int get_filesize(FILE * f)
{
	int curr, end;

	curr = ftell(f);	/* Save current position */
	fseek(f, 0L, SEEK_END);
	end = ftell(f);
	fseek(f, curr, SEEK_SET);	/* Restore position */
	return end;
}

int main(int argc, char *argv[])
{
	FILE *in, *out = NULL;
	unsigned char *inbuf, *outbuf, *tempbuf;
	int i, infile_size, iterations, outbuf_size, check;
	uint64_t inbuf_size;
	struct inflate_state state;

	if (argc > 3 || argc < 2) {
		fprintf(stderr, "Usage: igzip_inflate_file_perf  infile\n"
			"\t - Runs multiple iterations of igzip on a file to "
			"get more accurate time results.\n");
		exit(0);
	}
	in = fopen(argv[1], "rb");
	if (!in) {
		fprintf(stderr, "Can't open %s for reading\n", argv[1]);
		exit(0);
	}
	if (argc > 2) {
		out = fopen(argv[2], "wb");
		if (!out) {
			fprintf(stderr, "Can't open %s for writing\n", argv[2]);
			exit(0);
		}
		printf("outfile=%s\n", argv[2]);
	}
	printf("igzip_inflate_perf: \n");
	fflush(0);
	/* Allocate space for entire input file and output
	 * (assuming some possible expansion on output size)
	 */
	infile_size = get_filesize(in);

	if (infile_size != 0) {
		outbuf_size = infile_size;
		iterations = RUN_MEM_SIZE / infile_size;
	} else {
		printf("Error: input file has 0 size\n");
		exit(0);
	}
	if (iterations < MIN_TEST_LOOPS)
		iterations = MIN_TEST_LOOPS;

	tempbuf = malloc(infile_size);
	if (tempbuf == NULL) {
		fprintf(stderr, "Can't allocate temp buffer memory\n");
		exit(0);
	}
	inbuf_size = compressBound(infile_size);
	inbuf = malloc(inbuf_size);
	if (inbuf == NULL) {
		fprintf(stderr, "Can't allocate input buffer memory\n");
		exit(0);
	}
	outbuf = malloc(infile_size);
	if (outbuf == NULL) {
		fprintf(stderr, "Can't allocate output buffer memory\n");
		exit(0);
	}
	fread(tempbuf, 1, infile_size, in);
	i = compress2(inbuf, &inbuf_size, tempbuf, infile_size, 9);
	if (i != Z_OK) {
		printf("Compression of input file failed\n");
		exit(0);
	}
	printf("igzip_inflate_perf: %s %d iterations\n", argv[1], iterations);
	/* Read complete input file into buffer */
	fclose(in);
	struct perf start, stop;
	perf_start(&start);

	for (i = 0; i < iterations; i++) {
		igzip_inflate_init(&state, inbuf + 2, inbuf_size - 2, outbuf, outbuf_size);

		check = igzip_inflate(&state);
		if (check) {
			printf("Error in decompression with error %d\n", check);
			break;
		}
	}
	perf_stop(&stop);

	printf("  file %s - in_size=%d out_size=%d iter=%d\n", argv[1],
	       infile_size, state.out_buffer.total_out, i);

	printf("igzip_file: ");
	perf_print(stop, start, (long long)infile_size * i);

	printf("End of igzip_inflate_perf\n\n");
	fflush(0);

	free(inbuf);
	free(outbuf);
	free(tempbuf);

	return 0;
}
