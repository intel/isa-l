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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include "crc.h"
#include "crc64.h"
#include "test.h"
#include "types.h"

// Large buffer from which chunks can be taken for performing CRCs
#define BUFFER_LEN (1ull << 30)

#ifndef TEST_SEED
# define TEST_SEED 0x1234
#endif

typedef uint64_t(*crc64_func_t) (uint64_t, const uint8_t *, uint64_t);

typedef struct func_case {
	char *note;
	crc64_func_t crc64_func_call;
	crc64_func_t crc64_ref_call;
} func_case_t;

func_case_t test_funcs[] = {
	{"crc64_ecma_norm", crc64_ecma_norm, crc64_ecma_norm_base},
	{"crc64_ecma_refl", crc64_ecma_refl, crc64_ecma_refl_base},
	{"crc64_iso_norm", crc64_iso_norm, crc64_iso_norm_base},
	{"crc64_iso_refl", crc64_iso_refl, crc64_iso_refl_base},
	{"crc64_jones_norm", crc64_jones_norm, crc64_jones_norm_base},
	{"crc64_jones_refl", crc64_jones_refl, crc64_jones_refl_base}
};

typedef struct test_func_selection {
	char *name;
	int indices[6];
} test_func_selection_t;

test_func_selection_t test_func_selections[] = {
	{"all",         {0,  1,  2,  3,  4,  5}},
	{"norm",        {0,  2,  4, -1, -1, -1}},
	{"refl",        {1,  3,  5, -1, -1, -1}},
	{"ecma",        {0,  1, -1, -1, -1, -1}},
	{"ecma_norm",   {0, -1, -1, -1, -1, -1}},
	{"ecma_refl",   {1, -1, -1, -1, -1, -1}},
	{"iso",         {2,  3, -1, -1, -1, -1}},
	{"iso_norm",    {2, -1, -1, -1, -1, -1}},
	{"iso_refl",    {3, -1, -1, -1, -1, -1}},
	{"jones",       {4,  5, -1, -1, -1, -1}},
	{"jones_norm",  {4, -1, -1, -1, -1, -1}},
	{"jones_refl",  {5, -1, -1, -1, -1, -1}},
};

static uint64_t offset = 0;

uint64_t cold_buffer_crc_wrapper(crc64_func_t crc_func, uint64_t seed, const uint8_t * buf, uint64_t len, uint64_t len_pages)
{
	offset += (48271*4096); // move offset ~200MB into the buffer (large prime number of pages)

	if (offset + len > BUFFER_LEN)
	{
		offset += (len_pages - BUFFER_LEN);
	}

	return crc_func(seed, buf+offset, len);
}

void run_tests(test_func_selection_t selected_funcs, void *buf, uint64_t test_size, uint32_t benchmark_time, bool hot, bool verbose)
{
	struct perf start;
	uint64_t crc;

	// len_pages is len rounded up to the nearest number of pages (assuming 4KiB pages)
	// used to maintain page alignment in cold tests
	uint64_t len_pages = (1+((test_size-1) >> 12)) << 12;

	for (uint32_t j = 0; j < sizeof(selected_funcs.indices) / sizeof(int); j++) {
		if(selected_funcs.indices[j] < 0)
			break;

		func_case_t test_func = test_funcs[selected_funcs.indices[j]];

		if (verbose) {
			printf("%s_perf %luB %s:\n", test_func.note, test_size, hot ? "hot " : "cold");
			printf("Start timed tests\n");
		} else {
			printf("%s %lu %s ", test_func.note, test_size, hot ? "hot " : "cold");
		}

		fflush(0);

		if (hot) {
			BENCHMARK(&start, benchmark_time, crc =
				test_func.crc64_func_call(TEST_SEED, buf, test_size));
		} else {
			BENCHMARK(&start, benchmark_time, crc =
				cold_buffer_crc_wrapper(test_func.crc64_func_call, TEST_SEED, buf, test_size, len_pages));
		}

		perf_print_v(start, test_size, verbose);

		if (verbose) {
			printf("finish 0x%lx\n", crc);
		}
	}
}

static const bool default_verbose = false;
static const bool default_hot = false;
static const bool default_cold = false;
static const uint32_t default_benchmark_time = 3;
static const uint32_t default_start_size = 1024;
static const uint32_t default_end_size = 32768;
static const uint32_t default_stride = 1024;
static const uint32_t default_strides_before_doubling = 1;
static const uint32_t default_buffer_alignment = 4096;
static const uint32_t default_buffer_offset = 0;

