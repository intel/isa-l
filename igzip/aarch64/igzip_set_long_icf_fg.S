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

#include "../include/aarch64_label.h"

	.arch armv8-a
	.text
	.align	2

/*
declare Macros
*/

.macro	declare_generic_reg name:req,reg:req,default:req
	\name		.req	\default\reg
	w_\name		.req	w\reg
	x_\name		.req	x\reg
.endm

	.text
	.align	2
	.global	cdecl(set_long_icf_fg_aarch64)
#ifndef __APPLE__
	.type	set_long_icf_fg_aarch64, %function
#endif

/*
	void set_long_icf_fg_aarch64(uint8_t * next_in, uint64_t processed, uint64_t input_size,
				  struct deflate_icf *match_lookup)

	Produces the same output as set_long_icf_fg_base(). gen_icf_map compares
	at most 8 bytes. For each position whose entry has an 8-byte match, this
	extends the match, and writes it into that entry and the entries after
	it, one byte shorter each time, while it is longer than the match the
	entry already has. A NEON loop finds those positions 8 entries at a time.
*/

	/* arguments */
	declare_generic_reg	next_in,		0,x
	declare_generic_reg	processed,		1,x
	declare_generic_reg	input_size,		2,x
	declare_generic_reg	match_lookup,		3,x

	/* variables */
	declare_generic_reg	end_processed,		4,x
	declare_generic_reg	end_in,			5,x
	declare_generic_reg	end_scan,		6,x
	declare_generic_reg	dist_start,		7,x
	declare_generic_reg	entry,			8,x
	declare_generic_reg	len,			9,x
	declare_generic_reg	dist,			10,x
	declare_generic_reg	src,			11,x
	declare_generic_reg	dst,			12,x
	declare_generic_reg	cmp_end,		13,x
	declare_generic_reg	match_len,		14,x
	declare_generic_reg	tmp0,			15,x
	declare_generic_reg	tmp1,			16,x
	declare_generic_reg	tmp2,			17,x
	declare_generic_reg	tmp3,			1,x
	declare_generic_reg	cmp_start,		2,x

/* constant */
.equ	ISAL_LOOK_AHEAD, 288
.equ	LEN_OFFSET, 254
.equ	SHORTEST_MATCH, 4
.equ	LEN_MAX, 512
.equ	ICF_LIT_LEN_MASK, 0x3ff

cdecl(set_long_icf_fg_aarch64):
	add	end_processed, next_in, processed
	add	end_in, next_in, input_size
	add	tmp0, end_processed, ISAL_LOOK_AHEAD
	cmp	end_in, tmp0
	csel	end_in, end_in, tmp0, ls
	cmp	next_in, end_processed
	bhs	.done

	sub	end_scan, end_processed, 8
#ifndef __APPLE__
	adrp	dist_start, .data_dist_start
	add	dist_start, dist_start, :lo12:.data_dist_start
#else
	adrp	dist_start, .data_dist_start@PAGE
	add	dist_start, dist_start, .data_dist_start@PAGEOFF
#endif
	movi	v16.4s, (ICF_LIT_LEN_MASK >> 8), msl 8
	mov	w_tmp0, LEN_OFFSET + 8 - 1
	dup	v17.4s, w_tmp0

	/* Find the next entry with an 8-byte match, 8 entries at a time */
.scan8:
	cmp	next_in, end_scan
	bhi	.scan1
	ldp	q0, q1, [match_lookup]
	and	v0.16b, v0.16b, v16.16b
	and	v1.16b, v1.16b, v16.16b
	cmhi	v0.4s, v0.4s, v17.4s
	cmhi	v1.4s, v1.4s, v17.4s
	uzp1	v0.8h, v0.8h, v1.8h
	xtn	v0.8b, v0.8h
	fmov	tmp0, d0
	cbnz	tmp0, .found
	add	next_in, next_in, 8
	add	match_lookup, match_lookup, 32
	b	.scan8

.found:
	rbit	tmp0, tmp0
	clz	tmp0, tmp0
	lsr	tmp0, tmp0, 3
	add	next_in, next_in, tmp0
	add	match_lookup, match_lookup, tmp0, lsl 2
	b	.long_match

	/* Fewer than 8 entries are left */
