/**
 * @file libedax.c
 *
 * @brief Edax api.
 *
 * @date 2018
 * @author lavox
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

extern bool book_verbose;

void version(void);
//void bench(void);

UI* g_ui;
BenchResult* g_bench_result;

/**
 * @brief edax init function for library use.
 *
 * @param argc Number of arguments.
 * @param argv Command line arguments.
 */
DLL_API void libedax_initialize(int argc, char **argv) {
	int i, r = 0;

	// options.n_task default to system cpu number
	options.n_task = get_cpu_number();

	// options from edax.ini
	options_parse("edax.ini");

	// set verbosity
	options.verbosity = 0;

	// allocate ui
	g_ui = (UI*) malloc(sizeof *g_ui);
	if (g_ui == NULL)
		fatal_error("Cannot allocate a user interface.\n");
	g_ui->type = UI_LIBEDAX;
	g_ui->init = ui_init_libedax;
	g_ui->free = ui_free_libedax;
	g_ui->loop = NULL;

	// parse arguments
	for (i = 1; i < argc; i++) {
		char *arg = argv[i];
		while (*arg == '-')
			++arg;
		if (strcmp(arg, "v") == 0 || strcmp(arg, "version") == 0)
			version();
		else if ((r = (options_read(arg, argv[i + 1]))) > 0)
			i += r - 1;
		else
			usage();
	}
	options_bound();

	// initialize
	edge_stability_init();
	hash_code_init();
	hash_move_init();
	statistics_init();
	eval_open(options.eval_file);
	search_global_init();

	g_ui->init(g_ui);
}


/**
 * @brief edax terminate function for library use.
 */
DLL_API void libedax_terminate()
{
	if (g_ui->free) g_ui->free(g_ui);

	options_free();
	free(g_ui);
	g_ui = NULL;
}


/**
 * @brief default search oberver for libedax.
 * @param result Search Result.
 */
static void libedax_observer(Result *result)
{
	// do nothing
}

/**
 * @brief initialize libedax api.
 * @param ui user interface.
 */
void ui_init_libedax(UI *ui)
{
	Play *play = ui->play;

	book_verbose = false;
	play_init(play, ui->book);
	play->search->options.header = NULL;
	play->search->options.separator = NULL;
	ui->book->search = play->search;
	book_load(ui->book, options.book_file);
	play->search->id = 1;
	search_set_observer(play->search, libedax_observer);
	ui->mode = options.mode;
	play->type = ui->type;
}

/**
 * @brief free resources used by libedax api.
 * @param ui user interface.
 */
void ui_free_libedax(UI *ui)
{
	if (ui->book->need_saving) book_save(ui->book, options.book_file);
	book_free(ui->book);
	play_free(ui->play);
	book_verbose = false;
}

/**
 * @brief auto go with regard to mode.
 */
void auto_go() {
	if (g_ui == NULL) return;
	int repeat = options.repeat;
	Play *play = g_ui->play;
	for (;;) {
		if (!play_is_game_over(play) && (g_ui->mode == !play->player || g_ui->mode == 2)) {
			play_go(play, true);
			if (g_ui->mode != 2) play_ponder(play);
		} else {
			/* automatic rules after a game over*/
			if (play_is_game_over(play)) {
				if (options.auto_store) play_store(play);
				if (options.auto_swap && g_ui->mode < 2) g_ui->mode = !g_ui->mode;
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
				return;
			} else {
				return;
			}
		}
	}
}

/**
 * @brief init command.
 */
DLL_API void edax_init() {
	if (g_ui == NULL) return;
	Play *play = g_ui->play;
	// new game from standard position
	board_init(play->initial_board);
	play->initial_player = BLACK;
	play_force_init(play, "F5");
	play_new(play);
}

/**
 * @brief new command.
 */
DLL_API void edax_new() {
	if (g_ui == NULL) return;
	Play *play = g_ui->play;
	// new game from personnalized position
	play_new(play);
}

/**
 * @brief load command.
 * @param file file name to open
 */
DLL_API void edax_load(const char* file) {
	if (g_ui == NULL) return;
	Play *play = g_ui->play;
	// open a saved game
	play_load(play, file);
}

/**
 * @brief save command.
 * @param file file name to save
 */
DLL_API void edax_save(const char* file) {
	if (g_ui == NULL) return;
	Play *play = g_ui->play;
	// save a game
	play_save(play, file);
}

/**
 * @brief undo command.
 */
DLL_API void edax_undo() {
	if (g_ui == NULL) return;
	Play *play = g_ui->play;
	// undo last move
	play_undo(play);
	if (g_ui->mode == 0 || g_ui->mode == 1) play_undo(play);
}

