/**
 * @file endgame.c
 *
 * Search near the end of the game.
 *
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
 * @date 1998 - 2024
=======
 * @date 1998 - 2017
>>>>>>> b3f048d (copyright changes)
=======
 * @date 1998 - 2020
>>>>>>> f1d221c (Replace eval_restore with simple save-restore, as well as parity)
=======
 * @date 1998 - 2023
>>>>>>> bb98132 (Split 5 empties search_shallow loop; tune stabiliby cutoff)
 * @author Richard Delorme
 * @author Toshihiko Okuhara
=======
 * @date 1998 - 2022
 * @author Richard Delorme
<<<<<<< HEAD
>>>>>>> 6c3ed52 (Dogaishi hash reduction by Matsuo & Narazaki; edge-precise get_full_line)
=======
 * @author Toshihiko Okuhara
>>>>>>> 23e04d1 (Backport endgame_sse optimizations into endgame.c)
 * @version 4.5
 */


#include "search.h"

#include "bit.h"
#include "settings.h"
#include "stats.h"
#include "ybwc.h"

#include <assert.h>

#if LAST_FLIP_COUNTER == COUNT_LAST_FLIP_CARRY
	#include "count_last_flip_carry_64.c"
#elif LAST_FLIP_COUNTER == COUNT_LAST_FLIP_SSE
	#ifdef hasSSE2
		#include "count_last_flip_sse.c"
	#else
		#include "count_last_flip_neon.c"
	#endif
#elif LAST_FLIP_COUNTER == COUNT_LAST_FLIP_BITSCAN
	#include "count_last_flip_bitscan.c"
#elif LAST_FLIP_COUNTER == COUNT_LAST_FLIP_PLAIN
	#include "count_last_flip_plain.c"
#elif LAST_FLIP_COUNTER == COUNT_LAST_FLIP_32
	#include "count_last_flip_32.c"
#elif LAST_FLIP_COUNTER == COUNT_LAST_FLIP_BMI2
	#include "count_last_flip_bmi2.c"
<<<<<<< HEAD
<<<<<<< HEAD
=======
>>>>>>> 52949e1 (Add build options and files for new count_last_flips)
#elif LAST_FLIP_COUNTER == COUNT_LAST_FLIP_AVX_PPFILL
	#include "count_last_flip_avx_ppfill.c"
#elif LAST_FLIP_COUNTER == COUNT_LAST_FLIP_AVX512
	#include "count_last_flip_avx512cd.c"
<<<<<<< HEAD
#elif LAST_FLIP_COUNTER == COUNT_LAST_FLIP_NEON
	#include "count_last_flip_neon.c"
#elif LAST_FLIP_COUNTER == COUNT_LAST_FLIP_SVE
	#include "count_last_flip_sve_lzcnt.c"
=======
>>>>>>> 6506166 (More SSE optimizations)
=======
>>>>>>> 52949e1 (Add build options and files for new count_last_flips)
#else // LAST_FLIP_COUNTER == COUNT_LAST_FLIP_KINDERGARTEN
	#include "count_last_flip_kindergarten.c"
#endif

<<<<<<< HEAD
<<<<<<< HEAD
=======
#if ((MOVE_GENERATOR == MOVE_GENERATOR_AVX) || (MOVE_GENERATOR == MOVE_GENERATOR_SSE)) && (LAST_FLIP_COUNTER == COUNT_LAST_FLIP_SSE)
	#include "endgame_sse.c"	// vectorcall version
#elif (MOVE_GENERATOR == MOVE_GENERATOR_NEON) && (LAST_FLIP_COUNTER == COUNT_LAST_FLIP_SSE)
	#include "endgame_neon.c"
#endif

>>>>>>> 6506166 (More SSE optimizations)
=======
>>>>>>> 26dad03 (Use player bits only in board_score_1)
/**
 * @brief Get the final score.
 *
 * Get the final score, when no move can be made.
 *
 * @param player Board.player
 * @param n_empties Number of empty squares remaining on the board.
 * @return The final score, as a disc difference.
 */
