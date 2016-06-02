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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "igzip_lib.h"
#include "igzip_inflate_ref.h"
#include "crc_inflate.h"
#include <math.h>

#ifndef RANDOMS
# define RANDOMS   50
#endif
#ifndef TEST_SEED
# define TEST_SEED 0x1234
#endif

#define IBUF_SIZE  (1024*1024)

#ifndef IGZIP_USE_GZIP_FORMAT
# define DEFLATE 1
#endif

#define str1 "Short test string"
#define str2 "one two three four five six seven eight nine ten eleven twelve " \
		 "thirteen fourteen fifteen sixteen"

#define TYPE0_HDR_SIZE 5	/* Size of a type 0 blocks header in bytes */
#define TYPE0_MAX_SIZE 65535	/* Max length of a type 0 block in bytes (excludes the header) */

#define MAX_LOOPS 20
/* Defines for the possible error conditions */
enum IGZIP_TEST_ERROR_CODES {
	IGZIP_COMP_OK,

	MALLOC_FAILED,
	FILE_READ_FAILED,

	COMPRESS_INCORRECT_STATE,
	COMPRESS_INPUT_STREAM_INTEGRITY_ERROR,
	COMPRESS_OUTPUT_STREAM_INTEGRITY_ERROR,
	COMPRESS_END_OF_STREAM_NOT_SET,
	COMPRESS_ALL_INPUT_FAIL,
	COMPRESS_OUT_BUFFER_OVERFLOW,
	COMPRESS_LOOP_COUNT_OVERFLOW,
	COMPRESS_GENERAL_ERROR,

	INFLATE_END_OF_INPUT,
	INFLATE_INVALID_BLOCK_HEADER,
	INFLATE_INVALID_SYMBOL,
	INFLATE_OUT_BUFFER_OVERFLOW,
	INFLATE_INVALID_NON_COMPRESSED_BLOCK_LENGTH,
	INFLATE_LEFTOVER_INPUT,
	INFLATE_INCORRECT_OUTPUT_SIZE,
	INFLATE_INVALID_LOOK_BACK_DISTANCE,
	INVALID_GZIP_HEADER,
	INCORRECT_GZIP_TRAILER,
	INFLATE_GENERAL_ERROR,

	INVALID_FLUSH_ERROR,

	OVERFLOW_TEST_ERROR,
	RESULT_ERROR
};

const int hdr_bytes = 300;

#ifndef DEFLATE
const uint8_t gzip_hdr[10] = {
	0x1f, 0x8b, 0x08, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0xff
};

const uint32_t gzip_hdr_bytes = 10;
const uint32_t gzip_trl_bytes = 8;

const int trl_bytes = 8;
const int gzip_extra_bytes = 18;

#else
const int trl_bytes = 0;
const int gzip_extra_bytes = 0;

#endif

#define HISTORY_SIZE 32*1024
#define MIN_LENGTH 3
#define MIN_DIST 1

/* Create random compressible data. This is achieved by randomly choosing a
 * random character, or to repeat previous data in the stream for a random
 * length and look back distance. The probability of a random character or a
 * repeat being chosen is semi-randomly chosen by setting max_repeat_data to be
 * differing values */
void create_rand_repeat_data(uint8_t * data, int size)
{
	uint32_t next_data;
	uint8_t *data_start = data;
	uint32_t length, distance;
	uint32_t max_repeat_data = 256;
	uint32_t power = rand() % 32;
	/* An array of the powers of 2 (except the final element which is 0) */
	const uint32_t power_of_2_array[] = {
		0x00000001, 0x00000002, 0x00000004, 0x00000008,
		0x00000010, 0x00000020, 0x00000040, 0x00000080,
		0x00000100, 0x00000200, 0x00000400, 0x00000800,
		0x00001000, 0x00002000, 0x00004000, 0x00008000,
		0x00010000, 0x00020000, 0x00040000, 0x00080000,
		0x00100000, 0x00200000, 0x00400000, 0x00800000,
		0x01000000, 0x02000000, 0x04000000, 0x08000000,
		0x10000000, 0x20000000, 0x40000000, 0x00000000
	};

	max_repeat_data += power_of_2_array[power];

	if (size-- > 0)
		*data++ = rand();

	while (size > 0) {
		next_data = rand() % max_repeat_data;
		if (next_data < 256) {
			*data++ = next_data;
			size--;
		} else if (size < 3) {
			*data++ = rand() % 256;
			size--;
		} else {
			length = (rand() % 256) + MIN_LENGTH;
			if (length > size)
				length = (rand() % (size - 2)) + MIN_LENGTH;

			distance = (rand() % HISTORY_SIZE) + MIN_DIST;
			if (distance > data - data_start)
				distance = (rand() % (data - data_start)) + MIN_DIST;

			size -= length;
			if (distance <= length) {
				while (length-- > 0) {
					*data = *(data - distance);
					data++;
				}
			} else
				memcpy(data, data - distance, length);
		}
	}
}

