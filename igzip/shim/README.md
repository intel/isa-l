# Shim Library for zlib API with ISA-L (Experimental)

This library provides an experimental shim layer that implements the zlib API while utilizing the Intel® Storage Acceleration Library (ISA-L) for enhanced performance. It enables seamless integration of ISA-L's optimized compression and decompression routines into applications designed for the zlib interface.

**Note:** This is an experimental feature and may not be suitable for all production environments without thorough testing.

## Features

- **zlib API Compatibility**: Drop-in replacement for zlib with minimal code changes.
- **ISA-L**: Leverages ISA-L for high-performance data compression and decompression.

## Requirements

- **ISA-L**: Ensure ISA-L is installed and available on your system.

## Installation

1. Clone the repository:
    ```bash
    git clone https://github.com/intel-innersource/libraries.performance.storage.isa-l
    cd shim
    ```

2. Build isal-shim.so:
    ```bash
    mkdir build
    cd build

    cmake ..

    # Enable debug mode.
    cmake -DCMAKE_BUILD_TYPE=Debug ..

    # Specify the custom installation directory for ISA-L.
    cmake -DISAL_INSTALL_DIR=/path/to/isal/install/ ..

    make
    ```

3. Preload the shim library in your project:
    ```bash
    LD_PRELOAD=libraries.performance.storage.isa-l/shim/build/isal-shim.so  ./app
    LD_PRELOAD=libraries.performance.storage.isa-l/shim/build/isal-shim.so  python test.py
    ```

## Usage

Replace your existing zlib library with this shim library. The API remains the same, so no additional changes are required in the application code.

## Scope/Constraints

This library is a drop-in replacement for zlib that utilizes Intel's Storage Acceleration Library (ISA-L) for improved compression/decompression performance. However, it has the following limitations:

- **Platform Support**: Currently only tested and supported on Linux systems
- **Function Interception**: Only intercepts specific zlib functions (see Intercepted Zlib Functions section below)
- **API Compatibility**: While designed as a drop-in replacement, some zlib functions are not supported yet

For production use, thorough testing in your specific environment is recommended before deployment.

## Intercepted Zlib Functions

deflate/inflate and related functions
- deflateInit, deflateInit2, deflateSetDictionary, deflate, deflateEnd, deflateSetHeader
- inflateInit, inflateInit2, inflateSetDictionary, inflate, inflateEnd

utility functions
- compress, uncompress
- compress, uncompress2
