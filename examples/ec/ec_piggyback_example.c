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
#include <stdlib.h>
#include <string.h>
#include <getopt.h>
#include "erasure_code.h"	// use <isa-l.h> instead when linking against installed
#include "test.h"

#define MMAX 255
#define KMAX 255

typedef unsigned char u8;
int verbose = 0;

int usage(void)
{
	fprintf(stderr,
		"Usage: ec_piggyback_example [options]\n"
		"  -h        Help\n"
		"  -k <val>  Number of source fragments\n"
		"  -p <val>  Number of parity fragments\n"
		"  -l <val>  Length of fragments\n"
		"  -e <val>  Simulate erasure on frag index val. Zero based. Can be repeated.\n"
		"  -v        Verbose\n"
		"  -b        Run timed benchmark\n"
		"  -s        Toggle use of sparse matrix opt\n"
		"  -r <seed> Pick random (k, p) with seed\n");
	exit(0);
}

// Cauchy-based matrix
void gf_gen_full_pb_cauchy_matrix(u8 * a, int m, int k)
{
	int i, j, p = m - k;

	// Identity matrix in top k x k to indicate a symetric code
	memset(a, 0, k * m);
	for (i = 0; i < k; i++)
		a[k * i + i] = 1;

	for (i = k; i < (k + p / 2); i++) {
		for (j = 0; j < k / 2; j++)
			a[k * i + j] = gf_inv(i ^ j);
		for (; j < k; j++)
			a[k * i + j] = 0;
	}
	for (; i < m; i++) {
		for (j = 0; j < k / 2; j++)
			a[k * i + j] = 0;
		for (; j < k; j++)
			a[k * i + j] = gf_inv((i - p / 2) ^ (j - k / 2));
	}

	// Fill in mixture of B parity depending on a few localized A sources
	int r = 0, c = 0;
	int repeat_len = k / (p - 2);
	int parity_rows = p / 2;

	for (i = 1 + k + parity_rows; i < m; i++, r++) {
		if (r == (parity_rows - 1) - ((k / 2 % (parity_rows - 1))))
			repeat_len++;

		for (j = 0; j < repeat_len; j++, c++)
			a[k * i + c] = gf_inv((k + 1) ^ c);
	}
}

// Vandermonde based matrix - not recommended due to limits when invertable
void gf_gen_full_pb_vand_matrix(u8 * a, int m, int k)
{
	int i, j, p = m - k;
	unsigned char q, gen = 1;

	// Identity matrix in top k x k to indicate a symetric code
	memset(a, 0, k * m);
	for (i = 0; i < k; i++)
		a[k * i + i] = 1;

	for (i = k; i < (k + (p / 2)); i++) {
		q = 1;
		for (j = 0; j < k / 2; j++) {
			a[k * i + j] = q;
			q = gf_mul(q, gen);
		}
		for (; j < k; j++)
			a[k * i + j] = 0;
		gen = gf_mul(gen, 2);
	}
	gen = 1;
	for (; i < m; i++) {
		q = 1;
		for (j = 0; j < k / 2; j++) {
			a[k * i + j] = 0;
		}
		for (; j < k; j++) {
			a[k * i + j] = q;
			q = gf_mul(q, gen);
		}
		gen = gf_mul(gen, 2);
	}

	// Fill in mixture of B parity depending on a few localized A sources
	int r = 0, c = 0;
	int repeat_len = k / (p - 2);
	int parity_rows = p / 2;

	for (i = 1 + k + parity_rows; i < m; i++, r++) {
		if (r == (parity_rows - 1) - ((k / 2 % (parity_rows - 1))))
			repeat_len++;

		for (j = 0; j < repeat_len; j++)
			a[k * i + c++] = 1;
	}
}

void print_matrix(int m, int k, unsigned char *s, const char *msg)
{
	int i, j;

	printf("%s:\n", msg);
	for (i = 0; i < m; i++) {
		printf("%3d- ", i);
		for (j = 0; j < k; j++) {
			printf(" %2x", 0xff & s[j + (i * k)]);
		}
		printf("\n");
	}
	printf("\n");
}

