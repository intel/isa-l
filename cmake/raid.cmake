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

# RAID module CMake configuration

set(RAID_BASE_SOURCES
    raid/raid_base.c
)

set(RAID_BASE_ALIASES_SOURCES
    raid/raid_base_aliases.c
)

set(RAID_X86_64_SOURCES
    raid/xor_gen_sse.asm
    raid/pq_gen_sse.asm
    raid/xor_check_sse.asm
    raid/pq_check_sse.asm
    raid/pq_gen_avx.asm
    raid/xor_gen_avx.asm
    raid/pq_gen_avx2.asm
    raid/pq_gen_avx2_gfni.asm
    raid/xor_gen_avx512.asm
    raid/pq_gen_avx512.asm
    raid/pq_gen_avx512_gfni.asm
    raid/raid_multibinary.asm
)

set(RAID_AARCH64_SOURCES
    raid/aarch64/xor_gen_neon.S
    raid/aarch64/pq_gen_neon.S
    raid/aarch64/xor_check_neon.S
    raid/aarch64/pq_check_neon.S
    raid/aarch64/raid_multibinary_arm.S
    raid/aarch64/raid_aarch64_dispatcher.c
)

set(RAID_RISCV64_SOURCES
    raid/riscv64/raid_multibinary_riscv64_dispatcher.c
    raid/riscv64/raid_multibinary_riscv64.S
    raid/riscv64/raid_pq_gen_rvv.S
    raid/riscv64/raid_xor_gen_rvv.S
)

# Build source list based on architecture
set(RAID_SOURCES ${RAID_BASE_SOURCES})

if(CPU_X86_64)
    list(APPEND RAID_SOURCES ${RAID_X86_64_SOURCES})
elseif(CPU_AARCH64)
    list(APPEND RAID_SOURCES ${RAID_AARCH64_SOURCES})
elseif(CPU_PPC64LE)
    # PPC64LE uses base aliases
    list(APPEND RAID_SOURCES ${RAID_BASE_ALIASES_SOURCES})
elseif(CPU_RISCV64)
    list(APPEND RAID_SOURCES ${RAID_RISCV64_SOURCES})
elseif(CPU_UNDEFINED)
    list(APPEND RAID_SOURCES ${RAID_BASE_ALIASES_SOURCES})
endif()

# Headers exported by RAID module
set(RAID_HEADERS
    include/raid.h
)

# Add to main extern headers list
list(APPEND EXTERN_HEADERS ${RAID_HEADERS})

# Add test applications for raid module
if(ISAL_BUILD_TESTS)
    # Check tests (unit tests that are run by CTest)
    set(RAID_CHECK_TESTS
        xor_gen_test
        pq_gen_test
        xor_check_test
        pq_check_test
    )

    # Create check test executables
    foreach(test ${RAID_CHECK_TESTS})
        add_executable(${test} raid/${test}.c)
        target_link_libraries(${test} PRIVATE isal)
        target_include_directories(${test} PRIVATE include)
        add_test(NAME ${test} COMMAND ${test})
    endforeach()
endif()
