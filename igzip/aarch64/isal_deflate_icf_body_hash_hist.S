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

	.global	isal_deflate_icf_body_hash_hist_aarch64
	.type	isal_deflate_icf_body_hash_hist_aarch64, %function
/*
void isal_deflate_icf_body_hash_hist_base(struct isal_zstream *stream);
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

/* macros*/
.equ	ISAL_LOOK_AHEAD, 288

	/* arguments */
	declare_generic_reg	stream,		0,x
	declare_generic_reg	stream_saved,	11,x

	declare_generic_reg	param0,		0,x
	declare_generic_reg	param1,		1,x
	declare_generic_reg	param2,		2,x

	/* local varibale */
	declare_generic_reg	level_buf,	18,x
	declare_generic_reg	avail_in,	13,w
	declare_generic_reg	end_in,		13,x
	declare_generic_reg	start_in,	19,x
	declare_generic_reg	next_in,	9,x
	declare_generic_reg	next_in_iter,	14,x
	declare_generic_reg	state,		24,x
	declare_generic_reg	hist_size,	22,w
	declare_generic_reg	hash_mask,	21,w
	declare_generic_reg	start_out,	12,x
	declare_generic_reg	end_out,	12,x
	declare_generic_reg	next_out,	8,x
	declare_generic_reg	file_start,	20,x
	declare_generic_reg	last_seen,	15,x
	declare_generic_reg	total_in,	25,x
	declare_generic_reg	NULL_DIST_SYM,	23,w
	declare_generic_reg	match_length,	3,x
	declare_generic_reg	dist,		7,x
	declare_generic_reg	dist_inc,	26,w // dist - 1
	declare_generic_reg	literal,	10,x

	declare_generic_reg	tmp0,		4,x
	declare_generic_reg	tmp1,		5,x

isal_deflate_icf_body_hash_hist_aarch64:
	stp	x29, x30, [sp, -80]!
	add	x29, sp, 0
	str	x24, [sp, 56]

	ldr	avail_in, [stream, offset_avail_in]
	cbnz	avail_in, .stream_available

	ldr	w1, [stream, offset_end_of_stream] // w1 keeps two values of end_of_stream and flush
	cbz	w1, .done

	add	state, stream, offset_state
	b	.state_flush_read_buffer

	.align 2
.stream_available:
	stp	x19, x20, [x29, 16]
	stp	x21, x22, [x29, 32]
	str	x23, [x29, 48]
	stp	x25, x26, [x29, 64]

	ldr	level_buf, [stream, offset_level_buf]
	add	state, stream, offset_state // 64
	mov	stream_saved, stream
	ldr	start_in, [stream, offset_next_in] // 0
	ldr	w_total_in, [stream, offset_total_in]

	mov	x0, offset_hash_hist
	add	last_seen, level_buf, x0

	ldr	x0, [level_buf, offset_icf_buf_avail_out] // 4696
	ldr	start_out, [level_buf, offset_icf_buf_next] // 4688

	mov	next_in, start_in
	and	x0, x0, -4
	ldp	hist_size, hash_mask, [state, offset_dist_mask] // 12
	add	end_in, start_in, avail_in, uxtw
	mov	next_out, start_out
	add	end_out, start_out, x0

	add	x0, next_in, ISAL_LOOK_AHEAD // 288
	sub	file_start, start_in, w_total_in, uxtw
	mov	NULL_DIST_SYM, 30
	add	next_in_iter, next_in, 1
	cmp	end_in, x0
	bls	.while_loop_end

	.align 3
.while_loop:
	cmp	next_out, end_out
	bcs	.state_create_hdr

	ldr	w_literal, [next_in]
	mov	w0, w_literal
	crc32cw	w0, wzr, w0

	and	w0, w0, hash_mask
	sub	x1, next_in, file_start
	lsl	x0, x0, 1

	ldrh	w_dist, [last_seen, x0]
	strh	w1, [last_seen, x0]
	sub	w1, w1, w_dist
	and	w_dist, w1, 65535

	sub	dist_inc, w_dist, #1
	cmp	dist_inc, hist_size
	bcc	.dist_vs_hist_size

.while_latter_part:
	and	w_literal, w_literal, 255
	mov	next_in, next_in_iter
	add	next_out, next_out, 4
	add	x1, level_buf, w_literal, uxtb 2
	ldr	w0, [x1, 2296]
	add	w0, w0, 1
	str	w0, [x1, 2296]
	ldrh	w0, [next_out, -4]
	bfi	w0, w_literal, 0, 10
	strh	w0, [next_out, -4]
	ldr	w0, [next_out, -4]
	bfi	w0, NULL_DIST_SYM, 10, 9
	str	w0, [next_out, -4]
	ubfx	x0, x0, 16, 3
	strh	w0, [next_out, -2]

.while_loop_check:
	add	x0, next_in, ISAL_LOOK_AHEAD // 288
	add	next_in_iter, next_in, 1
	cmp	end_in, x0
	bhi	.while_loop
	b	.while_loop_end

	.align 2
