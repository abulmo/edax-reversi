/**
 * @file base.c
 *
 * Header file for game base management.
 *
 * @date 1998 - 2012
 * @author Richard Delorme
 * @version 4.3
 */

#include "base.h"
#include "options.h"
#include "search.h"
#include "perft.h"

#include <assert.h>
#include <stdio.h>

/**
 * @brief Set wthor header.
 *
 * @param wheader Wthor's file header.
 * @param n_games Game number;
 * @param n  Tourney/player number;
 * @param year Tournament year;
 */
static void wthor_header_set(WthorHeader *wheader, unsigned int n_games, unsigned int n, unsigned int year)
{
	time_t t;
	struct tm *date;

	t = time(NULL); date = localtime(&t);
	wheader->century = date->tm_year / 100 + 19;
	wheader->year = date->tm_year % 100;
	wheader->month = date->tm_mon + 1;
	wheader->day = date->tm_mday;
	wheader->n_games = n_games;
	wheader->n = n;
	if (year) wheader->game_year = year;
	else wheader->game_year = date->tm_year + 1900;
	wheader->board_size = 8;
	wheader->game_type = 0;
	wheader->depth = 1;
	wheader->reserved = 0;
}

/**
 * @brief Read wthor header.
 *
 * @param wheader Wthor's file header.
 * @param f Input stream .
 */
static bool wthor_header_read(WthorHeader *wheader, FILE *f)
{
	int r;

	r = fread(&wheader->century, 1, 1, f);
	r += fread(&wheader->year, 1, 1, f);
	r += fread(&wheader->month, 1, 1, f);
	r += fread(&wheader->day, 1, 1, f);
	r += fread(&wheader->n_games, 4, 1, f); // ok only on intel-x86 endianness.
	r += fread(&wheader->n, 2, 1, f);
	r += fread(&wheader->game_year, 2, 1, f);
	r += fread(&wheader->board_size, 1, 1, f);
	r += fread(&wheader->game_type, 1, 1, f);
	r += fread(&wheader->depth, 1, 1, f);
	r += fread(&wheader->reserved, 1, 1, f);

	if (r != 11) {
		warn("Cannot read wthor header (r = %d)\n", r);
		return false;
	}

	return true;
}

/**
 * @brief Read wthor header.
 *
 * @param wheader Wthor's file header.
 * @param f Input stream .
 */
static bool wthor_header_write(WthorHeader *wheader, FILE *f)
{
	int r;

	r = fwrite(&wheader->century, 1, 1, f);
	r += fwrite(&wheader->year, 1, 1, f);
	r += fwrite(&wheader->month, 1, 1, f);
	r += fwrite(&wheader->day, 1, 1, f);
	r += fwrite(&wheader->n_games, 4, 1, f); // ok only on intel-x86 endianness.
	r += fwrite(&wheader->n, 2, 1, f);
	r += fwrite(&wheader->game_year, 2, 1, f);
	r += fwrite(&wheader->board_size, 1, 1, f);
	r += fwrite(&wheader->game_type, 1, 1, f);
	r += fwrite(&wheader->depth, 1, 1, f);
	r += fwrite(&wheader->reserved, 1, 1, f);

	if (r != 11) {
		warn("Cannot write wthor header (r = %d)\n", r);
		return false;
	}

	return true;
}

/**
 * @brief Init wthor players.
 *
 * @param base Wthor base.
 */
static void wthor_players_init(WthorBase *base)
{
	base->n_players = 1;
	base->player = (char (*)[20]) malloc(sizeof (*base->player));
	if (base->player) {
	    strncpy(base->player[0], "?", 20); // used on purpose, as strncpy fills with '\0' the field name
	} else {
		warn("Cannot allocate Wthor players' array\n");
		base->n_players = 0;
	}
}

/**
 * @brief Load wthor players.
 *
 * @param base Wthor base.
 * @param file File name.
 */
