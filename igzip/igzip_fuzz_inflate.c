#include <stdio.h>
#include <assert.h>
#include <zlib.h>
#include "huff_codes.h"
#include "igzip_lib.h"
#include "test.h"

#define OUT_BUFFER_SIZE 64*1024
int get_filesize(FILE * f)
{
	int curr, end;

	curr = ftell(f);	/* Save current position */
	fseek(f, 0L, SEEK_END);
	end = ftell(f);
	fseek(f, curr, SEEK_SET);	/* Restore position */
	return end;
}

int main(int argc, char *argv[])
{
	FILE *in = NULL;
	unsigned char *in_buf = NULL, *isal_out_buf = NULL, *zlib_out_buf = NULL;
	int in_file_size, out_buf_size, zret, iret;
	struct inflate_state *state = NULL;
	z_stream zstate;
	char z_msg_invalid_code_set[] = "invalid code lengths set";
	char z_msg_invalid_dist_set[] = "invalid distances set";
	char z_msg_invalid_lit_len_set[] = "invalid literal/lengths set";

	if (argc != 2) {
		fprintf(stderr, "Usage: isal_inflate_file_perf  infile\n"
			"\t - Runs multiple iterations of igzip on a file to "
			"get more accurate time results.\n");
		exit(1);
	}
	in = fopen(argv[1], "rb");
	if (!in) {
		fprintf(stderr, "Can't open %s for reading\n", argv[1]);
		exit(1);
	}

	/* Allocate space for entire input file and output
	 * (assuming some possible expansion on output size)
	 */
	in_file_size = get_filesize(in);

	out_buf_size = OUT_BUFFER_SIZE;

	state = malloc(sizeof(struct inflate_state));
	in_buf = malloc(in_file_size);
	isal_out_buf = malloc(OUT_BUFFER_SIZE);
	zlib_out_buf = malloc(OUT_BUFFER_SIZE);

	if (state == NULL || in_buf == NULL || isal_out_buf == NULL || zlib_out_buf == NULL) {
		fprintf(stderr, "Failed to malloc input and outputs buffers");
		exit(1);
	}

	fread(in_buf, 1, in_file_size, in);

	/* Inflate data with isal_inflate */
	memset(state, 0xff, sizeof(struct inflate_state));

	isal_inflate_init(state);
	state->next_in = in_buf;
	state->avail_in = in_file_size;
	state->next_out = isal_out_buf;
	state->avail_out = out_buf_size;

	iret = isal_inflate_stateless(state);

	/* Inflate data with zlib */
	zstate.zalloc = Z_NULL;
	zstate.zfree = Z_NULL;
	zstate.opaque = Z_NULL;
	zstate.avail_in = in_file_size;
	zstate.next_in = in_buf;
	zstate.avail_out = out_buf_size;
	zstate.next_out = zlib_out_buf;
	inflateInit2(&zstate, -15);

	zret = inflate(&zstate, Z_FINISH);

	if (zret == Z_STREAM_END) {
		/* If zlib finished, assert isal finished with the same answer */
		assert(state->block_state == ISAL_BLOCK_FINISH);
		assert(zstate.total_out == state->total_out);
		assert(memcmp(isal_out_buf, zlib_out_buf, state->total_out) == 0);
	} else if (zret < 0) {
		if (zret != Z_BUF_ERROR)
			/* If zlib errors, assert isal errors, excluding a few
			 * cases where zlib is overzealous */
			assert(iret < 0 || strcmp(zstate.msg, z_msg_invalid_code_set) == 0
			       || strcmp(zstate.msg, z_msg_invalid_dist_set) == 0
			       || strcmp(zstate.msg, z_msg_invalid_lit_len_set) == 0);
	} else
		/* If zlib did not finish or error, assert isal did not finish
		 *  or that isal found an invalid header since isal notices the
		 *  error faster than zlib */
		assert(iret > 0 || iret == ISAL_INVALID_BLOCK);

	return 0;
}
