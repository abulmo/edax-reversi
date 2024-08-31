<<<<<<< HEAD
<<<<<<< HEAD
=======
>>>>>>> 3e1ed4f (fix cr/lf in repository to lf)
/**
 * @file settings.h
 *
 * Various macro / constants to control algorithm usage.
 *
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
 * @date 1998 - 2024
=======
 * @date 1998 - 2017
>>>>>>> b3f048d (copyright changes)
=======
 * @date 1998 - 2018
>>>>>>> 1dc032e (Improve visual c compatibility)
 * @author Richard Delorme
 * @version 4.5
=======
 * @date 1998 - 2020
 * @author Richard Delorme
 * @version 4.4
>>>>>>> 3e1ed4f (fix cr/lf in repository to lf)
 */


#ifndef EDAX_SETTINGS_H
#define EDAX_SETTINGS_H

#include <stdbool.h>

#define MOVE_GENERATOR_CARRY 1		// 32.6Mnps
#define MOVE_GENERATOR_KINDERGARTEN 2	// 31.1Mnps
<<<<<<< HEAD
<<<<<<< HEAD
#define MOVE_GENERATOR_SSE 3		// 34.4Mnps	// best for generic X64
#define MOVE_GENERATOR_BITSCAN 4	// 32.7Mnps	// best for AMD K10/FX	// 7.21Mnps (neon_bitscan)
=======
#define MOVE_GENERATOR_SSE 3		// 34.4Mnps	// best for generic X64
<<<<<<< HEAD
#define MOVE_GENERATOR_BITSCAN 4	// 32.7Mnps	// best for AMD K10/FX
>>>>>>> 3e1ed4f (fix cr/lf in repository to lf)
=======
#define MOVE_GENERATOR_BITSCAN 4	// 32.7Mnps	// best for AMD K10/FX	// 7.21Mnps (neon_bitscan)
>>>>>>> 343493d (More neon/sse optimizations; neon dispatch added for arm32)
#define MOVE_GENERATOR_ROXANE 5		// 29.0Mnps
#define MOVE_GENERATOR_32 6		// 31.3Mnps	// best for 32bit X86
#define MOVE_GENERATOR_SSE_BSWAP 7	// 30.6Mnps
#define MOVE_GENERATOR_AVX 8		// 34.7Mnps	// best for modern X64
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
#define MOVE_GENERATOR_AVX512 9
#define MOVE_GENERATOR_NEON 10		// 6.71Mnps (neon_rbit), 6.51Mnps (neon_lzcnt), 6.17Mnps (neon_ppfill)
#define MOVE_GENERATOR_SVE 11

<<<<<<< HEAD
#define COUNT_LAST_FLIP_CARRY 1		// 33.8Mnps
=======
=======
=======
#define MOVE_GENERATOR_AVX512	9
<<<<<<< HEAD
<<<<<<< HEAD
>>>>>>> 393b667 (Experimental AVX512VL/CD version of move generator)
#define MOVE_GENERATOR_NEON 10
>>>>>>> f2da03e (Refine arm builds adding neon support.)
=======
#define MOVE_GENERATOR_NEON 10		// neon_lzcnt (6.51Mnps), neon_ppfill (5.55Mnps)
#define MOVE_GENERATOR_NEON_BITSCAN 11	// neon_bitscan (6.43Mnps)
>>>>>>> 569c1f8 (More neon optimizations; split bit_intrinsics.h from bit.h)
=======
#define MOVE_GENERATOR_NEON 10		// 6.71Mnps (neon_rbit), 6.51Mnps (neon_lzcnt), 6.17Mnps (neon_ppfill)
>>>>>>> 343493d (More neon/sse optimizations; neon dispatch added for arm32)

