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
#ifdef __APPLE__
#include <arm_sme.h>
#define ARM_STREAMING __arm_streaming
#else
#include <arm_sve.h>
#define ARM_STREAMING
#endif
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

#ifdef __APPLE__
__attribute__((target("+sme"), always_inline))
#else
__attribute__((target("+sve"), always_inline))
#endif
static inline void
gf_nvect_dot_prod_sve_unrolled(int len, int vlen, unsigned char *gftbls, unsigned char **src,
                               unsigned char **dest, int nvect) ARM_STREAMING
{
        if (len < 16)
                return;

        const svuint8_t mask0f = svdup_u8(0x0f);
        const svbool_t predicate_true = svptrue_b8();
        int sve_len = svcntb();
        int pos = 0;

        // 4x unrolled main loop - SVE predicates handle ALL remaining data automatically
        while (pos < len) {
                // Create predicates for 4 batches - SVE masks beyond array bounds
                svbool_t predicate_0 = svwhilelt_b8_s32(pos + sve_len * 0, len);
                svbool_t predicate_1 = svwhilelt_b8_s32(pos + sve_len * 1, len);
                svbool_t predicate_2 = svwhilelt_b8_s32(pos + sve_len * 2, len);
                svbool_t predicate_3 = svwhilelt_b8_s32(pos + sve_len * 3, len);

                // Exit if no active lanes in first predicate
                if (!svptest_any(predicate_true, predicate_0))
                        break;

                // Initialize destination accumulators - use individual variables
                svuint8_t dest_acc0_0, dest_acc0_1, dest_acc0_2, dest_acc0_3, dest_acc0_4,
                        dest_acc0_5, dest_acc0_6;
                svuint8_t dest_acc1_0, dest_acc1_1, dest_acc1_2, dest_acc1_3, dest_acc1_4,
                        dest_acc1_5, dest_acc1_6;
                svuint8_t dest_acc2_0, dest_acc2_1, dest_acc2_2, dest_acc2_3, dest_acc2_4,
                        dest_acc2_5, dest_acc2_6;
                svuint8_t dest_acc3_0, dest_acc3_1, dest_acc3_2, dest_acc3_3, dest_acc3_4,
                        dest_acc3_5, dest_acc3_6;

                // Initialize based on nvect
                switch (nvect) {
                case 7:
                        dest_acc0_6 = dest_acc1_6 = dest_acc2_6 = dest_acc3_6 =
                                svdup_u8(0); // fallthrough
                case 6:
                        dest_acc0_5 = dest_acc1_5 = dest_acc2_5 = dest_acc3_5 =
                                svdup_u8(0); // fallthrough
                case 5:
                        dest_acc0_4 = dest_acc1_4 = dest_acc2_4 = dest_acc3_4 =
                                svdup_u8(0); // fallthrough
                case 4:
                        dest_acc0_3 = dest_acc1_3 = dest_acc2_3 = dest_acc3_3 =
                                svdup_u8(0); // fallthrough
                case 3:
                        dest_acc0_2 = dest_acc1_2 = dest_acc2_2 = dest_acc3_2 =
                                svdup_u8(0); // fallthrough
                case 2:
                        dest_acc0_1 = dest_acc1_1 = dest_acc2_1 = dest_acc3_1 =
                                svdup_u8(0); // fallthrough
                case 1:
                        dest_acc0_0 = dest_acc1_0 = dest_acc2_0 = dest_acc3_0 = svdup_u8(0);
                        break;
                }

                // Process all source vectors
                for (int v = 0; v < vlen; v++) {
                        // Load 4 batches of source data
                        svuint8_t src_data0 = svld1_u8(predicate_0, &src[v][pos + sve_len * 0]);
                        svuint8_t src_data1 = svld1_u8(predicate_1, &src[v][pos + sve_len * 1]);
                        svuint8_t src_data2 = svld1_u8(predicate_2, &src[v][pos + sve_len * 2]);
                        svuint8_t src_data3 = svld1_u8(predicate_3, &src[v][pos + sve_len * 3]);

                        // Extract nibbles for all batches
                        svuint8_t src_lo0 = svand_x(predicate_0, src_data0, mask0f);
                        svuint8_t src_hi0 = svlsr_x(predicate_0, src_data0, 4);
                        svuint8_t src_lo1 = svand_x(predicate_1, src_data1, mask0f);
                        svuint8_t src_hi1 = svlsr_x(predicate_1, src_data1, 4);
                        svuint8_t src_lo2 = svand_x(predicate_2, src_data2, mask0f);
                        svuint8_t src_hi2 = svlsr_x(predicate_2, src_data2, 4);
                        svuint8_t src_lo3 = svand_x(predicate_3, src_data3, mask0f);
                        svuint8_t src_hi3 = svlsr_x(predicate_3, src_data3, 4);

                        // Process each destination with unrolled batches
                        for (int d = 0; d < nvect; d++) {
                                unsigned char *tbl_base = &gftbls[d * vlen * 32 + v * 32];
                                svuint8_t tbl_lo = svld1_u8(predicate_true, tbl_base);
                                svuint8_t tbl_hi = svld1_u8(predicate_true, tbl_base + 16);

                                // Batch 0
                                svuint8_t gf_lo0 = svtbl_u8(tbl_lo, src_lo0);
                                svuint8_t gf_hi0 = svtbl_u8(tbl_hi, src_hi0);

                                // Batch 1
                                svuint8_t gf_lo1 = svtbl_u8(tbl_lo, src_lo1);
                                svuint8_t gf_hi1 = svtbl_u8(tbl_hi, src_hi1);

                                // Batch 2
                                svuint8_t gf_lo2 = svtbl_u8(tbl_lo, src_lo2);
                                svuint8_t gf_hi2 = svtbl_u8(tbl_hi, src_hi2);

                                // Batch 3
                                svuint8_t gf_lo3 = svtbl_u8(tbl_lo, src_lo3);
                                svuint8_t gf_hi3 = svtbl_u8(tbl_hi, src_hi3);

                                svuint8_t gf_result0 = sveor_x(predicate_0, gf_lo0, gf_hi0);
                                svuint8_t gf_result1 = sveor_x(predicate_1, gf_lo1, gf_hi1);
                                svuint8_t gf_result2 = sveor_x(predicate_2, gf_lo2, gf_hi2);
                                svuint8_t gf_result3 = sveor_x(predicate_3, gf_lo3, gf_hi3);

                                // Accumulate results
                                switch (d) {
                                case 0:
                                        dest_acc0_0 = sveor_x(predicate_0, dest_acc0_0, gf_result0);
                                        dest_acc1_0 = sveor_x(predicate_1, dest_acc1_0, gf_result1);
                                        dest_acc2_0 = sveor_x(predicate_2, dest_acc2_0, gf_result2);
                                        dest_acc3_0 = sveor_x(predicate_3, dest_acc3_0, gf_result3);
                                        break;
                                case 1:
                                        dest_acc0_1 = sveor_x(predicate_0, dest_acc0_1, gf_result0);
                                        dest_acc1_1 = sveor_x(predicate_1, dest_acc1_1, gf_result1);
                                        dest_acc2_1 = sveor_x(predicate_2, dest_acc2_1, gf_result2);
                                        dest_acc3_1 = sveor_x(predicate_3, dest_acc3_1, gf_result3);
                                        break;
                                case 2:
                                        dest_acc0_2 = sveor_x(predicate_0, dest_acc0_2, gf_result0);
                                        dest_acc1_2 = sveor_x(predicate_1, dest_acc1_2, gf_result1);
                                        dest_acc2_2 = sveor_x(predicate_2, dest_acc2_2, gf_result2);
                                        dest_acc3_2 = sveor_x(predicate_3, dest_acc3_2, gf_result3);
                                        break;
                                case 3:
                                        dest_acc0_3 = sveor_x(predicate_0, dest_acc0_3, gf_result0);
                                        dest_acc1_3 = sveor_x(predicate_1, dest_acc1_3, gf_result1);
                                        dest_acc2_3 = sveor_x(predicate_2, dest_acc2_3, gf_result2);
                                        dest_acc3_3 = sveor_x(predicate_3, dest_acc3_3, gf_result3);
                                        break;
                                case 4:
                                        dest_acc0_4 = sveor_x(predicate_0, dest_acc0_4, gf_result0);
                                        dest_acc1_4 = sveor_x(predicate_1, dest_acc1_4, gf_result1);
                                        dest_acc2_4 = sveor_x(predicate_2, dest_acc2_4, gf_result2);
                                        dest_acc3_4 = sveor_x(predicate_3, dest_acc3_4, gf_result3);
                                        break;
                                case 5:
                                        dest_acc0_5 = sveor_x(predicate_0, dest_acc0_5, gf_result0);
                                        dest_acc1_5 = sveor_x(predicate_1, dest_acc1_5, gf_result1);
                                        dest_acc2_5 = sveor_x(predicate_2, dest_acc2_5, gf_result2);
                                        dest_acc3_5 = sveor_x(predicate_3, dest_acc3_5, gf_result3);
                                        break;
                                case 6:
                                        dest_acc0_6 = sveor_x(predicate_0, dest_acc0_6, gf_result0);
                                        dest_acc1_6 = sveor_x(predicate_1, dest_acc1_6, gf_result1);
                                        dest_acc2_6 = sveor_x(predicate_2, dest_acc2_6, gf_result2);
                                        dest_acc3_6 = sveor_x(predicate_3, dest_acc3_6, gf_result3);
                                        break;
                                }
                        }
                }

                // Store results for all batches
                switch (nvect) {
                case 7:
                        svst1_u8(predicate_0, &dest[6][pos + sve_len * 0], dest_acc0_6);
                        svst1_u8(predicate_1, &dest[6][pos + sve_len * 1], dest_acc1_6);
                        svst1_u8(predicate_2, &dest[6][pos + sve_len * 2], dest_acc2_6);
                        svst1_u8(predicate_3, &dest[6][pos + sve_len * 3], dest_acc3_6);
                        // fallthrough
                case 6:
                        svst1_u8(predicate_0, &dest[5][pos + sve_len * 0], dest_acc0_5);
                        svst1_u8(predicate_1, &dest[5][pos + sve_len * 1], dest_acc1_5);
                        svst1_u8(predicate_2, &dest[5][pos + sve_len * 2], dest_acc2_5);
                        svst1_u8(predicate_3, &dest[5][pos + sve_len * 3], dest_acc3_5);
                        // fallthrough
                case 5:
                        svst1_u8(predicate_0, &dest[4][pos + sve_len * 0], dest_acc0_4);
                        svst1_u8(predicate_1, &dest[4][pos + sve_len * 1], dest_acc1_4);
                        svst1_u8(predicate_2, &dest[4][pos + sve_len * 2], dest_acc2_4);
                        svst1_u8(predicate_3, &dest[4][pos + sve_len * 3], dest_acc3_4);
                        // fallthrough
                case 4:
                        svst1_u8(predicate_0, &dest[3][pos + sve_len * 0], dest_acc0_3);
                        svst1_u8(predicate_1, &dest[3][pos + sve_len * 1], dest_acc1_3);
                        svst1_u8(predicate_2, &dest[3][pos + sve_len * 2], dest_acc2_3);
                        svst1_u8(predicate_3, &dest[3][pos + sve_len * 3], dest_acc3_3);
                        // fallthrough
                case 3:
                        svst1_u8(predicate_0, &dest[2][pos + sve_len * 0], dest_acc0_2);
                        svst1_u8(predicate_1, &dest[2][pos + sve_len * 1], dest_acc1_2);
                        svst1_u8(predicate_2, &dest[2][pos + sve_len * 2], dest_acc2_2);
                        svst1_u8(predicate_3, &dest[2][pos + sve_len * 3], dest_acc3_2);
                        // fallthrough
                case 2:
                        svst1_u8(predicate_0, &dest[1][pos + sve_len * 0], dest_acc0_1);
                        svst1_u8(predicate_1, &dest[1][pos + sve_len * 1], dest_acc1_1);
                        svst1_u8(predicate_2, &dest[1][pos + sve_len * 2], dest_acc2_1);
                        svst1_u8(predicate_3, &dest[1][pos + sve_len * 3], dest_acc3_1);
                        // fallthrough
                case 1:
                        svst1_u8(predicate_0, &dest[0][pos + sve_len * 0], dest_acc0_0);
                        svst1_u8(predicate_1, &dest[0][pos + sve_len * 1], dest_acc1_0);
                        svst1_u8(predicate_2, &dest[0][pos + sve_len * 2], dest_acc2_0);
                        svst1_u8(predicate_3, &dest[0][pos + sve_len * 3], dest_acc3_0);
                        break;
                }

                pos += sve_len * 4;
        }
}

