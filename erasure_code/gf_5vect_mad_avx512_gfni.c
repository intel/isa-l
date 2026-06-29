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
#include <immintrin.h>

__attribute__((target("avx512f,avx512bw,gfni"))) void
gf_5vect_mad_avx512_gfni(int len, int vec, int vec_i, unsigned char *gftbls, unsigned char *src,
                         unsigned char **dest)
{
        const long row_stride = (long) vec * 32;
        unsigned char *base = gftbls + (long) vec_i * 32;
        unsigned char *d1_p = dest[0], *d2_p = dest[1], *d3_p = dest[2], *d4_p = dest[3],
                      *d5_p = dest[4];
        __m512i gft1 = _mm512_set1_epi64((long long) *(const uint64_t *) (base + 0 * row_stride));
        __m512i gft2 = _mm512_set1_epi64((long long) *(const uint64_t *) (base + 1 * row_stride));
        __m512i gft3 = _mm512_set1_epi64((long long) *(const uint64_t *) (base + 2 * row_stride));
        __m512i gft4 = _mm512_set1_epi64((long long) *(const uint64_t *) (base + 3 * row_stride));
        __m512i gft5 = _mm512_set1_epi64((long long) *(const uint64_t *) (base + 4 * row_stride));

        int pos = 0;
        for (; pos + 64 <= len; pos += 64) {
                __m512i s = _mm512_loadu_si512((const void *) (src + pos));
                __m512i d1 = _mm512_loadu_si512((const void *) (d1_p + pos));
                __m512i d2 = _mm512_loadu_si512((const void *) (d2_p + pos));
                __m512i d3 = _mm512_loadu_si512((const void *) (d3_p + pos));
                __m512i d4 = _mm512_loadu_si512((const void *) (d4_p + pos));
                __m512i d5 = _mm512_loadu_si512((const void *) (d5_p + pos));
                d1 = _mm512_xor_si512(d1, _mm512_gf2p8affine_epi64_epi8(s, gft1, 0x00));
                d2 = _mm512_xor_si512(d2, _mm512_gf2p8affine_epi64_epi8(s, gft2, 0x00));
                d3 = _mm512_xor_si512(d3, _mm512_gf2p8affine_epi64_epi8(s, gft3, 0x00));
                d4 = _mm512_xor_si512(d4, _mm512_gf2p8affine_epi64_epi8(s, gft4, 0x00));
                d5 = _mm512_xor_si512(d5, _mm512_gf2p8affine_epi64_epi8(s, gft5, 0x00));
                _mm512_storeu_si512((void *) (d1_p + pos), d1);
                _mm512_storeu_si512((void *) (d2_p + pos), d2);
                _mm512_storeu_si512((void *) (d3_p + pos), d3);
                _mm512_storeu_si512((void *) (d4_p + pos), d4);
                _mm512_storeu_si512((void *) (d5_p + pos), d5);
        }

        if (pos < len) {
                const __mmask64 m = (__mmask64) (((uint64_t) 1 << (len - pos)) - 1);
                __m512i s = _mm512_maskz_loadu_epi8(m, src + pos);
                __m512i d1 = _mm512_maskz_loadu_epi8(m, d1_p + pos);
                __m512i d2 = _mm512_maskz_loadu_epi8(m, d2_p + pos);
                __m512i d3 = _mm512_maskz_loadu_epi8(m, d3_p + pos);
                __m512i d4 = _mm512_maskz_loadu_epi8(m, d4_p + pos);
                __m512i d5 = _mm512_maskz_loadu_epi8(m, d5_p + pos);
                d1 = _mm512_xor_si512(d1, _mm512_gf2p8affine_epi64_epi8(s, gft1, 0x00));
                d2 = _mm512_xor_si512(d2, _mm512_gf2p8affine_epi64_epi8(s, gft2, 0x00));
                d3 = _mm512_xor_si512(d3, _mm512_gf2p8affine_epi64_epi8(s, gft3, 0x00));
                d4 = _mm512_xor_si512(d4, _mm512_gf2p8affine_epi64_epi8(s, gft4, 0x00));
                d5 = _mm512_xor_si512(d5, _mm512_gf2p8affine_epi64_epi8(s, gft5, 0x00));
                _mm512_mask_storeu_epi8(d1_p + pos, m, d1);
                _mm512_mask_storeu_epi8(d2_p + pos, m, d2);
                _mm512_mask_storeu_epi8(d3_p + pos, m, d3);
                _mm512_mask_storeu_epi8(d4_p + pos, m, d4);
                _mm512_mask_storeu_epi8(d5_p + pos, m, d5);
        }
}
