/**
 * @file board.c
 *
 * This module deals with the Board management.
 *
 * The Board is represented with a structure containing the following data:
 *  - a bitboard with the current player's square.
 *  - a bitboard with the current opponent's square.
 *
 * High level functions are provided to set/modify the board data or to compute
 * some board properties. Most of the functions are optimized to be as fast as
 * possible, while remaining readable.
 *
 * @date 1998 - 2024
 * @author Richard Delorme
 * @version 4.6
 */

#include "board.h"

#include "bit.h"
#include "crc32c.h"
#include "hash.h"
#include "move.h"
#include "settings.h"
#include "util.h"
#include "simd.h"

#include <assert.h>
#include <ctype.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdalign.h>

#if MOVE_GENERATOR == MOVE_GENERATOR_KINDERGARTEN
	#include "flip_kindergarten.c"
#elif MOVE_GENERATOR == MOVE_GENERATOR_ROXANE
	#include "flip_roxane.c"
#elif MOVE_GENERATOR == MOVE_GENERATOR_AVX512CD
	#include "flip_avx512cd.c"
#elif MOVE_GENERATOR == MOVE_GENERATOR_AVX_ACEPCK
	#include "flip_avx_acepck.c"
#elif MOVE_GENERATOR == MOVE_GENERATOR_AVX_CVTPS
	#include "flip_avx_cvtps.c"
#elif MOVE_GENERATOR == MOVE_GENERATOR_AVX_LZCNT
	#include "flip_avx_lzcnt.c"
#elif MOVE_GENERATOR == MOVE_GENERATOR_AVX_PPFILL
	#include "flip_avx_ppfill.c"
#elif MOVE_GENERATOR == MOVE_GENERATOR_AVX_PPSEQ
	#include "flip_avx_ppseq.c"
#elif MOVE_GENERATOR == MOVE_GENERATOR_BITSCAN
	#include "flip_bitscan.c"
#elif MOVE_GENERATOR == MOVE_GENERATOR_BMI2
	#include "flip_bmi2.c"
#elif MOVE_GENERATOR == MOVE_GENERATOR_CARRY_64
	#include "flip_carry_64.c"
#elif MOVE_GENERATOR == MOVE_GENERATOR_SSE_BITSCAN
	#include "flip_sse_bitscan.c"
#elif MOVE_GENERATOR == MOVE_GENERATOR_SSE
	#include "flip_sse.c"
#elif MOVE_GENERATOR == MOVE_GENERATOR_NEON_BITSCAN
	#include "flip_neon_bitscan.c"
#elif MOVE_GENERATOR == MOVE_GENERATOR_NEON_LZCNT
	#include "flip_neon_lzcnt.c"
#elif MOVE_GENERATOR == MOVE_GENERATOR_NEON_PPFILL
	#include "flip_neon_ppfill.c"
#elif MOVE_GENERATOR == MOVE_GENERATOR_NEON_RBIT
	#include "flip_neon_rbit.c"
#elif MOVE_GENERATOR == MOVE_GENERATOR_SVE_LZCNT
	#include "flip_sve_lzcnt.c"
#else
	#error "You need to provide a file to compute which discs a move shall flip"
#endif

#if COUNT_LAST_FLIP == COUNT_LAST_FLIP_KINDERGARTEN
	#include "count_last_flip_kindergarten.c"
#elif COUNT_LAST_FLIP == COUNT_LAST_FLIP_PLAIN
	#include "count_last_flip_plain.c"
#elif COUNT_LAST_FLIP == COUNT_LAST_FLIP_AVX512CD
	#include "count_last_flip_avxx512cd.c"
#elif COUNT_LAST_FLIP == COUNT_LAST_FLIP_AVX_PPFILL
	#include "count_last_flip_avx_ppfill.c"
#elif COUNT_LAST_FLIP == COUNT_LAST_FLIP_BITSCAN
	#include "count_last_flip_bitscan.c"
#elif COUNT_LAST_FLIP == COUNT_LAST_FLIP_BMI2
	#include "count_last_flip_bmi2.c"
#elif COUNT_LAST_FLIP == COUNT_LAST_FLIP_BMI
	#include "count_last_flip_bmi.c"
#elif COUNT_LAST_FLIP == COUNT_LAST_FLIP_CARRY_64
	#include "count_last_flip_carry_64.c"
#elif COUNT_LAST_FLIP == COUNT_LAST_FLIP_LZCNT
	#include "count_last_flip_lzcnt.c"
#elif COUNT_LAST_FLIP == COUNT_LAST_FLIP_SSE
	#include "count_last_flip_sse.c"
#elif COUNT_LAST_FLIP == COUNT_LAST_FLIP_NEON
	#include "count_last_flip_neon.c"
#elif COUNT_LAST_FLIP == COUNT_LAST_FLIP_NEON_VADDVQ
	#include "count_last_flip_neon.c"
#elif COUNT_LAST_FLIP == COUNT_LAST_FLIP_SVE_LZCNT
	#include "count_last_flip_sve_lzcnt.c"
#else
	#error "You need to provide a file to compute which flipped discs to count"
#endif

/** edge stability global data */
static uint8_t EDGE_STABILITY[256 * 256];

/**
 * @brief Swap players.
 *
 * Swap players, i.e. change player's turn.
 *
 * @param board board
 */
void board_swap_players(Board *board)
{
	const uint64_t tmp = board->player;
	board->player = board->opponent;
	board->opponent = tmp;
}

/**
 * @brief Set a board from a string description.
 *
 * Read a standardized string. (See http://radagast.se/othello/download2.html
 * for details) and translate it into our internal Board structure.
 *
 * @param board the board to set
 * @param string string describing the board
 * @return turn's color.
 */
int board_set(Board *board, const char *string)
{
	int i;
	const char *s = string;

	board->player = board->opponent = 0;
	for (i = A1; i <= H8; ++i) {
		if (*s == '\0') break;
		switch (tolower(*s)) {
		case 'b':
		case 'x':
		case '*':
			board->player |= x_to_bit(i);
			break;
		case 'o':
		case 'w':
			board->opponent |= x_to_bit(i);
			break;
		case '-':
		case '.':
			break;
		default:
			i--;
			break;
		}
		++s;
	}
	board_check(board);

	for (;*s != '\0'; ++s) {
		switch (tolower(*s)) {
		case 'b':
		case 'x':
		case '*':
			return BLACK;
		case 'o':
		case 'w':
			board_swap_players(board);
			return WHITE;
		default:
			break;
		}
	}

	warn("board_set: bad string input: %s\n", string);
	return EMPTY;
}

/**
 * @brief Set a board from a string description.
 *
 * Read a Forsyth-Edwards Notation string and translate it into our
 * internal Board structure.
 *
 * @param board the board to set
 * @param string string describing the board
 * @return turn's color.
 */
int board_from_FEN(Board *board, const char *string)
{
	int i;
	const char *s;

	board->player = board->opponent = 0;
	i = A8;
	for (s = parse_skip_spaces(string); *s && *s != ' '; ++s) {
		if (isdigit(*s)) {
			i += (*s - '0');
		} else if (*s == '/') {
			if (i & 7) return EMPTY;
			i -= 16;
		} else if (*s == 'p') {
			board->player |= x_to_bit(i);
			++i;
		} else if (*s == 'P') {
			board->opponent |= x_to_bit(i);
			++i;
		} else {
			return EMPTY;
		}
	}

	s = parse_skip_spaces(s);
	if (*s == 'b') {
		return BLACK;
	} else if (*s == 'w') {
		board_swap_players(board);
		return WHITE;
	}

	return EMPTY;
}

/**
 * @brief Set a board to the starting position.
 *
 * @param board the board to initialize
 */
void board_init(Board *board)
{
	board->player   = 0x0000000810000000ULL; // BLACK
	board->opponent = 0x0000001008000000ULL; // WHITE
}

/**
 * @brief Check board consistency
 *
 * @param board the board to initialize
 */
void board_check(const Board *board)
{
#ifndef NDEBUG
	errno = 0;
	if (board->player & board->opponent) {
		error("Two discs on the same square?\n");
		board_print(board, BLACK, stderr);
		bitboard_print(board->player, stderr);
		bitboard_print(board->opponent, stderr);
		abort();
	}

	// empty center ?
	if (((board->player|board->opponent) & 0x0000001818000000ULL) != 0x0000001818000000ULL) {
		error("Empty center?\n");
		board_print(board, BLACK, stderr);
	}
#else
	(void) board;
#endif // NDEBUG
}

