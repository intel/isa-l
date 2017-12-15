;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;  Copyright(c) 2011-2017 Intel Corporation All rights reserved.
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
;
;       Function API:
;       UINT16 crc16_t10dif_copy_by4(
;               UINT16 init_crc, //initial CRC value, 16 bits
;               unsigned char *dst, //buffer pointer destination for copy
;               const unsigned char *src, //buffer pointer to calculate CRC on
;               UINT64 len //buffer length in bytes (64-bit data)
;       );
;
;       Authors:
;               Erdinc Ozturk
;               Vinodh Gopal
;               James Guilford
;
;       Reference paper titled "Fast CRC Computation for Generic Polynomials Using PCLMULQDQ Instruction"
;       URL: http://download.intel.com/design/intarch/papers/323102.pdf
;

%include "reg_sizes.asm"

%define	fetch_dist	1024

[bits 64]
default rel

section .text
%ifidn __OUTPUT_FORMAT__, win64
	%xdefine        arg1 rcx
	%xdefine        arg2 rdx
	%xdefine        arg3 r8
	%xdefine        arg4 r9
	%xdefine        tmp1 r10
	%xdefine        arg1_low32 ecx
%else
	%xdefine        arg1 rdi
	%xdefine        arg2 rsi
	%xdefine        arg3 rdx
	%xdefine        arg4 rcx
	%xdefine	tmp1 r10
	%xdefine        arg1_low32 edi
%endif

align 16
global	crc16_t10dif_copy_by4:function
crc16_t10dif_copy_by4:

	; adjust the 16-bit initial_crc value, scale it to 32 bits
	shl	arg1_low32, 16

	; After this point, code flow is exactly same as a 32-bit CRC.
	; The only difference is before returning eax, we will shift
	; it right 16 bits, to scale back to 16 bits.

	sub	rsp,16*4+8

	; push the xmm registers into the stack to maintain
	movdqa [rsp+16*2],xmm6
	movdqa [rsp+16*3],xmm7

	; check if smaller than 128B
	cmp	arg4, 128

	; for sizes less than 128, we can't fold 64B at a time...
	jl	_less_than_128


	; load the initial crc value
	movd	xmm6, arg1_low32	; initial crc

	; crc value does not need to be byte-reflected, but it needs to
	; be moved to the high part of the register.
	; because data will be byte-reflected and will align with
	; initial crc at correct place.
	pslldq	xmm6, 12

	movdqa xmm7, [SHUF_MASK]
	; receive the initial 64B data, xor the initial crc value
	movdqu	xmm0, [arg3]
	movdqu	xmm1, [arg3+16]
	movdqu	xmm2, [arg3+32]
	movdqu	xmm3, [arg3+48]

	; copy initial data
	movdqu	[arg2], xmm0
	movdqu	[arg2+16], xmm1
	movdqu	[arg2+32], xmm2
	movdqu	[arg2+48], xmm3

	pshufb	xmm0, xmm7
	; XOR the initial_crc value
	pxor	xmm0, xmm6
	pshufb	xmm1, xmm7
	pshufb	xmm2, xmm7
	pshufb	xmm3, xmm7

	movdqa	xmm6, [rk3]	;xmm6 has rk3 and rk4
					;imm value of pclmulqdq instruction
					;will determine which constant to use
	;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
	; we subtract 128 instead of 64 to save one instruction from the loop
	sub	arg4, 128

	; at this section of the code, there is 64*x+y (0<=y<64) bytes of
	; buffer. The _fold_64_B_loop
	; loop will fold 64B at a time until we have 64+y Bytes of buffer


	; fold 64B at a time. This section of the code folds 4 xmm
	; registers in parallel
