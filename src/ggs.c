/**
 * @file ggs.c
 *
 *  A ggs client in C language.
 *
 * @date 2002 - 2017
 * @author Richard Delorme
 * @version 4.4
 */

#include "board.h"
#include "move.h"
#include "options.h"
#include "util.h"
#include "ui.h"

#include <assert.h>
#include <ctype.h>
#include <errno.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


#include <sys/types.h>
#ifdef _WIN32
	#include <winsock.h>
	#include <winerror.h>
	#include <ws2tcpip.h>
	#define SHUT_RDWR SD_BOTH
#else
	#include <unistd.h>
	#include <sys/socket.h>
	#include <netdb.h>
	#include <netinet/tcp.h>
#endif


/** Word size */
#define WORD_SIZE 256

/** Board Size */
#define GGS_BOARD_SIZE 256

/** Move list size */
#define MOVELIST_SIZE 256

/** Text (set of lines) representation */
typedef struct Text {
	const char** line; /**< array of lines */
	int n_lines;       /**< number of lines */
} Text;

/** GGS clock (to play a game) */
typedef struct GGSClock {
	int ini_flag; /**< base time flag. true = NOT loss */
	int inc_flag; /**< incremental time flag.  true = NOT additive */
	int ext_flag; /**< extra time flag. true = NOT add */

	int ini_move; /**< number of moves allowed during base time */
	int inc_move; /**< number of moves allowed during incremental time */
	int ext_move; /**< number of moves allowed during extra time */

	int ini_time; /**< base time in ms. */
	int inc_time; /**< incremental time in ms. */
	int ext_time; /**< extra time in ms. */
} GGSClock;

/** GGS player */
typedef struct GGSPlayer {
	char *name;     /**< player's name on GGS */
	double rating;  /**< player's rating */
} GGSPlayer;

/** Match type description */
typedef struct GGSMatchType {
	int is_saved;   /**< is game saved ? */
	int is_rated;   /**< is game rated ? */
	int is_synchro; /**< is game synchro ? */
	int is_komi;    /**< is game komi ? */
	int is_rand;    /**< is game rand ? */
	int is_anti;    /**< is game anti ? */
	int discs;      /**< game disc number */
	int size;       /**< game size */
} GGSMatchType;

/* match off message */
typedef struct GGSMatchOff {
	char *id;              /**< match id */
	GGSPlayer player[2];   /**< match players */
} GGSMatchOff;

/* match on message */
typedef struct GGSMatchOn {
	char *id;                   /**< match id */
	GGSPlayer player[2];        /**< match players */
	GGSMatchType match_type[1]; /**< match type */
} GGSMatchOn;

/* match request message */
typedef struct GGSRequest {
	char *id;                   /**< match request id */
	GGSPlayer player[2];        /**< match players */
	GGSMatchType match_type[1]; /**< match type */
	GGSClock clock[2];          /**< match clock */
} GGSRequest;

/* match board message */
typedef struct GGSBoard {
	char *id;                   /**< match request id */
	GGSPlayer player[2];        /**< match players */
	GGSMatchType match_type[1]; /**< match type */
	GGSClock clock[2];          /**< match clock */
	double komi;                /**< komi value */
	int is_join;                /**< join a new game ? */
	int is_update;              /**< update an existing game? */  
	int move;                   /**< move */
	int move_no;                /**< move number */
	char color[2];              /**< color */
	char board[GGS_BOARD_SIZE]; /**< current board */
	char turn;                  /**< player on turn */
	char board_init[GGS_BOARD_SIZE]; /**< initial board */
	char turn_init;             /**< first player */
	int move_list[MOVELIST_SIZE]; /**< list of played moves */
	int move_list_n;              /**< number of played moves */                
} GGSBoard;


/* match admin message */
typedef struct GGSAdmin {
	char *command;           /**< admin command */
	char name[16];           /**< admin name */
} GGSAdmin;

/* GGSEvent. Deals with input from ggs */
typedef struct GGSEvent {
	int socket;                 /**< socket */
	bool loop;                  /**< loop */
	char *buffer;               /**< read buffer */
	Thread thread;              /**< thread */
	Lock lock;                  /**< lock */
} GGSEvent;

/* GGSClient structure */
typedef struct GGSClient {
	GGSBoard board[1];          /**< ggs board */
	GGSRequest request[1];      /**< ggs request */
	GGSMatchOn match_on[1];     /**< ggs match on */ 
	GGSMatchOff match_off[1];   /**< ggs match off */
	GGSAdmin admin[1];          /**< ggs admin */
	GGSEvent event;             /**< ggs event */
	const char *me;             /**< Edax's name on GGS */
	bool is_playing;            /**< is Edax playing ? */
	long long last_refresh;     /**< date of last refresh */
	struct {
		char *cmd;              /**< command */
		int i;                  /**< iteration number */
		long long delay;        /**< delay between commands */
	} loop[1];                  /**< loop instruction */
	struct {                    
		char *cmd;              /**< command */              
		long long delay;        /**< delay */
	} once[1];		            /**< command issued once after some delay */
} GGSClient;

static Log ggs_log[1];

static char admin_list[][16] = {"delorme", "dan", "mic", "romano", "HCyrano", "romelica", ""};

static const GGSClock GGS_CLOCK_INI = {0, 0, 0, 0, 0, 0, 0, 0, 0};
static const GGSMatchType GGS_MATCH_TYPE_INI = {0, 0, 0, 0, 0, 0, 0, 0};

/**
 * @brief text_init
 *
 * Initialise a Text structure.
 *
 * @param text Text.
 */
static void text_init(Text *text)
{
	text->line = NULL;
	text->n_lines =0;
}

/**
 * @brief text_add_line
 *
 * Add a line to a Text.
 *
 * @param text Text.
 * @param line line of text.
 */
static void text_add_line(Text *text, const char *line)
{
	++text->n_lines;
	text->line = (const char**) realloc(text->line, text->n_lines * sizeof (const char*));
	if (text->line == NULL) fatal_error("Allocation error\n");
	text->line[text->n_lines - 1] = line;
	log_receive(ggs_log, "GGS ", "%s\n", line);
}

