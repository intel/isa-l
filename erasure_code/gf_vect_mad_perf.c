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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>		// for memset, memcmp
#include "erasure_code.h"
#include "types.h"
#include "test.h"

#ifndef FUNCTION_UNDER_TEST
//By default, test sse version
#if __x86_64__  || __i386__ || _M_X64 || _M_IX86
# define FUNCTION_UNDER_TEST gf_4vect_mad_sse
# define REF_FUNCTION gf_4vect_dot_prod_sse
# define VECT 4
#else
# define FUNCTION_UNDER_TEST gf_vect_mad_base
# define REF_FUNCTION gf_vect_dot_prod_base
# define VECT 1
#endif //__x86_64__  || __i386__ || _M_X64 || _M_IX86
#endif

#define str(s) #s
#define xstr(s) str(s)

//#define CACHED_TEST
#ifdef CACHED_TEST
// Cached test, loop many times over small dataset
# define TEST_SOURCES 10
# define TEST_LEN     8*1024
# define TEST_LOOPS   40000
# define TEST_TYPE_STR "_warm"
#else
# ifndef TEST_CUSTOM
// Uncached test.  Pull from large mem base.
#  define TEST_SOURCES 10
#  define GT_L3_CACHE  32*1024*1024	/* some number > last level cache */
#  define TEST_LEN     ((GT_L3_CACHE / TEST_SOURCES) & ~(64-1))
#  define TEST_LOOPS   100
#  define TEST_TYPE_STR "_cold"
# else
#  define TEST_TYPE_STR "_cus"
#  ifndef TEST_LOOPS
#    define TEST_LOOPS 1000
#  endif
# endif
#endif

typedef unsigned char u8;

#if (VECT == 1)
# define LAST_ARG *dest
#else
# define LAST_ARG **dest
#endif

extern void FUNCTION_UNDER_TEST(int len, int vec, int vec_i, unsigned char *gftbls,
				unsigned char *src, unsigned char LAST_ARG);
extern void REF_FUNCTION(int len, int vlen, unsigned char *gftbls, unsigned char **src,
			 unsigned char LAST_ARG);

void dump(unsigned char *buf, int len)
{
	int i;
	for (i = 0; i < len;) {
		printf(" %2x", 0xff & buf[i++]);
		if (i % 32 == 0)
			printf("\n");
	}
	printf("\n");
}

void dump_matrix(unsigned char **s, int k, int m)
{
	int i, j;
	for (i = 0; i < k; i++) {
		for (j = 0; j < m; j++) {
			printf(" %2x", s[i][j]);
		}
		printf("\n");
	}
	printf("\n");
}

