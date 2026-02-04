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

/*
 * Advanced igzip fuzz test combining multiple test modes:
 * - Stateful deflate (isal_deflate with incremental I/O and flush modes)
 * - Stateful inflate (isal_inflate with incremental I/O)
 * - Dictionary compression (isal_deflate_set_dict, isal_deflate_process_dict, etc.)
 * - Custom Huffman tables (isal_create_hufftables, isal_deflate_set_hufftables)
 * - Gzip header reading (isal_read_gzip_header with extra/name/comment fields)
 * - Zlib header reading (isal_read_zlib_header)
 * - Gzip header writing (isal_write_gzip_header with extra/name/comment fields)
 * - Zlib header writing (isal_write_zlib_header)
 */
#include <stdlib.h>
#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include <assert.h>
#include "igzip_lib.h"

#define MIN(x, y)            (((x) > (y)) ? y : x)
#define MAX_HIST_SIZE        1024
#define COMPRESS_BUF_PADDING 1024
#define MAX_CHUNK_SIZE       4096
#define MAX_DICT_SIZE        16384

/* ========================================================================== */
/* Test function prototypes */
/* ========================================================================== */

static void
test_stateful_deflate(uint8_t *buff, size_t dataSize);
static void
test_stateful_inflate(uint8_t *buff, size_t dataSize);
static void
test_dict_compression(uint8_t *buff, size_t dataSize);
static void
test_hufftables(uint8_t *buff, size_t dataSize);
static void
test_read_gzip_header(uint8_t *buff, size_t dataSize);
static void
test_read_zlib_header(uint8_t *buff, size_t dataSize);
static void
test_write_gzip_header(uint8_t *buff, size_t dataSize);
static void
test_write_zlib_header(uint8_t *buff, size_t dataSize);

/* ========================================================================== */
/* Test function array */
/* ========================================================================== */

typedef void (*igzip_test_func_t)(uint8_t *, size_t);

static const igzip_test_func_t igzip_test_funcs[] = {
        test_stateful_deflate,  test_stateful_inflate,  test_dict_compression,
        test_hufftables,        test_read_gzip_header,  test_read_zlib_header,
        test_write_gzip_header, test_write_zlib_header,
};

#define NUM_IGZIP_TESTS (sizeof(igzip_test_funcs) / sizeof(igzip_test_funcs[0]))

/* ========================================================================== */
/* Test implementations */
/* ========================================================================== */

static void
test_stateful_deflate(uint8_t *buff, size_t dataSize)
{
        struct isal_zstream cstate;
        struct inflate_state istate;
        uint8_t *compressed_buf = NULL;
        uint8_t *decompressed_buf = NULL;
        uint8_t *level_buf = NULL;
        size_t compressed_size;
        int ret = 0;

        if (dataSize < 3)
                return;

        const uint8_t params = buff[0];
        const uint16_t chunk_params = (buff[1] << 8) | buff[2];
        const uint8_t *data = buff + 3;
        const size_t size = dataSize - 3;

        const int level = params & 0x03;
        int flush_type = (params >> 2) & 0x03;
        int wrapper_type = (params >> 4) & 0x07;

        wrapper_type = wrapper_type % (IGZIP_ZLIB_NO_HDR + 1);
        if (flush_type > FULL_FLUSH)
                flush_type = NO_FLUSH;

        const size_t input_chunk_size = 1 + (chunk_params % MAX_CHUNK_SIZE);

        compressed_size = size * 2 + ISAL_DEF_MAX_HDR_SIZE + COMPRESS_BUF_PADDING;
        compressed_buf = (uint8_t *) malloc(compressed_size);
        decompressed_buf = (uint8_t *) malloc(size);

        size_t lev_buf_size = ISAL_DEF_LVL0_DEFAULT;
        if (level == 1)
                lev_buf_size = ISAL_DEF_LVL1_DEFAULT;
        else if (level == 2)
                lev_buf_size = ISAL_DEF_LVL2_DEFAULT;
        else if (level == 3)
                lev_buf_size = ISAL_DEF_LVL3_DEFAULT;

        level_buf = (uint8_t *) malloc(lev_buf_size);
        if (!level_buf)
                goto cleanup;

        if (!compressed_buf || !decompressed_buf)
                goto cleanup;

        isal_deflate_init(&cstate);
        cstate.level = level;
        cstate.level_buf = level_buf;
        cstate.level_buf_size = lev_buf_size;
        cstate.gzip_flag = wrapper_type;
        cstate.flush = flush_type;
        cstate.next_out = compressed_buf;
        cstate.avail_out = compressed_size;

        size_t input_processed = 0;
        while (input_processed < size) {
                size_t chunk_size = MIN(input_chunk_size, size - input_processed);

                cstate.next_in = (uint8_t *) data + input_processed;
                cstate.avail_in = chunk_size;
                cstate.end_of_stream = (input_processed + chunk_size >= size) ? 1 : 0;

                ret = isal_deflate(&cstate);
                if (ret != COMP_OK)
                        /* If compression failed, clean up and return */
                        goto cleanup;

                input_processed += chunk_size;

                if (cstate.avail_out == 0 && !cstate.end_of_stream)
                        /* Output buffer full, compression incomplete */
                        goto cleanup;
        }

        size_t final_compressed_size = cstate.total_out;

        isal_inflate_init(&istate);
        istate.next_in = compressed_buf;
        istate.avail_in = final_compressed_size;
        istate.next_out = decompressed_buf;
        istate.avail_out = size;
        istate.crc_flag = wrapper_type;

        ret = isal_inflate_stateless(&istate);

cleanup:
        free(compressed_buf);
        free(decompressed_buf);
        free(level_buf);
}

