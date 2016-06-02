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

#include <immintrin.h>
#include <stdint.h>
#include <string.h>
#include <assert.h>
#include "igzip_lib.h"
#include "huff_codes.h"
#include "huffman.h"

#define LENGTH_BITS 5

/* The order code length codes are written in the dynamic code header. This is
 * defined in RFC 1951 page 13 */
static const uint8_t code_length_code_order[] =
    { 16, 17, 18, 0, 8, 7, 9, 6, 10, 5, 11, 4, 12, 3, 13, 2, 14, 1, 15 };

int heap_push(struct huff_tree element, struct histheap *heap)
{
	uint16_t index;
	uint16_t parent;
	assert(heap->size < MAX_HISTHEAP_SIZE);
	index = heap->size;
	heap->size += 1;
	parent = (index - 1) / 2;
	while ((index != 0) && (heap->tree[parent].frequency > element.frequency)) {
		heap->tree[index] = heap->tree[parent];
		index = parent;
		parent = (index - 1) / 2;

	}
	heap->tree[index] = element;

	return index;
}

struct huff_tree heap_pop(struct histheap *heap)
{
	struct huff_tree root, temp;
	uint16_t index = 0;
	uint16_t child = 1;
	assert(heap->size > 0);
	root = heap->tree[index];
	heap->size--;
	heap->tree[index] = heap->tree[heap->size];

	while (child + 1 < heap->size) {
		if (heap->tree[child].frequency < heap->tree[index].frequency
		    || heap->tree[child + 1].frequency < heap->tree[index].frequency) {
			if (heap->tree[child].frequency > heap->tree[child + 1].frequency)
				child += 1;
			temp = heap->tree[index];
			heap->tree[index] = heap->tree[child];
			heap->tree[child] = temp;
			index = child;
			child = 2 * child + 1;
		} else {
			break;
		}
	}

	if (child < heap->size) {
		if (heap->tree[child].frequency < heap->tree[index].frequency) {
			temp = heap->tree[index];
			heap->tree[index] = heap->tree[child];
			heap->tree[child] = temp;
		}
	}

	return root;

}

struct linked_list_node *pop_from_front(struct linked_list *list)
{
	struct linked_list_node *temp;

	temp = list->start;
	if (list->start != NULL) {
		list->start = list->start->next;
		if (list->start != NULL)
			list->start->previous = NULL;
		else
			list->end = NULL;
		list->length -= 1;
	}
	return temp;
}

void append_to_front(struct linked_list *list, struct linked_list_node *new_element)
{
	new_element->next = list->start;
	new_element->previous = NULL;
	if (list->start != NULL)
		list->start->previous = new_element;
	else
		list->end = new_element;
	list->start = new_element;
	list->length += 1;

	return;
}

void append_to_back(struct linked_list *list, struct linked_list_node *new_element)
{
	new_element->previous = list->end;
	new_element->next = NULL;
	if (list->end != NULL)
		list->end->next = new_element;
	else
		list->start = new_element;
	list->end = new_element;
	list->length += 1;

	return;
}

void isal_update_histogram(uint8_t * start_stream, int length,
			   struct isal_huff_histogram *histogram)
{
	uint32_t literal = 0, hash;
	uint8_t *last_seen[HASH_SIZE];
	uint8_t *current, *seen, *end_stream, *next_hash, *end;
	uint32_t match_length;
	uint32_t dist;
	uint64_t *lit_len_histogram = histogram->lit_len_histogram;
	uint64_t *dist_histogram = histogram->dist_histogram;

	if (length <= 0)
		return;

