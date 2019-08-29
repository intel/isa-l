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

DEFINE_INTERFACE_DISPATCHER(isal_adler32)
{
	unsigned long auxval = getauxval(AT_HWCAP);
	if (auxval & HWCAP_ASIMD)
		return PROVIDER_INFO(adler32_neon);

	return PROVIDER_BASIC(adler32);

}

DEFINE_INTERFACE_DISPATCHER(isal_deflate_body)
{
	unsigned long auxval = getauxval(AT_HWCAP);

	if (auxval & HWCAP_CRC32)
		return PROVIDER_INFO(isal_deflate_body_aarch64);

	return PROVIDER_BASIC(isal_deflate_body);

}

DEFINE_INTERFACE_DISPATCHER(isal_deflate_finish)
{
	unsigned long auxval = getauxval(AT_HWCAP);
	if (auxval & HWCAP_CRC32)
		return PROVIDER_INFO(isal_deflate_finish_aarch64);

	return PROVIDER_BASIC(isal_deflate_finish);

}

DEFINE_INTERFACE_DISPATCHER(isal_deflate_icf_body_lvl1)
{
	unsigned long auxval = getauxval(AT_HWCAP);
	if (auxval & HWCAP_CRC32)
		return PROVIDER_INFO(isal_deflate_icf_body_hash_hist_aarch64);

	return PROVIDER_BASIC(isal_deflate_icf_body_hash_hist);
}

DEFINE_INTERFACE_DISPATCHER(isal_deflate_icf_finish_lvl1)
{
	unsigned long auxval = getauxval(AT_HWCAP);
	if (auxval & HWCAP_CRC32)
		return PROVIDER_INFO(isal_deflate_icf_finish_hash_hist_aarch64);

	return PROVIDER_BASIC(isal_deflate_icf_finish_hash_hist);
}

DEFINE_INTERFACE_DISPATCHER(isal_deflate_icf_body_lvl2)
{
	unsigned long auxval = getauxval(AT_HWCAP);
	if (auxval & HWCAP_CRC32)
		return PROVIDER_INFO(isal_deflate_icf_body_hash_hist_aarch64);

	return PROVIDER_BASIC(isal_deflate_icf_body_hash_hist);
}

DEFINE_INTERFACE_DISPATCHER(isal_deflate_icf_finish_lvl2)
{
	unsigned long auxval = getauxval(AT_HWCAP);
	if (auxval & HWCAP_CRC32)
		return PROVIDER_INFO(isal_deflate_icf_finish_hash_hist_aarch64);

	return PROVIDER_BASIC(isal_deflate_icf_finish_hash_hist);
}

DEFINE_INTERFACE_DISPATCHER(isal_deflate_icf_body_lvl3)
{
	unsigned long auxval = getauxval(AT_HWCAP);
	if (auxval & HWCAP_CRC32)
		return PROVIDER_INFO(icf_body_hash1_fillgreedy_lazy);

	return PROVIDER_INFO(icf_body_hash1_fillgreedy_lazy);
}

DEFINE_INTERFACE_DISPATCHER(isal_deflate_icf_finish_lvl3)
{
	unsigned long auxval = getauxval(AT_HWCAP);
	if (auxval & HWCAP_CRC32)
		return PROVIDER_INFO(isal_deflate_icf_finish_hash_map_base);

	return PROVIDER_BASIC(isal_deflate_icf_finish_hash_map);
}

DEFINE_INTERFACE_DISPATCHER(isal_update_histogram)
{
	unsigned long auxval = getauxval(AT_HWCAP);
	if (auxval & HWCAP_CRC32)
		return PROVIDER_INFO(isal_update_histogram_aarch64);

	return PROVIDER_BASIC(isal_update_histogram);
}
