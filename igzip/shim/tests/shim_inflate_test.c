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

/*
 * Regression test for the zlib-compatibility shim's inflate() input accounting.
 *
 * A previous "trailer over-consumption" workaround in shim_inflate.c rewound
 * next_in by (8 - avail_in) whenever a raw-deflate stream finished with
 * 0 < avail_in < 8. The rewind was derived purely from avail_in and was never
 * bounded by the number of bytes actually consumed from the caller's buffer,
 * so any final inflate() call carrying fewer than 8 input bytes produced a
 * next_in pointing before the start of the caller's buffer, an avail_in larger
 * than the caller ever supplied, and an underflowed total_in.
 *
 * This test asserts the zlib contract on every inflate() call:
 *   next_in - chunk_start == supplied - avail_in,  0 <= consumed <= supplied
 * and that total_in never exceeds the bytes actually handed to the library.
 *
 * Each input chunk is placed at the start of a mapped region with a PROT_NONE
 * guard page immediately before it, so a rewind below the buffer faults
 * immediately rather than silently reading adjacent memory.
 *
 * The shim is only supported on Linux (see README.md), so this test uses POSIX
 * memory APIs unconditionally.
 */

#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <sys/mman.h>
#include <unistd.h>

#include "shim_inflate.h"

int
inflateInit2_(z_streamp strm, int windowBits);
int
inflate(z_streamp strm, int flush);
int
inflateEnd(z_streamp strm);

static int failures = 0;

