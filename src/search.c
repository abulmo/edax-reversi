/**
 * @file search.c Search the best move.
 *
 * Functions that evaluate a board with different methods depending on the
 * position in the tree search and/or that finds the best move of a given
 * board.
 *
 * At the end of the game, some trivial functions are used to compute the score.
 * For instance, the last move is not made and only the number of flipped discs
 * is evaluated. Special and optimized functions are used when one, two and three
 * empty squares remain on the board, in order to speed up the search.
 *
 * The search of the best move is driven with the Principal Variation Search
 * algorithm (PVS) [1], an enhanced variation of the alphabeta algorithm. The
 * alphabeta algorithm is known to visit less nodes when the alphabeta window is
 * reduced. PVS takes this property into account. From a set of sibling nodes,
 * the first node is searched using a plain alpha beta window. Then the sibling
 * nodes are only searched with minimal windows (where beta = alpha + 1), just
 * to refute the best (first) score. In rare cases the first move is actually
 * refuted, then the current move is re-searched a second time in order to
 * determinate its score more accurately. On highly ordered tree, very few
 * re-searches will be done. Moreover, thanks to the hash table, a second search
 * is usually faster than a first search. So the seldom and fast re-searches
 * will not impact too much the benefit of using minimal windows. Aspiration
 * windows have been added as another improvement, so that even the first
 * search is done with a reduced window.
 * During the 1990s, several re-re-search algorithms based on null-window
 * alphabeta have been proposed : SSS*ab [2], Dual*ab[2], NegaC*[3], MDT[2],
 * negascout[4]. Some (unpublished) experimental tests I made with them did not
 * show any significant improvement compare to the PVS algorithm with
 * aspiration-windows used here.
 *
 * To be efficient PVS need highly ordered tree. The following ordering has
 * been made :
 *       - fixed square ordering: square usually leading to a good move are
 * visited first, ie from corner squares to X and C squares.
 *       - parity: squares on odd set of empty squares should be played first,
 * especially near the end of the game.
 *       - most stable ordering: a crude evaluation of stability at the corner
 * (corner, X and C squares) to order the moves.
 *       - fast first ordering: the moves leading to the most reduced mobility
 * for the opponent are played first.
 *       - best move previously found: If the position has been previously
 * searched, the best move that was found is replayed as the first move.
 *
 * -# Campbell MS & Marsland TA (1983) A Comparison of Minimax Trees Search
 *    Algorithms. Artificial Intelligence, 20, pp. 347-367.
 * -# Plaat A. et al. (1996) Best-First Fixed Depth Minimax Algorithms. Artificial
 *    Intelligence, 84, pp. 255-293.
 * -# Weill JC. (1992) Experiments With The NegaC* Search. An alternative for Othello
 *    Endgame Search. ICCA journal, 15(1), pp. 3-7.
 * -# Reinsfeld A. (1983) An Improvement Of the Scout Tree-Search Algorithm. ICCA
 *     journal, 6(4), pp. 4-14.
 *
 * @date 1998 - 2017
 * @author Richard Delorme
 * @version 4.4
 */

#include "search.h"

#include "bit.h"
#include "options.h"
#include "stats.h"
#include "util.h"
#include "ybwc.h"
#include "settings.h"

#include <assert.h>
#include <limits.h>
#include <stdlib.h>
#include <math.h>

Log search_log[1];

#ifdef _MSC_VER
#define log2(x) (log(x)/log(2.0))
#endif

/** a quadrant id for each square */
const int QUADRANT_ID[] = {
		1, 1, 1, 1, 2, 2, 2, 2,
		1, 1, 1, 1, 2, 2, 2, 2,
		1, 1, 1, 1, 2, 2, 2, 2,
		1, 1, 1, 1, 2, 2, 2, 2,
		4, 4, 4, 4, 8, 8, 8, 8,
		4, 4, 4, 4, 8, 8, 8, 8,
		4, 4, 4, 4, 8, 8, 8, 8,
		4, 4, 4, 4, 8, 8, 8, 8,
		0, 0
	};

/** level with no selectivity */
const int NO_SELECTIVITY = 5;

/** predefined selectivity */
const Selectivity selectivity_table [] = {
	{1.1, 0, 73}, // strong selectivity
	{1.5, 1, 87}, //       |
	{2.0, 2, 95}, //       |
	{2.6, 3, 98}, //       |
	{3.3, 4, 99}, //       V
	{999, 5,100}, // no selectivity
};

/** threshold values to try stability cutoff during NWS search */
// TODO: better values may exist.
const int NWS_STABILITY_THRESHOLD[] = { // 99 = unused value...
	 99, 99, 99, 99,  6,  8, 10, 12,
	 14, 16, 20, 22, 24, 26, 28, 30,
	 32, 34, 36, 38, 40, 42, 44, 46,
	 48, 48, 50, 50, 52, 52, 54, 54,
	 56, 56, 58, 58, 60, 60, 62, 62,
	 64, 64, 64, 64, 64, 64, 64, 64,
	 99, 99, 99, 99, 99, 99, 99, 99, // no stable square at those depths
};

/** threshold values to try stability cutoff during PVS search */
// TODO: better values may exist.
const int PVS_STABILITY_THRESHOLD[] = { // 99 = unused value...
	 99, 99, 99, 99, -2,  0,  2,  4,
	  6,  8, 12, 14, 16, 18, 20, 22,
	 24, 26, 28, 30, 32, 34, 36, 38,
	 40, 40, 42, 42, 44, 44, 46, 46,
	 48, 48, 50, 50, 52, 52, 54, 54,
	 56, 56, 58, 58, 60, 60, 62, 62,
	 99, 99, 99, 99, 99, 99, 99, 99, // no stable square at those depths
};


