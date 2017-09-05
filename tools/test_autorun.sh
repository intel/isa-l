#!/usr/bin/env bash

set -e #exit on fail

# Override defaults if exist
READLINK=readlink
command -V greadlink >/dev/null 2>&1 && READLINK=greadlink


out="$PWD"
src=$($READLINK -f $(dirname $0))/..
cd "$src"

[ -z "$1" ] && ./tools/test_checks.sh

while [ -n "$1" ]; do
    case "$1" in
	check )
	    ./tools/test_checks.sh
	    shift ;;
	ext )
	    ./tools/test_extended.sh
	    shift ;;
	format )
	    shift ;;
	all )
	    ./tools/test_checks.sh
	    ./tools/test_extended.sh
	    shift ;;
	* )
	    echo $0 undefined option: $1
	    shift ;;
    esac
done

./tools/check_format.sh

