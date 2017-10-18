/**
 * @file root.c
 *
 * Search near the end of the game.
 *
 * @date 1998 - 2017
 * @author Richard Delorme
 * @version 4.4
 */

#include "search.h"

#include "bit.h"
#include "options.h"
#include "stats.h"
#include "util.h"
#include "ybwc.h"
#include "settings.h"

#include <stdlib.h>
#include <assert.h>

extern Log search_log[1];
extern Log engine_log[1];

/**
 * @brief Debug PV.
 *
 * @param search Search.
 * @param bestmove Best move.
 * @param f Output stream.
 */
void pv_debug(Search *search, const Move *bestmove, FILE *f)
{
	Board board[1];
	Move move[1];
	int x;
	unsigned long long hash_code;
	HashData hash_data[1];
	char s[4];
	int player = BLACK;

	spin_lock(search->result);

	*board = *search->board;
	if (search->height == 1) { // hack to call this function from height 1 or 0
		board_restore(board, bestmove);
	}

	x = bestmove->x;
	fprintf(f, "pv = %s ", move_to_string(x, player, s));
	hash_code = board_get_hash_code(board);
	if (hash_get(search->pv_table, board, hash_code, hash_data)) {
		fprintf(f, ":%02d@%d%%[%+03d,%+03d]; ", hash_data->depth, selectivity_table[hash_data->selectivity].percent, hash_data->lower, hash_data->upper);
	}
	while (x != NOMOVE) {
		board_get_move(board, x, move);
		board_update(board, move);
		player ^= 1;

		hash_code = board_get_hash_code(board);
		if (hash_get(search->pv_table, board, hash_code, hash_data)) {
			x = hash_data->move[0];
			fprintf(f, "%s:%02d@%d%%[%+03d,%+03d]; ", move_to_string(x, player, s), hash_data->depth, selectivity_table[hash_data->selectivity].percent, hash_data->lower, hash_data->upper);
		} else if (hash_get(search->hash_table, board, hash_code, hash_data)) {
			x = hash_data->move[0];
			fprintf(f, "{%s}:%2d@%d%%[%+03d,%+03d]; ", move_to_string(x, player, s), hash_data->depth, selectivity_table[hash_data->selectivity].percent, hash_data->lower, hash_data->upper);
		} else x = NOMOVE;
	}
	fputc('\n', f);

	spin_unlock(search->result);
}

/**
 * @brief Check if PV is ok.
 *
 * @param search Search.
 * @param bestmove Best move.
 * @param search_depth Depth.
 * @return true if PV is ok.
 */
bool is_pv_ok(Search *search, int bestmove, int search_depth)
{
	Board board[1];
	Move move[1];
	int x;
	unsigned long long hash_code;
	HashData hash_data[1];

	*board = *search->board;

	x = bestmove;
	while (search_depth > 0 && x != NOMOVE) {
		if (x != PASS) --search_depth;
		board_get_move(board, x, move);
		board_update(board, move);

		hash_code = board_get_hash_code(board);
		if (hash_get(search->pv_table, board, hash_code, hash_data)) {
			x = hash_data->move[0];
		} else if (hash_get(search->hash_table, board, hash_code, hash_data)) {
			x = hash_data->move[0];
		} else break;
		if (hash_data->depth < search_depth || hash_data->selectivity < search->selectivity || hash_data->lower != hash_data->upper) return false;
		if (x == NOMOVE && !board_is_game_over(board)) return false;
	}
	return true;
}


/**
 * @brief Guess a move.
 *
 * this function is used when no move is found in the position retrieved
 * from the hash table, meaning a fail-low search without a good move happened.
 * A move is guessed after a "shallow search".
 * 
 * @param search Search.
 * @param board Board to guess move from.
 * @return a best move guessed from a shallow search (depth = 4).
 */
static int guess_move(Search *search, Board *board)
{
	HashData hash_data[1];
	Board saved = *search->board;

	*search->board = *board; search_setup(search);

	PVS_shallow(search, SCORE_MIN, SCORE_MAX, MIN(search->n_empties, 6));
	hash_get(search->shallow_table, board, board_get_hash_code(board), hash_data);

	*search->board = saved; search_setup(search);

	assert(hash_data->move[0] != NOMOVE || board_is_game_over(board));

	return hash_data->move[0];
}

