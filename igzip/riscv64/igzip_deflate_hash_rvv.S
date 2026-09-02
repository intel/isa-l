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
 * RISC-V accelerated deflate hash-table update.
 *
 *   void isal_deflate_hash_rvv(uint16_t *hash_table,   // a0
 *                              uint32_t hash_mask,      // a1
 *                              uint32_t current_index,  // a2
 *                              uint8_t *dict,           // a3
 *                              uint32_t dict_len);      // a4
 */
#if HAVE_RVV
	.option arch, +v
	.text
	.align 2
	.global isal_deflate_hash_rvv
	.type isal_deflate_hash_rvv, @function
isal_deflate_hash_rvv:
	// if (dict_len < SHORTEST_MATCH=4) return
	li	t4, 4
	bltu	a4, t4, .Ldh_done

	// n = dict_len - SHORTEST_MATCH + 1   (number of 4-byte windows)
	addi	t0, a4, -3
	// index = (current_index - dict_len) & 0xFFFF
	sub	t1, a2, a4
	slli	t1, t1, 48
	srli	t1, t1, 48
	// constants
	li	t2, 0xB2D06057		// PROD (compute_hash multiplier)
	li	t3, -1
	srli	t3, t3, 32		// mask32 = 0xFFFFFFFF
	li	t5, 1			// 1-byte stride for overlapping windows

.Ldh_loop:
	beqz	t0, .Ldh_done
	// vl = vsetvli_e64m1(n); writes element count to t4
	vsetvli	t4, t0, e64, m1, ta, ma

	// load vl overlapping little-endian 4-byte windows, zero-ext to e64
	vlse64.v	v1, (a3), t5
	// keep only the low 32 bits (the actual window value)
	vand.vx		v1, v1, t3
	// hash = data * PROD
	vmul.vx		v1, v1, t2
	vsrl.vi		v1, v1, 16
	// hash *= PROD  (low 64 bits == uint64_t modular truncation)
	vmul.vx		v1, v1, t2
	vsrl.vi		v1, v1, 16

	// in-order scatter stores (lane 0 .. vl-1) -> last writer wins
	mv	a5, t4			// remaining lanes
.Ldh_store:
	vmv.x.s		t6, v1			// extract lane 0 -> GPR
	and		t6, t6, a1		// h & hash_mask
	slli		t6, t6, 1		// byte offset (uint16_t table)
	add		t6, t6, a0		// &hash_table[h]
	sh		t1, (t6)		// hash_table[h] = index (low 16 bits)
	addi		t1, t1, 1		// index++
	vslidedown.vi	v1, v1, 1		// next lane -> position 0
	addi		a5, a5, -1
	bnez		a5, .Ldh_store

	// advance over the vl windows just consumed
	add		a3, a3, t4
	sub		t0, t0, t4
	j		.Ldh_loop

.Ldh_done:
	ret
	.size isal_deflate_hash_rvv, .-isal_deflate_hash_rvv
#endif

