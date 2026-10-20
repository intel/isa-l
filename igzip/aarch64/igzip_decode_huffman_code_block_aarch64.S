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
#include "lz0a_const_aarch64.h"
#include "huffman_aarch64.h"
#include "bitbuf2_aarch64.h"
#include "stdmac_aarch64.h"

#define ENABLE_TBL_INSTRUCTION	1

.macro start_struct name:req
	.set _FIELD_OFFSET,0
	.set _STRUCT_ALIGN,0
.endm
.macro end_struct name:req
	.set _\name\()_size,_FIELD_OFFSET
	.set _\name\()_align,_STRUCT_ALIGN
.endm
.macro field name:req, size:req, align:req
	.set _FIELD_OFFSET,(_FIELD_OFFSET + (\align) - 1) & (~ ((\align)-1))
	.set \name,_FIELD_OFFSET
	.set _FIELD_OFFSET,_FIELD_OFFSET + \size
	.if \align > _STRUCT_ALIGN
		.set _STRUCT_ALIGN, \align
	.endif
.endm

#define ISAL_DECODE_LONG_BITS 12
#define ISAL_DECODE_SHORT_BITS 10

#define L_REM (21 - ISAL_DECODE_LONG_BITS)
#define S_REM (15 - ISAL_DECODE_SHORT_BITS)
#define L_DUP ((1 << L_REM) - (L_REM + 1))
#define S_DUP ((1 << S_REM) - (S_REM + 1))
#define L_UNUSED ((1 << L_REM) - (1 << ((L_REM)/2)) - (1 << ((L_REM + 1)/2)) + 1)
#define S_UNUSED ((1 << S_REM) - (1 << ((S_REM)/2)) - (1 << ((S_REM + 1)/2)) + 1)
#define L_SIZE (286 + L_DUP + L_UNUSED)
#define S_SIZE (30 + S_DUP + S_UNUSED)
#define HUFF_CODE_LARGE_LONG_ALIGNED (L_SIZE + (-L_SIZE & 0xf))
#define HUFF_CODE_SMALL_LONG_ALIGNED (S_SIZE + (-S_SIZE & 0xf))
#define MAX_LONG_CODE_LARGE (L_SIZE + (-L_SIZE & 0xf))
#define MAX_LONG_CODE_SMALL (S_SIZE + (-S_SIZE & 0xf))
#define LARGE_SHORT_CODE_SIZE 4
#define LARGE_LONG_CODE_SIZE 2
#define SMALL_SHORT_CODE_SIZE 2
#define SMALL_LONG_CODE_SIZE 2


// inflate_huff_code
start_struct inflate_huff_code_large
	//	 name	size	align
	field _short_code_lookup_large,	LARGE_SHORT_CODE_SIZE*(1<<(ISAL_DECODE_LONG_BITS)),	LARGE_LONG_CODE_SIZE
	field _long_code_lookup_large,	LARGE_LONG_CODE_SIZE*MAX_LONG_CODE_LARGE,	LARGE_SHORT_CODE_SIZE
end_struct inflate_huff_code_large

// inflate_huff_code
start_struct inflate_huff_code_small
	//	 name	size	align
	field _short_code_lookup_small,	SMALL_SHORT_CODE_SIZE*(1<<(ISAL_DECODE_SHORT_BITS)),	SMALL_LONG_CODE_SIZE
	field _long_code_lookup_small,	SMALL_LONG_CODE_SIZE*MAX_LONG_CODE_SMALL,	SMALL_SHORT_CODE_SIZE
end_struct inflate_huff_code_small

// inflate_state
start_struct inflate_state
	//	 name	size	align
	field _next_out,	8,	8
	field _avail_out,	4,	4
	field _total_out,	4,	4
	field _next_in,	8,	8
	field _read_in,	8,	8
	field _avail_in,	4,	4
	field _read_in_length,	4,	4
	field _lit_huff_code,	_inflate_huff_code_large_size,	_inflate_huff_code_large_align
	field _dist_huff_code,	_inflate_huff_code_small_size,	_inflate_huff_code_small_align
	field _block_state,	4,	4
	field _dict_length,	4,	4
	field _bfinal,	4,	4
	field _crc_flag,	4,	4
	field _crc,	4,	4
	field _hist_bits,	4,	4
	field _type0_block_len,	4,	4
	field _write_overflow_lits,	4,	4
	field _write_overflow_len,	4,	4
	field _copy_overflow_len,	4,	4
	field _copy_overflow_dist,	4,	4
end_struct inflate_state

.set _lit_huff_code_short_code_lookup , _lit_huff_code+_short_code_lookup_large
.set _lit_huff_code_long_code_lookup , _lit_huff_code+_long_code_lookup_large
.set _dist_huff_code_short_code_lookup , _dist_huff_code+_short_code_lookup_small
.set _dist_huff_code_long_code_lookup , _dist_huff_code+_long_code_lookup_small
.set ISAL_BLOCK_NEW_HDR , 0
.set ISAL_BLOCK_HDR , 1
.set ISAL_BLOCK_TYPE0 , 2
.set ISAL_BLOCK_CODED , 3
.set ISAL_BLOCK_INPUT_DONE , 4
.set ISAL_BLOCK_FINISH , 5