_fold_64_B_loop:

	; update the buffer pointer
	add	arg3, 64		;    buf += 64;
	add	arg2, 64

	prefetchnta [arg3+fetch_dist+0]
	movdqu	xmm4, xmm0
	movdqu	xmm5, xmm1

	pclmulqdq	xmm0, xmm6 , 0x11
	pclmulqdq	xmm1, xmm6 , 0x11

	pclmulqdq	xmm4, xmm6, 0x0
	pclmulqdq	xmm5, xmm6, 0x0

	pxor	xmm0, xmm4
	pxor	xmm1, xmm5

	prefetchnta [arg3+fetch_dist+32]
	movdqu	xmm4, xmm2
	movdqu	xmm5, xmm3

	pclmulqdq	xmm2, xmm6, 0x11
	pclmulqdq	xmm3, xmm6, 0x11

	pclmulqdq	xmm4, xmm6, 0x0
	pclmulqdq	xmm5, xmm6, 0x0

	pxor	xmm2, xmm4
	pxor	xmm3, xmm5

	movdqu	xmm4, [arg3]
	movdqu	xmm5, [arg3+16]
	movdqu	[arg2], xmm4
	movdqu	[arg2+16], xmm5
	pshufb	xmm4, xmm7
	pshufb	xmm5, xmm7
	pxor	xmm0, xmm4
	pxor	xmm1, xmm5

	movdqu	xmm4, [arg3+32]
	movdqu	xmm5, [arg3+48]
	movdqu	[arg2+32], xmm4
	movdqu	[arg2+48], xmm5
	pshufb	xmm4, xmm7
	pshufb	xmm5, xmm7

	pxor	xmm2, xmm4
	pxor	xmm3, xmm5

	sub	arg4, 64

	; check if there is another 64B in the buffer to be able to fold
	jge	_fold_64_B_loop
	;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;


	add	arg3, 64
	add	arg2, 64
	; at this point, the buffer pointer is pointing at the last y Bytes of the buffer
	; the 64B of folded data is in 4 of the xmm registers: xmm0, xmm1, xmm2, xmm3


	; fold the 4 xmm registers to 1 xmm register with different constants

	movdqa	xmm6, [rk1]	;xmm6 has rk1 and rk2
					;imm value of pclmulqdq instruction will
					;determine which constant to use

	movdqa	xmm4, xmm0
	pclmulqdq	xmm0, xmm6, 0x11
	pclmulqdq	xmm4, xmm6, 0x0
	pxor	xmm1, xmm4
	pxor	xmm1, xmm0

	movdqa	xmm4, xmm1
	pclmulqdq	xmm1, xmm6, 0x11
	pclmulqdq	xmm4, xmm6, 0x0
	pxor	xmm2, xmm4
	pxor	xmm2, xmm1

	movdqa	xmm4, xmm2
	pclmulqdq	xmm2, xmm6, 0x11
	pclmulqdq	xmm4, xmm6, 0x0
	pxor	xmm3, xmm4
	pxor	xmm3, xmm2


	; instead of 64, we add 48 to the loop counter to save 1 instruction from the loop
	; instead of a cmp instruction, we use the negative flag with the jl instruction
	add	arg4, 64-16
	jl	_final_reduction_for_128

	; now we have 16+y bytes left to reduce. 16 Bytes
	; is in register xmm3 and the rest is in memory
	; we can fold 16 bytes at a time if y>=16
	; continue folding 16B at a time

_16B_reduction_loop:
	movdqa	xmm4, xmm3
	pclmulqdq	xmm3, xmm6, 0x11
	pclmulqdq	xmm4, xmm6, 0x0
	pxor	xmm3, xmm4
	movdqu	xmm0, [arg3]
	movdqu	[arg2], xmm0
	pshufb	xmm0, xmm7
	pxor	xmm3, xmm0
	add	arg3, 16
	add	arg2, 16
	sub	arg4, 16
	; instead of a cmp instruction, we utilize the flags with the jge instruction
	; equivalent of: cmp arg4, 16-16
	; check if there is any more 16B in the buffer to be able to fold
	jge	_16B_reduction_loop

	;now we have 16+z bytes left to reduce, where 0<= z < 16.
	;first, we reduce the data in the xmm3 register


_final_reduction_for_128:
	; check if any more data to fold. If not, compute the CRC of the final 128 bits
	add	arg4, 16
	je	_128_done

	; here we are getting data that is less than 16 bytes.
	; since we know that there was data before the pointer,
	; we can offset the input pointer before the actual point,
	; to receive exactly 16 bytes.
	; after that the registers need to be adjusted.
_get_last_two_xmms:
	movdqa	xmm2, xmm3

	movdqu	xmm1, [arg3 - 16 + arg4]
	movdqu	[arg2 - 16 + arg4], xmm1
	pshufb	xmm1, xmm7

	; get rid of the extra data that was loaded before
	; load the shift constant
	lea	rax, [pshufb_shf_table + 16]
	sub	rax, arg4
	movdqu	xmm0, [rax]

	; shift xmm2 to the left by arg4 bytes
	pshufb	xmm2, xmm0

	; shift xmm3 to the right by 16-arg4 bytes
	pxor	xmm0, [mask1]
	pshufb	xmm3, xmm0
	pblendvb	xmm1, xmm2	;xmm0 is implicit

	; fold 16 Bytes
	movdqa	xmm2, xmm1
	movdqa	xmm4, xmm3
	pclmulqdq	xmm3, xmm6, 0x11
	pclmulqdq	xmm4, xmm6, 0x0
	pxor	xmm3, xmm4
	pxor	xmm3, xmm2

_128_done:
	; compute crc of a 128-bit value
	movdqa	xmm6, [rk5]	; rk5 and rk6 in xmm6
	movdqa	xmm0, xmm3

	;64b fold
	pclmulqdq	xmm3, xmm6, 0x1
	pslldq	xmm0, 8
	pxor	xmm3, xmm0

	;32b fold
	movdqa	xmm0, xmm3

	pand	xmm0, [mask2]

	psrldq	xmm3, 12
	pclmulqdq	xmm3, xmm6, 0x10
	pxor	xmm3, xmm0

	;barrett reduction