/**
 * @brief text_print
 *
 * Print a Text structure.
 *
 * @param text Text.
 * @param f Output stream.
 */
static void text_print(Text *text, FILE *f)
{
	int i;
	
	for (i = 0; i < text->n_lines; ++i) {
		fprintf(f, "GGS> %s\n", text->line[i]);
	}
}

/**
 * @brief text_free
 *
 * Free a Text structure.
 *
 * @param text Text.
 */
static void text_free(Text *text)
{

	int i;
	
	for (i = 0; i < text->n_lines; ++i) {
		free((void*)text->line[i]);
	}
	free((void*)text->line);
	text_init(text);
}

/**
 * @brief ggs_parse_line
 *
 * Parse a ('\n' terminated) line from a buffer.
 *
 * @param buffer buffer.
 * @param line Newly allocated line of text.
 * @return buffer's remaining.
 */
static char *ggs_parse_line(const char *buffer, char **line)
{
	int n;
	assert(buffer);

	*line = NULL;	
	for (n = 0; buffer[n] && buffer[n] != '\n' && buffer[n] != '\r'; ++n) ;
	if (buffer[n]) { /* complete line found */
		*line = (char*) malloc(n + 1);
		if (*line == NULL) fatal_error("Allocation error\n");
		if (n > 0) memcpy(*line, buffer, n); 
		(*line)[n] = '\0';

		while (buffer[n] == '\n' || buffer[n] == '\r') ++n;
	} else { /* no or incomplete line found */
		n = 0;
	}
	return (char*) (buffer + n);
}

/**
 * @brief ggs_parse_text
 *
 * Parse a text (ie a set of lines) from a buffer.
 *
 * @param buffer Buffer.
 * @param text Text.
 * @return buffer's remaining.
 */
static char* ggs_parse_text(const char *buffer, Text *text)
{
	char *line;
	assert(buffer);

	while (buffer[0] == '|' || text->n_lines == 0) {
		buffer = ggs_parse_line(buffer, &line);
		if (!line) break;
		text_add_line(text, line);
	};

	return (char*) buffer;
}


/**
 * @brief ggs_parse_int
 *
 * Translate input word into an integer.
 *
 * @param value integer.
 * @param word Word.
 * @return true if parsing succeed.
 */
static bool ggs_parse_int(int *value, const char *word)
{
	errno = 0;
	parse_int(word, value);
	return errno == 0;
}

/**
 * @brief ggs_parse_double
 *
 * Translate input word into a double.
 *
 * @param value double.
 * @param word Word.
 * @return true if parsing succeed.
 */
static bool ggs_parse_double(double *value, const char *word) {
	errno = 0;
	parse_real(word, value);
	return errno == 0;
}

/**
 * @brief ggs_parse_move
 *
 * Translate input word into a move.
 *
 * @param move Move.
 * @param word Word.
 * @return true if parsing succeed.
 */
static bool ggs_parse_move(int *move, const char *word) {
	*move = string_to_coordinate(parse_skip_spaces(word));
	return *move != NOMOVE;
}

/**
 * @brief ggs_parse_move
 *
 * Translate input word into time (as ms).
 *
 * @param time Time.
 * @param word Word.
 * @return true if parsing succeed.
 */
static bool ggs_parse_time(int *time, const char *word) {
	errno = 0;	
	*time = string_to_time(parse_skip_spaces(word));
	return errno == 0;
}

/**
 * @brief ggs_parse_clock
 *
 * Translate input word into a GGS clock time.
 *
 * @param ggsclock GGS clock.
 * @param line Text.
 * @return true if parsing succeed.
 */
static bool ggs_parse_clock(GGSClock *ggsclock, const char *line) {
	char time[WORD_SIZE];
	const char *move;
	char word[WORD_SIZE];

	errno = 0;
	*ggsclock = GGS_CLOCK_INI;
	line = parse_field(line, word, WORD_SIZE, '/');
	if (*word) {
		move = parse_field(word, time, WORD_SIZE, ',');
		if (!ggs_parse_time(&ggsclock->ini_time, time)) return false;
		if (*move) {
			if (*move == 'N') {
				ggsclock->ini_flag = 1;
				move++;
			}
			if (!ggs_parse_int(&ggsclock->ini_move, move)) return false;
		}
	}

	line = parse_field(line, word, WORD_SIZE, '/');
	if (*word) {
		move = parse_field(word, time, WORD_SIZE, ',');
		if (!ggs_parse_time(&ggsclock->inc_time, time)) return false;
		if (*move) {
			if (*move == 'N') {
				ggsclock->inc_flag = 1;
				move++;
			}
			if (!ggs_parse_int(&ggsclock->inc_move, move)) return false;
		}
	}

	line = parse_field(line, word, WORD_SIZE, '\0');
	if (*word) {
		move = parse_field(word, time, WORD_SIZE, ',');
		if (!ggs_parse_time(&ggsclock->ext_time, time)) return false;
		if (*move) {
			if (*move == 'N') {
				ggsclock->ext_flag = 1;
				move++;
			}
			if (!ggs_parse_int(&ggsclock->ext_move, move)) return false;
		}
	}

	return true;
}


/**
 * @brief ggs_player_set
 *
 * Set player's name and rating.
 *
 * @param player GGS player.
 * @param name Name.
 * @param rating Rating.
 * @return true if parsing succeed.
 */
/* parse player name & rating */
static bool ggs_player_set(GGSPlayer *player, const char *name, const char *rating) {
	char word[WORD_SIZE];

	if (name == NULL || *name == '\0') return false;

	parse_word(name, word, WORD_SIZE);
	player->name = string_duplicate(word);

	rating = parse_skip_spaces(rating);	
	if (*rating == '(') ++rating;
	return ggs_parse_double(&(player->rating), rating);
}

/**
 * @brief ggs_match_type_set
 *
 * Translate input word into a type of match.
 *
 * @param type GGS match type.
 * @param word Word.
 */
