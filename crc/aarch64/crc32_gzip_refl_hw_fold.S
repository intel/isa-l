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
	.global	crc32_gzip_refl_hw_fold
	.type	crc32_gzip_refl_hw_fold, %function

/* uint32_t crc32_gzip_refl_hw_fold(uint32_t seed, const unsigned char *buf, uint64_t len) */

w_seed		.req	w0
w_crc		.req	w0
x_buf		.req	x1
x_len		.req	x2

x_buf_loop_end	.req	x10
x_buf_iter	.req	x10

x_tmp		.req	x15
w_tmp		.req	w15

d_c0		.req	d3
d_c1		.req	d1
v_c0		.req	v3
v_c1		.req	v1
crc32_gzip_refl_hw_fold:
	mvn	w_seed, w_seed
	cmp	x_len, 1023
	mov	x_buf_iter, x_buf
	bls	.loop_fold_end

	sub	x_buf_loop_end, x_len, #1024
	and	x_buf_loop_end, x_buf_loop_end, -1024
	add	x_buf_loop_end, x_buf_loop_end, 1024
	add	x_buf_loop_end, x_buf, x_buf_loop_end

	mov	x_tmp, 0x819b
	movk	x_tmp, 0xb486, lsl 16
	fmov	d_c0, x_tmp

	mov	x_tmp, 0x8617
	movk	x_tmp, 0x7627, lsl 16
	fmov	d_c1, x_tmp

x_in64		.req	x3
w_crc0		.req	w0
w_crc1		.req	w4
w_crc2		.req	w5

d_crc0		.req	d4
d_crc1		.req	d5
v_crc0		.req	v4
v_crc1		.req	v5
	.align 3
.loop_fold:
	add	x9, x_buf, 336
	mov	x_in64, x_buf
	mov	w_crc1, 0
	mov	w_crc2, 0

	.align 3
.loop_for:
	ldr	x8, [x_in64]
	ldr	x7, [x_in64, 336]
	ldr	x6, [x_in64, 672]

	add	x_in64, x_in64, 8
	cmp	x_in64, x9

	crc32x	w_crc0, w_crc0, x8
	crc32x	w_crc1, w_crc1, x7
	crc32x	w_crc2, w_crc2, x6
	bne	.loop_for

	uxtw	x_tmp, w_crc0
	fmov	d_crc0, x_tmp
	pmull	v_crc0.1q, v_crc0.1d, v_c0.1d

	uxtw	x_tmp, w_crc1
	fmov	d_crc1, x_tmp
	pmull	v_crc1.1q, v_crc1.1d, v_c1.1d

	ldr	x_tmp, [x_buf, 1008]
	crc32x	w_crc2, w_crc2, x_tmp

	fmov	x_tmp, d_crc0
	crc32x	w_crc0, wzr, x_tmp

	fmov	x_tmp, d_crc1
	crc32x	w_crc1, wzr, x_tmp

	eor	w_crc0, w_crc0, w_crc1
	eor	w_crc0, w_crc0, w_crc2

	ldr	x_tmp, [x_buf, 1016]
	crc32x	w_crc0, w_crc0, x_tmp

	add	x_buf, x_buf, 1024
	cmp	x_buf_loop_end, x_buf
	bne	.loop_fold

	and	x_len, x_len, 1023

x_buf_loop_size8_end	.req	x3
.loop_fold_end:
	cmp	x_len, 7
	bls	.size_4

	sub	x_buf_loop_size8_end, x_len, #8
	and	x_buf_loop_size8_end, x_buf_loop_size8_end, -8
	add	x_buf_loop_size8_end, x_buf_loop_size8_end, 8
	add	x_buf_loop_size8_end, x_buf_iter, x_buf_loop_size8_end

	.align 3
.loop_size_8:
	ldr	x_tmp, [x_buf_iter], 8
	crc32x	w_crc, w_crc, x_tmp

	cmp	x_buf_iter, x_buf_loop_size8_end
	bne	.loop_size_8

	and	x_len, x_len, 7
.size_4:
	cmp	x_len, 3
	bls	.size_2

	ldr	w_tmp, [x_buf_iter], 4
	crc32w	w_crc, w_crc, w_tmp

	sub	x_len, x_len, #4
.size_2:
	cmp	x_len, 1
	bls	.size_1

	ldrh	w_tmp, [x_buf_iter], 2
	crc32h	w_crc, w_crc, w_tmp

	sub	x_len, x_len, #2
.size_1:
	cbz	x_len, .done

	ldrb	w_tmp, [x_buf_iter]
	crc32b	w_crc, w_crc, w_tmp

.done:
	mvn	w_crc, w_crc
	ret

	.size	crc32_gzip_refl_hw_fold, .-crc32_gzip_refl_hw_fold
