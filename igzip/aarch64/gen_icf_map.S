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
	.arch armv8-a+crc+crypto
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

.macro tzbytecnt param0:req,param1:req
	rbit	x_\param1, x_\param0
	cmp	x_\param0, 0
	clz	x_\param1, x_\param1
	mov	w_\param0, 8
	lsr	w_\param1, w_\param1, 3
	csel	w_\param0, w_\param1, w_\param0, ne
.endm

.macro write_deflate_icf param0:req,param1:req,param2:req,param3:req
	orr	w_\param1, w_\param1, w_\param3, lsl 19
	orr	w_\param1, w_\param1, w_\param2, lsl 10
	str	w_\param1, [x_\param0]
.endm

	.align	2
	.global	gen_icf_map_h1_aarch64
	.type	gen_icf_map_h1_aarch64, %function

	/* arguments */
	declare_generic_reg	stream_param,			0,x
	declare_generic_reg	matches_icf_lookup_param,	1,x
	declare_generic_reg	input_size_param,		2,x

	declare_generic_reg	param0,				0,x
	declare_generic_reg	param1,				1,x
	declare_generic_reg	param2,				2,x
	declare_generic_reg	param3,				3,x

	/* return */
	declare_generic_reg	ret_val,			0,x

	/* variables */
	declare_generic_reg	input_size,			3,x
	declare_generic_reg	next_in,			4,x
	declare_generic_reg	matches_icf_lookup,		6,x
	declare_generic_reg	hash_table,			7,x
	declare_generic_reg	end_in,				8,x
	declare_generic_reg	file_start,			9,x
	declare_generic_reg	hash_mask,			10,w
	declare_generic_reg	hist_size,			11,w
	declare_generic_reg	stream_saved,			12,x
	declare_generic_reg	literal_32,			13,w
	declare_generic_reg	literal_1,			14,w
	declare_generic_reg	dist,				15,w

	declare_generic_reg	tmp_has_hist,			0,w
	declare_generic_reg	tmp_offset_hash_table,		1,x
	declare_generic_reg	tmp0,				0,x
	declare_generic_reg	tmp1,				1,x
	declare_generic_reg	tmp2,				2,x
	declare_generic_reg	tmp3,				3,x
	declare_generic_reg	tmp5,				5,x

/* constant */
.equ	ISAL_LOOK_AHEAD, 288
.equ	SHORTEST_MATCH, 4
.equ	LEN_OFFSET, 254

/* mask */
.equ	mask_10bit, 1023
.equ	mask_lit_dist, 0x7800

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
.equ	offset_state_dist_mask, 76
.equ	offset_state_has_hist, 135

/* offset of struct level_buf */
.equ	offset_hash_map_hash_table, 4712

/*
uint64_t gen_icf_map_h1_base(struct isal_zstream *stream,
			     struct deflate_icf *matches_icf_lookup, uint64_t input_size)
*/

gen_icf_map_h1_aarch64:
	cmp	input_size_param, (ISAL_LOOK_AHEAD-1) // 287
	bls	.fast_exit
	stp	x29, x30, [sp, -16]!

	mov	stream_saved, stream_param
	mov	matches_icf_lookup, matches_icf_lookup_param
	mov	x29, sp

	ldrb	tmp_has_hist, [stream_saved, offset_state_has_hist]
	mov	tmp_offset_hash_table, offset_hash_map_hash_table
	ldr	end_in, [stream_saved, offset_next_in]
	mov	input_size, input_size_param
	ldr	hash_table, [stream_saved, offset_level_buf]
	ldr	w_file_start, [stream_saved, offset_total_in]
	ldp	hist_size, hash_mask, [stream_saved, offset_state_dist_mask]
	add	hash_table, hash_table, tmp_offset_hash_table
	sub	file_start, end_in, file_start
	cbz	tmp_has_hist, .igzip_no_hist
	b	.while_check1

	.align 3