void print_error(int error_code)
{
	switch (error_code) {
	case IGZIP_COMP_OK:
		break;
	case MALLOC_FAILED:
		printf("error: failed to allocate memory\n");
		break;
	case FILE_READ_FAILED:
		printf("error: failed to read in file\n");
		break;
	case COMPRESS_INCORRECT_STATE:
		printf("error: incorrect stream internal state\n");
		break;
	case COMPRESS_INPUT_STREAM_INTEGRITY_ERROR:
		printf("error: inconsistent stream input buffer\n");
		break;
	case COMPRESS_OUTPUT_STREAM_INTEGRITY_ERROR:
		printf("error: inconsistent stream output buffer\n");
		break;
	case COMPRESS_END_OF_STREAM_NOT_SET:
		printf("error: end of stream not set\n");
		break;
	case COMPRESS_ALL_INPUT_FAIL:
		printf("error: not all input data compressed\n");
		break;
	case COMPRESS_OUT_BUFFER_OVERFLOW:
		printf("error: output buffer overflow while compressing data\n");
		break;
	case COMPRESS_GENERAL_ERROR:
		printf("error: compression failed\n");
		break;
	case INFLATE_END_OF_INPUT:
		printf("error: did not decompress all input\n");
		break;
	case INFLATE_INVALID_BLOCK_HEADER:
		printf("error: invalid header\n");
		break;
	case INFLATE_INVALID_SYMBOL:
		printf("error: invalid symbol found when decompressing input\n");
		break;
	case INFLATE_OUT_BUFFER_OVERFLOW:
		printf("error: output buffer overflow while decompressing data\n");
		break;
	case INFLATE_INVALID_NON_COMPRESSED_BLOCK_LENGTH:
		printf("error: invalid length bits in non-compressed block\n");
		break;
	case INFLATE_GENERAL_ERROR:
		printf("error: decompression failed\n");
		break;
	case INFLATE_LEFTOVER_INPUT:
		printf("error: the trailer of igzip output contains junk\n");
		break;
	case INFLATE_INCORRECT_OUTPUT_SIZE:
		printf("error: incorrect amount of data was decompressed\n");
		break;
	case INFLATE_INVALID_LOOK_BACK_DISTANCE:
		printf("error: invalid look back distance found while decompressing\n");
		break;
	case INVALID_GZIP_HEADER:
		printf("error: incorrect gzip header found when inflating data\n");
		break;
	case INCORRECT_GZIP_TRAILER:
		printf("error: incorrect gzip trailer found when inflating data\n");
		break;
	case INVALID_FLUSH_ERROR:
		printf("error: invalid flush did not cause compression to error\n");
		break;
	case RESULT_ERROR:
		printf("error: decompressed data is not the same as the compressed data\n");
		break;
	case OVERFLOW_TEST_ERROR:
		printf("error: overflow undetected\n");
		break;
	default:
		printf("error: unknown error code\n");
	}
}

void print_uint8_t(uint8_t * array, uint64_t length)
{
	int i;

	const int line_size = 16;
	printf("Length = %lu", length);
	for (i = 0; i < length; i++) {
		if ((i % line_size) == 0)
			printf("\n0x%08x\t", i);
		else
			printf(" ");
		printf("0x%02x,", array[i]);
	}
	printf("\n");
}

#ifndef DEFLATE
uint32_t check_gzip_header(uint8_t * z_buf)
{
	/* These values are defined in RFC 1952 page 4 */
	const uint8_t ID1 = 0x1f, ID2 = 0x8b, CM = 0x08, FLG = 0;
	uint32_t ret = 0;
	int i;
	/* Verify that the gzip header is the one used in hufftables_c.c */
	for (i = 0; i < gzip_hdr_bytes; i++)
		if (z_buf[i] != gzip_hdr[i])
			ret = INVALID_GZIP_HEADER;

	/* Verify that the gzip header is a valid gzip header */
	if (*z_buf++ != ID1)
		ret = INVALID_GZIP_HEADER;

	if (*z_buf++ != ID2)
		ret = INVALID_GZIP_HEADER;

	/* Verfiy compression method is Deflate */
	if (*z_buf++ != CM)
		ret = INVALID_GZIP_HEADER;

	/* The following comparison is specific to how gzip headers are written in igzip */
	/* Verify no extra flags are set */
	if (*z_buf != FLG)
		ret = INVALID_GZIP_HEADER;

	/* The last 6 bytes in the gzip header do not contain any information
	 * important to decomrpessing the data */

	return ret;
}

uint32_t check_gzip_trl(struct inflate_state * gstream)
{
	uint8_t *index = NULL;
	uint32_t crc, ret = 0;

	index = gstream->out_buffer.next_out - gstream->out_buffer.total_out;
	crc = find_crc(index, gstream->out_buffer.total_out);

	if (gstream->out_buffer.total_out != *(uint32_t *) (gstream->in_buffer.next_in + 4) ||
	    crc != *(uint32_t *) gstream->in_buffer.next_in)
		ret = INCORRECT_GZIP_TRAILER;

	return ret;
}
#endif

