/**
 * @file base.h
 *
 * Header file for game base management.
 *
 * @date 1998 - 2025
 * @author Richard Delorme
 * @version 4.7
 */

#ifndef EDAX_BASE_H
#define EDAX_BASE_H

#include "game.h"
#include <stdint.h>
#include <stdbool.h>

/* structures */
struct Search;

/**
 * struct WthorHeader
 * @brief Header for wthor files
*/
typedef struct WthorHeader {
	uint8_t century;           /**< Century. */
	uint8_t year;              /**< Year. */
	uint8_t month;             /**< month */
	uint8_t day;               /**< day */
	uint8_t board_size;        /**< board size (8 or 10) */
	uint8_t game_type;         /**< game type (games or solitaires) */
	uint8_t depth;             /**< depth (of theoric score) */
	uint8_t reserved;          /**< reserved */
	uint16_t n;                /**< number of player or tournaments */
	uint16_t game_year;        /**< year of the game */
	int32_t n_games;          /**< number of games*/
} WthorHeader;

/**
 * struct WthorBase
 * @brief A set of games using the Wthor format.
*/
typedef struct WthorBase {
	WthorHeader header;        /** Header */
	char (*tournament)[26];    /** tournaments */
	int32_t n_tournaments;     /** tournament number */
	char (*player)[20];        /** players */
	int32_t n_players;         /** tournament players */
	WthorGame *game;           /** games */
	int32_t n_games;           /** n_games */
} WthorBase;

/**
 * struct Base
 * @brief a set of games.
 */
typedef struct Base {
	Game *game;
	int32_t n_games;
	int32_t size;
} Base;

/* function declarations */
void wthor_init(WthorBase*);
bool wthor_load(WthorBase*, const char*);
bool wthor_save(WthorBase*, const char*);
void wthor_test(const char*, struct Search*);
void wthor_eval(const char*, struct Search*, uint64_t histogram[129][65]);
void wthor_edaxify(const char*);
/**
 * @brief macro to loop over all games of a wthorBase.
 * @param wgame a Wthor game.
 * @param wbase a WthorBase.
 */
#define foreach_wthorgame(wgame, wbase) \
	for ((wgame) = (wbase).game ; (wgame) < (wbase).game + (wbase).header.n_games; ++(wgame))

void base_init(Base*);
void base_free(Base*);
bool base_load(Base*, const char*);
void base_save(const Base*, const char*, const bool);
void base_append(Base*, const Game*);
void base_to_problem(Base*, const int, const char*);
void base_to_FEN(Base*, const int, const char*);
void base_analyze(Base*, struct Search*, const int, const bool);
void base_complete(Base*, struct Search*, const int);
void base_unique(Base*);
void base_compare(const char*, const char*);

#endif /* EDAX_BASE_H */
