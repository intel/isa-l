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

.macro crc32_norm_func name:req
	.arch armv8-a+crc+crypto
	.text
	.align	3
	.global	\name
	.type	\name, %function

/* crc32_norm_func(uint32_t seed, uint8_t * buf, uint64_t len) */

// constant
.equ	FOLD_SIZE, 1024

// parameter
w_seed		.req	w0
x_seed		.req	x0
x_buf		.req	x1
x_len		.req	x2

x_buf_tmp	.req	x0

// crc32 normal function entry
\name\():
	mvn	w_seed, w_seed
	mov	x3, 0
	mov	w4, 0
	cmp	x_len, (FOLD_SIZE - 1)
	uxtw	x_seed, w_seed
	bhi	.crc32_clmul_pre

.crc32_norm_tab_pre:
	cmp	x_len, x3
	bls	.done

	sxtw	x4, w4
	adrp	x5, .LANCHOR0
	sub	x_buf, x_buf, x4
	add	x5, x5, :lo12:.LANCHOR0

	.align 3
.loop_crc32_norm_tab:
	ldrb	w3, [x_buf, x4]
	add	x4, x4, 1
	cmp	x_len, x4
	eor	x3, x3, x0, lsr 24
	and	x3, x3, 255
	ldr	w3, [x5, x3, lsl 2]
	eor	x0, x3, x0, lsl 8
	bhi	.loop_crc32_norm_tab

.done:
	mvn	w_seed, w_seed
	ret

// crcc32 clmul prepare

x_buf_end	.req	x3
q_shuffle	.req	q3
v_shuffle	.req	v3

q_x0_tmp	.req	q5
q_x1		.req	q6
q_x2		.req	q4
q_x3		.req	q1

v_x0_tmp	.req	v5
v_x0		.req	v2
v_x1		.req	v6
v_x2		.req	v4
v_x3		.req	v1

d_p4_high	.req	d7
d_p4_low	.req	d5

	.align 2
