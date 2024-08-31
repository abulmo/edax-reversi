/**
 * @file bit.h
 *
 * Bitwise operations header file.
 *
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
 * @date 1998 - 2023
=======
 * @date 1998 - 2017
>>>>>>> b3f048d (copyright changes)
=======
 * @date 1998 - 2018
>>>>>>> 1c68bd5 (SSE / AVX optimized eval feature added)
=======
 * @date 1998 - 2020
>>>>>>> 9ad160e (4.4.7 AVX/shuffle optimization in endgame_sse.c)
=======
 * @date 1998 - 2021
>>>>>>> 34a2291 (4.5.0: Use CRC32c for board hash)
=======
 * @date 1998 - 2022
>>>>>>> 9e2bbc5 (split get_all_full_lines from get_stability)
=======
 * @date 1998 - 2020
>>>>>>> 6c3ed52 (Dogaishi hash reduction by Matsuo & Narazaki; edge-precise get_full_line)
=======
 * @date 1998 - 2022
>>>>>>> fdb3c8a (SWAR vector eval update; more restore in search_restore_midgame)
 * @author Richard Delorme
 * @version 4.5
 */

#ifndef EDAX_BIT_H
#define EDAX_BIT_H

#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include <stdbool.h>

#include "bit_intrinsics.h"

struct Random;

/* declaration */
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
void bit_init(void);
// int next_bit(unsigned long long*);
void bitboard_write(unsigned long long, FILE*);
=======
int bit_weighted_count(const unsigned long long);
// int next_bit(unsigned long long*);
void bitboard_write(const unsigned long long, FILE*);
>>>>>>> 1c68bd5 (SSE / AVX optimized eval feature added)
=======
=======
void bit_init(void);
>>>>>>> 22be102 (table lookup bit_count for non-POPCOUNT from stockfish)
int bit_weighted_count(unsigned long long);
// int next_bit(unsigned long long*);
void bitboard_write(unsigned long long, FILE*);
>>>>>>> cd90dbb (Enable 32bit AVX build; optimize loop in board print; set version to 4.4.6)
unsigned long long transpose(unsigned long long);
<<<<<<< HEAD
<<<<<<< HEAD
unsigned int horizontal_mirror_32(unsigned int b);
=======
>>>>>>> dbeab1c (reduce asm and inline which sometimes breaks debug build)
=======
unsigned int horizontal_mirror_32(unsigned int b);
>>>>>>> 1b29848 (fix & optimize 32 bit build; other minor mods)
unsigned long long horizontal_mirror(unsigned long long);
int get_rand_bit(unsigned long long, struct Random*);

<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
#if !defined(__AVX2__) && defined(hasSSE2) && !defined(POPCOUNT)
	__m128i bit_weighted_count_sse(unsigned long long, unsigned long long);
#elif defined (__ARM_NEON)
	uint64x2_t bit_weighted_count_neon(unsigned long long, unsigned long long);
=======
#ifdef __GNUC__
#define	bswap_short(x)	__builtin_bswap16(x)
#define	bswap_int(x)	__builtin_bswap32(x)
#define	vertical_mirror(x)	__builtin_bswap64(x)
#elif defined(_MSC_VER)
#define	bswap_short(x)	_byteswap_ushort(x)
#define	bswap_int(x)	_byteswap_ulong(x)
#define	vertical_mirror(x)	_byteswap_uint64(x)
#else
unsigned short bswap_short(unsigned short);
unsigned int bswap_int(unsigned int);
unsigned long long vertical_mirror(unsigned long long);
=======
=======
=======
extern const unsigned long long X_TO_BIT[];
/** Return a bitboard with bit x set. */
#define x_to_bit(x) X_TO_BIT[x]
=======
extern unsigned long long X_TO_BIT[];
extern const unsigned long long NEIGHBOUR[];
>>>>>>> 343493d (More neon/sse optimizations; neon dispatch added for arm32)

/** Return a bitboard with bit x set. */
// https://eukaryote.hateblo.jp/entry/2020/04/12/054905
#ifdef HAS_CPU_64 // 1% slower on Sandy Bridge
#define x_to_bit(x) (1ULL << (x))
#else
#define x_to_bit(x) X_TO_BIT[x]
#endif

