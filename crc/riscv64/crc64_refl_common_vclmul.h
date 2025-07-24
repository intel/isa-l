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

.macro crc64_refl_func name
    .option arch, +v, +zvbc, +zbc, +zbb
    .text
    .align  3
    .type \name, @function
    .global \name
\name:
    xori seed, seed, -1
    li a3, 0
    li t1, 128
    bgeu len, t1, .crc_clmul_pre

.crc_tab_pre:
    beq len, zero, .done
    la crc_tab_addr, .lanchor_crc_tab
    add buf_end, buf, len

    .align 3
.loop_crc_tab:
    lbu t1, 0(buf)
    addi buf, buf, 1
    xor t1, t1, seed
    zext.b a7, t1
    slli a7, a7, 0x3
    add a7, a7, crc_tab_addr
    ld seed, 0(a7)
    srli t1, t1, 0x8
    xor seed, seed, t1
    bne buf, buf_end, .loop_crc_tab

.done:
    xori seed, seed, -1
    ret

    .align 2
.crc_clmul_pre:
    vsetivli zero, 2, e64, m1, ta, ma
    crc_refl_load_first_block
    crc_load_p4
    la t0, .refl_sld_mask
    vle64.v v0, (t0)
    beq buf, buf_end, .clmul_loop_end

    crc_refl_loop

.clmul_loop_end:
    refl_fold_1024b_to_128b

    # 128bit -> 64bit folding
    li t2, const_low
    clmul t4, t0, t2
    clmulh t5, t0, t2
    xor t1, t1, t4

    # Barrett reduction
    li t2, const_quo
    li t3, const_poly
    clmul t1, t1, t2
    clmulh t4, t1, t3
    xor t4, t4, t1
    xor seed, t4, t5

    j .crc_tab_pre
    .size   \name, .-\name
.section    .rodata,"a",@progbits
.refl_sld_mask:
    .quad   1, 0
.endm
