/**
 * @file perft.c
 *
 * @brief Move generator test.
 *
 * @date 1998 - 2024
 * @author Richard Delorme
 * @version 4.6
 */

#include "bit.h"
#include "board.h"
#include "crc32c.h"
#include "move.h"
#include "hash.h"
#include "options.h"
#include "settings.h"
#include "util.h"
#include "perft.h"

#include <inttypes.h>
#include <math.h>
#include <stdint.h>
#include <stdlib.h>

/**
 * Gathered statistiscs
 */
typedef struct {
	uint64_t n_moves;  /*< number of moves */
	uint64_t n_draws;  /*< number of draws */
	uint64_t n_losses; /*< number of losses */
	uint64_t n_wins;   /*< number of wins */
	uint64_t n_passes; /*< number of passes */
	uint32_t min_mobility;   /*< min mobility */
	uint32_t max_mobility;   /*< max mobility */
} GameStatistics;

/** initial statistics*/
const GameStatistics GAME_STATISTICS_INIT = {0ULL, 0ULL, 0ULL, 0ULL, 0ULL, 64, 0};

/**
 * @brief Accumulate statistics: add local data to global ones.
 * @param global Global statistics.
 * @param local Local statistics.
 */
static inline void game_statistics_cumulate(GameStatistics *global, const GameStatistics *local)
{
	global->n_moves += local->n_moves;
	global->n_draws += local->n_draws;
	global->n_losses += local->n_losses;
	global->n_wins += local->n_wins;
	global->n_passes += local->n_passes;
	if (global->min_mobility > local->min_mobility) global->min_mobility = local->min_mobility;
	if (global->max_mobility < local->max_mobility) global->max_mobility = local->max_mobility;
}


/**
 * @brief Move generator performance test function.
 *
 * @param board
 * @param depth
 * @param global_stats statistics
 */
static void count_game(const Board *board, const int depth, GameStatistics *global_stats)
{
	GameStatistics stats = GAME_STATISTICS_INIT;
	uint64_t moves;
	int x;
	Board next;

	if (depth == 1) {
		moves = board_get_moves(board);
		stats.n_moves = stats.max_mobility = stats.min_mobility = bit_count(moves);
		if (moves == 0) {
			if (can_move(board->opponent, board->player)) {
				stats.n_passes = 1;
			} else {
				const int n_player = bit_count(board->player);
				const int n_opponent = bit_count(board->opponent);
				if (n_player > n_opponent) stats.n_wins = 1;
				else if (n_player == n_opponent) stats.n_draws = 1;
				else stats.n_losses = 1;
			}
		}
	} else {
		moves = board_get_moves(board);
		if (moves) {
			foreach_bit (x, moves) {
				board_next(board, x, &next);
				count_game(&next, depth - 1, &stats);
			}
		} else {
			board_next(board, PASS, &next);
			if (can_move(next.player, next.opponent)) {
				count_game(&next, depth - 1, &stats);
			}
		}
	}
	game_statistics_cumulate(global_stats, &stats);
}

/**
 * @brief Move generator performance test
 *
 * @param board
 * @param depth
 */
void count_games(const Board *board, const int depth)
{
	int i;
	uint64_t t, n;
	GameStatistics stats;

	board_print(board, BLACK, stdout);
	puts("\n  ply           moves        passes          wins         draws        losses    mobility        time   speed");
	puts("------------------------------------------------------------------------------------------------------------------");
	n = 1;
	for (i = 1; i <= depth; ++i) {
		stats = GAME_STATISTICS_INIT;
		t = -cpu_clock();
		count_game(board, i, &stats);
		t += cpu_clock();
		printf("  %2d, %15" PRIu64 ", %12" PRIu64 ", %12" PRIu64 ", %12" PRIu64 ", %12" PRIu64 ", ", i, stats.n_moves + stats.n_passes, stats.n_passes, stats.n_wins, stats.n_draws, stats.n_losses);
		printf("  %2d - %2d, ", stats.min_mobility, stats.max_mobility);
		n += stats.n_moves + stats.n_passes;
		time_print(t, true, stdout);	printf(", ");
		print_scientific(n / (0.001 * t + 0.001), "N/s\n", stdout);
		if (stats.n_moves + stats.n_passes == 0) break;
	}
	printf("Total %12" PRIu64 "\n", n);
	puts("------------------------------------------------------------------------------------------------------------------");
}

/**
 * @brief Estimate move counts from a single game.
 *
 * @param board Board
 * @param depth Depth.
 * @param r random generator
 * @param n node counter array.
 */

static void estimate_game(const Board *board, const int depth, Random *r, double *n)
{
	uint64_t moves;
	int x, i, j, k;
	Board next;

	moves = board_get_moves(board);
	i = bit_count(moves);
	if (i == 0 && !can_move(board->opponent, board->player)) {
		n[depth] = 0;
	} else {
		n[depth] = 1;
		if (i == 0) {
			board_next(board, PASS, &next);
			estimate_game(&next, depth + 1, r, n);
		} else {
			j = random_get(r) % i;
			foreach_bit (x, moves) {
				if (j == 0) {
					board_next(board, x, &next);
					estimate_game(&next, depth + 1, r, n);
					for (k = depth; n[k]; ++k) n[k] *= i;
					break;
				} else --j;
			}
		}
	}
}

/**
 * @brief Move estimate games
 *
 * @param board
 * @param n Number of trials
 */
void estimate_games(const Board *board, const int64_t n)
{
	int i, j;
	uint64_t t;
	double x[128], m[128], s[128], em[128], es[128], en[128];
	double M, S, EM, ES;
	Random r;

	random_seed(&r, real_clock());
	M = S = EM = ES = 0.0;
	for (i = 0; i < 128; ++i) m[i] = s[i] = 0.0;
	for (i = 0; i < 128; ++i) em[i] = es[i] = en[i] = 0.0;

	board_print(board, BLACK, stdout);

	t = -cpu_clock();
	for (j = 1; j <= n; ++j) {
		for (i = 0; i < 128; ++i) x[i] = 0.0;
		estimate_game(board, 1, &r, x);
		for (i = 1; x[i]; ++i) {
			m[i] += x[i]; s[i] += x[i] * x[i];
			M += x[i]; S += x[i] * x[i];
		}
		{
			em[i] += x[i - 1]; es[i] += x[i - 1] * x[i - 1];
			EM += x[i - 1]; ES += x[i - 1] * x[i - 1];
			en[i]++;
		}
	}
	t += cpu_clock();

	for (i = 1; m[i] || en[i]; ++i) {
		m[i] /= n;
		s[i] = sqrt((s[i] / n - m[i] * m[i])/n);
		printf("%2d: %e +/- %e; ", i, m[i], s[i]);

		if (en[i]) {
			em[i] /= n;
			es[i] = sqrt((es[i] / n - em[i] * em[i])/n);
			printf("%e +/- %e;", em[i], es[i]);
		}
		putchar('\n');
	}
	M /= n;
	S = sqrt((S / n - M * M)/n);
	EM /= n;
	ES = sqrt((ES / n - EM * EM)/n);
	printf("Total %e +/- %e: %e +/- %e en", M, S, EM, ES);
	time_print(t, false, stdout); printf("\n");
}

/**
 * @brief Estimate move counts from a single game.
 *
 * @param board Board
 * @param ply Ply.
 * @param r random generator.
 * @param move move array.
 * @param max_mobility Highest mobility count.
 * @param max_empties Stage of highest mobility.
 * @param n test counter.
 */

