#include <stdint.h>
#include "igzip_lib.h"
#include "../huffman.h"
#include "../huff_codes.h"
#include "../encode_df.h"
#include "../igzip_level_buf_structs.h"
#include "unaligned.h"

extern uint32_t
compare_rvv(const uint8_t *str1, const uint8_t *str2, uint32_t max_length);

/* Replicated verbatim from igzip_icf_body.c (static inline there). */
static inline void
write_deflate_icf(struct deflate_icf *icf, uint32_t lit_len, uint32_t lit_dist, uint32_t extra_bits)
{
        store_native_u32((uint8_t *) icf,
                         lit_len | (lit_dist << LIT_LEN_BIT_COUNT) |
                                 (extra_bits << (LIT_LEN_BIT_COUNT + DIST_LIT_BIT_COUNT)));
}

void
set_long_icf_fg_rvv(uint8_t *next_in, uint64_t processed, uint64_t input_size,
                    struct deflate_icf *match_lookup)
{
        uint8_t *end_processed = next_in + processed;
        uint8_t *end_in = next_in + input_size;
        uint32_t dist_code, dist_extra, dist, len;
        uint32_t match_len;
        uint32_t dist_start[] = { 0x0001, 0x0002, 0x0003, 0x0004, 0x0005, 0x0007, 0x0009, 0x000d,
                                  0x0011, 0x0019, 0x0021, 0x0031, 0x0041, 0x0061, 0x0081, 0x00c1,
                                  0x0101, 0x0181, 0x0201, 0x0301, 0x0401, 0x0601, 0x0801, 0x0c01,
                                  0x1001, 0x1801, 0x2001, 0x3001, 0x4001, 0x6001, 0x0000, 0x0000 };

        if (end_in > end_processed + ISAL_LOOK_AHEAD)
                end_in = end_processed + ISAL_LOOK_AHEAD;

        while (next_in < end_processed) {
                dist_code = match_lookup->lit_dist;
                dist_extra = match_lookup->dist_extra;
                dist = dist_start[dist_code] + dist_extra;
                len = match_lookup->lit_len;
                if (len >= 8 + LEN_OFFSET) {
                        /* RVV-accelerated longest-common-prefix match (uncapped). */
                        match_len = compare_rvv((next_in + 8) - dist, next_in + 8,
                                                (uint32_t) (end_in - (next_in + 8))) +
                                    LEN_OFFSET + 8;

                        while (match_len > match_lookup->lit_len &&
                               match_len >= LEN_OFFSET + SHORTEST_MATCH) {
                                write_deflate_icf(match_lookup,
                                                  match_len > LEN_MAX ? LEN_MAX : match_len,
                                                  dist_code, dist_extra);
                                match_lookup++;
                                next_in++;
                                match_len--;
                        }
                }

                match_lookup++;
                next_in++;
        }
}