static void ggs_match_type_set(GGSMatchType *type, const char *word) {
	const char *c;

	*type = GGS_MATCH_TYPE_INI;

	if (word) {
		if (*word == 's') {
			type->is_synchro = 1;
			type->size = string_to_int(word + 1, 0);
		} else {
			type->size = string_to_int(word, 0);
		}
		for (c = word; *c; c++) {
			if (*c == 'k') type->is_komi = 1;
			if (*c == 'r') {
				type->is_rand = 1;
				type->discs = string_to_int(c + 1, 0);
			}
			if (*c == 'a') type->is_anti = 1;
		}
	}
}


/**
 * @brief ggs_player_free
 *
 * Free a GGS player structure.
 *
 * @param player GGS player.
 */
static void ggs_player_free(GGSPlayer *player) {
	assert(player);
	free(player->name);
	player->name = NULL;
}

/**
 * @brief ggs_MBR_free
 *
 * Free a GGS MatchOn/Off / Board / Request structure.
 *
 * @param v GGS match/Board or Request structure.
 */
static void ggs_MBR_free(void* v) {
	GGSMatchOff *m = (GGSMatchOff*) v; 
	assert(m);
	free(m->id);
	m->id = 0;
	ggs_player_free(m->player);
	ggs_player_free(m->player + 1);
}

/**
 * @brief ggs_match_check_destination
 *
 * Check if Edax is concerned by the match.
 *
 * @param player GGS player.
 * @param me Edax's login name.
 * @return 'true' if Edax is playing that match.
 */
static bool ggs_has_player(GGSPlayer *player, const char *me) {
	return  (strcmp(me, player[0].name) == 0 || strcmp(me, player[1].name) == 0);
}

/**
 * @brief ggs_request
 *
 * Parse a request from a GGS input text.
 *
 * @param request GGS request.
 * @param text GGS input text.
 * @return 'true' if the text is a valid request.
 */
static bool ggs_request(GGSRequest *request, Text *text)
{
	const char *line = text->line[0];
	char word[WORD_SIZE];

	line = parse_word(line, word, WORD_SIZE);
	if (strcmp("/os:", word) == 0) {
		line = parse_word(line, word, WORD_SIZE);
		if (strcmp("+", word) == 0) {
			if (*line) {
				line = parse_word(line, word, WORD_SIZE);
				request->id = string_duplicate(word);
				return true;
			}
		}
	}

	return false;
}

/**
 * @brief ggs_match_on
 *
 * Parse a "match on" from a GGS input text.
 *
 * @param match GGS match on.
 * @param text GGS input text.
 * @return 'true' if the text is a valid match start.
 */
static bool ggs_match_on(GGSMatchOn *match, Text *text)
{
	const char *line = text->line[0];
	char word[WORD_SIZE];

	line = parse_word(line, word, WORD_SIZE);
	if (strcmp("/os:", word) != 0) return false;

	line = parse_word(line, word, WORD_SIZE);
	if (strcmp("+", word) != 0) return false;

	line = parse_word(line, word, WORD_SIZE);
	if (strcmp("match", word) != 0) return false;

	line = parse_word(line, word, WORD_SIZE);
	match->id = string_duplicate(word);

	line = parse_word(line, word, WORD_SIZE);
	if (!ggs_player_set(&(match->player[0]), line, word))return false;

	line = parse_skip_word(line);

	line = parse_word(line, word, WORD_SIZE);
	if (!ggs_player_set(&(match->player[1]), line, word)) return false;
	line = parse_skip_word(line);

	line = parse_word(line, word, WORD_SIZE);
	ggs_match_type_set(match->match_type, word);

	line = parse_word(line, word, WORD_SIZE);
	match->match_type->is_rated = (strcmp(word, "R") == 0) ? 1 : 0;

	return true;
}

/**
 * @brief ggs_match_off
 *
 * Parse a "match off" from a GGS input text.
 *
 * @param match GGS match off.
 * @param text GGS input text.
 * @return 'true' if the text is a valid match end.
 */
static bool ggs_match_off(GGSMatchOff *match, Text *text)
{
	const char *line = text->line[0];
	char word[WORD_SIZE];

	line = parse_word(line, word, WORD_SIZE);
	if (strcmp("/os:", word) != 0) return false;

	line = parse_word(line, word, WORD_SIZE);
	if (strcmp("-", word) != 0) return false;

	line = parse_word(line, word, WORD_SIZE);
	if (strcmp("match", word) != 0) return false;

	line = parse_word(line, word, WORD_SIZE);
	match->id = string_duplicate(word);

	line = parse_word(line, word, WORD_SIZE);
	if (!ggs_player_set(&(match->player[0]), line, word)) return false;
	line = parse_skip_word(line);

	line = parse_word(line, word, WORD_SIZE);
	if (!ggs_player_set(&(match->player[1]), line, word)) return false;

	return true;
}

/**
 * @brief ggs_board
 *
 * Parse a "GGS board" from a GGS input text.
 *
 * @param board GGS board.
 * @param text GGS input text.
 * @return 'true' if the text is a valid board.
 */
