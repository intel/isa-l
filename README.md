=================================================
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

See [ISA-L for updates.](https://github.com/01org/isa-l)
For crypto functions see [isa-l_crypto on github.](https://github.com/01org/isa-l_crypto)

Build Prerequisites
===================

ISA-L requires yasm version 1.2.0 or later or nasm v2.11.01 or later.  Building
with autotools requires autoconf/automake packages.

Building ISA-L
==============

Autotools
---------

To build and install the library with autotools it is usually sufficient to run
the following:

    ./autogen.sh
    ./configure
    make
    sudo make install

Other targets include: make check, make tests, make perfs, make ex (examples)
and make other.

Windows
-------

On Windows use nmake to build dll and static lib:

    nmake -f Makefile.nmake

Other targes include: nmake check.
