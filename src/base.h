/**
 * @file base.h
 *
 * Header file for game base management
 *
 * @date 1998 - 2012
 * @author Richard Delorme
 * @version 4.3
 */

#ifndef EDAX_BASE_H
#define EDAX_BASE_H

#include "game.h"
#include <stdbool.h>

/* structures */
struct Search;

/**
 * struct WthorHeader
 * @brief Header for wthor files
*/
typedef struct WthorHeader {
	char century;                /**< Century. */
	char year;                   /**< Year. */
	char month;                  /**< month */
	char day;                    /**< day */
	char board_size;             /**< board size (8 or 10) */
	char game_type;              /**< game type (games or solitaires) */
	char depth;                  /**< depth (of theoric score) */
	char reserved;               /**< reserved */
	unsigned short n;            /**< */
	unsigned short game_year;    /**< */
	int n_games;                 /**< */
} WthorHeader;

typedef struct WthorBase {
	WthorHeader header[1];     /** Header */
	char (*tournament)[26];    /** tournaments */
	int n_tournaments;         /** tournament number */
	char (*player)[20];        /** players */
	int n_players;             /** tournament players */
	WthorGame *game;           /** games */
	int n_games;               /** n_games */
} WthorBase;

typedef struct Base {
	Game *game;
	int n_games;
	int size;
} Base;

/* function declarations */
void wthor_init(WthorBase*);
bool wthor_load(WthorBase*, const char*);
bool wthor_save(WthorBase*, const char*);
void wthor_test(const char*, struct Search*);
void wthor_eval(const char*, struct Search*, unsigned long long histogram[129][65]);
void wthor_edaxify(const char*);

#define foreach_wthorgame(wgame, wbase) \
	for ((wgame) = (wbase)->game ; (wgame) < (wbase)->game + (wbase)->header->n_games; ++(wgame))

void base_init(Base*);
void base_free(Base*);
bool base_load(Base*, const char*);
void base_save(const Base*, const char*);
void base_append(Base*, const Game*);
void base_to_problem(Base*, const int, const char*);
void base_to_FEN(Base*, const int, const char*);
void base_analyze(Base*, struct Search*, const int, const bool);
void base_complete(Base*, struct Search*);
void base_unique(Base*);
void base_compare(const char*, const char*);

#endif /* EDAX_BASE_H */

