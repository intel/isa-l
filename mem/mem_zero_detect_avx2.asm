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
align 32	; maximize mu-ops cache usage
mk_global  mem_zero_detect_avx2, function
func(mem_zero_detect_avx2)
	FUNC_SAVE
	cmp	len, 127
	jbe	.mem_z_small_block
	; check the first 128 bytes
	vpxor	xmm2, xmm2, xmm2
	vmovdqu ymm0, [src]
	vpor	ymm0, ymm0, [src+32]
	vmovdqu	ymm1, [src+64]
	vpor	ymm1, ymm1, [src+96]
	vpor	ymm0, ymm0, ymm1
	vpcmpeqb ymm0, ymm2, ymm0
	vpmovmskb DWORD(tmp0), ymm0
	not	DWORD(tmp0)
	mov	DWORD(tmp1), DWORD(len)
	and	DWORD(tmp1), 127
	add	src, tmp1
	xor	eax, eax
	shr	len, 7	; len/128
	test	len, len; break partial flag stall
	setz	al	; if len < 128, eax != 0
	add	eax, DWORD(tmp0) ; jump if (edx OR eax) !=0, use add for macrofusion
	jnz .return
	xor	eax, eax

align 16
.mem_z_loop:
	vmovdqu	ymm0, [src]
	vpor	ymm0, ymm0,[src+32]
	vmovdqu	ymm1, [src+64]
	vpor	ymm1, ymm1, [src+96]
	add	src, 128
	xor	DWORD(tmp1), DWORD(tmp1)
	sub	len, 1
	setz	BYTE(tmp1)
	vpor	ymm0, ymm0, ymm1
	vpcmpeqb ymm0, ymm2, ymm0
	vpmovmskb DWORD(tmp0), ymm0
	not	DWORD(tmp0)
	add	DWORD(tmp1), DWORD(tmp0)
	jz	.mem_z_loop

.return:
	xor	eax, eax
	test	tmp0, tmp0
	setnz	al
	FUNC_RESTORE
	ret


align 16
.mem_z_small_block:
	;len < 128
	xor	DWORD(tmp0), DWORD(tmp0)
	movzx	DWORD(tmp1), BYTE(len)
	cmp	DWORD(len), 16
	jb     .mem_z_small_check_zero
	;17 < len < 128
	shr	DWORD(len), 4
	xor	eax, eax ; alignment
.mem_z_small_block_loop:
	xor	eax, eax
	mov	tmp0, [src]
	or	tmp0, [src+8]
	sub	DWORD(len), 1
	setz	al
	add	src, 16
	add	rax, tmp0
	jz	.mem_z_small_block_loop

	test	tmp0, tmp0
	jnz	.return_small
	movzx	DWORD(len), BYTE(tmp1)

.mem_z_small_check_zero:
	xor	DWORD(tmp0), DWORD(tmp0)
	and	DWORD(len), 15
	jz	.return_small
.mem_z_small_byte_loop:
	movzx	eax, byte [src]
	add	src, 1
	or	DWORD(tmp0), eax
	sub	DWORD(len), 1
	jnz	.mem_z_small_byte_loop
.return_small:
	xor	eax, eax
	test	tmp0, tmp0
	setnz	al
	ret

endproc_frame
