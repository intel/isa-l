#include "igzip_lib.h"
#include "huffman.h"
#include "encode_df.h"
#include "igzip_level_buf_structs.h"

extern void gen_icf_map_lh1(struct isal_zstream *, struct deflate_icf *, uint32_t);
extern void set_long_icf_fg(uint8_t *, uint8_t *, struct deflate_icf *, struct level_buf *);
extern void isal_deflate_icf_body_lvl1(struct isal_zstream *);
extern void isal_deflate_icf_body_lvl2(struct isal_zstream *);
extern void isal_deflate_icf_body_lvl3(struct isal_zstream *);
/*
*************************************************************
 * Helper functions
 ************************************************************
*/
static inline void write_deflate_icf(struct deflate_icf *icf, uint32_t lit_len,
				     uint32_t lit_dist, uint32_t extra_bits)
{
	/* icf->lit_len = lit_len; */
	/* icf->lit_dist = lit_dist; */
	/* icf->dist_extra = extra_bits; */

	*(uint32_t *) icf = lit_len | (lit_dist << LIT_LEN_BIT_COUNT)
	    | (extra_bits << (LIT_LEN_BIT_COUNT + DIST_LIT_BIT_COUNT));
}

void set_long_icf_fg_base(uint8_t * next_in, uint8_t * end_in,
			  struct deflate_icf *match_lookup, struct level_buf *level_buf)
{
	uint32_t dist_code, dist_extra, dist, len;
	uint32_t match_len;
	uint32_t dist_start[] = {
		0x0001, 0x0002, 0x0003, 0x0004, 0x0005, 0x0007, 0x0009, 0x000d,
		0x0011, 0x0019, 0x0021, 0x0031, 0x0041, 0x0061, 0x0081, 0x00c1,
		0x0101, 0x0181, 0x0201, 0x0301, 0x0401, 0x0601, 0x0801, 0x0c01,
		0x1001, 0x1801, 0x2001, 0x3001, 0x4001, 0x6001, 0x0000, 0x0000
	};

	while (next_in < end_in - ISAL_LOOK_AHEAD) {
		dist_code = match_lookup->lit_dist;
		dist_extra = match_lookup->dist_extra;
		dist = dist_start[dist_code] + dist_extra;
		len = match_lookup->lit_len;
		if (len >= 8 + LEN_OFFSET) {
			match_len =
			    compare258(next_in - dist + 8, next_in + 8, 250) + LEN_OFFSET + 8;

			while (match_len > match_lookup->lit_len
			       && match_len >= LEN_OFFSET + SHORTEST_MATCH) {
				write_deflate_icf(match_lookup, match_len, dist_code,
						  dist_extra);
				match_lookup++;
				next_in++;
				match_len--;
			}
		}

		match_lookup++;
		next_in++;
	}
}

/*
*************************************************************
 * Methods for generating one pass match lookup table
 ************************************************************
*/
void gen_icf_map_h1_base(struct isal_zstream *stream,
			 struct deflate_icf *matches_icf_lookup, uint64_t input_size)
{

	uint32_t dist, len, extra_bits;
	uint8_t *next_in = stream->next_in, *end_in = stream->next_in + input_size;
	uint8_t *file_start = stream->next_in - stream->total_in;
	uint32_t hash;
	uint64_t next_bytes, match_bytes;
	uint64_t match;
	struct level_buf *level_buf = (struct level_buf *)stream->level_buf;
	uint16_t *hash_table = level_buf->hash_map.hash_table;

	if (input_size < ISAL_LOOK_AHEAD)
		return;

	matches_icf_lookup->lit_len = *next_in;
	matches_icf_lookup->lit_dist = 0x1e;
	matches_icf_lookup->dist_extra = 0;

	hash = compute_hash(*(uint32_t *) next_in) & HASH_MAP_HASH_MASK;
	hash_table[hash] = (uint64_t) (next_in - file_start);

	next_in++;
	matches_icf_lookup++;

