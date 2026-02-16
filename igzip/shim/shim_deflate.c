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
#include <string.h>

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
                free(isal_strm);
                return -1;
        }

        if (!isal_strm->level_buf) {
                free(isal_strm);
                fprintf(stderr, "Error: Memory allocation for level_buf failed\n");
                return -1;
        }
        deflate_state *s = (deflate_state *) malloc(sizeof(deflate_state));
        if (!s) {
                free(isal_strm->level_buf);
                free(isal_strm);
                fprintf(stderr, "Error: Memory allocation for deflate_state failed\n");
                return -1;
        }

        s->level = level;
        s->w_bits = windowBits;
        s->strm = strm;
        s->gz_header = NULL;   /* Initialize to NULL */
        s->header_written = 0; /* Header not yet written */
        s->crc32_value = 0;    /* Initialize CRC32 */
        s->input_size = 0;     /* Initialize input size */

        // Sanity checks on windowBits
        // Valid ranges: 0 (default), 8-15 (standard), -8 to -15 (raw), 16+ (gzip)
        if (windowBits != 0 &&
            (windowBits < -15 || (windowBits > -8 && windowBits < 8) || windowBits > 31)) {
                fprintf(stderr, "Error: Invalid windowBits value: %d\n", windowBits);
                free(s);
                free(isal_strm->level_buf);
                free(isal_strm);
                return Z_STREAM_ERROR;
        }

        if (windowBits < 0) {
                // Raw deflate mode - no headers/trailers
                isal_strm->gzip_flag = IGZIP_DEFLATE;
                isal_strm->hist_bits = -windowBits;
        } else if (windowBits > 15) {
                // Gzip format (windowBits > 15 means add gzip header)
                isal_strm->gzip_flag = IGZIP_GZIP;
                isal_strm->hist_bits = windowBits - 16;
        } else {
                // Standard zlib format (8..15)
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

        // Write custom gzip header if set and not yet written
        if (s->gz_header && !s->header_written && s->w_bits > 15) {
                // Switch to raw deflate mode for manual header/trailer
                isal_strm->gzip_flag = IGZIP_DEFLATE;

                gz_headerp gzh = s->gz_header;
                unsigned char *out = strm->next_out;
                int header_len = 10; // Base gzip header size

                // Calculate total header size needed
                int total_header_size = header_len;
                if (gzh->extra)
                        total_header_size += 2 + gzh->extra_len;
                if (gzh->name)
                        total_header_size += strlen((char *) gzh->name) + 1;
                if (gzh->comment)
                        total_header_size += strlen((char *) gzh->comment) + 1;
                if (gzh->hcrc)
                        total_header_size += 2;

                // Check if we have enough space
                if (strm->avail_out < (unsigned int) total_header_size) {
                        return Z_BUF_ERROR;
                }

                // Write standard gzip header (10 bytes)
                out[0] = 0x1f; // ID1
                out[1] = 0x8b; // ID2
                out[2] = 0x08; // CM (deflate)

                // Flags byte
                unsigned char flags = (gzh->text ? 0x01 : 0) | (gzh->hcrc ? 0x02 : 0) |
                                      (gzh->extra ? 0x04 : 0) | (gzh->name ? 0x08 : 0) |
                                      (gzh->comment ? 0x10 : 0);
                out[3] = flags;

                // MTIME (4 bytes, little-endian)
                for (int i = 0; i < 4; i++) {
                        out[4 + i] = (gzh->time >> (8 * i)) & 0xff;
                }

                out[8] = gzh->xflags; // XFL
                out[9] = gzh->os;     // OS

                // FEXTRA field
                if (gzh->extra) {
                        out[header_len++] = gzh->extra_len & 0xff;
                        out[header_len++] = (gzh->extra_len >> 8) & 0xff;
                        memcpy(&out[header_len], gzh->extra, gzh->extra_len);
                        header_len += gzh->extra_len;
                }

                // FNAME field
                if (gzh->name) {
                        const int name_len = strlen((char *) gzh->name) + 1;
                        memcpy(&out[header_len], gzh->name, name_len);
                        header_len += name_len;
                }

                // FCOMMENT field
                if (gzh->comment) {
                        const int comm_len = strlen((char *) gzh->comment) + 1;
                        memcpy(&out[header_len], gzh->comment, comm_len);
                        header_len += comm_len;
                }

                // FHCRC
                if (gzh->hcrc) {
                        unsigned long hcrc = crc32_gzip_refl(0, out, header_len);
                        out[header_len++] = hcrc & 0xff;
                        out[header_len++] = (hcrc >> 8) & 0xff;
                }

                // Update output pointers
                strm->next_out += header_len;
                strm->avail_out -= header_len;
                strm->total_out += header_len;

                s->header_written = 1;
                s->crc32_value = 0; // Initialize CRC for data
                s->input_size = 0;
        }

        // set stream->avail_in, next_in, avail_out, next_out (from zstream)​
        isal_strm->next_out = strm->next_out;
        isal_strm->avail_out = strm->avail_out;
        isal_strm->next_in = strm->next_in;
        isal_strm->avail_in = strm->avail_in;
        isal_strm->total_out = strm->total_out;
        isal_strm->total_in = strm->total_in;

        // Track input for CRC calculation when using custom headers
        const unsigned char *input_start = isal_strm->next_in;
        unsigned int input_len = isal_strm->avail_in;

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

        // Update CRC and input size for custom gzip trailer
        if (s->gz_header && input_len > 0) {
                unsigned int consumed = input_len - isal_strm->avail_in;
                if (consumed > 0) {
                        s->crc32_value = crc32_gzip_refl(s->crc32_value, input_start, consumed);
                        s->input_size += consumed;
                }
        }

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
                if (isal_strm->end_of_stream && isal_strm->avail_out > 0) {
                        // Write gzip trailer if using custom header
                        if (s->gz_header) {
                                if (strm->avail_out < 8) {
                                        // Not enough space for trailer
                                        ret = Z_BUF_ERROR;
                                } else {
                                        unsigned char *out = strm->next_out;

                                        // Write CRC32 and ISIZE (8 bytes total)
                                        for (int i = 0; i < 4; i++) {
                                                out[i] =
                                                        (s->crc32_value >> (8 * i)) & 0xff; // CRC32
                                                out[i + 4] =
                                                        (s->input_size >> (8 * i)) & 0xff; // ISIZE
                                        }

                                        strm->next_out += 8;
                                        strm->avail_out -= 8;
                                        strm->total_out += 8;

                                        ret = Z_STREAM_END;
                                }
                        } else {
                                ret = Z_STREAM_END; // Compression is done
                        }
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

        // Free allocated memory for level_buf and isal_strm
        if (strm->state) {
                const deflate_state *s = (deflate_state *) strm->state;
                if (s) {
                        // Free gz_header if allocated
                        if (s->gz_header) {
                                if (s->gz_header->extra)
                                        free(s->gz_header->extra);
                                if (s->gz_header->name)
                                        free(s->gz_header->name);
                                if (s->gz_header->comment)
                                        free(s->gz_header->comment);
                                free(s->gz_header);
                        }

                        if (s->isal_strm) {
                                if (s->isal_strm->level_buf) {
                                        free(s->isal_strm->level_buf);
                                }
                                free(s->isal_strm);
                        }
                }
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
        if (!strm) {
                fprintf(stderr, "Error: z_streamp is NULL\n");
                return Z_STREAM_ERROR;
        }

        deflate_state *s = (deflate_state *) strm->state;
        if (!s) {
                fprintf(stderr, "Error: deflate_state is NULL\n");
                return Z_STREAM_ERROR;
        }

        // Check mode (windowBits > 15)
        if (s->w_bits <= 15) {
                fprintf(stderr, "Error: deflateSetHeader requires gzip format (windowBits > 15)\n");
                return Z_STREAM_ERROR;
        }

        if (!head) {
                return Z_STREAM_ERROR;
        }

        // Free any existing gz_header
        if (s->gz_header) {
                if (s->gz_header->extra)
                        free(s->gz_header->extra);
                if (s->gz_header->name)
                        free(s->gz_header->name);
                if (s->gz_header->comment)
                        free(s->gz_header->comment);
                free(s->gz_header);
        }

        // Allocate and copy gz_header structure
        s->gz_header = (gz_headerp) malloc(sizeof(gz_header));
        if (!s->gz_header) {
                fprintf(stderr, "Error: Memory allocation for gz_header failed\n");
                return Z_MEM_ERROR;
        }

        gz_headerp src = (gz_headerp) head;
        memcpy(s->gz_header, src, sizeof(gz_header));

        // Copy the fields if they exist
        if (src->extra && src->extra_len > 0) {
                s->gz_header->extra = (unsigned char *) malloc(src->extra_len);
                if (!s->gz_header->extra)
                        goto cleanup;
                memcpy(s->gz_header->extra, src->extra, src->extra_len);
        } else {
                s->gz_header->extra = NULL;
        }

        if (src->name) {
                int name_len = strlen((char *) src->name) + 1;
                s->gz_header->name = (unsigned char *) malloc(name_len);
                if (!s->gz_header->name)
                        goto cleanup;
                memcpy(s->gz_header->name, src->name, name_len);
        } else {
                s->gz_header->name = NULL;
        }

        if (src->comment) {
                int comm_len = strlen((char *) src->comment) + 1;
                s->gz_header->comment = (unsigned char *) malloc(comm_len);
                if (!s->gz_header->comment)
                        goto cleanup;
                memcpy(s->gz_header->comment, src->comment, comm_len);
        } else {
                s->gz_header->comment = NULL;
        }

        return Z_OK;

cleanup:
        if (s->gz_header->extra)
                free(s->gz_header->extra);
        if (s->gz_header->name)
                free(s->gz_header->name);
        free(s->gz_header);
        s->gz_header = NULL;
        return Z_MEM_ERROR;
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
        unsigned long left;

        if (dest == NULL || dest_len == NULL || source == NULL) {
                return Z_STREAM_ERROR;
        }

        left = *dest_len;
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

unsigned long
crc32(unsigned long crc, const unsigned char *buf, unsigned int len)
{
        return crc32_gzip_refl(crc, buf, len);
}

unsigned long
adler32(unsigned long adler, const unsigned char *buf, unsigned int len)
{
        return isal_adler32(adler, buf, len);
}