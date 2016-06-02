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

#ifndef _IGZIP_H
#define _IGZIP_H

/**
 * @file igzip_lib.h
 *
 * @brief This file defines the igzip compression interface, a high performance
 * deflate compression interface for storage applications.
 *
 * Deflate is a widely used compression standard that can be used standalone, it
 * also forms the basis of gzip and zlib compression formats. Igzip supports the
 * following flush features:
 *
 * - No Flush: The default method where no flush is performed.
 *
 * - Sync flush: whereby isal_deflate() finishes the current deflate block at
 *   the end of each input buffer. The deflate block is byte aligned by
 *   appending an empty stored block.
 *
 * - Full flush: whereby isal_deflate() finishes and aligns the deflate block as
 *   in sync flush but also ensures that subsequent block's history does not
 *   look back beyond this point and new blocks are fully independent.
 *
 * Igzip's default configuration is:
 *
 * - 8K window size
 *
 * This option can be overridden to enable:
 *
 * - 32K window size, by adding \#define LARGE_WINDOW 1 in igzip_lib.h and
 *   \%define LARGE_WINDOW in options.asm, or via the command line with
 *   @verbatim gmake D="-D LARGE_WINDOW" @endverbatim on Linux and FreeBSD, or
 *   with @verbatim nmake -f Makefile.nmake D="-D LARGE_WINDOW" @endverbatim on
 *   Windows.
 *
 * KNOWN ISSUES:
 * - If building the code on Windows with the 32K window enabled, the
 *   /LARGEADDRESSAWARE:NO link option must be added.
 * - The 32K window isn't supported when used in a shared library.
 *
 */
#include <stdint.h>
#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

// Options:dir
// m - reschedule mem reads
// e b - bitbuff style
// t s x - compare style
// h - limit hash updates
// l - use longer huffman table
// f - fix cache read

#if defined(LARGE_WINDOW)
# define HIST_SIZE 32
#else
# define HIST_SIZE 8
#endif

/* bit buffer types
 * BITBUF8: (e) Always write 8 bytes of data
 * BITBUFB: (b) Always write data
 */
#if !(defined(USE_BITBUFB) || defined(USE_BITBUF8) || defined(USE_BITBUF_ELSE))
# define USE_BITBUFB
#endif

/* compare types
 * 1: ( ) original
 * 2: (t) with CMOV
 * 3: (s) with sttni
 * 4: (x) with xmm / pmovbmsk
 * 5: (y) with ymm / pmovbmsk (32-bytes at a time)
 */
# define LIMIT_HASH_UPDATE

/* (l) longer huffman table */
#define LONGER_HUFFTABLE

/* (f) fix cache read problem */
#define FIX_CACHE_READ

#if (HIST_SIZE > 8)
# undef LONGER_HUFFTABLE
#endif

#define IGZIP_K  1024
#define IGZIP_D  (HIST_SIZE * IGZIP_K)	/* Amount of history */
#define IGZIP_LA (17 * 16)		/* Max look-ahead, rounded up to 32 byte boundary */
#define BSIZE  (2*IGZIP_D + IGZIP_LA)	/* Nominal buffer size */

#define HASH_SIZE  IGZIP_D
#define HASH_MASK  (HASH_SIZE - 1)

#define SHORTEST_MATCH  3

#define IGZIP_MAX_DEF_HDR_SIZE 328

#ifdef LONGER_HUFFTABLE
enum {DIST_TABLE_SIZE = 8*1024};

/* DECODE_OFFSET is dist code index corresponding to DIST_TABLE_SIZE + 1 */
enum { DECODE_OFFSET = 26 };
#else
enum {DIST_TABLE_SIZE = 1024};
/* DECODE_OFFSET is dist code index corresponding to DIST_TABLE_SIZE + 1 */
enum { DECODE_OFFSET = 20 };
#endif
enum {LEN_TABLE_SIZE = 256};
enum {LIT_TABLE_SIZE = 257};

#define IGZIP_LIT_LEN 286
#define IGZIP_DIST_LEN 30

/* Flush Flags */
#define NO_FLUSH	0	/* Default */
#define SYNC_FLUSH	1
#define FULL_FLUSH	2
#define FINISH_FLUSH	0	/* Deprecated */

/* Return values */
#define COMP_OK 0
#define INVALID_FLUSH -7
#define INVALID_PARAM -8
#define STATELESS_OVERFLOW -1
#define DEFLATE_HDR_LEN 3
/**
 *  @enum isal_zstate
 *  @brief Compression State please note ZSTATE_TRL only applies for GZIP compression
 */


/* When the state is set to ZSTATE_NEW_HDR or TMP_ZSTATE_NEW_HEADER, the
 * hufftable being used for compression may be swapped
 */
