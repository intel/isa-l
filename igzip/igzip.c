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

#define ASM

#include <assert.h>
#include <string.h>
#ifdef _WIN32
# include <intrin.h>
#endif

#define MAX_WRITE_BITS_SIZE 8
#define FORCE_FLUSH 64
#define MIN_OBUF_SIZE 224
#define NON_EMPTY_BLOCK_SIZE 6
#define MAX_SYNC_FLUSH_SIZE NON_EMPTY_BLOCK_SIZE + MAX_WRITE_BITS_SIZE

#define MAX_TOKENS (16 * 1024)

#include "huffman.h"
#include "bitbuf2.h"
#include "igzip_lib.h"
#include "repeated_char_result.h"
#include "huff_codes.h"
#include "encode_df.h"
#include "igzip_level_buf_structs.h"

extern const uint8_t gzip_hdr[];
extern const uint32_t gzip_hdr_bytes;
extern const uint32_t gzip_trl_bytes;
extern const struct isal_hufftables hufftables_default;
extern const struct isal_hufftables hufftables_static;

extern uint32_t crc32_gzip(uint32_t init_crc, const unsigned char *buf, uint64_t len);

static int write_stored_block_stateless(struct isal_zstream *stream, uint32_t stored_len,
					uint32_t crc32);

static int write_gzip_header_stateless(struct isal_zstream *stream);
static void write_gzip_header(struct isal_zstream *stream);
static int write_deflate_header_stateless(struct isal_zstream *stream);
static int write_deflate_header_unaligned_stateless(struct isal_zstream *stream);

unsigned int detect_repeated_char(uint8_t * buf, uint32_t size);

#define STORED_BLK_HDR_BZ 5
#define STORED_BLK_MAX_BZ 65535

void isal_deflate_body(struct isal_zstream *stream);
void isal_deflate_finish(struct isal_zstream *stream);

void isal_deflate_icf_body(struct isal_zstream *stream);
void isal_deflate_icf_finish(struct isal_zstream *stream);
/*****************************************************************/

/* Forward declarations */
static inline void reset_match_history(struct isal_zstream *stream);
void write_header(struct isal_zstream *stream, uint8_t * deflate_hdr,
		  uint32_t deflate_hdr_count, uint32_t extra_bits_count, uint32_t next_state,
		  uint32_t toggle_end_of_stream);
void write_deflate_header(struct isal_zstream *stream);
void write_trailer(struct isal_zstream *stream);

struct slver {
	uint16_t snum;
	uint8_t ver;
	uint8_t core;
};

/* Version info */
struct slver isal_deflate_init_slver_01030081;
struct slver isal_deflate_init_slver = { 0x0081, 0x03, 0x01 };

struct slver isal_deflate_stateless_init_slver_00010084;
struct slver isal_deflate_stateless_init_slver = { 0x0084, 0x01, 0x00 };

struct slver isal_deflate_slver_01030082;
struct slver isal_deflate_slver = { 0x0082, 0x03, 0x01 };

struct slver isal_deflate_stateless_slver_01010083;
struct slver isal_deflate_stateless_slver = { 0x0083, 0x01, 0x01 };

struct slver isal_deflate_set_hufftables_slver_00_01_008b;
struct slver isal_deflate_set_hufftables_slver = { 0x008b, 0x01, 0x00 };

/*****************************************************************/
static
void sync_flush(struct isal_zstream *stream)
{
	struct isal_zstate *state = &stream->internal_state;
	uint64_t bits_to_write = 0xFFFF0000, bits_len;
	uint64_t bytes;
	int flush_size;

	if (stream->avail_out >= 8) {
		set_buf(&state->bitbuf, stream->next_out, stream->avail_out);

		flush_size = (-(state->bitbuf.m_bit_count + 3)) % 8;

		bits_to_write <<= flush_size + 3;
		bits_len = 32 + flush_size + 3;

#ifdef USE_BITBUFB		/* Write Bits Always */
		state->state = ZSTATE_NEW_HDR;
#else /* Not Write Bits Always */
		state->state = ZSTATE_FLUSH_WRITE_BUFFER;
#endif
		state->has_eob = 0;

		write_bits(&state->bitbuf, bits_to_write, bits_len);

		bytes = buffer_used(&state->bitbuf);
		stream->next_out = buffer_ptr(&state->bitbuf);
		stream->avail_out -= bytes;
		stream->total_out += bytes;

		if (stream->flush == FULL_FLUSH) {
			/* Clear match history so there are no cross
			 * block length distance pairs */
			state->file_start -= state->b_bytes_processed;
			state->b_bytes_valid -= state->b_bytes_processed;
			state->b_bytes_processed = 0;
			reset_match_history(stream);
		}
	}
}

static void flush_write_buffer(struct isal_zstream *stream)
{
	struct isal_zstate *state = &stream->internal_state;
	int bytes = 0;
	if (stream->avail_out >= 8) {
		set_buf(&state->bitbuf, stream->next_out, stream->avail_out);
		flush(&state->bitbuf);
		stream->next_out = buffer_ptr(&state->bitbuf);
		bytes = buffer_used(&state->bitbuf);
		stream->avail_out -= bytes;
		stream->total_out += bytes;
		state->state = ZSTATE_NEW_HDR;
	}
}

