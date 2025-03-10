/**
 * @file endgame.c
 *
 * Search near the end of the game.
 *
 * @date 1998 - 2024
 * @author Richard Delorme
 * @author Toshihiko Okuhara
 * @version 4.6
 */


#include "search.h"

#include "bit.h"
#include "hash.h"
#include "settings.h"
#include "stats.h"

#include <assert.h>
#include <stdbool.h>
#include <stdint.h>

/**
 * @brief Get the final score.
 *
 * Get the final score, when no move can be made.
 *
 * @param player player's discs.
 * @param n_empties Number of empty squares remaining on the board.
 * @return The final score, as a disc difference.
 */
static inline int solve(const uint64_t player, const int n_empties)
{
	int score = bit_count(player) * 2 - SCORE_MAX;	// score in case opponent wins, ie diff < 0 => score == diff - n_empies
	int diff = score + n_empties;		// => diff

	SEARCH_STATS(++statistics.n_solve);

	if (diff == 0) score = diff; // draw score == 0
	else if (diff > 0) score = diff + n_empties; // player wins.

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
	return solve(search->board.player, search->n_empties);
}

/**
 * @brief Get the final score.
 *
 * Get the final score, when the board is full.
 *
 * @param board Board.
 * @return The final score, as a disc difference.
 */
static inline int solve_0(const uint64_t player)
{
	SEARCH_STATS(++statistics.n_solve_0);

	return 2 * bit_count(player) - SCORE_MAX;
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
	return solve_0(search->board.player);
}

/**
 * @brief Get the final score.
 *
 * Get the final score, when a single empty square remains (max stage).
 *
 * @param player player's discs.
 * @param alpha  alpha bound (beta = alpha + 1).
 * @param x      Last empty square to play.
 * @return       The final opponent score, as a disc difference.
 */
static inline int solve_1(const uint64_t player, const int alpha, const int x)
{
	int n_flips = count_last_flip(x, player);
	int score = 2 * bit_count(player) - SCORE_MAX + 2 + n_flips;

	SEARCH_STATS(++statistics.n_solve_1);

	if (n_flips == 0) {
		if (score <= 0) {
			score -= 2;
			if (score > alpha) { // lazy cut-off
				n_flips = count_last_flip(x, ~player);
				score -= n_flips;
			}
		} else {
			if (score > alpha) { // lazy cut-off
				if ((n_flips = count_last_flip(x, ~player)) != 0) {
					score -= n_flips + 2;
				}
			}
		}
	}

	return score;
}

/**
 * @brief Get the final score.
 *
 * Get the final score, when 2 empty squares remain (min stage).
 *
 * @param .
 * @param alpha Alpha bound.
 * @param x1 First empty square coordinate.
 * @param x2 Second empty square coordinate.
 * @param search Search.
 * @return The final score, as a disc difference.
 */
static inline int solve_2(const uint64_t player, const uint64_t opponent, const int alpha, const int x1, const int x2, int64_t *n_nodes)
{
	uint64_t flipped;
	int score, bestscore, nodes = 1;
	const int beta = alpha + 1;

	if ((NEIGHBOUR[x1] & opponent) && (flipped = flip(x1, player, opponent))) {
		nodes = 2;
		bestscore = solve_1(opponent ^ flipped, alpha, x2);
		if ((bestscore > alpha) && (NEIGHBOUR[x2] & opponent) && (flipped = flip(x2, player, opponent))) {
			nodes = 3;
			score = solve_1(opponent ^ flipped, alpha, x1);
			if (score < bestscore) bestscore = score;
		}
	} else if ((NEIGHBOUR[x2] & opponent) && (flipped = flip(x2, player, opponent))) {
			nodes = 2;
			bestscore = solve_1(opponent ^ flipped, alpha, x1);
	} else {
		if ((NEIGHBOUR[x1] & player) && (flipped = flip(x1, opponent, player))) {
			nodes = 2;
			bestscore = -solve_1(player ^ flipped, -beta, x2);
			if ((bestscore < beta) && (NEIGHBOUR[x2] & player) && (flipped = flip(x2, opponent, player))) {
				nodes = 3;
				score = -solve_1(player ^ flipped, -beta, x1);
				if (score > bestscore) bestscore = score;
			}
		} else if ((NEIGHBOUR[x2] & player) && (flipped = flip(x2, opponent, player))) {
			nodes = 2;
			bestscore = -solve_1(player ^ flipped, -beta, x1);
		} else bestscore = solve(opponent, 2);
	}

	#if (COUNT_NODES & 1)
		*n_nodes += nodes;
	#endif
 	assert(SCORE_MIN <= bestscore && bestscore <= SCORE_MAX);
 	assert((bestscore & 1) == 0);
	return bestscore;
}

