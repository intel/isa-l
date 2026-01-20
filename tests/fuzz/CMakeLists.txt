# cmake-format: off
# Copyright (c) 2026, Intel Corporation
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

# Only build fuzz tests if using Clang with fuzzer support
if(NOT CMAKE_C_COMPILER_ID STREQUAL "Clang")
    message(STATUS "Skipping fuzz tests - requires Clang compiler with LibFuzzer support")
    return()
endif()

# Set fuzzing flags
set(FUZZ_FLAGS -g -O1 -fsanitize=fuzzer,address,undefined)

# CRC Fuzz Test Application
set(CRC_FUZZ_APP isal_fuzz_crc)
add_executable(${CRC_FUZZ_APP} crc_fuzz_test.c)

target_link_libraries(${CRC_FUZZ_APP} PRIVATE isal)
target_compile_options(${CRC_FUZZ_APP} PRIVATE ${FUZZ_FLAGS})
target_link_options(${CRC_FUZZ_APP} PRIVATE ${FUZZ_FLAGS})

message(STATUS "CRC Fuzz Test Application: ${CRC_FUZZ_APP}")
message(STATUS "  Compiler: ${CMAKE_C_COMPILER}")
message(STATUS "  Sanitizers: fuzzer, address, undefined")
