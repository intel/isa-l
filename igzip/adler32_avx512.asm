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

; uint32_t adler32_avx512(uint32_t init, const unsigned char *buf, uint64_t len)
;
; The vectors are 256 bits wide: on Ice Lake and Granite Rapids, 512-bit
; vpdpbusd lowers the core clock for a few milliseconds, which slows the
; deflate and inflate code between calls by more than wider vectors save.
;
; For a chunk of K bytes x[0..K) that starts with sums a and b,
;	a' = a + sum(x[i])
;	b' = b + K * a + sum((K - i) * x[i])
; The chunk is split into 32 byte blocks. With i = 32 * blk + j,
; K - i = 32 * (blocks after blk) + (32 - j), so
;	sum((K - i) * x[i]) = 32 * sum(bytes before each block) + sum((32 - j) * x[i])
; vpdpbusd with the weights 32..1 gives the second sum, and vpsadbw gives the
; byte sums for the first. Chunks have at most 5504 bytes, so b' fits in 32 bits
; as long as a and b are below BASE (zlib's NMAX bound).

%include "reg_sizes.asm"

%define BASE		0xFFF1 ; 65521
%define BASE_RECIP	0x80078071 ; x / BASE = (x * BASE_RECIP) >> 47 for 32-bit x
%define CHUNK_MAX	5504 ; largest multiple of 128 that is at most NMAX (5552)

%ifidn __OUTPUT_FORMAT__, elf64
 %define arg1	rdi
 %define arg2	rsi
 %define arg3	rdx
%else
 %define arg1	rcx
 %define arg2	rdx
 %define arg3	r8
%endif

%define data	r10
%define size	r11
%define sum_a	r8
%define sum_b	r9
%define chunk	rcx
%define tmp	rax
%define tmp2	rdx

;; Only ymm16-31, which are volatile in both calling conventions. Writing them
;; with EVEX instructions also zeroes bits 511:256.
%define yweights	ymm16
%define yzero		ymm17
%define ya		ymm18	; byte sums so far in the chunk, per qword
%define yprefix		ymm19	; with 4 * ygroups, the sum of ya before each block
%define ygroups		ymm20	; sum of ya before each 128 byte group
%define yb0		ymm21	; dot products with the weights
%define yb1		ymm22
%define yb2		ymm23
%define yb3		ymm24
%define yx0		ymm25
%define yx1		ymm26
%define yx2		ymm27
%define yx3		ymm28
%define yt		ymm29
%define yu		ymm30
%define yp		ymm31

;; %1 = %1 % BASE, for %1 < 2^32. Clobbers tmp2.
%macro MOD_BASE 1
	mov	tmp2 %+ d, BASE_RECIP
	imul	tmp2, %1
	shr	tmp2, 47
	imul	tmp2, tmp2, BASE
	sub	%1, tmp2
%endmacro

;; Horizontal sum of the qwords of ymm%2 into %1. Clobbers yt and yu.
%macro HSUM_Q 2
	vextracti32x4	xmm29, ymm%2, 1
	vpaddq		xmm29, xmm29, xmm%2
	vpshufd		xmm30, xmm29, 0x4E
	vpaddq		xmm29, xmm29, xmm30
	vmovq		%1, xmm29
%endmacro

;; Horizontal sum of the dwords of ymm%2 into %1. Clobbers yt and yu.
%macro HSUM_D 2
	vextracti32x4	xmm29, ymm%2, 1
	vpaddd		xmm29, xmm29, xmm%2
	vpshufd		xmm30, xmm29, 0x4E
	vpaddd		xmm29, xmm29, xmm30
	vpshufd		xmm30, xmm29, 0xB1
	vpaddd		xmm29, xmm29, xmm30
	vmovd		%1, xmm29
%endmacro

[bits 64]
default rel
section .text

mk_global adler32_avx512, function
adler32_avx512:
	endbranch
	mov	data, arg2
	mov	size, arg3
	mov	tmp %+ d, arg1 %+ d

	;; The 16-bit halves can be up to 0xFFFF, reduce them below BASE
	movzx	sum_a, tmp %+ w
	shr	tmp %+ d, 16
	mov	sum_b, tmp
	lea	tmp, [sum_a - BASE]
	cmp	sum_a, BASE
	cmovae	sum_a, tmp
	lea	tmp, [sum_b - BASE]
	cmp	sum_b, BASE
	cmovae	sum_b, tmp

	vmovdqa64	yweights, [weights]
	vpxorq		yzero, yzero, yzero

.chunk_loop:
	;; Whole 32 byte blocks in this chunk
	mov	chunk, size
	and	chunk, -32
	jz	.partial_block
	mov	tmp, CHUNK_MAX
	cmp	chunk, tmp
	cmova	chunk, tmp
	sub	size, chunk

	;; sum_b += chunk * sum_a, before sum_a changes
	mov	tmp, chunk
	imul	tmp, sum_a
	add	sum_b, tmp

	vpxorq	ya, ya, ya
	vpxorq	yprefix, yprefix, yprefix
	vpxorq	ygroups, ygroups, ygroups
	vpxorq	yb0, yb0, yb0
	vpxorq	yb1, yb1, yb1
	vpxorq	yb2, yb2, yb2
	vpxorq	yb3, yb3, yb3

	lea	tmp, [data + chunk]	; end of chunk
	shr	chunk, 7		; 128 byte groups
	jz	.blocks

align 32
.group_loop:
	vmovdqu64	yx0, [data + 0 * 32]
	vmovdqu64	yx1, [data + 1 * 32]
	vmovdqu64	yx2, [data + 2 * 32]
	vmovdqu64	yx3, [data + 3 * 32]
	add	data, 128

	vpdpbusd	yb0, yx0, yweights
	vpdpbusd	yb1, yx1, yweights
	vpdpbusd	yb2, yx2, yweights
	vpdpbusd	yb3, yx3, yweights

	vpsadbw	yx0, yx0, yzero
	vpsadbw	yx1, yx1, yzero
	vpsadbw	yx2, yx2, yzero
	vpsadbw	yx3, yx3, yzero

	;; The four blocks start after 4 * ya + 3 * s0 + 2 * s1 + s2 bytes
	vpaddq	ygroups, ygroups, ya
	vpaddq	yt, yx0, yx1		; s0 + s1
	vpaddq	yu, yt, yx2		; s0 + s1 + s2
	vpaddq	yp, yt, yx0
	vpaddq	yp, yp, yu
	vpaddq	yprefix, yprefix, yp
	vpaddq	yu, yu, yx3
	vpaddq	ya, ya, yu

	sub	chunk, 1
	jnz	.group_loop

.blocks:
	cmp	data, tmp
	jae	.reduce

.block_loop:
	vmovdqu64	yx0, [data]
	add	data, 32
	vpdpbusd	yb0, yx0, yweights
	vpsadbw	yx0, yx0, yzero
	vpaddq	yprefix, yprefix, ya
	vpaddq	ya, ya, yx0
	cmp	data, tmp
	jb	.block_loop

.reduce:
	vpsllq	ygroups, ygroups, 2
	vpaddq	yprefix, yprefix, ygroups
	HSUM_Q	tmp, 19
	shl	tmp, 5
	add	sum_b, tmp

	vpaddd	yb0, yb0, yb1
	vpaddd	yb2, yb2, yb3
	vpaddd	yb0, yb0, yb2
	HSUM_D	tmp %+ d, 21
	add	sum_b, tmp

	HSUM_Q	tmp, 18
	add	sum_a, tmp

	MOD_BASE sum_a
	MOD_BASE sum_b
	jmp	.chunk_loop

.partial_block:
	;; 0 to 31 bytes left. Load them into the low bytes of a block; the
	;; weights then count each byte 32 - size times too often.
	test	size, size
	jz	.done
	mov	tmp, -1
	bzhi	tmp, tmp, size
	kmovd	k1, tmp %+ d
	vmovdqu8	yx0 {k1}{z}, [data]

	vpxorq		yb0, yb0, yb0
	vpdpbusd	yb0, yx0, yweights
	vpsadbw		yx0, yx0, yzero

	mov	tmp, size
	imul	tmp, sum_a
	add	sum_b, tmp		; sum_b += size * sum_a

	HSUM_D	tmp %+ d, 21
	add	sum_b, tmp
	HSUM_Q	tmp, 25
	add	sum_a, tmp
	mov	chunk, 32
	sub	chunk, size
	imul	chunk, tmp
	sub	sum_b, chunk	; remove the (32 - size) * sum(x) extra

	MOD_BASE sum_a
	MOD_BASE sum_b

.done:
	mov	eax, sum_b %+ d
	shl	eax, 16
	or	eax, sum_a %+ d
	vzeroupper
	ret

section .data
align 32
weights:
%assign w 32
%rep 32
	db	w
%assign w (w - 1)
%endrep
