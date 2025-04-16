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
.global         gf_vect_dot_prod_rvv
.type           gf_vect_dot_prod_rvv, %function
gf_vect_dot_prod_rvv:
  blez          a0, 3f

  lla           t0, gff_base
  lla           t1, gflog_base
  addi          a2, a2, 1
  vsetvli       zero, a1, e8, mf2, ta, ma
  vmv.v.i       v20, 0
  li            t2, 0
  li            t3, 32
  csrr          t5, vlenb                // vlen/8
  srli          t5, t5, 1                // mf2: vlen/16
  blt           t5, a1, slow             // vlen/16(hardware) < vlen(software)

  vlse8.v       v24, (a2), t3            // v[j*32+1]
  vmsne.vi      v12, v24, 0              // if v == 0
  vluxei8.v     v24, (t1), v24           // gflag_base[v[]]

1:
  vsetvli       zero, zero, e8, mf2, ta, ma
  vle64.v       v16, (a3)                // src[j]
  vluxei64.v    v16, (t2), v16           // src[j][i]
  vmsne.vi      v0, v16, 0               // if src == 0
  vmand.mm      v0, v0, v12              // if src == 0 || v == 0
  vluxei8.v     v16, (t1), v16, v0.t     // gflag_base[src[j][i]]
  vwaddu.vv     v8, v16, v24, v0.t
  vmv.v.i       v16, 0
  vsetvli       zero, zero, e8, mf2, ta, mu
  vluxei16.v    v16, (t0), v8, v0.t      // gff_base[i]
  vxor.vv       v20, v16, v20

  vmv.s.x       v8, zero
  vredxor.vs    v8, v20, v8
  vmv.x.s       t5, v8
  addi          a0, a0, -1               // len
  sb            t5, (a4)
  addi          t2, t2, 1                // src[j][i]
  vmv.v.i       v20, 0
  addi          a4, a4, 1                // dest[i]
  bnez          a0, 1b
  ret

slow:
  mv            a7, a3                   // src
  mv            a6, a2                   // v
  mv            t4, a1                   // vlen

1:
  vsetvli       t6, a1, e8, mf2, ta, ma
  vle64.v       v16, (a3)
  vluxei64.v    v16, (t2), v16           // src[j][i]
  vlse8.v       v24, (a2), t3            // v[j*32+1]
  vmsne.vi      v0, v16, 0               // if src == 0
  vmsne.vi      v12, v24, 0              // if v == 0
  vmand.mm      v0, v0, v12
  vluxei8.v     v16, (t1), v16, v0.t     // gflag_base[src[j][i]]
  vluxei8.v     v24, (t1), v24, v0.t     // gflag_base[v[]]
  vwaddu.vv     v8, v16, v24, v0.t
  vmv.v.i       v16, 0
  vsetvli       zero, zero, e8, mf2, ta, mu
  vluxei16.v    v16, (t0), v8, v0.t      // gff_base[i]
  vxor.vv       v20, v16, v20
  slli          t5, t6, 5
  add           a2, a2, t5               // v += 32 * vlen
  slli          t5, t6, 3
  add           a3, a3, t5               // src += 8 * vlen
  sub           a1, a1, t6               // vlen
  bnez          a1, 1b                   // for (j = 0; j < vlen; j++)

  vsetvli       zero, t4, e8, mf2, ta, mu
  vmv.s.x       v8, zero
  vredxor.vs    v8, v20, v8
  vmv.x.s       t5, v8
  addi          a0, a0, -1               // len
  mv            a3, a7                   // src
  mv            a2, a6                   // v
  mv            a1, t4                   // vlen
  addi          t2, t2, 1                // i
  sb            t5, (a4)
  vmv.v.i       v20, 0
  addi          a4, a4, 1                // dest[i]
  bnez          a0, 1b                   // for (i = 0; i < len; i++) {

3:
  ret

#endif
