default rel

%include "reg_sizes.asm"

%define DECOMP_OK 0
%define END_INPUT 1
%define OUT_OVERFLOW 2
%define INVALID_BLOCK -1
%define INVALID_SYMBOL -2
%define INVALID_LOOKBACK -3

%define ISAL_DECODE_LONG_BITS 12
%define ISAL_DECODE_SHORT_BITS 10

%define MAX_LONG_CODE_LARGE (288 + (1 << (15 - ISAL_DECODE_LONG_BITS)))
%define MAX_LONG_CODE_SMALL (32 + (1 << (15 - ISAL_DECODE_SHORT_BITS)))

%define COPY_SIZE 16
%define	COPY_LEN_MAX 258

%define IN_BUFFER_SLOP 8
%define OUT_BUFFER_SLOP	COPY_SIZE + COPY_LEN_MAX

%include "inflate_data_structs.asm"
%include "stdmac.asm"

extern rfc1951_lookup_table

;; rax
%define	tmp3		rax
%define	read_in_2	rax
%define	look_back_dist	rax

;; rcx
;; rdx	arg3
%define	next_sym2	rdx
%define copy_start	rdx
%define	tmp4		rdx

;; rdi	arg1
%define	tmp1		rdi
%define look_back_dist2 rdi
%define next_bits2	rdi
%define next_sym3	rdi

;; rsi	arg2
%define	tmp2		rsi
%define	next_bits	rsi

;; rbx ; Saved
%define	next_in		rbx

;; rbp ; Saved
%define	end_in		rbp

;; r8
%define	repeat_length	r8

;; r9
%define	read_in		r9

;; r10
%define read_in_length	r10

;; r11
%define	state		r11

;; r12 ; Saved
%define next_out	r12

;; r13 ; Saved
%define	end_out		r13

;; r14 ; Saved
%define	next_sym	r14

;; r15 ; Saved
%define rfc_lookup	r15

start_out_mem_offset	equ	0
read_in_mem_offset	equ	8
read_in_length_mem_offset	equ	16
gpr_save_mem_offset	equ	24
stack_size		equ	3 * 8 + 8 * 8

%define	_dist_extra_bit_count	264
%define	_dist_start		_dist_extra_bit_count + 1*32
%define	_len_extra_bit_count	_dist_start + 4*32
%define	_len_start		_len_extra_bit_count + 1*32

%ifidn __OUTPUT_FORMAT__, elf64
%define arg0	rdi

%macro FUNC_SAVE 0
%ifdef ALIGN_STACK
	push	rbp
	mov	rbp, rsp
	sub	rsp, stack_size
	and	rsp, ~15
%else
	sub	rsp, stack_size
%endif

	mov [rsp + gpr_save_mem_offset + 0*8], rbx
	mov [rsp + gpr_save_mem_offset + 1*8], rbp
	mov [rsp + gpr_save_mem_offset + 2*8], r12
	mov [rsp + gpr_save_mem_offset + 3*8], r13
	mov [rsp + gpr_save_mem_offset + 4*8], r14
	mov [rsp + gpr_save_mem_offset + 5*8], r15
%endm

%macro FUNC_RESTORE 0
	mov	rbx, [rsp + gpr_save_mem_offset + 0*8]
	mov	rbp, [rsp + gpr_save_mem_offset + 1*8]
	mov	r12, [rsp + gpr_save_mem_offset + 2*8]
	mov	r13, [rsp + gpr_save_mem_offset + 3*8]
	mov	r14, [rsp + gpr_save_mem_offset + 4*8]
	mov	r15, [rsp + gpr_save_mem_offset + 5*8]

%ifndef ALIGN_STACK
	add	rsp, stack_size
%else
	mov	rsp, rbp
	pop	rbp
%endif
%endm
%endif

%ifidn __OUTPUT_FORMAT__, win64
%define arg0	rcx
%macro FUNC_SAVE 0
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
%endm

%macro FUNC_RESTORE 0
	mov	rbx, [rsp + gpr_save_mem_offset + 0*8]
	mov	rsi, [rsp + gpr_save_mem_offset + 1*8]
	mov	rdi, [rsp + gpr_save_mem_offset + 2*8]
	mov	rbp, [rsp + gpr_save_mem_offset + 3*8]
	mov	r12, [rsp + gpr_save_mem_offset + 4*8]
	mov	r13, [rsp + gpr_save_mem_offset + 5*8]
	mov	r14, [rsp + gpr_save_mem_offset + 6*8]
	mov	r15, [rsp + gpr_save_mem_offset + 7*8]

