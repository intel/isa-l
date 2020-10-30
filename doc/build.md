# ISA-L Build Details

For x86-64 builds it is highly recommended to get an up-to-date version of
[nasm] that can understand the latest instruction sets. Building with an older
version is usually possible but the library may lack some function versions for
the best performance.

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
