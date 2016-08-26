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

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;;
;;; uint64_t crc64_func(uint64_t init_crc, const unsigned char *buf, uint64_t len);
;;;

default rel
[bits 64]

%ifidn __OUTPUT_FORMAT__, elf64
%define WRT_OPT         wrt ..plt
%else
%define WRT_OPT
%endif

%include "reg_sizes.asm"

extern crc64_ecma_refl_by8
extern crc64_ecma_refl_base

extern crc64_ecma_norm_by8
extern crc64_ecma_norm_base

section .data
;;; *_mbinit are initial values for *_dispatched; is updated on first call.
;;; Therefore, *_dispatch_init is only executed on first call.

crc64_ecma_refl_dispatched:
	dq	crc64_ecma_refl_mbinit
crc64_ecma_norm_dispatched:
        dq      crc64_ecma_norm_mbinit

section .text

;;;;
; crc64_ecma_refl multibinary function
;;;;
global crc64_ecma_refl:function
crc64_ecma_refl_mbinit:
	call	crc64_ecma_refl_dispatch_init
crc64_ecma_refl:
	jmp	qword [crc64_ecma_refl_dispatched]

crc64_ecma_refl_dispatch_init:
	push    rax
	push    rbx
	push    rcx
	push    rdx
	push    rsi
	lea     rsi, [crc64_ecma_refl_base WRT_OPT] ; Default

	mov     eax, 1
	cpuid
	lea     rbx, [crc64_ecma_refl_by8 WRT_OPT]

	test	ecx, FLAG_CPUID1_ECX_SSE3
	jz	use_ecma_refl_base
	test    ecx, FLAG_CPUID1_ECX_CLMUL
	cmovne  rsi, rbx
use_ecma_refl_base:
	mov     [crc64_ecma_refl_dispatched], rsi
	pop     rsi
	pop     rdx
	pop     rcx
	pop     rbx
	pop     rax
	ret

;;;;
; crc64_ecma_norm multibinary function
;;;;
global crc64_ecma_norm:function
crc64_ecma_norm_mbinit:
        call    crc64_ecma_norm_dispatch_init
crc64_ecma_norm:
        jmp     qword [crc64_ecma_norm_dispatched]

crc64_ecma_norm_dispatch_init:
        push    rax
        push    rbx
        push    rcx
        push    rdx
        push    rsi
        lea     rsi, [crc64_ecma_norm_base WRT_OPT] ; Default

        mov     eax, 1
        cpuid
        lea     rbx, [crc64_ecma_norm_by8 WRT_OPT]

        test    ecx, FLAG_CPUID1_ECX_SSE3
        jz      use_ecma_norm_base
        test    ecx, FLAG_CPUID1_ECX_CLMUL
        cmovne  rsi, rbx
use_ecma_norm_base:
        mov     [crc64_ecma_norm_dispatched], rsi
        pop     rsi
        pop     rdx
        pop     rcx
        pop     rbx
        pop     rax
        ret

;;;       func            	core, ver, snum
slversion crc64_ecma_refl,	00,   00,  0018
slversion crc64_ecma_norm,	00,   00,  001e
