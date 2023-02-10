/**********************************************************************
  Copyright(c) 2011-2023 Intel Corporation All rights reserved.

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

/**
 *  @file crc_combine_example.c
 *  @brief Example of CRC combine logic.
 *
 *  Combine functions can produce the CRC from independent pieces as though they
 *  were computed sequentially. The example includes combine functions that are
 *  split into two parts; one that depends on length only, and another with
 *  optimizations that uses the previous and individual CRCs to combine. By
 *  splitting, the length-dependent constants can be pre-computed and the
 *  remaining combine logic kept fast and simple.
 *
 */

// Compile as c++ for multi-function versions

#include <stdio.h>
#include <inttypes.h>
#include <immintrin.h>
#include <isa-l.h>

int verbose;			// Global for tests

#if defined (_MSC_VER)
# define __builtin_parity(x) (__popcnt64(x) & 1)
#endif

#if defined (__GNUC__) || defined (__clang__)
# define ATTRIBUTE_TARGET(x) __attribute__((target(x)))
#else
# define ATTRIBUTE_TARGET(x)
#endif

struct crc64_desc {
	uint64_t poly;
	uint64_t k5;
	uint64_t k7;
	uint64_t k8;
};

void gen_crc64_refl_consts(uint64_t poly, struct crc64_desc *c)
{
	uint64_t quotienth = 0;
	uint64_t div;
	uint64_t rem = 1ull;
	int i;

	for (i = 0; i < 64; i++) {
		div = (rem & 1ull) != 0;
		quotienth = (quotienth >> 1) | (div ? 0x8000000000000000ull : 0);
		rem = (div ? poly : 0) ^ (rem >> 1);
	}
	c->k5 = rem;
	c->poly = poly;
	c->k7 = quotienth;
	c->k8 = poly << 1;
}

void gen_crc64_norm_consts(uint64_t poly, struct crc64_desc *c)
{
	uint64_t quotientl = 0;
	uint64_t div;
	uint64_t rem = 1ull << 63;
	int i;

	for (i = 0; i < 65; i++) {
		div = (rem & 0x8000000000000000ull) != 0;
		quotientl = (quotientl << 1) | div;
		rem = (div ? poly : 0) ^ (rem << 1);
	}

	c->poly = poly;
	c->k5 = rem;
	c->k7 = quotientl;
	c->k8 = poly;
}

uint32_t calc_xi_mod(int n)
{
	uint32_t rem = 0x1ul;
	int i, j;

	const uint32_t poly = 0x82f63b78;

	if (n < 16)
		return 0;

	for (i = 0; i < n - 8; i++)
		for (j = 0; j < 8; j++)
			rem = (rem & 0x1ul) ? (rem >> 1) ^ poly : (rem >> 1);

	return rem;
}

uint64_t calc64_refl_xi_mod(int n, struct crc64_desc *c)
{
	uint64_t rem = 1ull;
	int i, j;

	const uint64_t poly = c->poly;

	if (n < 32)
		return 0;

	for (i = 0; i < n - 16; i++)
		for (j = 0; j < 8; j++)
			rem = (rem & 0x1ull) ? (rem >> 1) ^ poly : (rem >> 1);

	return rem;
}

uint64_t calc64_norm_xi_mod(int n, struct crc64_desc *c)
{
	uint64_t rem = 1ull;
	int i, j;

	const uint64_t poly = c->poly;

	if (n < 32)
		return 0;

	for (i = 0; i < n - 8; i++)
		for (j = 0; j < 8; j++)
			rem = (rem & 0x8000000000000000ull ? poly : 0) ^ (rem << 1);

	return rem;
}

// Base function for crc32_iscsi_shiftx() if c++ multi-function versioning
#ifdef __cplusplus

static inline uint32_t bit_reverse32(uint32_t x)
{
	x = (((x & 0xaaaaaaaa) >> 1) | ((x & 0x55555555) << 1));
	x = (((x & 0xcccccccc) >> 2) | ((x & 0x33333333) << 2));
	x = (((x & 0xf0f0f0f0) >> 4) | ((x & 0x0f0f0f0f) << 4));
	x = (((x & 0xff00ff00) >> 8) | ((x & 0x00ff00ff) << 8));
	return ((x >> 16) | (x << 16));
}

