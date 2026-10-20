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

#include "huff_codes.h"
#include "huffman.h"
#include "flatten_ll.h"

/* The order code length codes are written in the dynamic code header. This is
 * defined in RFC 1951 page 13 */
static const uint8_t code_length_code_order[] = { 16, 17, 18, 0, 8,  7, 9,  6, 10, 5,
                                                  11, 4,  12, 3, 13, 2, 14, 1, 15 };

static const uint32_t len_code_extra_bits[] = { 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x1, 0x1,
                                                0x1, 0x1, 0x2, 0x2, 0x2, 0x2, 0x3, 0x3, 0x3, 0x3,
                                                0x4, 0x4, 0x4, 0x4, 0x5, 0x5, 0x5, 0x5, 0x0 };

static const uint32_t dist_code_extra_bits[] = { 0x0, 0x0, 0x0, 0x0, 0x1, 0x1, 0x2, 0x2, 0x3, 0x3,
                                                 0x4, 0x4, 0x5, 0x5, 0x6, 0x6, 0x7, 0x7, 0x8, 0x8,
                                                 0x9, 0x9, 0xa, 0xa, 0xb, 0xb, 0xc, 0xc, 0xd, 0xd };

static struct hufftables_icf static_hufftables = {
        .lit_len_table = { { { { .code_and_extra = 0x00c, .length2 = 0x8 } } },
                           { { { .code_and_extra = 0x08c, .length2 = 0x8 } } },
                           { { { .code_and_extra = 0x04c, .length2 = 0x8 } } },
                           { { { .code_and_extra = 0x0cc, .length2 = 0x8 } } },
                           { { { .code_and_extra = 0x02c, .length2 = 0x8 } } },
                           { { { .code_and_extra = 0x0ac, .length2 = 0x8 } } },
                           { { { .code_and_extra = 0x06c, .length2 = 0x8 } } },
                           { { { .code_and_extra = 0x0ec, .length2 = 0x8 } } },
                           { { { .code_and_extra = 0x01c, .length2 = 0x8 } } },
                           { { { .code_and_extra = 0x09c, .length2 = 0x8 } } },
                           { { { .code_and_extra = 0x05c, .length2 = 0x8 } } },
                           { { { .code_and_extra = 0x0dc, .length2 = 0x8 } } },
                           { { { .code_and_extra = 0x03c, .length2 = 0x8 } } },
                           { { { .code_and_extra = 0x0bc, .length2 = 0x8 } } },
                           { { { .code_and_extra = 0x07c, .length2 = 0x8 } } },
                           { { { .code_and_extra = 0x0fc, .length2 = 0x8 } } },
                           { { { .code_and_extra = 0x002, .length2 = 0x8 } } },
                           { { { .code_and_extra = 0x082, .length2 = 0x8 } } },
                           { { { .code_and_extra = 0x042, .length2 = 0x8 } } },
                           { { { .code_and_extra = 0x0c2, .length2 = 0x8 } } },
                           { { { .code_and_extra = 0x022, .length2 = 0x8 } } },
                           { { { .code_and_extra = 0x0a2, .length2 = 0x8 } } },
                           { { { .code_and_extra = 0x062, .length2 = 0x8 } } },
                           { { { .code_and_extra = 0x0e2, .length2 = 0x8 } } },
                           { { { .code_and_extra = 0x012, .length2 = 0x8 } } },
                           { { { .code_and_extra = 0x092, .length2 = 0x8 } } },
                           { { { .code_and_extra = 0x052, .length2 = 0x8 } } },
                           { { { .code_and_extra = 0x0d2, .length2 = 0x8 } } },
                           { { { .code_and_extra = 0x032, .length2 = 0x8 } } },
                           { { { .code_and_extra = 0x0b2, .length2 = 0x8 } } },
                           { { { .code_and_extra = 0x072, .length2 = 0x8 } } },
                           { { { .code_and_extra = 0x0f2, .length2 = 0x8 } } },
                           { { { .code_and_extra = 0x00a, .length2 = 0x8 } } },
                           { { { .code_and_extra = 0x08a, .length2 = 0x8 } } },
                           { { { .code_and_extra = 0x04a, .length2 = 0x8 } } },
                           { { { .code_and_extra = 0x0ca, .length2 = 0x8 } } },
                           { { { .code_and_extra = 0x02a, .length2 = 0x8 } } },
                           { { { .code_and_extra = 0x0aa, .length2 = 0x8 } } },
                           { { { .code_and_extra = 0x06a, .length2 = 0x8 } } },
                           { { { .code_and_extra = 0x0ea, .length2 = 0x8 } } },
                           { { { .code_and_extra = 0x01a, .length2 = 0x8 } } },
                           { { { .code_and_extra = 0x09a, .length2 = 0x8 } } },
                           { { { .code_and_extra = 0x05a, .length2 = 0x8 } } },
                           { { { .code_and_extra = 0x0da, .length2 = 0x8 } } },
                           { { { .code_and_extra = 0x03a, .length2 = 0x8 } } },
                           { { { .code_and_extra = 0x0ba, .length2 = 0x8 } } },
                           { { { .code_and_extra = 0x07a, .length2 = 0x8 } } },
                           { { { .code_and_extra = 0x0fa, .length2 = 0x8 } } },
                           { { { .code_and_extra = 0x006, .length2 = 0x8 } } },
                           { { { .code_and_extra = 0x086, .length2 = 0x8 } } },
                           { { { .code_and_extra = 0x046, .length2 = 0x8 } } },
                           { { { .code_and_extra = 0x0c6, .length2 = 0x8 } } },
                           { { { .code_and_extra = 0x026, .length2 = 0x8 } } },
                           { { { .code_and_extra = 0x0a6, .length2 = 0x8 } } },
                           { { { .code_and_extra = 0x066, .length2 = 0x8 } } },
                           { { { .code_and_extra = 0x0e6, .length2 = 0x8 } } },
                           { { { .code_and_extra = 0x016, .length2 = 0x8 } } },
                           { { { .code_and_extra = 0x096, .length2 = 0x8 } } },
                           { { { .code_and_extra = 0x056, .length2 = 0x8 } } },
                           { { { .code_and_extra = 0x0d6, .length2 = 0x8 } } },
                           { { { .code_and_extra = 0x036, .length2 = 0x8 } } },
                           { { { .code_and_extra = 0x0b6, .length2 = 0x8 } } },
                           { { { .code_and_extra = 0x076, .length2 = 0x8 } } },
                           { { { .code_and_extra = 0x0f6, .length2 = 0x8 } } },
                           { { { .code_and_extra = 0x00e, .length2 = 0x8 } } },
                           { { { .code_and_extra = 0x08e, .length2 = 0x8 } } },
                           { { { .code_and_extra = 0x04e, .length2 = 0x8 } } },
                           { { { .code_and_extra = 0x0ce, .length2 = 0x8 } } },
                           { { { .code_and_extra = 0x02e, .length2 = 0x8 } } },
                           { { { .code_and_extra = 0x0ae, .length2 = 0x8 } } },
                           { { { .code_and_extra = 0x06e, .length2 = 0x8 } } },
                           { { { .code_and_extra = 0x0ee, .length2 = 0x8 } } },
                           { { { .code_and_extra = 0x01e, .length2 = 0x8 } } },
                           { { { .code_and_extra = 0x09e, .length2 = 0x8 } } },
                           { { { .code_and_extra = 0x05e, .length2 = 0x8 } } },
                           { { { .code_and_extra = 0x0de, .length2 = 0x8 } } },
                           { { { .code_and_extra = 0x03e, .length2 = 0x8 } } },
                           { { { .code_and_extra = 0x0be, .length2 = 0x8 } } },
                           { { { .code_and_extra = 0x07e, .length2 = 0x8 } } },
                           { { { .code_and_extra = 0x0fe, .length2 = 0x8 } } },
                           { { { .code_and_extra = 0x001, .length2 = 0x8 } } },
                           { { { .code_and_extra = 0x081, .length2 = 0x8 } } },
                           { { { .code_and_extra = 0x041, .length2 = 0x8 } } },
                           { { { .code_and_extra = 0x0c1, .length2 = 0x8 } } },
                           { { { .code_and_extra = 0x021, .length2 = 0x8 } } },
                           { { { .code_and_extra = 0x0a1, .length2 = 0x8 } } },
                           { { { .code_and_extra = 0x061, .length2 = 0x8 } } },
                           { { { .code_and_extra = 0x0e1, .length2 = 0x8 } } },
                           { { { .code_and_extra = 0x011, .length2 = 0x8 } } },
                           { { { .code_and_extra = 0x091, .length2 = 0x8 } } },
                           { { { .code_and_extra = 0x051, .length2 = 0x8 } } },
                           { { { .code_and_extra = 0x0d1, .length2 = 0x8 } } },
                           { { { .code_and_extra = 0x031, .length2 = 0x8 } } },
                           { { { .code_and_extra = 0x0b1, .length2 = 0x8 } } },
                           { { { .code_and_extra = 0x071, .length2 = 0x8 } } },
                           { { { .code_and_extra = 0x0f1, .length2 = 0x8 } } },
                           { { { .code_and_extra = 0x009, .length2 = 0x8 } } },
                           { { { .code_and_extra = 0x089, .length2 = 0x8 } } },
                           { { { .code_and_extra = 0x049, .length2 = 0x8 } } },
                           { { { .code_and_extra = 0x0c9, .length2 = 0x8 } } },
                           { { { .code_and_extra = 0x029, .length2 = 0x8 } } },
                           { { { .code_and_extra = 0x0a9, .length2 = 0x8 } } },
                           { { { .code_and_extra = 0x069, .length2 = 0x8 } } },
                           { { { .code_and_extra = 0x0e9, .length2 = 0x8 } } },
                           { { { .code_and_extra = 0x019, .length2 = 0x8 } } },
                           { { { .code_and_extra = 0x099, .length2 = 0x8 } } },
                           { { { .code_and_extra = 0x059, .length2 = 0x8 } } },
                           { { { .code_and_extra = 0x0d9, .length2 = 0x8 } } },
                           { { { .code_and_extra = 0x039, .length2 = 0x8 } } },
                           { { { .code_and_extra = 0x0b9, .length2 = 0x8 } } },
                           { { { .code_and_extra = 0x079, .length2 = 0x8 } } },
                           { { { .code_and_extra = 0x0f9, .length2 = 0x8 } } },
                           { { { .code_and_extra = 0x005, .length2 = 0x8 } } },
                           { { { .code_and_extra = 0x085, .length2 = 0x8 } } },
                           { { { .code_and_extra = 0x045, .length2 = 0x8 } } },
                           { { { .code_and_extra = 0x0c5, .length2 = 0x8 } } },
                           { { { .code_and_extra = 0x025, .length2 = 0x8 } } },
                           { { { .code_and_extra = 0x0a5, .length2 = 0x8 } } },
                           { { { .code_and_extra = 0x065, .length2 = 0x8 } } },
                           { { { .code_and_extra = 0x0e5, .length2 = 0x8 } } },
                           { { { .code_and_extra = 0x015, .length2 = 0x8 } } },
                           { { { .code_and_extra = 0x095, .length2 = 0x8 } } },
                           { { { .code_and_extra = 0x055, .length2 = 0x8 } } },
                           { { { .code_and_extra = 0x0d5, .length2 = 0x8 } } },
                           { { { .code_and_extra = 0x035, .length2 = 0x8 } } },
                           { { { .code_and_extra = 0x0b5, .length2 = 0x8 } } },
                           { { { .code_and_extra = 0x075, .length2 = 0x8 } } },
                           { { { .code_and_extra = 0x0f5, .length2 = 0x8 } } },
                           { { { .code_and_extra = 0x00d, .length2 = 0x8 } } },
                           { { { .code_and_extra = 0x08d, .length2 = 0x8 } } },
                           { { { .code_and_extra = 0x04d, .length2 = 0x8 } } },
                           { { { .code_and_extra = 0x0cd, .length2 = 0x8 } } },
                           { { { .code_and_extra = 0x02d, .length2 = 0x8 } } },
                           { { { .code_and_extra = 0x0ad, .length2 = 0x8 } } },
                           { { { .code_and_extra = 0x06d, .length2 = 0x8 } } },
                           { { { .code_and_extra = 0x0ed, .length2 = 0x8 } } },
                           { { { .code_and_extra = 0x01d, .length2 = 0x8 } } },
                           { { { .code_and_extra = 0x09d, .length2 = 0x8 } } },
                           { { { .code_and_extra = 0x05d, .length2 = 0x8 } } },
                           { { { .code_and_extra = 0x0dd, .length2 = 0x8 } } },
                           { { { .code_and_extra = 0x03d, .length2 = 0x8 } } },
                           { { { .code_and_extra = 0x0bd, .length2 = 0x8 } } },
                           { { { .code_and_extra = 0x07d, .length2 = 0x8 } } },
                           { { { .code_and_extra = 0x0fd, .length2 = 0x8 } } },
                           { { { .code_and_extra = 0x013, .length2 = 0x9 } } },
                           { { { .code_and_extra = 0x113, .length2 = 0x9 } } },
                           { { { .code_and_extra = 0x093, .length2 = 0x9 } } },
                           { { { .code_and_extra = 0x193, .length2 = 0x9 } } },
                           { { { .code_and_extra = 0x053, .length2 = 0x9 } } },
                           { { { .code_and_extra = 0x153, .length2 = 0x9 } } },
                           { { { .code_and_extra = 0x0d3, .length2 = 0x9 } } },
                           { { { .code_and_extra = 0x1d3, .length2 = 0x9 } } },
                           { { { .code_and_extra = 0x033, .length2 = 0x9 } } },
                           { { { .code_and_extra = 0x133, .length2 = 0x9 } } },
                           { { { .code_and_extra = 0x0b3, .length2 = 0x9 } } },
                           { { { .code_and_extra = 0x1b3, .length2 = 0x9 } } },
                           { { { .code_and_extra = 0x073, .length2 = 0x9 } } },
                           { { { .code_and_extra = 0x173, .length2 = 0x9 } } },
                           { { { .code_and_extra = 0x0f3, .length2 = 0x9 } } },
                           { { { .code_and_extra = 0x1f3, .length2 = 0x9 } } },
                           { { { .code_and_extra = 0x00b, .length2 = 0x9 } } },
                           { { { .code_and_extra = 0x10b, .length2 = 0x9 } } },
                           { { { .code_and_extra = 0x08b, .length2 = 0x9 } } },
                           { { { .code_and_extra = 0x18b, .length2 = 0x9 } } },
                           { { { .code_and_extra = 0x04b, .length2 = 0x9 } } },
                           { { { .code_and_extra = 0x14b, .length2 = 0x9 } } },
                           { { { .code_and_extra = 0x0cb, .length2 = 0x9 } } },
                           { { { .code_and_extra = 0x1cb, .length2 = 0x9 } } },
                           { { { .code_and_extra = 0x02b, .length2 = 0x9 } } },
                           { { { .code_and_extra = 0x12b, .length2 = 0x9 } } },
                           { { { .code_and_extra = 0x0ab, .length2 = 0x9 } } },
                           { { { .code_and_extra = 0x1ab, .length2 = 0x9 } } },
                           { { { .code_and_extra = 0x06b, .length2 = 0x9 } } },
                           { { { .code_and_extra = 0x16b, .length2 = 0x9 } } },
                           { { { .code_and_extra = 0x0eb, .length2 = 0x9 } } },
                           { { { .code_and_extra = 0x1eb, .length2 = 0x9 } } },
                           { { { .code_and_extra = 0x01b, .length2 = 0x9 } } },
                           { { { .code_and_extra = 0x11b, .length2 = 0x9 } } },
                           { { { .code_and_extra = 0x09b, .length2 = 0x9 } } },
                           { { { .code_and_extra = 0x19b, .length2 = 0x9 } } },
                           { { { .code_and_extra = 0x05b, .length2 = 0x9 } } },
                           { { { .code_and_extra = 0x15b, .length2 = 0x9 } } },
                           { { { .code_and_extra = 0x0db, .length2 = 0x9 } } },
                           { { { .code_and_extra = 0x1db, .length2 = 0x9 } } },
                           { { { .code_and_extra = 0x03b, .length2 = 0x9 } } },
                           { { { .code_and_extra = 0x13b, .length2 = 0x9 } } },
                           { { { .code_and_extra = 0x0bb, .length2 = 0x9 } } },
                           { { { .code_and_extra = 0x1bb, .length2 = 0x9 } } },
                           { { { .code_and_extra = 0x07b, .length2 = 0x9 } } },
                           { { { .code_and_extra = 0x17b, .length2 = 0x9 } } },
                           { { { .code_and_extra = 0x0fb, .length2 = 0x9 } } },
                           { { { .code_and_extra = 0x1fb, .length2 = 0x9 } } },
                           { { { .code_and_extra = 0x007, .length2 = 0x9 } } },
                           { { { .code_and_extra = 0x107, .length2 = 0x9 } } },
                           { { { .code_and_extra = 0x087, .length2 = 0x9 } } },
                           { { { .code_and_extra = 0x187, .length2 = 0x9 } } },
                           { { { .code_and_extra = 0x047, .length2 = 0x9 } } },
                           { { { .code_and_extra = 0x147, .length2 = 0x9 } } },
                           { { { .code_and_extra = 0x0c7, .length2 = 0x9 } } },
                           { { { .code_and_extra = 0x1c7, .length2 = 0x9 } } },
                           { { { .code_and_extra = 0x027, .length2 = 0x9 } } },
                           { { { .code_and_extra = 0x127, .length2 = 0x9 } } },
                           { { { .code_and_extra = 0x0a7, .length2 = 0x9 } } },
                           { { { .code_and_extra = 0x1a7, .length2 = 0x9 } } },
                           { { { .code_and_extra = 0x067, .length2 = 0x9 } } },
                           { { { .code_and_extra = 0x167, .length2 = 0x9 } } },
                           { { { .code_and_extra = 0x0e7, .length2 = 0x9 } } },
                           { { { .code_and_extra = 0x1e7, .length2 = 0x9 } } },
                           { { { .code_and_extra = 0x017, .length2 = 0x9 } } },
                           { { { .code_and_extra = 0x117, .length2 = 0x9 } } },
                           { { { .code_and_extra = 0x097, .length2 = 0x9 } } },
                           { { { .code_and_extra = 0x197, .length2 = 0x9 } } },
                           { { { .code_and_extra = 0x057, .length2 = 0x9 } } },
                           { { { .code_and_extra = 0x157, .length2 = 0x9 } } },
                           { { { .code_and_extra = 0x0d7, .length2 = 0x9 } } },
                           { { { .code_and_extra = 0x1d7, .length2 = 0x9 } } },
                           { { { .code_and_extra = 0x037, .length2 = 0x9 } } },
                           { { { .code_and_extra = 0x137, .length2 = 0x9 } } },
                           { { { .code_and_extra = 0x0b7, .length2 = 0x9 } } },
                           { { { .code_and_extra = 0x1b7, .length2 = 0x9 } } },
                           { { { .code_and_extra = 0x077, .length2 = 0x9 } } },
                           { { { .code_and_extra = 0x177, .length2 = 0x9 } } },
                           { { { .code_and_extra = 0x0f7, .length2 = 0x9 } } },
                           { { { .code_and_extra = 0x1f7, .length2 = 0x9 } } },
                           { { { .code_and_extra = 0x00f, .length2 = 0x9 } } },
                           { { { .code_and_extra = 0x10f, .length2 = 0x9 } } },
                           { { { .code_and_extra = 0x08f, .length2 = 0x9 } } },
                           { { { .code_and_extra = 0x18f, .length2 = 0x9 } } },
                           { { { .code_and_extra = 0x04f, .length2 = 0x9 } } },
                           { { { .code_and_extra = 0x14f, .length2 = 0x9 } } },
                           { { { .code_and_extra = 0x0cf, .length2 = 0x9 } } },
                           { { { .code_and_extra = 0x1cf, .length2 = 0x9 } } },
                           { { { .code_and_extra = 0x02f, .length2 = 0x9 } } },
                           { { { .code_and_extra = 0x12f, .length2 = 0x9 } } },
                           { { { .code_and_extra = 0x0af, .length2 = 0x9 } } },
                           { { { .code_and_extra = 0x1af, .length2 = 0x9 } } },
                           { { { .code_and_extra = 0x06f, .length2 = 0x9 } } },
                           { { { .code_and_extra = 0x16f, .length2 = 0x9 } } },
                           { { { .code_and_extra = 0x0ef, .length2 = 0x9 } } },
                           { { { .code_and_extra = 0x1ef, .length2 = 0x9 } } },
                           { { { .code_and_extra = 0x01f, .length2 = 0x9 } } },
                           { { { .code_and_extra = 0x11f, .length2 = 0x9 } } },
                           { { { .code_and_extra = 0x09f, .length2 = 0x9 } } },
                           { { { .code_and_extra = 0x19f, .length2 = 0x9 } } },
                           { { { .code_and_extra = 0x05f, .length2 = 0x9 } } },
                           { { { .code_and_extra = 0x15f, .length2 = 0x9 } } },
                           { { { .code_and_extra = 0x0df, .length2 = 0x9 } } },
                           { { { .code_and_extra = 0x1df, .length2 = 0x9 } } },
                           { { { .code_and_extra = 0x03f, .length2 = 0x9 } } },
                           { { { .code_and_extra = 0x13f, .length2 = 0x9 } } },
                           { { { .code_and_extra = 0x0bf, .length2 = 0x9 } } },
                           { { { .code_and_extra = 0x1bf, .length2 = 0x9 } } },
                           { { { .code_and_extra = 0x07f, .length2 = 0x9 } } },
                           { { { .code_and_extra = 0x17f, .length2 = 0x9 } } },
                           { { { .code_and_extra = 0x0ff, .length2 = 0x9 } } },
                           { { { .code_and_extra = 0x1ff, .length2 = 0x9 } } },
                           { { { .code_and_extra = 0x000, .length2 = 0x7 } } },
                           { { { .code_and_extra = 0x040, .length2 = 0x7 } } },
                           { { { .code_and_extra = 0x020, .length2 = 0x7 } } },
                           { { { .code_and_extra = 0x060, .length2 = 0x7 } } },
                           { { { .code_and_extra = 0x010, .length2 = 0x7 } } },
                           { { { .code_and_extra = 0x050, .length2 = 0x7 } } },
                           { { { .code_and_extra = 0x030, .length2 = 0x7 } } },
                           { { { .code_and_extra = 0x070, .length2 = 0x7 } } },
                           { { { .code_and_extra = 0x008, .length2 = 0x7 } } },
                           { { { .code_and_extra = 0x048, .length2 = 0x7 } } },
                           { { { .code_and_extra = 0x028, .length2 = 0x7 } } },
                           { { { .code_and_extra = 0x068, .length2 = 0x7 } } },
                           { { { .code_and_extra = 0x018, .length2 = 0x7 } } },
                           { { { .code_and_extra = 0x058, .length2 = 0x7 } } },
                           { { { .code_and_extra = 0x038, .length2 = 0x7 } } },
                           { { { .code_and_extra = 0x078, .length2 = 0x7 } } },
                           { { { .code_and_extra = 0x004, .length2 = 0x7 } } },
                           { { { .code_and_extra = 0x044, .length2 = 0x7 } } },
                           { { { .code_and_extra = 0x024, .length2 = 0x7 } } },
                           { { { .code_and_extra = 0x064, .length2 = 0x7 } } },
                           { { { .code_and_extra = 0x014, .length2 = 0x7 } } },
                           { { { .code_and_extra = 0x054, .length2 = 0x7 } } },
                           { { { .code_and_extra = 0x034, .length2 = 0x7 } } },
                           { { { .code_and_extra = 0x074, .length2 = 0x7 } } },
                           { { { .code_and_extra = 0x003, .length2 = 0x8 } } },
                           { { { .code_and_extra = 0x083, .length2 = 0x8 } } },
                           { { { .code_and_extra = 0x043, .length2 = 0x8 } } },
                           { { { .code_and_extra = 0x0c3, .length2 = 0x8 } } },
                           { { { .code_and_extra = 0x023, .length2 = 0x8 } } },
                           { { { .code_and_extra = 0x0a3, .length2 = 0x8 } } },
                           { { { .code_and_extra = 0x063, .length2 = 0x8 } } },
                           { { { .code_and_extra = 0x0e3, .length2 = 0x8 } } },
                           { { { .code_and_extra = 0x000, .length2 = 0x0 } } },
                           { { { .code_and_extra = 0x000, .length2 = 0x0 } } },
                           { { { .code_and_extra = 0x000, .length2 = 0x0 } } },
                           { { { .code_and_extra = 0x000, .length2 = 0x0 } } },
                           { { { .code_and_extra = 0x000, .length2 = 0x0 } } },
                           { { { .code_and_extra = 0x000, .length2 = 0x0 } } },
                           { { { .code_and_extra = 0x000, .length2 = 0x0 } } },
                           { { { .code_and_extra = 0x000, .length2 = 0x0 } } },
                           { { { .code_and_extra = 0x000, .length2 = 0x0 } } },
                           { { { .code_and_extra = 0x000, .length2 = 0x0 } } },
                           { { { .code_and_extra = 0x000, .length2 = 0x0 } } },
                           { { { .code_and_extra = 0x000, .length2 = 0x0 } } },
                           { { { .code_and_extra = 0x000, .length2 = 0x0 } } },
                           { { { .code_and_extra = 0x000, .length2 = 0x0 } } },
                           { { { .code_and_extra = 0x000, .length2 = 0x0 } } },
                           { { { .code_and_extra = 0x000, .length2 = 0x0 } } },
                           { { { .code_and_extra = 0x000, .length2 = 0x0 } } },
                           { { { .code_and_extra = 0x000, .length2 = 0x0 } } },
                           { { { .code_and_extra = 0x000, .length2 = 0x0 } } },
                           { { { .code_and_extra = 0x000, .length2 = 0x0 } } },
                           { { { .code_and_extra = 0x000, .length2 = 0x0 } } },
                           { { { .code_and_extra = 0x000, .length2 = 0x0 } } },
                           { { { .code_and_extra = 0x000, .length2 = 0x0 } } },
                           { { { .code_and_extra = 0x000, .length2 = 0x0 } } },
                           { { { .code_and_extra = 0x000, .length2 = 0x0 } } },
                           { { { .code_and_extra = 0x000, .length2 = 0x0 } } },
                           { { { .code_and_extra = 0x000, .length2 = 0x0 } } },
                           { { { .code_and_extra = 0x000, .length2 = 0x0 } } },
                           { { { .code_and_extra = 0x000, .length2 = 0x0 } } },
                           { { { .code_and_extra = 0x000, .length2 = 0x0 } } },
                           { { { .code_and_extra = 0x000, .length2 = 0x0 } } },
                           { { { .code_and_extra = 0x000, .length2 = 0x0 } } },
                           { { { .code_and_extra = 0x000, .length2 = 0x0 } } },
                           { { { .code_and_extra = 0x000, .length2 = 0x0 } } },
                           { { { .code_and_extra = 0x000, .length2 = 0x0 } } },
                           { { { .code_and_extra = 0x000, .length2 = 0x0 } } },
                           { { { .code_and_extra = 0x000, .length2 = 0x0 } } },
                           { { { .code_and_extra = 0x000, .length2 = 0x0 } } },
                           { { { .code_and_extra = 0x000, .length2 = 0x0 } } },
                           { { { .code_and_extra = 0x000, .length2 = 0x0 } } },
                           { { { .code_and_extra = 0x000, .length2 = 0x0 } } },
                           { { { .code_and_extra = 0x000, .length2 = 0x0 } } },
                           { { { .code_and_extra = 0x000, .length2 = 0x0 } } },
                           { { { .code_and_extra = 0x000, .length2 = 0x0 } } },
                           { { { .code_and_extra = 0x000, .length2 = 0x0 } } },
                           { { { .code_and_extra = 0x000, .length2 = 0x0 } } },
                           { { { .code_and_extra = 0x000, .length2 = 0x0 } } },
                           { { { .code_and_extra = 0x000, .length2 = 0x0 } } },
                           { { { .code_and_extra = 0x000, .length2 = 0x0 } } },
                           { { { .code_and_extra = 0x000, .length2 = 0x0 } } },
                           { { { .code_and_extra = 0x000, .length2 = 0x0 } } },
                           { { { .code_and_extra = 0x000, .length2 = 0x0 } } },
                           { { { .code_and_extra = 0x000, .length2 = 0x0 } } },
                           { { { .code_and_extra = 0x000, .length2 = 0x0 } } },
                           { { { .code_and_extra = 0x000, .length2 = 0x0 } } },
                           { { { .code_and_extra = 0x000, .length2 = 0x0 } } },
                           { { { .code_and_extra = 0x000, .length2 = 0x0 } } },
                           { { { .code_and_extra = 0x000, .length2 = 0x0 } } },
                           { { { .code_and_extra = 0x000, .length2 = 0x0 } } },
                           { { { .code_and_extra = 0x000, .length2 = 0x0 } } },
                           { { { .code_and_extra = 0x000, .length2 = 0x0 } } },
                           { { { .code_and_extra = 0x000, .length2 = 0x0 } } },
                           { { { .code_and_extra = 0x000, .length2 = 0x0 } } },
                           { { { .code_and_extra = 0x000, .length2 = 0x0 } } },
                           { { { .code_and_extra = 0x000, .length2 = 0x0 } } },
                           { { { .code_and_extra = 0x000, .length2 = 0x0 } } },
                           { { { .code_and_extra = 0x000, .length2 = 0x0 } } },
                           { { { .code_and_extra = 0x000, .length2 = 0x0 } } },
                           { { { .code_and_extra = 0x000, .length2 = 0x0 } } },
                           { { { .code_and_extra = 0x000, .length2 = 0x0 } } },
                           { { { .code_and_extra = 0x000, .length2 = 0x0 } } },
                           { { { .code_and_extra = 0x000, .length2 = 0x0 } } },
                           { { { .code_and_extra = 0x000, .length2 = 0x0 } } },
                           { { { .code_and_extra = 0x000, .length2 = 0x0 } } },
                           { { { .code_and_extra = 0x000, .length2 = 0x0 } } },
                           { { { .code_and_extra = 0x000, .length2 = 0x0 } } },
                           { { { .code_and_extra = 0x000, .length2 = 0x0 } } },
                           { { { .code_and_extra = 0x000, .length2 = 0x0 } } },
                           { { { .code_and_extra = 0x000, .length2 = 0x0 } } },
                           { { { .code_and_extra = 0x000, .length2 = 0x0 } } },
                           { { { .code_and_extra = 0x000, .length2 = 0x0 } } },
                           { { { .code_and_extra = 0x000, .length2 = 0x0 } } },
                           { { { .code_and_extra = 0x000, .length2 = 0x0 } } },
                           { { { .code_and_extra = 0x000, .length2 = 0x0 } } },
                           { { { .code_and_extra = 0x000, .length2 = 0x0 } } },
                           { { { .code_and_extra = 0x000, .length2 = 0x0 } } },
                           { { { .code_and_extra = 0x000, .length2 = 0x0 } } },
                           { { { .code_and_extra = 0x000, .length2 = 0x0 } } },
                           { { { .code_and_extra = 0x000, .length2 = 0x0 } } },
                           { { { .code_and_extra = 0x000, .length2 = 0x0 } } },
                           { { { .code_and_extra = 0x000, .length2 = 0x0 } } },
                           { { { .code_and_extra = 0x000, .length2 = 0x0 } } },
                           { { { .code_and_extra = 0x000, .length2 = 0x0 } } },
                           { { { .code_and_extra = 0x000, .length2 = 0x0 } } },
                           { { { .code_and_extra = 0x000, .length2 = 0x0 } } },
                           { { { .code_and_extra = 0x000, .length2 = 0x0 } } },
                           { { { .code_and_extra = 0x000, .length2 = 0x0 } } },
                           { { { .code_and_extra = 0x000, .length2 = 0x0 } } },
                           { { { .code_and_extra = 0x000, .length2 = 0x0 } } },
                           { { { .code_and_extra = 0x000, .length2 = 0x0 } } },
                           { { { .code_and_extra = 0x000, .length2 = 0x0 } } },
                           { { { .code_and_extra = 0x000, .length2 = 0x0 } } },
                           { { { .code_and_extra = 0x000, .length2 = 0x0 } } },
                           { { { .code_and_extra = 0x000, .length2 = 0x0 } } },
                           { { { .code_and_extra = 0x000, .length2 = 0x0 } } },
                           { { { .code_and_extra = 0x000, .length2 = 0x0 } } },
                           { { { .code_and_extra = 0x000, .length2 = 0x0 } } },
                           { { { .code_and_extra = 0x000, .length2 = 0x0 } } },
                           { { { .code_and_extra = 0x000, .length2 = 0x0 } } },
                           { { { .code_and_extra = 0x000, .length2 = 0x0 } } },
                           { { { .code_and_extra = 0x000, .length2 = 0x0 } } },
                           { { { .code_and_extra = 0x000, .length2 = 0x0 } } },
                           { { { .code_and_extra = 0x000, .length2 = 0x0 } } },
                           { { { .code_and_extra = 0x000, .length2 = 0x0 } } },
                           { { { .code_and_extra = 0x000, .length2 = 0x0 } } },
                           { { { .code_and_extra = 0x000, .length2 = 0x0 } } },
                           { { { .code_and_extra = 0x000, .length2 = 0x0 } } },
                           { { { .code_and_extra = 0x000, .length2 = 0x0 } } },
                           { { { .code_and_extra = 0x000, .length2 = 0x0 } } },
                           { { { .code_and_extra = 0x000, .length2 = 0x0 } } },
                           { { { .code_and_extra = 0x000, .length2 = 0x0 } } },
                           { { { .code_and_extra = 0x000, .length2 = 0x0 } } },
                           { { { .code_and_extra = 0x000, .length2 = 0x0 } } },
                           { { { .code_and_extra = 0x000, .length2 = 0x0 } } },
                           { { { .code_and_extra = 0x000, .length2 = 0x0 } } },
                           { { { .code_and_extra = 0x000, .length2 = 0x0 } } },
                           { { { .code_and_extra = 0x000, .length2 = 0x0 } } },
                           { { { .code_and_extra = 0x000, .length2 = 0x0 } } },
                           { { { .code_and_extra = 0x000, .length2 = 0x0 } } },
                           { { { .code_and_extra = 0x000, .length2 = 0x0 } } },
                           { { { .code_and_extra = 0x000, .length2 = 0x0 } } },
                           { { { .code_and_extra = 0x000, .length2 = 0x0 } } },
                           { { { .code_and_extra = 0x000, .length2 = 0x0 } } },
                           { { { .code_and_extra = 0x000, .length2 = 0x0 } } },
                           { { { .code_and_extra = 0x000, .length2 = 0x0 } } },
                           { { { .code_and_extra = 0x000, .length2 = 0x0 } } },
                           { { { .code_and_extra = 0x000, .length2 = 0x0 } } },
                           { { { .code_and_extra = 0x000, .length2 = 0x0 } } },
                           { { { .code_and_extra = 0x000, .length2 = 0x0 } } },
                           { { { .code_and_extra = 0x000, .length2 = 0x0 } } },
                           { { { .code_and_extra = 0x000, .length2 = 0x0 } } },
                           { { { .code_and_extra = 0x000, .length2 = 0x0 } } },
                           { { { .code_and_extra = 0x000, .length2 = 0x0 } } },
                           { { { .code_and_extra = 0x000, .length2 = 0x0 } } },
                           { { { .code_and_extra = 0x000, .length2 = 0x0 } } },
                           { { { .code_and_extra = 0x000, .length2 = 0x0 } } },
                           { { { .code_and_extra = 0x000, .length2 = 0x0 } } },
                           { { { .code_and_extra = 0x000, .length2 = 0x0 } } },
                           { { { .code_and_extra = 0x000, .length2 = 0x0 } } },
                           { { { .code_and_extra = 0x000, .length2 = 0x0 } } },
                           { { { .code_and_extra = 0x000, .length2 = 0x0 } } },
                           { { { .code_and_extra = 0x000, .length2 = 0x0 } } },
                           { { { .code_and_extra = 0x000, .length2 = 0x0 } } },
                           { { { .code_and_extra = 0x000, .length2 = 0x0 } } },
                           { { { .code_and_extra = 0x000, .length2 = 0x0 } } },
                           { { { .code_and_extra = 0x000, .length2 = 0x0 } } },
                           { { { .code_and_extra = 0x000, .length2 = 0x0 } } },
                           { { { .code_and_extra = 0x000, .length2 = 0x0 } } },
                           { { { .code_and_extra = 0x000, .length2 = 0x0 } } },
                           { { { .code_and_extra = 0x000, .length2 = 0x0 } } },
                           { { { .code_and_extra = 0x000, .length2 = 0x0 } } },
                           { { { .code_and_extra = 0x000, .length2 = 0x0 } } },
                           { { { .code_and_extra = 0x000, .length2 = 0x0 } } },
                           { { { .code_and_extra = 0x000, .length2 = 0x0 } } },
                           { { { .code_and_extra = 0x000, .length2 = 0x0 } } },
                           { { { .code_and_extra = 0x000, .length2 = 0x0 } } },
                           { { { .code_and_extra = 0x000, .length2 = 0x0 } } },
                           { { { .code_and_extra = 0x000, .length2 = 0x0 } } },
                           { { { .code_and_extra = 0x000, .length2 = 0x0 } } },
                           { { { .code_and_extra = 0x000, .length2 = 0x0 } } },
                           { { { .code_and_extra = 0x000, .length2 = 0x0 } } },
                           { { { .code_and_extra = 0x000, .length2 = 0x0 } } },
                           { { { .code_and_extra = 0x000, .length2 = 0x0 } } },
                           { { { .code_and_extra = 0x000, .length2 = 0x0 } } },
                           { { { .code_and_extra = 0x000, .length2 = 0x0 } } },
                           { { { .code_and_extra = 0x000, .length2 = 0x0 } } },
                           { { { .code_and_extra = 0x000, .length2 = 0x0 } } },
                           { { { .code_and_extra = 0x000, .length2 = 0x0 } } },
                           { { { .code_and_extra = 0x000, .length2 = 0x0 } } },
                           { { { .code_and_extra = 0x000, .length2 = 0x0 } } },
                           { { { .code_and_extra = 0x000, .length2 = 0x0 } } },
                           { { { .code_and_extra = 0x000, .length2 = 0x0 } } },
                           { { { .code_and_extra = 0x000, .length2 = 0x0 } } },
                           { { { .code_and_extra = 0x000, .length2 = 0x0 } } },
                           { { { .code_and_extra = 0x000, .length2 = 0x0 } } },
                           { { { .code_and_extra = 0x000, .length2 = 0x0 } } },
                           { { { .code_and_extra = 0x000, .length2 = 0x0 } } },
                           { { { .code_and_extra = 0x000, .length2 = 0x0 } } },
                           { { { .code_and_extra = 0x000, .length2 = 0x0 } } },
                           { { { .code_and_extra = 0x000, .length2 = 0x0 } } },
                           { { { .code_and_extra = 0x000, .length2 = 0x0 } } },
                           { { { .code_and_extra = 0x000, .length2 = 0x0 } } },
                           { { { .code_and_extra = 0x000, .length2 = 0x0 } } },
                           { { { .code_and_extra = 0x000, .length2 = 0x0 } } },
                           { { { .code_and_extra = 0x000, .length2 = 0x0 } } },
                           { { { .code_and_extra = 0x000, .length2 = 0x0 } } },
                           { { { .code_and_extra = 0x000, .length2 = 0x0 } } },
                           { { { .code_and_extra = 0x000, .length2 = 0x0 } } },
                           { { { .code_and_extra = 0x000, .length2 = 0x0 } } },
                           { { { .code_and_extra = 0x000, .length2 = 0x0 } } },
                           { { { .code_and_extra = 0x000, .length2 = 0x0 } } },
                           { { { .code_and_extra = 0x000, .length2 = 0x0 } } },
                           { { { .code_and_extra = 0x000, .length2 = 0x0 } } },
                           { { { .code_and_extra = 0x000, .length2 = 0x0 } } },
                           { { { .code_and_extra = 0x000, .length2 = 0x0 } } },
                           { { { .code_and_extra = 0x000, .length2 = 0x0 } } },
                           { { { .code_and_extra = 0x000, .length2 = 0x0 } } },
                           { { { .code_and_extra = 0x000, .length2 = 0x0 } } },
                           { { { .code_and_extra = 0x000, .length2 = 0x0 } } },
                           { { { .code_and_extra = 0x000, .length2 = 0x0 } } },
                           { { { .code_and_extra = 0x000, .length2 = 0x0 } } },
                           { { { .code_and_extra = 0x000, .length2 = 0x0 } } },
                           { { { .code_and_extra = 0x000, .length2 = 0x0 } } },
                           { { { .code_and_extra = 0x000, .length2 = 0x0 } } },
                           { { { .code_and_extra = 0x000, .length2 = 0x0 } } },
                           { { { .code_and_extra = 0x000, .length2 = 0x0 } } },
                           { { { .code_and_extra = 0x000, .length2 = 0x0 } } },
                           { { { .code_and_extra = 0x000, .length2 = 0x0 } } },
                           { { { .code_and_extra = 0x000, .length2 = 0x0 } } },
                           { { { .code_and_extra = 0x000, .length2 = 0x0 } } },
                           { { { .code_and_extra = 0x000, .length2 = 0x0 } } },
                           { { { .code_and_extra = 0x000, .length2 = 0x0 } } },
                           { { { .code_and_extra = 0x000, .length2 = 0x0 } } },
                           { { { .code_and_extra = 0x000, .length2 = 0x0 } } },
                           { { { .code_and_extra = 0x000, .length2 = 0x0 } } } },
        .dist_table = { { { { .code_and_extra = 0x000, .length2 = 0x5 } } },
                        { { { .code_and_extra = 0x010, .length2 = 0x5 } } },
                        { { { .code_and_extra = 0x008, .length2 = 0x5 } } },
                        { { { .code_and_extra = 0x018, .length2 = 0x5 } } },
                        { { { .code_and_extra = 0x10004, .length2 = 0x5 } } },
                        { { { .code_and_extra = 0x10014, .length2 = 0x5 } } },
                        { { { .code_and_extra = 0x2000c, .length2 = 0x5 } } },
                        { { { .code_and_extra = 0x2001c, .length2 = 0x5 } } },
                        { { { .code_and_extra = 0x30002, .length2 = 0x5 } } },
                        { { { .code_and_extra = 0x30012, .length2 = 0x5 } } },
                        { { { .code_and_extra = 0x4000a, .length2 = 0x5 } } },
                        { { { .code_and_extra = 0x4001a, .length2 = 0x5 } } },
                        { { { .code_and_extra = 0x50006, .length2 = 0x5 } } },
                        { { { .code_and_extra = 0x50016, .length2 = 0x5 } } },
                        { { { .code_and_extra = 0x6000e, .length2 = 0x5 } } },
                        { { { .code_and_extra = 0x6001e, .length2 = 0x5 } } },
                        { { { .code_and_extra = 0x70001, .length2 = 0x5 } } },
                        { { { .code_and_extra = 0x70011, .length2 = 0x5 } } },
                        { { { .code_and_extra = 0x80009, .length2 = 0x5 } } },
                        { { { .code_and_extra = 0x80019, .length2 = 0x5 } } },
                        { { { .code_and_extra = 0x90005, .length2 = 0x5 } } },
                        { { { .code_and_extra = 0x90015, .length2 = 0x5 } } },
                        { { { .code_and_extra = 0xa000d, .length2 = 0x5 } } },
                        { { { .code_and_extra = 0xa001d, .length2 = 0x5 } } },
                        { { { .code_and_extra = 0xb0003, .length2 = 0x5 } } },
                        { { { .code_and_extra = 0xb0013, .length2 = 0x5 } } },
                        { { { .code_and_extra = 0xc000b, .length2 = 0x5 } } },
                        { { { .code_and_extra = 0xc001b, .length2 = 0x5 } } },
                        { { { .code_and_extra = 0xd0007, .length2 = 0x5 } } },
                        { { { .code_and_extra = 0xd0017, .length2 = 0x5 } } },
                        { { { .code_and_extra = 0x000, .length2 = 0x0 } } } }
};

