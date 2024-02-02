/**
 * @file hash.c
 *
 * @brief Locked transposition table.
 *
 * The hash table is an efficient memory system to remember the previously
 * analysed positions and re-use the collected data when needed.
 * The hash table contains entries of analysed data where the board is stored
 * and the results of the recorded analysis are two score bounds, the level of
 * the analysis and the best move found.
 * The implementation is now a multi-way (or bucket based) hashtable. It both
 * tries to keep the deepest records and to always add the latest one.
 * The following implementation store the whole board to avoid collision. 
 * When doing parallel search with a shared hashtable, a locked implementation
 * avoid concurrency collisions.
 *
 * @date 1998 - 2017
 * @author Richard Delorme
 * @version 4.4
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
 * @brief Initialise the hashtable.
 *
 * Allocate the hash table entries and initialise the hash masks.
 *
 * @param hash_table Hash table to setup.
 * @param size Requested size for the hash table in number of entries.
 */
void hash_init(HashTable *hash_table, const unsigned long long size)
{
	int i, n_way;

	for (n_way = 1; n_way < HASH_N_WAY; n_way <<= 1);

	assert(hash_table != NULL);
	assert((n_way & -n_way) == n_way);

	info("< init hashtable of %llu entries>\n", size);
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

	hash_cleanup(hash_table);

	hash_table->n_lock = 256 * MAX(get_cpu_number(), 1);
	hash_table->lock_mask = hash_table->n_lock - 1;
	hash_table->n_lock += n_way + 1;
	hash_table->lock = (HashLock*) malloc(hash_table->n_lock * sizeof (HashLock));

	for (i = 0; i < hash_table->n_lock; ++i) spin_init(hash_table->lock + i);
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
		HASH_COLLISIONS(hash_table->hash[i].key = 0;)
		hash_table->hash[i].board.player = hash_table->hash[i].board.opponent = 0; 
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
	int i;
	assert(hash_table != NULL && hash_table->hash != NULL);
	free(hash_table->memory);
	hash_table->hash = NULL;
	for (i = 0; i < hash_table->n_lock; ++i) spin_free(hash_table->lock + i);
	free(hash_table->lock);
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
 * @brief Upgrade an hash table data item.
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

/**
 * @brief Set an hash table data item.
 *
 * @param data Hash Data.
 * @param date Search Date.
 * @param depth Search depth.
 * @param selectivity Search selectivity.
 * @param cost Search cost (log2(node count)).
 * @param alpha Alpha bound.
 * @param beta Beta bound.
 * @param score Best score.
 * @param move Best move.
 */
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
 * @param lock Lock.
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
#if (HASH_COLLISIONS(1)+0)
static void hash_new(Hash *hash, HashLock *lock, const unsigned long long hash_code, const Board* board, const int date, const int depth, const int selectivity, const int cost, const int alpha, const int beta, const int score, const int move)
#else
static void hash_new(Hash *hash, HashLock *lock, const Board* board, const int date, const int depth, const int selectivity, const int cost, const int alpha, const int beta, const int score, const int move)
#endif
{
	spin_lock(lock);
	HASH_STATS(if (date == hash->data.date) ++statistics.n_hash_remove;)
	HASH_STATS(++statistics.n_hash_new;)
	HASH_COLLISIONS(hash->key = hash_code;)
	hash->board = *board;
	data_new(&hash->data, date, depth, selectivity, cost, alpha, beta, score, move);
	spin_unlock(lock);
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
 * @param lock Lock.
 * @param hash_code Hash code.
 * @param date Hash date.
 * @param depth Search depth.
 * @param selectivity Search selectivity.
 * @param cost Search cost.
 * @param lower Lower score bound.
 * @param upper Upper score bound.
 * @param move Best move.
 */
#if (HASH_COLLISIONS(1)+0)
static void hash_set(Hash *hash, HashLock *lock, const unsigned long long hash_code, const Board *board, const int date, const int depth, const int selectivity, const int cost, const int lower, const int upper, const int move)
#else
static void hash_set(Hash *hash, HashLock *lock, const Board *board, const int date, const int depth, const int selectivity, const int cost, const int lower, const int upper, const int move)
#endif
{
	spin_lock(lock);
	HASH_STATS(if (date == hash->data.date) ++statistics.n_hash_remove;)
	HASH_STATS(++statistics.n_hash_new;)
	HASH_COLLISIONS(hash->key = hash_code;)
	hash->board = *board;
	hash->data.upper = (signed char) upper;
	hash->data.lower = (signed char) lower;
	hash->data.move[0] = (unsigned char) move;
	hash->data.move[1] = NOMOVE;
	hash->data.depth = (unsigned char) depth;
	hash->data.selectivity = (unsigned char) selectivity;
	hash->data.cost = (unsigned char) cost;
	hash->data.date = (unsigned char) date;
	assert(hash->data.upper >= hash->data.lower);
	spin_unlock(lock);
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
 * @param lock Lock.
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
static bool hash_update(Hash *hash, HashLock *lock, const Board *board, const int date, const int depth, const int selectivity, const int cost, const int alpha, const int beta, const int score, const int move)
{
	bool ok = false;
	HashData *data;

	if (hash->board.player == board->player && hash->board.opponent == board->opponent) {
		spin_lock(lock);
		if (hash->board.player == board->player && hash->board.opponent == board->opponent) {
			data = &hash->data;
			if (data->selectivity == selectivity && data->depth == depth) data_update(data, cost, alpha, beta, score, move);
			else data_upgrade(data, depth, selectivity, cost, alpha, beta, score, move);
			data->date = (unsigned char) date;
			if (data->lower > data->upper) { // reset the hash-table...
				data_new(data, date, depth, selectivity, cost, alpha, beta, score, move);
			}
			ok = true;
		} 
		spin_unlock(lock);
	}
	return ok;
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
 * @param lock Lock.
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
static bool hash_replace(Hash *hash, HashLock *lock, const Board *board, const int date, const int depth, const int selectivity, const int cost, const int alpha, const int beta, const int score, const int move)
{
	bool ok = false;
	
	if (hash->board.player == board->player && hash->board.opponent == board->opponent) {
		spin_lock(lock);
		if (hash->board.player == board->player && hash->board.opponent == board->opponent) {
			data_new(&hash->data, date, depth, selectivity, cost, alpha, beta, score, move);
			ok = true;
		}
		spin_unlock(lock);
	}
	return ok;
}

/**
 * @brief Reset an hash entry from new data values.
 *
 * @param hash Hash Entry.
 * @param lock Lock.
 * @param hash_code Hash code.
 * @param date Hash date.
 * @param depth Search depth.
 * @param selectivity Search selectivity.
 * @param lower Lower score bound.
 * @param upper Upper score bound.
 * @param move Best move.
 */
static bool hash_reset(Hash *hash, HashLock *lock, const Board *board, const int date, const int depth, const int selectivity, const int lower, const int upper, const int move)
{
	bool ok = false;
	HashData *data;
	
	if (hash->board.player == board->player && hash->board.opponent == board->opponent) {
		spin_lock(lock);
		if (hash->board.player == board->player && hash->board.opponent == board->opponent) {
			data = &hash->data;
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
			ok = true;
		}
		spin_unlock(lock);
	}
	return ok;
}


/**
 * @brief feed hash table (from Cassio).
 *
 * @param hash_table Hash Table.
 * @param hash_code Hash code.
 * @param depth Search depth.
 * @param selectivity Selectivity level.
 * @param lower Alpha bound.
 * @param upper Beta bound.
 * @param move best move.
 */
void hash_feed(HashTable *hash_table, const Board *board, const unsigned long long hash_code, const int depth, const int selectivity, const int lower, const int upper, const int move)
{
	Hash *hash, *worst;
	HashLock *lock; 
	int i;
	int date = hash_table->date ? hash_table->date : 1;

	worst = hash = hash_table->hash + (hash_code & hash_table->hash_mask);
	lock = hash_table->lock + (hash_code & hash_table->lock_mask);
	if (hash_reset(hash, lock, board, date, depth, selectivity, lower, upper, move)) return;

	for (i = 1; i < HASH_N_WAY; ++i) {
		++hash;
		if (hash_reset(hash, lock, board, date, depth, selectivity, lower, upper, move)) return;
		if (writeable_level(&worst->data) > writeable_level(&hash->data)) {
			worst = hash;
		}
	}

	// new entry
#if (HASH_COLLISIONS(1)+0) 
	hash_set(worst, lock, hash_code, board, date, depth, selectivity, 0, lower, upper, move);
#else 
	hash_set(worst, lock, board, date, depth, selectivity, 0, lower, upper, move);
#endif
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
void hash_store(HashTable *hash_table, const Board *board, const unsigned long long hash_code, const int depth, const int selectivity, const int cost, const int alpha, const int beta, const int score, const int move)
{
	register int i;
	Hash *worst, *hash;
	HashLock *lock;

	worst = hash = hash_table->hash + (hash_code & hash_table->hash_mask);
	lock = hash_table->lock + (hash_code & hash_table->lock_mask);
	if (hash_update(hash, lock, board, hash_table->date, depth, selectivity, cost, alpha, beta, score, move)) return;

	for (i = 1; i < HASH_N_WAY; ++i) {
		++hash;
		if (hash_update(hash, lock, board, hash_table->date, depth, selectivity, cost, alpha, beta, score, move)) return;
		if (writeable_level(&worst->data) > writeable_level(&hash->data)) {
			worst = hash;
		}
	}
#if (HASH_COLLISIONS(1)+0) 
	hash_new(worst, lock, hash_code, board, hash_table->date, depth, selectivity, cost, alpha, beta, score, move);
#else 
	hash_new(worst, lock, board, hash_table->date, depth, selectivity, cost, alpha, beta, score, move);
#endif
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
void hash_force(HashTable *hash_table, const Board *board, const unsigned long long hash_code, const int depth, const int selectivity, const int cost, const int alpha, const int beta, const int score, const int move)
{
	register int i;
	Hash *worst, *hash;
	HashLock *lock;
	
	worst = hash = hash_table->hash + (hash_code & hash_table->hash_mask);
	lock = hash_table->lock + (hash_code & hash_table->lock_mask);
	if (hash_replace(hash, lock, board, hash_table->date, depth, selectivity, cost, alpha, beta, score, move)) return;
	
	for (i = 1; i < HASH_N_WAY; ++i) {
		++hash;
		if (hash_replace(hash, lock, board, hash_table->date, depth, selectivity, cost, alpha, beta, score, move)) return;
		if (writeable_level(&worst->data) > writeable_level(&hash->data)) {
			worst = hash;
		}
	}
	
#if (HASH_COLLISIONS(1)+0) 
	hash_new(worst, lock, hash_code, board, hash_table->date, depth, selectivity, cost, alpha, beta, score, move);
#else 
	hash_new(worst, lock, board, hash_table->date, depth, selectivity, cost, alpha, beta, score, move);
#endif
}

/**
 * @brief Find an hash table entry according to the evaluated board hash codes.
 *
 * @param hash_table Hash table.
 * @param hash_code Hash code of an othello board.
 * @param data Output hash data.
 * @return True the board was found, false otherwise.
 */
bool hash_get(HashTable *hash_table, const Board *board, const unsigned long long hash_code, HashData *data)
{
	register int i;
	Hash *hash;
	HashLock *lock;
	bool ok = false;

	HASH_STATS(++statistics.n_hash_search;)
	HASH_COLLISIONS(++statistics.n_hash_n;)
	hash = hash_table->hash + (hash_code & hash_table->hash_mask);
	lock = hash_table->lock + (hash_code & hash_table->lock_mask);
	for (i = 0; i < HASH_N_WAY; ++i) {
		HASH_COLLISIONS(if (hash->key == hash_code) {)
		HASH_COLLISIONS(	spin_lock(lock);)
		HASH_COLLISIONS(	if (hash->key == hash_code && (hash->board.player != board->player || hash->board.opponent != board->opponent)) {)
		HASH_COLLISIONS(		++statistics.n_hash_collision;)
		HASH_COLLISIONS(		printf("key = %llu\n", hash_code);)
		HASH_COLLISIONS(		board_print(board, WHITE, stdout);)
		HASH_COLLISIONS(		board_print(&hash->board, WHITE, stdout);)
		HASH_COLLISIONS(	})
		HASH_COLLISIONS(	spin_unlock(lock);)
		HASH_COLLISIONS(})
		if (hash->board.player == board->player && hash->board.opponent == board->opponent) {
			spin_lock(lock);
			if (hash->board.player == board->player && hash->board.opponent == board->opponent) {
				*data = hash->data;
				HASH_STATS(++statistics.n_hash_found;)
				hash->data.date = hash_table->date;
				ok = true;
			}
			spin_unlock(lock);
			if (ok) return true;
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
void hash_exclude_move(HashTable *hash_table, const Board *board, const unsigned long long hash_code, const int move)
{
	register int i;
	Hash *hash;
	HashLock *lock;

	hash = hash_table->hash + (hash_code & hash_table->hash_mask);
	lock = hash_table->lock + (hash_code & hash_table->lock_mask);
	for (i = 0; i < HASH_N_WAY; ++i) {
		if (hash->board.player == board->player && hash->board.opponent == board->opponent) {
			spin_lock(lock);
			if (hash->board.player == board->player && hash->board.opponent == board->opponent) {
				if (hash->data.move[0] == move) {
					hash->data.move[0] = hash->data.move[1];
					hash->data.move[1] = NOMOVE;
				} else if (hash->data.move[1] == move) {
					hash->data.move[1] = NOMOVE;
				}
				hash->data.lower = SCORE_MIN;
			}
			spin_unlock(lock);
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