%ifndef ALIGN_STACK
	add	rsp, stack_size
%else
	mov	rsp, rbp
	pop	rbp
%endif
%endm
%endif

;; Load read_in and updated in_buffer accordingly
;; when there are at least 8 bytes in the in buffer
;; Clobbers rcx, unless rcx is %%read_in_length
%macro inflate_in_load 6
%define	%%next_in		%1
%define %%end_in		%2
%define %%read_in		%3
%define %%read_in_length	%4
%define %%tmp1			%5 ; Tmp registers
%define %%tmp2			%6

	SHLX	%%tmp1, [%%next_in], %%read_in_length
	or	%%read_in, %%tmp1

	mov	%%tmp1, 64
	sub	%%tmp1, %%read_in_length
	shr	%%tmp1, 3

	add	%%next_in, %%tmp1
	lea	%%read_in_length, [%%read_in_length + 8 * %%tmp1]
%%end:
%endm

;; Load read_in and updated in_buffer accordingly
;; Clobbers rcx, unless rcx is %%read_in_length
%macro inflate_in_small_load 6
%define	%%next_in		%1
%define %%end_in		%2
%define %%read_in		%3
%define %%read_in_length	%4
%define %%avail_in		%5 ; Tmp registers
%define %%tmp1			%5
%define %%loop_count		%6

	mov	%%avail_in, %%end_in
	sub	%%avail_in, %%next_in

%ifnidn %%read_in_length, rcx
	mov	rcx, %%read_in_length
%endif

	mov	%%loop_count, 64
	sub	%%loop_count, %%read_in_length
	shr	%%loop_count, 3

	cmp	%%loop_count, %%avail_in
	cmovg	%%loop_count, %%avail_in
	cmp	%%loop_count, 0
	je	%%end

%%load_byte:
	xor	%%tmp1, %%tmp1
	mov	%%tmp1 %+ b, byte [%%next_in]
	SHLX	%%tmp1, %%tmp1, rcx
	or	%%read_in, %%tmp1
	add	rcx, 8
	add	%%next_in, 1
	sub	%%loop_count, 1
	jg	%%load_byte
%ifnidn %%read_in_length, rcx
	mov	%%read_in_length, rcx
%endif
%%end:
%endm

;; Decode next symbol
;; Clobber rcx
%macro decode_next		8
%define	%%state			%1 ; State structure associated with compressed stream
%define %%lookup_size		%2 ; Number of bits used for small lookup
%define	%%state_offset		%3
%define %%read_in		%4 ; Bits read in from compressed stream
%define %%read_in_length	%5 ; Number of valid bits in read_in
%define %%next_sym		%6 ; Returned symobl
%define %%next_bits		%7
%define	%%next_bits2		%8

	;; Lookup possible next symbol
	mov	%%next_bits, %%read_in
	and	%%next_bits, (1 << %%lookup_size) - 1
	movzx	%%next_sym, word [%%state + %%state_offset + 2 * %%next_bits]

	;; Save length associated with symbol
	mov	rcx, %%next_sym
	shr	rcx, 9
	jz	invalid_symbol

	;; Check if symbol or hint was looked up
	and	%%next_sym, 0x81FF
	cmp	%%next_sym, 0x8000
	jl	%%end

	;; Decode next_sym using hint
	mov	%%next_bits2, %%read_in

	;; Extract the 15-DECODE_LOOKUP_SIZE bits beyond the first DECODE_LOOKUP_SIZE bits.
%ifdef USE_HSWNI
	and	rcx, 0x1F
	bzhi	%%next_bits2, %%next_bits2, rcx
%else
	neg	rcx
	shl	%%next_bits2, cl
	shr	%%next_bits2, cl
%endif
	shr	%%next_bits2, %%lookup_size

	add	%%next_bits2, %%next_sym

	;; Lookup actual next symbol
	movzx	%%next_sym, word [%%state + %%state_offset + 2 * %%next_bits2 + 2 *((1 << %%lookup_size) - 0x8000)]

	;; Save length associated with symbol
	mov	rcx, %%next_sym
	shr	rcx, 9
	jz	invalid_symbol
	and	%%next_sym, 0x1FF
%%end:
	;; Updated read_in to reflect the bits which were decoded
	sub	%%read_in_length, rcx
	SHRX	%%read_in, %%read_in, rcx
%endm


