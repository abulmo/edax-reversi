/**
 * @file bit.c
 *
 * Bitwise operations.
 * Several algorithms manipulating bits are presented here. Quite often,
 * a macro needs to be defined to chose between different flavors of the
 * algorithm.
 *
 * @date 1998 - 2024
 * @author Richard Delorme
 * @version 4.6
 */

#include "bit.h"
#include "util.h"

#include <assert.h>
#if __STDC_VERSION__ >= 202311L
	#include <stdbit.h>
#endif

/** coordinate to bit table converter */
const uint64_t X_TO_BIT[] = {
	0x0000000000000001ULL, 0x0000000000000002ULL, 0x0000000000000004ULL, 0x0000000000000008ULL,
	0x0000000000000010ULL, 0x0000000000000020ULL, 0x0000000000000040ULL, 0x0000000000000080ULL,
	0x0000000000000100ULL, 0x0000000000000200ULL, 0x0000000000000400ULL, 0x0000000000000800ULL,
	0x0000000000001000ULL, 0x0000000000002000ULL, 0x0000000000004000ULL, 0x0000000000008000ULL,
	0x0000000000010000ULL, 0x0000000000020000ULL, 0x0000000000040000ULL, 0x0000000000080000ULL,
	0x0000000000100000ULL, 0x0000000000200000ULL, 0x0000000000400000ULL, 0x0000000000800000ULL,
	0x0000000001000000ULL, 0x0000000002000000ULL, 0x0000000004000000ULL, 0x0000000008000000ULL,
	0x0000000010000000ULL, 0x0000000020000000ULL, 0x0000000040000000ULL, 0x0000000080000000ULL,
	0x0000000100000000ULL, 0x0000000200000000ULL, 0x0000000400000000ULL, 0x0000000800000000ULL,
	0x0000001000000000ULL, 0x0000002000000000ULL, 0x0000004000000000ULL, 0x0000008000000000ULL,
	0x0000010000000000ULL, 0x0000020000000000ULL, 0x0000040000000000ULL, 0x0000080000000000ULL,
	0x0000100000000000ULL, 0x0000200000000000ULL, 0x0000400000000000ULL, 0x0000800000000000ULL,
	0x0001000000000000ULL, 0x0002000000000000ULL, 0x0004000000000000ULL, 0x0008000000000000ULL,
	0x0010000000000000ULL, 0x0020000000000000ULL, 0x0040000000000000ULL, 0x0080000000000000ULL,
	0x0100000000000000ULL, 0x0200000000000000ULL, 0x0400000000000000ULL, 0x0800000000000000ULL,
	0x1000000000000000ULL, 0x2000000000000000ULL, 0x4000000000000000ULL, 0x8000000000000000ULL,
	0, 0 // <- hack for passing move & nomove
};

/** Conversion array: flippable neighbour bits
    https://eukaryote.hateblo.jp/entry/2020/04/26/031246
*/
const uint64_t NEIGHBOUR[] = {
	0x0000000000000302ULL, 0x0000000000000604ULL, 0x0000000000000e0aULL, 0x0000000000001c14ULL,
	0x0000000000003828ULL, 0x0000000000007050ULL, 0x0000000000006020ULL, 0x000000000000c040ULL,
	0x0000000000030200ULL, 0x0000000000060400ULL, 0x00000000000e0a00ULL, 0x00000000001c1400ULL,
	0x0000000000382800ULL, 0x0000000000705000ULL, 0x0000000000602000ULL, 0x0000000000c04000ULL,
	0x0000000003020300ULL, 0x0000000006040600ULL, 0x000000000e0a0e00ULL, 0x000000001c141c00ULL,
	0x0000000038283800ULL, 0x0000000070507000ULL, 0x0000000060206000ULL, 0x00000000c040c000ULL,
	0x0000000302030000ULL, 0x0000000604060000ULL, 0x0000000e0a0e0000ULL, 0x0000001c141c0000ULL,
	0x0000003828380000ULL, 0x0000007050700000ULL, 0x0000006020600000ULL, 0x000000c040c00000ULL,
	0x0000030203000000ULL, 0x0000060406000000ULL, 0x00000e0a0e000000ULL, 0x00001c141c000000ULL,
	0x0000382838000000ULL, 0x0000705070000000ULL, 0x0000602060000000ULL, 0x0000c040c0000000ULL,
	0x0003020300000000ULL, 0x0006040600000000ULL, 0x000e0a0e00000000ULL, 0x001c141c00000000ULL,
	0x0038283800000000ULL, 0x0070507000000000ULL, 0x0060206000000000ULL, 0x00c040c000000000ULL,
	0x0002030000000000ULL, 0x0004060000000000ULL, 0x000a0e0000000000ULL, 0x00141c0000000000ULL,
	0x0028380000000000ULL, 0x0050700000000000ULL, 0x0020600000000000ULL, 0x0040c00000000000ULL,
	0x0203000000000000ULL, 0x0406000000000000ULL, 0x0a0e000000000000ULL, 0x141c000000000000ULL,
	0x2838000000000000ULL, 0x5070000000000000ULL, 0x2060000000000000ULL, 0x40c0000000000000ULL,
	0, 0 // <- hack for passing move & nomove
};

