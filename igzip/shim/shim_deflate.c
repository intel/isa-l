/**********************************************************************
  Copyright(c) 2025 Intel Corporation All rights reserved.

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

#include "shim_deflate.h"
#include "igzip_lib.h"
#include "crc.h"

#include <stdio.h>
#include <stdlib.h>

int
deflateInit2_(z_streamp strm, int level, int method, int windowBits, int memLevel, int strategy)
{
        if (!strm) {
                fprintf(stderr, "Error: z_streamp is NULL\n");
                return -1;
        }
#ifdef DEBUG
        fprintf(stderr,
                "\nInitializing deflate with level: %d, method: %d, windowBits: %d, memLevel: %d, "
                "strategy: %d\n",
                level, method, windowBits, memLevel, strategy);
#endif

        struct isal_zstream *isal_strm =
                (struct isal_zstream *) malloc(sizeof(struct isal_zstream));
        if (!isal_strm) {
                fprintf(stderr, "Error: Memory allocation for isal_zstream failed\n");
                return -1;
        }

        /* Setup ISA-L compression context */
        isal_deflate_init(isal_strm);

        isal_strm->end_of_stream = 0;
        isal_strm->flush = NO_FLUSH;
        strm->total_out = 0;
        strm->total_in = 0;

        // Map Zlib levels to ISA-L levels
        if (level >= 1 && level <= 2) {
                isal_strm->level = 1;
                isal_strm->level_buf = malloc(ISAL_DEF_LVL1_DEFAULT);
                isal_strm->level_buf_size = ISAL_DEF_LVL1_DEFAULT;
        } else if ((level >= 3 && level <= 6) || level == -1) {
                isal_strm->level = 2;
                isal_strm->level_buf = malloc(ISAL_DEF_LVL2_DEFAULT);
                isal_strm->level_buf_size = ISAL_DEF_LVL2_DEFAULT;
        } else if (level >= 7 && level <= 9) {
                isal_strm->level = 3;
                isal_strm->level_buf = malloc(ISAL_DEF_LVL3_DEFAULT);
                isal_strm->level_buf_size = ISAL_DEF_LVL3_DEFAULT;
        } else {
                fprintf(stderr, "Error: Invalid compression level\n");
                return -1;
        }

        deflate_state *s = (deflate_state *) malloc(sizeof(deflate_state));
        if (!s) {
                fprintf(stderr, "Error: Memory allocation for deflate_state failed\n");
                return -1;
        }

        s->level = level;
        s->w_bits = windowBits;
        s->strm = strm;

        strm->avail_out = isal_strm->avail_out;
        strm->avail_in = isal_strm->avail_in;
        strm->next_in = isal_strm->next_in;
        strm->next_out = isal_strm->next_out;
        strm->total_out = isal_strm->total_out;
        strm->total_in = isal_strm->total_in;

        // Set stream->gzip_flag to “IGZIP_DEFLATE” if windowsBits is negative (raw deflate) or
        // “IGZIP_ZLIB” if windowsBits is positive (ZLIB format)
        // Ensure hist_bits are non-negative
        if (windowBits < 0) {
                isal_strm->gzip_flag = IGZIP_DEFLATE;
                isal_strm->hist_bits = -windowBits;
        } else {
                isal_strm->gzip_flag = IGZIP_ZLIB;
                isal_strm->hist_bits = windowBits;
        }

        s->isal_strm = isal_strm;
        strm->state = (struct internal_state *) s;

        return Z_OK;
}

int
deflateInit_(z_streamp strm, int level)
{
        if (!strm) {
                fprintf(stderr, "Error: z_streamp is NULL\n");
                return -1;
        }

        return deflateInit2_(strm, level, 0, 15, 8, 0); // hardcoded windowBits
}

