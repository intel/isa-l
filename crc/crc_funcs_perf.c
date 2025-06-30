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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <ctype.h>
#include <inttypes.h>

#ifdef _WIN32
#include <Windows.h>
#include <io.h>
#define strcasecmp _stricmp
#define ssize_t    long
#else
#include <sys/types.h>
#endif

#include "crc.h"
#include "crc64.h"
#include "test.h"

#define DEFAULT_TEST_LEN 8 * 1024

#ifndef TEST_SEED
#define TEST_SEED 0x1234
#endif

// Define CRC algorithm types
typedef enum {
        // CRC32 types
        CRC32_GZIP_REFL = 0,
        CRC32_IEEE = 1,
        CRC32_ISCSI = 2,
        // CRC16 types
        CRC16_T10DIF = 3,
        CRC16_T10DIF_COPY = 4,
        // CRC64 types
        CRC64_ECMA_REFL = 5,
        CRC64_ECMA_NORM = 6,
        CRC64_ISO_REFL = 7,
        CRC64_ISO_NORM = 8,
        CRC64_JONES_REFL = 9,
        CRC64_JONES_NORM = 10,
        CRC64_ROCKSOFT_REFL = 11,
        CRC64_ROCKSOFT_NORM = 12,
        // Category groups
        CRC_ALL = 100,
        CRC32_ALL = 101,
        CRC16_ALL = 102,
        CRC64_ALL = 103
} crc_type_t;

// Define arrays of CRC types for each category group
static const crc_type_t CRC32_TYPES[] = { CRC32_GZIP_REFL, CRC32_IEEE, CRC32_ISCSI };
static const size_t CRC32_TYPES_LEN = 3;

static const crc_type_t CRC16_TYPES[] = { CRC16_T10DIF, CRC16_T10DIF_COPY };
static const size_t CRC16_TYPES_LEN = 2;

static const crc_type_t CRC64_TYPES[] = { CRC64_ECMA_REFL,     CRC64_ECMA_NORM,    CRC64_ISO_REFL,
                                          CRC64_ISO_NORM,      CRC64_JONES_REFL,   CRC64_JONES_NORM,
                                          CRC64_ROCKSOFT_REFL, CRC64_ROCKSOFT_NORM };
static const size_t CRC64_TYPES_LEN = 8;

#define COLD_CACHE_TEST_MEM (1024 * 1024 * 1024)
#define COLD_CACHE_MIN_LEN  (1024)

// Define function pointer types for CRC functions with standard parameter format (seed, buffer,
// len)
typedef uint32_t (*crc32_func_t)(uint32_t seed, const uint8_t *buf, uint64_t len);
typedef uint16_t (*crc16_func_t)(uint16_t seed, const uint8_t *buf, uint64_t len);
typedef uint64_t (*crc64_func_t)(uint64_t seed, const uint8_t *buf, uint64_t len);

// Result type enumeration
typedef enum { RESULT_CRC32 = 0, RESULT_CRC16, RESULT_CRC64 } crc_result_type_t;

// Structure to define CRC function tables with standard parameter signature
typedef struct {
        crc_type_t type;               // CRC type ID
        const char *name;              // Function name for output
        void *func;                    // Regular optimized function
        void *func_base;               // Base (non-optimized) function
        crc_result_type_t result_type; // Type of result value
} crc_func_info_t;

// Wrapper functions for CRC functions with non-standard parameter order
// ISCSI CRC wrapper that adapts (buffer, len, seed) to standard (seed, buffer, len) format
static uint32_t
crc32_iscsi_wrapped(const uint32_t seed, const uint8_t *buf, const uint64_t len)
{
        // Cast to match the expected function signature
        return crc32_iscsi((unsigned char *) buf, (int) len, seed);
}

static uint32_t
crc32_iscsi_base_wrapped(const uint32_t seed, const uint8_t *buf, const uint64_t len)
{
        // Cast to match the expected function signature
        return crc32_iscsi_base((unsigned char *) buf, (int) len, seed);
}

