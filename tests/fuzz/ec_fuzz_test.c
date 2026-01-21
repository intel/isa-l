/**********************************************************************
  Copyright(c) 2026, Intel Corporation All rights reserved.

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
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include "erasure_code.h"

int
LLVMFuzzerTestOneInput(const uint8_t *, size_t);

/* Maximum values for test parameters */
#define MAX_K    16    // Maximum number of source vectors
#define MAX_ROWS 16    // Maximum number of coding vectors
#define MAX_LEN  16384 // Maximum length of each vector

/* ========================================================================== */
/* Test helper functions*/
/* ========================================================================== */

/**
 * Helper function for testing ec_init_tables variants
 */
static void
test_ec_init_tables_helper(uint8_t *buff, size_t dataSize,
                           void (*func)(int k, int rows, unsigned char *a, unsigned char *gftbls))
{
        /* Extract parameters: k (1 byte), rows (1 byte) */
        const int k = 1 + (buff[0] % MAX_K);
        const int rows = 1 + (buff[1] % MAX_ROWS);

        /*
         * Required size: 2 bytes for parameters (k, rows) + k*rows bytes for coefficient matrix
         */
        const size_t required_size = 2 + k * rows;
        if (dataSize < required_size)
                return;

        unsigned char *a = buff + 2;
        unsigned char *gftbls = malloc(32 * k * rows);
        if (gftbls == NULL)
                return;

        func(k, rows, a, gftbls);

        free(gftbls);
}

/**
 * Helper function for testing ec_encode_data variants
 */
static void
test_ec_encode_data_helper(uint8_t *buff, size_t dataSize,
                           void (*encode_func)(int len, int k, int rows, unsigned char *gftbls,
                                               unsigned char **data, unsigned char **coding),
                           void (*init_func)(int k, int rows, unsigned char *a,
                                             unsigned char *gftbls))
{
        const int len = ((buff[0] << 8) | buff[1]) % (MAX_LEN + 1);
        const int k = 1 + (buff[2] % MAX_K);
        const int rows = 1 + (buff[3] % MAX_ROWS);

        /*
         * Required size: 4 bytes for parameters (2 for len, 1 for k, 1 for rows)
         *                + k*rows bytes for coefficient matrix
         *                + k*len bytes for k data buffers of length len
         */
        const size_t required_size = 4 + k * rows + k * len;
        if (dataSize < required_size)
                return;

        unsigned char *gftbls = malloc(32 * k * rows);
        unsigned char **data = malloc(k * sizeof(unsigned char *));
        unsigned char **coding = malloc(rows * sizeof(unsigned char *));

        if (gftbls == NULL || data == NULL || coding == NULL) {
                free(gftbls);
                free(data);
                free(coding);
                return;
        }

        unsigned char *a = buff + 4;
        init_func(k, rows, a, gftbls);

        unsigned char *data_buf = buff + 4 + k * rows;
        for (int i = 0; i < k; i++) {
                data[i] = data_buf + i * len;
        }

        for (int i = 0; i < rows; i++) {
                coding[i] = malloc(len);
                if (coding[i] == NULL) {
                        for (int j = 0; j < i; j++)
                                free(coding[j]);
                        free(gftbls);
                        free(data);
                        free(coding);
                        return;
                }
        }

        encode_func(len, k, rows, gftbls, data, coding);

        for (int i = 0; i < rows; i++)
                free(coding[i]);
        free(gftbls);
        free(data);
        free(coding);
}

/**
 * Helper function for testing gf_vect_dot_prod variants
 */
static void
test_gf_vect_dot_prod_helper(uint8_t *buff, size_t dataSize,
                             void (*func)(int len, int vlen, unsigned char *gftbls,
                                          unsigned char **src, unsigned char *dest))
{
        const int len = ((buff[0] << 8) | buff[1]) % (MAX_LEN + 1);
        const int vlen = 1 + (buff[2] % MAX_K);

        /*
         * Required size: 3 bytes for parameters (2 for len, 1 for vlen)
         *                + 32*vlen bytes for GF tables
         *                + vlen*len bytes for vlen source buffers of length len
         */
        const size_t required_size = 3 + 32 * vlen + vlen * len;
        if (dataSize < required_size)
                return;

        unsigned char *gftbls = buff + 3;
        unsigned char **src = malloc(vlen * sizeof(unsigned char *));
        unsigned char *dest = malloc(len);

        if (src == NULL || dest == NULL) {
                free(src);
                free(dest);
                return;
        }

        unsigned char *src_buf = buff + 3 + 32 * vlen;
        for (int i = 0; i < vlen; i++) {
                src[i] = src_buf + i * len;
        }

        func(len, vlen, gftbls, src, dest);

        free(src);
        free(dest);
}

/**
 * Helper function for testing gf_vect_mad variants
 */