/**
 * @brief Record best move.
 *
 * @param search Search.
 * @param init_board Initial board.
 * @param bestmove Best move.
 * @param alpha Alpha Bound.
 * @param beta Beta Bound.
 * @param depth Depth.
 */
void record_best_move(Search *search, const Board *init_board, const Move *bestmove, const int alpha, const int beta, const int depth)
{
	Board board[1];
	Move move[1];
	Result *result = search->result;
	int x;
	unsigned long long hash_code;
	HashData hash_data[1];
	bool has_changed;
	Bound *bound = result->bound + bestmove->x;
	bool fail_low;
	bool guess_pv;
	int expected_depth, expected_selectivity, tmp;
	Bound expected_bound;

	*board = *init_board;

	spin_lock(result);

	has_changed = (result->move != bestmove->x || result->depth != depth || result->selectivity != search->selectivity);

	result->move = bestmove->x;
	result->score = bestmove->score;

	assert(search->stability_bound.lower <= result->score && result->score <= search->stability_bound.upper);

	if (result->score < beta && result->score < bound->upper) bound->upper = result->score;
	if (result->score > alpha && result->score > bound->lower) bound->lower = result->score;
	if (bound->lower > bound->upper) {
		if (result->score < beta) bound->upper = result->score; else bound->upper = search->stability_bound.upper;
		if (result->score > alpha) bound->lower = result->score; else bound->lower = search->stability_bound.lower;
	}

	expected_depth = result->depth = depth;
	expected_selectivity = result->selectivity = search->selectivity;
	expected_bound = *bound;

	line_init(result->pv, search->player);
	x = bestmove->x;

	guess_pv = (search->options.guess_pv && depth == search->n_empties && (bestmove->score <= alpha || bestmove->score >= beta));
	fail_low = (bestmove->score <= alpha);

	while (x != NOMOVE) {
		board_get_move(board, x, move);
		if (board_check_move(board, move)) {
			board_update(board, move);
			--expected_depth; 
			tmp = expected_bound.upper; expected_bound.upper = -expected_bound.lower; expected_bound.lower = -tmp;
			fail_low = !fail_low;
			line_push(result->pv, move->x);

			hash_code = board_get_hash_code(board);
			if ((hash_get(search->pv_table, board, hash_code, hash_data) || hash_get(search->hash_table, board, hash_code, hash_data)) 
			 && (hash_data->depth >= expected_depth && hash_data->selectivity >= expected_selectivity)
			 && (hash_data->upper <= expected_bound.upper && hash_data->lower >= expected_bound.lower)) {
				x = hash_data->move[0];
			} else x = NOMOVE;
			if (guess_pv && x == NOMOVE && fail_low) x = guess_move(search, board);
		} else x = NOMOVE;
	}

	result->time = search_time(search);
	result->n_nodes = search_count_nodes(search);

	spin_unlock(result);

	if (log_is_open(search_log)) {
		lock(search_log);
			log_print(search_log, "id = %d ; ", search->id);
			log_print(search_log, "level = %2d@%2d%% ; ", result->depth, selectivity_table[result->selectivity].percent);
			log_print(search_log, "ab = [%+03d, %+03d]:\n", alpha, beta);
			log_print(search_log, "stability bounds = [%+03d, %+03d]:\n", search->stability_bound.lower, search->stability_bound.upper);
			log_print(search_log, "%+03d < score = %+03d < %+03d; time = ", result->bound[result->move].lower, result->score, result->bound[result->move].upper);
			time_print(result->time, false, search_log->f);
			log_print(search_log, "; nodes = %lld N; ", result->n_nodes);
			if (result->time > 0) {log_print(search_log, "speed = %9.0f Nps", 1000.0 * result->n_nodes / result->time);}
			log_print(search_log, "\npv = ");
			line_print(result->pv, 200, " ", search_log->f);
			log_print(search_log, "\npv-debug = ");
			pv_debug(search, bestmove, search_log->f);
			log_print(search_log, "\n\n");
			fflush(search_log->f);
		unlock(search_log);
	}

	if (has_changed && options.noise <= depth && search->options.verbosity == 3) search->observer(search->result);
}