static void flush_icf_block(struct isal_zstream *stream)
{
	struct isal_zstate *state = &stream->internal_state;
	struct level_2_buf *level_buf = (struct level_2_buf *)stream->level_buf;
	struct BitBuf2 *write_buf = &state->bitbuf;
	struct deflate_icf *icf_buf_encoded_next;

	set_buf(write_buf, stream->next_out, stream->avail_out);

#if defined (USE_BITBUF8) || (USE_BITBUF_ELSE)
	if (!is_full(write_buf))
		flush_bits(write_buf);
#endif

	icf_buf_encoded_next = encode_deflate_icf(level_buf->icf_buf_start + state->count,
						  level_buf->icf_buf_next, write_buf,
						  &level_buf->encode_tables);

	state->count = icf_buf_encoded_next - level_buf->icf_buf_start;
	stream->next_out = buffer_ptr(write_buf);
	stream->total_out += buffer_used(write_buf);
	stream->avail_out -= buffer_used(write_buf);

	if (level_buf->icf_buf_next <= icf_buf_encoded_next) {
		state->count = 0;
		if (stream->avail_in == 0 && stream->end_of_stream)
			state->state = ZSTATE_TRL;
		else if (stream->avail_in == 0 && stream->flush != NO_FLUSH)
			state->state = ZSTATE_SYNC_FLUSH;
		else
			state->state = ZSTATE_NEW_HDR;
	}
}

static void init_new_icf_block(struct isal_zstream *stream)
{
	struct isal_zstate *state = &stream->internal_state;
	struct level_2_buf *level_buf = (struct level_2_buf *)stream->level_buf;

	if (stream->level_buf_size >=
	    sizeof(struct level_2_buf) + 100 * sizeof(struct deflate_icf)) {
		level_buf->icf_buf_next = level_buf->icf_buf_start;
		level_buf->icf_buf_avail_out =
		    stream->level_buf_size - sizeof(struct level_2_buf) -
		    sizeof(struct deflate_icf);
		memset(&state->hist, 0, sizeof(struct isal_mod_hist));
		state->state = ZSTATE_BODY;
	}
}

static void create_icf_block_hdr(struct isal_zstream *stream)
{
	struct isal_zstate *state = &stream->internal_state;
	struct level_2_buf *level_buf = (struct level_2_buf *)stream->level_buf;
	struct BitBuf2 *write_buf = &state->bitbuf;
	struct BitBuf2 write_buf_tmp;
	uint32_t out_size = stream->avail_out;
	uint8_t *end_out = stream->next_out + out_size;
	/* Write EOB in icf_buf */
	state->hist.ll_hist[256] = 1;
	level_buf->icf_buf_next->lit_len = 0x100;
	level_buf->icf_buf_next->lit_dist = NULL_DIST_SYM;
	level_buf->icf_buf_next->dist_extra = 0;
	level_buf->icf_buf_next++;

	state->has_eob_hdr = stream->end_of_stream && !stream->avail_in;
	if (end_out - stream->next_out >= ISAL_DEF_MAX_HDR_SIZE) {
		/* Determine whether this is the final block */

		if (stream->gzip_flag == IGZIP_GZIP)
			write_gzip_header_stateless(stream);

		set_buf(write_buf, stream->next_out, stream->avail_out);

		create_hufftables_icf(write_buf, &level_buf->encode_tables, &state->hist,
				      state->has_eob_hdr);
		state->state = ZSTATE_FLUSH_ICF_BUFFER;
		stream->next_out = buffer_ptr(write_buf);
		stream->total_out += buffer_used(write_buf);
		stream->avail_out -= buffer_used(write_buf);
	} else {
		/* Start writing into temporary buffer */
		write_buf_tmp.m_bits = write_buf->m_bits;
		write_buf_tmp.m_bit_count = write_buf->m_bit_count;

		write_buf->m_bits = 0;
		write_buf->m_bit_count = 0;

		set_buf(&write_buf_tmp, level_buf->deflate_hdr, ISAL_DEF_MAX_HDR_SIZE);

		create_hufftables_icf(&write_buf_tmp, &level_buf->encode_tables,
				      &state->hist, state->has_eob_hdr);

		level_buf->deflate_hdr_count = buffer_used(&write_buf_tmp);
		level_buf->deflate_hdr_extra_bits = write_buf_tmp.m_bit_count;
		flush(&write_buf_tmp);

		state->state = ZSTATE_HDR;
	}
}

static void isal_deflate_pass(struct isal_zstream *stream)
{
	struct isal_zstate *state = &stream->internal_state;
	struct isal_hufftables *hufftables = stream->hufftables;
	uint8_t *start_in = stream->next_in;

	if (state->state == ZSTATE_NEW_HDR || state->state == ZSTATE_HDR) {
		if (state->count == 0)
			/* Assume the final header is being written since the header
			 * stored in hufftables is the final header. */
			state->has_eob_hdr = 1;
		write_header(stream, hufftables->deflate_hdr, hufftables->deflate_hdr_count,
			     hufftables->deflate_hdr_extra_bits, ZSTATE_BODY,
			     !stream->end_of_stream);
	}

	if (state->state == ZSTATE_BODY)
		isal_deflate_body(stream);

	if (state->state == ZSTATE_FLUSH_READ_BUFFER)
		isal_deflate_finish(stream);

	if (state->state == ZSTATE_SYNC_FLUSH)
		sync_flush(stream);

	if (state->state == ZSTATE_FLUSH_WRITE_BUFFER)
		flush_write_buffer(stream);

	if (stream->gzip_flag)
		state->crc = crc32_gzip(state->crc, start_in, stream->next_in - start_in);

	if (state->state == ZSTATE_TRL)
		write_trailer(stream);
}

