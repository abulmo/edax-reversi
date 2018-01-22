/**
 * @file cassio.c
 *
 * Engine low level Protocol to communicate with Cassio by Stephane Nicolet.
 *
 * The main purpose of this protocol is to help Cassio in this research. Cassio still does a 
 * lot of works like time management, etc.
 *
 *  - Edax needs to be run from its own path to have access to its evaluation function data.
 *  - It is recommended to run Edax with the "-cassio" argument to use this protocol.
 *  - With "-debug-cassio" Edax displays what it is doing with more details.
 *  - With "-ui-log-file cassio.log" edax will saved communicated data in the "cassio.log" file.
 *  - With "-follow-cassio" Edax will follow more closely Cassio's search request. By default, it
 * searches with settings that make it better in tournament mode against Roxane, Cassio, etc.
 *
 * @date 1998 - 2017
 * @author Richard Delorme
 * @version 4.4
 */

#include "cassio.h"

#include "event.h"
#include "options.h"
#include "search.h"
#include "settings.h"
#include "ybwc.h"
#include "stats.h"
#include "util.h"
#include <assert.h>

#include <stdarg.h>
#include <math.h>

Log engine_log[1];

#define ENGINE_N_POSITION 1024

/** a global variable used to display the search result */
static char engine_result[1024];
/** a global string with the last result sent to avoid duplicate result lines */
static char last_line_sent[1024];

/** Engine management data */
typedef struct Engine {
	Event event[1];          /** Events */
	Search *search;          /** Search */
	struct {
		Board board[ENGINE_N_POSITION];    /** Last position */
		int n;               /** # of last position */
	} last_position;
	bool is_searching;
} Engine;


/**
 * @brief Send a message on stdout.
 * @param format Format string.
 * @param ... variable arguments.
 */
static void engine_send(const char *format, ...)
{
	va_list args;

	va_start(args, format);
	vprintf(format, args);
	va_end(args);
	putchar('\n'); fflush(stdout);

	if (log_is_open(engine_log)) {
		time_stamp(engine_log->f);
		fprintf(engine_log->f, "sent> \"");
		va_start(args, format);
		vfprintf(engine_log->f, format, args);
		va_end(args);
		fprintf(engine_log->f, "\"\n");
		fflush(engine_log->f);
	}
}

/**
 * @brief Read an input line.
 *
 * @param engine Engine.
 */
static void engine_get_input(Engine *engine)
{
	char protocol[32], cmd[32];
	Event *event = engine->event;
	char *buffer_with_garbage;
	char *buffer = NULL;

	buffer_with_garbage = string_read_line(stdin);
	if (log_is_open(engine_log)) {
		time_stamp(engine_log->f);
		fprintf(engine_log->f, "received< \"%s\"\n", buffer_with_garbage);
	}

	if (buffer_with_garbage == NULL) { // stdin closed or not working
		buffer_with_garbage = string_duplicate("ENGINE-PROTOCOL eof");
		event->loop = false;
	}

	buffer = parse_word(buffer_with_garbage, protocol, 32);

	if (strcmp(protocol, "ENGINE-PROTOCOL") == 0) {
		buffer = string_duplicate(buffer);
		parse_word(buffer, cmd, 32);
		string_to_lowercase(cmd);
		// stop or quit: Interrupt immediately the current search & remove unprocessed messages.
		if (strcmp(cmd, "stop") == 0) {
			event_clear_messages(event);
			if (engine->is_searching) {
				engine_stop(engine->search);
			} else {
				engine_send("ready."); 
			}
		} else if (strcmp(cmd, "get-search-infos") == 0) {
			if (engine->is_searching) {
				engine_send("node %lld, time %.3f", search_count_nodes(engine->search), 0.001 * search_time(engine->search));
			} else {
				engine_send("ready."); 
			}
		} else {
			if (strcmp(cmd, "quit") == 0) {
				engine_stop(engine->search);
				event_clear_messages(event);
				event->loop = false;
			}
			// add the message (including quit) to a queue for later processing.
			event_add_message(event, buffer);
			lock(event);
				condition_signal(event);
			unlock(event);

		}
	} else if (*protocol == '\0') {
		if (engine->is_searching) {
			engine_send("ok.");
		} else {
			engine_send("ready.");
		}
	} else {
		engine_send("ERROR: Unknown protocol \"%s\"", buffer_with_garbage);
	}

	free(buffer_with_garbage);
}

