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

#define LIT_LEN IGZIP_LIT_LEN
#define DIST_LEN IGZIP_DIST_LEN
#define CODE_LEN_CODES 19
#define HUFF_LEN 19
#ifdef LONGER_HUFFTABLE
# define DCODE_OFFSET 26
#else
# define DCODE_OFFSET 20
#endif
#define DYN_HDR_START_LEN 17
#define MAX_HISTHEAP_SIZE LIT_LEN
#define MAX_HUFF_TREE_DEPTH 15
#define D      IGZIP_D	/* Amount of history */

#define MAX_DEFLATE_CODE_LEN 15
#define MAX_SAFE_LIT_CODE_LEN 13
#define MAX_SAFE_DIST_CODE_LEN 12

#define LONG_DIST_TABLE_SIZE 8192
#define SHORT_DIST_TABLE_SIZE 1024
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

/**
 * @brief Structure used to store huffman codes
 */
struct huff_code {
	uint16_t code;
	uint8_t length;
};

/**
 * @brief Binary tree used to store and create a huffman tree.
 */
struct huff_tree {
	uint16_t value;
	uint64_t frequency;
	struct huff_tree *left;
	struct huff_tree *right;
};

/**
 * @brief Nodes in a doubly linked list.
 */
struct linked_list_node {
	uint16_t value;
	struct linked_list_node *next;
	struct linked_list_node *previous;
};

/**
 * @brief This structure is a doubly linked list.
 */
struct linked_list {
	uint64_t length;
	struct linked_list_node *start;
	struct linked_list_node *end;
};

/**
 * @brief  This is a binary minheap structure which stores huffman trees.
 * @details The huffman trees are sorted by the frequency of the root.
 * The structure is represented in a fixed sized array.
 */
struct histheap {
	struct huff_tree tree[MAX_HISTHEAP_SIZE];
	uint16_t size;
};

/**
 * @brief Inserts a hufftree into a histheap.
 * @param element: the hufftree to be inserted
 * @param heap: the heap which element is being inserted into.
 * @requires This function assumes the heap has enough allocated space.
 * @returns Returns the index in heap of the inserted element
 */
int heap_push(struct huff_tree element, struct histheap *heap);

/**
 * @brief  Removes the top element from the heap and returns it.
 */
struct huff_tree heap_pop(struct histheap *heap);

/**
 * @brief Removes the first element from list and returns it.
 */
struct linked_list_node *pop_from_front(struct linked_list *list);

/**
 * @brief Adds new_element to the front of list.
 */
void append_to_front(struct linked_list *list, struct linked_list_node *new_element);

/**
 * @brief Adds new_element to the end of list.
 */
void append_to_back(struct linked_list *list, struct linked_list_node *new_element);

/**
 * @brief  Returns the deflate symbol value for a repeat length.
*/
uint32_t convert_length_to_len_sym(uint32_t length);

/**
 * @brief  Returns the deflate symbol value for a look back distance.
 */
uint32_t convert_dist_to_dist_sym(uint32_t dist);

/**
 * Constructs a huffman tree on tree_array which only uses elements with non-zero frequency.
 * @requires Assumes there will be at least two symbols in the produced tree.
 * @requires tree_array must have length at least 2*size-1, and size must be less than 286.
 * @param tree_array: array of huff_tree elements used to create a huffman tree, the first
 * size elements of the array are the leaf elements in the huffman tree.
 * @param  histogram: a histogram of the frequency of elements in tree_array.
 * @param size: the number of leaf elements in the huffman tree.
*/
struct huff_tree create_symbol_subset_huff_tree(struct huff_tree *tree_array,
						uint64_t * histogram, uint32_t size);

/**
 * @brief  Construct a huffman tree on tree_array which uses every symbol.
 * @requires tree_array must have length at least 2*size-1, and size must be less than 286.
 * @param tree_array: array of huff_tree elements used to create a huffman tree, the first
 * @param size elements of the array are the leaf elements in the huffman tree.
 * @param histogram: a histogram of the frequency of elements in tree_array.
 * @param size: the number of leaf elements in the huffman tree.
 */
struct huff_tree create_huff_tree(struct huff_tree *tree_array, uint64_t * histogram,
				  uint32_t size);

