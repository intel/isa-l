/**********************************************************************
  Copyright(c) 2011-2017 Intel Corporation All rights reserved.

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
#include <stdint.h>
#include "crc.h"
#include "test.h"

#define BLKSIZE (512)

//#define CACHED_TEST
#ifdef CACHED_TEST
// Cached test, loop many times over small dataset
# define NBLOCKS      100
# define TEST_TYPE_STR "_warm"
#else
// Uncached test.  Pull from large mem base.
#  define GT_L3_CACHE  32*1024*1024	/* some number > last level cache */
#  define TEST_LEN     (2 * GT_L3_CACHE)
#  define NBLOCKS      (TEST_LEN / BLKSIZE)
#  define TEST_TYPE_STR "_cold"
#endif

#ifndef TEST_SEED
# define TEST_SEED 0x1234
#endif

struct blk {
	uint8_t data[BLKSIZE];
};

struct blk_ext {
	uint8_t data[BLKSIZE];
	uint32_t tag;
	uint16_t meta;
	uint16_t crc;
};

void crc16_t10dif_copy_perf(struct blk *blks, struct blk *blkp, struct blk_ext *blks_ext,
			    struct blk_ext *blkp_ext, uint16_t * crc)
{
	int i;
	for (i = 0, blkp = blks, blkp_ext = blks_ext; i < NBLOCKS; i++) {
		*crc = crc16_t10dif_copy(TEST_SEED, blkp_ext->data, blkp->data,
					 sizeof(blks->data));
		blkp_ext->crc = *crc;
		blkp++;
		blkp_ext++;
	}
}

int main(int argc, char *argv[])
{
	uint16_t crc;
	struct blk *blks, *blkp;
	struct blk_ext *blks_ext, *blkp_ext;
	struct perf start;

	printf("crc16_t10dif_streaming_insert_perf:\n");

	if (posix_memalign((void *)&blks, 1024, NBLOCKS * sizeof(*blks))) {
		printf("alloc error: Fail");
		return -1;
	}
	if (posix_memalign((void *)&blks_ext, 1024, NBLOCKS * sizeof(*blks_ext))) {
		printf("alloc error: Fail");
		return -1;
	}

	printf(" size blk: %ld, blk_ext: %ld, blk data: %ld, stream: %ld\n",
	       sizeof(*blks), sizeof(*blks_ext), sizeof(blks->data),
	       NBLOCKS * sizeof(blks->data));
	memset(blks, 0xe5, NBLOCKS * sizeof(*blks));
	memset(blks_ext, 0xe5, NBLOCKS * sizeof(*blks_ext));

	printf("Start timed tests\n");
	fflush(0);

	// Copy and insert test
	BENCHMARK(&start, BENCHMARK_TIME,
		  crc16_t10dif_copy_perf(blks, blkp, blks_ext, blkp_ext, &crc));

	printf("crc16_t10pi_op_copy_insert" TEST_TYPE_STR ": ");
	perf_print(start, (long long)sizeof(blks->data) * NBLOCKS);

	printf("finish 0x%x\n", crc);
	return 0;
}
