#!/usr/bin/env bash

set -xe #exit on fail

# Defaults
cpus=1
S=$RANDOM
MAKE=make
READLINK=readlink

# Override defaults if exist
command -V gmake >/dev/null 2>&1 && MAKE=gmake
command -V greadlink >/dev/null 2>&1 && READLINK=greadlink

out="$PWD"
src=$($READLINK -f $(dirname $0))/..
cd "$src"
tmp_install_dir=$out/tmp_install

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

# Pick a random test seed
if [ -z "$S" ]; then
    S=`tr -cd 0-9 </dev/urandom | head -c 4 | sed -e 's/^0*/1/g'`
    [ "$S" -gt 0 ] 2> /dev/null || S="123"
fi
echo "Running with TEST_SEED=$S"

# Fix Darwin issues
if uname | grep -q 'Darwin' 2>&1; then
    export SED=`which sed`
fi

# Build and run check tests
time ./autogen.sh
time ./configure --prefix=$tmp_install_dir $opt_config_target
time $MAKE -j $cpus
time $MAKE check -j $cpus D="-D TEST_SEED=$S"

# Build other tests if deps found
if command -V ldconfig >/dev/null 2>&1; then
    if ldconfig -p | grep -q libz.so; then
	time $MAKE other -j $cpus
	time $MAKE ex    -j $cpus
	time $MAKE tests -j $cpus
    fi
fi
time $MAKE install

# Check for gnu executable stack set
if command -V readelf >/dev/null 2>&1; then
    if readelf -W -l $tmp_install_dir/lib/libisal.so | grep 'GNU_STACK' | grep -q 'RWE'; then
	echo Stack NX check $tmp_install_dir/lib/libisal.so Fail
	exit 1
    else
	echo Stack NX check $tmp_install_dir/lib/libisal.so Pass
    fi
else
    echo Stack NX check not supported
fi

$MAKE clean

# Check that make clean did not leave any junk behind
if git status > /dev/null 2>&1; then
    if git status --porcelain --ignored | grep -x '.*\.o\|.*\.lo\|.*\.a\|.*\.la\|.*\.s'; then
	echo Clean directory check Fail
	exit 1
    else
	echo Clean directory check Pass
    fi
else
    echo Clean directory check not supported
fi


echo $0: Pass
