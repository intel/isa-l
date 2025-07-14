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

# MEM module CMake configuration

set(MEM_BASE_SOURCES
    mem/mem_zero_detect_base.c
)

set(MEM_BASE_ALIASES_SOURCES
    mem/mem_zero_detect_base_aliases.c
)

set(MEM_X86_64_SOURCES
    mem/mem_zero_detect_avx512.asm
    mem/mem_zero_detect_avx2.asm
    mem/mem_zero_detect_avx.asm
    mem/mem_zero_detect_sse.asm
    mem/mem_multibinary.asm
)

set(MEM_AARCH64_SOURCES
    mem/aarch64/mem_zero_detect_neon.S
    mem/aarch64/mem_multibinary_arm.S
    mem/aarch64/mem_aarch64_dispatcher.c
)

set(MEM_RISCV64_SOURCES
    mem/riscv64/mem_multibinary_riscv64_dispatcher.c
    mem/riscv64/mem_multibinary_riscv64.S
    mem/riscv64/mem_zero_detect_rvv.S
)

# Build source list based on architecture
set(MEM_SOURCES ${MEM_BASE_SOURCES})

if(CPU_X86_64)
    list(APPEND MEM_SOURCES ${MEM_X86_64_SOURCES})
elseif(CPU_AARCH64)
    list(APPEND MEM_SOURCES ${MEM_AARCH64_SOURCES})
elseif(CPU_PPC64LE OR CPU_UNDEFINED)
    # These architectures use base aliases
    list(APPEND MEM_SOURCES ${MEM_BASE_ALIASES_SOURCES})
elseif(CPU_RISCV64)
    list(APPEND MEM_SOURCES ${MEM_RISCV64_SOURCES})
endif()

# Headers exported by MEM module
set(MEM_HEADERS
    include/mem_routines.h
)

# Add to main extern headers list
list(APPEND EXTERN_HEADERS ${MEM_HEADERS})
