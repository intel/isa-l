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


;; internal macro used by push_all
;; push args L to R
%macro push_all_ 1-*
%xdefine _PUSH_ALL_REGS_COUNT_ %0
%rep %0
	push %1
	%rotate 1
%endrep
%endmacro

;; internal macro used by pop_all
;; pop args R to L
%macro pop_all_ 1-*
%rep %0
	%rotate -1
	pop %1
%endrep
%endmacro

%xdefine _PUSH_ALL_REGS_COUNT_ 0
%xdefine _ALLOC_STACK_VAL_     0
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; STACK_OFFSET
;; Number of bytes subtracted from stack due to PUSH_ALL and ALLOC_STACK
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
%define STACK_OFFSET (_PUSH_ALL_REGS_COUNT_ * 8 + _ALLOC_STACK_VAL_)

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; PUSH_ALL reg1, reg2, ...
;; push args L to R, remember regs for pop_all
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
%macro PUSH_ALL 1+
%xdefine _PUSH_ALL_REGS_ %1
	push_all_ %1
%endmacro

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; POP_ALL
;; push args from prev "push_all" R to L
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
%macro POP_ALL 0
	pop_all_ _PUSH_ALL_REGS_
%xdefine _PUSH_ALL_REGS_COUNT_ 0
%endmacro

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; ALLOC_STACK n
;; subtract n from the stack pointer and remember the value for restore_stack
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
%macro ALLOC_STACK 1
%xdefine _ALLOC_STACK_VAL_ %1
	sub	rsp, %1
%endmacro

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; RESTORE_STACK
;; add n to the stack pointer, where n is the arg to the previous alloc_stack
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
%macro RESTORE_STACK 0
	add	rsp, _ALLOC_STACK_VAL_
%xdefine _ALLOC_STACK_VAL_     0
%endmacro


;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; NOPN n
;; Create n bytes of NOP, using nops of up to 8 bytes each
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
%macro NOPN 1

 %assign %%i %1
 %rep 200
  %if (%%i < 9)
	nopn %%i
	%exitrep
  %else
	nopn 8
	%assign %%i (%%i - 8)
  %endif
 %endrep
%endmacro

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; nopn n
;; Create n bytes of NOP, where n is between 1 and 9
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
%macro nopn 1
%if (%1 == 1)
	nop
%elif (%1 == 2)
	db	0x66
	nop
%elif (%1 == 3)
	db	0x0F
	db	0x1F
	db	0x00
%elif (%1 == 4)
	db	0x0F
	db	0x1F
	db	0x40
	db	0x00
%elif (%1 == 5)
	db	0x0F
	db	0x1F
	db	0x44
	db	0x00
	db	0x00
%elif (%1 == 6)
	db	0x66
	db	0x0F
	db	0x1F
	db	0x44
	db	0x00
	db	0x00
%elif (%1 == 7)
	db	0x0F
	db	0x1F
	db	0x80
	db	0x00
	db	0x00
	db	0x00
	db	0x00
%elif (%1 == 8)
	db	0x0F
	db	0x1F
	db	0x84
	db	0x00
	db	0x00
	db	0x00
	db	0x00
	db	0x00
%elif (%1 == 9)
	db	0x66
	db	0x0F
	db	0x1F
	db	0x84
	db	0x00
	db	0x00
	db	0x00
	db	0x00
	db	0x00
%else
%error Invalid value to nopn
%endif
%endmacro

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; rolx64 dst, src, amount
;; Emulate a rolx instruction using rorx, assuming data 64 bits wide
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
%macro rolx64 3
	rorx %1, %2, (64-%3)
%endm

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; rolx32 dst, src, amount
;; Emulate a rolx instruction using rorx, assuming data 32 bits wide
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
%macro rolx32 3
	rorx %1, %2, (32-%3)
%endm


;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Define a function void ssc(uint64_t x)
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
%macro DEF_SSC 0
global ssc
ssc:
	mov	rax, rbx
	mov	rbx, rcx
	db	0x64
	db	0x67
	nop
	mov	rbx, rax
	ret
%endm
