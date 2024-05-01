#!/usr/bin/env bash

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

while [ -n "$*" ]; do
    case "$1" in
        -v )
            verbose=1
            shift
            ;;
        -h )
            echo format.sh [-h -v]
            exit 0
            ;;
    esac
done

if [ $(clang_format_version) -ge $clang_format_min_version ]; then
    echo "Formatting files using clang-format v$(clang_format_version)..."
    for f in `git ls-files '*.[c|h]'`; do
        [ "$verbose" -gt 0 ] && echo "formatting $f"
        $clang_format -style=file -i "$f" &
    done
else
    echo "clang-format version ${clang_format_min_version}+ is required!"
fi

# wait for background processes to finish
wait