	end_stream = start_stream + length;
	memset(last_seen, 0, sizeof(last_seen));	/* Initialize last_seen to be 0. */
	for (current = start_stream; current < end_stream - 3; current++) {
		literal = *(uint32_t *) current;
		hash = compute_hash(literal) & HASH_MASK;
		seen = last_seen[hash];
		last_seen[hash] = current;
		dist = current - seen;
		if (dist < D) {
			match_length = compare258(seen, current, end_stream - current);
			if (match_length >= SHORTEST_MATCH) {
				next_hash = current;
#ifdef LIMIT_HASH_UPDATE
				end = next_hash + 3;
#else
				end = next_hash + match_length;
#endif
				if (end > end_stream - 3)
					end = end_stream - 3;
				next_hash++;
				for (; next_hash < end; next_hash++) {
					literal = *(uint32_t *) next_hash;
					hash = compute_hash(literal) & HASH_MASK;
					last_seen[hash] = next_hash;
				}

				dist_histogram[convert_dist_to_dist_sym(dist)] += 1;
				lit_len_histogram[convert_length_to_len_sym(match_length)] +=
				    1;
				current += match_length - 1;
				continue;
			}
		}
		lit_len_histogram[literal & 0xFF] += 1;
	}
	literal = literal >> 8;
	hash = compute_hash(literal) & HASH_MASK;
	seen = last_seen[hash];
	last_seen[hash] = current;
	dist = current - seen;
	if (dist < D) {
		match_length = compare258(seen, current, end_stream - current);
		if (match_length >= SHORTEST_MATCH) {
			dist_histogram[convert_dist_to_dist_sym(dist)] += 1;
			lit_len_histogram[convert_length_to_len_sym(match_length)] += 1;
			lit_len_histogram[256] += 1;
			return;
		}
	} else
		lit_len_histogram[literal & 0xFF] += 1;
	lit_len_histogram[(literal >> 8) & 0xFF] += 1;
	lit_len_histogram[(literal >> 16) & 0xFF] += 1;
	lit_len_histogram[256] += 1;
	return;
}

uint32_t convert_dist_to_dist_sym(uint32_t dist)
{
	assert(dist <= 32768 && dist > 0);
	if (dist <= 2)
		return dist - 1;
	else if (dist <= 4)
		return 0 + (dist - 1) / 1;
	else if (dist <= 8)
		return 2 + (dist - 1) / 2;
	else if (dist <= 16)
		return 4 + (dist - 1) / 4;
	else if (dist <= 32)
		return 6 + (dist - 1) / 8;
	else if (dist <= 64)
		return 8 + (dist - 1) / 16;
	else if (dist <= 128)
		return 10 + (dist - 1) / 32;
	else if (dist <= 256)
		return 12 + (dist - 1) / 64;
	else if (dist <= 512)
		return 14 + (dist - 1) / 128;
	else if (dist <= 1024)
		return 16 + (dist - 1) / 256;
	else if (dist <= 2048)
		return 18 + (dist - 1) / 512;
	else if (dist <= 4096)
		return 20 + (dist - 1) / 1024;
	else if (dist <= 8192)
		return 22 + (dist - 1) / 2048;
	else if (dist <= 16384)
		return 24 + (dist - 1) / 4096;
	else if (dist <= 32768)
		return 26 + (dist - 1) / 8192;
	else
		return ~0;	/* ~0 is an invalid distance code */

}

uint32_t convert_length_to_len_sym(uint32_t length)
{
	assert(length > 2 && length < 259);

	/* Based on tables on page 11 in RFC 1951 */
	if (length < 11)
		return 257 + length - 3;
	else if (length < 19)
		return 261 + (length - 3) / 2;
	else if (length < 35)
		return 265 + (length - 3) / 4;
	else if (length < 67)
		return 269 + (length - 3) / 8;
	else if (length < 131)
		return 273 + (length - 3) / 16;
	else if (length < 258)
		return 277 + (length - 3) / 32;
	else
		return 285;
}

struct huff_tree create_symbol_subset_huff_tree(struct huff_tree *tree_array,
						uint64_t * histogram, uint32_t size)
{
	/* Assumes there are at least 2 symbols. */
	int i;
	uint32_t node_index;
	struct huff_tree tree;
	struct histheap heap;

	heap.size = 0;

	tree.right = tree.left = NULL;

	/* Intitializes heap for construction of the huffman tree */
	for (i = 0; i < size; i++) {
		tree.value = i;
		tree.frequency = histogram[i];
		tree_array[i] = tree;

		/* If symbol does not appear (has frequency 0), ignore it. */
		if (tree_array[i].frequency != 0)
			heap_push(tree, &heap);
	}

	node_index = size;

	/* Construct the huffman tree */
	while (heap.size > 1) {

		tree = heap_pop(&heap);
		tree_array[node_index].frequency = tree.frequency;
		tree_array[node_index].left = &tree_array[tree.value];

		tree = heap_pop(&heap);
		tree_array[node_index].frequency += tree.frequency;
		tree_array[node_index].right = &tree_array[tree.value];

		tree_array[node_index].value = node_index;
		heap_push(tree_array[node_index], &heap);

		node_index += 1;
	}

	return heap_pop(&heap);
}