static void test_mobility(const Board *board, const int ply, Random *r, int *move, int *max_mobility, int *max_empties, const uint64_t n)
{
	uint64_t moves;
	int x, i, k, e;
	Board next;

	e = board_count_empties(board);

	if (e > *max_mobility) {
		moves = board_get_moves(board);
		if (moves) {
			i = bit_count(moves);
			if (i > *max_mobility || (i == *max_mobility && e > *max_empties)) {
				*max_mobility = i;
				*max_empties = e;
				printf("\n after %" PRIu64 " trials:\n\n", n);
				board_print(board, ply % 2, stdout);
				for (k = 1; k < ply; ++k) {
					move_print(move[k], k % 2, stdout);
					putchar(' ');
				}
				putchar('\n');
			}
			moves &= 0x007e7e7e7e7e7e00ULL;
			if (moves) {
				i = random_get(r) % bit_count(moves);
				foreach_bit (x, moves) {
					if (i-- == 0) {
						move[ply] = x;
						board_next(board, x, &next);
						test_mobility(&next, ply + 1, r, move, max_mobility, max_empties, n);
						break;
					}
				}
			}
		} else {
			if (can_move(board->opponent, board->player)) {
				move[ply] = PASS;
				board_next(board, PASS, &next);
				test_mobility(&next, ply + 1, r, move, max_mobility, max_empties, n);
			}
		}
	}
}

/**
 * @brief Move estimate games
 *
 * @param board
 * @param t time to test
 */
void seek_highest_mobility(const Board *board, const uint64_t t)
{
	int max_mobility = get_mobility(board->player, board->opponent);
	int max_empties = board_count_empties(board);
	const int64_t t_max = t * 1000 + cpu_clock();
	int i;
	uint64_t n = 0;
	const int bucket = 10000;
	int x[128];
	Random r;

	random_seed(&r, real_clock());

	while (cpu_clock() < t_max) {
		i = bucket;
		while (i-- > 0) {
			test_mobility(board, 1, &r, x, &max_mobility, &max_empties, ++n);
		}
	}
}

/**
 * Hash entry;
 */
typedef struct {
	Board board; /**< board */
	GameStatistics stats;    /**< statistics */
	int depth;               /**< depth */
} GameHash;

/** Hash entry initial value */
const GameHash GAME_HASH_INIT = {{0ULL, 0ULL}, {0ULL, 0ULL, 0ULL, 0ULL, 0ULL, 64, 0}, 0};

/** HashTable */
typedef struct {
	GameHash *array;   /**< array of hash entries */
	uint64_t n_tries;  /**< n_tries */
	uint64_t n_hits;   /**< n_tries */
	int size;          /**< size */
	int mask;          /**< mask */
} GameHashTable;

/**
 * @brief Hash table initialisation
 *
 * @param hash Hash table.
 * @param bitsize Hash table size (as log2(size)).
 */
static void gamehash_init(GameHashTable *hash, int bitsize)
{
	int i;

	hash->size = (1 << bitsize) + 3;
	hash->mask = (1 << bitsize) - 1;
	hash->array = (GameHash*) aligned_alloc(64, adjust_size(64, hash->size * sizeof (GameHash)));
	if (hash->array == NULL) fatal_error("Cannot allocate perft hashtable.\n");
	for (i = 0; i < hash->size + 3; ++i) hash->array[i] = GAME_HASH_INIT;
	hash->n_tries = hash->n_hits = 0;
}

/**
 * @brief Hash table resource freeing.
 *
 * @param hash Hash table.
 */
static void gamehash_delete(GameHashTable *hash)
{
	free(hash->array);
}

/**
 * @brief Store a game position.
 *
 * @param  hash Hash table.
 * @param b position.
 * @param depth Depth.
 * @param stats position's statistics.
 */
static void gamehash_store(GameHashTable *hash, const Board *b, const int depth, const GameStatistics *stats)
{
	Board u;
	GameHash *i, *j;

	if (depth > 2) {
		board_unique(b, &u);
		i = j = hash->array + (board_get_hash_code(&u) & hash->mask);

		++j; if (i->stats.n_moves > j->stats.n_moves) i = j;
		++j; if (i->stats.n_moves > j->stats.n_moves) i = j;
		++j; if (i->stats.n_moves > j->stats.n_moves) i = j;
		i->board = u;
		i->stats = *stats;
		i->depth = depth;
	}
}

/**
 * @brief Seek for a position in the hash table.
 *
 * @param hash Hash table.
 * @param b position.
 * @param depth Depth.
 * @param stats position's statistics.
 * @return true if no position is found, false otherwise.
 */