static bool ggs_board(GGSBoard *board, Text *text)
{
	const char *line = text->line[0];
	char word[WORD_SIZE];
	int ii = 1, i, j;

	if (text->n_lines < 17) return false;

	if (*line == '\0') return false;

	line = parse_word(line, word, WORD_SIZE);
	if (strcmp("/os:", word) != 0) return false;

	if (*line == '\0') return false;
	line = parse_word(line, word, WORD_SIZE);
	board->is_join = (strcmp("join", word) == 0);
	board->is_update = (strcmp("update", word) == 0);

	if ( !board->is_update && !board->is_join) {
		return false;
	}

	if (*line == '\0') return false;
	line = parse_word(line, word, WORD_SIZE);
	board->id = string_duplicate(word);

	if (*line == '\0') return false;
	line = parse_word(line, word, WORD_SIZE);
	ggs_match_type_set(board->match_type, word);

	if (*line) {
		line = parse_word(line, word, WORD_SIZE);
		if (word[1] == '?') {
			board->match_type->is_komi = 0;
			board->komi = 0;
		} else {
			board->match_type->is_komi = 1;
			ggs_parse_double(&board->komi, word + 1);
		}
	}

	if (board->is_join) {
		if (!ggs_parse_int(&board->move_list_n, text->line[1] + 1))
			return false;
		if (board->move_list_n > 0) {
			if (text->n_lines < board->move_list_n + 30)
				return false;
			ii = 5;
			for (i = 0; i < 8; i++) {
	    		++ii;
				for (j = 0; j < 8; j++)
					board->board_init[i * 8 + j] = text->line[ii][4 + j * 2];
			}
			ii += 3;
			board->turn_init = text->line[ii][1];

			for (i = 0; i < board->move_list_n; i++) {
				++ii;
				parse_field(text->line[ii] + 6, word, WORD_SIZE, '/');
				ggs_parse_move(board->move_list + i, word);
			}
		} else ii = 2;
	}
	if (text->n_lines < ii + 14)
		return false;

	line = parse_skip_word(text->line[ii]);
	line = parse_word(line, word, WORD_SIZE);
	ggs_parse_int(&board->move_no, word);
	ggs_parse_move(&board->move, line);

	++ii;
	line = parse_word(text->line[ii], word, WORD_SIZE);
	if (!ggs_player_set(board->player, word + 1, line))
		return false;
	line = parse_skip_word(line + 1);
	line = parse_word(line, word, WORD_SIZE);
	board->color[0] = word[0];
	if (!ggs_parse_clock(board->clock, line))	return false;

	++ii;
	line = parse_word(text->line[ii], word, WORD_SIZE);
	if (!ggs_player_set(board->player + 1, word + 1, line))
		return false;
	line = parse_skip_word(line + 1);
	line = parse_word(line, word, WORD_SIZE);
	board->color[1] = word[0];
	if (!ggs_parse_clock(board->clock + 1, line)) return false;

	ii += 2;

	for (i = 0; i < 8; i++) {
	    ++ii;
		for (j = 0; j < 8; j++)
			board->board[i * 8 + j] = text->line[ii][4 + j * 2];
	}
	ii += 3;
	board->turn = text->line[ii][1];

	if (board->is_join && board->move_list_n == 0) {
		memcpy(board->board_init, board->board, 64);
		board->turn_init = board->turn;
	}

	return true;
}


/**
 * @brief ggs_os_on
 *
 * Parse a "os on" from a GGS input text.
 * PS: os = (o)thello (s)ervice
 *
 * @param text GGS input text.
 * @return 'true' if the text is a "os on".
 */
static bool ggs_os_on(Text *text)
{
	if (strcmp(text->line[0], ": + /os 1") == 0) return true;
	return false;
}

/**
 * @brief ggs_os_off
 *
 * Parse a "os off" from a GGS input text.
 * PS: os = (o)thello (s)ervice
 *
 * @param text GGS input text.
 * @return 'true' if the text is a valid board.
 */
static bool ggs_os_off(Text *text)
{
	if (strcmp(text->line[0], ": - /os 1") == 0) return true;
	return false;
}

/**
 * @brief ggs_saio_delay
 *
 * Parse a "saio" from a GGS input text.
 * When Saio is building is book, it tells when it will be available again.
 * Edax will automatically re-start asking him to play again at the right moment.
 *
 * @param text GGS input text.
 * @param delay Time delay (in ms).
 * @return 'true' if the text is a valid saio delay.
 */
static bool ggs_saio_delay(Text *text, long long *delay)
{
	const char *s = strstr(text->line[0], "Sorry, i will accept new games in");

	if (s && strlen(s) > 35) {
		errno = 0;
		*delay = string_to_int(s + 34, 300) * 1000 + real_clock();
		return (errno == 0);
	}
	return false;
}


/**
 * @brief ggs_admin
 *
 * Parse an admin command from a GGS input text.
 * Admin are authorised people (see list above) that can interact with Edax.
 * Usually, edax will repeat the command to GGS.
 *
 * @param admin An authorised person.
 * @param text GGS input text.
 * @return 'true' if the text is an authorised admin.
 */
static int ggs_admin(GGSAdmin *admin, Text *text)
{
	const char *line = text->line[0];
	char word[WORD_SIZE];
	int i;

	line = parse_word(line, word, WORD_SIZE);
	for (i = 0; *admin_list[i]; i++) {
		if (strncmp(admin_list[i], word, strlen(admin_list[i])) == 0) {
			admin->command = string_duplicate(line);
			strcpy(admin->name, admin_list[i]);
			return true;
		}
	}

	return false;
}

/**
 * @brief ggs_ready
 *
 * Parse a "READY" from a GGS input text.
 *
 * @param text GGS input text.
 * @return 'true' if the text is "READY".
 */
static bool ggs_ready(Text *text)
{
	const char *line = text->line[0];
	return strcmp(line, "READY") == 0;
}

/**
 * @brief ggs_alert
 *
 * Parse a "ALERT" from a GGS input text.
 *
 * @param text GGS input text.
 * @return 'true' if the text is "ALERT".
 */
static bool ggs_alert(Text *text)
{
	const char *line = text->line[0];
	return strcmp(line, "ALERT") == 0;
}

/**
 * @brief ggs_login
 *
 * Parse a login request from a GGS input text.
 *
 * @param text GGS input text.
 * @return 'true' if the text is "ALERT".
 */
static bool ggs_login(Text *text)
{
	const char *line = text->line[0];
	return strcmp(line, ": Enter login (yours, or one you'd like to use).") == 0;
}

/**
 * @brief ggs_password
 *
 * Parse a login password from a GGS input text.
 *
 * @param text GGS input text.
 * @return 'true' if the text is "ALERT".
 */
static bool ggs_password(Text *text)
{
	const char *line = text->line[0];
	return strcmp(line, ": Enter your password.") == 0;
}

/**
 * @brief ggs_event_loop
 *
 * GGS event loop. Read all inputs from GGS within a thread.
 *
 * @param v Event.
 * @return NULL.
 */
