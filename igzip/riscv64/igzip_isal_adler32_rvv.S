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
.global         adler32_rvv
.type           adler32_rvv, %function
adler32_rvv:
  slli          t2, a0, 48
  srli          t2, t2, 48                           // t2: A = adler32 & 0xffff;
  srliw         t3, a0, 16                           // t3: B = adler32 >> 16;
  beqz          a2, 2f

  vsetvli       zero, a2, e64, m8, tu, ma
  vmv.v.i       v8, 0
  vmv.v.i       v16, 0
  vmv.s.x       v24, zero
  mv            t6, a2                               // t6 = length
  vsetvli       zero, zero, e32, m4, tu, ma
  vmv.s.x       v8, t2                               // v8 = adler32 & 0xffff

1:
  vsetvli       t1, a2, e8, m1, tu, ma
  vle8.v        v0, (a1)
  vsetvli       zero, zero, e32, m4, tu, ma
  vzext.vf4     v4, v0
  vid.v         v12                                  // 0, 1, 2, .. vl-1
  vadd.vv       v8, v8, v4
  vrsub.vx      v12, v12, a2                         // len, len-1, len-2
  vwmaccu.vv    v16, v12, v4                         // v16: B += weight * next
  sub           a2, a2, t1
  add           a1, a1, t1
  bnez          a2, 1b

  vsetvli       zero, t6, e32, m4, tu, ma
  vwredsumu.vs  v24, v8, v24
  mul           a7, t6, t2                           // B += A(init) * len
  vsetvli       zero, t6, e64, m8, tu, ma
  vmv.s.x       v0, a7
  vredsum.vs    v0, v16, v0
  vmv.x.s       t4, v0                               // B = t4
  vmv.x.s       t2, v24                              // A = t2
  add           t3, t4, t3

2:
  li            t0, 65521
  remu          t2, t2, t0                           // A = A % ADLER_MOD
  remu          t3, t3, t0                           // B = B % ADLER_MOD
  slli          t3, t3, 16                           // B << 16
  add           a0, t2, t3                           // a0 = A + B

  ret
#endif
