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

#ifndef IGZIP_USE_GZIP_FORMAT
# define DEFLATE 1
#endif

#define MAX_WRITE_BITS_SIZE 8
#define FORCE_FLUSH 64
#define MIN_OBUF_SIZE 224
#define NON_EMPTY_BLOCK_SIZE 6
#define MAX_SYNC_FLUSH_SIZE NON_EMPTY_BLOCK_SIZE + MAX_WRITE_BITS_SIZE

#include "huffman.h"
#include "bitbuf2.h"
#include "igzip_lib.h"
#include "repeated_char_result.h"

extern const uint8_t gzip_hdr[];
extern const uint32_t gzip_hdr_bytes;
extern const uint32_t gzip_trl_bytes;
extern const struct isal_hufftables hufftables_default;

extern uint32_t crc32_gzip(uint32_t init_crc, const unsigned char *buf, uint64_t len);

static int write_stored_block_stateless(struct isal_zstream *stream, uint32_t stored_len,
					uint32_t crc32);
#ifndef DEFLATE
static int write_gzip_header_stateless(struct isal_zstream *stream);
#endif
static int write_deflate_header_stateless(struct isal_zstream *stream);
static int write_deflate_header_unaligned_stateless(struct isal_zstream *stream);
static int write_trailer_stateless(struct isal_zstream *stream, uint32_t avail_in,
				   uint32_t crc32);

void isal_deflate_body_stateless(struct isal_zstream *stream);

unsigned int detect_repeated_char(uint8_t * buf, uint32_t size);

#define STORED_BLK_HDR_BZ 5
#define STORED_BLK_MAX_BZ 65535

void isal_deflate_body(struct isal_zstream *stream);
void isal_deflate_finish(struct isal_zstream *stream);
uint32_t crc_512to32_01(uint32_t * crc);
uint32_t get_crc(uint32_t * crc);

/*****************************************************************/

/* Forward declarations */
static inline void reset_match_history(struct isal_zstream *stream);
void write_header(struct isal_zstream *stream);
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

struct slver isal_deflate_slver_01030082;
struct slver isal_deflate_slver = { 0x0082, 0x03, 0x01 };

struct slver isal_deflate_stateless_slver_01010083;
struct slver isal_deflate_stateless_slver = { 0x0083, 0x01, 0x01 };

/*****************************************************************/

uint32_t file_size(struct isal_zstate *state)
{
	return state->b_bytes_valid + (uint32_t) (state->buffer - state->file_start);
}

