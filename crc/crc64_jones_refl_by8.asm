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

%define FUNCTION_NAME crc64_jones_refl_by8
%define USE_CONSTS
%macro INCLUDE_CONSTS 0
rk1:
DQ 0x381d0015c96f4444
rk2:
DQ 0xd9d7be7d505da32c
rk3:
DQ 0x768361524d29ed0b
rk4:
DQ 0xcc26fa7c57f8054c
rk5:
DQ 0x381d0015c96f4444
rk6:
DQ 0x0000000000000000
rk7:
DQ 0x3e6cfa329aef9f77
rk8:
DQ 0x2b5926535897936a
rk9:
DQ 0x5bc94ba8e2087636
rk10:
DQ 0x6cf09c8f37710b75
rk11:
DQ 0x3885fd59e440d95a
rk12:
DQ 0xbccba3936411fb7e
rk13:
DQ 0xe4dd0d81cbfce585
rk14:
DQ 0xb715e37b96ed8633
rk15:
DQ 0xf49784a634f014e4
rk16:
DQ 0xaf86efb16d9ab4fb
rk17:
DQ 0x7b3211a760160db8
rk18:
DQ 0xa062b2319d66692f
rk19:
DQ 0xef3d1d18ed889ed2
rk20:
DQ 0x6ba4d760ab38201e
%endm

%include "crc64_iso_refl_by8.asm"
