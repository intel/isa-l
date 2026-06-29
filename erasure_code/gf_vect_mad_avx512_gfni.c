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

/*
 * gf_vect_mad_avx512_gfni(len, vec, vec_i, gftbls, src, dest)
 *
 * Multiply-add update: dest[i] ^= mul(gftbls[vec_i], src[i]) for i in [0,len).
 * Only the affine matrix at gftbls[vec_i*32] is consulted.
 *
 * vec is the source count k for the parent ec_encode_data_update_avx512_gfni
 * dispatch; this single-vect kernel ignores it.
 */

#include <stdint.h>
#include <immintrin.h>

__attribute__((target("avx512f,avx512bw,gfni"))) void
gf_vect_mad_avx512_gfni(int len, int vec, int vec_i, unsigned char *gftbls, unsigned char *src,
                        unsigned char *dest)
{
        (void) vec;
        const __m512i gft =
                _mm512_set1_epi64((long long) *(const uint64_t *) (gftbls + vec_i * 32));

        int pos = 0;
        for (; pos + 64 <= len; pos += 64) {
                __m512i s = _mm512_loadu_si512((const void *) (src + pos));
                __m512i d = _mm512_loadu_si512((const void *) (dest + pos));
                d = _mm512_xor_si512(d, _mm512_gf2p8affine_epi64_epi8(s, gft, 0x00));
                _mm512_storeu_si512((void *) (dest + pos), d);
        }

        if (pos < len) {
                const __mmask64 m = (__mmask64) (((uint64_t) 1 << (len - pos)) - 1);
                __m512i s = _mm512_maskz_loadu_epi8(m, src + pos);
                __m512i d = _mm512_maskz_loadu_epi8(m, dest + pos);
                d = _mm512_xor_si512(d, _mm512_gf2p8affine_epi64_epi8(s, gft, 0x00));
                _mm512_mask_storeu_epi8(dest + pos, m, d);
        }
}
