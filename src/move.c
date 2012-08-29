/**
 * @file move.c
 *
 * @brief Move & list of moves management.
 *
 * @date 1998 - 2012
 * @author Richard Delorme
 * @version 4.3
 */

#include "move.h"

#include "bit.h"
#include "board.h"
#include "hash.h"
#include "search.h"
#include "settings.h"
#include "stats.h"

#include <limits.h>
#include <assert.h>
#include <ctype.h>
#include <string.h>

const Move MOVE_INIT = {0, NOMOVE, -SCORE_INF, 0, NULL};
const Move MOVE_PASS = {0, PASS, -SCORE_INF, 0, NULL};

const int SQUARE_VALUE[] = {
	// JCW's score:
	18,  4,  16, 12, 12, 16,  4, 18,
	 4,  2,   6,  8,  8,  6,  2,  4,
	16,  6,  14, 10, 10, 14,  6, 16,
	12,  8,  10,  0,  0, 10,  8, 12,
	12,  8,  10,  0,  0, 10,  8, 12,
	16,  6,  14, 10, 10, 14,  6, 16,
	 4,  2,   6,  8,  8,  6,  2,  4,
	18,  4,  16, 12, 12, 16,  4, 18
};

/**
 * @brief Get a symetric square coordinate
 *
 * @param x Square coordinate.
 * @param sym Symetry.
 * @return Symetric square coordinate.
 */
int symetry(int x, const int sym)
{
	if (A1 <= x && x <= H8) {
		if (sym & 1) {
			x = ((x & 070) | (7 - (x & 7)));
		}
		if (sym & 2) {
			x = ((070 - (x & 070)) | (x & 7));
		}
		if (sym & 4) {
			x = (((x & 070) >> 3) | ((x & 7) << 3));
		}
		assert(A1 <= x && x <= H8);
	}

	return x;
}


/**
 * @brief Print out a move
 *
 * Print the move, using letter case to distinguish player's color,
 * to an output stream.
 * @param x Square coordinate to print.
 * @param player Player color.
 * @param s Output string.
 * @return the output string.
 */
char* move_to_string(const int x, const int player, char *s)
{
	if (x == NOMOVE) {
		s[0] = '-';
		s[1] = '-';
	} else if (x == PASS) {
		s[0] = 'p';
		s[1] = 'a';
	} else if (x >= A1 && x <= H8) {
		s[0] = x % 8 + 'a';
		s[1] = x / 8 + '1';
	} else {
		s[0] = '?';
		s[1] = '?';
	}

	if (player == BLACK) {
		s[0] = toupper(s[0]);
		s[1] = toupper(s[1]);
	}
	s[2] = '\0';

	return s;
}

/**
 * @brief Print out a move
 *
 * Print the move, using letter case to distinguish player's color,
 * to an output stream.
 * @param x      square coordinate to print.
 * @param player player color.
 * @param f      output stream.
 */
void move_print(const int x, const int player, FILE *f)
{
	int s[2];

	if (x == NOMOVE) {
		s[0] = '-';
		s[1] = '-';
	} else if (x == PASS) {
		s[0] = 'p';
		s[1] = 's';
	} else if (x >= A1 && x <= H8) {
		s[0] = x % 8 + 'a';
		s[1] = x / 8 + '1';
	} else {
		s[0] = '?';
		s[1] = '?';
	}

	if (player == BLACK) {
		s[0] = toupper(s[0]);
		s[1] = toupper(s[1]);
	}
	fputc(s[0], f);
	fputc(s[1], f);
}




/**
 * @brief Check if a move wins 64-0.
 *
 * Check if a move flipped all the opponent discs.
 *
 * @param move Move.
 * @param board Board.
 * @return true if a move wins 64-0, false otherwise.
 */
bool move_wipeout(const Move *move, const Board *board)
{
	return move->flipped == board->opponent;
}

#ifdef TUNE_EDAX
static int w_hash = 1 << 15;
static int w_eval = 1 << 15;
static int w_mobility = 1 << 15;
static int w_corner_stability = 1 << 11;
static int w_edge_stability = 1 << 11;
static int w_potential_mobility = 1 << 5;
static int w_low_parity = 1 << 3;
static int w_mid_parity = 1 << 2;
static int w_high_parity = 1 << 1;
#endif


