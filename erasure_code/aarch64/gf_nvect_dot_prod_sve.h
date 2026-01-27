static inline void
gf_nvect_dot_prod_sve_unrolled(int len, int vlen, unsigned char *gftbls, unsigned char **src,
                               unsigned char **dest, int nvect)
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
