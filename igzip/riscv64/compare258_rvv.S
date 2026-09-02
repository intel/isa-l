/**********************************************************************
  Copyright (c) 2026 ZTE Corporation.
  Redistribution and use in source and binary forms, with or without
  modification, are permitted provided that the following conditions
  are met:
    * Redistributions of source code must retain the above copyright
      notice, this list of conditions and the following disclaimer.
    * Redistributions in binary form must reproduce the above copyright
      notice, this list of conditions and the following disclaimer in
      the documentation and/or other materials provided with the
      distribution.
    * Neither the name of ZTE Corporation nor the names of its
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
 *   uint32_t compare258_rvv(const uint8_t *str1,   // a0
 *                           const uint8_t *str2,   // a1
 *                           uint32_t max_length);  // a2
 */
#if HAVE_RVV
	.option arch, +v
	.text
	.align 2
	.global compare258_rvv
	.type compare258_rvv, @function
compare258_rvv:
	// if (max_length > 258) max_length = 258;
	li	t0, 258
	bltu	a2, t0, 1f
	mv	a2, t0
1:
	li	t1, 0			// count = 0
.Lc258_loop:
	bgeu	t1, a2, .Lc258_allmatch	// count >= max_length -> all matched
	sub	a3, a2, t1		// remaining = max_length - count
	vsetvli	t3, a3, e8, m1, ta, ma	// vl = min(remaining, VLMAX_e8m1)
	vle8.v	v1, (a0)		// str1 chunk
	vle8.v	v2, (a1)		// str2 chunk
	vmsne.vv v3, v1, v2		// mask: differing bytes
	vfirst.m a4, v3		// first mismatch index, or -1
	bltz	a4, .Lc258_advance	// -1 -> whole chunk matches
	// mismatch at byte (count + a4)
	add	a0, t1, a4
	ret
.Lc258_advance:
	add	a0, a0, t3
	add	a1, a1, t3
	add	t1, t1, t3
	j	.Lc258_loop
.Lc258_allmatch:
	mv	a0, a2			// return max_length
	ret
	.size compare258_rvv, .-compare258_rvv

/*
 *   uint32_t compare_rvv(const uint8_t *str1,   // a0
 *                        const uint8_t *str2,   // a1
 *                        uint32_t max_length);  // a2
 *
 * Same longest-common-prefix match as compare258_rvv, but WITHOUT the 258 cap
 * (matches up to max_length). Bit-identical to compare() in huffman.h.
 */
	.global compare_rvv
	.type compare_rvv, @function
compare_rvv:
	li	t1, 0			// count = 0
.Lcmp_loop:
	bgeu	t1, a2, .Lcmp_allmatch	// count >= max_length -> all matched
	sub	a3, a2, t1		// remaining = max_length - count
	vsetvli	t3, a3, e8, m1, ta, ma	// vl = min(remaining, VLMAX_e8m1)
	vle8.v	v1, (a0)
	vle8.v	v2, (a1)
	vmsne.vv v3, v1, v2		// mask: differing bytes
	vfirst.m a4, v3		// first mismatch index, or -1
	bltz	a4, .Lcmp_advance	// -1 -> whole chunk matches
	add	a0, t1, a4		// mismatch at byte (count + a4)
	ret
.Lcmp_advance:
	add	a0, a0, t3
	add	a1, a1, t3
	add	t1, t1, t3
	j	.Lcmp_loop
.Lcmp_allmatch:
	mv	a0, a2			// return max_length
	ret
	.size compare_rvv, .-compare_rvv
#endif