#define CHECK(cond, ...)                                                                           \
        do {                                                                                       \
                if (!(cond)) {                                                                     \
                        printf("  FAIL: " __VA_ARGS__);                                            \
                        printf("\n        (%s) at %s:%d\n", #cond, __FILE__, __LINE__);            \
                        failures++;                                                                \
                }                                                                                  \
        } while (0)

/* "hello world hello world" as a fixed-Huffman final raw-deflate block */
static const uint8_t vec_fixed[] = { 0xcb, 0x48, 0xcd, 0xc9, 0xc9, 0x57, 0x28, 0xcf,
                                     0x2f, 0xca, 0x49, 0x51, 0xc8, 0x40, 0xb0, 0x01 };
static const char vec_fixed_out[] = "hello world hello world";

/* the same payload as a final stored block: 0x01, LEN, NLEN, raw bytes */
static const uint8_t vec_stored[] = { 0x01, 0x17, 0x00, 0xe8, 0xff, 'h', 'e', 'l', 'l', 'o',
                                      ' ',  'w',  'o',  'r',  'l',  'd', ' ', 'h', 'e', 'l',
                                      'l',  'o',  ' ',  'w',  'o',  'r', 'l', 'd' };
static const char vec_stored_out[] = "hello world hello world";

/* complete, empty, final block - the minimal stream that reaches BLOCK_FINISH */
static const uint8_t vec_empty[] = { 0x03, 0x00 };
static const char vec_empty_out[] = "";

struct vector {
        const char *name;
        const uint8_t *data;
        unsigned int len;
        const char *out;
};

/*
 * Helper for byte displacement of `p` relative to `base`.
 *
 * Computed on integer-form addresses rather than by pointer subtraction.
 */
static inline intptr_t
addr_delta(const void *p, const void *base)
{
        return (intptr_t) ((uintptr_t) p - (uintptr_t) base);
}

/*
 * Allocate `bytes` such that the returned pointer is the first byte of a
 * mapping and the page before it is inaccessible.
 */
static uint8_t *
guarded_alloc(size_t bytes, void **base_out, size_t *map_len_out)
{
        size_t ps, rounded;
        uint8_t *b;
        long sc;

        sc = sysconf(_SC_PAGESIZE);
        if (sc <= 0)
                return NULL;

        ps = (size_t) sc;
        if (ps > UINT32_MAX)
                return NULL; /* invalid page size */

        rounded = bytes / ps + ((bytes % ps) != 0 ? 1 : 0); /* rounded up number of pages */
        rounded = rounded * ps; /* memory size of rounded up number of pages */

        if (rounded == 0)
                rounded = ps;

        b = mmap(NULL, rounded + ps, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
        if (b == MAP_FAILED)
                return NULL;

        if (mprotect(b, ps, PROT_NONE) != 0) {
                munmap(b, rounded + ps);
                return NULL;
        }

        if (base_out != NULL)
                *base_out = b;

        if (map_len_out != NULL)
                *map_len_out = rounded + ps;

        return b + ps;
}

static void
guarded_free(void *base, size_t map_len)
{
        if (base)
                munmap(base, map_len);
}

/*
 * Feed `total` bytes (stream followed by `trail` filler bytes) to inflate() in
 * two pieces split at `split`, validating the input accounting on every call.
 */
static void
run_case(const struct vector *v, unsigned int trail, unsigned int split)
{
        uint8_t all[64];
        unsigned int total = v->len + trail;
        unsigned int sizes[2];
        unsigned int offs[2];
        unsigned char out[256];
        unsigned int supplied_so_far = 0;
        z_stream strm;
        int ret = Z_OK;
        int i;

        if (total > sizeof(all))
                return;

        memcpy(all, v->data, v->len);
        memset(all + v->len, 0xAA, trail);

        sizes[0] = split;
        sizes[1] = total - split;
        offs[0] = 0;
        offs[1] = split;

        memset(&strm, 0, sizeof(strm));
        if (inflateInit2_(&strm, -15) != Z_OK) {
                printf("  FAIL: inflateInit2_ failed\n");
                failures++;
                return;
        }

        memset(out, 0, sizeof(out));
        strm.next_out = out;
        strm.avail_out = sizeof(out) - 1;

        for (i = 0; i < 2; i++) {
                unsigned int n = sizes[i];
                void *base = NULL;
                size_t map_len = 0;
                uint8_t *chunk;
                long consumed;

                if (n == 0)
                        continue;

                chunk = guarded_alloc(n, &base, &map_len);
                if (!chunk) {
                        printf("  FAIL: guarded_alloc failed\n");
                        failures++;
                        inflateEnd(&strm);
                        return;
                }
                memcpy(chunk, all + offs[i], n);

                strm.next_in = chunk;
                strm.avail_in = n;
                supplied_so_far += n;

                ret = inflate(&strm, Z_NO_FLUSH);

                consumed = (long) addr_delta(strm.next_in, chunk);

                CHECK(consumed >= 0,
                      "%s trail=%u split=%u chunk=%d: next_in moved %ld bytes BEFORE the "
                      "caller's buffer",
                      v->name, trail, split, i, consumed);
                CHECK(consumed <= (long) n,
                      "%s trail=%u split=%u chunk=%d: next_in advanced %ld bytes past a %u-byte "
                      "buffer",
                      v->name, trail, split, i, consumed, n);
                CHECK(strm.avail_in <= n,
                      "%s trail=%u split=%u chunk=%d: avail_in=%u exceeds the %u bytes supplied",
                      v->name, trail, split, i, strm.avail_in, n);
                CHECK(consumed == (long) n - (long) strm.avail_in,
                      "%s trail=%u split=%u chunk=%d: next_in/avail_in disagree "
                      "(consumed=%ld, n-avail_in=%ld)",
                      v->name, trail, split, i, consumed, (long) n - (long) strm.avail_in);
                CHECK(strm.total_in <= supplied_so_far,
                      "%s trail=%u split=%u chunk=%d: total_in=%lu exceeds the %u bytes supplied "
                      "so far",
                      v->name, trail, split, i, (unsigned long) strm.total_in, supplied_so_far);

                guarded_free(base, map_len);

                if (ret == Z_STREAM_END || ret < 0)
                        break;
        }

        CHECK(ret == Z_STREAM_END, "%s trail=%u split=%u: expected Z_STREAM_END, got %d", v->name,
              trail, split, ret);

        if (ret == Z_STREAM_END) {
                CHECK(strm.total_in == v->len,
                      "%s trail=%u split=%u: total_in=%lu, expected the %u-byte stream length",
                      v->name, trail, split, (unsigned long) strm.total_in, v->len);
                CHECK(strm.total_out == strlen(v->out),
                      "%s trail=%u split=%u: total_out=%lu, expected %u", v->name, trail, split,
                      (unsigned long) strm.total_out, (unsigned int) strlen(v->out));
                CHECK(memcmp(out, v->out, strlen(v->out)) == 0,
                      "%s trail=%u split=%u: output mismatch, got '%s'", v->name, trail, split,
                      out);
        }

        inflateEnd(&strm);
}

int
main(void)
{
        static const struct vector vectors[] = {
                { "fixed", vec_fixed, (unsigned int) sizeof(vec_fixed), vec_fixed_out },
                { "stored", vec_stored, (unsigned int) sizeof(vec_stored), vec_stored_out },
                { "empty", vec_empty, (unsigned int) sizeof(vec_empty), vec_empty_out },
        };
        unsigned int v, trail, split, total;

        printf("shim_inflate_test: raw-deflate input accounting\n");

        for (v = 0; v < sizeof(vectors) / sizeof(vectors[0]); v++) {
                for (trail = 0; trail <= 8; trail++) {
                        total = vectors[v].len + trail;
                        /*
                         * split == total is the single-shot case; the remaining
                         * splits exercise a final chunk of every size from 1 up,
                         * including the sub-8-byte sizes that triggered the
                         * out-of-bounds rewind.
                         */
                        for (split = 0; split <= total; split++)
                                run_case(&vectors[v], trail, split);
                }
        }

        if (failures) {
                printf("shim_inflate_test: FAILED (%d checks)\n", failures);
                return 1;
        }

        printf("shim_inflate_test: Pass\n");
        return 0;
}
