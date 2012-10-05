/**
 * @file flip_bitscan.c
 *
 * This module deals with flipping discs.
 *
 * A function is provided for each square of the board. These functions are
 * gathered into an array of functions, so that a fast access to each function
 * is allowed. The generic form of the function take as input the player and
 * the opponent bitboards and return the flipped squares into a bitboard.
 *
 * Given the following notation:
 *  - x = square where we play,
 *  - P = player's disc pattern,
 *  - O = opponent's disc pattern,
 * the basic principle is to read into an array the result of a move. Doing
 * this is easier for a single line ; so we can use arrays of the form:
 *  - ARRAY[x][8-bits disc pattern].
 * The problem is thus to convert any line of a 64-bits disc pattern into an
 * 8-bits disc pattern. A fast way to do this is to select the right line,
 * with a bit-mask, to gather the masked-bits into a continuous set by a simple
 * multiplication and to right-shift the result to scale it into a number
 * between 0 and 255.
 * Once we get our 8-bits disc patterns,a first array (OUTFLANK) is used to
 * get the player's discs that surround the opponent discs:
 *  - outflank = OUTFLANK[x][O] & P
 * (Only inner 6-bits of the P are in interest here.)
 * The result is then used as an index to access a second array giving the
 * flipped discs according to the surrounding player's discs:
 *  - flipped = FLIPPED[x][outflank].
 * (Flipped discs fall into inner 6-bits.)
 * Finally, a precomputed array transform the inner 6-bits disc pattern back into a
 * 64-bits disc pattern, and the flipped squares for each line are gathered and
 * returned to generate moves.
 *
 * If the OUTFLANK search is in LSB to MSB direction, carry propagation 
 * can be used to determine contiguous opponent discs.
 * If the OUTFLANK search is in MSB to LSB direction, GCC's __builtin_clz(ll)
 * is used to determine coutiguous opponent discs.
 *
 * @date 1998 - 2012
 * @author Richard Delorme
 * @author Toshihiko Okuhara
 * @version 4.3
 */

/** outflank array (indexed with inner 6 bits) */
/* static const unsigned char OUTFLANK_0[64] = {
	0x00, 0x04, 0x00, 0x08, 0x00, 0x04, 0x00, 0x10, 0x00, 0x04, 0x00, 0x08, 0x00, 0x04, 0x00, 0x20,
	0x00, 0x04, 0x00, 0x08, 0x00, 0x04, 0x00, 0x10, 0x00, 0x04, 0x00, 0x08, 0x00, 0x04, 0x00, 0x40,
	0x00, 0x04, 0x00, 0x08, 0x00, 0x04, 0x00, 0x10, 0x00, 0x04, 0x00, 0x08, 0x00, 0x04, 0x00, 0x20,
	0x00, 0x04, 0x00, 0x08, 0x00, 0x04, 0x00, 0x10, 0x00, 0x04, 0x00, 0x08, 0x00, 0x04, 0x00, 0x80
}; */

/* static const unsigned char OUTFLANK_1[64] = {
	0x00, 0x00, 0x08, 0x00, 0x00, 0x00, 0x10, 0x00, 0x00, 0x00, 0x08, 0x00, 0x00, 0x00, 0x20, 0x00,
	0x00, 0x00, 0x08, 0x00, 0x00, 0x00, 0x10, 0x00, 0x00, 0x00, 0x08, 0x00, 0x00, 0x00, 0x40, 0x00,
	0x00, 0x00, 0x08, 0x00, 0x00, 0x00, 0x10, 0x00, 0x00, 0x00, 0x08, 0x00, 0x00, 0x00, 0x20, 0x00,
	0x00, 0x00, 0x08, 0x00, 0x00, 0x00, 0x10, 0x00, 0x00, 0x00, 0x08, 0x00, 0x00, 0x00, 0x80, 0x00
}; */

static const unsigned char OUTFLANK_2[64] = {
	0x00, 0x01, 0x00, 0x00, 0x10, 0x11, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x20, 0x21, 0x00, 0x00,
	0x00, 0x01, 0x00, 0x00, 0x10, 0x11, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x40, 0x41, 0x00, 0x00,
	0x00, 0x01, 0x00, 0x00, 0x10, 0x11, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x20, 0x21, 0x00, 0x00,
	0x00, 0x01, 0x00, 0x00, 0x10, 0x11, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x80, 0x81, 0x00, 0x00
};

static const unsigned char OUTFLANK_3[64] = {
	0x00, 0x00, 0x02, 0x01, 0x00, 0x00, 0x00, 0x00, 0x20, 0x20, 0x22, 0x21, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x02, 0x01, 0x00, 0x00, 0x00, 0x00, 0x40, 0x40, 0x42, 0x41, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x02, 0x01, 0x00, 0x00, 0x00, 0x00, 0x20, 0x20, 0x22, 0x21, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x02, 0x01, 0x00, 0x00, 0x00, 0x00, 0x80, 0x80, 0x82, 0x81, 0x00, 0x00, 0x00, 0x00
};

static const unsigned char OUTFLANK_4[64] = {
	0x00, 0x00, 0x00, 0x00, 0x04, 0x04, 0x02, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x40, 0x40, 0x40, 0x40, 0x44, 0x44, 0x42, 0x41, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x04, 0x04, 0x02, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x80, 0x80, 0x80, 0x80, 0x84, 0x84, 0x82, 0x81, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};

static const unsigned char OUTFLANK_5[64] = {
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x08, 0x08, 0x08, 0x08, 0x04, 0x04, 0x02, 0x01,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x88, 0x88, 0x88, 0x88, 0x84, 0x84, 0x82, 0x81,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};

/* static const unsigned char OUTFLANK_6[64] = {
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x08, 0x08, 0x08, 0x08, 0x04, 0x04, 0x02, 0x01,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
}; */

static const unsigned char OUTFLANK_7[64] = {
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20,
	0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x08, 0x08, 0x08, 0x08, 0x04, 0x04, 0x02, 0x01
};

/* (Count of leading 1 from bit 5) * 8 */
static const unsigned char CONTIG_UP[64] = {
	 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
	 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
	 8,  8,  8,  8,  8,  8,  8,  8,  8,  8,  8,  8,  8,  8,  8,  8,
	16, 16, 16, 16, 16, 16, 16, 16, 24, 24, 24, 24, 32, 32, 40, 48
};

/* (Count of leading 1 from bit 5) * 9 */
static const unsigned char CONTIG_UPLEFT[64] = {
	 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
	 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
	 9,  9,  9,  9,  9,  9,  9,  9,  9,  9,  9,  9,  9,  9,  9,  9,
	18, 18, 18, 18, 18, 18, 18, 18, 27, 27, 27, 27, 36, 36, 45, 54
};

/* (Count of trailing 1) * 7 */
static const unsigned char CONTIG_UPRIGHT[64] = {
	 0,  7,  0, 14,  0,  7,  0, 21,  0,  7,  0, 14,  0,  7,  0, 28,
	 0,  7,  0, 14,  0,  7,  0, 21,  0,  7,  0, 14,  0,  7,  0, 35,
	 0,  7,  0, 14,  0,  7,  0, 21,  0,  7,  0, 14,  0,  7,  0, 28,
	 0,  7,  0, 14,  0,  7,  0, 21,  0,  7,  0, 14,  0,  7,  0, 42
};

/** flip array (indexed with outflank, returns inner 6 bits) */
static const unsigned char FLIPPED_2[130] = {
	0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x04, 0x05, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x0c, 0x0d, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x1c, 0x1d, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x3c, 0x3d
};

static const unsigned long long FLIPPED_3_H[131] = {
	0x0000000000000000ULL, 0x0606060606060606ULL, 0x0404040404040404ULL, 0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL,
	0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL,
	0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL,
	0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL,
	0x1010101010101010ULL, 0x1616161616161616ULL, 0x1414141414141414ULL, 0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL,
	0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL,
	0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL,
	0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL,
	0x3030303030303030ULL, 0x3636363636363636ULL, 0x3434343434343434ULL, 0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL,
	0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL,
	0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL,
	0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL,
	0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL,
	0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL,
	0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL,
	0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL,
	0x7070707070707070ULL, 0x7676767676767676ULL, 0x7474747474747474ULL
};

static const unsigned long long FLIPPED_4_H[133] = {
	0x0000000000000000ULL, 0x0e0e0e0e0e0e0e0eULL, 0x0c0c0c0c0c0c0c0cULL, 0x0000000000000000ULL, 0x0808080808080808ULL, 0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL,
	0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL,
	0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL,
	0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL,
	0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL,
	0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL,
	0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL,
	0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL,
	0x2020202020202020ULL, 0x2e2e2e2e2e2e2e2eULL, 0x2c2c2c2c2c2c2c2cULL, 0x0000000000000000ULL, 0x2828282828282828ULL, 0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL,
	0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL,
	0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL,
	0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL,
	0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL,
	0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL,
	0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL,
	0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL,
	0x6060606060606060ULL, 0x6e6e6e6e6e6e6e6eULL, 0x6c6c6c6c6c6c6c6cULL, 0x0000000000000000ULL, 0x6868686868686868ULL
};

static const unsigned char FLIPPED_5[137] = {
	0x00, 0x0f, 0x0e, 0x00, 0x0c, 0x00, 0x00, 0x00, 0x08, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x20, 0x2f, 0x2e, 0x00, 0x2c, 0x00, 0x00, 0x00, 0x28
};

static const unsigned long long FLIPPED_3_V[131] = {
	0x0000000000000000ULL, 0x0000000000ffff00ULL, 0x0000000000ff0000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL,
	0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL,
	0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL,
	0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL,
	0x000000ff00000000ULL, 0x000000ff00ffff00ULL, 0x000000ff00ff0000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL,
	0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL,
	0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL,
	0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL,
	0x0000ffff00000000ULL, 0x0000ffff00ffff00ULL, 0x0000ffff00ff0000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL,
	0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL,
	0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL,
	0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL,
	0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL,
	0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL,
	0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL,
	0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL,
	0x00ffffff00000000ULL, 0x00ffffff00ffff00ULL, 0x00ffffff00ff0000ULL
};

static const unsigned long long FLIPPED_4_V[133] = {
	0x0000000000000000ULL, 0x00000000ffffff00ULL, 0x00000000ffff0000ULL, 0x0000000000000000ULL, 0x00000000ff000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL,
	0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL,
	0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL,
	0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL,
	0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL,
	0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL,
	0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL,
	0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL,
	0x0000ff0000000000ULL, 0x0000ff00ffffff00ULL, 0x0000ff00ffff0000ULL, 0x0000000000000000ULL, 0x0000ff00ff000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL,
	0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL,
	0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL,
	0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL,
	0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL,
	0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL,
	0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL,
	0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL,
	0x00ffff0000000000ULL, 0x00ffff00ffffff00ULL, 0x00ffff00ffff0000ULL, 0x0000000000000000ULL, 0x00ffff00ff000000ULL
};


static const unsigned long long FLIPPED_3_U[131] = {
	0x0000000000000000ULL, 0x00ffff0000000000ULL, 0x0000ff0000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL,
	0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL,
	0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL,
	0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL,
	0x00000000ff000000ULL, 0x00ffff00ff000000ULL, 0x0000ff00ff000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL,
	0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL,
	0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL,
	0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL,
	0x00000000ffff0000ULL, 0x00ffff00ffff0000ULL, 0x0000ff00ffff0000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL,
	0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL,
	0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL,
	0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL,
	0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL,
	0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL,
	0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL,
	0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL,
	0x00000000ffffff00ULL, 0x00ffff00ffffff00ULL, 0x0000ff00ffffff00ULL
};


