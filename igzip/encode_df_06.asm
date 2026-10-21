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
%include "stdmac.asm"

%ifndef ARCH
%define ARCH 06
%endif
%define USE_HSWNI

;; With USE_BYTE_PLANES, which encode_df_10.asm defines, the encoder looks up
;; codes with AVX-512 VBMI instead of gathers

; tree entry is 4 bytes:
; lit/len tree (513 entries)
; |  3  |  2   |  1 | 0 |
; | len |       code    |
;
; dist tree
; |  3  |  2   |  1 | 0 |
; |eblen:codlen|   code |

; token format:
; DIST_OFFSET:0 : lit/len
; 31:(DIST_OFFSET + 5) : dist Extra Bits
; (DIST_OFFSET + 5):DIST_OFFSET : dist code
; lit/len: 0-256 (literal)
;          257-512 (dist + 254)

; returns final token pointer
; equal to token_end if successful
;    uint32_t* encode_df(uint32_t *token_start, uint32_t *token_end,
;                            BitBuf *out_buf, uint32_t *trees);

%ifidn __OUTPUT_FORMAT__, win64
%define arg1 rcx
%define arg2 rdx
%define arg3 r8
%define arg4 r9
%define sym		rsi
%define dsym		rdi
%define hufftables	r9
%define ptr		r11
%else
; Linux
%define arg1 rdi
%define arg2 rsi
%define arg3 rdx
%define arg4 rcx
%define sym		r9
%define dsym		r8
%define hufftables	r11
%define ptr		rdi
%endif

%define in_buf_end	arg2
%define bitbuf		arg3
%define out_buf		bitbuf
; bit_count is rcx
%define bits		rax
%define data		r12
%define tmp		rbx
%define len 		dsym
%define tmp2 		r10
%define end_ptr		rbp
%ifdef USE_BYTE_PLANES
%define planes		data	; byte planes of the code tables, and lookups of 64 entries
%define lidx		dsym	; offset of the next 16 lookups in the lookups of 64 entries
%endif

%define LIT_MASK	((0x1 << LIT_LEN_BIT_COUNT) - 1)
%define DIST_MASK	((0x1 << DIST_LIT_BIT_COUNT) - 1)

%define codes1		zmm1
%define code_lens1	zmm2
%define codes2		zmm3
%define code_lens2	zmm4
%define codes3		zmm5
%define ztmp		zmm5
%define	code_lens3	zmm6
%define codes4		zmm7
%define syms		zmm7

%define code_lens4	zmm8
%define dsyms		zmm8
%define zbits_count_q	zmm8

%define codes_lookup1	zmm9
%define	codes_lookup2	zmm10
%define datas		zmm11
%define zbits		zmm12
%define zbits_count	zmm13
%define zoffset_mask	zmm14
%define znotoffset_mask	zmm23

%define zq_64		zmm15
%define zlit_mask	zmm16
%define zdist_mask	zmm17
%define zlit_icr_mask	zmm18
%define zeb_icr_mask	zmm19
%define zmax_write	zmm20
%define zrot_perm	zmm21
%define zq_8		zmm22

%ifdef USE_BYTE_PLANES
;; Registers for looking up codes in byte planes
%define zidx_lit	zmm24
%define zidx_dist	zmm25
%define zp0		zmm26
%define zp1		zmm27
%define zp2		zmm28
%define zp3		zmm29
%define zt0		zmm30
%define zt1		zmm31
%define zt2		zmm7
%define zt3		zmm8
%endif

%define VECTOR_SIZE 0x40
%define VECTOR_LOOP_PROCESSED (2 * VECTOR_SIZE)
%define VECTOR_SLOP 0x40 - 8

%ifdef USE_BYTE_PLANES
;; Byte plane p of a code table holds byte p of each entry. Plane p of the
;; 513 entry lit/len table is at LIT_PLANES + 512 * p, without entry 512,
;; and plane p of the 288 entry dist table is at DIST_PLANES + 288 * p, with
;; room after the last for the 64 bytes that a lookup reads from entry 256.
;; LOOKUPS holds the lit/len and then the dist lookups of 64 entries.
%define LIT_PLANES	0
%define LOOKUPS		(LIT_PLANES + 4 * 512)
%define DIST_PLANES	(LOOKUPS + 2 * 4 * 64)
%define PLANES_SIZE	(DIST_PLANES + 3 * 288 + 320)
;; Fewest entries for which making the planes pays off
%define PLANES_MIN_ENTRIES 256
%endif

