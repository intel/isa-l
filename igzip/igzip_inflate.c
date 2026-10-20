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
#include "igzip_lib.h"
#include "crc.h"
#include "huff_codes.h"
#include "igzip_checksums.h"
#include "igzip_wrapper.h"
#include "unaligned.h"

#ifndef NO_STATIC_INFLATE_H
#include "static_inflate.h"
#endif

extern int
decode_huffman_code_block_stateless(struct inflate_state *, uint8_t *start_out);
extern struct isal_hufftables hufftables_default; /* For known header detection */

#define LARGE_SHORT_SYM_LEN         25
#define LARGE_SHORT_SYM_MASK        ((1 << LARGE_SHORT_SYM_LEN) - 1)
#define LARGE_LONG_SYM_LEN          10
#define LARGE_LONG_SYM_MASK         ((1 << LARGE_LONG_SYM_LEN) - 1)
#define LARGE_SHORT_CODE_LEN_OFFSET 28
#define LARGE_LONG_CODE_LEN_OFFSET  10
#define LARGE_FLAG_BIT_OFFSET       25
#define LARGE_FLAG_BIT              (1 << LARGE_FLAG_BIT_OFFSET)
#define LARGE_SYM_COUNT_OFFSET      26
#define LARGE_SYM_COUNT_LEN         2
#define LARGE_SYM_COUNT_MASK        ((1 << LARGE_SYM_COUNT_LEN) - 1)
#define LARGE_SHORT_MAX_LEN_OFFSET  26

#define SMALL_SHORT_SYM_LEN         9
#define SMALL_SHORT_SYM_MASK        ((1 << SMALL_SHORT_SYM_LEN) - 1)
#define SMALL_SHORT_CODE_LEN_OFFSET 11
#define SMALL_LONG_CODE_LEN_OFFSET  10
#define SMALL_FLAG_BIT_OFFSET       10
#define SMALL_FLAG_BIT              (1 << SMALL_FLAG_BIT_OFFSET)

#define DIST_SYM_OFFSET       0
#define DIST_SYM_LEN          5
#define DIST_SYM_MASK         ((1 << DIST_SYM_LEN) - 1)
#define DIST_SYM_EXTRA_OFFSET 5
#define DIST_SYM_EXTRA_LEN    4
#define DIST_SYM_EXTRA_MASK   ((1 << DIST_SYM_EXTRA_LEN) - 1)

#define MAX_LIT_LEN_CODE_LEN 21
#define MAX_LIT_LEN_SYM      512
#define LIT_LEN_ELEMS        514

#define INVALID_SYMBOL 0x1FFF

#define MIN_DEF_MATCH 3

#define TRIPLE_SYM_FLAG  0
#define DOUBLE_SYM_FLAG  TRIPLE_SYM_FLAG + 1
#define SINGLE_SYM_FLAG  DOUBLE_SYM_FLAG + 1
#define DEFAULT_SYM_FLAG TRIPLE_SYM_FLAG

#define SINGLE_SYM_THRESH (1 * 1024)
#define DOUBLE_SYM_THRESH (8 * 1024)

/* structure contain lookup data based on RFC 1951 */
struct rfc1951_tables {
        uint8_t dist_extra_bit_count[32];
        uint32_t dist_start[32];
        uint8_t len_extra_bit_count[32];
        uint16_t len_start[32];
};

/* The following tables are based on the tables in the deflate standard,
 * RFC 1951 page 11. */
static struct rfc1951_tables rfc_lookup_table = {
        .dist_extra_bit_count = { 0x00, 0x00, 0x00, 0x00, 0x01, 0x01, 0x02, 0x02, 0x03, 0x03, 0x04,
                                  0x04, 0x05, 0x05, 0x06, 0x06, 0x07, 0x07, 0x08, 0x08, 0x09, 0x09,
                                  0x0a, 0x0a, 0x0b, 0x0b, 0x0c, 0x0c, 0x0d, 0x0d, 0x00, 0x00 },

        .dist_start = { 0x0001, 0x0002, 0x0003, 0x0004, 0x0005, 0x0007, 0x0009, 0x000d,
                        0x0011, 0x0019, 0x0021, 0x0031, 0x0041, 0x0061, 0x0081, 0x00c1,
                        0x0101, 0x0181, 0x0201, 0x0301, 0x0401, 0x0601, 0x0801, 0x0c01,
                        0x1001, 0x1801, 0x2001, 0x3001, 0x4001, 0x6001, 0x0000, 0x0000 },

        .len_extra_bit_count = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x01, 0x01,
                                 0x01, 0x02, 0x02, 0x02, 0x02, 0x03, 0x03, 0x03, 0x03, 0x04, 0x04,
                                 0x04, 0x04, 0x05, 0x05, 0x05, 0x05, 0x00, 0x00, 0x00, 0x00 },

        .len_start = { 0x0003, 0x0004, 0x0005, 0x0006, 0x0007, 0x0008, 0x0009, 0x000a,
                       0x000b, 0x000d, 0x000f, 0x0011, 0x0013, 0x0017, 0x001b, 0x001f,
                       0x0023, 0x002b, 0x0033, 0x003b, 0x0043, 0x0053, 0x0063, 0x0073,
                       0x0083, 0x00a3, 0x00c3, 0x00e3, 0x0102, 0x0103, 0x0000, 0x0000 }
};

/*Performs a copy of length repeat_length data starting at dest -
 * lookback_distance into dest. This copy copies data previously copied when the
 * src buffer and the dest buffer overlap. */
static void inline byte_copy(uint8_t *dest, uint64_t lookback_distance, int repeat_length)
{
        uint8_t *src = dest - lookback_distance;

        for (; repeat_length > 0; repeat_length--)
                *dest++ = *src++;
}

static void
update_checksum(struct inflate_state *state, uint8_t *start_in, uint64_t length)
{
        switch (state->crc_flag) {
        case ISAL_GZIP:
        case ISAL_GZIP_NO_HDR:
        case ISAL_GZIP_NO_HDR_VER:
                state->crc = crc32_gzip_refl(state->crc, start_in, length);
                break;
        case ISAL_ZLIB:
        case ISAL_ZLIB_NO_HDR:
        case ISAL_ZLIB_NO_HDR_VER:
                state->crc = isal_adler32_bam1(state->crc, start_in, length);
                break;
        }
}

static void
finalize_adler32(struct inflate_state *state)
{

        state->crc = (state->crc & 0xffff0000) | (((state->crc & 0xffff) + 1) % ADLER_MOD);
}

static const uint8_t bitrev_table[] = {
        0x00, 0x80, 0x40, 0xc0, 0x20, 0xa0, 0x60, 0xe0, 0x10, 0x90, 0x50, 0xd0, 0x30, 0xb0, 0x70,
        0xf0, 0x08, 0x88, 0x48, 0xc8, 0x28, 0xa8, 0x68, 0xe8, 0x18, 0x98, 0x58, 0xd8, 0x38, 0xb8,
        0x78, 0xf8, 0x04, 0x84, 0x44, 0xc4, 0x24, 0xa4, 0x64, 0xe4, 0x14, 0x94, 0x54, 0xd4, 0x34,
        0xb4, 0x74, 0xf4, 0x0c, 0x8c, 0x4c, 0xcc, 0x2c, 0xac, 0x6c, 0xec, 0x1c, 0x9c, 0x5c, 0xdc,
        0x3c, 0xbc, 0x7c, 0xfc, 0x02, 0x82, 0x42, 0xc2, 0x22, 0xa2, 0x62, 0xe2, 0x12, 0x92, 0x52,
        0xd2, 0x32, 0xb2, 0x72, 0xf2, 0x0a, 0x8a, 0x4a, 0xca, 0x2a, 0xaa, 0x6a, 0xea, 0x1a, 0x9a,
        0x5a, 0xda, 0x3a, 0xba, 0x7a, 0xfa, 0x06, 0x86, 0x46, 0xc6, 0x26, 0xa6, 0x66, 0xe6, 0x16,
        0x96, 0x56, 0xd6, 0x36, 0xb6, 0x76, 0xf6, 0x0e, 0x8e, 0x4e, 0xce, 0x2e, 0xae, 0x6e, 0xee,
        0x1e, 0x9e, 0x5e, 0xde, 0x3e, 0xbe, 0x7e, 0xfe, 0x01, 0x81, 0x41, 0xc1, 0x21, 0xa1, 0x61,
        0xe1, 0x11, 0x91, 0x51, 0xd1, 0x31, 0xb1, 0x71, 0xf1, 0x09, 0x89, 0x49, 0xc9, 0x29, 0xa9,
        0x69, 0xe9, 0x19, 0x99, 0x59, 0xd9, 0x39, 0xb9, 0x79, 0xf9, 0x05, 0x85, 0x45, 0xc5, 0x25,
        0xa5, 0x65, 0xe5, 0x15, 0x95, 0x55, 0xd5, 0x35, 0xb5, 0x75, 0xf5, 0x0d, 0x8d, 0x4d, 0xcd,
        0x2d, 0xad, 0x6d, 0xed, 0x1d, 0x9d, 0x5d, 0xdd, 0x3d, 0xbd, 0x7d, 0xfd, 0x03, 0x83, 0x43,
        0xc3, 0x23, 0xa3, 0x63, 0xe3, 0x13, 0x93, 0x53, 0xd3, 0x33, 0xb3, 0x73, 0xf3, 0x0b, 0x8b,
        0x4b, 0xcb, 0x2b, 0xab, 0x6b, 0xeb, 0x1b, 0x9b, 0x5b, 0xdb, 0x3b, 0xbb, 0x7b, 0xfb, 0x07,
        0x87, 0x47, 0xc7, 0x27, 0xa7, 0x67, 0xe7, 0x17, 0x97, 0x57, 0xd7, 0x37, 0xb7, 0x77, 0xf7,
        0x0f, 0x8f, 0x4f, 0xcf, 0x2f, 0xaf, 0x6f, 0xef, 0x1f, 0x9f, 0x5f, 0xdf, 0x3f, 0xbf, 0x7f,
        0xff,

};

/*
 * Returns integer with first length bits reversed and all higher bits zeroed
 */
static uint32_t inline bit_reverse2(uint16_t bits, uint8_t length)
{
        uint32_t bitrev;
        bitrev = bitrev_table[bits >> 8];
        bitrev |= bitrev_table[bits & 0xFF] << 8;

        return bitrev >> (16 - length);
}