/**
 * Compute flipped discs when playing on square A1.
 *
 * @param P player's disc pattern.
 * @param O opponent's disc pattern.
 * @return flipped disc pattern.
 */
static unsigned long long flip_A1(const unsigned long long P, const unsigned long long O)
{
	unsigned long long flipped, outflank_v, outflank_h, outflank_d9;

	outflank_v = ((O | ~0x0101010101010100ULL) + 0x0000000000000100ULL) & P & 0x0101010101010100ULL;
	flipped = (outflank_v - (unsigned int) (outflank_v != 0)) & 0x0101010101010100ULL;

	outflank_h = ((O & 0x000000000000007eULL) + 0x0000000000000002ULL) & P;
	flipped |= (outflank_h - (unsigned int) (outflank_h != 0)) & 0x000000000000007eULL;

	outflank_d9 = ((O | ~0x8040201008040200ULL) + 0x0000000000000200ULL) & P & 0x8040201008040200ULL;
	flipped |= (outflank_d9 - (unsigned int) (outflank_d9 != 0)) & 0x8040201008040200ULL;

	return flipped;
}

/**
 * Compute flipped discs when playing on square B1.
 *
 * @param P player's disc pattern.
 * @param O opponent's disc pattern.
 * @return flipped disc pattern.
 */
static unsigned long long flip_B1(const unsigned long long P, const unsigned long long O)
{
	unsigned long long flipped, outflank_v, outflank_h, outflank_d9;

	outflank_v = ((O | ~0x0202020202020200ULL) + 0x0000000000000200ULL) & P & 0x0202020202020200ULL;
	flipped = (outflank_v - (unsigned int) (outflank_v != 0)) & 0x0202020202020200ULL;

	outflank_h = ((O & 0x000000000000007cULL) + 0x0000000000000004ULL) & P;
	flipped |= (outflank_h - (unsigned int) (outflank_h != 0)) & 0x000000000000007cULL;

	outflank_d9 = ((O | ~0x0080402010080400ULL) + 0x0000000000000400ULL) & P & 0x0080402010080400ULL;
	flipped |= (outflank_d9 - (unsigned int) (outflank_d9 != 0)) & 0x0080402010080400ULL;

	return flipped;
}

/**
 * Compute flipped discs when playing on square C1.
 *
 * @param P player's disc pattern.
 * @param O opponent's disc pattern.
 * @return flipped disc pattern.
 */
static unsigned long long flip_C1(const unsigned long long P, const unsigned long long O)
{
	unsigned int outflank_h;
	unsigned long long flipped, outflank_v, outflank_d9;

	outflank_v = ((O | ~0x0404040404040400ULL) + 0x0000000000000400ULL) & P & 0x0404040404040400ULL;
	flipped = (outflank_v - (unsigned int) (outflank_v != 0)) & 0x0404040404040400ULL;

	outflank_h = OUTFLANK_2[(O >> 1) & 0x3f] & P;
	flipped |= (FLIPPED_2[outflank_h] << 1);

	flipped |= ((P >> 7) & 0x0000000000000200ULL & O);

	outflank_d9 = ((O | ~0x0000804020100800ULL) + 0x0000000000000800ULL) & P & 0x0000804020100800ULL;
	flipped |= (outflank_d9 - (unsigned int) (outflank_d9 != 0)) & 0x0000804020100800ULL;

	return flipped;
}

/**
 * Compute flipped discs when playing on square D1.
 *
 * @param P player's disc pattern.
 * @param O opponent's disc pattern.
 * @return flipped disc pattern.
 */
static unsigned long long flip_D1(const unsigned long long P, const unsigned long long O)
{
	unsigned int outflank_h;
	unsigned long long flipped, outflank_v, outflank_d7, outflank_d9;

	outflank_v = ((O | ~0x0808080808080800ULL) + 0x0000000000000800ULL) & P & 0x0808080808080800ULL;
	flipped = (outflank_v - (unsigned int) (outflank_v != 0)) & 0x0808080808080800ULL;

	outflank_h = OUTFLANK_3[(O >> 1) & 0x3f] & P;
	flipped |= (unsigned char) FLIPPED_3_H[outflank_h];

	outflank_d7 = ((O | ~0x0000000001020400ULL) + 0x0000000000000400ULL) & P & 0x0000000001020400ULL;
	flipped |= (outflank_d7 - (unsigned int) (outflank_d7 != 0)) & 0x0000000001020400ULL;

	outflank_d9 = ((O | ~0x0000008040201000ULL) + 0x0000000000001000ULL) & P & 0x0000008040201000ULL;
	flipped |= (outflank_d9 - (unsigned int) (outflank_d9 != 0)) & 0x0000008040201000ULL;

	return flipped;
}

/**
 * Compute flipped discs when playing on square E1.
 *
 * @param P player's disc pattern.
 * @param O opponent's disc pattern.
 * @return flipped disc pattern.
 */
static unsigned long long flip_E1(const unsigned long long P, const unsigned long long O)
{
	unsigned int outflank_h;
	unsigned long long flipped, outflank_v, outflank_d7, outflank_d9;

	outflank_v = ((O | ~0x1010101010101000ULL) + 0x0000000000001000ULL) & P & 0x1010101010101000ULL;
	flipped = (outflank_v - (unsigned int) (outflank_v != 0)) & 0x1010101010101000ULL;

	outflank_h = OUTFLANK_4[(O >> 1) & 0x3f] & P;
	flipped |= (unsigned char) FLIPPED_4_H[outflank_h];

	outflank_d7 = ((O | ~0x0000000102040800ULL) + 0x0000000000000800ULL) & P & 0x0000000102040800ULL;
	flipped |= (outflank_d7 - (unsigned int) (outflank_d7 != 0)) & 0x0000000102040800ULL;

	outflank_d9 = ((O | ~0x0000000080402000ULL) + 0x0000000000002000ULL) & P & 0x0000000080402000ULL;
	flipped |= (outflank_d9 - (unsigned int) (outflank_d9 != 0)) & 0x0000000080402000ULL;

	return flipped;
}

/**
 * Compute flipped discs when playing on square F1.
 *
 * @param P player's disc pattern.
 * @param O opponent's disc pattern.
 * @return flipped disc pattern.
 */
static unsigned long long flip_F1(const unsigned long long P, const unsigned long long O)
{
	unsigned int outflank_h;
	unsigned long long flipped, outflank_v, outflank_d7;

	outflank_v = ((O | ~0x2020202020202000ULL) + 0x0000000000002000ULL) & P & 0x2020202020202000ULL;
	flipped = (outflank_v - (unsigned int) (outflank_v != 0)) & 0x2020202020202000ULL;

	outflank_h = OUTFLANK_5[(O >> 1) & 0x3f] & P;
	flipped |= (FLIPPED_5[outflank_h] << 1);

	outflank_d7 = ((O | ~0x0000010204081000ULL) + 0x0000000000001000ULL) & P & 0x0000010204081000ULL;
	flipped |= (outflank_d7 - (unsigned int) (outflank_d7 != 0)) & 0x0000010204081000ULL;

	flipped |= ((P >> 9) & 0x0000000000004000ULL & O);

	return flipped;
}

/**
 * Compute flipped discs when playing on square G1.
 *
 * @param P player's disc pattern.
 * @param O opponent's disc pattern.
 * @return flipped disc pattern.
 */
static unsigned long long flip_G1(const unsigned long long P, const unsigned long long O)
{
	unsigned int outflank_h;
	unsigned long long flipped, outflank_v, outflank_d7;

	outflank_v = ((O | ~0x4040404040404000ULL) + 0x0000000000004000ULL) & P & 0x4040404040404000ULL;
	flipped = (outflank_v - (unsigned int) (outflank_v != 0)) & 0x4040404040404000ULL;

	outflank_h = OUTFLANK_7[O & 0x3e] & (P << 1);
	flipped |= ((-outflank_h) & 0x3e) << 0;

	outflank_d7 = ((O | ~0x0001020408102000ULL) + 0x0000000000002000ULL) & P & 0x0001020408102000ULL;
	flipped |= (outflank_d7 - (unsigned int) (outflank_d7 != 0)) & 0x0001020408102000ULL;

	return flipped;
}

/**
 * Compute flipped discs when playing on square H1.
 *
 * @param P player's disc pattern.
 * @param O opponent's disc pattern.
 * @return flipped disc pattern.
 */
static unsigned long long flip_H1(const unsigned long long P, const unsigned long long O)
{
	unsigned int outflank_h;
	unsigned long long flipped, outflank_v, outflank_d7;

	outflank_v = ((O | ~0x8080808080808000ULL) + 0x0000000000008000ULL) & P & 0x8080808080808000ULL;
	flipped = (outflank_v - (unsigned int) (outflank_v != 0)) & 0x8080808080808000ULL;

	outflank_h = OUTFLANK_7[(O >> 1) & 0x3f] & P;
	flipped |= ((-outflank_h) & 0x3f) << 1;

	outflank_d7 = ((O | ~0x0102040810204000ULL) + 0x0000000000004000ULL) & P & 0x0102040810204000ULL;
	flipped |= (outflank_d7 - (unsigned int) (outflank_d7 != 0)) & 0x0102040810204000ULL;

	return flipped;
}

/**
 * Compute flipped discs when playing on square A2.
 *
 * @param P player's disc pattern.
 * @param O opponent's disc pattern.
 * @return flipped disc pattern.
 */
static unsigned long long flip_A2(const unsigned long long P, const unsigned long long O)
{
	unsigned int outflank_h;
	unsigned long long flipped, outflank_v, outflank_d9;

	outflank_v = ((O | ~0x0101010101010000ULL) + 0x0000000000010000ULL) & P & 0x0101010101010000ULL;
	flipped = (outflank_v - (unsigned int) (outflank_v != 0)) & 0x0101010101010000ULL;

	outflank_h = ((O & 0x0000000000007e00ULL) + 0x0000000000000200ULL) & P;
	flipped |= (outflank_h - (outflank_h >> 8)) & 0x0000000000007e00ULL;

	outflank_d9 = ((O | ~0x4020100804020000ULL) + 0x0000000000020000ULL) & P & 0x4020100804020000ULL;
	flipped |= (outflank_d9 - (unsigned int) (outflank_d9 != 0)) & 0x4020100804020000ULL;

	return flipped;
}

/**
 * Compute flipped discs when playing on square B2.
 *
 * @param P player's disc pattern.
 * @param O opponent's disc pattern.
 * @return flipped disc pattern.
 */
static unsigned long long flip_B2(const unsigned long long P, const unsigned long long O)
{
	unsigned int outflank_h;
	unsigned long long flipped, outflank_v, outflank_d9;

	outflank_v = ((O | ~0x0202020202020000ULL) + 0x0000000000020000ULL) & P & 0x0202020202020000ULL;
	flipped = (outflank_v - (unsigned int) (outflank_v != 0)) & 0x0202020202020000ULL;

	outflank_h = ((O & 0x0000000000007c00ULL) + 0x0000000000000400ULL) & P;
	flipped |= (outflank_h - (outflank_h >> 8)) & 0x0000000000007c00ULL;

	outflank_d9 = ((O | ~0x8040201008040000ULL) + 0x0000000000040000ULL) & P & 0x8040201008040000ULL;
	flipped |= (outflank_d9 - (unsigned int) (outflank_d9 != 0)) & 0x8040201008040000ULL;

	return flipped;
}

