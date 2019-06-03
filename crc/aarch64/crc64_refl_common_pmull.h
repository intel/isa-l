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

.macro crc64_refl_func name:req
	.arch armv8-a+crc+crypto
	.text
	.align	3
	.global	\name
	.type	\name, %function

// parameter
x_seed		.req	x0
x_buf		.req	x1
x_len		.req	x2

// return
x_crc_ret	.req	x0

// constant
.equ	FOLD_SIZE, 1024

// global variable
x_buf_end		.req	x3
x_counter		.req	x4
x_buf_iter		.req	x5
x_crc64_tab_addr	.req	x6
w_tmp			.req	w7
x_tmp			.req	x7

// crc64 refl function entry
\name\():
// crc64 for table
	mvn	x_seed, x_seed
	mov	x_counter, 0
	cmp	x_len, (FOLD_SIZE-1)
	bhi	.crc64_clmul_pre
.crc64_tab_pre:
	cmp	x_len, x_counter
	bls	.done

	adrp	x_tmp, .lanchor_crc64_tab
	add	x_buf_iter, x_buf, x_counter
	add	x_buf, x_buf, x_len
	add	x_crc64_tab_addr, x_tmp, :lo12:.lanchor_crc64_tab

	.align 3
.loop_crc64_tab:
	ldrb	w_tmp, [x_buf_iter], 1
	eor	w_tmp, w_tmp, w0
	cmp	x_buf, x_buf_iter
	and	x_tmp, x_tmp, 255
	ldr	x_tmp, [x_crc64_tab_addr, x_tmp, lsl 3]
	eor	x_seed, x_tmp, x_seed, lsr 8
	bne	.loop_crc64_tab
.done:
	mvn	x_crc_ret, x_crc_ret
	ret

// clmul prepare
q_x0		.req	q0
q_x1		.req	q4
q_x2		.req	q6
q_x3		.req	q1

v_x0		.req	v0
v_x1		.req	v4
v_x2		.req	v6
v_x3		.req	v1

d_p4_high	.req	d17
d_p4_low	.req	d7
v_p4_high	.req	v17
v_p4_low	.req	v7

d_y0_tmp	.req	d0
v_y0_tmp	.req	v0

q_tmp		.req	q2
v_tmp		.req	v2

	.align 2
.crc64_clmul_pre:
	ldr	q_tmp, [x_buf]
	ldr	q_x1, [x_buf, 16]
	ldr	q_x2, [x_buf, 32]
	ldr	q_x3, [x_buf, 48]

	and	x_counter, x_len, -64
	sub	x_tmp, x_counter, #64
	cmp	x_tmp, 63

	fmov	d_y0_tmp, x_seed // save crc to d0
	eor	v_x0.16b, v_y0_tmp.16b, v_tmp.16b

	add	x_buf_iter, x_buf, 64
	bls	.clmul_loop_end

	add	x_buf_end, x_buf_iter, x_tmp

	mov	x_tmp, p4_high_b0
	movk	x_tmp, p4_high_b1, lsl 16
	movk	x_tmp, p4_high_b2, lsl 32
	movk	x_tmp, p4_high_b3, lsl 48
	fmov	d_p4_high, x_tmp

	mov	x_tmp, p4_low_b0
	movk	x_tmp, p4_low_b1, lsl 16
	movk	x_tmp, p4_low_b2, lsl 32
	movk	x_tmp, p4_low_b3, lsl 48
	fmov	d_p4_low, x_tmp

// 1024bit --> 512bit loop
// merge x0, x1, x2, x3, y0, y1, y2, y3 => x0, x1, x2, x3 (uint64x2_t)
d_x0_high	.req	d24
d_x1_high	.req	d22
d_x2_high	.req	d20
d_x3_high	.req	d16

v_x0_high	.req	v24
v_x1_high	.req	v22
v_x2_high	.req	v20
v_x3_high	.req	v16

q_x0_tmp	.req	q2
q_x1_tmp	.req	q5
q_x2_tmp	.req	q3
q_x3_tmp	.req	q18

v_x0_tmp	.req	v2
v_x1_tmp	.req	v5
v_x2_tmp	.req	v3
v_x3_tmp	.req	v18

q_x0_tmp	.req	q2
q_x1_tmp	.req	q5
q_x2_tmp	.req	q3
q_x3_tmp	.req	q18

	.align 3
