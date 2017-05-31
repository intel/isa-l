/**********************************************************************
  Copyright(c) 2011-2015 Intel Corporation All rights reserved.

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

#include<stdio.h>
#include<stdint.h>
#include<string.h>
#include<stdlib.h>
#include<sys/time.h>
#include "raid.h"
#include "test.h"

//#define CACHED_TEST
#ifdef CACHED_TEST
// Loop many times over same
# define TEST_SOURCES 10
# define TEST_LEN     8*1024
# define TEST_LOOPS   2000000
# define TEST_TYPE_STR "_warm"
#else
// Uncached test.  Pull from large mem base.
# define TEST_SOURCES 10
# define GT_L3_CACHE  32*1024*1024	/* some number > last level cache */
# define TEST_LEN     GT_L3_CACHE / TEST_SOURCES
# define TEST_LOOPS   1000
# define TEST_TYPE_STR "_cold"
#endif

#define TEST_MEM ((TEST_SOURCES + 1)*(TEST_LEN))

int main(int argc, char *argv[])
{
	int i, ret, fail = 0;
	void **buffs;
	void *buff;
	struct perf start, stop;

	printf("Test xor_gen_perf\n");

	ret = posix_memalign((void **)&buff, 8, sizeof(int *) * (TEST_SOURCES + 6));
	if (ret) {
		printf("alloc error: Fail");
		return 1;
	}
	buffs = buff;

	// Allocate the arrays
	for (i = 0; i < TEST_SOURCES + 1; i++) {
		void *buf;
		ret = posix_memalign(&buf, 64, TEST_LEN);
		if (ret) {
			printf("alloc error: Fail");
			return 1;
		}
		buffs[i] = buf;
	}

	// Setup data
	for (i = 0; i < TEST_SOURCES + 1; i++)
		memset(buffs[i], 0, TEST_LEN);

	// Warm up
	xor_gen(TEST_SOURCES + 1, TEST_LEN, buffs);

	perf_start(&start);
	for (i = 0; i < TEST_LOOPS; i++)
		xor_gen(TEST_SOURCES + 1, TEST_LEN, buffs);
	perf_stop(&stop);
	printf("xor_gen" TEST_TYPE_STR ": ");
	perf_print(stop, start, (long long)TEST_MEM * i);

	return fail;
}
