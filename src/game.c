/**
 * @file game.c
 *
 * Game management
 *
 * @date 1998 - 2017
 * @author Richard Delorme
 * @version 4.4
 */

#include "bit.h"
#include "board.h"
#include "const.h"
#include "game.h"
#include "search.h"
#include "util.h"

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

/** error values */
enum {
	PARSE_OK = 0,
	PARSE_END_OF_FILE = 1,
	PARSE_INVALID_TAG = 2,
	PARSE_INVALID_VALUE = 3
};

/**
 * @brief Coordinates conversion from wthor to edax.
 *
 * @param x wthor coordinate.
 * @return edax coordinate.
 */
int move_from_wthor(int x)
{
	return 8 * ((x - 11) / 10) + ((x - 11) % 10);
}

/**
 * @brief Coordinates conversion from edax to wthor.
 *
 * @param x edax coordinate.
 * @return wthor coordinate.
 */
static int move_to_wthor(int x)
{
	return 10 * (x / 8) + (x % 8) + 11;
}

/**
 * @brief Coordinates conversion from oko
 *
 * allinf.oko is an old base of games between kitty & early logistello.
 *
 * @param x oko coordinate.
 * @return edax coordinate.
 */
static int move_from_oko(int x)
{
	static const char oko_to_edax[]={
		0,
		A1,B1,C1,D1,E1,F1,G1,H1,
		A2,B2,C2,D2,E2,F2,G2,H2,
		A3,B3,C3,D3,E3,F3,G3,H3,
		A4,B4,C4,      F4,G4,H4,
		A5,B5,C5,      F5,G5,H5,
		A6,B6,C6,D6,E6,F6,G6,H6,
		A7,B7,C7,D7,E7,F7,G7,H7,
		A8,B8,C8,D8,E8,F8,G8,H8
	};
	return oko_to_edax[x & 0x3f];
}

/**
 * @brief Create an empty game
 *
 * @param game Game.
 */
void game_init(Game *game)
{
	char name[2] = "?";
	board_init(game->initial_board);
	memset(game->move, NOMOVE, 60);
	game->player = BLACK;
	memcpy(game->name[0], name, 2);
	memcpy(game->name[1], name, 2);
	game->date.year = game->date.month = game->date.day = 0;;
	game->date.hour = -1; game->date.minute = game->date.second = 0;;
	game->hash = 0ULL;
}

/**
 * @brief Game copy.
 *
 * TODO: check if parameter's order is consistant all over the program
 * @param dest Destination Game.
 * @param src Source Game.
 */
void game_copy(Game *dest, const Game *src)
{
	*dest = *src;
}

/**
 * @brief  Test if two games are equal.
 *
 * @param game_1 First Game.
 * @param game_2 Second Game.
 */
bool game_equals(const Game *game_1, const Game *game_2)
{
	int i;
	if (game_1->hash == game_2->hash
	 && game_1->date.year == game_2->date.year && game_1->date.month == game_2->date.month && game_1->date.day == game_2->date.day
	 && game_1->date.hour == game_2->date.hour && game_1->date.minute == game_2->date.minute && game_1->date.second == game_2->date.second
	 && strcmp(game_1->name[0], game_2->name[0]) == 0 && strcmp(game_1->name[1], game_2->name[1]) == 0) {
		for (i = 0; i < 60; ++i) {
			if (game_1->move[i] != game_2->move[i]) return false;
		}
		return true;
	} else {
		return false;
	}
}

/**
 * @brief Test if two Wthor games are equal.
 *
 * @param game_1 First Game.
 * @param game_2 Second Game.
 */
bool wthor_equals(const WthorGame *game_1, const WthorGame *game_2)
{
	int i;
	if (game_1->black == game_2->black && game_1->white ==game_2->white && game_1->tournament == game_2->tournament) {
		for (i = 0; i < 60; ++i) {
			if (game_1->x[i] != game_2->x[i]) return false;
		}
		return true;
	} else {
		return false;
	}
}

/**
 * @brief update a board.
 *
 */
bool game_update_board(Board *board, int x)
{
	Move move[1];

	if (x < A1 || x > H8 || board_is_occupied(board, x)) return false;
	if (!can_move(board->player, board->opponent)) {
		board_pass(board);
	}
	if (board_get_move(board, x, move) == 0) return false;
	board_update(board, move);

	return true;
}

/**
 * @brief update a player.
 *
 */
static bool game_update_player(Board *board, int x)
{
	Move move[1];
	bool swap = false;
	
	if (A1 <= x && x <= H8 && !board_is_occupied(board, x)) {
		if (!can_move(board->player, board->opponent)) {
			board_pass(board);
			swap = !swap;
		}
		if (board_get_move(board, x, move) == 0) swap = !swap;
	}
	
	return swap;
}

/**
 * @brief Get the board after 'ply' move.
 *
 * @param game Game.
 * @param ply number of move.
 * @param board output board.
 * @return false if an error occurred.
 */
bool game_get_board(const Game *game, const int ply, Board *board)
{
	int i;

	*board = *game->initial_board;
	for (i = 0; i < ply; i++) {
		if (!game_update_board(board, game->move[i])) return false;
	}

	return true;
}

/**
 * @brief Check a game.
 *
 * @param game Game.
 * @return false if an error occurred.
 */
bool game_check(Game *game)
{
	Board board[1];
	int i;

	*board = *game->initial_board;
	for (i = 0; i < 60 && game->move[i] != NOMOVE; ++i) {
		if (!game_update_board(board, game->move[i])) {
			return false;
		}
	}
	return true;
}

/**
 * @brief Compute the final score of the game, for the initial player.
 *
 * @param game An input game.
 * @return The score of the game.
 */
int game_score(const Game *game)
{
	int n_discs_p, n_discs_o, n_empties;
	int i, player, score;
	Board board[1];

	*board = *game->initial_board;
	player = game->player;
	for (i = 0; i < 60 && game->move[i] != NOMOVE; ++i) {
		player ^= game_update_player(board, game->move[i]);
		if (!game_update_board(board, game->move[i])) {
			return -SCORE_INF;
		}
	}

	if (!board_is_game_over(board)) return -SCORE_INF;

	n_discs_p = bit_count(board->player);
	n_discs_o = bit_count(board->opponent);
	n_empties = 64 - n_discs_p - n_discs_o;
	score = n_discs_p - n_discs_o;

	if (score < 0) score -= n_empties;
	else if (score > 0) score += n_empties;

	if (player == game->player)	return score;
	else return -score;
}

/**
 * @brief Convert a text (ascii) game to a Game 
 * @param line A move sequence in ascii.
 * @param game The output game.
 */
void text_to_game(const char *line, Game *game)
{
	int i;
	Board board[1];
	Move move[1];
	char *s;

	board_init(game->initial_board);
	game_init(game);
	*board = *game->initial_board;
	for (i = 0; i < 60 && *line;) {
		s = parse_move(line, board, move);
		if (s == line && move->x == NOMOVE) return;
		if (move->x != PASS) {
			game->hash ^= hash_move[move->x][i];
			game->move[i++] = move->x;
		}
		board_update(board, move);
		line = s;
	}
}