/**
 * @brief redo command.
 */
DLL_API void edax_redo() {
	if (g_ui == NULL) return;
	Play *play = g_ui->play;
	// redo last move
	play_redo(play);
	if (g_ui->mode == 0 || g_ui->mode == 1) play_redo(play);
}

/**
 * @brief mode command.
 * @param mode mode to set
 */
DLL_API void edax_mode(const int mode) {
	if (g_ui == NULL) return;
	// mode
	g_ui->mode = mode;
	auto_go();
}

//TODO:
			// analyze a game
//			} else if (strcmp(cmd, "a") == 0 || strcmp(cmd, "analyze") == 0 || strcmp(cmd, "analyse") == 0) {
//				play_analyze(play, string_to_int(param, play->n_game));

/**
 * @brief setboard command.
 * @param board board to set
 */
DLL_API void edax_setboard(const char* board) {
	if (g_ui == NULL) return;
	Play *play = g_ui->play;
	// set a new initial position
	play_set_board(play, board);
}

/**
 * @brief setboard command with board object.
 * @param board board to set
 * @param turn player to play
 */
DLL_API void edax_setboard_from_obj(const Board* board, const int turn) {
	if (g_ui == NULL) return;
	Play *play = g_ui->play;
	// set a new initial position
	play_set_board_from_obj(play, board, turn);
}

/**
 * @brief vmirror command.
 */
DLL_API void edax_vmirror() {
	if (g_ui == NULL) return;
	Play *play = g_ui->play;
	// vertical mirror
	play_symetry(play, 2);
}

/**
 * @brief hmirror command.
 */
DLL_API void edax_hmirror() {
	if (g_ui == NULL) return;
	Play *play = g_ui->play;
	// horizontal mirror
	play_symetry(play, 1);
}

/**
 * @brief rotate command.
 * @param angle angle for rotation
 */
DLL_API void edax_rotate(const int angle) {
	if (g_ui == NULL) return;
	Play *play = g_ui->play;
	// rotate
	int _angle = angle % 360;
	if (_angle < 0) _angle += 360;
	switch (_angle) {
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
		break;
	}
}

/**
 * @brief symetry command.
 * @param sym symetry.
 */
DLL_API void edax_symetry(const int sym) {
	if (g_ui == NULL) return;
	Play *play = g_ui->play;
	// direct symetry...
	if (sym >= 0 && sym < 16) {
		if (sym & 8) play->player ^= 1;
		play_symetry(play, sym & 7);
	}
}

/**
 * @brief play command.
 * @param moves moves.
 */
DLL_API void edax_play(char* moves) {
	if (g_ui == NULL) return;
	Play *play = g_ui->play;
	// play a serie of moves
	string_to_lowercase(moves);
	play_game(play, moves);

	auto_go();
}

/**
 * @brief force command.
 * @param moves moves.
 */
DLL_API void edax_force(char* moves) {
	if (g_ui == NULL) return;
	Play *play = g_ui->play;
	// force edax to play an opening
	string_to_lowercase(moves);
	play_force_init(play, moves);
}