/* Inflate Return values */
#define ISAL_DECOMP_OK 0	/* No errors encountered while decompressing */
#define ISAL_END_INPUT 1	/* End of input reached */
#define ISAL_OUT_OVERFLOW 2	/* End of output reached */
#define ISAL_NAME_OVERFLOW 3	/* End of gzip name buffer reached */
#define ISAL_COMMENT_OVERFLOW 4	/* End of gzip name buffer reached */
#define ISAL_EXTRA_OVERFLOW 5	/* End of extra buffer reached */
#define ISAL_NEED_DICT 6 /* Stream needs a dictionary to continue */
#define ISAL_INVALID_BLOCK -1	/* Invalid deflate block found */
#define ISAL_INVALID_SYMBOL -2	/* Invalid deflate symbol found */
#define ISAL_INVALID_LOOKBACK -3	/* Invalid lookback distance found */
#define ISAL_INVALID_WRAPPER -4 /* Invalid gzip/zlib wrapper found */
#define ISAL_UNSUPPORTED_METHOD -5	/* Gzip/zlib wrapper specifies unsupported compress method */
#define ISAL_INCORRECT_CHECKSUM -6 /* Incorrect checksum found */


#define ISAL_DEF_MAX_CODE_LEN 15
#define LARGE_SHORT_SYM_LEN 25
#define LARGE_SHORT_SYM_MASK ((1 << LARGE_SHORT_SYM_LEN) - 1)
#define LARGE_LONG_SYM_LEN 10
#define LARGE_LONG_SYM_MASK ((1 << LARGE_LONG_SYM_LEN) - 1)
#define LARGE_SHORT_CODE_LEN_OFFSET 28
#define LARGE_LONG_CODE_LEN_OFFSET 10
#define LARGE_FLAG_BIT_OFFSET 25
#define LARGE_FLAG_BIT (1 << LARGE_FLAG_BIT_OFFSET)
#define LARGE_SYM_COUNT_OFFSET 26
#define LARGE_SYM_COUNT_LEN 2
#define LARGE_SYM_COUNT_MASK ((1 << LARGE_SYM_COUNT_LEN) - 1)
#define LARGE_SHORT_MAX_LEN_OFFSET 26

/*
	A dist code entry holds the length of the code plus its extra bits in
	bits 4:0, the symbol in bits 10:6 and the length of the code in bits
	14:11. An entry with a code length of 0 is either invalid, holding the
	length of its code in bits 4:0, or has bit SMALL_FLAG_BIT_OFFSET set and
	points to a subtable for longer codes, with the longest length in bits
	10:7 and the offset in bits 6:0.
*/
#define DIST_TOTAL_LEN_MASK 0x1f
#define DIST_SYM_OFFSET 6
#define DIST_SYM_LEN 5
#define DIST_CODE_LEN_OFFSET 11
#define DIST_CODE_LEN_LEN 4
#define SMALL_FLAG_BIT_OFFSET 15
#define SMALL_MAX_LEN_OFFSET 7
#define SMALL_MAX_LEN_LEN 4
#define SMALL_OFFSET_MASK 0x7f

#define MAX_LIT_LEN_CODE_LEN 21
#define MAX_LIT_LEN_COUNT (MAX_LIT_LEN_CODE_LEN + 2)
#define MAX_LIT_LEN_SYM 512
#define LIT_LEN_ELEMS 514

#define INVALID_SYMBOL 0x1FFF
#define INVALID_CODE 0xFFFFFF

#define MIN_DEF_MATCH 3

#define TRIPLE_SYM_FLAG 0
#define DOUBLE_SYM_FLAG TRIPLE_SYM_FLAG + 1
#define SINGLE_SYM_FLAG DOUBLE_SYM_FLAG + 1
#define DEFAULT_SYM_FLAG TRIPLE_SYM_FLAG

#define SINGLE_SYM_THRESH (2 * 1024)
#define DOUBLE_SYM_THRESH (4 * 1024)


/*
declare Macros
*/

.macro	declare_generic_reg name:req,reg:req,default:req
	\name		.req	\default\reg
	w_\name		.req	w\reg
	x_\name		.req	x\reg
.endm


.macro	inflate_in_load_read_byte
	cmp	read_in_length,56
	bgt	1f
	cbz	avail_in,1f
	ldrb	w_temp,[next_in],1
	sub	avail_in,avail_in,1
	lsl	temp,temp,x_read_in_length
	orr	read_in,read_in,temp
	add	read_in_length,read_in_length,8
	uxtw	x_read_in_length,read_in_length

.endm

.macro	inflate_in_load

	cmp	read_in_length, 63
	bgt	1f

	/*if (state->avail_in >= 8) */
	cmp	avail_in, 7
	bhi	2f

	// loop max 7 times
	// while (state->read_in_length < 57 && state->avail_in > 0)
	inflate_in_load_read_byte
	inflate_in_load_read_byte
	inflate_in_load_read_byte
	inflate_in_load_read_byte
	inflate_in_load_read_byte
	inflate_in_load_read_byte
	inflate_in_load_read_byte
	b 1f
2:
	add	new_bytes,read_in_length,7
	mov	w_temp,8
	lsr	new_bytes,new_bytes,3
	sub	new_bytes,w_temp,new_bytes
	ldr	temp,[next_in]
	lsl	temp,temp,x_read_in_length
	orr	read_in,read_in,temp
	add	next_in,next_in,new_bytes,uxtb
	add	read_in_length,read_in_length,new_bytes,lsl 3
	sub	avail_in,avail_in,new_bytes

1:
.endm

.macro copy_word
	sub	repeat_length,repeat_length,#4
	ldr	w_arg0, [arg1],4
	cmp	repeat_length, 3
	str	w_arg0, [next_out],4
	bls	load_byte_less_than_4
.endm


	.global	cdecl(decode_huffman_code_block_stateless_aarch64)
#ifndef __APPLE__
	.type	decode_huffman_code_block_stateless_aarch64, %function