extern uint32_t
build_huff_tree(struct heap_tree *heap, uint64_t heap_size, uint64_t node_ptr);

static uint32_t
convert_dist_to_dist_sym(uint32_t dist);
static uint32_t
convert_length_to_len_sym(uint32_t length);

static const uint8_t bitrev8[0x100] = {
        0x00, 0x80, 0x40, 0xC0, 0x20, 0xA0, 0x60, 0xE0, 0x10, 0x90, 0x50, 0xD0, 0x30, 0xB0, 0x70,
        0xF0, 0x08, 0x88, 0x48, 0xC8, 0x28, 0xA8, 0x68, 0xE8, 0x18, 0x98, 0x58, 0xD8, 0x38, 0xB8,
        0x78, 0xF8, 0x04, 0x84, 0x44, 0xC4, 0x24, 0xA4, 0x64, 0xE4, 0x14, 0x94, 0x54, 0xD4, 0x34,
        0xB4, 0x74, 0xF4, 0x0C, 0x8C, 0x4C, 0xCC, 0x2C, 0xAC, 0x6C, 0xEC, 0x1C, 0x9C, 0x5C, 0xDC,
        0x3C, 0xBC, 0x7C, 0xFC, 0x02, 0x82, 0x42, 0xC2, 0x22, 0xA2, 0x62, 0xE2, 0x12, 0x92, 0x52,
        0xD2, 0x32, 0xB2, 0x72, 0xF2, 0x0A, 0x8A, 0x4A, 0xCA, 0x2A, 0xAA, 0x6A, 0xEA, 0x1A, 0x9A,
        0x5A, 0xDA, 0x3A, 0xBA, 0x7A, 0xFA, 0x06, 0x86, 0x46, 0xC6, 0x26, 0xA6, 0x66, 0xE6, 0x16,
        0x96, 0x56, 0xD6, 0x36, 0xB6, 0x76, 0xF6, 0x0E, 0x8E, 0x4E, 0xCE, 0x2E, 0xAE, 0x6E, 0xEE,
        0x1E, 0x9E, 0x5E, 0xDE, 0x3E, 0xBE, 0x7E, 0xFE, 0x01, 0x81, 0x41, 0xC1, 0x21, 0xA1, 0x61,
        0xE1, 0x11, 0x91, 0x51, 0xD1, 0x31, 0xB1, 0x71, 0xF1, 0x09, 0x89, 0x49, 0xC9, 0x29, 0xA9,
        0x69, 0xE9, 0x19, 0x99, 0x59, 0xD9, 0x39, 0xB9, 0x79, 0xF9, 0x05, 0x85, 0x45, 0xC5, 0x25,
        0xA5, 0x65, 0xE5, 0x15, 0x95, 0x55, 0xD5, 0x35, 0xB5, 0x75, 0xF5, 0x0D, 0x8D, 0x4D, 0xCD,
        0x2D, 0xAD, 0x6D, 0xED, 0x1D, 0x9D, 0x5D, 0xDD, 0x3D, 0xBD, 0x7D, 0xFD, 0x03, 0x83, 0x43,
        0xC3, 0x23, 0xA3, 0x63, 0xE3, 0x13, 0x93, 0x53, 0xD3, 0x33, 0xB3, 0x73, 0xF3, 0x0B, 0x8B,
        0x4B, 0xCB, 0x2B, 0xAB, 0x6B, 0xEB, 0x1B, 0x9B, 0x5B, 0xDB, 0x3B, 0xBB, 0x7B, 0xFB, 0x07,
        0x87, 0x47, 0xC7, 0x27, 0xA7, 0x67, 0xE7, 0x17, 0x97, 0x57, 0xD7, 0x37, 0xB7, 0x77, 0xF7,
        0x0F, 0x8F, 0x4F, 0xCF, 0x2F, 0xAF, 0x6F, 0xEF, 0x1F, 0x9F, 0x5F, 0xDF, 0x3F, 0xBF, 0x7F,
        0xFF
};

