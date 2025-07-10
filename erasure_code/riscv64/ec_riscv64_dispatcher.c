/**************************************************************
  Copyright (c) 2025 sanechips Technologies Co., Ltd.

  Redistribution and use in source and binary forms, with or without
  modification, are permitted provided that the following conditions
  are met:
    * Redistributions of source code must retain the above copyright
      notice, this list of conditions and the following disclaimer.
    * Redistributions in binary form must reproduce the above copyright
      notice, this list of conditions and the following disclaimer in
      the documentation and/or other materials provided with the
      distribution.
    * Neither the name of sanechips Corporation nor the names of its
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

extern void
gf_vect_dot_prod_rvv(int len, int vlen, unsigned char *gftbls, unsigned char **src,
                     unsigned char *dest);
extern void
gf_vect_dot_prod_base(int len, int vlen, unsigned char *v, unsigned char **src,
                      unsigned char *dest);
extern void
gf_vect_mad_rvv(int len, int vec, int vec_i, unsigned char *gftbls, unsigned char *src,
                unsigned char *dest);
extern void
gf_vect_mad_base(int len, int vec, int vec_i, unsigned char *v, unsigned char *src,
                 unsigned char *dest);
extern void
ec_encode_data_rvv(int len, int k, int rows, int vec_i, unsigned char *g_tbls, unsigned char *data,
                   unsigned char **coding);
extern void
ec_encode_data_base(int len, int srcs, int dests, unsigned char *v, unsigned char **src,
                    unsigned char **dest);
extern int
gf_vect_mul_rvv(int len, unsigned char *a, unsigned char *src, unsigned char *dest);
extern int
gf_vect_mul_base(int len, unsigned char *a, unsigned char *src, unsigned char *dest);
extern void
ec_encode_data_update_rvv(int len, int k, int rows, int vec_i, unsigned char *g_tbls,
                          unsigned char *data, unsigned char **coding);
extern void
ec_encode_data_update_base(int len, int srcs, int dests, unsigned char *v, unsigned char **src,
                           unsigned char **dest);
extern void
ec_init_tables_base(int k, int rows, unsigned char *a, unsigned char *g_tbls);

DEFINE_INTERFACE_DISPATCHER(gf_vect_dot_prod)
{
#if HAVE_RVV
        unsigned long auxval = getauxval(AT_HWCAP);

        if (auxval & HWCAP_RV('V'))
                return gf_vect_dot_prod_rvv;
#endif
        return gf_vect_dot_prod_base;
}

DEFINE_INTERFACE_DISPATCHER(gf_vect_mad)
{
#if HAVE_RVV
        unsigned long auxval = getauxval(AT_HWCAP);

        if (auxval & HWCAP_RV('V'))
                return gf_vect_mad_rvv;
#endif
        return gf_vect_mad_base;
}

DEFINE_INTERFACE_DISPATCHER(ec_encode_data)
{
#if HAVE_RVV
        unsigned long auxval = getauxval(AT_HWCAP);

        if (auxval & HWCAP_RV('V'))
                return ec_encode_data_rvv;
#endif
        return ec_encode_data_base;
}

DEFINE_INTERFACE_DISPATCHER(ec_encode_data_update)
{
#if HAVE_RVV
        unsigned long auxval = getauxval(AT_HWCAP);

        if (auxval & HWCAP_RV('V'))
                return ec_encode_data_update_rvv;
#endif
        return ec_encode_data_update_base;
}

DEFINE_INTERFACE_DISPATCHER(gf_vect_mul)
{
#if HAVE_RVV
        unsigned long auxval = getauxval(AT_HWCAP);

        if (auxval & HWCAP_RV('V'))
                return gf_vect_mul_rvv;
#endif
        return gf_vect_mul_base;
}

DEFINE_INTERFACE_DISPATCHER(ec_init_tables) { return ec_init_tables_base; }
