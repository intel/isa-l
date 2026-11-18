#!/usr/bin/env bash

set -e
export LC_ALL=C

if [[ $# -ne 1 || ( $1 != --check && $1 != --fix ) ]]; then
    echo "Usage: $0 --check|--fix" >&2
    exit 1
fi

mode=$1
repo_root=$(git rev-parse --show-toplevel)
cd "$repo_root"

characters=(
    $'\xe2\x80\x94' $'\xe2\x80\x93' $'\xe2\x86\x92' $'\xc2\xa0'
    $'\xe2\x80\x8b' $'\xef\xbb\xbf' $'\xe2\x80\x9c' $'\xe2\x80\x9d'
    $'\xe2\x80\x98' $'\xe2\x80\x99' $'\xe2\x80\xa6'
)
replacements=('-' '-' '->' ' ' '' '' '"' '"' "'" "'" '...')
labels=(
    'Em dash (U+2014)' 'En dash (U+2013)' 'Right arrow (U+2192)'
    'Non-breaking space (U+00A0)' 'Zero-width space (U+200B)' 'BOM (U+FEFF)'
    'Left double quote (U+201C)' 'Right double quote (U+201D)'
    'Left single quote (U+2018)' 'Right single quote (U+2019)' 'Ellipsis (U+2026)'
)

patterns=()
substitutions=()
for index in "${!characters[@]}"; do
    patterns+=(-e "${characters[index]}")
    substitutions+=(-e "s/${characters[index]}/${replacements[index]}/g")
done

rc=0
while IFS= read -r -d '' file; do
    [[ -f $file && ! -L $file ]] || continue
    if ! grep -IqF "${patterns[@]}" -- "$file"; then
        continue
    fi

    if [[ $mode == --fix ]]; then
        sed -i "${substitutions[@]}" -- "$file"
        printf 'FIXED: %s\n' "$file"
    else
        for index in "${!characters[@]}"; do
            if grep -qF -- "${characters[index]}" "$file"; then
                printf 'ERROR: %s: %s\n' "$file" "${labels[index]}"
                grep -nF -- "${characters[index]}" "$file"
            fi
        done
        rc=1
    fi
done < <(git ls-files -z)

if [[ $mode == --check && $rc -eq 0 ]]; then
    echo "PASS: no special characters found."
fi
exit "$rc"