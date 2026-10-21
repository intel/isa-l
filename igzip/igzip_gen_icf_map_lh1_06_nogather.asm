;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;  Copyright(c) 2011-2018 Intel Corporation All rights reserved.
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

%include "reg_sizes.asm"
%include "lz0a_const.asm"
%include "data_struct2.asm"
%include "huffman.asm"


%define USE_HSWNI
%define ARCH 06

%ifidn __OUTPUT_FORMAT__, win64
%define arg1 rcx
%define arg2 rdx
%define arg3 r8
%define hash rsi
%define next_in rdi
%else
%define arg1 rdi
%define arg2 rsi
%define arg3 rdx
%define hash r8
%define next_in rcx
%endif

%define stream arg1
%define level_buf arg1
%define matches_next arg2
%define f_i_end arg3

%define f_i rax
%define file_start rbp
%define tmp r9
%define tmp2 r10
%define prev_len r11
%define prev_dist r12
%define f_i_orig r13
%define sbuf rbx
%define tmp3 r14
%define tmp4 r15

%define hash_table level_buf + _hash_map_hash_table

;; Results of each stage kept for the next stage
%define zold zmm0
%define zold_next zmm16
%define zmatch0 zmm1
%define zmatch1 zmm2
%define zmatch2 zmm3
%define zmatch3 zmm4
%define zdcode zmm5
%define zoffs zmm6

;; Temporaries
%define ztmp0 zmm7
%define ztmp1 zmm8
%define ztmp2 zmm9
%define ztmp3 zmm10
%define ztmp4 zmm11
%define ztmp5 zmm12
%define ztmp6 zmm13
%define ztmp7 zmm14

;; The 8 byte lengths, 16 byte lengths and distance codes of the vector before
%define zprev_len zmm15
%define zprev_len16 zmm17
%define zprev_dcode zmm18

%define zdist_mask zmm19
%define zthirty zmm20
%define zwindow_lo zmm21
%define zwindow_hi zmm22
%define zpack1 zmm23
%define zpack2 zmm24
%define zhash_prod zmm25
%define zhash_mask zmm26
%define zincrement zmm27
%define zdatas_perm zmm28
%define zones zmm29
%define zdatas_shuf zmm30
%define zbswap zmm31

;; Scratch space for the hashes of two vectors, and the hash table entries and look back
;; offsets of a vector
%define SBUF_HASHES	0
%define SBUF_ENTRIES	128
%define SBUF_OFFSETS	192
%define SBUF_SIZE	256

;; The hashes of the next vector for STAGE_A, which STAGE_H fills the other half from
%define hslot hash

%ifidn __OUTPUT_FORMAT__, win64
%define stack_size  10*16 + 8 * 8 + 8 + SBUF_SIZE + 128
%define sbuf_offset 10*16 + 8 * 8 + 8
%define func(x) proc_frame x

%macro FUNC_SAVE 0
	alloc_stack	stack_size
	vmovdqa	[rsp + 0*16], xmm6
	vmovdqa	[rsp + 1*16], xmm7
	vmovdqa	[rsp + 2*16], xmm8
	vmovdqa	[rsp + 3*16], xmm9
	vmovdqa	[rsp + 4*16], xmm10
	vmovdqa	[rsp + 5*16], xmm11
	vmovdqa	[rsp + 6*16], xmm12
	vmovdqa	[rsp + 7*16], xmm13
	vmovdqa	[rsp + 8*16], xmm14
	vmovdqa	[rsp + 9*16], xmm15
	save_reg	rsi, 10*16 + 0*8
	save_reg	rdi, 10*16 + 1*8
	save_reg	rbp, 10*16 + 2*8
	save_reg	r12, 10*16 + 3*8
	save_reg	r13, 10*16 + 4*8
	save_reg	rbx, 10*16 + 5*8
	save_reg	r14, 10*16 + 6*8
	save_reg	r15, 10*16 + 7*8
	end_prolog
%endm

