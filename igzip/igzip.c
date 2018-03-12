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

#include "huffman.h"
#include "bitbuf2.h"
#include "igzip_lib.h"
#include "repeated_char_result.h"
#include "huff_codes.h"
#include "encode_df.h"
#include "igzip_level_buf_structs.h"
#include "igzip_checksums.h"

#ifdef __FreeBSD__
#include <sys/types.h>
#include <sys/endian.h>
# define to_be32(x) bswap32(x)
#elif defined (__APPLE__)
#include <libkern/OSByteOrder.h>
# define to_be32(x) OSSwapInt32(x)
#elif defined (__GNUC__) && !defined (__MINGW32__)
# include <byteswap.h>
# define to_be32(x) bswap_32(x)
#elif defined _WIN64
# define to_be32(x) _byteswap_ulong(x)
#endif

extern void isal_deflate_hash_lvl0(uint16_t *, uint32_t, uint32_t, uint8_t *, uint32_t);
extern void isal_deflate_hash_lvl1(uint16_t *, uint32_t, uint32_t, uint8_t *, uint32_t);
extern void isal_deflate_hash_lvl2(uint16_t *, uint32_t, uint32_t, uint8_t *, uint32_t);
extern void isal_deflate_hash_lvl3(uint16_t *, uint32_t, uint32_t, uint8_t *, uint32_t);
extern const uint8_t gzip_hdr[];
extern const uint32_t gzip_hdr_bytes;
extern const uint32_t gzip_trl_bytes;
extern const uint8_t zlib_hdr[];
extern const uint32_t zlib_hdr_bytes;
extern const uint32_t zlib_trl_bytes;
extern const struct isal_hufftables hufftables_default;
extern const struct isal_hufftables hufftables_static;

static uint32_t write_stored_block(struct isal_zstream *stream);

static int write_stream_header_stateless(struct isal_zstream *stream);
static void write_stream_header(struct isal_zstream *stream);
static int write_deflate_header_stateless(struct isal_zstream *stream);
static int write_deflate_header_unaligned_stateless(struct isal_zstream *stream);

#define TYPE0_HDR_LEN 4
#define TYPE0_BLK_HDR_LEN 5
#define TYPE0_MAX_BLK_LEN 65535

void isal_deflate_body(struct isal_zstream *stream);
void isal_deflate_finish(struct isal_zstream *stream);

void isal_deflate_icf_body(struct isal_zstream *stream);
void isal_deflate_icf_finish_lvl1(struct isal_zstream *stream);
void isal_deflate_icf_finish_lvl2(struct isal_zstream *stream);
void isal_deflate_icf_finish_lvl3(struct isal_zstream *stream);
/*****************************************************************/

/* Forward declarations */
static inline void reset_match_history(struct isal_zstream *stream);
static void write_header(struct isal_zstream *stream, uint8_t * deflate_hdr,
			 uint32_t deflate_hdr_count, uint32_t extra_bits_count,
			 uint32_t next_state, uint32_t toggle_end_of_stream);
static void write_trailer(struct isal_zstream *stream);

struct slver {
	uint16_t snum;
	uint8_t ver;
	uint8_t core;
};

/* Version info */
struct slver isal_deflate_init_slver_01030081;
struct slver isal_deflate_init_slver = { 0x0081, 0x03, 0x01 };

struct slver isal_deflate_reset_slver_0001008e;
struct slver isal_deflate_reset_slver = { 0x008e, 0x01, 0x00 };

struct slver isal_deflate_stateless_init_slver_00010084;
struct slver isal_deflate_stateless_init_slver = { 0x0084, 0x01, 0x00 };

struct slver isal_deflate_slver_01030082;
struct slver isal_deflate_slver = { 0x0082, 0x03, 0x01 };

struct slver isal_deflate_stateless_slver_01010083;
struct slver isal_deflate_stateless_slver = { 0x0083, 0x01, 0x01 };

struct slver isal_deflate_set_hufftables_slver_0001008b;
struct slver isal_deflate_set_hufftables_slver = { 0x008b, 0x01, 0x00 };

struct slver isal_deflate_set_dict_slver_0001008c;
struct slver isal_deflate_set_dict_slver = { 0x008c, 0x01, 0x00 };

/*****************************************************************/

// isal_adler32_bam1 - adler with (B | A minus 1) storage

uint32_t isal_adler32_bam1(uint32_t adler32, const unsigned char *start, uint64_t length)
{
	uint64_t a;

	/* Internally the checksum is being stored as B | (A-1) so crc and
	 * addler have same init value */
	a = adler32 & 0xffff;
	a = (a == ADLER_MOD - 1) ? 0 : a + 1;
	adler32 = isal_adler32((adler32 & 0xffff0000) | a, start, length);
	a = (adler32 & 0xffff);
	a = (a == 0) ? ADLER_MOD - 1 : a - 1;

	return (adler32 & 0xffff0000) | a;
}