/**
 * @brief Count the number of bits set to one in an uint64_t.
 *
 * This is the classical popcount function.
 * Since 2007, it is part of the instruction set of some modern CPU,
 * (>= barcelona for AMD or >= nelhacem for Intel). Alternatively,
 * a fast SWAR algorithm, adding bits in parallel is provided here.
 * This function is massively used to count discs on the board,
 * the mobility, or the stability.
 *
 * @param b 64-bit integer to count bits of.
 * @return the number of bits set.
 */
int bit_count_64(const uint64_t b)
{
	#if __STDC_VERSION__ >= 202311L

		return stdc_count_ones_ul(b);      // C23 version

	#elif defined(_MSC_VER) && defined(__POPCNT__)

		return __popcnt64(b);           // Microsoft Visual C/C++ version

	#elif defined(__GNUC__)

		return __builtin_popcountll(b); // GNUC/CLANG version

	#else
		uint64_t c = b
			- ((b >> 1) & 0x7777777777777777)
			- ((b >> 2) & 0x3333333333333333)
			- ((b >> 3) & 0x1111111111111111);
		c = ((c + (c >> 4)) & 0x0F0F0F0F0F0F0F0F) * 0x0101010101010101;

		return  (int)(c >> 56);
	#endif
}

/**
 * @brief Count the number of bits set to one in an uint32_t.
 *
 * This is the classical popcount function.
 * Since 2007, it is part of the instruction set of some modern CPU,
 * (>= barcelona for AMD or >= nelhacem for Intel). Alternatively,
 * a fast SWAR algorithm, adding bits in parallel is provided here.
 * This function is massively used to count discs on the board,
 * the mobility, or the stability.
 *
 * @param b 32-bit integer to count bits of.
 * @return the number of bits set.
 */
int bit_count_32(const uint32_t b)
{
	#if __STDC_VERSION__ >= 202311L

		return stdc_count_ones_ui(b);      // C23 version

	#elif defined(_MSC_VER) && defined(__POPCNT__)

		return __popcnt(b);           // Microsoft Visual C/C++ version

	#elif defined(__GNUC__)

		return __builtin_popcount(b); // GNUC/CLANG version

	#else
		uint32_t c = b
			- ((b >> 1) & 0x33333333)
			- ((b >> 1) & 0x11111111);
		c = ((c + (c >> 3)) & 0x0F0F0F0F) * 0x01010101;

		return  (int)(c >> 28);
	#endif
}

/**
 *  @brief count leading zeros for uint64_t
 *
 * @param b 64-bit integer
 * @ return number of zeroes
 */
int bit_leading_zeros_64(uint64_t b)
{
	#if __STDC_VERSION__ >= 202311L

		return stdc_leading_zeros_ul(b);      // C23 version

	#elif defined(_MSC_VER) && defined(__AVX2__)

		return __lzcnt64(b);           // Microsoft Visual C/C++ BMI1 version

	#elif defined(_MSC_VER)

		unsigned long index;
		if (_BitScanReverse64(&index, b))
			return 63 - (int) index;
		return 64;

	#elif defined(__GNUC__)

		return __builtin_clzl(b); // GNUC/CLANG version

	#else

	int n = 64;
	uint64_t c;

	c = b >> 32; if (c != 0) { n = n -32; b = c; }
	c = b >> 16; if (c != 0) { n = n -16; b = c; }
	c = b >>  8; if (c != 0) { n = n - 8; b = c; }
	c = b >>  4; if (c != 0) { n = n - 4; b = c; }
	c = b >>  2; if (c != 0) { n = n - 2; b = c; }
	c = b >>  1; if (c != 0) return n - 2;
	return n - b;


	#endif
}

