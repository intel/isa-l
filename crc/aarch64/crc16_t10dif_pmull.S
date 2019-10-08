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

	.arch armv8-a+crc+crypto
	.text
	.align	3
	.global	crc16_t10dif_pmull
	.type	crc16_t10dif_pmull, %function

/* uint16_t crc16_t10dif_pmull(uint16_t seed, uint8_t *buf, uint64_t len) */

/* arguments */
w_seed			.req	w0
x_buf			.req	x1
x_len			.req	x2
w_len			.req	w2

/* returns */
w_ret			.req	w0

/* these as global temporary registers */
w_tmp			.req	w5
x_tmp			.req	x5
x_tmp1			.req	x6
x_tmp2			.req	x7

d_tmp1			.req	d0
d_tmp2			.req	d1
q_tmp1			.req	q0
q_tmp2			.req	q1
v_tmp1			.req	v0
v_tmp2			.req	v1

/* local variables */
w_counter		.req	w3
w_crc			.req	w0
x_crc			.req	x0
x_counter		.req	x3
x_crc16tab		.req	x4
x_buf_saved		.req	x0

crc16_t10dif_pmull:
	cmp	x_len, 1023
	sub	sp, sp, #16
	uxth	w_seed, w_seed
	bhi	.crc_fold

	mov	x_tmp, 0
	mov	w_counter, 0

.crc_table_loop_pre:
	cmp	x_len, x_tmp
	bls	.end

	sxtw	x_counter, w_counter
	adrp	x_crc16tab, .LANCHOR0
	sub	x_buf, x_buf, x_counter
	add	x_crc16tab, x_crc16tab, :lo12:.LANCHOR0

	.align 2
.crc_table_loop:
	ldrb	w_tmp, [x_buf, x_counter]
	add	x_counter, x_counter, 1
	cmp	x_len, x_counter
	eor	w_tmp, w_tmp, w_crc, lsr 8
	ldrh	w_tmp, [x_crc16tab, w_tmp, sxtw 1]
	eor	w_crc, w_tmp, w_crc, lsl 8
	uxth	w_crc, w_crc
	bhi	.crc_table_loop

.end:
	add	sp, sp, 16
	ret

/* carry less multiplication, part1 - before loop */
q_x0			.req	q2
q_x1			.req	q3
q_x2			.req	q4
q_x3			.req	q5

v_x0			.req	v2
v_x1			.req	v3
v_x2			.req	v4
v_x3			.req	v5

d_x0			.req	d2
d_x1			.req	d3
d_x2			.req	d4
d_x3			.req	d5

// the following registers only used this part1
d_tmp3			.req	d16
v_tmp3			.req	v16

	.align 3
.crc_fold:
	fmov	d_tmp1, x_crc
	fmov	d_tmp2, xzr
	dup	d_tmp3, v_tmp2.d[0]
	shl	d_tmp1, d_tmp1, 48
	ins	v_tmp3.d[1], v_tmp1.d[0]

	and	x_counter, x_len, -64
	sub	x_counter, x_counter, #64
	cmp	x_counter, 63
	add	x_buf_saved, x_buf, 64

	ldr	q_x0, [x_buf]
	ldr	q_x1, [x_buf, 16]
	ldr	q_x2, [x_buf, 32]
	ldr	q_x3, [x_buf, 48]

	adrp	x_tmp, .shuffle_mask_lanchor
	ldr	q7, [x_tmp, :lo12:.shuffle_mask_lanchor]

	tbl	v_tmp1.16b, {v_x0.16b}, v7.16b
	eor	v_x0.16b, v_tmp3.16b, v_tmp1.16b

	tbl	v_x1.16b, {v_x1.16b}, v7.16b
	tbl	v_x2.16b, {v_x2.16b}, v7.16b
	tbl	v_x3.16b, {v_x3.16b}, v7.16b
	bls	.crc_fold_loop_end

/* carry less multiplication, part2 - loop */
q_y0			.req	q28
q_y1			.req	q29
q_y2			.req	q30
q_y3			.req	q31

v_y0			.req	v28
v_y1			.req	v29
v_y2			.req	v30
v_y3			.req	v31

d_x0_h			.req	d24
d_x0_l			.req	d2
d_x1_h			.req	d25
d_x1_l			.req	d3
d_x2_h			.req	d26
d_x2_l			.req	d4
d_x3_h			.req	d27
d_x3_l			.req	d5

