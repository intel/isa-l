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
	.arch armv8-a+crc
	.text

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

/*
void isal_deflate_icf_finish_hash_hist_aarch64(struct isal_zstream *stream);
*/

/* constant */

/* offset of struct isal_zstream */
.equ	offset_next_in, 0
.equ	offset_avail_in, 8
.equ	offset_total_in, 12
.equ	offset_next_out, 16
.equ	offset_avail_out, 24
.equ	offset_total_out, 28
.equ	offset_hufftables, 32
.equ	offset_level, 40
.equ	offset_level_buf_size, 44
.equ	offset_level_buf, 48
.equ	offset_end_of_stream, 56
.equ	offset_flush, 58
.equ	offset_gzip_flag, 60
.equ	offset_hist_bits, 62
.equ	offset_state, 64
.equ	offset_state_block_end, 72
.equ	offset_state_state, 84
.equ	offset_state_has_hist, 135

/* offset of struct level_buf */
.equ	offset_encode_tables, 0
.equ	offset_hist, 2176
.equ	offset_hist_d_hist, 2176
.equ	offset_hist_ll_hist, 2296
.equ	offset_deflate_hdr_count, 4348
.equ	offset_deflate_hdr_extra_bits, 4352
.equ	offset_deflate_hdr, 4356
.equ	offset_icf_buf_next, 4688
.equ	offset_icf_buf_avail_out, 4696
.equ	offset_icf_buf_start, 4704
.equ	offset_hash8k, 4712
.equ	offset_hash_hist, 4712

/* offset of struct isal_zstate */
.equ	offset_dist_mask, 12
.equ	offset_hash_mask, 16
.equ	offset_state_of_zstate, 20

/* macros*/
.equ	ISAL_LOOK_AHEAD, 288

	/* arguments */
	declare_generic_reg	stream,			0,x

	declare_generic_reg	param0,			0,x
	declare_generic_reg	param1,			1,x
	declare_generic_reg	param2,			2,x
	declare_generic_reg	param3,			3,x
	declare_generic_reg	param4,			4,x
	declare_generic_reg	param5,			5,x
	declare_generic_reg	param6,			6,x

	/* local variable */
	declare_generic_reg	stream_saved,		15,x
	declare_generic_reg	level_buf,		13,x
	declare_generic_reg	start_in,		21,x
	declare_generic_reg	start_out,		22,x
	declare_generic_reg	state,			23,x
	declare_generic_reg	end_out,		12,x
	declare_generic_reg	end_in,			11,x
	declare_generic_reg	next_in,		8,x
	declare_generic_reg	next_out,		10,x
	declare_generic_reg	next_out_iter,		5,x
	declare_generic_reg	file_start,		18,x
	declare_generic_reg	last_seen,		14,x

	declare_generic_reg	literal_code,		9,w
	declare_generic_reg	hash_mask,		19,w
	declare_generic_reg	hist_size,		20,w
	declare_generic_reg	dist,			7,w
	declare_generic_reg	dist_inc,		24,w

	declare_generic_reg	tmp0,			25,x
	declare_generic_reg	tmp1,			26,x
	declare_generic_reg	tmp2,			27,x
	declare_generic_reg	tmp3,			28,x

	.align	2
	.type	write_deflate_icf_constprop, %function
write_deflate_icf_constprop:
	ldrh	w2, [x0]
	mov	w3, 30
	bfi	w2, w1, 0, 10
	strh	w2, [x0]
	ldr	w1, [x0]
	bfi	w1, w3, 10, 9
	str	w1, [x0]
	ubfx	x1, x1, 16, 3
	strh	w1, [x0, 2]
	ret
	.size	write_deflate_icf_constprop, .-write_deflate_icf_constprop

	.align	2
	.type	write_deflate_icf, %function
write_deflate_icf:
	ldrh	w4, [x0]
	bfi	w4, w1, 0, 10
	strh	w4, [x0]
	ldr	w1, [x0]
	bfi	w1, w2, 10, 9
	str	w1, [x0]
	lsr	w1, w1, 16
	bfi	w1, w3, 3, 13
	strh	w1, [x0, 2]
	ret
	.size	write_deflate_icf, .-write_deflate_icf

	.align	2
	.type	update_state, %function
