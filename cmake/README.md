# CMake Build System for ISA-L

This directory contains CMake build configuration files for the ISA-L library (Intelligent Storage Acceleration Library).

## Prerequisites

### Required Tools

- **CMake** 3.12 or later
- **C compiler** (GCC, Clang, or compatible)

### Architecture-Specific Requirements

#### x86_64
- **NASM** (Netwide Assembler) - Required for optimized assembly implementations

#### ARM64/AArch64, RISC-V, PowerPC
- Standard C compiler with assembly support

## Building

### Quick Start

```bash
mkdir build
cd build
cmake ..
make -j$(nproc)
```

### Build Options

#### Specify build type
```bash
cmake -DCMAKE_BUILD_TYPE=Release ..  # Default
cmake -DCMAKE_BUILD_TYPE=Debug ..
```

#### Cross-compilation example
```bash
cmake -DCMAKE_TOOLCHAIN_FILE=path/to/toolchain.cmake ..
```

## Installation

```bash
make install
```

Default installation paths:
- Libraries: `/usr/local/lib`
- Headers: `/usr/local/include/isa-l/`
- CMake config: `/usr/local/lib/cmake/ISAL/`
- pkg-config: `/usr/local/lib/pkgconfig/`

### Custom installation prefix
```bash
cmake -DCMAKE_INSTALL_PREFIX=/usr ..
make install
```

## Library Modules

The CMake build system is organized into the following modules:

- **erasure_code** - Erasure coding and Galois Field operations
- **raid** - RAID XOR and P+Q generation functions
- **crc** - CRC16, CRC32, and CRC64 implementations
- **igzip** - Fast deflate/inflate compression
- **mem** - Memory utility functions

## Using ISA-L in Your Project

### CMake Integration

```cmake
find_package(ISAL REQUIRED)
target_link_libraries(your_target ISAL::isal)
```

### pkg-config Integration

```bash
gcc $(pkg-config --cflags --libs libisal) your_program.c
```

### Direct Header Include

```c
#include <isa-l.h>  // Includes all ISA-L headers
// or include specific headers:
#include <isa-l/erasure_code.h>
#include <isa-l/raid.h>
#include <isa-l/crc.h>
```

## Architecture Support

| Architecture | Status | Assembly Optimizations |
|--------------|--------|------------------------|
| x86_64       | ✅     | SSE, AVX, AVX2, AVX-512 |
| AArch64      | ✅     | NEON, SVE |
| RISC-V 64    | ✅     | RVV (Vector extensions) |
| PowerPC64LE  | ✅     | VSX |

## Build Targets

- `isal` - Main shared library
- `install` - Install library and headers

## Troubleshooting

### NASM not found (x86_64)
```
sudo apt-get install nasm  # Ubuntu/Debian
sudo yum install nasm      # RHEL/CentOS
brew install nasm          # macOS
```

### Assembly compilation errors
Ensure you have the correct assembler for your platform:
- x86_64: NASM
- ARM/RISC-V: GCC with assembly support

## Contributing

When adding new source files, update the appropriate module file in the `cmake/` directory:
- `cmake/erasure_code.cmake`
- `cmake/raid.cmake`
- `cmake/crc.cmake`
- `cmake/igzip.cmake`
- `cmake/mem.cmake`
