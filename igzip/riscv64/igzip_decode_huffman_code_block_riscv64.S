/**********************************************************************
  Copyright (c) 2026 ZTE Corporation.
  AI
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

/* RV64 assembly rewrite of the huffman decode main loop.
 *
 * Defect in the base C version: the decode loop touches the hot state
 * (read_in / read_in_length / next_in / avail_in / next_out / ...) in
 * memory on every symbol -- each field access is an ld/sd pair against
 * struct inflate_state, and the load feeds the next dependent
 * instruction, so millions of loop iterations pay both the memory
 * traffic and the serial load latency. Profile showed these accesses
 * dominating the decode hot path.
 *
 * Fix (following the approach of the upstream aarch64 decode, which
 * keeps the same state in registers): this rewrite holds all hot state
 * in callee-saved registers s0-s11 (next_out s0, avail_out s1,
 * total_out s2, next_in s3, read_in s4, avail_in s5, read_in_length
 * s6), loading them once in the prologue and writing them back at
 * exit. The pieces of the original C (inflate_in_load,
 * decode_next_lit_len, decode_next_dist) are kept as .macro blocks and
 * labeled sections -- not real functions -- so nothing pushes the hot
 * state to the stack across their boundaries and the loop body runs
 * with zero memory round-trips.
 */

#include "stdmac_riscv64.h"

/* ---- struct inflate_state offsets (verified with offsetof, LP64D) ---- */
.set _next_out,        0
.set _avail_out,       8
.set _total_out,       12
.set _next_in,         16
.set _read_in,         24
.set _avail_in,        32
.set _read_in_length,  36
.set _lit_huff_code,   40    /* 18912 bytes */
.set _dist_huff_code,  18952 /* 2208 bytes */
.set _block_state,     21160
.set _bfinal,          21168
.set _write_overflow_lits, 21188
.set _write_overflow_len,  21192
.set _copy_overflow_length, 21196
.set _copy_overflow_distance, 21200

.set ISAL_BLOCK_NEW_HDR,    0
.set ISAL_BLOCK_HDR,        1
.set ISAL_BLOCK_TYPE0,      2
.set ISAL_BLOCK_CODED,      3
.set ISAL_BLOCK_INPUT_DONE, 4
.set ISAL_BLOCK_FINISH,     5

#define ISAL_DECOMP_OK          0
#define ISAL_END_INPUT          1
#define ISAL_OUT_OVERFLOW       2
#define ISAL_INVALID_BLOCK     -1
#define ISAL_INVALID_SYMBOL    -2
#define ISAL_INVALID_LOOKBACK  -3

#define ISAL_DEF_MAX_CODE_LEN 15
#define LARGE_SHORT_SYM_LEN 25
#define LARGE_SHORT_SYM_MASK ((1 << LARGE_SHORT_SYM_LEN) - 1)
#define LARGE_LONG_SYM_LEN 10
#define LARGE_LONG_SYM_MASK ((1 << LARGE_LONG_SYM_LEN) - 1)
#define LARGE_SHORT_CODE_LEN_OFFSET 28
#define LARGE_LONG_CODE_LEN_OFFSET 10
#define LARGE_FLAG_BIT_OFFSET 25
#define LARGE_FLAG_BIT (1 << LARGE_FLAG_BIT_OFFSET)
#define LARGE_SYM_COUNT_OFFSET 26
#define LARGE_SYM_COUNT_LEN 2
#define LARGE_SYM_COUNT_MASK ((1 << LARGE_SYM_COUNT_LEN) - 1)
#define LARGE_SHORT_MAX_LEN_OFFSET 26

#define SMALL_SHORT_SYM_LEN 9
#define SMALL_LONG_SYM_LEN 9
#define SMALL_SHORT_CODE_LEN_OFFSET 11
#define SMALL_LONG_CODE_LEN_OFFSET 10
#define SMALL_FLAG_BIT_OFFSET 10
#define SMALL_FLAG_BIT (1 << SMALL_FLAG_BIT_OFFSET)

