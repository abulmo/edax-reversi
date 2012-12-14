/**
 * @file count_last_flip_kindergarten.c
 *
 *
 * A function is provided to count the number of fipped disc of the last move
 * for each square of the board. These functions are gathered into an array of
 * functions, so that a fast access to each function is allowed. The generic
 * form of the function take as input the player bitboard and return twice
 * the number of flipped disc of the last move.
 *
 * The basic principle is to read into an array a precomputed result. Doing
 * this is easy for a single line ; as we can use arrays of the form:
 *  - COUNT_FLIP[square where we play][8-bits disc pattern].
 * The problem is thus to convert any line of a 64-bits disc pattern into an
 * 8-bits disc pattern. A fast way to do this is to select the right line,
 * with a bit-mask, to gather the masked-bits into a continuous set by a simple
 * multiplication and to right-shift the result to scale it into a number
 * between 0 and 255.
 * Once we get our 8-bits disc patterns, we directly get the number of
 * flipped discs from the precomputed array, and add them from each flipping
 * lines.
 * For optimization purpose, the value returned is twice the number of flipped
 * disc, to facilitate the computation of disc difference.
 *
 * With Modifications by Valéry ClaudePierre (merging diagonals).
 * @todo 135° merge as done by Toshihiko Okuhara
 *
 * @date 1998 - 2012
 * @author Richard Delorme
 * @version 4.3
 *
 */

