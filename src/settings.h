/**
 * @file settings.h
 *
 * Various macro / constants to control algorithm usage.
 *
 * @date 1998 - 2024
 * @author Richard Delorme
 * @version 4.6
 */


#ifndef EDAX_SETTINGS_H
#define EDAX_SETTINGS_H

#include <stdbool.h>

/**
 * @brief The list of move generator
 *
 * x86-64 code tested on an AMD Ryzen 9 5950x @ 4.2 Ghz using the command "-bench 20 -n 1 -q"
 * ARM code has been tested on a rpi 5 @ 2.7 Ghz
 */
// standard C only                         // x86-64-v3 | v2        |  ARM neon
#define MOVE_GENERATOR_KINDERGARTEN   1    // 61.9 Mnps | 53.4 Mnps | 33.9 Mnps
#define MOVE_GENERATOR_ROXANE         2    // 59.4 Mnps | 51.0      | 34.7 Mnps

// simd for X86-64
// need sse support                        // v3        | v2
#define MOVE_GENERATOR_CARRY_64       3    // 65.3 Mnps | 58.1
#define MOVE_GENERATOR_BITSCAN        4    // 65.6 Mnps | 57.0
#define MOVE_GENERATOR_SSE            5    // 59.9 Mnps | 53.5
#define MOVE_GENERATOR_SSE_BITSCAN    6    // 63.3 Mnps | 49.9
// need avx2 support
#define MOVE_GENERATOR_AVX_ACEPCK     7    // 71.4 Mnps *
#define MOVE_GENERATOR_AVX_CVTPS      8    // 65.0 Mnps
#define MOVE_GENERATOR_AVX_LZCNT      9    // 69.1 Mnps
#define MOVE_GENERATOR_AVX_PPFILL    10    // 70.7 Mnps
#define MOVE_GENERATOR_AVX_PPSEQ     11    // 67.3 Mnps
// need fast bmi2 support
#define MOVE_GENERATOR_BMI2          12    // 68.6 Mnps
// need avx512 support
#define MOVE_GENERATOR_AVX512CD      13    // untested (unsupported on Zen 3 cpus)
// ARM64                                   // ARM neon
#define MOVE_GENERATOR_NEON_BITSCAN  14    // 37.7 Mnps
#define MOVE_GENERATOR_NEON_LZCNT    15    // 32.8 Mnps
#define MOVE_GENERATOR_NEON_PPFILL   16    // 31.9 Mnps
#define MOVE_GENERATOR_NEON_RBIT     17    // 33.1 Mnps
#define MOVE_GENERATOR_SVE_LZCNT     19    // untested

// standard C                              // x86-64-v3 | v2      | ARM neon
#define COUNT_LAST_FLIP_KINDERGARTEN  1    // 69.5 Mnps | 56.4    | 37.7
#define COUNT_LAST_FLIP_CARRY_64      2    // 69.9 Mnps | 56.0    | 37.6
#define COUNT_LAST_FLIP_PLAIN         3    // 70.3 Mnps | 58.3    | 33.7
// simd for x86-64
// need sse
#define COUNT_LAST_FLIP_SSE           4    // 70.0 Mnps | 58.4
#define COUNT_LAST_FLIP_LZCNT         5    // 68.1 Mnps | 56.9
#define COUNT_LAST_FLIP_BITSCAN       6    // 70.1 Mnps | 56.3
// need avx2
#define COUNT_LAST_FLIP_AVX_PPFILL    7    // 68.0 Mnps
#define COUNT_LAST_FLIP_BMI2          8    // 71.4 Mnps
#define COUNT_LAST_FLIP_BMI           9    // 67.4 Mnps
// need avx512
#define COUNT_LAST_FLIP_AVX512CD     10    // untested (unsupported on Zen 3 cpus)
// ARM64 neon
#define COUNT_LAST_FLIP_NEON         11    // 36.6
#define COUNT_LAST_FLIP_NEON_VADDVQ  12    // 36.7
#define COUNT_LAST_FLIP_SVE_LZCNT    13    // untested (unsupported on ARM-cortex-A76)

/**move generation. */
#ifndef MOVE_GENERATOR
	#ifdef __AVX512__
		#define MOVE_GENERATOR MOVE_GENERATOR_AVX512CD
	#elif defined __AVX2__
		#define MOVE_GENERATOR MOVE_GENERATOR_AVX_ACEPCK
	#elif defined __SSE2__
		#define MOVE_GENERATOR MOVE_GENERATOR_CARRY_64
	#elif defined __ARM_NEON
		#define MOVE_GENERATOR MOVE_GENERATOR_NEON_BITSCAN
	#else
		#define MOVE_GENERATOR MOVE_GENERATOR_KINDERGARTEN
	#endif
#endif

#ifndef COUNT_LAST_FLIP
	#if defined(__BMI2__) && !defined(SLOW_BMI2)
		#define COUNT_LAST_FLIP COUNT_LAST_FLIP_BMI2
	#elif defined(__SSE__)
		#define COUNT_LAST_FLIP COUNT_LAST_FLIP_SSE
	#elif defined __ARM_NEON
		#define COUNT_LAST_FLIP COUNT_LAST_FLIP_KINDERGARTEN
	#else
		#define COUNT_LAST_FLIP COUNT_LAST_FLIP_PLAIN
	#endif
#endif

/** SIMD usage */
#ifndef USE_SIMD
	#define USE_SIMD true
#endif

/** CRC32c from hardware usage */
#ifndef USE_CRC32C
	#define USE_CRC32C true
#endif

/** SOLID usage (off by default) */
#ifndef USE_SOLID
	#define USE_SOLID false
#endif

/** Depth to use Solid heuristics */
#define SOLID_DEPTH 9

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

/** Hash-n-way. */
#define HASH_N_WAY 4

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
#define BRANCHING_FACTOR 2.0

/** Parallelisable work. */
#define SMP_W 49.0

/** Critical time. */
#define SMP_C 1.0

/** Fast perft */
#define  FAST_PERFT true

/** multi_pv depth */
#define MULTIPV_DEPTH 10

#endif /* EDAX_SETTINGS_H */