static
void sync_flush(struct isal_zstream *stream)
{
	struct isal_zstate *state = &stream->internal_state;
	uint64_t bits_to_write = 0xFFFF0000, bits_len;
	uint64_t code = 0, len = 0, bytes;
	int flush_size;

	if (stream->avail_out >= 8) {
		set_buf(&state->bitbuf, stream->next_out, stream->avail_out);

		if (!state->has_eob)
			get_lit_code(stream->hufftables, 256, &code, &len);

		flush_size = (-(state->bitbuf.m_bit_count + len + 3)) % 8;

		bits_to_write <<= flush_size + 3;
		bits_len = 32 + len + flush_size + 3;

#ifdef USE_BITBUFB		/* Write Bits Always */
		state->state = ZSTATE_NEW_HDR;
#else /* Not Write Bits Always */
		state->state = ZSTATE_FLUSH_WRITE_BUFFER;
#endif
		state->has_eob = 0;

		if (len > 0)
			bits_to_write = (bits_to_write << len) | code;

		write_bits(&state->bitbuf, bits_to_write, bits_len);

		bytes = buffer_used(&state->bitbuf);
		stream->next_out = buffer_ptr(&state->bitbuf);
		stream->avail_out -= bytes;
		stream->total_out += bytes;

		if (stream->flush == FULL_FLUSH) {
			/* Clear match history so there are no cross
			 * block length distance pairs */
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

static void isal_deflate_int(struct isal_zstream *stream)
{
	struct isal_zstate *state = &stream->internal_state;
	if (state->state == ZSTATE_NEW_HDR || state->state == ZSTATE_HDR)
		write_header(stream);

	if (state->state == ZSTATE_BODY)
		isal_deflate_body(stream);

	if (state->state == ZSTATE_FLUSH_READ_BUFFER)
		isal_deflate_finish(stream);

	if (state->state == ZSTATE_SYNC_FLUSH)
		sync_flush(stream);

	if (state->state == ZSTATE_FLUSH_WRITE_BUFFER)
		flush_write_buffer(stream);

	if (state->state == ZSTATE_TRL)
		write_trailer(stream);
}

static uint32_t write_constant_compressed_stateless(struct isal_zstream *stream,
						    uint32_t repeated_char,
						    uint32_t repeated_length,
						    uint32_t end_of_stream)
{
	/* Assumes repeated_length is at least 1.
	 * Assumes the input end_of_stream is either 0 or 1. */
	struct isal_zstate *state = &stream->internal_state;
	uint32_t rep_bits = ((repeated_length - 1) / 258) * 2;
	uint32_t rep_bytes = rep_bits / 8;
	uint32_t rep_extra = (repeated_length - 1) % 258;
	uint32_t bytes;

	/* Guarantee there is enough space for the header even in the worst case */
	if (stream->avail_out < HEADER_LENGTH + MAX_FIXUP_CODE_LENGTH + rep_bytes + 8)
		return STATELESS_OVERFLOW;

	/* Assumes the repeated char is either 0 or 0xFF. */
	memcpy(stream->next_out, repeated_char_header[repeated_char & 1], HEADER_LENGTH);

	if (end_of_stream > 0)
		stream->next_out[0] |= 1;

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
				   CODE_280 | ((rep_extra / 2 - 115) << CODE_280_LENGTH),
				   CODE_280_TOTAL_LENGTH);
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

	return COMP_OK;
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

static int isal_deflate_int_stateless(struct isal_zstream *stream, uint8_t * next_in,
				      const uint32_t avail_in)
{
	uint32_t crc32 = 0;
	uint32_t repeated_char_length;

#ifndef DEFLATE
	if (write_gzip_header_stateless(stream))
		return STATELESS_OVERFLOW;
#endif

	if (avail_in >= 8
	    && (*(uint64_t *) stream->next_in == 0
		|| *(uint64_t *) stream->next_in == ~(uint64_t) 0))
		repeated_char_length =
		    detect_repeated_char_length(stream->next_in, stream->avail_in);
	else
		repeated_char_length = 0;

	if (stream->avail_in == repeated_char_length) {
		if (write_constant_compressed_stateless(stream,
							stream->next_in[0],
							repeated_char_length, 1) != COMP_OK)
			return STATELESS_OVERFLOW;

#ifndef DEFLATE
		crc32 = crc32_gzip(0x0, next_in, avail_in);
#endif

		/* write_trailer_stateless is required because if flushes out the last of the output */
		if (write_trailer_stateless(stream, avail_in, crc32) != COMP_OK)
			return STATELESS_OVERFLOW;
		return COMP_OK;

	} else if (repeated_char_length >= MIN_REPEAT_LEN) {
		if (write_constant_compressed_stateless
		    (stream, stream->next_in[0], repeated_char_length, 0) != COMP_OK)
			return STATELESS_OVERFLOW;
	}

	if (write_deflate_header_unaligned_stateless(stream) != COMP_OK)
		return STATELESS_OVERFLOW;
	if (stream->avail_out < 8)
		return STATELESS_OVERFLOW;

	isal_deflate_body_stateless(stream);

	if (!stream->internal_state.has_eob)
		return STATELESS_OVERFLOW;

#ifndef DEFLATE
	crc32 = crc32_gzip(0x0, next_in, avail_in);
#endif

	if (write_trailer_stateless(stream, avail_in, crc32) != COMP_OK)
		return STATELESS_OVERFLOW;

	return COMP_OK;
}

static int write_stored_block_stateless(struct isal_zstream *stream,
					uint32_t stored_len, uint32_t crc32)
{
	uint64_t stored_blk_hdr;
	uint32_t copy_size;
	uint32_t avail_in;

#ifndef DEFLATE
	uint64_t gzip_trl;
#endif

	if (stream->avail_out < stored_len)
		return STATELESS_OVERFLOW;

	stream->avail_out -= stored_len;
	stream->total_out += stored_len;
	avail_in = stream->avail_in;

#ifndef DEFLATE
	memcpy(stream->next_out, gzip_hdr, gzip_hdr_bytes);
	stream->next_out += gzip_hdr_bytes;
#endif

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
		if (avail_in == 0)
			stored_blk_hdr |= 0x1;

		memcpy(stream->next_out, &stored_blk_hdr, STORED_BLK_HDR_BZ);
		stream->next_out += STORED_BLK_HDR_BZ;

		memcpy(stream->next_out, stream->next_in, copy_size);
		stream->next_out += copy_size;
		stream->next_in += copy_size;
		stream->total_in += copy_size;
	} while (avail_in != 0);

#ifndef DEFLATE
	gzip_trl = stream->avail_in;
	gzip_trl <<= 32;
	gzip_trl |= crc32 & 0xFFFFFFFF;
	memcpy(stream->next_out, &gzip_trl, gzip_trl_bytes);
	stream->next_out += gzip_trl_bytes;
#endif

	stream->avail_in = 0;
	return COMP_OK;
}

static inline void reset_match_history(struct isal_zstream *stream)
{
	struct isal_zstate *state = &stream->internal_state;
	uint16_t *head = stream->internal_state.head;
	int i = 0;

	for (i = 0; i < sizeof(state->head) / 2; i++)
		head[i] =
		    (uint16_t) (state->b_bytes_processed + state->buffer - state->file_start -
				(IGZIP_D + 1));
}

void isal_deflate_init_01(struct isal_zstream *stream)
{
	struct isal_zstate *state = &stream->internal_state;

	stream->total_in = 0;
	stream->total_out = 0;
	stream->hufftables = (struct isal_hufftables *)&hufftables_default;
	stream->flush = 0;

	state->b_bytes_valid = 0;
	state->b_bytes_processed = 0;
	state->has_eob = 0;
	state->has_eob_hdr = 0;
	state->left_over = 0;
	state->last_flush = 0;
	state->has_gzip_hdr = 0;
	state->state = ZSTATE_NEW_HDR;
	state->count = 0;

	state->tmp_out_start = 0;
	state->tmp_out_end = 0;

	state->file_start = state->buffer;

	init(&state->bitbuf);

	memset(state->crc, 0, sizeof(state->crc));
	*state->crc = 0x9db42487;

	reset_match_history(stream);

	return;
}

int isal_deflate_stateless(struct isal_zstream *stream)
{
	uint8_t *next_in = stream->next_in;
	const uint32_t avail_in = stream->avail_in;

	uint8_t *next_out = stream->next_out;
	const uint32_t avail_out = stream->avail_out;

	uint32_t crc32 = 0;
	uint32_t stored_len;
	uint32_t dyn_min_len;
	uint32_t min_len;
	uint32_t select_stored_blk = 0;

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

	dyn_min_len = stream->hufftables->deflate_hdr_count + 1;
#ifndef DEFLATE
	dyn_min_len += gzip_hdr_bytes + gzip_trl_bytes + 1;
	stored_len += gzip_hdr_bytes + gzip_trl_bytes;
#endif

	min_len = dyn_min_len;

	if (stored_len < dyn_min_len) {
		min_len = stored_len;
		select_stored_blk = 1;
	}

	/*
	   the output buffer should be no less than 8 bytes
	   while empty stored deflate block is 5 bytes only
	 */
	if (avail_out < min_len || stream->avail_out < 8)
		return STATELESS_OVERFLOW;

	if (!select_stored_blk) {
		if (isal_deflate_int_stateless(stream, next_in, avail_in) == COMP_OK)
			return COMP_OK;
	}
	if (avail_out < stored_len)
		return STATELESS_OVERFLOW;

	isal_deflate_init(stream);

	stream->next_in = next_in;
	stream->avail_in = avail_in;
	stream->total_in = 0;

	stream->next_out = next_out;
	stream->avail_out = avail_out;
	stream->total_out = 0;

#ifndef DEFLATE
	crc32 = crc32_gzip(0x0, next_in, avail_in);
#endif
	return write_stored_block_stateless(stream, stored_len, crc32);
}

int isal_deflate(struct isal_zstream *stream)
{
	struct isal_zstate *state = &stream->internal_state;
	uint32_t size;
	int ret = COMP_OK;

	if (stream->flush < 3) {

		state->last_flush = stream->flush;

		if (state->state >= TMP_OFFSET_SIZE) {
			size = state->tmp_out_end - state->tmp_out_start;
			if (size > stream->avail_out)
				size = stream->avail_out;
			memcpy(stream->next_out, state->tmp_out_buff + state->tmp_out_start,
			       size);
			stream->next_out += size;
			stream->avail_out -= size;
			stream->total_out += size;
			state->tmp_out_start += size;

			if (state->tmp_out_start == state->tmp_out_end)
				state->state -= TMP_OFFSET_SIZE;

			if (stream->avail_out == 0 || state->state == ZSTATE_END)
				return ret;
		}
		assert(state->tmp_out_start == state->tmp_out_end);

		isal_deflate_int(stream);

		if (stream->avail_out == 0)
			return ret;

		else if (stream->avail_out < 8) {
			uint8_t *next_out;
			uint32_t avail_out;
			uint32_t total_out;

			next_out = stream->next_out;
			avail_out = stream->avail_out;
			total_out = stream->total_out;

			stream->next_out = state->tmp_out_buff;
			stream->avail_out = sizeof(state->tmp_out_buff);
			stream->total_out = 0;

			isal_deflate_int(stream);

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
					state->state += TMP_OFFSET_SIZE;

			}
		}
	} else
		ret = INVALID_FLUSH;

	return ret;
}

#ifndef DEFLATE
static int write_gzip_header_stateless(struct isal_zstream *stream)
{
	if (gzip_hdr_bytes >= stream->avail_out)
		return STATELESS_OVERFLOW;

	stream->avail_out -= gzip_hdr_bytes;
	stream->total_out += gzip_hdr_bytes;

	memcpy(stream->next_out, gzip_hdr, gzip_hdr_bytes);

	stream->next_out += gzip_hdr_bytes;

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
		state->has_gzip_hdr = 1;
	}

	stream->avail_out -= bytes_to_write;
	stream->total_out += bytes_to_write;
	stream->next_out += bytes_to_write;

}
#endif