/** square type */
const int SQUARE_TYPE[] = {
	0, 1, 2, 3, 3, 2, 1, 0,
	1, 4, 5, 6, 6, 5, 4, 1,
	2, 5, 7, 8, 8, 7, 5, 2,
	3, 6, 8, 9, 9, 8, 6, 3,
	3, 6, 8, 9, 9, 8, 6, 3,
	2, 5, 7, 8, 8, 7, 5, 2,
	1, 4, 5, 6, 6, 5, 4, 1,
	0, 1, 2, 3, 3, 2, 1, 0,
	9, 9,
};

struct Level LEVEL[61][61];

/**
 * Global initialisations.
 *
 * @todo Add more global initialization from here?
 */
void search_global_init(void)
{
	int level, n_empties;

	for (level = 0; level <= 60; ++level)
	for (n_empties = 0; n_empties <= 60; ++n_empties) {
		if (level <= 0) {
			LEVEL[level][n_empties].depth = 0;
			LEVEL[level][n_empties].selectivity = 5;
		} else if (level <= 10) {
			LEVEL[level][n_empties].selectivity = 5;
			if (n_empties <= 2 * level) {
				LEVEL[level][n_empties].depth = n_empties;
			} else {
				LEVEL[level][n_empties].depth = level;
			}
		} else if (level <= 12) {
			if (n_empties <= 21) {
				LEVEL[level][n_empties].depth = n_empties;
				LEVEL[level][n_empties].selectivity = 5;
			} else if (n_empties <= 24) {
				LEVEL[level][n_empties].depth = n_empties;
				LEVEL[level][n_empties].selectivity = 3;
			} else {
				LEVEL[level][n_empties].depth = level;
				LEVEL[level][n_empties].selectivity = 0;
			}
		} else if (level <= 18) {
			if (n_empties <= 21) {
				LEVEL[level][n_empties].depth = n_empties;
				LEVEL[level][n_empties].selectivity = 5;
			} else if (n_empties <= 24) {
				LEVEL[level][n_empties].depth = n_empties;
				LEVEL[level][n_empties].selectivity = 3;
			} else if (n_empties <= 27) {
				LEVEL[level][n_empties].depth = n_empties;
				LEVEL[level][n_empties].selectivity = 1;
			} else {
				LEVEL[level][n_empties].depth = level;
				LEVEL[level][n_empties].selectivity = 0;
			}
		} else if (level <= 21) {
			if (n_empties <= 24) {
				LEVEL[level][n_empties].depth = n_empties;
				LEVEL[level][n_empties].selectivity = 5;
			} else if (n_empties <= 27) {
				LEVEL[level][n_empties].depth = n_empties;
				LEVEL[level][n_empties].selectivity = 3;
			} else if (n_empties <= 30) {
				LEVEL[level][n_empties].depth = n_empties;
				LEVEL[level][n_empties].selectivity = 1;
			} else {
				LEVEL[level][n_empties].depth = level;
				LEVEL[level][n_empties].selectivity = 0;
			}
		} else if (level <= 24) {
			if (n_empties <= 24) {
				LEVEL[level][n_empties].depth = n_empties;
				LEVEL[level][n_empties].selectivity = 5;
			} else if (n_empties <= 27) {
				LEVEL[level][n_empties].depth = n_empties;
				LEVEL[level][n_empties].selectivity = 4;
			} else if (n_empties <= 30) {
				LEVEL[level][n_empties].depth = n_empties;
				LEVEL[level][n_empties].selectivity = 2;
			} else if (n_empties <= 33) {
				LEVEL[level][n_empties].depth = n_empties;
				LEVEL[level][n_empties].selectivity = 0;
			} else {
				LEVEL[level][n_empties].depth = level;
				LEVEL[level][n_empties].selectivity = 0;
			}
		} else if (level <= 27) {
			if (n_empties <= 27) {
				LEVEL[level][n_empties].depth = n_empties;
				LEVEL[level][n_empties].selectivity = 5;
			} else if (n_empties <= 30) {
				LEVEL[level][n_empties].depth = n_empties;
				LEVEL[level][n_empties].selectivity = 3;
			} else if (n_empties <= 33) {
				LEVEL[level][n_empties].depth = n_empties;
				LEVEL[level][n_empties].selectivity = 1;
			} else {
				LEVEL[level][n_empties].depth = level;
				LEVEL[level][n_empties].selectivity = 0;
			}
		} else if (level < 30) {
			if (n_empties <= 27) {
				LEVEL[level][n_empties].depth = n_empties;
				LEVEL[level][n_empties].selectivity = 5;
			} else if (n_empties <= 30) {
				LEVEL[level][n_empties].depth = n_empties;
				LEVEL[level][n_empties].selectivity = 4;
			} else if (n_empties <= 33) {
				LEVEL[level][n_empties].depth = n_empties;
				LEVEL[level][n_empties].selectivity = 2;
			} else if (n_empties <= 36) {
				LEVEL[level][n_empties].depth = n_empties;
				LEVEL[level][n_empties].selectivity = 0;
			} else {
				LEVEL[level][n_empties].depth = level;
				LEVEL[level][n_empties].selectivity = 0;
			}
		} else if (level <= 31) {
			if (n_empties <= 30) {
				LEVEL[level][n_empties].depth = n_empties;
				LEVEL[level][n_empties].selectivity = 5;
			} else if (n_empties <= 33) {
				LEVEL[level][n_empties].depth = n_empties;
				LEVEL[level][n_empties].selectivity = 3;
			} else if (n_empties <= 36) {
				LEVEL[level][n_empties].depth = n_empties;
				LEVEL[level][n_empties].selectivity = 1;
			} else {
				LEVEL[level][n_empties].depth = level;
				LEVEL[level][n_empties].selectivity = 0;
			}
		} else if (level <= 33) {
			if (n_empties <= 30) {
				LEVEL[level][n_empties].depth = n_empties;
				LEVEL[level][n_empties].selectivity = 5;
			} else if (n_empties <= 33) {
				LEVEL[level][n_empties].depth = n_empties;
				LEVEL[level][n_empties].selectivity = 4;
			} else if (n_empties <= 36) {
				LEVEL[level][n_empties].depth = n_empties;
				LEVEL[level][n_empties].selectivity = 2;
			} else if (n_empties <= 39) {
				LEVEL[level][n_empties].depth = n_empties;
				LEVEL[level][n_empties].selectivity = 0;
			} else {
				LEVEL[level][n_empties].depth = level;
				LEVEL[level][n_empties].selectivity = 0;
			}
		} else if (level <= 35) {
			if (n_empties <= 30) {
				LEVEL[level][n_empties].depth = n_empties;
				LEVEL[level][n_empties].selectivity = 5;
			} else if (n_empties <= 33) {
				LEVEL[level][n_empties].depth = n_empties;
				LEVEL[level][n_empties].selectivity = 4;
			} else if (n_empties <= 36) {
				LEVEL[level][n_empties].depth = n_empties;
				LEVEL[level][n_empties].selectivity = 3;
			} else if (n_empties <= 39) {
				LEVEL[level][n_empties].depth = n_empties;
				LEVEL[level][n_empties].selectivity = 1;
			} else {
				LEVEL[level][n_empties].depth = level;
				LEVEL[level][n_empties].selectivity = 0;
			}
		} else if (level < 60) {
			if (n_empties <= level - 6) {
				LEVEL[level][n_empties].depth = n_empties;
				LEVEL[level][n_empties].selectivity = 5;
			} else if (n_empties <= level - 3) {
				LEVEL[level][n_empties].depth = n_empties;
				LEVEL[level][n_empties].selectivity = 4;
			} else if (n_empties <= level) {
				LEVEL[level][n_empties].depth = n_empties;
				LEVEL[level][n_empties].selectivity = 3;
			} else if (n_empties <= level + 3) {
				LEVEL[level][n_empties].depth = n_empties;
				LEVEL[level][n_empties].selectivity = 2;
			} else if (n_empties <= level + 6) {
				LEVEL[level][n_empties].depth = n_empties;
				LEVEL[level][n_empties].selectivity = 1;
			} else if (n_empties <= level + 9) {
				LEVEL[level][n_empties].depth = n_empties;
				LEVEL[level][n_empties].selectivity = 0;
			} else {
				LEVEL[level][n_empties].depth = level;
				LEVEL[level][n_empties].selectivity = 0;
			}
		} else {
			LEVEL[level][n_empties].depth = n_empties;
			LEVEL[level][n_empties].selectivity = 5;
		}
	}
	search_log->f = NULL;
}

