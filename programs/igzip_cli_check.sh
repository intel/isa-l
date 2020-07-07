#! /bin/bash
set -o pipefail

CWD=$PWD
SRC_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" >/dev/null && pwd )"
IGZIP="$SRC_DIR/igzip $@"
TEST_DIR="/tmp/igzip_cli_test_$$/"
TEST_FILE=$SRC_DIR/igzip
DIFF="diff -q"

mkdir -p $TEST_DIR
cd $TEST_DIR

cleanup ()
{
    cd $CWD
    rm -rf $TEST_DIR
    exit $1
}

clear_dir ()
{
    cd /tmp/
    rm -rf $TEST_DIR
    mkdir -p $TEST_DIR
    cd $TEST_DIR
}

pass_check()
{
    if [ $1 -eq 0 ]; then
	echo -e "\e[1;32mPass\e[0;39m: " $2
    else
	echo -e "\e[1;31mFail\e[0;39m: " $2
	cleanup 1
    fi
}

fail_check()
{
    if [ $1 -ne 0 ]; then
	echo -e "\e[1;32mPass\e[0;39m: " $2
    else
	echo -e "\e[1;31mFail\e[0;39m: " $2
	cleanup 1
    fi
}

file1=tmp
file2=jnk
file3=blah
bad_file=not_a_file
dir=this_is_a_directory

default_suffix=".gz"
ds=$default_suffix
gzip_standard_suffixes=(".gz" ".z")
bad_suffix=".bad"
custom_suffix=".custom"

# Test basic compression and decompression
ret=0
cp $TEST_FILE $file1
$IGZIP $file1 && rm $file1 || ret=1
for suffix in ${gzip_standard_suffixes[@]}; do
    if [ "$ds" != "$suffix" ]; then
	cp -u $file1$ds $file1$suffix
    fi
    $IGZIP -d $file1$suffix && $DIFF $file1 $TEST_FILE || ret=1
    rm $file1
done
pass_check $ret "Basic compression and decompression"
clear_dir

# Test piping
cat $TEST_FILE | $IGZIP | $IGZIP -d | $DIFF $TEST_FILE - || ret=1
cat $TEST_FILE | $IGZIP - | $IGZIP -d - | $DIFF $TEST_FILE - || ret=1
pass_check $ret "Piping compression and decompression"

# Test multiple concatenated gzip files
ret=0
(for i in `seq 3`; do $IGZIP -c $TEST_FILE ; done) | $IGZIP -t || ret=1
pass_check $ret "Multiple gzip concatenated files"

if command -V md5sum >/dev/null 2>&1; then
    sum1=$((for i in `seq 15`; do $IGZIP -c $TEST_FILE; done) |  $IGZIP -cd | md5sum)
    sum2=$((for i in `seq 15`; do cat $TEST_FILE; done) | md5sum)
    [[ "$sum1" == "$sum2" ]] && ret=0 || ret=1
    pass_check $ret "Multiple large gzip concat test"
    clear_dir
else
    echo "Skip:  Multiple large gzip concat test"
fi


#Test outifle options
$IGZIP $TEST_FILE -o $file2$ds && $IGZIP $file2$ds -d -o $file1 && \
    test -f $file2$ds && test -f $file1 && $DIFF $TEST_FILE $file1
pass_check $? "Setting outfile name"
clear_dir

# Not a file test
ret=0
$IGZIP $bad_file &> /dev/null && ret=1
test -f $bad_file$ds && ret=1
pass_check $ret "Bad file"
clear_dir

# Multiple files
cp $TEST_FILE $file1 && cp $TEST_FILE $file2 && cp $TEST_FILE $file3 && \
    $IGZIP $file1 $file2 $file3 && rm $file1 && rm $file2 && rm $file3 && \
    $IGZIP -d $file1$ds $file2$ds $file3$ds && \
    $DIFF $TEST_FILE $file1 && $DIFF $TEST_FILE $file2 && $DIFF $TEST_FILE $file3
pass_check $? "Multiple files compression and decompression"
clear_dir

# Multiple files, one doesn't exist
ret=0
cp $TEST_FILE $file1 && cp $TEST_FILE $file2 || ret=1
$IGZIP $file1 $bad_file $file2 &> /dev/null && ret=1
rm $file1 && rm $file2 || ret=1
$IGZIP -d $file1$ds $bad_file$ds $file2$ds &> /dev/null && ret=1
$DIFF $TEST_FILE $file1 && $DIFF $TEST_FILE $file2 || ret=1
pass_check $ret "Multiple files with a bad file"
clear_dir

# Custom suffix test
cp $TEST_FILE $file1 && $IGZIP -S $custom_suffix $file1 &&  rm $file1 && \
    $IGZIP -d -S $custom_suffix $file1$custom_suffix && $DIFF $TEST_FILE $file1
pass_check $? "Custom suffix"

# Bad suffix test
ret=0
cp $TEST_FILE $file1 && $IGZIP -S $bad_suffix $file1 &&  rm $file1 || ret=1
$IGZIP -d $file1$custom_suffix &> /dev/null && ret=1
pass_check $ret "Bad suffix"
clear_dir

