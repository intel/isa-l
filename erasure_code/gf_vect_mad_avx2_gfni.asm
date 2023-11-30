;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;  Copyright(c) 2023 Intel Corporation All rights reserved.
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
;;; gf_vect_mad_avx2_gfni(len, vec, vec_i, mul_array, src, dest);
;;;

%include "reg_sizes.asm"
%include "gf_vect_gfni.inc"
%include "memcpy.asm"

%if AS_FEATURE_LEVEL >= 10

%ifidn __OUTPUT_FORMAT__, elf64
 %define arg0  rdi
 %define arg1  rsi
 %define arg2  rdx
 %define arg3  rcx
 %define arg4  r8
 %define arg5  r9
 %define tmp   r11
 %define tmp2  r10
 %define func(x) x: endbranch
 %define FUNC_SAVE
 %define FUNC_RESTORE
%endif

%ifidn __OUTPUT_FORMAT__, win64
 %define arg0   rcx
 %define arg1   rdx
 %define arg2   r8
 %define arg3   r9
 %define arg4   r12 		; must be saved and loaded
 %define arg5   r13
 %define tmp    r11
 %define tmp2   r10
 %define stack_size 3*8
 %define arg(x)      [rsp + stack_size + 8 + 8*x]
 %define func(x) proc_frame x

 %macro FUNC_SAVE 0
	sub	rsp, stack_size
	mov	[rsp + 0*8], r12
	mov	[rsp + 1*8], r13
	end_prolog
	mov	arg4, arg(4)
	mov	arg5, arg(5)
 %endmacro

 %macro FUNC_RESTORE 0
	mov	r12,  [rsp + 0*8]
	mov	r13,  [rsp + 1*8]
	add	rsp, stack_size
 %endmacro
%endif

;;; gf_vect_mad_avx2_gfni(len, vec, vec_i, mul_array, src, dest)
%define len   arg0
%define vec   arg1
%define vec_i    arg2
%define mul_array arg3
%define	src   arg4
%define dest  arg5
%define pos   rax

%ifndef EC_ALIGNED_ADDR
;;; Use Un-aligned load/store
 %define XLDR vmovdqu
 %define XSTR vmovdqu
%else
;;; Use Non-temporal load/stor
 %ifdef NO_NT_LDST
  %define XLDR vmovdqa
  %define XSTR vmovdqa
 %else
  %define XLDR vmovntdqa
  %define XSTR vmovntdq
 %endif
%endif


default rel

[bits 64]
section .text

%define x0       ymm0
%define xd       ymm1
%define xgft1    ymm2
%define xret1    ymm3

;;
;; Encodes 32 bytes of a single source and updates single parity disk
;;
%macro ENCODE_32B 0

	XLDR	x0, [src + pos]			;Get next source vector
	XLDR	xd, [dest + pos]		;Get next dest vector

        GF_MUL_XOR VEX, x0, xgft1, xret1, xd

	XSTR	[dest + pos], xd
%endmacro

;;
;; Encodes less than 32 bytes of a single source and updates single parity disk
;;
%macro ENCODE_LT_32B 1
%define %%LEN 	%1

	simd_load_avx2 x0, src + pos, %%LEN, tmp, tmp2  ;Get next source vector
	simd_load_avx2 xd, dest + pos, %%LEN, tmp, tmp2 ;Get next dest vector

        GF_MUL_XOR VEX, x0, xgft1, xret1, xd

	lea 	tmp, [dest + pos]
	simd_store_avx2 tmp, xd, %%LEN, tmp2, pos ;Store updated encoded data
%endmacro

align 16
mk_global gf_vect_mad_avx2_gfni, function
func(gf_vect_mad_avx2_gfni)
	FUNC_SAVE
	xor	pos, pos
	shl	vec_i, 3		;Multiply by 8

        vbroadcastsd xgft1, [vec_i + mul_array]

.loop32:
        ENCODE_32B

	add	pos, 32			;Loop on 32 bytes at a time
        sub     len, 32
	cmp	len, 32
	jge	.loop32

.len_lt_32:
        cmp     len, 0
        jle     .exit

        ENCODE_LT_32B len

.exit:
        vzeroupper

	FUNC_RESTORE
	ret

endproc_frame
%endif  ; if AS_FEATURE_LEVEL >= 10
