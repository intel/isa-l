;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;  Copyright(c) 2026 Intel Corporation All rights reserved.
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

%include "reg_sizes.asm"

[bits 64]
default rel

section .rodata
; ---------------------------------------------------------------------------
; CRC-32 gzip  (polynomial 0x04C11DB7, reflected)
; ---------------------------------------------------------------------------
align 16
mk_global crc32_gzip_refl_const, data, hidden
crc32_gzip_refl_const:
        dq 0x00000000e95c1271, 0x00000000ce3371cb   ; fold_16x128b
        dq 0x00000000ccaa009e, 0x00000001751997d0   ; fold_1x128b
        dq 0x000000014a7fe880, 0x00000001e88ef372   ; fold_8x128b
        dq 0x00000000ccaa009e, 0x0000000163cd6124   ; fold_128b_to_64b
        dq 0x00000001f7011640, 0x00000001db710640   ; reduce_64b_to_32b
        dq 0x00000001d7cfc6ac, 0x00000001ea89367e   ; fold_7x128b
        dq 0x000000018cb44e58, 0x00000000df068dc2   ; fold_6x128b
        dq 0x00000000ae0b5394, 0x00000001c7569e54   ; fold_5x128b
        dq 0x00000001c6e41596, 0x0000000154442bd4   ; fold_4x128b
        dq 0x0000000174359406, 0x000000003db1ecdc   ; fold_3x128b
        dq 0x000000015a546366, 0x00000000f1da05aa   ; fold_2x128b
        dq 0x00000000ccaa009e, 0x00000001751997d0   ; fold_1x128b_b
        dq 0x0000000000000000, 0x0000000000000000   ; padding

; ---------------------------------------------------------------------------
; CRC-32 IEEE 802.3  (polynomial 0x04C11DB7, non-reflected / bit-reversed)
; ---------------------------------------------------------------------------
align 16
mk_global crc32_ieee_const, data, hidden
crc32_ieee_const:
        dq 0x1851689900000000, 0xa3dc855100000000   ; fold_16x128b
        dq 0xf200aa6600000000, 0x17d3315d00000000   ; fold_1x128b
        dq 0x022ffca500000000, 0x9d9ee22f00000000   ; fold_8x128b
        dq 0xf200aa6600000000, 0x490d678d00000000   ; fold_128b_to_64b
        dq 0x0000000104d101df, 0x0000000104c11db7   ; reduce_64b_to_32b
        dq 0x6ac7e7d700000000, 0xfcd922af00000000   ; rk9/rk10
        dq 0x34e45a6300000000, 0x8762c1f600000000   ; rk11/rk12
        dq 0x5395a0ea00000000, 0x54f2d5c700000000   ; rk13/rk14
        dq 0xd3504ec700000000, 0x57a8445500000000   ; rk15/rk16
        dq 0xc053585d00000000, 0x766f1b7800000000   ; rk17/rk18
        dq 0xcd8c54b500000000, 0xab40b71e00000000   ; rk19/rk20
        dq 0xf200aa6600000000, 0x17d3315d00000000   ; fold_1x128b_b (= fold_1x128b)
        dq 0x0000000000000000, 0x0000000000000000   ; padding

; ---------------------------------------------------------------------------
; CRC-32C iSCSI  (polynomial 0x1EDC6F41 Castagnoli, reflected)
; ---------------------------------------------------------------------------
align 16
mk_global crc32_iscsi_const, data, hidden
crc32_iscsi_const:
        dq 0x00000000b9e02b86, 0x00000000dcb17aa4   ; fold_16x128b
        dq 0x00000000493c7d27, 0x0000000ec1068c50   ; fold_1x128b
        dq 0x0000000206e38d70, 0x000000006992cea2   ; fold_8x128b
        dq 0x00000000493c7d27, 0x00000000dd45aab8   ; fold_128b_to_64b
        dq 0x00000000dea713f0, 0x0000000105ec76f0   ; reduce_64b_to_32b
        dq 0x0000000047db8317, 0x000000002ad91c30   ; rk9/rk10
        dq 0x000000000715ce53, 0x00000000c49f4f67   ; rk11/rk12
        dq 0x0000000039d3b296, 0x00000000083a6eec   ; rk13/rk14
        dq 0x000000009e4addf8, 0x00000000740eef02   ; rk15/rk16
        dq 0x00000000ddc0152b, 0x000000001c291d04   ; rk17/rk18
        dq 0x00000000ba4fc28e, 0x000000003da6d0cb   ; rk19/rk20
        dq 0x00000000493c7d27, 0x0000000ec1068c50   ; fold_1x128b_b (= fold_1x128b)
        dq 0x0000000000000000, 0x0000000000000000   ; padding

