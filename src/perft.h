/**
 * @file perft.h
 *
 * @brief Move generator test header file.
 *
 * @date 1998 - 2026
 * @author Richard Delorme
 * @author Toshihiko Okuhara
 * @version 4.7
 */

#ifndef EDAX_PERFT_H
#define EDAX_PERFT_H

#include "board.h"
#include <stdbool.h>
#include <stdint.h>

struct Board;
struct Line;
void divide(const Board*, const int depth, const bool count_pass);
void count_games(const Board*, const int, const bool);
void quick_count_games(const Board*, const int, const bool, const int);
void count_positions(const Board*, const int, const bool, const int);
void count_shapes(const Board*, const int, const bool, const int);
void estimate_games(const Board*, const int64_t, const bool, const int);
void seek_highest_mobility(const Board*, const uint64_t);
bool seek_position(const Board*, const Board*, struct Line*);

/** HashTable of positions */
typedef struct PositionHash {
	struct PosArray *array;
	int size;
	int mask;
} PositionHash;
void positionhash_init(PositionHash*, int);
void positionhash_delete(PositionHash*);
bool positionhash_append(PositionHash*, const struct Board*);

#endif /* EDAX_PERFT_H */