static bool gamehash_fail(GameHashTable *hash, const Board *b, const int depth, GameStatistics *stats)
{
	Board u;
	GameHash *i, *j;

	if (depth > 2) {
		board_unique(b, &u);
		j = hash->array + (board_get_hash_code(&u) & hash->mask);
		++hash->n_tries;

		for (i = j; i < j + 4; ++i) {
			if (depth == i->depth && i->board.player == u.player && i->board.opponent == u.opponent) {
				*stats = i->stats;
				++hash->n_hits;
				return false;
			}
		}

	}

	return true;
}


/**
 * @brief Count games recursively.
 *
 * @param hash Hash table.
 * @param board position.
 * @param depth Depth.
 * @param global_stats Game's statistics.
 */
static void quick_count_game_6x6(GameHashTable *hash, const Board *board, const int depth, GameStatistics *global_stats)
{
	GameStatistics stats = GAME_STATISTICS_INIT;
	uint64_t moves;
	int x;
	Board next;

	if (depth == 1) {
		moves = get_moves_6x6(board->player, board->opponent);
		stats.n_moves = stats.max_mobility = stats.min_mobility = bit_count(moves);
		if (moves == 0) {
			if (can_move_6x6(board->opponent, board->player)) {
				stats.n_passes = 1;
			} else {
				const int n_player = bit_count(board->player);
				const int n_opponent = bit_count(board->opponent);
				if (n_player > n_opponent) stats.n_wins = 1;
				else if (n_player == n_opponent) stats.n_draws = 1;
				else stats.n_losses = 1;
			}
		}
	} else if (gamehash_fail(hash, board, depth, &stats)) {
		moves = get_moves_6x6(board->player, board->opponent);
		if (moves) {
			foreach_bit (x, moves) {
				board_next(board, x, &next);
				quick_count_game_6x6(hash, &next, depth - 1, &stats);
			}
		} else {
			board_next(board, PASS, &next);
			if (can_move_6x6(next.player, next.opponent)) {
				quick_count_game_6x6(hash, &next, depth - 1, &stats);
			}
		}
		gamehash_store(hash, board, depth, &stats);
	}
	game_statistics_cumulate(global_stats, &stats);
}

/**
 * @brief Count games recursively.
 *
 * @param  hash Hash table.
 * @param board position.
 * @param depth Depth.
 * @param global_stats Game's statistics.
 */
static void quick_count_game(GameHashTable *hash, const Board *board, const int depth, GameStatistics *global_stats)
{
	GameStatistics stats = GAME_STATISTICS_INIT;
	uint64_t moves;
	int x;
	Board next;

	if (depth == 1) {
		moves = board_get_moves(board);
		stats.n_moves = stats.max_mobility = stats.min_mobility = bit_count(moves);
		if (moves == 0) {
			if (can_move(board->opponent, board->player)) {
				stats.n_passes = 1;
			} else {
				const int n_player = bit_count(board->player);
				const int n_opponent = bit_count(board->opponent);
				if (n_player > n_opponent) stats.n_wins = 1;
				else if (n_player == n_opponent) stats.n_draws = 1;
				else stats.n_losses = 1;
			}
		}
	} else if (gamehash_fail(hash, board, depth, &stats)) {
		moves = board_get_moves(board);
		if (moves) {
			foreach_bit (x, moves) {
				board_next(board, x, &next);
				quick_count_game(hash, &next, depth - 1, &stats);
			}
		} else {
			board_next(board, PASS, &next);
			if (can_move(next.player, next.opponent)) {
				quick_count_game(hash, &next, depth - 1, &stats);
			}
		}
		gamehash_store(hash, board, depth, &stats);
	}
	game_statistics_cumulate(global_stats, &stats);
}

/**
 * @brief Count games.
 *
 * @param board position.
 * @param depth Depth.
 * @param size Size of the board (6 or 8).
 */