/* Load data from the in_stream into a buffer to allow for handling unaligned data*/
static void inline inflate_in_load(struct inflate_state *state, int min_required)
{
        uint64_t temp = 0;
        uint8_t new_bytes;

        if (state->read_in_length >= 64)
                return;

        if (state->avail_in >= 8) {
                /* If there is enough space to load a 64 bits, load the data and use
                 * that to fill read_in */
                new_bytes = 8 - (state->read_in_length + 7) / 8;
                temp = load_le_u64(state->next_in);

                state->read_in |= temp << state->read_in_length;
                state->next_in += new_bytes;
                state->avail_in -= new_bytes;
                state->read_in_length += new_bytes * 8;

        } else {
                /* Else fill the read_in buffer 1 byte at a time */
                while (state->read_in_length < 57 && state->avail_in > 0) {
                        temp = *state->next_in;
                        state->read_in |= temp << state->read_in_length;
                        state->next_in++;
                        state->avail_in--;
                        state->read_in_length += 8;
                }
        }
}

static uint64_t inline inflate_in_read_bits_unsafe(struct inflate_state *state, uint8_t bit_count)
{
        uint64_t ret;

        ret = (state->read_in) & ((1 << bit_count) - 1);
        state->read_in >>= bit_count;
        state->read_in_length -= bit_count;

        return ret;
}

/* Returns the next bit_count bits from the in stream and shifts the stream over
 * by bit-count bits */
static uint64_t inline inflate_in_read_bits(struct inflate_state *state, uint8_t bit_count)
{
        /* Load inflate_in if not enough data is in the read_in buffer */
        inflate_in_load(state, bit_count);
        return inflate_in_read_bits_unsafe(state, bit_count);
}

/* Bits of the index of the code length code decode table, the longest code length code */
#define HDR_TABLE_BITS 7

/* An entry of the expanded lit/len code list holds the code and the symbol at this shift */
#define ELIST_SYM_SHIFT 21
#define ELIST_CODE_MASK ((1 << ELIST_SYM_SHIFT) - 1)

/* Counts the code lengths lens[0..n) into count[0..15] */
static void inline count_lengths(const uint8_t *lens, uint32_t n, uint16_t *count)
{
        uint32_t i;

        memset(count, 0, sizeof(*count) * (MAX_HUFF_TREE_DEPTH + 1));
        for (i = 0; i < n; i++)
                count[lens[i]]++;
}

/*
 * Sets next_code[1..15] to the first canonical code of each length, given the number of codes
 * of each length in count[1..15]. Returns ISAL_INVALID_BLOCK if the lengths oversubscribe the
 * code space.
 */
static int inline set_next_codes(const uint16_t *count, uint32_t *next_code)
{
        uint32_t i;

        next_code[0] = 0;
        next_code[1] = 0;
        for (i = 2; i <= MAX_HUFF_TREE_DEPTH; i++)
                next_code[i] = (next_code[i - 1] + count[i - 1]) << 1;

        if (next_code[MAX_HUFF_TREE_DEPTH] + count[MAX_HUFF_TREE_DEPTH] >
            (1 << MAX_HUFF_TREE_DEPTH))
                return ISAL_INVALID_BLOCK;

        return 0;
}

/*
 * Builds the decode table of the code length code. An entry holds the code length in bits 7:0
 * and the symbol in bits 15:8, or is 0 for bits that start no code.
 */
static int inline make_header_table(uint16_t *table, const uint8_t *lens)
{
        uint8_t count[HDR_TABLE_BITS + 1], pos[HDR_TABLE_BITS + 1];
        uint8_t sorted[CODE_LEN_CODES];
        uint32_t i, len, code, kraft = 0;

        memset(count, 0, sizeof(count));
        for (i = 0; i < CODE_LEN_CODES; i++)
                count[lens[i]]++;

        /* The lengths must not oversubscribe the code space */
        for (len = 1; len <= HDR_TABLE_BITS; len++)
                kraft += count[len] << (HDR_TABLE_BITS - len);
        if (kraft > (1 << HDR_TABLE_BITS))
                return ISAL_INVALID_BLOCK;

        pos[1] = 0;
        for (len = 1; len < HDR_TABLE_BITS; len++)
                pos[len + 1] = pos[len] + count[len];
        for (i = 0; i < CODE_LEN_CODES; i++)
                if (lens[i] != 0)
                        sorted[pos[lens[i]]++] = i;

        /* Double the table for each length, then add the codes of that length */
        table[0] = 0;
        code = 0;
        i = 0;
#if defined(__GNUC__) && __GNUC__ >= 8 && !defined(__clang__) && !defined(__INTEL_COMPILER)
#pragma GCC unroll 7
#endif
        for (len = 1; len <= HDR_TABLE_BITS; len++) {
                /* With the loop unrolled, the copies have constant sizes and need no calls */
                memcpy(&table[1 << (len - 1)], table, sizeof(*table) << (len - 1));
                for (; i < pos[len]; i++)
                        table[bit_reverse2(code++, len)] = len | sorted[i] << 8;
                code <<= 1;
        }

        return 0;
}

/* The number of literal symbols. End of block is the symbol after them. */
#define NUM_LITERALS (ISAL_DEF_LIT_SYMBOLS - 1)

/*
 * The literals of each code length, in order: sym[len][0..num[len]). End of block is not in the
 * lists. Its code follows the codes of the literals of its length.
 */
struct lit_order {
        uint16_t num[MAX_HUFF_TREE_DEPTH + 1];
        uint8_t sym[MAX_HUFF_TREE_DEPTH + 1][NUM_LITERALS];
};

/*
 * Sets result to the decode tables of the lit/len code with the code lengths lens[0..nlit),
 * where first_code[] is from set_next_codes(), and lits holds the literals by length. Symbols
 * from 286 on, which only the static code has, take part in assigning codes but are not decoded.
 *
 * Each length symbol is expanded to one code per value of its extra bits, with the extra bits
 * appended to the code, and the decoded symbol is the match length + 254. The table is built
 * from the expanded codes sorted by length and then by symbol, so the literals of each length
 * come first.
 *
 * Returns ISAL_INVALID_BLOCK if the long codes do not fit in long_code_lookup, and 0 otherwise.
 */
static int
make_inflate_huff_code_lit_len(struct inflate_huff_code_large *result, const uint8_t *lens,
                               uint32_t nlit, const uint32_t *first_code,
                               const struct lit_order *lits, uint32_t multisym)
{
        uint32_t *short_code_lookup = result->short_code_lookup;
        uint16_t *long_code_lookup = result->long_code_lookup;
        uint32_t next_code[MAX_HUFF_TREE_DEPTH + 1];
        uint16_t lit_first[MAX_LIT_LEN_CODE_LEN + 1];
        uint16_t start[MAX_LIT_LEN_CODE_LEN + 2], pos[MAX_LIT_LEN_CODE_LEN + 1];
        uint32_t elist[LIT_LEN_ELEMS];
        uint16_t group_offset[LIT_LEN_ELEMS + 1];
        uint8_t group_len[LIT_LEN_ELEMS + 1];
        uint32_t i, j, k, k1, k2, k3, len, len1, len2, len3, code, code1, code12, sym, sym1;
        uint32_t extra, num_lens, min_len, last_len, size, entry, end2, end3, num_groups, offset;

        memcpy(next_code, first_code, sizeof(next_code));
        num_lens = nlit < LIT_LEN ? nlit : LIT_LEN;

        /* Count the expanded codes of each length */
        memset(pos, 0, sizeof(pos));
        for (len = 1; len <= MAX_HUFF_TREE_DEPTH; len++)
                pos[len] = lits->num[len];
        pos[lens[NUM_LITERALS]]++;
        for (i = LIT_TABLE_SIZE; i < num_lens; i++) {
                len = lens[i];
                extra = rfc_lookup_table.len_extra_bit_count[i - LIT_TABLE_SIZE];
                if (len != 0)
                        pos[len + extra] += 1 << extra;
        }

        start[0] = 0;
        start[1] = 0;
        for (len = 1; len <= MAX_LIT_LEN_CODE_LEN; len++)
                start[len + 1] = start[len] + pos[len];

        /* Give the literals of each length consecutive codes, followed by end of block */
        for (len = 1; len <= MAX_HUFF_TREE_DEPTH; len++) {
                code = next_code[len];
                k = start[len];
                for (j = 0; j < lits->num[len]; j++)
                        elist[k + j] = bit_reverse2(code + j, len) | (uint32_t) lits->sym[len][j]
                                                                             << ELIST_SYM_SHIFT;
                lit_first[len] = j;
                if (lens[NUM_LITERALS] == len) {
                        sym = NUM_LITERALS;
                        elist[k + j] = bit_reverse2(code + j, len) | sym << ELIST_SYM_SHIFT;
                        j++;
                }
                next_code[len] = code + j;
                pos[len] = k + j;
        }
        for (; len <= MAX_LIT_LEN_CODE_LEN; len++) {
                pos[len] = start[len];
                lit_first[len] = 0;
        }

        /* Expand the length symbols after the literals */
        for (i = LIT_TABLE_SIZE; i < num_lens; i++) {
                len = lens[i];
                if (len == 0)
                        continue;
                extra = rfc_lookup_table.len_extra_bit_count[i - LIT_TABLE_SIZE];
                sym = rfc_lookup_table.len_start[i - LIT_TABLE_SIZE] + 254;
                code = bit_reverse2(next_code[len]++, len);
                k = pos[len + extra];
                for (j = 0; j < (1u << extra); j++)
                        elist[k + j] = (code | j << len) | (sym + j) << ELIST_SYM_SHIFT;
                pos[len + extra] = k + j;
        }

        if (start[MAX_LIT_LEN_CODE_LEN + 1] == 0) {
                memset(short_code_lookup, 0, sizeof(result->short_code_lookup));
                return 0;
        }

        for (min_len = 1; start[min_len + 1] == 0; min_len++)
                ;

        /*
         * Fill the table one code length at a time. Each length doubles the table filled so far,
         * which repeats the shorter codes, and then adds the codes of that length, followed by
         * the pairs and triples of codes that make up that length.
         */
        last_len = min_len;
        if (last_len > ISAL_DECODE_LONG_BITS)
                last_len = ISAL_DECODE_LONG_BITS + 1;
        size = 1 << (last_len - 1);
        memset(short_code_lookup, 0, size * sizeof(*short_code_lookup));

        for (; last_len <= ISAL_DECODE_LONG_BITS; last_len++) {
                memcpy(short_code_lookup + size, short_code_lookup,
                       size * sizeof(*short_code_lookup));
                size *= 2;

                entry = last_len << LARGE_SHORT_CODE_LEN_OFFSET | 1 << LARGE_SYM_COUNT_OFFSET;
                for (k = start[last_len]; k < start[last_len + 1]; k++)
                        short_code_lookup[elist[k] & ELIST_CODE_MASK] =
                                entry | elist[k] >> ELIST_SYM_SHIFT;

                if (multisym >= SINGLE_SYM_FLAG || last_len < 2 * min_len)
                        continue;

                /* A literal followed by any symbol */
                entry = last_len << LARGE_SHORT_CODE_LEN_OFFSET | 2 << LARGE_SYM_COUNT_OFFSET;
                for (len1 = min_len; len1 <= last_len - min_len; len1++) {
                        len2 = last_len - len1;
                        end2 = start[len2 + 1];
                        if (start[len2] == end2)
                                continue;
                        for (k1 = start[len1]; k1 < start[len1] + lit_first[len1]; k1++) {
                                code1 = elist[k1] & ELIST_CODE_MASK;
                                sym1 = entry | elist[k1] >> ELIST_SYM_SHIFT;
                                for (k2 = start[len2]; k2 < end2; k2++)
                                        short_code_lookup[code1 | (elist[k2] & ELIST_CODE_MASK)
                                                                          << len1] =
                                                sym1 | (elist[k2] >> ELIST_SYM_SHIFT) << 8;
                        }
                }

                if (multisym >= DOUBLE_SYM_FLAG || last_len < 3 * min_len)
                        continue;

                /* Two literals followed by any symbol but a length of 258 */
                entry = last_len << LARGE_SHORT_CODE_LEN_OFFSET | 3 << LARGE_SYM_COUNT_OFFSET;
                for (len1 = min_len; len1 <= last_len - 2 * min_len; len1++) {
                        for (k1 = start[len1]; k1 < start[len1] + lit_first[len1]; k1++) {
                                code1 = elist[k1] & ELIST_CODE_MASK;
                                sym1 = entry | elist[k1] >> ELIST_SYM_SHIFT;
                                for (len2 = min_len; len2 <= last_len - len1 - min_len; len2++) {
                                        len3 = last_len - len1 - len2;
                                        end3 = start[len3 + 1];
                                        for (k2 = start[len2]; k2 < start[len2] + lit_first[len2];
                                             k2++) {
                                                code12 = code1 | (elist[k2] & ELIST_CODE_MASK)
                                                                         << len1;
                                                sym = sym1 | (elist[k2] >> ELIST_SYM_SHIFT) << 8;
                                                for (k3 = start[len3]; k3 < end3; k3++) {
                                                        if ((elist[k3] >> ELIST_SYM_SHIFT) >=
                                                            MAX_LIT_LEN_SYM)
                                                                break;
                                                        short_code_lookup[code12 |
                                                                          (elist[k3] &
                                                                           ELIST_CODE_MASK)
                                                                                  << (len1 +
                                                                                      len2)] =
                                                                sym | (elist[k3] >> ELIST_SYM_SHIFT)
                                                                              << 16;
                                                }
                                        }
                                }
                        }
                }
        }

        /*
         * Codes longer than ISAL_DECODE_LONG_BITS share a long_code_lookup subtable with the
         * codes that start with the same ISAL_DECODE_LONG_BITS bits, in the order the groups
         * first appear. No shorter code starts with these bits, so their short_code_lookup
         * entries are 0 until they point to the subtables, and number the groups meanwhile.
         */
        num_groups = 0;
        for (len = ISAL_DECODE_LONG_BITS + 1; len <= MAX_LIT_LEN_CODE_LEN; len++) {
                for (k = start[len]; k < start[len + 1]; k++) {
                        code = elist[k] & ((1 << ISAL_DECODE_LONG_BITS) - 1);
                        i = short_code_lookup[code];
                        if (i == 0) {
                                i = ++num_groups;
                                short_code_lookup[code] = i;
                        }
                        /* The longest code of each group comes last */
                        group_len[i] = len;
                }
        }

        offset = 0;
        for (i = 1; i <= num_groups; i++) {
                group_offset[i] = offset;
                offset += 1 << (group_len[i] - ISAL_DECODE_LONG_BITS);
        }
        if (offset > ISAL_HUFF_CODE_LARGE_LONG_ALIGNED)
                return ISAL_INVALID_BLOCK;
        memset(long_code_lookup, 0, offset * sizeof(*long_code_lookup));

        for (len = ISAL_DECODE_LONG_BITS + 1; len <= MAX_LIT_LEN_CODE_LEN; len++) {
                for (k = start[len]; k < start[len + 1]; k++) {
                        i = short_code_lookup[elist[k] & ((1 << ISAL_DECODE_LONG_BITS) - 1)];
                        entry = elist[k] >> ELIST_SYM_SHIFT | len << LARGE_LONG_CODE_LEN_OFFSET;
                        size = 1 << (group_len[i] - ISAL_DECODE_LONG_BITS);
                        for (code = (elist[k] & ELIST_CODE_MASK) >> ISAL_DECODE_LONG_BITS;
                             code < size; code += 1 << (len - ISAL_DECODE_LONG_BITS))
                                long_code_lookup[group_offset[i] + code] = entry;
                }
        }

        /* Point the entry of each group at its subtable, the first time the group comes up */
        for (k = start[ISAL_DECODE_LONG_BITS + 1]; k < start[MAX_LIT_LEN_CODE_LEN + 1]; k++) {
                code = elist[k] & ((1 << ISAL_DECODE_LONG_BITS) - 1);
                i = short_code_lookup[code];
                if (i <= num_groups)
                        short_code_lookup[code] = group_offset[i] |
                                                  group_len[i] << LARGE_SHORT_MAX_LEN_OFFSET |
                                                  LARGE_FLAG_BIT;
        }

        return 0;
}