void show_current_move(FILE *f, Search *search, const Move *move, const int alpha, const int beta, const bool parallel) {
	char s[4];

	fprintf(f, "current move: %s [%+03d, %+03d], %s => %+03d; ", (parallel ? " // ":" -- "), alpha, beta, move_to_string(move->x, BLACK, s), move->score);
	pv_debug(search, move, f);
}

/**
 * @brief bound root scores according to stable squares 
 *
 * @param search Position to search.
 * @param score score to bound.
 * @return score;
 */
int search_bound(const Search *search, int score) 
{
	if (score < search->stability_bound.lower) score = search->stability_bound.lower;
	if (score > search->stability_bound.upper) score = search->stability_bound.upper;
	return score;
}

/**
 * @brief Reroute the PVS between midgame,endgame or terminal PVS.
 *
 * @param search Search.
 * @param alpha Alpha bound.
 * @param beta Beta bound.
 * @param depth Depth.
 * @param node Node position.
 * @return A score, as a disc difference.
 */
static int search_route_PVS(Search *search, int alpha, int beta, const int depth, Node *node)
{
	int score;

	assert(alpha < beta);
	assert(SCORE_MIN <= alpha && alpha <= SCORE_MAX);
	assert(SCORE_MIN <= beta && beta <= SCORE_MAX);
	assert(depth >= 0 && depth <= search->n_empties);

	if (depth == search->n_empties) {
		if (depth == 0) score = search_solve_0(search);
		else score = PVS_midgame(search, alpha, beta, depth, node);
	} else {
		if (depth == 0) score = search_eval_0(search);
		else if (depth == 1) score = search_eval_1(search, alpha, beta);
		else if (depth == 2) score = search_eval_2(search, alpha, beta);
		else score = PVS_midgame(search, alpha, beta, depth, node);
	}

	score = -search_bound(search, -score);
	assert(SCORE_MIN <= score && score <= SCORE_MAX);

	return score;
}

/**
 * @brief Compute a cost as a combination of node count, depth, etc. from hash_table.
 *
 * The board is supposed to be updated by a move after the root position.
 *
 * @param search Search.
 * @return A search cost. 
 */
int search_get_pv_cost(Search *search)
{
	HashTable *hash_table = search->hash_table;
	HashTable *pv_table = search->pv_table;
	HashTable *shallow_table = search->shallow_table;
	HashData hash_data[1];
	const unsigned long long hash_code = board_get_hash_code(search->board);

	if ((hash_get(pv_table, search->board, hash_code, hash_data) || hash_get(hash_table, search->board, hash_code, hash_data) || hash_get(shallow_table, search->board, hash_code, hash_data))) {
		return writeable_level(hash_data);
	}
	return 0;
}

/**
 * @brief Principal Variation Search algorithm at the root of the tree.
 *
 * this function solves the position provided within the limits set by the alpha
 * and beta bounds. The movelist parameter is updated so that the bestmove is the
 * first of the list when the search ended.
 *
 * @param search Search.
 * @param alpha Alpha bound.
 * @param beta Beta bound.
 * @param depth Remaining depth.
 * @return A score, as a disc difference.
 */
