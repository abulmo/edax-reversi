/**
 * @file options.h
 *
 * Options reader.
 *
 * @date 1998 - 2017
 * @author Richard Delorme
 * @version 4.4
 */

#include "options.h"
#include "stats.h"
#include "util.h"
#include "search.h"

#include <string.h>
#include <ctype.h>
#include <stdlib.h>
#include <limits.h>

/** global options with default value */
Options options = {
	21, // hash table size

	{0,-2,-3}, // inc_sort_depth

	1, // n_task (will be set to system available cpus at run-time)
	false, // cpu_affinity

	1, // verbosity
	0, // noise
	80, // width
	false, // echo
	false, // info
	false, // debug cassio
	true, // transgress cassio

	21, // max level
	TIME_MAX, // infinite time
	EDAX_FIXED_LEVEL, // play-type
	true, // can ponder
	-1, // depth
	-1, // selectivity

	3, // mode

	10000000,  // speed (default = 10e6)
	0,         // nps (default = 0)

	SCORE_MIN, // alpha
	SCORE_MAX, // beta

	false, // all_best

	NULL, // evaluation function's weights file.

	NULL, // book file
	true,            // book usage allowed
	0,               // book randomness

	NULL, // ggs host name
	NULL, // ggs login name
	NULL, // ggs password
	NULL, // port
	1,    // open

	0.25, // probcut depth reduction (/2)
	false, // pv debug
	false, // pv check
	false, // pv guess

	NULL, // game file.

	NULL, // search log file.
	NULL, // ui log file.
	NULL, // ggs log file.

	NULL, // edax name

	false, //auto start
	false, //auto store
	false, //auto learn
	false, //auto quit
	0, //repeat
};

/**
 * @brief Print options usage.
 */
void options_usage(void)
{
	fprintf(stderr, "\nCommon options:\n");
	fprintf(stderr, "  -?|help                       show this message.\n");
	fprintf(stderr, "  -o|option-file                read options from this file.\n");
	fprintf(stderr, "  -v|version                    display the version number.\n");
	fprintf(stderr, "  -name <string>                set Edax name to <string>.\n");
	fprintf(stderr, "  -verbose <n>                  verbosity level.\n");
	fprintf(stderr, "  -q                            silent mode (eq. -verbose 0).\n");
	fprintf(stderr, "  -vv                           very verbose mode (eq. -verbose 2).\n");
	fprintf(stderr, "  -noise <n>                    noise level (print search output from ply <n>).\n");
	fprintf(stderr, "  -width <n>                    line width.\n");
	fprintf(stderr, "  -h|hash-table-size <nbits>    hash table size.\n");
	fprintf(stderr, "  -n|n-tasks <n>                search in parallel using n tasks.\n");
	fprintf(stderr, "  -cpu                          search using 1 cpu/thread.\n");
#ifdef __APPLE__
	fprintf(stderr, "\nCassio protocol options:\n");
	fprintf(stderr, "  -debug-cassio                 print extra-information in cassio.\n");
	fprintf(stderr, "  -follow-cassio                follow more closely cassio requests.\n");
	fprintf(stderr, "\nOptions unavailable to Cassio protocol\n:");
#endif
	fprintf(stderr, "  -l|level <n>                  search using limited depth.\n");
	fprintf(stderr, "  -t|game-time <n>              search using limited time per game.\n");
	fprintf(stderr, "  -move-time <n>                search using limited time per move.\n");
	fprintf(stderr, "  -ponder <on/off>              search during opponent time.\n");
	fprintf(stderr, "  -eval-file                    read eval weight from this file.\n");
	fprintf(stderr, "  -book-file                    load opening book from this file.\n");
	fprintf(stderr, "  -book-usage <on/off>          play from the opening book.\n");
	fprintf(stderr, "  -book-randomness <n>          play various but worse moves from the opening book.\n");
	fprintf(stderr, "  -auto-start <on/off>          automatically restart a new game.\n");
	fprintf(stderr, "  -auto-swap <on/off>           automatically Edax's color between games\n");
	fprintf(stderr, "  -auto-store <on/off>          automatically save played games\n");
	fprintf(stderr, "  -game-file <file>             file to store all played game/s.\n");
	fprintf(stderr, "  -search-log-file <file>       file to store search detailed output/s.\n");
	fprintf(stderr, "  -ui-log-file <file>           file to store input/output to the (U)ser (I)nterface.\n");

	exit(EXIT_SUCCESS);
}