/**
 * Compute flipped discs when playing on square C2.
 *
 * @param P player's disc pattern.
 * @param O opponent's disc pattern.
 * @return flipped disc pattern.
 */
static unsigned long long flip_C2(const unsigned long long P, const unsigned long long O)
{
	unsigned int outflank_h;
	unsigned long long flipped, outflank_v, outflank_d9;

	outflank_v = ((O | ~0x0404040404040000ULL) + 0x0000000000040000ULL) & P & 0x0404040404040000ULL;
	flipped = (outflank_v - (unsigned int) (outflank_v != 0)) & 0x0404040404040000ULL;

	outflank_h = OUTFLANK_2[(O >> 9) & 0x3f] & (P >> 8);
	flipped |= (FLIPPED_2[outflank_h] << 9);

	flipped |= ((P >> 7) & 0x0000000000020000ULL & O);

	outflank_d9 = ((O | ~0x0080402010080000ULL) + 0x0000000000080000ULL) & P & 0x0080402010080400ULL;
	flipped |= (outflank_d9 - (unsigned int) (outflank_d9 != 0)) & 0x0080402010080000ULL;

	return flipped;
}

/**
 * Compute flipped discs when playing on square D2.
 *
 * @param P player's disc pattern.
 * @param O opponent's disc pattern.
 * @return flipped disc pattern.
 */
static unsigned long long flip_D2(const unsigned long long P, const unsigned long long O)
{
	unsigned int outflank_h;
	unsigned long long flipped, outflank_v, outflank_d7, outflank_d9;

	outflank_v = ((O | ~0x0808080808080000ULL) + 0x0000000000080000ULL) & P & 0x0808080808080000ULL;
	flipped = (outflank_v - (unsigned int) (outflank_v != 0)) & 0x0808080808080000ULL;

	outflank_h = OUTFLANK_3[(O >> 9) & 0x3f] & (P >> 8);
	flipped |= FLIPPED_3_H[outflank_h] & 0x000000000000ff00ULL;

	outflank_d7 = ((O | ~0x0000000102040000ULL) + 0x0000000000040000ULL) & P & 0x0000000102040000ULL;
	flipped |= (outflank_d7 - (unsigned int) (outflank_d7 != 0)) & 0x0000000102040000ULL;

	outflank_d9 = ((O | ~0x0000804020100000ULL) + 0x0000000000100000ULL) & P & 0x0000804020100000ULL;
	flipped |= (outflank_d9 - (unsigned int) (outflank_d9 != 0)) & 0x0000804020100000ULL;

	return flipped;
}

/**
 * Compute flipped discs when playing on square E2.
 *
 * @param P player's disc pattern.
 * @param O opponent's disc pattern.
 * @return flipped disc pattern.
 */
static unsigned long long flip_E2(const unsigned long long P, const unsigned long long O)
{
	unsigned int outflank_h;
	unsigned long long flipped, outflank_v, outflank_d7, outflank_d9;

	outflank_v = ((O | ~0x1010101010100000ULL) + 0x0000000000100000ULL) & P & 0x1010101010100000ULL;
	flipped = (outflank_v - (unsigned int) (outflank_v != 0)) & 0x1010101010100000ULL;

	outflank_h = OUTFLANK_4[(O >> 9) & 0x3f] & (P >> 8);
	flipped |= FLIPPED_4_H[outflank_h] & 0x000000000000ff00ULL;

	outflank_d7 = ((O | ~0x0000010204080000ULL) + 0x0000000000080000ULL) & P & 0x0000010204080000ULL;
	flipped |= (outflank_d7 - (unsigned int) (outflank_d7 != 0)) & 0x0000010204080000ULL;

	outflank_d9 = ((O | ~0x0000008040200000ULL) + 0x0000000000200000ULL) & P & 0x0000008040200000ULL;
	flipped |= (outflank_d9 - (unsigned int) (outflank_d9 != 0)) & 0x0000008040200000ULL;

	return flipped;
}

/**
 * Compute flipped discs when playing on square F2.
 *
 * @param P player's disc pattern.
 * @param O opponent's disc pattern.
 * @return flipped disc pattern.
 */
static unsigned long long flip_F2(const unsigned long long P, const unsigned long long O)
{
	unsigned int outflank_h;
	unsigned long long flipped, outflank_v, outflank_d7;

	outflank_v = ((O | ~0x2020202020200000ULL) + 0x0000000000200000ULL) & P & 0x2020202020200000ULL;
	flipped = (outflank_v - (unsigned int) (outflank_v != 0)) & 0x2020202020200000ULL;

	outflank_h = OUTFLANK_5[(O >> 9) & 0x3f] & (P >> 8);
	flipped |= (FLIPPED_5[outflank_h] << 9);

	outflank_d7 = ((O | ~0x0001020408100000ULL) + 0x0000000000100000ULL) & P & 0x0001020408100000ULL;
	flipped |= (outflank_d7 - (unsigned int) (outflank_d7 != 0)) & 0x0001020408100000ULL;

	flipped |= ((P >> 9) & 0x0000000000400000ULL & O);

	return flipped;
}

/**
 * Compute flipped discs when playing on square G2.
 *
 * @param P player's disc pattern.
 * @param O opponent's disc pattern.
 * @return flipped disc pattern.
 */
static unsigned long long flip_G2(const unsigned long long P, const unsigned long long O)
{
	unsigned int outflank_h;
	unsigned long long flipped, outflank_v, outflank_d7;

	outflank_v = ((O | ~0x4040404040400000ULL) + 0x0000000000400000ULL) & P & 0x4040404040400000ULL;
	flipped = (outflank_v - (unsigned int) (outflank_v != 0)) & 0x4040404040400000ULL;

	outflank_h = OUTFLANK_7[(O >> 8) & 0x3e] & (P >> 7);
	flipped |= ((-outflank_h) & 0x3e) << 8;

	outflank_d7 = ((O | ~0x0102040810200000ULL) + 0x0000000000200000ULL) & P & 0x0102040810200000ULL;
	flipped |= (outflank_d7 - (unsigned int) (outflank_d7 != 0)) & 0x0102040810200000ULL;

	return flipped;
}

/**
 * Compute flipped discs when playing on square H2.
 *
 * @param P player's disc pattern.
 * @param O opponent's disc pattern.
 * @return flipped disc pattern.
 */
static unsigned long long flip_H2(const unsigned long long P, const unsigned long long O)
{
	unsigned int outflank_h;
	unsigned long long flipped, outflank_v, outflank_d7;

	outflank_v = ((O | ~0x8080808080800000ULL) + 0x0000000000800000ULL) & P & 0x8080808080800000ULL;
	flipped = (outflank_v - (unsigned int) (outflank_v != 0)) & 0x8080808080800000ULL;

	outflank_h = OUTFLANK_7[(O >> 9) & 0x3f] & (P >> 8);
	flipped |= ((-outflank_h) & 0x3f) << 9;

	outflank_d7 = ((O | ~0x0204081020400000ULL) + 0x0000000000400000ULL) & P & 0x0204081020400000ULL;
	flipped |= (outflank_d7 - (unsigned int) (outflank_d7 != 0)) & 0x0204081020400000ULL;

	return flipped;
}

/**
 * Compute flipped discs when playing on square A3.
 *
 * @param P player's disc pattern.
 * @param O opponent's disc pattern.
 * @return flipped disc pattern.
 */
static unsigned long long flip_A3(const unsigned long long P, const unsigned long long O)
{
	unsigned long long flipped, outflank_v, outflank_h, outflank_d9;

	outflank_v = ((O | ~0x0101010101000000ULL) + 0x0000000001000000ULL) & P & 0x0101010101000000ULL;
	flipped = (outflank_v - (unsigned int) (outflank_v != 0)) & 0x0101010101000000ULL;

	outflank_h = ((O & 0x00000000007e0000ULL) + 0x0000000000020000ULL) & P;
	flipped |= (outflank_h - (outflank_h >> 8)) & 0x00000000007e0000ULL;

	outflank_d9 = ((O | ~0x2010080402000000ULL) + 0x0000000002000000ULL) & P & 0x2010080402000000ULL;
	flipped |= (outflank_d9 - (unsigned int) (outflank_d9 != 0)) & 0x2010080402000000ULL;

	flipped |= (((P << 8) & 0x0000000000000100ULL) | ((P << 7) & 0x0000000000000200ULL)) & O;

	return flipped;
}

/**
 * Compute flipped discs when playing on square B3.
 *
 * @param P player's disc pattern.
 * @param O opponent's disc pattern.
 * @return flipped disc pattern.
 */
static unsigned long long flip_B3(const unsigned long long P, const unsigned long long O)
{
	unsigned long long flipped, outflank_v, outflank_h, outflank_d9;

	outflank_v = ((O | ~0x0202020202000000ULL) + 0x0000000002000000ULL) & P & 0x0202020202000000ULL;
	flipped = (outflank_v - (unsigned int) (outflank_v != 0)) & 0x0202020202000000ULL;

	outflank_h = ((O & 0x00000000007c0000ULL) + 0x0000000000040000ULL) & P;
	flipped |= (outflank_h - (outflank_h >> 8)) & 0x00000000007c0000ULL;

	outflank_d9 = ((O | ~0x4020100804000000ULL) + 0x0000000004000000ULL) & P & 0x4020100804000000ULL;
	flipped |= (outflank_d9 - (unsigned int) (outflank_d9 != 0)) & 0x4020100804000000ULL;

	flipped |= (((P << 8) & 0x0000000000000200ULL) | ((P << 7) & 0x0000000000000400ULL)) & O;

	return flipped;
}

/**
 * Compute flipped discs when playing on square C3.
 *
 * @param P player's disc pattern.
 * @param O opponent's disc pattern.
 * @return flipped disc pattern.
 */
static unsigned long long flip_C3(const unsigned long long P, const unsigned long long O)
{
	unsigned int outflank_h;
	unsigned long long flipped, outflank_v, outflank_d9;

	outflank_v = ((O | ~0x0404040404000000ULL) + 0x0000000004000000ULL) & P & 0x0404040404000000ULL;
	flipped = (outflank_v - (unsigned int) (outflank_v != 0)) & 0x0404040404000000ULL;

	outflank_h = OUTFLANK_2[(O >> 17) & 0x3f] & (P >> 16);
	flipped |= (FLIPPED_2[outflank_h] << 17);

	outflank_d9 = ((O | ~0x8040201008000000ULL) + 0x0000000008000000ULL) & P & 0x8040201008000000ULL;
	flipped |= (outflank_d9 - (unsigned int) (outflank_d9 != 0)) & 0x8040201008000000ULL;

	flipped |= (((P << 8) & 0x0000000000000400ULL) | ((P << 9) & 0x0000000000000200ULL) | (((P >> 7) | (P << 7)) & 0x000000002000800ULL)) & O;

	return flipped;
}

/**
 * Compute flipped discs when playing on square D3.
 *
 * @param P player's disc pattern.
 * @param O opponent's disc pattern.
 * @return flipped disc pattern.
 */
