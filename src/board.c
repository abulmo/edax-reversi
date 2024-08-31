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
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
 * @date 1998 - 2024
=======
 * @date 1998 - 2017
>>>>>>> b3f048d (copyright changes)
=======
 * @date 1998 - 2018
>>>>>>> 1dc032e (Improve visual c compatibility)
=======
 * @date 1998 - 2020
>>>>>>> 4b9f204 (minor optimize in search_eval_1/2 and search_shallow)
=======
 * @date 1998 - 2021
>>>>>>> 34a2291 (4.5.0: Use CRC32c for board hash)
=======
 * @date 1998 - 2022
>>>>>>> 9e2bbc5 (split get_all_full_lines from get_stability)
=======
 * @date 1998 - 2023
>>>>>>> 8566ed0 (vector call version of board_next & get_moves)
 * @author Richard Delorme
 * @author Toshihiko Okuhara
 * @version 4.5
 */

#include "board.h"

#include "bit.h"
#include "hash.h"
#include "move.h"
#include "util.h"

#include <ctype.h>
#include <stdlib.h>
#include <assert.h>


#if MOVE_GENERATOR == MOVE_GENERATOR_CARRY
	#include "flip_carry_64.c"
#elif MOVE_GENERATOR == MOVE_GENERATOR_SSE
	#include "flip_sse.c"
#elif MOVE_GENERATOR == MOVE_GENERATOR_BITSCAN
<<<<<<< HEAD
  #ifdef __ARM_NEON
	#define	flip_neon	flip
	#include "flip_neon_bitscan.c"
  #else
	#include "flip_bitscan.c"
  #endif
=======
	#ifdef hasNeon
		#define	flip_neon	flip
		#include "flip_neon_bitscan.c"
	#else
		#include "flip_bitscan.c"
	#endif
>>>>>>> 343493d (More neon/sse optimizations; neon dispatch added for arm32)
#elif MOVE_GENERATOR == MOVE_GENERATOR_ROXANE
	#include "flip_roxane.c"
#elif MOVE_GENERATOR == MOVE_GENERATOR_32
	#include "flip_carry_sse_32.c"
#elif MOVE_GENERATOR == MOVE_GENERATOR_SSE_BSWAP
	#include "flip_sse_bswap.c"
#elif MOVE_GENERATOR == MOVE_GENERATOR_AVX
	#include "flip_avx_ppfill.c"
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
#elif MOVE_GENERATOR == MOVE_GENERATOR_AVX512
	#include "flip_avx512cd.c"
#elif MOVE_GENERATOR == MOVE_GENERATOR_NEON
<<<<<<< HEAD
  #ifdef __aarch64__
	#include "flip_neon_rbit.c"
  #else
	#include "flip_neon_lzcnt.c"
  #endif
#elif MOVE_GENERATOR == MOVE_GENERATOR_SVE
	#include "flip_sve_lzcnt.c"
=======
>>>>>>> cb149ab (Faster flip_avx (ppfill) and variants added)
=======
=======
#elif MOVE_GENERATOR == MOVE_GENERATOR_AVX512
	#include "flip_avx512cd.c"
>>>>>>> 393b667 (Experimental AVX512VL/CD version of move generator)
#elif MOVE_GENERATOR == MOVE_GENERATOR_NEON
	#include "flip_neon_lzcnt.c"
>>>>>>> f2da03e (Refine arm builds adding neon support.)
=======
	#ifdef __aarch64__
		#include "flip_neon_rbit.c"
	#else
		#include "flip_neon_lzcnt.c"
	#endif
>>>>>>> 343493d (More neon/sse optimizations; neon dispatch added for arm32)
#else // MOVE_GENERATOR == MOVE_GENERATOR_KINDERGARTEN
	#include "flip_kindergarten.c"
#endif

<<<<<<< HEAD
<<<<<<< HEAD
=======
#if LAST_FLIP_COUNTER == COUNT_LAST_FLIP_CARRY
	#include "count_last_flip_carry_64.c"
#elif LAST_FLIP_COUNTER == COUNT_LAST_FLIP_SSE
	#include "count_last_flip_sse.c"
#elif LAST_FLIP_COUNTER == COUNT_LAST_FLIP_BITSCAN
	#include "count_last_flip_bitscan.c"
#elif LAST_FLIP_COUNTER == COUNT_LAST_FLIP_PLAIN
	#include "count_last_flip_plain.c"
#elif LAST_FLIP_COUNTER == COUNT_LAST_FLIP_32
	#include "count_last_flip_32.c"
#elif LAST_FLIP_COUNTER == COUNT_LAST_FLIP_BMI2
	#include "count_last_flip_bmi2.c"
#else // LAST_FLIP_COUNTER == COUNT_LAST_FLIP_KINDERGARTEN
	#include "count_last_flip_kindergarten.c"
#endif

>>>>>>> feb7fa7 (count_last_flip_bmi2 and transpose_avx2 added)
=======
>>>>>>> 6506166 (More SSE optimizations)

/** edge stability global data */
unsigned char edge_stability[256 * 256];

<<<<<<< HEAD
<<<<<<< HEAD
#if (defined(USE_GAS_MMX) || defined(USE_MSVC_X86)) && !defined(hasSSE2)
	#include "board_mmx.c"
#endif
#if (defined(USE_GAS_MMX) || defined(USE_MSVC_X86) || defined(hasSSE2) || defined(__ARM_NEON)) && !defined(ANDROID)
	#include "board_sse.c"
=======
/** conversion from an 8-bit line to the A1-A8 line */
// unsigned long long A1_A8[256];

=======
>>>>>>> 9e2bbc5 (split get_all_full_lines from get_stability)
#if defined(USE_GAS_MMX) || defined(USE_MSVC_X86)
#include "board_mmx.c"
#endif
#if (defined(USE_GAS_MMX) || defined(USE_MSVC_X86) || defined(hasSSE2) || defined(hasNeon)) && !defined(ANDROID)
#include "board_sse.c"
>>>>>>> 1dc032e (Improve visual c compatibility)
#endif


/**
 * @brief Swap players.
 *
 * Swap players, i.e. change player's turn.
 *
 * @param board board
 */
void board_swap_players(Board *board)
{
	const unsigned long long tmp = board->player;
	board->player = board->opponent;
	board->opponent = tmp;
}

/**
 * @brief Set a board from a string description.
 *
 * Read a standardized string (See http://www.nada.kth.se/~gunnar/download2.html
 * for details) and translate it into our internal Board structure.
 *
 * @param board the board to set
 * @param string string describing the board
 * @return turn's color.
 */
