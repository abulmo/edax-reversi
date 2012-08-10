/**
 * @file hash.c
 *
 * @brief Lock-free transposition table.
 *
 * The hash table is an efficient memory system to remember the previously
 * analysed positions and re-use the collected data when needed.
 * The hash table contains entries of analysed data where the board is uniquely
 * identified through a 64-bit key and the results of the recorded analysis are
 * two score bounds, the level of the analysis and the best move found.
 * The implementation is now a multi-way hashtable. It both tries to keep the
 * deepest records and to always add the latest one.
 * The following implementation may suffer from hash collision: two different
 * positions may share the same hashcode. Fortunately, this is very unprobable
 * with hascode on 64 bits, and, thanks to alphabeta robustness, it propagates
 * even less often to the root.
 * When doing parallel search with a shared hashtable, a lockless implementation
 * [1] detects & eliminates concurrency collisions without much performance
 * impact.
 *
 * [1] http://www.cis.uab.edu/hyatt/hashing.html
 *
 * @date 1998 - 2012
 * @author Richard Delorme
 * @version 4.3
 */

#include "bit.h"
#include "hash.h"
#include "options.h"
#include "stats.h"
#include "search.h"
#include "util.h"
#include "settings.h"

#include <stdlib.h>
#include <stdio.h>
#include <assert.h>

/** hashing global data */
unsigned long long hash_rank[16][256];

/** hashing global data */
unsigned long long hash_move[64][60];

/** HashData init value */
const HashData HASH_DATA_INIT = {0, 0, 0, 0, -SCORE_INF, SCORE_INF, {NOMOVE, NOMOVE}};

/**
 * @brief Initialize global hash code data.
 */
void hash_code_init(void)
{
	int i, j;
	Random r;

	random_seed(&r, 0x5DEECE66Dull);
	for (i = 0; i < 16; ++i)
	for (j = 0; j < 256; ++j) {
		do {
			hash_rank[i][j] = random_get(&r);
		} while (bit_count(hash_rank[i][j]) < 8); 
	}
}


/**
 * @brief Initialize global hash move data.
 */
void hash_move_init(void)
{
	int i, j;
	Random r;

	random_seed(&r, 0x5DEECE66Dull);
	for (i = 0; i < 64; ++i)
	for (j = 0; j < 60; ++j) {
		do {
			hash_move[i][j] = random_get(&r);
		} while (bit_count(hash_move[i][j]) < 8); 
	}
}


/**
 * @brief Transform hash data into a 64 bits key.
 *
 * @param data Hash data.
 * @return a 64 bits key.
 */
static inline unsigned long long hash_key(const HashData *data)
{
	if (USE_TYPE_PUNING) { // fast version using type puning.
		union {
			HashData data;
			unsigned long long key;
		} u;
		u.data = *data;
		return u.key;
	} else { // more portable but slower version
		unsigned long long pack;
		pack  = ((unsigned long long) data->move[1]) << 56;
		pack |= ((unsigned long long) data->move[0]) << 48;
		pack |= (((unsigned long long) data->upper) & 0xff) << 40; // sign extension masked
		pack |= (((unsigned long long) data->lower) & 0xff) << 32; // sign extension masked
		pack |= ((unsigned long long) data->date) << 24;
		pack |= ((unsigned long long) data->cost) << 16;
		pack |= ((unsigned long long) data->selectivity) << 8;
		pack |= ((unsigned long long) data->depth);

		return pack;
	}
}

/**
 * @brief Initialise the hashtable.
 *
 * Allocate the hash table entries and initialise the hash masks.
 *
 * @param hash_table Hash table to setup.
 * @param size Requested size for the hash table in number of bits.
 */