<<<<<<< HEAD
<<<<<<< HEAD
>>>>>>> b1eae0d (Reduce flip table by rotated outflank; revise lzcnt & rol8 defs)
=======
#ifndef __has_builtin
	#define __has_builtin(x) 0  // Compatibility with non-clang compilers.
#endif

// mirror byte
#if defined(_M_ARM) // || defined(_M_ARM64) // https://developercommunity.visualstudio.com/content/problem/498995/arm64-missing-rbit-intrinsics.html
#define mirror_byte(b)	(_arm_rbit(b) >> 24)
#elif defined(__ARM_ACLE)
#include <arm_acle.h>
#define mirror_byte(b)	(__rbit(b) >> 24)
#elif defined(HAS_CPU_64)
>>>>>>> f2da03e (Refine arm builds adding neon support.)
// http://graphics.stanford.edu/~seander/bithacks.html
#define mirror_byte(b)	(unsigned char)((((b) * 0x80200802ULL) & 0x0884422110ULL) * 0x0101010101ULL >> 32)
#else
static inline unsigned char mirror_byte(unsigned int b) { return ((((b * 0x200802) & 0x4422110) + ((b << 7) & 0x880)) * 0x01010101 >> 24); }
#endif

<<<<<<< HEAD
>>>>>>> 0ee9c1c (mirror_byte added for 1 byte bit reverse)
#ifndef __has_builtin
	#define __has_builtin(x) 0  // Compatibility with non-clang compilers.
>>>>>>> ea39994 (Improve clang compatibility)
#endif

=======
// rotl8
>>>>>>> f2da03e (Refine arm builds adding neon support.)
#if __has_builtin(__builtin_rotateleft8)
	#define rotl8(x,y)	__builtin_rotateleft8((x),(y))
#elif defined(__GNUC__) && (__GNUC__ > 4 || (__GNUC__ == 4 && __GNUC_MINOR__ >= 5)) && (defined(__x86_64__) || defined(__i386__))
	#define rotl8(x,y)	__builtin_ia32_rolqi((x),(y))
#elif defined(_MSC_VER)
	#define	rotl8(x,y)	_rotl8((x),(y))
#else	// may not compile into 8-bit rotate
	#define	rotl8(x,y)	((unsigned char)(((x)<<(y))|((unsigned)(x)>>(8-(y)))))
#endif

// bswap
#ifdef _MSC_VER
	#define	bswap_short(x)	_byteswap_ushort(x)
	#define	bswap_int(x)	_byteswap_ulong(x)
	#define	vertical_mirror(x)	_byteswap_uint64(x)
#else
	#if (defined(__GNUC__) && (__GNUC__ > 4 || (__GNUC__ == 4 && __GNUC_MINOR__ >= 8))) || __has_builtin(__builtin_bswap16)
		#define	bswap_short(x)	__builtin_bswap16(x)
	#else
		#define bswap_short(x)	(((unsigned short) (x) >> 8) | ((unsigned short) (x) << 8))
	#endif
	#if (defined(__GNUC__) && (__GNUC__ > 4 || (__GNUC__ == 4 && __GNUC_MINOR__ >= 3))) || __has_builtin(__builtin_bswap64)
		#define	bswap_int(x)	__builtin_bswap32(x)
		#define	vertical_mirror(x)	__builtin_bswap64(x)
	#else
		unsigned int bswap_int(unsigned int);
		unsigned long long vertical_mirror(unsigned long long);
	#endif
#endif

// ctz / clz
=======
/** Loop over each bit set. */
>>>>>>> 569c1f8 (More neon optimizations; split bit_intrinsics.h from bit.h)
#if (defined(__GNUC__) && __GNUC__ >= 4) || __has_builtin(__builtin_ctzll)
	#define	first_bit(x)	__builtin_ctzll(x)
	#define	last_bit(x)	(63 - __builtin_clzll(x))
#elif defined(tzcnt_u64)
	#define	first_bit(x)	tzcnt_u64(x)
	#define	last_bit(x)	(63 - lzcnt_u64(x))
#else
	int first_bit(unsigned long long);
	int last_bit(unsigned long long);
#endif

