########################################################################
#  Copyright(c) 2024 ByteDance All rights reserved.
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
#    * Neither the name of ByteDance Corporation nor the names of its
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
########################################################################

#include "crc_fold_common_clmul.h"

/* folding final reduction */
/* expects 128-bit value in HIGH:LOW (t0:t1), puts return value in SEED (a0) */
.macro crc64_refl_fold_reduction
	/* precomputed constants */
	ld K5, .k5

	clmulh t2, K5, HIGH
	clmul HIGH, K5, HIGH
	xor LOW, HIGH, LOW

	clmul LOW, MU, LOW
	/* as poly constant is 65-bits long missing the leading 1,
	 * multiplication requires a clmul(h) and xor
	 */
	clmulh HIGH, POLY, LOW
	xor LOW, HIGH, LOW
	xor SEED, LOW, t2
.fold_1_done:
.endm

/* calculate crc64 of a misaligned buffer using a table */
/* \len is the register holding how many bytes to read */
/* expects SEED (a0) and BUF (a1) to hold corresponding values */
/* updates values of SEED and BUF */
/* trashes t0, t1, t2 and t3 */
.macro crc64_refl_table len:req
	beqz \len, .table_done_\@
	add t1, BUF, \len
	la t0, .crc64_table
.table_loop_\@:
	lbu t2, (BUF)
	andi t3, SEED, 0xff
	addi BUF, BUF, 1
	xor t2, t2, t3
	slli t2, t2, 3
	add t2, t2, t0
	ld t3, (t2)
	srli SEED, SEED, 8
	xor SEED, SEED, t3
	bne BUF, t1, .table_loop_\@
.table_done_\@:
.endm

/* define a function to calculate a crc64 refl hash */
.macro crc64_func_refl name:req
.text
.align 1
.global \name
.type \name\(), %function
\name\():
	/* load precomputed constants */
	ld POLY, .poly
	ld MU, .mu

	/* invert seed */
	not SEED, SEED

	/* align and fold buffer to 64-bits */
	and t4, BUF, 0b111
	bltu LEN, t4, .excess
	crc64_refl_table t4
	sub LEN, LEN, t4

	crc_fold_loop 64 0 1
	crc64_refl_fold_reduction

.excess:
	crc64_refl_table LEN

	/* invert result */
	not SEED, SEED
	ret
.endm
