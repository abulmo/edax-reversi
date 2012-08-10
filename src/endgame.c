/**
 * @file endgame.c
 *
 * Search near the end of the game.
 *
 * @date 1998 - 2012
 * @author Richard Delorme
 * @version 4.3
 */


#include "search.h"

#include "bit.h"
#include "settings.h"
#include "stats.h"
#include "ybwc.h"

#include <assert.h>

/**
 * @brief Get the final score.
 *
 * Get the final score, when no move can be made.
 *
 * @param board Board.
 * @param n_empties Number of empty squares remaining on the board.
 * @return The final score, as a disc difference.
 */
static inline int board_solve(const Board *board, const int n_empties)
{
	const int n_discs_p = bit_count(board->player);
	const int n_discs_o = 64 - n_empties - n_discs_p;
	register int score = n_discs_p - n_discs_o;

	SEARCH_STATS(++statistics.n_search_solve);

	if (score < 0) score -= n_empties;
	else if (score > 0) score += n_empties;

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
	return board_solve(search->board, search->n_empties);
}

/**
 * @brief Get the final score.
 *
 * Get the final score, when the board is full.
 *
 * @param board Board.
 * @return The final score, as a disc difference.
 */
static inline int board_solve_0(const Board *board)
{
	SEARCH_STATS(++statistics.n_search_solve_0);

	return 2 * bit_count(board->player) - SCORE_MAX;
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
	return board_solve_0(search->board);
}

/**
 * @brief Get the final score.
 *
 * Get the final score, when 1 empty squares remain.
 * The following code has been adapted from Zebra by Gunnar Anderson.
 *
 * @param board  Board to evaluate.
 * @param beta   Beta bound.
 * @param x      Last empty square to play.
 * @return       The final opponent score, as a disc difference.
 */
inline int board_score_1(const Board *board, const int beta, const int x)
{
	register int score, n_flips;

	score = 2 * bit_count(board->opponent) - SCORE_MAX;

	if ((n_flips = count_last_flip[x](board->player)) != 0) {
		score -= n_flips;
	} else {
		if (score >= 0) {
			score += 2;
			if (score < beta) { // lazy cut-off
				if ((n_flips = count_last_flip[x](board->opponent)) != 0) {
					score += n_flips;
				}
			}
		} else {
			if (score < beta) { // lazy cut-off
				if ((n_flips = count_last_flip[x](board->opponent)) != 0) {
					score += n_flips + 2;
				}
			}
		}
	}

	return score;
}

/**
 * @brief Get the final score.
 *
 * Get the final score, when 2 empty squares remain.
 *
 * @param board The board to evaluate.
 * @param alpha Alpha bound.
 * @param x1 First empty square coordinate.
 * @param x2 Second empty square coordinate.
 * @param search Search.
 * @return The final score, as a disc difference.
 */
static int board_solve_2(Board *board, int alpha, const int x1, const int x2, Search *search)
{
	Board next[1];
	register int score, bestscore;
	const int beta = alpha + 1;

	SEARCH_STATS(++statistics.n_board_solve_2);
	SEARCH_UPDATE_INTERNAL_NODES();

	if ((NEIGHBOUR[x1] & board->opponent) && board_next(board, x1, next)) {
		SEARCH_UPDATE_INTERNAL_NODES();
		bestscore = board_score_1(next, beta, x2);
	} else bestscore = -SCORE_INF;

	if (bestscore < beta) {
		if ((NEIGHBOUR[x2] & board->opponent) && board_next(board, x2, next)) {
			SEARCH_UPDATE_INTERNAL_NODES();
			score = board_score_1(next, beta, x1);
			if (score > bestscore) bestscore = score;
		}

		// pass
		if (bestscore == -SCORE_INF) {

			if ((NEIGHBOUR[x1] & board->player) && board_pass_next(board, x1, next)) {
				SEARCH_UPDATE_INTERNAL_NODES();
				bestscore = -board_score_1(next, -alpha, x2);
			} else bestscore = SCORE_INF;

			if (bestscore > alpha) {
				if ((NEIGHBOUR[x2] & board->player) && board_pass_next(board, x2, next)) {
					SEARCH_UPDATE_INTERNAL_NODES();
					score = -board_score_1(next, -alpha, x1);
					if (score < bestscore) bestscore = score;
				}
				// gameover
				if (bestscore == SCORE_INF) bestscore = board_solve(board, 2);
			}
		}
	}

 	assert(SCORE_MIN <= bestscore && bestscore <= SCORE_MAX);
 	assert((bestscore & 1) == 0);
	return bestscore;
}

/**
 * @brief Get the final score.
 *
 * Get the final score, when 3 empty squares remain.
 *
 * @param search Search.
 * @param alpha Alpha bound.
 * @return The final score, as a disc difference.
 */