static void
test_stateful_inflate(uint8_t *buff, size_t dataSize)
{
        struct isal_zstream cstate;
        struct inflate_state istate;
        uint8_t *compressed_buf = NULL;
        uint8_t *decompressed_buf = NULL;
        uint8_t *level_buf = NULL;
        size_t compressed_size;
        int ret = 0;

        if (dataSize < 3)
                return;

        const uint8_t params = buff[0];
        const uint16_t chunk_params = (buff[1] << 8) | buff[2];
        const uint8_t *data = buff + 3;
        const size_t size = dataSize - 3;

        int wrapper_type = params & 0x07;
        wrapper_type = wrapper_type % (IGZIP_ZLIB_NO_HDR + 1);

        const size_t output_chunk_size = 1 + (chunk_params % MAX_CHUNK_SIZE);

        compressed_size = size * 2 + ISAL_DEF_MAX_HDR_SIZE + COMPRESS_BUF_PADDING;
        compressed_buf = (uint8_t *) malloc(compressed_size);
        decompressed_buf = (uint8_t *) malloc(size * 2);
        level_buf = (uint8_t *) malloc(ISAL_DEF_LVL1_DEFAULT);

        if (!compressed_buf || !decompressed_buf || !level_buf)
                goto cleanup;

        isal_deflate_init(&cstate);
        cstate.end_of_stream = 1;
        cstate.next_in = (uint8_t *) data;
        cstate.avail_in = size;
        cstate.next_out = compressed_buf;
        cstate.avail_out = compressed_size;
        cstate.level = 1;
        cstate.level_buf = level_buf;
        cstate.level_buf_size = ISAL_DEF_LVL1_DEFAULT;
        cstate.gzip_flag = wrapper_type;

        ret = isal_deflate_stateless(&cstate);
        if (ret != COMP_OK)
                goto cleanup;

        size_t final_compressed_size = cstate.total_out;

        isal_inflate_init(&istate);
        istate.next_in = compressed_buf;
        istate.avail_in = final_compressed_size;
        istate.crc_flag = wrapper_type;

        size_t total_decompressed = 0;

        while (istate.avail_in > 0 || istate.block_state != ISAL_BLOCK_FINISH) {
                size_t out_size = MIN(output_chunk_size, size * 2 - total_decompressed);

                if (out_size == 0)
                        goto cleanup;

                istate.next_out = decompressed_buf + total_decompressed;
                istate.avail_out = out_size;

                ret = isal_inflate(&istate);

                if (ret != ISAL_DECOMP_OK && ret != ISAL_END_INPUT)
                        goto cleanup;

                total_decompressed = istate.total_out;
        }

cleanup:
        free(compressed_buf);
        free(decompressed_buf);
        free(level_buf);
}

