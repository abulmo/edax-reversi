/**
 * @file gtp.c
 *
 * Go Text Protocol
 *
 * @date 1998 - 2012
 * @author Richard Delorme
 * @version 4.3
 */

#include "options.h"
#include "search.h"
#include "util.h"
#include "ui.h"
#include "play.h"

#include <ctype.h>

static Log gtp_log[1];

static void gtp_observer(Result *result)
{
	if (log_is_open(gtp_log)) {
		result_print(result, gtp_log->f);
		putc('\n', gtp_log->f);
	}
}

/*
 * @brief Preprocess a line according to GTP2.
 *
 * @param line string to process
 */
void gtp_preprocess(char *line)
{
	char *s;

	for (s = line; *line; ++line) {
		if (*line == '\t') *line = ' ';
		if (*line == '#') break;
		if (*line < 32 && *line != '\n') continue;

		*s++ = *line;
	}
	*s = '\0';
}

static void gtp_send(const char *s, int id, bool has_id)
{
	if (has_id) printf("= %d ", id);
	else printf("= ");
	puts(s);
	putchar('\n'); fflush(stdout);

	log_print(gtp_log, "sent> \"%s\"\n", s);
}

static void gtp_fail(const char *s, int id, bool has_id)
{
	if (has_id) printf("? %d ", id);
	else printf("? ");
	puts(s);
	putchar('\n'); fflush(stdout);

	log_print(gtp_log, "error> \"%s\"\n", s);
}

static char *gtp_parse_color(char *s, int *color)
{
	char word[8];

	s = parse_word(s, word, 7);

	if (strcmp(word, "black") == 0) {
		*color = BLACK;
	} else if (strcmp(word, "b") == 0) {
		*color = BLACK;
	} else if (strcmp(word, "white") == 0) {
		*color = WHITE;
	} else if (strcmp(word, "w") == 0) {
		*color = WHITE;
	} else {
		*color = EMPTY;
	}

	return s;
}

/**
 * @brief initialize edax protocol
 * @param ui user interface
 */
void ui_init_gtp(UI *ui)
{
	Play *play = ui->play;

	options.verbosity = 0;
	options.info = 0;

	play_init(play, ui->book);
	ui->book->search = play->search;
	book_load(ui->book, options.book_file);
	play->search->id = 1;
	search_set_observer(play->search, gtp_observer);
	ui->mode = 3;
	play->type = ui->type;

	log_open(gtp_log, options.ui_log_file);
}

/**
 * @brief free resources used by edax protocol
 * @param ui user interface
 */
void ui_free_gtp(UI *ui)
{
	if (ui->book->need_saving) book_save(ui->book, options.book_file);
	book_free(ui->book);
	play_free(ui->play);
	log_close(gtp_log);
}

/**
 * @brief Loop event
 * @param ui user interface
 */