#define foreach_bit(i, b)	for (i = first_bit(b); b; i = first_bit(b &= (b - 1)))

#ifdef HAS_CPU_64
	typedef unsigned long long	widest_register;
	#define foreach_bit_r(i, b, j, r)	(void) j; r = b; foreach_bit(i, r)
#else
	typedef unsigned int	widest_register;
	#ifdef tzcnt_u32
		#define	first_bit_32(x)	tzcnt_u32(x)
	#else
		int first_bit_32(unsigned int);
	#endif
	#define foreach_bit_r(i, b, j, r)	for (j = 0; j < 64; j += sizeof(widest_register) * CHAR_BIT) \
		for (i = first_bit_32(r = (widest_register)(b >> j)) + j; r; i = first_bit_32(r &= (r - 1)) + j)
#endif

// popcount
#ifdef hasNeon
	#ifdef HAS_CPU_64
		#define bit_count(x)	vaddv_u8(vcnt_u8(vcreate_u8(x)))
	#else
		#define bit_count(x)	vget_lane_u32(vreinterpret_u32_u64(vpaddl_u32(vpaddl_u16(vpaddl_u8(vcnt_u8(vcreate_u8(x)))))), 0)
	#endif

#elif defined(POPCOUNT)
	/*
	#if defined (USE_GAS_X64)
		static inline int bit_count (unsigned long long x) {
			long long	y;
			__asm__ ( "popcntq %1,%0" : "=r" (y) : "rm" (x));
			return y;
		}
	#elif defined (USE_GAS_X86)
		static inline int bit_count (unsigned long long x) {
			unsigned int	y0, y1;
			__asm__ ( "popcntl %2,%0\n\t"
				"popcntl %3,%1"
				: "=&r" (y0), "=&r" (y1)
				: "rm" ((unsigned int) x), "rm" ((unsigned int) (x >> 32)));
			return y0 + y1;
		}
	*/
	#ifdef _MSC_VER
		#if defined(_M_ARM) || defined(_M_ARM64)
			#define bit_count(x)	_CountOneBits64(x)
		#elif defined(_M_X64)
			#define	bit_count(x)	((int) __popcnt64(x))
		#else
			#define bit_count(x)	(__popcnt((unsigned int) (x)) + __popcnt((unsigned int) ((x) >> 32)))
		#endif
	#else
		#define bit_count(x)	__builtin_popcountll(x)
	#endif
>>>>>>> 1c68bd5 (SSE / AVX optimized eval feature added)
#else
<<<<<<< HEAD
	int bit_weighted_count(unsigned long long);
=======
	extern unsigned char PopCnt16[1 << 16];
	static inline int bit_count(unsigned long long b) {
		union { unsigned long long bb; unsigned short u[4]; } v = { b };
		return (unsigned char)(PopCnt16[v.u[0]] + PopCnt16[v.u[1]] + PopCnt16[v.u[2]] + PopCnt16[v.u[3]]);
	}
>>>>>>> 22be102 (table lookup bit_count for non-POPCOUNT from stockfish)
#endif

<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
extern unsigned long long X_TO_BIT[];
extern const unsigned long long NEIGHBOUR[];

/** Return a bitboard with bit x set. */
// https://eukaryote.hateblo.jp/entry/2020/04/12/054905
#ifdef HAS_CPU_64 // 1% slower on Sandy Bridge
	#define x_to_bit(x) (1ULL << (x))
#else
	#define x_to_bit(x) X_TO_BIT[x]
#endif

/** Loop over each bit set. */
#if defined(tzcnt_u64)
	#define	first_bit(x)	tzcnt_u64(x)
	#define	last_bit(x)	(63 - lzcnt_u64(x))
#elif ((defined(__GNUC__) && (__GNUC__ >= 4)) || __has_builtin(__builtin_ctzll)) && !defined(__INTEL_COMPILER)
	#define	first_bit(x)	__builtin_ctzll(x)
	#define	last_bit(x)	(63 - __builtin_clzll(x))
#else
	int first_bit(unsigned long long);
	int last_bit(unsigned long long);
#endif

#if defined(HAS_CPU_64) || !defined(__STDC_HOSTED__)	// __STDC_HOSTED__ (C99) to declare var in for statement
	#define foreach_bit(i, b)	for (i = first_bit(b); b; i = first_bit(b &= (b - 1)))