//TODO:
//			// solve a set of problems
//			} else if (strcmp(cmd, "solve") == 0) {
//				char problem_file[FILENAME_MAX + 1], *hard_file;
//				hard_file = parse_word(param, problem_file, FILENAME_MAX);
//				parse_word(hard_file, hard_file, FILENAME_MAX);
//				obf_test(play->search, problem_file, hard_file);
//				search_set_observer(play->search, edax_observer);
//
//			// convert a set of problems in a .script file to a .obf file
//			} else if (strcmp(cmd, "script-to-obf") == 0) {
//				char script_file[FILENAME_MAX + 1], *obf_file;
//				obf_file = parse_word(param, script_file, FILENAME_MAX);
//				parse_word(obf_file, obf_file, FILENAME_MAX);
//				script_to_obf(play->search, script_file, obf_file);
//				search_set_observer(play->search, edax_observer);
//
//			} else if (strcmp(cmd, "select-hard") == 0) {
//				char full_file[FILENAME_MAX + 1], *hard_file;
//				hard_file = parse_word(param, full_file, FILENAME_MAX);
//				parse_word(hard_file, hard_file, FILENAME_MAX);
//				obf_filter(full_file, hard_file);
//
//			// game/position enumeration
//			} else if (strcmp(cmd, "count") == 0) {
//				char count_cmd[16], *count_param;
//				int depth = 10, size = 8;
//
//				count_param = parse_word(param, count_cmd, 15);
//				count_param = parse_int(count_param, &depth); BOUND(depth, 1, 90, "max-ply");
//				if (count_param) parse_int(count_param, &size); BOUND(size, 6, 8, "board-size");
//
//				if (strcmp(count_cmd, "games") == 0) { // game enumeration
//					quick_count_games(play->board, depth, size);
//				} else if (strcmp(count_cmd, "positions") == 0) { // position enumeration
//					count_positions(play->board, depth, size);
//				} else if (strcmp(count_cmd, "shapes") == 0) { // shape enumeration
//					count_shapes(play->board, depth, size);
//				} else {
//					warn("Unknown count command: \"%s %s\"\n", cmd, param);
//				}
//
//			} else if (strcmp(cmd, "perft") == 0) {
//				int depth = 14;
//				depth = string_to_int(param, 10); BOUND(depth, 1, 90, "max-ply");
//				count_games(play->board, depth);
//
//			// game/position enumeration
//			} else if (strcmp(cmd, "estimate") == 0) {
//				int n = 1000;
//				n = string_to_int(param, 10); BOUND(n, 1, 2000000000, "max-trials");
//
//				estimate_games(play->board, n);
//
//			// seek highest mobility
//			} else if (strcmp(cmd, "mobility") == 0) {
//				int t = 3600; // 1 hour
//				t = string_to_int(param, 10); BOUND(t, 1, 3600*24*365*10, "max time");
//
//				seek_highest_mobility(play->board, t);
//
//			// seek a position
//			} else if (strcmp(cmd, "seek") == 0) {
//				Board target;
//				Line solution;
//
//				board_set(&target, param);
//				line_init(&solution, play->player);
//
//				if (seek_position(&target, play->board, &solution)) {
//					printf("Solution found:\n");
//					line_print(&solution, 200, " ", stdout);
//					putchar('\n');
//				}
//
//			// microbench (a serie of low level tests).
//			} else if (strcmp(cmd, "microbench") == 0) {
//				bench();
//

/**
 * @brief bench (a serie of low level tests). command.
 */
DLL_API void edax_bench(BenchResult* result, int n) {
    result->n_nodes = 0;
    result->T = 0;
    result->positions = 0;
    lock_init(result);
    g_bench_result = result;
    BOUND(n, -1, 100, "n_problems");
    if (g_ui == NULL) return;
    Play *play = g_ui->play;
    _obf_speed(play->search, n, g_bench_result);
    lock(result);
    g_bench_result = NULL;
    unlock(result);
    lock_free(result);
}

DLL_API void edax_bench_get_result(BenchResult* result) {
    if ( g_bench_result != NULL ) {
        lock(g_bench_result);
        result->T = g_bench_result->T;
        result->n_nodes = g_bench_result->n_nodes;
        result->positions = g_bench_result->positions;
        unlock(g_bench_result);
    }
}

//			// wtest test the engine against wthor theoretical scores
//			} else if (strcmp(cmd, "wtest") == 0) {
//				wthor_test(param, play->search);
//
//			// make wthor games played by "Edax (Delorme)" as "Etudes" tournament.
//			} else if (strcmp(cmd, "edaxify") == 0) {
//				wthor_edaxify(param);
//
//			// wtest test the engine against wthor theoretical scores
//			} else if (strcmp(cmd, "weval") == 0) {
//				wthor_eval(param, play->search, histogram);
//				histogram_print(histogram);
//				histogram_stats(histogram);
//				histogram_to_ppm("weval.ppm", histogram);


/**
 * @brief go command.
 */
DLL_API void edax_go() {
	if (g_ui == NULL) return;
	Play *play = g_ui->play;
	// go think!
	if (play_is_game_over(play)) return;
	play_go(play, true);

	auto_go();
}

/**
 * @brief hint command.
 * @param n number of hints.
 * @param hintlist result (out parameter).
 */
DLL_API void edax_hint(const int n, HintList* hintlist) {
	if (g_ui == NULL) return;
	Play *play = g_ui->play;
	// hint for [n] moves
	play_hint_for_lib(play, n, hintlist);
}

/**
 * @brief get book moves.
 * @param move_list result (out parameter).
 */
DLL_API void edax_get_bookmove(MoveList* move_list) {
    play_get_bookmove(g_ui->play, move_list);
}

/**
 * @brief get book moves.
 * @param move_list result (out parameter).
 */
DLL_API void edax_get_bookmove_with_position(MoveList* move_list, Position* position) {
    play_get_bookmove_with_position(g_ui->play, move_list, position);
}

/**
 * @brief hint command.
 * Call edax_hint_next after calling this function.
 */
