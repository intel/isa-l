/* Copyright (c) 2018, Arm Limited. */

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