	while (next_in < end_in - ISAL_LOOK_AHEAD) {
		hash = compute_hash(*(uint32_t *) next_in) & HASH_MAP_HASH_MASK;
		dist = (next_in - file_start - hash_table[hash]);
		dist = ((dist - 1) & (IGZIP_HIST_SIZE - 1)) + 1;
		hash_table[hash] = (uint64_t) (next_in - file_start);

		match_bytes = *(uint64_t *) (next_in - dist);
		next_bytes = *(uint64_t *) next_in;
		match = next_bytes ^ match_bytes;

		len = tzbytecnt(match);

		if (len >= SHORTEST_MATCH) {
			len += LEN_OFFSET;
			get_dist_icf_code(dist, &dist, &extra_bits);
			write_deflate_icf(matches_icf_lookup, len, dist, extra_bits);
		} else {
			write_deflate_icf(matches_icf_lookup, *next_in, 0x1e, 0);
		}

		next_in++;
		matches_icf_lookup++;
	}
}

/*
*************************************************************
 * One pass methods for parsing provided match lookup table
 ************************************************************
*/
static struct deflate_icf *compress_icf_map_g(struct isal_zstream *stream,
					      struct deflate_icf *matches_next,
					      struct deflate_icf *matches_end)
{
	uint32_t lit_len, lit_len2, dist;
	uint64_t code;
	struct isal_zstate *state = &stream->internal_state;
	struct level_buf *level_buf = (struct level_buf *)stream->level_buf;
	struct deflate_icf *matches_start = matches_next;
	struct deflate_icf *icf_buf_end =
	    level_buf->icf_buf_next +
	    level_buf->icf_buf_avail_out / sizeof(struct deflate_icf);

	while (matches_next < matches_end - 1 && level_buf->icf_buf_next < icf_buf_end - 1) {
		code = *(uint64_t *) matches_next;
		lit_len = code & LIT_LEN_MASK;
		lit_len2 = (code >> ICF_CODE_LEN) & LIT_LEN_MASK;
		level_buf->hist.ll_hist[lit_len]++;

		if (lit_len >= LEN_START) {
			*(uint32_t *) level_buf->icf_buf_next = code;
			level_buf->icf_buf_next++;

			dist = (code >> ICF_DIST_OFFSET) & DIST_LIT_MASK;
			level_buf->hist.d_hist[dist]++;
			lit_len -= LEN_OFFSET;
			matches_next += lit_len;

		} else if (lit_len2 >= LEN_START) {
			*(uint64_t *) level_buf->icf_buf_next = code;
			level_buf->icf_buf_next += 2;

			level_buf->hist.ll_hist[lit_len2]++;

			dist = (code >> (ICF_CODE_LEN + ICF_DIST_OFFSET)) & DIST_LIT_MASK;
			level_buf->hist.d_hist[dist]++;
			lit_len2 -= LEN_OFFSET - 1;
			matches_next += lit_len2;

		} else {
			code = ((lit_len2 + LIT_START) << ICF_DIST_OFFSET) | lit_len;
			*(uint32_t *) level_buf->icf_buf_next = code;
			level_buf->icf_buf_next++;

			level_buf->hist.ll_hist[lit_len2]++;

			matches_next += 2;
		}
	}

	while (matches_next < matches_end && level_buf->icf_buf_next < icf_buf_end) {
		code = *(uint32_t *) matches_next;
		lit_len = code & LIT_LEN_MASK;
		*(uint32_t *) level_buf->icf_buf_next = code;
		level_buf->icf_buf_next++;

		level_buf->hist.ll_hist[lit_len]++;
		if (lit_len >= LEN_START) {
			dist = (code >> 10) & 0x1ff;
			level_buf->hist.d_hist[dist]++;
			lit_len -= LEN_OFFSET;
			matches_next += lit_len;
		} else {
			matches_next++;
		}
	}

	level_buf->icf_buf_avail_out =
	    (icf_buf_end - level_buf->icf_buf_next) * sizeof(struct deflate_icf);

	state->block_end += matches_next - matches_start;
	if (matches_next > matches_end && matches_start < matches_end) {
		stream->next_in += matches_next - matches_end;
		stream->avail_in -= matches_next - matches_end;
		stream->total_in += matches_next - matches_end;
	}

	return matches_next;

}

