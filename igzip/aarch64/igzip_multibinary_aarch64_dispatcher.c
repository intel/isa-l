/**********************************************************************
  Copyright(c) 2019 Arm Corporation All rights reserved.

  Redistribution and use in source and binary forms, with or without
  modification, are permitted provided that the following conditions
  are met:
    * Redistributions of source code must retain the above copyright
      notice, this list of conditions and the following disclaimer.
    * Redistributions in binary form must reproduce the above copyright
      notice, this list of conditions and the following disclaimer in
      the documentation and/or other materials provided with the
      distribution.
    * Neither the name of Arm Corporation nor the names of its
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
#include <aarch64_multibinary.h>
#include "igzip_lib.h"
#include "igzip/encode_df.h"

extern uint32_t
adler32_neon(uint32_t, uint8_t *, uint64_t);
extern uint32_t
adler32_base(uint32_t, uint8_t *, uint64_t);

extern void
isal_deflate_body_aarch64(struct isal_zstream *);
extern void
isal_deflate_body_base(struct isal_zstream *);

extern void
isal_deflate_finish_aarch64(struct isal_zstream *);
extern void
isal_deflate_finish_base(struct isal_zstream *);

extern void
isal_deflate_icf_body_hash_hist_aarch64(struct isal_zstream *);
extern void
isal_deflate_icf_body_hash_hist_base(struct isal_zstream *);

extern void
isal_deflate_icf_finish_hash_hist_aarch64(struct isal_zstream *);
extern void
isal_deflate_icf_finish_hash_hist_base(struct isal_zstream *);

extern void
icf_body_hash1_fillgreedy_lazy(struct isal_zstream *);

extern void
isal_deflate_icf_finish_hash_map_base(struct isal_zstream *);

extern void
set_long_icf_fg_aarch64(uint8_t *, uint64_t, uint64_t, struct deflate_icf *);

extern struct deflate_icf *
encode_deflate_icf_aarch64(struct deflate_icf *, struct deflate_icf *, struct BitBuf2 *,
                           struct hufftables_icf *);

extern void
isal_update_histogram_aarch64(uint8_t *, int, struct isal_huff_histogram *);
extern void
isal_update_histogram_base(uint8_t *, int, struct isal_huff_histogram *);

extern uint64_t
gen_icf_map_h1_aarch64(struct isal_zstream *, struct deflate_icf *, uint64_t input_size);
extern uint64_t
gen_icf_map_h1_base(struct isal_zstream *, struct deflate_icf *, uint64_t input_size);

extern void
isal_deflate_hash_aarch64(uint16_t *, uint32_t, uint32_t, uint8_t *, uint32_t);
extern void
isal_deflate_hash_base(uint16_t *, uint32_t, uint32_t, uint8_t *, uint32_t);

extern int
decode_huffman_code_block_stateless_aarch64(struct inflate_state *, uint8_t *);
extern int
decode_huffman_code_block_stateless_base(struct inflate_state *, uint8_t *);

DEFINE_INTERFACE_DISPATCHER(isal_adler32)
{
#if defined(__linux__)
        unsigned long auxval = getauxval(AT_HWCAP);
        if (auxval & HWCAP_ASIMD)
                return adler32_neon;
#elif defined(__APPLE__)
        return adler32_neon;
#endif
        return adler32_base;
}

DEFINE_INTERFACE_DISPATCHER(isal_deflate_body)
{
#if defined(__linux__)
        unsigned long auxval = getauxval(AT_HWCAP);

        if (auxval & HWCAP_CRC32)
                return isal_deflate_body_aarch64;
#elif defined(__APPLE__)
        if (sysctlEnabled(SYSCTL_CRC32_KEY))
                return isal_deflate_body_aarch64;
#endif
        return isal_deflate_body_base;
}

DEFINE_INTERFACE_DISPATCHER(isal_deflate_finish)
{
#if defined(__linux__)
        unsigned long auxval = getauxval(AT_HWCAP);
        if (auxval & HWCAP_CRC32)
                return isal_deflate_finish_aarch64;
#elif defined(__APPLE__)
        if (sysctlEnabled(SYSCTL_CRC32_KEY))
                return isal_deflate_finish_aarch64;
#endif
        return isal_deflate_finish_base;
}

DEFINE_INTERFACE_DISPATCHER(isal_deflate_icf_body_lvl1)
{
#if defined(__linux__)
        unsigned long auxval = getauxval(AT_HWCAP);
        if (auxval & HWCAP_CRC32)
                return isal_deflate_icf_body_hash_hist_aarch64;
#elif defined(__APPLE__)
        if (sysctlEnabled(SYSCTL_CRC32_KEY))
                return isal_deflate_icf_body_hash_hist_aarch64;
#endif
        return isal_deflate_icf_body_hash_hist_base;
}

DEFINE_INTERFACE_DISPATCHER(isal_deflate_icf_finish_lvl1)
{
#if defined(__linux__)
        unsigned long auxval = getauxval(AT_HWCAP);
        if (auxval & HWCAP_CRC32)
                return isal_deflate_icf_finish_hash_hist_aarch64;
#elif defined(__APPLE__)
        if (sysctlEnabled(SYSCTL_CRC32_KEY))
                return isal_deflate_icf_finish_hash_hist_aarch64;
#endif
        return isal_deflate_icf_finish_hash_hist_base;
}

DEFINE_INTERFACE_DISPATCHER(isal_deflate_icf_body_lvl2)
{
#if defined(__linux__)
        unsigned long auxval = getauxval(AT_HWCAP);
        if (auxval & HWCAP_CRC32)
                return isal_deflate_icf_body_hash_hist_aarch64;
#elif defined(__APPLE__)
        if (sysctlEnabled(SYSCTL_CRC32_KEY))
                return isal_deflate_icf_body_hash_hist_aarch64;
#endif
        return isal_deflate_icf_body_hash_hist_base;
}

DEFINE_INTERFACE_DISPATCHER(isal_deflate_icf_finish_lvl2)
{
#if defined(__linux__)
        unsigned long auxval = getauxval(AT_HWCAP);
        if (auxval & HWCAP_CRC32)
                return isal_deflate_icf_finish_hash_hist_aarch64;
#elif defined(__APPLE__)
        if (sysctlEnabled(SYSCTL_CRC32_KEY))
                return isal_deflate_icf_finish_hash_hist_aarch64;
#endif
        return isal_deflate_icf_finish_hash_hist_base;
}

DEFINE_INTERFACE_DISPATCHER(isal_deflate_icf_body_lvl3)
{
#if defined(__linux__)
        unsigned long auxval = getauxval(AT_HWCAP);
        if (auxval & HWCAP_CRC32)
                return icf_body_hash1_fillgreedy_lazy;
#elif defined(__APPLE__)
        if (sysctlEnabled(SYSCTL_CRC32_KEY))
                return icf_body_hash1_fillgreedy_lazy;
#endif
        return icf_body_hash1_fillgreedy_lazy;
}

DEFINE_INTERFACE_DISPATCHER(isal_deflate_icf_finish_lvl3)
{
#if defined(__linux__)
        unsigned long auxval = getauxval(AT_HWCAP);
        if (auxval & HWCAP_CRC32)
                return isal_deflate_icf_finish_hash_map_base;
#elif defined(__APPLE__)
        if (sysctlEnabled(SYSCTL_CRC32_KEY))
                return isal_deflate_icf_finish_hash_map_base;
#endif
        return isal_deflate_icf_finish_hash_map_base;
}

DEFINE_INTERFACE_DISPATCHER(set_long_icf_fg) { return set_long_icf_fg_aarch64; }

DEFINE_INTERFACE_DISPATCHER(encode_deflate_icf) { return encode_deflate_icf_aarch64; }

DEFINE_INTERFACE_DISPATCHER(isal_update_histogram)
{
#if defined(__linux__)
        unsigned long auxval = getauxval(AT_HWCAP);
        if (auxval & HWCAP_CRC32)
                return isal_update_histogram_aarch64;
#elif defined(__APPLE__)
        if (sysctlEnabled(SYSCTL_CRC32_KEY))
                return isal_update_histogram_aarch64;
#endif
        return isal_update_histogram_base;
}

DEFINE_INTERFACE_DISPATCHER(gen_icf_map_lh1)
{
#if defined(__linux__)
        unsigned long auxval = getauxval(AT_HWCAP);
        if (auxval & HWCAP_CRC32) {
                return gen_icf_map_h1_aarch64;
        }
#elif defined(__APPLE__)
        if (sysctlEnabled(SYSCTL_CRC32_KEY))
                return gen_icf_map_h1_aarch64;
#endif
        return gen_icf_map_h1_base;
}

DEFINE_INTERFACE_DISPATCHER(isal_deflate_hash_lvl0)
{
#if defined(__linux__)
        unsigned long auxval = getauxval(AT_HWCAP);
        if (auxval & HWCAP_CRC32)
                return isal_deflate_hash_aarch64;
#elif defined(__APPLE__)
        if (sysctlEnabled(SYSCTL_CRC32_KEY))
                return isal_deflate_hash_aarch64;
#endif
        return isal_deflate_hash_base;
}

DEFINE_INTERFACE_DISPATCHER(isal_deflate_hash_lvl1)
{
#if defined(__linux__)
        unsigned long auxval = getauxval(AT_HWCAP);
        if (auxval & HWCAP_CRC32)
                return isal_deflate_hash_aarch64;
#elif defined(__APPLE__)
        if (sysctlEnabled(SYSCTL_CRC32_KEY))
                return isal_deflate_hash_aarch64;
#endif
        return isal_deflate_hash_base;
}

DEFINE_INTERFACE_DISPATCHER(isal_deflate_hash_lvl2)
{
#if defined(__linux__)
        unsigned long auxval = getauxval(AT_HWCAP);
        if (auxval & HWCAP_CRC32)
                return isal_deflate_hash_aarch64;
#elif defined(__APPLE__)
        if (sysctlEnabled(SYSCTL_CRC32_KEY))
                return isal_deflate_hash_aarch64;
#endif
        return isal_deflate_hash_base;
}

DEFINE_INTERFACE_DISPATCHER(isal_deflate_hash_lvl3)
{
#if defined(__linux__)
        unsigned long auxval = getauxval(AT_HWCAP);
        if (auxval & HWCAP_CRC32)
                return isal_deflate_hash_aarch64;
#elif defined(__APPLE__)
        if (sysctlEnabled(SYSCTL_CRC32_KEY))
                return isal_deflate_hash_aarch64;
#endif
        return isal_deflate_hash_base;
}

DEFINE_INTERFACE_DISPATCHER(decode_huffman_code_block_stateless)
{
#if defined(__linux__)
        unsigned long auxval = getauxval(AT_HWCAP);
        if (auxval & HWCAP_CRC32)
                return decode_huffman_code_block_stateless_aarch64;
#elif defined(__APPLE__)
        if (sysctlEnabled(SYSCTL_CRC32_KEY))
                return decode_huffman_code_block_stateless_aarch64;
#endif
        return decode_huffman_code_block_stateless_base;
}
