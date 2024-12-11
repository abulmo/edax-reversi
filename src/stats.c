/**
 * @file stats.c
 *
 * @brief Statistics.
 *
 * The purpose of these functions is to gather performance
 * statistics on some algorithms or code.
 *
 * @date 1998 - 2024
 * @author Richard Delorme
 * @author Toshihiko Okuhara
 * @version 4.6
 */

#include "stats.h"
#include "options.h"
#include "search.h"
#include "ybwc.h"

#include <stdio.h>
#include <stdint.h>
#include <inttypes.h>

Statistics statistics;

/**
 * @brief Intialization of the statistics.
 */
void statistics_init(void)
{
	int i, j;

	statistics.n_hash_upgrade = 0;
	statistics.n_hash_update = 0;
	statistics.n_hash_new = 0;
	statistics.n_hash_remove = 0;
	statistics.n_hash_search = 0;
	statistics.n_hash_found = 0;
	statistics.n_hash_collision = 0;
	statistics.n_hash_n = 0;

	for (i = 0; i < MAX_THREADS; ++i) {
		statistics.n_task_nodes[i] = 0;
		statistics.n_task[i] = 0;
	}
	statistics.n_parallel_nodes = 0;
	statistics.n_nodes = 0;
	statistics.n_split_try = 0;
	statistics.n_split_success = 0;
	statistics.n_master_helper = 0;
	statistics.n_stopped_slave = 0;
	statistics.n_stopped_master = 0;
	statistics.n_waited_slave = 0;
	statistics.n_wake_up = 0;

	statistics.n_PVS_root = 0;
	statistics.n_PVS_midgame = 0;
	statistics.n_NWS_midgame = 0;
	statistics.n_NWS_endgame = 0;
	statistics.n_PVS_shallow = 0;
	statistics.n_NWS_shallow = 0;
	statistics.n_solve = 0;
	statistics.n_solve_0 = 0;
	statistics.n_solve_1 = 0;
	statistics.n_solve_2 = 0;
	statistics.n_solve_3 = 0;
	statistics.n_search_solve_4 = 0;
	statistics.n_search_eval_0 = 0;
	statistics.n_search_eval_1 = 0;
	statistics.n_search_eval_2 = 0;

	statistics.n_hash_try = 0;
	statistics.n_hash_low_cutoff = 0;
	statistics.n_hash_high_cutoff = 0;
	statistics.n_stability_try = 0;
	statistics.n_stability_low_cutoff = 0;
	statistics.n_probcut_try = 0;
	statistics.n_probcut_low_try = 0;
	statistics.n_probcut_high_try = 0;
	statistics.n_probcut_low_cutoff = 0;
	statistics.n_probcut_high_cutoff = 0;
	statistics.n_etc_try = 0;
	statistics.n_etc_high_cutoff = 0;
	statistics.n_esc_high_cutoff = 0;

	for (j = 0; j < BOARD_SIZE; ++j)
	for (i = 0; i < 10; ++i) {
		statistics.n_played_square[j][i] = 0;
		statistics.n_good_square[j][i] = 0;
	}

	statistics.n_NWS_candidate = 0;
	statistics.n_NWS_bad_candidate = 0;
}

/**
 * @brief Cumulate node counts from the last search.
 *
 * @param search Last search.
 */
void statistics_sum_nodes(Search *search)
{
	int i;

	statistics.n_parallel_nodes += search->child_nodes;
	statistics.n_nodes += search->n_nodes;
	for (i = 0; i < search->tasks->n; ++i) {
		statistics.n_task_nodes[i] = search->tasks->task[i].n_nodes;
		statistics.n_task[i] = search->tasks->task[i].n_calls;
	}
}

/**
 * @brief Print statistics.
 */
