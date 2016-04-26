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

;;; Optimized pq of N source vectors using SSE3
;;; int pq_gen_sse(int vects, int len, void **array)

;;; Generates P+Q parity vector from N (vects-2) sources in array of pointers
;;; (**array).  Last two pointers are the P and Q destinations respectively.
;;; Vectors must be aligned to 16 bytes.  Length must be 16 byte aligned.

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
 %define func(x) x:
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
 %define stack_size  7*16 + 8 	; must be an odd multiple of 8
 %define func(x) proc_frame x
 %macro FUNC_SAVE 0
	alloc_stack	stack_size
	save_xmm128	xmm6, 0*16
	save_xmm128	xmm7, 1*16
	save_xmm128	xmm8, 2*16
	save_xmm128	xmm9, 3*16
	save_xmm128	xmm10, 4*16
	save_xmm128	xmm11, 5*16
	save_xmm128	xmm15, 6*16
	end_prolog
 %endmacro

 %macro FUNC_RESTORE 0
	movdqa	xmm6, [rsp + 0*16]
	movdqa	xmm7, [rsp + 1*16]
	movdqa	xmm8, [rsp + 2*16]
	movdqa	xmm9, [rsp + 3*16]
	movdqa	xmm10, [rsp + 4*16]
	movdqa	xmm11, [rsp + 5*16]
	movdqa	xmm15, [rsp + 6*16]
	add	rsp, stack_size
 %endmacro
%endif

%define vec arg0
%define	len arg1
%define ptr arg3
%define pos rax

%define xp1   xmm0
%define xq1   xmm1
%define xtmp1 xmm2
%define xs1   xmm3

%define xp2   xmm4
%define xq2   xmm5
%define xtmp2 xmm6
%define xs2   xmm7

%define xp3   xmm8
%define xq3   xmm9
%define xtmp3 xmm10
%define xs3   xmm11

%define xpoly xmm15

;;; Use Non-temporal load/stor
%ifdef NO_NT_LDST
 %define XLDR movdqa
 %define XSTR movdqa
%else
 %define XLDR movntdqa
 %define XSTR movntdq
%endif

default rel

[bits 64]
section .text

align 16
global pq_gen_sse:function
func(pq_gen_sse)
	FUNC_SAVE
	sub	vec, 3			;Keep as offset to last source
	jng	return_fail		;Must have at least 2 sources
	cmp	len, 0
	je	return_pass
	test	len, (16-1)		;Check alignment of length
	jnz	return_fail
	mov	pos, 0
	movdqa	xpoly, [poly]
	cmp	len, 48
	jl	loop16

len_aligned_32bytes:
	sub	len, 48			;Len points to last block

loop48:
	mov 	ptr, [arg2+vec*8] 	;Fetch last source pointer
	mov	tmp, vec		;Set tmp to point back to last vector
	XLDR	xs1, [ptr+pos]		;Preload last vector (source)
	XLDR	xs2, [ptr+pos+16]	;Preload last vector (source)
	XLDR	xs3, [ptr+pos+32]	;Preload last vector (source)
	pxor	xp1, xp1		;p1 = 0
	pxor	xp2, xp2		;p2 = 0
	pxor	xp3, xp3		;p3 = 0
	pxor	xq1, xq1		;q1 = 0
	pxor	xq2, xq2		;q2 = 0
	pxor	xq3, xq3		;q3 = 0

