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
%macro write_bits_always 6
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

; Assumes m_out_buf is a register
; Clobbers RCX
; code is clobbered
; write_bits_safe	m_bits, m_bit_count, code, count, m_out_buf, tmp1
%macro write_bits_safe 6
%define %%m_bits	%1
%define %%m_bit_count	%2
%define %%code		%3
%define %%count		%4
%define %%m_out_buf	%5
%define %%tmp1		%6

	mov	%%tmp1, %%code
%ifdef USE_HSWNI
	shlx	%%tmp1, %%tmp1, %%m_bit_count
%else
	mov	rcx, %%m_bit_count
	shl	%%tmp1, cl
%endif
	or	%%m_bits, %%tmp1
	add	%%m_bit_count, %%count
	cmp	%%m_bit_count, 64
	jb	%%not_full
	sub	%%m_bit_count, 64
	movnti	[%%m_out_buf], %%m_bits
	add	%%m_out_buf, 8
	mov	rcx, %%count
	sub	rcx, %%m_bit_count
	mov	%%m_bits, %%code
%ifdef USE_HSWNI
	shrx	%%m_bits, %%m_bits, rcx
%else
	shr	%%m_bits, cl
%endif
%%not_full:
%endm

; Assumes m_out_buf is a register
; Clobbers RCX
;; check_space	num_bits, m_bits, m_bit_count, m_out_buf, tmp1
%macro check_space 5
%define %%num_bits	%1
%define %%m_bits	%2
%define %%m_bit_count	%3
%define %%m_out_buf	%4
%define %%tmp1		%5

	mov	%%tmp1, 63
	sub	%%tmp1, %%m_bit_count
	cmp	%%tmp1, %%num_bits
	jae	%%space_ok

	; if (63 - m_bit_count < num_bits)
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
%%space_ok:
%endm

; rcx is clobbered
; code is clobbered
; write_bits_unsafe	m_bits, m_bit_count, code, count
%macro write_bits_unsafe 4
%define %%m_bits		%1
%define %%m_bit_count	%2
%define %%code		%3
%define %%count		%4
%ifdef USE_HSWNI
	shlx	%%code, %%code, %%m_bit_count
%else
	mov	rcx, %%m_bit_count
	shl	%%code, cl
%endif
	or	%%m_bits, %%code
	add	%%m_bit_count, %%count
%endm

; pad_to_byte m_bit_count, extra_bits
%macro pad_to_byte 2
%define %%m_bit_count	%1
%define %%extra_bits	%2

	mov	%%extra_bits, %%m_bit_count
	neg	%%extra_bits
	and	%%extra_bits, 7
	add	%%m_bit_count, %%extra_bits
%endm

; Assumes m_out_buf is a memory reference
; flush	m_bits, m_bit_count, m_out_buf, tmp1
%macro flush 4
%define %%m_bits	%1
%define %%m_bit_count	%2
%define %%m_out_buf	%3
%define %%tmp1		%4

	test	%%m_bit_count, %%m_bit_count
	jz	%%bit_count_is_zero

	mov	%%tmp1, %%m_out_buf
	movnti	[%%tmp1], %%m_bits

	add	%%m_bit_count, 7
	shr	%%m_bit_count, 3	; bytes
	add	%%tmp1, %%m_bit_count
	mov	%%m_out_buf, %%tmp1

%%bit_count_is_zero:
	xor	%%m_bits, %%m_bits
	xor	%%m_bit_count, %%m_bit_count
%endm

%macro write_bits 6
%define %%m_bits	%1
%define %%m_bit_count	%2
%define %%code		%3
%define %%count		%4
%define %%m_out_buf	%5
%define %%tmp1		%6

%ifdef USE_BITBUF8
	write_bits_safe	%%m_bits, %%m_bit_count, %%code, %%count, %%m_out_buf, %%tmp1
%elifdef USE_BITBUFB
	write_bits_always %%m_bits, %%m_bit_count, %%code, %%count, %%m_out_buf, %%tmp1
%else
	; state->bitbuf.check_space(code_len2);
	check_space	%%count, %%m_bits, %%m_bit_count, %%m_out_buf, %%tmp1
	; state->bitbuf.write_bits(code2, code_len2);
	write_bits_unsafe	%%m_bits, %%m_bit_count, %%code, %%count
	; code2 is clobbered, rcx is clobbered
%endif
%endm

%macro write_dword 2
%define %%data %1d
%define %%addr %2
	movnti	[%%addr], %%data
	add	%%addr, 4
%endm
