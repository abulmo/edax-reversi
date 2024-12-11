/**
 * @file stats.h
 *
 * @brief Statistics header.
 *
 * @date 1998 - 2024
 * @author Richard Delorme
 * @version 4.6
 */

#ifndef EDAX_STATS_H
#define EDAX_STATS_H

#include "const.h"
#include "util.h"

#include <stdatomic.h>
#include <stdio.h>
#include <stdint.h>
#include <inttypes.h>

/* To turn on a statistics, add an x to the end of the line starting with #define .*/

/** YBWC statistics on/off */
#define YBWC_STATS(x)
/** Hash statistics on/off */
#define HASH_STATS(x)
/** Hash collision on/off */
#define HASH_COLLISIONS(x)
/** Search statistics on/off */
#define SEARCH_STATS(x)
/** Square type statistics on/off */
#define SQUARE_STATS(x)
/** Cutoff statistics on/off */
#define CUTOFF_STATS(x)
/** ProbCut statistics on/off */
#define PROBCUT_STATS(x)

/** how to count nodes... */
#define COUNT_NODES 7

#if COUNT_NODES & 1
	/** node counter for internal nodes */
	#define SEARCH_UPDATE_INTERNAL_NODES(n) (++(n))
#else
	/** no node counter for internal nodes */
	#define SEARCH_UPDATE_INTERNAL_NODES(n)
#endif
#if COUNT_NODES & 2
	/** node counter for pattern changes */
	#define SEARCH_UPDATE_EVAL_NODES(n) (++(n))
#else
	/** no node counter for pattern changes */
	#define SEARCH_UPDATE_EVAL_NODES(n)
#endif
#if COUNT_NODES & 4
	/** more general node counter */
	#define SEARCH_UPDATE_ALL_NODES(n) (++(n))
#else
	/** no general node counter */
	#define SEARCH_UPDATE_ALL_NODES(n)
#endif

/** struct Statistics */
typedef struct Statistics {
	uint64_t n_nodes;
	uint64_t n_task_nodes[MAX_THREADS];
	uint64_t n_task[MAX_THREADS];
	uint64_t n_parallel_nodes;

	uint64_t n_hash_update;
	uint64_t n_hash_upgrade;
	uint64_t n_hash_new;
	uint64_t n_hash_remove;
	uint64_t n_hash_search;
	uint64_t n_hash_found;
	uint64_t n_hash_collision;
	uint64_t n_hash_n;

	uint64_t n_PVS_root;
	uint64_t n_PVS_midgame;
	uint64_t n_NWS_midgame;
	uint64_t n_NWS_endgame;
	uint64_t n_PVS_shallow;
	uint64_t n_NWS_shallow;
	uint64_t n_solve;
	uint64_t n_solve_0;
	uint64_t n_solve_1;
	uint64_t n_solve_2;
	uint64_t n_solve_3;
	uint64_t n_search_solve_4;
	uint64_t n_search_eval_0;
	uint64_t n_search_eval_1;
	uint64_t n_search_eval_2;
	uint64_t n_cut_at_move_number[MAX_MOVE];
	uint64_t n_nocut_at_move_number[MAX_MOVE];
	uint64_t n_best_at_move_number[MAX_MOVE];
	uint64_t n_move_number[MAX_MOVE];

	_Atomic uint64_t n_split_try;
	_Atomic uint64_t n_split_success;
	_Atomic uint64_t n_master_helper;
	_Atomic uint64_t n_waited_slave;
	_Atomic uint64_t n_stopped_slave;
	_Atomic uint64_t n_stopped_master;
	_Atomic uint64_t n_wake_up;

	uint64_t n_hash_try, n_hash_low_cutoff, n_hash_high_cutoff;
	uint64_t n_stability_try, n_stability_low_cutoff;
	uint64_t n_probcut_try;
	uint64_t n_probcut_low_try, n_probcut_low_cutoff;
	uint64_t n_probcut_high_try, n_probcut_high_cutoff;
	uint64_t n_etc_try, n_etc_high_cutoff, n_esc_high_cutoff;

	uint64_t n_played_square[BOARD_SIZE][10];
	uint64_t n_good_square[BOARD_SIZE][10];

	uint64_t n_NWS_candidate;
	uint64_t n_NWS_bad_candidate;

} Statistics;

extern Statistics statistics;
struct Search;

void statistics_init(void);
void statistics_sum_nodes(struct Search*);
void statistics_print(FILE*);

#endif

