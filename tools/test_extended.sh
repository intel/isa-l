#!/usr/bin/env bash

# Extended tests: Run a few more options other than make check

set -xe #exit on fail

# Defaults
cpus=1
S=$RANDOM
MAKE=make
READLINK=readlink
test_level=check
build_opt=''
msg=''

# Override defaults if exist
command -V gmake >/dev/null 2>&1 && MAKE=gmake
command -V greadlink >/dev/null 2>&1 && READLINK=greadlink
[ -n "$CC" ] && build_opt+="CC=$CC "
[ -n "$AS" ] && build_opt+="AS=$AS "

out="$PWD"
src=$($READLINK -f $(dirname $0))/..
source $src/tools/test_tools.sh
cd "$src"

# Run on mult cpus
if command -V lscpu >/dev/null 2>&1; then
    cpus=`lscpu -p | tail -1 | cut -d, -f 2`
    cpus=$(($cpus + 1))
elif command -V sysctl; then
    if sysctl -n hw.ncpu >/dev/null 2>&1; then
	cpus=$(sysctl -n hw.ncpu)
	cpus=$(($cpus + 1))
    fi
fi
echo "Using $cpus cpu threads"

if [ -z "$S" ]; then
    S=`tr -cd 0-9 </dev/urandom | head -c 4 | sed -e 's/^0*/1/g'`
    [ "$S" -gt 0 ] 2> /dev/null || S="123"
fi
msg+="Running with TEST_SEED=$S".$'\n'

# Fix Darwin issues
if uname | grep -q 'Darwin' 2>&1; then
    export SED=`which sed`
fi

# Check for test libs to add
if command -V ldconfig >/dev/null 2>&1; then
    if ldconfig -p | grep -q libz.so; then
	test_level=test
	msg+=$'With extra tests\n'
    fi
    if ldconfig -p | grep -q libefence.so; then
	build_opt+="LDFLAGS+='-lefence' "
	msg+=$'With efence\n'
    fi
fi

# Std makefile build test
$MAKE -f Makefile.unx clean
test_start "extended_build_test"
time $MAKE -f Makefile.unx -j $cpus $build_opt
test_end "extended_build_test" $?
msg+=$'Std makefile build: Pass\n'

# Check for gnu executable stack set
if command -V readelf >/dev/null 2>&1; then
    test_start "stack_nx_check"
    if readelf -W -l bin/libisal.so | grep 'GNU_STACK' | grep -q 'RWE'; then
	echo $0: Stack NX check bin/libisal.so: Fail
	test_end "stack_nx_check" 1
	exit 1
    else
	test_end "stack_nx_check" 0
	msg+=$'Stack NX check bin/lib/libisal.so: Pass\n'
    fi
else
    msg+=$'Stack NX check not supported: Skip\n'
fi

# Std makefile build perf tests
test_start "extended_perf_test"
time $MAKE -f Makefile.unx -j $cpus perfs
test_end "extended_perf_test" $?
msg+=$'Std makefile build perf: Pass\n'

# Std makefile run tests
test_start "extended_makefile_tests"
time $MAKE -f Makefile.unx -j $cpus $build_opt D="TEST_SEED=$S" $test_level
test_end "extended_makefile_tests" $?
msg+=$'Std makefile tests: Pass\n'

# Std makefile build other
test_start "extended_other_tests"
time $MAKE -f Makefile.unx -j $cpus $build_opt D="TEST_SEED=$S" other
test_end "extended_other_tests" $?
msg+=$'Other tests build: Pass\n'

# Try to pick a random src file
if command -V shuf >/dev/null 2>&1; then
    in_file=$(find $src -type f -size +0 -name \*.c -o -name \*.asm -print 2>/dev/null | shuf | head -1 );
else
    in_file=configure.ac
fi

echo Other tests using $in_file
test_start "igzip_file_perf"
./igzip_file_perf $in_file
test_end "igzip_file_perf" $?
test_start "igzip_hist_perf"
./igzip_hist_perf $in_file
test_end "igzip_hist_perf" $?
test_start "igzip_semi_dyn_file_perf"
./igzip_semi_dyn_file_perf $in_file
test_end "igzip_semi_dyn_file_perf" $?
test_start "igzip_fuzz_inflate"
./igzip_fuzz_inflate $in_file
test_end "igzip_fuzz_inflate" $?
msg+=$'Other tests run: Pass\n'

if command -V shuf >/dev/null 2>&1; then
    in_files=$(find $src -type f -size +0 -print 2>/dev/null | shuf | head -10 );
    test_start "igzip_rand_test"
    ./igzip_rand_test $in_files
    test_end "igzip_rand_test" $?
    test_start "igzip_inflate_test"
    ./igzip_inflate_test $in_files
    test_end "igzip_inflate_test" $?
    msg+=$'Compression file tests: Pass\n'
else
    msg+=$'Compression file test: Skip\n'
fi

time $MAKE -f Makefile.unx -j $cpus $build_opt ex
msg+=$'Examples build: Pass\n'

test_start "ec_simple_example"
./ec_simple_example -r $S
test_end "ec_simple_example" $?
test_start "crc_simple_test"
./crc_simple_test
test_end "crc_simple_test" $?
test_start "crc64_example"
./crc64_example
test_end "crc64_example" $?
test_start "xor_example"
./xor_example
test_end "xor_example" $?
test_start "igzip_example"
./igzip_example ${in_file} ${in_file}.cmp
test_end "igzip_example" $?
rm -rf ${in_file}.cmp
msg+=$'Examples run: Pass\n'

# Test custom hufftables
test_start "generate_custom_hufftables"
./generate_custom_hufftables $in_file
$MAKE -f Makefile.unx clean
$MAKE -f Makefile.unx -j $cpus D="NO_STATIC_INFLATE_H" checks
./igzip_rand_test $in_file
rm -rf hufftables_c.c
test_end "generate_custom_hufftables" $?

msg+=$'Custom hufftable build: Pass\n'

$MAKE -f Makefile.unx clean

test_start "nmake_file_consistency"
$MAKE -f Makefile.unx host_cpu="x86_64" test_nmake_file
test_end "nmake_file_consistency" $?
msg+=$'Nmake file consistency: Pass\n'

# noarch build
test_start "noarch_build"
time $MAKE -f Makefile.unx -j $cpus arch=noarch $build_opt
test_end "noarch_build" $?
test_start "noarch_build_random"
time $MAKE -f Makefile.unx -j $cpus arch=noarch $build_opt D="TEST_SEED=$S" check
test_end "noarch_build_random" $?
$MAKE -f Makefile.unx arch=noarch clean
msg+=$'Noarch build: Pass\n'

# Try mingw build
if [ $(uname -m) == "x86_64" ] && command -V x86_64-w64-mingw32-gcc >/dev/null 2>&1; then
    test_start "mingw_build"
    time $MAKE -f Makefile.unx -j $cpus arch=mingw
    test_end "mingw_build" $?
    msg+=$'Mingw build: Pass\n'

    if command -V wine >/dev/null 2>&1; then
	test_start "mingw_check_tests"
	time $MAKE -f Makefile.unx -j $cpus arch=mingw D="TEST_SEED=$S" check
	test_end "mingw_check_tests" $?
	msg+=$'Mingw check tests: Pass\n'
    else
	msg+=$'No wine, mingw check: Skip\n'
    fi
    $MAKE -f Makefile.unx  arch=mingw clean
else
    msg+=$'No mingw build: Skip\n'
fi

set +x
echo
echo "Summary test $0:"
echo "Build opt: $build_opt"
echo "$msg"
echo "$0: Final: Pass"