// Table of CRC functions with standard parameter format (seed, buffer, len)
static const crc_func_info_t CRC_FUNCS_TABLE[] = {
        // CRC32 functions
        { CRC32_GZIP_REFL, "crc32_gzip_refl", (void *) crc32_gzip_refl,
          (void *) crc32_gzip_refl_base, RESULT_CRC32 },
        { CRC32_IEEE, "crc32_ieee", (void *) crc32_ieee, (void *) crc32_ieee_base, RESULT_CRC32 },
        { CRC32_ISCSI, "crc32_iscsi", (void *) crc32_iscsi_wrapped,
          (void *) crc32_iscsi_base_wrapped, RESULT_CRC32 },
        // CRC16 functions
        { CRC16_T10DIF, "crc16_t10dif", (void *) crc16_t10dif, (void *) crc16_t10dif_base,
          RESULT_CRC16 },
        // CRC64 functions
        { CRC64_ECMA_REFL, "crc64_ecma_refl", (void *) crc64_ecma_refl,
          (void *) crc64_ecma_refl_base, RESULT_CRC64 },
        { CRC64_ECMA_NORM, "crc64_ecma_norm", (void *) crc64_ecma_norm,
          (void *) crc64_ecma_norm_base, RESULT_CRC64 },
        { CRC64_ISO_REFL, "crc64_iso_refl", (void *) crc64_iso_refl, (void *) crc64_iso_refl_base,
          RESULT_CRC64 },
        { CRC64_ISO_NORM, "crc64_iso_norm", (void *) crc64_iso_norm, (void *) crc64_iso_norm_base,
          RESULT_CRC64 },
        { CRC64_JONES_REFL, "crc64_jones_refl", (void *) crc64_jones_refl,
          (void *) crc64_jones_refl_base, RESULT_CRC64 },
        { CRC64_JONES_NORM, "crc64_jones_norm", (void *) crc64_jones_norm,
          (void *) crc64_jones_norm_base, RESULT_CRC64 },
        { CRC64_ROCKSOFT_REFL, "crc64_rocksoft_refl", (void *) crc64_rocksoft_refl,
          (void *) crc64_rocksoft_refl_base, RESULT_CRC64 },
        { CRC64_ROCKSOFT_NORM, "crc64_rocksoft_norm", (void *) crc64_rocksoft_norm,
          (void *) crc64_rocksoft_norm_base, RESULT_CRC64 },
};

// Number of entries in the CRC function table
static const size_t CRC_FUNCS_TABLE_SIZE = sizeof(CRC_FUNCS_TABLE) / sizeof(CRC_FUNCS_TABLE[0]);

// Helper function to run benchmark for standard CRC function
static void
run_standard_crc_benchmark(struct perf *start, const crc_func_info_t *func_info,
                           const int run_base_version, const uint8_t *buffer, const size_t len,
                           const int csv_output, const int use_cold_cache, uint8_t **buffer_list,
                           const size_t num_buffers)
{
        const char *function_suffix = run_base_version ? "_base" : "";
        const char *crc_type_str = func_info->name;

        union {
                crc32_func_t crc32;
                crc16_func_t crc16;
                crc64_func_t crc64;
                void *ptr;
        } func;

        // Get appropriate function pointer based on base version flag
        func.ptr = run_base_version ? func_info->func_base : func_info->func;

        // Execute benchmark based on result type with appropriate benchmark macro
        if (use_cold_cache && buffer_list) {
                size_t current_buffer_idx = 0;

                switch (func_info->result_type) {
                case RESULT_CRC32:
                        BENCHMARK_COLD(start, BENCHMARK_TIME,
                                       current_buffer_idx = (current_buffer_idx + 1) % num_buffers,
                                       func.crc32(TEST_SEED, buffer_list[current_buffer_idx], len));
                        break;
                case RESULT_CRC16:
                        BENCHMARK_COLD(start, BENCHMARK_TIME,
                                       current_buffer_idx = (current_buffer_idx + 1) % num_buffers,
                                       func.crc16(TEST_SEED, buffer_list[current_buffer_idx], len));
                        break;
                case RESULT_CRC64:
                        BENCHMARK_COLD(start, BENCHMARK_TIME,
                                       current_buffer_idx = (current_buffer_idx + 1) % num_buffers,
                                       func.crc64(TEST_SEED, buffer_list[current_buffer_idx], len));
                        break;
                }
        } else {
                // Standard warm-cache benchmark
                switch (func_info->result_type) {
                case RESULT_CRC32:
                        BENCHMARK(start, BENCHMARK_TIME, func.crc32(TEST_SEED, buffer, len));
                        break;
                case RESULT_CRC16:
                        BENCHMARK(start, BENCHMARK_TIME, func.crc16(TEST_SEED, buffer, len));
                        break;
                case RESULT_CRC64:
                        BENCHMARK(start, BENCHMARK_TIME, func.crc64(TEST_SEED, buffer, len));
                        break;
                }
        }

        // Print results in appropriate format
        if (csv_output) {
#ifdef USE_RDTSC
                // When USE_RDTSC is defined, report total cycles per buffer
                double cycles = (double) get_base_elapsed(start);
                double cycles_per_buffer = cycles / (double) start->iterations;
                printf("%s%s,%zu,%.0f\n", crc_type_str, function_suffix, len, cycles_per_buffer);
#else
                // Calculate throughput in MB/s
                double time_elapsed = get_time_elapsed(start);
                long long total_units = start->iterations * (long long) len;
                double throughput = ((double) total_units) / (1000000 * time_elapsed);
                printf("%s%s,%zu,%.2f\n", crc_type_str, function_suffix, len, throughput);
#endif
        } else {
                printf("%s%s : ", crc_type_str, function_suffix);
                perf_print(*start, (long long) len);
        }
}

