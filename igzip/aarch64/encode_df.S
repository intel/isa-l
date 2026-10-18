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

	.arch armv8-a
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

	.global	cdecl(encode_deflate_icf_aarch64)
#ifndef __APPLE__
	.type	encode_deflate_icf_aarch64, %function
#endif

/*
	struct deflate_icf *encode_deflate_icf_base(struct deflate_icf *next_in,
				struct deflate_icf *end_in, struct BitBuf2 *bb,
				struct hufftables_icf *hufftables)

	The bit buffer stays in registers for the whole loop. Each token adds at
	most 48 bits (20 lit/len, 15 dist, and 13 dist extra) to at most 7
	pending bits, so one 64-bit store and one flush per token are enough.
*/

	// parameters
	declare_generic_reg	next_in,		0,x
	declare_generic_reg	end_in,			1,x
	declare_generic_reg	bb,			2,x
	declare_generic_reg	hufftables,		3,x

	// local variables
	declare_generic_reg	bits,			4,x
	declare_generic_reg	bit_count,		5,x
	declare_generic_reg	out_buf,		6,x
	declare_generic_reg	out_end,		7,x
	declare_generic_reg	lit_len_table,		8,x
	declare_generic_reg	icf,			9,x
	declare_generic_reg	lsym,			10,x
	declare_generic_reg	dsym,			11,x
	declare_generic_reg	code,			12,x
	declare_generic_reg	len,			13,x
	declare_generic_reg	tmp0,			14,x
	declare_generic_reg	tmp1,			15,x
	declare_generic_reg	tmp2,			16,x
	declare_generic_reg	tmp3,			17,x

// bit buffer offset
.equ	offset_m_bits, 0
.equ	offset_m_bit_count, 8
.equ	offset_m_out_buf, 16
.equ	offset_m_out_end, 24

// hufftables_icf offset
.equ	offset_lit_len_table, 31 * 4

cdecl(encode_deflate_icf_aarch64):
	cmp	next_in, end_in
	bhs	.done

	ldp	out_buf, out_end, [bb, offset_m_out_buf]
	cmp	out_buf, out_end
	bhi	.done

	ldr	bits, [bb, offset_m_bits]
	ldr	w_bit_count, [bb, offset_m_bit_count]
	add	lit_len_table, hufftables, offset_lit_len_table

	.align 4
.loop:
	ldr	w_icf, [next_in], 4

	/* lsym = lit_len_table[lit_len], dsym = dist_table[lit_dist] */
	and	w_tmp0, w_icf, 0x3ff
	ubfx	w_tmp1, w_icf, 10, 9
	ldr	w_lsym, [lit_len_table, x_tmp0, lsl 2]
	ldr	w_dsym, [hufftables, x_tmp1, lsl 2]

	/* code = lit code | dist code << lit len | dist extra << (lit len + dist len) */
	lsr	w_tmp0, w_icf, 19
	lsr	w_tmp1, w_dsym, 24
	and	w_tmp2, w_dsym, 0xffff
	lsl	x_tmp0, x_tmp0, x_tmp1
	lsr	w_len, w_lsym, 24
	orr	x_tmp0, x_tmp0, x_tmp2
	and	w_code, w_lsym, 0xffffff
	lsl	x_tmp0, x_tmp0, x_len
	ubfx	w_tmp3, w_dsym, 16, 8
	orr	x_code, x_code, x_tmp0
	add	w_len, w_len, w_tmp1
	add	w_len, w_len, w_tmp3

	/* write_bits_unsafe and flush_bits */
	lsl	x_code, x_code, bit_count
	add	w_bit_count, w_bit_count, w_len
	orr	bits, bits, x_code
	and	w_tmp0, w_bit_count, -8
	str	bits, [out_buf]
	add	out_buf, out_buf, bit_count, lsr 3
	lsr	bits, bits, x_tmp0
	and	w_bit_count, w_bit_count, 7

	/* Continue while next_in < end_in and the output is not full */
	cmp	next_in, end_in
	ccmp	out_buf, out_end, 2, lo
	bls	.loop

	str	bits, [bb, offset_m_bits]
	str	w_bit_count, [bb, offset_m_bit_count]
	str	out_buf, [bb, offset_m_out_buf]
.done:
	ret
#ifndef __APPLE__
	.size	encode_deflate_icf_aarch64, .-encode_deflate_icf_aarch64
#endif