int PVS_root(Search *search, const int alpha, const int beta, const int depth)
{
	HashData hash_data[1];
	MoveList *movelist = search->movelist;
	Board *board = search->board;
	Move *move;
	Node node[1];
	long long cost = -search_count_nodes(search);
	unsigned long long hash_code;
	assert(alpha < beta);
	assert(SCORE_MIN <= alpha && alpha <= SCORE_MAX);
	assert(SCORE_MIN <= beta && beta <= SCORE_MAX);
	assert(depth > 0 && depth <= search->n_empties);

	search->probcut_level = 0;
	search->result->n_moves_left = search->result->n_moves;

	cassio_debug("PVS_root [%d, %d], %d@%d%%\n", alpha, beta, depth, selectivity_table[search->selectivity].percent);
	if (search->options.verbosity == 4) printf("PVS_root [%d, %d], %d@%d%%\n", alpha, beta, depth, selectivity_table[search->selectivity].percent);
	SEARCH_STATS(++statistics.n_PVS_root);
	SEARCH_UPDATE_INTERNAL_NODES();

	// transposition cutoff
	hash_code = board_get_hash_code(board);

	node_init(node, search, alpha, beta, depth, movelist->n_moves, NULL);
	node->pv_node = true;
	search->node_type[0] = PV_NODE;
	search->time.can_update = false;
	
	// special cases: pass or game over
	if (movelist_is_empty(movelist)) {
		move = movelist->move->next = movelist->move + 1;
		move->flipped = 0;
		if (can_move(board->opponent, board->player)) {
			search_update_pass_midgame(search);
				node->bestscore = move->score = -search_route_PVS(search, -node->beta, -node->alpha, depth, node);
			search_restore_pass_midgame(search);
			node->bestmove =  move->x = PASS;
		} else  { // game over
			node->bestscore =  move->score = search_solve(search);
			move->x = NOMOVE;
		}
	} else {

		// first move
		if ((move = node_first_move(node, movelist))) {
			assert(board_check_move(board, move));
			search_update_midgame(search, move); search->node_type[search->height] = PV_NODE;
				move->score = -search_route_PVS(search, -beta, -alpha, depth - 1, node);
				move->cost = search_get_pv_cost(search);
				assert(SCORE_MIN <= move->score && move->score <= SCORE_MAX);
				assert(search->stability_bound.lower <= move->score && move->score <= search->stability_bound.upper);
			search_restore_midgame(search, move);
			if (log_is_open(search_log)) show_current_move(search_log->f, search, move, alpha, beta, false);
			node_update(node, move);
			if (search->options.verbosity == 4) pv_debug(search, move, stdout);

			search->time.can_update = true;

			// other moves : try to refute the first/best one
			while ((move = node_next_move(node))) {
				const int alpha = depth > search->options.multipv_depth ? node->alpha : SCORE_MIN;

				assert(board_check_move(board, move));
				if (depth > search->options.multipv_depth && node_split(node, move)) {
				} else {
					search_update_midgame(search, move);
						move->score = -search_route_PVS(search, -alpha - 1, -alpha, depth - 1, node);
						if (alpha < move->score && move->score < beta) {
							search->node_type[search->height] = PV_NODE;
							move->score = -search_route_PVS(search, -beta, -alpha, depth - 1, node);
						}
						move->cost = search_get_pv_cost(search);
					assert(SCORE_MIN <= move->score && move->score <= SCORE_MAX);
					search_restore_midgame(search, move);
					if (log_is_open(search_log)) show_current_move(search_log->f, search, move, alpha, beta, false);
					node_update(node, move);
					assert(SCORE_MIN <= node->bestscore && node->bestscore <= SCORE_MAX);
				}
				if (search->options.verbosity == 4) pv_debug(search, move, stdout);
				if (search_time(search) > search->time.maxi && node->bestscore > alpha) search->stop = STOP_TIMEOUT;
			}
			node_wait_slaves(node);
		}
	}


	if (!search->stop) {
		hash_get(search->pv_table, board, hash_code, hash_data);
		if (depth < search->options.multipv_depth) movelist_sort(movelist);
		else movelist_sort_cost(movelist, hash_data);
		movelist_sort_bestmove(movelist, node->bestmove);
		record_best_move(search, board, movelist_first(movelist), alpha, beta, depth);

		if (movelist->n_moves == get_mobility(board->player, board->opponent)) {
			cost += search_count_nodes(search);
			hash_store(search->hash_table, board, hash_code, depth, search->selectivity, last_bit(cost), alpha, beta, node->bestscore, node->bestmove);
			if (search->options.guess_pv) hash_force(search->pv_table, board, hash_code, depth, search->selectivity, last_bit(cost), alpha, beta, node->bestscore, node->bestmove);
			else hash_store(search->pv_table, board, hash_code, depth, search->selectivity, last_bit(cost), alpha, beta, node->bestscore, node->bestmove);
		}

		assert(SCORE_MIN <= node->bestscore && node->bestscore <= SCORE_MAX);

	}

	node_free(node);

	return node->bestscore;
}

/**
 * @brief Aspiration window.
 *
 * @param search Search.
 * @param alpha Alpha bound.
 * @param beta Beta bound.
 * @param depth Depth.
 * @param score Previous score.
 * @return A score.
 */