.clmul_loop:
	add	x_buf_iter, x_buf_iter, 64
	cmp	x_buf_iter, x_buf_end

	dup	d_x0_high, v_x0.d[1]
	dup	d_x1_high, v_x1.d[1]
	dup	d_x2_high, v_x2.d[1]
	dup	d_x3_high, v_x3.d[1]

	pmull	v_x0_high.1q, v_x0_high.1d, v_p4_high.1d
	pmull	v_x1_high.1q, v_x1_high.1d, v_p4_high.1d
	pmull	v_x2_high.1q, v_x2_high.1d, v_p4_high.1d
	pmull	v_x3_high.1q, v_x3_high.1d, v_p4_high.1d

	pmull	v_x0.1q, v_x0.1d, v_p4_low.1d
	pmull	v_x1.1q, v_x1.1d, v_p4_low.1d
	pmull	v_x2.1q, v_x2.1d, v_p4_low.1d
	pmull	v_x3.1q, v_x3.1d, v_p4_low.1d

	ldr	q_x0_tmp, [x_buf_iter, -64]
	ldr	q_x1_tmp, [x_buf_iter, -48]
	ldr	q_x2_tmp, [x_buf_iter, -32]
	ldr	q_x3_tmp, [x_buf_iter, -16]

	eor	v_x0_tmp.16b, v_x0_tmp.16b, v_x0_high.16b
	eor	v_x1_tmp.16b, v_x1_tmp.16b, v_x1_high.16b
	eor	v_x2_tmp.16b, v_x2_tmp.16b, v_x2_high.16b
	eor	v_x3_tmp.16b, v_x3_tmp.16b, v_x3_high.16b

	eor	v_x0.16b, v_x0_tmp.16b, v_x0.16b
	eor	v_x1.16b, v_x1_tmp.16b, v_x1.16b
	eor	v_x2.16b, v_x2_tmp.16b, v_x2.16b
	eor	v_x3.16b, v_x3_tmp.16b, v_x3.16b
	bne	.clmul_loop

// folding 512bit --> 128bit
// merge x0, x1, x2, x3 => x3 (uint64x2_t)
// input: x0 -> v_x0, x1 -> v_x1, x2 -> v_x2, x3 -> v_x3
// output: v_x3
d_p1_high	.req	d5
d_p1_low	.req	d3
v_p1_high	.req	v5
v_p1_low	.req	v3

d_tmp_high	.req	d16
d_tmp_low	.req	d2
v_tmp_high	.req	v16
v_tmp_low	.req	v2

.clmul_loop_end:
	mov	x_tmp, p1_high_b0
	movk	x_tmp, p1_high_b1, lsl 16
	movk	x_tmp, p1_high_b2, lsl 32
	movk	x_tmp, p1_high_b3, lsl 48
	fmov	d_p1_high, x_tmp

	mov	x_tmp, p1_low_b0
	movk	x_tmp, p1_low_b1, lsl 16
	movk	x_tmp, p1_low_b2, lsl 32
	movk	x_tmp, p1_low_b3, lsl 48
	fmov	d_p1_low, x_tmp

	dup	d_tmp_high, v_x0.d[1]
	dup	d_tmp_low, v_x0.d[0]

	pmull	v_tmp_high.1q, v_tmp_high.1d, v_p1_high.1d
	pmull	v_tmp_low.1q, v_tmp_low.1d, v_p1_low.1d
	eor	v_tmp_high.16b, v_tmp_high.16b, v_tmp_low.16b
	eor	v_x1.16b, v_tmp_high.16b, v_x1.16b

	dup	d_tmp_high, v_x1.d[1]
	pmull	v_x1.1q, v_x1.1d, v_p1_low.1d
	pmull	v_tmp_high.1q, v_tmp_high.1d, v_p1_high.1d
	eor	v_tmp_high.16b, v_tmp_high.16b, v_x1.16b
	eor	v_x2.16b, v_tmp_high.16b, v_x2.16b

	dup	d_tmp_high, v_x2.d[1]
	pmull	v_x2.1q, v_x2.1d, v_p1_low.1d
	pmull	v_tmp_high.1q, v_tmp_high.1d, v_p1_high.1d
	eor	v_tmp_high.16b, v_tmp_high.16b, v_x2.16b
	eor	v_x3.16b, v_tmp_high.16b, v_x3.16b

// fold 64b
// input: v_x3
// output: v_x3
d_p0_low		.req	d3
v_p0_low		.req	v3
d_x3_low_fold_64b	.req	d2
v_x3_low_fold_64b	.req	v2
v_zero_fold_64b		.req	v0
	mov	x_tmp, p0_low_b0
	movk	x_tmp, p0_low_b1, lsl 16
	movk	x_tmp, p0_low_b2, lsl 32
	movk	x_tmp, p0_low_b3, lsl 48
	fmov	d_p0_low, x_tmp

	dup	d_x3_low_fold_64b, v_x3.d[0]
	movi	v_zero_fold_64b.4s, 0
	ext	v_x3.16b, v_x3.16b, v0.16b, #8

	pmull	v_x3_low_fold_64b.1q, v_x3_low_fold_64b.1d, v_p0_low.1d
	eor	v_x3.16b, v_x3.16b, v_x3_low_fold_64b.16b

// barrett reduction
// input: v_x3
// output: x0
d_br_low	.req	d3
d_br_high	.req	d5
v_br_low	.req	v3
v_br_high	.req	v5
	mov	x0, br_low_b0
	movk	x0, br_low_b1, lsl 16
	movk	x0, br_low_b2, lsl 32
	movk	x0, br_low_b3, lsl 48
	fmov	d_br_low, x0

	mov	x0, br_high_b0
	movk	x0, br_high_b1, lsl 16
	movk	x0, br_high_b2, lsl 32
	movk	x0, br_high_b3, lsl 48
	fmov	d_br_high, x0

	dup	d2, v_x3.d[0]

	pmull	v2.1q, v2.1d, v_br_low.1d
	pmull	v4.1q, v2.1d, v_br_high.1d

	ext	v0.16b, v0.16b, v2.16b, #8

	eor	v0.16b, v0.16b, v4.16b
	eor	v0.16b, v0.16b, v_x3.16b
	umov	x0, v0.d[1]

	b	.crc64_tab_pre

	.size	\name, .-\name
.endm
