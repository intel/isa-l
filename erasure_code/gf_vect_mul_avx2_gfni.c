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
 * gf_vect_mul_avx2_gfni(len, mul_array, src, dest)
 *
 * Multiplies len bytes of src by the GF(2^8) constant whose 8x8 affine
 * matrix is broadcast in mul_array, writing the result to dest.
 *
 * Constraints (matching the prior NASM kernel):
 *   - len must be a multiple of 32; otherwise returns 1.
 *   - src and dest must be 32-byte aligned (non-temporal moves).
 *
 * Returns 0 on success, 1 if len is not a multiple of 32.
 */

#include <immintrin.h>

#define AFFINE(s, g) _mm256_gf2p8affine_epi64_epi8((s), (g), 0x00)

__attribute__((target("avx2,gfni"))) int
gf_vect_mul_avx2_gfni(int len, unsigned char *mul_array, void *src, void *dest)
{
        if (len & 0x1f)
                return 1;

        const __m256i gft = _mm256_loadu_si256((const __m256i *) mul_array);
        const unsigned char *s = (const unsigned char *) src;
        unsigned char *d = (unsigned char *) dest;

        for (int pos = 0; pos < len; pos += 32) {
#ifdef NO_NT_LDST
                __m256i x = _mm256_load_si256((const __m256i *) (s + pos));
                __m256i y = AFFINE(x, gft);
                _mm256_store_si256((__m256i *) (d + pos), y);
#else
                __m256i x = _mm256_stream_load_si256((const __m256i *) (s + pos));
                __m256i y = AFFINE(x, gft);
                _mm256_stream_si256((__m256i *) (d + pos), y);
#endif
        }

        _mm256_zeroupper();
        return 0;
}
