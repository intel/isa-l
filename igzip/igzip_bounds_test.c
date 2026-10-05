/**********************************************************************
  Copyright(c) 2026 Intel Corporation All rights reserved.

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

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifdef _WIN32
#include <windows.h>
#else
#include <sys/mman.h>
#include <unistd.h>
#endif
#include "igzip_lib.h"

#define MAX_INPUT  65536
#define MAX_OUTPUT (2 * MAX_INPUT + 1024)

static int
test_input(uint8_t *input, uint32_t size)
{
        static struct isal_zstream stream;
        static struct inflate_state inflate;
        static struct isal_huff_histogram histogram, padded_histogram;
        static uint8_t padded[MAX_INPUT + 8], output[MAX_OUTPUT], decoded[MAX_INPUT + 1];
        const uint32_t level_sizes[] = { ISAL_DEF_LVL0_DEFAULT, ISAL_DEF_LVL1_DEFAULT,
                                         ISAL_DEF_LVL2_DEFAULT, ISAL_DEF_LVL3_DEFAULT };

        memcpy(padded, input, size);
        memset(padded + size, 0, 8);
        memset(&histogram, 0, sizeof(histogram));
        memset(&padded_histogram, 0, sizeof(padded_histogram));
        isal_update_histogram(input, size, &histogram);
        isal_update_histogram(padded, size, &padded_histogram);
        if (memcmp(histogram.lit_len_histogram, padded_histogram.lit_len_histogram,
                   sizeof(histogram.lit_len_histogram)) ||
            memcmp(histogram.dist_histogram, padded_histogram.dist_histogram,
                   sizeof(histogram.dist_histogram))) {
                fprintf(stderr, "Histogram differs with input padding, size=%u\n", size);
                return 1;
        }

        for (uint32_t level = 0; level <= ISAL_DEF_MAX_LEVEL; level++) {
                void *level_buf = NULL;
                if (level_sizes[level]) {
                        level_buf = malloc(level_sizes[level]);
                        if (level_buf == NULL) {
                                fprintf(stderr, "Failed to allocate level buffer\n");
                                return 1;
                        }
                }
                for (int stateless = 0; stateless <= 1; stateless++) {
                        isal_deflate_init(&stream);
                        stream.level = level;
                        stream.level_buf = level_buf;
                        stream.level_buf_size = level_sizes[level];
                        stream.next_in = input;
                        stream.avail_in = size;
                        stream.next_out = output;
                        stream.avail_out = sizeof(output);
                        stream.end_of_stream = 1;
                        int ret =
                                stateless ? isal_deflate_stateless(&stream) : isal_deflate(&stream);
                        if (ret != COMP_OK || stream.total_in != size || stream.avail_in != 0 ||
                            stream.internal_state.state != ZSTATE_END) {
                                fprintf(stderr,
                                        "Compression failed: size=%u level=%u stateless=%d "
                                        "ret=%d\n",
                                        size, level, stateless, ret);
                                free(level_buf);
                                return 1;
                        }
                        isal_inflate_init(&inflate);
                        inflate.next_in = output;
                        inflate.avail_in = stream.total_out;
                        inflate.next_out = decoded;
                        inflate.avail_out = sizeof(decoded);
                        ret = isal_inflate_stateless(&inflate);
                        if (ret != ISAL_DECOMP_OK || inflate.total_out != size ||
                            memcmp(decoded, input, size)) {
                                fprintf(stderr,
                                        "Round trip failed: size=%u level=%u stateless=%d "
                                        "ret=%d\n",
                                        size, level, stateless, ret);
                                free(level_buf);
                                return 1;
                        }
                }
                free(level_buf);
        }
        return 0;
}

int
main(void)
{
        size_t page_size, map_size, readable_size;
        uint8_t *region;
        int ret = 0;
        uint32_t size;
#ifdef _WIN32
        DWORD old_protect;
        SYSTEM_INFO si;

        GetSystemInfo(&si);
        page_size = si.dwPageSize;
#else
        long ps = sysconf(_SC_PAGESIZE);

        if (ps <= 0) {
                fprintf(stderr, "Failed to determine page size\n");
                return 1;
        }
        page_size = (size_t) ps;
#endif

        if (page_size == 0 || page_size > MAX_INPUT) {
                fprintf(stderr, "Invalid page size\n");
                return 1;
        }

        readable_size = ((MAX_INPUT + page_size - 1) / page_size) * page_size;
        map_size = readable_size + page_size;

#ifdef _WIN32
        region = VirtualAlloc(NULL, map_size, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
        if (region == NULL) {
                fprintf(stderr, "VirtualAlloc failed: %lu\n", GetLastError());
                return 1;
        }
        if (!VirtualProtect(region + readable_size, page_size, PAGE_NOACCESS, &old_protect)) {
                fprintf(stderr, "VirtualProtect failed: %lu\n", GetLastError());
                VirtualFree(region, 0, MEM_RELEASE);
                return 1;
        }
#else
        region = mmap(NULL, map_size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
        if (region == MAP_FAILED) {
                perror("mmap");
                return 1;
        }
        if (mprotect(region + readable_size, page_size, PROT_NONE)) {
                perror("mprotect");
                munmap(region, map_size);
                return 1;
        }
#endif
        /* Guard pages catch assembly loads that sanitizers cannot instrument. */
        for (size = 0; size <= MAX_INPUT && !ret; size++) {
                uint8_t *input;
                int pattern;

                if (size > 17 && !(size >= 63 && size <= 65) && !(size >= 255 && size <= 273) &&
                    !(size >= 288 && size <= 303) && size != MAX_INPUT)
                        continue;

                input = region + readable_size - size;

                for (pattern = 0; pattern < 3 && !ret; pattern++) {
                        uint32_t i;

                        for (i = 0; i < size; i++)
                                input[i] = pattern == 0   ? 'A'
                                           : pattern == 1 ? "ABCD"[i & 3]
                                                          : (uint8_t) i;
                        ret = test_input(input, size);
                        if (ret)
                                fprintf(stderr, "Input bounds test failed: size=%u pattern=%d\n",
                                        size, pattern);
                }
        }
#ifdef _WIN32
        if (!VirtualFree(region, 0, MEM_RELEASE)) {
                fprintf(stderr, "VirtualFree failed: %lu\n", GetLastError());
                ret = 1;
        }
#else
        if (munmap(region, map_size)) {
                perror("munmap");
                ret = 1;
        }
#endif
        printf("igzip_bounds_test: %s\n", ret ? "Fail" : "Pass");
        return ret;
}
