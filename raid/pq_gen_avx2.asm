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

;;; Optimized pq of N source vectors using AVX
;;; int pq_gen_avx(int vects, int len, void **array)

;;; Generates P+Q parity vector from N (vects-2) sources in array of pointers
;;; (**array).  Last two pointers are the P and Q destinations respectively.
;;; Vectors must be aligned to 32 bytes.  Length must be 32 byte aligned.

%include "reg_sizes.asm"

%ifidn __OUTPUT_FORMAT__, elf64
 %define arg0  rdi
 %define arg1  rsi
 %define arg2  rdx
 %define arg3  rcx
 %define arg4  r8
 %define arg5  r9
 %define tmp   r11
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
 %define tmp3  r10
 %define return rax
 %define stack_size  8*32 + 8 	; must be an odd multiple of 8
 %define func(x) proc_frame x
 %macro FUNC_SAVE 0
	alloc_stack	stack_size
	;; Until a sav_ymm256 is defined
	vmovdqu	[rsp + 0*32], ymm6
	vmovdqu	[rsp + 1*32], ymm7
	vmovdqu	[rsp + 2*32], ymm8
	vmovdqu	[rsp + 3*32], ymm9
	vmovdqu	[rsp + 4*32], ymm10
	vmovdqu	[rsp + 5*32], ymm11
	vmovdqu	[rsp + 6*32], ymm14
	vmovdqu	[rsp + 7*32], ymm15
	end_prolog
 %endmacro

 %macro FUNC_RESTORE 0
	vmovdqu	ymm6, [rsp + 0*32]
	vmovdqu	ymm7, [rsp + 1*32]
	vmovdqu	ymm8, [rsp + 2*32]
	vmovdqu	ymm9, [rsp + 3*32]
	vmovdqu	ymm10, [rsp + 4*32]
	vmovdqu	ymm11, [rsp + 5*32]
	vmovdqu	ymm14, [rsp + 6*32]
	vmovdqu	ymm15, [rsp + 7*32]
	add	rsp, stack_size
 %endmacro
%endif

%define vec arg0
%define	len arg1
%define ptr arg3
%define pos rax

%define xp1   ymm0
%define xq1   ymm1
%define xtmp1 ymm2
%define xs1   ymm3

%define xp2   ymm4
%define xq2   ymm5
%define xtmp2 ymm6
%define xs2   ymm7

%define xp3   ymm8
%define xq3   ymm9
%define xtmp3 ymm10
%define xs3   ymm11

%define xzero ymm14
%define xpoly ymm15

;;; Use Non-temporal load/stor
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

align 16
mk_global  pq_gen_avx2, function
func(pq_gen_avx2)
	FUNC_SAVE
	sub	vec, 3			;Keep as offset to last source
	jng	return_fail		;Must have at least 2 sources
	cmp	len, 0
	je	return_pass
	test	len, (32-1)		;Check alignment of length
	jnz	return_fail
	mov	pos, 0
	vmovdqa	xpoly, [poly]
	vpxor	xzero, xzero, xzero
	cmp	len, 96
	jl	loop32

len_aligned_32bytes:
	sub	len, 3*32		;Len points to last block

loop96:
	mov	ptr, [arg2+vec*8] 	;Fetch last source pointer
	mov	tmp, vec		;Set tmp to point back to last vector
	XLDR	xs1, [ptr+pos]		;Preload last vector (source)
	XLDR	xs2, [ptr+pos+32]	;Preload last vector (source)
	XLDR	xs3, [ptr+pos+64]	;Preload last vector (source)
	vpxor	xp1, xp1, xp1		;p1 = 0
	vpxor	xp2, xp2, xp2		;p2 = 0
	vpxor	xp3, xp3, xp3		;p3 = 0
	vpxor	xq1, xq1, xq1		;q1 = 0
	vpxor	xq2, xq2, xq2		;q2 = 0
	vpxor	xq3, xq3, xq3		;q3 = 0

