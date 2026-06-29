/**********************************************************************
  Copyright(c) 2026 Intel Corporation All rights reserved.

  Redistribution and use in source and binary forms, with or without
  modification, are permitted provided that the following conditions
  are met:
    * Redistributions of source code must retain the above copyright
      notice, this list of conditions and the following disclaimer.
    * Redistributions in binary form must reproduce the above copyright
      notice, this list of conditions and the following disclaimer in
      the documentation and/or other materials provided with the
      distribution.
    * Neither the name of Intel Corporation nor the names of its
      contributors may be used to endorse or promote products derived
      from this software without specific prior written permission.

  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
  "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
  LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
  A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
  OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
  SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
  LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
  DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
  THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
  (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
  OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
**********************************************************************/

#include <stdint.h>
#include <string.h>
#include <immintrin.h>

#define AFFINE(s, g) _mm256_gf2p8affine_epi64_epi8((s), (g), 0x00)

__attribute__((target("avx2,gfni"))) void
gf_5vect_mad_avx2_gfni(int len, int vec, int vec_i, unsigned char *gftbls, unsigned char *src,
                       unsigned char **dest)
{
        const long row_stride = (long) vec * 32;
        unsigned char *base = gftbls + (long) vec_i * 32;
        unsigned char *dp1 = dest[0], *dp2 = dest[1], *dp3 = dest[2], *dp4 = dest[3],
                      *dp5 = dest[4];
        __m256i gft1 = _mm256_set1_epi64x((long long) *(const uint64_t *) (base + 0 * row_stride));
        __m256i gft2 = _mm256_set1_epi64x((long long) *(const uint64_t *) (base + 1 * row_stride));
        __m256i gft3 = _mm256_set1_epi64x((long long) *(const uint64_t *) (base + 2 * row_stride));
        __m256i gft4 = _mm256_set1_epi64x((long long) *(const uint64_t *) (base + 3 * row_stride));
        __m256i gft5 = _mm256_set1_epi64x((long long) *(const uint64_t *) (base + 4 * row_stride));

        int pos = 0;
        for (; pos + 96 <= len; pos += 96) {
                __m256i xl = _mm256_loadu_si256((const __m256i *) (src + pos + 0));
                __m256i xh = _mm256_loadu_si256((const __m256i *) (src + pos + 32));
                __m256i xx = _mm256_loadu_si256((const __m256i *) (src + pos + 64));
                __m256i d1l = _mm256_loadu_si256((const __m256i *) (dp1 + pos + 0));
                __m256i d2l = _mm256_loadu_si256((const __m256i *) (dp2 + pos + 0));
                __m256i d3l = _mm256_loadu_si256((const __m256i *) (dp3 + pos + 0));
                __m256i d4l = _mm256_loadu_si256((const __m256i *) (dp4 + pos + 0));
                __m256i d5l = _mm256_loadu_si256((const __m256i *) (dp5 + pos + 0));
                d1l = _mm256_xor_si256(d1l, AFFINE(xl, gft1));
                d2l = _mm256_xor_si256(d2l, AFFINE(xl, gft2));
                d3l = _mm256_xor_si256(d3l, AFFINE(xl, gft3));
                d4l = _mm256_xor_si256(d4l, AFFINE(xl, gft4));
                d5l = _mm256_xor_si256(d5l, AFFINE(xl, gft5));
                _mm256_storeu_si256((__m256i *) (dp1 + pos + 0), d1l);
                _mm256_storeu_si256((__m256i *) (dp2 + pos + 0), d2l);
                _mm256_storeu_si256((__m256i *) (dp3 + pos + 0), d3l);
                _mm256_storeu_si256((__m256i *) (dp4 + pos + 0), d4l);
                _mm256_storeu_si256((__m256i *) (dp5 + pos + 0), d5l);
                __m256i d1h = _mm256_loadu_si256((const __m256i *) (dp1 + pos + 32));
                __m256i d2h = _mm256_loadu_si256((const __m256i *) (dp2 + pos + 32));
                __m256i d3h = _mm256_loadu_si256((const __m256i *) (dp3 + pos + 32));
                __m256i d4h = _mm256_loadu_si256((const __m256i *) (dp4 + pos + 32));
                __m256i d5h = _mm256_loadu_si256((const __m256i *) (dp5 + pos + 32));
                d1h = _mm256_xor_si256(d1h, AFFINE(xh, gft1));
                d2h = _mm256_xor_si256(d2h, AFFINE(xh, gft2));
                d3h = _mm256_xor_si256(d3h, AFFINE(xh, gft3));
                d4h = _mm256_xor_si256(d4h, AFFINE(xh, gft4));
                d5h = _mm256_xor_si256(d5h, AFFINE(xh, gft5));
                _mm256_storeu_si256((__m256i *) (dp1 + pos + 32), d1h);
                _mm256_storeu_si256((__m256i *) (dp2 + pos + 32), d2h);
                _mm256_storeu_si256((__m256i *) (dp3 + pos + 32), d3h);
                _mm256_storeu_si256((__m256i *) (dp4 + pos + 32), d4h);
                _mm256_storeu_si256((__m256i *) (dp5 + pos + 32), d5h);
                __m256i d1x = _mm256_loadu_si256((const __m256i *) (dp1 + pos + 64));
                __m256i d2x = _mm256_loadu_si256((const __m256i *) (dp2 + pos + 64));
                __m256i d3x = _mm256_loadu_si256((const __m256i *) (dp3 + pos + 64));
                __m256i d4x = _mm256_loadu_si256((const __m256i *) (dp4 + pos + 64));
                __m256i d5x = _mm256_loadu_si256((const __m256i *) (dp5 + pos + 64));
                d1x = _mm256_xor_si256(d1x, AFFINE(xx, gft1));
                d2x = _mm256_xor_si256(d2x, AFFINE(xx, gft2));
                d3x = _mm256_xor_si256(d3x, AFFINE(xx, gft3));
                d4x = _mm256_xor_si256(d4x, AFFINE(xx, gft4));
                d5x = _mm256_xor_si256(d5x, AFFINE(xx, gft5));
                _mm256_storeu_si256((__m256i *) (dp1 + pos + 64), d1x);
                _mm256_storeu_si256((__m256i *) (dp2 + pos + 64), d2x);
                _mm256_storeu_si256((__m256i *) (dp3 + pos + 64), d3x);
                _mm256_storeu_si256((__m256i *) (dp4 + pos + 64), d4x);
                _mm256_storeu_si256((__m256i *) (dp5 + pos + 64), d5x);
        }

        if (pos + 64 <= len) {
                __m256i xl = _mm256_loadu_si256((const __m256i *) (src + pos + 0));
                __m256i xh = _mm256_loadu_si256((const __m256i *) (src + pos + 32));
                __m256i d1l = _mm256_loadu_si256((const __m256i *) (dp1 + pos + 0));
                __m256i d2l = _mm256_loadu_si256((const __m256i *) (dp2 + pos + 0));
                __m256i d3l = _mm256_loadu_si256((const __m256i *) (dp3 + pos + 0));
                __m256i d4l = _mm256_loadu_si256((const __m256i *) (dp4 + pos + 0));
                __m256i d5l = _mm256_loadu_si256((const __m256i *) (dp5 + pos + 0));
                d1l = _mm256_xor_si256(d1l, AFFINE(xl, gft1));
                d2l = _mm256_xor_si256(d2l, AFFINE(xl, gft2));
                d3l = _mm256_xor_si256(d3l, AFFINE(xl, gft3));
                d4l = _mm256_xor_si256(d4l, AFFINE(xl, gft4));
                d5l = _mm256_xor_si256(d5l, AFFINE(xl, gft5));
                _mm256_storeu_si256((__m256i *) (dp1 + pos + 0), d1l);
                _mm256_storeu_si256((__m256i *) (dp2 + pos + 0), d2l);
                _mm256_storeu_si256((__m256i *) (dp3 + pos + 0), d3l);
                _mm256_storeu_si256((__m256i *) (dp4 + pos + 0), d4l);
                _mm256_storeu_si256((__m256i *) (dp5 + pos + 0), d5l);
                __m256i d1h = _mm256_loadu_si256((const __m256i *) (dp1 + pos + 32));
                __m256i d2h = _mm256_loadu_si256((const __m256i *) (dp2 + pos + 32));
                __m256i d3h = _mm256_loadu_si256((const __m256i *) (dp3 + pos + 32));
                __m256i d4h = _mm256_loadu_si256((const __m256i *) (dp4 + pos + 32));
                __m256i d5h = _mm256_loadu_si256((const __m256i *) (dp5 + pos + 32));
                d1h = _mm256_xor_si256(d1h, AFFINE(xh, gft1));
                d2h = _mm256_xor_si256(d2h, AFFINE(xh, gft2));
                d3h = _mm256_xor_si256(d3h, AFFINE(xh, gft3));
                d4h = _mm256_xor_si256(d4h, AFFINE(xh, gft4));
                d5h = _mm256_xor_si256(d5h, AFFINE(xh, gft5));
                _mm256_storeu_si256((__m256i *) (dp1 + pos + 32), d1h);
                _mm256_storeu_si256((__m256i *) (dp2 + pos + 32), d2h);
                _mm256_storeu_si256((__m256i *) (dp3 + pos + 32), d3h);
                _mm256_storeu_si256((__m256i *) (dp4 + pos + 32), d4h);
                _mm256_storeu_si256((__m256i *) (dp5 + pos + 32), d5h);
                pos += 64;
        }

        if (pos + 32 <= len) {
                __m256i x = _mm256_loadu_si256((const __m256i *) (src + pos));
                __m256i d1 = _mm256_loadu_si256((const __m256i *) (dp1 + pos + 0));
                __m256i d2 = _mm256_loadu_si256((const __m256i *) (dp2 + pos + 0));
                __m256i d3 = _mm256_loadu_si256((const __m256i *) (dp3 + pos + 0));
                __m256i d4 = _mm256_loadu_si256((const __m256i *) (dp4 + pos + 0));
                __m256i d5 = _mm256_loadu_si256((const __m256i *) (dp5 + pos + 0));
                d1 = _mm256_xor_si256(d1, AFFINE(x, gft1));
                d2 = _mm256_xor_si256(d2, AFFINE(x, gft2));
                d3 = _mm256_xor_si256(d3, AFFINE(x, gft3));
                d4 = _mm256_xor_si256(d4, AFFINE(x, gft4));
                d5 = _mm256_xor_si256(d5, AFFINE(x, gft5));
                _mm256_storeu_si256((__m256i *) (dp1 + pos + 0), d1);
                _mm256_storeu_si256((__m256i *) (dp2 + pos + 0), d2);
                _mm256_storeu_si256((__m256i *) (dp3 + pos + 0), d3);
                _mm256_storeu_si256((__m256i *) (dp4 + pos + 0), d4);
                _mm256_storeu_si256((__m256i *) (dp5 + pos + 0), d5);
                pos += 32;
        }

        if (pos < len) {
                int rem = len - pos;
                unsigned char sbuf[32] __attribute__((aligned(32))) = { 0 };
                unsigned char dbuf[32] __attribute__((aligned(32)));
                memcpy(sbuf, src + pos, rem);
                __m256i x = _mm256_load_si256((const __m256i *) sbuf);
                memset(dbuf, 0, 32);
                memcpy(dbuf, dp1 + pos, rem);
                {
                        __m256i d = _mm256_load_si256((const __m256i *) dbuf);
                        d = _mm256_xor_si256(d, AFFINE(x, gft1));
                        _mm256_store_si256((__m256i *) dbuf, d);
                }
                memcpy(dp1 + pos, dbuf, rem);
                memset(dbuf, 0, 32);
                memcpy(dbuf, dp2 + pos, rem);
                {
                        __m256i d = _mm256_load_si256((const __m256i *) dbuf);
                        d = _mm256_xor_si256(d, AFFINE(x, gft2));
                        _mm256_store_si256((__m256i *) dbuf, d);
                }
                memcpy(dp2 + pos, dbuf, rem);
                memset(dbuf, 0, 32);
                memcpy(dbuf, dp3 + pos, rem);
                {
                        __m256i d = _mm256_load_si256((const __m256i *) dbuf);
                        d = _mm256_xor_si256(d, AFFINE(x, gft3));
                        _mm256_store_si256((__m256i *) dbuf, d);
                }
                memcpy(dp3 + pos, dbuf, rem);
                memset(dbuf, 0, 32);
                memcpy(dbuf, dp4 + pos, rem);
                {
                        __m256i d = _mm256_load_si256((const __m256i *) dbuf);
                        d = _mm256_xor_si256(d, AFFINE(x, gft4));
                        _mm256_store_si256((__m256i *) dbuf, d);
                }
                memcpy(dp4 + pos, dbuf, rem);
                memset(dbuf, 0, 32);
                memcpy(dbuf, dp5 + pos, rem);
                {
                        __m256i d = _mm256_load_si256((const __m256i *) dbuf);
                        d = _mm256_xor_si256(d, AFFINE(x, gft5));
                        _mm256_store_si256((__m256i *) dbuf, d);
                }
                memcpy(dp5 + pos, dbuf, rem);
        }

        _mm256_zeroupper();
}
