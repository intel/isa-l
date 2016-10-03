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
%define ll_tree		ARG4
%define ptr		r11
%else
; Linux
%define ARG1 rdi
%define ARG2 rsi
%define ARG3 rdx
%define ARG4 rcx
%define TMP1 r8
%define TMP2 r9
%define ll_tree		r11 ; ARG4
%define ptr		ARG1 ; r11
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
%define dist_tree	ll_tree + 4*513

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
	mov	ll_tree, ARG4
%endif

	mov	bits, [bb + _m_bits]
	mov	ecx, [bb + _m_bit_count]
	mov	end_ptr, [bb + _m_out_end]
	mov	out_buf, [bb + _m_out_buf]	; clobbers bb

.start_loop:
	mov	DWORD(data), [ptr]

	cmp	out_buf, end_ptr
	ja	.overflow

	mov	sym, data
	and	sym, 0x3FF	; sym has ll_code
	mov	DWORD(sym), [ll_tree + sym * 4]

	; look up dist sym
	mov	dsym, data
	shr	dsym, DIST_OFFSET
	and	dsym, 0x1F
	mov	DWORD(dsym), [dist_tree + dsym * 4]

	; insert LL code
	; sym: 31:24 length; 23:0 code
	mov	tmp2, sym
	and	sym, 0xFFFFFF
	shl	sym, cl
	shr	tmp2, 24
	or	bits, sym
	add	rcx, tmp2

	; insert dist code
	movzx	tmp, WORD(dsym)
	shl	tmp, cl
	or	bits, tmp
	mov	tmp, dsym
	shr	tmp, 24
	add	rcx, tmp

	; insert dist extra bits
	shr	data, EXTRA_BITS_OFFSET
	add	ptr, 4
	shl	data, cl
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
	shr	bits, cl
	mov	rcx, tmp
	and	rcx, 7

	cmp	ptr, in_buf_end
	jb	.start_loop

;.end:
;	; empty bits
;	mov	[out_buf], bits
;	mov	tmp, rcx
;	shr	tmp, 3		; byte count
;	add	out_buf, tmp
;	mov	tmp, rcx
;	and	rcx, ~7
;	shr	bits, cl
;	mov	rcx, tmp
;	and	rcx, 7

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
