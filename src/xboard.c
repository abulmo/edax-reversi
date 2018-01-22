/**
 * @file xboard.c
 *
 * @brief xboard protocol.
 *
 * Of course, only the "reversi" variant is supported.
 *
 * @date 1998 - 2017
 * @author Richard Delorme
 * @version 4.4
 *
 */

#include "bit.h"
#include "event.h"
#include "options.h"
#include "opening.h"
#include "play.h"
#include "search.h"
#include "stats.h"
#include "util.h"
#include "ui.h"

#include <stdarg.h>

Log xboard_log[1];

typedef struct {
	unsigned long long time;
	unsigned long long n_nodes;
	int n_games;
} XBoardStats;

/**
 * @brief Search oberver.
 * @param result Search Result.
 */
static void xboard_observer(Result *result)
{
	spin_lock(result);

	printf("%2d ", result->depth);
	printf("%4d ", 100 * result->score);
	printf("%6lld ", result->time / 10);
	printf("%10lld ", result->n_nodes);
	if (result->selectivity < 5) printf("@%2d%% ", selectivity_table[result->selectivity].percent);	
	if (result->book_move) putchar('(');
	line_print(result->pv, -200, " ", stdout);
	if (result->book_move) putchar(')');
	putchar('\n');
	fflush(stdout);

	if (log_is_open(xboard_log)) {
		fprintf(xboard_log->f, "edax> %2d ", result->depth);
		fprintf(xboard_log->f, "%4d ", 100 * result->score);
		fprintf(xboard_log->f, "%6lld ", result->time / 10);
		fprintf(xboard_log->f, "%10lld ", result->n_nodes);
		if (result->selectivity < 5) fprintf(xboard_log->f, "@%2d%% ", selectivity_table[result->selectivity].percent);	
		if (result->book_move) fputc('(', xboard_log->f);
		line_print(result->pv, -200, " ", xboard_log->f);
		if (result->book_move) fputc(')', xboard_log->f);
		putc('\n', xboard_log->f);
		fflush(xboard_log->f);
	}
	spin_unlock(result);
}

/**
 * @brief initialize xboard protocol.
 * @param ui user interface.
 */
void ui_init_xboard(UI *ui)
{
	Play *play = ui->play;
	Search *search = play->search;

	play_init(play, ui->book);
	play->initial_player = board_from_FEN(play->initial_board, "8/8/8/3Pp3/3pP3/8/8/8 w - - 0 1");
	play_new(play);
	search->options.header = NULL;
	search->options.separator = NULL;
	ui->book->search = search;
	book_load(ui->book, options.book_file);
	search->id = 1;
	search_set_observer(search, xboard_observer);
	options.level = 60;
	play->type = ui->type;
	play->ponder->verbose = true;

	log_open(xboard_log, options.ui_log_file);
}

/**
 * @brief free resources used by xboard protocol.
 * @param ui user interface.
 */
void ui_free_xboard(UI *ui)
{
	if (ui->book->need_saving) book_save(ui->book, options.book_file);
	book_free(ui->book);
	play_free(ui->play);
	log_close(xboard_log);
}

/**
 * @brief Print an error.
 *
 * @param format format string.
 */
static void xboard_error(const char *format, ...)
{
	va_list args;

	fprintf(stderr, "Error ");
	va_start(args, format);
	vfprintf(stderr, format, args);
	va_end(args);
	putc('\n', stderr);
	fflush(stderr);

	if (log_is_open(xboard_log)) {
		fprintf(xboard_log->f, "error> \"");
		va_start(args, format);
		vfprintf(xboard_log->f, format, args);
		va_end(args);
		fprintf(xboard_log->f, "\"\n");
		fflush(xboard_log->f);
	}
}

/**
 * @brief Send a command to xboard/winboard GUI.
 *
 * @param format format string.
 */
static void xboard_send(const char *format, ...)
{
	va_list args;

	va_start(args, format);
	vprintf(format, args);
	va_end(args);
	fflush(stdout);

	if (log_is_open(xboard_log)) {
		fprintf(xboard_log->f, "edax> ");
		va_start(args, format);
		vfprintf(xboard_log->f, format, args);
		va_end(args);
		fflush(xboard_log->f);
	}
}