/* Inflate the  compressed data and check that the decompressed data agrees with the input data */
int inflate_check(uint8_t * z_buf, int z_size, uint8_t * in_buf, int in_size)
{
	/* Test inflate with reference inflate */

	int ret = 0;
	struct inflate_state gstream;
	uint32_t test_size = in_size;
	uint8_t *test_buf = NULL;
	int mem_result = 0;

	assert(in_buf != NULL);

	if (in_size > 0) {
		test_buf = malloc(test_size);

		if (test_buf == NULL)
			return MALLOC_FAILED;
	}
	if (test_buf != NULL)
		memset(test_buf, 0xff, test_size);

#ifndef DEFLATE
	int gzip_hdr_result, gzip_trl_result;

	gzip_hdr_result = check_gzip_header(z_buf);
	z_buf += gzip_hdr_bytes;
	z_size -= gzip_hdr_bytes;
#endif

	igzip_inflate_init(&gstream, z_buf, z_size, test_buf, test_size);
	ret = igzip_inflate(&gstream);

	if (test_buf != NULL)
		mem_result = memcmp(in_buf, test_buf, in_size);

#ifdef VERBOSE
	int i;
	if (mem_result)
		for (i = 0; i < in_size; i++) {
			if (in_buf[i] != test_buf[i]) {
				printf("First incorrect data at 0x%x of 0x%x, 0x%x != 0x%x\n",
				       i, in_size, in_buf[i], test_buf[i]);
				break;
			}
		}
#endif

#ifndef DEFLATE
	gzip_trl_result = check_gzip_trl(&gstream);
	gstream.in_buffer.avail_in -= gzip_trl_bytes;
	gstream.in_buffer.next_in += gzip_trl_bytes;
#endif

	if (test_buf != NULL)
		free(test_buf);

	switch (ret) {
	case 0:
		break;
	case END_OF_INPUT:
		return INFLATE_END_OF_INPUT;
		break;
	case INVALID_BLOCK_HEADER:
		return INFLATE_INVALID_BLOCK_HEADER;
		break;
	case INVALID_SYMBOL:
		return INFLATE_INVALID_SYMBOL;
		break;
	case OUT_BUFFER_OVERFLOW:
		return INFLATE_OUT_BUFFER_OVERFLOW;
		break;
	case INVALID_NON_COMPRESSED_BLOCK_LENGTH:
		return INFLATE_INVALID_NON_COMPRESSED_BLOCK_LENGTH;
		break;
	case INVALID_LOOK_BACK_DISTANCE:
		return INFLATE_INVALID_LOOK_BACK_DISTANCE;
		break;
	default:
		return INFLATE_GENERAL_ERROR;
		break;
	}

	if (gstream.in_buffer.avail_in != 0)
		return INFLATE_LEFTOVER_INPUT;

	if (gstream.out_buffer.total_out != in_size)
		return INFLATE_INCORRECT_OUTPUT_SIZE;

	if (mem_result)
		return RESULT_ERROR;

#ifndef DEFLATE
	if (gzip_hdr_result)
		return INVALID_GZIP_HEADER;

	if (gzip_trl_result)
		return INCORRECT_GZIP_TRAILER;
#endif

	return 0;
}

/* Check if that the state of the data stream is consistent */
int stream_valid_check(struct isal_zstream *stream, uint8_t * in_buf, uint32_t in_size,
		       uint8_t * out_buf, uint32_t out_size, uint32_t in_processed,
		       uint32_t out_processed, uint32_t data_size)
{
	uint32_t total_in, in_buffer_size, total_out, out_buffer_size;

	total_in =
	    (in_size ==
	     0) ? in_processed : (in_processed - in_size) + (stream->next_in - in_buf);
	in_buffer_size = (in_size == 0) ? 0 : stream->next_in - in_buf + stream->avail_in;

	/* Check for a consistent amount of data processed */
	if (total_in != stream->total_in || in_buffer_size != in_size)
		return COMPRESS_INPUT_STREAM_INTEGRITY_ERROR;

	total_out =
	    (out_size == 0) ? out_processed : out_processed + (stream->next_out - out_buf);
	out_buffer_size = (out_size == 0) ? 0 : stream->next_out - out_buf + stream->avail_out;

	/* Check for a consistent amount of data compressed */
	if (total_out != stream->total_out || out_buffer_size != out_size) {
		return COMPRESS_OUTPUT_STREAM_INTEGRITY_ERROR;
	}

	return 0;
}

