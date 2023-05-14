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
#include <assert.h>
#include <time.h>
#include <sys/random.h>
#include "erasure_code.h"	// use <isa-l.h> instead when linking against installed

#define MMAX 255
#define KMAX 255

typedef unsigned char u8;

int usage(void)
{
    fprintf(stderr,
        "Usage: ec_simple_example [options]\n"
        "  -h        Help\n"
        "  -f <val>  Path to file to encode\n"
        "  -k <val>  Number of source fragments\n"
        "  -p <val>  Number of parity fragments\n"
        "  -e <val>  Exhaustive testing for 1 missing fragment\n"
        "  -r <val>  Randomly test up to r missing fragments\n"
        "  -n <val>  Number of times to repeat each random test\n");
    exit(0);
}


static int gf_gen_decode_matrix_simple(
                    const u8 * encode_matrix,
                    u8 * decode_matrix, // output matrix, modified by function
                    u8 * invert_matrix,
                    u8 * temp_matrix,
                    u8 * decode_index,  // output matrix, modified by function
                    const u8 * frag_err_list,
                    int nerrs,
                    int k,
                    int m);

void test_exhaustive(
            int k,
            int m,
            int p,
            int len,
            const u8 *encode_matrix,
            u8 const * const * const frag_ptrs);


void test_random(
            int k,
            int m,
            int p,
            int nerrs,
            int len,
            const u8 *encode_matrix,
            u8 const * const * const frag_ptrs);

int test_helper(
            int k,
            int m,
            int p,
            int nerrs,
            int len,
            const u8 *encode_matrix,
            const u8 *frag_err_list,
            u8 const * const * const frag_ptrs);

void print_matrix(
    const char* name,
    const u8** matrix,
    int rows,
    int cols);

const unsigned char generate_byte(const unsigned char upper_bound){ // generates numbers in the range [0, upper_bound] - all inclusive!
    // I have thoroughly tested this function and informally proved that it is correct to my satisfaction.
    // this function will reject up to half of all generated numbers
    // this is a bit inefficient, but is ok when speed doesn't matter
    // you can fairly easily extend this to more than 255 by interpreting multiple bytes as 32 or 64 bit integer
    // you can also make a batch version, which would be faster
    const int n = 256; // the number of numbers in our input space
    assert(upper_bound < n); // the biggest number we can generate, given our input range
    const int range = upper_bound + 1; // the number of numbers in our output space
    //printf("range:%u\n", range);
    unsigned char array[1];
    const int nearest_multiple = (n - (n % range));
    assert(nearest_multiple >= n / 2);
    //printf("nearest multiple: %d\n", nearest_multiple);
    while (1){
        int r = getrandom(array, 1, 0); // generate one random byte
        assert(r == 1); // verify that we got one byte back
        unsigned char x = array[0];
        if (x < nearest_multiple){
            //printf("generated: %u\n", x);
            return x % range;
        }
    }
}


void choose_without_replacement(
    // I've thoroughly tested this function and ensured that it works.
    // this function is limited to arrays of up to 255 elements due to generate_byte, see below
    #define ELEMENT_TYPE u8
    // the #define macro is to make this function generic, so feel free to replace u8 with some other type.
    const ELEMENT_TYPE* input_array,
    ELEMENT_TYPE* output_array, // caller must allocate this, but does not need to initialize it
    int input_array_length, // number of elements in input and output array
    int output_array_length, // number of elements in input and output array
    int m, // k + p
    int elements_to_pick // number of elements to randomly pick from the input array
){
    assert(input_array_length == output_array_length);
    assert(elements_to_pick <= input_array_length);
    assert(elements_to_pick < 256); // limitation due to generate_byte
    // first, copy input array into output array
    memcpy(output_array, input_array, sizeof(ELEMENT_TYPE) * input_array_length);
    // next, pick random elements and swap them to the front of the output array
    // this is basically a Knuth shuffle
    for (int i = 0; i < elements_to_pick; i++) {
        // initially we choose between 0 and n-1
        // next we choose between 1 and n-1, and so on
        unsigned char rand_num = generate_byte(m - 1 - i);
        int random_index = i + rand_num;
        // swap the ith element with the randomly chosen element
        ELEMENT_TYPE ith_element = output_array[i];
        output_array[i] = output_array[random_index];
        output_array[random_index] = ith_element;
    }
    return;
}


void print_array(
    const char* name,
    const u8* array,
    int num_elements
){
    printf("%s: [", name);
    if (num_elements == 0){
        printf("]\n");
        return;
    }
    for (int i = 0; i < num_elements-1; i++){
        printf("%u, ", array[i]);
    }
    printf("%u", array[num_elements-1]);
    printf("]\n");
}

void print_matrix(
    const char* name,
    const u8** matrix,
    int rows,
    int cols
){
    printf("=== Begin Matrix %s ===\n", name);
    for (int i = 0; i < rows; i++){
        printf("Row %d", i);
        print_array("", matrix[i], cols);
    }
    printf("=== End Matrix ===\n");
}



