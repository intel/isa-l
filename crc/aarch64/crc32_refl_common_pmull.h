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

.macro crc32_refl_func name:req
	.arch armv8-a+crc+crypto
	.text
	.align	3
	.global	\name
	.type	\name, %function

/* crc32_refl_func(uint32_t seed, uint8_t * buf, uint64_t len) */

// constant
.equ	FOLD_SIZE, 1024

// paramter
w_seed		.req	w0
x_seed		.req	x0
x_buf		.req	x1
x_len		.req	x2

x_buf_tmp	.req	x0

// crc32 refl function entry
\name\():
	mvn	w_seed, w_seed
	mov	x3, 0
	mov	w4, 0
	cmp	x_len, (FOLD_SIZE - 1)
	bhi	.crc32_clmul_pre

.crc32_refl_tab_pre:
	cmp	x_len, x3
	bls	.done
	sxtw	x4, w4
	adrp	x5, .LANCHOR0
	sub	x_buf, x_buf, x4
	add	x5, x5, :lo12:.LANCHOR0

	.align 3
.loop_crc32_refl_tab:
	ldrb	w3, [x_buf, x4]
	add	x4, x4, 1
	cmp	x_len, x4
	eor	x3, x_seed, x3
	and	x3, x3, 255
	ldr	w3, [x5, x3, lsl 2]
	eor	w_seed, w3, w_seed, lsr 8
	bhi	.loop_crc32_refl_tab
.done:
	mvn	w_seed, w_seed
	ret

d_y0_tmp	.req	d0
v_y0_tmp	.req	v0

q_x0_tmp	.req	q3
v_x0_tmp	.req	v3

v_x0		.req	v0
q_x1		.req	q2
q_x2		.req	q4
q_x3		.req	q1

d_p4_low	.req	d17
d_p4_high	.req	d19

x_buf_end	.req	x3
	.align 2
.crc32_clmul_pre:
	and	x4, x_len, -64
	uxtw	x_seed, w_seed
	cmp	x4, 63
	bls	.clmul_end

	fmov	d_y0_tmp, x_seed
	ins	v_y0_tmp.d[1], x3

	ldr	q_x0_tmp, [x_buf]
	ldr	q_x1, [x_buf, 16]
	ldr	q_x2, [x_buf, 32]
	ldr	q_x3, [x_buf, 48]
	eor	v_x0.16b, v_y0_tmp.16b, v_x0_tmp.16b

	sub	x5, x4, #64
	cmp	x5, 63

	add	x_buf_tmp, x_buf, 64
	bls	.clmul_loop_end

	mov	x4, p4_high_b0
	movk	x4, p4_high_b1, lsl 16
	fmov	d_p4_high, x4

	mov	x4, p4_low_b0
	movk	x4, p4_low_b1, lsl 16
	fmov	d_p4_low, x4

	add	x_buf_end, x_buf_tmp, x5

v_p4_low	.req	v17
v_p4_high	.req	v19

// v_x0		.req	v0
v_x1		.req	v2
v_x2		.req	v4
v_x3		.req	v1

q_y0		.req	q7
q_y1		.req	q5
q_y2		.req	q3
q_y3		.req	q21

v_y0		.req	v7
v_y1		.req	v5
v_y2		.req	v3
v_y3		.req	v21

d_x0_h		.req	d22
d_x1_h		.req	d20
d_x2_h		.req	d18
d_x3_h		.req	d6

v_x0_h		.req	v22
v_x1_h		.req	v20
v_x2_h		.req	v18
v_x3_h		.req	v6

	.align 3
