/**********************************************************************
  Copyright (c) 2025 Institute of Software Chinese Academy of Sciences (ISCAS).

  Redistribution and use in source and binary forms, with or without
  modification, are permitted provided that the following conditions
  are met:
    * Redistributions of source code must retain the above copyright
      notice, this list of conditions and the following disclaimer.
    * Redistributions in binary form must reproduce the above copyright
      notice, this list of conditions and the following disclaimer in
      the documentation and/or other materials provided with the
      distribution.
    * Neither the name of ISCAS Corporation nor the names of its
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
#ifndef __RISCV_MULTIBINARY_H__
#define __RISCV_MULTIBINARY_H__
#ifndef __riscv
#error "This file is for riscv only"
#endif

#ifdef __ASSEMBLY__

/**
 * # mbin_interface : the wrapper layer for isal-l api
 *
 * ## references:
 * * aarch64_multibinary.h
 * ## Usage:
 *
 * 	1. Define dispather function
 * 	2. name must be \name\()_dispatcher
 * 	3. Prototype should be *"void * \name\()_dispatcher"*
 * 	4. The dispather should return the right function pointer , revision and a string information .
 **/
.macro mbin_interface name:req
	.section .data
	.align 3
	.global \name\()_dispatcher_info
	.type \name\()_dispatcher_info, @object
\name\()_dispatcher_info:
	.quad \name\()_mbinit
	.section .text
	.global \name\()_mbinit
\name\()_mbinit:
	addi        sp, sp, -56
	sd          ra, 48(sp)
	sd          a0, 0(sp)
	sd          a1, 8(sp)
	sd          a2, 16(sp)
	sd          a3, 24(sp)
	sd          a4, 32(sp)
	sd          a5, 40(sp)
	call        \name\()_dispatcher
	mv          t2, a0
	la          t0, \name\()_dispatcher_info
	sd          a0, 0(t0)
	ld          ra, 48(sp)
	ld          a0, 0(sp)
	ld          a1, 8(sp)
	ld          a2, 16(sp)
	ld          a3, 24(sp)
	ld          a4, 32(sp)
	ld          a5, 40(sp)
	addi        sp, sp, 56
	jr          t2
.global \name\()
.type \name,%function
\name\():
	la          t0, \name\()_dispatcher_info
	ld          t1, 0(t0)
	jr          t1
.size \name,. - \name
.endm

/**
 * mbin_interface_base is used for the interfaces which have only
 * noarch implementation
 */
.macro mbin_interface_base name:req, base:req
	.extern \base
	.data
	.align 3
	.global \name\()_dispatcher_info
	.type \name\()_dispatcher_info, @object
\name\()_dispatcher_info:
	.dword \base
	.text
	.global \name
	.type \name, @function
\name:
	la      t0, \name\()_dispatcher_info
	ld      t0, (t0)
	jr      t0
.endm
#else /* __ASSEMBLY__ */
#include <sys/auxv.h>
#define HWCAP_RV(letter) (1ul << ((letter) - 'A'))

#define DEFINE_INTERFACE_DISPATCHER(name)                               \
	void * name##_dispatcher(void)

#endif /* __ASSEMBLY__ */
#endif /* __RISCV_MULTIBINARY_H__ */
