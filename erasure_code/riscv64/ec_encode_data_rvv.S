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
.global         ec_encode_data_rvv
.type           ec_encode_data_rvv, %function
ec_encode_data_rvv:
  blez          a2, 3f
  blez          a0, 3f

  lla           t0, gff_base
  lla           t1, gflog_base
  addi          a3, a3, 1
  vsetvli       zero, a1, e8, mf2, ta, ma
  vmv.v.i       v20, 0
  li            t3, 32
  mv            a6, a0                   // backup len
  mv            a7, a5                   // backup dest

  csrr          t5, vlenb                // vlen/8
  srli          t5, t5, 1                // mf2: vlen/16
  blt           t5, a1, slow             // vlen/16(hardware) < vlen(software)

2:
  li            t2, 0                    // l
  vlse8.v       v24, (a3), t3            // v[j*32+1]
  vmsne.vi      v12, v24, 0              // if v == 0
  vluxei8.v     v24, (t1), v24           // gflag_base[v[]]
  ld            a5, (a5)

1:
  vsetvli       zero, zero, e8, mf2, ta, ma
  vle64.v       v16, (a4)                // src[j]
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
  sb            t5, (a5)                 // dest[0][i]
  addi          t2, t2, 1                // src[j][i]
  vmv.v.i       v20, 0
  addi          a5, a5, 1                // dest[i]
  bnez          a0, 1b

  addi          a2, a2, -1               // l(dests)
  addi          a7, a7, 8
  mv            a0, a6                   // restore len
  mv            a5, a7                   // update unsigned char **dest
  slli          t5, a1, 5                // += vlen * 32
  add           a3, a3, t5
  bnez          a2, 2b

  ret

slow:
  addi          sp, sp, -16
  sd            s2, 0(sp)
  sd            s3, 8(sp)
  mv            s3, a4                   // src
  mv            s2, a3                   // v
  mv            t4, a1                   // backup vlen

2:
  li            t2, 0                    // i < len
  ld            a5, (a5)

1:
  vsetvli       t6, a1, e8, mf2, ta, ma
  vle64.v       v16, (a4)                // src[j]
  vluxei64.v    v16, (t2), v16           // src[j][i]
  vlse8.v       v24, (a3), t3            // v[j*32+1]
  vmsne.vi      v12, v24, 0              // if v == 0
  vmsne.vi      v0, v16, 0               // if src == 0
  vmand.mm      v0, v0, v12              // if src == 0 || v == 0
  vluxei8.v     v16, (t1), v16, v0.t     // gflag_base[src[j][i]]
  vluxei8.v     v24, (t1), v24           // gflag_base[v[]]
  vwaddu.vv     v8, v16, v24, v0.t
  vmv.v.i       v16, 0
  vsetvli       zero, zero, e8, mf2, ta, mu
  vluxei16.v    v16, (t0), v8, v0.t      // gff_base[i]
  vxor.vv       v20, v16, v20
  sub           a1, a1, t6
  slli          t5, t6, 5
  add           a3, a3, t5               // v += 32 * vlen
  slli          t5, t6, 3
  add           a4, a4, t5               // src += 8 * vlen
  bnez          a1, 1b                   // for (j = 0; j < vlen; j++)

  vsetvli       zero, t4, e8, mf2, ta, ma
  vmv.s.x       v8, zero
  vredxor.vs    v8, v20, v8
  vmv.x.s       t5, v8
  addi          a0, a0, -1               // len
  sb            t5, (a5)                 // dest[0][i]
  addi          t2, t2, 1                // src[j][i]
  vmv.v.i       v20, 0
  mv            a1, t4                   // restore vlen
  mv            a3, s2                   // restore v
  mv            a4, s3                   // restore src
  addi          a5, a5, 1                // dest[i]
  bnez          a0, 1b                   // for (i = 0; i < len; i++)

  addi          a2, a2, -1               // l(dests)
  addi          a7, a7, 8                // for (l = 0; l < dests; l++)
  mv            a0, a6                   // restore len
  mv            a5, a7
  slli          t5, t4, 5
  add           a3, a3, t5               // v += vlen * 32
  mv            s2, a3
  bnez          a2, 2b                   // for (l = 0; l < dests; l++) {

  ld            s2, 0(sp)
  ld            s3, 8(sp)
  addi          sp, sp, 16

3:
  ret

#endif
