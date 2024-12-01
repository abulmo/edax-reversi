/**
 * @file board.h
 *
 * Board management header file.
 *
 * @date 1998 - 2024
 * @author Richard Delorme
 * @author Toshihiko Okuhara
 * @version 4.6
 */

#ifndef EDAX_BOARD_H
#define EDAX_BOARD_H

#include "const.h"
#include "simd.h"

#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>


/** Board : board representation */
typedef struct Board {
	uint64_t player, opponent;     /**< bitboard representation */
} Board;

struct Move;
struct Random;

/* function declarations */
void board_init(Board*);
int board_set(Board*, const char*);
int board_from_FEN(Board*, const char*);
int board_compare(const Board*, const Board*);
bool board_equal(const Board*, const Board*);
void board_symetry(const Board*, const int, Board*);
int board_unique(const Board*, Board*);
void board_check(const Board*);
void board_rand(Board*, int, struct Random*);

uint64_t board_get_move(const Board*, const int, struct Move*);
bool board_check_move(const Board*, struct Move*);
void board_swap_players(Board*);
void board_update(Board*, const struct Move*);
void board_restore(Board*, const struct Move*);
void board_pass(Board*);
uint64_t board_next(const Board*, const int, Board*);
uint64_t board_pass_next(const Board*, const int, Board*);
uint64_t board_get_hash_code(const Board*);
int board_get_square_color(const Board*, const int);
bool board_is_occupied(const Board*, const int);
void board_print(const Board*, const int, FILE*);
char* board_to_string(const Board*, const int, char *);
void board_print_FEN(const Board*, const int, FILE*);
char* board_to_FEN(const Board*, const int, char*);
bool board_is_pass(const Board*);
bool board_is_game_over(const Board*);
int board_count_empties(const Board *board);
uint64_t board_flip(const Board*, const int);

uint64_t flip(const int, const uint64_t P, const uint64_t O);
int count_last_flip(const int, const uint64_t);
bool can_move(const uint64_t, const uint64_t);
uint64_t get_moves_6x6(const uint64_t, const uint64_t);
bool can_move_6x6(const uint64_t, const uint64_t);
int get_mobility(const uint64_t, const uint64_t);
int get_weighted_mobility(const uint64_t, const uint64_t);
int get_potential_mobility(const uint64_t, const uint64_t);
void edge_stability_init(void);
int get_stability(const uint64_t, const uint64_t);
int get_edge_stability(const uint64_t, const uint64_t);
int get_corner_stability(const uint64_t);

#if USE_SIMD && defined(__AVX2__)
	uint64_t vectorcall get_moves_avx2(__m256i, __m256i);
	#define get_moves(P, O) get_moves_avx2(_mm256_set1_epi64x(P), _mm256_set1_epi64x(O))
	#define	board_get_moves(board)	get_moves_avx2(_mm256_set1_epi64x((board)->player), _mm256_set1_epi64x((board)->opponent))
	__m128i vectorcall get_moves_and_potential(__m256i, __m256i);
#else
	uint64_t get_moves(const uint64_t, const uint64_t);
	#define	board_get_moves(board)	get_moves((board)->player, (board)->opponent)
#endif

void board_test(void);

#endif // EDAX_BOARD_H

