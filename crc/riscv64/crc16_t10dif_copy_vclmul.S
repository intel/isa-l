########################################################################
#  Copyright (c) 2025 ZTE Corporation.
#
#  Redistribution and use in source and binary forms, with or without
#  modification, are permitted provided that the following conditions
#  are met:
#    * Redistributions of source code must retain the above copyright
#      notice, this list of conditions and the following disclaimer.
#    * Redistributions in binary form must reproduce the above copyright
#      notice, this list of conditions and the following disclaimer in
#      the documentation and/or other materials provided with the
#      distribution.
#    * Neither the name of ZTE Corporation nor the names of its
#      contributors may be used to endorse or promote products derived
#      from this software without specific prior written permission.
#
#  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
#  "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
#  LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
#  A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
#  OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
#  SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
#  LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
#  DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
#  THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
#  (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
#  OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
#########################################################################
#if HAVE_ZBC && HAVE_ZVBC && HAVE_ZBB
#include "crc16_t10dif_vclmul.h"

.option arch, +v, +zvbc, +zbc, +zbb
.section .text
.align 2
.global crc16_t10dif_copy_vclmul

# Arguments:
# a0: uint16_t crc (seed)
# a1: uint8_t *dst (pointer to data)
# a2: uint8_t *src (pointer to data)
# a3: size_t len (data length)

crc16_t10dif_copy_vclmul:
    # initialize seed for calculation in CRC32 format
    slli a0, a0, 16
    li t1, 128
    bgeu a3, t1, .crc_fold

.crc_table_loop_pre:
    beq a3, zero, .end
    la a7, .lanchor_crc_tab
    add a6, a2, a3

.crc_table_loop:
    lbu a4, 0(a2)
    sb a4, 0(a1)
    addi a2, a2, 1
    addi a1, a1, 1
    slliw a4, a4, 0x18
    xor a4, a4, a0
    srliw a5, a4, 0x18
    slli a5, a5, 0x2
    add a5, a5, a7
    lw a0, 0(a5)
    slliw a4, a4, 0x8
    xor a0, a0, a4
    bne a2, a6, .crc_table_loop

.end:
    slli a0, a0, 32
    srli a0, a0, 48
    ret

.crc_fold:
    vsetivli zero, 2, e64, m1, ta, ma

    slli a0, a0, 32
    la t4, .shuffle_data_mask

    addi sp, sp, -128
    sd ra, 120(sp)
    sd gp, 112(sp)
    sd tp, 104(sp)
    sd s0, 96(sp)
    sd s1, 88(sp)
    sd s2, 80(sp)
    sd s3, 72(sp)
    sd s4, 64(sp)
    sd s5, 56(sp)
    sd s6, 48(sp)
    sd s7, 40(sp)
    sd s8, 32(sp)
    sd s9, 24(sp)
    sd s10, 16(sp)
    sd s11, 8(sp)

    ld s0, 0(a2)
    ld s1, 8(a2)
    ld s2, 16(a2)
    ld s3, 24(a2)
    ld s4, 32(a2)
    ld s5, 40(a2)
    ld s6, 48(a2)
    ld s7, 56(a2)
    sd s0, 0(a1)
    sd s1, 8(a1)
    sd s2, 16(a1)
    sd s3, 24(a1)
    sd s4, 32(a1)
    sd s5, 40(a1)
    sd s6, 48(a1)
    sd s7, 56(a1)
    addi a2, a2, 64
    addi a1, a1, 64
    vle64.v v8, (a2)
    addi a2, a2, 16
    vle64.v v9, (a2)
    addi a2, a2, 16
    vle64.v v10, (a2)
    addi a2, a2, 16
    vle64.v v11, (a2)
    addi a2, a2, 16
    vse64.v v8, (a1)
    addi a1, a1, 16
    vse64.v v9, (a1)
    addi a1, a1, 16
    vse64.v v10, (a1)
    addi a1, a1, 16
    vse64.v v11, (a1)
    addi a1, a1, 16
    rev8 s0, s0
    rev8 s1, s1
    rev8 s2, s2
    rev8 s3, s3
    rev8 s4, s4
    rev8 s5, s5
    rev8 s6, s6
    rev8 s7, s7
    vsetivli zero, 16, e8, m1, ta, ma
    vle8.v v23, 0(t4)
    vrgather.vv v1, v8, v23
    vrgather.vv v2, v9, v23
    vrgather.vv v3, v10, v23
    vrgather.vv v4, v11, v23
    vsetivli zero, 2, e64, m1, ta, ma

    andi a5, a3, ~127
    addi t0, a5, -128
    sub a3, a3, a5
    xor s0, s0, a0

    add a6, a2, t0
    la t4, .crc_loop_const
    ld a0, 0(t4)
    ld a4, 8(t4)
    vle64.v v5, 0(t4)
    la t0, .refl_sld_mask
    vle64.v v0, (t0)
    vrgather.vv v22, v5, v0
    beq a2, a6, crc_fold_finalization

