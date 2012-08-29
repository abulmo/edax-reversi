/**
 * @file play.h
 *
 * @brief Edax play control - header file.
 *
 * @date 1998 - 2012
 * @author Richard Delorme
 * @version 4.3
 */


#ifndef EDAX_PLAY_H
#define EDAX_PLAY_H

#include "board.h"
#include "book.h"
#include "search.h"
#include "move.h"
#include "util.h"

/** error message max length */
#define PLAY_MESSAGE_MAX_LENGTH 4096

/** play structure */
typedef struct Play {
	Board board[1];            /**< current board. */
	Board initial_board[1];    /**< initial board. */
	Search search[1];          /**< search. */
	Result result[1];          /**< search result. */
	Book *book;                /**< opening book */
	int type;                  /**< ui type */
	int player;                /**< current player's color. */
	int initial_player;        /**< initial player's color. */
	Move game[80];             /**< game (move sequence). */
	int i_game;                /**< current move index. */
	int n_game;                /**< last move index. */
	volatile PlayState state;  /**< current state */
	int level;                 /**< search level */
	long long clock;           /**< internal clock */
	struct {
		long long spent;       /**< time spent */
		long long left;        /**< time left */
		long long extra;       /**< extra time left */
	} time[2];                 /**< time of each player */
	struct {
		Board real[80];        /**< forced positions */
		Board unique[80];      /**< unique symetry of the forced positions */
		Move move[80];         /**< forced move sequence */
		int n_move;            /**< number of forced move */
		int i_move;            /**< current forced move */
	} force;                   /**< forced line */
	struct {
		Thread thread;         /**< thread. */
		Lock lock;             /**< lock. */
		Board board[1];        /**< pondered position */
		bool launched;         /**< launched thread */
		bool verbose;          /**< verbose pondering */
	} ponder[1];               /**< pondering thread */
	char error_message[PLAY_MESSAGE_MAX_LENGTH]; /**< error message */
} Play;

/* functions */
bool play_is_game_over(Play*);
bool play_must_pass(Play *play);
void play_init(Play*, Book*);
void play_free(Play*);
void play_new(Play*);
void play_ggs_init(Play*, const char*);
bool play_load(Play*, const char*);
void play_save(Play*, const char*);
void play_auto_save(Play*);
void play_go(Play*, const bool);
void play_hint(Play*, int);
void play_stop(Play*);
void* play_ponder_run(void*);
void play_ponder(Play*);
void* play_ponder_loop(void*);
void play_stop_pondering(Play*);
void play_update(Play*, Move*);
void play_pass(Play*);
void play_undo(Play*);
void play_redo(Play*);
void play_set_board(Play*, const char*);
void play_set_board_from_FEN(Play*, const char*);
void play_game(Play*, const char*);
bool play_move(Play*, int);
bool play_user_move(Play*, const char*);
Move *play_get_last_move(Play*);
void play_analyze(Play*, int);
void play_book_analyze(Play*, int);
void play_learn(Play*);
void play_store(Play*);
void play_adjust_time(Play*, const int, const int);
void play_print(Play*, FILE*);
void play_force_init(Play*, const char*);
void play_force_update(Play*);
void play_force_restore(Play*);
bool play_force_go(Play*, Move*);
void play_symetry(Play*, const int);
const char* play_show_opening_name(Play*, const char *(*opening_get_name)(const Board*));

#endif

