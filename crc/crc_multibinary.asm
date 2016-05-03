;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;  Copyright(c) 2011-2015 Intel Corporation All rights reserved.
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
[bits 64]

%ifidn __OUTPUT_FORMAT__, elf64
%define WRT_OPT         wrt ..plt
%else
%define WRT_OPT
%endif

%include "reg_sizes.asm"

extern crc32_iscsi_00
extern crc32_iscsi_01
extern crc32_iscsi_base

extern crc32_ieee_01
extern crc32_ieee_by4  ;; Optimized for SLM
extern crc32_ieee_base

extern crc16_t10dif_01
extern crc16_t10dif_by4  ;; Optimized for SLM
extern crc16_t10dif_base

section .data
;;; *_mbinit are initial values for *_dispatched; is updated on first call.
;;; Therefore, *_dispatch_init is only executed on first call.

crc32_iscsi_dispatched:
	dq	crc32_iscsi_mbinit

crc32_ieee_dispatched:
	dq	crc32_ieee_mbinit

crc16_t10dif_dispatched:
	dq	crc16_t10dif_mbinit

section .text
;;;;
; crc32_iscsi multibinary function
;;;;
global crc32_iscsi:function
crc32_iscsi_mbinit:
	call	crc32_iscsi_dispatch_init
crc32_iscsi:
	jmp	qword [crc32_iscsi_dispatched]

crc32_iscsi_dispatch_init:
	push 	rax
	push	rbx
	push	rcx
	push	rdx
	push	rsi
	lea     rsi, [crc32_iscsi_base WRT_OPT] ; Default

	mov	eax, 1
	cpuid
	lea     rbx, [crc32_iscsi_00 WRT_OPT]
	lea     rax, [crc32_iscsi_01 WRT_OPT]

	test	ecx, FLAG_CPUID1_ECX_SSE4_2
	cmovne	rsi, rbx
	test	ecx, FLAG_CPUID1_ECX_CLMUL
	cmovne	rsi, rax
	mov	[crc32_iscsi_dispatched], rsi
	pop	rsi
	pop	rdx
	pop	rcx
	pop	rbx
	pop	rax
	ret

;;;;
; crc32_ieee multibinary function
;;;;
global crc32_ieee:function
crc32_ieee_mbinit:
	call	crc32_ieee_dispatch_init
crc32_ieee:
	jmp	qword [crc32_ieee_dispatched]

crc32_ieee_dispatch_init:
	push    rax
	push    rbx
	push    rcx
	push    rdx
	push    rsi
	lea     rsi, [crc32_ieee_base WRT_OPT] ; Default

	mov     eax, 1
	cpuid
	lea     rbx, [crc32_ieee_01 WRT_OPT]
	lea     rdx, [crc32_ieee_by4 WRT_OPT]

	test	ecx, FLAG_CPUID1_ECX_SSE3
	jz	use_ieee_base
	test    ecx, FLAG_CPUID1_ECX_CLMUL
	cmovne  rsi, rbx
	and     eax, FLAG_CPUID1_EAX_STEP_MASK
	cmp     eax, FLAG_CPUID1_EAX_AVOTON
	cmove   rsi, rdx
use_ieee_base:
	mov     [crc32_ieee_dispatched], rsi
	pop     rsi
	pop     rdx
	pop     rcx
	pop     rbx
	pop     rax
	ret

;;;;
; crc16_t10dif multibinary function
;;;;
global crc16_t10dif:function
crc16_t10dif_mbinit:
	call	crc16_t10dif_dispatch_init
crc16_t10dif:
	jmp	qword [crc16_t10dif_dispatched]

crc16_t10dif_dispatch_init:
	push    rax
	push    rbx
	push    rcx
	push    rdx
	push    rsi
	lea     rsi, [crc16_t10dif_base WRT_OPT] ; Default

	mov     eax, 1
	cpuid
	lea     rbx, [crc16_t10dif_01 WRT_OPT]
	lea     rdx, [crc16_t10dif_by4 WRT_OPT]

	test    ecx, FLAG_CPUID1_ECX_SSE3
	jz      use_t10dif_base
	test    ecx, FLAG_CPUID1_ECX_CLMUL
	cmovne  rsi, rbx
	and     eax, FLAG_CPUID1_EAX_STEP_MASK
	cmp     eax, FLAG_CPUID1_EAX_AVOTON
	cmove   rsi, rdx
use_t10dif_base:
	mov     [crc16_t10dif_dispatched], rsi
	pop     rsi
	pop     rdx
	pop     rcx
	pop     rbx
	pop     rax
	ret

;;;       func            	core, ver, snum
slversion crc16_t10dif,		00,   03,  011a
slversion crc32_ieee,		00,   03,  011b
slversion crc32_iscsi,		00,   03,  011c