# Remove file test
ret=0
cp $TEST_FILE $file1 && $IGZIP --rm $file1 || ret=1
test -f $file1 && ret=1
$IGZIP --rm -d $file1$ds || ret=1
test -f $file1$ds && ret=1
pass_check $ret "Remove file"
clear_dir

# Pass a directory negative test
ret=0
mkdir -p $dir || ret=0
$IGZIP $dir &> /dev/null && ret=1
clear_dir

mkdir -p $dir$ds || ret=1
$IGZIP -d $dir &> /dev/null && ret=1
pass_check $ret "Compress/Decompress Directory without -r"
clear_dir

# Write permissions test
cp $TEST_FILE $file1
chmod 400 $file1
chmod 500 $TEST_DIR
$IGZIP $file1 &> /dev/null
fail_check $? "don't have write permissions"
chmod -R 700 $TEST_DIR
clear_dir

# Read permissions test
cp $TEST_FILE $file1
chmod 000 $file1
$IGZIP $file1 &> /dev/null
fail_check $? "don't have read permissions"
clear_dir

# File overwrite test -f
ret=0
cp $TEST_FILE $file1 && touch $file1$ds || ret=1
yes | $IGZIP $file1 &> /dev/null && ret=1
$IGZIP -f $file1 &> /dev/null && cp $file1$ds $file1 || ret=1
yes | $IGZIP -d $file1 &> /dev/null && ret=1
$IGZIP -df $file1$ds &> /dev/null && $DIFF $TEST_FILE $file1 || ret=1
pass_check $ret "Existing file overwrite only with force"
clear_dir

# Quiet suppresses interactivity
ret=0
cp $TEST_FILE $file1 && touch $file1$ds || ret=1
$IGZIP -q $file1 &> /dev/null && ret=1
$IGZIP -dq $file1 &> /dev/null && ret=1
pass_check $ret "Quiet will not overwrite"
clear_dir

# Input file and output file cannot be the same
ret=0
cp $TEST_FILE $file1 && $IGZIP $file1 -o $file1 &> /dev/null && ret=1
$DIFF $TEST_FILE $file1  &> /dev/null || ret=1
pass_check $ret "No in place compression"
clear_dir

# Input file and output file cannot be the same
ret=0
cp $TEST_FILE $file1 && $IGZIP $file1 -o $file1$ds &> /dev/null || ret=1
$IGZIP -do $file1 $file1 &> /dev/null && ret=1
$DIFF $TEST_FILE $file1  &> /dev/null || ret=1
pass_check $ret "No in place decompression"
clear_dir

ret=0
$IGZIP -n $TEST_FILE -o $file1$ds && $IGZIP -Nd $file1$ds && $DIFF $file1 $TEST_FILE || ret=1
pass_check $ret "Decompress name with no-name info"
clear_dir

ret=0
cp -p $TEST_FILE $file1 && sleep 1 &&\
$IGZIP -N $file1 -o $file1$ds &&  $IGZIP -Nfqd $file1$ds  || ret=1
TIME_ORIG=$(stat --printf=\("%Y\n"\) $TEST_FILE)
TIME_NEW=$(stat --printf=\("%Y\n"\) $file1)
if [ "$TIME_ORIG" != "$TIME_NEW" ] ; then
    ret=1
fi
pass_check $ret "Decompress with name info"
clear_dir

ret=0
cp -p $TEST_FILE $file1 && touch $file2\\
$IGZIP $file1 -o $file1$ds || ret=1
$IGZIP -t $file1$ds || ret=1
$IGZIP -t $file2 &> /dev/null && ret=1
cp $file1$ds $file2 && $IGZIP -t $file1$ds || ret=1
truncate -s -1 $file1$ds
$IGZIP -t $file1$ds &> /dev/null && ret=1
pass_check $ret "Test test"
clear_dir

# Large stream test with threading if enabled
ret=0
(for i in `seq 100`; do cat $TEST_FILE ; done) | $IGZIP -c -T 4 | $IGZIP -t || ret=1
pass_check $ret "Large stream test"


# Large stream tests with decompression and threading if enabled
if command -V md5sum >/dev/null 2>&1 && command -V dd >/dev/null 2>&1; then
    ret=0
    dd if=<(for i in `seq 1000`; do cat $TEST_FILE; done) iflag=fullblock bs=1M count=201 2> out.stder | tee >(md5sum > out.sum1) \
	| $IGZIP -c -T 4 | $IGZIP -d | md5sum > out.sum2 \
	&& $DIFF out.sum1 out.sum2 || ret=1
    pass_check $ret "Large stream compresss test"
    clear_dir

    if test -e /dev/urandom; then
	ret=0
	dd if=/dev/urandom iflag=fullblock bs=1M count=200 2> out.stder | tee >(md5sum > out.sum3) \
	    | $IGZIP -c -T 2 | $IGZIP -d | md5sum > out.sum4 \
	    && $DIFF out.sum3 out.sum4 || ret=1
	pass_check $ret "Large stream random data test"
	clear_dir
    fi
fi

echo "Passed all cli checks"
cleanup 0