#endif
/*
	void decode_huffman_code_block_stateless_aarch64(
				struct inflate_state *state,
				uint8_t * start_out)
*/
	declare_generic_reg	arg0,		0, x
	declare_generic_reg	arg1,		1, x
	declare_generic_reg	arg2,		2, x

	declare_generic_reg	state,		11,x
	declare_generic_reg	start_out,	29,x

	declare_generic_reg	read_in,	3,x
	declare_generic_reg	read_in_length,	4,w
	declare_generic_reg	sym_count,	5,w
	declare_generic_reg	next_bits,	6,w
	declare_generic_reg	next_lits,	6,w
	declare_generic_reg	avail_in,	20,w
	declare_generic_reg	next_in,	23,x

	declare_generic_reg	temp,		16,x	//local temp variable
	declare_generic_reg	new_bytes,	7,w	//temp variable
	declare_generic_reg	copy_overflow_length,		28,w



	declare_generic_reg	block_state,	8,w
	declare_generic_reg	block_state_adr,9,x
	declare_generic_reg	look_back_dist,	10,w
	declare_generic_reg	bfinal,		22,w

	declare_generic_reg	next_out,	12,x
	declare_generic_reg	avail_out,	13,w
	declare_generic_reg	total_out,	14,w

	declare_generic_reg	rfc_table,	15,x
	declare_generic_reg	next_sym,	17,w
	declare_generic_reg	next_dist,	17,w
	declare_generic_reg	bit_count,	19,w

	declare_generic_reg	bit_mask,	21,w
	declare_generic_reg	next_lit,	24,w
	declare_generic_reg	write_overflow_len,25,w
	declare_generic_reg	write_overflow_lits,26,w
	declare_generic_reg	repeat_length,27,w

/*
	Fast path registers. The fast path keeps the whole decode state in
	registers. It runs only while the input has at least FAST_IN_SLOP bytes
	left and the output has at least FAST_OUT_SLOP bytes of space, so it needs
	no per-symbol bounds checks. On exit, it recomputes avail_in, avail_out,
	and total_out from the pointers.
*/
	declare_generic_reg	f_lit_tbl,	0, x
	declare_generic_reg	f_dist_tbl,	1, x
	declare_generic_reg	f_aux_tbl,	2, x
	declare_generic_reg	f_entry,	5, w
	declare_generic_reg	f_next_entry,	6, w
	declare_generic_reg	f_cnt,		7, w
	declare_generic_reg	f_dist,		10,w
	declare_generic_reg	f_tmp0,		16,x
	declare_generic_reg	f_last,		17,w
	declare_generic_reg	f_sym_cnt,	19,w
	declare_generic_reg	f_tmp1,		20,x
	declare_generic_reg	f_len,		21,w
	declare_generic_reg	f_end_in,	24,x
	declare_generic_reg	f_end_out,	27,x
	declare_generic_reg	f_tmp2,		30,x

/*
	A refill loads 8 bytes. One fast loop iteration writes at most 2 literals,
	a 258-byte match, and 31 bytes of over-copy past next_out. Closer than
	FAST_OUT_SLOP to the end of the output, the loop checks each symbol before
	decoding it, and it stops FAST_OUT_MIN bytes from the end.
*/
#define FAST_IN_SLOP	8
#define FAST_OUT_SLOP	320
#define FAST_OUT_MIN	64

#define AUX_DIST_START	0
#define AUX_PATTERN_IDX	64
#define AUX_PATTERN_STEP	(AUX_PATTERN_IDX + 16 * 16)

/*
	Refill read_in to at least 56 valid bits by loading 8 bytes. Requires
	read_in_length < 64 and at least 8 readable bytes at next_in. Bits of
	read_in above read_in_length are either zero or the stream bits that
	belong there, so OR-ing in the new bytes is always correct.
*/
.macro fast_refill
	ldr	f_tmp0, [next_in]
	eor	w_f_tmp1, read_in_length, 63
	lsl	f_tmp0, f_tmp0, x_read_in_length
	lsr	w_f_tmp1, w_f_tmp1, 3
	orr	read_in, read_in, f_tmp0
	add	next_in, next_in, f_tmp1
	orr	read_in_length, read_in_length, 56
.endm

/* Recompute avail_in, avail_out, and total_out from the fast path pointers */
.macro fast_sync_state
	add	f_tmp0, f_end_out, FAST_OUT_SLOP
	sub	f_tmp0, f_tmp0, next_out
	sub	w_f_tmp2, avail_out, w_f_tmp0
	add	total_out, total_out, w_f_tmp2
	mov	avail_out, w_f_tmp0
	add	f_tmp0, f_end_in, FAST_IN_SLOP
	sub	x_avail_in, f_tmp0, next_in
.endm

cdecl(decode_huffman_code_block_stateless_aarch64):
	//save registers
	push_stack

	//load variables
	mov	state,arg0
	mov	block_state,_block_state
	mov	start_out,arg1
	add	block_state_adr,state,block_state,uxtw
	ldr	block_state,	[block_state_adr]
	ldr	bfinal,		[block_state_adr,_bfinal-_block_state]

	ldr	next_out, [state]
	ldp	avail_out,total_out,[state,_avail_out]
	ldp	next_in,  read_in,  [state,_next_in]
	ldp	avail_in, read_in_length, [state,_avail_in]
	ldp	write_overflow_lits,write_overflow_len,[block_state_adr,_write_overflow_lits-_block_state]

	//init rfc_table
#ifndef __APPLE__
	adrp	rfc_table,rfc_lookup_table
	add	rfc_table,rfc_table,:lo12:rfc_lookup_table
#else
	adrp	rfc_table,rfc_lookup_table@PAGE
	add	rfc_table,rfc_table,rfc_lookup_table@PAGEOFF
