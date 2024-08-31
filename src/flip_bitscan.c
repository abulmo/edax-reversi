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
 * Once we get our 8-bits disc patterns, a first array (OUTFLANK) is used to
 * get the player's discs that surround the opponent discs:
 *  - outflank = OUTFLANK[x][O] & P
 * Because neighbor-of-x bits in the P are not in interest here, outflank
 * is stored in bitwise rotated form to reduce the table size.
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
 * If the OUTFLANK search is in MSB to LSB direction, lzcnt64 is used if 
 * available, or __builtin_bswap is used to use carry propagation backwards.
 *
 * @date 1998 - 2020
 * @author Richard Delorme
 * @author Toshihiko Okuhara
 * @version 4.4
 */

#include "bit_intrinsics.h"

#define LODWORD(l) ((unsigned int)(l))
#define HIDWORD(l) ((unsigned int)((l)>>32))

/** rotated outflank array (indexed with inner 6 bits) */
static const unsigned char OUTFLANK_2[64] = {	// ...ahgfe
	0x00, 0x10, 0x00, 0x00, 0x01, 0x11, 0x00, 0x00, 0x00, 0x10, 0x00, 0x00, 0x02, 0x12, 0x00, 0x00,
	0x00, 0x10, 0x00, 0x00, 0x01, 0x11, 0x00, 0x00, 0x00, 0x10, 0x00, 0x00, 0x04, 0x14, 0x00, 0x00,
	0x00, 0x10, 0x00, 0x00, 0x01, 0x11, 0x00, 0x00, 0x00, 0x10, 0x00, 0x00, 0x02, 0x12, 0x00, 0x00,
	0x00, 0x10, 0x00, 0x00, 0x01, 0x11, 0x00, 0x00, 0x00, 0x10, 0x00, 0x00, 0x08, 0x18, 0x00, 0x00
};

static const unsigned char OUTFLANK_3[64] = {	// ...bahgf
	0x00, 0x00, 0x10, 0x08, 0x00, 0x00, 0x00, 0x00, 0x01, 0x01, 0x11, 0x09, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x10, 0x08, 0x00, 0x00, 0x00, 0x00, 0x02, 0x02, 0x12, 0x0a, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x10, 0x08, 0x00, 0x00, 0x00, 0x00, 0x01, 0x01, 0x11, 0x09, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x10, 0x08, 0x00, 0x00, 0x00, 0x00, 0x04, 0x04, 0x14, 0x0c, 0x00, 0x00, 0x00, 0x00
};

static const unsigned char OUTFLANK_4[64] = {	// ...cbahg
	0x00, 0x00, 0x00, 0x00, 0x10, 0x10, 0x08, 0x04, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x01, 0x01, 0x01, 0x01, 0x11, 0x11, 0x09, 0x05, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x10, 0x10, 0x08, 0x04, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x02, 0x02, 0x02, 0x02, 0x12, 0x12, 0x0a, 0x06, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};

static const unsigned char OUTFLANK_5[64] = {	// ...dcbah
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x10, 0x10, 0x10, 0x10, 0x08, 0x08, 0x04, 0x02,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x11, 0x11, 0x11, 0x11, 0x09, 0x09, 0x05, 0x03,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};

/** flip array (indexed with rotated outflank) */
static const unsigned long long FLIPPED_2_H[25] = {	// ...ahgfe
	0x0000000000000000, 0x0808080808080808, 0x1818181818181818, 0x0000000000000000,
	0x3838383838383838, 0x0000000000000000, 0x0000000000000000, 0x0000000000000000,
	0x7878787878787878, 0x0000000000000000, 0x0000000000000000, 0x0000000000000000,
	0x0000000000000000, 0x0000000000000000, 0x0000000000000000, 0x0000000000000000,
	0x0202020202020202, 0x0a0a0a0a0a0a0a0a, 0x1a1a1a1a1a1a1a1a, 0x0000000000000000,
	0x3a3a3a3a3a3a3a3a, 0x0000000000000000, 0x0000000000000000, 0x0000000000000000,
	0x7a7a7a7a7a7a7a7a
};

static const unsigned long long FLIPPED_2_V[25] = {
	0x0000000000000000, 0x00000000ff000000, 0x000000ffff000000, 0x0000000000000000,
	0x0000ffffff000000, 0x0000000000000000, 0x0000000000000000, 0x0000000000000000,
	0x00ffffffff000000, 0x0000000000000000, 0x0000000000000000, 0x0000000000000000,
	0x0000000000000000, 0x0000000000000000, 0x0000000000000000, 0x0000000000000000,
	0x000000000000ff00, 0x00000000ff00ff00, 0x000000ffff00ff00, 0x0000000000000000,
	0x0000ffffff00ff00, 0x0000000000000000, 0x0000000000000000, 0x0000000000000000,
	0x00ffffffff00ff00
};

static const unsigned long long FLIPPED_3_H[21] = {	// ...bahgf
	0x0000000000000000, 0x1010101010101010, 0x3030303030303030, 0x0000000000000000,
	0x7070707070707070, 0x0000000000000000, 0x0000000000000000, 0x0000000000000000,
	0x0606060606060606, 0x1616161616161616, 0x3636363636363636, 0x0000000000000000,
	0x7676767676767676, 0x0000000000000000, 0x0000000000000000, 0x0000000000000000,
	0x0404040404040404, 0x1414141414141414, 0x3434343434343434, 0x0000000000000000,
	0x7474747474747474
};

static const unsigned long long FLIPPED_3_V[21] = {
	0x0000000000000000, 0x000000ff00000000, 0x0000ffff00000000, 0x0000000000000000,
	0x00ffffff00000000, 0x0000000000000000, 0x0000000000000000, 0x0000000000000000,
	0x0000000000ffff00, 0x000000ff00ffff00, 0x0000ffff00ffff00, 0x0000000000000000,
	0x00ffffff00ffff00, 0x0000000000000000, 0x0000000000000000, 0x0000000000000000,
	0x0000000000ff0000, 0x000000ff00ff0000, 0x0000ffff00ff0000, 0x0000000000000000, 
	0x00ffffff00ff0000
};

static const unsigned long long FLIPPED_4_H[19] = {	// ...cbahg
	0x0000000000000000, 0x2020202020202020, 0x6060606060606060, 0x0000000000000000,
	0x0e0e0e0e0e0e0e0e, 0x2e2e2e2e2e2e2e2e, 0x6e6e6e6e6e6e6e6e, 0x0000000000000000,
	0x0c0c0c0c0c0c0c0c, 0x2c2c2c2c2c2c2c2c, 0x6c6c6c6c6c6c6c6c, 0x0000000000000000,
	0x0000000000000000, 0x0000000000000000, 0x0000000000000000, 0x0000000000000000,
	0x0808080808080808, 0x2828282828282828, 0x6868686868686868
};

static const unsigned long long FLIPPED_4_V[19] = {
	0x0000000000000000, 0x0000ff0000000000, 0x00ffff0000000000, 0x0000000000000000,
	0x00000000ffffff00, 0x0000ff00ffffff00, 0x00ffff00ffffff00, 0x0000000000000000,
	0x00000000ffff0000, 0x0000ff00ffff0000, 0x00ffff00ffff0000, 0x0000000000000000,
	0x0000000000000000, 0x0000000000000000, 0x0000000000000000, 0x0000000000000000,
	0x00000000ff000000, 0x0000ff00ff000000, 0x00ffff00ff000000
};

static const unsigned long long FLIPPED_5_H[18] = {	// ...dcbah
	0x0000000000000000, 0x4040404040404040, 0x1e1e1e1e1e1e1e1e, 0x5e5e5e5e5e5e5e5e,
	0x1c1c1c1c1c1c1c1c, 0x5c5c5c5c5c5c5c5c, 0x0000000000000000, 0x0000000000000000,
	0x1818181818181818, 0x5858585858585858, 0x0000000000000000, 0x0000000000000000,
	0x0000000000000000, 0x0000000000000000, 0x0000000000000000, 0x0000000000000000,
	0x1010101010101010, 0x5050505050505050
};

static const unsigned long long FLIPPED_5_V[18] = {
	0x0000000000000000, 0x00ff000000000000, 0x000000ffffffff00, 0x00ff00ffffffff00,
	0x000000ffffff0000, 0x00ff00ffffff0000, 0x0000000000000000, 0x0000000000000000,
	0x000000ffff000000, 0x00ff00ffff000000, 0x0000000000000000, 0x0000000000000000,
	0x0000000000000000, 0x0000000000000000, 0x0000000000000000, 0x0000000000000000,
	0x000000ff00000000, 0x00ff00ff00000000
};


/*
 * Set all bits below the sole outflank bit if outfrank != 0
 */
