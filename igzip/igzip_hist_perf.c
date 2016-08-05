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
#include <assert.h>
#include <string.h>
#include "igzip_lib.h"
#include "test.h"

#define BUF_SIZE 1024
#define MIN_TEST_LOOPS   8
#ifndef RUN_MEM_SIZE
# define RUN_MEM_SIZE 2000000000
#endif

extern uint64_t inflate_in_read_bits(struct inflate_state *, uint8_t);
extern int read_header(struct inflate_state *);
extern uint16_t decode_next_large(struct inflate_state *, struct inflate_huff_code_large *);
extern uint16_t decode_next_small(struct inflate_state *, struct inflate_huff_code_small *);

/* Inflates and fills a histogram of lit, len, and dist codes seen in non-type 0 blocks.*/
int isal_inflate_hist(struct inflate_state *state, struct isal_huff_histogram *histogram)
{
	/* The following tables are based on the tables in the deflate standard,
	 * RFC 1951 page 11. */
	const uint16_t len_start[29] = {
		0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0a,
		0x0b, 0x0d, 0x0f, 0x11, 0x13, 0x17, 0x1b, 0x1f,
		0x23, 0x2b, 0x33, 0x3b, 0x43, 0x53, 0x63, 0x73,
		0x83, 0xa3, 0xc3, 0xe3, 0x102
	};
	const uint8_t len_extra_bit_count[29] = {
		0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,
		0x1, 0x1, 0x1, 0x1, 0x2, 0x2, 0x2, 0x2,
		0x3, 0x3, 0x3, 0x3, 0x4, 0x4, 0x4, 0x4,
		0x5, 0x5, 0x5, 0x5, 0x0
	};
	const uint32_t dist_start[30] = {
		0x0001, 0x0002, 0x0003, 0x0004, 0x0005, 0x0007, 0x0009, 0x000d,
		0x0011, 0x0019, 0x0021, 0x0031, 0x0041, 0x0061, 0x0081, 0x00c1,
		0x0101, 0x0181, 0x0201, 0x0301, 0x0401, 0x0601, 0x0801, 0x0c01,
		0x1001, 0x1801, 0x2001, 0x3001, 0x4001, 0x6001
	};
	const uint8_t dist_extra_bit_count[30] = {
		0x0, 0x0, 0x0, 0x0, 0x1, 0x1, 0x2, 0x2,
		0x3, 0x3, 0x4, 0x4, 0x5, 0x5, 0x6, 0x6,
		0x7, 0x7, 0x8, 0x8, 0x9, 0x9, 0xa, 0xa,
		0xb, 0xb, 0xc, 0xc, 0xd, 0xd
	};

	uint16_t next_lit, len, nlen;
	uint8_t next_dist;
	uint32_t repeat_length;
	uint32_t look_back_dist;
	uint32_t tmp;

	memset(histogram, 0, sizeof(struct isal_huff_histogram));
	while (state->block_state != ISAL_BLOCK_INPUT_DONE) {
		if (state->block_state == ISAL_BLOCK_NEW_HDR) {
			tmp = read_header(state);

			if (tmp)
				return tmp;
		}

		if (state->block_state == ISAL_BLOCK_TYPE0) {
			/* If the block is uncompressed, update state data accordingly */
			if (state->avail_in < 4)
				return ISAL_END_INPUT;

			len = *(uint16_t *) state->next_in;
			state->next_in += 2;
			nlen = *(uint16_t *) state->next_in;
			state->next_in += 2;

			/* Check if len and nlen match */
			if (len != (~nlen & 0xffff))
				return ISAL_INVALID_BLOCK;

			if (state->avail_in < len)
				len = state->avail_in;
			else
				state->block_state = ISAL_BLOCK_NEW_HDR;

			state->total_out += len;
			state->next_in += len;
			state->avail_in -= len + 4;

			if (state->avail_in == 0 && state->block_state == 0)
				return ISAL_END_INPUT;

		} else {
			/* Else decode a huffman encoded block */
			while (state->block_state == ISAL_BLOCK_CODED) {
				/* While not at the end of block, decode the next
				 * symbol */
				next_lit = decode_next_large(state, &state->lit_huff_code);

				histogram->lit_len_histogram[next_lit] += 1;

				if (state->read_in_length < 0)
					return ISAL_END_INPUT;

				if (next_lit < 256)
					/* Next symbol is a literal */
					state->total_out++;

				else if (next_lit == 256)
					/* Next symbol is end of block */
					state->block_state = ISAL_BLOCK_NEW_HDR;

				else if (next_lit < 286) {
					/* Next symbol is a repeat length followed by a 
					   lookback distance */
					repeat_length =
					    len_start[next_lit - 257] +
					    inflate_in_read_bits(state,
								 len_extra_bit_count[next_lit -
										     257]);

					next_dist = decode_next_small(state,
								      &state->dist_huff_code);

					histogram->dist_histogram[next_dist] += 1;

					look_back_dist = dist_start[next_dist] +
					    inflate_in_read_bits(state,
								 dist_extra_bit_count
								 [next_dist]);

					if (state->read_in_length < 0)
						return ISAL_END_INPUT;

					if (look_back_dist > state->total_out)
						return ISAL_INVALID_LOOKBACK;

					state->total_out += repeat_length;

				} else
					return ISAL_INVALID_SYMBOL;
			}
		}

		if (state->bfinal != 0 && state->block_state == ISAL_BLOCK_NEW_HDR)
			state->block_state = ISAL_BLOCK_INPUT_DONE;
	}
	state->next_in -= state->read_in_length / 8;
	state->avail_in += state->read_in_length / 8;

	return ISAL_DECOMP_OK;
}