.clmul_loop:
	add	x_buf_tmp, x_buf_tmp, 64
	cmp	x_buf_tmp, x_buf_end

	dup	d_x0_h, v_x0.d[1]
	dup	d_x1_h, v_x1.d[1]
	dup	d_x2_h, v_x2.d[1]
	dup	d_x3_h, v_x3.d[1]

	ldr	q_y0, [x_buf_tmp, -64]
	ldr	q_y1, [x_buf_tmp, -48]
	ldr	q_y2, [x_buf_tmp, -32]
	ldr	q_y3, [x_buf_tmp, -16]

	pmull	v_x0.1q, v_x0.1d, v_p4_low.1d
	pmull	v_x1.1q, v_x1.1d, v_p4_low.1d
	pmull	v_x2.1q, v_x2.1d, v_p4_low.1d
	pmull	v_x3.1q, v_x3.1d, v_p4_low.1d

	pmull	v_x0_h.1q, v_x0_h.1d, v_p4_high.1d
	pmull	v_x1_h.1q, v_x1_h.1d, v_p4_high.1d
	pmull	v_x2_h.1q, v_x2_h.1d, v_p4_high.1d
	pmull	v_x3_h.1q, v_x3_h.1d, v_p4_high.1d

	eor	v_y0.16b, v_y0.16b, v22.16b
	eor	v_y1.16b, v_y1.16b, v20.16b
	eor	v_y2.16b, v_y2.16b, v18.16b
	eor	v_y3.16b, v_y3.16b, v6.16b

	eor	v_x0.16b, v_y0.16b, v_x0.16b
	eor	v_x1.16b, v_y1.16b, v_x1.16b
	eor	v_x2.16b, v_y2.16b, v_x2.16b
	eor	v_x3.16b, v_y3.16b, v_x3.16b

	bne	.clmul_loop


// v_x0		.req	v0
// v_x1		.req	v2
// v_x2		.req	v4
// v_x3		.req	v1

d_x0		.req	d0

d_p1_high	.req	d7
d_p1_low	.req	d17

v_p1_high	.req	v7
v_p1_low	.req	v17

.clmul_loop_end:
// fold 128b
	mov	x0, p1_high_b0
	movk	x0, p1_high_b1, lsl 16
	fmov	d_p1_high, x0

	mov	x0, p1_low_b0
	movk	x0, p1_low_b1, lsl 16
	fmov	d_p1_low, x0

	dup	d6, v_x0.d[1]
	pmull	v_x0.1q, v_x0.1d, v_p1_low.1d
	pmull	v6.1q, v6.1d, v_p1_high.1d
	eor	v6.16b, v6.16b, v_x0.16b
	eor	v_x1.16b, v6.16b, v_x1.16b

	dup	d6, v_x1.d[1]
	pmull	v_x1.1q, v_x1.1d, v_p1_low.1d
	pmull	v6.1q, v6.1d, v_p1_high.1d
	eor	v6.16b, v6.16b, v_x1.16b
	eor	v_x2.16b, v6.16b, v_x2.16b

	dup	d_x0, v_x2.d[1] // d_x0 temparory saved v_x2 high
	pmull	v_x2.1q, v_x2.1d, v_p1_low.1d
	pmull	v_x0.1q, v_x0.1d, v_p1_high.1d
	eor	v_x0.16b, v_x0.16b, v_x2.16b
	eor	v_x0.16b, v_x0.16b, v_x3.16b

// all
	mov	x0, 4294967295
	fmov	d3, x0

	movi	v5.4s, 0

// fold 64b
	mov	x4, p0_low_b0
	movk	x4, p0_low_b1, lsl 16
	fmov	d1, x4

	dup	d2, v0.d[0]
	ext	v0.16b, v0.16b, v5.16b, #8
	pmull	v2.1q, v2.1d, v7.1d
	eor	v0.16b, v0.16b, v2.16b
	and	v2.16b, v3.16b, v0.16b
	ext	v0.16b, v0.16b, v5.16b, #4
	pmull	v2.1q, v2.1d, v1.1d

// barrett reduction
	mov	x3, br_high_b0
	movk	x3, br_high_b1, lsl 16
	movk	x3, br_high_b2, lsl 32

	fmov	d1, x3
	eor	v0.16b, v0.16b, v2.16b
	and	v2.16b, v0.16b, v3.16b
	pmull	v2.1q, v2.1d, v1.1d

	mov	x0, br_low_b0
	movk	x0, br_low_b1, lsl 16
	movk	x0, br_low_b2, lsl 32

	fmov	d1, x0
	and	v2.16b, v3.16b, v2.16b
	pmull	v2.1q, v2.1d, v1.1d
	eor	v0.16b, v0.16b, v2.16b
	umov	w_seed, v0.s[1]
	uxtw	x_seed, w_seed

.clmul_end:
	and	w4, w2, -64
	sxtw	x3, w4
	add	x_buf, x_buf, x3
	b	.crc32_refl_tab_pre
	.size	\name, .-\name
.endm
