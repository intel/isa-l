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
#include <zlib.h>

#define MIN(x, y)           (((x) > (y)) ? y : x)
#define MAX_DICT_SIZE       32768
#define MAX_CHUNK_SIZE      4096
#define COMPRESS_BUF_MARGIN 128

/* ========================================================================== */
/* Test function prototypes */
/* ========================================================================== */

static void
test_deflate_init(const uint8_t *buff, size_t dataSize);
static void
test_deflate_init2(const uint8_t *buff, size_t dataSize);
static void
test_deflate_with_dict(const uint8_t *buff, size_t dataSize);
static void
test_deflate_with_header(const uint8_t *buff, size_t dataSize);
static void
test_inflate_init(const uint8_t *buff, size_t dataSize);
static void
test_inflate_init2(const uint8_t *buff, size_t dataSize);
static void
test_inflate_with_dict(const uint8_t *buff, size_t dataSize);
static void
test_inflate_reset(const uint8_t *buff, size_t dataSize);
static void
test_crc32(const uint8_t *buff, size_t dataSize);
static void
test_adler32(const uint8_t *buff, size_t dataSize);
static void
test_compress_uncompress(const uint8_t *buff, size_t dataSize);
static void
test_compress2_uncompress2(const uint8_t *buff, size_t dataSize);
static void
test_deflate_flush_modes(const uint8_t *buff, size_t dataSize);

/* ========================================================================== */
/* Test function array */
/* ========================================================================== */

typedef void (*shim_test_func_t)(const uint8_t *, size_t);

static const shim_test_func_t shim_test_funcs[] = {
        test_deflate_init,
        test_deflate_init2,
        test_deflate_with_dict,
        test_deflate_with_header,
        test_inflate_init,
        test_inflate_init2,
        test_inflate_with_dict,
        test_inflate_reset,
        test_crc32,
        test_adler32,
        test_compress_uncompress,
        test_compress2_uncompress2,
        test_deflate_flush_modes,
};

#define NUM_SHIM_TESTS (sizeof(shim_test_funcs) / sizeof(shim_test_funcs[0]))

/* ========================================================================== */
/* Helper functions */
/* ========================================================================== */

/*
 * Compress src[0..src_size) with deflateInit(level) + deflate into *dst_buf.
 * Allocates *dst_buf (caller must free). Returns the compressed size on
 * success, or 0 on failure (in which case *dst_buf is freed and set to NULL).
 */
static uLongf
deflate_to_buf(const uint8_t *src, size_t src_size, int level, uint8_t **dst_buf)
{
        z_stream strm;
        uLongf dst_size;
        int ret;

        dst_size = compressBound(src_size) + COMPRESS_BUF_MARGIN;
        *dst_buf = (uint8_t *) malloc(dst_size);
        if (!*dst_buf)
                return 0;

        ret = deflateInit(&strm, level);
        if (ret != Z_OK) {
                free(*dst_buf);
                *dst_buf = NULL;
                return 0;
        }

        strm.avail_in = src_size;
        strm.next_in = (Bytef *) src;
        strm.avail_out = dst_size;
        strm.next_out = *dst_buf;

        ret = deflate(&strm, Z_FINISH);
        dst_size = strm.total_out;
        deflateEnd(&strm);

        if (ret != Z_STREAM_END || dst_size == 0) {
                free(*dst_buf);
                *dst_buf = NULL;
                return 0;
        }
        return dst_size;
}

/*
 * Compress src[0..src_size) using deflateInit(level) + deflate + deflateSetDictionary.
 * Allocates *dst_buf (caller must free). Returns compressed size or 0.
 */