.igzip_no_hist:
	ldrb	w_tmp1, [end_in]
	add	next_in, end_in, 1
	ldrh	w_tmp0, [matches_icf_lookup]
	bfi	w_tmp0, w_tmp1, 0, 10
	strh	w_tmp0, [matches_icf_lookup]
	ldr	w_tmp0, [matches_icf_lookup]
	and	w_tmp0, w_tmp0, mask_10bit
	orr	w_tmp0, w_tmp0, mask_lit_dist
	str	w_tmp0, [matches_icf_lookup], 4
	ldr	w_tmp0, [end_in]
	crc32cw	w_tmp0, wzr, w_tmp0

	and	w_tmp5, w_tmp0, hash_mask
	sub	x_tmp1, end_in, file_start
	mov	w_tmp2, 1
	mov	x_tmp0, 1
	strh	w_tmp1, [hash_table, x_tmp5, lsl 1]
	strb	w_tmp2, [stream_saved, offset_state_has_hist]
	b	.while_check2

.while_check1:
	mov	next_in, end_in
	mov	x_tmp0, 0

.while_check2:
	sub	input_size, input_size, #288
	add	end_in, end_in, input_size
	cmp	next_in, end_in
	bcs	.exit
	mov	literal_32, 32
	mov	literal_1, 1
	b	.while_loop

	.align 3
.new_match_found:
	clz	w_tmp5, w_tmp2
	add	w_tmp1, w_tmp0, LEN_OFFSET
	sub	w_tmp5, literal_32, w_tmp5
	cmp	dist, 2
	sub	w_tmp5, w_tmp5, #2
	bls	.skip_compute_dist_icf_code

	lsl	w_tmp3, literal_1, w_tmp5
	sub	w_tmp3, w_tmp3, #1
	lsr	w_tmp0, w_tmp2, w_tmp5
	and	w_tmp3, w_tmp3, w_tmp2
	add	w_tmp2, w_tmp0, w_tmp5, lsl 1

.skip_compute_dist_icf_code:
	mov	param0, matches_icf_lookup
	write_deflate_icf param0,param1,param2,param3

	add	next_in, next_in, 1
	add	matches_icf_lookup, matches_icf_lookup, 4
	cmp	next_in, end_in
	beq	.save_with_exit

.while_loop:
	ldr	w_tmp0, [next_in]
	crc32cw	w_tmp0, wzr, w_tmp0

	and	w_tmp0, w_tmp0, hash_mask
	sub	x_tmp1, next_in, file_start
	lsl	x_tmp0, x_tmp0, 1
	sub	w_tmp2, w_tmp1, #1
	ldrh	w_tmp3, [hash_table, x_tmp0]
	strh	w_tmp1, [hash_table, x_tmp0]
	sub	w_tmp2, w_tmp2, w_tmp3
	and	w_tmp2, w_tmp2, hist_size
	add	dist, w_tmp2, 1
	ldr	x_tmp0, [next_in]
	sub	x_tmp1, next_in, x_dist, uxtw
	ldr	x_tmp1, [x_tmp1]
	eor	x_tmp0, x_tmp1, x_tmp0
	tzbytecnt	param0,param1

	cmp	w_tmp0, (SHORTEST_MATCH-1)
	mov	w_tmp3, 0
	bhi	.new_match_found

	ldrb	w_param1, [next_in]
	mov	x_param0, matches_icf_lookup
	mov	w_param3, 0
	mov	w_param2, 0x1e
	write_deflate_icf param0,param1,param2,param3

	add	next_in, next_in, 1
	add	matches_icf_lookup, matches_icf_lookup, 4
	cmp	next_in, end_in
	bne	.while_loop

.save_with_exit:
	ldr	ret_val, [stream_saved, offset_next_in]
	sub	ret_val, next_in, ret_val

.exit:
	ldp	x29, x30, [sp], 16
	ret

	.align 3
.fast_exit:
	mov	ret_val, 0
	ret
	.size	gen_icf_map_h1_aarch64, .-gen_icf_map_h1_aarch64
