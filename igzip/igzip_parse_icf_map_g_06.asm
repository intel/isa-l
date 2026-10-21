;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;  Copyright(c) 2026 Intel Corporation All rights reserved.
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
%include "lz0a_const.asm"
%include "data_struct2.asm"

;; Symbols of the match map entries
%define LEN_START	257
%define LEN_OFFSET	(LEN_START - 3)
%define LIT_START	31		; NULL_DIST_SYM + 1

;; Size of the table of advances, in words: a match map of MATCH_BUF_SIZE (4K) entries plus 64,
;; more than the 15 words the first pass, which writes 16 at a time, may write past the last
;; entry. igzip_icf_body.c stops the build if MATCH_BUF_SIZE + 16 does not fit.
%define MAX_ENTRIES	(4 * 1024 + 64)
%define ADV_SIZE	(2 * MAX_ENTRIES)

%define start	r12
%define nlim	r13
%define out	r14
%define olim	r15
%define hist	rbp
%define idx	rbx

%define code	rax
%define l0	rcx
%define l1	rdx
%define m0	rsi
%define m1	rdi
%define adv	r8
%define dsym	r9
%define tmp	r10
%define packed	r11

%define zlen_mask	zmm16
%define zlen_start	zmm17
%define zlen_offset	zmm18
%define zlen_offset1	zmm19
%define ztwo		zmm20
%define zlens		zmm21
%define zlens2		zmm22
%define zadv		zmm23

;; Touches each page of the %1 bytes below rsp, from the top down, so that the frame cannot
;; skip a guard page below the stack
%macro PROBE_FRAME 1
%assign %%off 4096
%rep (%1) / 4096
	test	[rsp - %%off], rsp
%assign %%off (%%off + 4096)
%endrep
	test	[rsp - (%1)], rsp
%endm

;; Stack: the table of advances, then the arguments kept for the end
%define ADV_OFFSET	0
%define END_OFFSET	ADV_SIZE
%define ICF_NEXT_OFFSET	(ADV_SIZE + 8)
%define LOCALS_SIZE	(ADV_SIZE + 16)

%ifidn __OUTPUT_FORMAT__, win64
%define arg1 rcx
%define arg2 rdx
%define arg3 r8
%define arg4 r9
%define GPR_SAVE	LOCALS_SIZE
%define stack_size	(LOCALS_SIZE + 8 * 8 + 8)
%define arg5		[rsp + stack_size + 8 + 32]
%define func(x) proc_frame x
%macro FUNC_SAVE 0
	PROBE_FRAME stack_size
	alloc_stack	stack_size
	save_reg	rbx, GPR_SAVE + 0*8
	save_reg	rbp, GPR_SAVE + 1*8
	save_reg	rsi, GPR_SAVE + 2*8
	save_reg	rdi, GPR_SAVE + 3*8
	save_reg	r12, GPR_SAVE + 4*8
	save_reg	r13, GPR_SAVE + 5*8
	save_reg	r14, GPR_SAVE + 6*8
	save_reg	r15, GPR_SAVE + 7*8
	end_prolog
%endm
%macro FUNC_RESTORE 0
	mov	rbx, [rsp + GPR_SAVE + 0*8]
	mov	rbp, [rsp + GPR_SAVE + 1*8]
	mov	rsi, [rsp + GPR_SAVE + 2*8]
	mov	rdi, [rsp + GPR_SAVE + 3*8]
	mov	r12, [rsp + GPR_SAVE + 4*8]
	mov	r13, [rsp + GPR_SAVE + 5*8]
	mov	r14, [rsp + GPR_SAVE + 6*8]
	mov	r15, [rsp + GPR_SAVE + 7*8]
	add	rsp, stack_size
%endm
%else
%define arg1 rdi
%define arg2 rsi
%define arg3 rdx
%define arg4 rcx
%define arg5 r8
%define stack_size	(LOCALS_SIZE + 8)
%define func(x) x: endbranch
%macro FUNC_SAVE 0
	push	rbx
	push	rbp
	push	r12
	push	r13
	push	r14
	push	r15
	PROBE_FRAME stack_size
	sub	rsp, stack_size
%endm
%macro FUNC_RESTORE 0
	add	rsp, stack_size
	pop	r15
	pop	r14
	pop	r13
	pop	r12
	pop	rbp
	pop	rbx
%endm
%endif

[bits 64]
default rel
section .text

;; struct deflate_icf *parse_icf_map_g_06(struct deflate_icf *next, struct deflate_icf *end,
;;					   struct deflate_icf **icf_next, struct deflate_icf *icf_end,
;;					   struct isal_mod_hist *hist)
;;
;; Parses the match map from next to end greedily as parse_icf_map_g_base() does, writing the
;; codes from *icf_next on and counting them in hist. A first pass finds how far the parse moves
;; from each entry, so that the parse itself only follows those advances and picks each step
;; with conditional moves instead of branches.
global parse_icf_map_g_06
func(parse_icf_map_g_06)
	endbranch
	FUNC_SAVE

	mov	start, arg1
	mov	[rsp + END_OFFSET], arg2
	mov	[rsp + ICF_NEXT_OFFSET], arg3
	mov	hist, arg5
	mov	out, [arg3]
	lea	olim, [arg4 - ICF_CODE_BYTES]

	xor	idx, idx
	mov	nlim, arg2
	sub	nlim, start
	jle	.walk_end
	shr	nlim, 2

	;; Find the advance from each entry: the length of its match, the length of the next
	;; entry's match plus 1, or 2 for two literals
	;; The loads may read up to 16 entries past end, which hash_map.overflow[] makes room for
	vpbroadcastd zlen_mask, [len_mask]
	vpbroadcastd zlen_start, [len_start]
	vpbroadcastd zlen_offset, [len_offset]
	vpbroadcastd zlen_offset1, [len_offset1]
	vpbroadcastd ztwo, [two]
