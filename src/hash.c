/**
 * @file hash.c
 *
 * @brief spined transposition table.
 *
 * The hash table is an efficient memory system to remember the previously
 * analysed positions and re-use the collected data when needed.
 * The hash table contains entries of analysed data where the board is stored
 * and the results of the recorded analysis are two score bounds, the level of
 * the analysis and the best move found.
 * The implementation is now a multi-way (or bucket based) hashtable. It both
 * tries to keep the deepest records and to always add the latest one.
 * The following implementation store the whole board to avoid collision.
 * When doing parallel search with a shared hashtable, a spined implementation
 * avoid concurrency collisions.
 *
 * @date 1998 - 2024
 * @author Richard Delorme
 * @author Toshihiko Okuhara
 * @version 4.6
 */

#include "bit.h"
#include "hash.h"
#include "options.h"
#include "stats.h"
#include "search.h"
#include "util.h"
#include "settings.h"

#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

/** HashData init value */
const HashData HASH_DATA_INIT = {{{0, 0, 0, 0}}, -SCORE_INF, SCORE_INF, {NOMOVE, NOMOVE}};
#if (HASH_COLLISIONS(1)+0)
	const Hash HASH_INIT = {0, {0, 0}, {{{0, 0, 0, 0}}, -SCORE_INF, SCORE_INF, {NOMOVE, NOMOVE}}};
#else
	const Hash HASH_INIT = {{0, 0}, {{{0, 0, 0, 0}}, -SCORE_INF, SCORE_INF, {NOMOVE, NOMOVE}}};
#endif

/**
 * @brief Initialise the hashtable.
 *
 * Allocate the hash table entries and initialise the hash masks.
 *
 * @param hash_table Hash table to setup.
 * @param size Requested size for the hash table in number of entries.
 */
void hash_init(HashTable *hash_table, const size_t size)
{
	const size_t ALIGNMENT = 32;

	assert(hash_table != NULL);
	assert(bit_is_single(size));
	assert(HASH_N_WAY + 1 <= ALIGNMENT);
	assert(((size + ALIGNMENT) * sizeof (Hash)) % ALIGNMENT == 0);

	info("< init hashtable of %zu entries>\n", size);
	if (hash_table->hash != NULL) free(hash_table->hash);
	hash_table->hash = aligned_alloc(ALIGNMENT, (size + ALIGNMENT) * sizeof (Hash));
	if (hash_table->hash == NULL) {
		fatal_error("hash_init: cannot allocate the hash table\n");
	}
	hash_table->hash_mask = size - 1;

	hash_cleanup(hash_table);

	hash_table->n_spin = 256 * MAX(get_cpu_number(), 1);
	hash_table->spin_mask = hash_table->n_spin - 1;
	hash_table->spin = (SpinLock*) malloc(hash_table->n_spin * sizeof (SpinLock));
	for (uint32_t i = 0; i < hash_table->n_spin; ++i) spinlock_init(hash_table->spin + i);

	HASH_STATS(hash_table->n_try   = 0;)
	HASH_STATS(hash_table->n_found = 0;)
	HASH_STATS(hash_table->n_store = 0;)
}

/**
 * @brief Clear the hashtable.
 *
 * Set all hash table entries to zero.
 * @param hash_table Hash table to clear.
 */