void quick_count_games(const Board *board, const int depth, const int size)
{
	int i;
	int64_t t;
	GameHashTable hash;
	GameStatistics stats;
	uint64_t n;

	board_print(board, BLACK, stdout);
	puts("\n  ply           moves        passes          wins         draws        losses    mobility        time   speed");
	puts("------------------------------------------------------------------------------------------------------------------");
	n = 1;
	for (i = 1; i <= depth; ++i) {
		gamehash_init(&hash, options.hash_table_size);
		stats = GAME_STATISTICS_INIT;
		t = -cpu_clock();
		if (size == 6) quick_count_game_6x6(&hash, board, i, &stats);
		else quick_count_game(&hash, board, i, &stats);
		t += cpu_clock();
		printf("  %2d, %15" PRIu64 ", %12" PRIu64 ", %12" PRIu64 ", %12" PRIu64 ", %12" PRIu64 ", ", i, stats.n_moves + stats.n_passes, stats.n_passes, stats.n_wins, stats.n_draws, stats.n_losses);
		printf("  %2d - %2d, ", stats.min_mobility, stats.max_mobility);
		time_print(t, true, stdout);	printf(", ");
		n += stats.n_moves + stats.n_passes;
		print_scientific(n / (0.001 * t + 0.001), "moves/s,", stdout);
		printf("  (h_tries = %" PRIu64 ", h_hits = %" PRIu64 ")\n", hash.n_tries, hash.n_hits);
		gamehash_delete(&hash);
		if (stats.n_moves + stats.n_passes == 0) break;
	}
	printf("Total %12" PRIu64 "\n", n);
	puts("------------------------------------------------------------------------------------------------------------------");
}


/**
 * Compact board 13 bytes
 */
#pragma pack(1)
typedef struct CBoard {
	uint8_t x[13];
} CBoard;
#pragma pack()

static void compact_board(const Board *b, CBoard *c) {
	int i, n = 0, x;

	i = 0;
	for (x = A1; x <= H8; ++x) {
		if (x % 5 == 0) n = 0;
		n = 3 * n + board_get_square_color(b, x);
		if (x % 5 == 4) c->x[i++] = n;
	}
	c->x[i] = n;
}


/**
 * Array of position.
 */
typedef struct PosArray {
	CBoard *item; /**< dynamic array */
	int n;       /**< number of items in the array */
	int size;    /**< capacity of the array */
} PosArray;

/**
 * @brief array initialisation.
 * @param array Array of positions.
 */
static void positionarray_init(PosArray *array)
{
	array->item = NULL;
	array->n = array->size = 0;
}

/**
 * @brief array supression.
 * @param array Array of positions.
 */
static void positionarray_delete(PosArray *array)
{
	free(array->item);
}

/**
 * @brief Append a position.
 * @param array Array of positions.
 * @param b Position.
 * @return true if a position is added to the array, false if it is already present.
 */
static bool positionarray_append(PosArray *array, const CBoard *b)
{
	int i;
	for (i = 0; i < array->n; ++i) {
		if (memcmp(array->item + i, b, 13) == 0) return false;
	}

	if (array->size < array->n + 1) {
		array->item = (CBoard*) realloc(array->item, (array->size = (array->size + 1)) * sizeof (CBoard));
		if (array->item == NULL) fatal_error("Cannot re-allocate board array.\n");
	}
	array->item[array->n++] = *b;
	return true;
}

/**
 * HashTable of shapes
 */
typedef struct {
	Board *array;
	int size;
	int mask;
} BoardCache;

/**
 * @brief Initialisation of the hash table.
 * @param hash Hash table.
 * @param bitsize Hash table size (as log2(size)).
 */
static void boardcache_init(BoardCache *hash, int bitsize)
{
	int i;

	hash->size = (1 << bitsize) + 3;
	hash->mask = (1 << bitsize) - 1;
	hash->array = (Board *) malloc(hash->size * sizeof (Board));
	if (hash->array == NULL) fatal_error("Cannot re-allocate board array.\n");
	for (i = 0; i < hash->size; ++i) hash->array[i].player = hash->array[i].opponent = -1ULL;
}

/**
 * @brief Free the hash table.
 * @param hash Hash table.
 */
static void boardcache_delete(BoardCache *hash)
{
	free(hash->array);
}

/**
 * @brief Append a shape to the hash table.
 * @param hash Hash table.
 * @param b Position.
 */
