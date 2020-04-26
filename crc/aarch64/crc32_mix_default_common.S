/**********************************************************************
  Copyright(c) 2020 Arm Corporation All rights reserved.

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

.macro	declare_generic_reg name:req, reg:req, default:req
	\name	.req	\default\reg
	w_\name	.req	w\reg
	x_\name	.req	x\reg
.endm

.macro	declare_neon_reg name:req, reg:req, default:req
	\name	.req	\default\reg
	v_\name	.req	v\reg
	q_\name	.req	q\reg
	d_\name	.req	d\reg
	s_\name	.req	s\reg
.endm

/**********************************************************************
	variables
**********************************************************************/
	declare_generic_reg	crc,		0,w
	declare_generic_reg	buf,		1,x
	declare_generic_reg	len,		2,x
	declare_generic_reg	buf_saved,	3,x
	declare_generic_reg	buf_iter,	4,x
	declare_generic_reg	len_saved,	5,x
	declare_generic_reg	buf_tmp,	6,x

	declare_generic_reg	crc0,		7,x
	declare_generic_reg	crc1,		8,x
	declare_generic_reg	crc2,		9,x
	declare_generic_reg	pconst,		10,x
	declare_generic_reg	data_crc0,	11,x
	declare_generic_reg	data_crc1,	12,x
	declare_generic_reg	data_crc2,	13,x

	declare_generic_reg	size,		9,x
	declare_generic_reg	crc_tmp,	10,w
	declare_generic_reg	size_tmp,	11,x
	declare_generic_reg	data_tmp1,	11,x
	declare_generic_reg	data_tmp2,	12,x
	declare_generic_reg	data_tmp3,	13,x

	declare_generic_reg	tmp,		14,x
	declare_generic_reg	tmp1,		15,x

// return
	declare_generic_reg	ret_crc,	0,w

/**********************************************************************
	simd variables
**********************************************************************/
	declare_neon_reg	a0,		0,v
	declare_neon_reg	a1,		1,v
	declare_neon_reg	a2,		2,v
	declare_neon_reg	a3,		3,v
	declare_neon_reg	a4,		4,v

	declare_neon_reg	a5,		16,v
	declare_neon_reg	a6,		17,v
	declare_neon_reg	a7,		18,v
	declare_neon_reg	a8,		19,v

	declare_neon_reg	y5,		20,v
	declare_neon_reg	y6,		21,v
	declare_neon_reg	y7,		22,v
	declare_neon_reg	y8,		23,v

	declare_neon_reg	neon_zero,	24,v
	declare_neon_reg	neon_tmp,	24,v

	declare_neon_reg	k5k0,		25,v
	declare_neon_reg	neon_tmp1,	26,v
	declare_neon_reg	neon_tmp2,	27,v
	declare_neon_reg	neon_tmp3,	28,v

	declare_neon_reg	crc_pmull,	29,v
	declare_neon_reg	neon_crc0,	30,v
	declare_neon_reg	neon_crc1,	31,v

	declare_neon_reg	neon_const0,	5,v
	declare_neon_reg	neon_const1,	6,v
	declare_neon_reg	neon_const2,	7,v

// constants
	.equ	offset_k3k4, 16
	.equ	offset_k5k0, 32
	.equ	offset_poly, 48
	.equ	offset_crc32_const, 64