int board_set(Board *board, const char *s)
{
	int i;
	unsigned long long b = 1;

	board->player = board->opponent = 0;
	for (i = A1; (i <= H8) && (*s != '\0'); ++s) {
		switch (tolower(*s)) {
		case 'b':
		case 'x':
		case '*':
			board->player |= b;
			break;
		case 'o':
		case 'w':
			board->opponent |= b;
			break;
		case '-':
		case '.':
			break;
		default:
			continue;
		}
		++i;
		b <<= 1;
	}
	board_check(board);

	for (; *s != '\0'; ++s) {
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

	warn("board_set: bad string input\n");
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
	board->player   = 0x0000000810000000; // BLACK
	board->opponent = 0x0000001008000000; // WHITE
}

/**
 * @brief Check board consistency
 *
 * @param board the board to initialize
 */
void board_check(const Board *board)
{
#ifndef NDEBUG
	if (board->player & board->opponent) {
		error("Two discs on the same square?\n");
		board_print(board, BLACK, stderr);
		bitboard_write(board->player, stderr);
		bitboard_write(board->opponent, stderr);
		abort();
	}

	// empty center ?
	if (~(board->player|board->opponent) & 0x0000001818000000) {
		error("Empty center?\n");
		board_print(board, BLACK, stderr);
	}
#else
	(void) board;
#endif // NDEBUG
}

/**
 * @brief Compare two board
 *
 * @param b1 first board
 * @param b2 second board
 * @return true if b1 is lesser than b2
 */
bool board_lesser(const Board *b1, const Board *b2)
{
	if (b1->player != b2->player)
		return (b1->player < b2->player);
	else	return (b1->opponent < b2->opponent);
<<<<<<< HEAD
=======
}

<<<<<<< HEAD
/**
 * @brief Compare two board for equality
 *
 * @param b1 first board
 * @param b2 second board
 * @return true if both board are equal
 */
bool board_equal(const Board *b1, const Board *b2)
{
	return (b1->player == b2->player && b1->opponent == b2->opponent);
>>>>>>> 8a7e354 (Exclude hash init time from count games; other minor size opts)
}

=======
>>>>>>> de58f52 (AVX2 board_equal; delayed hash lock code)
#if !defined(hasSSE2) && !defined(hasNeon)	// SSE version in board_sse.c
/**
 * @brief symetric board
 *
 * @param board input board
 * @param s symetry
 * @param sym symetric output board
 */
#if !defined(hasSSE2) && !defined(__ARM_NEON)	// SSE version in board_sse.c
void board_horizontal_mirror(const Board *board, Board *sym)
{
	sym->player = horizontal_mirror(board->player);
	sym->opponent = horizontal_mirror(board->opponent);
}

void board_vertical_mirror(const Board *board, Board *sym)
{
	sym->player = vertical_mirror(board->player);
	sym->opponent = vertical_mirror(board->opponent);
}

void board_transpose(const Board *board, Board *sym)
{
	sym->player = transpose(board->player);
	sym->opponent = transpose(board->opponent);
}

void board_symetry(const Board *board, const int s, Board *sym)
{
<<<<<<< HEAD
<<<<<<< HEAD
	*sym = *board;
	if (s & 1)
		board_horizontal_mirror(sym, sym);
	if (s & 2)
		board_vertical_mirror(sym, sym);
	if (s & 4)
		board_transpose(sym, sym);
=======
	register unsigned long long player, opponent;
=======
	unsigned long long player, opponent;
>>>>>>> 1b29848 (fix & optimize 32 bit build; other minor mods)

	player = board->player;
	opponent = board->opponent;

	if (s & 1) {
		player = horizontal_mirror(player);
		opponent = horizontal_mirror(opponent);
	}
	if (s & 2) {
		player = vertical_mirror(player);
		opponent = vertical_mirror(opponent);
	}
	if (s & 4) {
		player = transpose(player);
		opponent = transpose(opponent);
	}

	sym->player = player;
	sym->opponent = opponent;
>>>>>>> dbeab1c (reduce asm and inline which sometimes breaks debug build)

	board_check(sym);
}
#endif

#ifndef __AVX2__	// AVX2 version in board_sse.c
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
	Board sym[8];
	int i, s = 0;

	board_horizontal_mirror(board, &sym[1]);
	board_vertical_mirror(board, &sym[2]);
	board_vertical_mirror(&sym[1], &sym[3]);
	board_transpose(board, &sym[4]);
	board_vertical_mirror(&sym[4], &sym[5]);	// v-h reverted
	board_horizontal_mirror(&sym[4], &sym[6]);
	board_vertical_mirror(&sym[6], &sym[7]);

	*unique = *board;
	for (i = 1; i < 8; ++i) {
		// board_symetry(board, i, &sym);	// moved to before loop to minimize symetry ops
		if (board_lesser(&sym[i], unique)) {
			*unique = sym[i];
			s = i;
		}
	}

	board_check(unique);
	return s;
}
#endif

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
	unsigned long long moves;
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
<<<<<<< HEAD
<<<<<<< HEAD
		board_get_move_flip(board, get_rand_bit(moves, r), &move);
=======
		board_get_move(board, get_rand_bit(moves, r), &move);
>>>>>>> 0a166fd (Remove 1 element array coding style)
=======
		board_get_move_flip(board, get_rand_bit(moves, r), &move);
>>>>>>> 80ca4b1 (board_get_moves for AVX2; rename board_get_move_flip)
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
unsigned long long board_get_move_flip(const Board *board, const int x, Move *move)
{
<<<<<<< HEAD
<<<<<<< HEAD
=======
	move->flipped = board_flip(board, x);
>>>>>>> 6506166 (More SSE optimizations)
=======
>>>>>>> 542ee82 (Change store order to reduce register saving)
	move->x = x;
	move->flipped = board_flip(board, x);
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
	else if (x_to_bit(move->x) & (board->player | board->opponent)) return false;
	else if (move->flipped != board_flip(board, move->x)) return false;
	else return true;
}

<<<<<<< HEAD
<<<<<<< HEAD
=======
#if !(defined(hasMMX) && (defined(USE_GAS_MMX) || defined(USE_MSVC_X86)))
>>>>>>> 1dc032e (Improve visual c compatibility)
=======
#if !(defined(hasMMX) && (defined(USE_GAS_MMX) || defined(USE_MSVC_X86)))	// 32bit MMX/SSE version in board_mmx.c
>>>>>>> 6506166 (More SSE optimizations)
/**
 * @brief Update a board.
 *
 * Update a board by flipping its discs and updating every other data,
 * according to the 'move' description.
 *
 * @param board the board to modify
 * @param move  A Move structure describing the modification (may be PASS).
 */
void board_update(Board *board, const Move *move)
{
<<<<<<< HEAD
#if defined(hasSSE2) && (defined(HAS_CPU_64) || !defined(__3dNOW__))	// 3DNow CPU has fast emms, and possibly slow SSE
	__m128i	OP = _mm_loadu_si128((__m128i *) board);
	OP = _mm_xor_si128(OP, _mm_or_si128(_mm_set1_epi64x(move->flipped), _mm_loadl_epi64((__m128i *) &X_TO_BIT[move->x])));
	_mm_storeu_si128((__m128i *) board, _mm_shuffle_epi32(OP, 0x4e));

#elif defined(hasMMX)
	__m64	F = *(__m64 *) &move->flipped;
	__m64	P = _m_pxor(*(__m64 *) &board->player, _m_por(F, *(__m64 *) &X_TO_BIT[move->x]));
	__m64	O = _m_pxor(*(__m64 *) &board->opponent, F);
	*(__m64 *) &board->player = O;
	*(__m64 *) &board->opponent = P;
	_mm_empty();

#else
	unsigned long long O = board->opponent;
	board->opponent = board->player ^ (move->flipped | X_TO_BIT[move->x]);
	board->player = O ^ move->flipped;
#endif
=======
	unsigned long long O = board->opponent;
	board->opponent = board->player ^ (move->flipped | X_TO_BIT[move->x]);
	board->player = O ^ move->flipped;
>>>>>>> 4b9f204 (minor optimize in search_eval_1/2 and search_shallow)
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
<<<<<<< HEAD
#if defined(hasSSE2) && (defined(HAS_CPU_64) || !defined(__3dNOW__))
	__m128i	OP = _mm_shuffle_epi32(_mm_loadu_si128((__m128i *) board), 0x4e);
	OP = _mm_xor_si128(OP, _mm_or_si128(_mm_set1_epi64x(move->flipped), _mm_loadl_epi64((__m128i *) &X_TO_BIT[move->x])));
	_mm_storeu_si128((__m128i *) board, OP);

#elif defined(hasMMX)
	__m64	F = *(__m64 *) &move->flipped;
	__m64	P = *(__m64 *) &board->opponent;
	__m64	O = *(__m64 *) &board->player;
	*(__m64 *) &board->player = _m_pxor(P, _m_por(F, *(__m64 *) &X_TO_BIT[move->x]));
	*(__m64 *) &board->opponent = _m_pxor(O, F);
	_mm_empty();

#else
	unsigned long long P = board->player;
	board->player = board->opponent ^ (move->flipped | X_TO_BIT[move->x]);
	board->opponent = P ^ move->flipped;
#endif
=======
	unsigned long long P = board->player;
	board->player = board->opponent ^ (move->flipped | X_TO_BIT[move->x]);
	board->opponent = P ^ move->flipped;
>>>>>>> 4b9f204 (minor optimize in search_eval_1/2 and search_shallow)
	board_check(board);
}
<<<<<<< HEAD
=======
#endif // hasMMX
>>>>>>> 1dc032e (Improve visual c compatibility)

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

<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
#if (MOVE_GENERATOR != MOVE_GENERATOR_AVX) && (MOVE_GENERATOR != MOVE_GENERATOR_AVX512) && (MOVE_GENERATOR != MOVE_GENERATOR_SSE) && (MOVE_GENERATOR != MOVE_GENERATOR_NEON)	// SSE version in board_sse.c
=======
#if !(defined(hasSSE2) && ((MOVE_GENERATOR == MOVE_GENERATOR_AVX) || (MOVE_GENERATOR == MOVE_GENERATOR_SSE)))	// SSE version in endgame_sse.c
>>>>>>> 6506166 (More SSE optimizations)
=======
#if (MOVE_GENERATOR != MOVE_GENERATOR_AVX) && (MOVE_GENERATOR != MOVE_GENERATOR_SSE) && (MOVE_GENERATOR != MOVE_GENERATOR_NEON)	// SSE version in board_sse.c
>>>>>>> 343493d (More neon/sse optimizations; neon dispatch added for arm32)
=======
#if (MOVE_GENERATOR != MOVE_GENERATOR_AVX) && (MOVE_GENERATOR != MOVE_GENERATOR_AVX512) && (MOVE_GENERATOR != MOVE_GENERATOR_SSE) && (MOVE_GENERATOR != MOVE_GENERATOR_NEON)	// SSE version in board_sse.c
>>>>>>> ff1c5db (skip hash access if n_moves <= 1 in NWS_endgame)
/**
 * @brief Compute a board resulting of a move played on a previous board.
 *
 * @param board board to play the move on.
 * @param x move to play (may be PASS).
 * @param next resulting board.
 * @return flipped discs.
 */