void hash_init(HashTable *hash_table, const int size)
{
	int n_way;

	for (n_way = 1; n_way < HASH_N_WAY; n_way <<= 1);

	assert(hash_table != NULL && size > 0);
	assert((n_way & -n_way) == n_way);

	info("< init hashtable of %u entries>\n", size);
	if (hash_table->hash != NULL) free(hash_table->memory);
	hash_table->memory = malloc((size + n_way + 1) * sizeof (Hash));
	if (hash_table->memory == NULL) {
		fatal_error("hash_init: cannot allocate the hash table\n");
	}

	if (HASH_ALIGNED) {
		const size_t alignment = n_way * sizeof (Hash) - 1;
		hash_table->hash = (Hash*) (((size_t) hash_table->memory + alignment) & ~alignment);
		hash_table->hash_mask = size - n_way;
	} else {
		hash_table->hash = (Hash*) hash_table->memory;
		hash_table->hash_mask = size - 1;
	}

	for (hash_table->size = 1; (1 << hash_table->size) < size; ++hash_table->size);

	hash_cleanup(hash_table);
}

/**
 * @brief Clear the hashtable.
 *
 * Set all hash table entries to zero.
 * @param hash_table Hash table to clear.
 */
void hash_cleanup(HashTable *hash_table)
{
	register unsigned int i;

	assert(hash_table != NULL && hash_table->hash != NULL);

	info("< cleaning hashtable >\n");
	for (i = 0; i <= hash_table->hash_mask + HASH_N_WAY; ++i) {
		hash_table->hash[i].code = 0;
		hash_table->hash[i].data = HASH_DATA_INIT;
	}
	hash_table->date = 0;
}

/**
 * @brief Clear the hashtable.
 *
 * Change the date of the hash table.
 * @param hash_table Hash table to clear.
 */
void hash_clear(HashTable *hash_table)
{
	assert(hash_table != NULL);

	if (hash_table->date == 127) hash_cleanup(hash_table);
	++hash_table->date;
	info("< clearing hashtable -> date = %d>\n", hash_table->date);
	assert(hash_table->date > 0 && hash_table->date <= 127);
}

/**
 * @brief Free the hashtable.
 *
 * Free the memory allocated by the hash table entries
 * @param hash_table hash_table to free.
 */
void hash_free(HashTable *hash_table)
{
	assert(hash_table != NULL && hash_table->hash != NULL);
	free(hash_table->memory);
	hash_table->hash = NULL;
}

/**
 * @brief make a level from date, cost, depth & selectivity.
 *
 * @param data Hash data.
 * @return A level.
 */
inline unsigned int writeable_level(HashData *data)
{
	if (USE_TYPE_PUNING) { // HACK: depends on little endian.
		return (*(unsigned int*) data);
	} else { // slow but more portable implementation.
		return (((unsigned)data->date) << 24) + (((unsigned)data->cost) << 16) + (data->selectivity << 8) + data->depth;
	}
}

/**
 * @brief update an hash table item.
 *
 * This is done when the level is the same as the previous storage.
 * Best moves & bound scores are updated, other data are untouched.
 *
 * @param data Hash Data.
 * @param cost Search cost (log2(node count)).
 * @param alpha Alpha bound.
 * @param beta Beta bound.
 * @param score Best score.
 * @param move Best move.
 */
static void data_update(HashData *data, const int cost, const int alpha, const int beta, const int score, const int move)
{
	if (score < beta && score < data->upper) data->upper = (signed char) score;
	if (score > alpha && score > data->lower) data->lower = (signed char) score;
	if ((score > alpha || score == SCORE_MIN) && data->move[0] != move) {
		data->move[1] = data->move[0];
		data->move[0] = (unsigned char) move;
	}
	data->cost = (unsigned char) MAX(cost, data->cost);
	HASH_STATS(++statistics.n_hash_update;)
}

/**
 * @brief Upgrade an hash table item.
 *
 * Upgrade is done when the search level increases.
 * Best moves are updated, others data are reset to new value.
 *
 * @param data Hash Data.
 * @param depth Search depth.
 * @param selectivity Search selectivity.
 * @param cost Search cost (log2(node count)).
 * @param alpha Alpha bound.
 * @param beta Beta bound.
 * @param score Best score.
 * @param move Best move.
 */
