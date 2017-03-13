#include <stdint.h>
#include "igzip_lib.h"
#include "huffman.h"
#include "huff_codes.h"
#include "encode_df.h"
#include "igzip_level_buf_structs.h"

static inline void write_deflate_icf(struct deflate_icf *icf, uint32_t lit_len,
				     uint32_t lit_dist, uint32_t extra_bits)
{
	icf->lit_len = lit_len;
	icf->lit_dist = lit_dist;
	icf->dist_extra = extra_bits;
}

static inline void update_state(struct isal_zstream *stream, uint8_t * start_in,
				uint8_t * next_in, uint8_t * end_in,
				struct deflate_icf *start_out, struct deflate_icf *next_out,
				struct deflate_icf *end_out)
{
	stream->next_in = next_in;
	stream->total_in += next_in - start_in;
	stream->avail_in = end_in - next_in;

	((struct level_2_buf *)stream->level_buf)->icf_buf_next = next_out;
	((struct level_2_buf *)stream->level_buf)->icf_buf_avail_out = end_out - next_out;
}

void isal_deflate_icf_body_base(struct isal_zstream *stream)
{
	uint32_t literal, hash;
	uint8_t *start_in, *next_in, *end_in, *end, *next_hash;
	struct deflate_icf *start_out, *next_out, *end_out;
	uint16_t match_length;
	uint32_t dist;
	uint32_t code, code2, extra_bits;
	struct isal_zstate *state = &stream->internal_state;
	uint16_t *last_seen = state->head;

	if (stream->avail_in == 0) {
		if (stream->end_of_stream || stream->flush != NO_FLUSH)
			state->state = ZSTATE_FLUSH_READ_BUFFER;
		return;
	}

	start_in = stream->next_in;
	end_in = start_in + stream->avail_in;
	next_in = start_in;

	start_out = ((struct level_2_buf *)stream->level_buf)->icf_buf_next;
	end_out =
	    start_out + ((struct level_2_buf *)stream->level_buf)->icf_buf_avail_out /
	    sizeof(struct deflate_icf);
	next_out = start_out;

	while (next_in + ISAL_LOOK_AHEAD < end_in) {

		if (next_out >= end_out) {
			state->state = ZSTATE_CREATE_HDR;
			update_state(stream, start_in, next_in, end_in, start_out, next_out,
				     end_out);
			return;
		}

		literal = *(uint32_t *) next_in;
		hash = compute_hash(literal) & HASH_MASK;
		dist = (next_in - state->file_start - last_seen[hash]) & 0xFFFF;
		last_seen[hash] = (uint64_t) (next_in - state->file_start);

		/* The -1 are to handle the case when dist = 0 */
		if (dist - 1 < IGZIP_HIST_SIZE - 1) {
			assert(dist != 0);

			match_length = compare258(next_in - dist, next_in, 258);

			if (match_length >= SHORTEST_MATCH) {
				next_hash = next_in;
#ifdef ISAL_LIMIT_HASH_UPDATE
				end = next_hash + 3;
#else
				end = next_hash + match_length;
#endif
				next_hash++;

				for (; next_hash < end; next_hash++) {
					literal = *(uint32_t *) next_hash;
					hash = compute_hash(literal) & HASH_MASK;
					last_seen[hash] =
					    (uint64_t) (next_hash - state->file_start);
				}

				get_len_icf_code(match_length, &code);
				get_dist_icf_code(dist, &code2, &extra_bits);

				state->hist.ll_hist[code]++;
				state->hist.d_hist[code2]++;

				write_deflate_icf(next_out, code, code2, extra_bits);
				next_out++;
				next_in += match_length;

				continue;
			}
		}

		get_lit_icf_code(literal & 0xFF, &code);
		state->hist.ll_hist[code]++;
		write_deflate_icf(next_out, code, NULL_DIST_SYM, 0);
		next_out++;
		next_in++;
	}

	update_state(stream, start_in, next_in, end_in, start_out, next_out, end_out);

	assert(stream->avail_in <= ISAL_LOOK_AHEAD);
	if (stream->end_of_stream || stream->flush != NO_FLUSH)
		state->state = ZSTATE_FLUSH_READ_BUFFER;

	return;

}

void isal_deflate_icf_finish_base(struct isal_zstream *stream)
{
	uint32_t literal = 0, hash;
	uint8_t *start_in, *next_in, *end_in, *end, *next_hash;
	struct deflate_icf *start_out, *next_out, *end_out;
	uint16_t match_length;
	uint32_t dist;
	uint32_t code, code2, extra_bits;
	struct isal_zstate *state = &stream->internal_state;
	uint16_t *last_seen = state->head;

	start_in = stream->next_in;
	end_in = start_in + stream->avail_in;
	next_in = start_in;

	start_out = ((struct level_2_buf *)stream->level_buf)->icf_buf_next;
	end_out = start_out + ((struct level_2_buf *)stream->level_buf)->icf_buf_avail_out /
	    sizeof(struct deflate_icf);
	next_out = start_out;

	while (next_in + 3 < end_in) {
		if (next_out >= end_out) {
			state->state = ZSTATE_CREATE_HDR;
			update_state(stream, start_in, next_in, end_in, start_out, next_out,
				     end_out);
			return;
		}

		literal = *(uint32_t *) next_in;
		hash = compute_hash(literal) & HASH_MASK;
		dist = (next_in - state->file_start - last_seen[hash]) & 0xFFFF;
		last_seen[hash] = (uint64_t) (next_in - state->file_start);

		if (dist - 1 < IGZIP_HIST_SIZE - 1) {	/* The -1 are to handle the case when dist = 0 */
			match_length = compare258(next_in - dist, next_in, end_in - next_in);

			if (match_length >= SHORTEST_MATCH) {
				next_hash = next_in;
#ifdef ISAL_LIMIT_HASH_UPDATE
				end = next_hash + 3;
#else
				end = next_hash + match_length;
#endif
				next_hash++;

				for (; next_hash < end - 3; next_hash++) {
					literal = *(uint32_t *) next_hash;
					hash = compute_hash(literal) & HASH_MASK;
					last_seen[hash] =
					    (uint64_t) (next_hash - state->file_start);
				}

				get_len_icf_code(match_length, &code);
				get_dist_icf_code(dist, &code2, &extra_bits);

				state->hist.ll_hist[code]++;
				state->hist.d_hist[code2]++;

				write_deflate_icf(next_out, code, code2, extra_bits);

				next_out++;
				next_in += match_length;

				continue;
			}
		}

		get_lit_icf_code(literal & 0xFF, &code);
		state->hist.ll_hist[code]++;
		write_deflate_icf(next_out, code, NULL_DIST_SYM, 0);
		next_out++;
		next_in++;

	}

	while (next_in < end_in) {
		if (next_out >= end_out) {
			state->state = ZSTATE_CREATE_HDR;
			update_state(stream, start_in, next_in, end_in, start_out, next_out,
				     end_out);
			return;
		}

		literal = *next_in;
		get_lit_icf_code(literal & 0xFF, &code);
		state->hist.ll_hist[code]++;
		write_deflate_icf(next_out, code, NULL_DIST_SYM, 0);
		next_out++;
		next_in++;

	}

	if (next_in == end_in) {
		if (stream->end_of_stream || stream->flush != NO_FLUSH)
			state->state = ZSTATE_CREATE_HDR;
	}

	update_state(stream, start_in, next_in, end_in, start_out, next_out, end_out);

	return;
}
