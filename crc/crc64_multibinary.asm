;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;  Copyright(c) 2011-2016 Intel Corporation All rights reserved.
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
;;;
;;; uint64_t crc64_func(uint64_t init_crc, const unsigned char *buf, uint64_t len);
;;;

default rel
[bits 64]

%include "reg_sizes.asm"

extern crc64_ecma_refl_by8
extern crc64_ecma_refl_base

extern crc64_ecma_norm_by8
extern crc64_ecma_norm_base

extern crc64_iso_refl_by8
extern crc64_iso_refl_base

extern crc64_iso_norm_by8
extern crc64_iso_norm_base

extern crc64_jones_refl_by8
extern crc64_jones_refl_base

extern crc64_jones_norm_by8
extern crc64_jones_norm_base

section .text

%include "multibinary.asm"

mbin_interface			crc64_ecma_refl
mbin_dispatch_init_clmul	crc64_ecma_refl, crc64_ecma_refl_base, crc64_ecma_refl_by8
mbin_interface			crc64_ecma_norm
mbin_dispatch_init_clmul	crc64_ecma_norm, crc64_ecma_norm_base, crc64_ecma_norm_by8

mbin_interface			crc64_iso_refl
mbin_dispatch_init_clmul	crc64_iso_refl, crc64_iso_refl_base, crc64_iso_refl_by8
mbin_interface			crc64_iso_norm
mbin_dispatch_init_clmul	crc64_iso_norm, crc64_iso_norm_base, crc64_iso_norm_by8

mbin_interface			crc64_jones_refl
mbin_dispatch_init_clmul	crc64_jones_refl, crc64_jones_refl_base, crc64_jones_refl_by8
mbin_interface			crc64_jones_norm
mbin_dispatch_init_clmul	crc64_jones_norm, crc64_jones_norm_base, crc64_jones_norm_by8

;;;       func            	core, ver, snum
slversion crc64_ecma_refl,	00,   00,  001b
slversion crc64_ecma_norm,	00,   00,  0018
slversion crc64_iso_refl,	00,   00,  0021
slversion crc64_iso_norm,	00,   00,  001e
slversion crc64_jones_refl,	00,   00,  0027
slversion crc64_jones_norm,	00,   00,  0024
