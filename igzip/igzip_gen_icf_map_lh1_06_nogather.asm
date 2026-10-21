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
%define zmatch1 zmm1
%define zmatch2 zmm2
%define zdcode zmm3
%define zoffs zmm4

;; Temporaries
%define ztmp0 zmm5
%define ztmp1 zmm6
%define ztmp2 zmm7
%define ztmp3 zmm8
%define ztmp4 zmm9
%define ztmp5 zmm10
%define ztmp6 zmm11
%define ztmp7 zmm12

%define znull_dist_syms zmm15
%define zthirty zmm17
%define zdist_mask zmm18
%define zshortest_matches zmm19
%define zrot_left zmm20
%define zdatas_perm zmm21
%define zdatas_perm2 zmm22
%define zdatas_perm3 zmm23
%define zdatas_shuf zmm24
%define zhash_prod zmm25
%define zhash_mask zmm26
%define zincrement zmm27
%define zqword_shuf zmm28
%define zones zmm29
%define ztwofiftyfour zmm30
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
%define stack_size  8*16 + 8 * 8 + 8 + SBUF_SIZE + 128
%define sbuf_offset 8*16 + 8 * 8 + 8
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
	vmovdqa	[rsp + 7*16], xmm15
	save_reg	rsi, 8*16 + 0*8
	save_reg	rdi, 8*16 + 1*8
	save_reg	rbp, 8*16 + 2*8
	save_reg	r12, 8*16 + 3*8
	save_reg	r13, 8*16 + 4*8
	save_reg	rbx, 8*16 + 5*8
	save_reg	r14, 8*16 + 6*8
	save_reg	r15, 8*16 + 7*8
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
	vmovdqa	xmm15, [rsp + 7*16]

	mov	rsi, [rsp + 8*16 + 0*8]
	mov	rdi, [rsp + 8*16 + 1*8]
	mov	rbp, [rsp + 8*16 + 2*8]
	mov	r12, [rsp + 8*16 + 3*8]
	mov	r13, [rsp + 8*16 + 4*8]
	mov	rbx, [rsp + 8*16 + 5*8]
	mov	r14, [rsp + 8*16 + 6*8]
	mov	r15, [rsp + 8*16 + 7*8]
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
;; offsets of the matches, zdcode to their distance codes, and zmatch1 and zmatch2 to the
;; qwords at them
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

	;; Load the qwords at the matches, two at a time
	lea	next_in, [f_i + file_start + %1]
%assign %%i 0
%rep 8
%if %%i == 0
%xdefine %%x zmatch1 %+ x
%elif %%i == 4
%xdefine %%x zmatch2 %+ x
%elif (%%i & 3) == 1
%xdefine %%x ztmp4 %+ x
%elif (%%i & 3) == 2
%xdefine %%x ztmp5 %+ x
%else
%xdefine %%x ztmp6 %+ x
%endif
	movsxd	tmp, dword [sbuf + SBUF_OFFSETS + 8 * %%i]
	movsxd	tmp2, dword [sbuf + SBUF_OFFSETS + 8 * %%i + 4]
	vmovq	%%x, [next_in + tmp]
	vpinsrq	%%x, %%x, [next_in + tmp2], 1
%if %%i == 1
	vinserti128 zmatch1 %+ y, zmatch1 %+ y, ztmp4 %+ x, 1
%elif %%i == 5
	vinserti128 zmatch2 %+ y, zmatch2 %+ y, ztmp4 %+ x, 1
%elif %%i == 3
	vinserti128 ztmp5 %+ y, ztmp5 %+ y, ztmp6 %+ x, 1
	vinserti64x4 zmatch1, zmatch1, ztmp5 %+ y, 1
%elif %%i == 7
	vinserti128 ztmp5 %+ y, ztmp5 %+ y, ztmp6 %+ x, 1
	vinserti64x4 zmatch2, zmatch2, ztmp5 %+ y, 1
%endif
%assign %%i (%%i + 1)
%endrep
%endm