struct huff_tree create_huff_tree(struct huff_tree *tree_array, uint64_t * histogram,
				  uint32_t size)
{
	int i;
	uint32_t node_index;
	struct huff_tree tree;
	struct histheap heap;

	heap.size = 0;

	tree.right = tree.left = NULL;

	/* Intitializes heap for construction of the huffman tree */
	for (i = 0; i < size; i++) {
		tree.value = i;
		tree.frequency = histogram[i];
		tree_array[i] = tree;
		heap_push(tree, &heap);
	}

	node_index = size;

	/* Construct the huffman tree */
	while (heap.size > 1) {

		tree = heap_pop(&heap);
		tree_array[node_index].frequency = tree.frequency;
		tree_array[node_index].left = &tree_array[tree.value];

		tree = heap_pop(&heap);
		tree_array[node_index].frequency += tree.frequency;
		tree_array[node_index].right = &tree_array[tree.value];

		tree_array[node_index].value = node_index;
		heap_push(tree_array[node_index], &heap);

		node_index += 1;
	}

	return heap_pop(&heap);
}

int create_huff_lookup(struct huff_code *huff_lookup_table, int table_length,
		       struct huff_tree root, uint8_t max_depth)
{
	/* Used to create a count of number of elements with a given code length */
	uint16_t count[MAX_HUFF_TREE_DEPTH + 1];

	memset(count, 0, sizeof(count));

	if (find_code_lengths(huff_lookup_table, count, root, max_depth) != 0)
		return 1;

	set_huff_codes(huff_lookup_table, table_length, count);

	return 0;
}

int find_code_lengths(struct huff_code *huff_lookup_table, uint16_t * count,
		      struct huff_tree root, uint8_t max_depth)
{
	struct linked_list depth_array[MAX_HUFF_TREE_DEPTH + 2];
	struct linked_list_node linked_lists[MAX_HISTHEAP_SIZE];
	struct linked_list_node *temp;
	uint16_t extra_nodes = 0;
	int i, j;

	memset(depth_array, 0, sizeof(depth_array));
	memset(linked_lists, 0, sizeof(linked_lists));
	for (i = 0; i < MAX_HISTHEAP_SIZE; i++)
		linked_lists[i].value = i;

	huffman_tree_traversal(depth_array, linked_lists, &extra_nodes, max_depth, root, 0);

	/* This for loop fixes up the huffman tree to have a maximum depth not exceeding
	 * max_depth. This algorithm works by removing all elements below max_depth,
	 * filling up the empty leafs which are created with elements form the huffman
	 * tree and then iteratively pushing down the least frequent leaf that is above
	 * max_depth to a depth 1 lower, and moving up a leaf below max_depth to that
	 * same depth.*/
	for (i = MAX_HUFF_TREE_DEPTH + 1; i > max_depth; i--) {

		/* find element to push up the tree */
		while (depth_array[i].start != NULL) {
			if (extra_nodes > 0) {
				temp = pop_from_front(&depth_array[i]);
				append_to_back(&depth_array[max_depth], temp);
				extra_nodes -= 1;

			} else {
				assert(depth_array[max_depth].length % 2 == 0);
				assert(extra_nodes == 0);

				/* find element to push down in the tree */
				for (j = max_depth - 1; j >= 0; j--)
					if (depth_array[j].start != NULL)
						break;

				/* No element available to push down further. */
				if (j < 0)
					return 1;

				temp = pop_from_front(&depth_array[i]);
				append_to_front(&depth_array[j + 1], temp);

				temp = pop_from_front(&depth_array[j]);
				append_to_back(&depth_array[j + 1], temp);
			}
		}
	}

	for (i = 0; i < MAX_HUFF_TREE_DEPTH + 2; i++) {
		temp = depth_array[i].start;

		while (temp != NULL) {
			huff_lookup_table[temp->value].length = i;
			count[i] += 1;
			temp = temp->next;
		}
	}
	return 0;

}