/**
 * @brief Compare two boards for equality
 *
 * @param b1 first board
 * @param b2 second board
 * @return true if both board are equal
 */
bool board_equal(const Board *b1, const Board *b2)
{
	return (b1->player == b2->player && b1->opponent == b2->opponent);
}

/**
 * @brief Compare two boards
 *
 * @param b1 first board
 * @param b2 second board
 * @return true if b1 is lesser than b2.
 */
bool board_lesser(const Board *b1, const Board *b2)
{
	return (b1->player < b2->player) || (b1->player == b2->player && b1->opponent < b2->opponent);
}

#if USE_SIMD && defined(__AVX__)

#define H_MIRROR 15, 7, 11, 3, 13, 5, 9, 1, 14, 6, 10, 2, 12, 4, 8, 0
#define V_MIRROR 8, 9, 10, 11, 12, 13, 14, 15, 0, 1, 2, 3, 4, 5, 6, 7

/**
 * @brief Mirror a 128 bitboard vertically
 *
 * @param b the bitboard
 * @return a 128 bitboard mirrored
 */
static __m128i vectorcall horizontal_mirror_mm(const __m128i b)
{
	const __m128i mask = _mm_set1_epi16(0x0F0F);
	const __m128i rev  = _mm_set_epi8(H_MIRROR);
	return _mm_or_si128(_mm_shuffle_epi8(rev, _mm_and_si128(_mm_srli_epi64(b, 4), mask)), _mm_slli_epi64(_mm_shuffle_epi8(rev, _mm_and_si128(b, mask)), 4));
}

/**
 * @brief Mirror a 128 bitboard vertically
 *
 * @param b the bitboard
 * @return a 128 bitboard mirrored
 */
static __m128i vectorcall vertical_mirror_mm(const __m128i b)
{
	return _mm_shuffle_epi8(b, _mm_set_epi8(V_MIRROR));
}

/**
 * @brief Transposed a 128 bitboard
 *
 * @param b the bitboard
 * @return a 128 bitboard transposed
 */
static __m128i vectorcall transpose_mm(__m128i b)
{
	const __m128i mask00AA = _mm_set1_epi16(0x00AA);
	const __m128i maskCCCC = _mm_set1_epi32(0x0000CCCC);
	const __m128i mask00F0 = _mm_set1_epi64x(0x00000000F0F0F0F0);
	__m128i t = _mm_and_si128(_mm_xor_si128(b, _mm_srli_epi64(b, 7)), mask00AA);
	b = _mm_xor_si128(_mm_xor_si128(b, t), _mm_slli_epi64(t, 7));
	t = _mm_and_si128(_mm_xor_si128(b, _mm_srli_epi64(b, 14)), maskCCCC);
	b = _mm_xor_si128(_mm_xor_si128(b, t), _mm_slli_epi64(t, 14));
	t = _mm_and_si128(_mm_xor_si128(b, _mm_srli_epi64(b, 28)), mask00F0);
	b = _mm_xor_si128(_mm_xor_si128(b, t), _mm_slli_epi64(t, 28));

	return b;
}

#endif

#if USE_SIMD && defined(__AVX2__)

/**
 * @brief Mirror two consecutive boards horizontally
 *
 * @param b input boards
 * @param sym output boards
 */
static void vectorcall horizontal_mirror_avx2(const __m256i *b, __m256i *sym)
{
	const __m256i mask = _mm256_set1_epi16(0x0F0F);
	const __m256i rev  = _mm256_set_epi8(H_MIRROR, H_MIRROR);
	*sym = _mm256_or_si256(_mm256_shuffle_epi8(rev, _mm256_and_si256(_mm256_srli_epi64(*b, 4), mask)),
		_mm256_slli_epi64(_mm256_shuffle_epi8(rev, _mm256_and_si256(*b, mask)), 4));
}

/**
 * @brief Mirror two consecutive boards vertically
 *
 * @param b input boards
 * @param sym output boards
 */
static void vectorcall vertical_mirror_avx2(const __m256i *b, __m256i *sym)
{
	const __m256i mask = _mm256_set_epi8( V_MIRROR, V_MIRROR);
	*sym = _mm256_shuffle_epi8(*b, mask);
}

#endif

#if USE_SIMD && defined(__ARM_NEON)

/**
 * @brief Mirror a board horizontally using Neon's instructions.
 *
 * @param b input boards
 * @param sym output boards
 */
static uint64x2_t horizontal_mirror_neon(const uint64x2_t board)
{
	return vreinterpretq_u64_u8(vrbitq_u8(vreinterpretq_u8_u64(board)));
}

/**
 * @brief Mirror a board vertcally using Neon's instructions.
 *
 * @param b input boards
 * @param sym output boards
 */
static uint64x2_t vertical_mirror_neon(const uint64x2_t board)
{
	return vreinterpretq_u64_u8(vrev64q_u8(vreinterpretq_u8_u64(board)));
}

static uint64x2_t transpose_neon(const uint64x2_t board)
{
	uint64x2_t bb = board, tt;

	tt = vandq_u64(veorq_u64(bb, vshrq_n_u64(bb, 7)), vdupq_n_u64(0x00AA00AA00AA00AA));
	bb = veorq_u64(veorq_u64(bb, tt), vshlq_n_u64(tt, 7));
	tt = vandq_u64(veorq_u64(bb, vshrq_n_u64(bb, 14)), vdupq_n_u64(0x0000CCCC0000CCCC));
	bb = veorq_u64(veorq_u64(bb, tt), vshlq_n_u64(tt, 14));
	tt = vandq_u64(veorq_u64(bb, vshrq_n_u64(bb, 28)), vdupq_n_u64(0x00000000F0F0F0F0));
	bb = veorq_u64(veorq_u64(bb, tt), vshlq_n_u64(tt, 28));

	return bb;
}

#endif

/**
 * @brief Mirror a board  horizontally
 *
 * @param board input board
 * @param sym output board, horrizontally mirrored
 */
void board_horizontal_mirror(const Board *board, Board *sym)
{
#if USE_SIMD & defined(__ARM_NEON)
	vst1q_u64((uint64_t *) sym, horizontal_mirror_neon(vld1q_u64((uint64_t *) board)));
#else
	sym->player = horizontal_mirror(board->player);
	sym->opponent = horizontal_mirror(board->opponent);
#endif
}

/**
 * @brief Mirror a board  horizontally
 *
 * @param board input board
 * @param sym output board, horrizontally mirrored
 */
void board_vertical_mirror(const Board *board, Board *sym)
{
#if USE_SIMD & defined(__ARM_NEON)
	vst1q_u64((uint64_t *) sym, vertical_mirror_neon(vld1q_u64((uint64_t *) board)));
#else
	sym->player = vertical_mirror(board->player);
	sym->opponent = vertical_mirror(board->opponent);
#endif
}

/**
 * @brief Mirror a board  horizontally
 *
 * @param board input board
 * @param sym output board, horrizontally mirrored
 */
void board_transpose(const Board *board, Board *sym)
{
#if defined(_ARM_NEON)
	vst1q_u64((uint64_t *) sym, transpose_neon(vld1q_u64((uint64_t *) board)));
#else
	sym->player = transpose(board->player);
	sym->opponent = transpose(board->opponent);
#endif
}


/**
 * @brief symetric board
 *
 * @param board input board
 * @param s symetry
 * @param sym symetric output board
 */
void board_symetry(const Board *board, const int s, Board *sym)
{
#if USE_SIMD && defined(__AVX__)

	__m128i	b = _mm_lddqu_si128((__m128i *) board);

	if (s & 1) b = horizontal_mirror_mm(b);
	if (s & 2) b = vertical_mirror_mm(b);
	if (s & 4) b = transpose_mm(b);

	_mm_storeu_si128((__m128i *) sym, b);

#elif USE_SIMD && defined(__ARM_NEON)

	uint64x2_t b = vld1q_u64((uint64_t *) board);

	if (s & 1) b = horizontal_mirror_neon(b);
	if (s & 2) b = vertical_mirror_neon(b);
	if (s & 4) b = transpose_neon(b);

	vst1q_u64((uint64_t *) sym, b);

#else

	*sym = *board;
	if (s & 1) board_horizontal_mirror(sym, sym);
	if (s & 2) board_vertical_mirror(sym, sym);
	if (s & 4) board_transpose(sym, sym);

#endif

	board_check(sym);
}

/**
 * @brief unique board
 *
 * Compute a board unique from all its possible symertries.
 *
 * @param board input board
 * @param unique output board
 */
