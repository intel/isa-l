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

/* This program can be used to generate custom a custom huffman encoding to get
 * better data compression. This is most useful when the type of data being
 * compressed is well known.
 *
 * To use generate_custom_hufftables, pass a sequence of files to the program
 * that together form an accurate representation of the data that is being
 * compressed. Generate_custom_hufftables will then produce the file
 * hufftables_c.c, which should be moved to replace its counterpart in the igzip
 * source folder. After recompiling the Isa-l library, the igzip compression
 * functions will use the new hufftables.
 *
 * Generate_custom_hufftables should be compiled with the same compile time
 * parameters as the igzip source code. Generating custom hufftables with
 * different compile time parameters may cause igzip to produce invalid output
 * for the reasons described below. The default parameters used by
 * generate_custom_hufftables are the same as the default parameters used by
 * igzip.
 *
 * *WARNING* generate custom hufftables must be compiled with a HIST_SIZE that
 * is at least as large as the HIST_SIZE used by igzip. By default HIST_SIZE is
 * 8, the maximum usable HIST_SIZE is 32. The reason for this is to generate
 * better compression. Igzip cannot produce look back distances with sizes
 * larger than the HIST_SIZE * 1024 igzip was compiled with, so look back
 * distances with sizes larger than HIST_SIZE * 1024 are not assigned a huffman
 * code.
 *
 * To improve compression ratio, the compile time option LIT_SUB is provided to
 * allow generating custom hufftables which only use a subset of all possible
 * literals. This can be useful for getting better compression when it is known
 * that the data being compressed will never contain certain symbols, for
 * example text files. If this option is used, it needs to be checked that every
 * possible literal is in fact given a valid code in the output hufftable. This
 * can be done by checking that every required literal has a positive value for
 * the length of the code associated with that literal. Literals which have not
 * been given codes will have a code length of zero. The compile time option
 * PRINT_CODES (described below) can be used to help manually perform this
 * check.
 *
 * The compile time parameter PRINT_CODES causes the literal/length huffman code
 * and the distance huffman code created by generate_custom_hufftables to be
 * printed out. This is printed out where each line corresponds to a different
 * symbol. The first column is the symbol used to represent each literal (Lit),
 * end of block symbol (EOB), length (Len) or distance (Dist), the second column
 * is the associated code value, and the third column is the length in bits of
 * that code.
 */

#include <stdint.h>
#include <stdio.h>
#include <inttypes.h>
#include "huff_codes.h"
#include "bitbuf2.h"

/*These max code lengths are limited by how the data is stored in
 * hufftables.asm. The deflate standard max is 15.*/

#define LONG_DCODE_OFFSET 26
#define SHORT_DCODE_OFFSET 20

#define MAX_HEADER_SIZE IGZIP_MAX_DEF_HDR_SIZE

#define GZIP_HEADER_SIZE 10
#define GZIP_TRAILER_SIZE 8

/**
 * @brief Prints a table of uint8_t elements to a file.
 * @param outfile: the file the table is printed to.
 * @param table: the table to be printed.
 * @param length: number of elements to be printed.
 * @param header: header to append in front of the table.
 * @param footer: footer to append at the end of the table.
 * @param begin_line: string printed at beginning of new line
 */
void fprint_uint8_table(FILE * outfile, uint8_t * table, uint64_t length, char *header,
			char *footer, char *begin_line)
{
	int i;
	fprintf(outfile, "%s", header);
	for (i = 0; i < length - 1; i++) {
		if ((i & 7) == 0)
			fprintf(outfile, "\n%s", begin_line);
		else
			fprintf(outfile, " ");
		fprintf(outfile, "0x%02x,", table[i]);
	}

	if ((i & 7) == 0)
		fprintf(outfile, "\n%s", begin_line);
	else
		fprintf(outfile, " ");
	fprintf(outfile, "0x%02x", table[i]);
	fprintf(outfile, "%s", footer);

}

/**
 * @brief Prints a table of uint16_t elements to a file.
 * @param outfile: the file the table is printed to.
 * @param table: the table to be printed.
 * @param length: number of elements to be printed.
 * @param header: header to append in front of the table.
 * @param footer: footer to append at the end of the table.
 * @param begin_line: string printed at beginning of new line
 */
