########################################################################
#  Copyright(c) 2025 Tim Burke All rights reserved.
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
#    * Neither the name of Arm Corporation nor the names of its
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

# rk16
.equ	p4_low_b0, 0xa84a
.equ	p4_low_b1, 0x1e18
.equ	p4_low_b2, 0xcdb3
.equ	p4_low_b3, 0x0c32
# rk15
.equ	p4_high_b0, 0x045a
.equ	p4_high_b1, 0xace5
.equ	p4_high_b2, 0x2240
.equ	p4_high_b3, 0x6224

# rk2
.equ	p1_low_b0, 0xd420
.equ	p1_low_b1, 0x2ba3
.equ	p1_low_b2, 0x41fd
.equ	p1_low_b3, 0xeadc
# rk1
.equ	p1_high_b0, 0x21ac
.equ	p1_high_b1, 0x2526
.equ	p1_high_b2, 0x761e
.equ	p1_high_b3, 0x21e9

# rk1
.equ	p0_low_b0, 0x21ac
.equ	p0_low_b1, 0x2526
.equ	p0_low_b2, 0x761e
.equ	p0_low_b3, 0x21e9

# rk7
.equ	br_low_b0, 0x9f77
.equ	br_low_b1, 0x9aef
.equ	br_low_b2, 0xfa32
.equ	br_low_b3, 0x27ec
# rk8 + 1?
.equ	br_high_b0, 0x936b
.equ	br_high_b1, 0x5897
.equ	br_high_b2, 0x2653
.equ	br_high_b3, 0x34d9

ASM_DEF_RODATA
	.align	4
	.set	.lanchor_crc_tab,. + 0
#ifndef __APPLE__
	.type	crc64_tab, %object
	.size	crc64_tab, 2048
#endif