int get_filesize(FILE * f)
{
	int curr, end;

	curr = ftell(f);	/* Save current position */
	fseek(f, 0L, SEEK_END);
	end = ftell(f);
	fseek(f, curr, SEEK_SET);	/* Restore position */
	return end;
}

void print_histogram(struct isal_huff_histogram *histogram)
{
	int i;
	printf("Lit Len histogram");
	for (i = 0; i < ISAL_DEF_LIT_LEN_SYMBOLS; i++) {
		if (i % 16 == 0)
			printf("\n");
		else
			printf(", ");
		printf("%4lu", histogram->lit_len_histogram[i]);
	}
	printf("\n");

	printf("Dist histogram");
	for (i = 0; i < ISAL_DEF_DIST_SYMBOLS; i++) {
		if (i % 16 == 0)
			printf("\n");
		else
			printf(", ");
		printf("%4lu", histogram->dist_histogram[i]);
	}
	printf("\n");
}

void print_diff_histogram(struct isal_huff_histogram *histogram1,
			  struct isal_huff_histogram *histogram2)
{
	int i;
	double relative_error;
	printf("Lit Len histogram relative error");
	for (i = 0; i < ISAL_DEF_LIT_LEN_SYMBOLS; i++) {
		if (i % 16 == 0)
			printf("\n");
		else
			printf(", ");

		if (histogram1->lit_len_histogram[i] == histogram2->lit_len_histogram[i]) {
			printf(" % 4.0f %%", 0.0);
		} else {
			relative_error =
			    abs(histogram1->lit_len_histogram[i] -
				histogram2->lit_len_histogram[i]);
			relative_error = relative_error / histogram1->lit_len_histogram[i];
			relative_error = 100.0 * relative_error;
			printf("~% 4.0f %%", relative_error);
		}
	}
	printf("\n");

	printf("Dist histogram relative error");
	for (i = 0; i < ISAL_DEF_DIST_SYMBOLS; i++) {
		if (i % 16 == 0)
			printf("\n");
		else
			printf(", ");

		if (histogram1->dist_histogram[i] == histogram2->dist_histogram[i]) {
			printf(" % 4.0f %%", 0.0);
		} else {
			relative_error =
			    abs(histogram1->dist_histogram[i] - histogram2->dist_histogram[i]);
			relative_error = relative_error / histogram1->dist_histogram[i];
			relative_error = 100.0 * relative_error;
			printf("~% 4.0f %%", relative_error);
		}

	}
	printf("\n");
}

int main(int argc, char *argv[])
{
	FILE *in;
	unsigned char *inbuf, *outbuf;
	int i, infile_size, outbuf_size, iterations, avail_in;
	struct isal_huff_histogram histogram1, histogram2;
	struct isal_hufftables hufftables_custom;
	struct isal_zstream stream;
	struct inflate_state gstream;

	memset(&histogram1, 0, sizeof(histogram1));
	memset(&histogram2, 0, sizeof(histogram2));

	if (argc > 3 || argc < 2) {
		fprintf(stderr, "Usage: igzip_file_perf  infile [outfile]\n"
			"\t - Runs multiple iterations of igzip on a file to "
			"get more accurate time results.\n");
		exit(0);
	}
	in = fopen(argv[1], "rb");
	if (!in) {
		fprintf(stderr, "Can't open %s for reading\n", argv[1]);
		exit(0);
	}

	/* Allocate space for entire input file and output
	 * (assuming some possible expansion on output size)
	 */
	infile_size = get_filesize(in);
	outbuf_size = 2 * infile_size;

	if (infile_size != 0)
		iterations = RUN_MEM_SIZE / infile_size;
	else
		iterations = MIN_TEST_LOOPS;

	if (iterations < MIN_TEST_LOOPS)
		iterations = MIN_TEST_LOOPS;

	inbuf = malloc(infile_size);
	outbuf = malloc(outbuf_size);
	if (inbuf == NULL) {
		fprintf(stderr, "Can't allocate input buffer memory\n");
		exit(0);
	}

	if (outbuf == NULL) {
		fprintf(stderr, "Can't allocate output buffer memory\n");
		exit(0);
	}

	avail_in = fread(inbuf, 1, infile_size, in);
	if (avail_in != infile_size) {
		fprintf(stderr, "Couldn't fit all of input file into buffer\n");
		exit(0);
	}

	struct perf start, stop;
	perf_start(&start);

	for (i = 0; i < iterations; i++)
		isal_update_histogram(inbuf, infile_size, &histogram1);
	perf_stop(&stop);

	printf("  file %s - in_size=%d iter=%d\n", argv[1], infile_size, i);
	printf("igzip_file: ");
	perf_print(stop, start, (long long)infile_size * i);

	memset(&histogram1, 0, sizeof(histogram1));

	isal_update_histogram(inbuf, infile_size, &histogram1);

	isal_create_hufftables(&hufftables_custom, &histogram1);

	isal_deflate_init(&stream);
	stream.end_of_stream = 1;	/* Do the entire file at once */
	stream.flush = NO_FLUSH;
	stream.next_in = inbuf;
	stream.avail_in = infile_size;
	stream.next_out = outbuf;
	stream.avail_out = outbuf_size;
	stream.hufftables = &hufftables_custom;
	isal_deflate_stateless(&stream);

	isal_inflate_init(&gstream);
	gstream.next_in = outbuf;
	gstream.avail_in = outbuf_size;
	isal_inflate_hist(&gstream, &histogram2);

	printf("Histogram Error \n");
	print_diff_histogram(&histogram1, &histogram2);

	fclose(in);
	fflush(0);
	return 0;
}