static void data_upgrade(HashData *data, const int depth, const int selectivity, const int cost, const int alpha, const int beta, const int score, const int move)
{
	if (score < beta) data->upper = (signed char) score; else data->upper = SCORE_MAX;
	if (score > alpha) data->lower =(signed char) score; else data->lower = SCORE_MIN;
	if ((score > alpha || score == SCORE_MIN) && data->move[0] != move) {
		data->move[1] = data->move[0];
		data->move[0] = (unsigned char) move;
	}
	data->depth = depth;
	data->selectivity = selectivity;
	data->cost = (unsigned char) MAX(cost, data->cost);  // this may not work well in parallel search.
	HASH_STATS(++statistics.n_hash_upgrade;)

	assert(data->upper >= data->lower);
}

static void data_new(HashData *data, const int date, const int depth, const int selectivity, const int cost, const int alpha, const int beta, const int score, const int move)
{
	if (score < beta) data->upper = (signed char) score; else data->upper = SCORE_MAX;
	if (score > alpha) data->lower = (signed char) score; else data->lower = SCORE_MIN;
	if (score > alpha || score == SCORE_MIN) data->move[0] = (unsigned char) move;
	else data->move[0] = NOMOVE;
	data->move[1] = NOMOVE;
	data->depth = depth;
	data->selectivity = selectivity;
	data->cost = cost;
	data->date = date;
	assert(data->upper >= data->lower);
}

/**
 * @brief Initialize a new hash table item.
 *
 * This implementation tries to be robust against concurrency. Data are first
 * set up in a local thread-safe structure, before being copied into the
 * hashtable entry. Then the hashcode of the entry is xored with the thread
 * safe structure ; so that any corrupted entry won't be readable.
 *
 * @param hash Hash Entry.
 * @param hash_code Hash code.
 * @param date Hash date.
 * @param depth Search depth.
 * @param selectivity Search selectivity.
 * @param cost Search cost.
 * @param alpha Alpha bound.
 * @param beta Beta bound.
 * @param score Best score.
 * @param move Best move.
 */
static void hash_new(Hash *hash, const unsigned long long hash_code, const int date, const int depth, const int selectivity, const int cost, const int alpha, const int beta, const int score, const int move)
{
	HashData data[1];

	data_new(data, date, depth, selectivity, cost, alpha, beta, score, move);
	HASH_STATS(if (date == hash->data.date) ++statistics.n_hash_remove;)
	HASH_STATS(++statistics.n_hash_new;)
	hash->data = *data;
	hash->code = hash_code ^ hash_key(data);
	LOCKFREE_STATS(bool ok = (hash->code == (hash_code ^ hash_key(&hash->data)));)
	LOCKFREE_STATS(statistics.n_concurrent_garbage += !ok;)
}

/**
 * @brief Set a new hash table item.
 *
 * This implementation tries to be robust against concurrency. Data are first
 * set up in a local thread-safe structure, before being copied into the
 * hashtable entry. Then the hashcode of the entry is xored with the thread
 * safe structure ; so that any corrupted entry won't be readable.
 *
 * @param hash Hash Entry.
 * @param hash_code Hash code.
 * @param date Hash date.
 * @param depth Search depth.
 * @param selectivity Search selectivity.
 * @param cost Search cost.
 * @param lower Lower score bound.
 * @param upper Upper score bound.
 * @param move Best move.
 */
void hash_set(Hash *hash, const unsigned long long hash_code, const int date, const int depth, const int selectivity, const int cost, const int lower, const int upper, const int move)
{
	HashData data[1];
	
	data->upper = (signed char) upper;
	data->lower = (signed char) lower;
	data->move[0] = (unsigned char) move;
	data->move[1] = NOMOVE;
	data->depth = (unsigned char) depth;
	data->selectivity = (unsigned char) selectivity;
	data->cost = (unsigned char) cost;
	data->date = (unsigned char) date;
	assert(data->upper >= data->lower);
	HASH_STATS(if (date == hash->data.date) ++statistics.n_hash_remove;)
	HASH_STATS(++statistics.n_hash_new;)
	hash->data = *data;
	hash->code = hash_code ^ hash_key(data);
	LOCKFREE_STATS(bool ok = (hash->code == (hash_code ^ hash_key(&hash->data)));)
	LOCKFREE_STATS(statistics.n_concurrent_garbage += !ok;)
}


