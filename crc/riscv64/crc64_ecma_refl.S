########################################################################
#  Copyright(c) 2024 ByteDance All rights reserved.
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
#    * Neither the name of ByteDance Corporation nor the names of its
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
########################################################################

#include "crc64_refl_common_clmul.h"

/* uint64_t crc64_ecma_refl(uint64_t init_crc, uint8_t *buf, unt64_t len) */
crc64_func_refl crc64_ecma_refl

/* precomputed folding constants */
.poly:
	.dword 0x92d8af2baf0e1e85 /* poly reflected, excluding leading 1 */
.mu:
	.dword 0x9c3e466c172963d5
.k1:
	.dword 0x6ae3efbb9dd441f3
.k2:
	.dword 0x081f6054a7842df4
.k3:
	.dword 0xe05dd497ca393ae4
.k4:
.k5:
	.dword 0xdabe95afc7875f40

/* lookup table */
.crc64_table:
	.dword 0x0000000000000000, 0xb32e4cbe03a75f6f
	.dword 0xf4843657a840a05b, 0x47aa7ae9abe7ff34
	.dword 0x7bd0c384ff8f5e33, 0xc8fe8f3afc28015c
	.dword 0x8f54f5d357cffe68, 0x3c7ab96d5468a107
	.dword 0xf7a18709ff1ebc66, 0x448fcbb7fcb9e309
	.dword 0x0325b15e575e1c3d, 0xb00bfde054f94352
	.dword 0x8c71448d0091e255, 0x3f5f08330336bd3a
	.dword 0x78f572daa8d1420e, 0xcbdb3e64ab761d61
	.dword 0x7d9ba13851336649, 0xceb5ed8652943926
	.dword 0x891f976ff973c612, 0x3a31dbd1fad4997d
	.dword 0x064b62bcaebc387a, 0xb5652e02ad1b6715
	.dword 0xf2cf54eb06fc9821, 0x41e11855055bc74e
	.dword 0x8a3a2631ae2dda2f, 0x39146a8fad8a8540
	.dword 0x7ebe1066066d7a74, 0xcd905cd805ca251b
	.dword 0xf1eae5b551a2841c, 0x42c4a90b5205db73
	.dword 0x056ed3e2f9e22447, 0xb6409f5cfa457b28
	.dword 0xfb374270a266cc92, 0x48190ecea1c193fd
	.dword 0x0fb374270a266cc9, 0xbc9d3899098133a6
	.dword 0x80e781f45de992a1, 0x33c9cd4a5e4ecdce
	.dword 0x7463b7a3f5a932fa, 0xc74dfb1df60e6d95
	.dword 0x0c96c5795d7870f4, 0xbfb889c75edf2f9b
	.dword 0xf812f32ef538d0af, 0x4b3cbf90f69f8fc0
	.dword 0x774606fda2f72ec7, 0xc4684a43a15071a8
	.dword 0x83c230aa0ab78e9c, 0x30ec7c140910d1f3
	.dword 0x86ace348f355aadb, 0x3582aff6f0f2f5b4
	.dword 0x7228d51f5b150a80, 0xc10699a158b255ef
	.dword 0xfd7c20cc0cdaf4e8, 0x4e526c720f7dab87
	.dword 0x09f8169ba49a54b3, 0xbad65a25a73d0bdc
	.dword 0x710d64410c4b16bd, 0xc22328ff0fec49d2
	.dword 0x85895216a40bb6e6, 0x36a71ea8a7ace989
	.dword 0x0adda7c5f3c4488e, 0xb9f3eb7bf06317e1
	.dword 0xfe5991925b84e8d5, 0x4d77dd2c5823b7ba
	.dword 0x64b62bcaebc387a1, 0xd7986774e864d8ce
	.dword 0x90321d9d438327fa, 0x231c512340247895
	.dword 0x1f66e84e144cd992, 0xac48a4f017eb86fd
	.dword 0xebe2de19bc0c79c9, 0x58cc92a7bfab26a6
	.dword 0x9317acc314dd3bc7, 0x2039e07d177a64a8
	.dword 0x67939a94bc9d9b9c, 0xd4bdd62abf3ac4f3
	.dword 0xe8c76f47eb5265f4, 0x5be923f9e8f53a9b
	.dword 0x1c4359104312c5af, 0xaf6d15ae40b59ac0
	.dword 0x192d8af2baf0e1e8, 0xaa03c64cb957be87
	.dword 0xeda9bca512b041b3, 0x5e87f01b11171edc
	.dword 0x62fd4976457fbfdb, 0xd1d305c846d8e0b4
	.dword 0x96797f21ed3f1f80, 0x2557339fee9840ef
	.dword 0xee8c0dfb45ee5d8e, 0x5da24145464902e1
	.dword 0x1a083bacedaefdd5, 0xa9267712ee09a2ba
	.dword 0x955cce7fba6103bd, 0x267282c1b9c65cd2
	.dword 0x61d8f8281221a3e6, 0xd2f6b4961186fc89
	.dword 0x9f8169ba49a54b33, 0x2caf25044a02145c
	.dword 0x6b055fede1e5eb68, 0xd82b1353e242b407
	.dword 0xe451aa3eb62a1500, 0x577fe680b58d4a6f
	.dword 0x10d59c691e6ab55b, 0xa3fbd0d71dcdea34
	.dword 0x6820eeb3b6bbf755, 0xdb0ea20db51ca83a
	.dword 0x9ca4d8e41efb570e, 0x2f8a945a1d5c0861
	.dword 0x13f02d374934a966, 0xa0de61894a93f609
	.dword 0xe7741b60e174093d, 0x545a57dee2d35652
	.dword 0xe21ac88218962d7a, 0x5134843c1b317215
	.dword 0x169efed5b0d68d21, 0xa5b0b26bb371d24e
	.dword 0x99ca0b06e7197349, 0x2ae447b8e4be2c26
	.dword 0x6d4e3d514f59d312, 0xde6071ef4cfe8c7d
	.dword 0x15bb4f8be788911c, 0xa6950335e42fce73
	.dword 0xe13f79dc4fc83147, 0x521135624c6f6e28
	.dword 0x6e6b8c0f1807cf2f, 0xdd45c0b11ba09040
	.dword 0x9aefba58b0476f74, 0x29c1f6e6b3e0301b
	.dword 0xc96c5795d7870f42, 0x7a421b2bd420502d
	.dword 0x3de861c27fc7af19, 0x8ec62d7c7c60f076
	.dword 0xb2bc941128085171, 0x0192d8af2baf0e1e
	.dword 0x4638a2468048f12a, 0xf516eef883efae45
	.dword 0x3ecdd09c2899b324, 0x8de39c222b3eec4b
	.dword 0xca49e6cb80d9137f, 0x7967aa75837e4c10
	.dword 0x451d1318d716ed17, 0xf6335fa6d4b1b278
	.dword 0xb199254f7f564d4c, 0x02b769f17cf11223
	.dword 0xb4f7f6ad86b4690b, 0x07d9ba1385133664
	.dword 0x4073c0fa2ef4c950, 0xf35d8c442d53963f
	.dword 0xcf273529793b3738, 0x7c0979977a9c6857
	.dword 0x3ba3037ed17b9763, 0x888d4fc0d2dcc80c
	.dword 0x435671a479aad56d, 0xf0783d1a7a0d8a02
	.dword 0xb7d247f3d1ea7536, 0x04fc0b4dd24d2a59
	.dword 0x3886b22086258b5e, 0x8ba8fe9e8582d431
	.dword 0xcc0284772e652b05, 0x7f2cc8c92dc2746a
	.dword 0x325b15e575e1c3d0, 0x8175595b76469cbf
	.dword 0xc6df23b2dda1638b, 0x75f16f0cde063ce4
	.dword 0x498bd6618a6e9de3, 0xfaa59adf89c9c28c
	.dword 0xbd0fe036222e3db8, 0x0e21ac88218962d7
	.dword 0xc5fa92ec8aff7fb6, 0x76d4de52895820d9
	.dword 0x317ea4bb22bfdfed, 0x8250e80521188082
	.dword 0xbe2a516875702185, 0x0d041dd676d77eea
	.dword 0x4aae673fdd3081de, 0xf9802b81de97deb1
	.dword 0x4fc0b4dd24d2a599, 0xfceef8632775faf6
	.dword 0xbb44828a8c9205c2, 0x086ace348f355aad
	.dword 0x34107759db5dfbaa, 0x873e3be7d8faa4c5
	.dword 0xc094410e731d5bf1, 0x73ba0db070ba049e
	.dword 0xb86133d4dbcc19ff, 0x0b4f7f6ad86b4690
	.dword 0x4ce50583738cb9a4, 0xffcb493d702be6cb
	.dword 0xc3b1f050244347cc, 0x709fbcee27e418a3
	.dword 0x3735c6078c03e797, 0x841b8ab98fa4b8f8
	.dword 0xadda7c5f3c4488e3, 0x1ef430e13fe3d78c
	.dword 0x595e4a08940428b8, 0xea7006b697a377d7
	.dword 0xd60abfdbc3cbd6d0, 0x6524f365c06c89bf
	.dword 0x228e898c6b8b768b, 0x91a0c532682c29e4
	.dword 0x5a7bfb56c35a3485, 0xe955b7e8c0fd6bea
	.dword 0xaeffcd016b1a94de, 0x1dd181bf68bdcbb1
	.dword 0x21ab38d23cd56ab6, 0x9285746c3f7235d9
	.dword 0xd52f0e859495caed, 0x6601423b97329582
	.dword 0xd041dd676d77eeaa, 0x636f91d96ed0b1c5
	.dword 0x24c5eb30c5374ef1, 0x97eba78ec690119e
	.dword 0xab911ee392f8b099, 0x18bf525d915feff6
	.dword 0x5f1528b43ab810c2, 0xec3b640a391f4fad
	.dword 0x27e05a6e926952cc, 0x94ce16d091ce0da3
	.dword 0xd3646c393a29f297, 0x604a2087398eadf8
	.dword 0x5c3099ea6de60cff, 0xef1ed5546e415390
	.dword 0xa8b4afbdc5a6aca4, 0x1b9ae303c601f3cb
	.dword 0x56ed3e2f9e224471, 0xe5c372919d851b1e
	.dword 0xa26908783662e42a, 0x114744c635c5bb45
	.dword 0x2d3dfdab61ad1a42, 0x9e13b115620a452d
	.dword 0xd9b9cbfcc9edba19, 0x6a978742ca4ae576
	.dword 0xa14cb926613cf817, 0x1262f598629ba778
	.dword 0x55c88f71c97c584c, 0xe6e6c3cfcadb0723
	.dword 0xda9c7aa29eb3a624, 0x69b2361c9d14f94b
	.dword 0x2e184cf536f3067f, 0x9d36004b35545910
	.dword 0x2b769f17cf112238, 0x9858d3a9ccb67d57
	.dword 0xdff2a94067518263, 0x6cdce5fe64f6dd0c
	.dword 0x50a65c93309e7c0b, 0xe388102d33392364
	.dword 0xa4226ac498dedc50, 0x170c267a9b79833f
	.dword 0xdcd7181e300f9e5e, 0x6ff954a033a8c131
	.dword 0x28532e49984f3e05, 0x9b7d62f79be8616a
	.dword 0xa707db9acf80c06d, 0x14299724cc279f02
	.dword 0x5383edcd67c06036, 0xe0ada17364673f59
