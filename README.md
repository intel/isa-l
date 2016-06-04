=================================================
Intel(R) Intelligent Storage Acceleration Library
=================================================

[![Build Status](https://travis-ci.org/01org/isa-l.svg?branch=master)](https://travis-ci.org/01org/isa-l)

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