/*
 * Sets result to the decode tables of the distance code with the code lengths lens[0..n),
 * where count[] counts the lengths and first_code[] is from set_next_codes(). Symbols from
 * max_symbol on decode to their code length with a length of 0, which marks them invalid.
 * Returns ISAL_INVALID_BLOCK if the long codes do not fit in long_code_lookup, which can happen
 * when the code is incomplete, and 0 otherwise.
 */
static int
make_inflate_huff_code_dist(struct inflate_huff_code_small *result, const uint8_t *lens, uint32_t n,
                            const uint16_t *count, const uint32_t *first_code, uint32_t max_symbol)
{
        uint16_t *short_code_lookup = result->short_code_lookup;
        uint16_t *long_code_lookup = result->long_code_lookup;
        uint32_t next_code[MAX_HUFF_TREE_DEPTH + 1];
        uint16_t start[MAX_HUFF_TREE_DEPTH + 2], pos[MAX_HUFF_TREE_DEPTH + 1];
        uint32_t list[DIST_LEN + 2];
        uint16_t group_offset[DIST_LEN + 3];
        uint8_t group_len[DIST_LEN + 3];
        uint32_t i, k, len, code, sym, min_len, last_len, size, num_groups, offset;
        uint16_t entry;

        memcpy(next_code, first_code, sizeof(next_code));

        start[1] = 0;
        for (len = 1; len <= MAX_HUFF_TREE_DEPTH; len++) {
                pos[len] = start[len];
                start[len + 1] = start[len] + count[len];
        }

        if (start[MAX_HUFF_TREE_DEPTH + 1] == 0) {
                memset(short_code_lookup, 0, sizeof(result->short_code_lookup));
                return 0;
        }

        /* Sort the codes by length and then by symbol */
        for (i = 0; i < n; i++) {
                len = lens[i];
                if (len != 0)
                        list[pos[len]++] = bit_reverse2(next_code[len]++, len) | i << 16;
        }

        for (min_len = 1; start[min_len + 1] == 0; min_len++)
                ;

        last_len = min_len;
        if (last_len > ISAL_DECODE_SHORT_BITS)
                last_len = ISAL_DECODE_SHORT_BITS + 1;
        size = 1 << (last_len - 1);
        memset(short_code_lookup, 0, size * sizeof(*short_code_lookup));

        for (; last_len <= ISAL_DECODE_SHORT_BITS; last_len++) {
                memcpy(short_code_lookup + size, short_code_lookup,
                       size * sizeof(*short_code_lookup));
                size *= 2;

                for (k = start[last_len]; k < start[last_len + 1]; k++) {
                        sym = list[k] >> 16;
                        if (sym >= max_symbol)
                                entry = last_len;
                        else
                                entry = sym |
                                        rfc_lookup_table.dist_extra_bit_count[sym]
                                                << DIST_SYM_EXTRA_OFFSET |
                                        last_len << SMALL_SHORT_CODE_LEN_OFFSET;
                        short_code_lookup[list[k] & 0xFFFF] = entry;
                }
        }

        /* Longer codes go to long_code_lookup as in make_inflate_huff_code_lit_len() */
        num_groups = 0;
        for (len = ISAL_DECODE_SHORT_BITS + 1; len <= MAX_HUFF_TREE_DEPTH; len++) {
                for (k = start[len]; k < start[len + 1]; k++) {
                        code = list[k] & ((1 << ISAL_DECODE_SHORT_BITS) - 1);
                        i = short_code_lookup[code];
                        if (i == 0) {
                                i = ++num_groups;
                                short_code_lookup[code] = i;
                        }
                        group_len[i] = len;
                }
        }

        offset = 0;
        for (i = 1; i <= num_groups; i++) {
                group_offset[i] = offset;
                offset += 1 << (group_len[i] - ISAL_DECODE_SHORT_BITS);
        }
        if (offset > ISAL_HUFF_CODE_SMALL_LONG_ALIGNED)
                return ISAL_INVALID_BLOCK;
        memset(long_code_lookup, 0, offset * sizeof(*long_code_lookup));

        for (len = ISAL_DECODE_SHORT_BITS + 1; len <= MAX_HUFF_TREE_DEPTH; len++) {
                for (k = start[len]; k < start[len + 1]; k++) {
                        i = short_code_lookup[list[k] & ((1 << ISAL_DECODE_SHORT_BITS) - 1)];
                        sym = list[k] >> 16;
                        if (sym >= max_symbol)
                                entry = len;
                        else
                                entry = sym |
                                        rfc_lookup_table.dist_extra_bit_count[sym]
                                                << DIST_SYM_EXTRA_OFFSET |
                                        len << SMALL_LONG_CODE_LEN_OFFSET;
                        size = 1 << (group_len[i] - ISAL_DECODE_SHORT_BITS);
                        for (code = (list[k] & 0xFFFF) >> ISAL_DECODE_SHORT_BITS; code < size;
                             code += 1 << (len - ISAL_DECODE_SHORT_BITS))
                                long_code_lookup[group_offset[i] + code] = entry;
                }
        }

        for (k = start[ISAL_DECODE_SHORT_BITS + 1]; k < start[MAX_HUFF_TREE_DEPTH + 1]; k++) {
                code = list[k] & ((1 << ISAL_DECODE_SHORT_BITS) - 1);
                i = short_code_lookup[code];
                if (i <= num_groups)
                        short_code_lookup[code] = group_offset[i] |
                                                  group_len[i] << SMALL_SHORT_CODE_LEN_OFFSET |
                                                  SMALL_FLAG_BIT;
        }

        return 0;
}