/* Performs compression with checks to discover and verify the state of the
 * stream
 * stream: compress data structure which has been initialized to use
 * in_buf and out_buf as the buffers
 * data_size: size of all input data
 * compressed_size: size of all available output buffers
 * in_buf: next buffer of data to be compressed
 * in_size: size of in_buf
 * out_buf: next out put buffer where data is stored
 * out_size: size of out_buf
 * in_processed: the amount of input data which has been loaded into buffers
 * to be compressed, this includes the data in in_buf
 * out_processed: the amount of output data which has been compressed and stored,
 * this does not include the data in the current out_buf
*/
int isal_deflate_with_checks(struct isal_zstream *stream, uint32_t data_size,
			     uint32_t compressed_size, uint8_t * in_buf, uint32_t in_size,
			     uint32_t in_processed, uint8_t * out_buf, uint32_t out_size,
			     uint32_t out_processed)
{
	int ret, stream_check;
	struct isal_zstate *state = &stream->internal_state;

#ifdef VERBOSE
	printf("Pre compression\n");
	printf
	    ("data_size       = 0x%05x, in_processed  = 0x%05x, in_size  = 0x%05x, avail_in  = 0x%05x, total_in  = 0x%05x\n",
	     data_size, in_processed, in_size, stream->avail_in, stream->total_in);
	printf
	    ("compressed_size = 0x%05x, out_processed = 0x%05x, out_size = 0x%05x, avail_out = 0x%05x, total_out = 0x%05x\n",
	     compressed_size, out_processed, out_size, stream->avail_out, stream->total_out);
#endif

	ret = isal_deflate(stream);

#ifdef VERBOSE
	printf("Post compression\n");
	printf
	    ("data_size       = 0x%05x, in_processed  = 0x%05x, in_size  = 0x%05x, avail_in  = 0x%05x, total_in  = 0x%05x\n",
	     data_size, in_processed, in_size, stream->avail_in, stream->total_in);
	printf
	    ("compressed_size = 0x%05x, out_processed = 0x%05x, out_size = 0x%05x, avail_out = 0x%05x, total_out = 0x%05x\n",
	     compressed_size, out_processed, out_size, stream->avail_out, stream->total_out);
	printf("\n\n");
#endif

	/* Verify the stream is in a valid state */
	stream_check = stream_valid_check(stream, in_buf, in_size, out_buf, out_size,
					  in_processed, out_processed, data_size);

	if (stream_check != 0)
		return stream_check;

	if (ret != IGZIP_COMP_OK)
		return COMPRESS_GENERAL_ERROR;

	/* Check if the compression is completed */
	if (state->state != ZSTATE_END)
		if (compressed_size - out_processed - (out_size - stream->avail_out) <= 0)
			return COMPRESS_OUT_BUFFER_OVERFLOW;

	return ret;

}

/* Compress the input data into the output buffer where the input buffer and
 * output buffer are randomly segmented to test state information for the
 * compression*/
int compress_multi_pass(uint8_t * data, uint32_t data_size, uint8_t * compressed_buf,
			uint32_t * compressed_size, uint32_t flush_type)
{
	int ret = IGZIP_COMP_OK;
	uint8_t *in_buf = NULL, *out_buf = NULL;
	uint32_t in_size = 0, out_size = 0;
	uint32_t in_processed = 0, out_processed = 0;
	struct isal_zstream stream;
	struct isal_zstate *state = &stream.internal_state;
	uint32_t loop_count = 0;

#ifdef VERBOSE
	printf("Starting Compress Multi Pass\n");
#endif

	create_rand_repeat_data((uint8_t *) & stream, sizeof(stream));

	isal_deflate_init(&stream);

	if (state->state != ZSTATE_NEW_HDR)
		return COMPRESS_INCORRECT_STATE;

	stream.flush = flush_type;
	stream.end_of_stream = 0;

	/* These are set here to allow the loop to run correctly */
	stream.avail_in = 0;
	stream.avail_out = 0;

	while (1) {
		loop_count++;

		/* Setup in buffer for next round of compression */
		if (stream.avail_in == 0) {
			if (flush_type != SYNC_FLUSH || state->state == ZSTATE_NEW_HDR) {
				/* Randomly choose size of the next out buffer */
				in_size = rand() % (data_size + 1);

				/* Limit size of buffer to be smaller than maximum */
				if (in_size >= data_size - in_processed) {
					in_size = data_size - in_processed;
					stream.end_of_stream = 1;
				}

				if (in_size != 0) {
					if (in_buf != NULL) {
						free(in_buf);
						in_buf = NULL;
					}

					in_buf = malloc(in_size);
					if (in_buf == NULL) {
						ret = MALLOC_FAILED;
						break;
					}
					memcpy(in_buf, data + in_processed, in_size);
					in_processed += in_size;

					stream.avail_in = in_size;
					stream.next_in = in_buf;
				}
			}
		}

		/* Setup out buffer for next round of compression */
		if (stream.avail_out == 0) {
			/* Save compressed data inot compressed_buf */
			if (out_buf != NULL) {
				memcpy(compressed_buf + out_processed, out_buf,
				       out_size - stream.avail_out);
				out_processed += out_size - stream.avail_out;
			}

			/* Randomly choose size of the next out buffer */
			out_size = rand() % (*compressed_size + 1);

			/* Limit size of buffer to be smaller than maximum */
			if (out_size > *compressed_size - out_processed)
				out_size = *compressed_size - out_processed;

			if (out_size != 0) {
				if (out_buf != NULL) {
					free(out_buf);
					out_buf = NULL;
				}

				out_buf = malloc(out_size);
				if (out_buf == NULL) {
					ret = MALLOC_FAILED;
					break;
				}

				stream.avail_out = out_size;
				stream.next_out = out_buf;
			}
		}

		ret =
		    isal_deflate_with_checks(&stream, data_size, *compressed_size, in_buf,
					     in_size, in_processed, out_buf, out_size,
					     out_processed);

		if (ret) {
			if (ret == COMPRESS_OUT_BUFFER_OVERFLOW
			    || ret == COMPRESS_INCORRECT_STATE)
				memcpy(compressed_buf + out_processed, out_buf, out_size);
			break;
		}

		/* Check if the compression is completed */
		if (state->state == ZSTATE_END) {
			memcpy(compressed_buf + out_processed, out_buf, out_size);
			*compressed_size = stream.total_out;
			break;
		}

	}

	if (in_buf != NULL)
		free(in_buf);
	if (out_buf != NULL)
		free(out_buf);

	if (ret == COMPRESS_OUT_BUFFER_OVERFLOW && flush_type == SYNC_FLUSH
	    && loop_count >= MAX_LOOPS)
		ret = COMPRESS_LOOP_COUNT_OVERFLOW;

	return ret;

}

