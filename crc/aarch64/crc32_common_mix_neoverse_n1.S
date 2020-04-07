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


.macro	declare_var_vector_reg name:req,reg:req
	\name\()_q	.req	q\reg
	\name\()_v	.req	v\reg
	\name\()_s	.req	s\reg
	\name\()_d	.req	d\reg
.endm
	declare_var_vector_reg	k1k2,20
	declare_var_vector_reg	k3k4,21
	declare_var_vector_reg	poly,22
	declare_var_vector_reg	k5k0,23
	declare_var_vector_reg	mask,24
	declare_var_vector_reg	fold_poly,25

	declare_var_vector_reg	tmp0,0
	declare_var_vector_reg	tmp1,1
	declare_var_vector_reg	tmp2,2
	declare_var_vector_reg	tmp3,3
	declare_var_vector_reg	tmp4,4
	declare_var_vector_reg	tmp5,5
	declare_var_vector_reg	tmp6,6
	declare_var_vector_reg	tmp7,7
	declare_var_vector_reg	pmull_data0,16
	declare_var_vector_reg	pmull_data1,17
	declare_var_vector_reg	pmull_data2,18
	declare_var_vector_reg	pmull_data3,19

	vzr		.req	v26

	const_addr	.req	x3
	crc_blk_ptr	.req	x4
	pmull_blk_ptr	.req	x5
	crc_data0	.req	x6
	crc_data1	.req	x7
	crc_data2	.req	x9
	crc_data3	.req	x10
	wPmull		.req	w11
	xPmull		.req	x11

	data0		.req	x4
	data1		.req	x5
	data2		.req	x6
	data3		.req	x7
	wdata		.req	w4

.macro	pmull_fold

	pmull2		tmp4_v.1q, tmp0_v.2d, k1k2_v.2d
	pmull2		tmp5_v.1q, tmp1_v.2d, k1k2_v.2d
	pmull2		tmp6_v.1q, tmp2_v.2d, k1k2_v.2d
	pmull2		tmp7_v.1q, tmp3_v.2d, k1k2_v.2d

	pmull		tmp0_v.1q, tmp0_v.1d, k1k2_v.1d
	pmull		tmp1_v.1q, tmp1_v.1d, k1k2_v.1d
	pmull		tmp2_v.1q, tmp2_v.1d, k1k2_v.1d
	pmull		tmp3_v.1q, tmp3_v.1d, k1k2_v.1d
	ld1		{pmull_data0_v.16b-pmull_data3_v.16b},[pmull_blk_ptr],#64
	crc32_u64	wCRC,wCRC,crc_data0
	crc32_u64	wCRC,wCRC,crc_data1
	ldp		crc_data0,crc_data1,[crc_blk_ptr],16
	crc32_u64	wCRC,wCRC,crc_data2
	crc32_u64	wCRC,wCRC,crc_data3
	ldp		crc_data2,crc_data3,[crc_blk_ptr],16

	eor		tmp0_v.16b, tmp0_v.16b, tmp4_v.16b
	eor		tmp1_v.16b, tmp1_v.16b, tmp5_v.16b
	eor		tmp2_v.16b, tmp2_v.16b, tmp6_v.16b
	eor		tmp3_v.16b, tmp3_v.16b, tmp7_v.16b

	crc32_u64	wCRC,wCRC,crc_data0
	crc32_u64	wCRC,wCRC,crc_data1
	ldp		crc_data0,crc_data1,[crc_blk_ptr],16
	crc32_u64	wCRC,wCRC,crc_data2
	crc32_u64	wCRC,wCRC,crc_data3
	ldp		crc_data2,crc_data3,[crc_blk_ptr],16
	eor		tmp0_v.16b, tmp0_v.16b, v16.16b
	eor		tmp1_v.16b, tmp1_v.16b, v17.16b
	eor		tmp2_v.16b, tmp2_v.16b, v18.16b
	eor		tmp3_v.16b, tmp3_v.16b, v19.16b
	crc32_u64	wCRC,wCRC,crc_data0
	crc32_u64	wCRC,wCRC,crc_data1
	ldp		crc_data0,crc_data1,[crc_blk_ptr],16
	crc32_u64	wCRC,wCRC,crc_data2
	crc32_u64	wCRC,wCRC,crc_data3
	ldp		crc_data2,crc_data3,[crc_blk_ptr],16