#if __has_builtin(__builtin_subcll)
static inline unsigned long long OutflankToFlipmask(unsigned long long outflank) {
	unsigned long long flipmask, cy;
	flipmask = __builtin_subcll(outflank, 1, 0, &cy);
	return __builtin_addcll(flipmask, 0, cy, &cy);
}
#elif (defined(_M_X64) && (_MSC_VER >= 1800)) || (defined(__x86_64__) && defined(__GNUC__) && (__GNUC__ > 7 || (__GNUC__ == 7 && __GNUC_MINOR__ >= 2)))
static inline unsigned long long OutflankToFlipmask(unsigned long long outflank) {
	unsigned long long flipmask;
	unsigned char cy = _subborrow_u64(0, outflank, 1, &flipmask);
	_addcarry_u64(cy, flipmask, 0, &flipmask);
	return flipmask;
}
#else
	#define OutflankToFlipmask(outflank)	((outflank) - (unsigned int) ((outflank) != 0))
#endif

#if ((defined(__x86_64__) || defined(USE_GAS_X86)) && defined(__LZCNT__)) || defined(__ARM_FEATURE_CLZ) || defined(_MSC_VER)
	// Strictly, (long long) >> 64 is undefined in C, but either 0 bit (no change)
	// or 64 bit (zero out) shift will lead valid result (i.e. flipped == 0).
	#define	outflank_right(O,maskr)	(0x8000000000000000ULL >> lzcnt_u64(~(O) & (maskr)))
#elif defined(vertical_mirror)	// bswap to use carry propagation backwards - cannot be used for horizontal right
	// static inline unsigned long long outflank_right(unsigned long long O, unsigned long long maskr) {
	//	unsigned long long rOM = vertical_mirror(~(O) & maskr);
	//	return vertical_mirror(rOM & (-rOM));
	// }
	#define	outflank_right(O,maskr)	(vertical_mirror(vertical_mirror((O) | ~(maskr)) + 1) & (maskr))
#else	// with guardian bit to avoid __builtin_clz(0)
	#define	outflank_right(O,maskr)	(0x8000000000000000ULL >> __builtin_clzll(((O) & (((maskr) & ((maskr) - 1)))) ^ (maskr)))
#endif

// in case continuous from MSB
#if defined(__AVX2__) || defined(__LZCNT__) || defined(_MSC_VER)
	#define	outflank_right_H(O)	(0x80000000u >> lzcnt_u32(~(O)))
#else
	#define	outflank_right_H(O)	(0x80000000u >> __builtin_clz(~(O)))
#endif


/**
 * Compute flipped discs when playing on square A1.
 *
 * @param P player's disc pattern.
 * @param O opponent's disc pattern.
 * @return flipped disc pattern.
 */
static unsigned long long flip_A1(const unsigned long long P, const unsigned long long O)
{
	unsigned int outflank_h;
	unsigned long long flipped, outflank_v, outflank_d9;

	outflank_v = ((O | ~0x0101010101010100) + 1) & P & 0x0101010101010100;
	flipped = OutflankToFlipmask(outflank_v) & 0x0101010101010100;

	outflank_d9 = ((O | ~0x8040201008040200) + 1) & P & 0x8040201008040200;
	flipped |= OutflankToFlipmask(outflank_d9) & 0x8040201008040200;

	outflank_h = (unsigned char) (O + 0x02) & P;
	flipped |= outflank_h - ((unsigned int) (outflank_h != 0) << 1);

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
	unsigned int outflank_h;
	unsigned long long flipped, outflank_v, outflank_d9;

	outflank_v = ((O | ~0x0202020202020200) + 1) & P & 0x0202020202020200;
	flipped = OutflankToFlipmask(outflank_v) & 0x0202020202020200;

	outflank_d9 = ((O | ~0x0080402010080400) + 1) & P & 0x0080402010080400;
	flipped |= OutflankToFlipmask(outflank_d9) & 0x0080402010080400;

	outflank_h = (unsigned char) (O + 0x04) & P;
	flipped |= outflank_h - ((unsigned int) (outflank_h != 0) << 2);

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
	unsigned int outflank_h, outflank_d;
	unsigned long long flipped, outflank_v;

	outflank_v = ((O | ~0x0404040404040400) + 1) & P & 0x0404040404040400;
	flipped = OutflankToFlipmask(outflank_v) & 0x0404040404040400;

	outflank_d = OUTFLANK_2[(((HIDWORD(O) & 0x00000040) + (LODWORD(O) & 0x20100a04)) * 0x01010101) >> 25];
	outflank_d &= ((P & 0x0000804020110a04) * 0x0101010101010101) >> 52;	// hgfedcb[ahgfe]...
	flipped |= FLIPPED_2_H[outflank_d] & 0x0000004020100a04;	// A3C1H6

	outflank_h = OUTFLANK_2[(O >> 1) & 0x3f] & rotl8(P, 4);
	flipped |= (unsigned char) FLIPPED_2_H[outflank_h];

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
	unsigned int outflank_h, outflank_d;
	unsigned long long flipped, outflank_v;

	outflank_v = ((O | ~0x0808080808080800) + 1) & P & 0x0808080808080800;
	flipped = OutflankToFlipmask(outflank_v) & 0x0808080808080800;

	outflank_d = OUTFLANK_3[((LODWORD(O) & 0x40221408) * 0x01010101) >> 25];
	outflank_d &= ((P & 0x0000008041221408) * 0x0101010101010101) >> 53;	// hgfedc[bahgf]...
	flipped |= FLIPPED_3_H[outflank_d] & 0x0000000040221408;	// A4D1H5

	outflank_h = OUTFLANK_3[(O >> 1) & 0x3f] & rotl8(P, 3);
	flipped |= (unsigned char) FLIPPED_3_H[outflank_h];

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
	unsigned int outflank_h, outflank_d;
	unsigned long long flipped, outflank_v;

	outflank_v = ((O | ~0x1010101010101000) + 1) & P & 0x1010101010101000;
	flipped = OutflankToFlipmask(outflank_v) & 0x1010101010101000;

	outflank_d = OUTFLANK_4[((LODWORD(O) & 0x02442810) * 0x01010101) >> 25];
	outflank_d &= ((P & 0x0000000182442810) * 0x0101010101010101) >> 54;	// hgfed[cbahg]...
	flipped |= FLIPPED_4_H[outflank_d] & 0x0000000002442810;	// A5E1H4

	outflank_h = OUTFLANK_4[(O >> 1) & 0x3f] & rotl8(P, 2);
	flipped |= (unsigned char) FLIPPED_4_H[outflank_h];

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
	unsigned int outflank_h, outflank_d;
	unsigned long long flipped, outflank_v;

	outflank_v = ((O | ~0x2020202020202000) + 1) & P & 0x2020202020202000;
	flipped = OutflankToFlipmask(outflank_v) & 0x2020202020202000;

	outflank_d = OUTFLANK_5[(((HIDWORD(O) & 0x00000002) + (LODWORD(O) & 0x04085020)) * 0x01010101) >> 25];
	outflank_d &= ((P & 0x0000010204885020) * 0x0101010101010101) >> 55;	// hgfe[dcbah]...
	flipped |= FLIPPED_5_H[outflank_d] & 0x0000000204085020;	// A6F1H3

	outflank_h = OUTFLANK_5[(O >> 1) & 0x3f] & rotl8(P, 1);
	flipped |= (unsigned char) FLIPPED_5_H[outflank_h];

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

	outflank_v = ((O | ~0x4040404040404000) + 1) & P & 0x4040404040404000;
	flipped = OutflankToFlipmask(outflank_v) & 0x4040404040404000;

	outflank_d7 = ((O | ~0x0001020408102000) + 1) & P & 0x0001020408102000;
	flipped |= OutflankToFlipmask(outflank_d7) & 0x0001020408102000;

	outflank_h = outflank_right_H((unsigned int) O << 26) & ((unsigned int) P << 26);
	flipped |= (outflank_h * (unsigned int) -2) >> 26;

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

	outflank_v = ((O | ~0x8080808080808000) + 1) & P & 0x8080808080808000;
	flipped = OutflankToFlipmask(outflank_v) & 0x8080808080808000;

	outflank_d7 = ((O | ~0x0102040810204000) + 1) & P & 0x0102040810204000;
	flipped |= OutflankToFlipmask(outflank_d7) & 0x0102040810204000;

	outflank_h = outflank_right_H((unsigned int) O << 25) & ((unsigned int) P << 25);
	flipped |= (outflank_h * (unsigned int) -2) >> 25;

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

	outflank_v = ((O | ~0x0101010101010000) + 1) & P & 0x0101010101010000;
	flipped = OutflankToFlipmask(outflank_v) & 0x0101010101010000;

	outflank_d9 = ((O | ~0x4020100804020000) + 1) & P & 0x4020100804020000;
	flipped |= OutflankToFlipmask(outflank_d9) & 0x4020100804020000;

	outflank_h = (unsigned short) (O + 0x0200) & P;
	flipped |= (outflank_h - (outflank_h >> 8)) & 0x7e00;

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

	outflank_v = ((O | ~0x0202020202020000) + 1) & P & 0x0202020202020000;
	flipped = OutflankToFlipmask(outflank_v) & 0x0202020202020000;

	outflank_d9 = ((O | ~0x8040201008040000) + 1) & P & 0x8040201008040000;
	flipped |= OutflankToFlipmask(outflank_d9) & 0x8040201008040000;

	outflank_h = (unsigned short) (O + 0x0400) & P;
	flipped |= (outflank_h - (outflank_h >> 8)) & 0x7c00;

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
	unsigned int outflank_h, outflank_d;
	unsigned long long flipped, outflank_v;

	outflank_v = ((O | ~0x0404040404040000) + 1) & P & 0x0404040404040000;
	flipped = OutflankToFlipmask(outflank_v) & 0x0404040404040000;

	outflank_d = OUTFLANK_2[(((HIDWORD(O) & 0x00004020) + (LODWORD(O) & 0x100a0400)) * 0x01010101) >> 25];
	outflank_d &= ((P & 0x00804020110a0400) * 0x0101010101010101) >> 52;	// hgfedcb[ahgfe]...
	flipped |= FLIPPED_2_H[outflank_d] & 0x00004020100a0400;	// A4C2H7

	outflank_h = OUTFLANK_2[(O >> 9) & 0x3f] & rotl8(P >> 8, 4);
	flipped |= (unsigned char) FLIPPED_2_H[outflank_h] << 8;

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
	unsigned int outflank_h, outflank_d;
	unsigned long long flipped, outflank_v;

	outflank_v = ((O | ~0x0808080808080000) + 1) & P & 0x0808080808080000;
	flipped = OutflankToFlipmask(outflank_v) & 0x0808080808080000;

	outflank_d = OUTFLANK_3[(((unsigned int) (O >> 8) & 0x40221408) * 0x01010101) >> 25];
	outflank_d &= ((P & 0x0000804122140800) * 0x0101010101010101) >> 53;	// hgfedc[bahgf]...
	flipped |= FLIPPED_3_H[outflank_d] & 0x0000004022140800;	// A5D2H6

	outflank_h = OUTFLANK_3[(O >> 9) & 0x3f] & rotl8(P >> 8, 3);
	flipped |= (unsigned char) FLIPPED_3_H[outflank_h] << 8;

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
	unsigned int outflank_h, outflank_d;
	unsigned long long flipped, outflank_v;

	outflank_v = ((O | ~0x1010101010100000) + 1) & P & 0x1010101010100000;
	flipped = OutflankToFlipmask(outflank_v) & 0x1010101010100000;

	outflank_d = OUTFLANK_4[(((unsigned int) (O >> 8) & 0x02442810) * 0x01010101) >> 25];
	outflank_d &= ((P & 0x0000018244281000) * 0x0101010101010101) >> 54;	// hgfed[cbahg]...
	flipped |= FLIPPED_4_H[outflank_d] & 0x0000000244281000;	// A6E2H5

	outflank_h = OUTFLANK_4[(O >> 9) & 0x3f] & rotl8(P >> 8, 2);
	flipped |= (unsigned char) FLIPPED_4_H[outflank_h] << 8;

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
	unsigned int outflank_h, outflank_d;
	unsigned long long flipped, outflank_v;

	outflank_v = ((O | ~0x2020202020200000) + 1) & P & 0x2020202020200000;
	flipped = OutflankToFlipmask(outflank_v) & 0x2020202020200000;

	outflank_d = OUTFLANK_5[(((HIDWORD(O) & 0x00000204) + (LODWORD(O) & 0x08502000)) * 0x01010101) >> 25];
	outflank_d &= ((P & 0x0001020488502000) * 0x0101010101010101) >> 55;	// hgfe[dcbah]...
	flipped |= FLIPPED_5_H[outflank_d] & 0x0000020408502000;	// A7F2H4

	outflank_h = OUTFLANK_5[(O >> 9) & 0x3f] & rotl8(P >> 8, 1);
	flipped |= (unsigned char) FLIPPED_5_H[outflank_h] << 8;

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

	outflank_v = ((O | ~0x4040404040400000) + 1) & P & 0x4040404040400000;
	flipped = OutflankToFlipmask(outflank_v) & 0x4040404040400000;

	outflank_d7 = ((O | ~0x0102040810200000) + 1) & P & 0x0102040810200000;
	flipped |= (outflank_d7 - (unsigned int) (outflank_d7 != 0)) & 0x0102040810200000;

	outflank_h = outflank_right_H(((unsigned int) O >> 9) << 27) & ((unsigned int) P << 18);
	flipped |= (outflank_h * (unsigned int) -2) >> 18;

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

	outflank_v = ((O | ~0x8080808080800000) + 1) & P & 0x8080808080800000;
	flipped = OutflankToFlipmask(outflank_v) & 0x8080808080800000;

	outflank_d7 = ((O | ~0x0204081020400000) + 1) & P & 0x0204081020400000;
	flipped |= OutflankToFlipmask(outflank_d7) & 0x0204081020400000;

	outflank_h = outflank_right_H(((unsigned int) O >> 9) << 26) & ((unsigned int) P << 17);
	flipped |= (outflank_h * (unsigned int) -2) >> 17;

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
	unsigned int outflank_h, outflank_a1a3f8, outflank_a8a3c1;
	unsigned long long flipped;

	outflank_a1a3f8 = OUTFLANK_2[((O & 0x0010080402010100) * 0x0102040404040404) >> 57];
	outflank_a1a3f8 &= ((P & 0x2010080402010101) * 0x8000000002020202) >> 59;	// 18765
	flipped = FLIPPED_2_V[outflank_a1a3f8] & 0x0010080402010100;

	outflank_a8a3c1 = OUTFLANK_5[((O & 0x0001010101010200) * 0x2020201008040201) >> 57];
	outflank_a8a3c1 &= ((P & 0x0101010101010204) * 0x0200000080402010) >> 59;	// 56781
	flipped |= vertical_mirror(FLIPPED_5_V[outflank_a8a3c1]) & 0x0001010101010200;

	outflank_h = ((O & 0x007e0000) + 0x00020000) & P;
	flipped |= (outflank_h - (outflank_h >> 8)) & 0x007e0000;

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
	unsigned int outflank_h, outflank_b1b3g8, outflank_b8b3d1;
	unsigned long long flipped;

	outflank_b1b3g8 = OUTFLANK_2[((O & 0x0020100804020200) * 0x0081020202020202) >> 57];
	outflank_b1b3g8 &= ((P & 0x4020100804020202) * 0x4000000001010101) >> 59;	// 18765
	flipped = FLIPPED_2_V[outflank_b1b3g8] & 0x0020100804020200;

	outflank_b8b3d1 = OUTFLANK_5[((O & 0x0002020202020400) * 0x0010100804020100) >> 57];
	outflank_b8b3d1 &= ((P & 0x0202020202020408) * 0x0100000040201008) >> 59;	// 56781
	flipped |= vertical_mirror(FLIPPED_5_V[outflank_b8b3d1]) & 0x0002020202020400;

	outflank_h = ((O & 0x007c0000) + 0x00040000) & P;
	flipped |= (outflank_h - (outflank_h >> 8)) & 0x007c0000;

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
	unsigned int outflank_h, outflank_v, outflank_d9;
	unsigned long long flipped;

	outflank_v = OUTFLANK_2[((O & 0x0004040404040400) * 0x0040810204081020) >> 57];
	outflank_v &= ((P & 0x0404040404040404) * 0x2000000002040810) >> 59;	// 18765
	flipped = FLIPPED_2_V[outflank_v] & 0x0004040404040400;

	outflank_h = OUTFLANK_2[(O >> 17) & 0x3f] & rotl8(P >> 16, 4);
	flipped |= (unsigned char) FLIPPED_2_H[outflank_h] << 16;

	flipped |= (((P >> 7) | (P << 7)) & 0x000000002000800) & O;

	outflank_d9 = OUTFLANK_2[(((HIDWORD(O) & 0x00402010) + (LODWORD(O) & 0x08040200)) * 0x01010101) >> 25];
	outflank_d9 &= rotl8((((HIDWORD(P) & 0x80402010) + (LODWORD(P) & 0x08040201)) * 0x01010101) >> 24, 4);	// (h8)
	flipped |= FLIPPED_2_H[outflank_d9] & 0x0040201008040200;

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
	unsigned int outflank_h, outflank_v, outflank_d;
	unsigned long long flipped;

	outflank_v = OUTFLANK_2[((O & 0x0008080808080800) * 0x0020408102040810) >> 57];
	outflank_v &= ((P & 0x0808080808080808) * 0x1020408001020408) >> 59;	// 18765
	flipped = FLIPPED_2_V[outflank_v] & 0x0008080808080800;

	outflank_h = OUTFLANK_3[(O >> 17) & 0x3f] & rotl8(P >> 16, 3);
	flipped |= (unsigned char) FLIPPED_3_H[outflank_h] << 16;

	outflank_d = OUTFLANK_3[(((unsigned int) (O >> 16) & 0x40221408) * 0x01010101) >> 25];
	outflank_d &= ((P & 0x0080412214080000) * 0x0101010101010101) >> 53;	// hgfedc[bahgf]...
	flipped |= FLIPPED_3_H[outflank_d] & 0x0000402214080000;	// A6D3H7

	flipped |= (((P << 7) & 0x0000000000001000) | ((P << 9) & 0x000000000000400)) & O;

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
	unsigned int outflank_h, outflank_v, outflank_d;
	unsigned long long flipped;

	outflank_v = OUTFLANK_2[((O & 0x0010101010101000) * 0x0010204081020408) >> 57];
	outflank_v &= ((P & 0x1010101010101010) * 0x0810204000810204) >> 59;	// 18765
	flipped = FLIPPED_2_V[outflank_v] & 0x0010101010101000;

	outflank_h = OUTFLANK_4[(O >> 17) & 0x3f] & rotl8(P >> 16, 2);
	flipped |= (unsigned char) FLIPPED_4_H[outflank_h] << 16;

	outflank_d = OUTFLANK_4[(((unsigned int) (O >> 16) & 0x02442810) * 0x01010101) >> 25];
	outflank_d &= ((P & 0x0001824428100000) * 0x0101010101010101) >> 54;	// hgfed[cbahg]...
	flipped |= FLIPPED_4_H[outflank_d] & 0x0000024428100000;	// A7E3H6

	flipped |= (((P << 7) & 0x0000000000002000) | ((P << 9) & 0x000000000000800)) & O;

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
	unsigned int outflank_h, outflank_v, outflank_d7;
	unsigned long long flipped;

	outflank_v = OUTFLANK_2[((O & 0x0020202020202000) * 0x0008102040810204) >> 57];
	outflank_v &= ((P & 0x2020202020202020) * 0x0408102000408102) >> 59;	// 18765
	flipped = FLIPPED_2_V[outflank_v] & 0x0020202020202000;

	outflank_h = OUTFLANK_5[(O >> 17) & 0x3f] & rotl8(P >> 16, 1);
	flipped |= (unsigned char) FLIPPED_5_H[outflank_h] << 16;

	outflank_d7 = OUTFLANK_5[(((HIDWORD(O) & 0x00020408) + (LODWORD(O) & 0x10204000)) * 0x01010101) >> 25];
	outflank_d7 &= ((P & 0x0102040810204080) * 0x0010000010101010) >> 59;	// dcbah
	flipped |= FLIPPED_5_H[outflank_d7] & 0x0002040810204000;

	flipped |= (((P >> 9) | (P << 9)) & 0x0000000040001000) & O;

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
	unsigned int outflank_h, outflank_e1g3g8, outflank_b8g3g1;
	unsigned long long flipped;

	outflank_e1g3g8 = OUTFLANK_2[((O & 0x0040404040402000) * 0x0010101020408102) >> 57];
	outflank_e1g3g8 &= ((P & 0x4040404040402010) * 0x0800000000204081) >> 59;	// 18765
	flipped = FLIPPED_2_V[outflank_e1g3g8] & 0x0040404040402000;

	outflank_b8g3g1 = OUTFLANK_5[((O & 0x0004081020404000) * 0x0402010101010101) >> 58];
	outflank_b8g3g1 &= ((P & 0x0204081020404040) * 0x0020000008080808) >> 59;	// 43218
	flipped |= vertical_mirror(FLIPPED_5_V[outflank_b8g3g1]) & 0x0004081020404000;

	outflank_h = outflank_right_H(((unsigned int) O >> 17) << 27) & (unsigned int) (P << 10);
	flipped |= (outflank_h * (unsigned int) -2) >> 10;

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
	unsigned int outflank_h, outflank_f1h3h8, outflank_c8h3h1;
	unsigned long long flipped;

	outflank_f1h3h8 = OUTFLANK_2[((O & 0x0080808080804000) * 0x0008080810204081) >> 57];
	outflank_f1h3h8 &= rotl8(((P & 0x8080808080804020) * 0x0008080810204081) >> 56, 4);	// (h8)
	flipped = FLIPPED_2_V[outflank_f1h3h8] & 0x0080808080804000;

	outflank_c8h3h1 = OUTFLANK_5[((O & 0x0008102040808000) * 0x0000804040404040) >> 57];
	outflank_c8h3h1 &= ((P & 0x0408102040808080) * 0x0010000004040404) >> 59;	// 43218
	flipped |= vertical_mirror(FLIPPED_5_V[outflank_c8h3h1]) & 0x0008102040808000;

	outflank_h = outflank_right_H(((unsigned int) O >> 17) << 26) & (unsigned int) (P << 9);
	flipped |= (outflank_h * (unsigned int) -2) >> 9;

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
	unsigned int outflank_h, outflank_a1a4e8, outflank_a8a4d1;
	unsigned long long flipped;

	outflank_a1a4e8 = OUTFLANK_3[((O & 0x0008040201010100) * 0x0102040808080808) >> 57];
	outflank_a1a4e8 &= ((P & 0x1008040201010101) * 0x4080000000020202) >> 59;	// 21876
	flipped = FLIPPED_3_V[outflank_a1a4e8] & 0x0008040201010100;

	outflank_a8a4d1 = OUTFLANK_4[((O & 0x0001010101020400) * 0x1010101008040201) >> 57];
	outflank_a8a4d1 &= ((P & 0x0101010101020408) * 0x0202000000804020) >> 59;	// 67812
	flipped |= vertical_mirror(FLIPPED_4_V[outflank_a8a4d1]) & 0x0001010101020400;

	outflank_h = ((unsigned int) O + 0x02000000) & P;
	flipped |= (outflank_h - (outflank_h >> 8)) & 0x7e000000;

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
	unsigned int outflank_h, outflank_b1b4f8, outflank_b8b4e1;
	unsigned long long flipped;

	outflank_b1b4f8 = OUTFLANK_3[((O & 0x0010080402020200) * 0x0081020404040404) >> 57];
	outflank_b1b4f8 &= ((P & 0x2010080402020202) * 0x2040000000010101) >> 59;	// 21876
	flipped = FLIPPED_3_V[outflank_b1b4f8] & 0x0010080402020200;

	outflank_b8b4e1 = OUTFLANK_4[((O & 0x0002020202040800) * 0x1010101008040201) >> 58];
	outflank_b8b4e1 &= ((P & 0x0202020202040810) * 0x0101000000402010) >> 59;	// 67812
	flipped |= vertical_mirror(FLIPPED_4_V[outflank_b8b4e1]) & 0x0002020202040800;

	outflank_h = ((unsigned int) O + 0x04000000) & P;
	flipped |= (outflank_h - (outflank_h >> 8)) & 0x7c000000;

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
	unsigned int outflank_h, outflank_c1c4g8, outflank_c8c4f1;
	unsigned long long flipped;

	outflank_c1c4g8 = OUTFLANK_3[((O & 0x0020100804040400) * 0x0040810202020202) >> 57];
	outflank_c1c4g8 &= rotl8(((P & 0x4020100804040404) * 0x0040810202020202) >> 56, 3);	// (g8)
	flipped = FLIPPED_3_V[outflank_c1c4g8] & 0x0020100804040400;

	outflank_c8c4f1 = OUTFLANK_4[((O & 0x0004040404081000) * 0x0404040402010080) >> 57];
	outflank_c8c4f1 &= ((P & 0x0404040404081020) * 0x0080800000201008) >> 59;	// 67812
	flipped |= vertical_mirror(FLIPPED_4_V[outflank_c8c4f1]) & 0x0004040404081000;

	outflank_h = OUTFLANK_2[(O >> 25) & 0x3f] & rotl8(P >> 24, 4);
	flipped |= (unsigned char) FLIPPED_2_H[outflank_h] << 24;

	flipped |= (((P << 9) & 0x00000000000020000) | ((P >> 7) & 0x00000000200000000)) & O;

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

	outflank_v = OUTFLANK_3[((O & 0x0008080808080800) * 0x0020408102040810) >> 57];
	outflank_v &= ((P & 0x0808080808080808) * 0x0810000000010204) >> 59;	// 21876
	flipped = FLIPPED_3_V[outflank_v] & 0x0008080808080800;

	outflank_h = OUTFLANK_3[(O >> 25) & 0x3f] & rotl8(P >> 24, 3);
	flipped |= (unsigned char) FLIPPED_3_H[outflank_h] << 24;

	outflank_d7 = OUTFLANK_3[(((HIDWORD(O) & 0x00000204) + (LODWORD(O) & 0x08102000)) * 0x01010101) >> 25];
	outflank_d7 &= ((P & 0x0001020408102040) * 0x0040400000404000) >> 59;	// ba0gf
	flipped |= FLIPPED_3_H[outflank_d7] & 0x0000020408102000;

	outflank_d9 = OUTFLANK_3[(((HIDWORD(O) & 0x00402010) + (LODWORD(O) & 0x08040200)) * 0x01010101) >> 25];
	outflank_d9 &= rotl8((((HIDWORD(P) & 0x80402010) + (LODWORD(P) & 0x08040201)) * 0x01010101) >> 24, 3);	// (h8)
	flipped |= FLIPPED_3_H[outflank_d9] & 0x0040201008040200;

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

	outflank_v = OUTFLANK_3[((O & 0x0010101010101000) * 0x0010204081020408) >> 57];
	outflank_v &= ((P & 0x1010101010101010) * 0x0408000000008102) >> 59;	// 21876
	flipped = FLIPPED_3_V[outflank_v] & 0x0010101010101000;

	outflank_h = OUTFLANK_4[(O >> 25) & 0x3f] & rotl8(P >> 24, 2);
	flipped |= (unsigned char) FLIPPED_4_H[outflank_h] << 24;

	outflank_d7 = OUTFLANK_4[(((HIDWORD(O) & 0x00020408) + (LODWORD(O) & 0x10204000)) * 0x01010101) >> 25];
	outflank_d7 &= ((P & 0x0102040810204080) * 0x0020200000202020) >> 59;	// cbahg
	flipped |= FLIPPED_4_H[outflank_d7] & 0x0002040810204000;

	outflank_d9 = OUTFLANK_4[(((HIDWORD(O) & 0x00004020) + (LODWORD(O) & 0x10080400)) * 0x01010101) >> 25];
	outflank_d9 &= ((P & 0x0080402010080402) * 0x0404000000040404) >> 56;	// cbahg
	flipped |= FLIPPED_4_H[outflank_d9] & 0x0000402010080400;

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
	unsigned int outflank_h, outflank_c1f4f8, outflank_b8f4f1;
	unsigned long long flipped;

	outflank_c1f4f8 = OUTFLANK_3[((O & 0x0020202020100800) * 0x0040404040810204) >> 57];
	outflank_c1f4f8 &= ((P & 0x2020202020100804) * 0x1010000000004081) >> 59;	// 21876
	flipped = FLIPPED_3_V[outflank_c1f4f8] & 0x0020202020100800;

	outflank_b8f4f1 = OUTFLANK_4[((O & 0x0004081020202000) * 0x0804020101010101) >> 58];
	outflank_b8f4f1 &= ((P & 0x0204081020202020) * 0x0080400000101010) >> 59;	// 67812
	flipped |= vertical_mirror(FLIPPED_4_V[outflank_b8f4f1]) & 0x0004081020202000;

	outflank_h = OUTFLANK_5[(O >> 25) & 0x3f] & rotl8(P >> 24, 1);
	flipped |= (unsigned char) FLIPPED_5_H[outflank_h] << 24;

	flipped |= (((P << 7) & 0x0000000000400000) | ((P >> 9) & 0x0000004000000000)) & O;

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
	unsigned int outflank_h, outflank_d1g4g8, outflank_c8g4g1;
	unsigned long long flipped;

	outflank_d1g4g8 = OUTFLANK_3[((O & 0x0040404040201000) * 0x0020202020408102) >> 57];
	outflank_d1g4g8 &= rotl8(((P & 0x4040404040201008) * 0x0020202020408102) >> 56, 3);	// (g8)
	flipped = FLIPPED_3_V[outflank_d1g4g8] & 0x0040404040201000;

	outflank_c8g4g1 = OUTFLANK_4[((O & 0x0008102040404000) * 0x0001008040404040) >> 57];
	outflank_c8g4g1 &= ((P & 0x0408102040404040) * 0x0040200000080808) >> 59;	// 67812
	flipped |= vertical_mirror(FLIPPED_4_V[outflank_c8g4g1]) & 0x0008102040404000;

	outflank_h = outflank_right_H(((unsigned int) O >> 25) << 27) & (unsigned int) (P << 2);
	flipped |= (outflank_h * (unsigned int) -2) >> 2;

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
	unsigned int outflank_h, outflank_e1h4h8, outflank_d8h4h1;
	unsigned long long flipped;

	outflank_e1h4h8 = OUTFLANK_3[((O & 0x0080808080402000) * 0x0010101010204081) >> 57];
	outflank_e1h4h8 &= rotl8(((P & 0x8080808080402010) * 0x0010101010204081) >> 56, 3);	// (h8)
	flipped = FLIPPED_3_V[outflank_e1h4h8] & 0x0080808080402000;

	outflank_d8h4h1 = OUTFLANK_4[((O & 0x0010204080808000) * 0x0000804020202020) >> 57];
	outflank_d8h4h1 &= ((P & 0x0810204080808080) * 0x0020100000040404) >> 59;	// 67812
	flipped |= vertical_mirror(FLIPPED_4_V[outflank_d8h4h1]) & 0x0010204080808000;

	outflank_h = outflank_right_H(((unsigned int) O >> 25) << 26) & (unsigned int) (P << 1);
	flipped |= (outflank_h * (unsigned int) -2) >> 1;

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
	unsigned int outflank_h, outflank_a1a5d8, outflank_a8a5e1;
	unsigned long long flipped;

	outflank_a1a5d8 = OUTFLANK_4[((O & 0x0004020101010100) * 0x0102040810101010) >> 57];
	outflank_a1a5d8 &= ((P & 0x0804020101010101) * 0x2040800000000202) >> 59;	// 32187
	flipped = FLIPPED_4_V[outflank_a1a5d8] & 0x0004020101010100;

	outflank_a8a5e1 = OUTFLANK_3[((O & 0x0001010102040800) * 0x0808080808040201) >> 57];
	outflank_a8a5e1 &= ((P & 0x0101010102040810) * 0x0202020000008040) >> 59;	// 78123
	flipped |= vertical_mirror(FLIPPED_3_V[outflank_a8a5e1]) & 0x0001010102040800;

	outflank_h = ((unsigned int) (O >> 8) + 0x02000000) & (unsigned int) (P >> 8);
	flipped |= (((unsigned long long) outflank_h << 8) - outflank_h) & 0x0000007e00000000;

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
	unsigned int outflank_h, outflank_b1b5e8, outflank_b8b5f1;
	unsigned long long flipped;

	outflank_b1b5e8 = OUTFLANK_4[((O & 0x0008040202020200) * 0x0081020408080808) >> 57];
	outflank_b1b5e8 &= ((P & 0x1008040202020202) * 0x1020400000000101) >> 59;	// 32187
	flipped = FLIPPED_4_V[outflank_b1b5e8] & 0x0008040202020200;

	outflank_b8b5f1 = OUTFLANK_3[((O & 0x0002020204081000) * 0x0808080808040201) >> 58];
	outflank_b8b5f1 &= ((P & 0x0202020204081020) * 0x0101010000004020) >> 59;	// 78123
	flipped |= vertical_mirror(FLIPPED_3_V[outflank_b8b5f1]) & 0x0002020204081000;

	outflank_h = ((unsigned int) (O >> 8) + 0x04000000) & (unsigned int) (P >> 8);
	flipped |= (((unsigned long long) outflank_h << 8) - outflank_h) & 0x0000007c00000000;

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
	unsigned int outflank_h, outflank_c1c5f8, outflank_c8c5g1;
	unsigned long long flipped;

	outflank_c1c5f8 = OUTFLANK_4[((O & 0x0010080404040400) * 0x0040810204040404) >> 57];
	outflank_c1c5f8 &= rotl8(((P & 0x2010080404040404) * 0x0040810204040404) >> 56, 2);	// (f8)
	flipped = FLIPPED_4_V[outflank_c1c5f8] & 0x0010080404040400;

	outflank_c8c5g1 = OUTFLANK_3[((O & 0x0004040408102000) * 0x0002020202010080) >> 57];
	outflank_c8c5g1 &= ((P & 0x0404040408102040) * 0x0080808000002010) >> 59;	// 78123
	flipped |= vertical_mirror(FLIPPED_3_V[outflank_c8c5g1]) & 0x0004040408102000;

	outflank_h = OUTFLANK_2[(O >> 33) & 0x3f] & rotl8(P >> 32, 4);
	flipped |= (unsigned long long)(unsigned char) FLIPPED_2_H[outflank_h] << 32;

	flipped |= (((P << 9) & 0x0000000002000000) | ((P >> 7) & 0x0000020000000000)) & O;

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

	outflank_v = OUTFLANK_4[((O & 0x0008080808080800) * 0x0020408102040810) >> 57];
	outflank_v &= ((P & 0x0808080808080808) * 0x0408100000000102) >> 59;	// 32187
	flipped = FLIPPED_4_V[outflank_v] & 0x0008080808080800;

	outflank_h = OUTFLANK_3[(O >> 33) & 0x3f] & rotl8(P >> 32, 3);
	flipped |= (unsigned long long)(unsigned char) FLIPPED_3_H[outflank_h] << 32;

	outflank_d7 = OUTFLANK_3[(((HIDWORD(O) & 0x00020408) + (LODWORD(O) & 0x10204000)) * 0x01010101) >> 25];
	outflank_d7 &= ((P & 0x0102040810204080) * 0x0040404000004040) >> 59;	// bahgf
	flipped |= FLIPPED_3_H[outflank_d7] & 0x0002040810204000;

	outflank_d9 = OUTFLANK_3[(((HIDWORD(O) & 0x00201008) + (LODWORD(O) & 0x04020000)) * 0x01010101) >> 25];
	outflank_d9 &= rotl8((((HIDWORD(P) & 0x40201008) + (LODWORD(P) & 0x04020100)) * 0x01010101) >> 24, 3);	// (g8)
	flipped |= FLIPPED_3_H[outflank_d9] & 0x0020100804020000;

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

	outflank_v = OUTFLANK_4[((O & 0x0010101010101000) * 0x0010204081020408) >> 57];
	outflank_v &= ((P & 0x1010101010101010) * 0x0204080000000081) >> 59;	// 32187
	flipped = FLIPPED_4_V[outflank_v] & 0x0010101010101000;

	outflank_h = OUTFLANK_4[(O >> 33) & 0x3f] & rotl8(P >> 32, 2);
	flipped |= (unsigned long long)(unsigned char) FLIPPED_4_H[outflank_h] << 32;

	outflank_d7 = OUTFLANK_4[(((HIDWORD(O) & 0x00040810) + (LODWORD(O) & 0x20400000)) * 0x01010101) >> 25];
	outflank_d7 &= ((P & 0x0204081020408000) * 0x0000202000002020) >> 59;	// cb0hg
	flipped |= FLIPPED_4_H[outflank_d7] & 0x0004081020400000;

	outflank_d9 = OUTFLANK_4[(((HIDWORD(O) & 0x00402010) + (LODWORD(O) & 0x08040200)) * 0x01010101) >> 25];
	outflank_d9 &= rotl8((((HIDWORD(P) & 0x80402010) + (LODWORD(P) & 0x08040201)) * 0x01010101) >> 24, 2);	// (h8)
	flipped |= FLIPPED_4_H[outflank_d9] & 0x0040201008040200;

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

	outflank_b1f5f8 = OUTFLANK_4[((O & 0x0020202010080400) * 0x0080808080810204) >> 57];
	outflank_b1f5f8 &= rotl8(((P & 0x2020202010080402) * 0x0080808080810204) >> 56, 2);	// (f8)
	flipped = FLIPPED_4_V[outflank_b1f5f8] & 0x0020202010080400;

	outflank_c8f5f1 = OUTFLANK_3[((O & 0x0008102020202000) * 0x0002010080404040) >> 57];
	outflank_c8f5f1 &= ((P & 0x0408102020202020) * 0x0100804000001010) >> 59;	// 78123
	flipped |= vertical_mirror(FLIPPED_3_V[outflank_c8f5f1]) & 0x0008102020202000;

	outflank_h = OUTFLANK_5[(O >> 33) & 0x3f] & rotl8(P >> 32, 1);
	flipped |= (unsigned long long)(unsigned char) FLIPPED_5_H[outflank_h] << 32;

	flipped |= (((P << 7) & 0x0000000040000000) | ((P >> 9) & 0x0000400000000000)) & O;

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

	outflank_c1g5g8 = OUTFLANK_4[((O & 0x0040404020100800) * 0x0040404040408102) >> 57];
	outflank_c1g5g8 &= rotl8(((P & 0x4040404020100804) * 0x0040404040408102) >> 56, 2);	// (g8)
	flipped = FLIPPED_4_V[outflank_c1g5g8] & 0x0040404020100800;

	outflank_d8g5g1 = OUTFLANK_3[((O & 0x0010204040404000) * 0x0001008040202020) >> 57];
	outflank_d8g5g1 &= ((P & 0x0810204040404040) * 0x0080402000000808) >> 59;	// 78123
	flipped |= vertical_mirror(FLIPPED_3_V[outflank_d8g5g1]) & 0x0010204040404000;

	outflank_h = outflank_right_H((unsigned int) (O >> 33) << 27) & (unsigned int) (P >> 6);
	flipped |= (unsigned long long) (outflank_h * (unsigned int) -2) << 6;

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

	outflank_d1h5h8 = OUTFLANK_4[((O & 0x0080808040201000) * 0x0020202020204081) >> 57];
	outflank_d1h5h8 &= rotl8(((P & 0x8080808040201008) * 0x0020202020204081) >> 56, 2);	// (h8)
	flipped = FLIPPED_4_V[outflank_d1h5h8] & 0x0080808040201000;

	outflank_e8h5h1 = OUTFLANK_3[((O & 0x0020408080808000) * 0x0000804020101010) >> 57];
	outflank_e8h5h1 &= ((P & 0x1020408080808080) * 0x0040201000000404) >> 59;	// 78123
	flipped |= vertical_mirror(FLIPPED_3_V[outflank_e8h5h1]) & 0x0020408080808000;

	outflank_h = outflank_right_H((unsigned int) (O >> 33) << 26) & (unsigned int) (P >> 7);
	flipped |= (unsigned long long) (outflank_h * (unsigned int) -2) << 7;

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
	unsigned int outflank_h, outflank_a1a6c8, outflank_a8a6f1;
	unsigned long long flipped;

	outflank_a1a6c8 = OUTFLANK_5[((O & 0x0002010101010100) * 0x0102040810202020) >> 57];
	outflank_a1a6c8 &= ((P & 0x0402010101010101) * 0x1020408000000002) >> 59;	// 43218
	flipped = FLIPPED_5_V[outflank_a1a6c8] & 0x0002010101010100;

	outflank_a8a6f1 = OUTFLANK_2[((O & 0x0001010204081000) * 0x0404040404040201) >> 57];
	outflank_a8a6f1 &= ((P & 0x0101010204081020) * 0x0202020200000080) >> 59;	// 81234
	flipped |= vertical_mirror(FLIPPED_2_V[outflank_a8a6f1]) & 0x0001010204081000;

	outflank_h = ((unsigned int) (O >> 16) + 0x02000000) & (unsigned int) (P >> 16);
	flipped |= (((unsigned long long) outflank_h << 16) - outflank_h) & 0x00007e0000000000;

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
	unsigned int outflank_h, outflank_b1b6d8, outflank_b8b6g1;
	unsigned long long flipped;

	outflank_b1b6d8 = OUTFLANK_5[((O & 0x0004020202020200) * 0x0081020408101010) >> 57];
	outflank_b1b6d8 &= ((P & 0x0804020202020202) * 0x0810204000000001) >> 59;	// 43218
	flipped = FLIPPED_5_V[outflank_b1b6d8] & 0x0004020202020200;

	outflank_b8b6g1 = OUTFLANK_2[((O & 0x0002020408102000) * 0x0404040404040201) >> 58];
	outflank_b8b6g1 &= ((P & 0x0202020408102040) * 0x0101010100000040) >> 59;	// 81234
	flipped |= vertical_mirror(FLIPPED_2_V[outflank_b8b6g1]) & 0x0002020408102000;

	outflank_h = ((unsigned int) (O >> 16) + 0x04000000) & (unsigned int) (P >> 16);
	flipped |= (((unsigned long long) outflank_h << 16) - outflank_h) & 0x00007c0000000000;

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
	unsigned int outflank_h, outflank_v, outflank_d7;
	unsigned long long flipped;

	outflank_v = OUTFLANK_5[((O & 0x0004040404040400) * 0x0040810204081020) >> 57];
	outflank_v &= ((P & 0x0404040404040404) * 0x0408102000000002) >> 59;	// 43218
	flipped = FLIPPED_5_V[outflank_v] & 0x0004040404040400;

	outflank_h = OUTFLANK_2[(O >> 41) & 0x3f] & rotl8(P >> 40, 4);
	flipped |= (unsigned long long)(unsigned char) FLIPPED_2_H[outflank_h] << 40;

	outflank_d7 = OUTFLANK_2[(((HIDWORD(O) & 0x00020408) + (LODWORD(O) & 0x10204000)) * 0x01010101) >> 25];
	outflank_d7 &= ((P & 0x0102040810204080) * 0x0080808080000080) >> 59;	// ahgfe
	flipped |= FLIPPED_2_H[outflank_d7] & 0x0002040810204000;

	flipped |= ((P >> 9) | (P << 9)) & 0x0008000200000000 & O;

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
	unsigned int outflank_h, outflank_v, outflank_d;
	unsigned long long flipped;

	outflank_v = OUTFLANK_5[((O & 0x0008080808080800) * 0x0020408102040810) >> 57];
	outflank_v &= ((P & 0x0808080808080808) * 0x0204081020408001) >> 59;	// 43218
	flipped = FLIPPED_5_V[outflank_v] & 0x0008080808080800;

	outflank_h = OUTFLANK_3[(O >> 41) & 0x3f] & rotl8(P >> 40, 3);
	flipped |= (unsigned long long)(unsigned char) FLIPPED_3_H[outflank_h] << 40;

	outflank_d = OUTFLANK_3[(((unsigned int) (O >> 16) & 0x08142240) * 0x01010101) >> 25];
	outflank_d &= ((P & 0x0000081422418000) * 0x0101010101010101) >> 53;	// hgfedc[bahgf]...
	flipped |= FLIPPED_3_H[outflank_d] & 0x0000081422400000;	// A3D6H2

	flipped |= (((P >> 9) & 0x0010000000000000) | ((P >> 7) & 0x0004000000000000)) & O;

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
	unsigned int outflank_h, outflank_v, outflank_d;
	unsigned long long flipped;

	outflank_v = OUTFLANK_5[((O & 0x0010101010101000) * 0x0010204081020408) >> 57];
	outflank_v &= rotl8(((P & 0x1010101010101010) * 0x0010204081020408) >> 56, 1);	// (e8)
	flipped = FLIPPED_5_V[outflank_v] & 0x0010101010101000;

	outflank_h = OUTFLANK_4[(O >> 41) & 0x3f] & rotl8(P >> 40, 2);
	flipped |= (unsigned long long)(unsigned char) FLIPPED_4_H[outflank_h] << 40;

	outflank_d = OUTFLANK_4[(((unsigned int) (O >> 16) & 0x10284402) * 0x01010101) >> 25];
	outflank_d &= ((P & 0x0000102844820100) * 0x0101010101010101) >> 54;	// hgfed[cbahg]...
	flipped |= FLIPPED_4_H[outflank_d] & 0x0000102844020000;	// A2E6H3

	flipped |= (((P >> 9) & 0x0020000000000000) | ((P >> 7) & 0x0008000000000000)) & O;

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
	unsigned int outflank_h, outflank_v, outflank_d9;
	unsigned long long flipped;

	outflank_v = OUTFLANK_5[((O & 0x0020202020202000) * 0x0008102040810204) >> 57];
	outflank_v &= rotl8(((P & 0x2020202020202020) * 0x0008102040810204) >> 56, 1);	// (f8)
	flipped = FLIPPED_5_V[outflank_v] & 0x0020202020202000;

	outflank_h = OUTFLANK_5[(O >> 41) & 0x3f] & rotl8(P >> 40, 1);
	flipped |= (unsigned long long)(unsigned char) FLIPPED_5_H[outflank_h] << 40;

	flipped |= ((P >> 7) | (P << 7)) & 0x0010004000000000 & O;

	outflank_d9 = OUTFLANK_5[(((HIDWORD(O) & 0x00402010) + (LODWORD(O) & 0x08040200)) * 0x01010101) >> 25];
	outflank_d9 &= rotl8((((HIDWORD(P) & 0x80402010) + (LODWORD(P) & 0x08040201)) * 0x01010101) >> 24, 1);	// (h8)
	flipped |= FLIPPED_5_H[outflank_d9] & 0x0040201008040200;

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
	unsigned int outflank_h, outflank_b1g6g8, outflank_e8g6g1;
	unsigned long long flipped;

	outflank_b1g6g8 = OUTFLANK_5[((O & 0x0040402010080400) * 0x0080808080808102) >> 57];
	outflank_b1g6g8 &= rotl8(((P & 0x4040402010080402) * 0x0080808080808102) >> 56, 1);	// (g8)
	flipped = FLIPPED_5_V[outflank_b1g6g8] & 0x0040402010080400;

	outflank_e8g6g1 = OUTFLANK_2[((O & 0x0020404040404000) * 0x0001008040201010) >> 57];
	outflank_e8g6g1 &= ((P & 0x1020404040404040) * 0x0100804020000008) >> 59;	// 81234
	flipped |= vertical_mirror(FLIPPED_2_V[outflank_e8g6g1]) & 0x0020404040404000;

	outflank_h = outflank_right_H((unsigned int) (O >> 41) << 27) & (unsigned int) (P >> 14);
	flipped |= (unsigned long long) (outflank_h * (unsigned int) -2) << 14;

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
	unsigned int outflank_h, outflank_c1h6h8, outflank_f8h6h1;
	unsigned long long flipped;

	outflank_c1h6h8 = OUTFLANK_5[((O & 0x0080804020100800) * 0x0040404040404081) >> 57];
	outflank_c1h6h8 &= rotl8(((P & 0x8080804020100804) * 0x0040404040404081) >> 56, 1);	// (h8)
	flipped = FLIPPED_5_V[outflank_c1h6h8] & 0x0080804020100800;

	outflank_f8h6h1 = OUTFLANK_2[((O & 0x0040808080808000) * 0x0000804020100808) >> 57];
	outflank_f8h6h1 &= ((P & 0x2040808080808080) * 0x0080402010000004) >> 59;	// 81234
	flipped |= vertical_mirror(FLIPPED_2_V[outflank_f8h6h1]) & 0x0040808080808000;

	outflank_h = outflank_right_H((unsigned int) (O >> 41) << 26) & (unsigned int) (P >> 15);
	flipped |= (unsigned long long) (outflank_h * (unsigned int) -2) << 15;

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
	unsigned int outflank_h;
	unsigned long long flipped, outflank_v, outflank_d7;

	outflank_v = outflank_right(O, 0x0000010101010101) & P;
	flipped  = (outflank_v * -2) & 0x0000010101010101;

	outflank_d7 = outflank_right(O, 0x0000020408102040) & P;
	flipped |= (outflank_d7 * -2) & 0x0000020408102040;

	outflank_h = ((unsigned int) (O >> 24) + 0x02000000) & (unsigned int) (P >> 24);
	flipped |= (((unsigned long long) outflank_h << 24) - outflank_h) & 0x007e000000000000;

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
	unsigned int outflank_h;
	unsigned long long flipped, outflank_v, outflank_d7;

	outflank_v = outflank_right(O, 0x0000020202020202) & P;
	flipped  = (outflank_v * -2) & 0x0000020202020202;

	outflank_d7 = outflank_right(O, 0x0000040810204080) & P;
	flipped |= (outflank_d7 * -2) & 0x0000040810204080;

	outflank_h = ((unsigned int) (O >> 24) + 0x04000000) & (unsigned int) (P >> 24);
	flipped |= (((unsigned long long) outflank_h << 24) - outflank_h) & 0x007c000000000000;

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
	unsigned int outflank_h, outflank_d;
	unsigned long long flipped, outflank_v;

	outflank_v = outflank_right(O, 0x0000040404040404) & P;
	flipped  = (outflank_v * -2) & 0x0000040404040404;

	outflank_d = OUTFLANK_2[(((HIDWORD(O) & 0x00040a10) + (LODWORD(O) & 0x20400000)) * 0x01010101) >> 25];
	outflank_d &= ((P & 0x00040a1120408000) * 0x0101010101010101) >> 52;	// hgfedcb[ahgfe]...
	flipped |= FLIPPED_2_H[outflank_d] & 0x00040a1020400000;	// A5C7H2

	outflank_h = OUTFLANK_2[(O >> 49) & 0x3f] & rotl8(P >> 48, 4);
	flipped |= (unsigned long long)(unsigned char) FLIPPED_2_H[outflank_h] << 48;

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

	outflank_v = outflank_right(O, 0x0000080808080808) & P;
	flipped  = (outflank_v * -2) & 0x0000080808080808;

	outflank_d = OUTFLANK_3[(((unsigned int) (O >> 24) & 0x08142240) * 0x01010101) >> 25];
	outflank_d &= ((P & 0x0008142241800000) * 0x0101010101010101) >> 53;	// hgfedc[bahgf]...
	flipped |= FLIPPED_3_H[outflank_d] & 0x0008142240000000;	// A4D7H3

	outflank_h = OUTFLANK_3[(O >> 49) & 0x3f] & rotl8(P >> 48, 3);
	flipped |= (unsigned long long)(unsigned char) FLIPPED_3_H[outflank_h] << 48;

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

	outflank_v = outflank_right(O, 0x0000101010101010) & P;
	flipped  = (outflank_v * -2) & 0x0000101010101010;

	outflank_d = OUTFLANK_4[(((unsigned int) (O >> 24) & 0x10284402) * 0x01010101) >> 25];
	outflank_d &= ((P & 0x0010284482010000) * 0x0101010101010101) >> 54;	// hgfed[cbahg]...
	flipped |= FLIPPED_4_H[outflank_d] & 0x0010284402000000;	// A3E7H4

	outflank_h = OUTFLANK_4[(O >> 49) & 0x3f] & rotl8(P >> 48, 2);
	flipped |= (unsigned long long)(unsigned char) FLIPPED_4_H[outflank_h] << 48;

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
	unsigned int outflank_h, outflank_d;
	unsigned long long flipped, outflank_v;

	outflank_v = outflank_right(O, 0x0000202020202020) & P;
	flipped  = (outflank_v * -2) & 0x0000202020202020;

	outflank_d = OUTFLANK_5[(((HIDWORD(O) & 0x00205008) + (LODWORD(O) & 0x04020000)) * 0x01010101) >> 25];
	outflank_d &= ((P & 0x0020508804020100) * 0x0101010101010101) >> 55;	// hgfe[dcbah]...
	flipped |= FLIPPED_5_H[outflank_d] & 0x0020500804020000;	// A2F7H5

	outflank_h = OUTFLANK_5[(O >> 49) & 0x3f] & rotl8(P >> 48, 1);
	flipped |= (unsigned long long)(unsigned char) FLIPPED_5_H[outflank_h] << 48;

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

	outflank_v = outflank_right(O, 0x0000404040404040) & P;
	flipped  = (outflank_v * -2) & 0x0000404040404040;

	outflank_d9 = outflank_right(O, 0x0000201008040201) & P;
	flipped |= (outflank_d9 * -2) & 0x0000201008040201;

	outflank_h = outflank_right_H((unsigned int) (O >> 49) << 27) & (unsigned int) (P >> 22);
	flipped |= (unsigned long long) (outflank_h * (unsigned int) -2) << 22;

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

	outflank_v = outflank_right(O, 0x0000808080808080) & P;
	flipped  = (outflank_v * -2) & 0x0000808080808080;

	outflank_d9 = outflank_right(O, 0x0000402010080402) & P;
	flipped |= (outflank_d9 * -2) & 0x0000402010080402;

	outflank_h = outflank_right_H((unsigned int) (O >> 49) << 26) & (unsigned int) (P >> 23);
	flipped |= (unsigned long long) (outflank_h * (unsigned int) -2) << 23;

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

	outflank_v = outflank_right(O, 0x0001010101010101) & P;
	flipped  = (outflank_v * -2) & 0x0001010101010101;

	outflank_d7 = outflank_right(O, 0x0002040810204080) & P;
	flipped |= (outflank_d7 * -2) & 0x0002040810204080;

	outflank_h = (O + 0x0200000000000000) & P;
	flipped |= (outflank_h - (outflank_h >> 8)) & 0x7e00000000000000;

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

	outflank_v = outflank_right(O, 0x0002020202020202) & P;
	flipped  = (outflank_v * -2) & 0x0002020202020202;

	outflank_d7 = outflank_right(O, 0x0004081020408000) & P;
	flipped |= (outflank_d7 * -2) & 0x0004081020408000;

	outflank_h = (O + 0x0400000000000000) & P;
	flipped |= (outflank_h - (outflank_h >> 8)) & 0x7c00000000000000;

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
	unsigned int outflank_h, outflank_d;
	unsigned long long flipped, outflank_v;

	outflank_v = outflank_right(O, 0x0004040404040404) & P;
	flipped  = (outflank_v * -2) & 0x0004040404040404;

	outflank_d = OUTFLANK_2[(((HIDWORD(O) & 0x040a1020) + (LODWORD(O) & 0x40000000)) * 0x01010101) >> 25];
	outflank_d &= ((P & 0x040a112040800000) * 0x0101010101010101) >> 52;	// hgfedcb[ahgfe]d0ba...
	flipped |= FLIPPED_2_H[outflank_d] & 0x040a102040000000;	// A6C8H3

	outflank_h = OUTFLANK_2[(O >> 57) & 0x3f] & rotl8(P >> 56, 4);
	flipped |= (unsigned long long)(unsigned char) FLIPPED_2_H[outflank_h] << 56;

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

	outflank_v = outflank_right(O, 0x0008080808080808) & P;
	flipped  = (outflank_v * -2) & 0x0008080808080808;

	outflank_d = OUTFLANK_3[((HIDWORD(O) & 0x08142240) * 0x01010101) >> 25];
	outflank_d &= ((P & 0x0814224180000000) * 0x0101010101010101) >> 53;	// hgfedc[bahgf]e0cba...
	flipped |= FLIPPED_3_H[outflank_d] & 0x0814224000000000;	// A5D8H4

	outflank_h = OUTFLANK_3[(O >> 57) & 0x3f] & rotl8(P >> 56, 3);
	flipped |= (unsigned long long)(unsigned char) FLIPPED_3_H[outflank_h] << 56;

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

	outflank_v = outflank_right(O, 0x0010101010101010) & P;
	flipped  = (outflank_v * -2) & 0x0010101010101010;

	outflank_d = OUTFLANK_4[((HIDWORD(O) & 0x10284402) * 0x01010101) >> 25];
	outflank_d &= ((P & 0x1028448201000000) * 0x0101010101010101) >> 54;	// hgfed[cbahg]f0dcba...
	flipped |= FLIPPED_4_H[outflank_d] & 0x1028440200000000;	// A4E8H5

	outflank_h = OUTFLANK_4[(O >> 57) & 0x3f] & rotl8(P >> 56, 2);
	flipped |= (unsigned long long)(unsigned char) FLIPPED_4_H[outflank_h] << 56;

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
	unsigned int outflank_h, outflank_d;
	unsigned long long flipped, outflank_v;

	outflank_v = outflank_right(O, 0x0020202020202020) & P;
	flipped  = (outflank_v * -2) & 0x0020202020202020;

	outflank_d = OUTFLANK_5[(((HIDWORD(O) & 0x20500804) + (LODWORD(O) & 0x02000000)) * 0x01010101) >> 25];
	outflank_d &= ((P & 0x2050880402010000) * 0x0101010101010101) >> 55;	// hgfe[dcbah]g0edcba...
	flipped |= FLIPPED_5_H[outflank_d] & 0x2050080402000000;	// A3F8H6

	outflank_h = OUTFLANK_5[(O >> 57) & 0x3f] & rotl8(P >> 56, 1);
	flipped |= (unsigned long long)(unsigned char) FLIPPED_5_H[outflank_h] << 56;

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

	outflank_v = outflank_right(O, 0x0040404040404040) & P;
	flipped  = (outflank_v * -2) & 0x0040404040404040;

	outflank_d9 = outflank_right(O, 0x0020100804020100) & P;
	flipped |= (outflank_d9 * -2) & 0x0020100804020100;

	outflank_h = outflank_right_H((unsigned int) (O >> 57) << 27) & (unsigned int) (P >> 30);
	flipped |= (unsigned long long) (outflank_h * (unsigned int) -2) << 30;

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

	outflank_v = outflank_right(O, 0x0080808080808080) & P;
	flipped  = (outflank_v * -2) & 0x0080808080808080;

	outflank_d9 = outflank_right(O, 0x0040201008040201) & P;
	flipped |= (outflank_d9 * -2) & 0x0040201008040201;

	outflank_h = outflank_right_H((unsigned int) (O >> 57) << 26) & (unsigned int) (P >> 31);
	flipped |= (unsigned long long) (outflank_h * (unsigned int) -2) << 31;

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
	flip_A1, flip_B1, flip_C1, flip_D1, flip_E1, flip_F1, flip_G1, flip_H1,
	flip_A2, flip_B2, flip_C2, flip_D2, flip_E2, flip_F2, flip_G2, flip_H2,
	flip_A3, flip_B3, flip_C3, flip_D3, flip_E3, flip_F3, flip_G3, flip_H3,
	flip_A4, flip_B4, flip_C4, flip_D4, flip_E4, flip_F4, flip_G4, flip_H4,
	flip_A5, flip_B5, flip_C5, flip_D5, flip_E5, flip_F5, flip_G5, flip_H5,
	flip_A6, flip_B6, flip_C6, flip_D6, flip_E6, flip_F6, flip_G6, flip_H6,
	flip_A7, flip_B7, flip_C7, flip_D7, flip_E7, flip_F7, flip_G7, flip_H7,
	flip_A8, flip_B8, flip_C8, flip_D8, flip_E8, flip_F8, flip_G8, flip_H8,
	flip_pass, flip_pass
};

