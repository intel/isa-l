/**********************************************************************
  Copyright(c) 2011-2015 Intel Corporation All rights reserved.

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
 *  @file  crc.h
 *  @brief CRC functions.
 */


#ifndef _CRC_H_
#define _CRC_H_

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif


/* Multi-binary functions */

/**
 * @brief Generate CRC from the T10 standard, runs appropriate version.
 *
 * This function determines what instruction sets are enabled and
 * selects the appropriate version at runtime.
 *
 * @returns 16 bit CRC
 */
uint16_t crc16_t10dif(
	uint16_t init_crc,        //!< initial CRC value, 16 bits
	const unsigned char *buf, //!< buffer to calculate CRC on
	uint64_t len              //!< buffer length in bytes (64-bit data)
	);


/**
 * @brief Generate CRC and copy T10 standard, runs appropriate version.
 *
 * Stitched CRC + copy function.
 *
 * @returns 16 bit CRC
 */
uint16_t crc16_t10dif_copy(
	uint16_t init_crc,  //!< initial CRC value, 16 bits
	uint8_t *dst,       //!< buffer destination for copy
	uint8_t *src,       //!< buffer source to crc + copy
	uint64_t len        //!< buffer length in bytes (64-bit data)
	);


/**
 * @brief Generate CRC from the IEEE standard, runs appropriate version.
 *
 * This function determines what instruction sets are enabled and
 * selects the appropriate version at runtime.
 * Note: CRC32 IEEE standard is widely used in HDLC, Ethernet, Gzip and
 * many others. Its polynomial is 0x04C11DB7 in normal and 0xEDB88320
 * in reflection (or reverse). In ISA-L CRC, function crc32_ieee is
 * actually designed for normal CRC32 IEEE version. And function
 * crc32_gzip_refl is actually designed for reflected CRC32 IEEE.
 * These two versions of CRC32 IEEE are not compatible with each other.
 * Users who want to replace their not optimized crc32 ieee with ISA-L's
 * crc32 function should be careful of that.
 * Since many applications use CRC32 IEEE reflected version, Please have
 * a check whether crc32_gzip_refl is right one for you instead of
 * crc32_ieee.
 *
 * @returns 32 bit CRC
 */

uint32_t crc32_ieee(
	uint32_t init_crc,        //!< initial CRC value, 32 bits
	const unsigned char *buf, //!< buffer to calculate CRC on
	uint64_t len              //!< buffer length in bytes (64-bit data)
	);

/**
 * @brief Generate the customized CRC
 * based on RFC 1952 CRC (http://www.ietf.org/rfc/rfc1952.txt) standard,
 * runs appropriate version.
 *
 * This function determines what instruction sets are enabled and
 * selects the appropriate version at runtime.
 *
 * Note: CRC32 IEEE standard is widely used in HDLC, Ethernet, Gzip and
 * many others. Its polynomial is 0x04C11DB7 in normal and 0xEDB88320
 * in reflection (or reverse). In ISA-L CRC, function crc32_ieee is
 * actually designed for normal CRC32 IEEE version. And function
 * crc32_gzip_refl is actually designed for reflected CRC32 IEEE.
 * These two versions of CRC32 IEEE are not compatible with each other.
 * Users who want to replace their not optimized crc32 ieee with ISA-L's
 * crc32 function should be careful of that.
 * Since many applications use CRC32 IEEE reflected version, Please have
 * a check whether crc32_gzip_refl is right one for you instead of
 * crc32_ieee.
 *
 * @returns 32 bit CRC
 */
uint32_t crc32_gzip_refl(
	uint32_t init_crc,          //!< initial CRC value, 32 bits
	const unsigned char *buf, //!< buffer to calculate CRC on
	uint64_t len                //!< buffer length in bytes (64-bit data)
	);


/**
 * @brief ISCSI CRC function, runs appropriate version.
 *
 * This function determines what instruction sets are enabled and
 * selects the appropriate version at runtime.
 *
 * @returns 32 bit CRC
 */
unsigned int crc32_iscsi(
	unsigned char *buffer, //!< buffer to calculate CRC on
	int len,               //!< buffer length in bytes
	unsigned int init_crc  //!< initial CRC value
	);


/* Base functions */

/**
 * @brief ISCSI CRC function, baseline version
 * @returns 32 bit CRC
 */
unsigned int crc32_iscsi_base(
	unsigned char *buffer,	//!< buffer to calculate CRC on
	int len, 		//!< buffer length in bytes
	unsigned int crc_init	//!< initial CRC value
	);


/**
 * @brief Generate CRC from the T10 standard, runs baseline version
 * @returns 16 bit CRC
 */
uint16_t crc16_t10dif_base(
	uint16_t seed,	//!< initial CRC value, 16 bits
	uint8_t *buf,	//!< buffer to calculate CRC on
	uint64_t len 	//!< buffer length in bytes (64-bit data)
	);


/**
 * @brief Generate CRC and copy T10 standard, runs baseline version.
 * @returns 16 bit CRC
 */
uint16_t crc16_t10dif_copy_base(
	uint16_t init_crc,  //!< initial CRC value, 16 bits
	uint8_t *dst,       //!< buffer destination for copy
	uint8_t *src,       //!< buffer source to crc + copy
	uint64_t len        //!< buffer length in bytes (64-bit data)
	);


/**
 * @brief Generate CRC from the IEEE standard, runs baseline version
 * @returns 32 bit CRC
 */
uint32_t crc32_ieee_base(
	uint32_t seed, 	//!< initial CRC value, 32 bits
	uint8_t *buf,	//!< buffer to calculate CRC on
	uint64_t len 	//!< buffer length in bytes (64-bit data)
	);

/**
 * @brief Generate the customized CRC
 * based on RFC 1952 CRC (http://www.ietf.org/rfc/rfc1952.txt) standard,
 * runs baseline version
 * @returns 32 bit CRC
 */
uint32_t crc32_gzip_refl_base(
	uint32_t seed,	//!< initial CRC value, 32 bits
	uint8_t *buf,	//!< buffer to calculate CRC on
	uint64_t len	//!< buffer length in bytes (64-bit data)
	);


#ifdef __cplusplus
}
#endif

#endif // _CRC_H_