%macro FUNC_RESTORE 0
	vmovdqa	xmm6, [rsp + 0*16]
	vmovdqa	xmm7, [rsp + 1*16]
	vmovdqa	xmm8, [rsp + 2*16]
	vmovdqa	xmm9, [rsp + 3*16]
	vmovdqa	xmm10, [rsp + 4*16]
	vmovdqa	xmm11, [rsp + 5*16]
	vmovdqa	xmm12, [rsp + 6*16]
	vmovdqa	xmm13, [rsp + 7*16]
	vmovdqa	xmm14, [rsp + 8*16]
	vmovdqa	xmm15, [rsp + 9*16]

	mov	rsi, [rsp + 10*16 + 0*8]
	mov	rdi, [rsp + 10*16 + 1*8]
	mov	rbp, [rsp + 10*16 + 2*8]
	mov	r12, [rsp + 10*16 + 3*8]
	mov	r13, [rsp + 10*16 + 4*8]
	mov	rbx, [rsp + 10*16 + 5*8]
	mov	r14, [rsp + 10*16 + 6*8]
	mov	r15, [rsp + 10*16 + 7*8]
	add	rsp, stack_size
%endm
%else
%define stack_size  SBUF_SIZE + 128 + 8
%define sbuf_offset 0
%define func(x) x: endbranch
%macro FUNC_SAVE 0
	push	rbp
	push	r12
	push	r13
	push	rbx
	push	r14
	push	r15
	sub	rsp, stack_size
%endm

%macro FUNC_RESTORE 0
	add	rsp, stack_size
	pop	r15
	pop	r14
	pop	rbx
	pop	r13
	pop	r12
	pop	rbp
%endm
%endif

;; The vectors of 16 positions go through four stages, each a vector behind the one before,
;; so that the loads of each stage finish before the next stage needs them. STAGE_H hashes a
;; vector, STAGE_A looks up and updates the hash table, STAGE_B finds the look back distances
;; and loads the data there, and STAGE_C finds the match lengths and writes the match map.
;; Gathers and scatters are slow on some processors, so the hash table and the look back data
;; are accessed with scalar loads and stores.

;; Hashes the vector at f_i + %1 to the half of the hashes STAGE_A does not use, and sets k7
;; to the positions whose hash is one less than the hash of an earlier position. A 4 byte
;; store of such a position would put back the entry after its own. Hashes 2k and 2k + 1 have
;; the same hash / 2, and hashes 2k - 1 and 2k the same (hash + 1) / 2.
%macro STAGE_H 1
	vmovdqu64 ztmp0 %+ y, [f_i + file_start + %1]
	vpermq	ztmp0, zdatas_perm, ztmp0
	vpshufb	ztmp0, ztmp0, zdatas_shuf
	vpmaddwd ztmp0, ztmp0, zhash_prod
	vpmaddwd ztmp0, ztmp0, zhash_prod
	vpandd	ztmp0, ztmp0, zhash_mask
	mov	tmp, hslot
	xor	tmp, 64
	vmovdqa64 [tmp], ztmp0

	vpsrld	ztmp1, ztmp0, 1
	vpaddd	ztmp2, ztmp0, zones
	vpsrld	ztmp2, ztmp2, 1
	vpconflictd ztmp1, ztmp1
	vpconflictd ztmp2, ztmp2
	vptestmd k3, ztmp0, zones
	kmovw	tmp %+ d, k3
	vpbroadcastd ztmp3, tmp %+ d
	knotw	k4, k3
	vptestmd k4 {k4}, ztmp1, ztmp3
	vpandnd	ztmp2, ztmp3, ztmp2
	vptestmd k5 {k3}, ztmp2, ztmp2
	korw	k7, k4, k5
%endm

;; Sets zold_next to the hash table entries at the hashes from STAGE_H of the vector at
;; f_i + %1, whose k7 is now in k6, and stores the positions to the hash table. All 16 entries
;; are looked up before any is stored, and the stores leave the entries a 4 byte gather and
;; scatter of the 2 byte entries would leave.
%macro STAGE_A 1
	;; Look up the entries, four to a qword
%assign %%i 0
%rep 4
	mov	tmp %+ d, [hslot + 16 * %%i]
	mov	tmp2 %+ d, [hslot + 16 * %%i + 4]
	movzx	tmp %+ d, word [hash_table + HASH_BYTES * tmp]
	movzx	tmp2 %+ d, word [hash_table + HASH_BYTES * tmp2]
	shl	tmp2 %+ d, 16
	or	tmp %+ d, tmp2 %+ d
	mov	tmp2 %+ d, [hslot + 16 * %%i + 8]
	mov	tmp3 %+ d, [hslot + 16 * %%i + 12]
	movzx	tmp2 %+ d, word [hash_table + HASH_BYTES * tmp2]
	movzx	tmp3 %+ d, word [hash_table + HASH_BYTES * tmp3]
	shl	tmp3 %+ d, 16
	or	tmp2 %+ d, tmp3 %+ d
	shl	tmp2, 32
	or	tmp, tmp2
