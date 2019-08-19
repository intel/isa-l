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
	.arch armv8-a+crypto
	.text
	.align	3

/*
Macros
*/

.macro	declare_var_vector_reg name:req,reg:req
	\name\()_q	.req	q\reg
	\name\()_v	.req	v\reg
	\name\()_s	.req	s\reg
	\name\()_d	.req	d\reg
.endm

.macro	mod_adler	dest:req,tmp:req
	umull	\tmp\()_x,\dest,const_div1
	lsr	\tmp\()_x,\tmp\()_x,47
	msub	\dest,\tmp,const_div2,\dest
.endm

/*
	uint32_t adler32_neon(uint32_t adler32, uint8_t * start, uint32_t length);
*/
/*
Arguements list
*/
	adler32	.req	w0
	start	.req	x1
	length	.req	x2
	.global	adler32_neon
	.type	adler32_neon, %function
adler32_neon:
/*
local variables
*/
	declare_var_vector_reg factor0	, 6
	declare_var_vector_reg factor1	, 7
	declare_var_vector_reg d0	, 4
	declare_var_vector_reg d1	, 5
	declare_var_vector_reg adacc	, 2
	declare_var_vector_reg s2acc	, 3
	declare_var_vector_reg zero	, 16
	declare_var_vector_reg adler	, 17
	declare_var_vector_reg back_d0	, 18
	declare_var_vector_reg back_d1	, 19
	declare_var_vector_reg sum2	, 20
	declare_var_vector_reg tmp2	, 20

	adler0	.req 	w4
	adler1	.req 	w5
	adler0_x	.req 	x4
	adler1_x	.req 	x5
	end	.req	x0
	tmp	.req 	w8
	tmp_x	.req	x8
	tmp1_x	.req	x9
	loop_cnt	.req x10
	loop_const	.req x11
	const_div1	.req w6
	const_div2	.req w7
	mov     const_div1, 32881
        movk    const_div1, 0x8007, lsl 16
        mov     const_div2, 65521
	and	adler0, adler32, 0xffff
	lsr	adler1, adler32, 16

	lsr	loop_cnt,length,5
	adrp 	x3,factors
	add  	x3,x3,:lo12:factors
	ld1 	{factor0_v.16b-factor1_v.16b},[x3]

	add	end,start,length
	cbz	loop_cnt,final_accum32
	ld1 	{back_d0_v.16b-back_d1_v.16b},[start]
	mov 	loop_const,173

	movi	v16.4s,0




great_than_32:
	cmp	loop_cnt,173
	csel	loop_const,loop_cnt,loop_const,le
	mov	adacc_v.16b,zero_v.16b
	mov	s2acc_v.16b,zero_v.16b
	ins	adacc_v.s[0],adler0
	ins	s2acc_v.s[0],adler1
	add	tmp_x,start,loop_const,lsl 5

accum32_neon:
	add	start,start,32
	mov	d0_v.16b,back_d0_v.16b
	mov	d1_v.16b,back_d1_v.16b
	ld1 	{back_d0_v.16b-back_d1_v.16b},[start]

	shl	tmp2_v.4s,adacc_v.4s,5
	add	s2acc_v.4s,s2acc_v.4s,tmp2_v.4s

	uaddlp	adler_v.8h,d0_v.16b
	uadalp	adler_v.8h,d1_v.16b
	uadalp	adacc_v.4s,adler_v.8h

	umull	sum2_v.8h,factor0_v.8b ,d0_v.8b
	umlal2	sum2_v.8h,factor0_v.16b,d0_v.16b
	umlal	sum2_v.8h,factor1_v.8b ,d1_v.8b
	umlal2	sum2_v.8h,factor1_v.16b,d1_v.16b
	uadalp	s2acc_v.4s,sum2_v.8h

	cmp	start,tmp_x
	bne	accum32_neon

	uaddlv	adacc_d,adacc_v.4s
	uaddlv	s2acc_d,s2acc_v.4s
	fmov 	adler0_x,adacc_d
	fmov 	adler1_x,s2acc_d

	mod_adler	adler0,tmp
	mod_adler	adler1,tmp
	sub	loop_cnt,loop_cnt,loop_const
	cbnz	loop_cnt,great_than_32

final_accum32:
	and	length,length,31
	cbz	length,end_func

accum32_body:
	cmp 	start,end
	beq 	end_func
	ldrb 	tmp,[start],1
	add	adler0,adler0,tmp
	add	adler1,adler1,adler0
	b	accum32_body

end_func:
	mod_adler	adler0,tmp
	mod_adler	adler1,tmp
	orr w0,adler0,adler1,lsl 16
	ret

	.size	adler32_neon, .-adler32_neon
	.section	.rodata.cst16,"aM",@progbits,16
	.align	4
factors:
	.quad 0x191a1b1c1d1e1f20
	.quad 0x1112131415161718
	.quad 0x090a0b0c0d0e0f10
	.quad 0x0102030405060708

