/**
 * @file count_last_flip_carry_32.c
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
 * With 135 degree merge, instead of Valery ClaudePierre's modification.
 *
 * @date 1998 - 2012
 * @author Richard Delorme
 * @author Toshihiko Okuhara
 * @version 4.3
 * 
 */

#define LODWORD(l) ((unsigned int)(l))
#define HIDWORD(l) ((unsigned int)((l)>>32))

/** precomputed count flip array */
static const char COUNT_FLIP_R[128] = {
	 0,  0,  2,  0,  4,  0,  2,  0,  6,  0,  2,  0,  4,  0,  2,  0,
	 8,  0,  2,  0,  4,  0,  2,  0,  6,  0,  2,  0,  4,  0,  2,  0,
	10,  0,  2,  0,  4,  0,  2,  0,  6,  0,  2,  0,  4,  0,  2,  0,
	 8,  0,  2,  0,  4,  0,  2,  0,  6,  0,  2,  0,  4,  0,  2,  0,
	12,  0,  2,  0,  4,  0,  2,  0,  6,  0,  2,  0,  4,  0,  2,  0,
	 8,  0,  2,  0,  4,  0,  2,  0,  6,  0,  2,  0,  4,  0,  2,  0,
	10,  0,  2,  0,  4,  0,  2,  0,  6,  0,  2,  0,  4,  0,  2,  0,
	 8,  0,  2,  0,  4,  0,  2,  0,  6,  0,  2,  0,  4,  0,  2,  0
};

static const char COUNT_FLIP_2[256] = {
	 0,  2,  0,  0,  0,  2,  0,  0,  0,  2,  0,  0,  0,  2,  0,  0,  2,  4,  2,  2,  2,  4,  2,  2,  0,  2,  0,  0,  0,  2,  0,  0,
	 4,  6,  4,  4,  4,  6,  4,  4,  0,  2,  0,  0,  0,  2,  0,  0,  2,  4,  2,  2,  2,  4,  2,  2,  0,  2,  0,  0,  0,  2,  0,  0,
	 6,  8,  6,  6,  6,  8,  6,  6,  0,  2,  0,  0,  0,  2,  0,  0,  2,  4,  2,  2,  2,  4,  2,  2,  0,  2,  0,  0,  0,  2,  0,  0,
	 4,  6,  4,  4,  4,  6,  4,  4,  0,  2,  0,  0,  0,  2,  0,  0,  2,  4,  2,  2,  2,  4,  2,  2,  0,  2,  0,  0,  0,  2,  0,  0,
	 8, 10,  8,  8,  8, 10,  8,  8,  0,  2,  0,  0,  0,  2,  0,  0,  2,  4,  2,  2,  2,  4,  2,  2,  0,  2,  0,  0,  0,  2,  0,  0,
	 4,  6,  4,  4,  4,  6,  4,  4,  0,  2,  0,  0,  0,  2,  0,  0,  2,  4,  2,  2,  2,  4,  2,  2,  0,  2,  0,  0,  0,  2,  0,  0,
	 6,  8,  6,  6,  6,  8,  6,  6,  0,  2,  0,  0,  0,  2,  0,  0,  2,  4,  2,  2,  2,  4,  2,  2,  0,  2,  0,  0,  0,  2,  0,  0,
	 4,  6,  4,  4,  4,  6,  4,  4,  0,  2,  0,  0,  0,  2,  0,  0,  2,  4,  2,  2,  2,  4,  2,  2,  0,  2,  0,  0,  0,  2,  0,  0
};

static const char COUNT_FLIP_3[256] = {
	 0,  4,  2,  2,  0,  0,  0,  0,  0,  4,  2,  2,  0,  0,  0,  0,  0,  4,  2,  2,  0,  0,  0,  0,  0,  4,  2,  2,  0,  0,  0,  0,
	 2,  6,  4,  4,  2,  2,  2,  2,  2,  6,  4,  4,  2,  2,  2,  2,  0,  4,  2,  2,  0,  0,  0,  0,  0,  4,  2,  2,  0,  0,  0,  0,
	 4,  8,  6,  6,  4,  4,  4,  4,  4,  8,  6,  6,  4,  4,  4,  4,  0,  4,  2,  2,  0,  0,  0,  0,  0,  4,  2,  2,  0,  0,  0,  0,
	 2,  6,  4,  4,  2,  2,  2,  2,  2,  6,  4,  4,  2,  2,  2,  2,  0,  4,  2,  2,  0,  0,  0,  0,  0,  4,  2,  2,  0,  0,  0,  0,
	 6, 10,  8,  8,  6,  6,  6,  6,  6, 10,  8,  8,  6,  6,  6,  6,  0,  4,  2,  2,  0,  0,  0,  0,  0,  4,  2,  2,  0,  0,  0,  0,
	 2,  6,  4,  4,  2,  2,  2,  2,  2,  6,  4,  4,  2,  2,  2,  2,  0,  4,  2,  2,  0,  0,  0,  0,  0,  4,  2,  2,  0,  0,  0,  0,
	 4,  8,  6,  6,  4,  4,  4,  4,  4,  8,  6,  6,  4,  4,  4,  4,  0,  4,  2,  2,  0,  0,  0,  0,  0,  4,  2,  2,  0,  0,  0,  0,
	 2,  6,  4,  4,  2,  2,  2,  2,  2,  6,  4,  4,  2,  2,  2,  2,  0,  4,  2,  2,  0,  0,  0,  0,  0,  4,  2,  2,  0,  0,  0,  0
};