void search_resize_hashtable(Search *search) {
	if (search->options.hash_size != options.hash_table_size) {
		const int hash_size = 1u << options.hash_table_size;
		const int pv_size = hash_size > 16 ? hash_size >> 4 : 16;

		hash_init(search->hash_table, hash_size);
		hash_init(search->pv_table, pv_size);
		hash_init(search->shallow_table, hash_size);
		search->options.hash_size = options.hash_table_size;
	}
}

/**
 * @brief Init the *main* search.
 *
 * Initialize a new search structure.
 * @param search  search.
 */
void search_init(Search *search)
{
	/* id */
	search->id = 0;

	/* running state */
	search->stop = STOP_END;

	/* hash_table */
	search->options.hash_size = 0;
	search->hash_table->hash = NULL;
	search->hash_table->hash_mask = 0;
	search->pv_table->hash = NULL;
	search->pv_table->hash_mask = 0;
	search->shallow_table->hash = NULL;
	search->shallow_table->hash_mask = 0;
	search_resize_hashtable(search);

	/* board */
	search->board->player = search->board->opponent = 0;
	search->player = EMPTY;

	/* evaluation function */
	eval_init(search->eval);

	// radom generator
	random_seed(search->random, real_clock());

	/* task stack */
	search->tasks = (TaskStack*) malloc(sizeof (TaskStack));
	if (search->tasks == NULL) {
		fatal_error("Cannot allocate a task stack\n");
	}
	if (options.cpu_affinity) thread_set_cpu(thread_self(), 0);
	task_stack_init(search->tasks, options.n_task);
	search->allow_node_splitting = (search->tasks->n > 1);

	/* task associated with the current search */
	search->task = search->tasks->task;
	search->task->loop = false;
	search->task->run = true;
	search->task->node = NULL;
	search->task->move = NULL;
	search->task->n_calls = 0;
	search->task->n_nodes = 0;
	search->task->search = search;

	search->parent = NULL;
	search->n_child = 0;
	search->master = search; /* main search */

	/* lock */
	spin_init(search);

	/* result */
	search->result = (Result*) malloc(sizeof (Result));
	if (search->result == NULL) {
		fatal_error("Cannot allocate a task stack\n");
	}
	spin_init(search->result);
	search->result->move = NOMOVE;

	search->n_nodes = 0;
	search->child_nodes = 0;


	/* observers */
	search->observer = search_observer;

	/* options */
	search->options.depth = 60;
	search->options.selectivity = NO_SELECTIVITY;
	search->options.time = TIME_MAX;
	search->options.time_per_move = false;
	search->options.verbosity = options.verbosity;
	search->options.keep_date = false;
	search->options.header = NULL;
	search->options.separator = NULL;
	search->options.guess_pv = options.pv_guess;
	search->options.multipv_depth = MULTIPV_DEPTH;

	log_open(search_log, options.search_log_file);
}

