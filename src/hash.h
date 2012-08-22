/**
 * @file hash.h
 *
 * Hash table's header.
 *
 * @date 1998 - 2012
 * @author Richard Delorme
 * @version 4.3
 */

#ifndef EDAX_HASH_C
#define EDAX_HASH_C

#include "settings.h"

#if (USE_HASH_LOCK)
	#include "util.h"
#endif

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
	unsigned long long code;  /*!< hash code */
	HashData data;
} Hash;

#if (USE_HASH_LOCK)
/** HashLock : lock for table entries */
typedef struct HashLock {
	SpinLock spin;
} HashLock;
#endif

/** HashTable: position storage */
typedef struct HashTable {
	Hash *hash;  				  /*!< hash table */
	unsigned long long hash_mask; /*!< a bit mask for hash entries */
	void *memory;                 /*!< allocated memory */
	int size;                     /*!< hash table size */
#if (USE_HASH_LOCK)
	HashLock *lock;               /*!< table with locks */
	unsigned int lock_mask;       /*!< a bit mask for lock entries */
	int n_lock;                   /*!< number of locks */
#endif
	unsigned char date;           /*!< date */
} HashTable;

/* declaration */
void hash_code_init(void);
void hash_move_init(void);
void hash_init(HashTable*, const int);
void hash_cleanup(HashTable*);
void hash_clear(HashTable*);
void hash_free(HashTable*);
void hash_store(HashTable*, const unsigned long long, const int, const int, const int, const int, const int, const int, const int);
void hash_force(HashTable*, const unsigned long long, const int, const int, const int, const int, const int, const int, const int);
bool hash_get(HashTable*, const unsigned long long, HashData *);
void hash_copy(const HashTable*, HashTable*);
void hash_print(const HashData*, FILE*);
void hash_feed(HashTable*, const unsigned long long, const int, const int, const int, const int, const int);
void hash_exclude_move(HashTable*, const unsigned long long, const int);
extern unsigned int writeable_level(HashData *data);

extern const HashData HASH_DATA_INIT;
extern unsigned long long hash_rank[16][256];
extern unsigned long long hash_move[64][60];

#endif

