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
#include <inttypes.h>
#include <ctype.h>

#ifdef _WIN32
#include <Windows.h>
#include <io.h>
#define ssize_t    long
#define strcasecmp _stricmp
#else
#include <strings.h>
#include <sys/types.h>
#endif

#include "raid.h"
#include "test.h"

#define DEFAULT_SOURCES  10
#define DEFAULT_TEST_LEN 8 * 1024

#define COLD_CACHE_TEST_MEM (1024 * 1024 * 1024)

// Define RAID function types
typedef enum {
        // XOR function
        XOR_GEN = 0,
        // PQ function
        PQ_GEN = 1,
        // Category group for all functions
        RAID_ALL = 2
} raid_type_t;

// Function pointer type for RAID functions
typedef int (*raid_func_t)(int vects, int len, void **array);

// Helper function to get buffer count for a specific RAID type
static int
get_buffer_count(const raid_type_t type, const int sources)
{
        switch (type) {
        case XOR_GEN:
                return sources + 1; // +1 for destination buffer
        case PQ_GEN:
        default:
                return sources + 2; // +2 for P and Q buffers
        }
}

void
run_benchmark(void *buffs[], const size_t len, const int sources, const raid_type_t type,
              const int csv_output, const int use_cold_cache);
void
print_help(void);
void
print_raid_types(void);
raid_type_t
parse_raid_type(const char *type_str);
size_t
parse_size_value(const char *size_str);

static void
run_raid_type_range(const raid_type_t type, void *buffs[], const size_t min_len,
                    const size_t max_len, const int is_multiplicative, const size_t abs_step,
                    const int sources, const int csv_output, const int use_cold_cache);
static void
run_raid_type_size_list(const raid_type_t type, void *buffs[], const size_t *size_list,
                        const int size_count, const int sources, const int csv_output,
                        const int use_cold_cache);

void
benchmark_raid_type_range(const raid_type_t raid_type, void *buffs[], const size_t min_len,
                          const size_t max_len, const ssize_t step_len, const int sources,
                          const int csv_output, const int use_cold_cache);
void
benchmark_raid_type_size_list(const raid_type_t raid_type, void *buffs[], const size_t *size_list,
                              const int size_count, const int sources, const int csv_output,
                              const int use_cold_cache);

// Function to run a specific RAID benchmark
// len is the block size for each source and destination buffer, in bytes
void
run_benchmark(void *buffs[], const size_t len, const int num_sources_dest, const raid_type_t type,
              const int csv_output, const int use_cold_cache)
{
        struct perf start;
        const char *raid_type_str = "";
        raid_func_t raid_func = NULL;

        // Set up function pointer and type string based on RAID type
        switch (type) {
        case XOR_GEN:
                raid_type_str = "xor_gen";
                raid_func = xor_gen;
                break;
        case PQ_GEN:
                raid_type_str = "pq_gen";
                raid_func = pq_gen;
                break;
        default:
                fprintf(stderr, "Invalid RAID function type\n");
                exit(1);
        }

        // Create list of random buffer addresses within the large memory space for cold cache tests
        if (use_cold_cache) {
                const size_t buffer_size_each =
                        COLD_CACHE_TEST_MEM / (num_sources_dest + 2); // +2 for max buffers needed
                const size_t num_buffer_sets = buffer_size_each / len;

                if (num_buffer_sets == 0) {
                        fprintf(stderr, "Buffer size too large for cold cache test\n");
                        exit(1);
                }

                void ***buffer_set_list = (void ***) malloc(num_buffer_sets * sizeof(void **));
                if (!buffer_set_list) {
                        fprintf(stderr, "Failed to allocate memory for cold cache buffer list\n");
                        exit(1);
                }

                // For each buffer set, create pointers to random offsets within the allocated
                // memory
                for (size_t i = 0; i < num_buffer_sets; i++) {
                        buffer_set_list[i] = (void **) malloc(num_sources_dest * sizeof(void *));
                        if (!buffer_set_list[i]) {
                                fprintf(stderr,
                                        "Failed to allocate memory for cold cache buffer set %zu\n",
                                        i);
                                // Clean up previously allocated sets
                                for (size_t j = 0; j < i; j++)
                                        free(buffer_set_list[j]);
                                free(buffer_set_list);
                                exit(1);
                        }

                        // Calculate random offset for this buffer set, ensuring we don't exceed
                        // buffer bounds
                        const size_t offset = len * (rand() % num_buffer_sets);

                        for (int j = 0; j < num_sources_dest; j++)
                                buffer_set_list[i][j] = (uint8_t *) buffs[j] + offset;
                }

                size_t current_buffer_idx = 0;
                BENCHMARK_COLD(
                        &start, BENCHMARK_TIME,
                        current_buffer_idx = (current_buffer_idx + 1) % num_buffer_sets,
                        raid_func(num_sources_dest, len, buffer_set_list[current_buffer_idx]));

                for (size_t i = 0; i < num_buffer_sets; i++)
                        free(buffer_set_list[i]);
                free(buffer_set_list);

        } else {
                BENCHMARK(&start, BENCHMARK_TIME, raid_func(num_sources_dest, len, buffs));
        }

        if (csv_output) {
#ifdef USE_RDTSC
                // When USE_RDTSC is defined, report total cycles per buffer
                const double cycles = (double) get_base_elapsed(&start);
                const double cycles_per_buffer = cycles / (double) start.iterations;
                printf("%s,%zu,%d,%.0f\n", raid_type_str, len, num_sources_dest, cycles_per_buffer);
#else
                // Calculate throughput in MB/s
                const double time_elapsed = get_time_elapsed(&start);
                const long long total_sources_dest = start.iterations * num_sources_dest;
                const long long total_units = total_sources_dest * (long long) len;
                const double throughput = ((double) total_units) / (1000000 * time_elapsed);
                printf("%s,%zu,%d,%.2f\n", raid_type_str, len, num_sources_dest, throughput);
#endif
        } else {
                printf("%s: ", raid_type_str);
                perf_print(start, (long long) len * num_sources_dest);
        }
}