/* Compress the input data into the outbuffer in one call to isal_deflate */
int compress_single_pass(uint8_t * data, uint32_t data_size, uint8_t * compressed_buf,
			 uint32_t * compressed_size, uint32_t flush_type)
{
	int ret = IGZIP_COMP_OK;
	struct isal_zstream stream;
	struct isal_zstate *state = &stream.internal_state;

#ifdef VERBOSE
	printf("Starting Compress Single Pass\n");
#endif

	create_rand_repeat_data((uint8_t *) & stream, sizeof(stream));

	isal_deflate_init(&stream);

	if (state->state != ZSTATE_NEW_HDR)
		return COMPRESS_INCORRECT_STATE;

	stream.flush = flush_type;
	stream.avail_in = data_size;
	stream.next_in = data;
	stream.avail_out = *compressed_size;
	stream.next_out = compressed_buf;
	stream.end_of_stream = 1;

	ret =
	    isal_deflate_with_checks(&stream, data_size, *compressed_size, data, data_size,
				     data_size, compressed_buf, *compressed_size, 0);

	/* Check if the compression is completed */
	if (state->state == ZSTATE_END)
		*compressed_size = stream.total_out;
	else if (flush_type == SYNC_FLUSH && stream.avail_out < 16)
		ret = COMPRESS_OUT_BUFFER_OVERFLOW;

	return ret;

}

/* Statelessly compress the input buffer into the output buffer */
int compress_stateless(uint8_t * data, uint32_t data_size, uint8_t * compressed_buf,
		       uint32_t * compressed_size)
{
	int ret = IGZIP_COMP_OK;
	struct isal_zstream stream;

	create_rand_repeat_data((uint8_t *) & stream, sizeof(stream));

	isal_deflate_init(&stream);

	stream.avail_in = data_size;
	stream.end_of_stream = 1;
	stream.next_in = data;
	stream.flush = NO_FLUSH;

	stream.avail_out = *compressed_size;
	stream.next_out = compressed_buf;

	ret = isal_deflate_stateless(&stream);

	/* verify the stream */
	if (stream.next_in - data != stream.total_in ||
	    stream.total_in + stream.avail_in != data_size)
		return COMPRESS_INPUT_STREAM_INTEGRITY_ERROR;

	if (stream.next_out - compressed_buf != stream.total_out ||
	    stream.total_out + stream.avail_out != *compressed_size)
		return COMPRESS_OUTPUT_STREAM_INTEGRITY_ERROR;

	if (ret != IGZIP_COMP_OK) {
		if (ret == STATELESS_OVERFLOW)
			return COMPRESS_OUT_BUFFER_OVERFLOW;
		else
			return COMPRESS_GENERAL_ERROR;
	}

	if (!stream.end_of_stream) {
		return COMPRESS_END_OF_STREAM_NOT_SET;
	}

	if (stream.avail_in != 0)
		return COMPRESS_ALL_INPUT_FAIL;

	*compressed_size = stream.total_out;

	return ret;

}

/*Compress the input buffer into the output buffer, but switch the flush type in
 * the middle of the compression to test what happens*/
int compress_swap_flush(uint8_t * data, uint32_t data_size, uint8_t * compressed_buf,
			uint32_t * compressed_size, uint32_t flush_type)
{
	int ret = IGZIP_COMP_OK;
	struct isal_zstream stream;
	struct isal_zstate *state = &stream.internal_state;
	uint32_t partial_size;

#ifdef VERBOSE
	printf("Starting Compress Swap Flush\n");
#endif

	isal_deflate_init(&stream);

	if (state->state != ZSTATE_NEW_HDR)
		return COMPRESS_INCORRECT_STATE;

	partial_size = rand() % (data_size + 1);

	stream.flush = flush_type;
	stream.avail_in = partial_size;
	stream.next_in = data;
	stream.avail_out = *compressed_size;
	stream.next_out = compressed_buf;
	stream.end_of_stream = 0;

	ret =
	    isal_deflate_with_checks(&stream, data_size, *compressed_size, data, partial_size,
				     partial_size, compressed_buf, *compressed_size, 0);

	if (ret)
		return ret;

	if (flush_type == NO_FLUSH)
		flush_type = SYNC_FLUSH;
	else
		flush_type = NO_FLUSH;

	stream.flush = flush_type;
	stream.avail_in = data_size - partial_size;
	stream.next_in = data + partial_size;
	stream.end_of_stream = 1;

	ret =
	    isal_deflate_with_checks(&stream, data_size, *compressed_size, data + partial_size,
				     data_size - partial_size, data_size, compressed_buf,
				     *compressed_size, 0);

	if (ret == COMPRESS_GENERAL_ERROR)
		return INVALID_FLUSH_ERROR;

	*compressed_size = stream.total_out;

	return ret;
}

