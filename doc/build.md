# ISA-L Build Details

## Build tools

NASM: For x86-64 builds it is highly recommended to get an up-to-date version of
[nasm] that can understand the latest instruction sets. Building with an older
assembler version is often possible but the library may lack some function
versions for the best performance. For example, as a minimum, nasm v2.11.01 or
yasm 1.2.0 can be used to build a limited functionality library but it will not
include any function versions with AVX2, AVX512, or optimizations for many
processors before the assembler's build. The configure or make tools can run
tests to check the assembler's knowledge of new instructions and change build
defines. For autoconf builds, check the output of configure for full nasm
support as it includes the following lines.

    checking for nasm... yes
    checking for modern nasm... yes
    checking for optional nasm AVX512 support... yes
    checking for additional nasm AVX512 support... yes

If an appropriate nasm is not available from your distro, it is simple to build
from source or download an executable from [nasm].

    git clone --depth=10 https://github.com/netwide-assembler/nasm
    cd nasm
    ./autogen.sh
    ./configure
    make
    sudo make install

## Windows Build Environment Details

The windows dynamic and static libraries can be built with the nmake tool on the
windows command line when appropriate paths and tools are setup as follows.

### Download nasm and put into path

Download and install [nasm] and add location to path.

    set PATH=%PATH%;C:\Program Files\NASM

### Setup compiler environment

Install compiler and run environment setup script.

Compilers for windows usually have a batch file to setup environment variables
for the command line called `vcvarsall.bat` or `compilervars.bat` or a link to
run these. For Visual Studio this may be as follows for Community edition.

    C:\Program Files (x86)\Microsoft Visual Studio\2019\Community\VC\Auxiliary\Build\vcvarsall.bat x64

For the Intel compiler the path is typically as follows where yyyy, x, zzz
represent the version.

    C:\Program Files (x86)\IntelSWTools\system_studio_for_windows_yyyy.x.zzz\compilers_and_libraries_yyyy\bin\compilervars.bat intel64

### Build ISA-L libs and copy to appropriate place

Run `nmake /f Makefile.nmake`

This should build isa-l.dll, isa-l.lib and isa-l_static.lib. You may want to
copy the libs to a system directory in the dynamic linking path such as
`C:\windows\system32` or to a project directory.

To build a simple program with a static library.

    cl /Fe: test.exe test.c isa-l_static.lib

[nasm]: https://www.nasm.us