// Optimized wrapper functions
#ifdef __APPLE__
__attribute__((target("+sme")))
#else
__attribute__((target("+sve")))
#endif
void
gf_vect_dot_prod_sve(int len, int vlen, unsigned char *gftbls, unsigned char **src,
                     unsigned char *dest) ARM_STREAMING
{
        unsigned char *dest_array[1] = { dest };
        gf_nvect_dot_prod_sve_unrolled(len, vlen, gftbls, src, dest_array, 1);
}

#ifdef __APPLE__
__attribute__((target("+sme")))
#else
__attribute__((target("+sve")))
#endif
void
gf_2vect_dot_prod_sve(int len, int vlen, unsigned char *gftbls, unsigned char **src,
                      unsigned char **dest) ARM_STREAMING
{
        gf_nvect_dot_prod_sve_unrolled(len, vlen, gftbls, src, dest, 2);
}

#ifdef __APPLE__
__attribute__((target("+sme")))
#else
__attribute__((target("+sve")))
#endif
void
gf_3vect_dot_prod_sve(int len, int vlen, unsigned char *gftbls, unsigned char **src,
                      unsigned char **dest) ARM_STREAMING
{
        gf_nvect_dot_prod_sve_unrolled(len, vlen, gftbls, src, dest, 3);
}