// bit reverse low order LENGTH bits in code, and return result in low order bits
static inline uint16_t
bit_reverse(uint16_t code, uint32_t length)
{
        code = (bitrev8[code & 0x00FF] << 8) | (bitrev8[code >> 8]);
        return (code >> (16 - length));
}

void
isal_update_histogram_base(uint8_t *start_stream, int length, struct isal_huff_histogram *histogram)
{
        uint32_t literal = 0, hash;
        uint16_t seen, *last_seen = histogram->hash_table;
        uint8_t *current, *end_stream, *next_hash, *end;
        uint32_t match_length;
        uint32_t dist;
        uint64_t *lit_len_histogram = histogram->lit_len_histogram;
        uint64_t *dist_histogram = histogram->dist_histogram;

        if (length <= 0)
                return;

        end_stream = start_stream + length;
        memset(last_seen, 0, sizeof(histogram->hash_table)); /* Initialize last_seen to be 0. */
        for (current = start_stream; current < end_stream - 3; current++) {
                literal = load_le_u32(current);
                hash = compute_hash(literal) & LVL0_HASH_MASK;
                seen = last_seen[hash];
                last_seen[hash] = (current - start_stream) & 0xFFFF;
                dist = (current - start_stream - seen) & 0xFFFF;
                if (dist - 1 < D - 1) {
                        assert(start_stream <= current - dist);
                        match_length = compare258(current - dist, current,
                                                  (uint32_t) (end_stream - current));
                        if (match_length >= SHORTEST_MATCH) {
                                next_hash = current;
#ifdef ISAL_LIMIT_HASH_UPDATE
                                end = next_hash + 3;
#else
                                end = next_hash + match_length;
#endif
                                if (end > end_stream - 3)
                                        end = end_stream - 3;
                                next_hash++;
                                for (; next_hash < end; next_hash++) {
                                        literal = load_le_u32(next_hash);
                                        hash = compute_hash(literal) & LVL0_HASH_MASK;
                                        last_seen[hash] = (next_hash - start_stream) & 0xFFFF;
                                }

                                dist_histogram[convert_dist_to_dist_sym(dist)] += 1;
                                lit_len_histogram[convert_length_to_len_sym(match_length)] += 1;
                                current += match_length - 1;
                                continue;
                        }
                }
                lit_len_histogram[literal & 0xFF] += 1;
        }

        for (; current < end_stream; current++)
                lit_len_histogram[*current] += 1;

        lit_len_histogram[256] += 1;
        return;
}

