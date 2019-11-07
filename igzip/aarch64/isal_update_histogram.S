/**********************************************************************
  Copyright(c) 2019 Arm Corporation All rights reserved.

  Redistribution and use in source and binary forms, with or without
  modification, are permitted provided that the following conditions
  are met:
    * Redistributions of source code must retain the above copyright
      notice, this list of conditions and the following disclaimer.
    * Redistributions in binary form must reproduce the above copyright
      notice, this list of conditions and the following disclaimer in
      the documentation and/or other materials provided with the
      distribution.
    * Neither the name of Arm Corporation nor the names of its
      contributors may be used to endorse or promote products derived
      from this software without specific prior written permission.

  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
  "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
  LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
  A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
  OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
  SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
  LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
  DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
  THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
  (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
  OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
**********************************************************************/
	.arch armv8-a+crc
	.text
	.align	2

#include "lz0a_const_aarch64.h"
#include "data_struct_aarch64.h"
#include "huffman_aarch64.h"
#include "bitbuf2_aarch64.h"
#include "stdmac_aarch64.h"

/*
declare Macros
*/

.macro	declare_generic_reg name:req,reg:req,default:req
	\name		.req	\default\reg
	w_\name		.req	w\reg
	x_\name		.req	x\reg
.endm

.macro convert_dist_to_dist_sym dist:req,tmp0:req,tmp1:req
	mov     w_\tmp0, w_\dist
	mov     w_\dist, -1
	cmp     w_\tmp0, 32768
	bhi     .dist2code_done
	sub     w_\dist, w_\tmp0, #1
	cmp     w_\tmp0, 4
	bls     .dist2code_done
	clz     w_\tmp1, w_\dist
	mov     w_\tmp0, 30
	sub     w_\tmp0, w_\tmp0, w_\tmp1
	lsr     w_\dist, w_\dist, w_\tmp0
	add     w_\dist, w_\dist, w_\tmp0, lsl 1
.dist2code_done:
.endm

.macro convert_length_to_len_sym length:req,length_out:req,tmp0:req
        adrp    x_\tmp0, .len_to_code_tab_lanchor
        add     x_\tmp0, x_\tmp0, :lo12:.len_to_code_tab_lanchor
        ldr     w_\length_out, [x_\tmp0, w_\length, uxtw 2]
        add     w_\length_out, w_\length_out, 256
.endm

        .section        .rodata
        .align  4
.len_to_code_tab_lanchor = . + 0
        .type   len_to_code_tab, %object
        .size   len_to_code_tab, 1056