int board_unique(const Board *board, Board *unique)
{
	assert(board != unique);

#if USE_SIMD && defined(__AVX2__)

	alignas(32) Board sym[8];
	int i, s = 0;
	static const int reorder[8] = { 0, 4, 1, 6, 2, 5, 3, 7 };

	sym[0] = *board;
	board_transpose(board, &sym[1]);
	horizontal_mirror_avx2((__m256i *) &sym[0], (__m256i *) &sym[2]);
	vertical_mirror_avx2((__m256i *) &sym[0], (__m256i *) &sym[4]);
	vertical_mirror_avx2((__m256i *) &sym[2], (__m256i *) &sym[6]);

	*unique = *board;
	for (i = 1; i < 8; ++i) {
		if (board_lesser(&sym[i], unique)) {
			*unique = sym[i];
			s = i;
		}
	}
	s = reorder[s];

#else

	Board sym;
	int i, s = 0;

	*unique = *board;
	for (i = 1; i < 8; ++i) {
		board_symetry(board, i, &sym);
		if (board_lesser(&sym, unique)) {
			*unique = sym;
			s = i;
		}
	}

#endif

	board_check(unique);
	return s;
}

/**
 * @brief Get a random board by playing random moves.
 *
 * @param board The output board.
 * @param n_ply The number of random move to generate.
 * @param r The random generator.
 */
void board_rand(Board *board, int n_ply, Random *r)
{
	Move move;
	uint64_t moves;
	int ply;

	board_init(board);
	for (ply = 0; ply < n_ply; ply++) {
		moves = board_get_moves(board);
		if (!moves) {
			board_pass(board);
			moves = board_get_moves(board);
			if (!moves) {
				break;
			}
		}
		board_get_move(board, get_rand_bit(moves, r), &move);
		board_update(board, &move);
	}
}

/**
 * @brief Compute a move.
 *
 * Compute how the board will be modified by a move without playing it.
 *
 * @param board board
 * @param x     square on which to move.
 * @param move  a Move structure remembering the modification.
 * @return      the flipped discs.
 */
uint64_t board_get_move(const Board *board, const int x, Move *move)
{
	move->flipped = board_flip(board, x);
	move->x = x;
	return move->flipped;
}

/**
 * @brief Check if a move is legal.
 *
 * @param board board
 * @param move  a Move.
 * @return      true if the move is legal, false otherwise.
 */
bool board_check_move(const Board *board, Move *move)
{
	if (move->x == PASS) return !can_move(board->player, board->opponent);
	else if ((x_to_bit(move->x) & ~(board->player|board->opponent)) == 0) return false;
	else if (move->flipped != board_flip(board, move->x)) return false;
	else return true;
}

/**
 * @brief Update a board.
 *
 * Update a board by flipping its discs and updating every other data,
 * according to the 'move' description.
 *
 * @param board the board to modify
 * @param move  A Move structure describing the modification.
 */
void board_update(Board *board, const Move *move)
{
	const uint64_t tmp = board->player ^ (move->flipped | x_to_bit(move->x));
	board->player = board->opponent ^ move->flipped;
	board->opponent = tmp;

	board_check(board);
}

/**
 * @brief Restore a board.
 *
 * Restore a board by un-flipping its discs and restoring every other data,
 * according to the 'move' description, in order to cancel a board_update_move.
 *
 * @param board board to restore.
 * @param move  a Move structure describing the modification.
 */
void board_restore(Board *board, const Move *move)
{
	const uint64_t tmp = board->opponent ^ (move->flipped | x_to_bit(move->x));
	board->opponent = board->player ^ move->flipped;
	board->player = tmp;

	board_check(board);
}

/**
 * @brief Passing move
 *
 * Modify a board by passing player's turn.
 *
 * @param board board to update.
 */
void board_pass(Board *board)
{
	board_swap_players(board);
	board_check(board);
}

/**
 * @brief Compute a board resulting of a move played on a previous board.
 *
 * @param board board to play the move on.
 * @param x move to play.
 * @param next resulting board.
 * @return flipped discs.
 */
uint64_t board_next(const Board *board, const int x, Board *next)
{
	const uint64_t flipped = board_flip(board, x);
	const uint64_t tmp = board->opponent ^ flipped;
	next->opponent = board->player ^ (flipped | x_to_bit(x));
	next->player = tmp;

	return flipped;
}

/**
 * @brief Get a part of the moves.
 *
 * Partially compute a bitboard where each coordinate with a legal move is set to one.
 *
 * Three variants of the algorithm are provided, one based on Kogge-Stone parallel
 * prefix.
 *
 * @param P bitboard with player's discs.
 * @param mask bitboard with flippable opponent's discs.
 * @param dir flipping direction.
 * @return some legal moves in a 64-bit unsigned integer.
 */
#if USE_SIMD && defined(__ARM_NEON)

	static inline uint64x2_t get_some_moves_neon(const uint64x2_t PP, uint64x2_t MM, const int dir)
	{
		// parrallel prefix using ARM Neon SIMD
		// all operations are down left & right at the same time
		uint64x2_t FF; // flip;
		int64x2_t DD, DDx2; // (dir, -dir) & (2*dir, -2*dir)

		DD = vcombine_s64(vcreate_s64(dir), vcreate_s64(-dir));
		DDx2 = vaddq_s64(DD, DD);
		FF = vandq_u64(MM, vshlq_u64(PP, DD));
		FF = vorrq_u64(FF, vandq_u64(MM, vshlq_u64(FF, DD)));
		MM = vandq_u64(MM, vshlq_u64(MM, DD));
		FF = vorrq_u64(FF, vandq_u64(MM, vshlq_u64(FF, DDx2)));
		FF = vorrq_u64(FF, vandq_u64(MM, vshlq_u64(FF, DDx2)));
		FF = vshlq_u64(FF, DD);

		return FF;
	}

#endif

static inline uint64_t get_some_moves(const uint64_t P, const uint64_t mask, const int dir)
{
#if USE_SIMD && defined(__ARM_NEON)
	const uint64x2_t PP = vdupq_n_u64(P);
	const uint64x2_t MM = vdupq_n_u64(mask);
	const uint64x2_t moves = get_some_moves_neon(PP, MM, dir);
	return 	vgetq_lane_u64(moves, 0) | vgetq_lane_u64(moves, 1);

#elif PARALLEL_PREFIX & 1

	// 1-stage Parallel Prefix (intermediate between kogge stone & sequential)
	// 6 << + 6 >> + 7 | + 10 &
	uint64_t flip_l, flip_r;
	uint64_t mask_l, mask_r;
	const int dir2 = dir + dir;

	flip_l  = mask & (P << dir);          flip_r  = mask & (P >> dir);
	flip_l |= mask & (flip_l << dir);     flip_r |= mask & (flip_r >> dir);
	mask_l  = mask & (mask << dir);       mask_r  = mask & (mask >> dir);
	flip_l |= mask_l & (flip_l << dir2);  flip_r |= mask_r & (flip_r >> dir2);
	flip_l |= mask_l & (flip_l << dir2);  flip_r |= mask_r & (flip_r >> dir2);

	return (flip_l << dir) | (flip_r >> dir);

#elif KOGGE_STONE & 1

	// kogge-stone algorithm
	// 6 << + 6 >> + 12 & + 7 |
	// + better instruction independency
	uint64_t flip_l, flip_r;
	uint64_t mask_l, mask_r;
	const int dir2 = dir << 1;
	const int dir4 = dir << 2;

	flip_l  = P | (mask & (P << dir));    flip_r  = P | (mask & (P >> dir));
	mask_l  = mask & (mask << dir);       mask_r  = mask & (mask >> dir);
	flip_l |= mask_l & (flip_l << dir2);  flip_r |= mask_r & (flip_r >> dir2);
	mask_l &= (mask_l << dir2);           mask_r &= (mask_r >> dir2);
	flip_l |= mask_l & (flip_l << dir4);  flip_r |= mask_r & (flip_r >> dir4);

	return ((flip_l & mask) << dir) | ((flip_r & mask) >> dir);

#else

	// sequential algorithm
	// 7 << + 7 >> + 6 & + 12 |
	uint64_t flip;

	flip = (((P << dir) | (P >> dir)) & mask);
	flip |= (((flip << dir) | (flip >> dir)) & mask);
	flip |= (((flip << dir) | (flip >> dir)) & mask);
	flip |= (((flip << dir) | (flip >> dir)) & mask);
	flip |= (((flip << dir) | (flip >> dir)) & mask);
	flip |= (((flip << dir) | (flip >> dir)) & mask);
	return (flip << dir) | (flip >> dir);

#endif
}