/** precomputed count flip array */
const char COUNT_FLIP[8][256] = {
	{
		 0,  0,  0,  0,  2,  2,  0,  0,  4,  4,  0,  0,  2,  2,  0,  0,  6,  6,  0,  0,  2,  2,  0,  0,  4,  4,  0,  0,  2,  2,  0,  0,
		 8,  8,  0,  0,  2,  2,  0,  0,  4,  4,  0,  0,  2,  2,  0,  0,  6,  6,  0,  0,  2,  2,  0,  0,  4,  4,  0,  0,  2,  2,  0,  0,
		10, 10,  0,  0,  2,  2,  0,  0,  4,  4,  0,  0,  2,  2,  0,  0,  6,  6,  0,  0,  2,  2,  0,  0,  4,  4,  0,  0,  2,  2,  0,  0,
		 8,  8,  0,  0,  2,  2,  0,  0,  4,  4,  0,  0,  2,  2,  0,  0,  6,  6,  0,  0,  2,  2,  0,  0,  4,  4,  0,  0,  2,  2,  0,  0,
		12, 12,  0,  0,  2,  2,  0,  0,  4,  4,  0,  0,  2,  2,  0,  0,  6,  6,  0,  0,  2,  2,  0,  0,  4,  4,  0,  0,  2,  2,  0,  0,
		 8,  8,  0,  0,  2,  2,  0,  0,  4,  4,  0,  0,  2,  2,  0,  0,  6,  6,  0,  0,  2,  2,  0,  0,  4,  4,  0,  0,  2,  2,  0,  0,
		10, 10,  0,  0,  2,  2,  0,  0,  4,  4,  0,  0,  2,  2,  0,  0,  6,  6,  0,  0,  2,  2,  0,  0,  4,  4,  0,  0,  2,  2,  0,  0,
		 8,  8,  0,  0,  2,  2,  0,  0,  4,  4,  0,  0,  2,  2,  0,  0,  6,  6,  0,  0,  2,  2,  0,  0,  4,  4,  0,  0,  2,  2,  0,  0,
	},
	{
		 0,  0,  0,  0,  0,  0,  0,  0,  2,  2,  2,  2,  0,  0,  0,  0,  4,  4,  4,  4,  0,  0,  0,  0,  2,  2,  2,  2,  0,  0,  0,  0,
		 6,  6,  6,  6,  0,  0,  0,  0,  2,  2,  2,  2,  0,  0,  0,  0,  4,  4,  4,  4,  0,  0,  0,  0,  2,  2,  2,  2,  0,  0,  0,  0,
		 8,  8,  8,  8,  0,  0,  0,  0,  2,  2,  2,  2,  0,  0,  0,  0,  4,  4,  4,  4,  0,  0,  0,  0,  2,  2,  2,  2,  0,  0,  0,  0,
		 6,  6,  6,  6,  0,  0,  0,  0,  2,  2,  2,  2,  0,  0,  0,  0,  4,  4,  4,  4,  0,  0,  0,  0,  2,  2,  2,  2,  0,  0,  0,  0,
		10, 10, 10, 10,  0,  0,  0,  0,  2,  2,  2,  2,  0,  0,  0,  0,  4,  4,  4,  4,  0,  0,  0,  0,  2,  2,  2,  2,  0,  0,  0,  0,
		 6,  6,  6,  6,  0,  0,  0,  0,  2,  2,  2,  2,  0,  0,  0,  0,  4,  4,  4,  4,  0,  0,  0,  0,  2,  2,  2,  2,  0,  0,  0,  0,
		 8,  8,  8,  8,  0,  0,  0,  0,  2,  2,  2,  2,  0,  0,  0,  0,  4,  4,  4,  4,  0,  0,  0,  0,  2,  2,  2,  2,  0,  0,  0,  0,
		 6,  6,  6,  6,  0,  0,  0,  0,  2,  2,  2,  2,  0,  0,  0,  0,  4,  4,  4,  4,  0,  0,  0,  0,  2,  2,  2,  2,  0,  0,  0,  0,
	},
	{
		 0,  2,  0,  0,  0,  2,  0,  0,  0,  2,  0,  0,  0,  2,  0,  0,  2,  4,  2,  2,  2,  4,  2,  2,  0,  2,  0,  0,  0,  2,  0,  0,
		 4,  6,  4,  4,  4,  6,  4,  4,  0,  2,  0,  0,  0,  2,  0,  0,  2,  4,  2,  2,  2,  4,  2,  2,  0,  2,  0,  0,  0,  2,  0,  0,
		 6,  8,  6,  6,  6,  8,  6,  6,  0,  2,  0,  0,  0,  2,  0,  0,  2,  4,  2,  2,  2,  4,  2,  2,  0,  2,  0,  0,  0,  2,  0,  0,
		 4,  6,  4,  4,  4,  6,  4,  4,  0,  2,  0,  0,  0,  2,  0,  0,  2,  4,  2,  2,  2,  4,  2,  2,  0,  2,  0,  0,  0,  2,  0,  0,
		 8, 10,  8,  8,  8, 10,  8,  8,  0,  2,  0,  0,  0,  2,  0,  0,  2,  4,  2,  2,  2,  4,  2,  2,  0,  2,  0,  0,  0,  2,  0,  0,
		 4,  6,  4,  4,  4,  6,  4,  4,  0,  2,  0,  0,  0,  2,  0,  0,  2,  4,  2,  2,  2,  4,  2,  2,  0,  2,  0,  0,  0,  2,  0,  0,
		 6,  8,  6,  6,  6,  8,  6,  6,  0,  2,  0,  0,  0,  2,  0,  0,  2,  4,  2,  2,  2,  4,  2,  2,  0,  2,  0,  0,  0,  2,  0,  0,
		 4,  6,  4,  4,  4,  6,  4,  4,  0,  2,  0,  0,  0,  2,  0,  0,  2,  4,  2,  2,  2,  4,  2,  2,  0,  2,  0,  0,  0,  2,  0,  0,
	},
	{
		 0,  4,  2,  2,  0,  0,  0,  0,  0,  4,  2,  2,  0,  0,  0,  0,  0,  4,  2,  2,  0,  0,  0,  0,  0,  4,  2,  2,  0,  0,  0,  0,
		 2,  6,  4,  4,  2,  2,  2,  2,  2,  6,  4,  4,  2,  2,  2,  2,  0,  4,  2,  2,  0,  0,  0,  0,  0,  4,  2,  2,  0,  0,  0,  0,
		 4,  8,  6,  6,  4,  4,  4,  4,  4,  8,  6,  6,  4,  4,  4,  4,  0,  4,  2,  2,  0,  0,  0,  0,  0,  4,  2,  2,  0,  0,  0,  0,
		 2,  6,  4,  4,  2,  2,  2,  2,  2,  6,  4,  4,  2,  2,  2,  2,  0,  4,  2,  2,  0,  0,  0,  0,  0,  4,  2,  2,  0,  0,  0,  0,
		 6, 10,  8,  8,  6,  6,  6,  6,  6, 10,  8,  8,  6,  6,  6,  6,  0,  4,  2,  2,  0,  0,  0,  0,  0,  4,  2,  2,  0,  0,  0,  0,
		 2,  6,  4,  4,  2,  2,  2,  2,  2,  6,  4,  4,  2,  2,  2,  2,  0,  4,  2,  2,  0,  0,  0,  0,  0,  4,  2,  2,  0,  0,  0,  0,
		 4,  8,  6,  6,  4,  4,  4,  4,  4,  8,  6,  6,  4,  4,  4,  4,  0,  4,  2,  2,  0,  0,  0,  0,  0,  4,  2,  2,  0,  0,  0,  0,
		 2,  6,  4,  4,  2,  2,  2,  2,  2,  6,  4,  4,  2,  2,  2,  2,  0,  4,  2,  2,  0,  0,  0,  0,  0,  4,  2,  2,  0,  0,  0,  0,
	},
	{
		 0,  6,  4,  4,  2,  2,  2,  2,  0,  0,  0,  0,  0,  0,  0,  0,  0,  6,  4,  4,  2,  2,  2,  2,  0,  0,  0,  0,  0,  0,  0,  0,
		 0,  6,  4,  4,  2,  2,  2,  2,  0,  0,  0,  0,  0,  0,  0,  0,  0,  6,  4,  4,  2,  2,  2,  2,  0,  0,  0,  0,  0,  0,  0,  0,
		 2,  8,  6,  6,  4,  4,  4,  4,  2,  2,  2,  2,  2,  2,  2,  2,  2,  8,  6,  6,  4,  4,  4,  4,  2,  2,  2,  2,  2,  2,  2,  2,
		 0,  6,  4,  4,  2,  2,  2,  2,  0,  0,  0,  0,  0,  0,  0,  0,  0,  6,  4,  4,  2,  2,  2,  2,  0,  0,  0,  0,  0,  0,  0,  0,
		 4, 10,  8,  8,  6,  6,  6,  6,  4,  4,  4,  4,  4,  4,  4,  4,  4, 10,  8,  8,  6,  6,  6,  6,  4,  4,  4,  4,  4,  4,  4,  4,
		 0,  6,  4,  4,  2,  2,  2,  2,  0,  0,  0,  0,  0,  0,  0,  0,  0,  6,  4,  4,  2,  2,  2,  2,  0,  0,  0,  0,  0,  0,  0,  0,
		 2,  8,  6,  6,  4,  4,  4,  4,  2,  2,  2,  2,  2,  2,  2,  2,  2,  8,  6,  6,  4,  4,  4,  4,  2,  2,  2,  2,  2,  2,  2,  2,
		 0,  6,  4,  4,  2,  2,  2,  2,  0,  0,  0,  0,  0,  0,  0,  0,  0,  6,  4,  4,  2,  2,  2,  2,  0,  0,  0,  0,  0,  0,  0,  0,
	},
	{
		 0,  8,  6,  6,  4,  4,  4,  4,  2,  2,  2,  2,  2,  2,  2,  2,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
		 0,  8,  6,  6,  4,  4,  4,  4,  2,  2,  2,  2,  2,  2,  2,  2,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
		 0,  8,  6,  6,  4,  4,  4,  4,  2,  2,  2,  2,  2,  2,  2,  2,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
		 0,  8,  6,  6,  4,  4,  4,  4,  2,  2,  2,  2,  2,  2,  2,  2,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
		 2, 10,  8,  8,  6,  6,  6,  6,  4,  4,  4,  4,  4,  4,  4,  4,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,
		 2, 10,  8,  8,  6,  6,  6,  6,  4,  4,  4,  4,  4,  4,  4,  4,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,
		 0,  8,  6,  6,  4,  4,  4,  4,  2,  2,  2,  2,  2,  2,  2,  2,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
		 0,  8,  6,  6,  4,  4,  4,  4,  2,  2,  2,  2,  2,  2,  2,  2,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
	},
	{
		 0, 10,  8,  8,  6,  6,  6,  6,  4,  4,  4,  4,  4,  4,  4,  4,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,
		 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
		 0, 10,  8,  8,  6,  6,  6,  6,  4,  4,  4,  4,  4,  4,  4,  4,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,
		 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
		 0, 10,  8,  8,  6,  6,  6,  6,  4,  4,  4,  4,  4,  4,  4,  4,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,
		 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
		 0, 10,  8,  8,  6,  6,  6,  6,  4,  4,  4,  4,  4,  4,  4,  4,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,
		 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
	},
	{
		 0, 12, 10, 10,  8,  8,  8,  8,  6,  6,  6,  6,  6,  6,  6,  6,  4,  4,  4,  4,  4,  4,  4,  4,  4,  4,  4,  4,  4,  4,  4,  4,
		 2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,
		 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
		 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
		 0, 12, 10, 10,  8,  8,  8,  8,  6,  6,  6,  6,  6,  6,  6,  6,  4,  4,  4,  4,  4,  4,  4,  4,  4,  4,  4,  4,  4,  4,  4,  4,
		 2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,
		 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
		 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
	},
};

/**
 * Count last flipped discs when playing on square A1.
 *
 * @param P player's disc pattern.
 * @return flipped disc count.
 */
static int count_last_flip_A1(const unsigned long long P)
{
	register int n_flipped;

	n_flipped  = COUNT_FLIP[0][((P & 0x0101010101010101ULL) * 0x0102040810204080ULL) >> 56];
	n_flipped += COUNT_FLIP[0][P & 0xff];
	n_flipped += COUNT_FLIP[0][((P & 0x8040201008040201ULL) * 0x0101010101010101ULL) >> 56];

	return n_flipped;
}

/**
 * Count last flipped discs when playing on square B1.
 *
 * @param P player's disc pattern.
 * @return flipped disc count.
 */
