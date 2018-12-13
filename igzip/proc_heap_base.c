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
#include "unaligned.h"

static inline void heapify(uint64_t * heap, uint64_t heap_size, uint64_t index)
{
	uint64_t child = 2 * index, tmp;
	while (child <= heap_size) {
		child = (heap[child] <= heap[child + 1]) ? child : child + 1;

		if (heap[index] > heap[child]) {
			tmp = heap[index];
			heap[index] = heap[child];
			heap[child] = tmp;
			index = child;
			child = 2 * index;
		} else
			break;
	}
}

void build_heap(uint64_t * heap, uint64_t heap_size)
{
	uint64_t i;
	heap[heap_size + 1] = -1;
	for (i = heap_size / 2; i > 0; i--)
		heapify(heap, heap_size, i);

}

uint32_t build_huff_tree(struct heap_tree *heap_space, uint64_t heap_size, uint64_t node_ptr)
{
	uint64_t *heap = (uint64_t *) heap_space;
	uint64_t h1, h2;

	while (heap_size > 1) {
		h1 = heap[1];
		heap[1] = heap[heap_size];
		heap[heap_size--] = -1;

		heapify(heap, heap_size, 1);

		h2 = heap[1];
		heap[1] = ((h1 + h2) & ~0xFFFFull) | node_ptr;

		heapify(heap, heap_size, 1);

		store_u16((uint8_t *) & heap[node_ptr], h1);
		store_u16((uint8_t *) & heap[node_ptr - 1], h2);
		node_ptr -= 2;

	}
	h1 = heap[1];
	store_u16((uint8_t *) & heap[node_ptr], h1);
	return node_ptr;
}
