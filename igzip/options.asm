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

%ifndef __OPTIONS_ASM__
%define __OPTIONS_ASM__

; Options:dir
; m - reschedule mem reads
; e b - bitbuff style
; t s x - compare style
; h - limit hash updates
; l - use longer huffman table
; f - fix cache read

%ifndef IGZIP_HIST_SIZE
%define IGZIP_HIST_SIZE (32 * 1024)
%endif

%if (IGZIP_HIST_SIZE > (32 * 1024))
%undef IGZIP_HIST_SIZE
%define IGZIP_HIST_SIZE (32 * 1024)
%endif

%ifdef LONGER_HUFFTABLE
%if (IGZIP_HIST_SIZE > 8 * 1024)
%undef IGZIP_HIST_SIZE
%define IGZIP_HIST_SIZE (8 * 1024)
%endif
%endif

; (h) limit hash update
%define LIMIT_HASH_UPDATE

; (f) fix cache read problem
%define FIX_CACHE_READ

%define ISAL_DEF_MAX_HDR_SIZE 328

%ifidn __OUTPUT_FORMAT__, elf64
%ifndef __NASM_VER__
%define WRT_OPT         wrt ..sym
%else
%define WRT_OPT
%endif
%else
%define WRT_OPT
%endif

%endif ; ifndef __OPTIONS_ASM__
