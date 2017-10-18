/**
 * @file stats.h
 *
 * @brief Statistics header.
 *
 * @date 1998 - 2017
 * @author Richard Delorme
 * @version 4.4
 */

#ifndef EDAX_STATS_H
#define EDAX_STATS_H

#include "const.h"
#include "util.h"

#include <stdio.h>

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
	#define SEARCH_UPDATE_INTERNAL_NODES() (++search->n_nodes)
#else
	/** no node counter for internal nodes */
	#define SEARCH_UPDATE_INTERNAL_NODES()
#endif
#if COUNT_NODES & 2
	/** node counter for pattern changes */
	#define SEARCH_UPDATE_EVAL_NODES() (++search->n_nodes)
#else
	/** no node counter for pattern changes */
	#define SEARCH_UPDATE_EVAL_NODES()
#endif
#if COUNT_NODES & 4
	/** more general node counter */
	#define SEARCH_UPDATE_ALL_NODES() (++search->n_nodes)
#else
	/** no general node counter */
	#define SEARCH_UPDATE_ALL_NODES()
#endif

/** \struct Statistics */
typedef struct Statistics {
	unsigned long long n_nodes;
	unsigned long long n_task_nodes[MAX_THREADS];
	unsigned long long n_task[MAX_THREADS];
	unsigned long long n_parallel_nodes;

	unsigned long long n_hash_update;
	unsigned long long n_hash_upgrade;
	unsigned long long n_hash_new;
	unsigned long long n_hash_remove;
	unsigned long long n_hash_search;
	unsigned long long n_hash_found;
	unsigned long long n_hash_collision;
	unsigned long long n_hash_n;

	unsigned long long n_PVS_root;
	unsigned long long n_PVS_midgame;
	unsigned long long n_NWS_midgame;
	unsigned long long n_NWS_endgame;
	unsigned long long n_PVS_shallow;
	unsigned long long n_NWS_shallow;
	unsigned long long n_search_solve;
	unsigned long long n_search_solve_0;
	unsigned long long n_board_solve_2;
	unsigned long long n_search_solve_3;
	unsigned long long n_search_solve_4;
	unsigned long long n_search_eval_0;
	unsigned long long n_search_eval_1;
	unsigned long long n_search_eval_2;
	unsigned long long n_cut_at_move_number[MAX_MOVE];
	unsigned long long n_nocut_at_move_number[MAX_MOVE];
	unsigned long long n_best_at_move_number[MAX_MOVE];
	unsigned long long n_move_number[MAX_MOVE];

	unsigned long long n_split_try;
	unsigned long long n_split_success;
	unsigned long long n_master_helper;
	unsigned long long n_waited_slave;
	unsigned long long n_stopped_slave;
	unsigned long long n_stopped_master;
	unsigned long long n_wake_up;

	unsigned long long n_hash_try, n_hash_low_cutoff, n_hash_high_cutoff;
	unsigned long long n_stability_try, n_stability_low_cutoff;
	unsigned long long n_probcut_try;
	unsigned long long n_probcut_low_try, n_probcut_low_cutoff;
	unsigned long long n_probcut_high_try, n_probcut_high_cutoff;
	unsigned long long n_etc_try, n_etc_high_cutoff, n_esc_high_cutoff;

	unsigned long long n_played_square[BOARD_SIZE][10];
	unsigned long long n_good_square[BOARD_SIZE][10];

	unsigned long long n_NWS_candidate;
	unsigned long long n_NWS_bad_candidate;

} Statistics;

extern Statistics statistics;
struct Search;

void statistics_init(void);
void statistics_sum_nodes(struct Search*);
void statistics_print(FILE*);

#endif