/**
 * @brief Creates a deflate compliant huffman tree with maximum depth max_depth.
 * @details The huffman tree is represented as a lookup table.
 * @param huff_lookup_table: The output lookup table.
 * @param table_length: The length of table.
 * @param root: the input huffman tree the created tree is based on.
 * @param max_depth: maximum depth the huffman tree can have
 * @returns Returns 0 if sucessful and returns 1 otherwise.
 */
int create_huff_lookup(struct huff_code *huff_lookup_table, int table_length,
		       struct huff_tree root, uint8_t max_depth);

/**
 * @brief Determines the code length for every value in a huffmant tree.
 * @param huff_lookup_table: An output lookup table used to store the code lengths
 * @param corresponding to the possible values
 * @param count: An output histogram representing code length versus number of occurences.
 * @param current_node: A node of the huffman tree being analyzed currently.
 * @param current_depth: The depth of the current node in the huffman tree.
 * @returns Returns 0 if sucessful and returns 1 otherwise.
 */
int find_code_lengths(struct huff_code *huff_lookup_table, uint16_t * count,
		      struct huff_tree root, uint8_t max_depth);

/**
 * @brief Creates an array of linked lists.
 * @detail Each linked list contains all the elements with codes of a given length for
 * lengths less than 16, and an list for all elements with codes at least 16. These lists
 * are sorted by frequency from least frequent to most frequent within any given code length.
 * @param depth_array: depth_array[i] is a linked list of elements with code length i
 * @param linked_lists: An input structure the linked lists in depth array are built on.
 * @param current_node: the current node being visited in a huffman tree
 * @param current_depth: the depth of current_node in a huffman tree
 */
void huffman_tree_traversal(struct linked_list *depth_array,
			    struct linked_list_node *linked_lists, uint16_t * extra_nodes,
			    uint8_t max_depth, struct huff_tree current_node,
			    uint16_t current_depth);

/**
 * @brief Determines the code each element of a deflate compliant huffman tree and stores
 * it in a lookup table
 * @requires table has been initialized to already contain the code length for each element.
 * @param table: A lookup table used to store the codes.
 * @param table_length: The length of table.
 * @param count: a histogram representing the number of occurences of codes of a given length
 */
void set_huff_codes(struct huff_code *table, int table_length, uint16_t * count);

/* Reverse the first length bits in bits and returns that value */
uint16_t bit_reverse(uint16_t bits, uint8_t length);

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
int create_header(uint8_t *header, uint32_t header_length, struct huff_code *lit_huff_table,
		  struct huff_code *dist_huff_table, uint32_t end_of_block);

/**
 * @brief Creates a run length encoded reprsentation of huff_table.
 * @details Also creates a histogram representing the frequency of each symbols
 * @returns Returns the number of symbols written into huffman_rep.
 * @param huffman_rep: The output run length encoded version of huff_table.
 * @param histogram: The output histogram of frequencies of elements in huffman_rep.
 * @param extra_bits: An output table storing extra bits associated with huffman_rep.
 * @param huff_table: The input huffman_table or concatonation of huffman_tables.
 * @parma len: The length of huff_table.
 */
uint16_t create_huffman_rep(uint16_t * huffman_rep, uint64_t * histogram,
			    uint16_t * extra_bits, struct huff_code *huff_table, uint16_t len);

/**
 * @brief Flushes the symbols for a repeat of last_code for length run_length into huffman_rep.
 * @param huffman_rep: pointer to array containing the output huffman_rep.
 * @param histogram: histogram of elements seen in huffman_rep.
 * @param extra_bits: an array holding extra bits for the corresponding symbol in huffman_rep.
 * @param huff_table: a concatenated list of huffman lookup tables.
 * @param current_index: The next spot elements will be written in huffman_rep.
 */
uint16_t flush_repeats(uint16_t * huffman_rep, uint64_t * histogram, uint16_t * extra_bits,
		       uint16_t last_code, uint16_t run_length, uint16_t current_index);

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
int create_huffman_header(uint8_t *header, uint32_t header_length, struct huff_code *lookup_table,
			  uint16_t * huffman_rep, uint16_t * extra_bits,
			  uint16_t huffman_rep_length, uint32_t end_of_block, uint32_t hclen,
			  uint32_t hlit, uint32_t hdist);

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
#endif
