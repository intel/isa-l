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
 * gf_vect_dot_prod_avx2_gfni(len, k, g_tbls, src, dest)
 *
 * dest[0..len) = XOR over j in [0,k) of mul(g_tbls[j], src[j][0..len)).
 *
 * Uses 96B / 64B / 32B unrolled body matching the prior NASM kernel; the
 * sub-32B tail is staged through a 32-byte stack buffer.
 */

#include <string.h>
#include <immintrin.h>

#define BCAST_GF8(p) _mm256_set1_epi64x((long long) *(const uint64_t *) (p))
#define AFFINE(s, g) _mm256_gf2p8affine_epi64_epi8((s), (g), 0x00)
#define LOAD256(p)   _mm256_loadu_si256((const __m256i *) (p))

__attribute__((target("avx2,gfni"))) void
gf_vect_dot_prod_avx2_gfni(int len, int k, unsigned char *g_tbls, unsigned char **src,
                           unsigned char *dest)
{
        int pos = 0;

        for (; pos + 96 <= len; pos += 96) {
                __m256i p1l = _mm256_setzero_si256();
                __m256i p1h = _mm256_setzero_si256();
                __m256i p1x = _mm256_setzero_si256();
                unsigned char *tbl = g_tbls;

                for (int i = 0; i < k; i++) {
                        const unsigned char *s = src[i] + pos;
                        __m256i x0l = LOAD256(s + 0);
                        __m256i x0h = LOAD256(s + 32);
                        __m256i x0x = LOAD256(s + 64);
                        __m256i gft = _mm256_loadu_si256((const __m256i *) tbl);

                        tbl += 32;

                        p1l = _mm256_xor_si256(p1l, AFFINE(x0l, gft));
                        p1h = _mm256_xor_si256(p1h, AFFINE(x0h, gft));
                        p1x = _mm256_xor_si256(p1x, AFFINE(x0x, gft));
                }
                _mm256_storeu_si256((__m256i *) (dest + pos + 0), p1l);
                _mm256_storeu_si256((__m256i *) (dest + pos + 32), p1h);
                _mm256_storeu_si256((__m256i *) (dest + pos + 64), p1x);
        }

        if (pos + 64 <= len) {
                __m256i p1l = _mm256_setzero_si256();
                __m256i p1h = _mm256_setzero_si256();
                unsigned char *tbl = g_tbls;

                for (int i = 0; i < k; i++) {
                        const unsigned char *s = src[i] + pos;
                        __m256i x0l = LOAD256(s + 0);
                        __m256i x0h = LOAD256(s + 32);
                        __m256i gft = _mm256_loadu_si256((const __m256i *) tbl);

                        tbl += 32;

                        p1l = _mm256_xor_si256(p1l, AFFINE(x0l, gft));
                        p1h = _mm256_xor_si256(p1h, AFFINE(x0h, gft));
                }
                _mm256_storeu_si256((__m256i *) (dest + pos + 0), p1l);
                _mm256_storeu_si256((__m256i *) (dest + pos + 32), p1h);
                pos += 64;
        }

        if (pos + 32 <= len) {
                __m256i p1 = _mm256_setzero_si256();
                unsigned char *tbl = g_tbls;

                for (int i = 0; i < k; i++) {
                        __m256i x0 = LOAD256(src[i] + pos);
                        __m256i gft = _mm256_loadu_si256((const __m256i *) tbl);

                        tbl += 32;
                        p1 = _mm256_xor_si256(p1, AFFINE(x0, gft));
                }
                _mm256_storeu_si256((__m256i *) (dest + pos), p1);
                pos += 32;
        }

        if (pos < len) {
                int rem = len - pos;
                unsigned char buf[32] __attribute__ /**/ ((aligned(32))) = { 0 };
                __m256i p1 = _mm256_setzero_si256();
                unsigned char *tbl = g_tbls;

                for (int i = 0; i < k; i++) {
                        memcpy(buf, src[i] + pos, rem);
                        __m256i x0 = _mm256_load_si256((const __m256i *) buf);
                        __m256i gft = _mm256_loadu_si256((const __m256i *) tbl);

                        tbl += 32;
                        p1 = _mm256_xor_si256(p1, AFFINE(x0, gft));
                }
                _mm256_store_si256((__m256i *) buf, p1);
                memcpy(dest + pos, buf, rem);
        }

        _mm256_zeroupper();
}