DLL_API void edax_hint_prepare(MoveList* exclude_list) {
	if (g_ui == NULL) return;
	Play *play = g_ui->play;
	// prepare hint
	play_hint_prepare(play, exclude_list);
}

/**
 * @brief hint command.
 * Gets hint one by one. If there are no more hints, hint will be NOMOVE.
 * Call edax_hint_prepare before calling this function.
 * @param n number of hints.
 * @param hintlist result (out parameter).
 */
DLL_API void edax_hint_next(Hint* hint) {
	if (g_ui == NULL) return;
	Play *play = g_ui->play;
	// get next hint
	play_hint_next(play, hint, true);
}

/**
 * @brief hint command.
 * Gets hint one by one. If there are no more hints, hint will be NOMOVE.
 * Call edax_hint_prepare before calling this function.
 * This command is for analyze use.
 * @param n number of hints.
 * @param hintlist result (out parameter).
 */
DLL_API void edax_hint_next_no_multipv_depth(Hint* hint) {
    if (g_ui == NULL) return;
    Play *play = g_ui->play;
    // get next hint
    play_hint_next(play, hint, false);
}

/**
 * @brief stop command.
 */
DLL_API void edax_stop() {
	if (g_ui == NULL) return;
	// stop thinking
	g_ui->mode = 3;
    Play *play = g_ui->play;
    play_stop(play);
}

/**
 * @brief version command.
 */
DLL_API void edax_version() {
    version();
}

/**
 * @brief user move command.
 * @param move user move.
 * @return 1 if the move has been legally played, otherwise 0.
 */
DLL_API int edax_move(const char* move) {
	if (g_ui == NULL) return 0;
	Play *play = g_ui->play;
	// user move
	if (play_user_move(play, move) == 0) return 0;

	auto_go();
	return 1;
}

//TODO:
//			} else if (strcmp(cmd, "options") == 0) {
//					options_dump(stdout);

/**
 * @brief opening command.
 * @return opening name.
 */
DLL_API const char* edax_opening() {
	if (g_ui == NULL) return NULL;
	Play *play = g_ui->play;
	// opening name
	const char *name;
	name = play_show_opening_name(play, opening_get_english_name);
	if (name == NULL) name = "?";
	return name;
}

/**
 * @brief ouverture command.
 * @return opening name in french.
 */
DLL_API const char* edax_ouverture() {
	if (g_ui == NULL) return NULL;
	Play *play = g_ui->play;
	// opening name in french
	const char *name;
	name = play_show_opening_name(play, opening_get_french_name);
	if (name == NULL) name = "?";
	return name;
}

/**
 * @brief pre-process of book command.
 * @param ui user interface.
 */
void book_cmd_pre_process(UI* ui) {
	if (g_ui == NULL) return;
	Book *book = ui->play->book;
	book->search = ui->play->search;
	book->search->options.verbosity = book->options.verbosity;
}

/**
 * @brief post-process of book command.
 * @param ui user interface.
 */
void book_cmd_post_process(UI* ui) {
	if (g_ui == NULL) return;
	Book *book = ui->play->book;
	book->options.verbosity = book->search->options.verbosity;
	book->search->options.verbosity = options.verbosity;
}

/**
 * @brief book store command.
 */
DLL_API void edax_book_store() {
	if (g_ui == NULL) return;
	Play *play = g_ui->play;
	book_cmd_pre_process(g_ui);

	// store the last played game
	play_store(play);

	book_cmd_post_process(g_ui);
}

/**
 * @brief book on command.
 */
DLL_API void edax_book_on() {
	if (g_ui == NULL) return;
	book_cmd_pre_process(g_ui);

	// turn book usage on
	options.book_allowed = true;

	book_cmd_post_process(g_ui);
}

/**
 * @brief book off command.
 */
DLL_API void edax_book_off() {
	if (g_ui == NULL) return;
	book_cmd_pre_process(g_ui);

	// turn book usage off
	options.book_allowed = false;

	book_cmd_post_process(g_ui);
}

/**
 * @brief book randomness command.
 * @param randomness randomness.
 */
DLL_API void edax_book_randomness(const int randomness) {
	if (g_ui == NULL) return;
	book_cmd_pre_process(g_ui);

	// set book randomness
	options.book_randomness = randomness;

	book_cmd_post_process(g_ui);
}

/**
 * @brief book depth command.
 * @param depth depth.
 */
DLL_API void edax_book_depth(const int depth) {
	if (g_ui == NULL) return;
	Book *book = g_ui->play->book;
	book_cmd_pre_process(g_ui);

	// set book depth (until which to learn)
	book->options.n_empties = 61 - depth;

	book_cmd_post_process(g_ui);
}

