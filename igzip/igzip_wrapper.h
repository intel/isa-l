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

#ifndef IGZIP_WRAPPER_H

#define DEFLATE_METHOD 8
#define ZLIB_DICT_FLAG (1 << 5)
#define TEXT_FLAG (1 << 0)
#define HCRC_FLAG (1 << 1)
#define EXTRA_FLAG (1 << 2)
#define NAME_FLAG (1 << 3)
#define COMMENT_FLAG (1 << 4)
#define UNDEFINED_FLAG (-1)

#define GZIP_HDR_BASE 10
#define GZIP_EXTRA_LEN 2
#define GZIP_HCRC_LEN 2
#define GZIP_TRAILER_LEN 8

#define ZLIB_HDR_BASE 2
#define ZLIB_DICT_LEN 4
#define ZLIB_INFO_OFFSET 4
#define ZLIB_LEVEL_OFFSET 6
#define ZLIB_TRAILER_LEN 4

#endif