static int count_last_flip_B1(const unsigned long long P)
{
	register int n_flipped;

	n_flipped  = COUNT_FLIP[0][((P & 0x0202020202020202ULL) * 0x0081020408102040ULL) >> 56];
	n_flipped += COUNT_FLIP[1][P & 0xff];
	n_flipped += COUNT_FLIP[1][((P & 0x0080402010080402ULL) * 0x0101010101010101ULL) >> 56];

	return n_flipped;
}

/**
 * Count last flipped discs when playing on square C1.
 *
 * @param P player's disc pattern.
 * @return flipped disc count.
 */
static int count_last_flip_C1(const unsigned long long P)
{
	register int n_flipped;

	n_flipped  = COUNT_FLIP[0][((P & 0x0404040404040404ULL) * 0x0040810204081020ULL) >> 56];
	n_flipped += COUNT_FLIP[2][P & 0xff];
	n_flipped += COUNT_FLIP[2][(P & 0x0000804020110a04ULL) * 0x0101010101010101ULL >> 56];

	return n_flipped;
}

/**
 * Count last flipped discs when playing on square D1.
 *
 * @param P player's disc pattern.
 * @return flipped disc count.
 */
static int count_last_flip_D1(const unsigned long long P)
{
	register int n_flipped;

	n_flipped  = COUNT_FLIP[0][((P & 0x0808080808080808ULL) * 0x0020408102040810ULL) >> 56];
	n_flipped += COUNT_FLIP[3][P & 0xff];
	n_flipped += COUNT_FLIP[3][(P & 0x0000008041221408ULL) * 0x0101010101010101ULL >> 56];

	return n_flipped;
}

/**
 * Count last flipped discs when playing on square E1.
 *
 * @param P player's disc pattern.
 * @return flipped disc count.
 */
static int count_last_flip_E1(const unsigned long long P)
{
	register int n_flipped;

	n_flipped  = COUNT_FLIP[0][((P & 0x1010101010101010ULL) * 0x0010204081020408ULL) >> 56];
	n_flipped += COUNT_FLIP[4][P & 0xff];
	n_flipped += COUNT_FLIP[4][(P & 0x0000000182442810ULL) * 0x0101010101010101ULL >> 56];

	return n_flipped;
}

/**
 * Count last flipped discs when playing on square F1.
 *
 * @param P player's disc pattern.
 * @return flipped disc count.
 */
static int count_last_flip_F1(const unsigned long long P)
{
	register int n_flipped;

	n_flipped  = COUNT_FLIP[0][((P & 0x2020202020202020ULL) * 0x0008102040810204ULL) >> 56];
	n_flipped += COUNT_FLIP[5][P & 0xff];
	n_flipped += COUNT_FLIP[5][(P & 0x0000010204885020ULL) * 0x0101010101010101ULL >> 56];

	return n_flipped;
}

/**
 * Count last flipped discs when playing on square G1.
 *
 * @param P player's disc pattern.
 * @return flipped disc count.
 */
static int count_last_flip_G1(const unsigned long long P)
{
	register int n_flipped;

	n_flipped  = COUNT_FLIP[0][((P & 0x4040404040404040ULL) * 0x0004081020408102ULL) >> 56];
	n_flipped += COUNT_FLIP[6][P & 0xff];
	n_flipped += COUNT_FLIP[6][((P & 0x0001020408102040ULL) * 0x0101010101010101ULL) >> 56];

	return n_flipped;
}

/**
 * Count last flipped discs when playing on square H1.
 *
 * @param P player's disc pattern.
 * @return flipped disc count.
 */
static int count_last_flip_H1(const unsigned long long P)
{
	register int n_flipped;

	n_flipped  = COUNT_FLIP[0][((P & 0x8080808080808080ULL) * 0x0002040810204081ULL) >> 56];
	n_flipped += COUNT_FLIP[7][P & 0xff];
	n_flipped += COUNT_FLIP[7][((P & 0x0102040810204080ULL) * 0x0101010101010101ULL) >> 56];

	return n_flipped;
}

/**
 * Count last flipped discs when playing on square A2.
 *
 * @param P player's disc pattern.
 * @return flipped disc count.
 */
static int count_last_flip_A2(const unsigned long long P)
{
	register int n_flipped;

	n_flipped  = COUNT_FLIP[1][((P & 0x0101010101010101ULL) * 0x0102040810204080ULL) >> 56];
	n_flipped += COUNT_FLIP[0][(P >> 8) & 0xff];
	n_flipped += COUNT_FLIP[0][((P & 0x4020100804020100ULL) * 0x0101010101010101ULL) >> 56];

	return n_flipped;
}

/**
 * Count last flipped discs when playing on square B2.
 *
 * @param P player's disc pattern.
 * @return flipped disc count.
 */
static int count_last_flip_B2(const unsigned long long P)
{
	register int n_flipped;

	n_flipped  = COUNT_FLIP[1][((P & 0x0202020202020202ULL) * 0x0081020408102040ULL) >> 56];
	n_flipped += COUNT_FLIP[1][(P >> 8) & 0xff];
	n_flipped += COUNT_FLIP[1][((P & 0x8040201008040201ULL) * 0x0101010101010101ULL) >> 56];

	return n_flipped;
}

/**
 * Count last flipped discs when playing on square C2.
 *
 * @param P player's disc pattern.
 * @return flipped disc count.
 */
static int count_last_flip_C2(const unsigned long long P)
{
	register int n_flipped;

	n_flipped  = COUNT_FLIP[1][((P & 0x0404040404040404ULL) * 0x0040810204081020ULL) >> 56];
	n_flipped += COUNT_FLIP[2][(P >> 8) & 0xff];
	n_flipped += COUNT_FLIP[2][(P & 0x00804020110a0400ULL) * 0x0101010101010101ULL >> 56];

	return n_flipped;
}

/**
 * Count last flipped discs when playing on square D2.
 *
 * @param P player's disc pattern.
 * @return flipped disc count.
 */
static int count_last_flip_D2(const unsigned long long P)
{
	register int n_flipped;

	n_flipped  = COUNT_FLIP[1][((P & 0x0808080808080808ULL) * 0x0020408102040810ULL) >> 56];
	n_flipped += COUNT_FLIP[3][(P >> 8) & 0xff];
	n_flipped += COUNT_FLIP[3][(P & 0x0000804122140800ULL) * 0x0101010101010101ULL >> 56];

	return n_flipped;
}

/**
 * Count last flipped discs when playing on square E2.
 *
 * @param P player's disc pattern.
 * @return flipped disc count.
 */
static int count_last_flip_E2(const unsigned long long P)
{
	register int n_flipped;

	n_flipped  = COUNT_FLIP[1][((P & 0x1010101010101010ULL) * 0x0010204081020408ULL) >> 56];
	n_flipped += COUNT_FLIP[4][(P >> 8) & 0xff];
	n_flipped += COUNT_FLIP[4][(P & 0x0000018244281000ULL) * 0x0101010101010101ULL >> 56];

	return n_flipped;
}

/**
 * Count last flipped discs when playing on square F2.
 *
 * @param P player's disc pattern.
 * @return flipped disc count.
 */
