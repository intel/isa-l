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

.global gf_6vect_dot_prod_rvv
.type gf_6vect_dot_prod_rvv, @function

/* void gf_6vect_dot_prod_rvv(int len, int vlen, unsigned char *gftbls,
                             unsigned char **src, unsigned char **dest);
 */

/* arguments */
#define x_len a0  /* vector length */
#define x_vec a1  /* number of source vectors (ie. data blocks) */
#define x_tbl a2  /* gftbls */
#define x_src a3  /* src */
#define x_dest a4 /* dest */

/* local variables */
#define x_vec_i a7  /* loop counter for vectors */
#define x_ptr t1     /* pointer to current src */
#define x_pos t2     /* position in vector */
#define x_tbl1 t3    /* table pointer 1 */
#define x_tbl2 t4    /* table pointer 2 */
#define x_tbl3 t5    /* table pointer 3 */
#define x_tbl4 t6    /* table pointer 4 */
#define x_tbl5 s0    /* table pointer 5 */
#define x_tbl6 s1    /* table pointer 6 */
#define x_dest1 s2  /* dest pointer 1   */
#define x_dest2 s3  /* dest pointer 2   */
#define x_dest3 s4  /* dest pointer 3 */
#define x_dest4 s5  /* dest pointer 4  t12  -- x28 */
#define x_dest5 s6  /* dest pointer 5 */
#define x_dest6 s7  /* dest pointer 6 */

/* vector registers */
#define v_src v1     /* source vector */
#define v_src_lo v2  /* low 4 bits of source */
#define v_src_hi v3  /* high 4 bits of source */
#define v_dest1 v4   /* destination vector 1 */
#define v_dest2 v5   /* destination vector 2 */
#define v_dest3 v6   /* destination vector 3 */
#define v_dest4 v7   /* destination vector 4 */
#define v_dest5 v8   /* destination vector 5 */
#define v_dest6 v9   /* destination vector 6 */
#define v_gft1_lo v10 /* gf table 1 low */
#define v_gft1_hi v11 /* gf table 1 high */
#define v_gft2_lo v12 /* gf table 2 low */
#define v_gft2_hi v13 /* gf table 2 high */
#define v_gft3_lo v14 /* gf table 3 low */
#define v_gft3_hi v15 /* gf table 3 high */
#define v_gft4_lo v16 /* gf table 4 low */
#define v_gft4_hi v17 /* gf table 4 high */
#define v_gft5_lo v18 /* gf table 5 low */
#define v_gft5_hi v19 /* gf table 5 high */
#define v_gft6_lo v20 /* gf table 6 low */
#define v_gft6_hi v21 /* gf table 6 high */

gf_6vect_dot_prod_rvv:
    /* less than 16 bytes, return_fail */
    li t0, 16
    blt x_len, t0, .return_fail

    /* save callee-saved registers */
    addi sp, sp, -64
    sd s0, 0(sp)
    sd s1, 8(sp)
    sd s2, 16(sp)
    sd s3, 24(sp)
    sd s4, 32(sp)
    sd s5, 40(sp)
    sd s6, 48(sp)
    sd s7, 56(sp)

    li t0, 0x0F
    vsetvli a5, x0, e8, m1, ta, ma

    /* initialize position */
    li x_pos, 0

    slli x_vec, x_vec, 3

    /* load destination pointers */
    ld x_dest1, 0(x14)  #  a4 is also x14
    ld x_dest2, 8(x_dest)
    ld x_dest3, 16(x_dest)
    ld x_dest4, 24(x_dest)
    ld x_dest5, 32(x_dest)
    ld x_dest6, 40(x_dest)

.Llooprvv_vl:
    /* check if we have processed all elements */
    bge x_pos, x_len, .return_pass

    /* initialize vector loop counter */
    li x_vec_i, 0

    /* load source pointer */
    ld x_ptr, 0(x_src)

    /* clear destination vectors */
    vmv.v.i v_dest1, 0
    vmv.v.i v_dest2, 0
    vmv.v.i v_dest3, 0
    vmv.v.i v_dest4, 0
    vmv.v.i v_dest5, 0
    vmv.v.i v_dest6, 0

    /* initialize table pointers */
    /* gf_tbl base = (x_tbl + dest_idx * x_vec * 32) shifting left 5 is split shifting 3 then shifting 2 */
    mv x_tbl1, x_tbl
    slli t0, x_vec, 2
    add x_tbl2, x_tbl1, t0
    add x_tbl3, x_tbl2, t0
    add x_tbl4, x_tbl3, t0
    add x_tbl5, x_tbl4, t0
    add x_tbl6, x_tbl5, t0

