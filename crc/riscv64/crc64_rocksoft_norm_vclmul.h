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

.equ    const_low, 0x6b08c948f0dd2f08
.equ    const_quo, 0xddf3eeb298be6fc8
.equ    const_poly, 0xad93d23594c93659

    .section .rodata
    .text
    .align 4
    .set    .crc_loop_const,. + 0
    .type   const_fold, %object
    .size   const_fold, 32
const_fold:
    .quad 0x0473f99cf02ca70a
    .quad 0x769362f0dbe943f4
    .quad 0x08578ba97f0476ae
    .quad 0x6b08c948f0dd2f08

    .text
    .align  4
    .set    .lanchor_crc_tab,. + 0
    .type   crc64_rocksoft_norm_table, %object
    .size   crc64_rocksoft_norm_table, 2048
crc64_rocksoft_norm_table:
    .dword 0x0000000000000000, 0xad93d23594c93659
    .dword 0xf6b4765ebd5b5aeb, 0x5b27a46b29926cb2
    .dword 0x40fb3e88ee7f838f, 0xed68ecbd7ab6b5d6
    .dword 0xb64f48d65324d964, 0x1bdc9ae3c7edef3d
    .dword 0x81f67d11dcff071e, 0x2c65af2448363147
    .dword 0x77420b4f61a45df5, 0xdad1d97af56d6bac
    .dword 0xc10d439932808491, 0x6c9e91aca649b2c8
    .dword 0x37b935c78fdbde7a, 0x9a2ae7f21b12e823
    .dword 0xae7f28162d373865, 0x03ecfa23b9fe0e3c
    .dword 0x58cb5e48906c628e, 0xf5588c7d04a554d7
    .dword 0xee84169ec348bbea, 0x4317c4ab57818db3
    .dword 0x183060c07e13e101, 0xb5a3b2f5eadad758
    .dword 0x2f895507f1c83f7b, 0x821a873265010922
    .dword 0xd93d23594c936590, 0x74aef16cd85a53c9
    .dword 0x6f726b8f1fb7bcf4, 0xc2e1b9ba8b7e8aad
    .dword 0x99c61dd1a2ece61f, 0x3455cfe43625d046
    .dword 0xf16d8219cea74693, 0x5cfe502c5a6e70ca
    .dword 0x07d9f44773fc1c78, 0xaa4a2672e7352a21
    .dword 0xb196bc9120d8c51c, 0x1c056ea4b411f345
    .dword 0x4722cacf9d839ff7, 0xeab118fa094aa9ae
    .dword 0x709bff081258418d, 0xdd082d3d869177d4
    .dword 0x862f8956af031b66, 0x2bbc5b633bca2d3f
    .dword 0x3060c180fc27c202, 0x9df313b568eef45b
    .dword 0xc6d4b7de417c98e9, 0x6b4765ebd5b5aeb0
    .dword 0x5f12aa0fe3907ef6, 0xf281783a775948af
    .dword 0xa9a6dc515ecb241d, 0x04350e64ca021244
    .dword 0x1fe994870deffd79, 0xb27a46b29926cb20
    .dword 0xe95de2d9b0b4a792, 0x44ce30ec247d91cb
    .dword 0xdee4d71e3f6f79e8, 0x7377052baba64fb1
    .dword 0x2850a14082342303, 0x85c3737516fd155a
    .dword 0x9e1fe996d110fa67, 0x338c3ba345d9cc3e
    .dword 0x68ab9fc86c4ba08c, 0xc5384dfdf88296d5
    .dword 0x4f48d6060987bb7f, 0xe2db04339d4e8d26
    .dword 0xb9fca058b4dce194, 0x146f726d2015d7cd
    .dword 0x0fb3e88ee7f838f0, 0xa2203abb73310ea9
    .dword 0xf9079ed05aa3621b, 0x54944ce5ce6a5442
    .dword 0xcebeab17d578bc61, 0x632d792241b18a38
    .dword 0x380add496823e68a, 0x95990f7cfcead0d3
    .dword 0x8e45959f3b073fee, 0x23d647aaafce09b7
    .dword 0x78f1e3c1865c6505, 0xd56231f41295535c
    .dword 0xe137fe1024b0831a, 0x4ca42c25b079b543
    .dword 0x1783884e99ebd9f1, 0xba105a7b0d22efa8
    .dword 0xa1ccc098cacf0095, 0x0c5f12ad5e0636cc
    .dword 0x5778b6c677945a7e, 0xfaeb64f3e35d6c27
    .dword 0x60c18301f84f8404, 0xcd5251346c86b25d
    .dword 0x9675f55f4514deef, 0x3be6276ad1dde8b6
    .dword 0x203abd891630078b, 0x8da96fbc82f931d2
    .dword 0xd68ecbd7ab6b5d60, 0x7b1d19e23fa26b39
    .dword 0xbe25541fc720fdec, 0x13b6862a53e9cbb5
    .dword 0x489122417a7ba707, 0xe502f074eeb2915e
    .dword 0xfede6a97295f7e63, 0x534db8a2bd96483a
    .dword 0x086a1cc994042488, 0xa5f9cefc00cd12d1
    .dword 0x3fd3290e1bdffaf2, 0x9240fb3b8f16ccab
    .dword 0xc9675f50a684a019, 0x64f48d65324d9640
    .dword 0x7f281786f5a0797d, 0xd2bbc5b361694f24
    .dword 0x899c61d848fb2396, 0x240fb3eddc3215cf
    .dword 0x105a7c09ea17c589, 0xbdc9ae3c7edef3d0
    .dword 0xe6ee0a57574c9f62, 0x4b7dd862c385a93b
    .dword 0x50a1428104684606, 0xfd3290b490a1705f
    .dword 0xa61534dfb9331ced, 0x0b86e6ea2dfa2ab4
    .dword 0x91ac011836e8c297, 0x3c3fd32da221f4ce
    .dword 0x671877468bb3987c, 0xca8ba5731f7aae25
    .dword 0xd1573f90d8974118, 0x7cc4eda54c5e7741
    .dword 0x27e349ce65cc1bf3, 0x8a709bfbf1052daa
    .dword 0x9e91ac0c130f76fe, 0x33027e3987c640a7
    .dword 0x6825da52ae542c15, 0xc5b608673a9d1a4c
    .dword 0xde6a9284fd70f571, 0x73f940b169b9c328
    .dword 0x28dee4da402baf9a, 0x854d36efd4e299c3
    .dword 0x1f67d11dcff071e0, 0xb2f403285b3947b9
    .dword 0xe9d3a74372ab2b0b, 0x44407576e6621d52
    .dword 0x5f9cef95218ff26f, 0xf20f3da0b546c436
    .dword 0xa92899cb9cd4a884, 0x04bb4bfe081d9edd
    .dword 0x30ee841a3e384e9b, 0x9d7d562faaf178c2
    .dword 0xc65af24483631470, 0x6bc9207117aa2229
    .dword 0x7015ba92d047cd14, 0xdd8668a7448efb4d
    .dword 0x86a1cccc6d1c97ff, 0x2b321ef9f9d5a1a6
    .dword 0xb118f90be2c74985, 0x1c8b2b3e760e7fdc
    .dword 0x47ac8f555f9c136e, 0xea3f5d60cb552537
    .dword 0xf1e3c7830cb8ca0a, 0x5c7015b69871fc53
    .dword 0x0757b1ddb1e390e1, 0xaac463e8252aa6b8
    .dword 0x6ffc2e15dda8306d, 0xc26ffc2049610634
    .dword 0x9948584b60f36a86, 0x34db8a7ef43a5cdf
    .dword 0x2f07109d33d7b3e2, 0x8294c2a8a71e85bb
    .dword 0xd9b366c38e8ce909, 0x7420b4f61a45df50
    .dword 0xee0a530401573773, 0x43998131959e012a
    .dword 0x18be255abc0c6d98, 0xb52df76f28c55bc1
    .dword 0xaef16d8cef28b4fc, 0x0362bfb97be182a5
    .dword 0x58451bd25273ee17, 0xf5d6c9e7c6bad84e
    .dword 0xc1830603f09f0808, 0x6c10d43664563e51
    .dword 0x3737705d4dc452e3, 0x9aa4a268d90d64ba
    .dword 0x8178388b1ee08b87, 0x2cebeabe8a29bdde
    .dword 0x77cc4ed5a3bbd16c, 0xda5f9ce03772e735
    .dword 0x40757b122c600f16, 0xede6a927b8a9394f
    .dword 0xb6c10d4c913b55fd, 0x1b52df7905f263a4
    .dword 0x008e459ac21f8c99, 0xad1d97af56d6bac0
    .dword 0xf63a33c47f44d672, 0x5ba9e1f1eb8de02b
    .dword 0xd1d97a0a1a88cd81, 0x7c4aa83f8e41fbd8
    .dword 0x276d0c54a7d3976a, 0x8afede61331aa133
    .dword 0x91224482f4f74e0e, 0x3cb196b7603e7857
    .dword 0x679632dc49ac14e5, 0xca05e0e9dd6522bc
    .dword 0x502f071bc677ca9f, 0xfdbcd52e52befcc6
    .dword 0xa69b71457b2c9074, 0x0b08a370efe5a62d
    .dword 0x10d4399328084910, 0xbd47eba6bcc17f49
    .dword 0xe6604fcd955313fb, 0x4bf39df8019a25a2
    .dword 0x7fa6521c37bff5e4, 0xd2358029a376c3bd
    .dword 0x891224428ae4af0f, 0x2481f6771e2d9956
    .dword 0x3f5d6c94d9c0766b, 0x92cebea14d094032
    .dword 0xc9e91aca649b2c80, 0x647ac8fff0521ad9
    .dword 0xfe502f0deb40f2fa, 0x53c3fd387f89c4a3
    .dword 0x08e45953561ba811, 0xa5778b66c2d29e48
    .dword 0xbeab1185053f7175, 0x1338c3b091f6472c
    .dword 0x481f67dbb8642b9e, 0xe58cb5ee2cad1dc7
    .dword 0x20b4f813d42f8b12, 0x8d272a2640e6bd4b
    .dword 0xd6008e4d6974d1f9, 0x7b935c78fdbde7a0
    .dword 0x604fc69b3a50089d, 0xcddc14aeae993ec4
    .dword 0x96fbb0c5870b5276, 0x3b6862f013c2642f
    .dword 0xa142850208d08c0c, 0x0cd157379c19ba55
    .dword 0x57f6f35cb58bd6e7, 0xfa6521692142e0be
    .dword 0xe1b9bb8ae6af0f83, 0x4c2a69bf726639da
    .dword 0x170dcdd45bf45568, 0xba9e1fe1cf3d6331
    .dword 0x8ecbd005f918b377, 0x235802306dd1852e
    .dword 0x787fa65b4443e99c, 0xd5ec746ed08adfc5
    .dword 0xce30ee8d176730f8, 0x63a33cb883ae06a1
    .dword 0x388498d3aa3c6a13, 0x95174ae63ef55c4a
    .dword 0x0f3dad1425e7b469, 0xa2ae7f21b12e8230
    .dword 0xf989db4a98bcee82, 0x541a097f0c75d8db
    .dword 0x4fc6939ccb9837e6, 0xe25541a95f5101bf
    .dword 0xb972e5c276c36d0d, 0x14e137f7e20a5b54