/**
 * @brief Evaluate a list of move.
 *
 * Evaluate the moves to sort them. Evaluation is based on, in order of importance:
 *   - wipeout move    : 1 << 30
 *   - first hash move : 1 << 29
 *   - second hash move: 1 << 28
 *   - shallow search  : 1 << 22 to 1 << 14
 *   - opponent mobility:                 1 << 15            32768...1048576
 *   - player stability near the corner:  1 << 11             2048...24576
 *   - opponent potential mobility:       1 << 5                32...1024
 *   - square value                       1 << 1:               2 ...18
 *   - parity:                            1 << 0:               0 ... 1
 *
 * @param move   Move to evaluate.
 * @param search Position to evaluate.
 * @param hash_data   Position (maybe) stored in the hashtable.
 * @param sort_alpha   Alpha bound to evaluate moves.
 * @param sort_depth   depth for the shallow search
 */
static void move_evaluate(Move *move, Search *search, const HashData *hash_data, const int sort_alpha, const int sort_depth)
{
#ifndef TUNE_EDAX
	const int w_hash = 1 << 15;
	const int w_eval = 1 << 15;
	const int w_mobility = 1 << 15;
	const int w_corner_stability = 1 << 11;
	const int w_edge_stability = 1 << 11;
	const int w_potential_mobility = 1 << 5;
	const int w_low_parity = 1 << 3;
	const int w_mid_parity = 1 << 2;
	const int w_high_parity = 1 << 1;
#endif	

	Board *board = search->board;
	HashData dummy[1];

	if (move_wipeout(move, board)) move->score = (1 << 30);
	else if (move->x == hash_data->move[0]) move->score = (1 << 29);
	else if (move->x == hash_data->move[1]) move->score = (1 << 28);
	else {

		move->score = SQUARE_VALUE[move->x]; // square type
		if (search->n_empties < 12 && search->parity & QUADRANT_ID[move->x]) move->score += w_low_parity; // parity
		else if (search->n_empties < 21 && search->parity & QUADRANT_ID[move->x]) move->score += w_mid_parity; // parity
		else if (search->n_empties < 30 && search->parity & QUADRANT_ID[move->x]) move->score += w_high_parity; // parity

		if (sort_depth < 0) {
			board_update(board, move);
				SEARCH_UPDATE_ALL_NODES();
				move->score += (36 - get_potential_mobility(board->player, board->opponent)) * w_potential_mobility; // potential mobility
				move->score += get_corner_stability(board->opponent) * w_corner_stability; // corner stability
				move->score += (36 - get_weighted_mobility(board->player, board->opponent)) * w_mobility; // real mobility
			board_restore(board, move);
		} else {
			int selectivity = search->selectivity;
			search->selectivity = NO_SELECTIVITY;
			search_update_midgame(search, move);
				SEARCH_UPDATE_INTERNAL_NODES();
				move->score += (36 - get_potential_mobility(board->player, board->opponent)) * w_potential_mobility; // potential mobility
				move->score += get_edge_stability(board->opponent, board->player) * w_edge_stability; // edge stability
				move->score += (36 - get_weighted_mobility(board->player, board->opponent)) *  w_mobility; // real mobility
				switch(sort_depth) {
				case 0:
					move->score += ((SCORE_MAX - search_eval_0(search)) >> 2) * w_eval; // 1 level score bonus
					break;
				case 1:
					move->score += ((SCORE_MAX - search_eval_1(search, SCORE_MIN, -sort_alpha)) >> 1) * w_eval;  // 2 level score bonus
					break;
				case 2:
					move->score += ((SCORE_MAX - search_eval_2(search, SCORE_MIN, -sort_alpha)) >> 1) * w_eval;  // 3 level score bonus
					break;
				default:
					if (hash_get(search->hash_table, board_get_hash_code(search->board), dummy)) move->score += w_hash; // bonus if the position leads to a position stored in the hash-table
					move->score += ((SCORE_MAX - PVS_shallow(search, SCORE_MIN, -sort_alpha, sort_depth))) * w_eval; // > 3 level bonus
					break;
				}
			search_restore_midgame(search, move);
			search->selectivity = selectivity;
		}
	}
}

#ifdef TUNE_EDAX
#include "solver.h"

