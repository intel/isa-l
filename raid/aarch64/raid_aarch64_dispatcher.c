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

DEFINE_INTERFACE_DISPATCHER(xor_gen)
{
#if defined(__linux__)
	if (getauxval(AT_HWCAP) & HWCAP_ASIMD)
		return PROVIDER_INFO(xor_gen_neon);
#elif defined(__APPLE__)
	return PROVIDER_INFO(xor_gen_neon);
#endif
	return PROVIDER_BASIC(xor_gen);

}

DEFINE_INTERFACE_DISPATCHER(xor_check)
{
#if defined(__linux__)
	if (getauxval(AT_HWCAP) & HWCAP_ASIMD)
		return PROVIDER_INFO(xor_check_neon);
#elif defined(__APPLE__)
	return PROVIDER_INFO(xor_check_neon);
#endif
	return PROVIDER_BASIC(xor_check);

}

DEFINE_INTERFACE_DISPATCHER(pq_gen)
{
#if defined(__linux__)
	if (getauxval(AT_HWCAP) & HWCAP_ASIMD)
		return PROVIDER_INFO(pq_gen_neon);
#elif defined(__APPLE__)
	return PROVIDER_INFO(pq_gen_neon);
#endif
	return PROVIDER_BASIC(pq_gen);

}

DEFINE_INTERFACE_DISPATCHER(pq_check)
{
#if defined(__linux__)
	if (getauxval(AT_HWCAP) & HWCAP_ASIMD)
		return PROVIDER_INFO(pq_check_neon);
#elif defined(__APPLE__)
	return PROVIDER_INFO(pq_check_neon);
#endif
	return PROVIDER_BASIC(pq_check);

}
