/**
 * @file perft.h
 *
 * @brief Move generator test header file.
 *
 * @date 1998 - 2024
 * @author Richard Delorme
 * @author Toshihiko Okuhara
 * @version 4.6
 */

#ifndef EDAX_PERFT_H
#define EDAX_PERFT_H

#include <stdbool.h>
#include <stdint.h>

struct Board;
struct Line;
void count_games(const struct Board*, const int);
void quick_count_games(const struct Board*, const int, const int);
void count_positions(const struct Board*, const int, const int);
void count_shapes(const struct Board*, const int, const int);
void estimate_games(const struct Board*, const int64_t);
void seek_highest_mobility(const struct Board*, const uint64_t);
bool seek_position(const struct Board*, const struct Board*, struct Line*);

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
