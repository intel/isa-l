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

.global gf_6vect_mad_rvv
.type gf_6vect_mad_rvv, @function

/* gf_6vect_mad_rvv(int len, int vec, int vec_i, unsigned char *gftbls,
                   unsigned char *src, unsigned char **dest);
 */
/* arguments */
#define x_len a0
#define x_vec a1
#define x_vec_i a2
#define x_tbl a3
#define x_src a4
#define x_dest a5

/* returns */
#define w_ret a0

/* local variables */
#define x_pos t0
#define x_dest1 t1
#define x_dest2 t2
#define x_dest3 t3
#define x_dest4 t4
#define x_dest5 t5
#define x_dest6 t6

/* vectors */
#define v_src v1
#define v_src_lo v2
#define v_src_hi v3
#define v_dest1 v4
#define v_tmp_lo v5
#define v_tmp_hi v6
#define v_gft1_lo v7
#define v_gft1_hi v8
#define v_gft2_lo v9
#define v_gft2_hi v10
#define v_gft3_lo v11
#define v_gft3_hi v12
#define v_gft4_lo v13
#define v_gft4_hi v14
#define v_gft5_lo v15
#define v_gft5_hi v16
#define v_gft6_lo v17
#define v_gft6_hi v18
#define v_dest2 v19
#define v_dest3 v20
#define v_dest4 v21
#define v_dest5 v22
#define v_dest6 v23

gf_6vect_mad_rvv:
    /* less than 16 bytes, return_fail */
    li t6, 16
    blt x_len, t6, .return_fail

   /* save callee-saved registers */
    addi sp, sp, -16
    sd s8, 0(sp)

    vsetvli a6, x0, e8, m1, ta, ma

    /* Load table 1 */
    slli s8, x_vec_i, 5
    add x_tbl, x_tbl, s8
    vle8.v v_gft1_lo, (x_tbl)
    addi s8, x_tbl, 16
    vle8.v v_gft1_hi, (s8)

    /* Load table 2 */
    slli s8, x_vec, 5
    add x_tbl, x_tbl, s8
    vle8.v v_gft2_lo, (x_tbl)
    addi s8, x_tbl, 16
    vle8.v v_gft2_hi, (s8)

    /* Load table 3 */
    slli s8, x_vec, 5
    add x_tbl, x_tbl, s8
    vle8.v v_gft3_lo, (x_tbl)
    addi s8, x_tbl, 16
    vle8.v v_gft3_hi, (s8)

    /* Load table 4 */
    slli s8, x_vec, 5
    add x_tbl, x_tbl, s8
    vle8.v v_gft4_lo, (x_tbl)
    addi s8, x_tbl, 16
    vle8.v v_gft4_hi, (s8)

    /* Load table 5 */
    slli s8, x_vec, 5
    add x_tbl, x_tbl, s8
    vle8.v v_gft5_lo, (x_tbl)
    addi s8, x_tbl, 16
    vle8.v v_gft5_hi, (s8)

    /* Load table 6 */
    slli s8, x_vec, 5
    add x_tbl, x_tbl, s8
    vle8.v v_gft6_lo, (x_tbl)
    addi s8, x_tbl, 16
    vle8.v v_gft6_hi, (s8)

    /* Load destination pointers */
    ld x_dest1, 0(x_dest)
    ld x_dest2, 8(x_dest)
    ld x_dest3, 16(x_dest)
    ld x_dest4, 24(x_dest)
    ld x_dest5, 32(x_dest)
    ld x_dest6, 40(x_dest)

    li x_pos, 0

.Llooprvv_vl:
    blt x_pos, x_len, .Lloop_body
    j .return_pass

.Lloop_body:
    /* Load source data */
    add a7, x_src, x_pos
    vle8.v v_src, (a7)

    /* Split 4-bit lo; 4-bit hi */
    vand.vi v_src_lo, v_src, 0x0F
    vsrl.vi v_src_hi, v_src, 4

    /* load dest data */
    add a7, x_dest1, x_pos
    vle8.v v_dest1, (a7)
    add a7, x_dest2, x_pos
    vle8.v v_dest2, (a7)
    add a7, x_dest3, x_pos
    vle8.v v_dest3, (a7)
    add a7, x_dest4, x_pos
    vle8.v v_dest4, (a7)
    add a7, x_dest5, x_pos
    vle8.v v_dest5, (a7)
    add a7, x_dest6, x_pos
    vle8.v v_dest6, (a7)

    /* dest1 */
    vrgather.vv v_tmp_lo, v_gft1_lo, v_src_lo
    vrgather.vv v_tmp_hi, v_gft1_hi, v_src_hi
    vxor.vv v_dest1, v_tmp_lo, v_dest1
    vxor.vv v_dest1, v_tmp_hi, v_dest1

    /* dest2 */
    vrgather.vv v_tmp_lo, v_gft2_lo, v_src_lo
    vrgather.vv v_tmp_hi, v_gft2_hi, v_src_hi
    vxor.vv v_dest2, v_tmp_lo, v_dest2
    vxor.vv v_dest2, v_tmp_hi, v_dest2

    /* dest3 */
    vrgather.vv v_tmp_lo, v_gft3_lo, v_src_lo
    vrgather.vv v_tmp_hi, v_gft3_hi, v_src_hi
    vxor.vv v_dest3, v_tmp_lo, v_dest3
    vxor.vv v_dest3, v_tmp_hi, v_dest3

    /* dest4 */
    vrgather.vv v_tmp_lo, v_gft4_lo, v_src_lo
    vrgather.vv v_tmp_hi, v_gft4_hi, v_src_hi
    vxor.vv v_dest4, v_tmp_lo, v_dest4
    vxor.vv v_dest4, v_tmp_hi, v_dest4

    /* dest5 */
    vrgather.vv v_tmp_lo, v_gft5_lo, v_src_lo
    vrgather.vv v_tmp_hi, v_gft5_hi, v_src_hi
    vxor.vv v_dest5, v_tmp_lo, v_dest5
    vxor.vv v_dest5, v_tmp_hi, v_dest5

    /* dest6 */
    vrgather.vv v_tmp_lo, v_gft6_lo, v_src_lo
    vrgather.vv v_tmp_hi, v_gft6_hi, v_src_hi
    vxor.vv v_dest6, v_tmp_lo, v_dest6
    vxor.vv v_dest6, v_tmp_hi, v_dest6

    /* Store destination data */
    add a7, x_dest1, x_pos
    vse8.v v_dest1, (a7)
    add a7, x_dest2, x_pos
    vse8.v v_dest2, (a7)
    add a7, x_dest3, x_pos
    vse8.v v_dest3, (a7)
    add a7, x_dest4, x_pos
    vse8.v v_dest4, (a7)
    add a7, x_dest5, x_pos
    vse8.v v_dest5, (a7)
    add a7, x_dest6, x_pos
    vse8.v v_dest6, (a7)

    /* Increment position */
    add x_pos, x_pos, a6

    j .Llooprvv_vl

.return_pass:
    ld s8, 0(sp)
    addi sp, sp, 16

    li w_ret, 0
    ret

.return_fail:
    li w_ret, 1
    ret

#endif
