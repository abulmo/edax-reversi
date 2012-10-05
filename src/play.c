/**
 * @file play.c
 *
 * Edax play control.
 *
 * @date 1998 - 2012
 * @author Richard Delorme
 * @version 4.3
 */

#include "bit.h"
#include "const.h"
#include "game.h"
#include "move.h"
#include "opening.h"
#include "options.h"
#include "play.h"
#include "settings.h"

#include <assert.h>

/**
 * @brief Initialization.
 * @param play Play.
 * @param book Opening book.
 */
void play_init(Play *play, Book *book)
{
	search_init(play->search);
	play->book = book;
	board_init(play->initial_board);
	play->search->options.header = " depth|score|       time   |  nodes (N)  |   N/s    | principal variation";
	play->search->options.separator = "------+-----+--------------+-------------+----------+----------------------";
	play->player = play->initial_player = BLACK;
	play->time[0].left = options.time;
	play->time[0].extra = 0;
	play->time[1].left = options.time;
	play->time[1].extra = 0;
	play_new(play);
	lock_init(play->ponder);
	play->ponder->launched = false;
	spin_init(play->result);
	play->ponder->verbose = false;
	memset(play->error_message, 0, PLAY_MESSAGE_MAX_LENGTH);
	play_force_init(play, "F5");
}

/**
 * @brief Free resources.
 * @param play Play.
 */
void play_free(Play *play)
{
	play_stop_pondering(play);
	search_free(play->search);
}

/**
 * @brief Start a new game.
 * @param play Play.
 */
void play_new(Play *play)
{
	play->clock = real_clock();
	play->time[0].spent = play->time[1].spent = 0;
	*play->board = *play->initial_board;
	play->player = play->initial_player;
	play->ponder->board->player = play->ponder->board->opponent = 0;
	search_cleanup(play->search);
	play->i_game = play->n_game = 0;
	play->state = IS_WAITING;
	play->result->move = NOMOVE; // missing more initialisation ?
	play->time[0].left = options.time;
	play->time[1].left = options.time;
	play->force.i_move = 0;
}

/**
 * @brief Load a saved game.
 * @param play Play.
 * @param file File name of the game.
 * @return true if game is successfuly loaded.
 */
bool play_load(Play *play, const char *file)
{
	Game game[1];
	FILE *f;
	int i, l;
	char ext[8], move[8];

	f = fopen(file, "r");
	if (f == NULL) {
		sprintf(play->error_message, "Cannot open file %s\n", file);
		return false;
	}

	l = strlen(file); strcpy(ext, file + l - 4); string_to_lowercase(ext);

	if (strcmp(ext, ".txt") == 0) game_import_text(game, f);
	else if (strcmp(ext, ".ggf") == 0) game_import_ggf(game, f);
	else if (strcmp(ext, ".sgf") == 0) game_import_sgf(game, f);
	else if (strcmp(ext, ".pgn") == 0) game_import_pgn(game, f);
	else if (strcmp(ext, ".edx") == 0) game_read(game, f);
	else {
		sprintf(play->error_message, "Unknown game format extension: %s\n", ext);
		fclose(f);
		return false;
	}

	*play->initial_board = *game->initial_board;
	play->player = game->player;
	play_new(play);
	for (i = 0; i < 60 && game->move[i] != NOMOVE; ++i) {
		if (play_must_pass(play)) play_move(play, PASS);
		if (!play_move(play, game->move[i])) {
			sprintf(play->error_message, "Illegal move #%d: %s\n", i, move_to_string(game->move[i], play->player, move));
			fclose(f);
			return false;
		}
	}

	fclose(f);
	return true;
}

/**
 * @brief Save a played game.
 * @param play Play.
 * @param file File name of the game.
 */
void play_save(Play *play, const char *file)
{
	Game game[1];
	FILE *f;
	int i, j, l;
	char ext[8];


	game_init(game);
	*game->initial_board = *play->initial_board;
	game->player = play->player;
	for (i = j = 0; i < play->n_game; ++i) {
		if (play->game[i].x != PASS) {
			game->move[j++] = play->game[i].x;
		}
	}

	f = fopen(file, "w");
	if (f == NULL) {
		warn("Cannot open file %s\n", file);
		return;
	}

	l = strlen(file); strcpy(ext, file + l - 4); string_to_lowercase(ext);

	if (strcmp(ext, ".txt") == 0) game_export_text(game, f);
	else if (strcmp(ext, ".ggf") == 0) game_export_ggf(game, f);
	else if (strcmp(ext, ".sgf") == 0) game_save_sgf(game, f, true);
	else if (strcmp(ext, ".pgn") == 0) game_export_pgn(game, f);
	else if (strcmp(ext, ".eps") == 0) game_export_eps(game, f);
	else if (strcmp(ext, ".svg") == 0) game_export_svg(game, f);
	else if (strcmp(ext, ".edx") == 0) game_write(game, f);
	else warn("Unknown game format extension: %s\n", ext);

	fclose(f);
}