/**
 * @brief book new command.
 * @param level level.
 * @param depth depth.
 */
DLL_API void edax_book_new(const int level, const int depth) {
	if (g_ui == NULL) return;
	Book *book = g_ui->play->book;
	book_cmd_pre_process(g_ui);

	// create a new empty book
	book_free(book) ;
	book_new(book, level, 61 - depth);

	book_cmd_post_process(g_ui);
}

/**
 * @brief book load command.
 * @param book_file book file name to load.
 */
DLL_API void edax_book_load(const char* book_file) {
	if (g_ui == NULL) return;
	Book *book = g_ui->play->book;
	book_cmd_pre_process(g_ui);

	// load an opening book (binary format) from the disc
	book_free(book) ;
	book_load(book, book_file);

	book_cmd_post_process(g_ui);
}

/**
 * @brief book save command.
 * @param book_file book file name to save.
 */
DLL_API void edax_book_save(const char* book_file) {
	if (g_ui == NULL) return;
	Book *book = g_ui->play->book;
	book_cmd_pre_process(g_ui);

	// save an opening book (binary format) to the disc
	book_save(book, book_file);

	book_cmd_post_process(g_ui);
}

/**
 * @brief book import command.
 * @param import_file file name to import.
 */
DLL_API void edax_book_import(const char* import_file) {
	if (g_ui == NULL) return;
	Book *book = g_ui->play->book;
	book_cmd_pre_process(g_ui);

	// import an opening book (text format)
	book_free(book);
	book_import(book, import_file);
	book_link(book);
	book_fix(book);
	book_negamax(book);
	book_sort(book);

	book_cmd_post_process(g_ui);
}

/**
 * @brief book export command.
 * @param export_file file name to export.
 */
DLL_API void edax_book_export(const char* export_file) {
	if (g_ui == NULL) return;
	Book *book = g_ui->play->book;
	book_cmd_pre_process(g_ui);

	// export an opening book (text format)
	book_export(book, export_file);

	book_cmd_post_process(g_ui);
}

/**
 * @brief book merge command.
 * @param book_file file name to merge.
 */
DLL_API void edax_book_merge(const char* book_file) {
	if (g_ui == NULL) return;
	Play *play = g_ui->play;
	Book *book = play->book;
	book_cmd_pre_process(g_ui);

	// merge an opening book to the current one
	Book src[1];
	src->search = play->search;
	book_load(src, book_file);
	book_merge(book, src);
	book_free(src);

	book_cmd_post_process(g_ui);
}

/**
 * @brief book fix command.
 */
DLL_API void edax_book_fix() {
	if (g_ui == NULL) return;
	Book *book = g_ui->play->book;
	book_cmd_pre_process(g_ui);

	// fix an opening book
	book_fix(book);
	book_link(book);
	book_negamax(book);
	book_sort(book);

	book_cmd_post_process(g_ui);
}

/**
 * @brief book negamax command.
 */
DLL_API void edax_book_negamax() {
	if (g_ui == NULL) return;
	Book *book = g_ui->play->book;
	book_cmd_pre_process(g_ui);

	// negamax an opening book
	book_negamax(book);
	book_sort(book);

	book_cmd_post_process(g_ui);
}

/**
 * @brief book correct command.
 */
DLL_API void edax_book_correct() {
	if (g_ui == NULL) return;
	Book *book = g_ui->play->book;
	book_cmd_pre_process(g_ui);

	// check and correct solved positions of the book
	book_correct_solved(book);
	book_fix(book);
	book_link(book);
	book_negamax(book);
	book_sort(book);

	book_cmd_post_process(g_ui);
}

/**
 * @brief book prune command.
 */
DLL_API void edax_book_prune() {
	if (g_ui == NULL) return;
	Book *book = g_ui->play->book;
	book_cmd_pre_process(g_ui);

	// prune an opening book
	book_prune(book);
	book_fix(book);
	book_link(book);
	book_negamax(book);
	book_sort(book);

	book_cmd_post_process(g_ui);
}

/**
 * @brief book subtree command.
 */
DLL_API void edax_book_subtree() {
	if (g_ui == NULL) return;
	Play *play = g_ui->play;
	Book *book = play->book;
	book_cmd_pre_process(g_ui);

	// subtree an opening book
	book_subtree(book, play->board);
	book_fix(book);
	book_link(book);
	book_negamax(book);
	book_sort(book);

	book_cmd_post_process(g_ui);
}