; ---------------------------------------------------------------------------
; CRC-16 T10 DIF  (polynomial 0x8BB7)
; ---------------------------------------------------------------------------
align 16
mk_global crc16_t10dif_const, data, hidden
crc16_t10dif_const:
        dq 0xdccf000000000000, 0x4b0b000000000000   ; fold_16x128b  (rk_1/rk_2)
        dq 0x2d56000000000000, 0x06df000000000000   ; fold_1x128b   (rk1/rk2)
        dq 0x9d9d000000000000, 0x7cf5000000000000   ; fold_8x128b   (rk3/rk4)
        dq 0x2d56000000000000, 0x1368000000000000   ; fold_128b_to_64b (rk5/rk6)
        dq 0x00000001f65a57f8, 0x000000018bb70000   ; reduce_64b_to_32b (rk7/rk8)
        dq 0xceae000000000000, 0xbfd6000000000000   ; rk9/rk10
        dq 0x1e16000000000000, 0x713c000000000000   ; rk11/rk12
        dq 0xf7f9000000000000, 0x80a6000000000000   ; rk13/rk14
        dq 0x044c000000000000, 0xe658000000000000   ; rk15/rk16
        dq 0xad18000000000000, 0xa497000000000000   ; rk17/rk18
        dq 0x6ee3000000000000, 0xe7b5000000000000   ; rk19/rk20
        dq 0x2d56000000000000, 0x06df000000000000   ; fold_1x128b_b (= fold_1x128b)
        dq 0x0000000000000000, 0x0000000000000000   ; padding

; ---------------------------------------------------------------------------
; CRC-16 T10 DIF copy variant  (polynomial 0x8BB7, 4-stream copy)
; ---------------------------------------------------------------------------
align 16
mk_global crc16_t10dif_copy_const, data, hidden
crc16_t10dif_copy_const:
        dq 0x0000000000000000, 0x0000000000000000   ; fold_16x128b (unused)
        dq 0x2d56000000000000, 0x06df000000000000   ; fold_1x128b   (rk1/rk2)
        dq 0x044c000000000000, 0xe658000000000000   ; fold_8x128b   (rk3/rk4)
        dq 0x2d56000000000000, 0x1368000000000000   ; fold_128b_to_64b (rk5/rk6)
        dq 0x00000001f65a57f8, 0x000000018bb70000   ; reduce_64b_to_32b (rk7/rk8)

; ---------------------------------------------------------------------------
; CRC Masks and Shuffles
; ---------------------------------------------------------------------------
align 16
mk_global hi64_mask, data, hidden
hi64_mask: dq 0xFFFFFFFFFFFFFFFF, 0x0000000000000000
mk_global shf_xor_mask, data, hidden
shf_xor_mask: dq 0x8080808080808080, 0x8080808080808080
mk_global lo32_clr_mask, data, hidden
lo32_clr_mask: dq 0xFFFFFFFF00000000, 0xFFFFFFFFFFFFFFFF

mk_global hi32_clr_mask, data, hidden
hi32_clr_mask: dq 0xFFFFFFFFFFFFFFFF, 0x00000000FFFFFFFF
mk_global bswap_shuf_mask, data, hidden
bswap_shuf_mask: dq 0x08090A0B0C0D0E0F, 0x0001020304050607

align 16
mk_global shf_table_refl, data, hidden
shf_table_refl:
dq 0x8786858483828100, 0x8f8e8d8c8b8a8988
dq 0x0706050403020100, 0x000e0d0c0b0a0908

mk_global shift_table, data, hidden
shift_table:
db 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF
db 0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07
db 0x08, 0x09, 0x0A

mk_global shift_table_ieee, data, hidden
shift_table_ieee:
db 0x05, 0x06, 0x07, 0x08, 0x09, 0x0A, 0x0B
db 0x0C, 0x0D, 0x0E, 0x0F, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF
