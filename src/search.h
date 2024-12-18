/**
 * @file search.h
 *
 * Search's header file.
 *
 * @date 1998 - 2024
 * @author Richard Delorme
 * @version 4.6
 */

#ifndef EDAX_SEARCH_H
#define EDAX_SEARCH_H

#include "bit.h"
#include "board.h"
#include "const.h"
#include "empty.h"
#include "eval.h"
#include "hash.h"
#include "move.h"
#include "util.h"

#include <stdio.h>
#include <stdint.h>

/** Selectivity probcut */
typedef struct Selectivity {
	double t;     /**< selectivity value */
	int level;    /**< level of selectivity */
	int percent;  /**< selectivity value as a percentage */
} Selectivity;

struct Task;
struct TaskQueue;

/** Bound */
typedef struct Bound {
	int lower;
	int upper;
} Bound;

/** Result */
typedef struct Result {
	Bound bound[BOARD_SIZE + 2]; /**< score bounds / move */
	Line pv;                     /**< principal variation */
	SpinLock spin;               /**< spin lock */
	int64_t time;                /**< searched time */
	uint64_t n_nodes;            /**< searched node count */
	int depth;                   /**< searched depth */
	int selectivity;             /**< searched selectivity */
	int move;                    /**< best move found */
	int score;                   /**< best score */
	int n_moves;                 /**< total moves to search */
	int n_moves_left;            /**< left moves to search */
	bool book_move;              /**< book move origin */
} Result;

/** levels */
extern struct Level {
	int depth;         /** search depth */
	int selectivity;   /** search selectivity level */
} LEVEL[61][61];


/** search stare */
typedef struct Search {
	alignas(16) Board board;                      /**< othello board */
	SquareList empties[BOARD_SIZE + 2];           /**< list of empty squares */
	HashTable hash_table;                         /**< hashtable */
	HashTable pv_table;                           /**< hashtable for the pv */
	HashTable shallow_table;                      /**< hashtable for short search */
	Eval eval;                                     /**< eval */
	Random random;                                /**< random generator */
	int n_empties;                                /**< number of empty squares */
	int player;                                   /**< player color */
	int id;                                       /**< search id */

	struct TaskStack *tasks;                      /**< available task queue */
	struct Task *task;                            /**< search task */
	SpinLock spin;                                /**< search lock */
	struct Search *parent;                        /**< parent search */
	struct Search *child[MAX_THREADS];            /**< child search */
	struct Search *master;                        /**< master search (parent of all searches)*/
	int n_child;                                  /**< search child number */

	struct {
		int64_t extra;                            /**< extra alloted time */
		int64_t spent;                            /**< time spent thinking */
		bool extended;                            /**< flag to extend time only once */
		bool can_update;                          /**< flag allowing to extend time */
		int64_t mini;                             /**< minimal alloted time */
		int64_t maxi;                             /**< maximal alloted time */
	} time;                                       /**< time */
	MoveList movelist;                            /**< list of moves */
	NodeType node_type[GAME_SIZE];                /**< node type (pv node, cut node, all node) */
	Bound stability_bound;                        /**< score bounds according to stable squares */
	Stop stop;                                    /**< thinking status */
	int depth;                                    /**< depth level */
	int selectivity;                              /**< selectivity level */
	int probcut_level;                            /**< probcut recursivity level */
	uint32_t parity;                              /**< parity */
	int depth_pv_extension;                       /**< depth for pv_extension */
	int height;                                   /**< search height from root */
	bool allow_node_splitting;                    /**< allow parallelism */

	struct {
		int depth;                                /**< depth */
		int selectivity;                          /**< final selectivity */
		int64_t time;                             /**< time in sec. */
		bool time_per_move;                       /**< time_per_move or per game ?*/
		int verbosity;                            /**< verbosity level */
		bool keep_date;                           /**< keep date */
		const char *header;                       /**< header for search output */
		const char *separator;                    /**< separator for search output */
		bool guess_pv;                            /**< guess PV (in cassio mode only) */
		int multipv_depth;                        /**< multi PV depth */
		int hash_size;                            /**< hashtable size */
	} options;                                    /**< local (threadable) options. */

	Result *result;                               /**< shared result */ //TODO: remove allocation ?

	void (*observer)(Result*);                    /**< call back function to print search result */

	int64_t n_nodes;                              /**< node counter */
	int64_t child_nodes;                          /**< node counter */

} Search;