gpr_save_mem_offset	equ	0
gpr_save_mem_size	equ	8 * 6
xmm_save_mem_offset	equ	gpr_save_mem_offset + gpr_save_mem_size
xmm_save_mem_size	equ	10 * 16
bitbuf_mem_offset	equ	xmm_save_mem_offset + xmm_save_mem_size
bitbuf_mem_size		equ	8
%ifdef USE_BYTE_PLANES
refill_end_mem_offset	equ	bitbuf_mem_offset + bitbuf_mem_size
refill_end_mem_size	equ	8
planes_mem_offset	equ	refill_end_mem_offset + refill_end_mem_size
;; rsp + planes_mem_offset is 16 byte aligned, so 48 bytes are enough to align
;; the planes to 64. Keep stack_size under 4 KB, past which a win64 stack
;; frame must probe its pages.
planes_mem_size		equ	PLANES_SIZE + 48 + 8	; 8 keeps the stack 16 byte aligned
stack_size		equ	planes_mem_offset + planes_mem_size
%else
stack_size		equ	gpr_save_mem_size + xmm_save_mem_size + bitbuf_mem_size
%endif



%macro FUNC_SAVE 0
	sub	rsp, stack_size
	mov	[rsp + gpr_save_mem_offset + 0*8], rbx
	mov	[rsp + gpr_save_mem_offset + 1*8], rbp
	mov	[rsp + gpr_save_mem_offset + 2*8], r12

%ifidn __OUTPUT_FORMAT__, win64
	mov	[rsp + gpr_save_mem_offset + 3*8], rsi
	mov	[rsp + gpr_save_mem_offset + 4*8], rdi

	MOVDQU	[rsp + xmm_save_mem_offset + 0*8], xmm6
	MOVDQU	[rsp + xmm_save_mem_offset + 1*8], xmm7
	MOVDQU	[rsp + xmm_save_mem_offset + 2*8], xmm8
	MOVDQU	[rsp + xmm_save_mem_offset + 3*8], xmm9
	MOVDQU	[rsp + xmm_save_mem_offset + 4*8], xmm10
	MOVDQU	[rsp + xmm_save_mem_offset + 5*8], xmm11
	MOVDQU	[rsp + xmm_save_mem_offset + 6*8], xmm12
	MOVDQU	[rsp + xmm_save_mem_offset + 7*8], xmm13
	MOVDQU	[rsp + xmm_save_mem_offset + 8*8], xmm14
	MOVDQU	[rsp + xmm_save_mem_offset + 9*8], xmm15
%endif

%endm

%macro FUNC_RESTORE 0
	mov	rbx, [rsp + gpr_save_mem_offset + 0*8]
	mov	rbp, [rsp + gpr_save_mem_offset + 1*8]
	mov	r12, [rsp + gpr_save_mem_offset + 2*8]

%ifidn __OUTPUT_FORMAT__, win64
	mov	rsi, [rsp + gpr_save_mem_offset + 3*8]
	mov	rdi, [rsp + gpr_save_mem_offset + 4*8]

	MOVDQU	xmm6, [rsp + xmm_save_mem_offset + 0*8]
	MOVDQU	xmm7, [rsp + xmm_save_mem_offset + 1*8]
	MOVDQU	xmm8, [rsp + xmm_save_mem_offset + 2*8]
	MOVDQU	xmm9, [rsp + xmm_save_mem_offset + 3*8]
	MOVDQU	xmm10, [rsp + xmm_save_mem_offset + 4*8]
	MOVDQU	xmm11, [rsp + xmm_save_mem_offset + 5*8]
	MOVDQU	xmm12, [rsp + xmm_save_mem_offset + 6*8]
	MOVDQU	xmm13, [rsp + xmm_save_mem_offset + 7*8]
	MOVDQU	xmm14, [rsp + xmm_save_mem_offset + 8*8]
	MOVDQU	xmm15, [rsp + xmm_save_mem_offset + 9*8]
%endif
	add	rsp, stack_size

%endmacro