#define	COUNT_LAST_FLIP_CARRY 1		// 33.8Mnps
>>>>>>> 3e1ed4f (fix cr/lf in repository to lf)
#define COUNT_LAST_FLIP_KINDERGARTEN 2	// 33.5Mnps
#define COUNT_LAST_FLIP_SSE 3		// 34.7Mnps
#define COUNT_LAST_FLIP_BITSCAN 4	// 33.9Mnps
#define COUNT_LAST_FLIP_PLAIN 5		// 33.3Mnps
#define COUNT_LAST_FLIP_32 6		// 33.1Mnps
#define COUNT_LAST_FLIP_BMI2 7		// 34.7Mnps	// slow on AMD
<<<<<<< HEAD
#define COUNT_LAST_FLIP_AVX_PPFILL 8
#define COUNT_LAST_FLIP_AVX512 9
#define COUNT_LAST_FLIP_NEON 10
#define COUNT_LAST_FLIP_SVE 11
=======
#define	COUNT_LAST_FLIP_CARRY 1
#define COUNT_LAST_FLIP_KINDERGARTEN 2
#define COUNT_LAST_FLIP_SSE 3
#define COUNT_LAST_FLIP_BITSCAN 4
#define COUNT_LAST_FLIP_PLAIN 5
#define COUNT_LAST_FLIP_32 6
#define COUNT_LAST_FLIP_BMI2 7
>>>>>>> feb7fa7 (count_last_flip_bmi2 and transpose_avx2 added)

/**move generation. */
#ifndef MOVE_GENERATOR
	#if defined(__AVX512VL__) || defined(__AVX10_1__)
		#define MOVE_GENERATOR MOVE_GENERATOR_AVX512
	#elif defined(__AVX2__)
		#define MOVE_GENERATOR MOVE_GENERATOR_AVX
	#elif defined(__SSE2__) || defined(_M_X64) || defined(hasSSE2)
		#define MOVE_GENERATOR MOVE_GENERATOR_SSE
	#elif defined(__ARM_FEATURE_SVE)
		#define MOVE_GENERATOR MOVE_GENERATOR_SVE
	#elif defined(__aarch64__) || defined(_M_ARM64) || defined(__ARM_NEON)
		#define MOVE_GENERATOR MOVE_GENERATOR_NEON
	#elif defined(__arm__) || defined(_M_ARM)
		#define MOVE_GENERATOR MOVE_GENERATOR_BITSCAN
=======
#define MOVE_GENERATOR_SSE 3		// 33.3Mnps
#define MOVE_GENERATOR_BITSCAN 4	// 32.7Mnps
#define MOVE_GENERATOR_ROXANE 5		// 29.0Mnps
#define MOVE_GENERATOR_32 6		// 31.3Mnps
#define MOVE_GENERATOR_SSE_BSWAP 7	// 30.6Mnps
#define MOVE_GENERATOR_AVX 8		// 34.4Mnps

#define	COUNT_LAST_FLIP_CARRY 1		// 33.8Mnps
#define COUNT_LAST_FLIP_KINDERGARTEN 2	// 33.5Mnps
#define COUNT_LAST_FLIP_SSE 3		// 33.2Mnps
#define COUNT_LAST_FLIP_BITSCAN 4	// 33.9Mnps
#define COUNT_LAST_FLIP_PLAIN 5		// 33.3Mnps
#define COUNT_LAST_FLIP_32 6		// 33.1Mnps
#define COUNT_LAST_FLIP_BMI2 7		// 34.4Mnps

/**move generation. */
#ifndef MOVE_GENERATOR
	#if defined(__x86_64__) || defined(_M_X64)
		#ifdef __AVX2__
			#define MOVE_GENERATOR MOVE_GENERATOR_AVX
		#else
			#define MOVE_MOVE_GENERATOR MOVE_GENERATOR_BITSCAN
		#endif
>>>>>>> 1dc032e (Improve visual c compatibility)
=======

/**move generation. */
#ifndef MOVE_GENERATOR
	#if defined(__AVX512CD__) && defined(__AVX512VL__)
		#define MOVE_GENERATOR MOVE_GENERATOR_AVX512
	#elif defined(__AVX2__)
		#define MOVE_GENERATOR MOVE_GENERATOR_AVX
	#elif defined(__SSE2__) || defined(_M_X64) || defined(hasSSE2)
		#define MOVE_GENERATOR MOVE_GENERATOR_SSE
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
>>>>>>> 3e1ed4f (fix cr/lf in repository to lf)
=======
	#elif defined(HAS_CPU_64) // aarch64
		#define MOVE_GENERATOR MOVE_GENERATOR_BITSCAN
