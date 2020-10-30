# ISA-L Testing

Tests are divided into check tests, unit tests and fuzz tests. Check tests,
built with `make check`, should have no additional dependencies. Other unit
tests built with `make test` may have additional dependencies in order to make
comparisons of the output of ISA-L to other standard libraries and ensure
compatibility. Fuzz tests are meant to be run with a fuzzing tool such as [AFL]
or [llvm libFuzzer] fuzzing to direct the input data based on coverage. There
are a number of scripts in the /tools directory to help with automating the
running of tests.

## Test check

`./tools/test_autorun.sh` is a helper script for kicking off check tests, that
typically run for a few minutes, or extended tests that could run much
longer. The command `test_autorun.sh check` build and runs all check tests with
autotools and runs other short tests to ensure check tests, unit tests,
examples, install, exe stack, format are correct. Each run of `test_autorun.sh`
builds tests with a new random test seed that ensures that each run is unique to
the seed but deterministic for debugging. Tests are also built with sanitizers
and Electric Fence if available.

## Extended tests

Extended tests are initiated with the command `./tools/test_autorun.sh
ext`. These build and run check tests, unit tests, and other utilities that can
take much longer than check tests alone. This includes special compression tools
and some cross targets such as the no-arch build of base functions only and
mingw build if tools are available.

## Fuzz testing

`./tools/test_fuzz.sh` is a helper script for fuzzing to setup, build and run
the ISA-L inflate fuzz tests on multiple fuzz tools. Fuzzing with
[llvm libFuzzer] requires clang compiler tools with `-fsanitize=fuzzer` or
`libFuzzer` installed. You can invoke the default fuzz tests under llvm with

    ./tools/test_fuzz.sh -e checked

To use [AFL], install tools and system setup for `afl-fuzz` and run

    ./tools/test_fuzz.sh -e checked --afl 1 --llvm -1 -d 1

This uses internal vectors as a seed. You can also specify a sample file to use
as a seed instead with `-f <file>`. One of three fuzz tests can be invoked:
checked, simple, and round_trip.

[llvm libFuzzer]: https://llvm.org/docs/LibFuzzer.html
[AFL]: https://github.com/google/AFL