static int
header_matches_pregen(struct inflate_state *state)
{
#ifndef ISAL_STATIC_INFLATE_TABLE
        return 0;
#else
        uint8_t *in, *hdr;
        uint32_t in_end_bits, hdr_end_bits;
        uint32_t bytes_read_in, header_len, last_bits, last_bit_mask;
        uint64_t bits_read_mask;
        uint64_t hdr_stash, in_stash;
        const uint64_t bits_read_prior = 3; // Have read bfinal(1) and btype(2)

        /* Check if stashed read_in_bytes match header */
        hdr = &(hufftables_default.deflate_hdr[0]);
        bits_read_mask = (1ull << state->read_in_length) - 1;
        hdr_stash = (load_le_u64(hdr) >> bits_read_prior) & bits_read_mask;
        in_stash = state->read_in & bits_read_mask;

        if (hdr_stash != in_stash)
                return 0;

        /* Check if input is byte aligned */
        if ((state->read_in_length + bits_read_prior) % 8)
                return 0;

        /* Check if header bulk is the same */
        in = state->next_in;
        bytes_read_in = (uint32_t) ((state->read_in_length + bits_read_prior) / 8);
        header_len = (uint32_t) hufftables_default.deflate_hdr_count;

        if (memcmp(in, &hdr[bytes_read_in], header_len - bytes_read_in))
                return 0;

        /* If there are any last/end bits to the header check them too */
        last_bits = hufftables_default.deflate_hdr_extra_bits;
        last_bit_mask = (1 << last_bits) - 1;

        if (0 == last_bits) {
                state->next_in += header_len - bytes_read_in;
                state->avail_in -= header_len - bytes_read_in;
                state->read_in_length = 0;
                state->read_in = 0;
                return 1;
        }

        in_end_bits = in[header_len - bytes_read_in] & last_bit_mask;
        hdr_end_bits = hdr[header_len] & last_bit_mask;
        if (in_end_bits == hdr_end_bits) {
                state->next_in += header_len - bytes_read_in;
                state->avail_in -= header_len - bytes_read_in;
                state->read_in_length = 0;
                state->read_in = 0;
                inflate_in_read_bits(state, last_bits);
                return 1;
        }

        return 0;
#endif // ISAL_STATIC_INFLATE_TABLE
}

static int
setup_pregen_header(struct inflate_state *state)
{
#ifdef ISAL_STATIC_INFLATE_TABLE
        memcpy(&state->lit_huff_code, &pregen_lit_huff_code, sizeof(pregen_lit_huff_code));
        memcpy(&state->dist_huff_code, &pregen_dist_huff_code, sizeof(pregen_dist_huff_code));
        state->block_state = ISAL_BLOCK_CODED;
#endif // ISAL_STATIC_INFLATE_TABLE
        return 0;
}

/* Sets the inflate_huff_codes in state to be the huffcodes corresponding to the
 * deflate static header */
static int inline setup_static_header(struct inflate_state *state)
{
#ifdef ISAL_STATIC_INFLATE_TABLE
        memcpy(&state->lit_huff_code, &static_lit_huff_code, sizeof(static_lit_huff_code));
        memcpy(&state->dist_huff_code, &static_dist_huff_code, sizeof(static_dist_huff_code));
#else

#ifndef NO_STATIC_INFLATE_H
#warning "Defaulting to static inflate table fallback."
#warning                                                                                           \
        "For best performance, run generate_static_inflate, replace static_inflate.h, and recompile"
#endif
        uint8_t lens[LIT_LEN + 2], dist_lens[DIST_LEN + 2];
        uint16_t lit_count[MAX_HUFF_TREE_DEPTH + 1], dist_count[MAX_HUFF_TREE_DEPTH + 1];
        uint32_t next_code[MAX_HUFF_TREE_DEPTH + 1];
        struct lit_order lits;
        uint32_t i, max_dist = DIST_LEN;

        /* These are the code lengths of the static literal/length and distance codes defined
         * in the deflate standard RFC 1951 */
        memset(lens, 8, 144);
        memset(&lens[144], 9, 256 - 144);
        memset(&lens[256], 7, 280 - 256);
        memset(&lens[280], 8, LIT_LEN + 2 - 280);
        memset(dist_lens, 5, DIST_LEN + 2);

        count_lengths(lens, LIT_LEN + 2, lit_count);
        count_lengths(dist_lens, DIST_LEN + 2, dist_count);

        memset(lits.num, 0, sizeof(lits.num));
        for (i = 0; i < NUM_LITERALS; i++)
                lits.sym[lens[i]][lits.num[lens[i]]++] = i;

        set_next_codes(lit_count, next_code);
        if (make_inflate_huff_code_lit_len(&state->lit_huff_code, lens, LIT_LEN + 2, next_code,
                                           &lits, SINGLE_SYM_FLAG))
                return ISAL_INVALID_BLOCK;

        if (state->hist_bits && state->hist_bits < 15)
                max_dist = 2 * state->hist_bits;

        set_next_codes(dist_count, next_code);
        if (make_inflate_huff_code_dist(&state->dist_huff_code, dist_lens, DIST_LEN + 2, dist_count,
                                        next_code, max_dist))
                return ISAL_INVALID_BLOCK;
#endif
        state->block_state = ISAL_BLOCK_CODED;

        return 0;
}

/* Decodes the next symbol symbol in in_buffer using the huff code defined by
 * huff_code  and returns the value in next_lits and sym_count */
static void inline decode_next_lit_len(uint32_t *next_lits, uint32_t *sym_count,
                                       struct inflate_state *state,
                                       struct inflate_huff_code_large *huff_code)
{
        uint32_t next_bits;
        uint32_t next_sym;
        uint32_t bit_count;
        uint32_t bit_mask;

        if (state->read_in_length <= ISAL_DEF_MAX_CODE_LEN)
                inflate_in_load(state, 0);

        next_bits = state->read_in & ((1 << ISAL_DECODE_LONG_BITS) - 1);

        /* next_sym is a possible symbol decoded from next_bits. If bit 15 is 0,
         * next_code is a symbol. Bits 9:0 represent the symbol, and bits 14:10
         * represent the length of that symbols huffman code. If next_sym is not
         * a symbol, it provides a hint of where the large symbols containing
         * this code are located. Note the hint is at largest the location the
         * first actual symbol in the long code list.*/
        next_sym = huff_code->short_code_lookup[next_bits];

        if ((next_sym & LARGE_FLAG_BIT) == 0) {
                /* Return symbol found if next_code is a complete huffman code
                 * and shift in buffer over by the length of the next_code */
                bit_count = next_sym >> LARGE_SHORT_CODE_LEN_OFFSET;
                state->read_in >>= bit_count;
                state->read_in_length -= bit_count;

                if (bit_count == 0)
                        next_sym = INVALID_SYMBOL;

                *sym_count = (next_sym >> LARGE_SYM_COUNT_OFFSET) & LARGE_SYM_COUNT_MASK;
                *next_lits = next_sym & LARGE_SHORT_SYM_MASK;

        } else {
                /* If a symbol is not found, do a lookup in the long code
                 * list starting from the hint in next_sym */
                bit_mask = next_sym >> LARGE_SHORT_MAX_LEN_OFFSET;
                bit_mask = (1 << bit_mask) - 1;
                next_bits = state->read_in & bit_mask;
                next_sym = huff_code->long_code_lookup[(next_sym & LARGE_SHORT_SYM_MASK) +
                                                       (next_bits >> ISAL_DECODE_LONG_BITS)];
                bit_count = next_sym >> LARGE_LONG_CODE_LEN_OFFSET;
                state->read_in >>= bit_count;
                state->read_in_length -= bit_count;

                if (bit_count == 0)
                        next_sym = INVALID_SYMBOL;

                *sym_count = 1;
                *next_lits = next_sym & LARGE_LONG_SYM_MASK;
        }
}

static uint16_t inline decode_next_dist(struct inflate_state *state,
                                        struct inflate_huff_code_small *huff_code)
{
        uint64_t next_bits;
        uint16_t next_sym;
        uint32_t bit_count;
        uint32_t bit_mask;

        if (state->read_in_length <= ISAL_DEF_MAX_CODE_LEN)
                inflate_in_load(state, 0);

        next_bits = state->read_in & ((1 << ISAL_DECODE_SHORT_BITS) - 1);

        /* next_sym is a possible symbol decoded from next_bits. If bit 15 is 0,
         * next_code is a symbol. Bits 9:0 represent the symbol, and bits 14:10
         * represent the length of that symbols huffman code. If next_sym is not
         * a symbol, it provides a hint of where the large symbols containing
         * this code are located. Note the hint is at largest the location the
         * first actual symbol in the long code list.*/
        next_sym = huff_code->short_code_lookup[next_bits];

        if ((next_sym & SMALL_FLAG_BIT) == 0) {
                /* Return symbol found if next_code is a complete huffman code
                 * and shift in buffer over by the length of the next_code */
                bit_count = next_sym >> SMALL_SHORT_CODE_LEN_OFFSET;
                state->read_in >>= bit_count;
                state->read_in_length -= bit_count;

                if (bit_count == 0) {
                        state->read_in_length -= next_sym;
                        next_sym = INVALID_SYMBOL;
                }

                return next_sym & DIST_SYM_MASK;

        } else {
                /* If a symbol is not found, perform a linear search of the long code
                 * list starting from the hint in next_sym */
                bit_mask = (next_sym - SMALL_FLAG_BIT) >> SMALL_SHORT_CODE_LEN_OFFSET;
                bit_mask = (1 << bit_mask) - 1;
                next_bits = state->read_in & bit_mask;
                next_sym = huff_code->long_code_lookup[(next_sym & SMALL_SHORT_SYM_MASK) +
                                                       (next_bits >> ISAL_DECODE_SHORT_BITS)];
                bit_count = next_sym >> SMALL_LONG_CODE_LEN_OFFSET;
                state->read_in >>= bit_count;
                state->read_in_length -= bit_count;

                if (bit_count == 0) {
                        state->read_in_length -= next_sym;
                        next_sym = INVALID_SYMBOL;
                }

                return next_sym & DIST_SYM_MASK;
        }
}

/* The input bit buffer of an inflate_state, kept in local variables while decoding a header */
struct inflate_bits {
        uint64_t read_in;
        int32_t read_in_length;
        uint8_t *next_in;
        uint32_t avail_in;
};

/* Same as inflate_in_load() */
static void inline inflate_bits_load(struct inflate_bits *bits)
{
        uint8_t new_bytes;

        if (bits->read_in_length >= 64)
                return;

        if (bits->avail_in >= 8) {
                new_bytes = 8 - (bits->read_in_length + 7) / 8;
                bits->read_in |= load_le_u64(bits->next_in) << bits->read_in_length;
                bits->next_in += new_bytes;
                bits->avail_in -= new_bytes;
                bits->read_in_length += new_bytes * 8;
        } else {
                while (bits->read_in_length < 57 && bits->avail_in > 0) {
                        bits->read_in |= (uint64_t) *bits->next_in << bits->read_in_length;
                        bits->next_in++;
                        bits->avail_in--;
                        bits->read_in_length += 8;
                }
        }
}