static unsigned long long flip_D3(const unsigned long long P, const unsigned long long O)
{
	unsigned int outflank_h;
	unsigned long long flipped, outflank_v, outflank_d7, outflank_d9;

	outflank_v = ((O | ~0x0808080808000000ULL) + 0x0000000008000000ULL) & P & 0x0808080808000000ULL;
	flipped = (outflank_v - (unsigned int) (outflank_v != 0)) & 0x0808080808000000ULL;

	outflank_h = OUTFLANK_3[(O >> 17) & 0x3f] & (P >> 16);
	flipped |= FLIPPED_3_H[outflank_h] & 0x0000000000ff0000ULL;

	outflank_d7 = ((O | ~0x0000010204000000ULL) + 0x0000000004000000ULL) & P & 0x0000010204000000ULL;
	flipped |= (outflank_d7 - (outflank_d7 >> 24)) & 0x0000010204000000ULL;

	outflank_d9 = ((O | ~0x0080402010000000ULL) + 0x0000000010000000ULL) & P & 0x0080402010000000ULL;
	flipped |= (outflank_d9 - (unsigned int) (outflank_d9 != 0)) & 0x0080402010000000ULL;

	flipped |= (((P << 8) & 0x0000000000000800ULL) | ((P << 7) & 0x0000000000001000ULL) | ((P << 9) & 0x000000000000400ULL)) & O;

	return flipped;
}

/**
 * Compute flipped discs when playing on square E3.
 *
 * @param P player's disc pattern.
 * @param O opponent's disc pattern.
 * @return flipped disc pattern.
 */
static unsigned long long flip_E3(const unsigned long long P, const unsigned long long O)
{
	unsigned int outflank_h;
	unsigned long long flipped, outflank_v, outflank_d7, outflank_d9;

	outflank_v = ((O | ~0x1010101010000000ULL) + 0x0000000010000000ULL) & P & 0x1010101010000000ULL;
	flipped = (outflank_v - (unsigned int) (outflank_v != 0)) & 0x1010101010000000ULL;

	outflank_h = OUTFLANK_4[(O >> 17) & 0x3f] & (P >> 16);
	flipped |= FLIPPED_4_H[outflank_h] & 0x0000000000ff0000ULL;

	outflank_d7 = ((O | ~0x0001020408000000ULL) + 0x0000000008000000ULL) & P & 0x0001020408000000ULL;
	flipped |= (outflank_d7 - (unsigned int) (outflank_d7 != 0)) & 0x0001020408000000ULL;

	outflank_d9 = ((O | ~0x0000804020000000ULL) + 0x0000000020000000ULL) & P & 0x0000804020000000ULL;
	flipped |= (outflank_d9 - (outflank_d9 >> 24)) & 0x0000804020000000ULL;

	flipped |= (((P << 8) & 0x0000000000001000ULL) | ((P << 7) & 0x0000000000002000ULL) | ((P << 9) & 0x000000000000800ULL)) & O;

	return flipped;
}

/**
 * Compute flipped discs when playing on square F3.
 *
 * @param P player's disc pattern.
 * @param O opponent's disc pattern.
 * @return flipped disc pattern.
 */
static unsigned long long flip_F3(const unsigned long long P, const unsigned long long O)
{
	unsigned int outflank_h;
	unsigned long long flipped, outflank_v, outflank_d7;

	outflank_v = ((O | ~0x2020202020000000ULL) + 0x0000000020000000ULL) & P & 0x2020202020000000ULL;
	flipped = (outflank_v - (unsigned int) (outflank_v != 0)) & 0x2020202020000000ULL;

	outflank_h = OUTFLANK_5[(O >> 17) & 0x3f] & (P >> 16);
	flipped |= (FLIPPED_5[outflank_h] << 17);

	outflank_d7 = ((O | ~0x0102040810000000ULL) + 0x0000000010000000ULL) & P & 0x0102040810000000ULL;
	flipped |= (outflank_d7 - (unsigned int) (outflank_d7 != 0)) & 0x0102040810000000ULL;

	flipped |= (((P << 8) & 0x0000000000002000ULL) | ((P << 7) & 0x0000000000004000ULL) | (((P >> 9) | (P << 9)) & 0x0000000040001000ULL)) & O;

	return flipped;
}

/**
 * Compute flipped discs when playing on square G3.
 *
 * @param P player's disc pattern.
 * @param O opponent's disc pattern.
 * @return flipped disc pattern.
 */
static unsigned long long flip_G3(const unsigned long long P, const unsigned long long O)
{
	unsigned int outflank_h;
	unsigned long long flipped, outflank_v, outflank_d7;

	outflank_v = ((O | ~0x4040404040000000ULL) + 0x0000000040000000ULL) & P & 0x4040404040000000ULL;
	flipped = (outflank_v - (unsigned int) (outflank_v != 0)) & 0x4040404040000000ULL;

	outflank_h = OUTFLANK_7[(O >> 16) & 0x3e] & (P >> 15);
	flipped |= ((-outflank_h) & 0x3e) << 16;

	outflank_d7 = ((O | ~0x0204081020000000ULL) + 0x0000000020000000ULL) & P & 0x0204081020000000ULL;
	flipped |= (outflank_d7 - (unsigned int) (outflank_d7 != 0)) & 0x0204081020000000ULL;

	flipped |= (((P << 8) & 0x0000000000004000ULL) | ((P << 9) & 0x0000000000002000ULL)) & O;

	return flipped;
}

/**
 * Compute flipped discs when playing on square H3.
 *
 * @param P player's disc pattern.
 * @param O opponent's disc pattern.
 * @return flipped disc pattern.
 */
static unsigned long long flip_H3(const unsigned long long P, const unsigned long long O)
{
	unsigned int outflank_h;
	unsigned long long flipped, outflank_v, outflank_d7;

	outflank_v = ((O | ~0x8080808080000000ULL) + 0x0000000080000000ULL) & P & 0x8080808080000000ULL;
	flipped = (outflank_v - (unsigned int) (outflank_v != 0)) & 0x8080808080000000ULL;

	outflank_h = OUTFLANK_7[(O >> 17) & 0x3f] & (P >> 16);
	flipped |= ((-outflank_h) & 0x3f) << 17;

	outflank_d7 = ((O | ~0x0408102040000000ULL) + 0x0000000040000000ULL) & P & 0x0408102040000000ULL;
	flipped |= (outflank_d7 - (unsigned int) (outflank_d7 != 0)) & 0x0408102040000000ULL;

	flipped |= (((P << 8) & 0x0000000000008000ULL) | ((P << 9) & 0x0000000000004000ULL)) & O;

	return flipped;
}

/**
 * Compute flipped discs when playing on square A4.
 *
 * @param P player's disc pattern.
 * @param O opponent's disc pattern.
 * @return flipped disc pattern.
 */
static unsigned long long flip_A4(const unsigned long long P, const unsigned long long O)
{
	unsigned int outflank_v;
	unsigned long long flipped, outflank_h, flip_d7, outflank_d9;

	outflank_v = OUTFLANK_3[((O & 0x0001010101010100ULL) * 0x0102040810204080ULL) >> 57] & (((P & 0x0101010101010101ULL) * 0x0102040810204080ULL) >> 56);
	flipped = FLIPPED_3_V[outflank_v] & 0x0001010101010100ULL;

	outflank_h = ((O & 0x000000007e000000ULL) + 0x0000000002000000ULL) & P;
	flipped |= (outflank_h - (outflank_h >> 8)) & 0x000000007e000000ULL;

	flip_d7 = O & 0x0000000000020000ULL;
	flip_d7 |= (flip_d7 >> 7) & O;
	flipped |= flip_d7 & -(flip_d7 & (P << 7));

	outflank_d9 = ((O | ~0x1008040200000000ULL) + 0x0000000200000000ULL) & P & 0x1008040200000000ULL;
	flipped |= (outflank_d9 - (outflank_d9 >> 32)) & 0x1008040200000000ULL;

	return flipped;
}

/**
 * Compute flipped discs when playing on square B4.
 *
 * @param P player's disc pattern.
 * @param O opponent's disc pattern.
 * @return flipped disc pattern.
 */
static unsigned long long flip_B4(const unsigned long long P, const unsigned long long O)
{
	unsigned int outflank_v;
	unsigned long long flipped, outflank_h, flip_d7, outflank_d9;

	outflank_v = OUTFLANK_3[((O & 0x0002020202020200ULL) * 0x0081020408102040ULL) >> 57] & (((P & 0x0202020202020202ULL) * 0x0081020408102040ULL) >> 56);
	flipped = FLIPPED_3_V[outflank_v] & 0x0002020202020200ULL;

	outflank_h = ((O & 0x000000007c000000ULL) + 0x0000000004000000ULL) & P;
	flipped |= (outflank_h - (outflank_h >> 8)) & 0x000000007c000000ULL;

	flip_d7 = O & 0x0000000000040000ULL;
	flip_d7 |= (flip_d7 >> 7) & O;
	flipped |= flip_d7 & -(flip_d7 & (P << 7));

	outflank_d9 = ((O | ~0x2010080400000000ULL) + 0x0000000400000000ULL) & P & 0x2010080400000000ULL;
	flipped |= (outflank_d9 - (outflank_d9 >> 32)) & 0x2010080400000000ULL;

	return flipped;
}

/**
 * Compute flipped discs when playing on square C4.
 *
 * @param P player's disc pattern.
 * @param O opponent's disc pattern.
 * @return flipped disc pattern.
 */
static unsigned long long flip_C4(const unsigned long long P, const unsigned long long O)
{
	unsigned int outflank_h, outflank_v;
	unsigned long long flipped, flip_d7, outflank_d9;

	outflank_v = OUTFLANK_3[((O & 0x0004040404040400ULL) * 0x0040810204081020ULL) >> 57] & (((P & 0x0404040404040404ULL) * 0x0040810204081020ULL) >> 56);
	flipped = FLIPPED_3_V[outflank_v] & 0x0004040404040400ULL;

	outflank_h = OUTFLANK_2[(O >> 25) & 0x3f] & (P >> 24);
	flipped |= (FLIPPED_2[outflank_h] << 25);

	flip_d7 = O & 0x0000000000080000ULL;
	flip_d7 |= (flip_d7 >> 7) & O;
	flipped |= flip_d7 & -(flip_d7 & (P << 7));

	outflank_d9 = ((O | ~0x4020100800000000ULL) + 0x0000000800000000ULL) & P & 0x4020100800000000ULL;
	flipped |= (outflank_d9 - (outflank_d9 >> 32)) & 0x4020100800000000ULL;

	flipped |= (((P << 9) & 0x00000000000020000ULL) | ((P >> 7) & 0x00000000200000000ULL)) & O;

	return flipped;
}

/**
 * Compute flipped discs when playing on square D4.
 *
 * @param P player's disc pattern.
 * @param O opponent's disc pattern.
 * @return flipped disc pattern.
 */
static unsigned long long flip_D4(const unsigned long long P, const unsigned long long O)
{
	unsigned int outflank_h, outflank_v, outflank_d7, outflank_d9;
	unsigned long long flipped;

	outflank_v = OUTFLANK_3[((O & 0x0008080808080800ULL) * 0x0020408102040810ULL) >> 57] & (((P & 0x0808080808080808ULL) * 0x0020408102040810ULL) >> 56);
	flipped = FLIPPED_3_V[outflank_v] & 0x0008080808080800ULL;

	outflank_h = OUTFLANK_3[(O >> 25) & 0x3f] & (P >> 24);
	flipped |= FLIPPED_3_H[outflank_h] & 0x00000000ff000000ULL;

	outflank_d7 = OUTFLANK_3[((O & 0x0000020408102000ULL) * 0x0101010101010101ULL) >> 57] & (((P & 0x0001020408102040ULL) * 0x0101010101010101ULL) >> 56);
	flipped |= FLIPPED_3_H[outflank_d7] & 0x0000020408102000ULL;

	outflank_d9 = OUTFLANK_3[((O & 0x0040201008040200ULL) * 0x0101010101010101ULL) >> 57] & (((P & 0x8040201008040201ULL) * 0x0101010101010101ULL) >> 56);
	flipped |= FLIPPED_3_H[outflank_d9] & 0x0040201008040200ULL;

	return flipped;
}

