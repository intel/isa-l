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

#ifndef INFLATE_H
#define INFLATE_H

#include <stdint.h>
#include "huff_codes.h"

#define DECOMPRESSION_FINISHED 0
#define END_OF_INPUT 1
#define OUT_BUFFER_OVERFLOW 2
#define INVALID_BLOCK_HEADER 3
#define INVALID_SYMBOL 4
#define INVALID_NON_COMPRESSED_BLOCK_LENGTH 5
#define INVALID_LOOK_BACK_DISTANCE 6

#define DECODE_LOOKUP_SIZE 10

#if DECODE_LOOKUP_SIZE > 15
# undef DECODE_LOOKUP_SIZE
# define DECODE_LOOKUP_SIZE 15
#endif

#if DECODE_LOOKUP_SIZE > 7
# define MAX_LONG_CODE ((2 << 8) + 1) * (2 << (15 - DECODE_LOOKUP_SIZE)) + 32
#else
# define MAX_LONG_CODE (2 << (15 - DECODE_LOOKUP_SIZE)) + (2 << (8 + DECODE_LOOKUP_SIZE)) + 32
#endif

/* Buffer used to manage decompressed output */
struct inflate_out_buffer{
	uint8_t *next_out;
	uint32_t avail_out;
	uint32_t total_out;
};

/* Buffer used to manager compressed input */
struct inflate_in_buffer{
	uint8_t *start;
	uint8_t *next_in;
	uint32_t avail_in;
	uint64_t read_in;
	int32_t read_in_length;
};

/* Data structure used to store a huffman code for fast look up */
struct inflate_huff_code{
	uint16_t small_code_lookup[ 1 << (DECODE_LOOKUP_SIZE)];
	uint16_t long_code_lookup[MAX_LONG_CODE];
};

/* Structure contained current state of decompression of data */
struct inflate_state {
	struct inflate_out_buffer out_buffer;
	struct inflate_in_buffer in_buffer;
	struct inflate_huff_code lit_huff_code;
	struct inflate_huff_code dist_huff_code;
	uint8_t new_block;
	uint8_t bfinal;
	uint8_t btype;
};

/*Performs a copy of length repeat_length data starting at dest -
 * lookback_distance into dest. This copy copies data previously copied when the
 * src buffer and the dest buffer overlap. */
void byte_copy(uint8_t *dest, uint64_t lookback_distance, int repeat_length);

/* Initialize a struct in_buffer for use */
void init_inflate_in_buffer(struct inflate_in_buffer *inflate_in);

/* Set up the in_stream used for the in_buffer*/
void set_inflate_in_buffer(struct inflate_in_buffer *inflate_in, uint8_t *in_stream,
			uint32_t in_size);

/* Set up the out_stream used for the out_buffer */
void set_inflate_out_buffer(struct inflate_out_buffer *inflate_out, uint8_t *out_stream,
			uint32_t out_size);

/* Load data from the in_stream into a buffer to allow for handling unaligned data*/
void inflate_in_load(struct inflate_in_buffer *inflate_in, int min_load);

/* Returns the next bit_count bits from the in stream*/
uint64_t inflate_in_peek_bits(struct inflate_in_buffer *inflate_in, uint8_t bit_count);

/* Shifts the in stream over by bit-count bits */
void inflate_in_shift_bits(struct inflate_in_buffer *inflate_in, uint8_t bit_count);

/* Returns the next bit_count bits from the in stream and shifts the stream over
 * by bit-count bits */
uint64_t inflate_in_read_bits(struct inflate_in_buffer *inflate_in, uint8_t bit_count);

/* Sets the inflate_huff_codes in state to be the huffcodes corresponding to the
 * deflate static header */
int setup_static_header(struct inflate_state *state);

/* Sets result to the inflate_huff_code corresponding to the huffcode defined by
 * the lengths in huff_code_table,where count is a histogram of the appearance
 * of each code length */
void make_inflate_huff_code(struct inflate_huff_code *result, struct huff_code *huff_code_table,
			int table_length, uint16_t * count);

/* Decodes the next symbol symbol in in_buffer using the huff code defined by
 * huff_code */
uint16_t decode_next(struct inflate_in_buffer *in_buffer, struct inflate_huff_code *huff_code);

/* Reads data from the in_buffer and sets the huff code corresponding to that
 * data */
int setup_dynamic_header(struct inflate_state *state);

/* Reads in the header pointed to by in_stream and sets up state to reflect that
 * header information*/
int read_header(struct inflate_state *state);

/* Initialize a struct inflate_state for deflate compressed input data at in_stream and to output
 * data into out_stream */
void igzip_inflate_init(struct inflate_state *state, uint8_t *in_stream, uint32_t in_size,
			uint8_t *out_stream, uint64_t out_size);

/* Decompress a deflate data. This function assumes a call to igzip_inflate_init
 * has been made to set up the state structure to allow for decompression.*/
int igzip_inflate(struct inflate_state *state);

#endif //INFLATE_H