%ifdef USE_BYTE_PLANES
;; Looks up the codes of the 64 entries at ptr in the byte planes, and writes
;; them to LOOKUPS. Byte k of zidx_lit and zidx_dist holds the indexes of
;; entry entry_order[k], which puts the entries back in order when
;; PLANES_TO_ENTRIES interleaves the planes. Clobbers k1-k7, and restores the
;; masks of the main loop in k1-k5.
%macro LOOKUP_64 0
	kmovq	k7, [upper_entries]
	vmovdqu64	zp0, [ptr]
	vmovdqu64	zp1, [ptr + 64]
	vmovdqu64	zp2, [ptr + 128]
	vmovdqu64	zp3, [ptr + 192]

	;; Bytes 0, 1 and 2 of each entry
	vmovdqa64	zidx_lit, [entry_byte0]
	vmovdqa64	zt0, zidx_lit
	vpermi2b	zidx_lit, zp0, zp1
	vpermi2b	zt0, zp2, zp3
	vmovdqu8	zidx_lit {k7}, zt0
	vmovdqa64	zt2, [entry_byte1]
	vmovdqa64	zt0, zt2
	vpermi2b	zt2, zp0, zp1
	vpermi2b	zt0, zp2, zp3
	vmovdqu8	zt2 {k7}, zt0
	vmovdqa64	zt3, [entry_byte2]
	vmovdqa64	zt0, zt3
	vpermi2b	zt3, zp0, zp1
	vpermi2b	zt0, zp2, zp3
	vmovdqu8	zt3 {k7}, zt0

	;; The lit/len index is bits 9:0 of an entry, and the dist index bits
	;; 18:10. k1 and k2 get bits 7 and 8 of the lit/len index, and k3 bit 9,
	;; which is set only for entry 512. k4 and k5 get bits 7 and 8 of the
	;; dist index, which is less than 288.
	vpmovb2m	k1, zidx_lit
	vptestmb	k2, zt2, [bytes_1]
	vptestmb	k3, zt2, [bytes_2]
	vpsrlw	zt0, zt2, 2
	vpsllw	zidx_dist, zt3, 6
	vpternlogd	zidx_dist, zt0, [bytes_3f]{1to16}, 0xd8
	vpmovb2m	k4, zidx_dist
	vptestmb	k5, zt3, [bytes_4]

	;; vpermt2b looks up the low 7 bits of an index in 128 entries
%assign p 0
%rep 4
	vmovdqa64	zt0, [planes + LIT_PLANES + 512 * p]
	vpermt2b	zt0, zidx_lit, [planes + LIT_PLANES + 512 * p + 64]
	vmovdqa64	zt1, [planes + LIT_PLANES + 512 * p + 128]
	vpermt2b	zt1, zidx_lit, [planes + LIT_PLANES + 512 * p + 192]
	vmovdqu8	zt0 {k1}, zt1
	vmovdqa64	zt1, [planes + LIT_PLANES + 512 * p + 256]
	vpermt2b	zt1, zidx_lit, [planes + LIT_PLANES + 512 * p + 320]
	vmovdqa64	zp %+ p, [planes + LIT_PLANES + 512 * p + 384]
	vpermt2b	zp %+ p, zidx_lit, [planes + LIT_PLANES + 512 * p + 448]
	vmovdqu8	zt1 {k1}, zp %+ p
	vmovdqu8	zt0 {k2}, zt1
	vpbroadcastb	zt0 {k3}, [hufftables + _lit_len_table + 4 * 512 + p]
	vmovdqa64	zp %+ p, zt0
%assign p (p + 1)
%endrep
	PLANES_TO_ENTRIES	planes + LOOKUPS

%assign p 0
%rep 4
	vmovdqu64	zt0, [planes + DIST_PLANES + 288 * p]
	vpermt2b	zt0, zidx_dist, [planes + DIST_PLANES + 288 * p + 64]
	vmovdqu64	zt1, [planes + DIST_PLANES + 288 * p + 128]
	vpermt2b	zt1, zidx_dist, [planes + DIST_PLANES + 288 * p + 192]
	vmovdqu8	zt0 {k4}, zt1
	vpermb	zt1, zidx_dist, [planes + DIST_PLANES + 288 * p + 256]
	vmovdqu8	zt0 {k5}, zt1
	vmovdqa64	zp %+ p, zt0
%assign p (p + 1)
%endrep
	PLANES_TO_ENTRIES	planes + LOOKUPS + 4 * 64

	kmovq	k1, [k_mask_1]
	kmovq	k2, [k_mask_2]
	kmovq	k3, [k_mask_3]
	kmovq	k4, [k_mask_4]
	kmovq	k5, [k_mask_5]
	xor	lidx, lidx
%endm

;; Interleaves the byte planes zp0-zp3 of 64 lookups into dwords at %1
%macro PLANES_TO_ENTRIES 1
	vpunpcklbw	zt0, zp0, zp1
	vpunpckhbw	zt1, zp0, zp1
	vpunpcklbw	zt2, zp2, zp3
	vpunpckhbw	zt3, zp2, zp3
	vpunpcklwd	zp0, zt0, zt2
	vmovdqu64	[%1], zp0
	vpunpckhwd	zp1, zt0, zt2
	vmovdqu64	[%1 + 64], zp1
	vpunpcklwd	zp2, zt1, zt3
	vmovdqu64	[%1 + 128], zp2
	vpunpckhwd	zp3, zt1, zt3
	vmovdqu64	[%1 + 192], zp3