static int write_deflate_header_stateless(struct isal_zstream *stream)
{
	struct isal_zstate *state = &stream->internal_state;
	struct isal_hufftables *hufftables = stream->hufftables;
	uint32_t count;

	if (hufftables->deflate_hdr_count + 8 >= stream->avail_out)
		return STATELESS_OVERFLOW;

	memcpy(stream->next_out, hufftables->deflate_hdr, hufftables->deflate_hdr_count);

	stream->avail_out -= hufftables->deflate_hdr_count;
	stream->total_out += hufftables->deflate_hdr_count;
	stream->next_out += hufftables->deflate_hdr_count;

	set_buf(&state->bitbuf, stream->next_out, stream->avail_out);

	write_bits(&state->bitbuf, hufftables->deflate_hdr[hufftables->deflate_hdr_count],
		   hufftables->deflate_hdr_extra_bits);

	count = buffer_used(&state->bitbuf);
	stream->next_out = buffer_ptr(&state->bitbuf);
	stream->avail_out -= count;
	stream->total_out += count;

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

	/* Write out Complete Header bits */
	for (; header_next < header_end; header_next++) {
		header_bits = *header_next;
		write_bits(&state->bitbuf, header_bits, 32);
		header_bits >>= 32;
		write_bits(&state->bitbuf, header_bits, 32);
	}

	header_bits = *header_next;
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

	return COMP_OK;
}

