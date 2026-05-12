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
    beqz        a1, ret0                 # len == 0, return 0
    addi        t1, a0, -2               # vects - 2
    blez        t1, ret1                 # vects < 3, return 1

    slli        t0, a0, 3                # t0 = vects * 8
    add         t0, a2, t0               # array + vects * 8
    li          t2, 0                    # byte offset
    mv          t6, a1                   # remaining length

outer_strip:
    vsetvli     t4, t6, e8, m8, ta, ma

    ld          a3, 0(a2)                # array[0]
    add         a3, a3, t2
    vle8.v      v0, (a3)

    addi        t3, a2, 8                # &array[1]
    mv          t5, t1                   # inner source count

inner_src:
    ld          a3, 0(t3)                # array[j]
    add         a3, a3, t2
    vle8.v      v8, (a3)
    vxor.vv     v0, v0, v8
    addi        t3, t3, 8
    addi        t5, t5, -1
    bnez        t5, inner_src

    ld          a3, -8(t0)               # dest = array[vects-1]
    add         a3, a3, t2
    vse8.v      v0, (a3)

    sub         t6, t6, t4
    add         t2, t2, t4
    bnez        t6, outer_strip

ret0:
    li a0, 0
    ret

ret1:
    li a0, 1
    ret

#endif