static void update_checksum(struct isal_zstream *stream, uint8_t * start_in, uint64_t length)
{
	struct isal_zstate *state = &stream->internal_state;
	switch (stream->gzip_flag) {
	case IGZIP_GZIP:
	case IGZIP_GZIP_NO_HDR:
		state->crc = crc32_gzip(state->crc, start_in, length);
		break;
	case IGZIP_ZLIB:
	case IGZIP_ZLIB_NO_HDR:
		state->crc = isal_adler32_bam1(state->crc, start_in, length);
		break;
	}
}

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

		state->state = ZSTATE_NEW_HDR;
		state->has_eob = 0;

		write_bits(&state->bitbuf, bits_to_write, bits_len);

		bytes = buffer_used(&state->bitbuf);
		stream->next_out = buffer_ptr(&state->bitbuf);
		stream->avail_out -= bytes;
		stream->total_out += bytes;

		if (stream->flush == FULL_FLUSH) {
			/* Clear match history so there are no cross
			 * block length distance pairs */
			state->has_hist = IGZIP_NO_HIST;
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
	struct level_buf *level_buf = (struct level_buf *)stream->level_buf;
	struct BitBuf2 *write_buf = &state->bitbuf;
	struct deflate_icf *icf_buf_encoded_next;

	set_buf(write_buf, stream->next_out, stream->avail_out);

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

static int check_level_req(struct isal_zstream *stream)
{
	if (stream->level == 0)
		return 0;

	if (stream->level_buf == NULL)
		return ISAL_INVALID_LEVEL_BUF;

	switch (stream->level) {
	case 3:
		if (stream->level_buf_size < ISAL_DEF_LVL3_MIN)
			return ISAL_INVALID_LEVEL;
		break;

	case 2:
		if (stream->level_buf_size < ISAL_DEF_LVL2_MIN)
			return ISAL_INVALID_LEVEL;
		break;
	case 1:
		if (stream->level_buf_size < ISAL_DEF_LVL1_MIN)
			return ISAL_INVALID_LEVEL;
		break;
	default:
		return ISAL_INVALID_LEVEL;
	}

	return 0;
}

static int init_hash8k_buf(struct isal_zstream *stream)
{
	struct isal_zstate *state = &stream->internal_state;
	struct level_buf *level_buf = (struct level_buf *)stream->level_buf;
	state->has_level_buf_init = 1;
	return sizeof(struct level_buf) - MAX_LVL_BUF_SIZE + sizeof(level_buf->hash8k);
}

static int init_hash_hist_buf(struct isal_zstream *stream)
{
	struct isal_zstate *state = &stream->internal_state;
	struct level_buf *level_buf = (struct level_buf *)stream->level_buf;
	state->has_level_buf_init = 1;
	return sizeof(struct level_buf) - MAX_LVL_BUF_SIZE + sizeof(level_buf->hash_hist);
}

static int init_hash_map_buf(struct isal_zstream *stream)
{
	struct isal_zstate *state = &stream->internal_state;
	struct level_buf *level_buf = (struct level_buf *)stream->level_buf;
	if (!state->has_level_buf_init) {
		level_buf->hash_map.matches_next = level_buf->hash_map.matches;
		level_buf->hash_map.matches_end = level_buf->hash_map.matches;
	}
	state->has_level_buf_init = 1;

	return sizeof(struct level_buf) - MAX_LVL_BUF_SIZE + sizeof(level_buf->hash_map);

}

/* returns the size of the level specific buffer */
static int init_lvlX_buf(struct isal_zstream *stream)
{
	switch (stream->level) {
	case 3:
		return init_hash_map_buf(stream);
	case 2:
		return init_hash_hist_buf(stream);
	default:
		return init_hash8k_buf(stream);
	}

}

static void init_new_icf_block(struct isal_zstream *stream)
{
	struct isal_zstate *state = &stream->internal_state;
	struct level_buf *level_buf = (struct level_buf *)stream->level_buf;
	int level_struct_size;

	level_struct_size = init_lvlX_buf(stream);

	state->block_next = state->block_end;
	level_buf->icf_buf_start =
	    (struct deflate_icf *)(stream->level_buf + level_struct_size);

	level_buf->icf_buf_next = level_buf->icf_buf_start;
	level_buf->icf_buf_avail_out =
	    stream->level_buf_size - level_struct_size - sizeof(struct deflate_icf);

	memset(&level_buf->hist, 0, sizeof(struct isal_mod_hist));
	state->state = ZSTATE_BODY;
}

static int are_buffers_empty_hashX(struct isal_zstream *stream)
{
	return !stream->avail_in;
}

static int are_buffers_empty_hash_map(struct isal_zstream *stream)
{
	struct level_buf *level_buf = (struct level_buf *)stream->level_buf;

	return (!stream->avail_in
		&& level_buf->hash_map.matches_next >= level_buf->hash_map.matches_end);
}

static int are_buffers_empty(struct isal_zstream *stream)
{

	switch (stream->level) {
	case 3:
		return are_buffers_empty_hash_map(stream);
	case 2:
		return are_buffers_empty_hashX(stream);
	default:
		return are_buffers_empty_hashX(stream);
	}
}

static void create_icf_block_hdr(struct isal_zstream *stream)
{
	struct isal_zstate *state = &stream->internal_state;
	struct level_buf *level_buf = (struct level_buf *)stream->level_buf;
	struct BitBuf2 *write_buf = &state->bitbuf;
	struct BitBuf2 write_buf_tmp;
	uint32_t out_size = stream->avail_out;
	uint8_t *end_out = stream->next_out + out_size;
	uint64_t bit_count;
	uint64_t block_in_size = state->block_end - state->block_next;
	uint64_t block_size;
	int buffer_header = 0;

	memcpy(&write_buf_tmp, write_buf, sizeof(struct BitBuf2));

	block_size = (TYPE0_BLK_HDR_LEN) * ((block_in_size + TYPE0_MAX_BLK_LEN - 1) /
					    TYPE0_MAX_BLK_LEN) + block_in_size;
	block_size = block_size ? block_size : TYPE0_BLK_HDR_LEN;

	/* Write EOB in icf_buf */
	level_buf->hist.ll_hist[256] = 1;
	level_buf->icf_buf_next->lit_len = 0x100;
	level_buf->icf_buf_next->lit_dist = NULL_DIST_SYM;
	level_buf->icf_buf_next->dist_extra = 0;
	level_buf->icf_buf_next++;

	state->has_eob_hdr = (stream->end_of_stream && are_buffers_empty(stream)) ? 1 : 0;

	if (end_out - stream->next_out >= ISAL_DEF_MAX_HDR_SIZE) {
		/* Assumes ISAL_DEF_MAX_HDR_SIZE is large enough to contain a
		 * max length header and a gzip header */
		if (stream->gzip_flag == IGZIP_GZIP || stream->gzip_flag == IGZIP_ZLIB)
			write_stream_header_stateless(stream);
		set_buf(write_buf, stream->next_out, stream->avail_out);
		buffer_header = 0;

	} else {
		/* Start writing into temporary buffer */
		set_buf(write_buf, level_buf->deflate_hdr, ISAL_DEF_MAX_HDR_SIZE);
		buffer_header = 1;
	}

	bit_count = create_hufftables_icf(write_buf, &level_buf->encode_tables,
					  &level_buf->hist, state->has_eob_hdr);

	if (bit_count / 8 >= block_size && state->block_next >= state->total_in_start &&
	    block_size <=
	    stream->avail_out + sizeof(state->buffer) - (stream->total_in - state->block_end) -
	    ISAL_LOOK_AHEAD) {
		/* Reset stream for writing out a type0 block */
		state->has_eob_hdr = 0;
		memcpy(write_buf, &write_buf_tmp, sizeof(struct BitBuf2));
		state->state = ZSTATE_TYPE0_HDR;

	} else if (buffer_header) {
		/* Setup stream to write out a buffered header */
		level_buf->deflate_hdr_count = buffer_used(write_buf);
		level_buf->deflate_hdr_extra_bits = write_buf->m_bit_count;
		flush(write_buf);
		memcpy(write_buf, &write_buf_tmp, sizeof(struct BitBuf2));
		write_buf->m_bits = 0;
		write_buf->m_bit_count = 0;
		state->state = ZSTATE_HDR;

	} else {
		stream->next_out = buffer_ptr(write_buf);
		stream->total_out += buffer_used(write_buf);
		stream->avail_out -= buffer_used(write_buf);
		state->state = ZSTATE_FLUSH_ICF_BUFFER;
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
		update_checksum(stream, start_in, stream->next_in - start_in);

	if (state->state == ZSTATE_TRL)
		write_trailer(stream);
}

static void isal_deflate_icf_finish(struct isal_zstream *stream)
{
	switch (stream->level) {
	case 3:
		isal_deflate_icf_finish_lvl3(stream);
		break;
	case 2:
		isal_deflate_icf_finish_lvl2(stream);
		break;
	default:
		isal_deflate_icf_finish_lvl1(stream);
	}
}

static void isal_deflate_icf_pass(struct isal_zstream *stream)
{
	uint8_t *start_in = stream->next_in;
	struct isal_zstate *state = &stream->internal_state;
	struct level_buf *level_buf = (struct level_buf *)stream->level_buf;

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

		if (state->state == ZSTATE_TYPE0_HDR || state->state == ZSTATE_TYPE0_BODY) {
			if (stream->gzip_flag == IGZIP_GZIP || stream->gzip_flag == IGZIP_ZLIB)
				write_stream_header(stream);
			write_stored_block(stream);
		}

	}
	while (state->state == ZSTATE_NEW_HDR);

	if (state->state == ZSTATE_SYNC_FLUSH)
		sync_flush(stream);

	if (state->state == ZSTATE_FLUSH_WRITE_BUFFER)
		flush_write_buffer(stream);

	if (stream->gzip_flag)
		update_checksum(stream, start_in, stream->next_in - start_in);

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
	state->block_end += repeated_length;

	bytes = buffer_used(&state->bitbuf);
	stream->next_out = buffer_ptr(&state->bitbuf);
	stream->avail_out -= bytes;
	stream->total_out += bytes;

	if (stream->gzip_flag)
		update_checksum(stream, start_in, stream->next_in - start_in);

	return;
}

static int detect_repeated_char_length(uint8_t * in, uint32_t length)
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

	if (stream->gzip_flag == IGZIP_GZIP || stream->gzip_flag == IGZIP_ZLIB)
		if (write_stream_header_stateless(stream))
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

		isal_deflate_pass(stream);

	} else if (stream->level <= ISAL_DEF_MAX_LEVEL) {
		if (state->state == ZSTATE_NEW_HDR || state->state == ZSTATE_HDR)
			reset_match_history(stream);

		state->count = 0;
		isal_deflate_icf_pass(stream);

	}

	if (state->state == ZSTATE_END
	    || (state->state == ZSTATE_NEW_HDR && stream->flush == FULL_FLUSH))
		return COMP_OK;
	else
		return STATELESS_OVERFLOW;
}