/**
 * @brief Update the game.
 * @param play Play.
 * @param move Move.
 */
void play_update(Play *play, Move *move)
{
	play_force_update(play);
	board_update(play->board, move);
	play->game[play->i_game] = *move;
	play->n_game = ++play->i_game;
	play->time[play->player].spent += real_clock() - play->clock;
	play->clock = real_clock();
	play->player ^= 1;
}

/**
 * @brief Check if game is over.
 * @param play Play.
 * @return true if game is over.
 */
bool play_is_game_over(Play *play)
{
	Board *board = play->board;
	return can_move(board->player, board->opponent) == false &&
		can_move(board->opponent, board->player) == false;
}

/**
 * @brief Check if player must pass.
 * @param play Play.
 * @return true if player must pass.
 */
bool play_must_pass(Play *play)
{
	Board *board = play->board;
	return can_move(board->player, board->opponent) == false &&
		can_move(board->opponent, board->player) == true;
}

/**
 * @brief Start thinking.
 * @param play Play.
 * @param update Flag to tell if edax should update or no its game.
 */
void play_go(Play *play, const bool update)
{
	extern Log xboard_log[1];

	long long t_real = -real_clock();
	long long t_cpu = -cpu_clock();
	Move move[1];
	Search *search = play->search;
	char s_move[4];

	if (play_is_game_over(play)) return;

	if (play_force_go(play, move)) {
		play_stop_pondering(play);

		play->result->depth = 0;
		play->result->selectivity = 0;
		play->result->move = move->x;
		play->result->score = 0;
		play->result->book_move = false;
		play->result->time = real_clock() + t_real;
		play->result->n_nodes = 0;
		line_init(play->result->pv, play->player);
		line_push(play->result->pv, move->x);

		if (options.verbosity) {
			info("\n[Forced move %s]\n\n",  move_to_string(move->x, play->player, s_move));
		}

	} else if (options.book_allowed && book_get_random_move(play->book, play->board, move, options.book_randomness)) {
		play_stop_pondering(play);

		play->result->depth = 0;
		play->result->selectivity = 0;
		play->result->move = move->x;
		play->result->score = move->score;
		play->result->book_move = true;
		play->result->time = real_clock() + t_real;
		play->result->n_nodes = 0;
		line_init(play->result->pv, play->player);
		book_get_line(play->book, play->board, move, play->result->pv);
		
		if (options.verbosity) {
			info("\n[book move]\n");
			if (options.info) book_show(play->book, play->board);
			info("\n\n");

			if (play->type == UI_XBOARD) {
				search->observer(play->result);
			} else {
				if (search->options.header) puts(search->options.header);
				if (search->options.separator) puts(search->options.separator);
				printf("book    %+02d                                          ", move->score);
				line_print(play->result->pv, options.width - 54, " ", stdout);
				putchar('\n');
				if (search->options.separator) puts(search->options.separator);
			} 
		}
	} else if (play->state == IS_PONDERING && board_equal(play->board, play->ponder->board)) {
		play->state = IS_THINKING;

		search->options.verbosity = options.verbosity;
		if (options.verbosity) {
			info("\n[switch from pondering to thinking (id.%d)]\n", play->search->id);
			if (search->options.header) puts(search->options.header);
			if (search->options.separator) puts(search->options.separator);
		}

		if (options.play_type == EDAX_TIME_PER_MOVE) search_set_move_time(search, options.time);
		else search_set_game_time(search, play->time[play->player].left);

		search_time_reset(search, play->board);
		if (log_is_open(xboard_log)) {
			 fprintf(xboard_log->f, "edax search> cpu: %d\n", options.n_task);
			 fprintf(xboard_log->f, "edax search> time: spent while pondering = %.2f mini = %.2f; maxi = %.2f; extra = %.2f\n",
				0.001 * search_time(search), 0.001 * search->time.mini,  0.001 * search->time.maxi,  0.001 * search->time.extra);
			 fprintf(xboard_log->f, "edax search> level: %d@%d%%\n",
				search->options.depth, selectivity_table[search->options.selectivity].percent);
		}

		search->observer(search->result);
		thread_join(play->ponder->thread);
		play->ponder->launched = false;
		search->observer(search->result);
				
		*play->result = *search->result;
		play->state = IS_WAITING;
		if (!board_get_move(play->board, search->result->move, move) && move->x != PASS) {
			fatal_error("bad move found: %s\n", move_to_string(move->x, play->player, s_move));
		}
		if (options.verbosity) {
			if (search->options.separator) puts(search->options.separator);
			info("[stop thinking (id.%d)]\n", play->search->id);
		}

	} else {

		play_stop_pondering(play);

		play->state = IS_THINKING;

		search->options.verbosity = options.verbosity;
		if (options.verbosity) {
			info("\n[start thinking (id.%d)]\n", play->search->id);
			if (search->options.header) puts(search->options.header);
			if (search->options.separator) puts(search->options.separator);
		}
		search_set_board(search, play->board, play->player);
		search_set_level(search, options.level, search->n_empties);
		if (options.play_type == EDAX_TIME_PER_MOVE) search_set_move_time(search, options.time);
		else search_set_game_time(search, play->time[play->player].left);

		search_time_init(search); // redondant, 
		if (log_is_open(xboard_log)) {
			 fprintf(xboard_log->f, "edax search> cpu: %d\n", options.n_task);
			 fprintf(xboard_log->f, "edax search> time: left = %.2f mini = %.2f; maxi = %.2f; extra = %.2f\n",
				0.001 * play->time[play->player].left, 0.001 * search->time.mini,  0.001 * search->time.maxi,  0.001 * search->time.extra);
			 fprintf(xboard_log->f, "edax search> level: %d@%d%%\n",
				search->options.depth, selectivity_table[search->options.selectivity].percent);
		}
		
		search_run(search);
		*play->result = *search->result;
		play->state = IS_WAITING;
		if (!board_get_move(play->board, search->result->move, move) && move->x != PASS) {
			fatal_error("bad move found: %s\n", move_to_string(move->x, play->player, s_move));
		}
		if (options.verbosity) {
			if (search->options.separator) puts(search->options.separator);
			info("[stop thinking] (id.%d)]\n", play->search->id);
		}
	}

	t_real += real_clock() + 1;
	t_cpu += cpu_clock() + 1;
	info("[cpu usage: %.2f%%]\n", 100.0 * t_cpu / t_real);

	if (options.play_type != EDAX_TIME_PER_MOVE) play->time[play->player].left -= t_real;

	if (update) play_update(play, move);

}

