;;
;; Copyright(c) 2026 Intel Corporation All rights reserved.
;;
;; Redistribution and use in source and binary forms, with or without
;; modification, are permitted provided that the following conditions
;; are met:
;;   * Redistributions of source code must retain the above copyright
;;     notice, this list of conditions and the following disclaimer.
;;   * Redistributions in binary form must reproduce the above copyright
;;     notice, this list of conditions and the following disclaimer in
;;     the documentation and/or other materials provided with the
;;     distribution.
;;   * Neither the name of Intel Corporation nor the names of its
;;     contributors may be used to endorse or promote products derived
;;     from this software without specific prior written permission.
;;
;; THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
;; "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
;; LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
;; A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
;; OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
;; SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
;; LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
;; DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
;; THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
;; (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
;; OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
;;

%ifndef CRC_INC
%define CRC_INC

;;
;; Layout of the precomputed constants block shared by all CRC implementations
;; (crc32_gzip_refl, crc32_iscsi, crc32_ieee, crc16_t10dif, crc64_iso_norm/refl
;; and all polynomial variants).  Field order matches crc_const.asm so that
;; [r10 + crc_fold_const_<field>] produces the correct offset.
;;
struc CRC_FOLD_CONST
crc_fold_const_fold_16x128b:    resq 2  ;; offset   0 -- 256-byte unrolled loop
crc_fold_const_fold_1x128b:     resq 2  ;; offset  16 -- 128-bit CRC fold
crc_fold_const_fold_8x128b:     resq 2  ;; offset  32 -- 128-byte loop
crc_fold_const_fold_128b_to_64b: resq 2 ;; offset  48 -- reduce 128b -> 64b
crc_fold_const_barrett:         resq 2  ;; offset  64 -- Barrett reduction
crc_fold_const_fold_7x128b:     resq 2  ;; offset  80 -- fold 7x128b  (zmm load +80..+143)
crc_fold_const_fold_6x128b:     resq 2  ;; offset  96 -- fold 6x128b
crc_fold_const_fold_5x128b:     resq 2  ;; offset 112 -- fold 5x128b
crc_fold_const_fold_4x128b:     resq 2  ;; offset 128 -- fold 4x128b
crc_fold_const_fold_3x128b:     resq 2  ;; offset 144 -- fold 3x128b  (zmm load +144..+207)
crc_fold_const_fold_2x128b:     resq 2  ;; offset 160 -- fold 2x128b
crc_fold_const_fold_1x128b_b:   resq 4  ;; offset 176 -- fold_1x128b copy + 2x zero (zmm tail)
endstruc

;; CRC_FOLD_ZMM acc, tmp, fold_const, src, imm_tmp, imm_acc
;;   Folds a 512-bit ZMM accumulator using two EVEX vpclmulqdq steps and vpternlogq.
%macro CRC_FOLD_ZMM 6
%define %%ACC       %1  ; [in/out] ZMM accumulator
%define %%TMP       %2  ; [clobbered] ZMM for first product
%define %%FOLD_CST  %3  ; [in] ZMM holding fold polynomial constants
%define %%SRC       %4  ; [in] ZMM source data
%define %%IMM_TMP   %5  ; [in] vpclmulqdq immediate for first (TMP) multiply
%define %%IMM_ACC   %6  ; [in] vpclmulqdq immediate for second (ACC) multiply
	vpclmulqdq	%%TMP, %%ACC, %%FOLD_CST, %%IMM_TMP
	vpclmulqdq	%%ACC, %%ACC, %%FOLD_CST, %%IMM_ACC
	vpternlogq	%%ACC, %%TMP, %%SRC, 0x96
%endmacro

;; CRC_FOLD_ZMM_SHUF acc, tmp, data_tmp, fold_const, mem
;;   One fold step for normal (byte-swapping) CRC (imm8 0x00 / 0x11).
;;   Loads MEM into DATA_TMP, byte-swaps it using zmm18 (must be loaded
;;   by the caller), then folds into ACC via vpternlogq.
%macro CRC_FOLD_ZMM_SHUF 5
%define %%ACC       %1  ; [in/out] ZMM accumulator
%define %%TMP       %2  ; [clobbered] ZMM for high-half product
%define %%DATA_TMP  %3  ; [clobbered] ZMM for loaded + byte-swapped data
%define %%FOLD_CST  %4  ; [in] ZMM holding fold polynomial constants
%define %%MEM       %5  ; [in] memory operand [address + offset]
	vmovdqu8	%%DATA_TMP, %%MEM
	vpshufb		%%DATA_TMP, %%DATA_TMP, zmm18
	vpclmulqdq	%%TMP, %%ACC, %%FOLD_CST, 0x00
	vpclmulqdq	%%ACC, %%ACC, %%FOLD_CST, 0x11
	vpternlogq	%%ACC, %%TMP, %%DATA_TMP, 0x96
%endmacro

;; CRC_FOLD_8_TO_1_ZMM fold_7x128, fold_3x128, imm1, imm2, ztmp1, ztmp2
;;   Folds 8 128b chunks (zmm0, zmm4) down to a single 128b value in xmm7
;;   using packed ZMM carry-less multiplies.
;;   Also pre-loads xmm10 with fold_1x128b ready for the 16-byte loop.
;;   Clobbers: zmm1, zmm2, zmm5, zmm6, zmm7, zmm8, xmm5, xmm7, xmm10.
%macro CRC_FOLD_8_TO_1_ZMM 6
%define %%FOLD_CONST_7x128  %1  ; [in] address of fold_7x128b constants
%define %%FOLD_CONST_3x128  %2  ; [in] address of fold_3x128b constants
%define %%IMM1              %3  ; [in] vpclmulqdq first immediate  (e.g., 0x01 REFL, 0x00 NORM)
%define %%IMM2              %4  ; [in] vpclmulqdq second immediate (e.g., 0x10 REFL, 0x11 NORM)
%define %%ZTMP1             %5  ; [clobbered] ZMM register for fold_7x128b constants
%define %%ZTMP2             %6  ; [clobbered] ZMM register for fold_3x128b constants
	vmovdqu8	%%ZTMP1, %%FOLD_CONST_7x128
	vmovdqu8	%%ZTMP2, %%FOLD_CONST_3x128
	vpclmulqdq	zmm1, zmm0, %%ZTMP1, %%IMM1
	vpclmulqdq	zmm2, zmm0, %%ZTMP1, %%IMM2
	vextracti64x2	xmm7, zmm4, 3
	vpclmulqdq	zmm5, zmm4, %%ZTMP2, %%IMM1
	vpclmulqdq	zmm6, zmm4, %%ZTMP2, %%IMM2
	vmovdqa		xmm10, [r10 + crc_fold_const_fold_1x128b]
	vpternlogq	zmm1, zmm2, zmm5, 0x96
	vpternlogq	zmm1, zmm6, zmm7, 0x96
	vshufi64x2	zmm8, zmm1, zmm1, 0x4e
	vpxorq		ymm8, ymm8, ymm1
	vextracti64x2	xmm5, ymm8, 1
	vpxorq		xmm7, xmm5, xmm8
%endmacro
;; CRC_FOLD_YMM_XMM acc, tmp, fold_const, src, imm_tmp, imm_acc
;;   Generic YMM/XMM fold step (VEX vpclmulqdq×2 + vpxor×2). Works for any
;;   register source or memory operand; no byte-swap performed.
;;   For reflected CRC:            imm_tmp=0x10, imm_acc=0x01
;;   For normal CRC register fold: imm_tmp=0x0,  imm_acc=0x11
%macro CRC_FOLD_YMM_XMM 6
%define %%ACC       %1  ; [in/out] YMM/XMM accumulator
%define %%TMP       %2  ; [clobbered] YMM/XMM for first product
%define %%FOLD_CST  %3  ; [in] YMM/XMM holding fold polynomial constants
%define %%SRC       %4  ; [in] YMM/XMM source data
%define %%IMM_TMP   %5  ; [in] vpclmulqdq immediate for first (TMP) multiply
%define %%IMM_ACC   %6  ; [in] vpclmulqdq immediate for second (ACC) multiply
	vpclmulqdq	%%TMP, %%ACC, %%FOLD_CST, %%IMM_TMP
	vpclmulqdq	%%ACC, %%ACC, %%FOLD_CST, %%IMM_ACC
	vpxor		%%ACC, %%SRC
	vpxor		%%ACC, %%TMP
%endmacro

;; CRC_FOLD_YMM_SHUF acc, tmp, fold_const, mem
;;   One YMM fold step for normal (byte-swapping) CRC (VEX vpclmulqdq 0x00/0x11 + vpxor×2).
;;   Uses ymm14 as the data scratch register and ymm11 as the byte-swap shuffle
;;   mask (both must be set up by the caller before entering the fold loop).
%macro CRC_FOLD_YMM_SHUF 4
%define %%ACC       %1  ; [in/out] YMM accumulator
%define %%TMP       %2  ; [clobbered] YMM for low-half product
%define %%FOLD_CST  %3  ; [in] YMM holding fold polynomial constants
%define %%MEM       %4  ; [in] memory operand [address + offset]
	vmovdqu		ymm14, %%MEM
	vpshufb		ymm14, ymm11
	vpclmulqdq	%%TMP, %%ACC, %%FOLD_CST, 0x0
	vpclmulqdq	%%ACC, %%ACC, %%FOLD_CST, 0x11
	vpxor		%%ACC, ymm14
	vpxor		%%ACC, %%TMP
%endmacro


;; CRC_FOLD_YMM_TO_XMM7 accum, imm1, imm2
;;   Folds ymm0..ymm3 (4x128B accumulated CRC) down into xmm7 for the 16B
;;   reduction loop.  Works for both reflected and normal CRC by parameterising
;;   the accumulator register and the vpclmulqdq immediate bytes:
;;     REFL: CRC_FOLD_YMM_TO_XMM7 ymm11, 0x01, 0x10
;;     NORM: CRC_FOLD_YMM_TO_XMM7 ymm12, 0x0,  0x11
;;   Clobbers: ymm4, ymm5, ymm10, xmm4, xmm5, xmm7, xmm10.
%macro CRC_FOLD_YMM_TO_XMM7 3
%define %%ACCUM     %1  ; [in/out] YMM accumulator
%define %%IMM1      %2  ; [in] vpclmulqdq first immediate
%define %%IMM2      %3  ; [in] vpclmulqdq second immediate
	vextracti128	xmm7, ymm3, 1
	vmovdqu		ymm10, [r10 + crc_fold_const_fold_7x128b]
	vpclmulqdq	ymm4, ymm0, ymm10, %%IMM1
	vpclmulqdq	%%ACCUM, ymm0, ymm10, %%IMM2
	vpxor		%%ACCUM, ymm4
	vmovdqu		ymm10, [r10 + crc_fold_const_fold_5x128b]
	vpclmulqdq	ymm4, ymm1, ymm10, %%IMM1
	vpclmulqdq	ymm5, ymm1, ymm10, %%IMM2
	vpxor		%%ACCUM, ymm4
	vpxor		%%ACCUM, ymm5
	vmovdqu		ymm10, [r10 + crc_fold_const_fold_3x128b]
	vpclmulqdq	ymm4, ymm2, ymm10, %%IMM1
	vpclmulqdq	ymm5, ymm2, ymm10, %%IMM2
	vpxor		%%ACCUM, ymm4
	vpxor		%%ACCUM, ymm5
	vmovdqa		xmm10, [r10 + crc_fold_const_fold_1x128b]
	vpclmulqdq	xmm4, xmm3, xmm10, %%IMM1
	vpclmulqdq	xmm5, xmm3, xmm10, %%IMM2
	vpxor		%%ACCUM, ymm4
	vpxor		%%ACCUM, ymm5
	vextracti128	xmm4, %%ACCUM, 1
	vpxor		xmm7, xmm4
	vextracti128	xmm4, %%ACCUM, 0
	vpxor		xmm7, xmm4
%endmacro

;; CRC_FOLD_PAIR_STEP_XMM_REFL acc1, acc2, fold_const, mem1, mem2
;;   Two-accumulator XMM fold step for reflected-input CRC (SSE pclmulqdq 0x10/0x01).
;;   Clobbers xmm8, xmm9, xmm12, xmm13 (used as scratch for copies and loads).
%macro CRC_FOLD_PAIR_STEP_XMM_REFL 5
%define %%ACC1      %1  ; [in/out] XMM first accumulator
%define %%ACC2      %2  ; [in/out] XMM second accumulator
%define %%FOLD_CST  %3  ; [in] XMM holding fold polynomial constants
%define %%MEM1      %4  ; [in] first memory operand
%define %%MEM2      %5  ; [in] second memory operand
	movdqu		xmm9, %%MEM1
	movdqu		xmm12, %%MEM2
	movdqa		xmm8, %%ACC1
	movdqa		xmm13, %%ACC2
	pclmulqdq	%%ACC1, %%FOLD_CST, 0x10
	pclmulqdq	xmm8, %%FOLD_CST, 0x01
	pclmulqdq	%%ACC2, %%FOLD_CST, 0x10
	pclmulqdq	xmm13, %%FOLD_CST, 0x01
	pxor		%%ACC1, xmm9
	xorps		%%ACC1, xmm8
	pxor		%%ACC2, xmm12
	xorps		%%ACC2, xmm13
%endmacro

;; CRC_FOLD_PAIR_STEP_XMM_NORM acc1, acc2, fold_const, mem1, mem2
;;   Two-accumulator XMM fold step for normal (byte-swapping) CRC (SSE pclmulqdq 0x00/0x11).
;;   Clobbers xmm8, xmm9, xmm12, xmm13. Uses xmm11 as the byte-swap mask
;;   (must be loaded by the caller before entering the fold loop).
%macro CRC_FOLD_PAIR_STEP_XMM_NORM 5
%define %%ACC1      %1  ; [in/out] XMM first accumulator
%define %%ACC2      %2  ; [in/out] XMM second accumulator
%define %%FOLD_CST  %3  ; [in] XMM holding fold polynomial constants
%define %%MEM1      %4  ; [in] first memory operand
%define %%MEM2      %5  ; [in] second memory operand
	movdqu		xmm9, %%MEM1
	movdqu		xmm12, %%MEM2
	pshufb		xmm9, xmm11
	pshufb		xmm12, xmm11
	movdqa		xmm8, %%ACC1
	movdqa		xmm13, %%ACC2
	pclmulqdq	%%ACC1, %%FOLD_CST, 0x0
	pclmulqdq	xmm8, %%FOLD_CST, 0x11
	pclmulqdq	%%ACC2, %%FOLD_CST, 0x0
	pclmulqdq	xmm13, %%FOLD_CST, 0x11
	pxor		%%ACC1, xmm9
	xorps		%%ACC1, xmm8
	pxor		%%ACC2, xmm12
	xorps		%%ACC2, xmm13
%endmacro

;; CRC_FOLD_8_TO_1_XMM imm_acc, imm_hi, xtmp1, xtmp2
;;   Folds xmm0..xmm6 into xmm7 using the fold_7x..fold_1x 128b constants.
;;   After the macro, XTMP2 holds fold_1x128b, ready for the 16B reduction loop.
;;   imm_acc: pclmulqdq immediate for the accumulator register (e.g., 0x01 REFL, 0x11 NORM)
;;   imm_tmp: pclmulqdq immediate for XTMP1                   (e.g., 0x10 REFL, 0x00 NORM)
%macro CRC_FOLD_8_TO_1_XMM 4
%define %%IMM_ACC   %1  ; [in] pclmulqdq immediate for accumulator (e.g., 0x01 REFL, 0x11 NORM)
%define %%IMM_TMP   %2  ; [in] pclmulqdq immediate for XTMP1       (e.g., 0x10 REFL, 0x00 NORM)
%define %%XTMP1     %3  ; [clobbered] XMM scratch register
%define %%XTMP2     %4  ; [clobbered] XMM register for fold constants
	movdqa		%%XTMP2, [r10 + crc_fold_const_fold_7x128b]
	movdqa		%%XTMP1, xmm0
	pclmulqdq	xmm0, %%XTMP2, %%IMM_ACC
	pclmulqdq	%%XTMP1, %%XTMP2, %%IMM_TMP
	pxor		xmm7, %%XTMP1
	pxor		xmm7, xmm0
	movdqa		%%XTMP2, [r10 + crc_fold_const_fold_6x128b]
	movdqa		%%XTMP1, xmm1
	pclmulqdq	xmm1, %%XTMP2, %%IMM_ACC
	pclmulqdq	%%XTMP1, %%XTMP2, %%IMM_TMP
	pxor		xmm7, %%XTMP1
	pxor		xmm7, xmm1
	movdqa		%%XTMP2, [r10 + crc_fold_const_fold_5x128b]
	movdqa		%%XTMP1, xmm2
	pclmulqdq	xmm2, %%XTMP2, %%IMM_ACC
	pclmulqdq	%%XTMP1, %%XTMP2, %%IMM_TMP
	pxor		xmm7, %%XTMP1
	pxor		xmm7, xmm2
	movdqa		%%XTMP2, [r10 + crc_fold_const_fold_4x128b]
	movdqa		%%XTMP1, xmm3
	pclmulqdq	xmm3, %%XTMP2, %%IMM_ACC
	pclmulqdq	%%XTMP1, %%XTMP2, %%IMM_TMP
	pxor		xmm7, %%XTMP1
	pxor		xmm7, xmm3
	movdqa		%%XTMP2, [r10 + crc_fold_const_fold_3x128b]
	movdqa		%%XTMP1, xmm4
	pclmulqdq	xmm4, %%XTMP2, %%IMM_ACC
	pclmulqdq	%%XTMP1, %%XTMP2, %%IMM_TMP
	pxor		xmm7, %%XTMP1
	pxor		xmm7, xmm4
	movdqa		%%XTMP2, [r10 + crc_fold_const_fold_2x128b]
	movdqa		%%XTMP1, xmm5
	pclmulqdq	xmm5, %%XTMP2, %%IMM_ACC
	pclmulqdq	%%XTMP1, %%XTMP2, %%IMM_TMP
	pxor		xmm7, %%XTMP1
	pxor		xmm7, xmm5
	movdqa		%%XTMP2, [r10 + crc_fold_const_fold_1x128b]
	movdqa		%%XTMP1, xmm6
	pclmulqdq	xmm6, %%XTMP2, %%IMM_ACC
	pclmulqdq	%%XTMP1, %%XTMP2, %%IMM_TMP
	pxor		xmm7, %%XTMP1
	pxor		xmm7, xmm6
%endmacro

%endif ;; CRC_INC

