#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ(2.69)
AC_INIT([libisal],
        [2.29.0],
        [sg.support.isal@intel.com],
        [isa-l],
        [http://01.org/storage-acceleration-library])
AC_CONFIG_SRCDIR([])
AC_CONFIG_AUX_DIR([build-aux])
AM_INIT_AUTOMAKE([
	foreign
	1.11
	-Wall
	-Wno-portability
	silent-rules
	tar-pax
	no-dist-gzip
	dist-xz
	subdir-objects
])
AM_PROG_AS

AC_CANONICAL_HOST
CPU=""
AS_CASE([$host_cpu],
  [x86_64], [CPU="x86_64"],
  [amd64], [CPU="x86_64"],
  [i?86], [CPU="x86_32"],
  [aarch64], [CPU="aarch64"],
  [arm64], [CPU="aarch64"],
  [powerpc64le], [CPU="ppc64le"],
  [ppc64le], [CPU="ppc64le"],
)
AM_CONDITIONAL([CPU_X86_64], [test "$CPU" = "x86_64"])
AM_CONDITIONAL([CPU_X86_32], [test "$CPU" = "x86_32"])
AM_CONDITIONAL([CPU_AARCH64], [test "$CPU" = "aarch64"])
AM_CONDITIONAL([CPU_PPC64LE], [test "$CPU" = "ppc64le"])
AM_CONDITIONAL([CPU_UNDEFINED], [test "x$CPU" = "x"])

if test "$CPU" = "x86_64"; then
   is_x86=yes
else
   if test "$CPU" = "x86_32"; then
      is_x86=yes
   else
      is_x86=no
   fi
fi

# Check for programs
AC_PROG_CC_STDC
AC_USE_SYSTEM_EXTENSIONS
AM_SILENT_RULES([yes])
LT_INIT
AC_PREFIX_DEFAULT([/usr])
AC_PROG_SED
AC_PROG_MKDIR_P

# Options
AC_ARG_ENABLE([debug],
        AS_HELP_STRING([--enable-debug], [enable debug messages @<:@default=disabled@:>@]),
        [], [enable_debug=no])
AS_IF([test "x$enable_debug" = "xyes"], [
        AC_DEFINE(ENABLE_DEBUG, [1], [Debug messages.])
])

# If this build is for x86, look for yasm and nasm
if test x"$is_x86" = x"yes"; then
  # Pick an assembler yasm or nasm
  if test x"$AS" = x""; then
    # Check for yasm and yasm features
    yasm_feature_level=0
    AC_CHECK_PROG(HAVE_YASM, yasm, yes, no)
    if test "$HAVE_YASM" = "yes"; then
       yasm_feature_level=1
    else
       AC_MSG_RESULT([no yasm])
    fi
    if test x"$yasm_feature_level" = x"1"; then
       AC_MSG_CHECKING([for modern yasm])
       AC_LANG_CONFTEST([AC_LANG_SOURCE([[vmovdqa %xmm0, %xmm1;]])])
       if yasm -f elf64 -p gas conftest.c ; then
         AC_MSG_RESULT([yes])
         yasm_feature_level=4
       else
         AC_MSG_RESULT([no])
       fi
    fi
    if test x"$yasm_feature_level" = x"4"; then
       AC_MSG_CHECKING([for optional yasm AVX512 support])
       AC_LANG_CONFTEST([AC_LANG_SOURCE([[vpshufb %zmm0, %zmm1, %zmm2;]])])
       if yasm -f elf64 -p gas conftest.c 2> /dev/null; then
         AC_MSG_RESULT([yes])
         yasm_feature_level=6
       else
         AC_MSG_RESULT([no])
       fi
    fi
    if test x"$yasm_feature_level" = x"6"; then
      AC_MSG_CHECKING([for additional yasm AVX512 support])
      AC_LANG_CONFTEST([AC_LANG_SOURCE([[vpcompressb zmm0, k1, zmm1;]])])
      sed -i -e '/vpcompressb/!d' conftest.c
      if yasm -f elf64  conftest.c 2> /dev/null; then
        AC_MSG_RESULT([yes])
        yasm_feature_level=10
      else
        AC_MSG_RESULT([no])
      fi
    fi

    # Check for nasm and nasm features
    nasm_feature_level=0
    AC_CHECK_PROG(HAVE_NASM, nasm, yes, no)
    if test "$HAVE_NASM" = "yes"; then
       nasm_feature_level=1
    else
       AC_MSG_RESULT([no nasm])
    fi

    if test x"$nasm_feature_level" = x"1"; then
      AC_MSG_CHECKING([for modern nasm])
      AC_LANG_CONFTEST([AC_LANG_SOURCE([[pblendvb xmm2, xmm1;]])])
      sed -i -e '/pblendvb/!d' conftest.c
      if nasm -f elf64 conftest.c 2> /dev/null; then
        AC_MSG_RESULT([yes])
        nasm_feature_level=4
      else
        AC_MSG_RESULT([no])
      fi
    fi
    if test x"$nasm_feature_level" = x"4"; then
      AC_MSG_CHECKING([for optional nasm AVX512 support])
      AC_LANG_CONFTEST([AC_LANG_SOURCE([[vinserti32x8 zmm0, ymm1, 1;]])])
      sed -i -e '/vinsert/!d' conftest.c
      if nasm -f elf64  conftest.c 2> /dev/null; then
        AC_MSG_RESULT([yes])
        nasm_feature_level=6
      else
        AC_MSG_RESULT([no])
      fi
    fi
    if test x"$nasm_feature_level" = x"6"; then
      AC_MSG_CHECKING([for additional nasm AVX512 support])
      AC_LANG_CONFTEST([AC_LANG_SOURCE([[vpcompressb zmm0 {k1}, zmm1;]])])
      sed -i -e '/vpcompressb/!d' conftest.c
      if nasm -f elf64  conftest.c 2> /dev/null; then
        AC_MSG_RESULT([yes])
        nasm_feature_level=10
      else
        AC_MSG_RESULT([no])
      fi
    fi

    if test $nasm_feature_level -ge $yasm_feature_level ; then
      AS=nasm
      as_feature_level=$nasm_feature_level
    else
      AS=yasm
      as_feature_level=$yasm_feature_level
    fi

  else
    # Check for $AS supported features
    as_feature_level=0
    AC_CHECK_PROG(HAVE_AS, $AS, yes, no)
    if test "$HAVE_AS" = "yes"; then
       as_feature_level=1
    else
       AC_MSG_ERROR([no $AS])
    fi

    if test x"$as_feature_level" = x"1"; then
      AC_MSG_CHECKING([for modern $AS])
      AC_LANG_CONFTEST([AC_LANG_SOURCE([[pblendvb xmm2, xmm1;]])])
      sed -i -e '/pblendvb/!d' conftest.c
      if $AS -f elf64 conftest.c 2> /dev/null; then
        AC_MSG_RESULT([yes])
        as_feature_level=4
      else
        AC_MSG_RESULT([no])
      fi
    fi
    if test x"$as_feature_level" = x"4"; then
      AC_MSG_CHECKING([for optional as AVX512 support])
      AC_LANG_CONFTEST([AC_LANG_SOURCE([[vinserti32x8 zmm0, ymm1, 1;]])])
      sed -i -e '/vinsert/!d' conftest.c
      if $AS -f elf64  conftest.c 2> /dev/null; then
        AC_MSG_RESULT([yes])
        as_feature_level=6
      else
        AC_MSG_RESULT([no])
      fi
    fi
    if test x"$as_feature_level" = x"6"; then
      AC_MSG_CHECKING([for additional as AVX512 support])
      AC_LANG_CONFTEST([AC_LANG_SOURCE([[vpcompressb zmm0, k1, zmm1;]])])
      sed -i -e '/vpcompressb/!d' conftest.c
      if $AS -f elf64  conftest.c 2> /dev/null; then
        AC_MSG_RESULT([yes])
        as_feature_level=10
      else
        AC_MSG_RESULT([no])
      fi
    fi
  fi

  if test $as_feature_level -lt 2 ; then
    AC_MSG_ERROR([No modern nasm or yasm found as required. Nasm should be v2.11.01 or later (v2.13 for AVX512) and yasm should be 1.2.0 or later.])
  fi

  case $host_os in
       *linux*)  arch=linux   yasm_args="-f elf64";;
       *darwin*) arch=darwin  yasm_args="-f macho64 --prefix=_ ";;
       *netbsd*) arch=netbsd  yasm_args="-f elf64";;
       *mingw*)  arch=mingw   yasm_args="-f win64";;
       *)        arch=unknown yasm_args="-f elf64";;
  esac

  # Fix for nasm missing windows features
  if test x"$arch" = x"mingw"; then
    AS=yasm
    as_feature_level=$yasm_feature_level
    if test $as_feature_level -lt 2 ; then
      AC_MSG_ERROR([Mingw build requires Yasm 1.2.0 or later.])
    fi
  fi

  AC_DEFINE_UNQUOTED(AS_FEATURE_LEVEL, [$as_feature_level], [Assembler feature level.])
  if test $as_feature_level -ge 6 ; then
    AC_DEFINE(HAVE_AS_KNOWS_AVX512, [1], [Assembler can do AVX512.])
    have_as_knows_avx512=yes
  else
    AC_MSG_RESULT([Assembler does not understand AVX512 opcodes.  Consider upgrading for best performance.])
  fi

  AM_CONDITIONAL(USE_YASM, test x"$AS" = x"yasm")
  AM_CONDITIONAL(USE_NASM, test x"$AS" = x"nasm")
  AM_CONDITIONAL(WITH_AVX512, test x"$have_as_knows_avx512" = x"yes")
  AC_SUBST([yasm_args])
  AM_CONDITIONAL(DARWIN, test x"$arch" = x"darwin")
  AC_MSG_RESULT([Using $AS args target "$arch" "$yasm_args"])