void huffman_tree_traversal(struct linked_list *depth_array,
			    struct linked_list_node *linked_lists, uint16_t * extra_nodes,
			    uint8_t max_depth, struct huff_tree current_node,
			    uint16_t current_depth)
{
	/* This algorithm performs a traversal of the huffman tree. It is setup
	 * to visit the leaves in order of frequency and bin elements into a
	 * linked list by depth.*/
	if (current_node.left == NULL) {
		if (current_depth < MAX_HUFF_TREE_DEPTH + 1)
			append_to_front(&depth_array[current_depth],
					&linked_lists[current_node.value]);
		else
			append_to_front(&depth_array[MAX_HUFF_TREE_DEPTH + 1],
					&linked_lists[current_node.value]);
		return;

	} else if (current_depth == max_depth)
		*extra_nodes += 1;

	if (current_node.left->frequency < current_node.right->frequency) {
		huffman_tree_traversal(depth_array, linked_lists, extra_nodes, max_depth,
				       *current_node.right, current_depth + 1);
		huffman_tree_traversal(depth_array, linked_lists, extra_nodes, max_depth,
				       *current_node.left, current_depth + 1);

	} else {
		huffman_tree_traversal(depth_array, linked_lists, extra_nodes, max_depth,
				       *current_node.left, current_depth + 1);
		huffman_tree_traversal(depth_array, linked_lists, extra_nodes, max_depth,
				       *current_node.right, current_depth + 1);
	}

}

/*
 * Returns integer with first length bits reversed and all higher bits zeroed
 */
uint16_t bit_reverse(uint16_t bits, uint8_t length)
{
	bits = ((bits >> 1) & 0x55555555) | ((bits & 0x55555555) << 1);	// swap bits
	bits = ((bits >> 2) & 0x33333333) | ((bits & 0x33333333) << 2);	// swap pairs
	bits = ((bits >> 4) & 0x0F0F0F0F) | ((bits & 0x0F0F0F0F) << 4);	// swap nibbles
	bits = ((bits >> 8) & 0x00FF00FF) | ((bits & 0x00FF00FF) << 8);	// swap bytes
	return bits >> (16 - length);
}

void set_huff_codes(struct huff_code *huff_code_table, int table_length, uint16_t * count)
{
	/* Uses the algorithm mentioned in the deflate standard, Rfc 1951. */
	int i;
	uint16_t code = 0;
	uint16_t next_code[MAX_HUFF_TREE_DEPTH + 1];

	next_code[0] = code;

	for (i = 1; i < MAX_HUFF_TREE_DEPTH + 1; i++)
		next_code[i] = (next_code[i - 1] + count[i - 1]) << 1;

	for (i = 0; i < table_length; i++) {
		if (huff_code_table[i].length != 0) {
			huff_code_table[i].code =
			    bit_reverse(next_code[huff_code_table[i].length],
					huff_code_table[i].length);
			next_code[huff_code_table[i].length] += 1;
		}
	}

	return;
}

int create_header(uint8_t * header, uint32_t header_length, struct huff_code *lit_huff_table,
		  struct huff_code *dist_huff_table, uint32_t end_of_block)
{
	int i;
	uint64_t histogram[HUFF_LEN];
	uint16_t huffman_rep[LIT_LEN + DIST_LEN];
	uint16_t extra_bits[LIT_LEN + DIST_LEN];
	uint16_t length;
	struct huff_tree root;
	struct huff_tree tree_array[2 * HUFF_LEN - 1];
	struct huff_code lookup_table[HUFF_LEN];
	struct huff_code combined_table[LIT_LEN + DIST_LEN];

	/* hlit, hdist, and hclen are defined in RFC 1951 page 13 */
	uint32_t hlit, hdist, hclen;
	uint64_t bit_count;

	memset(lookup_table, 0, sizeof(lookup_table));

	/* Calculate hlit */
	for (i = LIT_LEN - 1; i > 256; i--)
		if (lit_huff_table[i].length != 0)
			break;

	hlit = i - 256;

	/* Calculate hdist */
	for (i = DIST_LEN - 1; i > 0; i--)
		if (dist_huff_table[i].length != 0)
			break;

	hdist = i;

	/* Combine huffman tables for run length encoding */
	for (i = 0; i < 257 + hlit; i++)
		combined_table[i] = lit_huff_table[i];
	for (i = 0; i < 1 + hdist; i++)
		combined_table[i + hlit + 257] = dist_huff_table[i];

