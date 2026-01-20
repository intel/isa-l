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
#include "crc.h"
#include "crc64.h"

int
LLVMFuzzerTestOneInput(const uint8_t *, size_t);

/* ========================================================================== */
/* Helper functions to reduce code duplication */
/* ========================================================================== */

/**
 * Generic test helper for CRC16 functions
 * Extracts 2-byte init_crc from input and calls the provided CRC function
 */
static int
test_crc16_generic(uint8_t *buff, size_t dataSize,
                   uint16_t (*crc_func)(uint16_t, const uint8_t *, uint64_t))
{
        const uint16_t init_crc = (dataSize >= 2) ? ((uint16_t *) buff)[0] : 0;
        const uint8_t *data = (dataSize >= 2) ? buff + 2 : buff;
        const size_t len = (dataSize >= 2) ? dataSize - 2 : dataSize;

        crc_func(init_crc, data, len);
        return 0;
}

/**
 * Generic test helper for CRC16 copy functions
 * Allocates destination buffer and calls the provided CRC copy function
 */
static int
test_crc16_copy_generic(uint8_t *buff, size_t dataSize,
                        uint16_t (*crc_func)(uint16_t, uint8_t *, uint8_t *, uint64_t))
{
        uint8_t *dst = malloc(dataSize);
        if (dst == NULL)
                return -1;

        const uint16_t init_crc = (dataSize >= 2) ? ((uint16_t *) buff)[0] : 0;
        uint8_t *src = (dataSize >= 2) ? buff + 2 : buff;
        const size_t len = (dataSize >= 2) ? dataSize - 2 : dataSize;

        crc_func(init_crc, dst, src, len);

        free(dst);
        return 0;
}

/**
 * Generic test helper for CRC32 functions
 * Extracts 4-byte init_crc from input and calls the provided CRC function
 */
static int
test_crc32_generic(uint8_t *buff, size_t dataSize,
                   uint32_t (*crc_func)(uint32_t, const uint8_t *, uint64_t))
{
        const uint32_t init_crc = (dataSize >= 4) ? ((uint32_t *) buff)[0] : 0;
        const uint8_t *data = (dataSize >= 4) ? buff + 4 : buff;
        const size_t len = (dataSize >= 4) ? dataSize - 4 : dataSize;

        crc_func(init_crc, data, len);
        return 0;
}

/**
 * Generic test helper for CRC64 functions
 * Extracts 8-byte init_crc from input and calls the provided CRC function
 */
static int
test_crc64_generic(uint8_t *buff, size_t dataSize,
                   uint64_t (*crc_func)(uint64_t, const uint8_t *, uint64_t))
{
        const uint64_t init_crc = (dataSize >= 8) ? ((uint64_t *) buff)[0] : 0;
        const uint8_t *data = (dataSize >= 8) ? buff + 8 : buff;
        const size_t len = (dataSize >= 8) ? dataSize - 8 : dataSize;

        crc_func(init_crc, data, len);
        return 0;
}

/* ========================================================================== */
/* Wrapper functions for base implementations with non-const pointers */
/* ========================================================================== */

/**
 * Wrappers for base (baseline) CRC functions that use non-const uint8_t *
 * These cast the const pointer to non-const for compatibility
 */

static uint16_t
crc16_t10dif_base_wrapper(uint16_t init_crc, const uint8_t *buf, uint64_t len)
{
        return crc16_t10dif_base(init_crc, (uint8_t *) buf, len);
}

static uint32_t
crc32_ieee_base_wrapper(uint32_t init_crc, const uint8_t *buf, uint64_t len)
{
        return crc32_ieee_base(init_crc, (uint8_t *) buf, len);
}

static uint32_t
crc32_gzip_refl_base_wrapper(uint32_t init_crc, const uint8_t *buf, uint64_t len)
{
        return crc32_gzip_refl_base(init_crc, (uint8_t *) buf, len);
}

/* ========================================================================== */
/* CRC16 Functions */
/* ========================================================================== */

/**
 * Test CRC16 T10 DIF function (multi-binary, optimized version)
 *
 * Input format:
 * - Bytes 0-1: Initial CRC value (little-endian uint16_t)
 * - Bytes 2-n: Data to calculate CRC on
 *
 * If dataSize < 2, uses init_crc=0 and processes the single byte as data
 */
static int
test_crc16_t10dif(uint8_t *buff, size_t dataSize)
{
        return test_crc16_generic(buff, dataSize, crc16_t10dif);
}

/**
 * Test CRC16 T10 DIF copy function (stitched CRC + copy operation)
 *
 * Tests both CRC calculation and memory copy in one operation.
 * Allocates destination buffer and validates memory handling.
 */
static int
test_crc16_t10dif_copy(uint8_t *buff, size_t dataSize)
{
        return test_crc16_copy_generic(buff, dataSize, crc16_t10dif_copy);
}

/**
 * Test CRC16 T10 DIF baseline function (portable C implementation)
 *
 * Tests the non-optimized, baseline implementation used for validation
 * and platforms without hardware acceleration.
 */
