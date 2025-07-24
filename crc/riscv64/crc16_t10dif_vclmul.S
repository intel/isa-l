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
#include "crc_norm_common_vclmul.h"

crc_norm_func crc16_t10dif_vclmul_internal

    .text
    .align 3
    .global crc16_t10dif_vclmul
    .type   crc16_t10dif_vclmul, %function
crc16_t10dif_vclmul:
    # initialize seed for calculation in CRC32 format
    slli a0, a0, 16
    xori a0, a0, -1
    addi sp, sp, -8
    sd ra, (sp)
1:
    auipc ra, %pcrel_hi(crc16_t10dif_vclmul_internal)
    jalr  ra, %pcrel_lo(1b)(ra)
    ld ra, (sp)
    addi sp, sp, 8
    xori a0, a0, -1
    srliw a0, a0, 16
    ret
#endif