#endif
#if ENABLE_TBL_INSTRUCTION
	ld1	{v1.16b,v2.16b,v3.16b},[rfc_table]
	add	rfc_table,rfc_table,48
	ld1	{v4.16b-v7.16b},[rfc_table]
#endif

	/*
		state->copy_overflow_length = 0;
		state->copy_overflow_distance = 0;
	*/
	mov	x_copy_overflow_length,xzr
	str	xzr,[block_state_adr,_copy_overflow_len-_block_state]

	/* Use the fast path while far enough from the ends of both buffers */
	cmp	block_state, ISAL_BLOCK_CODED
	bne	block_state_loop
	cmp	avail_in, FAST_IN_SLOP
	blo	block_state_loop
	cmp	avail_out, FAST_OUT_MIN
	blo	block_state_loop

	add	f_end_in, next_in, x_avail_in
	sub	f_end_in, f_end_in, FAST_IN_SLOP
	add	f_end_out, next_out, x_avail_out
	sub	f_end_out, f_end_out, FAST_OUT_SLOP
	add	f_lit_tbl, state, _lit_huff_code
	mov	f_dist_tbl, _dist_huff_code
	add	f_dist_tbl, state, f_dist_tbl
	adr	f_aux_tbl, fast_aux_table

	cmp	read_in_length, 56
	bhs	1f
	fast_refill
1:
	and	f_tmp0, read_in, (1 << ISAL_DECODE_LONG_BITS) - 1
	ldr	f_entry, [f_lit_tbl, f_tmp0, lsl 2]
	b	fast_loop

	/* while (state->block_state == ISAL_BLOCK_CODED) */
block_state_loop:
	cmp	block_state ,ISAL_BLOCK_CODED
	bne	exit_func_success

	inflate_in_load

	/* save state here  */
	str	next_out, [state]
	stp	avail_out,total_out,[state,_avail_out]
	stp	next_in,  read_in,  [state,_next_in]
	stp	avail_in, read_in_length, [state,_avail_in]
	stp	write_overflow_lits,write_overflow_len,[block_state_adr,_write_overflow_lits-_block_state]

	/*
	decode_next_lit_len(&next_lits, &sym_count,
				state, &state->lit_huff_code,
				&temp_dat, &temp_bytes);
	*/
	cmp	read_in_length,ISAL_DEF_MAX_CODE_LEN
	ble	inflate_in_load_decode
decode_next_lit_len_start:
	and	x_next_bits,read_in,((1 << ISAL_DECODE_LONG_BITS) - 1)
	/*next_sym = huff_code->short_code_lookup[next_bits];*/
	add	next_bits,next_bits,_lit_huff_code>>2
	ldr	next_sym,[state,x_next_bits,lsl 2]
	/*if ((next_sym & LARGE_FLAG_BIT) == 0) {*/
	tbnz	next_sym,LARGE_FLAG_BIT_OFFSET,long_code_lookup_routine
	lsr	bit_count,next_sym,LARGE_SHORT_CODE_LEN_OFFSET
	sub	read_in_length,read_in_length,bit_count
	lsr	read_in,read_in,x_bit_count
	mov	temp,0x1fff
	cmp	bit_count,0
	csel	next_sym,next_sym,w_temp,ne
	ubfx	sym_count,next_sym,LARGE_SYM_COUNT_OFFSET,LARGE_SYM_COUNT_LEN
	and	next_lits,next_sym,LARGE_SHORT_SYM_MASK
	b	decode_next_lit_len_end
long_code_lookup_routine:
	lsr	bit_mask,next_sym,LARGE_SHORT_MAX_LEN_OFFSET
	mov	sym_count,1
	and	next_sym,next_sym,LARGE_SHORT_SYM_MASK
	mov	temp,1023
	lsl	bit_mask,sym_count,bit_mask
	sub	bit_mask,bit_mask,1
	and	x_next_bits,read_in,x_bit_mask
	add	next_bits,next_sym,next_bits,lsr ISAL_DECODE_LONG_BITS
	mov	next_sym,(_lit_huff_code+_long_code_lookup_large)>>1
	add	next_bits,next_bits,next_sym
	ldrh	next_sym,[state,x_next_bits,lsl 1]
	lsr	bit_count,next_sym,10
	sub	read_in_length,read_in_length,bit_count
	and	next_lits,next_sym,w_temp
	lsr	read_in,read_in,x_bit_count
	cmp	bit_count,0
	csel	next_lits,next_lits,w_temp,ne
decode_next_lit_len_end:

	/* if (sym_count == 0) */
	cbz	sym_count,invalid_symbol
	tbnz	read_in_length,31, end_input

	/* while (sym_count > 0) start */
sym_count_loop:
	and	next_lit,next_lits , 0xffff

	/*if (next_lit < 256 || sym_count > 1) {*/
	cmp	next_lit,255
	ccmp	sym_count,1,0,hi
	beq	next_lit_256

	/* if (state->avail_out < 1) { */
	cbnz	avail_out,sym_count_adjust

	mov	write_overflow_len,sym_count
	lsl	sym_count,sym_count,3
	mov	write_overflow_lits,next_lits
	sub	sym_count,sym_count,8
	lsr	next_lits,next_lits,sym_count
	mov	sym_count,1
	cmp	next_lits,255
	bls	isal_out_overflow
	cmp	next_lits,256
	sub	write_overflow_len,write_overflow_len,1
	beq	isal_out_overflow_1
	b	sym_count_loop

sym_count_adjust:
	/*
		while (sym_count > 0) end
		next_lits >>= 8;
		sym_count--;
	*/
	subs	sym_count,sym_count,1
	lsr	next_lits,next_lits,8
	strb	next_lit,[next_out],1
	sub	avail_out,avail_out,1
	add	total_out,total_out,1
	bne	sym_count_loop
	b	block_state_loop

