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
cmake_minimum_required(VERSION 3.12)
cmake_policy(VERSION 3.12)

project(ISA-L
    VERSION 2.31.0
    DESCRIPTION "Intel's ISA-L (Intelligent Storage Acceleration Library)"
    LANGUAGES C ASM
)

# Enable NASM for x86_64 builds
if(CMAKE_SYSTEM_PROCESSOR STREQUAL "x86_64" OR CMAKE_SYSTEM_PROCESSOR STREQUAL "AMD64")
    enable_language(ASM_NASM)
endif()

# Set default build type
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

# Detect processor architecture
if(CMAKE_SYSTEM_PROCESSOR STREQUAL "x86_64" OR CMAKE_SYSTEM_PROCESSOR STREQUAL "AMD64")
    set(CPU_X86_64 ON)
    set(ARCH_DEF "x86_64")
elseif(CMAKE_SYSTEM_PROCESSOR STREQUAL "aarch64" OR CMAKE_SYSTEM_PROCESSOR STREQUAL "arm64")
    set(CPU_AARCH64 ON)
    set(ARCH_DEF "aarch64")
elseif(CMAKE_SYSTEM_PROCESSOR STREQUAL "ppc64le")
    set(CPU_PPC64LE ON)
    set(ARCH_DEF "ppc64le")
elseif(CMAKE_SYSTEM_PROCESSOR STREQUAL "riscv64")
    set(CPU_RISCV64 ON)
    set(ARCH_DEF "riscv64")
else()
    set(CPU_UNDEFINED ON)
endif()

# Compiler and assembler setup
if(CPU_X86_64)
    # Configure NASM flags
    set(CMAKE_ASM_NASM_FLAGS "-f elf64 -D LINUX")
    set(CMAKE_ASM_NASM_INCLUDES "-I ${CMAKE_SOURCE_DIR}/include/")
    set(USE_NASM ON)
elseif(CPU_AARCH64 OR CPU_RISCV64)
    # Use C compiler for assembly on ARM and RISC-V
    set(ASM_FILTER "${CMAKE_C_COMPILER} -D__ASSEMBLY__")
endif()

# Set include directories
set(ISAL_INCLUDE_DIRS
    ${CMAKE_SOURCE_DIR}/include
)

# Initialize EXTERN_HEADERS list
set(EXTERN_HEADERS)

# Compiler flags
if(ARCH_DEF)
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -D${ARCH_DEF}")
endif()

if(CPU_AARCH64 OR CPU_RISCV64)
    set(CMAKE_ASM_FLAGS "${CMAKE_C_FLAGS}")
endif()

# Library version (semantic versioning)
set(LIBISAL_VERSION_MAJOR 2)
set(LIBISAL_VERSION_MINOR 31)
set(LIBISAL_VERSION_PATCH 0)

# Include CMake modules for each library component
include(cmake/erasure_code.cmake)
include(cmake/raid.cmake)
include(cmake/crc.cmake)
include(cmake/igzip.cmake)
include(cmake/mem.cmake)

# Add test.h to extern headers (used by all modules)
list(APPEND EXTERN_HEADERS include/test.h)

# Create the main ISA-L library
# Build type option
option(BUILD_SHARED_LIBS "Build shared libraries" ON)

# Create the main ISA-L library
add_library(isal
    ${ERASURE_CODE_SOURCES}
    ${RAID_SOURCES}
    ${CRC_SOURCES}
    ${IGZIP_SOURCES}
    ${MEM_SOURCES}
)

# Set library properties
set_target_properties(isal PROPERTIES
    VERSION ${LIBISAL_VERSION_MAJOR}.${LIBISAL_VERSION_MINOR}.${LIBISAL_VERSION_PATCH}
    SOVERSION ${LIBISAL_VERSION_MAJOR}
    PUBLIC_HEADER "${EXTERN_HEADERS}"
)

# Configure include directories for NASM assembly files
if(CPU_X86_64 AND USE_NASM)
    # Filter assembly files by module and set appropriate include directories
    foreach(source IN LISTS ERASURE_CODE_SOURCES RAID_SOURCES CRC_SOURCES IGZIP_SOURCES MEM_SOURCES)
        if(source MATCHES "\\.asm$")
            get_filename_component(source_dir ${source} DIRECTORY)
            set_source_files_properties(${source} PROPERTIES
                INCLUDE_DIRECTORIES "${CMAKE_SOURCE_DIR}/include;${CMAKE_SOURCE_DIR}/${source_dir}")
        endif()
    endforeach()
endif()

# Include directories
target_include_directories(isal
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
)

# Generate isa-l.h header
set(ISAL_HEADER "${CMAKE_BINARY_DIR}/isa-l.h")
configure_file(${CMAKE_SOURCE_DIR}/cmake/isa-l.h.in ${ISAL_HEADER} @ONLY)

# Install targets
include(GNUInstallDirs)

# Install library
install(TARGETS isal
    EXPORT ISALTargets
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/isa-l
)

# Install generated header
install(FILES ${ISAL_HEADER}
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)

# Install headers
install(DIRECTORY include/
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/isa-l
    FILES_MATCHING PATTERN "*.h"
)

# Export targets
install(EXPORT ISALTargets
    FILE ISALTargets.cmake
    NAMESPACE ISAL::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/ISAL
)

# Generate and install package config files
include(CMakePackageConfigHelpers)

configure_package_config_file(
    "${CMAKE_SOURCE_DIR}/cmake/ISALConfig.cmake.in"
    "${CMAKE_BINARY_DIR}/ISALConfig.cmake"
    INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/ISAL
)

write_basic_package_version_file(
    "${CMAKE_BINARY_DIR}/ISALConfigVersion.cmake"
    VERSION ${PROJECT_VERSION}
    COMPATIBILITY SameMajorVersion
)

install(FILES
    "${CMAKE_BINARY_DIR}/ISALConfig.cmake"
    "${CMAKE_BINARY_DIR}/ISALConfigVersion.cmake"
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/ISAL
)

# Optional: Create pkg-config file
set(prefix ${CMAKE_INSTALL_PREFIX})
set(exec_prefix \${prefix})
set(libdir \${prefix}/${CMAKE_INSTALL_LIBDIR})
set(includedir \${prefix}/${CMAKE_INSTALL_INCLUDEDIR})
set(VERSION ${PROJECT_VERSION})

configure_file(
    "${CMAKE_SOURCE_DIR}/libisal.pc.in"
    "${CMAKE_BINARY_DIR}/libisal.pc"
    @ONLY
)

install(FILES "${CMAKE_BINARY_DIR}/libisal.pc"
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/pkgconfig
)
