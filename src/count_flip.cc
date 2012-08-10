/**
 * @file count_last_flip.c
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
static int count_flip_A1(const unsigned long long P) {
	register int n_flipped;

	n_flipped  = COUNT_FLIP[0][((P & 0x0101010101010101ULL) * 0x0102040810204080ULL) >> 56];
	n_flipped += COUNT_FLIP[0][P & 0xff];
	n_flipped += COUNT_FLIP[0][((P & 0x8040201008040201ULL) * 0x0101010101010101ULL) >> 56];

	return n_flipped;
}

/**
 * Count last flipped discs when playing on square A1.
 *
 * @param P player's disc pattern.
 * @return flipped disc count.
 */
static int count_flip_B1(const unsigned long long P) {
	register int n_flipped;

	n_flipped  = COUNT_FLIP[0][((P & 0x0202020202020202ULL) * 0x0081020408102040ULL) >> 56];
	n_flipped += COUNT_FLIP[1][P & 0xff];
	n_flipped += COUNT_FLIP[1][((P & 0x0080402010080402ULL) * 0x0101010101010101ULL) >> 56];

	return n_flipped;
}

/**
 * Count last flipped discs when playing on square A1.
 *
 * @param P player's disc pattern.
 * @return flipped disc count.
 */
static int count_flip_C1(const unsigned long long P) {
	register int n_flipped;

	n_flipped  = COUNT_FLIP[0][((P & 0x0404040404040404ULL) * 0x0040810204081020ULL) >> 56];
	n_flipped += COUNT_FLIP[2][P & 0xff];
	n_flipped += ((P & 0x0000000000010200ULL) == 0x0000000000010000ULL);
	n_flipped += COUNT_FLIP[2][((P & 0x0000804020100804ULL) * 0x0101010101010101ULL) >> 56];

	return n_flipped;
}

/**
 * Count last flipped discs when playing on square A1.
 *
 * @param P player's disc pattern.
 * @return flipped disc count.
 */
static int count_flip_D1(const unsigned long long P) {
	register int n_flipped;

	n_flipped  = COUNT_FLIP[0][((P & 0x0808080808080808ULL) * 0x0020408102040810ULL) >> 56];
	n_flipped += COUNT_FLIP[3][P & 0xff];
	n_flipped += COUNT_FLIP[3][((P & 0x0000000001020408ULL) * 0x0101010101010101ULL) >> 56];
	n_flipped += COUNT_FLIP[3][((P & 0x0000008040201008ULL) * 0x0101010101010101ULL) >> 56];

	return n_flipped;
}

/**
 * Count last flipped discs when playing on square A1.
 *
 * @param P player's disc pattern.
 * @return flipped disc count.
 */
static int count_flip_E1(const unsigned long long P) {
	register int n_flipped;

	n_flipped  = COUNT_FLIP[0][((P & 0x1010101010101010ULL) * 0x0010204081020408ULL) >> 56];
	n_flipped += COUNT_FLIP[4][P & 0xff];
	n_flipped += COUNT_FLIP[4][((P & 0x0000000102040810ULL) * 0x0101010101010101ULL) >> 56];
	n_flipped += COUNT_FLIP[4][((P & 0x0000000080402010ULL) * 0x0101010101010101ULL) >> 56];

	return n_flipped;
}

/**
 * Count last flipped discs when playing on square A1.
 *
 * @param P player's disc pattern.
 * @return flipped disc count.
 */
static int count_flip_F1(const unsigned long long P) {
	register int n_flipped;

	n_flipped  = COUNT_FLIP[0][((P & 0x2020202020202020ULL) * 0x0008102040810204ULL) >> 56];
	n_flipped += COUNT_FLIP[5][P & 0xff];
	n_flipped += COUNT_FLIP[5][((P & 0x0000010204081020ULL) * 0x0101010101010101ULL) >> 56];
	n_flipped += ((P & 0x0000000000804000ULL) == 0x0000000000800000ULL);

	return n_flipped;
}

/**
 * Count last flipped discs when playing on square A1.
 *
 * @param P player's disc pattern.
 * @return flipped disc count.
 */
static int count_flip_G1(const unsigned long long P) {
	register int n_flipped;

	n_flipped  = COUNT_FLIP[0][((P & 0x4040404040404040ULL) * 0x0004081020408102ULL) >> 56];
	n_flipped += COUNT_FLIP[6][P & 0xff];
	n_flipped += COUNT_FLIP[6][((P & 0x0001020408102040ULL) * 0x0101010101010101ULL) >> 56];

	return n_flipped;
}

/**
 * Count last flipped discs when playing on square A1.
 *
 * @param P player's disc pattern.
 * @return flipped disc count.
 */
static int count_flip_H1(const unsigned long long P) {
	register int n_flipped;

	n_flipped  = COUNT_FLIP[0][((P & 0x8080808080808080ULL) * 0x0002040810204081ULL) >> 56];
	n_flipped += COUNT_FLIP[7][P & 0xff];
	n_flipped += COUNT_FLIP[7][((P & 0x0102040810204080ULL) * 0x0101010101010101ULL) >> 56];

	return n_flipped;
}

/**
 * Count last flipped discs when playing on square A1.
 *
 * @param P player's disc pattern.
 * @return flipped disc count.
 */
static int count_flip_A2(const unsigned long long P) {
	register int n_flipped;

	n_flipped  = COUNT_FLIP[1][((P & 0x0101010101010101ULL) * 0x0102040810204080ULL) >> 56];
	n_flipped += COUNT_FLIP[0][(P >> 8) & 0xff];
	n_flipped += COUNT_FLIP[0][((P & 0x4020100804020100ULL) * 0x0101010101010101ULL) >> 56];

	return n_flipped;
}

