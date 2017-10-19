/**
 * @file edax.c
 *
 * @brief Edax protocol.
 *
 * This is version 4.4 of Edax User Interface. Several changes
 * occurred between this version and 3.x previous versions because of the
 * evolution of the search engine. Here is a summary of the commands:
 *
 * Options:
 * Options must be entered in the form '[set] <option> [=] <value>', with [set] and\n[=] being optional.
 *   -verbose [n]          set Edax verbosity (default 1).
 *   -noise [n]            start displaying Edax search result from this depth\n  (default 5).
 *   -witdh [n]            display edax search results using <width> characters\n  (default 80).
 *   -hash-table-size [n]  set hashtable size (default 18 bits).
 *   -n-tasks [n]          control the number of parallel threads used in searching\n  (default 1).
 *   -l|level [n]          search using limited depth (default 21).
 *   -t|game-time <time>   search using limited time per game.
 *   -move-time <time>     search using limited time per move.
 *   -ponder [on/off]      set pondering on/off.
 *   -book-file [file]     use [file] as default book file (default data/book.dat).
 *   -book-usage [on/off]  use or do not use the opening book.
 *   -book-randomness [n]  play various but worse moves from the opening book.
 *   -auto-start [on/off]  automatically start a new game.
 *   -auto-swap [on/off]   automatically swap players between each game.
 *   -auto-store [on/off]  automatically store each game into the opening book.
 *
 * Commands:
 * Commands must be entered in the form '<command> <parameters>'.
 *   -i|nit               start a new game from standard initial position.
 *   -n|ew                start a new game from a personalized position.
 *   -setboard <board>    set a personalized position to start from.
 *   -o|open|load [file]  load a played game.
 *   -s|save [file]       save a played game.
 *   -q|quit|exit         quit from edax.
 *   -u|undo              undo the last played move.
 *   -r|redo              redo the last played move.
 *   -play <moves>        play a sequence of moves.
 *   -force <moves>       force to play an opening.
 *   -go                  ask edax to play.
 *   -stop                stop edax search.
 *   -hint [n]            ask edax to search the first bestmoves.
 *   -m|mode [n]          ask edax to automatically play (default = 3).
 *   -a|analyze [n]       retro-analyze the game.
 *   -?|help              show this message.
 *   -v|version           display the version number.
 *
 * Book Commands:
 * Book Commands must be entered in the form 'b|book <command> <parameters>'.
 *   -new <n1> <n2>       create a new empty book with level <n1> and depth <n2>.
 *   -load [file]         load an opening book from a binary opening file.
 *   -merge [file]        merge an opening book with the current opening book.
 *   -save [file]         save an opening book to a binary opening file.
 *   -import [file]       load an opening book from a portable text file.
 *   -export [file]       save an opening book to a portable text file.
 *   -on                  use the opening book.
 *   -off                 do not use the opening book.
 *   -show                display details about the current position.
 *   -info                display book general information.
 *   -a|analyze [n]       retro-analyze the game using the opening book.
 *   -randomness [n]      play more various but worse move from the opening book.
 *   -depth [n]           change book depth (up to which to add positions).
 *   -deepen [n]          change book level & reevalute the whole book (very slow!).
 *   -fix                 fix the opening book: add missing links and negamax the\n  whole book tree.
 *   -store               add the last played game to the opening book.
 *   -deviate <n1> <n2>   add positions by deviating with a relative error <n1> and\n  an absolute error <n2>.
 *   -enhance <n1> <n2>   add positions by improving score accuracy with a midgame\n  error <n1> and an endcut error <n2>.
 *   -fill [n]            add positions between existing positions.
 *   -prune               remove unreachable positions.
 *   -add [file]          add positions from a game base file (txt, ggf, sgf or\n  wthor format).
 *
 * Game DataBase Commands:
 *   -convert [file_in] [file_out]     convert between different format.
 *   -unique [file_in] [file_out]      remove doublons in the base.
 *   -check [file_in] [n]              check error in the last <n> moves.
 *   -correct [file_in] [n]            correct error in the last <n> moves.
 *   -complete [file_in]               complete a database by playing the last\n  missing moves.
 *   -problem [file_in] [n] [file_out] build a set of <n> problems from a game\n  database.
 *
 * Tests commands:
 *   -solve [file]        solve a set of positions.
 *   -obftest [file]      Test from an obf file.
 *   -script-to-obf [file]Convert a script to an obf file.
 *   -wtest [file]        check the theoric scores of a wthor base file.
 *   -count games [d]     compute the number of moves from the current position up\n  to depth [d].
 *   -perft [d]           same as above, but without hash table.
 *   -estimate [d] [n]    estimate the number of moves from the current position up\n  to depth [d].
 *   -count positions [d] compute the number of positions from the current position\n  up to depth [d].
 *   -count shapes [d]    compute the number of shapes from the current position up\n  to depth [d].
 *
 *
 * @date 1998 - 2017
 * @author Richard Delorme
 * @version 4.4
 *
 */