// Function to run a specific CRC benchmark
static void
run_benchmark(const void *buf, const size_t len, const crc_type_t type, const int run_base_version,
              const int csv_output, const int use_cold_cache)
{
        struct perf start;
        perf_init(&start);
        uint8_t *buffer = (uint8_t *) buf; // Proper casting to match function signatures
        const char *crc_type_str = "";
        const char *function_suffix = run_base_version ? "_base" : "";

        // For cold cache benchmarking
        uint8_t **buffer_list = NULL;
        size_t num_buffers = 0;

        // Create list of random buffer address within the large memory space already allocated for
        // cold cache tests
        if (use_cold_cache) {
                num_buffers = COLD_CACHE_TEST_MEM / len;

                buffer_list = (uint8_t **) malloc(num_buffers * sizeof(uint8_t *));
                if (!buffer_list) {
                        printf("Failed to allocate memory for cold cache buffers\n");
                        return;
                }

                for (size_t i = 0; i < num_buffers; i++)
                        buffer_list[i] = buffer + len * (rand() % num_buffers);
        }

        // First check if this is a standard CRC function that can use the table
        for (size_t i = 0; i < CRC_FUNCS_TABLE_SIZE; i++) {
                if (CRC_FUNCS_TABLE[i].type == type) {
                        run_standard_crc_benchmark(&start, &CRC_FUNCS_TABLE[i], run_base_version,
                                                   buffer, len, csv_output, use_cold_cache,
                                                   buffer_list, num_buffers);
                        if (buffer_list)
                                free(buffer_list);
                        return;
                }
        }

        if (buffer_list)
                free(buffer_list);

        // Handle special cases that don't fit the standard parameter format
        switch (type) {
        case CRC16_T10DIF_COPY:
                crc_type_str = "crc16_t10dif_copy";
                uint8_t *dst_buf = malloc(len);
                if (!dst_buf) {
                        printf("Failed to allocate destination buffer for crc16_t10dif_copy\n");
                        return;
                }
                for (size_t i = 0; i < len; i++)
                        dst_buf[i] = (uint8_t) rand();

                if (run_base_version) {
                        BENCHMARK(&start, BENCHMARK_TIME,
                                  crc16_t10dif_copy_base(TEST_SEED, dst_buf, buffer, len));
                } else {
                        BENCHMARK(&start, BENCHMARK_TIME,
                                  crc16_t10dif_copy(TEST_SEED, dst_buf, buffer, len));
                }
                free(dst_buf);
                break;
        default:
                printf("Error: CRC type %d not found or not implemented\n", type);
                return;
        }

        if (csv_output) {
#ifdef USE_RDTSC
                // When USE_RDTSC is defined, report total cycles per buffer
                double cycles = (double) get_base_elapsed(&start);
                double cycles_per_buffer = cycles / (double) start.iterations;
                printf("%s%s,%zu,%.2f\n", crc_type_str, function_suffix, len, cycles_per_buffer);
#else
                // Calculate throughput in MB/s
                double time_elapsed = get_time_elapsed(&start);
                long long total_units = start.iterations * (long long) len;
                double throughput = ((double) total_units) / (1000000 * time_elapsed);
                printf("%s%s,%zu,%.2f\n", crc_type_str, function_suffix, len, throughput);
#endif
        } else {
                printf("%s%s : ", crc_type_str, function_suffix);
                perf_print(start, (long long) len);
        }
}

static void
print_crc_types(void)
{
        printf("Available CRC types:\n");
        printf("  CRC32 types:\n");
        printf("    0 or 'gzip refl'      - GZIP Reflected CRC32\n");
        printf("    1 or 'ieee'           - IEEE CRC32\n");
        printf("    2 or 'iscsi'          - iSCSI CRC32\n");
        printf("  CRC16 types:\n");
        printf("    3 or 't10dif'         - T10DIF CRC16\n");
        printf("    4 or 't10dif_copy'    - T10DIF CRC16 with copy\n");
        printf("  CRC64 types:\n");
        printf("    5 or 'ecma_refl'      - ECMA Reflected CRC64\n");
        printf("    6 or 'ecma_norm'      - ECMA Normal CRC64\n");
        printf("    7 or 'iso_refl'       - ISO Reflected CRC64\n");
        printf("    8 or 'iso_norm'       - ISO Normal CRC64\n");
        printf("    9 or 'jones_refl'     - Jones Reflected CRC64\n");
        printf("    10 or 'jones_norm'    - Jones Normal CRC64\n");
        printf("    11 or 'rocksoft_refl' - Rocksoft Reflected CRC64\n");
        printf("    12 or 'rocksoft_norm' - Rocksoft Normal CRC64\n");
        printf("  Groups:\n");
        printf("    100 or 'all'          - All CRC types (default)\n");
        printf("    101 or 'crc32_all'    - All CRC32 types\n");
        printf("    102 or 'crc16_all'    - All CRC16 types\n");
        printf("    103 or 'crc64_all'    - All CRC64 types\n");
}