void test_exhaustive(
            int k,
            int m,
            int p,
            int len,
            const u8 *encode_matrix,
            u8 const * const * const frag_ptrs)
{
    // Fragment buffer pointers
    u8 frag_err_list[MMAX];
    int nerrs = 1;
    for (int i = 0; i < m; i++){
        frag_err_list[0] = i;
        test_helper(k, m, p, nerrs, len, encode_matrix, frag_err_list, frag_ptrs);
    }
}


void test_random(
            int k,
            int m,
            int p,
            int nerrs,
            int len,
            const u8 *encode_matrix,
            u8 const * const * const frag_ptrs)
{
    // Fragment buffer pointers
    u8 frag_err_list[MMAX];
    // Generate errors
    u8 shard_numbers[MMAX];
    for (int i = 0; i < MMAX; i++){
        shard_numbers[i] = i;
    }
    choose_without_replacement(shard_numbers, frag_err_list, MMAX, MMAX, m, nerrs);
    print_array("frag_err_list", frag_err_list, nerrs);

    test_helper(k, m, p, nerrs, len, encode_matrix, frag_err_list, frag_ptrs);
}

int test_helper(
            int k,
            int m,
            int p,
            int nerrs,
            int len,
            const u8 *encode_matrix,
            const u8 *frag_err_list,
            u8 const * const * const frag_ptrs)
{
    u8 *recover_outp[KMAX];
    u8 *decode_matrix = malloc(m * k);
    u8 *invert_matrix = malloc(m * k);
    u8 *temp_matrix = malloc(m * k);
    u8 *g_tbls = malloc(k * p * 32);
    u8 decode_index[MMAX];
    const u8 * recover_srcs[KMAX];
    
    

    // Allocate buffers for recovered data
    for (int i = 0; i < p; i++) {
        if (NULL == (recover_outp[i] = malloc(len))) {
            printf("alloc error: Fail\n");
            return -1;
        }
    }

    if (encode_matrix == NULL || decode_matrix == NULL
        || invert_matrix == NULL || temp_matrix == NULL || g_tbls == NULL) {
        printf("Test failure! Error with malloc\n");
        return -1;
    }

    printf(" recover %d fragments\n", nerrs);

    // Find a decode matrix to regenerate all erasures from remaining frags
    int ret = gf_gen_decode_matrix_simple(encode_matrix, decode_matrix,
                      invert_matrix, temp_matrix, decode_index,
                      frag_err_list, nerrs, k, m);
    if (ret != 0) {
        printf("Fail on generate decode matrix\n");
        exit(-1);
    }
    // Pack recovery array pointers as list of valid fragments
    for (int i = 0; i < k; i++)
        recover_srcs[i] = frag_ptrs[decode_index[i]]; // we know that ec_encode_data doesn't modify the data...

    // Recover data
    ec_init_tables(k, nerrs, decode_matrix, g_tbls);
    ec_encode_data(len, k, nerrs, g_tbls, (const u8* const *)recover_srcs, recover_outp);


    // Check that recovered buffers are the same as original
    printf(" check recovery of block {");
    for (int i = 0; i < nerrs; i++) {
        printf(" %d", frag_err_list[i]);
        if (memcmp(recover_outp[i], frag_ptrs[frag_err_list[i]], len)) {
            printf(" Fail erasure recovery %d, frag %d\n", i, frag_err_list[i]);
            exit(-1);
        }
    }
    print_matrix("Recovered Matrix", (const u8**)recover_outp, nerrs, len);

    printf(" } done all: Pass\n");
    return 0;
}

/*
 * Generate decode matrix from encode matrix and erasure list
 *
 */

static int gf_gen_decode_matrix_simple(
                    const u8 * encode_matrix,
                    u8 * decode_matrix, // output matrix, modified by function
                    u8 * invert_matrix,
                    u8 * temp_matrix,
                    u8 * decode_index,  // output matrix, modified by function
                    const u8 * frag_err_list,
                    int nerrs,
                    int k,
                    int m)
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

    // Invert matrix to get recovery matrix
    if (gf_invert_matrix(b, invert_matrix, k) < 0)
        return -1;

    // Get decode matrix with only wanted recovery rows
    for (i = 0; i < nerrs; i++) {
        if (frag_err_list[i] < k)	// A src err
            for (j = 0; j < k; j++)
                decode_matrix[k * i + j] =
                    invert_matrix[k * frag_err_list[i] + j];
    }

    // For non-src (parity) erasures need to multiply encode matrix * invert
    for (p = 0; p < nerrs; p++) {
        if (frag_err_list[p] >= k) {	// A parity err
            for (i = 0; i < k; i++) {
                s = 0;
                for (j = 0; j < k; j++)
                    s ^= gf_mul(invert_matrix[j * k + i],
                            encode_matrix[k * frag_err_list[p] + j]);
                decode_matrix[k * p + i] = s;
            }
        }
    }
    return 0;
}



















