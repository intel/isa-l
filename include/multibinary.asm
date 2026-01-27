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

%ifndef _MULTIBINARY_ASM_
%define _MULTIBINARY_ASM_

%define dq	dq
%define ptr_sz	qword
%define rdi	rdi
%define rsi	rsi
%define rax	rax
%define rbx	rbx
%define rcx	rcx
%define rdx	rdx

;;;;
; multibinary macro:
;   creates the visible entry point that uses HW optimized call pointer
;   creates the init of the HW optimized call pointer
;;;;
%macro mbin_interface 1
	;;;;
	; *_dispatched is defaulted to *_mbinit and replaced on first call.
	; Therefore, *_dispatch_init is only executed on first call.
	;;;;
	section .data
	align 8
	%1_dispatched:
		dq	%1_mbinit

	section .text
	mk_global %1, function
	%1_mbinit:
		endbranch
		;;; only called the first time to setup hardware match
		call	%1_dispatch_init
		;;; falls thru to execute the hw optimized code
	%1:
		endbranch
		jmp	qword [%1_dispatched]
%endmacro

;;;;;
; mbin_dispatch_init parameters
; Use this function when SSE/00/01 is a minimum requirement
; 1-> function name
; 2-> SSE/00/01 optimized function used as base
; 3-> AVX or AVX/02 opt func
; 4-> AVX2 or AVX/04 opt func
;;;;;
%macro mbin_dispatch_init 4
	section .text
	%1_dispatch_init:
		push	rsi
		push	rax
		push	rbx
		push	rcx
		push	rdx
		lea	rsi, [%2 WRT_OPT] ; Default to SSE 00/01

		mov	eax, 1
		cpuid
		and	ecx, (FLAG_CPUID1_ECX_AVX | FLAG_CPUID1_ECX_OSXSAVE)
		cmp	ecx, (FLAG_CPUID1_ECX_AVX | FLAG_CPUID1_ECX_OSXSAVE)
		lea	rbx, [%3 WRT_OPT] ; AVX (gen2) opt func
		jne	_%1_init_done ; AVX is not available so end
		mov	rsi, rbx

		;; Try for AVX2
		xor	ecx, ecx
		mov	eax, 7
		cpuid
		test	ebx, FLAG_CPUID7_EBX_AVX2
		lea	rbx, [%4 WRT_OPT] ; AVX (gen4) opt func
		cmovne	rsi, rbx

		;; Does it have xmm and ymm support
		xor	ecx, ecx
		xgetbv
		and	eax, FLAG_XGETBV_EAX_XMM_YMM
		cmp	eax, FLAG_XGETBV_EAX_XMM_YMM
		je	_%1_init_done
		lea	rsi, [%2 WRT_OPT]

	_%1_init_done:
		pop	rdx
		pop	rcx
		pop	rbx
		pop	rax
		mov	[%1_dispatched], rsi
		pop	rsi
		ret