.endm



.macro	crc32_common_mix	poly_type
	.set		MIX_BLK_SIZE,2048

.ifc	\poly_type,crc32
	mvn		wCRC,wCRC
.endif
	cmp		LEN,MIX_BLK_SIZE-1
	adr		const_addr, .Lconstants
	bls		start_final
	ld1		{k1k2_v.16b,k3k4_v.16b,poly_v.16b},[const_addr],#48
	movi		vzr.16b, #0
	ld1		{k5k0_v.8b,mask_v.8b,fold_poly_v.8b},[const_addr]

loop_2048:
	ld1		{tmp0_v.16b-tmp3_v.16b}, [BUF]
	add		pmull_blk_ptr,BUF,0x40
	add		crc_blk_ptr, BUF,512
	mov		tmp4_v.16b,vzr.16b
	fmov		tmp4_s, wCRC
	ldp		crc_data0,crc_data1,[crc_blk_ptr],16
	eor		tmp0_v.16b,tmp0_v.16b,tmp4_v.16b
	mov		wCRC, 0
	sub		LEN,LEN,MIX_BLK_SIZE
	cmp		LEN,MIX_BLK_SIZE
	ldp		crc_data2,crc_data3,[crc_blk_ptr],16
	crc32_u64	wCRC,wCRC,crc_data0
	crc32_u64	wCRC,wCRC,crc_data1
	ldp		crc_data0,crc_data1,[crc_blk_ptr],16
	crc32_u64	wCRC,wCRC,crc_data2
	crc32_u64	wCRC,wCRC,crc_data3
	ldp		crc_data2,crc_data3,[crc_blk_ptr],16

	pmull_fold
	pmull_fold
	pmull_fold
	pmull_fold
	pmull_fold
	pmull_fold
	pmull_fold

	/* Folding cache line into 128bit */
	pmull2		tmp4_v.1q, tmp0_v.2d, k3k4_v.2d
	crc32_u64	wCRC,wCRC,crc_data0
	crc32_u64	wCRC,wCRC,crc_data1
	ldp		crc_data0,crc_data1,[crc_blk_ptr],16
	crc32_u64	wCRC,wCRC,crc_data2
	crc32_u64	wCRC,wCRC,crc_data3
	ldp		crc_data2,crc_data3,[crc_blk_ptr],16
	crc32_u64	wCRC,wCRC,crc_data0
	crc32_u64	wCRC,wCRC,crc_data1
	ldp		crc_data0,crc_data1,[crc_blk_ptr],16
	pmull		tmp0_v.1q, tmp0_v.1d, k3k4_v.1d
	crc32_u64	wCRC,wCRC,crc_data2
	crc32_u64	wCRC,wCRC,crc_data3
	ldp		crc_data2,crc_data3,[crc_blk_ptr],16
	crc32_u64	wCRC,wCRC,crc_data0
	crc32_u64	wCRC,wCRC,crc_data1
	ldp		crc_data0,crc_data1,[crc_blk_ptr],16
	crc32_u64	wCRC,wCRC,crc_data2
	crc32_u64	wCRC,wCRC,crc_data3
	ldp		crc_data2,crc_data3,[crc_blk_ptr],16
	eor		tmp0_v.16b, tmp0_v.16b, tmp4_v.16b
	crc32_u64	wCRC,wCRC,crc_data0
	crc32_u64	wCRC,wCRC,crc_data1
	ldp		crc_data0,crc_data1,[crc_blk_ptr],16
	eor		tmp0_v.16b, tmp0_v.16b, tmp1_v.16b
	crc32_u64	wCRC,wCRC,crc_data2
	crc32_u64	wCRC,wCRC,crc_data3
	ldp		crc_data2,crc_data3,[crc_blk_ptr],16
	crc32_u64	wCRC,wCRC,crc_data0
	crc32_u64	wCRC,wCRC,crc_data1
	ldp		crc_data0,crc_data1,[crc_blk_ptr],16
	pmull2		tmp4_v.1q, tmp0_v.2d, k3k4_v.2d
	crc32_u64	wCRC,wCRC,crc_data2
	crc32_u64	wCRC,wCRC,crc_data3
	ldp		crc_data2,crc_data3,[crc_blk_ptr],16
	pmull		tmp0_v.1q, tmp0_v.1d, k3k4_v.1d
	crc32_u64	wCRC,wCRC,crc_data0
	crc32_u64	wCRC,wCRC,crc_data1
	ldp		crc_data0,crc_data1,[crc_blk_ptr],16
	eor		tmp0_v.16b, tmp0_v.16b, tmp4_v.16b
	crc32_u64	wCRC,wCRC,crc_data2
	crc32_u64	wCRC,wCRC,crc_data3
	ldp		crc_data2,crc_data3,[crc_blk_ptr],16
	crc32_u64	wCRC,wCRC,crc_data0
	crc32_u64	wCRC,wCRC,crc_data1
	ldp		crc_data0,crc_data1,[crc_blk_ptr],16
	crc32_u64	wCRC,wCRC,crc_data2
	crc32_u64	wCRC,wCRC,crc_data3
	ldp		crc_data2,crc_data3,[crc_blk_ptr],16
	eor		tmp0_v.16b, tmp0_v.16b, tmp2_v.16b
	crc32_u64	wCRC,wCRC,crc_data0
	crc32_u64	wCRC,wCRC,crc_data1
	ldp		crc_data0,crc_data1,[crc_blk_ptr],16
	pmull2		tmp4_v.1q, tmp0_v.2d, k3k4_v.2d
	crc32_u64	wCRC,wCRC,crc_data2
	crc32_u64	wCRC,wCRC,crc_data3
	ldp		crc_data2,crc_data3,[crc_blk_ptr],16
	crc32_u64	wCRC,wCRC,crc_data0
	crc32_u64	wCRC,wCRC,crc_data1
	ldp		crc_data0,crc_data1,[crc_blk_ptr],16
	crc32_u64	wCRC,wCRC,crc_data2
	crc32_u64	wCRC,wCRC,crc_data3
	ldp		crc_data2,crc_data3,[crc_blk_ptr],16
	pmull		tmp0_v.1q, tmp0_v.1d, k3k4_v.1d
	crc32_u64	wCRC,wCRC,crc_data0
	crc32_u64	wCRC,wCRC,crc_data1
	ldp		crc_data0,crc_data1,[crc_blk_ptr],16
	crc32_u64	wCRC,wCRC,crc_data2
	crc32_u64	wCRC,wCRC,crc_data3
	ldp		crc_data2,crc_data3,[crc_blk_ptr],16
	eor		tmp0_v.16b, tmp0_v.16b, tmp4_v.16b
	crc32_u64	wCRC,wCRC,crc_data0
	crc32_u64	wCRC,wCRC,crc_data1
	ldp		crc_data0,crc_data1,[crc_blk_ptr],16
	crc32_u64	wCRC,wCRC,crc_data2
	crc32_u64	wCRC,wCRC,crc_data3
	ldp		crc_data2,crc_data3,[crc_blk_ptr],16
	crc32_u64	wCRC,wCRC,crc_data0
	crc32_u64	wCRC,wCRC,crc_data1
	ldp		crc_data0,crc_data1,[crc_blk_ptr],16
	eor		tmp0_v.16b, tmp0_v.16b, tmp3_v.16b
	crc32_u64	wCRC,wCRC,crc_data2
	crc32_u64	wCRC,wCRC,crc_data3
	ldp		crc_data2,crc_data3,[crc_blk_ptr],16
	crc32_u64	wCRC,wCRC,crc_data0
	crc32_u64	wCRC,wCRC,crc_data1
	ldp		crc_data0,crc_data1,[crc_blk_ptr],16


	/**
	 * perform the last 64 bit fold, also
	 * adds 32 zeroes to the input stream
	 */
	ext		tmp1_v.16b, tmp0_v.16b, tmp0_v.16b, #8
	crc32_u64	wCRC,wCRC,crc_data2
	crc32_u64	wCRC,wCRC,crc_data3
	ldp		crc_data2,crc_data3,[crc_blk_ptr],16
	crc32_u64	wCRC,wCRC,crc_data0
	crc32_u64	wCRC,wCRC,crc_data1
	ldp		crc_data0,crc_data1,[crc_blk_ptr],16
	pmull2		tmp1_v.1q, tmp1_v.2d, k3k4_v.2d
	crc32_u64	wCRC,wCRC,crc_data2
	crc32_u64	wCRC,wCRC,crc_data3
	ldp		crc_data2,crc_data3,[crc_blk_ptr],16
	crc32_u64	wCRC,wCRC,crc_data0
	crc32_u64	wCRC,wCRC,crc_data1
	ldp		crc_data0,crc_data1,[crc_blk_ptr],16
	crc32_u64	wCRC,wCRC,crc_data2
	crc32_u64	wCRC,wCRC,crc_data3
	ldp		crc_data2,crc_data3,[crc_blk_ptr],16
	ext		tmp0_v.16b, tmp0_v.16b, vzr.16b, #8
	crc32_u64	wCRC,wCRC,crc_data0
	crc32_u64	wCRC,wCRC,crc_data1
	ldp		crc_data0,crc_data1,[crc_blk_ptr],16
	crc32_u64	wCRC,wCRC,crc_data2
	crc32_u64	wCRC,wCRC,crc_data3
	ldp		crc_data2,crc_data3,[crc_blk_ptr],16
	eor		tmp0_v.16b, tmp0_v.16b, tmp1_v.16b
	crc32_u64	wCRC,wCRC,crc_data0
	crc32_u64	wCRC,wCRC,crc_data1
	ldp		crc_data0,crc_data1,[crc_blk_ptr],16
	crc32_u64	wCRC,wCRC,crc_data2
	crc32_u64	wCRC,wCRC,crc_data3
	ldp		crc_data2,crc_data3,[crc_blk_ptr],16

	/* final 32-bit fold */
	ext		tmp1_v.16b, tmp0_v.16b, vzr.16b, #4
	and		tmp0_v.16b, tmp0_v.16b, mask_v.16b
	crc32_u64	wCRC,wCRC,crc_data0
	crc32_u64	wCRC,wCRC,crc_data1
	ldp		crc_data0,crc_data1,[crc_blk_ptr],16
	pmull		tmp0_v.1q, tmp0_v.1d, k5k0_v.1d
	crc32_u64	wCRC,wCRC,crc_data2
	crc32_u64	wCRC,wCRC,crc_data3
	ldp		crc_data2,crc_data3,[crc_blk_ptr],16
	crc32_u64	wCRC,wCRC,crc_data0
	crc32_u64	wCRC,wCRC,crc_data1
	ldp		crc_data0,crc_data1,[crc_blk_ptr],16
	crc32_u64	wCRC,wCRC,crc_data2
	crc32_u64	wCRC,wCRC,crc_data3
	ldp		crc_data2,crc_data3,[crc_blk_ptr],16
	eor		tmp0_v.16b, tmp0_v.16b, tmp1_v.16b

	/**
	 * Finish up with the bit-reversed barrett
	 * reduction 64 ==> 32 bits
	 */
	crc32_u64	wCRC,wCRC,crc_data0
	crc32_u64	wCRC,wCRC,crc_data1
	and		tmp1_v.16b, tmp0_v.16b, mask_v.16b
	ldp		crc_data0,crc_data1,[crc_blk_ptr],16
	ext		tmp1_v.16b, vzr.16b, tmp1_v.16b, #8
	crc32_u64	wCRC,wCRC,crc_data2
	crc32_u64	wCRC,wCRC,crc_data3
	pmull2		tmp1_v.1q, tmp1_v.2d, poly_v.2d
	ldp		crc_data2,crc_data3,[crc_blk_ptr],16
	crc32_u64	wCRC,wCRC,crc_data0
	crc32_u64	wCRC,wCRC,crc_data1
	ldp		crc_data0,crc_data1,[crc_blk_ptr],16
	crc32_u64	wCRC,wCRC,crc_data2
	crc32_u64	wCRC,wCRC,crc_data3
	and		tmp1_v.16b, tmp1_v.16b, mask_v.16b
	ldp		crc_data2,crc_data3,[crc_blk_ptr],16
	pmull		tmp1_v.1q, tmp1_v.1d, poly_v.1d
	crc32_u64	wCRC,wCRC,crc_data0
	crc32_u64	wCRC,wCRC,crc_data1
	ldp		crc_data0,crc_data1,[crc_blk_ptr],16
	eor		tmp0_v.16b, tmp0_v.16b, tmp1_v.16b
	crc32_u64	wCRC,wCRC,crc_data2
	crc32_u64	wCRC,wCRC,crc_data3
	mov		tmp4_v.16b,vzr.16b
	mov		tmp4_v.s[0], tmp0_v.s[1]
	ldp		crc_data2,crc_data3,[crc_blk_ptr],16
	crc32_u64	wCRC,wCRC,crc_data0
	crc32_u64	wCRC,wCRC,crc_data1
	ldp		crc_data0,crc_data1,[crc_blk_ptr],16
	crc32_u64	wCRC,wCRC,crc_data2
	crc32_u64	wCRC,wCRC,crc_data3
	ldp		crc_data2,crc_data3,[crc_blk_ptr],16
	crc32_u64	wCRC,wCRC,crc_data0
	crc32_u64	wCRC,wCRC,crc_data1
	ldp		crc_data0,crc_data1,[crc_blk_ptr],16
	crc32_u64	wCRC,wCRC,crc_data2
	crc32_u64	wCRC,wCRC,crc_data3
	ldp		crc_data2,crc_data3,[crc_blk_ptr],16
	crc32_u64	wCRC,wCRC,crc_data0
	crc32_u64	wCRC,wCRC,crc_data1
	ldp		crc_data0,crc_data1,[crc_blk_ptr],16
	crc32_u64	wCRC,wCRC,crc_data2
	crc32_u64	wCRC,wCRC,crc_data3
	ldp		crc_data2,crc_data3,[crc_blk_ptr],16

	crc32_u64	wCRC,wCRC,crc_data0
	crc32_u64	wCRC,wCRC,crc_data1
	crc32_u64	wCRC,wCRC,crc_data2
	crc32_u64	wCRC,wCRC,crc_data3

	pmull		tmp4_v.1q, tmp4_v.1d, fold_poly_v.1d
	add		BUF,BUF,MIX_BLK_SIZE
	fmov		xPmull, tmp4_d
	crc32_u64  	wPmull, wzr, xPmull
	eor		wCRC, wPmull, wCRC
	bge		loop_2048
