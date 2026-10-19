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

	.global	cdecl(gen_icf_map_h1_aarch64)
#ifndef __APPLE__
	.type	gen_icf_map_h1_aarch64, %function
#endif

/*
	uint64_t gen_icf_map_h1_aarch64(struct isal_zstream *stream,
		struct deflate_icf *matches_icf_lookup, uint64_t input_size)

	Produces the same output as gen_icf_map_h1_base(), including its
	multiplicative compute_hash(), so it pairs with isal_deflate_hash_base()
	for preset dictionaries. The loop has no data-dependent branches: it
	builds both the literal and the match code for each position and
	selects one.
*/

	/* arguments */
	declare_generic_reg	stream,		0,x
	declare_generic_reg	next_icf,	1,x
	declare_generic_reg	input_size,	2,x

	/* variables */
	declare_generic_reg	next_in,	3,x
	declare_generic_reg	end_loop,	4,x
	declare_generic_reg	file_start,	5,x
	declare_generic_reg	hash_table,	6,x
	declare_generic_reg	hash_mask,	7,w
	declare_generic_reg	hist_size,	8,w
	declare_generic_reg	hash_mult,	9,x
	declare_generic_reg	start_in,	10,x
	declare_generic_reg	bytes,		11,x
	declare_generic_reg	hash,		12,x
	declare_generic_reg	pos,		13,x
	declare_generic_reg	dist,		14,x
	declare_generic_reg	match_len,	15,x
	declare_generic_reg	tmp0,		16,x
	declare_generic_reg	tmp1,		17,x
	declare_generic_reg	tmp2,		2,x
	/* stream is not used after the setup */
	declare_generic_reg	const_30,	0,x

/* constant */
.equ	ISAL_LOOK_AHEAD, 288
.equ	SHORTEST_MATCH, 4
.equ	LEN_OFFSET, 254
.equ	IGZIP_HIST, 1
.equ	LIT_ICF_DIST, (30 << 10)

/* offset of struct isal_zstream */
.equ	offset_next_in, 0
.equ	offset_total_in, 12
.equ	offset_level_buf, 48
.equ	offset_state_dist_mask, 76
.equ	offset_state_has_hist, 135

/* offset of struct level_buf */
.equ	offset_hash_map_hash_table, 4712

/*
	compute_hash(): hash = ((data * 0xB2D06057) >> 16) * 0xB2D06057 >> 16,
	with 64-bit wrapping multiplies, then masked with hash_mask. hash_mask
	is less than 1 << 16, so the masked hash only depends on the low 32 bits
	of the second product, and a 32-bit multiply gives the same result.
*/
.macro compute_hash data:req, hash:req
	umull	\hash, w_\data, w_hash_mult
	lsr	\hash, \hash, 16
	mul	w_\hash, w_\hash, w_hash_mult
	and	w_\hash, hash_mask, w_\hash, lsr 16
.endm

/*
	One position of the 4-wide loop: update the hash table, compute d and
	match_len like the scalar loop, and put them in lane \lane of v4 and v5.
	pos holds the position on entry and the next position on exit.
*/
.macro icf_map_lane lane:req, hash_reg:req
	ldrh	w_dist, [hash_table, w_\hash_reg, uxtw 1]
	strh	w_pos, [hash_table, w_\hash_reg, uxtw 1]
	sub	w_dist, w_pos, w_dist
	sub	w_dist, w_dist, 1
	and	w_dist, w_dist, hist_size
	sub	tmp0, next_in, dist
	ldur	tmp0, [tmp0, \lane - 1]
	ldur	bytes, [next_in, \lane]
	eor	tmp0, tmp0, bytes
	rbit	tmp0, tmp0
	clz	tmp0, tmp0
	lsr	w_match_len, w_tmp0, 3
.if \lane == 0
	fmov	s4, w_dist
	fmov	s5, w_match_len
.else
	mov	v4.s[\lane], w_dist
	mov	v5.s[\lane], w_match_len
.endif
	add	w_pos, w_pos, 1
.endm

cdecl(gen_icf_map_h1_aarch64):
	cmp	input_size, ISAL_LOOK_AHEAD
	blo	.return_zero

	ldr	next_in, [stream, offset_next_in]
	ldr	w_tmp0, [stream, offset_total_in]
	ldr	tmp1, [stream, offset_level_buf]
	ldp	hist_size, hash_mask, [stream, offset_state_dist_mask]
	mov	hash_table, offset_hash_map_hash_table
	add	hash_table, tmp1, hash_table
	sub	file_start, next_in, tmp0
	add	end_loop, next_in, input_size
	sub	end_loop, end_loop, ISAL_LOOK_AHEAD
	mov	start_in, next_in
	mov	w_hash_mult, 0x6057
	movk	w_hash_mult, 0xb2d0, lsl 16

	/* With no history, the first byte is a literal */
	ldrb	w_tmp0, [stream, offset_state_has_hist]
	cbnz	w_tmp0, .loop_entry
	ldr	w_bytes, [next_in]
	compute_hash	bytes, hash
	sub	pos, next_in, file_start
	strh	w_pos, [hash_table, hash, lsl 1]
	and	w_bytes, w_bytes, 0xff
	orr	w_bytes, w_bytes, LIT_ICF_DIST
	str	w_bytes, [next_icf], 4
	add	next_in, next_in, 1
	mov	w_tmp0, IGZIP_HIST
	strb	w_tmp0, [stream, offset_state_has_hist]