static void
print_help(void)
{
        printf("Usage: crc_funcs_perf [options]\n");
        printf("  -h, --help          Show this help message\n");
        printf("  -t, --type TYPE     CRC type to benchmark\n");
        print_crc_types();
        printf("                            If no value is provided, lists available types and "
               "exits\n");
        printf("  -b, --base          Include base (non-optimized) versions in benchmark\n");
        printf("  -r, --range MIN:STEP:MAX  Run tests with buffer sizes from MIN to MAX in STEP "
               "increments\n");
        printf("                      If STEP starts with '*', it's treated as a multiplier\n");
        printf("                      Size values can include K (KB) or M (MB) suffix\n");
        printf("                      Examples:\n");
        printf("                        --range=1K:1K:16K (1KB to 16KB in 1KB steps)\n");
        printf("                        --range=1K:*2:16K (1KB, 2KB, 4KB, 8KB, 16KB)\n");
        printf("                        --range=1M:1M:32M (1MB to 32MB in 1MB steps)\n");
        printf("  -s, --sizes SIZE1,SIZE2,...  Run tests with specified comma-separated buffer "
               "sizes\n");
        printf("                      Example: --sizes=1024,4096,8192,16384\n");
        printf("                      Size values can include K (KB) or M (MB) suffix\n");
        printf("                      Example: --sizes=1K,4K,8K,16K or --sizes=1M,32M\n");
        printf("                      If no size option is provided, default size (%d) is used\n",
               DEFAULT_TEST_LEN);
        printf("  -c, --csv           Output results in CSV format\n");
        printf("      --cold          Use cold cache for benchmarks (buffer not in cache, "
               "t10dif_copy not supported)\n");
}

// Helper function to parse string input for CRC type
// Define a struct to map string names to CRC types
typedef struct {
        const char *name;
        crc_type_t type;
} crc_name_map_t;

// Table of CRC name to type mappings
static const crc_name_map_t CRC_NAME_MAP[] = {
        // CRC32 types
        { "gzip refl", CRC32_GZIP_REFL },
        { "gzip", CRC32_GZIP_REFL },
        { "gzip_refl", CRC32_GZIP_REFL },
        { "ieee", CRC32_IEEE },
        { "iscsi", CRC32_ISCSI },
        // CRC16 types
        { "t10dif", CRC16_T10DIF },
        { "t10dif_copy", CRC16_T10DIF_COPY },
        // CRC64 types
        { "ecma_refl", CRC64_ECMA_REFL },
        { "ecma_norm", CRC64_ECMA_NORM },
        { "iso_refl", CRC64_ISO_REFL },
        { "iso_norm", CRC64_ISO_NORM },
        { "jones_refl", CRC64_JONES_REFL },
        { "jones_norm", CRC64_JONES_NORM },
        { "rocksoft_refl", CRC64_ROCKSOFT_REFL },
        { "rocksoft_norm", CRC64_ROCKSOFT_NORM },
        // Group types
        { "all", CRC_ALL },
        { "crc32_all", CRC32_ALL },
        { "crc16_all", CRC16_ALL },
        { "crc64_all", CRC64_ALL }
};

static const size_t CRC_NAME_MAP_LEN = sizeof(CRC_NAME_MAP) / sizeof(CRC_NAME_MAP[0]);

static crc_type_t
parse_crc_type(const char *type_str)
{
        // Default to CRC_ALL if no type provided
        if (!type_str)
                return CRC_ALL;

        // Check for numeric CRC type
        char *endptr;
        const long val = strtol(type_str, &endptr, 10);
        if (*type_str != '\0' && *endptr == '\0' && val >= 0 && val <= 103)
                return (crc_type_t) val;

        // Loop through name-to-type mapping table
        for (size_t i = 0; i < CRC_NAME_MAP_LEN; i++)
                if (strcasecmp(type_str, CRC_NAME_MAP[i].name) == 0)
                        return CRC_NAME_MAP[i].type;

        return CRC_ALL; // Default if not found
}

