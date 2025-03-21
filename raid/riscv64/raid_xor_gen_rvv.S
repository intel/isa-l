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
    * Neither the name of ISCAS Corporation nor the names of its
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
    beqz        a1, ret0                 # len <= 0, return 0
    addi        t1, a0, -2               # vects - 3
    blez        t1, ret1                 # vects < 3, return 1

    slli        t0, a0, 3                # t0 = vects * 8
    add         t0, a2, t0               # array + vects * 8
    ld          a4, 0(a2)                # src[0]
    ld          a3, -8(t0)               # dest = array[vects - 1]
    mv          t5, a3                   # save dest
    mv          t6, a1                   # save len

init_dest:
    vsetvli     t4, t6, e8, m8, ta, ma
    vle8.v      v0, (a4)                 # load src[0]
    vse8.v      v0, (a3)                 # dest
    sub         t6, t6, t4
    add         a4, a4, t4
    add         a3, a3, t4
    bnez        t6, init_dest

outer_j:
    mv          a3, t5                   # restore dest
    mv          t6, a1                   # restore length
    ld          a4, -16(t0)

inner_len:
    vsetvli     t4, t6, e8, m8, ta, ma
    vle8.v      v0, (a4)                 # src[j]
    vle8.v      v8, (a3)                 # dest
    vxor.vv     v8, v8, v0               # dest ^= src[j]
    vse8.v      v8, (a3)
    sub         t6, t6, t4
    add         a4, a4, t4
    add         a3, a3, t4
    bnez        t6, inner_len

    addi        t1, t1, -1
    addi        t0, t0, -8
    bnez        t1, outer_j

ret0:
    li a0, 0
    ret

ret1:
    li a0, 1
    ret

#endif