static void
test_dict_compression(uint8_t *buff, size_t dataSize)
{
        struct isal_zstream cstate;
        struct inflate_state istate;
        struct isal_dict dict_data;
        uint8_t *compressed_buf = NULL;
        uint8_t *decompressed_buf = NULL;
        uint8_t *level_buf = NULL;
        uint8_t *dict_buf = NULL;
        size_t compressed_size;
        int ret = 0;

        if (dataSize < 3)
                return;

        const uint8_t params = buff[0];
        const uint16_t dict_size_param = (buff[1] << 8) | buff[2];
        const uint8_t *data = buff + 3;
        const size_t size = dataSize - 3;

        int wrapper_type = params & 0x07;
        const int use_process_dict = (params >> 3) & 0x01;

        wrapper_type = wrapper_type % (IGZIP_ZLIB_NO_HDR + 1);

        const size_t dict_size = dict_size_param % MIN(size / 2 + 1, MAX_DICT_SIZE);

        if (dict_size == 0)
                return;

        if (size < dict_size)
                return;

        dict_buf = (uint8_t *) malloc(dict_size);
        if (!dict_buf)
                return;

        memcpy(dict_buf, data, dict_size);

        const uint8_t *input_data = data + dict_size;
        const size_t input_size = size - dict_size;

        compressed_size = input_size * 2 + ISAL_DEF_MAX_HDR_SIZE + COMPRESS_BUF_PADDING;
        compressed_buf = (uint8_t *) malloc(compressed_size);
        decompressed_buf = (uint8_t *) malloc(input_size);
        level_buf = (uint8_t *) malloc(ISAL_DEF_LVL1_DEFAULT);

        if (!compressed_buf || !decompressed_buf || !level_buf)
                goto cleanup;

        isal_deflate_init(&cstate);
        cstate.level = 1;
        cstate.level_buf = level_buf;
        cstate.level_buf_size = ISAL_DEF_LVL1_DEFAULT;
        cstate.gzip_flag = wrapper_type;

        if (use_process_dict) {
                ret = isal_deflate_process_dict(&cstate, &dict_data, dict_buf, dict_size);
                if (ret != COMP_OK)
                        goto cleanup;

                ret = isal_deflate_reset_dict(&cstate, &dict_data);
                if (ret != COMP_OK)
                        goto cleanup;

        } else {
                ret = isal_deflate_set_dict(&cstate, dict_buf, dict_size);
                if (ret != COMP_OK)
                        goto cleanup;
        }

        cstate.end_of_stream = 1;
        cstate.next_in = (uint8_t *) input_data;
        cstate.avail_in = input_size;
        cstate.next_out = compressed_buf;
        cstate.avail_out = compressed_size;

        ret = isal_deflate_stateless(&cstate);
        if (ret != COMP_OK)
                goto cleanup;

        size_t final_compressed_size = cstate.total_out;

        isal_inflate_init(&istate);

        ret = isal_inflate_set_dict(&istate, dict_buf, dict_size);
        if (ret != 0)
                goto cleanup;

        istate.next_in = compressed_buf;
        istate.avail_in = final_compressed_size;
        istate.next_out = decompressed_buf;
        istate.avail_out = input_size;
        istate.crc_flag = wrapper_type;

        ret = isal_inflate_stateless(&istate);

cleanup:
        free(compressed_buf);
        free(decompressed_buf);
        free(level_buf);
        free(dict_buf);
}