unsigned long long board_next(const Board *board, const int x, Board *next)
{
	const unsigned long long flipped = board_flip(board, x);
	const unsigned long long player = board->opponent ^ flipped;

	next->opponent = board->player ^ (flipped | X_TO_BIT[x]);
	next->player = player;

	return flipped;
}
<<<<<<< HEAD
#endif

<<<<<<< HEAD
#if !defined(hasSSE2) && !defined(__ARM_NEON)	// SSE version in board_sse.c
=======
/**
 * @brief Compute a board resulting of an opponent move played on a previous board.
 *
 * Compute the board after passing and playing a move.
 *
 * @param board board to play the move on.
 * @param x opponent move to play.
 * @param next resulting board.
 * @return flipped discs.
 */
unsigned long long board_pass_next(const Board *board, const int x, Board *next)
{
	const unsigned long long flipped = Flip(x, board->opponent, board->player);

	next->opponent = board->opponent ^ (flipped | x_to_bit(x));
	next->player = board->player ^ flipped;

	return flipped;
}
=======
>>>>>>> 23e04d1 (Backport endgame_sse optimizations into endgame.c)
#endif

<<<<<<< HEAD
<<<<<<< HEAD
#if !defined(__x86_64__) && !defined(_M_X64) && !defined(__AVX2__)
>>>>>>> 1b29848 (fix & optimize 32 bit build; other minor mods)
=======
#if !defined(__x86_64__) && !defined(_M_X64) && !defined(__AVX2__)	// sse version in board_sse.c
>>>>>>> 6506166 (More SSE optimizations)
=======
#if !defined(hasSSE2) && !defined(hasNeon)	// sse version in board_sse.c
>>>>>>> 343493d (More neon/sse optimizations; neon dispatch added for arm32)
/**
 * @brief Get a part of the moves.
 *
 * Partially compute a bitboard where each coordinate with a legal move is set to one.
 *
 * Two variants of the algorithm are provided, one based on Kogge-Stone parallel
 * prefix.
 *
 * @param P bitboard with player's discs.
 * @param mask bitboard with flippable opponent's discs.
 * @param dir flipping direction.
 * @return some legal moves in a 64-bit unsigned integer.
 */
static inline unsigned long long get_some_moves(const unsigned long long P, const unsigned long long mask, const int dir)
// x86 build will use helper for long long shift unless inlined
{

#if KOGGE_STONE & 1
	// kogge-stone algorithm
 	// 6 << + 6 >> + 12 & + 7 |
	// + better instruction independency
	unsigned long long flip_l, flip_r;
	unsigned long long mask_l, mask_r;
	int d;

	flip_l = flip_r = P;
	mask_l = mask_r = mask;
	d = dir;

	flip_l |= mask_l & (flip_l << d);   flip_r |= mask_r & (flip_r >> d);
	mask_l &= (mask_l << d);            mask_r &= (mask_r >> d);
	d <<= 1;
	flip_l |= mask_l & (flip_l << d);   flip_r |= mask_r & (flip_r >> d);
	mask_l &= (mask_l << d);            mask_r &= (mask_r >> d);
	d <<= 1;
	flip_l |= mask_l & (flip_l << d);   flip_r |= mask_r & (flip_r >> d);

	return ((flip_l & mask) << dir) | ((flip_r & mask) >> dir);

#elif PARALLEL_PREFIX & 1
	// 1-stage Parallel Prefix (intermediate between kogge stone & sequential) 
	// 6 << + 6 >> + 7 | + 10 &
	unsigned long long flip_l, flip_r;
	unsigned long long mask_l, mask_r;
	const int dir2 = dir + dir;

	flip_l  = mask & (P << dir);          flip_r  = mask & (P >> dir);
	flip_l |= mask & (flip_l << dir);     flip_r |= mask & (flip_r >> dir);
	mask_l  = mask & (mask << dir);       mask_r  = mask_l >> dir;
	flip_l |= mask_l & (flip_l << dir2);  flip_r |= mask_r & (flip_r >> dir2);
	flip_l |= mask_l & (flip_l << dir2);  flip_r |= mask_r & (flip_r >> dir2);

	return (flip_l << dir) | (flip_r >> dir);

#else
 	// sequential algorithm
 	// 7 << + 7 >> + 6 & + 12 |
	unsigned long long flip;

	flip = (((P << dir) | (P >> dir)) & mask);
	flip |= (((flip << dir) | (flip >> dir)) & mask);
	flip |= (((flip << dir) | (flip >> dir)) & mask);
	flip |= (((flip << dir) | (flip >> dir)) & mask);
	flip |= (((flip << dir) | (flip >> dir)) & mask);
	flip |= (((flip << dir) | (flip >> dir)) & mask);
	return (flip << dir) | (flip >> dir);

#endif
}

/**
 * @brief Get legal moves.
 *
 * Compute a bitboard where each coordinate with a legal move is set to one.
 *
 * @param P bitboard with player's discs.
 * @param O bitboard with opponent's discs.
 * @return all legal moves in a 64-bit unsigned integer.
 */
<<<<<<< HEAD
<<<<<<< HEAD
=======
#if !defined(__x86_64__) && !defined(_M_X64)
>>>>>>> 1dc032e (Improve visual c compatibility)
=======
>>>>>>> 1b29848 (fix & optimize 32 bit build; other minor mods)
unsigned long long get_moves(const unsigned long long P, const unsigned long long O)
{
	unsigned long long moves, OM;

<<<<<<< HEAD
<<<<<<< HEAD
	#if defined(USE_GAS_MMX) || defined(USE_MSVC_X86) || defined(DISPATCH_NEON)
=======
	#if defined(USE_GAS_MMX) || defined(USE_MSVC_X86)
>>>>>>> 1dc032e (Improve visual c compatibility)
	if (hasSSE2)
		return get_moves_sse(P, O);
<<<<<<< HEAD
	#endif
	#if defined(USE_GAS_MMX) || defined(USE_MSVC_X86)
	if (hasMMX)
=======
=======
	#if defined(USE_GAS_MMX) || defined(USE_MSVC_X86) || defined(ANDROID)
	if (hasSSE2)
		return get_moves_sse(P, O);
<<<<<<< HEAD
	#if defined(USE_GAS_MMX) || defined(USE_MSVC_X86)
>>>>>>> 343493d (More neon/sse optimizations; neon dispatch added for arm32)
	else if (hasMMX)
>>>>>>> 0f2fb39 (Chage 32-bit get_moves_mmx/sse parameters to 64 bits)
		return get_moves_mmx(P, O);
=======
>>>>>>> 6c3ed52 (Dogaishi hash reduction by Matsuo & Narazaki; edge-precise get_full_line)
	#endif
	#if defined(USE_GAS_MMX) || defined(USE_MSVC_X86)
	if (hasMMX)
		return get_moves_mmx(P, O);
	#endif

	OM = O & 0x7e7e7e7e7e7e7e7e;
	moves = ( get_some_moves(P, OM, 1) // horizontal
		| get_some_moves(P, O, 8)   // vertical
		| get_some_moves(P, OM, 7)   // diagonals
		| get_some_moves(P, OM, 9));

	return moves & ~(P|O);	// mask with empties
}
<<<<<<< HEAD
#endif // hasSSE2/__ARM_NEON
=======
#endif // hasSSE2/hasNeon
>>>>>>> 343493d (More neon/sse optimizations; neon dispatch added for arm32)

/**
 * @brief Get legal moves on a 6x6 board.
 *
 * Compute a bitboard where each coordinate with a legal move is set to one.
 *
 * @param P bitboard with player's discs.
 * @param O bitboard with opponent's discs.
 * @return all legal moves in a 64-bit unsigned integer.
 */
unsigned long long get_moves_6x6(const unsigned long long P, const unsigned long long O)
{
<<<<<<< HEAD
	unsigned long long PM = P & 0x007E7E7E7E7E7E00;
	unsigned long long OM = O & 0x007E7E7E7E7E7E00;
	return get_moves(PM, OM) & 0x007E7E7E7E7E7E00;
=======
	return get_moves(P & 0x007E7E7E7E7E7E00, O & 0x007E7E7E7E7E7E00) & 0x007E7E7E7E7E7E00;
>>>>>>> 6506166 (More SSE optimizations)
}