void tune_move_evaluate(Search *search, const char *filename, const char *w_name)
{
	int best_w;
	unsigned long long best_n_nodes;
	unsigned long long n_nodes, t;
	int i, *w;

	if (strcmp(w_name, "w_eval") == 0) w = &w_eval;
	else if (strcmp(w_name, "w_mobility") == 0) w = &w_mobility;
	else if (strcmp(w_name, "w_corner_stability") == 0) w = &w_corner_stability;
	else if (strcmp(w_name, "w_edge_stability") == 0) w = &w_edge_stability;
	else if (strcmp(w_name, "w_potential_mobility") == 0) w = &w_potential_mobility;
	else if (strcmp(w_name, "w_low_parity") == 0) w = &w_low_parity;
	else if (strcmp(w_name, "w_mid_parity") == 0) w = &w_mid_parity;
	else if (strcmp(w_name, "w_high_parity") == 0) w = &w_high_parity;
	else {
		warn("unknown parameter %s\n", w_name);
		return;
	}

	best_n_nodes = ULLONG_MAX;
	best_w = *w;

	for (i = -1; i <= 20; ++i) {
		*w = (i >= 0 ? 1 << i : 0);
		t = -time_clock();
		n_nodes = solver(search, filename);
		t += time_clock();
		printf("%s %d : nodes %llu : time %.3f\n", w_name, *w, n_nodes, 0.001 * t);
		if (n_nodes < best_n_nodes) {
			best_n_nodes = n_nodes;
			best_w = *w;
		}
	}
	printf("Best %s %d : %llu\n", w_name, best_w, best_n_nodes);
	*w = best_w;
}
#endif

/**
 * @brief Get moves from a position.
 *
 * @param movelist movelist.
 * @param board board.
 * @return move number.
 */
int movelist_get_moves(MoveList *movelist, const Board *board)
{
	Move *previous = movelist->move;
	Move *move = movelist->move + 1;
	unsigned long long moves = get_moves(board->player, board->opponent);
	int x;

	foreach_bit(x, moves) {
		board_get_move(board, x, move);
		move->score = -SCORE_INF;
		previous = previous->next = move;
		++move;
	}
	previous->next = NULL;
	movelist->n_moves = move - movelist->move - 1;

	assert(movelist->n_moves == get_mobility(board->player, board->opponent));

	return movelist->n_moves;
}



/**
 * @brief Print out a movelist
 *
 * Print the moves, using letter case to distinguish player's color,
 * to an output stream.
 * @param movelist a list of moves.
 * @param player   player color.
 * @param f        output stream.
 */
void movelist_print(const MoveList *movelist, const int player, FILE *f)
{
	Move *iter;

	for (iter = movelist->move->next; iter != NULL; iter = iter->next) {
		move_print(iter->x, player, f);
		fprintf(f, "[%d] ", iter->score);
	}
}

/**
 * @brief Return the next best move from the list.
 *
 * @param previous_best Last best move.
 * @return the following best move in the list.
 */
Move* move_next_best(Move *previous_best)
{
	if (previous_best->next) {
		Move *move, *best;
		best = previous_best;
		for (move = best->next; move->next != NULL; move = move->next) {
			if (move->next->score > best->next->score) {
				best = move;
			}
		}
		if (previous_best != best) {
			move = best->next;
			best->next = move->next;
			move->next = previous_best->next;
			previous_best->next = move;
		}
	}

	return previous_best->next;
}

/**
 * @brief Return the next best move from the list.
 *
 * @param previous_best Last best move.
 * @return the following best move in the list.
 */
Move* move_next_most_expensive(Move *previous_best)
{
	if (previous_best->next) {
		Move *move, *best;
		best = previous_best;
		for (move = best->next; move->next != NULL; move = move->next) {
			if (move->next->cost > best->next->cost) {
				best = move;
			}
		}
		if (previous_best != best) {
			move = best->next;
			best->next = move->next;
			move->next = previous_best->next;
			previous_best->next = move;
		}
	}

	return previous_best->next;
}

/**
 * @brief Return the next move from the list.
 *
 * @param move previous move.
 * @return the next move in the list.
 */
Move* move_next(Move *move)
{
	return move->next;
}

/**
 * @brief Return the best move of the list.
 *
 * @param movelist The list of move.
 * @return the best move.
 */
Move* movelist_best(MoveList *movelist)
{
	return move_next_best(movelist->move);
}

/**
 * @brief Return the first move of the list.
 *
 * @param movelist The list of move.
 * @return the first move.
 */
Move* movelist_first(MoveList *movelist)
{
	return move_next(movelist->move);
}

