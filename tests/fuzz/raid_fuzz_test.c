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
#include "raid.h"

int
LLVMFuzzerTestOneInput(const uint8_t *, size_t);

/* Maximum values for test parameters */
#define MAX_VECTS 16    // Maximum number of vectors
#define MAX_LEN   16384 // Maximum length of each vector

/* ========================================================================== */
/* Generic helper function for RAID testing */
/* ========================================================================== */

/**
 * Generic RAID function test helper
 *
 * Handles buffer allocation, data population, RAID function invocation, and cleanup.
 * This eliminates code duplication across all RAID test functions.
 *
 * @param buff         Fuzz input buffer
 * @param vects        Number of vectors
 * @param len          Length of each vector
 * @param raid_func    Pointer to RAID function to test
 * @return 0 on success, -1 on allocation failure
 */
static int
test_raid_generic(uint8_t *buff, int vects, int len, int alignment,
                  int (*raid_func)(int vects, int len, void **array))
{
        /* gen functions require 32-byte aligned pointers and lengths
                  check functions require 16-byte alignment */
        int len_rounded = len & ~(alignment - 1);
        if (len_rounded == 0)
                return 0;

        /* Allocate array of pointers */
        void **array = malloc(vects * sizeof(void *));
        if (array == NULL)
                return -1;

        /* Allocate properly aligned buffers for each vector */
        for (int i = 0; i < vects; i++) {
                /* Use posix_memalign for required alignment */
                if (posix_memalign(&array[i], alignment, len_rounded)) {
                        /* Free previously allocated buffers on failure */
                        for (int j = 0; j < i; j++)
                                free(array[j]);
                        free(array);
                        return -1;
                }
                /* Copy data from fuzz input to aligned buffer */
                memcpy(array[i], buff + 3 + i * len, len_rounded);
        }

        /* Execute the RAID function under test */
        raid_func(vects, len_rounded, array);

        /* Cleanup all allocated buffers */
        for (int i = 0; i < vects; i++)
                free(array[i]);
        free(array);

        return 0;
}

/* ========================================================================== */
/* RAID function wrappers */
/* ========================================================================== */

/**
 * Test xor_gen function (multi-binary, optimized version)
 */
static int
test_xor_gen(uint8_t *buff, size_t dataSize)
{
        const int vects = 3 + (buff[0] % (MAX_VECTS - 2)); // vects must be > 2
        const int len = ((buff[1] << 8) | buff[2]) % (MAX_LEN + 1);

        const size_t required_size = 3 + vects * len;
        if (dataSize < required_size)
                return 0;

        return test_raid_generic(buff, vects, len, 32, xor_gen);
}

/**
 * Test xor_gen baseline function
 */
static int
test_xor_gen_base(uint8_t *buff, size_t dataSize)
{
        const int vects = 3 + (buff[0] % (MAX_VECTS - 2));
        const int len = ((buff[1] << 8) | buff[2]) % (MAX_LEN + 1);

        const size_t required_size = 3 + vects * len;
        if (dataSize < required_size)
                return 0;

        return test_raid_generic(buff, vects, len, 32, xor_gen_base);
}

/**
 * Test xor_check function (multi-binary, optimized version)
 */
static int
test_xor_check(uint8_t *buff, size_t dataSize)
{
        const int vects = 2 + (buff[0] % (MAX_VECTS - 1)); // vects must be > 1
        const int len = ((buff[1] << 8) | buff[2]) % (MAX_LEN + 1);

        const size_t required_size = 3 + vects * len;
        if (dataSize < required_size)
                return 0;

        return test_raid_generic(buff, vects, len, 16, xor_check);
}

/**
 * Test xor_check baseline function
 */
static int
test_xor_check_base(uint8_t *buff, size_t dataSize)
{
        const int vects = 2 + (buff[0] % (MAX_VECTS - 1));
        const int len = ((buff[1] << 8) | buff[2]) % (MAX_LEN + 1);

        const size_t required_size = 3 + vects * len;
        if (dataSize < required_size)
                return 0;

        return test_raid_generic(buff, vects, len, 16, xor_check_base);
}

/**
 * Test pq_gen function (multi-binary, optimized version)
 */
static int
test_pq_gen(uint8_t *buff, size_t dataSize)
{
        const int vects = 4 + (buff[0] % (MAX_VECTS - 3)); // vects must be > 3
        const int len = ((buff[1] << 8) | buff[2]) % (MAX_LEN + 1);

        const size_t required_size = 3 + vects * len;
        if (dataSize < required_size)
                return 0;

        return test_raid_generic(buff, vects, len, 32, pq_gen);
}

/**
 * Test pq_gen baseline function
 */
static int
test_pq_gen_base(uint8_t *buff, size_t dataSize)
{
        const int vects = 4 + (buff[0] % (MAX_VECTS - 3));
        const int len = ((buff[1] << 8) | buff[2]) % (MAX_LEN + 1);

        const size_t required_size = 3 + vects * len;
        if (dataSize < required_size)
                return 0;

        return test_raid_generic(buff, vects, len, 32, pq_gen_base);
}

/**
 * Test pq_check function (multi-binary, optimized version)
 */
static int
test_pq_check(uint8_t *buff, size_t dataSize)
{
        const int vects = 4 + (buff[0] % (MAX_VECTS - 3)); // vects must be > 3
        const int len = ((buff[1] << 8) | buff[2]) % (MAX_LEN + 1);

        const size_t required_size = 3 + vects * len;
        if (dataSize < required_size)
                return 0;

        return test_raid_generic(buff, vects, len, 16, pq_check);
}

/**
 * Test pq_check baseline function
 */
static int
test_pq_check_base(uint8_t *buff, size_t dataSize)
{
        const int vects = 4 + (buff[0] % (MAX_VECTS - 3));
        const int len = ((buff[1] << 8) | buff[2]) % (MAX_LEN + 1);

        const size_t required_size = 3 + vects * len;
        if (dataSize < required_size)
                return 0;

        return test_raid_generic(buff, vects, len, 16, pq_check_base);
}

/* ========================================================================== */
/* Main fuzzer entry point */
/* ========================================================================== */

struct {
        int (*func)(uint8_t *buff, size_t dataSize);
        const char *func_name;
} raid_test_funcs[] = {
        /* XOR generation functions */
        { test_xor_gen, "test_xor_gen" },
        { test_xor_gen_base, "test_xor_gen_base" },

        /* XOR check functions */
        { test_xor_check, "test_xor_check" },
        { test_xor_check_base, "test_xor_check_base" },

        /* P+Q generation functions */
        { test_pq_gen, "test_pq_gen" },
        { test_pq_gen_base, "test_pq_gen_base" },

        /* P+Q check functions */
        { test_pq_check, "test_pq_check" },
        { test_pq_check_base, "test_pq_check_base" },
};

#define NUM_RAID_TESTS (sizeof(raid_test_funcs) / sizeof(raid_test_funcs[0]))

int
LLVMFuzzerTestOneInput(const uint8_t *data, size_t dataSize)
{
        /* Require at least 4 bytes for all tests*/
        if (dataSize < 4)
                return 0;

        /* Use first byte to select which RAID function to test */
        const uint8_t test_selector = data[0];
        const size_t test_index = test_selector % NUM_RAID_TESTS;

        /* Allocate a working buffer for the test */
        uint8_t *buff = malloc(dataSize);
        if (buff == NULL)
                return 0;

        /* Copy input data to working buffer */
        memcpy(buff, data, dataSize);

        /* Run the selected test */
        raid_test_funcs[test_index].func(buff, dataSize);

        /* Clean up */
        free(buff);

        return 0;
}