#define DIST_SYM_MASK 0x1f
#define MAX_LIT_LEN_SYM 512
#define INVALID_SYMBOL 0x1FFF
#define ISAL_DECODE_LONG_BITS 12
#define ISAL_DECODE_SHORT_BITS 10

#define DIST_LEN 30

/* rfc1951_tables field offsets (struct is packed by construction below) */
#define RFC_DIST_EXTRA 0     /* uint8[32]  */
#define RFC_DIST_START 32    /* uint32[32] */
#define RFC_LEN_EXTRA  160   /* uint8[32]  */
#define RFC_LEN_START  192   /* uint16[32] */

/* ---------------- register allocation ----------------
 * s0  next_out          s6  read_in_length (sign-extended int32)
 * s1  avail_out         s7  rfc_lookup_table base
 * s2  total_out         s8  state
 * s3  next_in           s9  start_out
 * s4  read_in           s10 write_overflow_lits
 * s5  avail_in          s11 write_overflow_len
 * a0/a1/a2..a7/t0..t6  scratch
 *
 * Stack frame (176 bytes, 16-byte aligned):
 *   0(sp)  snapshot read_in        24(sp) snapshot next_out
 *   8(sp)  snapshot next_in        32(sp) repeat_length spill across memcpy
 *  16(sp)  snapshot avail_in (w)   64..152  callee-saved s0..s11
 *  20(sp)  snapshot read_in_length (w)     160(sp) ra
 */
#define S_state   s8
#define S_start   s9

/* ============================================================
 * inflate_in_load -- refill the bit buffer from next_in (macro form)
 * Mirrors inflate_in_load() in igzip_inflate.c. Operates directly on
 * the register-resident state: reads s3 (next_in), s5 (avail_in),
 * s6 (read_in_length); updates s4 (read_in), s3, s5, s6 in place.
 * The fast path uses one plain 8-byte scalar load, matching the
 * load_le_u64() the C version uses. Uses only t0..t3.
 * ============================================================ */
.macro IN_LOAD
	li	t0, 64
	bge	s6, t0, 902f			/* read_in_length >= 64 */
	li	t0, 8
	bge	s5, t0, 901f
900:	/* avail_in < 8: byte-wise fill while read_in_length < 57 */
	li	t0, 57
	bge	s6, t0, 902f
	beqz	s5, 902f
	lbu	t1, 0(s3)
	addi	s3, s3, 1
	addi	s5, s5, -1
	sll	t1, t1, s6
	or	s4, s4, t1
	addi	s6, s6, 8
	j	900b
901:	/* avail_in >= 8: one plain 8-byte load, same as the C baseline's
	 * load_le_u64(). Scalar misaligned loads are already required by
	 * the C code this replaces. */
	addi	t1, s6, 7
	srai	t1, t1, 3
	li	t0, 8
	sub	t2, t0, t1			/* t2 = new_bytes (1..8) */
	ld	t3, 0(s3)
	sll	t3, t3, s6
	or	s4, s4, t3
	add	s3, s3, t2
	sub	s5, s5, t2
	slli	t2, t2, 3
	add	s6, s6, t2
902:
.endm

	.text
	.globl	decode_huffman_code_block_stateless_riscv64
	.type	decode_huffman_code_block_stateless_riscv64, @function
