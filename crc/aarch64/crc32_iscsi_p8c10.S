/**************************************************************
  Copyright (c) 2026 Huawei Technologies Co., Ltd.

  Redistribution and use in source and binary forms, with or without
  modification, are permitted provided that the following conditions
  are met:
    * Redistributions of source code must retain the above copyright
      notice, this list of conditions and the following disclaimer.
    * Redistributions in binary form must reproduce the above copyright
      notice, this list of conditions and the following disclaimer in
      the documentation and/or other materials provided with the
      distribution.
    * Neither the name of Huawei Corporation nor the names of its
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

#ifndef __APPLE__

#include "../include/aarch64_label.h"

.arch armv8.5-a+crc+sha3+sve2-aes
.file "crc32_iscsi_p8c10.S"
.text
.align 4
.global crc32_iscsi_fusion_p8_c10_asm
.type   crc32_iscsi_fusion_p8_c10_asm, %function

.set stack_size,        160
.set blk_size,          416   //the block_size corresponding to p8c10 is 8*32 + 10*16 = 416B
.set pmull_blk_size,    256
.set crc_parallels,     10
.set max_blk_num,       1260

.macro  push_stack
        stp     d8,  d9,  [sp, -stack_size]!
		stp     d10, d11, [sp, 16]
		stp     d12, d13, [sp, 32]
		stp     d14, d15, [sp, 48]
		stp     x19, x20, [sp, 64]
		stp     x21, x22, [sp, 80]
		stp     x23, x24, [sp, 96]
		stp     x25, x26, [sp, 112]
		stp     x27, x28, [sp, 128]
		stp     x29, x30, [sp, 144]
.endm

.macro  pop_stack
		ldp     d10, d11, [sp, 16]
		ldp     d12, d13, [sp, 32]
		ldp     d14, d15, [sp, 48]
		ldp     x19, x20, [sp, 64]
		ldp     x21, x22, [sp, 80]
		ldp     x23, x24, [sp, 96]
		ldp     x25, x26, [sp, 112]
		ldp     x27, x28, [sp, 128]
		ldp     x29, x30, [sp, 144]
		ldp     d8,  d9,  [sp], stack_size
.endm

    buffer0 .req    x0
    len     .req    w1
    len_x   .req    x1
    crc0    .req    w2
    crc0_x  .req    x2
    blk_num .req    w3
    klen    .req    w19
    klen_x  .req    x19
    bufferN .req    x4
    tmp_x   .req    x30
    tmp_w   .req    w30
    const_addr  .req    x30
    shifts_offset   .req    w19
    shifts_offset_x .req    x19

    crc1    .req    w5
    crc1_x  .req    x5
    crc2    .req    w6
    crc2_x  .req    x6
    crc3    .req    w7
    crc3_x  .req    x7
    crc4    .req    w8
    crc5    .req    w9
    crc6    .req    w10
    crc7    .req    w11
    crc8    .req    w26
    crc9    .req    w27

    buffer1 .req    x12
    buffer2 .req    x13
    buffer3 .req    x14
    buffer4 .req    x15
    buffer5 .req    x16
    buffer6 .req    x17
    buffer7 .req    x18
    buffer8 .req    x28
    buffer9 .req    x29

    data0   .req    x20
    data0_w .req    w20
    data1   .req    x21
    data2   .req    x22
    data3   .req    x23
    data4   .req    x24
    data5   .req    x25

    xnmodp_stack    .req    x12
    xnmodp_n        .req    x13
    xnmodp_n_w      .req    w13
    xnmodp_tmp      .req    x14
    xnmodp_tmp_w    .req    w14
    xnmodp_acc      .req    w15
    xnmodp_acc_x    .req    x15
    xnmodp_y        .req    x16
    xnmodp_vec_q    .req    q30
    xnmodp_vec_v    .req    v30
    xnmodp_vec_d    .req    d30

    k_96B   .req    x12
    k_64B   .req    x13
    k_32B   .req    x14

    mul_r0  .req    x15
    mul_r1  .req    x16
    mul_r2  .req    x17

.macro xnmodp_func  n_reg, result_s
    mov xnmodp_stack, #-2
.Lxnmodp_compress_large_index_\@:
    cmp \n_reg,#191
    b.ls .Lxnmodp_compress_large_index_end_\@

    and xnmodp_tmp, \n_reg, #1
    lsl xnmodp_stack, xnmodp_stack, #1
    add xnmodp_stack, xnmodp_stack, xnmodp_tmp

    lsr \n_reg, \n_reg, #1
    sub \n_reg, \n_reg, #16
    b   .Lxnmodp_compress_large_index_\@
.Lxnmodp_compress_large_index_end_\@:
    mvn xnmodp_stack, xnmodp_stack

    and xnmodp_tmp, \n_reg, #31
    mov xnmodp_acc, #0x80000000
    lsr xnmodp_acc, xnmodp_acc, xnmodp_tmp_w

    // n >>= 5
    lsr \n_reg, \n_reg, #5

.Lxnmodp_32bit_fold_\@:
    cbz \n_reg, .Lxnmodp_32bit_fold_end_\@
    crc32cw xnmodp_acc, xnmodp_acc, wzr
    sub \n_reg, \n_reg, #1
    b   .Lxnmodp_32bit_fold_\@

.Lxnmodp_32bit_fold_end_\@:
.Lxnmodp_left_fold_\@:
    and     \n_reg, xnmodp_stack, #1            //low = stack & 1
    lsr     xnmodp_stack, xnmodp_stack, #1      //stack >>=1
    //  Check the loop condition (whether the original stack is 0)
    cbz     xnmodp_stack, .Lxnmodp_left_fold_end_\@
.Lxnmodp_left_fold_body_\@:
    fmov    xnmodp_vec_d, xnmodp_acc_x          //vec_d = acc (As a 64-bit value)
    pmull   xnmodp_vec_v.8h, xnmodp_vec_v.8b, xnmodp_vec_v.8b  //v0 = x * x(Polynomial multiplication)
    fmov    xnmodp_y, xnmodp_vec_d              //x0 = y (Extract the lower 64 bits)
    lsl     xnmodp_y, xnmodp_y, \n_reg          //x0 = y << low
    crc32cx xnmodp_acc, wzr, xnmodp_y           //w22 = crc32cd(0, y << low)

    b .Lxnmodp_left_fold_\@
.Lxnmodp_left_fold_end_\@:
    // return acc
    fmov    \result_s, xnmodp_acc
.endm

crc32_iscsi_fusion_p8_c10_asm:
    push_stack
    cmp len, blk_size       //if len < blk_size
    blt .Ltail_less_blk_size
    movz tmp_w, blk_size
    udiv blk_num, len, tmp_w        //blk_num = len / blk_size
    lsl  klen, blk_num, #4          //klen = blk_num * 16
    mov  tmp_x, crc_parallels
    mul  tmp_x, klen_x, tmp_x       //offset = blk_num * 16 * 10
    add  bufferN, buffer0, tmp_x    //bufferN = buffer0 + offset, buffer0 is the first address of the input buffer

    ptrue   p0.b, all
    ld1d    z0.d, p0/z, [bufferN]
    ld1d    z1.d, p0/z, [bufferN, #1, mul vl]  //vl: current sve vector length(bytes)
    ld1d    z2.d, p0/z, [bufferN, #2, mul vl]
    ld1d    z3.d, p0/z, [bufferN, #3, mul vl]

    ld1d    z4.d, p0/z, [bufferN, #4, mul vl]
    ld1d    z5.d, p0/z, [bufferN, #5, mul vl]
    ld1d    z6.d, p0/z, [bufferN, #6, mul vl]
    ld1d    z7.d, p0/z, [bufferN, #7, mul vl]

    // the start address of each subsequent scalar channel is the address of the previous bufferx + klen
    add     buffer1, buffer0, klen_x
    add     buffer2, buffer1, klen_x
    add     buffer3, buffer2, klen_x
    add     buffer4, buffer3, klen_x
    add     buffer5, buffer4, klen_x
    add     buffer6, buffer5, klen_x
    add     buffer7, buffer6, klen_x
    add     buffer8, buffer7, klen_x
    add     buffer9, buffer8, klen_x

    adrp    const_addr, .Lp2048_symbol
    add     const_addr, const_addr, :lo12:.Lp2048_symbol
    ld1d    z16.d,  p0/z,  [const_addr]     // acceleration constant for the vector procesing part

    add bufferN, bufferN, pmull_blk_size
    sub len, len, blk_size

    mov crc1, #0
    mov crc2, #0
    mov crc3, #0
    mov crc4, #0
    mov crc5, #0
    mov crc6, #0
    mov crc7, #0
    mov crc8, #0
    mov crc9, #0

.Lloop_main:
    cmp len, blk_size
    b.lt .Lvector_processing_part_fold

    ldp data0, data1, [buffer0]
    ldp data2, data3, [buffer1]
    ldp data4, data5, [buffer2]

    crc32cx crc0, crc0, data0  //assembly instruction corresponding to the __crc32cd() intrinsic func is crc32cx
    crc32cx crc1, crc1, data2
    crc32cx crc2, crc2, data4

    crc32cx crc0, crc0, data1
    crc32cx crc1, crc1, data3
    crc32cx crc2, crc2, data5

    ldp data0, data1, [buffer3]
    ldp data2, data3, [buffer4]
    ldp data4, data5, [buffer5]

    crc32cx crc3, crc3, data0
    crc32cx crc4, crc4, data2
    crc32cx crc5, crc5, data4

    crc32cx crc3, crc3, data1
    crc32cx crc4, crc4, data3
    crc32cx crc5, crc5, data5

    /*
     *[Key: scalar-vector hybridization] Interleave crc32cx with pmullt/pmullb instructions,
     *and insert as many scalar calculations as possible between slower vector calculations.
     */
    pmullt  z17.q, z0.d, z16.d
    pmullb  z0.q,  z0.d, z16.d
    pmullt  z18.q, z1.d, z16.d
    pmullb  z1.q,  z1.d, z16.d
    pmullt  z19.q, z2.d, z16.d
    pmullb  z2.q,  z2.d, z16.d
    pmullt  z20.q, z3.d, z16.d
    pmullb  z3.q,  z3.d, z16.d

    ldp data0, data1, [buffer6]
    ldp data2, data3, [buffer7]
    ldp data4, data5, [buffer8]

    crc32cx crc6, crc6, data0
    crc32cx crc7, crc7, data2
    crc32cx crc8, crc8, data4
    crc32cx crc6, crc6, data1
    crc32cx crc7, crc7, data3
    crc32cx crc8, crc8, data5

    ldp data0, data1, [buffer9]
    crc32cx crc9, crc9, data0
    crc32cx crc9, crc9, data1

    pmullt  z21.q, z4.d, z16.d
    pmullb  z4.q,  z4.d, z16.d
    pmullt  z22.q, z5.d, z16.d
    pmullb  z5.q,  z5.d, z16.d
    pmullt  z23.q, z6.d, z16.d
    pmullb  z6.q,  z6.d, z16.d
    pmullt  z24.q, z7.d, z16.d
    pmullb  z7.q,  z7.d, z16.d

    ld1d    z8.d, p0/z, [bufferN]
    ld1d    z9.d, p0/z, [bufferN, #1, mul VL]
    ld1d    z10.d, p0/z, [bufferN, #2, mul VL]
    ld1d    z11.d, p0/z, [bufferN, #3, mul VL]
    ld1d    z12.d, p0/z, [bufferN, #4, mul VL]
    ld1d    z13.d, p0/z, [bufferN, #5, mul VL]
    ld1d    z14.d, p0/z, [bufferN, #6, mul VL]
    ld1d    z15.d, p0/z, [bufferN, #7, mul VL]

    add     bufferN, bufferN, pmull_blk_size

    add     buffer0, buffer0, #16
    add     buffer1, buffer1, #16
    add     buffer2, buffer2, #16
    add     buffer3, buffer3, #16
    add     buffer4, buffer4, #16
    add     buffer5, buffer5, #16
    add     buffer6, buffer6, #16
    add     buffer7, buffer7, #16
    add     buffer8, buffer8, #16
    add     buffer9, buffer9, #16

    sub     len, len, blk_size

    eor3    z0.d, z0.d, z17.d, z8.d
    eor3    z1.d, z1.d, z18.d, z9.d
    eor3    z2.d, z2.d, z19.d, z10.d
    eor3    z3.d, z3.d, z20.d, z11.d

    eor3    z4.d, z4.d, z21.d, z12.d
    eor3    z5.d, z5.d, z22.d, z13.d
    eor3    z6.d, z6.d, z23.d, z14.d
    eor3    z7.d, z7.d, z24.d, z15.d

    b .Lloop_main
.Lvector_processing_part_fold:
    adrp    const_addr, .Lp1024
    add     const_addr, const_addr, :lo12:.Lp1024
    ld1d    z16.d, p0/z, [const_addr]   //x0 = svld1_u64(pg, p4)

    adrp    const_addr, .Lp512
    add     const_addr, const_addr, :lo12:.Lp512
    ld1d    z17.d, p0/z, [const_addr]   //x00 = svld1_u64(pg, p3)

    adrp    const_addr, .Lp256
    add     const_addr, const_addr, :lo12:.Lp256
    ld1d    z18.d, p0/z, [const_addr]   //x000 = svld1_u64(pg, p2)

    //Group 1: x1(z0) treatment
    pmullt z20.q, z0.d, z16.d
    pmullb z0.q,  z0.d, z16.d
    eor3   z0.d, z0.d, z20.d, z4.d

    //Group 2: x2(z1) treatment
    pmullt z21.q, z1.d, z16.d
    pmullb z1.q,  z1.d, z16.d
    eor3   z1.d, z1.d, z21.d, z5.d

    //Group 3: x3(z2) treatment
    pmullt z22.q, z2.d, z16.d
    pmullb z2.q,  z2.d, z16.d
    eor3   z2.d, z2.d, z22.d, z6.d

    //Group 4: x4(z3) treatment
    pmullt z23.q, z3.d, z16.d
    pmullb z3.q,  z3.d, z16.d
    eor3   z3.d, z3.d, z23.d, z7.d

    //Group 5: x1(z0)  and x00(z17) treatments
    pmullt z24.q, z0.d, z17.d
    pmullb z0.q,  z0.d, z17.d
    eor3   z0.d, z0.d, z24.d, z2.d

    //Group 6: x2(z1) and x00(z17) treatments
    pmullt z25.q, z1.d, z17.d
    pmullb z1.q,  z1.d, z17.d
    eor3   z1.d, z1.d, z25.d, z3.d

    //Group 7: x1(z0) and x000(z18) treatments
    pmullt z26.q, z0.d, z18.d
    pmullb z0.q,  z0.d, z18.d
    eor3   z0.d, z0.d, z26.d, z1.d

    // extract_sve256_to_neon
    sub     sp, sp, #32
    st1d    {z0.d}, p0, [sp]
    ldr     q0, [sp]
    ldr     q1, [sp, #16]
    add     sp, sp, #32

    //crc32_fold_256b_to_128b
    adrp    const_addr, .Lp128
    ldr     q3, [const_addr, :lo12:.Lp128]

    pmull2  v4.1q, v3.2d, v0.2d
    pmull   v5.1q, v3.1d, v0.1d
    eor3    v0.16b, v1.16b, v4.16b, v5.16b

    //crc32c final
    ldp data0, data1, [buffer0]
    ldp data2, data3, [buffer1]
    ldp data4, data5, [buffer2]

    crc32cx crc0, crc0, data0
    crc32cx crc1, crc1, data2
    crc32cx crc2, crc2, data4
    crc32cx crc0, crc0, data1
    crc32cx crc1, crc1, data3
    crc32cx crc2, crc2, data5

    ldp data0, data1, [buffer3]
    ldp data2, data3, [buffer4]
    ldp data4, data5, [buffer5]

    crc32cx crc3, crc3, data0
    crc32cx crc4, crc4, data2
    crc32cx crc5, crc5, data4
    crc32cx crc3, crc3, data1
    crc32cx crc4, crc4, data3
    crc32cx crc5, crc5, data5

    ldp data0, data1, [buffer6]
    ldp data2, data3, [buffer7]
    ldp data4, data5, [buffer8]

    crc32cx crc6, crc6, data0
    crc32cx crc7, crc7, data2
    crc32cx crc8, crc8, data4
    crc32cx crc6, crc6, data1
    crc32cx crc7, crc7, data3
    crc32cx crc8, crc8, data5

    ldp data0, data1, [buffer9]
    crc32cx crc9, crc9, data0
    crc32cx crc9, crc9, data1

    cmp blk_num, max_blk_num
    bgt .Lxnmodp_cal
    adrp    const_addr, :got:crc32_iscsi_p8c10_clmul_const //acceleration constant for the scalar processing part
    ldr     const_addr, [const_addr, #:got_lo12:crc32_iscsi_p8c10_clmul_const]
    mov     shifts_offset, #40
    mul     shifts_offset, shifts_offset, blk_num
    add     const_addr, const_addr, shifts_offset_x

    ldp     s10, s12, [const_addr, #0]
    ldp     s14, s16, [const_addr, #8]
    ldp     s18, s20, [const_addr, #16]
    ldp     s22, s24, [const_addr, #24]
    ldp     s26, s28, [const_addr, #32]

.Lscalar_processing_part_fold:
    fmov    s11, crc0
    pmull   v2.1q, v10.1d, v11.1d

    fmov    s13, crc1
    pmull   v3.1q, v12.1d, v13.1d

    fmov    s15, crc2
    pmull   v4.1q, v14.1d, v15.1d

    fmov    s17, crc3
    pmull   v5.1q, v16.1d, v17.1d

    fmov    s19, crc4
    pmull   v6.1q, v18.1d, v19.1d

    fmov    s21, crc5
    pmull   v7.1q, v20.1d, v21.1d

    fmov    s23, crc6
    pmull   v8.1q, v22.1d, v23.1d

    fmov    s25, crc7
    pmull   v9.1q, v24.1d, v25.1d

    fmov    s27, crc8
    pmull   v10.1q, v26.1d, v27.1d

    fmov    s29, crc9
    pmull   v11.1q, v28.1d, v29.1d

    eor3    v2.16b, v2.16b, v3.16b, v4.16b
    eor3    v5.16b, v5.16b, v6.16b, v7.16b
    eor3    v8.16b, v8.16b, v9.16b, v10.16b
    eor3    v2.16b, v2.16b, v5.16b, v8.16b
    eor     v2.16b, v2.16b, v11.16b

    //Reduce
    fmov    data0, d2
    fmov    data1, d0
    crc32cx crc0, wzr, data1
    mov     data2, v0.d[1]
    eor     data0, data0, data2
    crc32cx crc0, crc0, data0

    mov     buffer0, bufferN
.Ltail_less_blk_size:
    cmp     len, #0
    beq     .Ldone
    movz    k_96B, #0xce53, lsl #0
    movk    k_96B, #0x0715, lsl #16
    fmov    d3, k_96B

    movz    k_64B, #0xddf8, lsl #0
    movk    k_64B, #0x9e4a, lsl #16
    fmov    d4, k_64B

    movz    k_32B, #0xc28e, lsl #0
    movk    k_32B, #0xba4f, lsl #16
    fmov    d5, k_32B
.L_tail_main_loop:
    cmp     len,#128
    b.lt    .L_tail_32
    mov     crc1, #0
    mov     crc2, #0
    mov     crc3, #0

    ldr     data0, [buffer0, #0]
    ldr     data1, [buffer0, #32]
    ldr     data2, [buffer0, #64]
    ldr     data3, [buffer0, #96]

    crc32cx crc0, crc0, data0
    crc32cx crc1, crc1, data1
    crc32cx crc2, crc2, data2
    crc32cx crc3, crc3, data3

    ldr     data0, [buffer0, #8]
    ldr     data1, [buffer0, #40]
    ldr     data2, [buffer0, #72]
    ldr     data3, [buffer0,#104]

    crc32cx crc0, crc0, data0
    crc32cx crc1, crc1, data1
    crc32cx crc2, crc2, data2
    crc32cx crc3, crc3, data3

    ldr     data0, [buffer0, #16]
    ldr     data1, [buffer0, #48]
    ldr     data2, [buffer0, #80]
    ldr     data3, [buffer0,#112]

    crc32cx crc0, crc0, data0
    crc32cx crc1, crc1, data1
    crc32cx crc2, crc2, data2
    crc32cx crc3, crc3, data3

    ldr     data0, [buffer0, #24]
    ldr     data1, [buffer0, #56]
    ldr     data2, [buffer0, #88]
    ldr     data3, [buffer0,#120]

    crc32cx crc0, crc0, data0
    crc32cx crc1, crc1, data1
    crc32cx crc2, crc2, data2
    crc32cx crc3, crc3, data3

    fmov    d6, crc0_x
    fmov    d7, crc1_x
    fmov    d8, crc2_x

    pmull   v6.1q, v6.1d, v3.1d
    pmull   v7.1q, v7.1d, v4.1d
    pmull   v8.1q, v8.1d, v5.1d

    fmov    mul_r0, d6
    fmov    mul_r1, d7
    fmov    mul_r2, d8

    crc32cx crc0, wzr, mul_r0
    crc32cx crc1, wzr, mul_r1
    crc32cx crc2, wzr, mul_r2

    eor     crc0, crc0, crc1
    eor     crc0, crc0, crc2
    eor     crc0, crc0, crc3

    add     buffer0, buffer0,#128
    sub     len, len,#128
    b   .L_tail_main_loop
.L_tail_32:
    cmp len, #32
    b.lt    .L_tail_8

    add     buffer1, buffer0, #16
    ldp     data0, data1, [buffer0]
    ldp     data2, data3, [buffer1]

    crc32cx crc0, crc0, data0
    crc32cx crc0, crc0, data1
    crc32cx crc0, crc0, data2
    crc32cx crc0, crc0, data3

    add     buffer0, buffer0, #32
    sub     len, len, #32
    b .L_tail_32

.L_tail_8:
    cmp len, #8
    b.lt    .L_tail_4

    ldr     data0, [buffer0], #8
    crc32cx crc0, crc0, data0
    sub     len, len, #8
    b .L_tail_8

.L_tail_4:
    tbz     len, #2, .L_tail_2
    ldr     data0_w, [buffer0], #4
    crc32cw crc0, crc0, data0_w

.L_tail_2:
    tbz      len, #1, .L_tail_1
    ldrh     data0_w, [buffer0], #2
    crc32ch  crc0, crc0, data0_w

.L_tail_1:
    tbz      len, #0, .Ldone
    ldrb     data0_w, [buffer0]
    crc32cb  crc0, crc0, data0_w

.Ldone:
    mov w0, crc0
    pop_stack
    ret

.Lxnmodp_cal:
    mov tmp_x, -33

    mov xnmodp_n_w, 3200
    smaddl x0, blk_num, xnmodp_n_w, tmp_x
    xnmodp_func x0, s10

    mov xnmodp_n_w, 3072
    smaddl x0, blk_num, xnmodp_n_w, tmp_x
    xnmodp_func x0, s12

    mov xnmodp_n_w, 2944
    smaddl x0, blk_num, xnmodp_n_w, tmp_x
    xnmodp_func x0, s14

    mov xnmodp_n_w, 2816
    smaddl x0, blk_num, xnmodp_n_w, tmp_x
    xnmodp_func x0, s16

    mov xnmodp_n_w, 2688
    smaddl x0, blk_num, xnmodp_n_w, tmp_x
    xnmodp_func x0, s18

    mov xnmodp_n_w, 2560
    smaddl x0, blk_num, xnmodp_n_w, tmp_x
    xnmodp_func x0, s20

    mov xnmodp_n_w, 2432
    smaddl x0, blk_num, xnmodp_n_w, tmp_x
    xnmodp_func x0, s22

    mov xnmodp_n_w, 2304
    smaddl x0, blk_num, xnmodp_n_w, tmp_x
    xnmodp_func x0, s24

    mov xnmodp_n_w, 2176
    smaddl x0, blk_num, xnmodp_n_w, tmp_x
    xnmodp_func x0, s26

    mov xnmodp_n_w, 2048
    smaddl x0, blk_num, xnmodp_n_w, tmp_x
    xnmodp_func x0, s28

    b .Lscalar_processing_part_fold
    .size   crc32_iscsi_fusion_p8_c10_asm, .-crc32_iscsi_fusion_p8_c10_asm

ASM_DEF_RODATA
    .align      4
    .set        .Lp2048_symbol,. + 0
    .type       .Lp2048, %object
    .size       .Lp2048, 32
.Lp2048:
	.word 0xdcb17aa4  // x^2079 mod P_crc32c(x) = 0xdcb17aa4
    .word 0x00000000
    .word 0xb9e02b86  // x^2015 mod P_crc32c(x) = 0xb9e02b86
    .word 0x00000000
    .word 0xdcb17aa4
    .word 0x00000000
    .word 0xb9e02b86
    .word 0x00000000

    .type   .Lp1024, %object
    .size   .Lp1024, 32
.Lp1024:
    .word 0x6992cea2, 0x00000000, 0x0d3b6092, 0x00000000  // x^1055 mod P_crc32c(x) = 0x6992cea2
    .word 0x6992cea2, 0x00000000, 0x0d3b6092, 0x00000000  // x^991 mod P_crc32c(x) = 0x0d3b6092

    .type   .Lp512, %object
    .size   .Lp512, 32
.Lp512:
    .word 0x740eef02, 0x00000000, 0x9e4addf8, 0x00000000  // x^543 mod P_crc32c(x) = 0x740eef02
    .word 0x740eef02, 0x00000000, 0x9e4addf8, 0x00000000  // x^479 mod P_crc32c(x) = 0x9e4addf8

    .type   .Lp256, %object
    .size   .Lp256, 32
.Lp256:
    .word 0x3da6d0cb, 0x00000000, 0xba4fc28e, 0x00000000  // x^287 mod P_crc32c(x) = 0x3da6d0cb
    .word 0x3da6d0cb, 0x00000000, 0xba4fc28e, 0x00000000  // x^223 mod P_crc32c(x) = 0xba4fc28e

    .type   .Lp128, %object
    .size   .Lp128, 16
.Lp128:
    .quad   0x00000000f20c0dfe  // x^159 mod P_crc32c(x) = 0xf20c0dfe
    .quad   0x00000000493c7d27  // x^95 mod P_crc32c(x) = 0x493c7d27

#endif