int aspiration_search(Search *search, int alpha, int beta, const int depth, int score)
{
	int low, high, left, right;
	int width;
	int i;
	int old_score;
	Move *move;
	extern Log xboard_log[1];

	assert(alpha < beta);
	assert(SCORE_MIN <= alpha && alpha <= SCORE_MAX);
	assert(SCORE_MIN <= beta && beta <= SCORE_MAX);
	assert(SCORE_MIN <= score && score <= SCORE_MAX);
	assert(depth >= 0 && depth <= search->n_empties);

	log_print(xboard_log, "edax (search)> search [%d, %d] %d (%d)\n", alpha, beta, depth, score);

	if (is_depth_solving(depth, search->n_empties)) {
		if (alpha & 1) --alpha;
		if (beta & 1) ++beta;
	}

	// at shallow depths always use a large window, for better move ordering
	if (depth <= search->options.multipv_depth) {
		alpha = SCORE_MIN;
		beta = SCORE_MAX;
	}


	high = MIN(SCORE_MAX, search->stability_bound.upper + 2);
	low = MAX(SCORE_MIN, search->stability_bound.lower - 2);
	if (alpha < low) alpha = low;
	if (beta > high) beta = high;
	if (score < low) score = low; else if (score > high) score = high;
	if (score < alpha) score = alpha; else if (score > beta) score = beta;
	log_print(search_log, "initial bound = [%+03d, %+03d]\n", low, high);

	foreach_move(move, search->movelist) {
		search->result->bound[move->x].lower = low;
		search->result->bound[move->x].upper = high;
	}

	width = 10 - depth; if (width < 1) width = 1;
	if ((width & 1) && depth == search->n_empties) ++width;
	
	for (i = 0; i < 10; ++i) {
		old_score = score;
	
		// if in multipv mode or the alphabeta window is already small, search directly
		if (depth <= search->options.multipv_depth || beta - alpha <= 2 * width) {
			log_print(search_log, "direct root_PVS [%d, %d]:\n", low, high);
			score = PVS_root(search, alpha, beta, depth);
		} else { // otherwise iterate search with small windows until the score is bounded by the window, or a cut
			left = right = (i <= 0 ? 1 : i) * width;
			for (;;) {
				low = score - left; if (low < alpha) low = alpha;
				high = score + right; if (high > beta) high = beta;
				if (low >= high) break;
				if (low >= SCORE_MAX) low = SCORE_MAX - 1;
				if (high <= SCORE_MIN) high = SCORE_MIN + 1;
				log_print(search_log, "aspiration search [%d, %d]:\n", low, high);

				score = PVS_root(search, low, high, depth);

				if (search->stop) break;

				if (score <= low && score > alpha && left > 0) {
					left *= 2;
					right = 0;
				} else if (score >= high && score < beta && right > 0) {
					left = 0;
					right *= 2;
				} else break;
			}
		}
		// emergency stop
		if (search->stop) break;

		// check PV if alpha < score < beta
		if (is_depth_solving(depth, search->n_empties)
		&& ((alpha < score && score < beta) || (score == alpha && score == options.alpha) || (score == beta && score == options.beta))
		&& !is_pv_ok(search, search->result->move, depth)) {
			log_print(search_log, "*** WRONG PV => re-research id %d ***\n", search->id);
			if (log_is_open(search_log)) {
				pv_debug(search, movelist_first(search->movelist), search_log->f);
				putc('\n', search_log->f); fflush(search_log->f);
			}
			if (options.debug_cassio) {
				printf("DEBUG: Wrong PV: "); pv_debug(search, movelist_first(search->movelist), stdout); putchar('\n'); fflush(stdout); 
				if (log_is_open(engine_log)) {
					fprintf(engine_log->f, "DEBUG: Wrong PV: "); pv_debug(search, movelist_first(search->movelist), engine_log->f);
					putc('\n', engine_log->f); fflush(engine_log->f);
				}
			}
			continue;
		}
		if (is_depth_solving(depth, search->n_empties) && (score & 1)) {
			log_print(search_log, "*** UNEXPECTED ODD SCORE (score=%+d) => re-research id %d ***\n", score, search->id);			
			cassio_debug("wrong odd score => re-research.\n");
			continue;
		}
		if (score == old_score) break;
	}

	if (!search->stop) record_best_move(search, search->board, movelist_first(search->movelist), alpha, beta, depth);
	search->result->time = search_time(search);
	search->result->n_nodes = search_count_nodes(search);
	if (options.noise <= depth && search->options.verbosity >= 2) {
		search->observer(search->result);
	}

	return score;
}