/**
 * @brief Convert game to a text (ascii).
 * @param game The intput game.
 * @param line A move sequence in ascii.
 */
void game_to_text(const Game *game, char *line)
{
	int i;

	for (i = 0; i < 60 && game->move[i] != NOMOVE; i++) {
		line = move_to_string(game->move[i], BLACK, line) + 2;
	}
	*line = '\0';
}

/**
 * @brief convert an allinf.oko game to a Game.
 *  
 * this was a serie of games played by early M. Buro's logistello
 * against I. Durdanovic's programs.
 *
 * @param oko A single input game.
 * @param game The output game.
 */
void oko_to_game(const OkoGame *oko, Game *game)
{
	int i;
	Board board[1];

	game_init(game);
	*board = *game->initial_board;
	for (i = 0; i < 60; i++) {
		game->move[i] = move_from_oko(oko->move[i]);
		if (!game_update_board(board, game->move[i])) {
			game->move[i] = NOMOVE;
			break;
		}
		game->hash ^= hash_move[(int)game->move[i]][i];
	}
}

/**
 * @brief convert a Wthor game to a Game.
 *  
 * The Wthor format is famous as all main event
 * games are recorded into this format. Its name
 * comes from Sylvain Quin's program.
 *
 * @param thor A single input game.
 * @param game The output game.
 */
void wthor_to_game(const WthorGame *thor, Game *game)
{
	int i;
	Board board[1];

	game_init(game);
	*board = *game->initial_board;
	for (i = 0; i < 60; i++) {
		game->move[i] = move_from_wthor(thor->x[i]);
		if (!game_update_board(board, game->move[i])) {
			game->move[i] = NOMOVE;
			break;
		}
		game->hash ^= hash_move[(int)game->move[i]][i];
	}
}

/**
 * @brief convert a Game to a Whor game.
 *  
 * The wthor format is famous as all main event
 * games are recorded into this format.
 *
 * @param game An input game.
 * @param thor The wthor output game.
 */
void game_to_wthor(const Game *game, WthorGame *thor)
{
	int i;

	for (i = 0; i < 60; i++) {
		thor->x[i] = move_to_wthor(game->move[i]);
	}
	thor->black = 1368; // edax
	thor->white = 1368; // edax
	thor->tournament = 0;//g->tournament;
	/* TODO: */
	thor->score = 32 + game_score(game) / 2;
	thor->theoric_score = thor->score; // may be wrong?
}

/** 
 * @brief Build a game from an initial position and a move sequence.
 *
 * @param game Game.
 * @param line Move sequence.
 * @param from Game stage from where to append the line.
 */
void game_append_line(Game *game, const Line *line, const int from)
{
	int i, j;
	Board board[1];

	if (game_get_board(game, from, board)) {
		for (i = 0, j = from; i < line->n_moves && j < 60; ++i) {
			if (line->move[i] != PASS) {
				if (game_update_board(board, line->move[i])) {
					game->hash ^= hash_move[(int)line->move[i]][j];
					game->move[j++] = line->move[i];
				} else {
					break;
				}
			}
		}
		for (; j < 60; ++j) game->move[j] = NOMOVE;
	}
}

/** 
 * @brief Build a game from an initial position and a move sequence.
 *
 * @param initial_board Initial board.
 * @param line Move sequence.
 * @param game The output game.
 */
void line_to_game(const Board *initial_board, const Line *line, Game *game)
{
	game_init(game);
	*game->initial_board = *initial_board;
	game->player = line->color;
	game_append_line(game, line, 0);
}

/**
 * @brief Read a game from a binary file 
 *
 * @param game The output game.
 * @param f The file stream.
 */
void game_read(Game *game, FILE *f)
{
	if (fread(game, sizeof (Game), 1, f) == 0) game_init(game);
}

/**
 * @brief Write a game to a binary file 
 *
 * @param game The input game.
 * @param f The file stream.
 */
void game_write(const Game *game, FILE *f)
{
	fwrite(game, sizeof (Game), 1, f);
}

/**
 * @brief Read a game from a text file 
 *
 * @param game The output game.
 * @param f The file stream.
 */
void game_import_text(Game *game, FILE *f)
{
	char *line = string_read_line(f);

	if (line) text_to_game(line, game);
	else  game_init(game);

	free(line);
}

/**
 * @brief Write a game to a text file 
 *
 * @param game The input game.
 * @param f The file stream.
 */
void game_export_text(const Game *game, FILE *f)
{
	char s_game[128], s_board[80];
	Board board[1];

	board_init(board);
	if (!board_equal(board, game->initial_board)) {
		board_to_string(game->initial_board, game->player, s_board);
		fprintf(f, "%s;", s_board);
	}
	game_to_text(game, s_game);
	fprintf(f, "%s\n", s_game);
}

/**
 * @brief Read a game from a Wthor file 
 *
 * @param game The output game.
 * @param f The file stream.
 */
void game_import_wthor(Game *game, FILE *f)
{
	WthorGame thor;
	if (fread(&thor, sizeof (WthorGame), 1, f) == 1) wthor_to_game(&thor, game);
	else game_init(game);
}

/**
 * @brief Write a game to a Wthor file 
 *
 * @param game The input game.
 * @param f The file stream.
 */
void game_export_wthor(const Game *game, FILE *f)
{
	WthorGame thor;
	game_to_wthor(game, &thor);
	fwrite(&thor, sizeof (WthorGame), 1, f);
}

/**
 * @brief Read a game from the "allinf.oko" file 
 *
 * @param game The output game.
 * @param f The file stream.
 */
void game_import_oko(Game *game, FILE *f)
{
	OkoGame oko;
	if (fread(&oko, sizeof (OkoGame), 1, f) == 1) oko_to_game(&oko, game);
	else game_init(game);
}

/**
 * @brief Parse a ggf game.
 *
 * From the current input stream, fill a tag/value pair.
 *
 * @param f The file stream.
 * @param tag The tag field.
 * @param value The value field.
 */
static int game_parse_ggf(FILE *f, char *tag, char *value)
{
	int i, c='\0';

	tag[0] = tag[1] = tag[2] = '\0';
	value[0] = '\0';

	for (i = 0; i < 3; i++) {
		c=fgetc(f);
		if (c==EOF) return PARSE_END_OF_FILE;
		else  if (c==' ' || c=='\n' || c=='\r' || c=='\t') {
			i--;
			continue;
		}
		else if (c == '[') break;
		else if ('A' <= c && c <= 'Z') tag[i] = (char)c;
		else if (i == 0 && (c == '(' || c == ';')) {
 			tag[0] = (char)c;
 			c = fgetc(f);
			if ((tag[0] == '(' && c == ';') || (tag[0] == ';' && c == ')')) {
				tag[1] = (char)c;
				tag[2] = '\0';
				return PARSE_OK;
			} else return PARSE_INVALID_TAG;
		} else  return  PARSE_INVALID_TAG;
	}
	if (c != '[') return PARSE_INVALID_TAG;
	tag[i] = '\0';

	for (i = 0; i < 1000; i++) {
		c = fgetc(f);
		if (c == EOF) return PARSE_END_OF_FILE;
		if (c == ']') break;
		value[i] = tolower(c);
	}
	value[i]='\0';

	if (i == 1000) {
		for (i = 0; ; i++) {
			c = fgetc(f);
			if (c == EOF) return PARSE_END_OF_FILE;
			if (c == ']') break;
		}
	}
	return PARSE_OK;
}

