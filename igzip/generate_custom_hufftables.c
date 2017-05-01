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
 * *WARNING* generate custom hufftables must be compiled with a IGZIP_HIST_SIZE
 * that is at least as large as the IGZIP_HIST_SIZE used by igzip. By default
 * IGZIP_HIST_SIZE is 32K, the maximum usable IGZIP_HIST_SIZE is 32K. The reason
 * for this is to generate better compression. Igzip cannot produce look back
 * distances with sizes larger than the IGZIP_HIST_SIZE igzip was compiled with,
 * so look back distances with sizes larger than IGZIP_HIST_SIZE are not
 * assigned a huffman code. The definition of LONGER_HUFFTABLES must be
 * consistent as well since that definition changes the size of the structures
 * printed by this tool.
 *
 */

#include <stdint.h>
#include <stdio.h>
#include <inttypes.h>
#include <string.h>
#include <stdlib.h>
#include "igzip_lib.h"

/*These max code lengths are limited by how the data is stored in
 * hufftables.asm. The deflate standard max is 15.*/

#define MAX_HEADER_SIZE ISAL_DEF_MAX_HDR_SIZE

#define GZIP_HEADER_SIZE 10
#define GZIP_TRAILER_SIZE 8
#define ZLIB_HEADER_SIZE 2
#define ZLIB_TRAILER_SIZE 4

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

void fprint_hufftables(FILE * output_file, char *hufftables_name,
		       struct isal_hufftables *hufftables)
{
	fprintf(output_file, "struct isal_hufftables %s = {\n\n", hufftables_name);

	fprint_uint8_table(output_file, hufftables->deflate_hdr,
			   hufftables->deflate_hdr_count +
			   (hufftables->deflate_hdr_extra_bits + 7) / 8,
			   "\t.deflate_hdr = {", "},\n\n", "\t\t");

	fprintf(output_file, "\t.deflate_hdr_count = %d,\n", hufftables->deflate_hdr_count);
	fprintf(output_file, "\t.deflate_hdr_extra_bits = %d,\n\n",
		hufftables->deflate_hdr_extra_bits);

	fprint_uint32_table(output_file, hufftables->dist_table, IGZIP_DIST_TABLE_SIZE,
			    "\t.dist_table = {", "},\n\n", "\t\t");

	fprint_uint32_table(output_file, hufftables->len_table, IGZIP_LEN_TABLE_SIZE,
			    "\t.len_table = {", "},\n\n", "\t\t");

	fprint_uint16_table(output_file, hufftables->lit_table, IGZIP_LIT_TABLE_SIZE,
			    "\t.lit_table = {", "},\n\n", "\t\t");
	fprint_uint8_table(output_file, hufftables->lit_table_sizes, IGZIP_LIT_TABLE_SIZE,
			   "\t.lit_table_sizes = {", "},\n\n", "\t\t");

	fprint_uint16_table(output_file, hufftables->dcodes,
			    ISAL_DEF_DIST_SYMBOLS - IGZIP_DECODE_OFFSET,
			    "\t.dcodes = {", "},\n\n", "\t\t");
	fprint_uint8_table(output_file, hufftables->dcodes_sizes,
			   ISAL_DEF_DIST_SYMBOLS - IGZIP_DECODE_OFFSET,
			   "\t.dcodes_sizes = {", "}\n", "\t\t");
	fprintf(output_file, "};\n");
}

void fprint_header(FILE * output_file)
{

	fprintf(output_file, "#include <stdint.h>\n");
	fprintf(output_file, "#include <igzip_lib.h>\n\n");

	fprintf(output_file, "#if IGZIP_HIST_SIZE > %d\n"
		"# error \"Invalid history size for the custom hufftable\"\n"
		"#endif\n", IGZIP_HIST_SIZE);

#ifdef LONGER_HUFFTABLE
	fprintf(output_file, "#ifndef LONGER_HUFFTABLE\n"
		"# error \"Custom hufftable requires LONGER_HUFFTABLE to be defined \"\n"
		"#endif\n");
#else
	fprintf(output_file, "#ifdef LONGER_HUFFTABLE\n"
		"# error \"Custom hufftable requires LONGER_HUFFTABLE to not be defined \"\n"
		"#endif\n");
#endif
	fprintf(output_file, "\n");

	fprintf(output_file, "const uint8_t gzip_hdr[] = {\n"
		"\t0x1f, 0x8b, 0x08, 0x00, 0x00,\n" "\t0x00, 0x00, 0x00, 0x00, 0xff\t};\n\n");

	fprintf(output_file, "const uint32_t gzip_hdr_bytes = %d;\n", GZIP_HEADER_SIZE);
	fprintf(output_file, "const uint32_t gzip_trl_bytes = %d;\n\n", GZIP_TRAILER_SIZE);

	fprintf(output_file, "const uint8_t zlib_hdr[] = { 0x78, 0x01 };\n\n");
	fprintf(output_file, "const uint32_t zlib_hdr_bytes = %d;\n", ZLIB_HEADER_SIZE);
	fprintf(output_file, "const uint32_t zlib_trl_bytes = %d;\n", ZLIB_TRAILER_SIZE);
}

int main(int argc, char *argv[])
{
	long int file_length;
	uint8_t *stream = NULL;
	struct isal_hufftables hufftables;
	struct isal_huff_histogram histogram;
	struct isal_zstream tmp_stream;
	FILE *file;

	if (argc == 1) {
		printf("Error, no input file.\n");
		return 1;
	}

	memset(&histogram, 0, sizeof(histogram));	/* Initialize histograms. */

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

	isal_create_hufftables(&hufftables, &histogram);

	file = fopen("hufftables_c.c", "w");
	if (file == NULL) {
		printf("Error creating file hufftables_c.c\n");
		return 1;
	}

	fprint_header(file);

	fprintf(file, "\n");

	fprint_hufftables(file, "hufftables_default", &hufftables);

	fprintf(file, "\n");

	isal_deflate_stateless_init(&tmp_stream);
	isal_deflate_set_hufftables(&tmp_stream, NULL, IGZIP_HUFFTABLE_STATIC);
	fprint_hufftables(file, "hufftables_static", tmp_stream.hufftables);

	fclose(file);

	return 0;
}