static uLongf
deflate_with_dict_to_buf(const uint8_t *src, size_t src_size, int level, const uint8_t *dict,
                         uInt dict_size, uint8_t **dst_buf)
{
        z_stream strm;
        uLongf dst_size;
        int ret;

        dst_size = compressBound(src_size) + COMPRESS_BUF_MARGIN;
        *dst_buf = (uint8_t *) malloc(dst_size);
        if (!*dst_buf)
                return 0;

        ret = deflateInit(&strm, level);
        if (ret != Z_OK)
                goto fail;

        ret = deflateSetDictionary(&strm, dict, dict_size);
        if (ret != Z_OK) {
                deflateEnd(&strm);
                goto fail;
        }

        strm.avail_in = src_size;
        strm.next_in = (Bytef *) src;
        strm.avail_out = dst_size;
        strm.next_out = *dst_buf;

        ret = deflate(&strm, Z_FINISH);
        dst_size = strm.total_out;
        deflateEnd(&strm);

        if (ret != Z_STREAM_END || dst_size == 0)
                goto fail;

        return dst_size;

fail:
        free(*dst_buf);
        *dst_buf = NULL;
        return 0;
}

/* ========================================================================== */
/* Test implementations */
/* ========================================================================== */

/* Test deflateInit */
static void
test_deflate_init(const uint8_t *buff, size_t dataSize)
{
        z_stream strm;

        const int level = buff[0];

        if (deflateInit(&strm, level) == Z_OK)
                deflateEnd(&strm);
}

/* Test deflateInit2 */
static void
test_deflate_init2(const uint8_t *buff, size_t dataSize)
{
        z_stream strm;

        if (dataSize < 4)
                return;

        const int level = buff[0];
        const int windowBits = (int) (int8_t) buff[1];
        const int memLevel = buff[2];
        const int strategy = buff[3];

        if (deflateInit2(&strm, level, Z_DEFLATED, windowBits, memLevel, strategy) == Z_OK)
                deflateEnd(&strm);
}

/* Test deflateSetDictionary with preset dictionary */
static void
test_deflate_with_dict(const uint8_t *buff, size_t dataSize)
{
        uint8_t *compressed_buf = NULL;
        uint8_t *dict_buf = NULL;

        if (dataSize < 4)
                return;

        const int level = buff[0] % (Z_BEST_COMPRESSION + 1);
        /* split_param selects the dictionary/payload boundary within data[].
         * The dictionary occupies data[0..split), the payload data[split..size). */
        const uint8_t split_param = buff[1];
        const uint8_t *data = buff + 2;
        const size_t size = dataSize - 2;
        const size_t split = 1 + (split_param % (size - 1)); /* 1..size-1 */
        const size_t dict_size = MIN(split, (size_t) MAX_DICT_SIZE);
        const uint8_t *payload = data + split;
        const size_t payload_size = size - split;

        if (dict_size == 0 || payload_size == 0)
                return;

        dict_buf = (uint8_t *) malloc(dict_size);
        if (!dict_buf)
                return;

        memcpy(dict_buf, data, dict_size);

        deflate_with_dict_to_buf(payload, payload_size, level, dict_buf, dict_size,
                                 &compressed_buf);
        free(compressed_buf);
        free(dict_buf);
}

/* Test deflateSetHeader with gzip header */
static void
test_deflate_with_header(const uint8_t *buff, size_t dataSize)
{
        z_stream strm;
        gz_header hdr;
        uint8_t *compressed_buf = NULL;
        uint8_t name[256];
        uint8_t comment[256];
        uLongf compressed_size;
        int ret;

        if (dataSize < 3)
                return;

        const int level = buff[0] % (Z_BEST_COMPRESSION + 1);
        const uint8_t hdr_flags = buff[1];
        const uint8_t *data = buff + 2;
        const size_t size = dataSize - 2;

        compressed_size = compressBound(size) + COMPRESS_BUF_MARGIN;
        compressed_buf = (uint8_t *) malloc(compressed_size);

        if (!compressed_buf)
                goto cleanup;

        /* Set up gzip header */
        memset(&hdr, 0, sizeof(hdr));
        hdr.text = (hdr_flags & 0x01) ? 1 : 0;
        hdr.time = 0x12345678;
        hdr.os = 0xFF;

        if (hdr_flags & 0x02) {
                snprintf((char *) name, sizeof(name), "test.dat");
                hdr.name = name;
        }

        if (hdr_flags & 0x04) {
                snprintf((char *) comment, sizeof(comment), "Test comment");
                hdr.comment = comment;
        }

        /* Compress with gzip header (windowBits 16+15 for gzip) */
        ret = deflateInit2(&strm, level, Z_DEFLATED, 16 + 15, 8, Z_DEFAULT_STRATEGY);
        if (ret != Z_OK)
                goto cleanup;

        ret = deflateSetHeader(&strm, &hdr);
        if (ret != Z_OK) {
                deflateEnd(&strm);
                goto cleanup;
        }

        strm.avail_in = size;
        strm.next_in = (Bytef *) data;
        strm.avail_out = compressed_size;
        strm.next_out = compressed_buf;

        ret = deflate(&strm, Z_FINISH);
        compressed_size = strm.total_out;
        deflateEnd(&strm);

        if (ret != Z_STREAM_END || compressed_size == 0)
                goto cleanup;

cleanup:
        free(compressed_buf);
}