/* Test isal_deflate_stateless */
int test_compress_stateless(uint8_t * in_buf, uint32_t in_size)
{
	int ret = IGZIP_COMP_OK;
	uint32_t z_size, overflow;
	uint8_t *z_buf = NULL;

	/* Test non-overflow case where a type 0 block is not written */
	z_size = 2 * in_size + hdr_bytes + trl_bytes;

	z_buf = malloc(z_size);

	if (z_buf == NULL)
		return MALLOC_FAILED;
	create_rand_repeat_data(z_buf, z_size);

	ret = compress_stateless(in_buf, in_size, z_buf, &z_size);

	if (!ret)
		ret = inflate_check(z_buf, z_size, in_buf, in_size);

	if (z_buf != NULL) {
		free(z_buf);
		z_buf = NULL;
	}

	print_error(ret);

	/*Test non-overflow case where a type 0 block is possible to be written */
	z_size =
	    TYPE0_HDR_SIZE * ((in_size + TYPE0_MAX_SIZE - 1) / TYPE0_MAX_SIZE) + in_size +
	    gzip_extra_bytes;

	if (z_size == gzip_extra_bytes)
		z_size += TYPE0_HDR_SIZE;

	if (z_size < 8)
		z_size = 8;

	z_buf = malloc(z_size);

	if (z_buf == NULL)
		return MALLOC_FAILED;

	create_rand_repeat_data(z_buf, z_size);

	ret = compress_stateless(in_buf, in_size, z_buf, &z_size);
	if (!ret)
		ret = inflate_check(z_buf, z_size, in_buf, in_size);
#ifdef VERBOSE
	if (ret) {
		printf("Compressed array: ");
		print_uint8_t(z_buf, z_size);
		printf("\n");
		printf("Data: ");
		print_uint8_t(in_buf, in_size);
	}
#endif

	if (!ret) {
		free(z_buf);
		z_buf = NULL;

		/* Test random overflow case */
		z_size = rand() % z_size;

		if (z_size > in_size)
			z_size = rand() & in_size;

		if (z_size > 0) {
			z_buf = malloc(z_size);

			if (z_buf == NULL)
				return MALLOC_FAILED;
		}

		overflow = compress_stateless(in_buf, in_size, z_buf, &z_size);

		if (overflow != COMPRESS_OUT_BUFFER_OVERFLOW) {
#ifdef VERBOSE
			printf("overflow error = %d\n", overflow);
			print_error(overflow);
			if (overflow == 0) {
				overflow = inflate_check(z_buf, z_size, in_buf, in_size);
				printf("inflate ret = %d\n", overflow);
				print_error(overflow);
			}
			printf("Compressed array: ");
			print_uint8_t(z_buf, z_size);
			printf("\n");
			printf("Data: ");
			print_uint8_t(in_buf, in_size);
#endif
			ret = OVERFLOW_TEST_ERROR;
		}
	}

	print_error(ret);

	if (z_buf != NULL)
		free(z_buf);

	return ret;
}

