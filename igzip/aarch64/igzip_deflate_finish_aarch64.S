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

.macro  update_state	stream:req,start_in:req,next_in:req,end_in:req,	\
	m_out_buf:req,m_out_start:req,tmp0:req,tmp1:req

	//m_out_buf=bytes_written
	sub	x_\m_out_buf,x_\m_out_buf,x_\m_out_start
	cmp	next_in,start_in
	bls	skip_has_hist
	mov	w_\tmp0,1
	strb	w_\tmp0,[x_\stream,_internal_state_has_hist]
skip_has_hist:
	ldr	w_\tmp0,[\stream,_total_in]
	ldr	x_\m_out_start,[\stream,_next_out] //m_out_start = next_out

	str	x_\next_in,[\stream,_next_in]
	sub	x_\start_in,x_\next_in,x_\start_in
	sub	x_\end_in,x_\end_in,x_\next_in
	add	w_\tmp0,w_\tmp0,w_\start_in
	stp	w_\end_in,w_\tmp0,[\stream,_avail_in]
	//next_in=avail_out,start_in=total_out
	ldp	w_\next_in,w_\start_in,[\stream,_avail_out]
	add	x_\m_out_start,x_\m_out_start,x_\m_out_buf
	str	x_\m_out_start,[\stream,_next_out]
	add	w_\start_in,w_\start_in,w_\m_out_buf
	sub	w_\next_in,w_\next_in,w_\m_out_buf
	stp	w_\next_in,w_\start_in,[\stream,_avail_out]
.endm
	.global	isal_deflate_finish_aarch64
	.arch armv8-a+crc
	.type	isal_deflate_finish_aarch64, %function
/*
	void isal_deflate_finish_aarch64(struct isal_zstream *stream)
*/
	declare_generic_reg	stream,		0,x	//struct isal_zstream *stream
	declare_generic_reg	state,		8,x	//&stream->state
	declare_generic_reg	avail_in,	9,w
	declare_generic_reg	end_of_stream,	10,w	//can be used in loop

	declare_generic_reg	hash_mask,	11,w
	declare_generic_reg	match_length,	12,w
	declare_generic_reg	hufftables,	13,x

	declare_generic_reg	m_out_buf,	14,x
	declare_generic_reg	m_out_start,	15,x
	declare_generic_reg	m_out_end,	16,x
	declare_generic_reg	m_bits,		17,x
	declare_generic_reg	m_bit_count,	18,w

	declare_generic_reg	start_in,	19,x
	declare_generic_reg	end_in,		20,x
	declare_generic_reg	next_in,	21,x
	declare_generic_reg	loop_end_cnt,	22,x

	declare_generic_reg	literal,	23,w
	declare_generic_reg	hash,		24,w
	declare_generic_reg	dist,		25,w

	declare_generic_reg	last_seen,	26,x
	declare_generic_reg	file_start,	27,x
	declare_generic_reg	hist_size,	28,w

	declare_generic_reg	tmp0,		5 ,w
	declare_generic_reg	tmp1,		6 ,w
	declare_generic_reg	tmp2,		7 ,w

	declare_generic_reg	code,		3,x
	declare_generic_reg	code_len,	24,x
	declare_generic_reg	code2,		10,x
	declare_generic_reg	code_len2,	4,x


isal_deflate_finish_aarch64:
	//save registers
	push_stack

	//	set_buf(&state->bitbuf, stream->next_out, stream->avail_out);
	ldr	w_m_out_end,[stream,_avail_out]
	ldr	m_out_buf,[stream,_next_out]
	add	m_out_end,m_out_buf,w_m_out_end,uxtw
	sub	m_out_end,m_out_end , 8
	mov	m_out_start,m_out_buf
	stp	m_out_buf,m_out_end,[stream, _bitbuf + _internal_state + _m_out_buf]
	str	m_out_start,[stream, _bitbuf + _internal_state + _m_out_start]
	ldr	m_bit_count ,[stream,_internal_state_bitbuf_m_bit_count]
	ldr	m_bits ,[stream,_internal_state_bitbuf_m_bits]

	//init variables
	//last_seen=&stream.internal_state.head = _internal_state+_head
	add	last_seen,stream,65536
	add	last_seen,last_seen,_internal_state+_head -65536


	//start_in=stream->next_in;next_in=start_in
	ldr	avail_in, [stream, _avail_in]
	ldr	start_in,[stream,_next_in]
	mov	next_in,start_in
	add	end_in,start_in,avail_in,uxtw  //avail_in reg is free now
	ldr	hufftables,[stream,_hufftables]
	cbz	avail_in, update_not_full


	sub	loop_end_cnt,end_in,4		//loop end
	cmp	next_in,loop_end_cnt


	//file_start = (uint8_t *) ((uintptr_t) stream->next_in - stream->total_in);
	ldr	w_file_start,[stream,_total_in]
	sub	file_start, next_in, w_file_start, uxtw

	//uint32_t hist_size = state->dist_mask;
	ldr	hist_size,[stream,_internal_state + _dist_mask]

	//uint32_t hash_mask = state->hash_mask;
	ldr	hash_mask,[stream,_internal_state + _hash_mask]

	bhi	main_loop_end
