#!/usr/bin/env bash

set -e #exit on fail

# Override defaults if exist
READLINK=readlink
command -V greadlink >/dev/null 2>&1 && READLINK=greadlink

# Run in build directory
out="$PWD"
src=$($READLINK -f $(dirname $0))/..
cd "$src"

# Echo environment info
if test -d .git; then
    branch=$(git describe --always)
    commitid=$(git rev-parse HEAD)
    brief=$(git log -1 --format='%s')
    branch_changes=$(git diff --shortstat)
fi
if command -V uname >/dev/null 2>&1; then
    node=$(uname -n)
    os_name=$(uname -s)
    os_all=$(uname -a)
fi

echo "Test report v1"
echo "branch:              $branch"
echo "brief:               $brief"
echo "commitid:            $commitid"
echo "node:                $node"
echo "os_name:             $os_name"
echo "os_all:              $os_all"
echo "test_args:           $@"
echo "changes:             $branch_changes"
command -V lscpu > /dev/null 2>&1 && lscpu

# Start tests

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