/**
 * @brief Engine wait input.
 *
 * @param engine Engine.
 * @param cmd Command.
 * @param param Command's parameters.
 */
static void engine_wait_input(Engine *engine, char **cmd, char **param)
{
	event_wait(engine->event, cmd, param);
}

/**
 * @brief Read event loop.
 *
 * @param v Engine.
 */
static void* engine_input_loop(void *v)
{
	Engine *engine = (Engine*) v;


	while (engine->event->loop && !feof(stdin) && !ferror(stdin)) {
		engine_get_input(engine);
	}

	engine->event->loop = false;
	cassio_debug("Quit input loop\n");

	return NULL;
}

/**
 * Check if a position has been already analyzed.
 * 
 */
static bool is_position_new(Engine *engine, Board *board)
{
	int i;
	
	for (i = 0; i < engine->last_position.n; ++i) {
		if (board_equal(board, engine->last_position.board + i)) return false;
	}
	
	if (i == ENGINE_N_POSITION) {
		cassio_debug("Position list: removing position %llx\n", board_get_hash_code(board));
	}
	while (--i > 0) {
		engine->last_position.board[i] = engine->last_position.board[i - 1];	
	}
	cassio_debug("Position list: adding position %llx\n", board_get_hash_code(board));
	engine->last_position.board[0] = *board;
	engine->last_position.n = MIN(ENGINE_N_POSITION, engine->last_position.n + 1);
	hash_clear(engine->search->hash_table);
	hash_clear(engine->search->pv_table);
	hash_clear(engine->search->shallow_table);
	
	return true;
}

/**
 * @brief Call back function use by search to print its results.
 * @param result Search results.
 */
static void engine_observer(Result *result)
{
	int n = 72;
	char color = (engine_result[64] == 'O' ? 'W' : 'B');
	
	move_to_string(result->move, color == 'W' ? WHITE : BLACK, engine_result + n); n += 2;
	n += sprintf(engine_result + n, ", depth %d, @%d%%, %c%+d.00 <= v <= %c%+d.00, ",
		 result->depth, selectivity_table[result->selectivity].percent,
		 color, result->bound[result->move].lower, color, result->bound[result->move].upper);
	line_to_string(result->pv, result->pv->n_moves, NULL, engine_result + n);
	n += 2 * result->pv->n_moves;
	n += sprintf(engine_result + n, ", node %llu, time %.3f", result->n_nodes, 0.001 * result->time);

	// avoid to send multiple times the same result.
	if (strncmp(engine_result + 72, last_line_sent, n - 75) != 0) {
		strcpy(last_line_sent, engine_result + 72);
		engine_send("%s", engine_result);
	}
}

/**
 * @brief Create engine search.
 * @return a pointer to a newly allocated search structure.
 */
static Search* engine_create_search(void)
{
	Search *search;
	
	search = (Search*) malloc(sizeof (Search));
	if (search == NULL) {
		engine_send("ERROR: Cannot allocate a new search engine.");
		engine_send("bye bye!");
		exit(EXIT_FAILURE);
	}
	
	search_init(search);
	search_set_observer(search, engine_observer);
	
	return search;
}

/**
 * @brief Open search engine.
 *
 * Set the board to analyze with the alpha-beta bounds and the depth-precision targets.
 *
 * @param search Search engine.
 * @param board Position to analyze.
 * @param player Player on turn.
 * @param alpha Alpha bound.
 * @param beta Beta bound.
 * @param depth Target depth to reach.
 * @param precision Target precision to reach.
 */
