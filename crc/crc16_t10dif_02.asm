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

;       Function API:
;       UINT16 crc16_t10dif_02(
;               UINT16 init_crc, //initial CRC value, 16 bits
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

%include "reg_sizes.asm"

%define	fetch_dist	1024

[bits 64]
default rel

section .text

%ifidn __OUTPUT_FORMAT__, win64
        %xdefine        arg1 rcx
        %xdefine        arg2 rdx
        %xdefine        arg3 r8

        %xdefine        arg1_low32 ecx
%else
        %xdefine        arg1 rdi
        %xdefine        arg2 rsi
        %xdefine        arg3 rdx

        %xdefine        arg1_low32 edi
%endif

%ifidn __OUTPUT_FORMAT__, win64
        %define XMM_SAVE 16*2
        %define VARIABLE_OFFSET 16*10+8
%else
        %define VARIABLE_OFFSET 16*2+8
%endif

align 16
global	crc16_t10dif_02:ISAL_SYM_TYPE_FUNCTION
crc16_t10dif_02:

	; adjust the 16-bit initial_crc value, scale it to 32 bits
	shl	arg1_low32, 16

	; After this point, code flow is exactly same as a 32-bit CRC.
	; The only difference is before returning eax, we will shift it right 16 bits, to scale back to 16 bits.

	sub	rsp, VARIABLE_OFFSET
%ifidn __OUTPUT_FORMAT__, win64
	; push the xmm registers into the stack to maintain
	vmovdqa [rsp+16*2],xmm6
	vmovdqa [rsp+16*3],xmm7
	vmovdqa [rsp+16*4],xmm8
	vmovdqa [rsp+16*5],xmm9
	vmovdqa [rsp+16*6],xmm10
	vmovdqa [rsp+16*7],xmm11
	vmovdqa [rsp+16*8],xmm12
	vmovdqa [rsp+16*9],xmm13
%endif

	; check if smaller than 256
	cmp	arg3, 256

	; for sizes less than 256, we can't fold 128B at a time...
	jl	_less_than_256


	; load the initial crc value
	vmovd	xmm10, arg1_low32	; initial crc

	; crc value does not need to be byte-reflected, but it needs to be moved to the high part of the register.
	; because data will be byte-reflected and will align with initial crc at correct place.
	vpslldq	xmm10, 12

	vmovdqa xmm11, [SHUF_MASK]
	; receive the initial 128B data, xor the initial crc value
	vmovdqu	xmm0, [arg2+16*0]
	vmovdqu	xmm1, [arg2+16*1]
	vmovdqu	xmm2, [arg2+16*2]
	vmovdqu	xmm3, [arg2+16*3]
	vmovdqu	xmm4, [arg2+16*4]
	vmovdqu	xmm5, [arg2+16*5]
	vmovdqu	xmm6, [arg2+16*6]
	vmovdqu	xmm7, [arg2+16*7]

	vpshufb	xmm0, xmm11
	; XOR the initial_crc value
	vpxor	xmm0, xmm10
	vpshufb	xmm1, xmm11
	vpshufb	xmm2, xmm11
	vpshufb	xmm3, xmm11
	vpshufb	xmm4, xmm11
	vpshufb	xmm5, xmm11
	vpshufb	xmm6, xmm11
	vpshufb	xmm7, xmm11

	vmovdqa	xmm10, [rk3]	;xmm10 has rk3 and rk4
					;imm value of pclmulqdq instruction will determine which constant to use
	;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
	; we subtract 256 instead of 128 to save one instruction from the loop
	sub	arg3, 256

	; at this section of the code, there is 128*x+y (0<=y<128) bytes of buffer. The _fold_128_B_loop
	; loop will fold 128B at a time until we have 128+y Bytes of buffer


	; fold 128B at a time. This section of the code folds 8 xmm registers in parallel
_fold_128_B_loop:

	; update the buffer pointer
	add	arg2, 128		;    buf += 128;

	prefetchnta [arg2+fetch_dist+0]
	vmovdqu	xmm9, [arg2+16*0]
	vmovdqu	xmm12, [arg2+16*1]
	vpshufb	xmm9, xmm11
	vpshufb	xmm12, xmm11
	vmovdqa	xmm8, xmm0
	vmovdqa	xmm13, xmm1
	vpclmulqdq	xmm0, xmm10, 0x0
	vpclmulqdq	xmm8, xmm10 , 0x11
	vpclmulqdq	xmm1, xmm10, 0x0
	vpclmulqdq	xmm13, xmm10 , 0x11
	vpxor	xmm0, xmm9
	vxorps	xmm0, xmm8
	vpxor	xmm1, xmm12
	vxorps	xmm1, xmm13

	prefetchnta [arg2+fetch_dist+32]
	vmovdqu	xmm9, [arg2+16*2]
	vmovdqu	xmm12, [arg2+16*3]
	vpshufb	xmm9, xmm11
	vpshufb	xmm12, xmm11
	vmovdqa	xmm8, xmm2
	vmovdqa	xmm13, xmm3
	vpclmulqdq	xmm2, xmm10, 0x0
	vpclmulqdq	xmm8, xmm10 , 0x11
	vpclmulqdq	xmm3, xmm10, 0x0
	vpclmulqdq	xmm13, xmm10 , 0x11
	vpxor	xmm2, xmm9
	vxorps	xmm2, xmm8
	vpxor	xmm3, xmm12
	vxorps	xmm3, xmm13

	prefetchnta [arg2+fetch_dist+64]
	vmovdqu	xmm9, [arg2+16*4]
	vmovdqu	xmm12, [arg2+16*5]
	vpshufb	xmm9, xmm11
	vpshufb	xmm12, xmm11
	vmovdqa	xmm8, xmm4
	vmovdqa	xmm13, xmm5
	vpclmulqdq	xmm4, xmm10, 0x0
	vpclmulqdq	xmm8, xmm10 , 0x11
	vpclmulqdq	xmm5, xmm10, 0x0
	vpclmulqdq	xmm13, xmm10 , 0x11
	vpxor	xmm4, xmm9
	vxorps	xmm4, xmm8
	vpxor	xmm5, xmm12
	vxorps	xmm5, xmm13

	prefetchnta [arg2+fetch_dist+96]
	vmovdqu	xmm9, [arg2+16*6]
	vmovdqu	xmm12, [arg2+16*7]
	vpshufb	xmm9, xmm11
	vpshufb	xmm12, xmm11
	vmovdqa	xmm8, xmm6
	vmovdqa	xmm13, xmm7
	vpclmulqdq	xmm6, xmm10, 0x0
	vpclmulqdq	xmm8, xmm10 , 0x11
	vpclmulqdq	xmm7, xmm10, 0x0
	vpclmulqdq	xmm13, xmm10 , 0x11
	vpxor	xmm6, xmm9
	vxorps	xmm6, xmm8
	vpxor	xmm7, xmm12
	vxorps	xmm7, xmm13

	sub	arg3, 128

	; check if there is another 128B in the buffer to be able to fold
	jge	_fold_128_B_loop
	;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;


	add	arg2, 128
	; at this point, the buffer pointer is pointing at the last y Bytes of the buffer
	; fold the 8 xmm registers to 1 xmm register with different constants

	vmovdqa	xmm10, [rk9]
	vmovdqa	xmm8, xmm0
	vpclmulqdq	xmm0, xmm10, 0x11
	vpclmulqdq	xmm8, xmm10, 0x0
	vpxor	xmm7, xmm8
	vxorps	xmm7, xmm0

	vmovdqa	xmm10, [rk11]
	vmovdqa	xmm8, xmm1
	vpclmulqdq	xmm1, xmm10, 0x11
	vpclmulqdq	xmm8, xmm10, 0x0
	vpxor	xmm7, xmm8
	vxorps	xmm7, xmm1

	vmovdqa	xmm10, [rk13]
	vmovdqa	xmm8, xmm2
	vpclmulqdq	xmm2, xmm10, 0x11
	vpclmulqdq	xmm8, xmm10, 0x0
	vpxor	xmm7, xmm8
	vpxor	xmm7, xmm2

	vmovdqa	xmm10, [rk15]
	vmovdqa	xmm8, xmm3
	vpclmulqdq	xmm3, xmm10, 0x11
	vpclmulqdq	xmm8, xmm10, 0x0
	vpxor	xmm7, xmm8
	vxorps	xmm7, xmm3

	vmovdqa	xmm10, [rk17]
	vmovdqa	xmm8, xmm4
	vpclmulqdq	xmm4, xmm10, 0x11
	vpclmulqdq	xmm8, xmm10, 0x0
	vpxor	xmm7, xmm8
	vpxor	xmm7, xmm4

	vmovdqa	xmm10, [rk19]
	vmovdqa	xmm8, xmm5
	vpclmulqdq	xmm5, xmm10, 0x11
	vpclmulqdq	xmm8, xmm10, 0x0
	vpxor	xmm7, xmm8
	vxorps	xmm7, xmm5

	vmovdqa	xmm10, [rk1]	;xmm10 has rk1 and rk2
				;imm value of pclmulqdq instruction will determine which constant to use
	vmovdqa	xmm8, xmm6
	vpclmulqdq	xmm6, xmm10, 0x11
	vpclmulqdq	xmm8, xmm10, 0x0
	vpxor	xmm7, xmm8
	vpxor	xmm7, xmm6


	; instead of 128, we add 112 to the loop counter to save 1 instruction from the loop
	; instead of a cmp instruction, we use the negative flag with the jl instruction
	add	arg3, 128-16
	jl	_final_reduction_for_128

	; now we have 16+y bytes left to reduce. 16 Bytes is in register xmm7 and the rest is in memory
	; we can fold 16 bytes at a time if y>=16
	; continue folding 16B at a time

_16B_reduction_loop:
	vmovdqa	xmm8, xmm7
	vpclmulqdq	xmm7, xmm10, 0x11
	vpclmulqdq	xmm8, xmm10, 0x0
	vpxor	xmm7, xmm8
	vmovdqu	xmm0, [arg2]
	vpshufb	xmm0, xmm11
	vpxor	xmm7, xmm0
	add	arg2, 16
	sub	arg3, 16
	; instead of a cmp instruction, we utilize the flags with the jge instruction
	; equivalent of: cmp arg3, 16-16
	; check if there is any more 16B in the buffer to be able to fold
	jge	_16B_reduction_loop

	;now we have 16+z bytes left to reduce, where 0<= z < 16.
	;first, we reduce the data in the xmm7 register


_final_reduction_for_128:
	; check if any more data to fold. If not, compute the CRC of the final 128 bits
	add	arg3, 16
	je	_128_done

	; here we are getting data that is less than 16 bytes.
	; since we know that there was data before the pointer, we can offset the input pointer before the actual point, to receive exactly 16 bytes.
	; after that the registers need to be adjusted.
_get_last_two_xmms:
	vmovdqa	xmm2, xmm7

	vmovdqu	xmm1, [arg2 - 16 + arg3]
	vpshufb	xmm1, xmm11

	; get rid of the extra data that was loaded before
	; load the shift constant
	lea	rax, [pshufb_shf_table + 16]
	sub	rax, arg3
	vmovdqu	xmm0, [rax]

	; shift xmm2 to the left by arg3 bytes
	vpshufb	xmm2, xmm0

	; shift xmm7 to the right by 16-arg3 bytes
	vpxor	xmm0, [mask1]
	vpshufb	xmm7, xmm0
	vpblendvb	xmm1, xmm1, xmm2, xmm0

	; fold 16 Bytes
	vmovdqa	xmm2, xmm1
	vmovdqa	xmm8, xmm7
	vpclmulqdq	xmm7, xmm10, 0x11
	vpclmulqdq	xmm8, xmm10, 0x0
	vpxor	xmm7, xmm8
	vpxor	xmm7, xmm2

_128_done:
	; compute crc of a 128-bit value
	vmovdqa	xmm10, [rk5]	; rk5 and rk6 in xmm10
	vmovdqa	xmm0, xmm7

	;64b fold
	vpclmulqdq	xmm7, xmm10, 0x1
	vpslldq	xmm0, 8
	vpxor	xmm7, xmm0

	;32b fold
	vmovdqa	xmm0, xmm7

	vpand	xmm0, [mask2]

	vpsrldq	xmm7, 12
	vpclmulqdq	xmm7, xmm10, 0x10
	vpxor	xmm7, xmm0

	;barrett reduction
_barrett:
	vmovdqa	xmm10, [rk7]	; rk7 and rk8 in xmm10
	vmovdqa	xmm0, xmm7
	vpclmulqdq	xmm7, xmm10, 0x01
	vpslldq	xmm7, 4
	vpclmulqdq	xmm7, xmm10, 0x11

	vpslldq	xmm7, 4
	vpxor	xmm7, xmm0
	vpextrd	eax, xmm7,1

_cleanup:
	; scale the result back to 16 bits
	shr	eax, 16
%ifidn __OUTPUT_FORMAT__, win64
	vmovdqa	xmm6, [rsp+16*2]
	vmovdqa	xmm7, [rsp+16*3]
	vmovdqa	xmm8, [rsp+16*4]
	vmovdqa	xmm9, [rsp+16*5]
	vmovdqa	xmm10, [rsp+16*6]
	vmovdqa	xmm11, [rsp+16*7]
	vmovdqa	xmm12, [rsp+16*8]
	vmovdqa	xmm13, [rsp+16*9]
%endif
	add	rsp, VARIABLE_OFFSET
	ret


;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

align 16
_less_than_256:

	; check if there is enough buffer to be able to fold 16B at a time
	cmp	arg3, 32
	jl	_less_than_32
	vmovdqa xmm11, [SHUF_MASK]

	; if there is, load the constants
	vmovdqa	xmm10, [rk1]	; rk1 and rk2 in xmm10

	vmovd	xmm0, arg1_low32	; get the initial crc value
	vpslldq	xmm0, 12	; align it to its correct place
	vmovdqu	xmm7, [arg2]	; load the plaintext
	vpshufb	xmm7, xmm11	; byte-reflect the plaintext
	vpxor	xmm7, xmm0


	; update the buffer pointer
	add	arg2, 16

	; update the counter. subtract 32 instead of 16 to save one instruction from the loop
	sub	arg3, 32

	jmp	_16B_reduction_loop


align 16
_less_than_32:
	; mov initial crc to the return value. this is necessary for zero-length buffers.
	mov	eax, arg1_low32
	test	arg3, arg3
	je	_cleanup

	vmovdqa xmm11, [SHUF_MASK]

	vmovd	xmm0, arg1_low32	; get the initial crc value
	vpslldq	xmm0, 12	; align it to its correct place

	cmp	arg3, 16
	je	_exact_16_left
	jl	_less_than_16_left

	vmovdqu	xmm7, [arg2]	; load the plaintext
	vpshufb	xmm7, xmm11	; byte-reflect the plaintext
	vpxor	xmm7, xmm0	; xor the initial crc value
	add	arg2, 16
	sub	arg3, 16
	vmovdqa	xmm10, [rk1]	; rk1 and rk2 in xmm10
	jmp	_get_last_two_xmms


align 16
_less_than_16_left:
	; use stack space to load data less than 16 bytes, zero-out the 16B in memory first.

	vpxor	xmm1, xmm1
	mov	r11, rsp
	vmovdqa	[r11], xmm1

	cmp	arg3, 4
	jl	_only_less_than_4

	;	backup the counter value
	mov	r9, arg3
	cmp	arg3, 8
	jl	_less_than_8_left

	; load 8 Bytes
	mov	rax, [arg2]
	mov	[r11], rax
	add	r11, 8
	sub	arg3, 8
	add	arg2, 8
_less_than_8_left:

	cmp	arg3, 4
	jl	_less_than_4_left

	; load 4 Bytes
	mov	eax, [arg2]
	mov	[r11], eax
	add	r11, 4
	sub	arg3, 4
	add	arg2, 4
_less_than_4_left:

	cmp	arg3, 2
	jl	_less_than_2_left

	; load 2 Bytes
	mov	ax, [arg2]
	mov	[r11], ax
	add	r11, 2
	sub	arg3, 2
	add	arg2, 2
_less_than_2_left:
	cmp     arg3, 1
        jl      _zero_left

	; load 1 Byte
	mov	al, [arg2]
	mov	[r11], al
_zero_left:
	vmovdqa	xmm7, [rsp]
	vpshufb	xmm7, xmm11
	vpxor	xmm7, xmm0	; xor the initial crc value

	lea	rax, [pshufb_shf_table + 16]
	sub	rax, r9
	vmovdqu	xmm0, [rax]
	vpxor	xmm0, [mask1]

	vpshufb	xmm7, xmm0
	jmp	_128_done

align 16
_exact_16_left:
	vmovdqu	xmm7, [arg2]
	vpshufb	xmm7, xmm11
	vpxor	xmm7, xmm0	; xor the initial crc value

	jmp	_128_done

_only_less_than_4:
	cmp	arg3, 3
	jl	_only_less_than_3

	; load 3 Bytes
	mov	al, [arg2]
	mov	[r11], al

	mov	al, [arg2+1]
	mov	[r11+1], al

	mov	al, [arg2+2]
	mov	[r11+2], al

	vmovdqa	xmm7, [rsp]
	vpshufb	xmm7, xmm11
	vpxor	xmm7, xmm0	; xor the initial crc value

	vpsrldq	xmm7, 5

	jmp	_barrett
_only_less_than_3:
	cmp	arg3, 2
	jl	_only_less_than_2

	; load 2 Bytes
	mov	al, [arg2]
	mov	[r11], al

	mov	al, [arg2+1]
	mov	[r11+1], al

	vmovdqa	xmm7, [rsp]
	vpshufb	xmm7, xmm11
	vpxor	xmm7, xmm0	; xor the initial crc value

	vpsrldq	xmm7, 6

	jmp	_barrett
_only_less_than_2:

	; load 1 Byte
	mov	al, [arg2]
	mov	[r11], al

	vmovdqa	xmm7, [rsp]
	vpshufb	xmm7, xmm11
	vpxor	xmm7, xmm0	; xor the initial crc value

	vpsrldq	xmm7, 7

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
DQ 0x9d9d000000000000
rk4:
DQ 0x7cf5000000000000
rk5:
DQ 0x2d56000000000000
rk6:
DQ 0x1368000000000000
rk7:
DQ 0x00000001f65a57f8
rk8:
DQ 0x000000018bb70000

rk9:
DQ 0xceae000000000000
rk10:
DQ 0xbfd6000000000000
rk11:
DQ 0x1e16000000000000
rk12:
DQ 0x713c000000000000
rk13:
DQ 0xf7f9000000000000
rk14:
DQ 0x80a6000000000000
rk15:
DQ 0x044c000000000000
rk16:
DQ 0xe658000000000000
rk17:
DQ 0xad18000000000000
rk18:
DQ 0xa497000000000000
rk19:
DQ 0x6ee3000000000000
rk20:
DQ 0xe7b5000000000000

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