/**
 * @brief Get the final score at 3 empties.
 *
 * Get the final score, when 3 empty squares remain  (max stage).
 *
 * @param search Search.
 * @param alpha Alpha bound.
 * @return The final score, as a disc difference.
 */
static int solve_3(const uint64_t player, const uint64_t opponent, const int alpha, int x1, int x2, int x3, const int parity, int64_t *n_nodes)
{
	uint64_t flipped, next_player, next_opponent;
	int score, bestscore, tmp;
	const int beta = alpha + 1;


	SEARCH_STATS(++statistics.n_solve_3);
	SEARCH_UPDATE_INTERNAL_NODES(*n_nodes);

	// parity based move sorting
	if (!(parity & QUADRANT_ID[x1])) {
		if (parity & QUADRANT_ID[x2]) { // case 1(x2) 2(x1 x3)
				tmp = x1; x1 = x2;
			if (SQUARE_VALUE[x3] > SQUARE_VALUE[tmp]) {
				x2 = x3; x3 = tmp;
			} else {
				x2 = tmp;
			}
		} else { // case 1(x3) 2(x1 x2)
			tmp = x1; x1 = x3;
			if (SQUARE_VALUE[x2] > SQUARE_VALUE[tmp]) {
				x3 = tmp;
			} else {
				x3 = x2; x2 = tmp;
			}
		}
	} else {
		if (SQUARE_VALUE[x3] > SQUARE_VALUE[x2]) {
			tmp = x2; x2 = x3; x3 = tmp;
		}
		if (SQUARE_VALUE[x2] > SQUARE_VALUE[x1]) {
			tmp = x2; x2 = x3; x3 = tmp;
		}
	}

	// best move alphabeta search
	if ((NEIGHBOUR[x1] & opponent) && (flipped = flip(x1, player, opponent))) {
		next_player = opponent ^ flipped; next_opponent = player ^ (flipped | x_to_bit(x1));
		bestscore = solve_2(next_player, next_opponent, alpha, x2, x3, n_nodes);
		if (bestscore >= beta) return bestscore;
	} else bestscore = -SCORE_INF;

	if ((NEIGHBOUR[x2] & opponent) && (flipped = flip(x2, player, opponent))) {
		next_player = opponent ^ flipped; next_opponent = player ^ (flipped | x_to_bit(x2));
		score = solve_2(next_player, next_opponent, alpha, x1, x3, n_nodes);
		if (score >= beta) return score;
		else if (score > bestscore) bestscore = score;
	}

	if ((NEIGHBOUR[x3] & opponent) && (flipped = flip(x3, player, opponent))) {
		next_player = opponent ^ flipped; next_opponent = player ^ (flipped | x_to_bit(x3));
		score = solve_2(next_player, next_opponent, alpha, x1, x2, n_nodes);
		if (score > bestscore) bestscore = score;
	}

	if (bestscore == -SCORE_INF) {
		// pass
		if ((NEIGHBOUR[x1] & player) && (flipped = flip(x1, opponent, player))) {
			next_player = player ^ flipped; next_opponent = opponent ^ (flipped | x_to_bit(x1));
			bestscore = -solve_2(next_player, next_opponent, -beta, x2, x3, n_nodes);
			if (bestscore <= alpha) return bestscore;
		} else bestscore = SCORE_INF;

		if ((NEIGHBOUR[x2] & player) && (flipped = flip(x2, opponent, player))) {
			next_player = player ^ flipped; next_opponent = opponent ^ (flipped | x_to_bit(x2));
			score = -solve_2(next_player, next_opponent, -beta, x1, x3, n_nodes);
			if (score <= alpha) return score;
			else if (score < bestscore) bestscore = score;
		}

		if ((NEIGHBOUR[x3] & player) && (flipped = flip(x3, opponent, player))) {
			next_player = player ^ flipped; next_opponent = opponent ^ (flipped | x_to_bit(x3));
			score = -solve_2(next_player, next_opponent, -beta, x1, x2, n_nodes);
			if (score < bestscore) bestscore = score;
		}

		// gameover
		if (bestscore == SCORE_INF) bestscore = solve(player, 3);
	}

 	assert(SCORE_MIN <= bestscore && bestscore <= SCORE_MAX);
	return bestscore;
}

