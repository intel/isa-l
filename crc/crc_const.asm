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
; CRC-64 ISO normal  (default iso polynomial)
; ---------------------------------------------------------------------------
align 16
mk_global crc64_iso_norm_const, data, hidden
crc64_iso_norm_const:
        dq 0x0000001a00000144, 0x0000015e00001dac   ; rk_1/rk_2
        dq 0x0000000000000145, 0x0000000000001db7   ; rk1/rk2
        dq 0x000100000001001a, 0x001b0000001b015e   ; rk3/rk4
        dq 0x0000000000000145, 0x0000000000000000   ; rk5/rk6
        dq 0x000000000000001b, 0x000000000000001b   ; rk7/rk8
        dq 0x0150145145145015, 0x1c71db6db6db71c7   ; rk9/rk10
        dq 0x0001110110110111, 0x001aab1ab1ab1aab   ; rk11/rk12
        dq 0x0000014445014445, 0x00001daab71daab7   ; rk13/rk14
        dq 0x0000000101000101, 0x0000001b1b001b1b   ; rk15/rk16
        dq 0x0000000001514515, 0x000000001c6db6c7   ; rk17/rk18
        dq 0x0000000000011011, 0x00000000001ab1ab   ; rk19/rk20
        dq 0x0000000000000145, 0x0000000000001db7   ; rk_1b/rk_2b (= rk1/rk2)
        dq 0x0000000000000000, 0x0000000000000000   ; padding (zmm alignment)

; ---------------------------------------------------------------------------
; CRC-64 ISO reflected  (default iso reflected polynomial)
; ---------------------------------------------------------------------------
align 16
mk_global crc64_iso_refl_const, data, hidden
crc64_iso_refl_const:
        dq 0x45000000b0000000, 0x6b700000f5000000   ; rk_1/rk_2
        dq 0xf500000000000001, 0x6b70000000000001   ; rk1/rk2
        dq 0xb001000000010000, 0xf501b0000001b000   ; rk3/rk4
        dq 0xf500000000000001, 0x0000000000000000   ; rk5/rk6
        dq 0xb000000000000001, 0xb000000000000000   ; rk7/rk8
        dq 0xe014514514501501, 0x771db6db6db71c71   ; rk9/rk10
        dq 0xa101101101110001, 0x1ab1ab1ab1aab001   ; rk11/rk12
        dq 0xf445014445000001, 0x6aab71daab700001   ; rk13/rk14
        dq 0xb100010100000001, 0x01b001b1b0000001   ; rk15/rk16
        dq 0xe145150000000001, 0x76db6c7000000001   ; rk17/rk18
        dq 0xa011000000000001, 0x1b1ab00000000001   ; rk19/rk20
        dq 0xf500000000000001, 0x6b70000000000001   ; rk_1b/rk_2b (= rk1/rk2)
        dq 0x0000000000000000, 0x0000000000000000   ; padding

; ---------------------------------------------------------------------------
; CRC-64 ECMA-182 normal
; ---------------------------------------------------------------------------
align 16
mk_global crc64_ecma_norm_const, data, hidden
crc64_ecma_norm_const:
        dq 0x7f52691a60ddc70d, 0x7036b0389f6a0c82   ; rk_1/rk_2
        dq 0x05f5c3c7eb52fab6, 0x4eb938a7d257740e   ; rk1/rk2
        dq 0x05cf79dea9ac37d6, 0x001067e571d7d5c2   ; rk3/rk4
        dq 0x05f5c3c7eb52fab6, 0x0000000000000000   ; rk5/rk6
        dq 0x578d29d06cc4f872, 0x42f0e1eba9ea3693   ; rk7/rk8
        dq 0xe464f4df5fb60ac1, 0xb649c5b35a759cf2   ; rk9/rk10
        dq 0x9af04e1eff82d0dd, 0x6e82e609297f8fe8   ; rk11/rk12
        dq 0x097c516e98bd2e73, 0x0b76477b31e22e7b   ; rk13/rk14
        dq 0x5f6843ca540df020, 0xddf4b6981205b83f   ; rk15/rk16
        dq 0x54819d8713758b2c, 0x4a6b90073eb0af5a   ; rk17/rk18
        dq 0x571bee0a227ef92b, 0x44bef2a201b5200c   ; rk19/rk20
        dq 0x05f5c3c7eb52fab6, 0x4eb938a7d257740e   ; rk_1b/rk_2b (= rk1/rk2)
        dq 0x0000000000000000, 0x0000000000000000   ; padding

