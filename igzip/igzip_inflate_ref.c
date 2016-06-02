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

#include "igzip_inflate_ref.h"

void inline byte_copy(uint8_t * dest, uint64_t lookback_distance, int repeat_length)
{
	uint8_t *src = dest - lookback_distance;

	for (; repeat_length > 0; repeat_length--)
		*dest++ = *src++;
}

/*
 * Returns integer with first length bits reversed and all higher bits zeroed
 */
uint16_t inline bit_reverse2(uint16_t bits, uint8_t length)
{
	bits = ((bits >> 1) & 0x55555555) | ((bits & 0x55555555) << 1);	// swap bits
	bits = ((bits >> 2) & 0x33333333) | ((bits & 0x33333333) << 2);	// swap pairs
	bits = ((bits >> 4) & 0x0F0F0F0F) | ((bits & 0x0F0F0F0F) << 4);	// swap nibbles
	bits = ((bits >> 8) & 0x00FF00FF) | ((bits & 0x00FF00FF) << 8);	// swap bytes
	return bits >> (16 - length);
}

void inline init_inflate_in_buffer(struct inflate_in_buffer *inflate_in)
{
	inflate_in->read_in = 0;
	inflate_in->read_in_length = 0;
}

void inline set_inflate_in_buffer(struct inflate_in_buffer *inflate_in, uint8_t * in_stream,
				  uint32_t in_size)
{
	inflate_in->next_in = inflate_in->start = in_stream;
	inflate_in->avail_in = in_size;
}

void inline set_inflate_out_buffer(struct inflate_out_buffer *inflate_out,
				   uint8_t * out_stream, uint32_t out_size)
{
	inflate_out->next_out = out_stream;
	inflate_out->avail_out = out_size;
	inflate_out->total_out = 0;
}

void inline inflate_in_clear_bits(struct inflate_in_buffer *inflate_in)
{
	uint8_t bytes;

	bytes = inflate_in->read_in_length / 8;

	inflate_in->read_in = 0;
	inflate_in->read_in_length = 0;
	inflate_in->next_in -= bytes;
	inflate_in->avail_in += bytes;
}

void inline inflate_in_load(struct inflate_in_buffer *inflate_in, int min_required)
{
	uint64_t temp = 0;
	uint8_t new_bytes;

	if (inflate_in->avail_in >= 8) {
		/* If there is enough space to load a 64 bits, load the data and use
		 * that to fill read_in */
		new_bytes = 8 - (inflate_in->read_in_length + 7) / 8;
		temp = *(uint64_t *) inflate_in->next_in;

		inflate_in->read_in |= temp << inflate_in->read_in_length;
		inflate_in->next_in += new_bytes;
		inflate_in->avail_in -= new_bytes;
		inflate_in->read_in_length += new_bytes * 8;

	} else {
		/* Else fill the read_in buffer 1 byte at a time */
		while (inflate_in->read_in_length < 57 && inflate_in->avail_in > 0) {
			temp = *inflate_in->next_in;
			inflate_in->read_in |= temp << inflate_in->read_in_length;
			inflate_in->next_in++;
			inflate_in->avail_in--;
			inflate_in->read_in_length += 8;

		}
	}

}

uint64_t inline inflate_in_peek_bits(struct inflate_in_buffer *inflate_in, uint8_t bit_count)
{
	assert(bit_count < 57);

	/* Load inflate_in if not enough data is in the read_in buffer */
	if (inflate_in->read_in_length < bit_count)
		inflate_in_load(inflate_in, 0);

	return (inflate_in->read_in) & ((1 << bit_count) - 1);
}

void inline inflate_in_shift_bits(struct inflate_in_buffer *inflate_in, uint8_t bit_count)
{

	inflate_in->read_in >>= bit_count;
	inflate_in->read_in_length -= bit_count;
}

uint64_t inline inflate_in_read_bits(struct inflate_in_buffer *inflate_in, uint8_t bit_count)
{
	uint64_t ret;
	assert(bit_count < 57);

	/* Load inflate_in if not enough data is in the read_in buffer */
	if (inflate_in->read_in_length < bit_count)
		inflate_in_load(inflate_in, bit_count);

	ret = (inflate_in->read_in) & ((1 << bit_count) - 1);
	inflate_in->read_in >>= bit_count;
	inflate_in->read_in_length -= bit_count;

	return ret;
}

