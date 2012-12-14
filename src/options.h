/**
 * @file options.h
 *
 * Options header.
 *
 * @date 1998 - 2012
 * @author Richard Delorme
 * @version 4.3
 */

#ifndef EDAX_OPTIONS_H
#define EDAX_OPTIONS_H

#include <stdio.h>
#include <stdbool.h>

typedef enum {
	EDAX_FIXED_LEVEL,
	EDAX_TIME_PER_GAME,
	EDAX_TIME_PER_MOVE
} PlayType;

/** options to control various heuristics */
typedef struct {
	int hash_table_size;                  /**< size (in number of bits) of the hash table */

	int inc_sort_depth[3];                /**< increment sorting depth */

	int n_task;                           /**< search in parallel, using n_tasks */
	bool cpu_affinity;                    /**< set one cpu/thread to diminish context change */

	int verbosity;                        /**< search display */
 	int noise;                            /**< search display min depth */
	int width;                            /**< line width */
	bool echo;                            /**< repeat user input */
	bool info;                            /**< info display */
	bool debug_cassio;                    /**< display debug info in cassio's "fenetre de rapport"*/
	bool transgress_cassio;               /**< adapt Cassio requests to search & solve faster */

	int level;                            /**< level */
	long long time;                       /**< time in sec. */
	PlayType play_type;                   /**< game|move-time switch */
	bool can_ponder;                      /**< pondering on/off */
	int depth;                            /**< depth (only for testing) */
	int selectivity;                      /**< selectivity (only for testing) */

	int mode;                             /**< mode play (human/edax, etc.) */

	double speed;                         /**< edax speed in N/S (for a more accurate time management) */
	double nps;                           /**< edax assumed speed (for nps based timing */                           

	int alpha;                            /**< alpha bound */
	int beta;                             /**< beta bound */

	bool all_best;                        /**< search for all best moves when solving problem */

	char *eval_file;                      /**< evaluation file */

	char *book_file;                      /**< opening book filename */
	bool book_allowed;                    /**< switch to use or not the opening book*/
	int book_randomness;                  /**< book randomness */

	char *ggs_host;                       /**< ggs host (ip or host name) */
	char *ggs_login;                      /**< ggs login */
	char *ggs_password;                   /**< ggs password */
	char *ggs_port;                       /**< ggs port */
	bool ggs_open;                        /**< ggs open number (set it false for tournaments) */

	double probcut_d;

	bool pv_debug;                        /**< debug PV */
	bool pv_check;                        /**< check PV correctness */
	bool pv_guess;                        /**< guess PV missing moves */

	char *game_file;                      /**< game file */

	char *search_log_file;                /**< log file (for search) */
	char *ui_log_file;                    /**< log file (for user interface) */
	char *ggs_log_file;                   /**< log file (for ggs) */

	char *name;                           /**< program name */

	bool auto_start;                      /**< start a new game after a game is over */
	bool auto_store;                      /**< store a game in a book after each game */
	bool auto_swap;                       /**< change computer's side after each game */
	bool auto_quit;                       /**< quit when game is over */
	int repeat;                           /**< repeat 'n' games (before quitting)*/

	// TODO: add more options?
} Options;

extern Options options;

void options_usage(void);
int options_read(const char*, const char*);
void options_parse(const char*);
void options_bound(void);
void options_free(void);
void options_dump(FILE *f);

void version(void);
void usage(void);

#endif

