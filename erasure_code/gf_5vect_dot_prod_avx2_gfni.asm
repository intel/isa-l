;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;  Copyright(c) 2026 Alibaba Group All rights reserved.
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
;    * Neither the name of Alibaba Group nor the names of its
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

;;;
;;; gf_5vect_dot_prod_avx2_gfni(len, vec, *g_tbls, **buffs, **dests);
;;;

%include "reg_sizes.asm"
%include "gf_vect_gfni.inc"
%include "memcpy.asm"

%ifidn __OUTPUT_FORMAT__, elf64
 %define arg0  rdi      ; len
 %define arg1  rsi      ; vec
 %define arg2  rdx      ; g_tbls
 %define arg3  rcx      ; buffs
 %define arg4  r8       ; dests
 %define arg5  r9

 %define tmp   r11
 %define tmp2  r10
 %define tmp3  r13      ; must be saved and restored
 %define tmp4  r12      ; must be saved and restored
 %define tmp5  r14      ; must be saved and restored
 %define tmp6  r15      ; must be saved and restored
 %define tmp7  rbp      ; must be saved and restored
 %define tmp8  rbx      ; must be saved and restored

 %define stack_size  6*8
 %define func(x) x: endbranch
 %macro FUNC_SAVE 0
    sub     rsp, stack_size
    mov     [rsp + 0*8], r12
    mov     [rsp + 1*8], r13
    mov     [rsp + 2*8], r14
    mov     [rsp + 3*8], r15
    mov     [rsp + 4*8], rbp
    mov     [rsp + 5*8], rbx
 %endmacro
 %macro FUNC_RESTORE 0
    mov     r12, [rsp + 0*8]
    mov     r13, [rsp + 1*8]
    mov     r14, [rsp + 2*8]
    mov     r15, [rsp + 3*8]
    mov     rbp, [rsp + 4*8]
    mov     rbx, [rsp + 5*8]
    add     rsp, stack_size
 %endmacro
%endif

%ifidn __OUTPUT_FORMAT__, win64
 %define arg0   rcx
 %define arg1   rdx
 %define arg2   r8
 %define arg3   r9

 %define arg4   r12     ; must be saved, loaded and restored
 %define arg5   r15     ; must be saved and restored
 %define tmp    r11
 %define tmp2   r10
 %define tmp3   r13     ; must be saved and restored
 %define tmp4   r14     ; must be saved and restored
 %define tmp5   rdi     ; must be saved and restored
 %define tmp6   rsi     ; must be saved and restored
 %define tmp7   rbp     ; must be saved and restored
 %define tmp8   rbx     ; must be saved and restored
 %define stack_size  5*16 + 9*8      ; must be an odd multiple of 8
 %define arg(x)      [rsp + stack_size + 8 + 8*x]

 %define func(x) proc_frame x
 %macro FUNC_SAVE 0
    alloc_stack stack_size
    vmovdqa [rsp + 0*16], xmm6
    vmovdqa [rsp + 1*16], xmm7
    vmovdqa [rsp + 2*16], xmm8
    vmovdqa [rsp + 3*16], xmm9
    vmovdqa [rsp + 4*16], xmm10
    mov     [rsp + 5*16 + 0*8], r12
    mov     [rsp + 5*16 + 1*8], r13
    mov     [rsp + 5*16 + 2*8], r14
    mov     [rsp + 5*16 + 3*8], r15
    mov     [rsp + 5*16 + 4*8], rdi
    mov     [rsp + 5*16 + 5*8], rsi
    mov     [rsp + 5*16 + 6*8], rbp
    mov     [rsp + 5*16 + 7*8], rbx
    end_prolog
    mov     arg4, arg(4)
 %endmacro

 %macro FUNC_RESTORE 0
    vmovdqa xmm6,  [rsp + 0*16]
    vmovdqa xmm7,  [rsp + 1*16]
    vmovdqa xmm8,  [rsp + 2*16]
    vmovdqa xmm9,  [rsp + 3*16]
    vmovdqa xmm10, [rsp + 4*16]
    mov     r12,  [rsp + 5*16 + 0*8]
    mov     r13,  [rsp + 5*16 + 1*8]
    mov     r14,  [rsp + 5*16 + 2*8]
    mov     r15,  [rsp + 5*16 + 3*8]
    mov     rdi,  [rsp + 5*16 + 4*8]
    mov     rsi,  [rsp + 5*16 + 5*8]
    mov     rbp,  [rsp + 5*16 + 6*8]
    mov     rbx,  [rsp + 5*16 + 7*8]
    add     rsp, stack_size
 %endmacro
%endif


%define len    arg0
%define vec    arg1
%define mul_array arg2
%define src    arg3
%define dest1  arg4
%define ptr    arg5
%define vec_i  tmp2
%define dest2  tmp3
%define dest3  tmp4
%define dest4  tmp5
%define vskip3 tmp6
%define dest5  tmp7
%define vskip4 tmp8
%define pos    rax

%ifndef EC_ALIGNED_ADDR
;;; Use Un-aligned load/store
 %define XLDR vmovdqu
 %define XSTR vmovdqu