crc_fold_loop:
    .align 3
.combine_8x16_norm_loop:
    clmul t0, a0, s0
    ld t4, 0(a2)
    clmul t1, a0, s2
    ld t5, 8(a2)
    clmul t2, a0, s4
    ld t6, 16(a2)
    clmul t3, a0, s6
    ld a7, 24(a2)
    clmulh s0, a0, s0
    ld s8, 32(a2)
    clmulh s2, a0, s2
    ld s9, 40(a2)
    clmulh s4, a0, s4
    ld s10, 48(a2)
    clmulh s6, a0, s6
    ld s11, 56(a2)

    // Use ra, gp, tp as temporaries to cover register shortage in loop
    clmulh a5, a4, s1
    clmulh ra, a4, s3
    clmulh gp, a4, s5
    clmulh tp, a4, s7
    clmul s1, a4, s1
    clmul s3, a4, s3
    clmul s5, a4, s5
    clmul s7, a4, s7
    sd t4, 0(a1)
    sd t5, 8(a1)
    sd t6, 16(a1)
    sd a7, 24(a1)
    sd s8, 32(a1)
    sd s9, 40(a1)
    sd s10, 48(a1)
    sd s11, 56(a1)
    addi a2, a2, 64
    vle64.v v10, (a2)
    addi a2, a2, 16
    vclmul.vv v6, v1, v22
    vclmulh.vv v1, v1, v22
    vle64.v v11, (a2)
    addi a2, a2, 16
    vclmul.vv v7, v2, v22
    vclmulh.vv v2, v2, v22
    vle64.v v12, (a2)
    addi a2, a2, 16
    vclmul.vv v8, v3, v22
    vclmulh.vv v3, v3, v22
    vle64.v v13, (a2)
    addi a2, a2, 16
    vclmul.vv v9, v4, v22
    vclmulh.vv v4, v4, v22
    addi a1, a1, 64
    vse64.v v10, (a1)
    addi a1, a1, 16
    vse64.v v11, (a1)
    addi a1, a1, 16
    vse64.v v12, (a1)
    addi a1, a1, 16
    vse64.v v13, (a1)
    addi a1, a1, 16
    vmv.v.v v14, v6
    vmv.v.v v15, v7
    vmv.v.v v16, v8
    vmv.v.v v17, v9
    vslideup.vi v6, v1, 1
    vslideup.vi v7, v2, 1
    vslideup.vi v8, v3, 1
    vslideup.vi v9, v4, 1
    vsetivli zero, 2, e64, m1, ta, mu
    vslidedown.vi v1, v14, 1, v0.t
    vslidedown.vi v2, v15, 1, v0.t
    vslidedown.vi v3, v16, 1, v0.t
    vslidedown.vi v4, v17, 1, v0.t
    vsetivli zero, 2, e64, m1, ta, ma

    rev8 t5, t5
    rev8 a7, a7
    rev8 s9, s9
    rev8 s11, s11
    rev8 t4, t4
    rev8 t6, t6
    rev8 s8, s8
    rev8 s10, s10
    vsetivli zero, 16, e8, m1, ta, ma
    vrgather.vv v18, v10, v23
    vrgather.vv v19, v11, v23
    vrgather.vv v20, v12, v23
    vrgather.vv v21, v13, v23
    vsetivli zero, 2, e64, m1, ta, ma

    xor t5, t5, t0
    xor a7, a7, t1
    xor s9, s9, t2
    xor s11, s11, t3
    xor s0, t4, s0
    xor s2, t6, s2
    xor s4, s8, s4
    xor s6, s10, s6
    xor s1, s1, t5
    xor s3, s3, a7
    xor s5, s5, s9
    xor s7, s7, s11
    xor s0, a5, s0
    xor s2, ra, s2
    xor s4, gp, s4
    xor s6, tp, s6

    vxor.vv v1, v1, v6
    vxor.vv v2, v2, v7
    vxor.vv v3, v3, v8
    vxor.vv v4, v4, v9
    vxor.vv v1, v1, v18
    vxor.vv v2, v2, v19
    vxor.vv v3, v3, v20
    vxor.vv v4, v4, v21

    bne a2, a6, .combine_8x16_norm_loop

