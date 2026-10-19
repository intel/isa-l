;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;  Copyright(c) 2011-2020 Intel Corporation All rights reserved.
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

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;       Function API:
;       UINT32 crc32_iscsi_by16_10(
;               UINT32 init_crc, //initial CRC value, 32 bits
;               const unsigned char *buf, //buffer pointer to calculate CRC on
;               UINT64 len //buffer length in bytes (64-bit data)
;       );
;
;       Authors:
;               Erdinc Ozturk
;               Vinodh Gopal
;               James Guilford
;
;       Reference paper titled "Fast CRC Computation for Generic Polynomials Using PCLMULQDQ Instruction"
;       URL: http://www.intel.com/content/dam/www/public/us/en/documents/white-papers/fast-crc-computation-generic-polynomials-pclmulqdq-paper.pdf
;
;

%include "reg_sizes.asm"
%include "crc.inc"

%ifndef FUNCTION_NAME
%define FUNCTION_NAME crc32_iscsi_by16_10
%endif

%ifndef fetch_dist
%define	fetch_dist	1536
%endif

%ifndef PREFETCH
%define PREFETCH        prefetcht0
%endif

;; Parameters of the fused folding and crc32 in FUSED_CRC32 builds
%define FUSED_ITERS	7			; fold iterations per fused chunk
%define FUSED_VEC	(256 * FUSED_ITERS)	; folded bytes per fused chunk
%define FUSED_STREAM	(32 * FUSED_ITERS)	; bytes per crc32 stream per fused chunk
%define FUSED_CHUNK	(FUSED_VEC + 8 * FUSED_STREAM)

[bits 64]
default rel

section .text


%ifidn __OUTPUT_FORMAT__, win64
	%xdefine	arg1 r8
	%xdefine	arg2 rcx
	%xdefine	arg3 rdx

	%xdefine	arg1_low32 r8d
%else
	%xdefine	arg1 rdx
	%xdefine	arg2 rdi
	%xdefine	arg3 rsi

	%xdefine	arg1_low32 edx
%endif

%ifdef NO_VPTERNLOGQ
%macro vpternlogq 4
%if %4 == 0x96
	vpxorq		%1, %1, %2
	vpxorq		%1, %1, %3
%else
	vpternlogq	%1, %2, %3, %4	; fallback
%endif
%endmacro
%endif

align 64
mk_global FUNCTION_NAME, function
FUNCTION_NAME:
	endbranch
	lea		r10, [rel crc32_iscsi_const]
%ifidn __OUTPUT_FORMAT__, win64
	sub		rsp, (16*10 + 8)

	; push the xmm registers into the stack to maintain
	vmovdqa		[rsp +  16*0], xmm6
	vmovdqa		[rsp + 16*1], xmm7
	vmovdqa		[rsp + 16*2], xmm8
	vmovdqa		[rsp + 16*3], xmm9
	vmovdqa		[rsp + 16*4], xmm10
	vmovdqa		[rsp + 16*5], xmm11
	vmovdqa		[rsp + 16*6], xmm12
	vmovdqa		[rsp + 16*7], xmm13
	vmovdqa		[rsp + 16*8], xmm14
	vmovdqa		[rsp + 16*9], xmm15
%endif

	;; fastpath for short data
	mov		eax, arg1_low32
	cmp		arg3, 8
	jb		.less_than_8
	cmp		arg3, 16
	jbe		.no_more_than_16

	; check if smaller than 256B
	cmp		arg3, 256
	jl		.less_than_256

	; load the initial crc value
	vmovd		xmm10, arg1_low32      ; initial crc

	; receive the initial 64B data, xor the initial crc value
	vmovdqu8	zmm0, [arg2+16*0]
	vmovdqu8	zmm4, [arg2+16*4]
	vpxorq		zmm0, zmm10
	vbroadcasti32x4	zmm10, [r10 + crc_fold_const_fold_8x128b]	;xmm10 has rk3 and rk4
					;imm value of pclmulqdq instruction will determine which constant to use

	sub		arg3, 256
	cmp		arg3, 256
	jl		.fold_128_B_loop

	vmovdqu8	zmm7, [arg2+16*8]
	vmovdqu8	zmm8, [arg2+16*12]
	vbroadcasti32x4 zmm16, [r10 + crc_fold_const_fold_16x128b]	;zmm16 has rk-1 and rk-2
	sub		arg3, 256

