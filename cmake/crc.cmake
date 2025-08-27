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

# CRC module CMake configuration
set(CRC_BASE_SOURCES
    crc/crc_base.c
    crc/crc64_base.c
)

set(CRC_BASE_ALIASES_SOURCES
    crc/crc_base_aliases.c
)

set(CRC_X86_64_SOURCES
    crc/crc16_t10dif_01.asm
    crc/crc16_t10dif_by4.asm
    crc/crc16_t10dif_02.asm
    crc/crc16_t10dif_by16_10.asm
    crc/crc16_t10dif_copy_by4.asm
    crc/crc16_t10dif_copy_by4_02.asm
    crc/crc32_ieee_01.asm
    crc/crc32_ieee_02.asm
    crc/crc32_ieee_by4.asm
    crc/crc32_ieee_by16_10.asm
    crc/crc32_iscsi_01.asm
    crc/crc32_iscsi_00.asm
    crc/crc32_iscsi_by16_10.asm
    crc/crc_multibinary.asm
    crc/crc64_multibinary.asm
    crc/crc64_ecma_refl_by8.asm
    crc/crc64_ecma_refl_by16_10.asm
    crc/crc64_ecma_norm_by8.asm
    crc/crc64_ecma_norm_by16_10.asm
    crc/crc64_iso_refl_by8.asm
    crc/crc64_iso_refl_by16_10.asm
    crc/crc64_iso_norm_by8.asm
    crc/crc64_iso_norm_by16_10.asm
    crc/crc64_jones_refl_by8.asm
    crc/crc64_jones_refl_by16_10.asm
    crc/crc64_jones_norm_by8.asm
    crc/crc64_jones_norm_by16_10.asm
    crc/crc64_rocksoft_refl_by8.asm
    crc/crc64_rocksoft_refl_by16_10.asm
    crc/crc64_rocksoft_norm_by8.asm
    crc/crc64_rocksoft_norm_by16_10.asm
    crc/crc32_gzip_refl_by8.asm
    crc/crc32_gzip_refl_by8_02.asm
    crc/crc32_gzip_refl_by16_10.asm
)

set(CRC_AARCH64_SOURCES
    crc/aarch64/crc_multibinary_arm.S
    crc/aarch64/crc_aarch64_dispatcher.c
    crc/aarch64/crc16_t10dif_pmull.S
    crc/aarch64/crc16_t10dif_copy_pmull.S
    crc/aarch64/crc32_ieee_norm_pmull.S
    crc/aarch64/crc64_ecma_refl_pmull.S
    crc/aarch64/crc64_ecma_norm_pmull.S
    crc/aarch64/crc64_iso_refl_pmull.S
    crc/aarch64/crc64_iso_norm_pmull.S
    crc/aarch64/crc64_jones_refl_pmull.S
    crc/aarch64/crc64_jones_norm_pmull.S
    crc/aarch64/crc64_rocksoft_refl_pmull.S
    crc/aarch64/crc64_rocksoft_norm_pmull.S
    crc/aarch64/crc32_iscsi_refl_pmull.S
    crc/aarch64/crc32_gzip_refl_pmull.S
    crc/aarch64/crc32_iscsi_3crc_fold.S
    crc/aarch64/crc32_gzip_refl_3crc_fold.S
    crc/aarch64/crc32_iscsi_crc_ext.S
    crc/aarch64/crc32_gzip_refl_crc_ext.S
    crc/aarch64/crc32_mix_default.S
    crc/aarch64/crc32c_mix_default.S
    crc/aarch64/crc32_mix_neoverse_n1.S
    crc/aarch64/crc32c_mix_neoverse_n1.S
)

# Build source list based on architecture
set(CRC_SOURCES ${CRC_BASE_SOURCES})

if(CPU_X86_64)
    list(APPEND CRC_SOURCES ${CRC_X86_64_SOURCES})
elseif(CPU_AARCH64)
    list(APPEND CRC_SOURCES ${CRC_AARCH64_SOURCES})
elseif(CPU_PPC64LE OR CPU_RISCV64 OR CPU_UNDEFINED)
    # These architectures use base aliases
    list(APPEND CRC_SOURCES ${CRC_BASE_ALIASES_SOURCES})
endif()

# Headers exported by CRC module
set(CRC_HEADERS
    include/crc.h
    include/crc64.h
)

# Add to main extern headers list
list(APPEND EXTERN_HEADERS ${CRC_HEADERS})

# Add test applications for crc module
if(BUILD_TESTS)
    # Check tests (unit tests that are run by CTest)
    set(CRC_CHECK_TESTS
        crc16_t10dif_test
        crc16_t10dif_copy_test
        crc64_funcs_test
        crc32_funcs_test
    )

    # Create check test executables
    foreach(test ${CRC_CHECK_TESTS})
        add_executable(${test} crc/${test}.c)
        target_link_libraries(${test} PRIVATE isal)
        target_include_directories(${test} PRIVATE include)
        add_test(NAME ${test} COMMAND ${test})
    endforeach()
endif()
