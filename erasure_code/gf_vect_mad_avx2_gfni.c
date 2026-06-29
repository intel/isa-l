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
 * gf_vect_mad_avx2_gfni(len, vec, vec_i, gftbls, src, dest)
 *
 * dest[i] ^= mul(gftbls[vec_i], src[i]) for i in [0,len).
 * 96B / 64B / 32B / <32B body, matching the prior NASM kernel.
 */

#include <stdint.h>
#include <string.h>
#include <immintrin.h>

#define BCAST_GF8(p) _mm256_set1_epi64x((long long) *(const uint64_t *) (p))
#define AFFINE(s, g) _mm256_gf2p8affine_epi64_epi8((s), (g), 0x00)
#define LOAD256(p)   _mm256_loadu_si256((const __m256i *) (p))

__attribute__((target("avx2,gfni"))) void
gf_vect_mad_avx2_gfni(int len, int vec, int vec_i, unsigned char *gftbls, unsigned char *src,
                      unsigned char *dest)
{
        (void) vec;
        const __m256i gft = BCAST_GF8(gftbls + vec_i * 32);

        int pos = 0;

        for (; pos + 96 <= len; pos += 96) {
                __m256i x0l = LOAD256(src + pos + 0);
                __m256i x0h = LOAD256(src + pos + 32);
                __m256i x0x = LOAD256(src + pos + 64);
                __m256i d0l = LOAD256(dest + pos + 0);
                __m256i d0h = LOAD256(dest + pos + 32);
                __m256i d0x = LOAD256(dest + pos + 64);

                d0l = _mm256_xor_si256(d0l, AFFINE(x0l, gft));
                d0h = _mm256_xor_si256(d0h, AFFINE(x0h, gft));
                d0x = _mm256_xor_si256(d0x, AFFINE(x0x, gft));

                _mm256_storeu_si256((__m256i *) (dest + pos + 0), d0l);
                _mm256_storeu_si256((__m256i *) (dest + pos + 32), d0h);
                _mm256_storeu_si256((__m256i *) (dest + pos + 64), d0x);
        }

        if (pos + 64 <= len) {
                __m256i x0l = LOAD256(src + pos + 0);
                __m256i x0h = LOAD256(src + pos + 32);
                __m256i d0l = LOAD256(dest + pos + 0);
                __m256i d0h = LOAD256(dest + pos + 32);

                d0l = _mm256_xor_si256(d0l, AFFINE(x0l, gft));
                d0h = _mm256_xor_si256(d0h, AFFINE(x0h, gft));

                _mm256_storeu_si256((__m256i *) (dest + pos + 0), d0l);
                _mm256_storeu_si256((__m256i *) (dest + pos + 32), d0h);
                pos += 64;
        }

        if (pos + 32 <= len) {
                __m256i x0 = LOAD256(src + pos);
                __m256i d0 = LOAD256(dest + pos);

                d0 = _mm256_xor_si256(d0, AFFINE(x0, gft));
                _mm256_storeu_si256((__m256i *) (dest + pos), d0);
                pos += 32;
        }

        if (pos < len) {
                int rem = len - pos;
                unsigned char sbuf[32] __attribute__ /**/ ((aligned(32))) = { 0 };
                unsigned char dbuf[32] __attribute__ /**/ ((aligned(32))) = { 0 };

                memcpy(sbuf, src + pos, rem);
                memcpy(dbuf, dest + pos, rem);
                __m256i x0 = _mm256_load_si256((const __m256i *) sbuf);
                __m256i d0 = _mm256_load_si256((const __m256i *) dbuf);

                d0 = _mm256_xor_si256(d0, AFFINE(x0, gft));
                _mm256_store_si256((__m256i *) dbuf, d0);
                memcpy(dest + pos, dbuf, rem);
        }

        _mm256_zeroupper();
}