static void write_type0_header(struct isal_zstream *stream)
{
	struct isal_zstate *state = &stream->internal_state;
	uint64_t stored_blk_hdr;
	uint32_t copy_size;
	uint32_t memcpy_len, avail_in;
	uint32_t block_in_size = state->block_end - state->block_next;
	struct BitBuf2 *bitbuf = &stream->internal_state.bitbuf;

	if (block_in_size > TYPE0_MAX_BLK_LEN) {
		stored_blk_hdr = 0xFFFF;
		copy_size = TYPE0_MAX_BLK_LEN;
	} else {
		stored_blk_hdr = ~block_in_size;
		stored_blk_hdr <<= 16;
		stored_blk_hdr |= (block_in_size & 0xFFFF);
		copy_size = block_in_size;

		/* Handle BFINAL bit */
		avail_in = stream->total_in + stream->avail_in - state->block_next;
		if (stream->end_of_stream && avail_in == block_in_size)
			stream->internal_state.has_eob_hdr = 1;
	}

	if (bitbuf->m_bit_count == 0 && stream->avail_out >= TYPE0_HDR_LEN + 1) {
		stored_blk_hdr = stored_blk_hdr << 8;
		stored_blk_hdr |= stream->internal_state.has_eob_hdr;
		memcpy_len = TYPE0_HDR_LEN + 1;
		memcpy(stream->next_out, &stored_blk_hdr, memcpy_len);
	} else if (stream->avail_out >= 8) {
		set_buf(bitbuf, stream->next_out, stream->avail_out);
		write_bits_flush(bitbuf, stream->internal_state.has_eob_hdr, 3);
		stream->next_out = buffer_ptr(bitbuf);
		stream->total_out += buffer_used(bitbuf);
		stream->avail_out -= buffer_used(bitbuf);
		memcpy_len = TYPE0_HDR_LEN;
		memcpy(stream->next_out, &stored_blk_hdr, memcpy_len);
	} else {
		stream->internal_state.has_eob_hdr = 0;
		return;
	}

	stream->next_out += memcpy_len;
	stream->avail_out -= memcpy_len;
	stream->total_out += memcpy_len;
	stream->internal_state.state = ZSTATE_TYPE0_BODY;

	stream->internal_state.count = copy_size;
}

