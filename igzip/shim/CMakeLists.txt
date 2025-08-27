#/**********************************************************************
#  Copyright(c) 2025 Intel Corporation All rights reserved.
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
#**********************************************************************/

# Only set project and cmake_minimum_required if this is the root project
if(CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
    cmake_minimum_required(VERSION 3.16)
    # Project name
    project(isal_shim LANGUAGES C)
endif()

# Set the library name
set(LIBRARY_NAME isal-shim)

# Collect all source files in the current directory
file(GLOB SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/*.c")

# Add the include directory for the header file
include_directories("${CMAKE_CURRENT_SOURCE_DIR}/../../include")

# Set default build type to Release if not specified
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Release" CACHE STRING "Choose the type of build" FORCE)
endif()

# Set debug flags for Debug build type
set(CMAKE_C_FLAGS_DEBUG "-g -O0 -DDEBUG")

# Create the shared library
add_library(${LIBRARY_NAME} SHARED ${SOURCES})

# Handle linking differently based on whether this is built as part of the main project or standalone
if(TARGET isal)
    # Building as part of the main ISA-L project - link with the isal target
    target_link_libraries(${LIBRARY_NAME} PRIVATE isal)
    target_include_directories(${LIBRARY_NAME} PRIVATE "${CMAKE_SOURCE_DIR}/include")
else()
    # Building standalone - find and link with installed isal library
    if (NOT DEFINED ISAL_INSTALL_DIR)
        # Link the library with isal
        find_library(ISAL_LIBRARY isal REQUIRED)
        target_link_libraries(${LIBRARY_NAME} PRIVATE ${ISAL_LIBRARY})
    else()
        # Link the library with isal built and installed in ISAL_INSTALL_DIR
        find_library(ISAL_LIBRARY isal PATHS "${ISAL_INSTALL_DIR}/lib" NO_DEFAULT_PATH REQUIRED)
        target_include_directories(${LIBRARY_NAME} PRIVATE "${ISAL_INSTALL_DIR}/include")
        target_link_libraries(${LIBRARY_NAME} PRIVATE ${ISAL_LIBRARY})
    endif()
endif()

# Set the output name for the shared library
set_target_properties(${LIBRARY_NAME} PROPERTIES OUTPUT_NAME "isal-shim")

# Remove the "lib" prefix from the shared library name
set_target_properties(${LIBRARY_NAME} PROPERTIES PREFIX "")

# Install the shim library when built as part of the main project
if(TARGET isal)
    include(GNUInstallDirs)
    install(TARGETS ${LIBRARY_NAME}
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
        ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    )
endif()
