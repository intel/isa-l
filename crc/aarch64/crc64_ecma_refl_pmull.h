########################################################################
#  Copyright(c) 2019 Arm Corporation All rights reserved.
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

.equ	p4_low_b0, 0x41f3
.equ	p4_low_b1, 0x9dd4
.equ	p4_low_b2, 0xefbb
.equ	p4_low_b3, 0x6ae3
.equ	p4_high_b0, 0x2df4
.equ	p4_high_b1, 0xa784
.equ	p4_high_b2, 0x6054
.equ	p4_high_b3, 0x081f

.equ	p1_low_b0, 0x3ae4
.equ	p1_low_b1, 0xca39
.equ	p1_low_b2, 0xd497
.equ	p1_low_b3, 0xe05d
.equ	p1_high_b0, 0x5f40
.equ	p1_high_b1, 0xc787
.equ	p1_high_b2, 0x95af
.equ	p1_high_b3, 0xdabe

.equ	p0_low_b0, 0x5f40
.equ	p0_low_b1, 0xc787
.equ	p0_low_b2, 0x95af
.equ	p0_low_b3, 0xdabe

.equ	br_low_b0, 0x63d5
.equ	br_low_b1, 0x1729
.equ	br_low_b2, 0x466c
.equ	br_low_b3, 0x9c3e
.equ	br_high_b0, 0x1e85
.equ	br_high_b1, 0xaf0e
.equ	br_high_b2, 0xaf2b
.equ	br_high_b3, 0x92d8

	.text
ASM_DEF_RODATA
	.align	4
	.set	.lanchor_crc_tab,. + 0
#ifndef __APPLE__
	.type	crc64_tab, %object
	.size	crc64_tab, 2048