static void
test_hufftables(uint8_t *buff, size_t dataSize)
{
        struct isal_zstream cstate;
        struct inflate_state istate;
        struct isal_hufftables hufftables;
        struct isal_huff_histogram histogram;
        uint8_t *compressed_buf = NULL;
        uint8_t *decompressed_buf = NULL;
        uint8_t *level_buf = NULL;
        size_t compressed_size;
        int ret = 0;

        const uint8_t params = buff[0];
        const uint8_t use_subset = params & 0x01;
        const uint8_t *data = buff + 1;
        const size_t size = dataSize - 1;

        /* Use up to half the input for the histogram, but limit histogram size to 1024 bytes */
        const size_t hist_size = MIN(size / 2, MAX_HIST_SIZE);
        if (hist_size == 0)
                return;

        const uint8_t *hist_data = data;
        const uint8_t *input_data = data + hist_size;
        const size_t input_size = size - hist_size;

        if (input_size == 0)
                return;

        compressed_size = input_size * 2 + ISAL_DEF_MAX_HDR_SIZE + COMPRESS_BUF_PADDING;
        compressed_buf = (uint8_t *) malloc(compressed_size);
        decompressed_buf = (uint8_t *) malloc(input_size);
        level_buf = (uint8_t *) malloc(ISAL_DEF_LVL0_DEFAULT);

        if (!compressed_buf || !decompressed_buf || !level_buf)
                goto cleanup;

        memset(&histogram, 0, sizeof(histogram));
        isal_update_histogram((uint8_t *) hist_data, hist_size, &histogram);

        if (use_subset)
                ret = isal_create_hufftables_subset(&hufftables, &histogram);
        else
                ret = isal_create_hufftables(&hufftables, &histogram);

        if (ret != 0)
                goto cleanup;

        isal_deflate_init(&cstate);
        cstate.level = 0;
        cstate.level_buf = level_buf;
        cstate.level_buf_size = ISAL_DEF_LVL0_DEFAULT;
        cstate.gzip_flag = IGZIP_DEFLATE;

        ret = isal_deflate_set_hufftables(&cstate, &hufftables, IGZIP_HUFFTABLE_CUSTOM);
        if (ret != COMP_OK)
                goto cleanup;

        cstate.end_of_stream = 1;
        cstate.next_in = (uint8_t *) input_data;
        cstate.avail_in = input_size;
        cstate.next_out = compressed_buf;
        cstate.avail_out = compressed_size;

        ret = isal_deflate_stateless(&cstate);
        if (ret != COMP_OK)
                goto cleanup;

        size_t final_compressed_size = cstate.total_out;

        isal_inflate_init(&istate);
        istate.next_in = compressed_buf;
        istate.avail_in = final_compressed_size;
        istate.next_out = decompressed_buf;
        istate.avail_out = input_size;
        istate.crc_flag = IGZIP_DEFLATE;

        ret = isal_inflate_stateless(&istate);

cleanup:
        free(compressed_buf);
        free(decompressed_buf);
        free(level_buf);
}

static void
test_read_gzip_header(uint8_t *buff, size_t dataSize)
{
        struct inflate_state state;
        struct isal_gzip_header gz_hdr;
        uint8_t *extra_buf = NULL;
        uint8_t *name_buf = NULL;
        uint8_t *comment_buf = NULL;
        int ret = 0;

        if (dataSize < 10)
                return;

        /* Use first 3 bytes to configure buffer sizes */
        const uint16_t extra_size = ((buff[0] << 8) | buff[1]) % 1024;
        const uint16_t name_size = buff[2] % 256;
        const uint16_t comment_size = buff[3] % 256;

        const uint8_t *gzip_data = buff + 4;
        const size_t gzip_data_size = dataSize - 4;

        /* Allocate buffers for header fields */
        if (extra_size > 0) {
                extra_buf = (uint8_t *) malloc(extra_size);
                if (!extra_buf)
                        return;
        }

        if (name_size > 0) {
                name_buf = (uint8_t *) malloc(name_size);
                if (!name_buf) {
                        free(extra_buf);
                        return;
                }
        }

        if (comment_size > 0) {
                comment_buf = (uint8_t *) malloc(comment_size);
                if (!comment_buf) {
                        free(extra_buf);
                        free(name_buf);
                        return;
                }
        }

        /* Initialize gzip header structure */
        isal_gzip_header_init(&gz_hdr);
        gz_hdr.extra = extra_buf;
        gz_hdr.extra_len = extra_size;
        gz_hdr.name = (char *) name_buf;
        gz_hdr.name_buf_len = name_size;
        gz_hdr.comment = (char *) comment_buf;
        gz_hdr.comment_buf_len = comment_size;

        /* Initialize inflate state */
        isal_inflate_init(&state);
        state.next_in = (uint8_t *) gzip_data;
        state.avail_in = gzip_data_size;
        state.crc_flag = IGZIP_GZIP;

        /* Try to read the gzip header */
        ret = isal_read_gzip_header(&state, &gz_hdr);

        /* Clean up */
        free(extra_buf);
        free(name_buf);
        free(comment_buf);
}

static void
test_read_zlib_header(uint8_t *buff, size_t dataSize)
{
        struct inflate_state state;
        struct isal_zlib_header zlib_hdr;
        int ret = 0;

        /* Initialize zlib header structure */
        isal_zlib_header_init(&zlib_hdr);

        /* Initialize inflate state */
        isal_inflate_init(&state);
        state.next_in = (uint8_t *) buff;
        state.avail_in = dataSize;
        state.crc_flag = IGZIP_ZLIB;

        /* Try to read the zlib header */
        ret = isal_read_zlib_header(&state, &zlib_hdr);
}