/**
 * @brief book show command.
 * @param position position information(out parameter).
 */
DLL_API void edax_book_show(Position *position) {
	if (g_ui == NULL) return;
	Play *play = g_ui->play;
	Book *book = play->book;
	book_cmd_pre_process(g_ui);

	// show the current position as stored in the book
	Position *p = book_show_for_api(book, play->board);
	memcpy(position, p, sizeof(Position));

	book_cmd_post_process(g_ui);
}

/**
 * @brief book info command.
 * @param book book information(out parameter).
 */
DLL_API void edax_book_info(Book *book) {
	if (g_ui == NULL) return;
	Book *srcbook = g_ui->play->book;
	book_cmd_pre_process(g_ui);

	// show book general information
	memcpy(&book->date, &srcbook->date, sizeof(book->date));
	memcpy(&book->options, &srcbook->options, sizeof(book->options));
	memcpy(&book->stats, &srcbook->stats, sizeof(book->stats));
	book->n = srcbook->n;
	book->n_nodes = srcbook->n_nodes;

	book->array = NULL;
	book->stack = NULL;
	book->need_saving = false;
	book->random->x = 0;
	book->search = NULL;

	book_cmd_post_process(g_ui);
}

//TODO:
//				// show book general information
//				} else if (strcmp(book_cmd, "stats") == 0) {
//					book_stats(book);

/**
 * @brief book verbose command.
 * @param book_verbosity book verbosity.
 */
DLL_API void edax_book_verbose(const int book_verbosity) {
	if (g_ui == NULL) return;
	Book *book = g_ui->play->book;
	book_cmd_pre_process(g_ui);

	// set book verbosity
	book->options.verbosity = book_verbosity;
	book->search->options.verbosity = book->options.verbosity;

	book_cmd_post_process(g_ui);
}

//TODO:
//				// analyze a game from the opening book point of view
//				} else if (strcmp(book_cmd, "a") == 0 || strcmp(book_cmd, "analyze") == 0 || strcmp(book_cmd, "analyse") == 0) {
//					val_1 = string_to_int(book_param, play->n_game); BOUND(val_1, 1, play->n_game, "depth");
//					play_book_analyze(play, val_1);

/**
 * @brief book add command.
 * @param base_file base file to add.
 */
DLL_API void edax_book_add(const char* base_file) {
	if (g_ui == NULL) return;
	Book *book = g_ui->play->book;
	book_cmd_pre_process(g_ui);

	// add positions from a game database
	Base base[1];
	base_init(base);
	base_load(base, base_file);
	book_add_base(book, base);
	base_free(base);

	book_cmd_post_process(g_ui);
}

/**
 * @brief book check command.
 * @param base_file base file to check.
 */
DLL_API void edax_book_check(const char* base_file) {
	if (g_ui == NULL) return;
	Book *book = g_ui->play->book;
	book_cmd_pre_process(g_ui);

	// check positions from a game database
	Base base[1];
	base_init(base);
	base_load(base, base_file);
	book_check_base(book, base);
	base_free(base);


	book_cmd_post_process(g_ui);
}

//TODO:
//				// extract positions
//				} else if (strcmp(book_cmd, "problem") == 0) {
//					val_1 = 24; book_param = parse_int(book_param, &val_1); BOUND(val_1, 0, 60, "number of empties");
//					val_2 = 10; book_param = parse_int(book_param, &val_2); BOUND(val_2, 1, 1000000, "number of positions");
//					book_extract_positions(book, val_1, val_2);


/**
 * @brief book extract command.
 * @param base_file base file to extract.
 */
DLL_API void edax_book_extract(const char* base_file) {
	if (g_ui == NULL) return;
	Book *book = g_ui->play->book;
	book_cmd_pre_process(g_ui);

	// extract pv to a game database
	Base base[1];
	base_init(base);
	book_extract_skeleton(book, base);
	base_save(base, base_file);
	base_free(base);

	book_cmd_post_process(g_ui);
}

/**
 * @brief book deviate command.
 * @param relative_error relative error.
 * @param absolute_error absolute error.
 */
DLL_API void edax_book_deviate(int relative_error, int absolute_error) {
	if (g_ui == NULL) return;
	Play *play = g_ui->play;
	Book *book = play->book;
	book_cmd_pre_process(g_ui);

	// add position using the "deviate algorithm"
	BOUND(relative_error, -129, 129, "relative error");
	BOUND(absolute_error, 0, 65, "absolute error");
	book_deviate(book, play->board, relative_error, absolute_error);

	book_cmd_post_process(g_ui);
}

/**
 * @brief book enhance command.
 * @param midgame_error midgame error.
 * @param endcut_error endcut error.
 */
