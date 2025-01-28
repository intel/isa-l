/**************************************************************
  Copyright (c) 2019 Huawei Technologies Co., Ltd.

  Redistribution and use in source and binary forms, with or without
  modification, are permitted provided that the following conditions
  are met:
    * Redistributions of source code must retain the above copyright
      notice, this list of conditions and the following disclaimer.
    * Redistributions in binary form must reproduce the above copyright
      notice, this list of conditions and the following disclaimer in
      the documentation and/or other materials provided with the
      distribution.
    * Neither the name of Huawei Corporation nor the names of its
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
#include "erasure_code.h"
#include "gf_vect_mul.h"

extern void
gf_vect_dot_prod_sve(int, int, unsigned char *, unsigned char **, unsigned char *);
extern void
gf_vect_dot_prod_neon(int, int, unsigned char *, unsigned char **, unsigned char *);

extern void
gf_vect_mad_sve(int, int, int, unsigned char *, unsigned char *, unsigned char *);
extern void
gf_vect_mad_neon(int, int, int, unsigned char *, unsigned char *, unsigned char *);

extern void
ec_encode_data_sve(int, int, int, unsigned char *, unsigned char **, unsigned char **coding);
extern void
ec_encode_data_neon(int, int, int, unsigned char *, unsigned char **, unsigned char **);

extern void
ec_encode_data_update_sve(int, int, int, int, unsigned char *, unsigned char *, unsigned char **);
extern void
ec_encode_data_update_neon(int, int, int, int, unsigned char *, unsigned char *, unsigned char **);

extern int
gf_vect_mul_sve(int, unsigned char *, unsigned char *, unsigned char *);
extern int
gf_vect_mul_neon(int, unsigned char *, unsigned char *, unsigned char *);

DEFINE_INTERFACE_DISPATCHER(gf_vect_dot_prod)
{
#if defined(__linux__)
        unsigned long auxval = getauxval(AT_HWCAP);

        if (auxval & HWCAP_SVE)
                return gf_vect_dot_prod_sve;
        if (auxval & HWCAP_ASIMD)
                return gf_vect_dot_prod_neon;
#elif defined(__APPLE__)
        if (sysctlEnabled(SYSCTL_SVE_KEY))
                return gf_vect_dot_prod_sve;
        return gf_vect_dot_prod_neon;
#endif
        return gf_vect_dot_prod_base;
}

DEFINE_INTERFACE_DISPATCHER(gf_vect_mad)
{
#if defined(__linux__)
        unsigned long auxval = getauxval(AT_HWCAP);

        if (auxval & HWCAP_SVE)
                return gf_vect_mad_sve;
        if (auxval & HWCAP_ASIMD)
                return gf_vect_mad_neon;
#elif defined(__APPLE__)
        if (sysctlEnabled(SYSCTL_SVE_KEY))
                return gf_vect_mad_sve;
        return gf_vect_mad_neon;
#endif
        return gf_vect_mad_base;
}

DEFINE_INTERFACE_DISPATCHER(ec_encode_data)
{
#if defined(__linux__)
        unsigned long auxval = getauxval(AT_HWCAP);

        if (auxval & HWCAP_SVE)
                return ec_encode_data_sve;
        if (auxval & HWCAP_ASIMD)
                return ec_encode_data_neon;
#elif defined(__APPLE__)
        if (sysctlEnabled(SYSCTL_SVE_KEY))
                return ec_encode_data_sve;
        return ec_encode_data_neon;
#endif
        return ec_encode_data_base;
}

DEFINE_INTERFACE_DISPATCHER(ec_encode_data_update)
{
#if defined(__linux__)
        unsigned long auxval = getauxval(AT_HWCAP);

        if (auxval & HWCAP_SVE)
                return ec_encode_data_update_sve;
        if (auxval & HWCAP_ASIMD)
                return ec_encode_data_update_neon;
#elif defined(__APPLE__)
        if (sysctlEnabled(SYSCTL_SVE_KEY))
                return ec_encode_data_update_sve;
        return ec_encode_data_update_neon;
#endif
        return ec_encode_data_update_base;
}

DEFINE_INTERFACE_DISPATCHER(gf_vect_mul)
{
#if defined(__linux__)
        unsigned long auxval = getauxval(AT_HWCAP);

        if (auxval & HWCAP_SVE)
                return gf_vect_mul_sve;
        if (auxval & HWCAP_ASIMD)
                return gf_vect_mul_neon;
#elif defined(__APPLE__)
        if (sysctlEnabled(SYSCTL_SVE_KEY))
                return gf_vect_mul_sve;
        return gf_vect_mul_neon;
#endif
        return gf_vect_mul_base;
}

DEFINE_INTERFACE_DISPATCHER(ec_init_tables) { return ec_init_tables_base; }