v_x0_h			.req	v24
v_x0_l			.req	v2
v_x1_h			.req	v25
v_x1_l			.req	v3
v_x2_h			.req	v26
v_x2_l			.req	v4
v_x3_h			.req	v27
v_x3_l			.req	v5

v_tmp1_x0		.req	v24
v_tmp1_x1		.req	v25
v_tmp1_x2		.req	v26
v_tmp1_x3		.req	v27

d_p4_h			.req	d19
v_p4_h			.req	v19
d_p4_l			.req	d17
v_p4_l			.req	v17

	mov	x_tmp, 0x371d0000		/* p4 [1] */
	fmov	d_p4_h, x_tmp
	mov	x_tmp, 0x87e70000		/* p4 [0] */
	fmov	d_p4_l, x_tmp

	.align 2
.crc_fold_loop:
	add	x_buf_saved, x_buf_saved, 64
	sub	x_counter, x_counter, #64
	cmp	x_counter, 63

	dup	d_x0_h, v_x0.d[1]
	dup	d_x1_h, v_x1.d[1]
	dup	d_x2_h, v_x2.d[1]
	dup	d_x3_h, v_x3.d[1]

	dup	d_x0_l, v_x0.d[0]
	dup	d_x1_l, v_x1.d[0]
	dup	d_x2_l, v_x2.d[0]
	dup	d_x3_l, v_x3.d[0]

	ldr	q_y0, [x_buf_saved, -64]
	ldr	q_y1, [x_buf_saved, -48]
	ldr	q_y2, [x_buf_saved, -32]
	ldr	q_y3, [x_buf_saved, -16]

	pmull	v_x0_h.1q, v_x0_h.1d, v_p4_h.1d
	pmull	v_x0_l.1q, v_x0_l.1d, v_p4_l.1d
	pmull	v_x1_h.1q, v_x1_h.1d, v_p4_h.1d
	pmull	v_x1_l.1q, v_x1_l.1d, v_p4_l.1d
	pmull	v_x2_h.1q, v_x2_h.1d, v_p4_h.1d
	pmull	v_x2_l.1q, v_x2_l.1d, v_p4_l.1d
	pmull	v_x3_h.1q, v_x3_h.1d, v_p4_h.1d
	pmull	v_x3_l.1q, v_x3_l.1d, v_p4_l.1d

	tbl	v_y0.16b, {v_y0.16b}, v7.16b
	tbl	v_y1.16b, {v_y1.16b}, v7.16b
	tbl	v_y2.16b, {v_y2.16b}, v7.16b
	tbl	v_y3.16b, {v_y3.16b}, v7.16b

	eor	v_tmp1_x0.16b, v_x0_h.16b, v_x0_l.16b
	eor	v_tmp1_x1.16b, v_x1_h.16b, v_x1_l.16b
	eor	v_tmp1_x2.16b, v_x2_h.16b, v_x2_l.16b
	eor	v_tmp1_x3.16b, v_x3_h.16b, v_x3_l.16b

	eor	v_x0.16b, v_tmp1_x0.16b, v_y0.16b
	eor	v_x1.16b, v_tmp1_x1.16b, v_y1.16b
	eor	v_x2.16b, v_tmp1_x2.16b, v_y2.16b
	eor	v_x3.16b, v_tmp1_x3.16b, v_y3.16b

	bhi	.crc_fold_loop

/* carry less multiplication, part3 - after loop */
/* folding 512bit ---> 128bit */

// input parameters:
// v_x0 => v2
// v_x1 => v3
// v_x2 => v4
// v_x3 => v5

// v0, v1, v6, v30, are tmp registers

.crc_fold_loop_end:
	mov	x_tmp, 0x4c1a0000	/* p1 [1] */
	fmov	d0, x_tmp
	mov	x_tmp, 0xfb0b0000	/* p1 [0] */
	fmov	d1, x_tmp

	and	w_counter, w_len, -64
	sxtw	x_tmp, w_counter
	add	x_buf, x_buf, x_tmp

	dup	d6, v_x0.d[1]
	dup	d30, v_x0.d[0]
	pmull	v6.1q, v6.1d, v0.1d
	pmull	v30.1q, v30.1d, v1.1d
	eor	v6.16b, v6.16b, v30.16b
	eor	v_x1.16b, v6.16b, v_x1.16b

	dup	d6, v_x1.d[1]
	dup	d30, v_x1.d[0]
	pmull	v6.1q, v6.1d, v0.1d
	pmull	v16.1q, v30.1d, v1.1d
	eor	v6.16b, v6.16b, v16.16b
	eor	v_x2.16b, v6.16b, v_x2.16b

	dup	d_x0, v_x2.d[1]
	dup	d30, v_x2.d[0]
	pmull	v0.1q, v_x0.1d, v0.1d
	pmull	v_x0.1q, v30.1d, v1.1d
	eor	v1.16b, v0.16b, v_x0.16b
	eor	v_x0.16b, v1.16b, v_x3.16b

