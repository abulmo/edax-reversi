/**
 * @file obftest.c
 *
 * @brief Testing Edax from Othello Board Files (OBF).
 *
 * An Othello Board File has the following format:
 * <ul>
 *    <li> A single position per line.</li>
 *    <li> Any text following the percent symbol % is a comment.</li>
 *    <li> A position is a set of fields, each field is terminated by a semi-colon ;. </li>
 *    <li> The first field describes the Othello board. </li>
 *    <li> The other fields contain all the playable moves with their score, separated
 * by a colon: 'move':'score'. </li>    
 * </ul>
 *
 *
 * @date 1998 - 2012
 * @author Richard Delorme
 * @version 4.3
 */
#include "search.h"
#include "options.h"
#include "const.h"
#include "settings.h"


/** OBF structure: Othello Board File */
typedef struct OBF {
	Board board[1];   /**<! Othello board */
	int player;       /**<! Player on turn */
	struct {
		int x;        /**<! Move coordinate */
		int score;    /**<! Move score */
	} move[MAX_MOVE]; /**<! Move array */
	int n_moves;      /**<! Move number */
	int best_score;   /**<! Position score */
	char *comments;   /**<! Array of comments */
} OBF;

/** OBF parse status */
enum {
	OBF_PARSE_OK,
	OBF_PARSE_SKIP,
	OBF_PARSE_END
};

/** 
 * @brief Free an OBF structure.
 * @param obf OBF structure.
 */
void obf_free(OBF *obf)
{
	free(obf->comments);
	obf->comments = NULL;
}

/** 
 * @brief Write an OBF structure.
 * @param obf OBF structure.
 * @param f Output stream.
 */
static void obf_write(OBF *obf, FILE *f)
{
	char s[80];
	int i;

	if ((obf->board->player | obf->board->opponent) != 0) {
		board_to_string(obf->board, obf->player, s);
		fprintf(f, "%s;", s);
		for (i = 0; i < obf->n_moves; ++i) {
			putc(' ', f);
			move_print(obf->move[i].x, 0, f);
			fprintf(f, ":%+d;", obf->move[i].score);
		}
		if (i == 0) fprintf(f, " %+d;", obf->best_score);

	}
	if (obf->comments) printf(" %% %s", obf->comments);
	putc('\n', f);
	fflush(f);
}

/** 
 * @brief Read an OBF structure.
 * @param obf OBF structure.
 * @param f Input stream.
 * @return Parsing status.
 */
static int obf_read(OBF *obf, FILE *f)
{
	char *string, *line, *next;
	Move move[1];
	int parse_ok;

	obf->n_moves = 0;
	obf->best_score = -SCORE_INF;
	obf->board->player = obf->board->opponent = 0;
	obf->comments = NULL;

	line = string_read_line(f);
	if (line) {

		string = parse_skip_spaces(line);
		if (*string == '%') {
			obf->comments = string_duplicate(string + 1);
			parse_ok = OBF_PARSE_SKIP;
		} else if (*string == '\n' || *string == '\r' || *string == '\0') {
			parse_ok = OBF_PARSE_SKIP;
		} else {
			next = parse_board(string, obf->board, &obf->player);
			parse_ok = (next > string) ? OBF_PARSE_OK : OBF_PARSE_SKIP;
		}

		while (parse_ok == OBF_PARSE_OK && *(string = parse_find(next, ';')) == ';') {
			next = parse_move(++string, obf->board, move);
			if (next != string) {
				string = parse_find(next, ':');
				if (*string == ':') ++string;
				else {
					warn("missing score in %s (%d) %s %s\n", line, move->x, next, string);
					printf("read>"); obf_write(obf, stdout);
					parse_ok = OBF_PARSE_SKIP;
					break;
				}
			}
			move->score = -SCORE_INF;
			next = parse_int(string, &move->score);
			if (next == string && obf->best_score == -SCORE_INF) {
				warn("missing best score in %s\n", line);
				break;
			}

			if (move->x == NOMOVE && move->score == -SCORE_INF) {
				break;
			}

			if (move->score > obf->best_score) obf->best_score = move->score;
			obf->move[obf->n_moves].x = move->x;
			obf->move[obf->n_moves].score = move->score;
			++obf->n_moves;
		}

		free(line);
		return parse_ok;
	}

	return OBF_PARSE_END;
}

/** 
 * @brief Analyze an OBF structure.
 * @param search Search.
 * @param obf OBF structure.
 * @param n position number.
 */
