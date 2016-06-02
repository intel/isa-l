#include <stdint.h>
#include "igzip_lib.h"
#include "huffman.h"
#include "huff_codes.h"
#include "bitbuf2.h"

extern const struct isal_hufftables hufftables_default;

void isal_deflate_init_base(struct isal_zstream *stream)
{
	struct isal_zstate *state = &stream->internal_state;
	int i;

	uint32_t *crc = state->crc;

	stream->total_in = 0;
	stream->total_out = 0;
	stream->hufftables = (struct isal_hufftables *)&hufftables_default;
	stream->flush = 0;
	state->b_bytes_valid = 0;
	state->b_bytes_processed = 0;
	state->has_eob = 0;
	state->has_eob_hdr = 0;
	state->left_over = 0;
	state->last_flush = 0;
	state->has_gzip_hdr = 0;
	state->state = ZSTATE_NEW_HDR;
	state->count = 0;

	state->tmp_out_start = 0;
	state->tmp_out_end = 0;

	state->file_start = state->buffer;

	init(&state->bitbuf);

	*crc = ~0;

	for (i = 0; i < HASH_SIZE; i++)
		state->head[i] = (uint16_t) - (IGZIP_D + 1);
	return;
}

uint32_t get_crc_base(uint32_t * crc)
{
	return ~*crc;
}

static inline void update_state(struct isal_zstream *stream, struct isal_zstate *state,
				uint8_t * start_in)
{
	uint32_t bytes_written;

	stream->total_in += stream->next_in - start_in;

	bytes_written = buffer_used(&state->bitbuf);
	stream->total_out += bytes_written;
	stream->next_out += bytes_written;
	stream->avail_out -= bytes_written;

}

void isal_deflate_body_base(struct isal_zstream *stream)
{
	uint32_t literal, hash;
	uint8_t *start_in, *next_in, *end_in, *end, *next_hash;
	uint16_t match_length;
	uint32_t dist, bytes_to_buffer, offset;
	uint64_t code, code_len, code2, code_len2;
	struct isal_zstate *state = &stream->internal_state;
	uint16_t *last_seen = state->head;
	uint32_t *crc = state->crc;

	if (stream->avail_in == 0) {
		if (stream->end_of_stream || stream->flush != NO_FLUSH)
			state->state = ZSTATE_FLUSH_READ_BUFFER;
		return;
	}

	set_buf(&state->bitbuf, stream->next_out, stream->avail_out);
	start_in = stream->next_in;

	while (stream->avail_in != 0) {
		bytes_to_buffer =
		    IGZIP_D + IGZIP_LA - (state->b_bytes_valid - state->b_bytes_processed);

		if (bytes_to_buffer > IGZIP_D)
			bytes_to_buffer = IGZIP_D;

		if (stream->avail_in < IGZIP_D)
			bytes_to_buffer = stream->avail_in;

		if (bytes_to_buffer > BSIZE - state->b_bytes_valid) {
			if (state->b_bytes_valid - state->b_bytes_processed > IGZIP_LA) {
				/* There was an out buffer overflow last round,
				 * complete the processing of data */
				bytes_to_buffer = 0;

			} else {
				/* Not enough room in the buffer, shift the
				 * buffer down to make space for the new data */
				offset = state->b_bytes_processed - IGZIP_D;	// state->b_bytes_valid - (IGZIP_D + IGZIP_LA);
				memmove(state->buffer, state->buffer + offset,
					IGZIP_D + IGZIP_LA);

				state->b_bytes_processed -= offset;
				state->b_bytes_valid -= offset;
				state->file_start -= offset;

				stream->avail_in -= bytes_to_buffer;
				memcpy(state->buffer + state->b_bytes_valid, stream->next_in,
				       bytes_to_buffer);
				update_crc(crc, stream->next_in, bytes_to_buffer);
				stream->next_in += bytes_to_buffer;
			}
		} else {
			/* There is enough space in the buffer, copy in the new data */
			stream->avail_in -= bytes_to_buffer;
			memcpy(state->buffer + state->b_bytes_valid, stream->next_in,
			       bytes_to_buffer);
			update_crc(crc, stream->next_in, bytes_to_buffer);
			stream->next_in += bytes_to_buffer;
		}

		state->b_bytes_valid += bytes_to_buffer;

		end_in = state->buffer + state->b_bytes_valid - IGZIP_LA;

		next_in = state->b_bytes_processed + state->buffer;

		while (next_in < end_in) {

			if (is_full(&state->bitbuf)) {
				state->b_bytes_processed = next_in - state->buffer;
				update_state(stream, state, start_in);
				return;
			}

			literal = *(uint32_t *) next_in;
			hash = compute_hash(literal) & HASH_MASK;
			dist = (next_in - state->file_start - last_seen[hash]) & 0xFFFF;
			last_seen[hash] = (uint64_t) (next_in - state->file_start);

			if (dist - 1 < IGZIP_D - 1) {	/* The -1 are to handle the case when dist = 0 */
				assert(next_in - dist >= state->buffer);
				assert(dist != 0);

				match_length = compare258(next_in - dist, next_in, 258);

				if (match_length >= SHORTEST_MATCH) {
					next_hash = next_in;
#ifdef LIMIT_HASH_UPDATE
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

					get_len_code(stream->hufftables, match_length, &code,
						     &code_len);
					get_dist_code(stream->hufftables, dist, &code2,
						      &code_len2);

					code |= code2 << code_len;
					code_len += code_len2;

					write_bits(&state->bitbuf, code, code_len);

					next_in += match_length;

					continue;
				}
			}

			get_lit_code(stream->hufftables, literal & 0xFF, &code, &code_len);
			write_bits(&state->bitbuf, code, code_len);
			next_in++;
		}

		state->b_bytes_processed = next_in - state->buffer;

	}

	update_state(stream, state, start_in);

	if (stream->avail_in == 0) {
		if (stream->end_of_stream || stream->flush != NO_FLUSH)
			state->state = ZSTATE_FLUSH_READ_BUFFER;
		return;
	}

	return;

}

