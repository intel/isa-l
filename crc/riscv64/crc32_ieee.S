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

#include "crc32_norm_common_clmul.h"

/* uint32_t crc32_ieee(uint32_t init_crc, uint8_t *buf, uint64_t len) */
.text
.align 1
.global crc32_ieee
.type crc32_ieee, %function
crc32_ieee:
	/* load precomputed constants */
	ld POLY, .poly
	ld MU, .mu

	/* reflect and zero-extend seed (riscv calling convention has uint32_t
         * passed in and returned sign-extended)
         */
	not SEED, SEED
	slli SEED, SEED, 32
	srli SEED, SEED, 32

	/* align and fold buffer */
	crc32_norm_align

	crc_fold_loop 32 1 0
	crc32_norm_fold_reduction

	crc32_norm_excess

	/* sign-extend and reflect result */
	sext.w SEED, SEED
	not SEED, SEED
	ret

/* precomputed constants */
.poly:
	.dword 0x0000000104c11db7
.mu:
	.dword 0x0000000104d101df
.k1:
	.dword 0x000000008833794c
.k2:
	.dword 0x00000000e6228b11
.k3:
	.dword 0x00000000c5b9cd4c
.k4:
	.dword 0x00000000e8a45605
.k5:
	.dword 0x00000000f200aa66
.k6:
	.dword 0x00000000490d678d