/**
 * @brief Read an option.
 *
 * @param option Option name.
 * @param value Option value.
 * @return The number of arguments read (0, 1 or 2).
 */
int options_read(const char *option, const char *value)
{
	int read = 0;

	if (option == NULL) return read;
	while (*option == '-') ++option;
	if (*option == '\0' || *option == '%' || *option == '#') return read;

	read = 1;
	if (strcmp(option, "vv") == 0) options.verbosity = 2;
	else if (strcmp(option, "q") == 0) options.verbosity = 0;
	else if (strcmp(option, "info") == 0) options.info = true;
	else if (strcmp(option, "debug-cassio") == 0) options.debug_cassio = true;
	else if (strcmp(option, "follow-cassio") == 0) options.transgress_cassio = false;
	else if (strcmp(option, "?") == 0 || strcmp(option, "help") == 0) usage();
	else if (strcmp(option, "cpu") == 0) options.cpu_affinity = true;
	else {
		read = 0;
		if (value == NULL || *value == '\0') return read;
		read = 2;
		if (strcmp(option, "verbose") == 0) options.verbosity = string_to_int(value, options.verbosity);
		else if (strcmp(option, "noise") == 0) options.noise = string_to_int(value, options.noise);
		else if (strcmp(option, "width") == 0) options.width = string_to_int(value, options.width);

		else if (strcmp(option, "h") == 0  || strcmp(option, "hash-table-size") == 0) options.hash_table_size = string_to_int(value, options.hash_table_size);
		else if (strcmp(option, "n") == 0 || strcmp(option, "n-tasks") == 0) options.n_task = string_to_int(value, options.n_task);
		else if (strcmp(option, "l") == 0 || strcmp(option, "level") == 0) {
			options.level = string_to_int(value, options.level);
			options.play_type = EDAX_FIXED_LEVEL;
		} else if (strcmp(option, "d") == 0 || strcmp(option, "depth") == 0) {
			options.depth = string_to_int(value, options.depth);
			options.play_type = EDAX_FIXED_LEVEL;
		} else if (strcmp(option, "selectivity") == 0) {
			options.selectivity = string_to_int(value, options.selectivity);
			options.play_type = EDAX_FIXED_LEVEL;
		} else if (strcmp(option, "t") == 0 || strcmp(option, "game-time") == 0) {
			options.time = string_to_time(value);
			options.play_type = EDAX_TIME_PER_GAME;
		} else if (strcmp(option, "move-time") == 0) {
			options.time = string_to_time(value);
			options.play_type = EDAX_TIME_PER_MOVE;
		} else if (strcmp(option, "alpha") == 0) options.alpha = string_to_int(value, options.alpha);
		else if (strcmp(option, "beta") == 0) options.beta = string_to_int(value, options.beta);
		else if (strcmp(option, "all-best") == 0) parse_boolean(value, &options.all_best);

		else if (strcmp(option, "o") == 0 || strcmp(option, "option-file") == 0) options_parse(value);
		else if (strcmp(option, "speed") == 0) options.speed = string_to_real(value, options.speed);
		else if (strcmp(option, "nps") == 0) options.nps = 0.001 * string_to_real(value, options.nps);
		else if (strcmp(option, "ponder") == 0) parse_boolean(value, &options.can_ponder);
		else if (strcmp(option, "mode") == 0) parse_int(value, &options.mode);

		else if (strcmp(option, "inc-pvnode-sort-depth") == 0) options.inc_sort_depth[PV_NODE] = string_to_int(value, options.inc_sort_depth[PV_NODE]);
		else if (strcmp(option, "inc-cutnode-sort-depth") == 0) options.inc_sort_depth[CUT_NODE] = string_to_int(value, options.inc_sort_depth[CUT_NODE]);
		else if (strcmp(option, "inc-allnode-sort-depth") == 0) options.inc_sort_depth[ALL_NODE] = string_to_int(value, options.inc_sort_depth[ALL_NODE]);

		else if (strcmp(option, "ggs-host") == 0) options.ggs_host = string_duplicate(value);
		else if (strcmp(option, "ggs-login") == 0) options.ggs_login = string_duplicate(value);
		else if (strcmp(option, "ggs-password") == 0) options.ggs_password = string_duplicate(value);
		else if (strcmp(option, "ggs-port") == 0) options.ggs_port = string_duplicate(value);
		else if (strcmp(option, "ggs-open") == 0) parse_boolean(value, &options.ggs_open);

		else if (strcmp(option, "probcut-d") == 0) parse_real(value, &options.probcut_d);

		else if (strcmp(option, "pv-debug") == 0) parse_boolean(value, &options.pv_debug);
		else if (strcmp(option, "pv-check") == 0) parse_boolean(value, &options.pv_check);
		else if (strcmp(option, "pv-guess") == 0) parse_boolean(value, &options.pv_guess);

		else if (strcmp(option, "game-file") == 0) options.game_file = string_duplicate(value);

		else if (strcmp(option, "eval-file") == 0) options.eval_file = string_duplicate(value);
		
		else if (strcmp(option, "book-file") == 0) options.book_file = string_duplicate(value);
		else if (strcmp(option, "book-usage") == 0) parse_boolean(value, &options.book_allowed);
		else if (strcmp(option, "book-randomness") == 0) parse_int(value, &options.book_randomness);

		else if (strcmp(option, "search-log-file") == 0) options.search_log_file = string_duplicate(value);
		else if (strcmp(option, "ui-log-file") == 0) options.ui_log_file = string_duplicate(value);
		else if (strcmp(option, "ggs-log-file") == 0) options.ggs_log_file = string_duplicate(value);

		else if (strcmp(option, "name") == 0) options.name = string_duplicate(value);
		else if (strcmp(option, "echo") == 0)parse_boolean(value, &options.echo);

		else if (strcmp(option, "auto-start") == 0) parse_boolean(value, &options.auto_start);
		else if (strcmp(option, "auto-store") == 0) parse_boolean(value, &options.auto_store);
		else if (strcmp(option, "auto-swap") == 0) parse_boolean(value, &options.auto_swap);
		else if (strcmp(option, "auto-quit") == 0) parse_boolean(value, &options.auto_quit);
		else if (strcmp(option, "repeat") == 0) parse_int(value, &options.repeat);

		else read = 0;
	}

	if (read) {
		info("<set option %s %s>\n", option, value);
	}

	return read;
}


