########################################################################
#  Copyright(c) 2011-2016 Intel Corporation All rights reserved.
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

lsrc        += 	igzip/igzip.c igzip/hufftables_c.c \
		igzip/crc_utils_01.asm \
		igzip/crc_utils_04.asm \
		igzip/igzip_body_01.asm igzip/igzip_body_04.asm igzip/igzip_finish.asm \
		igzip/igzip_stateless_01.asm igzip/igzip_stateless_04.asm \
		igzip/crc_data.asm \
		igzip/crc32_gzip.asm igzip/detect_repeated_char.asm \
		igzip/igzip_multibinary.asm \
		igzip/igzip_stateless_base.c \
		igzip/igzip_base.c

extern_hdrs +=	include/igzip_lib.h

pkginclude_HEADERS += include/types.h

unit_tests  += 	igzip/igzip_rand_test

check_tests +=  igzip/igzip_check

perf_tests  +=  igzip/igzip_perf igzip/igzip_sync_flush_perf

other_tests +=  igzip/igzip_file_perf igzip/igzip_sync_flush_file_perf igzip/igzip_stateless_file_perf

other_src   += 	igzip/bitbuf2.asm igzip/data_struct2.asm \
		igzip/igzip_buffer_utils_01.asm \
		igzip/igzip_buffer_utils_04.asm \
		igzip/igzip_body.asm igzip/igzip_finish.asm \
		igzip/lz0a_const.asm igzip/options.asm igzip/stdmac.asm igzip/igzip_compare_types.asm \
		igzip/bitbuf2.h igzip/repeated_char_result.h \
		igzip/igzip_body.asm \
		igzip/igzip_stateless.asm \
		igzip/huffman.asm \
		include/reg_sizes.asm \
		include/multibinary.asm \
		include/test.h \
		igzip/huffman.h


examples    += 	igzip/igzip_example igzip/igzip_sync_flush_example

igzip_rand_test: igzip_inflate_ref.o
igzip_igzip_rand_test_LDADD = igzip/igzip_inflate_ref.lo libisal.la

# Include tools to make custom Huffman tables based on sample data
other_tests += igzip/generate_custom_hufftables
other_tests += igzip/generate_constant_block_header
other_src   += igzip/huff_codes.h
lsrc        += igzip/huff_codes.c

# Include tools and tests using the reference inflate
other_tests += igzip/igzip_inflate_perf
other_tests += igzip/igzip_inflate_test
other_src   += igzip/igzip_inflate_ref.h
other_src   += igzip/igzip_inflate_ref.c
other_src   += igzip/crc_inflate.h

igzip_inflate_perf: igzip_inflate_ref.o
igzip_inflate_perf: LDLIBS += -lz
igzip_igzip_inflate_perf_LDADD = igzip/igzip_inflate_ref.lo libisal.la
igzip_igzip_inflate_perf_LDFLAGS = -lz
igzip_inflate_test: igzip_inflate_ref.o
igzip_inflate_test: LDLIBS += -lz
igzip_igzip_inflate_test_LDADD = igzip/igzip_inflate_ref.lo libisal.la
igzip_igzip_inflate_test_LDFLAGS = -lz
igzip_check: igzip_inflate_ref.o
igzip_igzip_check_LDADD = igzip/igzip_inflate_ref.lo libisal.la