int inline setup_static_header(struct inflate_state *state)
{
	/* This could be turned into a memcpy of this functions output for
	 * higher speed, but then DECODE_LOOKUP_SIZE couldn't be changed without
	 * regenerating the table. */

	int i;
	struct huff_code lit_code[LIT_LEN + 2];
	struct huff_code dist_code[DIST_LEN + 2];

	/* These tables are based on the static huffman tree described in RFC
	 * 1951 */
	uint16_t lit_count[16] = {
		0, 0, 0, 0, 0, 0, 0, 24, 152, 112, 0, 0, 0, 0, 0, 0
	};
	uint16_t dist_count[16] = {
		0, 0, 0, 0, 0, 32, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
	};

	/* These for loops set the code lengths for the static literal/length
	 * and distance codes defined in the deflate standard RFC 1951 */
	for (i = 0; i < 144; i++)
		lit_code[i].length = 8;

	for (i = 144; i < 256; i++)
		lit_code[i].length = 9;

	for (i = 256; i < 280; i++)
		lit_code[i].length = 7;

	for (i = 280; i < LIT_LEN + 2; i++)
		lit_code[i].length = 8;

	for (i = 0; i < DIST_LEN + 2; i++)
		dist_code[i].length = 5;

	make_inflate_huff_code(&state->lit_huff_code, lit_code, LIT_LEN + 2, lit_count);
	make_inflate_huff_code(&state->dist_huff_code, dist_code, DIST_LEN + 2, dist_count);

	return 0;
}

void inline make_inflate_huff_code(struct inflate_huff_code *result,
				   struct huff_code *huff_code_table, int table_length,
				   uint16_t * count)
{
	int i, j;
	uint16_t code = 0;
	uint16_t next_code[MAX_HUFF_TREE_DEPTH + 1];
	uint16_t long_code_list[LIT_LEN];
	uint32_t long_code_length = 0;
	uint16_t temp_code_list[1 << (15 - DECODE_LOOKUP_SIZE)];
	uint32_t temp_code_length;
	uint32_t long_code_lookup_length = 0;
	uint32_t max_length;
	uint16_t first_bits;
	uint32_t code_length;
	uint16_t long_bits;
	uint16_t min_increment;

	memset(result, 0, sizeof(struct inflate_huff_code));

	next_code[0] = code;

	for (i = 1; i < MAX_HUFF_TREE_DEPTH + 1; i++)
		next_code[i] = (next_code[i - 1] + count[i - 1]) << 1;

	for (i = 0; i < table_length; i++) {
		if (huff_code_table[i].length != 0) {
			/* Determine the code for symbol i */
			huff_code_table[i].code =
			    bit_reverse2(next_code[huff_code_table[i].length],
					huff_code_table[i].length);

			next_code[huff_code_table[i].length] += 1;

			if (huff_code_table[i].length <= DECODE_LOOKUP_SIZE) {
				/* Set lookup table to return the current symbol
				 * concatenated with the code length when the
				 * first DECODE_LENGTH bits of the address are
				 * the same as the code for the current
				 * symbol. The first 9 bits are the code, bits
				 * 14:10 are the code length, bit 15 is a flag
				 * representing this is a symbol*/
				for (j = 0; j < (1 << (DECODE_LOOKUP_SIZE -
						       huff_code_table[i].length)); j++)

					result->small_code_lookup[(j <<
								   huff_code_table[i].length) +
								  huff_code_table[i].code]
					    = i | (huff_code_table[i].length) << 9;

			} else {
				/* Store the element in a list of elements with long codes. */
				long_code_list[long_code_length] = i;
				long_code_length++;
			}
		}
	}

	for (i = 0; i < long_code_length; i++) {
		/*Set the look up table to point to a hint where the symbol can be found
		 * in the list of long codes and add the current symbol to the list of
		 * long codes. */
		if (huff_code_table[long_code_list[i]].code == 0xFFFF)
			continue;

		max_length = huff_code_table[long_code_list[i]].length;
		first_bits =
		    huff_code_table[long_code_list[i]].code & ((1 << DECODE_LOOKUP_SIZE) - 1);

		temp_code_list[0] = long_code_list[i];
		temp_code_length = 1;

		for (j = i + 1; j < long_code_length; j++) {
			if ((huff_code_table[long_code_list[j]].code &
			     ((1 << DECODE_LOOKUP_SIZE) - 1)) == first_bits) {
				if (max_length < huff_code_table[long_code_list[j]].length)
					max_length = huff_code_table[long_code_list[j]].length;
				temp_code_list[temp_code_length] = long_code_list[j];
				temp_code_length++;
			}
		}

		for (j = 0; j < temp_code_length; j++) {
			code_length = huff_code_table[temp_code_list[j]].length;
			long_bits =
			    huff_code_table[temp_code_list[j]].code >> DECODE_LOOKUP_SIZE;
			min_increment = 1 << (code_length - DECODE_LOOKUP_SIZE);
			for (; long_bits < (1 << (max_length - DECODE_LOOKUP_SIZE));
			     long_bits += min_increment) {
				result->long_code_lookup[long_code_lookup_length + long_bits] =
				    temp_code_list[j] | (code_length << 9);
			}
			huff_code_table[temp_code_list[j]].code = 0xFFFF;
		}
		result->small_code_lookup[first_bits] =
		    long_code_lookup_length | (max_length << 9) | 0x8000;
		long_code_lookup_length += 1 << (max_length - DECODE_LOOKUP_SIZE);

	}
}

