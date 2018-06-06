;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;  Copyright(c) 2011-2016 Intel Corporation All rights reserved.
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
%include "stdmac.asm"

%ifndef UTILS_ASM
%define UTILS_ASM
; compare macro

;; sttni2 is faster, but it can't be debugged
;; so following code is based on "mine5"

;; compare 258 bytes = 8 * 32 + 2
;; tmp16 is a 16-bit version of tmp
;; compare258 src1, src2, result, tmp
%macro compare258 4
%define %%src1		%1
%define %%src2		%2
%define %%result	%3
%define %%tmp		%4
%define %%tmp16		%4w	; tmp as a 16-bit register

	xor	%%result, %%result
%%loop1:
	mov	%%tmp, [%%src1 + %%result]
	xor	%%tmp, [%%src2 + %%result]
	jnz	%%miscompare
	add	%%result, 8

	mov	%%tmp, [%%src1 + %%result]
	xor	%%tmp, [%%src2 + %%result]
	jnz	%%miscompare
	add	%%result, 8

	cmp	%%result, 256
	jb	%%loop1

	; compare last two bytes
	mov	%%tmp16, [%%src1 + %%result]
	xor	%%tmp16, [%%src2 + %%result]
	jnz	%%miscompare16

	; no miscompares, return 258
	add	%%result, 2
	jmp	%%end

%%miscompare16:
	and	%%tmp, 0xFFFF
%%miscompare:
	bsf	%%tmp, %%tmp
	shr	%%tmp, 3
	add	%%result, %%tmp
%%end:
%endm

;; compare 258 bytes = 8 * 32 + 2
;; tmp16 is a 16-bit version of tmp
;; compare258 src1, src2, result, tmp
%macro compare250_r 4
%define %%src1		%1
%define %%src2		%2
%define %%result	%3
%define %%tmp		%4
%define %%tmp16		%4w	; tmp as a 16-bit register

	mov	%%result, 8
	mov	%%tmp, [%%src1 + 8]
	xor	%%tmp, [%%src2 + 8]
	jnz	%%miscompare
	add	%%result, 8

%%loop1:
	mov	%%tmp, [%%src1 + %%result]
	xor	%%tmp, [%%src2 + %%result]
	jnz	%%miscompare
	add	%%result, 8

	mov	%%tmp, [%%src1 + %%result]
	xor	%%tmp, [%%src2 + %%result]
	jnz	%%miscompare
	add	%%result, 8

	cmp	%%result, 256
	jb	%%loop1

	; compare last two bytes
	mov	%%tmp16, [%%src1 + %%result]
	xor	%%tmp16, [%%src2 + %%result]
	jnz	%%miscompare16

	; no miscompares, return 258
	add	%%result, 2
	jmp	%%end

%%miscompare16:
	and	%%tmp, 0xFFFF
%%miscompare:
	bsf	%%tmp, %%tmp
	shr	%%tmp, 3
	add	%%result, %%tmp
%%end:
%endm

;; compare 258 bytes = 8 * 32 + 2
;; compares 16 bytes at a time, using pcmpeqb/pmovmskb
;; compare258_x src1, src2, result, tmp, xtmp1, xtmp2
%macro compare258_x 6
%define %%src1		%1
%define %%src2		%2
%define %%result	%3
%define %%tmp		%4
%define %%tmp32		%4d
%define %%tmp16		%4w	; tmp as a 16-bit register
%define %%xtmp		%5
%define %%xtmp2		%6

	xor	%%result, %%result
%%loop1:
	MOVDQU		%%xtmp, [%%src1 + %%result]
	MOVDQU		%%xtmp2, [%%src2 + %%result]
	PCMPEQB		%%xtmp, %%xtmp, %%xtmp2
	PMOVMSKB	%%tmp32, %%xtmp
	xor		%%tmp, 0xFFFF
	jnz		%%miscompare
	add		%%result, 16

	MOVDQU		%%xtmp, [%%src1 + %%result]
	MOVDQU		%%xtmp2, [%%src2 + %%result]
	PCMPEQB		%%xtmp, %%xtmp, %%xtmp2
	PMOVMSKB	%%tmp32, %%xtmp
	xor		%%tmp, 0xFFFF
	jnz		%%miscompare
	add		%%result, 16

	cmp	%%result, 256
	jb	%%loop1

	; compare last two bytes
	mov	%%tmp16, [%%src1 + %%result]
	xor	%%tmp16, [%%src2 + %%result]
	jnz	%%miscompare16

	; no miscompares, return 258
	add	%%result, 2
	jmp	%%end

%%miscompare16:
	and	%%tmp, 0xFFFF
	bsf	%%tmp, %%tmp
	shr	%%tmp, 3
	add	%%result, %%tmp
	jmp	%%end
