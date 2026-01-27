/**************************************************************
  Copyright 2025 Amazon.com, Inc. or its affiliates.

  Redistribution and use in source and binary forms, with or without
  modification, are permitted provided that the following conditions
  are met:
    * Redistributions of source code must retain the above copyright
      notice, this list of conditions and the following disclaimer.
    * Redistributions in binary form must reproduce the above copyright
      notice, this list of conditions and the following disclaimer in
      the documentation and/or other materials provided with the
      distribution.
    * Neither the name of Amazon.com, Inc. nor the names of its
      contributors may be used to endorse or promote products derived
      from this software without specific prior written permission.

  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
  "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
  LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
  A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
  OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
  SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
  LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
  DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
  THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
  (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
  OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
**********************************************************************/
#include <arm_sve.h>
#include <stdint.h>

// This implementation of the nvect_dot_prod uses several techniques for optimization:
//
//  1. Instead of a separate assembly implementation for each n-vect function, a single
//     implementation in C can be optimized by the compiler to produce all of the versions.
//     This is accomplished with a static function with the main implementation and
//     non-static (i.e. exported) functions with the nvect argument hard coded. The compiler
//     will inline the main function into each exported function and discard unused portions
//     of the code.
//
//  2. SVE data types cannot be used in arrays since their sizes are not known. Instead
//     split them out into separate variables and use switch-case blocks to do what we
//     would normally do with a simple loop over an array. This also ensures that the
//     compiler does not use loops in the output.
//
//  3. Additional loop unrolling: in addition to unrolling to the vector width, we also
//     unroll 4x more and process 4x the vector width in each iteration of the loop.
//
//  4. A second version of each function is built with +sve2. SVE2 introduces the EOR3
//     instruction which allows consolidation of some of the XOR operations. The compiler
//     can do this automatically in optimization so a separate implementation isn't required.
//     We simply allow the compiler to generate SVE2 versions as well.

__attribute__((target("+sve")))
#include "gf_nvect_dot_prod_sve.h"

// Optimized wrapper functions
__attribute__((target("+sve"))) void
gf_vect_dot_prod_sve(int len, int vlen, unsigned char *gftbls, unsigned char **src,
                     unsigned char *dest)
{
        unsigned char *dest_array[1] = { dest };
        gf_nvect_dot_prod_sve_unrolled(len, vlen, gftbls, src, dest_array, 1);
}

__attribute__((target("+sve"))) void
gf_2vect_dot_prod_sve(int len, int vlen, unsigned char *gftbls, unsigned char **src,
                      unsigned char **dest)
{
        gf_nvect_dot_prod_sve_unrolled(len, vlen, gftbls, src, dest, 2);
}

__attribute__((target("+sve"))) void
gf_3vect_dot_prod_sve(int len, int vlen, unsigned char *gftbls, unsigned char **src,
                      unsigned char **dest)
{
        gf_nvect_dot_prod_sve_unrolled(len, vlen, gftbls, src, dest, 3);
}

__attribute__((target("+sve"))) void
gf_4vect_dot_prod_sve(int len, int vlen, unsigned char *gftbls, unsigned char **src,
                      unsigned char **dest)
{
        gf_nvect_dot_prod_sve_unrolled(len, vlen, gftbls, src, dest, 4);
}

__attribute__((target("+sve"))) void
gf_5vect_dot_prod_sve(int len, int vlen, unsigned char *gftbls, unsigned char **src,
                      unsigned char **dest)
{
        gf_nvect_dot_prod_sve_unrolled(len, vlen, gftbls, src, dest, 5);
}

__attribute__((target("+sve"))) void
gf_6vect_dot_prod_sve(int len, int vlen, unsigned char *gftbls, unsigned char **src,
                      unsigned char **dest)
{
        gf_nvect_dot_prod_sve_unrolled(len, vlen, gftbls, src, dest, 6);
}

__attribute__((target("+sve"))) void
gf_7vect_dot_prod_sve(int len, int vlen, unsigned char *gftbls, unsigned char **src,
                      unsigned char **dest)
{
        gf_nvect_dot_prod_sve_unrolled(len, vlen, gftbls, src, dest, 7);
}

// SVE2 wrapper functions - compiler will optimize eor to eor3 automatically

#define gf_nvect_dot_prod_sve_unrolled gf_nvect_dot_prod_sve_unrolled2

__attribute__((target("+sve+sve2")))
#include "gf_nvect_dot_prod_sve.h"

__attribute__((target("+sve+sve2"))) void
gf_vect_dot_prod_sve2(int len, int vlen, unsigned char *gftbls, unsigned char **src,
                      unsigned char *dest)
{
        unsigned char *dest_array[1] = { dest };
        gf_nvect_dot_prod_sve_unrolled(len, vlen, gftbls, src, dest_array, 1);
}

__attribute__((target("+sve+sve2"))) void
gf_2vect_dot_prod_sve2(int len, int vlen, unsigned char *gftbls, unsigned char **src,
                       unsigned char **dest)
{
        gf_nvect_dot_prod_sve_unrolled(len, vlen, gftbls, src, dest, 2);
}

__attribute__((target("+sve+sve2"))) void
gf_3vect_dot_prod_sve2(int len, int vlen, unsigned char *gftbls, unsigned char **src,
                       unsigned char **dest)
{
        gf_nvect_dot_prod_sve_unrolled(len, vlen, gftbls, src, dest, 3);
}

__attribute__((target("+sve+sve2"))) void
gf_4vect_dot_prod_sve2(int len, int vlen, unsigned char *gftbls, unsigned char **src,
                       unsigned char **dest)
{
        gf_nvect_dot_prod_sve_unrolled(len, vlen, gftbls, src, dest, 4);
}

__attribute__((target("+sve+sve2"))) void
gf_5vect_dot_prod_sve2(int len, int vlen, unsigned char *gftbls, unsigned char **src,
                       unsigned char **dest)
{
        gf_nvect_dot_prod_sve_unrolled(len, vlen, gftbls, src, dest, 5);
}

__attribute__((target("+sve+sve2"))) void
gf_6vect_dot_prod_sve2(int len, int vlen, unsigned char *gftbls, unsigned char **src,
                       unsigned char **dest)
{
        gf_nvect_dot_prod_sve_unrolled(len, vlen, gftbls, src, dest, 6);
}

__attribute__((target("+sve+sve2"))) void
gf_7vect_dot_prod_sve2(int len, int vlen, unsigned char *gftbls, unsigned char **src,
                       unsigned char **dest)
{
        gf_nvect_dot_prod_sve_unrolled(len, vlen, gftbls, src, dest, 7);
}

#undef gf_nvect_dot_prod_sve_unrolled
