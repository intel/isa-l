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
.global gf_2vect_dot_prod_rvv
.type gf_2vect_dot_prod_rvv, @function

/* void gf_2vect_dot_prod_rvv(int len, int vlen, unsigned char *gftbls,
                              unsigned char **src, unsigned char **dest);
 */

/* arguments */
#define x_len a0  /* vector length */
#define x_vec a1  /* number of source vectors (ie. data blocks) */
#define x_tbl a2
#define x_src a3
#define x_dest a4

/* local variables */
#define x_vec_i t0
#define x_ptr t1
#define x_pos t2

#define x_tbl1 t3
#define x_tbl2 t4
#define x_dest1 t5
#define x_dest2 a7
#define x_remain s0

/* vectors */
#define v_src v1
#define v_src_lo v2
#define v_src_hi v3

#define v_dest1 v4

#define v_gft1_lo v5
#define v_gft1_hi v6

#define v_gft2_lo v7
#define v_gft2_hi v8
#define v_dest2 v9

gf_2vect_dot_prod_rvv:
    /* less than 16 bytes, return_fail */
    li t6, 16
    blt x_len, t6, .return_fail

    /* save callee-saved registers */
    addi sp, sp, -8
    sd s0, 0(sp)

    li x_pos, 0
    slli x_vec, x_vec, 3

    ld x_dest1, 0(x_dest)
    ld x_dest2, 8(x_dest)

/* Loop 1: x_len, vector length */
.Llooprvv_vl:
    bge x_pos, x_len, .return_pass

    sub x_remain, x_len, x_pos
    vsetvli a5, x_remain, e8, m1, ta, ma

    li x_vec_i, 0              /* clear x_vec_i */
    ld x_ptr, 0(x_src)         /* x_ptr: src base addr. */

    vmv.v.i v_dest1, 0         /* clear v_dest1 */
    vmv.v.i v_dest2, 0         /* clear v_dest2 */

    /* gf_tbl base = (x_tbl + dest_idx * x_vec * 32) shifting left 5 is split shifting 3 then shifting 2 */
    mv x_tbl1, x_tbl           /* reset x_tbl1 */
    slli t6, x_vec, 2
    add x_tbl2, x_tbl1, t6     /* reset x_tbl2 */

/* Loop 2: x_vec, number of source vectors (ie. data blocks) */
.Llooprvv_vl_vects:
    /* load src data */
    add x_ptr, x_ptr, x_pos

    vle8.v v_src, (x_ptr)      /* load from: src base + pos offset */
    /* split 4-bit lo; 4-bit hi */
    vand.vi v_src_lo, v_src, 0x0F
    vsrl.vi v_src_hi, v_src, 4

    vsetivli zero, 16, e8, m1, ta, ma
    /* gf_tbl addr: (x_tbl + dest_idx * x_vec * 32) + src_vec_idx * 32 */
    /* load gf_table's */
    vle8.v v_gft1_lo, (x_tbl1)
    addi x_tbl1, x_tbl1, 16
    vle8.v v_gft1_hi, (x_tbl1)
    addi x_tbl1, x_tbl1, 16

    vle8.v v_gft2_lo, (x_tbl2)
    addi x_tbl2, x_tbl2, 16
    vle8.v v_gft2_hi, (x_tbl2)
    addi x_tbl2, x_tbl2, 16
    vsetvli zero, a5, e8, m1, ta, ma

    /* calc for next */
    addi x_vec_i, x_vec_i, 8   /* move x_vec_i to next */
    add a6, x_src, x_vec_i
    ld x_ptr, 0(a6)

    /* dest 1 */
    /* table indexing, ie. gf(2^8) multiplication */
    vrgather.vv v26, v_gft1_lo, v_src_lo
    vrgather.vv v27, v_gft1_hi, v_src_hi
    /* exclusive or, ie. gf(2^8) add */
    vxor.vv v_dest1, v_dest1, v26
    vxor.vv v_dest1, v_dest1, v27

    /* dest 2 */
    vrgather.vv v26, v_gft2_lo, v_src_lo
    vrgather.vv v27, v_gft2_hi, v_src_hi
    vxor.vv v_dest2, v_dest2, v26
    vxor.vv v_dest2, v_dest2, v27

    blt x_vec_i, x_vec, .Llooprvv_vl_vects
/* end of Loop 2 */

    /* store dest data */
    vse8.v v_dest1, (x_dest1)
    vse8.v v_dest2, (x_dest2)
    add x_dest1, x_dest1, a5
    add x_dest2, x_dest2, a5

    /* increment one vector length */
    add x_pos, x_pos, a5
    j .Llooprvv_vl
/* end of Loop 1 */

.return_pass:
    ld s0, 0(sp)
    addi sp, sp, 8
    li a0, 0
    ret

.return_fail:
    li a0, 1
    ret

#endif