/* Same as inflate_in_read_bits() */
static uint32_t inline inflate_bits_read(struct inflate_bits *bits, uint8_t bit_count)
{
        uint32_t ret;

        inflate_bits_load(bits);
        ret = bits->read_in & ((1 << bit_count) - 1);
        bits->read_in >>= bit_count;
        bits->read_in_length -= bit_count;

        return ret;
}

/*
 * Decodes the code lengths of a dynamic block header into lens[0..total), which must be zero,
 * adds the literals to lits by length, and returns 0 or the error setup_dynamic_header()
 * returns.
 */
static int inline decode_code_lengths(struct inflate_bits *bits, const uint16_t *table,
                                      uint8_t *lens, uint32_t total, struct lit_order *lits)
{
        uint32_t cur = 0, entry, len, rep, end;

        while (cur < total) {
                if (bits->read_in_length <= ISAL_DEF_MAX_CODE_LEN)
                        inflate_bits_load(bits);

                /* The code length is in the low bits of entry, so it can be the shift count */
                entry = table[bits->read_in & ((1 << HDR_TABLE_BITS) - 1)];
                bits->read_in >>= entry & 63;
                len = entry & 0xFF;
                bits->read_in_length -= len;

                if (bits->read_in_length < 0 || len == 0) {
                        if (bits->read_in_length >= 0)
                                return ISAL_INVALID_BLOCK;
                        if (cur > ISAL_DEF_LIT_SYMBOLS - 1 && lens[ISAL_DEF_LIT_SYMBOLS - 1] == 0)
                                return ISAL_INVALID_BLOCK;
                        return ISAL_END_INPUT;
                }

                entry >>= 8;
                if (entry < 16) {
                        if (cur < NUM_LITERALS)
                                lits->sym[entry][lits->num[entry]++] = cur;
                        lens[cur++] = entry;
                } else if (entry == 16) {
                        /* Repeat the previous length */
                        rep = 3 + inflate_bits_read(bits, 2);
                        if (cur == 0 || cur + rep > total)
                                return ISAL_INVALID_BLOCK;
                        len = lens[cur - 1];
                        memset(&lens[cur], len, rep);
                        end = cur + rep < NUM_LITERALS ? cur + rep : NUM_LITERALS;
                        for (; cur < end; cur++, rep--)
                                lits->sym[len][lits->num[len]++] = cur;
                        cur += rep;
                } else if (entry == 17) {
                        /* Repeat a length of 0 */
                        cur += 3 + inflate_bits_read(bits, 3);
                } else {
                        cur += 11 + inflate_bits_read(bits, 7);
                }
        }

        if (cur > total || lens[ISAL_DEF_LIT_SYMBOLS - 1] == 0)
                return ISAL_INVALID_BLOCK;

        if (bits->read_in_length < 0)
                return ISAL_END_INPUT;

        return 0;
}

/* Reads data from the in_buffer and sets the huff code corresponding to that
 * data */
static int inline setup_dynamic_header(struct inflate_state *state)
{
        struct inflate_bits bits;
        uint8_t lens[LIT_LEN + DIST_LEN];
        uint8_t code_lens[CODE_LEN_CODES];
        uint16_t table[1 << HDR_TABLE_BITS];
        uint16_t lit_count[MAX_HUFF_TREE_DEPTH + 1], dist_count[MAX_HUFF_TREE_DEPTH + 1];
        uint32_t next_code[MAX_HUFF_TREE_DEPTH + 1];
        struct lit_order lits;
        uint32_t i, hclen, hdist, hlit, length, flag = 0;
        uint32_t multisym = DEFAULT_SYM_FLAG, max_dist = DIST_LEN;
        int ret;

        /* This order is defined in RFC 1951 page 13 */
        const uint8_t code_length_order[CODE_LEN_CODES] = { 0x10, 0x11, 0x12, 0x00, 0x08,
                                                            0x07, 0x09, 0x06, 0x0a, 0x05,
                                                            0x0b, 0x04, 0x0c, 0x03, 0x0d,
                                                            0x02, 0x0e, 0x01, 0x0f };

        /* If you are given a whole header and it matches the pregen header */
        if (state->avail_in > (hufftables_default.deflate_hdr_count + sizeof(uint64_t)) &&
            header_matches_pregen(state))
                return setup_pregen_header(state);

        if (state->bfinal && state->avail_in <= SINGLE_SYM_THRESH) {
                multisym = SINGLE_SYM_FLAG;
        } else if (state->bfinal && state->avail_in <= DOUBLE_SYM_THRESH) {
                multisym = DOUBLE_SYM_FLAG;
        }

        bits.read_in = state->read_in;
        bits.read_in_length = state->read_in_length;
        bits.next_in = state->next_in;
        bits.avail_in = state->avail_in;

        /* These variables are defined in the deflate standard, RFC 1951 */
        inflate_bits_load(&bits);
        if (bits.read_in_length < 14) {
                ret = ISAL_END_INPUT;
                goto done;
        }

        hlit = bits.read_in & 0x1F;
        hdist = (bits.read_in >> 5) & 0x1F;
        hclen = (bits.read_in >> 10) & 0xF;
        bits.read_in >>= 14;
        bits.read_in_length -= 14;

        if (hlit > 29 || hdist > 29 || hclen > 15) {
                ret = ISAL_INVALID_BLOCK;
                goto done;
        }

        /* Create the code huffman code for decoding the lit/len and dist huffman codes */
        memset(code_lens, 0, sizeof(code_lens));
        for (i = 0; i < hclen + 4; i++) {
                /* The first four lengths fit in the bits loaded for hlit, hdist and hclen */
                if (i == 4)
                        inflate_bits_load(&bits);
                length = bits.read_in & 7;
                bits.read_in >>= 3;
                bits.read_in_length -= 3;
                code_lens[code_length_order[i]] = length;
                flag |= length;
        }
        if (hclen == 0)
                inflate_bits_load(&bits);

        if (bits.read_in_length < 0) {
                ret = ISAL_END_INPUT;
                goto done;
        }

        if (!flag || make_header_table(table, code_lens)) {
                ret = ISAL_INVALID_BLOCK;
                goto done;
        }

        /* Decode the lit/len and dist code lengths using the code length code */
        memset(lens, 0, sizeof(lens));
        memset(lits.num, 0, sizeof(lits.num));
        ret = decode_code_lengths(&bits, table, lens, LIT_TABLE_SIZE + hlit + hdist + 1, &lits);
        if (ret)
                goto done;

        count_lengths(&lens[LIT_TABLE_SIZE + hlit], hdist + 1, dist_count);
        if (set_next_codes(dist_count, next_code)) {
                ret = ISAL_INVALID_BLOCK;
                goto done;
        }

        if (state->hist_bits && state->hist_bits < 15)
                max_dist = 2 * state->hist_bits;

        if (make_inflate_huff_code_dist(&state->dist_huff_code, &lens[LIT_TABLE_SIZE + hlit],
                                        hdist + 1, dist_count, next_code, max_dist)) {
                ret = ISAL_INVALID_BLOCK;
                goto done;
        }

        /* Count the lengths of end of block and the length symbols, and add the literals */
        count_lengths(&lens[NUM_LITERALS], hlit + 1, lit_count);
        for (i = 1; i <= MAX_HUFF_TREE_DEPTH; i++)
                lit_count[i] += lits.num[i];

        if (set_next_codes(lit_count, next_code)) {
                ret = ISAL_INVALID_BLOCK;
                goto done;
        }

        if (make_inflate_huff_code_lit_len(&state->lit_huff_code, lens, LIT_TABLE_SIZE + hlit,
                                           next_code, &lits, multisym)) {
                ret = ISAL_INVALID_BLOCK;
                goto done;
        }

        state->block_state = ISAL_BLOCK_CODED;

done:
        state->read_in = bits.read_in;
        state->read_in_length = bits.read_in_length;
        state->next_in = bits.next_in;
        state->avail_in = bits.avail_in;

        return ret;
}

/* Reads in the header pointed to by in_stream and sets up state to reflect that
 * header information*/
static int
read_header(struct inflate_state *state)
{
        uint8_t bytes;
        uint64_t btype;
        uint16_t len, nlen;
        int ret = 0;

        /* btype and bfinal are defined in RFC 1951, bfinal represents whether
         * the current block is the end of block, and btype represents the
         * encoding method on the current block. */

        state->bfinal = (uint32_t) inflate_in_read_bits(state, 1);
        btype = inflate_in_read_bits(state, 2);

        if (state->read_in_length < 0)
                ret = ISAL_END_INPUT;

        else if (btype == 0) {
                inflate_in_load(state, 40);
                bytes = state->read_in_length / 8;

                if (bytes < 4)
                        return ISAL_END_INPUT;

                state->read_in >>= state->read_in_length % 8;
                state->read_in_length = bytes * 8;

                len = state->read_in & 0xFFFF;
                state->read_in >>= 16;
                nlen = state->read_in & 0xFFFF;
                state->read_in >>= 16;
                state->read_in_length -= 32;

                /* Check if len and nlen match */
                if (len != (~nlen & 0xffff))
                        return ISAL_INVALID_BLOCK;

                state->type0_block_len = len;
                state->block_state = ISAL_BLOCK_TYPE0;

                ret = 0;

        } else if (btype == 1)
                ret = setup_static_header(state);

        else if (btype == 2)
                ret = setup_dynamic_header(state);

        else
                ret = ISAL_INVALID_BLOCK;

        return ret;
}

/* Reads in the header pointed to by in_stream and sets up state to reflect that
 * header information*/
static int
read_header_stateful(struct inflate_state *state)
{
        uint64_t read_in_start = state->read_in;
        int32_t read_in_length_start = state->read_in_length;
        uint8_t *next_in_start = state->next_in;
        uint32_t avail_in_start = state->avail_in;
        int block_state_start = state->block_state;
        int ret;
        uint32_t copy_size;
        int32_t bytes_read;

        if (block_state_start == ISAL_BLOCK_HDR) {
                /* Setup so read_header decodes data in tmp_in_buffer */
                copy_size = ISAL_DEF_MAX_HDR_SIZE - state->tmp_in_size;
                if (copy_size > state->avail_in)
                        copy_size = state->avail_in;

                memcpy(&state->tmp_in_buffer[state->tmp_in_size], state->next_in, copy_size);
                state->next_in = state->tmp_in_buffer;
                state->avail_in = state->tmp_in_size + copy_size;
        }

        ret = read_header(state);

        if (block_state_start == ISAL_BLOCK_HDR) {
                /* Setup so state is restored to a valid state */
                bytes_read = (int32_t) (state->next_in - state->tmp_in_buffer - state->tmp_in_size);
                if (bytes_read < 0)
                        bytes_read = 0;
                state->next_in = next_in_start + bytes_read;
                state->avail_in = avail_in_start - bytes_read;
        }

        if (ret == ISAL_END_INPUT) {
                /* Save off data so header can be decoded again with more data */
                state->read_in = read_in_start;
                state->read_in_length = read_in_length_start;
                memcpy(&state->tmp_in_buffer[state->tmp_in_size], next_in_start, avail_in_start);
                state->tmp_in_size += avail_in_start;
                state->avail_in = 0;
                state->next_in = next_in_start + avail_in_start;
                state->block_state = ISAL_BLOCK_HDR;
        } else
                state->tmp_in_size = 0;

        return ret;
}