/**
 * @brief Read a game from the Generic Game Format (ggf) file.
 *
 * @param game The output game.
 * @param f The file stream.
 */
void game_import_ggf(Game* game, FILE* f)
{
	char tag[4], value[1000];
	int i = 0;

	game_init(game);
	while (game_parse_ggf(f, tag, value) != PARSE_END_OF_FILE && strcmp(tag, "(;") != 0) ;
	if (strcmp(tag, "(;") == 0) {
		while (game_parse_ggf(f, tag, value) == PARSE_OK) {
			if (strcmp(tag,";)")==0) {
				if (!game_check(game)) {
					warn("error while importing a GGF game\n");
				}
				return;
			}
			if (strcmp(tag,"GM") == 0 && strcmp(value, "othello") != 0) break;
			if (strcmp(tag, "BO") == 0) {
				if (value[0] != '8') break;
				game->player = board_set(game->initial_board, value + 2);
			} else if (strcmp(tag, "PB") == 0) {
				memcpy(game->name[BLACK], value, 31);
				game->name[BLACK][31] = '\0';
			} else if (strcmp(tag, "DT") == 0) {
				int v[6]; // hack for windows that does not support %hhd in sscanf
				sscanf(value, "%d.%d.%d_%d:%d:%d", v, v + 1, v + 2, v + 3, v + 4, v + 5);
				game->date.year = v[0]; game->date.month = v[1]; game->date.day = v[2];
				game->date.hour = v[3]; game->date.minute = v[4]; game->date.second = v[5];
		} else if (strcmp(tag, "PW") == 0) {
				memcpy(game->name[WHITE], value, 31);
				game->name[WHITE][31] = '\0';
			} else if (i < 60 && (strcmp(tag, "B") == 0 || strcmp(tag, "W") == 0)) {
				if (strncmp("pa", value, 2) == 0) continue;
				game->move[i] = string_to_coordinate(value);
				game->hash ^= hash_move[(int)game->move[i]][i];
				i++;
			}
		}
		while (game_parse_ggf(f, tag, value) != PARSE_END_OF_FILE && strcmp(tag,";)") != 0) ;
	}
	return;
}

/**
 * @brief Parse a Tag/value ggf pair from a string.
 *
 * @param string An input string.
 * @param tag The tag field.
 * @param value The value field.
 * @return The unprocessed remaining part of the string.
 */
static const char* parse_tag(const char *string, char *tag, char *value)
{
	const char *s;
	int n;

	s = parse_skip_spaces(string);
	if ((s[0] == '(' && s[1] == ';') || (s[0] == ';' && s[1] == ')')) {
		tag[0] = *s++;
		tag[1] = *s++;
		tag[2] = *value = '\0';
	} else {
		n = 3; while (*s && *s != '[' && n--) *tag++ = toupper(*s++);
		*tag = '\0';
		if (*s == '[') {
			++s;
			n = 255; while (*s && *s != ']' && n--) *value++ = tolower(*s++);
			if (*s == ']') ++s;
			else s = string;
		} else s = string;
		*value = '\0';
	}

	return s;
}

/**
 * @brief Parse a ggf game from a string.
 *
 * @param game The output game.
 * @param string An input string.
 * @return The unprocessed remaining part of the string.
 */
char* parse_ggf(Game *game, const char *string)
{
	const char *s = string;
	const char *next;
	char tag[4], value[256];
	int i = 0;

	game_init(game);

	while ((next = parse_tag(s, tag, value)) != s && strcmp(tag, "(;") != 0) s = next;

	if (strcmp(tag, "(;") == 0) {
		s = next;
		while ((next = parse_tag(s, tag, value)) != s && strcmp(tag, ";)") != 0) {
			s = next;

			if (strcmp(tag, "GM") == 0 && strcmp(value, "othello") != 0) {
				s = string;
				break;
			} else if (strcmp(tag, "BO") == 0) {
				if (value[0] != '8') {
					s = string;
					break;
				}
				game->player = board_set(game->initial_board, value + 2);
			} else if (strcmp(tag, "PB") == 0) {
				memcpy(game->name[BLACK], value, 31);
				game->name[BLACK][31] = '\0';
			} else if (strcmp(tag, "PW") == 0) {
				memcpy(game->name[WHITE], value, 31);
				game->name[WHITE][31] = '\0';
			} else if (i < 60 && (strcmp(tag, "B") == 0 || strcmp(tag, "W") == 0)) {
				if (strncmp("pa", value, 2) == 0) continue;
				game->move[i++] = string_to_coordinate(value);
			}
		}
	}

	if (!game_check(game)) s = string;

	return (char*) s;
}

/**
 * @brief Write a game to the Generic Game Format (ggf) file.
 *
 * @param game The output game.
 * @param f The file stream.
 */
void game_export_ggf(const Game *game, FILE *f)
{
	Board board[1];
	int player;
	static const char board_color[] = "*O-?";
	int i, x, square;
//	time_t t:
//	struct tm *date;
	static const char move_color[] = "BW";
	char move_string[3] = "xx";

	fputs("(;GM[othello]PC[Edax]", f);
//	t = time(NULL); 
//	date = localtime(&t);
//	fprintf(f, "DT[%d.%2d.%2d_%2d:%2d:%2d]", date->tm_year + 1900, date->tm_mon + 1, date->tm_mday, date->tm_hour, date->tm_min, date->tm_sec);
	fprintf(f, "PB[%s]PW[%s]", game->name[BLACK], game->name[WHITE]);
	fprintf(f, "RE[%+d.000]", game_score(game));
	fputs("BO[8 ", f);
	for (x = 0; x < 64; ++x) {
		if (game->player == BLACK) square = 2 - ((game->initial_board->opponent >> x) & 1) - 2 * ((game->initial_board->player >> x) & 1);
		else square = 2 - ((game->initial_board->player >> x) & 1) - 2 * ((game->initial_board->opponent >> x) & 1);
		putc(board_color[square], f); if ((x & 7) == 7) putc(' ', f);
	}
	putc(board_color[(int) game->player], f); fputc(']', f);

	*board = *game->initial_board;
	player = game->player;
	for (i = 0; i < 60 && game->move[i] != NOMOVE; ++i) {
		if (!can_move(board->player, board->opponent)) {
			fprintf(f, "%c[PA]", move_color[player]);
			player = !player;
		}
		if (game_update_board(board, game->move[i])) {
			fprintf(f, "%c[%s]", move_color[player], move_to_string(game->move[i], 0, move_string));
			player = !player;
		}
	}
	fputs(";)\n", f);
}

