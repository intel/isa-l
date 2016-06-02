#include <stdint.h>
#include "igzip_lib.h"
#include "huffman.h"
#include "huff_codes.h"
#include "bitbuf2.h"

static inline void update_state(struct isal_zstream *stream, struct isal_zstate *state,
				uint8_t * end_in, uint8_t * start_in)
{
	uint32_t count;
	stream->avail_in = end_in - stream->next_in;
	stream->total_in += stream->next_in - start_in;
	count = buffer_used(&state->bitbuf);
	stream->next_out = buffer_ptr(&state->bitbuf);
	stream->avail_out -= count;
	stream->total_out += count;

}

void isal_deflate_body_stateless_base(struct isal_zstream *stream)
{
	uint32_t literal = 0, hash;
	uint8_t *start_in, *end_in, *end, *next_hash;
	uint16_t match_length;
	uint32_t dist;
	uint64_t code, code_len, code2, code_len2, i;
	struct isal_zstate *state = &stream->internal_state;
	uint16_t *last_seen = state->head;

	if (stream->avail_in == 0)
		return;

	set_buf(&state->bitbuf, stream->next_out, stream->avail_out);
	start_in = stream->next_in;
	end_in = stream->next_in + stream->avail_in;

	while (stream->next_in < end_in - 3) {
		if (is_full(&state->bitbuf)) {
			update_state(stream, state, end_in, start_in);
			return;
		}

		literal = *(uint32_t *) stream->next_in;
		hash = compute_hash(literal) & HASH_MASK;
		dist = (uint64_t) (stream->next_in - last_seen[hash]) & 0xFFFF;
		last_seen[hash] = (uint64_t) stream->next_in;

		if (dist - 1 < IGZIP_D - 1 && stream->next_in - dist >= start_in) {	/* The -1 are to handle the case when dist = 0 */
			match_length =
			    compare258(stream->next_in - dist, stream->next_in,
				       end_in - stream->next_in);

			if (match_length >= SHORTEST_MATCH) {
				next_hash = stream->next_in;
#ifdef LIMIT_HASH_UPDATE
				end = next_hash + 3;
#else
				end = next_hash + match_length;
#endif
				if (end > end_in - 3)
					end = end_in - 3;
				next_hash++;
				for (; next_hash < end; next_hash++) {
					literal = *(uint32_t *) next_hash;
					hash = compute_hash(literal) & HASH_MASK;
					last_seen[hash] = (uint64_t) next_hash;
				}

				get_len_code(stream->hufftables, match_length, &code,
					     &code_len);
				get_dist_code(stream->hufftables, dist, &code2, &code_len2);

				code |= code2 << code_len;
				code_len += code_len2;

				write_bits(&state->bitbuf, code, code_len);

				stream->next_in += match_length;

				continue;
			}
		}

		get_lit_code(stream->hufftables, literal & 0xFF, &code, &code_len);
		write_bits(&state->bitbuf, code, code_len);
		stream->next_in++;
	}

	if (is_full(&state->bitbuf)) {
		update_state(stream, state, end_in, start_in);
		return;
	}

	literal = *(uint32_t *) (end_in - 4);

	for (i = 4; i > end_in - stream->next_in; i--)
		literal = literal >> 8;

	hash = compute_hash(literal) & HASH_MASK;
	dist = (uint64_t) (stream->next_in - last_seen[hash]) & 0xFFFF;

	if (dist - 1 < IGZIP_D - 1 && stream->next_in - dist >= start_in) {
		match_length =
		    compare258(stream->next_in - dist, stream->next_in,
			       end_in - stream->next_in);
		if (match_length >= SHORTEST_MATCH) {
			get_len_code(stream->hufftables, match_length, &code, &code_len);
			get_dist_code(stream->hufftables, dist, &code2, &code_len2);
			code |= code2 << code_len;
			code_len += code_len2;
			write_bits(&state->bitbuf, code, code_len);
			stream->next_in += 3;

			if (is_full(&state->bitbuf)) {
				update_state(stream, state, end_in, start_in);
				return;
			}

			get_lit_code(stream->hufftables, 256, &code, &code_len);
			write_bits(&state->bitbuf, code, code_len);

			if (is_full(&state->bitbuf)) {
				update_state(stream, state, end_in, start_in);
				return;
			}

			state->has_eob = 1;
			update_state(stream, state, end_in, start_in);
			return;
		}
	}

	while (stream->next_in < end_in) {
		get_lit_code(stream->hufftables, literal & 0xFF, &code, &code_len);
		write_bits(&state->bitbuf, code, code_len);
		stream->next_in++;

		if (is_full(&state->bitbuf)) {
			update_state(stream, state, end_in, start_in);
			return;
		}
		literal >>= 8;
	}

	get_lit_code(stream->hufftables, 256, &code, &code_len);
	write_bits(&state->bitbuf, code, code_len);

	state->has_eob = 1;
	update_state(stream, state, end_in, start_in);
	return;
}
