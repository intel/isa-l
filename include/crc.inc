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

%endif ;; CRC_INC