%endmacro
;;;;;
; mbin_dispatch_init_clmul 5 parameters
; Use this case for CRC which needs both SSE4_1 and CLMUL
; 1-> function name
; 2-> base function
; 3-> SSE4_1 and CLMUL optimized function
; 4-> AVX/02 opt func
; 5-> AVX512/10 opt func
;;;;;
%macro mbin_dispatch_init_clmul 5
	section .text
	%1_dispatch_init:
		push	rsi
		push	rax
		push	rbx
		push	rcx
		push	rdx
		push	rdi
		lea     rsi, [%2 WRT_OPT] ; Default - use base function

		mov     eax, 1
		cpuid
		mov	ebx, ecx ; save cpuid1.ecx
		test	ecx, FLAG_CPUID1_ECX_SSE4_1
		jz	_%1_init_done
		test    ecx, FLAG_CPUID1_ECX_CLMUL
		jz	_%1_init_done
		lea	rsi, [%3 WRT_OPT] ; SSE possible so use 00/01 opt

		;; Test for XMM_YMM support/AVX
		test	ecx, FLAG_CPUID1_ECX_OSXSAVE
		je	_%1_init_done
		xor	ecx, ecx
		xgetbv	; xcr -> edx:eax
		mov	edi, eax	  ; save xgetvb.eax

		and	eax, FLAG_XGETBV_EAX_XMM_YMM
		cmp	eax, FLAG_XGETBV_EAX_XMM_YMM
		jne	_%1_init_done
		test	ebx, FLAG_CPUID1_ECX_AVX
		je	_%1_init_done
		lea	rsi, [%4 WRT_OPT] ; AVX/02 opt

		;; Test for AVX2
		xor	ecx, ecx
		mov	eax, 7
		cpuid
		test	ebx, FLAG_CPUID7_EBX_AVX2
		je	_%1_init_done		; No AVX2 possible

		;; Test for AVX512
		and	edi, FLAG_XGETBV_EAX_ZMM_OPM
		cmp	edi, FLAG_XGETBV_EAX_ZMM_OPM
		jne	_%1_init_done	  ; No AVX512 possible
		and	ebx, FLAGS_CPUID7_EBX_AVX512_G1
		cmp	ebx, FLAGS_CPUID7_EBX_AVX512_G1
		jne	_%1_init_done

		and	ecx, FLAGS_CPUID7_ECX_AVX512_G2
		cmp	ecx, FLAGS_CPUID7_ECX_AVX512_G2
		lea	rbx, [%5 WRT_OPT] ; AVX512/10 opt
		cmove	rsi, rbx
	_%1_init_done:
		pop	rdi
		pop	rdx
		pop	rcx
		pop	rbx
		pop	rax
		mov	[%1_dispatched], rsi
		pop	rsi
		ret
%endmacro
;;;;;
; mbin_dispatch_init_clmul 6 parameters
; Use this case for CRC which needs both SSE4_1 and CLMUL
; 1-> function name
; 2-> base function
; 3-> SSE4_1 and CLMUL optimized function
; 4-> AVX/02 opt func
; 5-> AVX2 opt func
; 6-> AVX512/10 opt func
;;;;;
%macro mbin_dispatch_init_clmul6 6
	section .text
	%1_dispatch_init:
		push	rsi
		push	rax
		push	rbx
		push	rcx
		push	rdx
		push	rdi
		lea     rsi, [%2 WRT_OPT] ; Default - use base function

		mov     eax, 1
		cpuid
		mov	ebx, ecx ; save cpuid1.ecx
		test	ecx, FLAG_CPUID1_ECX_SSE4_1
		jz	_%1_init_done
		test    ecx, FLAG_CPUID1_ECX_CLMUL
		jz	_%1_init_done
		lea	rsi, [%3 WRT_OPT] ; SSE possible so use 00/01 opt

		;; Test for XMM_YMM support/AVX
		test	ecx, FLAG_CPUID1_ECX_OSXSAVE
		je	_%1_init_done
		xor	ecx, ecx
		xgetbv	; xcr -> edx:eax
		mov	edi, eax	  ; save xgetvb.eax

		and	eax, FLAG_XGETBV_EAX_XMM_YMM
		cmp	eax, FLAG_XGETBV_EAX_XMM_YMM
		jne	_%1_init_done
		test	ebx, FLAG_CPUID1_ECX_AVX
		je	_%1_init_done
		lea	rsi, [%4 WRT_OPT] ; AVX/02 opt

		;; Test for AVX2
		xor	ecx, ecx
		mov	eax, 7
		cpuid
		test	ebx, FLAG_CPUID7_EBX_AVX2
		je	_%1_init_done		; No AVX2 possible
		and	ecx, FLAGS_CPUID7_ECX_AVX2_G2
		cmp	ecx, FLAGS_CPUID7_ECX_AVX2_G2
		lea	rbx, [%5 WRT_OPT]
		cmove	rsi, rbx

		;; Test for AVX512
		and	edi, FLAG_XGETBV_EAX_ZMM_OPM
		cmp	edi, FLAG_XGETBV_EAX_ZMM_OPM
		jne	_%1_init_done	  ; No AVX512 possible
		and	ebx, FLAGS_CPUID7_EBX_AVX512_G1
		cmp	ebx, FLAGS_CPUID7_EBX_AVX512_G1
		jne	_%1_init_done

		and	ecx, FLAGS_CPUID7_ECX_AVX512_G2
		cmp	ecx, FLAGS_CPUID7_ECX_AVX512_G2
		lea	rbx, [%6 WRT_OPT] ; AVX512/10 opt
		cmove	rsi, rbx

	_%1_init_done:
		pop	rdi
		pop	rdx
		pop	rcx
		pop	rbx
		pop	rax
		mov	[%1_dispatched], rsi
		pop	rsi
		ret
