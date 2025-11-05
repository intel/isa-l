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

#if !defined(_WIN32) && !defined(_GNU_SOURCE)
#define _GNU_SOURCE // Required for CPU affinity functions on Linux
#endif

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
#ifndef strdup
#define strdup _strdup
#endif
#else
#include <strings.h>
#include <sys/types.h>
#include <pthread.h>
#include <sched.h>
#include <unistd.h>
#ifdef __APPLE__
#include <mach/mach.h>
#include <mach/thread_policy.h>
#endif
#endif

#include "raid.h"
#include "test.h"

// Cross-platform threading definitions
#ifdef _WIN32
typedef HANDLE perf_thread_t;
typedef DWORD(WINAPI *thread_func_t)(LPVOID);
#define THREAD_RETURN     DWORD WINAPI
#define THREAD_RETURN_VAL 0
#else
typedef pthread_t perf_thread_t;
typedef void *(*thread_func_t)(void *);
#define THREAD_RETURN     void *
#define THREAD_RETURN_VAL NULL
#endif

#define DEFAULT_SOURCES  10
#define DEFAULT_TEST_LEN 8 * 1024

#define COLD_CACHE_TEST_MEM (1024 * 1024 * 1024)

// Global variables:
//   g_thread_bufs: Pointer to per-thread buffer arrays for single memory space allocation.
//   g_coremask: Stores the CPU affinity mask for thread pinning.
static void ***g_thread_bufs = NULL;
static unsigned long g_coremask = 0;

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

// Thread data structure for multi-threaded benchmarking
typedef struct {
        int thread_id;
        int cpu_core; // CPU core number for affinity (-1 = no pinning)
        void **buffs; // This thread's buffer pointers
        size_t test_len;
        int sources;
        raid_type_t raid_type;
        int csv_output;
        int use_cold_cache;
} thread_data_t;

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
                    const int sources, const int csv_output, const int use_cold_cache,
                    const int num_threads);
static void
run_raid_type_size_list(const raid_type_t type, void *buffs[], const size_t *size_list,
                        const int size_count, const int sources, const int csv_output,
                        const int use_cold_cache, const int num_threads);

void
benchmark_raid_type_range(const raid_type_t raid_type, void *buffs[], const size_t min_len,
                          const size_t max_len, const ssize_t step_len, const int sources,
                          const int csv_output, const int use_cold_cache, const int num_threads);
void
benchmark_raid_type_size_list(const raid_type_t raid_type, void *buffs[], const size_t *size_list,
                              const int size_count, const int sources, const int csv_output,
                              const int use_cold_cache, const int num_threads);

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
                const size_t buffer_size_each = COLD_CACHE_TEST_MEM / num_sources_dest;
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
        printf("  --coremask MASK     CPU core mask in hex (e.g., 0xF for 4 threads on cores 0-3, "
               "0 = single-threaded). Max core ID is 63.\n");
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

// Helper function to count set bits in coremask
static int
count_coremask_bits(unsigned long coremask)
{
        if (coremask == 0)
                return 1; // Default to single-threaded when no coremask

        int count = 0;
        for (int bit = 0; bit < 64; bit++) {
                if (coremask & (1UL << bit))
                        count++;
        }
        return count;
}

// Helper function to extract core numbers from coremask
static int
get_core_from_mask(unsigned long coremask, int thread_index)
{
        if (coremask == 0)
                return -1; // No pinning

        // First count total cores to handle wrap-around efficiently
        int core_count = count_coremask_bits(coremask);

        if (core_count == 0)
                return -1; // No cores in mask

        // Use modulo to handle wrap-around without recursion
        const int target_index = thread_index % core_count;
        int current_index = 0;

        // Find the core at the target index position
        for (int bit = 0; bit < 64; bit++) {
                if (coremask & (1UL << bit)) {
                        if (current_index == target_index)
                                return bit;
                        current_index++;
                }
        }

        return -1; // Should never reach here
}