/**
 * Count last flipped discs when playing on square A1.
 *
 * @param P player's disc pattern.
 * @return flipped disc count.
 */
static int count_flip_B2(const unsigned long long P) {
	register int n_flipped;

	n_flipped  = COUNT_FLIP[1][((P & 0x0202020202020202ULL) * 0x0081020408102040ULL) >> 56];
	n_flipped += COUNT_FLIP[1][(P >> 8) & 0xff];
	n_flipped += COUNT_FLIP[1][((P & 0x8040201008040201ULL) * 0x0101010101010101ULL) >> 56];

	return n_flipped;
}

/**
 * Count last flipped discs when playing on square A1.
 *
 * @param P player's disc pattern.
 * @return flipped disc count.
 */
static int count_flip_C2(const unsigned long long P) {
	register int n_flipped;

	n_flipped  = COUNT_FLIP[1][((P & 0x0404040404040404ULL) * 0x0040810204081020ULL) >> 56];
	n_flipped += COUNT_FLIP[2][(P >> 8) & 0xff];
	n_flipped += ((P & 0x0000000001020000ULL) == 0x0000000001000000ULL);
	n_flipped += COUNT_FLIP[2][((P & 0x0080402010080402ULL) * 0x0101010101010101ULL) >> 56];

	return n_flipped;
}

/**
 * Count last flipped discs when playing on square A1.
 *
 * @param P player's disc pattern.
 * @return flipped disc count.
 */
static int count_flip_D2(const unsigned long long P) {
	register int n_flipped;

	n_flipped  = COUNT_FLIP[1][((P & 0x0808080808080808ULL) * 0x0020408102040810ULL) >> 56];
	n_flipped += COUNT_FLIP[3][(P >> 8) & 0xff];
	n_flipped += COUNT_FLIP[3][((P & 0x0000000102040810ULL) * 0x0101010101010101ULL) >> 56];
	n_flipped += COUNT_FLIP[3][((P & 0x0000804020100804ULL) * 0x0101010101010101ULL) >> 56];

	return n_flipped;
}

/**
 * Count last flipped discs when playing on square A1.
 *
 * @param P player's disc pattern.
 * @return flipped disc count.
 */
static int count_flip_E2(const unsigned long long P) {
	register int n_flipped;

	n_flipped  = COUNT_FLIP[1][((P & 0x1010101010101010ULL) * 0x0010204081020408ULL) >> 56];
	n_flipped += COUNT_FLIP[4][(P >> 8) & 0xff];
	n_flipped += COUNT_FLIP[4][((P & 0x0000010204081020ULL) * 0x0101010101010101ULL) >> 56];
	n_flipped += COUNT_FLIP[4][((P & 0x0000008040201008ULL) * 0x0101010101010101ULL) >> 56];

	return n_flipped;
}

/**
 * Count last flipped discs when playing on square A1.
 *
 * @param P player's disc pattern.
 * @return flipped disc count.
 */
static int count_flip_F2(const unsigned long long P) {
	register int n_flipped;

	n_flipped  = COUNT_FLIP[1][((P & 0x2020202020202020ULL) * 0x0008102040810204ULL) >> 56];
	n_flipped += COUNT_FLIP[5][(P >> 8) & 0xff];
	n_flipped += COUNT_FLIP[5][((P & 0x0001020408102040ULL) * 0x0101010101010101ULL) >> 56];
	n_flipped += ((P & 0x0000000080400000ULL) == 0x0000000080000000ULL);

	return n_flipped;
}

/**
 * Count last flipped discs when playing on square A1.
 *
 * @param P player's disc pattern.
 * @return flipped disc count.
 */
static int count_flip_G2(const unsigned long long P) {
	register int n_flipped;

	n_flipped  = COUNT_FLIP[1][((P & 0x4040404040404040ULL) * 0x0004081020408102ULL) >> 56];
	n_flipped += COUNT_FLIP[6][(P >> 8) & 0xff];
	n_flipped += COUNT_FLIP[6][((P & 0x0102040810204080ULL) * 0x0101010101010101ULL) >> 56];

	return n_flipped;
}

/**
 * Count last flipped discs when playing on square A1.
 *
 * @param P player's disc pattern.
 * @return flipped disc count.
 */
static int count_flip_H2(const unsigned long long P) {
	register int n_flipped;

	n_flipped  = COUNT_FLIP[1][((P & 0x8080808080808080ULL) * 0x0002040810204081ULL) >> 56];
	n_flipped += COUNT_FLIP[7][(P >> 8) & 0xff];
	n_flipped += COUNT_FLIP[7][((P & 0x0204081020408000ULL) * 0x0101010101010101ULL) >> 56];

	return n_flipped;
}

/**
 * Count last flipped discs when playing on square A1.
 *
 * @param P player's disc pattern.
 * @return flipped disc count.
 */
static int count_flip_A3(const unsigned long long P) {
	register int n_flipped;

	n_flipped  = COUNT_FLIP[2][((P & 0x0101010101010101ULL) * 0x0102040810204080ULL) >> 56];
	n_flipped += COUNT_FLIP[0][(P >> 16) & 0xff];
	n_flipped += ((P & 0x0000000000000204ULL) == 0x0000000000000004ULL);
	n_flipped += COUNT_FLIP[0][((P & 0x2010080402010000ULL) * 0x0101010101010101ULL) >> 56];

	return n_flipped;
}

/**
 * Count last flipped discs when playing on square A1.
 *
 * @param P player's disc pattern.
 * @return flipped disc count.
 */
