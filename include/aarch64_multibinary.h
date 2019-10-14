/**********************************************************************
  Copyright(c) 2019 Arm Corporation All rights reserved.

  Redistribution and use in source and binary forms, with or without
  modification, are permitted provided that the following conditions
  are met:
    * Redistributions of source code must retain the above copyright
      notice, this list of conditions and the following disclaimer.
    * Redistributions in binary form must reproduce the above copyright
      notice, this list of conditions and the following disclaimer in
      the documentation and/or other materials provided with the
      distribution.
    * Neither the name of Arm Corporation nor the names of its
      contributors may be used to endorse or promote products derived
      from this software without specific prior written permission.

  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
  "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
  LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
  A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
  OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
  SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
  LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
  DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
  THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
  (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
  OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
**********************************************************************/
#ifndef __AARCH64_MULTIBINARY_H__
#define __AARCH64_MULTIBINARY_H__
#ifndef __aarch64__
#error "This file is for aarch64 only"
#endif
#include <asm/hwcap.h>
#ifdef __ASSEMBLY__
/**
 * # mbin_interface : the wrapper layer for isal-l api
 *
 * ## references:
 * * https://sourceware.org/git/gitweb.cgi?p=glibc.git;a=blob;f=sysdeps/aarch64/dl-trampoline.S
 * * http://infocenter.arm.com/help/topic/com.arm.doc.ihi0055b/IHI0055B_aapcs64.pdf
 * * https://static.docs.arm.com/ihi0057/b/IHI0057B_aadwarf64.pdf?_ga=2.80574487.1870739014.1564969896-1634778941.1548729310
 *
 * ## Usage:
 * 	1. Define dispather function
 * 	2. name must be \name\()_dispatcher
 * 	3. Prototype should be *"void * \name\()_dispatcher"*
 * 	4. The dispather should return the right function pointer , revision and a string information .
 **/
.macro mbin_interface name:req
	.extern \name\()_dispatcher
	.section        .data
	.balign 8
	.global \name\()_dispatcher_info
	.type   \name\()_dispatcher_info,%object

	\name\()_dispatcher_info:
		.quad   \name\()_mbinit         //func_entry

	.size   \name\()_dispatcher_info,. - \name\()_dispatcher_info

	.balign 8
	.text
	\name\()_mbinit:
		//save lp fp, sub sp
		.cfi_startproc
		stp     x29, x30, [sp, -224]!

		//add cfi directive to avoid GDB bt cmds error
		//set cfi(Call Frame Information)
		.cfi_def_cfa_offset 224
		.cfi_offset 	    29, -224
		.cfi_offset 	    30, -216

		//save parameter/result/indirect result registers
		stp	x8,  x9,  [sp,   16]
		.cfi_offset 	    8, -208
		.cfi_offset 	    9, -200
		stp	x0,  x1,  [sp,   32]
		.cfi_offset 	    0, -192
		.cfi_offset 	    1, -184
		stp	x2,  x3,  [sp,   48]
		.cfi_offset 	    2, -176
		.cfi_offset 	    3, -168
		stp	x4,  x5,  [sp,   64]
		.cfi_offset 	    4, -160
		.cfi_offset 	    5, -152
		stp	x6,  x7,  [sp,   80]
		.cfi_offset 	    6, -144
		.cfi_offset 	    7, -136
		stp	q0,  q1,  [sp,   96]
		.cfi_offset 	   64, -128
		.cfi_offset 	   65, -112
		stp	q2,  q3,  [sp,  128]
		.cfi_offset 	   66,  -96
		.cfi_offset 	   67,  -80
		stp	q4,  q5,  [sp,  160]
		.cfi_offset 	   68,  -64
		.cfi_offset 	   69,  -48
		stp	q6,  q7,  [sp,  192]
		.cfi_offset 	   70,  -32
		.cfi_offset 	   71,  -16

		/**
		 * The dispatcher functions have the following prototype:
		 * 	void * function_dispatcher(void)
		 * As the dispatcher is returning a struct, by the AAPCS,
		 */


		bl \name\()_dispatcher
		//restore temp/indirect result registers
		ldp	x8,  x9,  [sp,    16]
		.cfi_restore 8
		.cfi_restore 9

		//	save function entry
		str	x0,  [x9]

		//restore parameter/result registers
		ldp	x0,  x1,  [sp,    32]
		.cfi_restore 0
		.cfi_restore 1
		ldp	x2,  x3,  [sp,    48]
		.cfi_restore 2
		.cfi_restore 3
		ldp	x4,  x5,  [sp,    64]
		.cfi_restore 4
		.cfi_restore 5
		ldp	x6,  x7,  [sp,    80]
		.cfi_restore 6
		.cfi_restore 7
		ldp	q0,  q1,  [sp,    96]
		.cfi_restore 64
		.cfi_restore 65
		ldp	q2,  q3,  [sp,   128]
		.cfi_restore 66
		.cfi_restore 67
		ldp	q4,  q5,  [sp,   160]
		.cfi_restore 68
		.cfi_restore 69
		ldp	q6,  q7,  [sp,   192]
		.cfi_restore 70
		.cfi_restore 71
		//save lp fp and sp
		ldp     x29, x30, [sp], 224
		//restore cfi setting
		.cfi_restore 30
		.cfi_restore 29
		.cfi_def_cfa_offset 0
		.cfi_endproc

	.global \name
	.type \name,%function
	.align  2
	\name\():
		adrp    x9, :got:\name\()_dispatcher_info
		ldr     x9, [x9, #:got_lo12:\name\()_dispatcher_info]
		ldr     x10,[x9]
		br      x10
	.size \name,. - \name

.endm

/**
 * mbin_interface_base is used for the interfaces which have only
 * noarch implementation
 */
.macro mbin_interface_base name:req, base:req
	.extern \base
	.section        .data
	.balign 8
	.global \name\()_dispatcher_info
	.type   \name\()_dispatcher_info,%object

	\name\()_dispatcher_info:
		.quad   \base         //func_entry
	.size   \name\()_dispatcher_info,. - \name\()_dispatcher_info

	.balign 8
	.text
	.global \name
	.type \name,%function
	.align  2
	\name\():
		adrp    x9, :got:\name\()_dispatcher_info
		ldr     x9, [x9, #:got_lo12:\name\()_dispatcher_info]
		ldr     x10,[x9]
		br      x10
	.size \name,. - \name

.endm

#else /* __ASSEMBLY__ */
#include <sys/auxv.h>



#define DEFINE_INTERFACE_DISPATCHER(name)                               \
	void * name##_dispatcher(void)

#define PROVIDER_BASIC(name)                                            \
	PROVIDER_INFO(name##_base)

#define DO_DIGNOSTIC(x)	_Pragma GCC diagnostic ignored "-W"#x
#define DO_PRAGMA(x) _Pragma (#x)
#define DIGNOSTIC_IGNORE(x) DO_PRAGMA(GCC diagnostic ignored #x)
#define DIGNOSTIC_PUSH()	DO_PRAGMA(GCC diagnostic push)
#define DIGNOSTIC_POP()		DO_PRAGMA(GCC diagnostic pop)


#define PROVIDER_INFO(_func_entry)                                  	\
	({	DIGNOSTIC_PUSH()					\
		DIGNOSTIC_IGNORE(-Wnested-externs)			\
		extern void  _func_entry(void);				\
		DIGNOSTIC_POP()						\
		_func_entry;						\
	})

#endif /* __ASSEMBLY__ */
#endif