static void isal_deflate_icf_pass(struct isal_zstream *stream)
{
	uint8_t *start_in = stream->next_in;
	struct isal_zstate *state = &stream->internal_state;
	struct level_2_buf *level_buf = (struct level_2_buf *)stream->level_buf;

	do {
		if (state->state == ZSTATE_NEW_HDR)
			init_new_icf_block(stream);

		if (state->state == ZSTATE_BODY)
			isal_deflate_icf_body(stream);

		if (state->state == ZSTATE_FLUSH_READ_BUFFER)
			isal_deflate_icf_finish(stream);

		if (state->state == ZSTATE_CREATE_HDR)
			create_icf_block_hdr(stream);

		if (state->state == ZSTATE_HDR)
			/* Note that the header may be prepended by the
			 * remaining bits in the previous block, as such the
			 * toggle header flag cannot be used */
			write_header(stream, level_buf->deflate_hdr,
				     level_buf->deflate_hdr_count,
				     level_buf->deflate_hdr_extra_bits,
				     ZSTATE_FLUSH_ICF_BUFFER, 0);

		if (state->state == ZSTATE_FLUSH_ICF_BUFFER)
			flush_icf_block(stream);

	} while (state->state == ZSTATE_NEW_HDR);

	if (state->state == ZSTATE_SYNC_FLUSH)
		sync_flush(stream);

	if (state->state == ZSTATE_FLUSH_WRITE_BUFFER)
		flush_write_buffer(stream);

	if (stream->gzip_flag)
		state->crc = crc32_gzip(state->crc, start_in, stream->next_in - start_in);

	if (state->state == ZSTATE_TRL)
		write_trailer(stream);
}

static void isal_deflate_int(struct isal_zstream *stream)
{
	struct isal_zstate *state = &stream->internal_state;
	uint32_t size;

	/* Move data from temporary output buffer to output buffer */
	if (state->state >= ZSTATE_TMP_OFFSET) {
		size = state->tmp_out_end - state->tmp_out_start;
		if (size > stream->avail_out)
			size = stream->avail_out;
		memcpy(stream->next_out, state->tmp_out_buff + state->tmp_out_start, size);
		stream->next_out += size;
		stream->avail_out -= size;
		stream->total_out += size;
		state->tmp_out_start += size;

		if (state->tmp_out_start == state->tmp_out_end)
			state->state -= ZSTATE_TMP_OFFSET;

		if (stream->avail_out == 0 || state->state == ZSTATE_END
		    // or do not write out empty blocks since the outbuffer was processed
		    || (state->state == ZSTATE_NEW_HDR && stream->avail_out == 0))
			return;
	}
	assert(state->tmp_out_start == state->tmp_out_end);

	if (stream->level == 0)
		isal_deflate_pass(stream);
	else
		isal_deflate_icf_pass(stream);

	/* Fill temporary output buffer then complete filling output buffer */
	if (stream->avail_out > 0 && stream->avail_out < 8 && state->state != ZSTATE_NEW_HDR) {
		uint8_t *next_out;
		uint32_t avail_out;
		uint32_t total_out;

		next_out = stream->next_out;
		avail_out = stream->avail_out;
		total_out = stream->total_out;

		stream->next_out = state->tmp_out_buff;
		stream->avail_out = sizeof(state->tmp_out_buff);
		stream->total_out = 0;

		if (stream->level == 0)
			isal_deflate_pass(stream);
		else
			isal_deflate_icf_pass(stream);

		state->tmp_out_start = 0;
		state->tmp_out_end = stream->total_out;

		stream->next_out = next_out;
		stream->avail_out = avail_out;
		stream->total_out = total_out;
		if (state->tmp_out_end) {
			size = state->tmp_out_end;
			if (size > stream->avail_out)
				size = stream->avail_out;
			memcpy(stream->next_out, state->tmp_out_buff, size);
			stream->next_out += size;
			stream->avail_out -= size;
			stream->total_out += size;
			state->tmp_out_start += size;
			if (state->tmp_out_start != state->tmp_out_end)
				state->state += ZSTATE_TMP_OFFSET;

		}
	}

}

