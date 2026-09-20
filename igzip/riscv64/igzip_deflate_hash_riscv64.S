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
	.text
	.align	2

#include "huffman_riscv64.h"

	.global isal_deflate_hash_riscv64
	.type isal_deflate_hash_riscv64, @function

# a0 = hash_table, a1 = hash_mask, a2 = current_index, a3 = dict, a4 = dict_len
isal_deflate_hash_riscv64:
	sub	t0, a2, a4			/* ind = current_index - dict_len (low 16 stored) */
	slli	t3, a4, 32			/* zext dict_len 		*/
	srli	t3, t3, 32
	sltiu	t6, t3, 4			/* t6 = (dict_len < 4), test before sub	*/
	addi	t3, t3, -4			/* dict_len - SHORTEST_MATCH(4)		*/
	add	t1, a3, t3			/* end_in = dict + dict_len - 4		*/
	bnez	t6, .exit_func		/* exit if dict_len < SHORTEST_MATCH	*/

	load_hash_const t4			/* t4 = PROD (0xB2D06057)		*/

	/* Slide a 4-byte little-endian window with byte loads.
	 * Prime win = le32 at dict..dict+3 (in bounds: len >= 4). */
	lbu	t2, 0(a3)
	lbu	t6, 1(a3);	slli	t6, t6, 8;	or	t2, t2, t6
	lbu	t6, 2(a3);	slli	t6, t6, 16;	or	t2, t2, t6
	lbu	t6, 3(a3);	slli	t6, t6, 24;	or	t2, t2, t6

.loop_start:
	compute_hash t6, t2, t4		/* t6 = hash (matches C compute_hash)	*/
	and	t6, t6, a1			/* hash &= hash_mask			*/
	slli	t6, t6, 1			/* byte index (uint16_t table)		*/
	add	t6, a0, t6			/* &hash_table[hash]			*/
	sh	t0, 0(t6)			/* hash_table[hash] = ind		*/
	addi	t0, t0, 1			/* ind++				*/
	addi	a3, a3, 1			/* next_in++				*/
	bgtu	a3, t1, .exit_func		/* done when next_in > end_in		*/
	srli	t2, t2, 8			/* win >>= 8; shift in next byte	*/
	lbu	t6, 3(a3)
	slli	t6, t6, 24
	or	t2, t2, t6
	j	.loop_start
.exit_func:
	ret
	.size isal_deflate_hash_riscv64, .-isal_deflate_hash_riscv64