static void xboard_setup(Play *play)
{
	char fen[120];

	xboard_send("setup (P.....p.....) %s\n", board_to_FEN(play->board, play->player, fen));
}

/**
 * @brief Send a move to xboard/winboard GUI.
 *
 * @param x Move coordinate.
 */
static void xboard_move(const int x)
{
	if (x == PASS) printf("move @@@@\n");
	else {
		printf("move P@");
		move_print(x, 1, stdout);
		putchar('\n');
	}
	fflush(stdout);
	if (log_is_open(xboard_log)) {
		if (x == PASS) printf("edax> move @@@@\n");
		else {
			fprintf(xboard_log->f, "edax> move P@");
			move_print(x, 1, xboard_log->f);
			fputc('\n', xboard_log->f);
		}
		fflush(xboard_log->f);
	}
}

/**
 * @brief Send a hint.
 *
 * @param play Play engine.
 */
static void xboard_hint(Play *play)
{
	const int x = search_guess(play->search, play->board);

	if (x == NOMOVE) return;
	
	if (x == PASS) printf("Hint:@@@@\n");
	else {
		printf("Hint:P@");
		move_print(x, 1, stdout);
		putchar('\n');
		fflush(stdout);
	}
	if (log_is_open(xboard_log)) {
		if (x == PASS) printf("edax> Hint:@@@@\n");
		else {
			fprintf(xboard_log->f, "edax> Hint:P@");
			move_print(x, 1, xboard_log->f);
			fputc('\n', xboard_log->f);
		}
		fflush(xboard_log->f);
	}
}

/**
 * @brief Send a move from the book, if available.
 *
 * @param play Play engine.
 */
static void xboard_book(Play *play) {
	Board *board = play->board;
	Book *book = play->book;
	MoveList movelist[1];
	Move *move;

	if (book_get_moves(book, board, movelist)) {
		printf("\tBook:");
		foreach_move(move, movelist) {
			printf("  P@"); move_print(move->x, 1, stdout);
			printf(":%d", move->score * 100);
		}
		putchar('\n');
		fflush(stdout);
		if (log_is_open(xboard_log)) {
			fprintf(xboard_log->f, "edax> Book:");
			foreach_move(move, movelist) {
				fprintf(xboard_log->f, "  P@"); move_print(move->x, 1, xboard_log->f);
				fprintf(xboard_log->f, ":%d", move->score * 100);
			}
			putc('\n', xboard_log->f);
			fflush(xboard_log->f);
		}		
	}
}

/**
 * @brief Check if the game is over.
 *
 * @param play Play engine.
 */
static void xboard_check_game_over(Play *play)
{
	int n_discs[2];
	const Board *board = play->board;
	const char *(color[2]) = {"Black", "White"};
	const int player = WHITE; // !!!!
	const int opponent = !player;

	if (play_is_game_over(play)) {
		n_discs[play->player] = bit_count(board->player);
		n_discs[!play->player] = bit_count(board->opponent);
		if (n_discs[player] > n_discs[opponent]) xboard_send("1-0 {%s wins %d-%d}\n", color[player], n_discs[player], n_discs[opponent]);
		else if (n_discs[player] < n_discs[opponent]) xboard_send("0-1 {%s wins %d-%d}\n", color[opponent], n_discs[opponent], n_discs[player]);
		else xboard_send("1/2-1/2 {Draw %d-%d}\n", n_discs[player], n_discs[opponent]);
	}
}

/**
 * Compute the hash tables size in MB
 */
static inline int hash_size(int n)
{
	unsigned long long s = sizeof (Hash) << n;
	return ((s << 1) + (s >> 4) + sizeof (HashTable) * 3) >> 20;
}

/**
 * Search for a move.
 *
 * @param ui User Interface.
 * @param stats total nodes & time statistics.
 */
