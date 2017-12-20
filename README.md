Intel(R) Intelligent Storage Acceleration Library
=================================================

[![Build Status](https://travis-ci.org/01org/isa-l.svg?branch=master)](https://travis-ci.org/01org/isa-l)

ISA-L is a collection of optimized low-level functions targeting storage
applications.  ISA-L includes:
* Erasure codes - Fast block Reed-Solomon type erasure codes for any
  encode/decode matrix in GF(2^8).
* CRC - Fast implementations of cyclic redundancy check.  Six different
  polynomials supported.
  - iscsi32, ieee32, t10dif, ecma64, iso64, jones64.
* Raid - calculate and operate on XOR and P+Q parity found in common RAID
  implementations.
* Compression - Fast deflate-compatible data compression.
* De-compression - Fast inflate-compatible data compression.

Also see:
* [ISA-L for updates](https://github.com/01org/isa-l).
* For crypto functions see [isa-l_crypto on github](https://github.com/01org/isa-l_crypto).
* The [github wiki](https://github.com/01org/isa-l/wiki) including a list of
  [distros/ports](https://github.com/01org/isa-l/wiki/Ports--Repos) offering binary packages.
* ISA-L [mailing list](https://lists.01.org/mailman/listinfo/isal).
* [Contributing](CONTRIBUTING.md).

Building ISA-L
--------------

### Prerequisites

* Assembler: nasm v2.11.01 or later (nasm v2.13 or better suggested for building in AVX512 support)
  or yasm version 1.2.0 or later.
* Compiler: gcc, clang, icc or VC compiler.
* Make: GNU 'make' or 'nmake' (Windows).
* Optional: Building with autotools requires autoconf/automake packages.

### Autotools
To build and install the library with autotools it is usually sufficient to run:

    ./autogen.sh
    ./configure
    make
    sudo make install

### Makefile
To use a standard makefile run:

    make -f Makefile.unx

### Windows
On Windows use nmake to build dll and static lib:

    nmake -f Makefile.nmake

### Other make targets
Other targets include:
* `make check` : create and run tests
* `make tests` : create additional unit tests
* `make perfs` : create included performance tests
* `make ex`    : build examples
* `make other` : build other utilities such as compression file tests
* `make doc`   : build API manual
