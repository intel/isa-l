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
	.arch		armv8-a+crc


#include "crc32_aarch64_common.h"
	BUF		.req	x0
	LEN		.req	x1
	wCRC		.req	w2
	data0		.req	x4
	data1		.req	x5
	data2		.req	x6
	data3		.req	x7
	wdata		.req	w3
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
	 * uint32_t crc32_iscsi_crc_ext(const unsigned char *BUF,
	 *                  uint64_t LEN,uint32_t wCRC);
	 */
	.global	crc32_iscsi_crc_ext
	.type	crc32_iscsi_crc_ext, %function
crc32_iscsi_crc_ext:
	crc32_hw_common	crc32c
	.size	crc32_iscsi_crc_ext, .-crc32_iscsi_crc_ext