static int count_flip_B3(const unsigned long long P) {
	register int n_flipped;

	n_flipped  = COUNT_FLIP[2][((P & 0x0202020202020202ULL) * 0x0081020408102040ULL) >> 56];
	n_flipped += COUNT_FLIP[1][(P >> 16) & 0xff];
	n_flipped += ((P & 0x0000000000000408ULL) == 0x0000000000000008ULL);
	n_flipped += COUNT_FLIP[1][((P & 0x4020100804020100ULL) * 0x0101010101010101ULL) >> 56];

	return n_flipped;
}

/**
 * Count last flipped discs when playing on square A1.
 *
 * @param P player's disc pattern.
 * @return flipped disc count.
 */
static int count_flip_C3(const unsigned long long P) {
	register int n_flipped;

	n_flipped  = COUNT_FLIP[2][((P & 0x0404040404040404ULL) * 0x0040810204081020ULL) >> 56];
	n_flipped += COUNT_FLIP[2][(P >> 16) & 0xff];
	n_flipped += COUNT_FLIP[2][((P & 0x0000000102040810ULL) * 0x0101010101010101ULL) >> 56];
	n_flipped += COUNT_FLIP[2][((P & 0x8040201008040201ULL) * 0x0101010101010101ULL) >> 56];

	return n_flipped;
}

/**
 * Count last flipped discs when playing on square A1.
 *
 * @param P player's disc pattern.
 * @return flipped disc count.
 */
static int count_flip_D3(const unsigned long long P) {
	register int n_flipped;

	n_flipped  = COUNT_FLIP[2][((P & 0x0808080808080808ULL) * 0x0020408102040810ULL) >> 56];
	n_flipped += COUNT_FLIP[3][(P >> 16) & 0xff];
	n_flipped += COUNT_FLIP[3][((P & 0x0000010204081020ULL) * 0x0101010101010101ULL) >> 56];
	n_flipped += COUNT_FLIP[3][((P & 0x0080402010080402ULL) * 0x0101010101010101ULL) >> 56];

	return n_flipped;
}

/**
 * Count last flipped discs when playing on square A1.
 *
 * @param P player's disc pattern.
 * @return flipped disc count.
 */
static int count_flip_E3(const unsigned long long P) {
	register int n_flipped;

	n_flipped  = COUNT_FLIP[2][((P & 0x1010101010101010ULL) * 0x0010204081020408ULL) >> 56];
	n_flipped += COUNT_FLIP[4][(P >> 16) & 0xff];
	n_flipped += COUNT_FLIP[4][((P & 0x0001020408102040ULL) * 0x0101010101010101ULL) >> 56];
	n_flipped += COUNT_FLIP[4][((P & 0x0000804020100804ULL) * 0x0101010101010101ULL) >> 56];

	return n_flipped;
}

/**
 * Count last flipped discs when playing on square A1.
 *
 * @param P player's disc pattern.
 * @return flipped disc count.
 */
static int count_flip_F3(const unsigned long long P) {
	register int n_flipped;

	n_flipped  = COUNT_FLIP[2][((P & 0x2020202020202020ULL) * 0x0008102040810204ULL) >> 56];
	n_flipped += COUNT_FLIP[5][(P >> 16) & 0xff];
	n_flipped += COUNT_FLIP[5][((P & 0x0102040810204080ULL) * 0x0101010101010101ULL) >> 56];
	n_flipped += COUNT_FLIP[5][((P & 0x0000008040201008ULL) * 0x0101010101010101ULL) >> 56];

	return n_flipped;
}

/**
 * Count last flipped discs when playing on square A1.
 *
 * @param P player's disc pattern.
 * @return flipped disc count.
 */
static int count_flip_G3(const unsigned long long P) {
	register int n_flipped;

	n_flipped  = COUNT_FLIP[2][((P & 0x4040404040404040ULL) * 0x0004081020408102ULL) >> 56];
	n_flipped += COUNT_FLIP[6][(P >> 16) & 0xff];
	n_flipped += COUNT_FLIP[6][((P & 0x0204081020408000ULL) * 0x0101010101010101ULL) >> 56];
	n_flipped += ((P & 0x0000000000002010ULL) == 0x0000000000000010ULL);

	return n_flipped;
}

/**
 * Count last flipped discs when playing on square A1.
 *
 * @param P player's disc pattern.
 * @return flipped disc count.
 */
static int count_flip_H3(const unsigned long long P) {
	register int n_flipped;

	n_flipped  = COUNT_FLIP[2][((P & 0x8080808080808080ULL) * 0x0002040810204081ULL) >> 56];
	n_flipped += COUNT_FLIP[7][(P >> 16) & 0xff];
	n_flipped += COUNT_FLIP[7][((P & 0x0408102040800000ULL) * 0x0101010101010101ULL) >> 56];
	n_flipped += ((P & 0x0000000000004020ULL) == 0x0000000000000020ULL);

	return n_flipped;
}

/**
 * Count last flipped discs when playing on square A1.
 *
 * @param P player's disc pattern.
 * @return flipped disc count.
 */
static int count_flip_A4(const unsigned long long P) {
	register int n_flipped;

	n_flipped  = COUNT_FLIP[3][((P & 0x0101010101010101ULL) * 0x0102040810204080ULL) >> 56];
	n_flipped += COUNT_FLIP[0][(P >> 24) & 0xff];
	n_flipped += COUNT_FLIP[0][((P & 0x0000000001020408ULL) * 0x0101010101010101ULL) >> 56];
	n_flipped += COUNT_FLIP[0][((P & 0x1008040201000000ULL) * 0x0101010101010101ULL) >> 56];

	return n_flipped;
}

/**
 * Count last flipped discs when playing on square A1.
 *
 * @param P player's disc pattern.
 * @return flipped disc count.
 */