/**
 * @brief Free the search allocated ressource.
 *
 * Free a previously initialized search structure.
 * @param search search.
 */
void search_free(Search *search)
{

	hash_free(search->hash_table);
	hash_free(search->pv_table);
	hash_free(search->shallow_table);
	eval_free(search->eval);
	
	task_stack_free(search->tasks);
	free(search->tasks);
	spin_free(search);

	spin_free(search->result);
	free(search->result);

	log_close(search_log);
}

/**
 * @brief Set up various structure once the board has been set.
 *
 * Initialize the list of empty squares, the parity and the evaluation function.

 * @param search search.
 */
void search_setup(Search *search)
{
	int i;
	SquareList *empty;
	const int presorted_x[] = {
		A1, A8, H1, H8,                    /* Corner */
		C4, C5, D3, D6, E3, E6, F4, F5,    /* E */
		C3, C6, F3, F6,                    /* D */
		A3, A6, C1, C8, F1, F8, H3, H6,    /* A */
		A4, A5, D1, D8, E1, E8, H4, H5,    /* B */
		B4, B5, D2, D7, E2, E7, G4, G5,    /* G */
		B3, B6, C2, C7, F2, F7, G3, G6,    /* F */
		A2, A7, B1, B8, G1, G8, H2, H7,    /* C */
		B2, B7, G2, G7,                    /* X */
		D4, E4, D5, E5,                    /* center */
	};

	Board *board = search->board;
	unsigned long long E;

	// init empties
	search->n_empties = 0;

	empty = search->empties;
	empty->x = NOMOVE; /* sentinel */
	empty->previous = NULL;
	empty->next = empty + 1;
	empty = empty->next;
	E = ~(board->player | board->opponent);
	for (i = 0; i < BOARD_SIZE; ++i) {    /* add empty squares */
		if ((E & x_to_bit(presorted_x[i]))) {
			empty->x = presorted_x[i];
			empty->b = x_to_bit(presorted_x[i]);
			empty->quadrant = QUADRANT_ID[empty->x];
			empty->previous = empty - 1;
			empty->next = empty + 1;
			search->x_to_empties[presorted_x[i]] = empty;
			empty = empty->next;
			++search->n_empties;
		}
	}
	empty->x = NOMOVE; /* sentinel */
	empty->b = 0;
	empty->previous = empty - 1;
	empty->next = NULL;

	empty = search->empties + PASS;
	empty->x = PASS;
	empty->b = 0;
	empty->previous = empty->next = empty;
	search->x_to_empties[PASS] = empty;

	empty = search->empties + NOMOVE;
	empty->x = NOMOVE;
	empty->b = 0;
	empty->previous = empty->next = empty;
	search->x_to_empties[NOMOVE] = empty;

	// init parity
	search->parity = 0;
	foreach_empty (empty, search->empties) {
		search->parity ^= empty->quadrant;
	}

	// init the evaluation function
	eval_set(search->eval, board);
}

/**
 * @brief Clone a search for parallel search.
 *
 * @param search search.
 * @param master search to be cloned.
 */
void search_clone(Search *search, Search *master)
{
	search->stop = STOP_END;
	search->player = master->player;
	*search->board = *master->board;
	search_setup(search);
	*search->hash_table = *master->hash_table; // share the hashtable
	*search->pv_table = *master->pv_table; // share the pvtable
	*search->shallow_table = *master->shallow_table; // share the pvtable
	search->tasks = master->tasks;
	search->observer = master->observer;

	search->depth = master->depth;
	search->selectivity = master->selectivity;
	search->probcut_level = master->probcut_level;
	search->depth_pv_extension = master->depth_pv_extension;
	search->time = master->time;
	search->height = master->height;
	search->allow_node_splitting = master->allow_node_splitting;
	search->node_type[search->height] = master->node_type[search->height];
	search->options = master->options;
	search->result = master->result;
	search->n_nodes = 0;
	search->child_nodes = 0;
	search->stability_bound = master->stability_bound;
	spin_lock(master);
	assert(master->n_child < MAX_THREADS);
	master->child[master->n_child++] = search;
	spin_unlock(master);
	search->parent = master;
	search->master = master->master;
}

/**
 * @brief Clean-up some search data.
 *
 * @param search search.
 */
void search_cleanup(Search *search)
{
	hash_cleanup(search->hash_table);
	hash_cleanup(search->pv_table);
	hash_cleanup(search->shallow_table);
}


/**
 * @brief Set the board to analyze.
 *
 * @param search search.
 * @param board board.
 * @param player player's turn.
 */
void search_set_board(Search *search, const Board *board, const int player)
{
	search->player = player;
	*search->board = *board;
	search_setup(search);
	search_get_movelist(search, search->movelist);
}

/**
 * @brief Set the search level.
 *
 * Compute the couple (depth, selectivity) as a function of (level, n_empties)
 * @param search search.
 * @param level  search level.
 * @param n_empties Search stage.
 */
void search_set_level(Search *search, const int level, const int n_empties)
{
	search->options.depth = LEVEL[level][n_empties].depth;
	search->options.selectivity = LEVEL[level][n_empties].selectivity;

	// post-condition:
	assert(0 <= search->options.depth && search->options.depth <= 60);
	assert(0 <= search->options.selectivity && search->options.selectivity <= 5);
	info("<set level (game level=%d, empties=%d) => position level=%d@%d>\n", level, n_empties, search->options.depth, selectivity_table[search->options.selectivity].percent);
}

/**
 * @brief Set the search level while pondering.
 *
 * Compute the couple (depth, selectivity) as a function of (level, n_empties)
 *
 * @param search Search.
 * @param level  Search level.
 * @param n_empties Search stage.
 */
