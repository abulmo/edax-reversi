/**
 * @file hash.h
 *
 * Hash table's header.
 *
 * @date 1998 - 2023
 * @author Richard Delorme
 * @version 4.5
 */

#ifndef EDAX_HASH_H
#define EDAX_HASH_H

#include "board.h"
#include "settings.h"
#include "util.h"
#include "stats.h"

#include <stdbool.h>
#include <stdio.h>

/** HashData : data stored in the hash table */
typedef struct HashData {
	union {
#ifdef __BIG_ENDIAN__
		struct {
			unsigned char date;       /*!< dating technique */
			unsigned char cost;       /*!< search cost */
			unsigned char selectivity;/*!< selectivity */
			unsigned char depth;      /*!< depth */
		} c;
		struct {
			unsigned short date_cost;
			unsigned short selectivity_depth;
		} us;
#else
		struct {
			unsigned char depth;      /*!< depth */
			unsigned char selectivity;/*!< selectivity */
			unsigned char cost;       /*!< search cost */
			unsigned char date;       /*!< dating technique */
		} c;
		struct {
			unsigned short selectivity_depth;
			unsigned short date_cost;
		} us;
#endif
		unsigned int	ui;      /*!< as writable level */
	} wl;
	signed char lower;        /*!< lower bound of the position score */
	signed char upper;        /*!< upper bound of the position score */
	unsigned char move[2];    /*!< best moves */
} HashData;

/** Hash  : item stored in the hash table */
typedef struct Hash {
	HASH_COLLISIONS(unsigned long long key;)
	Board board;
	HashData data;
} Hash;

/** HashLock : lock for table entries */
typedef struct HashLock {
	SpinLock spin;
} HashLock;

/** HashTable: position storage */
typedef struct HashTable {
	void *memory;                 /*!< allocated memory */
	Hash *hash;                   /*!< hash table */
	HashLock *lock;               /*!< table with locks */
	unsigned long long hash_mask; /*!< a bit mask for hash entries */
	unsigned int lock_mask;       /*!< a bit mask for lock entries */
	int n_hash;                   /*!< hash table size */
	int n_lock;                   /*!< number of locks */
	unsigned char date;           /*!< date */
} HashTable;

/** HashStoreData : data to store */
typedef struct HashStoreData {
	HashData data;
	HASH_COLLISIONS(unsigned long long hash_code;)
	int alpha;
	int beta;
	int score;
} HashStoreData;

/* declaration */

void hash_move_init(void);
void hash_init(HashTable*, const unsigned long long);
void hash_cleanup(HashTable*);
void hash_clear(HashTable*);
void hash_free(HashTable*);
void hash_feed(HashTable*, const Board *, const unsigned long long, HashStoreData *);
void hash_store(HashTable*, const Board *, const unsigned long long, HashStoreData *);
void hash_force(HashTable*, const Board *, const unsigned long long, HashStoreData *);
bool hash_get(HashTable*, const Board *, const unsigned long long, HashData *);
bool hash_get_from_board(HashTable*, const Board *, HashData *);
void hash_exclude_move(HashTable*, const Board *, const unsigned long long, const int);
void hash_copy(const HashTable*, HashTable*);
void hash_print(const HashData*, FILE*);
extern unsigned int writeable_level(HashData *data);

extern const HashData HASH_DATA_INIT;

inline void hash_prefetch(HashTable *hashtable, unsigned long long hashcode) {
	Hash *p = hashtable->hash + (hashcode & hashtable->hash_mask);
  #ifdef hasSSE2
	_mm_prefetch((char const *) p, _MM_HINT_T0);
	_mm_prefetch((char const *)(p + HASH_N_WAY - 1), _MM_HINT_T0);
  #elif defined(__ARM_ACLE)
	__pld(p);
	__pld(p + HASH_N_WAY - 1);
  #elif defined(__GNUC__)
	__builtin_prefetch(p);
	__builtin_prefetch(p + HASH_N_WAY - 1);
  #endif
}

#endif