static int
test_crc16_t10dif_base(uint8_t *buff, size_t dataSize)
{
        return test_crc16_generic(buff, dataSize, crc16_t10dif_base_wrapper);
}

/**
 * Test CRC16 T10 DIF copy baseline function (portable C implementation)
 *
 * Tests baseline implementation of stitched CRC + copy operation.
 */
static int
test_crc16_t10dif_copy_base(uint8_t *buff, size_t dataSize)
{
        return test_crc16_copy_generic(buff, dataSize, crc16_t10dif_copy_base);
}

/* ========================================================================== */
/* CRC32 Functions */
/* ========================================================================== */

/**
 * Test CRC32 IEEE function (multi-binary, optimized version)
 *
 * Input format:
 * - Bytes 0-3: Initial CRC value (little-endian uint32_t)
 * - Bytes 4-n: Data to calculate CRC on
 *
 * Tests CRC32 IEEE 802.3 standard (normal/non-reflected polynomial 0x04C11DB7)
 */
static int
test_crc32_ieee(uint8_t *buff, size_t dataSize)
{
        return test_crc32_generic(buff, dataSize, crc32_ieee);
}

/**
 * Test CRC32 IEEE baseline function (portable C implementation)
 */
static int
test_crc32_ieee_base(uint8_t *buff, size_t dataSize)
{
        return test_crc32_generic(buff, dataSize, crc32_ieee_base_wrapper);
}

/**
 * Test CRC32 GZIP reflected function (multi-binary, optimized version)
 *
 * Tests CRC32 with reflected polynomial (0xEDB88320) used in GZIP/RFC1952.
 * This is the more commonly used variant of CRC32 IEEE.
 */
static int
test_crc32_gzip_refl(uint8_t *buff, size_t dataSize)
{
        return test_crc32_generic(buff, dataSize, crc32_gzip_refl);
}

/**
 * Test CRC32 GZIP reflected baseline function (portable C implementation)
 */
static int
test_crc32_gzip_refl_base(uint8_t *buff, size_t dataSize)
{
        return test_crc32_generic(buff, dataSize, crc32_gzip_refl_base_wrapper);
}

/**
 * Test CRC32 iSCSI function (multi-binary, optimized version)
 *
 * Tests CRC32 used in iSCSI protocol (Castagnoli polynomial).
 * Note: crc32_iscsi has a different parameter order (data, len, init_crc)
 * compared to other CRC functions.
 */
static int
test_crc32_iscsi(uint8_t *buff, size_t dataSize)
{
        const uint32_t init_crc = (dataSize >= 4) ? ((uint32_t *) buff)[0] : 0;
        uint8_t *data = (dataSize >= 4) ? buff + 4 : buff;
        const int len = (int) ((dataSize >= 4) ? dataSize - 4 : dataSize);

        /* Note: iSCSI CRC has different parameter order: (data, len, init_crc) */
        crc32_iscsi(data, len, init_crc);
        return 0;
}

/**
 * Test CRC32 iSCSI baseline function (portable C implementation)
 */
static int
test_crc32_iscsi_base(uint8_t *buff, size_t dataSize)
{
        const uint32_t init_crc = (dataSize >= 4) ? ((uint32_t *) buff)[0] : 0;
        uint8_t *data = (dataSize >= 4) ? buff + 4 : buff;
        const int len = (int) ((dataSize >= 4) ? dataSize - 4 : dataSize);

        crc32_iscsi_base(data, len, init_crc);
        return 0;
}

/* ========================================================================== */
/* CRC64 Functions */
/* ========================================================================== */

/**
 * Test CRC64 ECMA reflected function (multi-binary, optimized version)
 *
 * Input format:
 * - Bytes 0-7: Initial CRC value (little-endian uint64_t)
 * - Bytes 8-n: Data to calculate CRC on
 *
 * Tests CRC64 ECMA-182 standard with reflected polynomial.
 */
static int
test_crc64_ecma_refl(uint8_t *buff, size_t dataSize)
{
        return test_crc64_generic(buff, dataSize, crc64_ecma_refl);
}

/**
 * Test CRC64 ECMA normal function (multi-binary, optimized version)
 *
 * Tests CRC64 ECMA-182 standard with normal (non-reflected) polynomial.
 */
static int
test_crc64_ecma_norm(uint8_t *buff, size_t dataSize)
{
        return test_crc64_generic(buff, dataSize, crc64_ecma_norm);
}

static int
test_crc64_iso_refl(uint8_t *buff, size_t dataSize)
{
        return test_crc64_generic(buff, dataSize, crc64_iso_refl);
}

static int
test_crc64_iso_norm(uint8_t *buff, size_t dataSize)
{
        return test_crc64_generic(buff, dataSize, crc64_iso_norm);
}

static int
test_crc64_jones_refl(uint8_t *buff, size_t dataSize)
{
        return test_crc64_generic(buff, dataSize, crc64_jones_refl);
}

static int
test_crc64_jones_norm(uint8_t *buff, size_t dataSize)
{
        return test_crc64_generic(buff, dataSize, crc64_jones_norm);
}

