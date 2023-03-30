;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;  Copyright(c) 2023 Intel Corporation All rights reserved.
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

%define FUNCTION_NAME crc64_rocksoft_refl_by16_10
%define USE_CONSTS
%macro INCLUDE_CONSTS 0
rk_1: dq 0xa043808c0f782663
rk_2: dq 0x37ccd3e14069cabc
rk1: dq 0x21e9761e252621ac
rk2: dq 0xeadc41fd2ba3d420
rk3: dq 0x5f852fb61e8d92dc
rk4: dq 0xa1ca681e733f9c40
rk5: dq 0x21e9761e252621ac
rk6: dq 0x0000000000000000
rk7: dq 0x27ecfa329aef9f77
rk8: dq 0x34d926535897936a
rk9: dq 0x946588403d4adcbc
rk10: dq 0xd083dd594d96319d
rk11: dq 0x34f5a24e22d66e90
rk12: dq 0x3c255f5ebc414423
rk13: dq 0x03363823e6e791e5
rk14: dq 0x7b0ab10dd0f809fe
rk15: dq 0x62242240ace5045a
rk16: dq 0x0c32cdb31e18a84a
rk17: dq 0xa3ffdc1fe8e82a8b
rk18: dq 0xbdd7ac0ee1a4a0f0
rk19: dq 0xe1e0bb9d45d7a44c
rk20: dq 0xb0bc2e589204f500
rk1_b: dq 0x21e9761e252621ac
rk2_b: dq 0xeadc41fd2ba3d420
	dq 0x0000000000000000
	dq 0x0000000000000000
%endm

%include "crc64_iso_refl_by16_10.asm"
