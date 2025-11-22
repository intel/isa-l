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
    addi    sp, sp, -32
    sd      ra, 24(sp)
    sd      s1, 16(sp)
    sd      s2, 8(sp)
    sd      s3, 0(sp)

    li      t3, 0x80078071          // Barrett reduction magic constant
    li      t4, 65521
    slli    s1, a0, 48
    srli    s1, s1, 48              // s1: A = adler32 & 0xffff
    srliw   s2, a0, 16              // s2: B = adler32 >> 16
    add     s3, a1, a2              // s3 = end
    li      t0, 32
    bltu    a2, t0, tail_bytes

    vsetvli zero, t0, e8, m2, ta, ma
    la      t6, factors
    vle8.v  v0, (t6)
    vmv.v.i v4, 0
    vmv.v.i v8, 0
    srli    t1, a2, 5               // t1 = length / 32

outer_loop:
    beqz    t1, tail_bytes
    li      t2, 173
    bgeu    t1, t2, 1f
    mv      t2, t1
1:
    mv      a3, s1
    mv      a4, s2
    slli    t6, t2, 5
    add     t6, a1, t6

inner_loop:
    vle8.v  v2, (a1)
    addi    a1, a1, 32
    slli    a5, a3, 5
    add     a4, a4, a5
    vwredsumu.vs v12, v2, v4
    vwmulu.vv v16, v2, v0

    vsetvli zero, t0, e16, m4, ta, ma
    vmv.x.s a6, v12
    add     a3, a3, a6
    vwredsumu.vs v20, v16, v8

    vsetvli zero, t0, e32, m4, ta, ma
    vmv.x.s a6, v20
    add     a4, a4, a6              // B += weighted_sum

    vsetvli zero, t0, e8, m2, ta, ma
    bne     a1, t6, inner_loop

    mv      s1, a3
    mv      s2, a4
    mul     a5, s1, t3
    srli    a5, a5, 47
    mul     a6, a5, t4
    sub     s1, s1, a6              // A %= 65521
    mul     a5, s2, t3
    srli    a5, a5, 47
    mul     a6, a5, t4
    sub     s2, s2, a6              // B %= 65521
    sub     t1, t1, t2
    j       outer_loop

tail_bytes:
    andi    a2, a2, 31
    beqz    a2, finalize
    add     a2, a1, a2

tail_loop:
    beq     a1, a2, finalize
    lbu     a3, 0(a1)
    add     s1, s1, a3
    add     s2, s2, s1
    addi    a1, a1, 1
    j       tail_loop

finalize:
    mul     a3, s1, t3
    srli    a3, a3, 47
    mul     a4, a3, t4
    sub     s1, s1, a4
    mul     a3, s2, t3
    srli    a3, a3, 47
    mul     a4, a3, t4
    sub     s2, s2, a4
    slli    s2, s2, 16
    or      a0, s1, s2

    ld      ra, 24(sp)
    ld      s1, 16(sp)
    ld      s2, 8(sp)
    ld      s3, 0(sp)
    addi    sp, sp, 32
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