static void
test_write_gzip_header(uint8_t *buff, size_t dataSize)
{
        struct isal_zstream stream;
        struct isal_gzip_header gz_hdr;
        uint8_t *output_buf = NULL;
        const size_t output_size = 512;
        uint32_t ret = 0;

        if (dataSize < 10)
                return;

        /* Use input data to populate header fields */
        const uint8_t flags = buff[0];
        const uint32_t mtime = ((uint32_t) buff[1] << 24) | ((uint32_t) buff[2] << 16) |
                               ((uint32_t) buff[3] << 8) | buff[4];
        const uint8_t extra_len = buff[5] % 64;
        const uint8_t name_len = buff[6] % 64;
        const uint8_t comment_len = buff[7] % 64;

        const size_t header_data_offset = 8;
        const uint8_t *header_data = buff + header_data_offset;
        const size_t remaining_data = dataSize - header_data_offset;

        /* Allocate output buffer */
        output_buf = (uint8_t *) malloc(output_size);
        if (!output_buf)
                return;

        /* Initialize gzip header structure */
        isal_gzip_header_init(&gz_hdr);
        gz_hdr.time = mtime;
        gz_hdr.xflags = (flags >> 4) & 0x0F;
        gz_hdr.os = flags & 0x0F;

        /* Set optional fields based on available data */
        size_t data_offset = 0;

        if (extra_len > 0 && data_offset + extra_len <= remaining_data) {
                gz_hdr.extra = (uint8_t *) header_data + data_offset;
                gz_hdr.extra_len = extra_len;
                data_offset += extra_len;
        }

        if (name_len > 0 && data_offset + name_len <= remaining_data) {
                gz_hdr.name = (char *) (header_data + data_offset);
                gz_hdr.name_buf_len = name_len;
                data_offset += name_len;
        }

        if (comment_len > 0 && data_offset + comment_len <= remaining_data) {
                gz_hdr.comment = (char *) (header_data + data_offset);
                gz_hdr.comment_buf_len = comment_len;
        }

        /* Initialize stream */
        isal_deflate_init(&stream);
        stream.next_out = output_buf;
        stream.avail_out = output_size;
        stream.gzip_flag = IGZIP_GZIP;

        /* Try to write the gzip header */
        ret = isal_write_gzip_header(&stream, &gz_hdr);

        /* Clean up */
        free(output_buf);
}

static void
test_write_zlib_header(uint8_t *buff, size_t dataSize)
{
        struct isal_zstream stream;
        struct isal_zlib_header zlib_hdr;
        uint8_t *output_buf = NULL;
        const size_t output_size = 16;
        uint32_t ret = 0;

        if (dataSize < 4)
                return;

        /* Use input data to populate header fields */
        const uint8_t info = buff[0] & 0x0F;
        const uint8_t level = (buff[0] >> 4) & 0x03;
        const uint8_t dict_flag = buff[1] & 0x01;
        uint32_t dict_id = ((uint32_t) buff[2] << 24) | ((uint32_t) buff[3] << 16);
        if (dataSize >= 6)
                dict_id |= ((uint32_t) buff[4] << 8) | buff[5];

        /* Allocate output buffer */
        output_buf = (uint8_t *) malloc(output_size);
        if (!output_buf)
                return;

        /* Initialize zlib header structure */
        isal_zlib_header_init(&zlib_hdr);
        zlib_hdr.info = info;
        zlib_hdr.level = level;
        zlib_hdr.dict_flag = dict_flag;
        zlib_hdr.dict_id = dict_id;

        /* Initialize stream */
        isal_deflate_init(&stream);
        stream.next_out = output_buf;
        stream.avail_out = output_size;
        stream.gzip_flag = IGZIP_ZLIB;

        /* Try to write the zlib header */
        ret = isal_write_zlib_header(&stream, &zlib_hdr);

        /* Clean up */
        free(output_buf);
}

int
LLVMFuzzerTestOneInput(const uint8_t *data, size_t dataSize)
{
        /* Require at least 2 bytes */
        if (dataSize < 2)
                return 0;

        /* Use first byte to select which test to run */
        const uint8_t test_selector = data[0];
        const size_t test_index = test_selector % NUM_IGZIP_TESTS;

        /* Run the selected test */
        igzip_test_funcs[test_index]((uint8_t *) data, dataSize);

        return 0;
}
