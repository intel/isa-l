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

.global pq_gen_neon
.type pq_gen_neon, %function

/* int pq_gen_neon(int vects, int len, void **src) */

/* arguments */
w_vects		.req	w0	/* MUST >= 3 */
x_vects		.req	x0
w_len		.req	w1	/* MUST be 16x bytes */
x_len		.req	x1
x_src		.req	x2

/* returns */
w_ret		.req	w0

/* local variables */
x_dst_p		.req	x3
x_dst_q		.req	x4
x_dst_q_end  	.req	x5
w_col		.req	w6
x_col		.req	x6
x_src_ptr	.req	x7
x_src_ptr_end	.req	x9
x_src_last	.req	x10
x_srcn		.req	x11
/* vectors */
/* v0  ~ v7 : temporary p */
/* v8  ~ v15: temporary q */
/* v16 ~ v23: next 128 bytes */
v_mask0		.req	v24
v_mask1		.req	v25
v_mask2		.req	v26
v_mask3		.req	v27
v_gf8poly	.req	v28
v_0x80		.req	v29

/*
 * src_ptr_end -->
 *          -------+----------+
 *           .     |  src[0]  |
 *           .     +----------+            +------------------+
 *     src_ptr --> |  src[1]  | - srcn ->  |     buffer       |
 *           .     +----------+            +------------------+
 *           .     |  ......  |
 *           .     +----------+
 *           .     | src[v-4] |
 *          -------+----------+  src_last  +------------------+
 *        src  --> | src[v-3] | ---------> |      buffer      |
 *                 +----------+            +------------------+
 *                 | src[v-2] | - dst_p -> |      buffer      |
 *                 +----------+            +------------------+
 *                 | src[v-1] | - dst_q -> |      buffer      | dst_q_end
 *                 +----------+            +------------------+
 */

