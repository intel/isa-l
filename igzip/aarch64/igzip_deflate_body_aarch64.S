/**********************************************************************
  Copyright(c) 2019 Arm Corporation All rights reserved.

  Redistribution and use in source and binary forms, with or without
  modification, are permitted provided that the following conditions
  are met:
    * Redistributions of source code must retain the above copyright
      notice, this list of conditions and the following disclaimer.
    * Redistributions in binary form must reproduce the above copyright
      notice, this list of conditions and the following disclaimer in
      the documentation and/or other materials provided with the
      distribution.
    * Neither the name of Arm Corporation nor the names of its
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

#include "../include/aarch64_label.h"

	.arch armv8-a+crc
	.text
	.align	2
#include "lz0a_const_aarch64.h"
#include "data_struct_aarch64.h"
#include "huffman_aarch64.h"
#include "bitbuf2_aarch64.h"
#include "stdmac_aarch64.h"
/*
declare Macros
*/

.macro	declare_generic_reg name:req,reg:req,default:req
	\name		.req	\default\reg
	w_\name		.req	w\reg
	x_\name		.req	x\reg
.endm

	.global	cdecl(isal_deflate_body_aarch64)
#ifndef __APPLE__
	.type	isal_deflate_body_aarch64, %function
#endif
/*
	void isal_deflate_body_aarch64(struct isal_zstream *stream)

	The main loop keeps the bit buffer in registers and writes it back to
	stream->internal_state.bitbuf on exit.
*/
	declare_generic_reg	stream,		0,x
	declare_generic_reg	hufftables,	1,x
	declare_generic_reg	bits,		2,x
	declare_generic_reg	bit_count,	3,x
	declare_generic_reg	out_buf,	4,x
	declare_generic_reg	out_end,	5,x
	declare_generic_reg	out_start,	6,x
	declare_generic_reg	start_in,	7,x
	declare_generic_reg	end_in,		8,x
	declare_generic_reg	next_in,	9,x
	declare_generic_reg	end_loop,	10,x
	declare_generic_reg	last_seen,	11,x
	declare_generic_reg	file_start,	12,x
	declare_generic_reg	hist_size,	13,w
	declare_generic_reg	hash_mask,	14,w
	declare_generic_reg	lit_table,	15,x
	declare_generic_reg	lit_sizes,	16,x
	declare_generic_reg	literal,	17,x
	declare_generic_reg	hash,		19,x
	declare_generic_reg	pos,		20,x
	declare_generic_reg	dist,		21,x
	declare_generic_reg	match_src,	22,x
	declare_generic_reg	match_len,	23,x
	declare_generic_reg	code,		24,x
	declare_generic_reg	code_len,	25,x
	declare_generic_reg	tmp0,		26,x
	declare_generic_reg	tmp1,		27,x
	declare_generic_reg	tmp2,		28,x
	declare_generic_reg	tmp3,		30,x

/* write_bits(&state->bitbuf, code, code_len) */
.macro write_code
	lsl	code, code, bit_count
	add	bit_count, bit_count, code_len
	orr	bits, bits, code
	and	tmp0, bit_count, -8
	str	bits, [out_buf]
	add	out_buf, out_buf, bit_count, lsr 3
	lsr	bits, bits, tmp0
	and	bit_count, bit_count, 7
.endm

cdecl(isal_deflate_body_aarch64):
	push_stack
	ldr	w_tmp0, [stream, _avail_in]
	cbz	w_tmp0, .exit_save_state

	/* set_buf(&state->bitbuf, stream->next_out, stream->avail_out) */
	ldr	w_out_end, [stream, _avail_out]
	ldr	out_buf, [stream, _next_out]
	add	out_end, out_buf, out_end
	sub	out_end, out_end, 8
	mov	out_start, out_buf
	stp	out_buf, out_end, [stream, _internal_state + _bitbuf_m_out_buf]
	str	out_start, [stream, _internal_state + _bitbuf_m_out_start]
	ldr	w_bit_count, [stream, _internal_state_bitbuf_m_bit_count]
	ldr	bits, [stream, _internal_state_bitbuf_m_bits]

	/* last_seen = stream->internal_state.head */
	add	last_seen, stream, 65536
	add	last_seen, last_seen, _internal_state + _head - 65536

	ldr	start_in, [stream, _next_in]
	mov	next_in, start_in
	add	end_in, start_in, tmp0
	sub	end_loop, end_in, 289		// ISAL_LOOK_AHEAD + 1
	ldr	w_tmp1, [stream, _total_in]
	sub	file_start, next_in, tmp1
	ldr	hist_size, [stream, _internal_state_dist_mask]
	ldr	hash_mask, [stream, _internal_state_hash_mask]
	ldr	hufftables, [stream, _hufftables]
	add	lit_table, hufftables, LIT_TABLE_OFFSET
	add	lit_sizes, hufftables, LIT_TABLE_SIZES_OFFSET

	/* while (next_in + ISAL_LOOK_AHEAD < end_in), ISAL_LOOK_AHEAD = 288 */
	cmp	next_in, end_loop
	bhi	.loop_end

	.align	5