void print_list(int n, unsigned char *s, const char *msg)
{
	int i;
	if (!verbose)
		return;

	printf("%s: ", msg);
	for (i = 0; i < n; i++)
		printf(" %d", s[i]);
	printf("\n");
}

static int gf_gen_decode_matrix(u8 * encode_matrix,
				u8 * decode_matrix,
				u8 * invert_matrix,
				u8 * temp_matrix,
				u8 * decode_index,
				u8 * frag_err_list, int nerrs, int k, int m);

int main(int argc, char *argv[])
{
	int i, j, m, c, e, ret;
	int k = 10, p = 4, len = 8 * 1024;	// Default params
	int nerrs = 0;
	int benchmark = 0;
	int sparse_matrix_opt = 1;

	// Fragment buffer pointers
	u8 *frag_ptrs[MMAX];
	u8 *parity_ptrs[KMAX];
	u8 *recover_srcs[KMAX];
	u8 *recover_outp[KMAX];
	u8 frag_err_list[MMAX];

	// Coefficient matrices
	u8 *encode_matrix, *decode_matrix;
	u8 *invert_matrix, *temp_matrix;
	u8 *g_tbls;
	u8 decode_index[MMAX];

	if (argc == 1)
		for (i = 0; i < p; i++)
			frag_err_list[nerrs++] = rand() % (k + p);

	while ((c = getopt(argc, argv, "k:p:l:e:r:hvbs")) != -1) {
		switch (c) {
		case 'k':
			k = atoi(optarg);
			break;
		case 'p':
			p = atoi(optarg);
			break;
		case 'l':
			len = atoi(optarg);
			if (len < 0)
				usage();
			break;
		case 'e':
			e = atoi(optarg);
			frag_err_list[nerrs++] = e;
			break;
		case 'r':
			srand(atoi(optarg));
			k = (rand() % MMAX) / 4;
			k = (k < 2) ? 2 : k;
			p = (rand() % (MMAX - k)) / 4;
			p = (p < 2) ? 2 : p;
			for (i = 0; i < k && nerrs < p; i++)
				if (rand() & 1)
					frag_err_list[nerrs++] = i;
			break;
		case 'v':
			verbose++;
			break;
		case 'b':
			benchmark = 1;
			break;
		case 's':
			sparse_matrix_opt = !sparse_matrix_opt;
			break;
		case 'h':
		default:
			usage();
			break;
		}
	}
	m = k + p;

	// Check for valid parameters
	if (m > (MMAX / 2) || k > (KMAX / 2) || m < 0 || p < 2 || k < 1) {
		printf(" Input test parameter error m=%d, k=%d, p=%d, erasures=%d\n",
		       m, k, p, nerrs);
		usage();
	}
	if (nerrs > p) {
		printf(" Number of erasures chosen exceeds power of code erasures=%d p=%d\n",
		       nerrs, p);
	}
	for (i = 0; i < nerrs; i++) {
		if (frag_err_list[i] >= m)
			printf(" fragment %d not in range\n", frag_err_list[i]);
	}

	printf("ec_piggyback_example:\n");

	/*
	 * One simple way to implement piggyback codes is to keep a 2x wide matrix
	 * that covers the how each parity is related to both A and B sources.  This
	 * keeps it easy to generalize in parameters m,k and the resulting sparse
	 * matrix multiplication can be optimized by pre-removal of zero items.
	 */

	int k2 = 2 * k;
	int p2 = 2 * p;
	int m2 = k2 + p2;
	int nerrs2 = nerrs;

	encode_matrix = malloc(m2 * k2);
	decode_matrix = malloc(m2 * k2);
	invert_matrix = malloc(m2 * k2);
	temp_matrix = malloc(m2 * k2);
	g_tbls = malloc(k2 * p2 * 32);

	if (encode_matrix == NULL || decode_matrix == NULL
	    || invert_matrix == NULL || temp_matrix == NULL || g_tbls == NULL) {
		printf("Test failure! Error with malloc\n");
		return -1;
	}
	// Allocate the src fragments
	for (i = 0; i < k; i++) {
		if (NULL == (frag_ptrs[i] = malloc(len))) {
			printf("alloc error: Fail\n");
			return -1;
		}
	}
	// Allocate the parity fragments
	for (i = 0; i < p2; i++) {
		if (NULL == (parity_ptrs[i] = malloc(len / 2))) {
			printf("alloc error: Fail\n");
			return -1;
		}
	}

	// Allocate buffers for recovered data
	for (i = 0; i < p2; i++) {
		if (NULL == (recover_outp[i] = malloc(len / 2))) {
			printf("alloc error: Fail\n");
			return -1;
		}
	}

	// Fill sources with random data
	for (i = 0; i < k; i++)
		for (j = 0; j < len; j++)
			frag_ptrs[i][j] = rand();

	printf(" encode (m,k,p)=(%d,%d,%d) len=%d\n", m, k, p, len);

	// Pick an encode matrix.
	gf_gen_full_pb_cauchy_matrix(encode_matrix, m2, k2);

	if (verbose)
		print_matrix(m2, k2, encode_matrix, "encode matrix");

	// Initialize g_tbls from encode matrix
	ec_init_tables(k2, p2, &encode_matrix[k2 * k2], g_tbls);

	// Fold A and B into single list of fragments
	for (i = 0; i < k; i++)
		frag_ptrs[i + k] = &frag_ptrs[i][len / 2];

	if (!sparse_matrix_opt) {
		// Standard encode using no assumptions on the encode matrix

		// Generate EC parity blocks from sources
		ec_encode_data(len / 2, k2, p2, g_tbls, frag_ptrs, parity_ptrs);

		if (benchmark) {
			struct perf start, stop;
			unsigned long long iterations = (1ull << 32) / (m * len);
			perf_start(&start);
			for (i = 0; i < iterations; i++) {
				ec_encode_data(len / 2, k2, p2, g_tbls, frag_ptrs,
					       parity_ptrs);
			}
			perf_stop(&stop);
			printf("ec_piggyback_encode_std: ");
			perf_print(stop, start, iterations * m2 * len / 2);
		}
	} else {
		// Sparse matrix optimization - use fact that input matrix is sparse

		// Keep an encode matrix with some zero elements removed
		u8 *encode_matrix_faster, *g_tbls_faster;
		encode_matrix_faster = malloc(m * k);
		g_tbls_faster = malloc(k * p * 32);
		if (encode_matrix_faster == NULL || g_tbls_faster == NULL) {
			printf("Test failure! Error with malloc\n");
			return -1;
		}

		/*
		 * Pack with only the part that we know are non-zero.  Alternatively
		 * we could search and keep track of non-zero elements but for
		 * simplicity we just skip the lower quadrant.
		 */
		for (i = k, j = k2; i < m; i++, j++)
			memcpy(&encode_matrix_faster[k * i], &encode_matrix[k2 * j], k);

		if (verbose) {
			print_matrix(p, k, &encode_matrix_faster[k * k],
				     "encode via sparse-opt");
			print_matrix(p2 / 2, k2, &encode_matrix[(k2 + p2 / 2) * k2],
				     "encode via sparse-opt");
		}
		// Initialize g_tbls from encode matrix
		ec_init_tables(k, p, &encode_matrix_faster[k * k], g_tbls_faster);

		// Generate EC parity blocks from sources
		ec_encode_data(len / 2, k, p, g_tbls_faster, frag_ptrs, parity_ptrs);
		ec_encode_data(len / 2, k2, p, &g_tbls[k2 * p * 32], frag_ptrs,
			       &parity_ptrs[p]);

		if (benchmark) {
			struct perf start, stop;
			unsigned long long iterations = (1ull << 32) / (m * len);
			perf_start(&start);
			for (i = 0; i < iterations; i++) {
				ec_encode_data(len / 2, k, p, g_tbls_faster, frag_ptrs,
					       parity_ptrs);
				ec_encode_data(len / 2, k2, p, &g_tbls[k2 * p * 32], frag_ptrs,
					       &parity_ptrs[p]);
			}
			perf_stop(&stop);
			printf("ec_piggyback_encode_sparse: ");
			perf_print(stop, start, iterations * m2 * len / 2);
		}
	}

	if (nerrs <= 0)
		return 0;

	printf(" recover %d fragments\n", nerrs);

	// Set frag pointers to correspond to parity
	for (i = k2; i < m2; i++)
		frag_ptrs[i] = parity_ptrs[i - k2];

	print_list(nerrs2, frag_err_list, " frag err list");

	// Find a decode matrix to regenerate all erasures from remaining frags
	ret = gf_gen_decode_matrix(encode_matrix, decode_matrix,
				   invert_matrix, temp_matrix, decode_index, frag_err_list,
				   nerrs2, k2, m2);

	if (ret != 0) {
		printf("Fail on generate decode matrix\n");
		return -1;
	}
	// Pack recovery array pointers as list of valid fragments
	for (i = 0; i < k2; i++)
		if (decode_index[i] < k2)
			recover_srcs[i] = frag_ptrs[decode_index[i]];
		else
			recover_srcs[i] = parity_ptrs[decode_index[i] - k2];

	print_list(k2, decode_index, " decode index");

	// Recover data
	ec_init_tables(k2, nerrs2, decode_matrix, g_tbls);
	ec_encode_data(len / 2, k2, nerrs2, g_tbls, recover_srcs, recover_outp);

	if (benchmark) {
		struct perf start, stop;
		unsigned long long iterations = (1ull << 32) / (k * len);
		perf_start(&start);
		for (i = 0; i < iterations; i++) {
			ec_encode_data(len / 2, k2, nerrs2, g_tbls, recover_srcs,
				       recover_outp);
		}
		perf_stop(&stop);
		printf("ec_piggyback_decode: ");
		perf_print(stop, start, iterations * (k2 + nerrs2) * len / 2);
	}
	// Check that recovered buffers are the same as original
	printf(" check recovery of block {");
	for (i = 0; i < nerrs2; i++) {
		printf(" %d", frag_err_list[i]);
		if (memcmp(recover_outp[i], frag_ptrs[frag_err_list[i]], len / 2)) {
			printf(" Fail erasure recovery %d, frag %d\n", i, frag_err_list[i]);
			return -1;
		}
	}
	printf(" } done all: Pass\n");

	return 0;
}