.crc32_clmul_pre:
	and	x3, x_len, -64
	cmp	x3, 63
	bls	.clmul_end

	lsl	x_seed, x_seed, 32
	movi	v2.4s, 0
	ins	v2.d[1], x_seed

	adrp	x4, .shuffle
	ldr	q_shuffle, [x4, #:lo12:.shuffle]

	sub	x4, x3, #64
	cmp	x4, 63

	ldr	q_x0_tmp, [x_buf]
	ldr	q_x1, [x_buf, 16]
	ldr	q_x2, [x_buf, 32]
	ldr	q_x3, [x_buf, 48]
	add	x_buf_tmp, x_buf, 64

	tbl	v_x0_tmp.16b, {v_x0_tmp.16b}, v_shuffle.16b
	tbl	v_x1.16b, {v_x1.16b}, v_shuffle.16b
	tbl	v_x2.16b, {v_x2.16b}, v_shuffle.16b
	tbl	v_x3.16b, {v_x3.16b}, v_shuffle.16b
	eor	v_x0.16b, v_x0.16b, v_x0_tmp.16b
	bls	.clmul_loop_end

	add	x_buf_end, x_buf_tmp, x4
	mov	x4, p4_high_b0
	movk	x4, p4_high_b1, lsl 16
	fmov	d_p4_high, x4

	mov	x4, p4_low_b0
	movk	x4, p4_low_b1, lsl 16
	fmov	d_p4_low, x4

// crc32 clmul loop
//v_x0		.req	v2
//v_x1		.req	v6
//v_x2		.req	v4
//v_x3		.req	v1

d_x0_high	.req	d22
d_x1_high	.req	d20
d_x2_high	.req	d18
d_x3_high	.req	d16

v_x0_high	.req	v22
v_x1_high	.req	v20
v_x2_high	.req	v18
v_x3_high	.req	v16

q_y0_high	.req	q23
q_y1_high	.req	q21
q_y2_high	.req	q19
q_y3_high	.req	q17

v_y0_high	.req	v23
v_y1_high	.req	v21
v_y2_high	.req	v19
v_y3_high	.req	v17

v_p4_high	.req	v7
v_p4_low	.req	v5
//v_shuffle	.req	v3

	.align 3
.clmul_loop:
	dup	d_x0_high, v_x0.d[1]
	dup	d_x1_high, v_x1.d[1]
	dup	d_x2_high, v_x2.d[1]
	dup	d_x3_high, v_x3.d[1]

	add	x_buf_tmp, x_buf_tmp, 64

	ldr	q_y0_high, [x_buf_tmp, -64]
	ldr	q_y1_high, [x_buf_tmp, -48]
	ldr	q_y2_high, [x_buf_tmp, -32]
	ldr	q_y3_high, [x_buf_tmp, -16]

	cmp	x_buf_tmp, x_buf_end

	pmull	v_x0.1q, v_x0.1d, v_p4_low.1d
	pmull	v_x1.1q, v_x1.1d, v_p4_low.1d
	pmull	v_x2.1q, v_x2.1d, v_p4_low.1d
	pmull	v_x3.1q, v_x3.1d, v_p4_low.1d

	pmull	v_x0_high.1q, v_x0_high.1d, v_p4_high.1d
	pmull	v_x1_high.1q, v_x1_high.1d, v_p4_high.1d
	pmull	v_x2_high.1q, v_x2_high.1d, v_p4_high.1d
	pmull	v_x3_high.1q, v_x3_high.1d, v_p4_high.1d

	eor	v_x0.16b, v_x0_high.16b, v_x0.16b
	eor	v_x1.16b, v_x1_high.16b, v_x1.16b
	eor	v_x2.16b, v_x2_high.16b, v_x2.16b
	eor	v_x3.16b, v_x3_high.16b, v_x3.16b

	tbl	v_y0_high.16b, {v_y0_high.16b}, v_shuffle.16b
	tbl	v_y1_high.16b, {v_y1_high.16b}, v_shuffle.16b
	tbl	v_y2_high.16b, {v_y2_high.16b}, v_shuffle.16b
	tbl	v_y3_high.16b, {v_y3_high.16b}, v_shuffle.16b

	eor	v_x0.16b, v_x0.16b, v_y0_high.16b
	eor	v_x1.16b, v_x1.16b, v_y1_high.16b
	eor	v_x2.16b, v_x2.16b, v_y2_high.16b
	eor	v_x3.16b, v_x3.16b, v_y3_high.16b
	bne	.clmul_loop

//v_x0		.req	v2
//v_x1		.req	v6
//v_x2		.req	v4
//v_x3		.req	v1

d_p1_high	.req	d7
d_p1_low	.req	d5

v_p1_high	.req	v7
v_p1_low	.req	v5
.clmul_loop_end:
// folding 512bit --> 128bit
	mov	x0, p1_high_b0
	movk	x0, p1_high_b1, lsl 16
	fmov	d_p1_high, x0

	mov	x0, p1_low_b0
	movk	x0, p1_low_b1, lsl 16
	fmov	d_p1_low, x0

	dup	d16, v_x0.d[1]
	pmull	v_x0.1q, v_x0.1d, v_p1_low.1d
	pmull	v16.1q, v16.1d, v_p1_high.1d
	eor	v_x0.16b, v16.16b, v_x0.16b
	eor	v_x1.16b, v_x0.16b, v_x1.16b

	dup	d17, v_x1.d[1]
	pmull	v_x1.1q, v_x1.1d, v_p1_low.1d
	pmull	v17.1q, v17.1d, v_p1_high.1d
	eor	v_x1.16b, v17.16b, v_x1.16b
	eor	v_x2.16b, v_x1.16b, v_x2.16b

	dup	d0, v_x2.d[1]
	pmull	v_x2.1q, v_x2.1d, v_p1_low.1d
	pmull	v0.1q, v0.1d, v_p1_high.1d
	eor	v_x2.16b, v0.16b, v_x2.16b
	eor	v_x3.16b, v_x2.16b, v_x3.16b

//v_x0		.req	v2
//v_x3		.req	v1

d_x0		.req	d2
v_zero		.req	v3
// fold 64b
	movi	v_zero.4s, 0

	mov	x5, p0_high_b0
	movk	x5, p0_high_b1, lsl 16

	mov	x0, p0_low_b0
	movk	x0, p0_low_b1, lsl 16

	dup	d_x0, v_x3.d[1]
	ext	v0.16b, v_zero.16b, v_x3.16b, #8

	fmov	d16, x5
	pmull	v_x0.1q, v_x0.1d, v16.1d

	fmov	d17, x0
	ext	v0.16b, v0.16b, v_zero.16b, #4
	eor	v0.16b, v0.16b, v_x0.16b
	dup	d_x0, v0.d[1]
	pmull	v_x0.1q, v_x0.1d, v17.1d

// barrett reduction
d_br_low	.req	d16
d_br_high	.req	d17

v_br_low	.req	v16
v_br_high	.req	v17
	mov	x4, br_low_b0
	movk	x4, br_low_b1, lsl 16
	movk	x4, br_low_b2, lsl 32

	mov	x3, br_high_b0
	movk	x3, br_high_b1, lsl 16
	movk	x3, br_high_b2, lsl 32

	fmov	d_br_low, x4
	eor	v0.16b, v0.16b, v2.16b
	umov	x0, v0.d[0]
	fmov	d2, x0
	ext	v2.16b, v2.16b, v3.16b, #4
	pmull	v2.1q, v2.1d, v_br_low.1d

	fmov	d_br_high, x3
	ext	v2.16b, v2.16b, v3.16b, #4
	pmull	v2.1q, v2.1d, v_br_high.1d
	eor	v0.16b, v0.16b, v2.16b
	umov	x_seed, v0.d[0]

.clmul_end:
	and	w4, w2, -64
	sxtw	x3, w4
	add	x1, x1, x3
	b	.crc32_norm_tab_pre
	.size	\name, .-\name
	.section	.rodata.cst16,"aM",@progbits,16

	.align	4
.shuffle:
	.byte	15, 14, 13, 12, 11, 10, 9
	.byte	8, 7, 6, 5, 4, 3, 2, 1, 0
.endm