main_loop_start:
	//is_full(&state->bitbuf)
	cmp	m_out_buf,m_out_end
	bhi	update_state_exit

	ldr	literal,[next_in]
	crc32cw	hash,wzr,literal
	and	hash,hash,hash_mask

	///dist = (next_in - file_start - last_seen[hash]) & 0xFFFF;
	ldrh	w_tmp0,[last_seen,x_hash,lsl 1] 	//tmp_w last_seen[hash]
	sub 	x_dist,next_in,file_start
	//last_seen[hash] = (uint64_t) (next_in - file_start);
	strh	dist,[last_seen,x_hash,lsl 1]
	sub 	dist,dist,w_tmp0
	and	dist,dist,0xffff

	sub	w_tmp0,dist,1
	cmp	hist_size,w_tmp0
	bls	get_lit_code

	/// match_length = compare258(next_in - dist, next_in, 258);
	sub	x_tmp2,next_in,x_dist
	sub	x_hash,end_in,next_in
	compare_max_258_bytes	tmp2,next_in,hash,match_length,tmp0,tmp1
	cmp	match_length,3
	bls	get_lit_code

	get_len_code 	hufftables,match_length,code,code_len,tmp0
	get_dist_code 	hufftables,dist,code2,code_len2,tmp0,tmp1,tmp2

	//code |= code2 << code_len;
	//code_len += code_len2;
	lsl	code2,code2,code_len
	orr	code,code,code2
	add	code_len,code_len,code_len2

	//next_in += match_length;
	add	next_in,next_in,match_length,uxtw

	//write_bits(&state->bitbuf, code, code_len);
	update_bits	stream,code,code_len,m_bits,m_bit_count,m_out_buf

	cmp	next_in,loop_end_cnt
	bls	main_loop_start
	b	main_loop_end
get_lit_code:
	//get_lit_code(stream->hufftables, literal & 0xFF, &code, &code_len);
	and		literal,literal,0xff
	get_lit_code	hufftables,literal,code,code_len

	//next_in++;
	add	next_in,next_in,1

	//write_bits(&state->bitbuf, code, code_len);
	update_bits	stream,code,code_len,m_bits,m_bit_count,m_out_buf
	cmp	next_in,loop_end_cnt
	bls	main_loop_start
main_loop_end:
	sub	loop_end_cnt,end_in,1
	cmp	next_in,loop_end_cnt
	bhi	update_not_full
second_loop_start:
	cmp	m_out_buf,m_out_end
	bhi	update_state_exit
	ldr	literal,[next_in]
	and		literal,literal,0xff
	get_lit_code	hufftables,literal,code,code_len
	//next_in++;
	add	next_in,next_in,1

	//write_bits(&state->bitbuf, code, code_len);
	update_bits	stream,code,code_len,m_bits,m_bit_count,m_out_buf
	cmp	next_in,loop_end_cnt
	bls	second_loop_start

update_not_full:
	cmp	m_out_buf,m_out_end
	bhi	update_state_exit

	mov	literal,256
	get_lit_code	hufftables,literal,code,code_len

	//write_bits(&state->bitbuf, code, code_len);
	update_bits	stream,code,code_len,m_bits,m_bit_count,m_out_buf
	ldrh	w_end_of_stream, [stream, _end_of_stream]
	mov	w_tmp0,1
	strb	w_tmp0,[stream,_internal_state_has_eob]
	cmp	w_end_of_stream,w_tmp0
	mov	w_tmp0, ZSTATE_TRL
	mov	w_tmp1,	ZSTATE_SYNC_FLUSH
	csel	w_tmp0,w_tmp0,w_tmp1,eq
	str	w_tmp0, [stream, _internal_state+_state]

update_state_exit:
	update_state	stream,start_in,next_in,end_in,m_out_buf,m_out_start,tmp0,tmp1
	pop_stack
	ret

	.size	isal_deflate_finish_aarch64, .-isal_deflate_finish_aarch64