void
print_help(void)
{
        printf("Usage: raid_funcs_perf [options]\n");
        printf("  -h, --help          Show this help\n");
        printf("  -t, --type TYPE     RAID function type to benchmark\n");
        printf("    Function types:\n");
        printf("      0 or 'xor_gen'        - XOR generation\n");
        printf("      1 or 'pq_gen'         - P+Q generation\n");
        printf("    Groups:\n");
        printf("      2 or 'all'            - All RAID functions (default)\n");
        printf("                             If no value is provided, lists available types and "
               "exits\n");
        printf("  --sources N         Number of source buffers to use (default: %d)\n",
               DEFAULT_SOURCES);
        printf("  -r, --range MIN:STEP:MAX  Run tests with buffer sizes from MIN to MAX in STEP "
               "increments\n");
        printf("                       If STEP starts with '*', it's treated as a multiplier\n");
        printf("                       Size values can include K (KB) or M (MB) suffix\n");
        printf("                       Examples:\n");
        printf("                         --range 1K:1K:16K (1KB to 16KB in 1KB steps)\n");
        printf("                         --range 1K:*2:16K (1KB, 2KB, 4KB, 8KB, 16KB)\n");
        printf("  -s, --sizes SIZE1,SIZE2,...  Run tests with specified comma-separated buffer "
               "sizes\n");
        printf("                       Example: --sizes 1024,4096,8192,16384\n");
        printf("                       Size values can include K (KB) or M (MB) suffix\n");
        printf("  -c, --csv           Output results in CSV format\n");
        printf("      --cold          Use cold cache for benchmarks (buffer not in cache)\n");
}

void
print_raid_types(void)
{
        printf("Available RAID function types:\n");
        printf("  Function types:\n");
        printf("    0 or 'xor_gen'        - XOR generation\n");
        printf("    1 or 'pq_gen'         - P+Q generation\n");
        printf("  Groups:\n");
        printf("    2 or 'all'            - All RAID functions (default)\n");
}

// Helper function to parse string input for RAID type
raid_type_t
parse_raid_type(const char *type_str)
{
        if (!type_str)
                return RAID_ALL;

        // Try as integer first
        char *endptr;
        const long val = strtol(type_str, &endptr, 10);

        // If the entire string was parsed as an integer and it's within range
        if (*type_str != '\0' && *endptr == '\0' && val >= 0 && val <= RAID_ALL)
                return (raid_type_t) val;

        // XOR function
        if (strcasecmp(type_str, "xor_gen") == 0)
                return XOR_GEN;

        // PQ function
        if (strcasecmp(type_str, "pq_gen") == 0)
                return PQ_GEN;

        // Group types
        if (strcasecmp(type_str, "all") == 0)
                return RAID_ALL;

        // Invalid input, return default
        return RAID_ALL;
}