/**
 * @brief Check if a player can move.
 *
 * @param P bitboard with player's discs.
 * @param O bitboard with opponent's discs.
 * @return true or false.
 */
bool can_move(const unsigned long long P, const unsigned long long O)
{
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
#if defined(hasMMX) || defined(__ARM_NEON)
=======
#if defined(USE_GAS_MMX) || defined(__x86_64__) || defined(USE_MSVC_X86)
>>>>>>> 1dc032e (Improve visual c compatibility)
=======
#if defined(__x86_64__) || defined(_M_X64) || defined(hasMMX)
>>>>>>> 1b29848 (fix & optimize 32 bit build; other minor mods)
=======
#if defined(hasMMX) || defined(hasNeon)
>>>>>>> 343493d (More neon/sse optimizations; neon dispatch added for arm32)
	return get_moves(P, O) != 0;

#else
	const unsigned long long E = ~(P|O); // empties
	const unsigned long long OM = O & 0x7E7E7E7E7E7E7E7E;

	return (get_some_moves(P, OM, 7) & E)  // diagonals
		|| (get_some_moves(P, OM, 9) & E)
		|| (get_some_moves(P, OM, 1) & E)  // horizontal
		|| (get_some_moves(P, O, 8) & E); // vertical
#endif
}

/**
 * @brief Check if a player can move.
 *
 * @param P bitboard with player's discs.
 * @param O bitboard with opponent's discs.
 * @return true or false.
 */
bool can_move_6x6(const unsigned long long P, const unsigned long long O)
{
	return get_moves_6x6(P, O) != 0;
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
int get_mobility(const unsigned long long P, const unsigned long long O)
{
	return bit_count(get_moves(P, O));
}

<<<<<<< HEAD
#ifndef __AVX2__	// AVX2 version in board_sse.c
=======
int get_weighted_mobility(const unsigned long long P, const unsigned long long O)
{
	return bit_weighted_count(get_moves(P, O));
}

<<<<<<< HEAD
#ifndef __AVX2__
>>>>>>> be2ba1c (add AVX get_potential_mobility; revise foreach_bit for CPU32/C99)
=======
>>>>>>> 6a997c5 (new get_moves_and_potential for AVX2)
/**
 * @brief Get some potential moves.
 *
 * @param O bitboard with opponent's discs.
 * @param dir flipping direction.
 * @return some potential moves in a 64-bit unsigned integer.
 */
static inline unsigned long long get_some_potential_moves(const unsigned long long O, const int dir)
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
unsigned long long get_potential_moves(const unsigned long long P, const unsigned long long O)
{
	return (get_some_potential_moves(O & 0x7E7E7E7E7E7E7E7E, 1) // horizontal
		| get_some_potential_moves(O & 0x00FFFFFFFFFFFF00, 8)   // vertical
		| get_some_potential_moves(O & 0x007E7E7E7E7E7E00, 7)   // diagonals
		| get_some_potential_moves(O & 0x007E7E7E7E7E7E00, 9))
		& ~(P|O); // mask with empties
}
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
#endif // AVX2
=======
=======
#endif // AVX2
>>>>>>> be2ba1c (add AVX get_potential_mobility; revise foreach_bit for CPU32/C99)
=======
>>>>>>> 6a997c5 (new get_moves_and_potential for AVX2)

/**
 * @brief Get potential mobility.
 *
 * Count the list of empty squares in contact of a player square.
 *
 * @param P bitboard with player's discs.
 * @param O bitboard with opponent's discs.
 * @return a count of potential moves.
 */
int get_potential_mobility(const unsigned long long P, const unsigned long long O)
{
  #if defined(USE_GAS_MMX) || defined(USE_MSVC_X86)
	if (hasMMX)
		return get_potential_mobility_mmx(P, O);
  #endif
	return bit_weighted_count(get_potential_moves(P, O));
}
>>>>>>> 1dc032e (Improve visual c compatibility)

/**
 * @brief search stable edge patterns.
 *
 * Compute a 8-bit bitboard where each stable square is set to one
 *
 * @param old_P previous player edge discs.
 * @param old_O previous opponent edge discs.
 * @param stable 8-bit bitboard with stable edge squares.
 */
static int find_edge_stable(const int old_P, const int old_O, int stable)
{
	int P, O, O2, X, F;
	const int E = ~(old_P | old_O); // empties

	stable &= old_P; // mask stable squares with remaining player squares.
	if (!stable || E == 0) return stable;

	for (X = 0x01; X <= 0x80; X <<= 1) {
		if (E & X) { // is x an empty square ?
			O = old_O;
			P = old_P | X; // player plays on it
<<<<<<< HEAD
<<<<<<< HEAD
=======
>>>>>>> 1b29848 (fix & optimize 32 bit build; other minor mods)
			if (X > 0x02) { // flip left discs (using parallel prefix)
				F  = O & (X >> 1);
				F |= O & (F >> 1);
				O2 = O & (O >> 1);
				F |= O2 & (F >> 2);
				F |= O2 & (F >> 2);
				F &= -(P & (F >> 1));
<<<<<<< HEAD
				O ^= F;
				P ^= F;
			}
			// if (X < 0x40) { // flip right discs (using carry propagation)
				F = (O + X + X) & P;
				F -= (X + X) & -(int)(F != 0);
				O ^= F;
				P ^= F;
=======
			// if (X > 0x02) { // flip left discs (using parallel prefix)
			F  = O & (X >> 1);
			F |= O & (F >> 1);
			Y  = O & (O >> 1);
			F |= Y & (F >> 2);
			F |= Y & (F >> 2);
			F &= -(P & (F >> 1));
			O ^= F;
			P ^= F;
			// }
			// if (X < 0x40) { // flip right discs (using carry propagation)
			F = (O + X + X) & P;
			if (F) {
				F -= X + X;
				O ^= F;
				P ^= F;
			}
>>>>>>> feb7fa7 (count_last_flip_bmi2 and transpose_avx2 added)
=======
				O ^= F;
				P ^= F;
			}
			// if (X < 0x40) { // flip right discs (using carry propagation)
				F = (O + X + X) & P;
				F -= (X + X) & -(int)(F != 0);
				O ^= F;
				P ^= F;
>>>>>>> 1b29848 (fix & optimize 32 bit build; other minor mods)
			// }
			stable = find_edge_stable(P, O, stable); // next move
			if (!stable) return stable;

			P = old_P;
			O = old_O | X; // opponent plays on it
<<<<<<< HEAD
<<<<<<< HEAD
=======
>>>>>>> 1b29848 (fix & optimize 32 bit build; other minor mods)
			if (X > 0x02) { // flip left discs (using parallel prefix)
				F  = P & (X >> 1);
				F |= P & (F >> 1);
				O2 = P & (P >> 1);
				F |= O2 & (F >> 2);
				F |= O2 & (F >> 2);
				F &= -(O & (F >> 1));
<<<<<<< HEAD
				O ^= F;
				P ^= F;
			}
			// if (X < 0x40) { // flip right discs (using carry propagation)
	 			F = (P + X + X) & O;
				F -= (X + X) & -(int)(F != 0);
				O ^= F;
				P ^= F;
=======
			// if (X > 0x02) { // flip left discs (using parallel prefix)
			F  = P & (X >> 1);
			F |= P & (F >> 1);
			Y  = P & (P >> 1);
			F |= Y & (F >> 2);
			F |= Y & (F >> 2);
			F &= -(O & (F >> 1));
			O ^= F;
			P ^= F;
			// }
			// if (X < 0x40) { // flip right discs (using carry propagation)
 			F = (P + X + X) & O;
			if (F) {
				F -= X + X;
				O ^= F;
				P ^= F;
			}
>>>>>>> feb7fa7 (count_last_flip_bmi2 and transpose_avx2 added)
=======
				O ^= F;
				P ^= F;
			}
			// if (X < 0x40) { // flip right discs (using carry propagation)
	 			F = (P + X + X) & O;
				F -= (X + X) & -(int)(F != 0);
				O ^= F;
				P ^= F;
>>>>>>> 1b29848 (fix & optimize 32 bit build; other minor mods)
			// }
			stable = find_edge_stable(P, O, stable); // next move
			if (!stable) return stable;
		}
	}

	return stable;
}

/**
<<<<<<< HEAD
<<<<<<< HEAD
 * @brief Initialize the edge stability table.
=======
 * @brief Initialize the edge stability and A1_A8 tables.
>>>>>>> 343493d (More neon/sse optimizations; neon dispatch added for arm32)
=======
 * @brief Initialize the edge stability table.
>>>>>>> 9e2bbc5 (split get_all_full_lines from get_stability)
 */