static int count_last_flip_F2(const unsigned long long P)
{
	register int n_flipped;

	n_flipped  = COUNT_FLIP[1][((P & 0x2020202020202020ULL) * 0x0008102040810204ULL) >> 56];
	n_flipped += COUNT_FLIP[5][(P >> 8) & 0xff];
	n_flipped += COUNT_FLIP[5][(P & 0x0001020488502000ULL) * 0x0101010101010101ULL >> 56];

	return n_flipped;
}

/**
 * Count last flipped discs when playing on square G2.
 *
 * @param P player's disc pattern.
 * @return flipped disc count.
 */
static int count_last_flip_G2(const unsigned long long P)
{
	register int n_flipped;

	n_flipped  = COUNT_FLIP[1][((P & 0x4040404040404040ULL) * 0x0004081020408102ULL) >> 56];
	n_flipped += COUNT_FLIP[6][(P >> 8) & 0xff];
	n_flipped += COUNT_FLIP[6][((P & 0x0102040810204080ULL) * 0x0101010101010101ULL) >> 56];

	return n_flipped;
}

/**
 * Count last flipped discs when playing on square H2.
 *
 * @param P player's disc pattern.
 * @return flipped disc count.
 */
static int count_last_flip_H2(const unsigned long long P)
{
	register int n_flipped;

	n_flipped  = COUNT_FLIP[1][((P & 0x8080808080808080ULL) * 0x0002040810204081ULL) >> 56];
	n_flipped += COUNT_FLIP[7][(P >> 8) & 0xff];
	n_flipped += COUNT_FLIP[7][((P & 0x0204081020408000ULL) * 0x0101010101010101ULL) >> 56];

	return n_flipped;
}

/**
 * Count last flipped discs when playing on square A3.
 *
 * @param P player's disc pattern.
 * @return flipped disc count.
 */
static int count_last_flip_A3(const unsigned long long P)
{
	register int n_flipped;

	n_flipped  = COUNT_FLIP[2][((P & 0x0101010101010101ULL) * 0x0102040810204080ULL) >> 56];
	n_flipped += COUNT_FLIP[0][(P >> 16) & 0xff];
	n_flipped += COUNT_FLIP[2][(((P & 0x2010080402010204ULL) + 0x6070787c7e7f7e7cULL) & 0x8080808080808080ULL) * 0x0002040810204081ULL >> 56];

	return n_flipped;
}

/**
 * Count last flipped discs when playing on square B3.
 *
 * @param P player's disc pattern.
 * @return flipped disc count.
 */
static int count_last_flip_B3(const unsigned long long P)
{
	register int n_flipped;

	n_flipped  = COUNT_FLIP[2][((P & 0x0202020202020202ULL) * 0x0081020408102040ULL) >> 56];
	n_flipped += COUNT_FLIP[1][(P >> 16) & 0xff];
	n_flipped += COUNT_FLIP[2][(((P & 0x4020100804020408ULL) + 0x406070787c7e7c78ULL) & 0x8080808080808080ULL) * 0x0002040810204081ULL >> 56];

	return n_flipped;
}

/**
 * Count last flipped discs when playing on square C3.
 *
 * @param P player's disc pattern.
 * @return flipped disc count.
 */
static int count_last_flip_C3(const unsigned long long P)
{
	register int n_flipped;

	n_flipped  = COUNT_FLIP[2][((P & 0x0404040404040404ULL) * 0x0040810204081020ULL) >> 56];
	n_flipped += COUNT_FLIP[2][(P >> 16) & 0xff];
	n_flipped += COUNT_FLIP[2][((P & 0x0000000102040810ULL) * 0x0101010101010101ULL) >> 56];
	n_flipped += COUNT_FLIP[2][((P & 0x8040201008040201ULL) * 0x0101010101010101ULL) >> 56];

	return n_flipped;
}

/**
 * Count last flipped discs when playing on square D3.
 *
 * @param P player's disc pattern.
 * @return flipped disc count.
 */
static int count_last_flip_D3(const unsigned long long P)
{
	register int n_flipped;

	n_flipped  = COUNT_FLIP[2][((P & 0x0808080808080808ULL) * 0x0020408102040810ULL) >> 56];
	n_flipped += COUNT_FLIP[3][(P >> 16) & 0xff];
	n_flipped += COUNT_FLIP[3][((P & 0x0000010204081020ULL) * 0x0101010101010101ULL) >> 56];
	n_flipped += COUNT_FLIP[3][((P & 0x0080402010080402ULL) * 0x0101010101010101ULL) >> 56];

	return n_flipped;
}

/**
 * Count last flipped discs when playing on square E3.
 *
 * @param P player's disc pattern.
 * @return flipped disc count.
 */
static int count_last_flip_E3(const unsigned long long P)
{
	register int n_flipped;

	n_flipped  = COUNT_FLIP[2][((P & 0x1010101010101010ULL) * 0x0010204081020408ULL) >> 56];
	n_flipped += COUNT_FLIP[4][(P >> 16) & 0xff];
	n_flipped += COUNT_FLIP[4][((P & 0x0001020408102040ULL) * 0x0101010101010101ULL) >> 56];
	n_flipped += COUNT_FLIP[4][((P & 0x0000804020100804ULL) * 0x0101010101010101ULL) >> 56];

	return n_flipped;
}

/**
 * Count last flipped discs when playing on square F3.
 *
 * @param P player's disc pattern.
 * @return flipped disc count.
 */
static int count_last_flip_F3(const unsigned long long P)
{
	register int n_flipped;

	n_flipped  = COUNT_FLIP[2][((P & 0x2020202020202020ULL) * 0x0008102040810204ULL) >> 56];
	n_flipped += COUNT_FLIP[5][(P >> 16) & 0xff];
	n_flipped += COUNT_FLIP[5][((P & 0x0102040810204080ULL) * 0x0101010101010101ULL) >> 56];
	n_flipped += COUNT_FLIP[5][((P & 0x0000008040201008ULL) * 0x0101010101010101ULL) >> 56];

	return n_flipped;
}

/**
 * Count last flipped discs when playing on square G3.
 *
 * @param P player's disc pattern.
 * @return flipped disc count.
 */
static int count_last_flip_G3(const unsigned long long P)
{
	register int n_flipped;

	n_flipped  = COUNT_FLIP[2][((P & 0x4040404040404040ULL) * 0x0004081020408102ULL) >> 56];
	n_flipped += COUNT_FLIP[6][(P >> 16) & 0xff];
	n_flipped += COUNT_FLIP[2][(((P & 0x0204081020402010ULL) + 0x7e7c787060406070ULL) & 0x8080808080808080ULL) * 0x0002040810204081ULL >> 56];

	return n_flipped;
}

/**
 * Count last flipped discs when playing on square H3.
 *
 * @param P player's disc pattern.
 * @return flipped disc count.
 */
static int count_last_flip_H3(const unsigned long long P)
{
	register int n_flipped;

	n_flipped  = COUNT_FLIP[2][((P & 0x8080808080808080ULL) * 0x0002040810204081ULL) >> 56];
	n_flipped += COUNT_FLIP[7][(P >> 16) & 0xff];
	n_flipped += COUNT_FLIP[2][(((P & 0x0408102040804020ULL) + 0x7c78706040004060ULL) & 0x8080808080808080ULL) * 0x0002040810204081ULL >> 56];

	return n_flipped;
}