static int engine_open(Search *search, const Board *board, const int player, const int alpha, const int beta, const int depth, const int precision)
{
	int k;
	HashData hash_data[1];
	Move *move;
	int score = 0;
	
	search->time.spent = -time_clock();
	search->stop = RUNNING;

	// set alphabeta bounds
	options.alpha = alpha;
	BOUND(options.alpha, SCORE_MIN, SCORE_MAX, "alpha");

	options.beta = beta;
	BOUND(options.beta, options.alpha + 1, SCORE_MAX, "beta");

	// other initializations
	search->n_nodes = 0;
	search->child_nodes = 0;
	search_time_init(search);
	if (!search->options.keep_date) {
		hash_clear(search->hash_table);
		hash_clear(search->pv_table);
		hash_clear(search->shallow_table);
	}

	search->height = 0;
	search->node_type[search->height] = PV_NODE;
	search->result->score = 0;
	search->stability_bound.upper = SCORE_MAX - 2 * get_stability(board->opponent, board->player);
	search->stability_bound.lower = 2 * get_stability(board->player, board->opponent) - SCORE_MAX;

	// set the board
	if (player != search->player || !board_equal(search->board, board)) {
		search_set_board(search, board, player);

		if (hash_get(search->pv_table, board, board_get_hash_code(board), hash_data)) {
			if (hash_data->lower == -SCORE_INF && hash_data->upper < SCORE_INF) score = hash_data->upper;
			else if (hash_data->upper == +SCORE_INF && hash_data->lower > -SCORE_INF) score = hash_data->lower;
			else score = (hash_data->upper + hash_data->lower) / 2;
		}
		if (!movelist_is_empty(search->movelist)) {
			movelist_evaluate(search->movelist, search, hash_data, options.alpha, depth);
			movelist_sort(search->movelist);
		}
	}

	foreach_move(move, search->movelist) {
		search->result->bound[move->x].lower = SCORE_MIN;
		search->result->bound[move->x].upper = SCORE_MAX;
	}

	search->result->n_moves_left = search->result->n_moves = search->movelist->n_moves;
	search->result->book_move = false;

	// set level
	search->depth = depth;
	if (options.transgress_cassio && (search->n_empties & 1) != (depth & 1)) ++search->depth;
	if (options.transgress_cassio && search->depth > search->n_empties - 10) search->depth = search->n_empties;
	search->options.depth = search->depth;

	BOUND(search->depth, 0, search->n_empties, "depth");
	search->depth_pv_extension = get_pv_extension(search->depth, search->n_empties);

	if (options.transgress_cassio && depth < search->n_empties) k = 0;
	else if (precision <= 73) k = 0;
	else if (precision <= 87) k = 1;
	else if (precision <= 95) k = 2;
	else if (precision <= 98) k = 3;
	else if (precision <= 99) k = 4;
	else k = 5;
	search->options.selectivity = search->selectivity = k;

	*last_line_sent = '\0';
	board_to_string(board, player, engine_result);
	engine_result[64] = engine_result[65];
	strcpy(engine_result + 65, ", move ");

	return score;
}

/**
 * @brief Finalize search
 *
 * @param search Search engine.
 */
static void engine_close(Search *search)
{
	search->result->n_nodes = search_count_nodes(search);
	search->time.spent += time_clock();
	search->result->time = search->time.spent;

	statistics_sum_nodes(search);

	// in case nothing have been sent yet...
	if (*last_line_sent == '\0' && search->stop == RUNNING) engine_observer(search->result);

	search->stop = STOP_END;
}

/**
 * @brief Create a new Othello engine.
 *
 * Allocate engine resources & launch user-input event thread.
 *
 * @return A pointer to a newly allocated engine structure.
 */
void* engine_init(void)
{
	Engine *engine;
	
	log_open(engine_log, options.ui_log_file);

	engine = (Engine*) malloc(sizeof (Engine));
	if (engine == NULL) {
		engine_send("ERROR: Cannot allocate a new engine.");
		engine_send("bye bye!");
		exit(EXIT_FAILURE);
	}
	engine->is_searching = false;
	engine->search = engine_create_search();
	event_init(engine->event);
	
	thread_create2(&engine->event->thread, engine_input_loop, engine); // modified for iOS by lavox. 2018/1/16
	thread_detach(engine->event->thread);
	
	return engine;
}

/**
 * @brief free resources allocated
 * @param v Engine.
 */
void engine_free(void *v)
{
	Search *search = (Search*) v;
	if (search) {
		search_free(search);
		free(search);
	}
	log_close(engine_log);
}



void feed_all_hash_table(Search *search, Board *board, const int depth, const int selectivity, const int lower, const int upper, const int move)
{
	const unsigned long long hash_code = board_get_hash_code(board);

	hash_feed(search->hash_table, board, hash_code, depth, selectivity, lower, upper, move);
	hash_feed(search->pv_table, board, hash_code, depth, selectivity, lower, upper, move);	
}

