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

/*
declare Macros
*/
.macro	declare_generic_reg name:req,reg:req,default:req
	\name		.req	\default\reg
	w_\name		.req	w\reg
	x_\name		.req	x\reg
.endm

	.global	cdecl(isal_deflate_icf_body_hash_hist_aarch64)
#ifndef __APPLE__
	.type	isal_deflate_icf_body_hash_hist_aarch64, %function
#endif
/*
void isal_deflate_icf_body_hash_hist_base(struct isal_zstream *stream);
*/

/* constant */

/* offset of struct isal_zstream */
.equ	offset_next_in, 0
.equ	offset_avail_in, 8
.equ	offset_total_in, 12
.equ	offset_level_buf, 48
.equ	offset_end_of_stream, 56
.equ	offset_state, 64
.equ	offset_state_block_end, 72
.equ	offset_state_dist_mask, 76
.equ	offset_state_state, 84
.equ	offset_state_has_hist, 135

/* offset of struct level_buf */
.equ	offset_hist_d_hist, 2176
.equ	offset_hist_ll_hist, 2296
.equ	offset_icf_buf_next, 4688
.equ	offset_icf_buf_avail_out, 4696
.equ	offset_hash_hist, 4712

/* enum isal_zstate_state */
.equ	ZSTATE_CREATE_HDR, 2
.equ	ZSTATE_FLUSH_READ_BUFFER, 4

.equ	ISAL_LOOK_AHEAD, 288
.equ	MAX_MATCH, 258
.equ	LEN_OFFSET, 254
.equ	NULL_DIST_SYM, 30
.equ	LIT_ICF_DIST, (NULL_DIST_SYM << 10)

	/* arguments */
	declare_generic_reg	stream,		0,x

	/* variables kept for the whole function */
	declare_generic_reg	stream_saved,	19,x
	declare_generic_reg	level_buf,	20,x
	declare_generic_reg	next_in,	21,x
	declare_generic_reg	end_loop,	22,x
	declare_generic_reg	next_out,	23,x
	declare_generic_reg	end_out,	24,x
	declare_generic_reg	file_start,	25,x
	declare_generic_reg	last_seen,	26,x
	declare_generic_reg	hist_size,	27,w
	declare_generic_reg	hash_mask,	28,w
	declare_generic_reg	ll_hist,	9,x
	declare_generic_reg	d_hist,		10,x
	declare_generic_reg	start_in,	11,x
	declare_generic_reg	end_in,		12,x
	declare_generic_reg	total_in,	13,w

	/* per iteration */
	declare_generic_reg	literal,	1,x
	declare_generic_reg	hash,		2,x
	declare_generic_reg	pos,		3,x
	declare_generic_reg	dist,		4,x
	declare_generic_reg	match_src,	5,x
	declare_generic_reg	match_len,	6,x
	declare_generic_reg	tmp0,		7,x
	declare_generic_reg	tmp1,		8,x
	declare_generic_reg	tmp2,		14,x
	declare_generic_reg	tmp3,		15,x
	declare_generic_reg	tmp4,		16,x
	declare_generic_reg	tmp5,		17,x

/*
	Write back the stream and level_buf state like update_state() in
	igzip_icf_base.c. With check_flush set, also move to
	ZSTATE_FLUSH_READ_BUFFER when end_of_stream or flush is set.
*/
.macro update_state check_flush:req
	sub	tmp0, next_in, start_in
	cmp	tmp0, 0
	ble	1f
	mov	w_tmp1, 1
	strb	w_tmp1, [stream_saved, offset_state_has_hist]
1:
	add	total_in, total_in, w_tmp0
	str	next_in, [stream_saved, offset_next_in]
	str	total_in, [stream_saved, offset_total_in]
	str	total_in, [stream_saved, offset_state_block_end]
	sub	tmp1, end_in, next_in
	str	w_tmp1, [stream_saved, offset_avail_in]
	str	next_out, [level_buf, offset_icf_buf_next]
	sub	tmp1, end_out, next_out
	str	tmp1, [level_buf, offset_icf_buf_avail_out]
.if \check_flush
	ldr	w_tmp1, [stream_saved, offset_end_of_stream]
	cbz	w_tmp1, 2f
	mov	w_tmp1, ZSTATE_FLUSH_READ_BUFFER
	str	w_tmp1, [stream_saved, offset_state_state]
2:
.endif
.endm

cdecl(isal_deflate_icf_body_hash_hist_aarch64):
	ldr	w1, [stream, offset_avail_in]
	cbnz	w1, .stream_available

	/* end_of_stream and flush are read together */
	ldr	w1, [stream, offset_end_of_stream]
	cbz	w1, .done_no_restore
	mov	w1, ZSTATE_FLUSH_READ_BUFFER
	str	w1, [stream, offset_state_state]
.done_no_restore:
	ret

