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
	.arch armv8-a
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

	.text
	.align	2
	.global	set_long_icf_fg_aarch64
	.type	set_long_icf_fg_aarch64, %function

/*
void set_long_icf_fg_aarch64(uint8_t * next_in, uint64_t processed, uint64_t input_size,
			  struct deflate_icf *match_lookup)
*/

	/* arguments */
	declare_generic_reg	next_in_param,		0,x
	declare_generic_reg	processed_param,	1,x
	declare_generic_reg	input_size_param,	2,x
	declare_generic_reg	match_lookup_param,	3,x

	declare_generic_reg	param0,			0,x
	declare_generic_reg	param1,			1,x
	declare_generic_reg	param2,			2,x

	/* local variable */
	declare_generic_reg	len,			7,w
	declare_generic_reg	dist_code,		8,w
	declare_generic_reg	shortest_match_len	9,w
	declare_generic_reg	len_max,		10,w
	declare_generic_reg	dist_extra,		11,w
	declare_generic_reg	const_8,		13,x
	declare_generic_reg	next_in,		20,x
	declare_generic_reg	dist_start,		21,x
	declare_generic_reg	end_processed,		22,x
	declare_generic_reg	end_in,			23,x
	declare_generic_reg	match_lookup,		19,x

	declare_generic_reg	match_length,		4,w
	declare_generic_reg	tmp0,			5,w
	declare_generic_reg	tmp1,			6,w

/* constant */
.equ	DIST_START_SIZE, 128
.equ	ISAL_LOOK_AHEAD, 288
.equ	LEN_OFFSET, 254
.equ	SHORTEST_MATCH, 4
.equ	LEN_MAX_CONST, 512

set_long_icf_fg_aarch64:
	stp	x29, x30, [sp, -192]!
	add	x29, sp, 0
	stp	x21, x22, [sp, 32]
	add	x21, x29, 64
	stp	x19, x20, [sp, 16]
	str	x23, [sp, 48]

	add	end_processed, next_in_param, processed_param
	mov	next_in, next_in_param
	add	end_in, next_in_param, input_size_param
	mov	match_lookup, match_lookup_param

	adrp	x1, .data_dist_start
	mov	x2, DIST_START_SIZE // 128
	add	x1, x1, :lo12:.data_dist_start
	mov	x0, dist_start
	bl	memcpy

	add	x_tmp0, end_processed, ISAL_LOOK_AHEAD // 288
	cmp	end_in, x_tmp0
	csel	end_in, end_in, x_tmp0, cc
	cmp	next_in, end_processed
	bcs	.done

	mov	const_8, 8
	mov	len_max, LEN_MAX_CONST // 512
	mov	shortest_match_len, (LEN_OFFSET + SHORTEST_MATCH - 1)
	b	.while_outer_loop

	.align 2
.while_outer_check:
	add	next_in, next_in, 1
	add	match_lookup, match_lookup, 4
	cmp	end_processed, next_in
	bls	.done

.while_outer_loop:
	ldrh	len, [match_lookup]
	and	len, len, LIT_LEN_MASK // 1023
	cmp	len, (LEN_OFFSET + 8 - 1) // 261
	bls	.while_outer_check

	ldr	dist_code, [match_lookup]
	add	x1, next_in, 8
	ldrh	dist_extra, [match_lookup, 2]
	sub	w2, w_end_in, w1
	ubfx	x_dist_code, x_dist_code, 10, 9
	ubfx	x_dist_extra, x_dist_extra, 3, 13
	uxtw	x0, dist_code
	ldr	w0, [dist_start, x0, lsl 2]
	add	w0, dist_extra, w0
	sub	x0, const_8, x0
	add	x0, next_in, x0

	compare_aarch64 param0,param1,param2,match_length,tmp0,tmp1
	mov	w0, w_match_length

	add	w0, w0, (LEN_OFFSET + 8) // 262
	cmp	w0, len
	bls	.while_outer_check

	lsl	w2, dist_extra, 19
	orr	w2, w2, dist_code, lsl 10

	.align 3
.while_inner_loop:
	cmp	w0, LEN_MAX_CONST // 512
	add	next_in, next_in, 1
	csel	w1, w0, len_max, ls
	sub	w0, w0, #1
	orr	w1, w1, w2
	str	w1, [match_lookup]
	ldrh	w1, [match_lookup, 4]!

	and	w1, w1, LIT_LEN_MASK // 1023
	cmp	w1, (LEN_OFFSET + SHORTEST_MATCH - 1) // 257
	csel	w1, w1, shortest_match_len, cs
	cmp	w1, w0
	bcc	.while_inner_loop

	add	next_in, next_in, 1
	add	match_lookup, match_lookup, 4
	cmp	end_processed, next_in
	bhi	.while_outer_loop

.done:
	ldp	x19, x20, [sp, 16]
	ldp	x21, x22, [sp, 32]
	ldr	x23, [sp, 48]
	ldp	x29, x30, [sp], 192
	ret
	.size	set_long_icf_fg_aarch64, .-set_long_icf_fg_aarch64

	.section	.rodata
	.align	3
	.set	.data_dist_start,. + 0
.real_data_dist_start:
	.word	0x0001, 0x0002, 0x0003, 0x0004, 0x0005, 0x0007, 0x0009, 0x000d
	.word	0x0011, 0x0019, 0x0021, 0x0031, 0x0041, 0x0061, 0x0081, 0x00c1
	.word	0x0101, 0x0181, 0x0201, 0x0301, 0x0401, 0x0601, 0x0801, 0x0c01
	.word	0x1001, 0x1801, 0x2001, 0x3001, 0x4001, 0x6001, 0x0000, 0x0000
