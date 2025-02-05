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

#include "crc32_refl_common_clmul.h"

/* uint32_t crc32_iscsi(uint8_t *buffer, int len, uint32_t init_crc) */
.text
.align 1
.global crc32_iscsi
.type crc32_iscsi, %function
crc32_iscsi:
	/* switch around arguments to match common crc functions */
	mv t0, a2
	mv a2, a1
	mv a1, a0
	mv a0, t0

	/* load precomputed constants */
	ld POLY, .poly_refl
	ld MU, .mu

	/* zero-extend seed */
	slli SEED, SEED, 32
	srli SEED, SEED, 32

	/* align buffer to 128-bits, then fold */
	crc32_refl_align

	crc_fold_loop 32 0 1
	crc32_refl_fold_reduction

	/* handle any remaining excess */
	crc32_refl_excess

	/* sign-extend result */
	sext.w SEED, SEED
	ret

/* precomputed constants */
.poly_refl:
	.dword 0x0000000105ec76f1
.mu:
	.dword 0x4869ec38dea713f1
.k1:
	.dword 0x00000000740eef02
.k2:
	.dword 0x000000009e4addf8
.k3:
	.dword 0x00000000f20c0dfe
.k4:
	.dword 0x000000014cd00bd6
.k5:
	.dword 0x00000000dd45aab8
