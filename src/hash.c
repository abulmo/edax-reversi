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
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
 * @date 1998 - 2023
=======
 * @date 1998 - 2020
>>>>>>> 0a166fd (Remove 1 element array coding style)
=======
 * @date 1998 - 2021
>>>>>>> 34a2291 (4.5.0: Use CRC32c for board hash)
=======
 * @date 1998 - 2022
>>>>>>> 11a54a6 (Revise get_corner_stability and hash_cleanup)
 * @author Richard Delorme
 * @version 4.5
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

<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
=======
/** hashing global data */
unsigned long long hash_move[64][60];

>>>>>>> 34a2291 (4.5.0: Use CRC32c for board hash)
=======
>>>>>>> f33d573 (Fix 'nboard pass not parsed' bug, crc32c for game hash too)
=======
// use vectored board if vectorcall available and hboard_equal is efficient enough
#ifdef _M_X64
	#define	store_hboard(p,b)	_mm_storeu_si128((__m128i *) (p), (b))
  #if defined(__SSE4__) || defined(__AVX__)
	inline bool hboard_equal(__m128i b1, Board *b2)
	{
		b1 = _mm_xor_si128(b1, _mm_loadu_si128((__m128i *) b2));
		return _mm_testz_si128(b1, b1);
	}
  #else
	#define	hboard_equal(b1,b2)	(_mm_movemask_epi8(_mm_cmpeq_epi8(b1, _mm_loadu_si128((__m128i *) b2))) == 0xffff)
  #endif

#elif defined(__aarch64__) || defined(_M_ARM64)
	#define	store_hboard(p,b)	vst1q_u64((uint64_t *) (p), (b))
  #ifdef _M_ARM64	// https://stackoverflow.com/questions/15389539/fastest-way-to-test-a-128-bit-neon-register-for-a-value-of-0-using-intrinsics
	#define	hboard_equal(b1,b2)	(neon_umaxvq32(veorq_u64((b1), vld1q_u64((uint64_t *) (b2)))) == 0)
  #else
	#define	hboard_equal(b1,b2)	(vmaxvq_u32(vreinterpretq_u32_u64(veorq_u64((b1), vld1q_u64((uint64_t *) (b2))))) == 0)
  #endif

#else
	#define	store_hboard(p,b)	*(p) = *(b)
	#define	hboard_equal(b1,b2)	board_equal(b1, b2)
#endif

>>>>>>> e88638e (add vectorcall interface to hash functions)
/** HashData init value */
const HashData HASH_DATA_INIT = {{{ 0, 0, 0, 0 }}, -SCORE_INF, SCORE_INF, { NOMOVE, NOMOVE }};
<<<<<<< HEAD
=======

