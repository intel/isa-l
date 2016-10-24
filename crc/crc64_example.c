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
#include "crc64.h"

#define BUF_SIZE 8192
#define INIT_SEED 0x12345678

int main(int argc, char *argv[])
{
	uint8_t inbuf[BUF_SIZE];
	uint64_t avail_in, total_in = 0;
	uint64_t crc64_checksum;
	FILE *in;

	if (argc != 2) {
		fprintf(stderr, "Usage: crc64_example infile\n");
		exit(0);
	}
	in = fopen(argv[1], "rb");
	if (!in) {
		fprintf(stderr, "Can't open %s for reading\n", argv[1]);
		exit(0);
	}

	printf("crc64_example -- crc64_ecma_refl:\n");
	fflush(0);

	crc64_checksum = INIT_SEED;
	while ((avail_in = fread(inbuf, 1, BUF_SIZE, in))) {
		// crc update mode
		crc64_checksum = crc64_ecma_refl(crc64_checksum, inbuf, avail_in);
		total_in += avail_in;
	}

	fclose(in);
	printf("total length is %ld, checksum is 0x%lx\n", total_in, crc64_checksum);

	return 0;
}