next_lit_256:
	/* if (next_lit == 256) { */
	cmp	next_lit,256
	beq	next_lit_eq_256


	/*
	if (next_lit <= MAX_LIT_LEN_SYM)
		sym_count must be 1
	*/
	cmp	next_lit,MAX_LIT_LEN_SYM
	bhi	invalid_symbol
	sub	repeat_length,next_lit,254
	/*
	    next_dist =
	    decode_next_dist(state, &state->dist_huff_code, &temp_dat,
			     &temp_bytes);
	*/
	cmp	read_in_length,ISAL_DEF_MAX_CODE_LEN
	ble	inflate_in_load_decode_next_dist
decode_next_dist_start:
	and	x_next_bits,read_in,((1 << ISAL_DECODE_SHORT_BITS) - 1)
	mov	next_sym,_dist_huff_code>>1
	add	next_bits,next_bits,next_sym
	ldrh	next_sym, [state,x_next_bits,lsl 1]
	tbz	next_sym,SMALL_FLAG_BIT_OFFSET,decode_next_dist_flag
	ubfx	bit_mask,next_sym,SMALL_MAX_LEN_OFFSET,SMALL_MAX_LEN_LEN
	mov	temp,1
	and	next_sym,next_sym,SMALL_OFFSET_MASK
	lsl	bit_mask,w_temp,bit_mask
	sub	bit_mask,bit_mask,1
	and	x_next_bits,read_in,x_bit_mask
	add	next_bits,next_sym,next_bits,lsr ISAL_DECODE_SHORT_BITS
	mov	next_sym,(_dist_huff_code + _long_code_lookup_small)>>1
	add	next_bits,next_bits,next_sym
	ldrh	next_sym,[state,x_next_bits,lsl 1]
decode_next_dist_flag:
	lsr	bit_count,next_sym,DIST_CODE_LEN_OFFSET
decode_next_dist_adjust:
	sub	read_in_length,read_in_length,bit_count
	lsr	read_in,read_in,x_bit_count
	cbnz	bit_count,decode_next_dist_end
	sub	read_in_length,read_in_length,next_sym
	mov	next_sym,INVALID_SYMBOL
decode_next_dist_end:
	ubfx	next_sym,next_sym,DIST_SYM_OFFSET,DIST_SYM_LEN

	tbnz	read_in_length,31,end_input_1
	cmp	next_dist,29
	bhi	invalid_symbol


#if ENABLE_TBL_INSTRUCTION
	ins	v0.b[0],next_dist
	tbl	v0.8b,{v2.16b,v3.16b},v0.8b
	umov	bit_count,v0.b[0]
#else
	ldrb	bit_count,[rfc_table,next_dist,sxtw]
#endif

	/*inflate_in_read_bits(state,
		 dist_extra_bit_count, &temp_dat,
		 &temp_bytes);
	*/
	inflate_in_load
	mov	temp,1
	lsl	temp,temp,x_bit_count
	sub	read_in_length,read_in_length,bit_count
	sub	temp,temp,1
	and	x_look_back_dist,temp,read_in
	lsr	read_in,read_in,x_bit_count
#if ENABLE_TBL_INSTRUCTION
	dup	v0.8b,next_dist
	add	v0.8b,v1.8b,v0.8b
	tbl	v0.8b,{v4.16b-v7.16b},v0.8b
	umov	next_dist,v0.h[0]
#else
	add	next_dist,next_dist,16
	ldrh	next_dist,[rfc_table,x_next_dist,lsl 1]
#endif
	add	look_back_dist,look_back_dist,next_dist

	/*
		if (state->read_in_length < 0) {
	*/
	tbnz	read_in_length,31,end_input_1

	/*
	if (state->next_out - look_back_dist < start_out) {
	*/
	sub	temp,next_out,x_look_back_dist
	cmp	temp,start_out
	bcc	isal_invalid_lookback
	/*
		if (state->avail_out < repeat_length) {
	*/
	cmp	avail_out , repeat_length
	bcs	decompress_data_start
	sub	copy_overflow_length,repeat_length,avail_out
	stp	copy_overflow_length,look_back_dist,[block_state_adr,_copy_overflow_len-_block_state]
	mov	repeat_length,avail_out

decompress_data_start:
	add	total_out,total_out,repeat_length
	sub	avail_out,avail_out,repeat_length
	sub	arg1,next_out,x_look_back_dist
	#if 1
	cmp	look_back_dist,repeat_length
	bls	byte_copy_start
	#else
	b	byte_copy_start
	#endif


	cbz	repeat_length,decompress_data_end
	cmp     repeat_length, 3
	bls     load_byte_less_than_4 //0.5% will jump
load_byte_4:
	sub     repeat_length, repeat_length, #4
	ldr     w_arg0, [arg1],4
	cmp	repeat_length, 3
	str     w_arg0, [next_out],4
	bls     load_byte_less_than_4
	.rept	62
	copy_word
	.endr
	sub     repeat_length, repeat_length, #4
	ldr     w_arg0, [arg1],4
	cmp	repeat_length, 4
	str     w_arg0, [next_out],4
	bge	load_byte_4
load_byte_less_than_4:
	tbz	repeat_length,0,load_byte_2
	ldrb    w_arg0, [arg1],1
	sub     repeat_length, repeat_length, #1
	strb    w_arg0, [next_out],1
load_byte_2:
	tbz	repeat_length,1,decompress_data_end
	ldrh     w_arg0, [arg1],2
	strh     w_arg0, [next_out],2
