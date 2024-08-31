/**
 * @file const.h
 *
 * Constants as macros, enums, or global consts.
 *
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
 * @date 1998 - 2024
=======
 * @date 1998 - 2020
>>>>>>> 9ad160e (4.4.7 AVX/shuffle optimization in endgame_sse.c)
=======
 * @date 1998 - 2021
>>>>>>> 34a2291 (4.5.0: Use CRC32c for board hash)
=======
 * @date 1998 - 2023
>>>>>>> d63619f (Change NodeType to char; next node_type TLU to trinary Op)
=======
 * @date 1998 - 2024
>>>>>>> a09308f (Renew version string and copyright year)
 * @author Richard Delorme
 * @version 4.5
 */

#ifndef EDAX_CONST_H
#define EDAX_CONST_H

/** maximal number of threads */
#define MAX_THREADS 64

/** maximal number of moves */
#define MAX_MOVE 33	// https://eukaryote.hateblo.jp/entry/2023/05/23/145945

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
enum {
	PV_NODE,
	CUT_NODE,
	ALL_NODE
};
<<<<<<< HEAD
<<<<<<< HEAD
typedef	unsigned char	NodeType;
=======
typedef	char	NodeType;
>>>>>>> d63619f (Change NodeType to char; next node_type TLU to trinary Op)
=======
typedef	unsigned char	NodeType;
>>>>>>> 2ea1e4f (Change NodeType to unsigned char to fix gcc warning)

#define VERSION 4
<<<<<<< HEAD
<<<<<<< HEAD
#define RELEASE 5
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
#define VERSION_STRING "4.5.3"
#define EDAX_NAME "Edax 4.5.3"
=======
#define RELEASE 4
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
#define VERSION_STRING "4.4.5"
#define EDAX_NAME "Edax 4.4.5"
>>>>>>> 5124720 (-eval-file options added as documented; minor fix on console output)
=======
#define VERSION_STRING "4.4.6"
#define EDAX_NAME "Edax 4.4.6"
>>>>>>> cd90dbb (Enable 32bit AVX build; optimize loop in board print; set version to 4.4.6)
=======
#define VERSION_STRING "4.4.7"
#define EDAX_NAME "Edax 4.4.7"
>>>>>>> 9ad160e (4.4.7 AVX/shuffle optimization in endgame_sse.c)
=======
#define VERSION_STRING "4.4.8"
#define EDAX_NAME "Edax 4.4.8"
>>>>>>> 569c1f8 (More neon optimizations; split bit_intrinsics.h from bit.h)
=======
=======
#define RELEASE 5
>>>>>>> fdb3c8a (SWAR vector eval update; more restore in search_restore_midgame)
#define VERSION_STRING "4.5.0"
#define EDAX_NAME "Edax 4.5.0"
>>>>>>> 34a2291 (4.5.0: Use CRC32c for board hash)
=======
#define VERSION_STRING "4.5.1"
#define EDAX_NAME "Edax 4.5.1"
>>>>>>> ff1c5db (skip hash access if n_moves <= 1 in NWS_endgame)
=======
#define VERSION_STRING "4.5.2"
#define EDAX_NAME "Edax 4.5.2"
>>>>>>> a9633d5 (Initial 4.5.2; some reformats)
=======
#define VERSION_STRING "4.5.1"
#define EDAX_NAME "Edax 4.5.1"
>>>>>>> 4087529 (Revise board0 usage; fix unused flips)
=======
#define VERSION_STRING "4.5.2"
#define EDAX_NAME "Edax 4.5.2"
>>>>>>> a09308f (Renew version string and copyright year)
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
	UI_XBOARD
};

#endif


