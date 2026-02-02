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

# Erasure Code module CMake configuration

set(ERASURE_CODE_BASE_SOURCES
    erasure_code/ec_base.c
)

set(ERASURE_CODE_BASE_ALIASES_SOURCES
    erasure_code/ec_base_aliases.c
)

set(ERASURE_CODE_X86_64_SOURCES
    erasure_code/ec_highlevel_func.c
    erasure_code/gf_vect_mul_sse.asm
    erasure_code/gf_vect_mul_avx.asm
    erasure_code/gf_vect_dot_prod_sse.asm
    erasure_code/gf_vect_dot_prod_avx.asm
    erasure_code/gf_vect_dot_prod_avx2.asm
    erasure_code/gf_2vect_dot_prod_sse.asm
    erasure_code/gf_3vect_dot_prod_sse.asm
    erasure_code/gf_4vect_dot_prod_sse.asm
    erasure_code/gf_5vect_dot_prod_sse.asm
    erasure_code/gf_6vect_dot_prod_sse.asm
    erasure_code/gf_2vect_dot_prod_avx.asm
    erasure_code/gf_3vect_dot_prod_avx.asm
    erasure_code/gf_4vect_dot_prod_avx.asm
    erasure_code/gf_5vect_dot_prod_avx.asm
    erasure_code/gf_6vect_dot_prod_avx.asm
    erasure_code/gf_2vect_dot_prod_avx2.asm
    erasure_code/gf_3vect_dot_prod_avx2.asm
    erasure_code/gf_4vect_dot_prod_avx2.asm
    erasure_code/gf_5vect_dot_prod_avx2.asm
    erasure_code/gf_6vect_dot_prod_avx2.asm
    erasure_code/gf_vect_mad_sse.asm
    erasure_code/gf_2vect_mad_sse.asm
    erasure_code/gf_3vect_mad_sse.asm
    erasure_code/gf_4vect_mad_sse.asm
    erasure_code/gf_5vect_mad_sse.asm
    erasure_code/gf_6vect_mad_sse.asm
    erasure_code/gf_vect_mad_avx.asm
    erasure_code/gf_2vect_mad_avx.asm
    erasure_code/gf_3vect_mad_avx.asm
    erasure_code/gf_4vect_mad_avx.asm
    erasure_code/gf_5vect_mad_avx.asm
    erasure_code/gf_6vect_mad_avx.asm
    erasure_code/gf_vect_mad_avx2.asm
    erasure_code/gf_2vect_mad_avx2.asm
    erasure_code/gf_3vect_mad_avx2.asm
    erasure_code/gf_4vect_mad_avx2.asm
    erasure_code/gf_5vect_mad_avx2.asm
    erasure_code/gf_6vect_mad_avx2.asm
    erasure_code/ec_multibinary.asm
    erasure_code/gf_vect_mad_avx2_gfni.asm
    erasure_code/gf_2vect_mad_avx2_gfni.asm
    erasure_code/gf_3vect_mad_avx2_gfni.asm
    erasure_code/gf_4vect_mad_avx2_gfni.asm
    erasure_code/gf_5vect_mad_avx2_gfni.asm
    erasure_code/gf_vect_dot_prod_avx512.asm
    erasure_code/gf_2vect_dot_prod_avx512.asm
    erasure_code/gf_3vect_dot_prod_avx512.asm
    erasure_code/gf_4vect_dot_prod_avx512.asm
    erasure_code/gf_5vect_dot_prod_avx512.asm
    erasure_code/gf_6vect_dot_prod_avx512.asm
    erasure_code/gf_vect_dot_prod_avx512_gfni.asm
    erasure_code/gf_vect_dot_prod_avx2_gfni.asm
    erasure_code/gf_2vect_dot_prod_avx2_gfni.asm
    erasure_code/gf_3vect_dot_prod_avx2_gfni.asm
    erasure_code/gf_2vect_dot_prod_avx512_gfni.asm
    erasure_code/gf_3vect_dot_prod_avx512_gfni.asm
    erasure_code/gf_4vect_dot_prod_avx512_gfni.asm
    erasure_code/gf_5vect_dot_prod_avx512_gfni.asm
    erasure_code/gf_6vect_dot_prod_avx512_gfni.asm
    erasure_code/gf_vect_mad_avx512.asm
    erasure_code/gf_2vect_mad_avx512.asm
    erasure_code/gf_3vect_mad_avx512.asm
    erasure_code/gf_4vect_mad_avx512.asm
    erasure_code/gf_5vect_mad_avx512.asm
    erasure_code/gf_6vect_mad_avx512.asm
    erasure_code/gf_vect_mad_avx512_gfni.asm
    erasure_code/gf_2vect_mad_avx512_gfni.asm
    erasure_code/gf_3vect_mad_avx512_gfni.asm
    erasure_code/gf_4vect_mad_avx512_gfni.asm
    erasure_code/gf_5vect_mad_avx512_gfni.asm
    erasure_code/gf_6vect_mad_avx512_gfni.asm
)