static const char COUNT_FLIP_4[256] = {
	 0,  6,  4,  4,  2,  2,  2,  2,  0,  0,  0,  0,  0,  0,  0,  0,  0,  6,  4,  4,  2,  2,  2,  2,  0,  0,  0,  0,  0,  0,  0,  0,
	 0,  6,  4,  4,  2,  2,  2,  2,  0,  0,  0,  0,  0,  0,  0,  0,  0,  6,  4,  4,  2,  2,  2,  2,  0,  0,  0,  0,  0,  0,  0,  0,
	 2,  8,  6,  6,  4,  4,  4,  4,  2,  2,  2,  2,  2,  2,  2,  2,  2,  8,  6,  6,  4,  4,  4,  4,  2,  2,  2,  2,  2,  2,  2,  2,
	 0,  6,  4,  4,  2,  2,  2,  2,  0,  0,  0,  0,  0,  0,  0,  0,  0,  6,  4,  4,  2,  2,  2,  2,  0,  0,  0,  0,  0,  0,  0,  0,
	 4, 10,  8,  8,  6,  6,  6,  6,  4,  4,  4,  4,  4,  4,  4,  4,  4, 10,  8,  8,  6,  6,  6,  6,  4,  4,  4,  4,  4,  4,  4,  4,
	 0,  6,  4,  4,  2,  2,  2,  2,  0,  0,  0,  0,  0,  0,  0,  0,  0,  6,  4,  4,  2,  2,  2,  2,  0,  0,  0,  0,  0,  0,  0,  0,
	 2,  8,  6,  6,  4,  4,  4,  4,  2,  2,  2,  2,  2,  2,  2,  2,  2,  8,  6,  6,  4,  4,  4,  4,  2,  2,  2,  2,  2,  2,  2,  2,
	 0,  6,  4,  4,  2,  2,  2,  2,  0,  0,  0,  0,  0,  0,  0,  0,  0,  6,  4,  4,  2,  2,  2,  2,  0,  0,  0,  0,  0,  0,  0,  0
};

static const char COUNT_FLIP_5[256] = {
	 0,  8,  6,  6,  4,  4,  4,  4,  2,  2,  2,  2,  2,  2,  2,  2,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
	 0,  8,  6,  6,  4,  4,  4,  4,  2,  2,  2,  2,  2,  2,  2,  2,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
	 0,  8,  6,  6,  4,  4,  4,  4,  2,  2,  2,  2,  2,  2,  2,  2,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
	 0,  8,  6,  6,  4,  4,  4,  4,  2,  2,  2,  2,  2,  2,  2,  2,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
	 2, 10,  8,  8,  6,  6,  6,  6,  4,  4,  4,  4,  4,  4,  4,  4,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,
	 2, 10,  8,  8,  6,  6,  6,  6,  4,  4,  4,  4,  4,  4,  4,  4,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,
	 0,  8,  6,  6,  4,  4,  4,  4,  2,  2,  2,  2,  2,  2,  2,  2,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
	 0,  8,  6,  6,  4,  4,  4,  4,  2,  2,  2,  2,  2,  2,  2,  2,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0
};

static const char COUNT_FLIP_L[128] = {
	 0, 12, 10, 10,  8,  8,  8,  8,  6,  6,  6,  6,  6,  6,  6,  6,  4,  4,  4,  4,  4,  4,  4,  4,  4,  4,  4,  4,  4,  4,  4,  4,
	 2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,
	 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
	 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0
};

/**
 * Count last flipped discs when playing on square A1.
 *
 * @param P player's disc pattern.
 * @return flipped disc count.
 */