static int count_flip_B4(const unsigned long long P) {
	register int n_flipped;

	n_flipped  = COUNT_FLIP[3][((P & 0x0202020202020202ULL) * 0x0081020408102040ULL) >> 56];
	n_flipped += COUNT_FLIP[1][(P >> 24) & 0xff];
	n_flipped += COUNT_FLIP[1][((P & 0x0000000102040810ULL) * 0x0101010101010101ULL) >> 56];
	n_flipped += COUNT_FLIP[1][((P & 0x2010080402010000ULL) * 0x0101010101010101ULL) >> 56];

	return n_flipped;
}

/**
 * Count last flipped discs when playing on square A1.
 *
 * @param P player's disc pattern.
 * @return flipped disc count.
 */
static int count_flip_C4(const unsigned long long P) {
	register int n_flipped;

	n_flipped  = COUNT_FLIP[3][((P & 0x0404040404040404ULL) * 0x0040810204081020ULL) >> 56];
	n_flipped += COUNT_FLIP[2][(P >> 24) & 0xff];
	n_flipped += COUNT_FLIP[2][((P & 0x0000010204081020ULL) * 0x0101010101010101ULL) >> 56];
	n_flipped += COUNT_FLIP[2][((P & 0x4020100804020100ULL) * 0x0101010101010101ULL) >> 56];

	return n_flipped;
}

/**
 * Count last flipped discs when playing on square A1.
 *
 * @param P player's disc pattern.
 * @return flipped disc count.
 */
static int count_flip_D4(const unsigned long long P) {
	register int n_flipped;

	n_flipped  = COUNT_FLIP[3][((P & 0x0808080808080808ULL) * 0x0020408102040810ULL) >> 56];
	n_flipped += COUNT_FLIP[3][(P >> 24) & 0xff];
	n_flipped += COUNT_FLIP[3][((P & 0x0001020408102040ULL) * 0x0101010101010101ULL) >> 56];
	n_flipped += COUNT_FLIP[3][((P & 0x8040201008040201ULL) * 0x0101010101010101ULL) >> 56];

	return n_flipped;
}

/**
 * Count last flipped discs when playing on square A1.
 *
 * @param P player's disc pattern.
 * @return flipped disc count.
 */
static int count_flip_E4(const unsigned long long P) {
	register int n_flipped;

	n_flipped  = COUNT_FLIP[3][((P & 0x1010101010101010ULL) * 0x0010204081020408ULL) >> 56];
	n_flipped += COUNT_FLIP[4][(P >> 24) & 0xff];
	n_flipped += COUNT_FLIP[4][((P & 0x0102040810204080ULL) * 0x0101010101010101ULL) >> 56];
	n_flipped += COUNT_FLIP[4][((P & 0x0080402010080402ULL) * 0x0101010101010101ULL) >> 56];

	return n_flipped;
}

/**
 * Count last flipped discs when playing on square A1.
 *
 * @param P player's disc pattern.
 * @return flipped disc count.
 */
static int count_flip_F4(const unsigned long long P) {
	register int n_flipped;

	n_flipped  = COUNT_FLIP[3][((P & 0x2020202020202020ULL) * 0x0008102040810204ULL) >> 56];
	n_flipped += COUNT_FLIP[5][(P >> 24) & 0xff];
	n_flipped += COUNT_FLIP[5][((P & 0x0204081020408000ULL) * 0x0101010101010101ULL) >> 56];
	n_flipped += COUNT_FLIP[5][((P & 0x0000804020100804ULL) * 0x0101010101010101ULL) >> 56];

	return n_flipped;
}

/**
 * Count last flipped discs when playing on square A1.
 *
 * @param P player's disc pattern.
 * @return flipped disc count.
 */
static int count_flip_G4(const unsigned long long P) {
	register int n_flipped;

	n_flipped  = COUNT_FLIP[3][((P & 0x4040404040404040ULL) * 0x0004081020408102ULL) >> 56];
	n_flipped += COUNT_FLIP[6][(P >> 24) & 0xff];
	n_flipped += COUNT_FLIP[6][((P & 0x0408102040800000ULL) * 0x0101010101010101ULL) >> 56];
	n_flipped += COUNT_FLIP[6][((P & 0x0000008040201008ULL) * 0x0101010101010101ULL) >> 56];

	return n_flipped;
}

/**
 * Count last flipped discs when playing on square A1.
 *
 * @param P player's disc pattern.
 * @return flipped disc count.
 */
static int count_flip_H4(const unsigned long long P) {
	register int n_flipped;

	n_flipped  = COUNT_FLIP[3][((P & 0x8080808080808080ULL) * 0x0002040810204081ULL) >> 56];
	n_flipped += COUNT_FLIP[7][(P >> 24) & 0xff];
	n_flipped += COUNT_FLIP[7][((P & 0x0810204080000000ULL) * 0x0101010101010101ULL) >> 56];
	n_flipped += COUNT_FLIP[7][((P & 0x0000000080402010ULL) * 0x0101010101010101ULL) >> 56];

	return n_flipped;
}

/**
 * Count last flipped discs when playing on square A1.
 *
 * @param P player's disc pattern.
 * @return flipped disc count.
 */
static int count_flip_A5(const unsigned long long P) {
	register int n_flipped;

	n_flipped  = COUNT_FLIP[4][((P & 0x0101010101010101ULL) * 0x0102040810204080ULL) >> 56];
	n_flipped += COUNT_FLIP[0][(P >> 32) & 0xff];
	n_flipped += COUNT_FLIP[0][((P & 0x0000000102040810ULL) * 0x0101010101010101ULL) >> 56];
	n_flipped += COUNT_FLIP[0][((P & 0x0804020100000000ULL) * 0x0101010101010101ULL) >> 56];

	return n_flipped;
}

