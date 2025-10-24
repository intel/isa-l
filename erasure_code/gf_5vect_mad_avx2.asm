;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;  Copyright(c) 2011-2015 Intel Corporation All rights reserved.
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
;;; gf_5vect_mad_avx2(len, vec, vec_i, mul_array, src, dest);
;;;

%include "reg_sizes.asm"

%define PS 8

%ifidn __OUTPUT_FORMAT__, win64
 %define arg0  rcx
 %define arg0.w ecx
 %define arg1  rdx
 %define arg2  r8
 %define arg3  r9
 %define arg4  r12
 %define arg5  r15
 %define tmp    r11
 %define tmp.w  r11d
 %define tmp.b  r11b
 %define tmp2   r10
 %define tmp3   r13
 %define tmp3.w   r13d
 %define tmp3.b   r13b
 %define tmp4   r14
 %define tmp5   rdi
 %define return rax
 %define return.w eax
 %define stack_size 16*10 + 5*8
 %define arg(x)      [rsp + stack_size + PS + PS*x]
 %define func(x) proc_frame x

%macro FUNC_SAVE 0
	sub	rsp, stack_size
	movdqa	[rsp+16*0],xmm6
	movdqa	[rsp+16*1],xmm7
	movdqa	[rsp+16*2],xmm8
	movdqa	[rsp+16*3],xmm9
	movdqa	[rsp+16*4],xmm10
	movdqa	[rsp+16*5],xmm11
	movdqa	[rsp+16*6],xmm12
	movdqa	[rsp+16*7],xmm13
	movdqa	[rsp+16*8],xmm14
	movdqa	[rsp+16*9],xmm15
	save_reg	r12,  10*16 + 0*8
	save_reg	r15,  10*16 + 1*8
	save_reg	r13,  10*16 + 2*8
	save_reg	r14,  10*16 + 3*8
	save_reg	rdi,  10*16 + 4*8
	end_prolog
	mov	arg4, arg(4)
	mov	arg5, arg(5)
%endmacro

%macro FUNC_RESTORE 0
	movdqa	xmm6, [rsp+16*0]
	movdqa	xmm7, [rsp+16*1]
	movdqa	xmm8, [rsp+16*2]
	movdqa	xmm9, [rsp+16*3]
	movdqa	xmm10, [rsp+16*4]
	movdqa	xmm11, [rsp+16*5]
	movdqa	xmm12, [rsp+16*6]
	movdqa	xmm13, [rsp+16*7]
	movdqa	xmm14, [rsp+16*8]
	movdqa	xmm15, [rsp+16*9]
	mov	r12,  [rsp + 10*16 + 0*8]
	mov	r15,  [rsp + 10*16 + 1*8]
	mov	r13,  [rsp + 10*16 + 2*8]
	mov	r14,  [rsp + 10*16 + 3*8]
	mov	rdi,  [rsp + 10*16 + 4*8]
	add	rsp, stack_size
%endmacro

%elifidn __OUTPUT_FORMAT__, elf64
 %define arg0  rdi
 %define arg0.w edi
 %define arg1  rsi
 %define arg2  rdx
 %define arg3  rcx
 %define arg4  r8
 %define arg5  r9
 %define tmp    r11
 %define tmp.w  r11d
 %define tmp.b  r11b
 %define tmp2   r10
 %define tmp3   r12		; must be saved and restored
 %define tmp3.w   r12d
 %define tmp3.b   r12b
 %define tmp4   r13		; must be saved and restored
 %define tmp5   r14		; must be saved and restored
 %define return rax
 %define return.w eax

 %define func(x) x: endbranch
 %macro FUNC_SAVE 0
	push	r12
	push	r13
	push	r14
 %endmacro
 %macro FUNC_RESTORE 0
	pop	r14
	pop	r13
	pop	r12
 %endmacro
%endif

;;; gf_5vect_mad_avx2(len, vec, vec_i, mul_array, src, dest)
%define len   arg0
%define len.w arg0.w
%define vec    arg1
%define vec_i    arg2
%define mul_array arg3
%define	src   arg4
%define dest1  arg5
%define pos   return
%define pos.w return.w

%define dest2 tmp2
%define dest3 mul_array
%define dest4 vec
%define dest5 vec_i

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

%define xmask0f   ymm15
%define xmask0fx  xmm15
%define xgft1_lo  ymm14
%define xgft2_lo  ymm13
%define xgft3_lo  ymm12
%define xgft4_lo  ymm11
%define xgft5_lo  ymm10

%define x0      ymm0
%define xtmpa   ymm1
%define xtmpl   ymm2
%define xtmplx  xmm2
%define xtmph1  ymm3
%define xtmph1x xmm3
%define xtmph2  ymm4
%define xd1     ymm5
%define xd2     ymm6
%define xd3     ymm7
%define xd4     ymm8
%define xd5     ymm9