	memset(extra_bits, 0, LIT_LEN + DIST_LEN);
	memset(histogram, 0, sizeof(histogram));

	/* Create a run length encoded representation of the literal/lenght and
	 * distance huffman trees. */
	length = create_huffman_rep(huffman_rep, histogram, extra_bits,
				    combined_table, hlit + 257 + hdist + 1);

	/* Create a huffman tree to encode run length encoded representation. */
	root = create_symbol_subset_huff_tree(tree_array, histogram, HUFF_LEN);
	create_huff_lookup(lookup_table, HUFF_LEN, root, 7);

	/* Calculate hclen */
	for (i = CODE_LEN_CODES - 1; i > 3; i--)	/* i must be at least 4 */
		if (lookup_table[code_length_code_order[i]].length != 0)
			break;

	hclen = i - 3;

	/* Generate actual header. */
	bit_count = create_huffman_header(header, header_length, lookup_table, huffman_rep,
					  extra_bits, length, end_of_block, hclen, hlit,
					  hdist);

	return bit_count;
}

uint16_t create_huffman_rep(uint16_t * huffman_rep, uint64_t * histogram,
			    uint16_t * extra_bits, struct huff_code * huff_table, uint16_t len)
{
	uint16_t current_in_index = 0, current_out_index = 0, run_length, last_code;

	while (current_in_index < len) {
		last_code = huff_table[current_in_index].length;
		run_length = 0;

		while (current_in_index < len
		       && last_code == huff_table[current_in_index].length) {
			run_length += 1;
			current_in_index += 1;
		}

		current_out_index = flush_repeats(huffman_rep, histogram, extra_bits,
						  last_code, run_length, current_out_index);
	}
	return current_out_index;
}

uint16_t flush_repeats(uint16_t * huffman_rep, uint64_t * histogram, uint16_t * extra_bits,
		       uint16_t last_code, uint16_t run_length, uint16_t current_index)
{
	int j;

	if (last_code != 0 && last_code < HUFF_LEN && run_length > 0) {
		huffman_rep[current_index++] = last_code;
		histogram[last_code] += 1;
		run_length -= 1;

	}

	if (run_length < SHORTEST_MATCH) {
		for (j = 0; j < run_length; j++) {
			huffman_rep[current_index++] = last_code;
			histogram[last_code] += 1;
		}
	} else {
		if (last_code == 0) {
			/* The values 138 is the maximum repeat length
			 * represented with code 18. The value 10 is the maximum
			 * repeate length represented with 17. */
			for (; run_length > 138; run_length -= 138) {
				huffman_rep[current_index] = 0x12;
				extra_bits[current_index++] = 0x7F7;
				histogram[18]++;
			}

			if (run_length > 10) {
				huffman_rep[current_index] = 18;
				extra_bits[current_index++] = ((run_length - 11) << 4) | 7;
				histogram[18] += 1;

			} else if (run_length >= SHORTEST_MATCH) {
				huffman_rep[current_index] = 17;
				extra_bits[current_index++] = ((run_length - 3) << 4) | 3;
				histogram[17] += 1;

			} else {
				for (j = 0; j < run_length; j++) {
					huffman_rep[current_index++] = last_code;
					histogram[last_code] += 1;
				}
			}

		} else {
			for (; run_length > 6; run_length -= 6) {
				huffman_rep[current_index] = 0x10;
				extra_bits[current_index++] = 0x32;
				histogram[16]++;
			}

			if (run_length >= SHORTEST_MATCH) {
				huffman_rep[current_index] = 16;
				extra_bits[current_index++] = ((run_length - 3) << 4) | 2;
				histogram[16] += 1;

			} else {
				for (j = 0; j < run_length; j++) {
					huffman_rep[current_index++] = last_code;
					histogram[last_code] += 1;
				}
			}
		}

	}

	return current_index;
}