/**
 * @brief parse an option from a string
 *
 * A line of the form:  <code>"[set] option [=] value"</code>
 * is parse into two strings: <code>option</code> and <code>value</code>.
 * The "set" and "=" are optionnal.
 * This function assume all string sizes are sufficient.
 *
 * @param line The string to parse.
 * @param option A string to be filled with the option name.
 * @param value A string to be filled with the option value.
 * @param size option & value string capacity.
 * @return remaining of the string.
 */
static const char* option_parse(const char *line, char *option, char *value, int size)
{
	line = parse_word(line, option, size);
	if (strcmp(option, "set") == 0) line = parse_word(line, option, size);
	line = parse_word(line, value, size);
	if (strcmp(value, "=") == 0) line = parse_word(line, value, size);

	options_read(option, value);

	return line;
}

/**
 * @brief parse options from a file
 *
 * @param file Option file name.
 */
void options_parse(const char *file)
{
	char *line, *option, *value;
	FILE *f = fopen(file, "r");

	if (f != NULL) {

		while ((line = string_read_line(f)) != NULL) {
			int n = strlen(line);
			option = (char*) malloc(n + 1);
			value = (char*) malloc(n + 1);
			option_parse(line, option, value, n);
			free(line); free(option); free(value);
		}

		fclose(f);
	}
}

