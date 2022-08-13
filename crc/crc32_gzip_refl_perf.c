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

#ifndef GT_L3_CACHE
# define GT_L3_CACHE  32*1024*1024	/* some number > last level cache */
#endif

#if !defined(COLD_TEST) && !defined(TEST_CUSTOM)
// Cached test, loop many times over small dataset
# define TEST_LEN     8*1024
# define TEST_TYPE_STR "_warm"
#elif defined (COLD_TEST)
// Uncached test.  Pull from large mem base.
# define TEST_LEN     (2 * GT_L3_CACHE)
# define TEST_TYPE_STR "_cold"
#endif

#ifndef TEST_SEED
# define TEST_SEED 0x1234
#endif

#define TEST_MEM TEST_LEN

int main(int argc, char *argv[])
{
	void *buf;
	uint32_t crc;
	struct perf start;

	printf("crc32_gzip_refl_perf:\n");

	if (posix_memalign(&buf, 1024, TEST_LEN)) {
		printf("alloc error: Fail");
		return -1;
	}

	printf("Start timed tests\n");
	fflush(0);

	memset(buf, 0, TEST_LEN);
	BENCHMARK(&start, BENCHMARK_TIME, crc = crc32_gzip_refl(TEST_SEED, buf, TEST_LEN));
	printf("crc32_gzip_refl" TEST_TYPE_STR ": ");
	perf_print(start, (long long)TEST_LEN);

	printf("finish 0x%x\n", crc);

	printf("crc32_gzip_refl_base_perf:\n");
	printf("Start timed tests\n");
	fflush(0);

	BENCHMARK(&start, BENCHMARK_TIME, crc =
		  crc32_gzip_refl_base(TEST_SEED, buf, TEST_LEN));
	printf("crc32_gzip_refl_base" TEST_TYPE_STR ": ");
	perf_print(start, (long long)TEST_LEN);

	printf("finish 0x%x\n", crc);

	return 0;
}