void write_header(struct isal_zstream *stream)
{
	struct isal_zstate *state = &stream->internal_state;
	struct isal_hufftables *hufftables = stream->hufftables;
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
#ifndef DEFLATE
	if (!state->has_gzip_hdr)
		write_gzip_header(stream);
#endif

	count = hufftables->deflate_hdr_count - state->count;

	if (count != 0) {
		if (count > stream->avail_out)
			count = stream->avail_out;

		memcpy(stream->next_out, hufftables->deflate_hdr + state->count, count);

		if (state->count == 0 && count > 0) {
			if (!stream->end_of_stream)
				*stream->next_out &= 0xfe;
			else
				state->has_eob_hdr = 1;
		}

		stream->next_out += count;
		stream->avail_out -= count;
		stream->total_out += count;
		state->count += count;

		count = hufftables->deflate_hdr_count - state->count;
	}

	if ((count == 0) && (stream->avail_out >= 8)) {

		set_buf(&state->bitbuf, stream->next_out, stream->avail_out);

		write_bits(&state->bitbuf,
			   hufftables->deflate_hdr[hufftables->deflate_hdr_count],
			   hufftables->deflate_hdr_extra_bits);

		state->state = ZSTATE_BODY;
		state->count = 0;

		count = buffer_used(&state->bitbuf);
		stream->next_out = buffer_ptr(&state->bitbuf);
		stream->avail_out -= count;
		stream->total_out += count;
	}

}