int create_huffman_header(uint8_t * header, uint32_t header_length,
			  struct huff_code *lookup_table, uint16_t * huffman_rep,
			  uint16_t * extra_bits, uint16_t huffman_rep_length,
			  uint32_t end_of_block, uint32_t hclen, uint32_t hlit, uint32_t hdist)
{
	/* hlit, hdist, hclen are as defined in the deflate standard, head is the
	 * first three deflate header bits.*/
	int i;
	uint32_t head;
	uint64_t bit_count;
	struct huff_code huffman_value;
	struct BitBuf2 header_bitbuf;

	if (end_of_block)
		head = 0x05;
	else
		head = 0x04;

	set_buf(&header_bitbuf, header, header_length);
	init(&header_bitbuf);

	write_bits(&header_bitbuf, (head | (hlit << 3) | (hdist << 8) | (hclen << 13)),
		   DYN_HDR_START_LEN);

	uint64_t tmp = 0;
	for (i = hclen + 3; i >= 0; i--) {
		tmp = (tmp << 3) | lookup_table[code_length_code_order[i]].length;
	}

	write_bits(&header_bitbuf, tmp, (hclen + 4) * 3);

	for (i = 0; i < huffman_rep_length; i++) {
		huffman_value = lookup_table[huffman_rep[i]];

		write_bits(&header_bitbuf, (uint64_t) huffman_value.code,
			   (uint32_t) huffman_value.length);

		if (huffman_rep[i] > 15) {
			write_bits(&header_bitbuf, (uint64_t) extra_bits[i] >> 4,
				   (uint32_t) extra_bits[i] & 0xF);
		}
	}
	bit_count = 8 * buffer_used(&header_bitbuf) + header_bitbuf.m_bit_count;
	flush(&header_bitbuf);

	return bit_count;
}

void create_code_tables(uint16_t * code_table, uint8_t * code_length_table, uint32_t length,
			struct huff_code *hufftable)
{
	int i;
	for (i = 0; i < length; i++) {
		code_table[i] = hufftable[i].code;
		code_length_table[i] = hufftable[i].length;
	}
}

void create_packed_len_table(uint32_t * packed_table, struct huff_code *lit_len_hufftable)
{
	int i, count = 0;
	uint16_t extra_bits;
	uint16_t extra_bits_count = 0;

	/* Gain extra bits is the next place where the number of extra bits in
	 * lenght codes increases. */
	uint16_t gain_extra_bits = LEN_EXTRA_BITS_START;

	for (i = 257; i < LIT_LEN - 1; i++) {
		for (extra_bits = 0; extra_bits < (1 << extra_bits_count); extra_bits++) {
			if (count > 254)
				break;
			packed_table[count++] =
			    (extra_bits << (lit_len_hufftable[i].length + LENGTH_BITS)) |
			    (lit_len_hufftable[i].code << LENGTH_BITS) |
			    (lit_len_hufftable[i].length + extra_bits_count);
		}

		if (i == gain_extra_bits) {
			gain_extra_bits += LEN_EXTRA_BITS_INTERVAL;
			extra_bits_count += 1;
		}
	}

	packed_table[count] = (lit_len_hufftable[LIT_LEN - 1].code << LENGTH_BITS) |
	    (lit_len_hufftable[LIT_LEN - 1].length);
}

void create_packed_dist_table(uint32_t * packed_table, uint32_t length,
			      struct huff_code *dist_hufftable)
{
	int i, count = 0;
	uint16_t extra_bits;
	uint16_t extra_bits_count = 0;

	/* Gain extra bits is the next place where the number of extra bits in
	 * distance codes increases. */
	uint16_t gain_extra_bits = DIST_EXTRA_BITS_START;

	for (i = 0; i < DIST_LEN; i++) {
		for (extra_bits = 0; extra_bits < (1 << extra_bits_count); extra_bits++) {
			if (count >= length)
				return;

			packed_table[count++] =
			    (extra_bits << (dist_hufftable[i].length + LENGTH_BITS)) |
			    (dist_hufftable[i].code << LENGTH_BITS) |
			    (dist_hufftable[i].length + extra_bits_count);

		}

		if (i == gain_extra_bits) {
			gain_extra_bits += DIST_EXTRA_BITS_INTERVAL;
			extra_bits_count += 1;
		}
	}
}

int are_hufftables_useable(struct huff_code *lit_len_hufftable,
			   struct huff_code *dist_hufftable)
{
	int max_lit_code_len = 0, max_len_code_len = 0, max_dist_code_len = 0;
	int dist_extra_bits = 0, len_extra_bits = 0;
	int gain_dist_extra_bits = DIST_EXTRA_BITS_START;
	int gain_len_extra_bits = LEN_EXTRA_BITS_START;
	int max_code_len;
	int i;