/**
 * @brief Parse a Smart Game Format (sgf) game.
 *
 * From the current input stream, fill a tag/value pair.
 *
 * @param f The file stream.
 * @param tag The tag field.
 * @param value The value field.
 */
static int game_parse_sgf(FILE *f, char *tag, char *value)
{
	int i, c = '\0';

	tag[0] = tag[1] = tag[2] = '\0';
	value[0] = '\0';

	for (i = 0; i < 3; i++) {
		c = fgetc(f);
		if (c == EOF) return 0;
		 else  if (c == ' ' || c == '\n' || c == '\r' || c == '\t' || c == ';') {
			i--;
			continue;
		}
		 else  if (c == '[') break;
		 else  if ('A' <= c && c <= 'Z') tag[i] = (char)c;
		 else  if (i == 0 && (c == '(' || c == ')')) {
 			tag[0] = (char)c;
 			tag[1] = '\0';
			return 1;
		}
		 else  return 0;

	}
	if (c != '[') return 0;
	tag[i] = '\0';

	for (i = 0; i < 1000; i++) {
		c = fgetc(f);
		if (c == EOF) return 0;
		if (c == ']') break;
		if (c == '\\') {
			c = fgetc(f);
			if (c == EOF) return 0;
		}
		value[i] = (char)c;
	}
	if (i < 1000) {
		if (c != ']') return 0;
		value[i] = '\0';
	} else {
		for (i = 0; ; i++) {
			c = fgetc(f);
			if (c == EOF) return 0;
			if (c == '\\') {
				c = fgetc(f);
				if (c == EOF) return 0;
			}
			if (c == ']') break;
		}
	}

	return 1;
}

/**
 * @brief Read a game from a sgf file.
 *
 * @param game The output game.
 * @param f The file stream.
 */
void game_import_sgf(Game *game, FILE *f)
{
	char tag[4], value[1000];
	int i = 0, level = 1;

	game_parse_sgf(f, tag, value);
	game_init(game);
	if (strcmp(tag, "(") == 0) {
		while (game_parse_sgf(f, tag, value)) {
			if (strcmp(tag,"(") == 0) level++;
			if (strcmp(tag,")") == 0) {
				level--;
				if (!game_check(game)) {
					warn("error while importing a SGF game\n");
				}
				return;
			}
			if (strcmp(tag, "GM") == 0 && strcmp(value, "2") != 0) break;
			if (strcmp(tag, "SZ") == 0 && strcmp(value, "8") != 0) break;
			if (strcmp(tag, "PB") == 0) {
				memcpy(game->name[BLACK], value, 31);
				game->name[BLACK][31] = '\0';
			} else if (strcmp(tag, "PW") == 0) {
				memcpy(game->name[WHITE], value, 31);
				game->name[WHITE][31] = '\0';
			} else if (i < 60 && (strcmp(tag,"B") == 0 || strcmp(tag, "W") == 0)) {
				game->move[i] = string_to_coordinate(value);
				game->hash ^= hash_move[(int)game->move[i]][i];
				i++;

			}
		}
		while (level > 0 && game_parse_sgf(f, tag, value)) {
			if (strcmp(tag,"(") == 0) level++;
			if (strcmp(tag,")") == 0) level--;
		}
	}
	return;
}

/**
 * @brief Write a game to the Generic Game Format (ggf) file.
 *
 * @param game The output game.
 * @param multiline A flag to create a long text or a single line.
 * @param f The file stream.
 */
void game_save_sgf(const Game *game, FILE *f, const bool multiline)
{
	Board board[1];
	int player;
	static const char color[2] = {'B', 'W'};
	char s[8];
	int i;
	time_t t = time(NULL);
	struct tm *date = localtime(&t);
	unsigned long long black, white;
	const int score = game_score(game);

	// game info
	fprintf(f, "(;FF[4]GM[2]AP[edax:%s]", VERSION_STRING);
	if (multiline) fputc('\n',f);
	fputs("PC[Edax]", f);
	fprintf(f,"DT[%04d-%02d-%02d]", 1900 + date->tm_year, 1 + date->tm_mon, date->tm_mday);
	if (multiline) fputc('\n',f);
	fprintf(f, "PB[%s]PW[%s]", game->name[BLACK], game->name[WHITE]);
	if (multiline) fputc('\n',f);

	// initial board
	if (game->player == BLACK) {
		black = game->initial_board->player;
		white = game->initial_board->opponent;
	} else {
		white = game->initial_board->player;
		black = game->initial_board->opponent;
	}
	fputs("SZ[8]",f);
	if (black) {
		fputs("AB", f);
		for (i = A1; i <= H8; i++){
			if (black & x_to_bit(i)) fprintf(f, "[%s]", move_to_string(i, WHITE, s));
		}
	}
	if (white) {
		fputs("AW", f);
		for (i = A1; i <= H8; i++){
			if (white & x_to_bit(i)) fprintf(f, "[%s]", move_to_string(i, WHITE, s));
		}
	}
	fprintf(f, "PL[%c]", color[(int) game->player]);
	if (multiline) fputc('\n', f);

	/* score */
	if (score >= SCORE_MIN) {
		if (score > 0) fprintf(f, "RE[%c%+d]", color[(int) game->player], score);
		else if (score < 0) fprintf(f, "RE[%c%+d]", color[(int) !game->player], -score);
		else fputs("RE[0]", f);
	} else {
		fputs("RE[Void]", f);
	}
	if (multiline) fputc('\n', f);

	/* moves */
	*board = *game->initial_board;
	player = game->player;
	for (i = 0; i < 60 && game->move[i] != NOMOVE; ++i) {
		if (!can_move(board->player, board->opponent)) {
			fprintf(f, "%c[PA];", color[player]);
			player = !player;
			if (multiline && player == game->player) fputc('\n', f);
		}
		if (game_update_board(board, game->move[i])) {
			fprintf(f, "%c[%s];", color[player], move_to_string(game->move[i], WHITE, s));
			player = !player;
			if (multiline && player == game->player) fputc('\n', f);
		}
	}
	/*end */
	fputs(")\n", f);
}

void game_export_sgf(const Game *game, FILE *f)
{
	game_save_sgf(game, f, false);
}

/**
 * @brief Read a game from a pgn file.
 *
 * @param game The output game.
 * @param f The file stream.
 */