static void obf_search(Search *search, OBF *obf, int n)
{
	int i, j;

//	search_cleanup(search);
	search_set_board(search, obf->board, obf->player);
	search_set_level(search, options.level, search->n_empties);
	if (options.depth >= 0) search->options.depth = MIN(options.depth, search->n_empties);
	if (options.selectivity >= 0) search->options.selectivity = options.selectivity;

	if (options.play_type == EDAX_TIME_PER_MOVE) search_set_move_time(search, options.time);
	else search_set_game_time(search, options.time);

	if (options.verbosity >= 2) {
		printf("\n*** problem # %d ***\n\n", n);
		board_print(search->board, search->player, stdout);
		putchar('\n');
		puts(search->options.header);
		puts(search->options.separator);
	} else if (options.verbosity == 1) printf("%3d|", n);

	search_run(search);

	if (options.verbosity) {
		if (options.verbosity == 1) { 
			result_print(search->result, stdout);
		}
		for (i = 0; i < obf->n_moves; ++i) {
			if (obf->move[i].x == search->result->move) break;
		}
		if (obf->best_score != -SCORE_INF) {
			putchar(' ');
			if (i < obf->n_moves) {
				if (obf->move[i].score != obf->best_score) {
					printf("Erroneous move: ");
					for (j = 0; j < obf->n_moves; ++j) {
						if (obf->move[j].score == obf->best_score) {
							move_print(obf->move[j].x, obf->player, stdout);
							putchar(' ');
						}
					}
					printf("expected, with score %+d, error = %+d", obf->best_score, obf->best_score - obf->move[i].score);
				}
			} else if (obf->best_score != search->result->score) {
				printf("Erroneous score: %+d expected", obf->best_score);
			}
		}
		putchar('\n');
		if (options.verbosity >= 2) {
			puts(search->options.separator);
		}
		fflush(stdout);
	}
}


/** 
 * @brief Build an OBF structure.
 * @param search Search.
 * @param obf OBF structure.
 * @param n position number.
 */
static void obf_build(Search *search, OBF *obf, int n)
{
	int n_moves;
	
	search_cleanup(search);
	search_set_board(search, obf->board, obf->player);
	search_set_level(search, options.level, search->n_empties);
	if (options.depth >= 0) {
		search->options.depth = MAX(options.depth, search->n_empties);
		search->options.selectivity = 0;
	}
	if (options.selectivity >= 0) search->options.selectivity = options.selectivity;

	if (options.play_type == EDAX_TIME_PER_MOVE) search_set_move_time(search, options.time);
	else search_set_game_time(search, options.time);


	if (options.verbosity >= 2) {
		printf("\n*** problem # %d ***\n\n", n);
		if (obf->comments) printf("* %s *\n\n", obf->comments);
		board_print(search->board, search->player, stdout);
		putchar('\n');
		puts(search->options.header);
		puts(search->options.separator);
		puts(search->options.separator);
	}

	obf->n_moves = 0;
	obf->best_score = -SCORE_INF;
	search->result->score = -SCORE_INF;
	n_moves = search->movelist->n_moves;

	if (n_moves == 0) {
		if (options.verbosity == 1) printf("%3d|", n);
		search_run(search);
		if (obf->best_score < search->result->score) obf->best_score = search->result->score;
		if (search->result->move == PASS) {
			obf->move[obf->n_moves].x = search->result->move;
			obf->move[obf->n_moves].score = search->result->score;
			++obf->n_moves;
		}
	}

	while (n_moves--) {
		if (options.verbosity == 1) printf("%3d|", n);

		search->options.multipv_depth = 60;
		search_run(search);
		search->options.multipv_depth = MULTIPV_DEPTH;

		obf->move[obf->n_moves].x = search->result->move;
		obf->move[obf->n_moves].score = search->result->score;
		if (obf->best_score < search->result->score) obf->best_score = search->result->score;
		++obf->n_moves;

		hash_exclude_move(search->pv_table, board_get_hash_code(search->board), search->result->move);
		hash_exclude_move(search->hash_table, board_get_hash_code(search->board), search->result->move);
		movelist_exclude(search->movelist, search->result->move);
	}

	if (options.verbosity) {
		puts(search->options.separator);
		if (options.verbosity >= 2) putchar('\n');
		fflush(stdout);
	}
}

/** 
 * @brief Test an OBF file.
 * @param search Search.
 * @param obf_file OBF file.
 * @param wrong_file OBF file with position wrongly analyzed.
 */