/**
 * @brief Evaluate a list of move in order to sort it.
 *
 * @param movelist List of moves to sort.
 * @param search Position to evaluate.
 * @param hash_data   Position (maybe) stored in the hashtable.
 * @param alpha   Alpha bound to evaluate moves.
 * @param depth   depth for the shallow search
 */
void movelist_evaluate(MoveList *movelist, Search *search, const HashData *hash_data, const int alpha, const int depth)
{
	int sort_depth, min_depth;
	int sort_alpha;
	Move *move;

	min_depth = 9;
	if (search->n_empties <= 27) min_depth += (30 - search->n_empties) / 3;
	if (depth >= min_depth) {
		sort_depth = (depth - 15) / 3;
		if (hash_data && hash_data->upper < alpha) sort_depth -= 2; 
		if (search->n_empties >= 27) ++sort_depth;
		if (sort_depth < 0) sort_depth = 0; else if (sort_depth > 6) sort_depth = 6;
	} else {
		sort_depth = -1;
	}

	sort_alpha = MAX(SCORE_MIN, alpha - SORT_ALPHA_DELTA);
	foreach_move (move, movelist) {
		move_evaluate(move, search, hash_data, sort_alpha, sort_depth);
	}
}

/**
 * @brief Sort a move as best.
 *
 * Put the best move at the head of the list.
 *
 * @param movelist List of moves to sort.
 * @param move Best move to to set first.
 * @return A move, occasionally illegal, whose successor was the original successor of the new best move.
 */
Move* movelist_sort_bestmove(MoveList *movelist, const int move)
{
	Move *iter, *previous;

	for (iter = (previous = movelist->move)->next; iter != NULL; iter = (previous = iter)->next) {
		if (iter->x == move) {
			previous->next = iter->next;
			iter->next = movelist->move->next;
			movelist->move->next = iter;
			break;
		}
	}
	return previous;
}

/**
 * @brief Sort all moves except the first, based on move cost & hash_table storage.
 *
 * @param movelist List of moves to sort.
 * @param hash_data  Data from the hash table.
 */
void movelist_sort_cost(MoveList *movelist, const HashData *hash_data)
{
	Move *iter;


	foreach_move(iter, movelist) {
		if (iter->x == hash_data->move[0]) iter->cost = INT_MAX;
		else if (iter->x == hash_data->move[1]) iter->cost = INT_MAX - 1;
	}
	for (iter =  move_next_most_expensive(movelist->move); iter; iter = move_next_most_expensive(iter));

}

/**
 * @brief Sort all moves.
 * @param movelist   List of moves to sort.
 */
void movelist_sort(MoveList *movelist)
{
	Move *move;
	foreach_best_move(move, movelist) ;
}

/**
 * @brief Exclude a move.
 * @param movelist   List of moves to sort.
 * @param move       Move to exclude.
 */
Move* movelist_exclude(MoveList *movelist, const int move)
{
	Move *iter, *previous;

	for (iter = (previous = movelist->move)->next; iter != NULL; iter = (previous = iter)->next) {
		if (iter->x == move) {
			previous->next = iter->next;
			break;
		}
	}
	if (iter) --movelist->n_moves;

	return iter;
}

/**
 * @brief Check if the list is empty.
 *
 * @param movelist The list of move.
 * @return true if the list is empty, false otherwise.
 */
bool movelist_is_empty(const MoveList *movelist)
{
	return movelist->n_moves == 0;
}


/**
 * @brief Initialize a sequence of moves.
 *
 * @param line the move sequence.
 * @param player color of the first player of the sequence.
 */
void line_init(Line *line, const int player)
{
	line->n_moves = 0;
	line->color = player;
}

/**
 * @brief Add a move to the sequence.
 *
 * @param line the move sequence.
 * @param x move coordinate.
 */
void line_push(Line* line, const int x)
{
	int i;

	assert(line->n_moves < GAME_SIZE);

	for (i = 0; i < line->n_moves; ++i) {
		assert(line->move[i] != x || x == PASS);
	}
	line->move[line->n_moves++] = (char) x;
}

/**
 * @brief Remove the last move from a sequence.
 *
 * @param line the move sequence.
 */
void line_pop(Line* line)
{
	assert(line->n_moves > 0);
	--line->n_moves;
}

/**
 * @brief Copy part of a sequence to another sequence.
 *
 * @param dest the destination move sequence.
 * @param src the source move sequence.
 * @param from the point to copy from.
 */
