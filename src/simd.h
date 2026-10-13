/**
 * @file simd.h
 *
 * vector  operations header file.
 *
 * @date 1998 - 2026
 * @author Richard Delorme
 * @version 4.7
 */

#ifndef EDAX_SIMD_H
#define EDAX_SIMD_H

#include "settings.h"

#if defined(_MSC_VER)
	#include <intrin.h>
#elif defined(__x86_64__)
	#include <x86intrin.h>
#endif
#if defined(__ARM_NEON)
	#include <arm_neon.h>
#endif
#include <stdalign.h>
#include <stdint.h>

/**
 * union V8DI to represent an array of 8 64 bits as smaller array of parallel vector.
 */

typedef union {
	alignas(64) uint64_t v1[8]; //< an array of 8 64 bit integers.
	#ifdef __SSE2__
		__m128i	v2[4];          //< an array of 4 2x64 bit integers.
	#endif
	#ifdef __AVX2__
		__m256i	v4[2];          //< an array of 2 4x64 bit integers.
	#endif
	#ifdef __AVX512VL__
		__m512i	v8;             //< one 8x64 bit integer.
	#endif
} V8DI;

/**
 * union V8DI to represent an array of 4 64 bits as smaller array of parallel vector.
 */
typedef union {
	alignas(32) uint64_t v1[4];  //< an array of 4 64 bit integers.
	#ifdef __SSE2__
		__m128i	v2[2];           //< an array of 2 2x64 bit integers.
	#endif
	#ifdef __AVX2__
		__m256i	v4;              // one 4x64 bit integer.
	#endif
} V4DI;

typedef union {
	alignas(16) uint64_t v1[2]; //< an array of 2 64 bit integers.
	#ifdef __SSE2__
		__m128i	v2;             //< 1 2x64 bit integer.
	#endif
} V2DI;

// slow bmi on old AMD cpu
#if defined(__bdver4__) || defined(__znver1__) || defined(__znver2__)
	#define SLOW_BMI2
#endif

// slow gather
#if defined(SLOW_BMI2) || defined(__znver3__)
	#define SLOW_GATHER
#endif

// vectorcall
#if defined(__clang__) || defined(_MSC_VER)
	#define vectorcall __vectorcall
#elif defined(__GNUC__) && defined(__i386__)
	#define	vectorcall	__attribute__((sseregparm))
#else
	#define vectorcall
#endif

#endif /* EDAX_SIMD_H */