static uint32_t write_stored_block(struct isal_zstream *stream)
{
	uint32_t copy_size, avail_in;
	uint8_t *next_in;
	struct isal_zstate *state = &stream->internal_state;

	do {
		if (state->state == ZSTATE_TYPE0_HDR) {
			write_type0_header(stream);
			if (state->state == ZSTATE_TYPE0_HDR)
				break;
		}

		assert(state->count <= state->block_end - state->block_next);
		copy_size = state->count;

		next_in = stream->next_in - stream->total_in + state->block_next;
		avail_in = stream->total_in + stream->avail_in - state->block_next;
		if (copy_size > stream->avail_out || copy_size > avail_in) {
			state->count = copy_size;
			copy_size = (stream->avail_out <= avail_in) ?
			    stream->avail_out : avail_in;

			memcpy(stream->next_out, next_in, copy_size);
			state->count -= copy_size;
		} else {
			memcpy(stream->next_out, next_in, copy_size);

			state->count = 0;
			state->state = ZSTATE_TYPE0_HDR;
		}

		state->block_next += copy_size;
		stream->next_out += copy_size;
		stream->avail_out -= copy_size;
		stream->total_out += copy_size;

		if (state->block_next == state->block_end) {
			state->state = state->has_eob_hdr ? ZSTATE_TRL : ZSTATE_NEW_HDR;
			if (stream->flush == FULL_FLUSH && state->state == ZSTATE_NEW_HDR
			    && are_buffers_empty(stream)) {
				/* Clear match history so there are no cross
				 * block length distance pairs */
				reset_match_history(stream);
			}
		}
	} while (state->state == ZSTATE_TYPE0_HDR);

	return state->block_end - state->block_next;
}