/**
 * Count last flipped discs when playing on square A1.
 *
 * @param P player's disc pattern.
 * @return flipped disc count.
 */
static int count_flip_B5(const unsigned long long P) {
	register int n_flipped;

	n_flipped  = COUNT_FLIP[4][((P & 0x0202020202020202ULL) * 0x0081020408102040ULL) >> 56];
	n_flipped += COUNT_FLIP[1][(P >> 32) & 0xff];
	n_flipped += COUNT_FLIP[1][((P & 0x0000010204081020ULL) * 0x0101010101010101ULL) >> 56];
	n_flipped += COUNT_FLIP[1][((P & 0x1008040201000000ULL) * 0x0101010101010101ULL) >> 56];

	return n_flipped;
}

/**
 * Count last flipped discs when playing on square A1.
 *
 * @param P player's disc pattern.
 * @return flipped disc count.
 */
static int count_flip_C5(const unsigned long long P) {
	register int n_flipped;

	n_flipped  = COUNT_FLIP[4][((P & 0x0404040404040404ULL) * 0x0040810204081020ULL) >> 56];
	n_flipped += COUNT_FLIP[2][(P >> 32) & 0xff];
	n_flipped += COUNT_FLIP[2][((P & 0x0001020408102040ULL) * 0x0101010101010101ULL) >> 56];
	n_flipped += COUNT_FLIP[2][((P & 0x2010080402010000ULL) * 0x0101010101010101ULL) >> 56];

	return n_flipped;
}

/**
 * Count last flipped discs when playing on square A1.
 *
 * @param P player's disc pattern.
 * @return flipped disc count.
 */
static int count_flip_D5(const unsigned long long P) {
	register int n_flipped;

	n_flipped  = COUNT_FLIP[4][((P & 0x0808080808080808ULL) * 0x0020408102040810ULL) >> 56];
	n_flipped += COUNT_FLIP[3][(P >> 32) & 0xff];
	n_flipped += COUNT_FLIP[3][((P & 0x0102040810204080ULL) * 0x0101010101010101ULL) >> 56];
	n_flipped += COUNT_FLIP[3][((P & 0x4020100804020100ULL) * 0x0101010101010101ULL) >> 56];

	return n_flipped;
}

/**
 * Count last flipped discs when playing on square A1.
 *
 * @param P player's disc pattern.
 * @return flipped disc count.
 */
static int count_flip_E5(const unsigned long long P) {
	register int n_flipped;

	n_flipped  = COUNT_FLIP[4][((P & 0x1010101010101010ULL) * 0x0010204081020408ULL) >> 56];
	n_flipped += COUNT_FLIP[4][(P >> 32) & 0xff];
	n_flipped += COUNT_FLIP[4][((P & 0x0204081020408000ULL) * 0x0101010101010101ULL) >> 56];
	n_flipped += COUNT_FLIP[4][((P & 0x8040201008040201ULL) * 0x0101010101010101ULL) >> 56];

	return n_flipped;
}

/**
 * Count last flipped discs when playing on square A1.
 *
 * @param P player's disc pattern.
 * @return flipped disc count.
 */
static int count_flip_F5(const unsigned long long P) {
	register int n_flipped;

	n_flipped  = COUNT_FLIP[4][((P & 0x2020202020202020ULL) * 0x0008102040810204ULL) >> 56];
	n_flipped += COUNT_FLIP[5][(P >> 32) & 0xff];
	n_flipped += COUNT_FLIP[5][((P & 0x0408102040800000ULL) * 0x0101010101010101ULL) >> 56];
	n_flipped += COUNT_FLIP[5][((P & 0x0080402010080402ULL) * 0x0101010101010101ULL) >> 56];

	return n_flipped;
}

/**
 * Count last flipped discs when playing on square A1.
 *
 * @param P player's disc pattern.
 * @return flipped disc count.
 */
static int count_flip_G5(const unsigned long long P) {
	register int n_flipped;

	n_flipped  = COUNT_FLIP[4][((P & 0x4040404040404040ULL) * 0x0004081020408102ULL) >> 56];
	n_flipped += COUNT_FLIP[6][(P >> 32) & 0xff];
	n_flipped += COUNT_FLIP[6][((P & 0x0810204080000000ULL) * 0x0101010101010101ULL) >> 56];
	n_flipped += COUNT_FLIP[6][((P & 0x0000804020100804ULL) * 0x0101010101010101ULL) >> 56];

	return n_flipped;
}

/**
 * Count last flipped discs when playing on square A1.
 *
 * @param P player's disc pattern.
 * @return flipped disc count.
 */
static int count_flip_H5(const unsigned long long P) {
	register int n_flipped;

	n_flipped  = COUNT_FLIP[4][((P & 0x8080808080808080ULL) * 0x0002040810204081ULL) >> 56];
	n_flipped += COUNT_FLIP[7][(P >> 32) & 0xff];
	n_flipped += COUNT_FLIP[7][((P & 0x1020408000000000ULL) * 0x0101010101010101ULL) >> 56];
	n_flipped += COUNT_FLIP[7][((P & 0x0000008040201008ULL) * 0x0101010101010101ULL) >> 56];

	return n_flipped;
}

/**
 * Count last flipped discs when playing on square A1.
 *
 * @param P player's disc pattern.
 * @return flipped disc count.
 */
static int count_flip_A6(const unsigned long long P) {
	register int n_flipped;

	n_flipped  = COUNT_FLIP[5][((P & 0x0101010101010101ULL) * 0x0102040810204080ULL) >> 56];
	n_flipped += COUNT_FLIP[0][(P >> 40) & 0xff];
	n_flipped += COUNT_FLIP[0][((P & 0x0000010204081020ULL) * 0x0101010101010101ULL) >> 56];
	n_flipped += ((P & 0x0402000000000000ULL) == 0x0400000000000000ULL);

	return n_flipped;
}

