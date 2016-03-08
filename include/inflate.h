#ifndef INFLATE_H
#define INFLATE_H

#include <stdint.h>

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
# define TMP1 ((2 << 8 ) + 1)
# define TMP2 (2 << (15 - DECODE_LOOKUP_SIZE))
# define MAX_LONG_CODE (TMP1 * TMP2 + 32)
#else
# define MAX_LONG_CODE (2 << (15 - DECODE_LOOKUP_SIZE)) + (2 << (8 + DECODE_LOOKUP_SIZE)) + 32
#endif

/* Buffer used to manage decompressed output */
struct inflate_out_buffer{
	uint8_t *start_out;
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

/* Initialize a struct inflate_state for deflate compressed input data at in_stream and to output
 * data into out_stream */
void isal_inflate_init(struct inflate_state *state, uint8_t *in_stream, uint32_t in_size,
			uint8_t *out_stream, uint64_t out_size);

/* Decompress a deflate data. This function assumes a call to igzip_inflate_init
 * has been made to set up the state structure to allow for decompression.*/
int isal_inflate_stateless(struct inflate_state *state);

#endif
