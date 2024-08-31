/**
 * @file midgame.c
 *
 * Search near the end of the game.
 *
<<<<<<< HEAD
<<<<<<< HEAD
 * @date 1998 - 2023
=======
 * @date 1998 - 2018
>>>>>>> 1c68bd5 (SSE / AVX optimized eval feature added)
=======
 * @date 1998 - 2020
>>>>>>> f1d221c (Replace eval_restore with simple save-restore, as well as parity)
 * @author Richard Delorme
 * @author Toshihiko Okuhara
 * @version 4.5
 */

#include "search.h"

#include "bit.h"
#include "options.h"
#include "stats.h"
#include "ybwc.h"
#include "settings.h"

#include <assert.h>
#include <stddef.h>
#include <math.h>

#ifndef RCD
/** macro RCD : set to 0.0 if -rcd is added to the icc compiler */
#define RCD 0.5
#endif

/**
 * @brief evaluate a midgame position with the evaluation function.
 *
 * @param ply	60 - n_empties
 * @param eval	Evaluation function.
 * @return An evaluated score.
 */
static int accumlate_eval(int ply, Eval *eval)
{
	unsigned short *f = eval->feature.us;
	const Eval_weight *w;
	int sum;

	if (ply >= EVAL_N_PLY)
		ply = EVAL_N_PLY - 2 + (ply & 1);
	ply -= 2;
	if (ply < 0)
		ply &= 1;
	w = &(*EVAL_WEIGHT)[ply];

#if defined(__AVX2__) && !defined(__bdver4__) && !defined(__znver1__) && !defined(__znver2__)
	enum {
		W_C9 = offsetof(Eval_weight, C9) / sizeof(short) - 1,	// -1 to load the data into hi-word
		W_C10 = offsetof(Eval_weight, C10) / sizeof(short) - 1,
		W_S100 = offsetof(Eval_weight, S100) / sizeof(short) - 1,
		W_S101 = offsetof(Eval_weight, S101) / sizeof(short) - 1
	};

	__m256i FF = _mm256_add_epi32(_mm256_cvtepu16_epi32(eval->feature.v8[0]),
		_mm256_set_epi32(W_C10, W_C10, W_C10, W_C10, W_C9, W_C9, W_C9, W_C9));
	__m256i DD = _mm256_i32gather_epi32((int *) w, FF, 2);
	__m256i SS = _mm256_srai_epi32(DD, 16);	// sign extend

	FF = _mm256_add_epi32(_mm256_cvtepu16_epi32(eval->feature.v8[1]),
		_mm256_set_epi32(W_S101, W_S101, W_S101, W_S101, W_S100, W_S100, W_S100, W_S100));
	DD = _mm256_i32gather_epi32((int *) w, FF, 2);
	SS = _mm256_add_epi32(SS, _mm256_srai_epi32(DD, 16));

	DD = _mm256_i32gather_epi32((int *)((short *) w->S8x4 - 1), _mm256_cvtepu16_epi32(eval->feature.v8[2]), 2);
	SS = _mm256_add_epi32(SS, _mm256_srai_epi32(DD, 16));

	DD = _mm256_i32gather_epi32((int *)((short *) w->S7654 - 1), _mm256_cvtepu16_epi32(*(__m128i *) &f[30]), 2);
	SS = _mm256_add_epi32(SS, _mm256_srai_epi32(DD, 16));

	DD = _mm256_i32gather_epi32((int *)((short *) w->S7654 - 1), _mm256_cvtepu16_epi32(*(__m128i *) &f[38]), 2);
	SS = _mm256_add_epi32(SS, _mm256_srai_epi32(DD, 16));
	__m128i S = _mm_add_epi32(_mm256_castsi256_si128(SS), _mm256_extracti128_si256(SS, 1));

	__m128i D = _mm_i32gather_epi32((int *)((short *) w->S8x4 - 1), _mm_cvtepu16_epi32(eval->feature.v8[3]), 2);
	S = _mm_add_epi32(S, _mm_srai_epi32(D, 16));

	S = _mm_hadd_epi32(S, S);
	sum = _mm_cvtsi128_si32(S) + _mm_extract_epi32(S, 1);

#else
	sum = w->C9[f[ 0]] + w->C9[f[ 1]] + w->C9[f[ 2]] + w->C9[f[ 3]]
	  + w->C10[f[ 4]] + w->C10[f[ 5]] + w->C10[f[ 6]] + w->C10[f[ 7]]
	  + w->S100[f[ 8]] + w->S100[f[ 9]] + w->S100[f[10]] + w->S100[f[11]]
	  + w->S101[f[12]] + w->S101[f[13]] + w->S101[f[14]] + w->S101[f[15]]
	  + w->S8x4[f[16]] + w->S8x4[f[17]] + w->S8x4[f[18]] + w->S8x4[f[19]]
	  + w->S8x4[f[20]] + w->S8x4[f[21]] + w->S8x4[f[22]] + w->S8x4[f[23]]
	  + w->S8x4[f[24]] + w->S8x4[f[25]] + w->S8x4[f[26]] + w->S8x4[f[27]]
	  + w->S7654[f[30]] + w->S7654[f[31]] + w->S7654[f[32]] + w->S7654[f[33]]
	  + w->S7654[f[34]] + w->S7654[f[35]] + w->S7654[f[36]] + w->S7654[f[37]]
	  + w->S7654[f[38]] + w->S7654[f[39]] + w->S7654[f[40]] + w->S7654[f[41]]
	  + w->S7654[f[42]] + w->S7654[f[43]] + w->S7654[f[44]] + w->S7654[f[45]];
#endif
	return sum + w->S8x4[f[28]] + w->S8x4[f[29]] + w->S0;
}

/**
 * @brief evaluate a midgame position with the evaluation function.
 *
 * @param search Position to evaluate.
 * @return An evaluated score.
 */
int search_eval_0(Search *search)
{
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
=======
	const short *w = EVAL_WEIGHT[search->eval->player][60 - search->n_empties];
	unsigned short int *f = search->eval->feature.us;
>>>>>>> 4a049b7 (Rewrite eval_open; Free SymetryPacking after init; short int feature)
=======
	const short *w = EVAL_WEIGHT[search->eval.player][60 - search->n_empties];
=======
	const short *w = EVAL_WEIGHT[search->eval.player][60 - search->eval.n_empties];
>>>>>>> c8248ad (Move n_empties into Eval; tweak eval_open and eval_set)
	unsigned short int *f = search->eval.feature.us;
>>>>>>> f1d221c (Replace eval_restore with simple save-restore, as well as parity)
	int score;

	SEARCH_STATS(++statistics.n_search_eval_0);
	SEARCH_UPDATE_EVAL_NODES(search->n_nodes);

<<<<<<< HEAD
	score = accumlate_eval(60 - search->eval.n_empties,  &search->eval);
=======
	score = w[f[ 0] + 0] + w[f[ 1] + 0] + w[f[ 2] + 0] + w[f[ 3] + 0]
	  + w[f[ 4] + 19683] + w[f[ 5] + 19683] + w[f[ 6] + 19683] + w[f[ 7] + 19683]
	  + w[f[ 8] + 78732] + w[f[ 9] + 78732] + w[f[10] + 78732] + w[f[11] + 78732]
	  + w[f[12] + 137781] + w[f[13] + 137781] + w[f[14] + 137781] + w[f[15] + 137781]
	  + w[f[16] + 196830] + w[f[17] + 196830] + w[f[18] + 196830] + w[f[19] + 196830]
	  + w[f[20] + 203391] + w[f[21] + 203391] + w[f[22] + 203391] + w[f[23] + 203391]
	  + w[f[24] + 209952] + w[f[25] + 209952] + w[f[26] + 209952] + w[f[27] + 209952]
	  + w[f[28] + 216513] + w[f[29] + 216513]
	  + w[f[30] + 223074] + w[f[31] + 223074] + w[f[32] + 223074] + w[f[33] + 223074]
	  + w[f[34] + 225261] + w[f[35] + 225261] + w[f[36] + 225261] + w[f[37] + 225261]
	  + w[f[38] + 225990] + w[f[39] + 225990] + w[f[40] + 225990] + w[f[41] + 225990]
	  + w[f[42] + 226233] + w[f[43] + 226233] + w[f[44] + 226233] + w[f[45] + 226233]
	  + w[f[46] + 226314];
>>>>>>> 4a049b7 (Rewrite eval_open; Free SymetryPacking after init; short int feature)

	if (score > 0) score += 64;	else score -= 64;
	score /= 128;

	if (score < SCORE_MIN + 1) score = SCORE_MIN + 1;
	if (score > SCORE_MAX - 1) score = SCORE_MAX - 1;

	return score;
}

/**
 * @brief Evaluate a position at depth 1. (min stage)
 *
 * As an optimization, the last move is used to only updates the evaluation
 * features.
 *
 * @param search Position to evaluate.
 * @param alpha Alpha bound.
 * @param beta Beta bound.
 * @param moves Next turn legal moves.
 * @return An evaluated min score.
 */
