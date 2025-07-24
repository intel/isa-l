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

#include "shim_inflate.h"
#include "igzip_lib.h"

#include <stdio.h>
#include <stdlib.h>

int
inflateInit2_(z_streamp strm, int windowBits)
{
        if (!strm) {
                fprintf(stderr, "Error: z_streamp is NULL\n");
                return -1;
        }

        struct inflate_state *isal_strm_inflate =
                (struct inflate_state *) malloc(sizeof(struct inflate_state));
        if (!isal_strm_inflate) {
                fprintf(stderr, "Error: Memory allocation for inflate_state failed\n");
                return -1;
        }

#ifdef DEBUG
        fprintf(stderr, "\nInitializing inflate with windowBits: %d", windowBits);
#endif

        /* Setup ISA-L de-compression context */
        isal_inflate_init(isal_strm_inflate);

        isal_strm_inflate->avail_in = 0;
        isal_strm_inflate->next_in = NULL;
        strm->total_out = 0;

        inflate_state2 *s = (inflate_state2 *) malloc(sizeof(inflate_state2));
        if (!s) {
                fprintf(stderr, "Error: Memory allocation for inflate_state2 failed\n");
                return -1;
        }

        s->strm = strm;
        s->isal_strm_inflate = isal_strm_inflate;
        s->w_bits = windowBits;

        if (windowBits < 0) {
                isal_strm_inflate->crc_flag = IGZIP_DEFLATE;
                isal_strm_inflate->hist_bits = -windowBits;
        } else {
                isal_strm_inflate->crc_flag = IGZIP_ZLIB;
                isal_strm_inflate->hist_bits = windowBits;
        }

        s->isal_strm_inflate = isal_strm_inflate;
        strm->state = (struct internal_state2 *) s;

        return Z_OK;
}

int
inflateInit_(z_streamp strm)
{
        if (!strm) {
                fprintf(stderr, "Error: z_streamp is NULL\n");
                return -1;
        }

        return inflateInit2_(strm, 15); // hardcoded windowBits
}

int
inflate(z_streamp strm, int flush)
{
        if (!strm) {
                fprintf(stderr, "Error: z_streamp is NULL\n");
                return -1;
        }

        inflate_state2 *s = (inflate_state2 *) strm->state;
        if (!s) {
                fprintf(stderr, "Error: inflate_state2 is NULL\n");
                return -1;
        }

        struct inflate_state *isal_strm_inflate = s->isal_strm_inflate;
        if (!isal_strm_inflate) {
                fprintf(stderr, "Error: isal_strm_inflate is NULL\n");
                return -1;
        }

        // set stream->avail_in, next_in, avail_out, next_out (from zstream)​
        isal_strm_inflate->next_out = strm->next_out;
        isal_strm_inflate->avail_out = strm->avail_out;
        isal_strm_inflate->avail_in = strm->avail_in;
        isal_strm_inflate->next_in = strm->next_in;
        isal_strm_inflate->total_out = strm->total_out;

#ifdef DEBUG
        fprintf(stderr, "\nInflate state: %p\n", s);
        fprintf(stderr, "Window bits: %d\n", s->w_bits);
        fprintf(stderr, "CRC flag: %d\n", isal_strm_inflate->crc_flag);
        fprintf(stderr, "Before isal_inflate: avail_in=%u, next_in=%p, avail_out=%u, next_out=%p\n",
                isal_strm_inflate->avail_in, isal_strm_inflate->next_in,
                isal_strm_inflate->avail_out, isal_strm_inflate->next_out);
        fprintf(stderr, "Total out: %u, Total_in: %lu\n", isal_strm_inflate->total_out,
                strm->total_in);
#endif

        const int decomp = isal_inflate(isal_strm_inflate);

        const unsigned int total_in = strm->total_in;
        const unsigned int avail_in = strm->avail_in;

#ifdef DEBUG
        fprintf(stderr, "After isal_inflate: avail_in=%u, next_in=%p, avail_out=%u, next_out=%p\n",
                isal_strm_inflate->avail_in, isal_strm_inflate->next_in,
                isal_strm_inflate->avail_out, isal_strm_inflate->next_out);
        fprintf(stderr, "Total out: %u, Total_in: %u\n", isal_strm_inflate->total_out,
                total_in + (avail_in - isal_strm_inflate->avail_in));
        fprintf(stderr, "Flush: %d\n", flush);
#endif

        strm->avail_out = isal_strm_inflate->avail_out;
        strm->avail_in = isal_strm_inflate->avail_in;
        strm->next_in = isal_strm_inflate->next_in;
        strm->next_out = isal_strm_inflate->next_out;
        strm->total_out = isal_strm_inflate->total_out;
        strm->total_in = total_in + (avail_in - isal_strm_inflate->avail_in);

        int ret;
        if (decomp == ISAL_DECOMP_OK) {
                if (isal_strm_inflate->block_state == ISAL_BLOCK_FINISH) {
                        ret = Z_STREAM_END; // Decompression is done
                        strm->msg = "ok";
                } else {
                        ret = Z_OK;
                }
        } else {
                ret = Z_ERRNO; // Decompression error
        }

#ifdef DEBUG
        if (ret == Z_OK) {
                fprintf(stderr, "Inflate finished successfully Z_OK\n");
        } else if (ret == Z_STREAM_END) {
                fprintf(stderr, "Inflate finished with Z_STREAM_END\n");
        } else {
                fprintf(stderr, "Inflate finished with error code: %d\n", ret);
                switch (decomp) {
                case ISAL_INVALID_BLOCK:
                        fprintf(stderr, "Error: ISA-L error - Invalid block\n");
                        break;
                case ISAL_INVALID_SYMBOL:
                        fprintf(stderr, "Error: ISA-L error - Invalid symbol\n");
                        break;
                case ISAL_INVALID_LOOKBACK:
                        fprintf(stderr, "Error: ISA-L error - Invalid lookback\n");
                        break;
                case ISAL_END_INPUT:
                        fprintf(stderr, "Error: ISA-L error - End of input reached unexpectedly\n");
                        break;
                case ISAL_UNSUPPORTED_METHOD:
                        fprintf(stderr, "Error: ISA-L error - Unsupported method\n");
                        break;
                case ISAL_NEED_DICT:
                        fprintf(stderr, "Error: ISA-L error - Need dictionary\n");
                        break;
                default:
                        fprintf(stderr, "Error: ISA-L error code: %d\n", decomp);
                        break;
                }
        }
#endif

        return ret;
}