%ifdef FUSED_CRC32
	; with room for a fused chunk and two blocks after it, also use crc32
	cmp		arg3, FUSED_CHUNK
	jae		.fused
%endif

%if fetch_dist != 0
	; check if there is at least 1.5KB (fetch distance) + 256B in the buffer
        cmp             arg3, (fetch_dist + 256)
        jb              .fold_256_B_loop

align 16
.fold_and_prefetch_256_B_loop:
	add		arg2, 256
	PREFETCH	[arg2+fetch_dist+0]
	vpclmulqdq	zmm1, zmm0, zmm16, 0x10
	vpclmulqdq	zmm0, zmm0, zmm16, 0x01
	vpternlogq	zmm0, zmm1, [arg2+16*0], 0x96

	PREFETCH	[arg2+fetch_dist+64]
	vpclmulqdq	zmm2, zmm4, zmm16, 0x10
	vpclmulqdq	zmm4, zmm4, zmm16, 0x01
	vpternlogq	zmm4, zmm2, [arg2+16*4], 0x96

	PREFETCH	[arg2+fetch_dist+64*2]
	vpclmulqdq	zmm3, zmm7, zmm16, 0x10
	vpclmulqdq	zmm7, zmm7, zmm16, 0x01
	vpternlogq	zmm7, zmm3, [arg2+16*8], 0x96

	PREFETCH	[arg2+fetch_dist+64*3]
	vpclmulqdq	zmm5, zmm8, zmm16, 0x10
	vpclmulqdq	zmm8, zmm8, zmm16, 0x01
	vpternlogq	zmm8, zmm5, [arg2+16*12], 0x96

	sub		arg3, 256

	; check if there is another 1.5KB (fetch distance) + 256B in the buffer
        cmp             arg3, (fetch_dist + 256)
	jge     	.fold_and_prefetch_256_B_loop
%endif ; fetch_dist != 0

align 16
.fold_256_B_loop:
	add		arg2, 256
	vpclmulqdq	zmm1, zmm0, zmm16, 0x10
	vpclmulqdq	zmm0, zmm0, zmm16, 0x01
	vpternlogq	zmm0, zmm1, [arg2+16*0], 0x96

	vpclmulqdq	zmm2, zmm4, zmm16, 0x10
	vpclmulqdq	zmm4, zmm4, zmm16, 0x01
	vpternlogq	zmm4, zmm2, [arg2+16*4], 0x96

	vpclmulqdq	zmm3, zmm7, zmm16, 0x10
	vpclmulqdq	zmm7, zmm7, zmm16, 0x01
	vpternlogq	zmm7, zmm3, [arg2+16*8], 0x96

	vpclmulqdq	zmm5, zmm8, zmm16, 0x10
	vpclmulqdq	zmm8, zmm8, zmm16, 0x01
	vpternlogq	zmm8, zmm5, [arg2+16*12], 0x96

	sub		arg3, 256
	jge     	.fold_256_B_loop

.fold_256_B_end:
	;; Fold 256 into 128
	add		arg2, 256
	vpclmulqdq	zmm1, zmm0, zmm10, 0x01
	vpclmulqdq	zmm2, zmm0, zmm10, 0x10
	vpternlogq	zmm7, zmm1, zmm2, 0x96	; xor ABC

	vpclmulqdq	zmm5, zmm4, zmm10, 0x01
	vpclmulqdq	zmm6, zmm4, zmm10, 0x10
	vpternlogq	zmm8, zmm5, zmm6, 0x96	; xor ABC

	vmovdqa32	zmm0, zmm7
	vmovdqa32	zmm4, zmm8

	add		arg3, 128
	jmp		.less_than_128_B

	; at this section of the code, there is 128*x+y (0<=y<128) bytes of buffer. The fold_128_B_loop
	; loop will fold 128B at a time until we have 128+y Bytes of buffer

	; fold 128B at a time. This section of the code folds 8 xmm registers in parallel
align 16
.fold_128_B_loop:
	add		arg2, 128
	vpclmulqdq	zmm2, zmm0, zmm10, 0x10
	vpclmulqdq	zmm0, zmm0, zmm10, 0x01
	vpternlogq	zmm0, zmm2, [arg2+16*0], 0x96

	vpclmulqdq	zmm5, zmm4, zmm10, 0x10
	vpclmulqdq	zmm4, zmm4, zmm10, 0x01
	vpternlogq	zmm4, zmm5, [arg2+16*4], 0x96

        sub		arg3, 128
	jge		.fold_128_B_loop
	;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

	add		arg2, 128
