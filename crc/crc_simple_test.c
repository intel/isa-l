/**********************************************************************
  Copyright(c) 2011-2013 Intel Corporation All rights reserved.

  Redistribution and use in source and binary forms, with or without
  modification, are permitted provided that the following conditions
  are met:
    * Redistributions of source code must retain the above copyright
      notice, this list of conditions and the following disclaimer.
    * Redistributions in binary form must reproduce the above copyright
      notice, this list of conditions and the following disclaimer in
      the documentation and/or other materials provided with the
      distribution.
    * Neither the name of Intel Corporation nor the names of its
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

#include <stdio.h>
#include <stdint.h>
#include "crc.h"

const uint16_t init_crc_16 = 0x1234;
const uint16_t t10_dif_expected = 0x60b3;
const uint32_t init_crc_32 = 0x12345678;
const uint32_t ieee_expected = 0x2ceadbe3;

int main(void)
{
	unsigned char p_buf[48];
	uint16_t t10_dif_computed;
	uint32_t ieee_computed;
	int i;

	for (i = 0; i < 48; i++)
		p_buf[i] = i;

	t10_dif_computed = crc16_t10dif(init_crc_16, p_buf, 48);

	if (t10_dif_computed != t10_dif_expected)
		printf("WRONG CRC-16(T10 DIF) value\n");
	else
		printf("CORRECT CRC-16(T10 DIF) value\n");

	ieee_computed = crc32_ieee(init_crc_32, p_buf, 48);

	if (ieee_computed != ieee_expected)
		printf("WRONG CRC-32(IEEE) value\n");
	else
		printf("CORRECT CRC-32(IEEE) value\n");

	return 0;
}
