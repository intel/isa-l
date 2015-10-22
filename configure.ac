#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ(2.69)
AC_INIT([libisal],
        [2.14.0],
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

# Check for programs
AC_PROG_CC_STDC
AC_USE_SYSTEM_EXTENSIONS
AM_SILENT_RULES([yes])
LT_INIT
AC_PREFIX_DEFAULT([/usr])
AC_PROG_SED
AC_PROG_MKDIR_P
AC_CHECK_PROG(HAVE_YASM, yasm, yes, no)
if test "$HAVE_YASM" = "no"; then
  AC_MSG_ERROR([yasm not found as required.])
fi
AC_MSG_CHECKING([checking for modern yasm])
AC_LANG_CONFTEST([AC_LANG_SOURCE([[vmovdqa %xmm0, %xmm1;]])])
if yasm -f elf64 -p gas conftest.c ; then
   AC_MSG_RESULT([yes])
else
   AC_MSG_FAILURE([need modern yasm])
fi

# Options
AC_ARG_ENABLE([debug],
        AS_HELP_STRING([--enable-debug], [enable debug messages @<:@default=disabled@:>@]),
        [], [enable_debug=no])
AS_IF([test "x$enable_debug" = "xyes"], [
        AC_DEFINE(ENABLE_DEBUG, [1], [Debug messages.])
])


case $target in
     *linux*)	arch=linux   yasm_args="-f elf64";;
     *darwin*)	arch=darwin  yasm_args="-f macho64 --prefix=_ ";;
     *netbsd*)	arch=netbsd  yasm_args="-f elf64";;
     *)		arch=unknown yasm_args="-f elf64";;
esac
AC_SUBST([yasm_args])
AM_CONDITIONAL(DARWIN, test x"$arch" = x"darwin")
AC_MSG_RESULT([Using yasm args target "$arch" "$yasm_args"])

# Check for header files
#AC_CHECK_HEADERS([limits.h stddef.h stdint.h stdlib.h string.h sys/time.h unistd.h])
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
AC_CHECK_FUNCS([memmove memset])

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