pq_gen_neon:
	sub	x_src_ptr_end, x_src, #8

	sub	w_vects, w_vects, #3
	add	x_src, x_src, x_vects, lsl #3

	ldr	x_src_last, [x_src]
	ldp	x_dst_p, x_dst_q, [x_src, #8]

	add	x_dst_q_end, x_dst_q, x_len

	mov	w_col, #0
	movi	v_gf8poly.16b, #0x1D
	movi	v_0x80.16b, #0x80

.Lloop128_init:
	/* less than 128 byts? */
	cmp	w_len, #128
	blo	.Lloop16_init

	/* save d8 ~ d15 to stack */
	sub	sp, sp, #64
	stp	d8,  d9,  [sp]
	stp	d10, d11, [sp, #16]
	stp	d12, d13, [sp, #32]
	stp	d14, d15, [sp, #48]

	sub	x_dst_q_end, x_dst_q_end, #128

	/* batch process (vects-2)*128 bytes */
	/* v0~v7: p;  v8~v15: q;  v16~v23: in */
.Lloop128:
	ldr	q0, [x_src_last, #16*0]
	ldr	q1, [x_src_last, #16*1]
	ldr	q2, [x_src_last, #16*2]
	ldr	q3, [x_src_last, #16*3]
	ldr	q4, [x_src_last, #16*4]
	ldr	q5, [x_src_last, #16*5]
	ldr	q6, [x_src_last, #16*6]
	ldr	q7, [x_src_last, #16*7]
	add	x_src_last, x_src_last, #128

	mov	v8.16b,  v0.16b
	mov	v9.16b,  v1.16b
	mov	v10.16b, v2.16b
	mov	v11.16b, v3.16b
	mov	v12.16b, v4.16b
	mov	v13.16b, v5.16b
	mov	v14.16b, v6.16b
	mov	v15.16b, v7.16b

	cbz	w_vects, .Lloop128_vects_end

	sub	x_src_ptr, x_src, #8
.Lloop128_vects:
	ldr	x_srcn, [x_src_ptr], #-8
	add	x_srcn, x_srcn, x_col
	cmp	x_src_ptr, x_src_ptr_end

	ldr	q16, [x_srcn, #16*0]
	ldr	q17, [x_srcn, #16*1]
	ldr	q18, [x_srcn, #16*2]
	ldr	q19, [x_srcn, #16*3]
	ldr	q20, [x_srcn, #16*4]
	ldr	q21, [x_srcn, #16*5]
	ldr	q22, [x_srcn, #16*6]
	ldr	q23, [x_srcn, #16*7]

	eor	v0.16b, v0.16b, v16.16b
	eor	v1.16b, v1.16b, v17.16b
	eor	v2.16b, v2.16b, v18.16b
	eor	v3.16b, v3.16b, v19.16b
	eor	v4.16b, v4.16b, v20.16b
	eor	v5.16b, v5.16b, v21.16b
	eor	v6.16b, v6.16b, v22.16b
	eor	v7.16b, v7.16b, v23.16b

	cmhs	v_mask0.16b, v8.16b,  v_0x80.16b
	cmhs	v_mask1.16b, v9.16b,  v_0x80.16b
	cmhs	v_mask2.16b, v10.16b, v_0x80.16b
	cmhs	v_mask3.16b, v11.16b, v_0x80.16b
	and	v_mask0.16b, v_mask0.16b, v_gf8poly.16b
	and	v_mask1.16b, v_mask1.16b, v_gf8poly.16b
	and	v_mask2.16b, v_mask2.16b, v_gf8poly.16b
	and	v_mask3.16b, v_mask3.16b, v_gf8poly.16b
	shl	v8.16b,  v8.16b,  #1
	shl	v9.16b,  v9.16b,  #1
	shl	v10.16b, v10.16b, #1
	shl	v11.16b, v11.16b, #1
	eor	v8.16b,  v8.16b,  v_mask0.16b
	eor	v9.16b,  v9.16b,  v_mask1.16b
	eor	v10.16b, v10.16b, v_mask2.16b
	eor	v11.16b, v11.16b, v_mask3.16b
	eor	v8.16b,  v8.16b,  v16.16b
	eor	v9.16b,  v9.16b,  v17.16b
	eor	v10.16b, v10.16b, v18.16b
	eor	v11.16b, v11.16b, v19.16b

	cmhs	v_mask0.16b, v12.16b, v_0x80.16b
	cmhs	v_mask1.16b, v13.16b, v_0x80.16b
	cmhs	v_mask2.16b, v14.16b, v_0x80.16b
	cmhs	v_mask3.16b, v15.16b, v_0x80.16b
	and	v_mask0.16b, v_mask0.16b, v_gf8poly.16b
	and	v_mask1.16b, v_mask1.16b, v_gf8poly.16b
	and	v_mask2.16b, v_mask2.16b, v_gf8poly.16b
	and	v_mask3.16b, v_mask3.16b, v_gf8poly.16b
	shl	v12.16b, v12.16b, #1
	shl	v13.16b, v13.16b, #1
	shl	v14.16b, v14.16b, #1
	shl	v15.16b, v15.16b, #1
	eor	v12.16b, v12.16b, v_mask0.16b
	eor	v13.16b, v13.16b, v_mask1.16b
	eor	v14.16b, v14.16b, v_mask2.16b
	eor	v15.16b, v15.16b, v_mask3.16b
	eor	v12.16b, v12.16b, v20.16b
	eor	v13.16b, v13.16b, v21.16b
	eor	v14.16b, v14.16b, v22.16b
	eor	v15.16b, v15.16b, v23.16b

	bne	.Lloop128_vects

.Lloop128_vects_end:
	str	q0, [x_dst_p, #16*0]
	str	q1, [x_dst_p, #16*1]
	str	q2, [x_dst_p, #16*2]
	str	q3, [x_dst_p, #16*3]
	str	q4, [x_dst_p, #16*4]
	str	q5, [x_dst_p, #16*5]
	str	q6, [x_dst_p, #16*6]
	str	q7, [x_dst_p, #16*7]

	str	q8,  [x_dst_q, #16*0]
	str	q9,  [x_dst_q, #16*1]
	str	q10, [x_dst_q, #16*2]
	str	q11, [x_dst_q, #16*3]
	str	q12, [x_dst_q, #16*4]
	str	q13, [x_dst_q, #16*5]
	str	q14, [x_dst_q, #16*6]
	str	q15, [x_dst_q, #16*7]

	add	x_dst_p, x_dst_p, #128
	add	x_dst_q, x_dst_q, #128
	cmp	x_dst_q, x_dst_q_end
	add	w_col, w_col, #128
	bls	.Lloop128

.Lloop128_end:
	/* restore d8 ~ d15 */
	ldp	d8,  d9,  [sp]
	ldp	d10, d11, [sp, #16]
	ldp	d12, d13, [sp, #32]
	ldp	d14, d15, [sp, #48]
	add	sp, sp, #64

	add	x_dst_q_end, x_dst_q_end, #128

.Lloop16_init:
	tst	w_len, #0x7F
	beq	.Lloop16_end
	sub	x_dst_q_end, x_dst_q_end, #16

	/* batch process (vects-2)*16 bytes */
	/* v0: p;  v1: q;  v2: in;  v3: mask */
.Lloop16:
	ldr	q0, [x_src_last], #16
	mov	v1.16b, v0.16b

	cbz	w_vects, .Lloop16_vects_end

	sub	x_src_ptr, x_src, #8
.Lloop16_vects:
	ldr	x_srcn, [x_src_ptr], #-8
	ldr	q2, [x_srcn, x_col]
	cmp	x_src_ptr, x_src_ptr_end

	eor	v0.16b, v0.16b, v2.16b

	cmhs	v3.16b, v1.16b, v_0x80.16b
	and	v3.16b, v3.16b, v_gf8poly.16b

	shl	v1.16b, v1.16b, #1
	eor	v1.16b, v1.16b, v2.16b
	eor	v1.16b, v1.16b, v3.16b

	bne	.Lloop16_vects

.Lloop16_vects_end:
	str	q0, [x_dst_p], #16
	str	q1, [x_dst_q], #16
	cmp	x_dst_q, x_dst_q_end
	add	w_col, w_col, #16
	bls	.Lloop16

.Lloop16_end:
	mov	w_ret, #0
	ret
