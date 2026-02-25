;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;  Copyright(c) 2026 Intel Corporation All rights reserved.
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

;;;
;;; gf_vect_mul_avx2_gfni(len, mul_array, src, dest)
;;;

%include "reg_sizes.asm"
%include "gf_vect_gfni.inc"

%ifidn __OUTPUT_FORMAT__, elf64
 %define arg0  rdi
 %define arg1  rsi
 %define arg2  rdx
 %define arg3  rcx
 %define return rax
 %define func(x) x: endbranch

%elifidn __OUTPUT_FORMAT__, win64
 %define arg0  rcx
 %define arg1  rdx
 %define arg2  r8
 %define arg3  r9
 %define return rax
 %define func(x) proc_frame x

%endif


%define len   arg0
%define mul_array arg1
%define	src   arg2
%define dest  arg3
%define pos   return


;;; Use Non-temporal load/store
%ifdef NO_NT_LDST
 %define XLDR vmovdqa
 %define XSTR vmovdqa
%else
 %define XLDR vmovntdqa
 %define XSTR vmovntdq
%endif

default rel

[bits 64]
section .text

%define y0      ymm6
%define ytmp1   ymm7
%define ygft1   ymm8

align 16
mk_global gf_vect_mul_avx2_gfni, function
func(gf_vect_mul_avx2_gfni)

	; Check if length is multiple of 32 bytes
	test    len, 0x1f
	jnz     return_fail

	mov	pos, 0
	vmovdqu	ygft1, [mul_array]

align 16
loop32:
	XLDR	y0, [src+pos]		;Get next data from source vector
	vgf2p8affineqb ytmp1, y0, ygft1, 0x00
	XSTR	[dest+pos], ytmp1	;Store result

	add	pos, 32			;Loop on 32 bytes at a time
	cmp	pos, len
	jb	loop32

return_pass:
	xor     return, return
	ret

return_fail:
	mov	return, 1
	ret

endproc_frame
