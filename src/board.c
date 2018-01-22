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
 * @date 1998 - 2017
 * @author Richard Delorme
 * @version 4.4
 */

#include "board.h"

#include "bit.h"
#include "hash.h"
#include "move.h"
#include "settings.h"
#include "util.h"

#include <ctype.h>
#include <stdlib.h>
#include <assert.h>


#if MOVE_GENERATOR == MOVE_GENERATOR_CARRY
	#ifdef HAS_CPU_64
		#include "flip_carry_64.c"
		#include "count_last_flip_carry_64.c"
	#else 
		#include "flip_carry_32.c"
		#include "count_last_flip_carry_32.c"
	#endif
#elif MOVE_GENERATOR == MOVE_GENERATOR_SSE
	#include "flip_sse.c"
	#include "count_last_flip_kindergarten.c"
#elif MOVE_GENERATOR == MOVE_GENERATOR_BITSCAN
	#include "flip_bitscan.c"
	#include "count_last_flip_bitscan.c"
#elif MOVE_GENERATOR == MOVE_GENERATOR_ROXANE
	#include "flip_roxane.c"
	#include "count_last_flip_kindergarten.c"
#else // MOVE_GENERATOR == MOVE_GENERATOR_KINDERGARTEN
	#include "flip_kindergarten.c"
	#include "count_last_flip_kindergarten.c"
#endif


/** edge stability global data */
static unsigned char edge_stability[256][256];