static void write_constant_compressed_stateless(struct isal_zstream *stream,
						uint32_t repeated_length)
{
	/* Assumes repeated_length is at least 1.
	 * Assumes the input end_of_stream is either 0 or 1. */
	struct isal_zstate *state = &stream->internal_state;
	uint32_t rep_bits = ((repeated_length - 1) / 258) * 2;
	uint32_t rep_bytes = rep_bits / 8;
	uint32_t rep_extra = (repeated_length - 1) % 258;
	uint32_t bytes;
	uint32_t repeated_char = *stream->next_in;
	uint8_t *start_in = stream->next_in;

	/* Guarantee there is enough space for the header even in the worst case */
	if (stream->avail_out < HEADER_LENGTH + MAX_FIXUP_CODE_LENGTH + rep_bytes + 8)
		return;

	/* Assumes the repeated char is either 0 or 0xFF. */
	memcpy(stream->next_out, repeated_char_header[repeated_char & 1], HEADER_LENGTH);

	if (stream->avail_in == repeated_length && stream->end_of_stream > 0) {
		stream->next_out[0] |= 1;
		state->has_eob_hdr = 1;
		state->has_eob = 1;
		state->state = ZSTATE_TRL;
	} else {
		state->state = ZSTATE_NEW_HDR;
	}

	memset(stream->next_out + HEADER_LENGTH, 0, rep_bytes);
	stream->avail_out -= HEADER_LENGTH + rep_bytes;
	stream->next_out += HEADER_LENGTH + rep_bytes;
	stream->total_out += HEADER_LENGTH + rep_bytes;

	set_buf(&state->bitbuf, stream->next_out, stream->avail_out);

	/* These two lines are basically a modified version of init. */
	state->bitbuf.m_bits = 0;
	state->bitbuf.m_bit_count = rep_bits % 8;

	/* Add smaller repeat codes as necessary. Code280 can describe repeat
	 * lengths of 115-130 bits. Code10 can describe repeat lengths of 10
	 * bits. If more than 230 bits, fill code with two code280s. Else if
	 * more than 115 repeates, fill with code10s until one code280 can
	 * finish the rest of the repeats. Else, fill with code10s and
	 * literals */
	if (rep_extra > 115) {
		while (rep_extra > 130 && rep_extra < 230) {
			write_bits(&state->bitbuf, CODE_10, CODE_10_LENGTH);
			rep_extra -= 10;
		}

		if (rep_extra >= 230) {
			write_bits(&state->bitbuf,
				   CODE_280 | ((rep_extra / 2 - 115) <<
					       CODE_280_LENGTH), CODE_280_TOTAL_LENGTH);
			rep_extra -= rep_extra / 2;
		}

		write_bits(&state->bitbuf,
			   CODE_280 | ((rep_extra - 115) << CODE_280_LENGTH),
			   CODE_280_TOTAL_LENGTH);

	} else {
		while (rep_extra >= 10) {

			write_bits(&state->bitbuf, CODE_10, CODE_10_LENGTH);
			rep_extra -= 10;
		}

		for (; rep_extra > 0; rep_extra--)
			write_bits(&state->bitbuf, CODE_LIT, CODE_LIT_LENGTH);
	}

	write_bits(&state->bitbuf, END_OF_BLOCK, END_OF_BLOCK_LEN);

	stream->next_in += repeated_length;
	stream->avail_in -= repeated_length;
	stream->total_in += repeated_length;

	bytes = buffer_used(&state->bitbuf);
	stream->next_out = buffer_ptr(&state->bitbuf);
	stream->avail_out -= bytes;
	stream->total_out += bytes;

	if (stream->gzip_flag)
		state->crc = crc32_gzip(state->crc, start_in, stream->next_in - start_in);

	return;
}

int detect_repeated_char_length(uint8_t * in, uint32_t length)
{
	/* This currently assumes the first 8 bytes are the same character.
	 * This won't work effectively if the input stream isn't aligned well. */
	uint8_t *p_8, *end = in + length;
	uint64_t *p_64 = (uint64_t *) in;
	uint64_t w = *p_64;
	uint8_t c = (uint8_t) w;

	for (; (p_64 <= (uint64_t *) (end - 8)) && (w == *p_64); p_64++) ;

	p_8 = (uint8_t *) p_64;

	for (; (p_8 < end) && (c == *p_8); p_8++) ;

	return p_8 - in;
}

static int isal_deflate_int_stateless(struct isal_zstream *stream)
{
	uint32_t repeat_length;
	struct isal_zstate *state = &stream->internal_state;

	if (stream->gzip_flag == IGZIP_GZIP)
		if (write_gzip_header_stateless(stream))
			return STATELESS_OVERFLOW;

	if (stream->avail_in >= 8
	    && (*(uint64_t *) stream->next_in == 0
		|| *(uint64_t *) stream->next_in == ~(uint64_t) 0)) {
		repeat_length = detect_repeated_char_length(stream->next_in, stream->avail_in);

		if (stream->avail_in == repeat_length || repeat_length >= MIN_REPEAT_LEN)
			write_constant_compressed_stateless(stream, repeat_length);
	}

	if (stream->level == 0) {
		if (state->state == ZSTATE_NEW_HDR || state->state == ZSTATE_HDR) {
			write_deflate_header_unaligned_stateless(stream);
			if (state->state == ZSTATE_NEW_HDR || state->state == ZSTATE_HDR)
				return STATELESS_OVERFLOW;

			reset_match_history(stream);
		}

		state->file_start = stream->next_in - stream->total_in;
		isal_deflate_pass(stream);

	} else if (stream->level == 1) {
		if (stream->level_buf == NULL || stream->level_buf_size < ISAL_DEF_LVL1_MIN) {
			/* Default to internal buffer if invalid size is supplied */
			stream->level_buf = state->buffer;
			stream->level_buf_size = sizeof(state->buffer);
		}

		if (state->state == ZSTATE_NEW_HDR || state->state == ZSTATE_HDR)
			reset_match_history(stream);

		state->count = 0;
		state->file_start = stream->next_in - stream->total_in;
		isal_deflate_icf_pass(stream);

	} else
		return ISAL_INVALID_LEVEL;

	if (state->state == ZSTATE_END
	    || (state->state == ZSTATE_NEW_HDR && stream->flush == FULL_FLUSH))
		return COMP_OK;
	else
		return STATELESS_OVERFLOW;
}