update_state:
	sub	x7, x2, x1
	ldr	x4, [x0, 48]
	cmp	x7, 0
	ble	.L48
	mov	w1, 1
	strb	w1, [x0, 135]
.L48:
	ldr	w1, [x0, 12]
	sub	x6, x6, x5
	str	x2, [x0]
	sub	x3, x3, x2
	add	w1, w1, w7
	stp	w3, w1, [x0, 8]
	str	w1, [x0, 72]
	asr	x6, x6, 2
	str	x5, [x4, 4688]
	str	x6, [x4, 4696]
	ret
	.size	update_state, .-update_state

	.align	2
	.global	isal_deflate_icf_finish_hash_hist_aarch64
	.type	isal_deflate_icf_finish_hash_hist_aarch64, %function
isal_deflate_icf_finish_hash_hist_aarch64:
	ldr	w_end_in, [stream, 8] // stream->avail_in
	cbz	w_end_in, .stream_not_available

	stp	x29, x30, [sp, -96]!
	add	x29, sp, 0
	stp	x19, x20, [sp, 16]
	stp	x21, x22, [sp, 32]
	stp	x23, x24, [sp, 48]
	stp	x25, x26, [sp, 64]
	stp	x27, x28, [sp, 80]

	mov	stream_saved, stream
	ldr	level_buf, [stream, offset_level_buf]      // 48
	ldr	start_in, [stream, offset_next_in]         // 0
	ldr	start_out, [level_buf, offset_icf_buf_next]      // 4688
	add	state, stream, offset_state // 64
	ldr	end_out, [level_buf, offset_icf_buf_avail_out] // 4696
	mov	next_in, start_in
	ldr	w_file_start, [stream, offset_total_in] // 12
	mov	tmp0, offset_hash_hist // 4712
	add	last_seen, level_buf, tmp0
	add	end_in, start_in, w_end_in, uxtw
	and	end_out, end_out, -4
	mov	next_out, start_out
	ldp	hist_size, hash_mask, [state, offset_dist_mask] // 12
	sub	file_start, start_in, file_start
	add	end_out, start_out, end_out
	mov	next_out_iter, next_out

	add	x0, next_in, 3
	cmp	end_in, x0 // x0 <= next_in + 3
	bls	.while_first_end

	.p2align 3
.while_first:
	cmp	next_out, end_out
	bcs	.save_and_update_state
	ldr	literal_code, [next_in]
	mov	w0, literal_code
	crc32cw	w0, wzr, w0
	and	w0, w0, hash_mask
	sub	x2, next_in, file_start
	lsl	x0, x0, 1
	ldrh	dist, [last_seen, x0]
	strh	w2, [last_seen, x0]
	sub	w2, w2, dist
	and	w_dist, w2, 65535
	sub	dist_inc, dist, #1
	cmp	dist_inc, hist_size
	bcs	.skip_compare258

	mov	x2, 0
	sub	w2, w_end_in, w8
	mov	x1, next_in
	sub	x0, next_in, w_dist, uxth

	compare_max_258_bytes param0,param1,param2,tmp2,tmp0,tmp1
	mov	w0, w_tmp2
	and	w2, w0, 65535

	cmp	w2, 3
	bhi	.while_first_match_length

.skip_compare258:
	and	literal_code, literal_code, 255 // get_lit_icf_code
	add	next_in, next_in, 1
	mov	w1, literal_code
	mov	x0, next_out
	add	x_literal_code, level_buf, literal_code, uxtb 2 // level_buf->hist.ll_hist

	ldr	w_tmp0, [x_literal_code, offset_hist_ll_hist] // 2296
	add	w_tmp0, w_tmp0, 1
	str	w_tmp0, [x_literal_code, offset_hist_ll_hist] // 2296

	bl	write_deflate_icf_constprop // write_deflate_icf

	add	next_out, next_out, 4
.while_first_check:
	add	x0, next_in, 3
	mov	next_out_iter, next_out
	cmp	end_in, x0
	bhi	.while_first

