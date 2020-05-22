;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;  Copyright(c) 2011-2018 Intel Corporation All rights reserved.
;
;  Redistribution and use in source and binary forms, with or without
;  modification, are permitted provided that the following conditions
;  are met:
;    * Redistributions of source code must retain the above copyright
;      notice, this list of conditions and the following disclaimer.
;    * Redistributions in binary form must reproduce the above copyright
;      notice, this list of conditions and the following disclaimer in
;      the documentation and/or other materials provided with the
;      distribution.
;    * Neither the name of Intel Corporation nor the names of its
;      contributors may be used to endorse or promote products derived
;      from this software without specific prior written permission.
;
;  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
;  "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
;  LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
;  A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
;  OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
;  SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
;  LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
;  DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
;  THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
;  (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
;  OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

%include "options.asm"
%include "lz0a_const.asm"
%include "data_struct2.asm"
%include "huffman.asm"
%include "reg_sizes.asm"

%define DICT_SLOP 8
%define DICT_END_SLOP 4

%ifidn __OUTPUT_FORMAT__, win64
%define arg1 rcx
%define arg2 rdx
%define arg3 r8
%define arg4 r9
%define arg5 rdi
%define swap1 rsi
%define stack_size 3 * 8
%define PS 8
%define arg(x)      [rsp + stack_size + PS*x]
%else
%define arg1 rdi
%define arg2 rsi
%define arg3 rdx
%define arg4 rcx
%define arg5 r8
%define swap1 r9
%endif

%define hash_table arg1

%define hash_mask arg2

%define f_i_end arg3

%define dict_offset arg4

%define dict_len arg5
%define f_i arg5

%define f_i_tmp rax

%define hash swap1

%define hash2 r10

%define hash3 r11

%define hash4 r12


%macro FUNC_SAVE 0
%ifidn __OUTPUT_FORMAT__, win64
	push rsi
	push rdi
	push r12
	mov arg5 %+ d, arg(5)
%else
	push r12
%endif
%endm

%macro FUNC_RESTORE 0
%ifidn __OUTPUT_FORMAT__, win64
	pop r12
	pop rdi
	pop rsi
%else
	pop r12
%endif
%endm

[bits 64]
default rel
section .text

global isal_deflate_hash_crc_01
isal_deflate_hash_crc_01:
	endbranch
	FUNC_SAVE

	neg	f_i
	add	f_i, f_i_end

	sub	dict_offset, f_i

	sub	f_i_end, DICT_SLOP
	cmp	f_i, f_i_end
	jg	end_main

main_loop:
	lea	f_i_tmp, [f_i + 2]

	xor	hash, hash
	crc32	hash %+ d, dword [f_i + dict_offset]

	xor	hash2, hash2
	crc32	hash2 %+ d, dword [f_i + dict_offset + 1]

	xor	hash3, hash3
	crc32	hash3 %+ d, dword [f_i_tmp + dict_offset]

	xor	hash4, hash4
	crc32	hash4 %+ d, dword [f_i_tmp + dict_offset + 1]

	and	hash, hash_mask
	and	hash2, hash_mask
	and	hash3, hash_mask
	and	hash4, hash_mask

	mov	[hash_table + 2 * hash], f_i %+ w
	add	f_i, 1

	mov	[hash_table + 2 * hash2], f_i %+ w
	add	f_i, 3

	mov	[hash_table + 2 * hash3], f_i_tmp %+ w
	add	f_i_tmp, 1

	mov	[hash_table + 2 * hash4], f_i_tmp %+ w

	cmp	f_i, f_i_end
	jle	main_loop

end_main:
	add	f_i_end, DICT_SLOP - DICT_END_SLOP
	cmp	f_i, f_i_end
	jg	end

end_loop:
	xor	hash, hash
	crc32	hash %+ d, dword [f_i + dict_offset]

	and	hash, hash_mask
	mov	[hash_table + 2 * hash], f_i %+ w

	add	f_i, 1
	cmp	f_i, f_i_end
	jle	end_loop
end:
	FUNC_RESTORE
	ret