crc64_tab:
	.xword 0x0000000000000000, 0x7f6ef0c830358979
	.xword 0xfedde190606b12f2, 0x81b31158505e9b8b
	.xword 0xc962e5739841b68f, 0xb60c15bba8743ff6
	.xword 0x37bf04e3f82aa47d, 0x48d1f42bc81f2d04
	.xword 0xa61cecb46814fe75, 0xd9721c7c5821770c
	.xword 0x58c10d24087fec87, 0x27affdec384a65fe
	.xword 0x6f7e09c7f05548fa, 0x1010f90fc060c183
	.xword 0x91a3e857903e5a08, 0xeecd189fa00bd371
	.xword 0x78e0ff3b88be6f81, 0x078e0ff3b88be6f8
	.xword 0x863d1eabe8d57d73, 0xf953ee63d8e0f40a
	.xword 0xb1821a4810ffd90e, 0xceecea8020ca5077
	.xword 0x4f5ffbd87094cbfc, 0x30310b1040a14285
	.xword 0xdefc138fe0aa91f4, 0xa192e347d09f188d
	.xword 0x2021f21f80c18306, 0x5f4f02d7b0f40a7f
	.xword 0x179ef6fc78eb277b, 0x68f0063448deae02
	.xword 0xe943176c18803589, 0x962de7a428b5bcf0
	.xword 0xf1c1fe77117cdf02, 0x8eaf0ebf2149567b
	.xword 0x0f1c1fe77117cdf0, 0x7072ef2f41224489
	.xword 0x38a31b04893d698d, 0x47cdebccb908e0f4
	.xword 0xc67efa94e9567b7f, 0xb9100a5cd963f206
	.xword 0x57dd12c379682177, 0x28b3e20b495da80e
	.xword 0xa900f35319033385, 0xd66e039b2936bafc
	.xword 0x9ebff7b0e12997f8, 0xe1d10778d11c1e81
	.xword 0x606216208142850a, 0x1f0ce6e8b1770c73
	.xword 0x8921014c99c2b083, 0xf64ff184a9f739fa
	.xword 0x77fce0dcf9a9a271, 0x08921014c99c2b08
	.xword 0x4043e43f0183060c, 0x3f2d14f731b68f75
	.xword 0xbe9e05af61e814fe, 0xc1f0f56751dd9d87
	.xword 0x2f3dedf8f1d64ef6, 0x50531d30c1e3c78f
	.xword 0xd1e00c6891bd5c04, 0xae8efca0a188d57d
	.xword 0xe65f088b6997f879, 0x9931f84359a27100
	.xword 0x1882e91b09fcea8b, 0x67ec19d339c963f2
	.xword 0xd75adabd7a6e2d6f, 0xa8342a754a5ba416
	.xword 0x29873b2d1a053f9d, 0x56e9cbe52a30b6e4
	.xword 0x1e383fcee22f9be0, 0x6156cf06d21a1299
	.xword 0xe0e5de5e82448912, 0x9f8b2e96b271006b
	.xword 0x71463609127ad31a, 0x0e28c6c1224f5a63
	.xword 0x8f9bd7997211c1e8, 0xf0f5275142244891
	.xword 0xb824d37a8a3b6595, 0xc74a23b2ba0eecec
	.xword 0x46f932eaea507767, 0x3997c222da65fe1e
	.xword 0xafba2586f2d042ee, 0xd0d4d54ec2e5cb97
	.xword 0x5167c41692bb501c, 0x2e0934dea28ed965
	.xword 0x66d8c0f56a91f461, 0x19b6303d5aa47d18
	.xword 0x980521650afae693, 0xe76bd1ad3acf6fea
	.xword 0x09a6c9329ac4bc9b, 0x76c839faaaf135e2
	.xword 0xf77b28a2faafae69, 0x8815d86aca9a2710
	.xword 0xc0c42c4102850a14, 0xbfaadc8932b0836d
	.xword 0x3e19cdd162ee18e6, 0x41773d1952db919f
	.xword 0x269b24ca6b12f26d, 0x59f5d4025b277b14
	.xword 0xd846c55a0b79e09f, 0xa72835923b4c69e6
	.xword 0xeff9c1b9f35344e2, 0x90973171c366cd9b
	.xword 0x1124202993385610, 0x6e4ad0e1a30ddf69
	.xword 0x8087c87e03060c18, 0xffe938b633338561
	.xword 0x7e5a29ee636d1eea, 0x0134d92653589793
	.xword 0x49e52d0d9b47ba97, 0x368bddc5ab7233ee
	.xword 0xb738cc9dfb2ca865, 0xc8563c55cb19211c
	.xword 0x5e7bdbf1e3ac9dec, 0x21152b39d3991495
	.xword 0xa0a63a6183c78f1e, 0xdfc8caa9b3f20667
	.xword 0x97193e827bed2b63, 0xe877ce4a4bd8a21a
	.xword 0x69c4df121b863991, 0x16aa2fda2bb3b0e8
	.xword 0xf86737458bb86399, 0x8709c78dbb8deae0
	.xword 0x06bad6d5ebd3716b, 0x79d4261ddbe6f812
	.xword 0x3105d23613f9d516, 0x4e6b22fe23cc5c6f
	.xword 0xcfd833a67392c7e4, 0xb0b6c36e43a74e9d
	.xword 0x9a6c9329ac4bc9b5, 0xe50263e19c7e40cc
	.xword 0x64b172b9cc20db47, 0x1bdf8271fc15523e
	.xword 0x530e765a340a7f3a, 0x2c608692043ff643
	.xword 0xadd397ca54616dc8, 0xd2bd67026454e4b1
	.xword 0x3c707f9dc45f37c0, 0x431e8f55f46abeb9
	.xword 0xc2ad9e0da4342532, 0xbdc36ec59401ac4b
	.xword 0xf5129aee5c1e814f, 0x8a7c6a266c2b0836
	.xword 0x0bcf7b7e3c7593bd, 0x74a18bb60c401ac4
	.xword 0xe28c6c1224f5a634, 0x9de29cda14c02f4d
	.xword 0x1c518d82449eb4c6, 0x633f7d4a74ab3dbf
	.xword 0x2bee8961bcb410bb, 0x548079a98c8199c2
	.xword 0xd53368f1dcdf0249, 0xaa5d9839ecea8b30
	.xword 0x449080a64ce15841, 0x3bfe706e7cd4d138
	.xword 0xba4d61362c8a4ab3, 0xc52391fe1cbfc3ca
	.xword 0x8df265d5d4a0eece, 0xf29c951de49567b7
	.xword 0x732f8445b4cbfc3c, 0x0c41748d84fe7545
	.xword 0x6bad6d5ebd3716b7, 0x14c39d968d029fce
	.xword 0x95708ccedd5c0445, 0xea1e7c06ed698d3c
	.xword 0xa2cf882d2576a038, 0xdda178e515432941
	.xword 0x5c1269bd451db2ca, 0x237c997575283bb3
	.xword 0xcdb181ead523e8c2, 0xb2df7122e51661bb
	.xword 0x336c607ab548fa30, 0x4c0290b2857d7349
	.xword 0x04d364994d625e4d, 0x7bbd94517d57d734
	.xword 0xfa0e85092d094cbf, 0x856075c11d3cc5c6
	.xword 0x134d926535897936, 0x6c2362ad05bcf04f
	.xword 0xed9073f555e26bc4, 0x92fe833d65d7e2bd
	.xword 0xda2f7716adc8cfb9, 0xa54187de9dfd46c0
	.xword 0x24f29686cda3dd4b, 0x5b9c664efd965432
	.xword 0xb5517ed15d9d8743, 0xca3f8e196da80e3a
	.xword 0x4b8c9f413df695b1, 0x34e26f890dc31cc8
	.xword 0x7c339ba2c5dc31cc, 0x035d6b6af5e9b8b5
	.xword 0x82ee7a32a5b7233e, 0xfd808afa9582aa47
	.xword 0x4d364994d625e4da, 0x3258b95ce6106da3
	.xword 0xb3eba804b64ef628, 0xcc8558cc867b7f51
	.xword 0x8454ace74e645255, 0xfb3a5c2f7e51db2c
	.xword 0x7a894d772e0f40a7, 0x05e7bdbf1e3ac9de
	.xword 0xeb2aa520be311aaf, 0x944455e88e0493d6
	.xword 0x15f744b0de5a085d, 0x6a99b478ee6f8124
	.xword 0x224840532670ac20, 0x5d26b09b16452559
	.xword 0xdc95a1c3461bbed2, 0xa3fb510b762e37ab
	.xword 0x35d6b6af5e9b8b5b, 0x4ab846676eae0222
	.xword 0xcb0b573f3ef099a9, 0xb465a7f70ec510d0
	.xword 0xfcb453dcc6da3dd4, 0x83daa314f6efb4ad
	.xword 0x0269b24ca6b12f26, 0x7d0742849684a65f
	.xword 0x93ca5a1b368f752e, 0xeca4aad306bafc57
	.xword 0x6d17bb8b56e467dc, 0x12794b4366d1eea5
	.xword 0x5aa8bf68aecec3a1, 0x25c64fa09efb4ad8
	.xword 0xa4755ef8cea5d153, 0xdb1bae30fe90582a
	.xword 0xbcf7b7e3c7593bd8, 0xc399472bf76cb2a1
	.xword 0x422a5673a732292a, 0x3d44a6bb9707a053
	.xword 0x759552905f188d57, 0x0afba2586f2d042e
	.xword 0x8b48b3003f739fa5, 0xf42643c80f4616dc
	.xword 0x1aeb5b57af4dc5ad, 0x6585ab9f9f784cd4
	.xword 0xe436bac7cf26d75f, 0x9b584a0fff135e26
	.xword 0xd389be24370c7322, 0xace74eec0739fa5b
	.xword 0x2d545fb4576761d0, 0x523aaf7c6752e8a9
	.xword 0xc41748d84fe75459, 0xbb79b8107fd2dd20
	.xword 0x3acaa9482f8c46ab, 0x45a459801fb9cfd2
	.xword 0x0d75adabd7a6e2d6, 0x721b5d63e7936baf
	.xword 0xf3a84c3bb7cdf024, 0x8cc6bcf387f8795d
	.xword 0x620ba46c27f3aa2c, 0x1d6554a417c62355
	.xword 0x9cd645fc4798b8de, 0xe3b8b53477ad31a7
	.xword 0xab69411fbfb21ca3, 0xd407b1d78f8795da
	.xword 0x55b4a08fdfd90e51, 0x2ada5047efec8728