/**
 * @brief Retrieve the last level of the search.
 *
 * @param search Search.
 * @param depth Depth of the last search.
 * @param selectivity Selectivity of the last search.
 */
static bool get_last_level(Search *search, int *depth, int *selectivity)
{
	int i, d, s, x;
	Board board[1];
	Move move[1];
	unsigned long long hash_code;
	HashData hash_data[1];
	

	*board = *search->board;

	*depth = *selectivity = -1;

	for (i = 0; i < 4; ++i) {
		hash_code = board_get_hash_code(board);
		if (hash_get(search->pv_table, board, hash_code, hash_data)) {
			x = hash_data->move[0];
		} else if (hash_get(search->hash_table, board, hash_code, hash_data)) {
			x = hash_data->move[0];
		} else break;

		d = hash_data->depth + i;
		s = hash_data->selectivity;

		if (d > *depth) *depth = d;
		if (s > *selectivity) *selectivity = s;

		board_get_move(board, x, move);
		board_update(board, move);
		
		if (x == PASS) --i;
	}

	return *depth > -1 && *selectivity > -1;
}



/**
 * @brief Iterative deepening.
 *
 * Search the current position at increasing depths, and once full depth
 * (equaling the number of empties) has been reached at decreasing selectivity ;
 * until the position is eventually solved.
 *
 * @param search Search.
 * @param alpha Alpha bound.
 * @param beta Beta bound.
 */
