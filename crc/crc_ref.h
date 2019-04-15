/**********************************************************************
  Copyright(c) 2011-2015 Intel Corporation All rights reserved.

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

#ifndef _CRC_REF_H
#define _CRC_REF_H

#ifdef __cplusplus
extern "C" {
#endif

#include "crc.h"

#ifdef _MSC_VER
# define inline __inline
#endif

#define MAX_ITER	8

// iSCSI CRC reference function
static inline unsigned int crc32_iscsi_ref(unsigned char *buffer, int len, unsigned int crc_init)
{
	uint64_t rem = crc_init;
	int i, j;

	uint32_t poly = 0x82F63B78;

	for (i = 0; i < len; i++) {
		rem = rem ^ (buffer[i]);
		for (j = 0; j < MAX_ITER; j++) {
			rem = (rem & 0x1ULL) ? (rem >> 1) ^ poly : (rem >> 1);
		}
	}
	return rem;
}

// crc16_t10dif reference function, slow crc16 from the definition.
static inline uint16_t crc16_t10dif_ref(uint16_t seed, uint8_t * buf, uint64_t len)
{
	size_t rem = seed;
	unsigned int i, j;

	uint16_t poly = 0x8bb7;	// t10dif standard

	for (i = 0; i < len; i++) {
		rem = rem ^ (buf[i] << 8);
		for (j = 0; j < MAX_ITER; j++) {
			rem = rem << 1;
			rem = (rem & 0x10000) ? rem ^ poly : rem;
		}
	}
	return rem;
}

// crc16_t10dif reference function, slow crc16 from the definition.
static inline uint16_t crc16_t10dif_copy_ref(uint16_t seed, uint8_t * dst, uint8_t * src, uint64_t len)
{
	size_t rem = seed;
	unsigned int i, j;

	uint16_t poly = 0x8bb7;	// t10dif standard

	for (i = 0; i < len; i++) {
		rem = rem ^ (src[i] << 8);
		dst[i] = src[i];
		for (j = 0; j < MAX_ITER; j++) {
			rem = rem << 1;
			rem = (rem & 0x10000) ? rem ^ poly : rem;
		}
	}
	return rem;
}

// crc32_ieee reference function, slow crc32 from the definition.
static inline uint32_t crc32_ieee_ref(uint32_t seed, uint8_t * buf, uint64_t len)
{
	uint64_t rem = ~seed;
	unsigned int i, j;

	uint32_t poly = 0x04C11DB7;	// IEEE standard

	for (i = 0; i < len; i++) {
		rem = rem ^ ((uint64_t) buf[i] << 24);
		for (j = 0; j < MAX_ITER; j++) {
			rem = rem << 1;
			rem = (rem & 0x100000000ULL) ? rem ^ poly : rem;
		}
	}
	return ~rem;
}

// crc32_gzip_refl reference function, slow crc32 from the definition.
// Please get difference details between crc32_gzip_ref and crc32_ieee
// from crc.h.
static inline uint32_t crc32_gzip_refl_ref(uint32_t seed, uint8_t * buf, uint64_t len)
{
	uint64_t rem = ~seed;
	int i, j;

	uint32_t poly = 0xEDB88320;	// IEEE standard

	for (i = 0; i < len; i++) {
		rem = rem ^ (buf[i]);
		for (j = 0; j < MAX_ITER; j++) {
			rem = (rem & 0x1ULL) ? (rem >> 1) ^ poly : (rem >> 1);
		}
	}
	return ~rem;
}

#ifdef __cplusplus
}
#endif

#endif
