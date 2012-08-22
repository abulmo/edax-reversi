/**
 * @file perft.h
 *
 * @brief Move generator test header file.
 *
 * @date 1998 - 2012
 * @author Richard Delorme
 * @version 4.3
 */

#ifndef EDAX_PERFT_H
#define EDAX_PERFT_H

struct Board;
void count_games(const struct Board*, const int);
void quick_count_games(const struct Board*, const int, const int);
void count_positions(const struct Board*, const int, const int);
void count_shapes(const struct Board*, const int, const int);
void estimate_games(const struct Board*, const long long);
void seek_highest_mobility(const struct Board*, const unsigned long long);

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
