;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;  Copyright(c) 2011-2015 Intel Corporation All rights reserved.
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

default rel
[bits 64]

%include "reg_sizes.asm"
%include "multibinary.asm"

extern crc32_iscsi_01
extern crc32_iscsi_by8_02
extern crc32_iscsi_avx2
extern crc32_iscsi_base

extern crc32_ieee_01
extern crc32_ieee_02
extern crc32_ieee_base
extern crc32_ieee_avx2

extern crc16_t10dif_01
extern crc16_t10dif_02
extern crc16_t10dif_base

extern crc32_gzip_refl_by8
extern crc32_gzip_refl_by8_02
extern crc32_gzip_refl_base
extern crc32_gzip_refl_avx2

extern crc16_t10dif_copy_by4
extern crc16_t10dif_copy_by4_02
extern crc16_t10dif_copy_base

extern crc32_gzip_refl_by16_10
extern crc32_ieee_by16_10
extern crc32_iscsi_by16_10
extern crc16_t10dif_by16_10

mbin_interface			crc32_iscsi
mbin_dispatch_init_clmul	crc32_iscsi, crc32_iscsi_base, crc32_iscsi_01, crc32_iscsi_by8_02, crc32_iscsi_avx2, crc32_iscsi_by16_10

mbin_interface			crc32_gzip_refl
mbin_dispatch_init_clmul	crc32_gzip_refl, crc32_gzip_refl_base, crc32_gzip_refl_by8, crc32_gzip_refl_by8_02, crc32_gzip_refl_avx2, crc32_gzip_refl_by16_10

mbin_interface			crc16_t10dif_copy
mbin_dispatch_init_clmul	crc16_t10dif_copy, crc16_t10dif_copy_base, crc16_t10dif_copy_by4, crc16_t10dif_copy_by4_02, crc16_t10dif_copy_by4_02, crc16_t10dif_copy_by4_02

mbin_interface			crc32_ieee
mbin_dispatch_init_clmul	crc32_ieee, crc32_ieee_base, crc32_ieee_01, crc32_ieee_02, crc32_ieee_avx2, crc32_ieee_by16_10
mbin_interface			crc16_t10dif
mbin_dispatch_init_clmul	crc16_t10dif, crc16_t10dif_base, crc16_t10dif_01, crc16_t10dif_02, crc16_t10dif_02, crc16_t10dif_by16_10
