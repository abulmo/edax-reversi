/**
 * @file book.c
 *
 * File for opening book management.
 *
 * Edax book is a set of positions. Each position is unique concerning all possible symetries.
 * A position is made of an othello board, a set of moves leading to other positions in the book,
 * called here "link", and the best remaining move, as evaluated by a search at fixed depth, called
 * here leaf. It also contains win/draw/loss statistics, which is actually useless, and a score
 * with two bounds from retropropagated error.
 * Several algorithms are present to add positions in the book, in the most usefull way.
 *
 * @date 1998 - 2017
 * @author Richard Delorme
 * @version 4.4
 */

#include "book.h"
#include "base.h"
#include "search.h"
#include "const.h"
#include "bit.h"
#include "options.h"
#include "util.h"

#include <assert.h>
#include <time.h>
#include <stdarg.h>
#include <limits.h>

#define BOOK_DEBUG 0
static const int BOOK_INFO_RESOLUTION = 100000;

#define clear_line() bprint("                                                                                \r")

bool book_verbose = false;

/**
 * @brief print a message on stdout.
 * @param format Format string.
 * @param ... variable arguments.
 */
static void bprint(const char *format, ...)
{
	if (book_verbose) {
		va_list args;

		va_start(args, format);
		vprintf(format, args);
		va_end(args);
		fflush (stdout);
		
	}
}

const Link BAD_LINK = {-SCORE_INF, NOMOVE};

/**
 * @brief read a link.
 *
 * @param link link to read in.
 * @param f input stream.
 */
static inline bool link_read(Link *link, FILE *f)
{
	int r;
	r  = fread(&link->score, 1, 1, f);
	r += fread(&link->move, 1, 1, f);
	return r == 2;
}

/**
 * @brief write a link.
 *
 * @param link link to write out.
 * @param f output stream.
 */
static inline bool link_write(const Link *link, FILE *f)
{
	int r;
	r  = fwrite(&link->score, 1, 1, f);
	r += fwrite(&link->move, 1, 1, f);
	return r == 2;
}

/**
 * @brief check if a link is unvalid.
 *
 * @param link checked link.
 * @return true if link is unvalid, false otherwise.
 */
static inline bool link_is_bad(const Link *link)
{
	return link->score == -SCORE_INF;
}

static Position* book_probe(const Book*, const Board*);
static void book_add(Book*, const Position*);
static void position_print(const Position*, const Board*, FILE*);

#define foreach_link(l, p)  \
	for ((l) = (p)->link; (l) < (p)->link + (p)->n_link; ++(l))

/**
 * @brief return the number of plies from where the search is solving.
 *
 * @param depth search depth.
 * @return book depth.
 */
static int get_book_depth(const int depth)
{
	if (depth <= 10) return 60 - 2 * depth;
	else if (depth <= 18) return 39;
	else if (depth <= 24) return 36;
	else if (depth < 30) return 33;
	else if (depth < 36) return 30;
	else if (depth < 42) return 66 - depth;
	else return 24;
}


/**
 * @brief Check if position is ok or need fixing.
 *
 * Note: All positions should always be OK! A wrong position means a BUG!
 *
 * @param position Position.
 * @return true if ok, false if it needs fixing.
 */
static bool position_is_ok(const Position *position)
{
	Board board[1];
	Move move[1];
	const Link *l;
	int i, j;
	char s[4];

	// board is legal ?
	if (position->board->player & position->board->opponent) {
		warn("Board is illegal: Two discs on the same square?\n");
		board_print(position->board, BLACK, stderr);
		return false;
	}
	if (((position->board->player|position->board->opponent) & 0x0000001818000000ULL) != 0x0000001818000000ULL) {
		warn("Board is illegal: Empty center?\n");
		board_print(position->board, BLACK, stderr);
		return false;
	}

	// is board unique
	board_unique(position->board, board);
	if (!board_equal(position->board, board)) {
		warn("board is not unique\n");
		position_print(position, position->board, stdout);
		return false;
	}

	// are moves legal ?
	foreach_link(l, position) {
		if (l->move == PASS) {
			if (position->n_link > 1
			 || can_move(board->player, board->opponent)
			 || !can_move(board->opponent, board->player)) {
				warn("passing move is wrong\n");
				position_print(position, position->board, stdout);
				return false;
			}
		} else {
			if (/*l->move < A1 ||*/ l->move > H8
			 || board_is_occupied(board, l->move)
			 || board_get_move(board, l->move, move) == 0) {
				warn("link %s is wrong\n", move_to_string(l->move, WHITE, s));
				position_print(position, position->board, stdout);
				return false;
			}
		}
	}

	l = &position->leaf;
	if (l->move == PASS) {
		if (position->n_link > 0
		 || can_move(board->player, board->opponent)
		 || !can_move(board->opponent, board->player)) {
			warn("passing move is wrong\n");
			position_print(position, position->board, stdout);
			return false;
		}
	} else if (l->move == NOMOVE) {
		if (get_mobility(position->board->player, position->board->opponent) != position->n_link && !(position->n_link == 1 && position->link->move == PASS)) {
			warn("nomove is wrong\n");
			position_print(position, position->board, stdout);
			return false;
		}
	} else if (/*l->move < A1 ||*/ l->move > H8
		 || board_is_occupied(board, l->move)
		 || board_get_move(board, l->move, move) == 0) {
			warn("leaf %s is wrong\n", move_to_string(l->move, WHITE, s));
			position_print(position, position->board, stdout);
			return false;
	}

	// doublons ?
	for (i = 0; i < position->n_link; ++i) {
		for (j = i + 1; j < position->n_link; ++j) {
			if (position->link[j].move == position->link[i].move) {
				warn("doublon found in links\n");
				position_print(position, position->board, stdout);
				return false;
			}
		}
		if (position->leaf.move == position->link[i].move) {
			warn("doublon found in links/leaf\n");
			position_print(position, position->board, stdout);
			return false;
		}
	}
	return true;
}

/**
 * @brief Initialize a position.
 *
 * @param position Position.
 */
static void position_init(Position *position)
{
	position->board->player = position->board->opponent = 0;

	position->leaf = BAD_LINK;
	position->link = NULL;

	position->n_wins = position->n_draws = position->n_losses = position->n_lines = 0;
	position->score.value = position->score.lower = -SCORE_INF;
	position->score.upper = +SCORE_INF;

	position->n_link = 0;
	position->level = 0;
	position->done = true;
	position->todo = false;
}

/**
 * @brief Merge a position with another one.
 *
 * A position is merged if its level is > to the destination position; or == and
 * its leaf move is not contains into the destination link moves.
 * 
 * Note: link moves are not copied. This can be done later with position_link().
 *
 * @param dest Destination position.
 * @param src Source position.
 */
static void position_merge(Position *dest, const Position *src)
{
	Link *l;

	position_init(dest);
	*dest->board = *src->board;
	if (dest->level == src->level) { 
		foreach_link(l, dest) {
			if (l->move == src->leaf.move) return;
		}
		dest->leaf = src->leaf;
	} else if (dest->level > src->level) {
		return;
	} else {
		dest->leaf = src->leaf;
		dest->level = src->level;
	}
}

/**
 * @brief Free resources used by a position.
 *
 * @param position Position.
 */
static void position_free(Position *position)
{
	free(position->link);
}

/**
 * @brief Read a position.
 *
 * @param position Position to read in.
 * @param f Input stream.
 */
static bool position_read(Position *position, FILE *f)
{
	int i;
	int r;

	r  = fread(&position->board->player, sizeof (unsigned long long), 1, f);
	r += fread(&position->board->opponent, sizeof (unsigned long long), 1, f);

	r += fread(&position->n_wins, sizeof (unsigned int), 1, f);
	r += fread(&position->n_draws, sizeof (unsigned int), 1, f);
	r += fread(&position->n_losses,  sizeof (unsigned int), 1, f);
	r += fread(&position->n_lines,  sizeof (unsigned int), 1, f);

	r += fread(&position->score.value, sizeof (short), 1, f);
	r += fread(&position->score.lower, sizeof (short), 1, f);
	r += fread(&position->score.upper, sizeof (short), 1, f);

	r += fread(&position->n_link, 1, 1, f);
	r += fread(&position->level, 1, 1, f);

	if (r != 11) return false;

	position->done = position->todo = false;

	if (position->n_link) {
		position->link = (Link*) malloc(sizeof (Link) * position->n_link);
		if (position->link == NULL) error("cannot allocate opening book position's moves\n");
		for (i = 0; i < position->n_link; ++i) {
			if (!link_read(position->link + i, f)) return false;
		}
	} else {
		position->link = NULL;
	}
	if (!link_read(&position->leaf, f)) return false;

	return true;
}

/**
 * @brief Read a position.
 *
 * @param position Position to read in.
 * @param f Input stream.
 */