/**
 * @brief update the hash entry
 *
 * This implementation tries to be robust against concurrency. Data are first
 * set up in a local thread-safe structure, before being copied into the
 * hashtable entry. Then the hashcode of the entry is xored with the thread
 * safe structure ; so that any corrupted entry won't be readable.
 *
 * @param hash Hash Entry.
 * @param hash_code Hash code.
 * @param date Hash date.
 * @param depth Search depth.
 * @param selectivity Search selectivity.
 * @param cost Hash Cost (log2(node count)).
 * @param alpha Alpha bound.
 * @param beta Beta bound.
 * @param score Best score.
 * @param move Best move.
 * @return true if an entry has been updated, false otherwise.
 */
static bool hash_update(Hash *hash, const unsigned long long hash_code, const int date, const int depth, const int selectivity, const int cost, const int alpha, const int beta, const int score, const int move)
{
	HashData data[1];
	bool ok;

	*data = hash->data;
	if ((hash->code ^  hash_key(data)) == hash_code) {
		if (data->selectivity == selectivity && data->depth == depth) data_update(data, cost, alpha, beta, score, move);
		else data_upgrade(data, depth, selectivity, cost, alpha, beta, score, move);
		data->date = (unsigned char) date;
		if (data->lower > data->upper) { // reset the hash-table...
			data_new(data, date, depth, selectivity, cost, alpha, beta, score, move);
		}
		hash->data = *data;
		hash->code = hash_code ^ hash_key(data);
		ok = (hash->code == (hash_code ^ hash_key(&hash->data)));
		LOCKFREE_STATS(statistics.n_concurrent_garbage += !ok;)
		return ok;
	}
	return false;
}

/**
 * @brief replace the hash entry.
 *
 * This implementation tries to be robust against concurrency. Data are first
 * set up in a local thread-safe structure, before being copied into the
 * hashtable entry. Then the hashcode of the entry is xored with the thread
 * safe structure ; so that any corrupted entry won't be readable.
 *
 * @param hash Hash Entry.
 * @param hash_code Hash code.
 * @param date Hash date.
 * @param depth Search depth.
 * @param selectivity Search selectivity.
 * @param cost Hash Cost (log2(node count)).
 * @param alpha Alpha bound.
 * @param beta Beta bound.
 * @param score Best score.
 * @param move Best move.
 * @return true if an entry has been replaced, false otherwise.
 */
static bool hash_replace(Hash *hash, const unsigned long long hash_code, const int date, const int depth, const int selectivity, const int cost, const int alpha, const int beta, const int score, const int move)
{
	HashData data[1];
	bool ok;
	
	*data = hash->data;
	if ((hash->code ^  hash_key(data)) == hash_code) {
		data_new(data, date, depth, selectivity, cost, alpha, beta, score, move);
		hash->data = *data;
		hash->code = hash_code ^ hash_key(data);
		ok = (hash->code == (hash_code ^ hash_key(&hash->data)));
		LOCKFREE_STATS(statistics.n_concurrent_garbage += !ok;)
		return ok;
	}
	return false;
}

/**
 * @brief Reset an hash entry from new data values.
 *
 * @param hash Hash Entry.
 * @param hash_code Hash code.
 * @param date Hash date.
 * @param depth Search depth.
 * @param selectivity Search selectivity.
 * @param lower Lower score bound.
 * @param upper Upper score bound.
 * @param move Best move.
 */
bool hash_reset(Hash *hash, const unsigned long long hash_code, const int date, const int depth, const int selectivity, const int lower, const int upper, const int move)
{
	HashData data[1];
	bool ok;

	*data = hash->data;
	if ((hash->code ^  hash_key(data)) == hash_code) {
		if (data->selectivity == selectivity && data->depth == depth) {
			if (data->lower < lower) data->lower = (signed char) lower;
			if (data->upper > upper) data->upper = (signed char) upper;
		} else {
			data->depth = (unsigned char) depth;
			data->selectivity = (unsigned char) selectivity;
			data->lower = (signed char) lower;
			data->upper = (signed char) upper;		
		}
		data->cost = 0;
		data->date = (unsigned char) date;
		if (move != NOMOVE) {
			if (data->move[0] != move) {
				data->move[1] = data->move[0];
				data->move[0] = (unsigned char) move;
			} else {
				data->move[1] = (unsigned char) move;
			}
		}
		hash->data = *data;
		hash->code = hash_code ^ hash_key(data);
		ok = (hash->code == (hash_code ^ hash_key(&hash->data)));
		LOCKFREE_STATS(statistics.n_concurrent_garbage += !ok;)
		return ok;
	}
	return false;
}