// Helper function to set CPU affinity for current thread
static int
set_cpu_affinity(int cpu_core)
{
        if (cpu_core < 0)
                return 0; // No pinning requested

#ifdef _WIN32
        DWORD_PTR affinity_mask = 1ULL << cpu_core;
        HANDLE thread_handle = GetCurrentThread();

        DWORD_PTR result = SetThreadAffinityMask(thread_handle, affinity_mask);
        if (result == 0) {
                fprintf(stderr, "Warning: Failed to set CPU affinity to core %d\n", cpu_core);
                return -1;
        }
        return 0;
#elif defined(__APPLE__)
        // macOS uses thread affinity policy with affinity tags
        thread_affinity_policy_data_t policy = { cpu_core };
        mach_port_t mach_thread = pthread_mach_thread_np(pthread_self());

        kern_return_t ret = thread_policy_set(mach_thread, THREAD_AFFINITY_POLICY,
                                              (thread_policy_t) &policy, 1);
        if (ret != KERN_SUCCESS) {
                fprintf(stderr, "Warning: Failed to set CPU affinity to core %d\n", cpu_core);
                return -1;
        }
        return 0;
#else
        cpu_set_t cpuset;
        CPU_ZERO(&cpuset);
        CPU_SET(cpu_core, &cpuset);

        int ret = sched_setaffinity(0, sizeof(cpuset), &cpuset);
        if (ret != 0) {
                fprintf(stderr, "Warning: Failed to set CPU affinity to core %d\n", cpu_core);
                return -1;
        }
        return 0;
#endif
}

// Thread worker function for multi-threaded benchmarking
static THREAD_RETURN
thread_worker(void *arg)
{
        thread_data_t *data = (thread_data_t *) arg;
        const int buffer_count = get_buffer_count(data->raid_type, data->sources);

        // Set CPU affinity if core pinning is requested
        set_cpu_affinity(data->cpu_core);

        // Run the benchmark with this thread's buffers
        run_benchmark(data->buffs, data->test_len, buffer_count, data->raid_type, data->csv_output,
                      data->use_cold_cache);

        return THREAD_RETURN_VAL;
}

// Multi-threaded wrapper for run_benchmark
static void
run_benchmark_multithreaded(const size_t len, const int sources, const raid_type_t type,
                            const int csv_output, const int use_cold_cache, const int num_threads)
{
        if (num_threads == 1 || !g_thread_bufs) {
                // Single-threaded fallback
                const int buffer_count = get_buffer_count(type, sources);
                run_benchmark(g_thread_bufs[0], len, buffer_count, type, csv_output,
                              use_cold_cache);
                return;
        }

        // Multi-threaded execution
        perf_thread_t *threads = malloc(num_threads * sizeof(perf_thread_t));
        thread_data_t *thread_data = malloc(num_threads * sizeof(thread_data_t));

        if (!threads || !thread_data) {
                fprintf(stderr, "Error: Failed to allocate memory for threads\n");
                free(threads);
                free(thread_data);
                return;
        }

        // Create and start threads
        for (int i = 0; i < num_threads; i++) {
                thread_data[i].thread_id = i;
                thread_data[i].cpu_core = get_core_from_mask(g_coremask, i);
                thread_data[i].buffs = g_thread_bufs[i];
                thread_data[i].test_len = len;
                thread_data[i].sources = sources;
                thread_data[i].raid_type = type;
                thread_data[i].csv_output = csv_output;
                thread_data[i].use_cold_cache = use_cold_cache;

#ifdef _WIN32
                threads[i] = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE) thread_worker,
                                          &thread_data[i], 0, NULL);
                if (threads[i] == NULL) {
                        fprintf(stderr, "Error: Failed to create thread %d\n", i);
                        // Clean up already created threads
                        for (int j = 0; j < i; j++) {
                                WaitForSingleObject(threads[j], INFINITE);
                                CloseHandle(threads[j]);
                        }
                        free(threads);
                        free(thread_data);
                        return;
                }
#else
                int ret = pthread_create(&threads[i], NULL, thread_worker, &thread_data[i]);
                if (ret != 0) {
                        fprintf(stderr, "Error: Failed to create thread %d\n", i);
                        // Clean up already created threads
                        for (int j = 0; j < i; j++) {
                                pthread_join(threads[j], NULL);
                        }
                        free(threads);
                        free(thread_data);
                        return;
                }
