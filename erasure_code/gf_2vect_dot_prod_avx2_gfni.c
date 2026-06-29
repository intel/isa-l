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

#include <string.h>
#include <immintrin.h>

#define BCAST_GF8(p) _mm256_set1_epi64x((long long) *(const uint64_t *) (p))
#define AFFINE(s, g) _mm256_gf2p8affine_epi64_epi8((s), (g), 0x00)
#define LOAD256(p)   _mm256_loadu_si256((const __m256i *) (p))

__attribute__((target("avx2,gfni"))) void
gf_2vect_dot_prod_avx2_gfni(int len, int k, unsigned char *g_tbls, unsigned char **src,
                            unsigned char **coding)
{
        unsigned char *d1 = coding[0], *d2 = coding[1];
        const long row_stride = (long) k * 32;
        int pos = 0;

        for (; pos + 96 <= len; pos += 96) {
                __m256i p1l = _mm256_setzero_si256();
                __m256i p2l = _mm256_setzero_si256();
                __m256i p1h = _mm256_setzero_si256();
                __m256i p2h = _mm256_setzero_si256();
                __m256i p1x = _mm256_setzero_si256();
                __m256i p2x = _mm256_setzero_si256();
                unsigned char *tbl = g_tbls;

                for (int i = 0; i < k; i++) {
                        const unsigned char *s = src[i] + pos;
                        __m256i xl = LOAD256(s + 0);
                        __m256i xh = LOAD256(s + 32);
                        __m256i xx = LOAD256(s + 64);
                        __m256i g1 = LOAD256(tbl + 0);
                        __m256i g2 = LOAD256(tbl + row_stride);

                        tbl += 32;
                        p1l = _mm256_xor_si256(p1l, AFFINE(xl, g1));
                        p2l = _mm256_xor_si256(p2l, AFFINE(xl, g2));
                        p1h = _mm256_xor_si256(p1h, AFFINE(xh, g1));
                        p2h = _mm256_xor_si256(p2h, AFFINE(xh, g2));
                        p1x = _mm256_xor_si256(p1x, AFFINE(xx, g1));
                        p2x = _mm256_xor_si256(p2x, AFFINE(xx, g2));
                }
                _mm256_storeu_si256((__m256i *) (d1 + pos + 0), p1l);
                _mm256_storeu_si256((__m256i *) (d2 + pos + 0), p2l);
                _mm256_storeu_si256((__m256i *) (d1 + pos + 32), p1h);
                _mm256_storeu_si256((__m256i *) (d2 + pos + 32), p2h);
                _mm256_storeu_si256((__m256i *) (d1 + pos + 64), p1x);
                _mm256_storeu_si256((__m256i *) (d2 + pos + 64), p2x);
        }

        if (pos + 64 <= len) {
                __m256i p1l = _mm256_setzero_si256();
                __m256i p2l = _mm256_setzero_si256();
                __m256i p1h = _mm256_setzero_si256();
                __m256i p2h = _mm256_setzero_si256();
                unsigned char *tbl = g_tbls;

                for (int i = 0; i < k; i++) {
                        const unsigned char *s = src[i] + pos;
                        __m256i xl = LOAD256(s + 0);
                        __m256i xh = LOAD256(s + 32);
                        __m256i g1 = LOAD256(tbl + 0);
                        __m256i g2 = LOAD256(tbl + row_stride);

                        tbl += 32;
                        p1l = _mm256_xor_si256(p1l, AFFINE(xl, g1));
                        p2l = _mm256_xor_si256(p2l, AFFINE(xl, g2));
                        p1h = _mm256_xor_si256(p1h, AFFINE(xh, g1));
                        p2h = _mm256_xor_si256(p2h, AFFINE(xh, g2));
                }
                _mm256_storeu_si256((__m256i *) (d1 + pos + 0), p1l);
                _mm256_storeu_si256((__m256i *) (d2 + pos + 0), p2l);
                _mm256_storeu_si256((__m256i *) (d1 + pos + 32), p1h);
                _mm256_storeu_si256((__m256i *) (d2 + pos + 32), p2h);
                pos += 64;
        }

        if (pos + 32 <= len) {
                __m256i p1 = _mm256_setzero_si256();
                __m256i p2 = _mm256_setzero_si256();
                unsigned char *tbl = g_tbls;

                for (int i = 0; i < k; i++) {
                        __m256i x = LOAD256(src[i] + pos);
                        __m256i g1 = LOAD256(tbl + 0);
                        __m256i g2 = LOAD256(tbl + row_stride);

                        tbl += 32;
                        p1 = _mm256_xor_si256(p1, AFFINE(x, g1));
                        p2 = _mm256_xor_si256(p2, AFFINE(x, g2));
                }
                _mm256_storeu_si256((__m256i *) (d1 + pos), p1);
                _mm256_storeu_si256((__m256i *) (d2 + pos), p2);
                pos += 32;
        }

        if (pos < len) {
                int rem = len - pos;
                unsigned char buf[32] __attribute__ /**/ ((aligned(32))) = { 0 };
                __m256i p1 = _mm256_setzero_si256();
                __m256i p2 = _mm256_setzero_si256();
                unsigned char *tbl = g_tbls;

                for (int i = 0; i < k; i++) {
                        memcpy(buf, src[i] + pos, rem);
                        __m256i x = _mm256_load_si256((const __m256i *) buf);
                        __m256i g1 = LOAD256(tbl + 0);
                        __m256i g2 = LOAD256(tbl + row_stride);

                        tbl += 32;
                        p1 = _mm256_xor_si256(p1, AFFINE(x, g1));
                        p2 = _mm256_xor_si256(p2, AFFINE(x, g2));
                }
                _mm256_store_si256((__m256i *) buf, p1);
                memcpy(d1 + pos, buf, rem);
                _mm256_store_si256((__m256i *) buf, p2);
                memcpy(d2 + pos, buf, rem);
        }

        _mm256_zeroupper();
}
