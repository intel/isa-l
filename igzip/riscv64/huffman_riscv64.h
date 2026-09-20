#ifndef __HUFFMAN_RISCV64_H__
#define __HUFFMAN_RISCV64_H__

#ifdef __ASSEMBLY__

.macro load_hash_const prod:req
	li	\prod, 0xB2D06057
.endm

/* compute_hash() from igzip/huffman.h on a zero-extended 32-bit \data. */
.macro compute_hash hash:req, data:req, prod:req
	mul	\hash, \data, \prod	/* hash = data * PROD (mod 2^64)	*/
	srli	\hash, \hash, 16
	mul	\hash, \hash, \prod	/* wraps mod 2^64, as in C		*/
	srli	\hash, \hash, 16
	slli	\hash, \hash, 32	/* (uint32_t) hash 		*/
	srli	\hash, \hash, 32
.endm

#endif /* __ASSEMBLY__ */
#endif /* __HUFFMAN_RISCV64_H__ */