// Helper function to parse size values with optional K (KB) or M (MB) suffix
size_t
parse_size_value(const char *size_str)
{
        size_t size_val;
        char *endptr;
        size_t multiplier = 1;
        char *const str_copy = strdup(size_str); // Check for size suffixes (K for KB, M for MB)
        if (!str_copy)
                return 0;

        const int len = strlen(str_copy);
        if (len > 0) {
                // Convert to uppercase for case-insensitive comparison
                const char last_char = toupper(str_copy[len - 1]);

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
#define MAX_SRC_BUFS   20 // Maximum number of source buffers

int
main(int argc, char *argv[])
{
        void **buffs;
        raid_type_t raid_type = RAID_ALL;
        int csv_output = 0;     // Flag for CSV output mode
        int use_cold_cache = 0; // Flag for cold cache mode
        size_t test_len = DEFAULT_TEST_LEN;
        int use_range = 0;
        size_t min_len = 0, max_len = 0;
        ssize_t step_len = 0; // Using signed type to indicate additive vs multiplicative
        int sources = DEFAULT_SOURCES;

        // For comma-separated sizes option
        size_t size_list[MAX_SIZE_COUNT];
        int size_count = 1;

        size_list[0] = DEFAULT_TEST_LEN; // Default size if no sizes are specified

        // Parse command line arguments manually (no getopt)
        for (int i = 1; i < argc; i++) {
                // Check for help option
                if (strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "--help") == 0) {
                        print_help();
                        return 0;
                }

                // Check for type option
                else if (strcmp(argv[i], "-t") == 0 || strcmp(argv[i], "--type") == 0) {
                        if (i + 1 < argc && argv[i + 1][0] != '-') {
                                raid_type = parse_raid_type(argv[i + 1]);
                                // Check if value is outside valid range for individual RAID types
                                // or group types
                                if (raid_type > RAID_ALL) {
                                        fprintf(stderr,
                                                "Invalid RAID type: '%s'. Using default (all).\n",
                                                argv[i + 1]);
                                        raid_type = RAID_ALL;
                                }
                                i++; // Skip the argument value in the next iteration
                        } else {
                                // -t provided without argument or the next arg starts with '-'
                                print_raid_types();
                                return 0;
                        }
                }

                // Check for sources option
                else if (strcmp(argv[i], "--sources") == 0) {
                        if (i + 1 < argc && argv[i + 1][0] != '-') {
                                sources = atoi(argv[i + 1]);
                                if (sources <= 0) {
                                        fprintf(stderr,
                                                "Invalid number of sources: %s. Using default "
                                                "(%d).\n",
                                                argv[i + 1], DEFAULT_SOURCES);
                                        sources = DEFAULT_SOURCES;
                                } else if (sources > MAX_SRC_BUFS) {
                                        fprintf(stderr,
                                                "Number of sources too large: %d. Using maximum "
                                                "(%d).\n",
                                                sources, MAX_SRC_BUFS);
                                        sources = MAX_SRC_BUFS;
                                }
                                i++; // Skip the argument value in the next iteration
                        } else {
                                fprintf(stderr, "Option --sources requires an argument.\n");
                                print_help();
                                return 0;
                        }
                }

                // Check for range option
                else if (strcmp(argv[i], "-r") == 0 || strcmp(argv[i], "--range") == 0) {
                        if (i + 1 < argc && argv[i + 1][0] != '-') {
                                // Range specified, parse it
                                char *range_arg = strdup(argv[i + 1]);
                                char *const min_str = strtok(range_arg, ":");
                                char *const step_str = strtok(NULL, ":");
                                char *const max_str = strtok(NULL, ":");

                                if (!range_arg)
                                        return 1;

                                if (min_str && step_str && max_str) {
                                        min_len = parse_size_value(min_str);
                                        max_len = parse_size_value(max_str);

                                        // Check for multiplicative step (starts with '*')
                                        int step_is_multiplicative = 0;
                                        if (step_str[0] == '*') {
                                                step_is_multiplicative = 1;
                                                if (strlen(step_str) > 1)
                                                        step_len = parse_size_value(
                                                                step_str + 1); // Skip the '*'
                                                else
                                                        step_len = 0; // Invalid step
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
                                                fprintf(stderr,
                                                        "Invalid range values. Ensure MIN > 0, MAX "
                                                        ">= MIN, "
                                                        "and STEP > 0.\n");
                                                free(range_arg);
                                                return 1;
                                        }
                                } else {
                                        fprintf(stderr,
                                                "Invalid range format. Use MIN:STEP:MAX (e.g., "
                                                "1024:1024:16384 or 1024:*2:16384)\n");
                                        free(range_arg);
                                        return 1;
                                }

                                free(range_arg);
                                i++; // Skip the argument value in the next iteration
                        } else {
                                fprintf(stderr, "Option -r requires an argument.\n");
                                print_help();
                                return 0;
                        }
                }

                // Check for sizes option
                else if (strcmp(argv[i], "-s") == 0 || strcmp(argv[i], "--sizes") == 0) {
                        if (i + 1 < argc && argv[i + 1][0] != '-') {
                                // Sizes specified, parse them as a list
                                char *sizes_arg = strdup(argv[i + 1]);
                                char *size_str = strtok(sizes_arg, ",");
                                size_t prev_size = 0;

                                while (size_str) {
                                        size_t size_val = parse_size_value(size_str);

                                        if (size_val > 0) {
                                                if (prev_size > 0 && size_val <= prev_size) {
                                                        fprintf(stderr,
                                                                "Invalid size value: %zu. Sizes "
                                                                "must be in "
                                                                "ascending order.\n",
                                                                size_val);
                                                        free(sizes_arg);
                                                        return 1;
                                                }
                                                prev_size = size_val;
                                        } else {
                                                fprintf(stderr,
                                                        "Invalid size value: '%s'. Sizes must be "
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
                                        use_range = 0;
                                        size_count = 0;
                                        char *sizes_copy = strdup(argv[i + 1]);
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
                                               argv[i + 1]);
                                }

                                free(sizes_arg);
                                i++; // Skip the argument value in the next iteration
                        } else {
                                fprintf(stderr, "Option -s requires an argument.\n");
                                print_help();
                                return 0;
                        }
                }

                // Check for csv option
                else if (strcmp(argv[i], "-c") == 0 || strcmp(argv[i], "--csv") == 0) {
                        csv_output = 1;
                }

                // Cold cache option
                else if (strcmp(argv[i], "--cold") == 0) {
                        use_cold_cache = 1;
                        printf("Cold cache option enabled\n");
                }

                // Unknown option
                else if (argv[i][0] == '-') {
                        fprintf(stderr, "Unknown option: %s\n", argv[i]);
                        print_help();
                        return 1;
                }
        }

        if (!csv_output) {
                printf("RAID Functions Performance Benchmark\n");
        } else {
#ifdef USE_RDTSC
                printf("raid_type,buffer_size,sources+dest,cycles\n");
#else
                printf("raid_type,buffer_size,sources+dest,throughput\n");
#endif
        }

        // For XOR and P+Q, we need sources + 1 or sources + 2 buffers
        const int max_needed_buffs =
                get_buffer_count(PQ_GEN, sources); // PQ_GEN needs the most buffers
        if (max_needed_buffs > MAX_SRC_BUFS) {
                fprintf(stderr, "Error: Source count too large for buffer allocation\n");
                return 1;
        }

        // For cold cache, we need larger buffers to accommodate the full memory space
        if (use_cold_cache)
                test_len = COLD_CACHE_TEST_MEM / max_needed_buffs;

        // Allocate buffer pointers
        buffs = (void **) malloc(sizeof(void *) * max_needed_buffs);
        if (!buffs) {
                fprintf(stderr, "Error: Failed to allocate buffer pointers\n");
                return 1;
        }

        srand(20250707);
        // Allocate the actual buffers
        for (int i = 0; i < max_needed_buffs; i++) {
                int ret = posix_memalign(&buffs[i], 64, test_len);
                if (ret) {
                        fprintf(stderr, "Error: Failed to allocate buffer memory\n");
                        // Free previously allocated buffers
                        for (int j = 0; j < i; j++) {
                                aligned_free(buffs[j]);
                        }
                        free(buffs);
                        return 1;
                }
                // Initialize buffer with random data
                for (size_t j = 0; j < test_len; j++)
                        ((uint8_t *) buffs[i])[j] = rand() & 0xFF;
        }

        // Process according to chosen RAID type and size options
        if (use_range) {
                // Use range-based sizes
                if (!csv_output) {
                        printf("Running benchmarks with size range from %zu to %zu\n", min_len,
                               max_len);
                }

                // Call the benchmark function for specific type with range parameters
                benchmark_raid_type_range(raid_type, buffs, min_len, max_len, step_len, sources,
                                          csv_output, use_cold_cache);
        } else {
                // Use list of specific sizes

                // Call the benchmark function for specific type with size list parameters
                benchmark_raid_type_size_list(raid_type, buffs, size_list, size_count, sources,
                                              csv_output, use_cold_cache);
        }

        if (!csv_output) {
                printf("\nBenchmark complete\n");
        }

        // Free allocated memory
        for (int i = 0; i < max_needed_buffs; i++) {
                aligned_free(buffs[i]);
        }
        free(buffs);

        return 0;
}

/* Helper function to process a specific RAID type for benchmark_raid_type_range */
static void
run_raid_type_range(const raid_type_t type, void *buffs[], const size_t min_len,
                    const size_t max_len, const int is_multiplicative, const size_t abs_step,
                    const int sources, const int csv_output, const int use_cold_cache)
{
        const char *type_names[] = { "XOR Generation", "P+Q Generation" };
        size_t len;
        const int buffer_count = get_buffer_count(type, sources);

        if (!csv_output && type < RAID_ALL) {
                printf("\n%s (%d sources):\n", type_names[type], sources);
        }

        for (len = min_len; len <= max_len;) {

                if (!csv_output) {
                        printf("\n  Buffer size: %zu bytes\n", len);
                }

                run_benchmark(buffs, len, buffer_count, type, csv_output, use_cold_cache);

                /* Update length based on step type */
                if (is_multiplicative) {
                        len *= abs_step;
                } else {
                        len += abs_step;
                }
        }
}

/* Helper function to run benchmarks for a specific RAID type with range of sizes */
void
benchmark_raid_type_range(const raid_type_t raid_type, void *buffs[], const size_t min_len,
                          const size_t max_len, const ssize_t step_len, const int sources,
                          const int csv_output, const int use_cold_cache)
{
        const int is_multiplicative = (step_len < 0);
        const size_t abs_step = is_multiplicative ? -step_len : step_len;

        /* Process according to the chosen RAID type */
        if (raid_type == RAID_ALL) {
                /* Run all RAID types */
                if (!csv_output)
                        printf("\n=========== XOR FUNCTION ===========\n");
                run_raid_type_range(XOR_GEN, buffs, min_len, max_len, is_multiplicative, abs_step,
                                    sources, csv_output, use_cold_cache);

                if (!csv_output)
                        printf("\n=========== P+Q FUNCTION ===========\n");
                run_raid_type_range(PQ_GEN, buffs, min_len, max_len, is_multiplicative, abs_step,
                                    sources, csv_output, use_cold_cache);
        } else {
                /* Run just the specific RAID type */
                run_raid_type_range(raid_type, buffs, min_len, max_len, is_multiplicative, abs_step,
                                    sources, csv_output, use_cold_cache);
        }
}

/* Helper function to process a specific RAID type for benchmark_raid_type_size_list */
static void
run_raid_type_size_list(const raid_type_t type, void *buffs[], const size_t *size_list,
                        const int size_count, const int sources, const int csv_output,
                        const int use_cold_cache)
{
        const char *type_names[] = { "XOR Generation", "P+Q Generation" };
        const int buffer_count = get_buffer_count(type, sources);
        int i;
        size_t len;

        if (!csv_output && type < RAID_ALL) {
                printf("\n%s (%d sources):\n", type_names[type], sources);
        }

        for (i = 0; i < size_count; i++) {
                len = size_list[i];
                if (!csv_output) {
                        printf("\n  Buffer size: %zu bytes\n", len);
                }

                run_benchmark(buffs, len, buffer_count, type, csv_output, use_cold_cache);
        }
}

/* Helper function to run benchmarks for a specific RAID type with list of sizes */
void
benchmark_raid_type_size_list(const raid_type_t raid_type, void *buffs[], const size_t *size_list,
                              const int size_count, const int sources, const int csv_output,
                              const int use_cold_cache)
{
        /* Process according to the chosen RAID type */
        if (raid_type == RAID_ALL) {
                /* Run all RAID types */
                if (!csv_output)
                        printf("\n=========== XOR FUNCTION ===========\n");
                run_raid_type_size_list(XOR_GEN, buffs, size_list, size_count, sources, csv_output,
                                        use_cold_cache);

                if (!csv_output)
                        printf("\n=========== P+Q FUNCTION ===========\n");
                run_raid_type_size_list(PQ_GEN, buffs, size_list, size_count, sources, csv_output,
                                        use_cold_cache);
        } else {
                /* Run just the specific RAID type */
                run_raid_type_size_list(raid_type, buffs, size_list, size_count, sources,
                                        csv_output, use_cold_cache);
        }
}