%if %%i == 0
	vmovq	ztmp4 %+ x, tmp
%elif %%i == 1
	vmovq	ztmp5 %+ x, tmp
%elif %%i == 2
	vmovq	ztmp6 %+ x, tmp
%else
	vmovq	ztmp7 %+ x, tmp
%endif
%assign %%i (%%i + 1)
%endrep
	vpunpcklqdq ztmp4 %+ x, ztmp4 %+ x, ztmp5 %+ x
	vpunpcklqdq ztmp6 %+ x, ztmp6 %+ x, ztmp7 %+ x
	vinserti128 ztmp4 %+ y, ztmp4 %+ y, ztmp6 %+ x, 1
	vpmovzxwd zold_next, ztmp4 %+ y

	kortestw k6, k6
	jnz	%%store_dwords

	;; Otherwise storing the 2 byte positions in order leaves the same entries
	lea	tmp2 %+ d, [f_i + %1]
%assign %%i 0
%rep 16
	mov	tmp %+ d, [hslot + 4 * %%i]
	lea	tmp3 %+ d, [tmp2 + %%i]
	mov	[hash_table + HASH_BYTES * tmp], tmp3 %+ w
%assign %%i (%%i + 1)
%endrep
	jmp	%%end

%%store_dwords:
	;; Store each position with the entry after it as it was before the stores
	xor	tmp4, tmp4
%%load_dword:
	mov	tmp %+ d, [hslot + 4 * tmp4]
	mov	tmp3 %+ d, [hash_table + HASH_BYTES * tmp]
	mov	[sbuf + SBUF_ENTRIES + 4 * tmp4], tmp3 %+ d
	add	tmp4, 1
	cmp	tmp4, VECT_SIZE
	jb	%%load_dword

	xor	tmp4, tmp4
	lea	tmp2 %+ d, [f_i + %1]
%%store_dword:
	mov	tmp %+ d, [hslot + 4 * tmp4]
	mov	tmp3 %+ d, [sbuf + SBUF_ENTRIES + 4 * tmp4]
	mov	tmp3 %+ w, tmp2 %+ w
	mov	[hash_table + HASH_BYTES * tmp], tmp3 %+ d
	add	tmp2, 1
	add	tmp4, 1
	cmp	tmp4, VECT_SIZE
	jb	%%store_dword
%%end:
%endm

;; Finds the look back distances of the vector at f_i + %1 from zold, and sets zoffs to the
;; offsets of the matches, zdcode to their distance codes, and zmatch0 to zmatch3 to the 16
;; bytes at them: zmatch<k> holds the matches of positions k, k + 4, k + 8 and k + 12.
%macro STAGE_B 1
	lea	tmp %+ d, [f_i + %1]
	vpbroadcastd ztmp0, tmp %+ d
	vpaddd	ztmp0, ztmp0, zincrement
	vpaddd	ztmp1, zold, zones
	vpsubd	ztmp1, ztmp0, ztmp1
	vpandd	ztmp1, ztmp1, zdist_mask
	vpsubd	zoffs, zincrement, ztmp1
	vpsubd	zoffs, zoffs, zones
	vmovdqa64 [sbuf + SBUF_OFFSETS], zoffs

	;; Calculate dist_icf_code from the distances - 1
	vpcmpgtd k5, ztmp1, zones
	vplzcntd ztmp2, ztmp1
	vpsubd	ztmp2 {k5}{z}, zthirty, ztmp2
	vpsllvd	ztmp3, zones, ztmp2
	vpsubd	ztmp3, ztmp3, zones
	vpandd	ztmp3 {k5}{z}, ztmp1, ztmp3
	vpsrlvd	ztmp1, ztmp1, ztmp2
	vpslld	ztmp2, ztmp2, 1
	vpaddd	ztmp1, ztmp1, ztmp2
	vpslld	ztmp3, ztmp3, EXTRA_BITS_OFFSET - DIST_OFFSET
	vpaddd	ztmp1, ztmp1, ztmp3
	vpslld	zdcode, ztmp1, DIST_OFFSET

	;; Load the 16 bytes at each match
	lea	next_in, [f_i + file_start + %1]
	LOAD_MATCH zmatch0, 0
	LOAD_MATCH zmatch1, 1
	LOAD_MATCH zmatch2, 2
	LOAD_MATCH zmatch3, 3