/*
 * Test deflate with intermediate flush modes (Z_NO_FLUSH, Z_SYNC_FLUSH,
 * Z_FULL_FLUSH) between chunks, followed by Z_FINISH.
 */
static void
test_deflate_flush_modes(const uint8_t *buff, size_t dataSize)
{
        z_stream strm;
        uint8_t *compressed_buf = NULL;
        uLongf compressed_size;
        int ret;

        if (dataSize < 4)
                return;

        const int level = buff[0] % (Z_BEST_COMPRESSION + 1);
        /* Valid flush range: Z_NO_FLUSH(0)..Z_TREES(6), 7 is invalid */
        const int inter_flush = buff[1] % 8;
        /* Chunk size: 1..MAX_CHUNK_SIZE bytes */
        const size_t chunk_size = 1 + (buff[2] % MAX_CHUNK_SIZE);
        const uint8_t *data = buff + 3;
        const size_t size = dataSize - 3;

        /* compressBound covers any expansion from sync/full-flush markers */
        compressed_size = compressBound(size) + COMPRESS_BUF_MARGIN;
        compressed_buf = (uint8_t *) malloc(compressed_size);

        if (!compressed_buf)
                goto cleanup;

        ret = deflateInit(&strm, level);
        if (ret != Z_OK)
                goto cleanup;

        strm.next_out = compressed_buf;
        strm.avail_out = compressed_size;

        /* Feed data in chunks, flushing with inter_flush between them */
        size_t remaining = size;
        const uint8_t *src = data;
        while (remaining > chunk_size) {
                strm.next_in = (Bytef *) src;
                strm.avail_in = chunk_size;
                ret = deflate(&strm, inter_flush);
                if (ret != Z_OK) {
                        deflateEnd(&strm);
                        goto cleanup;
                }
                src += chunk_size;
                remaining -= chunk_size;
        }

        /* Final chunk */
        strm.next_in = (Bytef *) src;
        strm.avail_in = remaining;
        ret = deflate(&strm, Z_FINISH);
        compressed_size = strm.total_out;
        deflateEnd(&strm);

        if (ret != Z_STREAM_END || compressed_size == 0)
                goto cleanup;

cleanup:
        free(compressed_buf);
}

/* Test inflateInit */
static void
test_inflate_init(const uint8_t *buff, size_t dataSize)
{
        z_stream strm;

        if (inflateInit(&strm) == Z_OK)
                inflateEnd(&strm);
}

/* Test inflateInit2 */
static void
test_inflate_init2(const uint8_t *buff, size_t dataSize)
{
        z_stream strm;

        const int windowBits = (int) (int8_t) buff[0];

        if (inflateInit2(&strm, windowBits) == Z_OK)
                inflateEnd(&strm);
}