void statistics_print(FILE *f)
{
	int i, j;

	statistics.n_split_success += statistics.n_master_helper;

	if (statistics.n_split_success) {
		uint64_t n_helper_nodes = statistics.n_parallel_nodes;
		fprintf(f, "YBWC:\n");
		fprintf(f, "nodes splitted:      %12" PRIu64 " (%6.2f%%)\n", statistics.n_split_success, 100.0 * statistics.n_split_success / statistics.n_split_try);
		fprintf(f, "master helper tasks: %12" PRIu64 " (%6.2f%%)\n", statistics.n_master_helper, 100.0 * statistics.n_master_helper / statistics.n_split_success);
		fprintf(f, "slave nodes stopped: %12" PRIu64 " (%6.2f%%)\n", statistics.n_stopped_slave, 100.0 * statistics.n_stopped_slave / statistics.n_split_success);
		fprintf(f, "slave master stopped:%12" PRIu64 " (%6.2f%%) = %12" PRIu64 "\n", statistics.n_stopped_master, 100.0 * statistics.n_stopped_master / statistics.n_split_success, statistics.n_wake_up);
		fprintf(f, "slave nodes waited:  %12" PRIu64 " (%6.2f%%)\n", statistics.n_waited_slave, 100.0 * statistics.n_waited_slave / statistics.n_split_success);
		fprintf(f, "main thread (%" PRIu64 " nodes)\n", statistics.n_nodes);
		for (i = 1; i < options.n_task; ++i) {
			fprintf(f, "task %d called %" PRIu64 " times (%" PRIu64 " nodes)\n", i, statistics.n_task[i], statistics.n_task_nodes[i]);
			n_helper_nodes -= statistics.n_task_nodes[i];
		}
		fprintf(f, "helper (%" PRIu64 " nodes)\n", n_helper_nodes);
		fprintf(f, "\n\n");
	}


	if (statistics.n_PVS_root) {
		fprintf(f, "Search:\n");
		fprintf(f, "PVS_root          = %12" PRIu64 "\n", statistics.n_PVS_root);
		fprintf(f, "PVS+NWS_midgame   = %12" PRIu64 " + %12" PRIu64 "\n", statistics.n_PVS_midgame, statistics.n_NWS_midgame);
		fprintf(f, "PVS+NWS_shallow   = %12" PRIu64 " + %12" PRIu64 "\n", statistics.n_PVS_shallow, statistics.n_NWS_shallow);
		fprintf(f, "search_eval_2     = %12" PRIu64 "\n", statistics.n_search_eval_2);
		fprintf(f, "search_eval_1     = %12" PRIu64 "\n", statistics.n_search_eval_1);
		fprintf(f, "search_eval_0     = %12" PRIu64 "\n\n", statistics.n_search_eval_0);
		fprintf(f, "NWS_endgame       = %12" PRIu64 "\n", statistics.n_NWS_endgame);
		fprintf(f, "NWS_solve_4       = %12" PRIu64 "\n", statistics.n_search_solve_4);
		fprintf(f, "NWS_solve_3       = %12" PRIu64 "\n", statistics.n_solve_3);
		fprintf(f, "NWS_solve_2       = %12" PRIu64 "\n", statistics.n_solve_2);
		fprintf(f, "NWS_solve_1       = %12" PRIu64 "\n", statistics.n_solve_1);
		fprintf(f, "solve_0           = %12" PRIu64 "\n", statistics.n_solve_0);
		fprintf(f, "solve             = %12" PRIu64 "\n\n\n", statistics.n_solve);
	}

	if (statistics.n_hash_found) {
		fprintf(f, "HashTable (all):\n");
		fprintf(f, "Probe: %" PRIu64 "   found: %" PRIu64 " (%6.2f%%)\n", statistics.n_hash_search, statistics.n_hash_found, 100.0 * statistics.n_hash_found / statistics.n_hash_search);
		fprintf(f, "New: %" PRIu64 "   Update: %" PRIu64 "   Ugrade: %" PRIu64 "   Remove: %" PRIu64 "\n",
			statistics.n_hash_new, statistics.n_hash_update, statistics.n_hash_upgrade, statistics.n_hash_remove);
	}

	if (statistics.n_hash_n) {
		fprintf(f, "HashTable collision:\n");
		fprintf(f, "Probes: %" PRIu64 "   Collisions: %" PRIu64 " (%6.2f%%)\n", statistics.n_hash_n, statistics.n_hash_collision, 100.0 * statistics.n_hash_collision / statistics.n_hash_n);
	}
	if (SQUARE_STATS(1) +0) {
		for (j = 0; j < BOARD_SIZE; ++j) {
			fprintf(f, "\n%2d: ", j);
			for (i = 0; i < 9; ++i) {
				if (statistics.n_played_square[j][i])
					fprintf(f, "[%d] = %.1f, ", i, 100.0 * statistics.n_good_square[j][i]/statistics.n_played_square[j][i]);
			}
		}
		fprintf(f, "\n\n");
	}

	if (CUTOFF_STATS(1) +0) {
		if (statistics.n_hash_try) {
			fprintf(f, "Transposition cutoff:\n");
			fprintf(f, "try = %" PRIu64 ", low cutoff = %" PRIu64 " (%6.2f%%), high cutoff = %" PRIu64 " (%6.2f%%)\n",
				statistics.n_hash_try,
				statistics.n_hash_low_cutoff, 100.0 * statistics.n_hash_low_cutoff / statistics.n_hash_try,
				statistics.n_hash_high_cutoff, 100.0 * statistics.n_hash_high_cutoff / statistics.n_hash_try);
		}
		if (statistics.n_stability_try) {
			fprintf(f, "Stability cutoff:\n");
			fprintf(f, "try = %" PRIu64 ", low cutoff = %" PRIu64 " (%6.2f%%)\n",
				statistics.n_stability_try,
				statistics.n_stability_low_cutoff, 100.0 * statistics.n_stability_low_cutoff / statistics.n_stability_try);
		}
		if (statistics.n_etc_try) {
			fprintf(f, "(E)nhance (T)ransposition & (S)tability (C)utoff:\n");
			fprintf(f, "try = %" PRIu64 ", high ETC = %" PRIu64 " (%6.2f%%), high ESC = %" PRIu64 " (%6.2f%%)\n",
				statistics.n_etc_try,
				statistics.n_etc_high_cutoff, 100.0 * statistics.n_etc_high_cutoff / statistics.n_etc_try,
				statistics.n_esc_high_cutoff, 100.0 * statistics.n_esc_high_cutoff / statistics.n_etc_try);
		}
		fprintf(f, "\n\n");
	}

	if (statistics.n_probcut_try) {
		fprintf(f, "Probcut:\n");
		fprintf(f, "\ttry = %" PRIu64 ",\n\tlow cutoff = %" PRIu64 " try (%6.2f%%) %" PRIu64 " success (%6.2f%% (%6.2f%%)),\n\thigh cutoff = %" PRIu64 " try (%6.2f%%) %" PRIu64 " success (%6.2f%% (%6.2f%%))\n",
			statistics.n_probcut_try,
			statistics.n_probcut_low_try, 100.0 * statistics.n_probcut_low_try / statistics.n_probcut_try,
			statistics.n_probcut_low_cutoff, 100.0 * statistics.n_probcut_low_cutoff / statistics.n_probcut_try, 100.0 * statistics.n_probcut_low_cutoff / statistics.n_probcut_low_try,
			statistics.n_probcut_high_try, 100.0 * statistics.n_probcut_high_try / statistics.n_probcut_try,
			statistics.n_probcut_high_cutoff, 100.0 * statistics.n_probcut_high_cutoff / statistics.n_probcut_try, 100.0 * statistics.n_probcut_high_cutoff / statistics.n_probcut_high_try);
	}

	if (statistics.n_NWS_candidate) {
		fprintf(f, "NWS candidate as best root move:\n");
		fprintf(f, "Candidate: %" PRIu64 ", Best Move: %" PRIu64 " (%6.2f%%), Bad Candidate: %" PRIu64 " (%6.2f%%)\n",
			statistics.n_NWS_candidate,
			statistics.n_NWS_candidate - statistics.n_NWS_bad_candidate, (100.0 * (statistics.n_NWS_candidate - statistics.n_NWS_bad_candidate) / statistics.n_NWS_candidate),
			statistics.n_NWS_bad_candidate, (100.0 * statistics.n_NWS_bad_candidate / statistics.n_NWS_candidate)
		);
	}
}