align 16
.less_than_128_B:
        ;; At this point, the buffer pointer is pointing at the last
        ;; y bytes of the buffer, where 0 <= y < 128.
        ;; The 128 bytes of folded data is in 2 of the zmm registers:
        ;;     zmm0 and zmm4

        cmp             arg3, -64
        jl              .fold_128_B_register

        vbroadcasti32x4 zmm10, [r10 + crc_fold_const_fold_4x128b]
        ;; If there are still 64 bytes left, folds from 128 bytes to 64 bytes
        ;; and handles the next 64 bytes
        vpclmulqdq      zmm2, zmm0, zmm10, 0x10
        vpclmulqdq      zmm0, zmm0, zmm10, 0x01
        vpternlogq      zmm0, zmm2, zmm4, 0x96
        add             arg3, 128

        jmp             .fold_64B_loop

align 16
.fold_128_B_register:
	; fold the 8 128b parts into 1 xmm register with different constants
	vmovdqu8	zmm16, [r10 + crc_fold_const_fold_7x128b]		; multiply by rk9-rk16
	vmovdqu8	zmm11, [r10 + crc_fold_const_fold_3x128b]		; multiply by rk17-rk20, rk1,rk2, 0,0
	vpclmulqdq	zmm1, zmm0, zmm16, 0x01
	vpclmulqdq	zmm2, zmm0, zmm16, 0x10
	vextracti64x2	xmm7, zmm4, 3		; save last that has no multiplicand

	vpclmulqdq	zmm5, zmm4, zmm11, 0x01
	vpclmulqdq	zmm6, zmm4, zmm11, 0x10
	vmovdqa		xmm10, [r10 + crc_fold_const_fold_1x128b]		; Needed later in reduction loop
	vpternlogq	zmm1, zmm2, zmm5, 0x96	; xor ABC
	vpternlogq	zmm1, zmm6, zmm7, 0x96	; xor ABC

	vshufi64x2      zmm8, zmm1, zmm1, 0x4e ; Swap 1,0,3,2 - 01 00 11 10
	vpxorq          ymm8, ymm8, ymm1
	vextracti64x2   xmm5, ymm8, 1
	vpxorq          xmm7, xmm5, xmm8

	; instead of 128, we add 128-16 to the loop counter to save 1 instruction from the loop
	; instead of a cmp instruction, we use the negative flag with the jl instruction
	add		arg3, 128-16
	jl		.final_reduction_for_128

	; now we have 16+y bytes left to reduce. 16 Bytes is in register xmm7 and the rest is in memory
	; we can fold 16 bytes at a time if y>=16
	; continue folding 16B at a time

align 16
.16B_reduction_loop:
	vpclmulqdq	xmm8, xmm7, xmm10, 0x1
	vpclmulqdq	xmm7, xmm7, xmm10, 0x10
        vpternlogq      xmm7, xmm8, [arg2], 0x96
	add		arg2, 16
	sub		arg3, 16
	; instead of a cmp instruction, we utilize the flags with the jge instruction
	; equivalent of: cmp arg3, 16-16
	; check if there is any more 16B in the buffer to be able to fold
	jge		.16B_reduction_loop

	;now we have 16+z bytes left to reduce, where 0<= z < 16.
	;first, we reduce the data in the xmm7 register


align 16
.final_reduction_for_128:
	add		arg3, 16
	je		.128_done

	; here we are getting data that is less than 16 bytes.
	; since we know that there was data before the pointer, we can offset
	; the input pointer before the actual point, to receive exactly 16 bytes.
	; after that the registers need to be adjusted.
align 16
.get_last_two_xmms:

	vmovdqa		xmm2, xmm7
	vmovdqu		xmm1, [arg2 - 16 + arg3]

	; get rid of the extra data that was loaded before
	; load the shift constant
	lea		rax, [rel shf_table_refl]
	add		rax, arg3
	vmovdqu		xmm0, [rax]

	vpshufb		xmm7, xmm0
	vpxor		xmm0, [rel shf_xor_mask]
	vpshufb		xmm2, xmm0

	vpblendvb	xmm2, xmm2, xmm1, xmm0
	;;;;;;;;;;
	vpclmulqdq	xmm8, xmm7, xmm10, 0x1
	vpclmulqdq	xmm7, xmm7, xmm10, 0x10
        vpternlogq      xmm7, xmm8, xmm2, 0x96