static void wthor_players_load(WthorBase *base, const char *file)
{
	FILE *f;
	WthorHeader header[1];
	int i, r;

	r = base->n_players = 0;

	if ((f = fopen(file, "rb")) == NULL) {
		warn("Cannot open Wthor players' file %s\n", file);
		fprintf(stderr, "Creating a new %s file\n\n", file);
		wthor_players_init(base);
		return;
	}

	if (wthor_header_read(header, f)) {;

		base->n_players = header->n;

		base->player = (char (*)[20]) malloc(base->n_players * sizeof (*base->player));
		if (base->player) {
			for (i = 0; i < base->n_players; ++i) {
				r += fread(base->player[i], 20, 1, f);
			}
		} else {
			warn("Cannot allocate Wthor players' array\n");
		}

		if (r != base->n_players) {
	        warn("Error while reading Wthor players' file %s %d/%d\n", file, r, base->n_players);
			base->n_players = 0;
		}
	}

	fclose(f);
}

/**
 * @brief Load wthor players.
 *
 * @param base Wthor base.
 * @param file File name.
 */
static void wthor_players_save(WthorBase *base, const char *file)
{
	FILE *f;
	WthorHeader header[1];
	int i, r;

	r = 0;

	wthor_header_set(header, 0, base->n_players, 0);

	if ((f = fopen(file, "wb")) == NULL) {
		warn("Cannot open Wthor players' file %s\n", file);
		return;
	}

	if (wthor_header_write(header, f)) {

		for (i = 0; i < base->n_players; ++i) {
			r += fwrite(base->player[i], 20, 1, f);
		}

		if (r != base->n_players) {
	        warn("Error while reading Wthor players' file %s %d/%d\n", file, r, base->n_players);
			base->n_players = 0;
		}
	}

	fclose(f);
}

/**
 * @brief Get a Wthor player's index.
 *
 * @param base Wthor base.
 * @param name Player's name.
 */
int wthor_player_get(WthorBase *base, const char *name) 
{
	int i, n;
	char (*player)[20];

	assert(base != NULL && name != NULL);
	assert(base->player != NULL && base->n_players > 0);

	for (i = 0; i < base->n_players; ++i) {
	    if (strcmp(name, base->player[i]) == 0) return i;            
	}
	
	n = base->n_players + 1;
	player = (char (*)[20]) realloc(base->player, n * sizeof (*base->player));
	if (player) {
	    base->player = player;
	    base->n_players = n; 
	    strncpy(base->player[i], name, 20); // used on purpose, as strncpy fills with '\0' the field name
		base->player[i][19] = '\0'; // force null terminated string
	} else {
		warn("Cannot allocate Wthor players' array\n");
		i = 0;
	}

	return i;
}

/**
 * @brief Load wthor tournaments.
 *
 * @param base Wthor base.
 * @param file File name.
 */
static void wthor_tournaments_load(WthorBase *base, const char *file)
{
	FILE *f;
	WthorHeader header[1];
	int i, r;

	r = 0;
	base->n_tournaments = 0;

	if ((f = fopen(file, "rb")) == NULL) {
		warn("Cannot open Wthor tournaments' file %s\n", file);
		return;
	}

	if (wthor_header_read(header, f)) {
		base->n_tournaments = header->n;

		base->tournament = (char (*)[26]) malloc(base->n_tournaments * sizeof (*base->tournament));
		if (base->tournament) {
			for (i = r = 0; i < base->n_tournaments; ++i) {
				r += fread(base->tournament[i], 26, 1, f);
			}
		} else {
			warn("Cannot allocate Wthor tournaments' array\n");
		}

		if (r != base->n_tournaments) {
			warn("Error while reading %s %d/%d\n", file, r, base->n_tournaments);
			base->n_tournaments = 0;
		}
	}

	fclose(f);
}

#if 0 // Unused
/**
 * @brief Load wthor tournaments.
 *
 * @param base Wthor base.
 * @param file File name.
 */