void search_set_ponder_level(Search *search, const int level, const int n_empties)
{
	search->options.depth = LEVEL[level][n_empties - 1].depth + 1;
	if (search->options.depth > n_empties) search->options.depth = n_empties;
	search->options.selectivity = LEVEL[level][n_empties - 1].selectivity;

	assert(0 <= search->options.depth && search->options.depth <= 60);
	assert(0 <= search->options.selectivity && search->options.selectivity <= 5);
}

/**
 * @brief Compute the deepest level that can be solved given a limited time...
 *
 * This is a very approximate computation... 
 * SMP_W & SMP_C depends on the depth and the position.
 * The branching factor depends also of the position. 
 *
 * @param limit Time limit in ms.
 * @param n_tasks Number of parallel tasks.
 * @return Reachable depth.
 */
int solvable_depth(const long long limit, int n_tasks)
{
	int d;
	long long t;
	double speed = 0.001 * (options.speed * (SMP_W + SMP_C) / (SMP_W / n_tasks + SMP_C));

	for (t = 0.0, d = 15; d <= 60 && t <= limit; ++d) {
		t += pow(BRANCHING_FACTOR, d) / speed;
	}
	return d - 1;
}

/**
 * @brief set time to search.
 *
 * This function should be called before each new search, with the accurate
 * remaining time to do the whole search for the game.
 *
 * @param search Search.
 * @param t time in ms.
 */
void search_set_game_time(Search *search, const long long t)
{
	search->options.time_per_move = false;
	search->options.time = t;
}

/**
 * @brief set time to search.
 *
 * This function should be called before each new search, with the accurate
 * remaining time to do the whole search for this move.
 *
 * @param search Search.
 * @param t time in ms.
 */
void search_set_move_time(Search *search, const long long t)
{
	search->options.time_per_move = true;
	search->options.time = t;
}

/**
 * @brief Initialize the alloted time.
 *
 * @param search Search.
 */
void search_time_init(Search *search)
{
	if (search->options.time_per_move) {
		const long long t = MAX(search->options.time - 10, 100);
		search->time.extra = t;
		search->time.maxi = t * 99 / 100;
		search->time.mini = t * 9 / 10;
		if (search->options.verbosity >= 2) {
			info("<Time-alloted: mini = %.2f; maxi = %.2f; extra = %.2f>\n", 0.001 * search->time.mini,  0.001 * search->time.maxi,  0.001 * search->time.extra);
		}
	} else {
		long long t = search->options.time;
		const int sd = solvable_depth(t / 10, search_count_tasks(search)); // depth solvable with 10% of the time
		const int d = MAX((search->n_empties - sd) / 2, 2); // unsolvable ply to play
		t = MAX(t / d - 10, 100); // keep 0.25 s./remaining move, make at least 1s. available
		search->time.extra = t;
		search->time.maxi = t * 3 / 4;
		search->time.mini = t / 4;
		if (search->options.verbosity >= 2) {
			info("<Time-init: rt = %.2f; sd = %d; d = %d; t = %.2f>\n", 0.001 * search->options.time, sd, d, 0.001 * t);
			info("<Time-alloted: mini = %.2f; maxi = %.2f; extra = %.2f>\n", 0.001 * search->time.mini,  0.001 * search->time.maxi,  0.001 * search->time.extra);
		}
	}
	search->time.extended = false;
	search->time.can_update = true;
}

/**
 * @brief Reset the alloted time.
 *
 * Adjust the time when continuing the search from pondering.
 *
 * @param search Search.
 * @param initial_board Initial board.
 */
void search_time_reset(Search *search, const Board *initial_board)
{
	const long long spent = search_time(search);
	const int n_empties = board_count_empties(initial_board);

	if (search->options.time_per_move) {
		const long long t = MAX(search->options.time - 10, 100);
		search->time.extra = spent + t;
		search->time.maxi = spent + t * 99 / 100;
		search->time.mini = spent + t * 9 / 10;
		if (search->options.verbosity >= 2) {
			info("<Time-alloted: mini = %.2f; maxi = %.2f; extra = %.2f>\n", 0.001 * search->time.mini,  0.001 * search->time.maxi,  0.001 * search->time.extra);
		}
	} else {
		long long t = search->options.time;
		const int sd = solvable_depth(t / 10, search_count_tasks(search)); // depth solvable with 10% of the time
		const int d = MAX((n_empties - sd) / 2, 2); // unsolvable ply to play
		t = MAX(t / d - 10, 100); // keep 0.25 s./remaining move, make at least 0.1 s available
		search->time.extra = spent + t;
		search->time.maxi = spent + t * 3 / 4;
		search->time.mini = spent + t / 4;
		if (search->options.verbosity >= 2) {
			info("<Time-reset: spent = %.2f rt = %.2f; sd = %d; d = %d; t = %.2f>\n", 0.001 * spent, 0.001 * search->options.time, sd, d, 0.001 * t);
			info("<Time-alloted: mini = %.2f; maxi = %.2f; extra = %.2f>\n", 0.001 * search->time.mini,  0.001 * search->time.maxi,  0.001 * search->time.extra);
		}
	}
	search->time.extended = false;
	search->time.can_update = true;
}




/**
 * @brief Give more time.
 *
 * @param search Search.
 * @param once A flag to prevent further time extension.
 */
void search_adjust_time(Search *search, const bool once)
{
	if (!search->options.time_per_move) {
		const long long t = MAX(search->time.extra, MAX(search->options.time - search_time(search) - 10, 100) / 2);
		search->time.mini = MIN(search->time.maxi, t);
		search->time.maxi = MIN(search->time.mini * 4 / 3, t);
		search->time.extra = MIN(search->time.maxi * 4 / 3, t);
		search->time.extended = once;
		if (search->options.verbosity >= 2) {
			info("\n<Time-adjusted: mini = %.2f; maxi = %.2f; extra = %.2f>\n", 0.001 * search->time.mini,  0.001 * search->time.maxi,  0.001 * search->time.extra);
		}
	}
}