enum isal_zstate_state {
	ZSTATE_NEW_HDR,  //!< Header to be written
	ZSTATE_HDR,	//!< Header state
	ZSTATE_BODY,	//!< Body state
	ZSTATE_FLUSH_READ_BUFFER, //!< Flush buffer
	ZSTATE_SYNC_FLUSH, //!< Write sync flush block
	ZSTATE_FLUSH_WRITE_BUFFER, //!< Flush bitbuf
	ZSTATE_TRL,	//!< Trailer state
	ZSTATE_END,	//!< End state
	ZSTATE_TMP_NEW_HDR, //!< Temporary Header to be written
	ZSTATE_TMP_HDR,	//!< Temporary Header state
	ZSTATE_TMP_BODY,	//!< Temporary Body state
	ZSTATE_TMP_FLUSH_READ_BUFFER, //!< Flush buffer
	ZSTATE_TMP_SYNC_FLUSH, //!< Write sync flush block
	ZSTATE_TMP_FLUSH_WRITE_BUFFER, //!< Flush bitbuf
	ZSTATE_TMP_TRL,	//!< Temporary Trailer state
	ZSTATE_TMP_END	//!< Temporary End state
};

/* Offset used to switch between TMP states and non-tmp states */
#define TMP_OFFSET_SIZE ZSTATE_TMP_HDR - ZSTATE_HDR

struct isal_huff_histogram {
	uint64_t lit_len_histogram[IGZIP_LIT_LEN];
	uint64_t dist_histogram[IGZIP_DIST_LEN];
};

/** @brief Holds Bit Buffer information*/
struct BitBuf2 {
	uint64_t m_bits;	//!< bits in the bit buffer
	uint32_t m_bit_count;	//!< number of valid bits in the bit buffer
	uint8_t *m_out_buf;	//!< current index of buffer to write to
	uint8_t *m_out_end;	//!< end of buffer to write to
	uint8_t *m_out_start;	//!< start of buffer to write to
};

/* Variable prefixes:
 * b_ : Measured wrt the start of the buffer
 * f_ : Measured wrt the start of the file (aka file_start)
 */

/** @brief Holds the internal state information for input and output compression streams*/
struct isal_zstate {
	uint32_t b_bytes_valid;	//!< number of bytes of valid data in buffer
	uint32_t b_bytes_processed;	//!< keeps track of the number of bytes processed in isal_zstate.buffer
	uint8_t *file_start;	//!< pointer to where file would logically start
	DECLARE_ALIGNED(uint32_t crc[16], 16);	//!< actually 4 128-bit integers
	struct BitBuf2 bitbuf;	//!< Bit Buffer
	enum isal_zstate_state state;	//!< Current state in processing the data stream
	uint32_t count;	//!< used for partial header/trailer writes
	uint8_t tmp_out_buff[16];	//!< temporary array
	uint32_t tmp_out_start;	//!< temporary variable
	uint32_t tmp_out_end;	//!< temporary variable
	uint32_t last_flush;    //!< keeps track of last submitted flush
	uint32_t has_gzip_hdr;	//!< keeps track of if the gzip header has been written.
	uint32_t has_eob;	//!< keeps track of eob on the last deflate block
	uint32_t has_eob_hdr;	//!< keeps track of eob hdr (with BFINAL set)
	uint32_t left_over;	//!< keeps track of overflow bytes



	DECLARE_ALIGNED(uint8_t buffer[BSIZE + 16], 32);	//!< Internal buffer

	DECLARE_ALIGNED(uint16_t head[HASH_SIZE], 16);	//!< Hash array

};

/** @brief Holds the huffman tree used to huffman encode the input stream **/
struct isal_hufftables {

	uint8_t deflate_hdr[IGZIP_MAX_DEF_HDR_SIZE]; //!< deflate huffman tree header
	uint32_t deflate_hdr_count; //!< Number of whole bytes in deflate_huff_hdr
	uint32_t deflate_hdr_extra_bits; //!< Number of bits in the partial byte in header
	uint32_t dist_table[DIST_TABLE_SIZE]; //!< bits 4:0 are the code length, bits 31:5 are the code
	uint32_t len_table[LEN_TABLE_SIZE]; //!< bits 4:0 are the code length, bits 31:5 are the code
	uint16_t lit_table[LIT_TABLE_SIZE]; //!< literal code
	uint8_t lit_table_sizes[LIT_TABLE_SIZE]; //!< literal code length
	uint16_t dcodes[30 - DECODE_OFFSET]; //!< distance code
	uint8_t dcodes_sizes[30 - DECODE_OFFSET]; //!< distance code length

};