align 16
.128_done:
	; compute crc of a 128-bit value
	xor		rax, rax
	vmovq		r11, xmm7
	vpextrq		r10, xmm7, 1
	crc32		rax, r11
	crc32		rax, r10

align 16
.cleanup:

%ifidn __OUTPUT_FORMAT__, win64
	vmovdqa		xmm6, [rsp + 16*0]
	vmovdqa		xmm7, [rsp + 16*1]
	vmovdqa		xmm8, [rsp + 16*2]
	vmovdqa		xmm9, [rsp + 16*3]
	vmovdqa		xmm10, [rsp + 16*4]
	vmovdqa		xmm11, [rsp + 16*5]
	vmovdqa		xmm12, [rsp + 16*6]
	vmovdqa		xmm13, [rsp + 16*7]
	vmovdqa		xmm14, [rsp + 16*8]
	vmovdqa		xmm15, [rsp + 16*9]

	add		rsp, (16*10 + 8)
%endif
	ret


;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

align 16
.less_than_256:

	; check if there is enough buffer to be able to fold 16B at a time
	cmp	arg3, 32
	jl	.less_than_32

	vmovd	xmm1, arg1_low32	; get the initial crc value

	cmp	arg3, 64
	jl	.less_than_64

        ;; receive the initial 64B data, xor the initial crc value
        vmovdqu8        zmm0, [arg2]
        vpxorq          zmm0, zmm1
        add             arg2, 64
        sub             arg3, 64

        cmp             arg3, 64
        jb              .reduce_64B

        vbroadcasti32x4 zmm10, [r10 + crc_fold_const_fold_4x128b]

align 16
.fold_64B_loop:
        vmovdqu8        zmm4, [arg2]
        vpclmulqdq      zmm2, zmm0, zmm10, 0x10
        vpclmulqdq      zmm0, zmm0, zmm10, 0x01
        vpternlogq      zmm0, zmm2, zmm4, 0x96

        add             arg2, 64
        sub             arg3, 64

        cmp             arg3, 64
        jge             .fold_64B_loop

align 16
.reduce_64B:
        ; Reduce from 64 bytes to 16 bytes
	vmovdqu8	zmm11, [r10 + crc_fold_const_fold_3x128b]
	vpclmulqdq	zmm1, zmm0, zmm11, 0x01
	vpclmulqdq	zmm2, zmm0, zmm11, 0x10
	vextracti64x2	xmm7, zmm0, 3		; save last that has no multiplicand
        vpternlogq      zmm1, zmm2, zmm7, 0x96

	vmovdqa		xmm10, [r10 + crc_fold_const_fold_1x128b_b] ; Needed later in reduction loop

	vshufi64x2      zmm8, zmm1, zmm1, 0x4e ; Swap 1,0,3,2 - 01 00 11 10
	vpxorq          ymm8, ymm8, ymm1
	vextracti64x2   xmm5, ymm8, 1
	vpxorq          xmm7, xmm5, xmm8

        sub             arg3, 16
        jns             .16B_reduction_loop ; At least 16 bytes of data to digest
        jmp             .final_reduction_for_128

align 16
.less_than_64:
	;; if there is, load the constants
	vmovdqa	xmm10, [r10 + crc_fold_const_fold_1x128b_b]

	vmovdqu	xmm7, [arg2]		; load the plaintext
	vpxor	xmm7, xmm1              ; xmm1 already has initial crc value

	;; update the buffer pointer
	add	arg2, 16

        ;; update the counter
        ;; - subtract 32 instead of 16 to save one instruction from the loop
	sub	arg3, 32
	jmp	.16B_reduction_loop


align 16
.less_than_32:
	;; should be in the range [17, 31] bytes
	vmovd	xmm0, arg1_low32	; get the initial crc value

	vmovdqu	xmm7, [arg2]		; load the plaintext
	vpxor	xmm7, xmm0		; xor the initial crc value
	add	arg2, 16
	sub	arg3, 16
	vmovdqa	xmm10, [r10 + crc_fold_const_fold_1x128b]		; rk1 and rk2 in xmm10
	jmp	.get_last_two_xmms

; fastpath for short data
align 16
.no_more_than_16:
	test	arg3, 16		; check if exact 16 bytes
	jz	.less_than_16		; no, do 8 bytes check
	crc32	rax, qword[arg2]
	crc32	rax, qword[arg2+8]
	jmp	.cleanup		; done

