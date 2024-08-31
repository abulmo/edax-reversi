/**
 * @file move.h
 *
 * @brief Move & list of moves management - header file.
 *
 * @date 1998 - 2023
 * @author Richard Delorme
 * @version 4.5
 */

#ifndef EDAX_MOVE_H
#define EDAX_MOVE_H

#include "const.h"

#include <stdio.h>
#include <stdbool.h>

/** move representation */
typedef struct Move {
	struct Move *next;            /**< next move in a MoveList */
	unsigned long long flipped;   /**< bitboard representation of flipped squares */
	int x;                        /**< square played */
	int score;                    /**< score for this move */
	unsigned int cost;            /**< move cost */
} Move;

/** (simple) list of a legal moves */
typedef struct MoveList {
	int n_moves;
	Move move[MAX_MOVE + 1];   /**< array of legal moves */
} MoveList;

/** (simple) sequence of a legal moves */
typedef struct Line {
	char move[GAME_SIZE];   /**< array of a suite of moves */
	int n_moves;
	int color;
} Line;

struct Search;
struct HashData;
struct Board;

/* useful constants */
extern const Move MOVE_INIT;
extern const Move MOVE_PASS;


/* function declarations */
int symetry(int, const int);

void move_print(const int, const int, FILE*);
Move* move_next_best(Move*);
char* move_to_string(const int, const int, char*);

void tune_move_evaluate(struct Search*, const char*, const char*);

int movelist_get_moves(MoveList*, const struct Board*);
void movelist_print(const MoveList*, const int, FILE*);
Move* movelist_sort_bestmove(MoveList*, const int);
void movelist_evaluate_fast(MoveList*, struct Search*, const struct HashData*);
void movelist_evaluate(MoveList*, struct Search*, const struct HashData*, const int, const int);

// bool move_wipeout(const Move*, const struct Board*);	// Check if a move wins 64-0.
#define	move_wipeout(move,board)	((move)->flipped == (board)->opponent)
// Move* move_next(Move*);	// Return the next move from the list.
#define move_next(move)	((move)->next)
// Move* movelist_best(MoveList*);	// Return the best move of the list.
#define	movelist_best(movelist)	move_next_best((movelist)->move)
// Move* movelist_first(MoveList*);	// Return the first move of the list.
#define	movelist_first(movelist)	move_next((movelist)->move)
// bool movelist_is_empty(const MoveList*);	// Check if the list is empty.
#define	movelist_is_empty(movelist)	((movelist)->n_moves < 1)

Move* movelist_exclude(MoveList*, const int);
void movelist_restore(MoveList*, Move*);

void movelist_sort(MoveList*);
void movelist_sort_cost(MoveList*, const struct HashData*);
bool movelist_is_single(const MoveList*);

/** macro to iterate over the movelist */
#define foreach_move(iter, movelist) \
	for ((iter) = (movelist).move[0].next; (iter); (iter) = (iter)->next)

/** macro to iterate over the movelist from best to worst move */
#define foreach_best_move(iter, movelist) \
	(iter) = &(movelist).move[0];\
	while (((iter) = move_next_best(iter)))

void line_init(Line*, const int);
void line_push(Line*, const int);
void line_pop(Line*);
void line_copy(Line*, const Line*, const int);
void line_print(const Line*, int, const char*, FILE*);
char* line_to_string(const Line *line, int n, const char*, char *string);

/** HashTable of position + move */
typedef struct MoveHash {
	struct MoveArray *array;
	int size;
	int mask;
} MoveHash;
void movehash_init(MoveHash*, int);
void movehash_delete(MoveHash*);
bool movehash_append(MoveHash*, const struct Board*, const int);

#endif