%endmacro
;;;;;
; mbin_dispatch_init5 parameters
; 1-> function name
; 2-> base function
; 3-> SSE4_2 or 00/01 optimized function
; 4-> AVX/02 opt func
; 5-> AVX2/04 opt func
;;;;;
%macro mbin_dispatch_init5 5
	section .text
	%1_dispatch_init:
		push	rsi
		push	rax
		push	rbx
		push	rcx
		push	rdx
		lea	rsi, [%2 WRT_OPT] ; Default - use base function

		mov	eax, 1
		cpuid
		; Test for SSE4.2
		test	ecx, FLAG_CPUID1_ECX_SSE4_2
		lea	rbx, [%3 WRT_OPT] ; SSE opt func
		cmovne	rsi, rbx

		and	ecx, (FLAG_CPUID1_ECX_AVX | FLAG_CPUID1_ECX_OSXSAVE)
		cmp	ecx, (FLAG_CPUID1_ECX_AVX | FLAG_CPUID1_ECX_OSXSAVE)
		lea	rbx, [%4 WRT_OPT] ; AVX (gen2) opt func
		jne	_%1_init_done ; AVX is not available so end
		mov	rsi, rbx

		;; Try for AVX2
		xor	ecx, ecx
		mov	eax, 7
		cpuid
		test	ebx, FLAG_CPUID7_EBX_AVX2
		lea	rbx, [%5 WRT_OPT] ; AVX (gen4) opt func
		cmovne	rsi, rbx

		;; Does it have xmm and ymm support
		xor	ecx, ecx
		xgetbv
		and	eax, FLAG_XGETBV_EAX_XMM_YMM
		cmp	eax, FLAG_XGETBV_EAX_XMM_YMM
		je	_%1_init_done
		lea	rsi, [%3 WRT_OPT]

	_%1_init_done:
		pop	rdx
		pop	rcx
		pop	rbx
		pop	rax
		mov	[%1_dispatched], rsi
		pop	rsi
		ret
%endmacro

;;;;;
; mbin_dispatch_init6 parameters
; 1-> function name
; 2-> base function
; 3-> SSE4_2 or 00/01 optimized function
; 4-> AVX/02 opt func
; 5-> AVX2/04 opt func
; 6-> AVX512/06 opt func
;;;;;
%macro mbin_dispatch_init6 6
	section .text
	%1_dispatch_init:
		push	rsi
		push	rax
		push	rbx
		push	rcx
		push	rdx
		push	rdi
		lea	rsi, [%2 WRT_OPT] ; Default - use base function

		mov	eax, 1
		cpuid
		mov	ebx, ecx ; save cpuid1.ecx
		test	ecx, FLAG_CPUID1_ECX_SSE4_2
		je	_%1_init_done	  ; Use base function if no SSE4_2
		lea	rsi, [%3 WRT_OPT] ; SSE possible so use 00/01 opt

		;; Test for XMM_YMM support/AVX
		test	ecx, FLAG_CPUID1_ECX_OSXSAVE
		je	_%1_init_done
		xor	ecx, ecx
		xgetbv	; xcr -> edx:eax
		mov	edi, eax	  ; save xgetvb.eax

		and	eax, FLAG_XGETBV_EAX_XMM_YMM
		cmp	eax, FLAG_XGETBV_EAX_XMM_YMM
		jne	_%1_init_done
		test	ebx, FLAG_CPUID1_ECX_AVX
		je	_%1_init_done
		lea	rsi, [%4 WRT_OPT] ; AVX/02 opt

		;; Test for AVX2
		xor	ecx, ecx
		mov	eax, 7
		cpuid
		test	ebx, FLAG_CPUID7_EBX_AVX2
		je	_%1_init_done		; No AVX2 possible
		lea	rsi, [%5 WRT_OPT] 	; AVX2/04 opt func

		;; Test for AVX512
		and	edi, FLAG_XGETBV_EAX_ZMM_OPM
		cmp	edi, FLAG_XGETBV_EAX_ZMM_OPM
		jne	_%1_init_done	  ; No AVX512 possible
		and	ebx, FLAGS_CPUID7_EBX_AVX512_G1
		cmp	ebx, FLAGS_CPUID7_EBX_AVX512_G1
		lea	rbx, [%6 WRT_OPT] ; AVX512/06 opt
		cmove	rsi, rbx

	_%1_init_done:
		pop	rdi
		pop	rdx
		pop	rcx
		pop	rbx
		pop	rax
		mov	[%1_dispatched], rsi
		pop	rsi
		ret