void hash_cleanup(HashTable *hash_table)
{
	assert(hash_table != NULL && hash_table->hash != NULL);

	size_t i = 0, hash_size = hash_table->hash_mask + HASH_N_WAY + 1;

	info("< cleaning hashtable >\n");

#if defined (__SSE2__) && !(HASH_COLLISIONS(1)+0)

	#if defined(__AVX__)

		assert(sizeof(Hash) == 24 && ((uint64_t)hash_table->hash & 0x1f) == 0);

		alignas(32) Hash hash_init[4] = {HASH_INIT, HASH_INIT, HASH_INIT, HASH_INIT};
		__m256i h0, h1, h2, *h = (__m256i*) hash_init;

		h0 = _mm256_load_si256(h);
		h1 = _mm256_load_si256(h + 1);
		h2 = _mm256_load_si256(h + 2);
		h = (__m256i*) hash_table->hash;
		for (; i < hash_size - 4; i += 4, h += 3) {
			_mm256_stream_si256(h, h0);
			_mm256_stream_si256(h + 1, h1);
			_mm256_stream_si256(h + 2, h2);
		}

	#else // __SSE2__

		Hash hash_init[2] = {HASH_INIT, HASH_INIT};

		__m128i h0, h1, h2, *h = (__m128i*) hash_init;
		h0 = _mm_load_si128(h);
		h1 = _mm_load_si128(h + 1);
		h2 = _mm_load_si128(h + 2);
		h = (__m128i*) hash_table->hash;
		for (; i < hash_size - 2; i += 2, h += 3) {
			_mm_stream_si128(h, h0);
			_mm_stream_si128(h + 1, h1);
			_mm_stream_si128(h + 2, h2);
		}

	#endif

	_mm_sfence();

#endif

	for (; i < hash_size; ++i) {
		hash_table->hash[i] = HASH_INIT;
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
	free(hash_table->hash);
	hash_table->hash = NULL;
	free(hash_table->spin);
	hash_table->spin = NULL;
	hash_table->n_spin = 0;
}

/**
 * @brief update a move in hash table item.
 *
 * @param data Hash data.
 * @param move move to add.
 */
static void data_add_move(HashData *data, const bool good_move, const int move)
{
	if (good_move && data->move[0] != move) {
		data->move[1] = data->move[0];
		data->move[0] = (uint8_t) move;
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
static void data_update(HashData *data, const HashStore *store)
{
	if (store->score < store->beta && store->score < data->upper) data->upper = store->score;
	if (store->score > store->alpha && store->score > data->lower) data->lower = store->score;
	data_add_move(data, (store->score > store->alpha || store->score == SCORE_MIN), store->move);
	data->draft.u1.cost = (uint8_t) MAX(store->draft.u1.cost, data->draft.u1.cost);
	data->draft.u1.date = store->draft.u1.date;
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
static void data_upgrade(HashData *data, const HashStore *store)
{
	if (store->score < store->beta) data->upper = store->score; else data->upper = SCORE_MAX;
	if (store->score > store->alpha) data->lower = store->score; else data->lower = SCORE_MIN;
	data_add_move(data, (store->score > store->alpha || store->score == SCORE_MIN), store->move);
	data->draft.u2.depth_selectivity = store->draft.u2.depth_selectivity;
	data->draft.u1.cost = MAX(store->draft.u1.cost, data->draft.u1.cost);
	data->draft.u1.date = store->draft.u1.date;
	HASH_STATS(++statistics.n_hash_upgrade;)

	assert(data->upper >= data->lower);
}

/**
 * @brief Set an hash table data item.
 *
 * All the data are set to a new value.
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
static void data_new(HashData *data, const HashStore *store)
{
	if (store->score < store->beta) data->upper = store->score; else data->upper = SCORE_MAX;
	if (store->score > store->alpha) data->lower = store->score; else data->lower = SCORE_MIN;
	data->move[0] = data->move[1] = NOMOVE;
	data_add_move(data, (store->score > store->alpha || store->score == SCORE_MIN), store->move);
	data->draft.u4 = store->draft.u4;
	assert(data->upper >= data->lower);
}

/**
 * @brief Prefetch the hash entry.
 *
 * The hash entry may not be in the CPU cache and take long to read, so
 * prefetch it as soon as the hash code is available.
 *
 * @param hashtable Hash table to fetch from.
 * @param hashcode Hash code.
*/
void hash_prefetch(HashTable *hashtable, const uint64_t hashcode) {
	Hash *hash = hashtable->hash + (hashcode & hashtable->hash_mask);
	#if defined(__GNUC__)
		__builtin_prefetch(hash);
		__builtin_prefetch(hash + HASH_N_WAY - 1);
	#elif defined(__SSE2__)
		_mm_prefetch((char const *) hash, _MM_HINT_T0);
		_mm_prefetch((char const *)(hash + HASH_N_WAY - 1), _MM_HINT_T0);
	#elif defined(__ARM_ACLE)
		__pld(hash);
		__pld(hash + HASH_N_WAY - 1);
	#elif defined(_M_ARM64)
		__prefetch(hash);
		__prefetch(hash + HASH_N_WAY - 1);
	#endif
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
 * @param spin spinlock.
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
static void hash_new(Hash *hash, SpinLock *spin, const uint64_t hash_code, const Board *board, const HashStore *store)
#else
static void hash_new(Hash *hash, SpinLock *spin, const Board* board, const HashStore *store)
#endif
{
	spinlock_lock(spin);
	HASH_STATS(if (date == hash->data.draft.u1.date) ++statistics.n_hash_remove;)
	HASH_STATS(++statistics.n_hash_new;)
	HASH_COLLISIONS(hash->key = hash_code;)
	hash->board = *board;
	data_new(&hash->data, store);
	spinlock_unlock(spin);
}

/**
 * @brief Set a new hash table item.
 *
 * This implementation tries to be robust against concurrency using a spin lock,
 * simpler and faster than mutex.
 *
 * @param hash Hash Entry.
 * @param spin a spin.
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
static void hash_set(Hash *hash, SpinLock *spin, const uint64_t hash_code, const Board *board, const HashData * data)
#else
static void hash_set(Hash *hash, SpinLock *spin, const Board *board, const HashData* data)
#endif
{
	spinlock_lock(spin);
	HASH_STATS(if (date == hash->data.draft.u1.date) ++statistics.n_hash_remove;)
	HASH_STATS(++statistics.n_hash_new;)
	HASH_COLLISIONS(hash->key = hash_code;)
	hash->board = *board;
	hash->data = *data;
	assert(hash->data.upper >= hash->data.lower);
	spinlock_unlock(spin);
}


/**
 * @brief update the hash entry
 *
 * This implementation tries to be robust against concurrency using a spin lock,
 * simpler and faster than mutex.
 *
 * @param hash Hash Entry.
 * @param spin spin.
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
static bool hash_update(Hash *hash, SpinLock *spin, const Board *board, const HashStore *store)
{
	bool ok = false;

	if (board_equal(&hash->board, board)) {
		spinlock_lock(spin);
		if (board_equal(&hash->board, board)) {
			if (hash->data.draft.u2.depth_selectivity == store->draft.u2.depth_selectivity) data_update(&hash->data,  store);
			else data_upgrade(&hash->data, store);
			if (hash->data.lower > hash->data.upper) data_new(&hash->data, store);
			ok = true;
		}
		spinlock_unlock(spin);
	}
	return ok;
}

/**
 * @brief replace the hash entry.
 *
 * This implementation tries to be robust against concurrency using a spin lock,
 * simpler and faster than mutex.
 *
 * @param hash Hash Entry.
 * @param spin spin.
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
static bool hash_replace(Hash *hash, SpinLock *spin, const Board *board, const HashStore *store)
{
	bool ok = false;

	if (board_equal(&hash->board, board)) {
		spinlock_lock(spin);
		if (board_equal(&hash->board, board)) {
			data_new(&hash->data, store);
			ok = true;
		}
		spinlock_unlock(spin);
	}
	return ok;
}

/**
 * @brief Reset an hash entry from new data values.
 *
 * @param hash Hash Entry.
 * @param spin spin.
 * @param hash_code Hash code.
 * @param date Hash date.
 * @param depth Search depth.
 * @param selectivity Search selectivity.
 * @param lower Lower score bound.
 * @param upper Upper score bound.
 * @param move Best move.
 */
static bool hash_reset(Hash *hash, SpinLock *spin, const Board *board, const HashData *data)
{
	bool ok = false;

	if (board_equal(&hash->board, board)) {
		spinlock_lock(spin);
		if (board_equal(&hash->board, board)) {
			if (hash->data.draft.u2.depth_selectivity == data->draft.u2.depth_selectivity) {
				hash->data.draft.u2.cost_date = data->draft.u2.cost_date;
				hash->data.lower = MAX(hash->data.lower, data->lower);
				hash->data.upper = MIN(hash->data.upper, data->upper);
			} else {
				hash->data = *data;
			}
			ok = true;
		}
		spinlock_unlock(spin);
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
void hash_feed(HashTable *hash_table, const Board *board, const uint64_t hash_code, const HashData *data)
{
	Hash *hash, *worst;
	SpinLock *spin;
	int i;

	worst = hash = hash_table->hash + (hash_code & hash_table->hash_mask);
	spin = hash_table->spin + (hash_code & hash_table->spin_mask);
	if (hash_reset(hash, spin, board, data)) return;

	for (i = 1; i < HASH_N_WAY; ++i) {
		++hash;
		if (hash_reset(hash, spin, board, data)) return;
		if (worst->data.draft.u4 > hash->data.draft.u4) worst = hash;
	}

	// new entry
#if (HASH_COLLISIONS(1)+0)
	hash_set(worst, spin, hash_code, board, data);
#else
	hash_set(worst, spin, board, data);
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
void hash_store(HashTable *hash_table, const Board *board, const uint64_t hash_code, const HashStore *store)
{
	int i;
	Hash *worst, *hash;
	SpinLock *spin;

	worst = hash = hash_table->hash + (hash_code & hash_table->hash_mask);
	spin = hash_table->spin + (hash_code & hash_table->spin_mask);
	if (hash_update(hash, spin, board, store)) return;

	for (i = 1; i < HASH_N_WAY; ++i) {
		++hash;
		if (hash_update(hash, spin, board, store)) return;
		if (worst->data.draft.u4 > hash->data.draft.u4) worst = hash;
	}
#if (HASH_COLLISIONS(1)+0)
	hash_new(worst, spin, hash_code, board, store);
#else
	hash_new(worst, spin, board, store);
#endif
	HASH_STATS(hash_table->n_store++;)

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
void hash_force(HashTable *hash_table, const Board *board, const uint64_t hash_code, const HashStore *store)
{
	int i;
	Hash *worst, *hash;
	SpinLock *spin;

	worst = hash = hash_table->hash + (hash_code & hash_table->hash_mask);
	spin = hash_table->spin + (hash_code & hash_table->spin_mask);
	if (hash_replace(hash, spin, board, store)) return;

	for (i = 1; i < HASH_N_WAY; ++i) {
		++hash;
		if (hash_replace(hash, spin, board, store)) return;
		if (worst->data.draft.u4 > hash->data.draft.u4) worst = hash;
	}

#if (HASH_COLLISIONS(1)+0)
	hash_new(worst, spin, hash_code, board, store);
#else
	hash_new(worst, spin, board, store);
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
bool hash_get(HashTable *hash_table, const Board *board, const uint64_t hash_code, HashData *data)
{
	int i;
	Hash *hash;
	SpinLock *spin;
	bool ok = false;

	HASH_STATS(++statistics.n_hash_search;)
	HASH_COLLISIONS(++statistics.n_hash_n;)
	HASH_STATS(hash_table->n_try++;)
	hash = hash_table->hash + (hash_code & hash_table->hash_mask);
	spin = hash_table->spin + (hash_code & hash_table->spin_mask);
	for (i = 0; i < HASH_N_WAY; ++i) {
		HASH_COLLISIONS(if (hash->key == hash_code) {)
		HASH_COLLISIONS(	spinlock_lock(spin);)
		HASH_COLLISIONS(	if (hash->key == hash_code && !board_equal(&hash->board, board)) {)
		HASH_COLLISIONS(		++statistics.n_hash_collision;)
		HASH_COLLISIONS(		printf("key = %" PRIu64 "\n", hash_code);)
		HASH_COLLISIONS(		board_print(board, WHITE, stdout);)
		HASH_COLLISIONS(		board_print(&hash->board, WHITE, stdout);)
		HASH_COLLISIONS(	})
		HASH_COLLISIONS(	spinlock_unlock(spin);)
		HASH_COLLISIONS(})
		if (board_equal(&hash->board, board)) {
			spinlock_lock(spin);
			if (board_equal(&hash->board, board)) {
				*data = hash->data;
				HASH_STATS(++statistics.n_hash_found;)
				HASH_STATS(hash_table->n_found++;)
				hash->data.draft.u1.date = hash_table->date;
				ok = true;
			}
			spinlock_unlock(spin);
			if (ok) return true;
		}
		++hash;
	}
	*data = HASH_DATA_INIT;
	return false;
}

/**
 * @brief Exclude a move from the hash table entry.
 *
 * @param hash_table Hash table.
 * @param hash_code Hash code of an othello board.
 * @param move Move to exclude.
 */
void hash_exclude_move(HashTable *hash_table, const Board *board, const uint64_t hash_code, const int move)
{
	int i;
	Hash *hash;

	hash = hash_table->hash + (hash_code & hash_table->hash_mask);
	for (i = 0; i < HASH_N_WAY; ++i) {
		if (board_equal(&hash->board, board)) {
			if (board_equal(&hash->board, board)) {
				if (hash->data.move[0] == move) {
					hash->data.move[0] = hash->data.move[1];
					hash->data.move[1] = NOMOVE;
				} else if (hash->data.move[1] == move) {
					hash->data.move[1] = NOMOVE;
				}
				hash->data.lower = SCORE_MIN;
			}
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
	unsigned int i;

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
	const int p_selectivity[] = {72, 87, 95, 98, 99, 100};

	fprintf(f, "moves = {%s, ", move_to_string(data->move[0], WHITE, s_move));
	fprintf(f, "%s }; ", move_to_string(data->move[1], WHITE, s_move));
	fprintf(f, "score = [%+02d, %+02d] ; ", data->lower, data->upper);
	fprintf(f, "draft = %2d:%2d:%2d@%d%%", data->draft.u1.date, data->draft.u1.cost, data->draft.u1.depth, p_selectivity[data->draft.u1.selectivity]);
}

