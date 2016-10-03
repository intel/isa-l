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

default rel
[bits 64]

%ifidn __OUTPUT_FORMAT__, elf64
%define WRT_OPT         wrt ..plt
%else
%define WRT_OPT
%endif

%include "reg_sizes.asm"

extern isal_deflate_body_base
extern isal_deflate_body_01
extern isal_deflate_body_02
extern isal_deflate_body_04
extern isal_deflate_finish_base
extern isal_deflate_finish_01


extern isal_deflate_icf_body_base
extern isal_deflate_icf_body_01
extern isal_deflate_icf_body_02
extern isal_deflate_icf_body_04
extern isal_deflate_icf_finish_base
extern isal_deflate_icf_finish_01

extern isal_update_histogram_base
extern isal_update_histogram_01
extern isal_update_histogram_04

extern encode_deflate_icf_base
extern encode_deflate_icf_04

extern crc32_gzip_base
extern crc32_gzip_01

section .text

%include "multibinary.asm"

mbin_interface		isal_deflate_body
mbin_dispatch_init5	isal_deflate_body, isal_deflate_body_base, isal_deflate_body_01, isal_deflate_body_02, isal_deflate_body_04
mbin_interface		isal_deflate_finish
mbin_dispatch_init5	isal_deflate_finish, isal_deflate_finish_base, isal_deflate_finish_01, isal_deflate_finish_01, isal_deflate_finish_01

mbin_interface		isal_deflate_icf_body
mbin_dispatch_init5	isal_deflate_icf_body, isal_deflate_icf_body_base, isal_deflate_icf_body_01, isal_deflate_icf_body_02, isal_deflate_icf_body_04
mbin_interface		isal_deflate_icf_finish
mbin_dispatch_init5	isal_deflate_icf_finish, isal_deflate_icf_finish_base, isal_deflate_icf_finish_01, isal_deflate_icf_finish_01, isal_deflate_icf_finish_01

mbin_interface		isal_update_histogram
mbin_dispatch_init5	isal_update_histogram, isal_update_histogram_base, isal_update_histogram_01, isal_update_histogram_01, isal_update_histogram_04

mbin_interface		encode_deflate_icf
mbin_dispatch_init5	encode_deflate_icf, encode_deflate_icf_base, encode_deflate_icf_base, encode_deflate_icf_base, encode_deflate_icf_04

mbin_interface		crc32_gzip
mbin_dispatch_init5	crc32_gzip, crc32_gzip_base, crc32_gzip_base, crc32_gzip_01, crc32_gzip_01