static void
test_gf_vect_mad_helper(uint8_t *buff, size_t dataSize,
                        void (*func)(int len, int vec, int vec_i, unsigned char *gftbls,
                                     unsigned char *src, unsigned char *dest))
{
        const int len = ((buff[0] << 8) | buff[1]) % (MAX_LEN + 1);
        const int vec = 1 + (buff[2] % MAX_K);
        const int vec_i = buff[3] % vec;

        /*
         * Required size: 4 bytes for parameters (2 for len, 1 for vec, 1 for vec_i)
         *                + 32*vec bytes for GF tables
         *                + len bytes for source buffer
         *                + len bytes for dest initialization data
         */
        const size_t required_size = 4 + 32 * vec + len + len;
        if (dataSize < required_size)
                return;

        unsigned char *gftbls = buff + 4;
        unsigned char *src = buff + 4 + 32 * vec;
        unsigned char *dest = malloc(len);

        if (dest == NULL)
                return;

        /* Initialize dest with some data */
        unsigned char *init_data = buff + 4 + 32 * vec + len;
        memcpy(dest, init_data, len);

        func(len, vec, vec_i, gftbls, src, dest);

        free(dest);
}

/* ========================================================================== */
/* Test functions */
/* ========================================================================== */

/**
 * Test ec_init_tables function (multi-binary, optimized version)
 */
static void
test_ec_init_tables(uint8_t *buff, size_t dataSize)
{
        test_ec_init_tables_helper(buff, dataSize, ec_init_tables);
}

/**
 * Test ec_init_tables baseline function
 */
static void
test_ec_init_tables_base(uint8_t *buff, size_t dataSize)
{
        test_ec_init_tables_helper(buff, dataSize, ec_init_tables_base);
}

/**
 * Test ec_encode_data function (multi-binary, optimized version)
 */
static void
test_ec_encode_data(uint8_t *buff, size_t dataSize)
{
        test_ec_encode_data_helper(buff, dataSize, ec_encode_data, ec_init_tables);
}

/**
 * Test ec_encode_data baseline function
 */
static void
test_ec_encode_data_base(uint8_t *buff, size_t dataSize)
{
        test_ec_encode_data_helper(buff, dataSize, ec_encode_data_base, ec_init_tables_base);
}

/**
 * Test gf_vect_dot_prod function
 */
static void
test_gf_vect_dot_prod(uint8_t *buff, size_t dataSize)
{
        test_gf_vect_dot_prod_helper(buff, dataSize, gf_vect_dot_prod);
}

/**
 * Test gf_vect_dot_prod baseline function
 */
static void
test_gf_vect_dot_prod_base(uint8_t *buff, size_t dataSize)
{
        test_gf_vect_dot_prod_helper(buff, dataSize, gf_vect_dot_prod_base);
}

/**
 * Test gf_vect_mad function
 */
static void
test_gf_vect_mad(uint8_t *buff, size_t dataSize)
{
        test_gf_vect_mad_helper(buff, dataSize, gf_vect_mad);
}

/**
 * Test gf_vect_mad baseline function
 */
static void
test_gf_vect_mad_base(uint8_t *buff, size_t dataSize)
{
        test_gf_vect_mad_helper(buff, dataSize, gf_vect_mad_base);
}

/* ========================================================================== */
/* Main fuzzer entry point */
/* ========================================================================== */

struct {
        void (*func)(uint8_t *buff, size_t dataSize);
        const char *func_name;
} ec_test_funcs[] = {
        /* Init tables functions */
        { test_ec_init_tables, "test_ec_init_tables" },
        { test_ec_init_tables_base, "test_ec_init_tables_base" },

        /* Encode data functions */
        { test_ec_encode_data, "test_ec_encode_data" },
        { test_ec_encode_data_base, "test_ec_encode_data_base" },

        /* Vector dot product functions */
        { test_gf_vect_dot_prod, "test_gf_vect_dot_prod" },
        { test_gf_vect_dot_prod_base, "test_gf_vect_dot_prod_base" },

        /* Vector multiply-accumulate functions */
        { test_gf_vect_mad, "test_gf_vect_mad" },
        { test_gf_vect_mad_base, "test_gf_vect_mad_base" },
};

#define NUM_EC_TESTS (sizeof(ec_test_funcs) / sizeof(ec_test_funcs[0]))

int
LLVMFuzzerTestOneInput(const uint8_t *data, size_t dataSize)
{
        /* Require at least 4 bytes for all tests */
        if (dataSize < 4)
                return 0;

        /* Use first byte to select which EC function to test */
        const uint8_t test_selector = data[0];
        const size_t test_index = test_selector % NUM_EC_TESTS;

        /* Allocate a working buffer for the test */
        uint8_t *buff = malloc(dataSize);
        if (buff == NULL)
                return 0;

        /* Copy input data to working buffer */
        memcpy(buff, data, dataSize);

        /* Run the selected test */
        ec_test_funcs[test_index].func(buff, dataSize);

        /* Clean up */
        free(buff);

        return 0;
}