#if USE_SIMD && defined(__AVX2__)

/**
 * @brief Get legal moves using vector parallelism.
 *
 * Compute a bitboard where each coordinate with a legal move is set to one.
 *
 * @param PP bitboardx4 with player's discs.
 * @param OO bitboardx4 with opponent's discs.
 * @return all legal moves in a 64-bit unsigned integer.
 */

uint64_t vectorcall get_moves_avx2(__m256i PP, __m256i OO)
{
	__m256i	MM, flip_l, flip_r, pre_l, pre_r;
	__m128i	M;
	const __m256i dir1 = _mm256_set_epi64x(7, 9, 8, 1);
	const __m256i dir2 = _mm256_add_epi64(dir1, dir1);
	const __m256i mask = _mm256_and_si256(OO, _mm256_set_epi64x(0x007E7E7E7E7E7E00, 0x007E7E7E7E7E7E00, 0x00FFFFFFFFFFFF00, 0x7E7E7E7E7E7E7E7E));
	const __m128i occupied = _mm_or_si128(_mm256_castsi256_si128(PP), _mm256_castsi256_si128(OO));

	flip_l = _mm256_and_si256(mask, _mm256_sllv_epi64(PP, dir1));
	flip_r = _mm256_and_si256(mask, _mm256_srlv_epi64(PP, dir1));
	flip_l = _mm256_or_si256(flip_l, _mm256_and_si256(mask, _mm256_sllv_epi64(flip_l, dir1)));
	flip_r = _mm256_or_si256(flip_r, _mm256_and_si256(mask, _mm256_srlv_epi64(flip_r, dir1)));
	pre_l = _mm256_and_si256(mask, _mm256_sllv_epi64(mask, dir1));
	pre_r = _mm256_srlv_epi64(pre_l, dir1);
	flip_l = _mm256_or_si256(flip_l, _mm256_and_si256(pre_l, _mm256_sllv_epi64(flip_l, dir2)));
	flip_r = _mm256_or_si256(flip_r, _mm256_and_si256(pre_r, _mm256_srlv_epi64(flip_r, dir2)));
	flip_l = _mm256_or_si256(flip_l, _mm256_and_si256(pre_l, _mm256_sllv_epi64(flip_l, dir2)));
	flip_r = _mm256_or_si256(flip_r, _mm256_and_si256(pre_r, _mm256_srlv_epi64(flip_r, dir2)));
	MM = _mm256_or_si256(_mm256_sllv_epi64(flip_l, dir1), _mm256_srlv_epi64(flip_r, dir1));

	M = _mm_or_si128(_mm256_castsi256_si128(MM), _mm256_extracti128_si256(MM, 1));
	return _mm_cvtsi128_si64(_mm_andnot_si128(occupied, _mm_or_si128(M, _mm_unpackhi_epi64(M, M))));	// mask with empties
}

#else

/**
 * @brief Get legal moves.
 *
 * Compute a bitboard where each coordinate with a legal move is set to one.
 *
 * @param P bitboard with player's discs.
 * @param O bitboard with opponent's discs.
 * @return all legal moves in a 64-bit unsigned integer.
 */
uint64_t get_moves(const uint64_t P, const uint64_t O)
{
	const uint64_t mask = O & 0x7E7E7E7E7E7E7E7Eull; // opponent
	const uint64_t E = ~(P|O); // empties

#if USE_SIMD && defined(__ARM_NEON)
	const uint64x2_t PP = vdupq_n_u64(P);
	const uint64x2_t OO = vdupq_n_u64(O);
	const uint64x2_t MM = vdupq_n_u64(mask);

	uint64x2_t moves = get_some_moves_neon(PP, MM, 1);        // horizontal
	moves = vorrq_u64(moves, get_some_moves_neon(PP, OO, 8));    // vertical
	moves = vorrq_u64(moves, get_some_moves_neon(PP, MM, 7)); // diagonals
	moves = vorrq_u64(moves, get_some_moves_neon(PP, MM, 9));
	return (vgetq_lane_u64(moves, 0) | vgetq_lane_u64(moves, 1)) & E; // mask with empties

#else

	return (get_some_moves(P, mask, 1)   // horizontal
	      | get_some_moves(P, O, 8)      // vertical
	      | get_some_moves(P, mask, 7)   // diagonals
	      | get_some_moves(P, mask, 9)) & E; // mask with empties

#endif
}

#endif

/**
 * @brief Get legal moves on a 6x6 board.
 *
 * Compute a bitboard where each coordinate with a legal move is set to one.
 *
 * @param P bitboard with player's discs.
 * @param O bitboard with opponent's discs.
 * @return all legal moves in a 64-bit unsigned integer.
 */
uint64_t get_moves_6x6(const uint64_t P, const uint64_t O)
{
	const uint64_t E = (~(P|O) & 0x007E7E7E7E7E7E00ull); // empties

	return ((get_some_moves(P, O & 0x003C3C3C3C3C3C00ull, 1) // horizontal
		| get_some_moves(P, O & 0x00007E7E7E7E0000ull, 8)    // vertical
		| get_some_moves(P, O & 0x00003C3C3C3C0000ull, 7)    // diagonals
		| get_some_moves(P, O & 0x00003C3C3C3C0000ull, 9))
		& E); // mask with empties
}

/**
 * @brief Check if a player can move.
 *
 * @param P bitboard with player's discs.
 * @param O bitboard with opponent's discs.
 * @return true or false.
 */
bool can_move(const uint64_t P, const uint64_t O)
{
	const uint64_t E = ~(P|O); // empties

	return (get_some_moves(P, O & 0x007E7E7E7E7E7E00ull, 7) & E)  // diagonals
		|| (get_some_moves(P, O & 0x007E7E7E7E7E7E00ull, 9) & E)
		|| (get_some_moves(P, O & 0x7E7E7E7E7E7E7E7Eull, 1) & E)  // horizontal
		|| (get_some_moves(P, O & 0x00FFFFFFFFFFFF00ull, 8) & E); // vertical
}

/**
 * @brief Check if a player can move.
 *
 * @param P bitboard with player's discs.
 * @param O bitboard with opponent's discs.
 * @return true or false.
 */
bool can_move_6x6(const uint64_t P, const uint64_t O)
{
	const uint64_t E = (~(P|O) & 0x007E7E7E7E7E7E00ull); // empties

	return (get_some_moves(P, O & 0x00003C3C3C3C0000ull, 7) & E)  // diagonals
		|| (get_some_moves(P, O & 0x00003C3C3C3C0000ull, 9) & E)
		|| (get_some_moves(P, O & 0x003C3C3C3C3C3C00ull, 1) & E)  // horizontal
		|| (get_some_moves(P, O & 0x00007E7E7E7E0000ull, 8) & E); // vertical
}

/**
 * @brief Count legal moves.
 *
 * Compute mobility, ie the number of legal moves.
 *
 * @param P bitboard with player's discs.
 * @param O bitboard with opponent's discs.
 * @return a count of all legal moves.
 */
int get_mobility(const uint64_t P, const uint64_t O)
{
	return bit_count(get_moves(P, O));
}

int get_weighted_mobility(const uint64_t P, const uint64_t O)
{
	return bit_weighted_count(get_moves(P, O));
}

/**
 * @brief Get some potential moves.
 *
 * @param O bitboard with opponent's discs.
 * @param dir flipping direction.
 * @return some potential moves in a 64-bit unsigned integer.
 */
static uint64_t get_some_potential_moves(const uint64_t O, const int dir)
{
	return (O << dir | O >> dir);
}

/**
 * @brief Get potential moves.
 *
 * Get the list of empty squares in contact of a player square.
 *
 * @param P bitboard with player's discs.
 * @param O bitboard with opponent's discs.
 * @return all potential moves in a 64-bit unsigned integer.
 */
uint64_t get_potential_moves(const uint64_t P, const uint64_t O)
{
	return (get_some_potential_moves(O & 0x7E7E7E7E7E7E7E7Eull, 1) // horizontal
		| get_some_potential_moves(O & 0x00FFFFFFFFFFFF00ull, 8)   // vertical
		| get_some_potential_moves(O & 0x007E7E7E7E7E7E00ull, 7)   // diagonals
		| get_some_potential_moves(O & 0x007E7E7E7E7E7E00ull, 9))
		& ~(P|O); // mask with empties
}

#if USE_SIMD && defined(__AVX2__)

