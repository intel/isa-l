########################################################################
#  Copyright(c) 2011-2017 Intel Corporation All rights reserved.
#
#  Redistribution and use in source and binary forms, with or without
#  modification, are permitted provided that the following conditions
#  are met:
#    * Redistributions of source code must retain the above copyright
#      notice, this list of conditions and the following disclaimer.
#    * Redistributions in binary form must reproduce the above copyright
#      notice, this list of conditions and the following disclaimer in
#      the documentation and/or other materials provided with the
#      distribution.
#    * Neither the name of Intel Corporation nor the names of its
#      contributors may be used to endorse or promote products derived
#      from this software without specific prior written permission.
#
#  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
#  "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
#  LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
#  A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
#  OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
#  SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
#  LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
#  DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
#  THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
#  (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
#  OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
########################################################################

include crc/aarch64/Makefile.am

lsrc  += \
	crc/crc_base.c \
	crc/crc64_base.c

lsrc_base_aliases += crc/crc_base_aliases.c
lsrc_x86_32       += crc/crc_base_aliases.c
lsrc_ppc64le      += crc/crc_base_aliases.c

lsrc_x86_64 += \
	crc/crc16_t10dif_01.asm \
	crc/crc16_t10dif_by4.asm \
	crc/crc16_t10dif_02.asm \
	crc/crc16_t10dif_by16_10.asm \
	crc/crc16_t10dif_copy_by4.asm \
	crc/crc16_t10dif_copy_by4_02.asm \
	crc/crc32_ieee_01.asm \
	crc/crc32_ieee_02.asm \
	crc/crc32_ieee_by4.asm \
	crc/crc32_ieee_by16_10.asm \
	crc/crc32_iscsi_01.asm \
	crc/crc32_iscsi_00.asm \
	crc/crc32_iscsi_by16_10.asm \
	crc/crc_multibinary.asm \
	crc/crc64_multibinary.asm \
	crc/crc64_ecma_refl_by8.asm \
	crc/crc64_ecma_refl_by16_10.asm \
	crc/crc64_ecma_norm_by8.asm \
	crc/crc64_ecma_norm_by16_10.asm \
	crc/crc64_iso_refl_by8.asm \
	crc/crc64_iso_refl_by16_10.asm \
	crc/crc64_iso_norm_by8.asm \
	crc/crc64_iso_norm_by16_10.asm \
	crc/crc64_jones_refl_by8.asm \
	crc/crc64_jones_refl_by16_10.asm \
	crc/crc64_jones_norm_by8.asm \
	crc/crc64_jones_norm_by16_10.asm \
	crc/crc32_gzip_refl_by8.asm \
	crc/crc32_gzip_refl_by8_02.asm \
	crc/crc32_gzip_refl_by16_10.asm

src_include += -I $(srcdir)/crc
extern_hdrs +=  include/crc.h include/crc64.h

other_src   +=  include/reg_sizes.asm include/types.h include/test.h \
		crc/crc_ref.h crc/crc64_ref.h

check_tests +=  crc/crc16_t10dif_test \
		crc/crc16_t10dif_copy_test \
		crc/crc64_funcs_test \
		crc/crc32_funcs_test

perf_tests  +=  crc/crc16_t10dif_perf crc/crc16_t10dif_copy_perf \
		crc/crc16_t10dif_op_perf \
		crc/crc32_ieee_perf crc/crc32_iscsi_perf \
		crc/crc64_funcs_perf crc/crc32_gzip_refl_perf

examples    +=  crc/crc_simple_test crc/crc64_example

