/**
 * @file bit_intrinsics.h
 *
 * CPU dependent bit operation intrinsics.
 *
 * @date 2020 - 2024
 * @author Richard Delorme
 * @author Toshihiko Okuhara
 * @version 4.5
 */

#ifndef EDAX_BIT_INTRINSICS_H
#define EDAX_BIT_INTRINSICS_H

#if defined(__x86_64__) || defined(_M_X64) || defined(__aarch64__) || defined(_M_ARM64)
	#define	HAS_CPU_64	1
#endif

#if defined(__SSE2__) || defined(__AVX__) || defined(_M_X64)
	#define hasSSE2	1
#endif

#ifdef hasSSE2
	#define	hasMMX	1
#endif

#if defined(ANDROID) && defined(__arm__)
  #if __ANDROID_API__ < 21
	#define	DISPATCH_NEON	1
  #else
	#define	__ARM_NEON	1
  #endif
#elif defined(__ARM_NEON__) || defined(__aarch64__) || defined(_M_ARM) || defined(_M_ARM64)
	#define	__ARM_NEON	1
#endif
#ifdef __ARM_NEON
	#include "arm_neon.h"
#endif

#ifdef _MSC_VER
	#include <intrin.h>
  #ifdef _M_IX86
	#define	USE_MSVC_X86	1
  #endif
#elif defined(hasSSE2)
	#include <x86intrin.h>
#endif

#ifndef __has_builtin  // Compatibility with non-clang compilers.
	#define __has_builtin(x) 0
#endif

// mirror byte
#if defined(_M_ARM) // || (defined(_M_ARM64) && _MSC_VER >= 1922)	// https://developercommunity.visualstudio.com/t/ARM64-still-missing-RBIT-intrinsics/10547420
	#define mirror_byte(b)	(_arm_rbit(b) >> 24)
#elif defined(__ARM_ACLE)
	#include <arm_acle.h>
	#define mirror_byte(b)	(__rbit(b) >> 24)
#elif defined(HAS_CPU_64)
	// http://graphics.stanford.edu/~seander/bithacks.html
	#define mirror_byte(b)	(unsigned char)((((b) * 0x80200802ULL) & 0x0884422110ULL) * 0x0101010101ULL >> 32)
#else
	static inline unsigned char mirror_byte(unsigned int b) { return ((((b * 0x200802) & 0x4422110) + ((b << 7) & 0x880)) * 0x01010101 >> 24); }
#endif

// rotl8
#if __has_builtin(__builtin_rotateleft8)
	#define rotl8(x,y)	__builtin_rotateleft8((x),(y))
#elif defined(__GNUC__) && (__GNUC__ > 4 || (__GNUC__ == 4 && __GNUC_MINOR__ >= 5)) && (defined(__x86_64__) || defined(__i386__))
	#define rotl8(x,y)	__builtin_ia32_rolqi((x),(y))
#elif defined(_MSC_VER)
	#define	rotl8(x,y)	_rotl8((x),(y))
#else	// may not compile into 8-bit rotate
	#define	rotl8(x,y)	((unsigned char)(((x)<<(y))|((unsigned char)(x)>>(8-(y)))))
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

// lzcnt / tzcnt (0 allowed)

#ifdef USE_GAS_X86
#ifdef __LZCNT__
static inline int _lzcnt_u64(unsigned long long x) {
	int	y;
	__asm__ (
		"lzcntl	%1, %0\n\t"
		"lzcntl	%2, %2\n\t"
		"leal	(%0, %2), %0\n\t"
		"cmovnc	%2, %0"
	: "=&r" (y) : "0" ((unsigned int) x), "r" ((unsigned int) (x >> 32)) );
	return y;
}
#endif
#ifdef __BMI__
static inline int _tzcnt_u64(unsigned long long x) {
	int	y;
	__asm__ (
		"tzcntl	%1, %0\n\t"
		"tzcntl	%2, %2\n\t"
		"leal	(%0, %2), %0\n\t"
		"cmovnc	%2, %0"
	: "=&r" (y) : "0" ((unsigned int) (x >> 32)), "r" ((unsigned int) x) );
	return y;
}
#endif
#elif defined(USE_MSVC_X86) && (defined(__AVX2__) || defined(__LZCNT__))
static inline int _lzcnt_u64(unsigned long long x) {
	__asm {
		lzcnt	eax, dword ptr x
		lzcnt	edx, dword ptr x+4
		lea	eax, [eax+edx]
		cmovnc	eax, edx
	}
}