void iterative_deepening(Search *search, int alpha, int beta)
{
	Result *result = search->result;
	MoveList *movelist = search->movelist;
	Board *board = search->board;
	Move *bestmove, *move;
	HashData hash_data[1];
	int score, end, start;
	long long t;
	bool has_time;
	int old_depth, old_selectivity, tmp_selectivity;

	assert(alpha < beta);
	assert(SCORE_MIN <= alpha && alpha <= SCORE_MAX);
	assert(SCORE_MIN <= beta && beta <= SCORE_MAX);

	// initialize result
	result->move = NOMOVE;
	result->score = -SCORE_INF;
	result->depth = -1;
	result->selectivity = 0;
	result->time = 0;
	result->n_nodes = 0;
	line_init(result->pv, search->player);

	// special case: game over...
	if (movelist_is_empty(movelist) && !can_move(board->opponent, board->player)) {
		result->move = NOMOVE;
		result->score = search_solve(search);
		result->depth = search->n_empties;
		result->selectivity = NO_SELECTIVITY;
		result->time = search_time(search);
		result->n_nodes = search_count_nodes(search);
		result->bound[NOMOVE].lower = result->bound[NOMOVE].upper = result->score;
		line_init(result->pv, search->player);
		return;
	}

	score = search_bound(search, search_eval_0(search));
	end = search->options.depth;
	if (end >= search->n_empties) {
		end = search->n_empties - ITERATIVE_MIN_EMPTIES + 2;
		if (end <= 0) end = 2 - (search->n_empties & 1);
	}
	start = 6 - (end & 1);
	if (start > end - 2) start = end - 2;
	if (start <= 0) start = 2 - (end & 1);

	if (USE_PROBCUT && search->options.depth > 10) search->selectivity = 0;
	else search->selectivity = NO_SELECTIVITY;

	old_depth = 0; old_selectivity = search->selectivity;

	if (log_is_open(search_log)) {
		lock(search_log);
		log_print(search_log, "\n\n*** Search: id: %d ***\n", search->id);
	}

	// reuse last search ?
	if (hash_get(search->pv_table, board, board_get_hash_code(board), hash_data)) {
		char s[2][3];
		if (search->options.verbosity >= 2) {
			info("<hash: value = [%+02d, %+02d] ; bestmove = %s, %s ; level = %d@%d%% ; date = %d ; cost = %d>\n",
				hash_data->lower, hash_data->upper,
				move_to_string(hash_data->move[0], search->player, s[0]),
				move_to_string(hash_data->move[1], search->player, s[1]),
				hash_data->depth, selectivity_table[hash_data->selectivity].percent,
				hash_data->date, hash_data->cost);
		}
		if (log_is_open(search_log)) {
			log_print(search_log, "--- Next Search ---: ");
			hash_print(hash_data, search_log->f);
		}
		old_depth = hash_data->depth;
		old_selectivity = hash_data->selectivity;

		if (USE_PREVIOUS_SEARCH) {
			if (hash_data->lower == hash_data->upper) {
				if (get_last_level(search, &old_depth, &old_selectivity)) {
					start = old_depth;
					search->selectivity = old_selectivity;
				}
				score = hash_data->lower;
			} else {
				search_adjust_time(search, true);
				log_print(search_log, "--- New Search (inexact score) ---:\n");
			}
		}

	} else {
		search_adjust_time(search, false);
		log_print(search_log, "--- New Search ---:\n");
	}

	if (search->selectivity > search->options.selectivity) search->selectivity = search->options.selectivity;

	if (start > search->options.depth) start = search->options.depth;
	if (start > search->n_empties) start = search->n_empties;
	if (start < search->n_empties) {
		if ((start & 1) != (end & 1)) ++start;
		if (start <= 0) start = 2 - (end & 1);
		if (start > end) start = end;
	}

	if (log_is_open(search_log)) {
		log_print(search_log,"date: pv = %d, main = %d %s\n", search->pv_table->date, search->hash_table->date, search->options.keep_date ? "(keep)":"");
		log_print(search_log,"iterating from level %d@%d\n", start, selectivity_table[search->selectivity].percent);
		log_print(search_log, "alloted time: mini=%.1fs maxi=%.1fs extra=%.1fs\n", 0.001 * search->time.mini, 0.001 * search->time.maxi, 0.001 * search->time.extra);
		unlock(search_log);
	}

	// sort moves & display initial value
	tmp_selectivity = search->selectivity; search->selectivity = old_selectivity;
	if (!movelist_is_empty(movelist)) {
		if (end == 0) { // shuffle the movelist
			foreach_move(move, movelist) move->score = (random_get(search->random) & 0x7fffffff);
			// randomness is not perfect as several moves may share the same value, but this should be rare enough not to care about it.
		} else { // sort the moves
			movelist_evaluate(movelist, search, hash_data, alpha, start);
		}
		movelist_sort(movelist);
		bestmove = movelist_first(movelist); bestmove->score = score;
		record_best_move(search, board, bestmove, alpha, beta, old_depth);
		assert(SCORE_MIN <= result->score  && result->score <= SCORE_MAX);
	} else {
		Move pass = MOVE_PASS;
		bestmove = &pass; bestmove->score = score;
		record_best_move(search, board, bestmove, alpha, beta, old_depth);
		assert(SCORE_MIN <= result->score  && result->score <= SCORE_MAX);
	}
	search->selectivity = tmp_selectivity;

	// display current search status
	if (options.noise <= start && search->options.verbosity >= 2) {
		search->result->time = search_time(search);
		search->result->n_nodes = search_count_nodes(search);
		search->observer(search->result);
	}

	// special case : level 0
	if (end == 0) {
		return;		
	}

	// midgame : iterative depth
	for (search->depth = start; search->depth < end; search->depth += 2) {
		search->depth_pv_extension = get_pv_extension(search->depth, search->n_empties);
		score = aspiration_search(search, alpha, beta, search->depth, score);
		if (!search_continue(search)) return;
		if (abs(score) >= SCORE_MAX - 1 && search->depth > end - ITERATIVE_MIN_EMPTIES && search->options.depth >= search->n_empties) break;
	}
	search->depth = end;

	// switch to endgame
	if (search->options.depth >= search->n_empties) search->depth = search->n_empties;

	// iterative selectivity
	t = (search->options.time - search_time(search));
	has_time = (solvable_depth(t / 10, search_count_tasks(search)) > search->depth);
	for (; search->selectivity <= search->options.selectivity; ++search->selectivity) {
		if (search->depth == search->n_empties && 
		(  (search->depth < 21 && search->selectivity >= 1)
		|| (search->depth < 24 && search->selectivity >= 2)
		|| (search->depth < 27 && search->selectivity >= 3)
		|| (search->depth < 30 && search->selectivity >= 4)
		|| (has_time && search->depth < 30 && search->selectivity >= 2)
		|| (abs(score) >= SCORE_MAX))) { // jump to exact endgame (to solve faster) ?
			search->selectivity = search->options.selectivity;
		}
		if (search->selectivity == search->options.selectivity) search_adjust_time(search, true);
		score = aspiration_search(search, alpha, beta, search->depth, score);
		if (!search_continue(search)) return;
	}
	if (search->selectivity > search->options.selectivity) search->selectivity = search->options.selectivity;
}

