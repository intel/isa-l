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

DEFINE_INTERFACE_DISPATCHER(crc16_t10dif)
{
	unsigned long auxval = getauxval(AT_HWCAP);
	if (auxval & HWCAP_PMULL)
		return PROVIDER_INFO(crc16_t10dif_pmull);

	return PROVIDER_BASIC(crc16_t10dif);

}

DEFINE_INTERFACE_DISPATCHER(crc16_t10dif_copy)
{
	unsigned long auxval = getauxval(AT_HWCAP);
	if (auxval & HWCAP_PMULL)
		return PROVIDER_INFO(crc16_t10dif_copy_pmull);

	return PROVIDER_BASIC(crc16_t10dif_copy);

}

DEFINE_INTERFACE_DISPATCHER(crc32_ieee)
{
	unsigned long auxval = getauxval(AT_HWCAP);
	if (auxval & HWCAP_PMULL) {
		return PROVIDER_INFO(crc32_ieee_norm_pmull);
	}

	return PROVIDER_BASIC(crc32_ieee);

}

DEFINE_INTERFACE_DISPATCHER(crc32_iscsi)
{
	unsigned long auxval = getauxval(AT_HWCAP);
	if (auxval & HWCAP_CRC32) {
		switch (get_micro_arch_id()) {
		case MICRO_ARCH_ID(ARM, NEOVERSE_N1):
		case MICRO_ARCH_ID(ARM, CORTEX_A57):
		case MICRO_ARCH_ID(ARM, CORTEX_A72):
			return PROVIDER_INFO(crc32_iscsi_crc_ext);
		}
	}
	if ((HWCAP_CRC32 | HWCAP_PMULL) == (auxval & (HWCAP_CRC32 | HWCAP_PMULL))) {
		return PROVIDER_INFO(crc32_iscsi_3crc_fold);
	}

	if (auxval & HWCAP_PMULL) {
		return PROVIDER_INFO(crc32_iscsi_refl_pmull);
	}
	return PROVIDER_BASIC(crc32_iscsi);

}

DEFINE_INTERFACE_DISPATCHER(crc32_gzip_refl)
{
	unsigned long auxval = getauxval(AT_HWCAP);

	if (auxval & HWCAP_CRC32) {
		switch (get_micro_arch_id()) {
		case MICRO_ARCH_ID(ARM, NEOVERSE_N1):
		case MICRO_ARCH_ID(ARM, CORTEX_A57):
		case MICRO_ARCH_ID(ARM, CORTEX_A72):
			return PROVIDER_INFO(crc32_gzip_refl_crc_ext);
		}
	}
	if ((HWCAP_CRC32 | HWCAP_PMULL) == (auxval & (HWCAP_CRC32 | HWCAP_PMULL))) {
		return PROVIDER_INFO(crc32_gzip_refl_3crc_fold);
	}

	if (auxval & HWCAP_PMULL)
		return PROVIDER_INFO(crc32_gzip_refl_pmull);

	return PROVIDER_BASIC(crc32_gzip_refl);

}

DEFINE_INTERFACE_DISPATCHER(crc64_ecma_refl)
{
	unsigned long auxval = getauxval(AT_HWCAP);

	if (auxval & HWCAP_PMULL)
		return PROVIDER_INFO(crc64_ecma_refl_pmull);

	return PROVIDER_BASIC(crc64_ecma_refl);

}

DEFINE_INTERFACE_DISPATCHER(crc64_ecma_norm)
{
	unsigned long auxval = getauxval(AT_HWCAP);
	if (auxval & HWCAP_PMULL)
		return PROVIDER_INFO(crc64_ecma_norm_pmull);

	return PROVIDER_BASIC(crc64_ecma_norm);

}

DEFINE_INTERFACE_DISPATCHER(crc64_iso_refl)
{
	unsigned long auxval = getauxval(AT_HWCAP);
	if (auxval & HWCAP_PMULL)
		return PROVIDER_INFO(crc64_iso_refl_pmull);

	return PROVIDER_BASIC(crc64_iso_refl);

}

DEFINE_INTERFACE_DISPATCHER(crc64_iso_norm)
{
	unsigned long auxval = getauxval(AT_HWCAP);
	if (auxval & HWCAP_PMULL)
		return PROVIDER_INFO(crc64_iso_norm_pmull);

	return PROVIDER_BASIC(crc64_iso_norm);

}

DEFINE_INTERFACE_DISPATCHER(crc64_jones_refl)
{
	unsigned long auxval = getauxval(AT_HWCAP);
	if (auxval & HWCAP_PMULL)
		return PROVIDER_INFO(crc64_jones_refl_pmull);

	return PROVIDER_BASIC(crc64_jones_refl);

}

DEFINE_INTERFACE_DISPATCHER(crc64_jones_norm)
{
	unsigned long auxval = getauxval(AT_HWCAP);
	if (auxval & HWCAP_PMULL)
		return PROVIDER_INFO(crc64_jones_norm_pmull);

	return PROVIDER_BASIC(crc64_jones_norm);

}