void isal_deflate_finish_base(struct isal_zstream *stream)
{
	uint32_t literal = 0, hash;
	uint8_t *next_in, *end_in, *end, *next_hash;
	uint16_t match_length;
	uint32_t dist;
	uint64_t code, code_len, code2, code_len2;
	struct isal_zstate *state = &stream->internal_state;
	uint16_t *last_seen = state->head;

	set_buf(&state->bitbuf, stream->next_out, stream->avail_out);

	end_in = state->b_bytes_valid + (uint8_t *) state->buffer;

	next_in = state->b_bytes_processed + state->buffer;

	while (next_in < end_in) {

		if (is_full(&state->bitbuf)) {
			state->b_bytes_processed = next_in - state->buffer;
			update_state(stream, state, stream->next_in);
			return;
		}

		literal = *(uint32_t *) next_in;
		hash = compute_hash(literal) & HASH_MASK;
		dist = (next_in - state->file_start - last_seen[hash]) & 0xFFFF;
		last_seen[hash] = (uint64_t) (next_in - state->file_start);

		if (dist - 1 < IGZIP_D - 1) {	/* The -1 are to handle the case when dist = 0 */
			assert(next_in - dist >= state->buffer);
			match_length = compare258(next_in - dist, next_in, end_in - next_in);

			if (match_length >= SHORTEST_MATCH) {
				next_hash = next_in;
#ifdef LIMIT_HASH_UPDATE
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

				get_len_code(stream->hufftables, match_length, &code,
					     &code_len);
				get_dist_code(stream->hufftables, dist, &code2, &code_len2);

				code |= code2 << code_len;
				code_len += code_len2;

				write_bits(&state->bitbuf, code, code_len);

				next_in += match_length;

				continue;
			}
		}

		get_lit_code(stream->hufftables, literal & 0xFF, &code, &code_len);
		write_bits(&state->bitbuf, code, code_len);
		next_in++;

	}

	state->b_bytes_processed = next_in - state->buffer;

	if (is_full(&state->bitbuf) || state->left_over > 0) {
		update_state(stream, state, stream->next_in);
		return;
	}

	get_lit_code(stream->hufftables, 256, &code, &code_len);
	write_bits(&state->bitbuf, code, code_len);
	state->has_eob = 1;

	update_state(stream, state, stream->next_in);

	if (stream->end_of_stream == 1)
		state->state = ZSTATE_TRL;
	else
		state->state = ZSTATE_SYNC_FLUSH;

	return;
}