static bool position_import(Position *position, FILE *f)
{
	char *line, *s, *old;
	int value;
	Move move[1];
	bool ok = false;

	if ((line = string_read_line(f)) != NULL) {
		position_init(position);
		s = parse_board(line, position->board, &value);
		if (s != line) {
			s = parse_find(s, ',');
			if (*s == ',') {
				value = -1;	s = parse_int(old = s + 1, &value); BOUND(value, -1, 60, "level");
				if (s != old && value != -1) {
					position->level = value;
					s = parse_find(s, ',');
					if (*s == ',') {
						s = parse_move(old = s + 1, position->board, move);
						if (s != old) {
							s = parse_find(s, ',');
							if (*s == ',') {
								s = parse_int(old = s + 1, &value);
								if (s != old) {
									position->leaf.move = move->x;
									position->leaf.score = value;
								}
							}
						}
					}
					ok = true;
				} else {
					warn("wrong level: %s\n", line);
				}
			} else {
				warn("missing ',' after board setting\n");
			}
		} else {
			warn("wrong board: %s\n", line);
		}
	}

	if (!ok) warn("=> wrong position\n");

	free(line);
	return ok;
}

/**
 * @brief Write a position.
 *
 * @param position position to write out.
 * @param f output stream.
 */
static bool position_write(const Position *position, FILE* f)
{
	int i;
	int r;

	r  = fwrite(&position->board->player, sizeof (unsigned long long), 1, f);
	r += fwrite(&position->board->opponent, sizeof (unsigned long long), 1, f);

	r += fwrite(&position->n_wins, sizeof (unsigned int), 1, f);
	r += fwrite(&position->n_draws, sizeof (unsigned int), 1, f);
	r += fwrite(&position->n_losses,  sizeof (unsigned int), 1, f);
	r += fwrite(&position->n_lines,  sizeof (unsigned int), 1, f);

	r += fwrite(&position->score.value, sizeof (short), 1, f);
	r += fwrite(&position->score.lower, sizeof (short), 1, f);
	r += fwrite(&position->score.upper, sizeof (short), 1, f);

	r += fwrite(&position->n_link, 1, 1, f);
	r += fwrite(&position->level, 1, 1, f);

	if (r != 11) return false;

	for (i = 0; i < position->n_link; ++i)
		if (!link_write(position->link + i, f)) return false;
	if (!link_write(&position->leaf, f)) return false;

	return true;
}

/**
 * @brief write a position.
 *
 * @param p position to write out.
 * @param f output stream.
 */
static bool position_export(const Position *p, FILE* f)
{
	char b[80], m[4];

	board_to_string(p->board, BLACK, b);
	move_to_string(p->leaf.move, BLACK, m);
	return (fprintf(f, "%s,%d,%s,%d\n", b, p->level, m, p->leaf.score) > 0);
}

/**
 * @brief Make position unique, regarding symetries.
 *
 * @param position position.
 */
static void position_unique(Position *position)
{
	Board board[1];
	int i, s;

	*board = *position->board;
	if ((s = board_unique(board, position->board)) != 0) {
		for (i = 0; i < position->n_link; ++i) {
			position->link[i].move = symetry(position->link[i].move, s);
		}
		position->leaf.move = symetry(position->leaf.move, s);
	}
}

/**
 * @brief Get moves from a position.
 *
 * @param position position to get moves from.
 * @param board board.
 * @param movelist movelist.
 */
static int position_get_moves(const Position *position, const Board *board, MoveList *movelist)
{
	Move *previous = movelist->move;
	Move *move = movelist->move + 1;
	Board sym[1];
	int i, x, s;

	for (s = 0; s < 8; ++s) {
		board_symetry(position->board, s, sym);

		if (board_equal(sym, board)) {
			for (i = 0; i < position->n_link; ++i) {
				x = symetry(position->link[i].move, s);
				board_get_move(board, x, move);
				move->score = position->link[i].score;
				previous = previous->next = move;
				++move;
			}
			x = symetry(position->leaf.move, s);
			if (x != NOMOVE) {
				board_get_move(board, x, move);
				move->score = position->leaf.score;
				previous = previous->next = move;
				++move;
			}
			previous->next = NULL;
			movelist->n_moves = move - movelist->move - 1;
			movelist_sort(movelist);
			return s;
		}
	}

	fatal_error("unreachable code\n");
	return -1;
}

/**
 * @brief print a position in a readable format.
 *
 * @param position position to print out.
 * @param board Symetrical board to use.
 * @param f output stream.
 */
static void position_show(const Position *position, const Board *board, FILE *f)
{
	MoveList movelist[1];
	Move *move;
	const int n_empties = board_count_empties(board);
	const int color = n_empties & 1;
	int sym;
	char s[4];

	board_print(board, color, f);

	fprintf(f, "\nLevel: %d\n", position->level);
	fprintf(f, "Best score: %+02d [%+02d, %+02d]\n", position->score.value, position->score.lower, position->score.upper);
	fprintf(f, "Moves:");
	sym = position_get_moves(position, board, movelist);
	foreach_move(move, movelist) {
		move_to_string(move->x, color, s);
		if (symetry(position->leaf.move, sym) == move->x) {
			fprintf(f, " <%s:%+02d>", s, move->score);
		} else {
			fprintf(f, " [%s:%+02d]", s, move->score);
		}
	}
}

/**
 * @brief print a position in a compact but readable format.
 *
 * @param position position to print out.
 * @param board Symetrical board to use.
 * @param f output stream.
 */
static void position_print(const Position *position, const Board *board, FILE *f)
{
	MoveList movelist[1];
	Move *move;
	int color = board_count_empties(board) & 1, sym;
	char b[80], m[4];

	board_to_string(board, color, b);
	fprintf(f, "{board:%s; ", b);
	fprintf(f, "level:%d; ", position->level);
	fprintf(f, "best: %+02d [%+02d, %+02d];", position->score.value, position->score.lower, position->score.upper);
	fprintf(f, "moves:");
	sym = position_get_moves(position, board, movelist);
	foreach_move(move, movelist) {
		move_to_string(move->x, color, m);
		if (symetry(position->leaf.move, sym) == move->x) {
			fprintf(f, " <%s:%+02d>", m, move->score);
		} else {
			fprintf(f, " [%s:%+02d]", m, move->score);
		}
	}
	fprintf(f, "}\n");
}

/**
 * @brief Chose a move at random from the position.
 *
 * @param position Position to chose a move from.
 * @param board Correctly rotated/mirrored board.
 * @param move Chosen move.
 * @param r Random data.
 * @param randomness Randomness intensity (randomness = 0 means no randomness).
 */
static void position_get_random_move(const Position *position, const Board *board, Move *move, Random *r, const int randomness)
{
	MoveList movelist[1];
	Move *m;
	int i, n;

	position_get_moves(position, board, movelist);

	n = 0;
	foreach_best_move(m, movelist) {
		if (position->score.value <= m->score + randomness) {
			++n;
		} else break;
	}

	if (n == 0) { // no move
		move->x = NOMOVE;
		move->flipped = 0;
		return;
	} else {
		i = (random_get(r) % n); // is good enough here
	}

	foreach_best_move(m, movelist) {
		if (i-- == 0) break;
	}

	*move = *m;
}

/**
 * @brief Add a link to this position.
 *
 * @param position Position to chose a move from.
 * @param link Link to add.
 * @return true if the link has been added, false if it was already present.
 */
static bool position_add_link(Position *position, const Link *link)
{
	Link *l;
	int last = position->n_link;

	foreach_link (l, position) {
		if (l->move == link->move) {
			l->score = link->score; // update the link ?
			return false;
		}
	}

	l = (Link*) realloc(position->link, sizeof (Link) * (++position->n_link));
	if (l == NULL) {
		--position->n_link;
		error("cannot allocate opening book position's moves\n");
		return false;
	}
	position->link = l;
	position->link[last] = *link;

	if (link->score > position->score.value) position->score.value = link->score;

	if (link->move == position->leaf.move) position->leaf = BAD_LINK;

	return true;
}

/**
 * @brief Sort the link moves.
 *
 * @param position Position to sort.
 */
static void position_sort(Position *position)
{
	Link *i, *j, *best;

	if (position->n_link > 1) {
		for (i = position->link; i < position->link + position->n_link - 1; ++i) {
			best = i;
			for (j = i + 1; j < position->link + position->n_link; ++j) {
				if (j->score > best->score) best = j;
			}
			if (best > i) {
				Link tmp = *best;
				*best = *i;
				*i = tmp;
			}
		}
	}
}

/**
 * @brief Evaluate a position.
 *
 * If needed, find the best remaining move, after link moves are excluded.
 *
 * @param position Position to search.
 * @param book Opening book.
 */
static void position_search(Position *position, Book *book)
{
	Search *search = book->search;
	Link *l;
	const int n_moves = get_mobility(position->board->player, position->board->opponent);
	long long time;
	bool time_per_move;

	if (position->leaf.move != NOMOVE && position_add_link(position, &position->leaf)) {
		book->need_saving = true;
		++book->stats.n_links;
	}

	if (position->n_link < n_moves || (position->n_link == 0 && n_moves == 0 && position->score.value == -SCORE_INF)) {
		search_set_board(search, position->board, BLACK);
		search_set_level(search, position->level, search->n_empties);

		foreach_link (l, position) {
			movelist_exclude(search->movelist, l->move);
		}

		if (search->options.verbosity >= 2) {
			board_print(search->board, search->player, stdout);
			puts(search->options.header);
			puts(search->options.separator);
		}

		time = search->options.time;
		time_per_move = search->options.time_per_move;
		search->options.time = TIME_MAX;
		search->options.time_per_move = true;

		search_run(search);

		search->options.time = time;
		search->options.time_per_move = time_per_move;

		position->leaf.score = search->result->score;
		position->leaf.move = search->result->move;
		if (position->leaf.score > position->score.value) {
			position->score.value = position->leaf.score;
		}
		book->need_saving = true;
	}
}