#else
  #ifdef tzcnt_u32
	#define	first_bit_32(x)	tzcnt_u32(x)
  #else
	int first_bit_32(unsigned int);
  #endif
	#define foreach_bit(i, b)	(void) i; for (unsigned int _j = 0; _j < sizeof(b) * CHAR_BIT; _j += sizeof(int) * CHAR_BIT) \
		for (int _r = (b >> _j), i = first_bit_32(_r) + _j; _r; i = first_bit_32(_r &= (_r - 1)) + _j)
#endif

// popcount
#ifdef __ARM_NEON
  #ifdef HAS_CPU_64
	#define bit_count(x)	vaddv_u8(vcnt_u8(vcreate_u8(x)))
	#define bit_count_32(x)	vaddv_u8(vcnt_u8(vcreate_u8((unsigned int) x)))
  #else
	#define bit_count(x)	vget_lane_u32(vreinterpret_u32_u64(vpaddl_u32(vpaddl_u16(vpaddl_u8(vcnt_u8(vcreate_u8(x)))))), 0)
	#define bit_count_32(x)	vget_lane_u32(vpaddl_u16(vpaddl_u8(vcnt_u8(vcreate_u8(x)))), 0)
  #endif

#elif defined(POPCOUNT)
  /*
  #if defined (USE_GAS_X64)
	static inline int bit_count (unsigned long long x) {
		long long	y;
		__asm__ ( "popcntq %1,%0" : "=r" (y) : "rm" (x));
		return y;
	}
  #elif defined (USE_GAS_X86)
	static inline int bit_count (unsigned long long x) {
		unsigned int	y0, y1;
		__asm__ ( "popcntl %2,%0\n\t"
			"popcntl %3,%1"
			: "=&r" (y0), "=&r" (y1)
			: "rm" ((unsigned int) x), "rm" ((unsigned int) (x >> 32)));
		return y0 + y1;
	}
  */
  #ifdef _MSC_VER
    #if defined(_M_ARM) || defined(_M_ARM64)
	#define bit_count(x)	_CountOneBits64(x)
	#define bit_count_32(x)	_CountOneBits(x)
    #elif defined(_M_X64)
	#define bit_count(x)	((int) __popcnt64(x))
	#define bit_count_32(x)	__popcnt(x)
    #else
	#define bit_count(x)	(__popcnt((unsigned int) (x)) + __popcnt((unsigned int) ((x) >> 32)))
	#define bit_count_32(x)	__popcnt(x)
    #endif
  #else
	#define bit_count(x)	__builtin_popcountll(x)
	#define bit_count_32(x)	__builtin_popcount(x)
  #endif
	#define bit_count_si64(x)	bit_count(_mm_cvtsi128_si64(x))

#else
	extern unsigned char PopCnt16[1 << 16];
	static inline int bit_count(unsigned long long b) {
		union { unsigned long long bb; unsigned short u[4]; } v = { b };
		return (unsigned char)(PopCnt16[v.u[0]] + PopCnt16[v.u[1]] + PopCnt16[v.u[2]] + PopCnt16[v.u[3]]);
	}
	static inline int bit_count_32(unsigned int b) {
		union { unsigned int bb; unsigned short u[2]; } v = { b };
		return (unsigned char)(PopCnt16[v.u[0]] + PopCnt16[v.u[1]]);
	}
	#define bit_count_si64(x)	((unsigned char)(PopCnt16[_mm_extract_epi16((x), 0)] + PopCnt16[_mm_extract_epi16((x), 1)] + PopCnt16[_mm_extract_epi16((x), 2)] + PopCnt16[_mm_extract_epi16((x), 3)]))
#endif

#if defined(USE_GAS_MMX) || defined(USE_MSVC_X86)
  #ifndef hasSSE2
	extern bool	hasSSE2;
  #endif
  #ifndef hasMMX
	extern bool	hasMMX;
  #endif
#endif

#if defined(ANDROID) && ((defined(__arm__) && !defined(__ARM_NEON)) || (defined(__i386__) && !defined(hasSSE2)))
extern bool	hasSSE2;
#endif