/* Test isal_deflate */
int test_compress(uint8_t * in_buf, uint32_t in_size, uint32_t flush_type)
{
	int ret = IGZIP_COMP_OK, fin_ret = IGZIP_COMP_OK;
	uint32_t overflow = 0;
	uint32_t z_size, z_size_max, z_compressed_size;
	uint8_t *z_buf = NULL;

	/* Test a non overflow case */
	if (flush_type == NO_FLUSH)
		z_size_max = 2 * in_size + hdr_bytes + trl_bytes + 2;
	else if (flush_type == SYNC_FLUSH)
		z_size_max = 2 * in_size + MAX_LOOPS * (hdr_bytes + trl_bytes + 5);
	else {
		printf("Invalid Flush Parameter\n");
		return COMPRESS_GENERAL_ERROR;
	}

	z_size = z_size_max;

	z_buf = malloc(z_size);
	if (z_buf == NULL) {
		print_error(MALLOC_FAILED);
		return MALLOC_FAILED;
	}
	create_rand_repeat_data(z_buf, z_size_max);

	ret = compress_single_pass(in_buf, in_size, z_buf, &z_size, flush_type);

	if (!ret)
		ret = inflate_check(z_buf, z_size, in_buf, in_size);

	if (ret) {
#ifdef VERBOSE
		printf("Compressed array: ");
		print_uint8_t(z_buf, z_size);
		printf("\n");
		printf("Data: ");
		print_uint8_t(in_buf, in_size);
#endif
		printf("Failed on compress single pass\n");
		print_error(ret);
	}

	fin_ret |= ret;

	z_compressed_size = z_size;
	z_size = z_size_max;
	create_rand_repeat_data(z_buf, z_size_max);

	ret = compress_multi_pass(in_buf, in_size, z_buf, &z_size, flush_type);

	if (!ret)
		ret = inflate_check(z_buf, z_size, in_buf, in_size);

	if (ret) {
#ifdef VERBOSE
		printf("Compressed array: ");
		print_uint8_t(z_buf, z_size);
		printf("\n");
		printf("Data: ");
		print_uint8_t(in_buf, in_size);
#endif
		printf("Failed on compress multi pass\n");
		print_error(ret);
	}

	fin_ret |= ret;

	ret = 0;

	/* Test random overflow case */
	if (flush_type == SYNC_FLUSH && z_compressed_size > in_size)
		z_compressed_size = in_size + 1;

	z_size = rand() % z_compressed_size;
	create_rand_repeat_data(z_buf, z_size_max);

	overflow = compress_single_pass(in_buf, in_size, z_buf, &z_size, flush_type);

	if (overflow != COMPRESS_OUT_BUFFER_OVERFLOW) {
		if (overflow == 0)
			ret = inflate_check(z_buf, z_size, in_buf, in_size);

		/* Rarely single pass overflow will compresses data
		 * better than the initial run. This is to stop that
		 * case from erroring. */
		if (overflow != 0 || ret != 0) {
#ifdef VERBOSE
			printf("overflow error = %d\n", overflow);
			print_error(overflow);
			printf("inflate ret = %d\n", ret);
			print_error(overflow);

			printf("Compressed array: ");
			print_uint8_t(z_buf, z_size);
			printf("\n");
			printf("Data: ");
			print_uint8_t(in_buf, in_size);
#endif
			printf("Failed on compress multi pass overflow\n");
			print_error(ret);
			ret = OVERFLOW_TEST_ERROR;
		}
	}

	fin_ret |= ret;

	if (flush_type == NO_FLUSH) {
		create_rand_repeat_data(z_buf, z_size_max);

		overflow = compress_multi_pass(in_buf, in_size, z_buf, &z_size, flush_type);

		if (overflow != COMPRESS_OUT_BUFFER_OVERFLOW) {
			if (overflow == 0)
				ret = inflate_check(z_buf, z_size, in_buf, in_size);

			/* Rarely multi pass overflow will compresses data
			 * better than the initial run. This is to stop that
			 * case from erroring */
			if (overflow != 0 || ret != 0) {
#ifdef VERBOSE
				printf("overflow error = %d\n", overflow);
				print_error(overflow);
				printf("inflate ret = %d\n", ret);
				print_error(overflow);

				printf("Compressed array: ");
				print_uint8_t(z_buf, z_size);
				printf("\n");
				printf("Data: ");
				print_uint8_t(in_buf, in_size);
#endif
				printf("Failed on compress multi pass overflow\n");
				print_error(ret);
				ret = OVERFLOW_TEST_ERROR;
			}
		}
		fin_ret |= ret;
	}

	free(z_buf);

	return fin_ret;
}

/* Test swapping flush types in the middle of compression */
int test_flush(uint8_t * in_buf, uint32_t in_size)
{
	int fin_ret = IGZIP_COMP_OK, ret;
	uint32_t z_size, flush_type = 0;
	uint8_t *z_buf = NULL;

	z_size = 2 * in_size + 2 * (hdr_bytes + trl_bytes) + 8;

	z_buf = malloc(z_size);

	if (z_buf == NULL)
		return MALLOC_FAILED;

	create_rand_repeat_data(z_buf, z_size);

	while (flush_type == NO_FLUSH || flush_type == SYNC_FLUSH)
		flush_type = rand();

	/* Test invalid flush */
	ret = compress_single_pass(in_buf, in_size, z_buf, &z_size, flush_type);

	if (ret == COMPRESS_GENERAL_ERROR)
		ret = 0;
	else {
		printf("Failed when passing invalid flush parameter\n");
		ret = INVALID_FLUSH_ERROR;
	}

	fin_ret |= ret;
	print_error(ret);

	create_rand_repeat_data(z_buf, z_size);

	/* Test the valid case of SYNC_FLUSH followed by NO_FLUSH */
	ret = compress_swap_flush(in_buf, in_size, z_buf, &z_size, rand() % 2);

	if (!ret)
		ret = inflate_check(z_buf, z_size, in_buf, in_size);

	if (ret) {
#ifdef VERBOSE
		printf("Compressed array: ");
		print_uint8_t(z_buf, z_size);
		printf("\n");
		printf("Data: ");
		print_uint8_t(in_buf, in_size);
#endif
		printf("Failed on swapping from SYNC_FLUSH to NO_FLUSH\n");
		print_error(ret);
	}

	fin_ret |= ret;
	print_error(ret);

	return fin_ret;
}