/** @brief Holds stream information*/
struct isal_zstream {
	uint8_t *next_in;	//!< Next input byte
	uint32_t avail_in;	//!< number of bytes available at next_in
	uint32_t total_in;	//!< total number of bytes read so far

	uint8_t *next_out;	//!< Next output byte
	uint32_t avail_out;	//!< number of bytes available at next_out
	uint32_t total_out;	//!< total number of bytes written so far

	struct isal_hufftables *hufftables; //!< Huffman encoding used when compressing
	uint32_t end_of_stream;	//!< non-zero if this is the last input buffer
	uint32_t flush;	//!< Flush type can be NO_FLUSH or SYNC_FLUSH

	struct isal_zstate internal_state;	//!< Internal state for this stream
};


/**
 * @brief Updates histograms to include the symbols found in the input
 * stream. Since this function only updates the histograms, it can be called on
 * multiple streams to get a histogram better representing the desired data
 * set. When first using histogram it must be initialized by zeroing the
 * structure.
 *
 * @param in_stream: Input stream of data.
 * @param length: The length of start_stream.
 * @param histogram: The returned histogram of lit/len/dist symbols.
 */
void isal_update_histogram(uint8_t * in_stream, int length, struct isal_huff_histogram * histogram);


/**
 * @brief Creates a custom huffman code for the given histograms in which
 *  every literal and repeat length is assigned a code and all possible lookback
 *  distances are assigned a code.
 *
 * @param hufftables: the output structure containing the huffman code
 * @param lit_histogram: histogram containing frequency of literal symbols and
 * repeat lengths
 * @param dist_histogram: histogram containing frequency of of lookback distances
 * @returns Returns a non zero value if an invalid huffman code was created.
 */
int isal_create_hufftables(struct isal_hufftables * hufftables,
			struct isal_huff_histogram * histogram);

/**
 * @brief Creates a custom huffman code for the given histograms like
 * isal_create_hufftables() except literals with 0 frequency in the histogram
 * are not assigned a code
 *
 * @param hufftables: the output structure containing the huffman code
 * @param lit_histogram: histogram containing frequency of literal symbols and
 * repeat lengths
 * @param dist_histogram: histogram containing frequency of of lookback distances
 * @returns Returns a non zero value if an invalid huffman code was created.
 */
int isal_create_hufftables_subset(struct isal_hufftables * hufftables,
				struct isal_huff_histogram * histogram);

/**
 * @brief Initialize compression stream data structure
 *
 * @param stream Structure holding state information on the compression streams.
 * @returns none
 */
void isal_deflate_init(struct isal_zstream *stream);


/**
 * @brief Fast data (deflate) compression for storage applications.
 *
 * On entry to isal_deflate(), next_in points to an input buffer and avail_in
 * indicates the length of that buffer. Similarly next_out points to an empty
 * output buffer and avail_out indicates the size of that buffer.
 *
 * The fields total_in and total_out start at 0 and are updated by
 * isal_deflate(). These reflect the total number of bytes read or written so far.
 *
 * The call to isal_deflate() will take data from the input buffer (updating
 * next_in, avail_in and write a compressed stream to the output buffer
 * (updating next_out and avail_out). The function returns when either the input
 * buffer is empty or the output buffer is full.
 *
 * When the last input buffer is passed in, signaled by setting the
 * end_of_stream, the routine will complete compression at the end of the input
 * buffer, as long as the output buffer is big enough.
 *
 * The equivalent of the zlib FLUSH_SYNC operation is currently supported.
 * Flush types can be NO_FLUSH or SYNC_FLUSH. Default flush type is NO_FLUSH.
 * If SYNC_FLUSH is selected each input buffer is compressed and byte aligned
 * with a type 0 block appended to the end. Switching between NO_FLUSH and
 * SYNC_FLUSH is supported to select after which input buffer a SYNC_FLUSH is
 * performed.
 *
 * @param  stream Structure holding state information on the compression streams.
 * @return COMP_OK (if everything is ok),
 *         INVALID_FLUSH (if an invalid FLUSH is selected),
 */
int isal_deflate(struct isal_zstream *stream);


/**
 * @brief Fast data (deflate) stateless compression for storage applications.
 *
 * Stateless (one shot) compression routine with a similar interface to
 * isal_deflate() but operates on entire input buffer at one time. Parameter
 * avail_out must be large enough to fit the entire compressed output. Max
 * expansion is limited to the input size plus the header size of a stored/raw
 * block.
 *
 * @param  stream Structure holding state information on the compression streams.
 * @return COMP_OK (if everything is ok),
 *         STATELESS_OVERFLOW (if output buffer will not fit output).
 */
int isal_deflate_stateless(struct isal_zstream *stream);


#ifdef __cplusplus
}
#endif
#endif	/* ifndef _IGZIP_H */
