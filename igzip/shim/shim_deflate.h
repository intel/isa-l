/**********************************************************************
  Copyright(c) 2025 Intel Corporation All rights reserved.

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

typedef struct {
        void *next_in;          /* next input byte */
        unsigned int avail_in;  /* number of bytes available at next_in */
        unsigned long total_in; /* total number of input bytes read so far */

        void *next_out;          /* next output byte should be put there */
        unsigned int avail_out;  /* remaining free space at next_out */
        unsigned long total_out; /* total number of bytes output so far */

        char *msg;   /* last error message, NULL if no error */
        void *state; /* not visible by applications */

        void *zalloc; /* used to allocate the internal state */
        void *zfree;  /* used to free the internal state */
        void *opaque; /* private data object passed to zalloc and zfree */

        int data_type;          /* best guess about the data type: binary or text */
        unsigned long adler;    /* adler32 value of the uncompressed data */
        unsigned long reserved; /* reserved for future use */
} z_stream;

typedef z_stream *z_streamp;

#include "igzip_lib.h"

typedef struct internal_state {
        z_streamp strm;
        int level;
        int w_bits;
        struct isal_zstream *isal_strm;
} deflate_state;

#define Z_NO_FLUSH      0
#define Z_PARTIAL_FLUSH 1
#define Z_SYNC_FLUSH    2
#define Z_FULL_FLUSH    3
#define Z_FINISH        4
#define Z_BLOCK         5
#define Z_TREES         6

#define Z_OK           0
#define Z_STREAM_END   1
#define Z_NEED_DICT    2
#define Z_ERRNO        (-1)
#define Z_STREAM_ERROR (-2)
#define Z_DATA_ERROR   (-3)
#define Z_BUF_ERROR    (-5)

#define Z_DEFAULT_COMPRESSION 6
