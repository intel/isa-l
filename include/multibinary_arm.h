########################################################################
#  Copyright(c) 2019 Arm Corporation All rights reserved.
#
#  Redistribution and use in source and binary forms, with or without
#  modification, are permitted provided that the following conditions
#  are met:
#    * Redistributions of source code must retain the above copyright
#      notice, this list of conditions and the following disclaimer.
#    * Redistributions in binary form must reproduce the above copyright
#      notice, this list of conditions and the following disclaimer in
#      the documentation and/or other materials provided with the
#      distribution.
#    * Neither the name of Arm Corporation nor the names of its
#      contributors may be used to endorse or promote products derived
#      from this software without specific prior written permission.
#
#  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
#  "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
#  LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
#  A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
#  OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
#  SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
#  LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
#  DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
#  THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
#  (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
#  OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
#########################################################################

/* TODO: support SVE */
.macro mbin_dispatch name:req, func_neon:req, func_sve
	.section .data
	.balign 8
	\name\()_dispatched:
		.quad	\name\()_dispatch_init

	.text
	.global \name
	\name\():
		adrp	x9, \name\()_dispatched
		ldr	x10, [x9, :lo12:\name\()_dispatched]
		br	x10
	\name\()_dispatch_init:
		add	x9, x9, :lo12:\name\()_dispatched
		adrp	x10, \func_neon
		add	x10, x10, :lo12:\func_neon
		str	x10, [x9]
		br	x10
.endm

#if 0
Macro expanded: mbin_dispatch xor_gen, xor_gen_neon

.section .data
.balign 8

xor_gen_dispatched:
	.quad	xor_gen_dispatch_init

.text

.global xor_gen
xor_gen:
	adrp	x9, xor_gen_dispatched
	ldr	x10, [x9, :lo12:xor_gen_dispatched]
	br	x10

xor_gen_dispatch_init:
	add	x9, x9, :lo12:xor_gen_dispatched
	adrp	x10, xor_gen_neon
	add	x10, x10, :lo12:xor_gen_neon
	str	x10, [x9]
	br	x10
#endif