%endm
%endif

default rel
section .text

global encode_deflate_icf_ %+ ARCH
encode_deflate_icf_ %+ ARCH:
	endbranch
	FUNC_SAVE

%ifnidn ptr, arg1
	mov	ptr, arg1
%endif
%ifnidn hufftables, arg4
	mov	hufftables, arg4
%endif

	mov	[rsp + bitbuf_mem_offset], bitbuf
	mov	bits, [bitbuf + _m_bits]
	mov	ecx, [bitbuf + _m_bit_count]
	mov	end_ptr, [bitbuf + _m_out_end]
	mov	out_buf, [bitbuf + _m_out_buf]	; clobbers bitbuf

	sub	end_ptr, VECTOR_SLOP
	sub	in_buf_end, VECTOR_LOOP_PROCESSED
	cmp	ptr, in_buf_end
	jge	.finish

	kxorq	k0, k0, k0
	kmovq	k1, [k_mask_1]
	kmovq	k2, [k_mask_2]
	kmovq	k3, [k_mask_3]
	kmovq	k4, [k_mask_4]
	kmovq	k5, [k_mask_5]

	vmovdqa64 zrot_perm, [rot_perm]

	vbroadcasti64x2 zq_64, [q_64]
	vbroadcasti64x2 zq_8, [q_8]

	vpbroadcastq zoffset_mask, [offset_mask]
	vpternlogd znotoffset_mask, znotoffset_mask, zoffset_mask, 0x55

	vpbroadcastd zlit_mask, [lit_mask]
	vpbroadcastd zdist_mask, [dist_mask]
	vpbroadcastd zlit_icr_mask, [lit_icr_mask]
	vpbroadcastd zeb_icr_mask, [eb_icr_mask]
	vpbroadcastd zmax_write, [max_write_d]

%ifdef USE_BYTE_PLANES
	;; With enough entries, look up their codes 64 at a time in byte planes
	;; of the code tables with vpermi2b, which takes less time than gathers
	mov	lidx, 4 * 64
	mov	qword [rsp + refill_end_mem_offset], 0
	mov	tmp, in_buf_end
	sub	tmp, ptr
	cmp	tmp, 4 * PLANES_MIN_ENTRIES
	jl	.planes_done

	;; Lookups of 64 entries read 256 bytes from ptr, and in_buf_end is
	;; 128 bytes before the end of the entries
	lea	tmp, [in_buf_end - 2 * VECTOR_SIZE]
	mov	[rsp + refill_end_mem_offset], tmp

	lea	planes, [rsp + planes_mem_offset + 48]
	and	planes, -64
	vmovdqa64	zt0, [plane_perm]
	xor	tmp, tmp
.lit_planes:
	vpermb	zt1, zt0, [hufftables + _lit_len_table + 4 * tmp]
	vmovdqu64	[planes + LIT_PLANES + tmp], zt1 %+ x
	vextracti32x4	[planes + LIT_PLANES + 512 + tmp], zt1, 1
	vextracti32x4	[planes + LIT_PLANES + 2 * 512 + tmp], zt1, 2
	vextracti32x4	[planes + LIT_PLANES + 3 * 512 + tmp], zt1, 3
	add	tmp, 16
	cmp	tmp, 512
	jb	.lit_planes

	xor	tmp, tmp
.dist_planes:
	vpermb	zt1, zt0, [hufftables + _dist_table + 4 * tmp]
	vmovdqu64	[planes + DIST_PLANES + tmp], zt1 %+ x
	vextracti32x4	[planes + DIST_PLANES + 288 + tmp], zt1, 1
	vextracti32x4	[planes + DIST_PLANES + 2 * 288 + tmp], zt1, 2
	vextracti32x4	[planes + DIST_PLANES + 3 * 288 + tmp], zt1, 3
	add	tmp, 16
	cmp	tmp, 288
	jb	.dist_planes
.planes_done:
%endif

	knotq	k6, k0
	vmovdqu64	datas, [ptr]
	vpandd	syms, datas, zlit_mask
	vpgatherdd codes_lookup1 {k6}, [hufftables + _lit_len_table + 4 * syms]

	knotq	k7, k0
	vpsrld	dsyms, datas, DIST_OFFSET
	vpandd	dsyms, dsyms, zdist_mask
	vpgatherdd codes_lookup2 {k7}, [hufftables + _dist_table + 4 * dsyms]

	vmovq	zbits %+ x, bits
	vmovq	zbits_count %+ x, rcx

