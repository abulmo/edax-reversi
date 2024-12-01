/**
 * @file all.c
 *
 * @brief Gather all other files to facilitate compiler inter-procedural optimization.
 *
 * @date 1998 - 2024
 * @author Richard Delorme
 * @version 4.6
 */

/* miscellaneous utilities */
#include "options.c"
#include "util.c"
#include "stats.c"
#include "bit.c"
#include "crc32c.c"

/* move generation */
#include "flip.c"
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

/* opening book & game database */
#include "book.c"
#include "game.c"
#include "base.c"
#include "opening.c"

/* game play with various protocols */
#include "play.c"
#include "event.c"
#include "ui.c"
#include "edax.c"
#include "cassio.c"
#include "ggs.c"
#include "gtp.c"
#include "nboard.c"
#include "xboard.c"

/* main */
#include "main.c"