int
inflateEnd(z_streamp strm)
{
        if (!strm) {
                fprintf(stderr, "Error: z_streamp is NULL\n");
                return -1;
        }

        inflate_state2 *s = (inflate_state2 *) strm->state;
        free(s);

        strm->state = NULL;

#ifdef DEBUG
        fprintf(stderr, "Inflate end\n");
#endif
        return Z_OK;
}

int
inflateSetDictionary(z_streamp strm, unsigned char *dict_data, unsigned int dict_len)
{
        if (!strm || !strm->state || !dict_data || dict_len == 0)
                return Z_STREAM_ERROR;

        const inflate_state2 *s = (inflate_state2 *) strm->state;

        if (!s || !s->isal_strm_inflate)
                return Z_STREAM_ERROR;

        return isal_inflate_set_dict(s->isal_strm_inflate, dict_data, dict_len);
}

int
uncompress2(uint8_t *dest, unsigned long *dest_len, const uint8_t *source,
            unsigned long *source_len)
{
        z_stream strm;
        int err;
        const unsigned int max = (unsigned int) -1;
        unsigned long len, left;
        uint8_t buf[1] = { 0 }; /* for detection of incomplete strm when *dest_len == 0 */

        len = *source_len;
        if (*dest_len) {
                left = *dest_len;
                *dest_len = 0;
        } else {
                left = 1;
                dest = buf;
        }

        strm.next_in = (uint8_t *) source;
        strm.avail_in = 0;
        strm.zalloc = NULL;
        strm.zfree = NULL;
        strm.opaque = NULL;

        err = inflateInit_(&strm);
        if (err != Z_OK)
                return err;

        strm.next_out = dest;
        strm.avail_out = 0;

        do {
                if (strm.avail_out == 0) {
                        strm.avail_out = left > (unsigned long) max ? max : (unsigned int) left;
                        left -= strm.avail_out;
                }
                if (strm.avail_in == 0) {
                        strm.avail_in = len > (unsigned long) max ? max : (unsigned int) len;
                        len -= strm.avail_in;
                }
                err = inflate(&strm, Z_NO_FLUSH);
        } while (err == Z_OK);

        *source_len -= len + strm.avail_in;
        if (dest != buf)
                *dest_len = strm.total_out;
        else if (strm.total_out && err == Z_BUF_ERROR)
                left = 1;

        inflateEnd(&strm);
        return err == Z_STREAM_END                           ? Z_OK
               : err == Z_NEED_DICT                          ? Z_DATA_ERROR
               : err == Z_BUF_ERROR && left + strm.avail_out ? Z_DATA_ERROR
                                                             : err;
}

int
uncompress(uint8_t *dest, unsigned long *dest_len, const uint8_t *source, unsigned long source_len)
{
        return uncompress2(dest, dest_len, source, &source_len);
}
