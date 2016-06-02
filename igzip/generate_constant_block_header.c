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

#include <stdint.h>
#include <stdio.h>
#include "huff_codes.h"
#include "bitbuf2.h"

#define MAX_HEADER_SIZE 350
#define BLOCK_SIZE 16*1024

void fprint_header(FILE * outfile, uint8_t * header, uint64_t bit_count)
{
	int i;
	fprintf(outfile, "unsigned char data[] = {");
	for (i = 0; i < bit_count / 8; i++) {
		if ((i & 7) == 0)
			fprintf(outfile, "\n\t");
		else
			fprintf(outfile, " ");
		fprintf(outfile, "0x%02x,", header[i]);
	}

	if ((i & 7) == 0)
		fprintf(outfile, "\n\t");
	else
		fprintf(outfile, " ");
	fprintf(outfile, "0x%02x", header[i]);
	fprintf(outfile, "\t};\n\n");

}

int main(int argc, char **argv)
{
	/* Generates a header for a constant block, along with some manual
	 * twiddling to create a header with the desired properties*/
	uint8_t stream[BLOCK_SIZE];
	struct isal_huff_histogram histogram;
	uint64_t *lit_histogram = histogram.lit_len_histogram;
	uint64_t *dist_histogram = histogram.dist_histogram;
	uint8_t header[MAX_HEADER_SIZE];
	struct huff_tree lit_tree, dist_tree;
	struct huff_tree lit_tree_array[2 * LIT_LEN - 1], dist_tree_array[2 * DIST_LEN - 1];
	struct huff_code lit_huff_table[LIT_LEN], dist_huff_table[DIST_LEN];
	uint64_t bit_count;

	uint8_t repeated_char = 0x00;

	memset(header, 0, sizeof(header));
	memset(&histogram, 0, sizeof(histogram));	/* Initialize histograms. */
	memset(stream, repeated_char, sizeof(stream));
	memset(lit_tree_array, 0, sizeof(lit_tree_array));
	memset(dist_tree_array, 0, sizeof(dist_tree_array));
	memset(lit_huff_table, 0, sizeof(lit_huff_table));
	memset(dist_huff_table, 0, sizeof(dist_huff_table));

	isal_update_histogram(stream, sizeof(stream), &histogram);

	/* These are set to manually change the histogram to create a header with the
	 * desired properties. In this case, the header is modified so that it is byte
	 * unaligned by 6 bits, so that 0 is a 2 bit code, so that the header plus the
	 * encoding of one 0 is byte aligned*/
	lit_histogram[repeated_char] = 20;
	lit_histogram[280] = 2;
	lit_histogram[264] = 5;
	lit_histogram[282] = 0;

	lit_tree = create_symbol_subset_huff_tree(lit_tree_array, lit_histogram, LIT_LEN);
	dist_tree = create_symbol_subset_huff_tree(dist_tree_array, dist_histogram, DIST_LEN);
	if (create_huff_lookup(lit_huff_table, LIT_LEN, lit_tree, 15) > 0) {
		printf("Error, code with invalid length for Deflate standard.\n");
		return 1;
	}

	if (create_huff_lookup(dist_huff_table, DIST_LEN, dist_tree, 15) > 0) {
		printf("Error, code with invalid length for Deflate standard.\n");
		return 1;
	}

	/* Remove litral symbol corresponding to the unoptimal look back
	 * distance of 258 found by gen_histogram*/
	dist_huff_table[16].length = 0;

	bit_count = create_header(header, sizeof(header), lit_huff_table, dist_huff_table, 1);
	printf("Header for %x\n", repeated_char);
	fprintf(stdout, "Complete Bytes: %lu\n", bit_count / 8);
	fprintf(stdout, "Byte Offset:    %lu\n\n", (bit_count) & 7);
	fprint_header(stdout, header, bit_count);
	printf("\n");

	return 0;
}