static bool boardcache_undone(BoardCache *hash, const Board *b)
{
	Board u;
	uint64_t h;
	int i;

	board_unique(b, &u);
	h = board_get_hash_code(&u);
	i = (h & hash->mask);
	if (board_equal(&u, hash->array + i)
	 || board_equal(&u, hash->array + i + 1)
	 || board_equal(&u, hash->array + i + 2)
	 || board_equal(&u, hash->array + i + 3)) return false;

	return true;
}


static void boardcache_append(BoardCache *hash, const Board *b)
{
	Board u;
	uint64_t h;
	int i, j, k, l;

	board_unique(b, &u);
	h = board_get_hash_code(&u);
	i = (h & hash->mask);
	if (board_equal(&u, hash->array + i)
	 || board_equal(&u, hash->array + i + 1)
	 || board_equal(&u, hash->array + i + 2)
	 || board_equal(&u, hash->array + i + 3)) return;

	l = board_count_empties(hash->array + i); j = i;
	k = board_count_empties(hash->array + ++i); if (k > l) {l = k; j = i;}
	k = board_count_empties(hash->array + ++i); if (k > l) {l = k; j = i;}
	k = board_count_empties(hash->array + ++i); if (k > l) {l = k; j = i;}

	hash->array[j] = u;
}

/**
 * @brief Initialisation of the hash table.
 * @param hash Hash table.
 * @param bitsize Hash table size (as log2(size)).
 */
void positionhash_init(PositionHash *hash, int bitsize)
{
	int i;

	hash->size = (1 << bitsize);
	hash->mask = hash->size - 1;
	hash->array = (PosArray*) malloc(hash->size * sizeof (PosArray));
	if (hash->array == NULL) fatal_error("Cannot re-allocate board array.\n");
	for (i = 0; i < hash->size; ++i) positionarray_init(hash->array + i);
}

/**
 * @brief Free the hash table.
 * @param hash Hash table.
 */
void positionhash_delete(PositionHash *hash)
{
	int i;

	for (i = 0; i < hash->size; ++i) positionarray_delete(hash->array + i);
	free(hash->array);
}

/**
 * @brief Append a position to the hash table.
 * @param hash Hash table.
 * @param b Position.
 * @return true if a position is added to the hash table, false otherwsise.
 */
bool positionhash_append(PositionHash *hash, const Board *b)
{
	Board u;
	CBoard c;
	uint64_t h;

	board_unique(b, &u);
	compact_board(&u, &c);
	h = board_get_hash_code(&u);
	return positionarray_append(hash->array + (h & hash->mask), &c);
}

/**
 * @brief Recursively count positions.
 * @param hash Hash table with unique positions.
 * @param cache Hash table caching count search.
 * @param board position.
 * @param depth depth.
 */
static uint64_t count_position(PositionHash *hash, BoardCache *cache, const Board *board, const int depth)
{
	uint64_t nodes = 0;
	uint64_t moves;
	int x;
	Board next;

	if (boardcache_undone(cache, board)) {
		if (depth == 0) return positionhash_append(hash, board);
		moves = board_get_moves(board);

		if (moves) {
			foreach_bit (x, moves) {
				board_next(board, x, &next);
				nodes += count_position(hash, cache, &next, depth - 1);
			}
		} else if (can_move(board->opponent, board->player)) {
			board_next(board, PASS, &next);
			nodes += count_position(hash, cache, &next, depth);
		}
		boardcache_append(cache, board);
	}

	return nodes;
}

/**
 * @brief Recursively count positions.
 * @param hash Hash table with unique positions.
 * @param cache Hash table caching count search.
 * @param board position.
 * @param depth depth.
 */
static uint64_t count_position_6x6(PositionHash *hash, BoardCache *cache, const Board *board, const int depth)
{
	uint64_t nodes = 0;
	uint64_t moves;
	int x;
	Board next;

	if (boardcache_undone(cache, board)) {
		if (depth == 0) return positionhash_append(hash, board);
		moves = get_moves_6x6(board->player, board->opponent);

		if (moves) {
			foreach_bit (x, moves) {
				board_next(board, x, &next);
				nodes += count_position_6x6(hash, cache, &next, depth - 1);
			}
		} else if (can_move_6x6(board->opponent, board->player)) {
			board_next(board, PASS, &next);
			nodes += count_position_6x6(hash, cache, &next, depth);
		}
		boardcache_append(cache, board);
	}

	return nodes;
}