/**
 * @brief AVX2 optimized get_moves + get_potential_moves.
 *
 * Get the bitboard of empty squares in contact of a player square, as well as real mobility.
 *
 * @param PP broadcasted bitboard with player's discs.
 * @param OO broadcasted bitboard with opponent's discs.
 * @return potential moves in a higner 64-bit, real moves in a lower 64-bit.
 */
__m128i vectorcall get_moves_and_potential(__m256i PP, __m256i OO)
{
	__m256i	MM, potmob, flip_l, flip_r, pre_l, pre_r, shift2;
	const __m256i shift1897 = _mm256_set_epi64x(7, 9, 8, 1);
	__m256i	mOO = _mm256_and_si256(OO, _mm256_set_epi64x(0x007E7E7E7E7E7E00, 0x007E7E7E7E7E7E00, 0x00FFFFFFFFFFFF00, 0x7E7E7E7E7E7E7E7E));
	__m128i occupied = _mm_or_si128(_mm256_castsi256_si128(PP), _mm256_castsi256_si128(OO));

	flip_l = _mm256_and_si256(mOO, _mm256_sllv_epi64(PP, shift1897));
	flip_r = _mm256_and_si256(mOO, _mm256_srlv_epi64(PP, shift1897));
	flip_l = _mm256_or_si256(flip_l, _mm256_and_si256(mOO, _mm256_sllv_epi64(flip_l, shift1897)));
	flip_r = _mm256_or_si256(flip_r, _mm256_and_si256(mOO, _mm256_srlv_epi64(flip_r, shift1897)));
	pre_l = _mm256_sllv_epi64(mOO, shift1897);	pre_r = _mm256_srlv_epi64(mOO, shift1897);
	potmob = _mm256_or_si256(pre_l, pre_r);
	pre_l = _mm256_and_si256(mOO, pre_l);		pre_r = _mm256_and_si256(mOO, pre_r);
	shift2 = _mm256_add_epi64(shift1897, shift1897);
	flip_l = _mm256_or_si256(flip_l, _mm256_and_si256(pre_l, _mm256_sllv_epi64(flip_l, shift2)));
	flip_r = _mm256_or_si256(flip_r, _mm256_and_si256(pre_r, _mm256_srlv_epi64(flip_r, shift2)));
	flip_l = _mm256_or_si256(flip_l, _mm256_and_si256(pre_l, _mm256_sllv_epi64(flip_l, shift2)));
	flip_r = _mm256_or_si256(flip_r, _mm256_and_si256(pre_r, _mm256_srlv_epi64(flip_r, shift2)));
	MM = _mm256_or_si256(_mm256_sllv_epi64(flip_l, shift1897), _mm256_srlv_epi64(flip_r, shift1897));

	MM = _mm256_or_si256(_mm256_unpacklo_epi64(MM, potmob), _mm256_unpackhi_epi64(MM, potmob));
	return _mm_andnot_si128(occupied, _mm_or_si128(_mm256_castsi256_si128(MM), _mm256_extracti128_si256(MM, 1)));	// mask with empties
}

#endif


/**
 * @brief Get potential mobility.
 *
 * Count the list of empty squares in contact of a player square.
 *
 * @param P bitboard with player's discs.
 * @param O bitboard with opponent's discs.
 * @return a count of potential moves.
 */
int get_potential_mobility(const uint64_t P, const uint64_t O)
{
	return bit_weighted_count(get_potential_moves(P, O));
}

/**
 * macro to pack/unpack vertical edges
 */
#if 0 && defined(__BMI2__) && !defined(SLOW_BMI2)
	#define	unpackA2A7(x) _pdep_u64((x), 0x0101010101010101)
	#define	unpackH2H7(x) _pdep_u64((x), 0x8080808080808080)
	#define	packA1A8(x)	  _pext_u64((x), 0x0101010101010101)
	#define	packH1H8(x)	  _pext_u64((x), 0x8080808080808080)
#else
	#define	unpackA2A7(x)	((((x) & 0x7e) * 0x0000040810204080) & 0x0001010101010100)
	#define	unpackH2H7(x)	((((x) & 0x7e) * 0x0002040810204000) & 0x0080808080808000)
	#define	packA1A8(X)	((((X) & 0x0101010101010101ULL) * 0x0102040810204080ULL) >> 56)
	#define	packH1H8(X)	((((X) & 0x8080808080808080ULL) * 0x0002040810204081ULL) >> 56)
#endif

/**
 * @brief search stable edge patterns.
 *
 * Compute a 8-bit bitboard where each stable square is set to one
 *
 * PS: no need to optimize this code
 *
 * @param old_P previous player edge discs.
 * @param old_O previous opponent edge discs.
 * @param stable 8-bit bitboard with stable edge squares.
 */
static int find_edge_stable(const int old_P, const int old_O, int stable)
{
	int P, O, x, y;
	const int E = ~(old_P | old_O); // empties

	stable &= old_P; // mask stable squares with remaining player squares.
	if (!stable || E == 0) return stable;

	for (x = 0; x < 8; ++x) {
		if (E & x_to_bit(x)) { //is x an empty square ?
			O = old_O;
			P = old_P | x_to_bit(x); // player plays on it
			if (x > 1) { // flip left discs
				for (y = x - 1; y > 0 && (O & x_to_bit(y)); --y) ;
				if (P & x_to_bit(y)) {
					for (y = x - 1; y > 0 && (O & x_to_bit(y)); --y) {
						O ^= x_to_bit(y); P ^= x_to_bit(y);
					}
				}
			}
			if (x < 6) { // flip right discs
				for (y = x + 1; y < 8 && (O & x_to_bit(y)); ++y) ;
				if (P & x_to_bit(y)) {
					for (y = x + 1; y < 8 && (O & x_to_bit(y)); ++y) {
						O ^= x_to_bit(y); P ^= x_to_bit(y);
					}
				}
			}
			stable = find_edge_stable(P, O, stable); // next move
			if (!stable) return stable;

			P = old_P;
			O = old_O | x_to_bit(x); // opponent plays on it
			if (x > 1) {
				for (y = x - 1; y > 0 && (P & x_to_bit(y)); --y) ;
				if (O & x_to_bit(y)) {
					for (y = x - 1; y > 0 && (P & x_to_bit(y)); --y) {
						O ^= x_to_bit(y); P ^= x_to_bit(y);
					}
				}
			}
			if (x < 6) {
				for (y = x + 1; y < 8 && (P & x_to_bit(y)); ++y) ;
				if (O & x_to_bit(y)) {
					for (y = x + 1; y < 8 && (P & x_to_bit(y)); ++y) {
						O ^= x_to_bit(y); P ^= x_to_bit(y);
					}
				}
			}
			stable = find_edge_stable(P, O, stable); // next move
			if (!stable) return stable;
		}
	}

	return stable;
}

/**
 * @brief Initialize the edge stability tables.
 */
void edge_stability_init(void)
{
	int P, O;

	for (P = 0; P < 256; ++P)
	for (O = 0; O < 256; ++O) {
		if (P & O) { // illegal positions
			EDGE_STABILITY[P * 256 + O] = 0;
		} else {
			EDGE_STABILITY[P * 256 + O] = find_edge_stable(P, O, P);
		}
	}
}

/**
 * @brief Get stable edge.
 *
 * This function uses precomputed exact stable edge table to accelerate
 * the computation.
 *
 * @param P bitboard with player's discs.
 * @param O bitboard with opponent's discs.
 * @return a bitboard with (some of) player's stable discs.
 *
 */