/**
 * Count last flipped discs when playing on square A1.
 *
 * @param P player's disc pattern.
 * @return flipped disc count.
 */
static int count_flip_B6(const unsigned long long P) {
	register int n_flipped;

	n_flipped  = COUNT_FLIP[5][((P & 0x0202020202020202ULL) * 0x0081020408102040ULL) >> 56];
	n_flipped += COUNT_FLIP[1][(P >> 40) & 0xff];
	n_flipped += COUNT_FLIP[1][((P & 0x0001020408102040ULL) * 0x0101010101010101ULL) >> 56];
	n_flipped += ((P & 0x0804000000000000ULL) == 0x0800000000000000ULL);

	return n_flipped;
}

/**
 * Count last flipped discs when playing on square A1.
 *
 * @param P player's disc pattern.
 * @return flipped disc count.
 */
static int count_flip_C6(const unsigned long long P) {
	register int n_flipped;

	n_flipped  = COUNT_FLIP[5][((P & 0x0404040404040404ULL) * 0x0040810204081020ULL) >> 56];
	n_flipped += COUNT_FLIP[2][(P >> 40) & 0xff];
	n_flipped += COUNT_FLIP[2][((P & 0x0102040810204080ULL) * 0x0101010101010101ULL) >> 56];
	n_flipped += COUNT_FLIP[2][((P & 0x1008040201000000ULL) * 0x0101010101010101ULL) >> 56];

	return n_flipped;
}

/**
 * Count last flipped discs when playing on square A1.
 *
 * @param P player's disc pattern.
 * @return flipped disc count.
 */
static int count_flip_D6(const unsigned long long P) {
	register int n_flipped;

	n_flipped  = COUNT_FLIP[5][((P & 0x0808080808080808ULL) * 0x0020408102040810ULL) >> 56];
	n_flipped += COUNT_FLIP[3][(P >> 40) & 0xff];
	n_flipped += COUNT_FLIP[3][((P & 0x0204081020408000ULL) * 0x0101010101010101ULL) >> 56];
	n_flipped += COUNT_FLIP[3][((P & 0x2010080402010000ULL) * 0x0101010101010101ULL) >> 56];

	return n_flipped;
}

/**
 * Count last flipped discs when playing on square A1.
 *
 * @param P player's disc pattern.
 * @return flipped disc count.
 */
static int count_flip_E6(const unsigned long long P) {
	register int n_flipped;

	n_flipped  = COUNT_FLIP[5][((P & 0x1010101010101010ULL) * 0x0010204081020408ULL) >> 56];
	n_flipped += COUNT_FLIP[4][(P >> 40) & 0xff];
	n_flipped += COUNT_FLIP[4][((P & 0x0408102040800000ULL) * 0x0101010101010101ULL) >> 56];
	n_flipped += COUNT_FLIP[4][((P & 0x4020100804020100ULL) * 0x0101010101010101ULL) >> 56];

	return n_flipped;
}

/**
 * Count last flipped discs when playing on square A1.
 *
 * @param P player's disc pattern.
 * @return flipped disc count.
 */
static int count_flip_F6(const unsigned long long P) {
	register int n_flipped;

	n_flipped  = COUNT_FLIP[5][((P & 0x2020202020202020ULL) * 0x0008102040810204ULL) >> 56];
	n_flipped += COUNT_FLIP[5][(P >> 40) & 0xff];
	n_flipped += COUNT_FLIP[5][((P & 0x0810204080000000ULL) * 0x0101010101010101ULL) >> 56];
	n_flipped += COUNT_FLIP[5][((P & 0x8040201008040201ULL) * 0x0101010101010101ULL) >> 56];

	return n_flipped;
}

/**
 * Count last flipped discs when playing on square A1.
 *
 * @param P player's disc pattern.
 * @return flipped disc count.
 */
static int count_flip_G6(const unsigned long long P) {
	register int n_flipped;

	n_flipped  = COUNT_FLIP[5][((P & 0x4040404040404040ULL) * 0x0004081020408102ULL) >> 56];
	n_flipped += COUNT_FLIP[6][(P >> 40) & 0xff];
	n_flipped += ((P & 0x1020000000000000ULL) == 0x1000000000000000ULL);
	n_flipped += COUNT_FLIP[6][((P & 0x0080402010080402ULL) * 0x0101010101010101ULL) >> 56];

	return n_flipped;
}

/**
 * Count last flipped discs when playing on square A1.
 *
 * @param P player's disc pattern.
 * @return flipped disc count.
 */
static int count_flip_H6(const unsigned long long P) {
	register int n_flipped;

	n_flipped  = COUNT_FLIP[5][((P & 0x8080808080808080ULL) * 0x0002040810204081ULL) >> 56];
	n_flipped += COUNT_FLIP[7][(P >> 40) & 0xff];
	n_flipped += ((P & 0x2040000000000000ULL) == 0x2000000000000000ULL);
	n_flipped += COUNT_FLIP[7][((P & 0x0000804020100804ULL) * 0x0101010101010101ULL) >> 56];

	return n_flipped;
}

/**
 * Count last flipped discs when playing on square A1.
 *
 * @param P player's disc pattern.
 * @return flipped disc count.
 */
static int count_flip_A7(const unsigned long long P) {
	register int n_flipped;

	n_flipped  = COUNT_FLIP[6][((P & 0x0101010101010101ULL) * 0x0102040810204080ULL) >> 56];
	n_flipped += COUNT_FLIP[0][(P >> 48) & 0xff];
	n_flipped += COUNT_FLIP[0][((P & 0x0001020408102040ULL) * 0x0101010101010101ULL) >> 56];

	return n_flipped;
}

