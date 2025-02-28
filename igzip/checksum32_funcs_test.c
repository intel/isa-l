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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include "igzip_checksums.h"
#include "checksum_test_ref.h"
#include "test.h"

#ifndef TEST_SEED
#define TEST_SEED 0x1234
#endif

#define MAX_BUF   512
#define TEST_SIZE 20

typedef uint32_t (*checksum32_func_t)(uint32_t, const unsigned char *, uint64_t);

typedef struct func_case {
        char *note;
        checksum32_func_t checksum32_func_call;
        checksum32_func_t checksum32_ref_call;
} func_case_t;

func_case_t test_funcs[] = {
        { "checksum32_adler", isal_adler32, adler_ref },
};

// Generates pseudo-random data

void
rand_buffer(unsigned char *buf, long buffer_size)
{
        long i;
        for (i = 0; i < buffer_size; i++)
                buf[i] = rand();
}

// Test cases
int
zeros_test(func_case_t *test_func);
int
simple_pattern_test(func_case_t *test_func);
int
seeds_sizes_test(func_case_t *test_func);
int
eob_test(func_case_t *test_func);
int
update_test(func_case_t *test_func);
int
update_over_mod_test(func_case_t *test_func);

void *buf_alloc = NULL;

int
main(int argc, char *argv[])
{
        int fail = 0, fail_case;
        int i, ret;
        func_case_t *test_func;

        // Align to MAX_BUF boundary
        ret = posix_memalign(&buf_alloc, MAX_BUF, MAX_BUF * TEST_SIZE);
        if (ret) {
                printf("alloc error: Fail");
                return -1;
        }
        srand(TEST_SEED);
        printf("CHECKSUM32 Tests seed=0x%x\n", TEST_SEED);

        for (i = 0; i < sizeof(test_funcs) / sizeof(test_funcs[0]); i++) {
                fail_case = 0;
                test_func = &test_funcs[i];

                printf("Test %s ", test_func->note);
                fail_case += zeros_test(test_func);
                fail_case += simple_pattern_test(test_func);
                fail_case += seeds_sizes_test(test_func);
                fail_case += eob_test(test_func);
                fail_case += update_test(test_func);
                fail_case += update_over_mod_test(test_func);
                printf("Test %s done: %s\n", test_func->note, fail_case ? "Fail" : "Pass");

                if (fail_case) {
                        printf("\n%s Failed %d tests\n", test_func->note, fail_case);
                        fail++;
                }
        }

        printf("CHECKSUM32 Tests all done: %s\n", fail ? "Fail" : "Pass");

        aligned_free(buf_alloc);

        return fail;
}

// Test of all zeros
int
zeros_test(func_case_t *test_func)
{
        volatile uint32_t c_dut, c_ref;
        int fail = 0;
        unsigned char *buf = NULL;

        buf = (unsigned char *) buf_alloc;
        memset(buf, 0, MAX_BUF * 10);
        c_dut = test_func->checksum32_func_call(TEST_SEED, buf, MAX_BUF * 10);
        c_ref = test_func->checksum32_ref_call(TEST_SEED, buf, MAX_BUF * 10);

        if (c_dut != c_ref) {
                fail++;
                printf("\n		opt    ref\n");
                printf("		------ ------\n");
                printf("checksum	zero = 0x%8x 0x%8x \n", c_dut, c_ref);
        }
#ifdef TEST_VERBOSE
        else
                printf(".");
#endif

        return fail;
}

// Another simple test pattern
int
simple_pattern_test(func_case_t *test_func)
{
        volatile uint32_t c_dut, c_ref;
        int fail = 0;
        unsigned char *buf = NULL;

        buf = (unsigned char *) buf_alloc;
        memset(buf, 0x8a, MAX_BUF);
        c_dut = test_func->checksum32_func_call(TEST_SEED, buf, MAX_BUF);
        c_ref = test_func->checksum32_ref_call(TEST_SEED, buf, MAX_BUF);
        if (c_dut != c_ref) {
                fail++;
                printf("fail checksum  all 8a = 0x%8x 0x%8x\n", c_dut, c_ref);
        }
#ifdef TEST_VERBOSE
        else
                printf(".");
#endif

        return fail;
}

