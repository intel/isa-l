#ifndef IGZIP_LEVEL_BUF_STRUCTS_H
#define IGZIP_LEVEL_BUF_STRUCTS_H

#include "huff_codes.h"
#include "encode_df.h"

struct level_2_buf {
	struct hufftables_icf encode_tables;
	uint32_t deflate_hdr_count;
	uint32_t deflate_hdr_extra_bits;
	uint8_t deflate_hdr[ISAL_DEF_MAX_HDR_SIZE];
	struct deflate_icf *icf_buf_next;
	uint64_t icf_buf_avail_out;
	struct deflate_icf icf_buf_start[];
};
#endif
