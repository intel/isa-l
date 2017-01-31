%include "reg_sizes.asm"
%include "lz0a_const.asm"
%include "data_struct2.asm"
%include "stdmac.asm"

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
;                            BitBuf *bb, uint32_t *trees);

%ifidn __OUTPUT_FORMAT__, win64
%define ARG1 rcx
%define ARG2 rdx
%define ARG3 r8
%define ARG4 r9
%define TMP1 rsi
%define TMP2 rdi
%define hufftables	ARG4
%define ptr		r11
%else
; Linux
%define ARG1 rdi
%define ARG2 rsi
%define ARG3 rdx
%define ARG4 rcx
%define TMP1 r8
%define TMP2 r9
%define hufftables	r11
%define ptr		ARG1
%endif

%define in_buf_end	ARG2
%define bb		ARG3
%define out_buf		bb
; bit_count is rcx
%define bits		rax
%define data		r12
%define tmp		rbx
%define sym		TMP1
%define dsym		TMP2
%define len 		dsym
%define tmp2 		r10
%define end_ptr		rbp

%define LIT_MASK	((0x1 << LIT_LEN_BIT_COUNT) - 1)
%define DIST_MASK	((0x1 << DIST_LIT_BIT_COUNT) - 1)

%define codes1		ymm1
%define code_lens1	ymm2
%define codes2		ymm3
%define code_lens2	ymm4
%define codes3		ymm5
%define	code_lens3	ymm6
%define codes4		ymm7
%define syms		ymm7

%define code_lens4	ymm8
%define dsyms		ymm8

%define ytmp		ymm9
%define codes_lookup1	ymm10
%define	codes_lookup2	ymm11
%define datas		ymm12
%define ybits		ymm13
%define ybits_count	ymm14
%define yoffset_mask	ymm15

%define VECTOR_SIZE 0x20
%define VECTOR_LOOP_PROCESSED (2 * VECTOR_SIZE)
%define VECTOR_SLOP 0x20 - 8

global encode_deflate_icf_ %+ ARCH
encode_deflate_icf_ %+ ARCH:
	push	rbx
%ifidn __OUTPUT_FORMAT__, win64
	push	rsi
	push	rdi
%endif
	push	r12
	push	rbp
	push	bb

	; free up rcx
%ifidn __OUTPUT_FORMAT__, win64
	mov	ptr, ARG1
%else
	mov	hufftables, ARG4
%endif

	mov	bits, [bb + _m_bits]
	mov	ecx, [bb + _m_bit_count]
	mov	end_ptr, [bb + _m_out_end]
	mov	out_buf, [bb + _m_out_buf]	; clobbers bb

	sub	end_ptr, VECTOR_SLOP
	sub	in_buf_end, VECTOR_LOOP_PROCESSED
	cmp	ptr, in_buf_end
	jge	.finish

	vpcmpeqq	ytmp, ytmp, ytmp
	vmovdqu	datas, [ptr]
	vpand	syms, datas, [lit_mask]
	vpgatherdd codes_lookup1, [hufftables + _lit_len_table + 4 * syms], ytmp

	vpcmpeqq	ytmp, ytmp, ytmp
	vpsrld	dsyms, datas, DIST_OFFSET
	vpand	dsyms, dsyms, [dist_mask]
	vpgatherdd codes_lookup2, [hufftables + _dist_table + 4 * dsyms], ytmp

	vmovq	ybits %+ x, bits
	vmovq	ybits_count %+ x, rcx
	vmovdqa	yoffset_mask, [offset_mask]

