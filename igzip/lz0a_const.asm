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

%include "options.asm"

%assign K      1024
%assign D      IGZIP_HIST_SIZE        ;; Amount of history
%assign LA     18 * 16      ;; Max look-ahead, rounded up to 32 byte boundary
%assign BSIZE  2*IGZIP_HIST_SIZE + LA     ;; Nominal buffer size

;; Constants for stateless compression
%define LAST_BYTES_COUNT	3 ;; Bytes to prevent reading out of array bounds
%define LA_STATELESS	258	;; No round up since no data is copied to a buffer

%ifndef IGZIP_HASH_SIZE
%assign IGZIP_HASH_SIZE  (8 * K)
%endif

%assign HASH_MASK  (IGZIP_HASH_SIZE - 1)

%assign SHORTEST_MATCH  4

%assign SLOP 8

%define LIT_LEN_BIT_COUNT 10
%define DIST_LIT_BIT_COUNT 9

%define DIST_OFFSET	LIT_LEN_BIT_COUNT
%define EXTRA_BITS_OFFSET	(DIST_OFFSET + DIST_LIT_BIT_COUNT)
%define LIT (0x1E << DIST_OFFSET)