static int inline decode_literal_block(struct inflate_state *state)
{
        uint32_t len = state->type0_block_len;
        uint32_t bytes = state->read_in_length / 8;
        uint64_t read_in;
        /* If the block is uncompressed, perform a memcopy while
         * updating state data */
        state->block_state = state->bfinal ? ISAL_BLOCK_INPUT_DONE : ISAL_BLOCK_NEW_HDR;

        if (state->avail_out < len) {
                len = state->avail_out;
                state->block_state = ISAL_BLOCK_TYPE0;
        }

        if (state->avail_in + bytes < len) {
                len = state->avail_in + bytes;
                state->block_state = ISAL_BLOCK_TYPE0;
        }
        if (state->read_in_length) {
                read_in = to_le64(state->read_in);
                if (len >= bytes) {
                        memcpy(state->next_out, &read_in, bytes);

                        state->next_out += bytes;
                        state->avail_out -= bytes;
                        state->total_out += bytes;
                        state->type0_block_len -= bytes;

                        state->read_in = 0;
                        state->read_in_length = 0;
                        len -= bytes;
                        bytes = 0;

                } else {
                        memcpy(state->next_out, &read_in, len);

                        state->next_out += len;
                        state->avail_out -= len;
                        state->total_out += len;
                        state->type0_block_len -= len;

                        state->read_in >>= 8 * len;
                        state->read_in_length -= 8 * len;
                        bytes -= len;
                        len = 0;
                }
        }
        memcpy(state->next_out, state->next_in, len);

        state->next_out += len;
        state->avail_out -= len;
        state->total_out += len;
        state->next_in += len;
        state->avail_in -= len;
        state->type0_block_len -= len;

        if (state->avail_in + bytes == 0 && state->block_state != ISAL_BLOCK_INPUT_DONE)
                return ISAL_END_INPUT;

        if (state->avail_out == 0 && state->type0_block_len > 0)
                return ISAL_OUT_OVERFLOW;

        return 0;
}

/* Decodes the next block if it was encoded using a huffman code */
int
decode_huffman_code_block_stateless_base(struct inflate_state *state, uint8_t *start_out)
{
        uint16_t next_lit;
        uint32_t next_dist;
        uint32_t repeat_length;
        uint32_t look_back_dist = 0;
        uint64_t read_in_tmp;
        int32_t read_in_length_tmp;
        uint8_t *next_in_tmp, *next_out_tmp;
        uint32_t avail_in_tmp, avail_out_tmp, total_out_tmp;
        uint32_t next_lits, sym_count;
        struct rfc1951_tables *rfc = &rfc_lookup_table;

        state->copy_overflow_length = 0;
        state->copy_overflow_distance = 0;

        while (state->block_state == ISAL_BLOCK_CODED) {
                /* While not at the end of block, decode the next
                 * symbol */
                inflate_in_load(state, 0);

                read_in_tmp = state->read_in;
                read_in_length_tmp = state->read_in_length;
                next_in_tmp = state->next_in;
                avail_in_tmp = state->avail_in;
                next_out_tmp = state->next_out;
                avail_out_tmp = state->avail_out;
                total_out_tmp = state->total_out;

                decode_next_lit_len(&next_lits, &sym_count, state, &state->lit_huff_code);

                if (sym_count == 0)
                        return ISAL_INVALID_SYMBOL;

                if (state->read_in_length < 0) {
                        state->read_in = read_in_tmp;
                        state->read_in_length = read_in_length_tmp;
                        state->next_in = next_in_tmp;
                        state->avail_in = avail_in_tmp;
                        return ISAL_END_INPUT;
                }

                while (sym_count > 0) {
                        next_lit = next_lits & 0xffff;
                        if (next_lit < 256 || sym_count > 1) {
                                /* If the next symbol is a literal,
                                 * write out the symbol and update state
                                 * data accordingly. */
                                if (state->avail_out < 1) {
                                        state->write_overflow_lits = next_lits;
                                        state->write_overflow_len = sym_count;
                                        next_lits = next_lits >> (8 * (sym_count - 1));
                                        sym_count = 1;

                                        if (next_lits < 256)
                                                return ISAL_OUT_OVERFLOW;
                                        else if (next_lits == 256) {
                                                state->write_overflow_len -= 1;
                                                state->block_state = state->bfinal
                                                                             ? ISAL_BLOCK_INPUT_DONE
                                                                             : ISAL_BLOCK_NEW_HDR;
                                                return ISAL_OUT_OVERFLOW;
                                        } else {
                                                state->write_overflow_len -= 1;
                                                continue;
                                        }
                                }

                                *state->next_out = (uint8_t) next_lit;
                                state->next_out++;
                                state->avail_out--;
                                state->total_out++;

                        } else if (next_lit == 256) {
                                /* If the next symbol is the end of
                                 * block, update the state data
                                 * accordingly */
                                state->block_state =
                                        state->bfinal ? ISAL_BLOCK_INPUT_DONE : ISAL_BLOCK_NEW_HDR;

                        } else if (next_lit <= MAX_LIT_LEN_SYM) {
                                /* Else if the next symbol is a repeat
                                 * length, read in the length extra
                                 * bits, the distance code, the distance
                                 * extra bits. Then write out the
                                 * corresponding data and update the
                                 * state data accordingly*/
                                repeat_length = next_lit - 254;
                                next_dist = decode_next_dist(state, &state->dist_huff_code);

                                if (state->read_in_length >= 0) {
                                        if (next_dist >= DIST_LEN)
                                                return ISAL_INVALID_SYMBOL;

                                        look_back_dist = (uint32_t) ((
                                                rfc->dist_start[next_dist] +
                                                inflate_in_read_bits(
                                                        state,
                                                        rfc->dist_extra_bit_count[next_dist])));
                                }

                                if (state->read_in_length < 0) {
                                        state->read_in = read_in_tmp;
                                        state->read_in_length = read_in_length_tmp;
                                        state->next_in = next_in_tmp;
                                        state->avail_in = avail_in_tmp;
                                        state->next_out = next_out_tmp;
                                        state->avail_out = avail_out_tmp;
                                        state->total_out = total_out_tmp;
                                        state->write_overflow_lits = 0;
                                        state->write_overflow_len = 0;
                                        return ISAL_END_INPUT;
                                }

                                if (state->next_out - look_back_dist < start_out)
                                        return ISAL_INVALID_LOOKBACK;

                                if (state->avail_out < repeat_length) {
                                        state->copy_overflow_length =
                                                repeat_length - state->avail_out;
                                        state->copy_overflow_distance = look_back_dist;
                                        repeat_length = state->avail_out;
                                }

                                if (look_back_dist > repeat_length)
                                        memcpy(state->next_out, state->next_out - look_back_dist,
                                               repeat_length);
                                else
                                        byte_copy(state->next_out, look_back_dist, repeat_length);

                                state->next_out += repeat_length;
                                state->avail_out -= repeat_length;
                                state->total_out += repeat_length;

                                if (state->copy_overflow_length > 0)
                                        return ISAL_OUT_OVERFLOW;
                        } else
                                /* Else the read in bits do not
                                 * correspond to any valid symbol */
                                return ISAL_INVALID_SYMBOL;

                        next_lits >>= 8;
                        sym_count--;
                }
        }
        return 0;
}

void
isal_inflate_init(struct inflate_state *state)
{

        state->read_in = 0;
        state->read_in_length = 0;
        state->next_in = NULL;
        state->avail_in = 0;
        state->next_out = NULL;
        state->avail_out = 0;
        state->total_out = 0;
        state->dict_length = 0;
        state->block_state = ISAL_BLOCK_NEW_HDR;
        state->bfinal = 0;
        state->crc_flag = 0;
        state->crc = 0;
        state->hist_bits = 0;
        state->type0_block_len = 0;
        state->write_overflow_lits = 0;
        state->write_overflow_len = 0;
        state->copy_overflow_length = 0;
        state->copy_overflow_distance = 0;
        state->wrapper_flag = 0;
        state->tmp_in_size = 0;
        state->tmp_out_processed = 0;
        state->tmp_out_valid = 0;
}

void
isal_inflate_reset(struct inflate_state *state)
{
        state->read_in = 0;
        state->read_in_length = 0;
        state->total_out = 0;
        state->dict_length = 0;
        state->block_state = ISAL_BLOCK_NEW_HDR;
        state->bfinal = 0;
        state->crc = 0;
        state->type0_block_len = 0;
        state->write_overflow_lits = 0;
        state->write_overflow_len = 0;
        state->copy_overflow_length = 0;
        state->copy_overflow_distance = 0;
        state->wrapper_flag = 0;
        state->tmp_in_size = 0;
        state->tmp_out_processed = 0;
        state->tmp_out_valid = 0;
}

static inline uint32_t
fixed_size_read(struct inflate_state *state, uint8_t **read_buf, uint32_t read_size)
{
        uint32_t tmp_in_size = state->tmp_in_size;

        if ((uint64_t) state->avail_in + tmp_in_size < read_size) {
                memcpy(state->tmp_in_buffer + tmp_in_size, state->next_in, state->avail_in);
                tmp_in_size += state->avail_in;
                state->tmp_in_size = tmp_in_size;
                state->next_in += state->avail_in;
                state->avail_in = 0;

                return ISAL_END_INPUT;
        }

        *read_buf = state->next_in;
        if (tmp_in_size) {
                memcpy(state->tmp_in_buffer + tmp_in_size, state->next_in, read_size - tmp_in_size);
                *read_buf = state->tmp_in_buffer;
                state->tmp_in_size = 0;
        }

        state->next_in += read_size - tmp_in_size;
        state->avail_in -= read_size - tmp_in_size;
        tmp_in_size = 0;

        return 0;
}

static inline uint32_t
buffer_header_copy(struct inflate_state *state, uint32_t in_len, uint8_t *buf, uint32_t buffer_len,
                   uint32_t offset, uint32_t buf_error)
{
        uint32_t len = in_len;
        uint32_t buf_len = buffer_len - offset;

        if (len > state->avail_in)
                len = state->avail_in;

        if (buf != NULL && buf_len < len) {
                memcpy(&buf[offset], state->next_in, buf_len);
                state->next_in += buf_len;
                state->avail_in -= buf_len;
                state->count = in_len - buf_len;
                return buf_error;
        } else {
                if (buf != NULL)
                        memcpy(&buf[offset], state->next_in, len);
                state->next_in += len;
                state->avail_in -= len;
                state->count = in_len - len;

                if (len == in_len)
                        return 0;
                else
                        return ISAL_END_INPUT;
        }
}