// Helper function to parse size values with optional K (KB) or M (MB) suffix
static size_t
parse_size_value(const char *const size_str)
{
        size_t size_val;
        char *endptr;
        size_t multiplier = 1;
        char *str_copy = strdup(size_str);

        // Check for size suffixes (K for KB, M for MB)
        const int len = strlen(str_copy);
        if (len > 0) {
                // Convert to uppercase for case-insensitive comparison
                char last_char = toupper(str_copy[len - 1]);

                if (last_char == 'K') {
                        multiplier = 1024;
                        str_copy[len - 1] = '\0'; // Remove the suffix
                } else if (last_char == 'M') {
                        multiplier = 1024 * 1024;
                        str_copy[len - 1] = '\0'; // Remove the suffix
                }
        }

        // Convert the numeric part
        size_val = strtoul(str_copy, &endptr, 10);

        // Check if the conversion was successful
        if (*endptr != '\0' && *endptr != 'K' && *endptr != 'M' && *endptr != 'k' &&
            *endptr != 'm') {
                // Invalid characters in the string
                free(str_copy);
                return 0;
        }

        free(str_copy);
        return size_val * multiplier;
}

#define MAX_SIZE_COUNT 32 // Maximum number of buffer sizes that can be specified

// Helper function to run benchmarks for a specific CRC type (range version)
static void
run_crc_type_range(const crc_type_t type, const void *buf, const size_t min_len,
                   const size_t max_len, const int is_multiplicative, const size_t abs_step,
                   const int csv_output, const int include_base, const int use_cold_cache)
{
        static const char *const type_names[] = {
                "GZIP Reflected CRC32",  "IEEE CRC32",          "iSCSI CRC32",
                "T10DIF CRC16",          "T10DIF Copy CRC16",   "ECMA Reflected CRC64",
                "ECMA Normal CRC64",     "ISO Reflected CRC64", "ISO Normal CRC64",
                "Jones Reflected CRC64", "Jones Normal CRC64",  "Rocksoft Reflected CRC64",
                "Rocksoft Normal CRC64"
        };

        if (!csv_output && type < CRC_ALL) {
                printf("\n%s:\n", type_names[type]);
        }

        for (size_t len = min_len; len <= max_len;) {
                if (!csv_output) {
                        printf("\n  Buffer size: %zu bytes\n", len);
                }

                run_benchmark(buf, len, type, 0, csv_output, use_cold_cache);
                if (include_base)
                        run_benchmark(buf, len, type, 1, csv_output, use_cold_cache);

                // Update length based on step type
                if (is_multiplicative) {
                        len *= abs_step;
                } else {
                        len += abs_step;
                }
        }
}

// Helper function to run benchmarks for a specific CRC type with range of sizes
static void
benchmark_crc_type_range(const crc_type_t crc_type, const void *buf, const size_t min_len,
                         const size_t max_len, const ssize_t step_len, const int csv_output,
                         const int include_base, const int use_cold_cache)
{
        const int is_multiplicative = (step_len < 0);
        const size_t abs_step = is_multiplicative ? -step_len : step_len;

        // Run selected benchmarks based on crc_type
        if (crc_type == CRC_ALL || crc_type == CRC32_ALL) {
                if (!csv_output)
                        printf("\n=========== CRC32 FUNCTIONS ===========\n");

                for (size_t i = 0; i < CRC32_TYPES_LEN; i++) {
                        run_crc_type_range(CRC32_TYPES[i], buf, min_len, max_len, is_multiplicative,
                                           abs_step, csv_output, include_base, use_cold_cache);
                }
        }

        if (crc_type == CRC_ALL || crc_type == CRC16_ALL) {
                if (!csv_output)
                        printf("\n=========== CRC16 FUNCTIONS ===========\n");

                for (size_t i = 0; i < CRC16_TYPES_LEN; i++) {
                        run_crc_type_range(CRC16_TYPES[i], buf, min_len, max_len, is_multiplicative,
                                           abs_step, csv_output, include_base, use_cold_cache);
                }
        }

        if (crc_type == CRC_ALL || crc_type == CRC64_ALL) {
                if (!csv_output)
                        printf("\n=========== CRC64 FUNCTIONS ===========\n");

                for (size_t i = 0; i < CRC64_TYPES_LEN; i++) {
                        run_crc_type_range(CRC64_TYPES[i], buf, min_len, max_len, is_multiplicative,
                                           abs_step, csv_output, include_base, use_cold_cache);
                }
        }

        // If none of the above, run just the specific CRC type
        if (crc_type != CRC_ALL && crc_type != CRC32_ALL && crc_type != CRC16_ALL &&
            crc_type != CRC64_ALL) {
                run_crc_type_range(crc_type, buf, min_len, max_len, is_multiplicative, abs_step,
                                   csv_output, include_base, use_cold_cache);
        }
}