.main_loop:
	;;  Sets codes1 to contain lit/len codes andcode_lens1 the corresponding lengths
	vpsrld	code_lens1, codes_lookup1, 24
	vpandd	codes1, codes_lookup1, zlit_icr_mask

	;; Sets codes2 to contain dist codes, code_lens2 the corresponding lengths,
	;; and code_lens3 the extra bit counts
	vmovdqu16	codes2 {k1}{z}, codes_lookup2 ;Bits 8 and above of zbits are 0
	vpsrld	code_lens2, codes_lookup2, 24
	vpsrld	code_lens3, codes_lookup2, 16
	vpandd	code_lens3, code_lens3, zeb_icr_mask

	;; Set codes3 to contain the extra bits
	vpsrld	codes3, datas, EXTRA_BITS_OFFSET

	cmp	out_buf, end_ptr
	ja	.main_loop_exit

	;; Start code lookups for next iteration
%ifdef USE_BYTE_PLANES
	add	ptr, VECTOR_SIZE
	vmovdqu64	datas, [ptr]
	cmp	lidx, 4 * 64
	jb	.next_lookups
	cmp	ptr, [rsp + refill_end_mem_offset]
	ja	.gather_lookups
	LOOKUP_64
.next_lookups:
	vmovdqu64	codes_lookup1, [planes + LOOKUPS + lidx]
	vmovdqu64	codes_lookup2, [planes + LOOKUPS + 4 * 64 + lidx]
	add	lidx, 64
.lookups_done:
%else
	knotq	k6, k0
	add	ptr, VECTOR_SIZE
	vmovdqu64	datas, [ptr]
	vpandd	syms, datas, zlit_mask
	vpgatherdd codes_lookup1 {k6}, [hufftables + _lit_len_table + 4 * syms]

	knotq	k7, k0
	vpsrld	dsyms, datas, DIST_OFFSET
	vpandd	dsyms, dsyms, zdist_mask
	vpgatherdd codes_lookup2 {k7}, [hufftables + _dist_table + 4 * dsyms]
%endif

	;; Merge dist code with extra bits
	vpsllvd	codes3, codes3, code_lens2
	vpxord	codes2, codes2, codes3
	vpaddd	code_lens2, code_lens2, code_lens3

	;; Check for long codes
	vpaddd	code_lens3, code_lens1, code_lens2
	vpcmpgtd	k6, code_lens3, zmax_write
	ktestd	k6, k6
	jnz	.long_codes

	;; Merge dist and len codes
	vpsllvd	codes2, codes2, code_lens1
	vpxord	codes1, codes1, codes2

	vmovdqa32 codes3 {k1}{z}, codes1
	vpsrlq	codes1, codes1, 32
	vpsrlq	code_lens1, code_lens3, 32
	vmovdqa32	code_lens3 {k1}{z}, code_lens3

	;; Merge bitbuf bits
	vpsllvq codes3, codes3, zbits_count
	vpxord	codes3, codes3, zbits
	vpaddq	code_lens3, code_lens3, zbits_count

	;; Merge two symbols into qwords
	vpsllvq	codes1, codes1, code_lens3
	vpxord codes1, codes1, codes3
	vpaddq code_lens1, code_lens1, code_lens3

	;; Determine total bits at end of each qword
	vpermq	zbits_count {k5}{z}, zrot_perm, code_lens1
	vpaddq	code_lens2, zbits_count, code_lens1
	vshufi64x2 zbits_count {k3}{z}, code_lens2, code_lens2, 0x90
	vpaddq	code_lens2, code_lens2, zbits_count
	vshufi64x2 zbits_count {k2}{z}, code_lens2, code_lens2, 0x40
	vpaddq	code_lens2, code_lens2, zbits_count

	;; Bit align quadwords
	vpandd	zbits_count, code_lens2, zoffset_mask
	vpermq	zbits_count_q {k5}{z}, zrot_perm, zbits_count
	vpsllvq	codes1, codes1, zbits_count_q

	;; Check whether any of the last bytes overlap
	vpcmpq k6 {k5}, code_lens1, zbits_count, 1

	;; Get last byte in each qword
	vpsrlq	code_lens2, code_lens2, 3
	vpaddq	code_lens1, code_lens1, zbits_count_q
	vpandq	code_lens1, code_lens1, znotoffset_mask
	vpsrlvq	codes3, codes1, code_lens1

	;; Branch to handle overlapping last bytes
	ktestd k6, k6
	jnz .small_codes