static uint64_t get_stable_edge(const uint64_t P, const uint64_t O)
{
#if USE_SIMD && defined(__SSE2__)

	// compute the exact stable edges (from precomputed tables)
	unsigned int a1a8, h1h8;
	uint64_t stable_edge;

	__m128i	P0 = _mm_cvtsi64_si128(P);
	__m128i	O0 = _mm_cvtsi64_si128(O);
	__m128i	PO = _mm_unpacklo_epi8(O0, P0);
	stable_edge = EDGE_STABILITY[_mm_extract_epi16(PO, 0)] | ((uint64_t) EDGE_STABILITY[_mm_extract_epi16(PO, 7)] << 56);

	PO = _mm_unpacklo_epi64(O0, P0);
	a1a8 = EDGE_STABILITY[_mm_movemask_epi8(_mm_slli_epi64(PO, 7))];
	h1h8 = EDGE_STABILITY[_mm_movemask_epi8(PO)];
	stable_edge |= unpackA2A7(a1a8) | unpackH2H7(h1h8);

	return stable_edge;

#elif 0 && USE_SIMD && defined(__ARM_NEON)

	// slower than standard version
	const uint64x2_t shiftv = { 0x0003000200010000, 0x0007000600050004 };
	uint8x16_t PO = vzip1q_u8(vreinterpretq_u8_u64(vdupq_n_u64(O)), vreinterpretq_u8_u64(vdupq_n_u64(P)));
	unsigned int a1a8 = EDGE_STABILITY[vaddvq_u16(vshlq_u16(vreinterpretq_u16_u8(vandq_u8(PO, vdupq_n_u8(1))), vreinterpretq_s16_u64(shiftv)))];
	unsigned int h1h8 = EDGE_STABILITY[vaddvq_u16(vshlq_u16(vreinterpretq_u16_u8(vshrq_n_u8(PO, 7)), vreinterpretq_s16_u64(shiftv)))];
	return EDGE_STABILITY[vgetq_lane_u16(vreinterpretq_u16_u8(PO), 0)]
	    |  (unsigned long long) EDGE_STABILITY[vgetq_lane_u16(vreinterpretq_u16_u8(PO), 7)] << 56
	    |  unpackA2A7(a1a8) | unpackH2H7(h1h8);
#else

	// compute the exact stable edges (from precomputed tables)
	return EDGE_STABILITY[(P & 0xff) * 256 + (O & 0xff)]
	    |  ((uint64_t)EDGE_STABILITY[(P >> 56) * 256 + (O >> 56)]) << 56
	    |  unpackA2A7(EDGE_STABILITY[packA1A8(P) * 256 + packA1A8(O)])
	    |  unpackH2H7(EDGE_STABILITY[packH1H8(P) * 256 + packH1H8(O)]);

#endif

}

/**
 * @brief Get full lines.
 *
 * @param line all discs on a line.
 * @param dir tested direction
 * @return a bitboard with full lines along the tested direction.
 */
#if 0 && USE_SIMD && defined(__AVX2__)

static __m256i get_full_lines(const uint64_t disc)
{
	__m128i l81, l79, l8;
	__m256i	v4_disc, lr79;
	const __m128i kff  = _mm_set1_epi8(-1);

	const __m128i mcpyswap = _mm_set_epi8(0, 1, 2, 3, 4, 5, 6, 7, 7, 6, 5, 4, 3, 2, 1, 0);
	const __m128i mbswapll = _mm_set_epi8(8, 9, 10, 11, 12, 13, 14, 15, 0, 1, 2, 3, 4, 5, 6, 7);
	static const V4DI shiftlr[] = {{{ 9, 7, 7, 9 }}, {{ 18, 14, 14, 18 }}, {{ 36, 28, 28, 36 }}};
	static const V4DI e790 = {{ 0xff80808080808080, 0xff01010101010101, 0xff01010101010101, 0xff80808080808080 }};
	static const V4DI e791 = {{ 0xffffc0c0c0c0c0c0, 0xffff030303030303, 0xffff030303030303, 0xffffc0c0c0c0c0c0 }};
	static const V4DI e792 = {{ 0xfffffffff0f0f0f0, 0xffffffff0f0f0f0f, 0xffffffff0f0f0f0f, 0xfffffffff0f0f0f0 }};

	l81 = _mm_cvtsi64_si128(disc);				        v4_disc = _mm256_castsi128_si256(_mm_shuffle_epi8(l81, mcpyswap));
	l81 = _mm_cmpeq_epi8(kff, l81);				        v4_disc = _mm256_permute4x64_epi64(v4_disc, 0x50);	// disc, disc, rdisc, rdisc
	                                                    lr79 = _mm256_and_si256(v4_disc, _mm256_or_si256(e790.v4, _mm256_srlv_epi64(v4_disc, shiftlr[0].v4)));
	l8 = _mm256_castsi256_si128(v4_disc);               lr79 = _mm256_and_si256(lr79, _mm256_or_si256(e791.v4, _mm256_srlv_epi64(lr79, shiftlr[1].v4)));
	l8 = _mm_and_si128(l8, _mm_alignr_epi8(l8, l8, 1));	lr79 = _mm256_and_si256(lr79, _mm256_or_si256(e792.v4, _mm256_srlv_epi64(lr79, shiftlr[2].v4)));
	l8 = _mm_and_si128(l8, _mm_alignr_epi8(l8, l8, 2));	l79 = _mm_shuffle_epi8(_mm256_extracti128_si256(lr79, 1), mbswapll);
	l8 = _mm_and_si128(l8, _mm_alignr_epi8(l8, l8, 4));	l79 = _mm_and_si128(l79, _mm256_castsi256_si128(lr79));

	l81 = _mm_unpacklo_epi64(l81, l8);
	return _mm256_insertf128_si256(_mm256_castsi128_si256(l81), l79, 1);
}

#else

uint64_t get_full_lines(const uint64_t disc, uint64_t full[4])
{
#if USE_SIMD && defined(__SSE2__)

	uint64_t rdisc = vertical_mirror(disc);
	uint64_t l8;
	__m128i l01, l79, r79;	// full lines
	const __m128i kff  = _mm_set1_epi8(-1);
	const __m128i e790 = _mm_set1_epi64x(0xff80808080808080);
	const __m128i e791 = _mm_set1_epi64x(0x01010101010101ff);
	const __m128i e792 = _mm_set1_epi64x(0x00003f3f3f3f3f3f);
	const __m128i e793 = _mm_set1_epi64x(0x0f0f0f0ff0f0f0f0);

	l01 = l79 = _mm_cvtsi64_si128(disc);	l79 = r79 = _mm_unpacklo_epi64(l79, _mm_cvtsi64_si128(rdisc));
	l01 = _mm_cmpeq_epi8(kff, l01);         l79 = _mm_and_si128(l79, _mm_or_si128(e790, _mm_srli_epi64(l79, 9)));
	_mm_storel_epi64((__m128i*) &full[0], l01);
	r79 = _mm_and_si128(r79, _mm_or_si128(e791, _mm_slli_epi64(r79, 9)));
	l8 = disc;                              l79 = _mm_andnot_si128(_mm_andnot_si128(_mm_srli_epi64(l79, 18), e792), l79);
	l8 &= (l8 >> 8) | (l8 << 56);           r79 = _mm_andnot_si128(_mm_slli_epi64(_mm_andnot_si128(r79, e792), 18), r79);
	l8 &= (l8 >> 16) | (l8 << 48);          l79 = _mm_and_si128(_mm_and_si128(l79, r79), _mm_or_si128(e793, _mm_or_si128(_mm_srli_epi64(l79, 36), _mm_slli_epi64(r79, 36))));
	l8 &= (l8 >> 32) | (l8 << 32);          _mm_storel_epi64((__m128i *) &full[3], l79);
	full[1] = l8;                           full[2] = vertical_mirror(_mm_cvtsi128_si64(_mm_unpackhi_epi64(l79, l79)));

#elif 0 && USE_SIMD && defined(__ARM_NEON)

	uint64_t v;
	uint8x8_t h;
	uint64x2_t l79, r79;
	const uint64x2_t e790 = vdupq_n_u64(0x007f7f7f7f7f7f7f);
	const uint64x2_t e791 = vdupq_n_u64(0xfefefefefefefe00);
	const uint64x2_t e792 = vdupq_n_u64(0x00003f3f3f3f3f3f);
	const uint64x2_t e793 = vdupq_n_u64(0x0f0f0f0ff0f0f0f0);

	h = vcreate_u8(disc);                   l79 = r79 = vreinterpretq_u64_u8(vcombine_u8(h, vrev64_u8(h)));
	h = vceq_u8(h, vdup_n_u8(0xff));        l79 = vandq_u64(l79, vornq_u64(vshrq_n_u64(l79, 9), e790));
	full[0] = vget_lane_u64(vreinterpret_u64_u8(h), 0);
	                                        r79 = vandq_u64(r79, vornq_u64(vshlq_n_u64(r79, 9), e791));
	v = disc;                               l79 = vbicq_u64(l79, vbicq_u64(e792, vshrq_n_u64(l79, 18)));
	v &= (v >> 8) | (v << 56);              r79 = vbicq_u64(r79, vshlq_n_u64(vbicq_u64(e792, r79), 18));
	v &= (v >> 16) | (v << 48);             l79 = vandq_u64(vandq_u64(l79, r79), vorrq_u64(e793, vsliq_n_u64(vshrq_n_u64(l79, 36), r79, 36)));
	v &= (v >> 32) | (v << 32);             full[2] = vertical_mirror(vgetq_lane_u64(l79, 1));
	full[1] = v;                            full[3] = vgetq_lane_u64(l79, 0);

#else

	uint64_t h = disc, v = disc, l7 = disc, l9 = disc, r7 = disc, r9 = disc;

	h &= h >> 1;
	h &= h >> 2;
	h &= h >> 4;
	full[0] = (h & 0x0101010101010101) * 0xff;

	v &= (v >> 8)  | (v << 56);	// ror 8
	v &= (v >> 16) | (v << 48);	// ror 16
	v &= (v >> 32) | (v << 32);	// ror 32
	full[1] = v;

	l7 &= 0xff01010101010101 | (l7 >> 7);	r7 &= 0x80808080808080ff | (r7 << 7);
	l7 &= 0xffff030303030303 | (l7 >> 14);	r7 &= 0xc0c0c0c0c0c0ffff | (r7 << 14);
	l7 &= 0xffffffff0f0f0f0f | (l7 >> 28);	r7 &= 0xf0f0f0f0ffffffff | (r7 << 28);
	full[2] = l7 & r7;

	l9 &= 0xff80808080808080 | (l9 >> 9);	r9 &= 0x01010101010101ff | (r9 << 9);
	l9 &= 0xffffc0c0c0c0c0c0 | (l9 >> 18);	r9 &= 0x030303030303ffff | (r9 << 18);
	full[3] = l9 & r9 & (0x0f0f0f0ff0f0f0f0 | (l9 >> 36) | (r9 << 36));

#endif

	return (full[0] & full[1] & full[2] & full[3]);
}