static void wthor_tournaments_save(WthorBase *base, const char *file)
{
	FILE *f;
	WthorHeader header[1];
	int i, r;

	r = 0;

	if ((f = fopen(file, "rw")) == NULL) {
		warn("Cannot open Wthor tournaments' file %s\n", file);
		return;
	}
	wthor_header_set(header, 0, base->n_tournaments, 0);
	if (wthor_header_write(header, f)) {
		for (i = r = 0; i < base->n_tournaments; ++i) {
			r += fwrite(base->tournament[i], 26, 1, f);
		}
		if (r != base->n_tournaments) {
			warn("Error while writing %s %d/%d\n", file, r, base->n_tournaments);
		}
	}

	fclose(f);
}
#endif

/**
 * @brief Initialize a Wthor base.
 *
 * @param base Wthor's base.
 */
void wthor_init(WthorBase *base)
{
	base->n_tournaments = 0;
	base->n_players = 0;
	base->n_games = 0;
	base->tournament = NULL;
	base->player = NULL;
	base->game = NULL;
}

/**
 * @brief Load a wthor base.
 *
 * @param base Wthor game base.
 * @param file Game file.
 */
bool wthor_load(WthorBase *base, const char *file)
{
	FILE *f;
	char path[FILENAME_MAX];
	int r = 0;

	wthor_init(base);

	path_get_dir(file, path); strcat(path, "WTHOR.TRN");
	wthor_tournaments_load(base, path);

	path_get_dir(file, path); strcat(path, "WTHOR.JOU");
	wthor_players_load(base, path);

	if ((f = fopen(file, "rb")) != NULL) {
		if (wthor_header_read(base->header, f) && base->header->board_size == 8) {
			base->n_games = base->header->n_games;

			base->game = (WthorGame*) malloc(base->n_games * sizeof (WthorGame));
			if (base->game) {
				r = (fread(base->game, sizeof (WthorGame), base->n_games, f) == (unsigned) base->n_games);
				if (!r) warn("Error while reading %s\n", file);
			} else {
				warn("Cannot allocate whor games\n");
			}
		}
		fclose(f);
	} else {
		base->n_games = 0;
		base->game = NULL;
		warn("Cannot open file %s\n", file);
	}

	return r != 0;
}

/**
 * @brief Free a wthor base.
 *
 * @param base Wthor game base.
 */
void wthor_free(WthorBase *base)
{
	free(base->player);
	free(base->tournament);
	free(base->game);
	wthor_init(base);
}

/**
 * @brief Save a wthor base.
 *
 * @param base Wthor game base.
 * @param file Game file name.
 */
bool wthor_save(WthorBase *base, const char *file)
{
	FILE *f;
	char path[FILENAME_MAX];
	int r = 0;

	path_get_dir(file, path); strcat(path, "WTHOR.JOU");
	wthor_players_save(base, path);

	if ((f = fopen(file, "wb")) != NULL) {
		wthor_header_set(base->header, base->n_games, 0, 0);
		r = wthor_header_write(base->header, f);
		if (base->game) {
			r += (fwrite(base->game, sizeof (WthorGame), base->n_games, f) == (unsigned) base->n_games);
		}
		if (r != base->n_games) warn("Error while writing %s\n", file);
		fclose(f);
	} else {
		warn("Cannot open file %s\n", file);
	}

	return r;
}

/**
 * @brief Convert to a wthor base.
 *
 * @param base Generic game base.
 * @param wthor Wthor Game base.
 */
bool base_to_wthor(const Base *base, WthorBase *wthor)
{
	bool ok = true;
	int i, j;

	j = wthor->n_games;
	wthor->n_games = j + base->n_games;
	wthor->game = (WthorGame*) realloc(wthor->game, wthor->n_games * sizeof (WthorGame));
	if (wthor->game) {
		for (i = 0; i < base->n_games; ++i, ++j) {
			game_to_wthor(base->game + i, wthor->game + j);
			wthor->game[j].black = wthor_player_get(wthor, base->game[i].name[BLACK]); 
			wthor->game[j].white = wthor_player_get(wthor, base->game[i].name[WHITE]); 
		}
	} else {
		warn("Cannot allocate wthor games\n");
		ok = false;
	}

	return ok;
}

