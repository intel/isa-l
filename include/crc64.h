/**********************************************************************
  Copyright(c) 2011-2016 Intel Corporation All rights reserved.

  Redistribution and use in source and binary forms, with or without
  modification, are permitted provided that the following conditions
  are met:
    * Redistributions of source code must retain the above copyright
      notice, this list of conditions and the following disclaimer.
    * Redistributions in binary form must reproduce the above copyright
      notice, this list of conditions and the following disclaimer in
      the documentation and/or other materials provided with the
      distribution.
    * Neither the name of Intel Corporation nor the names of its
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


/**
 *  @file  crc64.h
 *  @brief CRC64 functions.
 */


#ifndef _CRC64_H_
#define _CRC64_H_

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif


/* Multi-binary functions */

/**
 * @brief Generate CRC from ECMA-182 standard in reflected format, runs
 * appropriate version.
 *
 * This function determines what instruction sets are enabled and
 * selects the appropriate version at runtime.
 * @returns 64 bit CRC
 */
uint64_t crc64_ecma_refl(
	uint64_t init_crc,        //!< initial CRC value, 64 bits
	const unsigned char *buf, //!< buffer to calculate CRC on
	uint64_t len              //!< buffer length in bytes (64-bit data)
	);

/**
 * @brief Generate CRC from ECMA-182 standard in normal format, runs
 * appropriate version.
 *
 * This function determines what instruction sets are enabled and
 * selects the appropriate version at runtime.
 * @returns 64 bit CRC
 */
uint64_t crc64_ecma_norm(
	uint64_t init_crc,        //!< initial CRC value, 64 bits
	const unsigned char *buf, //!< buffer to calculate CRC on
	uint64_t len              //!< buffer length in bytes (64-bit data)
	);

/**
 * @brief Generate CRC from ISO standard in reflected format, runs
 * appropriate version.
 *
 * This function determines what instruction sets are enabled and
 * selects the appropriate version at runtime.
 * @returns 64 bit CRC
 */
uint64_t crc64_iso_refl(
	uint64_t init_crc,        //!< initial CRC value, 64 bits
	const unsigned char *buf, //!< buffer to calculate CRC on
	uint64_t len              //!< buffer length in bytes (64-bit data)
	);

/**
 * @brief Generate CRC from ISO standard in normal format, runs
 * appropriate version.
 *
 * This function determines what instruction sets are enabled and
 * selects the appropriate version at runtime.
 * @returns 64 bit CRC
 */
uint64_t crc64_iso_norm(
	uint64_t init_crc,        //!< initial CRC value, 64 bits
	const unsigned char *buf, //!< buffer to calculate CRC on
	uint64_t len              //!< buffer length in bytes (64-bit data)
	);

/**
 * @brief Generate CRC from "Jones" coefficients in reflected format, runs
 * appropriate version.
 *
 * This function determines what instruction sets are enabled and
 * selects the appropriate version at runtime.
 * @returns 64 bit CRC
 */
uint64_t crc64_jones_refl(
	uint64_t init_crc,        //!< initial CRC value, 64 bits
	const unsigned char *buf, //!< buffer to calculate CRC on
	uint64_t len              //!< buffer length in bytes (64-bit data)
	);

/**
 * @brief Generate CRC from "Jones" coefficients in normal format, runs
 * appropriate version.
 *
 * This function determines what instruction sets are enabled and
 * selects the appropriate version at runtime.
 * @returns 64 bit CRC
 */
uint64_t crc64_jones_norm(
	uint64_t init_crc,        //!< initial CRC value, 64 bits
	const unsigned char *buf, //!< buffer to calculate CRC on
	uint64_t len              //!< buffer length in bytes (64-bit data)
	);

/* Arch specific versions */

/**
 * @brief Generate CRC from ECMA-182 standard in reflected format.
 * @requires SSE3, CLMUL
 *
 * @returns 64 bit CRC
 */

uint64_t crc64_ecma_refl_by8(
	uint64_t init_crc,        //!< initial CRC value, 64 bits
	const unsigned char *buf, //!< buffer to calculate CRC on
	uint64_t len              //!< buffer length in bytes (64-bit data)
	);

/**
 * @brief Generate CRC from ECMA-182 standard in normal format.
 * @requires SSE3, CLMUL
 *
 * @returns 64 bit CRC
 */