/** conversion from an 8-bit line to the A1-A8 line */
static const unsigned long long A1_A8[256] = {
	0x0000000000000000ULL, 0x0000000000000001ULL, 0x0000000000000100ULL, 0x0000000000000101ULL, 0x0000000000010000ULL, 0x0000000000010001ULL, 0x0000000000010100ULL, 0x0000000000010101ULL,
	0x0000000001000000ULL, 0x0000000001000001ULL, 0x0000000001000100ULL, 0x0000000001000101ULL, 0x0000000001010000ULL, 0x0000000001010001ULL, 0x0000000001010100ULL, 0x0000000001010101ULL,
	0x0000000100000000ULL, 0x0000000100000001ULL, 0x0000000100000100ULL, 0x0000000100000101ULL, 0x0000000100010000ULL, 0x0000000100010001ULL, 0x0000000100010100ULL, 0x0000000100010101ULL,
	0x0000000101000000ULL, 0x0000000101000001ULL, 0x0000000101000100ULL, 0x0000000101000101ULL, 0x0000000101010000ULL, 0x0000000101010001ULL, 0x0000000101010100ULL, 0x0000000101010101ULL,
	0x0000010000000000ULL, 0x0000010000000001ULL, 0x0000010000000100ULL, 0x0000010000000101ULL, 0x0000010000010000ULL, 0x0000010000010001ULL, 0x0000010000010100ULL, 0x0000010000010101ULL,
	0x0000010001000000ULL, 0x0000010001000001ULL, 0x0000010001000100ULL, 0x0000010001000101ULL, 0x0000010001010000ULL, 0x0000010001010001ULL, 0x0000010001010100ULL, 0x0000010001010101ULL,
	0x0000010100000000ULL, 0x0000010100000001ULL, 0x0000010100000100ULL, 0x0000010100000101ULL, 0x0000010100010000ULL, 0x0000010100010001ULL, 0x0000010100010100ULL, 0x0000010100010101ULL,
	0x0000010101000000ULL, 0x0000010101000001ULL, 0x0000010101000100ULL, 0x0000010101000101ULL, 0x0000010101010000ULL, 0x0000010101010001ULL, 0x0000010101010100ULL, 0x0000010101010101ULL,
	0x0001000000000000ULL, 0x0001000000000001ULL, 0x0001000000000100ULL, 0x0001000000000101ULL, 0x0001000000010000ULL, 0x0001000000010001ULL, 0x0001000000010100ULL, 0x0001000000010101ULL,
	0x0001000001000000ULL, 0x0001000001000001ULL, 0x0001000001000100ULL, 0x0001000001000101ULL, 0x0001000001010000ULL, 0x0001000001010001ULL, 0x0001000001010100ULL, 0x0001000001010101ULL,
	0x0001000100000000ULL, 0x0001000100000001ULL, 0x0001000100000100ULL, 0x0001000100000101ULL, 0x0001000100010000ULL, 0x0001000100010001ULL, 0x0001000100010100ULL, 0x0001000100010101ULL,
	0x0001000101000000ULL, 0x0001000101000001ULL, 0x0001000101000100ULL, 0x0001000101000101ULL, 0x0001000101010000ULL, 0x0001000101010001ULL, 0x0001000101010100ULL, 0x0001000101010101ULL,
	0x0001010000000000ULL, 0x0001010000000001ULL, 0x0001010000000100ULL, 0x0001010000000101ULL, 0x0001010000010000ULL, 0x0001010000010001ULL, 0x0001010000010100ULL, 0x0001010000010101ULL,
	0x0001010001000000ULL, 0x0001010001000001ULL, 0x0001010001000100ULL, 0x0001010001000101ULL, 0x0001010001010000ULL, 0x0001010001010001ULL, 0x0001010001010100ULL, 0x0001010001010101ULL,
	0x0001010100000000ULL, 0x0001010100000001ULL, 0x0001010100000100ULL, 0x0001010100000101ULL, 0x0001010100010000ULL, 0x0001010100010001ULL, 0x0001010100010100ULL, 0x0001010100010101ULL,
	0x0001010101000000ULL, 0x0001010101000001ULL, 0x0001010101000100ULL, 0x0001010101000101ULL, 0x0001010101010000ULL, 0x0001010101010001ULL, 0x0001010101010100ULL, 0x0001010101010101ULL,
	0x0100000000000000ULL, 0x0100000000000001ULL, 0x0100000000000100ULL, 0x0100000000000101ULL, 0x0100000000010000ULL, 0x0100000000010001ULL, 0x0100000000010100ULL, 0x0100000000010101ULL,
	0x0100000001000000ULL, 0x0100000001000001ULL, 0x0100000001000100ULL, 0x0100000001000101ULL, 0x0100000001010000ULL, 0x0100000001010001ULL, 0x0100000001010100ULL, 0x0100000001010101ULL,
	0x0100000100000000ULL, 0x0100000100000001ULL, 0x0100000100000100ULL, 0x0100000100000101ULL, 0x0100000100010000ULL, 0x0100000100010001ULL, 0x0100000100010100ULL, 0x0100000100010101ULL,
	0x0100000101000000ULL, 0x0100000101000001ULL, 0x0100000101000100ULL, 0x0100000101000101ULL, 0x0100000101010000ULL, 0x0100000101010001ULL, 0x0100000101010100ULL, 0x0100000101010101ULL,
	0x0100010000000000ULL, 0x0100010000000001ULL, 0x0100010000000100ULL, 0x0100010000000101ULL, 0x0100010000010000ULL, 0x0100010000010001ULL, 0x0100010000010100ULL, 0x0100010000010101ULL,
	0x0100010001000000ULL, 0x0100010001000001ULL, 0x0100010001000100ULL, 0x0100010001000101ULL, 0x0100010001010000ULL, 0x0100010001010001ULL, 0x0100010001010100ULL, 0x0100010001010101ULL,
	0x0100010100000000ULL, 0x0100010100000001ULL, 0x0100010100000100ULL, 0x0100010100000101ULL, 0x0100010100010000ULL, 0x0100010100010001ULL, 0x0100010100010100ULL, 0x0100010100010101ULL,
	0x0100010101000000ULL, 0x0100010101000001ULL, 0x0100010101000100ULL, 0x0100010101000101ULL, 0x0100010101010000ULL, 0x0100010101010001ULL, 0x0100010101010100ULL, 0x0100010101010101ULL,
	0x0101000000000000ULL, 0x0101000000000001ULL, 0x0101000000000100ULL, 0x0101000000000101ULL, 0x0101000000010000ULL, 0x0101000000010001ULL, 0x0101000000010100ULL, 0x0101000000010101ULL,
	0x0101000001000000ULL, 0x0101000001000001ULL, 0x0101000001000100ULL, 0x0101000001000101ULL, 0x0101000001010000ULL, 0x0101000001010001ULL, 0x0101000001010100ULL, 0x0101000001010101ULL,
	0x0101000100000000ULL, 0x0101000100000001ULL, 0x0101000100000100ULL, 0x0101000100000101ULL, 0x0101000100010000ULL, 0x0101000100010001ULL, 0x0101000100010100ULL, 0x0101000100010101ULL,
	0x0101000101000000ULL, 0x0101000101000001ULL, 0x0101000101000100ULL, 0x0101000101000101ULL, 0x0101000101010000ULL, 0x0101000101010001ULL, 0x0101000101010100ULL, 0x0101000101010101ULL,
	0x0101010000000000ULL, 0x0101010000000001ULL, 0x0101010000000100ULL, 0x0101010000000101ULL, 0x0101010000010000ULL, 0x0101010000010001ULL, 0x0101010000010100ULL, 0x0101010000010101ULL,
	0x0101010001000000ULL, 0x0101010001000001ULL, 0x0101010001000100ULL, 0x0101010001000101ULL, 0x0101010001010000ULL, 0x0101010001010001ULL, 0x0101010001010100ULL, 0x0101010001010101ULL,
	0x0101010100000000ULL, 0x0101010100000001ULL, 0x0101010100000100ULL, 0x0101010100000101ULL, 0x0101010100010000ULL, 0x0101010100010001ULL, 0x0101010100010100ULL, 0x0101010100010101ULL,
	0x0101010101000000ULL, 0x0101010101000001ULL, 0x0101010101000100ULL, 0x0101010101000101ULL, 0x0101010101010000ULL, 0x0101010101010001ULL, 0x0101010101010100ULL, 0x0101010101010101ULL,
};