void game_import_pgn(Game *game, FILE *f)
{
	int c, state, i, j, k, n;
	char move[5] = "--\0\0";
	int score[2] = {0, 0};
	char info_tag[257] = "", info_value[257] = ""; // 256 is the maximal info size according to PGN standard
	int value[3];
	const int info_size = 256;
	
	enum {
		STATE_START,
		STATE_BEGIN_INFO,
		STATE_END_INFO,
		STATE_BEGIN_VALUE,
		STATE_END_VALUE,
		STATE_BEGIN_MOVE_N,
		STATE_END_MOVE_N,
		STATE_BEGIN_MOVE,
		STATE_END_MOVE,
		STATE_BEGIN_SCORE,
		STATE_END_SCORE,
		STATE_END_GAME
	};

	game_init(game);
	state = STATE_START;
	i = j = k = 0;
	while(state != STATE_END_GAME) {
		c = getc(f);
		putchar(c);
		if  (c == EOF) {
			state = STATE_END_GAME;
		}  else  if  (c == '{') { // skip comments
			do {
				c = getc(f);
			} while(c != EOF && c != '}');			
		}  else  if  (c == '[') {
			switch(state) {
			case STATE_START:
			case STATE_END_INFO:
				state = STATE_BEGIN_INFO;
				j = 0;
				break;
			case STATE_END_MOVE:
			case STATE_END_SCORE:
				ungetc(c, f);
				state = STATE_END_GAME;
				break;
			default:
				warn("unexpected '['");
			}
		}  else  if  (c == ']') {
			switch(state) {
			case STATE_END_VALUE:
				state = STATE_END_INFO;
				break;
			default:
				warn("unmatched ']'");
				break;
			}
		}  else  if  (c == '"') {
			switch (state) {
				case STATE_BEGIN_INFO:
					state = STATE_BEGIN_VALUE;
					for (--j; j >= 0 && isspace(info_tag[j]); --j);
					info_tag[j + 1] = '\0';
					j = 0;
					string_to_lowercase(info_tag);
					break;
				case STATE_BEGIN_VALUE:
					state = STATE_END_VALUE;
					info_value[j] = '\0';
					j = 0;					
					if (strcmp(info_tag, "black") == 0) {
						memcpy(game->name[BLACK], info_value, 31);
						game->name[BLACK][31] = '\0';
					} else if (strcmp(info_tag, "white") == 0) {
						memcpy(game->name[WHITE], info_value, 31);
						game->name[WHITE][31] = '\0';
					} else if (strcmp(info_tag, "date") == 0) {
						sscanf(info_value, "%d.%d.%d", value, value + 1, value + 2);
						game->date.year = value[0]; game->date.month = value[1]; game->date.day = value[2];
					} else if (strcmp(info_tag, "time") == 0) {
						sscanf(info_value, "%d:%d:%d", value, value + 1, value + 2);
						game->date.hour = value[0]; game->date.minute = value[1]; game->date.second = value[2];
					} else if (strcmp(info_tag, "FEN") == 0) {
						game->player = board_from_FEN(game->initial_board, info_value);
					}
					break;
			}				
		}  else  if (isdigit(c)) {
			switch(state) {
			case STATE_BEGIN_SCORE:
				score[1] = score[1] * 10 + (c - '0');
				break;
			case STATE_END_INFO:
			case STATE_END_MOVE:
				state = STATE_BEGIN_MOVE_N;
				n = c - '0';
				break;
			case STATE_BEGIN_MOVE_N:
				n = 10 * n + (c - '0');
				break;
			case STATE_BEGIN_MOVE:
				state = STATE_END_MOVE;
				move[k++] = c;
				game->move[i] = string_to_coordinate(move);
				game->hash ^= hash_move[(int)game->move[i]][i];
				i++;
				break;
			case STATE_BEGIN_INFO:
				if (j >= info_size) warn("info tag too long, will be truncated.");
				else info_tag[j++] = c;
				break;
			case STATE_BEGIN_VALUE:
				if (j >= info_size) warn("info value too long, will be truncated.");
				else info_value[j++] = c;
				break;
			default:
				warn("unexpected digit %c", c);
				break;
			}
		}  else  if  (c == '*') {
			switch(state) {
			case STATE_END_MOVE:
				state = STATE_BEGIN_SCORE;
				score[0] = score[1] = -SCORE_INF;
				warn("uncomplete game.");
				break;
			case STATE_BEGIN_VALUE:
				if (j >= info_size) warn("info value too long, will be truncated.");
				else info_value[j++] = c;
				break;
			default:
				warn("unexpected '*' %d", state);
				break;
			}
		}  else  if  (c == '.') {
			switch(state) {
			case STATE_BEGIN_MOVE_N:
				state = STATE_END_MOVE_N;
				break;
			case STATE_END_MOVE_N:
				break;
			case STATE_BEGIN_INFO:
				if (j >= info_size) warn("info tag too long, will be truncated.");
				else info_tag[j++] = c;
				break;
			case STATE_BEGIN_VALUE:
				if (j >= info_size) warn("info value too long, will be truncated.");
				else info_value[j++] = c;
				break;
			default:
				warn("unexpected '.'");
				break;
			}
		}  else  if  (c == '-') {
			switch(state) {
			case STATE_BEGIN_MOVE_N:
				state = STATE_BEGIN_SCORE;
				score[0] = n;
				break;
			case STATE_BEGIN_INFO:
				if (j >= info_size) warn("info tag too long, will be truncated.");
				else info_tag[j++] = c;
				break;
			case STATE_BEGIN_VALUE:
				if (j >= info_size) warn("info value too long, will be truncated.");
				else info_value[j++] = c;
				break;
			case STATE_END_MOVE_N:
			case STATE_END_MOVE:
				break;
			default:
				warn("unexpected '-'");
				break;
			}
		}  else  if  (isalpha(c) || c == '@') {
			switch(state) {
			case STATE_END_MOVE_N:
			case STATE_END_MOVE:
				state = STATE_BEGIN_MOVE;
				k = 0;
				move[k++] = c;
				break;
			case STATE_BEGIN_MOVE:
				if (k < 4) move[k++] = c;
				else warn("unexpected %c", c);
				break;
			case STATE_BEGIN_INFO:
				if (j >= info_size) warn("info tag too long, will be truncated.");
				else info_tag[j++] = c;
				break;
			case STATE_BEGIN_VALUE:
				if (j >= info_size) warn("info value too long, will be truncated.");
				else info_value[j++] = c;
				break;
			default:
				warn("unexpected %c", c);
				break;
			}
		}  else  if  (isspace(c)) {
			switch(state) {
			case STATE_BEGIN_SCORE:
			case STATE_END_SCORE:
				state = STATE_END_SCORE;
				break;
			case STATE_BEGIN_INFO:
				if (j >= info_size) warn("info tag too long, will be truncated.");
				else info_tag[j++] = c;
				break;
			case STATE_BEGIN_VALUE:
				if (j >= info_size) warn("info value too long, will be truncated.");
				else info_value[j++] = c;
				break;
			default:
				break;
			}
		} else {
			switch(state) {
			case STATE_BEGIN_INFO:
				if (j >= info_size) warn("info tag too long, will be truncated.");
				else info_tag[j++] = c;
				break;
			case STATE_BEGIN_VALUE:
				if (j >= info_size) warn("info value too long, will be truncated.");
				else info_value[j++] = c;
				break;
			default:
				warn("unexpected %c", c);
				break;
			}
		}
			
	}

	if (!game_check(game)) {
		warn("error while importing a PGN game\n");
	}
	return;
}

