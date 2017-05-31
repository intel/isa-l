########################################################################
#  Copyright(c) 2011-2015 Intel Corporation All rights reserved.
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

lsrc        += 	raid/raid_base.c

lsrc_base_aliases += raid/raid_base_aliases.c

lsrc_x86_64 += \
		raid/xor_gen_sse.asm \
		raid/pq_gen_sse.asm \
		raid/xor_check_sse.asm \
		raid/pq_check_sse.asm \
		raid/pq_gen_avx.asm \
		raid/xor_gen_avx.asm \
		raid/pq_gen_avx2.asm \
		raid/xor_gen_avx512.asm \
		raid/pq_gen_avx512.asm \
		raid/raid_multibinary.asm

lsrc_x86_32 += \
		raid/xor_gen_sse.asm \
		raid/pq_gen_sse_i32.asm \
		raid/xor_check_sse.asm \
		raid/pq_check_sse_i32.asm \
		raid/raid_multibinary_i32.asm


extern_hdrs +=	include/raid.h

other_src   +=  include/test.h include/types.h

check_tests +=  raid/xor_gen_test raid/pq_gen_test raid/xor_check_test raid/pq_check_test

perf_tests  +=  raid/xor_gen_perf raid/pq_gen_perf

examples    += 	raid/xor_example

lsrc32       += xor_gen_sse.asm pq_gen_sse_i32.asm xor_check_sse.asm pq_check_sse_i32.asm raid_base.c