len_to_code_tab:
        .word 0x00, 0x00, 0x00
        .word 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08
        .word 0x09, 0x09, 0x0a, 0x0a, 0x0b, 0x0b, 0x0c, 0x0c
        .word 0x0d, 0x0d, 0x0d, 0x0d, 0x0e, 0x0e, 0x0e, 0x0e
        .word 0x0f, 0x0f, 0x0f, 0x0f, 0x10, 0x10, 0x10, 0x10
        .word 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11
        .word 0x12, 0x12, 0x12, 0x12, 0x12, 0x12, 0x12, 0x12
        .word 0x13, 0x13, 0x13, 0x13, 0x13, 0x13, 0x13, 0x13
        .word 0x14, 0x14, 0x14, 0x14, 0x14, 0x14, 0x14, 0x14
        .word 0x15, 0x15, 0x15, 0x15, 0x15, 0x15, 0x15, 0x15
        .word 0x15, 0x15, 0x15, 0x15, 0x15, 0x15, 0x15, 0x15
        .word 0x16, 0x16, 0x16, 0x16, 0x16, 0x16, 0x16, 0x16
        .word 0x16, 0x16, 0x16, 0x16, 0x16, 0x16, 0x16, 0x16
        .word 0x17, 0x17, 0x17, 0x17, 0x17, 0x17, 0x17, 0x17
        .word 0x17, 0x17, 0x17, 0x17, 0x17, 0x17, 0x17, 0x17
        .word 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18
        .word 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18
        .word 0x19, 0x19, 0x19, 0x19, 0x19, 0x19, 0x19, 0x19
        .word 0x19, 0x19, 0x19, 0x19, 0x19, 0x19, 0x19, 0x19
        .word 0x19, 0x19, 0x19, 0x19, 0x19, 0x19, 0x19, 0x19
        .word 0x19, 0x19, 0x19, 0x19, 0x19, 0x19, 0x19, 0x19
        .word 0x1a, 0x1a, 0x1a, 0x1a, 0x1a, 0x1a, 0x1a, 0x1a
        .word 0x1a, 0x1a, 0x1a, 0x1a, 0x1a, 0x1a, 0x1a, 0x1a
        .word 0x1a, 0x1a, 0x1a, 0x1a, 0x1a, 0x1a, 0x1a, 0x1a
        .word 0x1a, 0x1a, 0x1a, 0x1a, 0x1a, 0x1a, 0x1a, 0x1a
        .word 0x1b, 0x1b, 0x1b, 0x1b, 0x1b, 0x1b, 0x1b, 0x1b
        .word 0x1b, 0x1b, 0x1b, 0x1b, 0x1b, 0x1b, 0x1b, 0x1b
        .word 0x1b, 0x1b, 0x1b, 0x1b, 0x1b, 0x1b, 0x1b, 0x1b
        .word 0x1b, 0x1b, 0x1b, 0x1b, 0x1b, 0x1b, 0x1b, 0x1b
        .word 0x1c, 0x1c, 0x1c, 0x1c, 0x1c, 0x1c, 0x1c, 0x1c
        .word 0x1c, 0x1c, 0x1c, 0x1c, 0x1c, 0x1c, 0x1c, 0x1c
        .word 0x1c, 0x1c, 0x1c, 0x1c, 0x1c, 0x1c, 0x1c, 0x1c
        .word 0x1c, 0x1c, 0x1c, 0x1c, 0x1c, 0x1c, 0x1c, 0x1d
        .word 0x00, 0x00, 0x00, 0x00, 0x00

	.text
	.global	isal_update_histogram_aarch64
	.arch armv8-a+crc
	.type	isal_update_histogram_aarch64, %function

/*
void isal_update_histogram_aarch64(uint8_t * start_stream, int length,
				struct isal_huff_histogram *histogram);
*/

	/* arguments */
	declare_generic_reg	start_stream,		0,x
	declare_generic_reg	length,			1,x
	declare_generic_reg	histogram,		2,x

	declare_generic_reg	param0,			0,x
	declare_generic_reg	param1,			1,x
	declare_generic_reg	param2,			2,x

	/* local variable */
	declare_generic_reg	start_stream_saved,	10,x
	declare_generic_reg	histogram_saved,	23,x
	declare_generic_reg	current,		19,x
	declare_generic_reg	last_seen,		20,x
	declare_generic_reg	end_stream,		21,x
	declare_generic_reg	loop_end_iter,		22,x
	declare_generic_reg	dist_histogram,		12,x
	declare_generic_reg	lit_len_histogram,	23,x
	declare_generic_reg	literal,		8,x
	declare_generic_reg	next_hash,		9,x
	declare_generic_reg	end,			4,x
	declare_generic_reg	dist,			7,x
	declare_generic_reg	D,			11,w
	declare_generic_reg	match_length,		3,w

	declare_generic_reg	tmp0,			5,w
	declare_generic_reg	tmp1,			6,w

/* constant */
.equ	LIT_LEN, 286
.equ	DIST_LEN, 30

.equ	lit_len_offset, 0
.equ	dist_offset, (8*LIT_LEN)                   // 2288
.equ	hash_offset, (dist_offset + 8*DIST_LEN)    // 2528
.equ	hash_table_size, (8*1024*2)                // 16384

isal_update_histogram_aarch64:
	cmp	w_length, 0
	ble	.done

	stp	x29, x30, [sp, -64]!
	add	x29, sp, 0
	stp	x19, x20, [sp, 16]
	stp	x21, x22, [sp, 32]
	str	x23, [sp, 48]

	add	last_seen, histogram, hash_offset
	add	end_stream, start_stream, w_length, sxtw
	mov	current, start_stream
	sub	loop_end_iter, end_stream, #3
	mov	histogram_saved, histogram

	mov	x0, last_seen
	mov	w1, 0
	mov	x2, hash_table_size
	bl	memset

	cmp	current, loop_end_iter
	bcs	.loop_end

	mov	start_stream_saved, current
	add	dist_histogram, histogram_saved, dist_offset
	mov	D, 32766
	b	.loop

	.align 2
