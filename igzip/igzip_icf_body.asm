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

%include "lz0a_const.asm"
%include "data_struct2.asm"
%include "bitbuf2.asm"
%include "huffman.asm"
%include "igzip_compare_types.asm"
%include "reg_sizes.asm"

%include "stdmac.asm"

%ifdef DEBUG
%macro MARK 1
global %1
%1:
%endm
%else
%macro MARK 1
%endm
%endif

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
%define	file_start	rdi
%define file_length	r15
%define	stream		r14
%define	f_i		r10
%define	m_out_buf	r11

%define	curr_data	rax

%define	tmp2		rcx

%define	dist		rbx
%define dist_code2	rbx
%define	lit_code2	rbx

%define	dist2		r12
%define dist_code	r12

%define	tmp1		rsi

%define	lit_code	rsi

%define	curr_data2	r8
%define	len2		r8
%define	tmp4		r8

%define	len		rdx
%define len_code	rdx
%define hash3		rdx

%define	tmp3		r13

%define hash		rbp
%define	hash2		r9

;; GPR r8 & r15 can be used

%define xtmp0		xmm0	; tmp
%define xtmp1		xmm1	; tmp
%define	xdata		xmm4

%define ytmp0		ymm0	; tmp
%define ytmp1		ymm1	; tmp


;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

m_out_end           equ  0	 ; local variable (8 bytes)
m_out_start         equ	 8
f_end_i_mem_offset  equ 16
gpr_save_mem_offset equ 24       ; gpr save area (8*8 bytes)
xmm_save_mem_offset equ 24 + 8*8 ; xmm save area (4*16 bytes) (16 byte aligned)
stack_size          equ 3*8 + 8*8 + 4*16

;;; 8 because stack address is odd multiple of 8 after a function call and
;;; we want it aligned to 16 bytes

; void isal_deflate_icf_body ( isal_zstream *stream )
; arg 1: rcx: addr of stream
global isal_deflate_icf_body_ %+ ARCH
isal_deflate_icf_body_ %+ ARCH %+ :
%ifidn __OUTPUT_FORMAT__, elf64
	mov	rcx, rdi
%endif

	;; do nothing if (avail_in == 0)
	cmp	dword [rcx + _avail_in], 0
	jne	skip1

	;; Set stream's next state
	mov	rdx, ZSTATE_FLUSH_READ_BUFFER
	mov	rax, ZSTATE_CREATE_HDR
	cmp	dword [rcx + _end_of_stream], 0
	cmovne	rax, rdx
	cmp	dword [rcx + _flush], _NO_FLUSH
	cmovne	rax, rdx
	mov	dword [rcx + _internal_state_state], eax
	ret
skip1:

%ifdef ALIGN_STACK
	push	rbp
	mov	rbp, rsp
	sub	rsp, stack_size
	and	rsp, ~15
%else
	sub	rsp, stack_size