%endmacro

;;;;;
; mbin_dispatch_init7 parameters
; 1-> function name
; 2-> base function
; 3-> SSE4_2 or 00/01 optimized function
; 4-> AVX/02 opt func
; 5-> AVX2/04 opt func
; 6-> AVX512/06 opt func
; 7-> AVX512 Update/10 opt func
;;;;;
%macro mbin_dispatch_init7 7
	section .text
	%1_dispatch_init:
		push	rsi
		push	rax
		push	rbx
		push	rcx
		push	rdx
		push	rdi
		lea	rsi, [%2 WRT_OPT] ; Default - use base function

		mov	eax, 1
		cpuid
		mov	ebx, ecx ; save cpuid1.ecx
		test	ecx, FLAG_CPUID1_ECX_SSE4_2
		je	_%1_init_done	  ; Use base function if no SSE4_2
		lea	rsi, [%3 WRT_OPT] ; SSE possible so use 00/01 opt

		;; Test for XMM_YMM support/AVX
		test	ecx, FLAG_CPUID1_ECX_OSXSAVE
		je	_%1_init_done
		xor	ecx, ecx
		xgetbv	; xcr -> edx:eax
		mov	edi, eax	  ; save xgetvb.eax

		and	eax, FLAG_XGETBV_EAX_XMM_YMM
		cmp	eax, FLAG_XGETBV_EAX_XMM_YMM
		jne	_%1_init_done
		test	ebx, FLAG_CPUID1_ECX_AVX
		je	_%1_init_done
		lea	rsi, [%4 WRT_OPT] ; AVX/02 opt

		;; Test for AVX2
		xor	ecx, ecx
		mov	eax, 7
		cpuid
		test	ebx, FLAG_CPUID7_EBX_AVX2
		je	_%1_init_done		; No AVX2 possible
		lea	rsi, [%5 WRT_OPT] 	; AVX2/04 opt func

		;; Test for AVX512
		and	edi, FLAG_XGETBV_EAX_ZMM_OPM
		cmp	edi, FLAG_XGETBV_EAX_ZMM_OPM
		jne	_%1_init_done	  ; No AVX512 possible
		and	ebx, FLAGS_CPUID7_EBX_AVX512_G1
		cmp	ebx, FLAGS_CPUID7_EBX_AVX512_G1
		lea	rbx, [%6 WRT_OPT] ; AVX512/06 opt
		cmove	rsi, rbx

		and	ecx, FLAGS_CPUID7_ECX_AVX512_G2
		cmp	ecx, FLAGS_CPUID7_ECX_AVX512_G2
		lea	rbx, [%7 WRT_OPT] ; AVX512/06 opt
		cmove	rsi, rbx

	_%1_init_done:
		pop	rdi
		pop	rdx
		pop	rcx
		pop	rbx
		pop	rax
		mov	[%1_dispatched], rsi
		pop	rsi
		ret
%endmacro