static void xboard_go(UI *ui, XBoardStats *stats)
{
	Play *play = ui->play;
	Search *search = play->search;
	Result *result = search->result;

	play_go(play, true);
	xboard_move(play_get_last_move(play)->x);
	play_ponder(play);
	xboard_check_game_over(play);
	
	stats->time += result->time;
	stats->n_nodes += result->n_nodes;

	if (log_is_open(xboard_log)) {
		if (search->stop == STOP_TIMEOUT) fprintf(xboard_log->f, "edax search> stop on time-out\n");
		else if (search->stop == STOP_ON_DEMAND) fprintf(xboard_log->f, "edax search> stop on user demand\n");
		else if (search->stop == STOP_PONDERING) fprintf(xboard_log->f, "edax search> BUG: stop pondering ???\n");
		else if (search->stop == STOP_END) fprintf(xboard_log->f, "edax search> search completed!\n");
		else  fprintf(xboard_log->f, "edax search> BUG: search stopped for no reason ???\n");
		fprintf(xboard_log->f, "edax search> time spent = %.2f; depth reached = %d@%d%%; nodes = %lld\n",
			0.001 * result->time,  result->depth, selectivity_table[result->selectivity].percent, result->n_nodes);
		fprintf(xboard_log->f, "edax search> best score = %d; pv = ", result->score);
		line_print(result->pv, 100, NULL, xboard_log->f);
		putc('\n', xboard_log->f);
		fflush(xboard_log->f);
	}
}


/**
 * @brief Stop analyzing.
 *
 * If edax is analyzing, stop the search, and wait that the analyzing thread
 * is suspended.
 *
 * @param play Play.
 */
void xboard_stop_analyzing(Play *play)
{
	while (play->state == IS_ANALYZING) {
		log_print(xboard_log, "edax (analyze)> stop\n");
		search_stop_all(play->search, STOP_PONDERING);
		relax(10);
	}

	if (play->ponder->launched) {
		thread_join(play->ponder->thread);
		play->ponder->launched = false;
		log_print(xboard_log, "edax (analyze)> stopped\n");
	}
}

/**
 * @brief Analyze.
 *
 * Analyze a position. Activate the thread suspended in
 * xboard_loop_analyze.
 *
 * @param play Play.
 */
static void xboard_analyze(Play *play)
{
	play_stop_pondering(play);
	xboard_stop_analyzing(play);

	if (play_is_game_over(play)) return;
	if (play->state == IS_WAITING) {
		play->ponder->board->player = play->ponder->board->opponent = 0;
		play->state = IS_ANALYZING;		
		search_cleanup(play->search);
		log_print(xboard_log, "edax (analyze)> start\n");
		thread_create2(&play->ponder->thread, play_ponder_run, play); // modified for iOS by lavox. 2018/1/16
		play->ponder->launched = true;
	}
}

/**
 * @brief Analyze.
 *
 * Analyze loop.
 *
 * @param ui User Interface.
 */
static void xboard_loop_analyze(UI *ui)
{
	Play *play = ui->play;
	char *cmd = NULL, *param = NULL;
	Search *search = play->search;

	play_stop_pondering(play);
	xboard_analyze(play);

	for (;;) {
		errno = 0;

		ui_event_wait(ui, &cmd, &param);
		log_print(xboard_log, "xboard (analyze)> %s %s\n", cmd, param);

		if (strcmp(cmd, ".") == 0) {
			spin_lock(search->result);
			xboard_send("stat01: %d %lld %d %d %d\n",
				search_time(search) / 10, search_count_nodes(search), search->depth, search->result->n_moves_left, search->result->n_moves);
			spin_unlock(search->result);

		} else if (strcmp(cmd, "hint") == 0) {
			xboard_hint(play);			

		} else if (strcmp(cmd, "bk") == 0) {
			xboard_book(play);			

		} else if (strcmp(cmd, "new") == 0) {
			xboard_stop_analyzing(play);
			play_new(play);
			xboard_analyze(play);
			
		} else if (strcmp(cmd, "undo") == 0) {
			xboard_stop_analyzing(play);
			play_undo(play);
			xboard_analyze(play);

		} else if ((strcmp(cmd, "setboard") == 0)) {
			xboard_stop_analyzing(play);
			play_set_board_from_FEN(play, param);
			if (play->initial_player == EMPTY) xboard_error("(bad FEN): %s\n", param);
			xboard_analyze(play);

		} else if (strcmp(cmd, "exit") == 0) {
			xboard_stop_analyzing(play);
			free(cmd); free(param);
			ui_loop_xboard(ui);
			return;

		} else if (strcmp(cmd, "quit") == 0) {
			xboard_stop_analyzing(play);
			free(cmd); free(param);
			return;

		} else {
			xboard_stop_analyzing(play);
			if (play_user_move(play, cmd)) {

			/* illegal cmd/move */
			} else if (play_is_game_over(play) && strcmp(cmd, "@@@@") == 0) {
					// Tolerate a pass when the game is over...
			} else {
				xboard_send("Illegal move: %s %s\n", cmd, param);
			}
			xboard_analyze(play);
		}
	}
}