.stream_available:
	stp	x29, x30, [sp, -96]!
	mov	x29, sp
	stp	x19, x20, [sp, 16]
	stp	x21, x22, [sp, 32]
	stp	x23, x24, [sp, 48]
	stp	x25, x26, [sp, 64]
	stp	x27, x28, [sp, 80]

	mov	stream_saved, stream
	ldr	level_buf, [stream, offset_level_buf]
	ldr	start_in, [stream, offset_next_in]
	ldr	total_in, [stream, offset_total_in]
	ldp	hist_size, hash_mask, [stream, offset_state_dist_mask]

	mov	tmp0, offset_hash_hist
	add	last_seen, level_buf, tmp0
	add	ll_hist, level_buf, offset_hist_ll_hist
	add	d_hist, level_buf, offset_hist_d_hist

	ldr	next_out, [level_buf, offset_icf_buf_next]
	ldr	tmp0, [level_buf, offset_icf_buf_avail_out]
	and	tmp0, tmp0, -4
	add	end_out, next_out, tmp0

	mov	next_in, start_in
	add	end_in, start_in, x1
	sub	end_loop, end_in, ISAL_LOOK_AHEAD
	sub	file_start, start_in, x_total_in

	/* while (next_in + ISAL_LOOK_AHEAD < end_in) */
	cmp	next_in, end_loop
	bhs	.loop_end
	cmp	next_out, end_out
	bhs	.create_hdr

	.align	5
.loop:
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
	and	w_literal, w_literal, 0xff
	ldr	w_tmp0, [ll_hist, literal, lsl 2]
	orr	w_tmp1, w_literal, LIT_ICF_DIST
	add	next_in, next_in, 1
	add	w_tmp0, w_tmp0, 1
	str	w_tmp1, [next_out], 4
	str	w_tmp0, [ll_hist, literal, lsl 2]

.loop_check:
	cmp	next_in, end_loop
	bhs	.loop_end
	cmp	next_out, end_out
	blo	.loop
	b	.create_hdr

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

	/*
		Distance code for dist - 1 = d: with n = max(30 - clz(d), 0)
		extra bits, code = (d >> n) + 2 * n and extra = d & ((1 << n) - 1).
	*/
	sub	w_tmp0, w_dist, 1
	clz	w_tmp1, w_tmp0
	mov	w_tmp2, 30
	subs	w_tmp1, w_tmp2, w_tmp1
	csel	w_tmp1, w_tmp1, wzr, gt
	mov	w_tmp2, -1
	lsl	w_tmp2, w_tmp2, w_tmp1
	bic	w_tmp3, w_tmp0, w_tmp2
	lsr	w_tmp0, w_tmp0, w_tmp1
	add	w_tmp0, w_tmp0, w_tmp1, lsl 1

	/* Histograms */
	add	w_tmp1, w_match_len, LEN_OFFSET
	ldr	w_tmp4, [ll_hist, tmp1, lsl 2]
	ldr	w_tmp5, [d_hist, tmp0, lsl 2]
	add	w_tmp4, w_tmp4, 1
	add	w_tmp5, w_tmp5, 1
	str	w_tmp4, [ll_hist, tmp1, lsl 2]
	str	w_tmp5, [d_hist, tmp0, lsl 2]

	/* write_deflate_icf(next_out, len + LEN_OFFSET, dist code, extra) */
	orr	w_tmp1, w_tmp1, w_tmp0, lsl 10
	orr	w_tmp1, w_tmp1, w_tmp3, lsl 19
	str	w_tmp1, [next_out], 4
	add	next_in, next_in, match_len
	b	.loop_check

	/*
		The first 8 bytes match. Compare 16 bytes per iteration from offset
		8. The lookahead guarantees 288 readable bytes past next_in.
	*/
.long_match:
	add	tmp0, next_in, 8
	add	tmp1, match_src, 8
	add	tmp2, next_in, MAX_MATCH
.long_match_loop:
	ldp	tmp3, tmp4, [tmp0], 16
	ldp	tmp5, match_len, [tmp1], 16
	eor	tmp3, tmp3, tmp5
	eor	tmp4, tmp4, match_len
	orr	tmp5, tmp3, tmp4
	cbnz	tmp5, .long_match_diff
	cmp	tmp0, tmp2
	blo	.long_match_loop
	mov	w_match_len, MAX_MATCH
	b	.match

.long_match_diff:
	/* tmp0 points 16 bytes past the start of the chunk with the mismatch */
	sub	tmp0, tmp0, next_in
	sub	tmp1, tmp0, 16
	sub	tmp0, tmp0, 8
	cmp	tmp3, 0
	csel	tmp3, tmp3, tmp4, ne
	csel	tmp0, tmp1, tmp0, ne
	rbit	tmp3, tmp3
	clz	tmp3, tmp3
	add	match_len, tmp0, tmp3, lsr 3
	mov	tmp1, MAX_MATCH
	cmp	match_len, tmp1
	csel	match_len, match_len, tmp1, ls
	b	.match

.create_hdr:
	mov	w_tmp0, ZSTATE_CREATE_HDR
	str	w_tmp0, [stream_saved, offset_state_state]
	update_state	0
	b	.done

.loop_end:
	update_state	1

.done:
	ldp	x19, x20, [sp, 16]
	ldp	x21, x22, [sp, 32]
	ldp	x23, x24, [sp, 48]
	ldp	x25, x26, [sp, 64]
	ldp	x27, x28, [sp, 80]
	ldp	x29, x30, [sp], 96
	ret
#ifndef __APPLE__
	.size	isal_deflate_icf_body_hash_hist_aarch64, .-isal_deflate_icf_body_hash_hist_aarch64
#endif