void ui_loop_gtp(UI *ui)
{
	char *cmd = NULL, *param = NULL, *move;
	Play *play = ui->play;
	int id = 0;
	bool has_id = false;
	int komi, color, size;
	int byo_yomi_time = 0, byo_yomi_stone = 0, main_time = 315576000;

	// loop forever
	for (;;) {
		errno = 0;

		if (log_is_open(gtp_log)) {
			play_print(play, gtp_log->f);
		}

		ui_event_wait(ui, &cmd, &param);

		log_print(gtp_log, "received: \"%s %s\"\n", cmd, param);

		if (isdigit(*cmd)) {
			has_id = true;
			parse_int(cmd, &id);
			parse_command(param, cmd, param, strlen(param));
		}

		if (*cmd == '\0') {

		} else if (strcmp(cmd, "protocol_version") == 0) {
			gtp_send("2", id, has_id);

		} else if (strcmp(cmd, "name") == 0) {
			gtp_send("Edax", id, has_id);

		} else if (strcmp(cmd, "version") == 0) {
			gtp_send(VERSION_STRING, id, has_id);

		} else if (strcmp(cmd, "known_command") == 0) {
			if (strcmp(param, "protocol_version") == 0 ||
				strcmp(param, "name") == 0 ||
				strcmp(param, "version") == 0 ||
				strcmp(param, "known_command") == 0 ||
				strcmp(param, "list_commands") == 0 ||
				strcmp(param, "quit") == 0 ||
				strcmp(param, "boardsize") == 0 ||
				strcmp(param, "clear_board") == 0 ||
				strcmp(param, "komi") == 0 ||
				strcmp(param, "play") == 0 ||
				strcmp(param, "genmove") == 0 ||
				strcmp(param, "undo") == 0 ||
				strcmp(param, "time_settings") == 0 ||
				strcmp(param, "time_left") == 0 ||
				strcmp(param, "set_game") == 0 ||
				strcmp(param, "list_games") == 0 ||
				strcmp(param, "loadsgf") == 0 ||
				strcmp(param, "reg_genmove") == 0 ||
				strcmp(param, "showboard") == 0) gtp_send("true", id, has_id);
			else gtp_send("false", id, has_id);

		} else if (strcmp(cmd, "list_commands") == 0) {
			gtp_send("protocol_version\nname\nversion\nknown_command\nlist_commands\n"
				"quit\nboardsize\nclear_board\nkomi\nplay\ngenmove\nundo\n"
				"time_settings\ntime_left\nset_game\nlist_games\n"
				"loadsgf\nreg_genmove\nshowboard", id, has_id);

		} else if (strcmp(cmd, "quit") == 0 || strcmp(cmd, "eof") == 0) {
			gtp_send("", id, has_id);
			free(cmd); free(param);
			return;

		} else if (strcmp(cmd, "boardsize") == 0) {
			size = string_to_int(param, 0);
			if (size != 8) gtp_fail("unacceptable size", id, has_id);
			else gtp_send("", id, has_id);

		} else if (strcmp(cmd, "clear_board") == 0) {
			play_new(play);
			gtp_send("", id, has_id);

		} else if (strcmp(cmd, "komi") == 0) {
			komi = string_to_int(param, 0);
			gtp_send("", id, has_id);
			(void) komi; // do nothing of it....

		} else if (strcmp(cmd, "play") == 0) {
			move = gtp_parse_color(param, &color);
			if (color == EMPTY) {
				gtp_fail("syntax error (wrong or missing color)", id, has_id);
				continue;
			} else if (color != play->player) {
				if (play_must_pass(play)) {
					play_move(play, PASS);
				} else {
					gtp_fail("wrong color", has_id, id);
					continue;
				}
			 }

			if (play_user_move(play, move)) {
				gtp_send("", id, has_id);
			} else {
				gtp_fail("illegal move", id, has_id);
			}

		} else if (strcmp(cmd, "genmove") == 0) {
			char m[4];
			gtp_parse_color(param, &color);
			if (color == EMPTY) {
				gtp_fail("syntax error (wrong or missing color)", id, has_id);
				continue;
			} else if (color != play->player) {
				if (play_must_pass(play)) {
					play_move(play, PASS);
				} else {
					gtp_fail("wrong color", has_id, id);
					continue;
				}
			}
			play_go(play, true);
			if (play_get_last_move(play)->x == PASS) gtp_send("pass", id, has_id);
			else gtp_send(move_to_string(play_get_last_move(play)->x, WHITE, m), id, has_id);

		// optional GTP commands but needed or supported by Quarry
		} else if (strcmp(cmd, "undo") == 0) {
			if (play->i_game <= 0) {
				gtp_fail("cannot undo", has_id, id);
			} else {
				play_undo(play);
				gtp_send("", id, has_id);
			}

		} else if (strcmp(cmd, "time_settings") == 0) {
			char *s;
			s = parse_int(param, &main_time);
			s = parse_int(s, &byo_yomi_time);
			s = parse_int(s, &byo_yomi_stone);
			if (byo_yomi_stone > 0) {
				options.play_type = EDAX_TIME_PER_MOVE;
			} else {
				options.play_type = EDAX_TIME_PER_GAME;
			}
			options.time = 1000 * main_time;
			gtp_send("", id, has_id);

		} else if (strcmp(cmd, "time_left") == 0) {
			char *s;
			double t = 0.0;
			int n = 0;
			s = gtp_parse_color(param, &color);
			s = parse_real(s, &t); t *= 1000;
			s = parse_int(s, &n);
			options.level = 60;
			if (options.play_type == EDAX_TIME_PER_MOVE) {// time_per_move ?
				if (n == 0) play->time[color].left = (t + byo_yomi_time * 1000) / byo_yomi_stone;
				else play->time[color].left = t / n;
			} else { // time_per_game
				play->time[color].left = t;
			}
			gtp_send("", id, has_id);

		} else if (strcmp(cmd, "set_game") == 0) {
			if (strcmp(param, "Othello") == 0) gtp_send("", id, has_id);
			else gtp_fail("unsupported game", id, has_id);

		} else if (strcmp(cmd, "list_games") == 0) {
			gtp_send("Othello", id, has_id);

		// regression subset
		} else if (strcmp(cmd, "loadsgf") == 0) {
			if (play_load(play, param)) {
				gtp_send("", id, has_id);
			} else {
				gtp_fail(play->error_message, id, has_id);
			}

		} else if (strcmp(cmd, "reg_genmove") == 0) {
			char m[4];
			gtp_parse_color(param, &color);
			if (color == EMPTY) {
				gtp_fail("syntax error (wrong or missing color)", id, has_id);
				continue;
			} else if (color != play->player) {
				if (play_must_pass(play)) {
					play_move(play, PASS);
				} else {
					gtp_fail("wrong color", has_id, id);
					continue;
				}
			}
			play_go(play, false);
			if (play_get_last_move(play)->x == PASS) gtp_send("pass", id, has_id);
			else gtp_send(move_to_string(play_get_last_move(play)->x, WHITE, m), id, has_id);

		// optional commands
		} else if (strcmp(cmd, "showboard") == 0) {
			printf("= ");
			if (has_id) printf("%d\n", id);
			board_print(play->board, play->player, stdout);
			putchar('\n'); fflush(stdout);

		// error: unknown message
		} else {
			gtp_fail("unknown command", id, has_id);
		}
	}
}


