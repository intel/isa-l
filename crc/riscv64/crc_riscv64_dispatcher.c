/**********************************************************************
  Copyright(c) 2025 ZTE Corporation All rights reserved.

  Redistribution and use in source and binary forms, with or without
  modification, are permitted provided that the following conditions
  are met:
    * Redistributions of source code must retain the above copyright
      notice, this list of conditions and the following disclaimer.
    * Redistributions in binary form must reproduce the above copyright
      notice, this list of conditions and the following disclaimer in
      the documentation and/or other materials provided with the
      distribution.
    * Neither the name of ZTE Corporation nor the names of its
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
#include <riscv64_multibinary.h>
#include "crc.h"
#include "crc64.h"

extern uint16_t
crc16_t10dif_vclmul(uint16_t, uint8_t *, uint64_t);

extern uint16_t
crc16_t10dif_copy_vclmul(uint16_t, uint8_t *, uint8_t *, uint64_t);

extern uint32_t
crc32_ieee_norm_vclmul(uint32_t, uint8_t *, uint64_t);

extern unsigned int
crc32_iscsi_refl_vclmul(unsigned char *, int, unsigned int);

extern uint32_t
crc32_gzip_refl_vclmul(uint32_t, uint8_t *, uint64_t);

extern uint64_t
crc64_ecma_refl_vclmul(uint64_t, const unsigned char *, uint64_t);

extern uint64_t
crc64_ecma_norm_vclmul(uint64_t, const unsigned char *, uint64_t);

extern uint64_t
crc64_iso_refl_vclmul(uint64_t, const unsigned char *, uint64_t);

extern uint64_t
crc64_iso_norm_vclmul(uint64_t, const unsigned char *, uint64_t);

extern uint64_t
crc64_jones_refl_vclmul(uint64_t, const unsigned char *, uint64_t);

extern uint64_t
crc64_jones_norm_vclmul(uint64_t, const unsigned char *, uint64_t);

extern uint64_t
crc64_rocksoft_refl_vclmul(uint64_t, const unsigned char *, uint64_t);

extern uint64_t
crc64_rocksoft_norm_vclmul(uint64_t, const unsigned char *, uint64_t);

DEFINE_INTERFACE_DISPATCHER(crc16_t10dif)
{
#if HAVE_ZBC && HAVE_ZBB && HAVE_ZVBC
        if (CHECK_RISCV_EXTENSIONS("ZVBC", "ZBC", "ZBB")) {
                return crc16_t10dif_vclmul;
        }
#endif
        return crc16_t10dif_base;
}

DEFINE_INTERFACE_DISPATCHER(crc16_t10dif_copy)
{
#if HAVE_ZBC && HAVE_ZBB && HAVE_ZVBC
        if (CHECK_RISCV_EXTENSIONS("ZVBC", "ZBC", "ZBB")) {
                return crc16_t10dif_copy_vclmul;
        }
#endif
        return crc16_t10dif_copy_base;
}

DEFINE_INTERFACE_DISPATCHER(crc32_ieee)
{
#if HAVE_ZBC && HAVE_ZBB && HAVE_ZVBC
        if (CHECK_RISCV_EXTENSIONS("ZVBC", "ZBC", "ZBB")) {
                return crc32_ieee_norm_vclmul;
        }
#endif
        return crc32_ieee_base;
}

DEFINE_INTERFACE_DISPATCHER(crc32_iscsi)
{
#if HAVE_ZBC && HAVE_ZBB && HAVE_ZVBC
        if (CHECK_RISCV_EXTENSIONS("ZVBC", "ZBC", "ZBB")) {
                return crc32_iscsi_refl_vclmul;
        }
#endif
        return crc32_iscsi_base;
}

DEFINE_INTERFACE_DISPATCHER(crc32_gzip_refl)
{
#if HAVE_ZBC && HAVE_ZBB && HAVE_ZVBC
        if (CHECK_RISCV_EXTENSIONS("ZVBC", "ZBC", "ZBB")) {
                return crc32_gzip_refl_vclmul;
        }
#endif
        return crc32_gzip_refl_base;
}

DEFINE_INTERFACE_DISPATCHER(crc64_ecma_refl)
{
#if HAVE_ZBC && HAVE_ZBB && HAVE_ZVBC
        if (CHECK_RISCV_EXTENSIONS("ZVBC", "ZBC", "ZBB")) {
                return crc64_ecma_refl_vclmul;
        }
#endif
        return crc64_ecma_refl_base;
}

DEFINE_INTERFACE_DISPATCHER(crc64_ecma_norm)
{
#if HAVE_ZBC && HAVE_ZBB && HAVE_ZVBC
        if (CHECK_RISCV_EXTENSIONS("ZVBC", "ZBC", "ZBB")) {
                return crc64_ecma_norm_vclmul;
        }
#endif
        return crc64_ecma_norm_base;
}

DEFINE_INTERFACE_DISPATCHER(crc64_iso_refl)
{
#if HAVE_ZBC && HAVE_ZBB && HAVE_ZVBC
        if (CHECK_RISCV_EXTENSIONS("ZVBC", "ZBC", "ZBB")) {
                return crc64_iso_refl_vclmul;
        }
#endif
        return crc64_iso_refl_base;
}

DEFINE_INTERFACE_DISPATCHER(crc64_iso_norm)
{
#if HAVE_ZBC && HAVE_ZBB && HAVE_ZVBC
        if (CHECK_RISCV_EXTENSIONS("ZVBC", "ZBC", "ZBB")) {
                return crc64_iso_norm_vclmul;
        }
#endif
        return crc64_iso_norm_base;
}

DEFINE_INTERFACE_DISPATCHER(crc64_jones_refl)
{
#if HAVE_ZBC && HAVE_ZBB && HAVE_ZVBC
        if (CHECK_RISCV_EXTENSIONS("ZVBC", "ZBC", "ZBB")) {
                return crc64_jones_refl_vclmul;
        }
#endif
        return crc64_jones_refl_base;
}

DEFINE_INTERFACE_DISPATCHER(crc64_jones_norm)
{
#if HAVE_ZBC && HAVE_ZBB && HAVE_ZVBC
        if (CHECK_RISCV_EXTENSIONS("ZVBC", "ZBC", "ZBB")) {
                return crc64_jones_norm_vclmul;
        }
#endif
        return crc64_jones_norm_base;
}

DEFINE_INTERFACE_DISPATCHER(crc64_rocksoft_refl)
{
#if HAVE_ZBC && HAVE_ZBB && HAVE_ZVBC
        if (CHECK_RISCV_EXTENSIONS("ZVBC", "ZBC", "ZBB")) {
                return crc64_rocksoft_refl_vclmul;
        }
#endif
        return crc64_rocksoft_refl_base;
}

DEFINE_INTERFACE_DISPATCHER(crc64_rocksoft_norm)
{
#if HAVE_ZBC && HAVE_ZBB && HAVE_ZVBC
        if (CHECK_RISCV_EXTENSIONS("ZVBC", "ZBC", "ZBB")) {
                return crc64_rocksoft_norm_vclmul;
        }
#endif
        return crc64_rocksoft_norm_base;
}