.loop_2nd_stream:
	and	literal, literal, 0xff
	mov	current, next_hash
	cmp	loop_end_iter, current

	ldr	x0, [lit_len_histogram, literal, lsl 3]
	add	x0, x0, 1
	str	x0, [lit_len_histogram, literal, lsl 3]
	bls	.loop_end

.loop:
	ldr	w_literal, [current]
	add	next_hash, current, 1

	mov	w0, w_literal
	crc32cw	w0, wzr, w0

	ubfiz	x0, x0, 1, 13
	sub	x2, current, start_stream_saved
	ldrh	w_dist, [last_seen, x0]
	strh	w2, [last_seen, x0]
	sub	w2, w2, w_dist
	and	w_dist, w2, 65535

	sub	w0, w_dist, #1
	cmp	w0, D
	bhi	.loop_2nd_stream

	sub	w2, w_end_stream, w_current
	mov	x1, current
	sub	x0, current, w_dist, uxth
	compare_max_258_bytes param0,param1,param2,match_length,tmp0,tmp1

	cmp	match_length, 3
	bls	.loop_2nd_stream

	add	end, current, 3
	cmp	end, loop_end_iter
	csel	end, end, loop_end_iter, ls
	cmp	end, next_hash
	bls	.skip_inner_loop

	.align 3
.inner_loop:
	ldr	w0, [next_hash]
	crc32cw	w0, wzr, w0

	ubfiz	x0, x0, 1, 13
	sub	x1, next_hash, start_stream_saved
	add	next_hash, next_hash, 1
	cmp	next_hash, end
	strh	w1, [last_seen, x0]
	bne	.inner_loop

.skip_inner_loop:
	convert_dist_to_dist_sym dist, tmp0, tmp1
	uxtw	x2, w_dist
	ldr	x1, [dist_histogram, x2, lsl 3]
	add	x1, x1, 1
	str	x1, [dist_histogram, x2, lsl 3]

	convert_length_to_len_sym match_length,tmp1,tmp0
	uxtw	x0, w_tmp1
	ldr	x1, [lit_len_histogram, x0, lsl 3]
	add	x1, x1, 1
	str	x1, [lit_len_histogram, x0, lsl 3]

	sub	match_length, match_length, #1
	add	x3, x3, 1
	add	current, current, x3
	cmp	loop_end_iter, current
	bhi	.loop

	.align 3
// fold the last for loop
.loop_end:
	cmp	end_stream, current
	bls	.loop_fold_end

	mov	x0, current
	ldrb	w1, [x0], 1
	cmp	end_stream, x0
	ldr	x0, [lit_len_histogram, x1, lsl 3]
	add	x0, x0, 1
	str	x0, [lit_len_histogram, x1, lsl 3]
	bls	.loop_fold_end

	ldrb	w1, [current, 1]
	add	x0, current, 2
	cmp	end_stream, x0
	ldr	x0, [lit_len_histogram, x1, lsl 3]
	add	x0, x0, 1
	str	x0, [lit_len_histogram, x1, lsl 3]
	bls	.loop_fold_end

	ldrb	w1, [current, 2]
	add	x0, current, 3
	cmp	end_stream, x0
	ldr	x0, [lit_len_histogram, x1, lsl 3]
	add	x0, x0, 1
	str	x0, [lit_len_histogram, x1, lsl 3]
	bls	.loop_fold_end

	ldrb	w1, [current, 3]
	ldr	x0, [lit_len_histogram, x1, lsl 3]
	add	x0, x0, 1
	str	x0, [lit_len_histogram, x1, lsl 3]

.loop_fold_end:
	ldr	x0, [lit_len_histogram, (256*8)]
	add	x0, x0, 1
	str	x0, [lit_len_histogram, (256*8)]

	ldr	x23, [sp, 48]
	ldp	x19, x20, [sp, 16]
	ldp	x21, x22, [sp, 32]
	ldp	x29, x30, [sp], 64
	ret
	.align 2
.done:
	ret
	.size	isal_update_histogram_aarch64, .-isal_update_histogram_aarch64
