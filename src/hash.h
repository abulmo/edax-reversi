/**
 * @file hash.h
 *
 * Hash table's header.
 *
 * @date 1998 - 2024
 * @author Richard Delorme
 * @author Toshihiko Okuhara
 * @version 4.6
 */

#ifndef EDAX_HASH_H
#define EDAX_HASH_H

#include "board.h"
#include "stats.h"
#include "util.h"

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

/** HashDraft: search setting to discriminate between hash entries */
typedef union {
	struct {
	#ifdef __BIG_ENDIAN__
		uint8_t date;       /*!< entry age */
		uint8_t cost;       /*!< search cost */
		uint8_t selectivity;/*!< selectivity */
		uint8_t depth;      /*!< depth */
	#else
		uint8_t depth;      /*!< depth */
		uint8_t selectivity;/*!< selectivity */
		uint8_t cost;       /*!< search cost */
		uint8_t date;       /*!< entry age */
	#endif
	} u1;
	struct {
	#ifdef __BIG_ENDIAN__
		uint16_t cost_date;         /*!< cost & date gathered */
		uint16_t depth_selectivity; /*!< depth & selectivity gathered */
	#else
		uint16_t depth_selectivity; /*!< depth & selectivity gathered */
		uint16_t cost_date;         /*!< cost & date gathered */
	#endif
	} u2;
	uint32_t u4; /** whole draft as an unsigned integer */
} HashDraft;


/** HashData : data stored in the hash table */
typedef struct HashData {
	HashDraft draft;
	int8_t lower;        /*!< lower bound of the position score */
	int8_t upper;        /*!< upper bound of the position score */
	int8_t move[2];      /*!< best moves */
} HashData;

/** HashStore : data to store in he hash table */
typedef struct HashStore {
	HashDraft draft;
	int8_t alpha;
	int8_t beta;
	int8_t score;
	int8_t move;
} HashStore;

/** Hash  : item stored in the hash table*/
typedef struct Hash {
	HASH_COLLISIONS(uint64_t key;) //<- collision counter
	Board board;                   //<- the full Othello board
	HashData data;                 //<- stored search results
} Hash;

/** HashTable: position storage */
typedef struct HashTable {
	Hash *hash;                   /*!< hash table */
	SpinLock *spin;               /*!< table with spinlocks */
	uint64_t n_hash;              /*!< hash table size */
	uint64_t hash_mask;           /*!< a bit mask for hash entries */
	uint32_t spin_mask;           /*!< a bit mask for lock entries */
	uint32_t n_spin;              /*!< number of locks */
	HASH_STATS(uint64_t n_try;)   /*!< number of probe attempt */
	HASH_STATS(uint64_t n_found;) /*!< number of succesful probes */
	HASH_STATS(uint64_t n_store;) /*!< number of stores */
	uint8_t date;                 /*!< date */
} HashTable;

/* declaration */
void hash_init(HashTable*, const size_t);
void hash_cleanup(HashTable*);
void hash_clear(HashTable*);
void hash_free(HashTable*);
void hash_prefetch(HashTable*, const uint64_t);
void hash_store(HashTable*, const Board*, const uint64_t, const HashStore*);
void hash_force(HashTable*, const Board*, const uint64_t, const HashStore*);
bool hash_get(HashTable*, const Board*, const uint64_t, HashData*);
void hash_copy(const HashTable*, HashTable*);
void hash_print(const HashData*, FILE*);
void hash_feed(HashTable*, const Board*, const uint64_t, const HashData*);
void hash_exclude_move(HashTable*, const Board*, const uint64_t, const int);

extern const HashData HASH_DATA_INIT;

/**
 * @brief Fill a Draft structure/union
 *
 * @param draft The draft to fill
 * @param depth Depth.
 * @param selectivity Selectivity.
 * @param cost Cost.
 * @param date Date.
 */
static inline void draft_set(HashDraft *draft, const int depth, const int selectivity, const int cost, const int date)
{
	draft->u1.depth = (uint8_t) depth;
	draft->u1.selectivity = (uint8_t) selectivity;
	draft->u1.cost = (uint8_t) cost;
	draft->u1.date = (uint8_t) date;
}

/**
 * @brief Fill a HashStore structure (exect its Draf part).
 *
 * @param store The store structure to fill.
 * @param alpha Alpha bound.
 * @param beta Beta bound.
 * @param score Best score found.
 * @param move Best move found.
 */
static inline void store_set(HashStore *store, const int alpha, const int beta, const int score, const int move)
{
	store->alpha = (int8_t) alpha;
	store->beta = (int8_t) beta;
	store->score = (int8_t) score;
	store->move = (int8_t) move;
}


#endif

