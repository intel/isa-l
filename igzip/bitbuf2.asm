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

; Assumes m_out_buf is a register
; Clobbers RCX
; code is clobbered
; write_bits_always	m_bits, m_bit_count, code, count, m_out_buf, tmp1
%macro write_bits 6
%define %%m_bits	%1
%define %%m_bit_count	%2
%define %%code		%3
%define %%count		%4
%define %%m_out_buf	%5
%define %%tmp1		%6

%ifdef USE_HSWNI
	shlx	%%code, %%code, %%m_bit_count
%else
	mov	rcx, %%m_bit_count
	shl	%%code, cl
%endif
	or	%%m_bits, %%code
	add	%%m_bit_count, %%count

	movnti	[%%m_out_buf], %%m_bits
	mov	rcx, %%m_bit_count
	shr	rcx, 3			; rcx = bytes
	add	%%m_out_buf, rcx
	shl	rcx, 3			; rcx = bits
	sub	%%m_bit_count, rcx
%ifdef USE_HSWNI
	shrx	%%m_bits, %%m_bits, rcx
%else
	shr	%%m_bits, cl
%endif
%endm

%macro write_dword 2
%define %%data %1d
%define %%addr %2
	movnti	[%%addr], %%data
	add	%%addr, 4
%endm
