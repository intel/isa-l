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

// parameters
#define seed          a0
#define buf           a1
#define len           a2
#define buf_end       a6

// global variables
#define counter       a3
#define buf_iter      a4
#define crc_tab_addr  a5
#define vec_shuffle   v13

.macro crc_refl_load_first_block
    mv buf_iter, buf
    vl4re64.v v0, 0(buf_iter)
    addi buf_iter, buf_iter, 64
    andi counter, len, ~63
    addi t0, counter, -64

    vmv.s.x v4, zero
    vmv.s.x v5, seed
    vslideup.vi v5, v4, 1
    vxor.vv v0, v0, v5
.endm

.macro crc_norm_load_first_block
    la t0, .shuffle_data
    mv buf_iter, buf
    vl4re64.v v4, (buf_iter)

    vsetivli zero, 16, e8, m1, ta, ma
    vle8.v vec_shuffle, 0(t0)
    vrgather.vv v0, v4, vec_shuffle
    vrgather.vv v1, v5, vec_shuffle
    vrgather.vv v2, v6, vec_shuffle
    vrgather.vv v3, v7, vec_shuffle
    vsetivli zero, 2, e64, m1, ta, ma

    addi buf_iter, buf_iter, 64
    andi counter, len, ~63
    addi t0, counter, -64

    vmv.s.x v4, zero
    vmv.s.x v5, seed
    vslideup.vi v4, v5, 1
    vxor.vv v0, v0, v4
.endm

.macro crc_load_p4
    add buf_end, buf_iter, t0
    la t4, .crc_loop_const
    vle64.v v5, 0(t4)
.endm

.macro crc_refl_loop
    .align 3
.clmul_loop:
    //reorganize the registers to ensure the correct order
    //of high and low parts in the result.
    vl4re64.v v8, (buf_iter)

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

    vxor.vv v0, v8, v15
    vxor.vv v1, v9, v16
    vxor.vv v2, v10, v17
    vxor.vv v3, v11, v18

    addi buf_iter, buf_iter, 64
    bne buf_iter, buf_end, .clmul_loop
.endm

.macro crc_norm_loop
    .align 3
.clmul_loop:
    //reorganize the registers to ensure the correct order
    //of high and low parts in the result.
    vl4re64.v v8, (buf_iter)

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
    vrgather.vv v0, v8, vec_shuffle
    vrgather.vv v1, v9, vec_shuffle
    vrgather.vv v2, v10, vec_shuffle
    vrgather.vv v3, v11, vec_shuffle
    vsetivli zero, 2, e64, m1, ta, ma
    vxor.vv v0, v0, v15
    vxor.vv v1, v1, v16
    vxor.vv v2, v2, v17
    vxor.vv v3, v3, v18

    addi buf_iter, buf_iter, 64
    bne buf_iter, buf_end, .clmul_loop
.endm

.macro crc_fold_512b_to_128b
    vle64.v v5, 0(t4)
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
.endm