static inline uint32_t
string_header_copy(struct inflate_state *state, char *str_buf, uint32_t str_len, uint32_t offset,
                   uint32_t str_error)
{
        uint32_t len, max_len = str_len - offset;

        if (max_len > state->avail_in || str_buf == NULL)
                max_len = state->avail_in;

        len = (uint32_t) strnlen((char *) state->next_in, max_len);

        if (str_buf != NULL)
                memcpy(&str_buf[offset], state->next_in, len);

        state->next_in += len;
        state->avail_in -= len;
        state->count += len;

        if (str_buf != NULL && len == (str_len - offset))
                return str_error;
        else if (state->avail_in <= 0)
                return ISAL_END_INPUT;
        else {
                state->next_in++;
                state->avail_in--;
                state->count = 0;
                if (str_buf != NULL)
                        str_buf[offset + len] = 0;
        }

        return 0;
}

static int
check_gzip_checksum(struct inflate_state *state)
{
        uint64_t trailer, crc, total_out;
        uint8_t *next_in;
        uint32_t byte_count, offset, tmp_in_size = state->tmp_in_size;
        int ret;

        if (state->read_in_length >= 8 * GZIP_TRAILER_LEN) {
                /* The following is unnecessary as state->read_in_length == 64 */
                /* bit_count = state->read_in_length % 8; */
                /* state->read_in >>= bit_count; */
                /* state->read_in_length -= bit_count; */

                trailer = state->read_in;
                state->read_in_length = 0;
                state->read_in = 0;
        } else {
                if (state->read_in_length >= 8) {
                        byte_count = state->read_in_length / 8;
                        offset = state->read_in_length % 8;

                        store_le_u64(state->tmp_in_buffer + tmp_in_size, state->read_in >> offset);
                        state->read_in = 0;
                        state->read_in_length = 0;

                        tmp_in_size += byte_count;
                        state->tmp_in_size = tmp_in_size;
                }

                ret = fixed_size_read(state, &next_in, GZIP_TRAILER_LEN);
                if (ret) {
                        state->block_state = ISAL_CHECKSUM_CHECK;
                        return ret;
                }

                trailer = load_le_u64(next_in);
        }

        state->block_state = ISAL_BLOCK_FINISH;

        crc = state->crc;
        total_out = state->total_out;

        if (trailer != (crc | (total_out << 32)))
                return ISAL_INCORRECT_CHECKSUM;
        else
                return ISAL_DECOMP_OK;
}

static int
check_zlib_checksum(struct inflate_state *state)
{

        uint32_t trailer;
        uint8_t *next_in;
        uint32_t byte_count, offset, tmp_in_size = state->tmp_in_size;
        int ret, bit_count;

        if (state->read_in_length >= 8 * ZLIB_TRAILER_LEN) {
                bit_count = state->read_in_length % 8;
                state->read_in >>= bit_count;
                state->read_in_length -= bit_count;

                trailer = (uint32_t) state->read_in;

                state->read_in_length -= 8 * ZLIB_TRAILER_LEN;
                state->read_in >>= 8 * ZLIB_TRAILER_LEN;
        } else {
                if (state->read_in_length >= 8) {
                        byte_count = state->read_in_length / 8;
                        offset = state->read_in_length % 8;

                        store_le_u64(state->tmp_in_buffer + tmp_in_size, state->read_in >> offset);
                        state->read_in = 0;
                        state->read_in_length = 0;

                        tmp_in_size += byte_count;
                        state->tmp_in_size = tmp_in_size;
                }

                ret = fixed_size_read(state, &next_in, ZLIB_TRAILER_LEN);
                if (ret) {
                        state->block_state = ISAL_CHECKSUM_CHECK;
                        return ret;
                }

                trailer = load_le_u32(next_in);
        }

        state->block_state = ISAL_BLOCK_FINISH;

        if (isal_bswap32(trailer) != state->crc)
                return ISAL_INCORRECT_CHECKSUM;
        else
                return ISAL_DECOMP_OK;
}

int
isal_read_gzip_header(struct inflate_state *state, struct isal_gzip_header *gz_hdr)
{
        int cm, flags = gz_hdr->flags, id1, id2;
        uint16_t xlen = gz_hdr->extra_len;
        uint32_t block_state = state->block_state;
        uint8_t *start_in = state->next_in, *next_in;
        uint32_t tmp_in_size = state->tmp_in_size;
        uint32_t count = state->count, offset;
        uint32_t hcrc = gz_hdr->hcrc;
        int ret = 0;

        /* This switch is a jump table into the function so that decoding the
         * header can continue where it stopped on the last call */
        switch (block_state) {
        case ISAL_BLOCK_NEW_HDR:
                state->count = 0;
                flags = UNDEFINED_FLAG;
                if (tmp_in_size == 0)
                        hcrc = 0;

                ret = fixed_size_read(state, &next_in, GZIP_HDR_BASE);
                if (ret)
                        break;

                id1 = next_in[0];
                id2 = next_in[1];
                cm = next_in[2];
                flags = next_in[3];
                gz_hdr->time = load_le_u32(next_in + 4);
                gz_hdr->xflags = *(next_in + 8);
                gz_hdr->os = *(next_in + 9);

                if (id1 != 0x1f || id2 != 0x8b)
                        return ISAL_INVALID_WRAPPER;

                if (cm != DEFLATE_METHOD)
                        return ISAL_UNSUPPORTED_METHOD;

                gz_hdr->text = 0;
                if (flags & TEXT_FLAG)
                        gz_hdr->text = 1;

                gz_hdr->flags = flags;

                if (flags & EXTRA_FLAG) {
                case ISAL_GZIP_EXTRA_LEN:
                        ret = fixed_size_read(state, &next_in, GZIP_EXTRA_LEN);
                        if (ret) {
                                state->block_state = ISAL_GZIP_EXTRA_LEN;
                                break;
                        }

                        xlen = load_le_u16(next_in);
                        count = xlen;

                        gz_hdr->extra_len = xlen;

                case ISAL_GZIP_EXTRA:
                        offset = gz_hdr->extra_len - count;
                        ret = buffer_header_copy(state, count, gz_hdr->extra, gz_hdr->extra_buf_len,
                                                 offset, ISAL_EXTRA_OVERFLOW);

                        if (ret) {
                                state->block_state = ISAL_GZIP_EXTRA;
                                break;
                        }
                } else {
                        gz_hdr->extra_len = 0;
                }

                if (flags & NAME_FLAG) {
                case ISAL_GZIP_NAME:
                        offset = state->count;
                        ret = string_header_copy(state, gz_hdr->name, gz_hdr->name_buf_len, offset,
                                                 ISAL_NAME_OVERFLOW);
                        if (ret) {
                                state->block_state = ISAL_GZIP_NAME;
                                break;
                        }
                }

                if (flags & COMMENT_FLAG) {
                case ISAL_GZIP_COMMENT:
                        offset = state->count;
                        ret = string_header_copy(state, gz_hdr->comment, gz_hdr->comment_buf_len,
                                                 offset, ISAL_COMMENT_OVERFLOW);
                        if (ret) {
                                state->block_state = ISAL_GZIP_COMMENT;
                                break;
                        }
                }

                if (flags & HCRC_FLAG) {
                        hcrc = crc32_gzip_refl(hcrc, start_in, state->next_in - start_in);
                        gz_hdr->hcrc = hcrc;

                case ISAL_GZIP_HCRC:
                        ret = fixed_size_read(state, &next_in, GZIP_HCRC_LEN);
                        if (ret) {
                                state->block_state = ISAL_GZIP_HCRC;
                                return ret;
                        }

                        if ((hcrc & 0xffff) != load_le_u16(next_in))
                                return ISAL_INCORRECT_CHECKSUM;
                }

                state->wrapper_flag = 1;
                state->block_state = ISAL_BLOCK_NEW_HDR;
                return ISAL_DECOMP_OK;
        }

        if (flags & HCRC_FLAG)
                gz_hdr->hcrc = crc32_gzip_refl(hcrc, start_in, state->next_in - start_in);

        return ret;
}

int
isal_read_zlib_header(struct inflate_state *state, struct isal_zlib_header *zlib_hdr)
{
        int cmf, method, flags;
        uint32_t block_state = state->block_state;
        uint8_t *next_in;
        int ret = 0;

        switch (block_state) {
        case ISAL_BLOCK_NEW_HDR:
                zlib_hdr->dict_flag = 0;
                ret = fixed_size_read(state, &next_in, ZLIB_HDR_BASE);
                if (ret)
                        break;

                cmf = *next_in;
                method = cmf & 0xf;
                flags = *(next_in + 1);

                zlib_hdr->info = cmf >> ZLIB_INFO_OFFSET;
                zlib_hdr->dict_flag = (flags & ZLIB_DICT_FLAG) ? 1 : 0;
                zlib_hdr->level = flags >> ZLIB_LEVEL_OFFSET;

                if (method != DEFLATE_METHOD)
                        return ISAL_UNSUPPORTED_METHOD;

                if ((256 * cmf + flags) % 31 != 0)
                        return ISAL_INCORRECT_CHECKSUM;

                if (zlib_hdr->dict_flag) {
                case ISAL_ZLIB_DICT:
                        ret = fixed_size_read(state, &next_in, ZLIB_DICT_LEN);
                        if (ret) {
                                state->block_state = ISAL_ZLIB_DICT;
                                break;
                        }

                        zlib_hdr->dict_id = load_be_u32(next_in);
                }

                state->wrapper_flag = 1;
                state->block_state = ISAL_BLOCK_NEW_HDR;
        }

        return ret;
}

int
isal_inflate_set_dict(struct inflate_state *state, uint8_t *dict, uint32_t dict_len)
{

        if (state->block_state != ISAL_BLOCK_NEW_HDR ||
            state->tmp_out_processed != state->tmp_out_valid)
                return ISAL_INVALID_STATE;

        if (dict_len > IGZIP_HIST_SIZE) {
                dict = dict + dict_len - IGZIP_HIST_SIZE;
                dict_len = IGZIP_HIST_SIZE;
        }

        memcpy(state->tmp_out_buffer, dict, dict_len);
        state->tmp_out_processed = dict_len;
        state->tmp_out_valid = dict_len;
        state->dict_length = dict_len;

        return COMP_OK;
}