static int count_last_flip_A1(const unsigned long long P)
{
	int n_flipped;

	n_flipped  = COUNT_FLIP_R[(((LODWORD(P) & 0x01010100u) + ((HIDWORD(P) & 0x01010101u) << 4)) * 0x01020408u) >> 25];
	n_flipped += COUNT_FLIP_R[(LODWORD(P) >> 1) & 0x7f];
	n_flipped += COUNT_FLIP_R[(((LODWORD(P) & 0x08040200u) + (HIDWORD(P) & 0x80402010u)) * 0x01010101u) >> 25];

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
	int n_flipped;

	n_flipped  = COUNT_FLIP_R[(((LODWORD(P) & 0x02020200u) + ((HIDWORD(P) & 0x02020202u) << 4)) * 0x00810204u) >> 25];
	n_flipped += COUNT_FLIP_R[(LODWORD(P) >> 2) & 0x3f];
	n_flipped += COUNT_FLIP_R[(((LODWORD(P) & 0x10080400u) + (HIDWORD(P) & 0x00804020u)) * 0x01010101u) >> 26];

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
	int n_flipped;

	n_flipped  = COUNT_FLIP_R[(((LODWORD(P) & 0x04040400u) + ((HIDWORD(P) & 0x04040404u) << 4)) * 0x00408102u) >> 25];
	n_flipped += COUNT_FLIP_2[LODWORD(P) & 0xff];
	n_flipped += COUNT_FLIP_2[(((LODWORD(P) & 0x20110A04u) + (HIDWORD(P) & 0x00008040u)) * 0x01010101u) >> 24];

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
	int n_flipped;

	n_flipped  = COUNT_FLIP_R[(((LODWORD(P) & 0x08080800u) + ((HIDWORD(P) & 0x08080808u) << 4)) * 0x00204081u) >> 25];
	n_flipped += COUNT_FLIP_3[LODWORD(P) & 0xff];
	n_flipped += COUNT_FLIP_3[(((LODWORD(P) & 0x41221408u) + (HIDWORD(P) & 0x00000080u)) * 0x01010101u) >> 24];

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
	int n_flipped;

	n_flipped  = COUNT_FLIP_R[((((LODWORD(P) & 0x10101000u) >> 4) + (HIDWORD(P) & 0x10101010u)) * 0x01020408u) >> 25];
	n_flipped += COUNT_FLIP_4[LODWORD(P) & 0xff];
	n_flipped += COUNT_FLIP_4[(((LODWORD(P) & 0x82442810u) + (HIDWORD(P) & 0x00000001u)) * 0x01010101u) >> 24];

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
	int n_flipped;

	n_flipped  = COUNT_FLIP_R[(((HIDWORD(P) & 0x20202020u) + ((LODWORD(P) >> 4) & 0x02020200u)) * 0x00810204u) >> 25];
	n_flipped += COUNT_FLIP_5[LODWORD(P) & 0xff];
	n_flipped += COUNT_FLIP_5[(((LODWORD(P) & 0x04885020u) + (HIDWORD(P) & 0x00000102u)) * 0x01010101u) >> 24];

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
	int n_flipped;

	n_flipped  = COUNT_FLIP_R[((((LODWORD(P) & 0x40404000u) >> 4) + (HIDWORD(P) & 0x40404040u)) * 0x00408102u) >> 25];
	n_flipped += COUNT_FLIP_L[(LODWORD(P) << 1) & 0x7e];
	n_flipped += COUNT_FLIP_L[(((LODWORD(P) & 0x08102000u) + (HIDWORD(P) & 0x00010204u)) * 0x02020202u) >> 24];

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
	int n_flipped;

	n_flipped  = COUNT_FLIP_R[((((LODWORD(P) & 0x80808000u) >> 4) + (HIDWORD(P) & 0x80808080u)) * 0x00204081u) >> 25];
	n_flipped += COUNT_FLIP_L[LODWORD(P) & 0x7f];
	n_flipped += COUNT_FLIP_L[(((LODWORD(P) & 0x10204000u) + (HIDWORD(P) & 0x01020408u)) * 0x01010101u) >> 24];

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
	int n_flipped;

	n_flipped  = COUNT_FLIP_R[(((LODWORD(P) & 0x01010000u) + ((HIDWORD(P) & 0x01010101u) << 4)) * 0x01020408u) >> 26];
	n_flipped += COUNT_FLIP_R[(LODWORD(P) >> 9) & 0x7f];
	n_flipped += COUNT_FLIP_R[(((LODWORD(P) & 0x04020000u) + (HIDWORD(P) & 0x40201008u)) * 0x01010101u) >> 25];

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
	int n_flipped;

	n_flipped  = COUNT_FLIP_R[(((LODWORD(P) & 0x02020000u) + ((HIDWORD(P) & 0x02020202u) << 4)) * 0x00810204u) >> 26];
	n_flipped += COUNT_FLIP_R[(LODWORD(P) >> 10) & 0x3f];
	n_flipped += COUNT_FLIP_R[(((LODWORD(P) & 0x08040000u) + (HIDWORD(P) & 0x80402010u)) * 0x01010101u) >> 26];

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
	int n_flipped;

	n_flipped  = COUNT_FLIP_R[(((LODWORD(P) & 0x04040000u) + ((HIDWORD(P) & 0x04040404u) << 4)) * 0x00408102u) >> 26];
	n_flipped += COUNT_FLIP_2[(LODWORD(P) >> 8) & 0xff];
	n_flipped += COUNT_FLIP_2[(((LODWORD(P) & 0x110A0400u) + (HIDWORD(P) & 0x00804020u)) * 0x01010101u) >> 24];

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
	int n_flipped;

	n_flipped  = COUNT_FLIP_R[(((LODWORD(P) & 0x08080000u) + ((HIDWORD(P) & 0x08080808u) << 4)) * 0x00204081u) >> 26];
	n_flipped += COUNT_FLIP_3[(LODWORD(P) >> 8) & 0xff];
	n_flipped += COUNT_FLIP_3[(((LODWORD(P) & 0x22140800u) + (HIDWORD(P) & 0x00008041u)) * 0x01010101u) >> 24];

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
	int n_flipped;

	n_flipped  = COUNT_FLIP_R[((((LODWORD(P) & 0x10100000u) >> 4) + (HIDWORD(P) & 0x10101010u)) * 0x01020408u) >> 26];
	n_flipped += COUNT_FLIP_4[(LODWORD(P) >> 8) & 0xff];
	n_flipped += COUNT_FLIP_4[(((LODWORD(P) & 0x44281000u) + (HIDWORD(P) & 0x00000182u)) * 0x01010101u) >> 24];

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
	int n_flipped;

	n_flipped  = COUNT_FLIP_R[(((HIDWORD(P) & 0x20202020u) + ((LODWORD(P) & 0x20200000u) >> 4)) * 0x00810204u) >> 26];
	n_flipped += COUNT_FLIP_5[(LODWORD(P) >> 8) & 0xff];
	n_flipped += COUNT_FLIP_5[(((LODWORD(P) & 0x88502000u) + (HIDWORD(P) & 0x00010204u)) * 0x01010101u) >> 24];

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
	int n_flipped;

	n_flipped  = COUNT_FLIP_R[((((LODWORD(P) & 0x40400000u) >> 4) + (HIDWORD(P) & 0x40404040u)) * 0x00408102u) >> 26];
	n_flipped += COUNT_FLIP_L[(LODWORD(P) >> 7) & 0x7e];
	n_flipped += COUNT_FLIP_L[(((LODWORD(P) & 0x10200000u) + (HIDWORD(P) & 0x01020408u)) * 0x02020202u) >> 24];

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
	int n_flipped;

	n_flipped  = COUNT_FLIP_R[((((LODWORD(P) & 0x80800000u) >> 4) + (HIDWORD(P) & 0x80808080u)) * 0x00204081u) >> 26];
	n_flipped += COUNT_FLIP_L[(LODWORD(P) >> 8) & 0x7f];
	n_flipped += COUNT_FLIP_L[(((LODWORD(P) & 0x20400000u) + (HIDWORD(P) & 0x02040810u)) * 0x01010101u) >> 24];

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
	int n_flipped;

	n_flipped  = COUNT_FLIP_2[((LODWORD(P) & 0x02010101u) * 0x01020404u + (HIDWORD(P) & 0x20100804u) * 0x04040404u) >> 24];	// A1A3F8
	n_flipped += COUNT_FLIP_R[(LODWORD(P) >> 17) & 0x7f];
	n_flipped += COUNT_FLIP_5[((LODWORD(P) & 0x01010204u) * 0x20202010u + (HIDWORD(P) & 0x01010101u) * 0x08040201u) >> 24];	// C1A3A8

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
	int n_flipped;

	n_flipped  = COUNT_FLIP_2[((LODWORD(P) & 0x04020202u) * 0x00810202u + (HIDWORD(P) & 0x40201008u) * 0x02020202u) >> 24];	// B1B3G8
	n_flipped += COUNT_FLIP_R[(LODWORD(P) >> 18) & 0x3f];
	n_flipped += COUNT_FLIP_5[((LODWORD(P) & 0x02020408u) * 0x10101008u + ((HIDWORD(P) & 0x02020202u) >> 1) * 0x08040201u) >> 24];	// D1B3B8

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
	int n_flipped;

	n_flipped  = COUNT_FLIP_2[(((LODWORD(P) & 0x04040404u) + ((HIDWORD(P) & 0x04040404u) << 4)) * 0x00408102u) >> 24];
	n_flipped += COUNT_FLIP_2[(LODWORD(P) >> 16) & 0xff];
	n_flipped += COUNT_FLIP_2[(((LODWORD(P) & 0x02040810u) + (HIDWORD(P) & 0x00000001u)) * 0x01010101u) >> 24];
	n_flipped += COUNT_FLIP_2[(((LODWORD(P) & 0x08040201u) + (HIDWORD(P) & 0x80402010u)) * 0x01010101u) >> 24];

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
	int n_flipped;

	n_flipped  = COUNT_FLIP_2[(((LODWORD(P) & 0x08080808u) + ((HIDWORD(P) & 0x08080808u) << 4)) * 0x00204081u) >> 24];
	n_flipped += COUNT_FLIP_3[(LODWORD(P) >> 16) & 0xff];
	n_flipped += COUNT_FLIP_3[(((LODWORD(P) & 0x04081020u) + (HIDWORD(P) & 0x00000102u)) * 0x01010101u) >> 24];
	n_flipped += COUNT_FLIP_3[(((LODWORD(P) & 0x10080402u) + (HIDWORD(P) & 0x00804020u)) * 0x01010101u) >> 24];

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
	int n_flipped;

	n_flipped  = COUNT_FLIP_2[((((LODWORD(P) & 0x10101010u) >> 4) + (HIDWORD(P) & 0x10101010u)) * 0x01020408u) >> 24];
	n_flipped += COUNT_FLIP_4[(LODWORD(P) >> 16) & 0xff];
	n_flipped += COUNT_FLIP_4[(((LODWORD(P) & 0x08102040u) + (HIDWORD(P) & 0x00010204u)) * 0x01010101u) >> 24];
	n_flipped += COUNT_FLIP_4[(((LODWORD(P) & 0x20100804u) + (HIDWORD(P) & 0x00008040u)) * 0x01010101u) >> 24];

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
	int n_flipped;

	n_flipped  = COUNT_FLIP_2[(((HIDWORD(P) & 0x20202020u) + ((LODWORD(P) & 0x20202020u) >> 4)) * 0x00810204u) >> 24];
	n_flipped += COUNT_FLIP_5[(LODWORD(P) >> 16) & 0xff];
	n_flipped += COUNT_FLIP_5[(((LODWORD(P) & 0x10204080u) + (HIDWORD(P) & 0x01020408u)) * 0x01010101u) >> 24];
	n_flipped += COUNT_FLIP_5[(((LODWORD(P) & 0x40201008u) + (HIDWORD(P) & 0x00000080u)) * 0x01010101u) >> 24];

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
	int n_flipped;

	n_flipped  = COUNT_FLIP_2[(((LODWORD(P) & 0x40402010u) >> 4) * 0x01010102u + (HIDWORD(P) & 0x40404040u) * 0x00408102u) >> 24];	// E1G3G8
	n_flipped += COUNT_FLIP_L[(LODWORD(P) >> 15) & 0x7e];
	n_flipped += COUNT_FLIP_5[(((LODWORD(P) & 0x20404040u) >> 1) * 0x04020101u + ((HIDWORD(P) & 0x02040810u) >> 1) * 0x01010101u) >> 24];	// G1G3B8

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
	int n_flipped;

	n_flipped  = COUNT_FLIP_2[(((LODWORD(P) & 0x80804020u) >> 4) * 0x00808081u + (HIDWORD(P) & 0x80808080u) * 0x00204081u) >> 24];	// F1H3H8
	n_flipped += COUNT_FLIP_L[(LODWORD(P) >> 16) & 0x7f];
	n_flipped += COUNT_FLIP_5[(((LODWORD(P) & 0x40808080u) >> 2) * 0x04020101u + ((HIDWORD(P) & 0x04081020u) >> 2) * 0x01010101u) >> 24];	// H1H3C8

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
	int n_flipped;

	n_flipped  = COUNT_FLIP_3[((LODWORD(P) & 0x01010101u) * 0x01020408u + (HIDWORD(P) & 0x10080402u) * 0x08080808u) >> 24];	// A1A4E8
	n_flipped += COUNT_FLIP_R[LODWORD(P) >> 25];
	n_flipped += COUNT_FLIP_4[((LODWORD(P) & 0x01020408u) * 0x10101010u + (HIDWORD(P) & 0x01010101u) * 0x08040201u) >> 24];	// D1A4A8

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
	int n_flipped;

	n_flipped  = COUNT_FLIP_3[((LODWORD(P) & 0x02020202u) * 0x00810204u + (HIDWORD(P) & 0x20100804u) * 0x04040404u) >> 24];	// B1B4F8
	n_flipped += COUNT_FLIP_R[LODWORD(P) >> 26];
	n_flipped += COUNT_FLIP_4[((LODWORD(P) & 0x02040810u) * 0x08080808u + ((HIDWORD(P) & 0x02020202u) >> 1) * 0x08040201u) >> 24];	// E1B4B8

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
	int n_flipped;

	n_flipped  = COUNT_FLIP_3[(((LODWORD(P) & 0x04040404u) + ((HIDWORD(P) & 0x04040404u) << 4)) * 0x00408102u) >> 24];
	n_flipped += COUNT_FLIP_2[LODWORD(P) >> 24];
	n_flipped += COUNT_FLIP_2[(((LODWORD(P) & 0x04081020u) + (HIDWORD(P) & 0x00000102u)) * 0x01010101u) >> 24];
	n_flipped += COUNT_FLIP_2[(((LODWORD(P) & 0x04020100u) + (HIDWORD(P) & 0x40201008u)) * 0x01010101u) >> 24];

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
	int n_flipped;

	n_flipped  = COUNT_FLIP_3[(((LODWORD(P) & 0x08080808u) + ((HIDWORD(P) & 0x08080808u) << 4)) * 0x00204081u) >> 24];
	n_flipped += COUNT_FLIP_3[LODWORD(P) >> 24];
	n_flipped += COUNT_FLIP_3[(((LODWORD(P) & 0x08102040u) + (HIDWORD(P) & 0x00010204u)) * 0x01010101u) >> 24];
	n_flipped += COUNT_FLIP_3[(((LODWORD(P) & 0x08040201u) + (HIDWORD(P) & 0x80402010u)) * 0x01010101u) >> 24];

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
	int n_flipped;

	n_flipped  = COUNT_FLIP_3[((((LODWORD(P) & 0x10101010u) >> 4) + (HIDWORD(P) & 0x10101010u)) * 0x01020408u) >> 24];
	n_flipped += COUNT_FLIP_4[LODWORD(P) >> 24];
	n_flipped += COUNT_FLIP_4[(((LODWORD(P) & 0x10204080u) + (HIDWORD(P) & 0x01020408u)) * 0x01010101u) >> 24];
	n_flipped += COUNT_FLIP_4[(((LODWORD(P) & 0x10080402u) + (HIDWORD(P) & 0x00804020u)) * 0x01010101u) >> 24];

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
	int n_flipped;

	n_flipped  = COUNT_FLIP_3[(((HIDWORD(P) & 0x20202020u) + ((LODWORD(P) & 0x20202020u) >> 4)) * 0x00810204u) >> 24];
	n_flipped += COUNT_FLIP_5[LODWORD(P) >> 24];
	n_flipped += COUNT_FLIP_5[(((LODWORD(P) & 0x20408000u) + (HIDWORD(P) & 0x02040810u)) * 0x01010101u) >> 24];
	n_flipped += COUNT_FLIP_5[(((LODWORD(P) & 0x20100804u) + (HIDWORD(P) & 0x00008040u)) * 0x01010101u) >> 24];

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
	int n_flipped;

	n_flipped  = COUNT_FLIP_3[(((LODWORD(P) & 0x40201008u) >> 3) * 0x01010101u + (HIDWORD(P) & 0x40404040u) * 0x00408102u) >> 24];	// D1G4G8
	n_flipped += COUNT_FLIP_L[(LODWORD(P) >> 23) & 0x7e];
	n_flipped += COUNT_FLIP_4[(((LODWORD(P) & 0x40404040u) >> 2) * 0x08040201u + ((HIDWORD(P) & 0x04081020u) >> 2) * 0x01010101u) >> 24];	// G1G4C8

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
	int n_flipped;

	n_flipped  = COUNT_FLIP_3[(((LODWORD(P) & 0x80402010u) >> 4) * 0x01010101u + (HIDWORD(P) & 0x80808080u) * 0x00204081u) >> 24];	// E1H4H8
	n_flipped += COUNT_FLIP_L[(LODWORD(P) >> 24) & 0x7f];
	n_flipped += COUNT_FLIP_4[(((LODWORD(P) & 0x80808080u) >> 3) * 0x08040201u + ((HIDWORD(P) & 0x08102040u) >> 3) * 0x01010101u) >> 24];	// H1H4D8

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
	int n_flipped;

	n_flipped  = COUNT_FLIP_4[((LODWORD(P) & 0x01010101u) * 0x01020408u + (HIDWORD(P) & 0x08040201u) * 0x10101010u) >> 24];	// A1A5D8
	n_flipped += COUNT_FLIP_R[(HIDWORD(P) >> 1) & 0x7f];
	n_flipped += COUNT_FLIP_3[((LODWORD(P) & 0x02040810u) * 0x08080808u + (HIDWORD(P) & 0x01010101u) * 0x08040201u) >> 24];	// E1A5A8

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
	int n_flipped;

	n_flipped  = COUNT_FLIP_4[((LODWORD(P) & 0x02020202u) * 0x00810204u + (HIDWORD(P) & 0x10080402u) * 0x08080808u) >> 24];	// B1B5E8
	n_flipped += COUNT_FLIP_R[(HIDWORD(P) >> 2) & 0x3f];
	n_flipped += COUNT_FLIP_3[((LODWORD(P) & 0x04081020u) * 0x04040404u + ((HIDWORD(P) & 0x02020202u) >> 1) * 0x08040201u) >> 24];	// F1B5B8

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
	int n_flipped;

	n_flipped  = COUNT_FLIP_4[(((LODWORD(P) & 0x04040404u) + ((HIDWORD(P) & 0x04040404u) << 4)) * 0x00408102u) >> 24];
	n_flipped += COUNT_FLIP_2[HIDWORD(P) & 0xff];
	n_flipped += COUNT_FLIP_2[(((LODWORD(P) & 0x08102040u) + (HIDWORD(P) & 0x00010204u)) * 0x01010101u) >> 24];
	n_flipped += COUNT_FLIP_2[(((LODWORD(P) & 0x02010000u) + (HIDWORD(P) & 0x20100804u)) * 0x01010101u) >> 24];

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
	int n_flipped;

	n_flipped  = COUNT_FLIP_4[(((LODWORD(P) & 0x08080808u) + ((HIDWORD(P) & 0x08080808u) << 4)) * 0x00204081u) >> 24];
	n_flipped += COUNT_FLIP_3[HIDWORD(P) & 0xff];
	n_flipped += COUNT_FLIP_3[(((LODWORD(P) & 0x10204080u) + (HIDWORD(P) & 0x01020408u)) * 0x01010101u) >> 24];
	n_flipped += COUNT_FLIP_3[(((LODWORD(P) & 0x04020100u) + (HIDWORD(P) & 0x40201008u)) * 0x01010101u) >> 24];

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
	int n_flipped;

	n_flipped  = COUNT_FLIP_4[((((LODWORD(P) & 0x10101010u) >> 4) + (HIDWORD(P) & 0x10101010u)) * 0x01020408u) >> 24];
	n_flipped += COUNT_FLIP_4[HIDWORD(P) & 0xff];
	n_flipped += COUNT_FLIP_4[(((LODWORD(P) & 0x20408000u) + (HIDWORD(P) & 0x02040810u)) * 0x01010101u) >> 24];
	n_flipped += COUNT_FLIP_4[(((LODWORD(P) & 0x08040201u) + (HIDWORD(P) & 0x80402010u)) * 0x01010101u) >> 24];

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
	int n_flipped;

	n_flipped  = COUNT_FLIP_4[(((HIDWORD(P) & 0x20202020u) + ((LODWORD(P) & 0x20202020u) >> 4)) * 0x00810204u) >> 24];
	n_flipped += COUNT_FLIP_5[HIDWORD(P) & 0xff];
	n_flipped += COUNT_FLIP_5[(((LODWORD(P) & 0x40800000u) + (HIDWORD(P) & 0x04081020u)) * 0x01010101u) >> 24];
	n_flipped += COUNT_FLIP_5[(((LODWORD(P) & 0x10080402u) + (HIDWORD(P) & 0x00804020u)) * 0x01010101u) >> 24];

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
	int n_flipped;

	n_flipped  = COUNT_FLIP_4[(((LODWORD(P) & 0x20100804u) >> 2) * 0x01010101u + (HIDWORD(P) & 0x40404040u) * 0x00408102u) >> 24];	// C1G5G8
	n_flipped += COUNT_FLIP_L[(HIDWORD(P) << 1) & 0x7e];
	n_flipped += COUNT_FLIP_3[(((LODWORD(P) & 0x40404040u) >> 3) * 0x10080402u + ((HIDWORD(P) & 0x08102040u) >> 3) * 0x01010101u) >> 24];	// G1G5D8

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
	int n_flipped;

	n_flipped  = COUNT_FLIP_4[(((LODWORD(P) & 0x40201008u) >> 3) * 0x01010101u + (HIDWORD(P) & 0x80808080u) * 0x00204081u) >> 24];	// D1H5H8
	n_flipped += COUNT_FLIP_L[HIDWORD(P) & 0x7f];
	n_flipped += COUNT_FLIP_3[(((LODWORD(P) & 0x80808080u) >> 4) * 0x10080402u + ((HIDWORD(P) & 0x10204080u) >> 4) * 0x01010101u) >> 24];	// H1H5E8

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
	int n_flipped;

	n_flipped  = COUNT_FLIP_5[((LODWORD(P) & 0x01010101u) * 0x01020408u + (HIDWORD(P) & 0x04020101u) * 0x10202020u) >> 24];	// A1A6C8
	n_flipped += COUNT_FLIP_R[(HIDWORD(P) >> 9) & 0x7f];
	n_flipped += COUNT_FLIP_2[((LODWORD(P) & 0x04081020u) * 0x04040404u + (HIDWORD(P) & 0x01010102u) * 0x04040201u) >> 24];	// F1A6A8

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
	int n_flipped;

	n_flipped  = COUNT_FLIP_5[((LODWORD(P) & 0x02020202u) * 0x00810204u + (HIDWORD(P) & 0x08040202u) * 0x08101010u) >> 24];	// B1B6D8
	n_flipped += COUNT_FLIP_R[(HIDWORD(P) >> 10) & 0x3f];
	n_flipped += COUNT_FLIP_2[((LODWORD(P) & 0x08102040u) * 0x02020202u + ((HIDWORD(P) & 0x02020204u) >> 1) * 0x04040201u) >> 24];	// G1B6B8

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
	int n_flipped;

	n_flipped  = COUNT_FLIP_5[(((LODWORD(P) & 0x04040404u) + ((HIDWORD(P) & 0x04040404u) << 4)) * 0x00408102u) >> 24];
	n_flipped += COUNT_FLIP_2[(HIDWORD(P) >> 8) & 0xff];
	n_flipped += COUNT_FLIP_2[(((LODWORD(P) & 0x10204080u) + (HIDWORD(P) & 0x01020408u)) * 0x01010101u) >> 24];
	n_flipped += COUNT_FLIP_2[(((LODWORD(P) & 0x01000000u) + (HIDWORD(P) & 0x10080402u)) * 0x01010101u) >> 24];

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
	int n_flipped;

	n_flipped  = COUNT_FLIP_5[(((LODWORD(P) & 0x08080808u) + ((HIDWORD(P) & 0x08080808u) << 4)) * 0x00204081u) >> 24];
	n_flipped += COUNT_FLIP_3[(HIDWORD(P) >> 8) & 0xff];
	n_flipped += COUNT_FLIP_3[(((LODWORD(P) & 0x20408000u) + (HIDWORD(P) & 0x02040810u)) * 0x01010101u) >> 24];
	n_flipped += COUNT_FLIP_3[(((LODWORD(P) & 0x02010000u) + (HIDWORD(P) & 0x20100804u)) * 0x01010101u) >> 24];

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
	int n_flipped;

	n_flipped  = COUNT_FLIP_5[((((LODWORD(P) & 0x10101010u) >> 4) + (HIDWORD(P) & 0x10101010u)) * 0x01020408u) >> 24];
	n_flipped += COUNT_FLIP_4[(HIDWORD(P) >> 8) & 0xff];
	n_flipped += COUNT_FLIP_4[(((LODWORD(P) & 0x40800000u) + (HIDWORD(P) & 0x04081020u)) * 0x01010101u) >> 24];
	n_flipped += COUNT_FLIP_4[(((LODWORD(P) & 0x04020100u) + (HIDWORD(P) & 0x40201008u)) * 0x01010101u) >> 24];

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
	int n_flipped;

	n_flipped  = COUNT_FLIP_5[(((HIDWORD(P) & 0x20202020u) + ((LODWORD(P) & 0x20202020u) >> 4)) * 0x00810204u) >> 24];
	n_flipped += COUNT_FLIP_5[(HIDWORD(P) >> 8) & 0xff];
	n_flipped += COUNT_FLIP_5[(((LODWORD(P) & 0x80000000u) + (HIDWORD(P) & 0x08102040u)) * 0x01010101u) >> 24];
	n_flipped += COUNT_FLIP_5[(((LODWORD(P) & 0x08040201u) + (HIDWORD(P) & 0x80402010u)) * 0x01010101u) >> 24];

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
	int n_flipped;

	n_flipped  = COUNT_FLIP_5[(((LODWORD(P) & 0x10080402u) >> 1) * 0x01010101u + (HIDWORD(P) & 0x40404020u) * 0x00808102u) >> 24];	// B1G6G8
	n_flipped += COUNT_FLIP_L[(HIDWORD(P) >> 7) & 0x7e];
	n_flipped += COUNT_FLIP_2[(((LODWORD(P) & 0x40404040u) >> 4) * 0x20100804u + ((HIDWORD(P) & 0x10204040u) >> 4 ) * 0x02010101u) >> 24];	// G1G6E8

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
	int n_flipped;

	n_flipped  = COUNT_FLIP_5[(((LODWORD(P) & 0x20100804u) >> 2) * 0x01010101u + (HIDWORD(P) & 0x80808040u) * 0x00404081u) >> 24];	// C1H6H8
	n_flipped += COUNT_FLIP_L[(HIDWORD(P) >> 8) & 0x7f];
	n_flipped += COUNT_FLIP_2[(((LODWORD(P) & 0x80808080u) >> 5) * 0x20100804u + ((HIDWORD(P) & 0x20408080u) >> 5) * 0x02010101u) >> 24];	// H1H6F8

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
	int n_flipped;

	n_flipped  = COUNT_FLIP_L[((((HIDWORD(P) & 0x00000101u) << 4) + (LODWORD(P) & 0x01010101u)) * 0x02040810u) >> 24];
	n_flipped += COUNT_FLIP_R[(HIDWORD(P) >> 17) & 0x7f];
	n_flipped += COUNT_FLIP_R[(((HIDWORD(P) & 0x00000204u) + (LODWORD(P) & 0x08102040u)) * 0x01010101u) >> 25];

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
	int n_flipped;

	n_flipped  = COUNT_FLIP_L[((((HIDWORD(P) & 0x00000202u) << 4) + (LODWORD(P) & 0x02020202u)) * 0x01020408u) >> 24];
	n_flipped += COUNT_FLIP_R[(HIDWORD(P) >> 18) & 0x3f];
	n_flipped += COUNT_FLIP_R[(((HIDWORD(P) & 0x00000408u) + (LODWORD(P) & 0x10204080u)) * 0x01010101u) >> 26];

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
	int n_flipped;

	n_flipped  = COUNT_FLIP_L[(((LODWORD(P) & 0x04040404u) + ((HIDWORD(P) & 0x00000404u) << 4)) * 0x00810204u) >> 24];
	n_flipped += COUNT_FLIP_2[(HIDWORD(P) >> 16) & 0xff];
	n_flipped += COUNT_FLIP_2[(((HIDWORD(P) & 0x00040A11u) + (LODWORD(P) & 0x20408000u)) * 0x01010101u) >> 24];

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
	int n_flipped;

	n_flipped  = COUNT_FLIP_L[((((HIDWORD(P) & 0x00000808u) << 4) + (LODWORD(P) & 0x08080808u)) * 0x00408102u) >> 24];
	n_flipped += COUNT_FLIP_3[(HIDWORD(P) >> 16) & 0xff];
	n_flipped += COUNT_FLIP_3[(((HIDWORD(P) & 0x00081422u) + (LODWORD(P) & 0x41800000u)) * 0x01010101u) >> 24];

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
	int n_flipped;

	n_flipped  = COUNT_FLIP_L[(((HIDWORD(P) & 0x00001010u) + ((LODWORD(P) & 0x10101010u) >> 4)) * 0x02040810u) >> 24];
	n_flipped += COUNT_FLIP_4[(HIDWORD(P) >> 16) & 0xff];
	n_flipped += COUNT_FLIP_4[(((HIDWORD(P) & 0x00102844u) + (LODWORD(P) & 0x82010000u)) * 0x01010101u) >> 24];

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
	int n_flipped;

	n_flipped  = COUNT_FLIP_L[(((HIDWORD(P) & 0x00002020u) + ((LODWORD(P) & 0x20202020u) >> 4)) * 0x01020408u) >> 24];
	n_flipped += COUNT_FLIP_5[(HIDWORD(P) >> 16) & 0xff];
	n_flipped += COUNT_FLIP_5[(((HIDWORD(P) & 0x00205088u) + (LODWORD(P) & 0x04020100u)) * 0x01010101u) >> 24];

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
	int n_flipped;

	n_flipped  = COUNT_FLIP_L[(((HIDWORD(P) & 0x00004040u) + ((LODWORD(P) & 0x40404040u) >> 4)) * 0x00810204u) >> 24];
	n_flipped += COUNT_FLIP_L[(HIDWORD(P) >> 15) & 0x7e];
	n_flipped += COUNT_FLIP_L[(((HIDWORD(P) & 0x00002010u) + (LODWORD(P) & 0x08040201u)) * 0x02020202u) >> 24];

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
	int n_flipped;

	n_flipped  = COUNT_FLIP_L[(((HIDWORD(P) & 0x00008080u) + ((LODWORD(P) & 0x80808080u) >> 4)) * 0x00408102u) >> 24];
	n_flipped += COUNT_FLIP_L[(HIDWORD(P) >> 16) & 0x7f];
	n_flipped += COUNT_FLIP_L[(((HIDWORD(P) & 0x00004020u) + (LODWORD(P) & 0x10080402u)) * 0x01010101u) >> 24];

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
	int n_flipped;

	n_flipped  = COUNT_FLIP_L[((((HIDWORD(P) & 0x00010101u) << 4) + (LODWORD(P) & 0x01010101u)) * 0x01020408u) >> 24];
	n_flipped += COUNT_FLIP_R[HIDWORD(P) >> 25];
	n_flipped += COUNT_FLIP_R[(((HIDWORD(P) & 0x00020408u) + (LODWORD(P) & 0x10204080u)) * 0x01010101u) >> 25];

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
	int n_flipped;

	n_flipped  = COUNT_FLIP_L[((((HIDWORD(P) & 0x00020202u) << 4) + (LODWORD(P) & 0x02020202u)) * 0x00810204u) >> 24];
	n_flipped += COUNT_FLIP_R[HIDWORD(P) >> 26];
	n_flipped += COUNT_FLIP_R[(((HIDWORD(P) & 0x00040810u) + (LODWORD(P) & 0x20408000u)) * 0x01010101u) >> 26];

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
	int n_flipped;

	n_flipped  = COUNT_FLIP_L[(((LODWORD(P) & 0x04040404u) + ((HIDWORD(P) & 0x00040404u) << 4)) * 0x00408102u) >> 24];
	n_flipped += COUNT_FLIP_2[HIDWORD(P) >> 24];
	n_flipped += COUNT_FLIP_2[(((HIDWORD(P) & 0x040A1120u) + (LODWORD(P) & 0x40800000u)) * 0x01010101u) >> 24];

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
	int n_flipped;

	n_flipped  = COUNT_FLIP_L[((((HIDWORD(P) & 0x00080808u) << 4) + (LODWORD(P) & 0x08080808u)) * 0x00204081u) >> 24];
	n_flipped += COUNT_FLIP_3[HIDWORD(P) >> 24];
	n_flipped += COUNT_FLIP_3[(((HIDWORD(P) & 0x08142241u) + (LODWORD(P) & 0x80000000u)) * 0x01010101u) >> 24];

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
	int n_flipped;

	n_flipped  = COUNT_FLIP_L[(((HIDWORD(P) & 0x00101010u) + ((LODWORD(P) & 0x10101010u) >> 4)) * 0x01020408u) >> 24];
	n_flipped += COUNT_FLIP_4[HIDWORD(P) >> 24];
	n_flipped += COUNT_FLIP_4[(((HIDWORD(P) & 0x10284482u) + (LODWORD(P) & 0x01000000u)) * 0x01010101u) >> 24];

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
	int n_flipped;

	n_flipped  = COUNT_FLIP_L[(((HIDWORD(P) & 0x00202020u) + ((LODWORD(P) & 0x20202020u) >> 4)) * 0x00810204u) >> 24];
	n_flipped += COUNT_FLIP_5[HIDWORD(P) >> 24];
	n_flipped += COUNT_FLIP_5[(((HIDWORD(P) & 0x00508804u) + (LODWORD(P) & 0x02010000u)) * 0x01010101u) >> 24];

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
	int n_flipped;

	n_flipped  = COUNT_FLIP_L[(((HIDWORD(P) & 0x00404040u) + ((LODWORD(P) & 0x40404040u) >> 4)) * 0x00408102u) >> 24];
	n_flipped += COUNT_FLIP_L[(HIDWORD(P) >> 23) & 0x7e];
	n_flipped += COUNT_FLIP_L[(((HIDWORD(P) & 0x00201008u) + (LODWORD(P) & 0x04020100u)) * 0x02020202u) >> 24];

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
	int n_flipped;

	n_flipped  = COUNT_FLIP_L[(((HIDWORD(P) & 0x00808080u) + ((LODWORD(P) & 0x80808080) >> 4)) * 0x00204081u) >> 24];
	n_flipped += COUNT_FLIP_L[(HIDWORD(P) >> 24) & 0x7f];
	n_flipped += COUNT_FLIP_L[(((HIDWORD(P) & 0x00402010u) + (LODWORD(P) & 0x08040201u)) * 0x01010101u) >> 24];

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