/** Board : board representation */
typedef struct Board {
	unsigned long long player, opponent;     /**< bitboard representation */
} Board;

typedef union {
	unsigned long long	ull[2];
	Board	board;	// for vboard optimization in search
  #ifdef __ARM_NEON
	uint64x2_t	v2;
  #elif defined(hasSSE2) || defined(USE_MSVC_X86)
	__m128i	v2;
	__m128d	d2;	// used in flip_carry_sse_32.c
  #endif
=======
#if defined(__x86_64__) || defined(_M_X64)
=======
#if defined(__x86_64__) || defined(_M_X64) || defined(__AVX2__)
>>>>>>> cd90dbb (Enable 32bit AVX build; optimize loop in board print; set version to 4.4.6)
=======
#if defined(__SSE2__) || defined(_M_X64)
>>>>>>> bc93772 (Avoid modern compliler warnings)
	#define hasSSE2	1
#endif

#ifdef _MSC_VER
	#include <intrin.h>
	#ifdef _M_IX86
		#define	USE_MSVC_X86	1
	#endif
#elif defined(hasSSE2)
	#include <x86intrin.h>
#endif

#ifdef hasSSE2
	#define	hasMMX	1
#endif

=======
>>>>>>> 569c1f8 (More neon optimizations; split bit_intrinsics.h from bit.h)
#if defined(USE_GAS_MMX) || defined(USE_MSVC_X86)
	#ifndef hasSSE2
=======
#if defined(USE_GAS_MMX) || defined(USE_MSVC_X86) || defined(ANDROID)
	#if !defined(hasSSE2) && !defined(hasNeon)
>>>>>>> 9e2bbc5 (split get_all_full_lines from get_stability)
=======
#if defined(USE_GAS_MMX) || defined(USE_MSVC_X86)
	#ifndef hasSSE2
>>>>>>> 6c3ed52 (Dogaishi hash reduction by Matsuo & Narazaki; edge-precise get_full_line)
		extern bool	hasSSE2;
	#endif
	#ifndef hasMMX
		extern bool	hasMMX;
	#endif
#endif

#if defined(ANDROID) && ((defined(__arm__) && !defined(hasNeon)) || (defined(__i386__) && !defined(hasSSE2)))
extern bool	hasSSE2;
#endif

typedef union {
	unsigned long long	ull[2];
#if defined(hasSSE2) || defined(USE_MSVC_X86)
	__m128i	v2;
	__m128d	d2;
#endif
>>>>>>> 1dc032e (Improve visual c compatibility)
}
#if defined(__GNUC__) && !defined(hasSSE2)
__attribute__ ((aligned (16)))
#endif
V2DI;

<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
typedef union {
	unsigned long long	ull[4];
  #ifdef __AVX2__
	__m256i	v4;
  #endif
  #ifdef hasSSE2
	__m128i	v2[2];
  #endif
  #ifdef USE_MSVC_X86
	__m64	v1[4];
  #endif
} V4DI;

typedef union {
	unsigned long long	ull[8];
  #ifdef __AVX512VL__
	__m512i	v8;
  #endif
  #ifdef __AVX2__
	__m256i	v4[2];
  #endif
} V8DI;

/* Define function attributes directive when available */

#if (defined(_MSC_VER) || defined(__clang__)) && defined(hasSSE2)
	#define	vectorcall	__vectorcall
#elif defined(__GNUC__) && defined(__i386__)
	#define	vectorcall	__attribute__((sseregparm))
#elif 0 // defined(__GNUC__)	// erroreous result on pgo-build
	#define	vectorcall	__attribute__((sysv_abi))
#else
	#define	vectorcall
#endif

// X64 compatibility sims for X86
#if !defined(HAS_CPU_64) && (defined(hasSSE2) || defined(USE_MSVC_X86))
	// static inline __m128i _mm_cvtsi64_si128(const unsigned long long x) {
	//	return _mm_unpacklo_epi32(_mm_cvtsi32_si128(x), _mm_cvtsi32_si128(x >> 32));
	// }
		// better code but requires lvalue
	#define	_mm_cvtsi64_si128(x)	_mm_loadl_epi64((__m128i *) &(x))
	static inline unsigned long long vectorcall _mm_cvtsi128_si64(__m128i x) {
		return *(unsigned long long *) &x;
	}
	static inline unsigned long long vectorcall _mm_extract_epi64(__m128i x, int i) {
		return ((unsigned long long *) &x)[i];
	}

  #if defined(_MSC_VER) && _MSC_VER<1900
	static inline __m128i _mm_set_epi64x(unsigned long long b, unsigned long long a) {
		return _mm_unpacklo_epi64(_mm_cvtsi64_si128(b), _mm_cvtsi64_si128(a));
	}
	static inline __m128i _mm_set1_epi64x(unsigned long long x) {
		__m128i t = _mm_cvtsi64_si128(x);
		return _mm_unpacklo_epi64(t, t);
	}
  #endif
#endif // !HAS_CPU_64

#if __clang_major__ == 3	// undefined reference to `llvm.x86.avx.storeu.dq.256'
	#define	_mm_storeu_si128(a,b)	*(__m128i *)(a) = (b)
	#define	_mm256_storeu_si256(a,b)	*(__m256i *)(a) = (b)
=======
#ifdef __AVX2__
=======
#ifdef hasSSE2
>>>>>>> 9ad160e (4.4.7 AVX/shuffle optimization in endgame_sse.c)
typedef union {
	unsigned long long	ull[4];
	#ifdef __AVX2__
		__m256i	v4;
	#endif
	__m128i	v2[2];
} V4DI;
>>>>>>> 1dc032e (Improve visual c compatibility)
=======
typedef union {
	unsigned long long	ull[4];
#ifdef __AVX2__
	__m256i	v4;
>>>>>>> 9e2bbc5 (split get_all_full_lines from get_stability)
#endif
#ifdef hasSSE2
=======
#ifdef hasSSE2
=======
>>>>>>> 81dec96 (Kindergarten last flip for arm32; MSVC arm Windows build (not tested))
typedef union {
	unsigned long long	ull[4];
	#ifdef __AVX2__
		__m256i	v4;
	#endif
<<<<<<< HEAD
>>>>>>> 6c3ed52 (Dogaishi hash reduction by Matsuo & Narazaki; edge-precise get_full_line)
=======
	#ifdef hasSSE2
<<<<<<< HEAD
>>>>>>> 81dec96 (Kindergarten last flip for arm32; MSVC arm Windows build (not tested))
	__m128i	v2[2];
=======
		__m128i	v2[2];
	#endif
	#ifdef USE_MSVC_X86
		__m64	v1[4];
>>>>>>> 21f8809 (Share all full lines between get_stability and Dogaishi hash reduction)
	#endif
} V4DI;

/* Define function attributes directive when available */

#if defined(_MSC_VER)	// including clang-win
#define	vectorcall	__vectorcall
#elif defined(__GNUC__) && !defined(__clang__) && defined(__i386__)
#define	vectorcall	__attribute__((sseregparm))
#elif 0 // defined(__GNUC__)	// erroreous result on pgo-build
#define	vectorcall	__attribute__((sysv_abi))
#else
#define	vectorcall
#endif

// X64 compatibility sims for X86
#ifndef HAS_CPU_64
#if defined(hasSSE2) || defined(USE_MSVC_X86)
static inline __m128i _mm_cvtsi64_si128(const unsigned long long x) {
	return _mm_unpacklo_epi32(_mm_cvtsi32_si128(x), _mm_cvtsi32_si128(x >> 32));
}
#endif

// Double casting (unsigned long long) (unsigned int) improves MSVC code
#ifdef __AVX2__
static inline unsigned long long _mm_cvtsi128_si64(__m128i x) {
	return ((unsigned long long) (unsigned int) _mm_extract_epi32(x, 1) << 32)
		| (unsigned int) _mm_cvtsi128_si32(x);
}
#elif defined(hasSSE2) || defined(USE_MSVC_X86)
static inline unsigned long long _mm_cvtsi128_si64(__m128i x) {
	return ((unsigned long long) (unsigned int) _mm_cvtsi128_si32(_mm_shuffle_epi32(x, 0xb1)) << 32)
		| (unsigned int) _mm_cvtsi128_si32(x);
}
#endif
#endif // !HAS_CPU_64

#endif // EDAX_BIT_H
