/**
 * @file midgame.c
 *
 * Search near the end of the game.
 *
 * @date 1998 - 2024
 * @author Richard Delorme
 * @version 4.6
 */

#include "search.h"

#include "bit.h"
#include "hash.h"
#include "options.h"
#include "stats.h"
#include "ybwc.h"
#include "settings.h"

#include <assert.h>
#include <math.h>


/**
 * @brief evaluate a midgame position with the evaluation function.
 *
 * @param search Position to evaluate.
 */
int search_eval_0(Search *search)
{
	int score;

	SEARCH_STATS(++statistics.n_search_eval_0);
	SEARCH_UPDATE_EVAL_NODES(search->n_nodes);

	score = eval_accumulate(&search->eval);

	if (score > 0) score += 64; else score -= 64;
	score /= 128;

	if (score <= SCORE_MIN) score = SCORE_MIN + 1;
	else if (score >= SCORE_MAX) score = SCORE_MAX - 1;

	return score;
}

/**
 * @brief Evaluate a position at depth 1.
 *
 * As an optimization, the last move is used to only updates the evaluation
 * features.
 *
 * @param search Position to evaluate.
 * @param alpha Alpha bound.
 * @param beta Beta bound.
 */
int search_eval_1(Search *search, const int alpha, int beta)
{
	const Board *board = &search->board;
	const uint64_t moves = board_get_moves(board);
	Eval *eval = &search->eval;
	Move move;
	int x, score, bestscore;


	SEARCH_STATS(++statistics.n_search_eval_1);
	SEARCH_UPDATE_INTERNAL_NODES(search->n_nodes);

	if (moves) {
		bestscore = -SCORE_INF;
		if (beta >= SCORE_MAX) beta = SCORE_MAX - 1;
		foreach_empty (x, search->empties) {
			if (moves & x_to_bit(x)) {
				board_get_move(board, x, &move);
				if (move_wipeout(&move, board)) return SCORE_MAX;
				eval_update(eval, &move);
				SEARCH_UPDATE_EVAL_NODES(search->n_nodes);
					score = -eval_accumulate(eval);
				eval_restore(eval);

				if (score > 0) score += 64; else score -= 64;
				score /= 128;

				if (score > bestscore) {
					bestscore = score;
					if (bestscore >= beta) break;
				}
			}
		}
		if (bestscore <= SCORE_MIN) bestscore = SCORE_MIN + 1;
		else if (bestscore >= SCORE_MAX) bestscore = SCORE_MAX - 1;
	} else {
		if (can_move(board->opponent, board->player)) {
			search_update_pass_midgame(search);
				bestscore = -search_eval_1(search, -beta, -alpha);
			search_restore_pass_midgame(search);
		} else { // game over
			bestscore = search_solve(search);
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
 */
int search_eval_2(Search *search, int alpha, const int beta)
{
	const Board *board = &search->board;
	const uint64_t moves = board_get_moves(board);
	Move move;
	int x, bestscore, score;

	SEARCH_STATS(++statistics.n_search_eval_2);
	SEARCH_UPDATE_INTERNAL_NODES(search->n_nodes);

	assert(-SCORE_MAX <= alpha && alpha <= SCORE_MAX);
	assert(-SCORE_MAX <= beta && beta <= SCORE_MAX);
	assert(alpha <= beta);

	if (moves) {
		bestscore = -SCORE_INF;
		foreach_empty(x, search->empties) {
			if (moves & x_to_bit(x)) {
				board_get_move(board, x, &move);
				search_update_midgame(search, &move);
					score = -search_eval_1(search, -beta, -alpha);
				search_restore_midgame(search, &move);
				if (score > bestscore) {
					bestscore = score;
					if (bestscore >= beta) break;
					else if (bestscore > alpha) alpha = bestscore;
				}
			}
		}
	} else {
		if (can_move(board->opponent, board->player)) {
			search_update_pass_midgame(search);
				bestscore = -search_eval_2(search, -beta, -alpha);
			search_restore_pass_midgame(search);
		} else {
			bestscore = search_solve(search);
		}
	}

	assert(SCORE_MIN <= bestscore && bestscore <= SCORE_MAX);

	return bestscore;
}

static inline void search_update_probcut(Search *search, const NodeType node_type)
{
	search->node_type[search->height] = node_type;
	if (!USE_RECURSIVE_PROBCUT) search->selectivity = NO_SELECTIVITY;
	LIMIT_RECURSIVE_PROBCUT(++search->probcut_level;)
}


static inline void search_restore_probcut(Search *search, const  NodeType node_type, const int selectivity)
{
	search->node_type[search->height] = node_type;
	if (!USE_RECURSIVE_PROBCUT) search->selectivity = selectivity;
	LIMIT_RECURSIVE_PROBCUT(--search->probcut_level;)
}

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
 *
 * @return true if probable cutoff has been found, false otherwise.
 */
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
		probcut_error = t * eval_sigma(search->n_empties, depth, probcut_depth) + 0.5;

		// compute evaluation error (i.e. error at depth 0) averaged for both depths
		eval_score = search_eval_0(search);
		eval_error = t * 0.5 * (eval_sigma(search->n_empties, depth, 0) + eval_sigma(search->n_empties, depth, probcut_depth)) + 0.5;

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
 * @brief Evaluate a midgame position with a Null Window Search algorithm.
 *
 * This function is used when there are still many empty squares on the board. Move
 * ordering, hash table cutoff, enhanced transposition cutoff, etc. are used in
 * order to diminish the size of the tree to analyse, but at the expense of a
 * slower speed.
 *
 * @param search     search.
 * @param alpha      lower bound.
 * @param depth Search remaining depth.
 * @param hash_table Hash Table to use.
 * @return An evaluated score, as a disc difference.
 */
int NWS_shallow(Search *search, const int alpha, int depth, HashTable *hash_table)
{
	HashData hash_data;
	HashStore store;
	Board *board = &search->board;
	MoveList movelist;
	Move *move;
	const int beta = alpha + 1;
	uint64_t hash_code;
	int score, bestscore, bestmove;
	int64_t cost = -search->n_nodes;

	if (depth == 2) return search_eval_2(search, alpha, beta);

	SEARCH_STATS(++statistics.n_NWS_midgame);
	SEARCH_UPDATE_INTERNAL_NODES(search->n_nodes);

	assert(search->n_empties == bit_count(~(board->player | board->opponent)));
	assert(SCORE_MIN <= alpha && alpha <= SCORE_MAX);
	assert(depth > 2);
	assert(hash_table != NULL);

	// transposition prefetch
	hash_code = board_get_hash_code(board);
	hash_prefetch(hash_table, hash_code);

	// stability cutoff
	if (search_SC_NWS(search, alpha, &score)) return score;

	// list of moves
	search_get_movelist(search, &movelist);

	if (movelist_is_empty(&movelist)) { // no moves ?
		if (can_move(board->opponent, board->player)) { // pass ?
			search_update_pass_midgame(search);
				bestscore = -NWS_shallow(search, -beta, depth, hash_table);
				bestmove = PASS;
			search_restore_pass_midgame(search);
		} else { // game-over !
			bestscore = search_solve(search);
			bestmove = NOMOVE;
		}
	} else {
		// transposition cutoff
		if (hash_get(hash_table, board, hash_code, &hash_data) && search_TC_NWS(&hash_data, depth, NO_SELECTIVITY, alpha, &score)) return score;

		// sort the list of moves
		movelist_evaluate(&movelist, search, &hash_data, alpha, depth);
		movelist_sort(&movelist) ;

		// loop over all moves
		bestscore = -SCORE_INF; bestmove = NOMOVE;
		foreach_move(move, &movelist) {
			search_update_midgame(search, move);
				score = -NWS_shallow(search, -beta, depth - 1, hash_table);
			search_restore_midgame(search, move);
			if (score > bestscore) {
				bestscore = score;
				bestmove = move->x;
				if (bestscore >= beta) break;
			}
		}
	}

	// save the best result in hash tables
	cost += search->n_nodes;
	draft_set(&store.draft, depth, NO_SELECTIVITY, last_bit(cost), hash_table->date);
	store_set(&store, alpha, beta, bestscore, bestmove);
	hash_store(hash_table, board, hash_code, &store);
 	assert(SCORE_MIN <= bestscore && bestscore <= SCORE_MAX);

	return bestscore;
}

/**
 * @brief Evaluate a midgame position at shallow depth.
 *
 * This function is used when there are still many empty squares on the board. Move
 * ordering, hash table cutoff, enhanced transposition cutoff, etc. are used in
 * order to diminish the size of the tree to analyse, but at the expense of a
 * slower speed.
 *
 * @param search Search.
 * @param alpha Alpha bound.
 * @param beta Beta bound.
 * @param depth Search depth.
 * @return An evaluated score, as a disc difference.
 */
int PVS_shallow(Search *search, int alpha, int beta, int depth)
{
	HashData hash_data;
	HashStore store;
	HashTable *shallow_table = &search->shallow_table;
	MoveList movelist;
	Board *board = &search->board;
	Move *move;
	uint64_t hash_code;
	int64_t cost = -search->n_nodes;
	int score, bestscore, bestmove;
	int lower;

	if (depth == 2) return search_eval_2(search, alpha, beta);

	SEARCH_STATS(++statistics.n_PVS_shallow);
	SEARCH_UPDATE_INTERNAL_NODES(search->n_nodes);

	assert(search->n_empties == bit_count(~(board->player | board->opponent)));
	assert(SCORE_MIN <= alpha && alpha <= SCORE_MAX);

	// transposition prefetch
	hash_code = board_get_hash_code(board);
	hash_prefetch(shallow_table, hash_code);

	search_get_movelist(search, &movelist);

	if (movelist_is_empty(&movelist)) { // no moves ?
		if (can_move(board->opponent, board->player)) { // pass ?
			search_update_pass_midgame(search);
				bestscore = -PVS_shallow(search, -beta, -alpha, depth);
				bestmove = PASS;
			search_restore_pass_midgame(search);
		} else { // game-over !
			bestscore = search_solve(search);
			bestmove = NOMOVE;
		}
	} else {
		// transposition
		hash_get(shallow_table, board, hash_code, &hash_data);

		// sort the list of moves
		movelist_evaluate(&movelist, search, &hash_data, alpha, depth);
		movelist_sort(&movelist) ;

		// loop over all moves
		bestscore = -SCORE_INF; bestmove = NOMOVE;
		lower = alpha;
		foreach_move(move, &movelist) {
			search_update_midgame(search, move);
				if (bestscore == -SCORE_INF) {
					score = -PVS_shallow(search, -beta, -lower, depth - 1);
				} else {
					score = -NWS_shallow(search, -lower - 1, depth - 1, shallow_table);
					if (alpha < score && score < beta) {
						score = -PVS_shallow(search, -beta, -lower, depth - 1);
					}
				}
			search_restore_midgame(search, move);
			if (score > bestscore) {
				bestscore = score;
				bestmove = move->x;
				if (bestscore >= beta) break;
				else if (bestscore > lower) lower = bestscore;
			}
		}
	}

	// save the best result in hash tables
	cost += search->n_nodes;
	draft_set(&store.draft, depth, NO_SELECTIVITY, last_bit(cost), shallow_table->date);
	store_set(&store, alpha, beta, bestscore, bestmove);
	hash_store(shallow_table, board, hash_code, &store);
 	assert(SCORE_MIN <= bestscore && bestscore <= SCORE_MAX);

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
	HashTable *hash_table = &search->hash_table;
	HashTable *pv_table = &search->pv_table;
	uint64_t hash_code;
	const int beta = alpha + 1;
	HashData hash_data;
	HashStore store;
	Board *board = &search->board;
	MoveList movelist;
	Move *move;
	Node node;
	int64_t cost = -search->n_nodes - search->child_nodes;
	int hash_selectivity;

	assert(search->n_empties == bit_count(~(board->player | board->opponent)));
	assert((2 <= depth && depth < search->n_empties) || depth == search->n_empties);
	assert(SCORE_MIN <= alpha && alpha <= SCORE_MAX);
	assert(parent != NULL);

	search_check_timeout(search);
	if (search->stop) return alpha;
	else if (search->n_empties == 0) return search_solve_0(search);
	else if (depth <= 3 && depth < search->n_empties) return NWS_shallow(search, alpha, depth, hash_table);
	else if (search->n_empties <= depth && depth < DEPTH_MIDGAME_TO_ENDGAME) return NWS_endgame(search, alpha);

	SEARCH_STATS(++statistics.n_NWS_midgame);
	SEARCH_UPDATE_INTERNAL_NODES(search->n_nodes);

	// transposition prefetch
	hash_code = board_get_hash_code(board);
	hash_prefetch(hash_table, hash_code);
	hash_prefetch(pv_table, hash_code);

	// stability cutoff
	if (search_SC_NWS(search, alpha, &score)) return score;

	search_get_movelist(search, &movelist);

	if (movelist_is_empty(&movelist)) { // no moves ?
		node_init(&node, search, alpha, beta, depth, movelist.n_moves, parent);
		if (can_move(board->opponent, board->player)) { // pass ?
			search_update_pass_midgame(search);
				node.bestscore = -NWS_midgame(search, -node.beta, depth, &node);
			search_restore_pass_midgame(search);
		} else { // game-over !
			node.bestscore = search_solve(search);
		}
	} else {
		// transposition cutoff
		hash_code = board_get_hash_code(board);
		if ((hash_get(hash_table, board, hash_code, &hash_data) || hash_get(pv_table, board, hash_code, &hash_data))
		 && search_TC_NWS(&hash_data, depth, search->selectivity, alpha, &score)) return score;

		// probcut
		if (search_probcut(search, alpha, depth, parent, &score)) return score;

		// sort the list of moves
		if (movelist.n_moves > 1) {
			if (hash_data.move[0] == NOMOVE) hash_get(hash_table, board, hash_code, &hash_data);
			movelist_evaluate(&movelist, search, &hash_data, alpha, depth + options.inc_sort_depth[search->node_type[search->height]]);
			movelist_sort(&movelist) ;
		}

		// ETC
		if (search_ETC_NWS(search, &movelist, hash_code, depth, search->selectivity, alpha, &score)) return score;

		node_init(&node, search, alpha, beta, depth, movelist.n_moves, parent);

		// loop over all moves
		for (move = node_first_move(&node, &movelist); move; move = node_next_move(&node)) {
			if (!node_split(&node, move)) {
				search_update_midgame(search, move);
					move->score = -NWS_midgame(search, -beta, depth - 1, &node);
				search_restore_midgame(search, move);
				node_update(&node, move);
			}
		}
		node_wait_slaves(&node);
	}

	// save the best result in hash tables
	if (!search->stop) {
		cost += search->n_nodes + search->child_nodes;
		if (search->n_empties < depth && depth <= DEPTH_MIDGAME_TO_ENDGAME) hash_selectivity = NO_SELECTIVITY; // hack
		else hash_selectivity = search->selectivity;
		draft_set(&store.draft,depth, hash_selectivity, last_bit(cost), hash_table->date);
		store_set(&store, alpha, beta, node.bestscore, node.bestmove);
		hash_store(hash_table, board, hash_code, &store);
		if (search->height <= PV_HASH_HEIGHT) hash_store(pv_table, board, hash_code, &store);
		SQUARE_STATS(foreach_move(move, &movelist))
		SQUARE_STATS(++statistics.n_played_square[search->n_empties][SQUARE_TYPE[move->x]];)
		SQUARE_STATS(if (node.bestscore > alpha) ++statistics.n_good_square[search->n_empties][SQUARE_TYPE[node.bestmove]];)

	 	assert(SCORE_MIN <= node.bestscore && node.bestscore <= SCORE_MAX);
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
	HashTable *hash_table = &search->hash_table;
	HashTable *pv_table = &search->pv_table;
	uint64_t hash_code;
	HashData hash_data;
	HashStore store;
	Board *board = &search->board;
	MoveList movelist;
	Move *move;
	Node node;
	int64_t cost;
	int reduced_depth, depth_pv_extension, saved_selectivity;
	int hash_selectivity;

	SEARCH_STATS(++statistics.n_PVS_midgame);

	assert(search->n_empties == bit_count(~(board->player | board->opponent)));
	assert(depth <= search->n_empties);
	assert((-SCORE_MAX <= alpha && alpha <= SCORE_MAX) || printf("alpha = %d\n", alpha));
	assert((-SCORE_MAX <= beta && beta <= SCORE_MAX) || printf("beta = %d\n", beta));
	assert(alpha <= beta);

	// end of search ?
	search_check_timeout(search);
	if (search->stop) return alpha;
	else if (search->n_empties == 0) return search_solve_0(search);
	else if (USE_PV_EXTENSION && depth < search->n_empties && search->n_empties <= search->depth_pv_extension) return PVS_midgame(search, alpha, beta, search->n_empties, parent);
	else if (depth == 2 && search->n_empties > 2) return search_eval_2(search, alpha, beta);

	cost = -search_count_nodes(search);
	SEARCH_UPDATE_INTERNAL_NODES(search->n_nodes);

	// transposition prefetch
	hash_code = board_get_hash_code(board);
	hash_prefetch(pv_table, hash_code);
	hash_prefetch(hash_table, hash_code);

	// move list & node initialization
	search_get_movelist(search, &movelist);
	node_init(&node, search, alpha, beta, depth, movelist.n_moves, parent);
	node.pv_node = true;

	// special cases
	if (movelist_is_empty(&movelist)) {
		if (can_move(board->opponent, board->player)) {
			search_update_pass_midgame(search); search->node_type[search->height] = PV_NODE;
				node.bestscore = -PVS_midgame(search, -beta, -alpha, depth, &node);
			search_restore_pass_midgame(search);
			node.bestmove = PASS;
		} else {
			node.alpha = -(node.beta = +SCORE_INF);
			node.bestscore = search_solve(search);
			node.bestmove = NOMOVE;
		}
	} else { // normal PVS
		if (movelist.n_moves > 1) {
			//IID
			const bool found = hash_get(pv_table, board, hash_code, &hash_data) || hash_get(hash_table, board, hash_code, &hash_data);

			if (USE_IID && !found) {
				if (depth == search->n_empties) reduced_depth = depth - ITERATIVE_MIN_EMPTIES;
				else reduced_depth = depth - 2;
				if (reduced_depth >= 3) {
					saved_selectivity = search->selectivity; search->selectivity = 0;
					depth_pv_extension = search->depth_pv_extension;
					search->depth_pv_extension = 0;
					PVS_midgame(search, SCORE_MIN, SCORE_MAX, reduced_depth, parent);
					hash_get(pv_table, board, hash_code, &hash_data);
					search->depth_pv_extension = depth_pv_extension;
					search->selectivity = saved_selectivity;
				}
			}

			// Evaluate moves for sorting. For a better ordering, the depth is artificially increased
			movelist_evaluate(&movelist, search, &hash_data, node.alpha, depth + options.inc_sort_depth[PV_NODE]);
			movelist_sort(&movelist) ;
		}

		// first move
		if ((move = node_first_move(&node, &movelist))) {
			search_update_midgame(search, move);
				search->node_type[search->height] = PV_NODE;
				move->score = -PVS_midgame(search, -beta, -alpha, depth - 1, &node);
			search_restore_midgame(search, move);
			node_update(&node, move);

			// other moves : try to refute the first/best one
			while ((move = node_next_move(&node))) {
				if (!node_split(&node, move)) {
					const int alpha = node.alpha;
					search_update_midgame(search, move);
						move->score = -NWS_midgame(search, -alpha - 1, depth - 1, &node);
						if (!search->stop && alpha < move->score && move->score < beta) {
							search->node_type[search->height] = PV_NODE;
							move->score = -PVS_midgame(search, -beta, -alpha, depth - 1, &node);
						}
					search_restore_midgame(search, move);
					node_update(&node, move);
				}
			}
			node_wait_slaves(&node);
		}
	}

	// save the best result in hash tables
	if (!search->stop) {
		cost += search_count_nodes(search);
		if (search->n_empties < depth && depth <= DEPTH_MIDGAME_TO_ENDGAME) hash_selectivity = NO_SELECTIVITY;
		else hash_selectivity = search->selectivity;
		draft_set(&store.draft,depth, hash_selectivity, last_bit(cost), hash_table->date);
		store_set(&store, alpha, beta, node.bestscore, node.bestmove);
		hash_store(hash_table, board, hash_code, &store);
		hash_store(pv_table, board, hash_code, &store);

		SQUARE_STATS(foreach_move(move, &movelist))
			SQUARE_STATS(++statistics.n_played_square[search->n_empties][SQUARE_TYPE[move->x]];)
		SQUARE_STATS(if (node.bestscore > alpha) ++statistics.n_good_square[search->n_empties][SQUARE_TYPE[node.bestmove]];)

	 	assert(SCORE_MIN <= node.bestscore && node.bestscore <= SCORE_MAX);

	} else {
		node.bestscore = alpha;
	}

	node_free(&node);

	return node.bestscore;
}

