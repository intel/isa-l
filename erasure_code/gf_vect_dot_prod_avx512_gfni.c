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

/*
 * gf_vect_dot_prod_avx512_gfni(len, k, g_tbls, src, dest)
 *
 * Compute dest[0..len) = XOR over j in [0,k) of mul(g_tbls[j], src[j][0..len)).
 * g_tbls is laid out as k entries of 32 bytes each; only the first 8 bytes
 * (the GF(2^8) affine matrix) are read, and broadcast to all 8 zmm lanes.
 *
 * Tail (len % 64) is handled with a byte-granular k-mask load/store.
 */

#include <stdint.h>
#include <immintrin.h>

#define BCAST_GF8(p)        _mm512_set1_epi64((long long) *(const uint64_t *) (p))
#define AFFINE(s, g)        _mm512_gf2p8affine_epi64_epi8((s), (g), 0x00)
#define LOAD512(p)          _mm512_loadu_si512((const void *) (p))
#define TAIL_MASK(len, pos) ((__mmask64) (((uint64_t) 1 << ((len) - (pos))) - 1))

__attribute__((target("avx512f,avx512bw,gfni"))) void
gf_vect_dot_prod_avx512_gfni(int len, int k, unsigned char *g_tbls, unsigned char **src,
                             unsigned char *dest)
{
        int pos = 0;

        for (; pos + 64 <= len; pos += 64) {
                __m512i p1 = _mm512_setzero_si512();
                unsigned char *tbl = g_tbls;

                for (int i = 0; i < k; i++) {
                        __m512i s = LOAD512(src[i] + pos);
                        __m512i gft = BCAST_GF8(tbl);
                        p1 = _mm512_xor_si512(p1, AFFINE(s, gft));
                        tbl += 32;
                }
                _mm512_storeu_si512((void *) (dest + pos), p1);
        }

        if (pos < len) {
                const __mmask64 m = TAIL_MASK(len, pos);
                __m512i p1 = _mm512_setzero_si512();
                unsigned char *tbl = g_tbls;

                for (int i = 0; i < k; i++) {
                        __m512i s = _mm512_maskz_loadu_epi8(m, src[i] + pos);
                        __m512i gft = BCAST_GF8(tbl);
                        p1 = _mm512_xor_si512(p1, AFFINE(s, gft));
                        tbl += 32;
                }
                _mm512_mask_storeu_epi8(dest + pos, m, p1);
        }
}