set(ERASURE_CODE_AARCH64_SOURCES
    erasure_code/aarch64/ec_aarch64_highlevel_func.c
    erasure_code/aarch64/ec_aarch64_dispatcher.c
    erasure_code/aarch64/gf_vect_dot_prod_neon.S
    erasure_code/aarch64/gf_2vect_dot_prod_neon.S
    erasure_code/aarch64/gf_3vect_dot_prod_neon.S
    erasure_code/aarch64/gf_4vect_dot_prod_neon.S
    erasure_code/aarch64/gf_5vect_dot_prod_neon.S
    erasure_code/aarch64/gf_vect_mad_neon.S
    erasure_code/aarch64/gf_2vect_mad_neon.S
    erasure_code/aarch64/gf_3vect_mad_neon.S
    erasure_code/aarch64/gf_4vect_mad_neon.S
    erasure_code/aarch64/gf_5vect_mad_neon.S
    erasure_code/aarch64/gf_6vect_mad_neon.S
    erasure_code/aarch64/gf_vect_mul_neon.S
    erasure_code/aarch64/gf_vect_mad_sve.S
    erasure_code/aarch64/gf_2vect_mad_sve.S
    erasure_code/aarch64/gf_3vect_mad_sve.S
    erasure_code/aarch64/gf_4vect_mad_sve.S
    erasure_code/aarch64/gf_5vect_mad_sve.S
    erasure_code/aarch64/gf_6vect_mad_sve.S
    erasure_code/aarch64/gf_vect_dot_prod_sve.S
    erasure_code/aarch64/gf_2vect_dot_prod_sve.S
    erasure_code/aarch64/gf_3vect_dot_prod_sve.S
    erasure_code/aarch64/gf_4vect_dot_prod_sve.S
    erasure_code/aarch64/gf_5vect_dot_prod_sve.S
    erasure_code/aarch64/gf_6vect_dot_prod_sve.S
    erasure_code/aarch64/gf_7vect_dot_prod_sve.S
    erasure_code/aarch64/gf_8vect_dot_prod_sve.S
    erasure_code/aarch64/gf_vect_mul_sve.S
    erasure_code/aarch64/ec_multibinary_arm.S
)

set(ERASURE_CODE_PPC64LE_SOURCES
    erasure_code/ppc64le/ec_base_vsx.c
    erasure_code/ppc64le/gf_vect_mul_vsx.c
    erasure_code/ppc64le/gf_vect_dot_prod_vsx.c
    erasure_code/ppc64le/gf_vect_mad_vsx.c
    erasure_code/ppc64le/gf_2vect_dot_prod_vsx.c
    erasure_code/ppc64le/gf_2vect_mad_vsx.c
    erasure_code/ppc64le/gf_3vect_dot_prod_vsx.c
    erasure_code/ppc64le/gf_3vect_mad_vsx.c
    erasure_code/ppc64le/gf_4vect_dot_prod_vsx.c
    erasure_code/ppc64le/gf_4vect_mad_vsx.c
    erasure_code/ppc64le/gf_5vect_dot_prod_vsx.c
    erasure_code/ppc64le/gf_5vect_mad_vsx.c
    erasure_code/ppc64le/gf_6vect_dot_prod_vsx.c
    erasure_code/ppc64le/gf_6vect_mad_vsx.c
)