;;;;;
; mbin_dispatch_init8 parameters
; 1-> function name
; 2-> base function
; 3-> SSE4_2 or 00/01 optimized function
; 4-> AVX/02 opt func
; 5-> AVX2/04 opt func
; 6-> AVX512/06 opt func
; 7-> AVX2 Update/07 opt func
; 8-> AVX512 Update/10 opt func
;;;;;
%macro mbin_dispatch_init8 8
	section .text
	%1_dispatch_init:
		push	rsi
		push	rax
		push	rbx
		push	rcx
		push	rdx
		push	rdi
		lea	rsi, [%2 WRT_OPT] ; Default - use base function

		mov	eax, 1
		cpuid
		mov	ebx, ecx ; save cpuid1.ecx
		test	ecx, FLAG_CPUID1_ECX_SSE4_2
		je	_%1_init_done	  ; Use base function if no SSE4_2
		lea	rsi, [%3 WRT_OPT] ; SSE possible so use 00/01 opt

		;; Test for XMM_YMM support/AVX
		test	ecx, FLAG_CPUID1_ECX_OSXSAVE
		je	_%1_init_done
		xor	ecx, ecx
		xgetbv	; xcr -> edx:eax
		mov	edi, eax	  ; save xgetvb.eax

		and	eax, FLAG_XGETBV_EAX_XMM_YMM
		cmp	eax, FLAG_XGETBV_EAX_XMM_YMM
		jne	_%1_init_done
		test	ebx, FLAG_CPUID1_ECX_AVX
		je	_%1_init_done
		lea	rsi, [%4 WRT_OPT] ; AVX/02 opt

		;; Test for AVX2
		xor	ecx, ecx
		mov	eax, 7
		cpuid
		test	ebx, FLAG_CPUID7_EBX_AVX2
		je	_%1_init_done		; No AVX2 possible
		lea	rsi, [%5 WRT_OPT] 	; AVX2/04 opt func

		;; Test for AVX512
		and	edi, FLAG_XGETBV_EAX_ZMM_OPM
		cmp	edi, FLAG_XGETBV_EAX_ZMM_OPM
		jne	_%1_check_avx2_g2	  ; No AVX512 possible
		and	ebx, FLAGS_CPUID7_EBX_AVX512_G1
		cmp	ebx, FLAGS_CPUID7_EBX_AVX512_G1
		lea	rbx, [%6 WRT_OPT] ; AVX512/06 opt
		cmove	rsi, rbx

		and	ecx, FLAGS_CPUID7_ECX_AVX512_G2
		cmp	ecx, FLAGS_CPUID7_ECX_AVX512_G2
		lea	rbx, [%8 WRT_OPT] ; AVX512/10 opt
		cmove	rsi, rbx
		jmp     _%1_init_done

	_%1_check_avx2_g2:
		;; Test for AVX2 Gen 2
		and	ecx, FLAGS_CPUID7_ECX_AVX2_G2
		cmp	ecx, FLAGS_CPUID7_ECX_AVX2_G2
		lea	rbx, [%7 WRT_OPT] ; AVX2/7 opt
		cmove	rsi, rbx

	_%1_init_done:
		pop	rdi
		pop	rdx
		pop	rcx
		pop	rbx
		pop	rax
		mov	[%1_dispatched], rsi
		pop	rsi
		ret
%endmacro