start_final:
	cmp		LEN, 63
	bls		.loop_16B
.loop_64B:
	ldp		data0, data1, [BUF],#16
	sub		LEN,LEN,#64
	ldp		data2, data3, [BUF],#16
	cmp		LEN,#64
	crc32_u64	wCRC, wCRC, data0
	crc32_u64	wCRC, wCRC, data1
	ldp		data0, data1, [BUF],#16
	crc32_u64	wCRC, wCRC, data2
	crc32_u64	wCRC, wCRC, data3
	ldp		data2, data3, [BUF],#16
	crc32_u64	wCRC, wCRC, data0
	crc32_u64	wCRC, wCRC, data1
	crc32_u64	wCRC, wCRC, data2
	crc32_u64	wCRC, wCRC, data3
	bge		.loop_64B

.loop_16B:
	cmp		LEN, 15
	bls		.less_16B
	ldp		data0, data1, [BUF],#16
	sub		LEN,LEN,#16
	cmp		LEN,15
	crc32_u64	wCRC, wCRC, data0
	crc32_u64	wCRC, wCRC, data1
	bls		.less_16B
	ldp		data0, data1, [BUF],#16
	sub		LEN,LEN,#16
	cmp		LEN,15
	crc32_u64	wCRC, wCRC, data0
	crc32_u64	wCRC, wCRC, data1
	bls		.less_16B
	ldp		data0, data1, [BUF],#16
	sub		LEN,LEN,#16   //MUST less than 16B
	crc32_u64	wCRC, wCRC, data0
	crc32_u64	wCRC, wCRC, data1
.less_16B:
	cmp		LEN, 7
	bls		.less_8B
	ldr		data0, [BUF], 8
	sub		LEN, LEN, #8
	crc32_u64	wCRC, wCRC, data0
.less_8B:
	cmp		LEN, 3
	bls		.less_4B
	ldr		wdata, [BUF], 4
	sub		LEN, LEN, #4
	crc32_u32	wCRC, wCRC, wdata
.less_4B:
	cmp		LEN, 1
	bls		.less_2B
	ldrh		wdata, [BUF], 2
	sub		LEN, LEN, #2
	crc32_u16	wCRC, wCRC, wdata
.less_2B:
	cbz		LEN, .finish_exit
	ldrb		wdata, [BUF]
	crc32_u8	wCRC, wCRC, wdata
.finish_exit:
.ifc	\poly_type,crc32
	mvn		w0, wCRC
.else
	mov		w0, wCRC
.endif
	ret
.endm