decode_huffman_code_block_stateless_riscv64:
	addi	sp, sp, -176
	sd	ra, 160(sp)
	sd	s0, 64(sp)
	sd	s1, 72(sp)
	sd	s2, 80(sp)
	sd	s3, 88(sp)
	sd	s4, 96(sp)
	sd	s5, 104(sp)
	sd	s6, 112(sp)
	sd	s7, 120(sp)
	sd	s8, 128(sp)
	sd	s9, 136(sp)
	sd	s10, 144(sp)
	sd	s11, 152(sp)

	mv	S_state, a0
	mv	S_start, a1

	/* load hot state into registers */
	ld	s0, _next_out(a0)
	lw	s1, _avail_out(a0)
	lw	s2, _total_out(a0)
	ld	s3, _next_in(a0)
	ld	s4, _read_in(a0)
	lw	s5, _avail_in(a0)
	lw	s6, _read_in_length(a0)
	sext.w	s6, s6			/* read_in_length is signed int32 */
	li	t0, _write_overflow_lits
	add	t0, t0, a0
	lw	s10, 0(t0)
	li	t0, _write_overflow_len
	add	t0, t0, a0
	lw	s11, 0(t0)

	la	s7, rfc_lookup_table

	/* state->copy_overflow_length = 0; copy_overflow_distance = 0; */
	li	t0, _copy_overflow_length
	add	t0, t0, S_state
	sw	zero, 0(t0)
	li	t0, _copy_overflow_distance
	add	t0, t0, S_state
	sw	zero, 0(t0)

	/* block_state, bfinal, write_overflow, copy_overflow all sit at
	 * offsets over 2047, beyond the RV load/store immediate range, so
	 * those accesses go through a li/add computed pointer. */
	li	t1, _block_state
	add	t1, t1, S_state
	lw	t0, 0(t1)
	li	t2, ISAL_BLOCK_CODED
	bne	t0, t2, exit_success

block_state_loop:
	/* ============ inflate_in_load (macro) ============ */
	IN_LOAD

load_done:
	/* ---- loop-top snapshot for END_INPUT rollback (store only) ---- */
	sd	s4, 0(sp)
	sd	s3, 8(sp)
	sw	s5, 16(sp)
	sw	s6, 20(sp)
	sd	s0, 24(sp)

	/* ============ decode_next_lit_len (section, was a function) ============
	 * Contract in register terms: consumes one lit/len symbol from
	 * s4/s6; leaves next_lits in t1, sym_count in t6.
	 * reload if read_in_length <= ISAL_DEF_MAX_CODE_LEN */
	li	t0, ISAL_DEF_MAX_CODE_LEN
	bgt	s6, t0, decode_lit_start
	IN_LOAD

decode_lit_start:
	li	t0, (1 << ISAL_DECODE_LONG_BITS) - 1
	and	t1, s4, t0			/* t1 = next_bits */
	slli	t2, t1, 2			/* uint32 short_code_lookup */
	add	t2, S_state, t2
	lw	t3, _lit_huff_code(t2)		/* t3 = next_sym */
	/* NOTE: entries with code_len >= 8 have bit31 set and are negative
	 * as int32; RV64 lw sign-extends, so every 64-bit op below would
	 * see 0xFFFF... high bits. Zero-extend to mirror the C uint32_t. */
	slli	t3, t3, 32
	srli	t3, t3, 32

	srli	t0, t3, LARGE_FLAG_BIT_OFFSET
	andi	t0, t0, 1			/* LARGE_FLAG_BIT test */
	beqz	t0, lit_short_path

	/* ---- long code lookup ---- */
	srli	t0, t3, LARGE_SHORT_MAX_LEN_OFFSET
	li	t1, 1
	sll	t1, t1, t0
	addi	t1, t1, -1			/* bit_mask */
	and	t2, s4, t1			/* wide next_bits */
	srl	t2, t2, ISAL_DECODE_LONG_BITS
	li	t0, LARGE_SHORT_SYM_MASK
	and	t4, t3, t0			/* hint */
	add	t4, t4, t2			/* long_code_lookup index */
	li	t5, _lit_huff_code
	li	t6, (1 << ISAL_DECODE_LONG_BITS) * 4
	add	t5, t5, t6
	add	t5, S_state, t5
	slli	t4, t4, 1
	add	t5, t5, t4
	lhu	t3, 0(t5)			/* next_sym */
	srli	t0, t3, LARGE_LONG_CODE_LEN_OFFSET	/* bit_count */
	sub	s6, s6, t0
	srl	s4, s4, t0
	andi	t3, t3, LARGE_LONG_SYM_MASK
	beqz	t0, 5f				/* bit_count == 0 -> INVALID */
	li	t6, 1				/* sym_count = 1 */
	mv	t1, t3				/* next_lits */
	j	lit_decode_end
	5:	li	t1, INVALID_SYMBOL
	li	t6, 1
	j	lit_decode_end