int main(int argc, char *argv[])
{
	int i, j, l;
	void *buf;
	u8 gf[6][TEST_SOURCES];
	u8 *g_tbls;
	u8 *dest_ref[VECT];
	u8 *dest_ptrs[VECT], *buffs[TEST_SOURCES];
	u8 *dest_perf_ptrs[VECT];
	struct perf start, stop;

	printf("test " xstr(FUNCTION_UNDER_TEST) ": %dx%d\n", TEST_SOURCES, TEST_LEN);

	// Allocate the arrays
	for (i = 0; i < TEST_SOURCES; i++) {
		if (posix_memalign(&buf, 64, TEST_LEN)) {
			printf("alloc error: Fail");
			return -1;
		}
		buffs[i] = buf;
	}

	if (posix_memalign(&buf, 16, VECT * TEST_SOURCES * 32)) {
		printf("alloc error: Fail");
		return -1;
	}
	g_tbls = buf;

	for (i = 0; i < VECT; i++) {
		if (posix_memalign(&buf, 64, TEST_LEN)) {
			printf("alloc error: Fail");
			return -1;
		}
		dest_ptrs[i] = buf;
		memset(dest_ptrs[i], 0, TEST_LEN);
	}

	for (i = 0; i < VECT; i++) {
		if (posix_memalign(&buf, 64, TEST_LEN)) {
			printf("alloc error: Fail");
			return -1;
		}
		dest_ref[i] = buf;
		memset(dest_ref[i], 0, TEST_LEN);
	}

	for (i = 0; i < VECT; i++) {
		if (posix_memalign(&buf, 64, TEST_LEN)) {
			printf("alloc error: Fail");
			return -1;
		}
		dest_perf_ptrs[i] = buf;
		memset(dest_perf_ptrs[i], 0, TEST_LEN);
	}

	// Performance test
	for (i = 0; i < TEST_SOURCES; i++)
		for (j = 0; j < TEST_LEN; j++)
			buffs[i][j] = rand();

	for (i = 0; i < VECT; i++)
		for (j = 0; j < TEST_SOURCES; j++) {
			gf[i][j] = rand();
			gf_vect_mul_init(gf[i][j], &g_tbls[i * (32 * TEST_SOURCES) + j * 32]);
		}

	for (i = 0; i < VECT; i++)
		gf_vect_dot_prod_base(TEST_LEN, TEST_SOURCES, &g_tbls[i * 32 * TEST_SOURCES],
				      buffs, dest_ref[i]);

	for (i = 0; i < VECT; i++)
		memset(dest_ptrs[i], 0, TEST_LEN);
	for (i = 0; i < TEST_SOURCES; i++) {
#if (VECT == 1)
		FUNCTION_UNDER_TEST(TEST_LEN, TEST_SOURCES, i, g_tbls, buffs[i], *dest_ptrs);
#else
		FUNCTION_UNDER_TEST(TEST_LEN, TEST_SOURCES, i, g_tbls, buffs[i], dest_ptrs);
#endif
	}
	for (i = 0; i < VECT; i++) {
		if (0 != memcmp(dest_ref[i], dest_ptrs[i], TEST_LEN)) {
			printf("Fail perf " xstr(FUNCTION_UNDER_TEST) " test1\n");
			dump_matrix(buffs, 5, TEST_SOURCES);
			printf("dprod_base:");
			dump(dest_ref[i], 25);
			printf("dprod_dut:");
			dump(dest_ptrs[i], 25);
			return -1;
		}
	}

#if (VECT == 1)
	REF_FUNCTION(TEST_LEN, TEST_SOURCES, g_tbls, buffs, *dest_ref);
#else
	REF_FUNCTION(TEST_LEN, TEST_SOURCES, g_tbls, buffs, dest_ref);
#endif
	for (i = 0; i < VECT; i++) {
		if (0 != memcmp(dest_ref[i], dest_ptrs[i], TEST_LEN)) {
			printf("Fail perf " xstr(FUNCTION_UNDER_TEST) " test1\n");
			dump_matrix(buffs, 5, TEST_SOURCES);
			printf("dprod_base:");
			dump(dest_ref[i], 25);
			printf("dprod_dut:");
			dump(dest_ptrs[i], 25);
			return -1;
		}
	}

#ifdef DO_REF_PERF

#if (VECT == 1)
	REF_FUNCTION(TEST_LEN, TEST_SOURCES, g_tbls, buffs, *dest_ref);
#else
	REF_FUNCTION(TEST_LEN, TEST_SOURCES, g_tbls, buffs, dest_ref);
#endif
	perf_start(&start);
	for (l = 0; l < TEST_LOOPS; l++) {
		for (j = 0; j < TEST_SOURCES; j++) {
#if (VECT == 1)
			gf_vect_mul_init(gf[0][j], &g_tbls[j * 32]);
#elif (VECT == 2)
			gf_vect_mul_init(gf[0][j], &g_tbls[j * 32]);
			gf_vect_mul_init(gf[1][j], &g_tbls[(32 * TEST_SOURCES) + (j * 32)]);
#elif (VECT == 3)
			gf_vect_mul_init(gf[0][j], &g_tbls[j * 32]);
			gf_vect_mul_init(gf[1][j], &g_tbls[(32 * TEST_SOURCES) + (j * 32)]);
			gf_vect_mul_init(gf[2][j], &g_tbls[(64 * TEST_SOURCES) + (j * 32)]);
#elif (VECT == 4)
			gf_vect_mul_init(gf[0][j], &g_tbls[j * 32]);
			gf_vect_mul_init(gf[1][j], &g_tbls[(32 * TEST_SOURCES) + (j * 32)]);
			gf_vect_mul_init(gf[2][j], &g_tbls[(64 * TEST_SOURCES) + (j * 32)]);
			gf_vect_mul_init(gf[3][j], &g_tbls[(96 * TEST_SOURCES) + (j * 32)]);
#elif (VECT == 5)
			gf_vect_mul_init(gf[0][j], &g_tbls[j * 32]);
			gf_vect_mul_init(gf[1][j], &g_tbls[(32 * TEST_SOURCES) + (j * 32)]);
			gf_vect_mul_init(gf[2][j], &g_tbls[(64 * TEST_SOURCES) + (j * 32)]);
			gf_vect_mul_init(gf[3][j], &g_tbls[(96 * TEST_SOURCES) + (j * 32)]);
			gf_vect_mul_init(gf[4][j], &g_tbls[(128 * TEST_SOURCES) + (j * 32)]);
#elif (VECT == 6)
			gf_vect_mul_init(gf[0][j], &g_tbls[j * 32]);
			gf_vect_mul_init(gf[1][j], &g_tbls[(32 * TEST_SOURCES) + (j * 32)]);
			gf_vect_mul_init(gf[2][j], &g_tbls[(64 * TEST_SOURCES) + (j * 32)]);
			gf_vect_mul_init(gf[3][j], &g_tbls[(96 * TEST_SOURCES) + (j * 32)]);
			gf_vect_mul_init(gf[4][j], &g_tbls[(128 * TEST_SOURCES) + (j * 32)]);
			gf_vect_mul_init(gf[5][j], &g_tbls[(160 * TEST_SOURCES) + (j * 32)]);
#endif
		}

#if (VECT == 1)
		REF_FUNCTION(TEST_LEN, TEST_SOURCES, g_tbls, buffs, *dest_ref);
#else
		REF_FUNCTION(TEST_LEN, TEST_SOURCES, g_tbls, buffs, dest_ref);
#endif
	}
	perf_stop(&stop);
	printf(xstr(REF_FUNCTION) TEST_TYPE_STR ": ");
	perf_print(stop, start, (long long)TEST_LEN * (TEST_SOURCES + VECT) * TEST_LOOPS);

#endif

	for (i = 0; i < TEST_SOURCES; i++) {
#if (VECT == 1)
		FUNCTION_UNDER_TEST(TEST_LEN, TEST_SOURCES, i, g_tbls, buffs[i],
				    *dest_perf_ptrs);
#else
		FUNCTION_UNDER_TEST(TEST_LEN, TEST_SOURCES, i, g_tbls, buffs[i],
				    dest_perf_ptrs);
#endif
	}
	perf_start(&start);
	for (l = 0; l < TEST_LOOPS; l++) {
		for (j = 0; j < TEST_SOURCES; j++) {
#if (VECT == 1)
			gf_vect_mul_init(gf[0][j], &g_tbls[j * 32]);
#elif (VECT == 2)
			gf_vect_mul_init(gf[0][j], &g_tbls[j * 32]);
			gf_vect_mul_init(gf[1][j], &g_tbls[(32 * TEST_SOURCES) + (j * 32)]);
#elif (VECT == 3)
			gf_vect_mul_init(gf[0][j], &g_tbls[j * 32]);
			gf_vect_mul_init(gf[1][j], &g_tbls[(32 * TEST_SOURCES) + (j * 32)]);
			gf_vect_mul_init(gf[2][j], &g_tbls[(64 * TEST_SOURCES) + (j * 32)]);
#elif (VECT == 4)
			gf_vect_mul_init(gf[0][j], &g_tbls[j * 32]);
			gf_vect_mul_init(gf[1][j], &g_tbls[(32 * TEST_SOURCES) + (j * 32)]);
			gf_vect_mul_init(gf[2][j], &g_tbls[(64 * TEST_SOURCES) + (j * 32)]);
			gf_vect_mul_init(gf[3][j], &g_tbls[(96 * TEST_SOURCES) + (j * 32)]);
#elif (VECT == 5)
			gf_vect_mul_init(gf[0][j], &g_tbls[j * 32]);
			gf_vect_mul_init(gf[1][j], &g_tbls[(32 * TEST_SOURCES) + (j * 32)]);
			gf_vect_mul_init(gf[2][j], &g_tbls[(64 * TEST_SOURCES) + (j * 32)]);
			gf_vect_mul_init(gf[3][j], &g_tbls[(96 * TEST_SOURCES) + (j * 32)]);
			gf_vect_mul_init(gf[4][j], &g_tbls[(128 * TEST_SOURCES) + (j * 32)]);
#elif (VECT == 6)
			gf_vect_mul_init(gf[0][j], &g_tbls[j * 32]);
			gf_vect_mul_init(gf[1][j], &g_tbls[(32 * TEST_SOURCES) + (j * 32)]);
			gf_vect_mul_init(gf[2][j], &g_tbls[(64 * TEST_SOURCES) + (j * 32)]);
			gf_vect_mul_init(gf[3][j], &g_tbls[(96 * TEST_SOURCES) + (j * 32)]);
			gf_vect_mul_init(gf[4][j], &g_tbls[(128 * TEST_SOURCES) + (j * 32)]);
			gf_vect_mul_init(gf[5][j], &g_tbls[(160 * TEST_SOURCES) + (j * 32)]);
#endif
		}
		for (i = 0; i < TEST_SOURCES; i++) {
#if (VECT == 1)
			FUNCTION_UNDER_TEST(TEST_LEN, TEST_SOURCES, i, g_tbls, buffs[i],
					    *dest_perf_ptrs);
#else
			FUNCTION_UNDER_TEST(TEST_LEN, TEST_SOURCES, i, g_tbls, buffs[i],
					    dest_perf_ptrs);
#endif
		}

	}
	perf_stop(&stop);
	printf(xstr(FUNCTION_UNDER_TEST) TEST_TYPE_STR ": ");
	perf_print(stop, start, (long long)TEST_LEN * (TEST_SOURCES + VECT) * TEST_LOOPS);

	perf_start(&start);
	for (l = 0; l < TEST_LOOPS; l++) {
		for (j = 0; j < TEST_SOURCES; j++) {
#if (VECT == 1)
			gf_vect_mul_init(gf[0][j], &g_tbls[j * 32]);
#elif (VECT == 2)
			gf_vect_mul_init(gf[0][j], &g_tbls[j * 32]);
			gf_vect_mul_init(gf[1][j], &g_tbls[(32 * TEST_SOURCES) + (j * 32)]);
#elif (VECT == 3)
			gf_vect_mul_init(gf[0][j], &g_tbls[j * 32]);
			gf_vect_mul_init(gf[1][j], &g_tbls[(32 * TEST_SOURCES) + (j * 32)]);
			gf_vect_mul_init(gf[2][j], &g_tbls[(64 * TEST_SOURCES) + (j * 32)]);
#elif (VECT == 4)
			gf_vect_mul_init(gf[0][j], &g_tbls[j * 32]);
			gf_vect_mul_init(gf[1][j], &g_tbls[(32 * TEST_SOURCES) + (j * 32)]);
			gf_vect_mul_init(gf[2][j], &g_tbls[(64 * TEST_SOURCES) + (j * 32)]);
			gf_vect_mul_init(gf[3][j], &g_tbls[(96 * TEST_SOURCES) + (j * 32)]);
#elif (VECT == 5)
			gf_vect_mul_init(gf[0][j], &g_tbls[j * 32]);
			gf_vect_mul_init(gf[1][j], &g_tbls[(32 * TEST_SOURCES) + (j * 32)]);
			gf_vect_mul_init(gf[2][j], &g_tbls[(64 * TEST_SOURCES) + (j * 32)]);
			gf_vect_mul_init(gf[3][j], &g_tbls[(96 * TEST_SOURCES) + (j * 32)]);
			gf_vect_mul_init(gf[4][j], &g_tbls[(128 * TEST_SOURCES) + (j * 32)]);
#elif (VECT == 6)
			gf_vect_mul_init(gf[0][j], &g_tbls[j * 32]);
			gf_vect_mul_init(gf[1][j], &g_tbls[(32 * TEST_SOURCES) + (j * 32)]);
			gf_vect_mul_init(gf[2][j], &g_tbls[(64 * TEST_SOURCES) + (j * 32)]);
			gf_vect_mul_init(gf[3][j], &g_tbls[(96 * TEST_SOURCES) + (j * 32)]);
			gf_vect_mul_init(gf[4][j], &g_tbls[(128 * TEST_SOURCES) + (j * 32)]);
			gf_vect_mul_init(gf[5][j], &g_tbls[(160 * TEST_SOURCES) + (j * 32)]);
#endif
		}
#if (VECT == 1)
		FUNCTION_UNDER_TEST(TEST_LEN, TEST_SOURCES, 0, g_tbls, buffs[0],
				    *dest_perf_ptrs);
#else
		FUNCTION_UNDER_TEST(TEST_LEN, TEST_SOURCES, 0, g_tbls, buffs[0],
				    dest_perf_ptrs);
#endif

	}
	perf_stop(&stop);
	printf(xstr(FUNCTION_UNDER_TEST) "_single_src" TEST_TYPE_STR ": ");
	perf_print(stop, start, (long long)TEST_LEN * (1 + VECT) * TEST_LOOPS);

	perf_start(&start);
	for (l = 0; l < TEST_LOOPS; l++) {
#if (VECT == 1)
		FUNCTION_UNDER_TEST(TEST_LEN, TEST_SOURCES, 0, g_tbls, buffs[0],
				    *dest_perf_ptrs);
#else
		FUNCTION_UNDER_TEST(TEST_LEN, TEST_SOURCES, 0, g_tbls, buffs[0],
				    dest_perf_ptrs);
#endif

	}
	perf_stop(&stop);
	printf(xstr(FUNCTION_UNDER_TEST) "_single_src_simple" TEST_TYPE_STR ": ");
	perf_print(stop, start, (long long)TEST_LEN * (1 + VECT) * TEST_LOOPS);

	printf("pass perf check\n");
	return 0;

}