.small_codes_next:
	;; Save off zbits and zbits_count for next loop
	knotq	k7, k5
	vpermq	zbits {k7}{z}, zrot_perm, codes3
	vpermq	zbits_count {k7}{z}, zrot_perm, zbits_count

	;; Merge last byte in each qword with the next qword
	vpermq	codes3 {k5}{z}, zrot_perm, codes3
	vpxord codes1, codes1, codes3

	;; Determine total bytes written
	vextracti64x2 code_lens1 %+ x, code_lens2, 3
	vpextrq tmp2, code_lens1 %+ x, 1

	;; Write out qwords
	knotq	k6, k0
	vpermq code_lens2 {k5}{z}, zrot_perm, code_lens2
	vpscatterqq [out_buf + code_lens2] {k6}, codes1

	add	out_buf, tmp2

	cmp	ptr, in_buf_end
	jbe	.main_loop

.main_loop_exit:
	vmovq	rcx, zbits_count %+ x
	vmovq	bits, zbits %+ x
	jmp	.finish

%ifdef USE_BYTE_PLANES
.gather_lookups:
	knotq	k6, k0
	vpandd	syms, datas, zlit_mask
	vpgatherdd codes_lookup1 {k6}, [hufftables + _lit_len_table + 4 * syms]

	knotq	k7, k0
	vpsrld	dsyms, datas, DIST_OFFSET
	vpandd	dsyms, dsyms, zdist_mask
	vpgatherdd codes_lookup2 {k7}, [hufftables + _dist_table + 4 * dsyms]
	jmp	.lookups_done
%endif

.small_codes:
	;; Merge overlapping last bytes
	vpermq	codes4 {k6}{z}, zrot_perm, codes3
	vporq codes3, codes3, codes4
	kshiftlq k7, k6, 1
	ktestd k6, k7
	jz .small_codes_next

	kandq k6, k6, k7
	jmp .small_codes

.long_codes:
	add	end_ptr, VECTOR_SLOP
	sub	ptr, VECTOR_SIZE

	vmovdqa32 codes3 {k1}{z}, codes1
	vmovdqa32 code_lens3 {k1}{z}, code_lens1
	vmovdqa32 codes4 {k1}{z}, codes2

	vpsllvq	codes4, codes4, code_lens3
	vpxord	codes3, codes3, codes4
	vpaddd	code_lens3, code_lens1, code_lens2

	vpsrlq	codes1, codes1, 32
	vpsrlq	code_lens1, code_lens1, 32
	vpsrlq	codes2, codes2, 32

	vpsllvq	codes2, codes2, code_lens1
	vpxord codes1, codes1, codes2

	vpsrlq code_lens1, code_lens3, 32
	vmovdqa32	code_lens3 {k1}{z}, code_lens3

	;; Merge bitbuf bits
	vpsllvq codes3, codes3, zbits_count
	vpxord	codes3, codes3, zbits
	vpaddq	code_lens3, code_lens3, zbits_count
	vpaddq code_lens1, code_lens1, code_lens3

	xor	bits, bits
	xor	rcx, rcx
	vpsubq code_lens1, code_lens1, code_lens3

	vmovdqu64 codes2, codes1
	vmovdqu64 code_lens2, code_lens1
	vmovdqu64 codes4, codes3
	vmovdqu64 code_lens4, code_lens3
%assign i 0
%rep 4
%assign i (i + 1)
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
	cmp	out_buf, end_ptr
	ja	.overflow
	;; insert LL code
	vmovq	sym, codes3 %+ x
	vmovq	tmp2, code_lens3 %+ x
	SHLX	sym, sym, rcx
	or	bits, sym
	add	rcx, tmp2

	; empty bits
	mov	[out_buf], bits
	mov	tmp, rcx
	shr	tmp, 3		; byte count
	add	out_buf, tmp
	mov	tmp, rcx
	and	rcx, ~7
	SHRX	bits, bits, rcx
	mov	rcx, tmp
	and	rcx, 7
	add	ptr, 4

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
	cmp	out_buf, end_ptr
	ja	.overflow
	;; insert LL code
	vmovq	sym, codes1 %+ x
	vmovq	tmp2, code_lens1 %+ x
	SHLX	sym, sym, rcx
	or	bits, sym
	add	rcx, tmp2

	; empty bits
	mov	[out_buf], bits
	mov	tmp, rcx
	shr	tmp, 3		; byte count
	add	out_buf, tmp
	mov	tmp, rcx
	and	rcx, ~7
	SHRX	bits, bits, rcx
	mov	rcx, tmp
	and	rcx, 7
	add	ptr, 4

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
	cmp	out_buf, end_ptr
	ja	.overflow
	;; insert LL code
	vpextrq	sym, codes3 %+ x, 1
	vpextrq	tmp2, code_lens3 %+ x, 1
	SHLX	sym, sym, rcx
	or	bits, sym
	add	rcx, tmp2

	; empty bits
	mov	[out_buf], bits
	mov	tmp, rcx
	shr	tmp, 3		; byte count
	add	out_buf, tmp
	mov	tmp, rcx
	and	rcx, ~7
	SHRX	bits, bits, rcx
	mov	rcx, tmp
	and	rcx, 7
	add	ptr, 4

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
	cmp	out_buf, end_ptr
	ja	.overflow
	;; insert LL code
	vpextrq	sym, codes1 %+ x, 1
	vpextrq	tmp2, code_lens1 %+ x, 1
	SHLX	sym, sym, rcx
	or	bits, sym
	add	rcx, tmp2

	; empty bits
	mov	[out_buf], bits
	mov	tmp, rcx
	shr	tmp, 3		; byte count
	add	out_buf, tmp
	mov	tmp, rcx
	and	rcx, ~7
	SHRX	bits, bits, rcx
	mov	rcx, tmp
	and	rcx, 7
	add	ptr, 4

	vextracti32x4 codes3 %+ x, codes4, i
	vextracti32x4 code_lens3 %+ x, code_lens4, i
	vextracti32x4 codes1 %+ x, codes2, i
	vextracti32x4 code_lens1 %+ x, code_lens2, i