void obf_test(Search *search, const char *obf_file, const char *wrong_file)
{
	FILE *f, *w = NULL;
	OBF obf[1];
	unsigned long long T = 0, n_nodes = 0;
	int n = 0, n_bad_score = 0, n_bad_move = 0;
	double score_error = 0.0, move_error = 0.0;
	int i, ok;
	bool print_summary = false;

	// add observers
	search_cleanup(search);
	search_set_observer(search, search_observer);
	search->options.verbosity = (options.verbosity == 1 ? 0 : options.verbosity);
	options.width -= 4;

	// open script file with problems
	f = fopen(obf_file, "r");
	if (f == NULL) {
		fprintf(stderr, "obf_test: cannot open Othello Position Description's file %s\n", obf_file);
		exit(EXIT_FAILURE);
	}
	if (wrong_file && *wrong_file) {
		w = fopen(wrong_file, "w");
		if (w == NULL) {
			fprintf(stderr, "obf_test: cannot open Othello Position Description's file %s\n", wrong_file);
			exit(EXIT_FAILURE);
		}
	}
	
	if (options.verbosity == 1) {
		if (search->options.header) printf(" # |%s\n", search->options.header);
		if (search->options.separator) printf("---+%s\n", search->options.separator);
	}

	while ((ok = obf_read(obf, f)) != OBF_PARSE_END) {
		if (ok == OBF_PARSE_OK) {
			obf_search(search, obf, ++n);
		
			T += search_time(search);
			n_nodes += search_count_nodes(search);
			for (i = 0; i < obf->n_moves; ++i) {
				if (obf->move[i].x == search->result->move) break;
			}
			if (i < obf->n_moves) {
				if (obf->move[i].score < obf->best_score) ++n_bad_move;
				if (obf->move[i].score != search->result->score) ++n_bad_score;
				move_error += abs(obf->best_score - obf->move[i].score);
				if (w && obf->move[i].score < obf->best_score) obf_write(obf, w);
			} 
			if (obf->best_score > -SCORE_INF) score_error += abs(obf->best_score - search->result->score);
			else print_summary = true;
		}
		obf_free(obf);			
	}

	if (options.verbosity == 1 && search->options.separator) puts(search->options.separator);
	printf("%.30s: ", obf_file);
	if (n_nodes) printf("%llu nodes in ", n_nodes);
	time_print(T, false, stdout);
	if (T > 0 && n_nodes > 0) printf(" (%8.0f nodes/s).", 1000.0 * n_nodes / T);
	putchar('\n');
	
	if (print_summary) {
		printf("%d positions; ", n);
		printf("%d erroneous move; ", n_bad_move);
		printf("%d erroneous score; ", n_bad_score);
		printf("mean absolute score error = %.3f; ", score_error / n);
		printf("mean absolute move error = %.3f\n", move_error / n);
	}

	options.width += 4;

	fclose(f);
	if (w) fclose(w);
}

/** 
 * @brief Build an OBF file from a Script file.
 *
 * @param search Search.
 * @param script_file file with a set of position.
 * @param obf_file OBF file.
 */
void script_to_obf(Search *search, const char *script_file, const char *obf_file)
{
	
	FILE *i, *o;
	OBF obf[1];
	int n = 0, ok;

	// add observers
	search_set_observer(search, search_observer);
	search->options.verbosity = options.verbosity;

	// open script file with problems
	if (script_file == NULL || obf_file == NULL) {
		warn("script_to_obf: missing files\n");
		return;
	}
	if (strcmp(script_file, obf_file) == 0) {
		warn("script_to_obf: files should be different\n");
		return;
	}

	i = fopen(script_file, "r");
	if (i == NULL) {
		warn("script_to_obf: cannot open script file %s\n", script_file);
		return;
	}
	o = fopen(obf_file, "w");
	if (o == NULL) {
		warn("script_to_obf: cannot open obf file %s\n", obf_file);
		fclose(i);
		return;
	}
	
	if (options.verbosity == 1) {
		if (search->options.header) printf(" # |%s\n", search->options.header);
		if (search->options.separator) printf("---+%s\n", search->options.separator);
	}

	while ((ok = obf_read(obf, i)) != OBF_PARSE_END) {
		if (ok == OBF_PARSE_OK) {
			obf_build(search, obf, ++n);
		}
		obf_write(obf, o);
		obf_free(obf);			
	}

	if (options.verbosity == 1 && search->options.separator) puts(search->options.separator);
	putchar('\n');

	fclose(o);
	fclose(i);

}

/**
 * @brief Select hard position from an OBF file.
 * @param input_file OBF file.
 * @param output_file Filtered OBF file.
 */
void obf_filter(const char *input_file, const char *output_file)
{
	FILE *in, *out;
	int i, n, f, ok;
	int n_best, second_best;
	OBF obf[1];

	// open script file with problems
	in = fopen(input_file, "r");
	if (in == NULL) {
		fprintf(stderr, "obf_filter: cannot open Othello Position Description's file %s\n", input_file);
		exit(EXIT_FAILURE);
	}
	out = fopen(output_file, "w");
	if (out == NULL) {
		fprintf(stderr, "obf_filter: cannot open Othello Position Description's file %s\n", output_file);
		exit(EXIT_FAILURE);
	}
	
	n = f = 0;
	while ((ok = obf_read(obf, in)) != OBF_PARSE_END) {
		if (ok == OBF_PARSE_OK) {
			++n;
			n_best = 0;
			second_best = obf->best_score - 4;
			for (i = 0; i < obf->n_moves; ++i) {
				if (obf->move[i].score == obf->best_score) ++n_best;
				else if (obf->move[i].score > second_best) second_best = obf->move[i].score;
			}
			if (n_best == 1 && second_best == obf->best_score - 2) {
				++f;
				obf_write(obf, out);
			}
		}
		obf_free(obf);			
	}

	printf("OBF filter: %d selected out of %d positions\n", f, n);

	fclose(in);
	fclose(out);
}