/* carry less multiplication, part3 - after loop */
/* crc16 fold function */
d_16fold_p0_h		.req	d18
v_16fold_p0_h		.req	v18

d_16fold_p0_l		.req	d4
v_16fold_p0_l		.req	v4

v_16fold_from		.req	v_x0
d_16fold_from_h		.req	d3
v_16fold_from_h		.req	v3

v_16fold_zero		.req	v7

v_16fold_from1		.req	v16

v_16fold_from2		.req	v0
d_16fold_from2_h	.req	d6
v_16fold_from2_h	.req	v6

v_16fold_tmp		.req	v0

	movi	v_16fold_zero.4s, 0
	mov	x_tmp1, 0x2d560000		/* p0 [1] */
	mov	x_tmp2, 0x13680000		/* p0 [0] */

	ext	v_16fold_tmp.16b, v_16fold_zero.16b, v_16fold_from.16b, #8
	ext	v_16fold_tmp.16b, v0.16b, v_16fold_zero.16b, #4

	dup	d_16fold_from_h, v_16fold_from.d[1]
	fmov	d_16fold_p0_h, x_tmp1
	pmull	v_16fold_from1.1q, v_16fold_from_h.1d, v_16fold_p0_h.1d
	eor	v_16fold_from2.16b, v_16fold_tmp.16b, v_16fold_from1.16b

	dup	d_16fold_from2_h, v_16fold_from2.d[1]
	fmov	d_16fold_p0_l, x_tmp2
	pmull	v6.1q, v_16fold_from2_h.1d, v_16fold_p0_l.1d
	eor	v_x0.16b, v0.16b, v6.16b

/* carry less multiplication, part3 - after loop */
/* crc16 barrett reduction function */

// input parameters:
// v_x0:			v2
// barrett reduction constant:	br[0], br[1]

d_br0			.req	d3
v_br0			.req	v3
d_br1			.req	d5
v_br1			.req	v5

	mov	x_tmp1, 0x57f9			/* br[0] low */
	movk	x_tmp1, 0xf65a, lsl 16		/* br[0] high */
	movk	x_tmp1, 0x1, lsl 32
	fmov	d_br0, x_tmp1

	dup	d1, v_x0.d[0]
	dup	d1, v1.d[0]
	ext	v1.16b, v1.16b, v7.16b, #4
	pmull	v4.1q, v1.1d, v_br0.1d

	ext	v1.16b, v4.16b, v7.16b, #4
	mov	x_tmp1, 0x8bb70000		/* br[1] low */
	movk	x_tmp1, 0x1, lsl 32		/* br[1] high */

	fmov	d_br1, x_tmp1
	pmull	v_br1.1q, v1.1d, v_br1.1d
	eor	v_x0.16b, v_x0.16b, v_br1.16b

	umov	x0, v_x0.d[0]
	ubfx	x0, x0, 16, 16
	b	.crc_table_loop_pre

	.size	crc16_t10dif_pmull, .-crc16_t10dif_pmull

	.section	.rodata

	.align	4
.shuffle_mask_lanchor = . + 0
	.type	shuffle_mask, %object
	.size	shuffle_mask, 16
shuffle_mask:
	.byte	15, 14, 13, 12, 11, 10, 9, 8
	.byte	7,   6,  5,  4,  3,  2, 1, 0

	.align	4
.LANCHOR0 = . + 0
	.type	crc16tab, %object
	.size	crc16tab, 512