void print_help()
{
	printf(	"Options listed below - boolean options set to true if specified. Other options expect a space and then a string specifying the value to take.\n\n"
			"-v, --verbose   : Make performance output more verbose (default %s)\n" \
			"--hot           : Test CRC functions with a hot buffer (i.e. repeatedly call the function on the same region of memory) (default %s)\n" \
			"--cold          : Test CRC functions with a cold buffer (i.e. use a distinct region of memory for each call to the function) (default %s)\n" \
			"-f, --functions : Select which CRC functions to test (default %s)\n" \
			"-t, --time      : Set how long in seconds to test the each CRC function for each combination of settings (default %u)\n" \
			"-s, --start     : The first test size in bytes (default %u)\n" \
			"-e, --end       : The maximum test size in bytes (default %u)\n" \
			"--stride        : The initial stride in bytes (default %u)\n" \
			"--stridesBeforeDoubling : The number of strides taken before the stride is doubled (default %u) - Note this can be set to -1 if no stride doubling is desired.\n" \
			"-a, --bufferAlignment : The tests will take place on chunks of data aligned to this parameter (default %u)\n" \
			"-o, --bufferOffset : The first byte of the buffers being tested will be offset by this parameter from the alignment (default %u)\n" \
			"-h, --help      : Print this message\n",
			default_verbose ? "true" : "false",
			default_hot ? "true" : "false",
			default_cold ? "true" : "false",
			test_func_selections[0].name,
			default_benchmark_time,
			default_start_size,
			default_end_size,
			default_stride,
			default_strides_before_doubling,
			default_buffer_alignment,
			default_buffer_offset);
}

