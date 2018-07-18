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

#define _FILE_OFFSET_BITS 64
#include <stdio.h>
#include <assert.h>
#include <getopt.h>
#include "huff_codes.h"
#include "igzip_lib.h"
#include "test.h"

#include <zlib.h>

#define BUF_SIZE 1024
#define MIN_TEST_LOOPS   1
#ifndef RUN_MEM_SIZE
# define RUN_MEM_SIZE 200000000
#endif

#define OPTARGS "hl:f:z:i:d:stub:"

#define COMPRESSION_QUEUE_LIMIT 32

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

enum {
	ISAL_STATELESS,
	ISAL_STATEFUL,
	ZLIB
};

struct compress_strategy {
	int32_t mode;
	int32_t level;
};

struct inflate_modes {
	int32_t stateless;
	int32_t stateful;
	int32_t zlib;
};

struct perf_info {
	char *file_name;
	size_t file_size;
	size_t deflate_size;
	uint32_t deflate_iter;
	uint32_t inflate_iter;
	uint32_t inblock_size;
	struct compress_strategy strategy;
	uint32_t inflate_mode;
	struct perf start;
	struct perf stop;
};

int usage(void)
{
	fprintf(stderr,
		"Usage: igzip_inflate_perf [options] <infile>\n"
		"  -h          help, print this message\n"
		"  The options -l, -f, -z may be used up to %d times\n"
		"  -l <level>  isa-l stateless deflate level to test\n"
		"  -f <level>  isa-l stateful deflate level to test\n"
		"  -z <level>  zlib  deflate level to test\n"
		"  -d <iter>   number of iterations for deflate (at least 1)\n"
		"  -i <iter>   number of iterations for inflate (at least 1)\n"
		"  -s          performance test isa-l stateful inflate\n"
		"  -t          performance test isa-l stateless inflate\n"
		"  -u          performance test zlib inflate\n"
		"  -b <size>   input buffer size, applies to stateful options (-f,-s)\n",
		COMPRESSION_QUEUE_LIMIT);
	exit(0);
}

void print_perf_info_line(struct perf_info *info)
{
	printf("igzip_perf-> compress level: %d compress_iterations: %d "
	       "decompress_iterations: %d block_size: %d\n",
	       info->strategy.level, info->deflate_iter, info->inflate_iter,
	       info->inblock_size);
}

void print_file_line(struct perf_info *info)
{
	printf("  file info-> name: %s file_size: %lu compress_size: %lu ratio: %2.02f%%\n",
	       info->file_name, info->file_size, info->deflate_size,
	       100.0 * info->deflate_size / info->file_size);
}

void print_deflate_perf_line(struct perf_info *info)
{
	if (info->strategy.mode == ISAL_STATELESS)
		printf("    isal_stateless_deflate-> ");
	else if (info->strategy.mode == ISAL_STATEFUL)
		printf("    isal_stateful_deflate->  ");
	else if (info->strategy.mode == ZLIB)
		printf("    zlib_deflate->           ");

	perf_print(info->stop, info->start, info->file_size * info->deflate_iter);
}

void print_inflate_perf_line(struct perf_info *info)
{
	if (info->inflate_mode == ISAL_STATELESS)
		printf("    isal_stateless_inflate-> ");
	else if (info->inflate_mode == ISAL_STATEFUL)
		printf("    isal_stateful_inflate->  ");
	else if (info->inflate_mode == ZLIB)
		printf("    zlib_inflate->           ");

	perf_print(info->stop, info->start, info->file_size * info->inflate_iter);
}

int isal_deflate_perf(uint8_t * outbuf, uint64_t * outbuf_size, uint8_t * inbuf,
		      uint64_t inbuf_size, int level, int iterations, struct perf *start,
		      struct perf *stop)
{
	struct isal_zstream stream;
	uint8_t *level_buf = NULL;
	int i, check;