/**
 * Compute flipped discs when playing on square E3.
 *
 * @param P player's disc pattern.
 * @param O opponent's disc pattern.
 * @return flipped disc pattern.
 */
static unsigned long long flip_E4(const unsigned long long P, const unsigned long long O)
{
	unsigned int outflank_h, outflank_v, outflank_d7, outflank_d9;
	unsigned long long flipped;

	outflank_v = OUTFLANK_3[((O & 0x0010101010101000ULL) * 0x0010204081020408ULL) >> 57] & (((P & 0x1010101010101010ULL) * 0x0010204081020408ULL) >> 56);
	flipped = FLIPPED_3_V[outflank_v] & 0x0010101010101000ULL;

	outflank_h = OUTFLANK_4[(O >> 25) & 0x3f] & (P >> 24);
	flipped |= FLIPPED_4_H[outflank_h] & 0x00000000ff000000ULL;

	outflank_d7 = OUTFLANK_4[((O & 0x0002040810204000ULL) * 0x0101010101010101ULL) >> 57] & (((P & 0x0102040810204080ULL) * 0x0101010101010101ULL) >> 56);
	flipped |= FLIPPED_4_H[outflank_d7] & 0x0002040810204000ULL;

	outflank_d9 = OUTFLANK_4[((O & 0x0000402010080400ULL) * 0x0101010101010101ULL) >> 57] & (((P & 0x0080402010080402ULL) * 0x0101010101010101ULL) >> 56);
	flipped |= FLIPPED_4_H[outflank_d9] & 0x0000402010080400ULL;

	return flipped;
}

/**
 * Compute flipped discs when playing on square F4.
 *
 * @param P player's disc pattern.
 * @param O opponent's disc pattern.
 * @return flipped disc pattern.
 */
static unsigned long long flip_F4(const unsigned long long P, const unsigned long long O)
{
	unsigned int outflank_h, outflank_v;
	unsigned long long flipped, outflank_d7, flip_d9;

	outflank_v = OUTFLANK_3[((O & 0x0020202020202000ULL) * 0x0008102040810204ULL) >> 57] & (((P & 0x2020202020202020ULL) * 0x0008102040810204ULL) >> 56);
	flipped = FLIPPED_3_V[outflank_v] & 0x0020202020202000ULL;

	outflank_h = OUTFLANK_5[(O >> 25) & 0x3f] & (P >> 24);
	flipped |= (FLIPPED_5[outflank_h] << 25);

	outflank_d7 = ((O | ~0x0204081000000000ULL) + 0x0000001000000000ULL) & P & 0x0204081000000000ULL;
	flipped |= (outflank_d7 - (outflank_d7 >> 32)) & 0x0204081000000000ULL;

	flip_d9 = O & 0x0000000000100000ULL;
	flip_d9 |= (flip_d9 >> 9) & O;
	flipped |= flip_d9 & -(flip_d9 & (P << 9));

	flipped |= (((P << 7) & 0x0000000000400000ULL) | ((P >> 9) & 0x0000004000000000ULL)) & O;

	return flipped;
}

/**
 * Compute flipped discs when playing on square G4.
 *
 * @param P player's disc pattern.
 * @param O opponent's disc pattern.
 * @return flipped disc pattern.
 */
static unsigned long long flip_G4(const unsigned long long P, const unsigned long long O)
{
	unsigned int outflank_h, outflank_v;
	unsigned long long flipped, outflank_d7, flip_d9;

	outflank_v = OUTFLANK_3[((O & 0x0040404040404000ULL) * 0x0004081020408102ULL) >> 57] & (((P & 0x4040404040404040ULL) * 0x0004081020408102ULL) >> 56);
	flipped = FLIPPED_3_V[outflank_v] & 0x0040404040404000ULL;

	outflank_h = OUTFLANK_7[(O >> 24) & 0x3e] & (P >> 23);
	flipped |= ((-outflank_h) & 0x3e) << 24;

	outflank_d7 = ((O | ~0x0408102000000000ULL) + 0x0000002000000000ULL) & P & 0x0408102000000000ULL;
	flipped |= (outflank_d7 - (outflank_d7 >> 32)) & 0x0408102000000000ULL;

	flip_d9 = O & 0x0000000000200000ULL;
	flip_d9 |= (flip_d9 >> 9) & O;
	flipped |= flip_d9 & -(flip_d9 & (P << 9));

	return flipped;
}

/**
 * Compute flipped discs when playing on square H4.
 *
 * @param P player's disc pattern.
 * @param O opponent's disc pattern.
 * @return flipped disc pattern.
 */
static unsigned long long flip_H4(const unsigned long long P, const unsigned long long O)
{
	unsigned int outflank_h, outflank_v;
	unsigned long long flipped, outflank_d7, flip_d9;

	outflank_v = OUTFLANK_3[((O & 0x0080808080808000ULL) * 0x0002040810204081ULL) >> 57] & (((P & 0x8080808080808080ULL) * 0x0002040810204081ULL) >> 56);
	flipped = FLIPPED_3_V[outflank_v] & 0x0080808080808000ULL;

	outflank_h = OUTFLANK_7[(O >> 25) & 0x3f] & (P >> 24);
	flipped |= ((-outflank_h) & 0x3f) << 25;

	outflank_d7 = ((O | ~0x0810204000000000ULL) + 0x0000004000000000ULL) & P & 0x0810204000000000ULL;
	flipped |= (outflank_d7 - (outflank_d7 >> 32)) & 0x0810204000000000ULL;

	flip_d9 = O & 0x0000000000400000ULL;
	flip_d9 |= (flip_d9 >> 9) & O;
	flipped |= flip_d9 & -(flip_d9 & (P << 9));

	return flipped;
}

/**
 * Compute flipped discs when playing on square A5.
 *
 * @param P player's disc pattern.
 * @param O opponent's disc pattern.
 * @return flipped disc pattern.
 */
static unsigned long long flip_A5(const unsigned long long P, const unsigned long long O)
{
	unsigned int outflank_a1a5d8, outflank_a8a5e1;
	unsigned long long flipped, outflank_h;

	outflank_a1a5d8 = OUTFLANK_4[((O & 0x0004020101010100ULL) * 0x0102040810101010ULL) >> 57] & (((P & 0x0804020101010101ULL) * 0x0102040810101010ULL) >> 56);
	flipped = FLIPPED_4_V[outflank_a1a5d8] & 0x0004020101010100ULL;

	outflank_a8a5e1 = OUTFLANK_3[((O & 0x0001010102040800ULL) * 0x0808080808040201ULL) >> 57] & (((P & 0x0101010102040810ULL) * 0x0808080808040201ULL) >> 56);
	flipped |= FLIPPED_3_U[outflank_a8a5e1] & 0x0001010102040800ULL;

	outflank_h = ((O & 0x0000007e00000000ULL) + 0x0000000200000000ULL) & P;
	flipped |= (outflank_h - (outflank_h >> 8)) & 0x0000007e00000000ULL;

	return flipped;
}

/**
 * Compute flipped discs when playing on square B5.
 *
 * @param P player's disc pattern.
 * @param O opponent's disc pattern.
 * @return flipped disc pattern.
 */
static unsigned long long flip_B5(const unsigned long long P, const unsigned long long O)
{
	unsigned int outflank_b1b5e8, outflank_b8b5f1;
	unsigned long long flipped, outflank_h;

	outflank_b1b5e8 = OUTFLANK_4[((O & 0x0008040202020200ULL) * 0x0081020408080808ULL) >> 57] & (((P & 0x1008040202020202ULL) * 0x0081020408080808ULL) >> 56);
	flipped = FLIPPED_4_V[outflank_b1b5e8] & 0x0008040202020200ULL;

	outflank_b8b5f1 = OUTFLANK_3[((O & 0x0002020204081000ULL) * 0x0808080808040201ULL) >> 58] & ((((P & 0x0202020204081020ULL) >> 1) * 0x0808080808040201ULL) >> 56);
	flipped |= FLIPPED_3_U[outflank_b8b5f1] & 0x0002020204081000ULL;

	outflank_h = ((O & 0x0000007c00000000ULL) + 0x0000000400000000ULL) & P;
	flipped |= (outflank_h - (outflank_h >> 8)) & 0x0000007c00000000ULL;

	return flipped;
}

/**
 * Compute flipped discs when playing on square C5.
 *
 * @param P player's disc pattern.
 * @param O opponent's disc pattern.
 * @return flipped disc pattern.
 */
static unsigned long long flip_C5(const unsigned long long P, const unsigned long long O)
{
	unsigned int outflank_c1c5f8, outflank_c8c5g1;
	unsigned long long flipped, outflank_h;

	outflank_c1c5f8 = OUTFLANK_4[((O & 0x0010080404040400ULL) * 0x0040810204040404ULL) >> 57] & (((P & 0x2010080404040404ULL) * 0x0040810204040404ULL) >> 56);
	flipped = FLIPPED_4_V[outflank_c1c5f8] & 0x0010080404040400ULL;

	outflank_c8c5g1 = OUTFLANK_3[((O & 0x0004040408102000ULL) * 0x0002020202010080ULL) >> 57] & ((((P & 0x0404040408102040ULL) >> 2) * 0x0808080808040201ULL) >> 56);
	flipped |= FLIPPED_3_U[outflank_c8c5g1] & 0x0004040408102000ULL;

	outflank_h = OUTFLANK_2[(O >> 33) & 0x3f] & (P >> 32);
	flipped |= ((unsigned long long) FLIPPED_2[outflank_h]) << 33;

	flipped |= (((P << 9) & 0x0000000002000000ULL) | ((P >> 7) & 0x0000020000000000ULL)) & O;

	return flipped;
}

/**
 * Compute flipped discs when playing on square D5.
 *
 * @param P player's disc pattern.
 * @param O opponent's disc pattern.
 * @return flipped disc pattern.
 */
static unsigned long long flip_D5(const unsigned long long P, const unsigned long long O)
{
	unsigned int outflank_h, outflank_v, outflank_d7, outflank_d9;
	unsigned long long flipped;

	outflank_v = OUTFLANK_4[((O & 0x0008080808080800ULL) * 0x0020408102040810ULL) >> 57] & (((P & 0x0808080808080808ULL) * 0x0020408102040810ULL) >> 56);
	flipped = FLIPPED_4_V[outflank_v] & 0x0008080808080800ULL;

	outflank_h = OUTFLANK_3[(O >> 33) & 0x3f] & (P >> 32);
	flipped |= FLIPPED_3_H[outflank_h] & 0x000000ff00000000ULL;

	outflank_d7 = OUTFLANK_3[((O & 0x0002040810204000ULL) * 0x0101010101010101ULL) >> 57] & (((P & 0x0102040810204080ULL) * 0x0101010101010101ULL) >> 56);
	flipped |= FLIPPED_3_H[outflank_d7] & 0x0002040810204000ULL;

	outflank_d9 = OUTFLANK_3[((O & 0x0020100804020000ULL) * 0x0101010101010101ULL) >> 57] & (((P & 0x4020100804020100ULL) * 0x0101010101010101ULL) >> 56);
	flipped |= FLIPPED_3_H[outflank_d9] & 0x0020100804020000ULL;

	return flipped;
}

/**
 * Compute flipped discs when playing on square E5.
 *
 * @param P player's disc pattern.
 * @param O opponent's disc pattern.
 * @return flipped disc pattern.
 */