/**
 * @brief print a wthor game.
 *
 * @param base Wthor game base.
 * @param i game intex
 * @param f output stream.
 */
void wthor_print_game(WthorBase *base, int i, FILE *f)
{
	Game game[1];

	if (0 <= i && i < base->n_games) {

		fprintf(f, "Game #%d: %s: %4d - %s vs. %s: ",
			i, base->tournament[base->game[i].tournament],
			base->header->game_year,
			base->player[base->game[i].black],
			base->player[base->game[i].white]);

		wthor_to_game(base->game + i, game);
		game_export_text(game, f);

		fprintf(f, "Theoric score %d empties : %+02d, ", base->header->depth, base->game[i].theoric_score);
		fprintf(f, "Score final : %+02d (as black disc count.)\n", base->game[i].score);
	}
}

/**
 * @brief Get a position from a Wthor game.
 *
 * @param game Wthor game.
 * @param n_empties number of empty square of the position to find.
 * @param board Board found.
 * @param player
 */
static void wthorgame_get_board(WthorGame *game, const int n_empties, Board *board, int *player)
{
	int i;
	Move move[1];
	char s_move[4];

	*player = BLACK; board_init(board);
	for (i = 0; i < 60 - n_empties && game->x[i]; ++i) {
		if (board_is_pass(board)) {
			board_pass(board); *player ^= 1;
		}
		board_get_move(board, move_from_wthor(game->x[i]), move);
		if (board_check_move(board, move)) {
			board_update(board, move); *player ^= 1;
		} else {
			warn("Illegal move %s\n", move_to_string(move->x, *player, s_move));
			break;
		}
	}
}

/**
 * @brief Verify that a PV does not contain errors.
 *
 * @param init_board Initial board.
 * @param pv PV to check
 * @param search Search engine.
 */
int pv_check(const Board *init_board, Line *pv, Search *search)
{
	Game game[1];

	line_to_game(init_board, pv, game);
	return game_analyze(game, search, board_count_empties(init_board), false);
}

/**
 * @brief Test Search with a wthor base.
 *
 * @param file Game File.
 * @param search Search.
 */
void wthor_test(const char *file, Search *search)
{
	WthorBase base[1];
	WthorGame *wthor;
	Board board[1];
	int player;
	int score;
	int n_empties;
	int n_failure;
	long long n_nodes;
	long long t;
	int n_err;

	if (wthor_load(base, file)) {

		if (search->options.verbosity == 1) {
			if (search->options.header) puts(search->options.header);
			if (search->options.separator) puts(search->options.separator);
		}

		n_failure = 0;
		n_nodes = 0;
		t = 0;

		foreach_wthorgame(wthor, base) {
			wthorgame_get_board(wthor, base->header->depth, board, &player);
			n_empties = board_count_empties(board);
			if (n_empties != base->header->depth && !board_is_game_over(board)) {
				warn("Incomplete or Illegal game: %d empties\n", n_empties);
				wthor_print_game(base, wthor - base->game, stderr);
				continue;
			}

			if (player == WHITE) score = 64 - 2 * wthor->theoric_score;
			else score = 2 * wthor->theoric_score - 64;
			if (abs(score) > 64) {
				warn("Impossible theoric score:\n");
				wthor_print_game(base, wthor - base->game, stderr);
				continue;
			}

			search_cleanup(search);
			search_set_board(search, board, player);
			search_set_level(search, 60, base->header->depth);
			search_run(search);
			if (search->options.verbosity) putchar('\n');
			n_nodes += search->result->n_nodes;
			t += search->result->time;
			if (score != search->result->score) {
				warn("Wrong theoric score: %+d (Wthor) instead of %+d (Edax)\n", score, search->result->score);
				wthor_print_game(base, wthor - base->game, stderr);
				++n_failure;
				assert(false); // stop here when debug is on
			}

			if (options.pv_check) {
				Line pv;
				line_copy(&pv, search->result->pv, 0);
				n_err = pv_check(board, &pv, search);
				if (n_err) {
					char s[80];
					warn("Wrong pv:\n");
					board_print(board, player, stderr);
					fprintf(stderr, "setboard %s\nplay ", board_to_string(board, player, s));
					line_print(&pv, 200, " ", stderr);
					putc('\n', stderr); putc('\n', stderr);
					assert(false); // stop here when debug is on
				}
			}

			if (search->options.verbosity == 0) {
				printf("%s  game: %4d, error: %2d ; ", file, (int)(wthor - base->game) + 1, n_failure);
				printf("%lld n, ", n_nodes); time_print(t, false, stdout); putchar('\r');
				fflush(stdout);
			}
		}
		if (search->options.verbosity == 1) {
			if (search->options.separator) puts(search->options.separator);
		}
		putchar('\n');

		wthor_free(base);
	}
	return;
}