/**
 * @brief Link a position.
 *
 * Find moves that lead to other positions in the book.
 *
 * @param position Position to link.
 * @param book Opening book.
 */
static void position_link(Position *position, Book *book)
{
	int x;
	unsigned long long moves = get_moves(position->board->player, position->board->opponent);
	Board next[1];
	Link link[1];
	Position *child;

	if (moves) {
		foreach_bit(x, moves) {
			board_next(position->board, x, next);
			child = book_probe(book, next);
			if (child) {
				link->score = -child->score.value;
				link->move = x;
				book->stats.n_links += position_add_link(position, link);
			}
		}
	} else if (can_move(position->board->opponent, position->board->player)) {// pass ?
		next->player = position->board->opponent;
		next->opponent = position->board->player;
		child = book_probe(book, next);
		if (child) {
			link->score = -child->score.value;
			link->move = PASS;
			book->stats.n_links += position_add_link(position, link);
		}
	}
}

/**
 * @brief Expand a position.
 *
 * Expand the best yet unlink move. This will add a new position to the book.
 * Two new moves will also be analyzed, one for the new position, the other for
 * the actual position as a new best unlink move.
 *
 * @param position Position to expand.
 * @param book Opening book.
 */
static void position_expand(Position *position, Book *book)
{
	Position child[1];

	if (position->leaf.move != NOMOVE) {
		position_init(child);

		board_next(position->board, position->leaf.move, child->board);

		child->level = position->level;
		position_link(child, book);
		search_cleanup(book->search);
		position_search(child, book);
		position->leaf.score = -child->score.value;
		position_search(position, book);
		position_unique(child);
		book_add(book, child);
	}
}

/**
 * @brief Negamax a position.
 *
 * Go through the book sub-tree following the current position & negamax the best scores back to this position.
 *
 * @param position Position to expand.
 * @param book Opening book.
 */
static int position_negamax(Position *position, Book *book)
{
	Link *l;
	Board target[1];
	Position *child;

	if (!position->done) {
		GameStats stat = {0,0,0,0};
		const int n_empties = board_count_empties(position->board);
		const int search_depth = LEVEL[position->level][n_empties].depth;
		const int bias = (search_depth & 1) - (n_empties & 1);

		position->done = 1;

		position->score.value = position->score.lower = position->score.upper = -SCORE_INF;

		if (position->leaf.score > -SCORE_INF) {
			position->score.value = position->leaf.score;
			// is solving
			if (search_depth == n_empties && LEVEL[position->level][n_empties].selectivity == NO_SELECTIVITY) {
				position->score.lower = position->score.upper = position->score.value;
				if (position->leaf.score > 0) ++stat.n_wins;
				else if (position->leaf.score < 0) ++stat.n_losses;
				else ++stat.n_draws;
			// is pre-solving
			} else if (search_depth == n_empties) {
				position->score.lower = position->score.value - book->options.endcut_error;
				position->score.upper = position->score.value + book->options.endcut_error;
			} else { // midgame
				position->score.lower = position->score.value - book->options.midgame_error - bias;
				position->score.upper = position->score.value + book->options.midgame_error - bias;
			}
			++stat.n_lines;
		}

		foreach_link(l, position) {
			board_next(position->board, l->move, target);
			child = book_probe(book, target);
			position_negamax(child, book);
			if (l->score != -child->score.value) {
				l->score = -child->score.value;
				book->need_saving = true;
			}
			if (l->score > position->score.value) position->score.value = l->score;
			if (-child->score.upper > position->score.lower) position->score.lower = -child->score.upper;
			if (-child->score.lower > position->score.upper) position->score.upper = -child->score.lower;

			stat.n_wins += child->n_losses;
			stat.n_draws += child->n_draws;
			stat.n_losses += child->n_wins;
			stat.n_lines += child->n_lines;
		}

		position->n_wins = (unsigned int) MIN(UINT_MAX, stat.n_wins);
		position->n_draws = (unsigned int) MIN(UINT_MAX, stat.n_draws);
		position->n_losses = (unsigned int) MIN(UINT_MAX, stat.n_losses);
		position->n_lines = (unsigned int) MIN(UINT_MAX, stat.n_lines);
	}

	return position->score.value;
}


/**
 * @brief Prune a position.
 *
 * @param position Position to prune.
 * @param book Opening book.
 * @param player_deviation Player's error.
 * @param opponent_deviation Opponent's error.
 * @param lower Error lower bound.
 * @param upper Error upper bound.
 */
static void position_prune(Position *position, Book *book, const int player_deviation, const int opponent_deviation, const int lower, const int upper)
{
	Link *l;
	Board target[1];
	Position *child;

	// if position is not done yet & good enough & inside the book height limit
	if (lower <= position->score.value && position->score.value <= upper && board_count_empties(position->board) >= book->options.n_empties - 1) {
		position->done = true; book->stats.n_todo++;

		// prune all children close to the best move
		foreach_link(l, position) {
			if (position->score.value - l->score <= player_deviation && lower <= l->score && l->score <= upper) {
				board_next(position->board, l->move, target);
				child = book_probe(book, target);
				position_prune(child, book, opponent_deviation, player_deviation, -upper, -lower);
			}
		}
		if (book->stats.n_todo % BOOK_INFO_RESOLUTION == 0) {
			bprint("Book prune %d to keep\r", book->stats.n_todo);
			
		}
	}
}

/**
 * @brief Remove bad links after book pruning.
 *
 * @param position Position to fix.
 * @param book Opening book.
 */
static void position_remove_links(Position *position, Book *book)
{
	int i, j;
	Link *l = position->link;
	Board target[1];

	for (i = 0; i < position->n_link; ++i) {
		board_next(position->board, l[i].move, target);
		if (!book_probe(book, target)) {
			if (l[i].score > position->leaf.score) position->leaf = l[i];
			for (j = i + 1; j < position->n_link; ++j) l[j - 1] = l[j];
			--position->n_link;
			--i;
		}
	}
}

/**
 * @brief Deviate a position.
 *
 * This is the important part of the opening book code, where it finds the best moves to add to the book.
 * Considering the current position, it will deviate a child position or expand a move if:
 * - move_score < best_score - player_deviation
 * - lower <= move_score <= upper.
 * So a good candidate move for expansion need to have a score close to the best move, and not too far
 * from the root score from where we deviates, in order to not derive to very bad positions.
 *
 * @param position Position to expand.
 * @param book Opening book.
 * @param player_deviation Player's error.
 * @param opponent_deviation Opponent's error.
 * @param lower Error lower bound.
 * @param upper Error upper bound.
 */
static void position_deviate(Position *position, Book *book, const int player_deviation, const int opponent_deviation, const int lower, const int upper)
{
	Link *l;
	Board target[1];
	Position *child;

	// if position is not done yet & good enough & inside the book height limit
	if (!position->done && lower <= position->score.value && position->score.value <= upper && board_count_empties(position->board) >= book->options.n_empties && !board_is_game_over(position->board)) {
		position->done = true;

		// deviate all children close to the best move
		foreach_link(l, position) {
			if (position->score.value - l->score <= player_deviation && lower <= l->score && l->score <= upper) {
				board_next(position->board, l->move, target);
				child = book_probe(book, target);
				position_deviate(child, book, opponent_deviation, player_deviation, -upper, -lower);
			}
		}

		// expand the best remaining move
		if (position->score.value - position->leaf.score <= player_deviation && lower <= position->leaf.score && position->leaf.score <= upper) {
			position->todo = true; book->stats.n_todo++;
			if (book->stats.n_todo % 10 == 0) bprint("Book deviate %d todo\r", book->stats.n_todo);
		}
	}
}

/**
 * @brief Enhance a position.
 *
 * This is the other important part of the opening book code, where it finds the best moves to add to the book by
 * another methods.
 * Here we use score negamaxed bounds to decide if a move is a good candidate for further enhancement or for expansion.
 * The purpose of this algorithm is to find more moves.
 *
 *
 * @param position Position to expand.
 * @param book Opening book.
 */
static void position_enhance(Position *position, Book *book)
{
	Link *l;
	Board target[1];
	Position *child;

	if (!position->done && board_count_empties(position->board) >= book->options.n_empties && !board_is_game_over(position->board)) {
		position->done = true;

		foreach_link(l, position) {
			board_next(position->board, l->move, target);
			child = book_probe(book, target);
			if (-child->score.upper >= position->score.lower || -child->score.lower >= position->score.upper) {
				position_enhance(child, book);
			}
		}

		if (position->leaf.score > -SCORE_INF) {
			const int n_empties = board_count_empties(position->board);
			const int search_depth = LEVEL[position->level][n_empties].depth;
			const int bias = (search_depth & 1) - (n_empties & 1);
			int lower, upper;
			// is solving
			if (search_depth == n_empties && LEVEL[position->level][n_empties].selectivity == NO_SELECTIVITY) {
				lower = upper = position->leaf.score;
			// is pre-solving
			} else if (search_depth == n_empties) {
				lower = position->leaf.score - book->options.endcut_error;
				upper = position->leaf.score + book->options.endcut_error;
			} else { // midgame
				lower = position->leaf.score - book->options.midgame_error - bias;
				upper = position->leaf.score + book->options.midgame_error - bias;
			}

			if (lower >= position->score.lower || upper >= position->score.upper) {
				position->todo = true;
			}
		}
	}
}