/**
 * @brief Start thinking.
 *
 * Evaluate first best moves of the position.
 *
 * @param play Play.
 * @param n Number of (best) moves to evaluate.
 */
void play_hint(Play *play, int n)
{
	Line pv[1];
	Move *m;
	Search *search = play->search;
	MoveList book_moves[1];
	GameStats stat;
	Board b[1];

	if (play_is_game_over(play)) return;

	play_stop_pondering(play);

	play->state = IS_THINKING;

	search->options.verbosity = options.verbosity;
	if (options.verbosity) {
		info("\n[start thinking]\n");
		if (search->options.header) puts(search->options.header);
		if (search->options.separator) puts(search->options.separator);
	}
	search_set_board(search, play->board, play->player);
	search_set_level(search, options.level, search->n_empties);
	if (n > search->movelist->n_moves) n = search->movelist->n_moves;
	info("<hint %d moves>\n", n);

	if (options.book_allowed && book_get_moves(play->book, play->board, book_moves)) {
		foreach_move (m, book_moves) if (n) {
			--n;
			line_init(pv, play->player);
			book_get_line(play->book, play->board, m, pv);
			movelist_exclude(search->movelist, m->x);
			if (play->type == UI_NBOARD) {
				board_next(play->board, m->x, b);
				book_get_game_stats(play->book, b, &stat);
				printf("book "); line_print(pv, 10, NULL, stdout);
				printf(" %d %llu %d\n", m->score, stat.n_lines, play->book->options.level);
			} else {
				printf("book    %+02d                                          ", m->score);
				line_print(pv, options.width - 54, " ", stdout); putchar('\n');
			}
		}
	}

	while (n--) {
		if (options.play_type == EDAX_TIME_PER_MOVE) search_set_move_time(search, options.time);
		else search_set_game_time(search, play->time[play->player].left);
		if (n) search->options.multipv_depth = 60;
		search_run(search);
		search->options.multipv_depth = MULTIPV_DEPTH;
		if (play->type == UI_NBOARD) {
			printf("search "); line_print(search->result->pv, 10, NULL, stdout);
			printf(" %d 0 %d\n", search->result->score, search->result->depth);
		} else {
			if (options.verbosity == 0) search->observer(search->result);
		}
		if (search->stop != STOP_END) break;
		movelist_exclude(search->movelist, search->result->move);
	}
	if (options.verbosity) {
		info("\n[stop thinking]\n");
		if (search->options.separator) puts(search->options.separator);
	}
}