/**
 * @brief Keep options between realistic values.
 */
void options_bound(void) 
{
	int tmp;
	int max_threads;

	if (sizeof (void*) == 4) {
		BOUND(options.hash_table_size, 10, 25, "hash-table-size");
	} else {
		BOUND(options.hash_table_size, 10, 30, "hash-table-size");
	}

	max_threads = MIN(get_cpu_number(), MAX_THREADS);
	BOUND(options.n_task, 1, max_threads, "n-tasks");

	BOUND(options.verbosity, 0, 4, "verbosity");
	BOUND(options.noise, 0, 60, "noise");
	BOUND(options.width, 3, 250, "width");
	BOUND(options.level, 0, 60, "level");
	BOUND(options.time, 1000, TIME_MAX, "time");

	BOUND(options.alpha, SCORE_MIN, SCORE_MAX, "alpha");
	BOUND(options.beta, SCORE_MIN, SCORE_MAX, "beta");

	BOUND(options.speed, 1e5, 1e12, "speed");

	if (options.alpha > options.beta) {
		fprintf(stderr, "WARNING: alphabeta [%d, %d] will be inverted.\n", options.alpha, options.beta);
		tmp = options.alpha;
		options.alpha = options.beta;
		options.beta = tmp;
	}

	if (options.name == NULL) options.name = string_duplicate(EDAX_NAME);
	if (options.game_file == NULL) options.game_file = string_duplicate("data/game.ggf");
	if (options.eval_file == NULL) options.eval_file = string_duplicate("data/eval.dat");
	if (options.book_file == NULL) options.book_file = string_duplicate("data/book.dat");
}

/**
 * @brief Print all global options.
 * @param f output stream.
 */
