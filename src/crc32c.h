/**
 * @file crc32c.h
 *
 * @date 1998 - 2024
 * @author Richard Delorme
 * @author Toshihiko Okuhara
 * @version 4.6
 */


#ifndef EDAX_CRC32C_H
#define EDAX_CRC32C_H

#include <stdint.h>

uint32_t crc32c_u8(uint32_t, const uint32_t);
uint32_t crc32c_u64(uint32_t, const uint64_t);

#endif