align 16
.less_than_16:
	test	arg3, 8			; check if 8 bytes remaining at least
	jz	.less_than_8		; no, do 4 bytes check
	crc32	rax, qword[arg2]	; calculate 8 bytes anyway
	add	arg2, 8

.less_than_8:
	test	arg3, 4			; check if 4 bytes remaining at least
	jz	.less_than_4		; no, do 2 bytes check
	crc32	eax, dword[arg2]	; calculate 4 bytes anyway
	add	arg2, 4

.less_than_4:
	test	arg3, 2			; check if 2 bytes remaining at least
	jz	.less_than_2		; no, do 1 byte check
	crc32	eax, word[arg2]		; calculate 2 bytes anyway
	add	arg2, 2

.less_than_2:
	test	arg3, 1			; check if 1 byte remaining
	jz	.cleanup		; no, done
	crc32	eax, byte[arg2]		; calculate 1 byte
	jmp	.cleanup		; all done

%ifdef FUSED_CRC32
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Fused folding and crc32
;;
;; vpclmulqdq leaves the crc32 instruction idle, so for long buffers each
;; chunk of FUSED_CHUNK bytes is split into FUSED_VEC bytes for the folding
;; loop, followed by eight streams of FUSED_STREAM bytes for crc32. Each fold
;; iteration also takes 32 bytes of each stream. The folding treats the
;; streams as a gap: the first block after them is folded with constants for
;; 256 + 8 * FUSED_STREAM bytes. The stream crcs are shifted to the start of
;; the second block after the gap, combined, and xored into that block like an
;; initial crc, which gives their latency a block of folding to hide behind.
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

%define f_c0	rax
%define f_c1	rbx
%define f_c2	rbp
%define f_c3	r8
%define f_c4	r11
%define f_c5	r12
%define f_c6	r13
%define f_c7	r15
%define f_sp	r9
%define f_tmp	r14

;; Fold the accumulators with the constants in %1 into the block at arg2 + %2
%macro FUSED_FOLD 2
	vpclmulqdq	zmm1, zmm0, %1, 0x10
	vpclmulqdq	zmm0, zmm0, %1, 0x01
	vpternlogq	zmm0, zmm1, [arg2 + %2 + 64*0], 0x96
	vpclmulqdq	zmm2, zmm4, %1, 0x10
	vpclmulqdq	zmm4, zmm4, %1, 0x01
	vpternlogq	zmm4, zmm2, [arg2 + %2 + 64*1], 0x96
	vpclmulqdq	zmm3, zmm7, %1, 0x10
	vpclmulqdq	zmm7, zmm7, %1, 0x01
	vpternlogq	zmm7, zmm3, [arg2 + %2 + 64*2], 0x96
	vpclmulqdq	zmm5, zmm8, %1, 0x10
	vpclmulqdq	zmm8, zmm8, %1, 0x01
	vpternlogq	zmm8, zmm5, [arg2 + %2 + 64*3], 0x96
%endmacro

;; crc32 of the 32 bytes at offset 32 * %1 of each stream
%macro FUSED_SCALAR 1
%assign %%t 0
%rep 4
	crc32	f_c0, qword [f_sp + 0*FUSED_STREAM + 32*%1 + 8*%%t]
	crc32	f_c1, qword [f_sp + 1*FUSED_STREAM + 32*%1 + 8*%%t]
	crc32	f_c2, qword [f_sp + 2*FUSED_STREAM + 32*%1 + 8*%%t]
	crc32	f_c3, qword [f_sp + 3*FUSED_STREAM + 32*%1 + 8*%%t]
	crc32	f_c4, qword [f_sp + 4*FUSED_STREAM + 32*%1 + 8*%%t]
	crc32	f_c5, qword [f_sp + 5*FUSED_STREAM + 32*%1 + 8*%%t]
	crc32	f_c6, qword [f_sp + 6*FUSED_STREAM + 32*%1 + 8*%%t]
	crc32	f_c7, qword [f_sp + 7*FUSED_STREAM + 32*%1 + 8*%%t]
%assign %%t %%t+1
%endrep
%endmacro

;; Start the streams of the chunk at arg2
%macro FUSED_START 0
	lea	f_sp, [arg2 + FUSED_VEC]
	xor	f_c0 %+ d, f_c0 %+ d
	xor	f_c1 %+ d, f_c1 %+ d
	xor	f_c2 %+ d, f_c2 %+ d
	xor	f_c3 %+ d, f_c3 %+ d
	xor	f_c4 %+ d, f_c4 %+ d
	xor	f_c5 %+ d, f_c5 %+ d
	xor	f_c6 %+ d, f_c6 %+ d
	xor	f_c7 %+ d, f_c7 %+ d
