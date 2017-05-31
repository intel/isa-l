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

#include <limits.h>
#include <stdint.h>

#if __WORDSIZE == 64 || _WIN64 || __x86_64__
# define notbit0 0xfefefefefefefefeULL
# define bit7    0x8080808080808080ULL
# define gf8poly 0x1d1d1d1d1d1d1d1dULL
#else
# define notbit0 0xfefefefeUL
# define bit7    0x80808080UL
# define gf8poly 0x1d1d1d1dUL
#endif

int pq_gen_base(int vects, int len, void **array)
{
	int i, j;
	unsigned long p, q, s;
	unsigned long **src = (unsigned long **)array;
	int blocks = len / sizeof(long);

	for (i = 0; i < blocks; i++) {
		q = p = src[vects - 3][i];

		for (j = vects - 4; j >= 0; j--) {
			p ^= s = src[j][i];
			q = s ^ (((q << 1) & notbit0) ^	// shift each byte
				 ((((q & bit7) << 1) - ((q & bit7) >> 7))	// mask out bytes
				  & gf8poly));	// apply poly
		}

		src[vects - 2][i] = p;	// second to last pointer is p
		src[vects - 1][i] = q;	// last pointer is q
	}
	return 0;
}

int pq_check_base(int vects, int len, void **array)
{
	int i, j;
	unsigned char p, q, s;
	unsigned char **src = (unsigned char **)array;

	for (i = 0; i < len; i++) {
		q = p = src[vects - 3][i];

		for (j = vects - 4; j >= 0; j--) {
			s = src[j][i];
			p ^= s;

			// mult by GF{2}
			q = s ^ ((q << 1) ^ ((q & 0x80) ? 0x1d : 0));
		}

		if (src[vects - 2][i] != p)	// second to last pointer is p
			return i | 1;
		if (src[vects - 1][i] != q)	// last pointer is q
			return i | 2;
	}
	return 0;
}

int xor_gen_base(int vects, int len, void **array)
{
	int i, j;
	unsigned char parity;
	unsigned char **src = (unsigned char **)array;

	for (i = 0; i < len; i++) {
		parity = src[0][i];
		for (j = 1; j < vects - 1; j++)
			parity ^= src[j][i];

		src[vects - 1][i] = parity;	// last pointer is dest

	}

	return 0;
}

int xor_check_base(int vects, int len, void **array)
{
	int i, j, fail = 0;

	unsigned char parity;
	unsigned char **src = (unsigned char **)array;

	for (i = 0; i < len; i++) {
		parity = 0;
		for (j = 0; j < vects; j++)
			parity ^= src[j][i];

		if (parity != 0) {
			fail = 1;
			break;
		}
	}
	if (fail && len > 0)
		return len;
	return fail;
}

struct slver {
	unsigned short snum;
	unsigned char ver;
	unsigned char core;
};

struct slver pq_gen_base_slver_0001012a;
struct slver pq_gen_base_slver = { 0x012a, 0x01, 0x00 };

struct slver xor_gen_base_slver_0001012b;
struct slver xor_gen_base_slver = { 0x012b, 0x01, 0x00 };

struct slver pq_check_base_slver_0001012c;
struct slver pq_check_base_slver = { 0x012c, 0x01, 0x00 };

struct slver xor_check_base_slver_0001012d;
struct slver xor_check_base_slver = { 0x012d, 0x01, 0x00 };