set(ERASURE_CODE_RISCV64_SOURCES
    erasure_code/riscv64/ec_multibinary_riscv64_dispatcher.c
    erasure_code/riscv64/ec_multibinary_riscv64.S
    erasure_code/riscv64/ec_gf_vect_mul_rvv.S
    erasure_code/riscv64/ec_gf_vect_dot_prod_rvv.S
    erasure_code/riscv64/ec_encode_data_rvv.S
)

# Build source list based on architecture
set(ERASURE_CODE_SOURCES ${ERASURE_CODE_BASE_SOURCES})

if(CPU_X86_64)
    list(APPEND ERASURE_CODE_SOURCES ${ERASURE_CODE_X86_64_SOURCES})
elseif(CPU_AARCH64)
    list(APPEND ERASURE_CODE_SOURCES ${ERASURE_CODE_AARCH64_SOURCES})
elseif(CPU_PPC64LE)
    list(APPEND ERASURE_CODE_SOURCES ${ERASURE_CODE_PPC64LE_SOURCES})
elseif(CPU_RISCV64)
    list(APPEND ERASURE_CODE_SOURCES ${ERASURE_CODE_RISCV64_SOURCES})
elseif(CPU_UNDEFINED)
    list(APPEND ERASURE_CODE_SOURCES ${ERASURE_CODE_BASE_ALIASES_SOURCES})
endif()

# Headers exported by erasure_code module
set(ERASURE_CODE_HEADERS
    include/erasure_code.h
    include/gf_vect_mul.h
)

# Add to main extern headers list
list(APPEND EXTERN_HEADERS ${ERASURE_CODE_HEADERS})

# Add test applications for erasure_code module
if(ISAL_BUILD_TESTS)
    # Check tests (unit tests that are run by CTest)
    set(ERASURE_CODE_CHECK_TESTS
        gf_vect_mul_test
        erasure_code_test
        gf_inverse_test
        erasure_code_update_test
    )

    # Unit tests (additional unit tests)
    set(ERASURE_CODE_UNIT_TESTS
        gf_vect_mul_base_test
        gf_vect_dot_prod_base_test
        gf_vect_dot_prod_test
        gf_vect_mad_test
        erasure_code_base_test
    )

    # Other tests
    set(ERASURE_CODE_OTHER_TESTS
        gen_rs_matrix_limits
    )

    # Create check test executables
    foreach(test ${ERASURE_CODE_CHECK_TESTS})
        add_executable(${test} erasure_code/${test}.c)
        target_link_libraries(${test} PRIVATE isal)
        target_include_directories(${test} PRIVATE include)
        add_test(NAME ${test} COMMAND ${test})
    endforeach()

    # Create unit test executables
    foreach(test ${ERASURE_CODE_UNIT_TESTS})
        add_executable(${test} erasure_code/${test}.c)
        target_link_libraries(${test} PRIVATE isal)
        target_include_directories(${test} PRIVATE include)
    endforeach()

    # Create other test executables
    foreach(test ${ERASURE_CODE_OTHER_TESTS})
        add_executable(${test} erasure_code/${test}.c)
        target_link_libraries(${test} PRIVATE isal)
        target_include_directories(${test} PRIVATE include)
    endforeach()
endif()

# Add performance test applications for erasure_code module
if(ISAL_BUILD_PERF_TESTS)
    # Performance tests
    set(ERASURE_CODE_PERF_TESTS
        gf_vect_mul_perf
        gf_vect_dot_prod_perf
        gf_vect_dot_prod_1tbl
        erasure_code_perf
        erasure_code_base_perf
        erasure_code_update_perf
    )

    # Create performance test executables
    foreach(test ${ERASURE_CODE_PERF_TESTS})
        add_executable(${test} erasure_code/${test}.c)
        target_link_libraries(${test} PRIVATE isal)
        target_include_directories(${test} PRIVATE include)
    endforeach()
endif()
