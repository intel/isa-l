;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;  Copyright(c) 2011-2018 Intel Corporation All rights reserved.
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

; heapify heap, heap_size, i, child, tmp1, tmp2, tmpd
%macro heapify2 7
%define %%heap      %1 ; qword ptr
%define %%heap_size %2 ; dword
%define %%i         %3 ; dword
%define %%child     %4 ; dword
%define %%tmp1      %5 ; qword
%define %%tmp2      %6 ; qword
%define %%tmpd      %7 ; dword
	align 16
%%heapify1:
	lea	%%child, [%%i + %%i]
	cmp	%%child, %%heap_size
	ja	%%end_heapify1
	mov	%%tmp1, [%%heap + %%child]
	mov	%%tmpd, %%child
	mov	%%tmp2, [%%heap + %%child) + 8]
	lea	%%child, [%%child + 1]
	cmove	%%tmp2, %%tmp1
	cmp	%%tmp1, %%tmp2
	cmovbe	%%child, %%tmpd
	cmovbe	%%tmp2, %%tmp1
	; child is correct, %%tmp2 = heap[child]
	mov	%%tmp1, [%%heap + %%i]
	cmp	%%tmp1, %%tmp2
	jbe	%%end_heapify1
	mov	[%%heap + %%i], %%tmp2
	mov	[%%heap + %%child], %%tmp1
	mov	%%i, %%child
	jmp	%%heapify1
%%end_heapify1
%endm

; heapify heap, heap_size, i, child, tmp1, tmp2, tmpd, tmp3
%macro heapify 8
%define %%heap      %1 ; qword ptr
%define %%heap_size %2 ; qword
%define %%i         %3 ; qword
%define %%child     %4 ; qword
%define %%tmp1      %5 ; qword
%define %%tmp2      %6 ; qword
%define %%tmpd      %7 ; qword
%define %%tmp3      %8
	align 16
%%heapify1:
	lea	%%child, [%%i + %%i]
;	mov	%%child, %%i
;	add	%%child, %%child
	cmp	%%child, %%heap_size
	ja	%%end_heapify1
	mov	%%tmp1, [%%heap + %%child*8]
	mov	%%tmp2, [%%heap + %%child*8 + 8]
	mov	%%tmp3, [%%heap + %%i*8]
	mov	%%tmpd, %%child
	add	%%tmpd, 1

	cmp	%%tmp2, %%tmp1
	cmovb	%%child, %%tmpd
	cmovb	%%tmp1, %%tmp2
	; child is correct, tmp1 = heap[child]
	cmp	%%tmp3, %%tmp1
	jbe	%%end_heapify1
	; swap i and child
	mov	[%%heap + %%i*8], %%tmp1
	mov	[%%heap + %%child*8], %%tmp3
	mov	%%i, %%child
	jmp	%%heapify1
%%end_heapify1:
%endm
