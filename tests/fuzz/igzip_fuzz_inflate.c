#define _FILE_OFFSET_BITS 64
#include <stdio.h>
#include <assert.h>
#include <zlib.h>
#include "huff_codes.h"
#include "igzip_lib.h"
#include "test.h"

extern int LLVMFuzzerTestOneInput(const uint8_t * data, size_t size);

int main(int argc, char *argv[])
{
	FILE *in = NULL;
	unsigned char *in_buf = NULL;
	uint64_t in_file_size;

	if (argc != 2) {
		fprintf(stderr, "Usage: isal_fuzz_inflate <infile>\n");
		exit(1);
	}
	in = fopen(argv[1], "rb");
	if (!in) {
		fprintf(stderr, "Can't open %s for reading\n", argv[1]);
		exit(1);
	}
	in_file_size = get_filesize(in);
	in_buf = malloc(in_file_size);

	if (in_buf == NULL) {
		fprintf(stderr, "Failed to malloc input and outputs buffers\n");
		exit(1);
	}

	fread(in_buf, 1, in_file_size, in);

	return LLVMFuzzerTestOneInput(in_buf, in_file_size);
}
