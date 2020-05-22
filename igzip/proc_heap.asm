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

; returns modified node_ptr
; uint32_t proc_heap(uint64_t *heap, uint32_t  heap_size);

%include "reg_sizes.asm"
%include "heap_macros.asm"

%ifidn __OUTPUT_FORMAT__, win64
%define heap		rcx	; pointer, 64-bit
%define heap_size	rdx
%define	arg3		r8
%define child		rsi
%define tmp32		rdi
%else
%define heap		rdi
%define heap_size	rsi
%define	arg3		rdx
%define child		rcx
%define tmp32		rdx
%endif

%define node_ptr	rax
%define h1		r8
%define h2		r9
%define h3		r10
%define i		r11
%define tmp2		r12

[bits 64]
default rel
section .text

	global build_huff_tree
build_huff_tree:
	endbranch
%ifidn __OUTPUT_FORMAT__, win64
	push	rsi
	push	rdi
%endif
	push	r12

	mov	node_ptr, arg3
.main_loop:
	; REMOVE_MIN64(heap, heap_size, h1);
	mov	h2, [heap + heap_size*8]
	mov	h1, [heap + 1*8]
	mov	qword [heap + heap_size*8], -1
	dec	heap_size
	mov	[heap + 1*8], h2

	mov	i, 1
	heapify	heap, heap_size, i, child, h2, h3, tmp32, tmp2

	mov	h2, [heap + 1*8]
	lea	h3, [h1 + h2]
	mov	[heap + node_ptr*8], h1 %+ w
	mov	[heap + node_ptr*8 - 8], h2 %+ w

	and 	h3, ~0xffff
	or	h3, node_ptr
	sub	node_ptr, 2

	; replace_min64(heap, heap_size, h3)
	mov	[heap + 1*8], h3
	mov	i, 1
	heapify	heap, heap_size, i, child, h2, h3, tmp32, tmp2

	cmp	heap_size, 1
	ja	.main_loop

	mov	h1, [heap + 1*8]
	mov	[heap + node_ptr*8], h1 %+ w

	pop	r12
%ifidn __OUTPUT_FORMAT__, win64
	pop	rdi
	pop	rsi
%endif
	ret

align 32
	global	build_heap
build_heap:
	endbranch
%ifidn __OUTPUT_FORMAT__, win64
	push	rsi
	push	rdi
%endif
	push	r12
	mov	qword [heap + heap_size*8 + 8], -1
	mov	i, heap_size
	shr	i, 1
.loop:
	mov	h1, i
	heapify	heap, heap_size, h1, child, h2, h3, tmp32, tmp2
	dec	i
	jnz	.loop

	pop	r12
%ifidn __OUTPUT_FORMAT__, win64
	pop	rdi
	pop	rsi
%endif
	ret