/**
 * @brief Get the final score.
 *
 * Get the final score, when 4 empty squares remain (min stage).
 *
 * @param search Search position.
 * @param alpha Upper score value.
 * @return The final score, as a disc difference.
 */
static int search_solve_4(Search *search, const int alpha)
{
	const uint64_t player = search->board.player, opponent = search->board.opponent;
	uint64_t flipped, next_player, next_opponent;
	int64_t *n_nodes = &search->n_nodes;
	SquareList *empties = search->empties;
	int x1 = empties[NOMOVE].next;
	int x2 = empties[x1].next;
	int x3 = empties[x2].next;
	int x4 = empties[x3].next;
	int score, bestscore;
	const int beta = alpha + 1;

	SEARCH_STATS(++statistics.n_search_solve_4);
	SEARCH_UPDATE_INTERNAL_NODES(search->n_nodes);

	// stability cutoff
	if (search_SC_NWS_4(search, alpha, &score)) return score;

	// parity based move sorting.
	// The following hole sizes are possible:
	//    4 - 1 3 - 2 2 - 1 1 2 - 1 1 1 1
	// Only the 1 1 2 case needs move sorting.
	if (!(search->parity & QUADRANT_ID[x1])) {
		if (search->parity & QUADRANT_ID[x2]) {
			if (search->parity & QUADRANT_ID[x3]) { // case 1(x2) 1(x3) 2(x1 x4)
				int tmp = x1; x1 = x2; x2 = x3; x3 = tmp;
			} else { // case 1(x2) 1(x4) 2(x1 x3)
				int tmp = x1; x1 = x2; x2 = x4; x4 = x3; x3 = tmp;
			}
		} else if (search->parity & QUADRANT_ID[x3]) { // case 1(x3) 1(x4) 2(x1 x2)
			int tmp = x1; x1 = x3; x3 = tmp; tmp = x2; x2 = x4; x4 = tmp;
		}
	} else {
		if (!(search->parity & QUADRANT_ID[x2])) {
			if (search->parity & QUADRANT_ID[x3]) { // case 1(x1) 1(x3) 2(x2 x4)
				int tmp = x2; x2 = x3; x3 = tmp;
			} else { // case 1(x1) 1(x4) 2(x2 x3)
				int tmp = x2; x2 = x4; x4 = x3; x3 = tmp;
			}
		}
	}

	// best move alphabeta search
	if ((NEIGHBOUR[x1] & opponent) && (flipped = flip(x1, player, opponent))) {
			next_player = opponent ^ flipped; next_opponent = player ^ (flipped | x_to_bit(x1));
			bestscore = solve_3(next_player, next_opponent, alpha, x2, x3, x4, search->parity ^ QUADRANT_ID[x1], n_nodes);
		if (bestscore <= alpha) return bestscore;
	} else bestscore = SCORE_INF;

	if ((NEIGHBOUR[x2] & opponent) && (flipped = flip(x2, player, opponent))) {
		next_player = opponent ^ flipped; next_opponent = player ^ (flipped | x_to_bit(x2));
			score = solve_3(next_player, next_opponent, alpha, x1, x3, x4, search->parity ^ QUADRANT_ID[x2], n_nodes);
		if (score <= alpha) return score;
		else if (score < bestscore) bestscore = score;
	}

	if ((NEIGHBOUR[x3] & opponent) && (flipped = flip(x3, player, opponent))) {
		next_player = opponent ^ flipped; next_opponent = player ^ (flipped | x_to_bit(x3));
			score = solve_3(next_player, next_opponent, alpha, x1, x2, x4, search->parity ^ QUADRANT_ID[x3], n_nodes);
		if (score <= alpha) return score;
		else if (score < bestscore) bestscore = score;
	}

	if ((NEIGHBOUR[x4] & opponent) && (flipped = flip(x4, player, opponent))) {
		next_player = opponent ^ flipped; next_opponent = player ^ (flipped | x_to_bit(x4));
			score = solve_3(next_player, next_opponent, alpha, x1, x2, x3, search->parity ^ QUADRANT_ID[x4], n_nodes);
		if (score < bestscore) bestscore = score;
	}

	// pass
	if (bestscore == SCORE_INF) {
		if (can_move(opponent, player)) { // pass ?
			search_pass_endgame(search);
				bestscore = -search_solve_4(search, -beta);
			search_pass_endgame(search);
		} else { // gameover
			bestscore = solve(opponent, 4);
		}
	}

 	assert(SCORE_MIN <= bestscore && bestscore <= SCORE_MAX);
	return bestscore;
}