/**
 * Count last flipped discs when playing on square A4.
 *
 * @param P player's disc pattern.
 * @return flipped disc count.
 */
static int count_last_flip_A4(const unsigned long long P)
{
	register int n_flipped;

	n_flipped  = COUNT_FLIP[3][((P & 0x0101010101010101ULL) * 0x0102040810204080ULL) >> 56];
	n_flipped += COUNT_FLIP[0][(P >> 24) & 0xff];
	n_flipped += COUNT_FLIP[3][(((P & 0x1008040201020408ULL) + 0x70787c7e7f7e7c78ULL) & 0x8080808080808080ULL) * 0x0002040810204081ULL >> 56];

	return n_flipped;
}

/**
 * Count last flipped discs when playing on square B4.
 *
 * @param P player's disc pattern.
 * @return flipped disc count.
 */
static int count_last_flip_B4(const unsigned long long P)
{
	register int n_flipped;

	n_flipped  = COUNT_FLIP[3][((P & 0x0202020202020202ULL) * 0x0081020408102040ULL) >> 56];
	n_flipped += COUNT_FLIP[1][(P >> 24) & 0xff];
	n_flipped += COUNT_FLIP[3][(((P & 0x2010080402040810ULL) + 0x6070787c7e7c7870ULL) & 0x8080808080808080ULL) * 0x0002040810204081ULL >> 56];

	return n_flipped;
}

/**
 * Count last flipped discs when playing on square C4.
 *
 * @param P player's disc pattern.
 * @return flipped disc count.
 */
static int count_last_flip_C4(const unsigned long long P)
{
	register int n_flipped;

	n_flipped  = COUNT_FLIP[3][((P & 0x0404040404040404ULL) * 0x0040810204081020ULL) >> 56];
	n_flipped += COUNT_FLIP[2][(P >> 24) & 0xff];
	n_flipped += COUNT_FLIP[2][((P & 0x0000010204081020ULL) * 0x0101010101010101ULL) >> 56];
	n_flipped += COUNT_FLIP[2][((P & 0x4020100804020100ULL) * 0x0101010101010101ULL) >> 56];

	return n_flipped;
}

/**
 * Count last flipped discs when playing on square D4.
 *
 * @param P player's disc pattern.
 * @return flipped disc count.
 */
static int count_last_flip_D4(const unsigned long long P)
{
	register int n_flipped;

	n_flipped  = COUNT_FLIP[3][((P & 0x0808080808080808ULL) * 0x0020408102040810ULL) >> 56];
	n_flipped += COUNT_FLIP[3][(P >> 24) & 0xff];
	n_flipped += COUNT_FLIP[3][((P & 0x0001020408102040ULL) * 0x0101010101010101ULL) >> 56];
	n_flipped += COUNT_FLIP[3][((P & 0x8040201008040201ULL) * 0x0101010101010101ULL) >> 56];

	return n_flipped;
}

/**
 * Count last flipped discs when playing on square E4.
 *
 * @param P player's disc pattern.
 * @return flipped disc count.
 */
static int count_last_flip_E4(const unsigned long long P)
{
	register int n_flipped;

	n_flipped  = COUNT_FLIP[3][((P & 0x1010101010101010ULL) * 0x0010204081020408ULL) >> 56];
	n_flipped += COUNT_FLIP[4][(P >> 24) & 0xff];
	n_flipped += COUNT_FLIP[4][((P & 0x0102040810204080ULL) * 0x0101010101010101ULL) >> 56];
	n_flipped += COUNT_FLIP[4][((P & 0x0080402010080402ULL) * 0x0101010101010101ULL) >> 56];

	return n_flipped;
}

/**
 * Count last flipped discs when playing on square F4.
 *
 * @param P player's disc pattern.
 * @return flipped disc count.
 */
static int count_last_flip_F4(const unsigned long long P)
{
	register int n_flipped;

	n_flipped  = COUNT_FLIP[3][((P & 0x2020202020202020ULL) * 0x0008102040810204ULL) >> 56];
	n_flipped += COUNT_FLIP[5][(P >> 24) & 0xff];
	n_flipped += COUNT_FLIP[5][((P & 0x0204081020408000ULL) * 0x0101010101010101ULL) >> 56];
	n_flipped += COUNT_FLIP[5][((P & 0x0000804020100804ULL) * 0x0101010101010101ULL) >> 56];

	return n_flipped;
}

/**
 * Count last flipped discs when playing on square G4.
 *
 * @param P player's disc pattern.
 * @return flipped disc count.
 */
static int count_last_flip_G4(const unsigned long long P)
{
	register int n_flipped;

	n_flipped  = COUNT_FLIP[3][((P & 0x4040404040404040ULL) * 0x0004081020408102ULL) >> 56];
	n_flipped += COUNT_FLIP[6][(P >> 24) & 0xff];
	n_flipped += COUNT_FLIP[3][(((P & 0x0408102040201008ULL) + 0x7c78706040607078ULL) & 0x8080808080808080ULL) * 0x0002040810204081ULL >> 56];

	return n_flipped;
}

/**
 * Count last flipped discs when playing on square H4.
 *
 * @param P player's disc pattern.
 * @return flipped disc count.
 */
static int count_last_flip_H4(const unsigned long long P)
{
	register int n_flipped;

	n_flipped  = COUNT_FLIP[3][((P & 0x8080808080808080ULL) * 0x0002040810204081ULL) >> 56];
	n_flipped += COUNT_FLIP[7][(P >> 24) & 0xff];
	n_flipped += COUNT_FLIP[3][(((P & 0x0810204080402010ULL) + 0x7870604000406070ULL) & 0x8080808080808080ULL) * 0x0002040810204081ULL >> 56];

	return n_flipped;
}

/**
 * Count last flipped discs when playing on square A5.
 *
 * @param P player's disc pattern.
 * @return flipped disc count.
 */
static int count_last_flip_A5(const unsigned long long P)
{
	register int n_flipped;

	n_flipped  = COUNT_FLIP[4][((P & 0x0101010101010101ULL) * 0x0102040810204080ULL) >> 56];
	n_flipped += COUNT_FLIP[0][(P >> 32) & 0xff];
	n_flipped += COUNT_FLIP[4][(((P & 0x0804020102040810ULL) + 0x787c7e7f7e7c7870ULL) & 0x8080808080808080ULL) * 0x0002040810204081ULL >> 56];

	return n_flipped;
}

/**
 * Count last flipped discs when playing on square B5.
 *
 * @param P player's disc pattern.
 * @return flipped disc count.
 */
static int count_last_flip_B5(const unsigned long long P)
{
	register int n_flipped;

	n_flipped  = COUNT_FLIP[4][((P & 0x0202020202020202ULL) * 0x0081020408102040ULL) >> 56];
	n_flipped += COUNT_FLIP[1][(P >> 32) & 0xff];
	n_flipped += COUNT_FLIP[4][(((P & 0x1008040204081020ULL) + 0x70787c7e7c787060ULL) & 0x8080808080808080ULL) * 0x0002040810204081ULL >> 56];

	return n_flipped;
}

/**
 * Count last flipped discs when playing on square C5.
 *
 * @param P player's disc pattern.
 * @return flipped disc count.
 */