next_vect:
	sub	tmp, 1		  	;Inner loop for each source vector
	mov 	ptr, [arg2+tmp*8] 	; get pointer to next vect
	pxor	xq1, xs1		; q1 ^= s1
	pxor	xq2, xs2		; q2 ^= s2
	pxor	xq3, xs3		; q3 ^= s3
	pxor	xp1, xs1		; p1 ^= s1
	pxor	xp2, xs2		; p2 ^= s2
	pxor	xp3, xs3		; p3 ^= s2
	pxor	xtmp1, xtmp1		; xtmp1 = 0 - for compare to 0
	pxor	xtmp2, xtmp2		; xtmp2 = 0
	pxor	xtmp3, xtmp3		; xtmp3 = 0
	pcmpgtb	xtmp1, xq1		; xtmp1 = mask 0xff or 0x00 if bit7 set
	pcmpgtb	xtmp2, xq2		; xtmp2 = mask 0xff or 0x00 if bit7 set
	pcmpgtb	xtmp3, xq3		; xtmp3 = mask 0xff or 0x00 if bit7 set
	pand	xtmp1, xpoly		; xtmp1 = poly or 0x00
	pand	xtmp2, xpoly		; xtmp2 = poly or 0x00
	pand	xtmp3, xpoly		; xtmp3 = poly or 0x00
	XLDR	xs1, [ptr+pos]		; Get next vector (source data1)
	XLDR	xs2, [ptr+pos+16]	; Get next vector (source data2)
	XLDR	xs3, [ptr+pos+32]	; Get next vector (source data3)
	paddb	xq1, xq1		; q1 = q1<<1
	paddb	xq2, xq2		; q2 = q2<<1
	paddb	xq3, xq3		; q3 = q3<<1
	pxor	xq1, xtmp1		; q1 = q1<<1 ^ poly_masked
	pxor	xq2, xtmp2		; q2 = q2<<1 ^ poly_masked
	pxor	xq3, xtmp3		; q3 = q3<<1 ^ poly_masked
	jg	next_vect		; Loop for each vect except 0

	mov	ptr, [arg2+8+vec*8]	;Get address of P parity vector
	mov	tmp, [arg2+(2*8)+vec*8]	;Get address of Q parity vector
	pxor	xp1, xs1		;p1 ^= s1[0] - last source is already loaded
	pxor	xq1, xs1		;q1 ^= 1 * s1[0]
	pxor	xp2, xs2		;p2 ^= s2[0]
	pxor	xq2, xs2		;q2 ^= 1 * s2[0]
	pxor	xp3, xs3		;p3 ^= s3[0]
	pxor	xq3, xs3		;q3 ^= 1 * s3[0]
	XSTR	[ptr+pos], xp1		;Write parity P1 vector
	XSTR	[ptr+pos+16], xp2	;Write parity P2 vector
	XSTR	[ptr+pos+32], xp3	;Write parity P3 vector
	XSTR	[tmp+pos], xq1		;Write parity Q1 vector
	XSTR	[tmp+pos+16], xq2	;Write parity Q2 vector
	XSTR	[tmp+pos+32], xq3	;Write parity Q3 vector
	add	pos, 48
	cmp	pos, len
	jle	loop48

	;; ------------------------------
	;; Do last 16 or 32 Bytes remaining
	add	len, 48
	cmp	pos, len
	je	return_pass

loop16:
	mov 	ptr, [arg2+vec*8] 	;Fetch last source pointer
	mov	tmp, vec		;Set tmp to point back to last vector
	XLDR	xs1, [ptr+pos]		;Preload last vector (source)
	pxor	xp1, xp1		;p = 0
	pxor	xq1, xq1		;q = 0

next_vect16:
	sub	tmp, 1		  	;Inner loop for each source vector
	mov 	ptr, [arg2+tmp*8] 	; get pointer to next vect
	pxor	xq1, xs1		; q1 ^= s1
	pxor	xtmp1, xtmp1		; xtmp = 0
	pcmpgtb	xtmp1, xq1		; xtmp = mask 0xff or 0x00 if bit7 set
	pand	xtmp1, xpoly		; xtmp = poly or 0x00
	pxor	xp1, xs1		; p ^= s
	paddb	xq1, xq1		; q = q<<1
	pxor	xq1, xtmp1		; q = q<<1 ^ poly_masked
	XLDR	xs1, [ptr+pos]		; Get next vector (source data)
	jg	next_vect16		; Loop for each vect except 0

	mov	ptr, [arg2+8+vec*8]	;Get address of P parity vector
	mov	tmp, [arg2+(2*8)+vec*8]	;Get address of Q parity vector
	pxor	xp1, xs1		;p ^= s[0] - last source is already loaded
	pxor	xq1, xs1		;q ^= 1 * s[0]
	XSTR	[ptr+pos], xp1		;Write parity P vector
	XSTR	[tmp+pos], xq1		;Write parity Q vector
	add	pos, 16
	cmp	pos, len
	jl	loop16


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

align 16
poly:
dq 0x1d1d1d1d1d1d1d1d, 0x1d1d1d1d1d1d1d1d

;;;       func        core, ver, snum
slversion pq_gen_sse, 00,   09,  0032