%else
;;; Use Non-temporal load/stor
 %ifdef NO_NT_LDST
  %define XLDR vmovdqa
  %define XSTR vmovdqa
 %else
  %define XLDR vmovntdqa
  %define XSTR vmovntdq
 %endif
%endif

%define x0      ymm0

%define xp1     ymm1
%define xp2     ymm2
%define xp3     ymm3
%define xp4     ymm4
%define xp5     ymm5

%define xgft1   ymm6
%define xgft2   ymm7
%define xgft3   ymm8
%define xgft4   ymm9
%define xgft5   ymm10

default rel
[bits 64]

section .text

;;
;; Encodes 32 bytes of all "k" sources into 5x 32 bytes (parity disks).
;;
%macro ENCODE_32B_5 0
    vpxor   xp1, xp1, xp1
    vpxor   xp2, xp2, xp2
    vpxor   xp3, xp3, xp3
    vpxor   xp4, xp4, xp4
    vpxor   xp5, xp5, xp5
    mov     tmp, mul_array
    xor     vec_i, vec_i

%%next_vect:
    mov     ptr, [src + vec_i]
    XLDR    x0, [ptr + pos]             ;Get next source vector (32 bytes)
    add     vec_i, 8

    vmovdqu xgft1, [tmp]
    vmovdqu xgft2, [tmp + vec*4]
    vmovdqu xgft3, [tmp + vec*8]
    vmovdqu xgft4, [tmp + vskip3]
    vmovdqu xgft5, [tmp + vskip4]
    add     tmp, 32

    GF_MUL_XOR VEX, x0, xgft1, xgft1, xp1, xgft2, xgft2, xp2, \
                       xgft3, xgft3, xp3, xgft4, xgft4, xp4, \
                       xgft5, xgft5, xp5

    cmp     vec_i, vec
    jl      %%next_vect

    XSTR    [dest1 + pos], xp1
    XSTR    [dest2 + pos], xp2
    XSTR    [dest3 + pos], xp3
    XSTR    [dest4 + pos], xp4
    XSTR    [dest5 + pos], xp5
%endmacro

;;
;; Encodes less than 32 bytes of all "k" sources into 5 parity disks.
;;
%macro ENCODE_LT_32B_5 1
%define %%LEN   %1

    vpxor   xp1, xp1, xp1
    vpxor   xp2, xp2, xp2
    vpxor   xp3, xp3, xp3
    vpxor   xp4, xp4, xp4
    vpxor   xp5, xp5, xp5
    xor     vec_i, vec_i

%%next_vect:
    mov     ptr, [src + vec_i]
    simd_load_avx2 x0, ptr + pos, %%LEN, tmp, vskip3 ;Get next source vector
    imul    vskip3, vec, 12             ;; restore vskip3 (clobbered by simd_load)

    lea     tmp, [mul_array + vec_i*4]
    add     vec_i, 8

    vmovdqu xgft1, [tmp]
    vmovdqu xgft2, [tmp + vec*4]
    vmovdqu xgft3, [tmp + vec*8]
    vmovdqu xgft4, [tmp + vskip3]
    vmovdqu xgft5, [tmp + vskip4]

    GF_MUL_XOR VEX, x0, xgft1, xgft1, xp1, xgft2, xgft2, xp2, \
                       xgft3, xgft3, xp3, xgft4, xgft4, xp4, \
                       xgft5, xgft5, xp5

    cmp     vec_i, vec
    jl      %%next_vect

    ;Store updated encoded data
    lea     ptr, [dest1 + pos]
    simd_store_avx2 ptr, xp1, %%LEN, tmp, vec_i

    lea     ptr, [dest2 + pos]
    simd_store_avx2 ptr, xp2, %%LEN, tmp, vec_i

    lea     ptr, [dest3 + pos]
    simd_store_avx2 ptr, xp3, %%LEN, tmp, vec_i

    lea     ptr, [dest4 + pos]
    simd_store_avx2 ptr, xp4, %%LEN, tmp, vec_i

    lea     ptr, [dest5 + pos]
    simd_store_avx2 ptr, xp5, %%LEN, tmp, vec_i
%endmacro

align 16
mk_global gf_5vect_dot_prod_avx2_gfni, function
func(gf_5vect_dot_prod_avx2_gfni)
    FUNC_SAVE

    xor     pos, pos
    mov     vskip3, vec
    imul    vskip3, 32*3                ;; vskip3 = vec*96 (parity-row-3 stride)
    mov     vskip4, vec
    imul    vskip4, 32*4                ;; vskip4 = vec*128 (parity-row-4 stride)
    shl     vec, 3                      ;; vec *= 8. Make vec_i count by 8
    mov     dest2, [dest1 + 8]
    mov     dest3, [dest1 + 2*8]
    mov     dest4, [dest1 + 3*8]
    mov     dest5, [dest1 + 4*8]
    mov     dest1, [dest1]

    cmp     len, 32
    jl      .len_lt_32

.loop32:
    ENCODE_32B_5

    add     pos, 32     ;; encode next 32 bytes
    sub     len, 32
    cmp     len, 32
    jge     .loop32

.len_lt_32:
    or      len, len
    jz      .exit

    ENCODE_LT_32B_5 len

.exit:
    vzeroupper

    FUNC_RESTORE
    ret

endproc_frame
