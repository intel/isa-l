########################################################################
#  Copyright (c) 2019 Microsoft Corporation.
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
#    * Neither the name of Microsoft Corporation nor the names of its
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

// parameters
#define w_seed          w0
#define x_seed          x0
#define x_buf           x1
#define w_len           w2
#define x_len           x2

// return
#define w_crc_ret       w0
#define x_crc_ret       x0

// constant
#define FOLD_SIZE       64

// global variables
#define x_buf_end       x3
#define w_counter       w4
#define x_counter       x4
#define x_buf_iter      x5
#define x_crc_tab_addr  x6
#define x_tmp2          x6
#define w_tmp           w7
#define x_tmp           x7

#define v_x0            v0
#define d_x0            d0
#define s_x0            s0

#define q_x1            q1
#define v_x1            v1

#define q_x2            q2
#define v_x2            v2

#define q_x3            q3
#define v_x3            v3
#define d_x3            d3
#define s_x3            s3

#define q_y0            q4
#define v_y0            v4
#define v_tmp_high      v4
#define d_tmp_high      d4

#define q_y1            q5
#define v_y1            v5
#define v_tmp_low       v5

#define q_y2            q6
#define v_y2            v6

#define q_y3            q7
#define v_y3            v7

#define q_x0_tmp        q30
#define v_x0_tmp        v30
#define d_p4_high       v30.d[1]
#define d_p4_low        d30
#define v_p4            v30
#define d_p1_high       v30.d[1]
#define d_p1_low        d30
#define v_p1            v30
#define d_p0_high       v30.d[1]
#define d_p0_low        d30
#define v_p0            v30
#define d_br_low        d30
#define d_br_low2       v30.d[1]
#define v_br_low        v30

#define q_shuffle       q31
#define v_shuffle       v31
#define d_br_high       d31
#define d_br_high2      v31.d[1]
#define v_br_high       v31
#define d_p0_low2       d31
#define d_p0_high2      v31.d[1]
#define v_p02           v31

#define v_x0_high       v16
#define v_x1_high       v17
#define v_x2_high       v18
#define v_x3_high       v19

.macro crc_refl_load_first_block
	ldr	q_x0_tmp, [x_buf]
	ldr	q_x1, [x_buf, 16]
	ldr	q_x2, [x_buf, 32]
	ldr	q_x3, [x_buf, 48]

	and	x_counter, x_len, -64
	sub	x_tmp, x_counter, #64
	cmp	x_tmp, 63

	add	x_buf_iter, x_buf, 64

	eor	v_x0.16b, v_x0.16b, v_x0_tmp.16b
.endm