int main(int argc, char *argv[])
{
    srand(time(NULL));
    int k = 10, p = 4, len = 8;	// Default params
    int random_test = 0;
    int random_repeat = 1;
    int exhaustive_test = 0;
	char* filepath = NULL;

    // Fragment buffer pointers
    u8 *frag_ptrs_encode[MMAX];
    u8 *frag_ptrs_encode_update[MMAX];


    // Coefficient matrices
    u8 *encode_matrix;
    u8 *g_tbls;

    int c;
    while ((c = getopt(argc, argv, "f:k:p:l:e:r:n:h")) != -1) {
        switch (c) {
        case 'f':
            filepath = strdup(optarg);
            break;
        case 'k':
            k = atoi(optarg);
            break;
        case 'p':
            p = atoi(optarg);
            break;
        case 'e':
            exhaustive_test = atoi(optarg);
            break;
        case 'r':
            random_test = atoi(optarg);
            break;
        case 'n':
            random_repeat = atoi(optarg);
            break;
        case 'h':
        default:
            usage();
            break;
        }
    }
    int m = k + p;

    // Check for valid parameters
    if (m > MMAX || k > KMAX || m < 0 || p < 1 || k < 1) {
        printf(" Input test parameter error m=%d, k=%d, p=%d\n",
               m, k, p);
        usage();
    }
    // Check for filename
    if (NULL == filepath) {
        puts("Error: You must specify a file to encode.");
        exit(-1);
    }

    printf("Encoding file: %s\n", filepath);

    printf("ec_simple_example:\n");

    // Allocate coding matrices
    encode_matrix = malloc(m * k);
    g_tbls = malloc(k * p * 32);

    if (encode_matrix == NULL || g_tbls == NULL) {
        printf("Test failure! Error with malloc\n");
        return -1;
    }
    // Allocate the src & parity buffers
    for (int i = 0; i < m; i++) {
        if (NULL == (frag_ptrs_encode[i] = malloc(len))) {
            printf("alloc 1 error: Fail\n");
            return -1;
        }
        if (NULL == (frag_ptrs_encode_update[i] = malloc(len))) {
            printf("alloc 2 error: Fail\n");
            return -1;
        }
    }

    // Fill sources with random data
    for (int i = 0; i < k; i++)
        for (int j = 0; j < len; j++){
            frag_ptrs_encode[i][j] = rand();
            frag_ptrs_encode_update[i][j] = frag_ptrs_encode[i][j];
        }

    print_matrix("Source matrix 1", (const u8**)frag_ptrs_encode, k, len);
    print_matrix("Source matrix 2", (const u8**)frag_ptrs_encode_update, k, len);

    printf(" encode (m,k,p)=(%d,%d,%d) len=%d\n", m, k, p, len);

    // Pick an encode matrix. A Cauchy matrix is a good choice as even
    // large k are always invertable keeping the recovery rule simple.
    gf_gen_cauchy1_matrix(encode_matrix, m, k);

    // Initialize g_tbls from encode matrix
    ec_init_tables(k, p, &encode_matrix[k * k], g_tbls);

    // Generate EC parity blocks from sources
    ec_encode_data(len, k, p, g_tbls, (const u8* const *)frag_ptrs_encode, &frag_ptrs_encode[k]);
    print_matrix("Source + Parity matrix encode 1", (const u8**)frag_ptrs_encode, m, len);

    // Generate EC parity blocks using progressive encoding
    for (int i = 0; i < k; i++){
        ec_encode_data_update(len, k, p, i, g_tbls, frag_ptrs_encode_update[i], &frag_ptrs_encode_update[k]);
    }

    print_matrix("Source + Parity matrix encode 2", (const u8**)frag_ptrs_encode, m, len);
    print_matrix("Source + Parity matrix encode_update", (const u8**)frag_ptrs_encode_update, m, len);

    // Check that encode and encode_update produce the same results
    printf(" check the two versions are identical ");
    for (int row = 0; row < m; row++) {
        if (memcmp(frag_ptrs_encode[row], frag_ptrs_encode_update[row], len)) {
            puts("Not identical");
            exit(-1);
        }
    }

    int nerrs = 1;
    if (exhaustive_test) {
        nerrs = 2;
        printf("======================== Exhaustive Testing 1 missing fragment ========================\n");
        test_exhaustive(k, m, p, len, (const u8*)encode_matrix, (u8 const * const * const)frag_ptrs_encode);
        test_exhaustive(k, m, p, len, (const u8*)encode_matrix, (u8 const * const * const)frag_ptrs_encode_update);
    }

    for (; nerrs <= random_test; nerrs++){
        printf("======================== Random Testing %d missing fragments ========================\n", nerrs);
        for (int j = 0; j < random_repeat; j++){
            test_random(k, m, p, nerrs, len, (const u8*)encode_matrix, (u8 const * const * const)frag_ptrs_encode);
            test_random(k, m, p, nerrs, len, (const u8*)encode_matrix, (u8 const * const * const)frag_ptrs_encode_update);
        }
    }
}