.scan1:
	cmp	next_in, end_processed
	bhs	.done
	ldrh	w_len, [match_lookup]
	and	w_len, w_len, ICF_LIT_LEN_MASK
	cmp	w_len, LEN_OFFSET + 8 - 1
	bhi	.long_match
	add	next_in, next_in, 1
	add	match_lookup, match_lookup, 4
	b	.scan1

.long_match:
	ldr	w_entry, [match_lookup]
	and	w_len, w_entry, ICF_LIT_LEN_MASK
	ubfx	w_tmp0, w_entry, 10, 9
	lsr	w_tmp1, w_entry, 19
	ldr	w_dist, [dist_start, x_tmp0, lsl 2]
	add	w_dist, w_dist, w_tmp1

	/*
		match_len = number of equal bytes at next_in + 8 and dist bytes
		before it, up to end_in. Compare 16 bytes at a time.
	*/
	add	dst, next_in, 8
	sub	src, dst, dist
	mov	cmp_start, dst
	mov	cmp_end, end_in
.cmp16:
	sub	tmp0, cmp_end, dst
	cmp	tmp0, 16
	blo	.cmp_tail
	ldp	tmp0, tmp1, [src], 16
	ldp	tmp2, tmp3, [dst], 16
	eor	tmp0, tmp0, tmp2
	eor	tmp1, tmp1, tmp3
	orr	tmp2, tmp0, tmp1
	cbz	tmp2, .cmp16
	/* Count the equal bytes of the 16 before dst */
	sub	dst, dst, 16
	add	tmp2, dst, 8
	cmp	tmp0, 0
	csel	tmp0, tmp0, tmp1, ne
	csel	dst, dst, tmp2, ne
	rbit	tmp0, tmp0
	clz	tmp0, tmp0
	add	dst, dst, tmp0, lsr 3
	b	.cmp_done
.cmp_tail:
	cmp	dst, cmp_end
	bhs	.cmp_done
	ldrb	w_tmp0, [src], 1
	ldrb	w_tmp1, [dst]
	cmp	w_tmp0, w_tmp1
	bne	.cmp_done
	add	dst, dst, 1
	b	.cmp_tail
.cmp_done:
	sub	match_len, dst, cmp_start
	add	w_match_len, w_match_len, LEN_OFFSET + 8
	cmp	w_match_len, w_len
	bls	.skip

	/*
		Write the match while it is longer than the match of the entry
		and at least SHORTEST_MATCH. The entry keeps its distance bits.
	*/
	and	w_entry, w_entry, ~ICF_LIT_LEN_MASK
	mov	w_tmp1, LEN_MAX
	mov	w_tmp2, LEN_OFFSET + SHORTEST_MATCH - 1
.fill:
	cmp	w_match_len, w_tmp1
	csel	w_tmp0, w_match_len, w_tmp1, ls
	orr	w_tmp0, w_tmp0, w_entry
	str	w_tmp0, [match_lookup], 4
	add	next_in, next_in, 1
	sub	w_match_len, w_match_len, 1
	ldrh	w_tmp0, [match_lookup]
	and	w_tmp0, w_tmp0, ICF_LIT_LEN_MASK
	cmp	w_tmp0, w_tmp2
	csel	w_tmp0, w_tmp0, w_tmp2, hs
	cmp	w_tmp0, w_match_len
	blo	.fill

	/* Like set_long_icf_fg_base(), skip the entry that ends the match */
.skip:
	add	next_in, next_in, 1
	add	match_lookup, match_lookup, 4
	b	.scan8

.done:
	ret
#ifndef __APPLE__
	.size	set_long_icf_fg_aarch64, .-set_long_icf_fg_aarch64
#endif

ASM_DEF_RODATA
	.align	3
	.set	.data_dist_start,. + 0
.real_data_dist_start:
	.word	0x0001, 0x0002, 0x0003, 0x0004, 0x0005, 0x0007, 0x0009, 0x000d
	.word	0x0011, 0x0019, 0x0021, 0x0031, 0x0041, 0x0061, 0x0081, 0x00c1
	.word	0x0101, 0x0181, 0x0201, 0x0301, 0x0401, 0x0601, 0x0801, 0x0c01
	.word	0x1001, 0x1801, 0x2001, 0x3001, 0x4001, 0x6001, 0x0000, 0x0000
