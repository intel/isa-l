/**********************************************************************
  Copyright(c) 2011-2018 Intel Corporation All rights reserved.

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

#include <stdint.h>
#include <stddef.h>
#include "unaligned.h"

int mem_zero_detect_base(void *buf, size_t n)
{
	uint8_t *c = buf;
	uintmax_t a = 0;

	// Check buffer in native machine width comparisons
	while (n >= sizeof(uintmax_t)) {
		n -= sizeof(uintmax_t);
		if (load_umax(c) != 0)
			return -1;
		c += sizeof(uintmax_t);
	}

	// Check remaining bytes
	switch (n) {
	case 7:
		a |= *c++;	// fall through to case 6,5,4
	case 6:
		a |= *c++;	// fall through to case 5,4
	case 5:
		a |= *c++;	// fall through to case 4
	case 4:
		a |= load_u32(c);
		break;
	case 3:
		a |= *c++;	// fall through to case 2
	case 2:
		a |= load_u16(c);
		break;
	case 1:
		a |= *c;
		break;
	}

	return (a == 0) ? 0 : -1;
}
