/**
 * @file main.c
 *
 * @brief Main file.
 *
  * @date 1998 - 2026
 * @author Richard Delorme
 * @author Toshihiko Okuhara
 * @version 4.7
 */

#include "bit.h"
#include "board.h"
#include "cassio.h"
#include "obftest.h"
#include "options.h"
#include "perft.h"
#include "search.h"
#include "stats.h"
#include "ui.h"
#include "util.h"

#include <locale.h>

/**
 * @brief Print version & copyright.
 */
void version(void)
{
	printf("Edax version " EDAX_VERSION " " __DATE__ " " __TIME__
#if defined(__linux__)
		" for Linux"
#elif defined(_WIN32)
		" for Windows"
#elif defined(__APPLE__)
		" for Apple"
#endif
		"\ncopyright 1998 - 2026 Richard Delorme, Toshihiko Okuhara\n\n");
}

/**
 * @brief Programme usage.
 */
void usage(void)
{
	fprintf(stderr, "Usage: edax <protocol> <options>\n"
		"User Interface Protocols:\n"
		" -edax     Edax's user interface (default)\n"
		" -ggs      Generic Game Server interface (play through internet)\n"
		" -gtp      Go Text Protocol.\n"
		" -xboard xboard/winboard protocol.\n"
		" -nboard NBoard protocol.\n"
		" -cassio Cassio protocol.\n"
		" -solve <problem_file>    Automatic problem solver/checker.\n"
		" -wtest <wthor_file>      Test edax using WThor's theoric score.\n"
		" -count <level>           Count positions up to <level>.\n"
		" -complete <in> <out>     Complete a database.\n"
		" -test                    Run some basic unit test.\n");
	options_usage();
}

/**
 * @brief edax main function.
 *
 * Do a global initialization and choose a User Interface protocol.
 *
 * @param argc Number of arguments.
 * @param argv Command line arguments.
 */
int main(int argc, char **argv)
{
	UI *ui;
	alignas(64) Search search;
	Base base;
	int i, r, level = 0, size = 8;
	char *problem_file = NULL;
	char *wthor_file = NULL;
	char *count_type = NULL;
	char *base_in = NULL;
	char *base_out = NULL;
	bool count_pass = true;
	int n_bench = 0;
	bool test = false;

	// options.n_task default to system cpu number
	options.n_task = get_cpu_number();

	// options from edax.ini
	options_parse("edax.ini");

	// allocate ui
	ui = (UI*) malloc(sizeof *ui);
	if (ui == NULL) fatal_error("Cannot allocate a user interface.\n");
	ui->type = UI_EDAX;
	ui->init = ui_init_edax;
	ui->free = ui_free_edax;
	ui->loop = ui_loop_edax;

	// parse arguments
	for (i = 1; i < argc; i++) {
		char *arg = argv[i];
		while (*arg == '-') ++arg;
		if (strcmp(arg, "v") == 0 || strcmp(arg, "version") == 0) version();
		else if (ui_switch(ui, arg)) ;
		else if ((r = (options_read(arg, argv[i + 1]))) > 0) i += r - 1;
		else if (strcmp(arg, "solve") == 0 && argv[i + 1]) problem_file = argv[++i];
		else if (strcmp(arg, "complete") == 0 && argv[i + 1] && argv[i + 2]) { base_in = argv[++i]; base_out = argv[++i]; }
		else if (strcmp(arg, "wtest") == 0 && argv[i + 1]) wthor_file = argv[++i];
		else if (strcmp(arg, "bench") == 0 && argv[i + 1]) n_bench = atoi(argv[++i]);
		else if (strcmp(arg, "test") == 0) test = true;
		else if (strcmp(arg, "count") == 0 && argv[i + 1]) {
			count_type = argv[++i];
			if (argv[i + 1]) level = string_to_int(argv[++i], 0);
			if (argv[i + 1] && strcmp(argv[i + 1], "6x6") == 0) {
				size = 6;
				++i;
			}
			if (argv[i + 1] && strcmp(argv[i + 1], "skip-pass") == 0) {
				count_pass = false;
				++i;
			}
		}
		else usage();
	}
	options_bound();

	// initialize
	edge_stability_init();
	statistics_init();
	eval_open(options.eval_file);

	// solver & tester
	if (problem_file || wthor_file || n_bench) {
		search_init(&search);
		search.options.header = " depth|score|       time   |  nodes (N)  |   N/s    | principal variation";
		search.options.separator = "------+-----+--------------+-------------+----------+---------------------";
		if (options.verbosity) version();
		if (problem_file) obf_test(&search, problem_file, NULL);
		if (wthor_file) wthor_test(wthor_file, &search);
		if (n_bench) obf_speed(&search, n_bench);
		HASH_STATS(printf("pv_table     : %12" PRIu64 " stores %12" PRIu64 " probes %12" PRIu64 " found probes\n", search.pv_table.n_store, search.pv_table.n_try, search.pv_table.n_found);)
		HASH_STATS(printf("hash_table   : %12" PRIu64 " stores %12" PRIu64 " probes %12" PRIu64 " found probes\n", search.hash_table.n_store, search.hash_table.n_try, search.hash_table.n_found);)
		HASH_STATS(printf("shallow_table: %12" PRIu64 " stores %12" PRIu64 " probes %12" PRIu64 " found probes\n", search.shallow_table.n_store, search.shallow_table.n_try, search.shallow_table.n_found);)
		search_free(&search);
	} else if (base_in && base_out){
		search_init(&search);
		base_init(&base);
		base_load(&base, base_in);
		base_complete(&base, &search, options.level);
		base_save(&base, base_out, false);
		base_free(&base);
		search_free(&search);
	} else if (count_type){
		alignas(16) Board board;
		board_init(&board);
		if (strcmp(count_type, "games") == 0) quick_count_games(&board, level, count_pass, size);
		else if (strcmp(count_type, "positions") == 0) count_positions(&board, level, count_pass, size);
		else if (strcmp(count_type, "shapes") == 0) count_shapes(&board, level, count_pass, size);

	} else if (test) {
		// TODO: add more complete unit test
		bit_test();	} else if (ui->type == UI_CASSIO) {
			engine_loop();

		board_test();

	// other protocols
	} else {
		ui_event_init(ui);
		ui->init(ui);
		ui->loop(ui);
		if (ui->free) ui->free(ui);
		ui_event_free(ui);
	}

	// display statistics
	statistics_print(stdout);


	// free;
	eval_close();
	options_free();
	free(ui);

	return 0;
}