.Llooprvv_vl_vects:
    /* load source data */
    add x_ptr, x_ptr, x_pos
    vle8.v v_src, (x_ptr)

    /* split 4-bit lo; 4-bit hi */
    vand.vi v_src_lo, v_src, 0x0F
    vsrl.vi v_src_hi, v_src, 4

    /* load gf_table's */
    vle8.v v_gft1_lo, (x_tbl1)
    addi x_tbl1, x_tbl1, 16
    vle8.v v_gft1_hi, (x_tbl1)
    addi x_tbl1, x_tbl1, 16

    vle8.v v_gft2_lo, (x_tbl2)
    addi x_tbl2, x_tbl2, 16
    vle8.v v_gft2_hi, (x_tbl2)
    addi x_tbl2, x_tbl2, 16

    /* load next source pointer */
    addi x_vec_i, x_vec_i, 8
    add a6, x_src, x_vec_i
    ld x_ptr, 0(a6)

    vle8.v v_gft3_lo, (x_tbl3)
    addi x_tbl3, x_tbl3, 16
    vle8.v v_gft3_hi, (x_tbl3)
    addi x_tbl3, x_tbl3, 16

    vle8.v v_gft4_lo, (x_tbl4)
    addi x_tbl4, x_tbl4, 16
    vle8.v v_gft4_hi, (x_tbl4)
    addi x_tbl4, x_tbl4, 16

    vle8.v v_gft5_lo, (x_tbl5)
    addi x_tbl5, x_tbl5, 16
    vle8.v v_gft5_hi, (x_tbl5)
    addi x_tbl5, x_tbl5, 16

    vle8.v v_gft6_lo, (x_tbl6)
    addi x_tbl6, x_tbl6, 16
    vle8.v v_gft6_hi, (x_tbl6)
    addi x_tbl6, x_tbl6, 16

    /* dest 1 */
    vrgather.vv v26, v_gft1_lo, v_src_lo
    vrgather.vv v27, v_gft1_hi, v_src_hi
    vxor.vv v_dest1, v_dest1, v26
    vxor.vv v_dest1, v_dest1, v27

    /* dest 2 */
    vrgather.vv v26, v_gft2_lo, v_src_lo
    vrgather.vv v27, v_gft2_hi, v_src_hi
    vxor.vv v_dest2, v_dest2, v26
    vxor.vv v_dest2, v_dest2, v27

    /* GF multiplication and accumulation for dest3 */
    vrgather.vv v26, v_gft3_lo, v_src_lo
    vrgather.vv v27, v_gft3_hi, v_src_hi
    vxor.vv v_dest3, v_dest3, v26
    vxor.vv v_dest3, v_dest3, v27

    /* GF multiplication and accumulation for dest4 */
    vrgather.vv v26, v_gft4_lo, v_src_lo
    vrgather.vv v27, v_gft4_hi, v_src_hi
    vxor.vv v_dest4, v_dest4, v26
    vxor.vv v_dest4, v_dest4, v27

    /* GF multiplication and accumulation for dest5 */
    vrgather.vv v26, v_gft5_lo, v_src_lo
    vrgather.vv v27, v_gft5_hi, v_src_hi
    vxor.vv v_dest5, v_dest5, v26
    vxor.vv v_dest5, v_dest5, v27

    /* GF multiplication and accumulation for dest6 */
    vrgather.vv v26, v_gft6_lo, v_src_lo
    vrgather.vv v27, v_gft6_hi, v_src_hi
    vxor.vv v_dest6, v_dest6, v26
    vxor.vv v_dest6, v_dest6, v27

    /* check if we have processed all vectors */
    blt x_vec_i, x_vec, .Llooprvv_vl_vects

    /* store destination data */
    vse8.v v_dest1, (x_dest1)  # x_dest1   v_dest1==v4
    vse8.v v_dest2, (x_dest2)  #x_dest2
    vse8.v v_dest3, (x_dest3)  #x_dest3
    vse8.v v_dest4, (x_dest4)  # x_dest4
    vse8.v v_dest5, (x_dest5)  # x_dest5
    vse8.v v_dest6, (x_dest6)  # x_dest6

    add x_dest1, x_dest1, a5
    add x_dest2, x_dest2, a5
    add x_dest3, x_dest3, a5
    add x_dest4, x_dest4, a5
    add x_dest5, x_dest5, a5
    add x_dest6, x_dest6, a5

    /* increment position */
    add x_pos, x_pos, a5
    j .Llooprvv_vl

.return_pass:
    /* restore callee-saved registers */
    ld s0, 0(sp)
    ld s1, 8(sp)
    ld s2, 16(sp)
    ld s3, 24(sp)
    ld s4, 32(sp)
    ld s5, 40(sp)
    ld s6, 48(sp)
    ld s7, 56(sp)
    addi sp, sp, 64

    li a0, 0
    ret

.return_fail:
    li a0, 1
    ret

#endif