struct Node;

extern const uint8_t QUADRANT_ID[];
extern const Selectivity selectivity_table[];
extern const int NO_SELECTIVITY;
extern const int8_t NWS_STABILITY_THRESHOLD[];
extern const int8_t PVS_STABILITY_THRESHOLD[];
extern const uint8_t SQUARE_TYPE[];

/* function definition */
void search_global_init(void);
void search_init(Search*);
void search_free(Search*);
void search_cleanup(Search*);
void search_setup(Search*);
void search_clone(Search*, Search*);
void search_set_board(Search*, const Board*, const int);
void search_set_level(Search*, const int, const int);
void search_set_ponder_level(Search*, const int, const int);
void search_resize_hashtable(Search*);

void search_set_game_time(Search*, const int64_t);
void search_set_move_time(Search*, const int64_t);
void search_time_init(Search*);
void search_time_reset(Search*, const Board*);
void search_adjust_time(Search*, const bool);
bool search_continue(Search *);
void search_check_timeout(Search *search);

void search_set_task_number(Search*, const int);

void search_swap_parity(Search*, const int);
void search_get_movelist(const Search*, MoveList*);
void search_update_endgame(Search*, const Move*);
void search_restore_endgame(Search*, const Move*);
void search_pass_endgame(Search*);
void search_update_midgame(Search*, const Move*);
void search_restore_midgame(Search*, const Move*);
void search_update_pass_midgame(Search*);
void search_restore_pass_midgame(Search*);
int64_t search_clock(Search*);
int64_t search_time(Search*);
int64_t search_count_nodes(Search*);
void search_print_pv(Search*, const int, const char*, FILE*);
void search_print(Search*, const int, const int, const char, FILE*);
int get_pv_extension(const int, const int);

void result_print(Result*, FILE*);

bool search_SC_PVS(Search*, int*, int*, int*);
bool search_SC_NWS(Search*, const int, int*);
bool search_SC_NWS_full(Search*, const int, int*, uint64_t*);
bool search_SC_NWS_4(Search*, const int, int*);
bool search_TC_PVS(HashData*, const int, const int, int*, int*, int*);
bool search_TC_NWS(HashData*, const int, const int, const int, int*);
bool search_ETC_PVS(Search*, MoveList*, uint64_t, const int, const int, int*, int*, int*);
bool search_ETC_NWS(Search*, MoveList*, uint64_t, const int, const int, const int, int*);

NodeType next_node_type(const NodeType parent, const bool first_move);

int search_solve(const Search*);
int search_solve_0(const Search*);
int NWS_endgame(Search*, const int);

int search_eval_0(Search*);
int search_eval_1(Search*, const int, int);
int search_eval_2(Search*, int, const int);
int NWS_midgame(Search*, const int, int, struct Node*);
int PVS_midgame(Search*, const int, const int, int, struct Node*);
int NWS_shallow(Search*, const int, int, HashTable*);
int PVS_shallow(Search*, int, int, int);

bool is_pv_ok(Search*, int, int);
void record_best_move(Search*, const Board*, const Move*, const int, const int, const int);
int PVS_root(Search*, const int, const int, const int);
int aspiration_search(Search*, int, int, const int, int);
void iterative_deepening(Search*, int, int);
int search_run(void*);
int search_guess(Search*, const Board*);
void search_stop_all(Search*, const Stop);
void search_set_state(Search*, const Stop);

void search_observer(Result*);
void search_set_observer(Search*, void (*Observer)(Result*));

void search_share(const Search*, Search*);
int search_count_tasks(const Search *);

bool is_depth_solving(const int, const int);
int solvable_depth(const int64_t, int);
void pv_debug(Search*, const Move*, FILE*);
int search_get_pv_cost(Search*);
void show_current_move(FILE *f, Search*, const Move*, const int, const int, const bool);
int search_bound(const Search*, int);

#endif

