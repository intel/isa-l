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
;       UINT32 crc32_iscsi_avx2(
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
;       AVX2 version using 256-bit YMM registers for improved performance
;

%include "reg_sizes.asm"
%include "crc.inc"

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

%xdefine	tmp	r10
%xdefine	tmp2 r11

align 16
mk_global  crc32_iscsi_avx2, function
crc32_iscsi_avx2:
	endbranch
	lea	r10, [rel crc32_iscsi_const]

%ifidn __OUTPUT_FORMAT__, win64
		sub	rsp, (16*7+8)
        ; push the xmm registers into the stack to maintain
        vmovdqa  [rsp + 16*0], xmm6
        vmovdqa  [rsp + 16*1], xmm7
        vmovdqa  [rsp + 16*2], xmm8
        vmovdqa  [rsp + 16*3], xmm9
        vmovdqa  [rsp + 16*4], xmm10
        vmovdqa  [rsp + 16*5], xmm11
        vmovdqa  [rsp + 16*6], xmm12
%endif

	;; fastpath for short data
	mov	eax, init_crc
	cmp	buf_len, 4
	jb	_less_than_4
	cmp	buf_len, 8
	jb	_less_than_8
	cmp	buf_len, 16
	jbe	_no_more_than_16

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

	vbroadcasti128	ymm10, [r10 + crc_fold_const_fold_8x128b]	; ymm10 has rk3 and rk4

	sub	buf_len, 256
	cmp	buf_len, 256
	jb	_fold_128_B_loop

	vmovdqu	ymm8, [in_buf+16*8]
	vmovdqu	ymm9, [in_buf+16*10]
	vmovdqu	ymm11, [in_buf+16*12]
	vmovdqu	ymm12, [in_buf+16*14]

	vbroadcasti128	ymm15, [r10 + crc_fold_const_fold_16x128b]	; ymm15 has rk_1 and rk_2
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
	vmovdqu	ymm10, [r10 + crc_fold_const_fold_7x128b]
	vpclmulqdq	ymm4, ymm0, ymm10, 0x01
	vpclmulqdq	ymm11, ymm0, ymm10, 0x10
	vpxor	ymm11, ymm4 ; use ymm11 to accumulate all products

	vmovdqu	ymm10, [r10 + crc_fold_const_fold_5x128b]
	vpclmulqdq	ymm4, ymm1, ymm10, 0x01
	vpclmulqdq	ymm5, ymm1, ymm10, 0x10
	vpxor	ymm11, ymm4
	vpxor	ymm11, ymm5

	vmovdqu	ymm10, [r10 + crc_fold_const_fold_3x128b]
	vpclmulqdq	ymm4, ymm2, ymm10, 0x01
	vpclmulqdq	ymm5, ymm2, ymm10, 0x10
	vpxor	ymm11, ymm4
	vpxor	ymm11, ymm5

	vmovdqa	xmm10, [r10 + crc_fold_const_fold_1x128b]
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
    lea     rax, [rel shf_table_refl]
    add     rax, buf_len
    vmovdqu  xmm0, [rax]

	vpshufb  xmm7, xmm0
    vpxor    xmm0, [rel shf_xor_mask]
    vpshufb  xmm2, xmm0

    vpblendvb  xmm2, xmm2, xmm1, xmm0
    ;;;;;;;;;;
    vpclmulqdq       xmm8, xmm7, xmm10, 0x1
    vpclmulqdq       xmm7, xmm7, xmm10, 0x10
    vpxor    xmm7, xmm8
    vpxor    xmm7, xmm2

_128_done:
	; compute crc of a 128-bit value
	; using CRC32Q can be easier than barrett reduction
    vmovq   tmp, xmm7
    vpextrq tmp2, xmm7, 1
    xor     rax, rax
    crc32   rax, tmp
    crc32   rax, tmp2

_cleanup:
%ifidn __OUTPUT_FORMAT__, win64
    vmovdqa  xmm6, [rsp + 16*0]
    vmovdqa  xmm7, [rsp + 16*1]
    vmovdqa  xmm8, [rsp + 16*2]
    vmovdqa  xmm9, [rsp + 16*3]
    vmovdqa  xmm10, [rsp + 16*4]
    vmovdqa  xmm11, [rsp + 16*5]
    vmovdqa  xmm12, [rsp + 16*6]

	add	rsp, (16*7 + 8)
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

	; if there is, load the constants
	vmovdqa	xmm10, [r10 + crc_fold_const_fold_1x128b]	; rk1 and rk2 in xmm10

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
	; data length can't be less than 17 bytes. already dealt with these cases in fastpath.
	; mov initial crc to the return value. this is necessary for zero-length buffers.
	vmovd	xmm0, init_crc	; get the initial crc value
	vmovdqu	xmm7, [in_buf]	; load the plaintext
	vpxor	xmm7, xmm0	; xor the initial crc value
	add	in_buf, 16
	sub	buf_len, 16
	vmovdqa	xmm10, [r10 + crc_fold_const_fold_1x128b]	; rk1 and rk2 in xmm10
	jmp	_get_last_two_xmms


; fastpath for short data
align 16
_no_more_than_16:
	test	buf_len, 16		; check if exact 16 bytes
	jz	_less_than_16		; no, do 8 bytes check
	crc32	rax, qword[in_buf]
	crc32	rax, qword[in_buf+8]
	jmp	_cleanup		; done

align 16
_less_than_16:
	test	buf_len, 8		; check if 8 bytes remaining at least
	jz	_less_than_8		; no, do 4 bytes check
	crc32	rax, qword[in_buf]	; calculate 8 bytes anyway
	add	in_buf,8

_less_than_8:
	test	buf_len, 4		; check if 4 bytes remaining at least
	jz	_less_than_4		; no, do 2 bytes check
	crc32	eax, dword[in_buf]	; calculate 4 bytes anyway
	add	in_buf, 4

_less_than_4:
	test	buf_len, 2		; check if 2 bytes remaining at least
	jz	_less_than_2		; no, do 1 byte check
	crc32	eax, word[in_buf]	; calculate 2 bytes anyway
	add	in_buf,2

_less_than_2:
	test	buf_len,1		; check if 1 byte remaining
	jz	_cleanup		; no, done
	crc32	eax, byte[in_buf]	; calculate 1 byte
	jmp	_cleanup		; all done


%include "crc_const_extern.asm"