void fprint_uint16_table(FILE * outfile, uint16_t * table, uint64_t length, char *header,
			 char *footer, char *begin_line)
{
	int i;
	fprintf(outfile, "%s", header);
	for (i = 0; i < length - 1; i++) {
		if ((i & 7) == 0)
			fprintf(outfile, "\n%s", begin_line);
		else
			fprintf(outfile, " ");
		fprintf(outfile, "0x%04x,", table[i]);
	}

	if ((i & 7) == 0)
		fprintf(outfile, "\n%s", begin_line);
	else
		fprintf(outfile, " ");
	fprintf(outfile, "0x%04x", table[i]);
	fprintf(outfile, "%s", footer);

}

/**
 * @brief Prints a table of uint32_t elements to a file.
 * @param outfile: the file the table is printed to.
 * @param table: the table to be printed.
 * @param length: number of elements to be printed.
 * @param header: header to append in front of the table.
 * @param footer: footer to append at the end of the table.
 * @param begin_line: string printed at beginning of new line
 */
void fprint_uint32_table(FILE * outfile, uint32_t * table, uint64_t length, char *header,
			 char *footer, char *begin_line)
{
	int i;
	fprintf(outfile, "%s", header);
	for (i = 0; i < length - 1; i++) {
		if ((i & 3) == 0)
			fprintf(outfile, "\n%s", begin_line);
		else
			fprintf(outfile, " ");
		fprintf(outfile, "0x%08x,", table[i]);
	}

	if ((i & 3) == 0)
		fprintf(outfile, "%s", begin_line);
	else
		fprintf(outfile, " ");
	fprintf(outfile, "0x%08x", table[i]);
	fprintf(outfile, "%s", footer);

}

/**
 * @brief Prints a table of uint64_t elements to a file.
 * @param outfile: the file the table is printed to.
 * @param table: the table to be printed.
 * @param length: number of elements to be printed.
 * @param header: header to append in front of the table.
 * @param footer: footer to append at the end of the table.
 */
void fprint_uint64_table(FILE * outfile, uint64_t * table, uint64_t length, char *header,
			 char *footer)
{
	int i;
	fprintf(outfile, "%s\n", header);
	for (i = 0; i < length - 1; i++)
		fprintf(outfile, "\t0x%016" PRIx64 ",\n", table[i]);
	fprintf(outfile, "\t0x%016" PRIx64, table[i]);
	fprintf(outfile, "%s", footer);

}

void fprint_hufftables(FILE * output_file, uint8_t * header, uint32_t bit_count,
		       uint16_t * lit_code_table, uint8_t * lit_code_size_table,
		       uint16_t * dcodes_code_table, uint8_t * dcodes_code_size_table,
		       uint32_t * packed_len_table, uint32_t * packed_dist_table)
{
	fprintf(output_file, "struct isal_hufftables hufftables_default = {\n\n");

	fprint_uint8_table(output_file, header, (bit_count + 7) / 8,
			   "\t.deflate_hdr = {", "\t},\n\n", "\t\t");
	fprintf(output_file, "\t.deflate_hdr_count = %d,\n", bit_count / 8);
	fprintf(output_file, "\t.deflate_hdr_extra_bits = %d,\n\n", bit_count & 7);

	fprint_uint32_table(output_file, packed_dist_table, SHORT_DIST_TABLE_SIZE,
			    "\t.dist_table = {", ",\n", "\t\t");
	fprint_uint32_table(output_file, &packed_dist_table[SHORT_DIST_TABLE_SIZE],
			    LONG_DIST_TABLE_SIZE - SHORT_DIST_TABLE_SIZE,
			    "#ifdef LONGER_HUFFTABLE",
			    "\n#endif /* LONGER_HUFFTABLE */\n\t},\n\n", "\t\t");

	fprint_uint32_table(output_file, packed_len_table, LEN_TABLE_SIZE, "\t.len_table = {",
			    "\t},\n\n", "\t\t");
	fprint_uint16_table(output_file, lit_code_table, LIT_TABLE_SIZE, "\t.lit_table = {",
			    "\t},\n\n", "\t\t");
	fprint_uint8_table(output_file, lit_code_size_table, LIT_TABLE_SIZE,
			   "\t.lit_table_sizes = {", "\t},\n\n", "\t\t");

	fprintf(output_file, "#ifndef LONGER_HUFFTABLE\n");
	fprint_uint16_table(output_file, dcodes_code_table + SHORT_DCODE_OFFSET,
			    DIST_LEN - SHORT_DCODE_OFFSET, "\t.dcodes = {", "\t},\n\n",
			    "\t\t");
	fprint_uint8_table(output_file, dcodes_code_size_table + SHORT_DCODE_OFFSET,
			   DIST_LEN - SHORT_DCODE_OFFSET, "\t.dcodes_sizes = {", "\t}\n",
			   "\t\t");
	fprintf(output_file, "#else\n");
	fprint_uint16_table(output_file, dcodes_code_table + LONG_DCODE_OFFSET,
			    DIST_LEN - LONG_DCODE_OFFSET, "\t.dcodes = {", "\t},\n\n", "\t\t");
	fprint_uint8_table(output_file, dcodes_code_size_table + LONG_DCODE_OFFSET,
			   DIST_LEN - LONG_DCODE_OFFSET, "\t.dcodes_sizes = {", "\t}\n",
			   "\t\t");
	fprintf(output_file, "#endif\n");
	fprintf(output_file, "};\n");
}

