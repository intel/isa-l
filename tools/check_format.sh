#!/usr/bin/env bash

set -e
rc=0
verbose=0
indent_args='-npro -kr -i8 -ts8 -sob -l95 -ss -ncs -cp1 -lps'

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

if hash indent && indent --version | grep -q GNU; then
    echo "Checking C files for coding style..."
    for f in `git ls-files '*.c'`; do
	[ "$verbose" -gt 0 ] 2> /dev/null && echo "checking $f"
	if ! indent $indent_args -st $f | diff -q $f - >& /dev/null; then
	    echo "  File found with formatting issues: $f"
	    [ "$verbose" -gt 0 ] 2> /dev/null && indent $indent_args -st $f | diff -u $f -
	    rc=1
	fi
    done
    [ "$rc" -gt 0 ] && echo "  Run ./tools/iindent on files"
else
	echo "You do not have indent installed so your code style is not being checked!"
fi

if hash grep; then
    echo "Checking for dos and whitespace violations..."
    for f in `git ls-files '*.c' '*.h' '*.asm' '*.inc' '*.am' '*.txt' '*.md' '*.def' '*.ac' '*.sh' '*.in' 'Makefile*' `; do
	[ "$verbose" -gt 0 ] 2> /dev/null && echo "checking $f"
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

echo "Checking for signoff in commit message..."
if ! git log -n 1 --format=%B | grep -q "^Signed-off-by:" ; then
    echo "  Commit not signed off. Please read src/CONTRIBUTING.md"
    rc=1
fi

[ "$rc" -gt 0 ] && echo Format Fail || echo Format Pass

exit $rc
