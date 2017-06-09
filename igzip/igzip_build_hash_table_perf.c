#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <getopt.h>
#include "igzip_lib.h"
#include "test.h"

#define DICT_LEN 32*1024
#define ITERATIONS 100000

extern void isal_deflate_hash(struct isal_zstream *stream, uint8_t * dict, int dict_len);

void create_rand_data(uint8_t * data, uint32_t size)
{
	int i;
	for (i = 0; i < size; i++) {
		data[i] = rand() % 256;
	}
}

int main(int argc, char *argv[])
{
	int i, iterations = ITERATIONS;
	struct isal_zstream stream;
	uint8_t dict[DICT_LEN];
	uint32_t dict_len = DICT_LEN;

	stream.level = 0;
	create_rand_data(dict, dict_len);

	struct perf start, stop;
	perf_start(&start);

	for (i = 0; i < iterations; i++) {
		isal_deflate_hash(&stream, dict, dict_len);
	}

	perf_stop(&stop);

	printf("igzip_build_hash_table_perf:\n");
	printf("  in_size=%u iter=%d ", dict_len, i);
	perf_print(stop, start, (long long)dict_len * i);
}