lit_short_path:
	srli	t0, t3, LARGE_SHORT_CODE_LEN_OFFSET	/* bit_count */
	sub	s6, s6, t0
	srl	s4, s4, t0
	beqz	t0, 6f
	srli	t6, t3, LARGE_SYM_COUNT_OFFSET
	andi	t6, t6, LARGE_SYM_COUNT_MASK	/* sym_count */
	li	t0, (1 << LARGE_SHORT_SYM_LEN) - 1
	and	t1, t3, t0			/* next_lits */
	j	lit_decode_end
	6:	li	t1, INVALID_SYMBOL
	li	t6, 0

lit_decode_end:
	beqz	t6, invalid_symbol		/* sym_count == 0 */
	bgez	s6, lit_ok

	/* read_in_length < 0: roll back to loop top, END_INPUT.
	 * No output was written since the snapshot, next_out untouched. */
	ld	s4, 0(sp)
	ld	s3, 8(sp)
	lw	s5, 16(sp)
	lw	s6, 20(sp)
	sext.w	s6, s6			/* restore as signed int32 */
	li	a0, ISAL_END_INPUT
	j	exit_store

lit_ok:
	/* output snapshot for the match path (24(sp) slot is 8 bytes and
	 * only overlaps nothing: 16/20(sp) hold the 32-bit avail_in /
	 * read_in_length snapshots -- keep it that way, an sd here once
	 * clobbered an avail_in snapshot that shared its upper half) */
	sd	s0, 24(sp)

sym_count_loop:
	/* next_lit = next_lits & 0xffff (andi needs a 12-bit imm, so
	 * extract with a shift pair) */
	slli	t2, t1, 48
	srli	t2, t2, 48			/* t2 = next_lit */
	li	t0, 256
	bltu	t2, t0, lit_write		/* literal */
	li	t0, 1
	bgt	t6, t0, lit_write		/* sym_count > 1: literal */
	li	t0, 256
	beq	t2, t0, next_lit_eq_256
	li	t0, MAX_LIT_LEN_SYM
	bgtu	t2, t0, invalid_symbol
	j	match_path

lit_write:
	beqz	s1, lit_overflow
	sb	t2, 0(s0)
	addi	s0, s0, 1
	addi	s1, s1, -1
	addi	s2, s2, 1
	srli	t1, t1, 8
	addi	t6, t6, -1
	bnez	t6, sym_count_loop
	j	block_state_loop

lit_overflow:
	/* avail_out == 0: stash the remaining packed symbols */
	mv	s10, t1				/* write_overflow_lits */
	mv	s11, t6				/* write_overflow_len */
	addi	t0, t6, -1
	slli	t0, t0, 3
	srl	t1, t1, t0			/* last packed symbol */
	li	t6, 1
	slli	t2, t1, 48
	srli	t2, t2, 48			/* last packed symbol */
	li	t3, 256
	bgeu	t2, t3, 7f
	li	a0, ISAL_OUT_OVERFLOW
	j	exit_store
	7:	bgtu	t2, t3, lit_overflow_len_dec
	/* == 256: block_state = bfinal ? INPUT_DONE : NEW_HDR; len -= 1 */
	addi	s11, s11, -1
	li	t6, _bfinal
	add	t6, t6, S_state
	lw	t4, 0(t6)
	li	t5, ISAL_BLOCK_INPUT_DONE
	bnez	t4, 9f
	li	t5, ISAL_BLOCK_NEW_HDR
	9:	li	t4, _block_state
	add	t4, t4, S_state
	sw	t5, 0(t4)
	li	a0, ISAL_OUT_OVERFLOW
	j	exit_store