/**
 *  @brief count leading zeros for uint32_t
 *
 * @param b 64-bit integer
 * @ return number of zeroes
 */
int bit_leading_zeros_32(uint32_t b)
{
	#if __STDC_VERSION__ >= 202311L

		return stdc_leading_zeros_ui(b);      // C23 version

	#elif defined(_MSC_VER) && defined(__AVX2__)

		return __lzcnt(b);           // Microsoft Visual C/C++ BMI1 version

	#elif defined(_MSC_VER)

		unsigned long index;
		if (_BitScanReverse(&index, b))
			return 31 - (int) index;
		return 32;

	#elif defined(__GNUC__)

		return __builtin_clz(b); // GNUC/CLANG version

	#else

	int n = 32;
	uint32_t c;

	c = b >> 16; if (c != 0) { n = n -16; b = c; }
	c = b >>  8; if (c != 0) { n = n - 8; b = c; }
	c = b >>  4; if (c != 0) { n = n - 4; b = c; }
	c = b >>  2; if (c != 0) { n = n - 2; b = c; }
	c = b >>  1; if (c != 0) return n - 2;
	return n - b;


	#endif
}


/**
 * @brief count the number of discs, counting the corners twice.
 *
 * This is a variation of the above algorithm to count the mobility and favour
 * the corners. This function is usefull for move sorting.
 *
 * @param v 64-bit integer to count bits of.
 * @return the number of bit set, counting the corners twice.
 */
int bit_weighted_count(const uint64_t v)
{
	return bit_count(v) + bit_count(v & 0x8100000000000081);
}

/**
 *
 * @brief check if a number has a single bit set,
 * i.e. is a power of two.
 *
 * @param b 64-bit number to test.
 * @return true if the number has a single bit.
 */
bool bit_is_single(uint64_t b)
{
	#if __STDC_VERSION__ >= 202311L

		return stdc_has_single_bit(b);

	#elif defined(__POPCNT__)

		return bit_count(b) == 1;

	#else

		return (b & (b - 1)) == 0;

	#endif
}


/**
 *
 * @brief Search the first bit set.
 *
 * On CPU with AMD64 or EMT64 instructions, a fast asm
 * code is provided. Alternatively, a fast algorithm based on
 * magic numbers is provided.
 *
 * @param b 64-bit integer.
 * @return the index of the first bit set.
 */
int first_bit(uint64_t b)
{
#if __STC_VERSION__ >= 202311L  // C23 version

	return std_first_leading_one(b);

#elif defined(_MSC_VER)        // Microsoft C/C++ version

	unsigned long index;
	_BitScanForward64(&index, b);
	return (int) index;

#elif defined(__GNUC__)        // GCC/Clang version

	return __builtin_ctzll(b);

#else                         // generic version

	static const unsigned char magic[64] = {
		63, 0, 58, 1, 59, 47, 53, 2,
		60, 39, 48, 27, 54, 33, 42, 3,
		61, 51, 37, 40, 49, 18, 28, 20,
		55, 30, 34, 11, 43, 14, 22, 4,
		62, 57, 46, 52, 38, 26, 32, 41,
		50, 36, 17, 19, 29, 10, 13, 21,
		56, 45, 25, 31, 35, 16, 9, 12,
		44, 24, 15, 8, 23, 7, 6, 5
	};

	return magic[((b & (-b)) * 0x07EDD5E59A4E28C2ULL) >> 58];

#endif
}

/**
 * @brief Search the next bit set.
 *
 * In practice, clear the first bit set and search the next one.
 *
 * @param b 64-bit integer.
 * @return the index of the next bit set.
 */
int next_bit(uint64_t *b)
{
	*b &= *b - 1;
	return first_bit(*b);
}

/**
 * @brief Search the last bit set (same as log2()).
 *
 * On CPU with AMD64 or EMT64 instructions, a fast asm
 * code is provided. Alternatively, a fast algorithm based on
 * magic numbers is provided.
 *
 * @param b 64-bit integer.
 * @return the index of the last bit set.
 */
