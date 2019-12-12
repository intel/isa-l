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
	.arch armv8-a
	.text
	.align	2
#include "lz0a_const_aarch64.h"
#include "huffman_aarch64.h"
#include "bitbuf2_aarch64.h"
#include "stdmac_aarch64.h"

#define ENABLE_TBL_INSTRUCTION	1

#define FIELD(name,size,align) \
	.set _FIELD_OFFSET,(_FIELD_OFFSET + (align) - 1) & (~ ((align)-1)); \
	.equ name,_FIELD_OFFSET ;					\
	.set _FIELD_OFFSET,_FIELD_OFFSET + size;			\
	.if align > _STRUCT_ALIGN;					\
		.set _STRUCT_ALIGN, align;				\
	.endif;

#define START_STRUCT(name) .set _FIELD_OFFSET,0;.set _STRUCT_ALIGN,0;

#define END_STRUCT(name)    .set _##name##_size , _FIELD_OFFSET;\
			    .set _##name##_align,_STRUCT_ALIGN

#define CONST(name,value)  .equ name,value

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
START_STRUCT( inflate_huff_code_large )
	//	 name	size	align
	FIELD ( _short_code_lookup_large,	LARGE_SHORT_CODE_SIZE*(1<<(ISAL_DECODE_LONG_BITS)),	LARGE_LONG_CODE_SIZE )
	FIELD ( _long_code_lookup_large,	LARGE_LONG_CODE_SIZE*MAX_LONG_CODE_LARGE,	LARGE_SHORT_CODE_SIZE )
END_STRUCT(inflate_huff_code_large)

// inflate_huff_code
START_STRUCT( inflate_huff_code_small )
	//	 name	size	align
	FIELD ( _short_code_lookup_small,	SMALL_SHORT_CODE_SIZE*(1<<(ISAL_DECODE_SHORT_BITS)),	SMALL_LONG_CODE_SIZE )
	FIELD ( _long_code_lookup_small,	SMALL_LONG_CODE_SIZE*MAX_LONG_CODE_SMALL,	SMALL_SHORT_CODE_SIZE )
END_STRUCT(inflate_huff_code_small)


// inflate_state
START_STRUCT( inflate_state )
	//	 name	size	align
	FIELD ( _next_out,	8,	8 )
	FIELD ( _avail_out,	4,	4 )
	FIELD ( _total_out,	4,	4 )
	FIELD ( _next_in,	8,	8 )
	FIELD ( _read_in,	8,	8 )
	FIELD ( _avail_in,	4,	4 )
	FIELD ( _read_in_length,	4,	4 )
	FIELD ( _lit_huff_code,	_inflate_huff_code_large_size,	_inflate_huff_code_large_align )
	FIELD ( _dist_huff_code,	_inflate_huff_code_small_size,	_inflate_huff_code_small_align )
	FIELD ( _block_state,	4,	4 )
	FIELD ( _dict_length,	4,	4 )
	FIELD ( _bfinal,	4,	4 )
	FIELD ( _crc_flag,	4,	4 )
	FIELD ( _crc,	4,	4 )
	FIELD ( _hist_bits,	4,	4 )
	FIELD ( _type0_block_len,	4,	4 )
	FIELD ( _write_overflow_lits,	4,	4 )
	FIELD ( _write_overflow_len,	4,	4 )
	FIELD ( _copy_overflow_len,	4,	4 )
	FIELD ( _copy_overflow_dist,	4,	4 )
END_STRUCT(inflate_state)

CONST( _lit_huff_code_short_code_lookup , _lit_huff_code+_short_code_lookup_large )
CONST( _lit_huff_code_long_code_lookup , _lit_huff_code+_long_code_lookup_large )
CONST( _dist_huff_code_short_code_lookup , _dist_huff_code+_short_code_lookup_small )
CONST( _dist_huff_code_long_code_lookup , _dist_huff_code+_long_code_lookup_small )
CONST( ISAL_BLOCK_NEW_HDR , 0 )
CONST( ISAL_BLOCK_HDR , 1 )
CONST( ISAL_BLOCK_TYPE0 , 2 )
CONST( ISAL_BLOCK_CODED , 3 )
CONST( ISAL_BLOCK_INPUT_DONE , 4 )
CONST( ISAL_BLOCK_FINISH , 5 )

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