/**
 * @brief do ponderation.
 *
 * Ponderation (thinking during opponent time) is done within a thread.
 * The thread is launched at startup and immediately suspended, thanks to
 * condition_wait. When edax is required to ponder, the thread is activated
 * by a condition_signal, and the search start. To stop the ponderation,
 * just stop the search and wait for the lock to be release.
 *
 * @param v the play.
 * @return NULL (unused).
 */
void* play_ponder_run(void *v)
{
	extern Log xboard_log[1];
	Play *play = (Play*) v;
	int player;
	Search *search = play->search;
	Board board[1];
	Move move[1];
	char m[4];

	lock(play->ponder);
	if (play->state == IS_PONDERING || play->state == IS_ANALYZING) {
		*board = *play->board;
		player = play->player;
		search_set_game_time(search, TIME_MAX);
		search->options.keep_date = (play->state == IS_PONDERING && search->pv_table->date > 0);
		if (play->ponder->verbose) search->options.verbosity = options.verbosity;
		else search->options.verbosity = 0;

		move->x = search_guess(search, board);

		// guess opponent move and start the search
		if (play->state == IS_PONDERING && move->x != NOMOVE) {
			board_get_move(board, move->x, move);

			board_update(board, move);
				*play->ponder->board = *board;
				search_set_board(search, board, player ^ 1);
				search_set_level(search, options.level, search->n_empties);
				search_run(search);
				if (options.info && play->state == IS_PONDERING) {
					printf("[ponder after %s id.%d: ", move_to_string(move->x, player, m), search->id);
					result_print(search->result, stdout);
					printf("]\n");
				}
			board_restore(board, move);
		} else {
			*play->ponder->board = *board;
			search_set_board(search, board, player);
			search_set_ponder_level(search, options.level, search->n_empties);
			log_print(xboard_log, "edax (ponder)> start search\n");
			search_run(search);
			log_print(xboard_log, "edax (ponder)> search ended\n");
			if (options.info && play->state == IS_PONDERING) {
				printf("[ponder (without move) id.%d: ", search->id);
				result_print(search->result, stdout);
				printf("]\n");
			}
		}

		info("[ponderation finished]\n");
		play->state = IS_WAITING;
		search->options.keep_date = false;
	}
	unlock(play->ponder);

	return NULL;
}

/**
 * @brief Ponder.
 *
 * Think during opponent time. Activate the thread suspended in
 * play_ponder_loop.
 *
 * @param play Play.
 */
void play_ponder(Play *play)
{
	if (play_is_game_over(play)) return;
	if (options.can_ponder && play->state == IS_WAITING) {
		play->ponder->board->player = play->ponder->board->opponent = 0;
		play->state = IS_PONDERING;		
		info("\n[start ponderation]\n");
		thread_create(&play->ponder->thread, play_ponder_run, play);
		play->ponder->launched = true;
	}
}

/**
 * @brief Stop pondering.
 *
 * If edax is pondering, stop the search, and wait that the pondering thread
 * is suspended.
 *
 * @param play Play.
 */
void play_stop_pondering(Play *play)
{
	while (play->state == IS_PONDERING) {
		info("[stop pondering]\n");
		search_stop_all(play->search, STOP_PONDERING);
		relax(10);
	}

	if (play->ponder->launched) {
		info("[joining thread]\n");
		thread_join(play->ponder->thread);
		play->ponder->launched = false;
		info("[thread joined]\n");
	}
}

/**
 * @brief Stop thinking.
 * @param play Play.
 */
void play_stop(Play *play)
{
	search_stop_all(play->search, STOP_ON_DEMAND);
	info("[stop on user demand]\n");
}

/**
 * @brief Undo a move.
 * @param play Play.
 */
void play_undo(Play *play)
{
	if (play->i_game > 0) {
		play_stop_pondering(play);
		lock(play->ponder);
		play_force_restore(play);
		board_restore(play->board, &play->game[--play->i_game]);
		play->player ^= 1;
		unlock(play->ponder);
	}
}

/**
 * @brief Redo a move.
 * @param play Play.
 */
void play_redo(Play *play)
{
	if (play->i_game < play->n_game) {
		play_stop_pondering(play);
		play_force_update(play);
		board_update(play->board, &play->game[play->i_game++]);
		play->player ^= 1;
	}
}

/**
 * @brief Set a new board.
 * @param play Play.
 * @param board A new board.
 */
void play_set_board(Play *play, const char *board)
{
	play_stop_pondering(play);
	play->initial_player = board_set(play->initial_board, board);
	if (play->initial_player == EMPTY) { /* bad board ? */
		play->initial_board->opponent &= (~play->initial_board->player);
		play->initial_player = (board_count_empties(play->initial_board) & 1);
		if (play->initial_player == WHITE) board_swap_players(play->initial_board);
	}
	play_force_init(play, "");
	play_new(play);
}

