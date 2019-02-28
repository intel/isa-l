########################################################################
#  Copyright(c) 2019 Arm Corporation All rights reserved.
#
#  Redistribution and use in source and binary forms, with or without
#  modification, are permitted provided that the following conditions
#  are met:
#    * Redistributions of source code must retain the above copyright
#      notice, this list of conditions and the following disclaimer.
#    * Redistributions in binary form must reproduce the above copyright
#      notice, this list of conditions and the following disclaimer in
#      the documentation and/or other materials provided with the
#      distribution.
#    * Neither the name of Arm Corporation nor the names of its
#      contributors may be used to endorse or promote products derived
#      from this software without specific prior written permission.
#
#  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
#  "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
#  LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
#  A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
#  OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
#  SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
#  LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
#  DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
#  THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
#  (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
#  OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
#########################################################################

.text

.global xor_gen_neon
.type xor_gen_neon, %function

/* int xor_gen_neon(int vects, int len, void **src) */

/* arguments */
w_vects		.req	w0	/* MUST >= 2 */
x_vects		.req	x0
w_len		.req	w1
x_len		.req	x1
x_src		.req	x2

/* returns */
w_ret		.req	w0

/* local variables */
w_in		.req	w1	/* share w_len */
x_src0		.req	x3
x_src0_end  	.req	x4
w_len256	.req	w5	/* share w_len16, w_xor */
x_len256	.req	x5
w_len16		.req	w5
x_len16		.req	x5
w_xor		.req	w5
w_col		.req	w6
x_col		.req	x6
x_src_ptr	.req	x7
x_srcn		.req	x9
x_dst		.req	x10
x_dst_ptr	.req	x11
/* v0  ~ v15: temporary results */
/* v16 ~ v31: next 256 bytes */

/*
 *                 +----------+            +------------------+
 *         src --> |  src[0]  | - src0 ->  |      buffer      | src0_end
 *         --------+----------+            +------------------+
 *           .     |  ......  |
 *           .     +----------+            +------------------+
 *     src_ptr ~~> |  src[n]  | - srcn ~>  |      buffer      |
 *           .     +----------+            +------------------+
 *           .     |  ......  |
 *           .     +----------+
 *           .     | src[v-2] |
 *         --------+----------+            +------------------+
 *     dst_ptr --> | src[v-1] | -- dst --> |      buffer      |
 *                 +----------+            +------------------+
 */