static void* ggs_event_loop(void *v)
{
	GGSEvent *event = (GGSEvent*) v;
	enum {BUCKET_SIZE = 16384};
	char buffer[BUCKET_SIZE + 1] = "";
	int r, l;

	while (event->loop) {
		r = recv(event->socket, buffer, BUCKET_SIZE, 0);
		if (r > 0) {
			lock(event);
				l = strlen(event->buffer);
				event->buffer = (char*) realloc(event->buffer, r + l + 1);
				memcpy(event->buffer + l, buffer, r);
				event->buffer[l + r] = '\0';
				*buffer = '\0';
			unlock(event);
		} else {
			event->loop = false;
		}
	}

	return NULL;
}

/**
 * @brief ggs_event_init
 *
 * GGS event init.
 *
 * @param event Event.
 */
static void ggs_event_init(GGSEvent *event)
{
	struct addrinfo hints;
	struct addrinfo *result, *rp = NULL;

	lock_init(event);
	event->loop = true;
	event->buffer = (char*) calloc(1, 1);

/* Windows needs this */
#ifdef _WIN32
	{
		WSADATA wsaData;
		int value = WSAStartup(MAKEWORD(2,2), &wsaData);
		if (value != NO_ERROR) {
			fatal_error("WSAStartup failed: %d \n", value);
		}
	}
#endif

	/* socket creation */
	memset(&hints, 0, sizeof (struct addrinfo));
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_PASSIVE;
	hints.ai_protocol = getprotobyname("tcp")->p_proto;
	hints.ai_canonname = NULL;
	hints.ai_addr = NULL;
	hints.ai_next = NULL;

	if (getaddrinfo(options.ggs_host, options.ggs_port, &hints, &result) == 0) {
		for (rp = result; rp != NULL; rp = rp->ai_next) {
			event->socket = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);
			if (event->socket != -1) {
				if (connect(event->socket, rp->ai_addr, rp->ai_addrlen) != -1) break; /* Success */
#ifdef _WIN32
				closesocket(event->socket);
				WSACleanup();
#else
				close(event->socket);
#endif
			}
		}
		if (rp == NULL) {
			fatal_error("Could not connect to %s %s\n", options.ggs_host, options.ggs_port);
		}
  		freeaddrinfo(result);
	}

	thread_create2(&event->thread, ggs_event_loop, event); // modified for iOS by lavox. 2018/1/16
}

/**
 * @brief ggs_event_free
 *
 * GGS event free.
 *
 * @param event Event.
 */
static void ggs_event_free(GGSEvent *event)
{
	event->loop = false;
	shutdown(event->socket,  SHUT_RDWR);
#ifdef _WIN32
	closesocket(event->socket);
	WSACleanup();
#else
	close(event->socket);
#endif
	thread_join(event->thread);
	free(event->buffer);
	lock_free(event);
}

/**
 * @brief ggs_event_peek
 *
 * Check if an event occured. If so copy the GGS input to a text,
 * i.e. a set of lines.
 *
 * @param event Event.
 * @param text Text.
 * @return 'true' if an event occured.
 */
static bool ggs_event_peek(GGSEvent *event, Text *text)
{
	bool ok = false;
	const char *s;

	lock(event);
	if (*event->buffer) {
		s = ggs_parse_text(event->buffer, text);
		if (s > event->buffer) {
			memmove(event->buffer, s, strlen(s) + 1);
			ok = (*event->buffer != '|');
		}
	}
	unlock(event);

	return ok;
}

/**
 * @brief ggs_client_free
 *
 * Free the GGS client structure.
 *
 * @param client GGS client.
 */
static void ggs_client_free(GGSClient *client) {
	ggs_event_free(&client->event);
	ggs_MBR_free(client->board);
	ggs_MBR_free(client->request);
	ggs_MBR_free(client->match_on);
	ggs_MBR_free(client->match_off);
}

/**
 * @brief ggs_client_send
 *
 * Send a message to the GGS server.
 * This function uses vnsprintf with some workaround
 * for MS-Windows incorrect behaviour.
 *
 * @param client GGS client.
 * @param fmt Text format.
 * @param ... other args.
 */
static void ggs_client_send(GGSClient *client, const char *fmt, ...) {
	int message_length;
	int size = 256;
	char *buffer;
 	va_list ap;

	if (fmt == NULL) return ;
	
	for (;;) {
		buffer = (char*) malloc(size + 1);
		if (buffer == NULL) return;
		va_start(ap, fmt);
		message_length = vsnprintf(buffer, size, fmt, ap);
		va_end(ap);

		if (message_length > 0 && message_length < size) break;

		if (message_length > 0) size = message_length + 1;
		else size *= 2;
		free(buffer);
	}

	log_send(ggs_log, "GGS", "%s", buffer);
	printf("GGS< %s", buffer);
	send(client->event.socket, buffer, message_length, 0);
	
	free(buffer);
}

/**
 * @brief ggs_client_refresh
 *
 * Periodically send a message to the GGS server.
 * This is a workaround to avoid spurious deconnection from GGS.
 *
 * @param client GGS client.
  */
static void ggs_client_refresh(GGSClient *client) 
{
	/* send "tell /os continue" every minute */
	if (real_clock() - client->last_refresh > 60000) { // 60 sec.
		if (client->is_playing) ggs_client_send(client, "tell /os open 0\n" );
		else ggs_client_send(client, "tell /os open %d\n", options.ggs_open);
		ggs_client_send(client, "tell /os continue\n");
		client->last_refresh = real_clock();
	}	

	/* check loop */
	if (client->loop->delay != 0 && real_clock() - client->loop->delay > 0) {
		client->loop->delay = 0;
		ggs_client_send(client, "%s\n", client->loop->cmd);
	}	

	/* check once */
	if (client->once->cmd && client->once->delay != 0 && real_clock() - client->once->delay > 0) {
		client->once->delay = 0;
		ggs_client_send(client, "%s\n", client->once->cmd);
	}
}

/**
 * @brief ui_login
 *
 * Connect to GGS.
 *
 * @param ui User Interface.
 */
