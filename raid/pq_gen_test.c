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
#include<limits.h>
#include "raid.h"
#include "types.h"

#define TEST_SOURCES 16
#define TEST_LEN     1024
#define TEST_MEM ((TEST_SOURCES + 2)*(TEST_LEN))
#ifndef TEST_SEED
# define TEST_SEED 0x1234
#endif

// Generates pseudo-random data

void rand_buffer(unsigned char *buf, long buffer_size)
{
	long i;
	for (i = 0; i < buffer_size; i++)
		buf[i] = rand();
}

int dump(unsigned char *buf, int len)
{
	int i;
	for (i = 0; i < len;) {
		printf(" %2x", buf[i++]);
		if (i % 16 == 0)
			printf("\n");
	}
	printf("\n");
	return 0;
}

int main(int argc, char *argv[])
{
	int i, j, k, ret, fail = 0;
	void *buffs[TEST_SOURCES + 2];	// Pointers to src and dest
	char *tmp_buf[TEST_SOURCES + 2];

	printf("Test pq_gen_test ");

	srand(TEST_SEED);

	// Allocate the arrays
	for (i = 0; i < TEST_SOURCES + 2; i++) {
		void *buf;
		ret = posix_memalign(&buf, 32, TEST_LEN);
		if (ret) {
			printf("alloc error: Fail");
			return 1;
		}
		buffs[i] = buf;
	}

	// Test of all zeros
	for (i = 0; i < TEST_SOURCES + 2; i++)
		memset(buffs[i], 0, TEST_LEN);

	pq_gen(TEST_SOURCES + 2, TEST_LEN, buffs);

	for (i = 0; i < TEST_LEN; i++) {
		if (((char *)buffs[TEST_SOURCES])[i] != 0)
			fail++;
	}

	for (i = 0; i < TEST_LEN; i++) {
		if (((char *)buffs[TEST_SOURCES + 1])[i] != 0)
			fail++;
	}

	if (fail > 0) {
		printf("fail zero test %d\n", fail);
		return 1;
	} else
		putchar('.');

	// Test rand1
	for (i = 0; i < TEST_SOURCES + 2; i++)
		rand_buffer(buffs[i], TEST_LEN);

	ret = pq_gen(TEST_SOURCES + 2, TEST_LEN, buffs);
	fail |= pq_check_base(TEST_SOURCES + 2, TEST_LEN, buffs);

	if (fail > 0) {
		int t;
		printf(" Fail rand test1 fail=%d, ret=%d\n", fail, ret);
		for (t = 0; t < TEST_SOURCES + 2; t++)
			dump(buffs[t], 15);

		printf(" reference function p,q\n");
		pq_gen_base(TEST_SOURCES + 2, TEST_LEN, buffs);
		for (t = TEST_SOURCES; t < TEST_SOURCES + 2; t++)
			dump(buffs[t], 15);

		return 1;
	} else
		putchar('.');

	// Test various number of sources
	for (j = 4; j <= TEST_SOURCES + 2; j++) {
		for (i = 0; i < j; i++)
			rand_buffer(buffs[i], TEST_LEN);

		pq_gen(j, TEST_LEN, buffs);
		fail |= pq_check_base(j, TEST_LEN, buffs);

		if (fail > 0) {
			printf("fail rand test %d sources\n", j);
			return 1;
		} else
			putchar('.');
	}

	fflush(0);

	// Test various number of sources and len
	k = 0;
	while (k <= TEST_LEN) {
		for (j = 4; j <= TEST_SOURCES + 2; j++) {
			for (i = 0; i < j; i++)
				rand_buffer(buffs[i], k);

			ret = pq_gen(j, k, buffs);
			fail |= pq_check_base(j, k, buffs);

			if (fail > 0) {
				printf("fail rand test %d sources, len=%d, fail="
				       "%d, ret=%d\n", j, k, fail, ret);
				return 1;
			}
		}
		putchar('.');
		k += 32;
	}

	// Test at the end of buffer
	k = 0;
	while (k <= TEST_LEN) {
		for (j = 0; j < (TEST_SOURCES + 2); j++) {
			rand_buffer(buffs[j], TEST_LEN - k);
			tmp_buf[j] = (char *)buffs[j] + k;
		}

		ret = pq_gen(TEST_SOURCES + 2, TEST_LEN - k, (void *)tmp_buf);
		fail |= pq_check_base(TEST_SOURCES + 2, TEST_LEN - k, (void *)tmp_buf);

		if (fail > 0) {
			printf("fail end test - offset: %d, len: %d, fail: %d, "
			       "ret: %d\n", k, TEST_LEN - k, fail, ret);
			return 1;
		}

		putchar('.');
		fflush(0);
		k += 32;
	}

	if (!fail)
		printf(" done: Pass\n");

	return fail;
}
