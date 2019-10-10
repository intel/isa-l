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

#include "crc_common_pmull.h"

.macro crc64_refl_func name:req
	.arch armv8-a+crypto
	.text
	.align	3
	.global	\name
	.type	\name, %function

/* uint64_t crc64_refl_func(uint64_t seed, const uint8_t * buf, uint64_t len) */

\name\():
	mvn	x_seed, x_seed
	mov	x_counter, 0
	cmp	x_len, (FOLD_SIZE-1)
	bhi	.crc_clmul_pre

.crc_tab_pre:
	cmp	x_len, x_counter
	bls	.done

	adrp	x_tmp, .lanchor_crc_tab
	add	x_buf_iter, x_buf, x_counter
	add	x_buf, x_buf, x_len
	add	x_crc_tab_addr, x_tmp, :lo12:.lanchor_crc_tab

	.align 3
.loop_crc_tab:
	ldrb	w_tmp, [x_buf_iter], 1
	eor	w_tmp, w_tmp, w0
	cmp	x_buf, x_buf_iter
	and	x_tmp, x_tmp, 255
	ldr	x_tmp, [x_crc_tab_addr, x_tmp, lsl 3]
	eor	x_seed, x_tmp, x_seed, lsr 8
	bne	.loop_crc_tab

.done:
	mvn	x_crc_ret, x_seed
	ret

	.align 2
.crc_clmul_pre:
	fmov	d_x0, x_seed // save crc to d_x0

	crc_refl_load_first_block

	bls	.clmul_loop_end

	crc64_load_p4

// 1024bit --> 512bit loop
// merge x0, x1, x2, x3, y0, y1, y2, y3 => x0, x1, x2, x3 (uint64x2_t)
	crc_refl_loop

.clmul_loop_end:
// folding 512bit --> 128bit
	crc64_fold_512b_to_128b

// folding 128bit --> 64bit
	mov	x_tmp, p0_low_b0
	movk	x_tmp, p0_low_b1, lsl 16
	movk	x_tmp, p0_low_b2, lsl 32
	movk	x_tmp, p0_low_b3, lsl 48
	fmov	d_p0_low, x_tmp

	pmull	v_tmp_low.1q, v_x3.1d, v_p0.1d

	mov	d_tmp_high, v_x3.d[1]

	eor	v_x3.16b, v_tmp_high.16b, v_tmp_low.16b

// barrett reduction
	mov	x_tmp, br_low_b0
	movk	x_tmp, br_low_b1, lsl 16
	movk	x_tmp, br_low_b2, lsl 32
	movk	x_tmp, br_low_b3, lsl 48
	fmov	d_br_low, x_tmp

	mov	x_tmp2, br_high_b0
	movk	x_tmp2, br_high_b1, lsl 16
	movk	x_tmp2, br_high_b2, lsl 32
	movk	x_tmp2, br_high_b3, lsl 48
	fmov	d_br_high, x_tmp2

	pmull	v_tmp_low.1q, v_x3.1d, v_br_low.1d
	pmull	v_tmp_high.1q, v_tmp_low.1d, v_br_high.1d

	ext	v_tmp_low.16b, v_br_low.16b, v_tmp_low.16b, #8

	eor	v_tmp_low.16b, v_tmp_low.16b, v_tmp_high.16b
	eor	v_tmp_low.16b, v_tmp_low.16b, v_x3.16b
	umov	x_crc_ret, v_tmp_low.d[1]

	b	.crc_tab_pre

	.size	\name, .-\name
.endm
