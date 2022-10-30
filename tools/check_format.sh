#!/usr/bin/env bash

set -e
rc=0
verbose=0
# NOTE: there is a bug in GNU indent command line parse where it says, that
#       -il6 require numeric parameter. This is because it treat it like "-i"
#       param. Here we pass -i8 which is default for linux code style and
#       we use long parameter name for indent label.
indent_args='-i8 -linux -l95 -cp1 -lps -ncs --indent-label6'
function iver { printf "%03d%03d%03d%03d" $(echo "$@" | sed 's/^.* indent//; y/./ /'); }

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

# On FreeBSD we need to use gindent
for indent_tool in indent gindent ''; do
	if hash $indent_tool && [ $(iver $($indent_tool --version)) -ge $(iver 2.2.12) ]; then
		break
	fi
done

if [ -n "$indent_tool" ]; then
    echo "Checking C files for coding style..."
    for f in `git ls-files '*.c'`; do
	[ "$verbose" -gt 0 ] && echo "checking style on $f"
	if ! $indent_tool $indent_args -st $f | diff -q $f - >& /dev/null; then
	    echo "  File found with formatting issues: $f"
	    [ "$verbose" -gt 0 ] 2> /dev/null && $indent_tool $indent_args -st $f | diff -u $f -
	    rc=1
	fi
    done
    [ "$rc" -gt 0 ] && echo "  Run ./tools/iindent on files"
else
	echo "You do not have a recent indent installed so your code style is not being checked!"
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
done <<< $(git ls-files -s -- ':(exclude)*.sh' ':(exclude)*iindent')

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