	for (i = 0; i < LIT_LEN; i++)
		if (lit_len_hufftable[i].length > max_lit_code_len)
			max_lit_code_len = lit_len_hufftable[i].length;

	for (i = 257; i < LIT_LEN - 1; i++) {
		if (lit_len_hufftable[i].length + len_extra_bits > max_len_code_len)
			max_len_code_len = lit_len_hufftable[i].length + len_extra_bits;

		if (i == gain_len_extra_bits) {
			gain_len_extra_bits += LEN_EXTRA_BITS_INTERVAL;
			len_extra_bits += 1;
		}
	}

	for (i = 0; i < DIST_LEN; i++) {
		if (dist_hufftable[i].length + dist_extra_bits > max_dist_code_len)
			max_dist_code_len = dist_hufftable[i].length + dist_extra_bits;

		if (i == gain_dist_extra_bits) {
			gain_dist_extra_bits += DIST_EXTRA_BITS_INTERVAL;
			dist_extra_bits += 1;
		}
	}

	max_code_len = max_lit_code_len + max_len_code_len + max_dist_code_len;

	/* Some versions of igzip can write upto one literal, one length and one
	 * distance code at the same time. This checks to make sure that is
	 * always writeable in bitbuf*/
	return (max_code_len > MAX_BITBUF_BIT_WRITE);
}

int isal_create_hufftables(struct isal_hufftables *hufftables,
			   struct isal_huff_histogram *histogram)
{
	struct huff_tree lit_tree, dist_tree;
	struct huff_tree lit_tree_array[2 * LIT_LEN - 1], dist_tree_array[2 * DIST_LEN - 1];
	struct huff_code lit_huff_table[LIT_LEN], dist_huff_table[DIST_LEN];
	uint64_t bit_count;
	int max_dist = convert_dist_to_dist_sym(IGZIP_D);

	uint32_t *dist_table = hufftables->dist_table;
	uint32_t *len_table = hufftables->len_table;
	uint16_t *lit_table = hufftables->lit_table;
	uint16_t *dcodes = hufftables->dcodes;
	uint8_t *lit_table_sizes = hufftables->lit_table_sizes;
	uint8_t *dcodes_sizes = hufftables->dcodes_sizes;
	uint8_t *deflate_hdr = hufftables->deflate_hdr;
	uint64_t *lit_len_histogram = histogram->lit_len_histogram;
	uint64_t *dist_histogram = histogram->dist_histogram;

	memset(hufftables, 0, sizeof(struct isal_hufftables));
	memset(lit_tree_array, 0, sizeof(lit_tree_array));
	memset(dist_tree_array, 0, sizeof(dist_tree_array));
	memset(lit_huff_table, 0, sizeof(lit_huff_table));
	memset(dist_huff_table, 0, sizeof(dist_huff_table));

	lit_tree = create_huff_tree(lit_tree_array, lit_len_histogram, LIT_LEN);
	dist_tree = create_huff_tree(dist_tree_array, dist_histogram, max_dist + 1);

	if (create_huff_lookup(lit_huff_table, LIT_LEN, lit_tree, MAX_DEFLATE_CODE_LEN) > 0)
		return INVALID_LIT_LEN_HUFFCODE;

	if (create_huff_lookup(dist_huff_table, DIST_LEN, dist_tree, MAX_DEFLATE_CODE_LEN) > 0)
		return INVALID_DIST_HUFFCODE;

	if (are_hufftables_useable(lit_huff_table, dist_huff_table)) {
		if (create_huff_lookup
		    (lit_huff_table, LIT_LEN, lit_tree, MAX_SAFE_LIT_CODE_LEN) > 0)
			return INVALID_LIT_LEN_HUFFCODE;

		if (create_huff_lookup
		    (dist_huff_table, DIST_LEN, dist_tree, MAX_SAFE_DIST_CODE_LEN) > 0)
			return INVALID_DIST_HUFFCODE;

		if (are_hufftables_useable(lit_huff_table, dist_huff_table))
			return INVALID_HUFFCODE;
	}

	create_code_tables(dcodes, dcodes_sizes, DIST_LEN - DCODE_OFFSET,
			   dist_huff_table + DCODE_OFFSET);