<<<<<<< HEAD
int search_eval_1(Search *search, int alpha, int beta, unsigned long long moves)
=======
int search_eval_1(Search *search, const int alpha, int beta, unsigned long long moves)
>>>>>>> 4b9f204 (minor optimize in search_eval_1/2 and search_shallow)
{
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
	int x, score, bestscore, alphathres;
	unsigned long long flipped;
	Eval Ev;
	V2DI board0;
=======
	const short *w = EVAL_WEIGHT[search->eval->player ^ 1][61 - search->n_empties];
=======
	const short *w = EVAL_WEIGHT[search->eval.player ^ 1][61 - search->n_empties];
<<<<<<< HEAD
>>>>>>> f1d221c (Replace eval_restore with simple save-restore, as well as parity)
	Move move[1];
=======
=======
>>>>>>> 037f46e (New eval_update_leaf updates eval on copy; save-restore eval.feature only)
	Move move;
<<<<<<< HEAD
>>>>>>> 0a166fd (Remove 1 element array coding style)
	SquareList *empty;
	Eval Ev;
	int score, bestscore;
<<<<<<< HEAD
<<<<<<< HEAD
	const Board *board = search->board;
	unsigned long long moves = get_moves(board->player, board->opponent);
<<<<<<< HEAD
	unsigned short int *f;
>>>>>>> 4a049b7 (Rewrite eval_open; Free SymetryPacking after init; short int feature)
=======
=======
	unsigned long long moves = get_moves(search->board.player, search->board.opponent);
<<<<<<< HEAD
>>>>>>> 0a166fd (Remove 1 element array coding style)
	unsigned short *f;
>>>>>>> 1c68bd5 (SSE / AVX optimized eval feature added)
=======
=======
>>>>>>> 4b9f204 (minor optimize in search_eval_1/2 and search_shallow)
=======
	Eval Ev;
	int x, score, bestscore;
>>>>>>> 5e86fd6 (Change pointer-linked empty list to index-linked)
	const short *w;
	const unsigned short *f;
>>>>>>> 037f46e (New eval_update_leaf updates eval on copy; save-restore eval.feature only)

	SEARCH_STATS(++statistics.n_search_eval_1);
	SEARCH_UPDATE_INTERNAL_NODES(search->n_nodes);

	if (moves) {
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
		bestscore = SCORE_INF * 128;	// min stage
		if (alpha < SCORE_MIN + 1) alphathres = ((SCORE_MIN + 1) * 128) + 64;
		else alphathres = (alpha * 128) + 63 + (int) (alpha < 0);	// highest score rounded to alpha
=======
=======
		w = EVAL_WEIGHT[search->eval.player ^ 1][61 - search->n_empties];
>>>>>>> 037f46e (New eval_update_leaf updates eval on copy; save-restore eval.feature only)
=======
		w = EVAL_WEIGHT[search->eval.player ^ 1][61 - search->eval.n_empties];
>>>>>>> c8248ad (Move n_empties into Eval; tweak eval_open and eval_set)
		bestscore = -SCORE_INF;
		if (beta >= SCORE_MAX) beta = SCORE_MAX - 1;
		foreach_empty (x, search->empties) {
			if (moves & x_to_bit(x)) {
				board_get_move(&search->board, x, &move);
				if (move_wipeout(&move, &search->board)) return SCORE_MAX;

				eval_update_leaf(&Ev, &search->eval, &move);
				f = Ev.feature.us;
				SEARCH_UPDATE_EVAL_NODES(search->n_nodes);
				score = -w[f[ 0] + 0] - w[f[ 1] + 0] - w[f[ 2] + 0] - w[f[ 3] + 0]
				  - w[f[ 4] + 19683] - w[f[ 5] + 19683] - w[f[ 6] + 19683] - w[f[ 7] + 19683]
				  - w[f[ 8] + 78732] - w[f[ 9] + 78732] - w[f[10] + 78732] - w[f[11] + 78732]
				  - w[f[12] + 137781] - w[f[13] + 137781] - w[f[14] + 137781] - w[f[15] + 137781]
				  - w[f[16] + 196830] - w[f[17] + 196830] - w[f[18] + 196830] - w[f[19] + 196830]
				  - w[f[20] + 203391] - w[f[21] + 203391] - w[f[22] + 203391] - w[f[23] + 203391]
				  - w[f[24] + 209952] - w[f[25] + 209952] - w[f[26] + 209952] - w[f[27] + 209952]
				  - w[f[28] + 216513] - w[f[29] + 216513]
				  - w[f[30] + 223074] - w[f[31] + 223074] - w[f[32] + 223074] - w[f[33] + 223074]
				  - w[f[34] + 225261] - w[f[35] + 225261] - w[f[36] + 225261] - w[f[37] + 225261]
				  - w[f[38] + 225990] - w[f[39] + 225990] - w[f[40] + 225990] - w[f[41] + 225990]
				  - w[f[42] + 226233] - w[f[43] + 226233] - w[f[44] + 226233] - w[f[45] + 226233]
				  - w[f[46] + 226314];
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
				eval_restore(search->eval, move);
>>>>>>> 4a049b7 (Rewrite eval_open; Free SymetryPacking after init; short int feature)
=======
				// eval_restore(search->eval, move);
>>>>>>> f1d221c (Replace eval_restore with simple save-restore, as well as parity)
=======
				// eval_restore(&search->eval, &move);
>>>>>>> 0a166fd (Remove 1 element array coding style)
=======
>>>>>>> 037f46e (New eval_update_leaf updates eval on copy; save-restore eval.feature only)

		board0.board = search->board;
		x = NOMOVE;
		do {
			do {
				x = search->empties[x].next;
			} while (!(moves & x_to_bit(x)));

			moves &= ~x_to_bit(x);
			flipped = vboard_flip(board0, x);
			if (flipped == search->board.opponent)
				return SCORE_MIN;	// wipeout

			eval_update_leaf(x, flipped, &Ev, &search->eval);
			SEARCH_UPDATE_EVAL_NODES(search->n_nodes);

			score = accumlate_eval(60 - search->eval.n_empties + 1, &Ev);

			if (score < bestscore)
				bestscore = score;
		} while (moves && (bestscore > alphathres));

		if (bestscore >= 0) bestscore += 64; else bestscore -= 64;
		bestscore /= 128;

		if (bestscore < SCORE_MIN + 1) bestscore = SCORE_MIN + 1;
		if (bestscore > SCORE_MAX - 1) bestscore = SCORE_MAX - 1;

<<<<<<< HEAD
	} else {
<<<<<<< HEAD
		moves = get_moves(search->board.opponent, search->board.player);
		if (moves) {
			search_update_pass_midgame(search, &Ev);
			bestscore = -search_eval_1(search, -beta, -alpha, moves);
			search_restore_pass_midgame(search, &Ev);
=======
		if (can_move(search->board.opponent, search->board.player)) {
=======
				if (score > bestscore) {
					bestscore = score;
					if (bestscore >= beta) break;
				}
			}
		}
		if (bestscore <= SCORE_MIN) bestscore = SCORE_MIN + 1;
		else if (bestscore >= SCORE_MAX) bestscore = SCORE_MAX - 1;

	} else {
		moves = get_moves(search->board.opponent, search->board.player);
		if (moves) {
>>>>>>> 4b9f204 (minor optimize in search_eval_1/2 and search_shallow)
			search_update_pass_midgame(search);
			bestscore = -search_eval_1(search, -beta, -alpha, moves);
			search_restore_pass_midgame(search);
>>>>>>> 0a166fd (Remove 1 element array coding style)
		} else { // game over
			bestscore = -search_solve(search);
		}
	}

	assert(SCORE_MIN <= bestscore && bestscore <= SCORE_MAX);
	return bestscore;
}

/**
 * @brief Evaluate a position at depth 2.
 *
 * Simple alpha-beta with no move sorting.
 *
 * @param search Position to evaluate.
 * @param alpha Lower bound
 * @param beta  Upper bound
 * @param moves Next turn legal moves.
 * @return An evaluated best score.
 */
<<<<<<< HEAD
int search_eval_2(Search *search, int alpha, int beta, unsigned long long moves)
=======
int search_eval_2(Search *search, int alpha, const int beta, unsigned long long moves)
>>>>>>> 4b9f204 (minor optimize in search_eval_1/2 and search_shallow)
{
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
	int x, bestscore, score;
	unsigned long long flipped;
	Eval eval0;
	V2DI board0;
=======
	register int bestscore, score;
=======
	int bestscore, score;
>>>>>>> d1c50ef (Structured hash_store parameters; AVXLASTFLIP changed to opt-in)
	SquareList *empty;
=======
	int x, bestscore, score;
>>>>>>> 5e86fd6 (Change pointer-linked empty list to index-linked)
	Move move;
	Eval Ev0;
<<<<<<< HEAD
<<<<<<< HEAD
	const Board *board = search->board;
	const unsigned long long moves = get_moves(board->player, board->opponent);
>>>>>>> f1d221c (Replace eval_restore with simple save-restore, as well as parity)
=======
	const unsigned long long moves = get_moves(search->board.player, search->board.opponent);
>>>>>>> 0a166fd (Remove 1 element array coding style)
=======
	Board board0;
>>>>>>> 4b9f204 (minor optimize in search_eval_1/2 and search_shallow)

	SEARCH_STATS(++statistics.n_search_eval_2);
	SEARCH_UPDATE_INTERNAL_NODES(search->n_nodes);

	assert(-SCORE_MAX <= alpha);
	assert(beta <= SCORE_MAX);
	assert(alpha <= beta);

	if (moves) {
		bestscore = -SCORE_INF;
<<<<<<< HEAD
<<<<<<< HEAD
		eval0.feature = search->eval.feature;
		eval0.n_empties = search->eval.n_empties--;
		board0.board = search->board;
		x = NOMOVE;
		do {
			do {
				x = search->empties[x].next;
			} while (!(moves & x_to_bit(x)));

			moves &= ~x_to_bit(x);
			// search->empties[prev].next = search->empties[x].next;	// let search_eval_1 skip the last occupied
			flipped = vboard_next(board0, x, &search->board);
			eval_update_leaf(x, flipped, &search->eval, &eval0);
			score = search_eval_1(search, alpha, beta, board_get_moves(&search->board));
			// search->empties[prev].next = x;	// restore

			if (score > bestscore) {
				bestscore = score;
				if (bestscore >= beta) break;
				else if (bestscore > alpha) alpha = bestscore;
=======
		Ev0 = search->eval;
=======
		Ev0.feature = search->eval.feature;
<<<<<<< HEAD
>>>>>>> 037f46e (New eval_update_leaf updates eval on copy; save-restore eval.feature only)
=======
		Ev0.player = search->eval.player;
		eval_swap(&search->eval);
		board0 = search->board;
		--search->eval.n_empties;

<<<<<<< HEAD
>>>>>>> 4b9f204 (minor optimize in search_eval_1/2 and search_shallow)
		foreach_empty(empty, search->empties) {
			if (moves & empty->b) {
				move.x = empty->x;
				move.flipped = board_next(&board0, move.x, &search->board);
				// empty_remove(search->x_to_empties[move.x]);
=======
		foreach_empty(x, search->empties) {
			if (moves & x_to_bit(x)) {
				move.x = x;
				move.flipped = board_next(&board0, x, &search->board);
				// empty_remove(search->empties, x);
>>>>>>> 5e86fd6 (Change pointer-linked empty list to index-linked)
				eval_update_leaf(&search->eval, &Ev0, &move);
				score = -search_eval_1(search, -beta, -alpha, get_moves(search->board.player, search->board.opponent));
				// empty_restore(search->empties, x);

				if (score > bestscore) {
					bestscore = score;
					if (bestscore >= beta) break;
					else if (bestscore > alpha) alpha = bestscore;
				}
>>>>>>> f1d221c (Replace eval_restore with simple save-restore, as well as parity)
			}
<<<<<<< HEAD
		} while (moves);
		search->eval.feature = eval0.feature;
		search->eval.n_empties = eval0.n_empties;
		search->board = board0.board;

	} else {
<<<<<<< HEAD
		moves = get_moves(search->board.opponent, search->board.player);
		if (moves) {
			search_update_pass_midgame(search, &eval0);
			bestscore = -search_eval_2(search, -beta, -alpha, moves);
			search_restore_pass_midgame(search, &eval0);
		} else { // game over
=======
		if (can_move(search->board.opponent, search->board.player)) {
=======
		}
		search->eval.feature = Ev0.feature;
		search->eval.player = Ev0.player;
		search->board = board0;
		++search->eval.n_empties;

	} else {
		moves = get_moves(search->board.opponent, search->board.player);
		if (moves) {
>>>>>>> 4b9f204 (minor optimize in search_eval_1/2 and search_shallow)
			search_update_pass_midgame(search);
			bestscore = -search_eval_2(search, -beta, -alpha, moves);
			search_restore_pass_midgame(search);
		} else {
>>>>>>> 0a166fd (Remove 1 element array coding style)
			bestscore = search_solve(search);
		}
	}

	assert(SCORE_MIN <= bestscore && bestscore <= SCORE_MAX);

	return bestscore;
}

<<<<<<< HEAD
=======
static inline void search_update_probcut(Search *search, const NodeType node_type) 
{
	search->node_type[search->height] = node_type;
	if (!USE_RECURSIVE_PROBCUT) search->selectivity = NO_SELECTIVITY;
	LIMIT_RECURSIVE_PROBCUT(++search->probcut_level;)
}


static inline void search_restore_probcut(Search *search, const NodeType node_type, const int selectivity) 
{
	search->node_type[search->height] = node_type;
	if (!USE_RECURSIVE_PROBCUT) search->selectivity = selectivity;
	LIMIT_RECURSIVE_PROBCUT(--search->probcut_level;)
}

>>>>>>> 4b9f204 (minor optimize in search_eval_1/2 and search_shallow)
/**
 * @brief Probcut
 *
 * Check if a position is worth to analyze further.
 *
 * @param search Position to test.
 * @param alpha Alpha lower bound.
 * @param depth Search depth.
 * @param parent Parent node.
 * @param value Returned value.
 * @return true if probable cutoff has been found, false otherwise.
 */
static inline void search_update_probcut(Search *search, const NodeType node_type) 
{
	search->node_type[search->height] = node_type;
	if (!USE_RECURSIVE_PROBCUT) search->selectivity = NO_SELECTIVITY;
	LIMIT_RECURSIVE_PROBCUT(++search->probcut_level;)
}


static inline void search_restore_probcut(Search *search, const NodeType node_type, const int selectivity) 
{
	search->node_type[search->height] = node_type;
	if (!USE_RECURSIVE_PROBCUT) search->selectivity = selectivity;
	LIMIT_RECURSIVE_PROBCUT(--search->probcut_level;)
}

static bool search_probcut(Search *search, const int alpha, const int depth, Node *parent, int *value)
{
	// assertion 
	assert(search != NULL);
	assert(parent != NULL);
	assert(search->node_type[search->height] != PV_NODE);
	assert(0 <= search->selectivity && search->selectivity <= NO_SELECTIVITY);

	// do probcut ?
	if (USE_PROBCUT && depth >= options.probcut_d && search->selectivity < NO_SELECTIVITY	LIMIT_RECURSIVE_PROBCUT(&& search->probcut_level < 2)) {
		int probcut_error, eval_error;
		int probcut_depth, probcut_beta, probcut_alpha;
		int eval_score, eval_beta, eval_alpha;
		int score;
		const int beta = alpha + 1;
		double t = selectivity_table[search->selectivity].t;
		const int saved_selectivity = search->selectivity;
		const NodeType node_type = search->node_type[search->height];

		PROBCUT_STATS(++statistics.n_probcut_try);

		// compute reduced depth & associated error
		probcut_depth = 2 * floor(options.probcut_d * depth) + (depth & 1);
		if (probcut_depth == 0) probcut_depth = depth - 2; 
		assert(probcut_depth > 1 && probcut_depth <= depth - 2 && (probcut_depth & 1) == (depth & 1));
		probcut_error = t * eval_sigma(search->eval.n_empties, depth, probcut_depth) + RCD;

		// compute evaluation error (i.e. error at depth 0) averaged for both depths
		eval_score = search_eval_0(search);
		eval_error = t * 0.5 * (eval_sigma(search->eval.n_empties, depth, 0) + eval_sigma(search->eval.n_empties, depth, probcut_depth)) + RCD;
	
		// try a probable upper cut first
		eval_beta = beta - eval_error;
		probcut_beta = beta + probcut_error;
		probcut_alpha = probcut_beta - 1;
		if (eval_score >= eval_beta && probcut_beta < SCORE_MAX) { 	// check if trying a beta probcut is worth
			PROBCUT_STATS(++statistics.n_probcut_high_try);
			search_update_probcut(search, CUT_NODE);
			score = NWS_midgame(search, probcut_alpha, probcut_depth, parent);
			search_restore_probcut(search, node_type, saved_selectivity);
			if (score >= probcut_beta) {
				*value = beta;
				PROBCUT_STATS(++statistics.n_probcut_high_cutoff);
				return true;
			}
		}
		
		// try a probable lower cut if upper cut failed
		eval_alpha = alpha + eval_error;
		probcut_alpha = alpha - probcut_error;
		if (eval_score < eval_alpha && probcut_alpha > SCORE_MIN) { // check if trying an alpha probcut is worth
			PROBCUT_STATS(++statistics.n_probcut_low_try);
			search_update_probcut(search, ALL_NODE);
			score = NWS_midgame(search, probcut_alpha, probcut_depth, parent);
			search_restore_probcut(search, node_type, saved_selectivity);
			if (score <= probcut_alpha) {
				*value = alpha;
				PROBCUT_STATS(++statistics.n_probcut_low_cutoff);
				return true;
			}
		}
	}

	return false;
}

/**
 * @brief Evaluate a midgame position with a Null Window Search algorithm. (No probcut)
 *
 * This function is used when there are still many empty squares on the board. Move
 * ordering, hash table cutoff, enhanced transposition cutoff, etc. are used in
 * order to diminish the size of the tree to analyse, but at the expense of a
 * slower speed.
 * (called from PVS_shallow and NWS_midgame @ depth <= 3)
 *
 * @param search     search.
 * @param alpha      lower bound.
 * @param depth Search remaining depth.
 * @param hash_table Hash Table to use.
 * @return An evaluated score, as a disc difference.
 */
static int NWS_shallow(Search *search, const int alpha, int depth, HashTable *hash_table)
{
	int score, bestscore;
	unsigned long long hash_code;
<<<<<<< HEAD
<<<<<<< HEAD
	// const int beta = alpha + 1;
	HashStoreData hash_data;
=======
	const int beta = alpha + 1;
	HashData hash_data;
>>>>>>> 0a166fd (Remove 1 element array coding style)
=======
	// const int beta = alpha + 1;
	HashData hash_data;
	HashStoreData hash_store_data;
>>>>>>> d1c50ef (Structured hash_store parameters; AVXLASTFLIP changed to opt-in)
	MoveList movelist;
	Move *move;
<<<<<<< HEAD
	Eval eval0;
	V2DI board0;
	long long nodes_org;
=======
	Eval Ev0;
<<<<<<< HEAD
	int bestscore, bestmove;
	long long cost = -search->n_nodes;
>>>>>>> f1d221c (Replace eval_restore with simple save-restore, as well as parity)

	if (depth == 2) return search_eval_2(search, alpha, alpha + 1, board_get_moves(&search->board));
=======
	int bestscore;
	long long nodes_org = search->n_nodes;

<<<<<<< HEAD
	if (depth == 2) return search_eval_2(search, alpha, alpha + 1);
>>>>>>> d1c50ef (Structured hash_store parameters; AVXLASTFLIP changed to opt-in)
=======
	if (depth == 2) return search_eval_2(search, alpha, alpha + 1, get_moves(search->board.player, search->board.opponent));
>>>>>>> 4b9f204 (minor optimize in search_eval_1/2 and search_shallow)

	SEARCH_STATS(++statistics.n_NWS_midgame);
	SEARCH_UPDATE_INTERNAL_NODES(search->n_nodes);

<<<<<<< HEAD
<<<<<<< HEAD
	assert(search->eval.n_empties == bit_count(~(search->board.player | search->board.opponent)));
=======
	assert(search->n_empties == bit_count(~(search->board.player | search->board.opponent)));
>>>>>>> 0a166fd (Remove 1 element array coding style)
=======
	assert(search->eval.n_empties == bit_count(~(search->board.player | search->board.opponent)));
>>>>>>> c8248ad (Move n_empties into Eval; tweak eval_open and eval_set)
	assert(SCORE_MIN <= alpha && alpha <= SCORE_MAX);
	assert(depth > 2);
	assert(hash_table != NULL);

	hash_code = board_get_hash_code(&search->board);
	hash_prefetch(hash_table, hash_code);

	// stability cutoff
	if (search_SC_NWS(search, alpha, &score)) return score;

<<<<<<< HEAD
	search_get_movelist(search, &movelist);
	board0.board = search->board;
	eval0 = search->eval;

	if (movelist.n_moves > 1) {
		// transposition cutoff
		if (hash_get(hash_table, &search->board, hash_code, &hash_data.data) && search_TC_NWS(&hash_data.data, depth, NO_SELECTIVITY, alpha, &score))
			return score;

		// sort the list of moves
		nodes_org = search->n_nodes;
		movelist_evaluate(&movelist, search, &hash_data.data, alpha, depth);
=======
	// transposition cutoff
	hash_code = board_get_hash_code(&search->board);
	if (hash_get(hash_table, &search->board, hash_code, &hash_data) && search_TC_NWS(&hash_data, depth, search->selectivity, alpha, &score)) return score;
	search_get_movelist(search, &movelist);

	if (movelist_is_empty(&movelist)) { // no moves ?
		if (can_move(search->board.opponent, search->board.player)) { // pass ?
			search_update_pass_midgame(search);
			bestscore = -NWS_shallow(search, -(alpha + 1), depth, hash_table);
			hash_store_data.data.move[0] = PASS;
			search_restore_pass_midgame(search);
		} else { // game-over !
			bestscore = search_solve(search);
			hash_store_data.data.move[0] = NOMOVE;
		}
	} else {
		// sort the list of moves
		movelist_evaluate(&movelist, search, &hash_data, alpha, depth);
		movelist_sort(&movelist) ;
>>>>>>> 0a166fd (Remove 1 element array coding style)

		// loop over all moves
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
		bestscore = -SCORE_INF;
		foreach_best_move(move, movelist) {
			search_update_midgame(search, move);
			score = -NWS_shallow(search, ~alpha, depth - 1, hash_table);
			search_restore_midgame(search, move->x, &eval0);
			search->board = board0.board;
=======
		bestscore = -SCORE_INF; bestmove = NOMOVE;
=======
		bestscore = -SCORE_INF; hash_store_data.move = NOMOVE;
<<<<<<< HEAD
>>>>>>> d1c50ef (Structured hash_store parameters; AVXLASTFLIP changed to opt-in)
		Ev0 = search->eval;
=======
=======
		bestscore = -SCORE_INF; hash_store_data.data.move[0] = NOMOVE;
>>>>>>> a556e46 (HashData and HashStoreData rearranged, TYPE_PUNING now uses union)
		Ev0.feature = search->eval.feature;
>>>>>>> 037f46e (New eval_update_leaf updates eval on copy; save-restore eval.feature only)
		foreach_move(move, movelist) {
			search_update_midgame(search, move);
			score = -NWS_shallow(search, -(alpha + 1), depth - 1, hash_table);
			search_restore_midgame(search, move, &Ev0);
>>>>>>> f1d221c (Replace eval_restore with simple save-restore, as well as parity)
			if (score > bestscore) {
				bestscore = score;
<<<<<<< HEAD
<<<<<<< HEAD
				hash_data.data.move[0] = move->x;
=======
				hash_store_data.move = move->x;
>>>>>>> d1c50ef (Structured hash_store parameters; AVXLASTFLIP changed to opt-in)
=======
				hash_store_data.data.move[0] = move->x;
>>>>>>> a556e46 (HashData and HashStoreData rearranged, TYPE_PUNING now uses union)
				if (score > alpha) break;
			}
		}

		// save the best result in hash tables
		hash_data.data.wl.c.depth = depth;
		hash_data.data.wl.c.selectivity = NO_SELECTIVITY;	// (4.5.1)
		hash_data.data.wl.c.cost = last_bit(search->n_nodes - nodes_org);
		// hash_data.data.move[0] = bestmove;
		hash_data.alpha = alpha;
		hash_data.beta = alpha + 1;
		hash_data.score = bestscore;
		hash_store(hash_table, &search->board, hash_code, &hash_data);

	} else if (movelist.n_moves == 1) {
		move = movelist_first(&movelist);
		search_update_midgame(search, move);
		bestscore = -NWS_shallow(search, ~alpha, depth - 1, hash_table);
		search_restore_midgame(search, move->x, &eval0);
		search->board = board0.board;

	} else { // no moves
		if (can_move(search->board.opponent, search->board.player)) { // pass ?
			search_update_pass_midgame(search, &eval0);
			bestscore = -NWS_shallow(search, ~alpha, depth, hash_table);
			search_restore_pass_midgame(search, &eval0);
		} else { // game-over !
			bestscore = search_solve(search);
		}
	}

<<<<<<< HEAD
	assert(SCORE_MIN <= bestscore && bestscore <= SCORE_MAX);
=======
	// save the best result in hash tables
	hash_store_data.data.wl.c.depth = depth;
	hash_store_data.data.wl.c.selectivity = search->selectivity;
	hash_store_data.data.wl.c.cost = last_bit(search->n_nodes - nodes_org);
	// hash_store_data.data.move[0] = bestmove;
	hash_store_data.alpha = alpha;
	hash_store_data.beta = alpha + 1;
	hash_store_data.score = bestscore;
	hash_store(hash_table, &search->board, hash_code, &hash_store_data);
 	assert(SCORE_MIN <= bestscore && bestscore <= SCORE_MAX);

>>>>>>> 0a166fd (Remove 1 element array coding style)
	return bestscore;
}

/**
 * @brief Evaluate a midgame position at shallow depth. (No probcut)
 *
 * This function is used when there are still many empty squares on the board. Move
 * ordering, hash table cutoff, enhanced transposition cutoff, etc. are used in
 * order to diminish the size of the tree to analyse, but at the expense of a
 * slower speed.
 * (called from guess_move @ depth <= 6 and movelist_evaluate @ depth > 2)
 *
 * @param search Search.
 * @param alpha Alpha bound.
 * @param beta Beta bound.
 * @param depth Search depth.
 * @return An evaluated score, as a disc difference.
 */
int PVS_shallow(Search *search, int alpha, int beta, int depth)
{
<<<<<<< HEAD
	int score, bestscore, lower;
	// unsigned long long hash_code;
	HashStoreData hash_data;
=======
	int score;
	HashTable *const hash_table = &search->shallow_table;
	unsigned long long hash_code;
	HashData hash_data;
<<<<<<< HEAD
>>>>>>> 0a166fd (Remove 1 element array coding style)
=======
	HashStoreData hash_store_data;
>>>>>>> d1c50ef (Structured hash_store parameters; AVXLASTFLIP changed to opt-in)
	MoveList movelist;
	Move *move;
<<<<<<< HEAD
	Eval eval0;
	Board board0;
	long long nodes_org;
=======
	Eval Ev0;
	int bestscore;
	long long nodes_org = search->n_nodes;
	int lower;
>>>>>>> f1d221c (Replace eval_restore with simple save-restore, as well as parity)

<<<<<<< HEAD
	if (depth == 2) return search_eval_2(search, alpha, beta, board_get_moves(&search->board));
=======
	if (depth == 2) return search_eval_2(search, alpha, beta, get_moves(search->board.player, search->board.opponent));
>>>>>>> 4b9f204 (minor optimize in search_eval_1/2 and search_shallow)

	SEARCH_STATS(++statistics.n_PVS_shallow);
	SEARCH_UPDATE_INTERNAL_NODES(search->n_nodes);

<<<<<<< HEAD
<<<<<<< HEAD
	assert(search->eval.n_empties == bit_count(~(search->board.player | search->board.opponent)));
	assert(SCORE_MIN <= alpha && alpha <= SCORE_MAX);

	// stability cutoff
	if (USE_SC && beta >= PVS_STABILITY_THRESHOLD[search->eval.n_empties]) {
		CUTOFF_STATS(++statistics.n_stability_try;)
		score = SCORE_MAX - 2 * get_stability(search->board.opponent, search->board.player);
		if (score <= alpha) {
			CUTOFF_STATS(++statistics.n_stability_low_cutoff;)
			return score;
=======
	assert(search->n_empties == bit_count(~(search->board.player | search->board.opponent)));
=======
	assert(search->eval.n_empties == bit_count(~(search->board.player | search->board.opponent)));
>>>>>>> c8248ad (Move n_empties into Eval; tweak eval_open and eval_set)
	assert(SCORE_MIN <= alpha && alpha <= SCORE_MAX);

	// stability cutoff
	if (search_SC_PVS(search, &alpha, &beta, &score)) return score;

	// transposition cutoff
	hash_code = board_get_hash_code(&search->board);
//	if (hash_get(hash_table, &search->board, hash_code, &hash_data) && search_TC_PVS(&hash_data, depth, search->selectivity, &alpha, &beta, &score)) return score;
	hash_get(hash_table, &search->board, hash_code, &hash_data);

	search_get_movelist(search, &movelist);

	if (movelist_is_empty(&movelist)) { // no moves ?
		if (can_move(search->board.opponent, search->board.player)) { // pass ?
			search_update_pass_midgame(search);
			bestscore = -PVS_shallow(search, -beta, -alpha, depth);
			hash_store_data.data.move[0] = PASS;
			search_restore_pass_midgame(search);
		} else { // game-over !
			bestscore = search_solve(search);
<<<<<<< HEAD
<<<<<<< HEAD
			bestmove = NOMOVE;
>>>>>>> 0a166fd (Remove 1 element array coding style)
=======
			hash_store_data.move = NOMOVE;
>>>>>>> d1c50ef (Structured hash_store parameters; AVXLASTFLIP changed to opt-in)
=======
			hash_store_data.data.move[0] = NOMOVE;
>>>>>>> a556e46 (HashData and HashStoreData rearranged, TYPE_PUNING now uses union)
		}
		else if (score < beta) beta = score;
	}

	search_get_movelist(search, &movelist);
	board0 = search->board;
	eval0 = search->eval;

	if (movelist.n_moves > 1) {
		// transposition cutoff (unused, normally first searched position)
		// hash_code = board_get_hash_code(&search->board);
		// if (hash_get(&search->shallow_table, &search->board, hash_code, &hash_data.data) && search_TC_PVS(&hash_data.data, depth, NO_SELECTIVITY, &alpha, &beta, &score)) return score;

		// sort the list of moves
<<<<<<< HEAD
		nodes_org = search->n_nodes;
		movelist_evaluate(&movelist, search, &HASH_DATA_INIT, alpha, depth);
=======
		movelist_evaluate(&movelist, search, &hash_data, alpha, depth);
		movelist_sort(&movelist) ;
>>>>>>> 0a166fd (Remove 1 element array coding style)

		// loop over all moves
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
		move = movelist_best(&movelist);
		search_update_midgame(search, move);
		bestscore = -PVS_shallow(search, -beta, -alpha, depth - 1);
		hash_data.data.move[0] = move->x;
		search_restore_midgame(search, move->x, &eval0);
		search->board = board0;
		lower = (bestscore > alpha) ? bestscore : alpha;

		while ((bestscore < beta) && (move = move_next_best(move))) {
			search_update_midgame(search, move);
			score = -NWS_shallow(search, ~lower, depth - 1, &search->shallow_table);
			if (lower < score && score < beta)
				lower = score = -PVS_shallow(search, -beta, -lower, depth - 1);
			search_restore_midgame(search, move->x, &eval0);
			search->board = board0;
=======
		bestscore = -SCORE_INF; bestmove = NOMOVE;
=======
		bestscore = -SCORE_INF; hash_store_data.move = NOMOVE;
>>>>>>> d1c50ef (Structured hash_store parameters; AVXLASTFLIP changed to opt-in)
=======
		bestscore = -SCORE_INF; hash_store_data.data.move[0] = NOMOVE;
>>>>>>> a556e46 (HashData and HashStoreData rearranged, TYPE_PUNING now uses union)
		lower = alpha;
		Ev0.feature = search->eval.feature;
		foreach_move(move, movelist) {
			search_update_midgame(search, move);
				if (bestscore == -SCORE_INF) {
					score = -PVS_shallow(search, -beta, -lower, depth - 1);
				} else {
					score = -NWS_shallow(search, -lower - 1, depth - 1, hash_table);
					if (alpha < score && score < beta) {
						score = -PVS_shallow(search, -beta, -lower, depth - 1);
					}
				}
			search_restore_midgame(search, move, &Ev0);
>>>>>>> f1d221c (Replace eval_restore with simple save-restore, as well as parity)
			if (score > bestscore) {
				bestscore = score;
<<<<<<< HEAD
<<<<<<< HEAD
				hash_data.data.move[0] = move->x;
=======
				hash_store_data.move = move->x;
=======
				hash_store_data.data.move[0] = move->x;
>>>>>>> a556e46 (HashData and HashStoreData rearranged, TYPE_PUNING now uses union)
				if (score >= beta) break;
				else if (score > lower) lower = score;
>>>>>>> d1c50ef (Structured hash_store parameters; AVXLASTFLIP changed to opt-in)
			}
		}

		// save the best result in shallow hash
		hash_data.data.wl.c.depth = depth;
		hash_data.data.wl.c.selectivity = NO_SELECTIVITY;	// (4.5.1)
		hash_data.data.wl.c.cost = last_bit(search->n_nodes - nodes_org);
		// hash_data.data.move[0] = bestmove;
		hash_data.alpha = alpha;
		hash_data.beta = beta;
		hash_data.score = bestscore;
		hash_store(&search->shallow_table, &search->board, board_get_hash_code(&search->board), &hash_data);

	} else if (movelist.n_moves == 1) {
		move = movelist.move[0].next;
		search_update_midgame(search, move);
		bestscore = -PVS_shallow(search, -beta, -alpha, depth - 1);
		search_restore_midgame(search, move->x, &eval0);
		search->board = board0;

	} else { // no moves
		if (can_move(search->board.opponent, search->board.player)) { // pass ?
			search_update_pass_midgame(search, &eval0);
			bestscore = -PVS_shallow(search, -beta, -alpha, depth);
			search_restore_pass_midgame(search, &eval0);
		} else { // game-over !
			bestscore = search_solve(search);
		}
	}

<<<<<<< HEAD
	assert(SCORE_MIN <= bestscore && bestscore <= SCORE_MAX);
=======
	// save the best result in hash tables
	hash_store_data.data.wl.c.depth = depth;
	hash_store_data.data.wl.c.selectivity = search->selectivity;
	hash_store_data.data.wl.c.cost = last_bit(search->n_nodes - nodes_org);
	// hash_store_data.data.move[0] = bestmove;
	hash_store_data.alpha = alpha;
	hash_store_data.beta = beta;
	hash_store_data.score = bestscore;
	hash_store(hash_table, &search->board, hash_code, &hash_store_data);
 	assert(SCORE_MIN <= bestscore && bestscore <= SCORE_MAX);

>>>>>>> 0a166fd (Remove 1 element array coding style)
	return bestscore;
}


/**
 * @brief Evaluate a midgame position with a Null Window Search algorithm.
 *
 * This function is used when there are still many empty squares on the board. Move
 * ordering, hash table cutoff, enhanced transposition cutoff, etc. are used in
 * order to diminish the size of the tree to analyse, but at the expense of a
 * slower speed.
 *
 * @param search Search.
 * @param alpha Alpha bound.
 * @param depth Depth.
 * @param parent Parent node.
 * @return A score, as a disc difference.
 */
int NWS_midgame(Search *search, const int alpha, int depth, Node *parent)
{
	int score;
<<<<<<< HEAD
	unsigned long long hash_code;
	// const int beta = alpha + 1;
	HashStoreData hash_data;
	MoveList movelist;
	Move *move;
<<<<<<< HEAD
	Node node;
	Eval eval0;
	V2DI board0;
	long long nodes_org;
=======
	Node node[1];
=======
	HashTable *const hash_table = &search->hash_table;
	HashTable *const pv_table = &search->pv_table;
	unsigned long long hash_code;
	const int beta = alpha + 1;
	HashData hash_data;
	HashStoreData hash_store_data;
	MoveList movelist;
	Move *move;
	Node node;
>>>>>>> 0a166fd (Remove 1 element array coding style)
	Eval Ev0;
<<<<<<< HEAD
	long long cost = -search->n_nodes - search->child_nodes;
	int cost_bits;
	int hash_selectivity;
>>>>>>> f1d221c (Replace eval_restore with simple save-restore, as well as parity)
=======
	long long nodes_org = search->n_nodes + search->child_nodes;
>>>>>>> d1c50ef (Structured hash_store parameters; AVXLASTFLIP changed to opt-in)

<<<<<<< HEAD
<<<<<<< HEAD
	assert(search->eval.n_empties == bit_count(~(search->board.player | search->board.opponent)));
	assert((2 <= depth && depth < search->eval.n_empties) || depth == search->eval.n_empties);
=======
	assert(search->n_empties == bit_count(~(search->board.player | search->board.opponent)));
	assert((2 <= depth && depth < search->n_empties) || depth == search->n_empties);
>>>>>>> 0a166fd (Remove 1 element array coding style)
=======
	assert(search->eval.n_empties == bit_count(~(search->board.player | search->board.opponent)));
	assert((2 <= depth && depth < search->eval.n_empties) || depth == search->eval.n_empties);
>>>>>>> c8248ad (Move n_empties into Eval; tweak eval_open and eval_set)
	assert(SCORE_MIN <= alpha && alpha <= SCORE_MAX);
	assert(parent != NULL);

	search_check_timeout(search);
<<<<<<< HEAD
	if (search->stop)
		return alpha;

	if (search->eval.n_empties == 0)
		return search_solve_0(search);
	else if (depth < search->eval.n_empties) {
		if (depth <= 3)
			return NWS_shallow(search, alpha, depth, &search->hash_table);
	} else {
		if (depth < DEPTH_MIDGAME_TO_ENDGAME)
			return NWS_endgame(search, alpha);
	}
=======
	if (search->stop) return alpha;
	else if (search->eval.n_empties == 0) return search_solve_0(search);
	else if (depth <= 3 && depth < search->eval.n_empties) return NWS_shallow(search, alpha, depth, hash_table);
	else if (search->eval.n_empties <= depth && depth < DEPTH_MIDGAME_TO_ENDGAME) return NWS_endgame(search, alpha);
>>>>>>> c8248ad (Move n_empties into Eval; tweak eval_open and eval_set)

	SEARCH_STATS(++statistics.n_NWS_midgame);
	SEARCH_UPDATE_INTERNAL_NODES(search->n_nodes);

	// stability cutoff
	if (search_SC_NWS(search, alpha, &score)) return score;

	hash_code = board_get_hash_code(&search->board);
	hash_prefetch(&search->hash_table, hash_code);
	hash_prefetch(&search->pv_table, hash_code);

	nodes_org = search->n_nodes + search->child_nodes;
	search_get_movelist(search, &movelist);

	// transposition cutoff
<<<<<<< HEAD
	if (hash_get(&search->hash_table, &search->board, hash_code, &hash_data.data) || hash_get(&search->pv_table, &search->board, hash_code, &hash_data.data))
		if (search_TC_NWS(&hash_data.data, depth, search->selectivity, alpha, &score)) return score;

	if (movelist_is_empty(&movelist)) { // no moves ?
		node_init(&node, search, alpha, alpha + 1, depth, movelist.n_moves, parent);
		if (can_move(search->board.opponent, search->board.player)) { // pass ?
			search_update_pass_midgame(search, &eval0);
			node.bestscore = -NWS_midgame(search, -node.beta, depth, &node);
			search_restore_pass_midgame(search, &eval0);
=======
	hash_code = board_get_hash_code(&search->board);
	if ((hash_get(hash_table, &search->board, hash_code, &hash_data) || hash_get(pv_table, &search->board, hash_code, &hash_data)) && search_TC_NWS(&hash_data, depth, search->selectivity, alpha, &score)) return score;

	search_get_movelist(search, &movelist);

	if (movelist_is_empty(&movelist)) { // no moves ?
		node_init(&node, search, alpha, beta, depth, movelist.n_moves, parent);
		if (can_move(search->board.opponent, search->board.player)) { // pass ?
			search_update_pass_midgame(search);
			node.bestscore = -NWS_midgame(search, -node.beta, depth, &node);
			search_restore_pass_midgame(search);
>>>>>>> 0a166fd (Remove 1 element array coding style)
		} else { // game-over !
			node.bestscore = search_solve(search);
		}

	} else {
		// probcut
		if (search_probcut(search, alpha, depth, parent, &score)) return score;

		// sort the list of moves
		if (movelist.n_moves > 1) {
<<<<<<< HEAD
			if (hash_data.data.move[0] == NOMOVE) hash_get(&search->hash_table, &search->board, hash_code, &hash_data.data);
			movelist_evaluate(&movelist, search, &hash_data.data, alpha, depth + options.inc_sort_depth[search->node_type[search->height]]);
			movelist_sort(&movelist);
=======
			if (hash_data.move[0] == NOMOVE) hash_get(hash_table, &search->board, hash_code, &hash_data);
			movelist_evaluate(&movelist, search, &hash_data, alpha, depth + options.inc_sort_depth[search->node_type[search->height]]);
			movelist_sort(&movelist) ;
>>>>>>> 0a166fd (Remove 1 element array coding style)
		}

		// ETC
		if (search_ETC_NWS(search, &movelist, hash_code, depth, search->selectivity, alpha, &score)) return score;

<<<<<<< HEAD
		node_init(&node, search, alpha, alpha + 1, depth, movelist.n_moves, parent);
=======
		node_init(&node, search, alpha, beta, depth, movelist.n_moves, parent);
>>>>>>> 0a166fd (Remove 1 element array coding style)

		// loop over all moves
<<<<<<< HEAD
<<<<<<< HEAD
		board0.board = search->board;
		eval0 = search->eval;
		for (move = node_first_move(&node, &movelist); move; move = node_next_move(&node)) {
			if (!node_split(&node, move)) {
				search_update_midgame(search, move);
				move->score = -NWS_midgame(search, ~alpha, depth - 1, &node);
				search_restore_midgame(search, move->x, &eval0);
				search->board = board0.board;
				node_update(&node, move);
=======
		Ev0 = search->eval;
=======
		Ev0.feature = search->eval.feature;
>>>>>>> 037f46e (New eval_update_leaf updates eval on copy; save-restore eval.feature only)
		for (move = node_first_move(&node, &movelist); move; move = node_next_move(&node)) {
			if (!node_split(&node, move)) {
				search_update_midgame(search, move);
				move->score = -NWS_midgame(search, -beta, depth - 1, &node);
				search_restore_midgame(search, move, &Ev0);
<<<<<<< HEAD
				node_update(node, move);
>>>>>>> f1d221c (Replace eval_restore with simple save-restore, as well as parity)
=======
				node_update(&node, move);
>>>>>>> 0a166fd (Remove 1 element array coding style)
			}
		}
		node_wait_slaves(&node);
	}

	// save the best result in hash tables
	if (!search->stop) {
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
		if (depth <= ((search->eval.n_empties <= depth) ? DEPTH_MIDGAME_TO_ENDGAME : 4))
			hash_data.data.wl.c.selectivity = NO_SELECTIVITY; // hack
		else hash_data.data.wl.c.selectivity = search->selectivity;
		hash_data.data.wl.c.depth = depth;
		hash_data.data.wl.c.cost = last_bit(search->n_nodes + search->child_nodes - nodes_org);
		hash_data.data.move[0] = node.bestmove;
		hash_data.alpha = alpha;
		hash_data.beta = alpha + 1;
		hash_data.score = node.bestscore;

		if (search->height <= PV_HASH_HEIGHT) hash_store(&search->pv_table, &search->board, hash_code, &hash_data);
		hash_store(&search->hash_table, &search->board, hash_code, &hash_data);

		SQUARE_STATS(foreach_move(move, &movelist))
		SQUARE_STATS(++statistics.n_played_square[search->eval.n_empties][SQUARE_TYPE[move->x]];)
		SQUARE_STATS(if (node.bestscore > alpha) ++statistics.n_good_square[search->eval.n_empties][SQUARE_TYPE[node->bestmove]];)

	 	assert(SCORE_MIN <= node.bestscore && node.bestscore <= SCORE_MAX);

=======
		cost += search->n_nodes + search->child_nodes;
		cost_bits = last_bit(cost);
		if (search->n_empties < depth && depth <= DEPTH_MIDGAME_TO_ENDGAME) hash_selectivity = NO_SELECTIVITY; // hack
		else hash_selectivity = search->selectivity;
		if (search->height <= PV_HASH_HEIGHT) hash_store(pv_table, &search->board, hash_code, depth, hash_selectivity, cost_bits, alpha, beta, node.bestscore, node.bestmove);
		hash_store(hash_table, &search->board, hash_code, depth, hash_selectivity, cost_bits, alpha, beta, node.bestscore, node.bestmove);
=======
		if (search->n_empties < depth && depth <= DEPTH_MIDGAME_TO_ENDGAME) hash_store_data.data.selectivity = NO_SELECTIVITY; // hack
		else hash_store_data.data.selectivity = search->selectivity;
		hash_store_data.data.depth = depth;
		hash_store_data.data.cost = last_bit(search->n_nodes + search->child_nodes - nodes_org);
=======
		if (search->n_empties < depth && depth <= DEPTH_MIDGAME_TO_ENDGAME) hash_store_data.data.wl.c.selectivity = NO_SELECTIVITY; // hack
=======
		if (search->eval.n_empties < depth && depth <= DEPTH_MIDGAME_TO_ENDGAME) hash_store_data.data.wl.c.selectivity = NO_SELECTIVITY; // hack
>>>>>>> c8248ad (Move n_empties into Eval; tweak eval_open and eval_set)
		else hash_store_data.data.wl.c.selectivity = search->selectivity;
		hash_store_data.data.wl.c.depth = depth;
		hash_store_data.data.wl.c.cost = last_bit(search->n_nodes + search->child_nodes - nodes_org);
		hash_store_data.data.move[0] = node.bestmove;
>>>>>>> a556e46 (HashData and HashStoreData rearranged, TYPE_PUNING now uses union)
		hash_store_data.alpha = alpha;
		hash_store_data.beta = beta;
		hash_store_data.score = node.bestscore;

		if (search->height <= PV_HASH_HEIGHT) hash_store(pv_table, &search->board, hash_code, &hash_store_data);
		hash_store(hash_table, &search->board, hash_code, &hash_store_data);

>>>>>>> d1c50ef (Structured hash_store parameters; AVXLASTFLIP changed to opt-in)
		SQUARE_STATS(foreach_move(move, &movelist))
		SQUARE_STATS(++statistics.n_played_square[search->eval.n_empties][SQUARE_TYPE[move->x]];)
		SQUARE_STATS(if (node.bestscore > alpha) ++statistics.n_good_square[search->eval.n_empties][SQUARE_TYPE[node->bestmove]];)

	 	assert(SCORE_MIN <= node.bestscore && node.bestscore <= SCORE_MAX);
>>>>>>> 0a166fd (Remove 1 element array coding style)
	} else {
		node.bestscore = alpha;
	}

	node_free(&node);

	return node.bestscore;
}


/**
 * @brief Evaluate a position with a deep Principal Variation Search algorithm.
 *
 * This function is used when there are still many empty squares on the board. Move
 * ordering, hash table cutoff, enhanced transposition cutoff, etc. are used in
 * order to diminish the size of the tree to analyse, but at the expense of a
 * slower speed.
 *
 * @param search Search.
 * @param alpha Lower bound.
 * @param beta Upper bound.
 * @param depth Depth.
 * @param parent Parent node.
 * @return A score, as a disc difference.
 */
int PVS_midgame(Search *search, const int alpha, const int beta, int depth, Node *parent)
{
	// declaration
<<<<<<< HEAD
	unsigned long long hash_code, solid_opp;
	HashStoreData hash_data;
	MoveList movelist;
	Move *move;
<<<<<<< HEAD
	Node node;
	Eval eval0;
	Board board0, hashboard;
	long long nodes_org;
	int reduced_depth, depth_pv_extension, saved_selectivity, ofssolid;
=======
	Node node[1];
=======
	HashTable *const hash_table = &search->hash_table;
	HashTable *const pv_table = &search->pv_table;
	unsigned long long hash_code;
	HashData hash_data;
	HashStoreData hash_store_data;
	MoveList movelist;
	Move *move;
	Node node;
>>>>>>> 0a166fd (Remove 1 element array coding style)
	Eval Ev0;
	long long nodes_org;
	int reduced_depth, depth_pv_extension, saved_selectivity;
<<<<<<< HEAD
	int hash_selectivity;
>>>>>>> f1d221c (Replace eval_restore with simple save-restore, as well as parity)
=======
>>>>>>> d1c50ef (Structured hash_store parameters; AVXLASTFLIP changed to opt-in)

	SEARCH_STATS(++statistics.n_PVS_midgame);

<<<<<<< HEAD
<<<<<<< HEAD
	assert(search->eval.n_empties == bit_count(~(search->board.player | search->board.opponent)));
	assert(depth <= search->eval.n_empties);
=======
	assert(search->n_empties == bit_count(~(search->board.player | search->board.opponent)));
	assert(depth <= search->n_empties);
>>>>>>> 0a166fd (Remove 1 element array coding style)
=======
	assert(search->eval.n_empties == bit_count(~(search->board.player | search->board.opponent)));
	assert(depth <= search->eval.n_empties);
>>>>>>> c8248ad (Move n_empties into Eval; tweak eval_open and eval_set)
	assert((-SCORE_MAX <= alpha && alpha <= SCORE_MAX) || printf("alpha = %d\n", alpha));
	assert((-SCORE_MAX <= beta && beta <= SCORE_MAX) || printf("beta = %d\n", beta));
	assert(alpha <= beta);

	// end of search ?
	search_check_timeout(search);
	if (search->stop) return alpha;
<<<<<<< HEAD
<<<<<<< HEAD
	else if (search->eval.n_empties == 0)
		return search_solve_0(search);
	else if (USE_PV_EXTENSION && search->eval.n_empties <= search->depth_pv_extension)
		depth = search->eval.n_empties;
	else if (depth == 2 && search->eval.n_empties > 2)
		return search_eval_2(search, alpha, beta, board_get_moves(&search->board));
=======
	else if (search->n_empties == 0)
=======
	else if (search->eval.n_empties == 0)
>>>>>>> c8248ad (Move n_empties into Eval; tweak eval_open and eval_set)
		return search_solve_0(search);
	else if (USE_PV_EXTENSION && depth < search->eval.n_empties && search->eval.n_empties <= search->depth_pv_extension)
		return PVS_midgame(search, alpha, beta, search->eval.n_empties, parent);
	else if (depth == 2 && search->eval.n_empties > 2)
		return search_eval_2(search, alpha, beta, get_moves(search->board.player, search->board.opponent));
>>>>>>> 4b9f204 (minor optimize in search_eval_1/2 and search_shallow)

<<<<<<< HEAD
<<<<<<< HEAD
	nodes_org = search_count_nodes(search);
=======
	cost = -search_count_nodes(search);
>>>>>>> 1b29848 (fix & optimize 32 bit build; other minor mods)
=======
	nodes_org = search_count_nodes(search);
>>>>>>> d1c50ef (Structured hash_store parameters; AVXLASTFLIP changed to opt-in)
	SEARCH_UPDATE_INTERNAL_NODES(search->n_nodes);

	search_get_movelist(search, &movelist);
	node_init(&node, search, alpha, beta, depth, movelist.n_moves, parent);
	node.pv_node = true;
	hash_code = board_get_hash_code(&search->board);

	// special cases
	if (movelist_is_empty(&movelist)) {
		if (can_move(search->board.opponent, search->board.player)) {
<<<<<<< HEAD
			search_update_pass_midgame(search, &eval0);
			search->node_type[search->height] = PV_NODE;
			node.bestscore = -PVS_midgame(search, -beta, -alpha, depth, &node);
			search_restore_pass_midgame(search, &eval0);
=======
			search_update_pass_midgame(search); search->node_type[search->height] = PV_NODE;
			node.bestscore = -PVS_midgame(search, -beta, -alpha, depth, &node);
			search_restore_pass_midgame(search);
>>>>>>> 0a166fd (Remove 1 element array coding style)
			node.bestmove = PASS;
		} else {
			node.alpha = -(node.beta = +SCORE_INF);
			node.bestscore = search_solve(search);
			node.bestmove = NOMOVE;
		}

	} else { // normal PVS
		if (movelist.n_moves > 1) {
			//IID
<<<<<<< HEAD
			if (!hash_get(&search->pv_table, &search->board, hash_code, &hash_data.data))
				hash_get(&search->hash_table, &search->board, hash_code, &hash_data.data);

			if (USE_IID && hash_data.data.move[0] == NOMOVE) {	// (unused)
				if (depth == search->eval.n_empties) reduced_depth = depth - ITERATIVE_MIN_EMPTIES;
=======
			if (!hash_get(pv_table, &search->board, hash_code, &hash_data)) hash_get(hash_table, &search->board, hash_code, &hash_data);
			if (USE_IID && hash_data.move[0] == NOMOVE) {
<<<<<<< HEAD
				if (depth == search->n_empties) reduced_depth = depth - ITERATIVE_MIN_EMPTIES;
>>>>>>> 0a166fd (Remove 1 element array coding style)
=======
				if (depth == search->eval.n_empties) reduced_depth = depth - ITERATIVE_MIN_EMPTIES;
>>>>>>> c8248ad (Move n_empties into Eval; tweak eval_open and eval_set)
				else reduced_depth = depth - 2;
				if (reduced_depth >= 3) {
					saved_selectivity = search->selectivity; search->selectivity = 0;
					depth_pv_extension = search->depth_pv_extension;
					search->depth_pv_extension = 0;
					PVS_midgame(search, SCORE_MIN, SCORE_MAX, reduced_depth, parent);
<<<<<<< HEAD
					hash_get(&search->pv_table, &search->board, hash_code, &hash_data.data);
=======
					hash_get(pv_table, &search->board, hash_code, &hash_data);
>>>>>>> 0a166fd (Remove 1 element array coding style)
					search->depth_pv_extension = depth_pv_extension;
					search->selectivity = saved_selectivity;
				}
			}

			// Evaluate moves for sorting. For a better ordering, the depth is artificially increased
<<<<<<< HEAD
			movelist_evaluate(&movelist, search, &hash_data.data, node.alpha, depth + options.inc_sort_depth[PV_NODE]);
			movelist_sort(&movelist);
=======
			movelist_evaluate(&movelist, search, &hash_data, node.alpha, depth + options.inc_sort_depth[PV_NODE]);
			movelist_sort(&movelist) ;
>>>>>>> 0a166fd (Remove 1 element array coding style)
		}

		// first move
<<<<<<< HEAD
<<<<<<< HEAD
		board0 = search->board;
		eval0 = search->eval;
		if ((move = node_first_move(&node, &movelist))) { // why if there ?
			search_update_midgame(search, move); search->node_type[search->height] = PV_NODE;
			move->score = -PVS_midgame(search, -beta, -alpha, depth - 1, &node);
			search_restore_midgame(search, move->x, &eval0);
			search->board = board0;
			node_update(&node, move);
=======
		Ev0 = search->eval;
=======
		Ev0.feature = search->eval.feature;
>>>>>>> 037f46e (New eval_update_leaf updates eval on copy; save-restore eval.feature only)
		if ((move = node_first_move(&node, &movelist))) { // why if there ?
			search_update_midgame(search, move); search->node_type[search->height] = PV_NODE;
			move->score = -PVS_midgame(search, -beta, -alpha, depth - 1, &node);
			search_restore_midgame(search, move, &Ev0);
<<<<<<< HEAD
			node_update(node, move);
>>>>>>> f1d221c (Replace eval_restore with simple save-restore, as well as parity)
=======
			node_update(&node, move);
>>>>>>> 0a166fd (Remove 1 element array coding style)

			// other moves : try to refute the first/best one
			while ((move = node_next_move(&node))) {
				if (!node_split(&node, move)) {
					const int alpha = node.alpha;
					search_update_midgame(search, move);
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
=======
>>>>>>> 4b9f204 (minor optimize in search_eval_1/2 and search_shallow)
					move->score = -NWS_midgame(search, -alpha - 1, depth - 1, &node);
					if (!search->stop && alpha < move->score && move->score < beta) {
						search->node_type[search->height] = PV_NODE;
						move->score = -PVS_midgame(search, -beta, -alpha, depth - 1, &node);
					}
<<<<<<< HEAD
					search_restore_midgame(search, move->x, &eval0);
					search->board = board0;
					node_update(&node, move);
=======
						move->score = -NWS_midgame(search, -alpha - 1, depth - 1, node);
=======
						move->score = -NWS_midgame(search, -alpha - 1, depth - 1, &node);
>>>>>>> 0a166fd (Remove 1 element array coding style)
						if (!search->stop && alpha < move->score && move->score < beta) {
							search->node_type[search->height] = PV_NODE;
							move->score = -PVS_midgame(search, -beta, -alpha, depth - 1, &node);
						}
=======
>>>>>>> 4b9f204 (minor optimize in search_eval_1/2 and search_shallow)
					search_restore_midgame(search, move, &Ev0);
<<<<<<< HEAD
					node_update(node, move);
>>>>>>> f1d221c (Replace eval_restore with simple save-restore, as well as parity)
=======
					node_update(&node, move);
>>>>>>> 0a166fd (Remove 1 element array coding style)
				}
			}
			node_wait_slaves(&node);
		}
	}

	// save the best result in hash tables
	if (!search->stop) {
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
		if (depth <= ((search->eval.n_empties <= depth) ? DEPTH_MIDGAME_TO_ENDGAME : 4))
			hash_data.data.wl.c.selectivity = NO_SELECTIVITY;
		else	hash_data.data.wl.c.selectivity = search->selectivity;
		hash_data.data.wl.c.depth = depth;
		hash_data.data.wl.c.cost = last_bit(search_count_nodes(search) - nodes_org);
		hash_data.data.move[0] = node.bestmove;
		hash_data.alpha = alpha;
		hash_data.beta = beta;
		hash_data.score = node.bestscore;

		hash_store(&search->hash_table, &search->board, hash_code, &hash_data);
		hash_store(&search->pv_table, &search->board, hash_code, &hash_data);

		// store solid-normalized for endgame TC
		if (search->eval.n_empties <= depth && depth <= MASK_SOLID_DEPTH && depth > DEPTH_TO_SHALLOW_SEARCH) {
			solid_opp = get_all_full_lines(search->board.player | search->board.opponent) & search->board.opponent;
			if (solid_opp) {
				hashboard.player = search->board.player ^ solid_opp;	// normalize solid to player
				hashboard.opponent = search->board.opponent ^ solid_opp;
				ofssolid = bit_count(solid_opp) * 2;	// hash score is ofssolid grater than real
				hash_data.alpha += ofssolid;
				hash_data.beta += ofssolid;
				hash_data.score += ofssolid;
				hash_store(&search->hash_table, &hashboard, board_get_hash_code(&hashboard), &hash_data);
			}
		}

		SQUARE_STATS(foreach_move(move, movelist))
		SQUARE_STATS(++statistics.n_played_square[search->eval.n_empties][SQUARE_TYPE[move->x]];)
		SQUARE_STATS(if (node.bestscore > alpha) ++statistics.n_good_square[search->eval.n_empties][SQUARE_TYPE[node.bestmove]];)
=======
		cost += search_count_nodes(search);
		cost_bits = last_bit(cost);
		if (search->n_empties < depth && depth <= DEPTH_MIDGAME_TO_ENDGAME) hash_selectivity = NO_SELECTIVITY;
		else hash_selectivity = search->selectivity;
		hash_store(hash_table, &search->board, hash_code, depth, hash_selectivity, cost_bits, alpha, beta, node.bestscore, node.bestmove);
		hash_store(pv_table, &search->board, hash_code, depth, hash_selectivity, cost_bits, alpha, beta, node.bestscore, node.bestmove);
=======
		if (search->n_empties < depth && depth <= DEPTH_MIDGAME_TO_ENDGAME) hash_store_data.data.selectivity = NO_SELECTIVITY;
		else hash_store_data.data.selectivity = search->selectivity;
		hash_store_data.data.depth = depth;
		hash_store_data.data.cost = last_bit(search_count_nodes(search) - nodes_org);
=======
		if (search->n_empties < depth && depth <= DEPTH_MIDGAME_TO_ENDGAME) hash_store_data.data.wl.c.selectivity = NO_SELECTIVITY;
=======
		if (search->eval.n_empties < depth && depth <= DEPTH_MIDGAME_TO_ENDGAME) hash_store_data.data.wl.c.selectivity = NO_SELECTIVITY;
>>>>>>> c8248ad (Move n_empties into Eval; tweak eval_open and eval_set)
		else hash_store_data.data.wl.c.selectivity = search->selectivity;
		hash_store_data.data.wl.c.depth = depth;
		hash_store_data.data.wl.c.cost = last_bit(search_count_nodes(search) - nodes_org);
		hash_store_data.data.move[0] = node.bestmove;
>>>>>>> a556e46 (HashData and HashStoreData rearranged, TYPE_PUNING now uses union)
		hash_store_data.alpha = alpha;
		hash_store_data.beta = beta;
		hash_store_data.score = node.bestscore;

		hash_store(hash_table, &search->board, hash_code, &hash_store_data);
		hash_store(pv_table, &search->board, hash_code, &hash_store_data);
>>>>>>> d1c50ef (Structured hash_store parameters; AVXLASTFLIP changed to opt-in)

		SQUARE_STATS(foreach_move(move, movelist))
<<<<<<< HEAD
		SQUARE_STATS(++statistics.n_played_square[search->n_empties][SQUARE_TYPE[move->x]];)
		SQUARE_STATS(if (node.bestscore > alpha) ++statistics.n_good_square[search->n_empties][SQUARE_TYPE[node.bestmove]];)
>>>>>>> 0a166fd (Remove 1 element array coding style)
=======
		SQUARE_STATS(++statistics.n_played_square[search->eval.n_empties][SQUARE_TYPE[move->x]];)
		SQUARE_STATS(if (node.bestscore > alpha) ++statistics.n_good_square[search->eval.n_empties][SQUARE_TYPE[node.bestmove]];)
>>>>>>> c8248ad (Move n_empties into Eval; tweak eval_open and eval_set)

	 	assert(SCORE_MIN <= node.bestscore && node.bestscore <= SCORE_MAX);

	} else {
		node.bestscore = alpha;
	}

	node_free(&node);

	return node.bestscore;
}