/**
 * @brief  Returns the deflate symbol value for a look back distance.
 */
static uint32_t
convert_dist_to_dist_sym(uint32_t dist)
{
        assert(dist <= 32768 && dist > 0);
        if (dist <= 32768) {
                uint32_t msb = dist > 4 ? bsr(dist - 1) - 2 : 0;
                return (msb * 2) + ((dist - 1) >> msb);
        } else {
                return ~0;
        }
}

/**
 * @brief  Returns the deflate symbol value for a repeat length.
 */
static uint32_t
convert_length_to_len_sym(uint32_t length)
{
        assert(length > 2 && length < 259);

        /* Based on tables on page 11 in RFC 1951 */
        if (length < 11)
                return 257 + length - 3;
        else if (length < 19)
                return 261 + (length - 3) / 2;
        else if (length < 35)
                return 265 + (length - 3) / 4;
        else if (length < 67)
                return 269 + (length - 3) / 8;
        else if (length < 131)
                return 273 + (length - 3) / 16;
        else if (length < 258)
                return 277 + (length - 3) / 32;
        else
                return 285;
}

/* Init heap with the histogram, and return the histogram size */
static inline uint32_t
init_heap64_semi_complete(struct heap_tree *heap_space, uint64_t *histogram, uint64_t hist_size,
                          uint64_t complete_start)
{
        uint32_t heap_size, i;

        heap_size = 0;
        for (i = 0; i < complete_start; i++) {
                heap_space->heap[heap_size + 1] = ((histogram[i]) << FREQ_SHIFT) | i;
                heap_size += histogram[i] != 0;
        }

        for (; i < hist_size; i++)
                heap_space->heap[++heap_size] = ((histogram[i]) << FREQ_SHIFT) | i;

        // make sure heap has at least two elements in it
        if (heap_size < 2) {
                if (heap_size == 0) {
                        heap_space->heap[1] = 1ULL << FREQ_SHIFT;
                        heap_space->heap[2] = (1ULL << FREQ_SHIFT) | 1;
                        heap_size = 2;
                } else {
                        // heap size == 1
                        if (histogram[0] == 0)
                                heap_space->heap[2] = 1ULL << FREQ_SHIFT;
                        else
                                heap_space->heap[2] = (1ULL << FREQ_SHIFT) | 1;
                        heap_size = 2;
                }
        }

        return heap_size;
}