/**
 * Count last flipped discs when playing on square A1.
 *
 * @param P player's disc pattern.
 * @return flipped disc count.
 */
static int count_flip_B7(const unsigned long long P) {
	register int n_flipped;

	n_flipped  = COUNT_FLIP[6][((P & 0x0202020202020202ULL) * 0x0081020408102040ULL) >> 56];
	n_flipped += COUNT_FLIP[1][(P >> 48) & 0xff];
	n_flipped += COUNT_FLIP[1][((P & 0x0102040810204080ULL) * 0x0101010101010101ULL) >> 56];

	return n_flipped;
}

/**
 * Count last flipped discs when playing on square A1.
 *
 * @param P player's disc pattern.
 * @return flipped disc count.
 */
static int count_flip_C7(const unsigned long long P) {
	register int n_flipped;

	n_flipped  = COUNT_FLIP[6][((P & 0x0404040404040404ULL) * 0x0040810204081020ULL) >> 56];
	n_flipped += COUNT_FLIP[2][(P >> 48) & 0xff];
	n_flipped += COUNT_FLIP[2][((P & 0x0204081020408000ULL) * 0x0101010101010101ULL) >> 56];
	n_flipped += ((P & 0x0000020100000000ULL) == 0x0000000100000000ULL);

	return n_flipped;
}

/**
 * Count last flipped discs when playing on square A1.
 *
 * @param P player's disc pattern.
 * @return flipped disc count.
 */
static int count_flip_D7(const unsigned long long P) {
	register int n_flipped;

	n_flipped  = COUNT_FLIP[6][((P & 0x0808080808080808ULL) * 0x0020408102040810ULL) >> 56];
	n_flipped += COUNT_FLIP[3][(P >> 48) & 0xff];
	n_flipped += COUNT_FLIP[3][((P & 0x0408102040800000ULL) * 0x0101010101010101ULL) >> 56];
	n_flipped += COUNT_FLIP[3][((P & 0x1008040201000000ULL) * 0x0101010101010101ULL) >> 56];

	return n_flipped;
}

/**
 * Count last flipped discs when playing on square A1.
 *
 * @param P player's disc pattern.
 * @return flipped disc count.
 */
static int count_flip_E7(const unsigned long long P) {
	register int n_flipped;

	n_flipped  = COUNT_FLIP[6][((P & 0x1010101010101010ULL) * 0x0010204081020408ULL) >> 56];
	n_flipped += COUNT_FLIP[4][(P >> 48) & 0xff];
	n_flipped += COUNT_FLIP[4][((P & 0x0810204080000000ULL) * 0x0101010101010101ULL) >> 56];
	n_flipped += COUNT_FLIP[4][((P & 0x2010080402010000ULL) * 0x0101010101010101ULL) >> 56];

	return n_flipped;
}

/**
 * Count last flipped discs when playing on square A1.
 *
 * @param P player's disc pattern.
 * @return flipped disc count.
 */
static int count_flip_F7(const unsigned long long P) {
	register int n_flipped;

	n_flipped  = COUNT_FLIP[6][((P & 0x2020202020202020ULL) * 0x0008102040810204ULL) >> 56];
	n_flipped += COUNT_FLIP[5][(P >> 48) & 0xff];
	n_flipped += ((P & 0x0000408000000000ULL) == 0x0000008000000000ULL);
	n_flipped += COUNT_FLIP[5][((P & 0x4020100804020100ULL) * 0x0101010101010101ULL) >> 56];

	return n_flipped;
}

/**
 * Count last flipped discs when playing on square A1.
 *
 * @param P player's disc pattern.
 * @return flipped disc count.
 */
static int count_flip_G7(const unsigned long long P) {
	register int n_flipped;

	n_flipped  = COUNT_FLIP[6][((P & 0x4040404040404040ULL) * 0x0004081020408102ULL) >> 56];
	n_flipped += COUNT_FLIP[6][(P >> 48) & 0xff];
	n_flipped += COUNT_FLIP[6][((P & 0x8040201008040201ULL) * 0x0101010101010101ULL) >> 56];

	return n_flipped;
}

/**
 * Count last flipped discs when playing on square A1.
 *
 * @param P player's disc pattern.
 * @return flipped disc count.
 */
static int count_flip_H7(const unsigned long long P) {
	register int n_flipped;

	n_flipped  = COUNT_FLIP[6][((P & 0x8080808080808080ULL) * 0x0002040810204081ULL) >> 56];
	n_flipped += COUNT_FLIP[7][(P >> 48) & 0xff];
	n_flipped += COUNT_FLIP[7][((P & 0x0080402010080402ULL) * 0x0101010101010101ULL) >> 56];

	return n_flipped;
}

/**
 * Count last flipped discs when playing on square A1.
 *
 * @param P player's disc pattern.
 * @return flipped disc count.
 */
static int count_flip_A8(const unsigned long long P) {
	register int n_flipped;

	n_flipped  = COUNT_FLIP[7][((P & 0x0101010101010101ULL) * 0x0102040810204080ULL) >> 56];
	n_flipped += COUNT_FLIP[0][P >> 56];
	n_flipped += COUNT_FLIP[0][((P & 0x0102040810204080ULL) * 0x0101010101010101ULL) >> 56];

	return n_flipped;
}

/**
 * Count last flipped discs when playing on square A1.
 *
 * @param P player's disc pattern.
 * @return flipped disc count.
 */
static int count_flip_B8(const unsigned long long P) {
	register int n_flipped;

	n_flipped  = COUNT_FLIP[7][((P & 0x0202020202020202ULL) * 0x0081020408102040ULL) >> 56];
	n_flipped += COUNT_FLIP[1][P >> 56];
	n_flipped += COUNT_FLIP[1][((P & 0x0204081020408000ULL) * 0x0101010101010101ULL) >> 56];

	return n_flipped;
}