// Base function for crc32_iscsi_shiftx without clmul optimizations

ATTRIBUTE_TARGET("default")
uint32_t crc32_iscsi_shiftx(uint32_t crc1, uint32_t x)
{
	int i;
	uint64_t xrev, q = 0;
	union {
		uint8_t a[8];
		uint64_t q;
	} qu;

	xrev = bit_reverse32(x);
	xrev <<= 32;

	for (i = 0; i < 64; i++, xrev >>= 1)
		q = (q << 1) | __builtin_parity(crc1 & xrev);

	qu.q = q;
	return crc32_iscsi(qu.a, 8, 0);
}
#endif // cplusplus

ATTRIBUTE_TARGET("pclmul,sse4.2")
uint32_t crc32_iscsi_shiftx(uint32_t crc1, uint32_t x)
{
	__m128i crc1x, constx;
	uint64_t crc64;

	crc1x = _mm_setr_epi32(crc1, 0, 0, 0);
	constx = _mm_setr_epi32(x, 0, 0, 0);
	crc1x = _mm_clmulepi64_si128(crc1x, constx, 0);
	crc64 = _mm_cvtsi128_si64(crc1x);
	crc64 = _mm_crc32_u64(0, crc64);
	return crc64 & 0xffffffff;
}

ATTRIBUTE_TARGET("pclmul,sse4.2")
uint64_t crc64_refl_shiftx(uint64_t crc1, uint64_t x, struct crc64_desc *c)
{
	__m128i crc1x, crc2x, crc3x, constx;
	const __m128i rk5 = _mm_loadu_si64(&c->k5);
	const __m128i rk7 = _mm_loadu_si128((__m128i *) & c->k7);

	crc1x = _mm_cvtsi64_si128(crc1);
	constx = _mm_cvtsi64_si128(x);
	crc1x = _mm_clmulepi64_si128(crc1x, constx, 0x00);

	// Fold to 64b
	crc2x = _mm_clmulepi64_si128(crc1x, rk5, 0x00);
	crc3x = _mm_bsrli_si128(crc1x, 8);
	crc1x = _mm_xor_si128(crc2x, crc3x);

	// Reduce
	crc2x = _mm_clmulepi64_si128(crc1x, rk7, 0x00);
	crc3x = _mm_clmulepi64_si128(crc2x, rk7, 0x10);
	crc2x = _mm_bslli_si128(crc2x, 8);
	crc1x = _mm_xor_si128(crc1x, crc2x);
	crc1x = _mm_xor_si128(crc1x, crc3x);
	return _mm_extract_epi64(crc1x, 1);
}

ATTRIBUTE_TARGET("pclmul,sse4.2")
uint64_t crc64_norm_shiftx(uint64_t crc1, uint64_t x, struct crc64_desc *c)
{
	__m128i crc1x, crc2x, crc3x, constx;
	const __m128i rk5 = _mm_loadu_si64(&c->k5);
	const __m128i rk7 = _mm_loadu_si128((__m128i *) & c->k7);

	crc1x = _mm_cvtsi64_si128(crc1);
	constx = _mm_cvtsi64_si128(x);
	crc1x = _mm_clmulepi64_si128(crc1x, constx, 0x00);

	// Fold to 64b
	crc2x = _mm_clmulepi64_si128(crc1x, rk5, 0x01);
	crc3x = _mm_bslli_si128(crc1x, 8);
	crc1x = _mm_xor_si128(crc2x, crc3x);

	// Reduce
	crc2x = _mm_clmulepi64_si128(crc1x, rk7, 0x01);
	crc2x = _mm_xor_si128(crc1x, crc2x);
	crc3x = _mm_clmulepi64_si128(crc2x, rk7, 0x11);
	crc1x = _mm_xor_si128(crc1x, crc3x);
	return _mm_extract_epi64(crc1x, 0);
}

