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
#ifndef _IGZIP_REPEATED_8K_CHAR_RESULT_H_
#define _IGZIP_REPEATED_8K_CHAR_RESULT_H_

/* The code for the literal being encoded */
#define CODE_LIT 0x1
#define CODE_LIT_LENGTH 0x2

/* The code for repeat 10. The Length includes the distance code length*/
#define CODE_10  0x3
#define CODE_10_LENGTH  0x4

/* The code for repeat 115-130. The Length includes the distance code length*/
#define CODE_280 0x0f
#define CODE_280_LENGTH 0x4
#define CODE_280_TOTAL_LENGTH CODE_280_LENGTH + 4 + 1

/* Code representing the end of block. */
#define END_OF_BLOCK 0x7
#define END_OF_BLOCK_LEN 0x4

/* MIN_REPEAT_LEN currently optimizes storage space, another possiblity is to
 * find the size which optimizes speed instead.*/
#define MIN_REPEAT_LEN 4*1024

#define HEADER_LENGTH 16

/* Maximum length of the portion of the header represented by repeat lengths
 * smaller than 258 */
#define MAX_FIXUP_CODE_LENGTH 8


/* Headers for constant 0x00 and 0xFF blocks
 * This also contains the first literal character. */
const uint32_t repeated_char_header[2][5] = {
	{ 0x0121c0ec, 0xc30c0000, 0x7d57fab0, 0x49270938}, /* Deflate header for 0x00 */
	{ 0x0121c0ec, 0xc30c0000, 0x7baaff30, 0x49270938}  /* Deflate header for 0xFF */

};

#endif /*_IGZIP_REPEATED_8K_CHAR_RESULT_H_*/
