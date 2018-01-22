/**
 * @file all.c
 *
 * @brief Gather all other files to facilitate compiler inter-procedural optimization.
 *
 * @date 1998 - 2017
 * @author Richard Delorme
 * @version 4.4
 */

/* miscellaneous utilities */
#include "options.c"
#include "util.c"
#include "stats.c"
#include "bit.c"

/* move generation */
#include "board.c"
#include "move.c"

/* eval & search */
#include "eval.c"
#include "hash.c"
#include "ybwc.c"
#include "search.c"
#include "endgame.c"
#include "midgame.c"
#include "root.c"

/* miscellaneous tests */
#include "perft.c"
#include "obftest.c"
#include "histogram.c"
#include "bench.c"

/* opening book & game database */
#include "book.c"
#include "game.c"
#include "base.c"
#include "opening.c"

/* game play with various protocols */
#include "play.c"
#include "event.c"
// add for libedax by lavox. 2018/1/16
#ifdef LIB_BUILD
	#include "libedax.c"
#endif
#include "ui.c"
#include "edax.c"
#include "cassio.c"
#include "ggs.c"
#include "gtp.c"
#include "nboard.c"
#include "xboard.c"

/* main */
#include "main.c"