decompress_data_end:



	/*
	if (state->copy_overflow_length > 0)
	*/
	cmp	copy_overflow_length,0
	bgt	isal_out_overflow
	b	block_state_loop
next_lit_eq_256:
	/*
		state->block_state = state->bfinal ?
				ISAL_BLOCK_INPUT_DONE : ISAL_BLOCK_NEW_HDR;
	*/
	mov	block_state, ISAL_BLOCK_INPUT_DONE
	cmp	bfinal,0
	csel	block_state, block_state, bfinal, ne
	str	block_state, [block_state_adr]

	b	block_state_loop
exit_func_success:
	mov	w0 , 0
exit_func:
	str	next_out, [state]
	stp	avail_out,total_out,[state,_avail_out]
	stp	next_in,  read_in,  [state,_next_in]
	stp	avail_in, read_in_length, [state,_avail_in]
	stp	write_overflow_lits,write_overflow_len,[block_state_adr,_write_overflow_lits-_block_state]

	pop_stack
	ret
end_input_1:
end_input:
	mov	w0,ISAL_END_INPUT
	pop_stack
	ret

invalid_symbol:
	/*
		below variable was changed
	*/
	str	next_out, [state]
	stp	avail_out,total_out,[state,_avail_out]
	stp	next_in,  read_in,  [state,_next_in]
	stp	avail_in, read_in_length, [state,_avail_in]
	stp	write_overflow_lits,write_overflow_len,[block_state_adr,_write_overflow_lits-_block_state]
	mov	w0,	ISAL_INVALID_SYMBOL
	b exit_func
isal_out_overflow_1:

	cmp	bfinal,0
	mov	block_state, ISAL_BLOCK_INPUT_DONE
	csel	block_state, block_state, wzr, ne
	str	block_state, [block_state_adr]
isal_out_overflow:
	mov	w0, ISAL_OUT_OVERFLOW

	b	exit_func
isal_invalid_lookback:
	mov	w0, ISAL_INVALID_LOOKBACK
	b	exit_func
inflate_in_load_decode:
	inflate_in_load
	b	decode_next_lit_len_start
inflate_in_load_decode_next_dist:
	inflate_in_load
	b	decode_next_dist_start
byte_copy_start:
	add	arg2,next_out,x_repeat_length
	cmp	arg2, next_out
	beq	decompress_data_end
	sub	arg2,arg2,1
byte_copy_loop:
	ldrb	w_arg0, [arg1] , 1
	cmp	arg2, next_out
	strb	w_arg0, [next_out],1
	bne	byte_copy_loop
	b	decompress_data_end

/*
	Fast path main loop.

	At the start of each iteration, f_entry holds the lit/len table entry for
	read_in, and read_in holds at least 28 valid bits, enough for the longest
	lit/len code (21 bits with length extra bits). After consuming the lit/len
	code, the loop refills read_in to at least 56 bits, enough for the longest
	distance code with extra bits (28 bits).
*/
	.balign	64
fast_loop:
	cmp	next_in, f_end_in
	ccmp	next_out, f_end_out, 2, ls
	bhi	fast_near_end
fast_loop_body:
	tbnz	f_entry, LARGE_FLAG_BIT_OFFSET, fast_long_lit
	lsr	f_cnt, f_entry, LARGE_SHORT_CODE_LEN_OFFSET
	ubfx	f_sym_cnt, f_entry, LARGE_SYM_COUNT_OFFSET, LARGE_SYM_COUNT_LEN
	/* Invalid symbol: the slow path reports it */
	cbz	f_cnt, fast_exit
fast_lit_decoded:
	lsr	read_in, read_in, x_f_cnt
	sub	read_in_length, read_in_length, f_cnt
	/* Speculatively write all packed symbols as literals */
	str	f_entry, [next_out]
	/* Look up the next symbol before the refill to shorten the dependency chain */
	and	f_tmp0, read_in, (1 << ISAL_DECODE_LONG_BITS) - 1
	ldr	f_next_entry, [f_lit_tbl, f_tmp0, lsl 2]
	/*
		If the last symbol is a length, the distance code follows it. Look
		it up now, so a mispredicted length does not wait for the load.
	*/
	and	f_tmp0, read_in, (1 << ISAL_DECODE_SHORT_BITS) - 1
	ldrh	f_dist, [f_dist_tbl, f_tmp0, lsl 1]
	fast_refill
	/*
		Only the last packed symbol can be a length or end of block, and
		then bit 8 or 9 of it is set. Those bits land at bits 0 and 1 of
		f_entry >> (8 * f_sym_cnt).
	*/
	lsl	f_last, f_sym_cnt, 3
	lsr	f_last, f_entry, f_last
	tst	f_last, 3
	bne	fast_len_or_eob
	add	next_out, next_out, x_f_sym_cnt
	mov	f_entry, f_next_entry
	b	fast_loop

fast_len_or_eob:
	/* Keep the literals that precede the length or end of block symbol */
	sub	f_sym_cnt, f_sym_cnt, 1
	add	next_out, next_out, x_f_sym_cnt
	lsl	f_last, f_sym_cnt, 3
	lsr	f_last, f_entry, f_last
	and	f_last, f_last, 0x3ff
	cmp	f_last, 256
	beq	fast_eob
	sub	f_len, f_last, 254

	/* Decode the distance symbol */
	tbnz	f_dist, SMALL_FLAG_BIT_OFFSET, fast_long_dist
