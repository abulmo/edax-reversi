/**
 * @file book.h
 *
 * Header file for opening book management
 *
 * @date 1998 - 2024
 * @author Richard Delorme
 * @version 4.6
 */

#ifndef EDAX_BOOK_H
#define EDAX_BOOK_H

#include "base.h"
#include "board.h"
#include "game.h"
#include "search.h"
#include "util.h"
#include <stdbool.h>

/**
 * struct Book
 * @brief The opening book.
 */
typedef struct Book {
	struct {
		short year;
		char month, day;
		char hour, minute, second;
	} date;
	struct {
		int level;
		int n_empties;
		int midgame_error;
		int endcut_error;
		int verbosity;
	} options;
	struct {
		int n_nodes;
		int n_links;
		int n_todo;
	} stats;
	Random random;
	struct PositionArray *array;
	struct PositionStack* stack;
	Search *search;
	int n;
	int n_nodes;
	bool need_saving;
} Book;

/**
 * struct GameStat
 * @brief Game statistics
 */
typedef struct GameStats {
 	uint64_t n_wins;       /**< game win count */
	uint64_t n_draws;      /**< game draw count */
	uint64_t n_losses;     /**< game loss count */
	uint64_t n_lines;      /**< unterminated line count */
} GameStats;

void book_init(Book*);
void book_free(Book*);

void book_new(Book*, int, int);
void book_load(Book*, const char*);
void book_save(Book*, const char*);
void book_import(Book*, const char*);
void book_export(Book*, const char*);
void book_merge(Book*, const Book*);
void book_sort(Book *book);
void book_negamax(Book*);
void book_prune(Book*);
void book_deepen(Book*);
void book_correct_solved(Book*);
void book_link(Book*);
void book_fix(Book*);
void book_fill(Book *book, const int);
void book_deviate(Book*, const Board*, const int, const int);
void book_subtree(Book*, const Board*); 
void book_enhance(Book*, Board*, const int, const int);
void book_extend(Book*, const Board*);
void book_play(Book*, const Board*);

void book_info(Book*);
void book_show(Book*, Board*);
void book_stats(Book *book);
bool book_get_moves(Book*, const Board*, MoveList*);
bool book_get_random_move(Book*, const Board*, Move*, const int);
void book_get_game_stats(Book*, const Board*, GameStats*);
void book_get_line(Book*, const Board*, const Move*, Line*);

void book_add_board(Book*, const Board*);
void book_add_game(Book*, const Game*);
void book_add_base(Book*, const Base*);
void book_check_base(Book*, const Base*);

void book_extract_skeleton(Book*, Base*);
void book_extract_positions(Book*, const int, const int);

void book_feed_hash(const Book*, Board*, Search*);

#endif /* EDAX_BOOK_H */

