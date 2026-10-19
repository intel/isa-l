;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;  Copyright(c) 2011-2026 Intel Corporation All rights reserved.
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
; For a chunk of K bytes x[0..K) that starts with sums a and b,
;	a' = a + sum(x[i])
;	b' = b + K * a + sum((K - i) * x[i])
; The chunk is split into 64 byte blocks. With i = 64 * blk + j,
; K - i = 64 * (blocks after blk) + (64 - j), so
;	sum((K - i) * x[i]) = 64 * sum(bytes before each block) + sum((64 - j) * x[i])
; vpdpbusd with the weights 64..1 gives the second sum, and vpsadbw gives the
; byte sums for the first. Chunks have at most 5504 bytes, so b' fits in 32 bits
; as long as a and b are below BASE (zlib's NMAX bound).

%include "reg_sizes.asm"

%define BASE		0xFFF1 ; 65521
%define BASE_RECIP	0x80078071 ; x / BASE = (x * BASE_RECIP) >> 47 for 32-bit x
%define CHUNK_MAX	5504 ; largest multiple of 64 that is at most NMAX (5552)

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

;; Only zmm16-31, which are volatile in both calling conventions
%define zweights	zmm16
%define zzero		zmm17
%define za		zmm18	; byte sums so far in the chunk, per qword
%define zprefix		zmm19	; with 4 * zgroups, the sum of za before each block
%define zgroups		zmm20	; sum of za before each 256 byte group
%define zb0		zmm21	; dot products with the weights
%define zb1		zmm22
%define zb2		zmm23
%define zb3		zmm24
%define zx0		zmm25
%define zx1		zmm26
%define zx2		zmm27
%define zx3		zmm28
%define zt		zmm29
%define zu		zmm30
%define zp		zmm31

;; %1 = %1 % BASE, for %1 < 2^32. Clobbers tmp2.
%macro MOD_BASE 1
	mov	tmp2 %+ d, BASE_RECIP
	imul	tmp2, %1
	shr	tmp2, 47
	imul	tmp2, tmp2, BASE
	sub	%1, tmp2
%endmacro

;; Horizontal sum of the qwords of %2 into %1. Clobbers zt and zu.
%macro HSUM_Q 2
	vextracti64x4	ymm29, %2, 1
	vpaddq		zmm29, zmm29, %2
	vextracti32x4	xmm30, ymm29, 1
	vpaddq		xmm29, xmm29, xmm30
	vpshufd		xmm30, xmm29, 0x4E
	vpaddq		xmm29, xmm29, xmm30
	vmovq		%1, xmm29
%endmacro

;; Horizontal sum of the dwords of %2 into %1. Clobbers zt and zu.
%macro HSUM_D 2
	vextracti64x4	ymm29, %2, 1
	vpaddd		zmm29, zmm29, %2
	vextracti32x4	xmm30, ymm29, 1
	vpaddd		xmm29, xmm29, xmm30
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

	vmovdqa64	zweights, [weights]
	vpxorq		zzero, zzero, zzero

.chunk_loop:
	;; Whole 64 byte blocks in this chunk
	mov	chunk, size
	and	chunk, -64
	jz	.partial_block
	mov	tmp, CHUNK_MAX
	cmp	chunk, tmp
	cmova	chunk, tmp
	sub	size, chunk

	;; sum_b += chunk * sum_a, before sum_a changes
	mov	tmp, chunk
	imul	tmp, sum_a
	add	sum_b, tmp

	vpxorq	za, za, za
	vpxorq	zprefix, zprefix, zprefix
	vpxorq	zgroups, zgroups, zgroups
	vpxorq	zb0, zb0, zb0
	vpxorq	zb1, zb1, zb1
	vpxorq	zb2, zb2, zb2
	vpxorq	zb3, zb3, zb3

	lea	tmp, [data + chunk]	; end of chunk
	shr	chunk, 8		; 256 byte groups
	jz	.blocks

align 32
.group_loop:
	vmovdqu64	zx0, [data + 0 * 64]
	vmovdqu64	zx1, [data + 1 * 64]
	vmovdqu64	zx2, [data + 2 * 64]
	vmovdqu64	zx3, [data + 3 * 64]
	add	data, 256

	vpdpbusd	zb0, zx0, zweights
	vpdpbusd	zb1, zx1, zweights
	vpdpbusd	zb2, zx2, zweights
	vpdpbusd	zb3, zx3, zweights

	vpsadbw	zx0, zx0, zzero
	vpsadbw	zx1, zx1, zzero
	vpsadbw	zx2, zx2, zzero
	vpsadbw	zx3, zx3, zzero

	;; The four blocks start after 4 * za + 3 * s0 + 2 * s1 + s2 bytes
	vpaddq	zgroups, zgroups, za
	vpaddq	zt, zx0, zx1		; s0 + s1
	vpaddq	zu, zt, zx2		; s0 + s1 + s2
	vpaddq	zp, zt, zx0
	vpaddq	zp, zp, zu
	vpaddq	zprefix, zprefix, zp
	vpaddq	zu, zu, zx3
	vpaddq	za, za, zu

	sub	chunk, 1
	jnz	.group_loop

.blocks:
	cmp	data, tmp
	jae	.reduce

.block_loop:
	vmovdqu64	zx0, [data]
	add	data, 64
	vpdpbusd	zb0, zx0, zweights
	vpsadbw	zx0, zx0, zzero
	vpaddq	zprefix, zprefix, za
	vpaddq	za, za, zx0
	cmp	data, tmp
	jb	.block_loop

.reduce:
	vpsllq	zgroups, zgroups, 2
	vpaddq	zprefix, zprefix, zgroups
	HSUM_Q	tmp, zprefix
	shl	tmp, 6
	add	sum_b, tmp

	vpaddd	zb0, zb0, zb1
	vpaddd	zb2, zb2, zb3
	vpaddd	zb0, zb0, zb2
	HSUM_D	tmp %+ d, zb0
	add	sum_b, tmp

	HSUM_Q	tmp, za
	add	sum_a, tmp

	MOD_BASE sum_a
	MOD_BASE sum_b
	jmp	.chunk_loop

.partial_block:
	;; 0 to 63 bytes left. Load them into the low bytes of a block; the
	;; weights then count each byte 64 - size times too often.
	test	size, size
	jz	.done
	mov	tmp, -1
	bzhi	tmp, tmp, size
	kmovq	k1, tmp
	vmovdqu8	zx0 {k1}{z}, [data]

	vpxorq		zb0, zb0, zb0
	vpdpbusd	zb0, zx0, zweights
	vpsadbw		zx0, zx0, zzero

	mov	tmp, size
	imul	tmp, sum_a
	add	sum_b, tmp		; sum_b += size * sum_a

	HSUM_D	tmp %+ d, zb0
	add	sum_b, tmp
	HSUM_Q	tmp, zx0
	add	sum_a, tmp
	mov	chunk, 64
	sub	chunk, size
	imul	chunk, tmp
	sub	sum_b, chunk	; remove the (64 - size) * sum(x) extra

	MOD_BASE sum_a
	MOD_BASE sum_b

.done:
	mov	eax, sum_b %+ d
	shl	eax, 16
	or	eax, sum_a %+ d
	vzeroupper
	ret

section .data
align 64
weights:
%assign w 64
%rep 64
	db	w
%assign w (w - 1)
%endrep