/* Test inflateSetDictionary */
static void
test_inflate_with_dict(const uint8_t *buff, size_t dataSize)
{
        z_stream strm;
        uint8_t *compressed_buf = NULL;
        uint8_t *decompressed_buf = NULL;
        uint8_t *dict_buf = NULL;
        uLongf compressed_size;
        int ret;

        if (dataSize < 4)
                return;

        const int level = buff[0] % (Z_BEST_COMPRESSION + 1);
        /* split_param selects the dictionary/payload boundary within data[].
         * The dictionary occupies data[0..split), the payload data[split..size). */
        const uint8_t split_param = buff[1];
        const uint8_t *data = buff + 2;
        const size_t size = dataSize - 2;
        const size_t split = 1 + (split_param % (size - 1)); /* 1..size-1 */
        const size_t dict_size = MIN(split, (size_t) MAX_DICT_SIZE);
        const uint8_t *payload = data + split;
        const size_t payload_size = size - split;

        if (dict_size == 0 || payload_size == 0)
                return;

        dict_buf = (uint8_t *) malloc(dict_size);
        if (!dict_buf)
                return;

        memcpy(dict_buf, data, dict_size);

        compressed_size = deflate_with_dict_to_buf(payload, payload_size, level, dict_buf,
                                                   dict_size, &compressed_buf);
        if (compressed_size == 0)
                goto cleanup;

        decompressed_buf = (uint8_t *) malloc(payload_size);
        if (!decompressed_buf)
                goto cleanup;

        /* Test inflateSetDictionary */
        ret = inflateInit(&strm);
        if (ret != Z_OK)
                goto cleanup;

        strm.avail_in = compressed_size;
        strm.next_in = compressed_buf;
        strm.avail_out = payload_size;
        strm.next_out = decompressed_buf;

        ret = inflate(&strm, Z_NO_FLUSH);
        if (ret == Z_NEED_DICT) {
                ret = inflateSetDictionary(&strm, dict_buf, dict_size);
                if (ret == Z_OK)
                        ret = inflate(&strm, Z_FINISH);
        }
        inflateEnd(&strm);

cleanup:
        free(compressed_buf);
        free(decompressed_buf);
        free(dict_buf);
}

/* Test inflateReset */
static void
test_inflate_reset(const uint8_t *buff, size_t dataSize)
{
        z_stream strm;
        uint8_t *compressed_buf = NULL;
        uint8_t *decompressed_buf1 = NULL;
        uint8_t *decompressed_buf2 = NULL;
        uLongf compressed_size;
        int ret;

        const int level = buff[0] % (Z_BEST_COMPRESSION + 1);
        const uint8_t *data = buff + 1;
        const size_t size = dataSize - 1;

        compressed_size = deflate_to_buf(data, size, level, &compressed_buf);
        if (compressed_size == 0)
                return;

        decompressed_buf1 = (uint8_t *) malloc(size);
        decompressed_buf2 = (uint8_t *) malloc(size);

        if (!decompressed_buf1 || !decompressed_buf2)
                goto cleanup;

        /* First decompression */
        ret = inflateInit(&strm);
        if (ret != Z_OK)
                goto cleanup;

        strm.avail_in = compressed_size;
        strm.next_in = compressed_buf;
        strm.avail_out = size;
        strm.next_out = decompressed_buf1;

        ret = inflate(&strm, Z_FINISH);

        /* Test inflateReset - reset the stream and decompress again */
        ret = inflateReset(&strm);
        if (ret != Z_OK) {
                inflateEnd(&strm);
                goto cleanup;
        }

        strm.avail_in = compressed_size;
        strm.next_in = compressed_buf;
        strm.avail_out = size;
        strm.next_out = decompressed_buf2;

        ret = inflate(&strm, Z_FINISH);
        inflateEnd(&strm);

cleanup:
        free(compressed_buf);
        free(decompressed_buf1);
        free(decompressed_buf2);
}

/* Test crc32 checksum function */
static void
test_crc32(const uint8_t *buff, size_t dataSize)
{
        uLong crc, crc1, crc2;
        size_t split_point;

        const uLong init_crc = buff[0];
        const uint8_t *data = buff + 1;
        const size_t size = dataSize - 1;

        /* Test basic CRC32 calculation with fuzzed initial value */
        crc = crc32(init_crc, data, size);

        /* Test incremental CRC32 calculation */
        split_point = size / 2;
        crc1 = crc32(init_crc, data, split_point);
        crc2 = crc32(crc1, data + split_point, size - split_point);
}