/**
 * @brief Count positions.
 * @param board position.
 * @param depth depth.
 * @param size board_size (8 or 6).
 */
void count_positions(const Board *board, const int depth, const int size)
{
	int i;
	uint64_t n, c;
	int64_t t;
	PositionHash hash;
	BoardCache cache;


	board_print(board, BLACK, stdout);
	puts("\n discs       nodes         total            time   speed");
	puts("----------------------------------------------------------");
	c = 0;
	for (i = 0; i <= depth; ++i) {
		positionhash_init(&hash, options.hash_table_size);
		boardcache_init(&cache, options.hash_table_size);
		t = -cpu_clock();
		if (size == 6) c += (n = count_position_6x6(&hash, &cache, board, i));
		else c += (n = count_position(&hash, &cache, board, i));
		t += cpu_clock();
		printf("  %2d, %12" PRIu64 ", %12" PRIu64 ", ", i + 4, n, c);
		time_print(t, true, stdout);	printf(", ");
		print_scientific(c / (0.001 * t + 0.001), "N/s\n", stdout);
		positionhash_delete(&hash);
		boardcache_delete(&cache);
	}
	puts("----------------------------------------------------------");
}


/**
 * @brief unique shape.
 *
 * @param shape input Shape.
 * @return
 */
uint64_t shape_unique(uint64_t shape)
{
	uint64_t sym, unique;
	int i;

	unique = shape;
	for (i = 1; i < 8; ++i) {
		sym = shape;
		if (i & 1) sym = horizontal_mirror(sym);
		if (i & 2) sym = vertical_mirror(sym);
		if (i & 4) sym = transpose(sym);
		if (unique > sym) unique = sym;
	}

	return unique;
}

/**
 * Array of shape.
 */
typedef struct {
	uint64_t *item; /**< dynamic array */
	int n;       /**< number of items in the array */
	int size;    /**< capacity of the array */
} ShapeArray;

/**
 * @brief array initialisation.
 * @param array Array of shapes.
 */
static void shapearray_init(ShapeArray *array)
{
	array->item = NULL;
	array->n = array->size = 0;
}

/**
 * @brief array supression.
 * @param array Array of shapes.
 */
static void shapearray_delete(ShapeArray *array)
{
	free(array->item);
}

/**
 * @brief Append a shape into the array.
 * @param array Array of shapes.
 * @param shape Shape.
 */
static bool shapearray_append(ShapeArray *array, const uint64_t shape)
{
	int i;

	for (i = 0; i < array->n; ++i) {
		if (array->item[i] == shape) return false;
	}

	if (array->size == array->n) {
		array->item = (uint64_t*) realloc(array->item, (array->size += 1) * sizeof (uint64_t));
		if (array->item == NULL) fatal_error("Cannot re-allocate board array.\n");
	}
	array->item[array->n++] = shape;
	return true;
}


/**
 * HashTable of shapes
 */
typedef struct {
	ShapeArray *array;
	int size;
	int mask;
} ShapeHash;


/**
 * @brief Initialisation of the hash table.
 * @param hash Hash table.
 * @param bitsize Hash table size (as log2(size)).
 */
static void shapehash_init(ShapeHash *hash, int bitsize)
{
	int i;

	hash->size = (1 << bitsize);
	hash->mask = hash->size - 1;
	hash->array = (ShapeArray *) malloc(hash->size * sizeof (ShapeArray));
	if (hash->array == NULL) fatal_error("Cannot re-allocate board array.\n");
	for (i = 0; i < hash->size; ++i) shapearray_init(hash->array + i);
}

/**
 * @brief Free the hash table.
 * @param hash Hash table.
 */
static void shapehash_delete(ShapeHash *hash)
{
	int i;

	for (i = 0; i < hash->size; ++i) shapearray_delete(hash->array + i);
	free(hash->array);
}

/**
 * @brief Append a shape to the hash table.
 * @param hash Hash table.
 * @param b Position.
 */