/**
 * @brief Feed hash from a position.
 *
 * Go through the book sub-tree following the current position & feed the hash table from this position.
 *
 * @param board Position to expand.
 * @param book Opening book.
 * @param search Hashtables container.
 * @param is_pv Flag to tell if the position is from the principal variation.
 */
static void board_feed_hash(Board *board, const Book *book, Search *search, const bool is_pv)
{
	Position *position;
	const unsigned long long hash_code = board_get_hash_code(board);
	MoveList movelist[1];
	Move *m;

	position = book_probe(book, board);
	if (position) {
		const int n_empties = board_count_empties(position->board);
		const int depth = LEVEL[position->level][n_empties].depth;
		const int selectivity = LEVEL[position->level][n_empties].selectivity;
		const int score = position->score.value;
		int move = NOMOVE;

		position_get_moves(position, board, movelist);
		foreach_move(m, movelist) {
			if (move == NOMOVE) move = m->x;
			board_update(board, m);
				board_feed_hash(board, book, search, is_pv && m->score == score);
			board_restore(board, m);
		}
		hash_feed(search->hash_table, board, hash_code, depth, selectivity, score, score, move);
		if (is_pv) hash_feed(search->pv_table, board, hash_code, depth, selectivity, score, score, move);
	}
}

/**
 * @brief Fill the opening book.
 *
 * Add positions to link existing positions.
 *
 * @param board Candidate position.
 * @param book Opening book.
 * @param depth Depth at which to search a link.
 * @return true if the board is in the book, possibly just after having been added to it.
 */
static bool board_fill(Board *board, Book *book, int depth)
{
	if (depth > 0) {
		MoveList movelist[1];
		Move *m;
		bool filled = false;

		movelist_get_moves(movelist, board);
		if (movelist->n_moves == 0 && can_move(board->opponent, board->player)) {
			board_pass(board);
			if (board_fill(board, book, depth - 1)) {
				book_add_board(book, board);
				filled = true;
			}
			board_pass(board);							
		} else {
			foreach_move(m, movelist) {
				board_update(board, m);
				if (board_fill(board, book, depth - 1)) {
					book_add_board(book, board);
					filled = true;
				}
				board_restore(board, m);					
			}
		}
		return filled;
	}
	return book_probe(book, board) != NULL;
}

/**
 * @brief Fix a position.
 *
 * Recompute all elements of a buggy position
 *
 * @param position Position to fix.
 * @param book Opening book.
 */
static void position_fix(Position *position, Book *book)
{
	Board board[1];

	if ((position->board->player & position->board->opponent) || 
	    ((position->board->player | position->board->opponent) & 0x0000001818000000ULL) != 0x0000001818000000ULL) {
		position_free(position);
		position_init(position);
		return;
	}
	board_unique(position->board, board);
	position_free(position);
	position_init(position);
	*position->board = *board;
	position->level = book->options.level;
	position_link(position, book);
	position_search(position, book);
}

/**
 * @brief An array with positions.
 */
typedef struct PositionArray {
	Position *positions;
	int n;
	int size;
} PositionArray;


/**
 * @brief Initialize the array.
 *
 * @param a Positions' array.
 */
static void position_array_init(PositionArray *a)
{
	a->size = a->n = 0;
	a->positions = NULL;
}

/**
 * @brief Add a position to the array.
 *
 * @param a Positions' array.
 * @param p Position to add.
 * @return true in case of success.
 */
static bool position_array_add(PositionArray *a, const Position *p)
{
	int i;

	board_check(p->board);
	assert(position_is_ok(p));

	for (i = 0; i < a->n; ++i) if (board_equal(a->positions[i].board, p->board)) return false;
	if (a->n == a->size) {
		Position *n;
		a->size += a->size / 2 + 1;
		n = (Position*) realloc(a->positions, a->size * sizeof (Position));
		if (n == NULL) {
			error("cannot add a position to the book\n");
			a->size = a->n;
			return false;
		}
		a->positions = n;
	}
	a->positions[a->n] = *p;
	a->positions[a->n].done = true;
	a->positions[a->n].todo = false;
	++a->n;
	return true;
}

/**
 * @brief Remove a position from an array.
 *
 * @param a Positions' array.
 * @param p Position to add.
 * @return true in case of success.
 */
static bool position_array_remove(PositionArray *a, const Position *p)
{
	int i, j;

	for (i = 0; i < a->n; ++i) {
		if (board_equal(a->positions[i].board, p->board)) {
			position_free(a->positions + i);
			for (j = i + 1; j < a->n; ++j) {
				a->positions[j - 1] = a->positions[j];
			}
			--a->n;
			return true;
		}
	}
	return false;
}

/**
 * @brief Free resources used by a position array.
 *
 * @param a Positions' array.
 */
static void position_array_free(PositionArray *a)
{
	int i;
	for (i = 0; i < a->n; ++i) position_free(a->positions + i);
	free(a->positions);
}

/**
 * @brief Find a position in the array.
 *
 * @param a Positions' array.
 * @param board Board to find in the array.
 * @return a position containg the board (or a symetry) or NULL is no position is found.
 */
static Position* position_array_probe(PositionArray *a, const Board *board)
{
	int i;
	for (i = 0; i < a->n; ++i) if (board_equal(a->positions[i].board, board)) return a->positions + i;
	return NULL;
}

#define foreach_position(p, a, b) \
	for (a = b->array; a < b->array + b->n; ++a) \
	for (p = a->positions; p < a->positions + a->n; ++p)

/**
 * @brief Set book date.
 *
 * @param book Opening book.
 */
static void book_set_date(Book *book)
{
	time_t t = time(NULL);
	struct tm *tm = localtime(&t);

	book->date.year = tm->tm_year + 1900;
	book->date.month = tm->tm_mon + 1;
	book->date.day = tm->tm_mday;
	book->date.hour = tm->tm_hour;
	book->date.minute = tm->tm_min;
	book->date.second = tm->tm_sec;
}

/**
 * @brief Get book age, in seconds.
 *
 * @param book Opening book.
 * @return book age.
 */
static double book_get_age(Book *book)
{
	struct tm tm;
	double t;

	tm.tm_year = book->date.year - 1900;
	tm.tm_mon = book->date.month - 1;
	tm.tm_mday = book->date.day;
	tm.tm_hour = book->date.hour;
	tm.tm_min = book->date.minute;
	tm.tm_sec = book->date.second;
	tm.tm_isdst = -1;

	t = difftime(time(NULL), mktime(&tm));

	return t;
}



/**
 * @brief Find a position in the book.
 *
 * @param book Opening book.
 * @param board Board to find in the array.
 * @return a position containg the board (or a symetry) or NULL is no position is found.
 */
static Position* book_probe(const Book *book, const Board *board)
{
	Board unique[1];
	board_unique(board, unique);
	return position_array_probe(book->array + (board_get_hash_code(unique) & (book->n - 1)), unique);
}

/**
 * @brief Add a position to the book.
 *
 * @param book Opening book.
 * @param p Position to add.
 */
static void book_add(Book *book, const Position *p)
{
	const unsigned long long i = board_get_hash_code(p->board) & (book->n - 1);

	if (position_array_add(book->array + i, p)) {
		++book->n_nodes;
		++book->stats.n_nodes;
	}
}

/**
 * @brief Remove a position from the book.
 *
 * @param book Opening book.
 * @param p Position to add.
 */
static void book_remove(Book *book, const Position *p)
{
	const unsigned long long i = board_get_hash_code(p->board) & (book->n - 1);

	if (position_array_remove(book->array + i, p)) {
		--book->n_nodes;
		--book->stats.n_nodes;
	}
}

/**
 * @brief Set all positions as undone.
 *
 * @param book Opening book.
 */
static void book_clean(Book *book)
{
	PositionArray *a;
	Position *p;
	book->stats.n_nodes = book->stats.n_links = book->stats.n_todo = 0;
	foreach_position(p, a, book) p->done = p->todo = false;
}

/**
 * @brief Find the initial position in the book.
 *
 * Attention: when a position is added to the book, the pointer
 * returned by this position may be wrong. If the root position is updated
 * the contents of the pointed structure may be wrong. So it is needed to
 * recall this function each time as necessary.
 *
 * @param book Opening book.
 * @return the inital position.
 */
static Position *book_root(Book *book)
{
	Board board[1];

	board_init(board);
	return book_probe(book, board);
}

/**
 * @brief Initialize the opening book.
 *
 * Create an empty opening book.
 *
 * @param book Opening book.
 */