static unsigned long long flip_E5(const unsigned long long P, const unsigned long long O)
{
	unsigned int outflank_h, outflank_v, outflank_d7, outflank_d9;
	unsigned long long flipped;

	outflank_v = OUTFLANK_4[((O & 0x0010101010101000ULL) * 0x0010204081020408ULL) >> 57] & (((P & 0x1010101010101010ULL) * 0x0010204081020408ULL) >> 56);
	flipped = FLIPPED_4_V[outflank_v] & 0x0010101010101000ULL;

	outflank_h = OUTFLANK_4[(O >> 33) & 0x3f] & (P >> 32);
	flipped |= FLIPPED_4_H[outflank_h] & 0x000000ff00000000ULL;

	outflank_d7 = OUTFLANK_4[((O & 0x0004081020400000ULL) * 0x0101010101010101ULL) >> 57] & (((P & 0x0204081020408000ULL) * 0x0101010101010101ULL) >> 56);
	flipped |= FLIPPED_4_H[outflank_d7] & 0x0004081020400000ULL;

	outflank_d9 = OUTFLANK_4[((O & 0x0040201008040200ULL) * 0x0101010101010101ULL) >> 57] & (((P & 0x8040201008040201ULL) * 0x0101010101010101ULL) >> 56);
	flipped |= FLIPPED_4_H[outflank_d9] & 0x0040201008040200ULL;

	return flipped;
}

/**
 * Compute flipped discs when playing on square F5.
 *
 * @param P player's disc pattern.
 * @param O opponent's disc pattern.
 * @return flipped disc pattern.
 */
static unsigned long long flip_F5(const unsigned long long P, const unsigned long long O)
{
	unsigned int outflank_h, outflank_b1f5f8, outflank_c8f5f1;
	unsigned long long flipped;

	outflank_b1f5f8 = OUTFLANK_4[((O & 0x0020202010080400ULL) * 0x0080808080810204ULL) >> 57] & (((P & 0x2020202010080402ULL) * 0x0080808080810204ULL) >> 56);
	flipped = FLIPPED_4_V[outflank_b1f5f8] & 0x0020202010080400ULL;

	outflank_c8f5f1 = OUTFLANK_3[((O & 0x0008102020202000ULL) * 0x0002010080404040ULL) >> 57] & ((((P & 0x0408102020202020ULL) >> 2) * 0x1008040201010101ULL) >> 56);
	flipped |= FLIPPED_3_U[outflank_c8f5f1] & 0x0008102020202000ULL;

	outflank_h = OUTFLANK_5[(O >> 33) & 0x3f] & (P >> 32);
	flipped |= ((unsigned long long) FLIPPED_5[outflank_h]) << 33;

	flipped |= (((P << 7) & 0x0000000040000000ULL) | ((P >> 9) & 0x0000400000000000ULL)) & O;

	return flipped;
}

/**
 * Compute flipped discs when playing on square G5.
 *
 * @param P player's disc pattern.
 * @param O opponent's disc pattern.
 * @return flipped disc pattern.
 */
static unsigned long long flip_G5(const unsigned long long P, const unsigned long long O)
{
	unsigned int outflank_h, outflank_c1g5g8, outflank_d8g5g1;
	unsigned long long flipped;

	outflank_c1g5g8 = OUTFLANK_4[((O & 0x0040404020100800ULL) * 0x0040404040408102ULL) >> 57] & (((P & 0x4040404020100804ULL) * 0x0040404040408102ULL) >> 56);
	flipped = FLIPPED_4_V[outflank_c1g5g8] & 0x0040404020100800ULL;

	outflank_d8g5g1 = OUTFLANK_3[((O & 0x0010204040404000ULL) * 0x0001008040202020ULL) >> 57] & ((((P & 0x0810204040404040ULL) >> 3) * 0x1008040201010101ULL) >> 56);
	flipped |= FLIPPED_3_U[outflank_d8g5g1] & 0x0010204040404000ULL;

	outflank_h = OUTFLANK_7[(O >> 32) & 0x3e] & (P >> 31);
	flipped |= (unsigned long long) ((-outflank_h) & 0x3e) << 32;

	return flipped;
}

/**
 * Compute flipped discs when playing on square H5.
 *
 * @param P player's disc pattern.
 * @param O opponent's disc pattern.
 * @return flipped disc pattern.
 */
static unsigned long long flip_H5(const unsigned long long P, const unsigned long long O)
{
	unsigned int outflank_h, outflank_d1h5h8, outflank_e8h5h1;
	unsigned long long flipped;

	outflank_d1h5h8 = OUTFLANK_4[((O & 0x0080808040201000ULL) * 0x0020202020204081ULL) >> 57] & (((P & 0x8080808040201008ULL) * 0x0020202020204081ULL) >> 56);
	flipped = FLIPPED_4_V[outflank_d1h5h8] & 0x0080808040201000ULL;

	outflank_e8h5h1 = OUTFLANK_3[((O & 0x0020408080808000ULL) * 0x0000804020101010ULL) >> 57] & ((((P & 0x1020408080808080ULL) >> 4) * 0x1008040201010101ULL) >> 56);
	flipped |= FLIPPED_3_U[outflank_e8h5h1] & 0x0020408080808000ULL;

	outflank_h = OUTFLANK_7[(O >> 33) & 0x3f] & (P >> 32);
	flipped |= (unsigned long long) ((-outflank_h) & 0x3f) << 33;

	return flipped;
}

/**
 * Compute flipped discs when playing on square A6.
 *
 * @param P player's disc pattern.
 * @param O opponent's disc pattern.
 * @return flipped disc pattern.
 */
static unsigned long long flip_A6(const unsigned long long P, const unsigned long long O)
{
	unsigned long long flipped, outflank_v, outflank_h, outflank_d7;

	outflank_v = (0x8000000000000000ULL >> __builtin_clzll((O & 0x0000000101010100ULL) ^ 0x0000000101010101ULL)) & P;
	flipped = (-outflank_v * 2) & 0x0000000101010100ULL;

	outflank_h = ((O & 0x00007e0000000000ULL) + 0x0000020000000000ULL) & P;
	flipped |= (outflank_h - (outflank_h >> 8)) & 0x00007e0000000000ULL;

	outflank_d7 = (0x8000000000000000ULL >> __builtin_clzll((O & 0x0000000204081000ULL) ^ 0x0000000204081020ULL)) & P;
	flipped |= (-outflank_d7 * 2) & 0x0000000204081000ULL;

	flipped |= (((P >> 8) & 0x0001000000000000ULL) | ((P >> 9) & 0x0002000000000000ULL)) & O;

	return flipped;
}

/**
 * Compute flipped discs when playing on square B6.
 *
 * @param P player's disc pattern.
 * @param O opponent's disc pattern.
 * @return flipped disc pattern.
 */
static unsigned long long flip_B6(const unsigned long long P, const unsigned long long O)
{
	unsigned long long flipped, outflank_v, outflank_h, outflank_d7;

	outflank_v = (0x8000000000000000ULL >> __builtin_clzll((O & 0x0000000202020200ULL) ^ 0x0000000202020202ULL)) & P;
	flipped = (-outflank_v * 2) & 0x0000000202020200ULL;

	outflank_h = ((O & 0x00007c0000000000ULL) + 0x0000040000000000ULL) & P;
	flipped |= (outflank_h - (outflank_h >> 8)) & 0x00007c0000000000ULL;

	outflank_d7 = (0x8000000000000000ULL >> __builtin_clzll((O & 0x0000000408102000ULL) ^ 0x0000000408102040ULL)) & P;
	flipped |= (-outflank_d7 * 2) & 0x0000000408102000ULL;

	flipped |= (((P >> 8) & 0x0002000000000000ULL) | ((P >> 9) & 0x0004000000000000ULL)) & O;

	return flipped;
}

/**
 * Compute flipped discs when playing on square C6.
 *
 * @param P player's disc pattern.
 * @param O opponent's disc pattern.
 * @return flipped disc pattern.
 */
static unsigned long long flip_C6(const unsigned long long P, const unsigned long long O)
{
	unsigned int outflank_h;
	unsigned long long flipped, outflank_v, outflank_d7;

	outflank_v = (0x8000000000000000ULL >> __builtin_clzll((O & 0x0000000404040400ULL) ^ 0x0000000404040404ULL)) & P;
	flipped = (-outflank_v * 2) & 0x0000000404040400ULL;

	outflank_h = OUTFLANK_2[(O >> 41) & 0x3f] & (P >> 40);
	flipped |= ((unsigned long long) FLIPPED_2[outflank_h]) << 41;

	outflank_d7 = (0x8000000000000000ULL >> __builtin_clzll((O & 0x0000000810204000ULL) ^ 0x0000000810204080ULL)) & P;
	flipped |= (-outflank_d7 * 2) & 0x0000000810204000ULL;

	flipped |= (((P >> 8) & 0x0004000000000000ULL) | ((P >> 7) & 0x0002000000000000ULL) | (((P >> 9) | (P << 9)) & 0x0008000200000000ULL)) & O;

	return flipped;
}

/**
 * Compute flipped discs when playing on square D6.
 *
 * @param P player's disc pattern.
 * @param O opponent's disc pattern.
 * @return flipped disc pattern.
 */
static unsigned long long flip_D6(const unsigned long long P, const unsigned long long O)
{
	unsigned int outflank_h, outflank_d;
	unsigned long long flipped, outflank_v;

	outflank_v = (0x8000000000000000ULL >> __builtin_clzll((O & 0x0000000808080800ULL) ^ 0x0000000808080808ULL)) & P;
	flipped = (-outflank_v * 2) & 0x0000000808080800ULL;

	outflank_h = OUTFLANK_3[(O >> 41) & 0x3f] & (P >> 40);
	flipped |= FLIPPED_3_H[outflank_h] & 0x0000ff0000000000ULL;

	outflank_d = OUTFLANK_3[((O & 0x0000001422400000ULL) * 0x0101010101010101ULL) >> 57]
		& (((P & 0x0000001422418000ULL) * 0x0101010101010101ULL) >> 56);
	flipped |= FLIPPED_3_H[outflank_d] & 0x0000001422400000ULL;	// A3D6H2

	flipped |= (((P >> 8) & 0x0008000000000000ULL) | ((P >> 9) & 0x0010000000000000ULL) | ((P >> 7) & 0x0004000000000000ULL)) & O;

	return flipped;
}

/**
 * Compute flipped discs when playing on square E6.
 *
 * @param P player's disc pattern.
 * @param O opponent's disc pattern.
 * @return flipped disc pattern.
 */
static unsigned long long flip_E6(const unsigned long long P, const unsigned long long O)
{
	unsigned int outflank_h, outflank_d;
	unsigned long long flipped, outflank_v;

	outflank_v = (0x8000000000000000ULL >> __builtin_clzll((O & 0x0000001010101000ULL) ^ 0x0000001010101010ULL)) & P;
	flipped = (-outflank_v * 2) & 0x0000001010101000ULL;

	outflank_h = OUTFLANK_4[(O >> 41) & 0x3f] & (P >> 40);
	flipped |= FLIPPED_4_H[outflank_h] & 0x0000ff0000000000ULL;

	outflank_d = OUTFLANK_4[((O & 0x0000002844020000ULL) * 0x0101010101010101ULL) >> 57]
		& (((P & 0x0000002844820100ULL) * 0x0101010101010101ULL) >> 56);
	flipped |= FLIPPED_4_H[outflank_d] & 0x0000002844020000ULL;	// A2E6H3

	flipped |= (((P >> 8) & 0x0010000000000000ULL) | ((P >> 9) & 0x0020000000000000ULL) | ((P >> 7) & 0x0008000000000000ULL)) & O;

	return flipped;
}

