/**********************************************************************
  Copyright(c) 2019-2020 Arm Corporation All rights reserved.

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
#include "crc.h"
#include "crc64.h"

extern uint16_t
crc16_t10dif_pmull(uint16_t, uint8_t *, uint64_t);

extern uint16_t
crc16_t10dif_copy_pmull(uint16_t, uint8_t *, uint8_t *, uint64_t);

extern uint32_t
crc32_ieee_norm_pmull(uint32_t, uint8_t *, uint64_t);

extern unsigned int
crc32_iscsi_crc_ext(unsigned char *, int, unsigned int);
extern unsigned int
crc32_iscsi_3crc_fold(unsigned char *, int, unsigned int);
extern unsigned int
crc32_iscsi_refl_pmull(unsigned char *, int, unsigned int);

extern uint32_t
crc32_gzip_refl_crc_ext(uint32_t, uint8_t *, uint64_t);
extern uint32_t
crc32_gzip_refl_3crc_fold(uint32_t, uint8_t *, uint64_t);
extern uint32_t
crc32_gzip_refl_pmull(uint32_t, uint8_t *, uint64_t);

extern uint64_t
crc64_ecma_refl_pmull(uint64_t, const unsigned char *, uint64_t);

extern uint64_t
crc64_ecma_norm_pmull(uint64_t, const unsigned char *, uint64_t);

extern uint64_t
crc64_iso_refl_pmull(uint64_t, const unsigned char *, uint64_t);

extern uint64_t
crc64_iso_norm_pmull(uint64_t, const unsigned char *, uint64_t);

extern uint64_t
crc64_jones_refl_pmull(uint64_t, const unsigned char *, uint64_t);

extern uint64_t
crc64_jones_norm_pmull(uint64_t, const unsigned char *, uint64_t);

DEFINE_INTERFACE_DISPATCHER(crc16_t10dif)
{
#if defined(__linux__)
        unsigned long auxval = getauxval(AT_HWCAP);
        if (auxval & HWCAP_PMULL)
                return crc16_t10dif_pmull;
#elif defined(__APPLE__)
        if (sysctlEnabled(SYSCTL_PMULL_KEY))
                return crc16_t10dif_pmull;
#endif
        return crc16_t10dif_base;
}

DEFINE_INTERFACE_DISPATCHER(crc16_t10dif_copy)
{
#if defined(__linux__)
        unsigned long auxval = getauxval(AT_HWCAP);
        if (auxval & HWCAP_PMULL)
                return crc16_t10dif_copy_pmull;
#elif defined(__APPLE__)
        if (sysctlEnabled(SYSCTL_PMULL_KEY))
                return crc16_t10dif_copy_pmull;
#endif
        return crc16_t10dif_copy_base;
}

DEFINE_INTERFACE_DISPATCHER(crc32_ieee)
{
#if defined(__linux__)
        unsigned long auxval = getauxval(AT_HWCAP);
        if (auxval & HWCAP_PMULL) {
                return crc32_ieee_norm_pmull;
        }
#elif defined(__APPLE__)
        if (sysctlEnabled(SYSCTL_PMULL_KEY))
                return crc32_ieee_norm_pmull;
#endif
        return crc32_ieee_base;
}

DEFINE_INTERFACE_DISPATCHER(crc32_iscsi)
{
#if defined(__linux__)
        unsigned long auxval = getauxval(AT_HWCAP);
        if (auxval & HWCAP_CRC32) {
                switch (get_micro_arch_id()) {
                case MICRO_ARCH_ID(ARM, NEOVERSE_N1):
                case MICRO_ARCH_ID(ARM, CORTEX_A57):
                case MICRO_ARCH_ID(ARM, CORTEX_A72):
                        return crc32_iscsi_crc_ext;
                }
        }
        if ((HWCAP_CRC32 | HWCAP_PMULL) == (auxval & (HWCAP_CRC32 | HWCAP_PMULL))) {
                return crc32_iscsi_3crc_fold;
        }

        if (auxval & HWCAP_PMULL) {
                return crc32_iscsi_refl_pmull;
        }
#elif defined(__APPLE__)
        if (sysctlEnabled(SYSCTL_CRC32_KEY))
                return crc32_iscsi_3crc_fold;
        if (sysctlEnabled(SYSCTL_PMULL_KEY))
                return crc32_iscsi_refl_pmull;
#endif
        return crc32_iscsi_base;
}

DEFINE_INTERFACE_DISPATCHER(crc32_gzip_refl)
{
#if defined(__linux__)
        unsigned long auxval = getauxval(AT_HWCAP);

        if (auxval & HWCAP_CRC32) {
                switch (get_micro_arch_id()) {
                case MICRO_ARCH_ID(ARM, NEOVERSE_N1):
                case MICRO_ARCH_ID(ARM, CORTEX_A57):
                case MICRO_ARCH_ID(ARM, CORTEX_A72):
                        return crc32_gzip_refl_crc_ext;
                }
        }
        if ((HWCAP_CRC32 | HWCAP_PMULL) == (auxval & (HWCAP_CRC32 | HWCAP_PMULL))) {
                return crc32_gzip_refl_3crc_fold;
        }

        if (auxval & HWCAP_PMULL)
                return crc32_gzip_refl_pmull;
#elif defined(__APPLE__)
        if (sysctlEnabled(SYSCTL_CRC32_KEY))
                return crc32_gzip_refl_3crc_fold;
        if (sysctlEnabled(SYSCTL_PMULL_KEY))
                return crc32_gzip_refl_pmull;
#endif
        return crc32_gzip_refl_base;
}

DEFINE_INTERFACE_DISPATCHER(crc64_ecma_refl)
{
#if defined(__linux__)
        unsigned long auxval = getauxval(AT_HWCAP);

        if (auxval & HWCAP_PMULL)
                return crc64_ecma_refl_pmull;
#elif defined(__APPLE__)
        if (sysctlEnabled(SYSCTL_PMULL_KEY))
                return crc64_ecma_refl_pmull;
#endif
        return crc64_ecma_refl_base;
}

DEFINE_INTERFACE_DISPATCHER(crc64_ecma_norm)
{
#if defined(__linux__)
        unsigned long auxval = getauxval(AT_HWCAP);
        if (auxval & HWCAP_PMULL)
                return crc64_ecma_norm_pmull;
#elif defined(__APPLE__)
        if (sysctlEnabled(SYSCTL_PMULL_KEY))
                return crc64_ecma_norm_pmull;
#endif
        return crc64_ecma_norm_base;
}

DEFINE_INTERFACE_DISPATCHER(crc64_iso_refl)
{
#if defined(__linux__)
        unsigned long auxval = getauxval(AT_HWCAP);
        if (auxval & HWCAP_PMULL)
                return crc64_iso_refl_pmull;
#elif defined(__APPLE__)
        if (sysctlEnabled(SYSCTL_PMULL_KEY))
                return crc64_iso_refl_pmull;
#endif
        return crc64_iso_refl_base;
}

DEFINE_INTERFACE_DISPATCHER(crc64_iso_norm)
{
#if defined(__linux__)
        unsigned long auxval = getauxval(AT_HWCAP);
        if (auxval & HWCAP_PMULL)
                return crc64_iso_norm_pmull;
#elif defined(__APPLE__)
        if (sysctlEnabled(SYSCTL_PMULL_KEY))
                return crc64_iso_norm_pmull;
#endif
        return crc64_iso_norm_base;
}

DEFINE_INTERFACE_DISPATCHER(crc64_jones_refl)
{
#if defined(__linux__)
        unsigned long auxval = getauxval(AT_HWCAP);
        if (auxval & HWCAP_PMULL)
                return crc64_jones_refl_pmull;
#elif defined(__APPLE__)
        if (sysctlEnabled(SYSCTL_PMULL_KEY))
                return crc64_jones_refl_pmull;
#endif
        return crc64_jones_refl_base;
}

DEFINE_INTERFACE_DISPATCHER(crc64_jones_norm)
{
#if defined(__linux__)
        unsigned long auxval = getauxval(AT_HWCAP);
        if (auxval & HWCAP_PMULL)
                return crc64_jones_norm_pmull;
#elif defined(__APPLE__)
        if (sysctlEnabled(SYSCTL_PMULL_KEY))
                return crc64_jones_norm_pmull;
#endif
        return crc64_jones_norm_base;
}