int main(int argc, char *argv[])
{
	uint64_t buffer_alignment = default_buffer_alignment;
	uint64_t buffer_offset = default_buffer_offset;
	void *buf;
	bool verbose = default_verbose;
	bool hot = default_hot;
	bool cold = default_cold;
	test_func_selection_t func_selection = test_func_selections[0];
	uint32_t benchmark_time = default_benchmark_time;
	uint32_t start_size = default_start_size;
	uint32_t end_size = default_end_size;
	uint32_t stride = default_stride;
	uint32_t strides_before_doubling = default_strides_before_doubling;

	uint32_t arg_index = 1;
	uint32_t parsed_arg_count = 0;
	bool arg_parsing_failed = false;

	while (arg_index < argc) {
		if (!strcmp("-h", argv[arg_index]) || !strcmp("--help", argv[arg_index])) {
			print_help();
			exit(0);
		} else if (!strcmp("-v", argv[arg_index]) || !strcmp("--verbose", argv[arg_index])) {
			verbose = true;
		} else if (!strcmp("--hot", argv[arg_index])) {
			hot = true;
		} else if (!strcmp("--cold", argv[arg_index])) {
			cold = true;
		} else if (!strcmp("-f", argv[arg_index]) || !strcmp("--functions", argv[arg_index])) {
			bool selection_success = false;
			if (arg_index+1 < argc) {
				arg_index++;
				for (uint32_t selectionIndex = 0; selectionIndex < sizeof(test_func_selections) / sizeof(test_func_selections[0]); selectionIndex++)
				{
					if (!strcmp(test_func_selections[selectionIndex].name, argv[arg_index])) {
						func_selection = test_func_selections[selectionIndex];
						selection_success = true;
						break;
					}
				}
			}

			if (!selection_success) {
				printf("Error parsing argument %d - Expecting a function selection following -f or --functions, one of:\n", parsed_arg_count);
				printf("%s", test_func_selections[0].name);
				for (uint32_t selectionIndex = 1; selectionIndex < sizeof(test_func_selections) / sizeof(test_func_selections[0]); selectionIndex++)
				{
					printf(", %s", test_func_selections[selectionIndex].name);
				}
				printf("\n");
				arg_parsing_failed = true;
			}
		} else if (!strcmp("-t", argv[arg_index]) || !strcmp("--time", argv[arg_index])) {
			bool time_parsing_success = false;
			if (arg_index+1 < argc) {
				arg_index++;
				int parsed_time = atoi(argv[arg_index]);
				if(parsed_time > 0 && parsed_time <= 600) {
					benchmark_time = parsed_time;
					time_parsing_success = true;
				}
			}

			if(!time_parsing_success) {
				printf("Error parsing argument %d - Expecting a number of seconds between 1 and 600 following -t or --time\n", parsed_arg_count);
				arg_parsing_failed = true;
			}
		} else if (!strcmp("-s", argv[arg_index]) || !strcmp("--start", argv[arg_index])) {
			bool start_parsing_success = false;
			if (arg_index+1 < argc) {
				arg_index++;
				int parsed_size = atoi(argv[arg_index]);
				if(parsed_size > 0 && parsed_size <= 1<<29) {
					start_size = parsed_size;
					start_parsing_success = true;
				}
			}

			if(!start_parsing_success) {
				printf("Error parsing argument %d - Expecting a number of bytes between 1 and 1<<29 following -s or --start\n", parsed_arg_count);
				arg_parsing_failed = true;
			}
		} else if (!strcmp("-e", argv[arg_index]) || !strcmp("--end", argv[arg_index])) {
			bool end_parsing_success = false;
			if (arg_index+1 < argc) {
				arg_index++;
				int parsed_size = atoi(argv[arg_index]);
				if(parsed_size > 0 && parsed_size <= 1<<29) {
					end_size = parsed_size;
					end_parsing_success = true;
				}
			}

			if(!end_parsing_success) {
				printf("Error parsing argument %d - Expecting a number of bytes between 1 and 1<<29 following -e or --end\n", parsed_arg_count);
				arg_parsing_failed = true;
			}
		} else if (!strcmp("--stride", argv[arg_index])) {
			bool stride_parsing_success = false;
			if (arg_index+1 < argc) {
				arg_index++;
				int parsed_stride = atoi(argv[arg_index]);
				if(parsed_stride > 0 && parsed_stride <= 1<<28) {
					stride = parsed_stride;
					stride_parsing_success = true;
				}
			}

			if(!stride_parsing_success) {
				printf("Error parsing argument %d - Expecting a number of bytes between 1 and 1<<28 following --stride\n", parsed_arg_count);
				arg_parsing_failed = true;
			}
		} else if (!strcmp("--stridesBeforeDoubling", argv[arg_index])) {
			bool stride_parsing_success = false;
			if (arg_index+1 < argc) {
				arg_index++;
				int parsed_strides_before_doubling = atoi(argv[arg_index]);
				if(parsed_strides_before_doubling > 0 && parsed_strides_before_doubling <= 1<<10) {
					strides_before_doubling = parsed_strides_before_doubling;
					stride_parsing_success = true;
				} else if (parsed_strides_before_doubling == -1) {
					strides_before_doubling = UINT32_MAX;
					stride_parsing_success = true;
				}
			}

			if(!stride_parsing_success) {
				printf("Error parsing argument %d - Expecting a number of strides between 1 and 1<<10, or -1 following --stridesBeforeDoubling\n", parsed_arg_count);
				arg_parsing_failed = true;
			}
		} else if (!strcmp("-a", argv[arg_index]) || !strcmp("--bufferAlignment", argv[arg_index])) {
			bool alignment_parsing_success = false;
			if (arg_index+1 < argc) {
				arg_index++;
				int parsed_alignment = atoi(argv[arg_index]);
				if(parsed_alignment > 0 && parsed_alignment <= 1<<24) {
					buffer_alignment = parsed_alignment;
					alignment_parsing_success = true;
				}
			}

			if(!alignment_parsing_success) {
				printf("Error parsing argument %d - Expecting a number of bytes between 1 and 1<<24 following -a or --bufferAlignment\n", parsed_arg_count);
				arg_parsing_failed = true;
			}
		} else if (!strcmp("-o", argv[arg_index]) || !strcmp("--bufferOffset", argv[arg_index])) {
			bool offset_parsing_success = false;
			if (arg_index+1 < argc) {
				arg_index++;
				int parsed_offset = atoi(argv[arg_index]);
				if(parsed_offset >= 0 && parsed_offset < 1<<24) {
					buffer_offset = parsed_offset;
					offset_parsing_success = true;
				}
			}

			if(!offset_parsing_success) {
				printf("Error parsing argument %d - Expecting a number of bytes between 0 and (1<<24)-1 following -o or --bufferOffset\n", parsed_arg_count);
				arg_parsing_failed = true;
			}
		} else {
			printf("Error parsing argument %d - Unexpected format %s\n", parsed_arg_count, argv[arg_index]);
			arg_parsing_failed = true;
		}

		parsed_arg_count++;
		arg_index++;
	}

	if(arg_parsing_failed)
	{
		print_help();
		exit(1);
	}

	if (!hot && !cold) {
		printf("Neither --hot nor --cold arguments specified - testing both hot and cold\n");
		hot = true;
		cold = true;
	}

	if (posix_memalign(&buf, buffer_alignment, BUFFER_LEN+buffer_offset)) {
		printf("alloc error: Fail");
		return -1;
	}
	memset(buf, (char)TEST_SEED, BUFFER_LEN);

	uint32_t remaining_strides = strides_before_doubling;
	uint32_t test_stride = stride;
	for(uint32_t test_size = start_size; test_size <= end_size; ) {
		if (hot)
		{
			run_tests(func_selection, ((uint8_t *) buf)+buffer_offset, test_size, benchmark_time, true, verbose);
		}
		if (cold)
		{
			run_tests(func_selection, ((uint8_t *) buf)+buffer_offset, test_size, benchmark_time, false, verbose);
		}

		test_size += test_stride;

		remaining_strides--;
		if(remaining_strides == 0) {
			remaining_strides = strides_before_doubling;
			test_stride <<= 1;
		}
	}

	return 0;
}
