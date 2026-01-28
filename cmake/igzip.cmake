# cmake-format: off
# Copyright (c) 2025, Intel Corporation
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are met:
#
#     * Redistributions of source code must retain the above copyright notice,
#       this list of conditions and the following disclaimer.
#     * Redistributions in binary form must reproduce the above copyright
#       notice, this list of conditions and the following disclaimer in the
#       documentation and/or other materials provided with the distribution.
#     * Neither the name of Intel Corporation nor the names of its contributors
#       may be used to endorse or promote products derived from this software
#       without specific prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
# AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
# IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
# DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE
# FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
# DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
# SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
# CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
# OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
# OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
# cmake-format: on

# IGZIP module CMake configuration

set(IGZIP_BASE_SOURCES
    igzip/igzip.c
    igzip/hufftables_c.c
    igzip/igzip_base.c
    igzip/igzip_icf_base.c
    igzip/adler32_base.c
    igzip/flatten_ll.c
    igzip/encode_df.c
    igzip/igzip_icf_body.c
    igzip/huff_codes.c
    igzip/igzip_inflate.c
)

set(IGZIP_BASE_ALIASES_SOURCES
    igzip/igzip_base_aliases.c
    igzip/proc_heap_base.c
)

set(IGZIP_X86_64_SOURCES
    igzip/igzip_body.asm
    igzip/igzip_finish.asm
    igzip/igzip_icf_body_h1_gr_bt.asm
    igzip/igzip_icf_finish.asm
    igzip/rfc1951_lookup.asm
    igzip/adler32_sse.asm
    igzip/adler32_avx2_4.asm
    igzip/igzip_multibinary.asm
    igzip/igzip_update_histogram_01.asm
    igzip/igzip_update_histogram_04.asm
    igzip/igzip_decode_block_stateless_01.asm
    igzip/igzip_decode_block_stateless_04.asm
    igzip/igzip_inflate_multibinary.asm
    igzip/encode_df_04.asm
    igzip/encode_df_06.asm
    igzip/proc_heap.asm
    igzip/igzip_deflate_hash.asm
    igzip/igzip_gen_icf_map_lh1_06.asm
    igzip/igzip_gen_icf_map_lh1_04.asm
    igzip/igzip_set_long_icf_fg_04.asm
    igzip/igzip_set_long_icf_fg_06.asm
)

set(IGZIP_AARCH64_SOURCES
    igzip/aarch64/igzip_inflate_multibinary_arm64.S
    igzip/aarch64/igzip_multibinary_arm64.S
    igzip/aarch64/igzip_isal_adler32_neon.S
    igzip/aarch64/igzip_multibinary_aarch64_dispatcher.c
    igzip/aarch64/igzip_deflate_body_aarch64.S
    igzip/aarch64/igzip_deflate_finish_aarch64.S
    igzip/aarch64/isal_deflate_icf_body_hash_hist.S
    igzip/aarch64/isal_deflate_icf_finish_hash_hist.S
    igzip/aarch64/igzip_set_long_icf_fg.S
    igzip/aarch64/encode_df.S
    igzip/aarch64/isal_update_histogram.S
    igzip/aarch64/gen_icf_map.S
    igzip/aarch64/igzip_deflate_hash_aarch64.S
    igzip/aarch64/igzip_decode_huffman_code_block_aarch64.S
)

set(IGZIP_RISCV64_SOURCES
    igzip/riscv64/igzip_multibinary_riscv64_dispatcher.c
    igzip/riscv64/igzip_multibinary_riscv64.S
    igzip/riscv64/igzip_isal_adler32_rvv.S
)

# Build source list based on architecture
set(IGZIP_SOURCES ${IGZIP_BASE_SOURCES})

if(CPU_X86_64)
    list(APPEND IGZIP_SOURCES ${IGZIP_X86_64_SOURCES})
elseif(CPU_AARCH64)
    list(APPEND IGZIP_SOURCES ${IGZIP_AARCH64_SOURCES})
    # AArch64 lacks assembly implementation for heap operations, use C fallback
    list(APPEND IGZIP_SOURCES igzip/proc_heap_base.c)
elseif(CPU_PPC64LE)
    # PPC64LE uses base aliases
    list(APPEND IGZIP_SOURCES ${IGZIP_BASE_ALIASES_SOURCES})
elseif(CPU_RISCV64)
    list(APPEND IGZIP_SOURCES ${IGZIP_RISCV64_SOURCES})
    # RISC-V lacks assembly implementation for heap operations, use C fallback
    list(APPEND IGZIP_SOURCES igzip/proc_heap_base.c)
elseif(CPU_UNDEFINED)
    list(APPEND IGZIP_SOURCES ${IGZIP_BASE_ALIASES_SOURCES})
endif()

# Headers exported by IGZIP module
set(IGZIP_HEADERS
    include/igzip_lib.h
)

# Add to main extern headers list
list(APPEND EXTERN_HEADERS ${IGZIP_HEADERS})

# Add test applications for igzip module
if(BUILD_TESTS)
    # Check tests (unit tests that are run by CTest)
    set(IGZIP_CHECK_TESTS
        igzip_rand_test
        igzip_wrapper_hdr_test
        checksum32_funcs_test
    )

    # Create check test executables
    foreach(test ${IGZIP_CHECK_TESTS})
        add_executable(${test} igzip/${test}.c)
        target_link_libraries(${test} PRIVATE isal)
        target_include_directories(${test} PRIVATE include igzip)
        add_test(NAME ${test} COMMAND ${test})
    endforeach()
endif()
