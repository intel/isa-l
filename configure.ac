#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ(2.69)
AC_INIT([libisal],
        [2.31.1],
        [https://github.com/intel/isa-l/issues],
        [isa-l])
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
  [aarch64], [CPU="aarch64"],
  [arm64], [CPU="aarch64"],
  [powerpc64le], [CPU="ppc64le"],
  [ppc64le], [CPU="ppc64le"],
  [riscv64], [CPU="riscv64"],
)
AM_CONDITIONAL([CPU_X86_64], [test "$CPU" = "x86_64"])
AM_CONDITIONAL([CPU_AARCH64], [test "$CPU" = "aarch64"])
AM_CONDITIONAL([CPU_PPC64LE], [test "$CPU" = "ppc64le"])
AM_CONDITIONAL([CPU_RISCV64], [test "$CPU" = "riscv64"])
AM_CONDITIONAL([CPU_UNDEFINED], [test "x$CPU" = "x"])
AM_CONDITIONAL([HAVE_RVV], [false])

# Check for programs
AC_PROG_CC_STDC
AC_USE_SYSTEM_EXTENSIONS
AM_SILENT_RULES([yes])
LT_INIT
AC_PREFIX_DEFAULT([/usr])
AC_PROG_SED
AC_PROG_MKDIR_P

case "${CPU}" in

	x86_64)

		is_x86=yes
		;;

	riscv64)

		AC_MSG_CHECKING([checking RVV support])
		AC_COMPILE_IFELSE(
			[AC_LANG_PROGRAM([], [
				__asm__ volatile(
					".option arch, +v\n"
					"vsetivli zero, 0, e8, m1, ta, ma\n"
				);
			])],
			[AC_DEFINE([HAVE_RVV], [1], [Enable RVV instructions])
			AM_CONDITIONAL([HAVE_RVV], [true]) rvv=yes],
			[AM_CONDITIONAL([HAVE_RVV], [false]) rvv=no]
		)
		AC_MSG_RESULT([$rvv])
		;;

	*)
		is_x86=no

esac

# Options
AC_ARG_ENABLE([debug],
        AS_HELP_STRING([--enable-debug], [enable debug messages @<:@default=disabled@:>@]),
        [], [enable_debug=no])
AS_IF([test "x$enable_debug" = "xyes"], [
        AC_DEFINE(ENABLE_DEBUG, [1], [Debug messages.])
])

# If this build is for x86, look for nasm
if test x"$is_x86" = x"yes"; then
  AC_MSG_CHECKING([whether Intel CET is enabled])
  AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[]], [[
#ifndef __CET__
# error CET is not enabled
#endif]])],[AC_MSG_RESULT([yes])
		   intel_cet_enabled=yes],[AC_MSG_RESULT([no])
		   intel_cet_enabled=no])
AS_IF([test "x$intel_cet_enabled" = "xyes"], [
        AC_DEFINE(INTEL_CET_ENABLED, [1], [Intel CET enabled.])
])

# check if LD -z options are supported
LDFLAGS="\
-Wl,-z,noexecstack \
-Wl,-z,relro \
-Wl,-z,now \
"
AC_MSG_CHECKING([if $LD supports $LDFLAGS])
AC_LINK_IFELSE([AC_LANG_PROGRAM([[]], [[
int main(int argc, char **argv)
{
	return 0;
}]])],
	[AC_MSG_RESULT([yes])],
	[AC_MSG_RESULT([no])
		LDFLAGS=""]
)

  # Pick NASM assembler
  if test x"$AS" = x""; then
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
      AC_LANG_CONFTEST([AC_LANG_SOURCE([[vpcompressb zmm0 {k1}, zmm1;]])])
      sed -i -e '/vpcompressb/!d' conftest.c
      if nasm -f elf64 conftest.c 2> /dev/null; then
        AC_MSG_RESULT([yes])
        nasm_feature_level=10
      else
        AC_MSG_RESULT([no])
      fi
    fi

    AS=nasm
    as_feature_level=$nasm_feature_level

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

  if test $as_feature_level -lt 10 ; then
    AC_MSG_ERROR([No modern nasm found as required. Nasm should be v2.14.01 or later.])
  fi

  case $host_os in
       *linux*)  arch=linux   asm_args="-f elf64";;
       *darwin*) arch=darwin  asm_args="-f macho64 --prefix=_ ";;
       *netbsd*) arch=netbsd  asm_args="-f elf64";;
       *mingw*)  arch=mingw   asm_args="-f win64";;
       *)        arch=unknown asm_args="-f elf64";;
  esac

  AM_CONDITIONAL(USE_NASM, test x"$AS" = x"nasm")
  AC_SUBST([asm_args])
  AM_CONDITIONAL(DARWIN, test x"$arch" = x"darwin")
  AC_MSG_RESULT([Using $AS args target "$arch" "$asm_args"])
else
  # Disable below conditionals if not x86
  AM_CONDITIONAL(USE_NASM, test "x" = "y")
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
-fstack-protector \
-D_FORTIFY_SOURCE=2 \
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