static inline void reset_match_history(struct isal_zstream *stream)
{
	struct isal_zstate *state = &stream->internal_state;
	struct level_buf *level_buf = (struct level_buf *)stream->level_buf;
	uint16_t *hash_table;
	uint32_t hash_table_size;
	int i = 0;

	switch (stream->level) {
	case 3:
		hash_table = level_buf->lvl3.hash_table;
		hash_table_size = sizeof(level_buf->lvl3.hash_table);
		break;

	case 2:
		hash_table = level_buf->lvl2.hash_table;
		hash_table_size = sizeof(level_buf->lvl2.hash_table);
		break;
	case 1:
		hash_table = level_buf->lvl1.hash_table;
		hash_table_size = sizeof(level_buf->lvl1.hash_table);
		break;
	default:
		hash_table = state->head;
		hash_table_size = sizeof(state->head);
	}

	state->has_hist = IGZIP_NO_HIST;

	if ((stream->total_in & 0xFFFF) == 0)
		memset(hash_table, 0, hash_table_size);
	else {
		for (i = 0; i < hash_table_size / 2; i++) {
			hash_table[i] = (uint16_t) (stream->total_in);
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

	state->block_next = 0;
	state->block_end = 0;
	state->b_bytes_valid = 0;
	state->b_bytes_processed = 0;
	state->total_in_start = 0;
	state->has_wrap_hdr = 0;
	state->has_eob = 0;
	state->has_eob_hdr = 0;
	state->has_hist = IGZIP_NO_HIST;
	state->has_level_buf_init = 0;
	state->state = ZSTATE_NEW_HDR;
	state->count = 0;

	state->tmp_out_start = 0;
	state->tmp_out_end = 0;

	init(&state->bitbuf);

	state->crc = 0;

	return;
}

void isal_deflate_reset(struct isal_zstream *stream)
{
	struct isal_zstate *state = &stream->internal_state;

	stream->total_in = 0;
	stream->total_out = 0;

	state->block_next = 0;
	state->block_end = 0;
	state->b_bytes_valid = 0;
	state->b_bytes_processed = 0;
	state->total_in_start = 0;
	state->has_wrap_hdr = 0;
	state->has_eob = 0;
	state->has_level_buf_init = 0;
	state->has_eob_hdr = 0;
	state->has_hist = IGZIP_NO_HIST;
	state->state = ZSTATE_NEW_HDR;
	state->count = 0;

	state->tmp_out_start = 0;
	state->tmp_out_end = 0;

	init(&state->bitbuf);

	state->crc = 0;

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
	stream->internal_state.has_wrap_hdr = 0;
	stream->internal_state.state = ZSTATE_NEW_HDR;
	return;
}

void isal_deflate_hash(struct isal_zstream *stream, uint8_t * dict, uint32_t dict_len)
{
	/* Reset history to prevent out of bounds matches this works because
	 * dictionary must set at least 1 element in the history */
	struct level_buf *level_buf = (struct level_buf *)stream->level_buf;
	switch (stream->level) {
	case 3:
		memset(level_buf->lvl3.hash_table, -1, sizeof(level_buf->lvl3.hash_table));
		isal_deflate_hash_lvl3(level_buf->lvl3.hash_table, LVL3_HASH_MASK,
				       stream->total_in, dict, dict_len);
		break;

	case 2:
		memset(level_buf->lvl2.hash_table, -1, sizeof(level_buf->lvl2.hash_table));
		isal_deflate_hash_lvl2(level_buf->lvl2.hash_table, LVL2_HASH_MASK,
				       stream->total_in, dict, dict_len);
		break;
	case 1:
		memset(level_buf->lvl1.hash_table, -1, sizeof(level_buf->lvl1.hash_table));
		isal_deflate_hash_lvl1(level_buf->lvl1.hash_table, LVL1_HASH_MASK,
				       stream->total_in, dict, dict_len);
		break;
	default:
		memset(stream->internal_state.head, -1, sizeof(stream->internal_state.head));
		isal_deflate_hash_lvl0(stream->internal_state.head, LVL0_HASH_MASK,
				       stream->total_in, dict, dict_len);
	}

	stream->internal_state.has_hist = IGZIP_HIST;
}

int isal_deflate_set_dict(struct isal_zstream *stream, uint8_t * dict, uint32_t dict_len)
{
	struct isal_zstate *state = &stream->internal_state;

	if (state->state != ZSTATE_NEW_HDR || state->b_bytes_processed != state->b_bytes_valid)
		return ISAL_INVALID_STATE;

	if (dict_len <= 0)
		return COMP_OK;

	if (dict_len > IGZIP_HIST_SIZE) {
		dict = dict + dict_len - IGZIP_HIST_SIZE;
		dict_len = IGZIP_HIST_SIZE;
	}

	memcpy(state->buffer, dict, dict_len);
	state->b_bytes_processed = dict_len;
	state->b_bytes_valid = dict_len;

	state->has_hist = IGZIP_DICT_HIST;

	return COMP_OK;
}

int isal_deflate_stateless(struct isal_zstream *stream)
{
	struct isal_zstate *state = &stream->internal_state;
	uint8_t *next_in = stream->next_in;
	const uint32_t avail_in = stream->avail_in;
	const uint32_t total_in = stream->total_in;

	uint8_t *next_out = stream->next_out;
	const uint32_t avail_out = stream->avail_out;
	const uint32_t total_out = stream->total_out;
	const uint32_t gzip_flag = stream->gzip_flag;
	const uint32_t has_wrap_hdr = state->has_wrap_hdr;

	int level_check;
	uint32_t stored_len;

	/* Final block has already been written */
	state->total_in_start = stream->total_in;
	state->block_next = stream->total_in;
	state->block_end = stream->total_in;
	state->has_eob_hdr = 0;
	init(&state->bitbuf);
	state->state = ZSTATE_NEW_HDR;
	state->crc = 0;
	state->has_level_buf_init = 0;

	if (stream->flush == NO_FLUSH)
		stream->end_of_stream = 1;

	if (stream->flush != NO_FLUSH && stream->flush != FULL_FLUSH)
		return INVALID_FLUSH;

	level_check = check_level_req(stream);
	if (level_check) {
		if (stream->level == 1 && stream->level_buf == NULL) {
			/* Default to internal buffer if invalid size is supplied */
			stream->level_buf = state->buffer;
			stream->level_buf_size = sizeof(state->buffer) + sizeof(state->head);
		} else
			return level_check;
	}

	if (avail_in == 0)
		stored_len = TYPE0_BLK_HDR_LEN;
	else
		stored_len =
		    TYPE0_BLK_HDR_LEN * ((avail_in + TYPE0_MAX_BLK_LEN - 1) /
					 TYPE0_MAX_BLK_LEN) + avail_in;

	/*
	   at least 1 byte compressed data in the case of empty dynamic block which only
	   contains the EOB
	 */
	if (stream->gzip_flag == IGZIP_GZIP)
		stored_len += gzip_hdr_bytes + gzip_trl_bytes;
	else if (stream->gzip_flag == IGZIP_GZIP_NO_HDR)
		stored_len += gzip_trl_bytes;

	else if (stream->gzip_flag == IGZIP_ZLIB)
		stored_len += zlib_hdr_bytes + zlib_trl_bytes;

	else if (stream->gzip_flag == IGZIP_ZLIB_NO_HDR)
		stored_len += zlib_trl_bytes;

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
			reset_match_history(stream);
		}
		stream->internal_state.has_eob_hdr = 0;
	}

	if (avail_out < stored_len)
		return STATELESS_OVERFLOW;

	stream->next_in = next_in + avail_in;
	stream->avail_in = 0;
	stream->total_in = total_in + avail_in;

	state->block_next = 0;
	state->block_end = avail_in;

	stream->next_out = next_out;
	stream->avail_out = avail_out;
	stream->total_out = total_out;

	stream->gzip_flag = gzip_flag;
	state->has_wrap_hdr = has_wrap_hdr;
	init(&stream->internal_state.bitbuf);
	stream->internal_state.count = 0;

	if (stream->gzip_flag == IGZIP_GZIP || stream->gzip_flag == IGZIP_ZLIB)
		write_stream_header_stateless(stream);

	stream->internal_state.state = ZSTATE_TYPE0_HDR;

	write_stored_block(stream);

	if (stream->gzip_flag) {
		stream->internal_state.crc = 0;
		update_checksum(stream, next_in, avail_in);
	}

	write_trailer(stream);
	return COMP_OK;

}