void edge_stability_init(void)
{
	int P, O, PO, rPO;
	// long long t = cpu_clock();

	for (PO = 0; PO < 256 * 256; ++PO) {
		P = PO >> 8;
		O = PO & 0xFF;
		if (P & O) { // illegal positions
			edge_stability[PO] = 0;
		} else {
			rPO = horizontal_mirror_32(PO);
			if (PO > rPO)
<<<<<<< HEAD
<<<<<<< HEAD
				edge_stability[PO] = mirror_byte(edge_stability[rPO]);
=======
				edge_stability[PO] = horizontal_mirror_32(edge_stability[rPO]);
>>>>>>> feb7fa7 (count_last_flip_bmi2 and transpose_avx2 added)
=======
				edge_stability[PO] = mirror_byte(edge_stability[rPO]);
>>>>>>> 0ee9c1c (mirror_byte added for 1 byte bit reverse)
			else
				edge_stability[PO] = find_edge_stable(P, O, P);
		}
	}
	// printf("edge_stability_init: %d\n", (int)(cpu_clock() - t));
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
=======

#if (defined(USE_GAS_MMX) || defined(USE_MSVC_X86)) && !defined(hasSSE2)
	init_mmx();
#endif
>>>>>>> feb7fa7 (count_last_flip_bmi2 and transpose_avx2 added)
=======
>>>>>>> cb149ab (Faster flip_avx (ppfill) and variants added)
=======

	/* Q = 0;
	for (P = 0; P < 256; ++P) {
		A1_A8[P] = Q;
		Q = ((Q | ~0x0101010101010101) + 1) & 0x0101010101010101;
<<<<<<< HEAD
	}
>>>>>>> 343493d (More neon/sse optimizations; neon dispatch added for arm32)
=======
	} */
>>>>>>> 93110ce (Use computation or optional pdep to unpack A1_A8)
=======
>>>>>>> 9e2bbc5 (split get_all_full_lines from get_stability)
}

#ifdef HAS_CPU_64
#define	packA1A8(X)	((((X) & 0x0101010101010101) * 0x0102040810204080) >> 56)
#define	packH1H8(X)	((((X) & 0x8080808080808080) * 0x0002040810204081) >> 56)
#else
#define	packA1A8(X)	(((((unsigned int)(X) & 0x01010101) + (((unsigned int)((X) >> 32) & 0x01010101) << 4)) * 0x01020408) >> 24)
#define	packH1H8(X)	(((((unsigned int)((X) >> 32) & 0x80808080) + (((unsigned int)(X) & 0x80808080) >> 4)) * 0x00204081) >> 24)
#endif

<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
#if !defined(hasSSE2) && !defined(__ARM_NEON)
=======
#if !defined(__x86_64__) && !defined(_M_X64)
=======
#ifndef HAS_CPU_64
=======
#ifndef __AVX2__
<<<<<<< HEAD
#if !(defined(__aarch64__) || defined(_M_ARM64) || defined(hasSSE2))
>>>>>>> 9e2bbc5 (split get_all_full_lines from get_stability)
=======
#if !defined(hasNeon) && !defined(hasSSE2)
>>>>>>> 81dec96 (Kindergarten last flip for arm32; MSVC arm Windows build (not tested))
=======
>>>>>>> 21f8809 (Share all full lines between get_stability and Dogaishi hash reduction)
=======
#if !defined(__AVX2__) && !defined(hasNeon) && !defined(hasSSE2)
>>>>>>> dc7c79c (Omit unpack from get_edge_stability)
/**
 * @brief Get stable edge.
 *
 * Compute the exact stable edges from precomputed tables.
 *
 * @param P bitboard with player's discs.
 * @param O bitboard with opponent's discs.
 * @return a bitboard with (some of) player's stable discs.
 *
 */
unsigned long long get_stable_edge(const unsigned long long P, const unsigned long long O)
{	// compute the exact stable edges (from precomputed tables)
	return edge_stability[((unsigned int) P & 0xff) * 256 + ((unsigned int) O & 0xff)]
	    |  (unsigned long long) edge_stability[(unsigned int) (P >> 56) * 256 + (unsigned int) (O >> 56)] << 56
	    |  unpackA2A7(edge_stability[packA1A8(P) * 256 + packA1A8(O)])
	    |  unpackH2H7(edge_stability[packH1H8(P) * 256 + packH1H8(O)]);
}

/**
 * @brief Estimate the stability of edges.
 *
 * Count the number (in fact a lower estimate) of stable discs on the edges.
 *
 * @param P bitboard with player's discs.
 * @param O bitboard with opponent's discs.
 * @return the number of stable discs on the edges.
 */
int get_edge_stability(const unsigned long long P, const unsigned long long O)
{
	unsigned int packedstable = edge_stability[((unsigned int) P & 0xff) * 256 + ((unsigned int) O & 0xff)]
	  | edge_stability[(unsigned int) (P >> 56) * 256 + (unsigned int) (O >> 56)] << 8
	  | edge_stability[packA1A8(P) * 256 + packA1A8(O)] << 16
	  | edge_stability[packH1H8(P) * 256 + packH1H8(O)] << 24;
	return bit_count_32(packedstable & 0xffff7e7e);
}
<<<<<<< HEAD
<<<<<<< HEAD
#endif
<<<<<<< HEAD
=======
#endif
>>>>>>> 21f8809 (Share all full lines between get_stability and Dogaishi hash reduction)

#if !defined(HAS_CPU_64) && !(defined(ANDROID) && (defined(hasNeon) || defined(hasSSE2)))
>>>>>>> 343493d (More neon/sse optimizations; neon dispatch added for arm32)
=======
#if !defined(hasNeon) && !defined(hasSSE2)
>>>>>>> 9e2bbc5 (split get_all_full_lines from get_stability)
=======

>>>>>>> 81dec96 (Kindergarten last flip for arm32; MSVC arm Windows build (not tested))
/**
 * @brief Get full lines.
 *
 * @param disc all discs on the board.
 * @param full all 1 if full line, otherwise all 0.
 */

#if !defined(hasNeon) && !defined(hasSSE2) && !defined(hasMMX)
  #ifdef HAS_CPU_64

static unsigned long long get_full_lines_h(unsigned long long full)
{
	full &= full >> 1;
	full &= full >> 2;
	full &= full >> 4;
	return (full & 0x0101010101010101) * 0xff;
}

static unsigned long long get_full_lines_v(unsigned long long full)
{
	full &= (full >> 8) | (full << 56);	// ror 8
	full &= (full >> 16) | (full << 48);	// ror 16
	full &= (full >> 32) | (full << 32);	// ror 32
	return full;
}

  #else

static unsigned int get_full_lines_h_32(unsigned int full)
{
	full &= full >> 1;
	full &= full >> 2;
	full &= full >> 4;
	return (full & 0x01010101) * 0xff;
}

static unsigned long long get_full_lines_h(unsigned long long full)
{
	return ((unsigned long long) get_full_lines_h_32(full >> 32) << 32) | get_full_lines_h_32(full);
}

static unsigned long long get_full_lines_v(unsigned long long full)
{
	unsigned int	t = (unsigned int) full & (unsigned int)(full >> 32);
	t &= (t >> 16) | (t << 16);	// ror 16
	t &= (t >> 8) | (t << 24);	// ror 8
	full = t | ((unsigned long long) t << 32);
	return full;
}

<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
>>>>>>> 1dc032e (Improve visual c compatibility)
=======
unsigned long long get_all_full_lines(const unsigned long long disc, V4DI *full)
=======
=======
  #endif

<<<<<<< HEAD
>>>>>>> 264e827 (calc solid stone only when stability cutoff tried)
void get_all_full_lines(const unsigned long long disc, unsigned long long full[5])
>>>>>>> 4303b09 (Returns all full lines in full[4])
=======
static void get_full_lines(const unsigned long long disc, unsigned long long full[4])
>>>>>>> 2969de2 (Refactor get_full_lines; fix get_stability MMX)
{
	unsigned long long l7, l9, r7, r9;	// full lines

	full[0] = get_full_lines_h(disc);
	full[1] = get_full_lines_v(disc);

	l7 = r7 = disc;
	l7 &= 0xff01010101010101 | (l7 >> 7);	r7 &= 0x80808080808080ff | (r7 << 7);
	l7 &= 0xffff030303030303 | (l7 >> 14);	r7 &= 0xc0c0c0c0c0c0ffff | (r7 << 14);
	l7 &= 0xffffffff0f0f0f0f | (l7 >> 28);	r7 &= 0xf0f0f0f0ffffffff | (r7 << 28);
	l7 &= r7;
	full[3] = l7;

	l9 = r9 = disc;
	l9 &= 0xff80808080808080 | (l9 >> 9);	r9 &= 0x01010101010101ff | (r9 << 9);
	l9 &= 0xffffc0c0c0c0c0c0 | (l9 >> 18);	r9 &= 0x030303030303ffff | (r9 << 18);
	l9 = l9 & r9 & (0x0f0f0f0ff0f0f0f0 | (l9 >> 36) | (r9 << 36));
	full[2] = l9;
}
#endif // hasSSE2/hasNeon/hasMMX

