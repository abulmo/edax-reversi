/**
 * @file move.c
 *
 * @brief Move & list of moves management - header file.
 *
 * @date 1998 - 2012
 * @author Richard Delorme
 * @version 4.3
 */

#ifndef EDAX_MOVE_H
#define EDAX_MOVE_H

#include "const.h"

#include <stdio.h>
#include <stdbool.h>

/** move representation */
typedef struct Move {
	unsigned long long flipped;   /**< bitboard representation of flipped squares */
	int x;                        /**< square played */
	int score;                    /**< score for this move */
	unsigned int cost;            /**< move cost */
	struct Move *next;            /**< next move in a MoveList */
} Move;

/** (simple) list of a legal moves */
typedef struct MoveList {
	Move move[MAX_MOVE + 2];   /**< array of legal moves */
	int n_moves;
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
bool move_wipeout(const Move*, const struct Board*);
Move* move_next_best(Move*);
Move* move_next(Move*);
char* move_to_string(const int, const int, char*);

void tune_move_evaluate(struct Search*, const char*, const char*);

int movelist_get_moves(MoveList*, const struct Board*);
void movelist_print(const MoveList*, const int, FILE*);
Move* movelist_sort_bestmove(MoveList*, const int);
void movelist_evaluate(MoveList*, struct Search*, const struct HashData*, const int, const int);
void movelist_evaluate_fast(MoveList*, struct Search*);
Move* movelist_best(MoveList*);
Move* movelist_first(MoveList*);

Move* movelist_exclude(MoveList*, const int);
void movelist_restore(MoveList*, Move*);

void movelist_sort(MoveList*);
void movelist_sort_cost(MoveList*, const struct HashData*);
bool movelist_is_empty(const MoveList*);
bool movelist_is_single(const MoveList*);

/** macro to iterate over the movelist */
#define foreach_move(iter, movelist) \
	for ((iter) = (movelist)->move->next; (iter); (iter) = (iter)->next)

/** macro to iterate over the movelist from best to worst move */
#define foreach_best_move(iter, movelist) \
	for ((iter) = movelist_best(movelist); (iter); (iter) = move_next_best(iter))

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