#ifdef __APPLE__
__attribute__((target("+sme")))
#else
__attribute__((target("+sve")))
#endif
void
gf_4vect_dot_prod_sve(int len, int vlen, unsigned char *gftbls, unsigned char **src,
                      unsigned char **dest) ARM_STREAMING
{
        gf_nvect_dot_prod_sve_unrolled(len, vlen, gftbls, src, dest, 4);
}

#ifdef __APPLE__
__attribute__((target("+sme")))
#else
__attribute__((target("+sve")))
#endif
void
gf_5vect_dot_prod_sve(int len, int vlen, unsigned char *gftbls, unsigned char **src,
                      unsigned char **dest) ARM_STREAMING
{
        gf_nvect_dot_prod_sve_unrolled(len, vlen, gftbls, src, dest, 5);
}

#ifdef __APPLE__
__attribute__((target("+sme")))
#else
__attribute__((target("+sve")))
#endif
void
gf_6vect_dot_prod_sve(int len, int vlen, unsigned char *gftbls, unsigned char **src,
                      unsigned char **dest) ARM_STREAMING
{
        gf_nvect_dot_prod_sve_unrolled(len, vlen, gftbls, src, dest, 6);
}

#ifdef __APPLE__
__attribute__((target("+sme")))
#else
__attribute__((target("+sve")))
#endif
void
gf_7vect_dot_prod_sve(int len, int vlen, unsigned char *gftbls, unsigned char **src,
                      unsigned char **dest) ARM_STREAMING
{
        gf_nvect_dot_prod_sve_unrolled(len, vlen, gftbls, src, dest, 7);
}