static void ui_login(UI *ui) 
{
	GGSClient *client = ui->ggs;
	
	/* sanity check */
	if (options.ggs_host == NULL) fatal_error("Unknown GGS host\n");
	if (options.ggs_port == NULL) fatal_error("Unknown GGS port\n");
	if (options.ggs_login == NULL) fatal_error("Unknown GGS login\n");
	if (options.ggs_password == NULL) fatal_error("Unknown GGS password\n");
	if (strlen(options.ggs_login) > 8) fatal_error("Bad GGS login %s (too much characters)\n", options.ggs_login);

	printf("Connecting to GGS...\n");
	memset(client, 0, sizeof (GGSClient));
	client->me = options.ggs_login;
	client->loop->cmd = NULL;
	client->loop->i = 0;
	client->loop->delay = 0;

	client->once->cmd = NULL;
	client->once->delay = 0;
	client->last_refresh = real_clock();

	ggs_event_init(&client->event);
}

/**
 * @brief ui_ggs_ponder
 *
 * Ponder, ie search during opponent time.
 *
 * @param ui User Interface.
 * @param turn Edax's color.
 */
static void ui_ggs_ponder(UI *ui, int turn) {
	Play *play;

	if (search_count_tasks(ui->play->search) == options.n_task) {
		play = ui->play;
	} else  {
		play = ui->play + turn;
	}

	play_ponder(play);
}

/**
 * @brief ui_ggs_play
 *
 * Search the best move.
 *
 * @param ui User Interface.
 * @param turn Edax's color.
 */
static void ui_ggs_play(UI *ui, int turn) {
	long long real_time = -time_clock();
	int remaining_time = ui->ggs->board->clock[turn].ini_time;
	int extra_time = ui->ggs->board->clock[turn].ext_time;
	Play *play;
	Result *result;
	char move[4], line[32];
	const char *(search_state_array[6]) = {"running", "interrupted", "stop pondering", "out of time", "stopped on user demand", "completed"};
	char search_state[32];

	if (ui->is_same_play) {
		play = ui->play;
		if (search_count_tasks(ui->play->search) < options.n_task) {
			printf("<use a single %d tasks search while a single game is played>\n", options.n_task);
			play_stop_pondering(ui->play);
			search_set_task_number(ui->play[0].search, options.n_task);
			play_stop_pondering(ui->play + 1);
			search_set_task_number(ui->play[1].search, 0);
		}
	} else {
		play = ui->play + turn;
		if (search_count_tasks(ui->play->search) == options.n_task && options.n_task > 1) {
			printf("<split single %d tasks search into two %d task searches>\n", options.n_task, options.n_task / 2);
			play_stop_pondering(ui->play);
			search_set_task_number(ui->play[0].search, options.n_task / 2);
			play_stop_pondering(ui->play + 1);
			search_set_task_number(ui->play[1].search, options.n_task / 2);
			search_share(ui->play[0].search, ui->play[1].search);
			ui_ggs_ponder(ui, turn ^ 1); // ponder opponent move
		}
	}

	// game over detection...
	if (play_is_game_over(play)) {
		ggs_client_send(ui->ggs, "tell .%s *** GAME OVER ***\n", ui->ggs->me);
		return ;
	}

	result = play->result;
	
	if (remaining_time > 60000) remaining_time -= 10000; // keep 10s. for safety.
	else if (remaining_time > 10000) remaining_time -= 2000; // keep 2s. for safety.
	if (remaining_time < 1000) remaining_time = 1000; // set time to at list 1ms
	play_adjust_time(play, remaining_time, extra_time);

	printf("<ggs: go thinking>\n");
	play_go(play, false);

	real_time += time_clock();

	move_to_string(result->move, play->player, move);

	ggs_client_send(ui->ggs, "tell /os play %s %s/%d/%.2f\n", ui->ggs->board->id, move, result->score, 0.001 * (real_time + 1));

	if (result->book_move) {
		printf("[%s plays %s in game %s ; score = %d from book]\n", ui->ggs->me, move, ui->ggs->board->id, result->score);
		ggs_client_send(ui->ggs, "tell .%s -----------------------------------------"
			"\\%s plays %s in game %s"
			"\\score == %d from book\n",
			ui->ggs->me,
			ui->ggs->me, move, ui->ggs->board->id,
			result->score
		);
	} else if (play->search->n_empties >= 15) { //avoid noisy display
		const char *bound;
		char s_nodes[16], s_speed[16];

		if (result->bound[result->move].lower < result->score && result->score == result->bound[result->move].upper) bound = "<=";
		else if (result->bound[result->move].lower == result->score && result->score < result->bound[result->move].upper) bound = ">=";
		else bound = "==";

		info("<%s plays %s in game %s ; score = %d at %d@%d%% ; %lld nodes in %.1fs (%.0f nodes/s.)>\n",
			ui->ggs->me, move, ui->ggs->board->id,
			result->score, result->depth, selectivity_table[result->selectivity].percent,
			result->n_nodes, 0.001 * real_time, (result->n_nodes / (0.001 * real_time + 0.001))
		);
		if (play->search->stop == STOP_TIMEOUT) {
			sprintf(search_state, "%s at %d@%d%%", search_state_array[play->search->stop], play->search->depth, selectivity_table[play->search->selectivity].percent);
		} else {
			sprintf(search_state, "%s", search_state_array[play->search->stop]);
		}
		ggs_client_send(ui->ggs, "tell .%s -----------------------------------------"
			"\\%s plays %s in game %s using %d thread%s"
			"\\score %s %+02d at %d@%d%% ; PV: %s ;"
			"\\nodes: %s ; time: search = %.1fs, move = %.1fs; speed: %s."
			"\\search %s\n",
			ui->ggs->me,
			ui->ggs->me, move, ui->ggs->board->id, search_count_tasks(play->search), search_count_tasks(play->search) > 1 ? "s ;" : " ;",
			bound, result->score,
			result->depth, selectivity_table[result->selectivity].percent,
			line_to_string(result->pv, 8, " ", line),
			format_scientific(result->n_nodes, "N", s_nodes), 0.001 * result->time, 0.001 * real_time, format_scientific(result->n_nodes / (0.001 * result->time+ 0.001), "N/s", s_speed),
			search_state
		);
	}
}