/**
 * @brief Check if it can iterate more...
 *
 * @param search Search.
 */
bool search_continue(Search *search)
{
	return search->stop == RUNNING && search_time(search) <= search->time.mini;
}

/**
 * @brief Check if it can iterate more...
 *
 * @param search Search.
 */
void search_check_timeout(Search *search)
{
	long long t;
	Search *master = search->master;

	assert(master->master == master);

	if (master->stop != STOP_TIMEOUT) {

		t = search_time(master);

		if (t > master->time.extra) {

			if (master->stop != RUNNING) {
				printf("*** MASTER ALREADY STOPPED FOR ANOTHER REASON (%d) ***\n", master->stop);
			}

			if (!master->time.extended && master->time.can_update) {
				Result *result = master->result;
				spin_lock(result);
				if ((!master->time.extended && master->time.can_update)
				&& (result->bound[result->move].lower < result->score || result->depth == 0)) {
					search_adjust_time(master, true);
				}
				spin_unlock(result);
			}
			if (t > search->time.extra) {
				search_stop_all(master, STOP_TIMEOUT);
			}
		}
	}	

	if (search->stop != STOP_TIMEOUT) {
		t = search_time(master);
		if (t > master->time.extra) {
			printf("*** SEARCH STILL RUNNING ? (%d) ***\n", search->stop);
			search->stop = STOP_TIMEOUT;
		}
	}
}

/**
 * @brief Change the number of task.
 *
 * @param search Search.
 * @param n New task number.
 */
void search_set_task_number(Search *search, const int n)
{
	assert(n > 0 && n < MAX_THREADS);
	task_stack_resize(search->tasks, n);
	search->allow_node_splitting = (n > 1);
}

/**
 * @brief Change parity.
 *
 * @param search Search.
 * @param x      Played square.
 */
void search_swap_parity(Search *search, const int x)
{
	search->parity ^= QUADRANT_ID[x];
}

/**
 * @brief Get a list of legal moves.
 *
 * Compute the complete list of legal moves and store it into a simple linked
 * list, to fasten ulterior move sorting.
 * Note: at this point the list is sorted from A1 to H8 (i.e. unsorted).
 *
 * @param search Search.
 * @param movelist List of moves.
 */
void search_get_movelist(const Search *search, MoveList *movelist)
{
	Move *previous = movelist->move;
	Move *move = movelist->move + 1;
	const Board *board = search->board;
	unsigned long long moves = get_moves(board->player, board->opponent);
	register int x;

	foreach_bit(x, moves) {
		board_get_move(board, x, move);
		move->cost = 0;
		previous = previous->next = move;
		++move;
	}
	previous->next = NULL;
	movelist->n_moves = move - movelist->move - 1;
	assert(movelist->n_moves == get_mobility(board->player, board->opponent));
}

/**
 * @brief Update the search state after a move.
 *
 * @param search  search.
 * @param move    played move.
 */
void search_update_endgame(Search *search, const Move *move)
{
	search_swap_parity(search, move->x);
	empty_remove(search->x_to_empties[move->x]);
	board_update(search->board, move);
	--search->n_empties;

}

/**
 * @brief Restore the search state as before a move.
 *
 * @param search  search.
 * @param move    played move.
 */
void search_restore_endgame(Search *search, const Move *move)
{
	search_swap_parity(search, move->x);
	empty_restore(search->x_to_empties[move->x]);
	board_restore(search->board, move);
	++search->n_empties;
}

/**
 * @brief Update the search state after a passing move.
 *
 * @param search  search.
 */
void search_pass_endgame(Search *search)
{
	board_pass(search->board);
}


//static Line debug_line;

/**
 * @brief Update the search state after a move.
 *
 * @param search  search.
 * @param move    played move.
 */
void search_update_midgame(Search *search, const Move *move)
{
	static const NodeType next_node_type[] = {CUT_NODE, ALL_NODE, CUT_NODE};

//	line_push(&debug_line, move->x);

	search_swap_parity(search, move->x);
	empty_remove(search->x_to_empties[move->x]);
	board_update(search->board, move);
	eval_update(search->eval, move);
	assert(search->n_empties > 0);
	--search->n_empties;
	++search->height;
	search->node_type[search->height] = next_node_type[search->node_type[search->height- 1]];
}

/**
 * @brief Restore the search state as before a move.
 *
 * @param search  search.
 * @param move    played move.
 */
void search_restore_midgame(Search *search, const Move *move)
{
//	line_print(&debug_line, 100, " ", stdout); putchar('\n');
//	line_pop(&debug_line);

	search_swap_parity(search, move->x);
	empty_restore(search->x_to_empties[move->x]);
	board_restore(search->board, move);
	eval_restore(search->eval, move);
	++search->n_empties;
	assert(search->height > 0);
	--search->height;
}

/**
 * @brief Update the search state after a passing move.
 *
 * @param search  search.
 */
void search_update_pass_midgame(Search *search)
{
	static const NodeType next_node_type[] = {CUT_NODE, ALL_NODE, CUT_NODE};

	board_pass(search->board);
	eval_pass(search->eval);
	++search->height;
	search->node_type[search->height] = next_node_type[search->node_type[search->height- 1]];
}

/**
 * @brief Update the search state after a passing move.
 *
 * @param search  search.
 */
void search_restore_pass_midgame(Search *search)
{
	board_pass(search->board);
	eval_pass(search->eval);
	assert(search->height > 0);
	--search->height;
}