/**
 * Count last flipped discs when playing on square A1.
 *
 * @param P player's disc pattern.
 * @return flipped disc count.
 */
static int count_flip_C8(const unsigned long long P) {
	register int n_flipped;

	n_flipped  = COUNT_FLIP[7][((P & 0x0404040404040404ULL) * 0x0040810204081020ULL) >> 56];
	n_flipped += COUNT_FLIP[2][P >> 56];
	n_flipped += COUNT_FLIP[2][((P & 0x0408102040800000ULL) * 0x0101010101010101ULL) >> 56];
	n_flipped += ((P & 0x0002010000000000ULL) == 0x0000010000000000ULL);

	return n_flipped;
}

/**
 * Count last flipped discs when playing on square A1.
 *
 * @param P player's disc pattern.
 * @return flipped disc count.
 */
static int count_flip_D8(const unsigned long long P) {
	register int n_flipped;

	n_flipped  = COUNT_FLIP[7][((P & 0x0808080808080808ULL) * 0x0020408102040810ULL) >> 56];
	n_flipped += COUNT_FLIP[3][P >> 56];
	n_flipped += COUNT_FLIP[3][((P & 0x0810204080000000ULL) * 0x0101010101010101ULL) >> 56];
	n_flipped += COUNT_FLIP[3][((P & 0x0804020100000000ULL) * 0x0101010101010101ULL) >> 56];

	return n_flipped;
}

/**
 * Count last flipped discs when playing on square A1.
 *
 * @param P player's disc pattern.
 * @return flipped disc count.
 */
static int count_flip_E8(const unsigned long long P) {
	register int n_flipped;

	n_flipped  = COUNT_FLIP[7][((P & 0x1010101010101010ULL) * 0x0010204081020408ULL) >> 56];
	n_flipped += COUNT_FLIP[4][P >> 56];
	n_flipped += COUNT_FLIP[4][((P & 0x1020408000000000ULL) * 0x0101010101010101ULL) >> 56];
	n_flipped += COUNT_FLIP[4][((P & 0x1008040201000000ULL) * 0x0101010101010101ULL) >> 56];

	return n_flipped;
}

/**
 * Count last flipped discs when playing on square A1.
 *
 * @param P player's disc pattern.
 * @return flipped disc count.
 */
static int count_flip_F8(const unsigned long long P) {
	register int n_flipped;

	n_flipped  = COUNT_FLIP[7][((P & 0x2020202020202020ULL) * 0x0008102040810204ULL) >> 56];
	n_flipped += COUNT_FLIP[5][P >> 56];
	n_flipped += ((P & 0x0040800000000000ULL) == 0x0000800000000000ULL);
	n_flipped += COUNT_FLIP[5][((P & 0x2010080402010000ULL) * 0x0101010101010101ULL) >> 56];

	return n_flipped;
}

/**
 * Count last flipped discs when playing on square A1.
 *
 * @param P player's disc pattern.
 * @return flipped disc count.
 */
static int count_flip_G8(const unsigned long long P) {
	register int n_flipped;

	n_flipped  = COUNT_FLIP[7][((P & 0x4040404040404040ULL) * 0x0004081020408102ULL) >> 56];
	n_flipped += COUNT_FLIP[6][P >> 56];
	n_flipped += COUNT_FLIP[6][((P & 0x4020100804020100ULL) * 0x0101010101010101ULL) >> 56];

	return n_flipped;
}

/**
 * Count last flipped discs when playing on square A1.
 *
 * @param P player's disc pattern.
 * @return flipped disc count.
 */
static int count_flip_H8(const unsigned long long P) {
	register int n_flipped;

	n_flipped  = COUNT_FLIP[7][((P & 0x8080808080808080ULL) * 0x0002040810204081ULL) >> 56];
	n_flipped += COUNT_FLIP[7][P >> 56];
	n_flipped += COUNT_FLIP[7][((P & 0x8040201008040201ULL) * 0x0101010101010101ULL) >> 56];

	return n_flipped;
}

static int count_flip_pass(const BitArray) {
		return 0;
}


int (*count_flip[65])(const BitArray) = {
	count_flip_A1, count_flip_B1, count_flip_C1, count_flip_D1, 
	count_flip_E1, count_flip_F1, count_flip_G1, count_flip_H1, 
	count_flip_A2, count_flip_B2, count_flip_C2, count_flip_D2, 
	count_flip_E2, count_flip_F2, count_flip_G2, count_flip_H2, 
	count_flip_A3, count_flip_B3, count_flip_C3, count_flip_D3, 
	count_flip_E3, count_flip_F3, count_flip_G3, count_flip_H3, 
	count_flip_A4, count_flip_B4, count_flip_C4, count_flip_D4, 
	count_flip_E4, count_flip_F4, count_flip_G4, count_flip_H4, 
	count_flip_A5, count_flip_B5, count_flip_C5, count_flip_D5, 
	count_flip_E5, count_flip_F5, count_flip_G5, count_flip_H5, 
	count_flip_A6, count_flip_B6, count_flip_C6, count_flip_D6, 
	count_flip_E6, count_flip_F6, count_flip_G6, count_flip_H6, 
	count_flip_A7, count_flip_B7, count_flip_C7, count_flip_D7, 
	count_flip_E7, count_flip_F7, count_flip_G7, count_flip_H7, 
	count_flip_A8, count_flip_B8, count_flip_C8, count_flip_D8, 
	count_flip_E8, count_flip_F8, count_flip_G8, count_flip_H8, 
	count_flip_pass,
};

