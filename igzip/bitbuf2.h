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
#ifndef BITBUF2_H
#define BITBUF2_H

#include "igzip_lib.h"

#if defined (__unix__) || (__APPLE__) || (__MINGW32__)
#define _mm_stream_si64x(dst, src) *((uint64_t*)dst) = src
#else
#include <intrin.h>
#endif

#ifdef _WIN64
#pragma warning(disable: 4996)
#endif

#ifdef _MSC_VER
#define inline __inline
#endif


/* MAX_BITBUF_BIT WRITE is the maximum number of bits than can be safely written
 * by consecutive calls of write_bits. Note this assumes the bitbuf is in a
 * state that is possible at the exit of write_bits */
#define MAX_BITBUF_BIT_WRITE 56

static inline void init(struct BitBuf2 *me)
{
	me->m_bits = 0;
	me->m_bit_count = 0;
}

static inline void set_buf(struct BitBuf2 *me, unsigned char *buf, unsigned int len)
{
	unsigned int slop = 8;
	me->m_out_buf = me->m_out_start = buf;
	me->m_out_end = buf + len - slop;
}

static inline int is_full(struct BitBuf2 *me)
{
	return (me->m_out_buf > me->m_out_end);
}

static inline uint8_t * buffer_ptr(struct BitBuf2 *me)
{
	return me->m_out_buf;
}

static inline uint32_t buffer_used(struct BitBuf2 *me)
{
	return (uint32_t)(me->m_out_buf - me->m_out_start);
}

static inline uint32_t buffer_bits_used(struct BitBuf2 *me)
{
	return (8 * (uint32_t)(me->m_out_buf - me->m_out_start) + me->m_bit_count);
}

static inline void flush_bits(struct BitBuf2 *me)
{
	uint32_t bits;
	_mm_stream_si64x((int64_t *) me->m_out_buf, me->m_bits);
	bits = me->m_bit_count & ~7;
	me->m_bit_count -= bits;
	me->m_out_buf += bits/8;
	me->m_bits >>= bits;

}

/* Can write up to 8 bytes to output buffer */
static inline void flush(struct BitBuf2 *me)
{
	uint32_t bytes;
	if (me->m_bit_count) {
		_mm_stream_si64x((int64_t *) me->m_out_buf, me->m_bits);
		bytes = (me->m_bit_count + 7) / 8;
		me->m_out_buf += bytes;
	}
	me->m_bits = 0;
	me->m_bit_count = 0;
}

static inline void check_space(struct BitBuf2 *me, uint32_t num_bits)
{
	/* Checks if bitbuf has num_bits extra space and flushes the bytes in
	 * the bitbuf if it doesn't. */
	if (63 - me->m_bit_count < num_bits)
		flush_bits(me);
}

static inline void write_bits_unsafe(struct BitBuf2 *me, uint64_t code, uint32_t count)
{
	me->m_bits |= code << me->m_bit_count;
	me->m_bit_count += count;
}

static inline void write_bits(struct BitBuf2 *me, uint64_t code, uint32_t count)
{	/* Assumes there is space to fit code into m_bits. */
	me->m_bits |= code << me->m_bit_count;
	me->m_bit_count += count;
	flush_bits(me);
}

static inline void write_bits_flush(struct BitBuf2 *me, uint64_t code, uint32_t count)
{	/* Assumes there is space to fit code into m_bits. */
	me->m_bits |= code << me->m_bit_count;
	me->m_bit_count += count;
	flush(me);
}

#endif //BITBUF2_H