%endif

	mov [rsp + gpr_save_mem_offset + 0*8], rbx
	mov [rsp + gpr_save_mem_offset + 1*8], rsi
	mov [rsp + gpr_save_mem_offset + 2*8], rdi
	mov [rsp + gpr_save_mem_offset + 3*8], rbp
	mov [rsp + gpr_save_mem_offset + 4*8], r12
	mov [rsp + gpr_save_mem_offset + 5*8], r13
	mov [rsp + gpr_save_mem_offset + 6*8], r14
	mov [rsp + gpr_save_mem_offset + 7*8], r15

	mov	stream, rcx
	mov	dword [stream + _internal_state_has_eob], 0

	; state->bitbuf.set_buf(stream->next_out, stream->avail_out);
	mov	tmp1, [stream + _level_buf]
	mov	m_out_buf, [tmp1 + _icf_buf_next]

	mov     [rsp + m_out_start], m_out_buf
	mov	tmp1, [tmp1 + _icf_buf_avail_out]
	add	tmp1, m_out_buf
	sub	tmp1, SLOP

	mov	[rsp + m_out_end], tmp1

	mov	file_start, [stream + _next_in]

	mov	f_i %+ d, dword [stream + _total_in]
	sub	file_start, f_i

	mov	file_length %+ d, [stream + _avail_in]
	add	file_length, f_i

	; file_length -= LA;
	sub	file_length, LA
	; if (file_length <= 0) continue;

	cmp	file_length, f_i
	jle	input_end

	; for (f_i = f_start_i; f_i < file_length; f_i++) {
MARK __body_compute_hash_ %+ ARCH
	MOVDQU	xdata, [file_start + f_i]
	mov	curr_data, [file_start + f_i]
	mov	tmp3, curr_data
	mov	tmp4, curr_data

	compute_hash	hash, curr_data

	shr	tmp3, 8
	compute_hash	hash2, tmp3

	and	hash, HASH_MASK
	and	hash2, HASH_MASK

	cmp	dword [stream + _internal_state_has_hist], 0
	je	write_first_byte

	jmp	loop2
	align	16

loop2:
	; if (state->bitbuf.is_full()) {
	cmp	m_out_buf, [rsp + m_out_end]
	ja	output_end

	xor	dist, dist
	xor	dist2, dist2
	xor	tmp3, tmp3

	lea	tmp1, [file_start + f_i]

	mov	dist %+ w, f_i %+ w
	dec	dist
	sub	dist %+ w, word [stream + _internal_state_head + 2 * hash]
	mov	[stream + _internal_state_head + 2 * hash], f_i %+ w

	inc	f_i

	mov	tmp2, curr_data
	shr	curr_data, 16
	compute_hash	hash, curr_data
	and	hash %+ d, HASH_MASK

	mov	dist2 %+ w, f_i %+ w
	dec	dist2
	sub	dist2 %+ w, word [stream + _internal_state_head + 2 * hash2]
	mov	[stream + _internal_state_head + 2 * hash2], f_i %+ w

	; if ((dist-1) < (D-1)) {
	and	dist %+ d, (D-1)
	neg	dist

	shr	tmp2, 24
	compute_hash	hash2, tmp2
	and	hash2 %+ d, HASH_MASK

	and	dist2 %+ d, (D-1)
	neg	dist2

MARK __body_compare_ %+ ARCH
	;; Check for long len/dist match (>7) with first literal
	MOVQ	len, xdata
	mov	curr_data, len
	PSRLDQ	xdata, 1
	xor	len, [tmp1 + dist - 1]
	jz	compare_loop

	;; Check for len/dist match (>7) with second literal
	MOVQ	len2, xdata
	xor	len2, [tmp1 + dist2]
	jz	compare_loop2

	movzx	lit_code, curr_data %+ b
	shr	curr_data, 8

	;; Check for len/dist match for first literal
	test    len %+ d, 0xFFFFFFFF
	jz      len_dist_huffman_pre

	inc	word [stream + _internal_state_hist_lit_len + HIST_ELEM_SIZE*lit_code]
	movzx	lit_code2, curr_data %+ b
	;; Check for len/dist match for second literal
	test    len2 %+ d, 0xFFFFFFFF
	jnz     write_lit_bits

MARK __body_len_dist_lit_huffman_ %+ ARCH
len_dist_lit_huffman_pre:
	bsf	len2, len2
	shr	len2, 3

len_dist_lit_huffman:
	or	lit_code, LIT
	movnti	dword [m_out_buf], lit_code %+ d

	neg	dist2

	get_dist_icf_code	dist2, dist_code2, tmp1

	;; Setup for updating hash
	lea	tmp3, [f_i + 1]	; tmp3 <= k

	add	file_start, f_i
	MOVDQU	xdata, [file_start + len2]
	mov	tmp1, [file_start + len2]

	shr	curr_data, 24
	compute_hash	hash3, curr_data
	and	hash3, HASH_MASK

	mov	curr_data, tmp1
	shr	tmp1, 8

	sub	file_start, f_i
	add	f_i, len2

	mov	[stream + _internal_state_head + 2 * hash], tmp3 %+ w

	compute_hash	hash, curr_data

	add	tmp3,1
	mov	[stream + _internal_state_head + 2 * hash2], tmp3 %+ w

	compute_hash	hash2, tmp1

	add	tmp3, 1
	mov	[stream + _internal_state_head + 2 * hash3], tmp3 %+ w

	add	dist_code2, 254
	add	dist_code2, len2

	inc	word [stream + _internal_state_hist_lit_len + HIST_ELEM_SIZE*(len2 + 254)]

	movnti	dword [m_out_buf + 4], dist_code2 %+ d
	add	m_out_buf, 8

	shr	dist_code2, DIST_OFFSET
	and	dist_code2, 0x1F
	inc	word [stream + _internal_state_hist_dist + HIST_ELEM_SIZE*dist_code2]

	; hash = compute_hash(state->file_start + f_i) & HASH_MASK;
	and	hash %+ d, HASH_MASK
	and	hash2 %+ d, HASH_MASK

	; continue
	cmp	f_i, file_length
	jl	loop2
	jmp	input_end
	;; encode as dist/len

MARK __body_len_dist_huffman_ %+ ARCH
len_dist_huffman_pre:
	bsf	len, len
	shr	len, 3

len_dist_huffman:
	dec	f_i
	;; Setup for updateing hash
	lea	tmp3, [f_i + 2]	; tmp3 <= k

	neg	dist

	; get_dist_code(dist, &code2, &code_len2);
	get_dist_icf_code   dist, dist_code, tmp1

	add	file_start, f_i
	MOVDQU	xdata, [file_start + len]
	mov	curr_data2, [file_start + len]
	mov	curr_data, curr_data2
	sub	file_start, f_i
	add	f_i, len
	; get_len_code(len, &code, &code_len);
	lea	len_code, [len + 254]
	or	dist_code, len_code

	mov	[stream + _internal_state_head + 2 * hash], tmp3 %+ w
	add	tmp3,1
	mov	[stream + _internal_state_head + 2 * hash2], tmp3 %+ w

	compute_hash	hash, curr_data

	shr	curr_data2, 8
	compute_hash	hash2, curr_data2

	inc	word [stream + _internal_state_hist_lit_len + HIST_ELEM_SIZE*len_code]

	movnti	dword [m_out_buf], dist_code %+ d
	add	m_out_buf, 4

	shr     dist_code, DIST_OFFSET
	and     dist_code, 0x1F
	inc     word [stream + _internal_state_hist_dist + HIST_ELEM_SIZE*dist_code]

	; hash = compute_hash(state->file_start + f_i) & HASH_MASK;
	and	hash %+ d, HASH_MASK
	and	hash2 %+ d, HASH_MASK

	; continue
	cmp	f_i, file_length
	jl	loop2
	jmp	input_end

MARK __body_write_lit_bits_ %+ ARCH
write_lit_bits:
	MOVDQU	xdata, [file_start + f_i + 1]
	add	f_i, 1
	MOVQ	curr_data, xdata

	inc	word [stream + _internal_state_hist_lit_len + HIST_ELEM_SIZE*lit_code2]

	shl	lit_code2, DIST_OFFSET
	lea	lit_code, [lit_code + lit_code2 + (31 << DIST_OFFSET)]

	movnti	dword [m_out_buf], lit_code %+ d
	add	m_out_buf, 4

	; continue
	cmp	f_i, file_length
	jl	loop2

input_end:
	mov	tmp1, ZSTATE_FLUSH_READ_BUFFER
	mov	tmp2, ZSTATE_BODY
	cmp	dword [stream + _end_of_stream], 0
	cmovne	tmp2, tmp1
	cmp	dword [stream + _flush], _NO_FLUSH

	cmovne	tmp2, tmp1
	mov	dword [stream + _internal_state_state], tmp2 %+ d
	jmp	end

output_end:
	mov	dword [stream + _internal_state_state], ZSTATE_CREATE_HDR

end:
	;; update input buffer
	add	file_length, LA
	mov	[stream + _total_in], f_i %+ d
	add	file_start, f_i
	mov     [stream + _next_in], file_start
	sub	file_length, f_i
	mov     [stream + _avail_in], file_length %+ d

	;; update output buffer
	mov	tmp1, [stream + _level_buf]
	mov	[tmp1 + _icf_buf_next], m_out_buf
	sub	m_out_buf, [rsp + m_out_start]
	sub	[tmp1 + _icf_buf_avail_out], m_out_buf %+ d

	mov rbx, [rsp + gpr_save_mem_offset + 0*8]
	mov rsi, [rsp + gpr_save_mem_offset + 1*8]
	mov rdi, [rsp + gpr_save_mem_offset + 2*8]
	mov rbp, [rsp + gpr_save_mem_offset + 3*8]
	mov r12, [rsp + gpr_save_mem_offset + 4*8]
	mov r13, [rsp + gpr_save_mem_offset + 5*8]
	mov r14, [rsp + gpr_save_mem_offset + 6*8]
	mov r15, [rsp + gpr_save_mem_offset + 7*8]

%ifndef ALIGN_STACK
	add	rsp, stack_size
%else
	mov	rsp, rbp
	pop	rbp
%endif
	ret

MARK __body_compare_loops_ %+ ARCH
compare_loop:
	lea	tmp2, [tmp1 + dist - 1]
%if (COMPARE_TYPE == 1)
	compare250	tmp1, tmp2, len, tmp3
%elif (COMPARE_TYPE == 2)
	compare250_x	tmp1, tmp2, len, tmp3, xtmp0, xtmp1
%elif (COMPARE_TYPE == 3)
	compare250_y	tmp1, tmp2, len, tmp3, ytmp0, ytmp1
%else
	%error Unknown Compare type COMPARE_TYPE
	 % error
%endif
	jmp	len_dist_huffman

compare_loop2:
	lea	tmp2, [tmp1 + dist2]
	add	tmp1, 1
%if (COMPARE_TYPE == 1)
	compare250	tmp1, tmp2, len2, tmp3
%elif (COMPARE_TYPE == 2)
	compare250_x	tmp1, tmp2, len2, tmp3, xtmp0, xtmp1
%elif (COMPARE_TYPE == 3)
	compare250_y	tmp1, tmp2, len2, tmp3, ytmp0, ytmp1
%else
%error Unknown Compare type COMPARE_TYPE
 % error
%endif
	movzx	lit_code, curr_data %+ b
	shr	curr_data, 8
	inc	word [stream + _internal_state_hist_lit_len + HIST_ELEM_SIZE*lit_code]
	jmp	len_dist_lit_huffman

MARK __write_first_byte_ %+ ARCH
write_first_byte:
	cmp	m_out_buf, [rsp + m_out_end]
	ja	output_end

	mov	dword [stream + _internal_state_has_hist], 1

	mov	[stream + _internal_state_head + 2 * hash], f_i %+ w

	mov	hash, hash2
	shr	tmp4, 16
	compute_hash	hash2, tmp4

	and	curr_data, 0xff
	inc	word [stream + _internal_state_hist_lit_len + HIST_ELEM_SIZE*curr_data]
	or	curr_data, LIT

	movnti	dword [m_out_buf], curr_data %+ d
	add	m_out_buf, 4

	MOVDQU	xdata, [file_start + f_i + 1]
	add	f_i, 1
	mov	curr_data, [file_start + f_i]
	and	hash %+ d, HASH_MASK
	and	hash2 %+ d, HASH_MASK

	cmp	f_i, file_length
	jl	loop2
	jmp	input_end

section .data
	align 16
mask:	dd	HASH_MASK, HASH_MASK, HASH_MASK, HASH_MASK
const_D: dq	D
