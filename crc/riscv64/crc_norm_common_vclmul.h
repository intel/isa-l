########################################################################
#  Copyright(c) 2025 ZTE Corporation All rights reserved.
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

#include "crc_common_vclmul.h"

# Used for the calculation of CRC32 and CRC16
.macro crc_norm_func name
    .text
    .align  3
    .type \name, @function
    .global \name
\name:
    xori seed, seed, -1
    li counter, 0
    li t1, 64
    bgeu len, t1, .crc_clmul_pre

.crc_tab_pre:
    bgeu counter, len, .done
    la crc_tab_addr, .lanchor_crc_tab
    add buf_iter, buf, counter
    add buf, buf, len

    .align 3
.loop_crc_tab:
    lbu t1, 0(buf_iter)
    addi buf_iter, buf_iter, 1
    slliw t1, t1, 0x18
    xor t1, t1, seed
    srliw a7, t1, 0x18
    slli a7, a7, 0x2
    add a7, a7, crc_tab_addr
    lw seed, 0(a7)
    slliw t1, t1, 0x8
    xor seed, seed, t1
    bne buf_iter, buf, .loop_crc_tab

.done:
    xori seed, seed, -1
    li t6, 0xffffffff
    and seed, seed, t6
    ret

    .align 2
.crc_clmul_pre:
    vsetivli zero, 2, e64, m1, ta, ma
    slli seed, seed, 32
    crc_norm_load_first_block
    crc_load_p4
    addi t0, len, -64
    bltu t0, t1, .clmul_loop_end

    crc_norm_loop

.clmul_loop_end:
    addi t4, t4, 16
    # 512bit -> 128bit folding
    crc_fold_512b_to_128b

    # 128bit -> 64bit folding
    vmv.x.s t0, v0
    vslidedown.vi v0, v0, 1
    vmv.x.s t1, v0
    li t2, const_low
    li t3, const_high
    clmul t4, t1, t2
    clmulh t5, t1, t2
    slli t1, t0, 32
    srli t2, t0, 32
    xor t4, t4, t1
    xor t5, t5, t2
    clmul t5, t5, t3
    xor t4, t4, t5

    # Barrett reduction
    srli t1, t4, 32
    li t2, const_quo
    clmul t1, t2, t1
    srli t1, t1, 32
    li t3, const_poly
    clmul t1, t1, t3
    xor seed, t1, t4

    j .crc_tab_pre
    .size   \name, .-\name
    .section    .rodata.cst16,"aM",@progbits,16
    .align  4
.shuffle_data:
    .byte   15, 14, 13, 12, 11, 10, 9, 8
    .byte   7, 6, 5, 4, 3, 2, 1, 0
.endm
