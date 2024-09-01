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
  #ifdef __ARM_NEON
	#define	flip_neon	flip
	#include "flip_neon_bitscan.c"
  #else
	#include "flip_bitscan.c"
  #endif
#elif MOVE_GENERATOR == MOVE_GENERATOR_ROXANE
	#include "flip_roxane.c"
#elif MOVE_GENERATOR == MOVE_GENERATOR_32
	#include "flip_carry_sse_32.c"
#elif MOVE_GENERATOR == MOVE_GENERATOR_SSE_BSWAP
	#include "flip_sse_bswap.c"
#elif MOVE_GENERATOR == MOVE_GENERATOR_AVX
	#include "flip_avx_ppfill.c"
#elif MOVE_GENERATOR == MOVE_GENERATOR_AVX512
	#include "flip_avx512cd.c"
#elif MOVE_GENERATOR == MOVE_GENERATOR_NEON
  #ifdef __aarch64__
	#include "flip_neon_rbit.c"
  #else
	#include "flip_neon_lzcnt.c"
  #endif
#elif MOVE_GENERATOR == MOVE_GENERATOR_SVE
	#include "flip_sve_lzcnt.c"
#else // MOVE_GENERATOR == MOVE_GENERATOR_KINDERGARTEN
	#include "flip_kindergarten.c"
#endif


/** edge stability global data */
unsigned char edge_stability[256 * 256];

#if (defined(USE_GAS_MMX) || defined(USE_MSVC_X86)) && !defined(hasSSE2)
	#include "board_mmx.c"
#endif
#if (defined(USE_GAS_MMX) || defined(USE_MSVC_X86) || defined(hasSSE2) || defined(__ARM_NEON)) && !defined(ANDROID)
	#include "board_sse.c"
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
}

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
	*sym = *board;
	if (s & 1)
		board_horizontal_mirror(sym, sym);
	if (s & 2)
		board_vertical_mirror(sym, sym);
	if (s & 4)
		board_transpose(sym, sym);

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
		board_get_move_flip(board, get_rand_bit(moves, r), &move);
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

#if (MOVE_GENERATOR != MOVE_GENERATOR_AVX) && (MOVE_GENERATOR != MOVE_GENERATOR_AVX512) && (MOVE_GENERATOR != MOVE_GENERATOR_SSE) && (MOVE_GENERATOR != MOVE_GENERATOR_NEON)	// SSE version in board_sse.c
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
#endif

#if !defined(hasSSE2) && !defined(__ARM_NEON)	// SSE version in board_sse.c
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
unsigned long long get_moves(const unsigned long long P, const unsigned long long O)
{
	unsigned long long moves, OM;

	#if defined(USE_GAS_MMX) || defined(USE_MSVC_X86) || defined(DISPATCH_NEON)
	if (hasSSE2)
		return get_moves_sse(P, O);
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
#endif // hasSSE2/__ARM_NEON

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
	unsigned long long PM = P & 0x007E7E7E7E7E7E00;
	unsigned long long OM = O & 0x007E7E7E7E7E7E00;
	return get_moves(PM, OM) & 0x007E7E7E7E7E7E00;
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
#if defined(hasMMX) || defined(__ARM_NEON)
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

#ifndef __AVX2__	// AVX2 version in board_sse.c
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
#endif // AVX2

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
			if (X > 0x02) { // flip left discs (using parallel prefix)
				F  = O & (X >> 1);
				F |= O & (F >> 1);
				O2 = O & (O >> 1);
				F |= O2 & (F >> 2);
				F |= O2 & (F >> 2);
				F &= -(P & (F >> 1));
				O ^= F;
				P ^= F;
			}
			// if (X < 0x40) { // flip right discs (using carry propagation)
				F = (O + X + X) & P;
				F -= (X + X) & -(int)(F != 0);
				O ^= F;
				P ^= F;
			// }
			stable = find_edge_stable(P, O, stable); // next move
			if (!stable) return stable;

			P = old_P;
			O = old_O | X; // opponent plays on it
			if (X > 0x02) { // flip left discs (using parallel prefix)
				F  = P & (X >> 1);
				F |= P & (F >> 1);
				O2 = P & (P >> 1);
				F |= O2 & (F >> 2);
				F |= O2 & (F >> 2);
				F &= -(O & (F >> 1));
				O ^= F;
				P ^= F;
			}
			// if (X < 0x40) { // flip right discs (using carry propagation)
	 			F = (P + X + X) & O;
				F -= (X + X) & -(int)(F != 0);
				O ^= F;
				P ^= F;
			// }
			stable = find_edge_stable(P, O, stable); // next move
			if (!stable) return stable;
		}
	}

	return stable;
}

/**
 * @brief Initialize the edge stability table.
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
				edge_stability[PO] = mirror_byte(edge_stability[rPO]);
			else
				edge_stability[PO] = find_edge_stable(P, O, P);
		}
	}
	// printf("edge_stability_init: %d\n", (int)(cpu_clock() - t));
}

#ifdef HAS_CPU_64
#define	packA1A8(X)	((((X) & 0x0101010101010101) * 0x0102040810204080) >> 56)
#define	packH1H8(X)	((((X) & 0x8080808080808080) * 0x0002040810204081) >> 56)
#else
#define	packA1A8(X)	(((((unsigned int)(X) & 0x01010101) + (((unsigned int)((X) >> 32) & 0x01010101) << 4)) * 0x01020408) >> 24)
#define	packH1H8(X)	(((((unsigned int)((X) >> 32) & 0x80808080) + (((unsigned int)(X) & 0x80808080) >> 4)) * 0x00204081) >> 24)
#endif

#if !defined(hasSSE2) && !defined(__ARM_NEON)
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
#ifdef POPCOUNT
	// stable = (((0x0100000000000001 & P) << 1) | ((0x8000000000000080 & P) >> 1) | ((0x0000000000000081 & P) << 8) | ((0x8100000000000000 & P) >> 8) | 0x8100000000000081) & P;
  	unsigned int P2187 = (P >> 48) | (P << 16);	// ror 48
	unsigned int stable = 0x00818100 & P2187;
	stable |= ((((stable * 5) >> 1) & 0x00424200) | (stable << 8) | (stable >> 8)) & P2187;	// 1-8 alias does not matter since corner is stable anyway
	return bit_count_32(stable);

#else	// kindergarten
	static const char n_stable_h2a2h1g1b1a1[64] = {
		0, 1, 0, 2, 0, 1, 0, 2, 1, 2, 1, 3, 2, 3, 2, 4,
		0, 2, 0, 3, 0, 2, 0, 3, 1, 3, 1, 4, 2, 4, 2, 5,
		0, 1, 0, 2, 0, 1, 0, 2, 2, 3, 2, 4, 3, 4, 3, 5,
		0, 2, 0, 3, 0, 2, 0, 3, 2, 4, 2, 5, 3, 5, 3, 6
	};

  #if 0 // defined(__BMI2__) && !defined(__bdver4__) && !defined(__znver1__) && !defined(__znver2__)	// BMI2 CPU has POPCOUNT
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
	unsigned long long crc = crc32c_u64(0, board->player);
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
	const char color[5] = "?*O-.";
	unsigned long long moves = board_get_moves(board);

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
			fputc(color[square + 1], f);
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