/**
 * @brief Set a new board.
 * @param play Play.
 * @param board A new board.
 */
void play_set_board_from_FEN(Play *play, const char *board)
{
	play_stop_pondering(play);
	play->initial_player = board_from_FEN(play->initial_board, board);
	if (play->initial_player != EMPTY) {
		play_force_init(play, "");
		play_new(play);
	}
}

/**
 * @brief Play a move sequence.
 * @param play Play.
 * @param string move sequence.
 */
void play_game(Play *play, const char *string)
{
	Move move[1];
	const char *next;

	play_stop_pondering(play);

	// convert an opening name to a move sequence...
	next = opening_get_line(string);
	if (next) string = next;	

	while ((next = parse_move(string, play->board, move)) != string || move->x == PASS) {
		string = next;
		play_update(play, move);
	}
}

/**
 * @brief Play a move.
 * @param play Play.
 * @param x Coordinate played.
 * @return true if the move has been legally played.
 */
bool play_move(Play *play, int x)
{
	Move move[1];

	*move = MOVE_INIT;
	board_get_move(play->board, x, move);
	if (board_check_move(play->board, move)) {
		play_update(play, move);
		return true;
	} else {
		return false;
	}
}

/**
 * @brief Play a user move.
 * @param play Play.
 * @param string Move as a string.
 * @return true if the move has been legally played.
 */
bool play_user_move(Play *play, const char *string)
{
	Move move[1];

	if (parse_move(string, play->board, move) != string) {
		play_update(play, move);
		return true;
	} else {
		return false;
	}
}


/**
 * @brief Get the last played move.
 * @param play Play.
 * @return last played move.
 */
Move* play_get_last_move(Play *play)
{
	return play->i_game ? play->game + play->i_game - 1 : NULL;
}


/**
 * @brief Seek for the best alternative move.
 *
 * @param play Play.
 * @param played Last played move.
 * @param alternative Second best move.
 * @param depth Depth searched.
 * @param percent Probcut selectivity searched.
 * @return The number of alternatives.
 */
static int play_alternative(Play *play, Move *played, Move *alternative, int *depth, int *percent)
{
	Search *search = play->search;
	Result *result = search->result;
	Board excluded[1], board[1], unique[1];
	Move *move;

	search_set_board(search, play->board, play->player);
	if (A1 <= played->x && played->x <= H8) {
		movelist_exclude(search->movelist, played->x);
		hash_exclude_move(search->pv_table, board_get_hash_code(search->board), played->x);
		hash_exclude_move(search->hash_table, board_get_hash_code(search->board), played->x);
		// also remove moves leading to symetrical positions
		board_next(play->board, played->x, board);
		board_unique(board,  excluded);
		foreach_move (move, search->movelist) {
			board_next(play->board, move->x, board);
			board_unique(board, unique);
			if (board_equal(excluded, unique)) {
				hash_exclude_move(search->pv_table, board_get_hash_code(search->board), move->x);
				hash_exclude_move(search->hash_table, board_get_hash_code(search->board), move->x);
				move = movelist_exclude(search->movelist, move->x);
			}
		}
	}
	if (search->movelist->n_moves >= 1 || played->x == NOMOVE) {
		search_set_level(search, options.level, search->n_empties);
		search->options.verbosity = 0;
		search_run(search);
		search->options.verbosity = options.verbosity;
		alternative->x = result->move;
		alternative->score = result->score;
		*depth = result->depth;
		*percent = selectivity_table[result->selectivity].percent;
	}
	return search->movelist->n_moves;
}

/**
 * @brief Write a line if an analysis.
 *
 * Write a line of a post-mortem game analysis.
 *
 * @param play Play.
 * @param m Move actually played.
 * @param a Best alternative move.
 * @param n_moves Number of alternatives.
 * @param depth Analysis depth.
 * @param percent Analysis probcut selectivity (as a percent).
 * @param f Output stream.
 */
static void play_write_analysis(Play *play, const Move *m, const Move *a, const int n_moves, const int depth, const int percent, FILE *f)
{
	char s[4];

	if (n_moves >= 0) {
		int n_empties = board_count_empties(play->board);
		fprintf(f, "%3d ", 61 - n_empties);
		if (depth == -1) fputs("  book  ", f);
		else {
			fprintf(f, " %3d", depth);
			if (percent < 100) fprintf(f, "@%2d%%", percent);
			else fputs("    ", f);
		};
		fprintf(f, "%3d   ", n_moves);
		fprintf(f, " %s    %+3d  ", move_to_string(m->x, play->player, s), m->score);
		if (n_moves > 0) {
			if (m->score > a->score) fputs(" > ", f);
			else if (m->score == a->score) fputs(" = ", f);
			else fputs( " < ", f);
			fprintf(f, "  %+3d     %s ", a->score, move_to_string(a->x, play->player, s));
			if (m->score < a->score) {
				if (depth == n_empties) {
					if (percent == 100) fputs("<- Mistake", f);
					else fputs("<- Possible mistake", f);
				} else {
					fputs("<- Edax disagrees", f);
					if (a->score - m->score > 4) fputs(" strongly", f);
				}
			}
		}
		putc('\n', f);
	}
}