uint16_t inline decode_next(struct inflate_in_buffer *in_buffer,
			    struct inflate_huff_code *huff_code)
{
	uint16_t next_bits;
	uint16_t next_sym;

	next_bits = inflate_in_peek_bits(in_buffer, DECODE_LOOKUP_SIZE);

	/* next_sym is a possible symbol decoded from next_bits. If bit 15 is 0,
	 * next_code is a symbol. Bits 9:0 represent the symbol, and bits 14:10
	 * represent the length of that symbols huffman code. If next_sym is not
	 * a symbol, it provides a hint of where the large symbols containin
	 * this code are located. Note the hint is at largest the location the
	 * first actual symbol in the long code list.*/
	next_sym = huff_code->small_code_lookup[next_bits];

	if (next_sym < 0x8000) {
		/* Return symbol found if next_code is a complete huffman code
		 * and shift in buffer over by the length of the next_code */
		inflate_in_shift_bits(in_buffer, next_sym >> 9);

		return next_sym & 0x1FF;

	} else {
		/* If a symbol is not found, perform a linear search of the long code
		 * list starting from the hint in next_sym */
		next_bits = inflate_in_peek_bits(in_buffer, (next_sym - 0x8000) >> 9);
		next_sym =
		    huff_code->long_code_lookup[(next_sym & 0x1FF) +
						(next_bits >> DECODE_LOOKUP_SIZE)];
		inflate_in_shift_bits(in_buffer, next_sym >> 9);
		return next_sym & 0x1FF;

	}
}