static inline uint32_t
init_heap64_complete(struct heap_tree *heap_space, uint64_t *histogram, uint64_t hist_size)
{
        uint32_t heap_size, i;

        heap_size = 0;
        for (i = 0; i < hist_size; i++)
                heap_space->heap[++heap_size] = ((histogram[i]) << FREQ_SHIFT) | i;

        return heap_size;
}

static inline uint32_t
fix_code_lens(struct heap_tree *heap_space, uint32_t root_node, uint32_t *bl_count,
              uint32_t max_code_len)
{
        struct tree_node *tree = heap_space->tree;
        uint32_t *code_len_count = (uint32_t *) heap_space->code_len_count;
        uint32_t i, j, k, child, depth, code_len, leaf;

        // compute code lengths and code length counts
        code_len = 0;
        j = root_node;
        for (i = root_node; i <= HEAP_TREE_NODE_START; i++) {
                /*
                 * Handle internal nodes and leaves without a branch. An internal node sets the
                 * depth of its children, and a leaf sets two unused nodes below root_node. A leaf
                 * moves to the list of leaves at tree[root_node..j) and counts toward its depth,
                 * and an internal node counts toward the unused code_len_count[0].
                 */
                child = tree[i].child;
                depth = tree[i].depth;
                leaf = child <= MAX_HISTHEAP_SIZE;
                k = leaf ? root_node - 1 : child;
                tree[k].depth = depth + 1;
                tree[k - 1].depth = depth + 1;
                tree[j] = tree[i];
                j += leaf;
                depth = leaf ? depth : 0;
                while (code_len < depth) {
                        code_len++;
                        code_len_count[code_len] = 0;
                }
                code_len_count[depth]++;
        }

        if (code_len > max_code_len) {
                while (code_len > max_code_len) {
                        assert(code_len_count[code_len] > 1);
                        for (i = max_code_len - 1; i != 0; i--)
                                if (code_len_count[i] != 0)
                                        break;
                        assert(i != 0);
                        code_len_count[i]--;
                        code_len_count[i + 1] += 2;
                        code_len_count[code_len - 1]++;
                        code_len_count[code_len] -= 2;
                        if (code_len_count[code_len] == 0)
                                code_len--;
                }

                bl_count[0] = 0;
                for (i = 1; i <= code_len; i++)
                        bl_count[i] = code_len_count[i];
                for (; i <= max_code_len; i++)
                        bl_count[i] = 0;

                for (k = 1; code_len_count[k] == 0; k++)
                        ;
                for (i = root_node; i < j; i++) {
                        tree[i].depth = k;
                        code_len_count[k]--;
                        for (; code_len_count[k] == 0; k++)
                                ;
                }
        } else {
                bl_count[0] = 0;
                for (i = 1; i <= code_len; i++)
                        bl_count[i] = code_len_count[i];
                for (; i <= max_code_len; i++)
                        bl_count[i] = 0;
        }

        return j;
}

