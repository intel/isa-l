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
%include "reg_sizes.asm"

; Functional versions of CRC macros

%include "igzip_buffer_utils_04.asm"

extern fold_4

%define crc_0		xmm0	; in/out: crc state
%define crc_1		xmm1	; in/out: crc state
%define crc_2		xmm2	; in/out: crc state
%define crc_3		xmm3	; in/out: crc state
%define crc_fold	xmm4	; in:	(loaded from fold_4)
%define crc_tmp0	xmm5	; tmp
%define crc_tmp1	xmm6	; tmp
%define crc_tmp2	xmm7	; tmp
%define crc_tmp3	xmm8	; tmp
%define crc_tmp4	xmm9	; tmp
%define tmp4		rax

; copy x bytes (rounded up to 16 bytes) from src to dst with crc
; src & dst are unaligned
; void copy_in_crc(uint8_t *dst, uint8_t *src, uint32_t size, uint32_t *crc)
; arg 1: rcx: pointer to dst
; arg 2: rdx: pointer to src
; arg 3: r8:  size (in bytes)
; arg 4: r9:  pointer to CRC
;; %if 0
global copy_in_crc_04
copy_in_crc_04:
%ifidn __OUTPUT_FORMAT__, elf64
	mov	r9, rcx
	mov	r8, rdx
	mov	rdx, rsi
	mov	rcx, rdi
%endif

	; Save xmm registers that need to be preserved.
	sub	rsp, 8 + 4*16
	vmovdqa	[rsp+0*16], xmm6
	vmovdqa	[rsp+1*16], xmm7
	vmovdqa	[rsp+2*16], xmm8
	vmovdqa	[rsp+3*16], xmm9

	vmovdqa	crc_0, [r9 + 0*16]
	vmovdqa	crc_1, [r9 + 1*16]
	vmovdqa	crc_2, [r9 + 2*16]
	vmovdqa	crc_3, [r9 + 3*16]

	vmovdqa	crc_fold, [fold_4 WRT_OPT]
	COPY_IN_CRC	rcx, rdx, r8, tmp4, crc_0, crc_1, crc_2, crc_3, \
			crc_fold, \
			crc_tmp0, crc_tmp1, crc_tmp2, crc_tmp3, crc_tmp4

	vmovdqa	[r9 + 0*16], crc_0
	vmovdqa	[r9 + 1*16], crc_1
	vmovdqa	[r9 + 2*16], crc_2
	vmovdqa	[r9 + 3*16], crc_3

	vmovdqa	xmm9, [rsp+3*16]
	vmovdqa	xmm8, [rsp+2*16]
	vmovdqa	xmm7, [rsp+1*16]
	vmovdqa	xmm6, [rsp+0*16]
	add	rsp, 8 + 4*16
ret

; Convert 512-bit CRC data to real 32-bit value
; uint32_t crc_512to32(uint32_t *crc)
; arg 1:   rcx: pointer to CRC
; returns: eax: 32 bit crc
global crc_512to32_04
crc_512to32_04:
%ifidn __OUTPUT_FORMAT__, elf64
	mov	rcx, rdi
%endif

	vmovdqa	crc_0, [rcx + 0*16]
	vmovdqa	crc_1, [rcx + 1*16]
	vmovdqa	crc_2, [rcx + 2*16]
	vmovdqa	crc_3, [rcx + 3*16]

	vmovdqa crc_fold, [rk1 WRT_OPT]		;k1

	; fold the 4 xmm registers to 1 xmm register with different constants
	vmovdqa crc_tmp0, crc_0
	vpclmulqdq crc_0, crc_fold, 0x1
	vpclmulqdq crc_tmp0, crc_fold, 0x10
	vpxor crc_1, crc_tmp0
	vpxor crc_1, crc_0

	vmovdqa crc_tmp0, crc_1
	vpclmulqdq crc_1, crc_fold, 0x1
	vpclmulqdq crc_tmp0, crc_fold, 0x10
	vpxor crc_2, crc_tmp0
	vpxor crc_2, crc_1

	vmovdqa crc_tmp0, crc_2
	vpclmulqdq crc_2, crc_fold, 0x1
	vpclmulqdq crc_tmp0, crc_fold, 0x10
	vpxor crc_3, crc_tmp0
	vpxor crc_3, crc_2


	vmovdqa crc_fold, [rk5 WRT_OPT]
	vmovdqa crc_0, crc_3

	vpclmulqdq crc_3, crc_fold, 0

	vpsrldq crc_0, 8

	vpxor crc_3, crc_0

	vmovdqa crc_0, crc_3


	vpslldq crc_3, 4

	vpclmulqdq crc_3, crc_fold, 0x10


	vpxor crc_3, crc_0

	vpand crc_3, [mask2 WRT_OPT]

	vmovdqa crc_1, crc_3

	vmovdqa crc_2, crc_3

	vmovdqa crc_fold, [rk7 WRT_OPT]

	vpclmulqdq crc_3, crc_fold, 0
	vpxor crc_3, crc_2

	vpand crc_3, [mask WRT_OPT]

	vmovdqa crc_2, crc_3

	vpclmulqdq crc_3, crc_fold, 0x10

	vpxor crc_3, crc_2

	vpxor crc_3, crc_1

	vpextrd eax, crc_3, 2

	not eax

	ret
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;


section .data

align 16

rk1:   dq 0x00000000ccaa009e
rk2:   dq 0x00000001751997d0
rk5:   dq 0x00000000ccaa009e
rk6:   dq 0x0000000163cd6124
rk7:   dq 0x00000001f7011640
rk8:   dq 0x00000001db710640

mask:	dq 0xFFFFFFFFFFFFFFFF, 0x0000000000000000
mask2:	dq 0xFFFFFFFF00000000, 0xFFFFFFFFFFFFFFFF