/**
 * @brief Test Eval with a wthor base.
 *
 * Given a wthor file, compare the result of a search to the theoretical scores.
 *
 * @param file Game File.
 * @param search Search.
 * @param histogram output array.
 */
void wthor_eval(const char *file, Search *search, unsigned long long histogram[129][65])
{
	WthorBase base[1];
	WthorGame *wthor;
	Board board[1];
	int player;
	int score;
	int n_empties;

	if (wthor_load(base, file)) {
		foreach_wthorgame(wthor, base) {
			wthorgame_get_board(wthor, base->header->depth, board, &player);
			n_empties = board_count_empties(board);
			if (n_empties != base->header->depth && !board_is_game_over(board)) {
				continue;
			}

			if (player == WHITE) score = 64 - 2 * wthor->theoric_score;
			else score = 2 * wthor->theoric_score - 64;
			if (abs(score) > 64) {
				continue;
			}

			search_cleanup(search);
			search_set_board(search, board, player);
			search_set_level(search, options.level, base->header->depth);
			search_run(search);
			++histogram[search->result->score + 64][(score + 64) / 2];
		}
		wthor_free(base);
	}
	return;
}

/**
 * @brief Change players to "Edax (delorme)" and tourney to "Etudes"
 * in a wthor base.
 *
 * @param file Wthor game file.
 */
void wthor_edaxify(const char *file)
{
	WthorBase base[1];
	WthorGame *wthor;

	if (wthor_load(base, file)) {
		foreach_wthorgame(wthor, base) {
			wthor->black = 1368; // "Edax (delorme)"
			wthor->white = 1368; // "Edax (delorme)"
			wthor->tournament = 157; // "Etudes"
		}
		wthor_save(base, file);
		wthor_free(base);
	}
}

/**
 * @brief Initialize a game database.
 *
 * @param base Game base.
 */
void base_init(Base *base)
{
	base->size = 0;
	base->n_games = 0;
	base->game = NULL;
}

/**
 * @brief Free resources of a game database.
 *
 * @param base Game base.
 */
void base_free(Base *base)
{
	free(base->game);
	base_init(base);
}

/**
 * @brief Add a game to a game database.
 *
 * @param base Game base.
 * @param game Game to add.
 */
void base_append(Base *base, const Game *game)
{
	if (base->n_games == base->size) {
		Game *ptr;
		int size = base->size;

		if (size == 0) size = 16384;
		else size *= 2;
		ptr = (Game*) realloc(base->game, size * sizeof (Game));
		if (ptr == NULL) {
			error("cannot reallocate base game");
			return;
		}
		base->game = ptr;
		base->size = size;
	}
	base->game[base->n_games++] = *game;
}

/**
 * @brief Make games unique in the game database.
 *
 * @param base Game base.
 */
void base_unique(Base *base)
{
	int i, j, k;
	bool found;

	for (i = k = 0; i < base->n_games; ++i) {
		for (j = 0, found = false; j < k && !found; ++j)
			found = game_equals(base->game + j, base->game + i);
		if (!found) base->game[k++] = base->game[i];
	}

	base->n_games = k;
}

/**
 * @brief Load a game database.
 *
 * @param base Game base.
 * @param file Game filename.
 */