static inline void
gen_huff_code_lens(struct heap_tree *heap_space, uint32_t heap_size, uint32_t *bl_count,
                   struct huff_code *codes, uint32_t codes_count, uint32_t max_code_len)
{
        struct tree_node *tree = heap_space->tree;
        uint32_t root_node = HEAP_TREE_NODE_START, node_ptr;
        uint32_t end_node;

        root_node = build_huff_tree(heap_space, heap_size, root_node);

        end_node = fix_code_lens(heap_space, root_node, bl_count, max_code_len);

        memset(codes, 0, codes_count * sizeof(*codes));
        for (node_ptr = root_node; node_ptr < end_node; node_ptr++)
                codes[tree[node_ptr].child].length = tree[node_ptr].depth;
}

/**
 * @brief Determines the code each element of a deflate compliant huffman tree and stores
 * it in a lookup table
 * @requires table has been initialized to already contain the code length for each element.
 * @param table: A lookup table used to store the codes.
 * @param table_length: The length of table.
 * @param count: a histogram representing the number of occurrences of codes of a given length
 */
static inline uint32_t
set_huff_codes(struct huff_code *huff_code_table, int table_length, uint32_t *count)
{
        /* Uses the algorithm mentioned in the deflate standard, Rfc 1951. */
        int i;
        uint16_t code = 0;
        uint16_t next_code[MAX_HUFF_TREE_DEPTH + 1];
        uint32_t max_code = 0, len;

        next_code[0] = code;

        for (i = 1; i < MAX_HUFF_TREE_DEPTH + 1; i++)
                next_code[i] = (next_code[i - 1] + count[i - 1]) << 1;

        for (i = 0; i < table_length; i++) {
                /* Unused symbols get the code 0 they already have, and next_code[0] is unused */
                len = huff_code_table[i].length;
                huff_code_table[i].code = bit_reverse(next_code[len], len);
                next_code[len] += 1;
                max_code = len != 0 ? i : max_code;
        }

        return max_code;
}

/* Code lengths of the lit/len and dist codes, followed by room for rl_encode_lens() */
#define CODE_LENS_SIZE (LIT_LEN + DIST_LEN + 8)

/* Number of extra bits of each code length code */
static const uint8_t cl_extra_bits[CODE_LEN_CODES] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                                                       0, 0, 0, 0, 0, 0, 2, 3, 7 };

/*
 * Sets lens[] of the symbols with nonzero count[0..n) to the lengths of a Huffman code for them,
 * limited to max_len, and returns the number of symbols. While fewer than two symbols occur, it
 * adds symbol 0, or symbol 1 if symbol 0 occurs, with a count of 1. Sets syms[] to the symbols
 * in ascending order, and bl_count[0..max_len] to the number of codes of each length.
 *
 * The code lengths are the ones gen_huff_code_lens() gives for these symbols. The tree comes
 * from build_huff_tree(), and the lengths are limited the way fix_code_lens() limits them, which
 * depends on the order it visits the leaves in. heap_space is scratch space for the tree.
 */
static uint32_t
make_code_lens(const uint32_t *count, uint32_t n, uint32_t max_len, uint16_t *syms, uint8_t *lens,
               uint32_t *bl_count, struct heap_tree *heap_space)
{
        uint64_t *heap = heap_space->heap;
        uint8_t depth[HEAP_TREE_SIZE];
        uint32_t leaf[MAX_HISTHEAP_SIZE];
        uint16_t depth_count[MAX_HISTHEAP_SIZE];
        uint32_t i, num, root, child, d, internal, num_leaves, max_depth, code_len, k;

        num = 0;
        for (i = 0; i < n; i++) {
                heap[num + 1] = (uint64_t) count[i] << FREQ_SHIFT | i;
                syms[num] = i;
                num += count[i] != 0;
        }

        if (num < 2) {
                /* Add symbols with a count of 1 */
                if (num == 0) {
                        heap[1] = 1ULL << FREQ_SHIFT;
                        syms[0] = 0;
                }
                if (syms[0] == 0) {
                        heap[2] = (1ULL << FREQ_SHIFT) | 1;
                        syms[1] = 1;
                } else {
                        heap[2] = 1ULL << FREQ_SHIFT;
                        syms[1] = syms[0];
                        syms[0] = 0;
                }
                num = 2;
        }

        root = build_huff_tree(heap_space, num, HEAP_TREE_NODE_START);

        /*
         * Visit the tree in the order fix_code_lens() does. An internal node sets the depth of
         * its children at depth[child] and depth[child - 1], and a leaf sets two unused entries
         * below root instead. The leaves go to leaf[], with their depth above bit 16, in the
         * order they are visited.
         */
        depth[root] = 0;
        max_depth = 0;
        num_leaves = 0;
        for (i = root; i <= HEAP_TREE_NODE_START; i++) {
                child = (uint32_t) heap[i];
                d = depth[i];
                internal = child > MAX_HISTHEAP_SIZE;
                k = internal ? child : root - 1;
                depth[k] = d + 1;
                depth[k - 1] = d + 1;
                leaf[num_leaves] = child | d << 16;
                num_leaves += !internal;
                max_depth = d > max_depth ? d : max_depth;
        }

        memset(depth_count, 0, (max_depth + 1) * sizeof(*depth_count));
        for (i = 0; i < num_leaves; i++)
                depth_count[leaf[i] >> 16]++;

        /* Limit the code lengths to max_len as fix_code_lens() does */
        code_len = max_depth;
        while (code_len > max_len) {
                for (i = max_len - 1; i != 0; i--)
                        if (depth_count[i] != 0)
                                break;
                depth_count[i]--;
                depth_count[i + 1] += 2;
                depth_count[code_len - 1]++;
                depth_count[code_len] -= 2;
                if (depth_count[code_len] == 0)
                        code_len--;
        }

        bl_count[0] = 0;
        for (i = 1; i <= code_len; i++)
                bl_count[i] = depth_count[i];
        for (; i <= max_len; i++)
                bl_count[i] = 0;

        if (max_depth > max_len) {
                /* Give the leaves the limited lengths in the order they were visited */
                for (k = 1; depth_count[k] == 0; k++)
                        ;
                for (i = 0; i < num_leaves; i++) {
                        leaf[i] = (leaf[i] & 0xFFFF) | k << 16;
                        depth_count[k]--;
                        while (depth_count[k] == 0 && k < code_len)
                                k++;
                }
        }

        for (i = 0; i < num_leaves; i++)
                lens[leaf[i] & 0xFFFF] = leaf[i] >> 16;

        return num;
}

/*
 * Sets codes[] of the num symbols in syms[], in ascending order, to their canonical codes and
 * lengths, as set_huff_codes() would. lens[] holds their lengths, and bl_count[] the number of
 * codes of each length up to max_len.
 */
static void
set_codes_by_sym(struct huff_code *codes, const uint16_t *syms, uint32_t num, const uint8_t *lens,
                 const uint32_t *bl_count, uint32_t max_len)
{
        uint32_t next_code[MAX_HUFF_TREE_DEPTH + 1];
        uint32_t i, sym, len;

        next_code[0] = 0;
        next_code[1] = 0;
        for (i = 2; i <= max_len; i++)
                next_code[i] = (next_code[i - 1] + bl_count[i - 1]) << 1;

        for (i = 0; i < num; i++) {
                sym = syms[i];
                len = lens[sym];
                codes[sym].code_and_length = bit_reverse(next_code[len]++, len) | len << 24;
        }
}

/*
 * Run length encodes the code lengths lens[0..n) with the code length codes of RFC 1951, writes
 * the codes and their extra bits to out[], adds the count of each code to counts[], and returns
 * the number of codes. A run of a nonzero length writes the length, then code 16 for each 6
 * repeats while more than 6 are left, then code 16 for the last 3 to 6 repeats or the length for
 * each of the last 1 or 2. A run of zeros writes code 18 for each 138 zeros while more than 138
 * are left, then code 18 for 11 to 138, code 17 for 3 to 10, or a 0 for each of 1 or 2.
 *
 * lens[n] must hold a value that is not a code length, and lens[] must be readable up to
 * lens[n + 7]. out[] needs room for n + 2 codes, because the end of each run writes two codes
 * and then keeps the ones it needs.
 */
static uint32_t
rl_encode_lens(uint8_t *lens, uint32_t n, uint32_t *counts, struct rl_code *out)
{
        struct rl_code *pout = out;
        uint32_t i = 0, len, run_len;
        uint64_t mismatch;

        while (i < n) {
                len = lens[i];
                run_len = 0;
                do {
                        mismatch = load_le_u64(&lens[i + run_len]) ^ (len * 0x0101010101010101ULL);
                        run_len += tzbytecnt(mismatch);
                } while (mismatch == 0);
                i += run_len;

                if (len == 0) {
                        while (run_len > 138) {
                                pout->code = 18;
                                pout->extra_bits = 138 - 11;
                                pout++;
                                run_len -= 138;
                                counts[18]++;
                        }
                        if (run_len > 10) {
                                pout->code = 18;
                                pout->extra_bits = run_len - 11;
                                pout++;
                                counts[18]++;
                        } else if (run_len > 2) {
                                pout->code = 17;
                                pout->extra_bits = run_len - 3;
                                pout++;
                                counts[17]++;
                        } else {
                                pout[0].code = 0;
                                pout[0].extra_bits = 0;
                                pout[1].code = 0;
                                pout[1].extra_bits = 0;
                                pout += run_len;
                                counts[0] += run_len;
                        }
                } else {
                        pout->code = len;
                        pout->extra_bits = 0;
                        pout++;
                        counts[len]++;
                        run_len--;
                        while (run_len > 6) {
                                pout->code = 16;
                                pout->extra_bits = 6 - 3;
                                pout++;
                                run_len -= 6;
                                counts[16]++;
                        }
                        if (run_len > 2) {
                                pout->code = 16;
                                pout->extra_bits = run_len - 3;
                                pout++;
                                counts[16]++;
                        } else {
                                pout[0].code = len;
                                pout[0].extra_bits = 0;
                                pout[1].code = len;
                                pout[1].extra_bits = 0;
                                pout += run_len;
                                counts[len] += run_len;
                        }
                }
        }

        return (uint32_t) (pout - out);
}

/*
 * Writes the dynamic block header with the code lengths run length encoded in
 * cl_tokens[0..num_tokens), using a Huffman code of up to 7 bits for the code length codes,
 * whose counts are in cl_counts[]. Returns the number of bits written. heap_space is scratch
 * space for make_code_lens().
 */