/* Test adler32 checksum function */
static void
test_adler32(const uint8_t *buff, size_t dataSize)
{
        uLong adler, adler1, adler2;
        size_t split_point;

        const uLong init_adler = buff[0];
        const uint8_t *data = buff + 1;
        const size_t size = dataSize - 1;

        /* Test basic Adler32 calculation with fuzzed initial value */
        adler = adler32(init_adler, data, size);

        /* Test incremental Adler32 calculation */
        split_point = size / 2;
        adler1 = adler32(init_adler, data, split_point);
        adler2 = adler32(adler1, data + split_point, size - split_point);
}

/* Test compress/uncompress utility functions */
static void
test_compress_uncompress(const uint8_t *buff, size_t dataSize)
{
        uint8_t *compressed_buf = NULL;
        uint8_t *decompressed_buf = NULL;
        uLongf compressed_size, decompressed_size;
        int ret;

        compressed_size = compressBound(dataSize) + COMPRESS_BUF_MARGIN;
        compressed_buf = (uint8_t *) malloc(compressed_size);
        decompressed_buf = (uint8_t *) malloc(dataSize);

        if (!compressed_buf || !decompressed_buf)
                goto cleanup;

        /* Test compress */
        ret = compress(compressed_buf, &compressed_size, buff, dataSize);
        if (ret != Z_OK || compressed_size == 0)
                goto cleanup;

        /* Test uncompress */
        decompressed_size = dataSize;
        ret = uncompress(decompressed_buf, &decompressed_size, compressed_buf, compressed_size);

        /* Too-small output buffer - exercises Z_BUF_ERROR path */
        if (dataSize > 10) {
                decompressed_size = dataSize / 2;
                ret = uncompress(decompressed_buf, &decompressed_size, compressed_buf,
                                 compressed_size);
        }

cleanup:
        free(compressed_buf);
        free(decompressed_buf);
}

/* Test compress2/uncompress2 utility functions */
static void
test_compress2_uncompress2(const uint8_t *buff, size_t dataSize)
{
        uint8_t *compressed_buf = NULL;
        uint8_t *decompressed_buf = NULL;
        uLongf compressed_size, decompressed_size;
        uLong source_len;
        int ret;

        const int level = buff[0] % (Z_BEST_COMPRESSION + 1);
        const uint8_t *data = buff + 1;
        const size_t size = dataSize - 1;

        compressed_size = compressBound(size) + COMPRESS_BUF_MARGIN;
        compressed_buf = (uint8_t *) malloc(compressed_size);
        decompressed_buf = (uint8_t *) malloc(size);

        if (!compressed_buf || !decompressed_buf)
                goto cleanup;

        /* Test compress2 with specified level */
        ret = compress2(compressed_buf, &compressed_size, data, size, level);
        if (ret != Z_OK || compressed_size == 0)
                goto cleanup;

        /* Decompress with uncompress2 to verify source_len tracking */
        decompressed_size = size;
        source_len = compressed_size;
        ret = uncompress2(decompressed_buf, &decompressed_size, compressed_buf, &source_len);

cleanup:
        free(compressed_buf);
        free(decompressed_buf);
}

/* ========================================================================== */
/* Fuzzer entry point */
/* ========================================================================== */

int
LLVMFuzzerTestOneInput(const uint8_t *data, size_t dataSize)
{
        /* Require at least 3 bytes */
        if (dataSize < 3)
                return 0;

        /* Use first byte to select which test to run */
        const uint8_t test_selector = data[0];
        const size_t test_index = test_selector % NUM_SHIM_TESTS;

        /* Skip the selector byte so each test's buff[0] is independent */
        shim_test_funcs[test_index](data + 1, dataSize - 1);

        return 0;
}