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
.global         pq_gen_rvv
.type           pq_gen_rvv, %function
pq_gen_rvv:
    srli        a1, a1, 3                  // blocks = len / 8
    beqz        a1, ret0                   // blocks <= 0
    addi        a6, a0, -3                 // j = vects - 4
    blez        a6, ret1                   // vects < 4

    slli        t0, a0, 3                  // t0 = vects * 8
    add         t0, a2, t0                 // array + vects * 8
    li          t1, 0x8080808080808080     // bit7
    li          t2, 0xfefefefefefefefe     // notbit0
    li          t3, 0x1d1d1d1d1d1d1d1d     // gf8poly
    ld          a3, -24(t0)                // src[vects-3]
    ld          a4, -16(t0)                // p
    ld          a5, -8(t0)                 // q
    mv          t6, a1                     // save blocks
    mv          t5, a4                     // save p
    mv          a7, a5                     // save q

init_pq:
    vsetvli     t4, t6, e64, m4, ta, ma
    vle64.v     v0, (a3)
    vse64.v     v0, (a4)                   // init p
    vse64.v     v0, (a5)                   // init q
    sub         t6, t6, t4
    slli        t4, t4, 3
    add         a3, a3, t4
    add         a4, a4, t4
    add         a5, a5, t4
    bnez        t6, init_pq

outer_j:
    mv          a4, t5                     // restore p
    mv          a5, a7                     // restore q
    mv          t6, a1                     // restore blocks
    ld          a0, -32(t0)                // src[j]

inner_block:
    vsetvli     t4, t6, e64, m4, ta, ma
    vle64.v     v8, (a0)                   // s
    vle64.v     v0, (a4)                   // p
    vle64.v     v4, (a5)                   // q
    vxor.vv     v0, v0, v8                 // p ^= s
    vand.vx     v20, v4, t1                // q & bit7
    vsll.vi     v24, v4, 1                 // (q << 1)
    vand.vx     v24, v24, t2               // (q << 1) & notbit0
    vsrl.vi     v16, v20, 7                // (q & bit7)>>7
    vsll.vi     v20, v20, 1                // (q & bit7)<<1
    vsub.vv     v20, v20, v16              // (q & bit7)<<1 - (q & bit7)>>7
    vand.vx     v20, v20, t3               // ((q & bit7)<<1 - (q & bit7)>>7) & gf8poly
    vxor.vv     v4, v24, v20               // ((q << 1) & notbit0) ^
    vxor.vv     v4, v4, v8                 // s^
    vse64.v     v0, (a4)                   // p
    vse64.v     v4, (a5)                   // q
    sub         t6, t6, t4                 // blocks
    slli        t4, t4, 3
    add         a4, a4, t4                 // p+=
    add         a5, a5, t4                 // q+=
    add         a0, a0, t4                 // s+=
    bnez        t6, inner_block

    addi        a6, a6, -1
    addi        t0, t0, -8
    bnez        a6, outer_j

ret0:
    li a0, 0
    ret

ret1:
    li a0, 1
    ret

#endif
