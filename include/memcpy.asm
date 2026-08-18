;;
;; Copyright (c) 2023, Intel Corporation
;;
;; Redistribution and use in source and binary forms, with or without
;; modification, are permitted provided that the following conditions are met:
;;
;;     * Redistributions of source code must retain the above copyright notice,
;;       this list of conditions and the following disclaimer.
;;     * Redistributions in binary form must reproduce the above copyright
;;       notice, this list of conditions and the following disclaimer in the
;;       documentation and/or other materials provided with the distribution.
;;     * Neither the name of Intel Corporation nor the names of its contributors
;;       may be used to endorse or promote products derived from this software
;;       without specific prior written permission.
;;
;; THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
;; AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
;; IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
;; DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE
;; FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
;; DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
;; SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
;; CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
;; OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
;; OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
;;

%ifndef __MEMCPY_INC__
%define __MEMCPY_INC__

%include "reg_sizes.asm"

; This section defines macros to load small to medium amounts of data from
; memory to SIMD registers, where the size is variable but limited.
;
; The macros are called as:
; simd_load DST, SRC, SIZE
; with the parameters defined as:
;    DST     : register: destination XMM register
;    SRC     : register: pointer to src data (not modified)
;    SIZE    : register: length in bytes (not modified)
;
; The name indicates the options. The name is of the form:
; simd_load_<VEC>_<SZ><ZERO>
; where:
; <VEC> is the SIMD instruction type, e.g. "avx"
; <SZ> defines the largest value of SIZE
; <ZERO> is blank or "_1". If "_1" then the min SIZE is 1 (otherwise 0)
;
; For example:
; simd_load_avx_15_1	        : AVX, 1 <= size <= 15

%macro simd_load_avx_15_1 3
        __simd_load %1,%2,%3,0,0,AVX
%endm

%macro __simd_load 6
%define %%DST       %1    ; [out] destination XMM register
%define %%SRC       %2    ; [in] pointer to src data
%define %%SIZE      %3    ; [in] length in bytes (0-16 bytes)
%define %%ACCEPT_0  %4    ; 0 = min length = 1, 1 = min length = 0
%define %%ACCEPT_16 %5    ; 0 = max length = 15 , 1 = max length = 16
%define %%SIMDTYPE  %6    ; "SSE" or "AVX"

%ifidn %%SIMDTYPE, SSE
 %define %%MOVDQU movdqu
 %define %%PINSRB pinsrb
 %define %%PINSRQ pinsrq
 %define %%PXOR   pxor
%else
 %define %%MOVDQU vmovdqu
 %define %%PINSRB vpinsrb
 %define %%PINSRQ vpinsrq
 %define %%PXOR   vpxor
%endif

%if (%%ACCEPT_16 != 0)
        test    %%SIZE, 16
        jz      %%_skip_16
        %%MOVDQU %%DST, [%%SRC]
        jmp     %%end_load

%%_skip_16:
%endif
        %%PXOR  %%DST, %%DST ; clear XMM register
%if (%%ACCEPT_0 != 0)
        or      %%SIZE, %%SIZE
        je      %%end_load
%endif
        cmp     %%SIZE, 2
        jb      %%_size_1
        je      %%_size_2
        cmp     %%SIZE, 4
        jb      %%_size_3
        je      %%_size_4
        cmp     %%SIZE, 6
        jb      %%_size_5
        je      %%_size_6
        cmp     %%SIZE, 8
        jb      %%_size_7
        je      %%_size_8
        cmp     %%SIZE, 10
        jb      %%_size_9
        je      %%_size_10
        cmp     %%SIZE, 12
        jb      %%_size_11
        je      %%_size_12
        cmp     %%SIZE, 14
        jb      %%_size_13
        je      %%_size_14

%%_size_15:
        %%PINSRB %%DST, [%%SRC + 14], 14
%%_size_14:
        %%PINSRB %%DST, [%%SRC + 13], 13
%%_size_13:
        %%PINSRB %%DST, [%%SRC + 12], 12
%%_size_12:
        %%PINSRB %%DST, [%%SRC + 11], 11
%%_size_11:
        %%PINSRB %%DST, [%%SRC + 10], 10
%%_size_10:
        %%PINSRB %%DST, [%%SRC + 9], 9
%%_size_9:
        %%PINSRB %%DST, [%%SRC + 8], 8
%%_size_8:
        %%PINSRQ %%DST, [%%SRC], 0
        jmp    %%end_load
%%_size_7:
        %%PINSRB %%DST, [%%SRC + 6], 6
%%_size_6:
        %%PINSRB %%DST, [%%SRC + 5], 5
%%_size_5:
        %%PINSRB %%DST, [%%SRC + 4], 4
%%_size_4:
        %%PINSRB %%DST, [%%SRC + 3], 3
%%_size_3:
        %%PINSRB %%DST, [%%SRC + 2], 2
%%_size_2:
        %%PINSRB %%DST, [%%SRC + 1], 1
%%_size_1:
        %%PINSRB %%DST, [%%SRC + 0], 0
%%end_load:
%endm