.macro crc_norm_load_first_block
	adrp	x_tmp, .shuffle_data
	ldr	q_shuffle, [x_tmp, #:lo12:.shuffle_data]

	ldr	q_x0_tmp, [x_buf]
	ldr	q_x1, [x_buf, 16]
	ldr	q_x2, [x_buf, 32]
	ldr	q_x3, [x_buf, 48]

	and	x_counter, x_len, -64
	sub	x_tmp, x_counter, #64
	cmp	x_tmp, 63

	add	x_buf_iter, x_buf, 64

	tbl	v_x0_tmp.16b, {v_x0_tmp.16b}, v_shuffle.16b
	tbl	v_x1.16b, {v_x1.16b}, v_shuffle.16b
	tbl	v_x2.16b, {v_x2.16b}, v_shuffle.16b
	tbl	v_x3.16b, {v_x3.16b}, v_shuffle.16b

	eor	v_x0.16b, v_x0.16b, v_x0_tmp.16b
.endm

.macro crc32_load_p4
	add	x_buf_end, x_buf_iter, x_tmp

	mov	x_tmp, p4_low_b0
	movk	x_tmp, p4_low_b1, lsl 16
	fmov	d_p4_low, x_tmp

	mov	x_tmp2, p4_high_b0
	movk	x_tmp2, p4_high_b1, lsl 16
	fmov	d_p4_high, x_tmp2
.endm

.macro crc64_load_p4
	add	x_buf_end, x_buf_iter, x_tmp

	mov	x_tmp, p4_low_b0
	movk	x_tmp, p4_low_b1, lsl 16
	movk	x_tmp, p4_low_b2, lsl 32
	movk	x_tmp, p4_low_b3, lsl 48
	fmov	d_p4_low, x_tmp

	mov	x_tmp2, p4_high_b0
	movk	x_tmp2, p4_high_b1, lsl 16
	movk	x_tmp2, p4_high_b2, lsl 32
	movk	x_tmp2, p4_high_b3, lsl 48
	fmov	d_p4_high, x_tmp2
.endm

.macro crc_refl_loop
	.align 3
.clmul_loop:
	// interleave ldr and pmull(2) for arch which can only issue quadword load every
	// other cycle (i.e. A55)
	ldr	q_y0, [x_buf_iter]
	pmull2	v_x0_high.1q, v_x0.2d, v_p4.2d
	ldr	q_y1, [x_buf_iter, 16]
	pmull2	v_x1_high.1q, v_x1.2d, v_p4.2d
	ldr	q_y2, [x_buf_iter, 32]
	pmull2	v_x2_high.1q, v_x2.2d, v_p4.2d
	ldr	q_y3, [x_buf_iter, 48]
	pmull2	v_x3_high.1q, v_x3.2d, v_p4.2d

	pmull	v_x0.1q, v_x0.1d, v_p4.1d
	add	x_buf_iter, x_buf_iter, 64
	pmull	v_x1.1q, v_x1.1d, v_p4.1d
	cmp	x_buf_iter, x_buf_end
	pmull	v_x2.1q, v_x2.1d, v_p4.1d
	pmull	v_x3.1q, v_x3.1d, v_p4.1d

	eor	v_x0.16b, v_x0.16b, v_x0_high.16b
	eor	v_x1.16b, v_x1.16b, v_x1_high.16b
	eor	v_x2.16b, v_x2.16b, v_x2_high.16b
	eor	v_x3.16b, v_x3.16b, v_x3_high.16b

	eor	v_x0.16b, v_x0.16b, v_y0.16b
	eor	v_x1.16b, v_x1.16b, v_y1.16b
	eor	v_x2.16b, v_x2.16b, v_y2.16b
	eor	v_x3.16b, v_x3.16b, v_y3.16b
	bne	.clmul_loop
.endm

.macro crc_norm_loop
	.align 3
.clmul_loop:
	// interleave ldr and pmull(2) for arch which can only issue quadword load every
	// other cycle (i.e. A55)
	ldr	q_y0, [x_buf_iter]
	pmull2	v_x0_high.1q, v_x0.2d, v_p4.2d
	ldr	q_y1, [x_buf_iter, 16]
	pmull2	v_x1_high.1q, v_x1.2d, v_p4.2d
	ldr	q_y2, [x_buf_iter, 32]
	pmull2	v_x2_high.1q, v_x2.2d, v_p4.2d
	ldr	q_y3, [x_buf_iter, 48]
	pmull2	v_x3_high.1q, v_x3.2d, v_p4.2d

	pmull	v_x0.1q, v_x0.1d, v_p4.1d
	add	x_buf_iter, x_buf_iter, 64
	pmull	v_x1.1q, v_x1.1d, v_p4.1d
	cmp	x_buf_iter, x_buf_end
	pmull	v_x2.1q, v_x2.1d, v_p4.1d
	pmull	v_x3.1q, v_x3.1d, v_p4.1d

	tbl	v_y0.16b, {v_y0.16b}, v_shuffle.16b
	tbl	v_y1.16b, {v_y1.16b}, v_shuffle.16b
	tbl	v_y2.16b, {v_y2.16b}, v_shuffle.16b
	tbl	v_y3.16b, {v_y3.16b}, v_shuffle.16b

	eor	v_x0.16b, v_x0.16b, v_x0_high.16b
	eor	v_x1.16b, v_x1.16b, v_x1_high.16b
	eor	v_x2.16b, v_x2.16b, v_x2_high.16b
	eor	v_x3.16b, v_x3.16b, v_x3_high.16b

	eor	v_x0.16b, v_x0.16b, v_y0.16b
	eor	v_x1.16b, v_x1.16b, v_y1.16b
	eor	v_x2.16b, v_x2.16b, v_y2.16b
	eor	v_x3.16b, v_x3.16b, v_y3.16b
	bne	.clmul_loop
.endm

.macro crc32_fold_512b_to_128b
	mov	x_tmp, p1_low_b0
	movk	x_tmp, p1_low_b1, lsl 16
	fmov	d_p1_low, x_tmp

	mov	x_tmp2, p1_high_b0
	movk	x_tmp2, p1_high_b1, lsl 16
	fmov	d_p1_high, x_tmp2

	pmull2	v_tmp_high.1q, v_x0.2d, v_p1.2d
	pmull	v_tmp_low.1q, v_x0.1d, v_p1.1d
	eor	v_x1.16b, v_x1.16b, v_tmp_high.16b
	eor	v_x1.16b, v_x1.16b, v_tmp_low.16b

	pmull2	v_tmp_high.1q, v_x1.2d, v_p1.2d
	pmull	v_tmp_low.1q, v_x1.1d, v_p1.1d
	eor	v_x2.16b, v_x2.16b, v_tmp_high.16b
	eor	v_x2.16b, v_x2.16b, v_tmp_low.16b

	pmull2	v_tmp_high.1q, v_x2.2d, v_p1.2d
	pmull	v_tmp_low.1q, v_x2.1d, v_p1.1d
	eor	v_x3.16b, v_x3.16b, v_tmp_high.16b
	eor	v_x3.16b, v_x3.16b, v_tmp_low.16b
.endm

.macro crc64_fold_512b_to_128b
	mov	x_tmp, p1_low_b0
	movk	x_tmp, p1_low_b1, lsl 16
	movk	x_tmp, p1_low_b2, lsl 32
	movk	x_tmp, p1_low_b3, lsl 48
	fmov	d_p1_low, x_tmp

	mov	x_tmp2, p1_high_b0
	movk	x_tmp2, p1_high_b1, lsl 16
	movk	x_tmp2, p1_high_b2, lsl 32
	movk	x_tmp2, p1_high_b3, lsl 48
	fmov	d_p1_high, x_tmp2

	pmull2	v_tmp_high.1q, v_x0.2d, v_p1.2d
	pmull	v_tmp_low.1q, v_x0.1d, v_p1.1d
	eor	v_x1.16b, v_x1.16b, v_tmp_high.16b
	eor	v_x1.16b, v_x1.16b, v_tmp_low.16b

	pmull2	v_tmp_high.1q, v_x1.2d, v_p1.2d
	pmull	v_tmp_low.1q, v_x1.1d, v_p1.1d
	eor	v_x2.16b, v_x2.16b, v_tmp_high.16b
	eor	v_x2.16b, v_x2.16b, v_tmp_low.16b

	pmull2	v_tmp_high.1q, v_x2.2d, v_p1.2d
	pmull	v_tmp_low.1q, v_x2.1d, v_p1.1d
	eor	v_x3.16b, v_x3.16b, v_tmp_high.16b
	eor	v_x3.16b, v_x3.16b, v_tmp_low.16b
.endm