>>>>>>> 9e2bbc5 (split get_all_full_lines from get_stability)
/**
<<<<<<< HEAD
 * @brief Get stable edge.
 *
 * Compute the exact stable edges from precomputed tables.
 *
 * @param P bitboard with player's discs.
 * @param O bitboard with opponent's discs.
 * @return a bitboard with (some of) player's stable discs.
 *
 */
<<<<<<< HEAD
unsigned long long get_stable_edge(const unsigned long long P, const unsigned long long O)
=======
static unsigned long long get_stable_edge(const unsigned long long P, const unsigned long long O)
>>>>>>> 1b29848 (fix & optimize 32 bit build; other minor mods)
{	// compute the exact stable edges (from precomputed tables)
	return edge_stability[((unsigned int) P & 0xff) * 256 + ((unsigned int) O & 0xff)]
	    |  (unsigned long long) edge_stability[(unsigned int) (P >> 56) * 256 + (unsigned int) (O >> 56)] << 56
	    |  unpackA2A7(edge_stability[packA1A8(P) * 256 + packA1A8(O)])
	    |  unpackH2H7(edge_stability[packH1H8(P) * 256 + packH1H8(O)]);
}

/**
<<<<<<< HEAD
=======
=======
>>>>>>> 343493d (More neon/sse optimizations; neon dispatch added for arm32)
 * @brief Estimate the stability.
 *
 * Count the number (in fact a lower estimate) of stable discs.
 *
 * @param P bitboard with player's discs.
 * @param O bitboard with opponent's discs.
 * @return the number of stable discs.
 */
#if !defined(__AVX2__) && !(defined(hasMMX) && !defined(hasSSE2))
  #if !(defined(hasSSE2) && !defined(HAS_CPU_64))
// compute the other stable discs (ie discs touching another stable disc in each flipping direction).
static int get_spreaded_stability(unsigned long long stable, unsigned long long P_central, unsigned long long full[4])
{
	unsigned long long stable_h, stable_v, stable_d7, stable_d9, old_stable;

	if (stable == 0)	// (2%)
		return 0;

	do {
		old_stable = stable;
		stable_h = ((stable >> 1) | (stable << 1) | full[0]);
		stable_v = ((stable >> 8) | (stable << 8) | full[1]);
		stable_d9 = ((stable >> 9) | (stable << 9) | full[2]);
		stable_d7 = ((stable >> 7) | (stable << 7) | full[3]);
		stable |= (stable_h & stable_v & stable_d9 & stable_d7 & P_central);
	} while (stable != old_stable);	// (44%)

	return bit_count(stable);
}
  #endif

// returns stability count only
int get_stability(const unsigned long long P, const unsigned long long O)
{
	unsigned long long stable = get_stable_edge(P, O);	// compute the exact stable edges
	unsigned long long P_central = P & 0x007e7e7e7e7e7e00;
	unsigned long long full[4];

	get_full_lines(P | O, full);	// add full lines
	stable |= (P_central & full[0] & full[1] & full[2] & full[3]);

	return get_spreaded_stability(stable, P_central, full);	// compute the other stable discs
}

// returns all full in full[4] in addition to stability count
int get_stability_fulls(const unsigned long long P, const unsigned long long O, unsigned long long full[5])
{
	unsigned long long stable = get_stable_edge(P, O);	// compute the exact stable edges
	unsigned long long P_central = P & 0x007e7e7e7e7e7e00;

	get_full_lines(P | O, full);	// add full lines
	full[4] = full[0] & full[1] & full[2] & full[3];
	stable |= (P_central & full[4]);

	return get_spreaded_stability(stable, P_central, full);	// compute the other stable discs
}
#endif

#ifndef __AVX2__
/**
 * @brief Get intersection of full lines.
 *
 * Get intersection of full lines.
 *
 * @param disc bitboard with occupied discs.
 * @return the intersection of full lines.
 */
unsigned long long get_all_full_lines(const unsigned long long disc)
{
	unsigned long long full[4];
	get_full_lines(disc, full);
	return full[0] & full[1] & full[2] & full[3];
}
#endif

/**
<<<<<<< HEAD
>>>>>>> 1a7b0ed (flip_bmi2 added; bmi2 version of stability and corner_stability)
 * @brief Estimate the stability of edges.
 *
 * Count the number (in fact a lower estimate) of stable discs on the edges.
 *
 * @param P bitboard with player's discs.
 * @param O bitboard with opponent's discs.
 * @return the number of stable discs on the edges.
 */
int get_edge_stability(const unsigned long long P, const unsigned long long O)
{
	unsigned int packedstable = edge_stability[((unsigned int) P & 0xff) * 256 + ((unsigned int) O & 0xff)]
	  | edge_stability[(unsigned int) (P >> 56) * 256 + (unsigned int) (O >> 56)] << 8
	  | edge_stability[packA1A8(P) * 256 + packA1A8(O)] << 16
	  | edge_stability[packH1H8(P) * 256 + packH1H8(O)] << 24;
	return bit_count_32(packedstable & 0xffff7e7e);
}
#endif

/**
 * @brief Get full lines.
 *
 * @param disc all discs on the board.
 * @param full all 1 if full line, otherwise all 0.
 */

#if !defined(__ARM_NEON) && !defined(hasSSE2) && !defined(hasMMX)
  #ifdef HAS_CPU_64

static unsigned long long get_full_lines_h(unsigned long long full)
{
	full &= full >> 1;
	full &= full >> 2;
	full &= full >> 4;
	return (full & 0x0101010101010101) * 0xff;
}

static unsigned long long get_full_lines_v(unsigned long long full)
{
	full &= (full >> 8) | (full << 56);	// ror 8
	full &= (full >> 16) | (full << 48);	// ror 16
	full &= (full >> 32) | (full << 32);	// ror 32
	return full;
}

  #else

static unsigned int get_full_lines_h_32(unsigned int full)
{
	full &= full >> 1;
	full &= full >> 2;
	full &= full >> 4;
	return (full & 0x01010101) * 0xff;
}

static unsigned long long get_full_lines_h(unsigned long long full)
{
	return ((unsigned long long) get_full_lines_h_32(full >> 32) << 32) | get_full_lines_h_32(full);
}

static unsigned long long get_full_lines_v(unsigned long long full)
{
	unsigned int	t = (unsigned int) full & (unsigned int)(full >> 32);
	t &= (t >> 16) | (t << 16);	// ror 16
	t &= (t >> 8) | (t << 24);	// ror 8
	return t | ((unsigned long long) t << 32);
}

  #endif

void get_full_lines(const unsigned long long disc, unsigned long long full[4])
{
	unsigned long long l7, l9, r7, r9;	// full lines

	full[0] = get_full_lines_h(disc);
	full[1] = get_full_lines_v(disc);

	l7 = r7 = disc;
	l7 &= 0xff01010101010101 | (l7 >> 7);	r7 &= 0x80808080808080ff | (r7 << 7);
	l7 &= 0xffff030303030303 | (l7 >> 14);	r7 &= 0xc0c0c0c0c0c0ffff | (r7 << 14);
	l7 &= 0xffffffff0f0f0f0f | (l7 >> 28);	r7 &= 0xf0f0f0f0ffffffff | (r7 << 28);
	full[3] = l7 & r7;

	l9 = r9 = disc;
	l9 &= 0xff80808080808080 | (l9 >> 9);	r9 &= 0x01010101010101ff | (r9 << 9);
	l9 &= 0xffffc0c0c0c0c0c0 | (l9 >> 18);	r9 &= 0x030303030303ffff | (r9 << 18);
	full[2] = l9 & r9 & (0x0f0f0f0ff0f0f0f0 | (l9 >> 36) | (r9 << 36));
}
#endif // __ARM_NEON/hasSSE2/hasMMX

/**
 * @brief Estimate the stability.
 *
 * Count the number (in fact a lower estimate) of stable discs.
 *
 * @param P bitboard with player's discs.
 * @param O bitboard with opponent's discs.
 * @return the number of stable discs.
 */
#ifndef __AVX2__	// AVX2 version in board_sse.c
  #if !(defined(hasMMX) && !defined(hasSSE2))	// MMX version of get_stability in board_mmx.c
    #if !(defined(hasSSE2) && !defined(HAS_CPU_64))	// 32bit SSE version in board_sse.c