<<<<<<< HEAD
<<<<<<< HEAD
static int board_solve(const unsigned long long player, const int n_empties)
{
	int score = bit_count(player) * 2 - SCORE_MAX;	// in case of opponents win
=======
static int board_solve(const Board *board, const int n_empties)
{
	int score = bit_count(board->player) * 2 - SCORE_MAX;	// in case of opponents win
>>>>>>> 1b29848 (fix & optimize 32 bit build; other minor mods)
=======
static int board_solve(const unsigned long long player, const int n_empties)
{
	int score = bit_count(player) * 2 - SCORE_MAX;	// in case of opponents win
>>>>>>> 26dad03 (Use player bits only in board_score_1)
	int diff = score + n_empties;		// = n_discs_p - (64 - n_empties - n_discs_p)

	SEARCH_STATS(++statistics.n_search_solve);

<<<<<<< HEAD
<<<<<<< HEAD
	if (diff == 0)
		score = diff;
	else if (diff > 0)
		score = diff + n_empties;
=======
	if (diff >= 0)
		score = diff;
	if (diff > 0)
		score += n_empties;
>>>>>>> 1b29848 (fix & optimize 32 bit build; other minor mods)
=======
	if (diff == 0)
		score = diff;
	else if (diff > 0)
		score = diff + n_empties;
>>>>>>> c0fb778 (small optimizations in endgame)
	return score;
}

/**
 * @brief Get the final score.
 *
 * Get the final score, when no move can be made.
 *
 * @param search Search.
 * @return The final score, as a disc difference.
 */
int search_solve(const Search *search)
{
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
	return board_solve(search->board.player, search->eval.n_empties);
=======
	return board_solve(search->board, search->n_empties);
>>>>>>> 1b29848 (fix & optimize 32 bit build; other minor mods)
=======
	return board_solve(&search->board, search->n_empties);
>>>>>>> 0a166fd (Remove 1 element array coding style)
=======
	return board_solve(&search->board, search->eval.n_empties);
>>>>>>> c8248ad (Move n_empties into Eval; tweak eval_open and eval_set)
=======
	return board_solve(search->board.player, search->eval.n_empties);
>>>>>>> 26dad03 (Use player bits only in board_score_1)
}

/**
 * @brief Get the final score.
 *
 * Get the final score, when the board is full.
 *
 * @param search Search.
 * @return The final score, as a disc difference.
 */
int search_solve_0(const Search *search)
{
	SEARCH_STATS(++statistics.n_search_solve_0);

<<<<<<< HEAD
<<<<<<< HEAD
	return 2 * bit_count(search->board.player) - SCORE_MAX;
=======
	return 2 * bit_count(search->board->player) - SCORE_MAX;
>>>>>>> 1b29848 (fix & optimize 32 bit build; other minor mods)
=======
	return 2 * bit_count(search->board.player) - SCORE_MAX;
>>>>>>> 0a166fd (Remove 1 element array coding style)
}

<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
#if ((MOVE_GENERATOR == MOVE_GENERATOR_AVX) || (MOVE_GENERATOR == MOVE_GENERATOR_AVX512) || (MOVE_GENERATOR == MOVE_GENERATOR_SSE)) && ((LAST_FLIP_COUNTER == COUNT_LAST_FLIP_SSE) || (LAST_FLIP_COUNTER >= COUNT_LAST_FLIP_BMI2))
	#include "endgame_sse.c"	// vectorcall version
#elif ((MOVE_GENERATOR == MOVE_GENERATOR_NEON) || (MOVE_GENERATOR == MOVE_GENERATOR_SVE)) && ((LAST_FLIP_COUNTER == COUNT_LAST_FLIP_NEON) || ((LAST_FLIP_COUNTER == COUNT_LAST_FLIP_SVE) && defined(SIMULLASTFLIP)))
	#include "endgame_neon.c"
#else
=======
#if ((MOVE_GENERATOR != MOVE_GENERATOR_AVX) && (MOVE_GENERATOR != MOVE_GENERATOR_SSE) && (MOVE_GENERATOR != MOVE_GENERATOR_NEON)) || (LAST_FLIP_COUNTER != COUNT_LAST_FLIP_SSE)
>>>>>>> 81dec96 (Kindergarten last flip for arm32; MSVC arm Windows build (not tested))
=======
#if ((MOVE_GENERATOR == MOVE_GENERATOR_AVX) || (MOVE_GENERATOR == MOVE_GENERATOR_SSE)) && (LAST_FLIP_COUNTER == COUNT_LAST_FLIP_SSE)
=======
#if ((MOVE_GENERATOR == MOVE_GENERATOR_AVX) || (MOVE_GENERATOR == MOVE_GENERATOR_AVX512) || (MOVE_GENERATOR == MOVE_GENERATOR_SSE)) && (LAST_FLIP_COUNTER == COUNT_LAST_FLIP_SSE)
>>>>>>> ff1c5db (skip hash access if n_moves <= 1 in NWS_endgame)
=======
#if ((MOVE_GENERATOR == MOVE_GENERATOR_AVX) || (MOVE_GENERATOR == MOVE_GENERATOR_AVX512) || (MOVE_GENERATOR == MOVE_GENERATOR_SSE)) && ((LAST_FLIP_COUNTER == COUNT_LAST_FLIP_SSE) || (LAST_FLIP_COUNTER == COUNT_LAST_FLIP_BMI2))
>>>>>>> 9ea5b5e (BMI2 and mm_LastFlip version of board_score_sse_1 added (but not enabled))
=======
#if ((MOVE_GENERATOR == MOVE_GENERATOR_AVX) || (MOVE_GENERATOR == MOVE_GENERATOR_AVX512) || (MOVE_GENERATOR == MOVE_GENERATOR_SSE)) && ((LAST_FLIP_COUNTER == COUNT_LAST_FLIP_SSE) || (LAST_FLIP_COUNTER >= COUNT_LAST_FLIP_BMI2))
>>>>>>> 52949e1 (Add build options and files for new count_last_flips)
	#include "endgame_sse.c"	// vectorcall version
#elif (MOVE_GENERATOR == MOVE_GENERATOR_NEON) && (LAST_FLIP_COUNTER == COUNT_LAST_FLIP_SSE)
	#include "endgame_neon.c"
#else
>>>>>>> 26dad03 (Use player bits only in board_score_1)
/**
 * @brief Get the final score.
 *
 * Get the final score, when 1 empty square remain.
 * The following code has been adapted from Zebra by Gunnar Anderson.
 *
 * @param player Board.player to evaluate.
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
 * @param alpha  Alpha bound. (beta - 1)
=======
 * @param beta   Beta bound.
>>>>>>> 26dad03 (Use player bits only in board_score_1)
=======
 * @param beta   Beta bound - 1.
>>>>>>> 9ec6e5d (Negative score in endgame solve 2/3/4; offset beta in score_1)
=======
 * @param alpha  Alpha bound. (beta - 1)
>>>>>>> c8118a8 (Use minimax instead of nagamax for solve 4 or less)
 * @param x      Last empty square to play.
 * @return       The final score, as a disc difference.
 */
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
int board_score_1(const unsigned long long player, const int alpha, const int x)
=======
int board_score_1(const Board *board, const int beta, const int x)
>>>>>>> 1b29848 (fix & optimize 32 bit build; other minor mods)
{
	int score, score2, n_flips;

	score = 2 * bit_count(player) - SCORE_MAX + 2;	// = (bit_count(P) + 1) - (SCORE_MAX - 1 - bit_count(P))

<<<<<<< HEAD
<<<<<<< HEAD
	n_flips = last_flip(x, player);
	score += n_flips;

	if (n_flips == 0) {	// (23%)
		score2 = score - 2;	// empty for opponent
		if (score <= 0)
			score = score2;
		if (score > alpha) {	// lazy cut-off (40%)
			if ((n_flips = last_flip(x, ~player)) != 0)	// (98%)
				score = score2 - n_flips;
=======
	if ((n_flips = last_flip(x, board->player)) != 0) {
		score -= n_flips;
	} else {
=======
	n_flips = last_flip(x, board->player);
=======
int board_score_1(const unsigned long long player, const int beta, const int x)
=======
int board_score_1(const unsigned long long player, const int alpha, const int x)
>>>>>>> c8118a8 (Use minimax instead of nagamax for solve 4 or less)
{
	int score, score2, n_flips;

	score = 2 * bit_count(player) - SCORE_MAX  + 2;	// = (bit_count(P) + 1) - (SCORE_MAX - 1 - bit_count(P))

	n_flips = last_flip(x, player);
<<<<<<< HEAD
>>>>>>> 26dad03 (Use player bits only in board_score_1)
	score -= n_flips;
=======
	score += n_flips;
>>>>>>> 266ad5a (minimax from 5 empties and swap min/max stages)

<<<<<<< HEAD
	if (n_flips == 0) {
>>>>>>> 9ad160e (4.4.7 AVX/shuffle optimization in endgame_sse.c)
=======
	if (n_flips == 0) {	// (23%)
<<<<<<< HEAD
<<<<<<< HEAD
>>>>>>> 9f982ee (Revise PASS handling; prioritymoves in shallow; optimize Neighbour test)
		score2 = score + 2;	// empty for player
=======
		score2 = score + 2;	// empty for opponent
>>>>>>> 0ba5408 (add vectorcall to inline functions in case not inlined)
		if (score >= 0)
=======
		score2 = score - 2;	// empty for opponent
		if (score <= 0)
>>>>>>> 266ad5a (minimax from 5 empties and swap min/max stages)
			score = score2;
		if (score > alpha) {	// lazy cut-off (40%)
			if ((n_flips = last_flip(x, ~player)) != 0)	// (98%)
<<<<<<< HEAD
				score = score2 + n_flips;
<<<<<<< HEAD
			}
>>>>>>> 1b29848 (fix & optimize 32 bit build; other minor mods)
=======
>>>>>>> 9ad160e (4.4.7 AVX/shuffle optimization in endgame_sse.c)
=======
				score = score2 - n_flips;
>>>>>>> 266ad5a (minimax from 5 empties and swap min/max stages)
		}
	}

	return score;
}

/**
 * @brief Get the final score.
 *
<<<<<<< HEAD
<<<<<<< HEAD
 * Get the final min score, when 2 empty squares remain.
=======
 * Get the final max score, when 2 empty squares remain.
>>>>>>> c8118a8 (Use minimax instead of nagamax for solve 4 or less)
=======
 * Get the final min score, when 2 empty squares remain.
>>>>>>> 266ad5a (minimax from 5 empties and swap min/max stages)
 *
 * @param player Board.player to evaluate.
 * @param opponent Board.opponent to evaluate.
 * @param alpha Alpha bound.
 * @param x1 First empty square coordinate.
 * @param x2 Second empty square coordinate.
 * @param n_nodes Node counter.
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
 * @return The final min score, as a disc difference.
 */
<<<<<<< HEAD
static int board_solve_2(unsigned long long player, unsigned long long opponent, int alpha, int x1, int x2, volatile unsigned long long *n_nodes)
{
	unsigned long long flipped;
	int score, bestscore, nodes;
	// const int beta = alpha + 1;

	SEARCH_STATS(++statistics.n_board_solve_2);

	if ((NEIGHBOUR[x1] & opponent) && (flipped = Flip(x1, player, opponent))) {	// (84%/84%)
		bestscore = board_score_1(opponent ^ flipped, alpha, x2);

		if ((bestscore > alpha) && (NEIGHBOUR[x2] & opponent) && (flipped = Flip(x2, player, opponent))) {	// (50%/93%/92%)
			score = board_score_1(opponent ^ flipped, alpha, x1);
			if (score < bestscore)
				bestscore = score;
			nodes = 3;
		} else	nodes = 2;

	} else if ((NEIGHBOUR[x2] & opponent) && (flipped = Flip(x2, player, opponent))) {	// (96%/75%)
		bestscore = board_score_1(opponent ^ flipped, alpha, x1);
		nodes = 2;

	} else {	// pass (17%) - NEIGHBOUR test is almost 100% true
		alpha = ~alpha;	// = -alpha - 1
		if ((flipped = Flip(x1, opponent, player))) {	// (95%)
			bestscore = board_score_1(player ^ flipped, alpha, x2);

			if ((bestscore > alpha) && (flipped = Flip(x2, opponent, player))) {	// (20%/100%)
				score = board_score_1(player ^ flipped, alpha, x1);
				if (score < bestscore)
					bestscore = score;
				nodes = 3;
			} else	nodes = 2;

		} else if ((flipped = Flip(x2, opponent, player))) {	// (97%)
			bestscore = board_score_1(player ^ flipped, alpha, x1);
			nodes = 2;

		} else {	// gameover
			bestscore = board_solve(player, 2);
			nodes = 1;
=======
static int board_solve_2(Board *board, int alpha, const int x1, const int x2, volatile unsigned long long *n_nodes)
=======
 * @return The final score, as a disc difference.
=======
 * @return The final max score, as a disc difference.
>>>>>>> c8118a8 (Use minimax instead of nagamax for solve 4 or less)
=======
 * @return The final min score, as a disc difference.
>>>>>>> 266ad5a (minimax from 5 empties and swap min/max stages)
 */
static int board_solve_2(unsigned long long player, unsigned long long opponent, int alpha, int x1, int x2, volatile unsigned long long *n_nodes)
>>>>>>> 23e04d1 (Backport endgame_sse optimizations into endgame.c)
{
	unsigned long long flipped;
	int score, bestscore, nodes;
	// const int beta = alpha + 1;

	SEARCH_STATS(++statistics.n_board_solve_2);

	if ((NEIGHBOUR[x1] & opponent) && (flipped = Flip(x1, player, opponent))) {	// (84%/84%)
		bestscore = board_score_1(opponent ^ flipped, alpha, x2);

		if ((bestscore > alpha) && (NEIGHBOUR[x2] & opponent) && (flipped = Flip(x2, player, opponent))) {	// (50%/93%/92%)
			score = board_score_1(opponent ^ flipped, alpha, x1);
			if (score < bestscore)
				bestscore = score;
			nodes = 3;
		} else	nodes = 2;

	} else if ((NEIGHBOUR[x2] & opponent) && (flipped = Flip(x2, player, opponent))) {	// (96%/75%)
		bestscore = board_score_1(opponent ^ flipped, alpha, x1);
		nodes = 2;

	} else {	// pass (17%) - NEIGHBOUR test is almost 100% true
		alpha = ~alpha;	// = -alpha - 1
		if ((flipped = Flip(x1, opponent, player))) {	// (95%)
			bestscore = board_score_1(player ^ flipped, alpha, x2);

			if ((bestscore > alpha) && (flipped = Flip(x2, opponent, player))) {	// (20%/100%)
				score = board_score_1(player ^ flipped, alpha, x1);
				if (score < bestscore)
					bestscore = score;
				nodes = 3;
			} else	nodes = 2;

<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
			if (bestscore > alpha) {
				if ((NEIGHBOUR[x2] & board->player) && board_pass_next(board, x2, &next)) {
					SEARCH_UPDATE_INTERNAL_NODES(nodes);
					score = -board_score_1(&next, -alpha, x1);
					if (score < bestscore) bestscore = score;
				}
				// gameover
				if (bestscore == SCORE_INF) bestscore = board_solve(board, 2);
			}
>>>>>>> 1b29848 (fix & optimize 32 bit build; other minor mods)
=======
		} else if ((NEIGHBOUR[x2] & board->player) && board_pass_next(board, x2, &next)) {
			bestscore = -board_score_1(&next, -alpha, x1);
=======
		} else if ((NEIGHBOUR[x2] & board->player) && (flipped = Flip(x2, board->opponent, board->player))) {
			bestscore = -board_score_1(board->player ^ flipped, -alpha, x1);
>>>>>>> 26dad03 (Use player bits only in board_score_1)
=======
		} else if ((NEIGHBOUR[x2] & player) && (flipped = Flip(x2, opponent, player))) {
=======
		} else if ((flipped = Flip(x2, opponent, player))) {	// (97%)
<<<<<<< HEAD
>>>>>>> 9f982ee (Revise PASS handling; prioritymoves in shallow; optimize Neighbour test)
			bestscore = -board_score_1(player ^ flipped, -alpha, x1);
>>>>>>> 23e04d1 (Backport endgame_sse optimizations into endgame.c)
=======
			bestscore = board_score_1(player ^ flipped, alpha, x1);
>>>>>>> 9ec6e5d (Negative score in endgame solve 2/3/4; offset beta in score_1)
			nodes = 2;

		} else {	// gameover
			bestscore = board_solve(player, 2);
			nodes = 1;
>>>>>>> 46e4b64 (Optimize endgame (esp. 2 empties) score comparisons)
		}
		bestscore = -bestscore;
	}

<<<<<<< HEAD
<<<<<<< HEAD
	SEARCH_UPDATE_2EMPTIES_NODES(*n_nodes += nodes;)
	assert(SCORE_MIN <= bestscore && bestscore <= SCORE_MAX);
	assert((bestscore & 1) == 0);
=======
	*n_nodes += nodes;
 	assert(SCORE_MIN <= bestscore && bestscore <= SCORE_MAX);
 	assert((bestscore & 1) == 0);
>>>>>>> 6506166 (More SSE optimizations)
=======
	SEARCH_UPDATE_2EMPTIES_NODES(*n_nodes += nodes;)
	assert(SCORE_MIN <= bestscore && bestscore <= SCORE_MAX);
	assert((bestscore & 1) == 0);
>>>>>>> 46e4b64 (Optimize endgame (esp. 2 empties) score comparisons)
	return bestscore;
}

/**
 * @brief Get the final score.
 *
<<<<<<< HEAD
<<<<<<< HEAD
 * Get the final max score, when 3 empty squares remain.
=======
 * Get the final min score, when 3 empty squares remain.
>>>>>>> c8118a8 (Use minimax instead of nagamax for solve 4 or less)
=======
 * Get the final max score, when 3 empty squares remain.
>>>>>>> 266ad5a (minimax from 5 empties and swap min/max stages)
 *
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
 * @param player Board.player to evaluate.
 * @param opponent Board.opponent to evaluate.
=======
 * @param board The board to evaluate.
>>>>>>> 23e04d1 (Backport endgame_sse optimizations into endgame.c)
=======
 * @param board The board to evaluate. (may be broken)
>>>>>>> 9f982ee (Revise PASS handling; prioritymoves in shallow; optimize Neighbour test)
=======
 * @param player Board.player to evaluate.
 * @param opponent Board.opponent to evaluate.
>>>>>>> 92a4ad9 (Expand board to 2 ULLs in non-SSE search_solve_3 and _4)
 * @param alpha Alpha bound.
 * @param sort3 Parity flags.
 * @param x1 First empty square coordinate.
 * @param x2 Second empty square coordinate.
 * @param x3 Third empty square coordinate.
 * @param n_nodes Node counter.
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
 * @return The final max score, as a disc difference.
 */
<<<<<<< HEAD
static int search_solve_3(unsigned long long player, unsigned long long opponent, int alpha, int sort3, int x1, int x2, int x3, volatile unsigned long long *n_nodes)
{
<<<<<<< HEAD
	unsigned long long flipped, next_player, next_opponent;
	int score, bestscore, pol, tmp;
	// const int beta = alpha + 1;

	SEARCH_STATS(++statistics.n_search_solve_3);
	SEARCH_UPDATE_INTERNAL_NODES(*n_nodes);
=======
	Board *board = search->board;
=======
static int search_solve_3(Search *search, const int alpha, Board *board, unsigned int parity)
{
<<<<<<< HEAD
>>>>>>> 6506166 (More SSE optimizations)
	Board next[1];
=======
	Board next;
<<<<<<< HEAD
>>>>>>> 0a166fd (Remove 1 element array coding style)
	SquareList *empty = search->empties->next;
	int x1 = empty->x;
	int x2 = (empty = empty->next)->x;
	int x3 = empty->next->x;
=======
	int x1 = search->empties[NOMOVE].next;
	int x2 = search->empties[x1].next;
	int x3 = search->empties[x2].next;
>>>>>>> 5e86fd6 (Change pointer-linked empty list to index-linked)
	int score, bestscore;
	// const int beta = alpha + 1;

	SEARCH_STATS(++statistics.n_search_solve_3);
	SEARCH_UPDATE_INTERNAL_NODES(search->n_nodes);
>>>>>>> 1b29848 (fix & optimize 32 bit build; other minor mods)

	// parity based move sorting
<<<<<<< HEAD
=======
 * @return The final score, as a disc difference.
=======
 * @return The final min score, as a disc difference.
>>>>>>> c8118a8 (Use minimax instead of nagamax for solve 4 or less)
=======
 * @return The final max score, as a disc difference.
>>>>>>> 266ad5a (minimax from 5 empties and swap min/max stages)
 */
static int search_solve_3(unsigned long long player, unsigned long long opponent, int alpha, int sort3, int x1, int x2, int x3, volatile unsigned long long *n_nodes)
{
	unsigned long long flipped, next_player, next_opponent;
	int score, bestscore, pol, tmp;
	// const int beta = alpha + 1;

	SEARCH_STATS(++statistics.n_search_solve_3);
	SEARCH_UPDATE_INTERNAL_NODES(*n_nodes);

	// parity based move sorting
>>>>>>> 23e04d1 (Backport endgame_sse optimizations into endgame.c)
	switch (sort3 & 0x03) {
		case 1:
			tmp = x1; x1 = x2; x2 = tmp;	// case 1(x2) 2(x1 x3)
			break;
		case 2:
			tmp = x1; x1 = x3; x3 = x2; x2 = tmp;	// case 1(x3) 2(x1 x2)
			break;
		case 3:
			tmp = x2; x2 = x3; x3 = tmp;
			break;
<<<<<<< HEAD
=======
	if (!(parity & QUADRANT_ID[x1])) {
		if (parity & QUADRANT_ID[x2]) { // case 1(x2) 2(x1 x3)
			int tmp = x1; x1 = x2; x2 = tmp;
		} else { // case 1(x3) 2(x1 x2)
			int tmp = x1; x1 = x3; x3 = x2; x2 = tmp;
		}
>>>>>>> 6506166 (More SSE optimizations)
=======
>>>>>>> 23e04d1 (Backport endgame_sse optimizations into endgame.c)
	}

<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
	bestscore = -SCORE_INF;
	pol = 1;
	do {
		// best move alphabeta search
		if ((NEIGHBOUR[x1] & opponent) && (flipped = Flip(x1, player, opponent))) {	// (89%/91%)
			next_player = opponent ^ flipped;
			next_opponent = player ^ (flipped | x_to_bit(x1));
			bestscore = board_solve_2(next_player, next_opponent, alpha, x2, x3, n_nodes);
			if (bestscore > alpha) return bestscore * pol;	// (78%/63%)
		}

		if (/* (NEIGHBOUR[x2] & opponent) && */ (flipped = Flip(x2, player, opponent))) {	// (97%/78%)
			next_player = opponent ^ flipped;
			next_opponent = player ^ (flipped | x_to_bit(x2));
			score = board_solve_2(next_player, next_opponent, alpha, x1, x3, n_nodes);
			if (score > alpha) return score * pol;	// (32%/9%)
			else if (score > bestscore) bestscore = score;
=======
	// best move alphabeta search
	bestscore = -SCORE_INF;
	if ((NEIGHBOUR[x1] & board->opponent) && (flipped = board_flip(board, x1))) {
		next_player = board->opponent ^ flipped;
		next_opponent = board->player ^ (flipped | x_to_bit(x1));
		bestscore = -board_solve_2(next_player, next_opponent, -(alpha + 1), x2, x3, n_nodes);
		if (bestscore > alpha) return bestscore;
	}

	if ((NEIGHBOUR[x2] & board->opponent) && (flipped = board_flip(board, x2))) {
		next_player = board->opponent ^ flipped;
		next_opponent = board->player ^ (flipped | x_to_bit(x2));
		score = -board_solve_2(next_player, next_opponent, -(alpha + 1), x1, x3, n_nodes);
		if (score > alpha) return score;
		else if (score > bestscore) bestscore = score;
	}

	if ((NEIGHBOUR[x3] & board->opponent) && (flipped = board_flip(board, x3))) {
		next_player = board->opponent ^ flipped;
		next_opponent = board->player ^ (flipped | x_to_bit(x3));
		score = -board_solve_2(next_player, next_opponent, -(alpha + 1), x1, x2, n_nodes);
		if (score > bestscore) bestscore = score;
	}

	// pass ?
	else if (bestscore == -SCORE_INF) {
=======
	for (pol = 1; pol >= -1; pol -= 2) {
>>>>>>> 9f982ee (Revise PASS handling; prioritymoves in shallow; optimize Neighbour test)
=======
	pol = 1;
	do {
>>>>>>> c0fb778 (small optimizations in endgame)
		// best move alphabeta search
		bestscore = -SCORE_INF;
=======
	pol = -1;
	do {
		// best move alphabeta search
		alpha = ~alpha;	// = -(alpha + 1)
		bestscore = SCORE_INF;	// Negative score
>>>>>>> 9ec6e5d (Negative score in endgame solve 2/3/4; offset beta in score_1)
=======
	bestscore = SCORE_INF;	// min stage
=======
	bestscore = -SCORE_INF;
>>>>>>> 266ad5a (minimax from 5 empties and swap min/max stages)
	pol = 1;
	do {
		// best move alphabeta search
>>>>>>> c8118a8 (Use minimax instead of nagamax for solve 4 or less)
		if ((NEIGHBOUR[x1] & opponent) && (flipped = Flip(x1, player, opponent))) {	// (89%/91%)
			next_player = opponent ^ flipped;
			next_opponent = player ^ (flipped | x_to_bit(x1));
			bestscore = board_solve_2(next_player, next_opponent, alpha, x2, x3, n_nodes);
			if (bestscore > alpha) return bestscore * pol;	// (78%/63%)
		}

		if (/* (NEIGHBOUR[x2] & opponent) && */ (flipped = Flip(x2, player, opponent))) {	// (97%/78%)
			next_player = opponent ^ flipped;
			next_opponent = player ^ (flipped | x_to_bit(x2));
			score = board_solve_2(next_player, next_opponent, alpha, x1, x3, n_nodes);
			if (score > alpha) return score * pol;	// (32%/9%)
			else if (score > bestscore) bestscore = score;
		}

<<<<<<< HEAD
<<<<<<< HEAD
		if ((NEIGHBOUR[x3] & board->player) && (flipped = Flip(x3, board->opponent, board->player))) {
			next_player = board->player ^ flipped;
			next_opponent = board->opponent ^ (flipped | x_to_bit(x3));
			score = board_solve_2(next_player, next_opponent, alpha, x1, x2, n_nodes);
			if (score < bestscore) bestscore = score;
>>>>>>> 1b29848 (fix & optimize 32 bit build; other minor mods)
		}

<<<<<<< HEAD
		if (/* (NEIGHBOUR[x3] & opponent) && */ (flipped = Flip(x3, player, opponent))) {	// (100%/89%)
			next_player = opponent ^ flipped;
			next_opponent = player ^ (flipped | x_to_bit(x3));
			score = board_solve_2(next_player, next_opponent, alpha, x1, x2, n_nodes);
<<<<<<< HEAD
<<<<<<< HEAD
			if (score > bestscore) bestscore = score;
			return bestscore * pol;	// (26%)
		}
=======
		else if (bestscore == SCORE_INF)	// gameover
			bestscore = board_solve(board->player, 3);
=======
		if (/* (NEIGHBOUR[x3] & board->opponent) && */ (flipped = board_flip(board, x3))) {	// (100%/89%)
			next_player = board->opponent ^ flipped;
			next_opponent = board->player ^ (flipped | x_to_bit(x3));
=======
		if (/* (NEIGHBOUR[x3] & opponent) && */ (flipped = Flip(x3, player, opponent))) {	// (100%/89%)
			next_player = opponent ^ flipped;
			next_opponent = player ^ (flipped | x_to_bit(x3));
>>>>>>> 92a4ad9 (Expand board to 2 ULLs in non-SSE search_solve_3 and _4)
			score = -board_solve_2(next_player, next_opponent, ~alpha, x1, x2, n_nodes);
			if (score > bestscore) bestscore = score;
=======
			if (score < bestscore) bestscore = score;
>>>>>>> 9ec6e5d (Negative score in endgame solve 2/3/4; offset beta in score_1)
=======
			if (score > bestscore) bestscore = score;
>>>>>>> 266ad5a (minimax from 5 empties and swap min/max stages)
			return bestscore * pol;	// (26%)
		}

		if (bestscore > -SCORE_INF)	// (76%)
			return bestscore * pol;	// (9%)

<<<<<<< HEAD
		flipped = player; player = opponent; opponent = flipped;
		alpha = ~alpha;	// = -(alpha + 1)
<<<<<<< HEAD
>>>>>>> 9f982ee (Revise PASS handling; prioritymoves in shallow; optimize Neighbour test)
	}
>>>>>>> 46e4b64 (Optimize endgame (esp. 2 empties) score comparisons)

<<<<<<< HEAD
<<<<<<< HEAD
		if (bestscore > -SCORE_INF)	// (76%)
			return bestscore * pol;	// (9%)

		next_opponent = player; player = opponent; opponent = next_opponent;	// pass
		alpha = ~alpha;	// = -(alpha + 1)
=======
>>>>>>> c0fb778 (small optimizations in endgame)
	} while ((pol = -pol) < 0);
=======
		next_opponent = player; player = opponent; opponent = next_opponent;	// pass
<<<<<<< HEAD
	} while ((pol = -pol) >= 0);
>>>>>>> 9ec6e5d (Negative score in endgame solve 2/3/4; offset beta in score_1)

	return board_solve(player, 3);	// gameover
=======
	return board_solve(board->player, 3);	// gameover
>>>>>>> 9f982ee (Revise PASS handling; prioritymoves in shallow; optimize Neighbour test)
=======
	return board_solve(player, 3);	// gameover
>>>>>>> 92a4ad9 (Expand board to 2 ULLs in non-SSE search_solve_3 and _4)
=======
		alpha = ~alpha;	// = -(alpha + 1)
	} while ((pol = -pol) < 0);

<<<<<<< HEAD
	return board_solve(opponent, 3);	// gameover
>>>>>>> c8118a8 (Use minimax instead of nagamax for solve 4 or less)
=======
	return board_solve(player, 3);	// gameover
>>>>>>> 266ad5a (minimax from 5 empties and swap min/max stages)
}

/**
 * @brief Get the final score.
 *
<<<<<<< HEAD
<<<<<<< HEAD
 * Get the final min score, when 4 empty squares remain.
 *
 * @param search Search position.
 * @param alpha Upper score value.
 * @return The final min score, as a disc difference.
=======
 * Get the final max score, when 4 empty squares remain.
 *
 * @param search Search position.
 * @param alpha Upper score value.
 * @return The final max score, as a disc difference.
>>>>>>> c8118a8 (Use minimax instead of nagamax for solve 4 or less)
=======
 * Get the final min score, when 4 empty squares remain.
 *
 * @param search Search position.
 * @param alpha Upper score value.
 * @return The final min score, as a disc difference.
>>>>>>> 266ad5a (minimax from 5 empties and swap min/max stages)
 */
static int search_solve_4(Search *search, int alpha)
{
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
	unsigned long long player, opponent, flipped, next_player, next_opponent;
	int x1, x2, x3, x4, tmp, paritysort, score, bestscore, pol;
	// const int beta = alpha + 1;
	static const unsigned char parity_case[64] = {	/* x4x3x2x1 = */
		/*0000*/  0, /*0001*/  0, /*0010*/  1, /*0011*/  9, /*0100*/  2, /*0101*/ 10, /*0110*/ 11, /*0111*/  3,
		/*0002*/  0, /*0003*/  0, /*0012*/  0, /*0013*/  0, /*0102*/  4, /*0103*/  4, /*0112*/  5, /*0113*/  5,
		/*0020*/  1, /*0021*/  0, /*0030*/  1, /*0031*/  0, /*0120*/  6, /*0121*/  7, /*0130*/  6, /*0131*/  7,
		/*0022*/  9, /*0023*/  0, /*0032*/  0, /*0033*/  9, /*0122*/  8, /*0123*/  0, /*0132*/  0, /*0133*/  8,
		/*0200*/  2, /*0201*/  4, /*0210*/  6, /*0211*/  8, /*0300*/  2, /*0301*/  4, /*0310*/  6, /*0311*/  8,
		/*0202*/ 10, /*0203*/  4, /*0212*/  7, /*0213*/  0, /*0302*/  4, /*0303*/ 10, /*0312*/  0, /*0313*/  7,
		/*0220*/ 11, /*0221*/  5, /*0230*/  6, /*0231*/  0, /*0320*/  6, /*0321*/  0, /*0330*/ 11, /*0331*/  5,
		/*0222*/  3, /*0223*/  5, /*0232*/  7, /*0233*/  8, /*0322*/  8, /*0323*/  7, /*0332*/  5, /*0333*/  3
	};
	int sort3;	// for move sorting on 3 empties
	static const short sort3_shuf[] = {
		0x0000,	//  0: 1(x1) 3(x2 x3 x4), 1(x1) 1(x2) 2(x3 x4), 1 1 1 1, 4		x4x1x2x3-x3x1x2x4-x2x1x3x4-x1x2x3x4
		0x1100,	//  1: 1(x2) 3(x1 x3 x4)	x4x2x1x3-x3x2x1x4-x2x1x3x4-x1x2x3x4
		0x2011,	//  2: 1(x3) 3(x1 x2 x4)	x4x3x1x2-x3x1x2x4-x2x3x1x4-x1x3x2x4
		0x0222,	//  3: 1(x4) 3(x1 x2 x3)	x4x1x2x3-x3x4x1x2-x2x4x1x3-x1x4x2x3
		0x3000,	//  4: 1(x1) 1(x3) 2(x2 x4)	x4x1x2x3-x2x1x3x4-x3x1x2x4-x1x3x2x4 <- x4x1x3x2-x2x1x3x4-x3x1x2x4-x1x3x2x4
		0x3300,	//  5: 1(x1) 1(x4) 2(x2 x3)	x3x1x2x4-x2x1x3x4-x4x1x2x3-x1x4x2x3 <- x3x1x4x2-x2x1x4x3-x4x1x2x3-x1x4x2x3
		0x2000,	//  6: 1(x2) 1(x3) 2(x1 x4)	x4x1x2x3-x1x2x3x4-x3x2x1x4-x2x3x1x4 <- x4x2x3x1-x1x2x3x4-x3x2x1x4-x2x3x1x4
		0x2300,	//  7: 1(x2) 1(x4) 2(x1 x3)	x3x1x2x4-x1x2x3x4-x4x2x1x3-x2x4x1x3 <- x3x2x4x1-x1x2x4x3-x4x2x1x3-x2x4x1x3
		0x2200,	//  8: 1(x3) 1(x4) 2(x1 x2)	x2x1x3x4-x1x2x3x4-x4x3x1x2-x3x4x1x2 <- x2x3x4x1-x1x3x4x2-x4x3x1x2-x3x4x1x2
		0x2200,	//  9: 2(x1 x2) 2(x3 x4)	x4x3x1x2-x3x4x1x2-x2x1x3x4-x1x2x3x4
		0x1021,	// 10: 2(x1 x3) 2(x2 x4)	x4x2x1x3-x3x1x2x4-x2x4x1x3-x1x3x2x4
		0x0112	// 11: 2(x1 x4) 2(x2 x3)	x4x1x2x3-x3x2x1x4-x2x3x1x4-x1x4x2x3
	};
=======
	Board *board;
	Board next[1];
=======
	Board next;
<<<<<<< HEAD
<<<<<<< HEAD
>>>>>>> 0a166fd (Remove 1 element array coding style)
	SquareList *empty;
=======
>>>>>>> 5e86fd6 (Change pointer-linked empty list to index-linked)
	int x1, x2, x3, x4;
	int score, bestscore;
<<<<<<< HEAD
	const int beta = alpha + 1;
>>>>>>> 1b29848 (fix & optimize 32 bit build; other minor mods)
=======
	unsigned int parity;
	// const int beta = alpha + 1;
>>>>>>> 6506166 (More SSE optimizations)
=======
=======
	Board board0, next;
>>>>>>> 9f982ee (Revise PASS handling; prioritymoves in shallow; optimize Neighbour test)
	unsigned long long flipped;
=======
	unsigned long long player, opponent, flipped, next_player, next_opponent;
>>>>>>> 92a4ad9 (Expand board to 2 ULLs in non-SSE search_solve_3 and _4)
	int x1, x2, x3, x4, tmp, paritysort, score, bestscore, pol;
	// const int beta = alpha + 1;
	static const unsigned char parity_case[64] = {	/* x4x3x2x1 = */
		/*0000*/  0, /*0001*/  0, /*0010*/  1, /*0011*/  9, /*0100*/  2, /*0101*/ 10, /*0110*/ 11, /*0111*/  3,
		/*0002*/  0, /*0003*/  0, /*0012*/  0, /*0013*/  0, /*0102*/  4, /*0103*/  4, /*0112*/  5, /*0113*/  5,
		/*0020*/  1, /*0021*/  0, /*0030*/  1, /*0031*/  0, /*0120*/  6, /*0121*/  7, /*0130*/  6, /*0131*/  7,
		/*0022*/  9, /*0023*/  0, /*0032*/  0, /*0033*/  9, /*0122*/  8, /*0123*/  0, /*0132*/  0, /*0133*/  8,
		/*0200*/  2, /*0201*/  4, /*0210*/  6, /*0211*/  8, /*0300*/  2, /*0301*/  4, /*0310*/  6, /*0311*/  8,
		/*0202*/ 10, /*0203*/  4, /*0212*/  7, /*0213*/  0, /*0302*/  4, /*0303*/ 10, /*0312*/  0, /*0313*/  7,
		/*0220*/ 11, /*0221*/  5, /*0230*/  6, /*0231*/  0, /*0320*/  6, /*0321*/  0, /*0330*/ 11, /*0331*/  5,
		/*0222*/  3, /*0223*/  5, /*0232*/  7, /*0233*/  8, /*0322*/  8, /*0323*/  7, /*0332*/  5, /*0333*/  3
	};
	int sort3;	// for move sorting on 3 empties
	static const short sort3_shuf[] = {
		0x0000,	//  0: 1(x1) 3(x2 x3 x4), 1(x1) 1(x2) 2(x3 x4), 1 1 1 1, 4		x4x1x2x3-x3x1x2x4-x2x1x3x4-x1x2x3x4
		0x1100,	//  1: 1(x2) 3(x1 x3 x4)	x4x2x1x3-x3x2x1x4-x2x1x3x4-x1x2x3x4
		0x2011,	//  2: 1(x3) 3(x1 x2 x4)	x4x3x1x2-x3x1x2x4-x2x3x1x4-x1x3x2x4
		0x0222,	//  3: 1(x4) 3(x1 x2 x3)	x4x1x2x3-x3x4x1x2-x2x4x1x3-x1x4x2x3
		0x3000,	//  4: 1(x1) 1(x3) 2(x2 x4)	x4x1x2x3-x2x1x3x4-x3x1x2x4-x1x3x2x4 <- x4x1x3x2-x2x1x3x4-x3x1x2x4-x1x3x2x4
		0x3300,	//  5: 1(x1) 1(x4) 2(x2 x3)	x3x1x2x4-x2x1x3x4-x4x1x2x3-x1x4x2x3 <- x3x1x4x2-x2x1x4x3-x4x1x2x3-x1x4x2x3
		0x2000,	//  6: 1(x2) 1(x3) 2(x1 x4)	x4x1x2x3-x1x2x3x4-x3x2x1x4-x2x3x1x4 <- x4x2x3x1-x1x2x3x4-x3x2x1x4-x2x3x1x4
		0x2300,	//  7: 1(x2) 1(x4) 2(x1 x3)	x3x1x2x4-x1x2x3x4-x4x2x1x3-x2x4x1x3 <- x3x2x4x1-x1x2x4x3-x4x2x1x3-x2x4x1x3
		0x2200,	//  8: 1(x3) 1(x4) 2(x1 x2)	x2x1x3x4-x1x2x3x4-x4x3x1x2-x3x4x1x2 <- x2x3x4x1-x1x3x4x2-x4x3x1x2-x3x4x1x2
		0x2200,	//  9: 2(x1 x2) 2(x3 x4)	x4x3x1x2-x3x4x1x2-x2x1x3x4-x1x2x3x4
		0x1021,	// 10: 2(x1 x3) 2(x2 x4)	x4x2x1x3-x3x1x2x4-x2x4x1x3-x1x3x2x4
		0x0112	// 11: 2(x1 x4) 2(x2 x3)	x4x1x2x3-x3x2x1x4-x2x3x1x4-x1x4x2x3
	};
>>>>>>> 23e04d1 (Backport endgame_sse optimizations into endgame.c)

	SEARCH_STATS(++statistics.n_search_solve_4);
	SEARCH_UPDATE_INTERNAL_NODES(search->n_nodes);

	// stability cutoff (try 12%, cut 7%)
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
	if (search_SC_NWS_4(search, alpha, &score)) return score;

	x1 = search->empties[NOMOVE].next;
	x2 = search->empties[x1].next;
	x3 = search->empties[x2].next;
	x4 = search->empties[x3].next;
=======
	if (search_SC_NWS(search, alpha, &score)) return score;
>>>>>>> 9f982ee (Revise PASS handling; prioritymoves in shallow; optimize Neighbour test)
=======
	if (search_SC_NWS(search, alpha, 4, &score)) return score;
>>>>>>> bb98132 (Split 5 empties search_shallow loop; tune stabiliby cutoff)
=======
	if (search_SC_NWS_4(search, alpha, &score)) return score;
>>>>>>> 266ad5a (minimax from 5 empties and swap min/max stages)

	x1 = search->empties[NOMOVE].next;
	x2 = search->empties[x1].next;
	x3 = search->empties[x2].next;
	x4 = search->empties[x3].next;

	// parity based move sorting.
	// The following hole sizes are possible:
	//    4 - 1 3 - 2 2 - 1 1 2 - 1 1 1 1
<<<<<<< HEAD
<<<<<<< HEAD
=======
>>>>>>> 23e04d1 (Backport endgame_sse optimizations into endgame.c)
	// Only the 1 1 2 case needs move sorting on this ply.
	paritysort = parity_case[((x3 ^ x4) & 0x24) + ((((x2 ^ x4) & 0x24) * 2 + ((x1 ^ x4) & 0x24)) >> 2)];
	switch (paritysort) {
		case 4: // case 1(x1) 1(x3) 2(x2 x4)
			tmp = x2; x2 = x3; x3 = tmp;
			break;
		case 5: // case 1(x1) 1(x4) 2(x2 x3)
			tmp = x2; x2 = x4; x4 = x3; x3 = tmp;
			break;
		case 6:	// case 1(x2) 1(x3) 2(x1 x4)
			tmp = x1; x1 = x2; x2 = x3; x3 = tmp;
			break;
		case 7: // case 1(x2) 1(x4) 2(x1 x3)
			tmp = x1; x1 = x2; x2 = x4; x4 = x3; x3 = tmp;
			break;
		case 8:	// case 1(x3) 1(x4) 2(x1 x2)
			tmp = x1; x1 = x3; x3 = tmp; tmp = x2; x2 = x4; x4 = tmp;
			break;
<<<<<<< HEAD
=======
	// Only the 1 1 2 case needs move sorting.
	parity = search->eval.parity;
	if (!(parity & QUADRANT_ID[x1])) {
		if (parity & QUADRANT_ID[x2]) {
			if (parity & QUADRANT_ID[x3]) { // case 1(x2) 1(x3) 2(x1 x4)
				int tmp = x1; x1 = x2; x2 = x3; x3 = tmp;
			} else { // case 1(x2) 1(x4) 2(x1 x3)
				int tmp = x1; x1 = x2; x2 = x4; x4 = x3; x3 = tmp;
			}
		} else if (parity & QUADRANT_ID[x3]) { // case 1(x3) 1(x4) 2(x1 x2)
			int tmp = x1; x1 = x3; x3 = tmp; tmp = x2; x2 = x4; x4 = tmp;
		}
	} else {
		if (!(parity & QUADRANT_ID[x2])) {
			if (parity & QUADRANT_ID[x3]) { // case 1(x1) 1(x3) 2(x2 x4)
				int tmp = x2; x2 = x3; x3 = tmp;
			} else { // case 1(x1) 1(x4) 2(x2 x3)
				int tmp = x2; x2 = x4; x4 = x3; x3 = tmp;
			}
		}
>>>>>>> 6506166 (More SSE optimizations)
=======
>>>>>>> 23e04d1 (Backport endgame_sse optimizations into endgame.c)
	}
	sort3 = sort3_shuf[paritysort];

<<<<<<< HEAD
<<<<<<< HEAD
	player = search->board.player;
	opponent = search->board.opponent;
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
	bestscore = SCORE_INF;	// min stage
	pol = 1;
	do {
		// best move alphabeta search
		if ((NEIGHBOUR[x1] & opponent) && (flipped = Flip(x1, player, opponent))) {	// (76%/77%)
			next_player = opponent ^ flipped;
			next_opponent = player ^ (flipped | x_to_bit(x1));
			bestscore = search_solve_3(next_player, next_opponent, alpha, sort3, x2, x3, x4, &search->n_nodes);
			if (bestscore <= alpha) return bestscore * pol;	// (68%)
		}

		if ((NEIGHBOUR[x2] & opponent) && (flipped = Flip(x2, player, opponent))) {	// (87%/84%)
			next_player = opponent ^ flipped;
			next_opponent = player ^ (flipped | x_to_bit(x2));
			score = search_solve_3(next_player, next_opponent, alpha, sort3 >> 4, x1, x3, x4, &search->n_nodes);
			if (score <= alpha) return score * pol;	// (37%)
			else if (score < bestscore) bestscore = score;
		}

<<<<<<< HEAD
		if ((NEIGHBOUR[x3] & opponent) && (flipped = Flip(x3, player, opponent))) {	// (77%/80%)
			next_player = opponent ^ flipped;
			next_opponent = player ^ (flipped | x_to_bit(x3));
			score = search_solve_3(next_player, next_opponent, alpha, sort3 >> 8, x1, x2, x4, &search->n_nodes);
			if (score <= alpha) return score * pol;	// (14%)
			else if (score < bestscore) bestscore = score;
=======
	// best move alphabeta search
	bestscore = -SCORE_INF;
	if ((NEIGHBOUR[x1] & search->board.opponent) && (flipped = board_flip(&search->board, x1))) {
		board_flip_next(&search->board, x1, flipped, &next);
		bestscore = -search_solve_3(&next, -(alpha + 1), sort3, x2, x3, x4, &search->n_nodes);
		if (bestscore > alpha) return bestscore;
	}

	if ((NEIGHBOUR[x2] & search->board.opponent) && (flipped = board_flip(&search->board, x2))) {
		board_flip_next(&search->board, x2, flipped, &next);
		score = -search_solve_3(&next, -(alpha + 1), sort3 >> 4, x1, x3, x4, &search->n_nodes);
		if (score > alpha) return score;
		else if (score > bestscore) bestscore = score;
	}

	if ((NEIGHBOUR[x3] & search->board.opponent) && (flipped = board_flip(&search->board, x3))) {
		board_flip_next(&search->board, x3, flipped, &next);
		score = -search_solve_3(&next, -(alpha + 1), sort3 >> 8, x1, x2, x4, &search->n_nodes);
		if (score > alpha) return score;
		else if (score > bestscore) bestscore = score;
	}

	if ((NEIGHBOUR[x4] & search->board.opponent) && (flipped = board_flip(&search->board, x4))) {
		board_flip_next(&search->board, x4, flipped, &next);
		score = -search_solve_3(&next, -(alpha + 1), sort3 >> 12, x1, x2, x3, &search->n_nodes);
		if (score > bestscore) bestscore = score;
	}

	else if (bestscore == -SCORE_INF) {	// no move
		if (can_move(search->board.opponent, search->board.player)) { // pass
			search_pass_endgame(search);
			bestscore = -search_solve_4(search, -(alpha + 1));
			search_pass_endgame(search);
		} else { // gameover
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
			bestscore = search_solve(search);
>>>>>>> 1b29848 (fix & optimize 32 bit build; other minor mods)
=======
			bestscore = board_solve(board, 4);
>>>>>>> 6506166 (More SSE optimizations)
=======
			bestscore = board_solve(&search->board, 4);
>>>>>>> 0a166fd (Remove 1 element array coding style)
=======
			bestscore = board_solve(search->board.player, 4);
>>>>>>> 26dad03 (Use player bits only in board_score_1)
		}

		if ((NEIGHBOUR[x4] & opponent) && (flipped = Flip(x4, player, opponent))) {	// (79%/88%)
			next_player = opponent ^ flipped;
			next_opponent = player ^ (flipped | x_to_bit(x4));
			score = search_solve_3(next_player, next_opponent, alpha, sort3 >> 12, x1, x2, x3, &search->n_nodes);
			if (score < bestscore) bestscore = score;
			return bestscore * pol;	// (37%)
		}

		if (bestscore < SCORE_INF)	// (72%)
			return bestscore * pol;	// (13%)

		next_opponent = player; player = opponent; opponent = next_opponent;	// pass
		alpha = ~alpha;	// = -(alpha + 1)
	} while ((pol = -pol) < 0);

	return board_solve(opponent, 4);	// gameover
=======
	board0 = search->board;
=======
	player = search->board.player;
	opponent = search->board.opponent;
>>>>>>> 92a4ad9 (Expand board to 2 ULLs in non-SSE search_solve_3 and _4)
	for (pol = 1; pol >= -1; pol -= 2) {
=======
	pol = 1;
	do {
>>>>>>> c0fb778 (small optimizations in endgame)
		// best move alphabeta search
		bestscore = -SCORE_INF;
=======
	pol = -1;
	do {
		// best move alphabeta search
		alpha = ~alpha;	// = -(alpha + 1)
		bestscore = SCORE_INF;	// Negative score
>>>>>>> 9ec6e5d (Negative score in endgame solve 2/3/4; offset beta in score_1)
=======
	bestscore = -SCORE_INF;
=======
	bestscore = SCORE_INF;	// min stage
>>>>>>> 266ad5a (minimax from 5 empties and swap min/max stages)
	pol = 1;
	do {
		// best move alphabeta search
>>>>>>> c8118a8 (Use minimax instead of nagamax for solve 4 or less)
		if ((NEIGHBOUR[x1] & opponent) && (flipped = Flip(x1, player, opponent))) {	// (76%/77%)
			next_player = opponent ^ flipped;
			next_opponent = player ^ (flipped | x_to_bit(x1));
			bestscore = search_solve_3(next_player, next_opponent, alpha, sort3, x2, x3, x4, &search->n_nodes);
			if (bestscore <= alpha) return bestscore * pol;	// (68%)
		}

		if ((NEIGHBOUR[x2] & opponent) && (flipped = Flip(x2, player, opponent))) {	// (87%/84%)
			next_player = opponent ^ flipped;
			next_opponent = player ^ (flipped | x_to_bit(x2));
			score = search_solve_3(next_player, next_opponent, alpha, sort3 >> 4, x1, x3, x4, &search->n_nodes);
			if (score <= alpha) return score * pol;	// (37%)
			else if (score < bestscore) bestscore = score;
		}

		if ((NEIGHBOUR[x3] & opponent) && (flipped = Flip(x3, player, opponent))) {	// (77%/80%)
			next_player = opponent ^ flipped;
			next_opponent = player ^ (flipped | x_to_bit(x3));
			score = search_solve_3(next_player, next_opponent, alpha, sort3 >> 8, x1, x2, x4, &search->n_nodes);
			if (score <= alpha) return score * pol;	// (14%)
			else if (score < bestscore) bestscore = score;
		}

		if ((NEIGHBOUR[x4] & opponent) && (flipped = Flip(x4, player, opponent))) {	// (79%/88%)
			next_player = opponent ^ flipped;
			next_opponent = player ^ (flipped | x_to_bit(x4));
			score = search_solve_3(next_player, next_opponent, alpha, sort3 >> 12, x1, x2, x3, &search->n_nodes);
			if (score < bestscore) bestscore = score;
			return bestscore * pol;	// (37%)
		}

		if (bestscore < SCORE_INF)	// (72%)
			return bestscore * pol;	// (13%)

		next_opponent = player; player = opponent; opponent = next_opponent;	// pass
		alpha = ~alpha;	// = -(alpha + 1)
	} while ((pol = -pol) < 0);

<<<<<<< HEAD
<<<<<<< HEAD
	return board_solve(search->board.player, 4);	// gameover
>>>>>>> 9f982ee (Revise PASS handling; prioritymoves in shallow; optimize Neighbour test)
=======
	return board_solve(player, 4);	// gameover
>>>>>>> 9ec6e5d (Negative score in endgame solve 2/3/4; offset beta in score_1)
=======
	return board_solve(opponent, 4);	// gameover
>>>>>>> 266ad5a (minimax from 5 empties and swap min/max stages)
}
#endif

/**
 * @brief  Evaluate a position using a shallow NWS.
 *
 * This function is used when there are few empty squares on the board. Here,
 * optimizations are in favour of speed instead of efficiency.
 * Move ordering is constricted to the hole parity and the type of squares.
 * No hashtable are used and anticipated cut-off is limited to stability cut-off.
 *
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
 * @param search Search. (breaks board and parity; caller has a copy)
=======
 * @param search Search. (breaks board and parity)
>>>>>>> 867c81c (Omit restore board/parity in search_shallow; tweak NWS_STABILITY)
=======
 * @param search Search. (breaks board and parity; caller take a copy)
>>>>>>> ea8595b (Split v3hi_empties from search_solve_3 & moved to solve_4)
=======
 * @param search Search. (breaks board and parity; caller has a copy)
>>>>>>> 6a63841 (exit search_shallow/search_eval loop when all bits processed)
 * @param alpha Alpha bound.
 * @return The final score, as a disc difference.
 */
static int search_shallow(Search *search, const int alpha, bool pass1)
{
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
	unsigned long long moves, prioritymoves;
	int x, prev, score, bestscore;
=======
	unsigned long long moves;
	int x, prev, score, bestscore = -SCORE_INF;
>>>>>>> 8ee1734 (Use get_moves in search_shallow)
=======
	unsigned long long moves, prioritymoves;
	int x, prev, score, bestscore;
>>>>>>> 9f982ee (Revise PASS handling; prioritymoves in shallow; optimize Neighbour test)
	// const int beta = alpha + 1;
<<<<<<< HEAD
	V2DI board0;
=======
	vBoard board0;
>>>>>>> 8566ed0 (vector call version of board_next & get_moves)
	unsigned int parity0;
=======
	Board *board = search->board;
=======
>>>>>>> 0a166fd (Remove 1 element array coding style)
	SquareList *empty;
=======
>>>>>>> 5e86fd6 (Change pointer-linked empty list to index-linked)
	Move move;
=======
	unsigned long long flipped;
>>>>>>> 9b4cd06 (Optimize search_shallow in endgame.c; revise eval_update parameters)
	int x, score, bestscore = -SCORE_INF;
	// const int beta = alpha + 1;
<<<<<<< HEAD
>>>>>>> 6506166 (More SSE optimizations)
=======
	Board board0;
<<<<<<< HEAD
	unsigned int parity0, paritymask;
>>>>>>> 4b9f204 (minor optimize in search_eval_1/2 and search_shallow)
=======
	unsigned int parity0;
>>>>>>> 9f982ee (Revise PASS handling; prioritymoves in shallow; optimize Neighbour test)

	assert(SCORE_MIN <= alpha && alpha <= SCORE_MAX);
	assert(0 <= search->eval.n_empties && search->eval.n_empties <= DEPTH_TO_SHALLOW_SEARCH);

	SEARCH_STATS(++statistics.n_NWS_shallow);
	SEARCH_UPDATE_INTERNAL_NODES(search->n_nodes);

<<<<<<< HEAD
<<<<<<< HEAD
	// stability cutoff (try 8%, cut 7%)
=======
	// stability cutoff (try 15%, cut 5%)
<<<<<<< HEAD
<<<<<<< HEAD
>>>>>>> 9f982ee (Revise PASS handling; prioritymoves in shallow; optimize Neighbour test)
=======
	// stability cutoff (try 8%, cut 7%)
>>>>>>> 264e827 (calc solid stone only when stability cutoff tried)
	if (search_SC_NWS(search, alpha, &score)) return score;
=======
	if (search_SC_NWS(search, alpha, search->eval.n_empties, &score)) return score;
>>>>>>> bb98132 (Split 5 empties search_shallow loop; tune stabiliby cutoff)
=======
	if (search_SC_NWS(search, alpha, &score)) return score;
>>>>>>> 266ad5a (minimax from 5 empties and swap min/max stages)

<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
	board0.board = search->board;
	moves = vboard_get_moves(board0);
	if (moves == 0) {	// pass (2%)
		if (pass1)	// gameover (1%)
			return search_solve(search);

<<<<<<< HEAD
		search_pass(search);
		bestscore = -search_shallow(search, ~alpha, true);
		// search_pass(search);
		return bestscore;
	}

	bestscore = -SCORE_INF;
	parity0 = search->eval.parity;
	prioritymoves = moves & quadrant_mask[parity0];
	if (prioritymoves == 0)	// all even
		prioritymoves = moves;

	if (search->eval.n_empties == 5)	// transfer to search_solve_n, no longer uses n_empties, parity (53%)
<<<<<<< HEAD
		do {
			moves ^= prioritymoves;
			x = NOMOVE;
			do {
				do {
					x = search->empties[prev = x].next;
				} while (!(prioritymoves & x_to_bit(x)));	// (58%)

				prioritymoves &= ~x_to_bit(x);
				search->empties[prev].next = search->empties[x].next;	// remove - maintain single link only
				vboard_next(board0, x, &search->board);
				score = search_solve_4(search, alpha);
				search->empties[prev].next = x;	// restore

				if (score > alpha)	// (49%)
					return score;
				else if (score > bestscore)
					bestscore = score;
			} while (prioritymoves);	// (34%)
		} while ((prioritymoves = moves));	// (38%)

	else {
		--search->eval.n_empties;	// for next depth
		do {
			moves ^= prioritymoves;
			x = NOMOVE;
			do {
				do {
					x = search->empties[prev = x].next;
				} while (!(prioritymoves & x_to_bit(x)));	// (57%)

				prioritymoves &= ~x_to_bit(x);
				search->eval.parity = parity0 ^ QUADRANT_ID[x];
				search->empties[prev].next = search->empties[x].next;	// remove - maintain single link only
				vboard_next(board0, x, &search->board);
				score = -search_shallow(search, ~alpha, false);
				search->empties[prev].next = x;	// restore

				if (score > alpha) {	// (40%)
					// search->board = board0.board;
					// search->eval.parity = parity0;
					++search->eval.n_empties;
					return score;

				} else if (score > bestscore)
					bestscore = score;
			} while (prioritymoves);	// (54%)
		} while ((prioritymoves = moves));	// (23%)
		++search->eval.n_empties;
=======
		foreach_odd_empty (empty, search->empties, search->parity) {
=======
	if (search->eval.parity > 0 && search->eval.parity < 15) {

		foreach_odd_empty (empty, search->empties, search->eval.parity) {
<<<<<<< HEAD
>>>>>>> f1d221c (Replace eval_restore with simple save-restore, as well as parity)
			if ((NEIGHBOUR[empty->x] & board->opponent)
			&& board_get_move(board, empty->x, &move)) {
=======
			if ((NEIGHBOUR[empty->x] & search->board.opponent)
			&& board_get_move(&search->board, empty->x, &move)) {
>>>>>>> 0a166fd (Remove 1 element array coding style)
				search_update_endgame(search, &move);
					if (search->n_empties == 4) score = -search_solve_4(search, -(alpha + 1));
					else score = -search_shallow(search, -(alpha + 1));
				search_restore_endgame(search, &move);
				if (score > alpha) return score;
				else if (score > bestscore) bestscore = score;
			}
		}
=======
=======
	moves = get_moves(search->board.player, search->board.opponent);
=======
	board0 = load_vboard(search->board);
	moves = vboard_get_moves(board0, search->board);
>>>>>>> 8566ed0 (vector call version of board_next & get_moves)
	if (moves == 0) {	// pass (2%)
		if (pass1)	// gameover
			return search_solve(search);

		search_pass(search);
		bestscore = -search_shallow(search, ~alpha, true);
		// search_pass(search);
		return bestscore;
	}

<<<<<<< HEAD
<<<<<<< HEAD
>>>>>>> 8ee1734 (Use get_moves in search_shallow)
=======
	bestscore  = -SCORE_INF;
<<<<<<< HEAD
>>>>>>> 9f982ee (Revise PASS handling; prioritymoves in shallow; optimize Neighbour test)
	board0 = search->board;
=======
>>>>>>> 8566ed0 (vector call version of board_next & get_moves)
=======
	bestscore = -SCORE_INF;
>>>>>>> 9ea5b5e (BMI2 and mm_LastFlip version of board_score_sse_1 added (but not enabled))
	parity0 = search->eval.parity;
	prioritymoves = moves & quadrant_mask[parity0];
	if (prioritymoves == 0)	// all even
		prioritymoves = moves;
<<<<<<< HEAD
	--search->eval.n_empties;	// for next depth
<<<<<<< HEAD
	do {	// odd first, even second
<<<<<<< HEAD
		if (paritymask) {	// skip all even or all add
			foreach_empty (x, search->empties) {
=======
		if (paritymask) {	// skip no odd or no even
			for (x = search->empties[prev = NOMOVE].next; x != NOMOVE; x = search->empties[prev = x].next) {	// maintain single link only
<<<<<<< HEAD
>>>>>>> be2ba1c (add AVX get_potential_mobility; revise foreach_bit for CPU32/C99)
				if (paritymask & QUADRANT_ID[x]) {
					if ((NEIGHBOUR[x] & board0.opponent) && (flipped = board_flip(&board0, x))) {
						search->eval.parity = parity0 ^ QUADRANT_ID[x];
						empty_remove(search->empties, x);
						search->board.player = board0.opponent ^ flipped;
						search->board.opponent = board0.player ^ (flipped | x_to_bit(x));
						board_check(&search->board);

<<<<<<< HEAD
						if (search->eval.n_empties == 4) score = -search_solve_4(search, -(alpha + 1));
						else score = -search_shallow(search, -(alpha + 1));
>>>>>>> 4b9f204 (minor optimize in search_eval_1/2 and search_shallow)
=======
						if (search->eval.n_empties == 4)
							score = -search_solve_4(search, -(alpha + 1));
						else	score = -search_shallow(search, -(alpha + 1));
>>>>>>> 9b4cd06 (Optimize search_shallow in endgame.c; revise eval_update parameters)

						empty_restore(search->empties, x);
=======
				if ((moves & x_to_bit(x)) && (paritymask & QUADRANT_ID[x])) {
					search->eval.parity = parity0 ^ QUADRANT_ID[x];
					search->empties[prev].next = search->empties[x].next;	// remove
					board_next(&board0, x, &search->board);
=======
	do {
		x = search->empties[prev = NOMOVE].next;	// maintain single link only
		do {
			if (prioritymoves & x_to_bit(x)) {	// (37%)
				search->eval.parity = parity0 ^ QUADRANT_ID[x];
				search->empties[prev].next = search->empties[x].next;	// remove
				board_next(&board0, x, &search->board);
>>>>>>> 9f982ee (Revise PASS handling; prioritymoves in shallow; optimize Neighbour test)

				if (search->eval.n_empties == 4)	// (57%)
=======

	if (search->eval.n_empties == 5)	// transfer to search_solve_n, no longer uses n_empties, parity
=======
>>>>>>> 264e827 (calc solid stone only when stability cutoff tried)
		do {
			moves ^= prioritymoves;
			x = NOMOVE;
			do {
<<<<<<< HEAD
				if (prioritymoves & x_to_bit(x)) {
					search->empties[prev].next = search->empties[x].next;	// remove
<<<<<<< HEAD
					board_next(&board0, x, &search->board);
>>>>>>> bb98132 (Split 5 empties search_shallow loop; tune stabiliby cutoff)
=======
					vboard_next(board0, x, &search->board);
>>>>>>> 8566ed0 (vector call version of board_next & get_moves)
					score = -search_solve_4(search, ~alpha);
					search->empties[prev].next = x;	// restore

<<<<<<< HEAD
<<<<<<< HEAD
					search->empties[prev].next = x;	// restore
>>>>>>> 8ee1734 (Use get_moves in search_shallow)
=======
				search->empties[prev].next = x;	// restore
>>>>>>> 9f982ee (Revise PASS handling; prioritymoves in shallow; optimize Neighbour test)
=======
					if (score > alpha)
						return score;
					else if (score > bestscore)
						bestscore = score;
				}
			} while ((x = search->empties[prev = x].next) != NOMOVE);
		} while ((prioritymoves = (moves ^= prioritymoves)));
>>>>>>> bb98132 (Split 5 empties search_shallow loop; tune stabiliby cutoff)
=======
				do {
					x = search->empties[prev = x].next;
				} while (!(prioritymoves & x_to_bit(x)));	// (58%)

				prioritymoves &= ~x_to_bit(x);
				search->empties[prev].next = search->empties[x].next;	// remove - maintain single link only
				vboard_next(board0, x, &search->board);
				score = search_solve_4(search, alpha);
				search->empties[prev].next = x;	// restore

				if (score > alpha)	// (49%)
					return score;
				else if (score > bestscore)
					bestscore = score;
<<<<<<< HEAD
			} while (prioritymoves);
		} while ((prioritymoves = moves));
>>>>>>> 6a63841 (exit search_shallow/search_eval loop when all bits processed)
=======
			} while (prioritymoves);	// (34%)
		} while ((prioritymoves = moves));	// (38%)
>>>>>>> 264e827 (calc solid stone only when stability cutoff tried)

	else {
		--search->eval.n_empties;	// for next depth
		do {
			moves ^= prioritymoves;
			x = NOMOVE;
			do {
				do {
					x = search->empties[prev = x].next;
				} while (!(prioritymoves & x_to_bit(x)));	// (57%)

				prioritymoves &= ~x_to_bit(x);
				search->eval.parity = parity0 ^ QUADRANT_ID[x];
				search->empties[prev].next = search->empties[x].next;	// remove - maintain single link only
				vboard_next(board0, x, &search->board);
				score = -search_shallow(search, ~alpha, false);
				search->empties[prev].next = x;	// restore

				if (score > alpha) {	// (40%)
					// store_vboard(search->board, board0);
					// search->eval.parity = parity0;
					++search->eval.n_empties;
					return score;

				} else if (score > bestscore)
					bestscore = score;
			} while (prioritymoves);	// (54%)
		} while ((prioritymoves = moves));	// (23%)
		++search->eval.n_empties;
	}
	// store_vboard(search->board, board0);
	// search->eval.parity = parity0;

<<<<<<< HEAD
	// no move
	if (bestscore == -SCORE_INF) {
		if (can_move(search->board.opponent, search->board.player)) { // pass
			search_pass_endgame(search);
			bestscore = -search_shallow(search, -(alpha + 1));
			search_pass_endgame(search);
		} else { // gameover
			bestscore = search_solve(search);
		}
>>>>>>> 6506166 (More SSE optimizations)
	}
	// search->board = board0.board;
	// search->eval.parity = parity0;

=======
>>>>>>> 8ee1734 (Use get_moves in search_shallow)
 	assert(SCORE_MIN <= bestscore && bestscore <= SCORE_MAX);
	return bestscore;	// (33%)
}

/**
 * @brief Evaluate an endgame position with a Null Window Search algorithm.
 *
 * This function is used when there are still many empty squares on the board. Move
 * ordering, hash table cutoff, enhanced transposition cutoff, etc. are used in
 * order to diminish the size of the tree to analyse, but at the expense of a
 * slower speed.
 *
 * @param search Search.
 * @param alpha Alpha bound.
 * @return The final score, as a disc difference.
 */
int NWS_endgame(Search *search, const int alpha)
{
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
	int score, ofssolid, bestscore;
<<<<<<< HEAD
	unsigned long long hash_code, solid_opp;
	// const int beta = alpha + 1;
	HashStoreData hash_data;
	Move *move;
	long long nodes_org;
	V2DI board0;
	Board hashboard;
	unsigned int parity0;
	unsigned long long full[5];
	MoveList movelist;

	assert(bit_count(~(search->board.player|search->board.opponent)) < DEPTH_MIDGAME_TO_ENDGAME);
	assert(SCORE_MIN <= alpha && alpha <= SCORE_MAX);
=======
	int score;
=======
	int score, ofssolid;
>>>>>>> 6c3ed52 (Dogaishi hash reduction by Matsuo & Narazaki; edge-precise get_full_line)
=======
	int score, ofssolid, bestscore;
>>>>>>> e832f60 (Inlining move_evaluate; skip movelist_evaluate if empty = 1)
	HashTable *const hash_table = &search->hash_table;
=======
>>>>>>> 3a92d84 (minor AVX512/SSE optimizations)
	unsigned long long hash_code, solid_opp;
	// const int beta = alpha + 1;
	HashData hash_data;
	HashStoreData hash_store_data;
	MoveList movelist;
<<<<<<< HEAD
	Move *move, *bestmove;
<<<<<<< HEAD
	long long cost;
>>>>>>> 6506166 (More SSE optimizations)
=======
=======
	Move *move;
>>>>>>> e832f60 (Inlining move_evaluate; skip movelist_evaluate if empty = 1)
	long long nodes_org;
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
>>>>>>> d1c50ef (Structured hash_store parameters; AVXLASTFLIP changed to opt-in)
=======
	Board board0;
	unsigned int parity0;
>>>>>>> 4b9f204 (minor optimize in search_eval_1/2 and search_shallow)
=======
	Board board0, hashboard;
=======
	vBoard board0;
=======
	rBoard board0;
>>>>>>> 78ce5d7 (more precise rboard/vboard opt; reexamine neon vboard_next)
	Board hashboard;
>>>>>>> 3a92d84 (minor AVX512/SSE optimizations)
	unsigned int parity0;
<<<<<<< HEAD
	V4DI full;
>>>>>>> 6c3ed52 (Dogaishi hash reduction by Matsuo & Narazaki; edge-precise get_full_line)
=======
	unsigned long long full[5];
<<<<<<< HEAD
<<<<<<< HEAD
>>>>>>> 4303b09 (Returns all full lines in full[4])
=======
	bool ffull;
>>>>>>> ff1c5db (skip hash access if n_moves <= 1 in NWS_endgame)
=======
>>>>>>> 264e827 (calc solid stone only when stability cutoff tried)

	if (search->stop) return alpha;

<<<<<<< HEAD
<<<<<<< HEAD
=======
	assert(search->n_empties == bit_count(~(search->board.player|search->board.opponent)));
=======
	assert(search->eval.n_empties == bit_count(~(search->board.player|search->board.opponent)));
>>>>>>> c8248ad (Move n_empties into Eval; tweak eval_open and eval_set)
	assert(SCORE_MIN <= alpha && alpha <= SCORE_MAX);

>>>>>>> 0a166fd (Remove 1 element array coding style)
	SEARCH_STATS(++statistics.n_NWS_endgame);
<<<<<<< HEAD
<<<<<<< HEAD
=======

<<<<<<< HEAD
	if (search->eval.n_empties <= DEPTH_TO_SHALLOW_SEARCH) return search_shallow(search, alpha);

>>>>>>> 1b29848 (fix & optimize 32 bit build; other minor mods)
=======
>>>>>>> 8ee1734 (Use get_moves in search_shallow)
	SEARCH_UPDATE_INTERNAL_NODES(search->n_nodes);

<<<<<<< HEAD
<<<<<<< HEAD
	// stability cutoff
<<<<<<< HEAD
	hashboard = board0.board = search->board;
	ofssolid = 0;
	if (USE_SC && alpha >= NWS_STABILITY_THRESHOLD[search->eval.n_empties]) {	// (7%)
		CUTOFF_STATS(++statistics.n_stability_try;)
		score = SCORE_MAX - 2 * get_stability_fulls(search->board.opponent, search->board.player, full);
		if (score <= alpha) {	// (3%)
			CUTOFF_STATS(++statistics.n_stability_low_cutoff;)
			return score;
=======
	if (search_SC_NWS(search, alpha, &score)) return score;

=======
>>>>>>> 21f8809 (Share all full lines between get_stability and Dogaishi hash reduction)
	// transposition cutoff

=======
>>>>>>> 9794cc1 (Store solid-normalized hash in PVS_midgame)
	// Improvement of Serch by Reducing Redundant Information in a Position of Othello
	// Hidekazu Matsuo, Shuji Narazaki
	// http://id.nii.ac.jp/1001/00156359/
	// (1-2% improvement)
	hashboard = search->board;
	ofssolid = 0;
	if (search->eval.n_empties <= MASK_SOLID_DEPTH) {	// (72%)
		get_all_full_lines(hashboard.player | hashboard.opponent, full);
		solid_opp = full[4] & hashboard.opponent;	// full[4] = all full
		hashboard.player ^= solid_opp;	// normalize solid to player
		hashboard.opponent ^= solid_opp;
		ofssolid = bit_count(solid_opp) * 2;	// hash score is ofssolid grater than real
	}
	hash_code = board_get_hash_code(&hashboard);

=======
	SEARCH_UPDATE_INTERNAL_NODES(search->n_nodes);

>>>>>>> ff1c5db (skip hash access if n_moves <= 1 in NWS_endgame)
	// stability cutoff
	hashboard = search->board;
	ofssolid = 0;
	if (USE_SC && alpha >= NWS_STABILITY_THRESHOLD[search->eval.n_empties]) {	// (3%)
		CUTOFF_STATS(++statistics.n_stability_try;)
		score = SCORE_MAX - 2 * get_stability_fulls(search->board.opponent, search->board.player, full);
		if (score <= alpha) {	// (5%)
			CUTOFF_STATS(++statistics.n_stability_low_cutoff;)
			return score;
		}

		// Improvement of Serch by Reducing Redundant Information in a Position of Othello
		// Hidekazu Matsuo, Shuji Narazaki
		// http://id.nii.ac.jp/1001/00156359/
		if (search->eval.n_empties <= MASK_SOLID_DEPTH) {	// (72%)
			solid_opp = full[4] & hashboard.opponent;	// full[4] = all full
#ifndef POPCOUNT
			if (solid_opp)
#endif
			{
				hashboard.player ^= solid_opp;	// normalize solid to player
				hashboard.opponent ^= solid_opp;
				ofssolid = bit_count(solid_opp) * 2;	// hash score is ofssolid grater than real
			}
		}
	}

	hash_code = board_get_hash_code(&hashboard);
	hash_prefetch(&search->hash_table, hash_code);

	search_get_movelist(search, &movelist);

<<<<<<< HEAD
	nodes_org = search->n_nodes;

	// special cases
	if (movelist_is_empty(&movelist)) {	// (1%)
		if (can_move(search->board.opponent, search->board.player)) { // pass
			board_pass(&search->board);
			bestscore = -NWS_endgame(search, ~alpha);
			board_pass(&search->board);
			hash_store_data.data.move[0] = PASS;
		} else  { // game over
<<<<<<< HEAD
			bestmove->score = search_solve(search);
			bestmove->x = NOMOVE;
>>>>>>> 6506166 (More SSE optimizations)
=======
			bestscore = search_solve(search);
			hash_store_data.data.move[0] = NOMOVE;
>>>>>>> e832f60 (Inlining move_evaluate; skip movelist_evaluate if empty = 1)
		}
<<<<<<< HEAD

<<<<<<< HEAD
		// Improvement of Serch by Reducing Redundant Information in a Position of Othello
		// Hidekazu Matsuo, Shuji Narazaki
		// http://id.nii.ac.jp/1001/00156359/
		if (search->eval.n_empties <= MASK_SOLID_DEPTH) {	// (99%)
			solid_opp = full[4] & hashboard.opponent;	// full[4] = all full
#ifndef POPCOUNT
			if (solid_opp)	// (72%)
#endif
			{
				hashboard.player ^= solid_opp;	// normalize solid to player
				hashboard.opponent ^= solid_opp;
				ofssolid = bit_count(solid_opp) * 2;	// hash score is ofssolid grater than real
=======
		bestmove = movelist->move; bestmove->score = -SCORE_INF;
=======
	} else {
		if (movelist.n_moves > 1)	// (97%)
			movelist_evaluate(&movelist, search, &hash_data, alpha, 0);
=======
	if (movelist.n_moves > 1) {	// (96%)
<<<<<<< HEAD
		// Improvement of Serch by Reducing Redundant Information in a Position of Othello
		// Hidekazu Matsuo, Shuji Narazaki
		// http://id.nii.ac.jp/1001/00156359/
		// (1-2% improvement)
		hashboard = search->board;
		ofssolid = 0;
		if (search->eval.n_empties <= MASK_SOLID_DEPTH) {	// (72%)
			if (!ffull)
				get_all_full_lines(hashboard.player | hashboard.opponent, full);
			solid_opp = full[4] & hashboard.opponent;	// full[4] = all full
			hashboard.player ^= solid_opp;	// normalize solid to player
			hashboard.opponent ^= solid_opp;
			ofssolid = bit_count(solid_opp) * 2;	// hash score is ofssolid grater than real
		}
<<<<<<< HEAD
>>>>>>> ff1c5db (skip hash access if n_moves <= 1 in NWS_endgame)
=======
		hash_code = board_get_hash_code(&hashboard);
>>>>>>> 44fd278 (Rearrange PVS_shallow loop)

=======
>>>>>>> 30464b5 (add hash_prefetch to NWS_endgame)
		// transposition cutoff
		if (hash_get(&search->hash_table, &hashboard, hash_code, &hash_data)) {	// (6%)
			hash_data.lower -= ofssolid;
			hash_data.upper -= ofssolid;
			if (search_TC_NWS(&hash_data, search->eval.n_empties, NO_SELECTIVITY, alpha, &score))
				return score;
		}
		// else if (ofssolid)	// slows down
		//	hash_get_from_board(&search->hash_table, &search->board, &hash_data);

		movelist_evaluate_fast(&movelist, search, &hash_data);

		nodes_org = search->n_nodes;
		board0 = load_rboard(search->board);
		parity0 = search->eval.parity;
<<<<<<< HEAD
<<<<<<< HEAD
		bestmove = movelist.move; bestmove->score = -SCORE_INF;
>>>>>>> 0a166fd (Remove 1 element array coding style)
=======
=======
		--search->eval.n_empties;	// for next move
>>>>>>> 9f982ee (Revise PASS handling; prioritymoves in shallow; optimize Neighbour test)
		bestscore = -SCORE_INF;
>>>>>>> e832f60 (Inlining move_evaluate; skip movelist_evaluate if empty = 1)
		// loop over all moves
		foreach_best_move(move, movelist) {
			search_swap_parity(search, move->x);
			empty_remove(search->empties, move->x);
			rboard_update(&search->board, board0, move);

			if (search->eval.n_empties <= DEPTH_TO_SHALLOW_SEARCH)	// (43%)
				score = -search_shallow(search, ~alpha, false);
			else	score = -NWS_endgame(search, ~alpha);

			search->eval.parity = parity0;
			empty_restore(search->empties, move->x);
			store_rboard(search->board, board0);

<<<<<<< HEAD
<<<<<<< HEAD
			if (move->score > bestmove->score) {
				bestmove = move;
				if (bestmove->score > alpha) break;
>>>>>>> 6506166 (More SSE optimizations)
=======
			if (score > bestscore) {
				bestscore = score;
				hash_store_data.data.move[0] = move->x;
				if (bestscore > alpha) break;
>>>>>>> e832f60 (Inlining move_evaluate; skip movelist_evaluate if empty = 1)
=======
			if (score > bestscore) {	// (66%)
				bestscore = score;
				hash_store_data.data.move[0] = move->x;
				if (bestscore > alpha) break;	// (57%)
>>>>>>> 9f982ee (Revise PASS handling; prioritymoves in shallow; optimize Neighbour test)
			}
		}
		++search->eval.n_empties;

		if (search->stop)
			return alpha;

		hash_store_data.data.wl.c.depth = search->eval.n_empties;
		hash_store_data.data.wl.c.selectivity = NO_SELECTIVITY;
		hash_store_data.data.wl.c.cost = last_bit(search->n_nodes - nodes_org);
		// hash_store_data.data.move[0] = bestmove;
		hash_store_data.alpha = alpha + ofssolid;
		hash_store_data.beta = alpha + ofssolid + 1;
		hash_store_data.score = bestscore + ofssolid;
		hash_store(&search->hash_table, &hashboard, hash_code, &hash_store_data);

	// special cases
	} else if (movelist.n_moves == 1) {	// (3%)
		board0 = load_rboard(search->board);
		parity0 = search->eval.parity;
		move = movelist.move[0].next;
		search_swap_parity(search, move->x);
		empty_remove(search->empties, move->x);
		rboard_update(&search->board, board0, move);
		--search->eval.n_empties;
		if (search->eval.n_empties <= DEPTH_TO_SHALLOW_SEARCH)
			bestscore = -search_shallow(search, ~alpha, false);
		else	bestscore = -NWS_endgame(search, ~alpha);
		++search->eval.n_empties;

		empty_restore(search->empties, move->x);
		search->eval.parity = parity0;
		store_rboard(search->board, board0);

	} else {	// (1%)
		if (can_move(search->board.opponent, search->board.player)) { // pass
			search_pass(search);
			bestscore = -NWS_endgame(search, ~alpha);
			search_pass(search);
		} else  { // game over
			bestscore = search_solve(search);
		}
	}

<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
	hash_code = board_get_hash_code(&hashboard);
	hash_prefetch(&search->hash_table, hash_code);

	search_get_movelist(search, &movelist);

	if (movelist.n_moves > 1) {	// (96%)
		// transposition cutoff
		if (hash_get(&search->hash_table, &hashboard, hash_code, &hash_data.data)) {	// (6%)
			hash_data.data.lower -= ofssolid;
			hash_data.data.upper -= ofssolid;
			if (search_TC_NWS(&hash_data.data, search->eval.n_empties, NO_SELECTIVITY, alpha, &score))	// (6%)
				return score;
		}
		// else if (ofssolid)	// slows down
		//	hash_get_from_board(&search->hash_table, HBOARD_V(board0), &hash_data.data);

		movelist_evaluate_fast(&movelist, search, &hash_data.data);

		nodes_org = search->n_nodes;
		parity0 = search->eval.parity;
		bestscore = -SCORE_INF;
		// loop over all moves
		move = &movelist.move[0];
		if (--search->eval.n_empties <= DEPTH_TO_SHALLOW_SEARCH)	// for next move (44%)
			while ((move = move_next_best(move))) {	// (72%)
				search->eval.parity = parity0 ^ QUADRANT_ID[move->x];
				search->empties[search->empties[move->x].previous].next = search->empties[move->x].next;	// remove - maintain single link only
				vboard_update(&search->board, board0, move);
				score = -search_shallow(search, ~alpha, false);
				search->empties[search->empties[move->x].previous].next = move->x;	// restore
				search->board = board0.board;

				if (score > bestscore) {	// (63%)
					bestscore = score;
					hash_data.data.move[0] = move->x;
					if (bestscore > alpha) break;	// (48%)
				}
			}
		else
			while ((move = move_next_best(move))) {	// (76%)
				search->eval.parity = parity0 ^ QUADRANT_ID[move->x];
				empty_remove(search->empties, move->x);
				vboard_update(&search->board, board0, move);
				score = -NWS_endgame(search, ~alpha);
				empty_restore(search->empties, move->x);
				search->board = board0.board;

				if (score > bestscore) {	// (63%)
					bestscore = score;
					hash_data.data.move[0] = move->x;
					if (bestscore > alpha) break;	// (39%)
				}
			}
		++search->eval.n_empties;
		search->eval.parity = parity0;

		if (search->stop)	// (1%)
			return alpha;

		hash_data.data.wl.c.depth = search->eval.n_empties;
		hash_data.data.wl.c.selectivity = NO_SELECTIVITY;
		hash_data.data.wl.c.cost = last_bit(search->n_nodes - nodes_org);
		// hash_data.data.move[0] = bestmove;
		hash_data.alpha = alpha + ofssolid;
		hash_data.beta = alpha + ofssolid + 1;
		hash_data.score = bestscore + ofssolid;
		hash_store(&search->hash_table, &hashboard, hash_code, &hash_data);

	// special cases
	} else if (movelist.n_moves == 1) {	// (3%)
		parity0 = search->eval.parity;
		move = movelist_first(&movelist);
		search_swap_parity(search, move->x);
		empty_remove(search->empties, move->x);
		vboard_update(&search->board, board0, move);
		if (--search->eval.n_empties <= DEPTH_TO_SHALLOW_SEARCH)	// (56%)
			bestscore = -search_shallow(search, ~alpha, false);
		else	bestscore = -NWS_endgame(search, ~alpha);
		++search->eval.n_empties;
		empty_restore(search->empties, move->x);
		search->eval.parity = parity0;
		search->board = board0.board;

	} else {	// (1%)
		if (can_move(search->board.opponent, search->board.player)) { // pass
			search_pass(search);
			bestscore = -NWS_endgame(search, ~alpha);
			search_pass(search);
		} else  { // game over
			bestscore = search_solve(search);
=======
	if (!search->stop) {
		hash_store_data.data.wl.c.depth = search->eval.n_empties;
		hash_store_data.data.wl.c.selectivity = NO_SELECTIVITY;
		hash_store_data.data.wl.c.cost = last_bit(search->n_nodes - nodes_org);
		// hash_store_data.data.move[0] = bestmove;
		hash_store_data.alpha = alpha + ofssolid;
		hash_store_data.beta = alpha + ofssolid + 1;
		hash_store_data.score = bestscore + ofssolid;
		hash_store(hash_table, &hashboard, hash_code, &hash_store_data);

		if (SQUARE_STATS(1) + 0) {
			foreach_move(move, movelist)
<<<<<<< HEAD
				++statistics.n_played_square[search->n_empties][SQUARE_TYPE[move->x]];
			if (bestmove->score > alpha) ++statistics.n_good_square[search->n_empties][SQUARE_TYPE[bestmove->score]];
>>>>>>> 6506166 (More SSE optimizations)
=======
				++statistics.n_played_square[search->eval.n_empties][SQUARE_TYPE[move->x]];
<<<<<<< HEAD
<<<<<<< HEAD
			if (bestmove->score > alpha) ++statistics.n_good_square[search->eval.n_empties][SQUARE_TYPE[bestmove->score]];
>>>>>>> c8248ad (Move n_empties into Eval; tweak eval_open and eval_set)
=======
			if (bestmove->score > alpha)
				++statistics.n_good_square[search->eval.n_empties][SQUARE_TYPE[bestmove->score]];
>>>>>>> 6c3ed52 (Dogaishi hash reduction by Matsuo & Narazaki; edge-precise get_full_line)
		}
=======
			if (bestscore > alpha)
				++statistics.n_good_square[search->eval.n_empties][SQUARE_TYPE[bestscore]];
		}
	 	assert(SCORE_MIN <= bestscore && bestscore <= SCORE_MAX);
	 	assert((bestscore & 1) == 0);
		return bestscore;
>>>>>>> e832f60 (Inlining move_evaluate; skip movelist_evaluate if empty = 1)
	}

	if (SQUARE_STATS(1) + 0) {
		foreach_move(move, movelist)
			++statistics.n_played_square[search->eval.n_empties][SQUARE_TYPE[move->x]];
		if (bestscore > alpha)
			++statistics.n_good_square[search->eval.n_empties][SQUARE_TYPE[bestscore]];
	}
=======
	if (search->stop)
		return alpha;

	hash_store_data.data.wl.c.depth = search->eval.n_empties;
	hash_store_data.data.wl.c.selectivity = NO_SELECTIVITY;
	hash_store_data.data.wl.c.cost = last_bit(search->n_nodes - nodes_org);
	// hash_store_data.data.move[0] = bestmove;
	hash_store_data.alpha = alpha + ofssolid;
	hash_store_data.beta = alpha + ofssolid + 1;
	hash_store_data.score = bestscore + ofssolid;
	hash_store(hash_table, &hashboard, hash_code, &hash_store_data);

=======
>>>>>>> ff1c5db (skip hash access if n_moves <= 1 in NWS_endgame)
	if (SQUARE_STATS(1) + 0) {
		foreach_move(move, movelist)
			++statistics.n_played_square[search->eval.n_empties][SQUARE_TYPE[move->x]];
		if (bestscore > alpha)
			++statistics.n_good_square[search->eval.n_empties][SQUARE_TYPE[bestscore]];
	}
>>>>>>> 9f982ee (Revise PASS handling; prioritymoves in shallow; optimize Neighbour test)
 	assert(SCORE_MIN <= bestscore && bestscore <= SCORE_MAX);
 	assert((bestscore & 1) == 0);
	return bestscore;
}
