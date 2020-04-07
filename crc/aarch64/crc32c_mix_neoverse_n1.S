/**********************************************************************
  Copyright(c) 2020 Arm Corporation All rights reserved.

  Redistribution and use in source and binary forms, with or without
  modification, are permitted provided that the following conditions
  are met:
    * Redistributions of source code must retain the above copyright
      notice, this list of conditions and the following disclaimer.
    * Redistributions in binary form must reproduce the above copyright
      notice, this list of conditions and the following disclaimer in
      the documentation and/or other materials provided with the
      distribution.
    * Neither the name of Arm Corporation nor the names of its
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

	.text
	.align		6
	.arch		armv8-a+crypto+crc

#include "crc32_common_mix_neoverse_n1.S"
.Lconstants:
	.octa		0x000000009e4addf800000000740eef02
	.octa		0x000000014cd00bd600000000f20c0dfe
	.octa		0x00000000dea713f10000000105ec76f0
	.quad		0x00000000dd45aab8
	.quad		0x00000000FFFFFFFF
	.quad		0x000000009ef68d35

.macro	crc32_u64	dst,src,data
	crc32cx		\dst,\src,\data
.endm
.macro	crc32_u32	dst,src,data
	crc32cw		\dst,\src,\data
.endm
.macro	crc32_u16	dst,src,data
	crc32ch		\dst,\src,\data
.endm
.macro	crc32_u8	dst,src,data
	crc32cb		\dst,\src,\data
.endm
/**
 *	 uint32_t crc32c_mix_neoverse_n1(uint8_t * BUF,
 *	                     size_t LEN, uint CRC)
 */
	BUF		.req	x0
	LEN		.req	x1
	CRC		.req	x2
	wCRC		.req	w2
	.align		6
	.global		crc32c_mix_neoverse_n1
	.type		crc32c_mix_neoverse_n1, %function
crc32c_mix_neoverse_n1:
	crc32_common_mix	crc32c
	.size		crc32c_mix_neoverse_n1, .-crc32c_mix_neoverse_n1