// compute the other stable discs (ie discs touching another stable disc in each flipping direction).
int get_spreaded_stability(unsigned long long stable, unsigned long long P_central, unsigned long long full[4])
{
	unsigned long long stable_h, stable_v, stable_d7, stable_d9, old_stable;

	if (stable == 0)	// (2%)
		return 0;

	do {
		old_stable = stable;
		stable_h = ((stable >> 1) | (stable << 1) | full[0]);
		stable_v = ((stable >> 8) | (stable << 8) | full[1]);
		stable_d9 = ((stable >> 9) | (stable << 9) | full[2]);
		stable_d7 = ((stable >> 7) | (stable << 7) | full[3]);
		stable |= (stable_h & stable_v & stable_d9 & stable_d7 & P_central);
	} while (stable != old_stable);	// (44%)

	return bit_count(stable);
}
    #endif

// returns stability count only
int get_stability(const unsigned long long P, const unsigned long long O)
{
	unsigned long long stable = get_stable_edge(P, O);	// compute the exact stable edges
	unsigned long long P_central = P & 0x007e7e7e7e7e7e00;
	unsigned long long full[4];

	get_full_lines(P | O, full);	// add full lines
	stable |= (P_central & full[0] & full[1] & full[2] & full[3]);

	return get_spreaded_stability(stable, P_central, full);	// compute the other stable discs
}

// returns all full in full[4] in addition to stability count
int get_stability_fulls(const unsigned long long P, const unsigned long long O, unsigned long long full[5])
{
	unsigned long long stable = get_stable_edge(P, O);	// compute the exact stable edges
	unsigned long long P_central = P & 0x007e7e7e7e7e7e00;

	get_full_lines(P | O, full);	// add full lines
	full[4] = full[0] & full[1] & full[2] & full[3];
	stable |= (P_central & full[4]);

	return get_spreaded_stability(stable, P_central, full);	// compute the other stable discs
}
  #endif

/**
 * @brief Get intersection of full lines.
 *
 * Get intersection of full lines.
 *
 * @param disc bitboard with occupied discs.
 * @return the intersection of full lines.
 */
unsigned long long get_all_full_lines(const unsigned long long disc)
{
	unsigned long long full[4];
	get_full_lines(disc, full);
	return full[0] & full[1] & full[2] & full[3];
}
#endif // __AVX2__

/**
=======
>>>>>>> dc7c79c (Omit unpack from get_edge_stability)
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
int get_corner_stability(const unsigned long long P)
{
<<<<<<< HEAD
<<<<<<< HEAD
#ifdef POPCOUNT
	// stable = (((0x0100000000000001 & P) << 1) | ((0x8000000000000080 & P) >> 1) | ((0x0000000000000081 & P) << 8) | ((0x8100000000000000 & P) >> 8) | 0x8100000000000081) & P;
  	unsigned int P2187 = (P >> 48) | (P << 16);	// ror 48
	unsigned int stable = 0x00818100 & P2187;
	stable |= ((((stable * 5) >> 1) & 0x00424200) | (stable << 8) | (stable >> 8)) & P2187;	// 1-8 alias does not matter since corner is stable anyway
	return bit_count_32(stable);
=======
#if 0

	const unsigned long long stable = ((((0x0100000000000001 & P) << 1) | ((0x8000000000000080 & P) >> 1) | ((0x0000000000000081 & P) << 8) | ((0x8100000000000000 & P) >> 8) | 0x8100000000000081) & P);
	return bit_count(stable);
>>>>>>> 6506166 (More SSE optimizations)
=======
#ifdef POPCOUNT
	// stable = (((0x0100000000000001 & P) << 1) | ((0x8000000000000080 & P) >> 1) | ((0x0000000000000081 & P) << 8) | ((0x8100000000000000 & P) >> 8) | 0x8100000000000081) & P;
  	unsigned int P2187 = (P >> 48) | (P << 16);	// ror 48
	unsigned int stable = 0x00818100 & P2187;
	stable |= ((((stable * 5) >> 1) & 0x00424200) | (stable << 8) | (stable >> 8)) & P2187;	// 1-8 alias does not matter since corner is stable anyway
	return bit_count_32(stable);
<<<<<<< HEAD
  #endif
>>>>>>> 11a54a6 (Revise get_corner_stability and hash_cleanup)
=======
>>>>>>> 9078deb (new get_corner_stability for both 64&32 bit)

#else	// kindergarten
	static const char n_stable_h2a2h1g1b1a1[64] = {
		0, 1, 0, 2, 0, 1, 0, 2, 1, 2, 1, 3, 2, 3, 2, 4,
		0, 2, 0, 3, 0, 2, 0, 3, 1, 3, 1, 4, 2, 4, 2, 5,
		0, 1, 0, 2, 0, 1, 0, 2, 2, 3, 2, 4, 3, 4, 3, 5,
		0, 2, 0, 3, 0, 2, 0, 3, 2, 4, 2, 5, 3, 5, 3, 6
	};

<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
  #if 0 // defined(__BMI2__) && !defined(__bdver4__) && !defined(__znver1__) && !defined(__znver2__)	// BMI2 CPU has POPCOUNT
=======
  #if 0 // defined(__BMI2__) && !defined(__bdver4__) && !defined(__znver1__) && !defined(__znver2__)	// kindergarten for generic modern build
>>>>>>> 867c81c (Omit restore board/parity in search_shallow; tweak NWS_STABILITY)
=======
  #if 0 // defined(__BMI2__) && !defined(__bdver4__) && !defined(__znver1__) && !defined(__znver2__)	// BMI2 CPU has POPCOUNT
>>>>>>> 9078deb (new get_corner_stability for both 64&32 bit)
	int cnt = n_stable_h2a2h1g1b1a1[_pext_u32((unsigned int) vertical_mirror(P), 0x000081c3)]
		+ n_stable_h2a2h1g1b1a1[_pext_u32((unsigned int) P, 0x000081c3)];

  #else
	static const char n_stable_h8g8b8a8h7a7[64] = {
		0, 0, 0, 0, 1, 2, 1, 2, 0, 0, 0, 0, 2, 3, 2, 3,
		0, 0, 0, 0, 1, 2, 1, 2, 0, 0, 0, 0, 2, 3, 2, 3,
		1, 1, 2, 2, 2, 3, 3, 4, 1, 1, 2, 2, 3, 4, 4, 5,
		2, 2, 3, 3, 3, 4, 4, 5, 2, 2, 3, 3, 4, 5, 5, 6
	};

	int cnt = n_stable_h8g8b8a8h7a7[(((unsigned int) (P >> 32) & 0xc3810000) * 0x00000411) >> 26]
		+ n_stable_h2a2h1g1b1a1[(((unsigned int) P & 0x000081c3) * 0x04410000) >> 26];
  #endif
	// assert(cnt == bit_count((((0x0100000000000001 & P) << 1) | ((0x8000000000000080 & P) >> 1) | ((0x0000000000000081 & P) << 8) | ((0x8100000000000000 & P) >> 8) | 0x8100000000000081) & P));
=======
#if defined(__BMI2__) && defined(__x86_64__)
=======
#if 0 // defined(__BMI2__) && defined(__x86_64__) // pext is slow on AMD
<<<<<<< HEAD
>>>>>>> f24cc06 (avoid BMI2 for AMD; more lzcnt/tzcnt in count_last_flip_bitscan)
	int cnt = n_stable_h8g8b8a8h7a7[_pext_u64(P, 0xc381000000000000ULL)]
		+ n_stable_h2a2h1g1b1a1[_pext_u32((unsigned int) P, 0x000081c3U)];
=======
=======
#ifdef USEPEXT // defined(__BMI2__) && defined(__x86_64__) && !defined(AMD_BEFORE_ZEN3)	// kindergarten for generic modern build
>>>>>>> 6f4eb2e (VPGATHERDD accumlate_eval)
=======
#ifdef USEPEXT // defined(__BMI2__) && defined(__x86_64__) && !defined(AMD_BEFORE_ZEN3)	// kindergarten for generic modern build
>>>>>>> bbc1ddf (VPGATHERDD accumlate_eval)
	int cnt = n_stable_h8g8b8a8h7a7[_pext_u64(P, 0xc381000000000000)]
		+ n_stable_h2a2h1g1b1a1[_pext_u32((unsigned int) P, 0x000081c3)];
>>>>>>> 6506166 (More SSE optimizations)
#else
	int cnt = n_stable_h8g8b8a8h7a7[(((unsigned int) (P >> 32) & 0xc3810000) * 0x00000411) >> 26]
		+ n_stable_h2a2h1g1b1a1[(((unsigned int) P & 0x000081c3) * 0x04410000) >> 26];
#endif
<<<<<<< HEAD
	// assert(cnt == bit_count((((0x0100000000000001ULL & P) << 1) | ((0x8000000000000080ULL & P) >> 1) | ((0x0000000000000081ULL & P) << 8) | ((0x8100000000000000ULL & P) >> 8) | 0x8100000000000081ULL) & P));
>>>>>>> 1a7b0ed (flip_bmi2 added; bmi2 version of stability and corner_stability)
=======
=======
  #if 0 // defined(__BMI2__) && !defined(AMD_BEFORE_ZEN3)	// kindergarten for generic modern build
	int cnt = n_stable_h2a2h1g1b1a1[_pext_u32((unsigned int) vertical_mirror(P), 0x000081c3)]
		+ n_stable_h2a2h1g1b1a1[_pext_u32((unsigned int) P, 0x000081c3)];

  #else
	static const char n_stable_h8g8b8a8h7a7[64] = {
		0, 0, 0, 0, 1, 2, 1, 2, 0, 0, 0, 0, 2, 3, 2, 3,
		0, 0, 0, 0, 1, 2, 1, 2, 0, 0, 0, 0, 2, 3, 2, 3,
		1, 1, 2, 2, 2, 3, 3, 4, 1, 1, 2, 2, 3, 4, 4, 5,
		2, 2, 3, 3, 3, 4, 4, 5, 2, 2, 3, 3, 4, 5, 5, 6
	};

	int cnt = n_stable_h8g8b8a8h7a7[(((unsigned int) (P >> 32) & 0xc3810000) * 0x00000411) >> 26]
		+ n_stable_h2a2h1g1b1a1[(((unsigned int) P & 0x000081c3) * 0x04410000) >> 26];
  #endif
>>>>>>> 11a54a6 (Revise get_corner_stability and hash_cleanup)
	// assert(cnt == bit_count((((0x0100000000000001 & P) << 1) | ((0x8000000000000080 & P) >> 1) | ((0x0000000000000081 & P) << 8) | ((0x8100000000000000 & P) >> 8) | 0x8100000000000081) & P));
>>>>>>> 6506166 (More SSE optimizations)
	return cnt;

#endif
}

/**
 * @brief Compute a hash code.
 *
 * @param board the board.
 * @return the hash code of the bitboard
 */