%%miscompare:
	bsf	%%tmp, %%tmp
	add	%%result, %%tmp
%%end:
%endm

;; compare 258 bytes = 8 * 32 + 2, assuming first 8 bytes
;; were already checked
;; compares 16 bytes at a time, using pcmpeqb/pmovmskb
;; compare250_x src1, src2, result, tmp, xtmp1, xtmp2
%macro compare250_x 6
%define %%src1		%1
%define %%src2		%2
%define %%result	%3
%define %%tmp		%4
%define %%tmp32		%4d	; tmp as a 16-bit register
%define %%xtmp		%5
%define %%xtmp2		%6

	mov	%%result, 8
	MOVDQU		%%xtmp, [%%src1 + 8]
	MOVDQU		%%xtmp2, [%%src2 + 8]
	PCMPEQB		%%xtmp, %%xtmp, %%xtmp2
	PMOVMSKB	%%tmp32, %%xtmp
	xor		%%tmp, 0xFFFF
	jnz		%%miscompare
	add		%%result, 16
%%loop1:
	MOVDQU		%%xtmp, [%%src1 + %%result]
	MOVDQU		%%xtmp2, [%%src2 + %%result]
	PCMPEQB		%%xtmp, %%xtmp, %%xtmp2
	PMOVMSKB	%%tmp32, %%xtmp
	xor		%%tmp, 0xFFFF
	jnz		%%miscompare
	add		%%result, 16

	MOVDQU		%%xtmp, [%%src1 + %%result]
	MOVDQU		%%xtmp2, [%%src2 + %%result]
	PCMPEQB		%%xtmp, %%xtmp, %%xtmp2
	PMOVMSKB	%%tmp32, %%xtmp
	xor		%%tmp, 0xFFFF
	jnz		%%miscompare
	add		%%result, 16

	cmp	%%result, 258 - 16
	jb	%%loop1

	MOVDQU		%%xtmp, [%%src1 + %%result]
	MOVDQU		%%xtmp2, [%%src2 + %%result]
	PCMPEQB		%%xtmp, %%xtmp, %%xtmp2
	PMOVMSKB	%%tmp32, %%xtmp
	xor		%%tmp, 0xFFFF
	jnz		%%miscompare_last
	; no miscompares, return 258
	mov		%%result, 258
	jmp	%%end

%%miscompare_last:
	bsf	%%tmp, %%tmp
	add	%%result, %%tmp

	;; Guarantee the result has length at most 258.
	mov	%%tmp, 258
	cmp	%%result, 258
	cmova	%%result, %%tmp
	jmp	%%end
%%miscompare:
	bsf	%%tmp, %%tmp
	add	%%result, %%tmp
%%end:
%endm

;; compare 258 bytes = 8 * 32 + 2
;; compares 32 bytes at a time, using pcmpeqb/pmovmskb
;; compare258_y src1, src2, result, tmp, xtmp1, xtmp2
%macro compare258_y 6
%define %%src1		%1
%define %%src2		%2
%define %%result	%3
%define %%tmp		%4
%define %%tmp16		%4w	; tmp as a 16-bit register
%define %%tmp32		%4d	; tmp as a 32-bit register
%define %%ytmp		%5
%define %%ytmp2		%6

	xor	%%result, %%result
%%loop1:
	vmovdqu		%%ytmp, [%%src1 + %%result]
	vmovdqu		%%ytmp2, [%%src2 + %%result]
	vpcmpeqb	%%ytmp, %%ytmp, %%ytmp2
	vpmovmskb	%%tmp, %%ytmp
	xor		%%tmp32, 0xFFFFFFFF
	jnz		%%miscompare
	add		%%result, 32

	vmovdqu		%%ytmp, [%%src1 + %%result]
	vmovdqu		%%ytmp2, [%%src2 + %%result]
	vpcmpeqb	%%ytmp, %%ytmp, %%ytmp2
	vpmovmskb	%%tmp, %%ytmp
	xor		%%tmp32, 0xFFFFFFFF
	jnz		%%miscompare
	add		%%result, 32

	cmp	%%result, 256
	jb	%%loop1

	; compare last two bytes
	mov	%%tmp16, [%%src1 + %%result]
	xor	%%tmp16, [%%src2 + %%result]
	jnz	%%miscompare16

	; no miscompares, return 258
	add	%%result, 2
	jmp	%%end

%%miscompare16:
	and	%%tmp, 0xFFFF
	bsf	%%tmp, %%tmp
	shr	%%tmp, 3
	add	%%result, %%tmp
	jmp	%%end
%%miscompare:
	bsf	%%tmp, %%tmp
	add	%%result, %%tmp