// Helper function to run benchmarks for a specific CRC type with list of sizes
static void
run_crc_type_size_list(const crc_type_t type, const void *buf, const size_t *size_list,
                       const int size_count, const int csv_output, const int include_base,
                       const int use_cold_cache)
{
        static const char *const type_names[] = {
                "GZIP Reflected CRC32",  "IEEE CRC32",          "iSCSI CRC32",
                "T10DIF CRC16",          "T10DIF Copy CRC16",   "ECMA Reflected CRC64",
                "ECMA Normal CRC64",     "ISO Reflected CRC64", "ISO Normal CRC64",
                "Jones Reflected CRC64", "Jones Normal CRC64",  "Rocksoft Reflected CRC64",
                "Rocksoft Normal CRC64"
        };

        if (!csv_output && type < CRC_ALL) {
                printf("\n%s:\n", type_names[type]);
        }

        for (int i = 0; i < size_count; i++) {
                if (!csv_output) {
                        printf("\n  Buffer size: %zu bytes\n", size_list[i]);
                }

                run_benchmark(buf, size_list[i], type, 0, csv_output, use_cold_cache);
                if (include_base)
                        run_benchmark(buf, size_list[i], type, 1, csv_output, use_cold_cache);
        }
}

// Helper function to run benchmarks for a specific CRC type with default size
static void
run_crc_type_default(const crc_type_t type, const void *buf, const size_t test_len,
                     const int csv_output, const int include_base, const int use_cold_cache)
{
        static const char *const type_names[] = {
                "GZIP Reflected CRC32",  "IEEE CRC32",          "iSCSI CRC32",
                "T10DIF CRC16",          "T10DIF Copy CRC16",   "ECMA Reflected CRC64",
                "ECMA Normal CRC64",     "ISO Reflected CRC64", "ISO Normal CRC64",
                "Jones Reflected CRC64", "Jones Normal CRC64",  "Rocksoft Reflected CRC64",
                "Rocksoft Normal CRC64"
        };

        if (!csv_output && type < CRC_ALL) {
                printf("\n%s:\n", type_names[type]);
        }

        if (!csv_output) {
                printf("\n  Buffer size: %zu bytes\n", test_len);
        }

        run_benchmark(buf, test_len, type, 0, csv_output, use_cold_cache);
        if (include_base)
                run_benchmark(buf, test_len, type, 1, csv_output, use_cold_cache);
}

// Helper function to run benchmarks for a specific CRC type with default size
static void
benchmark_crc_type_default(const crc_type_t crc_type, const void *buf, const size_t test_len,
                           const int csv_output, const int include_base, const int use_cold_cache)
{
        // Run selected benchmarks based on crc_type
        if (crc_type == CRC_ALL || crc_type == CRC32_ALL) {
                if (!csv_output)
                        printf("\n=========== CRC32 FUNCTIONS ===========\n");

                for (size_t i = 0; i < CRC32_TYPES_LEN; i++) {
                        run_crc_type_default(CRC32_TYPES[i], buf, test_len, csv_output,
                                             include_base, use_cold_cache);
                }
        }

        if (crc_type == CRC_ALL || crc_type == CRC16_ALL) {
                if (!csv_output)
                        printf("\n=========== CRC16 FUNCTIONS ===========\n");

                for (size_t i = 0; i < CRC16_TYPES_LEN; i++) {
                        run_crc_type_default(CRC16_TYPES[i], buf, test_len, csv_output,
                                             include_base, use_cold_cache);
                }
        }

        if (crc_type == CRC_ALL || crc_type == CRC64_ALL) {
                if (!csv_output)
                        printf("\n=========== CRC64 FUNCTIONS ===========\n");

                for (size_t i = 0; i < CRC64_TYPES_LEN; i++) {
                        run_crc_type_default(CRC64_TYPES[i], buf, test_len, csv_output,
                                             include_base, use_cold_cache);
                }
        }

        // If none of the above, run just the specific CRC type
        if (crc_type != CRC_ALL && crc_type != CRC32_ALL && crc_type != CRC16_ALL &&
            crc_type != CRC64_ALL) {
                run_crc_type_default(crc_type, buf, test_len, csv_output, include_base,
                                     use_cold_cache);
        }
}

// Helper function to run benchmarks for a specific CRC type with list of sizes
static void
benchmark_crc_type_size_list(const crc_type_t crc_type, const void *buf, const size_t *size_list,
                             const int size_count, const int csv_output, const int include_base,
                             const int use_cold_cache)
{
        // Run selected benchmarks based on crc_type
        if (crc_type == CRC_ALL || crc_type == CRC32_ALL) {
                if (!csv_output)
                        printf("\n=========== CRC32 FUNCTIONS ===========\n");

                for (size_t i = 0; i < CRC32_TYPES_LEN; i++) {
                        run_crc_type_size_list(CRC32_TYPES[i], buf, size_list, size_count,
                                               csv_output, include_base, use_cold_cache);
                }
        }

        if (crc_type == CRC_ALL || crc_type == CRC16_ALL) {
                if (!csv_output)
                        printf("\n=========== CRC16 FUNCTIONS ===========\n");

                for (size_t i = 0; i < CRC16_TYPES_LEN; i++) {
                        run_crc_type_size_list(CRC16_TYPES[i], buf, size_list, size_count,
                                               csv_output, include_base, use_cold_cache);
                }
        }

        if (crc_type == CRC_ALL || crc_type == CRC64_ALL) {
                if (!csv_output)
                        printf("\n=========== CRC64 FUNCTIONS ===========\n");

                for (size_t i = 0; i < CRC64_TYPES_LEN; i++) {
                        run_crc_type_size_list(CRC64_TYPES[i], buf, size_list, size_count,
                                               csv_output, include_base, use_cold_cache);
                }
        }

        // If none of the above, run just the specific CRC type
        if (crc_type != CRC_ALL && crc_type != CRC32_ALL && crc_type != CRC16_ALL &&
            crc_type != CRC64_ALL) {
                run_crc_type_size_list(crc_type, buf, size_list, size_count, csv_output,
                                       include_base, use_cold_cache);
        }
}

