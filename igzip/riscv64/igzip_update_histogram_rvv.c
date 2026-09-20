/**********************************************************************
  Copyright(c) 2011-2016 Intel Corporation All rights reserved.

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
#include <stdint.h>
#include <string.h>
#include "igzip_lib.h"
#include "../huffman.h"
#include "../huff_codes.h"
#include "unaligned.h"

extern uint32_t
compare258_rvv(const uint8_t *str1, const uint8_t *str2, uint32_t max_length);

/* Replicated verbatim from huff_codes.c (static there). */
static inline uint32_t
convert_dist_to_dist_sym(uint32_t dist)
{
        assert(dist <= 32768 && dist > 0);
        if (dist <= 32768) {
                uint32_t msb = dist > 4 ? bsr(dist - 1) - 2 : 0;
                return (msb * 2) + ((dist - 1) >> msb);
        } else {
                return ~0;
        }
}

static inline uint32_t
convert_length_to_len_sym(uint32_t length)
{
        assert(length > 2 && length < 259);

        /* Based on tables on page 11 in RFC 1951 */
        if (length < 11)
                return 257 + length - 3;
        else if (length < 19)
                return 261 + (length - 3) / 2;
        else if (length < 35)
                return 265 + (length - 3) / 4;
        else if (length < 67)
                return 269 + (length - 3) / 8;
        else if (length < 131)
                return 273 + (length - 3) / 16;
        else if (length < 258)
                return 277 + (length - 3) / 32;
        else
                return 285;
}

void
isal_update_histogram_rvv(uint8_t *start_stream, int length, struct isal_huff_histogram *histogram)
{
        uint32_t literal = 0, hash;
        uint16_t seen, *last_seen = histogram->hash_table;
        uint8_t *current, *end_stream, *next_hash, *end;
        uint32_t match_length;
        uint32_t dist;
        uint64_t *lit_len_histogram = histogram->lit_len_histogram;
        uint64_t *dist_histogram = histogram->dist_histogram;

        if (length <= 0)
                return;

        end_stream = start_stream + length;
        memset(last_seen, 0, sizeof(histogram->hash_table)); /* Initialize last_seen to be 0. */
        for (current = start_stream; current < end_stream - 3; current++) {
                literal = load_le_u32(current);
                hash = compute_hash(literal) & LVL0_HASH_MASK;
                seen = last_seen[hash];
                last_seen[hash] = (current - start_stream) & 0xFFFF;
                dist = (current - start_stream - seen) & 0xFFFF;
                if (dist - 1 < D - 1) {
                        assert(start_stream <= current - dist);
                        /* RVV-accelerated longest-common-prefix match. */
                        match_length = compare258_rvv(current - dist, current,
                                                      (uint32_t) (end_stream - current));
                        if (match_length >= SHORTEST_MATCH) {
                                next_hash = current;
#ifdef ISAL_LIMIT_HASH_UPDATE
                                end = next_hash + 3;
#else
                                end = next_hash + match_length;
#endif
                                if (end > end_stream - 3)
                                        end = end_stream - 3;
                                next_hash++;
                                for (; next_hash < end; next_hash++) {
                                        literal = load_le_u32(next_hash);
                                        hash = compute_hash(literal) & LVL0_HASH_MASK;
                                        last_seen[hash] = (next_hash - start_stream) & 0xFFFF;
                                }

                                dist_histogram[convert_dist_to_dist_sym(dist)] += 1;
                                lit_len_histogram[convert_length_to_len_sym(match_length)] += 1;
                                current += match_length - 1;
                                continue;
                        }
                }
                lit_len_histogram[literal & 0xFF] += 1;
        }

        for (; current < end_stream; current++)
                lit_len_histogram[*current] += 1;

        lit_len_histogram[256] += 1;
        return;
}
