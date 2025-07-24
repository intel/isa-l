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
#if HAVE_ZBC && HAVE_ZVBC
#include "crc16_t10dif_vclmul.h"

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
    li t1, 64
    bgeu a3, t1, .crc_fold

.crc_table_loop_pre:
    beq a3, zero, .end
    la a7, .lanchor_crc_tab
    add a3, a3, a2

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
    bne a2, a3, .crc_table_loop

.end:
    slli a0, a0, 32
    srli a0, a0, 48
    ret

.crc_fold:
    vsetivli zero, 2, e64, m1, ta, ma
    vl4re64.v v4, 0(a2)
    addi a2, a2, 64
    addi a3, a3, -64

    vs4r.v v4, (a1)
    addi a1, a1, 64

    la t0, .shuffle_data_mask
    vsetivli zero, 16, e8, m1, ta, ma
    vle8.v v13, 0(t0)
    slli a0, a0, 32
    vrgather.vv v0, v4, v13
    vrgather.vv v1, v5, v13
    vrgather.vv v2, v6, v13
    vrgather.vv v3, v7, v13
    vsetivli zero, 2, e64, m1, ta, ma

    vmv.v.x v5, a0
    vmv.s.x v4, zero
    vslideup.vi v4, v5, 1
    la t2, .crc_loop_const
    vle64.v v5, 0(t2)
    vxor.vv v0, v0, v4
    bltu a3, t1, crc_fold_finalization

    li t0, 64

crc_fold_loop:
    vl4re64.v v8, (a2)
    addi a2, a2, 64
    vs4r.v v8, (a1)
    addi a1, a1, 64

    vclmul.vv v4, v0, v5
    vclmulh.vv v0, v0, v5
    vslidedown.vi v15, v4, 1
    vslidedown.vi v14, v0, 1
    vxor.vv v15, v15, v4
    vxor.vv v14, v14, v0
    vslideup.vi v15, v14, 1

    vclmul.vv v4, v1, v5
    vclmulh.vv v1, v1, v5
    vslidedown.vi v16, v4, 1
    vslidedown.vi v14, v1, 1
    vxor.vv v16, v16, v4
    vxor.vv v14, v14, v1
    vslideup.vi v16, v14, 1

    vclmul.vv v4, v2, v5
    vclmulh.vv v2, v2, v5
    vslidedown.vi v17, v4, 1
    vslidedown.vi v14, v2, 1
    vxor.vv v17, v17, v4
    vxor.vv v14, v14, v2
    vslideup.vi v17, v14, 1

    vclmul.vv v4, v3, v5
    vclmulh.vv v3, v3, v5
    vslidedown.vi v18, v4, 1
    vslidedown.vi v14, v3, 1
    vxor.vv v18, v18, v4
    vxor.vv v14, v14, v3
    vslideup.vi v18, v14, 1

    vsetivli zero, 16, e8, m1, ta, ma
    vrgather.vv v0, v8, v13
    vrgather.vv v1, v9, v13
    vrgather.vv v2, v10, v13
    vrgather.vv v3, v11, v13
    vsetivli zero, 2, e64, m1, ta, ma
    vxor.vv v0, v0, v15
    vxor.vv v1, v1, v16
    vxor.vv v2, v2, v17
    vxor.vv v3, v3, v18

    addi a3, a3, -64
    bge a3, t0, crc_fold_loop

crc_fold_finalization:
    # 512bit -> 128bit folding
    addi t2, t2, 16
    vle64.v v5, 0(t2)
    vclmul.vv v6, v0, v5
    vclmulh.vv v7, v0, v5
    vslidedown.vi v8, v6, 1
    vslidedown.vi v9, v7, 1
    vxor.vv v8, v8, v6
    vxor.vv v9, v9, v7
    vslideup.vi v8, v9, 1
    vxor.vv v0, v8, v1

    vclmul.vv v6, v0, v5
    vclmulh.vv v7, v0, v5
    vslidedown.vi v8, v6, 1
    vslidedown.vi v9, v7, 1
    vxor.vv v8, v8, v6
    vxor.vv v9, v9, v7
    vslideup.vi v8, v9, 1
    vxor.vv v0, v8, v2

    vclmul.vv v6, v0, v5
    vclmulh.vv v7, v0, v5
    vslidedown.vi v8, v6, 1
    vslidedown.vi v9, v7, 1
    vxor.vv v8, v8, v6
    vxor.vv v9, v9, v7
    vslideup.vi v8, v9, 1
    vxor.vv v0, v8, v3

    # 128bit -> 64bit folding
    vmv.x.s t0, v0
    vslidedown.vi v0, v0, 1
    vmv.x.s t1, v0
    li t2, const_low
    li t3, const_high
    clmul a4, t1, t2
    clmulh a5, t1, t2
    slli a6, t0, 32
    srli a7, t0, 32
    xor a4, a4, a6
    xor a5, a5, a7
    clmul a5, a5, t3
    xor a4, a4, a5

    # Barrett reduction
    srli a5, a4, 32
    li t2, const_quo
    clmul a5, t2, a5
    srli a5, a5, 32
    li t3, const_poly
    clmul a5, a5, t3
    xor a0, a5, a4

tail_processing:
    beqz a3, .end
    jal x0, .crc_table_loop_pre

.shuffle_data_mask = . + 0
    .type	shuffle_data, %object
    .size	shuffle_data, 16
shuffle_data:
    .byte   15, 14, 13, 12, 11, 10, 9, 8
    .byte   7, 6, 5, 4, 3, 2, 1, 0

#endif
