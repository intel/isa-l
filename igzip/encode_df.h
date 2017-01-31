#ifndef ENCODE_DF_H
#define ENCODE_DF_H

#include <stdint.h>
#include "huff_codes.h"

/* Deflate Intermediate Compression Format */
#define LIT_LEN_BIT_COUNT 10
#define DIST_LIT_BIT_COUNT 9
#define ICF_DIST_OFFSET LIT_LEN_BIT_COUNT
#define NULL_DIST_SYM 30

struct deflate_icf {
	uint32_t lit_len:LIT_LEN_BIT_COUNT;
	uint32_t lit_dist:DIST_LIT_BIT_COUNT;
	uint32_t dist_extra:32 - DIST_LIT_BIT_COUNT - ICF_DIST_OFFSET;
};

struct deflate_icf *encode_deflate_icf(struct deflate_icf *next_in, struct deflate_icf *end_in,
			               struct BitBuf2 *bb, struct hufftables_icf * hufftables);
#endif