/**
 * @brief Loop event.
 * @param ui user interface.
 */
void ui_loop_xboard(UI *ui)
{
	char *cmd = NULL, *param = NULL;
	Play *play = ui->play;
	bool alien_variant = false;
	XBoardStats stats = {0, 0, 0};
	int edax_turn = EMPTY;
	int last_edax_turn = !play->player;
	const char *(color[2]) = {"black", "white"};
	
	// loop forever
	for (;;) {
		errno = 0;

		if (!ui_event_exist(ui) && !play_is_game_over(play) && (edax_turn == play->player)) {
			log_print(xboard_log, "edax (auto_play)> turn = %s\n", color[edax_turn]);
			xboard_go(ui, &stats);

		// proceed by reading a command
		} else {

			ui_event_wait(ui, &cmd, &param);
			log_print(xboard_log, "xboard> %s %s\n", cmd, param);

			if (cmd == NULL) {
				xboard_error("(unexpected null command)");
				continue;
			}

			// skip empty lines or commented lines
			if (*cmd == '\0' || *cmd == '#') {

			// xboard
			} else if (strcmp(cmd, "xboard") == 0) {
			
			// protover 2
			} else if (strcmp(cmd, "protover") == 0) {
				int version = string_to_int(param, 1);
				if (version >= 2) {
					xboard_send("feature "
						   "setboard=1 "
						   "playother=1 "
						   "ping=1 "
						   "draw=0 "
						   "sigint=0 "
						   "sigterm=0 "
						   "analyze=1 "
		                   "myname=\"%s\" "
						   "variants=\"reversi\" "
						   "colors=0 "
		                   "nps=1 "
		                   "memory=1 "
		                   "smp=1 "
		                   "done=1\n", options.name);
				}

			// accepted features
			} else if (strcmp(cmd, "accepted") == 0) {

			// accepted features
			} else if (strcmp(cmd, "debug") == 0) {
				play_print(play, stdout);

			// rejected features
			} else if (strcmp(cmd, "rejected") == 0) {
				char feature[16], *value;
				value = parse_word(param, feature, 15);
				// fatal error if reversi variant is rejected
				if (strcmp(feature, "variants") == 0 && strcmp(value, "reversi") == 0) {
					free(cmd), free(param);
					xboard_error("(Reversi only is supported)");
					return;
				}

			// new
			} else if (strcmp(cmd, "new") == 0 ) {
				options.level = 60;
				play->initial_player = board_from_FEN(play->initial_board, "8/8/8/3Pp3/3pP3/8/8/8 w - - 0 1");
				play_new(play);
				edax_turn = !play->player;
				if (alien_variant) xboard_setup(play);

			// variant
			} else if (strcmp(cmd, "variant") == 0 ) {
				string_to_lowercase(param);
				if (strcmp(param, "alien") == 0) {
					alien_variant = true;
				} else if (strcmp(param, "reversi") == 0) {
					alien_variant = false;
				} else {
					xboard_error("(Unsupported variant) '%s'", param);
				}		
				xboard_setup(play);

			// quit
			} else if (strcmp(cmd, "quit") == 0 || strcmp(cmd, "eof") == 0 || strcmp(cmd, "q") == 0) {
				xboard_send("%d games played in %.2f s. %llu nodes searched\n", stats.n_games, 0.001 * stats.time, stats.n_nodes);
				free(cmd); free(param);
				if (log_is_open(xboard_log)) statistics_print(xboard_log->f);
				return;

			// random command
			} else if ((strcmp(cmd, "random") == 0)) {
				// ignored command;
			
			// random command
			} else if ((strcmp(cmd, "force") == 0)) {
				play_stop_pondering(play);
				last_edax_turn = edax_turn;
				edax_turn = EMPTY;

			// go think!
			} else if (strcmp(cmd, "go") == 0) {
				edax_turn = play->player;
				xboard_go(ui, &stats);

			// playother
			} else if (strcmp(cmd, "playother") == 0) {
				edax_turn = play->player;
				play_ponder(play);

			// white turn (obsolete in protocol version 2)
			} else if (strcmp(cmd, "white") == 0) {
				xboard_error("(unknown command): %s %s", cmd, param);

			// black turn (obsolete in protocol version 2)
			} else if (strcmp(cmd, "black") == 0) {
				xboard_error("(unknown command): %s %s", cmd, param);

			// time control
			// TODO: enhance all time support
			} else if (strcmp(cmd, "level") == 0) {
				int mps, base, inc, m, s;
				char *next;
				
				mps = 0; next = parse_int(param, &mps);			
				m = s = 0; next = parse_skip_spaces(parse_int(next, &m));
				if (*next == ':') next = parse_int(next + 1, &s);
				base = 60 * m + s;
				inc = 0; next = parse_int(next, &inc);

				if ((mps == 0 || mps > 30) && inc == 0) {
					options.time = 1000ull * base ;
					options.play_type = EDAX_TIME_PER_GAME;
					log_print(xboard_log, "edax setup> time per game = %.2f s.\n", 0.001 * options.time);
				} else {
					int t1 = base * 1000ull / mps;
					int t2 = (base + inc * mps) * 30; // 30 <- 1000ms / 30moves
					options.time = MIN(t1, t2);
					options.play_type = EDAX_TIME_PER_MOVE;
					log_print(xboard_log, "edax setup> time per move = %.2f s.\n", 0.001 * options.time);
				}
				
			// time control
			} else if (strcmp(cmd, "st") == 0) {
				options.time = string_to_time(param);
				options.play_type = EDAX_TIME_PER_MOVE;
				log_print(xboard_log, "edax setup> time per move = %.2f s.\n", 0.001 * options.time);

			// depth level
			} else if (strcmp(cmd, "sd") == 0) {
				options.level = string_to_int(param, 60);
				BOUND(options.level, 0, 60, "level");
				log_print(xboard_log, "edax setup> fixed level = %d\n", options.level);

			// nps
			} else if ((strcmp(cmd, "nps") == 0)) {
				options.nps = 0.001 * string_to_real(param, options.nps);

			// time
			} else if ((strcmp(cmd, "time") == 0)) {
				int t;
				t = string_to_int(param, 100); 
				if (t > 6000) t -= 1000; else if (t > 1000) t -= 100; // keep a margin
				play->time[edax_turn == EMPTY ? last_edax_turn : edax_turn].left = t * 10; // 100% of available time...

			} else if ((strcmp(cmd, "otim") == 0)) {
				int t;
				t = string_to_int(param, 100);
				if (t > 6000) t -= 1000; else if (t > 1000) t -= 100; // keep a margin
				play->time[edax_turn == EMPTY ? !last_edax_turn : !edax_turn].left = t * 10; // 100% of available time...

			// interrupt thinking
			} else if (strcmp(cmd, "?") == 0) {
				// processed in the event loop

			// ping
			} else if ((strcmp(cmd, "ping") == 0)) {
				xboard_send("pong %s\n", param);

			// draw
			} else if ((strcmp(cmd, "draw") == 0)) {
				// never accept draw... terminating a solved game should take no time
		
			// result		
			} else if ((strcmp(cmd, "result") == 0)) {
				++stats.n_games;
				if (options.auto_store) {
					long t = real_clock();
					play->book->options.verbosity = play->book->search->options.verbosity = 0;				
					log_print(xboard_log, "edax learning>\n");		
					play_store(play);
					log_print(xboard_log, "edax learning> done in %.2fs\n", 0.001 * (real_clock() - t));
				}
			
			// setboard
			} else if ((strcmp(cmd, "setboard") == 0)) {
				play_set_board_from_FEN(play, param);
				if (play->initial_player == EMPTY) xboard_error("(bad FEN): %s\n", param);
				xboard_check_game_over(play);

			} else if ((strcmp(cmd, "edit") == 0)) {
				xboard_error("(unknown command): %s %s", cmd, param);

			} else if ((strcmp(cmd, "hint") == 0)) {
				xboard_hint(play);

			} else if ((strcmp(cmd, "bk") == 0)) {
				xboard_book(play);		

			} else if ((strcmp(cmd, "undo") == 0)) {
				play_undo(play);
			
			} else if ((strcmp(cmd, "remove") == 0)) {
				play_undo(play);
				play_undo(play);

			} else if ((strcmp(cmd, "hard") == 0)) {
				options.can_ponder = true;
				if (edax_turn != play->player) play_ponder(play);

			} else if ((strcmp(cmd, "easy") == 0)) {
				options.can_ponder = false;
				play_stop_pondering(play);

			} else if ((strcmp(cmd, "post") == 0)) {
				options.verbosity = 2;

			} else if ((strcmp(cmd, "nopost") == 0)) {
				options.verbosity = 0;

			} else if ((strcmp(cmd, "analyze") == 0)) {
				free(cmd); free(param);
				xboard_loop_analyze(ui);
				return;

			} else if ((strcmp(cmd, "name") == 0)) {
				xboard_send("Hello %s!\n", param);

			} else if ((strcmp(cmd, "rating") == 0)) {

			} else if ((strcmp(cmd, "ics") == 0)) {

			} else if ((strcmp(cmd, "computer") == 0)) {

			} else if ((strcmp(cmd, "pause") == 0)) {
				xboard_error("(unknown command): %s %s", cmd, param);

			} else if ((strcmp(cmd, "resume") == 0)) {
				xboard_error("(unknown command): %s %s", cmd, param);

			} else if ((strcmp(cmd, "memory") == 0)) {
				int size = string_to_int(param, 100);
				
				for (options.hash_table_size = 10; hash_size(options.hash_table_size + 1) < size; ++options.hash_table_size) ;
				BOUND(options.hash_table_size, 10, 30, "hash-table-size");
				log_print(xboard_log, "edax setup> hash table size: 2**%d entries\n", options.hash_table_size);
				play_stop_pondering(play);
				search_resize_hashtable(play->search);
			
			} else if ((strcmp(cmd, "cores") == 0)) {
				options.n_task = string_to_int(param, 1);
				log_print(xboard_log, "edax setup> cores: %d\n", options.n_task);
				if (search_count_tasks(play->search) != options.n_task) {
					play_stop_pondering(play);
					search_set_task_number(play->search, options.n_task);
				}

			} else if ((strcmp(cmd, "egtpath") == 0)) {
				xboard_error("(unknown command): %s %s", cmd, param);
				
			} else if ((strcmp(cmd, "option") == 0)) {
				xboard_error("(unknown command): %s %s", cmd, param);

			// move
			} else if (strcmp(cmd, "usermove") == 0) {
				if (!play_user_move(play, param)) {
					xboard_send("Illegal move %s\n", param);
				}
				xboard_check_game_over(play);
				if (alien_variant) xboard_setup(play);

			} else if (play_user_move(play, cmd)) {
				xboard_check_game_over(play);
				if (alien_variant) xboard_setup(play);

			/* illegal cmd/move */
			} else {
				if (play_is_game_over(play) && strcmp(cmd, "@@@@") == 0) {
					// Tolerate a pass when the game is over...
				} else {
					xboard_send("Illegal move: %s %s\n", cmd, param);
				}
			}
		}
	}
}
