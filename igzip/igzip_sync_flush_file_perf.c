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
#include <stdlib.h>
#include <assert.h>
#include "igzip_lib.h"
#include "test.h"

#define BUF_SIZE 1024
#define MIN_TEST_LOOPS   100
#ifndef RUN_MEM_SIZE
# define RUN_MEM_SIZE 500000000
#endif

struct isal_zstream stream;

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
	unsigned char *inbuf, *outbuf;
	int i, infile_size, iterations, outbuf_size;

	if (argc > 3 || argc < 2) {
		fprintf(stderr, "Usage: igzip_sync_flush_file_perf  infile [outfile]\n"
			"\t - Runs multiple iterations of igzip on a file to get more accurate time results.\n");
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
	printf("Window Size: %d K\n", IGZIP_HIST_SIZE / 1024);
	printf("igzip_sync_flush_file_perf: \n");
	fflush(0);
	/* Allocate space for entire input file and
	 * output (assuming 1:1 max output size)
	 */
	infile_size = get_filesize(in);

	if (infile_size != 0) {
		outbuf_size = infile_size;
		iterations = RUN_MEM_SIZE / infile_size;
	} else {
		outbuf_size = BUF_SIZE;
		iterations = MIN_TEST_LOOPS;
	}
	if (iterations < MIN_TEST_LOOPS)
		iterations = MIN_TEST_LOOPS;

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

	printf("igzip_sync_flush_file_perf: %s %d iterations\n", argv[1], iterations);
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
		stream.end_of_stream = 0;
		stream.flush = SYNC_FLUSH;
		stream.next_in = inbuf;
		stream.avail_in = infile_size / 2;
		stream.next_out = outbuf;
		stream.avail_out = outbuf_size / 2;
		isal_deflate(&stream);
		if (infile_size == 0)
			continue;
		stream.avail_in = infile_size - infile_size / 2;
		stream.end_of_stream = 1;
		stream.next_in = inbuf + stream.total_in;
		stream.flush = SYNC_FLUSH;
		stream.avail_out = infile_size - outbuf_size / 2;
		stream.next_out = outbuf + stream.total_out;
		isal_deflate(&stream);
		if (stream.avail_in != 0)
			break;
	}
	perf_stop(&stop);

	if (stream.avail_in != 0) {
		fprintf(stderr, "Could not compress all of inbuf\n");
		exit(0);
	}

	printf("  file %s - in_size=%d out_size=%d iter=%d ratio=%3.1f%%\n", argv[1],
	       infile_size, stream.total_out, i, 100.0 * stream.total_out / infile_size);

	printf("igzip_file: ");
	perf_print(stop, start, (long long)infile_size * i);

	if (argc > 2 && out) {
		printf("writing %s\n", argv[2]);
		fwrite(outbuf, 1, stream.total_out, out);
		fclose(out);
	}

	fclose(in);
	printf("End of igzip_sync_flush_file_perf\n\n");
	fflush(0);
	return 0;
}