/**
 * Compute flipped discs when playing on square F6.
 *
 * @param P player's disc pattern.
 * @param O opponent's disc pattern.
 * @return flipped disc pattern.
 */
static unsigned long long flip_F6(const unsigned long long P, const unsigned long long O)
{
	unsigned int outflank_h;
	unsigned long long flipped, outflank_v, outflank_d9;

	outflank_v = (0x8000000000000000ULL >> __builtin_clzll((O & 0x0000002020202000ULL) ^ 0x0000002020202020ULL)) & P;
	flipped = (-outflank_v * 2) & 0x0000002020202000ULL;

	outflank_h = OUTFLANK_5[(O >> 41) & 0x3f] & (P >> 40);
	flipped |= ((unsigned long long) FLIPPED_5[outflank_h]) << 41;

	outflank_d9 = (0x8000000000000000ULL >> __builtin_clzll((O & 0x0000001008040200ULL) ^ 0x0000001008040201ULL)) & P;
	flipped |= (-outflank_d9 * 2) & 0x0000001008040200ULL;

	flipped |= (((P >> 8) & 0x0020000000000000ULL) | ((P >> 9) & 0x0040000000000000ULL) | (((P >> 7) | (P << 7)) & 0x0010004000000000ULL)) & O;

	return flipped;
}

/**
 * Compute flipped discs when playing on square G6.
 *
 * @param P player's disc pattern.
 * @param O opponent's disc pattern.
 * @return flipped disc pattern.
 */
static unsigned long long flip_G6(const unsigned long long P, const unsigned long long O)
{
	unsigned int outflank_h;
	unsigned long long flipped, outflank_v, outflank_d9;

	outflank_v = (0x8000000000000000ULL >> __builtin_clzll((O & 0x0000004040404000ULL) ^ 0x0000004040404040ULL)) & P;
	flipped = (-outflank_v * 2) & 0x0000004040404000ULL;

	outflank_h = OUTFLANK_7[(O >> 40) & 0x3e] & (P >> 39);
	flipped |= (unsigned long long) ((-outflank_h) & 0x3e) << 40;

	outflank_d9 = (0x8000000000000000ULL >> __builtin_clzll((O & 0x0000002010080400ULL) ^ 0x0000002010080402ULL)) & P;
	flipped |= (-outflank_d9 * 2) & 0x0000002010080400ULL;

	flipped |= (((P >> 8) & 0x0040000000000000ULL) | ((P >> 7) & 0x0020000000000000ULL)) & O;

	return flipped;
}

/**
 * Compute flipped discs when playing on square H6.
 *
 * @param P player's disc pattern.
 * @param O opponent's disc pattern.
 * @return flipped disc pattern.
 */
static unsigned long long flip_H6(const unsigned long long P, const unsigned long long O)
{
	unsigned int outflank_h;
	unsigned long long flipped, outflank_v, outflank_d9;

	outflank_v = (0x8000000000000000ULL >> __builtin_clzll((O & 0x0000008080808000ULL) ^ 0x0000008080808080ULL)) & P;
	flipped = (-outflank_v * 2) & 0x0000008080808000ULL;

	outflank_h = OUTFLANK_7[(O >> 41) & 0x3f] & (P >> 40);
	flipped |= (unsigned long long) ((-outflank_h) & 0x3f) << 41;

	outflank_d9 = (0x8000000000000000ULL >> __builtin_clzll((O & 0x0000004020100800ULL) ^ 0x0000004020100804ULL)) & P;
	flipped |= (-outflank_d9 * 2) & 0x0000004020100800ULL;

	flipped |= (((P >> 8) & 0x0080000000000000ULL) | ((P >> 7) & 0x0040000000000000ULL)) & O;

	return flipped;
}

/**
 * Compute flipped discs when playing on square A7.
 *
 * @param P player's disc pattern.
 * @param O opponent's disc pattern.
 * @return flipped disc pattern.
 */
static unsigned long long flip_A7(const unsigned long long P, const unsigned long long O)
{
	unsigned long long flipped, outflank_v, outflank_h, outflank_d7;

	outflank_v = (0x8000000000000000ULL >> __builtin_clzll((O & 0x0000010101010100ULL) ^ 0x0000010101010101ULL)) & P;
	flipped  = (-outflank_v * 2) & 0x0000010101010100ULL;

	outflank_h = ((O & 0x007e000000000000ULL) + 0x0002000000000000ULL) & P;
	flipped |= (outflank_h - (outflank_h >> 8)) & 0x007e000000000000ULL;

	outflank_d7 = (0x8000000000000000ULL >> __builtin_clzll((O & 0x0000020408102000ULL) ^ 0x0000020408102040ULL)) & P;
	flipped |= (-outflank_d7 * 2) & 0x0000020408102000ULL;

	return flipped;
}

/**
 * Compute flipped discs when playing on square B7.
 *
 * @param P player's disc pattern.
 * @param O opponent's disc pattern.
 * @return flipped disc pattern.
 */
static unsigned long long flip_B7(const unsigned long long P, const unsigned long long O)
{
	unsigned long long flipped, outflank_v, outflank_h, outflank_d7;

	outflank_v = (0x8000000000000000ULL >> __builtin_clzll((O & 0x0000020202020200ULL) ^ 0x0000020202020202ULL)) & P;
	flipped  = (-outflank_v * 2) & 0x0000020202020200ULL;

	outflank_h = ((O & 0x007c000000000000ULL) + 0x0004000000000000ULL) & P;
	flipped |= (outflank_h - (outflank_h >> 8)) & 0x007c000000000000ULL;

	outflank_d7 = (0x8000000000000000ULL >> __builtin_clzll((O & 0x0000040810204000ULL) ^ 0x0000040810204080ULL)) & P;
	flipped |= (-outflank_d7 * 2) & 0x0000040810204000ULL;

	return flipped;
}

/**
 * Compute flipped discs when playing on square C7.
 *
 * @param P player's disc pattern.
 * @param O opponent's disc pattern.
 * @return flipped disc pattern.
 */
static unsigned long long flip_C7(const unsigned long long P, const unsigned long long O)
{
	unsigned int outflank_h;
	unsigned long long flipped, outflank_d7, outflank_v;

	outflank_v = (0x8000000000000000ULL >> __builtin_clzll((O & 0x0000040404040400ULL) ^ 0x0000040404040404ULL)) & P;
	flipped  = (-outflank_v * 2) & 0x0000040404040400ULL;

	outflank_h = OUTFLANK_2[(O >> 49) & 0x3f] & (P >> 48);
	flipped |= ((unsigned long long) FLIPPED_2[outflank_h]) << 49;

	outflank_d7 = (0x8000000000000000ULL >> __builtin_clzll((O & 0x0000081020400000ULL) ^ 0x0000081020408000ULL)) & P;
	flipped |= (-outflank_d7 * 2) & 0x0000081020400000ULL;

	flipped |= (P << 9) & 0x0000020000000000ULL & O;

	return flipped;
}

/**
 * Compute flipped discs when playing on square D7.
 *
 * @param P player's disc pattern.
 * @param O opponent's disc pattern.
 * @return flipped disc pattern.
 */
static unsigned long long flip_D7(const unsigned long long P, const unsigned long long O)
{
	unsigned int outflank_h, outflank_d;
	unsigned long long flipped, outflank_v;

	outflank_v = (0x8000000000000000ULL >> __builtin_clzll((O & 0x0000080808080800ULL) ^ 0x0000080808080808ULL)) & P;
	flipped  = (-outflank_v * 2) & 0x0000080808080800ULL;

	outflank_h = OUTFLANK_3[(O >> 49) & 0x3f] & (P >> 48);
	flipped |= FLIPPED_3_H[outflank_h] & 0x00ff000000000000ULL;

	outflank_d = OUTFLANK_3[((O & 0x0000142240000000ULL) * 0x0101010101010101ULL) >> 57]
		& (((P & 0x0000142241800000ULL) * 0x0101010101010101ULL) >> 56);
	flipped |= FLIPPED_3_H[outflank_d] & 0x0000142240000000ULL;	// A4D7H3

	return flipped;
}

/**
 * Compute flipped discs when playing on square E7.
 *
 * @param P player's disc pattern.
 * @param O opponent's disc pattern.
 * @return flipped disc pattern.
 */
static unsigned long long flip_E7(const unsigned long long P, const unsigned long long O)
{
	unsigned int outflank_h, outflank_d;
	unsigned long long flipped, outflank_v;

	outflank_v = (0x8000000000000000ULL >> __builtin_clzll((O & 0x0000101010101000ULL) ^ 0x0000101010101010ULL)) & P;
	flipped  = (-outflank_v * 2) & 0x0000101010101000ULL;

	outflank_h = OUTFLANK_4[(O >> 49) & 0x3f] & (P >> 48);
	flipped |= FLIPPED_4_H[outflank_h] & 0x00ff000000000000ULL;

	outflank_d = OUTFLANK_4[((O & 0x0000284402000000ULL) * 0x0101010101010101ULL) >> 57]
		& (((P & 0x0000284482010000ULL) * 0x0101010101010101ULL) >> 56);
	flipped |= FLIPPED_4_H[outflank_d] & 0x0000284402000000ULL;	// A3E7H4

	return flipped;
}

/**
 * Compute flipped discs when playing on square F7.
 *
 * @param P player's disc pattern.
 * @param O opponent's disc pattern.
 * @return flipped disc pattern.
 */
static unsigned long long flip_F7(const unsigned long long P, const unsigned long long O)
{
	unsigned int outflank_h;
	unsigned long long flipped, outflank_d9, outflank_v;

	outflank_v = (0x8000000000000000ULL >> __builtin_clzll((O & 0x0000202020202000ULL) ^ 0x0000202020202020ULL)) & P;
	flipped  = (-outflank_v * 2) & 0x0000202020202000ULL;

	outflank_h = OUTFLANK_5[(O >> 49) & 0x3f] & (P >> 48);
	flipped |= ((unsigned long long) FLIPPED_5[outflank_h]) << 49;

	flipped |= (P << 7) & 0x0000400000000000ULL & O;

	outflank_d9 = (0x8000000000000000ULL >> __builtin_clzll((O & 0x0000100804020000ULL) ^ 0x0000100804020100ULL)) & P;
	flipped |= (-outflank_d9 * 2) & 0x0000100804020000ULL;

	return flipped;
}

/**
 * Compute flipped discs when playing on square G7.
 *
 * @param P player's disc pattern.
 * @param O opponent's disc pattern.
 * @return flipped disc pattern.
 */
static unsigned long long flip_G7(const unsigned long long P, const unsigned long long O)
{
	unsigned int outflank_h;
	unsigned long long flipped, outflank_v, outflank_d9;

	outflank_v = (0x8000000000000000ULL >> __builtin_clzll((O & 0x0000404040404000ULL) ^ 0x0000404040404040ULL)) & P;
	flipped  = (-outflank_v * 2) & 0x0000404040404000ULL;

	outflank_h = OUTFLANK_7[(O >> 48) & 0x3e] & (P >> 47);
	flipped |= (unsigned long long) ((-outflank_h) & 0x3e) << 48;

	outflank_d9 = (0x8000000000000000ULL >> __builtin_clzll((O & 0x0000201008040200ULL) ^ 0x0000201008040201ULL)) & P;
	flipped |= (-outflank_d9 * 2) & 0x0000201008040200ULL;

	return flipped;
}

