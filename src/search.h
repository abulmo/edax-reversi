/**
 * @file search.h
 *
 * Search's header file.
 *
 * @date 1998 - 2012
 * @author Richard Delorme
 * @version 4.3
 */

#ifndef EDAX_SEARCH_H
#define EDAX_SEARCH_H

#include "board.h"
#include "const.h"
#include "empty.h"
#include "eval.h"
#include "hash.h"
#include "move.h"
#include "util.h"

#include <stdio.h>

/** Selectivity probcut */
typedef struct Selectivity {
	double t; /**< selectivity value */
	int level; /**< level of selectivity */
	int percent; /**< selectivity value as a percentage */
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
	int depth;                   /**< searched depth */
	int selectivity;             /**< searched selectivity */
	int move;                    /**< best move found */
	int score;                   /**< best score */
	Bound bound[BOARD_SIZE + 2]; /**< score bounds / move */
	Line pv[1];                  /**< principal variation */
	long long time;              /**< searched time */
	unsigned long long n_nodes;  /**< searched node count */
	bool book_move;              /**< book move origin */
	int n_moves;                 /**< total moves to search */
	int n_moves_left;            /**< left moves to search */
	SpinLock spin;
} Result;

/** levels */
extern struct Level {
	int depth;         /** search depth */
	int selectivity;   /** search selectivity level */
} LEVEL[61][61];


/** search stare */
typedef struct Search {
	Board board[1];                               /**< othello board */
	SquareList empties[BOARD_SIZE + 2];           /**< list of empty squares */
	SquareList *x_to_empties[BOARD_SIZE + 2];     /**< link position to the list */
	int n_empties;                                /**< number of empty squares */
	int player;                                   /**< player color */
	int id;                                       /**< search id */

	HashTable hash_table[1];                      /**< hashtable */
	HashTable pv_table[1];                        /**< hashtable for the pv */
	HashTable shallow_table[1];                   /**< hashtable for short search */
	Eval eval[1];                                 /**< eval */
	Random random[1];                             /**< random generator */

	struct TaskStack *tasks;                      /**< available task queue */
	struct Task *task;                            /**< search task */
	SpinLock spin;                                /**< search lock */
	struct Search *parent;                        /**< parent search */
	struct Search *child[MAX_THREADS];            /**< child search */
	struct Search *master;                        /**< master search (parent of all searches)*/
	volatile int n_child;                         /**< search child number */

	int depth;                                    /**< depth level */
	int selectivity;                              /**< selectivity level */
	int probcut_level;                            /**< probcut recursivity level */
	unsigned int parity;                          /**< parity */
	int depth_pv_extension;                       /**< depth for pv_extension */
	volatile Stop stop;                           /**< thinking status */
	bool allow_node_splitting;                    /**< allow parallelism */
	struct {
		long long  extra;                         /**< extra alloted time */
		volatile long long spent;                 /**< time spent thinking */
		bool extended;                            /**< flag to extend time only once */
		bool can_update;                          /**< flag allowing to extend time */
		long long  mini;                          /**< minimal alloted time */
		long long  maxi;                          /**< maximal alloted time */
	} time;                                       /**< time */
	MoveList movelist[1];                         /**< list of moves */
	int height;                                   /**< search height from root */
	NodeType node_type[GAME_SIZE];                /**< node type (pv node, cut node, all node) */
	Bound stability_bound;                        /**< score bounds according to stable squares */

	struct {
		int depth;                                /**< depth */
		int selectivity;                          /**< final selectivity */
		long long time;                           /**< time in sec. */
		bool time_per_move;                       /**< time_per_move or per game ?*/
		int verbosity;                            /**< verbosity level */
		bool keep_date;                           /**< keep date */
		const char *header;                       /**< header for search output */
		const char *separator;                    /**< separator for search output */
		bool guess_pv;                            /**< guess PV (in cassio mode only) */
		int multipv_depth;                        /**< multi PV depth */
		int hash_size;                            /**< hashtable size */                    
	} options;                                    /**< local (threadable) options. */

	Result *result;                               /**< shared result */

	void (*observer)(Result*);                    /**< call back function to print search result */

	volatile unsigned long long n_nodes;          /**< node counter */
	volatile unsigned long long child_nodes;      /**< node counter */

} Search;

struct Node;

extern const int QUADRANT_ID[];
extern const Selectivity selectivity_table[];
extern const int NO_SELECTIVITY;
extern const int NWS_STABILITY_THRESHOLD[];
extern const int PVS_STABILITY_THRESHOLD[];
extern const int SQUARE_TYPE[];

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

void search_set_game_time(Search*, const long long);
void search_set_move_time(Search*, const long long);
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
long long search_clock(Search*);
long long search_time(Search*);
unsigned long long search_count_nodes(Search*);
void search_print_pv(Search*, const int, const char*, FILE*);
void search_print(Search*, const int, const int, const char, FILE*);
int get_pv_extension(const int, const int);

void result_print(Result*, FILE*);

bool search_SC_PVS(Search*, volatile int*, volatile int*, int*);
bool search_SC_NWS(Search*, const int, int*);
bool search_TC_PVS(HashData*, const int, const int, volatile int*, volatile int*, int*);
bool search_TC_NWS(HashData*, const int, const int, const int, int*);
bool search_ETC_PVS(Search*, MoveList*, unsigned long long, const int, const int, volatile int*, volatile int*, int*);
bool search_ETC_NWS(Search*, MoveList*, unsigned long long, const int, const int, const int, int*);

NodeType next_node_type(const NodeType parent, const bool first_move);

int search_solve(const Search*);
int search_solve_0(const Search*);
extern int board_score_1(const Board*, const int, const int);
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
void* search_run(void*);
int search_guess(Search*, const Board*);
void search_stop_all(Search*, const Stop);
void search_set_state(Search*, const Stop);

void search_observer(Result*);
void search_set_observer(Search*, void (*Observer)(Result*));

void search_share(const Search*, Search*);
int search_count_tasks(const Search *);

bool is_depth_solving(const int, const int);
int solvable_depth(const long long, int);
void pv_debug(Search*, const Move*, FILE*);
int search_get_pv_cost(Search*);
void show_current_move(FILE *f, Search*, const Move*, const int, const int, const bool);
int search_bound(const Search*, int);

#endif

