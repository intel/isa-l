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




.macro	crc32_hw_common		poly_type
	cbz		LEN, .zero_length_ret
.ifc	\poly_type,crc32
	mvn		wCRC,wCRC
.endif
	tbz		BUF, 0, .align_short
	ldrb		wdata,[BUF],1
	sub		LEN,LEN,1
	crc32_u8	wCRC,wCRC,wdata
.align_short:
	tst		BUF,2
	ccmp		LEN,1,0,ne
	bhi		.align_short_2
	tst		BUF,4
	ccmp		LEN,3,0,ne
	bhi		.align_word

.align_finish:

	cmp		LEN, 63
	bls		.loop_16B
.loop_64B:
	ldp		data0, data1, [BUF],#16
	sub		LEN,LEN,#64
	ldp		data2, data3, [BUF],#16
	cmp		LEN,#64
	crc32_u64	wCRC, wCRC, data0
	crc32_u64	wCRC, wCRC, data1
	ldp		data0, data1, [BUF],#16
	crc32_u64	wCRC, wCRC, data2
	crc32_u64	wCRC, wCRC, data3
	ldp		data2, data3, [BUF],#16
	crc32_u64	wCRC, wCRC, data0
	crc32_u64	wCRC, wCRC, data1
	crc32_u64	wCRC, wCRC, data2
	crc32_u64	wCRC, wCRC, data3
	bge		.loop_64B

.loop_16B:
	cmp		LEN, 15
	bls		.less_16B
	ldp		data0, data1, [BUF],#16
	sub		LEN,LEN,#16
	cmp		LEN,15
	crc32_u64	wCRC, wCRC, data0
	crc32_u64	wCRC, wCRC, data1
	bls		.less_16B
	ldp		data0, data1, [BUF],#16
	sub		LEN,LEN,#16
	cmp		LEN,15
	crc32_u64	wCRC, wCRC, data0
	crc32_u64	wCRC, wCRC, data1
	bls		.less_16B
	ldp		data0, data1, [BUF],#16
	sub		LEN,LEN,#16   //MUST less than 16B
	crc32_u64	wCRC, wCRC, data0
	crc32_u64	wCRC, wCRC, data1
.less_16B:
	cmp		LEN, 7
	bls		.less_8B
	ldr		data0, [BUF], 8
	sub		LEN, LEN, #8
	crc32_u64	wCRC, wCRC, data0
.less_8B:
	cmp		LEN, 3
	bls		.less_4B
	ldr		wdata, [BUF], 4
	sub		LEN, LEN, #4
	crc32_u32	wCRC, wCRC, wdata
.less_4B:
	cmp		LEN, 1
	bls		.less_2B
	ldrh		wdata, [BUF], 2
	sub		LEN, LEN, #2
	crc32_u16	wCRC, wCRC, wdata
.less_2B:
	cbz		LEN, .finish_exit
	ldrb		wdata, [BUF]
	crc32_u8	wCRC, wCRC, wdata
.finish_exit:
.ifc	\poly_type,crc32
	mvn		w0, wCRC
.else
	mov		w0, wCRC
.endif
	ret
.zero_length_ret:
	mov		w0, wCRC
	ret
.align_short_2:
	ldrh		wdata, [BUF], 2
	sub		LEN, LEN, 2
	tst		BUF, 4
	crc32_u16	wCRC, wCRC, wdata
	ccmp		LEN, 3, 0, ne
	bls		.align_finish
.align_word:
	ldr		wdata, [BUF], 4
	sub		LEN, LEN, #4
	crc32_u32	wCRC, wCRC, wdata
	b .align_finish

.endm