.main_loop:
	;;  Sets codes1 to contain lit/len codes andcode_lens1 the corresponding lengths
	vpsrld	code_lens1, codes_lookup1, 24
	vpand	codes1, codes_lookup1, [lit_icr_mask]

	;; Sets codes2 to contain dist codes, code_lens2 the corresponding lengths,
	;; and code_lens3 the extra bit counts
	vpblendw	codes2, ybits, codes_lookup2, 0x55 ;Bits 8 and above of ybits are 0
	vpsrld	code_lens2, codes_lookup2, 24
	vpsrld	code_lens3, codes_lookup2, 16
	vpand	code_lens3, [eb_icr_mask]

	;; Set codes3 to contain the extra bits
	vpsrld	codes3, datas, EXTRA_BITS_OFFSET

	cmp	out_buf, end_ptr
	ja	.main_loop_exit

	;; Start code lookups for next iteration
	add	ptr, VECTOR_SIZE
	vpcmpeqq	ytmp, ytmp, ytmp
	vmovdqu	datas, [ptr]
	vpand	syms, datas, [lit_mask]
	vpgatherdd codes_lookup1, [hufftables + _lit_len_table + 4 * syms], ytmp

	vpcmpeqq	ytmp, ytmp, ytmp
	vpsrld	dsyms, datas, DIST_OFFSET
	vpand	dsyms, dsyms, [dist_mask]
	vpgatherdd codes_lookup2, [hufftables + _dist_table + 4 * dsyms], ytmp

	;; Merge dist code with extra bits
	vpsllvd	codes3, codes3, code_lens2
	vpxor	codes2, codes2, codes3
	vpaddd	code_lens2, code_lens2, code_lens3

	;; Check for long codes
	vpaddd	code_lens3, code_lens1, code_lens2
	vpcmpgtd	ytmp, code_lens3, [max_write_d]
	vptest	ytmp, ytmp
	jnz	.long_codes

	;; Merge dist and len codes
	vpsllvd	codes2, codes2, code_lens1
	vpxor	codes1, codes1, codes2

	;; Split buffer data into qwords, ytmp is 0 after last branch
	vpblendd codes3, ytmp, codes1, 0x55
	vpsrlq	codes1, codes1, 32
	vpsrlq	code_lens1, code_lens3, 32
	vpblendd	code_lens3, ytmp, code_lens3, 0x55

	;; Merge bitbuf bits
	vpsllvq codes3, codes3, ybits_count
	vpxor	codes3, codes3, ybits
	vpaddq	code_lens3, code_lens3, ybits_count

	;; Merge two symbols into qwords
	vpsllvq	codes1, codes1, code_lens3
	vpxor codes1, codes1, codes3
	vpaddq code_lens1, code_lens1, code_lens3

	;; Split buffer data into dqwords, ytmp is 0 after last branch
	vpblendd	codes2, ytmp, codes1, 0x33
	vpblendd	code_lens2, ytmp, code_lens1, 0x33
	vpsrldq	codes1, 8
	vpsrldq	code_lens1, 8

	;; Merge two qwords into dqwords
	vmovdqa	ytmp, [q_64]
	vpsubq	code_lens3, ytmp, code_lens2
	vpsrlvq	codes3, codes1, code_lens3
	vpslldq	codes3, codes3, 8

	vpsllvq	codes1, codes1, code_lens2

	vpxor	codes1, codes1, codes3
	vpxor	codes1, codes1, codes2
	vpaddq	code_lens1, code_lens1, code_lens2

	vmovq	tmp, code_lens1 %+ x 	;Number of bytes
	shr	tmp, 3
	vpand	ybits_count, code_lens1, yoffset_mask ;Extra bits

	;; bit shift upper dqword combined bits to line up with lower dqword
	vextracti128	codes2 %+ x, codes1, 1
	vextracti128	code_lens2 %+ x, code_lens1, 1

	vpbroadcastq	ybits_count, ybits_count %+ x
	vpsrldq	codes3, codes2, 1
	vpsllvq	codes2, codes2, ybits_count
	vpsllvq	codes3, codes3, ybits_count
	vpslldq	codes3, codes3, 1
	vpor	codes2, codes2, codes3

	; Write out lower dqword of combined bits
	vmovdqu	[out_buf], codes1
	movzx	bits, byte [out_buf + tmp]
	vmovq	codes1 %+ x, bits
	vpaddq	code_lens1, code_lens1, code_lens2

	vmovq	tmp2, code_lens1 %+ x	;Number of bytes
	shr	tmp2, 3
	vpand	ybits_count, code_lens1, yoffset_mask ;Extra bits

	; Write out upper dqword of combined bits
	vpor	codes1 %+ x, codes1 %+ x, codes2 %+ x
	vmovdqu	[out_buf + tmp], codes1 %+ x
	add	out_buf, tmp2
	movzx	bits, byte [out_buf]
	vmovq	ybits %+ x, bits

	cmp	ptr, in_buf_end
	jbe	.main_loop

