/**********************************************************************
  Copyright (c) 2025 Institute of Software Chinese Academy of Sciences (ISCAS).

  Redistribution and use in source and binary forms, with or without
  modification, are permitted provided that the following conditions
  are met:
    * Redistributions of source code must retain the above copyright
      notice, this list of conditions and the following disclaimer.
    * Redistributions in binary form must reproduce the above copyright
      notice, this list of conditions and the following disclaimer in
      the documentation and/or other materials provided with the
      distribution.
    * Neither the name of ISCAS nor the names of its
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
#if HAVE_RVV
.option         arch, +v
.global         xor_gen_rvv
.type           xor_gen_rvv, %function
xor_gen_rvv:
    beqz        a1, .Lret0
    li          t0, 2
    blt         a0, t0, .Lret1

    addi        t1, a0, -1                 # t1 = vects - 1
    slli        t1, t1, 3                  # t1 = t1 * 8
    add         t2, a2, t1
    ld          a7, 0(t2)                  # a7 = dest
    ld          a6, 0(a2)                  # a6 = src[0]
    li          t6, 0                      # t6 = offset
    mv          t5, a1
    addi        t0, a0, -2                 # t0 = vects - 2
    vsetvli     t4, zero, e8, m2, ta, ma
    slli        t3, t4, 3                  # t3 = 8 * vlmax

.Lunrolled_chunk_loop:
    blt         t5, t3, .Ltail_loop

    vsetvli     zero, t4, e8, m2, ta, ma 
    add         t1, a6, t6                 # t1 = src[0] + offset
    vle8.v      v0,  (t1)
    add         t1, t1, t4
    vle8.v      v2,  (t1)
    add         t1, t1, t4
    vle8.v      v4,  (t1)
    add         t1, t1, t4
    vle8.v      v6,  (t1)
    add         t1, t1, t4
    vle8.v      v8,  (t1)
    add         t1, t1, t4
    vle8.v      v10, (t1)
    add         t1, t1, t4
    vle8.v      v12, (t1)
    add         t1, t1, t4
    vle8.v      v14, (t1)
    beqz        t0, .Lstore_unrolled

    mv          a4, t0
    addi        a5, a2, 8

.Lunrolled_sources_loop:
    ld          t1, 0(a5)
    add         t1, t1, t6

    vle8.v      v16, (t1)
    add         t1, t1, t4
    vxor.vv     v0, v0, v16

    vle8.v      v18, (t1)
    add         t1, t1, t4
    vxor.vv     v2, v2, v18

    vle8.v      v20, (t1)
    add         t1, t1, t4
    vxor.vv     v4, v4, v20

    vle8.v      v22, (t1)
    add         t1, t1, t4
    vxor.vv     v6, v6, v22

    vle8.v      v24, (t1)
    add         t1, t1, t4
    vxor.vv     v8, v8, v24

    vle8.v      v26, (t1)
    add         t1, t1, t4
    vxor.vv     v10, v10, v26

    vle8.v      v28, (t1)
    add         t1, t1, t4
    vxor.vv     v12, v12, v28

    vle8.v      v30, (t1)
    vxor.vv     v14, v14, v30

    addi        a5, a5, 8
    addi        a4, a4, -1
    bnez        a4, .Lunrolled_sources_loop

.Lstore_unrolled:
    add         t1, a7, t6
    vse8.v      v0,  (t1)
    add         t1, t1, t4
    vse8.v      v2,  (t1)
    add         t1, t1, t4
    vse8.v      v4,  (t1)
    add         t1, t1, t4
    vse8.v      v6,  (t1)
    add         t1, t1, t4
    vse8.v      v8,  (t1)
    add         t1, t1, t4
    vse8.v      v10, (t1)
    add         t1, t1, t4
    vse8.v      v12, (t1)
    add         t1, t1, t4
    vse8.v      v14, (t1)
    add         t6, t6, t3
    sub         t5, t5, t3
    j           .Lunrolled_chunk_loop

.Ltail_loop:
    beqz        t5, .Lret0

    vsetvli     t4, t5, e8, m2, ta, ma   
    add         t1, a6, t6
    vle8.v      v0, (t1)
    beqz        t0, .Lstore_tail

    mv          a4, t0
    addi        a5, a2, 8

.Ltail_sources_loop:
    ld          t1, 0(a5)
    add         t1, t1, t6
    vle8.v      v2, (t1)
    vxor.vv     v0, v0, v2
    addi        a5, a5, 8
    addi        a4, a4, -1
    bnez        a4, .Ltail_sources_loop

.Lstore_tail:
    add         t1, a7, t6
    vse8.v      v0, (t1)
    add         t6, t6, t4
    sub         t5, t5, t4
    j           .Ltail_loop

.Lret0:
    li          a0, 0
    ret

.Lret1:
    li          a0, 1
    ret

#endif