;; Decode next symbol
;; Clobber rcx
%macro decode_next2		7
%define	%%state			%1 ; State structure associated with compressed stream
%define %%lookup_size		%2 ; Number of bits used for small lookup
%define	%%state_offset		%3 ; Type of huff code, should be either LIT or DIST
%define %%read_in		%4 ; Bits read in from compressed stream
%define %%read_in_length	%5 ; Number of valid bits in read_in
%define %%next_sym		%6 ; Returned symobl
%define	%%next_bits2		%7

	;; Save length associated with symbol
	mov	%%next_bits2, %%read_in
	shr	%%next_bits2, %%lookup_size

	mov	rcx, %%next_sym
	shr	rcx, 9
	jz	invalid_symbol

	;; Check if symbol or hint was looked up
	and	%%next_sym, 0x81FF
	cmp	%%next_sym, 0x8000
	jl	%%end

	;; Extract the 15-DECODE_LOOKUP_SIZE bits beyond the first %%lookup_size bits.
	lea	%%next_sym, [%%state + 2 * %%next_sym]
	sub	rcx, 0x40 + %%lookup_size

%ifdef USE_HSWNI
	bzhi	%%next_bits2, %%next_bits2, rcx
%else
	;; Decode next_sym using hint
	neg	rcx
	shl	%%next_bits2, cl
	shr	%%next_bits2, cl
%endif

	;; Lookup actual next symbol
	movzx	%%next_sym, word [%%next_sym + %%state_offset + 2 * %%next_bits2 + 2 * ((1 << %%lookup_size) - 0x8000)]

	;; Save length associated with symbol
	mov	rcx, %%next_sym
	shr	rcx, 9
	jz	invalid_symbol
	and	%%next_sym, 0x1FF

%%end:
	;; Updated read_in to reflect the bits which were decoded
	SHRX	%%read_in, %%read_in, rcx
	sub	%%read_in_length, rcx
%endm

global decode_huffman_code_block_stateless_ %+ ARCH
decode_huffman_code_block_stateless_ %+ ARCH %+ :

	FUNC_SAVE

	mov	state, arg0
	lea	rfc_lookup, [rfc1951_lookup_table]

	mov	read_in,[state + _read_in]
	mov	read_in_length %+ d, dword [state + _read_in_length]
	mov	next_out, [state + _next_out]
	mov	end_out %+ d, dword [state + _avail_out]
	add	end_out, next_out
	mov	next_in, [state + _next_in]
	mov	end_in %+ d, dword [state + _avail_in]
	add	end_in, next_in

	mov	dword [state + _copy_overflow_len], 0
	mov	dword [state + _copy_overflow_dist], 0

	mov	tmp3 %+ d, dword [state + _total_out]
	sub	tmp3, next_out
	neg	tmp3

	mov	[rsp + start_out_mem_offset], tmp3

	sub	end_out, OUT_BUFFER_SLOP
	sub	end_in, IN_BUFFER_SLOP

	cmp	next_in, end_in
	jg	end_loop_block_pre

	cmp	read_in_length, 64
	je	skip_load

	inflate_in_load	next_in, end_in, read_in, read_in_length, tmp1, tmp2

skip_load:
	mov	tmp3, read_in
	and	tmp3, (1 << ISAL_DECODE_LONG_BITS) - 1
	movzx	next_sym, word [state + _lit_huff_code + 2 * tmp3]

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; Main Loop
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
loop_block:
	;; Check if near end of in buffer or out buffer
	cmp	next_in, end_in
	jg	end_loop_block_pre
	cmp	next_out, end_out
	jg	end_loop_block_pre

	;; Decode next symbol and reload the read_in buffer
	decode_next2	state, ISAL_DECODE_LONG_BITS, _lit_huff_code, read_in, read_in_length, next_sym, tmp1

	;; Save next_sym in next_sym2 so next_sym can be preloaded
	mov	next_sym2, next_sym

	;; Find index to specutively preload next_sym from
	mov	tmp3, read_in
	and	tmp3, (1 << ISAL_DECODE_LONG_BITS) - 1

	;; Start reloading read_in
	mov	tmp1, [next_in]
	SHLX	tmp1, tmp1, read_in_length
	or	read_in, tmp1

	;; Specutively load data associated with length symbol
	movzx	rcx, byte [rfc_lookup + _len_extra_bit_count + next_sym2 - 257]
	movzx	repeat_length, word [rfc_lookup + _len_start + 2 * (next_sym2 - 257)]

	;; Test for end of block symbol
	cmp	next_sym2, 256
	je	end_symbol_pre

	;; Specutively load next_sym for next loop if a literal was decoded
	movzx	next_sym, word [state + _lit_huff_code + 2 * tmp3]

	;; Finish updating read_in_length for read_in
	mov	tmp1, 64
	sub	tmp1, read_in_length
	shr	tmp1, 3
	add	next_in, tmp1
	lea	read_in_length, [read_in_length + 8 * tmp1]

	;; Specultively load next dist code
	SHRX	read_in_2, read_in, rcx
	mov	next_bits2, read_in_2
	and	next_bits2, (1 << ISAL_DECODE_SHORT_BITS) - 1
	movzx	next_sym3, word [state + _dist_huff_code + 2 * next_bits2]

	;; Specutively write next_sym2 if it is a literal
	mov	[next_out], next_sym2
	add	next_out, 1

	;; Check if next_sym2 is a literal, length, or end of block symbol
	cmp	next_sym2, 256
	jl	loop_block