/** conversion from an 8-bit line to the H1-H8 line */
static const unsigned long long H1_H8[256] = {
	0x0000000000000000ULL, 0x0000000000000080ULL, 0x0000000000008000ULL, 0x0000000000008080ULL, 0x0000000000800000ULL, 0x0000000000800080ULL, 0x0000000000808000ULL, 0x0000000000808080ULL,
	0x0000000080000000ULL, 0x0000000080000080ULL, 0x0000000080008000ULL, 0x0000000080008080ULL, 0x0000000080800000ULL, 0x0000000080800080ULL, 0x0000000080808000ULL, 0x0000000080808080ULL,
	0x0000008000000000ULL, 0x0000008000000080ULL, 0x0000008000008000ULL, 0x0000008000008080ULL, 0x0000008000800000ULL, 0x0000008000800080ULL, 0x0000008000808000ULL, 0x0000008000808080ULL,
	0x0000008080000000ULL, 0x0000008080000080ULL, 0x0000008080008000ULL, 0x0000008080008080ULL, 0x0000008080800000ULL, 0x0000008080800080ULL, 0x0000008080808000ULL, 0x0000008080808080ULL,
	0x0000800000000000ULL, 0x0000800000000080ULL, 0x0000800000008000ULL, 0x0000800000008080ULL, 0x0000800000800000ULL, 0x0000800000800080ULL, 0x0000800000808000ULL, 0x0000800000808080ULL,
	0x0000800080000000ULL, 0x0000800080000080ULL, 0x0000800080008000ULL, 0x0000800080008080ULL, 0x0000800080800000ULL, 0x0000800080800080ULL, 0x0000800080808000ULL, 0x0000800080808080ULL,
	0x0000808000000000ULL, 0x0000808000000080ULL, 0x0000808000008000ULL, 0x0000808000008080ULL, 0x0000808000800000ULL, 0x0000808000800080ULL, 0x0000808000808000ULL, 0x0000808000808080ULL,
	0x0000808080000000ULL, 0x0000808080000080ULL, 0x0000808080008000ULL, 0x0000808080008080ULL, 0x0000808080800000ULL, 0x0000808080800080ULL, 0x0000808080808000ULL, 0x0000808080808080ULL,
	0x0080000000000000ULL, 0x0080000000000080ULL, 0x0080000000008000ULL, 0x0080000000008080ULL, 0x0080000000800000ULL, 0x0080000000800080ULL, 0x0080000000808000ULL, 0x0080000000808080ULL,
	0x0080000080000000ULL, 0x0080000080000080ULL, 0x0080000080008000ULL, 0x0080000080008080ULL, 0x0080000080800000ULL, 0x0080000080800080ULL, 0x0080000080808000ULL, 0x0080000080808080ULL,
	0x0080008000000000ULL, 0x0080008000000080ULL, 0x0080008000008000ULL, 0x0080008000008080ULL, 0x0080008000800000ULL, 0x0080008000800080ULL, 0x0080008000808000ULL, 0x0080008000808080ULL,
	0x0080008080000000ULL, 0x0080008080000080ULL, 0x0080008080008000ULL, 0x0080008080008080ULL, 0x0080008080800000ULL, 0x0080008080800080ULL, 0x0080008080808000ULL, 0x0080008080808080ULL,
	0x0080800000000000ULL, 0x0080800000000080ULL, 0x0080800000008000ULL, 0x0080800000008080ULL, 0x0080800000800000ULL, 0x0080800000800080ULL, 0x0080800000808000ULL, 0x0080800000808080ULL,
	0x0080800080000000ULL, 0x0080800080000080ULL, 0x0080800080008000ULL, 0x0080800080008080ULL, 0x0080800080800000ULL, 0x0080800080800080ULL, 0x0080800080808000ULL, 0x0080800080808080ULL,
	0x0080808000000000ULL, 0x0080808000000080ULL, 0x0080808000008000ULL, 0x0080808000008080ULL, 0x0080808000800000ULL, 0x0080808000800080ULL, 0x0080808000808000ULL, 0x0080808000808080ULL,
	0x0080808080000000ULL, 0x0080808080000080ULL, 0x0080808080008000ULL, 0x0080808080008080ULL, 0x0080808080800000ULL, 0x0080808080800080ULL, 0x0080808080808000ULL, 0x0080808080808080ULL,
	0x8000000000000000ULL, 0x8000000000000080ULL, 0x8000000000008000ULL, 0x8000000000008080ULL, 0x8000000000800000ULL, 0x8000000000800080ULL, 0x8000000000808000ULL, 0x8000000000808080ULL,
	0x8000000080000000ULL, 0x8000000080000080ULL, 0x8000000080008000ULL, 0x8000000080008080ULL, 0x8000000080800000ULL, 0x8000000080800080ULL, 0x8000000080808000ULL, 0x8000000080808080ULL,
	0x8000008000000000ULL, 0x8000008000000080ULL, 0x8000008000008000ULL, 0x8000008000008080ULL, 0x8000008000800000ULL, 0x8000008000800080ULL, 0x8000008000808000ULL, 0x8000008000808080ULL,
	0x8000008080000000ULL, 0x8000008080000080ULL, 0x8000008080008000ULL, 0x8000008080008080ULL, 0x8000008080800000ULL, 0x8000008080800080ULL, 0x8000008080808000ULL, 0x8000008080808080ULL,
	0x8000800000000000ULL, 0x8000800000000080ULL, 0x8000800000008000ULL, 0x8000800000008080ULL, 0x8000800000800000ULL, 0x8000800000800080ULL, 0x8000800000808000ULL, 0x8000800000808080ULL,
	0x8000800080000000ULL, 0x8000800080000080ULL, 0x8000800080008000ULL, 0x8000800080008080ULL, 0x8000800080800000ULL, 0x8000800080800080ULL, 0x8000800080808000ULL, 0x8000800080808080ULL,
	0x8000808000000000ULL, 0x8000808000000080ULL, 0x8000808000008000ULL, 0x8000808000008080ULL, 0x8000808000800000ULL, 0x8000808000800080ULL, 0x8000808000808000ULL, 0x8000808000808080ULL,
	0x8000808080000000ULL, 0x8000808080000080ULL, 0x8000808080008000ULL, 0x8000808080008080ULL, 0x8000808080800000ULL, 0x8000808080800080ULL, 0x8000808080808000ULL, 0x8000808080808080ULL,
	0x8080000000000000ULL, 0x8080000000000080ULL, 0x8080000000008000ULL, 0x8080000000008080ULL, 0x8080000000800000ULL, 0x8080000000800080ULL, 0x8080000000808000ULL, 0x8080000000808080ULL,
	0x8080000080000000ULL, 0x8080000080000080ULL, 0x8080000080008000ULL, 0x8080000080008080ULL, 0x8080000080800000ULL, 0x8080000080800080ULL, 0x8080000080808000ULL, 0x8080000080808080ULL,
	0x8080008000000000ULL, 0x8080008000000080ULL, 0x8080008000008000ULL, 0x8080008000008080ULL, 0x8080008000800000ULL, 0x8080008000800080ULL, 0x8080008000808000ULL, 0x8080008000808080ULL,
	0x8080008080000000ULL, 0x8080008080000080ULL, 0x8080008080008000ULL, 0x8080008080008080ULL, 0x8080008080800000ULL, 0x8080008080800080ULL, 0x8080008080808000ULL, 0x8080008080808080ULL,
	0x8080800000000000ULL, 0x8080800000000080ULL, 0x8080800000008000ULL, 0x8080800000008080ULL, 0x8080800000800000ULL, 0x8080800000800080ULL, 0x8080800000808000ULL, 0x8080800000808080ULL,
	0x8080800080000000ULL, 0x8080800080000080ULL, 0x8080800080008000ULL, 0x8080800080008080ULL, 0x8080800080800000ULL, 0x8080800080800080ULL, 0x8080800080808000ULL, 0x8080800080808080ULL,
	0x8080808000000000ULL, 0x8080808000000080ULL, 0x8080808000008000ULL, 0x8080808000008080ULL, 0x8080808000800000ULL, 0x8080808000800080ULL, 0x8080808000808000ULL, 0x8080808000808080ULL,
	0x8080808080000000ULL, 0x8080808080000080ULL, 0x8080808080008000ULL, 0x8080808080008080ULL, 0x8080808080800000ULL, 0x8080808080800080ULL, 0x8080808080808000ULL, 0x8080808080808080ULL,
};

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
 * @brief Set a board from a Board object.
 *
 * @param board the board to set
 * @param obj object describing the board. assumes player attribute as black, opponent attribute as white
 * @param turn turn's color
 * @return turn's color.
 * @date 2018
 * @author lavox
 */