/**
 * @brief Analyze a played game.
 * @param play Play.
 * @param n number of moves to analyze.
 */
void play_analyze(Play *play, int n)
{
	int i, score, n_alternatives;
	int depth = 0, percent = 0, n_empties;
	Move *move, alternative[1];
	int n_exact[2] = {0, 0}, n_eval[2] = {0, 0};
	int n_error[2] = {0, 0}, n_rejection[2] = {0, 0};
	int disc_error[2] = {0, 0}, disc_rejection[2] = {0, 0};
	const char *clr = "                                                                              \r";
	

	play_stop_pondering(play);

	puts("\n              N     played        alternative");
	puts("ply  level   alt. move  score     score   move");
	puts("---+-------+-----+-----------+--+---------------");

	search_cleanup(play->search);
	search_set_board(play->search, play->board, play->player);
	alternative->x = NOMOVE;
	play_alternative(play, alternative, alternative, &depth, &percent);
	score = alternative->score;

	for (i = play->i_game - 1; i >= 0 && i >= play->i_game - n; --i) {
		move = play->game + i;
		move->score = -score;
		play->player ^= 1;
		board_restore(play->board, move);
		if (move->x == PASS) ++n;

		n_empties = board_count_empties(play->board);
		n_alternatives = play_alternative(play, move, alternative, &depth, &percent);
		if (options.verbosity == 1) fputs(clr, stdout);
		play_write_analysis(play, move, alternative, n_alternatives, depth, percent, stdout);

		score = move->score; 
		if (n_alternatives > 0) {
			if (depth == n_empties && percent == 100) ++n_exact[play->player]; else ++n_eval[play->player];
			if (alternative->score > score) {
				if (depth == n_empties && percent == 100) {
					++n_error[play->player];
					disc_error[play->player] += alternative->score - score;
				} else {
					++n_rejection[play->player];
					disc_rejection[play->player] += alternative->score - score;
				}
				score = alternative->score;
			}
		}
		if (play->search->stop == STOP_ON_DEMAND) break;
	}
	puts("\n      | rejections : discs | errors    : discs | error rate |");
	printf("Black | %3d / %3d  :  %+4d | %3d / %3d :  %+4d |      %5.3f |\n",
		n_rejection[BLACK], n_eval[BLACK], disc_rejection[BLACK], n_error[BLACK], n_exact[BLACK], disc_error[BLACK], 1.0 * disc_error[BLACK] / n_exact[BLACK]);
	printf("White | %3d / %3d  :  %+4d | %3d / %3d :  %+4d |      %5.3f |\n",
		n_rejection[WHITE], n_eval[WHITE], disc_rejection[WHITE], n_error[WHITE], n_exact[WHITE], disc_error[WHITE], 1.0 * disc_error[WHITE] / n_exact[WHITE]);

	if (i < 0 || i < play->i_game - n) ++i;
	for (; i < play->i_game; ++i) {
		board_update(play->board, play->game + i);
		play->player ^= 1;
	}
}

/**
 * @brief Seek for the best alternative move from the opening book.
 *
 * @param play Play.
 * @param played Last played move.
 * @param alternative Second best move.
 * @return The number of alternatives.
 */
static int play_book_alternative(Play *play, Move *played, Move* alternative)
{
	MoveList movelist[1];
	Move *exclude, *best;
	Board excluded[1], board[1], unique[1];
	Move *move;

	if (book_get_moves(play->book, play->board, movelist)) {
		exclude = movelist_exclude(movelist, played->x);

		if (exclude && exclude->x == played->x) {
			played->score = exclude->score;
			// also remove moves leading to symetrical positions
			board_next(play->board, played->x, board);
			board_unique(board,  excluded);
			foreach_move (move, movelist) {
				board_next(play->board, move->x, board);
				board_unique(board, unique);
				if (board_equal(excluded, unique)) move = movelist_exclude(movelist, move->x);
			}
			if (movelist->n_moves) {
				best = movelist_best(movelist);
				*alternative = *best;
			}
			return movelist->n_moves;
		}
	}
	return -1;
}

/**
 * @brief Analyze a played game.
 * @param play Play.
 * @param n number of moves to analyze.
 */
