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

.equ    const_low, 0x2d560000
.equ    const_high, 0x13680000
.equ    const_quo, 0x1f65a57f9
.equ    const_poly, 0x18bb70000

    .section .rodata
    .text
    .align 4
    .set    .crc_loop_const,. + 0
    .type   const_fold, %object
    .size   const_fold, 32
const_fold:
    .quad 0x39b40000
    .quad 0x51d10000
    .quad 0x4c1a0000
    .quad 0xfb0b0000

    .text
    .align  4
    .set    .lanchor_crc_tab,. + 0
    .type   crc16tab, %object
    .size   crc16tab, 1024
crc16tab:
    .word 0x00000000, 0x8bb70000, 0x9cd90000, 0x176e0000, 0xb2050000, 0x39b20000, 0x2edc0000, 0xa56b0000
    .word 0xefbd0000, 0x640a0000, 0x73640000, 0xf8d30000, 0x5db80000, 0xd60f0000, 0xc1610000, 0x4ad60000
    .word 0x54cd0000, 0xdf7a0000, 0xc8140000, 0x43a30000, 0xe6c80000, 0x6d7f0000, 0x7a110000, 0xf1a60000
    .word 0xbb700000, 0x30c70000, 0x27a90000, 0xac1e0000, 0x09750000, 0x82c20000, 0x95ac0000, 0x1e1b0000
    .word 0xa99a0000, 0x222d0000, 0x35430000, 0xbef40000, 0x1b9f0000, 0x90280000, 0x87460000, 0x0cf10000
    .word 0x46270000, 0xcd900000, 0xdafe0000, 0x51490000, 0xf4220000, 0x7f950000, 0x68fb0000, 0xe34c0000
    .word 0xfd570000, 0x76e00000, 0x618e0000, 0xea390000, 0x4f520000, 0xc4e50000, 0xd38b0000, 0x583c0000
    .word 0x12ea0000, 0x995d0000, 0x8e330000, 0x05840000, 0xa0ef0000, 0x2b580000, 0x3c360000, 0xb7810000
    .word 0xd8830000, 0x53340000, 0x445a0000, 0xcfed0000, 0x6a860000, 0xe1310000, 0xf65f0000, 0x7de80000
    .word 0x373e0000, 0xbc890000, 0xabe70000, 0x20500000, 0x853b0000, 0x0e8c0000, 0x19e20000, 0x92550000
    .word 0x8c4e0000, 0x07f90000, 0x10970000, 0x9b200000, 0x3e4b0000, 0xb5fc0000, 0xa2920000, 0x29250000
    .word 0x63f30000, 0xe8440000, 0xff2a0000, 0x749d0000, 0xd1f60000, 0x5a410000, 0x4d2f0000, 0xc6980000
    .word 0x71190000, 0xfaae0000, 0xedc00000, 0x66770000, 0xc31c0000, 0x48ab0000, 0x5fc50000, 0xd4720000
    .word 0x9ea40000, 0x15130000, 0x027d0000, 0x89ca0000, 0x2ca10000, 0xa7160000, 0xb0780000, 0x3bcf0000
    .word 0x25d40000, 0xae630000, 0xb90d0000, 0x32ba0000, 0x97d10000, 0x1c660000, 0x0b080000, 0x80bf0000
    .word 0xca690000, 0x41de0000, 0x56b00000, 0xdd070000, 0x786c0000, 0xf3db0000, 0xe4b50000, 0x6f020000
    .word 0x3ab10000, 0xb1060000, 0xa6680000, 0x2ddf0000, 0x88b40000, 0x03030000, 0x146d0000, 0x9fda0000
    .word 0xd50c0000, 0x5ebb0000, 0x49d50000, 0xc2620000, 0x67090000, 0xecbe0000, 0xfbd00000, 0x70670000
    .word 0x6e7c0000, 0xe5cb0000, 0xf2a50000, 0x79120000, 0xdc790000, 0x57ce0000, 0x40a00000, 0xcb170000
    .word 0x81c10000, 0x0a760000, 0x1d180000, 0x96af0000, 0x33c40000, 0xb8730000, 0xaf1d0000, 0x24aa0000
    .word 0x932b0000, 0x189c0000, 0x0ff20000, 0x84450000, 0x212e0000, 0xaa990000, 0xbdf70000, 0x36400000
    .word 0x7c960000, 0xf7210000, 0xe04f0000, 0x6bf80000, 0xce930000, 0x45240000, 0x524a0000, 0xd9fd0000
    .word 0xc7e60000, 0x4c510000, 0x5b3f0000, 0xd0880000, 0x75e30000, 0xfe540000, 0xe93a0000, 0x628d0000
    .word 0x285b0000, 0xa3ec0000, 0xb4820000, 0x3f350000, 0x9a5e0000, 0x11e90000, 0x06870000, 0x8d300000
    .word 0xe2320000, 0x69850000, 0x7eeb0000, 0xf55c0000, 0x50370000, 0xdb800000, 0xccee0000, 0x47590000
    .word 0x0d8f0000, 0x86380000, 0x91560000, 0x1ae10000, 0xbf8a0000, 0x343d0000, 0x23530000, 0xa8e40000
    .word 0xb6ff0000, 0x3d480000, 0x2a260000, 0xa1910000, 0x04fa0000, 0x8f4d0000, 0x98230000, 0x13940000
    .word 0x59420000, 0xd2f50000, 0xc59b0000, 0x4e2c0000, 0xeb470000, 0x60f00000, 0x779e0000, 0xfc290000
    .word 0x4ba80000, 0xc01f0000, 0xd7710000, 0x5cc60000, 0xf9ad0000, 0x721a0000, 0x65740000, 0xeec30000
    .word 0xa4150000, 0x2fa20000, 0x38cc0000, 0xb37b0000, 0x16100000, 0x9da70000, 0x8ac90000, 0x017e0000
    .word 0x1f650000, 0x94d20000, 0x83bc0000, 0x080b0000, 0xad600000, 0x26d70000, 0x31b90000, 0xba0e0000
    .word 0xf0d80000, 0x7b6f0000, 0x6c010000, 0xe7b60000, 0x42dd0000, 0xc96a0000, 0xde040000, 0x55b30000