/*
*************************************************************
 * Compression functions combining different methods
 ************************************************************
*/
static inline void icf_body_next_state(struct isal_zstream *stream)
{
	struct level_buf *level_buf = (struct level_buf *)stream->level_buf;
	struct isal_zstate *state = &stream->internal_state;

	if (level_buf->icf_buf_avail_out <= 0)
		state->state = ZSTATE_CREATE_HDR;

	else if (stream->avail_in <= ISAL_LOOK_AHEAD
		 && (stream->end_of_stream || stream->flush != NO_FLUSH))
		state->state = ZSTATE_FLUSH_READ_BUFFER;
}

void icf_body_hash1_fillgreedy_lazy(struct isal_zstream *stream)
{
	struct deflate_icf *matches_icf, *matches_next_icf, *matches_end_icf;
	struct deflate_icf *matches_icf_lookup;
	struct level_buf *level_buf = (struct level_buf *)stream->level_buf;
	uint32_t input_size;

	matches_icf = level_buf->hash_map.matches;
	matches_icf_lookup = matches_icf;
	matches_next_icf = level_buf->hash_map.matches_next;
	matches_end_icf = level_buf->hash_map.matches_end;

	matches_next_icf = compress_icf_map_g(stream, matches_next_icf, matches_end_icf);

	while (matches_next_icf >= matches_end_icf) {
		input_size = MATCH_BUF_SIZE;
		input_size = (input_size > stream->avail_in) ? stream->avail_in : input_size;

		if (input_size <= ISAL_LOOK_AHEAD)
			break;

		gen_icf_map_h1_base(stream, matches_icf_lookup, input_size);

		set_long_icf_fg(stream->next_in, stream->next_in + input_size,
				matches_icf_lookup, level_buf);

		stream->next_in += input_size - ISAL_LOOK_AHEAD;
		stream->avail_in -= input_size - ISAL_LOOK_AHEAD;
		stream->total_in += input_size - ISAL_LOOK_AHEAD;

		matches_end_icf = matches_icf + input_size - ISAL_LOOK_AHEAD;
		matches_next_icf = compress_icf_map_g(stream, matches_icf, matches_end_icf);
	}

	level_buf->hash_map.matches_next = matches_next_icf;
	level_buf->hash_map.matches_end = matches_end_icf;

	icf_body_next_state(stream);
}

void icf_body_lazyhash1_fillgreedy_greedy(struct isal_zstream *stream)
{
	struct deflate_icf *matches_icf, *matches_next_icf, *matches_end_icf;
	struct deflate_icf *matches_icf_lookup;
	struct level_buf *level_buf = (struct level_buf *)stream->level_buf;
	uint32_t input_size;

	matches_icf = level_buf->hash_map.matches;
	matches_icf_lookup = matches_icf;
	matches_next_icf = level_buf->hash_map.matches_next;
	matches_end_icf = level_buf->hash_map.matches_end;

	matches_next_icf = compress_icf_map_g(stream, matches_next_icf, matches_end_icf);

	while (matches_next_icf >= matches_end_icf) {
		input_size = MATCH_BUF_SIZE;
		input_size = (input_size > stream->avail_in) ? stream->avail_in : input_size;

		if (input_size <= ISAL_LOOK_AHEAD)
			break;

		gen_icf_map_lh1(stream, matches_icf_lookup, input_size);

		set_long_icf_fg(stream->next_in, stream->next_in + input_size,
				matches_icf_lookup, level_buf);

		stream->next_in += input_size - ISAL_LOOK_AHEAD;
		stream->avail_in -= input_size - ISAL_LOOK_AHEAD;
		stream->total_in += input_size - ISAL_LOOK_AHEAD;

		matches_end_icf = matches_icf + input_size - ISAL_LOOK_AHEAD;
		matches_next_icf = compress_icf_map_g(stream, matches_icf, matches_end_icf);
	}

	level_buf->hash_map.matches_next = matches_next_icf;
	level_buf->hash_map.matches_end = matches_end_icf;

	icf_body_next_state(stream);
}

void isal_deflate_icf_body(struct isal_zstream *stream)
{
	switch (stream->level) {
	case 3:
		isal_deflate_icf_body_lvl3(stream);
		break;
	case 2:
		isal_deflate_icf_body_lvl2(stream);
		break;
	case 1:
	default:
		isal_deflate_icf_body_lvl1(stream);
	}
}
