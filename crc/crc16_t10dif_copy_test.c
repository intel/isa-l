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
#include <string.h>
#include <stdint.h>
#include <stdlib.h>
#include <assert.h>
#include "crc.h"
#include "crc_ref.h"

#ifndef RANDOMS
# define RANDOMS   20
#endif
#ifndef TEST_SEED
# define TEST_SEED 0x1234
#endif

#define MAX_BUF    2345
#define TEST_SIZE  217
#define TEST_LEN  (8 * 1024)

typedef uint16_t u16;
typedef uint8_t u8;

// bitwise crc version
uint16_t crc16_t10dif_copy_ref(uint16_t seed, uint8_t * dst, uint8_t * src, uint64_t len);

void rand_buffer(unsigned char *buf, long buffer_size)
{
	long i;
	for (i = 0; i < buffer_size; i++)
		buf[i] = rand();
}

int memtst(unsigned char *buf, unsigned char c, int len)
{
	int i;
	for (i = 0; i < len; i++)
		if (*buf++ != c)
			return 1;

	return 0;
}

int crc_copy_check(const char *description, u8 * dst, u8 * src, u8 dst_fill_val, int len,
		   int tot)
{
	u16 seed;
	int rem;

	assert(tot >= len);
	seed = rand();
	rem = tot - len;
	memset(dst, dst_fill_val, tot);

	// multi-binary crc version
	u16 crc_dut = crc16_t10dif_copy(seed, dst, src, len);
	u16 crc_ref = crc16_t10dif(seed, src, len);
	if (crc_dut != crc_ref) {
		printf("%s, crc gen fail: 0x%4x 0x%4x len=%d\n", description, crc_dut,
		       crc_ref, len);
		return 1;
	} else if (memcmp(dst, src, len)) {
		printf("%s, copy fail: len=%d\n", description, len);
		return 1;
	} else if (memtst(&dst[len], dst_fill_val, rem)) {
		printf("%s, writeover fail: len=%d\n", description, len);
		return 1;
	}
	// bitwise crc version
	crc_dut = crc16_t10dif_copy_ref(seed, dst, src, len);
	crc_ref = crc16_t10dif_ref(seed, src, len);
	if (crc_dut != crc_ref) {
		printf("%s, crc gen fail (table-driven): 0x%4x 0x%4x len=%d\n", description,
		       crc_dut, crc_ref, len);
		return 1;
	} else if (memcmp(dst, src, len)) {
		printf("%s, copy fail (table driven): len=%d\n", description, len);
		return 1;
	} else if (memtst(&dst[len], dst_fill_val, rem)) {
		printf("%s, writeover fail (table driven): len=%d\n", description, len);
		return 1;
	}
	return 0;
}

int main(int argc, char *argv[])
{
	int r = 0;
	int i;
	int len, tot;
	u8 *src_raw, *dst_raw;
	u8 *src, *dst;

	printf("Test crc16_t10dif_copy_test:\n");
	src_raw = (u8 *) malloc(TEST_LEN);
	dst_raw = (u8 *) malloc(TEST_LEN);
	if (NULL == src_raw || NULL == dst_raw) {
		printf("alloc error: Fail");
		return -1;
	}
	src = src_raw;
	dst = dst_raw;

	srand(TEST_SEED);

	// Test of all zeros
	memset(src, 0, TEST_LEN);
	r |= crc_copy_check("zero tst", dst, src, 0x5e, MAX_BUF, TEST_LEN);

	// Another simple test pattern
	memset(src, 0xff, TEST_LEN);
	r |= crc_copy_check("simp tst", dst, src, 0x5e, MAX_BUF, TEST_LEN);

	// Do a few short len random data tests
	rand_buffer(src, TEST_LEN);
	rand_buffer(dst, TEST_LEN);
	for (i = 0; i < MAX_BUF; i++) {
		r |= crc_copy_check("short len", dst, src, rand(), i, MAX_BUF);
	}
	printf(".");

	// Do a few longer tests, random data
	for (i = TEST_LEN; i >= (TEST_LEN - TEST_SIZE); i--) {
		r |= crc_copy_check("long len", dst, src, rand(), i, TEST_LEN);
	}
	printf(".");

	// Do random size, random data
	for (i = 0; i < RANDOMS; i++) {
		len = rand() % TEST_LEN;
		r |= crc_copy_check("rand len", dst, src, rand(), len, TEST_LEN);
	}
	printf(".");

	// Run tests at end of buffer
	for (i = 0; i < RANDOMS; i++) {
		len = rand() % TEST_LEN;
		src = &src_raw[TEST_LEN - len - 1];
		dst = &dst_raw[TEST_LEN - len - 1];
		tot = len;
		r |= crc_copy_check("end of buffer", dst, src, rand(), len, tot);
	}
	printf(".");

	printf("Test done: %s\n", r ? "Fail" : "Pass");
	return r;
}