static inline uint32_t hist_add(struct isal_zstream *stream, uint32_t history_size,
				uint32_t add_size)
{
	struct isal_zstate *state = &stream->internal_state;

	/* Calculate requried match History */
	history_size += add_size;
	if (history_size > IGZIP_HIST_SIZE)
		history_size = IGZIP_HIST_SIZE;

	/* Calculate required block history */
	if (state->state == ZSTATE_TYPE0_HDR
	    || state->state == ZSTATE_TYPE0_BODY
	    || state->state == ZSTATE_TMP_TYPE0_HDR || state->state == ZSTATE_TMP_TYPE0_BODY) {
		if (stream->total_in - state->block_next > history_size)
			history_size = (stream->total_in - state->block_next);
	}

	return history_size;
}

int isal_deflate(struct isal_zstream *stream)
{
	struct isal_zstate *state = &stream->internal_state;
	int ret = COMP_OK;
	uint8_t *next_in;
	uint32_t avail_in, avail_in_start, total_start, hist_size, future_size;
	uint32_t flush_type = stream->flush;
	uint32_t end_of_stream = stream->end_of_stream;
	uint32_t size = 0;
	uint8_t *copy_down_src = NULL;
	uint64_t copy_down_size = 0, copy_start_offset;

	if (stream->flush >= 3)
		return INVALID_FLUSH;

	ret = check_level_req(stream);
	if (ret)
		return ret;

	next_in = stream->next_in;
	avail_in = stream->avail_in;
	total_start = stream->total_in;
	stream->total_in -= state->b_bytes_valid - state->b_bytes_processed;

	hist_size = hist_add(stream, state->b_bytes_processed, 0);
	if (state->has_hist == IGZIP_NO_HIST) {
		reset_match_history(stream);
		hist_size = 0;
	} else if (state->has_hist == IGZIP_DICT_HIST)
		isal_deflate_hash(stream, state->buffer, state->b_bytes_processed);

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

		if (stream->avail_in > IGZIP_HIST_SIZE
		    || stream->total_in - state->block_next > IGZIP_HIST_SIZE
		    || stream->end_of_stream || stream->flush != NO_FLUSH) {
			avail_in_start = stream->avail_in;
			state->total_in_start = stream->total_in - state->b_bytes_processed;
			isal_deflate_int(stream);
			state->b_bytes_processed += avail_in_start - stream->avail_in;
			hist_size =
			    hist_add(stream, hist_size, avail_in_start - stream->avail_in);

			if (state->b_bytes_processed > hist_size) {
				copy_start_offset = state->b_bytes_processed - hist_size;

				copy_down_src = &state->buffer[copy_start_offset];
				copy_down_size = state->b_bytes_valid - copy_start_offset;
				memmove(state->buffer, copy_down_src, copy_down_size);

				state->b_bytes_valid -= copy_down_src - state->buffer;
				state->b_bytes_processed -= copy_down_src - state->buffer;
			}
		}

		stream->flush = flush_type;
		stream->end_of_stream = end_of_stream;
	} while (stream->total_in < total_start + hist_size && avail_in > 0
		 && stream->avail_out > 0);

	stream->total_in += state->b_bytes_valid - state->b_bytes_processed;
	stream->next_in = next_in;
	stream->avail_in = avail_in;

	if (stream->avail_in > 0 && stream->avail_out > 0) {
		/* Due to exiting conditions for the while loop, we know that
		 * stream->total_in < total_start + hist_size */
		stream->next_in -= state->b_bytes_valid - state->b_bytes_processed;
		stream->avail_in += state->b_bytes_valid - state->b_bytes_processed;
		stream->total_in -= state->b_bytes_valid - state->b_bytes_processed;

		avail_in_start = stream->avail_in;
		state->total_in_start = total_start;
		isal_deflate_int(stream);

		hist_size = hist_add(stream, hist_size, avail_in_start - stream->avail_in);
		future_size = stream->avail_in;
		if (future_size > ISAL_LOOK_AHEAD)
			future_size = ISAL_LOOK_AHEAD;

		memmove(state->buffer, stream->next_in - hist_size, hist_size + future_size);
		state->b_bytes_processed = hist_size;
		state->b_bytes_valid = hist_size + future_size;
		stream->next_in += future_size;
		stream->total_in += future_size;
		stream->avail_in -= future_size;
	}

	return ret;
}