.loop:
	/* is_full(&state->bitbuf) */
	cmp	out_buf, out_end
	bhi	.update_state_exit

	ldr	literal, [next_in]
	sub	pos, next_in, file_start
	crc32cw	w_hash, wzr, w_literal
	and	w_hash, w_hash, hash_mask
	ldrh	w_dist, [last_seen, hash, lsl 1]
	strh	w_pos, [last_seen, hash, lsl 1]
	sub	w_dist, w_pos, w_dist
	and	w_dist, w_dist, 0xffff

	/*
		Compare the first 4 bytes against the candidate. If the distance is
		out of range (the -1 handles dist = 0), load from next_in instead to
		keep the load in bounds, and make the comparison fail. This leaves
		one data-dependent branch per position, and it resolves as soon as
		the candidate loads.
	*/
	sub	w_tmp0, w_dist, 1
	cmp	w_tmp0, hist_size
	sub	match_src, next_in, dist
	csel	match_src, match_src, next_in, lo
	ldr	tmp1, [match_src]
	ccmp	w_literal, w_tmp1, 0, lo
	beq	.check_match

.literal:
	/* get_lit_code(stream->hufftables, literal & 0xFF, &code, &code_len) */
	and	w_literal, w_literal, 0xff
	ldrh	w_code, [lit_table, literal, lsl 1]
	ldrb	w_code_len, [lit_sizes, literal]
	add	next_in, next_in, 1
	write_code

.loop_check:
	cmp	next_in, end_loop
	bls	.loop
	b	.loop_end

.check_match:
	/* At least 4 bytes match. Count the matching bytes of the first 8. */
	eor	tmp0, literal, tmp1
	rbit	tmp0, tmp0
	clz	tmp0, tmp0
	lsr	w_match_len, w_tmp0, 3
	/* All of the first 8 bytes match; keep comparing */
	cmp	w_match_len, 8
	beq	.long_match

.match:
	/* Hash the next two positions inside the match */
	ldur	w_tmp0, [next_in, 1]
	ldur	w_tmp1, [next_in, 2]
	add	w_tmp2, w_pos, 1
	add	w_tmp3, w_pos, 2
	crc32cw	w_tmp0, wzr, w_tmp0
	crc32cw	w_tmp1, wzr, w_tmp1
	and	w_tmp0, w_tmp0, hash_mask
	and	w_tmp1, w_tmp1, hash_mask
	strh	w_tmp2, [last_seen, tmp0, lsl 1]
	strh	w_tmp3, [last_seen, tmp1, lsl 1]

	/* get_len_code(stream->hufftables, match_length, &code, &code_len) */
	add	tmp0, hufftables, LEN_TABLE_OFFSET
	ldr	w_code, [tmp0, match_len, lsl 2]

	/* get_dist_code(stream->hufftables, dist, &code2, &code_len2) */
	cmp	w_dist, DIST_TABLE_SIZE
	bls	.small_dist
	sub	w_tmp0, w_dist, 1
	clz	w_tmp1, w_tmp0
	mov	w_tmp2, 30
	sub	w_tmp1, w_tmp2, w_tmp1
	mov	w_tmp2, -1
	lsl	w_tmp2, w_tmp2, w_tmp1
	bic	w_tmp3, w_tmp0, w_tmp2
	lsr	w_tmp0, w_tmp0, w_tmp1
	add	w_tmp0, w_tmp0, w_tmp1, lsl 1
	add	tmp2, hufftables, DCODE_TABLE_OFFSET
	ldrh	w_tmp2, [tmp2, tmp0, lsl 1]
	add	hash, hufftables, DCODE_TABLE_SIZE_OFFSET
	ldrb	w_hash, [hash, tmp0]
	lsl	w_tmp3, w_tmp3, w_hash
	orr	w_tmp2, w_tmp2, w_tmp3
	add	w_hash, w_hash, w_tmp1