%endm

;; Loads the 16 bytes at the matches of positions %2, %2 + 4, %2 + 8 and %2 + 12 to %1
%macro LOAD_MATCH 2
	movsxd	tmp, dword [sbuf + SBUF_OFFSETS + 4 * %2]
	movsxd	tmp2, dword [sbuf + SBUF_OFFSETS + 4 * (%2 + 4)]
	movsxd	tmp3, dword [sbuf + SBUF_OFFSETS + 4 * (%2 + 8)]
	movsxd	tmp4, dword [sbuf + SBUF_OFFSETS + 4 * (%2 + 12)]
	vmovdqu	%1 %+ x, [next_in + tmp]
	vinserti32x4 %1, %1, [next_in + tmp2], 1
	vinserti32x4 %1, %1, [next_in + tmp3], 2
	vinserti32x4 %1, %1, [next_in + tmp4], 3
%endm

;; Sets the even qwords of %1 to 8 times the number of bytes that the 16 bytes in each lane of
;; %1 and %2 have in common at the start
%macro MATCH_LEN16 3
	vpxorq	%1, %1, %2
	vpshufb	%1, %1, zbswap
	vplzcntq %1, %1
	vpshufd	%3, %1, 0x4e
	vpcmpeqq k3, %1, [sixtyfour]{1to8}
	vpaddq	%1 {k3}, %1, %3
%endm

;; Finds the match lengths of the vector at f_i, up to 16 bytes, and writes the match map entries
;; of the positions one before. Each match is taken unless the next position has a longer one,
;; comparing lengths of at most 8 bytes, and gets its length of up to 16 bytes.
%macro STAGE_C 0
	;; Lane k of the windows has the 16 bytes at position 4 * k + s for s from 0 to 3
	vmovdqu64 ztmp0, [f_i + file_start]
	vpermd	ztmp1, zwindow_lo, ztmp0
	vpermd	ztmp2, zwindow_hi, ztmp0
	vpalignr ztmp4, ztmp2, ztmp1, 1
	vpalignr ztmp5, ztmp2, ztmp1, 2
	vpalignr ztmp6, ztmp2, ztmp1, 3

	MATCH_LEN16 ztmp1, zmatch0, ztmp7
	MATCH_LEN16 ztmp4, zmatch1, ztmp7
	MATCH_LEN16 ztmp5, zmatch2, ztmp7
	MATCH_LEN16 ztmp6, zmatch3, ztmp7

	;; Put the lengths in position order
	vpermt2d ztmp1, zpack1, ztmp4
	vpermt2d ztmp5, zpack1, ztmp6
	vpermt2d ztmp1, zpack2, ztmp5
	vpsrld	ztmp1, ztmp1, 3
	vpminud	ztmp0, ztmp1, [eight]{1to16}

	;; Zero out matches which should not be taken, using the values of the position before
	valignd	ztmp2, ztmp0, zprev_len, 15
	valignd	ztmp3, ztmp1, zprev_len16, 15
	valignd	ztmp4, zdcode, zprev_dcode, 15
	vmovdqa64 zprev_len, ztmp0
	vmovdqa64 zprev_len16, ztmp1
	vmovdqa64 zprev_dcode, zdcode

	vpcmpgtd k3, ztmp2, [shortest_matches]{1to16}
	vpcmpgtd k4, ztmp0, ztmp2

	knotw	k3, k3
	korw	k3, k3, k4
	knotw	k4, k3
	vmovdqu32 ztmp3 {k4}{z}, ztmp3

	;; Update the distance codes to match the lengths
	vpaddd	ztmp4, ztmp4, ztmp3
	vpaddd	ztmp4, ztmp4, [twofiftyfour]{1to16}
	vpmovzxbd ztmp4 {k3}, [f_i + file_start - 1]
	vpaddd	ztmp4 {k3}, ztmp4, [null_dist_syms]{1to16}

	vmovdqu64 [matches_next], ztmp4
	add	matches_next, ICF_CODE_BYTES * VECT_SIZE
%endm

%define VECT_SIZE 16
%define HASH_BYTES 2

[bits 64]
default rel
section .text

global gen_icf_map_lh1_06_nogather
func(gen_icf_map_lh1_06_nogather)
	endbranch
	FUNC_SAVE

	lea	sbuf, [rsp + sbuf_offset + 127]
	and	sbuf, -128

	mov	file_start, [stream + _next_in]
	mov	f_i %+ d, dword [stream + _total_in]
	mov	f_i_orig, f_i

	sub	file_start, f_i
	add	f_i_end, f_i
	cmp	f_i, f_i_end
	jge	end_main

