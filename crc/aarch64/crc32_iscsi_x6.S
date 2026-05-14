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

.text
.align    6
.arch     armv8-a+crc+crypto

.macro crc32_u64    dst,src,data
     crc32cx     \dst, \src, \data
.endm

.macro crc32_u32    dst,src,data
     crc32cw     \dst, \src, \data
.endm

.macro crc32_u16    dst,src,data
     crc32ch     \dst, \src, \data
.endm

.macro crc32_u8     dst,src,data
     crc32cb     \dst, \src, \data
.endm

.set stack_size, 160
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

.macro declare_var_vector_reg name:req,reg:req
     q\name   .req   q\reg
     v\name   .req   v\reg
     s\name   .req   s\reg
     d\name   .req   d\reg
.endm

    BUF       .req   x0
    LEN       .req   x1
    wCRC      .req   w2
    crc0      .req   w2
    crc1      .req   w3
    crc2      .req   w4
    crc3      .req   w5
    crc4      .req   w6
    crc5      .req   w7
    xcrc0     .req   x2
    xcrc1     .req   x3
    xcrc2     .req   x4
    xcrc3     .req   x5
    xcrc4     .req   x6
    xcrc5     .req   x7

    const_adr .req   x8
    ptr_crc0  .req   x0
    ptr_crc1  .req   x9
    ptr_crc2  .req   x10
    ptr_crc3  .req   x11
    ptr_crc4  .req   x12
    ptr_crc5  .req   x13

    crc0_data0  .req  x14
    crc0_data1  .req  x15
    crc0_data2  .req  x24
    crc0_data3  .req  x25
    crc0_data4  .req  x22
    crc0_data5  .req  x23

    crc0_data6  .req  x16
    crc0_data7  .req  x17
    crc0_data8  .req  x18
    crc0_data9  .req  x19
    crc0_data10 .req  x20
    crc0_data11 .req  x26

    wdata       .req  w8
    data0       .req  x8
    data1       .req  x4
    data2       .req  x21
    data3       .req  x9

    declare_var_vector_reg tmp0, 0
    declare_var_vector_reg tmp1, 1
    declare_var_vector_reg tmp2, 2
    declare_var_vector_reg tmp3, 3
    declare_var_vector_reg tmp4, 4

    declare_var_vector_reg const0, 5
    declare_var_vector_reg const1, 6
    declare_var_vector_reg const2, 7
    declare_var_vector_reg const3, 8
    declare_var_vector_reg const4, 9

/**
    unsigned int crc32_iscsi_x6(
        unsigned char *BUF,
        int LEN,
        unsigned int wCRC
    );

*/

.global crc32_iscsi_x6
.type   crc32_iscsi_x6, %function
crc32_iscsi_x6:

    push_stack
    cbz      LEN, .zero_length_ret

.align_finish:
    cmp LEN,1024
    bls 1f
    adrp     const_adr, .Lconstants
    add      const_adr, const_adr, #:lo12:.Lconstants
    ldp      dconst0,dconst1,[const_adr]
    ldp      dconst2,dconst3,[const_adr, 16]
    ldr      dconst4,[const_adr, 32]
2:
    mov      crc1,0
    mov      crc2,0
    mov      crc3,0
    mov      crc4,0
    mov      crc5,0

    ldr      crc0_data0, [ptr_crc0], 8
    crc32_u64   crc0, crc0, crc0_data0

    add      ptr_crc1,ptr_crc0,168
    add      ptr_crc2,ptr_crc0,168*2
    add      ptr_crc3,ptr_crc0,168*3
    add      ptr_crc4,ptr_crc0,168*4
    add      ptr_crc5,ptr_crc0,168*5

    ldp      crc0_data0,crc0_data6,[ptr_crc0],16
    ldp      crc0_data1,crc0_data7,[ptr_crc1],16
    ldp      crc0_data2,crc0_data8,[ptr_crc2],16
    ldp      crc0_data3,crc0_data9,[ptr_crc3],16
    ldp      crc0_data4,crc0_data10,[ptr_crc4],16
    ldp      crc0_data5,crc0_data11,[ptr_crc5],16