>>>>>>> f2da03e (Refine arm builds adding neon support.)
=======
	#elif defined(__aarch64__)
		#define MOVE_GENERATOR MOVE_GENERATOR_NEON
>>>>>>> 569c1f8 (More neon optimizations; split bit_intrinsics.h from bit.h)
=======
	#elif defined(__aarch64__) || defined(_M_ARM64)
		#define MOVE_GENERATOR MOVE_GENERATOR_NEON
	#elif defined(__arm__) || defined(_M_ARM)
		#define MOVE_GENERATOR MOVE_GENERATOR_BITSCAN
>>>>>>> 343493d (More neon/sse optimizations; neon dispatch added for arm32)
	#else
		#define MOVE_GENERATOR MOVE_GENERATOR_32
	#endif
#endif
#ifndef LAST_FLIP_COUNTER
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
	#if (defined(__AVX512VL__) || defined(__AVX10_1__)) && (defined(SIMULLASTFLIP512) || defined(SIMULLASTFLIP) || defined(LASTFLIP_HIGHCUT))
		#define LAST_FLIP_COUNTER COUNT_LAST_FLIP_AVX512
	#elif defined(__SSE2__) || defined(_M_X64) || defined(hasSSE2)
		#define LAST_FLIP_COUNTER COUNT_LAST_FLIP_SSE
	#elif defined(__aarch64__) || defined(_M_ARM64) || defined(__ARM_NEON)
		#define LAST_FLIP_COUNTER COUNT_LAST_FLIP_NEON
	#elif defined(__arm__) || defined(_M_ARM)
		#define LAST_FLIP_COUNTER COUNT_LAST_FLIP_BITSCAN
=======
	#ifdef __x86_64__
		#if defined(__BMI2__) && !defined(__BDVER4__) && !defined(__ZNVER1__)	// BMI2 is slow on AMD
=======
	#if defined(__x86_64__) || defined(_M_X64)
		#if defined(__AVX2__) && !defined(__BDVER4__) && !defined(__ZNVER1__)	// BMI2 is slow on AMD
>>>>>>> 1dc032e (Improve visual c compatibility)
			#define LAST_FLIP_COUNTER COUNT_LAST_FLIP_BMI2
		#else
			#define LAST_FLIP_COUNTER COUNT_LAST_FLIP_BITSCAN
		#endif
>>>>>>> feb7fa7 (count_last_flip_bmi2 and transpose_avx2 added)
=======
	#ifdef hasSSE2
		#define LAST_FLIP_COUNTER COUNT_LAST_FLIP_SSE
<<<<<<< HEAD
<<<<<<< HEAD
		// #define AVXLASTFLIP	1	// slower on slow vzeroupper CPU
>>>>>>> 3e1ed4f (fix cr/lf in repository to lf)
=======
	#elif defined(HAS_CPU_64) // aarch64
		#define LAST_FLIP_COUNTER COUNT_LAST_FLIP_CARRY
>>>>>>> f2da03e (Refine arm builds adding neon support.)
=======
	#elif defined(__aarch64__)
=======
	#if defined(__SSE2__) || defined(_M_X64) || defined(hasSSE2) || defined(__aarch64__) || defined(_M_ARM64)
		#define LAST_FLIP_COUNTER COUNT_LAST_FLIP_SSE
	#elif defined(__arm__) || defined(_M_ARM)
>>>>>>> 343493d (More neon/sse optimizations; neon dispatch added for arm32)
		#define LAST_FLIP_COUNTER COUNT_LAST_FLIP_BITSCAN
>>>>>>> 569c1f8 (More neon optimizations; split bit_intrinsics.h from bit.h)
	#else
		#define LAST_FLIP_COUNTER COUNT_LAST_FLIP_32
	#endif
#endif

/** transposition cutoff usage. */
#define USE_TC true

/** stability cutoff usage. */
#define USE_SC true

/** enhanced transposition cutoff usage. */
#define USE_ETC true

/** probcut usage. */
#define USE_PROBCUT true

/** Use recursive probcut */
#define USE_RECURSIVE_PROBCUT true

