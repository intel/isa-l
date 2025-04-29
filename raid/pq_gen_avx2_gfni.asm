;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;  Copyright(c) 2025 Intel Corporation All rights reserved.
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

;;; Optimized pq of N source vectors using AVX2+GFNI
;;; int pq_gen_avx2_gfni(int vects, int len, void **array)

;;; Generates P+Q parity vector from N (vects-2) sources in array of pointers
;;; (**array).  Last two pointers are the P and Q destinations respectively.
;;; Vectors must be aligned to 64 bytes if NO_NT_LDST is not defined.
;;; Length must be 32 byte multiple.

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
 %define stack_size  1*16 + 8 	; must be an odd multiple of 8
 %define func(x) proc_frame x
 %macro FUNC_SAVE 0
	alloc_stack	stack_size
	vmovdqa	[rsp + 0*16], xmm6
	end_prolog
 %endmacro

 %macro FUNC_RESTORE 0
	vmovdqa	xmm6, [rsp + 0*16]
	add	rsp, stack_size
 %endmacro
%endif

%define vec    arg0
%define len    arg1
%define ptr    arg3
%define pos    rax

%define xp1    ymm0
%define xq1    ymm1
%define xs1    ymm2

%define xp2    ymm3
%define xq2    ymm4
%define xs2    ymm5

%define gfmatrix ymm6

%define xp1x   xmm0
%define xq1x   xmm1
%define xs1x   xmm2

%define gfmatrixy ymm6

%define NO_NT_LDST
;;; Use Non-temporal load/stor
%ifdef NO_NT_LDST
 %define XLDR vmovdqu 		;u8
 %define XSTR vmovdqu
%else
 %define XLDR vmovntdqa
 %define XSTR vmovntdq
%endif

; Matrix with 0x11d as first column
; and identity matrix shited by 1 (as we are multiplying data by 2, mod 0x11d)
; 0 1 0 0 0 0 0 0
; 0 0 1 0 0 0 0 0
; 0 0 0 1 0 0 0 0
; 0 0 0 0 1 0 0 0
; 1 0 0 0 0 1 0 0
; 1 0 0 0 0 0 1 0
; 0 0 0 0 0 0 0 1
; 1 0 0 0 0 0 0 0
default rel
align 32
gf_matrix:
db 0x40, 0x20, 0x10, 0x88, 0x84, 0x82, 0x01, 0x80
db 0x40, 0x20, 0x10, 0x88, 0x84, 0x82, 0x01, 0x80
db 0x40, 0x20, 0x10, 0x88, 0x84, 0x82, 0x01, 0x80
db 0x40, 0x20, 0x10, 0x88, 0x84, 0x82, 0x01, 0x80


[bits 64]
section .text

align 16
mk_global  pq_gen_avx2_gfni, function
func(pq_gen_avx2_gfni)
	FUNC_SAVE
	sub	vec, 3			;Keep as offset to last source
	jng	return_fail		;Must have at least 2 sources
	cmp	len, 0
	je	return_pass
	test	len, (32-1)		;Check alignment of length
	jnz	return_fail

        vmovdqa  gfmatrix, [rel gf_matrix]

	xor	pos, pos
	cmp	len, 64
	jb	loop32

len_aligned_32bytes:
	sub	len, 64		;Len points to last block

loop64:
	mov	ptr, [arg2+vec*8] 	;Fetch last source pointer
	mov	tmp, vec		;Set tmp to point back to last vector
	XLDR	xs1, [ptr+pos]		;Preload last vector (source)
	XLDR	xs2, [ptr+pos+32]	;Preload last vector (source)
	vpxor 	xp1, xp1, xp1		;p1 = 0
	vpxor 	xp2, xp2, xp2		;p2 = 0
	vpxor 	xq1, xq1, xq1		;q1 = 0
	vpxor 	xq2, xq2, xq2		;q2 = 0

next_vect:
	sub	tmp, 1		  	;Inner loop for each source vector
	mov 	ptr, [arg2+tmp*8] 	; get pointer to next vect
	vpxor 	xq1, xq1, xs1		; q1 ^= s1
	vpxor 	xq2, xq2, xs2		; q2 ^= s2
	vpxor 	xp1, xp1, xs1		; p1 ^= s1
	vpxor 	xp2, xp2, xs2		; p2 ^= s2
	XLDR	xs1, [ptr+pos]		; Get next vector (source data1)
	XLDR	xs2, [ptr+pos+32]	; Get next vector (source data2)
        vgf2p8affineqb  xq1, xq1, gfmatrix, 0x00
        vgf2p8affineqb  xq2, xq2, gfmatrix, 0x00
	jg	next_vect		; Loop for each vect except 0

	mov	ptr, [arg2+8+vec*8]	;Get address of P parity vector
	mov	tmp, [arg2+(2*8)+vec*8]	;Get address of Q parity vector
	vpxor 	xp1, xp1, xs1		;p1 ^= s1[0] - last source is already loaded
	vpxor 	xq1, xq1, xs1		;q1 ^= 1 * s1[0]
	vpxor 	xp2, xp2, xs2		;p2 ^= s2[0]
	vpxor 	xq2, xq2, xs2		;q2 ^= 1 * s2[0]
	XSTR	[ptr+pos], xp1		;Write parity P1 vector
	XSTR	[ptr+pos+32], xp2	;Write parity P2 vector
	XSTR	[tmp+pos], xq1		;Write parity Q1 vector
	XSTR	[tmp+pos+32], xq2	;Write parity Q2 vector
	add	pos, 2*32
	cmp	pos, len
	jle	loop64

	;; ------------------------------
	;; Do last 32 or 64 Bytes remaining
	add	len, 2*32
	cmp	pos, len
	je	return_pass

loop32:
	mov 	ptr, [arg2+vec*8] 	;Fetch last source pointer
	mov	tmp, vec		;Set tmp to point back to last vector
	XLDR	xs1, [ptr+pos]		;Preload last vector (source)
	vpxor 	xp1, xp1, xp1	;p = 0
	vpxor 	xq1, xq1, xq1	;q = 0

next_vect32:
	sub	tmp, 1		  	;Inner loop for each source vector
	mov 	ptr, [arg2+tmp*8] 	; get pointer to next vect
	vpxor 	xq1, xq1, xs1	; q1 ^= s1
        vgf2p8affineqb  xq1, xq1, gfmatrix, 0x00
	vpxor 	xp1, xp1, xs1	; p ^= s
	XLDR	xs1, [ptr+pos]		; Get next vector (source data)
	jg	next_vect32		; Loop for each vect except 0

	mov	ptr, [arg2+8+vec*8]	;Get address of P parity vector
	mov	tmp, [arg2+(2*8)+vec*8]	;Get address of Q parity vector
	vpxor 	xp1, xp1, xs1	        ;p ^= s[0] - last source is already loaded
	vpxor 	xq1, xq1, xs1	        ;q ^= 1 * s[0]
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