static inline int _tzcnt_u64(unsigned long long x) {
	__asm {
		tzcnt	eax, dword ptr x+4
		tzcnt	edx, dword ptr x
		lea	eax, [eax+edx]
		cmovnc	eax, edx
	}
}
#endif

#if defined(__AVX2__) || defined(__LZCNT__)
	#define	lzcnt_u32(x)	_lzcnt_u32(x)
	#define	lzcnt_u64(x)	_lzcnt_u64(x)

#elif defined(_M_ARM) || defined(_M_ARM64)
	#define lzcnt_u32(x)	_CountLeadingZeros(x)
	#define lzcnt_u64(x)	_CountLeadingZeros64(x)

#elif defined(_MSC_VER)
	static inline int lzcnt_u32(unsigned int n) {
		unsigned long i;
		if (!_BitScanReverse(&i, n))
			i = 32 ^ 31;
		return i ^ 31;
	}
  #ifdef _M_X64
	static inline int lzcnt_u64(unsigned long long n) {
		unsigned long i;
		if (!_BitScanReverse64(&i, n))
			i = 64 ^ 63;
		return i ^ 63;
	}
  #else
	static inline int lzcnt_u64(unsigned long long n) {
		unsigned long i;
		if (_BitScanReverse(&i, n >> 32))
			return i ^ 31;
		if (!_BitScanReverse(&i, (unsigned int) n))
			i = 64 ^ 63;
		return i ^ 63;
	}
  #endif

#elif defined(__ARM_FEATURE_CLZ)
  #if __ARM_ACLE >= 110
	#define	lzcnt_u32(x)	__clz(x)
	#define	lzcnt_u64(x)	__clzll(x)
  #else // strictly-incorrect patch
	#define	lzcnt_u32(x)	__builtin_clz(x)
	#define	lzcnt_u64(x)	__builtin_clzll(x)
  #endif

#else
	static inline int lzcnt_u32(unsigned long x) { return (x ? __builtin_clz(x) : 32); }
	static inline int lzcnt_u64(unsigned long x) { return (x ? __builtin_clzll(x) : 64); }
#endif

#if defined(__BMI__) || defined(__AVX2__)
	#define	tzcnt_u32(x)	_tzcnt_u32(x)
	#define	tzcnt_u64(x)	_tzcnt_u64(x)

#elif defined(__ARM_FEATURE_CLZ)
  #ifdef _M_ARM
	#define	tzcnt_u32(x)	_arm_clz(_arm_rbit(x))
  #elif __has_builtin(__rbit) // (__ARM_ARCH >= 6 && __ARM_ISA_THUMB >= 2) || __ARM_ARCH >= 7	// not for gcc
	#define	tzcnt_u32(x)	__clz(__rbit(x))
  #endif
#endif

#if defined(__SSE4_2__) || defined(__AVX__)
  #ifdef HAS_CPU_64
	#define	crc32c_u64(crc,d)	_mm_crc32_u64((crc),(d))
  #else
	#define	crc32c_u64(crc,d)	_mm_crc32_u32(_mm_crc32_u32((crc),(d)),((d)>>32))
  #endif
	#define	crc32c_u8(crc,d)	_mm_crc32_u8((crc),(d))

#elif defined(__ARM_FEATURE_CRC32)
	#include "arm_acle.h"
	#define	crc32c_u64(crc,d)	__crc32cd((crc),(d))
	#define crc32c_u8(crc,d)	__crc32cb((crc),(d))

#else
	unsigned int crc32c_u64(unsigned int crc, unsigned long long data);
	unsigned int crc32c_u8(unsigned int crc, unsigned int data);
#endif

#endif // EDAX_BIT_INTRINSICS_H