/** limit recursive probcut level */
#define LIMIT_RECURSIVE_PROBCUT(x) x

/** kogge-stone parallel prefix algorithm usage.
 *  0 -> none, 1 -> move generator, 2 -> stability, 3 -> both.
 */
#define KOGGE_STONE 2

/** 1 stage parallel prefix algorithm usage.
 *  0 -> none, 1 -> move generator, 2 -> stability, 3 -> both.
 */
#define PARALLEL_PREFIX 1

#if (KOGGE_STONE & PARALLEL_PREFIX)
	#error "usage of 2 incompatible algorithms"
#endif

/** Internal Iterative Deepening. */
#define USE_IID false

/** Use previous search result */
#define USE_PREVIOUS_SEARCH true

/** Allow type puning */
#ifndef USE_TYPE_PUNING
// #ifndef ANDROID
#define USE_TYPE_PUNING 1
// #endif
#endif

/** Hash-n-way. */
#define HASH_N_WAY 4

/** hash align */
#define HASH_ALIGNED 1

/** PV extension (solve PV alone sooner) */
#define USE_PV_EXTENSION true

/** Swith from endgame to shallow search (faster but less node efficient) at this depth. */
#define DEPTH_TO_SHALLOW_SEARCH 7

/** Switch from midgame to endgame search (faster but less node efficient) at this depth. */
#define DEPTH_MIDGAME_TO_ENDGAME 15

/** Switch from midgame result (evaluated score) to endgame result (exact score) at this number of empties. */
#define ITERATIVE_MIN_EMPTIES 10

/** Store bestmoves in the pv_hash up to this height. */
#define PV_HASH_HEIGHT 5

/** Try ETC down to this depth. */
#define ETC_MIN_DEPTH 5

<<<<<<< HEAD
/** Dogaishi hash reduction Depth (before DEPTH_TO_SHALLOW_SEARCH) */
#define MASK_SOLID_DEPTH 9

=======
>>>>>>> 3e1ed4f (fix cr/lf in repository to lf)
/** bound for usefull move sorting */
#define SORT_ALPHA_DELTA 8

/** Try Node splitting (for parallel search) down to that depth. */
#define SPLIT_MIN_DEPTH 5

/** Stop Node splitting (for parallel search) when few move remains.  */
#define SPLIT_MIN_MOVES_TODO 1

/** Stop Node splitting (for parallel search) after a few splitting.  */
#define SPLIT_MAX_SLAVES 3

/** Branching factor (to adjust alloted time). */
#define BRANCHING_FACTOR 2.24

/** Parallelisable work. */
#define SMP_W 49.0

/** Critical time. */
#define SMP_C 1.0

/** Fast perft */
#define  FAST_PERFT true

/** multi_pv depth */
#define MULTIPV_DEPTH 10

#endif /* EDAX_SETTINGS_H */

<<<<<<< HEAD
=======
/**
 * @file settings.h
 *
 * Various macro / constants to control algorithm usage.
 *
 * @date 1998 - 2020
 * @author Richard Delorme
 * @version 4.4
 */


#ifndef EDAX_SETTINGS_H
#define EDAX_SETTINGS_H

#include <stdbool.h>

#define MOVE_GENERATOR_CARRY 1		// 32.6Mnps
#define MOVE_GENERATOR_KINDERGARTEN 2	// 31.1Mnps
#define MOVE_GENERATOR_SSE 3		// 34.4Mnps	// best for generic X64
#define MOVE_GENERATOR_BITSCAN 4	// 32.7Mnps	// best for AMD K10/FX
#define MOVE_GENERATOR_ROXANE 5		// 29.0Mnps
#define MOVE_GENERATOR_32 6		// 31.3Mnps	// best for 32bit X86
#define MOVE_GENERATOR_SSE_BSWAP 7	// 30.6Mnps
#define MOVE_GENERATOR_AVX 8		// 34.7Mnps	// best for modern X64

