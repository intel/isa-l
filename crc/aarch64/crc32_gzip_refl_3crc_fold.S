########################################################################
#  Copyright(c) 2020 Arm Corporation All rights reserved.
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
#include "crc32_aarch64_common.h"
	.text
	.align		6
	.arch		armv8-a+crc+crypto
.macro	crc32_u64	dst,src,data
	crc32x		\dst,\src,\data
.endm
.macro	crc32_u32	dst,src,data
	crc32w		\dst,\src,\data
.endm
.macro	crc32_u16	dst,src,data
	crc32h		\dst,\src,\data
.endm
.macro	crc32_u8	dst,src,data
	crc32b		\dst,\src,\data
.endm
.macro	declare_var_vector_reg name:req,reg:req
	q\name	.req	q\reg
	v\name	.req	v\reg
	s\name	.req	s\reg
	d\name	.req	d\reg
.endm

	BUF		.req	x1
	ptr_crc0	.req	x1
	LEN		.req	x2
	wCRC		.req	w0
	crc0		.req	w0
	xcrc0		.req	x0

	crc1		.req	w3
	crc2		.req	w4
	xcrc1		.req	x3
	const_adr	.req	x3
	ptr_crc1	.req	x6
	ptr_crc2	.req	x7
	crc0_data0	.req	x9
	crc0_data1	.req	x10
	crc1_data0	.req	x11
	crc1_data1	.req	x12
	crc2_data0	.req	x13
	crc2_data1	.req	x14

	wdata		.req	w3
	data0		.req	x3
	data1		.req	x4
	data2		.req	x5
	data3		.req	x6

	declare_var_vector_reg	tmp0,0
	declare_var_vector_reg	tmp1,1
	declare_var_vector_reg	const0,2
	declare_var_vector_reg	const1,3

/**
	uint32_t crc32_gzip_refl(
		uint32_t wCRC,
		const unsigned char *BUF,
		uint64_t LEN
		);
*/

	.global	crc32_gzip_refl_3crc_fold
	.type	crc32_gzip_refl_3crc_fold, %function
crc32_gzip_refl_3crc_fold:
	crc32_3crc_fold	crc32
	.size	crc32_gzip_refl_3crc_fold, .-crc32_gzip_refl_3crc_fold