/**
 * @brief ui_ggs_join
 *
 * Join a new game.
 * This is a new game from Edax point of view.
 * This may be a saved game from GGS side.
 *
 * @param ui User Interface.
 */
static void ui_ggs_join(UI *ui) {
	char buffer[256];
	char s_move[4];
	Play *play;
	int edax_turn, i;
	
	printf("[received GGS_BOARD_JOIN]\n");

	// set correct played game
	if (strcmp(ui->ggs->board->player[0].name, ui->ggs->me) == 0) {
		play = ui->play;
		edax_turn = BLACK;
	} else if (strcmp(ui->ggs->board->player[1].name, ui->ggs->me) == 0) {
		play = ui->play + 1;
		edax_turn = WHITE;
	} else {
		warn("Edax is not concerned by this game\n");
		return ;
	}

	// non synchro games => play a single match
	if (!ui->ggs->board->match_type->is_synchro) play = ui->play;

	// set board
	sprintf(buffer, "%s %c", ui->ggs->board->board_init, ui->ggs->board->turn_init);
	play_set_board(play, buffer);

	for (i = 0; i < ui->ggs->board->move_list_n; i++) {
		if (!play_move(play, ui->ggs->board->move_list[i])) {
			error("cannot play GGS move %s ?", move_to_string(ui->ggs->board->move_list[i], play->player, s_move));
			break;
		}
	}
	printf("[%s's turn in game %s]\n", ui->ggs->board->player[play->player].name, ui->ggs->board->id);
	board_print(play->board, play->player, stdout);

	ui->is_same_play = (ui->ggs->board->move_list_n == 0 || board_equal(ui->play[0].board, ui->play[1].board) || !ui->ggs->board->match_type->is_synchro);
	if (ui->is_same_play) printf("[Playing same game]\n");

	// set time & start thinking
	if (play->player == edax_turn) {
		printf("<My turn>\n");
		ggs_client_send(ui->ggs, "tell .%s =====================================\n", ui->ggs->me);
		ui_ggs_play(ui, edax_turn);
		ui_ggs_ponder(ui, edax_turn);
	} else {
		printf("[Waiting opponent move]\n");
//		ui_ggs_ponder(ui, edax_turn);
	}
}

/**
 * @brief ui_ggs_update
 *
 * Update a game.
 *
 * @param ui User Interface.
 */
static void ui_ggs_update(UI *ui) {
	char buffer[256], s_move[4];
	Play *play;
	int edax_turn, turn;
	Board board[1];
	
	printf("[received GGS_BOARD_UPDATE]\n");

	// set correct played game
	if (strcmp(ui->ggs->board->player[0].name, ui->ggs->me) == 0) {
		play = ui->play;
		edax_turn = BLACK;
	} else if (strcmp(ui->ggs->board->player[1].name, ui->ggs->me) == 0) {
		play = ui->play + 1;
		edax_turn = WHITE;
	} else {
		return ;
	}

	if (!ui->ggs->board->match_type->is_synchro) play = ui->play;
		
	// set board as an edax's board
	sprintf(buffer, "%s %c", ui->ggs->board->board, ui->ggs->board->turn);
	turn = board_set(board, buffer);

	// update the board... if possible
	if (!play_move(play, ui->ggs->board->move)) {
		info("<Updating with bad move %s>\n", move_to_string(ui->ggs->board->move, play->player, s_move));
	}

	if (!board_equal(board, play->board)) { // may happens when game diverges
		info("<Resynchronize boards: diverging games>\n");
		*play->board = *board; play->player = turn;
	}

	if (turn != play->player) { // should never happen: TODO fatal error?
		printf("[WARNING: updating player's turn]\n");
		play->player = turn;
	}

	printf("[%s's turn in game %s]\n", ui->ggs->board->player[play->player].name, ui->ggs->board->id);

	// playing same game... ?
	ui->is_same_play = (!ui->ggs->board->match_type->is_synchro || board_equal(ui->play[0].board, ui->play[1].board));
	if (ui->is_same_play) printf("<Playing same game...>\n");

	// set time & start thinking
	if (play->player == edax_turn) {
		printf("<My turn>\n");
		ui_ggs_play(ui, edax_turn);
	} else {
		printf("<Opponent turn>\n");
		ui_ggs_ponder(ui, edax_turn);
	}
}

/**
 * @brief ui_init_ggs
 *
 * Init GGS interface.
 *
 * @param ui User Interface.
 */
void ui_init_ggs(UI *ui) {
	const int n_task = options.n_task;

	ui->ggs = (GGSClient*) malloc (sizeof (GGSClient));
	if (ui->ggs == NULL) fatal_error("ui_init_ggs: cannot allocate the GGS client\n");

	play_init(ui->play, ui->book);
	ui->book->search = ui->play->search;
	book_load(ui->book, options.book_file);

	ui->play[0].search->id = 1;
	options.n_task = 1;
	play_init(ui->play + 1, ui->book);
	ui->play[1].search->id = 2;
	options.n_task = n_task;
	log_open(ggs_log, options.ggs_log_file);

	ui_login(ui);
}

/**
 * @brief ui_loop_ggs
 *
 * GGS main loop. Here the input from both the user and
 * GGS server is interpreted.
 *
 * @param ui User Interface.
 */