fast_dist_decoded:
	ubfx	f_cnt, f_dist, DIST_CODE_LEN_OFFSET, DIST_CODE_LEN_LEN
	cbz	f_cnt, fast_invalid_dist

	/*
		Consume the code and its extra bits with one shift, read the extra
		bits and add the distance base
	*/
	and	f_last, f_dist, DIST_TOTAL_LEN_MASK
	ubfx	f_dist, f_dist, DIST_SYM_OFFSET, DIST_SYM_LEN
	ldrh	f_dist, [f_aux_tbl, x_f_dist, lsl 1]
	mov	f_tmp1, -1
	lsl	f_tmp1, f_tmp1, x_f_last
	bic	f_tmp1, read_in, f_tmp1
	lsr	f_tmp1, f_tmp1, x_f_cnt
	lsr	read_in, read_in, x_f_last
	sub	read_in_length, read_in_length, f_last
	add	f_dist, f_dist, w_f_tmp1

	/* Look up the next symbol before copying the match */
	and	f_tmp0, read_in, (1 << ISAL_DECODE_LONG_BITS) - 1
	ldr	f_entry, [f_lit_tbl, f_tmp0, lsl 2]

	sub	f_tmp0, next_out, x_f_dist
	cmp	f_tmp0, start_out
	blo	fast_invalid_lookback

	cmp	f_dist, 16
	blo	fast_copy_short_dist

	/* Distance is at least 16, so 16-byte chunks never overlap the source */
	ldr	q16, [f_tmp0]
	str	q16, [next_out]
	cmp	f_len, 16
	bhi	fast_copy_long
	add	next_out, next_out, x_f_len
	b	fast_loop

fast_copy_long:
	add	f_tmp1, next_out, x_f_len
	add	next_out, next_out, 16
	add	f_tmp0, f_tmp0, 16
	cmp	f_dist, 32
	blo	fast_copy_16
fast_copy_32:
	ldp	q16, q17, [f_tmp0], 32
	stp	q16, q17, [next_out], 32
	cmp	next_out, f_tmp1
	blo	fast_copy_32
	mov	next_out, f_tmp1
	b	fast_loop
fast_copy_16:
	ldr	q16, [f_tmp0], 16
	str	q16, [next_out], 16
	cmp	next_out, f_tmp1
	blo	fast_copy_16
	mov	next_out, f_tmp1
	b	fast_loop

	/*
		Distance is less than 16. Replicate the first f_dist bytes of the
		source across a vector, then store the vector at steps of the
		largest multiple of f_dist that fits in 16 bytes.
	*/
fast_copy_short_dist:
	ldr	q16, [f_tmp0]
	add	f_tmp2, f_aux_tbl, AUX_PATTERN_IDX
	ldr	q17, [f_tmp2, x_f_dist, lsl 4]
	add	f_tmp2, f_aux_tbl, AUX_PATTERN_STEP
	ldrb	w_f_tmp2, [f_tmp2, x_f_dist]
	tbl	v16.16b, {v16.16b}, v17.16b
	add	f_tmp1, next_out, x_f_len
1:
	str	q16, [next_out]
	add	next_out, next_out, f_tmp2
	cmp	next_out, f_tmp1
	blo	1b
	mov	next_out, f_tmp1
	b	fast_loop

	/* The lit/len code is longer than ISAL_DECODE_LONG_BITS */
fast_long_lit:
	lsr	f_cnt, f_entry, LARGE_SHORT_MAX_LEN_OFFSET
	mov	f_tmp1, -1
	lsl	f_tmp1, f_tmp1, x_f_cnt
	bic	f_tmp1, read_in, f_tmp1
	and	f_entry, f_entry, LARGE_SHORT_SYM_MASK
	add	f_entry, f_entry, w_f_tmp1, lsr ISAL_DECODE_LONG_BITS
	add	f_tmp0, f_lit_tbl, _long_code_lookup_large
	ldrh	f_entry, [f_tmp0, x_f_entry, lsl 1]
	lsr	f_cnt, f_entry, LARGE_LONG_CODE_LEN_OFFSET
	and	f_entry, f_entry, LARGE_LONG_SYM_MASK
	mov	f_sym_cnt, 1
	cbnz	f_cnt, fast_lit_decoded
	/* Invalid symbol: the slow path reports it */
	b	fast_exit

	/* The distance code is longer than ISAL_DECODE_SHORT_BITS */
fast_long_dist:
	ubfx	f_cnt, f_dist, SMALL_MAX_LEN_OFFSET, SMALL_MAX_LEN_LEN
	mov	f_tmp1, -1
	lsl	f_tmp1, f_tmp1, x_f_cnt
	bic	f_tmp1, read_in, f_tmp1
	and	f_dist, f_dist, SMALL_OFFSET_MASK
	add	f_dist, f_dist, w_f_tmp1, lsr ISAL_DECODE_SHORT_BITS
	add	f_tmp0, f_dist_tbl, _long_code_lookup_small
	ldrh	f_dist, [f_tmp0, x_f_dist, lsl 1]
	b	fast_dist_decoded

	/*
		Near the end of the output, decode the next symbol only if its
		output fits. Literals write 4 bytes, and a match of length len
		after up to 2 literals writes at most len + 33 bytes. Leave long
		codes to the slow path.
	*/
fast_near_end:
	cmp	next_in, f_end_in
	bhi	fast_exit
	add	f_tmp1, f_end_out, FAST_OUT_SLOP
	sub	f_tmp1, f_tmp1, next_out
	cmp	f_tmp1, FAST_OUT_MIN
	blo	fast_exit
	tbnz	f_entry, LARGE_FLAG_BIT_OFFSET, fast_exit
	ubfx	f_sym_cnt, f_entry, LARGE_SYM_COUNT_OFFSET, LARGE_SYM_COUNT_LEN
	lsl	f_last, f_sym_cnt, 3
	lsr	f_last, f_entry, f_last
	tst	f_last, 3
	beq	fast_loop_body
	sub	f_last, f_sym_cnt, 1
	lsl	f_last, f_last, 3
	lsr	f_last, f_entry, f_last
	and	f_last, f_last, 0x3ff
	sub	f_last, f_last, 254 - 33
	cmp	x_f_last, f_tmp1
	bhi	fast_exit
	b	fast_loop_body