int
deflate(z_streamp strm, int flush)
{
        int ret;

        if (!strm) {
                fprintf(stderr, "Error: z_streamp is NULL\n");
                return -1;
        }

        deflate_state *s = (deflate_state *) strm->state;
        if (!s) {
                fprintf(stderr, "Error: deflate_state is NULL\n");
                return -1;
        }

        struct isal_zstream *isal_strm = s->isal_strm;
        if (!isal_strm) {
                fprintf(stderr, "Error: deflate isal_strm is NULL\n");
                return -1;
        }

        // set stream->avail_in, next_in, avail_out, next_out (from zstream)​
        isal_strm->next_out = strm->next_out;
        isal_strm->avail_out = strm->avail_out;
        isal_strm->next_in = strm->next_in;
        isal_strm->avail_in = strm->avail_in;
        isal_strm->total_out = strm->total_out;
        isal_strm->total_in = strm->total_in;

        // stream->flush mapping
        switch (flush) {
        case Z_NO_FLUSH:
                isal_strm->flush = NO_FLUSH;
                break;
        case Z_SYNC_FLUSH:
        case Z_PARTIAL_FLUSH:
        case Z_BLOCK:
                isal_strm->flush = SYNC_FLUSH;
                break;
        case Z_FULL_FLUSH:
                isal_strm->flush = FULL_FLUSH;
                break;
        case Z_FINISH:
                isal_strm->flush = FULL_FLUSH;
                isal_strm->end_of_stream = 1;
                break;
        default:
                fprintf(stderr, "Error: Invalid flush value\n");
                return -1;
        }

#ifdef DEBUG
        fprintf(stderr, "Gzip flag: %d, Window bits: %d, Flush: %d, Level: %d\n",
                isal_strm->gzip_flag, isal_strm->hist_bits, isal_strm->flush, isal_strm->level);
        fprintf(stderr, "Before isal_deflate: avail_in=%u, next_in=%p, avail_out=%u, next_out=%p\n",
                isal_strm->avail_in, isal_strm->next_in, isal_strm->avail_out, isal_strm->next_out);
        fprintf(stderr, "Total out: %u, Total in %lu\n", isal_strm->total_out, strm->total_in);
#endif

        int comp = isal_deflate(isal_strm);

        strm->avail_out = isal_strm->avail_out;
        strm->avail_in = isal_strm->avail_in;
        strm->next_in = isal_strm->next_in;
        strm->next_out = isal_strm->next_out;
        strm->total_out = isal_strm->total_out;
        strm->total_in = isal_strm->total_in;

#ifdef DEBUG
        fprintf(stderr, "After isal_deflate: avail_in=%u, next_in=%p, avail_out=%u, next_out=%p\n",
                strm->avail_in, strm->next_in, strm->avail_out, strm->next_out);
        fprintf(stderr, "Total out: %lu, Total in: %lu\n", strm->total_out, strm->total_in);
#endif

        if (comp == COMP_OK) {
                if (isal_strm->avail_in == 0 && isal_strm->end_of_stream) {
                        ret = Z_STREAM_END; // Compression is done
                } else {
                        ret = Z_OK;
                }
        } else {
                ret = Z_ERRNO; // Compression error
        }

#ifdef DEBUG
        if (ret == Z_OK) {
                fprintf(stderr, "Deflate finished successfully Z_OK\n");
        } else if (ret == Z_STREAM_END) {
                fprintf(stderr, "Deflate finished successfully Z_STREAM_END\n");
        } else {
                fprintf(stderr, "Deflate finished with error code: %d\n", ret);
                switch (comp) {
                case INVALID_FLUSH:
                        fprintf(stderr, "Error: Invalid flush\n");
                        break;
                case INVALID_PARAM:
                        fprintf(stderr, "Error: Invalid parameter\n");
                        break;
                case STATELESS_OVERFLOW:
                        fprintf(stderr, "Error: Stateless overflow\n");
                        break;
                case ISAL_INVALID_OPERATION:
                        fprintf(stderr, "Error: Invalid operation\n");
                        break;
                case ISAL_INVALID_STATE:
                        fprintf(stderr, "Error: Invalid state\n");
                        break;
                case ISAL_INVALID_LEVEL:
                        fprintf(stderr, "Error: Invalid level\n");
                        break;
                case ISAL_INVALID_LEVEL_BUF:
                        fprintf(stderr, "Error: Invalid level buffer\n");
                        break;
                }
        }
#endif

        return ret;
}

int
deflateEnd(z_streamp strm)
{
        if (!strm) {
                fprintf(stderr, "Error: z_streamp is NULL\n");
                return -1;
        }

        // Free allocated memory for level_buf
        if (strm->state) {
                free(strm->state);
                strm->state = NULL;
        }

#ifdef DEBUG
        fprintf(stderr, "Deflate end\n");
#endif
        return Z_OK;
}

int
deflateSetHeader(z_streamp strm, void *head)
{
        (void) head; // Suppress unused parameter warning
        return Z_OK;
}

int
deflateSetDictionary(z_streamp strm, unsigned char *dict_data, unsigned int dict_len)
{
        if (!strm || !strm->state || !dict_data || dict_len == 0)
                return Z_STREAM_ERROR;

        deflate_state *s = (deflate_state *) strm->state;

        if (!s || !s->isal_strm)
                return Z_STREAM_ERROR;

        return isal_deflate_set_dict(s->isal_strm, dict_data, dict_len);
}

int
compress2(uint8_t *dest, unsigned long *dest_len, const uint8_t *source, unsigned long source_len,
          int level)
{
        z_stream strm;
        int err;
        const unsigned int max = (unsigned int) -1;
        unsigned long left = *dest_len;
        if (dest == NULL || dest_len == NULL || source == NULL) {
                return Z_STREAM_ERROR;
        }
        *dest_len = 0;

        strm.zalloc = NULL;
        strm.zfree = NULL;
        strm.opaque = NULL;

        err = deflateInit_(&strm, level);
        if (err != Z_OK)
                return err;

        strm.next_out = dest;
        strm.avail_out = 0;
        strm.next_in = (uint8_t *) source;
        strm.avail_in = 0;

        do {
                if (strm.avail_out == 0) {
                        strm.avail_out = left > (unsigned long) max ? max : (unsigned int) left;
                        left -= strm.avail_out;
                }
                if (strm.avail_in == 0) {
                        strm.avail_in =
                                source_len > (unsigned long) max ? max : (unsigned int) source_len;
                        source_len -= strm.avail_in;
                }
                err = deflate(&strm, source_len ? Z_NO_FLUSH : Z_FINISH);
        } while (err == Z_OK);

        *dest_len = strm.total_out;
        deflateEnd(&strm);

        return err == Z_STREAM_END ? Z_OK : err;
}

int
compress(uint8_t *dest, unsigned long *dest_len, const uint8_t *source, unsigned long source_len)
{
        return compress2(dest, dest_len, source, source_len, Z_DEFAULT_COMPRESSION);
}