#endif
crc64_tab:
	.xword 0x0000000000000000, 0xb32e4cbe03a75f6f
	.xword 0xf4843657a840a05b, 0x47aa7ae9abe7ff34
	.xword 0x7bd0c384ff8f5e33, 0xc8fe8f3afc28015c
	.xword 0x8f54f5d357cffe68, 0x3c7ab96d5468a107
	.xword 0xf7a18709ff1ebc66, 0x448fcbb7fcb9e309
	.xword 0x0325b15e575e1c3d, 0xb00bfde054f94352
	.xword 0x8c71448d0091e255, 0x3f5f08330336bd3a
	.xword 0x78f572daa8d1420e, 0xcbdb3e64ab761d61
	.xword 0x7d9ba13851336649, 0xceb5ed8652943926
	.xword 0x891f976ff973c612, 0x3a31dbd1fad4997d
	.xword 0x064b62bcaebc387a, 0xb5652e02ad1b6715
	.xword 0xf2cf54eb06fc9821, 0x41e11855055bc74e
	.xword 0x8a3a2631ae2dda2f, 0x39146a8fad8a8540
	.xword 0x7ebe1066066d7a74, 0xcd905cd805ca251b
	.xword 0xf1eae5b551a2841c, 0x42c4a90b5205db73
	.xword 0x056ed3e2f9e22447, 0xb6409f5cfa457b28
	.xword 0xfb374270a266cc92, 0x48190ecea1c193fd
	.xword 0x0fb374270a266cc9, 0xbc9d3899098133a6
	.xword 0x80e781f45de992a1, 0x33c9cd4a5e4ecdce
	.xword 0x7463b7a3f5a932fa, 0xc74dfb1df60e6d95
	.xword 0x0c96c5795d7870f4, 0xbfb889c75edf2f9b
	.xword 0xf812f32ef538d0af, 0x4b3cbf90f69f8fc0
	.xword 0x774606fda2f72ec7, 0xc4684a43a15071a8
	.xword 0x83c230aa0ab78e9c, 0x30ec7c140910d1f3
	.xword 0x86ace348f355aadb, 0x3582aff6f0f2f5b4
	.xword 0x7228d51f5b150a80, 0xc10699a158b255ef
	.xword 0xfd7c20cc0cdaf4e8, 0x4e526c720f7dab87
	.xword 0x09f8169ba49a54b3, 0xbad65a25a73d0bdc
	.xword 0x710d64410c4b16bd, 0xc22328ff0fec49d2
	.xword 0x85895216a40bb6e6, 0x36a71ea8a7ace989
	.xword 0x0adda7c5f3c4488e, 0xb9f3eb7bf06317e1
	.xword 0xfe5991925b84e8d5, 0x4d77dd2c5823b7ba
	.xword 0x64b62bcaebc387a1, 0xd7986774e864d8ce
	.xword 0x90321d9d438327fa, 0x231c512340247895
	.xword 0x1f66e84e144cd992, 0xac48a4f017eb86fd
	.xword 0xebe2de19bc0c79c9, 0x58cc92a7bfab26a6
	.xword 0x9317acc314dd3bc7, 0x2039e07d177a64a8
	.xword 0x67939a94bc9d9b9c, 0xd4bdd62abf3ac4f3
	.xword 0xe8c76f47eb5265f4, 0x5be923f9e8f53a9b
	.xword 0x1c4359104312c5af, 0xaf6d15ae40b59ac0
	.xword 0x192d8af2baf0e1e8, 0xaa03c64cb957be87
	.xword 0xeda9bca512b041b3, 0x5e87f01b11171edc
	.xword 0x62fd4976457fbfdb, 0xd1d305c846d8e0b4
	.xword 0x96797f21ed3f1f80, 0x2557339fee9840ef
	.xword 0xee8c0dfb45ee5d8e, 0x5da24145464902e1
	.xword 0x1a083bacedaefdd5, 0xa9267712ee09a2ba
	.xword 0x955cce7fba6103bd, 0x267282c1b9c65cd2
	.xword 0x61d8f8281221a3e6, 0xd2f6b4961186fc89
	.xword 0x9f8169ba49a54b33, 0x2caf25044a02145c
	.xword 0x6b055fede1e5eb68, 0xd82b1353e242b407
	.xword 0xe451aa3eb62a1500, 0x577fe680b58d4a6f
	.xword 0x10d59c691e6ab55b, 0xa3fbd0d71dcdea34
	.xword 0x6820eeb3b6bbf755, 0xdb0ea20db51ca83a
	.xword 0x9ca4d8e41efb570e, 0x2f8a945a1d5c0861
	.xword 0x13f02d374934a966, 0xa0de61894a93f609
	.xword 0xe7741b60e174093d, 0x545a57dee2d35652
	.xword 0xe21ac88218962d7a, 0x5134843c1b317215
	.xword 0x169efed5b0d68d21, 0xa5b0b26bb371d24e
	.xword 0x99ca0b06e7197349, 0x2ae447b8e4be2c26
	.xword 0x6d4e3d514f59d312, 0xde6071ef4cfe8c7d
	.xword 0x15bb4f8be788911c, 0xa6950335e42fce73
	.xword 0xe13f79dc4fc83147, 0x521135624c6f6e28
	.xword 0x6e6b8c0f1807cf2f, 0xdd45c0b11ba09040
	.xword 0x9aefba58b0476f74, 0x29c1f6e6b3e0301b
	.xword 0xc96c5795d7870f42, 0x7a421b2bd420502d
	.xword 0x3de861c27fc7af19, 0x8ec62d7c7c60f076
	.xword 0xb2bc941128085171, 0x0192d8af2baf0e1e
	.xword 0x4638a2468048f12a, 0xf516eef883efae45
	.xword 0x3ecdd09c2899b324, 0x8de39c222b3eec4b
	.xword 0xca49e6cb80d9137f, 0x7967aa75837e4c10
	.xword 0x451d1318d716ed17, 0xf6335fa6d4b1b278
	.xword 0xb199254f7f564d4c, 0x02b769f17cf11223
	.xword 0xb4f7f6ad86b4690b, 0x07d9ba1385133664
	.xword 0x4073c0fa2ef4c950, 0xf35d8c442d53963f
	.xword 0xcf273529793b3738, 0x7c0979977a9c6857
	.xword 0x3ba3037ed17b9763, 0x888d4fc0d2dcc80c
	.xword 0x435671a479aad56d, 0xf0783d1a7a0d8a02
	.xword 0xb7d247f3d1ea7536, 0x04fc0b4dd24d2a59
	.xword 0x3886b22086258b5e, 0x8ba8fe9e8582d431
	.xword 0xcc0284772e652b05, 0x7f2cc8c92dc2746a
	.xword 0x325b15e575e1c3d0, 0x8175595b76469cbf
	.xword 0xc6df23b2dda1638b, 0x75f16f0cde063ce4
	.xword 0x498bd6618a6e9de3, 0xfaa59adf89c9c28c
	.xword 0xbd0fe036222e3db8, 0x0e21ac88218962d7
	.xword 0xc5fa92ec8aff7fb6, 0x76d4de52895820d9
	.xword 0x317ea4bb22bfdfed, 0x8250e80521188082
	.xword 0xbe2a516875702185, 0x0d041dd676d77eea
	.xword 0x4aae673fdd3081de, 0xf9802b81de97deb1
	.xword 0x4fc0b4dd24d2a599, 0xfceef8632775faf6
	.xword 0xbb44828a8c9205c2, 0x086ace348f355aad
	.xword 0x34107759db5dfbaa, 0x873e3be7d8faa4c5
	.xword 0xc094410e731d5bf1, 0x73ba0db070ba049e
	.xword 0xb86133d4dbcc19ff, 0x0b4f7f6ad86b4690
	.xword 0x4ce50583738cb9a4, 0xffcb493d702be6cb
	.xword 0xc3b1f050244347cc, 0x709fbcee27e418a3
	.xword 0x3735c6078c03e797, 0x841b8ab98fa4b8f8
	.xword 0xadda7c5f3c4488e3, 0x1ef430e13fe3d78c
	.xword 0x595e4a08940428b8, 0xea7006b697a377d7
	.xword 0xd60abfdbc3cbd6d0, 0x6524f365c06c89bf
	.xword 0x228e898c6b8b768b, 0x91a0c532682c29e4
	.xword 0x5a7bfb56c35a3485, 0xe955b7e8c0fd6bea
	.xword 0xaeffcd016b1a94de, 0x1dd181bf68bdcbb1
	.xword 0x21ab38d23cd56ab6, 0x9285746c3f7235d9
	.xword 0xd52f0e859495caed, 0x6601423b97329582
	.xword 0xd041dd676d77eeaa, 0x636f91d96ed0b1c5
	.xword 0x24c5eb30c5374ef1, 0x97eba78ec690119e
	.xword 0xab911ee392f8b099, 0x18bf525d915feff6
	.xword 0x5f1528b43ab810c2, 0xec3b640a391f4fad
	.xword 0x27e05a6e926952cc, 0x94ce16d091ce0da3
	.xword 0xd3646c393a29f297, 0x604a2087398eadf8
	.xword 0x5c3099ea6de60cff, 0xef1ed5546e415390
	.xword 0xa8b4afbdc5a6aca4, 0x1b9ae303c601f3cb
	.xword 0x56ed3e2f9e224471, 0xe5c372919d851b1e
	.xword 0xa26908783662e42a, 0x114744c635c5bb45
	.xword 0x2d3dfdab61ad1a42, 0x9e13b115620a452d
	.xword 0xd9b9cbfcc9edba19, 0x6a978742ca4ae576
	.xword 0xa14cb926613cf817, 0x1262f598629ba778
	.xword 0x55c88f71c97c584c, 0xe6e6c3cfcadb0723
	.xword 0xda9c7aa29eb3a624, 0x69b2361c9d14f94b
	.xword 0x2e184cf536f3067f, 0x9d36004b35545910
	.xword 0x2b769f17cf112238, 0x9858d3a9ccb67d57
	.xword 0xdff2a94067518263, 0x6cdce5fe64f6dd0c
	.xword 0x50a65c93309e7c0b, 0xe388102d33392364
	.xword 0xa4226ac498dedc50, 0x170c267a9b79833f
	.xword 0xdcd7181e300f9e5e, 0x6ff954a033a8c131
	.xword 0x28532e49984f3e05, 0x9b7d62f79be8616a
	.xword 0xa707db9acf80c06d, 0x14299724cc279f02
	.xword 0x5383edcd67c06036, 0xe0ada17364673f59
