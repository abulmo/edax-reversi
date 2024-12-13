/**
 * @file bit.h
 *
 * Bitwise operations header file.
 *
 * @date 1998 - 2024
 * @author Richard Delorme
 * @author Toshihiko Okuhara
 * @version 4.6
 */

#ifndef EDAX_BIT_H
#define EDAX_BIT_H

#include <stdio.h>
#include <stdint.h>

#ifndef __has_builtin  // Compatibility with non-clang compilers.
	#define __has_builtin(x) 0
#endif

struct Random;

/* declaration */
int bit_count_32(const uint32_t);
int bit_count_64(const uint64_t);
#define bit_count(b) _Generic((b),\
	int:bit_count_32(b), unsigned int:bit_count_32(b), \
	long:bit_count_64(b), unsigned long:bit_count_64(b), \
	long long:bit_count_64(b), unsigned long long:bit_count_64(b))
int bit_weighted_count(const uint64_t);

uint16_t bswap_16(uint16_t);
uint32_t bswap_32(uint32_t);
uint64_t bswap_64(uint64_t);
#define bswap(b) _Generic((b), \
	short:bswap_16(b), unsigned short: bswap_16(b), \
	int:bswap_32(b), unsigned int:bswap_32(b), \
	long:bswap_64(b), unsigned long:bswap_64(b), \
	long long: bswap_64(b), unsigned long long:bswap_64(b)

int bit_leading_zeros_32(const uint32_t);
int bit_leading_zeros_64(const uint64_t);
#define bit_leading_zeros(b) _Generic((b),\
	int:bit_leading_zeros_32(b), unsigned int:bit_leading_zeros_32(b), \
	long:bit_leading_zeros_64(b), unsigned long:bit_leading_zeros_64(b), \
	long long:bit_leading_zeros_64(b), unsigned long long:bit_leading_zeros_64(b))

uint8_t bit_rotate_left_8(const uint8_t, const int);
uint16_t bit_rotate_left_16(const uint16_t, const int);
uint32_t bit_rotate_left_32(const uint32_t, const int);
uint64_t bit_rotate_left_64(const uint64_t, const int);
#define bit_rotate_left(b, n) _Generic((b), \
	unsigned char: bit_rotate_left_8(b, n), \
	unsigned short: bit_rotate_left_16(b, n), \
	unsigned int:bit_rotate_left_32(b, n), \
	unsigned long:bit_rotate_left_(b, n), \
	unsigned long long:bit_rotate_left_64(b, n))

#define bit_rotate_right(b, n) _Generic((b), \
	unsigned char: bit_rotate_left_8(8 - (b), n), \
	unsigned short: bit_rotate_left_16(16 - (b), n), \
	unsigned int:bit_rotate_left_32(32 - (b), n), \
	unsigned long:bit_rotate_left_64(64 - (b), n), \
	unsigned long long:bit_rotate_left_64(64 - (b), n))

int first_bit(uint64_t);
int next_bit(uint64_t*);
int last_bit(uint64_t);
uint64_t transpose(uint64_t);
#define vertical_mirror(t) bswap_64((t))
uint64_t horizontal_mirror(uint64_t);
int get_rand_bit(uint64_t, struct Random*);
void bitboard_print(const uint64_t, FILE*);
void bit_test(void);

/** Loop over each bit set. */
#define foreach_bit(i, b) for (i = first_bit(b); b; i = next_bit(&b))

extern const uint64_t X_TO_BIT[];
/** Return a bitboard with bit x set. */
#define x_to_bit(x) X_TO_BIT[x]

//#define x_to_bit(x) (1ULL << (x)) // 1% slower on Sandy Bridge

#endif

