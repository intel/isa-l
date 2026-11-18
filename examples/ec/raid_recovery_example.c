/**********************************************************************
  Copyright(c) 2026 Intel Corporation All rights reserved.

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
#include "raid.h"
#include "erasure_code.h"
#include "test.h"

#define MAX_DATA     255
#define TEST_SOURCES 16
#define TEST_LEN     (8 * 1024)

/*
 * This example restores data blocks when their indexes are already known to be
 * missing. It does not locate silent corruption, and it assumes both parity
 * blocks and every non-erased data block are valid.
 */
/*
 * All block pointers must reference distinct buffers. Missing data buffers are
 * writable outputs; parity buffers are inputs. Preserve data order because the
 * Q coefficient for each block depends on its index. Buffers must be 32-byte
 * aligned, and len must be a positive multiple of 32.
 */
/*
 * Recover one missing data block.
 * @param sources  Number of data blocks.
 * @param len      Bytes per block.
 * @param data     Data buffers; data[missing] is the writable output.
 * @param parity   Intact P parity buffer.
 * @param missing  Index of the missing data block.
 */
static int
recover_one_data(int sources, int len, unsigned char *const *data, const unsigned char *parity,
                 int missing)
{
        void *vectors[MAX_DATA + 1];
        int index, count = 0;

        if (sources < 2 || sources > MAX_DATA || missing < 0 || missing >= sources)
                return -1;

        /* XOR of the surviving data and P equals the missing data block. */
        for (index = 0; index < sources; index++)
                if (index != missing)
                        vectors[count++] = data[index];
        /* xor_gen's source-pointer API is not const-qualified. */
        vectors[count++] = (void *) parity;
        vectors[count++] = data[missing]; /* xor_gen writes to the last pointer. */
        return xor_gen(count, len, vectors);
}

/*
 * Recover two missing data blocks.
 * @param sources  Number of data blocks.
 * @param len      Bytes per block.
 * @param data     Data buffers; data[first] and data[second] are outputs.
 * @param parity_p Intact P parity buffer.
 * @param parity_q Intact Q parity buffer.
 * @param first    Index of the first missing data block.
 * @param second   Index of the second missing data block.
 */
static int
recover_two_data(int sources, int len, unsigned char *const *data, const unsigned char *parity_p,
                 const unsigned char *parity_q, int first, int second)
{
        unsigned char weights[MAX_DATA], coefficients[2 * MAX_DATA];
        unsigned char tables[32 * 2 * MAX_DATA], inverse;
        unsigned char *survivors[MAX_DATA], *outputs[2];
        int index, count = 0;

        if (sources < 2 || sources > MAX_DATA || first < 0 || first >= sources || second < 0 ||
            second >= sources || first == second)
                return -1;

        /* Match pq_gen's Q coefficients: powers of 2 in GF(2^8). */
        weights[0] = 1;
        for (index = 1; index < sources; index++)
                weights[index] = gf_mul(weights[index - 1], 2);
        /* Distinct indexes have distinct weights, so this denominator is nonzero. */
        inverse = gf_inv(weights[first] ^ weights[second]);

        /*
         * After removing surviving data, P' = A + B and Q' = a*A + b*B.
         * Thus A = (Q' + b*P')/(a+b), B = (Q' + a*P')/(a+b).
         * Fold removal of surviving data into two rows of coefficients. Field
         * addition/subtraction is XOR; multiplication uses gf_mul.
         */
        for (index = 0; index < sources; index++) {
                if (index == first || index == second)
                        continue;
                survivors[count] = data[index];
                coefficients[count] = gf_mul(weights[index] ^ weights[second], inverse);
                coefficients[sources + count] = gf_mul(weights[index] ^ weights[first], inverse);
                count++;
        }
        /* ec_encode_data's source-pointer API is not const-qualified. */
        survivors[count] = (unsigned char *) parity_p;
        coefficients[count] = gf_mul(weights[second], inverse);
        coefficients[sources + count] = gf_mul(weights[first], inverse);
        count++;
        survivors[count] = (unsigned char *) parity_q;
        coefficients[count] = inverse;
        coefficients[sources + count] = inverse;
        outputs[0] = data[first];
        outputs[1] = data[second];

        /*
         * ec_encode_data consumes sources coefficients per output row. The first
         * row starts at coefficients[0], the second at coefficients[sources].
         * There are sources-2 surviving data blocks plus P and Q inputs.
         */
        /* Encoding with decoding coefficients reconstructs the missing data. */
        ec_init_tables(sources, 2, coefficients, tables);
        ec_encode_data(len, sources, 2, tables, survivors, outputs);
        return 0;
}

