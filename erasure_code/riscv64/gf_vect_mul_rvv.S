##################################################################
#  Copyright (c) 2025 sanechips Technologies Co., Ltd.
#
#  Redistribution and use in source and binary forms, with or without
#  modification, are permitted provided that the following conditions
#  are met:
#    * Redistributions of source code must retain the above copyright
#      notice, this list of conditions and the following disclaimer.
#    * Redistributions in binary form must reproduce the above copyright
#      notice, this list of conditions and the following disclaimer in
#      the documentation and/or other materials provided with the
#      distribution.
#    * Neither the name of sanechips Corporation nor the names of its
#      contributors may be used to endorse or promote products derived
#      from this software without specific prior written permission.
#
#  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
#  "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
#  LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
#  A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
#  OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
#  SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
#  LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
#  DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
#  THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
#  (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
#  OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
########################################################################

#if HAVE_RVV
.text
.align 2

.option arch, +v
.global gf_vect_mul_rvv
.type gf_vect_mul_rvv, @function

/* Function arguments:
 *   a0: len    - Length of vector in bytes.
 *   a1: gftbl  - Pointer to 32-byte array of pre-calculated constants.
 *   a2: src    - Pointer to source data array.
 *   a3: dest   - Pointer to destination data array.
 * Returns:
 *   a0: 0 for success, 1 for failure.
 */

/* Local variables */
#define x_pos t0
#define x_tmp t1
#define x_ptr t2
#define x_len a0
#define x_tbl a1
#define x_src a2
#define x_dest a3

/* Vector registers */
#define v_src v1
#define v_src_lo v2
#define v_src_hi v3
#define v_dest v4
#define v_tmp1_lo v5
#define v_tmp1_hi v6
#define v_gft1_lo v7
#define v_gft1_hi v8

gf_vect_mul_rvv:
    /* Check if len is 32 bytes */
    andi x_tmp, x_len, 0x1F
    bnez x_tmp, .return_fail

    vsetvli t6, x0, e8, m1, ta, ma

    /* Load pre-calculated constants into v_gft1_lo and v_gft1_hi */
    vle8.v v_gft1_lo, (x_tbl)
    addi t3, x_tbl, 16
    vle8.v v_gft1_hi, (t3)

    /* Initialize position counter */
    li x_pos, 0

.Llooprvv_vl:
    /* Load source data into v_src */
    add x_ptr, x_src, x_pos
    vle8.v v_src, (x_ptr)

    /* Split 4-bit lo and 4-bit hi */
    vand.vi v_src_lo, v_src, 0x0F
    vsrl.vi v_src_hi, v_src, 4

    /* Table lookup (GF multiplication) */
    vrgather.vv v_tmp1_lo, v_gft1_lo, v_src_lo
    vrgather.vv v_tmp1_hi, v_gft1_hi, v_src_hi

    /* XOR (GF addition) */
    vxor.vv v_dest, v_tmp1_hi, v_tmp1_lo

    /* Store result to destination */
    vse8.v v_dest, (x_dest)

    /* Increment position counter */
    add x_pos, x_pos, t6
    add x_dest, x_dest, t6

    /* Check if we have processed all bytes */
    blt x_pos, x_len, .Llooprvv_vl

.return_pass:
    li a0, 0
    ret

.return_fail:
    li a0, 1
    ret

#endif