.dist_vs_hist_size:
	mov	x1, next_in
	mov	w2, 258
	sub	x0, next_in, w_dist, uxth
	compare_258_bytes param0,param1,match_length,tmp0,tmp1

	and	w1, w_match_length, 65535 // 0xffff
	cmp	w1, 3
	bls	.while_latter_part

	ldr	w0, [next_in, 1]
	mov	x4, next_in
	add	next_in, next_in, w1, uxth
	crc32cw	w0, wzr, w0

	and	w0, hash_mask, w0
	sub	next_in_iter, next_in_iter, file_start
	strh	w_next_in_iter, [last_seen, x0, lsl 1]
	ldr	w0, [x4, 2]!
	crc32cw	w0, wzr, w0

	and	w0, hash_mask, w0
	and	w_match_length, w_match_length, 65535 // 0xffff
	sub	x4, x4, file_start

	// get_len_icf_code
	add	w_match_length, w_match_length, 254
	// get_dist_icf_code, first part
	mov	w1, 0 // w1 => dist_extra
	strh	w4, [last_seen, x0, lsl 1]
	cmp	w_dist, 2
	ubfiz	x0, match_length, 2, 17
	add	x0, level_buf, x0
	bhi	.compute_dist_icf_code

.match_length_end:
	// handle level_buf->hist
	ldr	w2, [x0, offset_hist_ll_hist]       // 2296, ll_hist
	add	x4, level_buf, dist_inc, uxtw 2 // d_hist
	add	next_out, next_out, 4
	add	w2, w2, 1                           // ll_hist
	str	w2, [x0, offset_hist_ll_hist]       // 2296, ll_hist
	ldr	w0, [x4, offset_hist_d_hist]        // 2176, d_hist
	add	w0, w0, 1                           // d_hist
	str	w0, [x4, offset_hist_d_hist]        // 2176, d_hist

	// write_deflate_icf
	ldrh	w0, [next_out, -4]
	bfi	w0, w3, 0, 10
	strh	w0, [next_out, -4]
	ldr	w0, [next_out, -4]
	bfi	w0, dist_inc, 10, 9
	str	w0, [next_out, -4]
	lsr	w0, w0, 16
	bfi	w0, w1, 3, 13 // w1 => dist_extra
	strh	w0, [next_out, -2]
	b	.while_loop_check

	.align 2
// get_dist_icf_code, 2nd part
.compute_dist_icf_code:
	clz	w1, dist_inc
	mov	w2, 30
	sub	w2, w2, w1
	mov	w1, 1
	lsl	w1, w1, w2
	sub	w1, w1, #1
	and	w1, w1, dist_inc
	lsr	dist_inc, dist_inc, w2
	add	dist_inc, dist_inc, w2, lsl 1
	and	w1, w1, 8191
	b	.match_length_end

.while_loop_end:
	sub	x19, next_in, x19
	cmp	x19, 0
	ble	.skip_igzip_hist2

	mov	w0, 1
	strb	w0, [stream_saved, offset_state_has_hist] // 135

.skip_igzip_hist2:
	add	w19, w_total_in, w19
	ldr	w0, [stream_saved, offset_end_of_stream] // 56
	sub	x12, end_out, next_out
	asr	x12, x12, 2                                   // x12 => end_out - next_out
	str	next_in, [stream_saved]
	str	w19, [stream_saved, offset_total_in] // 12
	sub	next_in, end_in, next_in
	str	w19, [stream_saved, offset_state_block_end] // 72

	ldp	x25, x26, [x29, 64]
	ldr	x23, [x29, 48]
	ldp	x21, x22, [x29, 32]
	ldp	x19, x20, [x29, 16]

	str	w9, [stream_saved, offset_avail_in] // 8
	str	next_out, [level_buf, offset_icf_buf_next] // 4688
	str	x12, [level_buf, offset_icf_buf_avail_out]   // 4696, x12 => end_out - next_out
	cbnz	w0, .state_flush_read_buffer
	b	.done

	.align 2
.state_create_hdr:
	mov	w0, 2
	str	w0, [x24, 20]
	sub	start_in, next_in, start_in
	cmp	start_in, 0
	ble	.skip_igzip_hist

	mov	w0, 1
	strb	w0, [stream_saved, offset_state_has_hist] // 135

.skip_igzip_hist:
	add	w_total_in, w_total_in, w19
	sub	x12, end_out, next_out
	asr	x12, x12, 2                                  // x12 => end_out - next_out
	str	next_in, [stream_saved]
	sub	next_in, end_in, next_in
	str	w_total_in, [stream_saved, offset_total_in] // 12
	str	w_total_in, [stream_saved, offset_state_block_end] // 72

	ldp	x25, x26, [x29, 64]
	ldr	x23, [x29, 48]
	ldp	x21, x22, [x29, 32]
	ldp	x19, x20, [x29, 16]

	str	w9, [stream_saved, offset_avail_in] // 8
	str	next_out, [level_buf, offset_icf_buf_next] // 4688
	str	x12, [level_buf, offset_icf_buf_avail_out]   // 4696, x12 => end_out - next_out
	b	.done

.state_flush_read_buffer:
	mov	w0, 4
	str	w0, [x24, 20]

.done:
	ldr	x24, [sp, 56]
	ldp	x29, x30, [sp], 80
	ret

	.size	isal_deflate_icf_body_hash_hist_aarch64, .-isal_deflate_icf_body_hash_hist_aarch64
