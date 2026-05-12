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
.global         pq_gen_rvv
.type           pq_gen_rvv, %function
pq_gen_rvv:
    srli        a1, a1, 3                  // blocks = len / 8
    beqz        a1, ret0                   // blocks <= 0
    addi        a6, a0, -3                 // vects - 3
    blez        a6, ret1                   // vects < 4

    slli        t0, a0, 3                  // t0 = vects * 8
    add         t0, a2, t0                 // array + vects * 8
    li          t1, 0x8080808080808080     // bit7
    li          t2, 0xfefefefefefefefe     // notbit0
    li          t3, 0x1d1d1d1d1d1d1d1d     // gf8poly
    li          a7, 0                      // byte offset

outer_strip:
    vsetvli     t4, a1, e64, m4, ta, ma

    ld          a3, -24(t0)                // src[vects-3]
    add         a3, a3, a7
    vle64.v     v0, (a3)                   // p
    vmv.v.v     v4, v0                     // q

    addi        a4, t0, -32                // &array[vects-4]
    mv          a5, a6                     // inner source count

inner_src:
    ld          a3, 0(a4)                  // src[j]
    add         a3, a3, a7
    vle64.v     v8, (a3)                   // s

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

    addi        a4, a4, -8
    addi        a5, a5, -1
    bnez        a5, inner_src

    ld          a3, -16(t0)                // p
    add         a3, a3, a7
    vse64.v     v0, (a3)

    ld          a3, -8(t0)                 // q
    add         a3, a3, a7
    vse64.v     v4, (a3)

    sub         a1, a1, t4
    slli        t5, t4, 3
    add         a7, a7, t5
    bnez        a1, outer_strip

ret0:
    li a0, 0
    ret

ret1:
    li a0, 1
    ret

#endif