%%end:
%endm


;; compare 258 bytes = 8 * 32 + 2, assuming first 8 bytes
;; were already checked
;; compares 32 bytes at a time, using pcmpeqb/pmovmskb
;; compare258_y src1, src2, result, tmp, xtmp1, xtmp2
%macro compare250_y 6
%define %%src1		%1
%define %%src2		%2
%define %%result	%3
%define %%tmp		%4
%define %%tmp16		%4w	; tmp as a 16-bit register
%define %%tmp32		%4d	; tmp as a 32-bit register
%define %%ytmp		%5
%define %%ytmp2		%6

	mov	%%result, 8
	vmovdqu		%%ytmp, [%%src1 + 8]
	vmovdqu		%%ytmp2, [%%src2 + 8]
	vpcmpeqb	%%ytmp, %%ytmp, %%ytmp2
	vpmovmskb	%%tmp, %%ytmp
	xor		%%tmp32, 0xFFFFFFFF
	jnz		%%miscompare
	add		%%result, 32
%%loop1:
	vmovdqu		%%ytmp, [%%src1 + %%result]
	vmovdqu		%%ytmp2, [%%src2 + %%result]
	vpcmpeqb	%%ytmp, %%ytmp, %%ytmp2
	vpmovmskb	%%tmp, %%ytmp
	xor		%%tmp32, 0xFFFFFFFF
	jnz		%%miscompare
	add		%%result, 32

	vmovdqu		%%ytmp, [%%src1 + %%result]
	vmovdqu		%%ytmp2, [%%src2 + %%result]
	vpcmpeqb	%%ytmp, %%ytmp, %%ytmp2
	vpmovmskb	%%tmp, %%ytmp
	xor		%%tmp32, 0xFFFFFFFF
	jnz		%%miscompare
	add		%%result, 32

	cmp	%%result, 258 - 32
	jb	%%loop1

	vmovdqu		%%ytmp, [%%src1 + %%result]
	vmovdqu		%%ytmp2, [%%src2 + %%result]
	vpcmpeqb	%%ytmp, %%ytmp, %%ytmp2
	vpmovmskb	%%tmp, %%ytmp
	xor		%%tmp32, 0xFFFFFFFF
	jnz		%%miscompare_last
	mov	%%result, 258
	jmp	%%end

%%miscompare_last:
	bsf	%%tmp, %%tmp
	add	%%result, %%tmp

	;; Guarantee the result has length at most 258.
	mov	%%tmp, 258
	cmp	%%result, 258
	cmova	%%result, %%tmp
	jmp	%%end

%%miscompare:
	bsf	%%tmp, %%tmp
	add	%%result, %%tmp
%%end:
%endm

%macro compare250 6
%define %%src1		%1
%define %%src2		%2
%define %%result	%3
%define %%tmp		%4
%define %%xtmp0		%5x
%define %%xtmp1		%6x
%define %%ytmp0		%5
%define %%ytmp1		%6

%if (COMPARE_TYPE == 1)
	compare250_r	%%src1, %%src2, %%result, %%tmp
%elif (COMPARE_TYPE == 2)
	compare250_x	%%src1, %%src2, %%result, %%tmp, %%xtmp0, %%xtmp1
%elif (COMPARE_TYPE == 3)
	compare250_y	%%src1, %%src2, %%result, %%tmp, %%ytmp0, %%ytmp1
%else
%error Unknown Compare type COMPARE_TYPE
 % error
%endif
%endmacro


;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

;; compare size, src1, src2, result, tmp
%macro compare 5
%define %%size		%1
%define %%src1		%2
%define %%src2		%3
%define %%result	%4
%define %%tmp		%5
%define %%tmp8		%5b	; tmp as a 8-bit register

	xor	%%result, %%result
	sub	%%size, 7
	jle	%%lab2
%%loop1:
	mov	%%tmp, [%%src1 + %%result]
	xor	%%tmp, [%%src2 + %%result]
	jnz	%%miscompare
	add	%%result, 8
	sub	%%size, 8
	jg	%%loop1
%%lab2:
	;; if we fall through from above, we have found no mismatches,
	;; %%size+7 is the number of bytes left to look at, and %%result is the
	;; number of bytes that have matched
	add	%%size, 7
	jle	%%end
%%loop3:
	mov	%%tmp8, [%%src1 + %%result]
	cmp	%%tmp8, [%%src2 + %%result]
	jne	%%end
	inc	%%result
	dec	%%size
	jg	%%loop3
	jmp	%%end
%%miscompare:
	bsf	%%tmp, %%tmp
	shr	%%tmp, 3
	add	%%result, %%tmp
%%end:
%endm

%endif	;UTILS_ASM