static int write_stream_header_stateless(struct isal_zstream *stream)
{
	uint32_t hdr_bytes;
	const uint8_t *hdr;
	uint32_t next_flag;

	if (stream->internal_state.has_wrap_hdr)
		return COMP_OK;

	if (stream->gzip_flag == IGZIP_ZLIB) {
		hdr_bytes = zlib_hdr_bytes;
		hdr = zlib_hdr;
		next_flag = IGZIP_ZLIB_NO_HDR;

	} else {
		hdr_bytes = gzip_hdr_bytes;
		hdr = gzip_hdr;
		next_flag = IGZIP_GZIP_NO_HDR;
	}

	if (hdr_bytes >= stream->avail_out)
		return STATELESS_OVERFLOW;

	stream->avail_out -= hdr_bytes;
	stream->total_out += hdr_bytes;

	memcpy(stream->next_out, hdr, hdr_bytes);

	stream->next_out += hdr_bytes;
	stream->internal_state.has_wrap_hdr = 1;
	stream->gzip_flag = next_flag;

	return COMP_OK;
}

static void write_stream_header(struct isal_zstream *stream)
{
	struct isal_zstate *state = &stream->internal_state;
	int bytes_to_write;
	uint32_t hdr_bytes;
	const uint8_t *hdr;

	if (stream->internal_state.has_wrap_hdr)
		return;

	if (stream->gzip_flag == IGZIP_ZLIB) {
		hdr_bytes = zlib_hdr_bytes;
		hdr = zlib_hdr;
	} else {
		hdr_bytes = gzip_hdr_bytes;
		hdr = gzip_hdr;
	}

	bytes_to_write = hdr_bytes;
	bytes_to_write -= state->count;

	if (bytes_to_write > stream->avail_out)
		bytes_to_write = stream->avail_out;

	memcpy(stream->next_out, hdr + state->count, bytes_to_write);
	state->count += bytes_to_write;

	if (state->count == hdr_bytes) {
		state->count = 0;
		state->has_wrap_hdr = 1;
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
static void write_header(struct isal_zstream *stream, uint8_t * deflate_hdr,
			 uint32_t deflate_hdr_count, uint32_t extra_bits_count,
			 uint32_t next_state, uint32_t toggle_end_of_stream)
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

	if (stream->gzip_flag == IGZIP_GZIP || stream->gzip_flag == IGZIP_ZLIB)
		write_stream_header(stream);

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

static void write_trailer(struct isal_zstream *stream)
{
	struct isal_zstate *state = &stream->internal_state;
	unsigned int bytes = 0;
	uint32_t crc = state->crc;

	set_buf(&state->bitbuf, stream->next_out, stream->avail_out);

	if (!state->has_eob_hdr) {
		/* If the final header has not been written, write a
		 * final block. This block is a static huffman block
		 * which only contains the end of block symbol. The code
		 * that happens to do this is the fist 10 bits of
		 * 0x003 */
		if (stream->avail_out < 8)
			return;

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

	if (state->bitbuf.m_bit_count) {
		/* the flush() will pad to the next byte and write up to 8 bytes
		 * to the output stream/buffer.
		 */
		if (stream->avail_out < 8)
			return;

		flush(&state->bitbuf);
	}

	stream->next_out = buffer_ptr(&state->bitbuf);
	bytes = buffer_used(&state->bitbuf);

	switch (stream->gzip_flag) {
	case IGZIP_GZIP:
	case IGZIP_GZIP_NO_HDR:
		if (stream->avail_out - bytes >= gzip_trl_bytes) {
			*(uint64_t *) stream->next_out =
			    ((uint64_t) stream->total_in << 32) | crc;
			stream->next_out += gzip_trl_bytes;
			bytes += gzip_trl_bytes;
			state->state = ZSTATE_END;
		}
		break;

	case IGZIP_ZLIB:
	case IGZIP_ZLIB_NO_HDR:
		if (stream->avail_out - bytes >= zlib_trl_bytes) {
			*(uint32_t *) stream->next_out =
			    to_be32((crc & 0xFFFF0000) | ((crc & 0xFFFF) + 1) % ADLER_MOD);
			stream->next_out += zlib_trl_bytes;
			bytes += zlib_trl_bytes;
			state->state = ZSTATE_END;
		}
		break;

	default:
		state->state = ZSTATE_END;
	}

	stream->avail_out -= bytes;
	stream->total_out += bytes;
}
