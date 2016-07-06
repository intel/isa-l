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

#define DECODE_LOOKUP_SIZE 12

#if DECODE_LOOKUP_SIZE > 15
# undef DECODE_LOOKUP_SIZE
# define DECODE_LOOKUP_SIZE 15
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

/*
 * Data structure used to store a Huffman code for fast lookup. It works by
 * performing a lookup in small_code_lookup that hopefully yields the correct
 * symbol. Otherwise a lookup into long_code_lookup is performed to find the
 * correct symbol. The details of how this works follows:
 *
 * Let i be some index into small_code_lookup and let e be the associated
 * element.  Bit 15 in e is a flag. If bit 15 is not set, then index i contains
 * a Huffman code for a symbol which has length at most DECODE_LOOKUP_SIZE. Bits
 * 0 through 8 are the symbol associated with that code and bits 9 through 12 of
 * e represent the number of bits in the code. If bit 15 is set, the i
 * corresponds to the first DECODE_LOOKUP_SIZE bits of a Huffman code which has
 * length longer than DECODE_LOOKUP_SIZE. In this case, bits 0 through 8
 * represent an offset into long_code_lookup table and bits 9 through 12
 * represent the maximum length of a Huffman code starting with the bits in the
 * index i. The offset into long_code_lookup is for an array associated with all
 * codes which start with the bits in i.
 *
 * The elements of long_code_lookup are in the same format as small_code_lookup,
 * except bit 15 is never set. Let i be a number made up of DECODE_LOOKUP_SIZE
 * bits.  Then all Huffman codes which start with DECODE_LOOKUP_SIZE bits are
 * stored in an array starting at index h in long_code_lookup. This index h is
 * stored in bits 0 through 9 at index i in small_code_lookup. The index j is an
 * index of this array if the number of bits contained in j and i is the number
 * of bits in the longest huff_code starting with the bits of i. The symbol
 * stored at index j is the symbol whose huffcode can be found in (j <<
 * DECODE_LOOKUP_SIZE) | i. Note these arrays will be stored sorted in order of
 * maximum Huffman code length.
 *
 * The following are explanations for sizes of the tables:
 *
 * Since small_code_lookup is a lookup on DECODE_LOOKUP_SIZE bits, it must have
 * size 2^DECODE_LOOKUP_SIZE.
 *
 * Since deflate Huffman are stored such that the code size and the code value
 * form an increasing function, At most 2^(15 - DECODE_LOOKUP_SIZE) - 1 elements
 * of long_code_lookup duplicate an existing symbol. Since there are at most 285
 * - DECODE_LOOKUP_SIZE possible symbols contained in long_code lookup. Rounding
 * this to the nearest 16 byte boundary yields the size of long_code_lookup of
 * 288 + 2^(15 - DECODE_LOOKUP_SIZE).
 *
 * Note that DECODE_LOOKUP_SIZE can be any length even though the offset in
 * small_lookup_code is 9 bits long because the increasing relationship between
 * code length and code value forces the maximum offset to be less than 288.
 */
struct inflate_huff_code{
	uint16_t small_code_lookup[ 1 << (DECODE_LOOKUP_SIZE)];
	uint16_t long_code_lookup[288 + (1 << (15 - DECODE_LOOKUP_SIZE))];
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
