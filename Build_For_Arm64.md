Building ISA-L on Arm
=================================================

## Prerequisites

* HOST CPU MUST be aarch64
* Assembler: gas v2.24 or later
* Compiler: gcc v4.7 or later
* Make: GNU 'make'
* Optional:
  * Building with autotools requires autoconf/automake packages.

## Autotools

To build and install the library with autotools it is usually sufficient to run:

    ./autogen.sh
    ./configure
    make
    sudo make install

## Makefile
To use a standard makefile run:

    make -f Makefile.unx


### Other make targets
Other targets include:
* `make check` : create and run tests
* `make tests` : create additional unit tests
* `make perfs` : create included performance tests
* `make ex`    : build examples
* `make other` : build other utilities such as compression file tests
* `make doc`   : build API manual