lit_overflow_len_dec:
	addi	s11, s11, -1
	j	sym_count_loop

next_lit_eq_256:
	li	t4, _bfinal
	add	t4, t4, S_state
	lw	t4, 0(t4)
	li	t5, ISAL_BLOCK_INPUT_DONE
	bnez	t4, 10f
	li	t5, ISAL_BLOCK_NEW_HDR
	10:	li	t4, _block_state
	add	t4, t4, S_state
	sw	t5, 0(t4)
	j	exit_success

/* ---------------- match path (next_lit in t2, 257..512) ----------------
 * ============ decode_next_dist (section, was a function) ============
 * Contract in register terms: consumes one dist symbol + its extra
 * bits from s4/s6; leaves look_back_dist in t1; repeat_length stays
 * in t3/spill. */
match_path:
	addi	t3, t2, -254			/* repeat_length */
	/* Keep repeat_length on the stack: the dist decode below reuses
	 * every t register (IN_LOAD, table loads). */
	sw	t3, 32(sp)

	li	t0, ISAL_DEF_MAX_CODE_LEN
	bgt	s6, t0, 11f
	IN_LOAD
11:
	li	t0, (1 << ISAL_DECODE_SHORT_BITS) - 1
	and	t1, s4, t0			/* next_bits */
	slli	t1, t1, 1			/* uint16 table */
	li	t2, _dist_huff_code
	add	t2, t1, t2
	add	t2, S_state, t2
	lhu	t3, 0(t2)			/* next_sym */

	srli	t0, t3, SMALL_FLAG_BIT_OFFSET
	andi	t0, t0, 1			/* SMALL_FLAG_BIT test */
	beqz	t0, dist_short

	/* ---- dist long path ---- */
	li	t1, SMALL_FLAG_BIT		/* 1024: not an addi-imm */
	sub	t0, t3, t1
	srli	t0, t0, SMALL_SHORT_CODE_LEN_OFFSET
	li	t1, 1
	sll	t1, t1, t0
	addi	t1, t1, -1			/* bit_mask */
	and	t2, s4, t1
	srl	t2, t2, ISAL_DECODE_SHORT_BITS
	li	t0, 0x1ff			/* SMALL_SHORT_SYM_MASK */
	and	t4, t3, t0			/* hint */
	add	t4, t4, t2			/* long_code_lookup index */
	li	t5, _dist_huff_code
	li	t6, (1 << ISAL_DECODE_SHORT_BITS) * 2
	add	t5, t5, t6
	add	t5, S_state, t5
	slli	t4, t4, 1
	add	t5, t5, t4
	lhu	t3, 0(t5)			/* next_sym */
	srli	t0, t3, SMALL_LONG_CODE_LEN_OFFSET	/* bit_count */
	j	dist_adjust

dist_short:
	srli	t0, t3, SMALL_SHORT_CODE_LEN_OFFSET	/* bit_count */