%endmacro

;; Combine the stream crcs into xmm19. Stream i is shifted over the
;; (7 - i) * FUSED_STREAM + 256 bytes before the second block after the
;; streams: crc32(0, xor of clmul(crc_i, x^(8 * shift_i - 33) mod P)).
;; zmm20 holds the eight crcs, and two vpclmulqdq multiply them all.
%macro FUSED_COMBINE 0
	vmovq		xmm20, f_c0
	vpinsrq		xmm20, xmm20, f_c1, 1
	vmovq		xmm21, f_c2
	vpinsrq		xmm21, xmm21, f_c3, 1
	vmovq		xmm22, f_c4
	vpinsrq		xmm22, xmm22, f_c5, 1
	vmovq		xmm23, f_c6
	vpinsrq		xmm23, xmm23, f_c7, 1
	vinserti32x4	ymm20, ymm20, xmm21, 1
	vinserti32x4	ymm22, ymm22, xmm23, 1
	vinserti64x4	zmm20, zmm20, ymm22, 1
	vpclmulqdq	zmm21, zmm20, zmm18, 0x00
	vpclmulqdq	zmm20, zmm20, zmm18, 0x11
	vpxorq		zmm20, zmm20, zmm21
	vextracti64x4	ymm21, zmm20, 1
	vpxorq		ymm20, ymm20, ymm21
	vextracti32x4	xmm21, ymm20, 1
	vpxorq		xmm20, xmm20, xmm21
	vmovq		f_tmp, xmm20
	xor		eax, eax
	crc32		rax, f_tmp
	vmovd		xmm19, eax
%endmacro

align 32
.fused:
	push	rbx
	push	rbp
	push	r12
	push	r13
	push	r14
	push	r15
	vbroadcasti32x4	zmm17, [rel fused_fold_gap]
	vmovdqu64	zmm18, [rel fused_shift]

	;; The first block of the first chunk is already in the accumulators
	FUSED_START
	FUSED_SCALAR 0
%assign i 1
%rep FUSED_ITERS - 1
	FUSED_FOLD	zmm16, 256*i
	FUSED_SCALAR	i
%assign i i+1
%endrep
	FUSED_COMBINE
	add	arg2, FUSED_CHUNK
	sub	arg3, FUSED_CHUNK
	cmp	arg3, FUSED_CHUNK
	jb	.fused_end

align 32
.fused_loop:
	FUSED_START
	FUSED_FOLD	zmm17, 0
	FUSED_SCALAR	0
	FUSED_FOLD	zmm16, 256
	vpxorq		zmm0, zmm0, zmm19
	FUSED_SCALAR	1
%assign i 2
%rep FUSED_ITERS - 2
	FUSED_FOLD	zmm16, 256*i
	FUSED_SCALAR	i
%assign i i+1
%endrep
	FUSED_COMBINE
	add	arg2, FUSED_CHUNK
	sub	arg3, FUSED_CHUNK
	cmp	arg3, FUSED_CHUNK
	jae	.fused_loop

.fused_end:
	;; The two blocks after the last streams take the gap and their crc
	FUSED_FOLD	zmm17, 0
	FUSED_FOLD	zmm16, 256
	vpxorq		zmm0, zmm0, zmm19

	pop	r15
	pop	r14
	pop	r13
	pop	r12
	pop	rbp
	pop	rbx

	;; Continue in the folding loop after the second block
	add	arg2, 256
	sub	arg3, 256
	jge	.fold_256_B_loop
	jmp	.fold_256_B_end

section .rodata
align 64
;; x^(8 * ((7 - i) * FUSED_STREAM + 256) - 33) mod P for stream i
fused_shift:
	dq 0x00000000ded288f8, 0x000000002664fd8b
	dq 0x0000000093e106a4, 0x0000000086d8e4d2
	dq 0x000000006956fc3b, 0x00000000e6fc4e6a
	dq 0x00000000b3e32c28, 0x00000000b9e02b86
;; Folding constants for 256 + 8 * FUSED_STREAM bytes
fused_fold_gap:
	dq 0x00000001a0f717c4, 0x00000000f7506984
%endif ; FUSED_CRC32

%include "crc_const_extern.asm"