_barrett:
	movdqa	xmm6, [rk7]	; rk7 and rk8 in xmm6
	movdqa	xmm0, xmm3
	pclmulqdq	xmm3, xmm6, 0x01
	pslldq	xmm3, 4
	pclmulqdq	xmm3, xmm6, 0x11

	pslldq	xmm3, 4
	pxor	xmm3, xmm0
	pextrd	eax, xmm3,1

_cleanup:
	; scale the result back to 16 bits
	shr	eax, 16
	movdqa	xmm6, [rsp+16*2]
	movdqa	xmm7, [rsp+16*3]
	add	rsp,16*4+8
	ret


;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

align 16
_less_than_128:

	; check if there is enough buffer to be able to fold 16B at a time
	cmp	arg4, 32
	jl	_less_than_32
	movdqa xmm7, [SHUF_MASK]

	; if there is, load the constants
	movdqa	xmm6, [rk1]	; rk1 and rk2 in xmm6

	movd	xmm0, arg1_low32	; get the initial crc value
	pslldq	xmm0, 12	; align it to its correct place
	movdqu	xmm3, [arg3]	; load the plaintext
	movdqu	[arg2], xmm3	; store copy
	pshufb	xmm3, xmm7	; byte-reflect the plaintext
	pxor	xmm3, xmm0


	; update the buffer pointer
	add	arg3, 16
	add	arg2, 16

	; update the counter. subtract 32 instead of 16 to save one instruction from the loop
	sub	arg4, 32

	jmp	_16B_reduction_loop


align 16
_less_than_32:
	; mov initial crc to the return value. this is necessary for zero-length buffers.
	mov	eax, arg1_low32
	test	arg4, arg4
	je	_cleanup

	movdqa xmm7, [SHUF_MASK]

	movd	xmm0, arg1_low32	; get the initial crc value
	pslldq	xmm0, 12		; align it to its correct place

	cmp	arg4, 16
	je	_exact_16_left
	jl	_less_than_16_left

	movdqu	xmm3, [arg3]	; load the plaintext
	movdqu	[arg2], xmm3	; store the copy
	pshufb	xmm3, xmm7	; byte-reflect the plaintext
	pxor	xmm3, xmm0	; xor the initial crc value
	add	arg3, 16
	add	arg2, 16
	sub	arg4, 16
	movdqa	xmm6, [rk1]	; rk1 and rk2 in xmm6
	jmp	_get_last_two_xmms


align 16
_less_than_16_left:
	; use stack space to load data less than 16 bytes, zero-out the 16B in memory first.

	pxor	xmm1, xmm1
	mov	r11, rsp
	movdqa	[r11], xmm1

	cmp	arg4, 4
	jl	_only_less_than_4

	;	backup the counter value
	mov	tmp1, arg4
	cmp	arg4, 8
	jl	_less_than_8_left

	; load 8 Bytes
	mov	rax, [arg3]
	mov	[arg2], rax
	mov	[r11], rax
	add	r11, 8
	sub	arg4, 8
	add	arg3, 8
	add	arg2, 8
_less_than_8_left:

	cmp	arg4, 4
	jl	_less_than_4_left

	; load 4 Bytes
	mov	eax, [arg3]
	mov	[arg2], eax
	mov	[r11], eax
	add	r11, 4
	sub	arg4, 4
	add	arg3, 4
	add	arg2, 4
_less_than_4_left:

	cmp	arg4, 2
	jl	_less_than_2_left

	; load 2 Bytes
	mov	ax, [arg3]
	mov	[arg2], ax
	mov	[r11], ax
	add	r11, 2
	sub	arg4, 2
	add	arg3, 2
	add	arg2, 2
_less_than_2_left:
	cmp	arg4, 1
	jl	_zero_left

	; load 1 Byte
	mov	al, [arg3]
	mov	[arg2], al
	mov	[r11], al
_zero_left:
	movdqa	xmm3, [rsp]
	pshufb	xmm3, xmm7
	pxor	xmm3, xmm0	; xor the initial crc value

	; shl tmp1, 4
	lea	rax, [pshufb_shf_table + 16]
	sub	rax, tmp1
	movdqu	xmm0, [rax]
	pxor	xmm0, [mask1]

	pshufb	xmm3, xmm0
	jmp	_128_done

align 16
_exact_16_left:
	movdqu	xmm3, [arg3]
	movdqu	[arg2], xmm3
	pshufb	xmm3, xmm7
	pxor	xmm3, xmm0	; xor the initial crc value

	jmp	_128_done

