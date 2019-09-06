;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;  Copyright(c) 2011-2019 Intel Corporation All rights reserved.
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

%define FUNCTION_NAME crc64_ecma_refl_by16_10
%define USE_CONSTS
%macro INCLUDE_CONSTS 0
rk_1: dq 0xf31fd9271e228b79
rk_2: dq 0x8260adf2381ad81c
rk1: dq 0xdabe95afc7875f40
rk2: dq 0xe05dd497ca393ae4
rk3: dq 0xd7d86b2af73de740
rk4: dq 0x8757d71d4fcc1000
rk5: dq 0xdabe95afc7875f40
rk6: dq 0x0000000000000000
rk7: dq 0x9c3e466c172963d5
rk8: dq 0x92d8af2baf0e1e84
rk9: dq 0x947874de595052cb
rk10: dq 0x9e735cb59b4724da
rk11: dq 0xe4ce2cd55fea0037
rk12: dq 0x2fe3fd2920ce82ec
rk13: dq 0x0e31d519421a63a5
rk14: dq 0x2e30203212cac325
rk15: dq 0x081f6054a7842df4
rk16: dq 0x6ae3efbb9dd441f3
rk17: dq 0x69a35d91c3730254
rk18: dq 0xb5ea1af9c013aca4
rk19: dq 0x3be653a30fe1af51
rk20: dq 0x60095b008a9efa44
rk_1b: dq 0xdabe95afc7875f40
rk_2b: dq 0xe05dd497ca393ae4
	dq 0x0000000000000000
	dq 0x0000000000000000
%endm

%include "crc64_iso_refl_by16_10.asm"