.main_loop_exit
	vmovq	rcx, ybits_count %+ x
	vmovq	bits, ybits %+ x
	jmp	.finish

.long_codes:
	add	end_ptr, VECTOR_SLOP
	sub	ptr, VECTOR_SIZE

	vpxor ytmp, ytmp, ytmp
	vpblendd codes3, ytmp, codes1, 0x55
	vpblendd code_lens3, ytmp, code_lens1, 0x55
	vpblendd codes4, ytmp, codes2, 0x55

	vpsllvq	codes4, codes4, code_lens3
	vpxor	codes3, codes3, codes4
	vpaddd	code_lens3, code_lens1, code_lens2

	vpsrlq	codes1, codes1, 32
	vpsrlq	code_lens1, code_lens1, 32
	vpsrlq	codes2, codes2, 32

	vpsllvq	codes2, codes2, code_lens1
	vpxor codes1, codes1, codes2

	vpsrlq code_lens1, code_lens3, 32
	vpblendd	code_lens3, ytmp, code_lens3, 0x55

	;; Merge bitbuf bits
	vpsllvq codes3, codes3, ybits_count
	vpxor	codes3, codes3, ybits
	vpaddq	code_lens3, code_lens3, ybits_count
	vpaddq code_lens1, code_lens1, code_lens3

	xor	bits, bits
	xor	rcx, rcx
	vpsubq code_lens1, code_lens1, code_lens3
%rep 2
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

	vextracti128 codes3 %+ x, codes3, 1
	vextracti128 code_lens3 %+ x, code_lens3, 1
	vextracti128 codes1 %+ x, codes1, 1
	vextracti128 code_lens1 %+ x, code_lens1, 1
%endrep
	sub	end_ptr, VECTOR_SLOP

	vmovq	ybits %+ x, bits
	vmovq	ybits_count %+ x, rcx
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
	pop	TMP1	; TMP1 now points to bb
	mov	[TMP1 + _m_bits], bits
	mov	[TMP1 + _m_bit_count], ecx
	mov	[TMP1 + _m_out_buf], out_buf

	pop	rbp
	pop	r12
%ifidn __OUTPUT_FORMAT__, win64
	pop	rdi
	pop	rsi
%endif
	pop	rbx

	mov	rax, ptr

	ret

section .data
	align 32
max_write_d:
	dd	0x1c, 0x1d, 0x20, 0x20, 0x1e, 0x1e, 0x1e, 0x1e
offset_mask:
	dq	0x0000000000000007, 0x0000000000000000
	dq	0x0000000000000000, 0x0000000000000000
q_64:
	dq	0x0000000000000040, 0x0000000000000000
	dq	0x0000000000000040, 0x0000000000000000
lit_mask:
	dd LIT_MASK, LIT_MASK, LIT_MASK, LIT_MASK
	dd LIT_MASK, LIT_MASK, LIT_MASK, LIT_MASK
dist_mask:
	dd DIST_MASK, DIST_MASK, DIST_MASK, DIST_MASK
	dd DIST_MASK, DIST_MASK, DIST_MASK, DIST_MASK
lit_icr_mask:
	dd 0x00FFFFFF, 0x00FFFFFF, 0x00FFFFFF, 0x00FFFFFF
	dd 0x00FFFFFF, 0x00FFFFFF, 0x00FFFFFF, 0x00FFFFFF
eb_icr_mask:
	dd 0x000000FF, 0x000000FF, 0x000000FF, 0x000000FF
	dd 0x000000FF, 0x000000FF, 0x000000FF, 0x000000FF