decode_len_dist:
	;; Find length for length/dist pair
	mov	next_bits, read_in

	BZHI	next_bits, next_bits, rcx, tmp4
	add	repeat_length, next_bits

	;; Update read_in for the length extra bits which were read in
	sub	read_in_length, rcx

	;; Decode distance code
	decode_next2 state, ISAL_DECODE_SHORT_BITS, _dist_huff_code, read_in_2, read_in_length, next_sym3, tmp2

	movzx	rcx, byte [rfc_lookup + _dist_extra_bit_count + next_sym3]
	mov	look_back_dist2 %+ d, [rfc_lookup + _dist_start + 4 * next_sym3]

	;; Load distance code extra bits
	mov	next_bits, read_in_2

	;; Determine next_out after the copy is finished
	add	next_out, repeat_length
	sub	next_out, 1

	;; Calculate the look back distance
	BZHI	next_bits, next_bits, rcx, tmp4
	SHRX	read_in_2, read_in_2, rcx

	;; Setup next_sym, read_in, and read_in_length for next loop
	mov	read_in, read_in_2
	and	read_in_2, (1 << ISAL_DECODE_LONG_BITS) - 1
	movzx	next_sym, word [state + _lit_huff_code + 2 * read_in_2]
	sub	read_in_length, rcx

	;; Copy distance in len/dist pair
	add	look_back_dist2, next_bits

	;; Find beginning of copy
	mov	copy_start, next_out
	sub	copy_start, repeat_length
	sub	copy_start, look_back_dist2

	;; Check if a valid look back distances was decoded
	cmp	copy_start, [rsp + start_out_mem_offset]
	jl	invalid_look_back_distance
	MOVDQU	xmm1, [copy_start]

	;; Set tmp2 to be the minimum of COPY_SIZE and repeat_length
	;; This is to decrease use of small_byte_copy branch
	mov	tmp2, COPY_SIZE
	cmp	tmp2, repeat_length
	cmovg	tmp2, repeat_length

	;; Check for overlapping memory in the copy
	cmp	look_back_dist2, tmp2
	jl	small_byte_copy_pre

large_byte_copy:
	;; Copy length distance pair when memory overlap is not an issue
	MOVDQU [copy_start + look_back_dist2], xmm1

	sub	repeat_length, COPY_SIZE
	jle	loop_block

	add	copy_start, COPY_SIZE
	MOVDQU	xmm1, [copy_start]
	jmp	large_byte_copy

small_byte_copy_pre:
	;; Copy length distance pair when source and destination overlap
	add	repeat_length, look_back_dist2
small_byte_copy:
	MOVDQU [copy_start + look_back_dist2], xmm1

	shl	look_back_dist2, 1
	MOVDQU	xmm1, [copy_start]
	cmp	look_back_dist2, COPY_SIZE
	jl	small_byte_copy

	sub	repeat_length, look_back_dist2
	jge	large_byte_copy
	jmp	loop_block

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; Finish Main Loop
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
end_loop_block_pre:
	;; Fix up in buffer and out buffer to reflect the actual buffer end
	add	end_out, OUT_BUFFER_SLOP
	add	end_in, IN_BUFFER_SLOP