static int write_stored_block_stateless(struct isal_zstream *stream,
					uint32_t stored_len, uint32_t crc32)
{
	uint64_t stored_blk_hdr;
	uint32_t copy_size;
	uint32_t avail_in;
	uint64_t gzip_trl;

	if (stream->avail_out < stored_len)
		return STATELESS_OVERFLOW;

	stream->avail_out -= stored_len;
	stream->total_out += stored_len;
	avail_in = stream->avail_in;

	if (stream->gzip_flag == IGZIP_GZIP) {
		memcpy(stream->next_out, gzip_hdr, gzip_hdr_bytes);
		stream->next_out += gzip_hdr_bytes;
		stream->gzip_flag = IGZIP_GZIP_NO_HDR;
	}

	do {
		if (avail_in >= STORED_BLK_MAX_BZ) {
			stored_blk_hdr = 0xFFFF00;
			copy_size = STORED_BLK_MAX_BZ;
		} else {
			stored_blk_hdr = ~avail_in;
			stored_blk_hdr <<= 24;
			stored_blk_hdr |= (avail_in & 0xFFFF) << 8;
			copy_size = avail_in;
		}

		avail_in -= copy_size;

		/* Handle BFINAL bit */
		if (avail_in == 0) {
			if (stream->flush == NO_FLUSH || stream->end_of_stream) {
				stored_blk_hdr |= 0x1;
				stream->internal_state.has_eob_hdr = 1;
			}
		}
		memcpy(stream->next_out, &stored_blk_hdr, STORED_BLK_HDR_BZ);
		stream->next_out += STORED_BLK_HDR_BZ;

		memcpy(stream->next_out, stream->next_in, copy_size);
		stream->next_out += copy_size;
		stream->next_in += copy_size;
		stream->total_in += copy_size;
	} while (avail_in != 0);

	if (stream->gzip_flag && stream->internal_state.has_eob_hdr) {
		gzip_trl = stream->avail_in;
		gzip_trl <<= 32;
		gzip_trl |= crc32 & 0xFFFFFFFF;
		memcpy(stream->next_out, &gzip_trl, gzip_trl_bytes);
		stream->next_out += gzip_trl_bytes;
	}

	stream->avail_in = 0;
	return COMP_OK;
}

static inline void reset_match_history(struct isal_zstream *stream)
{
	struct isal_zstate *state = &stream->internal_state;
	uint16_t *head = stream->internal_state.head;
	int i = 0;

	state->has_hist = 0;

	if (stream->total_in == 0)
		memset(stream->internal_state.head, 0, sizeof(stream->internal_state.head));
	else {
		for (i = 0; i < sizeof(state->head) / 2; i++) {
			head[i] = (uint16_t) (stream->total_in);
		}
	}
}

void isal_deflate_init(struct isal_zstream *stream)
{
	struct isal_zstate *state = &stream->internal_state;

	stream->total_in = 0;
	stream->total_out = 0;
	stream->hufftables = (struct isal_hufftables *)&hufftables_default;
	stream->level = 0;
	stream->level_buf = NULL;
	stream->level_buf_size = 0;
	stream->end_of_stream = 0;
	stream->flush = NO_FLUSH;
	stream->gzip_flag = 0;

	state->b_bytes_valid = 0;
	state->b_bytes_processed = 0;
	state->has_eob = 0;
	state->has_eob_hdr = 0;
	state->has_hist = 0;
	state->state = ZSTATE_NEW_HDR;
	state->count = 0;

	state->tmp_out_start = 0;
	state->tmp_out_end = 0;

	state->file_start = stream->next_in;

	init(&state->bitbuf);

	state->crc = 0;

	memset(state->head, 0, sizeof(state->head));

	return;
}

int isal_deflate_set_hufftables(struct isal_zstream *stream,
				struct isal_hufftables *hufftables, int type)
{
	if (stream->internal_state.state != ZSTATE_NEW_HDR)
		return ISAL_INVALID_OPERATION;

	switch (type) {
	case IGZIP_HUFFTABLE_DEFAULT:
		stream->hufftables = (struct isal_hufftables *)&hufftables_default;
		break;
	case IGZIP_HUFFTABLE_STATIC:
		stream->hufftables = (struct isal_hufftables *)&hufftables_static;
		break;
	case IGZIP_HUFFTABLE_CUSTOM:
		if (hufftables != NULL) {
			stream->hufftables = hufftables;
			break;
		}
	default:
		return ISAL_INVALID_OPERATION;
	}

	return COMP_OK;
}