#define	COUNT_LAST_FLIP_CARRY 1		// 33.8Mnps
#define COUNT_LAST_FLIP_KINDERGARTEN 2	// 33.5Mnps
#define COUNT_LAST_FLIP_SSE 3		// 34.7Mnps
#define COUNT_LAST_FLIP_BITSCAN 4	// 33.9Mnps
#define COUNT_LAST_FLIP_PLAIN 5		// 33.3Mnps
#define COUNT_LAST_FLIP_32 6		// 33.1Mnps
#define COUNT_LAST_FLIP_BMI2 7		// 34.7Mnps	// slow on AMD

/**move generation. */
#ifndef MOVE_GENERATOR
	#ifdef __AVX2__
		#define MOVE_GENERATOR MOVE_GENERATOR_AVX
	#elif defined(hasSSE2)
		#define MOVE_GENERATOR MOVE_GENERATOR_SSE
	#else
		#define MOVE_GENERATOR MOVE_GENERATOR_32
	#endif
#endif
#ifndef LAST_FLIP_COUNTER
	#ifdef hasSSE2
		#define LAST_FLIP_COUNTER COUNT_LAST_FLIP_SSE
		// #define AVXLASTFLIP	1	// slower on slow vzeroupper CPU
	#else
		#define LAST_FLIP_COUNTER COUNT_LAST_FLIP_32
	#endif
#endif

/** transposition cutoff usage. */
#define USE_TC true

/** stability cutoff usage. */
#define USE_SC true

/** enhanced transposition cutoff usage. */
#define USE_ETC true

/** probcut usage. */
#define USE_PROBCUT true

/** Use recursive probcut */
#define USE_RECURSIVE_PROBCUT true

/** limit recursive probcut level */
#define LIMIT_RECURSIVE_PROBCUT(x) x

/** kogge-stone parallel prefix algorithm usage.
 *  0 -> none, 1 -> move generator, 2 -> stability, 3 -> both.
 */
#define KOGGE_STONE 2

/** 1 stage parallel prefix algorithm usage.
 *  0 -> none, 1 -> move generator, 2 -> stability, 3 -> both.
 */
#define PARALLEL_PREFIX 1

#if (KOGGE_STONE & PARALLEL_PREFIX)
	#error "usage of 2 incompatible algorithms"
#endif

/** Internal Iterative Deepening. */
#define USE_IID false

/** Use previous search result */
#define USE_PREVIOUS_SEARCH true

/** Allow type puning */
#ifndef USE_TYPE_PUNING
// #ifndef ANDROID
#define USE_TYPE_PUNING 1
// #endif
#endif

/** Hash-n-way. */
#define HASH_N_WAY 4

/** hash align */
#define HASH_ALIGNED 1

/** PV extension (solve PV alone sooner) */
#define USE_PV_EXTENSION true

/** Swith from endgame to shallow search (faster but less node efficient) at this depth. */
#define DEPTH_TO_SHALLOW_SEARCH 7

/** Switch from midgame to endgame search (faster but less node efficient) at this depth. */
#define DEPTH_MIDGAME_TO_ENDGAME 15

/** Switch from midgame result (evaluated score) to endgame result (exact score) at this number of empties. */
#define ITERATIVE_MIN_EMPTIES 10

/** Store bestmoves in the pv_hash up to this height. */
#define PV_HASH_HEIGHT 5

/** Try ETC down to this depth. */
#define ETC_MIN_DEPTH 5

/** bound for usefull move sorting */
#define SORT_ALPHA_DELTA 8

/** Try Node splitting (for parallel search) down to that depth. */
#define SPLIT_MIN_DEPTH 5

/** Stop Node splitting (for parallel search) when few move remains.  */
#define SPLIT_MIN_MOVES_TODO 1

/** Stop Node splitting (for parallel search) after a few splitting.  */
#define SPLIT_MAX_SLAVES 3

/** Branching factor (to adjust alloted time). */
#define BRANCHING_FACTOR 2.24

/** Parallelisable work. */
#define SMP_W 49.0

/** Critical time. */
#define SMP_C 1.0

/** Fast perft */
#define  FAST_PERFT true

/** multi_pv depth */
#define MULTIPV_DEPTH 10

#endif /* EDAX_SETTINGS_H */

>>>>>>> cd90dbb (Enable 32bit AVX build; optimize loop in board print; set version to 4.4.6)
=======
>>>>>>> 3e1ed4f (fix cr/lf in repository to lf)
