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
#include <stdint.h>
#include "crc64.h"
#include "types.h"
#include "crc64_ref.h"

#ifndef TEST_SEED
# define TEST_SEED 0x1234
#endif

#define MAX_BUF   4096
#define TEST_SIZE  32

typedef uint64_t u64;
typedef uint32_t u32;
typedef uint16_t u16;
typedef uint8_t u8;

typedef uint64_t(*crc64_func_t) (uint64_t, const uint8_t *, uint64_t);

typedef struct func_case {
	char *note;
	crc64_func_t crc64_func_call;
	crc64_func_t crc64_base_call;
	crc64_func_t crc64_ref_call;
} func_case_t;

func_case_t test_funcs[] = {
	{"crc64_ecma_norm", crc64_ecma_norm, crc64_ecma_norm_base, crc64_ecma_norm_ref},
	{"crc64_ecma_refl", crc64_ecma_refl, crc64_ecma_refl_base, crc64_ecma_refl_ref},
	{"crc64_iso_norm", crc64_iso_norm, crc64_iso_norm_base, crc64_iso_norm_ref},
	{"crc64_iso_refl", crc64_iso_refl, crc64_iso_refl_base, crc64_iso_refl_ref},
	{"crc64_jones_norm", crc64_jones_norm, crc64_jones_norm_base,
	 crc64_jones_norm_ref},
	{"crc64_jones_refl", crc64_jones_refl, crc64_jones_refl_base, crc64_jones_refl_ref}
};

// Generates pseudo-random data

void rand_buffer(unsigned char *buf, long buffer_size)
{
	long i;
	for (i = 0; i < buffer_size; i++)
		buf[i] = rand();
}

// Test cases
int zeros_test(func_case_t * test_func);

int simple_pattern_test(func_case_t * test_func);

int seeds_sizes_test(func_case_t * test_func);

int eob_test(func_case_t * test_func);

int update_test(func_case_t * test_func);

int verbose = 0;
void *buf_alloc = NULL;

int main(int argc, char *argv[])
{
	int fail = 0, fail_case;
	int i, ret;
	func_case_t *test_func;

	verbose = argc - 1;

	// Align to 32B boundary
	ret = posix_memalign(&buf_alloc, TEST_SIZE, MAX_BUF * TEST_SIZE);
	if (ret) {
		printf("alloc error: Fail");
		return -1;
	}
	srand(TEST_SEED);
	printf("CRC64 Tests\n");

	for (i = 0; i < sizeof(test_funcs) / sizeof(test_funcs[0]); i++) {
		fail_case = 0;
		test_func = &test_funcs[i];

		printf("Test %s\t", test_func->note);
		fail_case += zeros_test(test_func);
		fail_case += simple_pattern_test(test_func);
		fail_case += seeds_sizes_test(test_func);
		fail_case += eob_test(test_func);
		fail_case += update_test(test_func);
		printf(" done: %s\n", fail_case ? "Fail" : "Pass");

		if (fail_case) {
			printf("\n%s Failed %d tests\n", test_func->note, fail_case);
			fail++;
		}
	}

	printf("CRC64 Tests all done: %s\n", fail ? "Fail" : "Pass");

	return fail;
}

// Test of all zeros
int zeros_test(func_case_t * test_func)
{
	uint64_t crc_ref, crc_base, crc;
	int fail = 0;
	unsigned char *buf = NULL;

	buf = (unsigned char *)buf_alloc;
	memset(buf, 0, MAX_BUF * 10);
	crc_ref = test_func->crc64_ref_call(TEST_SEED, buf, MAX_BUF * 10);
	crc_base = test_func->crc64_base_call(TEST_SEED, buf, MAX_BUF * 10);
	crc = test_func->crc64_func_call(TEST_SEED, buf, MAX_BUF * 10);

	if ((crc_base != crc_ref) || (crc != crc_ref)) {
		fail++;
		printf("\n		   opt   ref\n");
		printf("		 ------ ------\n");
		printf("crc	zero = 0x%16lx 0x%16lx 0x%16lx \n", crc_ref, crc_base, crc);
	} else
		printf(".");

	return fail;
}

// Another simple test pattern
int simple_pattern_test(func_case_t * test_func)
{
	uint64_t crc_ref, crc_base, crc;
	int fail = 0;
	unsigned char *buf = NULL;

	buf = (unsigned char *)buf_alloc;
	memset(buf, 0x8a, MAX_BUF);
	crc_ref = test_func->crc64_ref_call(TEST_SEED, buf, MAX_BUF);
	crc_base = test_func->crc64_base_call(TEST_SEED, buf, MAX_BUF);
	crc = test_func->crc64_func_call(TEST_SEED, buf, MAX_BUF);

	if ((crc_base != crc_ref) || (crc != crc_ref))
		fail++;
	if (verbose)
		printf("crc  all 8a = 0x%16lx 0x%16lx 0x%16lx\n", crc_ref, crc_base, crc);
	else
		printf(".");

	return fail;
}