uint32_t get_crc_01(uint32_t * crc)
{
	return crc_512to32_01(crc);
}

void write_trailer(struct isal_zstream *stream)
{
	struct isal_zstate *state = &stream->internal_state;
	unsigned int bytes;

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

#ifndef DEFLATE
		uint32_t *crc = state->crc;

		if (!is_full(&state->bitbuf)) {
			*(uint64_t *) stream->next_out =
			    ((uint64_t) file_size(state) << 32) | get_crc(crc);
			stream->next_out += 8;
			bytes += 8;
			state->state = ZSTATE_END;
		}
#else
		state->state = ZSTATE_END;
#endif

		stream->avail_out -= bytes;
		stream->total_out += bytes;
	}
}

static int write_trailer_stateless(struct isal_zstream *stream, uint32_t avail_in,
				   uint32_t crc32)
{
	int ret = COMP_OK;
	struct isal_zstate *state = &stream->internal_state;
	unsigned int bytes;

	if (stream->avail_out < 8) {
		ret = STATELESS_OVERFLOW;
	} else {
		set_buf(&state->bitbuf, stream->next_out, stream->avail_out);

		/* the flush() will pad to the next byte and write up to 8 bytes
		 * to the output stream/buffer.
		 */
		flush(&state->bitbuf);
		stream->next_out = buffer_ptr(&state->bitbuf);
		bytes = buffer_used(&state->bitbuf);
#ifndef DEFLATE
		if (is_full(&state->bitbuf)) {
			ret = STATELESS_OVERFLOW;
		} else {
			*(uint64_t *) stream->next_out = ((uint64_t) avail_in << 32) | crc32;
			stream->next_out += 8;
			bytes += 8;
		}
#endif
		stream->avail_out -= bytes;
		stream->total_out += bytes;
	}

	return ret;
}