; ---------------------------------------------------------------------------
; CRC-64 ECMA-182 reflected
; ---------------------------------------------------------------------------
align 16
mk_global crc64_ecma_refl_const, data, hidden
crc64_ecma_refl_const:
        dq 0xf31fd9271e228b79, 0x8260adf2381ad81c   ; rk_1/rk_2
        dq 0xdabe95afc7875f40, 0xe05dd497ca393ae4   ; rk1/rk2
        dq 0xd7d86b2af73de740, 0x8757d71d4fcc1000   ; rk3/rk4
        dq 0xdabe95afc7875f40, 0x0000000000000000   ; rk5/rk6
        dq 0x9c3e466c172963d5, 0x92d8af2baf0e1e84   ; rk7/rk8
        dq 0x947874de595052cb, 0x9e735cb59b4724da   ; rk9/rk10
        dq 0xe4ce2cd55fea0037, 0x2fe3fd2920ce82ec   ; rk11/rk12
        dq 0x0e31d519421a63a5, 0x2e30203212cac325   ; rk13/rk14
        dq 0x081f6054a7842df4, 0x6ae3efbb9dd441f3   ; rk15/rk16
        dq 0x69a35d91c3730254, 0xb5ea1af9c013aca4   ; rk17/rk18
        dq 0x3be653a30fe1af51, 0x60095b008a9efa44   ; rk19/rk20
        dq 0xdabe95afc7875f40, 0xe05dd497ca393ae4   ; rk_1b/rk_2b (= rk1/rk2)
        dq 0x0000000000000000, 0x0000000000000000   ; padding

; ---------------------------------------------------------------------------
; CRC-64 Jones normal
; ---------------------------------------------------------------------------
align 16
mk_global crc64_jones_norm_const, data, hidden
crc64_jones_norm_const:
        dq 0x44ff5212394b1c52, 0x956d6cb0582122b2   ; rk_1/rk_2
        dq 0x4445ed2750017038, 0x698b74157cfbd736   ; rk1/rk2
        dq 0x0cfcfb5101c4b775, 0x65403fd47cbec866   ; rk3/rk4
        dq 0x4445ed2750017038, 0x0000000000000000   ; rk5/rk6
        dq 0xddf3eeb298be6cf8, 0xad93d23594c935a9   ; rk7/rk8
        dq 0xd8dc208e2ba527b4, 0xf032cfec76bb2bc5   ; rk9/rk10
        dq 0xb536044f357f4238, 0xfdbf104d938ba67a   ; rk11/rk12
        dq 0xeeddad9297a843e7, 0x3550bce629466473   ; rk13/rk14
        dq 0x4e501e58ca43d25e, 0x13c961588f27f643   ; rk15/rk16
        dq 0x3b60d00dcb1099bc, 0x44bf1f468c53b9a3   ; rk17/rk18
        dq 0x96f2236e317179ee, 0xf00839aa0dd64bac   ; rk19/rk20
        dq 0x4445ed2750017038, 0x698b74157cfbd736   ; rk_1b/rk_2b (= rk1/rk2)
        dq 0x0000000000000000, 0x0000000000000000   ; padding

; ---------------------------------------------------------------------------
; CRC-64 Jones reflected
; ---------------------------------------------------------------------------
align 16
mk_global crc64_jones_refl_const, data, hidden
crc64_jones_refl_const:
        dq 0x9471a5389095fe44, 0x9a8908341a6d6d52   ; rk_1/rk_2
        dq 0x381d0015c96f4444, 0xd9d7be7d505da32c   ; rk1/rk2
        dq 0x768361524d29ed0b, 0xcc26fa7c57f8054c   ; rk3/rk4
        dq 0x381d0015c96f4444, 0x0000000000000000   ; rk5/rk6
        dq 0x3e6cfa329aef9f77, 0x2b5926535897936a   ; rk7/rk8
        dq 0x5bc94ba8e2087636, 0x6cf09c8f37710b75   ; rk9/rk10
        dq 0x3885fd59e440d95a, 0xbccba3936411fb7e   ; rk11/rk12
        dq 0xe4dd0d81cbfce585, 0xb715e37b96ed8633   ; rk13/rk14
        dq 0xf49784a634f014e4, 0xaf86efb16d9ab4fb   ; rk15/rk16
        dq 0x7b3211a760160db8, 0xa062b2319d66692f   ; rk17/rk18
        dq 0xef3d1d18ed889ed2, 0x6ba4d760ab38201e   ; rk19/rk20
        dq 0x381d0015c96f4444, 0xd9d7be7d505da32c   ; rk_1b/rk_2b (= rk1/rk2)
        dq 0x0000000000000000, 0x0000000000000000   ; padding

