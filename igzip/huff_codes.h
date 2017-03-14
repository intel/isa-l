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

#ifndef HUFF_CODES_H
#define HUFF_CODES_H

#include <immintrin.h>
#include <stdint.h>
#include <string.h>
#include <assert.h>
#include "igzip_lib.h"
#include "bitbuf2.h"

#ifdef _MSC_VER
# include <intrin.h>
#else
# include <x86intrin.h>
#endif

#define LIT_LEN ISAL_DEF_LIT_LEN_SYMBOLS
#define DIST_LEN ISAL_DEF_DIST_SYMBOLS
#define CODE_LEN_CODES 19
#define HUFF_LEN 19
#ifdef LONGER_HUFFTABLE
# define DCODE_OFFSET 26
#else
# define DCODE_OFFSET 0
#endif
#define DYN_HDR_START_LEN 17
#define MAX_HISTHEAP_SIZE LIT_LEN
#define MAX_HUFF_TREE_DEPTH 15
#define D      IGZIP_HIST_SIZE	/* Amount of history */

#define MAX_DEFLATE_CODE_LEN 15
#define MAX_SAFE_LIT_CODE_LEN 13
#define MAX_SAFE_DIST_CODE_LEN 12

#define LONG_DIST_TABLE_SIZE 8192
#define SHORT_DIST_TABLE_SIZE 2
#define LEN_TABLE_SIZE 256
#define LIT_TABLE_SIZE 257
#define LAST_BLOCK 1

#define LEN_EXTRA_BITS_START 264
#define LEN_EXTRA_BITS_INTERVAL 4
#define DIST_EXTRA_BITS_START 3
#define DIST_EXTRA_BITS_INTERVAL 2

#define INVALID_LIT_LEN_HUFFCODE 1
#define INVALID_DIST_HUFFCODE 1
#define INVALID_HUFFCODE 1

#define HASH_MASK  (IGZIP_HASH_SIZE - 1)
#define SHORTEST_MATCH  4

#define LENGTH_BITS 5
#define FREQ_SHIFT 16
#define FREQ_MASK_HI (0xFFFFFFFFFFFF0000)
#define DEPTH_SHIFT 24
#define DEPTH_MASK 0x7F
#define DEPTH_MASK_HI (DEPTH_MASK << DEPTH_SHIFT)
#define DEPTH_1       (1 << DEPTH_SHIFT)
#define HEAP_TREE_SIZE (3*MAX_HISTHEAP_SIZE + 1)
#define HEAP_TREE_NODE_START (HEAP_TREE_SIZE-1)
#define MAX_BL_CODE_LEN 7

/**
 * @brief Structure used to store huffman codes
 */
struct huff_code {
	union {
		struct {
			uint16_t code;
			uint8_t extra_bit_count;
			uint8_t length;
		};
		struct {
			uint32_t code_and_extra:24;
			uint32_t length2:8;
		};
	};
};

struct tree_node {
	uint32_t child;
	uint32_t depth;
};

struct heap_tree {
	union {
		uint64_t heap[HEAP_TREE_SIZE];
		uint64_t code_len_count[MAX_HUFF_TREE_DEPTH + 1];
		struct tree_node tree[HEAP_TREE_SIZE];
	};
};

struct rl_code {
	uint8_t code;
	uint8_t extra_bits;
};

struct hufftables_icf {
	struct huff_code dist_table[31];
	struct huff_code lit_len_table[513];
};

/**
 * @brief  Returns the deflate symbol value for a repeat length.
*/
uint32_t convert_length_to_len_sym(uint32_t length);

/**
 * @brief  Returns the deflate symbol value for a look back distance.
 */
uint32_t convert_dist_to_dist_sym(uint32_t dist);

/**
 * @brief Determines the code each element of a deflate compliant huffman tree and stores
 * it in a lookup table
 * @requires table has been initialized to already contain the code length for each element.
 * @param table: A lookup table used to store the codes.
 * @param table_length: The length of table.
 * @param count: a histogram representing the number of occurences of codes of a given length
 */
uint32_t set_huff_codes(struct huff_code *table, int table_length, uint32_t * count);

/**
 * @brief Checks if a literal/length huffman table can be stored in the igzip hufftables files.
 * @param table: A literal/length huffman code lookup table.
 * @returns index of the first symbol which fails and 0xFFFF otherwise.
 */