_only_less_than_4:
	cmp	arg4, 3
	jl	_only_less_than_3

	; load 3 Bytes
	mov	al, [arg3]
	mov	[arg2], al
	mov	[r11], al

	mov	al, [arg3+1]
	mov	[arg2+1], al
	mov	[r11+1], al

	mov	al, [arg3+2]
	mov	[arg2+2], al
	mov	[r11+2], al

	movdqa	xmm3, [rsp]
	pshufb	xmm3, xmm7
	pxor	xmm3, xmm0	; xor the initial crc value

	psrldq	xmm3, 5

	jmp	_barrett
_only_less_than_3:
	cmp	arg4, 2
	jl	_only_less_than_2

	; load 2 Bytes
	mov	al, [arg3]
	mov	[arg2], al
	mov	[r11], al

	mov	al, [arg3+1]
	mov	[arg2+1], al
	mov	[r11+1], al

	movdqa	xmm3, [rsp]
	pshufb	xmm3, xmm7
	pxor	xmm3, xmm0	; xor the initial crc value

	psrldq	xmm3, 6

	jmp	_barrett
_only_less_than_2:

	; load 1 Byte
	mov	al, [arg3]
	mov	[arg2],al
	mov	[r11], al

	movdqa	xmm3, [rsp]
	pshufb	xmm3, xmm7
	pxor	xmm3, xmm0	; xor the initial crc value

	psrldq	xmm3, 7

	jmp	_barrett

section .data

; precomputed constants
; these constants are precomputed from the poly: 0x8bb70000 (0x8bb7 scaled to 32 bits)
align 16
; Q = 0x18BB70000
; rk1 = 2^(32*3) mod Q << 32
; rk2 = 2^(32*5) mod Q << 32
; rk3 = 2^(32*15) mod Q << 32
; rk4 = 2^(32*17) mod Q << 32
; rk5 = 2^(32*3) mod Q << 32
; rk6 = 2^(32*2) mod Q << 32
; rk7 = floor(2^64/Q)
; rk8 = Q
rk1:
DQ 0x2d56000000000000
rk2:
DQ 0x06df000000000000
rk3:
DQ 0x044c000000000000
rk4:
DQ 0xe658000000000000
rk5:
DQ 0x2d56000000000000
rk6:
DQ 0x1368000000000000
rk7:
DQ 0x00000001f65a57f8
rk8:
DQ 0x000000018bb70000
mask1:
dq 0x8080808080808080, 0x8080808080808080
mask2:
dq 0xFFFFFFFFFFFFFFFF, 0x00000000FFFFFFFF

SHUF_MASK:
dq 0x08090A0B0C0D0E0F, 0x0001020304050607

pshufb_shf_table:
; use these values for shift constants for the pshufb instruction
; different alignments result in values as shown:
;	dq 0x8887868584838281, 0x008f8e8d8c8b8a89 ; shl 15 (16-1) / shr1
;	dq 0x8988878685848382, 0x01008f8e8d8c8b8a ; shl 14 (16-3) / shr2
;	dq 0x8a89888786858483, 0x0201008f8e8d8c8b ; shl 13 (16-4) / shr3
;	dq 0x8b8a898887868584, 0x030201008f8e8d8c ; shl 12 (16-4) / shr4
;	dq 0x8c8b8a8988878685, 0x04030201008f8e8d ; shl 11 (16-5) / shr5
;	dq 0x8d8c8b8a89888786, 0x0504030201008f8e ; shl 10 (16-6) / shr6
;	dq 0x8e8d8c8b8a898887, 0x060504030201008f ; shl 9  (16-7) / shr7
;	dq 0x8f8e8d8c8b8a8988, 0x0706050403020100 ; shl 8  (16-8) / shr8
;	dq 0x008f8e8d8c8b8a89, 0x0807060504030201 ; shl 7  (16-9) / shr9
;	dq 0x01008f8e8d8c8b8a, 0x0908070605040302 ; shl 6  (16-10) / shr10
;	dq 0x0201008f8e8d8c8b, 0x0a09080706050403 ; shl 5  (16-11) / shr11
;	dq 0x030201008f8e8d8c, 0x0b0a090807060504 ; shl 4  (16-12) / shr12
;	dq 0x04030201008f8e8d, 0x0c0b0a0908070605 ; shl 3  (16-13) / shr13
;	dq 0x0504030201008f8e, 0x0d0c0b0a09080706 ; shl 2  (16-14) / shr14
;	dq 0x060504030201008f, 0x0e0d0c0b0a090807 ; shl 1  (16-15) / shr15
dq 0x8786858483828100, 0x8f8e8d8c8b8a8988
dq 0x0706050403020100, 0x000e0d0c0b0a0908

;;;       func                   core, ver, snum
slversion crc16_t10dif_copy_by4, 05,   02,  0000
