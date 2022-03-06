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
#include "stdlib.h"
#include "string.h"

#ifdef __FreeBSD__
#include <sys/types.h>
#include <sys/endian.h>
# define isal_bswap16(x) bswap16(x)
# define isal_bswap32(x) bswap32(x)
# define isal_bswap64(x) bswap64(x)
#elif defined (__APPLE__)
#include <libkern/OSByteOrder.h>
# define isal_bswap16(x) OSSwapInt16(x)
# define isal_bswap32(x) OSSwapInt32(x)
# define isal_bswap64(x) OSSwapInt64(x)
#elif defined (__GNUC__) && !defined (__MINGW32__)
# include <byteswap.h>
# define isal_bswap16(x) bswap_16(x)
# define isal_bswap32(x) bswap_32(x)
# define isal_bswap64(x) bswap_64(x)
#elif defined _WIN64
# define isal_bswap16(x) _byteswap_ushort(x)
# define isal_bswap32(x) _byteswap_ulong(x)
# define isal_bswap64(x) _byteswap_uint64(x)
#endif

#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
# define to_be16(x) isal_bswap16(x)
# define from_be16(x) isal_bswap16(x)
# define to_be32(x) isal_bswap32(x)
# define from_be32(x) isal_bswap32(x)
# define to_be64(x) isal_bswap64(x)
# define from_be64(x) isal_bswap64(x)
# define to_le16(x) (x)
# define from_le16(x) (x)
# define to_le32(x) (x)
# define from_le32(x) (x)
# define to_le64(x) (x)
# define from_le64(x) (x)
#else
# define to_be16(x) (x)
# define from_be16(x) (x)
# define to_be32(x) (x)
# define from_be32(x) (x)
# define to_be64(x) (x)
# define from_be64(x) (x)
# define to_le16(x) isal_bswap16(x)
# define from_le16(x) isal_bswap16(x)
# define to_le32(x) isal_bswap32(x)
# define from_le32(x) isal_bswap32(x)
# define to_le64(x) isal_bswap64(x)
# define from_le64(x) isal_bswap64(x)
#endif

static inline uint16_t load_native_u16(uint8_t * buf)
{
	uint16_t ret;
	memcpy(&ret, buf, sizeof(ret));
	return ret;
}

static inline uint16_t load_le_u16(uint8_t * buf)
{
	return from_le16(load_native_u16(buf));
}

static inline uint16_t load_be_u16(uint8_t * buf)
{
	return from_be16(load_native_u16(buf));
}

static inline uint32_t load_native_u32(uint8_t * buf)
{
	uint32_t ret;
	memcpy(&ret, buf, sizeof(ret));
	return ret;
}

static inline uint32_t load_le_u32(uint8_t * buf)
{
	return from_le32(load_native_u32(buf));
}

static inline uint32_t load_be_u32(uint8_t * buf)
{
	return from_be32(load_native_u32(buf));
}

static inline uint64_t load_native_u64(uint8_t * buf)
{
	uint64_t ret;
	memcpy(&ret, buf, sizeof(ret));
	return ret;
}

static inline uint64_t load_le_u64(uint8_t * buf)
{
	return from_le64(load_native_u64(buf));
}

static inline uint64_t load_be_u64(uint8_t * buf)
{
	return from_be64(load_native_u64(buf));
}

static inline uintmax_t load_le_umax(uint8_t * buf)
{
	switch (sizeof(uintmax_t)) {
	case sizeof(uint32_t):
		return from_le32(load_native_u32(buf));
	case sizeof(uint64_t):
		return from_le64(load_native_u64(buf));
	default:
		return 0;
	}
}

static inline void store_native_u16(uint8_t * buf, uint16_t val)
{
	memcpy(buf, &val, sizeof(val));
}

static inline void store_le_u16(uint8_t * buf, uint16_t val)
{
	store_native_u16(buf, to_le16(val));
}

static inline void store_be_u16(uint8_t * buf, uint16_t val)
{
	store_native_u16(buf, to_be16(val));
}

static inline void store_native_u16_to_u64(uint64_t * buf, uint16_t val)
{
#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
	store_native_u16((uint8_t *) buf, val);
#else
	store_native_u16((uint8_t *) buf + 6, val);
#endif
}

static inline void store_native_u32(uint8_t * buf, uint32_t val)
{
	memcpy(buf, &val, sizeof(val));
}

static inline void store_le_u32(uint8_t * buf, uint32_t val)
{
	store_native_u32(buf, to_le32(val));
}

static inline void store_be_u32(uint8_t * buf, uint32_t val)
{
	store_native_u32(buf, to_be32(val));
}

static inline void store_native_u64(uint8_t * buf, uint64_t val)
{
	memcpy(buf, &val, sizeof(val));
}

static inline void store_le_u64(uint8_t * buf, uint64_t val)
{
	store_native_u64(buf, to_le64(val));
}

static inline void store_be_u64(uint8_t * buf, uint64_t val)
{
	store_native_u64(buf, to_be64(val));
}

#endif