static int search_solve_3(Search *search, const int alpha)
{
	Board *board = search->board;
	Board next[1];
	SquareList *empty = search->empties->next;
	int x1 = empty->x;
	int x2 = (empty = empty->next)->x;
	int x3 = empty->next->x;
	register int score, bestscore;
	const int beta = alpha + 1;

	SEARCH_STATS(++statistics.n_search_solve_3);
	SEARCH_UPDATE_INTERNAL_NODES();

	// parity based move sorting
	if (!(search->parity & QUADRANT_ID[x1])) {
		if (search->parity & QUADRANT_ID[x2]) { // case 1(x2) 2(x1 x3)
			int tmp = x1; x1 = x2; x2 = tmp;
		} else { // case 1(x3) 2(x1 x2)
			int tmp = x1; x1 = x3; x3 = x2; x2 = tmp;
		}
	}

	// best move alphabeta search
	if ((NEIGHBOUR[x1] & board->opponent) && board_next(board, x1, next)) {
		bestscore = -board_solve_2(next, -beta, x2, x3, search);
		if (bestscore >= beta) return bestscore;
	} else bestscore = -SCORE_INF;

	if ((NEIGHBOUR[x2] & board->opponent) && board_next(board, x2, next)) {
		score = -board_solve_2(next, -beta, x1, x3, search);
		if (score >= beta) return score;
		else if (score > bestscore) bestscore = score;
	}

	if ((NEIGHBOUR[x3] & board->opponent) && board_next(board, x3, next)) {
		score = -board_solve_2(next, -beta, x1, x2, search);
		if (score > bestscore) bestscore = score;
	}

	// pass ?
	if (bestscore == -SCORE_INF) {
		// best move alphabeta search
		if ((NEIGHBOUR[x1] & board->player) && board_pass_next(board, x1, next)) {
			bestscore = board_solve_2(next, alpha, x2, x3, search);
			if (bestscore <= alpha) return bestscore;
		} else bestscore = SCORE_INF;

		if ((NEIGHBOUR[x2] & board->player) && board_pass_next(board, x2, next)) {
			score = board_solve_2(next, alpha, x1, x3, search);
			if (score <= alpha) return score;
			else if (score < bestscore) bestscore = score;
		}

		if ((NEIGHBOUR[x3] & board->player) && board_pass_next(board, x3, next)) {
			score = board_solve_2(next, alpha, x1, x2, search);
			if (score < bestscore) bestscore = score;
		}

		// gameover
		if (bestscore == SCORE_INF) bestscore = board_solve(board, 3);
	}

 	assert(SCORE_MIN <= bestscore && bestscore <= SCORE_MAX);
	return bestscore;
}

/**
 * @brief Get the final score.
 *
 * Get the final score, when 4 empty squares remain.
 *
 * @param search Search position.
 * @param alpha Upper score value.
 * @return The final score, as a disc difference.
 */