uint16_t valid_lit_huff_table(struct huff_code *huff_code_table);

/**
 * @brief Checks if a distance huffman table can be stored in the igzip hufftables files.
 * @param table: A distance huffman code lookup table.
 * @returnsthe index of the first symbol which fails and 0xFFFF otherwise.
 */
uint16_t valid_dist_huff_table(struct huff_code *huff_code_table);

/**
 * @brief Creates the dynamic huffman deflate header.
 * @returns Returns the  length of header in bits.
 * @requires This function requires header is large enough to store the whole header.
 * @param header: The output header.
 * @param lit_huff_table: A literal/length code huffman lookup table.
 * @param dist_huff_table: A distance huffman code lookup table.
 * @param end_of_block: Value determining whether end of block header is produced or not;
 * 0 corresponds to not end of block and all other inputs correspond to end of block.
 */
int create_header(struct BitBuf2 *header_bitbuf, struct rl_code *huffman_rep, uint32_t length,
		uint64_t * histogram, uint32_t hlit, uint32_t hdist, uint32_t end_of_block);

/**
 * @brief Creates the header for run length encoded huffman trees.
 * @param header: the output header.
 * @param lookup_table: a huffman lookup table.
 * @param huffman_rep: a run length encoded huffman tree.
 * @extra_bits: extra bits associated with the corresponding spot in huffman_rep
 * @param huffman_rep_length: the length of huffman_rep.
 * @param end_of_block: Value determining whether end of block header is produced or not;
 * 0 corresponds to not end of block and all other inputs correspond to end of block.
 * @param hclen: Length of huffman code for huffman codes minus 4.
 * @param hlit: Length of literal/length table minus 257.
 * @parm hdist: Length of distance table minus 1.
 */
int create_huffman_header(struct BitBuf2 *header_bitbuf, struct huff_code *lookup_table,
			  struct rl_code * huffman_rep, uint16_t huffman_rep_length,
			  uint32_t end_of_block, uint32_t hclen, uint32_t hlit,
			  uint32_t hdist);

/**
 * @brief Creates a two table representation of huffman codes.
 * @param code_table: output table containing the code
 * @param code_size_table: output table containing the code length
 * @param length: the lenght of hufftable
 * @param hufftable: a huffman lookup table
 */
void create_code_tables(uint16_t * code_table, uint8_t * code_length_table,
			uint32_t length, struct huff_code *hufftable);

/**
 * @brief Creates a packed representation of length huffman codes.
 * @details In packed_table, bits 32:8 contain the extra bits appended to the huffman
 * code and bits 8:0 contain the code length.
 * @param packed_table: the output table
 * @param length: the length of lit_len_hufftable
 * @param lit_len_hufftable: a literal/length huffman lookup table
 */
void create_packed_len_table(uint32_t * packed_table, struct huff_code *lit_len_hufftable);

/**
 * @brief Creates a packed representation of distance  huffman codes.
 * @details In packed_table, bits 32:8 contain the extra bits appended to the huffman
 * code and bits 8:0 contain the code length.
 * @param packed_table: the output table
 * @param length: the length of lit_len_hufftable
 * @param dist_hufftable: a distance huffman lookup table
 */
void create_packed_dist_table(uint32_t * packed_table, uint32_t length,
			      struct huff_code *dist_hufftable);

/**
 * @brief Checks to see if the hufftable is usable by igzip
 *
 * @param lit_len_hufftable: literal/lenght huffman code
 * @param dist_hufftable: distance huffman code
 * @returns Returns 0 if the table is usable
 */
int are_hufftables_useable(struct huff_code *lit_len_hufftable,
			   struct huff_code *dist_hufftable);

/**
 * @brief Creates a representation of the huffman code from a histogram used to
 * decompress the intermediate compression format.
 *
 * @param bb: bitbuf structure where the header huffman code header is written
 * @param hufftables: output huffman code representation
 * @param hist: histogram used to generat huffman code
 * @param end_of_block: flag whether this is the final huffman code
 */
void
create_hufftables_icf(struct BitBuf2 *bb, struct hufftables_icf * hufftables,
                  struct isal_mod_hist *hist, uint32_t end_of_block);

#endif