/**
 * @brief feed hash table
 *
 * @param v Engine.
 * @param board Position to add.
 * @param upper Upper score bound.
 * @param lower Lower score bound.
 * @param depth depth.
 * @param precision Probcut percentage.
 * @param pv Main line to add to the hashtable.
 */
void engine_feed_hash(void *v, Board *board, int lower, int upper, const int depth, const int precision, Line *pv)
{
	Engine *engine = (Engine*) v;
	Search *search = engine->search;
	int i, selectivity, tmp;
	int current_depth;
	Move *move, *child_move;
	MoveList movelist[1], child_movelist[1];
	
	if (options.transgress_cassio && depth < board_count_empties(board)) selectivity = 0;
	else if (precision <= 73) selectivity = 0;
	else if (precision <= 87) selectivity = 1;
	else if (precision <= 95) selectivity = 2;
	else if (precision <= 98) selectivity = 3;
	else if (precision <= 99) selectivity = 4;
	else selectivity = 5;
	
	pv->move[pv->n_moves] = NOMOVE;

	for (i = 0; i <= pv->n_moves; ++i) {
		current_depth = depth - i;
		feed_all_hash_table(search, board, current_depth, selectivity, lower, upper, pv->move[i]);

		movelist_get_moves(movelist, board);
		movelist_sort_bestmove(movelist, pv->move[i]);

		foreach_move(move, movelist) {
			board_update(board, move);
				if (move->x == pv->move[i]) {
					feed_all_hash_table(search, board, current_depth - 1, selectivity, -upper, -lower, NOMOVE);
					if (lower > SCORE_MIN) {
						movelist_get_moves(child_movelist, board);
						foreach_move(child_move, child_movelist) {
							board_update(board, child_move);
								feed_all_hash_table(search, board, current_depth - 2, selectivity, lower, SCORE_MAX, NOMOVE);
							board_restore(board, child_move);
						}					
					}
				} else if (upper < SCORE_MAX) {
					feed_all_hash_table(search, board, current_depth - 1, selectivity, -upper, SCORE_MAX, NOMOVE);
				}
			board_restore(board, move);
		}

		move = movelist_first(movelist);
	
		if (move && move->x == pv->move[i]) {
			board_update(board, move);
			tmp = lower;
			lower = -upper;
			upper = -tmp;
		} else if (!move && pv->move[i] == PASS && board_is_pass(board)) {
			board_pass(board);
			tmp = lower;
			lower = -upper;
			upper = -tmp;
		} else {
			break;
		}
	}
}


/**
 * @brief Empty (ie completely clear) the engine hash table.
 *
 * @param v Engine.
 */
void engine_empty_hash(void *v)
{
	Engine *engine = (Engine*) v;
	
	if (engine && engine->search && engine->search->hash_table && engine->search->pv_table) {
		cassio_debug("clear the hash-table.\n");
		engine->last_position.n = 0;
		hash_cleanup(engine->search->hash_table);
		hash_cleanup(engine->search->pv_table);
		hash_cleanup(engine->search->shallow_table);
	}
}

/**
 * @brief Check if a search has already been done here.
 * @param engine Search engine.
 * @param old_score Previously found score.
 * @return true if a new search can be skipped, false otherwise.
 */