static uint32_t
write_dynamic_header(struct BitBuf2 *bb, const struct rl_code *cl_tokens, uint32_t num_tokens,
                     const uint32_t *cl_counts, uint32_t hlit, uint32_t hdist,
                     uint32_t end_of_block, struct heap_tree *heap_space)
{
        struct huff_code cl_codes[CODE_LEN_CODES];
        uint8_t cl_lens[CODE_LEN_CODES];
        uint16_t syms[CODE_LEN_CODES];
        uint32_t bl_count[MAX_HUFF_TREE_DEPTH + 1];
        uint32_t code_bits[CODE_LEN_CODES], code_len[CODE_LEN_CODES];
        struct BitBuf2 bits = *bb;
        uint32_t i, num, hclen, code, bits_start = buffer_bits_used(bb);
        uint64_t data;

        memset(cl_lens, 0, sizeof(cl_lens));
        memset(cl_codes, 0, sizeof(cl_codes));
        num = make_code_lens(cl_counts, CODE_LEN_CODES, 7, syms, cl_lens, bl_count, heap_space);
        set_codes_by_sym(cl_codes, syms, num, cl_lens, bl_count, 7);

        for (i = CODE_LEN_CODES - 1; i > 3; i--) /* i must be at least 4 */
                if (cl_lens[code_length_code_order[i]] != 0)
                        break;
        hclen = i - 3;

        data = (end_of_block ? 5 : 4) | (hlit << 3) | (hdist << 8) | (hclen << 13);
        data |= (uint64_t) cl_lens[code_length_code_order[0]] << DYN_HDR_START_LEN;
        write_bits(&bits, data, DYN_HDR_START_LEN + 3);
        data = 0;
        for (i = hclen + 3; i >= 1; i--)
                data = (data << 3) | cl_lens[code_length_code_order[i]];
        write_bits(&bits, data, (hclen + 3) * 3);

        /* Write each token's code and extra bits together */
        for (i = 0; i < CODE_LEN_CODES; i++) {
                code_bits[i] = cl_codes[i].code;
                code_len[i] = cl_codes[i].length;
        }
        for (i = 0; i < num_tokens; i++) {
                code = cl_tokens[i].code;
                write_bits(&bits,
                           code_bits[code] | (uint64_t) cl_tokens[i].extra_bits << code_len[code],
                           code_len[code] + cl_extra_bits[code]);
        }

        *bb = bits;
        return buffer_bits_used(bb) - bits_start;
}

/**
 * @brief Creates a two table representation of huffman codes.
 * @param code_table: output table containing the code
 * @param code_size_table: output table containing the code length
 * @param length: the length of hufftable
 * @param hufftable: a huffman lookup table
 */
static void
create_code_tables(uint16_t *code_table, uint8_t *code_length_table, uint32_t length,
                   struct huff_code *hufftable)
{
        uint32_t i;
        for (i = 0; i < length; i++) {
                code_table[i] = hufftable[i].code;
                code_length_table[i] = hufftable[i].length;
        }
}

/**
 * @brief Creates a packed representation of length huffman codes.
 * @details In packed_table, bits 32:8 contain the extra bits appended to the huffman
 * code and bits 8:0 contain the code length.
 * @param packed_table: the output table
 * @param length: the length of lit_len_hufftable
 * @param lit_len_hufftable: a literal/length huffman lookup table
 */
static void
create_packed_len_table(uint32_t *packed_table, struct huff_code *lit_len_hufftable)
{
        int i, count = 0;
        uint16_t extra_bits;
        uint16_t extra_bits_count = 0;

        /* Gain extra bits is the next place where the number of extra bits in
         * length codes increases. */
        uint16_t gain_extra_bits = LEN_EXTRA_BITS_START;

        for (i = 257; i < LIT_LEN - 1; i++) {
                for (extra_bits = 0; extra_bits < (1 << extra_bits_count); extra_bits++) {
                        if (count > 254)
                                break;
                        packed_table[count++] =
                                (extra_bits << (lit_len_hufftable[i].length + LENGTH_BITS)) |
                                (lit_len_hufftable[i].code << LENGTH_BITS) |
                                (lit_len_hufftable[i].length + extra_bits_count);
                }

                if (i == gain_extra_bits) {
                        gain_extra_bits += LEN_EXTRA_BITS_INTERVAL;
                        extra_bits_count += 1;
                }
        }

        packed_table[count] = (lit_len_hufftable[LIT_LEN - 1].code << LENGTH_BITS) |
                              (lit_len_hufftable[LIT_LEN - 1].length);
}

/**
 * @brief Creates a packed representation of distance  huffman codes.
 * @details In packed_table, bits 32:8 contain the extra bits appended to the huffman
 * code and bits 8:0 contain the code length.
 * @param packed_table: the output table
 * @param length: the length of lit_len_hufftable
 * @param dist_hufftable: a distance huffman lookup table
 */
static void
create_packed_dist_table(uint32_t *packed_table, uint32_t length, struct huff_code *dist_hufftable)
{
        uint32_t i, count = 0;
        uint16_t extra_bits;
        uint16_t extra_bits_count = 0;

        /* Gain extra bits is the next place where the number of extra bits in
         * distance codes increases. */
        uint16_t gain_extra_bits = DIST_EXTRA_BITS_START;

        for (i = 0; i < DIST_LEN; i++) {
                for (extra_bits = 0; extra_bits < (1 << extra_bits_count); extra_bits++) {
                        if (count >= length)
                                return;

                        packed_table[count++] =
                                (extra_bits << (dist_hufftable[i].length + LENGTH_BITS)) |
                                (dist_hufftable[i].code << LENGTH_BITS) |
                                (dist_hufftable[i].length + extra_bits_count);
                }

                if (i == gain_extra_bits) {
                        gain_extra_bits += DIST_EXTRA_BITS_INTERVAL;
                        extra_bits_count += 1;
                }
        }
}

/**
 * @brief Checks to see if the hufftable is usable by igzip
 *
 * @param lit_len_hufftable: literal/length huffman code
 * @param dist_hufftable: distance huffman code
 * @returns Returns 0 if the table is usable
 */
static int
are_hufftables_useable(struct huff_code *lit_len_hufftable, struct huff_code *dist_hufftable)
{
        int max_lit_code_len = 0, max_len_code_len = 0, max_dist_code_len = 0;
        int dist_extra_bits = 0, len_extra_bits = 0;
        int gain_dist_extra_bits = DIST_EXTRA_BITS_START;
        int gain_len_extra_bits = LEN_EXTRA_BITS_START;
        int max_code_len;
        int i;

        for (i = 0; i < LIT_LEN; i++)
                if (lit_len_hufftable[i].length > max_lit_code_len)
                        max_lit_code_len = lit_len_hufftable[i].length;

        for (i = 257; i < LIT_LEN - 1; i++) {
                if (lit_len_hufftable[i].length + len_extra_bits > max_len_code_len)
                        max_len_code_len = lit_len_hufftable[i].length + len_extra_bits;

                if (i == gain_len_extra_bits) {
                        gain_len_extra_bits += LEN_EXTRA_BITS_INTERVAL;
                        len_extra_bits += 1;
                }
        }

        for (i = 0; i < DIST_LEN; i++) {
                if (dist_hufftable[i].length + dist_extra_bits > max_dist_code_len)
                        max_dist_code_len = dist_hufftable[i].length + dist_extra_bits;

                if (i == gain_dist_extra_bits) {
                        gain_dist_extra_bits += DIST_EXTRA_BITS_INTERVAL;
                        dist_extra_bits += 1;
                }
        }

        max_code_len = max_lit_code_len + max_len_code_len + max_dist_code_len;

        /* Some versions of igzip can write up to one literal, one length and one
         * distance code at the same time. This checks to make sure that is
         * always writeable in bitbuf*/
        return (max_code_len > MAX_BITBUF_BIT_WRITE);
}

int
isal_create_hufftables(struct isal_hufftables *hufftables, struct isal_huff_histogram *histogram)
{
        struct huff_code lit_huff_table[LIT_LEN], dist_huff_table[DIST_LEN];
        uint32_t bit_count;
        int max_dist = convert_dist_to_dist_sym(IGZIP_HIST_SIZE);
        struct heap_tree heap_space;
        uint32_t heap_size;
        uint32_t code_len_count[MAX_HUFF_TREE_DEPTH + 1];
        struct BitBuf2 header_bitbuf;
        uint32_t max_lit_len_sym;
        uint32_t max_dist_sym;
        uint32_t hlit, hdist, i;
        uint8_t lens[CODE_LENS_SIZE];
        uint32_t cl_counts[CODE_LEN_CODES];
        struct rl_code cl_tokens[LIT_LEN + DIST_LEN + 2];
        uint32_t num_lens, num_cl_tokens;

        uint32_t *dist_table = hufftables->dist_table;
        uint32_t *len_table = hufftables->len_table;
        uint16_t *lit_table = hufftables->lit_table;
        uint16_t *dcodes = hufftables->dcodes;
        uint8_t *lit_table_sizes = hufftables->lit_table_sizes;
        uint8_t *dcodes_sizes = hufftables->dcodes_sizes;
        uint64_t *lit_len_histogram = histogram->lit_len_histogram;
        uint64_t *dist_histogram = histogram->dist_histogram;

        memset(hufftables, 0, sizeof(struct isal_hufftables));

        heap_size = init_heap64_complete(&heap_space, lit_len_histogram, LIT_LEN);
        gen_huff_code_lens(&heap_space, heap_size, code_len_count,
                           (struct huff_code *) lit_huff_table, LIT_LEN, MAX_DEFLATE_CODE_LEN);
        max_lit_len_sym = set_huff_codes(lit_huff_table, LIT_LEN, code_len_count);

        heap_size = init_heap64_complete(&heap_space, dist_histogram, DIST_LEN);
        gen_huff_code_lens(&heap_space, heap_size, code_len_count,
                           (struct huff_code *) dist_huff_table, max_dist, MAX_DEFLATE_CODE_LEN);
        max_dist_sym = set_huff_codes(dist_huff_table, DIST_LEN, code_len_count);

        if (are_hufftables_useable(lit_huff_table, dist_huff_table)) {
                heap_size = init_heap64_complete(&heap_space, lit_len_histogram, LIT_LEN);
                gen_huff_code_lens(&heap_space, heap_size, code_len_count,
                                   (struct huff_code *) lit_huff_table, LIT_LEN,
                                   MAX_SAFE_LIT_CODE_LEN);
                max_lit_len_sym = set_huff_codes(lit_huff_table, LIT_LEN, code_len_count);

                heap_size = init_heap64_complete(&heap_space, dist_histogram, DIST_LEN);
                gen_huff_code_lens(&heap_space, heap_size, code_len_count,
                                   (struct huff_code *) dist_huff_table, max_dist,
                                   MAX_SAFE_DIST_CODE_LEN);
                max_dist_sym = set_huff_codes(dist_huff_table, DIST_LEN, code_len_count);
        }

        create_code_tables(dcodes, dcodes_sizes, DIST_LEN - DCODE_OFFSET,
                           dist_huff_table + DCODE_OFFSET);

        create_code_tables(lit_table, lit_table_sizes, IGZIP_LIT_TABLE_SIZE, lit_huff_table);

        create_packed_len_table(len_table, lit_huff_table);
        create_packed_dist_table(dist_table, IGZIP_DIST_TABLE_SIZE, dist_huff_table);

        set_buf(&header_bitbuf, hufftables->deflate_hdr, sizeof(hufftables->deflate_hdr));
        init(&header_bitbuf);

        hlit = max_lit_len_sym - 256;
        hdist = max_dist_sym;

        /* Run length encode the length and distance huffman codes */
        memset(lens, 0, sizeof(lens));
        for (i = 0; i < 257 + hlit; i++)
                lens[i] = lit_huff_table[i].length;
        for (i = 0; i < 1 + hdist; i++)
                lens[i + hlit + 257] = dist_huff_table[i].length;
        num_lens = hlit + 257 + hdist + 1;
        lens[num_lens] = 0xFF;
        memset(cl_counts, 0, sizeof(cl_counts));
        num_cl_tokens = rl_encode_lens(lens, num_lens, cl_counts, cl_tokens);

        /* Create header */
        bit_count = write_dynamic_header(&header_bitbuf, cl_tokens, num_cl_tokens, cl_counts, hlit,
                                         hdist, LAST_BLOCK, &heap_space);
        flush(&header_bitbuf);

        hufftables->deflate_hdr_count = bit_count / 8;
        hufftables->deflate_hdr_extra_bits = bit_count % 8;

        return 0;
}