/**
 * @brief Write a game to a pgn file.
 *
 * @param game The input game.
 * @param f The file stream.
 */
void game_export_pgn(const Game *game, FILE *f)
{
	time_t t = time(NULL);
	struct tm *date = localtime(&t);
	int half_score = game_score(game) / 2;
	const char *result = half_score < -32 ? "*" : (half_score < 0 ? "0-1" : (half_score > 0 ? "1-0" : "1/2-1/2"));
	const char *winner = (half_score < 0 ?  game->name[WHITE]: (half_score > 0 ? game->name[BLACK] : NULL));
	Board board[1];
	char s[80];
	int i, j, k;
	int player;

	board_init(board);

	fputs("[Event \"?\"]\n", f);
	fputs("[Site \"edax\"]\n", f);
	if (game->date.year == 0) fprintf(f, "[Date \"%d.??.??\"]\n", date->tm_year + 1900);
	else if (game->date.month == 0) fprintf(f, "[Date \"%d.??.??\"]\n", game->date.year);
	else if (game->date.day == 0) fprintf(f, "[Date \"%d.%d.??\"]\n", game->date.year, game->date.month);
	fprintf(f, "[Date \"%d.%d.%d\"]\n", game->date.year, game->date.month, game->date.day);
	if (game->date.hour >= 0) fprintf(f, "[Time \"%d.%d.%d\"]\n", game->date.hour, game->date.minute, game->date.second);
	fputs("[Round \"?\"]\n", f);
	fprintf(f, "[Black \"%s\"]\n", game->name[BLACK]);
	fprintf(f, "[White \"%s\"]\n", game->name[WHITE]);
	fprintf(f, "[Result \"%s\"]\n", result);
	if (!board_equal(game->initial_board, board)) {
		fprintf(f, "[FEN \"%s\"]\n", board_to_FEN(game->initial_board, game->player, s));
		*board = *game->initial_board;
	}
	fputc('\n', f);

	player = game->player;
	for (i = j = k = 0; i < 60 && game->move[i] != NOMOVE; ++i, ++k) {
		if (!can_move(board->player, board->opponent)) {
			s[0] = 'p'; s[1] = 'a'; s[2] = 's'; s[3] = 's'; s[4] = '\0'; --i;
			board_pass(board);
		} else if (game_update_board(board, game->move[i])) {
			move_to_string(game->move[i], WHITE, s);
		}
		if ((j >= 78) || (player == game->player && j >= 74)) {
			fputc('\n', f);
			j = 0;
		} else {
			fputc(' ', f);
			++j;
		}
		if (player == game->player) j += fprintf(f, "%d. ", (k >> 1) + 1);
		fputs(s, f); j += 2;
		player = !player;
	}
	if (winner) fprintf(f, "\n{%s wins %d-%d}", winner, 32 + half_score, 32 - half_score);
	else if (half_score == 0) fprintf(f, "\n{Draw 32-32}");
	fprintf(f, " %s\n\n\n", result);
}

/**
 * @brief Write a game to an eps file.
 *
 * @param game The input game.
 * @param f The file stream.
 */
