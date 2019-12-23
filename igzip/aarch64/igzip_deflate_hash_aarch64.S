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



	.global	isal_deflate_hash_aarch64
	.type	isal_deflate_hash_aarch64, %function
/*
	void isal_deflate_hash_aarch64(uint16_t * hash_table, uint32_t hash_mask,
			    uint32_t current_index, uint8_t * dict, uint32_t dict_len)
*/
	declare_generic_reg	hash_table,	0,x
	declare_generic_reg	hash_mask,	1,w
	declare_generic_reg	current_index,	2,w
	declare_generic_reg	dict,		3,x
	declare_generic_reg	dict_len,	4,w

	declare_generic_reg 	next_in		3,x
	declare_generic_reg 	end_in		6,x
	declare_generic_reg 	ind		5,w
	declare_generic_reg 	hash		2,w
	declare_generic_reg 	literal		2,w
#define SHORTEST_MATCH #4

isal_deflate_hash_aarch64:
	sub     ind, current_index, dict_len
	and	ind,ind,0xffff


        uxtw    x_dict_len, dict_len
        sub     x_dict_len, x_dict_len, SHORTEST_MATCH
        add     end_in, dict, x_dict_len



        cmp     next_in, end_in
        bcs     exit_func

        mov     w7, 0
loop_start:
        ldr     literal, [next_in]
        add     next_in, next_in, 1
        cmp     next_in, end_in
        crc32cw hash, w7, literal
        and     hash, hash, hash_mask
        strh    ind, [hash_table, x_hash, lsl 1]
        add	ind,ind,1
        bne     loop_start
exit_func:

	ret
	.size	isal_deflate_hash_aarch64, .-isal_deflate_hash_aarch64