int
main(void)
{
        unsigned char *data[TEST_SOURCES + 2] = { NULL };
        unsigned char *original = NULL;
        void *vectors[TEST_SOURCES + 2];
        int index, offset, sources, first, second;
        int single_cases = 0, double_cases = 0, ret = 1;

        original = malloc(TEST_SOURCES * TEST_LEN);
        if (original == NULL) {
                fprintf(stderr, "Failed to allocate original data\n");
                goto cleanup;
        }
        /* Each input/output block needs the alignment required by pq_gen(). */
        for (index = 0; index < TEST_SOURCES + 2; index++) {
                if (posix_memalign(&vectors[index], 32, TEST_LEN) != 0) {
                        fprintf(stderr, "Failed to allocate aligned block\n");
                        goto cleanup;
                }
                data[index] = vectors[index];
        }

        /* Exercise every source count and every one- or two-data-block erasure. */
        for (sources = 2; sources <= TEST_SOURCES; sources++) {
                /* Retain a separate copy so recovery can be checked byte-for-byte. */
                for (index = 0; index < sources; index++) {
                        for (offset = 0; offset < TEST_LEN; offset++)
                                original[index * TEST_LEN + offset] =
                                        (unsigned char) (offset * 37 + index * 71 +
                                                         (offset ^ index) * 13);
                        memcpy(data[index], original + index * TEST_LEN, TEST_LEN);
                }

                /* The final two pointers receive P and Q, respectively. */
                if (pq_gen(sources + 2, TEST_LEN, vectors) != 0) {
                        fprintf(stderr, "P/Q generation failed\n");
                        goto cleanup;
                }
                for (first = 0; first < sources; first++) {
                        /* Zero the known missing block to simulate its loss. */
                        memset(data[first], 0, TEST_LEN);
                        if (recover_one_data(sources, TEST_LEN, data, data[sources], first) != 0 ||
                            memcmp(data[first], original + first * TEST_LEN, TEST_LEN) != 0) {
                                fprintf(stderr, "Single recovery failed: sources=%d missing=%d\n",
                                        sources, first);
                                goto cleanup;
                        }
                        single_cases++;
                        for (second = first + 1; second < sources; second++) {
                                /* Recreate a two-block erasure for this pair. */
                                memset(data[first], 0, TEST_LEN);
                                memset(data[second], 0, TEST_LEN);
                                if (recover_two_data(sources, TEST_LEN, data, data[sources],
                                                     data[sources + 1], first, second) != 0 ||
                                    memcmp(data[first], original + first * TEST_LEN, TEST_LEN) !=
                                            0 ||
                                    memcmp(data[second], original + second * TEST_LEN, TEST_LEN) !=
                                            0) {
                                        fprintf(stderr,
                                                "Double recovery failed: sources=%d "
                                                "missing=%d,%d\n",
                                                sources, first, second);
                                        goto cleanup;
                                }
                                double_cases++;
                        }
                }
                /* Restored data should reproduce the original P and Q parity. */
                if (pq_check(sources + 2, TEST_LEN, vectors) != 0) {
                        fprintf(stderr, "Parity check failed after recovery\n");
                        goto cleanup;
                }
        }
        printf("RAID recovery: %d single-data and %d two-data cases: Pass\n", single_cases,
               double_cases);
        ret = 0;
cleanup:
        /* aligned_free() is the matching release for posix_memalign() here. */
        for (index = 0; index < TEST_SOURCES + 2; index++)
                aligned_free(data[index]);
        free(original);
        return ret;
}