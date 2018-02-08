/**********************************************************************
  Copyright(c) 2011-2017 Intel Corporation All rights reserved.

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
#include "encode_df.h"
#include "igzip_level_buf_structs.h"

void isal_deflate_body_base(struct isal_zstream *stream);
void isal_deflate_finish_base(struct isal_zstream *stream);
void isal_deflate_icf_body_hash8k_base(struct isal_zstream *stream);
void isal_deflate_icf_body_hash_hist_base(struct isal_zstream *stream);
void icf_body_hash1_fillgreedy_lazy(struct isal_zstream *stream);
void isal_deflate_icf_finish_hash8k_base(struct isal_zstream *stream);
void isal_deflate_icf_finish_hash_hist_base(struct isal_zstream *stream);
void isal_deflate_icf_finish_hash_map_base(struct isal_zstream *stream);
void isal_update_histogram_base(uint8_t * start_stream, int length,
				struct isal_huff_histogram *histogram);
struct deflate_icf *encode_deflate_icf_base(struct deflate_icf *next_in,
					    struct deflate_icf *end_in, struct BitBuf2 *bb,
					    struct hufftables_icf *hufftables);
uint32_t crc32_gzip_base(uint32_t init_crc, const unsigned char *buf, uint64_t len);
uint32_t adler32_base(uint32_t init, const unsigned char *buf, uint64_t len);
int decode_huffman_code_block_stateless_base(struct inflate_state *s);

extern void isal_deflate_hash_base(uint16_t *, uint32_t, uint32_t, uint8_t *, uint32_t);

void set_long_icf_fg_base(uint8_t * next_in, uint8_t * end_in,
			  struct deflate_icf *match_lookup, struct level_buf *level_buf);
void gen_icf_map_h1_base(struct isal_zstream *stream,
			 struct deflate_icf *matches_icf_lookup, uint64_t input_size);

void isal_deflate_body(struct isal_zstream *stream)
{
	isal_deflate_body_base(stream);
}

void isal_deflate_finish(struct isal_zstream *stream)
{
	isal_deflate_finish_base(stream);
}

void isal_deflate_icf_body_lvl1(struct isal_zstream *stream)
{
	isal_deflate_icf_body_hash8k_base(stream);
}

void isal_deflate_icf_body_lvl2(struct isal_zstream *stream)
{
	isal_deflate_icf_body_hash_hist_base(stream);
}

void isal_deflate_icf_body_lvl3(struct isal_zstream *stream)
{
	icf_body_hash1_fillgreedy_lazy(stream);
}

void isal_deflate_icf_finish_lvl1(struct isal_zstream *stream)
{
	isal_deflate_icf_finish_hash8k_base(stream);
}

void isal_deflate_icf_finish_lvl2(struct isal_zstream *stream)
{
	isal_deflate_icf_finish_hash_hist_base(stream);
}

void isal_deflate_icf_finish_lvl3(struct isal_zstream *stream)
{
	isal_deflate_icf_finish_hash_map_base(stream);
}

void isal_update_histogram(uint8_t * start_stream, int length,
			   struct isal_huff_histogram *histogram)
{
	isal_update_histogram_base(start_stream, length, histogram);
}

struct deflate_icf *encode_deflate_icf(struct deflate_icf *next_in,
				       struct deflate_icf *end_in, struct BitBuf2 *bb,
				       struct hufftables_icf *hufftables)
{
	return encode_deflate_icf_base(next_in, end_in, bb, hufftables);
}

uint32_t crc32_gzip(uint32_t init_crc, const unsigned char *buf, uint64_t len)
{
	return crc32_gzip_base(init_crc, buf, len);
}

uint32_t isal_adler32(uint32_t init, const unsigned char *buf, uint64_t len)
{
	return adler32_base(init, buf, len);
}

int decode_huffman_code_block_stateless(struct inflate_state *s)
{
	return decode_huffman_code_block_stateless_base(s);
}

void isal_deflate_hash_lvl0(uint16_t * hash_table, uint32_t hash_mask,
			    uint32_t current_index, uint8_t * dict, uint32_t dict_len)
{
	isal_deflate_hash_base(hash_table, hash_mask, current_index, dict, dict_len);
}

void isal_deflate_hash_lvl1(uint16_t * hash_table, uint32_t hash_mask,
			    uint32_t current_index, uint8_t * dict, uint32_t dict_len)
{
	isal_deflate_hash_base(hash_table, hash_mask, current_index, dict, dict_len);
}

void isal_deflate_hash_lvl2(uint16_t * hash_table, uint32_t hash_mask,
			    uint32_t current_index, uint8_t * dict, uint32_t dict_len)
{
	isal_deflate_hash_base(hash_table, hash_mask, current_index, dict, dict_len);
}

void isal_deflate_hash_lvl3(uint16_t * hash_table, uint32_t hash_mask,
			    uint32_t current_index, uint8_t * dict, uint32_t dict_len)
{
	isal_deflate_hash_base(hash_table, hash_mask, current_index, dict, dict_len);
}

void set_long_icf_fg(uint8_t * next_in, uint8_t * end_in,
		     struct deflate_icf *match_lookup, struct level_buf *level_buf)
{
	set_long_icf_fg_base(next_in, end_in, match_lookup, level_buf);
}

void gen_icf_map_lh1(struct isal_zstream *stream,
		     struct deflate_icf *matches_icf_lookup, uint64_t input_size)
{
	gen_icf_map_h1_base(stream, matches_icf_lookup, input_size);
}