static int
test_crc64_rocksoft_refl(uint8_t *buff, size_t dataSize)
{
        return test_crc64_generic(buff, dataSize, crc64_rocksoft_refl);
}

static int
test_crc64_rocksoft_norm(uint8_t *buff, size_t dataSize)
{
        return test_crc64_generic(buff, dataSize, crc64_rocksoft_norm);
}

/* Base versions of CRC64 */

static int
test_crc64_ecma_refl_base(uint8_t *buff, size_t dataSize)
{
        return test_crc64_generic(buff, dataSize, crc64_ecma_refl_base);
}

static int
test_crc64_ecma_norm_base(uint8_t *buff, size_t dataSize)
{
        return test_crc64_generic(buff, dataSize, crc64_ecma_norm_base);
}

static int
test_crc64_iso_refl_base(uint8_t *buff, size_t dataSize)
{
        return test_crc64_generic(buff, dataSize, crc64_iso_refl_base);
}

static int
test_crc64_iso_norm_base(uint8_t *buff, size_t dataSize)
{
        return test_crc64_generic(buff, dataSize, crc64_iso_norm_base);
}

static int
test_crc64_jones_refl_base(uint8_t *buff, size_t dataSize)
{
        return test_crc64_generic(buff, dataSize, crc64_jones_refl_base);
}

static int
test_crc64_jones_norm_base(uint8_t *buff, size_t dataSize)
{
        return test_crc64_generic(buff, dataSize, crc64_jones_norm_base);
}

static int
test_crc64_rocksoft_refl_base(uint8_t *buff, size_t dataSize)
{
        return test_crc64_generic(buff, dataSize, crc64_rocksoft_refl_base);
}

static int
test_crc64_rocksoft_norm_base(uint8_t *buff, size_t dataSize)
{
        return test_crc64_generic(buff, dataSize, crc64_rocksoft_norm_base);
}

/* ========================================================================== */
/* Main fuzzer entry point */
/* ========================================================================== */

struct {
        int (*func)(uint8_t *buff, size_t dataSize);
        const char *func_name;
} crc_test_funcs[] = {
        /* CRC16 functions */
        { test_crc16_t10dif, "test_crc16_t10dif" },
        { test_crc16_t10dif_copy, "test_crc16_t10dif_copy" },
        { test_crc16_t10dif_base, "test_crc16_t10dif_base" },
        { test_crc16_t10dif_copy_base, "test_crc16_t10dif_copy_base" },

        /* CRC32 functions */
        { test_crc32_ieee, "test_crc32_ieee" },
        { test_crc32_ieee_base, "test_crc32_ieee_base" },
        { test_crc32_gzip_refl, "test_crc32_gzip_refl" },
        { test_crc32_gzip_refl_base, "test_crc32_gzip_refl_base" },
        { test_crc32_iscsi, "test_crc32_iscsi" },
        { test_crc32_iscsi_base, "test_crc32_iscsi_base" },

        /* CRC64 functions */
        { test_crc64_ecma_refl, "test_crc64_ecma_refl" },
        { test_crc64_ecma_norm, "test_crc64_ecma_norm" },
        { test_crc64_iso_refl, "test_crc64_iso_refl" },
        { test_crc64_iso_norm, "test_crc64_iso_norm" },
        { test_crc64_jones_refl, "test_crc64_jones_refl" },
        { test_crc64_jones_norm, "test_crc64_jones_norm" },
        { test_crc64_rocksoft_refl, "test_crc64_rocksoft_refl" },
        { test_crc64_rocksoft_norm, "test_crc64_rocksoft_norm" },

        /* CRC64 base functions */
        { test_crc64_ecma_refl_base, "test_crc64_ecma_refl_base" },
        { test_crc64_ecma_norm_base, "test_crc64_ecma_norm_base" },
        { test_crc64_iso_refl_base, "test_crc64_iso_refl_base" },
        { test_crc64_iso_norm_base, "test_crc64_iso_norm_base" },
        { test_crc64_jones_refl_base, "test_crc64_jones_refl_base" },
        { test_crc64_jones_norm_base, "test_crc64_jones_norm_base" },
        { test_crc64_rocksoft_refl_base, "test_crc64_rocksoft_refl_base" },
        { test_crc64_rocksoft_norm_base, "test_crc64_rocksoft_norm_base" },
};

#define NUM_CRC_TESTS (sizeof(crc_test_funcs) / sizeof(crc_test_funcs[0]))

int
LLVMFuzzerTestOneInput(const uint8_t *data, size_t dataSize)
{
        /* Require at least 1 byte for test selection */
        if (dataSize < 1)
                return 0;

        /* Use first byte to select which CRC function to test */
        const uint8_t test_selector = data[0];
        const size_t test_index = test_selector % NUM_CRC_TESTS;

        /* Allocate a working buffer for the test */
        uint8_t *buff = malloc(dataSize);
        if (buff == NULL)
                return 0;

        /* Copy input data to working buffer */
        memcpy(buff, data, dataSize);

        /* Run the selected test */
        crc_test_funcs[test_index].func(buff, dataSize);

        /* Clean up */
        free(buff);

        return 0;
}