void line_copy(Line *dest, const Line *src, const int from)
{
	int i;

	for (i = from; i < src->n_moves; ++i) { // todo: replace by memcpy ?
		dest->move[i] = src->move[i];
	}
	dest->n_moves = src->n_moves;
	dest->color = src->color;
}

/**
 * @brief Print a move sequence.
 *
 * @param line the move sequence.
 * @param width width of the line to print (in characters).
 * @param separator a string to print between moves.
 * @param f output stream.
 */
void line_print(const Line *line, int width, const char *separator, FILE *f)
{
	int i;
	int player = line->color;
	int w = 2 + (separator ? strlen(separator) : 0);
	int len = abs(width);

	for (i = 0; i < line->n_moves && len > w; ++i, len -= w) {
		player = !player;
		if (separator && i) fputs(separator, f);
		move_print(line->move[i], player, f);
	}
	for (len = MIN(len, width); len > w; len -= w) {
		fputc(' ', f);fputc(' ', f); if (separator) fputs(separator, f);
	}
}

/**
 * @brief Line to string.
 *
 * @param line the move sequence.
 * @param n number of moves to add.
 * @param separator a string to print between moves.
 * @param string output string receiving the line.
 */
char* line_to_string(const Line *line, int n, const char *separator, char *string)
{
	int i;
	int player = line->color;
	int w = 2 + (separator ? strlen(separator) : 0);

	for (i = 0; i < line->n_moves && i < n; ++i) {
		move_to_string(line->move[i], player, string + w * i);
		player = !player;
		if (separator) strcpy(string + w * i + 2, separator);
	}
	string[w * i] =  '\0';
	return string;
}

/**
 * A position and a move issued from it.
 */
typedef struct {
	Board board;
	int x;
} MBoard;

/**
 * Array of MBoard.
 */
typedef struct MoveArray {
	MBoard *item; /**< dynamic array */
	int n;       /**< number of items in the array */
	int size;    /**< capacity of the array */
} MoveArray;

/**
 * @brief array initialisation.
 * @param array Array of positions.
 */
static void movearray_init(MoveArray *array)
{
	array->item = NULL;
	array->n = array->size = 0;
}

/**
 * @brief array supression.
 * @param array Array of positions.
 */
static void movearray_delete(MoveArray *array)
{
	free(array->item);
}

/**
 * @brief Append a position.
 * @param array Array of positions.
 * @param b Position.
 * @param x Move.
 * @return true if a position is added to the array, false if it is already present.
 */
static bool movearray_append(MoveArray *array, const Board *b, const int x)
{
	int i;
	for (i = 0; i < array->n; ++i) {
		if (array->item[i].x == x && board_equal(b, &(array->item[i].board))) return false;
	}

	if (array->size < array->n + 1) {
		array->item = (MBoard*) realloc(array->item, (array->size = (array->size + 1)) * sizeof (MBoard));
		if (array->item == NULL) fatal_error("Cannot re-allocate board array.\n");
	}
	array->item[array->n].board = *b;
	array->item[array->n].x = x;
	++array->n;
	return true;
}

/**
 * @brief Initialisation of the hash table.
 * @param hash Hash table.
 * @param bitsize Hash table size (as log2(size)).
 */
void movehash_init(MoveHash *hash, int bitsize)
{
	int i;

	hash->size = (1 << bitsize);
	hash->mask = hash->size - 1;
	hash->array = (MoveArray*) malloc(hash->size * sizeof (MoveArray));
	if (hash->array == NULL) fatal_error("Cannot re-allocate board array.\n");
	for (i = 0; i < hash->size; ++i) movearray_init(hash->array + i);
}

/**
 * @brief Free the hash table.
 * @param hash Hash table.
 */
void movehash_delete(MoveHash *hash)
{
	int i;

	for (i = 0; i < hash->size; ++i) movearray_delete(hash->array + i);
	free(hash->array);
}

/**
 * @brief Append a position to the hash table.
 * @param hash Hash table.
 * @param b Position.
 * @param x Move.
 * @return true if a position is added to the hash table, false otherwsise.
 */
bool movehash_append(MoveHash *hash, const Board *b, const int x)
{
	Board u;
	int y;
	unsigned long long h;

	y = symetry(x, board_unique(b, &u));
	h = board_get_hash_code(&u);
	return movearray_append(hash->array + (h & hash->mask), &u, y);
}