else
  # Disable below conditionals if not x86
  AM_CONDITIONAL(USE_YASM, test "x" = "y")
  AM_CONDITIONAL(USE_NASM, test "x" = "y")
  AM_CONDITIONAL(WITH_AVX512, test "x" = "y")
  AM_CONDITIONAL(DARWIN, test "x" = "y")
fi


# Check for header files
AC_CHECK_HEADERS([limits.h stdint.h stdlib.h string.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_C_INLINE
AC_TYPE_SIZE_T
AC_TYPE_UINT16_T
AC_TYPE_UINT32_T
AC_TYPE_UINT64_T
AC_TYPE_UINT8_T

# Checks for library functions.
AC_FUNC_MALLOC  # Used only in tests
AC_CHECK_FUNCS([memmove memset getopt])

my_CFLAGS="\
-Wall \
-Wchar-subscripts \
-Wformat-security \
-Wnested-externs \
-Wpointer-arith \
-Wshadow \
-Wstrict-prototypes \
-Wtype-limits \
"
AC_SUBST([my_CFLAGS])

AC_CONFIG_FILES([\
	Makefile\
	libisal.pc
])

AC_OUTPUT
AC_MSG_RESULT([
        $PACKAGE $VERSION
        =====

        prefix:                 ${prefix}
        sysconfdir:             ${sysconfdir}
        libdir:                 ${libdir}
        includedir:             ${includedir}

        compiler:               ${CC}
        cflags:                 ${CFLAGS}
        ldflags:                ${LDFLAGS}

        debug:                  ${enable_debug}
])