static int count_last_flip_C5(const unsigned long long P)
{
	register int n_flipped;

	n_flipped  = COUNT_FLIP[4][((P & 0x0404040404040404ULL) * 0x0040810204081020ULL) >> 56];
	n_flipped += COUNT_FLIP[2][(P >> 32) & 0xff];
	n_flipped += COUNT_FLIP[2][((P & 0x0001020408102040ULL) * 0x0101010101010101ULL) >> 56];
	n_flipped += COUNT_FLIP[2][((P & 0x2010080402010000ULL) * 0x0101010101010101ULL) >> 56];

	return n_flipped;
}

/**
 * Count last flipped discs when playing on square D5.
 *
 * @param P player's disc pattern.
 * @return flipped disc count.
 */
static int count_last_flip_D5(const unsigned long long P)
{
	register int n_flipped;

	n_flipped  = COUNT_FLIP[4][((P & 0x0808080808080808ULL) * 0x0020408102040810ULL) >> 56];
	n_flipped += COUNT_FLIP[3][(P >> 32) & 0xff];
	n_flipped += COUNT_FLIP[3][((P & 0x0102040810204080ULL) * 0x0101010101010101ULL) >> 56];
	n_flipped += COUNT_FLIP[3][((P & 0x4020100804020100ULL) * 0x0101010101010101ULL) >> 56];

	return n_flipped;
}

/**
 * Count last flipped discs when playing on square E5.
 *
 * @param P player's disc pattern.
 * @return flipped disc count.
 */
static int count_last_flip_E5(const unsigned long long P)
{
	register int n_flipped;

	n_flipped  = COUNT_FLIP[4][((P & 0x1010101010101010ULL) * 0x0010204081020408ULL) >> 56];
	n_flipped += COUNT_FLIP[4][(P >> 32) & 0xff];
	n_flipped += COUNT_FLIP[4][((P & 0x0204081020408000ULL) * 0x0101010101010101ULL) >> 56];
	n_flipped += COUNT_FLIP[4][((P & 0x8040201008040201ULL) * 0x0101010101010101ULL) >> 56];

	return n_flipped;
}

/**
 * Count last flipped discs when playing on square F5.
 *
 * @param P player's disc pattern.
 * @return flipped disc count.
 */
static int count_last_flip_F5(const unsigned long long P)
{
	register int n_flipped;

	n_flipped  = COUNT_FLIP[4][((P & 0x2020202020202020ULL) * 0x0008102040810204ULL) >> 56];
	n_flipped += COUNT_FLIP[5][(P >> 32) & 0xff];
	n_flipped += COUNT_FLIP[5][((P & 0x0408102040800000ULL) * 0x0101010101010101ULL) >> 56];
	n_flipped += COUNT_FLIP[5][((P & 0x0080402010080402ULL) * 0x0101010101010101ULL) >> 56];

	return n_flipped;
}

/**
 * Count last flipped discs when playing on square G5.
 *
 * @param P player's disc pattern.
 * @return flipped disc count.
 */
static int count_last_flip_G5(const unsigned long long P)
{
	register int n_flipped;

	n_flipped  = COUNT_FLIP[4][((P & 0x4040404040404040ULL) * 0x0004081020408102ULL) >> 56];
	n_flipped += COUNT_FLIP[6][(P >> 32) & 0xff];
	n_flipped += COUNT_FLIP[4][(((P & 0x0810204020100804ULL) + 0x787060406070787cULL) & 0x8080808080808080ULL) * 0x0002040810204081ULL >> 56];

	return n_flipped;
}

/**
 * Count last flipped discs when playing on square H5.
 *
 * @param P player's disc pattern.
 * @return flipped disc count.
 */
static int count_last_flip_H5(const unsigned long long P)
{
	register int n_flipped;

	n_flipped  = COUNT_FLIP[4][((P & 0x8080808080808080ULL) * 0x0002040810204081ULL) >> 56];
	n_flipped += COUNT_FLIP[7][(P >> 32) & 0xff];
	n_flipped += COUNT_FLIP[4][(((P & 0x1020408040201008ULL) + 0x7060400040607078ULL) & 0x8080808080808080ULL) * 0x0002040810204081ULL >> 56];

	return n_flipped;
}

/**
 * Count last flipped discs when playing on square A6.
 *
 * @param P player's disc pattern.
 * @return flipped disc count.
 */
static int count_last_flip_A6(const unsigned long long P)
{
	register int n_flipped;

	n_flipped  = COUNT_FLIP[5][((P & 0x0101010101010101ULL) * 0x0102040810204080ULL) >> 56];
	n_flipped += COUNT_FLIP[0][(P >> 40) & 0xff];
	n_flipped += COUNT_FLIP[5][(((P & 0x0402010204081020ULL) + 0x7c7e7f7e7c787060ULL) & 0x8080808080808080ULL) * 0x0002040810204081ULL >> 56];

	return n_flipped;
}

/**
 * Count last flipped discs when playing on square B6.
 *
 * @param P player's disc pattern.
 * @return flipped disc count.
 */
static int count_last_flip_B6(const unsigned long long P)
{
	register int n_flipped;

	n_flipped  = COUNT_FLIP[5][((P & 0x0202020202020202ULL) * 0x0081020408102040ULL) >> 56];
	n_flipped += COUNT_FLIP[1][(P >> 40) & 0xff];
	n_flipped += COUNT_FLIP[5][(((P & 0x0804020408102040ULL) + 0x787c7e7c78706040ULL) & 0x8080808080808080ULL) * 0x0002040810204081ULL >> 56];

	return n_flipped;
}

/**
 * Count last flipped discs when playing on square C6.
 *
 * @param P player's disc pattern.
 * @return flipped disc count.
 */
static int count_last_flip_C6(const unsigned long long P)
{
	register int n_flipped;

	n_flipped  = COUNT_FLIP[5][((P & 0x0404040404040404ULL) * 0x0040810204081020ULL) >> 56];
	n_flipped += COUNT_FLIP[2][(P >> 40) & 0xff];
	n_flipped += COUNT_FLIP[2][((P & 0x0102040810204080ULL) * 0x0101010101010101ULL) >> 56];
	n_flipped += COUNT_FLIP[2][((P & 0x1008040201000000ULL) * 0x0101010101010101ULL) >> 56];

	return n_flipped;
}

/**
 * Count last flipped discs when playing on square D6.
 *
 * @param P player's disc pattern.
 * @return flipped disc count.
 */
static int count_last_flip_D6(const unsigned long long P)
{
	register int n_flipped;

	n_flipped  = COUNT_FLIP[5][((P & 0x0808080808080808ULL) * 0x0020408102040810ULL) >> 56];
	n_flipped += COUNT_FLIP[3][(P >> 40) & 0xff];
	n_flipped += COUNT_FLIP[3][((P & 0x0204081020408000ULL) * 0x0101010101010101ULL) >> 56];
	n_flipped += COUNT_FLIP[3][((P & 0x2010080402010000ULL) * 0x0101010101010101ULL) >> 56];

	return n_flipped;
}

/**
 * Count last flipped discs when playing on square E6.
 *
 * @param P player's disc pattern.
 * @return flipped disc count.
 */
