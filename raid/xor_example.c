/**********************************************************************
  Copyright(c) 2011-2013 Intel Corporation All rights reserved.

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
#include <stdio.h>
#include <stdlib.h>
#include "raid.h"
#include "types.h"

#define TEST_SOURCES 16
#define TEST_LEN     16*1024

int main(int argc, char *argv[])
{
	int i, j, should_pass, should_fail;
	void *buffs[TEST_SOURCES + 1];

	printf("XOR example\n");
	for (i = 0; i < TEST_SOURCES + 1; i++) {
		void *buf;
		if (posix_memalign(&buf, 32, TEST_LEN)) {
			printf("alloc error: Fail");
			return 1;
		}
		buffs[i] = buf;
	}

	printf("Make random data\n");
	for (i = 0; i < TEST_SOURCES + 1; i++)
		for (j = 0; j < TEST_LEN; j++)
			((char *)buffs[i])[j] = rand();

	printf("Generate xor parity\n");
	xor_gen(TEST_SOURCES + 1, TEST_LEN, buffs);

	printf("Check parity: ");
	should_pass = xor_check(TEST_SOURCES + 1, TEST_LEN, buffs);
	printf("%s\n", should_pass == 0 ? "Pass" : "Fail");

	printf("Find corruption: ");
	((char *)buffs[TEST_SOURCES / 2])[TEST_LEN / 2] ^= 1;	// flip one bit
	should_fail = xor_check(TEST_SOURCES + 1, TEST_LEN, buffs);	//recheck
	printf("%s\n", should_fail != 0 ? "Pass" : "Fail");

	return 0;
}