#define SMALL_SHORT_SYM_LEN 9
#define SMALL_SHORT_SYM_MASK ((1 << SMALL_SHORT_SYM_LEN) - 1)
#define SMALL_LONG_SYM_LEN 9
#define SMALL_LONG_SYM_MASK ((1 << SMALL_LONG_SYM_LEN) - 1)
#define SMALL_SHORT_CODE_LEN_OFFSET 11
#define SMALL_LONG_CODE_LEN_OFFSET 10
#define SMALL_FLAG_BIT_OFFSET 10
#define SMALL_FLAG_BIT (1 << SMALL_FLAG_BIT_OFFSET)

#define DIST_SYM_OFFSET 0
#define DIST_SYM_LEN 5
#define DIST_SYM_MASK ((1 << DIST_SYM_LEN) - 1)
#define DIST_SYM_EXTRA_OFFSET 5
#define DIST_SYM_EXTRA_LEN 4
#define DIST_SYM_EXTRA_MASK ((1 << DIST_SYM_EXTRA_LEN) - 1)

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
	uxtw	read_in_length,read_in_length

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


	.global	decode_huffman_code_block_stateless_aarch64
	.type	decode_huffman_code_block_stateless_aarch64, %function
/*
	void decode_huffman_code_block_stateless_aarch64(
				struct inflate_state *state,
				uint8_t * start_out)
*/
	declare_generic_reg	arg0		0, x
	declare_generic_reg	arg1		1, x
	declare_generic_reg	arg2		2, x

	declare_generic_reg	state,		11,x
	declare_generic_reg	start_out,	18,x

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
	declare_generic_reg	bfinal,		22,x

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

decode_huffman_code_block_stateless_aarch64:
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
	adrp	rfc_table,rfc_lookup_table
	add	rfc_table,rfc_table,:lo12:rfc_lookup_table
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
	sub	bit_mask,next_sym,SMALL_FLAG_BIT
	mov	temp,1
	asr	bit_mask,bit_mask,SMALL_SHORT_CODE_LEN_OFFSET
	and	next_sym,next_sym,SMALL_SHORT_SYM_MASK
	lsl	bit_mask,w_temp,bit_mask
	sub	bit_mask,bit_mask,1
	and	x_next_bits,read_in,x_bit_mask
	add	next_bits,next_sym,next_bits,lsr ISAL_DECODE_SHORT_BITS
	mov	next_sym,(_dist_huff_code + _long_code_lookup_small)>>1
	add	next_bits,next_bits,next_sym
	ldrh	next_sym,[state,x_next_bits,lsl 1]
	lsr	bit_count,next_sym,SMALL_LONG_CODE_LEN_OFFSET
	b	decode_next_dist_adjust
decode_next_dist_flag:
	lsr	bit_count,next_sym,SMALL_SHORT_CODE_LEN_OFFSET
decode_next_dist_adjust:
	sub	read_in_length,read_in_length,bit_count
	lsr	read_in,read_in,x_bit_count
	cbnz	bit_count,decode_next_dist_end
	sub	read_in_length,read_in_length,next_sym
	mov	next_sym,INVALID_SYMBOL
decode_next_dist_end:
	and	next_sym,next_sym,DIST_SYM_MASK

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
	cmp	w_bfinal,0
	csel	block_state, block_state, w_bfinal, ne
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
	.size	decode_huffman_code_block_stateless_aarch64, .-decode_huffman_code_block_stateless_aarch64

	.type	rfc_lookup_table, %object

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
	.size	rfc_lookup_table, . - rfc_lookup_table
