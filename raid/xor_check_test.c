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
#include "raid.h"
#include "types.h"

#define TEST_SOURCES 16
#define TEST_LEN     1024
#define TEST_MEM ((TEST_SOURCES + 1)*(TEST_LEN))
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

int main(int argc, char *argv[])
{
	int i, j, k, ret, fail = 0;
	void *buffs[TEST_SOURCES + 1];
	char c;
	int serr, lerr;
	char *tmp_buf[TEST_SOURCES + 1];

	printf("Test xor_check_test %d sources X %d bytes\n", TEST_SOURCES, TEST_LEN);

	srand(TEST_SEED);

	// Allocate the arrays
	for (i = 0; i < TEST_SOURCES + 1; i++) {
		void *buf;
		if (posix_memalign(&buf, 16, TEST_LEN)) {
			printf("alloc error: Fail");
			return 1;
		}
		buffs[i] = buf;
	}

	// Test of all zeros
	for (i = 0; i < TEST_SOURCES + 1; i++)
		memset(buffs[i], 0, TEST_LEN);

	xor_gen_base(TEST_SOURCES + 1, TEST_LEN, buffs);
	ret = xor_check(TEST_SOURCES + 1, TEST_LEN, buffs);
	if (ret != 0) {
		fail++;
		printf("\nfail zero test %d\n", ret);
	}

	((char *)(buffs[0]))[TEST_LEN - 2] = 0x7;	// corrupt buffer
	ret = xor_check(TEST_SOURCES + 1, TEST_LEN, buffs);
	if (ret == 0) {
		fail++;
		printf("\nfail corrupt buffer test %d\n", ret);
	}
	((char *)(buffs[0]))[TEST_LEN - 2] = 0;	// un-corrupt buffer

	// Test corrupted buffer any location on all sources
	for (j = 0; j < TEST_SOURCES + 1; j++) {
		for (i = TEST_LEN - 1; i >= 0; i--) {
			((char *)buffs[j])[i] = 0x5;	// corrupt buffer
			ret = xor_check(TEST_SOURCES + 1, TEST_LEN, buffs);
			if (ret == 0) {
				fail++;
				printf("\nfail corrupt buffer test j=%d, i=%d\n", j, i);
				return 1;
			}
			((char *)buffs[j])[i] = 0;	// un-corrupt buffer
		}
		putchar('.');
	}

	// Test rand1
	for (i = 0; i < TEST_SOURCES + 1; i++)
		rand_buffer(buffs[i], TEST_LEN);

	xor_gen_base(TEST_SOURCES + 1, TEST_LEN, buffs);
	ret = xor_check(TEST_SOURCES + 1, TEST_LEN, buffs);
	if (ret != 0) {
		fail++;
		printf("fail first rand test %d\n", ret);
	}

	c = ((char *)(buffs[0]))[TEST_LEN - 2];
	((char *)(buffs[0]))[TEST_LEN - 2] = c ^ 0x1;
	ret = xor_check(TEST_SOURCES + 1, TEST_LEN, buffs);
	if (ret == 0) {
		fail++;
		printf("\nFail corrupt buffer test, passed when should have failed\n");
	}
	((char *)(buffs[0]))[TEST_LEN - 2] = c;	// un-corrupt buffer

	// Test corrupted buffer any location on all sources w/ random data
	for (j = 0; j < TEST_SOURCES + 1; j++) {
		for (i = TEST_LEN - 1; i >= 0; i--) {
			// Check it still passes
			ret = xor_check(TEST_SOURCES + 1, TEST_LEN, buffs);
			if (ret != 0) {	// should pass
				fail++;
				printf
				    ("\nFail rand test with un-corrupted buffer j=%d, i=%d\n",
				     j, i);
				return 1;
			}
			c = ((char *)buffs[j])[i];
			((char *)buffs[j])[i] = c ^ 1;	// corrupt buffer
			ret = xor_check(TEST_SOURCES + 1, TEST_LEN, buffs);
			if (ret == 0) {	// Check it now fails
				fail++;
				printf("\nfail corrupt buffer test j=%d, i=%d\n", j, i);
				return 1;
			}
			((char *)buffs[j])[i] = c;	// un-corrupt buffer
		}
		putchar('.');
	}

	// Test various number of sources, full length
	for (j = 3; j <= TEST_SOURCES + 1; j++) {
		// New random data
		for (i = 0; i < j; i++)
			rand_buffer(buffs[i], TEST_LEN);

		// Generate xor parity for this number of sources
		xor_gen_base(j, TEST_LEN, buffs);

		// Set errors up in each source and len position
		for (i = 0; i < j; i++) {
			for (k = 0; k < TEST_LEN; k++) {
				// See if it still passes
				ret = xor_check(j, TEST_LEN, buffs);
				if (ret != 0) {	// Should pass
					printf("\nfail rand test %d sources\n", j);
					fail++;
					return 1;
				}

				c = ((char *)buffs[i])[k];
				((char *)buffs[i])[k] = c ^ 1;	// corrupt buffer

				ret = xor_check(j, TEST_LEN, buffs);
				if (ret == 0) {	// Should fail
					printf
					    ("\nfail rand test corrupted buffer %d sources\n",
					     j);
					fail++;
					return 1;
				}
				((char *)buffs[i])[k] = c;	// un-corrupt buffer
			}
		}
		putchar('.');
	}

	fflush(0);

	// Test various number of sources and len
	k = 1;
	while (k <= TEST_LEN) {
		for (j = 3; j <= TEST_SOURCES + 1; j++) {
			for (i = 0; i < j; i++)
				rand_buffer(buffs[i], k);

			// Generate xor parity for this number of sources
			xor_gen_base(j, k, buffs);

			// Inject errors at various source and len positions
			for (lerr = 0; lerr < k; lerr += 10) {
				for (serr = 0; serr < j; serr++) {

					// See if it still passes
					ret = xor_check(j, k, buffs);
					if (ret != 0) {	// Should pass
						printf("\nfail rand test %d sources\n", j);
						fail++;
						return 1;
					}

					c = ((char *)buffs[serr])[lerr];
					((char *)buffs[serr])[lerr] = c ^ 1;	// corrupt buffer

					ret = xor_check(j, k, buffs);
					if (ret == 0) {	// Should fail
						printf("\nfail rand test corrupted buffer "
						       "%d sources, len=%d, ret=%d\n", j, k,
						       ret);
						fail++;
						return 1;
					}
					((char *)buffs[serr])[lerr] = c;	// un-corrupt buffer
				}
			}
		}
		putchar('.');
		fflush(0);
		k += 1;
	}

	// Test at the end of buffer
	for (i = 0; i < TEST_LEN; i += 32) {
		for (j = 0; j < TEST_SOURCES + 1; j++) {
			rand_buffer(buffs[j], TEST_LEN - i);
			tmp_buf[j] = (char *)buffs[j] + i;
		}

		xor_gen_base(TEST_SOURCES + 1, TEST_LEN - i, (void *)tmp_buf);

		// Test good data
		ret = xor_check(TEST_SOURCES + 1, TEST_LEN - i, (void *)tmp_buf);
		if (ret != 0) {
			printf("fail end test - offset: %d, len: %d\n", i, TEST_LEN - i);
			fail++;
			return 1;
		}
		// Test bad data
		for (serr = 0; serr < TEST_SOURCES + 1; serr++) {
			for (lerr = 0; lerr < (TEST_LEN - i); lerr++) {
				c = tmp_buf[serr][lerr];
				tmp_buf[serr][lerr] = c ^ 1;

				ret =
				    xor_check(TEST_SOURCES + 1, TEST_LEN - i, (void *)tmp_buf);
				if (ret == 0) {
					printf("fail end test corrupted buffer - "
					       "offset: %d, len: %d, ret: %d\n", i,
					       TEST_LEN - i, ret);
					fail++;
					return 1;
				}

				tmp_buf[serr][lerr] = c;
			}
		}

		putchar('.');
		fflush(0);
	}

	if (fail == 0)
		printf("Pass\n");

	return fail;

}
