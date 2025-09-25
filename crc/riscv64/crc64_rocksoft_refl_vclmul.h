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

.equ    const_low, 0x21e9761e252621ac
.equ    const_quo, 0x27ecfa329aef9f77
.equ    const_poly, 0x34d926535897936b

    .section .rodata
    .text
    .align 3
    .set    .crc_loop_const,. + 0
    .type   const_fold, %object
    .size   const_fold, 64
const_fold:
    .quad 0x0c32cdb31e18a84a
    .quad 0x62242240ace5045a
    .quad 0xeadc41fd2ba3d420
    .quad 0x21e9761e252621ac

    .text
    .align  4
    .set    .lanchor_crc_tab,. + 0
    .type   crc64_rocksoft_refl_table, %object
    .size   crc64_rocksoft_refl_table, 2048
crc64_rocksoft_refl_table:
    .dword 0x0000000000000000, 0x7f6ef0c830358979
    .dword 0xfedde190606b12f2, 0x81b31158505e9b8b
    .dword 0xc962e5739841b68f, 0xb60c15bba8743ff6
    .dword 0x37bf04e3f82aa47d, 0x48d1f42bc81f2d04
    .dword 0xa61cecb46814fe75, 0xd9721c7c5821770c
    .dword 0x58c10d24087fec87, 0x27affdec384a65fe
    .dword 0x6f7e09c7f05548fa, 0x1010f90fc060c183
    .dword 0x91a3e857903e5a08, 0xeecd189fa00bd371
    .dword 0x78e0ff3b88be6f81, 0x078e0ff3b88be6f8
    .dword 0x863d1eabe8d57d73, 0xf953ee63d8e0f40a
    .dword 0xb1821a4810ffd90e, 0xceecea8020ca5077
    .dword 0x4f5ffbd87094cbfc, 0x30310b1040a14285
    .dword 0xdefc138fe0aa91f4, 0xa192e347d09f188d
    .dword 0x2021f21f80c18306, 0x5f4f02d7b0f40a7f
    .dword 0x179ef6fc78eb277b, 0x68f0063448deae02
    .dword 0xe943176c18803589, 0x962de7a428b5bcf0
    .dword 0xf1c1fe77117cdf02, 0x8eaf0ebf2149567b
    .dword 0x0f1c1fe77117cdf0, 0x7072ef2f41224489
    .dword 0x38a31b04893d698d, 0x47cdebccb908e0f4
    .dword 0xc67efa94e9567b7f, 0xb9100a5cd963f206
    .dword 0x57dd12c379682177, 0x28b3e20b495da80e
    .dword 0xa900f35319033385, 0xd66e039b2936bafc
    .dword 0x9ebff7b0e12997f8, 0xe1d10778d11c1e81
    .dword 0x606216208142850a, 0x1f0ce6e8b1770c73
    .dword 0x8921014c99c2b083, 0xf64ff184a9f739fa
    .dword 0x77fce0dcf9a9a271, 0x08921014c99c2b08
    .dword 0x4043e43f0183060c, 0x3f2d14f731b68f75
    .dword 0xbe9e05af61e814fe, 0xc1f0f56751dd9d87
    .dword 0x2f3dedf8f1d64ef6, 0x50531d30c1e3c78f
    .dword 0xd1e00c6891bd5c04, 0xae8efca0a188d57d
    .dword 0xe65f088b6997f879, 0x9931f84359a27100
    .dword 0x1882e91b09fcea8b, 0x67ec19d339c963f2
    .dword 0xd75adabd7a6e2d6f, 0xa8342a754a5ba416
    .dword 0x29873b2d1a053f9d, 0x56e9cbe52a30b6e4
    .dword 0x1e383fcee22f9be0, 0x6156cf06d21a1299
    .dword 0xe0e5de5e82448912, 0x9f8b2e96b271006b
    .dword 0x71463609127ad31a, 0x0e28c6c1224f5a63
    .dword 0x8f9bd7997211c1e8, 0xf0f5275142244891
    .dword 0xb824d37a8a3b6595, 0xc74a23b2ba0eecec
    .dword 0x46f932eaea507767, 0x3997c222da65fe1e
    .dword 0xafba2586f2d042ee, 0xd0d4d54ec2e5cb97
    .dword 0x5167c41692bb501c, 0x2e0934dea28ed965
    .dword 0x66d8c0f56a91f461, 0x19b6303d5aa47d18
    .dword 0x980521650afae693, 0xe76bd1ad3acf6fea
    .dword 0x09a6c9329ac4bc9b, 0x76c839faaaf135e2
    .dword 0xf77b28a2faafae69, 0x8815d86aca9a2710
    .dword 0xc0c42c4102850a14, 0xbfaadc8932b0836d
    .dword 0x3e19cdd162ee18e6, 0x41773d1952db919f
    .dword 0x269b24ca6b12f26d, 0x59f5d4025b277b14
    .dword 0xd846c55a0b79e09f, 0xa72835923b4c69e6
    .dword 0xeff9c1b9f35344e2, 0x90973171c366cd9b
    .dword 0x1124202993385610, 0x6e4ad0e1a30ddf69
    .dword 0x8087c87e03060c18, 0xffe938b633338561
    .dword 0x7e5a29ee636d1eea, 0x0134d92653589793
    .dword 0x49e52d0d9b47ba97, 0x368bddc5ab7233ee
    .dword 0xb738cc9dfb2ca865, 0xc8563c55cb19211c
    .dword 0x5e7bdbf1e3ac9dec, 0x21152b39d3991495
    .dword 0xa0a63a6183c78f1e, 0xdfc8caa9b3f20667
    .dword 0x97193e827bed2b63, 0xe877ce4a4bd8a21a
    .dword 0x69c4df121b863991, 0x16aa2fda2bb3b0e8
    .dword 0xf86737458bb86399, 0x8709c78dbb8deae0
    .dword 0x06bad6d5ebd3716b, 0x79d4261ddbe6f812
    .dword 0x3105d23613f9d516, 0x4e6b22fe23cc5c6f
    .dword 0xcfd833a67392c7e4, 0xb0b6c36e43a74e9d
    .dword 0x9a6c9329ac4bc9b5, 0xe50263e19c7e40cc
    .dword 0x64b172b9cc20db47, 0x1bdf8271fc15523e
    .dword 0x530e765a340a7f3a, 0x2c608692043ff643
    .dword 0xadd397ca54616dc8, 0xd2bd67026454e4b1
    .dword 0x3c707f9dc45f37c0, 0x431e8f55f46abeb9
    .dword 0xc2ad9e0da4342532, 0xbdc36ec59401ac4b
    .dword 0xf5129aee5c1e814f, 0x8a7c6a266c2b0836
    .dword 0x0bcf7b7e3c7593bd, 0x74a18bb60c401ac4
    .dword 0xe28c6c1224f5a634, 0x9de29cda14c02f4d
    .dword 0x1c518d82449eb4c6, 0x633f7d4a74ab3dbf
    .dword 0x2bee8961bcb410bb, 0x548079a98c8199c2
    .dword 0xd53368f1dcdf0249, 0xaa5d9839ecea8b30
    .dword 0x449080a64ce15841, 0x3bfe706e7cd4d138
    .dword 0xba4d61362c8a4ab3, 0xc52391fe1cbfc3ca
    .dword 0x8df265d5d4a0eece, 0xf29c951de49567b7
    .dword 0x732f8445b4cbfc3c, 0x0c41748d84fe7545
    .dword 0x6bad6d5ebd3716b7, 0x14c39d968d029fce
    .dword 0x95708ccedd5c0445, 0xea1e7c06ed698d3c
    .dword 0xa2cf882d2576a038, 0xdda178e515432941
    .dword 0x5c1269bd451db2ca, 0x237c997575283bb3
    .dword 0xcdb181ead523e8c2, 0xb2df7122e51661bb
    .dword 0x336c607ab548fa30, 0x4c0290b2857d7349
    .dword 0x04d364994d625e4d, 0x7bbd94517d57d734
    .dword 0xfa0e85092d094cbf, 0x856075c11d3cc5c6
    .dword 0x134d926535897936, 0x6c2362ad05bcf04f
    .dword 0xed9073f555e26bc4, 0x92fe833d65d7e2bd
    .dword 0xda2f7716adc8cfb9, 0xa54187de9dfd46c0
    .dword 0x24f29686cda3dd4b, 0x5b9c664efd965432
    .dword 0xb5517ed15d9d8743, 0xca3f8e196da80e3a
    .dword 0x4b8c9f413df695b1, 0x34e26f890dc31cc8
    .dword 0x7c339ba2c5dc31cc, 0x035d6b6af5e9b8b5
    .dword 0x82ee7a32a5b7233e, 0xfd808afa9582aa47
    .dword 0x4d364994d625e4da, 0x3258b95ce6106da3
    .dword 0xb3eba804b64ef628, 0xcc8558cc867b7f51
    .dword 0x8454ace74e645255, 0xfb3a5c2f7e51db2c
    .dword 0x7a894d772e0f40a7, 0x05e7bdbf1e3ac9de
    .dword 0xeb2aa520be311aaf, 0x944455e88e0493d6
    .dword 0x15f744b0de5a085d, 0x6a99b478ee6f8124
    .dword 0x224840532670ac20, 0x5d26b09b16452559
    .dword 0xdc95a1c3461bbed2, 0xa3fb510b762e37ab
    .dword 0x35d6b6af5e9b8b5b, 0x4ab846676eae0222
    .dword 0xcb0b573f3ef099a9, 0xb465a7f70ec510d0
    .dword 0xfcb453dcc6da3dd4, 0x83daa314f6efb4ad
    .dword 0x0269b24ca6b12f26, 0x7d0742849684a65f
    .dword 0x93ca5a1b368f752e, 0xeca4aad306bafc57
    .dword 0x6d17bb8b56e467dc, 0x12794b4366d1eea5
    .dword 0x5aa8bf68aecec3a1, 0x25c64fa09efb4ad8
    .dword 0xa4755ef8cea5d153, 0xdb1bae30fe90582a
    .dword 0xbcf7b7e3c7593bd8, 0xc399472bf76cb2a1
    .dword 0x422a5673a732292a, 0x3d44a6bb9707a053
    .dword 0x759552905f188d57, 0x0afba2586f2d042e
    .dword 0x8b48b3003f739fa5, 0xf42643c80f4616dc
    .dword 0x1aeb5b57af4dc5ad, 0x6585ab9f9f784cd4
    .dword 0xe436bac7cf26d75f, 0x9b584a0fff135e26
    .dword 0xd389be24370c7322, 0xace74eec0739fa5b
    .dword 0x2d545fb4576761d0, 0x523aaf7c6752e8a9
    .dword 0xc41748d84fe75459, 0xbb79b8107fd2dd20
    .dword 0x3acaa9482f8c46ab, 0x45a459801fb9cfd2
    .dword 0x0d75adabd7a6e2d6, 0x721b5d63e7936baf
    .dword 0xf3a84c3bb7cdf024, 0x8cc6bcf387f8795d
    .dword 0x620ba46c27f3aa2c, 0x1d6554a417c62355
    .dword 0x9cd645fc4798b8de, 0xe3b8b53477ad31a7
    .dword 0xab69411fbfb21ca3, 0xd407b1d78f8795da
    .dword 0x55b4a08fdfd90e51, 0x2ada5047efec8728
