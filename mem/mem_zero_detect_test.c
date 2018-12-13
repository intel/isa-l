/**********************************************************************
  Copyright(c) 2011-2018 Intel Corporation All rights reserved.

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
#include <string.h>
#include <stdlib.h>
#include <limits.h>
#include "mem_routines.h"
#include "types.h"

#define TEST_MEM  10*1024*1024
#define TEST_LEN  8*1024
#define RAND_ALIGN   32
#define BORDER_BYTES (5*RAND_ALIGN + 7)

#ifndef RANDOMS
# define RANDOMS 2000
#endif
#ifndef TEST_SEED
# define TEST_SEED 0x1234
#endif

int main(int argc, char *argv[])
{
	int i, j, sign;
	long long r, l;
	void *buf;
	unsigned char *a;
	int failures = 0, ret_neg = 1;

	printf("mem_zero_detect_test %d bytes, %d randoms, seed=0x%x ", TEST_MEM, RANDOMS,
	       TEST_SEED);
	if (posix_memalign(&buf, 64, TEST_MEM)) {
		printf("alloc error: Fail");
		return -1;
	}

	srand(TEST_SEED);

	// Test full zero buffer
	memset(buf, 0, TEST_MEM);
	failures = isal_zero_detect(buf, TEST_MEM);

	if (failures) {
		printf("Fail large buf test\n");
		return failures;
	}
	putchar('.');

	// Test small buffers
	for (i = 0; i < TEST_LEN; i++) {
		failures |= isal_zero_detect(buf, i);
		if (failures) {
			printf("Fail len=%d\n", i);
			return failures;
		}
	}
	putchar('.');

	// Test small buffers near end of alloc region
	a = buf;
	for (i = 0; i < TEST_LEN; i++)
		failures |= isal_zero_detect(&a[TEST_LEN - i], i);

	if (failures) {
		printf("Fail:\n");
		return failures;
	}
	putchar('.');

	// Test for detect non zero
	a[TEST_MEM / 2] = 1;
	ret_neg = isal_zero_detect(a, TEST_MEM);
	if (ret_neg == 0) {
		printf("Fail on not detect\n");
		return -1;
	}
	a[TEST_MEM / 2] = 0;
	putchar('.');

	// Test various non-zero offsets
	for (i = 0; i < BORDER_BYTES; i++) {
		for (j = 0; j < CHAR_BIT; j++) {
			a[i] = 1 << j;
			ret_neg = isal_zero_detect(a, TEST_MEM);
			if (ret_neg == 0) {
				printf("Fail on not detect offsets %d, %d\n", i, j);
				return -1;
			}
			a[i] = 0;
		}
	}
	putchar('.');
	fflush(0);

	// Test random non-zero offsets
	for (i = 0; i < RANDOMS; i++) {
		r = rand();
		r = (r % TEST_LEN) ^ (r & (RAND_ALIGN - 1));
		if (r >= TEST_LEN)
			continue;

		a[r] = 1 << (r & (CHAR_BIT - 1));
		ret_neg = isal_zero_detect(a, TEST_MEM);
		if (ret_neg == 0) {
			printf("Fail on not detect rand %d, e=%lld\n", i, r);
			return -1;
		}
		a[r] = 0;
	}
	putchar('.');
	fflush(0);

	// Test putting non-zero byte at end of buffer
	for (i = 1; i < BORDER_BYTES; i++) {
		for (j = 0; j < CHAR_BIT; j++) {
			a[TEST_MEM - i] = 1 << j;
			ret_neg = isal_zero_detect(a, TEST_MEM);
			if (ret_neg == 0) {
				printf("Fail on not detect rand offset=%d, idx=%d\n", i, j);
				return -1;
			}
			a[TEST_MEM - i] = 0;
		}
	}
	putchar('.');

	// Test various size buffers and non-zero offsets
	for (l = 1; l < TEST_LEN; l++) {
		for (i = 0; i < l + BORDER_BYTES; i++) {
			failures = isal_zero_detect(a, l);

			if (failures) {
				printf("Fail on detect non-zero with l=%lld\n", l);
				return -1;
			}

			a[i] = 1;
			ret_neg = isal_zero_detect(a, l);

			if ((i < l) && (ret_neg == 0)) {
				printf("Fail on non-zero buffer l=%lld err=%d\n", l, i);
				return -1;
			}
			if ((i >= l) && (ret_neg != 0)) {
				printf("Fail on bad pass detect l=%lld err=%d\n", l, i);
				return -1;
			}
			a[i] = 0;
		}
	}
	putchar('.');

	// Test random test size and non-zero error offsets
	for (i = 0; i < RANDOMS; i++) {
		r = rand();
		r = (r % TEST_LEN) ^ (r & (RAND_ALIGN - 1));
		l = r + 1 + (rand() & (CHAR_BIT - 1));
		a[r] = 1 << (r & (CHAR_BIT - 1));
		ret_neg = isal_zero_detect(a, l);
		if (ret_neg == 0) {
			printf("Fail on not detect rand %d, l=%lld, e=%lld\n", i, l, r);
			return -1;
		}
		a[r] = 0;
	}
	putchar('.');
	fflush(0);

	// Test combinations of zero and non-zero buffers
	for (i = 0; i < RANDOMS; i++) {
		r = rand();
		r = (r % TEST_LEN) ^ (r & (RAND_ALIGN - 1));
		sign = rand() & 1 ? 1 : -1;
		l = r + sign * (rand() & (2 * RAND_ALIGN - 1));

		if ((l >= TEST_LEN) || (l < 0) || (r >= TEST_LEN))
			continue;

		a[r] = 1 << (r & (CHAR_BIT - 1));
		ret_neg = isal_zero_detect(a, l);

		if ((r < l) && (ret_neg == 0)) {
			printf("Fail on non-zero rand buffer %d, l=%lld, e=%lld\n", i, l, r);
			return -1;
		}
		if ((r >= l) && (ret_neg != 0)) {
			printf("Fail on bad pass zero detect rand %d, l=%lld, e=%lld\n", i, l,
			       r);
			return -1;
		}

		a[r] = 0;
	}
	putchar('.');
	fflush(0);

	printf(failures == 0 ? " Pass\n" : " Fail\n");
	return failures;
}
