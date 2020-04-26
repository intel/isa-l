/**********************************************************************
  Copyright(c) 2020 Arm Corporation All rights reserved.

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

	.text
	.arch armv8-a+crypto+crc
	.align	6

.macro crc32_u64	dst,src,data
	crc32cx		\dst,\src,\data
.endm

.macro crc32_u32	dst,src,data
	crc32cw		\dst,\src,\data
.endm

.macro crc32_u16	dst,src,data
	crc32ch		\dst,\src,\data
.endm

.macro crc32_u8		dst,src,data
	crc32cb		\dst,\src,\data
.endm

#include "crc32_mix_default_common.S"

	.global	crc32c_mix_default
	.type	crc32c_mix_default, %function
crc32c_mix_default:
	mov	w3, w2
	sxtw	x2, w1
	mov	x1, x0
	mov	w0, w3
	crc32_mix_main_default
	.size	crc32c_mix_default, .-crc32c_mix_default

	.section	.rodata
	.align	4
	.set	lanchor_crc32,. + 0

	.type	k1k2, %object
	.size	k1k2, 16
k1k2:
	.xword	0x00740eef02
	.xword	0x009e4addf8

	.type	k3k4, %object
	.size	k3k4, 16
k3k4:
	.xword	0x00f20c0dfe
	.xword	0x014cd00bd6

	.type	k5k0, %object
	.size	k5k0, 16
k5k0:
	.xword	0x00dd45aab8
	.xword	0

	.type	poly, %object
	.size	poly, 16
poly:
	.xword	0x0105ec76f0
	.xword	0x00dea713f1

	.type	crc32_const, %object
	.size	crc32_const, 48
crc32_const:
	.xword	0x9ef68d35
	.xword	0
	.xword	0x170076fa
	.xword	0
	.xword	0xdd7e3b0c
	.xword	0

	.align	4
	.set	.lanchor_mask,. + 0

	.type	mask, %object
	.size	mask, 16
mask:
	.word	-1
	.word	0
	.word	-1
	.word	0