/**
 * @brief Search the bestmove of a given board.
 *
 * this is a function runable within its own thread.
 * The board is supposed to have been set (by search_set_board()), and all
 * search options (level, time, etc.) too. this function proceeds to some
 * internal initialisations and then call the iterative deepening function, from
 * where the search is actually done. After the search ends, some finalizations
 * are done before the function returns.
 *
 * @param v Search cast as void.
 * @return The search result.
 */
void* search_run(void *v)
{
	Search *search = (Search*) v;
	Move *move;

	search->stop = RUNNING;

	//initialisations
	search->n_nodes = 0;
	search->child_nodes = 0;
	search->time.spent = -search_clock(search);
	search_time_init(search);
	if (!search->options.keep_date) {
		hash_clear(search->hash_table);
		hash_clear(search->pv_table);
		hash_clear(search->shallow_table);
	}
	search->height = 0;
	search->node_type[search->height] = PV_NODE;
	search->depth_pv_extension = get_pv_extension(0, search->n_empties);
	search->stability_bound.upper = SCORE_MAX - 2 * get_stability(search->board->opponent, search->board->player);
	search->stability_bound.lower = 2 * get_stability(search->board->player, search->board->opponent) - SCORE_MAX;
	search->result->score = search_bound(search, search_eval_0(search));
	search->result->n_moves_left = search->result->n_moves = search->movelist->n_moves;
	search->result->book_move = false;

	if (!movelist_is_empty(search->movelist)) {
		foreach_move(move, search->movelist) {
			search->result->bound[move->x].lower = SCORE_MIN;
			search->result->bound[move->x].upper = SCORE_MAX;
		}
	} else {
		search->result->bound[PASS].lower = SCORE_MIN;
		search->result->bound[PASS].upper = SCORE_MAX;
	}
	
	// search using iterative deepening (& widening).
	iterative_deepening(search, options.alpha, options.beta);

	// finalizations
	search->result->n_nodes = search_count_nodes(search);
	if (search->options.verbosity) {
		if (search->options.verbosity == 1 || options.noise > search->result->depth) search->observer(search->result);
		if (search->stop == STOP_TIMEOUT) {info("[Search out of time]\n");}
		else if (search->stop == STOP_ON_DEMAND) {info("[Search stopped on user demand]\n");}
		else if (search->stop == STOP_PONDERING) {info("[Pondering stopped]\n");}
		else if (search->stop == RUNNING) {info("[Search completed]\n");}
	}

	if (log_is_open(search_log)) {
		lock(search_log);
		log_print(search_log, "\n*** Search id: %d ", search->id);
		if (search->stop == STOP_TIMEOUT) log_print(search_log, "out of time");
		else if (search->stop == STOP_ON_DEMAND) log_print(search_log, "stopped on user demand");
		else if (search->stop == STOP_PONDERING) log_print(search_log, "stop pondering");
		else if (search->stop == STOP_PARALLEL_SEARCH) log_print(search_log, "### BUG: stop parallel search reached root! ###");
		else if (search->stop == RUNNING) log_print(search_log, "completed");
		else log_print(search_log, "### BUG: unkwown stop condition %d ###", search->stop);
		log_print(search_log, " ***\n\n");
		unlock(search_log);
	}

	if (search->stop == RUNNING) search->stop = STOP_END;
	search->time.spent += search_clock(search);
	search->result->time = search->time.spent;

	statistics_sum_nodes(search);
	if (search->options.verbosity >= 3) statistics_print(stdout);

	assert(search->height == 0);

	return search->result;
}

