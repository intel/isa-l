#!/usr/bin/env bash

set -e
rc=0
verbose=0
clang_format_min_version=18

function clang_format_version() {
    version_str=$($clang_format --version)
    regex="[0-9]+"
    if [[ $version_str =~ $regex ]]; then
        major_version="${BASH_REMATCH[0]}"
	echo $major_version
    fi
}

# set clang-format binary if not set externally
if [[ -z $CLANGFORMAT ]]; then
    clang_format="clang-format"
else
    clang_format=$CLANGFORMAT
fi

echo "Clang-format version: " $(clang_format_version)

while [ -n "$*" ]; do
    case "$1" in
	-v )
	    verbose=1
	    shift
	    ;;
	-h )
	    echo check_format.sh [-h -v]
	    exit 0
	    ;;
    esac
done

echo "Checking format of files in the git index at $PWD"
if ! git rev-parse --is-inside-work-tree >& /dev/null; then
    echo "Not in a git repo: Fail"
    exit 1
fi

if [ $(clang_format_version) -ge $clang_format_min_version ]; then
    echo "Checking C files for coding style (clang-format v$(clang_format_version))..."
    for f in `git ls-files '*.[c|h]'`; do
	[ "$verbose" -gt 0 ] && echo "checking style on $f"
	if ! $clang_format -style=file --dry-run --Werror "$f" >/dev/null 2>&1; then
	    echo "  File found with formatting issues: $f"
	    [ "$verbose" -gt 0 ] && $clang_format -style=file --dry-run "$f"
	    rc=1
	fi
    done
    [ "$rc" -gt 0 ] && echo "  Run ./tools/format.sh to fix formatting issues"
else
    echo "You do not have clang-format version ${clang_format_min_version}+" \
	"installed so your code style is not being checked!"
fi

if hash grep; then
    echo "Checking for dos and whitespace violations..."
    for f in $(git ls-files); do
	[ "$verbose" -gt 0 ] && echo "checking whitespace on $f"
	if grep -q '[[:space:]]$' $f ; then
	    echo "  File found with trailing whitespace: $f"
	    rc=1
	fi
	if grep -q $'\r' $f ; then
	    echo "  File found with dos formatting: $f"
	    rc=1
	fi
    done
fi

echo "Checking source files for permissions..."
while read -r perm _res0 _res1 f; do
    [ -z "$f" ] && continue
    [ "$verbose" -gt 0 ] && echo "checking permissions on $f"
    if [ "$perm" -ne 100644 ]; then
	echo "  File found with permissions issue ($perm): $f"
	rc=1
    fi
done <<< $(git ls-files -s -- ':(exclude)*.sh')

echo "Checking script files for permissions..."
while read -r perm _res0 _res1 f; do
    [ -z "$f" ] && continue
    [ "$verbose" -gt 0 ] && echo "checking permissions on $f"
    if [ "$perm" -ne 100755 ]; then
	echo "  Script found with permissions issue ($perm): $f"
	rc=1
    fi
done <<< $(git ls-files -s '*.sh')


echo "Checking for signoff in commit message..."
if ! git log -n 1 --format=%B --no-merges | grep -q "^Signed-off-by:" ; then
    echo "  Commit not signed off. Please read src/CONTRIBUTING.md"
    rc=1
fi

[ "$rc" -gt 0 ] && echo Format Fail || echo Format Pass

exit $rc
