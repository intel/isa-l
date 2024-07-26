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

/* uint16_t crc16_t10dif(uint16_t init_crc, uint8_t *buf, uint64_t len) */
.text
.align 1
.global crc16_t10dif
.type crc16_t10dif, %function
crc16_t10dif:
	/* load precomputed constants */
	ld POLY, .poly
	ld MU, .mu

	/* shift 16-bit seed into upper nibble */
	slli SEED, SEED, 16

	/* align and fold as though we're calculating a 32-bit crc */
	crc32_norm_align

	crc_fold_loop 32 1 0
	crc32_norm_fold_reduction

	crc32_norm_excess

	/* shift back down result */
	srli SEED, SEED, 16
	ret

/* precomputed constants */
.poly:
	.dword 0x000000018bb70000
.mu:
	.dword 0x00000001f65a57f8
.k1:
	.dword 0x00000000371d0000
.k2:
	.dword 0x0000000087e70000
.k3:
	.dword 0x000000004c1a0000
.k4:
	.dword 0x00000000fb0b0000
.k5:
	.dword 0x000000002d560000
.k6:
	.dword 0x0000000013680000


/* uint16_t crc16_t10dif_copy(uint16_t seed, uint8_t * dst, uint8_t * src, uint64_t len) */
/* in addition to calculating crc, also copies from src to dst */
.text
.align 1
.global crc16_t10dif_copy
.type crc16_t10dif_copy, %function
crc16_t10dif_copy:
	beqz a3, .memcpy_done
	add t0, a2, a3
	mv t1, a2
.memcpy_loop:
	lb t2, 0(t1)
	sb t2, 0(a1)
	addi t1, t1, 1
	addi a1, a1, 1
	bne t1, t0, .memcpy_loop

.memcpy_done:
	/* tail-call crc function */
	mv a1, a2
	mv a2, a3
	tail crc16_t10dif