static bool shapehash_append(ShapeHash *hash, const Board *b)
{
	uint64_t u;

	u = shape_unique(b->player | b->opponent);
	return shapearray_append(hash->array + (crc32c_u64(0, u) & hash->mask), u);
}

/**
 * @brief Recursively count shapes.
 * @param hash Hash table with unique shapes.
 * @param cache Hash table caching count search.
 * @param board shape.
 * @param depth depth.
 */
static uint64_t count_shape(ShapeHash *hash, BoardCache *cache, const Board *board, const int depth)
{
	uint64_t nodes = 0;
	uint64_t moves;
	int x;
	Board next;

	if (boardcache_undone(cache, board)) {
		if (depth == 0) return shapehash_append(hash, board);
		moves = board_get_moves(board);

		if (moves) {
			foreach_bit (x, moves) {
				board_next(board, x, &next);
				nodes += count_shape(hash, cache, &next, depth - 1);
			}
		} else {
			board_next(board, PASS, &next);
			if (can_move(board->player, board->opponent)) {
				nodes += count_shape(hash, cache, &next, depth);
			}
		}
		boardcache_append(cache, board);
	}

	return nodes;
}

/**
 * @brief Recursively count shapes.
 * @param hash Hash table with unique shapes.
 * @param cache Hash table caching count search.
 * @param board Board.
 * @param depth depth.
 */
static uint64_t count_shape_6x6(ShapeHash *hash, BoardCache *cache, const Board *board, const int depth)
{
	uint64_t nodes = 0;
	uint64_t moves;
	int x;
	Board next;

	if (boardcache_undone(cache, board)) {
		if (depth == 0) return shapehash_append(hash, board);
		moves = get_moves_6x6(board->player, board->opponent);

		if (moves) {
			foreach_bit (x, moves) {
				board_next(board, x, &next);
				nodes += count_shape_6x6(hash, cache, &next, depth - 1);
			}
		} else {
			board_next(board, PASS, &next);
			if (can_move_6x6(board->player, board->opponent)) {
				nodes += count_shape_6x6(hash, cache, &next, depth);
			}
		}
		boardcache_append(cache, board);
	}

	return nodes;
}


/**
 * @brief Count shapes.
 * @param board Board.
 * @param depth depth.
 * @param size size (8 or 6).
 */
void count_shapes(const Board *board, const int depth, const int size)
{
	int i;
	uint64_t n, c;
	int64_t t;
	ShapeHash hash;
	BoardCache cache;

	board_print(board, BLACK, stdout);
	puts("\n discs       nodes         total            time   speed");
	puts("----------------------------------------------------------");
	c = 0;
	for (i = 0; i <= depth; ++i) {
		shapehash_init(&hash, options.hash_table_size);
		boardcache_init(&cache, options.hash_table_size);
		t = -cpu_clock();
		if (size == 6) c += (n = count_shape_6x6(&hash, &cache, board, i));
		else c += (n = count_shape(&hash, &cache, board, i));
		t += cpu_clock();
		printf("  %2d, %12" PRIu64 ", %12" PRIu64 ", ", i + 4, n, c);
		time_print(t, true, stdout);	printf(", ");
		print_scientific(c / (0.001 * t + 0.001), "N/s\n", stdout);
		shapehash_delete(&hash);
		boardcache_delete(&cache);
	}
	puts("----------------------------------------------------------");
}



/**
 * @brief seek a game that reach to a position
 *
 * @param target position seeked
 * @param board starting position.
 * @param line line to reach the target position
 */
bool seek_position(const Board *target, const Board *board, Line *line) {
	const uint64_t mask = target->opponent | target->player;
	uint64_t moves;
	int x;
	Board next;

	if (board_equal(board, target)) return true;

	moves = board_get_moves(board);
	if (moves) {
		moves &= mask;
		foreach_bit (x, moves) {
			line_push(line, x);
			board_next(board, x, &next);
			if (seek_position(target, &next, line)) return true;
			line_pop(line);
		}
	} else {
		board_next(board, PASS, &next);
		if (can_move(next.player, next.opponent)) {
			if (seek_position(target, &next, line)) return true;
		}
	}

	return false;
}