int seeds_sizes_test(func_case_t * test_func)
{
	uint64_t crc_ref, crc_base, crc;
	int fail = 0;
	int i;
	uint64_t r, s;
	unsigned char *buf = NULL;

	// Do a few random tests
	buf = (unsigned char *)buf_alloc;	//reset buf
	r = rand();
	rand_buffer(buf, MAX_BUF * TEST_SIZE);

	for (i = 0; i < TEST_SIZE; i++) {
		crc_ref = test_func->crc64_ref_call(r, buf, MAX_BUF);
		crc_base = test_func->crc64_base_call(r, buf, MAX_BUF);
		crc = test_func->crc64_func_call(r, buf, MAX_BUF);

		if ((crc_base != crc_ref) || (crc != crc_ref))
			fail++;
		if (verbose)
			printf("crc rand%3d = 0x%16lx 0x%16lx 0x%16lx\n", i, crc_ref, crc_base,
			       crc);
		else if (i % (TEST_SIZE / 8) == 0)
			printf(".");
		buf += MAX_BUF;
	}

	// Do a few random sizes
	buf = (unsigned char *)buf_alloc;	//reset buf
	r = rand();

	for (i = MAX_BUF; i >= 0; i--) {
		crc_ref = test_func->crc64_ref_call(r, buf, i);
		crc_base = test_func->crc64_base_call(r, buf, i);
		crc = test_func->crc64_func_call(r, buf, i);

		if ((crc_base != crc_ref) || (crc != crc_ref)) {
			fail++;
			printf("fail random size%i 0x%16lx 0x%16lx 0x%16lx\n", i, crc_ref,
			       crc_base, crc);
		} else if (i % (MAX_BUF / 8) == 0)
			printf(".");
	}

	// Try different seeds
	for (s = 0; s < 20; s++) {
		buf = (unsigned char *)buf_alloc;	//reset buf

		r = rand();	// just to get a new seed
		rand_buffer(buf, MAX_BUF * TEST_SIZE);	// new pseudo-rand data

		if (verbose)
			printf("seed = 0x%lx\n", r);

		for (i = 0; i < TEST_SIZE; i++) {
			crc_ref = test_func->crc64_ref_call(r, buf, MAX_BUF);
			crc_base = test_func->crc64_base_call(r, buf, MAX_BUF);
			crc = test_func->crc64_func_call(r, buf, MAX_BUF);

			if ((crc_base != crc_ref) || (crc != crc_ref))
				fail++;
			if (verbose)
				printf("crc rand%3d = 0x%16lx 0x%16lx 0x%16lx\n", i, crc_ref,
				       crc_base, crc);
			else if (i % (TEST_SIZE * 20 / 8) == 0)
				printf(".");
			buf += MAX_BUF;
		}
	}

	return fail;
}

// Run tests at end of buffer
int eob_test(func_case_t * test_func)
{
	uint64_t crc_ref, crc_base, crc;
	int fail = 0;
	int i;
	unsigned char *buf = NULL;

	// Null test
	if (0 != test_func->crc64_func_call(0, NULL, 0)) {
		fail++;
		printf("crc null test fail\n");
	}

	buf = (unsigned char *)buf_alloc;	//reset buf
	buf = buf + ((MAX_BUF - 1) * TEST_SIZE);	//Line up TEST_SIZE from end
	for (i = 0; i <= TEST_SIZE; i++) {
		crc_ref = test_func->crc64_ref_call(TEST_SEED, buf + i, TEST_SIZE - i);
		crc_base = test_func->crc64_base_call(TEST_SEED, buf + i, TEST_SIZE - i);
		crc = test_func->crc64_func_call(TEST_SEED, buf + i, TEST_SIZE - i);

		if ((crc_base != crc_ref) || (crc != crc_ref))
			fail++;
		if (verbose)
			printf("crc eob rand%3d = 0x%16lx 0x%16lx 0x%16lx\n", i, crc_ref,
			       crc_base, crc);
		else if (i % (TEST_SIZE / 8) == 0)
			printf(".");
	}

	return fail;
}

int update_test(func_case_t * test_func)
{
	uint64_t crc_ref, crc_base, crc;
	int fail = 0;
	int i;
	uint64_t r;
	unsigned char *buf = NULL;

	buf = (unsigned char *)buf_alloc;	//reset buf
	r = rand();
	// Process the whole buf with reference func single call.
	crc_ref = test_func->crc64_ref_call(r, buf, MAX_BUF * TEST_SIZE);
	crc_base = test_func->crc64_base_call(r, buf, MAX_BUF * TEST_SIZE);
	// Process buf with update method.
	for (i = 0; i < TEST_SIZE; i++) {
		crc = test_func->crc64_func_call(r, buf, MAX_BUF);
		// Update crc seeds and buf pointer.
		r = crc;
		buf += MAX_BUF;
	}

	if ((crc_base != crc_ref) || (crc != crc_ref))
		fail++;
	if (verbose)
		printf("crc rand%3d = 0x%16lx 0x%16lx 0x%16lx\n", i, crc_ref, crc_base, crc);
	else
		printf(".");

	return fail;
}