static int search_solve_4(Search *search, const int alpha)
{
	Board *board = search->board;
	SquareList *empty = search->empties->next;
	int x1 = empty->x;
	int x2 = (empty = empty->next)->x;
	int x3 = (empty = empty->next)->x;
	int x4 = empty->next->x;
	Move move[1];
	int score, bestscore;
	const int beta = alpha + 1;

	SEARCH_STATS(++statistics.n_search_solve_4);
	SEARCH_UPDATE_INTERNAL_NODES();

	// stability cutoff
	if (search_SC_NWS(search, alpha, &score)) return score;

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
	if ((NEIGHBOUR[x1] & board->opponent) && board_get_move(board, x1, move)) {
		search_update_endgame(search, move);
			bestscore = -search_solve_3(search, -beta);
		search_restore_endgame(search, move);
		if (bestscore >= beta) return bestscore;
	} else bestscore = -SCORE_INF;

	if ((NEIGHBOUR[x2] & board->opponent) && board_get_move(board, x2, move)) {
		search_update_endgame(search, move);
			score = -search_solve_3(search, -beta);
		search_restore_endgame(search, move);
		if (score >= beta) return score;
		else if (score > bestscore) bestscore = score;
	}

	if ((NEIGHBOUR[x3] & board->opponent) && board_get_move(board, x3, move)) {
		search_update_endgame(search, move);
			score = -search_solve_3(search, -beta);
		search_restore_endgame(search, move);
		if (score >= beta) return score;
		else if (score > bestscore) bestscore = score;
	}

	if ((NEIGHBOUR[x4] & board->opponent) && board_get_move(board, x4, move)) {
		search_update_endgame(search, move);
			score = -search_solve_3(search, -beta);
		search_restore_endgame(search, move);
		if (score > bestscore) bestscore = score;
	}

	// no move
	if (bestscore == -SCORE_INF) {
		if (can_move(board->opponent, board->player)) { // pass
			search_pass_endgame(search);
				bestscore = -search_solve_4(search, -beta);
			search_pass_endgame(search);
		} else { // gameover
			bestscore = search_solve(search);
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
	Board *board = search->board;
	SquareList *empty;
	Move move;
	int score, bestscore = -SCORE_INF;
	const int beta = alpha + 1;

	assert(SCORE_MIN <= alpha && alpha <= SCORE_MAX);
	assert(0 <= search->n_empties && search->n_empties <= DEPTH_TO_SHALLOW_SEARCH);

	SEARCH_STATS(++statistics.n_NWS_shallow);
	SEARCH_UPDATE_INTERNAL_NODES();

	// stability cutoff
	if (search_SC_NWS(search, alpha, &score)) return score;

	if (search->parity > 0 && search->parity < 15) {

		foreach_odd_empty (empty, search->empties, search->parity) {
			if ((NEIGHBOUR[empty->x] & board->opponent)
			&& board_get_move(board, empty->x, &move)) {
				search_update_endgame(search, &move);
					if (search->n_empties == 4) score = -search_solve_4(search, -beta);
					else score = -search_shallow(search, -beta);
				search_restore_endgame(search, &move);
				if (score >= beta) return score;
				else if (score > bestscore) bestscore = score;
			}
		}

		foreach_even_empty (empty, search->empties, search->parity) {
			if ((NEIGHBOUR[empty->x] & board->opponent)
			&& board_get_move(board, empty->x, &move)) {
				search_update_endgame(search, &move);
					if (search->n_empties == 4) score = -search_solve_4(search, -beta);
					else score = -search_shallow(search, -beta);
				search_restore_endgame(search, &move);
				if (score >= beta) return score;
				else if (score > bestscore) bestscore = score;
			}
		}
	} else 	{
		foreach_empty (empty, search->empties) {
			if ((NEIGHBOUR[empty->x] & board->opponent)
			&& board_get_move(board, empty->x, &move)) {
				search_update_endgame(search, &move);
					if (search->n_empties == 4) score = -search_solve_4(search, -beta);
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
	int score;
	HashTable *hash_table = search->hash_table;
	unsigned long long hash_code;
	const int beta = alpha + 1;
	HashData hash_data[1];
	Board *board = search->board;
	MoveList movelist[1];
	Move *move, *bestmove;
	long long cost;

	if (search->stop) return alpha;

	assert(search->n_empties == bit_count(~(search->board->player|search->board->opponent)));
	assert(SCORE_MIN <= alpha && alpha <= SCORE_MAX);

	SEARCH_STATS(++statistics.n_NWS_endgame);

	if (search->n_empties <= DEPTH_TO_SHALLOW_SEARCH) return search_shallow(search, alpha);

	SEARCH_UPDATE_INTERNAL_NODES();

	// stability cutoff
	if (search_SC_NWS(search, alpha, &score)) return score;

	// transposition cutoff
	hash_code = board_get_hash_code(board);
	if (hash_get(hash_table, hash_code, hash_data) && search_TC_NWS(hash_data, search->n_empties, NO_SELECTIVITY, alpha, &score)) return score;

	search_get_movelist(search, movelist);

	cost = -search->n_nodes;

	// special cases
	if (movelist_is_empty(movelist)) {
		bestmove = movelist->move->next = movelist->move + 1;
		bestmove->next = 0;
		if (can_move(board->opponent, board->player)) { // pass
			search_pass_endgame(search);
				bestmove->score = -NWS_endgame(search, -beta);
			search_pass_endgame(search);
			bestmove->x = PASS;
		} else  { // game over
			bestmove->score = search_solve(search);
			bestmove->x = NOMOVE;
		}
	} else {
		movelist_evaluate(movelist, search, hash_data, alpha, 0);

		bestmove = movelist->move; bestmove->score = -SCORE_INF;
		// loop over all moves
		foreach_best_move(move, movelist) {
			search_update_endgame(search, move);
				move->score = -NWS_endgame(search, -beta);
			search_restore_endgame(search, move);
			if (move->score > bestmove->score) {
				bestmove = move;
				if (bestmove->score >= beta) break;
			}
		}
	}

	if (!search->stop) {
		cost += search->n_nodes;
		hash_store(hash_table, hash_code, search->n_empties, NO_SELECTIVITY, last_bit(cost), alpha, beta, bestmove->score, bestmove->x);
		if (SQUARE_STATS(1) + 0) {
			foreach_move(move, movelist)
				++statistics.n_played_square[search->n_empties][SQUARE_TYPE[move->x]];
			if (bestmove->score > alpha) ++statistics.n_good_square[search->n_empties][SQUARE_TYPE[bestmove->score]];
		}
	 	assert(SCORE_MIN <= bestmove->score && bestmove->score <= SCORE_MAX);
	 	assert((bestmove->score & 1) == 0);
		return bestmove->score;
	}

	return alpha;
}

