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
.global gf_vect_mad_rvv
.type gf_vect_mad_rvv, @function

/* gf_vect_mad_rvv(int len, int vec, int vec_i, unsigned char *gftbls,
                   unsigned char *src, unsigned char *dest);
 */
/* arguments */
#define x_len a0
#define x_vec_i a2
#define x_tbl a3
#define x_src a4
#define x_dest a5

/* returns */
#define w_ret a0

/* local variables */
#define x_pos t0
#define x_remain t3

/* vectors */
#define v_src v1
#define v_src_lo v2
#define v_src_hi v3
#define v_dest v4
#define v_tmp1_lo v5
#define v_tmp1_hi v6
#define v_gft1_lo v7
#define v_gft1_hi v8

gf_vect_mad_rvv:
    /* less than 16 bytes, return_fail */
    li t1, 16
    blt x_len, t1, .return_fail

    vsetivli zero, 16, e8, m1, ta, ma

    /* x_tbl += x_vec_i * 2^5 */
    slli t1, x_vec_i, 5
    add x_tbl, x_tbl, t1

    /* Load gft1_lo and gft1_hi */
    vle8.v v_gft1_lo, (x_tbl)
    addi t1, x_tbl, 16
    vle8.v v_gft1_hi, (t1)

    li x_pos, 0

.Lloop_rvv_vl:
    sub x_remain, x_len, x_pos
    vsetvli t2, x_remain, e8, m1, ta, ma

    /* load src data */
    vle8.v v_src, (x_src)

    /* split 4-bit lo; 4-bit hi */
    vand.vi v_src_lo, v_src, 0x0F
    vsrl.vi v_src_hi, v_src, 4

    /* load dest data */
    vle8.v v_dest, (x_dest)

    /* table indexing, ie. gf(2^8) multiplication */
    /* RISC-V RVV does not have tbl instruction, use vrgather.vv */
    vrgather.vv v_tmp1_lo, v_gft1_lo, v_src_lo
    vrgather.vv v_tmp1_hi, v_gft1_hi, v_src_hi

    /* exclusive or, ie. gf(2^8) add */
    vxor.vv v_dest, v_tmp1_lo, v_dest
    vxor.vv v_dest, v_tmp1_hi, v_dest

    /* store dest data */
    vse8.v v_dest, (x_dest)

    /* increment one vector length */
    add x_pos, x_pos, t2
    add x_src, x_src, t2
    add x_dest, x_dest, t2

    blt x_pos, x_len, .Lloop_rvv_vl

.return_pass:
    li w_ret, 0
    ret

.return_fail:
    li w_ret, 1
    ret

#endif