DLL_API void edax_book_enhance(int midgame_error, int endcut_error) {
	if (g_ui == NULL) return;
	Play *play = g_ui->play;
	Book *book = play->book;
	book_cmd_pre_process(g_ui);

	// add position using the "enhance algorithm"
	BOUND(midgame_error, 0, 129, "midgame error");
	BOUND(endcut_error, 0, 129, "endcut error");
	book_enhance(book, play->board, midgame_error, endcut_error);

	book_cmd_post_process(g_ui);
}

/**
 * @brief book fill command.
 * @param fill_depth fill depth.
 */
DLL_API void edax_book_fill(int fill_depth) {
	if (g_ui == NULL) return;
	Book *book = g_ui->play->book;
	book_cmd_pre_process(g_ui);

	// add position by filling hole in the book
	BOUND(fill_depth, 1, 61, "fill depth");
	book_fill(book, fill_depth);

	book_cmd_post_process(g_ui);
}

/**
 * @brief book play command.
 */
DLL_API void edax_book_play() {
	if (g_ui == NULL) return;
	Book *book = g_ui->play->book;
	book_cmd_pre_process(g_ui);

	// add positions by expanding positions with no-link
	book_play(book);

	book_cmd_post_process(g_ui);
}

/**
 * @brief book deepen command.
 * caution: Currently, this function does not work correctly.
 */
DLL_API void edax_book_deepen() {
	if (g_ui == NULL) return;
	Book *book = g_ui->play->book;
	book_cmd_pre_process(g_ui);

	// add positions by expanding positions with no-link
	book_deepen(book);

	book_cmd_post_process(g_ui);
}

/**
 * @brief book feed-hash command.
 */
DLL_API void edax_book_feed_hash() {
	if (g_ui == NULL) return;
	Play *play = g_ui->play;
	Book *book = play->book;
	book_cmd_pre_process(g_ui);

	// add book positions to the hash table
	book_feed_hash(book, play->board, play->search);

	book_cmd_post_process(g_ui);
}

/**
 * @brief book add board preprocess.
 */
DLL_API void edax_book_add_board_pre_process() {
    if (g_ui == NULL) return;
    book_cmd_pre_process(g_ui);
    Book *book = g_ui->play->book;
    book_preprocess(book);
}

/**
 * @brief book add board postprocess.
 */
DLL_API void edax_book_add_board_post_process() {
    if (g_ui == NULL) return;
    book_cmd_post_process(g_ui);
}

/**
 * @brief book add board.
 * @param board board to add.
 */
DLL_API void edax_book_add_board(const Board* board) {
    if (g_ui == NULL) return;
    Book *book = g_ui->play->book;
    book_add_board(book, board);
}

/**
 * @brief base problem command.
 * @param base_file game database file.
 * @param n_empties number of empties.
 * @param problem_file problem_file to save.
 */
DLL_API void edax_base_problem(const char* base_file, const int n_empties, const char* problem_file) {
	Base base[1];
	base_init(base);

	// extract problem from a game base
	base_load(base, base_file);
	base_to_problem(base, n_empties, problem_file);

	base_free(base);
}

/**
 * @brief base tofen command.
 * @param base_file game database file.
 * @param n_empties number of empties.
 * @param problem_file problem_file to save.
 */
DLL_API void edax_base_tofen(const char* base_file, const int n_empties, const char* problem_file) {
	Base base[1];
	base_init(base);

	// extract FEN
	base_load(base, base_file);
	base_to_FEN(base, n_empties, problem_file);

	base_free(base);
}

/**
 * @brief base correct command.
 * @param base_file game database file.
 * @param n_empties number of empties.
 */
DLL_API void edax_base_correct(const char* base_file, const int n_empties) {
	if (g_ui == NULL) return;
	Base base[1];
	base_init(base);
	Play *play = g_ui->play;

	// correct erroneous games
	base_load(base, base_file);
	base_analyze(base, play->search, n_empties, true);
	remove(base_file);
	base_save(base, base_file);

	base_free(base);
}


//TODO:
//				// check erroneous games
//				} else if (strcmp(base_cmd, "check") == 0) {
//					int n_empties = 24;
//					base_param = parse_int(base_param, &n_empties);
//
//					base_load(base, base_file);
//					base_analyze(base, play->search, n_empties, false);


/**
 * @brief base complete command.
 * @param base_file game database file.
 */
