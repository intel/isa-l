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

	.global	encode_deflate_icf_aarch64
	.type	encode_deflate_icf_aarch64, %function

/*
	struct deflate_icf *encode_deflate_icf_base(struct deflate_icf *next_in,
				struct deflate_icf *end_in, struct BitBuf2 *bb,
				struct hufftables_icf *hufftables)

*/

	// parameters
	declare_generic_reg	next_in,		0,x
	declare_generic_reg	end_in,			1,x
	declare_generic_reg	bb,			2,x
	declare_generic_reg	hufftables,		3,x

	// local variable
	declare_generic_reg	bb_out_end,		4,x
	declare_generic_reg	bb_bit_count,		5,w
	declare_generic_reg	dist_extra,		6,x
	declare_generic_reg	dist_lit_table,		7,x
	declare_generic_reg	code_and_extra,		8,x
	declare_generic_reg	bb_out_buf,		9,x
	declare_generic_reg	bb_bits,		10,x
	declare_generic_reg	d_length,		11,x
	declare_generic_reg	l_length,		12,x
	declare_generic_reg	d_extra_bit_count,	13,x

	declare_generic_reg	code_sum,		4,x
	declare_generic_reg	count_sum,		7,x

	declare_generic_reg	tmp0,			14,x
	declare_generic_reg	tmp1,			15,x

// bit buffer offset
.equ	offset_m_bits, 0
.equ	offset_m_bit_count, 8
.equ	offset_m_out_buf, 16
.equ	offset_m_out_end, 24

encode_deflate_icf_aarch64:
	cmp	next_in, end_in
	bcs	.done

	ldp	bb_out_buf, bb_out_end, [bb, offset_m_out_buf]
	cmp	bb_out_end, bb_out_buf
	bcc	.done

	ldr	bb_bit_count, [bb, offset_m_bit_count]
	ldr	bb_bits, [bb, offset_m_bits]
	b	.loop_start

	.align 3
.loop:
	ldr	bb_out_end, [bb, offset_m_out_end]
	cmp	bb_out_end, bb_out_buf
	bcc	.done

.loop_start:
	ldrh	w_code_and_extra, [next_in]
	add	next_in, next_in, 4
	ldr	w_dist_lit_table, [next_in, -4]
	and	code_and_extra, code_and_extra, 1023

	ldrh	w_dist_extra, [next_in, -2]
	add	code_and_extra, code_and_extra, 31
	ubfx	x_dist_lit_table, x_dist_lit_table, 10, 9
	add	x_tmp0, hufftables, code_and_extra, lsl 2
	ubfx	x_dist_extra, x_dist_extra, 3, 13
	lsl	x_dist_lit_table, x_dist_lit_table, 2

	ldr	w_code_and_extra, [hufftables, code_and_extra, lsl 2]
	add	x_d_extra_bit_count, hufftables, x_dist_lit_table
	ldrb	w_l_length, [x_tmp0, 3]
	and	code_and_extra, code_and_extra, 0xffffff
	ldrh	w_code_sum, [hufftables, x_dist_lit_table]
	ldrb	w_d_length, [x_d_extra_bit_count, 3]
	add	w_l_length, w_l_length, bb_bit_count
	ldrb	w_d_extra_bit_count, [x_d_extra_bit_count, 2]

	lsl	x_tmp0, code_and_extra, x_bb_bit_count
	add	bb_bit_count, w_d_length, w_l_length
	lsl	x_code_sum, x_code_sum, x_l_length
	orr	x_code_sum, x_code_sum, x_tmp0
	add	w_count_sum, w_d_extra_bit_count, bb_bit_count
	lsl	x_bb_bit_count, x_dist_extra, x_bb_bit_count

	orr	x_bb_bit_count, x_bb_bit_count, bb_bits
	orr	x_tmp0, x_code_sum, x_bb_bit_count         // me->m_bits => x_tmp0
	str	x_tmp0, [bb, offset_m_bits]                // me->m_bits => x_tmp0
	str	w_count_sum, [bb, offset_m_bit_count]

	str	x_tmp0, [bb_out_buf]                       // me->m_bits => x_tmp0
	ldr	bb_bit_count, [bb, offset_m_bit_count]
	ldr	bb_bits, [bb, offset_m_bits]
	and	w_tmp0, bb_bit_count, -8                   // bits => w_tmp0
	ldr	bb_out_buf, [bb, offset_m_out_buf]
	lsr	w_tmp1, bb_bit_count, 3                    // bits/8 => w_tmp1
	lsr	bb_bits, bb_bits, x_tmp0                   // bits => x_tmp0
	sub	bb_bit_count, bb_bit_count, w_tmp0         // bits => w_tmp0
	add	bb_out_buf, bb_out_buf, x_tmp1             // bits/8 => x_tmp1
	str	bb_bits, [bb,offset_m_bits]
	str	bb_bit_count, [bb, offset_m_bit_count]
	str	bb_out_buf, [bb, offset_m_out_buf]

	cmp	end_in, next_in
	bhi	.loop

.done:
	ret
	.size	encode_deflate_icf_aarch64, .-encode_deflate_icf_aarch64