crc_fold_finalization:
    mv t6, sp
    la t5, .crc_loop_const
    ld a0, 16(t5)
    ld a4, 24(t5)

    addi sp, sp, -16
    vse64.v v4, 0(sp)
    addi sp, sp, -16
    vse64.v v3, 0(sp)
    addi sp, sp, -16
    vse64.v v2, 0(sp)
    addi sp, sp, -16
    vse64.v v1, 0(sp)

    clmulh t0, a0, s0
    clmul t1, a0, s0
    clmulh t2, a4, s1
    clmul t3, a4, s1
    xor s0, s2, t0
    xor s0, s0, t2
    xor s1, s3, t1
    xor s1, s1, t3
    clmulh t0, a0, s0
    clmul t1, a0, s0
    clmulh t2, a4, s1
    clmul t3, a4, s1
    xor s0, s4, t0
    xor s0, s0, t2
    xor s1, s5, t1
    xor s1, s1, t3
    clmulh t4, a0, s0
    clmul t5, a0, s0
    clmulh t2, a4, s1
    clmul t3, a4, s1
    xor s0, s6, t4
    xor t0, s0, t2
    xor s1, s7, t5
    xor t1, s1, t3

    .align 3
.crc_1_fold:
    clmulh t4, a0, t0
    clmul t5, a0, t0
    clmulh t2, a4, t1
    clmul t3, a4, t1
    ld s0, 8(sp)
    ld s1, 0(sp)
    xor t0, s0, t4
    xor t0, t0, t2
    xor t1, s1, t5
    xor t1, t1, t3
    addi sp, sp, 16
    bne sp, t6, .crc_1_fold

    ld ra, 120(sp)
    ld gp, 112(sp)
    ld tp, 104(sp)
    ld s0, 96(sp)
    ld s1, 88(sp)
    ld s2, 80(sp)
    ld s3, 72(sp)
    ld s4, 64(sp)
    ld s5, 56(sp)
    ld s6, 48(sp)
    ld s7, 40(sp)
    ld s8, 32(sp)
    ld s9, 24(sp)
    ld s10, 16(sp)
    ld s11, 8(sp)
    addi sp, sp, 128

    # 128bit -> 64bit folding
    li t2, const_low
    li t3, const_high
    clmul t4, t0, t2
    clmulh t5, t0, t2
    slli t2, t1, 32
    srli t1, t1, 32
    xor t4, t4, t2
    xor t5, t5, t1
    clmul t5, t5, t3
    xor t4, t4, t5

    # Barrett reduction
    li t2, const_quo
    li t3, const_poly
    srli t1, t4, 32
    clmul t1, t2, t1
    srli t1, t1, 32
    clmul t1, t1, t3
    xor a0, t1, t4
    j .crc_table_loop_pre

.shuffle_data_mask = . + 0
    .type	shuffle_data, %object
    .size	shuffle_data, 16
shuffle_data:
    .byte   15, 14, 13, 12, 11, 10, 9, 8
    .byte   7, 6, 5, 4, 3, 2, 1, 0
.refl_sld_mask:
    .quad  1, 0
#endif