static bool skip_search(Engine *engine, int *old_score)
{
	Search *search = engine->search;
	Board *board = search->board;
	MoveList *movelist = search->movelist;
	HashData hash_data[1];
	Move *bestmove;
	int alpha = options.alpha;
	int beta = options.beta;
	Bound *bound;
	char s[4], b[80];
	const unsigned long long hash_code = board_get_hash_code(board);
	
	*old_score = 0;
	
	if (hash_get(search->pv_table, board, hash_code, hash_data)
	|| hash_get(search->hash_table, board, hash_code, hash_data)) {
		// compute bounds
		if (alpha < hash_data->lower) alpha = *old_score = hash_data->lower;
		if (beta > hash_data->upper) beta = *old_score = hash_data->upper;
		// skip search ?
		if (hash_data->depth >= search->depth && hash_data->selectivity >= search->selectivity && alpha >= beta) {
			if (hash_data->move[0] != NOMOVE) movelist_sort_bestmove(movelist, hash_data->move[0]);
			else if (hash_data->lower > SCORE_MIN) return false;
			bestmove = movelist_first(movelist);
			bestmove->score = *old_score;
			record_best_move(search, board, bestmove, options.alpha, options.beta, search->depth);
			bound =  search->result->bound + bestmove->x;

			if (bound->lower != bound->upper || is_pv_ok(search, bestmove->x, search->depth)) {
				cassio_debug("Edax skips the search. The position is already in the hash table: %s (%d, %d) ?\n", move_to_string(bestmove->x, search->player, s), hash_data->lower, hash_data->upper);
				engine_observer(search->result);
				return true;
			} else {
				cassio_debug("Edax does not skip the search : BAD PV!\n");
			}
		} else {
			if (hash_data->depth < search->depth || hash_data->selectivity < search->selectivity) {
				cassio_debug("Edax does not skip the search: Level %d@%d < %d@%d\n", hash_data->depth,selectivity_table[hash_data->selectivity].percent, search->depth, selectivity_table[search->selectivity].percent);
			} else {
				cassio_debug("Edax does not skip the search: unsolved score alpha %d < beta %d\n", alpha, beta); 
			}
		}
	} else {
		cassio_debug("Edax does not skip the search: Position %s (hash=%llx) not found\n", board_to_string(board, search->player, b), board_get_hash_code(board));
	}
	
	return false;
}


/**
 * @brief Midgame search.
 *
 * Edax evaluation resolution is only one disc, so the doubleing point accuracy is not
 * actually used here. In case (beta - alpha) < 1, Edax will reajust beta so that beta = alpha + 1.
 * For some depths, extension-pv heuristics will return endgame score.
 *
 * @param v Engine.
 * @param position String description of the position.
 * @param alpha Alpha bound.
 * @param beta Beta bound.
 * @param depth Search depth.
 * @param precision Probcut percentage.
 */
double engine_midgame_search(void *v, const char *position, const double alpha, const double beta, const int depth, const int precision)
{
	Engine *engine = (Engine*) v;
	Search *search = engine->search;
	Board board[1];
	int player;
	int old_score;
	
	if (search == NULL) {
		engine_send("ERROR: Engine need to be initialized.");
		return -SCORE_INF;
	}
	
	engine->is_searching = true;
	player = board_set(board, position);
	
	old_score = engine_open(search, board, player, floor(alpha), ceil(beta), depth, precision);
	
	if (skip_search(engine, &old_score)) {
	} else if (is_position_new(engine, board)) {
		cassio_debug("iterative deepening.\n");
		iterative_deepening(search, options.alpha, options.beta);
	} else {
		cassio_debug("aspiration search.\n");
		aspiration_search(search, options.alpha, options.beta, search->depth, old_score);
	}

	engine_close(search);
	engine->is_searching = false;

	return search->result->score * 1.0f;
}

/**
 * @brief Endgame search.
 *
 * Practically, Edax does the same things than in midgame search, with depth set as the number of empty squares.
 * In case (beta - alpha) < 1, Edax will reajust beta so that beta = alpha + 1.
 *
 * @param v Engine.
 * @param position String description of the position.
 * @param alpha Alpha bound.
 * @param beta Beta bound.
 * @param precision Probcut percentage.
 */
int engine_endgame_search(void *v, const char *position, const int  alpha, const int beta, const int precision)
{
	Engine *engine = (Engine*) v;
	Search *search = engine->search;
	Board board[1];
	int player;
	int old_score;
	int depth;
	
	if (search == NULL) {
		engine_send("ERROR: Engine need to be initialized.");
		return -SCORE_INF;
	}
	
	engine->is_searching = true;
	player = board_set(board, position);
	depth = board_count_empties(board);
	
	old_score = engine_open(search, board, player, alpha, beta, depth, precision);
	
	if (skip_search(engine, &old_score)) {
	} else if (is_position_new(engine, board)) {
		cassio_debug("iterative deepening.\n");
		iterative_deepening(search, options.alpha, options.beta);
	} else {
		cassio_debug("aspiration search.\n");
		aspiration_search(search, options.alpha, options.beta, search->depth, old_score);
	}
	
	engine_close(search);
	engine->is_searching = false;
	
	return search->result->score;
}

/**
 * @brief Stop searching.
 *
 * @param v Search.
 */
void engine_stop(void *v)
{
	Search *search = (Search*) v;
	if (search == NULL) {
		engine_send("ERROR: Engine need to be initialized.");
		return;
	}
	search_stop_all(search, STOP_ON_DEMAND);
}