%endrep
	sub	end_ptr, VECTOR_SLOP

	vmovq	zbits %+ x, bits
	vmovq	zbits_count %+ x, rcx
	cmp	ptr, in_buf_end
	jbe	.main_loop

.finish:
	add	in_buf_end, VECTOR_LOOP_PROCESSED
	add	end_ptr, VECTOR_SLOP

	cmp	ptr, in_buf_end
	jge	.overflow

.finish_loop:
	mov	DWORD(data), [ptr]

	cmp	out_buf, end_ptr
	ja	.overflow

	mov	sym, data
	and	sym, LIT_MASK	; sym has ll_code
	mov	DWORD(sym), [hufftables + _lit_len_table + sym * 4]

	; look up dist sym
	mov	dsym, data
	shr	dsym, DIST_OFFSET
	and	dsym, DIST_MASK
	mov	DWORD(dsym), [hufftables + _dist_table + dsym * 4]

	; insert LL code
	; sym: 31:24 length; 23:0 code
	mov	tmp2, sym
	and	sym, 0xFFFFFF
	SHLX	sym, sym, rcx
	shr	tmp2, 24
	or	bits, sym
	add	rcx, tmp2

	; insert dist code
	movzx	tmp, WORD(dsym)
	SHLX	tmp, tmp, rcx
	or	bits, tmp
	mov	tmp, dsym
	shr	tmp, 24
	add	rcx, tmp

	; insert dist extra bits
	shr	data, EXTRA_BITS_OFFSET
	add	ptr, 4
	SHLX	data, data, rcx
	or	bits, data
	shr	dsym, 16
	and	dsym, 0xFF
	add	rcx, dsym

	; empty bits
	mov	[out_buf], bits
	mov	tmp, rcx
	shr	tmp, 3		; byte count
	add	out_buf, tmp
	mov	tmp, rcx
	and	rcx, ~7
	SHRX	bits, bits, rcx
	mov	rcx, tmp
	and	rcx, 7

	cmp	ptr, in_buf_end
	jb	.finish_loop

.overflow:
	mov	tmp, [rsp + bitbuf_mem_offset]
	mov	[tmp + _m_bits], bits
	mov	[tmp + _m_bit_count], ecx
	mov	[tmp + _m_out_buf], out_buf

	mov	rax, ptr

	FUNC_RESTORE

	ret

section .data
	align 64
;; 64 byte data
rot_perm:
	dq 0x00000007, 0x00000000, 0x00000001, 0x00000002
	dq 0x00000003, 0x00000004, 0x00000005, 0x00000006
%ifdef USE_BYTE_PLANES
section .rodata
	align 64
;; Byte 16 * p + k is byte p of dword k, to split 16 dwords into byte planes
plane_perm:
	db 0x00, 0x04, 0x08, 0x0c, 0x10, 0x14, 0x18, 0x1c, 0x20, 0x24, 0x28, 0x2c, 0x30, 0x34, 0x38, 0x3c
	db 0x01, 0x05, 0x09, 0x0d, 0x11, 0x15, 0x19, 0x1d, 0x21, 0x25, 0x29, 0x2d, 0x31, 0x35, 0x39, 0x3d
	db 0x02, 0x06, 0x0a, 0x0e, 0x12, 0x16, 0x1a, 0x1e, 0x22, 0x26, 0x2a, 0x2e, 0x32, 0x36, 0x3a, 0x3e
	db 0x03, 0x07, 0x0b, 0x0f, 0x13, 0x17, 0x1b, 0x1f, 0x23, 0x27, 0x2b, 0x2f, 0x33, 0x37, 0x3b, 0x3f