#include "cassio.h"
#include "event.h"
#include "histogram.h"
#include "options.h"
#include "opening.h"
#include "obftest.h"
#include "perft.h"
#include "play.h"
#include "search.h"
#include "util.h"
#include "ui.h"
#ifdef __linux__
	#include <sys/time.h>
	#include <sys/resource.h>
#endif

static Log edax_log[1];
extern bool book_verbose;

void version(void);
void bench(void);

/**
 * @brief default search oberver.
 * @param result Search Result.
 */
static void edax_observer(Result *result)
{
	search_observer(result);
	if (log_is_open(edax_log)) {
		result_print(result, edax_log->f);
		putc('\n', edax_log->f);
	}
}

/**
 * @brief initialize edax protocol.
 * @param ui user interface.
 */
void ui_init_edax(UI *ui)
{
	Play *play = ui->play;

	book_verbose = true;
	play_init(play, ui->book);
	ui->book->search = play->search;
	book_load(ui->book, options.book_file);
	play->search->id = 1;
	search_set_observer(play->search, edax_observer);
	ui->mode = options.mode;
	play->type = ui->type;

	log_open(edax_log, options.ui_log_file);
}

/**
 * @brief free resources used by edax protocol.
 * @param ui user interface.
 */
void ui_free_edax(UI *ui)
{
	if (ui->book->need_saving) book_save(ui->book, options.book_file);
	book_free(ui->book);
	play_free(ui->play);
	log_close(edax_log);
	book_verbose = false;
}

/**
 * @brief print help about options.
 */
void help_options(void) 
{
	printf("Options:\n");
	printf("Options must be entered in the form '[set] <option> [=] <value>', with [set] and\n[=] being optional.\n");
	printf("  verbose [n]          set Edax verbosity (default 1).\n");
	printf("  noise [n]            start displaying Edax search result from this depth\n  (default 5).\n");
	printf("  witdh [n]            display edax search results using <width> characters\n  (default 80).\n");
	printf("  hash-table-size [n]  set hashtable size (default 18 bits).\n");
	printf("  n-tasks [n]          control the number of parallel threads used in searching\n  (default 1).\n");
	printf("  l|level [n]          search using limited depth (default 21).\n");
	printf("  t|game-time <time>   search using limited time per game.\n");
	printf("  move-time <time>     search using limited time per move.\n");
	printf("  ponder [on/off]      set pondering on/off.\n");
	printf("  book-file [file]     use [file] as default book file (default data/book.dat).\n");
	printf("  book-usage [on/off]  use or do not use the opening book.\n");
	printf("  book-randomness [n]  play various but worse moves from the opening book.\n");
	printf("  auto-start [on/off]  automatically start a new game.\n");
	printf("  auto-swap [on/off]   automatically swap players between each game.\n");
	printf("  auto-store [on/off]  automatically store each game into the opening book.\n");
}

/**
 * @brief print help commands.
 */
void help_commands(void) 
{
	printf("\nCommands:\n");
	printf("Commands must be entered in the form '<command> <parameters>'.\n");
	printf("  i|nit               start a new game from standard initial position.\n");
	printf("  n|ew                start a new game from a personalized position.\n");
	printf("  setboard <board>    set a personalized position to start from.\n");
	printf("  o|open|load [file]  load a played game.\n");
	printf("  s|save [file]       save a played game.\n");
	printf("  q|quit|exit         quit from edax.\n");
	printf("  u|undo              undo the last played move.\n");
	printf("  r|redo              redo the last played move.\n");
	printf("  play <moves>        play a sequence of moves.\n");
	printf("  force <moves>       force to play an opening.\n");
	printf("  go                  ask edax to play.\n");
	printf("  stop                stop edax search.\n");
	printf("  hint [n]            ask edax to search the first bestmoves.\n");
	printf("  m|mode [n]          ask edax to automatically play (default = 3).\n");
	printf("  a|analyze [n]       retro-analyze the game.\n");
	printf("  ?|help              show this message.\n");
	printf("  v|version           display the version number.\n");
}

/**
 * @brief print book's help.
 */
