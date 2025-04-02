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
#include "ec_table.S"
.option         arch, +v
.global         gf_vect_mul_rvv
.type           gf_vect_mul_rvv, %function
gf_vect_mul_rvv:
  li            t4, 32
  rem           t4, a0, t4
  bnez          t4, ret1                // (len % 32) != 0

  lla           t0, gff_base
  lla           t1, gflog_base
  lbu           t2, 1(a1)               // unsigned char c = a[1];
  beqz          t2, 2f
  add           t2, t1, t2              // &gflog_base[c]
  lbu           t2, (t2)                // gflog_base[c]

1:
  vsetvli       t6, a0, e8, m4, ta, ma
  vle8.v        v16, (a2)               // src
  vmsne.vi      v0, v16, 0              // if b == 0
  vluxei8.v     v16, (t1), v16, v0.t    // gflag_base[b]
  vwaddu.vx     v8, v16, t2, v0.t
  vmv.v.i       v16, 0
  vluxei16.v    v16, (t0), v8, v0.t     // gff_base[i]
  vse8.v        v16, (a3)
  add           a2, a2, t6
  add           a3, a3, t6
  sub           a0, a0, t6
  bnez          a0, 1b
  ret

2:
  vsetvli       t6, a0, e8, m8, ta, ma
  vmv.v.i       v0, 0
3:
  vsetvli       t6, a0, e8, m8, ta, ma
  vse8.v        v0, (a3)
  add           a3, a3, t6
  sub           a0, a0, t6
  bnez          a0, 3b
  ret

ret1:
  li            a0, -1
  ret

#endif