/**
 * @brief Feed hash table (from Cassio).
 *
 * @param hash_table Hash Table.
 * @param hash_code Hash code.
 * @param depth Search depth.
 * @param selectivity Selectivity level.
 * @param lower Alpha bound.
 * @param upper Beta bound.
 * @param move best move.
 */
void hash_feed(HashTable *hash_table, const unsigned long long hash_code, const int depth, const int selectivity, const int lower, const int upper, const int move)
{
	Hash *hash, *worst;
	int i;
	int date = hash_table->date ? hash_table->date : 1;

	worst = hash = hash_table->hash + (hash_code & hash_table->hash_mask);
	if (hash_reset(hash, hash_code, date, depth, selectivity, lower, upper, move)) return;

	for (i = 1; i < HASH_N_WAY; ++i) {
		++hash;
		if (hash_reset(hash, hash_code, date, depth, selectivity, lower, upper, move)) return;
		if (writeable_level(&worst->data) > writeable_level(&hash->data)) worst = hash;
	}

	// new entry
	hash_set(worst, hash_code, date, depth, selectivity, 0, lower, upper, move);
}

/**
 * @brief Store an hashtable item
 *
 * Find an hash table entry according to the evaluated board hash codes. Then
 * update the entry if it already exists otherwise create a new one. Collisions
 * are managed in such a way that better existing entries are always preserved
 * and the new evaluated data is always added. Lower and  upper score bounds
 * are then updated/set from the alpha, beta and score values according to the
 * following alphabeta property (where alpha < beta):
 *     -if (score >= beta) score is a lower bound of the real score
 *     -if (score <= alpha) score is an upper bound of the real score
 *     -if (alpha < score && score < beta) score equals the real score
 * So:
 *     -if (score < beta) update the upper bound of the hash entry
 *     -if (score > alpha) update the lower bound of the hash entry
 * The best move is also stored, but only if score >= alpha. In case the entry
 * already exists with better data, nothing is stored.
 *
 * @param hash_table Hash table to update.
 * @param hash_code  Hash code of an othello board.
 * @param alpha      Alpha bound when calling the alphabeta function.
 * @param depth      Search depth.
 * @param selectivity   Search selectivity.
 * @param cost       Search cost (i.e. log2(node count)).
 * @param beta       Beta bound when calling the alphabeta function.
 * @param score      Best score found.
 * @param move       Best move found.
 */
void hash_store(HashTable *hash_table, const unsigned long long hash_code, const int depth, const int selectivity, const int cost, const int alpha, const int beta, const int score, const int move)
{
	register int i;
	Hash *worst, *hash;

	worst = hash = hash_table->hash + (hash_code & hash_table->hash_mask);
	if (hash_update(hash, hash_code, hash_table->date, depth, selectivity, cost, alpha, beta, score, move)) return;

	for (i = 1; i < HASH_N_WAY; ++i) {
		++hash;
		if (hash_update(hash, hash_code, hash_table->date, depth, selectivity, cost, alpha, beta, score, move)) return;
		if (writeable_level(&worst->data) > writeable_level(&hash->data)) worst = hash;
	}

	hash_new(worst, hash_code, hash_table->date, depth, selectivity, cost, alpha, beta, score, move);
}

/**
 * @brief Store an hashtable item.
 *
 * Does the same as hash_store() except it always store the current search state
 *
 * @param hash_table Hash table to update.
 * @param hash_code  Hash code of an othello board.
 * @param alpha      Alpha bound when calling the alphabeta function.
 * @param depth      Search depth.
 * @param selectivity   Search selectivity.
 * @param cost       Search cost (i.e. log2(node count)).
 * @param beta       Beta bound when calling the alphabeta function.
 * @param score      Best score found.
 * @param move       Best move found.
 */