void isal_deflate_stateless_init(struct isal_zstream *stream)
{
	stream->total_in = 0;
	stream->total_out = 0;
	stream->hufftables = (struct isal_hufftables *)&hufftables_default;
	stream->level = 0;
	stream->level_buf = NULL;
	stream->level_buf_size = 0;
	stream->end_of_stream = 0;
	stream->flush = NO_FLUSH;
	stream->gzip_flag = 0;
	stream->internal_state.state = ZSTATE_NEW_HDR;
	return;
}

int isal_deflate_stateless(struct isal_zstream *stream)
{
	uint8_t *next_in = stream->next_in;
	const uint32_t avail_in = stream->avail_in;
	const uint32_t total_in = stream->total_in;

	uint8_t *next_out = stream->next_out;
	const uint32_t avail_out = stream->avail_out;
	const uint32_t total_out = stream->total_out;
	const uint32_t gzip_flag = stream->gzip_flag;

	uint32_t crc32 = 0;
	uint32_t stored_len;

	/* Final block has already been written */
	stream->internal_state.has_eob_hdr = 0;
	init(&stream->internal_state.bitbuf);
	stream->internal_state.state = ZSTATE_NEW_HDR;
	stream->internal_state.crc = 0;

	if (stream->flush == NO_FLUSH)
		stream->end_of_stream = 1;

	if (stream->flush != NO_FLUSH && stream->flush != FULL_FLUSH)
		return INVALID_FLUSH;

	if (stream->level != 0 && stream->level != 1)
		return ISAL_INVALID_LEVEL;

	if (avail_in == 0)
		stored_len = STORED_BLK_HDR_BZ;
	else
		stored_len =
		    STORED_BLK_HDR_BZ * ((avail_in + STORED_BLK_MAX_BZ - 1) /
					 STORED_BLK_MAX_BZ) + avail_in;

	/*
	   at least 1 byte compressed data in the case of empty dynamic block which only
	   contains the EOB
	 */

	if (stream->gzip_flag == IGZIP_GZIP)
		stored_len += gzip_hdr_bytes + gzip_trl_bytes;

	else if (stream->gzip_flag == IGZIP_GZIP_NO_HDR)
		stored_len += gzip_trl_bytes;

	/*
	   the output buffer should be no less than 8 bytes
	   while empty stored deflate block is 5 bytes only
	 */
	if (stream->avail_out < 8)
		return STATELESS_OVERFLOW;

	if (isal_deflate_int_stateless(stream) == COMP_OK)
		return COMP_OK;
	else {
		if (stream->flush == FULL_FLUSH) {
			stream->internal_state.file_start =
			    (uint8_t *) & stream->internal_state.buffer;
			reset_match_history(stream);
		}
		stream->internal_state.has_eob_hdr = 0;
	}

	if (avail_out < stored_len)
		return STATELESS_OVERFLOW;

	stream->next_in = next_in;
	stream->avail_in = avail_in;
	stream->total_in = total_in;

	stream->next_out = next_out;
	stream->avail_out = avail_out;
	stream->total_out = total_out;

	stream->gzip_flag = gzip_flag;

	if (stream->gzip_flag)
		crc32 = crc32_gzip(0x0, next_in, avail_in);

	return write_stored_block_stateless(stream, stored_len, crc32);
}

int isal_deflate(struct isal_zstream *stream)
{
	struct isal_zstate *state = &stream->internal_state;
	int ret = COMP_OK;
	uint8_t *next_in;
	uint32_t avail_in, avail_in_start;
	uint32_t flush_type = stream->flush;
	uint32_t end_of_stream = stream->end_of_stream;
	int size = 0;
	uint8_t *copy_down_src = NULL;
	uint64_t copy_down_size = 0;
	int32_t processed = -(state->b_bytes_valid - state->b_bytes_processed);

	if (stream->flush >= 3)
		return INVALID_FLUSH;

	next_in = stream->next_in;
	avail_in = stream->avail_in;
	stream->total_in -= state->b_bytes_valid - state->b_bytes_processed;

	do {
		size = avail_in;
		if (size > sizeof(state->buffer) - state->b_bytes_valid) {
			size = sizeof(state->buffer) - state->b_bytes_valid;
			stream->flush = NO_FLUSH;
			stream->end_of_stream = 0;
		}
		memcpy(&state->buffer[state->b_bytes_valid], next_in, size);

		next_in += size;
		avail_in -= size;
		state->b_bytes_valid += size;

		stream->next_in = &state->buffer[state->b_bytes_processed];
		stream->avail_in = state->b_bytes_valid - state->b_bytes_processed;
		state->file_start = stream->next_in - stream->total_in;
		processed += stream->avail_in;

		if (stream->avail_in > IGZIP_HIST_SIZE
		    || stream->end_of_stream || stream->flush != NO_FLUSH) {
			avail_in_start = stream->avail_in;
			isal_deflate_int(stream);
			state->b_bytes_processed += avail_in_start - stream->avail_in;

			if (state->b_bytes_processed > IGZIP_HIST_SIZE) {
				copy_down_src =
				    &state->buffer[state->b_bytes_processed - IGZIP_HIST_SIZE];
				copy_down_size =
				    state->b_bytes_valid - state->b_bytes_processed +
				    IGZIP_HIST_SIZE;
				memmove(state->buffer, copy_down_src, copy_down_size);

				state->b_bytes_valid -= copy_down_src - state->buffer;
				state->b_bytes_processed -= copy_down_src - state->buffer;
			}

		}

		stream->flush = flush_type;
		stream->end_of_stream = end_of_stream;
		processed -= stream->avail_in;
	} while (processed < IGZIP_HIST_SIZE + ISAL_LOOK_AHEAD && avail_in > 0
		 && stream->avail_out > 0);

	if (processed >= IGZIP_HIST_SIZE + ISAL_LOOK_AHEAD) {
		stream->next_in = next_in - stream->avail_in;
		stream->avail_in = avail_in + stream->avail_in;

		state->file_start = stream->next_in - stream->total_in;

		if (stream->avail_in > 0 && stream->avail_out > 0)
			isal_deflate_int(stream);

		size = stream->avail_in;
		if (stream->avail_in > IGZIP_HIST_SIZE)
			size = 0;

		memmove(state->buffer, stream->next_in - IGZIP_HIST_SIZE,
			size + IGZIP_HIST_SIZE);
		state->b_bytes_processed = IGZIP_HIST_SIZE;
		state->b_bytes_valid = size + IGZIP_HIST_SIZE;

		stream->next_in += size;
		stream->avail_in -= size;
		stream->total_in += size;

	} else {
		stream->total_in += state->b_bytes_valid - state->b_bytes_processed;
		stream->next_in = next_in;
		stream->avail_in = avail_in;
		state->file_start = stream->next_in - stream->total_in;

	}

	return ret;
}

