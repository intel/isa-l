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
%ifndef TEST

extern fold_4

%include "lz0a_const.asm"
%include "data_struct2.asm"
%include "bitbuf2.asm"
%include "huffman.asm"
%include "igzip_compare_types.asm"

%include "reg_sizes.asm"

%include "stdmac.asm"

%if (ARCH == 04)
	%define MOVDQA vmovdqa
%else
	%define MOVDQA movdqa
%endif

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
%define tmp2             rcx
%define hash2            rcx

%define b_bytes_valid    rax
%define curr_data        rax
%define code             rax
%define tmp5             rax

%define tmp4             rbx
%define dist             rbx
%define code2            rbx

%define x                rdx
%define len              rdx
%define hash             rdx
%define code_len3        rdx
%define	tmp8             rdx

%define tmp1             rsi
%define code_len2        rsi

%define blen             rdi
%define file_start       rdi

%define m_bit_count      rbp

%define in_buf           r8
%define curr_data2       r8
%define len2             r8
%define tmp6             r8

%define m_bits           r9

%define f_i              r10

%define m_out_buf        r11

%define f_end_i          r12
%define dist2            r12
%define tmp7             r12
%define code4            r12

%define tmp3             r13
%define code3            r13

%define stream           r14

%define hufftables	r15

%define crc_0		xmm0	; in/out: crc state
%define crc_1		xmm1	; in/out: crc state
%define crc_2		xmm2	; in/out: crc state
%define crc_3		xmm3	; in/out: crc state
%define crc_fold	xmm4	; in:	(loaded from fold_4)

%define xtmp0		xmm5	; tmp
%define xtmp1		xmm6	; tmp
%define xtmp2		xmm7	; tmp
%define xtmp3		xmm8	; tmp
%define xtmp4		xmm9	; tmp
%define	xhash		xmm10
%define	xmask		xmm11
%define	xdata		xmm12

%define ytmp0		ymm5	; tmp
%define ytmp1		ymm6	; tmp

%if ( ARCH == 02 || ARCH == 04)
%define vtmp0		ymm5	; tmp
%define vtmp1		ymm6	; tmp
%define vtmp2		ymm7	; tmp
%define vtmp3		ymm8	; tmp
%define vtmp4		ymm9	; tmp
%else
%define vtmp0		xmm5	; tmp
%define vtmp1		xmm6	; tmp
%define vtmp2		xmm7	; tmp
%define vtmp3		xmm8	; tmp
%define vtmp4		xmm9	; tmp
%endif
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

%define b_bytes_processed	f_i

blen_mem_offset     equ  0	 ; local variable (8 bytes)
in_buf_mem_offset   equ  8
f_end_i_mem_offset  equ 16
empty_buffer_flag   equ	24
gpr_save_mem_offset equ 32       ; gpr save area (8*8 bytes)
xmm_save_mem_offset equ 32 + 8*8 ; xmm save area (8*16 bytes) (16 byte aligned)
stack_size          equ 4*8 + 8*8 + 8*16 + 8
;;; 8 because stack address is odd multiple of 8 after a function call and
;;; we want it aligned to 16 bytes

; void isal_deflate_body ( isal_zstream *stream )
; arg 1: rcx: addr of stream
global isal_deflate_body_ %+ ARCH
isal_deflate_body_ %+ ARCH %+ :
%ifidn __OUTPUT_FORMAT__, elf64
	mov	rcx, rdi
