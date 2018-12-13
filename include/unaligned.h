/**********************************************************************
  Copyright(c) 2011-2019 Intel Corporation All rights reserved.

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

#ifndef UNALIGNED_H
#define UNALIGNED_H

#include "stdint.h"
#include "string.h"

static inline uint16_t load_u16(uint8_t * buf) {
	uint16_t ret;
	memcpy(&ret, buf, sizeof(ret));
	return ret;
}

static inline uint32_t load_u32(uint8_t * buf) {
	uint32_t ret;
	memcpy(&ret, buf, sizeof(ret));
	return ret;
}

static inline uint64_t load_u64(uint8_t * buf) {
	uint64_t ret;
	memcpy(&ret, buf, sizeof(ret));
	return ret;
}

static inline uintmax_t load_umax(uint8_t * buf) {
	uintmax_t ret;
	memcpy(&ret, buf, sizeof(ret));
	return ret;
}

static inline void store_u16(uint8_t * buf, uint16_t val) {
	memcpy(buf, &val, sizeof(val));
}

static inline void store_u32(uint8_t * buf, uint32_t val) {
	memcpy(buf, &val, sizeof(val));
}

static inline void store_u64(uint8_t * buf, uint64_t val) {
	memcpy(buf, &val, sizeof(val));
}

static inline void store_umax(uint8_t * buf, uintmax_t val) {
	memcpy(buf, &val, sizeof(val));
}

#endif
