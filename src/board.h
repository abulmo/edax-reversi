/**
 * @file board.h
 *
 * Board management header file.
 *
 * @date 1998 - 2012
 * @author Richard Delorme
 * @version 4.3
 */

#ifndef EDAX_BOARD_H
#define EDAX_BOARD_H

#include "const.h"

#include <stdio.h>
#include <stdbool.h>

/** Board : board representation */
typedef struct Board {
	unsigned long long player, opponent;     /**< bitboard representation */
} Board;

struct Move;

/* function declarations */
void board_init(Board*);
int board_set(Board*, const char*);
int board_from_FEN(Board*, const char*);
int board_compare(const Board*, const Board*);
bool board_equal(const Board*, const Board*);
void board_symetry(const Board*, const int, Board*);
int board_unique(const Board*, Board*);
void board_check(const Board*);

int board_count_last_flips(const Board*, const int);
unsigned long long board_get_move(const Board*, const int, struct Move*);
bool board_check_move(const Board*, struct Move*);
void board_swap_players(Board*);
void board_update(Board*, const struct Move*);
void board_restore(Board*, const struct Move*);
void board_pass(Board*);
unsigned long long board_next(const Board*, const int, Board*);
unsigned long long board_pass_next(const Board*, const int, Board*);
unsigned long long board_get_hash_code(const Board*);
int board_get_square_color(const Board*, const int);
bool board_is_occupied(const Board*, const int);
void board_print(const Board*, const int, FILE*);
char* board_to_string(const Board*, const int, char *);
void board_print_FEN(const Board*, const int, FILE*);
char* board_to_FEN(const Board*, const int, char*);
bool board_is_pass(const Board*);
bool board_is_game_over(const Board*);
int board_count_empties(const Board *board);

extern int (*count_last_flip[BOARD_SIZE + 1])(const unsigned long long);
extern unsigned long long (*flip[BOARD_SIZE + 2])(const unsigned long long, const unsigned long long);
unsigned long long get_moves(const unsigned long long, const unsigned long long);
bool can_move(const unsigned long long, const unsigned long long);
unsigned long long get_moves_6x6(const unsigned long long, const unsigned long long);
bool can_move_6x6(const unsigned long long, const unsigned long long);
int get_mobility(const unsigned long long, const unsigned long long);
int get_weighted_mobility(const unsigned long long, const unsigned long long);
int get_potential_mobility(const unsigned long long, const unsigned long long);
void edge_stability_init(void);
int get_stability(const unsigned long long, const unsigned long long);
int get_edge_stability(const unsigned long long, const unsigned long long);
int get_corner_stability(const unsigned long long);

#endif