void ui_loop_ggs(UI *ui) {
	char *cmd = NULL, *param = NULL;
	Text text[1];
	GGSClient *client = ui->ggs;

	ui->mode = 3;

	text_init(text);
	for (;;) {
		relax(10);

		/* look for a user event */
		if (ui_event_peek(ui, &cmd, &param)) {
			/* stop the search */
			if (strcmp(cmd, "stop") == 0) { 
				if (ui->play[0].state == IS_THINKING) play_stop(ui->play);
				else if (ui->play[1].state == IS_THINKING) play_stop(ui->play + 1);

			/* repeat a cmd <n> times */			
			} else if (strcmp(cmd, "loop") == 0) { 
				free(client->loop->cmd);
				errno = 0;
				client->loop->cmd = string_duplicate(parse_int(param, &client->loop->i));
				if (errno) client->loop->i = 100;
				if (client->loop->i > 0) {
					info("<loop %d>\n", client->loop->i);
					--client->loop->i;
					ggs_client_send(client, "%s\n", client->loop->cmd);
				}

			/* exit from ggs */
			} else if (strcmp(cmd, "quit") == 0 || strcmp(cmd, "q") == 0) { 
				ggs_client_send(client, "tell .%s Bye bye!\n", client->me);
				ggs_client_send(client, "quit\n");
				free(cmd); free(param);
				return;

			/* send the command to ggs */
			} else {
				ggs_client_send(client, "%s %s\n", cmd, param);
			}
		}
		
		/* stay on line... */
		ggs_client_refresh(client);

		/* look for a ggs event */
		if (!ggs_event_peek(&client->event, text)) {
			continue;
		}

		text_print(text, stdout);

		/* login */
		if (ggs_login(text)) {
			ggs_client_send(client, "%s\n", options.ggs_login);

		/* password */
		} else if (ggs_password(text)) {
			ggs_client_send(client, "%s\n", options.ggs_password);

			ggs_client_send(client, "vt100 -\n");
			ggs_client_send(client, "bell -t -tc -tg -n -nc -ng -ni -nn\n");
			ggs_client_send(client, "verbose -news -faq -help -ack\n");
			ggs_client_send(client, "chann %%\n");
			ggs_client_send(client, "chann + .chat\n");
			ggs_client_send(client, "chann + .%s\n", client->me);
			ggs_client_send(client, "tell .%s Hello!\n", client->me);

		/* os on */
		} else if (ggs_os_on(text)) {
			printf("[received GGS_OS_ON]\n");
			ggs_client_send(client, "tell /os trust +\n" );
			ggs_client_send(client, "tell /os rated +\n" );
			ggs_client_send(client, "tell /os request +\n" );
			ggs_client_send(client, "tell /os client -\n" );
			ggs_client_send(client, "tell /os open %d\n", options.ggs_open);
			ggs_client_send(client, "mso\n" );
	
		/* os off */
		} else if (ggs_os_off(text)) {
			printf("[received GGS_OS_OFF]\n");

		/* match on */
		} else if (ggs_match_on(client->match_on, text)) {
			if (ggs_has_player(client->match_on->player, client->me)) {
				printf("[received GGS_MATCH_ON]\n");
				client->is_playing = true;
				ggs_client_send(client, "tell /os open 0\n" );
			} else {
				printf("[received GGS_WATCH_ON]\n");
			}

		/* match off */
		} else if (ggs_match_off(client->match_off, text)) {
			if (ggs_has_player(client->match_off->player, client->me)) {
				printf("[received GGS_MATCH_OFF]\n");

				if (!client->match_on->match_type->is_rand) {
					if (client->match_on->match_type->is_synchro) {
						printf("[store match #1]\n");
						play_store(ui->play);
						printf("[store match #2]\n");
						play_store(ui->play + 1);
					} else {
						printf("[store match]\n");
						play_store(ui->play);
					}
					if (ui->book->need_saving) {
						book_save(ui->book, options.book_file);
						ui->book->need_saving = false;
					}
				}

				client->is_playing = false;
				ggs_client_send(client, "tell /os open %d\n", options.ggs_open);
				if (client->loop->i > 0) {
					info("<loop %d>\n", client->loop->i);
					--client->loop->i;
					client->loop->delay = 10000 + real_clock(); // wait 10 sec.
				}
			} else {
				printf("[received GGS_WATCH_OFF]\n");
			}

		/* board join/update */
		} else if (ggs_board(client->board,  text)) {
			if (ggs_has_player(client->board->player, client->me)) {
				if (client->board->is_join) ui_ggs_join(ui);
				else ui_ggs_update(ui);
			} else {
				printf("[received GGS_WATCH_BOARD]\n");
			}

		/* request */
		} else if (ggs_request(client->request, text)) {
			printf("[received GGS_REQUEST]\n");

		/* admin on */
		} else if (ggs_admin(client->admin, text)) {
			printf("[received GGS_ADMIN_CMD]\n");
			ggs_client_send(client, client->admin->command);
			ggs_client_send(client, "\ntell %s command processed\n", client->admin->name);

		/* To request Saio a game later */
		} else if (ggs_saio_delay(text, &client->once->delay)) {
			printf("[received GGS_SAIO_DELAY]\n");
			free(client->once->cmd); client->once->cmd = NULL;
			if (cmd != NULL && param != NULL) {
				if (strcmp(cmd, "loop") == 0) {
					client->once->cmd = string_duplicate(client->loop->cmd);
				} else {
					client->once->cmd = (char*) malloc(strlen(cmd) + strlen(param) + 3);
					sprintf(client->once->cmd, "%s %s\n", cmd, param);
				}	
				printf("[received GGS_SAIO_DELAY, retry request in %.1f s]\n", 0.001 * (client->once->delay - real_clock()));
			} else {
				client->once->delay = 0;
			}

		/* READY */
		} else if (ggs_ready(text)) {

		/* ALERT */
		} else if (ggs_alert(text)) {
			printf("[received ALERT]\n");

		/* Other messages */
		} else {
		}
		text_free(text);
	}
}

/**
 * @brief ui_free_ggs
 *
 * Free GGS.
 *
 * @param ui User Interface.
 */
void ui_free_ggs(UI *ui) {
	play_free(ui->play);
	play_free(ui->play + 1);
	if (ui->book->need_saving) book_save(ui->book, options.book_file);
	book_free(ui->book);
	ggs_client_free(ui->ggs);
	free(ui->ggs->loop->cmd);
	free(ui->ggs->once->cmd);
	free(ui->ggs);
	log_close(ggs_log);
}

