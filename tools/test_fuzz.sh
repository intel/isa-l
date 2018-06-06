#!/usr/bin/env bash

usage ()
{
test_ids=$(echo "${llvm_all_ids[*]}" | sed 's/ /, /g')
cat << EOF
usage: $0 options
options:
   -h                 Help
   -l, --llvm <n>     Use llvm fuzz tests and run n times 0=just build, -1=skip (default $use_llvm).
   -a, --afl  <n>     Use AFL fuzz tests and run n times 0=just build, -1=skip (default $use_afl).
   -t, --time <n>     Run each group of max time <n>[s,h,m,d] - n seconds, hours, minutes or days.
   -e <exec|rand|all> Run a specific llvm test or [$test_ids, rand, all].
   -f <file>          Use this file as initial raw input.  Can be repeated.
   -d <0,1>           Use dump of internal inflate test corpus (default $use_internal_corp).
   -i <dir>           Fuzz input dir (default $fuzzin_dir).
   -o <dir>           Fuzz output dir (default $fuzzout_dir).
EOF
exit 0
}

# Defaults
use_afl=-1
use_llvm=1
samp_files=
use_internal_corp=0
fuzzin_dir=fuzzin
fuzzout_dir=fuzzout
llvm_opts=" -print_final_stats=1"
afl_timeout_cmd=""
run_secs=0
llvm_tests=("igzip_simple_inflate_fuzz_test")
llvm_all_ids=("simple" "checked" "round_trip")
llvm_all_tests=("igzip_simple_inflate_fuzz_test" "igzip_checked_inflate_fuzz_test" "igzip_simple_round_trip_fuzz_test")

# Options
while [ "$1" != "${1##-}" ]; do
    case $1 in
	-h | --help)
	    usage
	    ;;
	-t | --time)
	    run_secs=$(echo $2 | sed -e 's/d$/*24h/' -e 's/h$/*60m/' -e 's/m$/*60/' -e 's/s$//'| bc)
	    llvm_opts+=" -max_total_time=$run_secs"
	    afl_timeout_cmd="timeout --preserve-status $run_secs"
	    echo Run each for $run_secs seconds
	    shift 2
	    ;;
	-a | --afl)
	    use_afl=$2
	    shift 2
	    ;;
	-l | --llvm)
	    use_llvm=$2
	    shift 2
	    ;;
	-f)
	    samp_files+="$2 "
	    use_internal_corp=0
	    shift 2
	    ;;
	-d)
	    use_internal_corp=$2
	    shift 2
	    ;;
	-e)
	    case $2 in
		all)
		    llvm_tests=${llvm_all_tests[@]}
		    ;;
		rand)
		    llvm_tests=${llvm_all_tests[$RANDOM % ${#llvm_all_tests[@]} ]}
		    ;;
		*)
		    flag=0
		    for id_index in "${!llvm_all_ids[@]}"; do
			 if [[ "${llvm_all_ids[$id_index]}" = "$2" ]]; then
			     flag=1
			     llvm_tests[0]="${llvm_all_tests[$id_index]}"
			     break;
			 fi
		    done

		    if [ $flag -eq 0 ]; then
			test_ids=$(echo "${llvm_all_ids[*]}" | sed 's/ /, /g')
			echo "Invalid test, valid options: $test_ids, rand, or all"
			exit 0
		    fi
		    ;;
	    esac
	    shift 2
	    ;;
	-i)
	    fuzzin_dir=$2
	    shift 2
	    ;;
	-o)
	    fuzzout_dir=$2
	    shift 2
	    ;;
    esac
done

set -xe #exit on fail

# Optionally build afl fuzz tests
if [ $use_afl -ge 0 ]; then
    echo Build afl fuzz tests
    if ! command -V afl-gcc > /dev/null; then
	echo $0 option --afl requires package afl installed
	exit 0
    fi
    make -f Makefile.unx clean
    make -f Makefile.unx units=igzip CC=afl-gcc other
fi

# Optionally build llvm fuzz tests
if [ $use_llvm -ge 0 ]; then
    echo Build llvm fuzz tests
    if ( command -V clang++ > /dev/null ); then
	if (echo int LLVMFuzzerTestOneInput\(\)\{return 0\;\} | clang++ -x c - -fsanitize=fuzzer,address -lpthread -o /dev/null >& /dev/null); then
	    echo have modern clang
	    llvm_link_args='FUZZLINK=-fsanitize=fuzzer,address'
	elif (echo int LLVMFuzzerTestOneInput\(\)\{return 0\;\} | clang++ -x c - -lFuzzer -lpthread -o /dev/null >& /dev/null); then
	    echo have libFuzzer
	    llvm_link_args='FUZZLINK=-lFuzzer'
	else
	    echo $0 option --llvm requires clang++ and libFuzzer
	    exit 0
	fi
    fi
    rm -rf bin
    make -f Makefile.unx units=igzip llvm_fuzz_tests igzip_dump_inflate_corpus CC=clang CXX=clang++ ${llvm_link_args}
fi

#Create fuzz input/output directories
mkdir -p $fuzzin_dir
if [ $use_afl -ge 0 ]; then
       mkdir -p $fuzzout_dir
fi

# Optionally fill fuzz input with internal tests corpus
[ $use_internal_corp -gt 0 ] && ./igzip_dump_inflate_corpus $fuzzin_dir

# Optionally compress input samples as input into fuzz dir
for f in $samp_files; do
    echo Using sample file $f
    f_base=`basename $f`
    ./igzip_stateless_file_perf $f -o $fuzzin_dir/samp_${f_base}_cmp
done

# Optionally run tests alternately one after the other
while [ $use_llvm -gt 0 -o $use_afl -gt 0 ]; do
    if [ $use_afl -gt 0 ]; then
	echo afl run $use_afl
	let use_afl--
	$afl_timeout_cmd afl-fuzz -T "Run inflate $run_secs s" -i $fuzzin_dir -o $fuzzout_dir -M fuzzer1 -- ./igzip_fuzz_inflate @@
	afl-whatsup $fuzzout_dir
    fi

    if [ $use_llvm -gt 0 ]; then
	echo llvm run $use_llvm
	let use_llvm--
	for test in $llvm_tests; do
	    echo "Run llvm test $test"
	    ./$test $fuzzin_dir $llvm_opts
	done
    fi
done

make -f Makefile.unx clean