uint32_t crc32_iscsi_combine_4k(uint32_t * crc_array, int n)
{
	const uint32_t cn4k = 0x82f89c77;	//calc_xi_mod(4*1024);
	int i;

	if (n < 1)
		return 0;

	uint32_t crc = crc_array[0];

	for (i = 1; i < n; i++)
		crc = crc32_iscsi_shiftx(crc, cn4k) ^ crc_array[i];

	return crc;
}

// Tests

#define printv(...) {if (verbose) printf(__VA_ARGS__); else printf(".");}

uint64_t test_combine64(uint8_t * inp, size_t len, uint64_t poly, int reflected,
			uint64_t(*func) (uint64_t, const uint8_t *, uint64_t))
{

	uint64_t crc64_init, crc64, crc64a, crc64b;
	uint64_t crc64_1, crc64_2, crc64_3, crc64_n, err = 0;
	uint64_t xi_mod;
	struct crc64_desc crc64_c;
	size_t l1, l2, l3;

	l1 = len / 2;
	l2 = len - l1;

	crc64_init = rand();
	crc64 = func(crc64_init, inp, len);
	printv("\ncrc64 all       = 0x%" PRIx64 "\n", crc64);

	// Do a sequential crc update
	crc64a = func(crc64_init, &inp[0], l1);
	crc64b = func(crc64a, &inp[l1], l2);
	printv("crc64 seq       = 0x%" PRIx64 "\n", crc64b);

	// Split into 2 independent crc calc and combine
	crc64_1 = func(crc64_init, &inp[0], l1);
	crc64_2 = func(0, &inp[l1], l2);

	if (reflected) {
		gen_crc64_refl_consts(poly, &crc64_c);
		xi_mod = calc64_refl_xi_mod(l1, &crc64_c);
		crc64_1 = crc64_refl_shiftx(crc64_1, xi_mod, &crc64_c);
	} else {
		gen_crc64_norm_consts(poly, &crc64_c);
		xi_mod = calc64_norm_xi_mod(l1, &crc64_c);
		crc64_1 = crc64_norm_shiftx(crc64_1, xi_mod, &crc64_c);
	}
	crc64_n = crc64_1 ^ crc64_2;

	printv("crc64 combined2 = 0x%" PRIx64 "\n", crc64_n);
	err |= crc64_n ^ crc64;
	if (err)
		return err;

	// Split into 3 uneven segments and combine
	l1 = len / 3;
	l2 = (len / 3) - 3;
	l3 = len - l2 - l1;
	crc64_1 = func(crc64_init, &inp[0], l1);
	crc64_2 = func(0, &inp[l1], l2);
	crc64_3 = func(0, &inp[l1 + l2], l3);
	if (reflected) {
		xi_mod = calc64_refl_xi_mod(l3, &crc64_c);
		crc64_2 = crc64_refl_shiftx(crc64_2, xi_mod, &crc64_c);
		xi_mod = calc64_refl_xi_mod(len - l1, &crc64_c);
		crc64_1 = crc64_refl_shiftx(crc64_1, xi_mod, &crc64_c);
	} else {
		xi_mod = calc64_norm_xi_mod(l3, &crc64_c);
		crc64_2 = crc64_norm_shiftx(crc64_2, xi_mod, &crc64_c);
		xi_mod = calc64_norm_xi_mod(len - l1, &crc64_c);
		crc64_1 = crc64_norm_shiftx(crc64_1, xi_mod, &crc64_c);
	}
	crc64_n = crc64_1 ^ crc64_2 ^ crc64_3;

	printv("crc64 combined3 = 0x%" PRIx64 "\n", crc64_n);
	err |= crc64_n ^ crc64;

	return err;
}

#define N (1024)
#define B (2*N)
#define T (3*N)
#define N4k (4*1024)
#define NMAX 32
#define NMAX_SIZE (NMAX * N4k)