DLL_API void edax_base_complete(const char* base_file) {
	if (g_ui == NULL) return;
	Base base[1];
	base_init(base);
	Play *play = g_ui->play;

	// terminate unfinished base
	base_load(base, base_file);
	base_complete(base, play->search);
	remove(base_file);
	base_save(base, base_file);

	base_free(base);
}

/**
 * @brief base convert command.
 * @param base_file_from input game database file.
 * @param base_file_to output game database file.
 */
DLL_API void edax_base_convert(const char* base_file_from, const char* base_file_to) {
	Base base[1];
	base_init(base);

	// convert a base to another format
	base_load(base, base_file_from);
	base_save(base, base_file_to);

	base_free(base);
}

/**
 * @brief base unique command.
 * @param base_file_from input game database file.
 * @param base_file_to output game database file.
 */
DLL_API void edax_base_unique(const char* base_file_from, const char* base_file_to) {
	Base base[1];
	base_init(base);

	// make a base unique by removing identical games
	base_load(base, base_file_from);
	base_unique(base);
	base_save(base, base_file_to);

	base_free(base);
}

//TODO:
//				// compare two game bases
//				} else if (strcmp(base_cmd, "compare") == 0) {
//					char base_file_2[FILENAME_MAX + 1];
//					base_param = parse_word(base_param, base_file_2, FILENAME_MAX);
//					base_compare(base_file, base_file_2);


/**
 * @brief set (option) command.
 * @param option_name name of option.
 * @param val value to set.
 */
DLL_API void edax_set_option(const char* option_name, const char* val) {
	if (g_ui == NULL) return;
	Play *play = g_ui->play;

	/* edax options */
	if (options_read(option_name, val)) {;
		options_bound();
		// parallel search changes:
		if (search_count_tasks(play->search) != options.n_task) {
			play_stop_pondering(play);
			search_set_task_number(play->search, options.n_task);
		}
		auto_go();
	}
}

/**
 * @brief get moves of the current game.
 * @param str buffer of length 160 + 1 or more (out parameter).
 * @return moves(equals to str. This is for Java, as Java String is immutable).
 */
DLL_API char* edax_get_moves(char* str) {
	if (g_ui == NULL) return NULL;
	int i;
	int player = BLACK;
	int w = 2;

	for (i = 0; i < g_ui->play->i_game && i < 80; ++i) {
		move_to_string(g_ui->play->game[i].x, player, str + w * i);
		player = !player;
	}
	str[w * i] = '\0';
	return str;
}

/**
 * @brief check if the current game is over.
 * @return 1 if game is over, otherwise 0.
 */
DLL_API int edax_is_game_over() {
	if (g_ui == NULL) return 0;
	return play_is_game_over(g_ui->play) ? 1 : 0;
}

/**
 * @brief check if the current player can move.
 * @return 1 if the current player can move, otherwise 0.
 */
DLL_API int edax_can_move() {
	if (g_ui == NULL) return 0;
	Play* play = g_ui->play;
	return can_move(play->board->player, play->board->opponent) ? 1 : 0;
}

/**
 * @brief get last move.
 * @param move last move(out parameter).
 */
DLL_API void edax_get_last_move(Move* move) {
	if (g_ui == NULL) return;
	Play* play = g_ui->play;

	Move* org = play_get_last_move(play);
	move->flipped = org->flipped;
	move->x = org->x;
	move->score = org->score;
	move->cost = org->cost;
	move->next = NULL;
}

/**
 * @brief get current board.
 * @param board current board(out parameter).
 */
DLL_API void edax_get_board(Board* board) {
	if (g_ui == NULL) return;
	Play* play = g_ui->play;

	board->player = play->board->player;
	board->opponent = play->board->opponent;
}

/**
 * @brief get current player.
 * @return current player(0:BLACK, 1:WHITE).
 */
DLL_API int edax_get_current_player() {
	if (g_ui == NULL) return -1;
	return g_ui->play->player;
}

/**
 * @brief get current number of discs.
 * @param color player's color(0:BLACK, 1:WHITE).
 * @return number of discs.
 */
DLL_API int edax_get_disc(const int color) {
	if (g_ui == NULL) return -1;
	Play* play = g_ui->play;
	Board* board = play->board;
	return color == play->player ? bit_count(board->player) : bit_count(board->opponent);
}

/**
 * @brief get current number of legal moves.
 * @param color player's color(0:BLACK, 1:WHITE).
 * @return number of legal moves.
 */
DLL_API int edax_get_mobility_count(const int color) {
	if (g_ui == NULL) return -1;
	Play* play = g_ui->play;
	Board* board = play->board;
	return color == play->player ?
			get_mobility(board->player, board->opponent) :
			get_mobility(board->opponent, board->player);
}