void fprint_header(FILE * output_file, uint8_t * header, uint32_t bit_count,
		   uint16_t * lit_code_table, uint8_t * lit_code_size_table,
		   uint16_t * dcodes_code_table, uint8_t * dcodes_code_size_table,
		   uint32_t * packed_len_table, uint32_t * packed_dist_table)
{
	fprintf(output_file, "#include <stdint.h>\n");
	fprintf(output_file, "#include <igzip_lib.h>\n\n");

	fprintf(output_file, "const uint8_t gzip_hdr[] = {\n"
		"\t0x1f, 0x8b, 0x08, 0x00, 0x00,\n" "\t0x00, 0x00, 0x00, 0x00, 0xff\t};\n\n");

	fprintf(output_file, "const uint32_t gzip_hdr_bytes = %d;\n", GZIP_HEADER_SIZE);
	fprintf(output_file, "const uint32_t gzip_trl_bytes = %d;\n\n", GZIP_TRAILER_SIZE);

	fprint_hufftables(output_file, header, bit_count, lit_code_table, lit_code_size_table,
			  dcodes_code_table, dcodes_code_size_table, packed_len_table,
			  packed_dist_table);
}

int main(int argc, char *argv[])
{
	long int file_length;
	uint8_t *stream = NULL;
	struct isal_huff_histogram histogram;
	uint64_t *lit_histogram = histogram.lit_len_histogram;
	uint64_t *dist_histogram = histogram.dist_histogram;
	uint8_t header[MAX_HEADER_SIZE];
	FILE *file;
	struct huff_tree lit_tree, dist_tree;
	struct huff_tree lit_tree_array[2 * LIT_LEN - 1], dist_tree_array[2 * DIST_LEN - 1];
	struct huff_code lit_huff_table[LIT_LEN], dist_huff_table[DIST_LEN];
	uint64_t bit_count;
	uint32_t packed_len_table[LEN_TABLE_SIZE];
	uint32_t packed_dist_table[LONG_DIST_TABLE_SIZE];
	uint16_t lit_code_table[LIT_TABLE_SIZE];
	uint16_t dcodes_code_table[DIST_LEN];
	uint8_t lit_code_size_table[LIT_TABLE_SIZE];
	uint8_t dcodes_code_size_table[DIST_LEN];
	int max_dist = convert_dist_to_dist_sym(D);

	if (argc == 1) {
		printf("Error, no input file.\n");
		return 1;
	}

	memset(&histogram, 0, sizeof(histogram));	/* Initialize histograms. */
	memset(lit_tree_array, 0, sizeof(lit_tree_array));
	memset(dist_tree_array, 0, sizeof(dist_tree_array));
	memset(lit_huff_table, 0, sizeof(lit_huff_table));
	memset(dist_huff_table, 0, sizeof(dist_huff_table));

	while (argc > 1) {
		printf("Processing %s\n", argv[argc - 1]);
		file = fopen(argv[argc - 1], "r");
		if (file == NULL) {
			printf("Error opening file\n");
			return 1;
		}
		fseek(file, 0, SEEK_END);
		file_length = ftell(file);
		fseek(file, 0, SEEK_SET);
		file_length -= ftell(file);
		stream = malloc(file_length);
		if (stream == NULL) {
			printf("Failed to allocate memory to read in file\n");
			fclose(file);
			return 1;
		}
		fread(stream, 1, file_length, file);
		if (ferror(file)) {
			printf("Error occurred when reading file");
			fclose(file);
			free(stream);
			return 1;
		}

		/* Create a histogram of frequency of symbols found in stream to
		 * generate the huffman tree.*/
		isal_update_histogram(stream, file_length, &histogram);

		fclose(file);
		free(stream);
		argc--;
	}

	/* Create a huffman tree corresponding to the histograms created in
	 * gen_histogram*/
#ifdef LIT_SUB
	int j;
	/* Guarantee every possible repeat length is given a symbol. It is hard
	 * to guarantee data will never have a repeat of a given length */
	for (j = LIT_TABLE_SIZE; j < LIT_LEN; j++)
		if (lit_histogram[j] == 0)
			lit_histogram[j]++;

	lit_tree = create_symbol_subset_huff_tree(lit_tree_array, lit_histogram, LIT_LEN);
#else
	lit_tree = create_huff_tree(lit_tree_array, lit_histogram, LIT_LEN);
#endif
	dist_tree = create_huff_tree(dist_tree_array, dist_histogram, max_dist + 1);

	/* Create a look up table to represent huffman tree above in deflate
	 * standard form after it has been modified to satisfy max depth
	 * criteria.*/
	if (create_huff_lookup(lit_huff_table, LIT_LEN, lit_tree, MAX_DEFLATE_CODE_LEN) > 0) {
		printf("Error, code with invalid length for Deflate standard.\n");
		return 1;
	}

	if (create_huff_lookup(dist_huff_table, DIST_LEN, dist_tree, MAX_DEFLATE_CODE_LEN) > 0) {
		printf("Error, code with invalid length for Deflate standard.\n");
		return 1;
	}

	if (are_hufftables_useable(lit_huff_table, dist_huff_table)) {
		if (create_huff_lookup
		    (lit_huff_table, LIT_LEN, lit_tree, MAX_SAFE_LIT_CODE_LEN) > 0)
			printf("Error, code with invalid length for Deflate standard.\n");
		return 1;

		if (create_huff_lookup
		    (dist_huff_table, DIST_LEN, dist_tree, MAX_SAFE_DIST_CODE_LEN) > 0)
			printf("Error, code with invalid length for Deflate standard.\n");
		return 1;

		if (are_hufftables_useable(lit_huff_table, dist_huff_table)) {
			printf("Error, hufftable is not usable\n");
			return 1;
		}
	}
#ifdef PRINT_CODES
	int i;
	printf("Lit/Len codes\n");
	for (i = 0; i < LIT_TABLE_SIZE - 1; i++)
		printf("Lit %3d: Code 0x%04x, Code_Len %d\n", i, lit_huff_table[i].code,
		       lit_huff_table[i].length);

	printf("EOB %3d: Code 0x%04x, Code_Len %d\n", 256, lit_huff_table[256].code,
	       lit_huff_table[256].length);

	for (i = LIT_TABLE_SIZE; i < LIT_LEN; i++)
		printf("Len %d: Code 0x%04x, Code_Len %d\n", i, lit_huff_table[i].code,
		       lit_huff_table[i].length);
	printf("\n");

	printf("Dist codes \n");
	for (i = 0; i < DIST_LEN; i++)
		printf("Dist %2d: Code 0x%04x, Code_Len %d\n", i, dist_huff_table[i].code,
		       dist_huff_table[i].length);
	printf("\n");
#endif

	create_code_tables(lit_code_table, lit_code_size_table, LIT_TABLE_SIZE,
			   lit_huff_table);
	create_code_tables(dcodes_code_table, dcodes_code_size_table, DIST_LEN,
			   dist_huff_table);
	create_packed_len_table(packed_len_table, lit_huff_table);
	create_packed_dist_table(packed_dist_table, LONG_DIST_TABLE_SIZE, dist_huff_table);

	bit_count =
	    create_header(header, sizeof(header), lit_huff_table, dist_huff_table, LAST_BLOCK);

	file = fopen("hufftables_c.c", "w");
	if (file == NULL) {
		printf("Error creating file hufftables_c.c\n");
		return 1;
	}

	fprint_header(file, header, bit_count, lit_code_table, lit_code_size_table,
		      dcodes_code_table, dcodes_code_size_table, packed_len_table,
		      packed_dist_table);

	fclose(file);

	return 0;
}