crc16tab:
	.hword  0x0000, 0x8bb7, 0x9cd9, 0x176e, 0xb205, 0x39b2, 0x2edc, 0xa56b
	.hword  0xEFBD, 0x640a, 0x7364, 0xf8d3, 0x5db8, 0xd60f, 0xc161, 0x4ad6
	.hword  0x54CD, 0xdf7a, 0xc814, 0x43a3, 0xe6c8, 0x6d7f, 0x7a11, 0xf1a6
	.hword  0xBB70, 0x30c7, 0x27a9, 0xac1e, 0x0975, 0x82c2, 0x95ac, 0x1e1b
	.hword  0xA99A, 0x222d, 0x3543, 0xbef4, 0x1b9f, 0x9028, 0x8746, 0x0cf1
	.hword  0x4627, 0xcd90, 0xdafe, 0x5149, 0xf422, 0x7f95, 0x68fb, 0xe34c
	.hword  0xFD57, 0x76e0, 0x618e, 0xea39, 0x4f52, 0xc4e5, 0xd38b, 0x583c
	.hword  0x12EA, 0x995d, 0x8e33, 0x0584, 0xa0ef, 0x2b58, 0x3c36, 0xb781
	.hword  0xD883, 0x5334, 0x445a, 0xcfed, 0x6a86, 0xe131, 0xf65f, 0x7de8
	.hword  0x373E, 0xbc89, 0xabe7, 0x2050, 0x853b, 0x0e8c, 0x19e2, 0x9255
	.hword  0x8C4E, 0x07f9, 0x1097, 0x9b20, 0x3e4b, 0xb5fc, 0xa292, 0x2925
	.hword  0x63F3, 0xe844, 0xff2a, 0x749d, 0xd1f6, 0x5a41, 0x4d2f, 0xc698
	.hword  0x7119, 0xfaae, 0xedc0, 0x6677, 0xc31c, 0x48ab, 0x5fc5, 0xd472
	.hword  0x9EA4, 0x1513, 0x027d, 0x89ca, 0x2ca1, 0xa716, 0xb078, 0x3bcf
	.hword  0x25D4, 0xae63, 0xb90d, 0x32ba, 0x97d1, 0x1c66, 0x0b08, 0x80bf
	.hword  0xCA69, 0x41de, 0x56b0, 0xdd07, 0x786c, 0xf3db, 0xe4b5, 0x6f02
	.hword  0x3AB1, 0xb106, 0xa668, 0x2ddf, 0x88b4, 0x0303, 0x146d, 0x9fda
	.hword  0xD50C, 0x5ebb, 0x49d5, 0xc262, 0x6709, 0xecbe, 0xfbd0, 0x7067
	.hword  0x6E7C, 0xe5cb, 0xf2a5, 0x7912, 0xdc79, 0x57ce, 0x40a0, 0xcb17
	.hword  0x81C1, 0x0a76, 0x1d18, 0x96af, 0x33c4, 0xb873, 0xaf1d, 0x24aa
	.hword  0x932B, 0x189c, 0x0ff2, 0x8445, 0x212e, 0xaa99, 0xbdf7, 0x3640
	.hword  0x7C96, 0xf721, 0xe04f, 0x6bf8, 0xce93, 0x4524, 0x524a, 0xd9fd
	.hword  0xC7E6, 0x4c51, 0x5b3f, 0xd088, 0x75e3, 0xfe54, 0xe93a, 0x628d
	.hword  0x285B, 0xa3ec, 0xb482, 0x3f35, 0x9a5e, 0x11e9, 0x0687, 0x8d30
	.hword  0xE232, 0x6985, 0x7eeb, 0xf55c, 0x5037, 0xdb80, 0xccee, 0x4759
	.hword  0x0D8F, 0x8638, 0x9156, 0x1ae1, 0xbf8a, 0x343d, 0x2353, 0xa8e4
	.hword  0xB6FF, 0x3d48, 0x2a26, 0xa191, 0x04fa, 0x8f4d, 0x9823, 0x1394
	.hword  0x5942, 0xd2f5, 0xc59b, 0x4e2c, 0xeb47, 0x60f0, 0x779e, 0xfc29
	.hword  0x4BA8, 0xc01f, 0xd771, 0x5cc6, 0xf9ad, 0x721a, 0x6574, 0xeec3
	.hword  0xA415, 0x2fa2, 0x38cc, 0xb37b, 0x1610, 0x9da7, 0x8ac9, 0x017e
	.hword  0x1F65, 0x94d2, 0x83bc, 0x080b, 0xad60, 0x26d7, 0x31b9, 0xba0e
	.hword  0xF0D8, 0x7b6f, 0x6c01, 0xe7b6, 0x42dd, 0xc96a, 0xde04, 0x55b3
