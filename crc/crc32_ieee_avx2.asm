;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;  Copyright(c) 2011-2026 Intel Corporation All rights reserved.
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
;       UINT32 crc32_ieee_avx2(
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

%include "reg_sizes.asm"
%include "memcpy.asm"

%ifndef fetch_dist
%define	fetch_dist	4096
%endif

%ifndef PREFETCH
%define PREFETCH        prefetcht1
%endif

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

%define init_crc arg1_low32
%define in_buf  arg2
%define buf_len arg3

align 16
mk_global 	crc32_ieee_avx2, function
crc32_ieee_avx2:
	endbranch
	not	init_crc

%ifidn __OUTPUT_FORMAT__, win64
	sub	        rsp,(16*10+8)
     ; push the xmm registers into the stack to maintain
	vmovdqa		[rsp + 16*0], xmm6
	vmovdqa		[rsp + 16*1], xmm7
	vmovdqa		[rsp + 16*2], xmm8
	vmovdqa		[rsp + 16*3], xmm9
	vmovdqa		[rsp + 16*4], xmm10
	vmovdqa		[rsp + 16*5], xmm11
	vmovdqa		[rsp + 16*6], xmm12
	vmovdqa     [rsp + 16*7], xmm13
	vmovdqa     [rsp + 16*8], xmm14
	vmovdqa     [rsp + 16*9], xmm15
%endif

    ; check if smaller than 256
	cmp	buf_len, 256
	; for sizes less than 256, we can't fold 128B at a time...
	jb	_less_than_256

	; load the initial crc value
	vmovd	xmm10, init_crc	; initial crc
    ; crc value does not need to be byte-reflected, but it needs to be moved to the high part of the register.
	; because data will be byte-reflected and will align with initial crc at correct place.
	vpslldq	xmm10, 12
    vbroadcasti128 ymm11, [SHUF_MASK]

    vmovdqu	ymm0, [in_buf+16*0]
	vmovdqu	ymm1, [in_buf+16*2]
	vmovdqu	ymm2, [in_buf+16*4]
	vmovdqu	ymm3, [in_buf+16*6]

    vpshufb ymm0, ymm11
	; XOR the initial_crc value
	vpxor	ymm0, ymm10
    vpshufb ymm1, ymm11
    vpshufb ymm2, ymm11
    vpshufb ymm3, ymm11

	vbroadcasti128	ymm10, [rk3]	; ymm10 has rk3 and rk4

	sub	buf_len, 256
	cmp	buf_len, 256
	jb	_fold_128_B_loop

	vmovdqu	ymm8, [in_buf+16*8]
	vmovdqu	ymm9, [in_buf+16*10]
	vmovdqu	ymm12, [in_buf+16*12]
	vmovdqu	ymm13, [in_buf+16*14]

    vpshufb ymm8, ymm11
    vpshufb ymm9, ymm11
    vpshufb ymm12, ymm11
    vpshufb ymm13, ymm11

	vbroadcasti128	ymm15, [rk_1]	; ymm15 has rk_1 and rk_2
	sub	buf_len, 256

%if fetch_dist != 0
	; check if there is at least 4KB (fetch distance) + 256B in the buffer
    cmp	buf_len, (fetch_dist + 256)
    jb	_fold_256_B_loop

align 16
_fold_and_prefetch_256_B_loop:
	add	in_buf, 256

    PREFETCH [in_buf+fetch_dist+0]
	vmovdqu ymm14, [in_buf+16*0]
    vpshufb ymm14, ymm11
	vpclmulqdq	ymm4, ymm0, ymm15, 0x0
	vpclmulqdq	ymm0, ymm0, ymm15, 0x11
	vpxor	ymm0, ymm14
	vpxor 	ymm0, ymm4

    vmovdqu ymm14, [in_buf+16*2]
    vpshufb ymm14, ymm11
	vpclmulqdq	ymm5, ymm1, ymm15, 0x0
	vpclmulqdq	ymm1, ymm1, ymm15, 0x11
	vpxor	ymm1, ymm14
	vpxor 	ymm1, ymm5

	PREFETCH [in_buf+fetch_dist+64]
    vmovdqu ymm14, [in_buf+16*4]
    vpshufb ymm14, ymm11
	vpclmulqdq	ymm4, ymm2, ymm15, 0x0
	vpclmulqdq	ymm2, ymm2, ymm15, 0x11
	vpxor	ymm2, ymm14
	vpxor 	ymm2, ymm4

    vmovdqu ymm14, [in_buf+16*6]
    vpshufb ymm14, ymm11
	vpclmulqdq	ymm5, ymm3, ymm15, 0x0
	vpclmulqdq	ymm3, ymm3, ymm15, 0x11
	vpxor	ymm3, ymm14
	vpxor 	ymm3, ymm5

	PREFETCH [in_buf+fetch_dist+64*2]
    vmovdqu ymm14, [in_buf+16*8]
    vpshufb ymm14, ymm11
	vpclmulqdq	ymm6, ymm8, ymm15, 0x0
	vpclmulqdq	ymm8, ymm8, ymm15, 0x11
	vpxor	ymm8, ymm14
	vpxor 	ymm8, ymm6

    vmovdqu ymm14, [in_buf+16*10]
    vpshufb ymm14, ymm11
	vpclmulqdq	ymm7, ymm9, ymm15, 0x0
	vpclmulqdq	ymm9, ymm9, ymm15, 0x11
	vpxor	ymm9, ymm14
	vpxor 	ymm9, ymm7

    PREFETCH [in_buf+fetch_dist+64*3]
    vmovdqu ymm14, [in_buf+16*12]
    vpshufb ymm14, ymm11
	vpclmulqdq	ymm6, ymm12, ymm15, 0x0
	vpclmulqdq	ymm12, ymm12, ymm15, 0x11
	vpxor	ymm12, ymm14
	vpxor 	ymm12, ymm6

    vmovdqu ymm14, [in_buf+16*14]
    vpshufb ymm14, ymm11
	vpclmulqdq	ymm7, ymm13, ymm15, 0x0
	vpclmulqdq	ymm13, ymm13, ymm15, 0x11
	vpxor	ymm13, ymm14
	vpxor 	ymm13, ymm7

	sub	buf_len, 256
	; check if there is another 256B in the buffer to be able to fold
	cmp buf_len, (fetch_dist + 256)
	jge	_fold_and_prefetch_256_B_loop
%endif ; fetch_dist != 0

align 16
_fold_256_B_loop:
	add	in_buf, 256

    vmovdqu ymm14, [in_buf+16*0]
    vpshufb ymm14, ymm11
	vpclmulqdq	ymm4, ymm0, ymm15, 0x0
	vpclmulqdq	ymm0, ymm0, ymm15, 0x11
	vpxor	ymm0, ymm14
	vpxor 	ymm0, ymm4

    vmovdqu ymm14, [in_buf+16*2]
    vpshufb ymm14, ymm11
	vpclmulqdq	ymm5, ymm1, ymm15, 0x0
	vpclmulqdq	ymm1, ymm1, ymm15, 0x11
	vpxor	ymm1, ymm14
	vpxor 	ymm1, ymm5

    vmovdqu ymm14, [in_buf+16*4]
    vpshufb ymm14, ymm11
	vpclmulqdq	ymm4, ymm2, ymm15, 0x0
	vpclmulqdq	ymm2, ymm2, ymm15, 0x11
	vpxor	ymm2, ymm14
	vpxor 	ymm2, ymm4

    vmovdqu ymm14, [in_buf+16*6]
    vpshufb ymm14, ymm11
	vpclmulqdq	ymm5, ymm3, ymm15, 0x0
	vpclmulqdq	ymm3, ymm3, ymm15, 0x11
	vpxor	ymm3, ymm14
	vpxor 	ymm3, ymm5

    vmovdqu ymm14, [in_buf+16*8]
    vpshufb ymm14, ymm11
	vpclmulqdq	ymm6, ymm8, ymm15, 0x0
	vpclmulqdq	ymm8, ymm8, ymm15, 0x11
	vpxor	ymm8, ymm14
	vpxor 	ymm8, ymm6

    vmovdqu ymm14, [in_buf+16*10]
    vpshufb ymm14, ymm11
	vpclmulqdq	ymm7, ymm9, ymm15, 0x0
	vpclmulqdq	ymm9, ymm9, ymm15, 0x11
	vpxor	ymm9, ymm14
	vpxor 	ymm9, ymm7

    vmovdqu ymm14, [in_buf+16*12]
    vpshufb ymm14, ymm11
	vpclmulqdq	ymm6, ymm12, ymm15, 0x0
	vpclmulqdq	ymm12, ymm12, ymm15, 0x11
	vpxor	ymm12, ymm14
	vpxor 	ymm12, ymm6

    vmovdqu ymm14, [in_buf+16*14]
    vpshufb ymm14, ymm11
	vpclmulqdq	ymm7, ymm13, ymm15, 0x0
	vpclmulqdq	ymm13, ymm13, ymm15, 0x11
	vpxor	ymm13, ymm14
	vpxor 	ymm13, ymm7

	sub	buf_len, 256
	; check if there is another 256B in the buffer to be able to fold
	jge	_fold_256_B_loop

	; Fold 256B into 128B using rk3/rk4
	add in_buf, 128
	vpclmulqdq	ymm4, ymm0, ymm10, 0x0
	vpclmulqdq	ymm0, ymm0, ymm10, 0x11
	vpxor	ymm0, ymm4
	vpxor	ymm0, ymm8

	vpclmulqdq	ymm5, ymm1, ymm10, 0x0
	vpclmulqdq	ymm1, ymm1, ymm10, 0x11
	vpxor	ymm1, ymm5
	vpxor	ymm1, ymm9

	vpclmulqdq	ymm4, ymm2, ymm10, 0x0
	vpclmulqdq	ymm2, ymm2, ymm10, 0x11
	vpxor	ymm2, ymm4
	vpxor	ymm2, ymm12

	vpclmulqdq	ymm5, ymm3, ymm10, 0x0
	vpclmulqdq	ymm3, ymm3, ymm10, 0x11
	vpxor	ymm3, ymm5
	vpxor	ymm3, ymm13

	add	buf_len, 128
    cmp buf_len, 128
	jl	_fold_less_than_128_B

align 16
_fold_128_B_loop:
	; update the buffer pointer
	add	in_buf, 128		;    buf += 128;

    vmovdqu ymm14, [in_buf+16*0]
    vpshufb ymm14, ymm11
	vpclmulqdq	ymm4, ymm0, ymm10, 0x0
	vpclmulqdq	ymm0, ymm0, ymm10, 0x11
	vpxor	ymm0, ymm14
	vpxor 	ymm0, ymm4

    vmovdqu ymm14, [in_buf+16*2]
    vpshufb ymm14, ymm11
	vpclmulqdq	ymm5, ymm1, ymm10, 0x0
	vpclmulqdq	ymm1, ymm1, ymm10, 0x11
	vpxor	ymm1, ymm14
	vpxor 	ymm1, ymm5

    vmovdqu ymm14, [in_buf+16*4]
    vpshufb ymm14, ymm11
	vpclmulqdq	ymm4, ymm2, ymm10, 0x0
	vpclmulqdq	ymm2, ymm2, ymm10, 0x11
	vpxor	ymm2, ymm14
	vpxor 	ymm2, ymm4

    vmovdqu ymm14, [in_buf+16*6]
    vpshufb ymm14, ymm11
	vpclmulqdq	ymm5, ymm3, ymm10, 0x0
	vpclmulqdq	ymm3, ymm3, ymm10, 0x11
	vpxor	ymm3, ymm14
	vpxor 	ymm3, ymm5
	sub	buf_len, 128

	; check if there is another 128B in the buffer to be able to fold
	jge	_fold_128_B_loop

align 16
_fold_less_than_128_B:
	; at this point, the buffer pointer is pointing at the last y Bytes of the buffer
	; the 128 of folded data is in 4 of the ymm registers: ymm0, ymm1, ymm2, ymm3
	add	in_buf, 128

	;Extract the final 16B into xmm7 - this is the accumulator
	vextracti128	xmm7, ymm3, 1

	; fold the rest of the data in the ymm registers into xmm7
    vmovdqu	ymm10, [rk9]
	vpclmulqdq	ymm4, ymm0, ymm10, 0x0
	vpclmulqdq	ymm12, ymm0, ymm10, 0x11
	vpxor	ymm12, ymm4 ; use ymm12 to accumulate all products

	vmovdqu	ymm10, [rk13]
	vpclmulqdq	ymm4, ymm1, ymm10, 0x0
	vpclmulqdq	ymm5, ymm1, ymm10, 0x11
	vpxor	ymm12, ymm4
	vpxor	ymm12, ymm5

	vmovdqu	ymm10, [rk17]
	vpclmulqdq	ymm4, ymm2, ymm10, 0x0
	vpclmulqdq	ymm5, ymm2, ymm10, 0x11
	vpxor	ymm12, ymm4
	vpxor	ymm12, ymm5

	vmovdqa	xmm10, [rk1]
	vpclmulqdq	xmm4, xmm3, xmm10, 0x0
	vpclmulqdq	xmm5, xmm3, xmm10, 0x11
	vpxor	ymm12, ymm4
	vpxor	ymm12, ymm5

	; horizontal xor of ymm7
	vextracti128	xmm4, ymm12, 1
	vpxor	xmm7, xmm4
	vpxor	xmm7, xmm12

	; instead of 128, we add 112 to the loop counter to save 1 instruction from the loop
	; instead of a cmp instruction, we use the negative flag with the jl instruction
	add	buf_len, 128-16
	jl	_final_reduction_for_128

align 16
_16B_reduction_loop:
	vmovdqa	xmm8, xmm7
	vpclmulqdq	xmm7, xmm10, 0x11
	vpclmulqdq	xmm8, xmm10, 0x0
	vpxor	xmm7, xmm8
	vmovdqu	xmm0, [in_buf]
	vpshufb	xmm0, xmm11
	vpxor	xmm7, xmm0
	add	in_buf, 16
	sub	buf_len, 16
	; instead of a cmp instruction, we utilize the flags with the jge instruction
	; equivalent of: cmp buf_len, 16-16
	; check if there is any more 16B in the buffer to be able to fold
	jge	_16B_reduction_loop

	;now we have 16+z bytes left to reduce, where 0<= z < 16.
	;first, we reduce the data in the xmm7 register

align 16
_final_reduction_for_128:
	; check if any more data to fold. If not, compute the CRC of the final 128 bits
	add	buf_len, 16
	je	_128_done

	; here we are getting data that is less than 16 bytes.
	; since we know that there was data before the pointer, we can offset the input pointer before the actual point, to receive exactly 16 bytes.
	; after that the registers need to be adjusted.
align 16
_get_last_two_xmms:
	vmovdqa	xmm2, xmm7

	vmovdqu	xmm1, [in_buf - 16 + buf_len]
	vpshufb	xmm1, xmm11

	; get rid of the extra data that was loaded before
	; load the shift constant
	lea	rax, [pshufb_shf_table + 16]
	sub	rax, buf_len
	vmovdqu	xmm0, [rax]

	; shift xmm2 to the left by buf_len bytes
	vpshufb	xmm2, xmm0

	; shift xmm7 to the right by 16-buf_len bytes
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

align 16
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
align 16
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
	not     eax
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
	add	        rsp,(16*10+8)
%endif
	ret

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

align 16
_less_than_256:

	; check if there is enough buffer to be able to fold 16B at a time
	cmp	buf_len, 32
	jb	_less_than_32
	vmovdqa xmm11, [SHUF_MASK]

	; if there is, load the constants
	vmovdqa	xmm10, [rk1]	; rk1 and rk2 in xmm10

	vmovd	xmm0, init_crc	; get the initial crc value
	vpslldq	xmm0, 12	; align it to its correct place
	vmovdqu	xmm7, [in_buf]	; load the plaintext
	vpshufb	xmm7, xmm11	; byte-reflect the plaintext
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

	vmovdqa xmm11, [SHUF_MASK]

	vmovd	xmm0, init_crc	; get the initial crc value
	vpslldq	xmm0, 12	; align it to its correct place

	cmp	buf_len, 16
	je	_exact_16_left
	jb	_less_than_16_left

	vmovdqu	xmm7, [in_buf]	; load the plaintext
	vpshufb	xmm7, xmm11	; byte-reflect the plaintext
	vpxor	xmm7, xmm0	; xor the initial crc value
	add	in_buf, 16
	sub	buf_len, 16
	vmovdqa	xmm10, [rk1]	; rk1 and rk2 in xmm10
	jmp	_get_last_two_xmms

align 16
_exact_16_left:
	vmovdqu	xmm7, [in_buf]
	vpshufb	xmm7, xmm11
	vpxor	xmm7, xmm0	; xor the initial crc value

	jmp	_128_done

align 16
_less_than_16_left:
	; load 1-15 bytes directly into xmm7
	simd_load_avx_15_1 xmm7, in_buf, buf_len
	vpshufb	xmm7, xmm11
	vpxor	xmm7, xmm0	; xor the initial crc value

	cmp	buf_len, 4
	jb	_only_less_than_4

	; use shuffle to align data
	lea	rax, [pshufb_shf_table + 16]
	sub	rax, buf_len
	vmovdqu	xmm0, [rax]
	vpxor	xmm0, [mask1]

	vpshufb	xmm7, xmm0
	jmp	_128_done

_only_less_than_4:
	; use shuffle table for variable right shift
	; offset = (8 - buf_len) - 5 = 3 - buf_len
	mov	rax, 3
	sub	rax, buf_len
	lea	r11, [pshufb_shift_table]
	add	r11, rax
	vmovdqu	xmm0, [r11]
	vpshufb	xmm7, xmm0
	jmp	_barrett

section .data

; precomputed constants
align 16
rk_1: dq 0x1851689900000000
rk_2: dq 0xa3dc855100000000
rk1 : dq 0xf200aa6600000000
rk2 : dq 0x17d3315d00000000
rk3 : dq 0x022ffca500000000
rk4 : dq 0x9d9ee22f00000000
rk5 : dq 0xf200aa6600000000
rk6 : dq 0x490d678d00000000
rk7 : dq 0x0000000104d101df
rk8 : dq 0x0000000104c11db7
rk9 : dq 0x6ac7e7d700000000
rk10 : dq 0xfcd922af00000000
rk11 : dq 0x34e45a6300000000
rk12 : dq 0x8762c1f600000000
rk13 : dq 0x5395a0ea00000000
rk14 : dq 0x54f2d5c700000000
rk15 : dq 0xd3504ec700000000
rk16 : dq 0x57a8445500000000
rk17 : dq 0xc053585d00000000
rk18 : dq 0x766f1b7800000000
rk19 : dq 0xcd8c54b500000000
rk20 : dq 0xab40b71e00000000

mask1: dq 0x8080808080808080, 0x8080808080808080
mask2: dq 0xFFFFFFFFFFFFFFFF, 0x00000000FFFFFFFF

SHUF_MASK: dq 0x08090A0B0C0D0E0F, 0x0001020304050607

pshufb_shf_table:
dq 0x8786858483828100, 0x8f8e8d8c8b8a8988
dq 0x0706050403020100, 0x000e0d0c0b0a0908

pshufb_shift_table:
	;; use these values for variable right shift (5, 6, 7 bytes)
	db 0x05, 0x06, 0x07, 0x08, 0x09, 0x0A, 0x0B
	db 0x0C, 0x0D, 0x0E, 0x0F, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF
