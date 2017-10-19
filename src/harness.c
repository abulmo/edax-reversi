/**
 * @file harness.c
 *
 * This module verifies if the move generator is correct.
 *
 * @date 1998 - 2017
 * @author Richard Delorme
 * @version 4.4
 */

#include "bit.h"
#include "board.h"

unsigned long long flip_slow(const unsigned long long P, const unsigned long long O, const int x0)
{
	int x, d, dir[8] = {-9,-8,-7,-1,1,7,8,9};
	const unsigned long long edge[8] = {
		0x01010101010101ffull,
		0x00000000000000ffull,
		0x80808080808080ffull,
		0x0101010101010101ull,
		0x8080808080808080ull,
		0xff01010101010101ull,
		0xff00000000000000ull,
		0xff80808080808080ull
	};
	unsigned long long flipped = 0, f;
	char s[3];

	if (x0 == PASS) return;

	for (d = 0; d < 8; ++d) {
		if ((x_to_bit(x0) & edge[d]) == 0) {
			f = 0;
			for (x = x0 + dir[d];  (O & x_to_bit(x)) && (x_to_bit(x) & edge[d]) == 0; x += dir[d]) {
				f |= x_to_bit(x);
			}
			if (board->player & x_to_bit(x)) flipped |= f;
		}
	}
	return flipped;

	
if (move->flipped != flipped) {
		printf("Bug found in flip[%s]()\n", move_to_string(move->x, 1, s));
		board_print(board, 0, stdout);
		bitboard_write(move->flipped, stdout);
		bitboard_write(flipped, stdout);
		abort();
	}
}

