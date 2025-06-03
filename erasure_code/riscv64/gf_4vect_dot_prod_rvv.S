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

.global gf_4vect_dot_prod_rvv
.type gf_4vect_dot_prod_rvv, @function

/* void gf_4vect_dot_prod_rvv(int len, int vlen, unsigned char *gftbls,
                             unsigned char **src, unsigned char **dest);
 */

/* arguments */
#define x_len a0  /* vector length */
#define x_vec a1  /* number of source vectors (ie. data blocks) */
#define x_tbl a2
#define x_src a3
#define x_dest a4

/* local variables */
#define x_vec_i a7
#define x_ptr t1
#define x_pos t2

#define x_tbl1 t3
#define x_tbl2 t4
#define x_tbl3 t5
#define x_tbl4 t6
#define x_dest1 s0
#define x_dest2 s1
#define x_dest3 s2
#define x_dest4 s3
#define t_offset a5

/* vectors */
#define v_src v1
#define v_src_lo v2
#define v_src_hi v3

#define v_dest1 v4
#define v_dest2 v5
#define v_dest3 v6
#define v_dest4 v7

#define v_gft1_lo v8
#define v_gft1_hi v9
#define v_gft2_lo v10
#define v_gft2_hi v11
#define v_gft3_lo v12
#define v_gft3_hi v13
#define v_gft4_lo v14
#define v_gft4_hi v15

gf_4vect_dot_prod_rvv:
    /* less than 16 bytes, return_fail */
    li t0, 16
    blt x_len, t0, .return_fail

/* save callee-saved registers */
    addi sp, sp, -32
    sd s0, 0(sp)
    sd s1, 8(sp)
    sd s2, 16(sp)
    sd s3, 24(sp)

    vsetvli t0, x0, e8, m1  /* Set vector length to maximum */

    li x_pos, 0
    slli t_offset, x_vec, 5
    ld x_dest1, 0(x_dest)
    ld x_dest2, 8(x_dest)
    ld x_dest3, 16(x_dest)
    ld x_dest4, 24(x_dest)

/* Loop 1: x_len, vector length */
.Lloop_rvv_vl:
    /* check if we have processed all elements */
    bge x_pos, x_len, .return_pass

    /* Clear destination vectors */
    vmv.v.i v_dest1, 0
    vmv.v.i v_dest2, 0
    vmv.v.i v_dest3, 0
    vmv.v.i v_dest4, 0

    /* Reset table pointers */
    mv x_tbl1, x_tbl
    add x_tbl2, x_tbl1, t_offset
    add x_tbl3, x_tbl2, t_offset
    add x_tbl4, x_tbl3, t_offset

    /* Loop 2: x_vec, number of source vectors (ie. data blocks) */
    li x_vec_i, 0
.Lloop_rvv_vl_vects:
    /* Load source data */
    slli a6, x_vec_i, 3
    add a6,x_src,a6
    ld x_ptr, 0(a6)
    add x_ptr,x_ptr,x_pos

    vle8.v v_src, (x_ptr)

    /* Split 4-bit lo; 4-bit hi */
    vand.vi v_src_lo, v_src, 0x0F
    vsrl.vi v_src_hi, v_src, 4

    /* Load gf_table's */
    vle8.v v_gft1_lo, (x_tbl1)
    addi x_tbl1, x_tbl1, 16
    vle8.v v_gft1_hi, (x_tbl1)
    addi x_tbl1, x_tbl1, 16
    vle8.v v_gft2_lo, (x_tbl2)
    addi x_tbl2, x_tbl2, 16
    vle8.v v_gft2_hi, (x_tbl2)
    addi x_tbl2, x_tbl2, 16


    /* Load next gf_table's */
    vle8.v v_gft3_lo, (x_tbl3)
    addi x_tbl3, x_tbl3, 16
    vle8.v v_gft3_hi, (x_tbl3)
    addi x_tbl3, x_tbl3, 16

    vle8.v v_gft4_lo, (x_tbl4)
    addi x_tbl4, x_tbl4, 16
    vle8.v v_gft4_hi, (x_tbl4)
    addi x_tbl4, x_tbl4, 16

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

    /* dest 3 */
    vrgather.vv v26, v_gft3_lo, v_src_lo
    vrgather.vv v27, v_gft3_hi, v_src_hi
    vxor.vv v_dest3, v_dest3, v26
    vxor.vv v_dest3, v_dest3, v27

    /* dest 4 */
    vrgather.vv v26, v_gft4_lo, v_src_lo
    vrgather.vv v27, v_gft4_hi, v_src_hi
    vxor.vv v_dest4, v_dest4, v26
    vxor.vv v_dest4, v_dest4, v27

    /* Move to next source vector */
    addi x_vec_i, x_vec_i, 1

    /* Check if we have processed all vectors */
    blt x_vec_i, x_vec, .Lloop_rvv_vl_vects

    /* Store destination data */
    vse8.v v_dest1, (x_dest1)
    vse8.v v_dest2, (x_dest2)
    vse8.v v_dest3, (x_dest3)
    vse8.v v_dest4, (x_dest4)
    add x_dest1,x_dest1, t0
    add x_dest2,x_dest2, t0
    add x_dest3,x_dest3, t0
    add x_dest4,x_dest4, t0
    /* Increment position */
    add x_pos, x_pos, t0
    j .Lloop_rvv_vl

.return_pass:
/* restore callee-saved registers */
    ld s0, 0(sp)
    ld s1, 8(sp)
    ld s2, 16(sp)
    ld s3, 24(sp)
    addi sp, sp, 32
    li a0, 0
    ret

.return_fail:
    li a0, 1
    ret

#endif
