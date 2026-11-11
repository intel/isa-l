/**********************************************************************
  Copyright (c) 2025 Institute of Software Chinese Academy of Sciences (ISCAS).

  Redistribution and use in source and binary forms, with or without
  modification, are permitted provided that the following conditions
  are met:
    * Redistributions of source code must retain the above copyright
      notice, this list of conditions and the following disclaimer.
    * Redistributions in binary form must reproduce the above copyright
      notice, this list of conditions and the following disclaimer in
      the documentation and/or other materials provided with the
      distribution.
    * Neither the name of ISCAS nor the names of its
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
#include "riscv64_multibinary.h"

extern uint32_t
adler32_rvv(uint32_t, uint8_t *, uint64_t);
extern uint32_t
adler32_rvv128(uint32_t, uint8_t *, uint64_t);
extern uint32_t
adler32_base(uint32_t, uint8_t *, uint64_t);

extern void
isal_deflate_hash_riscv64(uint16_t *, uint32_t, uint32_t, uint8_t *, uint32_t);
extern void
isal_deflate_hash_base(uint16_t *, uint32_t, uint32_t, uint8_t *, uint32_t);

struct isal_zstream;
extern void
isal_deflate_icf_body_hash_hist_rvv(struct isal_zstream *);
extern void
isal_deflate_icf_body_hash_hist_base(struct isal_zstream *);

extern void
isal_deflate_icf_finish_hash_hist_rvv(struct isal_zstream *);
extern void
isal_deflate_icf_finish_hash_hist_base(struct isal_zstream *);

extern void
isal_deflate_body_rvv(struct isal_zstream *);
extern void
isal_deflate_body_base(struct isal_zstream *);
extern void
isal_deflate_finish_rvv(struct isal_zstream *);
extern void
isal_deflate_finish_base(struct isal_zstream *);

struct isal_huff_histogram;
extern void
isal_update_histogram_rvv(uint8_t *, int, struct isal_huff_histogram *);
extern void
isal_update_histogram_base(uint8_t *, int, struct isal_huff_histogram *);

struct deflate_icf;
extern void
set_long_icf_fg_rvv(uint8_t *, uint64_t, uint64_t, struct deflate_icf *);
extern void
set_long_icf_fg_base(uint8_t *, uint64_t, uint64_t, struct deflate_icf *);

DEFINE_INTERFACE_DISPATCHER(isal_adler32)
{
#if HAVE_RVV
        const unsigned long hwcap = getauxval(AT_HWCAP);
        if (hwcap & HWCAP_RV('V')) {
                unsigned long vlenb;
                __asm__ volatile("csrr %0, vlenb" : "=r"(vlenb));
                if (vlenb == 16)
                        return adler32_rvv128;
                else
                        return adler32_rvv;
        } else
#endif
                return adler32_base;
}

DEFINE_INTERFACE_DISPATCHER(isal_deflate_hash_lvl0) { return isal_deflate_hash_riscv64; }

DEFINE_INTERFACE_DISPATCHER(isal_deflate_hash_lvl1) { return isal_deflate_hash_riscv64; }

DEFINE_INTERFACE_DISPATCHER(isal_deflate_hash_lvl2) { return isal_deflate_hash_riscv64; }

DEFINE_INTERFACE_DISPATCHER(isal_deflate_hash_lvl3) { return isal_deflate_hash_riscv64; }

DEFINE_INTERFACE_DISPATCHER(isal_deflate_icf_body_lvl1)
{
#if HAVE_RVV
        if (getauxval(AT_HWCAP) & HWCAP_RV('V'))
                return isal_deflate_icf_body_hash_hist_rvv;
#endif
        return isal_deflate_icf_body_hash_hist_base;
}
DEFINE_INTERFACE_DISPATCHER(isal_deflate_icf_body_lvl2)
{
#if HAVE_RVV
        if (getauxval(AT_HWCAP) & HWCAP_RV('V'))
                return isal_deflate_icf_body_hash_hist_rvv;
#endif
        return isal_deflate_icf_body_hash_hist_base;
}

DEFINE_INTERFACE_DISPATCHER(isal_deflate_icf_finish_lvl1)
{
#if HAVE_RVV
        if (getauxval(AT_HWCAP) & HWCAP_RV('V'))
                return isal_deflate_icf_finish_hash_hist_rvv;
#endif
        return isal_deflate_icf_finish_hash_hist_base;
}
DEFINE_INTERFACE_DISPATCHER(isal_deflate_icf_finish_lvl2)
{
#if HAVE_RVV
        if (getauxval(AT_HWCAP) & HWCAP_RV('V'))
                return isal_deflate_icf_finish_hash_hist_rvv;
#endif
        return isal_deflate_icf_finish_hash_hist_base;
}

DEFINE_INTERFACE_DISPATCHER(isal_deflate_body)
{
#if HAVE_RVV
        if (getauxval(AT_HWCAP) & HWCAP_RV('V'))
                return isal_deflate_body_rvv;
#endif
        return isal_deflate_body_base;
}
DEFINE_INTERFACE_DISPATCHER(isal_deflate_finish)
{
#if HAVE_RVV
        if (getauxval(AT_HWCAP) & HWCAP_RV('V'))
                return isal_deflate_finish_rvv;
#endif
        return isal_deflate_finish_base;
}

DEFINE_INTERFACE_DISPATCHER(isal_update_histogram)
{
#if HAVE_RVV
        if (getauxval(AT_HWCAP) & HWCAP_RV('V'))
                return isal_update_histogram_rvv;
#endif
        return isal_update_histogram_base;
}

DEFINE_INTERFACE_DISPATCHER(set_long_icf_fg)
{
#if HAVE_RVV
        if (getauxval(AT_HWCAP) & HWCAP_RV('V'))
                return set_long_icf_fg_rvv;
#endif
        return set_long_icf_fg_base;
}