.loop_entry:
	/*
		Four positions per iteration while at least four remain. NEON
		computes the hashes, distance codes, and deflate_icf entries. The
		hash table updates, the candidate loads, and the match lengths stay
		scalar, one position after the other, so a position sees the hash
		table update of the position before it.
	*/
	sub	tmp1, end_loop, 3
	cmp	next_in, tmp1
	bhs	.loop1_entry

	adr	tmp0, .hash_word_idx
	ldr	q16, [tmp0]
	dup	v17.4s, w_hash_mult
	dup	v18.4s, hash_mask
	movi	v19.4s, 30
	movi	v20.4s, 0
	movi	v21.4s, LEN_OFFSET
	movi	v22.4s, 0xff
	movi	v23.4s, (LIT_ICF_DIST >> 8), lsl 8
	movi	v24.4s, SHORTEST_MATCH

	.align	5
.loop4:
	/* v2 = compute_hash() of the 4-byte words at next_in + 0..3 */
	ldr	q0, [next_in]
	tbl	v1.16b, {v0.16b}, v16.16b
	umull	v2.2d, v1.2s, v17.2s
	umull2	v3.2d, v1.4s, v17.4s
	shrn	v2.2s, v2.2d, 16
	shrn2	v2.4s, v3.2d, 16
	mul	v2.4s, v2.4s, v17.4s
	ushr	v2.4s, v2.4s, 16
	and	v2.16b, v2.16b, v18.16b
	fmov	hash, d2
	mov	tmp2, v2.d[1]
	sub	pos, next_in, file_start

	icf_map_lane	0, hash
	lsr	hash, hash, 32
	icf_map_lane	1, hash
	icf_map_lane	2, tmp2
	lsr	tmp2, tmp2, 32
	icf_map_lane	3, tmp2

	/*
		v4 = d and v5 = match_len for the 4 positions. Distance code:
		with n = max(30 - clz(d), 0), code = (d >> n) + 2 * n and
		extra = d - ((d >> n) << n).
	*/
	clz	v6.4s, v4.4s
	sub	v6.4s, v19.4s, v6.4s
	smax	v6.4s, v6.4s, v20.4s
	neg	v7.4s, v6.4s
	ushl	v7.4s, v4.4s, v7.4s
	ushl	v3.4s, v7.4s, v6.4s
	sub	v3.4s, v4.4s, v3.4s
	add	v7.4s, v7.4s, v6.4s
	add	v7.4s, v7.4s, v6.4s

	/* Select the match code when match_len >= SHORTEST_MATCH */
	add	v4.4s, v5.4s, v21.4s
	sli	v4.4s, v7.4s, 10
	sli	v4.4s, v3.4s, 19
	and	v6.16b, v1.16b, v22.16b
	orr	v6.16b, v6.16b, v23.16b
	cmhs	v7.4s, v5.4s, v24.4s
	bif	v4.16b, v6.16b, v7.16b
	st1	{v4.4s}, [next_icf], 16

	add	next_in, next_in, 4
	cmp	next_in, tmp1
	blo	.loop4

.loop1_entry:
	mov	w_const_30, 30
	cmp	next_in, end_loop
	bhs	.done

	.align	5
.loop:
	ldr	bytes, [next_in]
	compute_hash	bytes, hash
	sub	pos, next_in, file_start
	ldrh	w_dist, [hash_table, hash, lsl 1]
	strh	w_pos, [hash_table, hash, lsl 1]

	/* d = dist - 1, where dist = ((pos - last - 1) & hist_size) + 1 */
	sub	w_dist, w_pos, w_dist
	sub	w_dist, w_dist, 1
	and	w_dist, w_dist, hist_size

	/* match_len = tzbytecnt(load(next_in) ^ load(next_in - dist)) */
	sub	tmp0, next_in, dist
	ldur	tmp0, [tmp0, -1]
	eor	tmp0, tmp0, bytes
	rbit	tmp0, tmp0
	clz	tmp0, tmp0
	lsr	w_match_len, w_tmp0, 3

	/*
		Distance code: with n = max(30 - clz(d), 0) extra bits,
		code = (d >> n) + 2 * n and extra = d - ((d >> n) << n).
	*/
	clz	w_tmp0, w_dist
	subs	w_tmp0, w_const_30, w_tmp0
	csel	w_tmp0, w_tmp0, wzr, gt
	lsr	w_tmp2, w_dist, w_tmp0
	lsl	w_tmp1, w_tmp2, w_tmp0
	sub	w_tmp1, w_dist, w_tmp1
	add	w_tmp2, w_tmp2, w_tmp0, lsl 1

	/* Select the match code when match_len >= SHORTEST_MATCH */
	add	w_tmp0, w_match_len, LEN_OFFSET
	orr	w_tmp0, w_tmp0, w_tmp2, lsl 10
	orr	w_tmp0, w_tmp0, w_tmp1, lsl 19
	and	w_bytes, w_bytes, 0xff
	orr	w_bytes, w_bytes, LIT_ICF_DIST
	cmp	w_match_len, SHORTEST_MATCH
	csel	w_bytes, w_tmp0, w_bytes, hs
	str	w_bytes, [next_icf], 4

	add	next_in, next_in, 1
	cmp	next_in, end_loop
	blo	.loop

.done:
	sub	x0, next_in, start_in
	ret

.return_zero:
	mov	x0, 0
	ret

	/* tbl indices that put the 4-byte words at offsets 0..3 in 4 lanes */
	.balign	16
.hash_word_idx:
	.byte	0, 1, 2, 3, 1, 2, 3, 4, 2, 3, 4, 5, 3, 4, 5, 6
#ifndef __APPLE__
	.size	gen_icf_map_h1_aarch64, .-gen_icf_map_h1_aarch64
#endif