xor_gen_neon:
	add	x_dst_ptr, x_src, x_vects, lsl #3
	ldr	x_dst, [x_dst_ptr, #-8]!
	ldr	x_src0, [x_src]
	add	x_src0_end, x_src0, x_len

	sub	w_vects, w_vects, #2
	mov	w_col, #0

.Loop256_init:
	/* len256 = len - len%256; len %= 256 */
	mov	w_len256, w_len
	and	w_len, w_len, #0xFF
	sub	w_len256, w_len256, w_len

	/* less than 256 byts? */
	cbz	w_len256, .Lloop16_init

	/* save d8 ~ d15 to stack */
	sub	sp, sp, #64
	stp	d8, d9, [sp]
	stp	d10, d11, [sp, #16]
	stp	d12, d13, [sp, #32]
	stp	d14, d15, [sp, #48]

	sub	x_src0_end, x_src0_end, #256

	/* batch process (vects-1)*256 bytes */
.Lloop256:
	ldr	q0,  [x_src0, #16*0]
	ldr	q1,  [x_src0, #16*1]
	ldr	q2,  [x_src0, #16*2]
	ldr	q3,  [x_src0, #16*3]
	ldr	q4,  [x_src0, #16*4]
	ldr	q5,  [x_src0, #16*5]
	ldr	q6,  [x_src0, #16*6]
	ldr	q7,  [x_src0, #16*7]
	ldr	q8,  [x_src0, #16*8]
	ldr	q9,  [x_src0, #16*9]
	ldr	q10, [x_src0, #16*10]
	ldr	q11, [x_src0, #16*11]
	ldr	q12, [x_src0, #16*12]
	ldr	q13, [x_src0, #16*13]
	ldr	q14, [x_src0, #16*14]
	ldr	q15, [x_src0, #16*15]
	add	x_src0, x_src0, #256

	cbz	w_vects, .Lloop256_vects_end

	add	x_src_ptr, x_src, #8
.Lloop256_vects:
	ldr	x_srcn, [x_src_ptr], #8
	add	x_srcn, x_srcn, x_col
	cmp	x_src_ptr, x_dst_ptr

	ldr	q16, [x_srcn, #16*0]
	ldr	q17, [x_srcn, #16*1]
	ldr	q18, [x_srcn, #16*2]
	ldr	q19, [x_srcn, #16*3]
	ldr	q20, [x_srcn, #16*4]
	ldr	q21, [x_srcn, #16*5]
	ldr	q22, [x_srcn, #16*6]
	ldr	q23, [x_srcn, #16*7]
	ldr	q24, [x_srcn, #16*8]
	ldr	q25, [x_srcn, #16*9]
	ldr	q26, [x_srcn, #16*10]
	ldr	q27, [x_srcn, #16*11]
	ldr	q28, [x_srcn, #16*12]
	ldr	q29, [x_srcn, #16*13]
	ldr	q30, [x_srcn, #16*14]
	ldr	q31, [x_srcn, #16*15]

	eor	v0.16b,  v0.16b,  v16.16b
	eor	v1.16b,  v1.16b,  v17.16b
	eor	v2.16b,  v2.16b,  v18.16b
	eor	v3.16b,  v3.16b,  v19.16b
	eor	v4.16b,  v4.16b,  v20.16b
	eor	v5.16b,  v5.16b,  v21.16b
	eor	v6.16b,  v6.16b,  v22.16b
	eor	v7.16b,  v7.16b,  v23.16b
	eor	v8.16b,  v8.16b,  v24.16b
	eor	v9.16b,  v9.16b,  v25.16b
	eor	v10.16b, v10.16b, v26.16b
	eor	v11.16b, v11.16b, v27.16b
	eor	v12.16b, v12.16b, v28.16b
	eor	v13.16b, v13.16b, v29.16b
	eor	v14.16b, v14.16b, v30.16b
	eor	v15.16b, v15.16b, v31.16b

	bne	.Lloop256_vects

.Lloop256_vects_end:
	str	q0,  [x_dst, #16*0]
	str	q1,  [x_dst, #16*1]
	str	q2,  [x_dst, #16*2]
	str	q3,  [x_dst, #16*3]
	str	q4,  [x_dst, #16*4]
	str	q5,  [x_dst, #16*5]
	str	q6,  [x_dst, #16*6]
	str	q7,  [x_dst, #16*7]
	str	q8,  [x_dst, #16*8]
	str	q9,  [x_dst, #16*9]
	str	q10, [x_dst, #16*10]
	str	q11, [x_dst, #16*11]
	str	q12, [x_dst, #16*12]
	str	q13, [x_dst, #16*13]
	str	q14, [x_dst, #16*14]
	str	q15, [x_dst, #16*15]

	cmp	x_src0, x_src0_end
	add	x_dst, x_dst, #256
	add	w_col, w_col, #256
	bls	.Lloop256

.Lloop256_end:
	/* restore d8 ~ d15 */
	ldp	d8, d9, [sp]
	ldp	d10, d11, [sp, #16]
	ldp	d12, d13, [sp, #32]
	ldp	d14, d15, [sp, #48]
	add	sp, sp, #64

	add	x_src0_end, x_src0_end, #256

.Lloop16_init:
	/* len16 = len - len%16; len %= 16 */
	mov	w_len16, w_len
	and	w_len, w_len, #0xF
	sub	w_len16, w_len16, w_len

	/* less than 16 bytes? */
	cbz	w_len16, .Lloop1_init

	sub	x_src0_end, x_src0_end, #16

	/* batch process (vects-1)*16 bytes */
.Lloop16:
	ldr	q0, [x_src0], #16
	cbz	w_vects, .Lloop16_vects_end

	add	x_src_ptr, x_src, #8
.Lloop16_vects:
	ldr	x_srcn, [x_src_ptr], #8
	cmp	x_src_ptr, x_dst_ptr
	ldr	q1, [x_srcn, x_col]
	eor	v0.16b, v0.16b, v1.16b
	bne	.Lloop16_vects

.Lloop16_vects_end:
	cmp	x_src0, x_src0_end
	str	q0, [x_dst], #16
	add	w_col, w_col, #16
	bls	.Lloop16

.Loop16_end:
	add	x_src0_end, x_src0_end, #16

.Lloop1_init:
	cbnz	w_len, .Lloop1
	mov	w_ret, #0
	ret

	/* batch process (vects-1)*1 bytes */
.Lloop1:
	ldrb	w_xor, [x_src0], #1
	cbz	w_vects, .Lloop1_vects_end

	add	x_src_ptr, x_src, #8
.Lloop1_vects:
	ldr	x_srcn, [x_src_ptr], #8
	cmp	x_src_ptr, x_dst_ptr
	ldrb	w_in, [x_srcn, x_col]
	eor	w_xor, w_xor, w_in
	bne	.Lloop1_vects

.Lloop1_vects_end:
	cmp	x_src0, x_src0_end
	strb	w_xor, [x_dst], #1
	add	w_col, w_col, #1
	bne	.Lloop1

.Loop1_end:
	mov	w_ret, #0
	ret
