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

#ifndef __HUFFMAN_AARCH64_H__
#define __HUFFMAN_AARCH64_H__

#ifdef __ASSEMBLY__
#ifdef LONGER_HUFFTABLE
	#if (D > 8192)
		#error History D is larger than 8K
	#else
		#define DIST_TABLE_SIZE 8192
		#define DECODE_OFFSET 26
	#endif
#else
	#define DIST_TABLE_SIZE 2
	#define DECODE_OFFSET 0
#endif

#define LEN_TABLE_SIZE 256
#define LIT_TABLE_SIZE 257

#define DIST_TABLE_START (ISAL_DEF_MAX_HDR_SIZE + 8)	//328+8
#define DIST_TABLE_OFFSET (DIST_TABLE_START + - 4 * 1)	//336-4
#define LEN_TABLE_OFFSET (DIST_TABLE_START + DIST_TABLE_SIZE * 4 - 4*3) //332 + 2*4 -4*3 =328
#define LIT_TABLE_OFFSET (DIST_TABLE_START + 4 * DIST_TABLE_SIZE + 4 * LEN_TABLE_SIZE)
#define LIT_TABLE_SIZES_OFFSET (LIT_TABLE_OFFSET + 2 * LIT_TABLE_SIZE)
#define DCODE_TABLE_OFFSET (LIT_TABLE_SIZES_OFFSET + LIT_TABLE_SIZE + 1 - DECODE_OFFSET * 2)
#define DCODE_TABLE_SIZE_OFFSET (DCODE_TABLE_OFFSET + 2 * 30 - DECODE_OFFSET)

#define IGZIP_DECODE_OFFSET	0
#define IGZIP_DIST_TABLE_SIZE	2

.macro	get_len_code hufftables:req,length:req,code:req,code_len:req,tmp0:req
	add x_\tmp0,\hufftables,LEN_TABLE_OFFSET
	ldr	w_\code_len,[x_\tmp0,x_\length,lsl 2]
	lsr	w_\code, w_\code_len , 5
	and	x_\code_len,x_\code_len,0x1f
.endm

.macro	get_lit_code	hufftables:req,lit:req,code:req,code_len:req
	add	x_\code,\hufftables,LIT_TABLE_OFFSET
	ldrh	w_\code,[x_\code,x_\lit,lsl 1]
	add	x_\code_len,\hufftables,LIT_TABLE_SIZES_OFFSET
	ldrb	w_\code_len,[x_\code_len,x_\lit]
.endm

.macro	get_dist_code	hufftables:req,dist:req,code:req,code_len:req,tmp0:req,tmp1:req,tmp2:req
	cmp 	dist,DIST_TABLE_SIZE
	bhi	_compute_dist_code
	add 	x_\tmp0,\hufftables,DIST_TABLE_OFFSET
	ldr	w_\code_len,[x_\tmp0,x_\dist,lsl 2]
	lsr	w_\code, w_\code_len , 5
	and	x_\code_len,x_\code_len,0x1f
	b	_end_get_dist_code
_compute_dist_code:
	and	w_\dist,w_\dist,0xffff
	sub	w_\dist,w_\dist,1
	clz	w_\tmp0,w_\dist
	mov	w_\tmp1,30
	sub	w_\tmp0,w_\tmp1,w_\tmp0		//tmp0== num_extra_bists
	mov	w_\tmp1,1
	lsl	w_\tmp1,w_\tmp1,w_\tmp0
	sub	w_\tmp1,w_\tmp1,1
	and	w_\tmp1,w_\tmp1,w_\dist		//tmp1=extra_bits
	asr	w_\dist,w_\dist,w_\tmp0
	lsl	w_\tmp2,w_\tmp0,1
	add	w_\tmp2,w_\dist,w_\tmp2		//tmp2=sym

	add 	x_\code,\hufftables,DCODE_TABLE_OFFSET - IGZIP_DECODE_OFFSET*2
	add 	x_\code_len,\hufftables,DCODE_TABLE_SIZE_OFFSET - IGZIP_DECODE_OFFSET
	ldrh	w_\code,[x_\code,x_\tmp2,lsl 1]
	ldrb	w_\code_len,[x_\code_len,x_\tmp2]
	lsl	w_\tmp1,w_\tmp1,w_\code_len
	orr	w_\code,w_\code,w_\tmp1
	add	w_\code_len,w_\code_len,w_\tmp0

	//compute_dist_code
_end_get_dist_code:
.endm


.macro compare_258_bytes str0:req,str1:req,match_length:req,tmp0:req,tmp1:req
	mov	x_\match_length,0
_compare_258_loop:
	ldr	x_\tmp0,[x_\str0,x_\match_length]
	ldr	x_\tmp1,[x_\str1,x_\match_length]
	eor 	x_\tmp0,x_\tmp1,x_\tmp0
	rbit	x_\tmp0,x_\tmp0
	clz	x_\tmp0,x_\tmp0
	lsr	x_\tmp0,x_\tmp0,3
	add	x_\match_length,x_\match_length,x_\tmp0


	cmp	x_\match_length,257
	ccmp	x_\tmp0,8,0,ls
	beq	_compare_258_loop

	cmp	x_\match_length,258
	mov	x_\tmp1,258
	csel	x_\match_length,x_\match_length,x_\tmp1,ls
.endm

.macro compare_max_258_bytes str0:req,str1:req,max_length:req,match_length:req,tmp0:req,tmp1:req
	mov	x_\match_length,0
	mov 	x_\tmp0,258
	cmp	x_\max_length,x_\tmp0
	csel	x_\max_length,x_\max_length,x_\tmp0,ls
_compare_258_loop:
	ldr	x_\tmp0,[x_\str0,x_\match_length]
	ldr	x_\tmp1,[x_\str1,x_\match_length]
	eor 	x_\tmp0,x_\tmp1,x_\tmp0
	rbit	x_\tmp0,x_\tmp0
	clz	x_\tmp0,x_\tmp0
	lsr	x_\tmp0,x_\tmp0,3
	add	x_\match_length,x_\match_length,x_\tmp0


	cmp	x_\max_length,x_\match_length
	ccmp	x_\tmp0,8,0,hi
	beq	_compare_258_loop

	cmp	x_\match_length,x_\max_length
	csel	x_\match_length,x_\match_length,x_\max_length,ls
.endm

.macro compare_aarch64 str0:req,str1:req,max_length:req,match_length:req,tmp0:req,tmp1:req
	mov	x_\match_length,0
_compare_loop:
	ldr	x_\tmp0,[x_\str0,x_\match_length]
	ldr	x_\tmp1,[x_\str1,x_\match_length]
	eor 	x_\tmp0,x_\tmp1,x_\tmp0
	rbit	x_\tmp0,x_\tmp0
	clz	x_\tmp0,x_\tmp0
	lsr	x_\tmp0,x_\tmp0,3
	add	x_\match_length,x_\match_length,x_\tmp0

	cmp	x_\max_length,x_\match_length
	ccmp	x_\tmp0,8,0,hi
	beq	_compare_loop

	cmp	x_\match_length,x_\max_length
	csel	x_\match_length,x_\match_length,x_\max_length,ls
.endm

#endif
#endif