/**
 * Compute flipped discs when playing on square H7.
 *
 * @param P player's disc pattern.
 * @param O opponent's disc pattern.
 * @return flipped disc pattern.
 */
static unsigned long long flip_H7(const unsigned long long P, const unsigned long long O)
{
	unsigned int outflank_h;
	unsigned long long flipped, outflank_v, outflank_d9;

	outflank_v = (0x8000000000000000ULL >> __builtin_clzll((O & 0x0000808080808000ULL) ^ 0x0000808080808080ULL)) & P;
	flipped  = (-outflank_v * 2) & 0x0000808080808000ULL;

	outflank_h = OUTFLANK_7[(O >> 49) & 0x3f] & (P >> 48);
	flipped |= (unsigned long long) ((-outflank_h) & 0x3f) << 49;

	outflank_d9 = (0x8000000000000000ULL >> __builtin_clzll((O & 0x0000402010080400ULL) ^ 0x0000402010080402ULL)) & P;
	flipped |= (-outflank_d9 * 2) & 0x0000402010080400ULL;

	return flipped;
}

/**
 * Compute flipped discs when playing on square A8.
 *
 * @param P player's disc pattern.
 * @param O opponent's disc pattern.
 * @return flipped disc pattern.
 */
static unsigned long long flip_A8(const unsigned long long P, const unsigned long long O)
{
	unsigned long long flipped, outflank_v, outflank_h, outflank_d7;

	outflank_v = (0x8000000000000000ULL >> __builtin_clzll((O & 0x0001010101010100ULL) ^ 0x0001010101010101ULL)) & P;
	flipped  = (-outflank_v * 2) & 0x0001010101010100ULL;

	outflank_h = ((O & 0x7e00000000000000ULL) + 0x0200000000000000ULL) & P;
	flipped |= (outflank_h - (outflank_h >> 8)) & 0x7e00000000000000ULL;

	outflank_d7 = (0x8000000000000000ULL >> __builtin_clzll((O & 0x0002040810204000ULL) ^ 0x0002040810204080ULL)) & P;
	flipped |= (-outflank_d7 * 2) & 0x0002040810204000ULL;

	return flipped;
}

/**
 * Compute flipped discs when playing on square B8.
 *
 * @param P player's disc pattern.
 * @param O opponent's disc pattern.
 * @return flipped disc pattern.
 */
static unsigned long long flip_B8(const unsigned long long P, const unsigned long long O)
{
	unsigned long long flipped, outflank_v, outflank_h, outflank_d7;

	outflank_v = (0x8000000000000000ULL >> __builtin_clzll((O & 0x0002020202020200ULL) ^ 0x0002020202020202ULL)) & P;
	flipped  = (0x8000000000000000ULL - outflank_v * 2) & 0x0002020202020200ULL;

	outflank_h = ((O & 0x7c00000000000000ULL) + 0x0400000000000000ULL) & P;
	flipped |= (outflank_h - (outflank_h >> 8)) & 0x7c00000000000000ULL;

	outflank_d7 = (0x8000000000000000ULL >> __builtin_clzll((O & 0x0004081020400000ULL) ^ 0x0004081020408000ULL)) & P;
	flipped |= (0x8000000000000000ULL - outflank_d7 * 2) & 0x0004081020400000ULL;

	return flipped;
}

/**
 * Compute flipped discs when playing on square C8.
 *
 * @param P player's disc pattern.
 * @param O opponent's disc pattern.
 * @return flipped disc pattern.
 */
static unsigned long long flip_C8(const unsigned long long P, const unsigned long long O)
{
	unsigned int outflank_h;
	unsigned long long flipped, outflank_d7, outflank_v;

	outflank_v = (0x8000000000000000ULL >> __builtin_clzll((O & 0x0004040404040400ULL) ^ 0x0004040404040404ULL)) & P;
	flipped  = (-outflank_v * 2) & 0x0004040404040400ULL;

	outflank_h = OUTFLANK_2[(O >> 57) & 0x3f] & (P >> 56);
	flipped |= ((unsigned long long) FLIPPED_2[outflank_h]) << 57;

	outflank_d7 = (0x8000000000000000ULL >> __builtin_clzll((O & 0x0008102040000000ULL) ^ 0x0008102040800000ULL)) & P;
	flipped |= (0x8000000000000000ULL - outflank_d7 * 2) & 0x0008102040000000ULL;

	flipped |= (P << 9) & 0x0002000000000000ULL & O;

	return flipped;
}

/**
 * Compute flipped discs when playing on square D8.
 *
 * @param P player's disc pattern.
 * @param O opponent's disc pattern.
 * @return flipped disc pattern.
 */
static unsigned long long flip_D8(const unsigned long long P, const unsigned long long O)
{
	unsigned int outflank_h, outflank_d;
	unsigned long long flipped, outflank_v;

	outflank_v = (0x8000000000000000ULL >> __builtin_clzll((O & 0x0008080808080800ULL) ^ 0x0008080808080808ULL)) & P;
	flipped  = (-outflank_v * 2) & 0x0008080808080800ULL;

	outflank_h = OUTFLANK_3[(O >> 57) & 0x3f] & (P >> 56);
	flipped |= FLIPPED_3_H[outflank_h] & 0xff00000000000000ULL;

	outflank_d = OUTFLANK_3[((O & 0x0014224000000000ULL) * 0x0101010101010101ULL) >> 57]
		& (((P & 0x0014224180000000ULL) * 0x0101010101010101ULL) >> 56);
	flipped |= FLIPPED_3_H[outflank_d] & 0x0014224000000000ULL;	// A5D8H4

	return flipped;
}

/**
 * Compute flipped discs when playing on square E8.
 *
 * @param P player's disc pattern.
 * @param O opponent's disc pattern.
 * @return flipped disc pattern.
 */
static unsigned long long flip_E8(const unsigned long long P, const unsigned long long O)
{
	unsigned int outflank_h, outflank_d;
	unsigned long long flipped, outflank_v;

	outflank_v = (0x8000000000000000ULL >> __builtin_clzll((O & 0x0010101010101000ULL) ^ 0x0010101010101010ULL)) & P;
	flipped  = (-outflank_v * 2) & 0x0010101010101000ULL;

	outflank_h = OUTFLANK_4[(O >> 57) & 0x3f] & (P >> 56);
	flipped |= FLIPPED_4_H[outflank_h] & 0xff00000000000000ULL;

	outflank_d = OUTFLANK_4[((O & 0x0028440200000000ULL) * 0x0101010101010101ULL) >> 57]
		& (((P & 0x0028448201000000ULL) * 0x0101010101010101ULL) >> 56);
	flipped |= FLIPPED_4_H[outflank_d] & 0x0028440200000000ULL;	// A4E8H5

	return flipped;
}

/**
 * Compute flipped discs when playing on square F8.
 *
 * @param P player's disc pattern.
 * @param O opponent's disc pattern.
 * @return flipped disc pattern.
 */
static unsigned long long flip_F8(const unsigned long long P, const unsigned long long O)
{
	unsigned int outflank_h;
	unsigned long long flipped, outflank_d9, outflank_v;

	outflank_v = (0x8000000000000000ULL >> __builtin_clzll((O & 0x0020202020202000ULL) ^ 0x0020202020202020ULL)) & P;
	flipped  = (-outflank_v * 2) & 0x0020202020202000ULL;

	outflank_h = OUTFLANK_5[(O >> 57) & 0x3f] & (P >> 56);
	flipped |= ((unsigned long long) FLIPPED_5[outflank_h]) << 57;

	flipped |= ((P << 7) & 0x0040000000000000ULL & O);

	outflank_d9 = (0x8000000000000000ULL >> __builtin_clzll((O & 0x0010080402000000ULL) ^ 0x0010080402010000ULL)) & P;
	flipped |= (-outflank_d9 * 2) & 0x0010080402000000ULL;

	return flipped;
}

/**
 * Compute flipped discs when playing on square G8.
 *
 * @param P player's disc pattern.
 * @param O opponent's disc pattern.
 * @return flipped disc pattern.
 */
static unsigned long long flip_G8(const unsigned long long P, const unsigned long long O)
{
	unsigned int outflank_h;
	unsigned long long flipped, outflank_v, outflank_d9;

	outflank_v = (0x8000000000000000ULL >> __builtin_clzll((O & 0x0040404040404000ULL) ^ 0x0040404040404040ULL)) & P;
	flipped  = (-outflank_v * 2) & 0x0040404040404000ULL;

	outflank_h = OUTFLANK_7[(O >> 56) & 0x3e] & (P >> 55);
	flipped |= (unsigned long long) ((-outflank_h) & 0x3e) << 56;

	outflank_d9 = (0x8000000000000000ULL >> __builtin_clzll((O & 0x0020100804020000ULL) ^ 0x0020100804020100ULL)) & P;
	flipped |= (-outflank_d9 * 2) & 0x0020100804020000ULL;

	return flipped;
}

/**
 * Compute flipped discs when playing on square H8.
 *
 * @param P player's disc pattern.
 * @param O opponent's disc pattern.
 * @return flipped disc pattern.
 */
static unsigned long long flip_H8(const unsigned long long P, const unsigned long long O)
{
	unsigned int outflank_h;
	unsigned long long flipped, outflank_v, outflank_d9;

	outflank_v = (0x8000000000000000ULL >> __builtin_clzll((O & 0x0080808080808000ULL) ^ 0x0080808080808080ULL)) & P;
	flipped  = (-outflank_v * 2) & 0x0080808080808000ULL;

	outflank_h = OUTFLANK_7[(O >> 57) & 0x3f] & (P >> 56);
	flipped |= (unsigned long long) ((-outflank_h) & 0x3f) << 57;

	outflank_d9 = (0x8000000000000000ULL >> __builtin_clzll((O & 0x0040201008040200ULL) ^ 0x0040201008040201ULL)) & P;
	flipped |= (-outflank_d9 * 2) & 0x0040201008040200ULL;

	return flipped;
}

/**
 * Compute (zero-) flipped discs when plassing.
 *
 * @param P player's disc pattern.
 * @param O opponent's disc pattern.
 * @return flipped disc pattern.
 */
static unsigned long long flip_pass(const unsigned long long P, const unsigned long long O)
{
	(void) P; // useless code to shut-up compiler warning
	(void) O;
	return 0;
}


/** Array of functions to compute flipped discs */
unsigned long long (*flip[])(const unsigned long long, const unsigned long long) = {
	flip_A1, flip_B1, flip_C1, flip_D1,
	flip_E1, flip_F1, flip_G1, flip_H1,
	flip_A2, flip_B2, flip_C2, flip_D2,
	flip_E2, flip_F2, flip_G2, flip_H2,
	flip_A3, flip_B3, flip_C3, flip_D3,
	flip_E3, flip_F3, flip_G3, flip_H3,
	flip_A4, flip_B4, flip_C4, flip_D4,
	flip_E4, flip_F4, flip_G4, flip_H4,
	flip_A5, flip_B5, flip_C5, flip_D5,
	flip_E5, flip_F5, flip_G5, flip_H5,
	flip_A6, flip_B6, flip_C6, flip_D6,
	flip_E6, flip_F6, flip_G6, flip_H6,
	flip_A7, flip_B7, flip_C7, flip_D7,
	flip_E7, flip_F7, flip_G7, flip_H7,
	flip_A8, flip_B8, flip_C8, flip_D8,
	flip_E8, flip_F8, flip_G8, flip_H8,
	flip_pass, flip_pass
};