uint64_t crc64_ecma_norm_by8(
	uint64_t init_crc,        //!< initial CRC value, 64 bits
	const unsigned char *buf, //!< buffer to calculate CRC on
	uint64_t len              //!< buffer length in bytes (64-bit data)
	);

/**
 * @brief Generate CRC from ECMA-182 standard in reflected format, runs baseline version
 * @returns 64 bit CRC
 */
uint64_t crc64_ecma_refl_base(
	uint64_t init_crc,        //!< initial CRC value, 64 bits
	const unsigned char *buf, //!< buffer to calculate CRC on
	uint64_t len              //!< buffer length in bytes (64-bit data)
	);

/**
 * @brief Generate CRC from ECMA-182 standard in normal format, runs baseline version
 * @returns 64 bit CRC
 */
uint64_t crc64_ecma_norm_base(
	uint64_t init_crc,        //!< initial CRC value, 64 bits
	const unsigned char *buf, //!< buffer to calculate CRC on
	uint64_t len              //!< buffer length in bytes (64-bit data)
	);

/**
 * @brief Generate CRC from ISO standard in reflected format.
 * @requires SSE3, CLMUL
 *
 * @returns 64 bit CRC
 */

uint64_t crc64_iso_refl_by8(
	uint64_t init_crc,        //!< initial CRC value, 64 bits
	const unsigned char *buf, //!< buffer to calculate CRC on
	uint64_t len              //!< buffer length in bytes (64-bit data)
	);

/**
 * @brief Generate CRC from ISO standard in normal format.
 * @requires SSE3, CLMUL
 *
 * @returns 64 bit CRC
 */

uint64_t crc64_iso_norm_by8(
	uint64_t init_crc,        //!< initial CRC value, 64 bits
	const unsigned char *buf, //!< buffer to calculate CRC on
	uint64_t len              //!< buffer length in bytes (64-bit data)
	);

/**
 * @brief Generate CRC from ISO standard in reflected format, runs baseline version
 * @returns 64 bit CRC
 */
uint64_t crc64_iso_refl_base(
	uint64_t init_crc,        //!< initial CRC value, 64 bits
	const unsigned char *buf, //!< buffer to calculate CRC on
	uint64_t len              //!< buffer length in bytes (64-bit data)
	);

/**
 * @brief Generate CRC from ISO standard in normal format, runs baseline version
 * @returns 64 bit CRC
 */
uint64_t crc64_iso_norm_base(
	uint64_t init_crc,        //!< initial CRC value, 64 bits
	const unsigned char *buf, //!< buffer to calculate CRC on
	uint64_t len              //!< buffer length in bytes (64-bit data)
	);

/**
 * @brief Generate CRC from "Jones" coefficients in reflected format.
 * @requires SSE3, CLMUL
 *
 * @returns 64 bit CRC
 */

uint64_t crc64_jones_refl_by8(
	uint64_t init_crc,        //!< initial CRC value, 64 bits
	const unsigned char *buf, //!< buffer to calculate CRC on
	uint64_t len              //!< buffer length in bytes (64-bit data)
	);

/**
 * @brief Generate CRC from "Jones" coefficients in normal format.
 * @requires SSE3, CLMUL
 *
 * @returns 64 bit CRC
 */

uint64_t crc64_jones_norm_by8(
	uint64_t init_crc,        //!< initial CRC value, 64 bits
	const unsigned char *buf, //!< buffer to calculate CRC on
	uint64_t len              //!< buffer length in bytes (64-bit data)
	);

/**
 * @brief Generate CRC from "Jones" coefficients in reflected format, runs baseline version
 * @returns 64 bit CRC
 */
uint64_t crc64_jones_refl_base(
	uint64_t init_crc,        //!< initial CRC value, 64 bits
	const unsigned char *buf, //!< buffer to calculate CRC on
	uint64_t len              //!< buffer length in bytes (64-bit data)
	);

/**
 * @brief Generate CRC from "Jones" coefficients in normal format, runs baseline version
 * @returns 64 bit CRC
 */
uint64_t crc64_jones_norm_base(
	uint64_t init_crc,        //!< initial CRC value, 64 bits
	const unsigned char *buf, //!< buffer to calculate CRC on
	uint64_t len              //!< buffer length in bytes (64-bit data)
	);

#ifdef __cplusplus
}
#endif

#endif // _CRC64_H_
