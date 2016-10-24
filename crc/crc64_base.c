/**********************************************************************
  Copyright(c) 2011-2016 Intel Corporation All rights reserved.

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

#include <stdlib.h>
#include "crc64.h"

#define MAX_ITER	8

// crc64_ecma baseline function
// Slow crc64 from the definition.  Can be sped up with a lookup table.
uint64_t crc64_ecma_refl_base(uint64_t seed, const uint8_t * buf, uint64_t len)
{
	uint64_t rem = ~seed;
	unsigned int i, j;

	uint64_t poly = 0xC96C5795D7870F42ULL;	// ECMA-182 standard reflected

	for (i = 0; i < len; i++) {
		rem = rem ^ (uint64_t) buf[i];
		for (j = 0; j < MAX_ITER; j++) {
			rem = (rem & 0x1ULL ? poly : 0) ^ (rem >> 1);
		}
	}
	return ~rem;
}

uint64_t crc64_ecma_norm_base(uint64_t seed, const uint8_t * buf, uint64_t len)
{
	uint64_t rem = ~seed;
	unsigned int i, j;

	uint64_t poly = 0x42F0E1EBA9EA3693ULL;	// ECMA-182 standard

	for (i = 0; i < len; i++) {
		rem = rem ^ ((uint64_t) buf[i] << 56);
		for (j = 0; j < MAX_ITER; j++) {
			rem = (rem & 0x8000000000000000ULL ? poly : 0) ^ (rem << 1);
		}
	}
	return ~rem;
}

// crc64_iso baseline function
// Slow crc64 from the definition.  Can be sped up with a lookup table.
uint64_t crc64_iso_refl_base(uint64_t seed, const uint8_t * buf, uint64_t len)
{
	uint64_t rem = ~seed;
	unsigned int i, j;

	uint64_t poly = 0xD800000000000000ULL;	// ISO standard reflected

	for (i = 0; i < len; i++) {
		rem = rem ^ (uint64_t) buf[i];
		for (j = 0; j < MAX_ITER; j++) {
			rem = (rem & 0x1ULL ? poly : 0) ^ (rem >> 1);
		}
	}
	return ~rem;
}

uint64_t crc64_iso_norm_base(uint64_t seed, const uint8_t * buf, uint64_t len)
{
	uint64_t rem = ~seed;
	unsigned int i, j;

	uint64_t poly = 0x000000000000001BULL;	// ISO standard

	for (i = 0; i < len; i++) {
		rem = rem ^ ((uint64_t) buf[i] << 56);
		for (j = 0; j < MAX_ITER; j++) {
			rem = (rem & 0x8000000000000000ULL ? poly : 0) ^ (rem << 1);
		}
	}
	return ~rem;
}

// crc64_jones baseline function
// Slow crc64 from the definition.  Can be sped up with a lookup table.
uint64_t crc64_jones_refl_base(uint64_t seed, const uint8_t * buf, uint64_t len)
{
	uint64_t rem = ~seed;
	unsigned int i, j;

	uint64_t poly = 0x95ac9329ac4bc9b5ULL;	// Jones coefficients reflected

	for (i = 0; i < len; i++) {
		rem = rem ^ (uint64_t) buf[i];
		for (j = 0; j < MAX_ITER; j++) {
			rem = (rem & 0x1ULL ? poly : 0) ^ (rem >> 1);
		}
	}
	return ~rem;
}

uint64_t crc64_jones_norm_base(uint64_t seed, const uint8_t * buf, uint64_t len)
{
	uint64_t rem = ~seed;
	unsigned int i, j;

	uint64_t poly = 0xad93d23594c935a9ULL;	// Jones coefficients

	for (i = 0; i < len; i++) {
		rem = rem ^ ((uint64_t) buf[i] << 56);
		for (j = 0; j < MAX_ITER; j++) {
			rem = (rem & 0x8000000000000000ULL ? poly : 0) ^ (rem << 1);
		}
	}
	return ~rem;
}

struct slver {
	unsigned short snum;
	unsigned char ver;
	unsigned char core;
};

struct slver crc64_ecma_refl_base_slver_0000001c;
struct slver crc64_ecma_refl_base_slver = { 0x001c, 0x00, 0x00 };

struct slver crc64_ecma_norm_base_slver_00000019;
struct slver crc64_ecma_norm_base_slver = { 0x0019, 0x00, 0x00 };

struct slver crc64_iso_refl_base_slver_00000022;
struct slver crc64_iso_refl_base_slver = { 0x0022, 0x00, 0x00 };

struct slver crc64_iso_norm_base_slver_0000001f;
struct slver crc64_iso_norm_base_slver = { 0x001f, 0x00, 0x00 };

struct slver crc64_jones_refl_base_slver_00000028;
struct slver crc64_jones_refl_base_slver = { 0x0028, 0x00, 0x00 };

struct slver crc64_jones_norm_base_slver_00000025;
struct slver crc64_jones_norm_base_slver = { 0x0025, 0x00, 0x00 };