void book_init(Book *book)
{
	int i;

	book_set_date(book);

	book->options.level = 21;
	book->options.n_empties = 24;
	book->options.midgame_error = 2;
	book->options.endcut_error = 1;

	book->n = 65536;
	book->array = (PositionArray*) malloc(book->n * sizeof *book->array);
	if (book->array == NULL) fatal_error("cannot allocate space to store the positions");
	for (i = 0; i < book->n; ++i) position_array_init(book->array + i);

	book->n_nodes = 0;
	random_seed(book->random, real_clock());
	book->need_saving = false;
}

/**
 * @brief Free resources used by the opening book.
 *
 * @param book Opening book.
 */
void book_free(Book *book)
{
	int i;
	for (i = 0; i < book->n; ++i) {
		position_array_free(book->array + i);
	}
	free(book->array);
}

/**
 * @brief clean opening book.
 *
 * @param book Opening book.
 */
void book_preprocess(Book *book)
{
    book_clean(book);
}

/**
 * @brief Create a new opening book.
 *
 * Create an opening book with the initial position & a single non link move.
 *
 * @param book Opening book.
 * @param level search level to evaluate positions.
 * @param n_empties number of empty positions up to which to evaluate positions.
 */
void book_new(Book *book, int level, int n_empties)
{
	Board board[1];

	bprint("New book %d %d...", level, n_empties);
	book_init(book);
	book->options.level = level;
	book->options.n_empties = n_empties;

	board_init(board);
	book_add_board(book, board);
	bprint("...done>\n");
	book->need_saving = true;
}

/**
 * @brief Load the opening book.
 *
 * @param book Opening book.
 * @param file File name.
 */
void book_load(Book *book, const char *file)
{
	FILE *f = fopen(file, "rb");
	if (f) {
		Position p;
		unsigned int header_edax, header_book;
		unsigned char header_version, header_release;
		int i;
		int r;

		info("Loading book from %s...", file);
		r = fread(&header_edax, sizeof (unsigned int), 1, f);
		r += fread(&header_book, sizeof (unsigned int), 1, f);
		if (r != 2 || header_edax != EDAX || header_book != BOOK) {
			error("%s is not an edax opening book", file);
			book_new(book, options.level, 61 - get_book_depth(options.level));
			return;
		}

		r = fread(&header_version, 1, 1, f);
		r += fread(&header_release, 1, 1, f);
		if (r != 2 || header_version != VERSION) {
			error("%s is not a compatible version", file);
			book_new(book, options.level, 61 - get_book_depth(options.level));
			return;
		}

		r = fread(&book->date, sizeof book->date, 1, f);
		r += fread(&book->options, sizeof book->options, 1, f);
		r += fread(&book->n_nodes, sizeof book->n_nodes, 1, f);
		if (r != 3) {
			error("Cannot read book settings from %s", file);
			book_new(book, options.level, 61 - get_book_depth(options.level));
			return;
		}

		book->n = 65536;
		while ((book->n << 4) < book->n_nodes) book->n <<= 1;

		book->array = (PositionArray*) malloc(book->n * sizeof (PositionArray));
		if (book->array == NULL) {
			error("cannot allocate space to store the positions");
			book_new(book, options.level, 61 - get_book_depth(options.level));
			return;
		}
		for (i = 0; i < book->n; ++i) position_array_init(book->array + i);

		book->n_nodes = 0;
		while (position_read(&p, f)) {
			book_add(book, &p);
		}

		if (!feof(f)) {
			error("error while reading %s", file);
		}

		random_seed(book->random, real_clock());
		book->need_saving = false;

		info("done\n");
		fclose(f);
	} else {
		book_new(book, options.level, 60 - get_book_depth(options.level));
	}
}

/**
 * @brief Import an opening book.
 *
 * Read the opening book from a portable text format.
 * After the book is imported, it is needed to
 * relink & negamax it.
 *
 * @param book Opening book.
 * @param file File name.
 */
void book_import(Book *book, const char *file)
{
	FILE *f = fopen(file, "r");
	if (f) {
		PositionArray *a;
		Position *p, position;
		int n_empties;

		book_init(book);
		while (position_import(&position, f)) {
			book_add(book, &position);
			if (book->n_nodes % BOOK_INFO_RESOLUTION == 0) bprint("importing book from %s... %d positions\r", file, book->n_nodes);
		}
		bprint("importing book from %s... %d positions", file, book->n_nodes);

		book->options.n_empties = 60;
		book->options.level = 0;
		foreach_position(p, a, book) {
			n_empties = board_count_empties(p->board);
			if (p->level > book->options.level) book->options.level = p->level;
			if (n_empties < book->options.n_empties) book->options.n_empties = n_empties;
		}

		random_seed(book->random, real_clock());
		book->need_saving = true;

		bprint("...done\n");
		fclose(f);
	} else {
		error("cannot open \"%s\" to import the opening book\n", file);
		book_new(book, options.level, 61 - get_book_depth(options.level));
	}
}

/**
 * @brief Export an opening book.
 *
 * Save the book in a portable text format.
 *
 * @param book Opening book.
 * @param file File name.
 */
void book_export(Book *book, const char *file)
{
	FILE *f;
	PositionArray *a;
	Position *p;

	f = fopen(file, "w");
	if (f == NULL) {
		error("cannot open file %s", file);
		return;
	}
	
	info("Exporting book to %s...", file);
	foreach_position(p, a, book) {
		if (!position_export(p, f)) {
			error("cannot export book to %s", file);
			goto book_export_end;
		}
	}
	info("done\n");

book_export_end:
	fclose(f);
}

/**
 * @brief Save an opening book.
 *
 * Save the book in a fast binary format.
 *
 * @param book Opening book.
 * @param file File name.
 */
void book_save(Book *book, const char *file)
{
	unsigned int header_edax = EDAX, header_book = BOOK;
	unsigned char header_version = VERSION, header_release = RELEASE;
	FILE *f = fopen(file, "wb");
	int r;
	PositionArray *a;
	Position *p;

	info("Saving book to %s...", file);
	book_set_date(book);

	r = fwrite(&header_edax, sizeof (unsigned int), 1, f);
	r += fwrite(&header_book, sizeof (unsigned int), 1, f);
	r += fwrite(&header_version, 1, 1, f);
	r += fwrite(&header_release, 1, 1, f);
	r += fwrite(&book->date, sizeof book->date, 1, f);
	r += fwrite(&book->options, sizeof book->options, 1, f);
	r += fwrite(&book->n_nodes, sizeof book->n_nodes, 1, f);

	if (r == 7) {
		foreach_position(p, a, book) {
			if (!position_write(p, f)) {
				error("\nCannot save book to %s", file);
				goto book_write_end;
			}
		}
	}
	info("done\n");

book_write_end:
	fclose(f);
}

/**
 * @brief Merge two opening books.
 *
 * It is needed to relink & negamax the destination book
 * after merging.
 *
 * @param dest Destination opening book.
 * @param src Source opening book.
 */
void book_merge(Book *dest, const Book *src)
{
	PositionArray *a;
	const Position *p_src;
	Position p_dest[1];

	foreach_position(p_src, a, src) {
		if (!book_probe(dest, p_src->board)) {
			position_merge(p_dest, p_src);
			book_add(dest, p_dest);
		}
	}
}

/**
 * @brief Negamax a book.
 *
 * @param book opening book.
 */
void book_negamax(Book *book)
{
	Position *root = book_root(book);

	if (root) {
		bprint("Negamaxing book...");
		book_clean(book);
		position_negamax(root, book);
		bprint("done\n");
	}
}

/**
 * @brief Link a book.
 *
 * @param book opening book.
 */
void book_link(Book *book)
{
	PositionArray *a;
	Position *p;
	int i = 0;

	bprint("Linking book...\r");
	foreach_position(p, a, book) {
		position_link(p, book);
		if (p->leaf.move == NOMOVE) {
			position_search(p, book);
		}
		if (++i % BOOK_INFO_RESOLUTION == 0) bprint("Linking book...%d\r", i);
	}
	bprint("Linking book...%d done\n", i);
}

/**
 * @brief Fix a book.
 *
 * @param book opening book.
 */
void book_fix(Book *book)
{
	PositionArray *a;
	Position *p;
	int i = 0;

	bprint("Fixing book...\r"); 
	foreach_position(p, a, book) {
		if (!position_is_ok(p)) {
			position_fix(p, book);
			if (++i % BOOK_INFO_RESOLUTION == 0) { bprint("fixing book...%d\r", i);  }
		}
	}
	bprint("Fixing book...%d done\n", i);
}

/**
 * @brief Deepen a book.
 *
 * Research all non link best move.
 *
 * @param book opening book.
 */
void book_deepen(Book *book)
{
	PositionArray *a;
	Position *p;
	int i = 0;
	unsigned long long t = real_clock();
	char file[FILENAME_MAX + 1];
	
	file_add_ext(options.book_file, ".dep", file);

	bprint("Deepening book...\r"); 
	foreach_position(p, a, book) {
		int n_empties = board_count_empties(p->board);
		if (LEVEL[p->level][n_empties].depth != LEVEL[book->options.level][n_empties].depth
		 || LEVEL[p->level][n_empties].selectivity != LEVEL[book->options.level][n_empties].selectivity) { // No! compare depth & selectivity;
			p->leaf = BAD_LINK;
			position_search(p, book);
			if (++i % 10 == 0) {
				bprint("Deepening book...%d\r", i); 
			}
			if (real_clock() - t > HOUR) {
				book_save(book, file); // save every hour
				t = real_clock();
			}
		}
	}
	bprint("Deepening book...%d done\n", i);
}

