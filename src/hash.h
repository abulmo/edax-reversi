/**
 * @file hash.h
 *
 * Hash table's header.
 *
 * @date 1998 - 2017
 * @author Richard Delorme
 * @version 4.4
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
	unsigned char depth;      /*!< depth */
	unsigned char selectivity;/*!< selectivity */
	unsigned char cost;       /*!< search cost */
	unsigned char date;       /*!< dating technique */
	signed char lower;        /*!< lower bound of the position score */
	signed char upper;        /*!< upper bound of the position score */
	unsigned char move[2];    /*!< best moves */
} HashData;

/** Hash  : item stored in the hash table*/
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
	Hash *hash;  				  /*!< hash table */
	HashLock *lock;               /*!< table with locks */
	unsigned long long hash_mask; /*!< a bit mask for hash entries */
	unsigned int lock_mask;       /*!< a bit mask for lock entries */
	int n_hash;                   /*!< hash table size */
	int n_lock;                   /*!< number of locks */
	unsigned char date;           /*!< date */
} HashTable;

/* declaration */
void hash_code_init(void);
void hash_move_init(void);
void hash_init(HashTable*, const unsigned long long);
void hash_cleanup(HashTable*);
void hash_clear(HashTable*);
void hash_free(HashTable*);
void hash_store(HashTable*, const Board*, const unsigned long long, const int, const int, const int, const int, const int, const int, const int);
void hash_force(HashTable*, const Board*, const unsigned long long, const int, const int, const int, const int, const int, const int, const int);
bool hash_get(HashTable*, const Board*, const unsigned long long, HashData *);
void hash_copy(const HashTable*, HashTable*);
void hash_print(const HashData*, FILE*);
void hash_feed(HashTable*, const Board*, const unsigned long long, const int, const int, const int, const int, const int);
void hash_exclude_move(HashTable*, const Board*, const unsigned long long, const int);
extern unsigned int writeable_level(HashData *data);

extern const HashData HASH_DATA_INIT;
extern unsigned long long hash_rank[16][256];
extern unsigned long long hash_move[64][60];

#endif

