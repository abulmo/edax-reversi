/**
 * @file bit.c
 *
 * Bitwise operations.
 * Several algorithms manipulating bits are presented here. Quite often,
 * a macro needs to be defined to chose between different flavors of the
 * algorithm.
 *
 * @date 1998 - 2023
 * @author Richard Delorme
 * @version 4.5
 */

#include "bit.h"
#include "util.h"

/** Table for a 32-bits-at-a-time software CRC-32C calculation.
 * This tablehas built into it the pre and post bit inversion of the CRC. */
#ifndef crc32c_u64
static unsigned int crc32c_table[4][256];
#endif

/** coordinate to bit table converter */
unsigned long long X_TO_BIT[66];

/** Conversion array: flippable neighbour bits */
// https://eukaryote.hateblo.jp/entry/2020/04/26/031246
const unsigned long long NEIGHBOUR[] = {
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
 * @brief Count the number of bits set to one in an unsigned long long.
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

#ifndef POPCOUNT
  #if 0
int bit_count(unsigned long long b)
{
	int	c;

	b  = b - ((b >> 1) & 0x5555555555555555ULL);
	b  = ((b >> 2) & 0x3333333333333333ULL) + (b & 0x3333333333333333ULL);
    #ifdef HAS_CPU_64
	b = (b + (b >> 4)) & 0x0F0F0F0F0F0F0F0FULL;
	c = (b * 0x0101010101010101ULL) >> 56;
    #else
	c = (b >> 32) + b;
	c = (c & 0x0F0F0F0F) + ((c >> 4) & 0x0F0F0F0F);
	c = (c * 0x01010101) >> 24;
    #endif
	return c;
}

  #else
// https://github.com/official-stockfish/Stockfish/pull/620/files
// 2% faster than SWAR bit_count for 32 & 64 non-POPCOUNT build
unsigned char PopCnt16[1 << 16];

static int bit_count_32_SWAR(unsigned int b)
{
	b = b - ((b >> 1) & 0x55555555);
	b = ((b >> 2) & 0x333333333) + (b & 0x33333333);
	b = ((b >> 4) + b) & 0x0F0F0F0F;
	return (b * 0x01010101) >> 24;
}
  #endif
#endif

/**
 * @brief initialize PopCnt16 table and check MMX/SSE availability.
 */
void bit_init(void)
{
	unsigned int	n;
	unsigned long long	ll;
#ifndef crc32c_u64
	unsigned int	k, crc;

	// http://stackoverflow.com/a/17646775/1821055
	// https://github.com/baruch/crcbench
	// Generate byte-wise table.
	for (n = 0; n < 256; n++) {
		crc = ~n;
		for (k = 0; k < 8; k++)
			crc = (crc >> 1) ^ (-(int)(crc & 1) & 0x82f63b78);
		crc32c_table[0][n] = ~crc;
	}
	// Use byte-wise table to generate word-wise table.
	for (n = 0; n < 256; n++) {
		crc = ~crc32c_table[0][n];
		for (k = 1; k < 4; k++) {
			crc = crc32c_table[0][crc & 0xff] ^ (crc >> 8);
			crc32c_table[k][n] = ~crc;
		}
	}
#endif

	ll = 1;
	for (n = 0; n < 66; ++n) {	// X_TO_BIT[64] = X_TO_BIT[65] = 0 for passing move & nomove
		X_TO_BIT[n] = ll;
		ll <<= 1;
	}

#ifndef POPCOUNT
	for (n = 0; n < (1 << 16); ++n)
		PopCnt16[n] = bit_count_32_SWAR(n);
#endif

#if (defined(USE_GAS_MMX) || defined(USE_MSVC_X86)) && !defined(hasSSE2)
	init_mmx();
#endif
#if defined(ANDROID) && !defined(__ARM_NEON) && !defined(hasSSE2)
	init_neon();
#endif
}

/**
 * @brief count the number of discs, counting the corners twice.
 *
 * This is a variation of the above algorithm to count the mobility and favour
 * the corners. This function is useful for move sorting.
 * (SSE/Neon version caliculates 2 elements in parallel.)
 *
 * @param v 64-bit integer to count bits of.
 * @return the number of bit set, counting the corners twice.
 */
#if !defined(__AVX2__) && defined(hasSSE2) && !defined(POPCOUNT)
__m128i bit_weighted_count_sse(unsigned long long Q0, unsigned long long Q1)
{
	static const V2DI mask15 = {{ 0x1555555555555515, 0x1555555555555515 }};
	static const V2DI mask01 = {{ 0x0100000000000001, 0x0100000000000001 }};
	static const V2DI mask33 = {{ 0x3333333333333333, 0x3333333333333333 }};
	static const V2DI mask0F = {{ 0x0F0F0F0F0F0F0F0F, 0x0F0F0F0F0F0F0F0F }};

	__m128i v = _mm_set_epi64x(Q1, Q0);
	v = _mm_add_epi64(_mm_sub_epi64(v, _mm_and_si128(_mm_srli_epi64(v, 1), mask15.v2)), _mm_and_si128(v, mask01.v2));
	v = _mm_add_epi64(_mm_and_si128(v, mask33.v2), _mm_and_si128(_mm_srli_epi64(v, 2), mask33.v2));
	v = _mm_and_si128(_mm_add_epi64(v, _mm_srli_epi64(v, 4)), mask0F.v2);
	return _mm_sad_epu8(v, _mm_setzero_si128());
}

#elif defined(__ARM_NEON)
uint64x2_t bit_weighted_count_neon(unsigned long long Q0, unsigned long long Q1)
{
	uint64x2_t v = vcombine_u64(vcreate_u64(Q0), vcreate_u64(Q1));
	return vpaddlq_u32(vpaddlq_u16(vpaddlq_u8(vaddq_u8(vcntq_u8(vreinterpretq_u8_u64(v)),
		vcntq_u8(vreinterpretq_u8_u64(vandq_u64(v, vdupq_n_u64(0x8100000000000081))))))));
}

#elif 0	// SWAR, for record
int bit_weighted_count(unsigned long long v)
{
	int	c;

	v  = v - ((v >> 1) & 0x1555555555555515) + (v & 0x0100000000000001);
	v  = ((v >> 2) & 0x3333333333333333) + (v & 0x3333333333333333);
	c = (v >> 32) + v;
	c = (c & 0x0F0F0F0F) + ((c >> 4) & 0x0F0F0F0F);
	c = (c * 0x01010101) >> 24;
	return c;
}

#else
int bit_weighted_count(unsigned long long v)
{
  	unsigned int AH18 = ((v >> 56) | (v << 8)) & 0x8181;	// ror 56
  #ifdef POPCOUNT
	return bit_count(v) + bit_count_32(AH18);
  #else
  	return bit_count(v) + PopCnt16[AH18];
  #endif
}
#endif

/**
 *
 * @brief Search the first bit set.
 *
 * On CPU with AMD64 or EMT64 instructions, a fast asm
 * code is provided. Alternatively, a fast algorithm based on
 * magic numbers is provided.
 *
 * @param b 64-bit integer.
 * @return the index of the first bit set. (undefined if b = 0)
 */
#if !defined(first_bit_32) && !defined(HAS_CPU_64)
int first_bit_32(unsigned int b)
{
  #if defined(_MSC_VER)
	unsigned long index;
	_BitScanForward(&index, b);
	return (int) index;

  #elif defined(USE_GAS_X64) || defined(USE_GAS_X86)
	__asm__("rep; bsf	%1, %0" : "=r" (b) : "rm" (b));	// tzcnt on BMI CPUs, bsf otherwise
	return (int) b;

  #elif defined(USE_MSVC_X86)
	__asm {
		bsf	eax, word ptr b
	}

  #elif defined(USE_GCC_ARM)
	return  __builtin_clz(b & -b) ^ 31;

  #else
	static const unsigned char magic[32] = {
		0, 1, 28, 2, 29, 14, 24, 3, 30, 22, 20, 15, 25, 17, 4, 8, 
		31, 27, 13, 23, 21, 19, 16, 7, 26, 12, 18, 6, 11, 5, 10, 9
	};

	return magic[((b & (-b)) * 0x077CB531U) >> 27];
  #endif
}
#endif // first_bit_32

#ifndef first_bit
int first_bit(unsigned long long b)
{
  #if defined(USE_GAS_X64)
	__asm__("rep; bsfq	%1, %0" : "=r" (b) : "rm" (b));	// tzcntq on BMI CPUs
	return (int) b;

  #elif defined(USE_GAS_X86)
	int 	x;
	__asm__ ("bsf	%2, %0\n\t"	// (ZF differs from tzcnt)
		"jnz	1f\n\t"
		"bsf	%1, %0\n\t"
		"addl	$32, %0\n"
	"1:" : "=&q" (x) : "g" ((int) (b >> 32)), "g" ((int) b));
	return x;

  #elif defined(_MSC_VER) && (defined(_M_X64) || defined(_M_ARM64))
	unsigned long index;
	_BitScanForward64(&index, b);
	return (int) index;

  #elif defined(USE_MSVC_X86)
	__asm {
		bsf	eax, dword ptr b
		jnz	l1
		bsf	eax, dword ptr b+4
		add	eax, 32
	l1:
	}

  #elif defined(HAS_CPU_64)
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

  #else
	const unsigned int lb = (unsigned int) b;
	if (lb) {
		return first_bit_32(lb);
	} else {
		return 32 + first_bit_32(b >> 32);
	}
  #endif
}
#endif // first_bit

#if 0
/**
 * @brief Search the next bit set.
 *
 * In practice, clear the first bit set and search the next one.
 *
 * @param b 64-bit integer.
 * @return the index of the next bit set.
 */
int next_bit(unsigned long long *b)
{
	*b &= *b - 1;
	return first_bit(*b);
}
#endif

#ifndef last_bit
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
int last_bit(unsigned long long b)
{
  #if defined(USE_GAS_X64)
	__asm__("bsrq	%1, %0" :"=r" (b) :"rm" (b));
	return b;

  #elif defined(_MSC_VER) && (defined(_M_X64) || defined(_M_ARM64))
	unsigned long index;
	_BitScanReverse64(&index, b);
	return (int) index;

  #elif defined(USE_GAS_X86)
	int	x;
	__asm__ ("bsr	%1, %0\n\t"
		"leal	32(%0), %0\n\t"
		"jnz	1f\n\t"
		"bsr	%2, %0\n\t"
        "1:" : "=&q" (x) : "g" ((int) (b >> 32)), "g" ((int) b));
	return x;

  #elif 0 // defined(USE_GCC_ARM)
	const unsigned int hb = b >> 32;
	if (hb) {
		return 63 - __builtin_clz(hb);
	} else {
		return 31 - __builtin_clz((int) b);
	}

  #elif defined(USE_MSVC_X86)
	__asm {
		bsr	eax, dword ptr b+4
		lea	eax, [eax+32]
		jnz	l1
		bsr	eax, dword ptr b
	l1:
	}

  #elif defined(HAS_CPU_64)
	// https://www.chessprogramming.org/BitScan#De_Bruijn_Multiplication_2
	static const unsigned char magic[64] = {
		 0, 47,  1, 56, 48, 27,  2, 60,
		57, 49, 41, 37, 28, 16,  3, 61,
		54, 58, 35, 52, 50, 42, 21, 44,
		38, 32, 29, 23, 17, 11,  4, 62,
		46, 55, 26, 59, 40, 36, 15, 53,
		34, 51, 20, 43, 31, 22, 10, 45,
		25, 39, 14, 33, 19, 30,  9, 24,
		13, 18,  8, 12,  7,  6,  5, 63
	};

	b |= b >> 1;
	b |= b >> 2;
	b |= b >> 4;
	b |= b >> 8;
	b |= b >> 16;
	b |= b >> 32;

	return magic[(b * 0x03f79d71b4cb0a89) >> 58];

  #else
	static const unsigned char clz_table_4bit[16] = { 4, 3, 2, 2, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0 };
	int	n = 63;
	unsigned int	x;

	x = b >> 32;
	if (x == 0) { n = 31; x = (unsigned int) b; }
	if ((x & 0xFFFF0000) == 0) { n -= 16; x <<= 16; }
	if ((x & 0xFF000000) == 0) { n -=  8; x <<=  8; }
	if ((x & 0xF0000000) == 0) { n -=  4; x <<=  4; }
	n -= clz_table_4bit[x >> (32 - 4)];
	return n;
  #endif
}
#endif // last_bit

#ifndef bswap_int
/**
 * @brief Mirror the unsigned int (little <-> big endian).
 * @param i An unsigned int.
 * @return The mirrored int.
 */
unsigned int bswap_int(unsigned int i)
{
	i = ((i >>  8) & 0x00FF00FFU) | ((i & 0x00FF00FFU) <<  8);
	i = (i >> 16) | (i << 16);
	return i;
}

/**
 * @brief Mirror the unsigned long long (exchange the lines A - H, B - G, C - F & D - E.).
 * @param b An unsigned long long
 * @return The mirrored unsigned long long.
 */
unsigned long long vertical_mirror(unsigned long long b)
{
	return bswap_int((unsigned int)(b >> 32)) | ((unsigned long long) bswap_int((unsigned int) b) << 32);
}
#endif // bswap_int

/**
 * @brief Mirror the unsigned long long (exchange the line 1 - 8, 2 - 7, 3 - 6 & 4 - 5).
 * @param b An unsigned long long.
 * @return The mirrored unsigned long long.
 */
unsigned int horizontal_mirror_32(unsigned int b)
{
#ifdef __ARM_ACLE
	return __rev(__rbit(b));
#else
	b = ((b >> 1) & 0x55555555U) +  2 * (b & 0x55555555U);
	b = ((b >> 2) & 0x33333333U) +  4 * (b & 0x33333333U);
	b = ((b >> 4) & 0x0F0F0F0FU) + 16 * (b & 0x0F0F0F0FU);
	return b;
#endif
}

unsigned long long horizontal_mirror(unsigned long long b)
{
#if defined(HAS_CPU_64) && !defined(__ARM_ACLE)
	b = ((b >> 1) & 0x5555555555555555ULL) | ((b & 0x5555555555555555ULL) << 1);
	b = ((b >> 2) & 0x3333333333333333ULL) | ((b & 0x3333333333333333ULL) << 2);
	b = ((b >> 4) & 0x0F0F0F0F0F0F0F0FULL) | ((b & 0x0F0F0F0F0F0F0F0FULL) << 4);
	return b;
#else
	return ((unsigned long long) horizontal_mirror_32(b >> 32) << 32)
		| horizontal_mirror_32((unsigned int) b);
#endif
}

/**
 * @brief Transpose the unsigned long long (symetry % A1-H8 diagonal, or swap axes).
 * @param b An unsigned long long
 * @return The transposed unsigned long long.
 */
#ifdef __AVX2__
unsigned long long transpose(unsigned long long b)
{
	__m256i	v = _mm256_sllv_epi64(_mm256_broadcastq_epi64(_mm_cvtsi64_si128(b)), _mm256_set_epi64x(0, 1, 2, 3));
	return ((unsigned long long) _mm256_movemask_epi8(v) << 32)
		| (unsigned int) _mm256_movemask_epi8(_mm256_slli_epi64(v, 4));
}

#else
unsigned long long transpose(unsigned long long b)
{
	unsigned long long t;

	t = (b ^ (b >> 7)) & 0x00aa00aa00aa00aaULL;
	b = b ^ t ^ (t << 7);
	t = (b ^ (b >> 14)) & 0x0000cccc0000ccccULL;
	b = b ^ t ^ (t << 14);
	t = (b ^ (b >> 28)) & 0x00000000f0f0f0f0ULL;
	b = b ^ t ^ (t << 28);

	return b;
}
#endif // __AVX2__

#ifndef crc32c_u64
/**
 * @brief Caliculate crc32c checksum for 8 bytes data
 * @param crc Initial crc from previous data.
 * @param data Data to accumulate.
 * @return Resulting crc.
 */
unsigned int crc32c_u64(unsigned int crc, unsigned long long data)
{
	crc ^= (unsigned int) data;
	crc =	crc32c_table[3][crc & 0xff] ^
		crc32c_table[2][(crc >> 8) & 0xff] ^
		crc32c_table[1][(crc >> 16) & 0xff] ^
		crc32c_table[0][crc >> 24];
	crc ^= (unsigned int) (data >> 32);
	return	crc32c_table[3][crc & 0xff] ^
		crc32c_table[2][(crc >> 8) & 0xff] ^
		crc32c_table[1][(crc >> 16) & 0xff] ^
		crc32c_table[0][crc >> 24];
}

/**
 * @brief Caliculate crc32c checksum for a byte
 * @param crc Initial crc from previous data.
 * @param data Data to accumulate.
 * @return Resulting crc.
 */
unsigned int crc32c_u8(unsigned int crc, unsigned int data)
{
	return	crc32c_table[0][(crc ^ data) & 0xff] ^ (crc >> 8);
}
#endif

/**
 * @brief Get a random set bit index.
 *
 * @param b The unsigned long long.
 * @param r The pseudo-number generator.
 * @return a random bit index, or -1 if b value is zero.
 */
int get_rand_bit(unsigned long long b, Random *r)
{
	int n = bit_count(b), x;

	if (n == 0) return -1;

	n = random_get(r) % n;
	foreach_bit(x, b) if (n-- == 0) return x;

	return -2; // impossible.
}

/**
 * @brief Print an unsigned long long as a board.
 *
 * Write a 64-bit long number as an Othello board.
 *
 * @param b The unsigned long long.
 * @param f Output stream.
 */
void bitboard_write(unsigned long long b, FILE *f)
{
	int i, j;
	static const char color[2] = ".X";

	fputs("  A B C D E F G H\n", f);
	for (i = 0; i < 8; ++i) {
		fputc(i + '1', f);
		fputc(' ', f);
		for (j = 0; j < 8; ++j) {
			fputc(color[b & 1], f);
			fputc(' ', f);
			b >>= 1;
		}
		fputc(i + '1', f);
		fputc('\n', f);
	}
	fputs("  A B C D E F G H\n", f);
}