next_vect:
	sub	tmp, 1		  	;Inner loop for each source vector
	mov 	ptr, [arg2+tmp*8] 	; get pointer to next vect
	vpxor	xq1, xq1, xs1		; q1 ^= s1
	vpxor	xq2, xq2, xs2		; q2 ^= s2
	vpxor	xq3, xq3, xs3		; q3 ^= s3
	vpxor	xp1, xp1, xs1		; p1 ^= s1
	vpxor	xp2, xp2, xs2		; p2 ^= s2
	vpxor	xp3, xp3, xs3		; p3 ^= s2
	vpblendvb xtmp1, xzero, xpoly, xq1 ; xtmp1 = poly or 0x00
	vpblendvb xtmp2, xzero, xpoly, xq2 ; xtmp2 = poly or 0x00
	vpblendvb xtmp3, xzero, xpoly, xq3 ; xtmp3 = poly or 0x00
	XLDR	xs1, [ptr+pos]		; Get next vector (source data1)
	XLDR	xs2, [ptr+pos+32]	; Get next vector (source data2)
	XLDR	xs3, [ptr+pos+64]	; Get next vector (source data3)
	vpaddb	xq1, xq1, xq1		; q1 = q1<<1
	vpaddb	xq2, xq2, xq2		; q2 = q2<<1
	vpaddb	xq3, xq3, xq3		; q3 = q3<<1
	vpxor	xq1, xq1, xtmp1		; q1 = q1<<1 ^ poly_masked
	vpxor	xq2, xq2, xtmp2		; q2 = q2<<1 ^ poly_masked
	vpxor	xq3, xq3, xtmp3		; q3 = q3<<1 ^ poly_masked
	jg	next_vect		; Loop for each vect except 0

	mov	ptr, [arg2+8+vec*8]	;Get address of P parity vector
	mov	tmp, [arg2+(2*8)+vec*8]	;Get address of Q parity vector
	vpxor	xp1, xp1, xs1		;p1 ^= s1[0] - last source is already loaded
	vpxor	xq1, xq1, xs1		;q1 ^= 1 * s1[0]
	vpxor	xp2, xp2, xs2		;p2 ^= s2[0]
	vpxor	xq2, xq2, xs2		;q2 ^= 1 * s2[0]
	vpxor	xp3, xp3, xs3		;p3 ^= s3[0]
	vpxor	xq3, xq3, xs3		;q3 ^= 1 * s3[0]
	XSTR	[ptr+pos], xp1		;Write parity P1 vector
	XSTR	[ptr+pos+32], xp2	;Write parity P2 vector
	XSTR	[ptr+pos+64], xp3	;Write parity P3 vector
	XSTR	[tmp+pos], xq1		;Write parity Q1 vector
	XSTR	[tmp+pos+32], xq2	;Write parity Q2 vector
	XSTR	[tmp+pos+64], xq3	;Write parity Q3 vector
	add	pos, 3*32
	cmp	pos, len
	jle	loop96

	;; ------------------------------
	;; Do last 16 or 32 Bytes remaining
	add	len, 3*32
	cmp	pos, len
	je	return_pass

loop32:
	mov 	ptr, [arg2+vec*8] 	;Fetch last source pointer
	mov	tmp, vec		;Set tmp to point back to last vector
	XLDR	xs1, [ptr+pos]		;Preload last vector (source)
	vpxor	xp1, xp1, xp1		;p = 0
	vpxor	xq1, xq1, xq1		;q = 0

next_vect32:
	sub	tmp, 1		  	;Inner loop for each source vector
	mov 	ptr, [arg2+tmp*8] 	; get pointer to next vect
	vpxor	xq1, xq1, xs1		; q1 ^= s1
	vpblendvb xtmp1, xzero, xpoly, xq1 ; xtmp1 = poly or 0x00
	vpxor	xp1, xp1, xs1		; p ^= s
	vpaddb	xq1, xq1, xq1		; q = q<<1
	vpxor	xq1, xq1, xtmp1		; q = q<<1 ^ poly_masked
	XLDR	xs1, [ptr+pos]		; Get next vector (source data)
	jg	next_vect32		; Loop for each vect except 0

	mov	ptr, [arg2+8+vec*8]	;Get address of P parity vector
	mov	tmp, [arg2+(2*8)+vec*8]	;Get address of Q parity vector
	vpxor	xp1, xp1, xs1		;p ^= s[0] - last source is already loaded
	vpxor	xq1, xq1, xs1		;q ^= 1 * s[0]
	XSTR	[ptr+pos], xp1		;Write parity P vector
	XSTR	[tmp+pos], xq1		;Write parity Q vector
	add	pos, 32
	cmp	pos, len
	jl	loop32


return_pass:
	mov	return, 0
	FUNC_RESTORE
	ret

return_fail:
	mov	return, 1
	FUNC_RESTORE
	ret

endproc_frame

section .data

align 32
poly:
dq 0x1d1d1d1d1d1d1d1d, 0x1d1d1d1d1d1d1d1d
dq 0x1d1d1d1d1d1d1d1d, 0x1d1d1d1d1d1d1d1d

;;;       func        core,  ver, snum
slversion pq_gen_avx2, 04,   03,  0041