;;;;;
; mbin_dispatch_init8_hygon parameters
; 1-> function name
; 2-> base function
; 3-> SSE4_2 or 00/01 optimized function
; 4-> AVX/02 opt func
; 5-> AVX2/04 opt func
; 6-> AVX512/06 opt func
; 7-> AVX2 Update/07 opt func
; 8-> AVX512 Update/10 opt func
;
; With special case:
; - Use AVX on Hygon 1/2/3 platform
;;;;;
%macro mbin_dispatch_init8_hygon 8
	section .text
	%1_dispatch_init:
		push	rsi
		push	rax
		push	rbx
		push	rcx
		push	rdx
		push	rdi
		lea	rsi, [%2 WRT_OPT] ; Default - use base function

		mov	eax, 1
		cpuid
		mov	ebx, ecx ; save cpuid1.ecx
		test	ecx, FLAG_CPUID1_ECX_SSE4_2
		je	_%1_init_done	  ; Use base function if no SSE4_2
		lea	rsi, [%3 WRT_OPT] ; SSE possible so use 00/01 opt

		;; Test for XMM_YMM support/AVX
		test	ecx, FLAG_CPUID1_ECX_OSXSAVE
		je	_%1_init_done
		xor	ecx, ecx
		xgetbv	; xcr -> edx:eax
		mov	edi, eax	  ; save xgetvb.eax

		and	eax, FLAG_XGETBV_EAX_XMM_YMM
		cmp	eax, FLAG_XGETBV_EAX_XMM_YMM
		jne	_%1_init_done
		test	ebx, FLAG_CPUID1_ECX_AVX
		je	_%1_init_done
		lea	rsi, [%4 WRT_OPT] ; AVX/02 opt

		;; Hygon platform check: Use AVX opt on Hygon 1/2/3 for performance
		;; Even if the have the ability to use AVX2 opt
		xor	eax, eax
		cpuid
		mov	eax, FLAG_CPUID0_EBX_HYGON
		cmp	eax, ebx
		jne	_%1_check_avx2	; Not Hygon. Proceed as normal

		mov	eax, FLAG_CPUID0_EDX_HYGON
		cmp	eax, edx
		jne	_%1_check_avx2	; Not Hygon. Proceed as normal

		mov	eax, FLAG_CPUID0_ECX_HYGON
		cmp	eax, ecx
		jne	_%1_check_avx2	; Not Hygon. Proceed as normal

		;; All vendor ID matches: Hygon confirmed
		;; Further family & model check: Identify Hygon 1/2/3
		mov	eax, 1
		cpuid
		and	eax, FLAG_CPUID1_EAX_STEP_MASK
		mov	ecx, FLAG_CPUID1_EAX_HYGON1
		mov	edx, FLAG_CPUID1_EAX_HYGON2
		mov	ebx, FLAG_CPUID1_EAX_HYGON3

		cmp	eax, ecx	; Hygon 1
		je	_%1_hygon_123_init
		cmp	eax, edx	; Hygon 2
		je	_%1_hygon_123_init
		cmp	eax, ebx	; Hygon 3
		jne	_%1_check_avx2	; Not any of Hygon 1/2/3: Continue normal procedure

	_%1_hygon_123_init:
		;; Init complete early for Hygon 1/2/3.
		jmp	_%1_init_done	; Use AVX opt func registered before

	_%1_check_avx2:
		;; Test for AVX2
		xor	ecx, ecx
		mov	eax, 7
		cpuid
		test	ebx, FLAG_CPUID7_EBX_AVX2
		je	_%1_init_done		; No AVX2 possible
		lea	rsi, [%5 WRT_OPT] 	; AVX2/04 opt func

		;; Test for AVX512
		and	edi, FLAG_XGETBV_EAX_ZMM_OPM
		cmp	edi, FLAG_XGETBV_EAX_ZMM_OPM
		jne	_%1_check_avx2_g2	  ; No AVX512 possible
		and	ebx, FLAGS_CPUID7_EBX_AVX512_G1
		cmp	ebx, FLAGS_CPUID7_EBX_AVX512_G1
		lea	rbx, [%6 WRT_OPT] ; AVX512/06 opt
		cmove	rsi, rbx

		and	ecx, FLAGS_CPUID7_ECX_AVX512_G2
		cmp	ecx, FLAGS_CPUID7_ECX_AVX512_G2
		lea	rbx, [%8 WRT_OPT] ; AVX512/10 opt
		cmove	rsi, rbx
		jmp     _%1_init_done

	_%1_check_avx2_g2:
		;; Test for AVX2 Gen 2
		and	ecx, FLAGS_CPUID7_ECX_AVX2_G2
		cmp	ecx, FLAGS_CPUID7_ECX_AVX2_G2
		lea	rbx, [%7 WRT_OPT] ; AVX2/7 opt
		cmove	rsi, rbx

	_%1_init_done:
		pop	rdi
		pop	rdx
		pop	rcx
		pop	rbx
		pop	rax
		mov	[%1_dispatched], rsi
		pop	rsi
		ret
%endmacro

%endif ; ifndef _MULTIBINARY_ASM_