.adv_loop:
	vpandd	zlens, zlen_mask, [start + ICF_CODE_BYTES * idx]
	vpandd	zlens2, zlen_mask, [start + ICF_CODE_BYTES * idx + ICF_CODE_BYTES]
	vpcmpud	k1, zlens, zlen_start, 6
	vpcmpud	k2, zlens2, zlen_start, 6
	vmovdqa32 zadv, ztwo
	vpsubd	zadv {k2}, zlens2, zlen_offset1
	vpsubd	zadv {k1}, zlens, zlen_offset
	vpmovdw	[rsp + ADV_OFFSET + 2 * idx], zadv
	add	idx, 16
	cmp	idx, nlim
	jb	.adv_loop

	sub	nlim, 1
	xor	idx, idx
	cmp	idx, nlim
	jae	.walk_end
	cmp	out, olim
	jae	.walk_end

.walk:
	movzx	adv %+ d, word [rsp + ADV_OFFSET + 2 * idx]
	mov	code, [start + ICF_CODE_BYTES * idx]
	mov	l0 %+ d, code %+ d
	and	l0 %+ d, LIT_LEN_MASK
	rorx	l1, code, 32
	and	l1 %+ d, LIT_LEN_MASK
	add	dword [hist + _ll_hist + HIST_ELEM_SIZE * l0], 1

	;; m0 and m1 are 1 for matches
	xor	m0 %+ d, m0 %+ d
	cmp	l0 %+ d, LEN_START - 1
	seta	m0 %+ b
	xor	m1 %+ d, m1 %+ d
	cmp	l1 %+ d, LEN_START - 1
	seta	m1 %+ b

	;; The second entry counts unless the first is a match
	mov	dsym %+ d, m0 %+ d
	xor	dsym %+ d, 1
	add	[hist + _ll_hist + HIST_ELEM_SIZE * l1], dsym %+ d

	;; The distance of the match taken counts. Two literals add 0 to d_hist[30], which is
	;; ll_hist[0].
	rorx	dsym, code, 32 + DIST_OFFSET
	and	dsym %+ d, LIT_DIST_MASK
	rorx	tmp, code, DIST_OFFSET
	and	tmp %+ d, LIT_DIST_MASK
	test	m0 %+ d, m0 %+ d
	cmovnz	dsym %+ d, tmp %+ d
	andn	tmp %+ d, m0 %+ d, m1 %+ d
	or	m1 %+ d, m0 %+ d
	add	[hist + _d_hist + HIST_ELEM_SIZE * dsym], m1 %+ d

	;; Write the first entry, both entries for a literal and a match, or both literals in one
	lea	packed %+ d, [l1 + LIT_START]
	shl	packed %+ d, DIST_OFFSET
	or	packed %+ d, l0 %+ d
	test	m1 %+ d, m1 %+ d
	cmovnz	packed, code
	mov	[out], packed
	lea	out, [out + ICF_CODE_BYTES + ICF_CODE_BYTES * tmp]

	add	idx, adv
	cmp	idx, nlim
	jae	.walk_end
	cmp	out, olim
	jb	.walk

.walk_end:
	;; Finish the entries left one at a time
	lea	start, [start + ICF_CODE_BYTES * idx]
	mov	nlim, [rsp + END_OFFSET]
	add	olim, ICF_CODE_BYTES
	cmp	start, nlim
	jae	.end
	cmp	out, olim
	jae	.end
.tail:
	mov	code %+ d, [start]
	mov	[out], code %+ d
	add	out, ICF_CODE_BYTES
	mov	l0 %+ d, code %+ d
	and	l0 %+ d, LIT_LEN_MASK
	add	dword [hist + _ll_hist + HIST_ELEM_SIZE * l0], 1
	add	start, ICF_CODE_BYTES
	cmp	l0 %+ d, LEN_START
	jb	.tail_next
	shr	code %+ d, DIST_OFFSET
	and	code %+ d, LIT_DIST_MASK
	add	dword [hist + _d_hist + HIST_ELEM_SIZE * code], 1
	lea	start, [start + ICF_CODE_BYTES * l0 - ICF_CODE_BYTES * (LEN_OFFSET + 1)]
.tail_next:
	cmp	start, nlim
	jae	.end
	cmp	out, olim
	jb	.tail

.end:
	mov	tmp, [rsp + ICF_NEXT_OFFSET]
	mov	[tmp], out
	mov	rax, start

	FUNC_RESTORE
	ret

endproc_frame

section .data
align 4
len_mask:
	dd LIT_LEN_MASK
len_start:
	dd LEN_START - 1
len_offset:
	dd LEN_OFFSET
len_offset1:
	dd LEN_OFFSET - 1
two:
	dd 2