void help_book(void) 
{
	printf("\nBook Commands:\n");
	printf("Book Commands must be entered in the form 'b|book <command> <parameters>'.\n");
	printf("  new <n1> <n2>       create a new empty book with level <n1> and depth <n2>.\n");
	printf("  load [file]         load an opening book from a binary opening file.\n");
	printf("  merge [file]        merge an opening book with the current opening book.\n");
	printf("  save [file]         save an opening book to a binary opening file.\n");
	printf("  import [file]       load an opening book from a portable text file.\n");
	printf("  export [file]       save an opening book to a portable text file.\n");
	printf("  on                  use the opening book.\n");
	printf("  off                 do not use the opening book.\n");
	printf("  show                display details about the current position.\n");
	printf("  info                display book general information.\n");
	printf("  a|analyze [n]       retro-analyze the game using the opening book.\n");
	printf("  randomness [n]      play more various but worse move from the opening book.\n");
	printf("  depth [n]           change book depth (up to which to add positions).\n");
	printf("  deepen [n]          change book level & reevalute the whole book (very slow!).\n");
	printf("  fix                 fix the opening book: add missing links and negamax the\n  whole book tree.\n");
	printf("  store               add the last played game to the opening book.\n");
	printf("  deviate <n1> <n2>   add positions by deviating with a relative error <n1> and\n  an absolute error <n2>.\n");
	printf("  enhance <n1> <n2>   add positions by improving score accuracy with a midgame\n  error <n1> and an endcut error <n2>.\n");
	printf("  fill [n]            add positions between existing positions.\n");
	printf("  prune               remove unreachable positions.\n");
	printf("  subtree             only keep positions from the current position.\n");
	printf("  add [file]          add positions from a game base file (txt, ggf, sgf or\n  wthor format).\n");
}

/**
 * @brief print base's help.
 */
void help_base(void) 
{
	printf("\nGame DataBase :\n");
	printf("  convert [file_in] [file_out]     convert between different format.\n");
	printf("  unique [file_in] [file_out]      remove doublons in the base.\n");
	printf("  check [file_in] [n]              check error in the last <n> moves.\n");
	printf("  correct [file_in] [n]            correct error in the last <n> moves.\n");
	printf("  complete [file_in]               complete a database by playing the last\n  missing moves.\n");
	printf("  problem [file_in] [n] [file_out] build a set of <n> problems from a game\n  database.\n");
}

/**
 * @brief print base's help.
 */
void help_test(void) 
{
	printf("\nTests:\n");
	printf("  bench               test edax speed.\n");
	printf("  microbench          test CPU cycle speed of some major functions.\n");
	printf("  obftest [file]      Test from an obf file.\n");
	printf("  script-to-obf [file]Convert a script to an obf file.\n");
	printf("  wtest [file]        check the theoric scores of a wthor base file.\n");
	printf("  count games [d]     compute the number of moves from the current position up\n  to depth [d].\n");
	printf("  perft [d]           same as above, but without hash table.\n");
	printf("  estimate [d] [n]    estimate the number of moves from the current position up\n  to depth [d].\n");
	printf("  count positions [d] compute the number of positions from the current position\n  up to depth [d].\n");
	printf("  count shapes [d]    compute the number of shapes from the current position up\n  to depth [d].\n");
}


/**
 * @brief Loop event.
 * @param ui user interface.
 */