;; The entry whose indexes byte k of a lookup index vector holds
;; entry_order: 0 1 2 3 16 17 18 19 32 33 34 35 48 49 50 51 4 5 6 7 20 21 22 23 36 37 38 39 52 53 54 55 8 9 10 11 24 25 26 27 40 41 42 43 56 57 58 59 12 13 14 15 28 29 30 31 44 45 46 47 60 61 62 63
;; Byte 0 of entry entry_order[k], in two vectors of 32 entries
entry_byte0:
	db 0x00, 0x04, 0x08, 0x0c, 0x40, 0x44, 0x48, 0x4c, 0x00, 0x04, 0x08, 0x0c, 0x40, 0x44, 0x48, 0x4c
	db 0x10, 0x14, 0x18, 0x1c, 0x50, 0x54, 0x58, 0x5c, 0x10, 0x14, 0x18, 0x1c, 0x50, 0x54, 0x58, 0x5c
	db 0x20, 0x24, 0x28, 0x2c, 0x60, 0x64, 0x68, 0x6c, 0x20, 0x24, 0x28, 0x2c, 0x60, 0x64, 0x68, 0x6c
	db 0x30, 0x34, 0x38, 0x3c, 0x70, 0x74, 0x78, 0x7c, 0x30, 0x34, 0x38, 0x3c, 0x70, 0x74, 0x78, 0x7c
;; Byte 1 of entry entry_order[k], in two vectors of 32 entries
entry_byte1:
	db 0x01, 0x05, 0x09, 0x0d, 0x41, 0x45, 0x49, 0x4d, 0x01, 0x05, 0x09, 0x0d, 0x41, 0x45, 0x49, 0x4d
	db 0x11, 0x15, 0x19, 0x1d, 0x51, 0x55, 0x59, 0x5d, 0x11, 0x15, 0x19, 0x1d, 0x51, 0x55, 0x59, 0x5d
	db 0x21, 0x25, 0x29, 0x2d, 0x61, 0x65, 0x69, 0x6d, 0x21, 0x25, 0x29, 0x2d, 0x61, 0x65, 0x69, 0x6d
	db 0x31, 0x35, 0x39, 0x3d, 0x71, 0x75, 0x79, 0x7d, 0x31, 0x35, 0x39, 0x3d, 0x71, 0x75, 0x79, 0x7d
;; Byte 2 of entry entry_order[k], in two vectors of 32 entries
entry_byte2:
	db 0x02, 0x06, 0x0a, 0x0e, 0x42, 0x46, 0x4a, 0x4e, 0x02, 0x06, 0x0a, 0x0e, 0x42, 0x46, 0x4a, 0x4e
	db 0x12, 0x16, 0x1a, 0x1e, 0x52, 0x56, 0x5a, 0x5e, 0x12, 0x16, 0x1a, 0x1e, 0x52, 0x56, 0x5a, 0x5e
	db 0x22, 0x26, 0x2a, 0x2e, 0x62, 0x66, 0x6a, 0x6e, 0x22, 0x26, 0x2a, 0x2e, 0x62, 0x66, 0x6a, 0x6e
	db 0x32, 0x36, 0x3a, 0x3e, 0x72, 0x76, 0x7a, 0x7e, 0x32, 0x36, 0x3a, 0x3e, 0x72, 0x76, 0x7a, 0x7e
bytes_1:
	times 64 db 0x01
bytes_2:
	times 64 db 0x02
bytes_4:
	times 64 db 0x04
;; 32 bit constant for vpternlogd, applied to every byte
bytes_3f:
	dd 0x3f3f3f3f
;; Bytes of entry_order with entries 32 to 63
upper_entries:
	dq 0xff00ff00ff00ff00

section .data
%endif

;; 16 byte data
q_64:
	dq 0x0000000000000040, 0x0000000000000000
q_8 :
	dq 0x0000000000000000, 0x0000000000000008

;; 8 byte data
offset_mask:
	dq 0x0000000000000007

;; 4 byte data
max_write_d:
	dd 0x1c
lit_mask:
	dd LIT_MASK
dist_mask:
	dd DIST_MASK
lit_icr_mask:
	dd 0x00ffffff
eb_icr_mask:
	dd 0x000000ff

;; k mask constants
k_mask_1: dq 0x55555555
k_mask_2: dq 0xfffffff0
k_mask_3: dq 0xfffffffc
k_mask_4: dw 0x0101, 0x0101, 0x0101, 0x0101
k_mask_5: dq 0xfffffffe