void play_book_analyze(Play *play, int n)
{
	int i, n_alternatives;
	Move *move, alternative[1];

	play_stop_pondering(play);

	puts("\n              N     played        alternative");
	puts("ply  level   alt. move  score     score   move");
	puts("---+-------+-----+-----------+--+---------------");

	for (i = play->i_game - 1; i >= 0 && i >= play->i_game - n; --i) {
		move = play->game + i;
		play->player ^= 1;
		board_restore(play->board, move);
		if (move->x == PASS) ++n;

		n_alternatives = play_book_alternative(play, move, alternative);
		play_write_analysis(play, move, alternative, n_alternatives, -1, 100, stdout);

		if (play->search->stop == STOP_ON_DEMAND) break;
	}

	if (i < 0 || i < play->i_game - n) ++i;
	for (; i < play->i_game; ++i) {
		board_update(play->board, play->game + i);
		play->player ^= 1;
	}
}

/**
 * @brief store the game into the opening book
 *
 * @param play Play.
 */
void play_store(Play *play)
{
	Board board[1];
	int i;
	char file[FILENAME_MAX + 1];

	file_add_ext(options.book_file, ".store", file);

	play->book->stats.n_nodes = play->book->stats.n_links = 0;

	*board = *play->initial_board;
	for (i = 0; i < play->n_game && board_check_move(board, play->game + i); ++i) {
		board_update(board, play->game + i);
	}

	for (--i; i >= 0; --i) {
		book_add_board(play->book, board);
		board_restore(board, play->game + i);
	}
	book_add_board(play->book, board);
	if (play->book->stats.n_nodes + play->book->stats.n_links) {
		book_link(play->book);
		book_negamax(play->book);
		book_save(play->book, file);
	}
}

/**
 * @brief adjust time.
 *
 * Set remaining time to play from a server (GGS) or a GUI (Quarry, ...).
 *
 * @param play Play.
 * @param left Time left.
 * @param extra Extra time.
 */
void play_adjust_time(Play *play, const int left, const int extra)
{
	play->time[play->player].left = left;
	play->time[play->player].extra = extra;
}

/**
 * @brief Print the game state.
 *
 * Print the game state: board, time, played move, etc.
 *
 * @param play Play.
 * @param f Output stream.
 */
void play_print(Play *play, FILE *f)
{
	int i, j, x, discs[2], mobility[2], square;
	char history[64];
	const char *color = "?*O-." + 1;
	const char big_color[3][4] = {"|##", "|()", "|  "};
	const char player[2][6] = {"Black", "White"};
	Board *board = play->board;
	const int p = play->player;
	const int o = !p;
	const int ip = play->player ^ (play->i_game & 1);
	unsigned long long moves;

	moves = get_moves(board->player, board->opponent);
	discs[p] = bit_count(board->player);
	discs[o] = bit_count(board->opponent);
	mobility[p] = get_mobility(board->player, board->opponent);
	mobility[o] = get_mobility(board->opponent, board->player);
	memset(history, 0, 64);
	for (i = j = 0; i < play->i_game; i++) {
		x = play->game[i].x;
		if (A1 <= x && x <= H8) history[x] = ++j;
	}

	fputs("  A B C D E F G H            BLACK            A  B  C  D  E  F  G  H\n", f);
	for (i = 0; i < 8; i++) {
		fputc(i + '1', f);
		fputc(' ', f);
		for (j = 0; j < 8; j++) {
			x = i * 8 + j;
			if (p == BLACK) square = 2 - ((board->opponent >> x) & 1) - 2 * ((board->player >> x) & 1);
			else square = 2 - ((board->player >> x) & 1) - 2 * ((board->opponent >> x) & 1);
			if (square == EMPTY && (moves & x_to_bit(x))) ++square;
			fputc(color[square], f);
			fputc(' ', f);
		}
		fputc(i + '1', f);

		switch(i) {
		case 0:
			fputs("  ", f); time_print(play->time[BLACK].spent, true, f); fputs("       ", f);
			break;
		case 1:
			fprintf(f,"   %2d discs  %2d moves   ", discs[BLACK], mobility[BLACK]);
			break;
		case 3:
			if (mobility[BLACK] + mobility[WHITE] == 0) fprintf(f,"       Game over        ");
			else fprintf(f,"  ply %2d (%2d empties)   ", play->i_game + 1, board_count_empties(board));
			break;
		case 4:
			if (mobility[BLACK] + mobility[WHITE] == 0) {
				if (discs[BLACK] > discs[WHITE]) fprintf(f,"       %s won        ", player[BLACK]);
				else if (discs[BLACK] < discs[WHITE]) fprintf(f,"       %s won        ", player[WHITE]);
				else fprintf(f,"          draw          ");
			} else fprintf(f,"    %s's turn (%c)    ",player[p], color[p]);
			break;
		case 6:
			fprintf(f,"   %2d discs  %2d moves   ",discs[WHITE], mobility[WHITE]);
			break;
		case 7:
			fputs("  ", f); time_print(play->time[WHITE].spent, true, f); fputs("       ", f);
			break;
		default:
			fputs("                        ", f);
			break;
		}
		fputc(i + '1', f); fputc(' ', f);
		for (j = 0; j < 8; j++){
			x = i * 8 + j;
			if (history[x]) fprintf(f,"|%2d",history[x]);
			else {
				if (ip == BLACK) square = 2 - ((play->initial_board->opponent >> x) & 1) - 2 * ((play->initial_board->player >> x) & 1);
				else square = 2 - ((play->initial_board->player >> x) & 1) - 2 * ((play->initial_board->opponent >> x) & 1);
				fputs(big_color[square], f);
			}
		}
		fprintf(f, "| %1d\n", i + 1);
	}
	fputs("  A B C D E F G H            WHITE            A  B  C  D  E  F  G  H\n", f);
	fflush(f);
}