#endif

/**
 * @brief Get stable discs by contact with other stable discs.
 *
 * @param P bitboard with player's discs.
 * @param O bitboard with opponent's discs.
 * @return the number of stable discs.
 */
// too slow on zen3 cpu
#if 0 && USE_SIMD && defined(__AVX2__)

static uint64_t vectorcall get_stable_by_contact(const uint64_t central_mask, const uint64_t previous_stable, const __m256i full)
{
	__m128i	stable, old_stable, central_mask_v2;
	__m256i	stable_v4;
	const __m256i dir = _mm256_set_epi64x(7, 9, 8, 1);

	if (previous_stable == 0) return 0;

	stable = _mm_cvtsi64_si128(previous_stable);
	central_mask_v2 = _mm_cvtsi64_si128(central_mask);
	do {
		old_stable = stable;
		stable_v4 = _mm256_broadcastq_epi64(stable);
		stable_v4 = _mm256_or_si256(_mm256_or_si256(_mm256_srlv_epi64(stable_v4, dir), _mm256_sllv_epi64(stable_v4, dir)), full);
		stable = _mm_and_si128(_mm256_castsi256_si128(stable_v4), _mm256_extracti128_si256(stable_v4, 1));
		stable = _mm_and_si128(stable, _mm_unpackhi_epi64(stable, stable));
		stable = _mm_or_si128(old_stable, _mm_and_si128(stable, central_mask_v2));
	} while (!_mm_testc_si128(old_stable, stable));

	return _mm_cvtsi128_si64(stable);
}

#else

static uint64_t get_stable_by_contact(const uint64_t central_mask, const uint64_t previous_stable, const uint64_t full[4])
{
	uint64_t stable_h, stable_v, stable_d7, stable_d9;
	uint64_t old_stable = 0, stable = previous_stable;

	while (stable != old_stable) {
		old_stable = stable;
		stable_h = ((stable >> 1) | (stable << 1) | full[0]);
		stable_v = ((stable >> 8) | (stable << 8) | full[1]);
		stable_d7 = ((stable >> 7) | (stable << 7) | full[2]);
		stable_d9 = ((stable >> 9) | (stable << 9) | full[3]);
		stable |= (stable_h & stable_v & stable_d7 & stable_d9 & central_mask);
	}

	return stable;
}

#endif

/**
 * @brief Estimate the stable discs.
 *
 * Return the stable discs.
 *
 * @param P bitboard with player's discs.
 * @param O bitboard with opponent's discs.
 * @return the stable discs.
 */
// too slow on zen3 cpu
#if 0 && USE_SIMD && defined(__AVX2__)
uint64_t get_stable_discs(const uint64_t P, const uint64_t O)
{
	const uint64_t disc = (P | O);
	const uint64_t central_mask = P & 0x007e7e7e7e7e7e00;
	uint64_t stable;

	// compute the exact stable edges
	stable = get_stable_edge(P, O);

	// add full lines
	__m256i	full_v4 = get_full_lines(disc);
	__m128i full_v2 = _mm_and_si128(_mm256_castsi256_si128(full_v4), _mm256_extracti128_si256(full_v4, 1));
	stable |= (central_mask & _mm_cvtsi128_si64(_mm_and_si128(full_v2, _mm_unpackhi_epi64(full_v2, full_v2))));

	// compute the other stable discs
	stable = get_stable_by_contact(central_mask, stable, full_v4);

	return stable;
}

#else

uint64_t get_stable_discs(const uint64_t P, const uint64_t O)
{
	const uint64_t disc = (P | O);
	const uint64_t central_mask = (P & 0x007e7e7e7e7e7e00ULL);
	uint64_t full[4], stable;

	// compute the exact stable edges (from precomputed tables)
	stable = get_stable_edge(P, O);

	// add full lines
	stable |= get_full_lines(disc, full) & central_mask;

	// now compute the other stable discs (ie discs touching another stable disc in each flipping direction).
	stable = get_stable_by_contact(central_mask, stable, full);

	return stable;
}
#if USE_SOLID
uint64_t get_stable_full_discs(const uint64_t P, const uint64_t O, uint64_t* F)
{
	const uint64_t disc = (P | O);
	const uint64_t central_mask = (P & 0x007e7e7e7e7e7e00ULL);
	uint64_t full[4], stable;

	// compute the exact stable edges (from precomputed tables)
	stable = get_stable_edge(P, O);

	// add full lines
	*F = get_full_lines(disc, full);
	stable |= *F & central_mask;

	// now compute the other stable discs (ie discs touching another stable disc in each flipping direction).
	stable = get_stable_by_contact(central_mask, stable, full);

	return stable;
}
#endif

#endif

/**
 * @brief Estimate the stability.
 *
 * Count the number (in fact a lower estimate) of stable discs.
 *
 * @param P bitboard with player's discs.
 * @param O bitboard with opponent's discs.
 * @return the number of stable discs.
 */
int get_stability(const uint64_t P, const uint64_t O)
{
	return bit_count(get_stable_discs(P, O));
}
#if USE_SOLID
int get_stability_full(const uint64_t P, const uint64_t O, uint64_t *F)
{
	return bit_count(get_stable_full_discs(P, O, F));
}
#endif

/**
 * @brief Estimate the stability of edges.
 *
 * Count the number (in fact a lower estimate) of stable discs on the edges.
 *
 * @param P bitboard with player's discs.
 * @param O bitboard with opponent's discs.
 * @return the number of stable discs on the edges.
 */
int get_edge_stability(const uint64_t P, const uint64_t O)
{
#if USE_SIMD && defined(__ARM_NEON)

	const int16x8_t shiftv = { 0, 1, 2, 3, 4, 5, 6, 7 };	// error on MSVC
	uint8x16_t PO = vzip1q_u8(vreinterpretq_u8_u64(vdupq_n_u64(O)), vreinterpretq_u8_u64(vdupq_n_u64(P)));
	uint8x8_t packedstable = vcreate_u8((EDGE_STABILITY[vgetq_lane_u16(vreinterpretq_u16_u8(PO), 0)]
	  | EDGE_STABILITY[vgetq_lane_u16(vreinterpretq_u16_u8(PO), 7)] << 8) & 0x7e7e);
	packedstable = vset_lane_u8(EDGE_STABILITY[vaddvq_u16(vshlq_u16(vreinterpretq_u16_u8(vandq_u8(PO, vdupq_n_u8(1))), shiftv))], packedstable, 2);
	packedstable = vset_lane_u8(EDGE_STABILITY[vaddvq_u16(vshlq_u16(vreinterpretq_u16_u8(vshrq_n_u8(PO, 7)), shiftv))], packedstable, 3);
	return vaddv_u8(vcnt_u8(packedstable));

#else

	return bit_count(get_stable_edge(P, O));

#endif
}