.dist_code_done:
	/* code |= code2 << code_len; code_len += code_len2 */
	and	w_code_len, w_code, 0x1f
	lsr	w_code, w_code, 5
	lsl	tmp2, tmp2, code_len
	orr	code, code, tmp2
	add	code_len, code_len, hash
	add	next_in, next_in, match_len
	write_code
	b	.loop_check

.small_dist:
	add	tmp0, hufftables, DIST_TABLE_OFFSET
	ldr	w_tmp2, [tmp0, dist, lsl 2]
	and	w_hash, w_tmp2, 0x1f
	lsr	w_tmp2, w_tmp2, 5
	b	.dist_code_done

	/*
		The first 8 bytes match. Compare 16 bytes per iteration from offset
		8. The lookahead guarantees 288 readable bytes past next_in.
	*/
.long_match:
	add	tmp0, next_in, 8
	add	tmp1, match_src, 8
	add	tmp2, next_in, 258
.long_match_loop:
	ldp	tmp3, code, [tmp0], 16
	ldp	code_len, hash, [tmp1], 16
	eor	tmp3, tmp3, code_len
	eor	code, code, hash
	orr	code_len, tmp3, code
	cbnz	code_len, .long_match_diff
	cmp	tmp0, tmp2
	blo	.long_match_loop
	mov	w_match_len, 258
	b	.match

.long_match_diff:
	/* tmp0 points 16 bytes past the start of the chunk with the mismatch */
	sub	tmp0, tmp0, next_in
	sub	tmp1, tmp0, 16
	sub	tmp0, tmp0, 8
	cmp	tmp3, 0
	csel	tmp3, tmp3, code, ne
	csel	tmp0, tmp1, tmp0, ne
	rbit	tmp3, tmp3
	clz	tmp3, tmp3
	add	match_len, tmp0, tmp3, lsr 3
	mov	tmp1, 258
	cmp	match_len, tmp1
	csel	match_len, match_len, tmp1, ls
	b	.match

.loop_end:
	/* Load end_of_stream and flush together */
	ldr	w_tmp0, [stream, _end_of_stream]
	cbz	w_tmp0, .update_state_exit
	mov	w_tmp0, ZSTATE_FLUSH_READ_BUFFER
	str	w_tmp0, [stream, _internal_state_state]

.update_state_exit:
	str	bits, [stream, _internal_state_bitbuf_m_bits]
	str	w_bit_count, [stream, _internal_state_bitbuf_m_bit_count]
	str	out_buf, [stream, _internal_state + _bitbuf_m_out_buf]

	/* update_state() */
	sub	tmp0, out_buf, out_start
	cmp	next_in, start_in
	bls	1f
	mov	w_tmp1, 1
	strb	w_tmp1, [stream, _internal_state_has_hist]
1:
	ldr	w_tmp1, [stream, _total_in]
	ldr	tmp2, [stream, _next_out]
	str	next_in, [stream, _next_in]
	sub	tmp3, next_in, start_in
	sub	end_in, end_in, next_in
	add	w_tmp1, w_tmp1, w_tmp3
	stp	w_end_in, w_tmp1, [stream, _avail_in]
	ldp	w_tmp1, w_tmp3, [stream, _avail_out]
	add	tmp2, tmp2, tmp0
	str	tmp2, [stream, _next_out]
	sub	w_tmp1, w_tmp1, w_tmp0
	add	w_tmp3, w_tmp3, w_tmp0
	stp	w_tmp1, w_tmp3, [stream, _avail_out]
.exit_ret:
	pop_stack
	ret

.exit_save_state:
	/* (stream->end_of_stream || stream->flush != 0) */
	ldr	w_tmp0, [stream, _end_of_stream]
	cbz	w_tmp0, .exit_ret
	mov	w_tmp0, ZSTATE_FLUSH_READ_BUFFER
	str	w_tmp0, [stream, _internal_state_state]
	b	.exit_ret
#ifndef __APPLE__
	.size	isal_deflate_body_aarch64, .-isal_deflate_body_aarch64
#endif