%macro simd_load_avx2 5
%define %%DST       %1    ; [out] destination YMM register
%define %%SRC       %2    ; [in] pointer to src data
%define %%SIZE      %3    ; [in] length in bytes (0-32 bytes)
%define %%IDX       %4    ; [clobbered] Temp GP register to store src idx
%define %%TMP       %5    ; [clobbered] Temp GP register

        test    %%SIZE, 32
        jz      %%_skip_32
        vmovdqu %%DST, [%%SRC]
        jmp     %%end_load

%%_skip_32:
        vpxor   %%DST, %%DST ; clear YMM register
        or      %%SIZE, %%SIZE
        je      %%end_load

        lea     %%IDX, [%%SRC]
        mov     %%TMP, %%SIZE
        cmp     %%SIZE, 16
        jle     %%_check_size

        add     %%IDX, 16
        sub     %%TMP, 16

%%_check_size:
        cmp     %%TMP, 2
        jb      %%_size_1
        je      %%_size_2
        cmp     %%TMP, 4
        jb      %%_size_3
        je      %%_size_4
        cmp     %%TMP, 6
        jb      %%_size_5
        je      %%_size_6
        cmp     %%TMP, 8
        jb      %%_size_7
        je      %%_size_8
        cmp     %%TMP, 10
        jb      %%_size_9
        je      %%_size_10
        cmp     %%TMP, 12
        jb      %%_size_11
        je      %%_size_12
        cmp     %%TMP, 14
        jb      %%_size_13
        je      %%_size_14
        cmp     %%TMP, 15
        je      %%_size_15

%%_size_16:
        vmovdqu XWORD(%%DST), [%%IDX]
        jmp    %%end_load
%%_size_15:
        vpinsrb XWORD(%%DST), [%%IDX + 14], 14
%%_size_14:
        vpinsrb XWORD(%%DST), [%%IDX + 13], 13
%%_size_13:
        vpinsrb XWORD(%%DST), [%%IDX + 12], 12
%%_size_12:
        vpinsrb XWORD(%%DST), [%%IDX + 11], 11
%%_size_11:
        vpinsrb XWORD(%%DST), [%%IDX + 10], 10
%%_size_10:
        vpinsrb XWORD(%%DST), [%%IDX + 9], 9
%%_size_9:
        vpinsrb XWORD(%%DST), [%%IDX + 8], 8
%%_size_8:
        vpinsrq XWORD(%%DST), [%%IDX], 0
        jmp    %%_check_higher_16
%%_size_7:
        vpinsrb XWORD(%%DST), [%%IDX + 6], 6
%%_size_6:
        vpinsrb XWORD(%%DST), [%%IDX + 5], 5
%%_size_5:
        vpinsrb XWORD(%%DST), [%%IDX + 4], 4
%%_size_4:
        vpinsrb XWORD(%%DST), [%%IDX + 3], 3
%%_size_3:
        vpinsrb XWORD(%%DST), [%%IDX + 2], 2
%%_size_2:
        vpinsrb XWORD(%%DST), [%%IDX + 1], 1
%%_size_1:
        vpinsrb XWORD(%%DST), [%%IDX + 0], 0
%%_check_higher_16:
        test    %%SIZE, 16
        jz      %%end_load

        ; Move last bytes loaded to upper half and load 16 bytes in lower half
        vinserti128 %%DST, XWORD(%%DST), 1
        vinserti128 %%DST, [%%SRC], 0
%%end_load:
%endm

; This section defines a macro to store small to medium amounts of data from
; SIMD registers to memory, where the size is variable but limited.

%macro simd_store_avx2 5
%define %%DST      %1    ; register: pointer to dst (not modified)
%define %%SRC      %2    ; register: src data (clobbered)
%define %%SIZE     %3    ; register: length in bytes (not modified)
%define %%TMP      %4    ; 64-bit temp GPR (clobbered)
%define %%IDX      %5    ; 64-bit temp GPR to store dst idx (clobbered)

        xor %%IDX, %%IDX        ; zero idx

        test    %%SIZE, 32
        jz      %%lt32
        vmovdqu [%%DST], %%SRC
        jmp     %%end
%%lt32:

        test    %%SIZE, 16
        jz      %%lt16
        vmovdqu [%%DST], XWORD(%%SRC)
        ; Move upper half to lower half for further stores
        vperm2i128 %%SRC, %%SRC, %%SRC, 0x81
        add     %%IDX, 16
%%lt16:

        test    %%SIZE, 8
        jz      %%lt8
        vmovq  [%%DST + %%IDX], XWORD(%%SRC)
        vpsrldq XWORD(%%SRC), 8
        add     %%IDX, 8
%%lt8:

        vmovq %%TMP, XWORD(%%SRC)     ; use GPR from now on

        test    %%SIZE, 4
        jz      %%lt4
        mov     [%%DST + %%IDX], DWORD(%%TMP)
        shr     %%TMP, 32
        add     %%IDX, 4
%%lt4:

        test    %%SIZE, 2
        jz      %%lt2
        mov     [%%DST + %%IDX], WORD(%%TMP)
        shr     %%TMP, 16
        add     %%IDX, 2
%%lt2:
        test    %%SIZE, 1
        jz      %%end
        mov     [%%DST + %%IDX], BYTE(%%TMP)
%%end:
%endm

%endif ; ifndef __MEMCPY_INC__