bool base_load(Base *base, const char *file)
{
	void (*load)(Game*, FILE*) = game_import_text;
	Game game[1];
	FILE *f;
	char ext[8];
	int l;
	WthorHeader header[1];

	l = strlen(file); strcpy(ext, file + l - 4); string_to_lowercase(ext);
	if (strcmp(ext, ".txt") == 0) load = game_import_text;
	else if (strcmp(ext, ".ggf") == 0) load = game_import_ggf;
	else if (strcmp(ext, ".sgf") == 0) load = game_import_sgf;
	else if (strcmp(ext, ".pgn") == 0) load = game_import_pgn;
	else if (strcmp(ext, ".wtb") == 0) load = game_import_wthor;
	else if (strcmp(ext, ".edx") == 0) load = game_read;
	else {
		warn("Unknown game format extension: %s\n", ext);
		return false;
	}

	if (load == game_import_wthor) f = fopen(file, "rb");
	else f = fopen(file, "r");
	if (f == NULL) {
		warn("Cannot open file %s\n", file);
		return false;
	}

	info("loading games...");
	if (load == game_import_wthor) wthor_header_read(header, f);
	for (;;) {
		load(game, f);
		if (ferror(f) || feof(f)) break;
		base_append(base, game);
	}
	info("done (%d games loaded)\n", base->n_games);

	fclose(f);

	return base->n_games > 0;
}

/**
 * @brief Save a game database.
 *
 * @param base Game base.
 * @param file Game filename.
 */
void base_save(const Base *base, const char *file)
{
	void (*save)(const Game*, FILE*)= game_export_text;
	FILE *f;
	char ext[8];
	int i, l;
	WthorBase wbase;
	Base old;

	l = strlen(file); strcpy(ext, file + l - 4); string_to_lowercase(ext);
	if (strcmp(ext, ".txt") == 0) save = game_export_text;
	else if (strcmp(ext, ".ggf") == 0) save = game_export_ggf;
	else if (strcmp(ext, ".sgf") == 0) save = game_export_sgf;
	else if (strcmp(ext, ".pgn") == 0) save = game_export_pgn;
	else if (strcmp(ext, ".wtb") == 0) {
		wthor_load(&wbase, file);
		base_to_wthor(base, &wbase);
		wthor_save(&wbase, file);
		wthor_free(&wbase);
		return;
	} else if (strcmp(ext, ".edx") == 0) save = game_write;
	else {
		warn("Unknown game format extension: %s\n", ext);
		return;
	}

	base_init(&old);
	base_load(&old, file);
	for (i = 0; i < base->n_games; ++i) {
		base_append(&old, base->game + i);
	}

	f = fopen(file, "w");
	if (f == NULL) {
		warn("Cannot open file %s\n", file);
		return;
	}
	for (i = 0; i < old.n_games && !ferror(f); ++i) {
		save(old.game + i, f);
	}

	fclose(f);

}


/**
 * @brief Convert a game database to a set of problems.
 *
 * @param base Game base.
 * @param n_empties Number of empties.
 * @param problem Problems filename.
 */
void base_to_problem(Base *base, const int n_empties, const char *problem)
{
	int i;
	Board board[1];
	char s[80];
	FILE *f;

	f = fopen(problem, "w");

	for (i = 0; i < base->n_games; ++i) {
		if (game_get_board(base->game + i, 60 - n_empties, board)) {
			board_to_string(board, n_empties & 1, s);
			fprintf(f, "%s\n", s);
		}
	}

	fclose(f);
}

/**
 * @brief Convert a game database to a set of problems.
 *
 * @param base Game base.
 * @param n_empties Number of empties.
 * @param problem Problems filename.
 */
void base_to_FEN(Base *base, const int n_empties, const char *problem)
{
	int i;
	Board board[1];
	FILE *f;

	f = fopen(problem, "w");

	for (i = 0; i < base->n_games; ++i) {
		if (game_get_board(base->game + i, 60 - n_empties, board)) {
			board_print_FEN(board, n_empties & 1, f);
			putc('\n', f);
		}
	}

	fclose(f);
}

