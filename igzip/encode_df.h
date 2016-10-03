#ifndef ENCODE_DF_H
#define ENCODE_DF_H

#include <stdint.h>
#include "huff_codes.h"

/* Deflate Intermediate Compression Format */
#define ICF_DIST_OFFSET 14
#define NULL_DIST_SYM 30

struct deflate_icf {
	uint32_t lit_len:ICF_DIST_OFFSET;
	uint32_t lit_dist:5;
	uint32_t dist_extra:32 - 5 - ICF_DIST_OFFSET;
};

struct deflate_icf *encode_deflate_icf(struct deflate_icf *next_in, struct deflate_icf *end_in,
			               struct BitBuf2 *bb, struct hufftables_icf * hufftables);
#endif