fast_eob:
	mov	block_state, ISAL_BLOCK_INPUT_DONE
	cmp	bfinal, 0
	csel	block_state, block_state, bfinal, ne
	str	block_state, [block_state_adr]
fast_exit:
	fast_sync_state
	b	block_state_loop

	/*
		An invalid distance entry holds the code length. Consume it as the
		slow path does, so both paths leave the same state.
	*/
fast_invalid_dist:
	sub	read_in_length, read_in_length, f_dist
fast_invalid_symbol:
	fast_sync_state
	b	invalid_symbol

fast_invalid_lookback:
	fast_sync_state
	b	isal_invalid_lookback

#ifndef __APPLE__
	.size	decode_huffman_code_block_stateless_aarch64, .-decode_huffman_code_block_stateless_aarch64
	.type	rfc_lookup_table, %object
#endif

rfc_lookup_table:
#if ENABLE_TBL_INSTRUCTION
	.byte	0x00, 0x20, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
	.zero	8
#endif
	//dist_extra_bit_count
	.byte	0x00, 0x00, 0x00, 0x00, 0x01, 0x01, 0x02, 0x02
	.byte	0x03, 0x03, 0x04, 0x04, 0x05, 0x05, 0x06, 0x06
	.byte	0x07, 0x07, 0x08, 0x08, 0x09, 0x09, 0x0a, 0x0a
	.byte	0x0b, 0x0b, 0x0c, 0x0c, 0x0d, 0x0d, 0x00, 0x00
	//dist_start
#if ENABLE_TBL_INSTRUCTION
	.byte 0x01,0x02,0x03,0x04,0x05,0x07,0x09,0x0d,0x11,0x19,0x21,0x31,0x41,0x61,0x81,0xc1
	.byte 0x01,0x81,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x00,0x00
	.byte 0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00
	.byte 0x01,0x01,0x02,0x03,0x04,0x06,0x08,0x0c,0x10,0x18,0x20,0x30,0x40,0x60,0x00,0x00
#else
	.short	0x0001, 0x0002, 0x0003, 0x0004, 0x0005, 0x0007, 0x0009, 0x000d
	.short	0x0011, 0x0019, 0x0021, 0x0031, 0x0041, 0x0061, 0x0081, 0x00c1
	.short	0x0101, 0x0181, 0x0201, 0x0301, 0x0401, 0x0601, 0x0801, 0x0c01
	.short	0x1001, 0x1801, 0x2001, 0x3001, 0x4001, 0x6001, 0x0000, 0x0000
#endif
#ifndef __APPLE__
	.size	rfc_lookup_table, . - rfc_lookup_table
	.type	fast_aux_table, %object
#endif

	.balign	16
fast_aux_table:
	/* dist_start: base distance for each distance symbol */
	.short	0x0001, 0x0002, 0x0003, 0x0004, 0x0005, 0x0007, 0x0009, 0x000d
	.short	0x0011, 0x0019, 0x0021, 0x0031, 0x0041, 0x0061, 0x0081, 0x00c1
	.short	0x0101, 0x0181, 0x0201, 0x0301, 0x0401, 0x0601, 0x0801, 0x0c01
	.short	0x1001, 0x1801, 0x2001, 0x3001, 0x4001, 0x6001, 0x0000, 0x0000
	/* pattern_idx: byte i of row d is i % d, to replicate a d-byte pattern */
	.byte	 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
	.byte	 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
	.byte	 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1
	.byte	 0, 1, 2, 0, 1, 2, 0, 1, 2, 0, 1, 2, 0, 1, 2, 0
	.byte	 0, 1, 2, 3, 0, 1, 2, 3, 0, 1, 2, 3, 0, 1, 2, 3
	.byte	 0, 1, 2, 3, 4, 0, 1, 2, 3, 4, 0, 1, 2, 3, 4, 0
	.byte	 0, 1, 2, 3, 4, 5, 0, 1, 2, 3, 4, 5, 0, 1, 2, 3
	.byte	 0, 1, 2, 3, 4, 5, 6, 0, 1, 2, 3, 4, 5, 6, 0, 1
	.byte	 0, 1, 2, 3, 4, 5, 6, 7, 0, 1, 2, 3, 4, 5, 6, 7
	.byte	 0, 1, 2, 3, 4, 5, 6, 7, 8, 0, 1, 2, 3, 4, 5, 6
	.byte	 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 0, 1, 2, 3, 4, 5
	.byte	 0, 1, 2, 3, 4, 5, 6, 7, 8, 9,10, 0, 1, 2, 3, 4
	.byte	 0, 1, 2, 3, 4, 5, 6, 7, 8, 9,10,11, 0, 1, 2, 3
	.byte	 0, 1, 2, 3, 4, 5, 6, 7, 8, 9,10,11,12, 0, 1, 2
	.byte	 0, 1, 2, 3, 4, 5, 6, 7, 8, 9,10,11,12,13, 0, 1
	.byte	 0, 1, 2, 3, 4, 5, 6, 7, 8, 9,10,11,12,13,14, 0
	/* pattern_step: largest multiple of d that is at most 16 */
	.byte	16,16,16,15,16,15,12,14,16, 9,10,11,12,13,14,15
#ifndef __APPLE__
	.size	fast_aux_table, . - fast_aux_table
#endif