static int count_last_flip_E6(const unsigned long long P)
{
	register int n_flipped;

	n_flipped  = COUNT_FLIP[5][((P & 0x1010101010101010ULL) * 0x0010204081020408ULL) >> 56];
	n_flipped += COUNT_FLIP[4][(P >> 40) & 0xff];
	n_flipped += COUNT_FLIP[4][((P & 0x0408102040800000ULL) * 0x0101010101010101ULL) >> 56];
	n_flipped += COUNT_FLIP[4][((P & 0x4020100804020100ULL) * 0x0101010101010101ULL) >> 56];

	return n_flipped;
}

/**
 * Count last flipped discs when playing on square F6.
 *
 * @param P player's disc pattern.
 * @return flipped disc count.
 */
static int count_last_flip_F6(const unsigned long long P)
{
	register int n_flipped;

	n_flipped  = COUNT_FLIP[5][((P & 0x2020202020202020ULL) * 0x0008102040810204ULL) >> 56];
	n_flipped += COUNT_FLIP[5][(P >> 40) & 0xff];
	n_flipped += COUNT_FLIP[5][((P & 0x0810204080000000ULL) * 0x0101010101010101ULL) >> 56];
	n_flipped += COUNT_FLIP[5][((P & 0x8040201008040201ULL) * 0x0101010101010101ULL) >> 56];

	return n_flipped;
}

/**
 * Count last flipped discs when playing on square G6.
 *
 * @param P player's disc pattern.
 * @return flipped disc count.
 */
static int count_last_flip_G6(const unsigned long long P)
{
	register int n_flipped;

	n_flipped  = COUNT_FLIP[5][((P & 0x4040404040404040ULL) * 0x0004081020408102ULL) >> 56];
	n_flipped += COUNT_FLIP[6][(P >> 40) & 0xff];
	n_flipped += COUNT_FLIP[5][(((P & 0x1020402010080402ULL) + 0x7060406070787c7eULL) & 0x8080808080808080ULL) * 0x0002040810204081ULL >> 56];

	return n_flipped;
}

/**
 * Count last flipped discs when playing on square H6.
 *
 * @param P player's disc pattern.
 * @return flipped disc count.
 */
static int count_last_flip_H6(const unsigned long long P)
{
	register int n_flipped;

	n_flipped  = COUNT_FLIP[5][((P & 0x8080808080808080ULL) * 0x0002040810204081ULL) >> 56];
	n_flipped += COUNT_FLIP[7][(P >> 40) & 0xff];
	n_flipped += COUNT_FLIP[5][(((P & 0x2040804020100804ULL) + 0x604000406070787cULL) & 0x8080808080808080ULL) * 0x0002040810204081ULL >> 56];

	return n_flipped;
}

/**
 * Count last flipped discs when playing on square A7.
 *
 * @param P player's disc pattern.
 * @return flipped disc count.
 */
static int count_last_flip_A7(const unsigned long long P)
{
	register int n_flipped;

	n_flipped  = COUNT_FLIP[6][((P & 0x0101010101010101ULL) * 0x0102040810204080ULL) >> 56];
	n_flipped += COUNT_FLIP[0][(P >> 48) & 0xff];
	n_flipped += COUNT_FLIP[0][((P & 0x0001020408102040ULL) * 0x0101010101010101ULL) >> 56];

	return n_flipped;
}

/**
 * Count last flipped discs when playing on square B7.
 *
 * @param P player's disc pattern.
 * @return flipped disc count.
 */
static int count_last_flip_B7(const unsigned long long P)
{
	register int n_flipped;

	n_flipped  = COUNT_FLIP[6][((P & 0x0202020202020202ULL) * 0x0081020408102040ULL) >> 56];
	n_flipped += COUNT_FLIP[1][(P >> 48) & 0xff];
	n_flipped += COUNT_FLIP[1][((P & 0x0102040810204080ULL) * 0x0101010101010101ULL) >> 56];

	return n_flipped;
}

/**
 * Count last flipped discs when playing on square C7.
 *
 * @param P player's disc pattern.
 * @return flipped disc count.
 */
static int count_last_flip_C7(const unsigned long long P)
{
	register int n_flipped;

	n_flipped  = COUNT_FLIP[6][((P & 0x0404040404040404ULL) * 0x0040810204081020ULL) >> 56];
	n_flipped += COUNT_FLIP[2][(P >> 48) & 0xff];
	n_flipped += COUNT_FLIP[2][(P & 0x00040a1120408000ULL) * 0x0101010101010101ULL >> 56];

	return n_flipped;
}

/**
 * Count last flipped discs when playing on square D7.
 *
 * @param P player's disc pattern.
 * @return flipped disc count.
 */
static int count_last_flip_D7(const unsigned long long P)
{
	register int n_flipped;

	n_flipped  = COUNT_FLIP[6][((P & 0x0808080808080808ULL) * 0x0020408102040810ULL) >> 56];
	n_flipped += COUNT_FLIP[3][(P >> 48) & 0xff];
	n_flipped += COUNT_FLIP[3][(P & 0x0008142241800000ULL) * 0x0101010101010101ULL >> 56];

	return n_flipped;
}

/**
 * Count last flipped discs when playing on square E7.
 *
 * @param P player's disc pattern.
 * @return flipped disc count.
 */
static int count_last_flip_E7(const unsigned long long P)
{
	register int n_flipped;

	n_flipped  = COUNT_FLIP[6][((P & 0x1010101010101010ULL) * 0x0010204081020408ULL) >> 56];
	n_flipped += COUNT_FLIP[4][(P >> 48) & 0xff];
	n_flipped += COUNT_FLIP[4][(P & 0x0010284482010000ULL) * 0x0101010101010101ULL >> 56];

	return n_flipped;
}

/**
 * Count last flipped discs when playing on square F7.
 *
 * @param P player's disc pattern.
 * @return flipped disc count.
 */
static int count_last_flip_F7(const unsigned long long P)
{
	register int n_flipped;

	n_flipped  = COUNT_FLIP[6][((P & 0x2020202020202020ULL) * 0x0008102040810204ULL) >> 56];
	n_flipped += COUNT_FLIP[5][(P >> 48) & 0xff];
	n_flipped += COUNT_FLIP[5][(P & 0x0020508804020100ULL) * 0x0101010101010101ULL >> 56];

	return n_flipped;
}

/**
 * Count last flipped discs when playing on square G7.
 *
 * @param P player's disc pattern.
 * @return flipped disc count.
 */
static int count_last_flip_G7(const unsigned long long P)
{
	register int n_flipped;

	n_flipped  = COUNT_FLIP[6][((P & 0x4040404040404040ULL) * 0x0004081020408102ULL) >> 56];
	n_flipped += COUNT_FLIP[6][(P >> 48) & 0xff];
	n_flipped += COUNT_FLIP[6][((P & 0x8040201008040201ULL) * 0x0101010101010101ULL) >> 56];

	return n_flipped;
}

/**
 * Count last flipped discs when playing on square H7.
 *
 * @param P player's disc pattern.
 * @return flipped disc count.
 */