void game_export_eps(const Game *game, FILE *f)
{
	time_t t = time(NULL);
	struct tm *date = localtime(&t);
	Board board[1];
	int i, color, player;
	char s_player[2][8] = {"black", "white"};
	char s[8];

	fputs("%!PS-Adobe-3.0 EPSF-3.0\n",f);
	fputs("%%Creator: Edax-3.0\n",f);
	fprintf(f, "%%%%CreationDate:  %d/%d/%d %d:%d:%d\n", date->tm_year + 1900, date->tm_mon + 1, date->tm_mday, date->tm_hour, date->tm_min, date->tm_sec);
	fputs("%%BoundingBox: 0 0 200 200\n\n",f);
	fputs("%%BeginProlog\n\n",f);
	fputs("% othello coordinates\n",f);
	fputs("/A1 {40 160} def /A2 {40 140} def /A3 {40 120} def /A4 {40 100} def /A5 {40 80} def /A6 {40 60} def /A7 {40 40} def /A8 {40 20} def\n",f);
	fputs("/B1 {60 160} def /B2 {60 140} def /B3 {60 120} def /B4 {60 100} def /B5 {60 80} def /B6 {60 60} def /B7 {60 40} def /B8 {60 20} def\n",f);
	fputs("/C1 {80 160} def /C2 {80 140} def /C3 {80 120} def /C4 {80 100} def /C5 {80 80} def /C6 {80 60} def /C7 {80 40} def /C8 {80 20} def\n",f);
	fputs("/D1 {100 160} def /D2 {100 140} def /D3 {100 120} def /D4 {100 100} def /D5 {100 80} def /D6 {100 60} def /D7 {100 40} def /D8 {100 20} def\n",f);
	fputs("/E1 {120 160} def /E2 {120 140} def /E3 {120 120} def /E4 {120 100} def /E5 {120 80} def /E6 {120 60} def /E7 {120 40} def /E8 {120 20} def\n",f);
	fputs("/F1 {140 160} def /F2 {140 140} def /F3 {140 120} def /F4 {140 100} def /F5 {140 80} def /F6 {140 60} def /F7 {140 40} def /F8 {140 20} def\n",f);
	fputs("/G1 {160 160} def /G2 {160 140} def /G3 {160 120} def /G4 {160 100} def /G5 {160 80} def /G6 {160 60} def /G7 {160 40} def /G8 {160 20} def\n",f);
	fputs("/H1 {180 160} def /H2 {180 140} def /H3 {180 120} def /H4 {180 100} def /H5 {180 80} def /H6 {180 60} def /H7 {180 40} def /H8 {180 20} def\n\n",f);
	fputs("% draw a black disc\n",f);
	fputs("/disc_black{\n",f);
	fputs("\tnewpath\n",f);
	fputs("\t8.5 0 360 arc\n",f);
	fputs("\tfill\n",f);
	fputs("} def\n\n",f);
	fputs("% draw a white disc\n",f);
	fputs("/disc_white{\n",f);
	fputs("\tnewpath\n",f);
	fputs("\t0.5 setlinewidth\n",f);
	fputs("\t8.5 0 360 arc\n",f);
	fputs("\tstroke\n",f);
	fputs("} def\n\n",f);
	fputs("% draw a black move\n",f);
	fputs("/move_black{\n",f);
	fputs("\t/y exch def\n",f);
	fputs("\t/x exch def\n",f);
	fputs("\tnewpath\n",f);
	fputs("\tx y 8.5 0 360 arc\n",f);
	fputs("\tfill\n",f);
	fputs("\t1 setgray\n",f);
	fputs("\tx y moveto dup stringwidth pop 2 div neg -4.5 rmoveto\n",f);
	fputs("\tshow\n",f);
	fputs("\t0 setgray\n",f);
	fputs("} def\n\n",f);
	fputs("% draw a white move\n",f);
	fputs("/move_white{\n",f);
	fputs("\t/y exch def\n",f);
	fputs("\t/x exch def\n",f);
	fputs("\tnewpath\n",f);
	fputs("\t0.5 setlinewidth\n",f);
	fputs("\tx y 8.5 0 360 arc\n",f);
	fputs("\tstroke\n",f);
	fputs("\tx y moveto dup stringwidth pop 2 div neg -4.5 rmoveto\n",f);
	fputs("\tshow\n",f);
	fputs("} def\n\n",f);
	fputs("% draw the grid\n",f);
	fputs("/board_grid{\n",f);
	fputs("\tnewpath\n\n",f);
	fputs("\t%border\n",f);
	fputs("\t1.5 setlinewidth\n",f);
	fputs("\t  27   7 moveto\n",f);
	fputs("\t 166   0 rlineto\n",f);
	fputs("\t   0 166 rlineto\n",f);
	fputs("\t-166   0 rlineto\n",f);
	fputs("\tclosepath\n",f);
	fputs("\tstroke\n\n",f);
	fputs("\t%vertical lines\n",f);
	fputs("\t0.5 setlinewidth\n",f);
	fputs("\t30 10 moveto\n",f);
	fputs("\t0 1 8{\n",f);
	fputs("\t\t 0  160 rlineto\n",f);
	fputs("\t\t20 -160 rmoveto\n",f);
	fputs("\t}for\n\n",f);
	fputs("\t%horizontal lines\n",f);
	fputs("\t30 10 moveto\n",f);
	fputs("\t0 1 8{\n",f);
	fputs("\t\t 160  0 rlineto\n",f);
	fputs("\t\t-160 20 rmoveto\n",f);
	fputs("\t}for\n",f);
	fputs("\tstroke\n\n",f);
	fputs("\t%marks\n",f);
	fputs("\t 70  50 2 0 360 arc fill\n",f);
	fputs("\t150  50 2 0 360 arc fill\n",f);
	fputs("\t 70 130 2 0 360 arc fill\n",f);
	fputs("\t150 130 2 0 360 arc fill\n",f);
	fputs("}def\n\n",f);
	fputs("% draw coordinates\n",f);
	fputs("/board_coord{\n",f);
	fputs("\t/NewCenturySchoolbook-Roman findfont 15 scalefont setfont\n",f);
	fputs("\tnewpath\n",f);
	fputs("\t(a)  35 180 moveto show\n",f);
	fputs("\t(b)  55 180 moveto show\n",f);
	fputs("\t(c)  75 180 moveto show\n",f);
	fputs("\t(d)  95 180 moveto show\n",f);
	fputs("\t(e) 115 180 moveto show\n",f);
	fputs("\t(f) 135 180 moveto show\n",f);
	fputs("\t(g) 155 180 moveto show\n",f);
	fputs("\t(h) 175 180 moveto show\n",f);
	fputs("\t(1)  14 155 moveto show\n",f);
	fputs("\t(2)  14 135 moveto show\n",f);
	fputs("\t(3)  14 115 moveto show\n",f);
	fputs("\t(4)  14  95 moveto show\n",f);
	fputs("\t(5)  14  75 moveto show\n",f);
	fputs("\t(6)  14  55 moveto show\n",f);
	fputs("\t(7)  14  35 moveto show\n",f);
	fputs("\t(8)  14  15 moveto show\n",f);
	fputs("}def\n",f);
	fputs("%%EndProlog\n\n",f);
	
	fputs("% do the drawing\n",f);
	fputs("gsave\n",f);
	fputs("\n\t% draw an empty board\n",f);
	fputs("\tboard_coord\n",f);
	fputs("\tboard_grid\n",f);
	fputs("\n\t% draw the discs\n",f);

	*board = *game->initial_board;
	for (i = A1; i <= H8; i++) {
		color = board_get_square_color(board, i);
		if (color != EMPTY) {
			if (game->player == WHITE) color = !color;
			fprintf(f, "\t%s disc_%s\n", move_to_string(i, 0, s), s_player[color]);
		}
	}

	fputs("\n\t% draw the moves\n",f);
	fputs("\t/Utopia-Bold findfont 12 scalefont setfont\n",f);
	player = game->player;
	for (i = 0; i < 60 && game->move[i] != NOMOVE; ++i) {
		if (!can_move(board->player, board->opponent)) {
			player = !player;
		}
		if (game_update_board(board, game->move[i])) {
			fprintf(f, "\t(%d) %s move_%s\n", i + 1,  move_to_string(game->move[i], BLACK, s), s_player[player]);
			player = !player;
		}
	}
	fputc('\n',f);
	fputs("grestore\n",f);
}

void game_export_svg(const Game *game, FILE *f)
{
	int i, color, player;
	char s_color[2][8] = {"black", "white"};
	Board board[1];
	const char *style = "font-size:22px;text-align:center;text-anchor:middle;font-family:Times New Roman;font-weight:bold";

	// prolog
	fputs("<?xml version=\"1.0\" encoding=\"utf-8\"?>\n", f);
	fputs("<svg\n", f);
		fputs("\txmlns=\"http://www.w3.org/2000/svg\"\n", f);
		fputs("\tversion=\"1.1\"\n", f);
		fputs("\twidth=\"440\"\n", f);
		fputs("\theight=\"440\">\n", f);
	fputs("\t<desc>Othello Game</desc>\n", f);

	// board
	fputs("\t<rect\n", f);
		fputs("\t\twidth=\"332\" height=\"332\" ", f);
		fputs("x=\"54\" y=\"54\" ", f);
		fputs("stroke=\"black\" ", f);
		fputs("stroke-width=\"2\" ", f);
		fputs("fill=\"white\" />\n", f);
	fputs("\t<rect\n", f);
		fputs("\t\twidth=\"320\" height=\"320\" ", f);
		fputs("x=\"60\" y=\"60\" ", f);
		fputs("stroke=\"black\" ", f);
		fputs("fill=\"green\" />\n", f);
	for (i = 1; i < 8; ++i) {
		fputs("\t<line\n", f);
			fprintf(f, "\t\tx1=\"60\" y1=\"%d\" ", 60 + 40 * i);
			fprintf(f, "x2=\"380\" y2=\"%d\" ", 60 + 40 * i);
			fputs("stroke=\"black\" />\n", f);
		fputs("\t<line\n", f);
			fprintf(f, "\t\tx1=\"%d\" y1=\"60\" ", 60 + 40 * i);
			fprintf(f, "x2=\"%d\" y2=\"380\" ", 60 + 40 * i);
			fputs("stroke=\"black\" />\n", f);
	}
	fputs("\t<circle cx=\"140\" cy=\"140\" r=\"4\" fill=\"black\" />\n", f);
	fputs("\t<circle cx=\"300\" cy=\"140\" r=\"4\" fill=\"black\" />\n", f);
	fputs("\t<circle cx=\"140\" cy=\"300\" r=\"4\" fill=\"black\" />\n", f);
	fputs("\t<circle cx=\"300\" cy=\"300\" r=\"4\" fill=\"black\" />\n", f);

	// coordinates
	for (i = 0; i < 8; ++i) {
		fprintf(f, "\t<text x=\"%d\" y=\"%d\" style = \"%s\" > %d </text>\n", 40, 85 + i * 40, style, i + 1);
		fprintf(f, "\t<text x=\"%d\" y=\"%d\" style = \"%s\" > %c </text>\n", 80 + i * 40, 50, style, i + 'a');
	}

	// discs
	for (i = A1; i <= H8; i++) {
		color = board_get_square_color(game->initial_board, i);
		if (color != EMPTY) {
			if (game->player == WHITE) color = !color;
			fprintf(f, "\t<circle cx=\"%d\" cy=\"%d\"  r=\"17\" fill=\"%s\" stroke=\"%s\" />\n",
				 80 + 40 * (i % 8), 80 + 40 * (i / 8), s_color[color], s_color[!color]);
		}
	}

	// moves
	*board = *game->initial_board;
	player = game->player;
	for (i = 0; i < 60 && game->move[i] != NOMOVE; ++i) {
		if (!can_move(board->player, board->opponent)) {
			player = !player;
		}
		if (game_update_board(board, game->move[i])) {
			fprintf(f, "\t<circle cx=\"%d\" cy=\"%d\" r=\"17\" fill=\"%s\" stroke=\"%s\" />\n",
				 80 + 40 * (game->move[i] % 8), 80 + 40 * (game->move[i] / 8), s_color[player], s_color[!player]);
			player = !player;
			fprintf(f, "\t<text x=\"%d\" y=\"%d\" fill=\"%s\" style=\"%s\"> %d </text>\n",
				83 + 40 * (game->move[i] % 8), 87 + 40 * (game->move[i] / 8), s_color[player], style, i + 1);
		}
	}
	fputc('\n',f);

	// end
	fputs("</svg>\n", f);
}