end_loop_block:
	;; Load read in buffer and decode next lit/len symbol
	inflate_in_small_load	next_in, end_in, read_in, read_in_length, tmp1, tmp2
	mov	[rsp + read_in_mem_offset], read_in
	mov	[rsp + read_in_length_mem_offset], read_in_length

	decode_next state, ISAL_DECODE_LONG_BITS, _lit_huff_code, read_in, read_in_length, next_sym, tmp1, tmp2

	;; Check that enough input was available to decode symbol
	cmp	read_in_length, 0
	jl	end_of_input

	cmp	next_sym, 256
	jl	decode_literal
	je	end_symbol

decode_len_dist_2:
	;; Load length exta bits
	mov	next_bits, read_in

	movzx	repeat_length, word [rfc_lookup + _len_start + 2 * (next_sym - 257)]
	movzx	rcx, byte [rfc_lookup + _len_extra_bit_count + next_sym - 257]

	;; Calculate repeat length
	BZHI	next_bits, next_bits, rcx, tmp1
	add	repeat_length, next_bits

	;; Update read_in for the length extra bits which were read in
	SHRX	read_in, read_in, rcx
	sub	read_in_length, rcx

	;; Decode distance code
	decode_next state, ISAL_DECODE_SHORT_BITS, _dist_huff_code, read_in, read_in_length, next_sym, tmp1, tmp2

	;; Load distance code extra bits
	mov	next_bits, read_in
	mov	look_back_dist %+ d, [rfc_lookup + _dist_start + 4 * next_sym]
	movzx	rcx, byte [rfc_lookup + _dist_extra_bit_count + next_sym]


	;; Calculate the look back distance and check for enough input
	BZHI	next_bits, next_bits, rcx, tmp1
	SHRX	read_in, read_in, rcx
	add	look_back_dist, next_bits
	sub	read_in_length, rcx
	jl	end_of_input

	;; Setup code for byte copy using rep  movsb
	mov	rsi, next_out
	mov	rdi, rsi
	mov	rcx, repeat_length
	sub	rsi, look_back_dist

	;; Check if a valid look back distance was decoded
	cmp	rsi, [rsp + start_out_mem_offset]
	jl	invalid_look_back_distance

	;; Check for out buffer overflow
	add	repeat_length, next_out
	cmp	repeat_length, end_out
	jg	out_buffer_overflow_repeat

	mov	next_out, repeat_length

	rep	movsb
	jmp	end_loop_block

decode_literal:
	;; Store literal decoded from the input stream
	cmp	next_out, end_out
	jge	out_buffer_overflow_lit
	add	next_out, 1
	mov	byte [next_out - 1], next_sym %+ b
	jmp	end_loop_block

;; Set exit codes
end_of_input:
	mov	read_in, [rsp + read_in_mem_offset]
	mov	read_in_length, [rsp + read_in_length_mem_offset]
	mov	rax, END_INPUT
	jmp	end

out_buffer_overflow_repeat:
	mov	rcx, end_out
	sub	rcx, next_out
	sub	repeat_length, rcx
	sub	repeat_length, next_out
	rep	movsb

	mov	[state + _copy_overflow_len], repeat_length %+ d
	mov	[state + _copy_overflow_dist], look_back_dist %+ d

	mov	next_out, end_out

	mov	rax, OUT_OVERFLOW
	jmp	end

out_buffer_overflow_lit:
	mov	read_in, [rsp + read_in_mem_offset]
	mov	read_in_length, [rsp + read_in_length_mem_offset]
	mov	rax, OUT_OVERFLOW
	jmp	end

invalid_look_back_distance:
	mov	rax, INVALID_LOOKBACK
	jmp	end

invalid_symbol:
	mov	rax, INVALID_SYMBOL
	jmp	end

end_symbol_pre:
	;; Fix up in buffer and out buffer to reflect the actual buffer
	add	end_out, OUT_BUFFER_SLOP
	add	end_in, IN_BUFFER_SLOP
end_symbol:
	;;  Set flag identifying a new block is required
	mov	byte [state + _block_state], ISAL_BLOCK_NEW_HDR
	xor	rax, rax
end:
	;; Save current buffer states
	mov	[state + _read_in], read_in
	mov	[state + _read_in_length], read_in_length %+ d
	mov	[state + _next_out], next_out
	sub	end_out, next_out
	mov	dword [state + _avail_out], end_out %+ d
	sub	next_out, [rsp + start_out_mem_offset]
	mov	[state + _total_out], next_out %+ d
	mov	[state + _next_in], next_in
	sub	end_in, next_in
	mov	[state + _avail_in], end_in %+ d

	FUNC_RESTORE

	ret