; ---------------------------------------------------------------------------
; CRC-64 Rocksoft normal
; ---------------------------------------------------------------------------
align 16
mk_global crc64_rocksoft_norm_const, data, hidden
crc64_rocksoft_norm_const:
        dq 0x215befd5f6cab253, 0x7aa72c050f9667d8   ; rk_1/rk_2
        dq 0x6b08c948f0dd2f08, 0x08578ba97f0476ae   ; rk1/rk2
        dq 0x769362f0dbe943f4, 0x0473f99cf02ca70a   ; rk3/rk4
        dq 0x6b08c948f0dd2f08, 0x0000000000000000   ; rk5/rk6
        dq 0xddf3eeb298be6fc8, 0xad93d23594c93659   ; rk7/rk8
        dq 0x7a76a57804234c52, 0xde8b0150a1beb44f   ; rk9/rk10
        dq 0x12ecd688e48b5e58, 0x25d6d64f613c7e21   ; rk11/rk12
        dq 0xe2801cfa1cf1efd9, 0xff203e17611aa1bc   ; rk13/rk14
        dq 0xb4414e6a0488488c, 0xa42a30f19b669860   ; rk15/rk16
        dq 0x0f3bfc1a64bec9d3, 0x1e0a4b0ee06bd77a   ; rk17/rk18
        dq 0x644bd74573ba0f0e, 0x015e409234e87a1a   ; rk19/rk20
        dq 0x6b08c948f0dd2f08, 0x08578ba97f0476ae   ; rk_1b/rk_2b (= rk1/rk2)
        dq 0x0000000000000000, 0x0000000000000000   ; padding

; ---------------------------------------------------------------------------
; CRC-64 Rocksoft reflected
; ---------------------------------------------------------------------------
align 16
mk_global crc64_rocksoft_refl_const, data, hidden
crc64_rocksoft_refl_const:
        dq 0xa043808c0f782663, 0x37ccd3e14069cabc   ; rk_1/rk_2
        dq 0x21e9761e252621ac, 0xeadc41fd2ba3d420   ; rk1/rk2
        dq 0x5f852fb61e8d92dc, 0xa1ca681e733f9c40   ; rk3/rk4
        dq 0x21e9761e252621ac, 0x0000000000000000   ; rk5/rk6
        dq 0x27ecfa329aef9f77, 0x34d926535897936a   ; rk7/rk8
        dq 0x946588403d4adcbc, 0xd083dd594d96319d   ; rk9/rk10
        dq 0x34f5a24e22d66e90, 0x3c255f5ebc414423   ; rk11/rk12
        dq 0x03363823e6e791e5, 0x7b0ab10dd0f809fe   ; rk13/rk14
        dq 0x62242240ace5045a, 0x0c32cdb31e18a84a   ; rk15/rk16
        dq 0xa3ffdc1fe8e82a8b, 0xbdd7ac0ee1a4a0f0   ; rk17/rk18
        dq 0xe1e0bb9d45d7a44c, 0xb0bc2e589204f500   ; rk19/rk20
        dq 0x21e9761e252621ac, 0xeadc41fd2ba3d420   ; rk_1b/rk_2b (= rk1/rk2)
        dq 0x0000000000000000, 0x0000000000000000   ; padding

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
mk_global lo64_mask, data, hidden
lo64_mask: dq 0x0000000000000000, 0xFFFFFFFFFFFFFFFF
mk_global zero128, data, hidden
zero128: dq 0x0000000000000000, 0x0000000000000000

align 16
mk_global shf_table_refl, data, hidden
shf_table_refl:
dq 0x8786858483828100, 0x8f8e8d8c8b8a8988
dq 0x0706050403020100, 0x000e0d0c0b0a0908

align 16
mk_global tail_shuf_refl, data, hidden
tail_shuf_refl:
db 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF
db 0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07
db 0x08, 0x09, 0x0A

align 16
mk_global tail_shuf_norm, data, hidden
tail_shuf_norm:
db 0x05, 0x06, 0x07, 0x08, 0x09, 0x0A, 0x0B
db 0x0C, 0x0D, 0x0E, 0x0F, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF

align 16
mk_global shf_table_norm, data, hidden
shf_table_norm:
dq 0x8786858483828100, 0x8f8e8d8c8b8a8988
dq 0x0706050403020100, 0x0f0e0d0c0b0a0908
dq 0x8080808080808080, 0x0f0e0d0c0b0a0908
dq 0x8080808080808080, 0x8080808080808080

align 16
mk_global byte_len_mask_table, data, hidden
byte_len_mask_table:
dw      0x0000, 0x0001, 0x0003, 0x0007,
dw      0x000f, 0x001f, 0x003f, 0x007f,
dw      0x00ff, 0x01ff, 0x03ff, 0x07ff,
dw      0x0fff, 0x1fff, 0x3fff, 0x7fff,