int
main(const int argc, char *const argv[])
{
        size_t test_len = DEFAULT_TEST_LEN;
        int csv_output = 0;            // Default to human-readable output
        int include_base = 0;          // Don't include base versions by default
        int use_cold_cache = 0;        // Don't use cold cache by default
        crc_type_t crc_type = CRC_ALL; // Default to all CRC types
        void *buf = NULL;

        // Size range options
        size_t min_len = DEFAULT_TEST_LEN;
        size_t max_len = DEFAULT_TEST_LEN;
        int use_range = 0;
        ssize_t step_len = 0; // Using signed type to indicate additive vs multiplicative

        // For comma-separated sizes option
        int use_size_list = 0;
        size_t size_list[MAX_SIZE_COUNT];
        int size_count = 0;

        // Manual parsing of command-line arguments
        for (int i = 1; i < argc; i++) {
                // Help option
                if (strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "--help") == 0) {
                        print_help();
                        return 0;
                }

                // CSV output option
                else if (strcmp(argv[i], "-c") == 0 || strcmp(argv[i], "--csv") == 0) {
                        csv_output = 1;
                }

                // Base option
                else if (strcmp(argv[i], "-b") == 0 || strcmp(argv[i], "--base") == 0) {
                        include_base = 1;
                }

                // Cold cache option
                else if (strcmp(argv[i], "--cold") == 0) {
                        use_cold_cache = 1;
                        printf("Cold cache option enabled\n");
                }

                // Type option
                else if ((strcmp(argv[i], "-t") == 0 || strcmp(argv[i], "--type") == 0)) {
                        if (i + 1 < argc && argv[i + 1][0] != '-') {
                                // Option has an argument
                                i++; // Move to the argument
                                const char *type_arg = argv[i];
                                crc_type = parse_crc_type(type_arg);
                                // Check if value is outside valid range for individual CRC types or
                                // group types
                                if ((crc_type > CRC64_ROCKSOFT_NORM) &&
                                    (crc_type < CRC_ALL || crc_type > CRC64_ALL)) {
                                        printf("Invalid CRC type: '%s'. Using default (all).\n",
                                               type_arg);
                                        crc_type = CRC_ALL;
                                }
                        } else {
                                // No argument provided, show available types
                                print_crc_types();
                                return 0;
                        }
                }

                // Range option
                else if ((strcmp(argv[i], "-r") == 0 || strcmp(argv[i], "--range") == 0)) {
                        if (i + 1 < argc && argv[i + 1][0] != '-') {
                                // Option has an argument
                                i++; // Move to the argument
                                const char *range_arg_str = argv[i];

                                // Range specified, parse it
                                char *range_arg = strdup(range_arg_str);
                                char *min_str = strtok(range_arg, ":");
                                char *step_str = strtok(NULL, ":");
                                char *max_str = strtok(NULL, ":");

                                if (min_str && step_str && max_str) {
                                        min_len = parse_size_value(min_str);
                                        max_len = parse_size_value(max_str);

                                        // Check for multiplicative step (starts with '*')
                                        int step_is_multiplicative = 0;
                                        if (step_str[0] == '*') {
                                                step_is_multiplicative = 1;
                                                if (strlen(step_str) > 1) {
                                                        step_len = parse_size_value(
                                                                step_str + 1); // Skip the '*'
                                                } else {
                                                        step_len = 0; // Invalid step
                                                }
                                        } else {
                                                step_len = parse_size_value(step_str);
                                        }

                                        if (min_len > 0 && max_len >= min_len && step_len > 0) {
                                                use_range = 1;
                                                // Set test_len to the largest value for memory
                                                // allocation
                                                test_len = max_len;
                                                // Store whether step is multiplicative
                                                if (step_is_multiplicative) {
                                                        // Mark as multiplicative by making step
                                                        // negative We'll use abs(step_len) as the
                                                        // multiplier
                                                        step_len = -step_len;
                                                }
                                        } else {
                                                printf("Invalid range values. Ensure MIN > 0, MAX "
                                                       ">= MIN, "
                                                       "and STEP > 0.\n");
                                        }
                                } else {
                                        printf("Invalid range format. Use MIN:STEP:MAX (e.g., "
                                               "1024:1024:16384 or 1024:*2:16384)\n");
                                }

                                free(range_arg);
                        } else {
                                printf("Option --range requires an argument.\n");
                                print_help();
                                return 1;
                        }
                }

                // Sizes option
                else if ((strcmp(argv[i], "-s") == 0 || strcmp(argv[i], "--sizes") == 0)) {
                        if (i + 1 < argc && argv[i + 1][0] != '-') {
                                // Option has an argument
                                i++; // Move to the argument
                                const char *sizes_arg_str = argv[i];

                                // Sizes specified, parse them as a list
                                char *sizes_arg = strdup(sizes_arg_str);
                                char *size_str = strtok(sizes_arg, ",");
                                size_t prev_size = 0;

                                while (size_str) {
                                        size_t size_val = parse_size_value(size_str);

                                        if (size_val > 0) {
                                                if (prev_size > 0 && size_val <= prev_size) {
                                                        printf("Invalid size value: %zu. Sizes "
                                                               "must be in "
                                                               "ascending order.\n",
                                                               size_val);
                                                        free(sizes_arg);
                                                        return 1;
                                                }
                                                prev_size = size_val;
                                        } else {
                                                printf("Invalid size value: '%s'. Sizes must be "
                                                       "positive "
                                                       "integers with optional K (KB) or M (MB) "
                                                       "suffix.\n",
                                                       size_str);
                                                free(sizes_arg);
                                                return 1;
                                        }

                                        size_str = strtok(NULL, ",");
                                }

                                // If we parsed some sizes successfully, use them
                                if (prev_size > 0) {
                                        use_size_list = 1;
                                        size_count = 0;
                                        char *sizes_copy = strdup(sizes_arg_str);
                                        char *token = strtok(sizes_copy, ",");
                                        while (token && size_count < MAX_SIZE_COUNT) {
                                                size_list[size_count++] = parse_size_value(token);
                                                token = strtok(NULL, ",");
                                        }
                                        free(sizes_copy);
                                        test_len =
                                                size_list[size_count -
                                                          1]; // Set test_len to the largest value
                                        printf("Running benchmark with specified buffer sizes: "
                                               "%s\n",
                                               sizes_arg_str);
                                }

                                free(sizes_arg);
                        } else {
                                printf("Option --sizes requires an argument.\n");
                                print_help();
                                return 1;
                        }
                }

                // Unknown option
                else if (argv[i][0] == '-') {
                        printf("Unknown option: %s\n", argv[i]);
                        print_help();
                        return 1;
                }
        }

        if (!csv_output) {
                printf("CRC Functions Performance Benchmark\n");
        } else {
#ifdef USE_RDTSC
                printf("crc_type,buffer_size,cycles\n");
#else
                printf("crc_type,buffer_size,throughput\n");
#endif
        }

        if (use_cold_cache)
                test_len = COLD_CACHE_TEST_MEM;

        if (posix_memalign(&buf, 64, test_len)) {
                printf("alloc error: Fail\n");
                return -1;
        }

        srand(20250618);
        for (size_t i = 0; i < test_len; i++)
                ((uint8_t *) buf)[i] = (uint8_t) rand();

        // Process according to chosen CRC type and size options
        if (use_range) {
                if (use_cold_cache && min_len <= COLD_CACHE_MIN_LEN)
                        printf("Error: Cold cache tests require buffer sizes larger than "
                               "%u bytes.",
                               COLD_CACHE_MIN_LEN);

                // Use range-based sizes
                if (!csv_output) {
                        printf("Running benchmarks with size range from %zu to %zu\n", min_len,
                               max_len);
                }

                // Call the benchmark function for specific type with range parameters
                benchmark_crc_type_range(crc_type, buf, min_len, max_len, step_len, csv_output,
                                         include_base, use_cold_cache);

        } else if (use_size_list) {
                if (use_cold_cache && size_list[0] <= COLD_CACHE_MIN_LEN)
                        printf("Error: Cold cache tests require buffer sizes larger than "
                               "%u bytes.",
                               COLD_CACHE_MIN_LEN);

                // Use list of specific sizes

                // Call the benchmark function for specific type with size list parameters
                benchmark_crc_type_size_list(crc_type, buf, size_list, size_count, csv_output,
                                             include_base, use_cold_cache);

        } else {
                // Use default size or single size specified

                // Call the benchmark function for specific type with default size
                benchmark_crc_type_default(crc_type, buf, test_len, csv_output, include_base,
                                           use_cold_cache);
        }

        if (!csv_output) {
                printf("\nBenchmark complete\n");
        }

        aligned_free(buf);
        return 0;
}