/**
 * @brief Correct wrong solved score in the book.
 *
 * Correct erroneous solved positions. Edax may be unstable and introduce bugs from time to time...
 *
 * @param book opening book.
 */
void book_correct_solved(Book *book)
{
	PositionArray *a;
	Position *p;
	int i = 0;
	unsigned long long t = real_clock();
	char file[FILENAME_MAX + 1];
	Link old_leaf;
	int n_error = 0;
	char s[4];
	
	file_add_ext(options.book_file, ".err", file);

	bprint("Correcting solved positions...\r"); 
	foreach_position(p, a, book) {
		int n_empties = board_count_empties(p->board);
		if (LEVEL[p->level][n_empties].depth == n_empties && LEVEL[p->level][n_empties].selectivity == NO_SELECTIVITY) { // No! compare depth & selectivity;
			old_leaf = p->leaf;
			p->leaf = BAD_LINK;
			position_search(p, book);
			if (p->leaf.score != old_leaf.score) {
				++n_error;
				bprint("\nError found:\n");
				position_print(p, p->board, stdout);
				move_to_string(old_leaf.move, n_empties & 1, s);
				bprint("instead of <%s:%d>\n\n", s, old_leaf.score);
			}
			if (++i % 10 == 0 || p->leaf.score != old_leaf.score) {
				bprint("Correcting solved positions...%d (%d error found)\r", i, n_error); 
			}
			if (real_clock() - t > HOUR) {
				book_save(book, file); // save every hour
				t = real_clock();
			}
		}
	}
	bprint("Correcting solved positions...%d done (%d error found)\n", i, n_error);
}

/**
 * @brief Expand a book.
 *
 * Research all non link best move.
 *
 * @param book opening book.
 * @param action String with a description of current action.
 * @param tmp_file Temporary file name.
 */
static void book_expand(Book *book, const char *action, const char *tmp_file)
{
	PositionArray *a;
	Position *p;
	int i = 0, k;
	unsigned long long t = real_clock();

	bprint("%s...\r", action);
	
	for (a = book->array; a < book->array + book->n; ++a)
	for (k = 0; k < a->n; ++k) { // do not use foreach_positions here! a->positions may change!
		p = a->positions + k;
		if (p->todo) {
			position_expand(p, book);
			bprint("%s...%d/%d done: %d positions, %d links\r", action, ++i, book->stats.n_todo, book->stats.n_nodes, book->stats.n_links);
			if (book->search->options.verbosity >= 2) putchar('\n'); else putchar('\r');
			
			if (real_clock() - t > HOUR) {
				book_save(book, tmp_file); // save every hour
				t = real_clock();
			}
		}
	}
	bprint("%s...%d/%d done: %d positions, %d links\n", action, i, book->stats.n_todo, book->stats.n_nodes, book->stats.n_links);
}

/**
 * @brief Sort a book.
 *
 * @param book opening book.
 */
void book_sort(Book *book)
{
	PositionArray *a;
	Position *p;

	bprint("Sorting book...");
	foreach_position(p, a, book) {
		position_sort(p);
	}
	bprint("done>\n");
}

/**
 * @brief Play.
 *
 * Add positions to the opening book by adding links
 * to position with no links.
 *
 * @param book opening book.
 */
void book_play(Book *book)
{
	PositionArray *a;
	Position *p;
	int n_diffs;
	char file[FILENAME_MAX + 1];

	file_add_ext(options.book_file, ".play", file);
	do {
		n_diffs = 0;
		book->stats.n_nodes = book->stats.n_links = book->stats.n_todo = 0;
		foreach_position(p, a, book) {
			if (p->n_link == 0 && board_count_empties(p->board) >= book->options.n_empties && !board_is_game_over(p->board)) {
				p->todo = true; ++book->stats.n_todo;
			} else {
				p->todo = false;
			}
			if (book->stats.n_todo && book->stats.n_todo % BOOK_INFO_RESOLUTION == 0) bprint("Book play...%d todo\r", book->stats.n_todo);
		}
		bprint("Book play...%d todo\n", book->stats.n_todo);

		book_expand(book, "Book play", file);

		n_diffs = book->stats.n_nodes + book->stats.n_links;
		if (n_diffs) {
			book_negamax(book);
			book_save(book, file);
		}
	} while (n_diffs);
	bprint("Book play... finished\n");
}

/**
 * @brief Fill a book.
 *
 * @param book opening book.
 * @param depth Distance to fill between two positions.
 */
void book_fill(Book *book, const int depth)
{
	PositionArray *a;
	Position *p;
	int n_diffs, n_empties, k;
	char file[FILENAME_MAX + 1];

	file_add_ext(options.book_file, ".fill", file);

	do {
		n_diffs = 0;
		book->stats.n_nodes = book->stats.n_links = 0;
		for (a = book->array; a < book->array + book->n; ++a)
		for (k = 0; k < a->n; ++k) { // do not use foreach_positions here! a->positions may change!
			p = a->positions + k;
			n_empties = board_count_empties(p->board);
			if (n_empties >= book->options.n_empties) {
				board_fill(p->board, book, depth);
				if (n_diffs < book->stats.n_nodes + book->stats.n_links) {
					n_diffs = book->stats.n_nodes + book->stats.n_links;
					bprint("Book fill...%d %d done\r", book->stats.n_nodes, book->stats.n_links); 
				}
			}
		}
		bprint("Book fill...%d %d done\n", book->stats.n_nodes, book->stats.n_links);
		if (n_diffs) {
			book_negamax(book);
			book_save(book, file);
		}
	} while (n_diffs);
	bprint("Book fill... finished\n");
}

/**
 * @brief Deviate a book.
 *
 * @param book opening book.
 * @param board Position to start from.
 * @param relative_error Error relative to the current position's score.
 * @param absolute_error Error relative to the root position's score.
 */
void book_deviate(Book *book, Board *board, const int relative_error, const int absolute_error)
{
	Position *root = book_probe(book, board);
	if (root) {
		int score;
		int n_diffs;
		char file[FILENAME_MAX + 1];

		file_add_ext(options.book_file, ".dev", file);
		book_clean(book);
		position_negamax(root, book);

		do {
			score = root->score.value;

			bprint("Book deviate %d %d:\n", relative_error, absolute_error);
			book_clean(book);
			position_deviate(root, book, relative_error, 0, score - absolute_error, score + absolute_error);
			bprint("Book deviate %d todo\n", book->stats.n_todo);

			book_expand(book, "Book deviate", file);
			n_diffs = book->stats.n_nodes + book->stats.n_links;

			bprint("Book deviate %d %d:\n", relative_error, absolute_error);
			book_clean(book);
			position_deviate(root, book, 0, relative_error, score - absolute_error, score + absolute_error);
			bprint("Book deviate %d todo\n", book->stats.n_todo);

			book_expand(book, "Book deviate", file);
			n_diffs += book->stats.n_nodes + book->stats.n_links;

			root = book_probe(book, board);
			book_clean(book);
			position_negamax(root, book);
			if (n_diffs) book_save(book, file);
		} while (n_diffs);
		bprint("Book deviate %d %d...finished\n", relative_error, absolute_error);
	}
}

/**
 * @brief Prune a book.
 *
 * Remove positions Edax cannot reach.
 *
 * @param book opening book.
 */
void book_prune(Book *book)
{
	PositionArray *a;
	Position *p;
	Position *root = book_root(book);
	int i;

	if (root) {
		book_clean(book);
		position_negamax(root, book);

		book_clean(book);
		position_prune(root, book, 2*SCORE_INF, 0, -SCORE_INF, SCORE_INF);
		position_print(root, root->board, stdout);
		bprint("Book prune %d... done\n", book->stats.n_todo);

		position_prune(root, book, 0, 2*SCORE_INF, -SCORE_INF, SCORE_INF);
		bprint("Book prune %d... done\n", book->stats.n_todo);
		for (a = book->array; a < book->array + book->n; ++a)
		for (i = 0; i < a->n; ++i) if (!a->positions[i].done) {book_remove(book, a->positions + i); --i;}
		foreach_position(p, a, book) position_remove_links(p, book);
		bprint("done\n");
	}
}

/**
 * @brief Prune a book.
 *
 * Remove positions Edax cannot reach.
 *
 * @param book opening book.
 */
void book_subtree(Book *book, const Board *board)
{
	PositionArray *a;
	Position *p;
	Position *root = book_probe(book, board);
	int i;

	if (root) {
		book_clean(book);
		position_negamax(root, book);

		book_clean(book);
		position_prune(root, book, 2*SCORE_INF, 2*SCORE_INF, -SCORE_INF, SCORE_INF);
		position_print(root, root->board, stdout);
		bprint("Book subtree %d... done\n", book->stats.n_todo);
		for (a = book->array; a < book->array + book->n; ++a)
		for (i = 0; i < a->n; ++i) if (!a->positions[i].done) {book_remove(book, a->positions + i); --i;}
		foreach_position(p, a, book) position_remove_links(p, book);
		bprint("done\n");
	}
}


