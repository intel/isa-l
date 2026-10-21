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

/*
 * Level 3 extends matches past 8 bytes only if set_long_icf_fg and gen_icf_map_lh1 are
 * dispatched to versions that fit together. Copies of a random block then compress to about
 * 1.1 times the block, and without the long matches to more than 4 times.
 */

#include <stdint.h>
#include <stdio.h>
#include "igzip_lib.h"

#define BLOCK_SIZE 4096

static uint8_t in_buf[16 * BLOCK_SIZE], out_buf[32 * BLOCK_SIZE], level_buf[ISAL_DEF_LVL3_DEFAULT];

int
main(void)
{
        struct isal_zstream stream;
        uint64_t rand_state = 0x0123456789abcdefULL;
        uint32_t i;

        for (i = 0; i < sizeof(in_buf); i++) {
                rand_state = rand_state * 6364136223846793005ULL + 1442695040888963407ULL;
                in_buf[i] = i < BLOCK_SIZE ? (uint8_t) (rand_state >> 56) : in_buf[i - BLOCK_SIZE];
        }

        isal_deflate_stateless_init(&stream);
        stream.next_in = in_buf;
        stream.avail_in = sizeof(in_buf);
        stream.next_out = out_buf;
        stream.avail_out = sizeof(out_buf);
        stream.end_of_stream = 1;
        stream.level = 3;
        stream.level_buf = level_buf;
        stream.level_buf_size = sizeof(level_buf);

        if (isal_deflate_stateless(&stream) != COMP_OK || stream.total_out > 2 * BLOCK_SIZE) {
                printf("igzip_long_match_test: level 3 wrote %u bytes: Fail\n", stream.total_out);
                return -1;
        }
        printf("igzip_long_match_test: Pass\n");
        return 0;
}