align 16
mk_global gf_5vect_mad_avx2, function
func(gf_5vect_mad_avx2)
	FUNC_SAVE
	sub	len, 32
	jl	.return_fail
	xor	pos, pos
	mov	tmp.b, 0x0f
	vpinsrb	xmask0fx, xmask0fx, tmp.w, 0
	vpbroadcastb xmask0f, xmask0fx	;Construct mask 0x0f0f0f...

	sal	vec_i, 5		;Multiply by 32
	lea	tmp, [mul_array + vec_i]
	mov	tmp3, tmp
	sal	vec, 5			;Multiply by 32

	mov	tmp4, vec

	mov	dest3, [dest1+2*PS]	; reuse mul_array
	mov	dest4, [dest1+3*PS]	; reuse vec
	mov	dest5, [dest1+4*PS]	; reuse vec_i
	mov	dest2, [dest1+PS]
	mov	dest1, [dest1]

	lea	tmp5, [tmp4+2*tmp4]	; vec*3, for addressing

.loop32:
	XLDR	x0, [src+pos]		;Get next source vector

	XLDR	xd1, [dest1+pos]	;Get next dest vector
	XLDR	xd2, [dest2+pos]	;Get next dest vector
	XLDR	xd3, [dest3+pos]	;Get next dest vector
	XLDR	xd4, [dest4+pos]	;Get next dest vector
	XLDR	xd5, [dest5+pos]	;Get next dest vector

	vpand	xtmpl, x0, xmask0f	;Mask low src nibble in bits 4-0
	vpsraw	x0, x0, 4		;Shift to put high nibble into bits 4-0
	vpand	x0, x0, xmask0f		;Mask high src nibble in bits 4-0

	vbroadcasti128	xgft1_lo, [tmp]		;Load array: lo | lo
	vbroadcasti128	xtmph1,   [tmp+16]	;            hi | hi
	vbroadcasti128	xgft2_lo, [tmp+tmp4]	;Load array: lo | lo
	vbroadcasti128	xtmph2,   [tmp+tmp4+16]	;            hi | hi

	; dest1
	vpshufb	xtmph1, x0			;Lookup mul table of high nibble
	vpshufb	xgft1_lo, xtmpl			;Lookup mul table of low nibble
	vpxor	xtmph1, xgft1_lo		;GF add high and low partials
	vpxor	xd1, xtmph1			;xd1 += partial

	vbroadcasti128	xgft3_lo, [tmp+2*tmp4]
	vbroadcasti128	xtmph1,   [tmp+2*tmp4+16]

	; dest2
	vpshufb	xtmph2, x0			;Lookup mul table of high nibble
	vpshufb	xgft2_lo, xtmpl			;Lookup mul table of low nibble
	vpxor	xtmph2, xgft2_lo		;GF add high and low partials
	vpxor	xd2, xtmph2			;xd2 += partial

	vbroadcasti128	xgft4_lo, [tmp+tmp5]	;Load array: lo | lo (vec*3)
	vbroadcasti128	xtmph2,   [tmp+tmp5+16]	;            hi | hi (vec*3)

	; dest3
	vpshufb	xtmph1, x0			;Lookup mul table of high nibble
	vpshufb	xgft3_lo, xtmpl			;Lookup mul table of low nibble
	vpxor	xtmph1, xgft3_lo		;GF add high and low partials
	vpxor	xd3, xtmph1			;xd3 += partial

	vbroadcasti128	xgft5_lo, [tmp+4*tmp4]
	vbroadcasti128	xtmph1,   [tmp+4*tmp4+16]

	; dest4
	vpshufb	xtmph2, x0			;Lookup mul table of high nibble
	vpshufb	xgft4_lo, xtmpl			;Lookup mul table of low nibble
	vpxor	xtmph2, xgft4_lo		;GF add high and low partials
	vpxor	xd4, xtmph2			;xd4 += partial

	; dest5
	vpshufb	xtmph1, x0			;Lookup mul table of high nibble
	vpshufb	xgft5_lo, xtmpl			;Lookup mul table of low nibble
	vpxor	xtmph1, xgft5_lo		;GF add high and low partials
	vpxor	xd5, xtmph1			;xd5 += partial

	XSTR	[dest1+pos], xd1
	XSTR	[dest2+pos], xd2
	XSTR	[dest3+pos], xd3
	XSTR	[dest4+pos], xd4
	XSTR	[dest5+pos], xd5

	add	pos, 32			;Loop on 32 bytes at a time
	cmp	pos, len
	jle	.loop32

	lea	tmp, [len + 32]
	cmp	pos, tmp
	je	.return_pass

