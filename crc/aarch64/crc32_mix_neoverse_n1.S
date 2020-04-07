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
	.octa		0x00000001c6e415960000000154442bd4
	.octa		0x00000000ccaa009e00000001751997d0
	.octa		0x00000001F701164100000001DB710641
	.quad		0x0000000163cd6124
	.quad		0x00000000FFFFFFFF
	.quad		0x000000001753ab84
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


/**
 *	 uint32_t crc32_mix_neoverse_n1(uint CRC ,uint8_t * BUF,
 *	                     size_t LEN)
 */
	BUF		.req	x1
	LEN		.req	x2
	CRC		.req	x0
	wCRC		.req	w0
	.align		6
	.global		crc32_mix_neoverse_n1
	.type		crc32_mix_neoverse_n1, %function
crc32_mix_neoverse_n1:
	crc32_common_mix	crc32
	.size		crc32_mix_neoverse_n1, .-crc32_mix_neoverse_n1