int main(int argc, char *argv[])
{
	int i;
	uint32_t crc, crca, crcb, crc1, crc2, crc3, crcn;
	uint32_t crc_init = rand();
	uint32_t err = 0;
	uint8_t *inp = (uint8_t *) malloc(NMAX_SIZE);
	verbose = argc - 1;

	if (NULL == inp)
		return -1;

	for (int i = 0; i < NMAX_SIZE; i++)
		inp[i] = rand();

	printf("crc_combine_test:");

	// Calc crc all at once
	crc = crc32_iscsi(inp, B, crc_init);
	printv("\ncrcB all       = 0x%" PRIx32 "\n", crc);

	// Do a sequential crc update
	crca = crc32_iscsi(&inp[0], N, crc_init);
	crcb = crc32_iscsi(&inp[N], N, crca);
	printv("crcB seq       = 0x%" PRIx32 "\n", crcb);

	// Split into 2 independent crc calc and combine
	crc1 = crc32_iscsi(&inp[0], N, crc_init);
	crc2 = crc32_iscsi(&inp[N], N, 0);
	crcn = crc32_iscsi_shiftx(crc1, calc_xi_mod(N)) ^ crc2;
	printv("crcB combined2 = 0x%" PRIx32 "\n", crcn);
	err |= crcn ^ crc;

	// Split into 3 uneven segments and combine
	crc1 = crc32_iscsi(&inp[0], 100, crc_init);
	crc2 = crc32_iscsi(&inp[100], 100, 0);
	crc3 = crc32_iscsi(&inp[200], B - 200, 0);
	crcn = crc3 ^
	    crc32_iscsi_shiftx(crc2, calc_xi_mod(B - 200)) ^
	    crc32_iscsi_shiftx(crc1, calc_xi_mod(B - 100));
	printv("crcB combined3 = 0x%" PRIx32 "\n\n", crcn);
	err |= crcn ^ crc;

	// Call all size T at once
	crc = crc32_iscsi(inp, T, crc_init);
	printv("crcT all       = 0x%" PRIx32 "\n", crc);

	// Split into 3 segments and combine with 2 consts
	crc1 = crc32_iscsi(&inp[0], N, crc_init);
	crc2 = crc32_iscsi(&inp[N], N, 0);
	crc3 = crc32_iscsi(&inp[2 * N], N, 0);
	crcn = crc3 ^
	    crc32_iscsi_shiftx(crc2, calc_xi_mod(N)) ^
	    crc32_iscsi_shiftx(crc1, calc_xi_mod(2 * N));
	printv("crcT combined3 = 0x%" PRIx32 "\n", crcn);
	err |= crcn ^ crc;

	// Combine 3 segments with one const by sequential shift
	uint32_t xi_mod_n = calc_xi_mod(N);
	crcn = crc3 ^ crc32_iscsi_shiftx(crc32_iscsi_shiftx(crc1, xi_mod_n)
					 ^ crc2, xi_mod_n);
	printv("crcT comb3 seq = 0x%" PRIx32 "\n\n", crcn);
	err |= crcn ^ crc;

	// Test 4k array function
	crc = crc32_iscsi(inp, NMAX_SIZE, crc_init);
	printv("crc 4k x n all = 0x%" PRIx32 "\n", crc);

	// Test crc 4k array combine function
	uint32_t crcs[NMAX];
	crcs[0] = crc32_iscsi(&inp[0], N4k, crc_init);
	for (i = 1; i < NMAX; i++)
		crcs[i] = crc32_iscsi(&inp[i * N4k], N4k, 0);

	crcn = crc32_iscsi_combine_4k(crcs, NMAX);
	printv("crc4k_array    = 0x%" PRIx32 "\n", crcn);
	err |= crcn ^ crc;

	// CRC64 generic poly tests - reflected
	uint64_t len = NMAX_SIZE;
	err |= test_combine64(inp, len, 0xc96c5795d7870f42ull, 1, crc64_ecma_refl);
	err |= test_combine64(inp, len, 0xd800000000000000ull, 1, crc64_iso_refl);
	err |= test_combine64(inp, len, 0x95ac9329ac4bc9b5ull, 1, crc64_jones_refl);

	// CRC64 non-reflected polynomial tests
	err |= test_combine64(inp, len, 0x42f0e1eba9ea3693ull, 0, crc64_ecma_norm);
	err |= test_combine64(inp, len, 0x000000000000001bull, 0, crc64_iso_norm);
	err |= test_combine64(inp, len, 0xad93d23594c935a9ull, 0, crc64_jones_norm);

	printf(err == 0 ? "pass\n" : "fail\n");
	free(inp);
	return err;
}