int
isal_inflate_stateless(struct inflate_state *state)
{
        int ret = 0;
        uint8_t *start_out = state->next_out;

        state->read_in = 0;
        state->read_in_length = 0;
        state->block_state = ISAL_BLOCK_NEW_HDR;
        state->dict_length = 0;
        state->bfinal = 0;
        state->crc = 0;
        state->total_out = 0;
        state->hist_bits = 0;
        state->tmp_in_size = 0;

        if (state->crc_flag == IGZIP_GZIP) {
                struct isal_gzip_header gz_hdr;

                isal_gzip_header_init(&gz_hdr);
                ret = isal_read_gzip_header(state, &gz_hdr);
                if (ret)
                        return ret;
        } else if (state->crc_flag == IGZIP_ZLIB) {
                struct isal_zlib_header z_hdr;

                isal_zlib_header_init(&z_hdr);
                ret = isal_read_zlib_header(state, &z_hdr);
                if (ret)
                        return ret;
                if (z_hdr.dict_flag)
                        return ISAL_NEED_DICT;
        }

        while (state->block_state != ISAL_BLOCK_FINISH) {
                if (state->block_state == ISAL_BLOCK_NEW_HDR) {
                        ret = read_header(state);

                        if (ret)
                                break;
                }

                if (state->block_state == ISAL_BLOCK_TYPE0)
                        ret = decode_literal_block(state);
                else
                        ret = decode_huffman_code_block_stateless(state, start_out);

                if (ret)
                        break;
                if (state->block_state == ISAL_BLOCK_INPUT_DONE)
                        state->block_state = ISAL_BLOCK_FINISH;
        }

        /* Undo count stuff of bytes read into the read buffer */
        state->next_in -= state->read_in_length / 8;
        state->avail_in += state->read_in_length / 8;
        state->read_in_length = 0;
        state->read_in = 0;

        if (!ret && state->crc_flag) {
                update_checksum(state, start_out, state->next_out - start_out);
                switch (state->crc_flag) {
                case ISAL_ZLIB:
                case ISAL_ZLIB_NO_HDR_VER:
                        finalize_adler32(state);
                        ret = check_zlib_checksum(state);
                        break;

                case ISAL_ZLIB_NO_HDR:
                        finalize_adler32(state);
                        break;

                case ISAL_GZIP:
                case ISAL_GZIP_NO_HDR_VER:
                        ret = check_gzip_checksum(state);
                        break;
                }
        }

        return ret;
}

int
isal_inflate(struct inflate_state *state)
{

        uint8_t *start_out = state->next_out;
        uint32_t avail_out = state->avail_out;
        uint32_t copy_size = 0;
        int32_t shift_size = 0;
        int ret = 0;

        if (!state->wrapper_flag && state->crc_flag == IGZIP_GZIP) {
                struct isal_gzip_header gz_hdr;

                isal_gzip_header_init(&gz_hdr);
                ret = isal_read_gzip_header(state, &gz_hdr);
                if (ret < 0)
                        return ret;
                else if (ret > 0)
                        return ISAL_DECOMP_OK;
        } else if (!state->wrapper_flag && state->crc_flag == IGZIP_ZLIB) {
                struct isal_zlib_header z_hdr;

                isal_zlib_header_init(&z_hdr);
                ret = isal_read_zlib_header(state, &z_hdr);
                if (ret < 0)
                        return ret;
                else if (ret > 0)
                        return ISAL_DECOMP_OK;

                if (z_hdr.dict_flag) {
                        state->dict_id = z_hdr.dict_id;
                        return ISAL_NEED_DICT;
                }
        } else if (state->block_state == ISAL_CHECKSUM_CHECK) {
                switch (state->crc_flag) {
                case ISAL_ZLIB:
                case ISAL_ZLIB_NO_HDR_VER:
                        ret = check_zlib_checksum(state);
                        break;
                case ISAL_GZIP:
                case ISAL_GZIP_NO_HDR_VER:
                        ret = check_gzip_checksum(state);
                        break;
                }

                return (ret > 0) ? ISAL_DECOMP_OK : ret;
        }

        if (state->block_state != ISAL_BLOCK_FINISH) {
                state->total_out += state->tmp_out_valid - state->tmp_out_processed;
                /* If space in tmp_out buffer, decompress into the tmp_out_buffer */
                if (state->tmp_out_valid < 2 * ISAL_DEF_HIST_SIZE) {
                        /* Setup to start decoding into temp buffer */
                        state->next_out = &state->tmp_out_buffer[state->tmp_out_valid];
                        state->avail_out = sizeof(state->tmp_out_buffer) - ISAL_LOOK_AHEAD -
                                           state->tmp_out_valid;

                        if ((int32_t) state->avail_out < 0)
                                state->avail_out = 0;

                        /* Decode into internal buffer until exit */
                        while (state->block_state != ISAL_BLOCK_INPUT_DONE) {
                                if (state->block_state == ISAL_BLOCK_NEW_HDR ||
                                    state->block_state == ISAL_BLOCK_HDR) {
                                        ret = read_header_stateful(state);

                                        if (ret)
                                                break;
                                }

                                if (state->block_state == ISAL_BLOCK_TYPE0) {
                                        ret = decode_literal_block(state);
                                } else {
                                        uint8_t *tmp = state->tmp_out_buffer;
                                        ret = decode_huffman_code_block_stateless(state, tmp);
                                }

                                if (ret)
                                        break;
                        }

                        /* Copy valid data from internal buffer into out_buffer */
                        if (state->write_overflow_len != 0) {
                                store_le_u32(state->next_out, state->write_overflow_lits);
                                state->next_out += state->write_overflow_len;
                                state->total_out += state->write_overflow_len;
                                state->write_overflow_lits = 0;
                                state->write_overflow_len = 0;
                        }

                        if (state->copy_overflow_length != 0) {
                                byte_copy(state->next_out, state->copy_overflow_distance,
                                          state->copy_overflow_length);
                                state->tmp_out_valid += state->copy_overflow_length;
                                state->next_out += state->copy_overflow_length;
                                state->total_out += state->copy_overflow_length;
                                state->copy_overflow_distance = 0;
                                state->copy_overflow_length = 0;
                        }

                        state->tmp_out_valid = (int32_t) (state->next_out - state->tmp_out_buffer);

                        /* Setup state for decompressing into out_buffer */
                        state->next_out = start_out;
                        state->avail_out = avail_out;
                }

                /* Copy data from tmp_out buffer into out_buffer */
                copy_size = state->tmp_out_valid - state->tmp_out_processed;
                if (copy_size > avail_out)
                        copy_size = avail_out;

                memcpy(state->next_out, &state->tmp_out_buffer[state->tmp_out_processed],
                       copy_size);

                state->tmp_out_processed += copy_size;
                state->avail_out -= copy_size;
                state->next_out += copy_size;

                if (ret == ISAL_INVALID_LOOKBACK || ret == ISAL_INVALID_BLOCK ||
                    ret == ISAL_INVALID_SYMBOL) {
                        /* Set total_out to not count data in tmp_out_buffer */
                        state->total_out -= state->tmp_out_valid - state->tmp_out_processed;
                        if (state->crc_flag)
                                update_checksum(state, start_out, state->next_out - start_out);
                        return ret;
                }

                /* If all data from tmp_out buffer has been processed, start
                 * decompressing into the out buffer */
                if (state->tmp_out_processed == state->tmp_out_valid) {
                        while (state->block_state != ISAL_BLOCK_INPUT_DONE) {
                                if (state->block_state == ISAL_BLOCK_NEW_HDR ||
                                    state->block_state == ISAL_BLOCK_HDR) {
                                        ret = read_header_stateful(state);
                                        if (ret)
                                                break;
                                }

                                if (state->block_state == ISAL_BLOCK_TYPE0)
                                        ret = decode_literal_block(state);
                                else
                                        ret = decode_huffman_code_block_stateless(state, start_out);
                                if (ret)
                                        break;
                        }
                }

                if (state->crc_flag)
                        update_checksum(state, start_out, state->next_out - start_out);

                if (state->block_state != ISAL_BLOCK_INPUT_DONE ||
                    state->copy_overflow_length + state->write_overflow_len + state->tmp_out_valid >
                            sizeof(state->tmp_out_buffer)) {
                        /* Save decompression history in tmp_out buffer */
                        if (state->tmp_out_valid == state->tmp_out_processed &&
                            avail_out - state->avail_out >= ISAL_DEF_HIST_SIZE) {
                                memcpy(state->tmp_out_buffer, state->next_out - ISAL_DEF_HIST_SIZE,
                                       ISAL_DEF_HIST_SIZE);
                                state->tmp_out_valid = ISAL_DEF_HIST_SIZE;
                                state->tmp_out_processed = ISAL_DEF_HIST_SIZE;

                        } else if (state->tmp_out_processed >= ISAL_DEF_HIST_SIZE) {
                                shift_size = state->tmp_out_valid - ISAL_DEF_HIST_SIZE;
                                if (shift_size > state->tmp_out_processed)
                                        shift_size = state->tmp_out_processed;

                                memmove(state->tmp_out_buffer, &state->tmp_out_buffer[shift_size],
                                        state->tmp_out_valid - shift_size);
                                state->tmp_out_valid -= shift_size;
                                state->tmp_out_processed -= shift_size;
                        }
                }

                /* Write overflow data into tmp buffer */
                if (state->write_overflow_len != 0) {
                        store_le_u32(&state->tmp_out_buffer[state->tmp_out_valid],
                                     state->write_overflow_lits);
                        state->tmp_out_valid += state->write_overflow_len;
                        state->total_out += state->write_overflow_len;
                        state->write_overflow_lits = 0;
                        state->write_overflow_len = 0;
                }

                if (state->copy_overflow_length != 0) {
                        byte_copy(&state->tmp_out_buffer[state->tmp_out_valid],
                                  state->copy_overflow_distance, state->copy_overflow_length);
                        state->tmp_out_valid += state->copy_overflow_length;
                        state->total_out += state->copy_overflow_length;
                        state->copy_overflow_distance = 0;
                        state->copy_overflow_length = 0;
                }

                if (ret == ISAL_INVALID_LOOKBACK || ret == ISAL_INVALID_BLOCK ||
                    ret == ISAL_INVALID_SYMBOL) {
                        state->total_out -= state->tmp_out_valid - state->tmp_out_processed;
                        return ret;
                }

                if (state->block_state == ISAL_BLOCK_INPUT_DONE &&
                    state->tmp_out_valid == state->tmp_out_processed) {
                        state->block_state = ISAL_BLOCK_FINISH;

                        switch (state->crc_flag) {
                        case ISAL_ZLIB:
                        case ISAL_ZLIB_NO_HDR_VER:
                                finalize_adler32(state);
                                ret = check_zlib_checksum(state);
                                break;

                        case ISAL_ZLIB_NO_HDR:
                                finalize_adler32(state);
                                break;

                        case ISAL_GZIP:
                        case ISAL_GZIP_NO_HDR_VER:
                                ret = check_gzip_checksum(state);
                                break;
                        }
                        /* Undo count stuff of bytes read into the read buffer */
                        state->next_in -= state->read_in_length / 8;
                        state->avail_in += state->read_in_length / 8;
                        state->read_in_length = 0;
                        state->read_in = 0;
                }
                state->total_out -= state->tmp_out_valid - state->tmp_out_processed;
        }

        return (ret > 0) ? ISAL_DECOMP_OK : ret;
}