;; Finds the match lengths of the vector at f_i and writes the match map entries of the
;; positions one before, taking each match unless the next position has a longer one
%macro STAGE_C 0
	vmovdqu64 ztmp0 %+ y, [f_i + file_start]
	vpermq	ztmp1, zdatas_perm2, ztmp0
	vpshufb	ztmp1, ztmp1, zqword_shuf
	vpermq	ztmp2, zdatas_perm3, ztmp0
	vpshufb	ztmp2, ztmp2, zqword_shuf

	;; Compute match length
	vpxorq	ztmp1, ztmp1, zmatch1
	vpxorq	ztmp2, ztmp2, zmatch2
	vpshufb	ztmp1, ztmp1, zbswap
	vpshufb	ztmp2, ztmp2, zbswap
	vplzcntq ztmp1, ztmp1
	vplzcntq ztmp2, ztmp2
	vpmovqd	ztmp1 %+ y, ztmp1
	vpmovqd	ztmp2 %+ y, ztmp2
	vinserti32x8 ztmp1, ztmp1, ztmp2 %+ y, 1
	vpsrld	ztmp1, ztmp1, 3

	;; Zero out matches which should not be taken
	vpermd	ztmp2, zrot_left, ztmp1
	vpermd	ztmp3, zrot_left, zdcode

	vmovd	ztmp4 %+ x, prev_len %+ d
	vmovd	prev_len %+ d, ztmp2 %+ x
	vmovdqu32 ztmp2 {k1}, ztmp4

	vmovd	ztmp4 %+ x, prev_dist %+ d
	vmovd	prev_dist %+ d, ztmp3 %+ x
	vmovdqu32 ztmp3 {k1}, ztmp4

	vpcmpgtd k3, ztmp2, zshortest_matches
	vpcmpgtd k4, ztmp1, ztmp2

	knotq	k3, k3
	korq	k3, k3, k4
	knotq	k4, k3
	vmovdqu32 ztmp1 {k4}{z}, ztmp2

	;; Update the distance codes to match the lengths
	vpaddd	ztmp3, ztmp3, ztmp1
	vpaddd	ztmp3, ztmp3, ztwofiftyfour
	vpmovzxbd ztmp3 {k3}, [f_i + file_start - 1]
	vpaddd	ztmp3 {k3}, ztmp3, znull_dist_syms

	vmovdqu64 [matches_next], ztmp3
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
	vmovdqu64 zqword_shuf, [qword_shuf]
	vbroadcasti64x2 zdatas_perm2, [datas_perm2]
	vbroadcasti64x2 zdatas_perm3, [datas_perm3]
	vpbroadcastd zones, [ones]
	vbroadcasti32x4 zbswap, [bswap_shuf]
	vpbroadcastd zthirty, [thirty]
	vmovdqu64 zrot_left, [drot_left]
	vpbroadcastd zshortest_matches, [shortest_matches]
	vpbroadcastd ztwofiftyfour, [twofiftyfour]
	vpbroadcastd znull_dist_syms, [null_dist_syms]
	mov	tmp2 %+ d, 1
	kmovw	k1, tmp2 %+ d

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

	;; Check first 8 bytes of match
	mov	prev_len, [next_in]
	xor	prev_len, [next_in + tmp - 1]
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

	;; Set prev_len
	xor	tmp2, tmp2
	tzcnt	prev_len, prev_len
	shr	prev_len, 3
	cmp	prev_len, MIN_DEF_MATCH
	cmovl	prev_len, tmp2

.byte_processed:
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
drot_left:
	dd 0xf, 0x0, 0x1, 0x2, 0x3, 0x4, 0x5, 0x6
	dd 0x7, 0x8, 0x9, 0xa, 0xb, 0xc, 0xd, 0xe
qword_shuf:
	db 0x0, 0x1, 0x2, 0x3, 0x4, 0x5, 0x6, 0x7
	db 0x1, 0x2, 0x3, 0x4, 0x5, 0x6, 0x7, 0x8
	db 0x2, 0x3, 0x4, 0x5, 0x6, 0x7, 0x8, 0x9
	db 0x3, 0x4, 0x5, 0x6, 0x7, 0x8, 0x9, 0xa
	db 0x4, 0x5, 0x6, 0x7, 0x8, 0x9, 0xa, 0xb
	db 0x5, 0x6, 0x7, 0x8, 0x9, 0xa, 0xb, 0xc
	db 0x6, 0x7, 0x8, 0x9, 0xa, 0xb, 0xc, 0xd
	db 0x7, 0x8, 0x9, 0xa, 0xb, 0xc, 0xd, 0xe
	db 0x8, 0x9, 0xa, 0xb, 0xc, 0xd, 0xe, 0xf
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
datas_perm2:
	dq 0x0, 0x1
datas_perm3:
	dq 0x1, 0x2
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