/**
 * @brief Compute the pv_extension.
 *
 * @param depth Depth.
 * @param n_empties Game stage (number of empty squares).
 */
int get_pv_extension(const int depth, const int n_empties)
{
	int depth_pv_extension;

	if (depth >= n_empties || depth <= 9) depth_pv_extension = -1;
	else if (depth <= 12) depth_pv_extension = 10; // depth + 8
	else if (depth <= 18) depth_pv_extension = 12; // depth + 10
	else if (depth <= 24) depth_pv_extension = 14; // depth + 12
	else depth_pv_extension = 16; // depth + 14

	return depth_pv_extension;
}

/**
 * @brief Check if final score use pv_extension or is solved.
 *
 * @param depth search depth.
 * @param n_empties position depth.
 * @return true if the score should be a final score.
 */
bool is_depth_solving(const int depth, const int n_empties)
{
	return (depth >= n_empties)
	    || (depth >  9 && depth <= 12 && depth +  8 >= n_empties)
	    || (depth > 12 && depth <= 18 && depth + 10 >= n_empties)
	    || (depth > 18 && depth <= 24 && depth + 12 >= n_empties)
	    || (depth > 24 && depth + 14 >= n_empties);
}



/**
 * @brief Return the time spent by the search
 *
 * @param search  Search.
 * @return time (in milliseconds).
 */
long long search_clock(Search *search)
{
	if (options.nps > 0) return search_count_nodes(search) / options.nps;
	else return time_clock();
}

/**
 * @brief Return the time spent by the search
 *
 * @param search  Search.
 * @return time (in milliseconds).
 */
long long search_time(Search *search)
{
	if (search->stop != STOP_END) return search_clock(search) + search->time.spent;
	else return search->time.spent;
}

/**
 * @brief Return the number of nodes searched.
 *
 * @param search  Search.
 * @return node count.
 */
unsigned long long search_count_nodes(Search *search)
{
	return search->n_nodes + search->child_nodes;
}

/**
 * @brief default observer.
 *
 * @param result search results to print.
 */
void search_observer(Result *result)
{
	result_print(result, stdout);
	putchar('\n');
}

/**
 * @brief set observer.
 *
 * @param search Searched position.
 * @param observer call back function to print the current search.
 */
void search_set_observer(Search *search, void (*observer)(Result*))
{
	search->observer = observer;
}

/**
 * @brief Print the current search result.
 *
 * @param result Last search result.
 * @param f output stream.
 */
void result_print(Result *result, FILE *f)
{
	char bound;
#ifdef _WIN32
	const int PRINTED_WIDTH = 53;
#else
	const int PRINTED_WIDTH = 52;
#endif

	spin_lock(result);

	if (result->bound[result->move].lower < result->score && result->score == result->bound[result->move].upper) bound = '<';
	else if (result->bound[result->move].lower == result->score && result->score < result->bound[result->move].upper) bound = '>';
	else if (result->bound[result->move].lower == result->score && result->score == result->bound[result->move].upper) bound = ' ';
	else bound = '?';

	if (result->selectivity < 5) fprintf(f, "%2d@%2d%% ", result->depth, selectivity_table[result->selectivity].percent);
	else fprintf(f, "   %2d  ", result->depth);
	fprintf(f, "%c%+03d ", bound, result->score);
	time_print(result->time, true, f);
	if (result->n_nodes) {
		fprintf(f, " %13lld ", result->n_nodes);
		if (result->time > 0) fprintf(f, "%10.0f ", 1000.0 * result->n_nodes / result->time);
		else fprintf(f, "           ");
	} else fputs("                          ", f);
	line_print(result->pv, options.width - PRINTED_WIDTH, " ", f);
	fflush(f);

	spin_unlock(result);
}

/**
 * @brief Stability Cutoff (SC).
 *
 * @param search Current position.
 * @param alpha Alpha bound.
 * @param beta Beta bound, to adjust if necessary.
 * @param score Score to return in case of a cutoff is found.
 * @return 'true' if a cutoff is found, false otherwise.
 */
bool search_SC_PVS(Search *search, volatile int *alpha, volatile int *beta, int *score)
{
	const Board *board = search->board;

	if (USE_SC && *beta >= PVS_STABILITY_THRESHOLD[search->n_empties]) {
		CUTOFF_STATS(++statistics.n_stability_try;)
		*score = SCORE_MAX - 2 * get_stability(board->opponent, board->player);
		if (*score <= *alpha) {
			CUTOFF_STATS(++statistics.n_stability_low_cutoff;)
			return true;
		}
		else if (*score < *beta) *beta = *score;
	}
	return false;
}

/**
 * @brief Stability Cutoff (TC).
 *
 * @param search Current position.
 * @param alpha Alpha bound.
 * @param score Score to return in case of a cutoff is found.
 * @return 'true' if a cutoff is found, false otherwise.
 */
bool search_SC_NWS(Search *search, const int alpha, int *score)
{
	const Board *board = search->board;

	if (USE_SC && alpha >= NWS_STABILITY_THRESHOLD[search->n_empties]) {
		CUTOFF_STATS(++statistics.n_stability_try;)
		*score = SCORE_MAX - 2 * get_stability(board->opponent, board->player);
		if (*score <= alpha) {
			CUTOFF_STATS(++statistics.n_stability_low_cutoff;)
			return true;
		}
	}
	return false;
}

/**
 * @brief Transposition Cutoff (TC).
 *
 * @param data Transposition data.
 * @param depth Search depth.
 * @param selectivity Search selecticity level.
 * @param alpha Alpha bound, to adjust of necessary.
 * @param beta Beta bound, to adjust of necessary.
 * @param score Score to return in case of a cutoff is found.
 * @return 'true' if a cutoff is found, false otherwise.
 */
