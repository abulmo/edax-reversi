/**
 * @file settings.h
 *
 * Various macro / constants to control algorithm usage.
 *
 * @date 1998 - 2012
 * @author Richard Delorme
 * @version 4.3
 */


#ifndef EDAX_SETTINGS_H
#define EDAX_SETTINGS_H

#include <stdbool.h>

#define MOVE_GENERATOR_CARRY 1
#define MOVE_GENERATOR_KINDERGARTEN 2
#define MOVE_GENERATOR_SSE 3
#define MOVE_GENERATOR_BITSCAN 4
#define MOVE_GENERATOR_ROXANE 5

/**move generation. */
#define MOVE_GENERATOR MOVE_GENERATOR_CARRY

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

/** LockFree Hash Table */
#define USE_HASH_LOCK false

/** Allow type puning */
#ifndef USE_TYPE_PUNING
#ifdef ANDROID
#define USE_TYPE_PUNING false
#else
#define USE_TYPE_PUNING true
#endif
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

/** Stop Node splitting (for parallel search) when few move remains.  */
#define MAX_SLAVES 1

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