#endif
        }

        // Wait for all threads to complete
        for (int i = 0; i < num_threads; i++) {
#ifdef _WIN32
                WaitForSingleObject(threads[i], INFINITE);
                CloseHandle(threads[i]);
#else
                pthread_join(threads[i], NULL);
#endif
        }

        free(threads);
        free(thread_data);
}

int
main(int argc, char *argv[])
{
        void **buffs;
        raid_type_t raid_type = RAID_ALL;
        int csv_output = 0;         // Flag for CSV output mode
        int use_cold_cache = 0;     // Flag for cold cache mode
        int num_threads = 1;        // Number of threads for benchmarking (default: single-threaded)
        unsigned long coremask = 0; // CPU core mask for thread affinity (0 = no pinning)
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

                // Check for coremask option
                else if (strcmp(argv[i], "--coremask") == 0) {
                        if (i + 1 < argc && argv[i + 1][0] != '-') {
                                char *endptr;
                                coremask = strtoul(argv[i + 1], &endptr,
                                                   0); // 0 = auto-detect base (hex/dec)
                                if (*endptr != '\0') {
                                        fprintf(stderr, "Invalid coremask: %s. Using no pinning.\n",
                                                argv[i + 1]);
                                        coremask = 0;
                                }
                                i++; // Skip the argument value in the next iteration
                        } else {
                                fprintf(stderr, "Option --coremask requires an argument.\n");
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

        // Determine number of threads from coremask
        num_threads = count_coremask_bits(coremask);

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

        // Single memory space allocation for all buffers across all threads
        const size_t allocated_size_per_buf = test_len;
        const size_t total_memory_size =
                (size_t) max_needed_buffs * num_threads * allocated_size_per_buf;

        void *shared_memory;
        int ret = posix_memalign(&shared_memory, 64, total_memory_size);
        if (ret) {
                fprintf(stderr, "Error: Failed to allocate shared memory\n");
                return 1;
        }

        srand(20250707);
        // Initialize all memory with incremental data
        for (size_t j = 0; j < total_memory_size; j++)
                ((uint8_t *) shared_memory)[j] = j & 0xFF;

        // Create void ***bufs structure for thread management
        void ***bufs = (void ***) malloc(num_threads * sizeof(void **));
        if (!bufs) {
                fprintf(stderr, "Error: Failed to allocate thread buffer pointers\n");
                free(shared_memory);
                return 1;
        }

        // Set up memory layout for each thread
        for (int thread = 0; thread < num_threads; thread++) {
                bufs[thread] = (void **) malloc(max_needed_buffs * sizeof(void *));
                if (!bufs[thread]) {
                        fprintf(stderr, "Error: Failed to allocate buffer pointers for thread %d\n",
                                thread);
                        // Cleanup previously allocated thread buffers
                        for (int cleanup_thread = 0; cleanup_thread < thread; cleanup_thread++)
                                free(bufs[cleanup_thread]);
                        free(bufs);
                        free(shared_memory);
                        return 1;
                }

                // Assign memory slices to each thread's buffers
                for (int buffer = 0; buffer < max_needed_buffs; buffer++) {
                        size_t offset =
                                (thread * max_needed_buffs + buffer) * allocated_size_per_buf;
                        bufs[thread][buffer] = (void *) ((uint8_t *) shared_memory + offset);
                }
        }

        // For single-threaded case, use the first thread's buffers
        buffs = bufs[0];

        // Store globally for potential future use by helper functions
        g_thread_bufs = bufs;
        g_coremask = coremask;

        // Process according to chosen RAID type and size options
        if (use_range) {
                // Use range-based sizes
                if (!csv_output) {
                        printf("Running benchmarks with size range from %zu to %zu\n", min_len,
                               max_len);
                }

                // Call the benchmark function for specific type with range parameters
                benchmark_raid_type_range(raid_type, buffs, min_len, max_len, step_len, sources,
                                          csv_output, use_cold_cache, num_threads);
        } else {
                // Use list of specific sizes

                // Call the benchmark function for specific type with size list parameters
                benchmark_raid_type_size_list(raid_type, buffs, size_list, size_count, sources,
                                              csv_output, use_cold_cache, num_threads);
        }

        if (!csv_output) {
                printf("\nBenchmark complete\n");
        }

        // Free allocated memory
        // Free thread buffer pointers
        for (int thread = 0; thread < num_threads; thread++) {
                free(bufs[thread]);
        }
        free(bufs);

        // Free shared memory
        aligned_free(shared_memory);

        return 0;
}

/* Helper function to process a specific RAID type for benchmark_raid_type_range */
static void
run_raid_type_range(const raid_type_t type, void *buffs[], const size_t min_len,
                    const size_t max_len, const int is_multiplicative, const size_t abs_step,
                    const int sources, const int csv_output, const int use_cold_cache,
                    const int num_threads)
{
        const char *type_names[] = { "XOR Generation", "P+Q Generation" };
        size_t len;

        if (!csv_output && type < RAID_ALL) {
                printf("\n%s (%d sources):\n", type_names[type], sources);
        }

        for (len = min_len; len <= max_len;) {

                if (!csv_output) {
                        printf("\n  Buffer size: %zu bytes\n", len);
                }

                run_benchmark_multithreaded(len, sources, type, csv_output, use_cold_cache,
                                            num_threads);

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
                          const int csv_output, const int use_cold_cache, const int num_threads)
{
        const int is_multiplicative = (step_len < 0);
        const size_t abs_step = is_multiplicative ? -step_len : step_len;

        /* Process according to the chosen RAID type */
        if (raid_type == RAID_ALL) {
                /* Run all RAID types */
                if (!csv_output)
                        printf("\n=========== XOR FUNCTION ===========\n");
                run_raid_type_range(XOR_GEN, buffs, min_len, max_len, is_multiplicative, abs_step,
                                    sources, csv_output, use_cold_cache, num_threads);

                if (!csv_output)
                        printf("\n=========== P+Q FUNCTION ===========\n");
                run_raid_type_range(PQ_GEN, buffs, min_len, max_len, is_multiplicative, abs_step,
                                    sources, csv_output, use_cold_cache, num_threads);
        } else {
                /* Run just the specific RAID type */
                run_raid_type_range(raid_type, buffs, min_len, max_len, is_multiplicative, abs_step,
                                    sources, csv_output, use_cold_cache, num_threads);
        }
}

/* Helper function to process a specific RAID type for benchmark_raid_type_size_list */
static void
run_raid_type_size_list(const raid_type_t type, void *buffs[], const size_t *size_list,
                        const int size_count, const int sources, const int csv_output,
                        const int use_cold_cache, const int num_threads)
{
        const char *type_names[] = { "XOR Generation", "P+Q Generation" };
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

                run_benchmark_multithreaded(len, sources, type, csv_output, use_cold_cache,
                                            num_threads);
        }
}

/* Helper function to run benchmarks for a specific RAID type with list of sizes */
void
benchmark_raid_type_size_list(const raid_type_t raid_type, void *buffs[], const size_t *size_list,
                              const int size_count, const int sources, const int csv_output,
                              const int use_cold_cache, const int num_threads)
{
        /* Process according to the chosen RAID type */
        if (raid_type == RAID_ALL) {
                /* Run all RAID types */
                if (!csv_output)
                        printf("\n=========== XOR FUNCTION ===========\n");
                run_raid_type_size_list(XOR_GEN, buffs, size_list, size_count, sources, csv_output,
                                        use_cold_cache, num_threads);

                if (!csv_output)
                        printf("\n=========== P+Q FUNCTION ===========\n");
                run_raid_type_size_list(PQ_GEN, buffs, size_list, size_count, sources, csv_output,
                                        use_cold_cache, num_threads);
        } else {
                /* Run just the specific RAID type */
                run_raid_type_size_list(raid_type, buffs, size_list, size_count, sources,
                                        csv_output, use_cold_cache, num_threads);
        }
}
