/**
 * @file const.h
 *
 * Constants as macros, enums, or global consts.
 *
 * @date 1998 - 2017
 * @author Richard Delorme
 * @version 4.4
 */

#ifndef EDAX_CONST_H
#define EDAX_CONST_H

/** maximal number of threads */
#define MAX_THREADS 64

/** maximal number of moves */
#define MAX_MOVE 32

/** size of the board */
#define BOARD_SIZE 64

/** size of the game including passing moves. We use here an arbitrary big
 enough value */
#define GAME_SIZE 80

/** constants for square coordinates */
enum {
	A1, B1, C1, D1, E1, F1, G1, H1,
	A2, B2, C2, D2, E2, F2, G2, H2,
	A3, B3, C3, D3, E3, F3, G3, H3,
	A4, B4, C4, D4, E4, F4, G4, H4,
	A5, B5, C5, D5, E5, F5, G5, H5,
	A6, B6, C6, D6, E6, F6, G6, H6,
	A7, B7, C7, D7, E7, F7, G7, H7,
	A8, B8, C8, D8, E8, F8, G8, H8,
	PASS, NOMOVE
};

/** constants for colors */
enum {
	BLACK = 0,
	WHITE,
	EMPTY,
	OFF_SIDE
};

extern const unsigned long long X_TO_BIT[];
extern const unsigned long long NEIGHBOUR[];

/** infinite score: a huge value unreachable as a score and fitting in a char */
#define SCORE_INF 127

/** minimal score */
#define SCORE_MIN -64

/** maximal score */
#define SCORE_MAX 64

/** maximal time (1 millenium) */
#define TIME_MAX 31557600000000LL

/** 1 hour */
#define HOUR 3600000LL

/** observers */
#define MAX_OBSERVER 2

/** constants for search interruption */
typedef enum Stop {
	RUNNING = 0,
	STOP_PARALLEL_SEARCH,
	STOP_PONDERING,
	STOP_TIMEOUT,
	STOP_ON_DEMAND,
	STOP_END
} Stop;

/** node type */
typedef enum {
	PV_NODE,
	CUT_NODE,
	ALL_NODE
} NodeType;

#define VERSION 4
#define RELEASE 4
#define VERSION_STRING "4.4"
#define EDAX_NAME "Edax 4.4"
#define BOOK 0x424f4f4b
#define EDAX 0x45444158
#define EVAL 0x4556414c
#define XADE 0x58414445
#define LAVE 0x4c415645

/**
 * Edax state.
 */
typedef enum PlayState {
	IS_WAITING,
	IS_PONDERING,
	IS_ANALYZING,
	IS_THINKING
} PlayState;

/** Type of User Interface */
enum {
	UI_NONE = 0,
	UI_CASSIO,
	UI_EDAX,
	UI_GGS,
	UI_GTP,
	UI_NBOARD,
	UI_XBOARD,
	UI_LIBEDAX // add for libedax by lavox. 2018/1/16
};

#endif