/**
 * @brief Loop event
 */
void engine_loop(void)
{
	char *cmd = NULL, *param = NULL;
	Engine *engine = (Engine*) engine_init();

	// loop forever
	for (;;) {
		errno = 0;

		engine_wait_input(engine, &cmd, &param);

		if (cmd == NULL || *cmd == '\0') {

		} else if (strcmp(cmd, "init") == 0) {
			engine_send("ready."); 

		} else if (strcmp(cmd, "get-version") == 0) {
			engine_send("version: Edax %s", VERSION_STRING);
			engine_send("ready."); 

		} else if (strcmp(cmd, "new-position") == 0) {
			engine->last_position.n = 0;
			engine_send("ready.");

		} else if (strcmp(cmd, "feed-hash") == 0) {
			int depth = 21, precision = 73, player;
			double lower = -SCORE_INF, upper = SCORE_INF;
			Board board[1];
			Line pv[1];
			char *string;
			
			errno = 0;
			string = parse_board(param, board, &player);
			if (string == param) engine_send("Error: in feed-hash, Edax cannot parse position.");
			else {
				string = parse_real(string, &lower);
				if (errno) engine_send("Error: in feed-hash, Edax cannot parse lower.");
				else {
					string = parse_real(string, &upper);
					if (errno) engine_send("Error: in feed-hash, Edax cannot parse upper.");
					else {
						string = parse_int(string, &depth);
						if (errno) engine_send("Error: in feed-hash, Edax cannot parse depth.");
						else {
							string = parse_int(string, &precision);
							if (errno) engine_send("Error: in feed-hash, Edax cannot parse precision.");
							else {
								line_init(pv, player);
								parse_game(string, board, pv);
								engine_feed_hash(engine, board, floor(lower), ceil(upper), depth, precision, pv);
							}
						}
					}
				}
			}
			
		} else if (strcmp(cmd, "empty-hash") == 0) {
			engine_empty_hash(engine);
			
		} else if (strcmp(cmd, "quit") == 0 || strcmp(cmd, "eof") == 0) {
			engine_free(engine->search);
			event_free(engine->event);
			free(engine);
			free(cmd);
			free(param);
			engine_send("bye bye.");
			return;

		} else if (strcmp(cmd, "midgame-search") == 0) {
			double alpha = -SCORE_INF, beta = SCORE_INF;
			int player, depth = 60, precision = 100;
			Board board[1];
			char b[80];
			char *s;

			errno = 0;
			s = parse_board(param, board, &player);
			if (s == param) engine_send("ERROR: midgame-search cannot parse position.");
			else {
				s = parse_real(s, &alpha);
				if (errno) engine_send("ERROR: midgame-search cannot parse alpha.");
				else {
					s = parse_real(s, &beta);
					if (errno) engine_send("ERROR: midgame_search cannot parse beta.");
					else {
						s = parse_int(s, &depth);
						if (errno) engine_send("ERROR: midgame_search cannot parse depth.");
						else {
							s = parse_int(s, &precision);
							if (errno) engine_send("ERROR: midgame_search cannot parse precision.");
							engine_midgame_search(engine, board_to_string(board, player, b), alpha, beta, depth, precision);
						}
					}
				}
			}
			engine_send("ready."); 

		} else if (strcmp(cmd, "endgame-search") == 0) {
			int alpha = -SCORE_INF, beta = SCORE_INF;
			int player, precision = 100;
			Board board[1];
			char b[80];
			char *s;

			errno = 0;
			s = parse_board(param, board, &player);
			if (s == param) engine_send("ERROR: endgame_search cannot parse position.");
			else {
				s = parse_int(s, &alpha);
				if (errno) engine_send("ERROR: endgame_search cannot parse alpha.");
				else {
					s = parse_int(s, &beta);
					if (errno) engine_send("ERROR: endgame_search cannot parse beta.");
					else {
						s = parse_int(s, &precision);
						if (errno) engine_send("ERROR: endgame_search cannot parse precision.");
						engine_endgame_search(engine, board_to_string(board, player, b), alpha, beta, precision);
					}
				}
			}
			engine_send("ready."); 

		// ERROR: unknown message
		} else {
			engine_send("ERROR: unknown command %s", cmd);
			engine_send("ready.");
		}
	}
}