unsigned long long board_get_hash_code(const Board *board)
{
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
	unsigned long long crc = crc32c_u64(0, board->player);
	return (crc << 32) | crc32c_u64(crc, board->opponent);
=======
	const unsigned char *p = (const unsigned char*)board;
=======
	const unsigned char *const p = (const unsigned char*)board;
>>>>>>> 0a166fd (Remove 1 element array coding style)
	unsigned long long h1, h2;

#if defined(USE_GAS_MMX) && defined(__3dNOW__)	// Faster on AMD but not suitable for CPU with slow emms
	if (hasMMX)
		return board_get_hash_code_mmx(p);
#elif defined(USE_GAS_MMX) || defined(USE_MSVC_X86) // || defined(__x86_64__)
	if (hasSSE2)
		return board_get_hash_code_sse(p);
#endif

	h1  = hash_rank[0][p[0]];	h2  = hash_rank[1][p[1]];
	h1 ^= hash_rank[2][p[2]];	h2 ^= hash_rank[3][p[3]];
	h1 ^= hash_rank[4][p[4]];	h2 ^= hash_rank[5][p[5]];
	h1 ^= hash_rank[6][p[6]];	h2 ^= hash_rank[7][p[7]];
	h1 ^= hash_rank[8][p[8]];	h2 ^= hash_rank[9][p[9]];
	h1 ^= hash_rank[10][p[10]];	h2 ^= hash_rank[11][p[11]];
	h1 ^= hash_rank[12][p[12]];	h2 ^= hash_rank[13][p[13]];
	h1 ^= hash_rank[14][p[14]];	h2 ^= hash_rank[15][p[15]];

	// assert((h1 ^ h2) == board_get_hash_code_sse(p));

	return h1 ^ h2;
>>>>>>> 1b29848 (fix & optimize 32 bit build; other minor mods)
=======
	unsigned long long	crc;

	crc = crc32c_u64(0, board->player);
	return (crc << 32) | crc32c_u64(crc, board->opponent);
>>>>>>> 34a2291 (4.5.0: Use CRC32c for board hash)
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
	unsigned long long b = x_to_bit(x);
	return (int) ((board->player & b) == 0) * 2 - (int) ((board->opponent & b) != 0);
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
	return ((board->player | board->opponent) & x_to_bit(x)) != 0;	// omitting != 0 causes bogus code on MSVC19 /GL
}

/**
 * @brief Check if current player should pass.
 *
 * @param board board.
 * @return true if player is passing, false otherwise.
 */
bool board_is_pass(const Board *board)
{
	return !can_move(board->player, board->opponent) &&
		can_move(board->opponent, board->player);
}

/**
 * @brief Check if the game is over.
 *
 * @param board board.
 * @return true if game is over, false otherwise.
 */
bool board_is_game_over(const Board *board)
{
	return !can_move(board->player, board->opponent) &&
		!can_move(board->opponent, board->player);
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
	int i, j, square;
	unsigned long long bk, wh;
<<<<<<< HEAD
<<<<<<< HEAD
	const char color[5] = "?*O-.";
	unsigned long long moves = board_get_moves(board);
<<<<<<< HEAD

	if (player == BLACK) {
		bk = board->player;
		wh = board->opponent;
	} else {
		bk = board->opponent;
		wh = board->player;
	}
=======
	const char *color = "?*O-." + 1;
=======
	const char color[5] = "?*O-.";
>>>>>>> bc93772 (Avoid modern compliler warnings)
	unsigned long long moves = get_moves(board->player, board->opponent);
>>>>>>> cd90dbb (Enable 32bit AVX build; optimize loop in board print; set version to 4.4.6)
=======
>>>>>>> 80ca4b1 (board_get_moves for AVX2; rename board_get_move_flip)

	if (player == BLACK) {
		bk = board->player;
		wh = board->opponent;
	} else {
		bk = board->opponent;
		wh = board->player;
	}

	fputs("  A B C D E F G H\n", f);
	for (i = 0; i < 8; ++i) {
		fputc(i + '1', f);
		fputc(' ', f);
		for (j = 0; j < 8; ++j) {
			square = 2 - (wh & 1) - 2 * (bk & 1);
			if ((square == EMPTY) && (moves & 1))
				square = EMPTY + 1;
<<<<<<< HEAD
<<<<<<< HEAD
			fputc(color[square + 1], f);
=======
			fputc(color[square], f);
>>>>>>> cd90dbb (Enable 32bit AVX build; optimize loop in board print; set version to 4.4.6)
=======
			fputc(color[square + 1], f);
>>>>>>> bc93772 (Avoid modern compliler warnings)
			fputc(' ', f);
			bk >>= 1;
			wh >>= 1;
			moves >>= 1;
		}
		fputc(i + '1', f);
		if (i == 1)
			fprintf(f, " %c to move", color[player + 1]);
		else if (i == 3)
			fprintf(f, " %c: discs = %2d    moves = %2d",
				color[player + 1], bit_count(board->player), get_mobility(board->player, board->opponent));
		else if (i == 4)
			fprintf(f, " %c: discs = %2d    moves = %2d",
				color[2 - player], bit_count(board->opponent), get_mobility(board->opponent, board->player));
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
	unsigned long long bk, wh;
	static const char color[4] = "XO-?";

	if (player == BLACK) {
		bk = board->player;
		wh = board->opponent;
	} else {
		bk = board->opponent;
		wh = board->player;
	}

	for (x = 0; x < 64; ++x) {
		square = 2 - (wh & 1) - 2 * (bk & 1);
		s[x] = color[square];
		bk >>= 1;
		wh >>= 1;
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
	unsigned long long bk, wh;
	static const char piece[4] = "pP-?";
	static const char color[2] = "bw";
	int n_empties = 0;
	char *s = string;
	static char local_string[128];

	if (s == NULL) s = string = local_string;

	if (player == BLACK) {
		bk = board->player;
		wh = board->opponent;
	} else {
		bk = board->opponent;
		wh = board->player;
	}

	for (r = 7; r >= 0; --r) {
		for (c = 0; c < 8; ++c) {
			x = 8 * r + c;
			square = 2 - ((wh >> x) & 1) - 2 * ((bk >> x) & 1);

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
		if (r > 0)
			*s++ = '/';
	}
	*s++ = ' ';
	*s++ = color[player];
	strcpy(s, " - - 0 1");

	return string;
}