int board_from_obj(Board *board, const Board *obj, const int turn)
{
	board->player = obj->player;
	board->opponent = obj->opponent;
	board_check(board);
	switch (turn) {
	case BLACK:
		return BLACK;
	case WHITE:
		board_swap_players(board);
		return WHITE;
	default:
		break;
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
	if (board->player & board->opponent) {
		error("Two discs on the same square?\n");
		board_print(board, BLACK, stderr);
		bitboard_write(board->player, stderr);
		bitboard_write(board->opponent, stderr);
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
 * @brief Compare two board
 *
 * @param b1 first board
 * @param b2 second board
 * @return -1, 0, 1
 */
int board_compare(const Board *b1, const Board *b2)
{
	if (b1->player > b2->player) return 1;
	else if (b1->player < b2->player) return -1;
	else if (b1->opponent > b2->opponent) return 1;
	else if (b1->opponent < b2->opponent) return -1;
	else return 0;
}

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
	register unsigned long long player = board->player;
	register unsigned long long opponent = board->opponent;

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
	Board sym;
	int i, s = 0;

	assert(board != unique);

	*unique = *board;
	for (i = 1; i < 8; ++i) {
		board_symetry(board, i, &sym);
		if (board_compare(&sym, unique) < 0) {
			*unique = sym;
			s = i;
		}
	}

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
	Move move[1];
	unsigned long long moves;
	int ply;

	board_init(board);
	for (ply = 0; ply < n_ply; ply++) {
		moves = get_moves(board->player, board->opponent);
		if (!moves) {
			board_pass(board);
			moves = get_moves(board->player, board->opponent);
			if (!moves) {
				break;
			}
		}
		board_get_move(board, get_rand_bit(moves, r), move);
		board_update(board, move);
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
unsigned long long board_get_move(const Board *board, const int x, Move *move)
{
	move->flipped = flip[x](board->player, board->opponent);
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
	else if (move->flipped != flip[move->x](board->player, board->opponent)) return false;
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
	board->player ^= (move->flipped | x_to_bit(move->x));
	board->opponent ^= move->flipped;
	board_swap_players(board);

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
	board_swap_players(board);
	board->player ^= (move->flipped | x_to_bit(move->x));
	board->opponent ^= move->flipped;

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
unsigned long long board_next(const Board *board, const int x, Board *next)
{
	const unsigned long long flipped = flip[x](board->player, board->opponent);
	const unsigned long long player = board->opponent ^ flipped;

	next->opponent = board->player ^ (flipped | x_to_bit(x));
	next->player = player;

	return flipped;
}

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
	const unsigned long long flipped = flip[x](board->opponent, board->player);

	next->opponent = board->opponent ^ (flipped | x_to_bit(x));
	next->player = board->player ^ flipped;

	return flipped;
}

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
{

#if PARALLEL_PREFIX & 1
	// 1-stage Parallel Prefix (intermediate between kogge stone & sequential) 
	// 6 << + 6 >> + 7 | + 10 &
	register unsigned long long flip_l, flip_r;
	register unsigned long long mask_l, mask_r;
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
	register unsigned long long flip_l, flip_r;
	register unsigned long long mask_l, mask_r;
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
	register unsigned long long flip;

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
DLL_API unsigned long long get_moves(const unsigned long long P, const unsigned long long O)
{
#if defined(USE_GAS_MMX)
 			/* mm7: P, mm6: O */
	const unsigned long long mask_7e = 0x7e7e7e7e7e7e7e7eULL;

  __asm__ volatile(
	"movl	%3, %%esi\n\t"		"movq	%1, %%mm7\n\t"
	"movl	%4, %%edi\n\t"		"movq	%2, %%mm6\n\t"
			/* shift=+1 */			/* shift=+8 */
	"movl	%%esi, %%eax\n\t"	"movq	%%mm7, %%mm0\n\t"
	"movq	%5, %%mm5\n\t"
	"shrl	$1, %%eax\n\t"		"psrlq	$8, %%mm0\n\t"
	"andl	$2122219134, %%edi\n\t"	"pand	%%mm6, %%mm5\n\t"
	"andl	%%edi, %%eax\n\t"	"pand	%%mm6, %%mm0\n\t"	/* 0 m7&o6 m6&o5 .. m1&o0 */
	"movl	%%eax, %%edx\n\t"	"movq	%%mm0, %%mm1\n\t"
	"shrl	$1, %%eax\n\t"		"psrlq	$8, %%mm0\n\t"
	"movl	%%edi, %%ecx\n\t"	"movq	%%mm6, %%mm3\n\t"
	"andl	%%edi, %%eax\n\t"	"pand	%%mm6, %%mm0\n\t"	/* 0 0 m7&o6&o5 .. m2&o1&o0 */
	"shrl	$1, %%ecx\n\t"		"psrlq	$8, %%mm3\n\t"
	"orl	%%edx, %%eax\n\t"	"por	%%mm1, %%mm0\n\t"	/* 0 m7&o6 (m6&o5)|(m7&o6&o5) .. (m1&o0) */
	"andl	%%edi, %%ecx\n\t"	"pand	%%mm6, %%mm3\n\t"	/* 0 o7&o6 o6&o5 o5&o4 o4&o3 .. */
	"movl	%%eax, %%edx\n\t"	"movq	%%mm0, %%mm4\n\t"
	"shrl	$2, %%eax\n\t"		"psrlq	$16, %%mm0\n\t"
	"andl	%%ecx, %%eax\n\t"	"pand	%%mm3, %%mm0\n\t"	/* 0 0 0 m7&o6&o5&o4 (m6&o5&o4&o3)|(m7&o6&o5&o4&o3) .. */
	"orl	%%eax, %%edx\n\t"	"por	%%mm0, %%mm4\n\t"
	"shrl	$2, %%eax\n\t"		"psrlq	$16, %%mm0\n\t"
	"andl	%%ecx, %%eax\n\t"	"pand	%%mm3, %%mm0\n\t"	/* 0 0 0 0 0 m7&o6&..&o2 (m6&o5&..&o1)|(m7&o6&..&o1) .. */
	"orl	%%edx, %%eax\n\t"	"por	%%mm0, %%mm4\n\t"
	"shrl	$1, %%eax\n\t"		"psrlq	$8, %%mm4\n\t"		/* result of +8 */
	/* shift=-1 */				/* shift=-8 */
	"movq	%%mm7, %%mm0\n\t"
	"addl	%%esi, %%esi\n\t"	"psllq	$8, %%mm0\n\t"
	"andl	%%edi, %%esi\n\t"	"pand	%%mm6, %%mm0\n\t"
	"movl	%%esi, %%edx\n\t"	"movq	%%mm0, %%mm1\n\t"
	"addl	%%esi, %%esi\n\t"	"psllq	$8, %%mm0\n\t"
	"andl	%%edi, %%esi\n\t"	"pand	%%mm6, %%mm0\n\t"
	"orl	%%esi, %%edx\n\t"	"por	%%mm1, %%mm0\n\t"
	"addl	%%ecx, %%ecx\n\t"	"psllq	$8, %%mm3\n\t"
	"movq	%%mm0, %%mm1\n\t"
	"leal	(,%%edx,4), %%esi\n\t"	"psllq	$16, %%mm0\n\t"
	"andl	%%ecx, %%esi\n\t"	"pand	%%mm3, %%mm0\n\t"
	"orl	%%esi, %%edx\n\t"	"por	%%mm0, %%mm1\n\t"
	"shll	$2, %%esi\n\t"		"psllq	$16, %%mm0\n\t"
	"andl	%%ecx, %%esi\n\t"	"pand	%%mm3, %%mm0\n\t"
	"orl	%%edx, %%esi\n\t"	"por	%%mm1, %%mm0\n\t"
	"addl	%%esi, %%esi\n\t"	"psllq	$8, %%mm0\n\t"
	"orl	%%eax, %%esi\n\t"	"por	%%mm0, %%mm4\n\t"
	/* Serialize */				/* shift=+7 */
	"movq	%%mm7, %%mm0\n\t"
	"movd	%%esi, %%mm1\n\t"
	"psrlq	$7, %%mm0\n\t"
	"psllq	$32, %%mm1\n\t"
	"pand	%%mm5, %%mm0\n\t"
	"por	%%mm1, %%mm4\n\t"
	"movq	%%mm0, %%mm1\n\t"
	"psrlq	$7, %%mm0\n\t"
	"pand	%%mm5, %%mm0\n\t"
	"movq	%%mm5, %%mm3\n\t"
	"por	%%mm1, %%mm0\n\t"
	"psrlq	$7, %%mm3\n\t"
	"movq	%%mm0, %%mm1\n\t"
	"pand	%%mm5, %%mm3\n\t"
	"psrlq	$14, %%mm0\n\t"
	"pand	%%mm3, %%mm0\n\t"
	"movl	%1, %%esi\n\t"		"por	%%mm0, %%mm1\n\t"
	"movl	%2, %%edi\n\t"		"psrlq	$14, %%mm0\n\t"
	"andl	$2122219134, %%edi\n\t"	"pand	%%mm3, %%mm0\n\t"
	"movl	%%edi, %%ecx\n\t"	"por	%%mm1, %%mm0\n\t"
	"shrl	$1, %%ecx\n\t"		"psrlq	$7, %%mm0\n\t"
	"andl	%%edi, %%ecx\n\t"	"por	%%mm0, %%mm4\n\t"
			/* shift=+1 */			/* shift=-7 */
	"movl	%%esi, %%eax\n\t"	"movq	%%mm7, %%mm0\n\t"
	"shrl	$1, %%eax\n\t"		"psllq	$7, %%mm0\n\t"
	"andl	%%edi, %%eax\n\t"	"pand	%%mm5, %%mm0\n\t"
	"movl	%%eax, %%edx\n\t"	"movq	%%mm0, %%mm1\n\t"
	"shrl	$1, %%eax\n\t"		"psllq	$7, %%mm0\n\t"
	"andl	%%edi, %%eax\n\t"	"pand	%%mm5, %%mm0\n\t"
	"orl	%%edx, %%eax\n\t"	"por	%%mm1, %%mm0\n\t"
					"psllq	$7, %%mm3\n\t"
	"movl	%%eax, %%edx\n\t"	"movq	%%mm0, %%mm1\n\t"
	"shrl	$2, %%eax\n\t"		"psllq	$14, %%mm0\n\t"
	"andl	%%ecx, %%eax\n\t"	"pand	%%mm3, %%mm0\n\t"
	"orl	%%eax, %%edx\n\t"	"por	%%mm0, %%mm1\n\t"
	"shrl	$2, %%eax\n\t"		"psllq	$14, %%mm0\n\t"
	"andl	%%ecx, %%eax\n\t"	"pand	%%mm3, %%mm0\n\t"
	"orl	%%edx, %%eax\n\t"	"por	%%mm1, %%mm0\n\t"
	"shrl	$1, %%eax\n\t"		"psllq	$7, %%mm0\n\t"
	"por	%%mm0, %%mm4\n\t"
	/* shift=-1 */			/* shift=+9 */
	"movq	%%mm7, %%mm0\n\t"
	"addl	%%esi, %%esi\n\t"	"psrlq	$9, %%mm0\n\t"
	"andl	%%edi, %%esi\n\t"	"pand	%%mm5, %%mm0\n\t"
	"movl	%%esi, %%edx\n\t"	"movq	%%mm0, %%mm1\n\t"
	"addl	%%esi, %%esi\n\t"	"psrlq	$9, %%mm0\n\t"
	"andl	%%edi, %%esi\n\t"	"pand	%%mm5, %%mm0\n\t"
	"movq	%%mm5, %%mm3\n\t"
	"orl	%%esi, %%edx\n\t"	"por	%%mm1, %%mm0\n\t"
	"psrlq	$9, %%mm3\n\t"
	"movq	%%mm0, %%mm1\n\t"
	"addl	%%ecx, %%ecx\n\t"	"pand	%%mm5, %%mm3\n\t"
	"leal	(,%%edx,4), %%esi\n\t"	"psrlq	$18, %%mm0\n\t"
	"andl	%%ecx, %%esi\n\t"	"pand	%%mm3, %%mm0\n\t"
	"orl	%%esi, %%edx\n\t"	"por	%%mm0, %%mm1\n\t"
	"shll	$2, %%esi\n\t"		"psrlq	$18, %%mm0\n\t"
	"andl	%%ecx, %%esi\n\t"	"pand	%%mm3, %%mm0\n\t"
	"orl	%%edx, %%esi\n\t"	"por	%%mm1, %%mm0\n\t"
	"addl	%%esi, %%esi\n\t"	"psrlq	$9, %%mm0\n\t"
	"orl	%%eax, %%esi\n\t"	"por	%%mm0, %%mm4\n\t"
	/* Serialize */			/* shift=-9 */
	"movq	%%mm7, %%mm0\n\t"
	"movd	%%esi, %%mm1\n\t"
	"psllq	$9, %%mm0\n\t"
	"por	%%mm1, %%mm4\n\t"
	"pand	%%mm5, %%mm0\n\t"
	"movq	%%mm0, %%mm1\n\t"
	"psllq	$9, %%mm0\n\t"
	"pand	%%mm5, %%mm0\n\t"
	"por	%%mm1, %%mm0\n\t"
	"psllq	$9, %%mm3\n\t"
	"movq	%%mm0, %%mm1\n\t"
	"psllq	$18, %%mm0\n\t"
	"pand	%%mm3, %%mm0\n\t"
	"por	%%mm0, %%mm1\n\t"
	"psllq	$18, %%mm0\n\t"
	"pand	%%mm3, %%mm0\n\t"
	"por	%%mm1, %%mm0\n\t"
	"psllq	$9, %%mm0\n\t"
	"por	%%mm0, %%mm4\n\t"
	/* mm4 is the pseudo-feasible moves at this point. */
	/* Let mm7 be the feasible moves, i.e., mm4 restricted to empty squares. */
	"por	%%mm6, %%mm7\n\t"
	"pandn	%%mm4, %%mm7\n\t"
	"movq	%%mm7, %0\n\t"
	"emms"		/* Reset the FP/MMX unit. */
    : "=g" (moves) : "m" (P), "m" (O), "g" (P >> 32), "g" (O >> 32), "m" (mask_7e) : "eax", "edx", "ecx", "esi", "edi" );

#else
	const unsigned long long mask = O & 0x7E7E7E7E7E7E7E7Eull;

	return (get_some_moves(P, mask, 1) // horizontal
		| get_some_moves(P, O, 8)   // vertical
		| get_some_moves(P, mask, 7)   // diagonals
		| get_some_moves(P, mask, 9))
		& ~(P|O); // mask with empties

#endif
}

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
	const unsigned long long E = (~(P|O) & 0x007E7E7E7E7E7E00ull); // empties

	return ((get_some_moves(P, O & 0x003C3C3C3C3C3C00ull, 1) // horizontal
		| get_some_moves(P, O & 0x00007E7E7E7E0000ull, 8)   // vertical
		| get_some_moves(P, O & 0x00003C3C3C3C0000ull, 7)   // diagonals
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
DLL_API bool can_move(const unsigned long long P, const unsigned long long O)
{
	const unsigned long long E = ~(P|O); // empties

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
bool can_move_6x6(const unsigned long long P, const unsigned long long O)
{
	const unsigned long long E = (~(P|O) & 0x007E7E7E7E7E7E00ull); // empties

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
int get_mobility(const unsigned long long P, const unsigned long long O)
{
	return bit_count(get_moves(P, O));
}

int get_weighted_mobility(const unsigned long long P, const unsigned long long O)
{
	return bit_weighted_count(get_moves(P, O));
}

/**
 * @brief Get some potential moves.
 *
 * @param P bitboard with player's discs.
 * @param dir flipping direction.
 * @return some potential moves in a 64-bit unsigned integer.
 */
static inline unsigned long long get_some_potential_moves(const unsigned long long P, const int dir)
{
	return (P << dir | P >> dir);
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
static unsigned long long get_potential_moves(const unsigned long long P, const unsigned long long O)
{
	return (get_some_potential_moves(O & 0x7E7E7E7E7E7E7E7Eull, 1) // horizontal
		| get_some_potential_moves(O & 0x00FFFFFFFFFFFF00ull, 8)   // vertical
		| get_some_potential_moves(O & 0x007E7E7E7E7E7E00ull, 7)   // diagonals
		| get_some_potential_moves(O & 0x007E7E7E7E7E7E00ull, 9))
		& ~(P|O); // mask with empties
}

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
	return bit_weighted_count(get_potential_moves(P, O));
}

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
	register int P, O, x, y;
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
			edge_stability[P][O] = 0;
		} else {
			edge_stability[P][O] = find_edge_stable(P, O, P);
		}
	}
}

/**
 * @brief Get full lines.
 *
 * @param line all discs on a line.
 * @param dir tested direction
 * @return a bitboard with full lines along the tested direction.
 */
static inline unsigned long long get_full_lines(const unsigned long long line, const int dir)
{
#if KOGGE_STONE & 2

	// kogge-stone algorithm
 	// 5 << + 5 >> + 7 & + 10 |
	// + better instruction independency
	register unsigned long long full_l, full_r, edge_l, edge_r;
	const  unsigned long long edge = 0xff818181818181ffULL;
	const int dir2 = dir << 1;
	const int dir4 = dir << 2;

	full_l = line & (edge | (line >> dir)); full_r  = line & (edge | (line << dir));
	edge_l = edge | (edge >> dir);        edge_r  = edge | (edge << dir);
	full_l &= edge_l | (full_l >> dir2);  full_r &= edge_r | (full_r << dir2);
	edge_l |= edge_l >> dir2;             edge_r |= edge_r << dir2;
	full_l &= edge_l | (full_l >> dir4);  full_r &= edge_r | (full_r << dir4);

	return full_r & full_l;

#elif PARALLEL_PREFIX & 2

	// 1-stage Parallel Prefix (intermediate between kogge stone & sequential) 
	// 5 << + 5 >> + 7 & + 10 |
	register unsigned long long full_l, full_r;
	register unsigned long long edge_l, edge_r;
	const  unsigned long long edge = 0xff818181818181ffULL;
	const int dir2 = dir + dir;

	full_l  = edge | (line << dir);       full_r  = edge | (line >> dir);
	full_l &= edge | (full_l << dir);     full_r &= edge | (full_r >> dir);
	edge_l  = edge | (edge << dir);       edge_r  = edge | (edge >> dir);
	full_l &= edge_l | (full_l << dir2);  full_r &= edge_r | (full_r >> dir2);
	full_l &= edge_l | (full_l << dir2);  full_r &= edge_r | (full_r >> dir2);

	return full_l & full_r;

#else

	// sequential algorithm
 	// 6 << + 6 >> + 12 & + 5 |
	register unsigned long long full;
	const unsigned long long edge = line & 0xff818181818181ffULL;

	full = (line & (((line >> dir) & (line << dir)) | edge));
	full &= (((full >> dir) & (full << dir)) | edge);
	full &= (((full >> dir) & (full << dir)) | edge);
	full &= (((full >> dir) & (full << dir)) | edge);
	full &= (((full >> dir) & (full << dir)) | edge);

	return ((full >> dir) & (full << dir));

#endif
}


#ifdef __X86_64__
#define	packA1A8(X)	((((X) & 0x0101010101010101ULL) * 0x0102040810204080ULL) >> 56)
#define	packH1H8(X)	((((X) & 0x8080808080808080ULL) * 0x0002040810204081ULL) >> 56)
#else
#define	packA1A8(X)	(((((unsigned int)(X) & 0x01010101u) + (((unsigned int)((X) >> 32) & 0x01010101u) << 4)) * 0x01020408u) >> 24)
#define	packH1H8(X)	(((((unsigned int)((X) >> 32) & 0x80808080u) + (((unsigned int)(X) & 0x80808080u) >> 4)) * 0x00204081u) >> 24)
#endif

/**
 * @brief Get stable edge.
 *
 * @param P bitboard with player's discs.
 * @param O bitboard with opponent's discs.
 * @return a bitboard with (some of) player's stable discs.
 *
 */
static inline unsigned long long get_stable_edge(const unsigned long long P, const unsigned long long O)
{
	// compute the exact stable edges (from precomputed tables)
	return edge_stability[P & 0xff][O & 0xff]
	    |  ((unsigned long long)edge_stability[P >> 56][O >> 56]) << 56
	    |  A1_A8[edge_stability[packA1A8(P)][packA1A8(O)]]
	    |  H1_H8[edge_stability[packH1H8(P)][packH1H8(O)]];
}

/**
 * @brief Estimate the stability.
 *
 * Count the number (in fact a lower estimate) of stable discs.
 *
 * @param P bitboard with player's discs.
 * @param O bitboard with opponent's discs.
 * @return the number of stable discs.
 */
int get_stability(const unsigned long long P, const unsigned long long O)
{
	const unsigned long long disc = (P | O);
	const unsigned long long central_mask = (P & 0x007e7e7e7e7e7e00ULL);
	const unsigned long long full_h = get_full_lines(disc, 1);
	const unsigned long long full_v = get_full_lines(disc, 8);
	const unsigned long long full_d7 = get_full_lines(disc, 7);
	const unsigned long long full_d9 = get_full_lines(disc, 9);
	register unsigned long long stable_h, stable_v, stable_d7, stable_d9, stable, new_stable;

	// compute the exact stable edges (from precomputed tables)
	new_stable = get_stable_edge(P, O);

	// add full lines
	new_stable |= (full_h & full_v & full_d7 & full_d9 & central_mask);

	// now compute the other stable discs (ie discs touching another stable disc in each flipping direction).
	stable = 0;
	while (new_stable & ~stable) {
		stable |= new_stable;
		stable_h = ((stable >> 1) | (stable << 1) | full_h);
		stable_v = ((stable >> 8) | (stable << 8) | full_v);
		stable_d7 = ((stable >> 7) | (stable << 7) | full_d7);
		stable_d9 = ((stable >> 9) | (stable << 9) | full_d9);
		new_stable = (stable_h & stable_v & stable_d7 & stable_d9 & central_mask);
	}

	return bit_count(stable);
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
	return bit_count(get_stable_edge(P, O));
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
int get_corner_stability(const unsigned long long P)
{
	const unsigned long long stable = ((((0x0100000000000001ULL & P) << 1) | ((0x8000000000000080ULL & P) >> 1) | ((0x0000000000000081ULL & P) << 8) | ((0x8100000000000000ULL & P) >> 8) | 0x8100000000000081ULL) & P);
	return bit_count(stable);
}

/**
 * @brief Compute a hash code.
 *
 * @param board the board.
 * @return the hash code of the bitboard
 */
unsigned long long board_get_hash_code(const Board *board)
{
	unsigned long long h1, h2;
	const unsigned char *p = (const unsigned char*)board;

	h1  = hash_rank[0][p[0]];
	h2  = hash_rank[1][p[1]];
	h1 ^= hash_rank[2][p[2]];
	h2 ^= hash_rank[3][p[3]];
	h1 ^= hash_rank[4][p[4]];
	h2 ^= hash_rank[5][p[5]];
	h1 ^= hash_rank[6][p[6]];
	h2 ^= hash_rank[7][p[7]];
	h1 ^= hash_rank[8][p[8]];
	h2 ^= hash_rank[9][p[9]];
	h1 ^= hash_rank[10][p[10]];
	h2 ^= hash_rank[11][p[11]];
	h1 ^= hash_rank[12][p[12]];
	h2 ^= hash_rank[13][p[13]];
	h1 ^= hash_rank[14][p[14]];
	h2 ^= hash_rank[15][p[15]];

	return h1 ^ h2;
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
	return (board->player | board->opponent) & x_to_bit(x);
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
	const char *color = "?*O-." + 1;
	unsigned long long moves = get_moves(board->player, board->opponent);

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

