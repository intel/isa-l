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

/* uint32_t crc32_gzip_refl(uint32_t seed, uint8_t *buf, uint64_t len) */
.text
.align 1
.global crc32_gzip_refl
.type crc32_gzip_refl, %function
crc32_gzip_refl:
	/* load precomputed constants */
	ld POLY, .poly_refl
	ld MU, .mu

	/* invert and zero-extend seed */
	not SEED, SEED
	slli SEED, SEED, 32
	srli SEED, SEED, 32

	/* align buffer to 128-bits, then fold */
	crc32_refl_align

	crc_fold_loop 32 0 1
	crc32_refl_fold_reduction

	/* handle any excess */
	crc32_refl_excess

	/* sign-extend and reflect result */
	sext.w SEED, SEED
	not SEED, SEED
	ret

/* precomputed constants */
.poly_refl:
	.dword 0x00000001db710641
.mu:
	.dword 0xb4e5b025f7011641
.k1:
	.dword 0x0000000154442bd4
.k2:
	.dword 0x00000001c6e41596
.k3:
	.dword 0x00000001751997d0
.k4:
	.dword 0x00000000ccaa009e
.k5:
	.dword 0x0000000163cd6124
