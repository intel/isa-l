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

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;       Function API:
;       UINT32 crc32_gzip_refl_avx2(
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
;       URL: http://download.intel.com/design/intarch/papers/323102.pdf
;
;       As explained here:
;       http://docs.oracle.com/javase/7/docs/api/java/util/zip/package-summary.html
;       CRC-32 checksum is described in RFC 1952
;       Implementing RFC 1952 CRC:
;       http://www.ietf.org/rfc/rfc1952.txt

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
	%xdefine	arg1 rcx
	%xdefine	arg2 rdx
	%xdefine	arg3 r8

	%xdefine	arg1_low32 ecx
%else
	%xdefine	arg1 rdi
	%xdefine	arg2 rsi
	%xdefine	arg3 rdx

	%xdefine	arg1_low32 edi
%endif

%define in_buf  arg2
%define buf_len arg3
%define init_crc arg1_low32

align 16
mk_global  crc32_gzip_refl_avx2, function
crc32_gzip_refl_avx2:
	endbranch
	not		init_crc

%ifidn __OUTPUT_FORMAT__, win64
    ; push the xmm registers into the stack to maintain
    sub      rsp, (16*8+8)
    vmovdqa  [rsp + 16*0], xmm6
    vmovdqa  [rsp + 16*1], xmm7
    vmovdqa  [rsp + 16*2], xmm8
    vmovdqa  [rsp + 16*3], xmm9
    vmovdqa  [rsp + 16*4], xmm10
    vmovdqa  [rsp + 16*5], xmm11
    vmovdqa  [rsp + 16*6], xmm12
    vmovdqa  [rsp + 16*7], xmm15
%endif

	; check if smaller than 256
	cmp	buf_len, 256
	; for sizes less than 256, we can't fold 128B at a time...
	jb	_less_than_256

	; load the initial crc value
	vmovd	xmm10, init_crc	; initial crc

	vmovdqu	ymm0, [in_buf+16*0]
	vmovdqu	ymm1, [in_buf+16*2]
	vmovdqu	ymm2, [in_buf+16*4]
	vmovdqu	ymm3, [in_buf+16*6]

	; XOR the initial_crc value
	vpxor	ymm0, ymm10

	vbroadcasti128	ymm10, [rk3]	; ymm10 has rk3 and rk4

	sub	buf_len, 256
	cmp	buf_len, 256
	jb	_fold_128_B_loop

	vmovdqu	ymm8, [in_buf+16*8]
	vmovdqu	ymm9, [in_buf+16*10]
	vmovdqu	ymm11, [in_buf+16*12]
	vmovdqu	ymm12, [in_buf+16*14]

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
	vpclmulqdq	ymm4, ymm0, ymm15, 0x10
	vpclmulqdq	ymm0, ymm0, ymm15, 0x01
	vpxor	ymm0, [in_buf+16*0]
	vpxor 	ymm0, ymm4

	vpclmulqdq	ymm5, ymm1, ymm15, 0x10
	vpclmulqdq	ymm1, ymm1, ymm15, 0x01
	vpxor	ymm1, [in_buf+16*2]
	vpxor 	ymm1, ymm5

	PREFETCH [in_buf+fetch_dist+64]
	vpclmulqdq	ymm4, ymm2, ymm15, 0x10
	vpclmulqdq	ymm2, ymm2, ymm15, 0x01
	vpxor	ymm2, [in_buf+16*4]
	vpxor 	ymm2, ymm4

	vpclmulqdq	ymm5, ymm3, ymm15, 0x10
	vpclmulqdq	ymm3, ymm3, ymm15, 0x01
	vpxor	ymm3, [in_buf+16*6]
	vpxor 	ymm3, ymm5

	PREFETCH [in_buf+fetch_dist+64*2]
	vpclmulqdq	ymm6, ymm8, ymm15, 0x10
	vpclmulqdq	ymm8, ymm8, ymm15, 0x01
	vpxor	ymm8, [in_buf+16*8]
	vpxor 	ymm8, ymm6

	vpclmulqdq	ymm7, ymm9, ymm15, 0x10
	vpclmulqdq	ymm9, ymm9, ymm15, 0x01
	vpxor	ymm9, [in_buf+16*10]
	vpxor 	ymm9, ymm7

	PREFETCH [in_buf+fetch_dist+64*3]
	vpclmulqdq	ymm6, ymm11, ymm15, 0x10
	vpclmulqdq	ymm11, ymm11, ymm15, 0x01
	vpxor	ymm11, [in_buf+16*12]
	vpxor 	ymm11, ymm6

	vpclmulqdq	ymm7, ymm12, ymm15, 0x10
	vpclmulqdq	ymm12, ymm12, ymm15, 0x01
	vpxor	ymm12, [in_buf+16*14]
	vpxor 	ymm12, ymm7

	sub	buf_len, 256
	; check if there is another 256B in the buffer to be able to fold
	cmp buf_len, (fetch_dist + 256)
	jge	_fold_and_prefetch_256_B_loop
%endif ; fetch_dist != 0

align 16
_fold_256_B_loop:
	add	in_buf, 256
	vpclmulqdq	ymm4, ymm0, ymm15, 0x10
	vpclmulqdq	ymm0, ymm0, ymm15, 0x01
	vpxor	ymm0, [in_buf+16*0]
	vpxor 	ymm0, ymm4

	vpclmulqdq	ymm5, ymm1, ymm15, 0x10
	vpclmulqdq	ymm1, ymm1, ymm15, 0x01
	vpxor	ymm1, [in_buf+16*2]
	vpxor 	ymm1, ymm5

	vpclmulqdq	ymm4, ymm2, ymm15, 0x10
	vpclmulqdq	ymm2, ymm2, ymm15, 0x01
	vpxor	ymm2, [in_buf+16*4]
	vpxor 	ymm2, ymm4

	vpclmulqdq	ymm5, ymm3, ymm15, 0x10
	vpclmulqdq	ymm3, ymm3, ymm15, 0x01
	vpxor	ymm3, [in_buf+16*6]
	vpxor 	ymm3, ymm5

	vpclmulqdq	ymm6, ymm8, ymm15, 0x10
	vpclmulqdq	ymm8, ymm8, ymm15, 0x01
	vpxor	ymm8, [in_buf+16*8]
	vpxor 	ymm8, ymm6

	vpclmulqdq	ymm7, ymm9, ymm15, 0x10
	vpclmulqdq	ymm9, ymm9, ymm15, 0x01
	vpxor	ymm9, [in_buf+16*10]
	vpxor 	ymm9, ymm7

	vpclmulqdq	ymm6, ymm11, ymm15, 0x10
	vpclmulqdq	ymm11, ymm11, ymm15, 0x01
	vpxor	ymm11, [in_buf+16*12]
	vpxor 	ymm11, ymm6

	vpclmulqdq	ymm7, ymm12, ymm15, 0x10
	vpclmulqdq	ymm12, ymm12, ymm15, 0x01
	vpxor	ymm12, [in_buf+16*14]
	vpxor 	ymm12, ymm7

	sub	buf_len, 256
	; check if there is another 256B in the buffer to be able to fold
	jge	_fold_256_B_loop

	; Fold 256B into 128B using rk3/rk4
	add in_buf, 128
	vpclmulqdq	ymm4, ymm0, ymm10, 0x10
	vpclmulqdq	ymm0, ymm0, ymm10, 0x01
	vpxor	ymm0, ymm4
	vpxor	ymm0, ymm8

	vpclmulqdq	ymm5, ymm1, ymm10, 0x10
	vpclmulqdq	ymm1, ymm1, ymm10, 0x01
	vpxor	ymm1, ymm5
	vpxor	ymm1, ymm9

	vpclmulqdq	ymm4, ymm2, ymm10, 0x10
	vpclmulqdq	ymm2, ymm2, ymm10, 0x01
	vpxor	ymm2, ymm4
	vpxor	ymm2, ymm11

	vpclmulqdq	ymm5, ymm3, ymm10, 0x10
	vpclmulqdq	ymm3, ymm3, ymm10, 0x01
	vpxor	ymm3, ymm5
	vpxor	ymm3, ymm12

	add	buf_len, 128
	cmp buf_len, 128
	jl	_fold_less_than_128_B

align 16
_fold_128_B_loop:
	; update the buffer pointer
	add	in_buf, 128		;    buf += 128;

	vpclmulqdq	ymm4, ymm0, ymm10, 0x10
	vpclmulqdq	ymm0, ymm0, ymm10, 0x1
	vpxor	ymm0, [in_buf+16*0]
	vpxor 	ymm0, ymm4

	vpclmulqdq	ymm5, ymm1, ymm10, 0x10
	vpclmulqdq	ymm1, ymm1, ymm10, 0x1
	vpxor	ymm1, [in_buf+16*2]
	vpxor 	ymm1, ymm5

	vpclmulqdq	ymm4, ymm2, ymm10, 0x10
	vpclmulqdq	ymm2, ymm2, ymm10, 0x1
	vpxor	ymm2, [in_buf+16*4]
	vpxor 	ymm2, ymm4

	vpclmulqdq	ymm5, ymm3, ymm10, 0x10
	vpclmulqdq	ymm3, ymm3, ymm10, 0x1
	vpxor	ymm3, [in_buf+16*6]
	vpxor 	ymm3, ymm5
	sub	buf_len, 128

	; check if there is another 128B in the buffer to be able to fold
	jge	_fold_128_B_loop

_fold_less_than_128_B:
	; at this point, the buffer pointer is pointing at the last y Bytes of the buffer
	; the 128 of folded data is in 4 of the ymm registers: ymm0, ymm1, ymm2, ymm3
	add	in_buf, 128

	vextracti128	xmm7, ymm3, 1
	; fold the rest of the data in the ymm registers into xmm7
	vmovdqu	ymm10, [rk9]
	vpclmulqdq	ymm4, ymm0, ymm10, 0x01
	vpclmulqdq	ymm11, ymm0, ymm10, 0x10
	vpxor	ymm11, ymm4 ; use ymm11 to accumulate all products

	vmovdqu	ymm10, [rk13]
	vpclmulqdq	ymm4, ymm1, ymm10, 0x01
	vpclmulqdq	ymm5, ymm1, ymm10, 0x10
	vpxor	ymm11, ymm4
	vpxor	ymm11, ymm5

	vmovdqu	ymm10, [rk17]
	vpclmulqdq	ymm4, ymm2, ymm10, 0x01
	vpclmulqdq	ymm5, ymm2, ymm10, 0x10
	vpxor	ymm11, ymm4
	vpxor	ymm11, ymm5

	vmovdqa	xmm10, [rk1]
	vpclmulqdq	xmm4, xmm3, xmm10, 0x01
	vpclmulqdq	xmm5, xmm3, xmm10, 0x10
	vpxor	ymm11, ymm4
	vpxor	ymm11, ymm5

	; horizontal xor of ymm7
	vextracti128	xmm4, ymm11, 1
	vpxor	xmm7, xmm4
	vpxor	xmm7, xmm11

	; instead of 128, we add 112 to the loop counter to save 1 instruction from the loop
	; instead of a cmp instruction, we use the negative flag with the jl instruction
	add	buf_len, 128-16
	jl	_final_reduction_for_128

_16B_reduction_loop:
	vpclmulqdq	xmm8, xmm7, xmm10, 0x1
	vpclmulqdq	xmm7, xmm7, xmm10, 0x10
	vpxor		xmm7, xmm8
	vmovdqu		xmm0, [in_buf]
	vpxor		xmm7, xmm0
	add		in_buf, 16
	sub		buf_len, 16
	; instead of a cmp instruction, we utilize the flags with the jge instruction
	; equivalent of: cmp buf_len, 16-16
	; check if there is any more 16B in the buffer to be able to fold
	jge		_16B_reduction_loop

	;now we have 16+z bytes left to reduce, where 0<= z < 16.
	;first, we reduce the data in the xmm7 register

_final_reduction_for_128:
	add		buf_len, 16
	je		_128_done

	; here we are getting data that is less than 16 bytes.
	; since we know that there was data before the pointer, we can offset
	; the input pointer before the actual point, to receive exactly 16 bytes.
	; after that the registers need to be adjusted.
_get_last_two_xmms:

	vmovdqa		xmm2, xmm7
	vmovdqu		xmm1, [in_buf - 16 + buf_len]

	; get rid of the extra data that was loaded before
	; load the shift constant
	lea		rax, [pshufb_shf_table]
	vmovdqu		xmm0, [rax + buf_len]

	vpshufb		xmm7, xmm0
	vpxor		xmm0, [mask3]
	vpshufb		xmm2, xmm0

	vpblendvb	xmm2, xmm2, xmm1, xmm0
	;;;;;;;;;;
	vpclmulqdq	xmm8, xmm7, xmm10, 0x1
	vpclmulqdq	xmm7, xmm7, xmm10, 0x10
	vpxor		xmm7, xmm8
	vpxor		xmm7, xmm2

_128_done:
	; compute crc of a 128-bit value
	vmovdqa		xmm10, [rk5]
	vmovdqa		xmm0, xmm7

	;64b fold
	vpclmulqdq	xmm7, xmm10, 0
	vpsrldq		xmm0, 8
	vpxor		xmm7, xmm0

	;32b fold
	vmovdqa		xmm0, xmm7
	vpslldq		xmm7, 4
	vpclmulqdq	xmm7, xmm10, 0x10
	vpxor		xmm7, xmm0

	;barrett reduction
_barrett:
	vpand		xmm7, [mask2]
	vmovdqa		xmm1, xmm7
	vmovdqa		xmm2, xmm7
	vmovdqa		xmm10, [rk7]

	vpclmulqdq	xmm7, xmm10, 0
	vpxor		xmm7, xmm2
	vpand		xmm7, [mask]
	vmovdqa		xmm2, xmm7
	vpclmulqdq	xmm7, xmm10, 0x10
	vpxor		xmm7, xmm2
	vpxor		xmm7, xmm1
	vpextrd		eax, xmm7, 2

_cleanup:
	not		eax

%ifidn __OUTPUT_FORMAT__, win64
    vmovdqa  xmm6, [rsp + 16*0]
    vmovdqa  xmm7, [rsp + 16*1]
    vmovdqa  xmm8, [rsp + 16*2]
    vmovdqa  xmm9, [rsp + 16*3]
    vmovdqa  xmm10, [rsp + 16*4]
    vmovdqa  xmm11, [rsp + 16*5]
    vmovdqa  xmm12, [rsp + 16*6]
    vmovdqa  xmm15, [rsp + 16*7]
    add      rsp, (16*8+8)
%endif
	ret

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

align 16
_less_than_256:

	; check if there is enough buffer to be able to fold 16B at a time
	cmp		buf_len, 32
	jb	_less_than_32

	; if there is, load the constants
	vmovdqa	xmm10, [rk1]    ; rk1 and rk2 in xmm10

	vmovd	xmm0, init_crc	; get the initial crc value
	vmovdqu	xmm7, [in_buf]		; load the plaintext
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
	jb	_less_than_16_left

	vmovdqu	xmm7, [in_buf]		; load the plaintext
	vpxor	xmm7, xmm0		; xor the initial crc value
	add	in_buf, 16
	sub	buf_len, 16
	vmovdqa	xmm10, [rk1]		; rk1 and rk2 in xmm10
	jmp	_get_last_two_xmms

align 16
_exact_16_left:
	vmovdqu	xmm7, [in_buf]
	vpxor	xmm7, xmm0      ; xor the initial crc value
	jmp	_128_done

align 16
_less_than_16_left:
	; load 1-15 bytes directly into xmm7
	simd_load_avx_15_1 xmm7, in_buf, buf_len
	vpxor	xmm7, xmm0	; xor the initial crc value

	cmp	buf_len, 4
	jb	_only_less_than_4

	; use shuffle to align data
	lea	rax, [pshufb_shf_table]
	vmovdqu	xmm0, [rax + buf_len]
	vpshufb	xmm7, xmm0
	jmp	_128_done

_only_less_than_4:
	; use pshufb to shift left by (8-buf_len) bytes
	lea	rax, [pshufb_shift_table]
	vmovdqu	xmm0, [rax + buf_len]
	vpshufb	xmm7, xmm0
	jmp	_barrett

section .data

; precomputed constants
align 16
rk_1: dq 0x00000000e95c1271
rk_2: dq 0x00000000ce3371cb
rk1:  dq 0x00000000ccaa009e
rk2:  dq 0x00000001751997d0
rk3:  dq 0x000000014a7fe880
rk4:  dq 0x00000001e88ef372
rk5:  dq 0x00000000ccaa009e
rk6:  dq 0x0000000163cd6124
rk7:  dq 0x00000001f7011640
rk8:  dq 0x00000001db710640
rk9:  dq 0x00000001d7cfc6ac
rk10: dq 0x00000001ea89367e
rk11: dq 0x000000018cb44e58
rk12: dq 0x00000000df068dc2
rk13: dq 0x00000000ae0b5394
rk14: dq 0x00000001c7569e54
rk15: dq 0x00000001c6e41596
rk16: dq 0x0000000154442bd4
rk17: dq 0x0000000174359406
rk18: dq 0x000000003db1ecdc
rk19: dq 0x000000015a546366
rk20: dq 0x00000000f1da05aa

mask:  dq     0xFFFFFFFFFFFFFFFF, 0x0000000000000000
mask2: dq     0xFFFFFFFF00000000, 0xFFFFFFFFFFFFFFFF
mask3: dq     0x8080808080808080, 0x8080808080808080

pshufb_shf_table:
; use these values for shift constants for the pshufb instruction
; different alignments result in values as shown:
;       dq 0x8887868584838281, 0x008f8e8d8c8b8a89 ; shl 15 (16-1) / shr1
;       dq 0x8988878685848382, 0x01008f8e8d8c8b8a ; shl 14 (16-3) / shr2
;       dq 0x8a89888786858483, 0x0201008f8e8d8c8b ; shl 13 (16-4) / shr3
;       dq 0x8b8a898887868584, 0x030201008f8e8d8c ; shl 12 (16-4) / shr4
;       dq 0x8c8b8a8988878685, 0x04030201008f8e8d ; shl 11 (16-5) / shr5
;       dq 0x8d8c8b8a89888786, 0x0504030201008f8e ; shl 10 (16-6) / shr6
;       dq 0x8e8d8c8b8a898887, 0x060504030201008f ; shl 9  (16-7) / shr7
;       dq 0x8f8e8d8c8b8a8988, 0x0706050403020100 ; shl 8  (16-8) / shr8
;       dq 0x008f8e8d8c8b8a89, 0x0807060504030201 ; shl 7  (16-9) / shr9
;       dq 0x01008f8e8d8c8b8a, 0x0908070605040302 ; shl 6  (16-10) / shr10
;       dq 0x0201008f8e8d8c8b, 0x0a09080706050403 ; shl 5  (16-11) / shr11
;       dq 0x030201008f8e8d8c, 0x0b0a090807060504 ; shl 4  (16-12) / shr12
;       dq 0x04030201008f8e8d, 0x0c0b0a0908070605 ; shl 3  (16-13) / shr13
;       dq 0x0504030201008f8e, 0x0d0c0b0a09080706 ; shl 2  (16-14) / shr14
;       dq 0x060504030201008f, 0x0e0d0c0b0a090807 ; shl 1  (16-15) / shr15
dq 0x8786858483828100, 0x8f8e8d8c8b8a8988
dq 0x0706050403020100, 0x000e0d0c0b0a0908

pshufb_shift_table:
	; use these values to shift data for the pshufb instruction (for 1-3 byte inputs)
	db 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF
	db 0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07
	db 0x08, 0x09, 0x0A
