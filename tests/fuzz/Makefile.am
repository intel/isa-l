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

src_include += -I $(srcdir)/tests/fuzz

# AFL fuzz tests
other_tests += tests/fuzz/igzip_fuzz_inflate
igzip_fuzz_inflate: igzip_checked_inflate_fuzz_test.o
igzip_fuzz_inflate: LDLIBS += -lz
tests_fuzz_igzip_fuzz_inflate_LDADD = tests/fuzz/igzip_checked_inflate_fuzz_test.o libisal.la
tests_fuzz_igzip_fuzz_inflate_LDFLAGS = -lz

other_tests += tests/fuzz/igzip_dump_inflate_corpus
tests_fuzz_igzip_dump_inflate_corpus_LDADD = libisal.la

# LLVM fuzz tests
llvm_fuzz_tests = tests/fuzz/igzip_simple_inflate_fuzz_test
other_src      += tests/fuzz/igzip_simple_inflate_fuzz_test.c

llvm_fuzz_tests += tests/fuzz/igzip_checked_inflate_fuzz_test
other_src       += tests/fuzz/igzip_checked_inflate_fuzz_test.c

llvm_fuzz_tests += tests/fuzz/igzip_simple_round_trip_fuzz_test
other_src       += tests/fuzz/igzip_simple_round_trip_fuzz_test.c

igzip_checked_inflate_fuzz_test: LDLIBS += -lz