int inline setup_dynamic_header(struct inflate_state *state)
{
	int i, j;
	struct huff_code code_huff[CODE_LEN_CODES];
	struct huff_code lit_and_dist_huff[LIT_LEN + DIST_LEN];
	struct huff_code *previous = NULL, *current;
	struct inflate_huff_code inflate_code_huff;
	uint8_t hclen, hdist, hlit;
	uint16_t code_count[16], lit_count[16], dist_count[16];
	uint16_t *count;
	uint16_t symbol;

	/* This order is defined in RFC 1951 page 13 */
	const uint8_t code_length_code_order[CODE_LEN_CODES] = {
		0x10, 0x11, 0x12, 0x00, 0x08, 0x07, 0x09, 0x06,
		0x0a, 0x05, 0x0b, 0x04, 0x0c, 0x03, 0x0d, 0x02,
		0x0e, 0x01, 0x0f
	};

	memset(code_count, 0, sizeof(code_count));
	memset(lit_count, 0, sizeof(lit_count));
	memset(dist_count, 0, sizeof(dist_count));
	memset(code_huff, 0, sizeof(code_huff));
	memset(lit_and_dist_huff, 0, sizeof(lit_and_dist_huff));

	/* These variables are defined in the deflate standard, RFC 1951 */
	hlit = inflate_in_read_bits(&state->in_buffer, 5);
	hdist = inflate_in_read_bits(&state->in_buffer, 5);
	hclen = inflate_in_read_bits(&state->in_buffer, 4);

	/* Create the code huffman code for decoding the lit/len and dist huffman codes */
	for (i = 0; i < hclen + 4; i++) {
		code_huff[code_length_code_order[i]].length =
		    inflate_in_read_bits(&state->in_buffer, 3);

		code_count[code_huff[code_length_code_order[i]].length] += 1;
	}

	if (state->in_buffer.read_in_length < 0)
		return END_OF_INPUT;

	make_inflate_huff_code(&inflate_code_huff, code_huff, CODE_LEN_CODES, code_count);

	/* Decode the lit/len and dist huffman codes using the code huffman code */
	count = lit_count;
	current = lit_and_dist_huff;

	while (current < lit_and_dist_huff + LIT_LEN + hdist + 1) {
		/* If finished decoding the lit/len huffman code, start decoding
		 * the distance code these decodings are in the same loop
		 * because the len/lit and dist huffman codes are run length
		 * encoded together. */
		if (current == lit_and_dist_huff + 257 + hlit)
			current = lit_and_dist_huff + LIT_LEN;

		if (current == lit_and_dist_huff + LIT_LEN)
			count = dist_count;

		symbol = decode_next(&state->in_buffer, &inflate_code_huff);

		if (state->in_buffer.read_in_length < 0)
			return END_OF_INPUT;

		if (symbol < 16) {
			/* If a length is found, update the current lit/len/dist
			 * to have length symbol */
			count[symbol]++;
			current->length = symbol;
			previous = current;
			current++;

		} else if (symbol == 16) {
			/* If a repeat length is found, update the next repeat
			 * length lit/len/dist elements to have the value of the
			 * repeated length */
			if (previous == NULL)	/* No elements available to be repeated */
				return INVALID_BLOCK_HEADER;

			i = 3 + inflate_in_read_bits(&state->in_buffer, 2);
			for (j = 0; j < i; j++) {
				*current = *previous;
				count[current->length]++;
				previous = current;

				if (current == lit_and_dist_huff + 256 + hlit) {
					current = lit_and_dist_huff + LIT_LEN;
					count = dist_count;

				} else
					current++;
			}

		} else if (symbol == 17) {
			/* If a repeat zeroes if found, update then next
			 * repeated zeroes length lit/len/dist elements to have
			 * length 0. */
			i = 3 + inflate_in_read_bits(&state->in_buffer, 3);

			for (j = 0; j < i; j++) {
				previous = current;

				if (current == lit_and_dist_huff + 256 + hlit) {
					current = lit_and_dist_huff + LIT_LEN;
					count = dist_count;

				} else
					current++;
			}

		} else if (symbol == 18) {
			/* If a repeat zeroes if found, update then next
			 * repeated zeroes length lit/len/dist elements to have
			 * length 0. */
			i = 11 + inflate_in_read_bits(&state->in_buffer, 7);

			for (j = 0; j < i; j++) {
				previous = current;

				if (current == lit_and_dist_huff + 256 + hlit) {
					current = lit_and_dist_huff + LIT_LEN;
					count = dist_count;

				} else
					current++;
			}
		} else
			return INVALID_BLOCK_HEADER;

	}

	if (state->in_buffer.read_in_length < 0)
		return END_OF_INPUT;

	make_inflate_huff_code(&state->lit_huff_code, lit_and_dist_huff, LIT_LEN, lit_count);
	make_inflate_huff_code(&state->dist_huff_code, &lit_and_dist_huff[LIT_LEN], DIST_LEN,
			       dist_count);

	return 0;
}

int read_header(struct inflate_state *state)
{
	state->new_block = 0;

	/* btype and bfinal are defined in RFC 1951, bfinal represents whether
	 * the current block is the end of block, and btype represents the
	 * encoding method on the current block. */
	state->bfinal = inflate_in_read_bits(&state->in_buffer, 1);
	state->btype = inflate_in_read_bits(&state->in_buffer, 2);

	if (state->in_buffer.read_in_length < 0)
		return END_OF_INPUT;

	if (state->btype == 0) {
		inflate_in_clear_bits(&state->in_buffer);
		return 0;

	} else if (state->btype == 1)
		return setup_static_header(state);

	else if (state->btype == 2)
		return setup_dynamic_header(state);

	return INVALID_BLOCK_HEADER;
}