dist_adjust:
	sub	s6, s6, t0
	srl	s4, s4, t0
	bnez	t0, 15f
	/* bit_count == 0: read_in_length -= next_sym; INVALID */
	sub	s6, s6, t3
	li	t3, INVALID_SYMBOL
	15:
	li	t0, DIST_SYM_MASK
	and	t3, t3, t0			/* next_dist */

	/* if read_in_length < 0: full rollback, END_INPUT */
	bgez	s6, 16f
	j	restore_all_end_input
	16:
	li	t0, DIST_LEN
	bgeu	t3, t0, invalid_symbol

	/* look_back_dist = dist_start[nd] + read_bits(dist_extra[nd]).
	 * The C path here is inflate_in_read_bits -> inflate_in_load,
	 * which tops the buffer up unconditionally (its read_in_length
	 * >= 64 check makes it a no-op when already full); skipping it
	 * can leave fewer than the extra bits buffered and trip a false
	 * END_INPUT rollback. IN_LOAD clobbers t0..t3, so the extra bit
	 * count and the rfc table pointer must be re-derived after it
	 * from nd kept in a7 (dead past the dist decode). */
	mv	a7, t3				/* keep nd across IN_LOAD */
	IN_LOAD
	mv	t3, a7				/* nd */
	add	t4, s7, t3
	lbu	t5, RFC_DIST_EXTRA(t4)		/* extra bit count (uint8[nd]) */
	slli	t3, t3, 2			/* dist_start is uint32[nd] */
	add	t4, s7, t3
	lwu	t6, RFC_DIST_START(t4)		/* dist_start[nd] */
	li	t2, 1
	sll	t2, t2, t5
	addi	t2, t2, -1			/* extra-bits mask */
	and	t1, s4, t2			/* extra bits value */
	srl	s4, s4, t5
	sub	s6, s6, t5
	add	t1, t1, t6			/* look_back_dist */

	/* if read_in_length < 0: full rollback, END_INPUT */
	bgez	s6, 21f
	j	restore_all_end_input
	21:
	/* if next_out - look_back_dist < start_out -> INVALID_LOOKBACK */
	sub	t2, s0, t1
	bltu	t2, S_start, invalid_lookback

	/* if avail_out < repeat_length: split, remember overflow */
	lw	t3, 32(sp)			/* restore repeat_length */
	bleu	t3, s1, 22f
	sub	t5, t3, s1
	li	t6, _copy_overflow_length
	add	t6, t6, S_state
	sw	t5, 0(t6)			/* copy_overflow_length */
	li	t6, _copy_overflow_distance
	add	t6, t6, S_state
	sw	t1, 0(t6)			/* copy_overflow_distance */
	mv	t3, s1				/* repeat_length = avail_out */
	/* update the spill too: memcpy_path reloads it and match_copied
	 * advances next_out/total_out by it; keeping the full length here
	 * would over-advance next_out when avail_out < repeat_length */
	sw	t3, 32(sp)
22:
	/* look_back_dist > repeat_length -> memcpy else byte_copy */
	bgeu	t1, t3, memcpy_path

byte_copy_path:
	/* overlapping copy, byte at a time */
	sub	a1, s0, t1
	mv	a2, t3
	23:	beqz	a2, 24f
	lbu	t5, 0(a1)
	addi	a1, a1, 1
	sb	t5, 0(s0)
	addi	s0, s0, 1
	addi	a2, a2, -1
	j	23b
	24:
	/* s0 already advanced by repeat_length */
	j	match_copied

memcpy_path:
	/* memcpy(next_out, next_out - look_back, repeat_length) */
	sub	a1, s0, t1
	mv	a0, s0
	mv	a2, t3
	call	memcpy
	lw	t3, 32(sp)			/* reload repeat_length (sw'd as 32-bit) */
	add	s0, s0, t3			/* byte_copy path advanced s0 in-loop */
	/* s0..s11 are callee-saved so memcpy preserved them; only the
	 * t/a registers were clobbered and none of them is live here */

match_copied:
	sub	s1, s1, t3
	add	s2, s2, t3
	li	t6, _copy_overflow_length
	add	t6, t6, S_state
	lw	t4, 0(t6)
	blez	t4, block_state_loop
	li	a0, ISAL_OUT_OVERFLOW
	j	exit_store