static int write_gzip_header_stateless(struct isal_zstream *stream)
{
	if (gzip_hdr_bytes >= stream->avail_out)
		return STATELESS_OVERFLOW;

	stream->avail_out -= gzip_hdr_bytes;
	stream->total_out += gzip_hdr_bytes;

	memcpy(stream->next_out, gzip_hdr, gzip_hdr_bytes);

	stream->next_out += gzip_hdr_bytes;
	stream->gzip_flag = IGZIP_GZIP_NO_HDR;

	return COMP_OK;
}

static void write_gzip_header(struct isal_zstream *stream)
{
	struct isal_zstate *state = &stream->internal_state;
	int bytes_to_write = gzip_hdr_bytes;

	bytes_to_write -= state->count;

	if (bytes_to_write > stream->avail_out)
		bytes_to_write = stream->avail_out;

	memcpy(stream->next_out, gzip_hdr + state->count, bytes_to_write);
	state->count += bytes_to_write;

	if (state->count == gzip_hdr_bytes) {
		state->count = 0;
		stream->gzip_flag = IGZIP_GZIP_NO_HDR;
	}

	stream->avail_out -= bytes_to_write;
	stream->total_out += bytes_to_write;
	stream->next_out += bytes_to_write;

}

static int write_deflate_header_stateless(struct isal_zstream *stream)
{
	struct isal_zstate *state = &stream->internal_state;
	struct isal_hufftables *hufftables = stream->hufftables;
	uint64_t hdr_extra_bits = hufftables->deflate_hdr[hufftables->deflate_hdr_count];
	uint32_t count;

	if (hufftables->deflate_hdr_count + 8 >= stream->avail_out)
		return STATELESS_OVERFLOW;

	memcpy(stream->next_out, hufftables->deflate_hdr, hufftables->deflate_hdr_count);

	if (stream->end_of_stream == 0) {
		if (hufftables->deflate_hdr_count > 0)
			*stream->next_out -= 1;
		else
			hdr_extra_bits -= 1;
	} else
		state->has_eob_hdr = 1;

	stream->avail_out -= hufftables->deflate_hdr_count;
	stream->total_out += hufftables->deflate_hdr_count;
	stream->next_out += hufftables->deflate_hdr_count;

	set_buf(&state->bitbuf, stream->next_out, stream->avail_out);

	write_bits(&state->bitbuf, hdr_extra_bits, hufftables->deflate_hdr_extra_bits);

	count = buffer_used(&state->bitbuf);
	stream->next_out = buffer_ptr(&state->bitbuf);
	stream->avail_out -= count;
	stream->total_out += count;

	state->state = ZSTATE_BODY;

	return COMP_OK;
}