int last_bit(uint64_t b)
{
#if __STC_VERSION__ >= 202311L  // C23 version

	return std_first_trailing_one(b);

#elif defined(_MSC_VER)

	unsigned long index;
	_BitScanReverse64(&index, b);
	return (int) index;

#elif defined(__GNUC__)

	return 63 - __builtin_clzl(b);

#else
	static const unsigned char magic[64] = {
		63, 0, 58, 1, 59, 47, 53, 2,
		60, 39, 48, 27, 54, 33, 42, 3,
		61, 51, 37, 40, 49, 18, 28, 20,
		55, 30, 34, 11, 43, 14, 22, 4,
		62, 57, 46, 52, 38, 26, 32, 41,
		50, 36, 17, 19, 29, 10, 13, 21,
		56, 45, 25, 31, 35, 16, 9, 12,
		44, 24, 15, 8, 23, 7, 6, 5
	};

	b |= b >> 1;
	b |= b >> 2;
	b |= b >> 4;
	b |= b >> 8;
	b |= b >> 16;
	b |= b >> 32;
	b = (b >> 1) + 1;

	return magic[(b * 0x07EDD5E59A4E28C2ULL) >> 58];

#endif
}



/**
 * @brief Transpose the uint64_t (symetry % A1-H8 diagonal).
 * @param b An uint64_t
 * @return The transposed uint64_t.
 */

uint64_t transpose(uint64_t b)
{
#if USE_SIMD && !defined(_MSC_VER) && defined(__AVX2__) // ms c compiler is buggy here

	__m256i	v = _mm256_sllv_epi64(_mm256_broadcastq_epi64(_mm_cvtsi64_si128(b)), _mm256_set_epi64x(0, 1, 2, 3));
	return ((uint64_t) _mm256_movemask_epi8(v) << 32) | (uint32_t) _mm256_movemask_epi8(_mm256_slli_epi64(v, 4));

#else

	uint64_t t;

	t = (b ^ (b >> 7)) & 0x00aa00aa00aa00aaULL;
	b = b ^ t ^ (t << 7);
	t = (b ^ (b >> 14)) & 0x0000cccc0000ccccULL;
	b = b ^ t ^ (t << 14);
	t = (b ^ (b >> 28)) & 0x00000000f0f0f0f0ULL;
	b = b ^ t ^ (t << 28);

	return b;

#endif
}

/**
 * @brief Swap bytes of a short (little <-> big endian).
 * @param s An uint16_t.
 * @return The mirrored short.
 */
uint16_t bswap_16(uint16_t b)
{
#if defined(_MSC_VER)

	return _byteswap_ushort(b);

#elif defined(__GNUC__)

	return __builtin_bswap16(b);

#else

	return (uint16_t) ((b >> 8) & 0x00FF) | ((b <<  8) & 0xFF00);

#endif
}

/**
 * @brief Mirror an uint32_t (little <-> big endian).
 * @param i An uint32_t.
 * @return The mirrored int.
 */
uint32_t bswap_32(uint32_t b)
{
#if defined(_MSC_VER)

	return _byteswap_ulong(b);

#elif defined(__GNUC__)

	return __builtin_bswap32(b);

#else

	b = ((b >>  8) & 0x00FF00FFU) | ((b <<  8) & 0xFF00FF00U);
	b = ((b >> 16) & 0x0000FFFFU) | ((b << 16) & 0xFFFF0000U);
	return b;

#endif
}

uint64_t bswap_64(uint64_t b)
{
#if defined(_MSC_VER)

	return _byteswap_uint64(b);

#elif defined(__GNUC__)

	return __builtin_bswap64(b);

#else

	b = ((b >>  8) & 0x00FF00FF00FF00FFULL) | ((b <<  8) & 0xFF00FF00FF00FF00ULL);
	b = ((b >> 16) & 0x0000FFFF0000FFFFULL) | ((b << 16) & 0xFFFF0000FFFF0000ULL);
	b = ((b >> 32) & 0x00000000FFFFFFFFULL) | ((b << 32) & 0xFFFFFFFF00000000ULL);
	return b;

#endif
}

/**
 * @brief Mirror the uint64_t (exchange the line 1 - 8, 2 - 7, 3 - 6 & 4 - 5).
 * @param b An uint64_t.
 * @return The mirrored uint64_t.
 */
uint64_t horizontal_mirror(uint64_t b)
{
  b = ((b >> 1) & 0x5555555555555555ULL) | ((b << 1) & 0xAAAAAAAAAAAAAAAAULL);
  b = ((b >> 2) & 0x3333333333333333ULL) | ((b << 2) & 0xCCCCCCCCCCCCCCCCULL);
  b = ((b >> 4) & 0x0F0F0F0F0F0F0F0FULL) | ((b << 4) & 0xF0F0F0F0F0F0F0F0ULL);

  return b;
}