/** 
 * @brief Fill a game with some random moves.
 * 
 * @param game The output game.
 * @param n_ply The number of random move to generate.
 * @param r The random generator.
 */
void game_rand(Game *game, int n_ply, Random *r)
{
	Move move[1];
	unsigned long long moves;
	int ply;
	Board board[1];

	game_init(game);
	board_init(board);
	for (ply = 0; ply < n_ply; ply++) {
		moves = get_moves(board->player, board->opponent);
		if (!moves) {
			board_pass(board);
			moves = get_moves(board->player, board->opponent);
			if (!moves) {
				break;
			}
		}
		;
		board_get_move(board, get_rand_bit(moves, r), move);
		game->move[ply] = move->x;
		board_update(board, move);
	}
}

/**
 * @brief Analyze an endgame.

 * Count how many mistakes occured in the last moves.
 *
 * @param game Game to analyze.
 * @param search Search analyzer.
 * @param n_empties Move stage to analyze.
 * @param apply_correction Flag to correct or not a game.
 */
int game_analyze(Game *game, Search *search, const int n_empties, const bool apply_correction)
{
	Board board[1];
	struct {
		Move played;
		Move best;
		Line pv[1];
		int n_empties;
	} stack[99];
	int n_error = 0;
	int n_move;
	const int verbosity = search->options.verbosity;
	int player;
	int score;
	int i;

	search->options.verbosity = 0;
	search_cleanup(search);
	*board = *game->initial_board;
	player = game->player;
	for (i = n_move = 0; i < 60 && game->move[i] != NOMOVE; ++i) {
		if (!can_move(board->player, board->opponent)) {
			stack[n_move].best = MOVE_INIT;
			line_init(stack[n_move].pv, player);
			stack[n_move++].played = MOVE_PASS;
			board_pass(board);
			player = !player;
		} 
		if (!board_is_occupied(board, game->move[i]) && board_get_move(board, game->move[i], &stack[n_move].played)) {
			stack[n_move].best = MOVE_INIT;
			line_init(stack[n_move].pv, player);
			search_set_board(search, board, player);
			search_set_level(search, 60, search->n_empties);
			stack[n_move].n_empties = search->n_empties;
			if (search->movelist->n_moves > 1 && search->n_empties <= n_empties) {
				movelist_exclude(search->movelist, game->move[i]);
				search_run(search);
				stack[n_move].best = *(movelist_first(search->movelist));
				*stack[n_move].pv = *search->result->pv;
			}
			board_update(board, &stack[n_move].played);
			player = !player;
			++n_move;
		} else {
			char s[4];
			warn("\nillegal move %s in game:\n", move_to_string(game->move[i], player, s));			
			game_export_text(game, stderr);
			board_print(board, player, stderr);
			fprintf(stderr, "\n\n");			
			return 1; // stop, illegal moves
		}
	}

	search_set_board(search, board, player);
	if (search->n_empties <= n_empties) {
		search_set_level(search, 60, search->n_empties);
		search_run(search);
		score = search->result->score;
		
		for (i = n_move - 1; stack[i].n_empties <= n_empties; --i) {
			stack[i].played.score = -score;
			score = MAX(stack[i].played.score, stack[i].best.score);
		}
		
		//backpropagate the score
		while (stack[--n_move].n_empties <= n_empties) {
			if (stack[n_move].played.score < stack[n_move].best.score) {
				++n_error;
				// correct the move?
				if (apply_correction && stack[n_move].best.x != NOMOVE) {
					for (i = 0; i < 60 && game->move[i] != 0; ++i) {
						if (game->move[i] == stack[n_move].played.x) {
							game_append_line(game, stack[n_move].pv, i);
						}
					}
				}
			}
		}
	}

	search->options.verbosity = verbosity;

	return n_error;
}

/**
 * @brief Terminate an unfinished game.
 *
 *
 * @param game Game to analyze.
 * @param search Search analyzer.
 * @return number of iterations to terminate the game.
 */
int game_complete(Game *game, Search *search) 
{
	Board board[1];
	const int verbosity = search->options.verbosity;
	int i, n;
	int player;

	search->options.verbosity = 0;
	search_cleanup(search);

	player = game->player;
	for (n = 0; n < 60; ++n) {
		*board = *game->initial_board;
		for (i = 0; i < 60 && game->move[i] != NOMOVE; ++i) {
			player ^= game_update_player(board, game->move[i]);
			if (!game_update_board(board, game->move[i])) {
				break;
			}
		}

		if (!can_move(board->player, board->opponent)) {
			if (!can_move(board->opponent, board->player)) break;
			player ^= 1;
			board_pass(board);
		}

		search_set_board(search, board, player);
		search_run(search);
		if (search->result->depth == search->n_empties && search->result->selectivity == NO_SELECTIVITY) {
			game_append_line(game, search->result->pv, i);
		} else {
			game->move[i] = search->result->move;
		}
		if (search->result->score != 0) {
			putchar('\n'); search->observer(search->result);
		}
	}

	search->options.verbosity = verbosity;

	return n;
}

