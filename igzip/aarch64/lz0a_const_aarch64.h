/**********************************************************************
  Copyright(c) 2019 Arm Corporation All rights reserved.

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

#ifndef __LZ0A_CONST_AARCH64_H__
#define __LZ0A_CONST_AARCH64_H__
#include "options_aarch64.h"

#ifdef __ASSEMBLY__
.set K , 1024
.set D , IGZIP_HIST_SIZE //  Amount of history
.set LA , 18 * 16 //  Max look-ahead, rounded up to 32 byte boundary
.set BSIZE , 2*IGZIP_HIST_SIZE + LA //  Nominal buffer size

///    Constants for stateless compression
#define LAST_BYTES_COUNT 3 //  Bytes to prevent reading out of array bounds
#define LA_STATELESS 258 //  No round up since no data is copied to a buffer

.set IGZIP_LVL0_HASH_SIZE , (8 * K)
.set IGZIP_HASH8K_HASH_SIZE , (8 * K)
.set IGZIP_HASH_HIST_HASH_SIZE , IGZIP_HIST_SIZE
.set IGZIP_HASH_MAP_HASH_SIZE , IGZIP_HIST_SIZE

#define LVL0_HASH_MASK (IGZIP_LVL0_HASH_SIZE - 1)
#define HASH8K_HASH_MASK (IGZIP_HASH8K_HASH_SIZE - 1)
#define HASH_HIST_HASH_MASK (IGZIP_HASH_HIST_HASH_SIZE - 1)
#define HASH_MAP_HASH_MASK (IGZIP_HASH_MAP_HASH_SIZE - 1)

.set MIN_DEF_MATCH , 3 // Minimum length of a match in deflate
.set SHORTEST_MATCH , 4

.set SLOP , 8

#define ICF_CODE_BYTES 4
#define LIT_LEN_BIT_COUNT 10
#define DIST_LIT_BIT_COUNT 9

#define LIT_LEN_MASK ((1 << LIT_LEN_BIT_COUNT) - 1)
#define LIT_DIST_MASK ((1 << DIST_LIT_BIT_COUNT) - 1)

#define DIST_OFFSET LIT_LEN_BIT_COUNT
#define EXTRA_BITS_OFFSET (DIST_OFFSET + DIST_LIT_BIT_COUNT)
#define LIT (0x1E << DIST_OFFSET)


#endif
#endif