.rept 4
    crc32_u64    crc0,crc0,crc0_data0
    crc32_u64    crc1,crc1,crc0_data1
    crc32_u64    crc2,crc2,crc0_data2
    crc32_u64    crc3,crc3,crc0_data3
    crc32_u64    crc4,crc4,crc0_data4
    crc32_u64    crc5,crc5,crc0_data5

    ldp     crc0_data0, crc0_data3,[ptr_crc0],16
    ldp     crc0_data1, crc0_data4,[ptr_crc1],16
    ldp     crc0_data2, crc0_data5,[ptr_crc2],16

    crc32_u64    crc0,crc0,crc0_data6
    crc32_u64    crc1,crc1,crc0_data7
    crc32_u64    crc2,crc2,crc0_data8
    crc32_u64    crc3,crc3,crc0_data9
    crc32_u64    crc4,crc4,crc0_data10
    crc32_u64    crc5,crc5,crc0_data11

    ldp     crc0_data6, crc0_data9,[ptr_crc3],16
    ldp     crc0_data7, crc0_data10,[ptr_crc4],16
    ldp     crc0_data8, crc0_data11,[ptr_crc5],16

    crc32_u64    crc0,crc0,crc0_data0
    crc32_u64    crc1,crc1,crc0_data1
    crc32_u64    crc2,crc2,crc0_data2
    crc32_u64    crc3,crc3,crc0_data6
    crc32_u64    crc4,crc4,crc0_data7
    crc32_u64    crc5,crc5,crc0_data8

    ldp     crc0_data0, crc0_data6,[ptr_crc0],16
    ldp     crc0_data1, crc0_data7,[ptr_crc1],16
    ldp     crc0_data2, crc0_data8,[ptr_crc2],16

    crc32_u64    crc0,crc0,crc0_data3
    crc32_u64    crc1,crc1,crc0_data4
    crc32_u64    crc2,crc2,crc0_data5
    crc32_u64    crc3,crc3,crc0_data9
    crc32_u64    crc4,crc4,crc0_data10
    crc32_u64    crc5,crc5,crc0_data11

    ldp     crc0_data3, crc0_data9,[ptr_crc3],16
    ldp     crc0_data4, crc0_data10,[ptr_crc4],16
    ldp     crc0_data5, crc0_data11,[ptr_crc5],16
.endr

    crc32_u64    crc0,crc0,crc0_data0
    crc32_u64    crc1,crc1,crc0_data1
    crc32_u64    crc2,crc2,crc0_data2
    crc32_u64    crc3,crc3,crc0_data3
    crc32_u64    crc4,crc4,crc0_data4
    crc32_u64    crc5,crc5,crc0_data5

    ldp     crc0_data0, crc0_data3,[ptr_crc0],16
    ldp     crc0_data1, crc0_data4,[ptr_crc1],16
    ldp     crc0_data2, crc0_data5,[ptr_crc2],16

    crc32_u64    crc0,crc0,crc0_data6
    crc32_u64    crc1,crc1,crc0_data7
    crc32_u64    crc2,crc2,crc0_data8
    crc32_u64    crc3,crc3,crc0_data9
    crc32_u64    crc4,crc4,crc0_data10
    crc32_u64    crc5,crc5,crc0_data11

    ldp     crc0_data6, crc0_data9,[ptr_crc3],16
    ldp     crc0_data7, crc0_data10,[ptr_crc4],16
    ldp     crc0_data8, crc0_data11,[ptr_crc5],16

    crc32_u64    crc0,crc0,crc0_data0
    crc32_u64    crc1,crc1,crc0_data1
    crc32_u64    crc2,crc2,crc0_data2
    crc32_u64    crc3,crc3,crc0_data6
    crc32_u64    crc4,crc4,crc0_data7
    crc32_u64    crc5,crc5,crc0_data8

    ldr     crc0_data0,[ptr_crc0],8
    ldr     crc0_data1,[ptr_crc1],8
    ldr     crc0_data2,[ptr_crc2],8

    crc32_u64    crc0,crc0,crc0_data3
    crc32_u64    crc1,crc1,crc0_data4
    crc32_u64    crc2,crc2,crc0_data5
    crc32_u64    crc3,crc3,crc0_data9
    crc32_u64    crc4,crc4,crc0_data10
    crc32_u64    crc5,crc5,crc0_data11

    ldr     crc0_data3,[ptr_crc3],8
    ldr     crc0_data4,[ptr_crc4],8
    ldr     crc0_data5,[ptr_crc5],8

    crc32_u64    crc0,crc0,crc0_data0
    crc32_u64    crc1,crc1,crc0_data1
    crc32_u64    crc2,crc2,crc0_data2
    crc32_u64    crc3,crc3,crc0_data3
    crc32_u64    crc4,crc4,crc0_data4
    crc32_u64    crc5,crc5,crc0_data5

    ldr     crc0_data5, [ptr_crc5], 8
    crc32_u64  crc5, crc5, crc0_data5

    fmov    dtmp0,xcrc0
    fmov    dtmp1,xcrc1
    fmov    dtmp2,xcrc2
    fmov    dtmp3,xcrc3
    fmov    dtmp4,xcrc4

    mov     ptr_crc0,ptr_crc5
    pmull   vtmp0.1q,vtmp0.1d,vconst0.1d
    sub     LEN,LEN,1024
    pmull   vtmp1.1q,vtmp1.1d,vconst1.1d
    pmull   vtmp2.1q,vtmp2.1d,vconst2.1d
    pmull   vtmp3.1q,vtmp3.1d,vconst3.1d
    pmull   vtmp4.1q,vtmp4.1d,vconst4.1d
    cmp     LEN,1024
    fmov    xcrc0, dtmp0
    fmov    xcrc1, dtmp1
    fmov    xcrc2, dtmp2
    fmov    xcrc3, dtmp3
    fmov    xcrc4, dtmp4

    crc32_u64  crc0,wzr,xcrc0
    crc32_u64  crc1,wzr,xcrc1
    crc32_u64  crc2,wzr,xcrc2
    crc32_u64  crc3,wzr,xcrc3
    crc32_u64  crc4,wzr,xcrc4

    eor   crc0,crc0,crc1
    eor   crc2,crc2,crc3
    eor   crc4,crc4,crc5
    eor   crc2,crc2,crc4
    eor   crc0,crc0,crc2
    bhs 2b

