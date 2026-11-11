/**********************************************************************
  Copyright (c) 2026 ZTE Corporation.
  Redistribution and use in source and binary forms, with or without
  modification, are permitted provided that the following conditions
  are met:
    * Redistributions of source code must retain the above copyright
      notice, this list of conditions and the following disclaimer.
    * Redistributions in binary form must reproduce the above copyright
      notice, this list of conditions and the following disclaimer in
      the documentation and/or other materials provided with the
      distribution.
    * Neither the name of ZTE Corporation nor the names of its
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
#ifndef __HUFFMAN_RISCV64_H__
#define __HUFFMAN_RISCV64_H__

#ifdef __ASSEMBLY__

.macro load_hash_const prod:req
	li	\prod, 0xB2D06057
.endm

/* compute_hash() from igzip/huffman.h on a zero-extended 32-bit \data. */
.macro compute_hash hash:req, data:req, prod:req
	mul	\hash, \data, \prod	/* hash = data * PROD (mod 2^64)	*/
	srli	\hash, \hash, 16
	mul	\hash, \hash, \prod	/* wraps mod 2^64, as in C		*/
	srli	\hash, \hash, 16
	slli	\hash, \hash, 32	/* (uint32_t) hash 		*/
	srli	\hash, \hash, 32
.endm

#else /* !__ASSEMBLY__ */
#include <stdint.h>

/* RVV-accelerated compare primitives (compare258_rvv.S). */
extern uint32_t
compare258_rvv(const uint8_t *str1, const uint8_t *str2, uint32_t max_length);
extern uint32_t
compare_rvv(const uint8_t *str1, const uint8_t *str2, uint32_t max_length);

#endif /* __ASSEMBLY__ */
#endif /* __HUFFMAN_RISCV64_H__ */