// SVE2 wrapper functions - compiler will optimize eor to eor3 automatically
#ifdef __APPLE__
__attribute__((target("+sme+sme2")))
#else
__attribute__((target("+sve+sve2")))
#endif
void
gf_vect_dot_prod_sve2(int len, int vlen, unsigned char *gftbls, unsigned char **src,
                      unsigned char *dest) ARM_STREAMING
{
        unsigned char *dest_array[1] = { dest };
        gf_nvect_dot_prod_sve_unrolled(len, vlen, gftbls, src, dest_array, 1);
}

#ifdef __APPLE__
__attribute__((target("+sme+sme2")))
#else
__attribute__((target("+sve+sve2")))
#endif
void
gf_2vect_dot_prod_sve2(int len, int vlen, unsigned char *gftbls, unsigned char **src,
                       unsigned char **dest) ARM_STREAMING
{
        gf_nvect_dot_prod_sve_unrolled(len, vlen, gftbls, src, dest, 2);
}

#ifdef __APPLE__
__attribute__((target("+sme+sme2")))
#else
__attribute__((target("+sve+sve2")))
#endif
void
gf_3vect_dot_prod_sve2(int len, int vlen, unsigned char *gftbls, unsigned char **src,
                       unsigned char **dest) ARM_STREAMING
{
        gf_nvect_dot_prod_sve_unrolled(len, vlen, gftbls, src, dest, 3);
}

#ifdef __APPLE__
__attribute__((target("+sme+sme2")))
#else
__attribute__((target("+sve+sve2")))
#endif
void
gf_4vect_dot_prod_sve2(int len, int vlen, unsigned char *gftbls, unsigned char **src,
                       unsigned char **dest) ARM_STREAMING
{
        gf_nvect_dot_prod_sve_unrolled(len, vlen, gftbls, src, dest, 4);
}

#ifdef __APPLE__
__attribute__((target("+sme+sme2")))
#else
__attribute__((target("+sve+sve2")))
#endif
void
gf_5vect_dot_prod_sve2(int len, int vlen, unsigned char *gftbls, unsigned char **src,
                       unsigned char **dest) ARM_STREAMING
{
        gf_nvect_dot_prod_sve_unrolled(len, vlen, gftbls, src, dest, 5);
}

#ifdef __APPLE__
__attribute__((target("+sme+sme2")))
#else
__attribute__((target("+sve+sve2")))
#endif
void
gf_6vect_dot_prod_sve2(int len, int vlen, unsigned char *gftbls, unsigned char **src,
                       unsigned char **dest) ARM_STREAMING
{
        gf_nvect_dot_prod_sve_unrolled(len, vlen, gftbls, src, dest, 6);
}

#ifdef __APPLE__
__attribute__((target("+sme+sme2")))
#else
__attribute__((target("+sve+sve2")))
#endif
void
gf_7vect_dot_prod_sve2(int len, int vlen, unsigned char *gftbls, unsigned char **src,
                       unsigned char **dest) ARM_STREAMING
{
        gf_nvect_dot_prod_sve_unrolled(len, vlen, gftbls, src, dest, 7);
}