/**
 * @brief Enhance a book.
 *
 * @param book opening book.
 * @param board Position to start from.
 * @param midgame_error Error in midgame search.
 * @param endcut_error Error in endgame search.
 */
void book_enhance(Book *book, Board *board, const int midgame_error, const int endcut_error)
{
	Position *root = book_probe(book, board);
	if (root) {
		int n_diffs;
		char file[FILENAME_MAX + 1];

		file_add_ext(options.book_file, ".enh", file);

		book->options.midgame_error = midgame_error;
		book->options.endcut_error = endcut_error;

		book_clean(book);
		position_negamax(root, book);

		do {
			bprint("Book enhance %d %d...%d %d:\n", midgame_error, endcut_error, book->stats.n_nodes, book->stats.n_links);
			book_clean(book);
			position_enhance(root, book);
			n_diffs = book->stats.n_nodes + book->stats.n_links;
			book_expand(book, "Book enhance", file);

			root = book_probe(book, board);
			book_clean(book);
			position_negamax(root, book);
			if (n_diffs) book_save(book, file);
		} while (n_diffs);
		bprint("Book enhance %d %d...finished\n", midgame_error, endcut_error);
	}
}

/**
 * @brief display some book's informations.
 *
 * @param book opening book.
 */
void book_info(Book *book)
{
	PositionArray *a;
	Position *p;
	unsigned long long n_links = 0;
	unsigned long long n_leaves = 0;
	unsigned long long n_level[61] = {0};
	int min_array = book->n_nodes, max_array = 0;
	int i;

	foreach_position(p, a, book) {
		n_links += p->n_link;
		if (p->leaf.move != NOMOVE) ++n_leaves;
		++n_level[p->level];
		if (p->level != book->options.level) {
			position_print(p, p->board, stdout);
		}
	}

	for (a = book->array; a < book->array + book->n; ++a) {
		if (a->n > max_array) max_array = a->n;
		if (a->n < min_array) min_array = a->n;
	}

	bprint("Edax Book %d.%d; ", VERSION, RELEASE);
	bprint("%d-%d-%d ", book->date.year, book->date.month, book->date.day);
	bprint("%d:%02d:%02d;\n", book->date.hour, book->date.minute, book->date.second);
	bprint("Positions: %d (moves = %lld links + %lld leaves);\n", book->n_nodes, n_links, n_leaves);
	for (i = 0; i < 61; ++i) {
		if (n_level[i]) {
			bprint("Level %d : %lld nodes\n", i, n_level[i]);
		}
	}
	bprint("Depth: %d\n", 61 - book->options.n_empties);
	bprint("Memory occupation: %lld\n", (long long) (book->n_nodes * sizeof (Position) + book->n * sizeof (PositionArray) + n_links * sizeof (Link)));
	bprint("Hash balance: %d < %d < %d\n", min_array, (int) (book->n_nodes / book->n), max_array);
}

/**
 * @brief Display a position from the book.
 *
 * @param book opening book.
 * @param board position to display.
 */
void book_show(Book *book, Board *board)
{
	GameStats stat = {0,0,0,0};
	Position *position = book_probe(book, board);
	unsigned long long n_games;

	if (position) {
		position_show(position, board, stdout);
		book_get_game_stats(book, board, &stat);
		n_games = stat.n_wins + stat.n_draws + stat.n_losses;
		if (n_games) {
			bprint("\nLines: %lld full games", n_games);
			bprint(" with %.2f%% win, %.2f%% draw, %.2f%% loss",
				100.0 * stat.n_wins / n_games, 100.0 * stat.n_draws / n_games, 100.0 * stat.n_losses / n_games);
		}
		bprint("\n       %lld incomplete lines.\n\n", stat.n_lines - n_games);
	}
}

// add for libedax by lavox. 2018/5/20
/**
 * @brief Display a position from the book.
 *
 * @param book opening book.
 * @param board position to display.
 */
Position* book_show_for_api(Book *book, Board *board)
{
    return book_probe(book, board);
}

/**
 * @brief Get a list of moves from the book.
 *
 * @param book Opening book.
 * @param board Position to display.
 * @param movelist List of moves.
 */
bool book_get_moves(Book *book, const Board *board, MoveList *movelist)
{
	Position *position = book_probe(book, board);
	if (position) {
		position_get_moves(position, board, movelist);
		return true;
	}

	return false;
}

/**
 * @brief Get a list of moves from the book.
 *
 * @param book Opening book.
 * @param board Position to display.
 * @param movelist List of moves.
 * @param position position.
 */
bool book_get_moves_with_position(Book *book, const Board *board, MoveList *movelist, Position *position)
{
    Position *p = book_probe(book, board);
    if (p) {
        position_get_moves(p, board, movelist);
        memcpy(position, p, sizeof(Position));
        return true;
    }

    return false;
}

/**
 * @brief Get a variation from the book.
 *
 * @param book Opening book.
 * @param board Position.
 * @param move First move;
 * @param line Bariation.
 */
void book_get_line(Book *book, const Board *board, const Move *move, Line *line)
{
	Position *position;
	Board b[1];
	Move m[1];

	line_push(line, move->x);
	board_next(board, move->x, b);

	while ((position = book_probe(book, b)) != NULL && !board_is_game_over(position->board)) {
		position_get_random_move(position, b, m, book->random, 0);
		line_push(line, m->x);
		board_update(b, m);
	}
}


/**
 * @brief Get a move at random from the opening book.
 *
 * @param book Opening book.
 * @param board Position to find a move from.
 * @param move Chosen move.
 * @param randomness Randomness.
 */
bool book_get_random_move(Book *book, const Board *board, Move *move, const int randomness)
{
	Position *position = book_probe(book, board);
	if (position) {
		position_get_random_move(position, board, move, book->random, randomness);
		return true;
	}

	return false;
}

/**
 * @brief Get game statistics from a position.
 *
 * @param book Opening book.
 * @param board Position to find a move from.
 * @param stat Game statistics output.
 */
void book_get_game_stats(Book *book, const Board *board, GameStats *stat)
{
	Position *position;

	assert(book != NULL);
	assert(board !=NULL);
	assert(stat != NULL);
	
	stat->n_wins = stat->n_losses = stat->n_draws = stat->n_lines = 0;

	position = book_probe(book, board);
	if (position) {
		if (position->n_wins == UINT_MAX || position->n_losses == UINT_MAX || position->n_draws == UINT_MAX || position->n_lines == UINT_MAX) {
			Board target[1];
			Link *l;
			GameStats child;
			
			foreach_link(l, position) {
				board_next(position->board, l->move, target);
				book_get_game_stats(book, target, &child);
				stat->n_wins += child.n_losses;
				stat->n_draws += child.n_draws;
				stat->n_losses += child.n_wins;
				stat->n_lines += child.n_lines;
			}
		} else {
			stat->n_wins = position->n_wins;
			stat->n_draws = position->n_draws;
			stat->n_losses = position->n_losses;
			stat->n_lines = position->n_lines;
		}
	}
}


/**
 * @brief Add a position.
 *
 * @param book opening book.
 * @param board position to add.
 */
void book_add_board(Book *book, const Board *board)
{
	Position position[1];
	Position *probe;

	if (board_count_empties(board) >= book->options.n_empties - 1) {
		probe = book_probe(book, board);
		if (probe) {
			position_link(probe, book);
			if (probe->leaf.move == NOMOVE) position_search(probe, book);
			if (BOOK_DEBUG) {printf("update: "); position_print(probe, board, stdout);}
		} else {
			position_init(position);
			*position->board = *board;
			position->level = book->options.level;
			position_link(position, book);
			position_search(position, book);
			if (BOOK_DEBUG) {printf("new: "); position_print(position, board, stdout);}
			position_unique(position);
			book_add(book, position);
		}
	}
}

/**
 * @brief Add positions from a game.
 *
 * @param book opening book.
 * @param game game to add.
 */
void book_add_game(Book *book, const Game *game)
{
	Board board[1];
	Move stack[99];
	int i, n_moves;
	char file[FILENAME_MAX + 1];
	const int n_stats = book->stats.n_nodes + book->stats.n_links;

	file_add_ext(options.book_file, ".gam", file);
	
	board_init(board);
	if (!board_equal(board, game->initial_board)) return; // skip non standard game
	for (i = n_moves = 0; i < 60 - book->options.n_empties && game->move[i] != NOMOVE; ++i) {
		if (!can_move(board->player, board->opponent)) {
			stack[n_moves++] = MOVE_PASS;
			board_pass(board);
		}
		if (!board_is_occupied(board, game->move[i]) && board_get_move(board, game->move[i], &stack[n_moves])) {
			board_update(board, stack + n_moves);
			++n_moves;
		} else {
			warn("illegal move in game");
			break; // stop, illegal moves
		}
	}

	search_cleanup(book->search);
	while (--n_moves >= 0) {
		book_add_board(book, board);
		board_restore(board, stack + n_moves);
	}

	if (book->stats.n_nodes + book->stats.n_links > n_stats && book_get_age(book) > 3600) book_save(book, file);
}

