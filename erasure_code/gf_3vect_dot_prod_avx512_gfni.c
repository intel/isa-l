// SPDX-License-Identifier: BSD-3-Clause
/*
 * Copyright(c) 2026 Intel Corporation All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *   * Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 *   * Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in
 *     the documentation and/or other materials provided with the
 *     distribution.
 *   * Neither the name of Intel Corporation nor the names of its
 *     contributors may be used to endorse or promote products derived
 *     from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <stdint.h>
#include <immintrin.h>

#define AFFINE(s, g)        _mm512_gf2p8affine_epi64_epi8((s), (g), 0x00)
#define LOAD512(p)          _mm512_loadu_si512((const void *) (p))
#define TAIL_MASK(len, pos) ((__mmask64) (((uint64_t) 1 << ((len) - (pos))) - 1))

/*
 * Embedded-broadcast GF multiply-accumulate for the dot_prod hot loop.
 *
 * acc ^= gf2p8affine(src, broadcast8(*coeff)).  The coefficient is read
 * straight from the gftbls memory as an EVEX {1to8} embedded-broadcast operand
 * of vgf2p8affineqb, folding the 8-byte broadcast into the affine's own load
 * micro-op.  This deliberately avoids letting the compiler lower the broadcast
 * to a GPR-source `vpbroadcastq %gpr,zmm` (+ a scalar `movq` load), which on
 * Sapphire Rapids issues on the vector-ALU Port5 and made the multi-vect C
 * encode ~25% slower than the NASM baseline (which keeps the broadcast on the
 * load ports).  gcc/clang will NOT emit {1to8} from _mm512_set1_epi64()+affine
 * (verified), so this must be inline asm.  A distinct scratch temp per call
 * also keeps the N independent affines off a single shared dependency chain.
 */
#define AFFINE_XOR_BCAST(acc, s, coeff)                                                        \
        do {                                                                                  \
                __m512i _t;                                                                    \
                __asm__("vgf2p8affineqb $0x00, (%[c])%{1to8%}, %[src], %[t]\n\t"               \
                        "vpxorq %[t], %[p], %[p]"                                              \
                        : [p] "+v"(acc), [t] "=&v"(_t)                                         \
                        : [src] "v"(s), [c] "r"(coeff)                                         \
                        :);                                                                    \
        } while (0)

__attribute__((target("avx512f,avx512bw,gfni"))) void
gf_3vect_dot_prod_avx512_gfni(int len, int k, unsigned char *g_tbls, unsigned char **src,
                              unsigned char **coding)
{
        unsigned char *d1 = coding[0], *d2 = coding[1];
        unsigned char *d3 = coding[2];
        const long row_stride = (long) k * 32;
        int pos = 0;

        for (; pos + 64 <= len; pos += 64) {
                __m512i p1 = _mm512_setzero_si512();
                __m512i p2 = _mm512_setzero_si512();
                __m512i p3 = _mm512_setzero_si512();
                unsigned char *tbl = g_tbls;

                for (int i = 0; i < k; i++) {
                        __m512i s = LOAD512(src[i] + pos);
                        AFFINE_XOR_BCAST(p1, s, tbl + 0);
                        AFFINE_XOR_BCAST(p2, s, tbl + 1 * row_stride);
                        AFFINE_XOR_BCAST(p3, s, tbl + 2 * row_stride);
                        tbl += 32;
                }
                _mm512_storeu_si512((void *) (d1 + pos), p1);
                _mm512_storeu_si512((void *) (d2 + pos), p2);
                _mm512_storeu_si512((void *) (d3 + pos), p3);
        }

        if (pos < len) {
                const __mmask64 m = TAIL_MASK(len, pos);
                __m512i p1 = _mm512_setzero_si512();
                __m512i p2 = _mm512_setzero_si512();
                __m512i p3 = _mm512_setzero_si512();
                unsigned char *tbl = g_tbls;

                for (int i = 0; i < k; i++) {
                        __m512i s = _mm512_maskz_loadu_epi8(m, src[i] + pos);
                        AFFINE_XOR_BCAST(p1, s, tbl + 0);
                        AFFINE_XOR_BCAST(p2, s, tbl + 1 * row_stride);
                        AFFINE_XOR_BCAST(p3, s, tbl + 2 * row_stride);
                        tbl += 32;
                }
                _mm512_mask_storeu_epi8(d1 + pos, m, p1);
                _mm512_mask_storeu_epi8(d2 + pos, m, p2);
                _mm512_mask_storeu_epi8(d3 + pos, m, p3);
        }
}