/**
<<<<<<< HEAD
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
>>>>>>> a556e46 (HashData and HashStoreData rearranged, TYPE_PUNING now uses union)

/**
=======
>>>>>>> f33d573 (Fix 'nboard pass not parsed' bug, crc32c for game hash too)
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

<<<<<<< HEAD
<<<<<<< HEAD
	for (n_way = 1; n_way < HASH_N_WAY; n_way <<= 1);	// round up HASH_N_WAY to 2 ^ n
=======
	for (n_way = 1; n_way < HASH_N_WAY; n_way <<= 1);	// 2 ^ n, at leaset HASH_N_WAY
>>>>>>> 494a38b (AVX/SSE optimized hash_cleanup)
=======
	for (n_way = 1; n_way < HASH_N_WAY; n_way <<= 1);	// round up HASH_N_WAY to 2 ^ n
>>>>>>> 42dc349 (add sfence to be sure; correct comments)

	assert(hash_table != NULL);
	assert((n_way & -n_way) == n_way);

	info("< init hashtable of %llu entries>\n", size);
	if (hash_table->hash != NULL) free(hash_table->memory);
	hash_table->memory = malloc((size + n_way + 1) * sizeof (Hash));
	if (hash_table->memory == NULL) {
		fatal_error("hash_init: cannot allocate the hash table\n");
	}

	if (HASH_ALIGNED) {
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
		size_t alignment = n_way * sizeof (Hash);	// (4 * 24)
		alignment = (alignment & -alignment) - 1;	// LS1B - 1 (0x1f)
=======
		size_t alignment = n_way * sizeof (Hash);
		alignment = (alignment & -alignment) - 1;	// LS1B - 1
>>>>>>> c7739ca (Clearer Hash align for non-pow-2 sizeof(HASH))
=======
		size_t alignment = n_way * sizeof (Hash);	// (4 * 48)
		alignment = (alignment & -alignment) - 1;	// LS1B - 1 (0x3f)
>>>>>>> 494a38b (AVX/SSE optimized hash_cleanup)
=======
		size_t alignment = n_way * sizeof (Hash);	// (4 * 24)
		alignment = (alignment & -alignment) - 1;	// LS1B - 1 (0x1f)
>>>>>>> 42dc349 (add sfence to be sure; correct comments)
		hash_table->hash = (Hash*) (((size_t) hash_table->memory + alignment) & ~alignment);
		hash_table->hash_mask = size - n_way;
	} else {
		hash_table->hash = (Hash*) hash_table->memory;
		hash_table->hash_mask = size - 1;
	}

	hash_cleanup(hash_table);

	hash_table->n_lock = 1 << (31 - lzcnt_u32(get_cpu_number() | 1) + 8);	// round down to 2 ^ n, then * 256
	hash_table->lock_mask = hash_table->n_lock - 1;
	// hash_table->n_lock += n_way + 1;
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
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
	unsigned int i = 0, imax = hash_table->hash_mask + HASH_N_WAY;
	Hash *pHash = hash_table->hash;
=======
	unsigned int i;
>>>>>>> 0a166fd (Remove 1 element array coding style)
=======
	unsigned int i, imax = hash_table->hash_mask + HASH_N_WAY;
=======
	unsigned int i = 0, imax = hash_table->hash_mask + HASH_N_WAY;
>>>>>>> 494a38b (AVX/SSE optimized hash_cleanup)
	Hash *pHash = hash_table->hash;
>>>>>>> 11a54a6 (Revise get_corner_stability and hash_cleanup)

	assert(hash_table != NULL && hash_table->hash != NULL);

	info("< cleaning hashtable >\n");
<<<<<<< HEAD
<<<<<<< HEAD

  #if defined(hasSSE2) || defined(USE_MSVC_X86)
	if (hasSSE2 && (sizeof(Hash) == 24) && (((size_t) pHash & 0x1f) == 0) && (imax >= 7)) {
=======

  #if defined(hasSSE2) || defined(USE_MSVC_X86)
<<<<<<< HEAD
	if (hasSSE2 && (sizeof(Hash) == 24) && (((uintptr_t) pHash & 0x1f) == 0) && (imax >= 7)) {
>>>>>>> 494a38b (AVX/SSE optimized hash_cleanup)
=======
	if (hasSSE2 && (sizeof(Hash) == 24) && (((size_t) pHash & 0x1f) == 0) && (imax >= 7)) {
>>>>>>> 47c2589 (Fix w32-modern build and gcc build)
		for (; i < 4; ++i, ++pHash) {
			HASH_COLLISIONS(pHash->key = 0;)
			pHash->board.player = pHash->board.opponent = 0;
			pHash->data = HASH_DATA_INIT;
		}
    #ifdef __AVX__
		__m256i d0 = _mm256_load_si256((__m256i *)(pHash - 4));
		__m256i d1 = _mm256_load_si256((__m256i *)(pHash - 4) + 1);
		__m256i d2 = _mm256_load_si256((__m256i *)(pHash - 4) + 2);
		for (i = 4; i <= imax - 3; i += 4, pHash += 4) {
			_mm256_stream_si256((__m256i *) pHash, d0);
			_mm256_stream_si256((__m256i *) pHash + 1, d1);
			_mm256_stream_si256((__m256i *) pHash + 2, d2);
		}
    #else
		__m128i d0 = _mm_load_si128((__m128i *)(pHash - 4));
		__m128i d1 = _mm_load_si128((__m128i *)(pHash - 4) + 1);
		__m128i d2 = _mm_load_si128((__m128i *)(pHash - 4) + 2);
		for (i = 4; i <= imax - 1; i += 2, pHash += 2) {
			_mm_stream_si128((__m128i *) pHash, d0);
			_mm_stream_si128((__m128i *) pHash + 1, d1);
			_mm_stream_si128((__m128i *) pHash + 2, d2);
		}
    #endif
<<<<<<< HEAD
<<<<<<< HEAD
=======
>>>>>>> 42dc349 (add sfence to be sure; correct comments)
		_mm_sfence();
	}
  #endif
	for (; i <= imax; ++i, ++pHash) {
=======
	for (i = 0; i <= imax; ++i, ++pHash) {
>>>>>>> 11a54a6 (Revise get_corner_stability and hash_cleanup)
=======
	}
  #endif
	for (; i <= imax; ++i, ++pHash) {
>>>>>>> 494a38b (AVX/SSE optimized hash_cleanup)
		HASH_COLLISIONS(pHash->key = 0;)
		pHash->board.player = pHash->board.opponent = 0; 
		pHash->data = HASH_DATA_INIT;
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
#if USE_TYPE_PUNING	// HACK
	return (data->wl.ui);
#else	// slow but more portable implementation.
	return (data->wl.c.date << 24) + (data->wl.c.cost << 16) + (data->wl.c.selectivity << 8) + data->wl.c.depth;
#endif
}

/**
 * @brief update an hash table item.
 *
 * This is done when the level is the same as the previous storage.
 * Best moves & bound scores are updated, other data are untouched.
 *
 * @param data Hash Data.
 * @param storedata.data.cost Search cost (log2(node count)).
 * @param storedata.alpha Alpha bound.
 * @param storedata.beta Beta bound.
 * @param storedata.score Best score.
 * @param storedata.move Best move.
 */