/**
 * @brief  Evaluate a position using a shallow NWS.
 *
 * This function is used when there are few empty squares on the board. Here,
 * optimizations are in favour of speed instead of efficiency.
 * Move ordering is constricted to the hole parity and the type of squares.
 * No hashtable are used and anticipated cut-off is limited to stability cut-off.
 *
 * @param search Search.
 * @param alpha Alpha bound.
 * @return The final score, as a disc difference.
 */
static int search_shallow(Search *search, const int alpha)
{
	Board *board = &search->board;
	int x;
	Move move;
	int score, bestscore = -SCORE_INF;
	const int beta = alpha + 1;

	assert(SCORE_MIN <= alpha && alpha <= SCORE_MAX);
	assert(0 <= search->n_empties && search->n_empties <= DEPTH_TO_SHALLOW_SEARCH);

	SEARCH_STATS(++statistics.n_NWS_shallow);
	SEARCH_UPDATE_INTERNAL_NODES(search->n_nodes);

	// stability cutoff
	if (search_SC_NWS(search, alpha, &score)) return score;

	if (search->parity > 0 && search->parity < 15) {

		foreach_odd_empty (x, search->empties, search->parity) {
			if ((NEIGHBOUR[x] & board->opponent) && board_get_move(board, x, &move)) {
				search_update_endgame(search, &move);
					if (search->n_empties == 4) score = search_solve_4(search, alpha);
					else score = -search_shallow(search, -beta);
				search_restore_endgame(search, &move);
				if (score >= beta) return score;
				else if (score > bestscore) bestscore = score;
			}
		}

		foreach_even_empty (x, search->empties, search->parity) {
			if ((NEIGHBOUR[x] & board->opponent) && board_get_move(board, x, &move)) {
				search_update_endgame(search, &move);
					if (search->n_empties == 4) score = search_solve_4(search, alpha);
					else score = -search_shallow(search, -beta);
				search_restore_endgame(search, &move);
				if (score >= beta) return score;
				else if (score > bestscore) bestscore = score;
			}
		}
	} else 	{
		foreach_empty (x, search->empties) {
			if ((NEIGHBOUR[x] & board->opponent) && board_get_move(board, x, &move)) {
				search_update_endgame(search, &move);
					if (search->n_empties == 4) score = search_solve_4(search, alpha);
					else score = -search_shallow(search, -beta);
				search_restore_endgame(search, &move);
				if (score >= beta) return score;
				else if (score > bestscore) bestscore = score;
			}
		}
	}

	// no move
	if (bestscore == -SCORE_INF) {
		if (can_move(board->opponent, board->player)) { // pass
			search_pass_endgame(search);
				bestscore = -search_shallow(search, -beta);
			search_pass_endgame(search);
		} else { // gameover
			bestscore = search_solve(search);
		}
	}

 	assert(SCORE_MIN <= bestscore && bestscore <= SCORE_MAX);
	return bestscore;
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
	HashTable *hash_table = &search->hash_table;
	HashData hash_data;
	HashStore store;
	uint64_t hash_code;
	const int beta = alpha + 1;
	Board *board = &search->board;
	MoveList movelist;
	Move *move;
	int score, bestscore, bestmove;
	uint64_t cost;

	if (search->stop) return alpha;

	assert(search->n_empties == bit_count(~(search->board.player | search->board.opponent)));
	assert(SCORE_MIN <= alpha && alpha <= SCORE_MAX);

	SEARCH_STATS(++statistics.n_NWS_endgame);

	SEARCH_UPDATE_INTERNAL_NODES(search->n_nodes);

#if USE_SOLID
	// discs on all lines full do not participate to the game anymore,
	// so we transfer them to the player side.
	Board solid;
	int solid_delta;
	uint64_t full = 0;

	// stability cutoff
	if (search_SC_NWS_full(search, alpha, &score, &full)) return score;
	solid = *board;
	solid_delta = 0;
	if (search->n_empties <= SOLID_DEPTH) {
		full &= solid.opponent;
		solid.opponent ^= full;
		solid.player ^=full;
		solid_delta = 2 * bit_count(full);
	}
	// transposition prefetch
	hash_code = board_get_hash_code(&solid);
	hash_prefetch(hash_table, hash_code);
#else
	hash_code = board_get_hash_code(board);
	hash_prefetch(hash_table, hash_code);

	// stability cutoff
	if (search_SC_NWS(search, alpha, &score)) return score;
#endif

	search_get_movelist(search, &movelist);

	if (movelist.n_moves > 1) {
		// transposition cutoff
#if USE_SOLID
		if (hash_get(hash_table, &solid, hash_code, &hash_data)) {
			hash_data.lower -= solid_delta;
			hash_data.upper -= solid_delta;
			if (search_TC_NWS(&hash_data, search->n_empties, NO_SELECTIVITY, alpha, &score)) return score;
		}
#else
		if (hash_get(hash_table, board, hash_code, &hash_data) && search_TC_NWS(&hash_data, search->n_empties, NO_SELECTIVITY, alpha, &score)) return score;
#endif
		cost = -search->n_nodes;
		movelist_evaluate_fast(&movelist, search, &hash_data);
		bestscore = -SCORE_INF;

		// loop over all moves
		foreach_best_move(move, &movelist) {
			search_update_endgame(search, move);
				if (search->n_empties <= DEPTH_TO_SHALLOW_SEARCH) score = -search_shallow(search, -beta);
				else score = -NWS_endgame(search, -beta);
			search_restore_endgame(search, move);
			if (score > bestscore) {
				bestmove = move->x;
				bestscore = score;
				if (bestscore >= beta) break;
			}
		}
		if (!search->stop) {
			cost += search->n_nodes;
			draft_set(&store.draft, search->n_empties, NO_SELECTIVITY, last_bit(cost), search->hash_table.date);
#if USE_SOLID
			store_set(&store, alpha + solid_delta, beta + solid_delta, bestscore + solid_delta, bestmove);
			hash_store(hash_table, &solid, hash_code, &store);
#else
			store_set(&store, alpha, beta, bestscore, bestmove);
			hash_store(hash_table, board, hash_code, &store);
#endif
		}
	} else if (movelist.n_moves == 1) {
		move = movelist_first(&movelist);
		search_update_endgame(search, move);
			if (search->n_empties <= DEPTH_TO_SHALLOW_SEARCH) bestscore = -search_shallow(search, -beta);
			else bestscore = -NWS_endgame(search, -beta);
		search_restore_endgame(search, move);
	} else {
		if (can_move(board->opponent, board->player)) { // pass
			search_pass_endgame(search);
				bestscore = -NWS_endgame(search, -beta);
			search_pass_endgame(search);
		} else  { // game over
			bestscore = search_solve(search);
		}
	}

	if (SQUARE_STATS(1) + 0) {
		foreach_move(move, &movelist)
			++statistics.n_played_square[search->n_empties][SQUARE_TYPE[move->x]];
		if (bestscore > alpha) ++statistics.n_good_square[search->n_empties][SQUARE_TYPE[bestscore]];
	}
 	assert(SCORE_MIN <= bestscore && bestscore <= SCORE_MAX);
	return bestscore;
}

