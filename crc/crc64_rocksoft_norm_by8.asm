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

%define FUNCTION_NAME crc64_rocksoft_norm_by8
%define USE_CONSTS
%macro INCLUDE_CONSTS 0
rk1:
DQ 0x6b08c948f0dd2f08
rk2:
DQ 0x08578ba97f0476ae
rk3:
DQ 0x769362f0dbe943f4
rk4:
DQ 0x0473f99cf02ca70a
rk5:
DQ 0x6b08c948f0dd2f08
rk6:
DQ 0x0000000000000000
rk7:
DQ 0xddf3eeb298be6fc8
rk8:
DQ 0xad93d23594c93659
rk9:
DQ 0x7a76a57804234c52
rk10:
DQ 0xde8b0150a1beb44f
rk11:
DQ 0x12ecd688e48b5e58
rk12:
DQ 0x25d6d64f613c7e21
rk13:
DQ 0xe2801cfa1cf1efd9
rk14:
DQ 0xff203e17611aa1bc
rk15:
DQ 0xb4414e6a0488488c
rk16:
DQ 0xa42a30f19b669860
rk17:
DQ 0x0f3bfc1a64bec9d3
rk18:
DQ 0x1e0a4b0ee06bd77a
rk19:
DQ 0x644bd74573ba0f0e
rk20:
DQ 0x015e409234e87a1a
%endm

%include "crc64_iso_norm_by8.asm"