static void data_update(HashData *data, HashStoreData *storedata)
{
	int score = storedata->score;

	if (score < storedata->beta && score < data->upper) data->upper = (signed char) score;
	if (score > storedata->alpha && score > data->lower) data->lower = (signed char) score;
<<<<<<< HEAD
<<<<<<< HEAD
	if ((score > storedata->alpha || score == SCORE_MIN) && data->move[0] != storedata->data.move[0]) {
		data->move[1] = data->move[0];
		data->move[0] = storedata->data.move[0];
	}
	data->wl.c.cost = (unsigned char) MAX(storedata->data.wl.c.cost, data->wl.c.cost);
=======
	if ((score > storedata->alpha || score == SCORE_MIN) && data->move[0] != storedata->move) {
=======
	if ((score > storedata->alpha || score == SCORE_MIN) && data->move[0] != storedata->data.move[0]) {
>>>>>>> a556e46 (HashData and HashStoreData rearranged, TYPE_PUNING now uses union)
		data->move[1] = data->move[0];
		data->move[0] = storedata->data.move[0];
	}
<<<<<<< HEAD
	data->cost = (unsigned char) MAX(storedata->data.cost, data->cost);
>>>>>>> d1c50ef (Structured hash_store parameters; AVXLASTFLIP changed to opt-in)
=======
	data->wl.c.cost = (unsigned char) MAX(storedata->data.wl.c.cost, data->wl.c.cost);
>>>>>>> a556e46 (HashData and HashStoreData rearranged, TYPE_PUNING now uses union)
	HASH_STATS(++statistics.n_hash_update;)
}

/**
 * @brief Upgrade an hash table data item.
 *
 * Upgrade is done when the search level increases.
 * Best moves are updated, others data are reset to new value.
 *
 * @param data Hash Data.
 * @param storedata.data.depth Search depth.
 * @param storedata.data.selectivity Search selectivity.
 * @param storedata.data.cost Search cost (log2(node count)).
 * @param storedata.alpha Alpha bound.
 * @param storedata.beta Beta bound.
 * @param storedata.score Best score.
 * @param storedata.move Best move.
 */
static void data_upgrade(HashData *data, HashStoreData *storedata)
{
	int score = storedata->score;

	if (score < storedata->beta) data->upper = (signed char) score; else data->upper = SCORE_MAX;
	if (score > storedata->alpha) data->lower = (signed char) score; else data->lower = SCORE_MIN;
<<<<<<< HEAD
<<<<<<< HEAD
	if ((score > storedata->alpha || score == SCORE_MIN) && data->move[0] != storedata->data.move[0]) {
		data->move[1] = data->move[0];
		data->move[0] = storedata->data.move[0];
	}
	data->wl.us.selectivity_depth = storedata->data.wl.us.selectivity_depth;
	data->wl.c.cost = (unsigned char) MAX(storedata->data.wl.c.cost, data->wl.c.cost);  // this may not work well in parallel search.
=======
	if ((score > storedata->alpha || score == SCORE_MIN) && data->move[0] != storedata->move) {
=======
	if ((score > storedata->alpha || score == SCORE_MIN) && data->move[0] != storedata->data.move[0]) {
>>>>>>> a556e46 (HashData and HashStoreData rearranged, TYPE_PUNING now uses union)
		data->move[1] = data->move[0];
		data->move[0] = storedata->data.move[0];
	}
<<<<<<< HEAD
	data->depth = storedata->data.depth;
	data->selectivity = storedata->data.selectivity;
	data->cost = (unsigned char) MAX(storedata->data.cost, data->cost);  // this may not work well in parallel search.
>>>>>>> d1c50ef (Structured hash_store parameters; AVXLASTFLIP changed to opt-in)
=======
	data->wl.us.selectivity_depth = storedata->data.wl.us.selectivity_depth;
	data->wl.c.cost = (unsigned char) MAX(storedata->data.wl.c.cost, data->wl.c.cost);  // this may not work well in parallel search.
>>>>>>> a556e46 (HashData and HashStoreData rearranged, TYPE_PUNING now uses union)
	HASH_STATS(++statistics.n_hash_upgrade;)

	assert(data->upper >= data->lower);
}

/**
 * @brief Set an hash table data item.
 *
 * @param data Hash Data.
 * @param storedata.data.date Search Date.
 * @param storedata.data.depth Search depth.
 * @param storedata.data.selectivity Search selectivity.
 * @param storedata.data.cost Search cost (log2(node count)).
 * @param storedata.alpha Alpha bound.
 * @param storedata.beta Beta bound.
 * @param storedata.score Best score.
 * @param storedata.move Best move.
 */
static void data_new(HashData *data, HashStoreData *storedata)
{
	int score = storedata->score;

<<<<<<< HEAD
<<<<<<< HEAD
=======
>>>>>>> a556e46 (HashData and HashStoreData rearranged, TYPE_PUNING now uses union)
	if (score < storedata->beta) data->upper = (signed char) score; else data->upper = SCORE_MAX;
	if (score > storedata->alpha) data->lower = (signed char) score; else data->lower = SCORE_MIN;
	if (score > storedata->alpha || score == SCORE_MIN) data->move[0] = storedata->data.move[0];
	else data->move[0] = NOMOVE;
	data->move[1] = NOMOVE;
	data->wl = storedata->data.wl;
<<<<<<< HEAD
=======
	if (score < storedata->beta) storedata->data.upper = (signed char) score; else storedata->data.upper = SCORE_MAX;
	if (score > storedata->alpha) storedata->data.lower = (signed char) score; else storedata->data.lower = SCORE_MIN;
	if (score > storedata->alpha || score == SCORE_MIN) storedata->data.move[0] = storedata->move;
	else storedata->data.move[0] = NOMOVE;
	storedata->data.move[1] = NOMOVE;
	*data = storedata->data;
>>>>>>> d1c50ef (Structured hash_store parameters; AVXLASTFLIP changed to opt-in)
=======
>>>>>>> a556e46 (HashData and HashStoreData rearranged, TYPE_PUNING now uses union)
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
 * @param storedata.data.date Hash date.
 * @param storedata.data.depth Search depth.
 * @param storedata.data.selectivity Search selectivity.
 * @param storedata.data.cost Search cost.
 * @param storedata.alpha Alpha bound.
 * @param storedata.beta Beta bound.
 * @param storedata.score Best score.
 * @param storedata.move Best move.
 */
<<<<<<< HEAD
<<<<<<< HEAD
static void hash_new(Hash *hash, HashLock *lock, const Board *board, HashStoreData *storedata)
=======
static void hash_new(Hash *hash, HashLock *lock, const Board* board, HashStoreData *storedata)
>>>>>>> d1c50ef (Structured hash_store parameters; AVXLASTFLIP changed to opt-in)
=======
static void vectorcall hash_new(Hash *hash, HashLock *lock, HBOARD board, HashStoreData *storedata)
>>>>>>> e88638e (add vectorcall interface to hash functions)
{
	spin_lock(lock);
	HASH_STATS(if (date == hash->data.date) ++statistics.n_hash_remove;)
	HASH_STATS(++statistics.n_hash_new;)
	HASH_COLLISIONS(hash->key = storedata->hash_code;)
	store_hboard(&hash->board, board);
	data_new(&hash->data, storedata);
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
 * @param board Bitboard.
 * @param storedata.data.date Hash date.
 * @param storedata.data.depth Search depth.
 * @param storedata.data.selectivity Search selectivity.
 * @param storedata.data.cost Search cost.
 * @param storedata.data.lower Lower score bound.
 * @param storedata.data.upper Upper score bound.
 * @param storedata.move Best move.
 */
static void vectorcall hash_set(Hash *hash, HashLock *lock, HBOARD board, HashStoreData *storedata)
{
	storedata->data.move[1] = NOMOVE;
	spin_lock(lock);
	HASH_STATS(if (date == hash->data.date) ++statistics.n_hash_remove;)
	HASH_STATS(++statistics.n_hash_new;)
	HASH_COLLISIONS(hash->key = storedata->hash_code;)
<<<<<<< HEAD
	hash->board = *board;
<<<<<<< HEAD
<<<<<<< HEAD
=======
	storedata->data.move[0] = storedata->move;
	storedata->data.move[1] = NOMOVE;
>>>>>>> d1c50ef (Structured hash_store parameters; AVXLASTFLIP changed to opt-in)
=======
>>>>>>> a556e46 (HashData and HashStoreData rearranged, TYPE_PUNING now uses union)
=======
	store_hboard(&hash->board, board);
>>>>>>> e88638e (add vectorcall interface to hash functions)
	hash->data = storedata->data;
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
 * @param board Bitboard.
 * @param storedata.data.date Hash date.
 * @param storedata.data.depth Search depth.
 * @param storedata.data.selectivity Search selectivity.
 * @param storedata.data.cost Hash Cost (log2(node count)).
 * @param storedata.alpha Alpha bound.
 * @param storedata.beta Beta bound.
 * @param storedata.score Best score.
 * @param storedata.move Best move.
 * @return true if an entry has been updated, false otherwise.
 */
static bool vectorcall hash_update(Hash *hash, HashLock *lock, HBOARD board, HashStoreData *storedata)
{
	bool ok = false;
<<<<<<< HEAD
<<<<<<< HEAD
=======
	HashData *const data = &hash->data;
>>>>>>> d1c50ef (Structured hash_store parameters; AVXLASTFLIP changed to opt-in)
=======
>>>>>>> a556e46 (HashData and HashStoreData rearranged, TYPE_PUNING now uses union)

	if (hboard_equal(board, &hash->board)) {
		spin_lock(lock);
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
=======
>>>>>>> de58f52 (AVX2 board_equal; delayed hash lock code)
		if (board_equal(&hash->board, board)) {
=======
		if (hboard_equal(board, &hash->board)) {
>>>>>>> e88638e (add vectorcall interface to hash functions)
			if (hash->data.wl.us.selectivity_depth == storedata->data.wl.us.selectivity_depth)
				data_update(&hash->data, storedata);
			else	data_upgrade(&hash->data, storedata);
			hash->data.wl.c.date = storedata->data.wl.c.date;
			if (hash->data.lower > hash->data.upper) { // reset the hash-table...
				data_new(&hash->data, storedata);
=======
		if (hash->board.player == board->player && hash->board.opponent == board->opponent) {
<<<<<<< HEAD
			if (data->selectivity == storedata->data.selectivity && data->depth == storedata->data.depth)
				data_update(data, storedata);
			else	data_upgrade(data, storedata);
			data->date = storedata->data.date;
			if (data->lower > data->upper) { // reset the hash-table...
				data_new(data, storedata);
>>>>>>> d1c50ef (Structured hash_store parameters; AVXLASTFLIP changed to opt-in)
=======
			if (hash->data.wl.us.selectivity_depth == storedata->data.wl.us.selectivity_depth)
				data_update(&hash->data, storedata);
			else	data_upgrade(&hash->data, storedata);
			hash->data.wl.c.date = storedata->data.wl.c.date;
			if (hash->data.lower > hash->data.upper) { // reset the hash-table...
				data_new(&hash->data, storedata);
>>>>>>> a556e46 (HashData and HashStoreData rearranged, TYPE_PUNING now uses union)
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
 * @param board Bitboard.
 * @param storedata.data.date Hash date.
 * @param storedata.data.depth Search depth.
 * @param storedata.data.selectivity Search selectivity.
 * @param storedata.data.cost Hash Cost (log2(node count)).
 * @param storedata.alpha Alpha bound.
 * @param storedata.beta Beta bound.
 * @param storedata.score Best score.
 * @param storedata.move Best move.
 * @return true if an entry has been replaced, false otherwise.
 */
static bool vectorcall hash_replace(Hash *hash, HashLock *lock, HBOARD board, HashStoreData *storedata)
{
	bool ok = false;

<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
	if (board_equal(&hash->board, board)) {
=======
	if (hash->board.player == board->player && hash->board.opponent == board->opponent) {
>>>>>>> 0a166fd (Remove 1 element array coding style)
		spin_lock(lock);
<<<<<<< HEAD
		if (board_equal(&hash->board, board)) {
=======
		if (hash->board.player == board->player && hash->board.opponent == board->opponent) {
>>>>>>> d1c50ef (Structured hash_store parameters; AVXLASTFLIP changed to opt-in)
=======
	if (board_equal(&hash->board, board)) {
		spin_lock(lock);
		if (board_equal(&hash->board, board)) {
>>>>>>> de58f52 (AVX2 board_equal; delayed hash lock code)
=======
	if (hboard_equal(board, &hash->board)) {
		spin_lock(lock);
		if (hboard_equal(board, &hash->board)) {
>>>>>>> e88638e (add vectorcall interface to hash functions)
			data_new(&hash->data, storedata);
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
 * @param board Bitboard.
 * @param storedata.data.date Hash date.
 * @param storedata.data.depth Search depth.
 * @param storedata.data.selectivity Search selectivity.
 * @param storedata.data.lower Lower score bound.
 * @param storedata.data.upper Upper score bound.
 * @param storedata.move Best move.
 */
static bool vectorcall hash_reset(Hash *hash, HashLock *lock, HBOARD board, HashStoreData *storedata)
{
	bool ok = false;
<<<<<<< HEAD
<<<<<<< HEAD

	if (hboard_equal(board, &hash->board)) {
		spin_lock(lock);
		if (hboard_equal(board, &hash->board)) {
			if (hash->data.wl.us.selectivity_depth == storedata->data.wl.us.selectivity_depth) {
				if (hash->data.lower < storedata->data.lower) hash->data.lower = storedata->data.lower;
				if (hash->data.upper > storedata->data.upper) hash->data.upper = storedata->data.upper;
			} else {
				hash->data.lower = storedata->data.lower;
				hash->data.upper = storedata->data.upper;
			}
			hash->data.wl = storedata->data.wl;
			if (storedata->data.move[0] != NOMOVE) {
				// if (hash->data.move[0] != storedata->data.move[0]) {
					hash->data.move[1] = hash->data.move[0];
					hash->data.move[0] = storedata->data.move[0];
				// } else {
				//	hash->data.move[1] = storedata->data.move[0];
				// }
=======
	HashData *const data = &hash->data;
=======
>>>>>>> a556e46 (HashData and HashStoreData rearranged, TYPE_PUNING now uses union)

	if (hash->board.player == board->player && hash->board.opponent == board->opponent) {
		spin_lock(lock);
		if (hash->board.player == board->player && hash->board.opponent == board->opponent) {
			if (hash->data.wl.us.selectivity_depth == storedata->data.wl.us.selectivity_depth) {
				if (hash->data.lower < storedata->data.lower) hash->data.lower = storedata->data.lower;
				if (hash->data.upper > storedata->data.upper) hash->data.upper = storedata->data.upper;
			} else {
				hash->data.lower = storedata->data.lower;
				hash->data.upper = storedata->data.upper;
			}
<<<<<<< HEAD
			data->cost = 0;
			data->date = storedata->data.date;
			if (storedata->move != NOMOVE) {
				if (data->move[0] != storedata->move) {
					data->move[1] = data->move[0];
					data->move[0] = storedata->move;
				} else {
					data->move[1] = storedata->move;
				}
>>>>>>> d1c50ef (Structured hash_store parameters; AVXLASTFLIP changed to opt-in)
=======
			hash->data.wl = storedata->data.wl;
			if (storedata->data.move[0] != NOMOVE) {
				// if (hash->data.move[0] != storedata->data.move[0]) {
					hash->data.move[1] = hash->data.move[0];
					hash->data.move[0] = storedata->data.move[0];
				// } else {
				//	hash->data.move[1] = storedata->data.move[0];
				// }
>>>>>>> a556e46 (HashData and HashStoreData rearranged, TYPE_PUNING now uses union)
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
 * @param board Bitboard.
 * @param storedata.data.depth Search depth.
 * @param storedata.data.selectivity Selectivity level.
 * @param storedata.data.lower Alpha bound.
 * @param storedata.data.upper Beta bound.
 * @param storedata.move best move.
 */
void vectorcall hash_feed(HashTable *hash_table, HBOARD board, const unsigned long long hash_code, HashStoreData *storedata)
{
	Hash *hash, *worst;
	HashLock *lock; 
	int i;

<<<<<<< HEAD
<<<<<<< HEAD
	storedata->data.wl.c.date = hash_table->date ? hash_table->date : 1;
	storedata->data.wl.c.cost = 0;
=======

	storedata->data.date = hash_table->date ? hash_table->date : 1;
>>>>>>> d1c50ef (Structured hash_store parameters; AVXLASTFLIP changed to opt-in)
=======
	storedata->data.wl.c.date = hash_table->date ? hash_table->date : 1;
	storedata->data.wl.c.cost = 0;
>>>>>>> a556e46 (HashData and HashStoreData rearranged, TYPE_PUNING now uses union)

	worst = hash = hash_table->hash + (hash_code & hash_table->hash_mask);
	lock = hash_table->lock + (hash_code & hash_table->lock_mask);
	if (hash_reset(hash, lock, board, storedata)) return;

	for (i = 1; i < HASH_N_WAY; ++i) {
		++hash;
		if (hash_reset(hash, lock, board, storedata)) return;
		if (writeable_level(&worst->data) > writeable_level(&hash->data)) {
			worst = hash;
		}
	}

	// new entry
	HASH_COLLISIONS(storedata->hash_code = hash_code;)
<<<<<<< HEAD
<<<<<<< HEAD
=======
	storedata->data.cost = 0;
>>>>>>> d1c50ef (Structured hash_store parameters; AVXLASTFLIP changed to opt-in)
=======
>>>>>>> a556e46 (HashData and HashStoreData rearranged, TYPE_PUNING now uses union)
	hash_set(worst, lock, board, storedata);
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
 * @param board Bitboard.
 * @param hash_code  Hash code of an othello board.
 * @param storedata.data.depth      Search depth.
 * @param storedata.data.selectivity   Search selectivity.
 * @param storedata.data.cost       Search cost (i.e. log2(node count)).
 * @param storedata.alpha      Alpha bound when calling the alphabeta function.
 * @param storedata.beta       Beta bound when calling the alphabeta function.
 * @param storedata.score      Best score found.
 * @param storedata.move       Best move found.
 */
void vectorcall hash_store(HashTable *hash_table, HBOARD board, const unsigned long long hash_code, HashStoreData *storedata)
{
	int i;
	Hash *worst, *hash;
	HashLock *lock;

	worst = hash = hash_table->hash + (hash_code & hash_table->hash_mask);
	lock = hash_table->lock + (hash_code & hash_table->lock_mask);
<<<<<<< HEAD
<<<<<<< HEAD
	storedata->data.wl.c.date = hash_table->date;
=======
	storedata->data.date = hash_table->date;
>>>>>>> d1c50ef (Structured hash_store parameters; AVXLASTFLIP changed to opt-in)
=======
	storedata->data.wl.c.date = hash_table->date;
>>>>>>> a556e46 (HashData and HashStoreData rearranged, TYPE_PUNING now uses union)
	if (hash_update(hash, lock, board, storedata)) return;

	for (i = 1; i < HASH_N_WAY; ++i) {
		++hash;
		if (hash_update(hash, lock, board, storedata)) return;
		if (writeable_level(&worst->data) > writeable_level(&hash->data)) {
			worst = hash;
		}
	}

	HASH_COLLISIONS(storedata->hash_code = hash_code;)
	hash_new(worst, lock, board, storedata);
}

/**
 * @brief Store an hashtable item.
 *
 * Does the same as hash_store() except it always store the current search state
 *
 * @param hash_table Hash table to update.
 * @param board Bitboard.
 * @param hash_code  Hash code of an othello board.
 * @param storedata.data.depth      Search depth.
 * @param storedata.data.selectivity   Search selectivity.
 * @param storedata.data.cost       Search cost (i.e. log2(node count)).
 * @param storedata.alpha      Alpha bound when calling the alphabeta function.
 * @param storedata.beta       Beta bound when calling the alphabeta function.
 * @param storedata.score      Best score found.
 * @param storedata.move       Best move found.
 */
void vectorcall hash_force(HashTable *hash_table, HBOARD board, const unsigned long long hash_code, HashStoreData *storedata)
{
	int i;
	Hash *worst, *hash;
	HashLock *lock;

	worst = hash = hash_table->hash + (hash_code & hash_table->hash_mask);
	lock = hash_table->lock + (hash_code & hash_table->lock_mask);
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
	storedata->data.wl.c.date = hash_table->date;
	if (hash_replace(hash, lock, board, storedata)) return;
=======
	if (hash_replace(hash, lock, board, hash_table->date, depth, selectivity, cost, alpha, beta, score, move)) return;
>>>>>>> 0a166fd (Remove 1 element array coding style)
=======
	storedata->data.date = hash_table->date;
=======
	storedata->data.wl.c.date = hash_table->date;
>>>>>>> a556e46 (HashData and HashStoreData rearranged, TYPE_PUNING now uses union)
	if (hash_replace(hash, lock, board, storedata)) return;
>>>>>>> d1c50ef (Structured hash_store parameters; AVXLASTFLIP changed to opt-in)

	for (i = 1; i < HASH_N_WAY; ++i) {
		++hash;
		if (hash_replace(hash, lock, board, storedata)) return;
		if (writeable_level(&worst->data) > writeable_level(&hash->data)) {
			worst = hash;
		}
	}

<<<<<<< HEAD
<<<<<<< HEAD
	HASH_COLLISIONS(storedata->hash_code = hash_code;)
	hash_new(worst, lock, board, storedata);
=======
#if (HASH_COLLISIONS(1)+0) 
	hash_new(worst, lock, hash_code, board, hash_table->date, depth, selectivity, cost, alpha, beta, score, move);
#else 
	hash_new(worst, lock, board, hash_table->date, depth, selectivity, cost, alpha, beta, score, move);
#endif
>>>>>>> 0a166fd (Remove 1 element array coding style)
=======
	HASH_COLLISIONS(storedata->hash_code = hash_code;)
	hash_new(worst, lock, board, storedata);
>>>>>>> d1c50ef (Structured hash_store parameters; AVXLASTFLIP changed to opt-in)
}

/**
 * @brief Find an hash table entry according to the evaluated board hash codes.
 *
 * @param hash_table Hash table.
 * @param board Bitboard.
 * @param hash_code Hash code of an othello board.
 * @param data Output hash data.
 * @return True the board was found, false otherwise.
 */
bool vectorcall hash_get(HashTable *hash_table, HBOARD board, const unsigned long long hash_code, HashData *data)
{
	int i;
	Hash *hash;
	HashLock *lock;
	bool ok = false;

	HASH_STATS(++statistics.n_hash_search;)
	HASH_COLLISIONS(++statistics.n_hash_n;)
	hash = hash_table->hash + (hash_code & hash_table->hash_mask);
	for (i = 0; i < HASH_N_WAY; ++i) {
		HASH_COLLISIONS(if (hash->key == hash_code) {)
		HASH_COLLISIONS(	lock = hash_table->lock + (hash_code & hash_table->lock_mask);)
		HASH_COLLISIONS(	spin_lock(lock);)
<<<<<<< HEAD
<<<<<<< HEAD
		HASH_COLLISIONS(	if (hash->key == hash_code && !vboard_equal(board, &hash->board)) {)
=======
		HASH_COLLISIONS(	if (hash->key == hash_code && !board_equal(&hash->board, board)) {)
>>>>>>> de58f52 (AVX2 board_equal; delayed hash lock code)
=======
		HASH_COLLISIONS(	if (hash->key == hash_code && !vboard_equal(board, &hash->board)) {)
>>>>>>> 7bd8076 (vboard opt using union V2DI; MSVC can assign it to XMM)
		HASH_COLLISIONS(		++statistics.n_hash_collision;)
		HASH_COLLISIONS(		printf("key = %llu\n", hash_code);)
		HASH_COLLISIONS(		board_print(board, WHITE, stdout);)
		HASH_COLLISIONS(		board_print(&hash->board, WHITE, stdout);)
		HASH_COLLISIONS(	})
		HASH_COLLISIONS(	spin_unlock(lock);)
		HASH_COLLISIONS(})
		if (hboard_equal(board, &hash->board)) {
			lock = hash_table->lock + (hash_code & hash_table->lock_mask);
			spin_lock(lock);
			if (hboard_equal(board, &hash->board)) {
				*data = hash->data;
				HASH_STATS(++statistics.n_hash_found;)
				hash->data.wl.c.date = hash_table->date;
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
 * @brief Find an hash table entry from the board.
 *
 * @param hash_table Hash table.
 * @param board Bitboard.
 * @param data Output hash data.
 * @return True the board was found, false otherwise.
 */
bool hash_get_from_board(HashTable *hash_table, HBOARD board, HashData *data)
{
	return hash_get(hash_table, board, vboard_get_hash_code(board), data);
}

/**
 * @brief Erase an hash table entry.
 *
 * @param hash_table Hash table.
 * @param board Bitboard.
 * @param hash_code Hash code of an othello board.
 * @param move Move to exclude.
 */
void vectorcall hash_exclude_move(HashTable *hash_table, HBOARD board, const unsigned long long hash_code, const int move)
{
	int i;
	Hash *hash;
	HashLock *lock;

	hash = hash_table->hash + (hash_code & hash_table->hash_mask);
	for (i = 0; i < HASH_N_WAY; ++i) {
		if (hboard_equal(board, &hash->board)) {
			lock = hash_table->lock + (hash_code & hash_table->lock_mask);
			spin_lock(lock);
			if (hboard_equal(board, &hash->board)) {
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
<<<<<<< HEAD
<<<<<<< HEAD
	unsigned int i, imax = src->hash_mask + HASH_N_WAY;
	Hash *pSrc = src->hash, *pDest = dest->hash;
=======
	unsigned int i;
>>>>>>> 0a166fd (Remove 1 element array coding style)
=======
	unsigned int i, imax = src->hash_mask + HASH_N_WAY;
	Hash *pSrc = src->hash, *pDest = dest->hash;
>>>>>>> 11a54a6 (Revise get_corner_stability and hash_cleanup)

	assert(src->hash_mask == dest->hash_mask);
	info("<hash copy>\n");
	for (i = 0; i <= imax; ++i) {
		*pDest++ = *pSrc++;
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

	fprintf(f, "moves = %s, ", move_to_string(data->move[0], WHITE, s_move));
	fprintf(f, "%s ; ", move_to_string(data->move[1], WHITE, s_move));
	fprintf(f, "score = [%+02d, %+02d] ; ", data->lower, data->upper);
<<<<<<< HEAD
<<<<<<< HEAD
	fprintf(f, "level = %2d:%2d:%2d@%3d%%", data->wl.c.date, data->wl.c.cost, data->wl.c.depth, selectivity_table[data->wl.c.selectivity].percent);
=======
	fprintf(f, "level = %2d:%2d:%2d@%3d%%", data->date, data->cost, data->depth, selectivity_table[data->selectivity].percent);
>>>>>>> 0a166fd (Remove 1 element array coding style)
=======
	fprintf(f, "level = %2d:%2d:%2d@%3d%%", data->wl.c.date, data->wl.c.cost, data->wl.c.depth, selectivity_table[data->wl.c.selectivity].percent);
>>>>>>> a556e46 (HashData and HashStoreData rearranged, TYPE_PUNING now uses union)
}
