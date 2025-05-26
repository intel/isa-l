;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;  Copyright(c) 2011-2025 Intel Corporation All rights reserved.
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
;       UINT32 crc32_iscsi_by8_02(
;               const unsigned char *buf, //buffer pointer to calculate CRC on
;               UINT64 len, //buffer length in bytes (64-bit data)
;               UINT32 init_crc //initial CRC value, 32 bits
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
;
;       CRC-32 checksum is described in RFC 1952
;       Implementing RFC 1952 CRC:
;       http://www.ietf.org/rfc/rfc1952.txt

%include "reg_sizes.asm"

%ifndef fetch_dist
%define	fetch_dist	1024
%endif

%ifndef PREFETCH
%define PREFETCH        prefetcht0
%endif

[bits 64]
default rel

section .text

%ifidn __OUTPUT_FORMAT__, win64
        %xdefine        arg1 rcx
        %xdefine        arg2 rdx
        %xdefine        arg3 r8

        %xdefine        arg3_low32 r8d
%else
        %xdefine        arg1 rdi
        %xdefine        arg2 rsi
        %xdefine        arg3 rdx

        %xdefine        arg3_low32 edx
%endif

%define in_buf  arg1
%define buf_len arg2
%define init_crc arg3_low32

%ifidn __OUTPUT_FORMAT__, win64
        %define XMM_OFFSET 16*2
        %define VARIABLE_OFFSET 16*10+8
%else
        %define VARIABLE_OFFSET 16*2+8
%endif

align 16
mk_global  crc32_iscsi_by8_02, function
crc32_iscsi_by8_02:
	endbranch

	sub	rsp,VARIABLE_OFFSET

%ifidn __OUTPUT_FORMAT__, win64
        ; push the xmm registers into the stack to maintain
        vmovdqa  [rsp + XMM_OFFSET + 16*0], xmm6
        vmovdqa  [rsp + XMM_OFFSET + 16*1], xmm7
        vmovdqa  [rsp + XMM_OFFSET + 16*2], xmm8
        vmovdqa  [rsp + XMM_OFFSET + 16*3], xmm9
        vmovdqa  [rsp + XMM_OFFSET + 16*4], xmm10
        vmovdqa  [rsp + XMM_OFFSET + 16*5], xmm11
        vmovdqa  [rsp + XMM_OFFSET + 16*6], xmm12
        vmovdqa  [rsp + XMM_OFFSET + 16*7], xmm13
%endif

	; check if smaller than 256
	cmp	buf_len, 256

	; for sizes less than 256, we can't fold 128B at a time...
	jl	_less_than_256


	; load the initial crc value
	vmovd	xmm10, init_crc	; initial crc

	; receive the initial 128B data, xor the initial crc value
	vmovdqu	xmm0, [in_buf+16*0]
	vmovdqu	xmm1, [in_buf+16*1]
	vmovdqu	xmm2, [in_buf+16*2]
	vmovdqu	xmm3, [in_buf+16*3]
	vmovdqu	xmm4, [in_buf+16*4]
	vmovdqu	xmm5, [in_buf+16*5]
	vmovdqu	xmm6, [in_buf+16*6]
	vmovdqu	xmm7, [in_buf+16*7]

	; XOR the initial_crc value
	vpxor	xmm0, xmm10

	vmovdqa	xmm10, [rk3]	;xmm10 has rk3 and rk4
					;imm value of pclmulqdq instruction will determine which constant to use
	;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
	; we subtract 256 instead of 128 to save one instruction from the loop
	sub	buf_len, 256

	; at this section of the code, there is 128*x+y (0<=y<128) bytes of buffer. The _fold_128_B_loop
	; loop will fold 128B at a time until we have 128+y Bytes of buffer


	; fold 128B at a time. This section of the code folds 8 xmm registers in parallel
_fold_128_B_loop:

	; update the buffer pointer
	add	in_buf, 128		;    buf += 128;

	PREFETCH [in_buf+fetch_dist+0]
	vmovdqu	xmm9, [in_buf+16*0]
	vmovdqu	xmm12, [in_buf+16*1]
	vpclmulqdq	xmm8, xmm0, xmm10, 0x10
	vpclmulqdq	xmm0, xmm0, xmm10, 0x1
	vpclmulqdq	xmm13, xmm1, xmm10, 0x10
	vpclmulqdq	xmm1, xmm1, xmm10, 0x1
	vpxor	xmm0, xmm9
	vpxor 	xmm0, xmm8
	vpxor	xmm1, xmm12
	vpxor 	xmm1, xmm13

	PREFETCH [in_buf+fetch_dist+32]
	vmovdqu	xmm9, [in_buf+16*2]
	vmovdqu	xmm12, [in_buf+16*3]
	vpclmulqdq	xmm8, xmm2, xmm10, 0x10
	vpclmulqdq	xmm2, xmm2, xmm10, 0x1
	vpclmulqdq	xmm13, xmm3, xmm10, 0x10
	vpclmulqdq	xmm3, xmm3, xmm10, 0x1
	vpxor	xmm2, xmm9
	vpxor 	xmm2, xmm8
	vpxor	xmm3, xmm12
	vpxor 	xmm3, xmm13

	PREFETCH [in_buf+fetch_dist+64]
	vmovdqu	xmm9, [in_buf+16*4]
	vmovdqu	xmm12, [in_buf+16*5]
	vpclmulqdq	xmm8, xmm4, xmm10, 0x10
	vpclmulqdq	xmm4, xmm4, xmm10, 0x1
	vpclmulqdq	xmm13, xmm5, xmm10, 0x10
	vpclmulqdq	xmm5, xmm5, xmm10, 0x1
	vpxor	xmm4, xmm9
	vpxor 	xmm4, xmm8
	vpxor	xmm5, xmm12
	vpxor 	xmm5, xmm13

	PREFETCH [in_buf+fetch_dist+96]
	vmovdqu	xmm9, [in_buf+16*6]
	vmovdqu	xmm12, [in_buf+16*7]
	vpclmulqdq	xmm8, xmm6, xmm10, 0x10
	vpclmulqdq	xmm6, xmm6, xmm10, 0x1
	vpclmulqdq	xmm13, xmm7, xmm10, 0x10
	vpclmulqdq	xmm7, xmm7, xmm10, 0x1
	vpxor	xmm6, xmm9
	vpxor 	xmm6, xmm8
	vpxor	xmm7, xmm12
	vpxor 	xmm7, xmm13

	sub	buf_len, 128

	; check if there is another 128B in the buffer to be able to fold
	jge	_fold_128_B_loop
	;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;


	add	in_buf, 128
	; at this point, the buffer pointer is pointing at the last y Bytes of the buffer
	; the 128 of folded data is in 4 of the xmm registers: xmm0, xmm1, xmm2, xmm3


	; fold the 8 xmm registers to 1 xmm register with different constants

	vmovdqa	xmm10, [rk9]
	vpclmulqdq	xmm8, xmm0, xmm10, 0x1
	vpclmulqdq	xmm0, xmm0, xmm10, 0x10
	vpxor	xmm7, xmm8
	vpxor 	xmm7, xmm0

	vmovdqa	xmm10, [rk11]
	vpclmulqdq	xmm8, xmm1, xmm10, 0x1
	vpclmulqdq	xmm1, xmm1, xmm10, 0x10
	vpxor	xmm7, xmm8
	vpxor 	xmm7, xmm1

	vmovdqa	xmm10, [rk13]
	vpclmulqdq	xmm8, xmm2, xmm10, 0x1
	vpclmulqdq	xmm2, xmm2, xmm10, 0x10
	vpxor	xmm7, xmm8
	vpxor	xmm7, xmm2

	vmovdqa	xmm10, [rk15]
	vpclmulqdq	xmm8, xmm3, xmm10, 0x1
	vpclmulqdq	xmm3, xmm3, xmm10, 0x10
	vpxor	xmm7, xmm8
	vpxor 	xmm7, xmm3

	vmovdqa	xmm10, [rk17]
	vpclmulqdq	xmm8, xmm4, xmm10, 0x1
	vpclmulqdq	xmm4, xmm4, xmm10, 0x10
	vpxor	xmm7, xmm8
	vpxor	xmm7, xmm4

	vmovdqa	xmm10, [rk19]
	vpclmulqdq	xmm8, xmm5, xmm10, 0x1
	vpclmulqdq	xmm5, xmm5, xmm10, 0x10
	vpxor	xmm7, xmm8
	vpxor 	xmm7, xmm5

	vmovdqa	xmm10, [rk1]	;xmm10 has rk1 and rk2
									;imm value of pclmulqdq instruction will determine which constant to use
	vpclmulqdq	xmm8, xmm6, xmm10, 0x1
	vpclmulqdq	xmm6, xmm6, xmm10, 0x10
	vpxor	xmm7, xmm8
	vpxor	xmm7, xmm6


	; instead of 128, we add 112 to the loop counter to save 1 instruction from the loop
	; instead of a cmp instruction, we use the negative flag with the jl instruction
	add	buf_len, 128-16
	jl	_final_reduction_for_128

	; now we have 16+y bytes left to reduce. 16 Bytes is in register xmm7 and the rest is in memory
	; we can fold 16 bytes at a time if y>=16
	; continue folding 16B at a time

_16B_reduction_loop:
	vpclmulqdq	xmm8, xmm7, xmm10, 0x1
	vpclmulqdq	xmm7, xmm7, xmm10, 0x10
	vpxor	xmm7, xmm8
	vmovdqu	xmm0, [in_buf]
	vpxor	xmm7, xmm0
	add	in_buf, 16
	sub	buf_len, 16
	; instead of a cmp instruction, we utilize the flags with the jge instruction
	; equivalent of: cmp buf_len, 16-16
	; check if there is any more 16B in the buffer to be able to fold
	jge	_16B_reduction_loop

	;now we have 16+z bytes left to reduce, where 0<= z < 16.
	;first, we reduce the data in the xmm7 register


_final_reduction_for_128:
	; check if any more data to fold. If not, compute the CRC of the final 128 bits
	add	buf_len, 16
	je	_128_done

	; here we are getting data that is less than 16 bytes.
	; since we know that there was data before the pointer, we can offset the input pointer before the actual point, to receive exactly 16 bytes.
	; after that the registers need to be adjusted.
_get_last_two_xmms:
        vmovdqa xmm2, xmm7
        vmovdqu xmm1, [in_buf - 16 + buf_len]

        ; get rid of the extra data that was loaded before
        ; load the shift constant
        lea     rax, [pshufb_shf_table]
        add     rax, buf_len
        vmovdqu  xmm0, [rax]


        vpshufb  xmm7, xmm0
        vpxor    xmm0, [mask3]
        vpshufb  xmm2, xmm0

        vpblendvb  xmm2, xmm2, xmm1, xmm0
        ;;;;;;;;;;
        vpclmulqdq       xmm8, xmm7, xmm10, 0x1

        vpclmulqdq       xmm7, xmm7, xmm10, 0x10
        vpxor    xmm7, xmm8
        vpxor    xmm7, xmm2

_128_done:
        ; compute crc of a 128-bit value
        vmovdqa  xmm10, [rk5]
        vmovdqa  xmm0, xmm7

        ;64b fold
        vpclmulqdq       xmm7, xmm10, 0
        vpsrldq  xmm0, 8
        vpxor    xmm7, xmm0

        ;32b fold
        vmovdqa  xmm0, xmm7
        vpslldq  xmm7, 4
        vpclmulqdq       xmm7, xmm10, 0x10

        pxor    xmm7, xmm0


        ;barrett reduction
_barrett:
        vpand    xmm7, [mask2]
        vmovdqa  xmm1, xmm7
        vmovdqa  xmm2, xmm7
        vmovdqa  xmm10, [rk7]

        vpclmulqdq       xmm7, xmm10, 0
        vpxor    xmm7, xmm2
        vpand    xmm7, [mask]
        vmovdqa  xmm2, xmm7
        vpclmulqdq       xmm7, xmm10, 0x10
        vpxor    xmm7, xmm2
        vpxor    xmm7, xmm1
        vpextrd  eax, xmm7, 2

_cleanup:
%ifidn __OUTPUT_FORMAT__, win64
        vmovdqa  xmm6, [rsp + XMM_OFFSET + 16*0]
        vmovdqa  xmm7, [rsp + XMM_OFFSET + 16*1]
        vmovdqa  xmm8, [rsp + XMM_OFFSET + 16*2]
        vmovdqa  xmm9, [rsp + XMM_OFFSET + 16*3]
        vmovdqa  xmm10, [rsp + XMM_OFFSET + 16*4]
        vmovdqa  xmm11, [rsp + XMM_OFFSET + 16*5]
        vmovdqa  xmm12, [rsp + XMM_OFFSET + 16*6]
        vmovdqa  xmm13, [rsp + XMM_OFFSET + 16*7]
%endif
	add	rsp,VARIABLE_OFFSET
	ret


;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

align 16
_less_than_256:

	; check if there is enough buffer to be able to fold 16B at a time
	cmp	buf_len, 32
	jl	_less_than_32

	; if there is, load the constants
	vmovdqa	xmm10, [rk1]	; rk1 and rk2 in xmm10

	vmovd	xmm0, init_crc	; get the initial crc value
	vmovdqu	xmm7, [in_buf]	; load the plaintext
	vpxor	xmm7, xmm0

	; update the buffer pointer
	add	in_buf, 16

	; update the counter. subtract 32 instead of 16 to save one instruction from the loop
	sub	buf_len, 32

	jmp	_16B_reduction_loop


align 16
_less_than_32:
	; mov initial crc to the return value. this is necessary for zero-length buffers.
	mov	eax, init_crc
	test	buf_len, buf_len
	je	_cleanup

	vmovd	xmm0, init_crc	; get the initial crc value

	cmp	buf_len, 16
	je	_exact_16_left
	jl	_less_than_16_left

	vmovdqu	xmm7, [in_buf]	; load the plaintext
	vpxor	xmm7, xmm0	; xor the initial crc value
	add	in_buf, 16
	sub	buf_len, 16
	vmovdqa	xmm10, [rk1]	; rk1 and rk2 in xmm10
	jmp	_get_last_two_xmms


align 16
_less_than_16_left:
	; use stack space to load data less than 16 bytes, zero-out the 16B in memory first.

	vpxor	xmm1, xmm1
	mov	r11, rsp
	vmovdqa	[r11], xmm1

	cmp	buf_len, 4
	jl	_only_less_than_4

	;	backup the counter value
	mov	r9, buf_len
	cmp	buf_len, 8
	jl	_less_than_8_left

	; load 8 Bytes
	mov	rax, [in_buf]
	mov	[r11], rax
	add	r11, 8
	sub	buf_len, 8
	add	in_buf, 8
_less_than_8_left:

	cmp	buf_len, 4
	jl	_less_than_4_left

	; load 4 Bytes
	mov	eax, [in_buf]
	mov	[r11], eax
	add	r11, 4
	sub	buf_len, 4
	add	in_buf, 4
_less_than_4_left:

	cmp	buf_len, 2
	jl	_less_than_2_left

	; load 2 Bytes
	mov	ax, [in_buf]
	mov	[r11], ax
	add	r11, 2
	sub	buf_len, 2
	add	in_buf, 2
_less_than_2_left:
	cmp     buf_len, 1
        jl      _zero_left

	; load 1 Byte
	mov	al, [in_buf]
	mov	[r11], al
_zero_left:
	vmovdqa	xmm7, [rsp]
	vpxor	xmm7, xmm0	; xor the initial crc value

	lea	rax, [pshufb_shf_table]
	vmovdqu	xmm0, [rax + r9]
	vpshufb	xmm7, xmm0

	jmp	_128_done

align 16
_exact_16_left:
	vmovdqu	xmm7, [in_buf]
	vpxor	xmm7, xmm0	; xor the initial crc value

	jmp	_128_done

_only_less_than_4:
	cmp	buf_len, 3
	jl	_only_less_than_3

	; load 3 Bytes
	mov	al, [in_buf]
	mov	[r11], al

	mov	al, [in_buf+1]
	mov	[r11+1], al

	mov	al, [in_buf+2]
	mov	[r11+2], al

	vmovdqa	xmm7, [rsp]
	vpxor	xmm7, xmm0	; xor the initial crc value

	vpslldq	xmm7, 5

	jmp	_barrett
_only_less_than_3:
	cmp	buf_len, 2
	jl	_only_less_than_2

	; load 2 Bytes
	mov	al, [in_buf]
	mov	[r11], al

	mov	al, [in_buf+1]
	mov	[r11+1], al

	vmovdqa	xmm7, [rsp]
	vpxor	xmm7, xmm0	; xor the initial crc value

	vpslldq	xmm7, 6

	jmp	_barrett
_only_less_than_2:

	; load 1 Byte
	mov	al, [in_buf]
	mov	[r11], al

	vmovdqa	xmm7, [rsp]
	vpxor	xmm7, xmm0	; xor the initial crc value

	vpslldq	xmm7, 7

	jmp	_barrett

section .data

; precomputed constants
align 16
rk1: dq 0x00000000493c7d27
rk2: dq 0x0000000ec1068c50
rk3: dq 0x0000000206e38d70
rk4: dq 0x000000006992cea2
rk5: dq 0x00000000493c7d27
rk6: dq 0x00000000dd45aab8
rk7: dq 0x00000000dea713f0
rk8: dq 0x0000000105ec76f0
rk9: dq 0x0000000047db8317
rk10: dq 0x000000002ad91c30
rk11: dq 0x000000000715ce53
rk12: dq 0x00000000c49f4f67
rk13: dq 0x0000000039d3b296
rk14: dq 0x00000000083a6eec
rk15: dq 0x000000009e4addf8
rk16: dq 0x00000000740eef02
rk17: dq 0x00000000ddc0152b
rk18: dq 0x000000001c291d04
rk19: dq 0x00000000ba4fc28e
rk20: dq 0x000000003da6d0cb

mask:
dq     0xFFFFFFFFFFFFFFFF, 0x0000000000000000
mask2:
dq     0xFFFFFFFF00000000, 0xFFFFFFFFFFFFFFFF
mask3:
dq     0x8080808080808080, 0x8080808080808080

pshufb_shf_table:
dq 0x8786858483828100, 0x8f8e8d8c8b8a8988
dq 0x0706050403020100, 0x000e0d0c0b0a0908
