/**
 * @file bit.c
 *
 * Bitwise operations.
 * Several algorithms manipulating bits are presented here. Quite often,
 * a macro needs to be defined to chose between different flavors of the
 * algorithm.
 *
 * @date 1998 - 2017
 * @author Richard Delorme
 * @version 4.4
 */

#include "bit.h"
#include "util.h"

/** coordinate to bit table converter */
const unsigned long long X_TO_BIT[] = {
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

/** Conversion array: neighbour bits */
const unsigned long long NEIGHBOUR[] = {
	0x0000000000000302ULL, 0x0000000000000705ULL, 0x0000000000000e0aULL, 0x0000000000001c14ULL,
	0x0000000000003828ULL, 0x0000000000007050ULL, 0x000000000000e0a0ULL, 0x000000000000c040ULL,
	0x0000000000030203ULL, 0x0000000000070507ULL, 0x00000000000e0a0eULL, 0x00000000001c141cULL,
	0x0000000000382838ULL, 0x0000000000705070ULL, 0x0000000000e0a0e0ULL, 0x0000000000c040c0ULL,
	0x0000000003020300ULL, 0x0000000007050700ULL, 0x000000000e0a0e00ULL, 0x000000001c141c00ULL,
	0x0000000038283800ULL, 0x0000000070507000ULL, 0x00000000e0a0e000ULL, 0x00000000c040c000ULL,
	0x0000000302030000ULL, 0x0000000705070000ULL, 0x0000000e0a0e0000ULL, 0x0000001c141c0000ULL,
	0x0000003828380000ULL, 0x0000007050700000ULL, 0x000000e0a0e00000ULL, 0x000000c040c00000ULL,
	0x0000030203000000ULL, 0x0000070507000000ULL, 0x00000e0a0e000000ULL, 0x00001c141c000000ULL,
	0x0000382838000000ULL, 0x0000705070000000ULL, 0x0000e0a0e0000000ULL, 0x0000c040c0000000ULL,
	0x0003020300000000ULL, 0x0007050700000000ULL, 0x000e0a0e00000000ULL, 0x001c141c00000000ULL,
	0x0038283800000000ULL, 0x0070507000000000ULL, 0x00e0a0e000000000ULL, 0x00c040c000000000ULL,
	0x0302030000000000ULL, 0x0705070000000000ULL, 0x0e0a0e0000000000ULL, 0x1c141c0000000000ULL,
	0x3828380000000000ULL, 0x7050700000000000ULL, 0xe0a0e00000000000ULL, 0xc040c00000000000ULL,
	0x0203000000000000ULL, 0x0507000000000000ULL, 0x0a0e000000000000ULL, 0x141c000000000000ULL,
	0x2838000000000000ULL, 0x5070000000000000ULL, 0xa0e0000000000000ULL, 0x40c0000000000000ULL,
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
DLL_API int bit_count(unsigned long long b)
{
#if defined(POPCOUNT)

	#if defined(USE_GAS_X64)
		__asm__("popcntq %1,%0" :"=r" (b) :"rm" (b));
		return (int) b;
	#elif defined(USE_MSVC_X64)
		return __popcnt64(b);
	#elif defined(USE_GCC_X64)
		return __builtin_popcountll(b);
	#endif

// MMX does not help much here :-(
#elif defined (USE_GAS_MMX)
 
	const unsigned long long M55 = 0x5555555555555555ULL;
	const unsigned long long M33 = 0x3333333333333333ULL;
	const unsigned long long M0F = 0x0F0F0F0F0F0F0F0FULL;
	int count;

	__asm__ volatile(
 		"movq  %1, %%mm1\n\t"
		"pxor  %%mm2, %%mm2\n\t"

		"movq  %%mm1, %%mm0\n\t"
		"psrlq $1, %%mm1\n\t"
		"pand  %2, %%mm1\n\t"
		"psubd %%mm1, %%mm0\n\t"

		"movq  %%mm0, %%mm1\n\t"
		"psrlq $2, %%mm0\n\t"
		"pand  %3, %%mm1\n\t"
		"pand  %3, %%mm0\n\t"
		"paddd %%mm1, %%mm0\n\t"

		"movq  %%mm0, %%mm1\n\t"
		"psrlq $4, %%mm0\n\t"
		"paddd %%mm1, %%mm0\n\t"
		"pand  %4, %%mm0\n\t"

		"psadbw %%mm2, %%mm0\n\t"
		"movd	%%mm0, %0\n\t"
		"emms\n\t"
		: "=a" (count) : "m" (b), "m" (M55), "m" (M33), "m" (M0F));

	return count;

#else

	register unsigned long long c = b
		- ((b >> 1) & 0x7777777777777777ULL)
		- ((b >> 2) & 0x3333333333333333ULL)
		- ((b >> 3) & 0x1111111111111111ULL);
	c = ((c + (c >> 4)) & 0x0F0F0F0F0F0F0F0FULL) * 0x0101010101010101ULL;

	return  (int)(c >> 56);

#endif
}


/**
 * @brief count the number of discs, counting the corners twice.
 *
 * This is a variation of the above algorithm to count the mobility and favour
 * the corners. This function is useful for move sorting.
 *
 * @param v 64-bit integer to count bits of.
 * @return the number of bit set, counting the corners twice.
 */
int bit_weighted_count(const unsigned long long v)
{
#if defined(POPCOUNT)

	return bit_count(v) + bit_count(v & 0x8100000000000081ULL);

#else

	register unsigned long long b;
	b  = v - ((v >> 1) & 0x1555555555555515ULL) + (v & 0x0100000000000001ULL);
	b  = ((b >> 2) & 0x3333333333333333ULL) + (b & 0x3333333333333333ULL);
	b  = ((b >> 4) + b) & 0x0f0f0f0f0f0f0f0fULL;
	b *= 0x0101010101010101ULL;

	return  (int)(b >> 56);

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
DLL_API int first_bit(unsigned long long b)
{
#if defined(USE_GAS_X64)

	__asm__("bsfq %1,%0" : "=r" (b) : "rm" (b));
	return (int) b;

#elif defined(USE_GAS_X86)

  int x1, x2;
	__asm__ ("bsf %0,%0\n"
	         "jnz 1f\n"
	         "bsf %1,%0\n"
	         "jz 1f\n"
	         "addl $32,%0\n"
		     "1:": "=&q" (x1), "=&q" (x2):"1" ((int) (b >> 32)), "0" ((int) b));
	return x1;

#elif defined(USE_MSVC_X64)

	unsigned long index;
	_BitScanForward64(&index, b);
	return (int) index;

#elif defined(USE_GCC_X64)

	return __builtin_ctzll(b);

#elif defined(USE_MASM_X86)
	__asm {
		xor eax, eax
		bsf edx, dword ptr b
		jnz l1
		bsf edx, dword ptr b+4
		mov eax, 32
		jnz l1
		mov edx, -32
	l1:	add eax, edx
	}

#elif defined(USE_GCC_ARM)
	const unsigned int lb = (unsigned int)b;
	if (lb) {
		return  __builtin_clz(lb & -lb) ^ 31;
	} else {
		const unsigned int hb = b >> 32;
		return 32 + (__builtin_clz(hb & -hb) ^ 31);
	}

#else

	const int magic[64] = {
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
int next_bit(unsigned long long *b)
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
DLL_API int last_bit(unsigned long long b)
{
#if defined(USE_GAS_X64)

	__asm__("bsrq %1,%0" :"=r" (b) :"rm" (b));
	return b;

#elif defined(USE_MSVC_X64)

	unsigned long index;
	_BitScanReverse64(&index, b);
	return (int) index;

#elif defined(USE_GCC_X64)

	return 63 - __builtin_clzll(b);
	
#elif defined(USE_GAS_X86)

  int x1, x2;
	__asm__ ("bsr %1,%0\n"
	         "jnz 1f\n"
	         "bsr %0,%0\n"
	         "subl $32,%0\n"
             "1: addl $32,%0\n" : "=&q" (x1), "=&q" (x2) : "1" ((int) (b >> 32)), "0" ((int) b));
  return x1;

	
#elif defined(USE_GCC_ARM)
	const unsigned int hb = b >> 32;
	if (hb) {
		return 63 - __builtin_clz(hb);
	} else {
		return 31 - __builtin_clz((int) b);
	}


#elif defined(USE_MASM_X86)
	__asm {
		xor eax, eax
		bsr edx, dword ptr b+4
		jnz l1
		bsr edx, dword ptr b
		mov eax, 32
		jnz l1
		mov edx, -32
	l1:	add eax, edx
	}

	
#else

	const int magic[64] = {
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
 * @brief Transpose the unsigned long long (symetry % A1-H8 diagonal).
 * @param b An unsigned long long
 * @return The transposed unsigned long long.
 */
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

/**
 * @brief Mirror the unsigned long long (exchange the lines A - H, B - G, C - F & D - E.).
 * @param b An unsigned long long
 * @return The mirrored unsigned long long.
 */
unsigned long long vertical_mirror(unsigned long long b)
{
	b = ((b >>  8) & 0x00FF00FF00FF00FFULL) | ((b <<  8) & 0xFF00FF00FF00FF00ULL);
	b = ((b >> 16) & 0x0000FFFF0000FFFFULL) | ((b << 16) & 0xFFFF0000FFFF0000ULL);
	b = ((b >> 32) & 0x00000000FFFFFFFFULL) | ((b << 32) & 0xFFFFFFFF00000000ULL);
	return b;
}

/**
 * @brief Mirror the unsigned long long (exchange the line 1 - 8, 2 - 7, 3 - 6 & 4 - 5).
 * @param b An unsigned long long.
 * @return The mirrored unsigned long long.
 */
unsigned long long horizontal_mirror(unsigned long long b)
{
  b = ((b >> 1) & 0x5555555555555555ULL) | ((b << 1) & 0xAAAAAAAAAAAAAAAAULL);
  b = ((b >> 2) & 0x3333333333333333ULL) | ((b << 2) & 0xCCCCCCCCCCCCCCCCULL);
  b = ((b >> 4) & 0x0F0F0F0F0F0F0F0FULL) | ((b << 4) & 0xF0F0F0F0F0F0F0F0ULL);

  return b;
}

/**
 * @brief Swap bytes of a short (little <-> big endian).
 * @param s An unsigned short.
 * @return The mirrored short.
 */
unsigned short bswap_short(unsigned short s)
{
	return (unsigned short) ((s >> 8) & 0x00FF) | ((s <<  8) & 0xFF00);
}

/**
 * @brief Mirror the unsigned int (little <-> big endian).
 * @param i An unsigned int.
 * @return The mirrored int.
 */
unsigned int bswap_int(unsigned int i)
{
	i = ((i >>  8) & 0x00FF00FFU) | ((i <<  8) & 0xFF00FF00U);
	i = ((i >> 16) & 0x0000FFFFU) | ((i << 16) & 0xFFFF0000U);
	return i;
}

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
void bitboard_write(const unsigned long long b, FILE *f)
{
	int i, j, x;
	const char *color = ".X";

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





