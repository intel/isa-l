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

#define _FILE_OFFSET_BITS 64
#include <stdio.h>
#include <assert.h>
#include <getopt.h>
#include "huff_codes.h"
#include "igzip_lib.h"
#include "test.h"

# include <zlib.h>

#define BUF_SIZE 1024
#define MIN_TEST_LOOPS   8
#ifndef RUN_MEM_SIZE
# define RUN_MEM_SIZE 1000000000
#endif

#define OPTARGS "hl:z:i:"

#define LEVEL_QUEUE_LIMIT 16
int level_size_buf[10] = {
#ifdef ISAL_DEF_LVL0_DEFAULT
	ISAL_DEF_LVL0_DEFAULT,
#else
	0,
#endif
#ifdef ISAL_DEF_LVL1_DEFAULT
	ISAL_DEF_LVL1_DEFAULT,
#else
	0,
#endif
#ifdef ISAL_DEF_LVL2_DEFAULT
	ISAL_DEF_LVL2_DEFAULT,
#else
	0,
#endif
#ifdef ISAL_DEF_LVL3_DEFAULT
	ISAL_DEF_LVL3_DEFAULT,
#else
	0,
#endif
#ifdef ISAL_DEF_LVL4_DEFAULT
	ISAL_DEF_LVL4_DEFAULT,
#else
	0,
#endif
#ifdef ISAL_DEF_LVL5_DEFAULT
	ISAL_DEF_LVL5_DEFAULT,
#else
	0,
#endif
#ifdef ISAL_DEF_LVL6_DEFAULT
	ISAL_DEF_LVL6_DEFAULT,
#else
	0,
#endif
#ifdef ISAL_DEF_LVL7_DEFAULT
	ISAL_DEF_LVL7_DEFAULT,
#else
	0,
#endif
#ifdef ISAL_DEF_LVL8_DEFAULT
	ISAL_DEF_LVL8_DEFAULT,
#else
	0,
#endif
#ifdef ISAL_DEF_LVL9_DEFAULT
	ISAL_DEF_LVL9_DEFAULT,
#else
	0,
#endif
};

int usage(void)
{
	fprintf(stderr,
		"Usage: igzip_stateless_file_perf [options] <infile>\n"
		"  -h          help\n"
		"  -l <level>  isa-l compression level to test, may be specified upto %d times\n"
		"  -z <level>  zlib  compression level to test, may be specified upto %d times\n"
		"  -i <iter>   number of iterations (at least 1)\n", LEVEL_QUEUE_LIMIT,
		LEVEL_QUEUE_LIMIT);
	exit(0);
}

void isal_compress(uint8_t * outbuf, uint64_t * outbuf_size, uint8_t * inbuf,
		   uint64_t inbuf_size, int level)
{
	struct isal_zstream stream;
	uint8_t *level_buf = NULL;

	if (level_size_buf[level] > 0) {
		level_buf = malloc(level_size_buf[level]);
	}

	isal_deflate_init(&stream);
	stream.end_of_stream = 1;	/* Do the entire file at once */
	stream.flush = NO_FLUSH;
	stream.next_in = inbuf;
	stream.avail_in = inbuf_size;
	stream.next_out = outbuf;
	stream.avail_out = *outbuf_size;
	stream.level = level;
	stream.level_buf = level_buf;
	stream.level_buf_size = level_size_buf[level];
	isal_deflate(&stream);
	free(level_buf);

	*outbuf_size = stream.total_out;

	if (stream.avail_in != 0) {
		printf("Compression of input file failed\n");
		exit(0);
	}
}

void isal_inflate_perf(uint8_t * inbuf, uint64_t inbuf_size, uint8_t * outbuf,
		       uint64_t outbuf_size, int iterations)
{
	struct inflate_state state;
	struct perf start, stop;
	int i, check;

	/* Check that data decompresses */
	isal_inflate_init(&state);
	state.next_in = inbuf;
	state.avail_in = inbuf_size;
	state.next_out = outbuf;
	state.avail_out = outbuf_size;

	check = isal_inflate(&state);
	if (check) {
		printf("Error in decompression with error %d\n", check);
		return;
	}

	perf_start(&start);

	for (i = 0; i < iterations; i++) {
		isal_inflate_init(&state);
		state.next_in = inbuf;
		state.avail_in = inbuf_size;
		state.next_out = outbuf;
		state.avail_out = outbuf_size;
		isal_inflate(&state);
	}
	perf_stop(&stop);

	perf_print(stop, start, (long long)outbuf_size * i);

}