void igzip_inflate_init(struct inflate_state *state, uint8_t * in_stream, uint32_t in_size,
			uint8_t * out_stream, uint64_t out_size)
{

	init_inflate_in_buffer(&state->in_buffer);

	set_inflate_in_buffer(&state->in_buffer, in_stream, in_size);
	set_inflate_out_buffer(&state->out_buffer, out_stream, out_size);

	state->new_block = 1;
	state->bfinal = 0;
}

int igzip_inflate(struct inflate_state *state)
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

	while (state->new_block == 0 || state->bfinal == 0) {
		if (state->new_block != 0) {
			tmp = read_header(state);

			if (tmp)
				return tmp;
		}

		if (state->btype == 0) {
			/* If the block is uncompressed, perform a memcopy while
			 * updating state data */
			if (state->in_buffer.avail_in < 4)
				return END_OF_INPUT;

			len = *(uint16_t *) state->in_buffer.next_in;
			state->in_buffer.next_in += 2;
			nlen = *(uint16_t *) state->in_buffer.next_in;
			state->in_buffer.next_in += 2;

			/* Check if len and nlen match */
			if (len != (~nlen & 0xffff))
				return INVALID_NON_COMPRESSED_BLOCK_LENGTH;

			if (state->out_buffer.avail_out < len)
				return OUT_BUFFER_OVERFLOW;

			if (state->in_buffer.avail_in < len)
				len = state->in_buffer.avail_in;

			else
				state->new_block = 1;

			memcpy(state->out_buffer.next_out, state->in_buffer.next_in, len);

			state->out_buffer.next_out += len;
			state->out_buffer.avail_out -= len;
			state->out_buffer.total_out += len;
			state->in_buffer.next_in += len;
			state->in_buffer.avail_in -= len + 4;

			if (state->in_buffer.avail_in == 0 && state->new_block == 0)
				return END_OF_INPUT;

		} else {
			/* Else decode a huffman encoded block */
			while (state->new_block == 0) {
				/* While not at the end of block, decode the next
				 * symbol */

				next_lit =
				    decode_next(&state->in_buffer, &state->lit_huff_code);

				if (state->in_buffer.read_in_length < 0)
					return END_OF_INPUT;

				if (next_lit < 256) {
					/* If the next symbol is a literal,
					 * write out the symbol and update state
					 * data accordingly. */
					if (state->out_buffer.avail_out < 1)
						return OUT_BUFFER_OVERFLOW;

					*state->out_buffer.next_out = next_lit;
					state->out_buffer.next_out++;
					state->out_buffer.avail_out--;
					state->out_buffer.total_out++;

				} else if (next_lit == 256) {
					/* If the next symbol is the end of
					 * block, update the state data
					 * accordingly */
					state->new_block = 1;

				} else if (next_lit < 286) {
					/* Else if the next symbol is a repeat
					 * length, read in the length extra
					 * bits, the distance code, the distance
					 * extra bits. Then write out the
					 * corresponding data and update the
					 * state data accordingly*/
					repeat_length =
					    len_start[next_lit - 257] +
					    inflate_in_read_bits(&state->in_buffer,
								 len_extra_bit_count[next_lit -
										     257]);

					if (state->out_buffer.avail_out < repeat_length)
						return OUT_BUFFER_OVERFLOW;

					next_dist = decode_next(&state->in_buffer,
								&state->dist_huff_code);

					look_back_dist = dist_start[next_dist] +
					    inflate_in_read_bits(&state->in_buffer,
								 dist_extra_bit_count
								 [next_dist]);

					if (state->in_buffer.read_in_length < 0)
						return END_OF_INPUT;

					if (look_back_dist > state->out_buffer.total_out)
						return INVALID_LOOK_BACK_DISTANCE;

					if (look_back_dist > repeat_length) {
						memcpy(state->out_buffer.next_out,
						       state->out_buffer.next_out -
						       look_back_dist, repeat_length);
					} else
						byte_copy(state->out_buffer.next_out,
							  look_back_dist, repeat_length);

					state->out_buffer.next_out += repeat_length;
					state->out_buffer.avail_out -= repeat_length;
					state->out_buffer.total_out += repeat_length;

				} else
					/* Else the read in bits do not
					 * correspond to any valid symbol */
					return INVALID_SYMBOL;
			}
		}
	}
	state->in_buffer.next_in -= state->in_buffer.read_in_length / 8;
	state->in_buffer.avail_in += state->in_buffer.read_in_length / 8;

	return DECOMPRESSION_FINISHED;
}
