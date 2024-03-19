Intel(R) Intelligent Storage Acceleration Library
=================================================

![Continuous Integration](https://github.com/intel/isa-l/actions/workflows/ci.yml/badge.svg)
[![Package on conda-forge](https://img.shields.io/conda/v/conda-forge/isa-l.svg)](https://anaconda.org/conda-forge/isa-l)
[![Coverity Status](https://scan.coverity.com/projects/29480/badge.svg)](https://scan.coverity.com/projects/intel-isa-l)
[![OpenSSF Scorecard](https://api.securityscorecards.dev/projects/github.com/intel/isa-l/badge)](https://securityscorecards.dev/viewer/?uri=github.com/intel/isa-l)

ISA-L is a collection of optimized low-level functions targeting storage
applications.  ISA-L includes:
* Erasure codes - Fast block Reed-Solomon type erasure codes for any
  encode/decode matrix in GF(2^8).
* CRC - Fast implementations of cyclic redundancy check.  Six different
  polynomials supported.
  - iscsi32, ieee32, t10dif, ecma64, iso64, jones64, rocksoft64.
* Raid - calculate and operate on XOR and P+Q parity found in common RAID
  implementations.
* Compression - Fast deflate-compatible data compression.
* De-compression - Fast inflate-compatible data compression.
* igzip - A command line application like gzip, accelerated with ISA-L.

Also see:
* [ISA-L for updates](https://github.com/intel/isa-l).
* For crypto functions see [isa-l_crypto on github](https://github.com/intel/isa-l_crypto).
* The [github wiki](https://github.com/intel/isa-l/wiki) including a list of
  [distros/ports](https://github.com/intel/isa-l/wiki/Ports--Repos) offering binary packages
  as well as a list of [language bindings](https://github.com/intel/isa-l/wiki/Language-Bindings).
* [Contributing](CONTRIBUTING.md).
* [Security Policy](SECURITY.md).
* Docs on [units](doc/functions.md), [tests](doc/test.md), or [build details](doc/build.md).

Building ISA-L
--------------

### Prerequisites

* Make: GNU 'make' or 'nmake' (Windows).
* Optional: Building with autotools requires autoconf/automake/libtool packages.
* Optional: Manual generation requires help2man package.

x86_64:
* Assembler: nasm. Version 2.15 or later suggested (other versions of nasm and
  yasm may build but with limited function [support](doc/build.md)).
* Compiler: gcc, clang, icc or VC compiler.

aarch64:
* Assembler: gas v2.24 or later.
* Compiler: gcc v4.7 or later.

other:
* Compiler: Portable base functions are available that build with most C compilers.

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

or see [details on setting up environment here](doc/build.md).

### Other make targets
Other targets include:
* `make check` : create and run tests
* `make tests` : create additional unit tests
* `make perfs` : create included performance tests
* `make ex`    : build examples
* `make other` : build other utilities such as compression file tests
* `make doc`   : build API manual

DLL Injection Attack
--------------------

### Problem

The Windows OS has an insecure predefined search order and set of defaults when trying to locate a resource. If the resource location is not specified by the software, an attacker need only place a malicious version in one of the locations Windows will search, and it will be loaded instead. Although this weakness can occur with any resource, it is especially common with DLL files.

### Solutions

Applications using libisal DLL library may need to apply one of the solutions to prevent from DLL injection attack.

Two solutions are available:
- Using a Fully Qualified Path is the most secure way to load a DLL
- Signature verification of the DLL

### Resources and Solution Details

- Security remarks section of LoadLibraryEx documentation by Microsoft: <https://docs.microsoft.com/en-us/windows/win32/api/libloaderapi/nf-libloaderapi-loadlibraryexa#security-remarks>
- Microsoft Dynamic Link Library Security article: <https://docs.microsoft.com/en-us/windows/win32/dlls/dynamic-link-library-security>
- Hijack Execution Flow: DLL Search Order Hijacking: <https://attack.mitre.org/techniques/T1574/001>
- Hijack Execution Flow: DLL Side-Loading: <https://attack.mitre.org/techniques/T1574/002>