/**
 * @brief Estimate corner stability.
 *
 * Count the number of stable discs around the corner. Limiting the count
 * to the corner keep the function fast but still get this information,
 * particularly important at Othello. Corner stability will be used for
 * move sorting.
 *
 * @param P bitboard with player's discs.
 * @return the number of stable discs around the corner.
 */
int get_corner_stability(const uint64_t P)
{
	// NB: other algorithms from 4.5.3 are slower on a zen3 CPU
	const uint64_t stable = ((((0x0100000000000001ULL & P) << 1) | ((0x8000000000000080ULL & P) >> 1)
	                      | ((0x0000000000000081ULL & P) << 8) | ((0x8100000000000000ULL & P) >> 8)
	                      | 0x8100000000000081ULL) & P);
	return bit_count(stable);
}

/**
 * @brief Compute a hash code.
 *
 * @param board the board.
 * @return the hash code of the bitboard
 */
uint64_t board_get_hash_code(const Board *board)
{
	const uint64_t crc = crc32c_u64(0, board->player);
	return (crc << 32) | crc32c_u64(crc, board->opponent);
}

/**
 * @brief Get square color.
 *
 * returned value: 0 = player, 1 = opponent, 2 = empty;
 *
 * @param board board.
 * @param x square coordinate.
 * @return square color.
 */
int board_get_square_color(const Board *board, const int x)
{
	return 2 - 2 * ((board->player >> x) & 1) - ((board->opponent >> x) & 1);
}

/**
 * @brief Check if a square is occupied.
 *
 * @param board board.
 * @param x square coordinate.
 * @return true if a square is occupied.
 */
bool board_is_occupied(const Board *board, const int x)
{
	return ((board->player | board->opponent) & x_to_bit(x)) != 0;
}

/**
 * @brief Check if current player should pass.
 *
 * @param board board.
 * @return true if player is passing, false otherwise.
 */
bool board_is_pass(const Board *board)
{
	return can_move(board->player, board->opponent) == false &&
		can_move(board->opponent, board->player) == true;
}

/**
 * @brief Check if the game is over.
 *
 * @param board board.
 * @return true if game is over, false otherwise.
 */
bool board_is_game_over(const Board *board)
{
	return can_move(board->player, board->opponent) == false &&
		can_move(board->opponent, board->player) == false;
}


/**
 * @brief Check if the game is over.
 *
 * @param board board.
 * @return true if game is over, false otherwise.
 */
int board_count_empties(const Board *board)
{
	return bit_count(~(board->player | board->opponent));
}

/**
 * @brief Print out the board.
 *
 * Print an ASCII representation of the board to an output stream.
 *
 * @param board board to print.
 * @param player player's color.
 * @param f output stream.
 */
void board_print(const Board *board, const int player, FILE *f)
{
	int i, j, square, x;
	const char *color = &"?*O-."[1];
	uint64_t moves = board_get_moves(board);

	fputs("  A B C D E F G H\n", f);
	for (i = 0; i < 8; ++i) {
		fputc(i + '1', f);
		fputc(' ', f);
		for (j = 0; j < 8; ++j) {
			x = i * 8 + j;
			if (player == BLACK) square = 2 - ((board->opponent >> x) & 1) - 2 * ((board->player >> x) & 1);
			else square = 2 - ((board->player >> x) & 1) - 2 * ((board->opponent >> x) & 1);
			if (square == EMPTY && (moves & x_to_bit(x))) ++square;
			fputc(color[square], f);
			fputc(' ', f);
		}
		fputc(i + '1', f);
		if (i == 1)
			fprintf(f, " %c to move", color[player]);
		else if (i == 3)
			fprintf(f, " %c: discs = %2d    moves = %2d",
				color[player], bit_count(board->player), get_mobility(board->player, board->opponent));
		else if (i == 4)
			fprintf(f, " %c: discs = %2d    moves = %2d",
				color[!player], bit_count(board->opponent), get_mobility(board->opponent, board->player));
		else if (i == 5)
			fprintf(f, "  empties = %2d      ply = %2d",
				64 - bit_count(board->opponent|board->player), bit_count(board->opponent|board->player) - 3);
		fputc('\n', f);
	}
	fputs("  A B C D E F G H\n", f);
}

/**
 * @brief convert the to a compact string.
 *
 * @param board board to convert.
 * @param player player's color.
 * @param s output string.
 */
char* board_to_string(const Board *board, const int player, char *s)
{
	int square, x;
	const char *color = "XO-?";

	for (x = 0; x < 64; ++x) {
		if (player == BLACK) square = 2 - ((board->opponent >> x) & 1) - 2 * ((board->player >> x) & 1);
		else square = 2 - ((board->player >> x) & 1) - 2 * ((board->opponent >> x) & 1);
		s[x] = color[square];
	}
	s[64] = ' ';
	s[65] = color[player];
	s[66] = '\0';
	return s;
}

/**
 * @brief print using FEN description.
 *
 * Write the board according to the Forsyth-Edwards Notation.
 *
 * @param board the board to write
 * @param player turn's color.
 * @param f output stream.
 */
void board_print_FEN(const Board *board, const int player, FILE *f)
{
	char s[256];
	fputs(board_to_FEN(board, player, s), f);
}

/**
 * @brief print to FEN description.
 *
 * Write the board into a Forsyth-Edwards Notation string.
 *
 * @param board the board to write
 * @param player turn's color.
 * @param string output string.
 */
char* board_to_FEN(const Board *board, const int player, char *string)
{
	int square, x, r, c;
	const char *piece = "pP-?";
	const char *color = "bw";
	int n_empties = 0;
	char *s = string;
	static char local_string[128];

	if (s == NULL) s = string = local_string;

	for (r = 7; r >= 0; --r)
	for (c = 0; c < 8; ++c) {
		if (c == 0 && r < 7) {
			if (n_empties) {
				*s++ = n_empties + '0';
				n_empties = 0;
			}
			*s++ = '/';
		}
		x = 8 * r + c;
		if (player == BLACK) square = 2 - ((board->opponent >> x) & 1) - 2 * ((board->player >> x) & 1);
		else square = 2 - ((board->player >> x) & 1) - 2 * ((board->opponent >> x) & 1);

		if (square == EMPTY) {
			++n_empties;
		} else {
			if (n_empties) {
				*s++ = n_empties + '0';
				n_empties = 0;
			}
			*s++ = piece[square];
		}
	}
	if (n_empties) {
		*s++ = n_empties + '0';
		n_empties = 0;
	}
	*s++ = ' ';
	*s++ = color[player];
	*s++ = ' '; *s++ = '-'; *s++ = ' '; *s++ = '-';
	*s++ = ' '; *s++ = '0'; *s++ = ' '; *s++ = '1'; *s = '\0';

	return string;
}

/**
 * @brief test if a board operation meet its expectation
 *
 * @param out board result
 * @param expected board expected
 * @param test function tested *
 */
void board_test_check(Board *out, Board *expected, char *test)
{
	if (!board_equal(out, expected)) {
		fprintf(stderr, "%s failed. Got:\n", test);
		board_print(out, BLACK, stderr);
		fprintf(stderr, "instead of:\n");
		board_print(expected, BLACK, stderr);
		abort();
	}
}

void board_test(void) {
	Board in = {0x000804000b030120, 0x0004113ef41c1c14}, out;
	Board sym[] = {
		{0x000804000b030120, 0x0004113ef41c1c14},
		{0x00102000d0c08004, 0x0020887c2f383828},
		{0x2001030b00040800, 0x141c1cf43e110400},
		{0x0480c0d000201000, 0x2838382f7c882000},
		{0x0000010048200c0e, 0x0808183f165f1020},
		{0x0e0c204800010000, 0x20105f163f180808},
		{0x0000800012043070, 0x101018fc68fa0804},
		{0x7030041200800000, 0x0408fa68fc181010},
	};

	board_set(&out, "--O-OX--X-OOO---XXOOO---XXOXOOOO-OOOOO--O-X-O-----OX------------ X");
	board_test_check(&in, &out, "board_set");

	for (int i = 0; i < 8; ++i) {
		board_symetry(&in, i, &out);
		board_test_check(&out, &sym[i], "board_symetry");
	}
	board_unique(&in, &out);
	board_test_check(&out, &sym[4], "board_unique");

	Board board = { 18304334016151747588ul, 142410057557804025ul };
	int n_flip = count_last_flip(B1, board.player);
	if (n_flip != 2) {
		board_print(&board, 0, stderr);
		fprintf(stderr, "n_flip = %d != 2\n", n_flip);
	}

	fprintf(stderr, "board_test done\n");
}

