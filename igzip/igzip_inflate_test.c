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
#include <zlib.h>
#include "igzip_inflate_ref.h"
#include "huff_codes.h"

/*Don't use file larger memory can support because compression and decompression
 * are done in a stateless manner. */
#define MAX_INPUT_FILE_SIZE 2L*1024L*1024L*1024L

int test(uint8_t * compressed_stream, uint64_t * compressed_length,
	 uint8_t * uncompressed_stream, int uncompressed_length,
	 uint8_t * uncompressed_test_stream)
{
	struct inflate_state state;
	int ret;
	ret =
	    compress2(compressed_stream, compressed_length, uncompressed_stream,
		      uncompressed_length, 9);
	if (ret) {
		printf("Failed compressing input with exit code %d", ret);
		return ret;
	}

	igzip_inflate_init(&state, compressed_stream + 2, *compressed_length - 2,
			   uncompressed_test_stream, uncompressed_length);
	ret = igzip_inflate(&state);

	switch (ret) {
	case 0:
		break;
	case END_OF_INPUT:
		printf(" did not decompress all input\n");
		return END_OF_INPUT;
		break;
	case INVALID_BLOCK_HEADER:
		printf("  invalid header\n");
		return INVALID_BLOCK_HEADER;
		break;
	case INVALID_SYMBOL:
		printf(" invalid symbol\n");
		return INVALID_SYMBOL;
		break;
	case OUT_BUFFER_OVERFLOW:
		printf(" out buffer overflow\n");
		return OUT_BUFFER_OVERFLOW;
		break;
	case INVALID_NON_COMPRESSED_BLOCK_LENGTH:
		printf("Invalid length bits in non-compressed block\n");
		return INVALID_NON_COMPRESSED_BLOCK_LENGTH;
		break;
	case INVALID_LOOK_BACK_DISTANCE:
		printf("Invalid lookback distance");
		return INVALID_LOOK_BACK_DISTANCE;
		break;
	default:
		printf(" error\n");
		return -1;
		break;
	}

	if (state.out_buffer.total_out != uncompressed_length) {
		printf("incorrect amount of data was decompressed from compressed data\n");
		printf("%d decompressed of %d compressed", state.out_buffer.total_out,
		       uncompressed_length);
		return -1;
	}
	if (memcmp(uncompressed_stream, uncompressed_test_stream, uncompressed_length)) {
		printf(" decompressed data is not the same as the compressed data\n");
		return -1;
	}
	return 0;
}

int main(int argc, char **argv)
{
	int i, j, ret = 0, fin_ret = 0;
	FILE *file;
	uint64_t compressed_length, file_length, uncompressed_length;
	uint8_t *uncompressed_stream, *compressed_stream, *uncompressed_test_stream;

	if (argc == 1)
		printf("Error, no input file\n");

	for (i = 1; i < argc; i++) {
		file = fopen(argv[i], "r");
		if (file == NULL) {
			printf("Error opening file %s\n", argv[i]);
			return 1;
		} else
			printf("Starting file %s", argv[i]);

		fseek(file, 0, SEEK_END);
		file_length = ftell(file);
		fseek(file, 0, SEEK_SET);
		file_length -= ftell(file);
		if (file_length > MAX_INPUT_FILE_SIZE) {
			printf("File too large to run on this test\n");
			fclose(file);
			continue;
		}
		compressed_length = compressBound(file_length);
		uncompressed_stream = malloc(file_length);
		compressed_stream = malloc(compressed_length);
		uncompressed_test_stream = malloc(file_length);

		if (uncompressed_stream == NULL) {
			printf("Failed to allocate memory\n");
			exit(0);
		}

		if (compressed_stream == NULL) {
			printf("Failed to allocate memory\n");
			exit(0);
		}

		if (uncompressed_test_stream == NULL) {
			printf("Failed to allocate memory\n");
			exit(0);
		}

		uncompressed_length = fread(uncompressed_stream, 1, file_length, file);
		ret =
		    test(compressed_stream, &compressed_length, uncompressed_stream,
			 uncompressed_length, uncompressed_test_stream);
		if (ret) {
			for (j = 0; j < compressed_length; j++) {
				if ((j & 31) == 0)
					printf("\n");
				else
					printf(" ");
				printf("0x%02x,", compressed_stream[j]);

			}
			printf("\n");

		}

		fclose(file);
		free(compressed_stream);
		free(uncompressed_stream);
		free(uncompressed_test_stream);

		if (ret) {
			printf(" ... Fail with exit code %d\n", ret);
			return ret;
		} else
			printf(" ... Pass\n");

		fin_ret |= ret;
	}
	return fin_ret;
}