;; Prep for main loop
	vpbroadcastd zdist_mask, dword [stream + _internal_state_dist_mask]
	vpbroadcastd zhash_mask, dword [stream + _internal_state_hash_mask]
	mov	tmp, stream
	mov	level_buf, [stream + _level_buf]
	sub	f_i_end, LA
	vmovdqu64 zdatas_perm, [datas_perm]
	vbroadcasti32x8 zdatas_shuf, [datas_shuf]
	vpbroadcastd zhash_prod, [hash_prod]
	vmovdqu64 zincrement, [increment]
	vpbroadcastd zones, [ones]
	vbroadcasti32x4 zbswap, [bswap_shuf]
	vpbroadcastd zthirty, [thirty]
	vmovdqu64 zwindow_lo, [window_lo]
	vmovdqu64 zwindow_hi, [window_hi]
	vmovdqu64 zpack1, [pack1]
	vmovdqu64 zpack2, [pack2]

;; Process first byte
	vmovd	ztmp0 %+ x, dword [f_i + file_start]
	vpmaddwd ztmp0, ztmp0, zhash_prod
	vpmaddwd ztmp0, ztmp0, zhash_prod
	vpandd	ztmp0, ztmp0, zhash_mask
	vmovd	hash %+ d, ztmp0 %+ x

	cmp	byte [tmp + _internal_state_has_hist], IGZIP_NO_HIST
	jne	.has_hist
	;; No history, the byte is a literal
	xor	prev_len, prev_len
	xor	prev_dist, prev_dist
	xor	tmp3, tmp3
	mov	byte [tmp + _internal_state_has_hist], IGZIP_HIST
	jmp .byte_processed

.has_hist:
	;; History exists, need to set prev_len and prev_dist accordingly
	lea	next_in, [f_i + file_start]

	;; Determine match lookback distance
	xor	tmp, tmp
	mov	tmp %+ w, f_i %+ w
	dec	tmp
	sub	tmp %+ w, word [hash_table + HASH_BYTES * hash]

	vmovd	tmp2 %+ d, zdist_mask %+ x
	and	tmp %+ d, tmp2 %+ d
	neg	tmp

	;; Check first 16 bytes of match
	mov	prev_len, [next_in]
	xor	prev_len, [next_in + tmp - 1]
	mov	tmp3, [next_in + 8]
	xor	tmp3, [next_in + tmp - 1 + 8]
	neg	tmp

	;; Set prev_dist
%ifidn arg1, rcx
	mov	tmp2, rcx
%endif
	;; The third register is unused on Haswell and later,
	;; This line will not work on previous architectures
	get_dist_icf_code tmp, prev_dist, tmp

%ifidn arg1, rcx
	mov	rcx, tmp2
%endif

	;; Set prev_len, and the 16 byte length in tmp3
	xor	tmp2, tmp2
	tzcnt	prev_len, prev_len
	shr	prev_len, 3
	cmp	prev_len, MIN_DEF_MATCH
	cmovl	prev_len, tmp2
	tzcnt	tmp3, tmp3
	shr	tmp3, 3
	add	tmp3, 8
	cmp	prev_len, 8
	cmovne	tmp3, prev_len

.byte_processed:
	vpbroadcastd zprev_len, prev_len %+ d
	vpbroadcastd zprev_len16, tmp3 %+ d
	vpbroadcastd zprev_dcode, prev_dist %+ d
	mov	word [hash_table + HASH_BYTES * hash], f_i %+ w

	add	f_i, 1
	cmp	f_i, f_i_end
	jg	end_main

;; Find the start of the last vector
	sub	f_i_end, VECT_SIZE
	mov	tmp, f_i_end
	sub	tmp, f_i
	and	tmp, -VECT_SIZE
	lea	tmp, [f_i + tmp + VECT_SIZE]
	cmp	f_i, f_i_end
	cmovg	tmp, f_i
	mov	f_i_end, tmp

	lea	hslot, [sbuf + SBUF_HASHES + 64]
	STAGE_H	0
	kmovw	k6, k7
	xor	hslot, 64
	STAGE_A	0
	vmovdqa64 zold, zold_next
	cmp	f_i, f_i_end
	je	.last_vect_b

	STAGE_H	VECT_SIZE
	kmovw	k6, k7
	xor	hslot, 64
	STAGE_A	VECT_SIZE
	STAGE_B	0
	vmovdqa64 zold, zold_next
	STAGE_H	2 * VECT_SIZE
	kmovw	k6, k7
	xor	hslot, 64

	sub	f_i_end, 2 * VECT_SIZE
	cmp	f_i, f_i_end
	jg	.loop1_end