%endif

	;; do nothing if (avail_in == 0)
	cmp	dword [rcx + _avail_in], 0
	jne	skip1

	;; Set stream's next state
	mov	rdx, ZSTATE_FLUSH_READ_BUFFER
	mov	rax, ZSTATE_BODY
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
	MOVDQA	[rsp + xmm_save_mem_offset + 0*16], xmm6
	MOVDQA	[rsp + xmm_save_mem_offset + 1*16], xmm7
	MOVDQA	[rsp + xmm_save_mem_offset + 2*16], xmm8
	MOVDQA	[rsp + xmm_save_mem_offset + 3*16], xmm9
	MOVDQA	[rsp + xmm_save_mem_offset + 4*16], xmm10
	MOVDQA	[rsp + xmm_save_mem_offset + 5*16], xmm11
	MOVDQA	[rsp + xmm_save_mem_offset + 6*16], xmm12

	mov	stream, rcx
	MOVDQU	xmask, [mask]

	MOVDQA	crc_0, [stream + _internal_state_crc + 0*16]
	MOVDQA	crc_1, [stream + _internal_state_crc + 1*16]
	MOVDQA	crc_2, [stream + _internal_state_crc + 2*16]
	MOVDQA	crc_3, [stream + _internal_state_crc + 3*16]
	MOVDQA	crc_fold, [fold_4]
	mov	dword [stream + _internal_state_has_eob], 0

	; state->bitbuf.set_buf(stream->next_out, stream->avail_out);
	mov	m_out_buf, [stream + _next_out]
	mov	[stream + _internal_state_bitbuf_m_out_start], m_out_buf
	mov	tmp1 %+ d, [stream + _avail_out]
	add	tmp1, m_out_buf
	sub	tmp1, SLOP
skip_SLOP:
	mov	[stream + _internal_state_bitbuf_m_out_end], tmp1

	mov	m_bits,           [stream + _internal_state_bitbuf_m_bits]
	mov	m_bit_count %+ d, [stream + _internal_state_bitbuf_m_bit_count]

	mov	hufftables, [stream + _hufftables]
	; in_buf = stream->next_in
	mov	in_buf,    [stream + _next_in]
	mov	blen %+ d, [stream + _avail_in]

	mov	dword [rsp + empty_buffer_flag], 0
	cmp	dword [stream + _internal_state_b_bytes_processed], 0
	sete	byte [rsp + empty_buffer_flag]

	; while (blen != 0)