/**
 * @brief Initialize a forced line.
 *
 * @param play Play.
 * @param string A string with a sequence of moves.
 */
void play_force_init(Play *play, const char *string)
{
	Move move[1];
	const char *next;
	Board board[1];

	play->force.n_move = play->force.i_move = 0;
	*board = *play->initial_board;
	play->force.real[play->force.n_move] = *board;
	board_unique(board, play->force.unique + play->force.n_move);

	next = opening_get_line(string);
	if (next) string = next;

	while ((next = parse_move(string, board, move)) != string || move->x == PASS) {
		string = next;
		play->force.move[play->force.n_move] = *move;
		board_update(board, move);
		++play->force.n_move;
		play->force.real[play->force.n_move] = *board;
		board_unique(board, play->force.unique + play->force.n_move);
	}
}

/**
 * @brief Update a forced line.
 *
 * Check if the actual board is in the forced line, and update the
 * forced line accordingly.
 *
 * @param play Play.
 */
void play_force_update(Play *play)
{
	Board unique[1];
	if (play->force.i_move < play->force.n_move) {
		board_unique(play->board, unique);
		if ((board_equal(unique, play->force.unique + play->force.i_move))) {
			++play->force.i_move;
		}
	}
}

/**
 * @brief Restore a forced line.
 *
 * Check if the actual board is in the forced line, and restore the
 * forced line accordingly.
 *
 * @param play Play.
 */
void play_force_restore(Play *play)
{
	Board unique[1];
	if (play->force.i_move > 0) {
		board_unique(play->board, unique);
		if (board_equal(unique, play->force.unique + play->force.i_move)) {
			--play->force.i_move;
		}
	}
}

/**
 * @brief Play a forced move.
 *
 * Check if the actual board is in the forced line, and play the next forced move.
 *
 * @param play Play.
 * @param move Output move.
 * @return 'true' if a forced move have been found.
 */
bool play_force_go(Play *play, Move *move)
{
	Board unique[1];
	Board sym[1];
	int s, x;

	if (play->force.i_move < play->force.n_move) {
		if (board_equal(play->board, play->force.real + play->force.i_move)) {
			*move = play->force.move[play->force.i_move];
			return true;
		}

		board_unique(play->board, unique);
		if (board_equal(unique, play->force.unique + play->force.i_move)) {
			for (s = 1; s < 8; ++s) {
				board_symetry(play->force.real +  + play->force.i_move, s, sym);
				if (board_equal(play->board, sym)) {
					x = symetry(play->force.move[play->force.i_move].x, s);
					board_get_move(play->board, x, move);
					return true;
				}
			}
		}
	}

	return false;
}

/**
 * @brief Get the symetry of the actual position.
 *
 * @param play Play.
 * @param sym Symetry.
 */
void play_symetry(Play *play, const int sym)
{
	int i, x;
	Move move = MOVE_INIT;
	Board board[1];

	board_symetry(play->initial_board, sym, play->initial_board);
	board_symetry(play->board, sym, play->board);
	*board = *play->initial_board;
	for (i = 0; i  < play->n_game; ++i) {
		x = symetry(play->game[i].x, sym);
		board_get_move(board, x, &move);
		board_update(board, &move);
		play->game[i] = move;
	}
}

/**
 * @brief Print the opening name 
 */
const char* play_show_opening_name(Play *play, const char *(*opening_get_name)(const Board*))
{
	int i;
	Board board[1];
	const char *name;
	const char *last = NULL;

	*board = *play->initial_board;
	for (i = 0; i  < play->i_game; ++i) {
		board_update(board, play->game + i);
		name = opening_get_name(board);
		if (name != NULL) last = name;
	}

	return last;
}