int
seeds_sizes_test(func_case_t *test_func)
{
        volatile uint32_t c_dut, c_ref;
        int fail = 0;
        int i;
        uint32_t r, s;
        unsigned char *buf = NULL;

        // Do a few random tests
        buf = (unsigned char *) buf_alloc; // reset buf
        r = rand();
        rand_buffer(buf, MAX_BUF * TEST_SIZE);

        for (i = 0; i < TEST_SIZE; i++) {
                c_dut = test_func->checksum32_func_call(r, buf, MAX_BUF);
                c_ref = test_func->checksum32_ref_call(r, buf, MAX_BUF);
                if (c_dut != c_ref) {
                        fail++;
                        printf("fail checksum rand%3d = 0x%8x 0x%8x\n", i, c_dut, c_ref);
                }
#ifdef TEST_VERBOSE
                else
                        printf(".");
#endif
                buf += MAX_BUF;
        }

        // Do a few random sizes
        buf = (unsigned char *) buf_alloc; // reset buf
        r = rand();

        for (i = MAX_BUF; i >= 0; i--) {
                c_dut = test_func->checksum32_func_call(r, buf, i);
                c_ref = test_func->checksum32_ref_call(r, buf, i);
                if (c_dut != c_ref) {
                        fail++;
                        printf("fail random size%i 0x%8x 0x%8x\n", i, c_dut, c_ref);
                }
#ifdef TEST_VERBOSE
                else
                        printf(".");
#endif
        }

        // Try different seeds
        for (s = 0; s < 20; s++) {
                buf = (unsigned char *) buf_alloc; // reset buf

                r = rand();                            // just to get a new seed
                rand_buffer(buf, MAX_BUF * TEST_SIZE); // new pseudo-rand data

#ifdef TEST_VERBOSE
                printf("seed = 0x%x\n", r);
#endif

                for (i = 0; i < TEST_SIZE; i++) {
                        c_dut = test_func->checksum32_func_call(r, buf, MAX_BUF);
                        c_ref = test_func->checksum32_ref_call(r, buf, MAX_BUF);
                        if (c_dut != c_ref) {
                                fail++;
                                printf("fail checksum rand%3d = 0x%8x 0x%8x\n", i, c_dut, c_ref);
                        }
#ifdef TEST_VERBOSE
                        else
                                printf(".");
#endif
                        buf += MAX_BUF;
                }
        }

        return fail;
}

// Run tests at end of buffer
int
eob_test(func_case_t *test_func)
{
        volatile uint32_t c_dut, c_ref;
        int fail = 0;
        int i;
        unsigned char *buf = NULL;

        buf = (unsigned char *) buf_alloc;       // reset buf
        buf = buf + ((MAX_BUF - 1) * TEST_SIZE); // Line up TEST_SIZE from end
        for (i = 0; i < TEST_SIZE; i++) {
                c_dut = test_func->checksum32_func_call(TEST_SEED, buf + i, TEST_SIZE - i);
                c_ref = test_func->checksum32_ref_call(TEST_SEED, buf + i, TEST_SIZE - i);
                if (c_dut != c_ref) {
                        fail++;
                        printf("fail checksum eob rand%3d = 0x%8x 0x%8x\n", i, c_dut, c_ref);
                }
#ifdef TEST_VERBOSE
                else
                        printf(".");
#endif
        }

        return fail;
}

int
update_test(func_case_t *test_func)
{
        volatile uint32_t c_dut, c_ref;
        int fail = 0;
        int i;
        uint32_t r;
        unsigned char *buf = NULL;

        buf = (unsigned char *) buf_alloc; // reset buf
        r = rand();
        // Process the whole buf with reference func single call.
        c_ref = test_func->checksum32_ref_call(r, buf, MAX_BUF * TEST_SIZE);
        // Process buf with update method.
        for (i = 0; i < TEST_SIZE; i++) {
                c_dut = test_func->checksum32_func_call(r, buf, MAX_BUF);
                // Update checksum seeds and buf pointer.
                r = c_dut;
                buf += MAX_BUF;
        }

        if (c_dut != c_ref) {
                fail++;
                printf("checksum rand%3d = 0x%8x 0x%8x\n", i, c_dut, c_ref);
        }
#ifdef TEST_VERBOSE
        else
                printf(".");
#endif

        return fail;
}

int
update_over_mod_test(func_case_t *test_func)
{
        volatile uint32_t c_dut, c_ref;
        int fail = 0;
        int i;
        unsigned char *buf = NULL;

        buf = malloc(ADLER_MOD);
        if (buf == NULL)
                return 1;
        memset(buf, 0xff, ADLER_MOD);

        c_ref = c_dut = rand();

        // Process buf with update method.
        for (i = 0; i < 20; i++) {
                c_ref = test_func->checksum32_ref_call(c_ref, buf, ADLER_MOD - 64);
                c_dut = test_func->checksum32_func_call(c_dut, buf, ADLER_MOD - 64);
        }

        if (c_dut != c_ref) {
                fail++;
                printf("checksum rand%3d = 0x%8x 0x%8x\n", i, c_dut, c_ref);
        }
#ifdef TEST_VERBOSE
        else
                printf(".");
#endif

        free(buf);
        return fail;
}