int
isal_create_hufftables_subset(struct isal_hufftables *hufftables,
                              struct isal_huff_histogram *histogram)
{
        struct huff_code lit_huff_table[LIT_LEN], dist_huff_table[DIST_LEN];
        uint32_t bit_count;
        int max_dist = convert_dist_to_dist_sym(IGZIP_HIST_SIZE);
        struct heap_tree heap_space;
        uint32_t heap_size;
        uint32_t code_len_count[MAX_HUFF_TREE_DEPTH + 1];
        struct BitBuf2 header_bitbuf;
        uint32_t max_lit_len_sym;
        uint32_t max_dist_sym;
        uint32_t hlit, hdist, i;
        uint8_t lens[CODE_LENS_SIZE];
        uint32_t cl_counts[CODE_LEN_CODES];
        struct rl_code cl_tokens[LIT_LEN + DIST_LEN + 2];
        uint32_t num_lens, num_cl_tokens;

        uint32_t *dist_table = hufftables->dist_table;
        uint32_t *len_table = hufftables->len_table;
        uint16_t *lit_table = hufftables->lit_table;
        uint16_t *dcodes = hufftables->dcodes;
        uint8_t *lit_table_sizes = hufftables->lit_table_sizes;
        uint8_t *dcodes_sizes = hufftables->dcodes_sizes;
        uint64_t *lit_len_histogram = histogram->lit_len_histogram;
        uint64_t *dist_histogram = histogram->dist_histogram;

        memset(hufftables, 0, sizeof(struct isal_hufftables));

        heap_size = init_heap64_semi_complete(&heap_space, lit_len_histogram, LIT_LEN,
                                              ISAL_DEF_LIT_SYMBOLS);
        gen_huff_code_lens(&heap_space, heap_size, code_len_count,
                           (struct huff_code *) lit_huff_table, LIT_LEN, MAX_DEFLATE_CODE_LEN);
        max_lit_len_sym = set_huff_codes(lit_huff_table, LIT_LEN, code_len_count);

        heap_size = init_heap64_complete(&heap_space, dist_histogram, DIST_LEN);
        gen_huff_code_lens(&heap_space, heap_size, code_len_count,
                           (struct huff_code *) dist_huff_table, max_dist, MAX_DEFLATE_CODE_LEN);
        max_dist_sym = set_huff_codes(dist_huff_table, DIST_LEN, code_len_count);

        if (are_hufftables_useable(lit_huff_table, dist_huff_table)) {
                heap_size = init_heap64_complete(&heap_space, lit_len_histogram, LIT_LEN);
                gen_huff_code_lens(&heap_space, heap_size, code_len_count,
                                   (struct huff_code *) lit_huff_table, LIT_LEN,
                                   MAX_SAFE_LIT_CODE_LEN);
                max_lit_len_sym = set_huff_codes(lit_huff_table, LIT_LEN, code_len_count);

                heap_size = init_heap64_complete(&heap_space, dist_histogram, DIST_LEN);
                gen_huff_code_lens(&heap_space, heap_size, code_len_count,
                                   (struct huff_code *) dist_huff_table, max_dist,
                                   MAX_SAFE_DIST_CODE_LEN);
                max_dist_sym = set_huff_codes(dist_huff_table, DIST_LEN, code_len_count);
        }

        create_code_tables(dcodes, dcodes_sizes, DIST_LEN - DCODE_OFFSET,
                           dist_huff_table + DCODE_OFFSET);

        create_code_tables(lit_table, lit_table_sizes, IGZIP_LIT_TABLE_SIZE, lit_huff_table);

        create_packed_len_table(len_table, lit_huff_table);
        create_packed_dist_table(dist_table, IGZIP_DIST_TABLE_SIZE, dist_huff_table);

        set_buf(&header_bitbuf, hufftables->deflate_hdr, sizeof(hufftables->deflate_hdr));
        init(&header_bitbuf);

        hlit = max_lit_len_sym - 256;
        hdist = max_dist_sym;

        /* Run length encode the length and distance huffman codes */
        memset(lens, 0, sizeof(lens));
        for (i = 0; i < 257 + hlit; i++)
                lens[i] = lit_huff_table[i].length;
        for (i = 0; i < 1 + hdist; i++)
                lens[i + hlit + 257] = dist_huff_table[i].length;
        num_lens = hlit + 257 + hdist + 1;
        lens[num_lens] = 0xFF;
        memset(cl_counts, 0, sizeof(cl_counts));
        num_cl_tokens = rl_encode_lens(lens, num_lens, cl_counts, cl_tokens);

        /* Create header */
        bit_count = write_dynamic_header(&header_bitbuf, cl_tokens, num_cl_tokens, cl_counts, hlit,
                                         hdist, LAST_BLOCK, &heap_space);
        flush(&header_bitbuf);

        hufftables->deflate_hdr_count = bit_count / 8;
        hufftables->deflate_hdr_extra_bits = bit_count % 8;

        return 0;
}

static void
expand_hufftables_icf(struct hufftables_icf *hufftables)
{
        uint32_t i, eb, j, k, len, code;
        struct huff_code orig[21];
        struct huff_code *lit_len_codes = hufftables->lit_len_table;
        struct huff_code *dist_codes = hufftables->dist_table;

        for (i = 0; i < 21; i++)
                orig[i] = lit_len_codes[i + 265];

        /* Each length code with eb extra bits takes 1 << eb entries, one per length */
        k = 265;
        for (i = 0; i < 20; i++) {
                eb = i / 4 + 1;
                len = orig[i].length;
                code = orig[i].code;
                for (j = 0; j < (1u << eb); j++)
                        lit_len_codes[k + j].code_and_length = (code | j << len) | (len + eb) << 24;
                k += 1 << eb;
        }
        /* The last entry is length 258, which has its own code */
        lit_len_codes[k - 1] = orig[20];

        dist_codes[DIST_LEN].code_and_length = 0;
}

uint64_t
create_hufftables_icf(struct BitBuf2 *bb, struct hufftables_icf *hufftables,
                      struct isal_mod_hist *hist, uint32_t end_of_block)
{
        uint32_t bl_count[MAX_DEFLATE_CODE_LEN + 1];
        uint32_t max_ll_code, max_d_code, num_ll, num_d, num_lens;
        struct heap_tree heap_space;
        struct rl_code cl_tokens[LIT_LEN + DIST_LEN + 2];
        uint32_t num_cl_tokens;
        uint32_t cl_counts[CODE_LEN_CODES];
        uint8_t lens[CODE_LENS_SIZE], d_lens[DIST_LEN];
        uint16_t ll_syms[LIT_LEN], d_syms[DIST_LEN];
        uint32_t i, s;
        uint64_t compressed_len = 0;
        uint64_t static_compressed_len = 3; /* The static header size */
        struct BitBuf2 bb_tmp;

        struct huff_code *ll_codes = hufftables->lit_len_table;
        struct huff_code *d_codes = hufftables->dist_table;
        uint32_t *ll_hist = hist->ll_hist;
        uint32_t *d_hist = hist->d_hist;
        struct huff_code *static_ll_codes = static_hufftables.lit_len_table;
        struct huff_code *static_d_codes = static_hufftables.dist_table;

        memcpy(&bb_tmp, bb, sizeof(struct BitBuf2));

        flatten_ll(hist->ll_hist);

        // make sure EOB is present
        if (ll_hist[256] == 0)
                ll_hist[256] = 1;

        memset(lens, 0, sizeof(lens));
        memset(d_lens, 0, sizeof(d_lens));
        num_ll = make_code_lens(ll_hist, LIT_LEN, MAX_DEFLATE_CODE_LEN, ll_syms, lens, bl_count,
                                &heap_space);
        memset(ll_codes, 0, LIT_LEN * sizeof(*ll_codes));
        set_codes_by_sym(ll_codes, ll_syms, num_ll, lens, bl_count, MAX_DEFLATE_CODE_LEN);
        max_ll_code = ll_syms[num_ll - 1];

        num_d = make_code_lens(d_hist, DIST_LEN, MAX_DEFLATE_CODE_LEN, d_syms, d_lens, bl_count,
                               &heap_space);
        memset(d_codes, 0, DIST_LEN * sizeof(*d_codes));
        set_codes_by_sym(d_codes, d_syms, num_d, d_lens, bl_count, MAX_DEFLATE_CODE_LEN);
        for (i = 0; i < num_d; i++)
                d_codes[d_syms[i]].extra_bit_count = dist_code_extra_bits[d_syms[i]];
        max_d_code = d_syms[num_d - 1];

        assert(max_ll_code >= 256); // must be EOB code
        assert(max_d_code != 0);

        /* Only the symbols that occur add to the lengths of the block */
        for (i = 0; i < num_ll; i++) {
                s = ll_syms[i];
                if (s <= 256) {
                        compressed_len += lens[s] * ll_hist[s];
                        static_compressed_len += static_ll_codes[s].length * ll_hist[s];
                } else {
                        compressed_len += (lens[s] + len_code_extra_bits[s - 257]) * ll_hist[s];
                        static_compressed_len +=
                                (static_ll_codes[s].length + len_code_extra_bits[s - 257]) *
                                ll_hist[s];
                }
        }

        for (i = 0; i < num_d; i++) {
                s = d_syms[i];
                compressed_len += (d_lens[s] + dist_code_extra_bits[s]) * d_hist[s];
                static_compressed_len +=
                        (static_d_codes[s].length + dist_code_extra_bits[s]) * d_hist[s];
        }

        if (static_compressed_len > compressed_len) {
                /* Run length encode the length and distance huffman codes */
                num_lens = max_ll_code + max_d_code + 2;
                memcpy(&lens[max_ll_code + 1], d_lens, max_d_code + 1);
                lens[num_lens] = 0xFF;
                memset(cl_counts, 0, sizeof(cl_counts));
                num_cl_tokens = rl_encode_lens(lens, num_lens, cl_counts, cl_tokens);

                /* Create header */
                write_dynamic_header(bb, cl_tokens, num_cl_tokens, cl_counts, max_ll_code - 256,
                                     max_d_code, end_of_block, &heap_space);
                compressed_len += 8 * buffer_used(bb) + bb->m_bit_count;
        }

        /* Substitute in static block since it creates smaller block */
        if (static_compressed_len <= compressed_len) {
                memcpy(hufftables, &static_hufftables, sizeof(struct hufftables_icf));
                memcpy(bb, &bb_tmp, sizeof(struct BitBuf2));
                end_of_block = end_of_block ? 1 : 0;
                write_bits(bb, 0x2 | end_of_block, 3);
                compressed_len = static_compressed_len;
        }

        expand_hufftables_icf(hufftables);
        return compressed_len;
}
