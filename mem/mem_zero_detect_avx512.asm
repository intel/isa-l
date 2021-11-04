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

%include "reg_sizes.asm"

%ifidn __OUTPUT_FORMAT__, elf64
 %define arg0  rdi
 %define arg1  rsi
 %define arg2  rdx
 %define arg3  rcx
 %define arg4  r8
 %define arg5  r9
 %define tmp   r11
 %define tmpb  r11b
 %define tmp3  arg4
 %define return rax
 %define func(x) x: endbranch
 %define FUNC_SAVE
 %define FUNC_RESTORE
%endif

%ifidn __OUTPUT_FORMAT__, win64
 %define arg0  rcx
 %define arg1  rdx
 %define arg2  r8
 %define arg3  r9
 %define tmp   r11
 %define tmpb  r11b
 %define tmp3  r10
 %define return rax
 %define func(x) proc_frame x
 %macro FUNC_SAVE 0
	end_prolog
 %endmacro
 %macro FUNC_RESTORE 0
 %endmacro
%endif

%define src arg0
%define	len arg1
%define tmp0 arg2
%define tmp1 arg3

%use smartalign
ALIGNMODE P6
default rel

[bits 64]
section .text
align 32	; maximize mu-ops cache coverage
mk_global  mem_zero_detect_avx512, function
func(mem_zero_detect_avx512)
	FUNC_SAVE
	or	tmp1, -1	; all ones mask
	mov	eax, DWORD(src)
	and	eax, 63
	neg	rax
	add	rax, 64		; 64 - eax
	cmp	rax, len
	cmovae	eax, DWORD(len)
	bzhi	tmp1, tmp1, rax ; alignment mask
	kmovq 	k1, tmp1
	vmovdqu8 zmm0{k1}{z}, [src]
	add	src, rax	; align to cacheline
	sub	len, rax
	vptestmb k1, zmm0, zmm0
	xor	DWORD(tmp0), DWORD(tmp0)
	ktestq	k1, k1
	setnz	BYTE(tmp0)
	mov	DWORD(tmp3), DWORD(len)
	xor	eax, eax
	shr	len, 7		; len/128
	setz	al
	add	eax, DWORD(tmp0)
	jnz	.mem_z_small_block


align 16
.mem_z_loop:
	vmovdqa64	zmm0, [src]
	vporq	zmm0, zmm0,[src+64]
	xor	tmp1,tmp1
	sub	len, 1
	setz	BYTE(tmp1)
	add	src, 128
	vptestmb k1, zmm0, zmm0
	kmovq	tmp0, k1
	add	tmp1, tmp0 ; for macrofusion.
	jz	.mem_z_loop

align 16
.mem_z_small_block:
	;len < 128
	xor	eax, eax
	lea	tmp1, [rax-1]   ; 0xFFFFFF...
	mov	DWORD(len), DWORD(tmp3)
	and	DWORD(len), 127 ; len % 128
	and	DWORD(tmp3),63  ; len % 64
	bzhi	tmp, tmp1, tmp3; mask
	cmp	DWORD(len), 64
	cmovb	tmp1, tmp
	cmovb	tmp, rax
	kmovq	k1, tmp1
	kmovq	k2, tmp
	vmovdqu8 zmm0{k1}{z}, [src]
	vmovdqu8 zmm1{k2}{z}, [src+64]
	vporq	zmm0, zmm0, zmm1
	vptestmb k1, zmm0, zmm0
	kmovq	tmp1, k1
	or	tmp0, tmp1
	setnz	al	; eax is still zero
	FUNC_RESTORE
	ret


endproc_frame