/**
 * @brief Add positions from a game database.
 *
 * @param book opening book.
 * @param base games to add.
 */
void book_add_base(Book *book, const Base *base)
{
	int i;
	char file[FILENAME_MAX + 1];
	long long t0, t;

	file_add_ext(options.book_file, ".gam", file);

	book_clean(book);
	bprint("Adding %d games to book...\n", base->n_games);
	t0 = real_clock();
	for (i = 0; i < base->n_games; ++i) {
		book_add_game(book, base->game + i);
		t = real_clock();
		if (t - t0 > 1000) {
		    bprint("Adding games...%d/%d done: %d positions, %d links\r", i + 1, base->n_games, book->stats.n_nodes, book->stats.n_links);
			t0 = t;
		}
		if (book->search->options.verbosity) putchar('\n');
		
	}
	bprint("Adding games...%d/%d done: %d positions, %d links\n", i, base->n_games, book->stats.n_nodes, book->stats.n_links);
	bprint("%d games added to book\n", i);

	book_save(book, file);
}

typedef struct BookCheckGame {
	unsigned long long missing;
	unsigned long long good;
	unsigned long long bad;
} BookCheckGame;

/**
 * @brief Check positions from a game.
 *
 * @param book opening book.
 * @param hash Board + Move hash table.
 * @param game game to check.
 * @param stat Count statistics.
 */
void book_check_game(Book *book, MoveHash *hash, const Game *game, BookCheckGame *stat)
{
	Board board[1];
	Move stack[99], *iter;
	MoveList movelist[1];
	int i, n_moves;
	int bestscore;

	board_init(board);
	if (!board_equal(board, game->initial_board)) return; // skip non standard game
	for (i = n_moves = 0; i <= 60 - book->options.n_empties && game->move[i] != NOMOVE; ++i) {
		if (!can_move(board->player, board->opponent)) {
			stack[n_moves++] = MOVE_PASS;
			board_pass(board);
		}
		if (!board_is_occupied(board, game->move[i]) && board_get_move(board, game->move[i], &stack[n_moves])) {
			board_update(board, stack + n_moves);
			++n_moves;
		} else {
			warn("illegal move in game");
			break; // stop, illegal moves
		}
	}

	while (--n_moves >= 0) {
		board_restore(board, stack + n_moves);
		if (movehash_append(hash, board, stack[n_moves].x)) {
			if (book_get_moves(book, board, movelist)) {
				bestscore = movelist_first(movelist)->score;
				foreach_move(iter, movelist) {
					if (iter->x == stack[n_moves].x) {
						if (iter->score < bestscore) ++stat->bad;
						else ++stat->good;
						break;
					}
				}
			} else ++stat->missing;		
		}
	}
}

/**
 * @brief Check positions from a game database.
 *
 * @param book opening book.
 * @param base games to add.
 */
void book_check_base(Book *book, const Base *base)
{
	int i;
	BookCheckGame stat = {0, 0, 0};
	MoveHash hash;

	bprint("Checking %d games to book...\n", base->n_games);
	movehash_init(&hash, options.hash_table_size);
	for (i = 0; i < base->n_games; ++i) {
		book_check_game(book, &hash, base->game + i, &stat);
	}
	movehash_delete(&hash);
    bprint("Positions : %llu missing, %llu good, %llu bad (%.2f%% bad)\n", stat.missing, stat.good, stat.bad, (100.0 * stat.bad)/(stat.bad + stat.good));
}


/**
 * @brief Extract book lines to a game base 
 *
 * Recursively add move to a PV until no move are available, 
 * where we dump the PV to a game data base.
 *
 * @param book Opening book.
 * @param board Starting position.
 * @param pv Previous moves leading to the starting  position.
 * @param base game database.
 */
static void extract_skeleton(Book *book, Board *board, Line *pv, Base *base)
{
	MoveList movelist[1];
	Move *move;
	Board init[1];
	Game game[1];
	int bestscore;

	if (book_get_moves(book, board, movelist)) {
		bestscore = movelist_best(movelist)->score;
		
		foreach_move(move, movelist) {
			if (move->score == bestscore) {
				board_update(board, move); line_push(pv, move->x);
					extract_skeleton(book, board, pv, base);
				board_restore(board, move); line_pop(pv);
			}
		}
	} else if (pv->n_moves) {
		board_init(init);
		line_to_game(init, pv, game);
		base_append(base, game);
		if (base->n_games % 1000 == 0) {
			bprint("extracting %d games\r", base->n_games);
			
		}
	}
}

/**
 * @brief Extract book draws to a game base 
 *
 * This function supposes that f5d6c4 & f5f6e6f4 are draws and the only draws, excluding the transpositions
 * f5d6c3d3c4 & f5f6e6c6 & c4..., d3..., e6...
 *
 * @param book Opening book.
 * @param base game database.
 */
void book_extract_skeleton(Book *book, Base *base)
{
	Line pv[1];
	Board board[1];

	line_init(pv, BLACK);
	line_push(pv, F5); line_push(pv, D6); line_push(pv, C4);
	board_init(board);
	board_next(board, F5, board); board_next(board, D6, board); board_next(board, C4, board);
	extract_skeleton(book, board, pv, base);
	
	line_init(pv, BLACK);
	line_push(pv, F5); line_push(pv, F6); line_push(pv, E6); line_push(pv, F4);
	board_init(board);
	board_next(board, F5, board); board_next(board, F6, board);
	board_next(board, E6, board); board_next(board, F4, board);	
	extract_skeleton(book, board, pv, base);
	bprint("%d games extracted   \n", base->n_games);
}


/**
 * @brief print a set of position.
 *
 * @param book Opening book.
 * @param n_empties Game stage.
 * @param n_positions Number of positions to extract.
 */
void book_extract_positions(Book *book, const int n_empties, const int n_positions)
{
	PositionArray *a;
	Position *p;
	MoveList movelist[1];
	Move *best, *second_best;
	int i = 0;
	char s[80];

	bprint("Extracting %d positions at %d ...\n", n_positions, n_empties); 
	foreach_position(p, a, book) {
		if (i == n_positions) break;
		if (board_count_empties(p->board) == n_empties) {
			position_get_moves(p, p->board, movelist);
			best = movelist_first(movelist);
			if (best) {
				second_best = move_next(best);
				if (second_best && best->score > second_best->score) {
					++i;
					board_to_string(p->board, n_empties & 1, s);
					bprint("%s %% bm ", s);
					move_print(best->x, n_empties & 1, stdout);
					bprint(":%+2d; ba ", best->score);
					move_print(second_best->x, n_empties & 1, stdout);
					bprint(":%+2d;\n", second_best->score);
				}
			}
		}
	}
}

/**
 * @brief print book statistics.
 *
 * @param book Opening book.
 */
void book_stats(Book *book)
{
	PositionArray *a;
	Position *p;
	int i;
	unsigned long long n_hash[256];
	unsigned long long n_pos[61], n_leaf[61], n_link[61], n_terminal[61];
	unsigned long long n_score[129];

	printf("\n\nBook statistics:\n");

	printf("\nHash distribution:\n");
	for (i = 0; i < 256; ++i) n_hash[i] = 0;
	for (a = book->array; a < book->array + book->n; ++a) {
		if (a->n < 256) ++n_hash[a->n];
		else ++n_hash[255];
	}
	printf("index    positions\n");
	for (i = 0; i < 255; ++i) if (n_hash[i]) printf("%5d %12llu\n", i, n_hash[i]);
	if (n_hash[i]) printf(">%4d %12llu\n", i - 1, n_hash[i]);

	printf("\nStage distribution:\n");
	printf("stage    positions        links       leaves      terminal nodes\n");
	for (i = 0; i < 61; ++i) n_pos[i] = n_leaf[i] = n_link[i] = n_terminal[i] = 0;
	foreach_position(p, a, book) {
		i = board_count_empties(p->board);
		++n_pos[i];
		if (p->leaf.move != NOMOVE) ++n_leaf[i];
		if (p->n_link == 0) ++n_terminal[i];
		n_link[i] += p->n_link;
	}
	for (i = 0; i < 61; ++i) if (n_pos[i]) printf("%5d %12llu %12llu %12llu %12llu\n", i, n_pos[i], n_link[i], n_leaf[i], n_terminal[i]);
		
	printf("\nBest Score Distribution:\n");
	printf("Score    positions\n");
	for (i = 0; i < 129; ++i) n_score[i] = 0;
	foreach_position(p, a, book) {
		++n_score[64 + p->score.value];
	}
	for (i = 0; i < 129; ++i) if (n_score[i]) printf("%+5d %12llu\n", i - 64, n_score[i]);
	printf("\n\n");
	fflush(stdout);
}

/**
 * @brief feed hash table from the opening book.
 * 
 * @param book Opening book.
 * @param board Position to start from.
 * @param search HashTables container.
 */
void book_feed_hash(const Book *book, Board *board, Search *search)
{
	board_feed_hash(board, book, search, true);
}