.lessthan32:
	;; Tail len
	;; Do one more overlap pass
	mov	tmp.b, 0x1f
	vpinsrb	xtmph1x, xtmph1x, tmp.w, 0
	vpbroadcastb xtmph1, xtmph1x	;Construct mask 0x1f1f1f...

	mov	tmp, len		;Overlapped offset length-32

	XLDR	x0, [src+tmp]		;Get next source vector

	XLDR	xd1, [dest1+tmp]	;Get next dest vector
	XLDR	xd2, [dest2+tmp]	;Get next dest vector
	XLDR	xd3, [dest3+tmp]	;Get next dest vector
	XLDR	xd4, [dest4+tmp]	;Get next dest vector
	XLDR	xd5, [dest5+tmp]	;Get next dest vector

	sub	len, pos

	vmovdqa	xtmph2, [constip32]	;Load const of i + 32
	vpinsrb	xtmplx, xtmplx, len.w, 15
	vinserti128	xtmpl, xtmpl, xtmplx, 1 ;swapped to xtmplx | xtmplx
	vpshufb	xtmpl, xtmpl, xtmph1	;Broadcast len to all bytes. xtmph1=0x1f1f1f...
	vpcmpgtb	xtmpl, xtmpl, xtmph2

	vpand	xtmpa, x0, xmask0f	;Mask low src nibble in bits 4-0
	vpsraw	x0, x0, 4		;Shift to put high nibble into bits 4-0
	vpand	x0, x0, xmask0f		;Mask high src nibble in bits 4-0

	vbroadcasti128	xgft1_lo, [tmp3]	;Load array: lo | lo
	vbroadcasti128	xtmph1,   [tmp3+16]	;            hi | hi
	vbroadcasti128	xgft2_lo, [tmp3+tmp4]	;Load array: lo | lo
	vbroadcasti128	xtmph2,   [tmp3+tmp4+16];            hi | hi

	; dest1
	vpshufb	xtmph1, xtmph1, x0		;Lookup mul table of high nibble
	vpshufb	xgft1_lo, xgft1_lo, xtmpa	;Lookup mul table of low nibble
	vpxor	xtmph1, xtmph1, xgft1_lo	;GF add high and low partials
	vpand	xtmph1, xtmph1, xtmpl
	vpxor	xd1, xd1, xtmph1		;xd1 += partial

	vbroadcasti128	xgft3_lo, [tmp3+2*tmp4]
	vbroadcasti128	xtmph1,   [tmp3+2*tmp4+16]

	; dest2
	vpshufb	xtmph2, xtmph2, x0		;Lookup mul table of high nibble
	vpshufb	xgft2_lo, xgft2_lo, xtmpa	;Lookup mul table of low nibble
	vpxor	xtmph2, xtmph2, xgft2_lo	;GF add high and low partials
	vpand	xtmph2, xtmph2, xtmpl
	vpxor	xd2, xd2, xtmph2		;xd2 += partial

	vbroadcasti128	xgft4_lo, [tmp3+tmp5]	;Load array: lo | lo (vec*3)
	vbroadcasti128	xtmph2,   [tmp3+tmp5+16];            hi | hi (vec*3)

	; dest3
	vpshufb	xtmph1, xtmph1, x0		;Lookup mul table of high nibble
	vpshufb	xgft3_lo, xgft3_lo, xtmpa	;Lookup mul table of low nibble
	vpxor	xtmph1, xtmph1, xgft3_lo	;GF add high and low partials
	vpand	xtmph1, xtmph1, xtmpl
	vpxor	xd3, xd3, xtmph1		;xd3 += partial

	vbroadcasti128	xgft5_lo, [tmp3+4*tmp4]
	vbroadcasti128	xtmph1,   [tmp3+4*tmp4+16]

	; dest4
	vpshufb	xtmph2, xtmph2, x0		;Lookup mul table of high nibble
	vpshufb	xgft4_lo, xgft4_lo, xtmpa	;Lookup mul table of low nibble
	vpxor	xtmph2, xtmph2, xgft4_lo	;GF add high and low partials
	vpand	xtmph2, xtmph2, xtmpl
	vpxor	xd4, xd4, xtmph2		;xd4 += partial

	; dest5
	vpshufb	xtmph1, xtmph1, x0		;Lookup mul table of high nibble
	vpshufb	xgft5_lo, xgft5_lo, xtmpa	;Lookup mul table of low nibble
	vpxor	xtmph1, xtmph1, xgft5_lo	;GF add high and low partials
	vpand	xtmph1, xtmph1, xtmpl
	vpxor	xd5, xd5, xtmph1		;xd5 += partial

	XSTR	[dest1+tmp], xd1
	XSTR	[dest2+tmp], xd2
	XSTR	[dest3+tmp], xd3
	XSTR	[dest4+tmp], xd4
	XSTR	[dest5+tmp], xd5

.return_pass:
	FUNC_RESTORE
	mov	return, 0
	ret

.return_fail:
	FUNC_RESTORE
	mov	return, 1
	ret

endproc_frame

section .data
align 32
constip32:
	dq 0xf8f9fafbfcfdfeff, 0xf0f1f2f3f4f5f6f7
	dq 0xe8e9eaebecedeeef, 0xe0e1e2e3e4e5e6e7