	if (level_size_buf[level] > 0) {
		level_buf = malloc(level_size_buf[level]);
		if (level_buf == NULL)
			return 1;
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
	check = isal_deflate_stateless(&stream);

	if (check || stream.avail_in)
		return 1;

	perf_start(start);
	for (i = 0; i < iterations; i++) {
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
		isal_deflate_stateless(&stream);
	}
	perf_stop(stop);

	*outbuf_size = stream.total_out;
	return 0;
}

int isal_deflate_stateful_perf(uint8_t * outbuf, uint64_t * outbuf_size, uint8_t * inbuf,
			       uint64_t inbuf_size, int level, uint64_t in_block_size,
			       int iterations, struct perf *start, struct perf *stop)
{
	struct isal_zstream stream;
	uint8_t *level_buf = NULL;
	int i, check;
	uint64_t inbuf_remaining;

	if (in_block_size == 0)
		in_block_size = inbuf_size;

	if (level_size_buf[level] > 0) {
		level_buf = malloc(level_size_buf[level]);
		if (level_buf == NULL)
			return 1;
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
	check = isal_deflate(&stream);

	if (check || stream.avail_in)
		return 1;

	perf_start(start);

	for (i = 0; i < iterations; i++) {
		inbuf_remaining = inbuf_size;
		isal_deflate_init(&stream);
		stream.flush = NO_FLUSH;
		stream.next_in = inbuf;
		stream.next_out = outbuf;
		stream.avail_out = *outbuf_size;
		stream.level = level;
		stream.level_buf = level_buf;
		stream.level_buf_size = level_size_buf[level];

		while (ISAL_DECOMP_OK == check && inbuf_remaining > in_block_size) {
			stream.avail_in = in_block_size;
			inbuf_remaining -= in_block_size;
			check = isal_deflate(&stream);
		}
		if (ISAL_DECOMP_OK == check) {
			stream.avail_in = inbuf_remaining;
			stream.end_of_stream = 1;
			check = isal_deflate(&stream);
		}

		if (ISAL_DECOMP_OK != check || stream.avail_in > 0)
			return 1;
	}

	perf_stop(stop);

	*outbuf_size = stream.total_out;
	return 0;

}

int zlib_deflate_perf(uint8_t * outbuf, uint64_t * outbuf_size, uint8_t * inbuf,
		      uint64_t inbuf_size, int level, int iterations, struct perf *start,
		      struct perf *stop)
{
	int i, check;
	z_stream gstream;

	gstream.next_in = inbuf;
	gstream.avail_in = inbuf_size;
	gstream.zalloc = Z_NULL;
	gstream.zfree = Z_NULL;
	gstream.opaque = Z_NULL;
	if (0 != deflateInit2(&gstream, level, Z_DEFLATED, -15, 9, Z_DEFAULT_STRATEGY))
		return 1;

	gstream.next_out = outbuf;
	gstream.avail_out = *outbuf_size;
	check = deflate(&gstream, Z_FINISH);
	if (check != 1)
		return 1;

	perf_start(start);
	for (i = 0; i < iterations; i++) {
		if (0 != deflateReset(&gstream))
			return 1;

		gstream.next_in = inbuf;
		gstream.avail_in = inbuf_size;
		gstream.next_out = outbuf;
		gstream.avail_out = *outbuf_size;
		deflate(&gstream, Z_FINISH);
	}
	perf_stop(stop);
	deflateEnd(&gstream);

	*outbuf_size = gstream.total_out;
	return 0;
}

int isal_inflate_perf(uint8_t * inbuf, uint64_t inbuf_size, uint8_t * outbuf,
		      uint64_t outbuf_size, uint8_t * filebuf, uint64_t file_size,
		      int iterations, struct perf *start, struct perf *stop)
{
	struct inflate_state state;
	int i, check;

	/* Check that data decompresses */
	state.next_in = inbuf;
	state.avail_in = inbuf_size;
	state.next_out = outbuf;
	state.avail_out = outbuf_size;
	state.crc_flag = ISAL_DEFLATE;

	check = isal_inflate_stateless(&state);
	if (check || state.total_out != file_size || memcmp(outbuf, filebuf, file_size))
		return 1;

	perf_start(start);

	for (i = 0; i < iterations; i++) {
		state.next_in = inbuf;
		state.avail_in = inbuf_size;
		state.next_out = outbuf;
		state.avail_out = outbuf_size;
		state.crc_flag = ISAL_DEFLATE;

		isal_inflate_stateless(&state);
	}
	perf_stop(stop);

	return 0;
}

int isal_inflate_stateful_perf(uint8_t * inbuf, uint64_t inbuf_size, uint8_t * outbuf,
			       uint64_t outbuf_size, uint8_t * filebuf, uint64_t file_size,
			       uint64_t in_block_size, int iterations, struct perf *start,
			       struct perf *stop)
{
	struct inflate_state state;
	int i, check;
	uint64_t inbuf_remaining;

	if (in_block_size == 0)
		in_block_size = inbuf_size;
	/* Check that data decompresses */
	isal_inflate_init(&state);
	state.next_in = inbuf;
	state.avail_in = inbuf_size;
	state.next_out = outbuf;
	state.avail_out = outbuf_size;

	check = isal_inflate(&state);
	if (check || state.total_out != file_size || memcmp(outbuf, filebuf, file_size))
		return 1;

	perf_start(start);

	for (i = 0; i < iterations; i++) {
		isal_inflate_init(&state);
		state.next_in = inbuf;
		state.next_out = outbuf;
		state.avail_out = outbuf_size;
		inbuf_remaining = inbuf_size;

		while (ISAL_DECOMP_OK == check && inbuf_remaining >= in_block_size) {
			state.avail_in = in_block_size;
			inbuf_remaining -= in_block_size;
			check = isal_inflate(&state);
		}
		if (ISAL_DECOMP_OK == check && inbuf_remaining > 0) {
			state.avail_in = inbuf_remaining;
			check = isal_inflate(&state);
		}

		if (ISAL_DECOMP_OK != check || state.avail_in > 0)
			return 1;
	}
	perf_stop(stop);

	return 0;

}

int zlib_inflate_perf(uint8_t * inbuf, uint64_t inbuf_size, uint8_t * outbuf,
		      uint64_t outbuf_size, uint8_t * filebuf, uint64_t file_size,
		      int iterations, struct perf *start, struct perf *stop)
{
	int i, check;
	z_stream gstream;

	gstream.next_in = inbuf;
	gstream.avail_in = inbuf_size;
	gstream.zalloc = Z_NULL;
	gstream.zfree = Z_NULL;
	gstream.opaque = Z_NULL;
	if (0 != inflateInit2(&gstream, -15))
		return 1;

	gstream.next_out = outbuf;
	gstream.avail_out = outbuf_size;
	check = inflate(&gstream, Z_FINISH);
	if (check != 1 || gstream.total_out != file_size || memcmp(outbuf, filebuf, file_size))
		return 1;

	perf_start(start);
	for (i = 0; i < iterations; i++) {
		if (0 != inflateReset(&gstream))
			return 1;

		gstream.next_in = inbuf;
		gstream.avail_in = inbuf_size;
		gstream.next_out = outbuf;
		gstream.avail_out = outbuf_size;
		inflate(&gstream, Z_FINISH);
	}
	perf_stop(stop);
	inflateEnd(&gstream);
	return 0;
}

int main(int argc, char *argv[])
{
	FILE *in = NULL;
	unsigned char *compressbuf, *decompbuf, *filebuf;
	int i, c, ret = 0;
	uint64_t decompbuf_size, compressbuf_size;

	struct compress_strategy compression_queue[COMPRESSION_QUEUE_LIMIT];

	int compression_queue_size = 0;
	struct compress_strategy compress_strat;
	struct inflate_modes inflate_strat = { 0 };
	struct perf_info info;
	memset(&info, 0, sizeof(info));

	while ((c = getopt(argc, argv, OPTARGS)) != -1) {
		switch (c) {
		case 'l':
			if (compression_queue_size >= COMPRESSION_QUEUE_LIMIT) {
				printf("Too many levels specified");
				exit(0);
			}

			compress_strat.mode = ISAL_STATELESS;
			compress_strat.level = atoi(optarg);
			if (compress_strat.level > ISAL_DEF_MAX_LEVEL) {
				printf("Unsupported isa-l compression level\n");
				exit(0);
			}

			compression_queue[compression_queue_size] = compress_strat;
			compression_queue_size++;
			break;
		case 'f':
			if (compression_queue_size >= COMPRESSION_QUEUE_LIMIT) {
				printf("Too many levels specified");
				exit(0);
			}

			compress_strat.mode = ISAL_STATEFUL;
			compress_strat.level = atoi(optarg);
			if (compress_strat.level > ISAL_DEF_MAX_LEVEL) {
				printf("Unsupported isa-l compression level\n");
				exit(0);
			}

			compression_queue[compression_queue_size] = compress_strat;
			compression_queue_size++;
			break;
		case 'z':
			if (compression_queue_size >= COMPRESSION_QUEUE_LIMIT) {
				printf("Too many levels specified");
				exit(0);
			}

			compress_strat.mode = ZLIB;
			compress_strat.level = atoi(optarg);
			if (compress_strat.level > Z_BEST_COMPRESSION) {
				printf("Unsupported zlib compression level\n");
				exit(0);
			}
			compression_queue[compression_queue_size] = compress_strat;
			compression_queue_size++;
			break;
		case 'i':
			info.inflate_iter = atoi(optarg);
			if (info.inflate_iter < 1)
				usage();
			break;
		case 'd':
			info.deflate_iter = atoi(optarg);
			if (info.deflate_iter < 1)
				usage();
			break;
		case 's':
			inflate_strat.stateful = 1;
			break;
		case 't':
			inflate_strat.stateless = 1;
			break;
		case 'u':
			inflate_strat.zlib = 1;
			break;
		case 'b':
			inflate_strat.stateful = 1;
			info.inblock_size = atoi(optarg);
			break;
		case 'h':
		default:
			usage();
			break;
		}
	}

	if (optind >= argc)
		usage();

	if (!inflate_strat.stateless && !inflate_strat.stateful && !inflate_strat.zlib) {
		if (info.inblock_size == 0)
			inflate_strat.stateless = 1;
		else
			inflate_strat.stateful = 1;
	}

	/* Allocate space for entire input file and output
	 * (assuming some possible expansion on output size)
	 */
	info.file_name = argv[optind];
	in = fopen(info.file_name, "rb");
	if (NULL == in) {
		printf("Error: Can not find file %s\n", info.file_name);
		exit(0);
	}

	info.file_size = get_filesize(in);
	if (info.file_size == 0) {
		printf("Error: input file has 0 size\n");
		exit(0);
	}

	decompbuf_size = info.file_size;
	if (info.inflate_iter == 0) {
		info.inflate_iter =
		    info.file_size ? RUN_MEM_SIZE / info.file_size : MIN_TEST_LOOPS;
		if (info.inflate_iter < MIN_TEST_LOOPS)
			info.inflate_iter = MIN_TEST_LOOPS;
	}

	decompbuf_size = info.file_size;
	if (info.deflate_iter == 0) {
		info.deflate_iter =
		    info.file_size ? RUN_MEM_SIZE / info.file_size : MIN_TEST_LOOPS;
		if (info.deflate_iter < MIN_TEST_LOOPS)
			info.deflate_iter = MIN_TEST_LOOPS;
	}

	if (compression_queue_size == 0) {
		if (info.inblock_size == 0)
			compression_queue[0].mode = ISAL_STATELESS;
		else
			compression_queue[0].mode = ISAL_STATEFUL;
		compression_queue[0].level = 1;
		compression_queue_size = 1;
	}

	filebuf = malloc(info.file_size);
	if (filebuf == NULL) {
		fprintf(stderr, "Can't allocate temp buffer memory\n");
		exit(0);
	}

	compressbuf_size = 2 * info.file_size;
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

	if (info.file_size != fread(filebuf, 1, info.file_size, in)) {
		fprintf(stderr, "Could not read in all input\n");
		exit(0);
	}
	fclose(in);

	for (i = 0; i < compression_queue_size; i++) {
		if (i > 0)
			printf("\n\n");

		info.strategy = compression_queue[i];
		print_perf_info_line(&info);

		info.deflate_size = compressbuf_size;

		if (info.strategy.mode == ISAL_STATELESS)
			ret = isal_deflate_perf(compressbuf, &info.deflate_size, filebuf,
						info.file_size, compression_queue[i].level,
						info.deflate_iter, &info.start, &info.stop);
		else if (info.strategy.mode == ISAL_STATEFUL)
			ret =
			    isal_deflate_stateful_perf(compressbuf, &info.deflate_size,
						       filebuf, info.file_size,
						       compression_queue[i].level,
						       info.inblock_size, info.deflate_iter,
						       &info.start, &info.stop);
		else if (info.strategy.mode == ZLIB)
			ret = zlib_deflate_perf(compressbuf, &info.deflate_size, filebuf,
						info.file_size, compression_queue[i].level,
						info.deflate_iter, &info.start, &info.stop);

		if (ret) {
			printf("  Error in compression\n");
			continue;
		}

		print_file_line(&info);
		printf("\n");
		print_deflate_perf_line(&info);
		printf("\n");

		if (inflate_strat.stateless) {
			info.inflate_mode = ISAL_STATELESS;
			ret = isal_inflate_perf(compressbuf, info.deflate_size, decompbuf,
						decompbuf_size, filebuf, info.file_size,
						info.inflate_iter, &info.start, &info.stop);
			if (ret)
				printf("    Error in isal stateless inflate\n");
			else
				print_inflate_perf_line(&info);
		}

		if (inflate_strat.stateful) {
			info.inflate_mode = ISAL_STATEFUL;
			ret =
			    isal_inflate_stateful_perf(compressbuf, info.deflate_size,
						       decompbuf, decompbuf_size, filebuf,
						       info.file_size, info.inblock_size,
						       info.inflate_iter, &info.start,
						       &info.stop);

			if (ret)
				printf("    Error in isal stateful inflate\n");
			else
				print_inflate_perf_line(&info);
		}

		if (inflate_strat.zlib) {
			info.inflate_mode = ZLIB;
			ret = zlib_inflate_perf(compressbuf, info.deflate_size, decompbuf,
						decompbuf_size, filebuf, info.file_size,
						info.inflate_iter, &info.start, &info.stop);
			if (ret)
				printf("    Error in zlib inflate\n");
			else
				print_inflate_perf_line(&info);
		}
	}

	free(compressbuf);
	free(decompbuf);
	free(filebuf);
	return 0;
}