// pmull fold
.macro pmull_fold
	ldr		x_data_crc0, [x_buf_tmp, 464]
	ldr		x_data_crc1, [x_buf_tmp, 976]
	ldr		x_data_crc2, [x_buf_tmp, 1488]

	pmull		v_a5.1q, v_a1.1d, v_a0.1d
	crc32_u64	w_crc0, w_crc0, x_data_crc0
	crc32_u64	w_crc1, w_crc1, x_data_crc1
	crc32_u64	w_crc2, w_crc2, x_data_crc2

	ldr		x_data_crc0, [x_buf_tmp, 472]
	ldr		x_data_crc1, [x_buf_tmp, 984]
	ldr		x_data_crc2, [x_buf_tmp, 1496]

	pmull		v_a6.1q, v_a2.1d, v_a0.1d
	crc32_u64	w_crc0, w_crc0, x_data_crc0
	crc32_u64	w_crc1, w_crc1, x_data_crc1
	crc32_u64	w_crc2, w_crc2, x_data_crc2

	ldr		x_data_crc0, [x_buf_tmp, 480]
	ldr		x_data_crc1, [x_buf_tmp, 992]
	ldr		x_data_crc2, [x_buf_tmp, 1504]

	pmull		v_a7.1q, v_a3.1d, v_a0.1d
	crc32_u64	w_crc0, w_crc0, x_data_crc0
	crc32_u64	w_crc1, w_crc1, x_data_crc1
	crc32_u64	w_crc2, w_crc2, x_data_crc2

	ldr		x_data_crc0, [x_buf_tmp, 488]
	ldr		x_data_crc1, [x_buf_tmp, 1000]
	ldr		x_data_crc2, [x_buf_tmp, 1512]

	pmull		v_a8.1q, v_a4.1d, v_a0.1d
	crc32_u64	w_crc0, w_crc0, x_data_crc0
	crc32_u64	w_crc1, w_crc1, x_data_crc1
	crc32_u64	w_crc2, w_crc2, x_data_crc2

	ldr		x_data_crc0, [x_buf_tmp, 496]
	ldr		x_data_crc1, [x_buf_tmp, 1008]
	ldr		x_data_crc2, [x_buf_tmp, 1520]

	pmull2		v_a1.1q, v_a1.2d, v_a0.2d
	crc32_u64	w_crc0, w_crc0, x_data_crc0
	crc32_u64	w_crc1, w_crc1, x_data_crc1
	crc32_u64	w_crc2, w_crc2, x_data_crc2

	ld1		{v_y5.4s, v_y6.4s, v_y7.4s, v_y8.4s}, [x_buf_tmp]

	ldr		x_data_crc0, [x_buf_tmp, 504]
	ldr		x_data_crc1, [x_buf_tmp, 1016]
	ldr		x_data_crc2, [x_buf_tmp, 1528]

	pmull2		v_a2.1q, v_a2.2d, v_a0.2d
	crc32_u64	w_crc0, w_crc0, x_data_crc0
	crc32_u64	w_crc1, w_crc1, x_data_crc1
	crc32_u64	w_crc2, w_crc2, x_data_crc2

	pmull2		v_a3.1q, v_a3.2d, v_a0.2d
	pmull2		v_a4.1q, v_a4.2d, v_a0.2d

	eor		v_y5.16b, v_y5.16b, v_a5.16b
	eor		v_y6.16b, v_y6.16b, v_a6.16b
	eor		v_y7.16b, v_y7.16b, v_a7.16b
	eor		v_y8.16b, v_y8.16b, v_a8.16b

	ldr		x_data_crc0, [x_buf_tmp, 512]
	ldr		x_data_crc1, [x_buf_tmp, 1024]
	ldr		x_data_crc2, [x_buf_tmp, 1536]

	eor		v_a1.16b, v_y5.16b, v_a1.16b
	eor		v_a2.16b, v_y6.16b, v_a2.16b
	eor		v_a3.16b, v_y7.16b, v_a3.16b
	eor		v_a4.16b, v_y8.16b, v_a4.16b

	crc32_u64	w_crc0, w_crc0, x_data_crc0
	crc32_u64	w_crc1, w_crc1, x_data_crc1
	crc32_u64	w_crc2, w_crc2, x_data_crc2

	ldr		x_data_crc0, [x_buf_tmp, 520]
	ldr		x_data_crc1, [x_buf_tmp, 1032]
	ldr		x_data_crc2, [x_buf_tmp, 1544]

	crc32_u64	w_crc0, w_crc0, x_data_crc0
	crc32_u64	w_crc1, w_crc1, x_data_crc1
	crc32_u64	w_crc2, w_crc2, x_data_crc2
.endm

// crc32 mix for 2048 byte input data
.macro crc32_mix2048
	fmov		s_a1, w_crc
	movi		v_neon_tmp.4s, 0

	adrp		x_pconst, lanchor_crc32
	add		x_buf_tmp, x_buf, 64

	ldr		x_data_crc0, [x_buf, 512]
	ldr		x_data_crc1, [x_buf, 1024]
	ldr		x_data_crc2, [x_buf, 1536]

	crc32_u64	w_crc0, wzr, x_data_crc0
	crc32_u64	w_crc1, wzr, x_data_crc1
	crc32_u64	w_crc2, wzr, x_data_crc2

#ifdef CRC32
	mvn		v_a1.8b, v_a1.8b