1:
    cmp   LEN, 63
    bls   .loop_16B
.loop_64B:
    ldp   data0,data1,[BUF],#16
    sub   LEN,LEN,#64
    ldp   data2,data3,[BUF],#16
    cmp   LEN,#64
    crc32_u64  wCRC, wCRC, data0
    crc32_u64  wCRC, wCRC, data1
    ldp   data0,data1,[BUF],#16
    crc32_u64  wCRC, wCRC, data2
    crc32_u64  wCRC, wCRC, data3
    ldp   data2,data3,[BUF],#16
    crc32_u64  wCRC, wCRC, data0
    crc32_u64  wCRC, wCRC, data1
    crc32_u64  wCRC, wCRC, data2
    crc32_u64  wCRC, wCRC, data3
    bge     .loop_64B

.loop_16B:
    cmp    LEN, 15
    bls    .less_16B
    ldp    data0, data1, [BUF], #16
    sub    LEN, LEN, #16
    cmp    LEN,15
    crc32_u64  wCRC, wCRC, data0
    crc32_u64  wCRC, wCRC, data1
    bls    .less_16B
    ldp    data0, data1, [BUF], #16
    sub    LEN,LEN,#16
    cmp    LEN,15
    crc32_u64 wCRC,wCRC,data0
    crc32_u64 wCRC,wCRC,data1
    bls   .less_16B
    ldp    data0, data1, [BUF], #16
    sub    LEN,LEN,#16
    crc32_u64 wCRC,wCRC,data0
    crc32_u64 wCRC,wCRC,data1
.less_16B:
    cmp    LEN, 7
    bls    .less_8B
    ldr    data0, [BUF], 8
    sub    LEN, LEN, #8
    crc32_u64 wCRC, wCRC,data0
.less_8B:
    cmp    LEN, 3
    bls    .less_4B
    ldr    wdata, [BUF], 4
    sub    LEN, LEN, #4
    crc32_u32 wCRC, wCRC, wdata
.less_4B:
    cmp    LEN, 1
    bls    .less_2B
    ldrh   wdata, [BUF], 2
    sub    LEN, LEN, #2
    crc32_u16  wCRC, wCRC, wdata
.less_2B:
    cbz    LEN, .zero_length_ret
    ldrb   wdata, [BUF]
    crc32_u8  wCRC,wCRC,wdata

.zero_length_ret:
    mov    w0, wCRC
    pop_stack
    ret

.align_short_2:
    ldrh    wdata, [BUF], 2
    sub     LEN, LEN, 2
    tst     BUF, 4
    crc32_u16 wCRC, wCRC, wdata
    ccmp    LEN, 3, 0, ne
    bls     .align_finish
.align_word:
    ldr     wdata, [BUF], 4
    sub     LEN, LEN, #4
    crc32_u32   wCRC, wCRC, wdata
    b       .align_finish

.size  crc32_iscsi_x6, .-crc32_iscsi_x6

ASM_DEF_RODATA
.align 4
.Lconstants:
    .quad     0xa741c1bf
    .quad     0xe417f38a
    .quad     0xdd7e3b0c
    .quad     0x8f158014
    .quad     0xdaece73e

#endif
