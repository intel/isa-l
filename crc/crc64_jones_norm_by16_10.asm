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

%define FUNCTION_NAME crc64_jones_norm_by16_10
%define USE_CONSTS
%macro INCLUDE_CONSTS 0
rk_1: dq 0x44ff5212394b1c52
rk_2: dq 0x956d6cb0582122b2
rk1: dq 0x4445ed2750017038
rk2: dq 0x698b74157cfbd736
rk3: dq 0x0cfcfb5101c4b775
rk4: dq 0x65403fd47cbec866
rk5: dq 0x4445ed2750017038
rk6: dq 0x0000000000000000
rk7: dq 0xddf3eeb298be6cf8
rk8: dq 0xad93d23594c935a9
rk9: dq 0xd8dc208e2ba527b4
rk10: dq 0xf032cfec76bb2bc5
rk11: dq 0xb536044f357f4238
rk12: dq 0xfdbf104d938ba67a
rk13: dq 0xeeddad9297a843e7
rk14: dq 0x3550bce629466473
rk15: dq 0x4e501e58ca43d25e
rk16: dq 0x13c961588f27f643
rk17: dq 0x3b60d00dcb1099bc
rk18: dq 0x44bf1f468c53b9a3
rk19: dq 0x96f2236e317179ee
rk20: dq 0xf00839aa0dd64bac
rk_1b: dq 0x4445ed2750017038
rk_2b: dq 0x698b74157cfbd736
	dq 0x0000000000000000
	dq 0x0000000000000000
%endm

%include "crc64_iso_norm_by16_10.asm"