	create_code_tables(lit_table, lit_table_sizes, LIT_TABLE_SIZE, lit_huff_table);

	create_packed_len_table(len_table, lit_huff_table);
	create_packed_dist_table(dist_table, DIST_TABLE_SIZE, dist_huff_table);

	bit_count =
	    create_header(deflate_hdr, sizeof(deflate_hdr), lit_huff_table, dist_huff_table,
			  LAST_BLOCK);

	hufftables->deflate_hdr_count = bit_count / 8;
	hufftables->deflate_hdr_extra_bits = bit_count % 8;

	return 0;
}

int isal_create_hufftables_subset(struct isal_hufftables *hufftables,
				  struct isal_huff_histogram *histogram)
{
	struct huff_tree lit_tree, dist_tree;
	struct huff_tree lit_tree_array[2 * LIT_LEN - 1], dist_tree_array[2 * DIST_LEN - 1];
	struct huff_code lit_huff_table[LIT_LEN], dist_huff_table[DIST_LEN];
	uint64_t bit_count;
	int j, max_dist = convert_dist_to_dist_sym(IGZIP_D);

	uint32_t *dist_table = hufftables->dist_table;
	uint32_t *len_table = hufftables->len_table;
	uint16_t *lit_table = hufftables->lit_table;
	uint16_t *dcodes = hufftables->dcodes;
	uint8_t *lit_table_sizes = hufftables->lit_table_sizes;
	uint8_t *dcodes_sizes = hufftables->dcodes_sizes;
	uint8_t *deflate_hdr = hufftables->deflate_hdr;
	uint64_t *lit_len_histogram = histogram->lit_len_histogram;
	uint64_t *dist_histogram = histogram->dist_histogram;

	memset(hufftables, 0, sizeof(struct isal_hufftables));
	memset(lit_tree_array, 0, sizeof(lit_tree_array));
	memset(dist_tree_array, 0, sizeof(dist_tree_array));
	memset(lit_huff_table, 0, sizeof(lit_huff_table));
	memset(dist_huff_table, 0, sizeof(dist_huff_table));

	for (j = LIT_TABLE_SIZE; j < LIT_LEN; j++)
		if (lit_len_histogram[j] == 0)
			lit_len_histogram[j]++;

	lit_tree = create_symbol_subset_huff_tree(lit_tree_array, lit_len_histogram, LIT_LEN);
	dist_tree = create_huff_tree(dist_tree_array, dist_histogram, max_dist + 1);

	if (create_huff_lookup(lit_huff_table, LIT_LEN, lit_tree, MAX_DEFLATE_CODE_LEN) > 0)
		return INVALID_LIT_LEN_HUFFCODE;

	if (create_huff_lookup(dist_huff_table, DIST_LEN, dist_tree, MAX_DEFLATE_CODE_LEN) > 0)
		return INVALID_DIST_HUFFCODE;

	if (are_hufftables_useable(lit_huff_table, dist_huff_table)) {
		if (create_huff_lookup
		    (lit_huff_table, LIT_LEN, lit_tree, MAX_SAFE_LIT_CODE_LEN) > 0)
			return INVALID_LIT_LEN_HUFFCODE;

		if (create_huff_lookup
		    (dist_huff_table, DIST_LEN, dist_tree, MAX_SAFE_DIST_CODE_LEN) > 0)
			return INVALID_DIST_HUFFCODE;

		if (are_hufftables_useable(lit_huff_table, dist_huff_table))
			return INVALID_HUFFCODE;
	}

	create_code_tables(dcodes, dcodes_sizes, DIST_LEN - DCODE_OFFSET,
			   dist_huff_table + DCODE_OFFSET);

	create_code_tables(lit_table, lit_table_sizes, LIT_TABLE_SIZE, lit_huff_table);

	create_packed_len_table(len_table, lit_huff_table);
	create_packed_dist_table(dist_table, DIST_TABLE_SIZE, dist_huff_table);

	bit_count =
	    create_header(deflate_hdr, sizeof(deflate_hdr), lit_huff_table, dist_huff_table,
			  LAST_BLOCK);

	hufftables->deflate_hdr_count = bit_count / 8;
	hufftables->deflate_hdr_extra_bits = bit_count % 8;

	return 0;
}