void options_dump(FILE *f) 
{
	const char *(play_type[3]) = {"fixed depth", "fixed time per game", "fixed time per move"};
	const char *(boolean_string[2]) = {"false", "true"};
	const char *(mode[4]) = {"human/edax", "edax/human", "edax/edax", "human/human"};	

	fprintf(f, "search display options\n");
	fprintf(f, "\tverbosity: %d\n", options.verbosity);
	fprintf(f, "\tminimal depth (noise): %d\n", options.noise);
	fprintf(f, "\tline width: %d\n", options.width);
	fprintf(f, "\tuser input echo: %s\n", boolean_string[options.echo]);
	fprintf(f, "\t<detailed info>: %s\n\n", boolean_string[options.info]);
	fprintf(f, "Cassio options\n");
	fprintf(f, "\tdisplay debug info in Cassio's 'fenetre de rapport': %s\n", boolean_string[options.debug_cassio]);
	fprintf(f, "\tadapt Cassio requests to search & solve faster: %s\n\n", boolean_string[options.transgress_cassio]);

	fprintf(f, "\tsearch options\n");
	fprintf(f, "\tsize (in number of bits) of the hash table: %d\n", options.hash_table_size);
	fprintf(f, "\tsorting depth increment: pv = %d, all = %d, cut = %d\n",  options.inc_sort_depth[0], options.inc_sort_depth[1], options.inc_sort_depth[2]);
	fprintf(f, "\ttask number for parallel search: %d\n", options.n_task);
	fprintf(f, "\tsearch level: %d\n", options.level);
	fprintf(f, "\tsearch alloted time:"); time_print(options.time, false, stdout); fprintf(f, "\n");
	fprintf(f, "\tsearch with: %s\n", play_type[options.play_type]);
	fprintf(f, "\tsearch pondering: %s\n", boolean_string[options.can_ponder]);
	fprintf(f, "\tsearch depth: %d\n", options.depth);
	fprintf(f, "\tsearch selectivity: %d\n", options.selectivity);
	fprintf(f, "\tsearch speed %.0f N/s\n", options.speed);
	fprintf(f, "\tsearch nps %.0f N/s\n", options.nps);
	fprintf(f, "\tsearch alpha: %d\n", options.alpha);
	fprintf(f, "\tsearch beta: %d\n", options.beta);
	fprintf(f, "\tsearch all best moves: %s\n", boolean_string[options.all_best]);
	fprintf(f, "\teval file: %s\n", options.eval_file);
	fprintf(f, "\tbook file: %s\n", options.book_file);
	fprintf(f, "\tbook allowed: %s\n", boolean_string[options.book_allowed]);
	fprintf(f, "\tbook randomness: %d\n\n", options.book_randomness);

	fprintf(f, "ggs options\n");
	fprintf(f, "\thost: %s\n", options.ggs_host ? options.ggs_host : "?");
	fprintf(f, "\tport: %s\n", options.ggs_port ? options.ggs_port : "?");
	fprintf(f, "\tlogin: %s\n", options.ggs_login ? options.ggs_login : "?");
	fprintf(f, "\tpassword: %s\n", options.ggs_password ? options.ggs_password : "?");
	fprintf(f, "\topen: %s\n\n", boolean_string[options.ggs_open]);

	fprintf(f, "PV options\n");
	fprintf(f, "\tdebug: %s\n", boolean_string[options.pv_debug]);
	fprintf(f, "\tcheck: %s\n", boolean_string[options.pv_check]);
	fprintf(f, "\tguess: %s\n\n", boolean_string[options.pv_guess]);

	fprintf(f, "game file: %s\n", options.game_file ? options.game_file : "?");

	fprintf(f, "log files\n");
	fprintf(f, "\tsearch: %s\n", options.search_log_file ? options.search_log_file : "?");
	fprintf(f, "\tui: %s\n", options.ui_log_file ? options.ui_log_file : "?");
	fprintf(f, "\tggs: %s\n", options.ggs_log_file ? options.ggs_log_file : "?");

	fprintf(f, "name: %s\n", options.name ? options.name : "?");

	fprintf(f, "Game play\n");
	fprintf(f, "\tmode: %s\n", mode[options.mode]);
	fprintf(f, "\tstart a new game after a game is over: %s\n", boolean_string[options.auto_start]);
	fprintf(f, "\tstore each played game in the opening book: %s\n", boolean_string[options.auto_start]);
	fprintf(f, "\tchange computer's side after each game: %s\n", boolean_string[options.auto_start]);
	fprintf(f, "\tquit when game is over: %s\n", boolean_string[options.auto_start]);
	fprintf(f, "\trepeat %d games (before exiting)\n\n\n", options.repeat);
}

/**
 * @brief free allocated resources.
 */
void options_free(void)
{
	if (options.ggs_host) free(options.ggs_host);
	if (options.ggs_login) free(options.ggs_login);
	if (options.ggs_password) free(options.ggs_password);
	if (options.ggs_port) free(options.ggs_port);

	if (options.game_file) free(options.game_file);
	if (options.ui_log_file) free(options.ui_log_file);
	if (options.search_log_file) free(options.search_log_file);
	if (options.ggs_log_file) free(options.ggs_log_file);
	if (options.name) free(options.name);
	if (options.book_file) free(options.book_file);
	if (options.eval_file) free(options.eval_file);

	options.ggs_host = NULL;
	options.ggs_login = NULL;
	options.ggs_password = NULL;
	options.ggs_port = NULL;

	options.game_file = NULL;
	options.ui_log_file = NULL;
	options.search_log_file = NULL;
	options.ggs_log_file = NULL;
	options.name = NULL;
	options.book_file = NULL;
	options.eval_file = NULL;

}