void ui_loop_edax(UI *ui)
{
	char *cmd = NULL, *param = NULL;
	Play *play = ui->play;
	char book_file[FILENAME_MAX];
	unsigned long long histogram[129][65];
	int repeat = options.repeat;

	histogram_init(histogram);

	// loop forever
	for (;;) {
		errno = 0;

		if (options.verbosity) {
			putchar('\n');
			play_print(play, stdout);
			if (play_is_game_over(play)) printf("\n*** Game Over ***\n");
			putchar('\n');
		}

		if (log_is_open(edax_log)) {
			putc('\n', edax_log->f);
			play_print(play, edax_log->f);
			if (play_is_game_over(play)) fputs("\n*** Game Over ***\n", edax_log->f);
			putc('\n', edax_log->f);
		}

		// edax turn ? (automatic play mode)
		if (!ui_event_exist(ui) && !play_is_game_over(play) && (ui->mode == !play->player || ui->mode == 2)) {
			putchar('\n');
			play_go(play, true);
			printf("\nEdax plays "); move_print(play_get_last_move(play)->x, 0, stdout); putchar('\n');
			if (ui->mode != 2) play_ponder(play);

		// proceed by reading a command
		} else {

			/* automatic rules after a game over*/
			if (play_is_game_over(play)) {
				if (options.auto_store) play_store(play);
				if (options.auto_swap && ui->mode < 2) ui->mode = !ui->mode;
				if (options.repeat && repeat > 1) {
					--repeat;
					play_new(play);
					continue;
				}
				if (options.auto_quit) {
					return;
				}
				if (options.auto_start) {
					play_new(play);
					continue;
				}
			}

			putchar('>'); fflush(stdout);
			ui_event_wait(ui, &cmd, &param);
			log_print(edax_log, "cmd=\"%s\" ; param=\"%s\"\n", cmd, param);
			putchar('\n');

			if (cmd == NULL) {
				warn("unexpected null command?\n");
				continue;
			}

			// skip empty lines or commented lines
			if (*cmd == '\0' || *cmd == '#') {
			
			// help
			} else if (strcmp(cmd, "help") == 0 || strcmp(cmd, "?") == 0) {
				if (*param == '\0' || strcmp(param, "options") == 0) help_options();
				if (*param == '\0' || strcmp(param, "commands") == 0) help_commands();
				if (*param == '\0' || strcmp(param, "book") == 0) help_book();
				if (*param == '\0' || strcmp(param, "base") == 0) help_base();
				if (*param == '\0' || strcmp(param, "test") == 0) help_test(); 

			// new game from standard position
			} else if (strcmp(cmd, "i") == 0 || strcmp(cmd, "init") == 0) {
				board_init(play->initial_board);
				play->initial_player = BLACK;
				play_force_init(play, "F5");
				play_new(play);

			// new game from personnalized position
			} else if ((strcmp(cmd, "n") == 0 || strcmp(cmd, "new") == 0) && *param == '\0') {
				play_new(play);

			// open a saved game
			} else if (strcmp(cmd, "o") == 0 || strcmp(cmd, "open") == 0 || strcmp(cmd, "load") == 0) {
				play_load(play, param);

			// save a game
			} else if (strcmp(cmd, "s") == 0 || strcmp(cmd, "save") == 0) {
				play_save(play, param);

			// quit
			} else if (strcmp(cmd, "quit") == 0 || strcmp(cmd, "q") == 0 || strcmp(cmd, "exit") == 0) {
				free(cmd); free(param);
				return;

			} else if (!options.auto_quit && (strcmp(cmd, "eof") == 0 && (ui->mode != 2 || play_is_game_over(play)))){
				free(cmd); free(param);
				return;

			// undo last move
			} else if (strcmp(cmd, "u") == 0 || strcmp(cmd, "undo") == 0) {
				play_undo(play);
				if (ui->mode == 0 || ui->mode == 1) play_undo(play);

			// redo last move
			} else if (strcmp(cmd, "r") == 0 || strcmp(cmd, "redo") == 0) {
				play_redo(play);
				if (ui->mode == 0 || ui->mode == 1) play_redo(play);

			// mode
			} else if (strcmp(cmd, "m") == 0 || strcmp(cmd, "mode") == 0) {
				ui->mode = string_to_int(param, 3);

			// analyze a game
			} else if (strcmp(cmd, "a") == 0 || strcmp(cmd, "analyze") == 0 || strcmp(cmd, "analyse") == 0) {
				play_analyze(play, string_to_int(param, play->n_game));

			// set a new initial position
			} else if (strcmp(cmd, "setboard") == 0) {
				play_set_board(play, param);

			// vertical mirror
			} else if (strcmp(cmd, "vmirror") == 0) {
				play_symetry(play, 2);

			// horizontal mirror
			} else if (strcmp(cmd, "hmirror") == 0) {
				play_symetry(play, 1);

			// rotate
			} else if (strcmp(cmd, "rotate") == 0) {
				int angle = string_to_int(param, 90) % 360;
				if (angle < 0) angle += 360;
				switch (angle) {
				case 90: 
					play_symetry(play, 5);
					break; 
				case 180:
					play_symetry(play, 3);
					break; 
				case 270:
					play_symetry(play, 6);
					break; 
				default:
					warn("Rotate angle should be 90°, 180° or 270°");
					break;
				}

			// direct symetry...
			} else if (strcmp(cmd, "symetry") == 0) {
				int sym = string_to_int(param, 1);
				if (sym < 0 || sym >= 16) warn("symetry parameter should be a number between 0 and 15\n");
				else {
					if (sym & 8) play->player ^= 1;
					play_symetry(play, sym & 7);
				}

			// play a serie of moves
			} else if (strcmp(cmd, "play") == 0) {
				string_to_lowercase(param);
				play_game(play, param);

			// force edax to play an opening
			} else if (strcmp(cmd, "force") == 0) {
				string_to_lowercase(param);
				play_force_init(play, param);

			// solve a set of problems
			} else if (strcmp(cmd, "solve") == 0) {
				char problem_file[FILENAME_MAX + 1], *hard_file;
				hard_file = parse_word(param, problem_file, FILENAME_MAX);
				parse_word(hard_file, hard_file, FILENAME_MAX);
				obf_test(play->search, problem_file, hard_file);
				search_set_observer(play->search, edax_observer);

			// convert a set of problems in a .script file to a .obf file
			} else if (strcmp(cmd, "script-to-obf") == 0) {
				char script_file[FILENAME_MAX + 1], *obf_file;
				obf_file = parse_word(param, script_file, FILENAME_MAX);
				parse_word(obf_file, obf_file, FILENAME_MAX);
				script_to_obf(play->search, script_file, obf_file);
				search_set_observer(play->search, edax_observer);

			} else if (strcmp(cmd, "select-hard") == 0) {
				char full_file[FILENAME_MAX + 1], *hard_file;
				hard_file = parse_word(param, full_file, FILENAME_MAX);
				parse_word(hard_file, hard_file, FILENAME_MAX);
				obf_filter(full_file, hard_file);

			// game/position enumeration
			} else if (strcmp(cmd, "count") == 0) {
				char count_cmd[16], *count_param;
				int depth = 10, size = 8;

				count_param = parse_word(param, count_cmd, 15);
				count_param = parse_int(count_param, &depth); BOUND(depth, 1, 90, "max-ply");
				if (count_param) parse_int(count_param, &size); BOUND(size, 6, 8, "board-size");

				if (strcmp(count_cmd, "games") == 0) { // game enumeration
					quick_count_games(play->board, depth, size);
				} else if (strcmp(count_cmd, "positions") == 0) { // position enumeration
					count_positions(play->board, depth, size);
				} else if (strcmp(count_cmd, "shapes") == 0) { // shape enumeration
					count_shapes(play->board, depth, size);
				} else {
					warn("Unknown count command: \"%s %s\"\n", cmd, param);
				}

			} else if (strcmp(cmd, "perft") == 0) {
				int depth = 14;
				depth = string_to_int(param, 10); BOUND(depth, 1, 90, "max-ply");
				count_games(play->board, depth);
			
			// game/position enumeration
			} else if (strcmp(cmd, "estimate") == 0) {
				int n = 1000;
				n = string_to_int(param, 10); BOUND(n, 1, 2000000000, "max-trials");

				estimate_games(play->board, n);
	
			// seek highest mobility
			} else if (strcmp(cmd, "mobility") == 0) {
				int t = 3600; // 1 hour
				t = string_to_int(param, 10); BOUND(t, 1, 3600*24*365*10, "max time");

				seek_highest_mobility(play->board, t);

			// seek a position
			} else if (strcmp(cmd, "seek") == 0) {
				Board target;
				Line solution;
				
				board_set(&target, param);
				line_init(&solution, play->player);
				
				if (seek_position(&target, play->board, &solution)) {
					printf("Solution found:\n");
					line_print(&solution, 200, " ", stdout);
					putchar('\n');
				}
			
			// microbench (a serie of low level tests).
			} else if (strcmp(cmd, "microbench") == 0) {
				bench();

			// bench (a serie of low level tests).
			} else if (strcmp(cmd, "bench") == 0) {
				int n = string_to_int(param, -1); BOUND(n, -1, 100, "n_problems");
				obf_speed(play->search, n);

			// wtest test the engine against wthor theoretical scores
			} else if (strcmp(cmd, "wtest") == 0) {
				wthor_test(param, play->search);

			// make wthor games played by "Edax (Delorme)" as "Etudes" tournament.
			} else if (strcmp(cmd, "edaxify") == 0) {
				wthor_edaxify(param);

			// wtest test the engine against wthor theoretical scores
			} else if (strcmp(cmd, "weval") == 0) {
				wthor_eval(param, play->search, histogram);
				histogram_print(histogram);
				histogram_stats(histogram);
				histogram_to_ppm("weval.ppm", histogram);

			// go think!
			} else if (strcmp(cmd, "go") == 0) {
				if (play_is_game_over(play)) printf("\n*** Game Over ***\n");
				else {
					play_go(play, true);
					printf("\nEdax plays "); move_print(play_get_last_move(play)->x, 0, stdout); putchar('\n');
				}

			// hint for [n] moves
			} else if (strcmp(cmd, "hint") == 0) {
				int n = string_to_int(param, 1); BOUND(n, 1, 60, "n_moves");
				play_hint(play, n);

			// stop thinking
			} else if (strcmp(cmd, "stop") == 0) {
				ui->mode = 3;

			// stop thinking
			} else if (strcmp(cmd, "version") == 0 || strcmp(cmd, "v") == 0) {
				version();

			// user move
			} else if (play_user_move(play, cmd)) {
				printf("\nYou play "); move_print(play_get_last_move(play)->x, 0, stdout); putchar('\n');

			// debug pv
			} else if (strcmp(cmd, "debug-pv") == 0) {
				Move move[1];
				if (parse_move(param, play->board, move) != param) {
					search_set_board(play->search, play->board, play->player);
					pv_debug(play->search, move, stdout);
				}
			} else if (strcmp(cmd, "options") == 0) {
					options_dump(stdout);
#ifdef __unix__
			} else if (strcmp(cmd, "resources") == 0) {
				struct rusage u;
				long long t;
	 			getrusage(RUSAGE_SELF, &u);
				t = 1000 * u.ru_utime.tv_sec + u.ru_utime.tv_usec / 1000;
				printf("user cpu time: "); time_print(t, false, stdout); printf("\n");	
				t = 1000 * u.ru_stime.tv_sec + u.ru_stime.tv_usec / 1000;
				printf("system cpu time: "); time_print(t, false, stdout); printf("\n");	
				printf("max resident memory: %ld\n", u.ru_maxrss); 
				printf("page fault without I/O: %ld\n", u.ru_minflt); 
				printf("page fault with I/O: %ld\n", u.ru_majflt); 
				printf("number of input: %ld\n", u.ru_inblock); 
				printf("number of output: %ld\n", u.ru_oublock); 
				printf("number of voluntary context switch: %ld\n", u.ru_nvcsw); 
				printf("number of unvoluntary context switch: %ld\n\n", u.ru_nivcsw); 
#endif		
			// opening name
			} else if (strcmp(cmd, "opening") == 0) {
				const char *name;
				name = play_show_opening_name(play, opening_get_english_name);
				if (name == NULL) name = "?";
				puts(name);  

			// opening name in french
			} else if (strcmp(cmd, "ouverture") == 0) {
				const char *name;
				name = play_show_opening_name(play, opening_get_french_name);
				if (name == NULL) name = "?";
				puts(name); 

			// opening book commands
			} else if (strcmp(cmd, "book") == 0 || strcmp(cmd, "b") == 0) {
				char book_cmd[FILENAME_MAX + 1], *book_param;
				int val_1, val_2;
				Book *book = play->book;

				book->search = play->search;
				book->search->options.verbosity = book->options.verbosity;
				book_param = parse_word(param, book_cmd, FILENAME_MAX);

				// store the last played game
				if (strcmp(book_cmd, "store") == 0) {
					play_store(play);

				// turn book usage on
				} else if (strcmp(book_cmd, "on") == 0) { // learn
					options.book_allowed = true;

				// turn book usage off
				} else if (strcmp(book_cmd, "off") == 0) { // learn
					options.book_allowed = false;

				// set book randomness
				} else if (strcmp(book_cmd, "randomness") == 0) { // learn
					val_1 = 0; book_param = parse_int(book_param, &val_1);
					options.book_randomness = val_1;

				// set book depth (until which to learn)
				} else if (strcmp(book_cmd, "depth") == 0) { // learn
					val_1 = 36; book_param = parse_int(book_param, &val_1);
					book->options.n_empties = 61 - val_1;

				// create a new empty book
				} else if (strcmp(book_cmd, "new") == 0) {
					val_1 = 21; book_param = parse_int(book_param, &val_1);
					val_2 = 36;	book_param = parse_int(book_param, &val_2);
					book_free(book) ;
					book_new(book, val_1, 61 - val_2);

				// load an opening book (binary format) from the disc
				} else if (strcmp(book_cmd, "load") == 0 || strcmp(book_cmd, "open") == 0) {
					book_free(book) ;
					parse_word(book_param, book_file, FILENAME_MAX);
					book_load(book, book_file);

				// save an opening book (binary format) to the disc
				} else if (strcmp(book_cmd, "save") == 0) {
					parse_word(book_param, book_file, FILENAME_MAX);
					book_save(book, book_file);

				// import an opening book (text format)
				} else if (strcmp(book_cmd, "import") == 0) {
					book_free(book);
					parse_word(book_param, book_file, FILENAME_MAX);
					book_import(book, book_file);
					book_link(book);
					book_fix(book);
					book_negamax(book);
					book_sort(book);

				// export an opening book (text format)
				} else if (strcmp(book_cmd, "export") == 0) {
					parse_word(book_param, book_file, FILENAME_MAX);
					book_export(book, book_file);

				// merge an opening book to the current one
				} else if (strcmp(book_cmd, "merge") == 0) {
					Book src[1];
					parse_word(book_param, book_file, FILENAME_MAX);
					src->search = play->search;
					book_load(src, book_file);
					book_merge(book, src);
					book_free(src);
					warn("Book needs to be fixed before usage\n");

				// fix an opening book
				} else if (strcmp(book_cmd, "fix") == 0) {
					book_fix(book); // do nothing (or edax is buggy)
					book_link(book); // links nodes
					book_negamax(book); // negamax nodes
					book_sort(book); // sort moves

				// negamax an opening book
				} else if (strcmp(book_cmd, "negamax") == 0) {
					book_negamax(book); // negamax nodes
					book_sort(book); // sort moves

				// check and correct solved positions of the book
				} else if (strcmp(book_cmd, "correct") == 0) {
					book_correct_solved(book); // do nothing (or edax is buggy)
					book_fix(book); // do nothing (or edax is buggy)
					book_link(book); // links nodes
					book_negamax(book); // negamax nodes
					book_sort(book); // sort moves

				// prune an opening book
				} else if (strcmp(book_cmd, "prune") == 0) {
					book_prune(book); // remove unreachable lines.
					book_fix(book); // do nothing (or edax is buggy)
					book_link(book); // links nodes
					book_negamax(book); // negamax nodes
					book_sort(book); // sort moves

				// subtree an opening book
				} else if (strcmp(book_cmd, "subtree") == 0) {
					book_subtree(book, play->board); // remove unreachable lines.
					book_fix(book); // do nothing (or edax is buggy)
					book_link(book); // links nodes
					book_negamax(book); // negamax nodes
					book_sort(book); // sort moves

				// show the current position as stored in the book
				} else if (strcmp(book_cmd, "show") == 0) {
					book_show(book, play->board);

				// show book general information
				} else if (strcmp(book_cmd, "info") == 0) {
					book_info(book);

				// show book general information
				} else if (strcmp(book_cmd, "stats") == 0) {
					book_stats(book);


				// set book verbosity
				} else if (strcmp(book_cmd, "verbose") == 0) {
					parse_int(book_param, &book->options.verbosity);
					book->search->options.verbosity = book->options.verbosity;

				// analyze a game from the opening book point of view
				} else if (strcmp(book_cmd, "a") == 0 || strcmp(book_cmd, "analyze") == 0 || strcmp(book_cmd, "analyse") == 0) {
					val_1 = string_to_int(book_param, play->n_game); BOUND(val_1, 1, play->n_game, "depth");
					play_book_analyze(play, val_1);

				// add positions from a game database
				} else if (strcmp(book_cmd, "add") == 0) {
					Base base[1];
					parse_word(book_param, book_file, FILENAME_MAX);
					base_init(base);
					base_load(base, book_file);
					book_add_base(book, base);
					base_free(base);

				// check positions from a game database
				} else if (strcmp(book_cmd, "check") == 0) {
					Base base[1];
					parse_word(book_param, book_file, FILENAME_MAX);
					base_init(base);
					base_load(base, book_file);
					book_check_base(book, base);
					base_free(base);

				// extract positions
				} else if (strcmp(book_cmd, "problem") == 0) {
					val_1 = 24; book_param = parse_int(book_param, &val_1); BOUND(val_1, 0, 60, "number of empties");
					val_2 = 10; book_param = parse_int(book_param, &val_2); BOUND(val_2, 1, 1000000, "number of positions");
					book_extract_positions(book, val_1, val_2);
					
				// extract pv to a game database
				} else if (strcmp(book_cmd, "extract") == 0) {
					Base base[1];
					parse_word(book_param, book_file, FILENAME_MAX);
					base_init(base);
					book_extract_skeleton(book, base);
					base_save(base, book_file);
					base_free(base);

				// add position using the "deviate algorithm"
				} else if (strcmp(book_cmd, "deviate") == 0) {
					val_1 = 2; book_param = parse_int(book_param, &val_1); BOUND(val_1, -129, 129, "relative error");
					val_2 = 4; book_param = parse_int(book_param, &val_2); BOUND(val_2, 0, 65, "absolute error");
					book_deviate(book, play->board, val_1, val_2);

				// add position using the "enhance algorithm"
				} else if (strcmp(book_cmd, "enhance") == 0) {
					val_1 = 2; book_param = parse_int(book_param, &val_1); BOUND(val_1, 0, 129, "midgame error");
					val_2 = 4; book_param = parse_int(book_param, &val_2); BOUND(val_2, 0, 129, "endcut error");
					book_enhance(book, play->board, val_1, val_2);

				// add position by filling hole in the book
				} else if (strcmp(book_cmd, "fill") == 0) {
					val_1 = 1; book_param = parse_int(book_param, &val_1); BOUND(val_1, 1, 61, "fill depth");
					book_fill(book, val_1);

				// add positions by expanding positions with no-link
				} else if (strcmp(book_cmd, "play") == 0) {
					book_play(book);

				// add positions by expanding positions with no-link
				} else if (strcmp(book_cmd, "deepen") == 0) {
					book_deepen(book);

				// add book positions to the hash table
				} else if (strcmp(book_cmd, "feed-hash") == 0) {
					book_feed_hash(book, play->board, play->search);

				// wrong command ?
				} else {
					warn("Unknown book command: \"%s %s\"\n", cmd, param);
				}
				book->options.verbosity = book->search->options.verbosity;
				book->search->options.verbosity = options.verbosity;

			/* base TODO: add more actions... */
			} else if (strcmp(cmd, "base") == 0) {
				char base_file[FILENAME_MAX + 1];
				char base_cmd[512], *base_param;
				Base base[1];

				base_init(base);
				base_param = parse_word(param, base_cmd, 511);
				base_param = parse_word(base_param, base_file, FILENAME_MAX);

				// extract problem from a game base
				if (strcmp(base_cmd, "problem") == 0) {
					char problem_file[FILENAME_MAX + 1];
					int n_empties = 24;
					base_param = parse_int(base_param, &n_empties);
					base_param = parse_word(base_param, problem_file, FILENAME_MAX);

					base_load(base, base_file);
					base_to_problem(base, n_empties, problem_file);

				// extract FEN 
				} else if (strcmp(base_cmd, "tofen") == 0) {
					char problem_file[FILENAME_MAX + 1];
					int n_empties = 24;
					base_param = parse_int(base_param, &n_empties);
					base_param = parse_word(base_param, problem_file, FILENAME_MAX);

					base_load(base, base_file);
					base_to_FEN(base, n_empties, problem_file);
	
				// correct erroneous games
				} else if (strcmp(base_cmd, "correct") == 0) {
					int n_empties = 24;
					base_param = parse_int(base_param, &n_empties);

					base_load(base, base_file);
					base_analyze(base, play->search, n_empties, true);
					remove(base_file);
					base_save(base, base_file);

				// check erroneous games
				} else if (strcmp(base_cmd, "check") == 0) {
					int n_empties = 24;
					base_param = parse_int(base_param, &n_empties);

					base_load(base, base_file);
					base_analyze(base, play->search, n_empties, false);

				// terminate unfinished base
				} else if (strcmp(base_cmd, "complete") == 0) {
					base_load(base, base_file);
					base_complete(base, play->search);
					remove(base_file);
					base_save(base, base_file);

				// convert a base to another format
				} else if (strcmp(base_cmd, "convert") == 0) {
					base_load(base, base_file);
					base_param = parse_word(base_param, base_file, FILENAME_MAX);
					base_save(base, base_file);

				// make a base unique by removing identical games
				} else if (strcmp(base_cmd, "unique") == 0) {
					base_load(base, base_file);
					base_param = parse_word(base_param, base_file, FILENAME_MAX);
					base_unique(base);
					base_save(base, base_file);

				// compare two game bases
				} else if (strcmp(base_cmd, "compare") == 0) {
					char base_file_2[FILENAME_MAX + 1];
					base_param = parse_word(base_param, base_file_2, FILENAME_MAX);
					base_compare(base_file, base_file_2);

				} else {
					warn("Unknown base command: \"%s %s\"\n", cmd, param);
				}

				base_free(base);

			/* edax options */
			} else if (options_read(cmd, param)) {
				options_bound();
				// parallel search changes:
				if (search_count_tasks(play->search) != options.n_task) {
					play_stop_pondering(play);
					search_set_task_number(play->search, options.n_task);
				}

			/* switch to another protocol */
			} else if (strcmp(cmd, "nboard") == 0 && strcmp(param, "1") == 0) {
				free(cmd); free(param);
				play_stop_pondering(play);
				ui->free(ui);
				ui_switch(ui, "nboard");
				ui->init(ui);
				ui->loop(ui);
				return;

			} else if (strcmp(cmd, "xboard") == 0) {
				free(cmd); free(param);
				play_stop_pondering(play);
				ui->free(ui);
				ui_switch(ui, "xboard");
				ui->init(ui);
				ui->loop(ui);
				return;

			} else if (strcmp(cmd, "engine-protocol") == 0 && strcmp(param, "init") == 0) {
				free(cmd); free(param);
				play_stop_pondering(play);
				ui->free(ui);
				ui_switch(ui, "cassio");
				engine_loop();
				return;

			} else if (strcmp(cmd, "protocol_version") == 0) {
				free(cmd); free(param);
				play_stop_pondering(play);
				ui->free(ui);
				ui_switch(ui, "gtp");
				ui->init(ui);
				puts("= 2\n"); fflush(stdout);
				ui->loop(ui);
				return;

#ifdef TUNE_EDAX
			/* edax tuning */
			} else if (strcmp(cmd, "tune") == 0) {
				char problem[FILENAME_MAX];
				char *w_name;
				play_stop_pondering(play);
				w_name = parse_word(param, problem, FILENAME_MAX);
				tune_move_evaluate(play->search, problem, parse_skip_spaces(w_name));
				search_set_observer(play->search, edax_observer);
#endif
			/* illegal cmd/move */
			} else {
				warn("Unknown command/Illegal move: \"%s %s\"\n", cmd, param);
			}
		}
	}
}

