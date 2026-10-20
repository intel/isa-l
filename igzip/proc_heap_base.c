/**********************************************************************
  Copyright(c) 2011-2017 Intel Corporation All rights reserved.

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

#include "igzip_lib.h"
#include "huff_codes.h"
#include <string.h>

/*
 * build_huff_tree() builds a Huffman tree from the keys in heap[1..heap_size]. Each key holds
 * a frequency at FREQ_SHIFT and up, and a node id in the bits below. Each step removes the two
 * smallest keys h1 and h2, stores their ids as the children of a new node with id node_ptr, and
 * inserts the key of the new node, ((h1 + h2) & ~0xFFFF) | node_ptr.
 *
 * It gives the same tree as a binary heap of the keys, without the heap. It sorts the leaf
 * keys, and it keeps the keys of new nodes in the order they are made, which is ascending order
 * of frequency. New nodes get decreasing ids, so among new nodes with the same frequency, the
 * newest one has the smallest key. The smallest remaining key is then either the smallest leaf,
 * or the newest new node with the lowest frequency.
 */

#define SORT_MAX_DIGIT_BITS 8
#define SORT_SMALL          32

/* Sorts n keys in ascending order, with room for n keys at tmp */
static void
sort_keys(uint64_t *keys, uint64_t *tmp, uint32_t n)
{
        uint16_t count[1 << SORT_MAX_DIGIT_BITS];
        uint64_t max_freq = 0, key, *src = keys, *dst = tmp, *swap;
        uint32_t i, j, pos, sorted_ids = 1, freq_bits, passes, bits, shift, mask;

        for (i = 0; i < n; i++) {
                max_freq |= keys[i] >> FREQ_SHIFT;
                if (i > 0 && (keys[i] & 0xFFFF) <= (keys[i - 1] & 0xFFFF))
                        sorted_ids = 0;
        }

        if (n <= SORT_SMALL || !sorted_ids) {
                /*
                 * Small sorts, and the two keys make_code_lens() pads to, which can be out of
                 * order. The keys are distinct, so the position of each is the number of
                 * smaller keys, which takes no branches to count.
                 */
                for (i = 0; i < n; i++) {
                        key = keys[i];
                        for (j = 0, pos = 0; j < n; j++)
                                pos += keys[j] < key;
                        tmp[pos] = key;
                }
                memcpy(keys, tmp, n * sizeof(*keys));
                return;
        }

        /*
         * The keys are in ascending order of id, so a stable sort on the frequency sorts them.
         * Use an LSD radix sort with the fewest digits of at most SORT_MAX_DIGIT_BITS bits,
         * spreading the frequency bits evenly over the digits.
         */
        for (freq_bits = 0; (max_freq >> freq_bits) != 0; freq_bits++)
                ;
        passes = (freq_bits + SORT_MAX_DIGIT_BITS - 1) / SORT_MAX_DIGIT_BITS;
        for (shift = FREQ_SHIFT; passes > 0; passes--, freq_bits -= bits) {
                bits = (freq_bits + passes - 1) / passes;
                mask = (1 << bits) - 1;
                memset(count, 0, sizeof(count[0]) << bits);
                for (i = 0; i < n; i++)
                        count[(src[i] >> shift) & mask]++;
                for (i = 0, pos = 0; i <= mask; i++) {
                        j = count[i];
                        count[i] = pos;
                        pos += j;
                }
                for (i = 0; i < n; i++)
                        dst[count[(src[i] >> shift) & mask]++] = src[i];
                swap = src;
                src = dst;
                dst = swap;
                shift += bits;
        }

        if (src != keys)
                memcpy(keys, src, n * sizeof(*keys));
}

/*
 * Queue of new node keys. node[lo..top) holds the nodes with the lowest frequency that have not
 * been removed, oldest first. node[next..end) holds the nodes with higher frequencies, oldest
 * first. Removed nodes leave unused slots in node[top..next).
 */
struct node_queue {
        uint64_t node[MAX_HISTHEAP_SIZE];
        uint32_t lo, top, next, end;
};

static inline uint64_t
min_key(uint64_t **leaf, uint64_t *leaf_end, struct node_queue *q)
{
        uint64_t key;
        uint32_t i;

        if (q->top == q->lo || (*leaf != leaf_end && **leaf < q->node[q->top - 1]))
                return *(*leaf)++;

        key = q->node[--q->top];
        if (q->top == q->lo && q->next != q->end) {
                /* Move on to the nodes with the next frequency */
                q->lo = q->next;
                for (i = q->lo + 1; i < q->end; i++)
                        if ((q->node[i] >> FREQ_SHIFT) != (q->node[q->lo] >> FREQ_SHIFT))
                                break;
                q->top = i;
                q->next = i;
        }
        return key;
}

static inline void
add_node(struct node_queue *q, uint64_t key)
{
        /* key has the highest frequency so far */
        if (q->next == q->end) {
                if (q->top == q->lo) {
                        q->node[q->end] = key;
                        q->lo = q->end;
                        q->top = q->next = ++q->end;
                        return;
                }
                if ((q->node[q->top - 1] >> FREQ_SHIFT) == (key >> FREQ_SHIFT)) {
                        q->node[q->top++] = key;
                        if (q->top > q->end)
                                q->next = q->end = q->top;
                        return;
                }
        }
        q->node[q->end++] = key;
}

uint32_t
build_huff_tree(struct heap_tree *heap_space, uint64_t heap_size, uint64_t node_ptr)
{
        uint64_t *heap = (uint64_t *) heap_space;
        uint64_t *leaf = &heap[1], *leaf_end = &heap[1 + heap_size];
        struct node_queue q;
        uint64_t h1, h2;

        /*
         * The tree takes the 2 * heap_size - 1 entries that end at node_ptr. Use them to sort
         * the leaves before building the tree. Each tree entry is then written in full, as a
         * child id with a depth of 0.
         */
        sort_keys(leaf, &heap[node_ptr + 2 - 2 * heap_size], heap_size);
        q.lo = q.top = q.next = q.end = 0;

        while (heap_size > 1) {
                h1 = min_key(&leaf, leaf_end, &q);
                h2 = min_key(&leaf, leaf_end, &q);
                add_node(&q, ((h1 + h2) & ~0xFFFFull) | node_ptr);
                heap[node_ptr] = h1 & 0xFFFF;
                heap[node_ptr - 1] = h2 & 0xFFFF;
                node_ptr -= 2;
                heap_size--;
        }
        h1 = min_key(&leaf, leaf_end, &q);
        heap[node_ptr] = h1 & 0xFFFF;
        return node_ptr;
}