/**
 * @brief Base analysis.
 *
 * @param base Game base.
 * @param search Search engine.
 * @param n_empties Number of empties.
 * @param apply_correction Correct bad moves from the games.
 */
void base_analyze(Base *base, Search *search, const int n_empties, const bool apply_correction)
{
	int i;
	int n_error;

	for (i = 0; i < base->n_games; ++i) {
		if (game_score(base->game + i) == 0) continue;
		game_export_text(base->game + i, stdout);
		n_error = game_analyze(base->game + i, search, n_empties, apply_correction);
		if (n_error) {
			printf("Game #%d contains %d errors", i, n_error);
			if (apply_correction) {
				if (game_analyze(base->game + i, search, n_empties, false)) printf("... correction failed! ***BUG DETECTED!***\n");
				else printf("... corrected!\n");
			} else putchar('\n');
		}
		printf("%d/%d %.1f %% done.\r", i + 1, base->n_games, 100.0 * (i + 1) / base->n_games); fflush(stdout);
	}
}

/**
 * @brief Base analysis.
 *
 * @param base Game base.
 * @param search Search engine.
 */
void base_complete(Base *base, Search *search)
{
	int i, n;
	int completed;

	for (i = n = 0; i < base->n_games; ++i) {
		completed = 0;
		if (game_complete(base->game + i, search) > 0) completed = 1;
		n += completed;
		if (completed || (i % 1000) == 0) {
			printf("%d/%d games completed (%.1f %% done).\r", n, i + 1, 100.0 * (i + 1) / base->n_games); fflush(stdout);
		}
	}
	printf("%d/%d games completed (all done).          \n", n, i + 1);
}

/**
 * @brief Base Compare.
 *
 * Display the number of positions two base files have in common. 
 *
 * @param file_1 Game base file.
 * @param file_2 Game base file.
 */
void base_compare(const char *file_1, const char *file_2)
{
	Base base_1[1], base_2[2];
	PositionHash hash[1];
	Board board[1];
	int i, j;
	long long n_1, n_2, n_2_only;

	base_init(base_1);
	base_init(base_2);
	n_1 = 0;
	n_2 = 0;
	n_2_only = 0;

	base_load(base_1, file_1);
	positionhash_init(hash, options.hash_table_size);
	for (i = 0; i < base_1->n_games; ++i) {
		Game *game = base_1->game + i;
		*board = *game->initial_board;
		for (j = 0; j < 60 && game->move[j] != NOMOVE; ++j) {
			if (!game_update_board(board, game->move[j])) break; // BAD MOVE -> end of game
			if (positionhash_append(hash, board)) {
				++n_1;
			}
		}
	}
	base_free(base_1);

	base_load(base_2, file_2);
	for (i = 0; i < base_2->n_games; ++i) {
		Game *game = base_2->game + i;
		*board = *game->initial_board;
		for (j = 0; j < 60 && game->move[j] != NOMOVE; ++j) {
			if (!game_update_board(board, game->move[j])) break; // BAD MOVE -> end of game
			if (positionhash_append(hash, board)) {
				++n_2_only;
			}
		}
	}

	positionhash_delete(hash);
	positionhash_init(hash, options.hash_table_size);
	for (i = 0; i < base_2->n_games; ++i) {
		Game *game = base_2->game + i;
		*board = *game->initial_board;
		for (j = 0; j < 60 && game->move[j] != NOMOVE; ++j) {
			if (!game_update_board(board, game->move[j])) break; // BAD MOVE -> end of game
			if (positionhash_append(hash, board)) {
				++n_2;
			}
		}
	}
	base_free(base_2);

	positionhash_delete(hash);

	printf("%s : %lld positions - %lld original positions\n", file_1, n_1, n_1 - (n_2- n_2_only));
	printf("%s : %lld positions - %lld original positions\n", file_2, n_2, n_2_only);
	printf("%lld common positions\n", n_2-n_2_only);
}