int main(int argc, char *argv[])
{
	FILE *in;
	unsigned char *compressbuf, *decompbuf, *filebuf;
	int i, c, iterations = 0;
	uint64_t infile_size, decompbuf_size, compressbuf_size, compress_size;

	char *in_file_name = NULL;

	uint32_t level_queue[LEVEL_QUEUE_LIMIT] = {
		0
	};
	int level_queue_size = 0;
	uint32_t level = 0;

	uint32_t zlib_queue[LEVEL_QUEUE_LIMIT] = {
		0
	};

	int zlib_queue_size = 0;

	while ((c = getopt(argc, argv, OPTARGS)) != -1) {
		switch (c) {
		case 'l':
			if (level_queue_size >= LEVEL_QUEUE_LIMIT) {
				printf("Too many levels specified");
				exit(0);
			}

			level = atoi(optarg);
			if (level > ISAL_DEF_MAX_LEVEL) {
				printf("Unsupported isa-l compression level\n");
				exit(0);
			}

			level_queue[level_queue_size] = level;
			level_queue_size++;
			break;
		case 'z':
			if (zlib_queue_size >= LEVEL_QUEUE_LIMIT) {
				printf("Too many levels specified");
				exit(0);
			}

			level = atoi(optarg);
			if (level > Z_BEST_COMPRESSION) {
				printf("Unsupported zlib compression level\n");
				exit(0);
			}

			zlib_queue[zlib_queue_size] = atoi(optarg);
			zlib_queue_size++;
			break;
		case 'i':
			iterations = atoi(optarg);
			if (iterations < 1)
				usage();
			break;
		case 'h':
		default:
			usage();
			break;
		}
	}

	if (optind < argc) {
		in_file_name = argv[optind];
		in = fopen(in_file_name, "rb");
	} else
		usage();

	/* Allocate space for entire input file and output
	 * (assuming some possible expansion on output size)
	 */
	infile_size = get_filesize(in);

	if (infile_size != 0) {
		decompbuf_size = infile_size;
	} else {
		printf("Error: input file has 0 size\n");
		exit(0);
	}

	if (iterations == 0) {
		iterations = infile_size ? RUN_MEM_SIZE / infile_size : MIN_TEST_LOOPS;
		if (iterations < MIN_TEST_LOOPS)
			iterations = MIN_TEST_LOOPS;
	}

	if (level_queue_size == 0 && zlib_queue_size == 0)
		level_queue_size = 1;

	filebuf = malloc(infile_size);
	if (filebuf == NULL) {
		fprintf(stderr, "Can't allocate temp buffer memory\n");
		exit(0);
	}

	compressbuf_size = 2 * infile_size;
	compressbuf = malloc(compressbuf_size);
	if (compressbuf == NULL) {
		fprintf(stderr, "Can't allocate input buffer memory\n");
		exit(0);
	}
	decompbuf = malloc(decompbuf_size);
	if (decompbuf == NULL) {
		fprintf(stderr, "Can't allocate output buffer memory\n");
		exit(0);
	}

	fread(filebuf, 1, infile_size, in);
	fclose(in);

	for (i = 0; i < level_queue_size; i++) {
		printf("isal_inflate_stateless_perf: compress_type = isal\n");

		compress_size = compressbuf_size;
		isal_compress(compressbuf, &compress_size, filebuf, infile_size,
			      level_queue[i]);

		printf("  file = %s - size = %lu compress_size = %lu iter = %d level = %d\n",
		       in_file_name, infile_size, compress_size, iterations, level_queue[i]);
		printf("  ");
		isal_inflate_perf(compressbuf, compress_size, decompbuf, decompbuf_size,
				  iterations);

	}

	for (i = 0; i < zlib_queue_size; i++) {
		printf("isal_inflate_stateless_perf: compress_type = zlib\n");

		compress_size = compressbuf_size;
		compress2(compressbuf, &compress_size, filebuf, infile_size, zlib_queue[i]);

		printf("  file = %s - size = %lu compress_size = %lu iter = %d level = %d\n",
		       in_file_name, infile_size, compress_size, iterations, zlib_queue[i]);
		printf("  ");

		isal_inflate_perf(compressbuf + 2, compress_size - 2, decompbuf,
				  decompbuf_size, iterations);
	}

	free(compressbuf);
	free(decompbuf);
	free(filebuf);

	return 0;
}