static int count_last_flip_H7(const unsigned long long P)
{
	register int n_flipped;

	n_flipped  = COUNT_FLIP[6][((P & 0x8080808080808080ULL) * 0x0002040810204081ULL) >> 56];
	n_flipped += COUNT_FLIP[7][(P >> 48) & 0xff];
	n_flipped += COUNT_FLIP[7][((P & 0x0080402010080402ULL) * 0x0101010101010101ULL) >> 56];

	return n_flipped;
}

/**
 * Count last flipped discs when playing on square A8.
 *
 * @param P player's disc pattern.
 * @return flipped disc count.
 */
static int count_last_flip_A8(const unsigned long long P)
{
	register int n_flipped;

	n_flipped  = COUNT_FLIP[7][((P & 0x0101010101010101ULL) * 0x0102040810204080ULL) >> 56];
	n_flipped += COUNT_FLIP[0][P >> 56];
	n_flipped += COUNT_FLIP[0][((P & 0x0102040810204080ULL) * 0x0101010101010101ULL) >> 56];

	return n_flipped;
}

/**
 * Count last flipped discs when playing on square B8.
 *
 * @param P player's disc pattern.
 * @return flipped disc count.
 */
static int count_last_flip_B8(const unsigned long long P)
{
	register int n_flipped;

	n_flipped  = COUNT_FLIP[7][((P & 0x0202020202020202ULL) * 0x0081020408102040ULL) >> 56];
	n_flipped += COUNT_FLIP[1][P >> 56];
	n_flipped += COUNT_FLIP[1][((P & 0x0204081020408000ULL) * 0x0101010101010101ULL) >> 56];

	return n_flipped;
}

/**
 * Count last flipped discs when playing on square C8.
 *
 * @param P player's disc pattern.
 * @return flipped disc count.
 */
static int count_last_flip_C8(const unsigned long long P)
{
	register int n_flipped;

	n_flipped  = COUNT_FLIP[7][((P & 0x0404040404040404ULL) * 0x0040810204081020ULL) >> 56];
	n_flipped += COUNT_FLIP[2][P >> 56];
	n_flipped += COUNT_FLIP[2][(P & 0x040a112040800000ULL) * 0x0101010101010101ULL >> 56];

	return n_flipped;
}

/**
 * Count last flipped discs when playing on square D8.
 *
 * @param P player's disc pattern.
 * @return flipped disc count.
 */
static int count_last_flip_D8(const unsigned long long P)
{
	register int n_flipped;

	n_flipped  = COUNT_FLIP[7][((P & 0x0808080808080808ULL) * 0x0020408102040810ULL) >> 56];
	n_flipped += COUNT_FLIP[3][P >> 56];
	n_flipped += COUNT_FLIP[3][(P & 0x0814224180000000ULL) * 0x0101010101010101ULL >> 56];

	return n_flipped;
}

/**
 * Count last flipped discs when playing on square E8.
 *
 * @param P player's disc pattern.
 * @return flipped disc count.
 */
static int count_last_flip_E8(const unsigned long long P)
{
	register int n_flipped;

	n_flipped  = COUNT_FLIP[7][((P & 0x1010101010101010ULL) * 0x0010204081020408ULL) >> 56];
	n_flipped += COUNT_FLIP[4][P >> 56];
	n_flipped += COUNT_FLIP[4][(P & 0x1028448201000000ULL) * 0x0101010101010101ULL >> 56];

	return n_flipped;
}

/**
 * Count last flipped discs when playing on square F8.
 *
 * @param P player's disc pattern.
 * @return flipped disc count.
 */
static int count_last_flip_F8(const unsigned long long P)
{
	register int n_flipped;

	n_flipped  = COUNT_FLIP[7][((P & 0x2020202020202020ULL) * 0x0008102040810204ULL) >> 56];
	n_flipped += COUNT_FLIP[5][P >> 56];
	n_flipped += COUNT_FLIP[5][(P & 0x2050880402010000ULL) * 0x0101010101010101ULL >> 56];

	return n_flipped;
}

/**
 * Count last flipped discs when playing on square G8.
 *
 * @param P player's disc pattern.
 * @return flipped disc count.
 */
static int count_last_flip_G8(const unsigned long long P)
{
	register int n_flipped;

	n_flipped  = COUNT_FLIP[7][((P & 0x4040404040404040ULL) * 0x0004081020408102ULL) >> 56];
	n_flipped += COUNT_FLIP[6][P >> 56];
	n_flipped += COUNT_FLIP[6][((P & 0x4020100804020100ULL) * 0x0101010101010101ULL) >> 56];

	return n_flipped;
}

/**
 * Count last flipped discs when playing on square H8.
 *
 * @param P player's disc pattern.
 * @return flipped disc count.
 */
static int count_last_flip_H8(const unsigned long long P)
{
	register int n_flipped;

	n_flipped  = COUNT_FLIP[7][((P & 0x8080808080808080ULL) * 0x0002040810204081ULL) >> 56];
	n_flipped += COUNT_FLIP[7][P >> 56];
	n_flipped += COUNT_FLIP[7][((P & 0x8040201008040201ULL) * 0x0101010101010101ULL) >> 56];

	return n_flipped;
}

/**
 * Count last flipped discs when plassing.
 *
 * @param P player's disc pattern (unused).
 * @return zero.
 */
static int count_last_flip_pass(const unsigned long long P)
{
	(void) P; // useless code to shut-up compiler warning
	return 0;
}

/** Array of functions to count flipped discs of the last move */
int (*count_last_flip[])(const unsigned long long) = {
	count_last_flip_A1, count_last_flip_B1, count_last_flip_C1, count_last_flip_D1,
	count_last_flip_E1, count_last_flip_F1, count_last_flip_G1, count_last_flip_H1,
	count_last_flip_A2, count_last_flip_B2, count_last_flip_C2, count_last_flip_D2,
	count_last_flip_E2, count_last_flip_F2, count_last_flip_G2, count_last_flip_H2,
	count_last_flip_A3, count_last_flip_B3, count_last_flip_C3, count_last_flip_D3,
	count_last_flip_E3, count_last_flip_F3, count_last_flip_G3, count_last_flip_H3,
	count_last_flip_A4, count_last_flip_B4, count_last_flip_C4, count_last_flip_D4,
	count_last_flip_E4, count_last_flip_F4, count_last_flip_G4, count_last_flip_H4,
	count_last_flip_A5, count_last_flip_B5, count_last_flip_C5, count_last_flip_D5,
	count_last_flip_E5, count_last_flip_F5, count_last_flip_G5, count_last_flip_H5,
	count_last_flip_A6, count_last_flip_B6, count_last_flip_C6, count_last_flip_D6,
	count_last_flip_E6, count_last_flip_F6, count_last_flip_G6, count_last_flip_H6,
	count_last_flip_A7, count_last_flip_B7, count_last_flip_C7, count_last_flip_D7,
	count_last_flip_E7, count_last_flip_F7, count_last_flip_G7, count_last_flip_H7,
	count_last_flip_A8, count_last_flip_B8, count_last_flip_C8, count_last_flip_D8,
	count_last_flip_E8, count_last_flip_F8, count_last_flip_G8, count_last_flip_H8,
	count_last_flip_pass,
};