// Generate decode matrix from encode matrix and erasure list

static int gf_gen_decode_matrix(u8 * encode_matrix,
				u8 * decode_matrix,
				u8 * invert_matrix,
				u8 * temp_matrix,
				u8 * decode_index, u8 * frag_err_list, int nerrs, int k, int m)
{
	int i, j, p, r;
	int nsrcerrs = 0;
	u8 s, *b = temp_matrix;
	u8 frag_in_err[MMAX];

	memset(frag_in_err, 0, sizeof(frag_in_err));

	// Order the fragments in erasure for easier sorting
	for (i = 0; i < nerrs; i++) {
		if (frag_err_list[i] < k)
			nsrcerrs++;
		frag_in_err[frag_err_list[i]] = 1;
	}

	// Construct b (matrix that encoded remaining frags) by removing erased rows
	for (i = 0, r = 0; i < k; i++, r++) {
		while (frag_in_err[r])
			r++;
		for (j = 0; j < k; j++)
			b[k * i + j] = encode_matrix[k * r + j];
		decode_index[i] = r;
	}
	if (verbose > 1)
		print_matrix(k, k, b, "matrix to invert");

	// Invert matrix to get recovery matrix
	if (gf_invert_matrix(b, invert_matrix, k) < 0)
		return -1;

	if (verbose > 2)
		print_matrix(k, k, invert_matrix, "matrix inverted");

	// Get decode matrix with only wanted recovery rows
	for (i = 0; i < nsrcerrs; i++) {
		for (j = 0; j < k; j++) {
			decode_matrix[k * i + j] = invert_matrix[k * frag_err_list[i] + j];
		}
	}

	// For non-src (parity) erasures need to multiply encode matrix * invert
	for (p = nsrcerrs; p < nerrs; p++) {
		for (i = 0; i < k; i++) {
			s = 0;
			for (j = 0; j < k; j++)
				s ^= gf_mul(invert_matrix[j * k + i],
					    encode_matrix[k * frag_err_list[p] + j]);

			decode_matrix[k * p + i] = s;
		}
	}
	if (verbose > 1)
		print_matrix(nerrs, k, decode_matrix, "decode matrix");
	return 0;
}
