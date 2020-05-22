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
%define ptr arg2
%define pos return

default rel

[bits 64]
section .text

align 16
mk_global  mem_zero_detect_avx, function
func(mem_zero_detect_avx)
	FUNC_SAVE
	mov	pos, 0
	sub	len, 4*32
	jle	.mem_z_small_block

.mem_z_loop:
	vmovdqu	ymm0, [src+pos]
	vmovdqu	ymm1, [src+pos+1*32]
	vmovdqu	ymm2, [src+pos+2*32]
	vmovdqu	ymm3, [src+pos+3*32]
	vptest	ymm0, ymm0
	jnz	.return_fail
	vptest	ymm1, ymm1
	jnz	.return_fail
	vptest	ymm2, ymm2
	jnz	.return_fail
	vptest	ymm3, ymm3
	jnz	.return_fail
	add	pos, 4*32
	cmp	pos, len
	jl	.mem_z_loop

.mem_z_last_block:
	vmovdqu	ymm0, [src+len]
	vmovdqu	ymm1, [src+len+1*32]
	vmovdqu	ymm2, [src+len+2*32]
	vmovdqu	ymm3, [src+len+3*32]
	vptest	ymm0, ymm0
	jnz	.return_fail
	vptest	ymm1, ymm1
	jnz	.return_fail
	vptest	ymm2, ymm2
	jnz	.return_fail
	vptest	ymm3, ymm3
	jnz	.return_fail

.return_pass:
	mov	return, 0
	FUNC_RESTORE
	ret


.mem_z_small_block:
	add	len, 4*32
	cmp	len, 2*32
	jl	.mem_z_lt64
	vmovdqu	ymm0, [src]
	vmovdqu	ymm1, [src+32]
	vmovdqu	ymm2, [src+len-2*32]
	vmovdqu	ymm3, [src+len-1*32]
	vptest	ymm0, ymm0
	jnz	.return_fail
	vptest	ymm1, ymm1
	jnz	.return_fail
	vptest	ymm2, ymm2
	jnz	.return_fail
	vptest	ymm3, ymm3
	jnz	.return_fail
	jmp	.return_pass

.mem_z_lt64:
	cmp	len, 32
	jl	.mem_z_lt32
	vmovdqu	ymm0, [src]
	vmovdqu	ymm1, [src+len-32]
	vptest	ymm0, ymm0
	jnz	.return_fail
	vptest	ymm1, ymm1
	jnz	.return_fail
	jmp	.return_pass


.mem_z_lt32:
	cmp	len, 16
	jl	.mem_z_lt16
	vmovdqu	xmm0, [src]
	vmovdqu	xmm1, [src+len-16]
	vptest	xmm0, xmm0
	jnz	.return_fail
	vptest	xmm1, xmm1
	jnz	.return_fail
	jmp	.return_pass


.mem_z_lt16:
	cmp	len, 8
	jl	.mem_z_lt8
	mov	tmp, [src]
	mov	tmp3,[src+len-8]
	or	tmp, tmp3
	test	tmp, tmp
	jnz	.return_fail
	jmp	.return_pass

.mem_z_lt8:
	cmp	len, 0
	je	.return_pass
.mem_z_1byte_loop:
	mov	tmpb, [src+pos]
	cmp	tmpb, 0
	jnz	.return_fail
	add	pos, 1
	cmp	pos, len
	jl	.mem_z_1byte_loop
	jmp	.return_pass

.return_fail:
	mov	return, 1
	FUNC_RESTORE
	ret

endproc_frame