MARK __Compute_X_ %+ ARCH
loop1:
	; x = D + LA - (state->b_bytes_valid - state->b_bytes_processed);
	mov	b_bytes_valid %+ d,     [stream + _internal_state_b_bytes_valid]
	mov	b_bytes_processed %+ d, [stream + _internal_state_b_bytes_processed]
	lea	x, [b_bytes_processed + D + LA]
	sub	x, b_bytes_valid

	; if (x > D) x = D;
	cmp	x, D
	cmova	x, [const_D]

	; if (blen < D) x = blen;
	cmp	blen, D
	cmovb	x, blen

	;; process x bytes starting at in_buf

	;; If there isn't enough room, shift buffer down
	; if (x > BSIZE - state->b_bytes_valid) {
	mov	tmp1, BSIZE
	sub	tmp1, b_bytes_valid
	cmp	x, tmp1
	jbe	skip_move

	; if (state->b_bytes_processed < state->b_bytes_valid - LA) {
	mov	tmp1, b_bytes_valid
	sub	tmp1, LA
	cmp	b_bytes_processed, tmp1
	jae	do_move

	;; We need to move an odd amount, skip move for this copy of loop
	xor	x,x
	mov	[rsp + blen_mem_offset], blen
	jmp	skip_move_zero

MARK __shift_data_down_ %+ ARCH
do_move:
	; offset = state->b_bytes_valid - (D + LA);
	mov	tmp4, b_bytes_valid
	sub	tmp4, D + LA
	; copy_D_LA(state->buffer, state->buffer + offset);
	lea	tmp1, [stream + _internal_state_buffer]
	lea	tmp2, [tmp1 + tmp4]
	copy_D_LA	tmp1, tmp2, tmp3, vtmp0, vtmp1, vtmp2, vtmp3
	; tmp1 clobbered

	; state->file_start        -= offset;
	sub	[stream + _internal_state_file_start], tmp4
	; state->b_bytes_processed -= offset;
	sub	b_bytes_processed, tmp4
	mov	b_bytes_valid, D + LA

MARK __copy_in_ %+ ARCH
skip_move:
	sub	blen, x

	mov	[rsp + blen_mem_offset], blen

	; copy_in(state->buffer + state->b_bytes_valid, in_buf, x);
	lea	tmp1, [stream + _internal_state_buffer + b_bytes_valid]
	mov	tmp2, in_buf
	mov	tmp3, x


	COPY_IN_CRC	tmp1, tmp2, tmp3, tmp4, crc_0, crc_1, crc_2, crc_3, crc_fold, \
				xtmp0, xtmp1, xtmp2, xtmp3, xtmp4

	; in_buf += x;
	add	in_buf, x
MARK __prepare_loop_ %+ ARCH
skip_move_zero:
	mov	[rsp + in_buf_mem_offset], in_buf
	; state->b_bytes_valid += x;
	add	b_bytes_valid, x
	mov	[stream + _internal_state_b_bytes_valid], b_bytes_valid %+ d

	; f_end_i   = state->b_bytes_valid - LA;
%ifnidn f_end_i, b_bytes_valid
	mov	f_end_i, b_bytes_valid
%endif
	sub	f_end_i, LA
	; if (f_end_i <= 0) continue;
	cmp	f_end_i, 0
	jle	continue_while

	; f_start_i = state->b_bytes_processed;
	;; f_i and b_bytes_processed are same register, just store b_bytes_proc
	mov	[stream + _internal_state_b_bytes_processed], b_bytes_processed %+ d

	; f_start_i += (uint32_t)(state->buffer - state->file_start);
	mov	file_start, [stream + _internal_state_file_start]
	lea	tmp1, [stream + _internal_state_buffer]
	sub	tmp1, file_start
	add	f_i, tmp1
	add	f_end_i, tmp1
	mov	[rsp + f_end_i_mem_offset], f_end_i

	; for (f_i = f_start_i; f_i < f_end_i; f_i++) {
	cmp	f_i, f_end_i
	jge	end_loop_2

MARK __misc_compute_hash_lookup_ %+ ARCH
	MOVDQU	xdata, [file_start + f_i]
	mov	curr_data, [file_start + f_i]
	mov	tmp3, curr_data
	mov	tmp6, curr_data

	compute_hash	hash, curr_data

	shr	tmp3, 8
	compute_hash	hash2, tmp3

	and	hash, HASH_MASK
	and	hash2, HASH_MASK

	cmp	dword [rsp + empty_buffer_flag], 0
	jne	write_first_byte

	jmp	loop2

	align	16

loop2:
	; if (state->bitbuf.is_full()) {
	cmp	m_out_buf, [stream + _internal_state_bitbuf_m_out_end]
	ja	bitbuf_full

	xor	dist, dist
	xor	dist2, dist2
	xor	tmp3, tmp3

	lea	tmp1, [file_start + f_i]

	mov	dist %+ w, f_i %+ w
	dec	dist
	sub	dist %+ w, word [stream + _internal_state_head + 2 * hash]
	mov	[stream + _internal_state_head + 2 * hash], f_i %+ w

	inc	f_i

	MOVQ	tmp6, xdata
	shr	tmp5, 16
	mov	tmp8, tmp5
	compute_hash	tmp6, tmp5

	mov	dist2 %+ w, f_i %+ w
	dec	dist2
	sub	dist2 %+ w, word [stream + _internal_state_head + 2 * hash2]
	mov	[stream + _internal_state_head + 2 * hash2], f_i %+ w

	; if ((dist-1) < (D-1)) {
	and	dist %+ d, (D-1)
	neg	dist

	shr	tmp8, 8
	compute_hash	tmp2, tmp8

	and	dist2 %+ d, (D-1)
	neg	dist2

MARK __compare_ %+ ARCH
	;; Check for long len/dist match (>7) with first literal
	MOVQ	len, xdata
	mov	curr_data, len
	PSRLDQ	xdata, 1
	xor	len, [tmp1 + dist - 1]
	jz	compare_loop

	MOVD	xhash, tmp6 %+ d
	PINSRD	xhash, tmp2 %+ d, 1
	PAND	xhash, xhash, xmask

	;; Check for len/dist match (>7) with second literal
	MOVQ	len2, xdata
	xor	len2, [tmp1 + dist2]
	jz	compare_loop2

	;; Specutively load the code for the first literal
	movzx   tmp1, curr_data %+ b
	get_lit_code    tmp1, code3, rcx, hufftables

	;; Check for len/dist match for first literal
	test    len %+ d, 0xFFFFFFFF
	jz      len_dist_huffman_pre

	;; Specutively load the code for the second literal
	shr     curr_data, 8
	and     curr_data, 0xff
	get_lit_code    curr_data, code2, code_len2, hufftables

	SHLX    code2, code2, rcx
	or      code2, code3
	add     code_len2, rcx

	;; Check for len/dist match for second literal
	test    len2 %+ d, 0xFFFFFFFF
	jnz     write_lit_bits

MARK __len_dist_lit_huffman_ %+ ARCH
len_dist_lit_huffman_pre:
	mov     code_len3, rcx
	bsf	len2, len2
	shr	len2, 3

len_dist_lit_huffman:
	neg	dist2

%ifndef LONGER_HUFFTABLE
	mov	tmp4, dist2
	get_dist_code	tmp4, code4, code_len2, hufftables ;; clobbers dist, rcx
%else
	get_dist_code	dist2, code4, code_len2, hufftables
%endif
	get_len_code	len2, code, rcx, hufftables ;; rcx is code_len

	SHLX	code4, code4, rcx
	or	code4, code
	add	code_len2, rcx

	add	f_i, len2
	neg	len2

	MOVQ	tmp5, xdata
	shr	tmp5, 24
	compute_hash	tmp4, tmp5
	and	tmp4, HASH_MASK

	SHLX	code4, code4, code_len3
	or	code4, code3
	add	code_len2, code_len3

	;; Setup for updating hash
	lea	tmp3, [f_i + len2 + 1]	; tmp3 <= k

	MOVDQU	xdata, [file_start + f_i]
	mov	curr_data, [file_start + f_i]
	mov	curr_data2, curr_data

	MOVD	hash %+ d, xhash
	PEXTRD	hash2 %+ d, xhash, 1
	mov	[stream + _internal_state_head + 2 * hash], tmp3 %+ w

	compute_hash	hash, curr_data

	add	tmp3,1
	mov	[stream + _internal_state_head + 2 * hash2], tmp3 %+ w

	add	tmp3, 1
	mov	[stream + _internal_state_head + 2 * tmp4], tmp3 %+ w

	write_bits	m_bits, m_bit_count, code4, code_len2, m_out_buf, tmp4
	mov	f_end_i, [rsp + f_end_i_mem_offset]

	shr	curr_data2, 8
	compute_hash	hash2, curr_data2

%ifdef NO_LIMIT_HASH_UPDATE
loop3:
	add	tmp3,1
	cmp	tmp3, f_i
	jae	loop3_done
	mov	tmp6, [file_start + tmp3]
	compute_hash	tmp4, tmp6
	and	tmp4 %+ d, HASH_MASK
	; state->head[hash] = k;
	mov	[stream + _internal_state_head + 2 * tmp4], tmp3 %+ w
	jmp	loop3
loop3_done:
%endif
	; hash = compute_hash(state->file_start + f_i) & HASH_MASK;
	and	hash %+ d, HASH_MASK
	and	hash2 %+ d, HASH_MASK

	; continue
	cmp	f_i, f_end_i
	jl	loop2
	jmp	end_loop_2
	;; encode as dist/len

MARK __len_dist_huffman_ %+ ARCH
len_dist_huffman_pre:
	bsf	len, len
	shr	len, 3
len_dist_huffman:
	dec	f_i
	neg	dist

	; get_dist_code(dist, &code2, &code_len2);
%ifndef LONGER_HUFFTABLE
	mov tmp3, dist	; since code2 and dist are rbx
	get_dist_code	tmp3, code2, code_len2, hufftables ;; clobbers dist, rcx
%else
	get_dist_code	dist, code2, code_len2, hufftables
%endif
	; get_len_code(len, &code, &code_len);
	get_len_code	len, code, rcx, hufftables ;; rcx is code_len

	; code2 <<= code_len
	; code2 |= code
	; code_len2 += code_len
	SHLX	code2, code2, rcx
	or	code2, code
	add	code_len2, rcx

	;; Setup for updateing hash
	lea	tmp3, [f_i + 2]	; tmp3 <= k
	add	f_i, len

	MOVD	hash %+ d, xhash
	PEXTRD	hash2 %+ d, xhash, 1
	mov	[stream + _internal_state_head + 2 * hash], tmp3 %+ w
	add	tmp3,1
	mov	[stream + _internal_state_head + 2 * hash2], tmp3 %+ w

	MOVDQU	xdata, [file_start + f_i]
	mov	curr_data, [file_start + f_i]
	mov	curr_data2, curr_data
	compute_hash	hash, curr_data

	write_bits	m_bits, m_bit_count, code2, code_len2, m_out_buf, tmp7
	mov	f_end_i, [rsp + f_end_i_mem_offset]

	shr	curr_data2, 8
	compute_hash	hash2, curr_data2

%ifdef NO_LIMIT_HASH_UPDATE
loop4:
	add	tmp3,1

	cmp	tmp3, f_i
	jae	loop4_done
	mov	tmp6, [file_start + tmp3]
	compute_hash	tmp4, tmp6
	and	tmp4, HASH_MASK
	mov	[stream + _internal_state_head + 2 * tmp4], tmp3 %+ w
	jmp	loop4
loop4_done:
%endif

	and	hash, HASH_MASK
	and	hash2, HASH_MASK

	; continue
	cmp	f_i, f_end_i
	jl	loop2
	jmp	end_loop_2

MARK __write_lit_bits_ %+ ARCH
write_lit_bits:
	MOVDQU	xdata, [file_start + f_i + 1]
	mov	f_end_i, [rsp + f_end_i_mem_offset]
	add	f_i, 1
	mov	curr_data, [file_start + f_i]

	MOVD	hash %+ d, xhash

	write_bits	m_bits, m_bit_count, code2, code_len2, m_out_buf, tmp3

	PEXTRD	hash2 %+ d, xhash, 1

	; continue
	cmp	f_i, f_end_i
	jl	loop2


MARK __end_loops_ %+ ARCH
end_loop_2:

	; state->b_bytes_processed = f_i - (state->buffer - state->file_start);
	add	f_i, [stream + _internal_state_file_start]
	sub	f_i, stream
	sub	f_i, _internal_state_buffer
	mov	[stream + _internal_state_b_bytes_processed], f_i %+ d

	; continue
continue_while:
	mov	blen, [rsp + blen_mem_offset]
	mov	in_buf, [rsp + in_buf_mem_offset]
	cmp	blen, 0
	jnz	loop1

end:
	;; update input buffer
	; stream->total_in += (uint32_t)(in_buf - stream->next_in); // bytes copied
	mov	tmp1 %+ d, [stream + _total_in]
	mov	in_buf, [rsp + in_buf_mem_offset]
	add	tmp1, in_buf
	sub	tmp1, [stream + _next_in]
	mov	[stream + _total_in], tmp1 %+ d

	mov	[stream + _next_in], in_buf
	mov	[stream + _avail_in], blen %+ d

	cmp	blen, 0
	jne	skip2

	;; Set stream's next state
	mov	tmp1, ZSTATE_FLUSH_READ_BUFFER
	mov	tmp5, ZSTATE_BODY
	cmp	dword [stream + _end_of_stream], 0
	cmovne	tmp5, tmp1
	cmp	dword [stream + _flush], _NO_FLUSH
	cmovne	tmp5, tmp1
	mov	dword [stream + _internal_state_state], tmp5 %+ d
skip2:
	mov	[stream + _next_out], m_out_buf
	; offset = state->bitbuf.buffer_used();
	sub	m_out_buf, [stream + _internal_state_bitbuf_m_out_start]
	sub	[stream + _avail_out], m_out_buf %+ d
	add	[stream + _total_out], m_out_buf %+ d

	mov	[stream + _internal_state_bitbuf_m_bits], m_bits
	mov	[stream + _internal_state_bitbuf_m_bit_count], m_bit_count %+ d


	MOVDQA	[stream + _internal_state_crc + 0*16], crc_0
	MOVDQA	[stream + _internal_state_crc + 1*16], crc_1
	MOVDQA	[stream + _internal_state_crc + 2*16], crc_2
	MOVDQA	[stream + _internal_state_crc + 3*16], crc_3

	mov rbx, [rsp + gpr_save_mem_offset + 0*8]
	mov rsi, [rsp + gpr_save_mem_offset + 1*8]
	mov rdi, [rsp + gpr_save_mem_offset + 2*8]
	mov rbp, [rsp + gpr_save_mem_offset + 3*8]
	mov r12, [rsp + gpr_save_mem_offset + 4*8]
	mov r13, [rsp + gpr_save_mem_offset + 5*8]
	mov r14, [rsp + gpr_save_mem_offset + 6*8]
	mov r15, [rsp + gpr_save_mem_offset + 7*8]
	MOVDQA	xmm6, [rsp + xmm_save_mem_offset + 0*16]
	MOVDQA	xmm7, [rsp + xmm_save_mem_offset + 1*16]
	MOVDQA	xmm8, [rsp + xmm_save_mem_offset + 2*16]
	MOVDQA	xmm9, [rsp + xmm_save_mem_offset + 3*16]
	MOVDQA	xmm10, [rsp + xmm_save_mem_offset + 4*16]
	MOVDQA	xmm11, [rsp + xmm_save_mem_offset + 5*16]
	MOVDQA	xmm12, [rsp + xmm_save_mem_offset + 6*16]

%ifndef ALIGN_STACK
	add	rsp, stack_size
%else
	mov	rsp, rbp
	pop	rbp
%endif
	ret

MARK __bitbuf_full_ %+ ARCH
bitbuf_full:
	mov	blen, [rsp + blen_mem_offset]
	; state->b_bytes_processed = f_i - (state->buffer - state->file_start);
	add	f_i, [stream + _internal_state_file_start]
	sub	f_i, stream
	sub	f_i, _internal_state_buffer
	mov	[stream + _internal_state_b_bytes_processed], f_i %+ d
	jmp	end

MARK __compare_loops_ %+ ARCH
compare_loop:
	MOVD	xhash, tmp6 %+ d
	PINSRD	xhash, tmp2 %+ d, 1
	PAND	xhash, xhash, xmask
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
	lea     tmp2, [tmp1 + dist2]
	add     tmp1, 1
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
	and	curr_data, 0xff
	get_lit_code	curr_data, code3, code_len3, hufftables
	jmp	len_dist_lit_huffman

MARK __write_first_byte_ %+ ARCH
write_first_byte:
	cmp	m_out_buf, [stream + _internal_state_bitbuf_m_out_end]
	ja	bitbuf_full

	mov	dword [rsp + empty_buffer_flag], 0

	mov	[stream + _internal_state_head + 2 * hash], f_i %+ w

	mov	hash, hash2
	shr	tmp6, 16
	compute_hash	hash2, tmp6

	MOVD	xhash, hash %+ d
	PINSRD	xhash, hash2 %+ d, 1
	PAND	xhash, xhash, xmask

	and	curr_data, 0xff
	get_lit_code	curr_data, code2, code_len2, hufftables
	jmp	write_lit_bits

section .data
	align 16
mask:	dd	HASH_MASK, HASH_MASK, HASH_MASK, HASH_MASK
const_D: dq	D

%endif ;; ifndef TEST