void hash_force(HashTable *hash_table, const unsigned long long hash_code, const int depth, const int selectivity, const int cost, const int alpha, const int beta, const int score, const int move)
{
	register int i;
	Hash *worst, *hash;
	
	worst = hash = hash_table->hash + (hash_code & hash_table->hash_mask);
	if (hash_replace(hash, hash_code, hash_table->date, depth, selectivity, cost, alpha, beta, score, move)) return;
	
	for (i = 1; i < HASH_N_WAY; ++i) {
		++hash;
		if (hash_replace(hash, hash_code, hash_table->date, depth, selectivity, cost, alpha, beta, score, move)) return;
		if (writeable_level(&worst->data) > writeable_level(&hash->data)) worst = hash;
	}
	
	hash_new(worst, hash_code, hash_table->date, depth, selectivity, cost, alpha, beta, score, move);
}

/**
 * @brief Find an hash table entry according to the evaluated board hash codes.
 *
 * @param hash_table Hash table.
 * @param hash_code Hash code of an othello board.
 * @param data Output hash data.
 * @return True the board was found, false otherwise.
 */
bool hash_get(HashTable *hash_table, const unsigned long long hash_code, HashData *data)
{
	register int i;
	const Hash *hash;
	bool ok = false;

	HASH_STATS(++statistics.n_hash_search;)
	hash = hash_table->hash + (hash_code & hash_table->hash_mask);
	for (i = 0; i < HASH_N_WAY; ++i) {
		if ((hash->code ^ hash_key(&hash->data)) == hash_code) {
			*data = hash->data;
			ok = ((hash->code ^ hash_key(data)) == hash_code);
			LOCKFREE_STATS(statistics.n_concurrent_garbage += !ok;)
			HASH_STATS(++statistics.n_hash_found);
			return ok && (data->date > 0);
		}
		++hash;
	}
	*data = HASH_DATA_INIT;
	return false;
}

/**
 * @brief Erase an hash table entry.
 *
 * @param hash_table Hash table.
 * @param hash_code Hash code of an othello board.
 * @param move Move to exclude.
 */
void hash_exclude_move(HashTable *hash_table, const unsigned long long hash_code, const int move)
{
	register int i;
	Hash *hash;

	hash = hash_table->hash + (hash_code & hash_table->hash_mask);
	for (i = 0; i < HASH_N_WAY; ++i) {
		if ((hash->code ^ hash_key(&hash->data)) == hash_code) {
			if (hash->data.move[0] == move) hash->data.move[0] = hash->data.move[1];
			hash->data.move[1] = NOMOVE;
			hash->data.lower = SCORE_MIN;
			return;
		}
		++hash;
	}
}

/**
 * @brief Copy an hastable to another one.
 *
 * @param src Source hash table to copy.
 * @param dest Destination hash table.
 */
void hash_copy(const HashTable *src, HashTable *dest)
{
	register unsigned int i;

	assert(src->hash_mask == dest->hash_mask);
	info("<hash copy>\n");
	for (i = 0; i <= src->hash_mask + HASH_N_WAY; ++i) {
		dest->hash[i] = src->hash[i];
	}
	dest->date = src->date;
}

/**
 * @brief print HashData content.
 *
 * @param data Hash Data
 * @param f Output stream
 */
void hash_print(const HashData *data, FILE *f)
{
	char s_move[4];
	int p_selectivity[] = {72, 87, 95, 98, 99, 100};

	fprintf(f, "moves = %s, ", move_to_string(data->move[0], WHITE, s_move));
	fprintf(f, "%s ; ", move_to_string(data->move[1], WHITE, s_move));
	fprintf(f, "score = [%+02d, %+02d] ; ", data->lower, data->upper);
	fprintf(f, "level = %2d:%2d:%2d@%3d%%", data->date, data->cost, data->depth, p_selectivity[data->selectivity]);
}