static int write_deflate_header_unaligned_stateless(struct isal_zstream *stream)
{
	struct isal_zstate *state = &stream->internal_state;
	struct isal_hufftables *hufftables = stream->hufftables;
	unsigned int count;
	uint64_t bit_count;
	uint64_t *header_next;
	uint64_t *header_end;
	uint64_t header_bits;

	if (state->bitbuf.m_bit_count == 0)
		return write_deflate_header_stateless(stream);

	if (hufftables->deflate_hdr_count + 16 >= stream->avail_out)
		return STATELESS_OVERFLOW;

	set_buf(&state->bitbuf, stream->next_out, stream->avail_out);

	header_next = (uint64_t *) hufftables->deflate_hdr;
	header_end = header_next + hufftables->deflate_hdr_count / 8;

	header_bits = *header_next;

	if (stream->end_of_stream == 0)
		header_bits--;
	else
		state->has_eob_hdr = 1;

	header_next++;

	/* Write out Complete Header bits */
	for (; header_next <= header_end; header_next++) {
		write_bits(&state->bitbuf, header_bits, 32);
		header_bits >>= 32;
		write_bits(&state->bitbuf, header_bits, 32);
		header_bits = *header_next;
	}

	bit_count =
	    (hufftables->deflate_hdr_count & 0x7) * 8 + hufftables->deflate_hdr_extra_bits;

	if (bit_count > MAX_BITBUF_BIT_WRITE) {
		write_bits(&state->bitbuf, header_bits, MAX_BITBUF_BIT_WRITE);
		header_bits >>= MAX_BITBUF_BIT_WRITE;
		bit_count -= MAX_BITBUF_BIT_WRITE;

	}

	write_bits(&state->bitbuf, header_bits, bit_count);

	/* check_space flushes extra bytes in bitbuf. Required because
	 * write_bits_always fails when the next commit makes the buffer
	 * length exceed 64 bits */
	check_space(&state->bitbuf, FORCE_FLUSH);

	count = buffer_used(&state->bitbuf);
	stream->next_out = buffer_ptr(&state->bitbuf);
	stream->avail_out -= count;
	stream->total_out += count;

	state->state = ZSTATE_BODY;

	return COMP_OK;
}

/* Toggle end of stream only works when deflate header is aligned */
void write_header(struct isal_zstream *stream, uint8_t * deflate_hdr,
		  uint32_t deflate_hdr_count, uint32_t extra_bits_count, uint32_t next_state,
		  uint32_t toggle_end_of_stream)
{
	struct isal_zstate *state = &stream->internal_state;
	uint32_t hdr_extra_bits = deflate_hdr[deflate_hdr_count];
	uint32_t count;
	state->state = ZSTATE_HDR;

	if (state->bitbuf.m_bit_count != 0) {
		if (stream->avail_out < 8)
			return;
		set_buf(&state->bitbuf, stream->next_out, stream->avail_out);
		flush(&state->bitbuf);
		count = buffer_used(&state->bitbuf);
		stream->next_out = buffer_ptr(&state->bitbuf);
		stream->avail_out -= count;
		stream->total_out += count;
	}

	if (stream->gzip_flag == IGZIP_GZIP)
		write_gzip_header(stream);

	count = deflate_hdr_count - state->count;

	if (count != 0) {
		if (count > stream->avail_out)
			count = stream->avail_out;

		memcpy(stream->next_out, deflate_hdr + state->count, count);

		if (toggle_end_of_stream && state->count == 0 && count > 0) {
			/* Assumes the final block bit is the first bit */
			*stream->next_out ^= 1;
			state->has_eob_hdr = !state->has_eob_hdr;
		}

		stream->next_out += count;
		stream->avail_out -= count;
		stream->total_out += count;
		state->count += count;

		count = deflate_hdr_count - state->count;
	} else if (toggle_end_of_stream && deflate_hdr_count == 0) {
		/* Assumes the final block bit is the first bit */
		hdr_extra_bits ^= 1;
		state->has_eob_hdr = !state->has_eob_hdr;
	}

	if ((count == 0) && (stream->avail_out >= 8)) {

		set_buf(&state->bitbuf, stream->next_out, stream->avail_out);

		write_bits(&state->bitbuf, hdr_extra_bits, extra_bits_count);

		state->state = next_state;
		state->count = 0;

		count = buffer_used(&state->bitbuf);
		stream->next_out = buffer_ptr(&state->bitbuf);
		stream->avail_out -= count;
		stream->total_out += count;
	}

}

void write_trailer(struct isal_zstream *stream)
{
	struct isal_zstate *state = &stream->internal_state;
	unsigned int bytes;
	uint32_t crc = state->crc;

	if (stream->avail_out >= 8) {
		set_buf(&state->bitbuf, stream->next_out, stream->avail_out);

		/* the flush() will pad to the next byte and write up to 8 bytes
		 * to the output stream/buffer.
		 */
		if (!state->has_eob_hdr) {
			/* If the final header has not been written, write a
			 * final block. This block is a static huffman block
			 * which only contains the end of block symbol. The code
			 * that happens to do this is the fist 10 bits of
			 * 0x003 */
			state->has_eob_hdr = 1;
			write_bits(&state->bitbuf, 0x003, 10);
			if (is_full(&state->bitbuf)) {
				stream->next_out = buffer_ptr(&state->bitbuf);
				bytes = buffer_used(&state->bitbuf);
				stream->avail_out -= bytes;
				stream->total_out += bytes;
				return;
			}
		}

		flush(&state->bitbuf);
		stream->next_out = buffer_ptr(&state->bitbuf);
		bytes = buffer_used(&state->bitbuf);

		if (stream->gzip_flag) {
			if (!is_full(&state->bitbuf)) {
				*(uint64_t *) stream->next_out =
				    ((uint64_t) stream->total_in << 32) | crc;
				stream->next_out += 8;
				bytes += 8;
				state->state = ZSTATE_END;
			}
		} else
			state->state = ZSTATE_END;

		stream->avail_out -= bytes;
		stream->total_out += bytes;
	}
}