restore_all_end_input:
	/* roll the whole iteration back to the loop-top snapshot */
	ld	s4, 0(sp)
	ld	s3, 8(sp)
	lw	s5, 16(sp)
	lw	s6, 20(sp)
	sext.w	s6, s6			/* restore as signed int32 */
	ld	t0, 24(sp)			/* snapshot next_out */
	sub	t1, s0, t0			/* delta written since then */
	add	s1, s1, t1			/* snapshot avail_out */
	sub	s2, s2, t1			/* snapshot total_out */
	mv	s0, t0
	mv	s10, zero			/* write_overflow_* = 0 */
	mv	s11, zero
	li	a0, ISAL_END_INPUT
	j	exit_store

invalid_symbol:
	li	a0, ISAL_INVALID_SYMBOL
	j	exit_store

invalid_lookback:
	li	a0, ISAL_INVALID_LOOKBACK
	j	exit_store

exit_success:
	li	a0, ISAL_DECOMP_OK

exit_store:
	sd	s0, _next_out(S_state)
	sw	s1, _avail_out(S_state)
	sw	s2, _total_out(S_state)
	sd	s3, _next_in(S_state)
	sd	s4, _read_in(S_state)
	sw	s5, _avail_in(S_state)
	sw	s6, _read_in_length(S_state)
	li	t0, _write_overflow_lits
	add	t0, t0, S_state
	sw	s10, 0(t0)
	li	t0, _write_overflow_len
	add	t0, t0, S_state
	sw	s11, 0(t0)
	ld	ra, 160(sp)
	ld	s0, 64(sp)
	ld	s1, 72(sp)
	ld	s2, 80(sp)
	ld	s3, 88(sp)
	ld	s4, 96(sp)
	ld	s5, 104(sp)
	ld	s6, 112(sp)
	ld	s7, 120(sp)
	ld	s8, 128(sp)
	ld	s9, 136(sp)
	ld	s10, 144(sp)
	ld	s11, 152(sp)
	addi	sp, sp, 176
	ret
	.size	decode_huffman_code_block_stateless_riscv64, .-decode_huffman_code_block_stateless_riscv64

/* ---------------- rfc1951 lookup tables ---------------- */
	.section .rodata
	.align 4
	.globl	rfc_lookup_table
rfc_lookup_table:
	/* dist_extra_bit_count[32] */
	.byte	0x00,0x00,0x00,0x00,0x01,0x01,0x02,0x02
	.byte	0x03,0x03,0x04,0x04,0x05,0x05,0x06,0x06
	.byte	0x07,0x07,0x08,0x08,0x09,0x09,0x0a,0x0a
	.byte	0x0b,0x0b,0x0c,0x0c,0x0d,0x0d,0x00,0x00
	/* dist_start[32] (uint32) */
	.word	0x0001,0x0002,0x0003,0x0004,0x0005,0x0007,0x0009,0x000d
	.word	0x0011,0x0019,0x0021,0x0031,0x0041,0x0061,0x0081,0x00c1
	.word	0x0101,0x0181,0x0201,0x0301,0x0401,0x0601,0x0801,0x0c01
	.word	0x1001,0x1801,0x2001,0x3001,0x4001,0x6001,0x0000,0x0000
	/* len_extra_bit_count[32] */
	.byte	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00
	.byte	0x01,0x01,0x01,0x01,0x02,0x02,0x02,0x02
	.byte	0x03,0x03,0x03,0x03,0x04,0x04,0x04,0x04
	.byte	0x05,0x05,0x05,0x05,0x00,0x00,0x00,0x00
	/* len_start[32] (uint16) */
	.short	0x0003,0x0004,0x0005,0x0006,0x0007,0x0008,0x0009,0x000a
	.short	0x000b,0x000d,0x000f,0x0011,0x0013,0x0017,0x001b,0x001f
	.short	0x0023,0x002b,0x0033,0x003b,0x0043,0x0053,0x0063,0x0073
	.short	0x0083,0x00a3,0x00c3,0x00e3,0x0102,0x0103,0x0000,0x0000
	.size	rfc_lookup_table, . - rfc_lookup_table
