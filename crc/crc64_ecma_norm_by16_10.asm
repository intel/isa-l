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

%define FUNCTION_NAME crc64_ecma_norm_by16_10
%define USE_CONSTS
%macro INCLUDE_CONSTS 0
rk_1: dq 0x7f52691a60ddc70d
rk_2: dq 0x7036b0389f6a0c82
rk1: dq 0x05f5c3c7eb52fab6
rk2: dq 0x4eb938a7d257740e
rk3: dq 0x05cf79dea9ac37d6
rk4: dq 0x001067e571d7d5c2
rk5: dq 0x05f5c3c7eb52fab6
rk6: dq 0x0000000000000000
rk7: dq 0x578d29d06cc4f872
rk8: dq 0x42f0e1eba9ea3693
rk9: dq 0xe464f4df5fb60ac1
rk10: dq 0xb649c5b35a759cf2
rk11: dq 0x9af04e1eff82d0dd
rk12: dq 0x6e82e609297f8fe8
rk13: dq 0x097c516e98bd2e73
rk14: dq 0x0b76477b31e22e7b
rk15: dq 0x5f6843ca540df020
rk16: dq 0xddf4b6981205b83f
rk17: dq 0x54819d8713758b2c
rk18: dq 0x4a6b90073eb0af5a
rk19: dq 0x571bee0a227ef92b
rk20: dq 0x44bef2a201b5200c
rk_1b: dq 0x05f5c3c7eb52fab6
rk_2b: dq 0x4eb938a7d257740e
	dq 0x0000000000000000
	dq 0x0000000000000000
%endm

%include "crc64_iso_norm_by16_10.asm"