#endif

	ins		v_neon_tmp.s[0], v_a1.s[0]

	ld1		{v_a1.4s, v_a2.4s, v_a3.4s, v_a4.4s}, [x_buf]

	ldr		x_data_crc0, [x_buf, 520]
	ldr		x_data_crc1, [x_buf, 1032]
	ldr		x_data_crc2, [x_buf, 1544]

	eor		v_a1.16b, v_a1.16b, v_neon_tmp.16b
	ldr		q_a0, [x_pconst, #:lo12:lanchor_crc32] // k1k2

	crc32_u64	w_crc0, w_crc0, x_data_crc0
	crc32_u64	w_crc1, w_crc1, x_data_crc1
	crc32_u64	w_crc2, w_crc2, x_data_crc2

// loop start, unroll the loop
	.align 4
	pmull_fold

	add		x_buf_tmp, x_buf_tmp, 64
	pmull_fold

	add		x_buf_tmp, x_buf_tmp, 64
	pmull_fold

	add		x_buf_tmp, x_buf_tmp, 64
	pmull_fold

	add		x_buf_tmp, x_buf_tmp, 64
	pmull_fold

	add		x_buf_tmp, x_buf_tmp, 64
	pmull_fold

	add		x_buf_tmp, x_buf_tmp, 64
	pmull_fold
// loop end

// PMULL: fold into 128-bits
	add		x_pconst, x_pconst, :lo12:lanchor_crc32

	ldr		x_data_crc0, [x_buf, 976]
	ldr		x_data_crc1, [x_buf, 1488]
	ldr		x_data_crc2, [x_buf, 2000]

	ldr		q_a0, [x_pconst, offset_k3k4] // k3k4

	crc32_u64	w_crc0, w_crc0, x_data_crc0
	crc32_u64	w_crc1, w_crc1, x_data_crc1
	crc32_u64	w_crc2, w_crc2, x_data_crc2

	pmull		v_a5.1q, v_a1.1d, v_a0.1d
	pmull2		v_a1.1q, v_a1.2d, v_a0.2d

	eor		v_a1.16b, v_a5.16b, v_a1.16b
	eor		v_a1.16b, v_a1.16b, v_a2.16b

	ldr		x_data_crc0, [x_buf, 984]
	ldr		x_data_crc1, [x_buf, 1496]
	ldr		x_data_crc2, [x_buf, 2008]

	crc32_u64	w_crc0, w_crc0, x_data_crc0
	crc32_u64	w_crc1, w_crc1, x_data_crc1
	crc32_u64	w_crc2, w_crc2, x_data_crc2

	pmull		v_a5.1q, v_a1.1d, v_a0.1d
	pmull2		v_a1.1q, v_a1.2d, v_a0.2d

	ldr		x_data_crc0, [x_buf, 992]
	ldr		x_data_crc1, [x_buf, 1504]
	ldr		x_data_crc2, [x_buf, 2016]

	eor		v_a1.16b, v_a5.16b, v_a1.16b
	eor		v_a1.16b, v_a1.16b, v_a3.16b

	crc32_u64	w_crc0, w_crc0, x_data_crc0
	crc32_u64	w_crc1, w_crc1, x_data_crc1
	crc32_u64	w_crc2, w_crc2, x_data_crc2

	pmull		v_a5.1q, v_a1.1d, v_a0.1d
	pmull2		v_a1.1q, v_a1.2d, v_a0.2d

	ldr		x_data_crc0, [x_buf, 1000]
	ldr		x_data_crc1, [x_buf, 1512]
	ldr		x_data_crc2, [x_buf, 2024]

	eor		v_a1.16b, v_a5.16b, v_a1.16b
	eor		v_a1.16b, v_a1.16b, v_a4.16b

// PMULL: fold 128-bits to 64-bits
	crc32_u64	w_crc0, w_crc0, x_data_crc0
	crc32_u64	w_crc1, w_crc1, x_data_crc1
	crc32_u64	w_crc2, w_crc2, x_data_crc2

	dup		d_a0, v_a0.d[1]
	pmull		v_a2.1q, v_a1.1d, v_a0.1d

	movi		v_neon_zero.4s, 0
	ldr		q_k5k0, [x_pconst, offset_k5k0] // k5k0
	adrp		x_tmp, .lanchor_mask

	ldr		x_data_crc0, [x_buf, 1008]
	ldr		x_data_crc1, [x_buf, 1520]
	ldr		x_data_crc2, [x_buf, 2032]

	ext		v_a1.16b, v_a1.16b, v_neon_zero.16b, #8
	eor		v_a1.16b, v_a2.16b, v_a1.16b
	ldr		q_neon_tmp3, [x_tmp, #:lo12:.lanchor_mask]

	crc32_u64	w_crc0, w_crc0, x_data_crc0
	crc32_u64	w_crc1, w_crc1, x_data_crc1
	crc32_u64	w_crc2, w_crc2, x_data_crc2

	dup		d_a0, v_k5k0.d[1]
	pmull		v_a3.1q, v_a2.1d, v_a0.1d

	ext		v_a2.16b, v_a1.16b, v_neon_zero.16b, #4
	and		v_a1.16b, v_a1.16b, v_neon_tmp3.16b
	pmull		v_a1.1q, v_a1.1d, v_k5k0.1d
	eor		v_a1.16b, v_a2.16b, v_a1.16b

// PMULL: barret reduce to 32-bits
	ldr		q_neon_tmp1, [x_pconst, offset_poly] // poly

	ldr		x_data_crc0, [x_buf, 1016]
	ldr		x_data_crc1, [x_buf, 1528]
	ldr		x_data_crc2, [x_buf, 2040]

	dup		d_neon_tmp2, v_neon_tmp1.d[1]

	crc32_u64	w_crc0, w_crc0, x_data_crc0
	crc32_u64	w_crc1, w_crc1, x_data_crc1
	crc32_u64	w_crc2, w_crc2, x_data_crc2

	and		v_a2.16b, v_a1.16b, v_neon_tmp3.16b
	pmull		v_a2.1q, v_a2.1d, v_neon_tmp2.1d
	and		v_a2.16b, v_neon_tmp3.16b, v_a2.16b
	pmull		v_a2.1q, v_a2.1d, v_neon_tmp1.1d

// crc_pmull result
	eor		v_a1.16b, v_a1.16b, v_a2.16b
	dup		s_crc_pmull, v_a1.s[1]

// merge crc_pmull, crc0, crc1, crc2 using pmull instruction
	fmov		s_neon_crc0, w_crc0
	fmov		s_neon_crc1, w_crc1

	ldr		q_neon_const0, [x_pconst, offset_crc32_const]
	ldr		q_neon_const1, [x_pconst, offset_crc32_const+16]
	ldr		q_neon_const2, [x_pconst, offset_crc32_const+32]

	pmull		v_crc_pmull.1q, v_crc_pmull.1d, v_neon_const0.1d
	pmull		v_neon_crc0.1q, v_neon_crc0.1d, v_neon_const1.1d
	pmull		v_neon_crc1.1q, v_neon_crc1.1d, v_neon_const2.1d

	fmov		x_tmp1, d_neon_crc0
	crc32_u64	w_crc0, wzr, x_tmp1

	fmov		x_tmp1, d_neon_crc1
	crc32_u64	w_crc1, wzr, x_tmp1

	eor		w_ret_crc, w_crc1, w_crc0

	fmov		x_tmp1, d_crc_pmull
	crc32_u64	w_tmp, wzr, x_tmp1

	eor		w_crc2, w_tmp, w_crc2

// handle crc32/crc32c
#ifdef CRC32
	eon		w_ret_crc, w_crc2, w_ret_crc
#else
	eor		w_ret_crc, w_crc2, w_ret_crc
#endif
.endm

// crc32 mix main default
.macro crc32_mix_main_default
	cmp		x_len, 2047
	mov		x_len_saved, x_len
	mov		x_buf_saved, x_buf
	bls		.less_than_2048

	sub		x_buf_iter, x_len, #2048
	stp		x29, x30, [sp, -16]!

	mov		x29, sp
	and		x_buf_iter, x_buf_iter, -2048
	add		x_buf_iter, x_buf_iter, 2048
	add		x_buf_iter, x_buf, x_buf_iter

	.align 4
.loop_mix:
	mov		x_buf, x_buf_saved
	crc32_mix2048

	add		x_buf_saved, x_buf_saved, 2048
	cmp		x_buf_saved, x_buf_iter
	bne		.loop_mix

	and		x_len_saved, x_len_saved, 2047
	cbnz		x_len_saved, .remain_ldp

	ldp		x29, x30, [sp], 16
	ret

	.align 4
.remain_ldp:
	mov		w_crc_tmp, crc
	ldp		x29, x30, [sp], 16
	mov		size, x_len_saved
	mov		buf, x_buf_iter
	b		.crc32_hw_handle

.remain:
	mov		w_crc_tmp, crc
	mov		size, x_len_saved
	mov		buf, x_buf_saved
	b		.crc32_hw_handle

	.align 4
.less_than_2048:
	cbnz		x_len, .remain
	ret

.crc32_hw_handle:
	cmp		size, 63

#ifdef CRC32
	mvn		crc_tmp, crc_tmp
#endif

	bls		.less_than_64
	sub		buf_saved, size, #64
	and		buf_saved, buf_saved, -64
	add		buf_saved, buf_saved, 64
	add		buf_saved, buf, buf_saved

	.align 4
.loop_64:
	ldp		data_tmp1, data_tmp2, [buf]
	ldr		data_tmp3, [buf, 16]
	crc32_u64	crc_tmp, crc_tmp, data_tmp1
	crc32_u64	crc_tmp, crc_tmp, data_tmp2

	ldp		data_tmp1, data_tmp2, [buf, 24]
	add		buf, buf, 64

	crc32_u64	crc_tmp, crc_tmp, data_tmp3
	ldr		data_tmp3, [buf, -24]

	crc32_u64	crc_tmp, crc_tmp, data_tmp1
	crc32_u64	crc_tmp, crc_tmp, data_tmp2

	ldp		data_tmp1, data_tmp2, [buf, -16]
	cmp		buf_saved, buf
	crc32_u64	crc_tmp, crc_tmp, data_tmp3

	crc32_u64	crc_tmp, crc_tmp, data_tmp1
	crc32_u64	crc_tmp, crc_tmp, data_tmp2
	bne		.loop_64

	and		size, size, 63
.less_than_64:
	cmp		size, 7
	bls		.crc32_hw_w

	ldr		data_tmp2, [buf]
	sub		size_tmp, size, #8
	cmp		size_tmp, 7
	crc32_u64	crc_tmp, crc_tmp, data_tmp2
	bls		.crc32_hw_w_pre

	ldr		data_tmp2, [buf, 8]
	sub		data_tmp3, size, #16
	cmp		data_tmp3, 7
	crc32_u64	crc_tmp, crc_tmp, data_tmp2
	bls		.crc32_hw_w_pre

	ldr		data_tmp2, [buf, 16]
	sub		data_tmp3, size, #24
	cmp		data_tmp3, 7
	crc32_u64	crc_tmp, crc_tmp, data_tmp2
	bls		.crc32_hw_w_pre

	ldr		data_tmp2, [buf, 24]
	sub		data_tmp3, size, #32
	cmp		data_tmp3, 7
	crc32_u64	crc_tmp, crc_tmp, data_tmp2
	bls		.crc32_hw_w_pre

	ldr		data_tmp2, [buf, 32]
	sub		data_tmp3, size, #40
	cmp		data_tmp3, 7
	crc32_u64	crc_tmp, crc_tmp, data_tmp2
	bls		.crc32_hw_w_pre

	ldr		data_tmp2, [buf, 40]
	sub		data_tmp3, size, #48
	cmp		data_tmp3, 7
	crc32_u64	crc_tmp, crc_tmp, data_tmp2
	bls		.crc32_hw_w_pre

	ldr		data_tmp2, [buf, 48]
	crc32_u64	crc_tmp, crc_tmp, data_tmp2

.crc32_hw_w_pre:
	and		size_tmp, size_tmp, -8
	and		size, size, 7
	add		size_tmp, size_tmp, 8
	add		buf, buf, size_tmp

.crc32_hw_w:
	cmp		size, 3
	bls		.crc32_hw_h
	ldr		w_data_tmp2, [buf], 4
	sub		size, size, #4
	crc32_u32	crc_tmp, crc_tmp, w_data_tmp2

.crc32_hw_h:
	cmp		size, 1
	bls		.crc32_hw_b
	ldrh		w_data_tmp2, [buf], 2
	sub		size, size, #2
	crc32_u16	crc_tmp, crc_tmp, w_data_tmp2

.crc32_hw_b:
	cbz		size, .crc32_hw_done
	ldrb		w_data_tmp2, [buf]
	crc32_u8	crc_tmp, crc_tmp, w_data_tmp2

.crc32_hw_done:
#ifdef CRC32
	mvn		ret_crc, crc_tmp
#else
	mov		ret_crc, crc_tmp
#endif
	ret
.endm
