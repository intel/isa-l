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

lsrc        += 	igzip/igzip.c \
		igzip/hufftables_c.c \
		igzip/igzip_base.c \
		igzip/igzip_icf_base.c \
		igzip/crc32_gzip_base.c \
		igzip/flatten_ll.c \
		igzip/encode_df.c \
		igzip/igzip_icf_body.c

lsrc_base_aliases += igzip/igzip_base_aliases.c igzip/proc_heap_base.c
lsrc_x86_32       += igzip/igzip_base_aliases.c igzip/proc_heap_base.c

lsrc_x86_64 +=	igzip/igzip_body.asm \
		igzip/igzip_finish.asm \
		igzip/igzip_icf_body_h1_gr_bt.asm \
		igzip/igzip_icf_finish.asm \
		igzip/rfc1951_lookup.asm \
		igzip/crc32_gzip.asm \
		igzip/adler32_sse.asm \
		igzip/adler32_avx2_4.asm \
		igzip/igzip_multibinary.asm \
		igzip/igzip_update_histogram_01.asm \
		igzip/igzip_update_histogram_04.asm \
		igzip/igzip_decode_block_stateless_01.asm \
		igzip/igzip_decode_block_stateless_04.asm \
		igzip/igzip_inflate_multibinary.asm \
		igzip/encode_df_04.asm \
		igzip/encode_df_06.asm \
		igzip/proc_heap.asm \
		igzip/igzip_deflate_hash.asm \
		igzip/igzip_gen_icf_map_lh1_06.asm \
		igzip/igzip_gen_icf_map_lh1_04.asm \
		igzip/igzip_set_long_icf_fg_04.asm \
		igzip/igzip_set_long_icf_fg_06.asm

src_include += -I $(srcdir)/igzip
extern_hdrs +=	include/igzip_lib.h

check_tests +=  igzip/igzip_rand_test
unit_tests  +=  igzip/checksum32_funcs_test

perf_tests  +=  igzip/igzip_perf

other_tests +=  igzip/igzip_file_perf igzip/igzip_stateless_file_perf igzip/igzip_hist_perf
other_tests +=  igzip/igzip_semi_dyn_file_perf
other_tests +=  igzip/igzip_build_hash_table_perf

other_src   += 	igzip/bitbuf2.asm  \
		igzip/data_struct2.asm \
		igzip/inflate_data_structs.asm \
		igzip/igzip_body.asm \
		igzip/igzip_finish.asm \
		igzip/lz0a_const.asm \
		igzip/options.asm \
		igzip/stdmac.asm \
		igzip/igzip_compare_types.asm \
		igzip/bitbuf2.h \
		igzip/repeated_char_result.h \
		igzip/igzip_update_histogram.asm \
		igzip/huffman.asm \
		include/reg_sizes.asm \
		include/multibinary.asm \
		include/test.h \
		igzip/huffman.h \
		igzip/igzip_level_buf_structs.h \
		igzip/igzip_decode_block_stateless.asm \
		igzip/inflate_std_vects.h \
		igzip/flatten_ll.h \
		igzip/encode_df.h \
		igzip/heap_macros.asm \
		igzip/igzip_checksums.h

examples    += 	igzip/igzip_example igzip/igzip_sync_flush_example

igzip_igzip_rand_test_LDADD = libisal.la

# Include tools to make custom Huffman tables based on sample data
other_tests += igzip/generate_custom_hufftables
other_src   += igzip/huff_codes.h
lsrc        += igzip/huff_codes.c

# Include tools and tests using the reference inflate
other_tests += igzip/igzip_inflate_perf
other_tests += igzip/igzip_inflate_test
lsrc   += igzip/igzip_inflate.c
other_src   += igzip/checksum_test_ref.h

igzip_inflate_perf: LDLIBS += -lz
igzip_igzip_inflate_perf_LDADD = libisal.la
igzip_igzip_inflate_perf_LDFLAGS = -lz
igzip_inflate_test: LDLIBS += -lz
igzip_igzip_inflate_test_LDADD = libisal.la
igzip_igzip_inflate_test_LDFLAGS = -lz
igzip_igzip_hist_perf_LDADD = libisal.la