.while_first_end:
	cmp	next_in, end_in
	bcs	.while_2nd_end

	cmp	next_out, end_out
	bcc	.while_2nd_handle
	b	.save_and_update_state_2nd

	.p2align 2
.while_2nd:
	cmp	end_out, next_out_iter
	bls	.save_and_update_state_2nd

.while_2nd_handle:
	ldrb	w2, [next_in], 1
	mov	x0, next_out_iter
	add	next_out_iter, next_out_iter, 4
	mov	w1, w2
	add	x2, level_buf, w2, uxtb 2

	ldr	w_tmp0, [x2, offset_hist_ll_hist] // 2296
	add	w_tmp0, w_tmp0, 1
	str	w_tmp0, [x2, offset_hist_ll_hist] // 2296

	bl	write_deflate_icf_constprop
	cmp	end_in, next_in
	bne	.while_2nd

	mov	next_in, end_in
	b	.end_of_stream_check_and_exit

	.p2align 2
.while_first_match_length:
	and	w0, w0, 65535
	mov	w3, 0
	add	w1, w0, 254 // get_len_icf_code
	cmp	dist, 2
	bhi	.compute_dist_icf_code

.while_first_match_length_end:
	ubfiz	x_tmp2, x1, 2, 17
	add	x_tmp1, level_buf, dist_inc, uxtw 2
	add	x_tmp2, level_buf, x_tmp2

	add	next_in, next_in, w2, uxth
	mov	w2, dist_inc

	ldr	w_tmp0, [x_tmp2, offset_hist_ll_hist] // 2296
	add	w_tmp0, w_tmp0, 1
	str	w_tmp0, [x_tmp2, offset_hist_ll_hist] // 2296

	mov	x0, next_out
	ldr	w_tmp0, [x_tmp1, offset_hist_d_hist] // 2176
	add	w_tmp0, w_tmp0, 1
	str	w_tmp0, [x_tmp1, offset_hist_d_hist] // 2176

	bl	write_deflate_icf
	add	next_out, next_out, 4
	b	.while_first_check

// compute_dist_icf_code
	.p2align 2
.compute_dist_icf_code:
	clz	w3, dist_inc
	mov	w0, 30
	sub	w0, w0, w3

	mov	w3, 1
	lsl	w3, w3, w0
	sub	w3, w3, #1
	and	w3, w3, dist_inc
	lsl	w4, w0, 1
	lsr	dist_inc, dist_inc, w0
	add	dist_inc, dist_inc, w4
	b	.while_first_match_length_end

.while_2nd_end:
	beq	.end_of_stream_check_and_exit
	mov	param6, end_out
	b	.update_state

.end_of_stream_check_and_exit:
	ldr	w_tmp0, [stream_saved, offset_end_of_stream] // 56
	cbz	w_tmp0, .update_state_2nd
	b	.save_and_update_state_2nd

	.p2align 3
.save_and_update_state_2nd:
	mov	w_tmp0, 2
	str	w_tmp0, [state, offset_state_of_zstate] // 20
.update_state_2nd:
	mov	param6, end_out
	b	.update_state

	.p2align 2
.save_and_update_state:
	mov	param6, end_out
	mov	param5, next_out
	mov	w_tmp0, 2
	str	w_tmp0, [state, offset_state_of_zstate] // 20
.update_state:
	mov	param4, start_out
	mov	param1, start_in
	mov	param3, end_in
	mov	param2, next_in
	mov	param0, stream_saved

	ldp	x19, x20, [sp, 16]
	ldp	x21, x22, [sp, 32]
	ldp	x23, x24, [sp, 48]
	ldp	x25, x26, [sp, 64]
	ldp	x27, x28, [sp, 80]
	ldp	x29, x30, [sp], 96

	b	update_state

	.p2align 2
.stream_not_available:
	ldr	w1, [stream, offset_end_of_stream] // 56
	cbz	w1, .done

	mov	w1, 2
	str	w1, [stream, offset_state_state] // 84
.done:
	ret

	.size	isal_deflate_icf_finish_hash_hist_aarch64, .-isal_deflate_icf_finish_hash_hist_aarch64