int get_filesize(FILE * f)
{
	int curr, end;

	curr = ftell(f);	/* Save current position */
	fseek(f, 0L, SEEK_END);
	end = ftell(f);
	fseek(f, curr, SEEK_SET);	/* Restore position */
	return end;
}

/* Run multiple compression tests on data stored in a file */
int test_compress_file(char *file_name)
{
	int ret = IGZIP_COMP_OK;
	uint32_t in_size;
	uint8_t *in_buf = NULL;
	FILE *in_file = NULL;

	in_file = fopen(file_name, "rb");
	if (!in_file)
		return FILE_READ_FAILED;

	in_size = get_filesize(in_file);
	if (in_size != 0) {
		in_buf = malloc(in_size);
		if (in_buf == NULL)
			return MALLOC_FAILED;
		fread(in_buf, 1, in_size, in_file);
	}

	ret |= test_compress_stateless(in_buf, in_size);
	ret |= test_compress(in_buf, in_size, NO_FLUSH);
	ret |= test_compress(in_buf, in_size, SYNC_FLUSH);
	ret |= test_flush(in_buf, in_size);

	if (ret)
		printf("Failed on file %s\n", file_name);

	if (in_buf != NULL)
		free(in_buf);

	return ret;
}

int main(int argc, char *argv[])
{
	int i = 0, ret = 0, fin_ret = 0;
	uint32_t in_size = 0, offset = 0;
	uint8_t *in_buf;

#ifndef VERBOSE
	setbuf(stdout, NULL);
#endif

	printf("Window Size: %d K\n", HIST_SIZE);
	printf("Test Seed  : %d\n", TEST_SEED);
	printf("Randoms    : %d\n", RANDOMS);
	srand(TEST_SEED);

	in_buf = malloc(IBUF_SIZE);
	if (in_buf == NULL) {
		fprintf(stderr, "Can't allocate in_buf memory\n");
		return -1;
	}

	if (argc > 1) {
		printf("igzip_rand_test files:        ");

		for (i = 1; i < argc; i++) {
			ret |= test_compress_file(argv[i]);
			if (ret)
				return ret;
		}

		printf("................");
		printf("%s\n", ret ? "Fail" : "Pass");
		fin_ret |= ret;
	}

	printf("igzip_rand_test stateless:    ");

	ret = test_compress_stateless((uint8_t *) str1, sizeof(str1));
	if (ret)
		return ret;

	ret |= test_compress_stateless((uint8_t *) str2, sizeof(str2));
	if (ret)
		return ret;

	for (i = 0; i < RANDOMS; i++) {
		in_size = rand() % (IBUF_SIZE + 1);
		offset = rand() % (IBUF_SIZE + 1 - in_size);
		in_buf += offset;

		create_rand_repeat_data(in_buf, in_size);

		ret |= test_compress_stateless(in_buf, in_size);

		in_buf -= offset;

		if (i % (RANDOMS / 16) == 0)
			printf(".");

		if (ret)
			return ret;
	}

	fin_ret |= ret;

	printf("%s\n", ret ? "Fail" : "Pass");

	printf("igzip_rand_test NO_FLUSH:     ");

	ret = test_compress((uint8_t *) str1, sizeof(str1), NO_FLUSH);
	if (ret)
		return ret;

	ret |= test_compress((uint8_t *) str2, sizeof(str2), NO_FLUSH);
	if (ret)
		return ret;

	for (i = 0; i < RANDOMS; i++) {
		in_size = rand() % (IBUF_SIZE + 1);
		offset = rand() % (IBUF_SIZE + 1 - in_size);
		in_buf += offset;

		create_rand_repeat_data(in_buf, in_size);

		ret |= test_compress(in_buf, in_size, NO_FLUSH);

		in_buf -= offset;

		if (i % (RANDOMS / 16) == 0)
			printf(".");
		if (ret)
			return ret;
	}

	fin_ret |= ret;

	printf("%s\n", ret ? "Fail" : "Pass");

	printf("igzip_rand_test SYNC_FLUSH:   ");

	ret = test_compress((uint8_t *) str1, sizeof(str1), SYNC_FLUSH);
	if (ret)
		return ret;

	ret |= test_compress((uint8_t *) str2, sizeof(str2), SYNC_FLUSH);
	if (ret)
		return ret;

	for (i = 0; i < RANDOMS; i++) {
		in_size = rand() % (IBUF_SIZE + 1);
		offset = rand() % (IBUF_SIZE + 1 - in_size);
		in_buf += offset;

		create_rand_repeat_data(in_buf, in_size);

		ret |= test_compress(in_buf, in_size, SYNC_FLUSH);

		in_buf -= offset;

		if (i % (RANDOMS / 16) == 0)
			printf(".");
		if (ret)
			return ret;
	}

	fin_ret |= ret;

	printf("%s\n", ret ? "Fail" : "Pass");

	printf("igzip rand test finished:     %s\n",
	       fin_ret ? "Some tests failed" : "All tests passed");

	return fin_ret != IGZIP_COMP_OK;
}
