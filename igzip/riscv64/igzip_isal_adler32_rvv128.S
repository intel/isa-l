/**********************************************************************
  Copyright (c) 2025 ZTE Corporation.

  Redistribution and use in source and binary forms, with or without
  modification, are permitted provided that the following conditions
  are met:
    * Redistributions of source code must retain the above copyright
      notice, this list of conditions and the following disclaimer.
    * Redistributions in binary form must reproduce the above copyright
      notice, this list of conditions and the following disclaimer in
      the documentation and/or other materials provided with the
      distribution.
    * Neither the name of ZTE Corporation nor the names of its
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
.text
.align          3
.option         arch, +v
.globl          adler32_rvv128
.type           adler32_rvv128, @function
adler32_rvv128:
    li      t3, 0x80078071          // Barrett reduction magic constant
    li      t4, 65521
    slli    t5, a0, 48
    srli    t5, t5, 48              // t5: A = adler32 & 0xffff
    srliw   t6, a0, 16              // t6: B = adler32 >> 16
    li      t0, 32
    bltu    a2, t0, tail_bytes
    vsetvli zero, t0, e32, m8, ta, ma
    vmv.v.i v8, 0
    vmv.v.i v24, 0
    vsetvli zero, t0, e8, m2, ta, ma
    la      a7, factors
    vle8.v  v0, (a7)
    vmv.v.i v4, 0
    srli    t1, a2, 5               // t1 = length / 32

outer_loop:
    beqz    t1, tail_bytes
    vmv.v.i v24, 0
    li      t2, 173
    bgeu    t1, t2, 1f
    mv      t2, t1
1:
    slli    a7, t2, 5
    add     a7, a1, a7

inner_loop:
    slli    a5, t5, 5
    add     t6, t6, a5
    vsetvli zero, t0, e8, m1, ta, ma
    vle8.v  v2, (a1)
    addi    a1, a1, 16
    vle8.v  v3, (a1)
    addi    a1, a1, 16
    vwredsumu.vs v12, v2, v4
    vwredsumu.vs v12, v3, v12
    vwmulu.vv v16, v2, v0
    vwmulu.vv v18, v3, v1

    vsetvli zero, t0, e16, m2, ta, ma
    vwaddu.wv v24, v24, v16
    vwaddu.wv v24, v24, v18
    vmv.x.s a6, v12
    add     t5, t5, a6
    bne     a1, a7, inner_loop

    vsetvli zero, t0, e32, m4, ta, ma
    vredsum.vs v16, v24, v4
    vmv.x.s a6, v16
    add     t6, t6, a6
    mul     a3, t5, t3
    srli    a3, a3, 47
    mul     a4, a3, t4
    sub     t5, t5, a4              // A %= 65521
    mul     a3, t6, t3
    srli    a3, a3, 47
    mul     a4, a3, t4
    sub     t6, t6, a4              // B %= 65521
    sub     t1, t1, t2
    j       outer_loop

tail_bytes:
    andi    a2, a2, 31
    beqz    a2, finalize
    add     a2, a1, a2

tail_loop:
    beq     a1, a2, finalize
    lbu     a3, 0(a1)
    add     t5, t5, a3
    add     t6, t6, t5
    addi    a1, a1, 1
    j       tail_loop

finalize:
    mul     a3, t5, t3
    srli    a3, a3, 47
    mul     a4, a3, t4
    sub     t5, t5, a4
    mul     a3, t6, t3
    srli    a3, a3, 47
    mul     a4, a3, t4
    sub     t6, t6, a4
    slli    t6, t6, 16
    or      a0, t5, t6

    ret
    .size adler32_rvv128, .-adler32_rvv128

    .section .rodata
    .align 4
factors:
    .byte 32, 31, 30, 29, 28, 27, 26, 25
    .byte 24, 23, 22, 21, 20, 19, 18, 17
    .byte 16, 15, 14, 13, 12, 11, 10, 9
    .byte 8, 7, 6, 5, 4, 3, 2, 1
#endif
