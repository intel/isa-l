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
#include <string.h>
#include <assert.h>
#include "igzip_lib.h"
#include "test.h"

#define TEST_LEN   (1024*1024)
#define IBUF_SIZE  (1024*1024)
#define OBUF_SIZE  (1024*1024)

#define TEST_LOOPS   400
#define TEST_TYPE_STR "_warm"

void create_data(unsigned char *data, int size)
{
	char c = 'a';
	while (size--)
		*data++ = c = c < 'z' ? c + 1 : 'a';
}

int main(int argc, char *argv[])
{
	int i = 1;
	struct isal_zstream stream;
	unsigned char inbuf[IBUF_SIZE], zbuf[OBUF_SIZE];

	printf("Window Size: %d K\n", IGZIP_HIST_SIZE / 1024);
	printf("igzip_perf: \n");
	fflush(0);
	create_data(inbuf, TEST_LEN);

	struct perf start, stop;
	perf_start(&start);

	for (i = 0; i < TEST_LOOPS; i++) {
		isal_deflate_init(&stream);

		stream.avail_in = TEST_LEN;
		stream.end_of_stream = 1;
		stream.next_in = inbuf;
		stream.flush = NO_FLUSH;

		do {
			stream.avail_out = OBUF_SIZE;
			stream.next_out = zbuf;
			isal_deflate(&stream);
		} while (stream.avail_out == 0);
	}

	perf_stop(&stop);

	printf("igzip" TEST_TYPE_STR ": ");
	perf_print(stop, start, (long long)TEST_LEN * i);

	if (!stream.end_of_stream) {
		printf("error: compression test could not fit into allocated buffers\n");
		return -1;
	}
	printf("End of igzip_perf\n\n");
	fflush(0);
	return 0;
}