.loop1:
	STAGE_H	3 * VECT_SIZE
	STAGE_A	2 * VECT_SIZE
	STAGE_C
	STAGE_B	VECT_SIZE
	vmovdqa64 zold, zold_next
	kmovw	k6, k7
	xor	hslot, 64
	add	f_i, VECT_SIZE
	cmp	f_i, f_i_end
	jle	.loop1

.loop1_end:
	STAGE_C
	STAGE_B	VECT_SIZE
	add	f_i, VECT_SIZE
	jmp	.last_vect_c

.last_vect_b:
	STAGE_B	0

.last_vect_c:
	STAGE_C

;; Restore last update hash value
	vextracti32x4 ztmp0 %+ x, zoffs, 3
	vpextrd tmp %+ d, ztmp0 %+ x, 3
	add	tmp %+ d, f_i %+ d

	vmovd	ztmp0 %+ x, dword [f_i + file_start + VECT_SIZE - 1]
	vpmaddwd ztmp0 %+ x, ztmp0 %+ x, zhash_prod %+ x
	vpmaddwd ztmp0 %+ x, ztmp0 %+ x, zhash_prod %+ x
	vpandd	ztmp0 %+ x, ztmp0 %+ x, zhash_mask %+ x
	vmovd	hash %+ d, ztmp0 %+ x

	mov	word [hash_table + HASH_BYTES * hash], tmp %+ w

	add	f_i, VECT_SIZE

end_main:
	sub	f_i, f_i_orig
	sub	f_i, 1
%ifnidn f_i, rax
	mov	rax, f_i
%endif
	FUNC_RESTORE
	ret

endproc_frame

section .data
align 64
;; 64 byte data
datas_perm:
	dq 0x0, 0x1, 0x0, 0x1, 0x1, 0x2, 0x1, 0x2
window_lo:
	dd 0x0, 0x1, 0x2, 0x3, 0x1, 0x2, 0x3, 0x4
	dd 0x2, 0x3, 0x4, 0x5, 0x3, 0x4, 0x5, 0x6
window_hi:
	dd 0x4, 0x5, 0x6, 0x7, 0x5, 0x6, 0x7, 0x8
	dd 0x6, 0x7, 0x8, 0x9, 0x7, 0x8, 0x9, 0xa
pack1:
	dd 0x00, 0x10, 0x04, 0x14, 0x08, 0x18, 0x0c, 0x1c
	dd 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
pack2:
	dd 0x00, 0x01, 0x10, 0x11, 0x02, 0x03, 0x12, 0x13
	dd 0x04, 0x05, 0x14, 0x15, 0x06, 0x07, 0x16, 0x17
datas_shuf:
	db 0x0, 0x1, 0x2, 0x3
	db 0x1, 0x2, 0x3, 0x4
	db 0x2, 0x3, 0x4, 0x5
	db 0x3, 0x4, 0x5, 0x6
	db 0x4, 0x5, 0x6, 0x7
	db 0x5, 0x6, 0x7, 0x8
	db 0x6, 0x7, 0x8, 0x9
	db 0x7, 0x8, 0x9, 0xa
increment:
	dd 0x0, 0x1, 0x2, 0x3, 0x4, 0x5, 0x6, 0x7
	dd 0x8, 0x9, 0xa, 0xb, 0xc, 0xd, 0xe, 0xf

;; 16 byte data
bswap_shuf:
	db 0x07, 0x06, 0x05, 0x04, 0x03, 0x02, 0x01, 0x00
	db 0x0f, 0x0e, 0x0d, 0x0c, 0x0b, 0x0a, 0x09, 0x08
;; 4 byte data
null_dist_syms:
	dd LIT
%define PROD1 0xE84B
%define PROD2 0x97B1
hash_prod:
	dw PROD1, PROD2
ones:
	dd 0x1
thirty:
	dd 0x1e
twofiftyfour:
	dd 0xfe
shortest_matches:
	dd MIN_DEF_MATCH
eight:
	dd 8
;; 8 byte data
sixtyfour:
	dq 64