/**
 * @brief Rotate left a 8-bit number
 *
 * @param b the number to rotate.
 * @param n the number of bits to rotate.
 * @return the rotated number.
 */
 uint8_t bit_rotate_left_8(const uint8_t b, const int n)
 {
	 assert(0 < n && n < 8);
	#if defined(_MSC_VER)
		return _rotl8(b, n);
	#else
		return (b << n) | (b >> (8 - n));
	#endif
}

/**
 * @brief Rotate left a 16-bit number
 *
 * @param b the number to rotate.
 * @param n the number of bits to rotate.
 * @return the rotated number.
 */
 uint16_t bit_rotate_left_16(const uint16_t b, const int n)
 {
	 assert(0 < n && n < 16);
	#if defined(_MSC_VER)
		return _rotl16(b, n);
	#else
		return (b << n) | (b >> (16 - n));
	#endif
}

/**
 * @brief Rotate left a 32-bit number
 *
 * @param b the number to rotate.
 * @param n the number of bits to rotate.
 * @return the rotated number.
 */
 uint32_t bit_rotate_left_32(const uint32_t b, const int n)
 {
	assert(0 < n && n < 32);
	#if defined(_MSC_VER)
		return _rotl(b, n);
	#else
		return (b << n) | (b >> (32 - n));
	#endif
}

/**
 * @brief Rotate left a 64-bit number
 *
 * @param b the number to rotate.
 * @param n the number of bits to rotate.
 * @return the rotated number.
 */
uint64_t bit_rotate_left_64(const uint64_t b, const int n)
{
	assert(0 < n && n < 64);
	#if defined(_MSC_VER)
		return _rotl64(b, n);
	#else
		return (b << n) | (b >> (64 - n));
	#endif
}



/**
 * @brief Get a random set bit index.
 *
 * @param b The uint64_t.
 * @param r The pseudo-number generator.
 * @return a random bit index, or -1 if b value is zero.
 */
int get_rand_bit(uint64_t b, Random *r)
{
	int n = bit_count(b), x;

	if (n == 0) return -1;

	n = random_get(r) % n;
	foreach_bit(x, b) if (n-- == 0) return x;

	return -2; // impossible.
}

/**
 * @brief Print an uint64_t as a board.
 *
 * Write a 64-bit long number as an Othello board.
 *
 * @param b The uint64_t.
 * @param f Output stream.
 */
void bitboard_print(const uint64_t b, FILE *f)
{
	int i, j, x;
	static const char color[] = ".X";

	fputs("  A B C D E F G H\n", f);
	for (i = 0; i < 8; ++i) {
		fputc(i + '1', f);
		fputc(' ', f);
		for (j = 0; j < 8; ++j) {
			x = i * 8 + j;
			fputc(color[((b >> (unsigned)x) & 1)], f);
			fputc(' ', f);
		}
		fputc(i + '1', f);
		fputc('\n', f);
	}
	fputs("  A B C D E F G H\n", f);
}

/**
 * @brief test the functions above
 *
 */
void bit_test(void) {
	expect_eq(bit_count(0x123), 4, "bit_count(0x123) == 4");
	expect_eq(bit_count(0x123456), 9, "bit_count(0x123456) == 9");
	expect_eq(first_bit(0x123), 0, "first_bit(0x123) == 0");
	expect_eq(first_bit(0x123456), 1, "first_bit(0x123456) == 1");
	expect_eq(last_bit(0x123), 8, "last_bit(0x123) == 8");
	expect_eq(last_bit(0x123456), 20, "last_bit(0x123456) == 20");
	expect_eq(bswap_16(0x1234), 0x3412, "bswap_16(0x1234) == 0x3412");
	expect_eq(bswap_32(0x123456), 0x56341200, "bswap_32(0x123456) == 0x56341200");
	expect_eq(bswap_64(0x0000001234560000), 0x563412000000, "bswap_64(0x0000001234560000) == 0x563412000000");
	expect_eq(transpose(0x12345678), 0x3050f01060a00, "transpose(0x12345678) == 0x3050f01060a00")
	expect_eq(horizontal_mirror(0x12345678), 0x0482c6a1e, "horizontal_mirror(0x12345678) == 0x482c6a1e")

	printf("bit_test done\n");
}

