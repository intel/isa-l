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
	.global	crc32_iscsi_refl_hw_fold
	.type	crc32_iscsi_refl_hw_fold, %function

/* unsigned int crc32_iscsi_refl_hw_fold(unsigned char *buffer, int len, unsigned int crc_init) */

x_buffer	.req	x0
w_len		.req	w1
w_crc_init	.req	w2
w_crc		.req	w2

w_len_loop_end	.req	w9
x_buf_loop_end	.req	x9
x_buf_iter	.req	x9

x_tmp		.req	x15
w_tmp		.req	w15

w_crc_ret	.req	w0
crc32_iscsi_refl_hw_fold:
	cmp	w_len, 1023
	mov	x_buf_iter, x_buffer
	ble	.loop_fold_end

	sub	w10, w_len, #1024
	lsr	w12, w10, 10
	lsl	w_len_loop_end, w12, 10

	add	x_buf_loop_end, x_buf_loop_end, 1024
	add	x_buf_loop_end, x_buffer, x_buf_loop_end

	mov	x_tmp, 0xf38a
	movk	x_tmp, 0xe417, lsl 16
	fmov	d3, x_tmp

	mov	x_tmp, 0x8014
	movk	x_tmp, 0x8f15, lsl 16
	fmov	d1, x_tmp

x_in64	.req	x1
w_crc0	.req	w2
w_crc1	.req	w3
w_crc2	.req	w4
	.align 3
.loop_fold:
	add	x8, x_buffer, 336
	mov	x_in64, x_buffer
	mov	w_crc1, 0
	mov	w_crc2, 0

	.align 3
.loop_for:
	ldr	x7, [x_in64]
	ldr	x6, [x_in64, 336]
	ldr	x5, [x_in64, 672]

	add	x_in64, x_in64, 8
	cmp	x_in64, x8

	crc32cx	w_crc0, w_crc0, x7
	crc32cx	w_crc1, w_crc1, x6
	crc32cx	w_crc2, w_crc2, x5
	bne	.loop_for

	uxtw	x_tmp, w_crc0
	fmov	d4, x_tmp
	pmull	v2.1q, v4.1d, v3.1d

	uxtw	x_tmp, w_crc1
	fmov	d5, x_tmp
	pmull	v5.1q, v5.1d, v1.1d

	fmov	x_tmp, d2
	crc32cx	w_crc0, wzr, x_tmp

	fmov	x_tmp, d5
	crc32cx	w_crc1, wzr, x_tmp

	ldr	x_tmp, [x_buffer, 1008]
	crc32cx	w_crc2, w_crc2, x_tmp

	eor	w_crc1, w_crc1, w_crc0
	eor	w_crc1, w_crc1, w_crc2

	ldr	x_tmp, [x_buffer, 1016]
	crc32cx	w_crc0, w_crc1, x_tmp

	add	x_buffer, x_buffer, 1024
	cmp	x_buf_loop_end, x_buffer
	bne	.loop_fold

	sub	w_len, w10, w12, lsl 10

x_buf_loop_size8_end	.req	x3
.loop_fold_end:
	cmp	w_len, 7
	ble	.size_4

	sub	w_len, w_len, #8
	lsr	w4, w_len, 3
	lsl	w3, w4, 3
	add	x_buf_loop_size8_end, x_buf_loop_size8_end, 8
	add	x_buf_loop_size8_end, x_buf_iter, x_buf_loop_size8_end

	.align 3
.loop_size_8:
	ldr	x_tmp, [x_buf_iter], 8
	crc32cx	w_crc, w_crc, x_tmp

	cmp	x_buf_iter, x_buf_loop_size8_end
	bne	.loop_size_8

	sub	w_len, w_len, w4, lsl 3
.size_4:
	cmp	w_len, 3
	ble	.size_2

	ldr	w_tmp, [x_buf_iter], 4
	crc32cw	w_crc, w_crc, w_tmp
	sub	w_len, w_len, #4

.size_2:
	cmp	w_len, 1
	ble	.size_1

	ldrh	w_tmp, [x_buf_iter], 2
	crc32ch	w_crc, w_crc, w_tmp
	sub	w_len, w_len, #2

.size_1:
	mov	w_crc_ret, w_crc
	cmp	w_len, 1
	bne	.done

	ldrb	w_tmp, [x_buf_iter]
	crc32cb	w_crc_ret, w_crc, w_tmp

.done:
	ret

	.size	crc32_iscsi_refl_hw_fold, .-crc32_iscsi_refl_hw_fold
