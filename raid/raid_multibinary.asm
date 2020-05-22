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

%include "reg_sizes.asm"
%include "multibinary.asm"

default rel
[bits 64]

extern pq_gen_base
extern pq_gen_sse
extern pq_gen_avx
extern pq_gen_avx2

extern xor_gen_base
extern xor_gen_sse
extern xor_gen_avx

extern pq_check_base
extern pq_check_sse

extern xor_check_base
extern xor_check_sse

%ifdef HAVE_AS_KNOWS_AVX512
 extern xor_gen_avx512
 extern pq_gen_avx512
%endif

mbin_interface xor_gen
mbin_interface pq_gen


mbin_dispatch_init6 xor_gen, xor_gen_base, xor_gen_sse, xor_gen_avx, xor_gen_avx, xor_gen_avx512
mbin_dispatch_init6 pq_gen, pq_gen_base, pq_gen_sse, pq_gen_avx, pq_gen_avx2, pq_gen_avx512

section .data

xor_check_dispatched:
	dq      xor_check_mbinit
pq_check_dispatched:
	dq      pq_check_mbinit

section .text

;;;;
; pq_check multibinary function
;;;;
mk_global  pq_check, function
pq_check_mbinit:
	endbranch
	call	pq_check_dispatch_init
pq_check:
	endbranch
	jmp     qword [pq_check_dispatched]

pq_check_dispatch_init:
	push    rax
	push    rbx
	push    rcx
	push    rdx
	push    rsi
	lea     rsi, [pq_check_base WRT_OPT] ; Default

	mov     eax, 1
	cpuid
	test    ecx, FLAG_CPUID1_ECX_SSE4_1
	lea     rbx, [pq_check_sse WRT_OPT]
	cmovne	rsi, rbx

	mov     [pq_check_dispatched], rsi
	pop     rsi
	pop     rdx
	pop     rcx
	pop     rbx
	pop     rax
	ret


;;;;
; xor_check multibinary function
;;;;
mk_global  xor_check, function
xor_check_mbinit:
	endbranch
	call    xor_check_dispatch_init
xor_check:
	endbranch
	jmp     qword [xor_check_dispatched]

xor_check_dispatch_init:
	push    rax
	push    rbx
	push    rcx
	push    rdx
	push    rsi
	lea     rsi, [xor_check_base WRT_OPT] ; Default

	mov     eax, 1
	cpuid
	test    ecx, FLAG_CPUID1_ECX_SSE4_1
	lea     rbx, [xor_check_sse WRT_OPT]
	cmovne  rsi, rbx

	mov     [xor_check_dispatched], rsi
	pop     rsi
	pop     rdx
	pop     rcx
	pop     rbx
	pop     rax
	ret

;;;       func          	core, ver, snum
slversion xor_gen,		00,   03,  0126
slversion xor_check,		00,   03,  0127
slversion pq_gen,		00,   03,  0128
slversion pq_check,		00,   03,  0129
