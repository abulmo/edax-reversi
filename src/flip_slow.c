/**
 * @file flip_slow.c
 *
 * This module verifies if the move generator is correct.
 *
 * @date 1998 - 2024
 * @author Richard Delorme
 * @version 4.6
 */

#include "bit.h"

#ifndef NDEBUG

uint64_t flip_slow(const uint64_t P, const uint64_t O, const int x0)
{
	int x, d, dir[8] = {-9,-8,-7,-1,1,7,8,9};
	const uint64_t edge[8] = {
		0x01010101010101ffull,
		0x00000000000000ffull,
		0x80808080808080ffull,
		0x0101010101010101ull,
		0x8080808080808080ull,
		0xff01010101010101ull,
		0xff00000000000000ull,
		0xff80808080808080ull
	};
	uint64_t flipped, f;

	if (x0 == PASS) return 0;

	flipped = 0;
	for (d = 0; d < 8; ++d) {
		if ((x_to_bit(x0) & edge[d]) == 0) {
			f = 0;
			for (x = x0 + dir[d];  (O & x_to_bit(x)) && (x_to_bit(x) & edge[d]) == 0; x += dir[d]) {
				f |= x_to_bit(x);
			}
			if (P & x_to_bit(x)) flipped |= f;
		}
	}
	return flipped;
}

bool test_generator(const uint64_t flipped, const uint64_t P, const uint64_t O, const int x0)
{
	if (flipped != flip_slow(P, O, x0)) {
		Board b[1];
		char s[3] = "--";
		b->player = P; b->opponent = O;
		board_print(b, BLACK, stderr);
		fprintf(stderr, "move wrong : %s\n", move_to_string(x0, BLACK, s));
		bitboard_print(flipped, stderr);
		return true;
	}
	return false;
}

#endif