bool search_TC_PVS(HashData *data, const int depth, const int selectivity, volatile int *alpha, volatile int *beta, int *score)
{
	if (USE_TC && (data->selectivity >= selectivity && data->depth >= depth)) {
		CUTOFF_STATS(++statistics.n_hash_try;)
		if (*alpha < data->lower) {
			*alpha = data->lower;
			if (*alpha >= *beta) {
				CUTOFF_STATS(++statistics.n_hash_high_cutoff;)
				*score = *alpha;
				return true;
			}
		}
		if (*beta > data->upper) {
			*beta = data->upper;
			if (*beta <= *alpha) {
				CUTOFF_STATS(++statistics.n_hash_low_cutoff;)
				*score = *beta;
				return true;
			}
		}
	}
	return false;
}

/**
 * @brief Transposition Cutoff (TC).
 *
 * @param data Transposition data.
 * @param depth Search depth.
 * @param selectivity Search selecticity level.
 * @param alpha Alpha bound.
 * @param score Score to return in case of a cutoff is found.
 * @return 'true' if a cutoff is found, false otherwise.
 */
bool search_TC_NWS(HashData *data, const int depth, const int selectivity, const int alpha, int *score)
{
	if (USE_TC && (data->selectivity >= selectivity && data->depth >= depth)) {
		CUTOFF_STATS(++statistics.n_hash_try;)
		if (alpha < data->lower) {
			CUTOFF_STATS(++statistics.n_hash_high_cutoff;)
			*score = data->lower;
			return true;
		}
		if (alpha >= data->upper) {
			CUTOFF_STATS(++statistics.n_hash_low_cutoff;)
			*score = data->upper;
			return true;
		}
	}
	return false;
}

/**
 * @brief Enhanced Transposition Cutoff (ETC).
 *
 * Before looping over each moves to search at next ply, ETC looks if a cutoff
 * is available from the hashtable. This version also looks for a cutoff based
 * on stability.
 *
 * @param search Current position.
 * @param movelist List of moves for the current position.
 * @param hash_code Hashing code.
 * @param depth Search depth.
 * @param selectivity Search selectivity.
 * @param alpha Alpha bound.
 * @param score Score to return in case a cutoff is found.
 * @return 'true' if a cutoff is found, 'false' otherwise.
 */
bool search_ETC_NWS(Search *search, MoveList *movelist, unsigned long long hash_code, const int depth, const int selectivity, const int alpha, int *score)
{
	if (USE_ETC && depth > ETC_MIN_DEPTH) {

		Move *move;
		Board next[1];
		HashData etc[1];
		unsigned long long etc_hash_code;
		HashTable *hash_table = search->hash_table;
		const int etc_depth = depth - 1;
		const int beta = alpha + 1;
	
		CUTOFF_STATS(++statistics.n_etc_try;)
		foreach_move (move, movelist) {
			next->opponent = search->board->player ^ (move->flipped | x_to_bit(move->x));
			next->player = search->board->opponent ^ move->flipped;
			SEARCH_UPDATE_ALL_NODES();

			if (USE_SC && alpha <= -NWS_STABILITY_THRESHOLD[search->n_empties]) {
				*score = 2 * get_stability(next->opponent, next->player) - SCORE_MAX;
				if (*score > alpha) {
					hash_store(hash_table, search->board, hash_code, depth, selectivity, 0, alpha, beta, *score, move->x);
					CUTOFF_STATS(++statistics.n_esc_high_cutoff;)
					return true;
				}
			}

			etc_hash_code = board_get_hash_code(next);
			if (USE_TC && hash_get(hash_table, next, etc_hash_code, etc) && etc->selectivity >= selectivity && etc->depth >= etc_depth) {
				*score = -etc->upper;
				if (*score > alpha) {
					hash_store(hash_table, search->board, hash_code, depth, selectivity, 0, alpha, beta, *score, move->x);
					CUTOFF_STATS(++statistics.n_etc_high_cutoff;)
					return true;
				}
			}
		}
	}

	return false;
}

/**
 * @brief Share search information
 *
 * @param src source search.
 * @param dest destination search.
 */
void search_share(const Search *src, Search *dest)
{
	hash_copy(src->pv_table, dest->pv_table);
	hash_copy(src->hash_table, dest->hash_table);
}

/**
 * @brief Count the number of tasks used in parallel search.
 *
 * @param search Search.
 * @return number of tasks.
 */
int search_count_tasks(const Search *search)
{
	return search->tasks->n;
}

/**
 * @brief Stop the search.
 *
 * @param search Search.
 * @param stop Source of stopping.
 */
void search_stop_all(Search *search, const Stop stop)
{
	int i;

	spin_lock(search);
		search->stop = stop;
		for (i = 0; i < search->n_child; ++i) {
			search_stop_all(search->child[i], stop);
		}
	spin_unlock(search);
}

/**
 * @brief Set the search running/waiting state.
 *
 * @param search Search.
 * @param stop State.
 */
void search_set_state(Search *search, const Stop stop)
{
	spin_lock(search);
	search->stop = stop;
	spin_unlock(search);
}

/**
 * @brief Guess the bestmove of a given board.
 *
 * Look for a move from the hash tables.
 *
 * @param search Search.
 * @param board Board.
 * @return the best move.
 */
int search_guess(Search *search, const Board *board)
{
	HashData hash_data[1];
	int move = NOMOVE;

	if (hash_get(search->pv_table, board, board_get_hash_code(board), hash_data)) move = hash_data->move[0];
	if (move == NOMOVE && hash_get(search->hash_table, board, board_get_hash_code(board), hash_data)) move = hash_data->move[0];

	return move;
}

