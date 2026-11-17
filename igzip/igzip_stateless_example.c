/**********************************************************************
  Copyright(c) 2026 Intel Corporation All rights reserved.

  Redistribution and use in source and binary forms, with or without
  modification, are permitted provided that the following conditions
  are met:
    * Redistributions of source code must retain the above copyright
      notice, this list of conditions and the following disclaimer.
    * Redistributions in binary form must reproduce the above copyright
      notice, this list of conditions and the following disclaimer in the
      documentation and/or other materials provided with the distribution.
    * Neither the name of Intel Corporation nor the names of its
      contributors may be used to endorse or promote products derived from
      this software without specific prior written permission.

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
#include <string.h>
#include "igzip_lib.h"

/* Demonstrate one-shot raw DEFLATE compression and decompression with buffers. */
int
main(void)
{
        static const uint8_t input[] =
                "ISA-L stateless compression example. Repeated text makes compression visible. "
                "ISA-L stateless compression example. Repeated text makes compression visible.";
        uint8_t compressed[sizeof(input) + 64];
        uint8_t output[sizeof(input)];
        uint8_t level_buf[ISAL_DEF_LVL1_DEFAULT];
        struct isal_zstream comp_stream;
        struct inflate_state decomp_stream;
        uint32_t input_len = sizeof(input) - 1;
        uint32_t compressed_len;
        uint32_t output_len;
        int ret;

        /* Set up one-shot raw DEFLATE compression and its level-1 workspace. */
        isal_deflate_stateless_init(&comp_stream);
        comp_stream.level = 1;
        comp_stream.level_buf = level_buf;
        comp_stream.level_buf_size = sizeof(level_buf);
        comp_stream.gzip_flag = IGZIP_DEFLATE;
        comp_stream.end_of_stream = 1;
        comp_stream.next_in = (uint8_t *) input;
        comp_stream.avail_in = input_len;
        comp_stream.next_out = compressed;
        comp_stream.avail_out = sizeof(compressed);

        /* Mark this as the final input so this call emits a complete stream. */
        ret = isal_deflate_stateless(&comp_stream);
        if (ret != COMP_OK) {
                fprintf(stderr, "Compression failed: %d\n", ret);
                return 1;
        }
        compressed_len = sizeof(compressed) - comp_stream.avail_out;

        /* Decompress the resulting raw DEFLATE stream, then verify the round trip. */
        isal_inflate_init(&decomp_stream);
        decomp_stream.crc_flag = ISAL_DEFLATE;
        decomp_stream.next_in = compressed;
        decomp_stream.avail_in = compressed_len;
        decomp_stream.next_out = output;
        decomp_stream.avail_out = sizeof(output);

        ret = isal_inflate_stateless(&decomp_stream);
        if (ret != ISAL_DECOMP_OK) {
                fprintf(stderr, "Decompression failed: %d\n", ret);
                return 1;
        }
        output_len = sizeof(output) - decomp_stream.avail_out;

        if (output_len != input_len || memcmp(input, output, input_len) != 0) {
                fprintf(stderr, "Round-trip verification failed\n");
                return 1;
        }

        printf("Compressed %u bytes to %u bytes; round-trip verified.\n", input_len,
               compressed_len);
        return 0;
}