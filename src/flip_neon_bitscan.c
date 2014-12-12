/**
 * @file flip_neon_bitscan.c
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
 * (with Neon if appropriate) can be used to determine contiguous opponent discs.
 * If the OUTFLANK search is in MSB to LSB direction, lzcnt64 is used.
 *
 * @date 1998 - 2022
 * @author Richard Delorme
 * @author Toshihiko Okuhara
 * @version 4.5
 */

#include "bit_intrinsics.h"

// included from board.c or linked in Android Arm32 dispatch build
#if defined(flip_neon) || defined(DISPATCH_NEON)

/** rotated outflank array (indexed with inner 6 bits) */
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

/** flip array (indexed with rotated outflank) */
static const unsigned long long FLIPPED_3_H[21] = {	// ...bahgf
	0x0000000000000000, 0x1010101010101010, 0x3030303030303030, 0x0000000000000000,
	0x7070707070707070, 0x0000000000000000, 0x0000000000000000, 0x0000000000000000,
	0x0606060606060606, 0x1616161616161616, 0x3636363636363636, 0x0000000000000000,
	0x7676767676767676, 0x0000000000000000, 0x0000000000000000, 0x0000000000000000,
	0x0404040404040404, 0x1414141414141414, 0x3434343434343434, 0x0000000000000000,
	0x7474747474747474
};

static const unsigned long long FLIPPED_4_H[19] = {	// ...cbahg
	0x0000000000000000, 0x2020202020202020, 0x6060606060606060, 0x0000000000000000,
	0x0e0e0e0e0e0e0e0e, 0x2e2e2e2e2e2e2e2e, 0x6e6e6e6e6e6e6e6e, 0x0000000000000000,
	0x0c0c0c0c0c0c0c0c, 0x2c2c2c2c2c2c2c2c, 0x6c6c6c6c6c6c6c6c, 0x0000000000000000,
	0x0000000000000000, 0x0000000000000000, 0x0000000000000000, 0x0000000000000000,
	0x0808080808080808, 0x2828282828282828, 0x6868686868686868
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
#else
	#define OutflankToFlipmask(outflank)	((outflank) - (unsigned int) ((outflank) != 0))
#endif

// Strictly, (long long) >> 64 is undefined in C, but either 0 bit (no change)
// or 64 bit (zero out) shift will lead valid result (i.e. flipped == 0).
#define	outflank_right(O,maskr)	(0x8000000000000000ULL >> lzcnt_u64(~(O) & (maskr)))

// in case continuous from MSB
#define	outflank_right_H(O)	(0x80000000u >> lzcnt_u32(~(O)))


#ifdef __clang__	// poor optimization for vbicq(const,x) (ndk-r15)
#define not_O_in_mask(mask,O)	vandq_u64((mask), vdupq_n_u64(~(O)))
#else
#define not_O_in_mask(mask,O)	vbicq_u64((mask), vdupq_n_u64(O))
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
	uint64x2_t outflank_vd, flipped_vd;
	const uint64x2_t mask = { 0x0101010101010100, 0x8040201008040200 };
	const uint64x2_t one = vdupq_n_u64(1);
	unsigned int outflank_h, flipped_h;

	outflank_vd = not_O_in_mask(mask, O);
	outflank_vd = vandq_u64(vbicq_u64(outflank_vd, vsubq_u64(outflank_vd, one)), vdupq_n_u64(P));
	flipped_vd = vandq_u64(mask, vqsubq_u64(outflank_vd, one));

	outflank_h = (unsigned char) (O + 0x02) & P;
	flipped_h = outflank_h - ((unsigned int) (outflank_h != 0) << 1);

	return vget_lane_u64(vorr_u64(vget_low_u64(flipped_vd), vget_high_u64(flipped_vd)), 0) | flipped_h;
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
	uint64x2_t outflank_vd, flipped_vd;
	const uint64x2_t mask = { 0x0202020202020200, 0x0080402010080400 };
	const uint64x2_t one = vdupq_n_u64(1);
	unsigned int outflank_h, flipped_h;

	outflank_vd = not_O_in_mask(mask, O);
	outflank_vd = vandq_u64(vbicq_u64(outflank_vd, vsubq_u64(outflank_vd, one)), vdupq_n_u64(P));
	flipped_vd = vandq_u64(mask, vqsubq_u64(outflank_vd, one));

	outflank_h = (unsigned char) (O + 0x04) & P;
	flipped_h = outflank_h - ((unsigned int) (outflank_h != 0) << 2);

	return vget_lane_u64(vorr_u64(vget_low_u64(flipped_vd), vget_high_u64(flipped_vd)), 0) | flipped_h;
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
	uint64x2_t outflank_vd, flipped_vd;
	const uint64x2_t mask = { 0x0404040404040400, 0x0000804020100800 };
	const uint64x2_t one = vdupq_n_u64(1);
	unsigned int outflank_h, flipped_h;

	outflank_vd = not_O_in_mask(mask, O);
	outflank_vd = vandq_u64(vbicq_u64(outflank_vd, vsubq_u64(outflank_vd, one)), vdupq_n_u64(P));
	flipped_vd = vandq_u64(mask, vqsubq_u64(outflank_vd, one));

	outflank_h = (unsigned char) (O + 0x08) & P;
	flipped_h = outflank_h - ((unsigned int) (outflank_h != 0) << 3);

	flipped_h |= ((((unsigned int) P << 1) & 0x00000002) | (((unsigned int) P >> 7) & 0x00000200)) & O;

	return vget_lane_u64(vorr_u64(vget_low_u64(flipped_vd), vget_high_u64(flipped_vd)), 0) | flipped_h;
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

	outflank_v = ~O & 0x0808080808080800;
	outflank_v = (outflank_v & -outflank_v) & 0x0808080808080800 & P;
	flipped = OutflankToFlipmask(outflank_v) & 0x0808080808080800;

	outflank_d = OUTFLANK_3[(((unsigned int) O & 0x40221408) * 0x01010101) >> 25];
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

	outflank_v = ~O & 0x1010101010101000;
	outflank_v = (outflank_v & -outflank_v) & 0x1010101010101000 & P;
	flipped = OutflankToFlipmask(outflank_v) & 0x1010101010101000;

	outflank_d = OUTFLANK_4[(((unsigned int) O & 0x02442810) * 0x01010101) >> 25];
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
	uint64x2_t outflank_vd, flipped;
	const uint64x2_t mask = { 0x2020202020202000, 0x0000010204081000 };
	const uint64x2_t one = vdupq_n_u64(1);
	unsigned int outflank_h, flipped_h;

	outflank_vd = not_O_in_mask(mask, O);
	outflank_vd = vandq_u64(vbicq_u64(outflank_vd, vsubq_u64(outflank_vd, one)), vdupq_n_u64(P));
	flipped = vandq_u64(vqsubq_u64(outflank_vd, one), mask);

	outflank_h = outflank_right_H((unsigned int) O << 27) & ((unsigned int) P << 27);
	flipped_h = (outflank_h * (unsigned int) -2) >> 27;

	flipped_h |= ((((unsigned int) P >> 1) & 0x00000040) | (((unsigned int) P >> 9) & 0x00004000)) & O;

	return vget_lane_u64(vorr_u64(vget_low_u64(flipped), vget_high_u64(flipped)), 0) | flipped_h;
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
	uint64x2_t outflank_vd, flipped;
	const uint64x2_t mask = { 0x4040404040404000, 0x0001020408102000 };
	const uint64x2_t one = vdupq_n_u64(1);
	unsigned int outflank_h, flipped_h;

	outflank_vd = not_O_in_mask(mask, O);
	outflank_vd = vandq_u64(vbicq_u64(outflank_vd, vsubq_u64(outflank_vd, one)), vdupq_n_u64(P));
	flipped = vandq_u64(vqsubq_u64(outflank_vd, one), mask);

	outflank_h = outflank_right_H((unsigned int) O << 26) & ((unsigned int) P << 26);
	flipped_h = (outflank_h * (unsigned int) -2) >> 26;

	return vget_lane_u64(vorr_u64(vget_low_u64(flipped), vget_high_u64(flipped)), 0) | flipped_h;
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
	uint64x2_t outflank_vd, flipped;
	const uint64x2_t mask = { 0x8080808080808000, 0x0102040810204000 };
	const uint64x2_t one = vdupq_n_u64(1);
	unsigned int outflank_h, flipped_h;

	outflank_vd = not_O_in_mask(mask, O);
	outflank_vd = vandq_u64(vbicq_u64(outflank_vd, vsubq_u64(outflank_vd, one)), vdupq_n_u64(P));
	flipped = vandq_u64(vqsubq_u64(outflank_vd, one), mask);

	outflank_h = outflank_right_H((unsigned int) O << 25) & ((unsigned int) P << 25);
	flipped_h = (outflank_h * (unsigned int) -2) >> 25;

	return vget_lane_u64(vorr_u64(vget_low_u64(flipped), vget_high_u64(flipped)), 0) | flipped_h;
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
	uint64x2_t outflank_vd, flipped_vd;
	const uint64x2_t mask = { 0x0101010101010000, 0x4020100804020000 };
	const uint64x2_t one = vdupq_n_u64(1);
	unsigned int outflank_h, flipped_h;

	outflank_vd = not_O_in_mask(mask, O);
	outflank_vd = vandq_u64(vbicq_u64(outflank_vd, vsubq_u64(outflank_vd, one)), vdupq_n_u64(P));
	flipped_vd = vandq_u64(mask, vqsubq_u64(outflank_vd, one));

	outflank_h = (unsigned short) (O + 0x0200) & P;
	flipped_h = (outflank_h - (outflank_h >> 8)) & 0x7e00;

	return vget_lane_u64(vorr_u64(vget_low_u64(flipped_vd), vget_high_u64(flipped_vd)), 0) | flipped_h;
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
	uint64x2_t outflank_vd, flipped_vd;
	const uint64x2_t mask = { 0x0202020202020000, 0x8040201008040000 };
	const uint64x2_t one = vdupq_n_u64(1);
	unsigned int outflank_h, flipped_h;

	outflank_vd = not_O_in_mask(mask, O);
	outflank_vd = vandq_u64(vbicq_u64(outflank_vd, vsubq_u64(outflank_vd, one)), vdupq_n_u64(P));
	flipped_vd = vandq_u64(mask, vqsubq_u64(outflank_vd, one));

	outflank_h = (unsigned short) (O + 0x0400) & P;
	flipped_h = (outflank_h - (outflank_h >> 8)) & 0x7c00;

	return vget_lane_u64(vorr_u64(vget_low_u64(flipped_vd), vget_high_u64(flipped_vd)), 0) | flipped_h;
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
	uint64x2_t outflank_vd, flipped_vd;
	const uint64x2_t mask = { 0x0404040404040000, 0x0080402010080000 };
	const uint64x2_t one = vdupq_n_u64(1);
	unsigned int outflank_h, flipped_h;

	outflank_vd = not_O_in_mask(mask, O);
	outflank_vd = vandq_u64(vbicq_u64(outflank_vd, vsubq_u64(outflank_vd, one)), vdupq_n_u64(P));
	flipped_vd = vandq_u64(mask, vqsubq_u64(outflank_vd, one));

	outflank_h = (unsigned short) (O + 0x0800) & P;
	flipped_h = (outflank_h - (outflank_h >> 8)) & 0x7800;

	flipped_h |= ((((unsigned int) P << 1) & 0x00000200) | (((unsigned int) P >> 7) & 0x00020000)) & O;

	return vget_lane_u64(vorr_u64(vget_low_u64(flipped_vd), vget_high_u64(flipped_vd)), 0) | flipped_h;
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

	outflank_v = ~O & 0x0808080808080000;
	outflank_v = (outflank_v & -outflank_v) & 0x0808080808080000 & P;
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

	outflank_v = ~O & 0x1010101010100000;
	outflank_v = (outflank_v & -outflank_v) & 0x1010101010100000 & P;
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
	uint64x2_t outflank_vd, flipped;
	const uint64x2_t mask = { 0x2020202020200000, 0x0001020408100000 };
	const uint64x2_t one = vdupq_n_u64(1);
	unsigned int outflank_h, flipped_h;

	outflank_vd = not_O_in_mask(mask, O);
	outflank_vd = vandq_u64(vbicq_u64(outflank_vd, vsubq_u64(outflank_vd, one)), vdupq_n_u64(P));
	flipped = vandq_u64(vqsubq_u64(outflank_vd, one), mask);

	outflank_h = outflank_right_H(((unsigned int) O >> 9) << 28) & ((unsigned int) P << 19);
	flipped_h = (outflank_h * (unsigned int) -2) >> 19;

	flipped_h |= ((((unsigned int) P >> 1) & 0x00004000) | (((unsigned int) P >> 9) & 0x00400000)) & O;

	return vget_lane_u64(vorr_u64(vget_low_u64(flipped), vget_high_u64(flipped)), 0) | flipped_h;
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
	uint64x2_t outflank_vd, flipped;
	const uint64x2_t mask = { 0x4040404040400000, 0x0102040810200000 };
	const uint64x2_t one = vdupq_n_u64(1);
	unsigned int outflank_h, flipped_h;

	outflank_vd = not_O_in_mask(mask, O);
	outflank_vd = vandq_u64(vbicq_u64(outflank_vd, vsubq_u64(outflank_vd, one)), vdupq_n_u64(P));
	flipped = vandq_u64(vqsubq_u64(outflank_vd, one), mask);

	outflank_h = outflank_right_H(((unsigned int) O >> 9) << 27) & ((unsigned int) P << 18);
	flipped_h = (outflank_h * (unsigned int) -2) >> 18;

	return vget_lane_u64(vorr_u64(vget_low_u64(flipped), vget_high_u64(flipped)), 0) | flipped_h;
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
	uint64x2_t outflank_vd, flipped;
	const uint64x2_t mask = { 0x8080808080800000, 0x0204081020400000 };
	const uint64x2_t one = vdupq_n_u64(1);
	unsigned int outflank_h, flipped_h;

	outflank_vd = not_O_in_mask(mask, O);
	outflank_vd = vandq_u64(vbicq_u64(outflank_vd, vsubq_u64(outflank_vd, one)), vdupq_n_u64(P));
	flipped = vandq_u64(vqsubq_u64(outflank_vd, one), mask);

	outflank_h = outflank_right_H(((unsigned int) O >> 9) << 26) & ((unsigned int) P << 17);
	flipped_h = (outflank_h * (unsigned int) -2) >> 17;

	return vget_lane_u64(vorr_u64(vget_low_u64(flipped), vget_high_u64(flipped)), 0) | flipped_h;
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
	uint64x2_t outflank_vd, flipped_vd;
	const uint64x2_t mask = { 0x0101010101000000, 0x2010080402000000 };
	const uint64x2_t one = vdupq_n_u64(1);
	unsigned int outflank_h, flipped_h;

	outflank_vd = not_O_in_mask(mask, O);
	outflank_vd = vandq_u64(vbicq_u64(outflank_vd, vsubq_u64(outflank_vd, one)), vdupq_n_u64(P));
	flipped_vd = vandq_u64(mask, vqsubq_u64(outflank_vd, one));

	outflank_h = ((O & 0x007e0000) + 0x00020000) & P;
	flipped_h = (outflank_h - (outflank_h >> 8)) & 0x007e0000;

	flipped_h |= ((((unsigned int) P << 8) & 0x00000100) | (((unsigned int) P << 7) & 0x00000200)) & O;

	return vget_lane_u64(vorr_u64(vget_low_u64(flipped_vd), vget_high_u64(flipped_vd)), 0) | flipped_h;
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
	uint64x2_t outflank_vd, flipped_vd;
	const uint64x2_t mask = { 0x0202020202000000, 0x4020100804000000 };
	const uint64x2_t one = vdupq_n_u64(1);
	unsigned int outflank_h, flipped_h;

	outflank_vd = not_O_in_mask(mask, O);
	outflank_vd = vandq_u64(vbicq_u64(outflank_vd, vsubq_u64(outflank_vd, one)), vdupq_n_u64(P));
	flipped_vd = vandq_u64(mask, vqsubq_u64(outflank_vd, one));

	outflank_h = ((O & 0x007c0000) + 0x00040000) & P;
	flipped_h = (outflank_h - (outflank_h >> 8)) & 0x007c0000;

	flipped_h |= ((((unsigned int) P << 8) & 0x00000200) | (((unsigned int) P << 7) & 0x00000400)) & O;

	return vget_lane_u64(vorr_u64(vget_low_u64(flipped_vd), vget_high_u64(flipped_vd)), 0) | flipped_h;
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
	uint64x2_t PP = vdupq_n_u64(P);
	uint64x2_t OO = vdupq_n_u64(O);
	uint64x2_t outflank_vd, flipped;
	const uint64x2_t mask = { 0x0404040404000000, 0x8040201008000000 };
	const uint64x2_t one = vdupq_n_u64(1);
	unsigned int outflank_h, flipped_h;
	// uint32x4_t PL = vtrnq_u32(vreinterpretq_u32_u64(PP), vreinterpretq_u32_u64(PP)).val[0];
	// const int32x4_t shiftL = { 1, 7, 8, 9 };
	// const uint32x4_t maskL = { 0x00020000, 0x00000800, 0x00000400, 0x00000200 };

	outflank_vd = vbicq_u64(mask, OO);
	outflank_vd = vandq_u64(vbicq_u64(outflank_vd, vsubq_u64(outflank_vd, one)), PP);
	flipped = vandq_u64(mask, vqsubq_u64(outflank_vd, one));

	outflank_h = ((O & 0x00780000) + 0x00080000) & P;
	flipped_h = (outflank_h - (outflank_h >> 8)) & 0x00780000;
#if 1
	flipped_h |= (((P << 9) & 0x00000200) | ((P << 8) & 0x00000400) | ((P << 1) & 0x00020000)
		| (((P >> 7) | (P << 7)) & 0x02000800)) & O;
#else
	flipped = vbslq_u64(vpaddlq_u32(vandq_u32(vshlq_u32(PL, shiftL), maskL)), OO, flipped);
	flipped_h |= (P >> 7) & 0x02000000 & O;
#endif
	return vget_lane_u64(vorr_u64(vget_low_u64(flipped), vget_high_u64(flipped)), 0) | flipped_h;
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
	unsigned int outflank_h, outflank_d;
	unsigned long long flipped, outflank_v;

	outflank_v = ~O & 0x0808080808000000;
	outflank_v = (outflank_v & -outflank_v) & 0x0808080808000000 & P;
	flipped = OutflankToFlipmask(outflank_v) & 0x0808080808000000;

	outflank_h = OUTFLANK_3[(O >> 17) & 0x3f] & rotl8(P >> 16, 3);
	flipped |= (unsigned char) FLIPPED_3_H[outflank_h] << 16;

	outflank_d = OUTFLANK_3[(((unsigned int) (O >> 16) & 0x40221408) * 0x01010101) >> 25];
	outflank_d &= ((P & 0x0080412214080000) * 0x0101010101010101) >> 53;	// hgfedc[bahgf]...
	flipped |= FLIPPED_3_H[outflank_d] & 0x0000402214080000;	// A6D3H7

	flipped |= (((P << 7) & 0x0000000000001000) | ((P << 8) & 0x000000000000800) | ((P << 9) & 0x000000000000400)) & O;

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
	unsigned int outflank_h, outflank_d;
	unsigned long long flipped, outflank_v;

	outflank_v = ~O & 0x1010101010000000;
	outflank_v = (outflank_v & -outflank_v) & 0x1010101010000000 & P;
	flipped = OutflankToFlipmask(outflank_v) & 0x1010101010000000;

	outflank_h = OUTFLANK_4[(O >> 17) & 0x3f] & rotl8(P >> 16, 2);
	flipped |= (unsigned char) FLIPPED_4_H[outflank_h] << 16;

	outflank_d = OUTFLANK_4[(((unsigned int) (O >> 16) & 0x02442810) * 0x01010101) >> 25];
	outflank_d &= ((P & 0x0001824428100000) * 0x0101010101010101) >> 54;	// hgfed[cbahg]...
	flipped |= FLIPPED_4_H[outflank_d] & 0x0000024428100000;	// A7E3H6

	flipped |= (((P << 7) & 0x0000000000002000) | ((P << 8) & 0x000000000001000) | ((P << 9) & 0x000000000000800)) & O;

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
	uint64x2_t PP = vdupq_n_u64(P);
	uint64x2_t OO = vdupq_n_u64(O);
	uint32x4_t PL = vtrnq_u32(vreinterpretq_u32_u64(PP), vreinterpretq_u32_u64(PP)).val[0];
	uint32x4_t OL = vtrnq_u32(vreinterpretq_u32_u64(OO), vreinterpretq_u32_u64(OO)).val[0];
	uint32x4_t outflankL;
	uint64x2_t outflankH, flipped;
	const uint64x2_t maskL = { 0x001f000000002020, 0x0000408000001008 };
	const uint64x2_t maskH = { 0x2020202020000000, 0x0102040810000000 };
	const uint32x4_t msb = vdupq_n_u32(0x80000000);
	const uint64x2_t one = vdupq_n_u64(1);
	unsigned long long flipped_g3g4;

	outflankL = vandq_u32(vshlq_u32(msb, vnegq_s32(vreinterpretq_s32_u32(
		vclzq_u32(vbicq_u32(vreinterpretq_u32_u64(maskL), OL))))), PL);
	flipped = vpaddlq_u32(vandq_u32(vreinterpretq_u32_u64(maskL), vreinterpretq_u32_s32(
		vnegq_s32(vreinterpretq_s32_u32(vaddq_u32(outflankL, outflankL))))));

	outflankH = vbicq_u64(maskH, OO);
	outflankH = vandq_u64(vbicq_u64(outflankH, vsubq_u64(outflankH, one)), PP);
	flipped = vbslq_u64(maskH, vqsubq_u64(outflankH, one), flipped);

	flipped_g3g4 = (((P >> 9) & 0x0000000040000000) | ((P >> 1) & 0x000000000400000)) & O;

	return vget_lane_u64(vorr_u64(vget_low_u64(flipped), vget_high_u64(flipped)), 0) | flipped_g3g4;
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
	uint64x2_t PP = vdupq_n_u64(P);
	uint64x2_t OO = vdupq_n_u64(O);
	uint32x4_t PL = vtrnq_u32(vreinterpretq_u32_u64(PP), vreinterpretq_u32_u64(PP)).val[0];
	uint32x4_t OL = vtrnq_u32(vreinterpretq_u32_u64(OO), vreinterpretq_u32_u64(OO)).val[0];
	uint32x4_t outflankL;
	uint64x2_t outflankH, flipped;
	const uint64x2_t maskL = { 0x003f000000004040, 0x0000000000002010 };
	const uint64x2_t maskH = { 0x4040404040000000, 0x0204081020000000 };
	const uint32x4_t msb = vdupq_n_u32(0x80000000);
	const uint64x2_t one = vdupq_n_u64(1);

	outflankL = vandq_u32(vshlq_u32(msb, vnegq_s32(vreinterpretq_s32_u32(
		vclzq_u32(vbicq_u32(vreinterpretq_u32_u64(maskL), OL))))), PL);
	flipped = vpaddlq_u32(vandq_u32(vreinterpretq_u32_u64(maskL), vreinterpretq_u32_s32(
		vnegq_s32(vreinterpretq_s32_u32(vaddq_u32(outflankL, outflankL))))));

	outflankH = vbicq_u64(maskH, OO);
	outflankH = vandq_u64(vbicq_u64(outflankH, vsubq_u64(outflankH, one)), PP);
	flipped = vbslq_u64(maskH, vqsubq_u64(outflankH, one), flipped);

	return vget_lane_u64(vorr_u64(vget_low_u64(flipped), vget_high_u64(flipped)), 0);
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
	uint64x2_t PP = vdupq_n_u64(P);
	uint64x2_t OO = vdupq_n_u64(O);
	uint32x4_t PL = vtrnq_u32(vreinterpretq_u32_u64(PP), vreinterpretq_u32_u64(PP)).val[0];
	uint32x4_t OL = vtrnq_u32(vreinterpretq_u32_u64(OO), vreinterpretq_u32_u64(OO)).val[0];
	uint32x4_t outflankL;
	uint64x2_t outflankH, flipped;
	const uint64x2_t maskL = { 0x007f000000008080, 0x0000000000004020 };
	const uint64x2_t maskH = { 0x8080808080000000, 0x0408102040000000 };
	const uint32x4_t msb = vdupq_n_u32(0x80000000);
	const uint64x2_t one = vdupq_n_u64(1);

	outflankL = vandq_u32(vshlq_u32(msb, vnegq_s32(vreinterpretq_s32_u32(
		vclzq_u32(vbicq_u32(vreinterpretq_u32_u64(maskL), OL))))), PL);
	flipped = vpaddlq_u32(vandq_u32(vreinterpretq_u32_u64(maskL), vreinterpretq_u32_s32(
		vnegq_s32(vreinterpretq_s32_u32(vaddq_u32(outflankL, outflankL))))));

	outflankH = vbicq_u64(maskH, OO);
	outflankH = vandq_u64(vbicq_u64(outflankH, vsubq_u64(outflankH, one)), PP);
	flipped = vbslq_u64(maskH, vqsubq_u64(outflankH, one), flipped);

	return vget_lane_u64(vorr_u64(vget_low_u64(flipped), vget_high_u64(flipped)), 0);
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
	uint32x4_t PP = vreinterpretq_u32_u64(vdupq_n_u64(P));
	uint32x4_t OO = vreinterpretq_u32_u64(vdupq_n_u64(O));
	uint64x2_t flipped;
	uint32x4_t outflankL, outflankH;
	const uint64x2_t maskL = { 0x0000000000010101, 0x0000000000020408 };
	const uint64x2_t maskH = { 0x01010101fe000000, 0x1008040200000000 };
	const uint32x4_t msb = vdupq_n_u32(0x80000000);
	const uint32x4_t one = vdupq_n_u32(1);

	outflankL = vshlq_u32(msb, vnegq_s32(vreinterpretq_s32_u32(
		vclzq_u32(vbicq_u32(vreinterpretq_u32_u64(maskL), OO)))));
	outflankL = vandq_u32(outflankL, PP);
	flipped = vandq_u64(maskL, vreinterpretq_u64_s32(
		vnegq_s32(vreinterpretq_s32_u32(vaddq_u32(outflankL, outflankL)))));

	outflankH = vbicq_u32(vreinterpretq_u32_u64(maskH), OO);
	outflankH = vandq_u32(vbicq_u32(outflankH, vsubq_u32(outflankH, one)), PP);
	flipped = vbslq_u64(maskH, vreinterpretq_u64_u32(vqsubq_u32(outflankH, one)), flipped);

	return vget_lane_u64(vorr_u64(vget_low_u64(flipped), vget_high_u64(flipped)), 0);
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
	uint32x4_t PP = vreinterpretq_u32_u64(vdupq_n_u64(P));
	uint32x4_t OO = vreinterpretq_u32_u64(vdupq_n_u64(O));
	uint64x2_t flipped;
	uint32x4_t outflankL, outflankH;
	const uint64x2_t maskL = { 0x0000000000020202, 0x0000000000040810 };
	const uint64x2_t maskH = { 0x02020202fc000000, 0x2010080400000000 };
	const uint32x4_t msb = vdupq_n_u32(0x80000000);
	const uint32x4_t one = vdupq_n_u32(1);

	outflankL = vshlq_u32(msb, vnegq_s32(vreinterpretq_s32_u32(
		vclzq_u32(vbicq_u32(vreinterpretq_u32_u64(maskL), OO)))));
	outflankL = vandq_u32(outflankL, PP);
	flipped = vandq_u64(maskL, vreinterpretq_u64_s32(
		vnegq_s32(vreinterpretq_s32_u32(vaddq_u32(outflankL, outflankL)))));

	outflankH = vbicq_u32(vreinterpretq_u32_u64(maskH), OO);
	outflankH = vandq_u32(vbicq_u32(outflankH, vsubq_u32(outflankH, one)), PP);
	flipped = vbslq_u64(maskH, vreinterpretq_u64_u32(vqsubq_u32(outflankH, one)), flipped);

	return vget_lane_u64(vorr_u64(vget_low_u64(flipped), vget_high_u64(flipped)), 0);
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
	uint32x4_t PP = vreinterpretq_u32_u64(vdupq_n_u64(P));
	uint32x4_t OO = vreinterpretq_u32_u64(vdupq_n_u64(O));
	uint32x4_t PH = vsetq_lane_u32(vgetq_lane_u32(PP, 1), PP, 2);	// HHHL
	uint32x4_t OH = vsetq_lane_u32(vgetq_lane_u32(OO, 1), OO, 2);
	uint32x4_t outflankL, outflankH, flippedL4, flippedH;
	uint32x2x2_t flippedLH;
	uint64x1_t flipped;
	const uint64x2_t maskL = { 0x0004040403000000, 0x0008102000020100 };
	const uint64x2_t maskH = { 0x04040404f8000000, 0x0000010240201008 };
	const uint32x4_t msb = vdupq_n_u32(0x80000000);
	const uint32x4_t one = vdupq_n_u32(1);

	outflankL = vshlq_u32(msb, vnegq_s32(vreinterpretq_s32_u32(
		vclzq_u32(vbicq_u32(vreinterpretq_u32_u64(maskL), vtrnq_u32(OO, OO).val[0])))));
	outflankL = vandq_u32(outflankL, vtrnq_u32(PP, PP).val[0]);
	flippedL4 = vandq_u32(vreinterpretq_u32_u64(maskL), vreinterpretq_u32_s32(
		vnegq_s32(vreinterpretq_s32_u32(vaddq_u32(outflankL, outflankL)))));

	outflankH = vbicq_u32(vreinterpretq_u32_u64(maskH), OH);
	outflankH = vandq_u32(vbicq_u32(outflankH, vsubq_u32(outflankH, one)), PH);
	flippedH = vandq_u32(vreinterpretq_u32_u64(maskH), vqsubq_u32(outflankH, one));

	flippedLH = vtrn_u32(vorr_u32(vget_low_u32(flippedL4), vget_high_u32(flippedL4)), vget_high_u32(flippedH));
	flipped = vreinterpret_u64_u32(vorr_u32(vorr_u32(flippedLH.val[0], flippedLH.val[1]), vget_low_u32(flippedH)));

	return vget_lane_u64(flipped, 0);
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
	uint32x4_t PP = vreinterpretq_u32_u64(vdupq_n_u64(P));
	uint32x4_t OO = vreinterpretq_u32_u64(vdupq_n_u64(O));
	uint32x4_t PH = vsetq_lane_u32(vgetq_lane_u32(PP, 1), PP, 2);	// HHHL
	uint32x4_t OH = vsetq_lane_u32(vgetq_lane_u32(OO, 1), OO, 2);
	uint32x4_t outflankL, outflankH, flippedL4, flippedH;
	uint32x2x2_t flippedLH;
	uint64x1_t flipped;
	const uint64x2_t maskL = { 0x0008080807000000, 0x0010204000040201 };
	const uint64x2_t maskH = { 0x08080808f0000000, 0x0001020480402010 };
	const uint32x4_t msb = vdupq_n_u32(0x80000000);
	const uint32x4_t one = vdupq_n_u32(1);

	outflankL = vshlq_u32(msb, vnegq_s32(vreinterpretq_s32_u32(
		vclzq_u32(vbicq_u32(vreinterpretq_u32_u64(maskL), vtrnq_u32(OO, OO).val[0])))));
	outflankL = vandq_u32(outflankL, vtrnq_u32(PP, PP).val[0]);
	flippedL4 = vandq_u32(vreinterpretq_u32_u64(maskL), vreinterpretq_u32_s32(
		vnegq_s32(vreinterpretq_s32_u32(vaddq_u32(outflankL, outflankL)))));

	outflankH = vbicq_u32(vreinterpretq_u32_u64(maskH), OH);
	outflankH = vandq_u32(vbicq_u32(outflankH, vsubq_u32(outflankH, one)), PH);
	flippedH = vandq_u32(vreinterpretq_u32_u64(maskH), vqsubq_u32(outflankH, one));

	flippedLH = vtrn_u32(vorr_u32(vget_low_u32(flippedL4), vget_high_u32(flippedL4)), vget_high_u32(flippedH));
	flipped = vreinterpret_u64_u32(vorr_u32(vorr_u32(flippedLH.val[0], flippedLH.val[1]), vget_low_u32(flippedH)));

	return vget_lane_u64(flipped, 0);
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
	uint32x4_t PP = vreinterpretq_u32_u64(vdupq_n_u64(P));
	uint32x4_t OO = vreinterpretq_u32_u64(vdupq_n_u64(O));
	uint32x4_t PH = vsetq_lane_u32(vgetq_lane_u32(PP, 1), PP, 2);	// HHHL
	uint32x4_t OH = vsetq_lane_u32(vgetq_lane_u32(OO, 1), OO, 2);
	uint32x4_t outflankL, outflankH, flippedL4, flippedH;
	uint32x2x2_t flippedLH;
	uint64x1_t flipped;
	const uint64x2_t maskL = { 0x001010100f000000, 0x0020408000080402 };
	const uint64x2_t maskH = { 0x10101010e0000000, 0x0102040800804020 };
	const uint32x4_t msb = vdupq_n_u32(0x80000000);
	const uint32x4_t one = vdupq_n_u32(1);

	outflankL = vshlq_u32(msb, vnegq_s32(vreinterpretq_s32_u32(
		vclzq_u32(vbicq_u32(vreinterpretq_u32_u64(maskL), vtrnq_u32(OO, OO).val[0])))));
	outflankL = vandq_u32(outflankL, vtrnq_u32(PP, PP).val[0]);
	flippedL4 = vandq_u32(vreinterpretq_u32_u64(maskL), vreinterpretq_u32_s32(
		vnegq_s32(vreinterpretq_s32_u32(vaddq_u32(outflankL, outflankL)))));

	outflankH = vbicq_u32(vreinterpretq_u32_u64(maskH), OH);
	outflankH = vandq_u32(vbicq_u32(outflankH, vsubq_u32(outflankH, one)), PH);
	flippedH = vandq_u32(vreinterpretq_u32_u64(maskH), vqsubq_u32(outflankH, one));

	flippedLH = vtrn_u32(vorr_u32(vget_low_u32(flippedL4), vget_high_u32(flippedL4)), vget_high_u32(flippedH));
	flipped = vreinterpret_u64_u32(vorr_u32(vorr_u32(flippedLH.val[0], flippedLH.val[1]), vget_low_u32(flippedH)));

	return vget_lane_u64(flipped, 0);
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
	uint32x4_t PP = vreinterpretq_u32_u64(vdupq_n_u64(P));
	uint32x4_t OO = vreinterpretq_u32_u64(vdupq_n_u64(O));
	uint32x4_t PH = vsetq_lane_u32(vgetq_lane_u32(PP, 1), PP, 2);	// HHHL
	uint32x4_t OH = vsetq_lane_u32(vgetq_lane_u32(OO, 1), OO, 2);
	uint32x4_t outflankL, outflankH, flippedL4, flippedH;
	uint32x2x2_t flippedLH;
	uint64x1_t flipped;
	const uint64x2_t maskL = { 0x002020201f000000, 0x0040800000100804 };
	const uint64x2_t maskH = { 0x20202020c0000000, 0x0204081000008040 };
	const uint32x4_t msb = vdupq_n_u32(0x80000000);
	const uint32x4_t one = vdupq_n_u32(1);

	outflankL = vshlq_u32(msb, vnegq_s32(vreinterpretq_s32_u32(
		vclzq_u32(vbicq_u32(vreinterpretq_u32_u64(maskL), vtrnq_u32(OO, OO).val[0])))));
	outflankL = vandq_u32(outflankL, vtrnq_u32(PP, PP).val[0]);
	flippedL4 = vandq_u32(vreinterpretq_u32_u64(maskL), vreinterpretq_u32_s32(
		vnegq_s32(vreinterpretq_s32_u32(vaddq_u32(outflankL, outflankL)))));

	outflankH = vbicq_u32(vreinterpretq_u32_u64(maskH), OH);
	outflankH = vandq_u32(vbicq_u32(outflankH, vsubq_u32(outflankH, one)), PH);
	flippedH = vandq_u32(vreinterpretq_u32_u64(maskH), vqsubq_u32(outflankH, one));

	flippedLH = vtrn_u32(vorr_u32(vget_low_u32(flippedL4), vget_high_u32(flippedL4)), vget_high_u32(flippedH));
	flipped = vreinterpret_u64_u32(vorr_u32(vorr_u32(flippedLH.val[0], flippedLH.val[1]), vget_low_u32(flippedH)));

	return vget_lane_u64(flipped, 0);
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
	uint64x2_t PP = vdupq_n_u64(P);
	uint64x2_t OO = vdupq_n_u64(O);
	uint32x4_t PL = vtrnq_u32(vreinterpretq_u32_u64(PP), vreinterpretq_u32_u64(PP)).val[0];
	uint32x4_t OL = vtrnq_u32(vreinterpretq_u32_u64(OO), vreinterpretq_u32_u64(OO)).val[0];
	uint32x4_t outflankL;
	uint64x2_t outflankH, flipped;
	const uint64x2_t maskL = { 0x3f00000000404040, 0x0000000000201008 };
	const uint64x2_t maskH = { 0x4040404000000000, 0x0408102000000000 };
	const uint32x4_t msb = vdupq_n_u32(0x80000000);
	const uint64x2_t one = vdupq_n_u64(1);

	outflankL = vandq_u32(vshlq_u32(msb, vnegq_s32(vreinterpretq_s32_u32(
		vclzq_u32(vbicq_u32(vreinterpretq_u32_u64(maskL), OL))))), PL);
	flipped = vpaddlq_u32(vandq_u32(vreinterpretq_u32_u64(maskL), vreinterpretq_u32_s32(
		vnegq_s32(vreinterpretq_s32_u32(vaddq_u32(outflankL, outflankL))))));

	outflankH = vbicq_u64(maskH, OO);
	outflankH = vandq_u64(vbicq_u64(outflankH, vsubq_u64(outflankH, one)), PP);
	flipped = vbslq_u64(maskH, vqsubq_u64(outflankH, one), flipped);

	return vget_lane_u64(vorr_u64(vget_low_u64(flipped), vget_high_u64(flipped)), 0);
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
	uint64x2_t PP = vdupq_n_u64(P);
	uint64x2_t OO = vdupq_n_u64(O);
	uint32x4_t PL = vtrnq_u32(vreinterpretq_u32_u64(PP), vreinterpretq_u32_u64(PP)).val[0];
	uint32x4_t OL = vtrnq_u32(vreinterpretq_u32_u64(OO), vreinterpretq_u32_u64(OO)).val[0];
	uint32x4_t outflankL;
	uint64x2_t outflankH, flipped;
	const uint64x2_t maskL = { 0x7f00000000808080, 0x0000000000402010 };
	const uint64x2_t maskH = { 0x8080808000000000, 0x0810204000000000 };
	const uint32x4_t msb = vdupq_n_u32(0x80000000);
	const uint64x2_t one = vdupq_n_u64(1);

	outflankL = vandq_u32(vshlq_u32(msb, vnegq_s32(vreinterpretq_s32_u32(
		vclzq_u32(vbicq_u32(vreinterpretq_u32_u64(maskL), OL))))), PL);
	flipped = vpaddlq_u32(vandq_u32(vreinterpretq_u32_u64(maskL), vreinterpretq_u32_s32(
		vnegq_s32(vreinterpretq_s32_u32(vaddq_u32(outflankL, outflankL))))));

	outflankH = vbicq_u64(maskH, OO);
	outflankH = vandq_u64(vbicq_u64(outflankH, vsubq_u64(outflankH, one)), PP);
	flipped = vbslq_u64(maskH, vqsubq_u64(outflankH, one), flipped);

	return vget_lane_u64(vorr_u64(vget_low_u64(flipped), vget_high_u64(flipped)), 0);
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
	uint32x4_t PP = vreinterpretq_u32_u64(vdupq_n_u64(P));
	uint32x4_t OO = vreinterpretq_u32_u64(vdupq_n_u64(O));
	uint32x4_t PH = vtrnq_u32(PP, PP).val[1];
	uint32x4_t OH = vtrnq_u32(OO, OO).val[1];
	uint32x4_t outflankL, outflankH, flippedL, flippedH;
	uint32x4x2_t flippedLH;
	uint64x2_t flipped;
	const uint64x2_t maskL = { 0x0000000001010101, 0x0000000002040810 };
	const uint64x2_t maskH = { 0x01010100000000fe, 0x0804020000000000 };
	const uint32x4_t msb = vdupq_n_u32(0x80000000);
	const uint32x4_t one = vdupq_n_u32(1);

	outflankL = vandq_u32(vshlq_u32(msb, vnegq_s32(vreinterpretq_s32_u32(
		vclzq_u32(vbicq_u32(vreinterpretq_u32_u64(maskL), OO))))), PP);
	flippedL = vandq_u32(vreinterpretq_u32_u64(maskL), vreinterpretq_u32_s32(
		vnegq_s32(vreinterpretq_s32_u32(vaddq_u32(outflankL, outflankL)))));

	outflankH = vbicq_u32(vreinterpretq_u32_u64(maskH), OH);
	outflankH = vandq_u32(vbicq_u32(outflankH, vsubq_u32(outflankH, one)), PH);
	flippedH = vandq_u32(vreinterpretq_u32_u64(maskH), vqsubq_u32(outflankH, one));

	flippedLH = vtrnq_u32(flippedL, flippedH);
	flipped = vreinterpretq_u64_u32(vorrq_u32(flippedLH.val[0], flippedLH.val[1]));

	return vget_lane_u64(vorr_u64(vget_low_u64(flipped), vget_high_u64(flipped)), 0);
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
	uint32x4_t PP = vreinterpretq_u32_u64(vdupq_n_u64(P));
	uint32x4_t OO = vreinterpretq_u32_u64(vdupq_n_u64(O));
	uint32x4_t PH = vtrnq_u32(PP, PP).val[1];
	uint32x4_t OH = vtrnq_u32(OO, OO).val[1];
	uint32x4_t outflankL, outflankH, flippedL, flippedH;
	uint32x4x2_t flippedLH;
	uint64x2_t flipped;
	const uint64x2_t maskL = { 0x0000000002020202, 0x0000000004081020 };
	const uint64x2_t maskH = { 0x02020200000000fc, 0x1008040000000000 };
	const uint32x4_t msb = vdupq_n_u32(0x80000000);
	const uint32x4_t one = vdupq_n_u32(1);

	outflankL = vandq_u32(vshlq_u32(msb, vnegq_s32(vreinterpretq_s32_u32(
		vclzq_u32(vbicq_u32(vreinterpretq_u32_u64(maskL), OO))))), PP);
	flippedL = vandq_u32(vreinterpretq_u32_u64(maskL), vreinterpretq_u32_s32(
		vnegq_s32(vreinterpretq_s32_u32(vaddq_u32(outflankL, outflankL)))));

	outflankH = vbicq_u32(vreinterpretq_u32_u64(maskH), OH);
	outflankH = vandq_u32(vbicq_u32(outflankH, vsubq_u32(outflankH, one)), PH);
	flippedH = vandq_u32(vreinterpretq_u32_u64(maskH), vqsubq_u32(outflankH, one));

	flippedLH = vtrnq_u32(flippedL, flippedH);
	flipped = vreinterpretq_u64_u32(vorrq_u32(flippedLH.val[0], flippedLH.val[1]));

	return vget_lane_u64(vorr_u64(vget_low_u64(flipped), vget_high_u64(flipped)), 0);
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
	uint32x4_t PP = vreinterpretq_u32_u64(vdupq_n_u64(P));
	uint32x4_t OO = vreinterpretq_u32_u64(vdupq_n_u64(O));
	uint32x4_t PL = vsetq_lane_u32(vgetq_lane_u32(PP, 0), PP, 3);	// LLHL
	uint32x4_t OL = vsetq_lane_u32(vgetq_lane_u32(OO, 0), OO, 3);
	uint32x4_t outflankL, outflankH, flippedH4, flippedL;
	uint32x2x2_t flippedLH;
	uint64x1_t flipped;
	const uint64x2_t maskL = { 0x0000000304040404, 0x0810204002010000 };
	const uint64x2_t maskH = { 0x000000f804040400, 0x0001020020100800 };
	const uint32x4_t msb = vdupq_n_u32(0x80000000);
	const uint32x4_t one = vdupq_n_u32(1);

	outflankL = vandq_u32(vshlq_u32(msb, vnegq_s32(vreinterpretq_s32_u32(
		vclzq_u32(vbicq_u32(vreinterpretq_u32_u64(maskL), OL))))), PL);
	flippedL = vandq_u32(vreinterpretq_u32_u64(maskL), vreinterpretq_u32_s32(
		vnegq_s32(vreinterpretq_s32_u32(vaddq_u32(outflankL, outflankL)))));

	outflankH = vbicq_u32(vreinterpretq_u32_u64(maskH), vtrnq_u32(OO, OO).val[1]);
	outflankH = vandq_u32(vbicq_u32(outflankH, vsubq_u32(outflankH, one)), vtrnq_u32(PP, PP).val[1]);
	flippedH4 = vandq_u32(vreinterpretq_u32_u64(maskH), vqsubq_u32(outflankH, one));

	flippedLH = vtrn_u32(vget_high_u32(flippedL), vorr_u32(vget_low_u32(flippedH4), vget_high_u32(flippedH4)));
	flipped = vreinterpret_u64_u32(vorr_u32(vorr_u32(flippedLH.val[0], flippedLH.val[1]), vget_low_u32(flippedL)));

	return vget_lane_u64(flipped, 0);
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
	uint32x4_t PP = vreinterpretq_u32_u64(vdupq_n_u64(P));
	uint32x4_t OO = vreinterpretq_u32_u64(vdupq_n_u64(O));
	uint32x4_t PL = vsetq_lane_u32(vgetq_lane_u32(PP, 0), PP, 3);	// LLHL
	uint32x4_t OL = vsetq_lane_u32(vgetq_lane_u32(OO, 0), OO, 3);
	uint32x4_t outflankL, outflankH, flippedH4, flippedL;
	uint32x2x2_t flippedLH;
	uint64x1_t flipped;
	const uint64x2_t maskL = { 0x0000000708080808, 0x1020408004020100 };
	const uint64x2_t maskH = { 0x000000f008080800, 0x0102040040201000 };
	const uint32x4_t msb = vdupq_n_u32(0x80000000);
	const uint32x4_t one = vdupq_n_u32(1);

	outflankL = vandq_u32(vshlq_u32(msb, vnegq_s32(vreinterpretq_s32_u32(
		vclzq_u32(vbicq_u32(vreinterpretq_u32_u64(maskL), OL))))), PL);
	flippedL = vandq_u32(vreinterpretq_u32_u64(maskL), vreinterpretq_u32_s32(
		vnegq_s32(vreinterpretq_s32_u32(vaddq_u32(outflankL, outflankL)))));

	outflankH = vbicq_u32(vreinterpretq_u32_u64(maskH), vtrnq_u32(OO, OO).val[1]);
	outflankH = vandq_u32(vbicq_u32(outflankH, vsubq_u32(outflankH, one)), vtrnq_u32(PP, PP).val[1]);
	flippedH4 = vandq_u32(vreinterpretq_u32_u64(maskH), vqsubq_u32(outflankH, one));

	flippedLH = vtrn_u32(vget_high_u32(flippedL), vorr_u32(vget_low_u32(flippedH4), vget_high_u32(flippedH4)));
	flipped = vreinterpret_u64_u32(vorr_u32(vorr_u32(flippedLH.val[0], flippedLH.val[1]), vget_low_u32(flippedL)));

	return vget_lane_u64(flipped, 0);
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
	uint32x4_t PP = vreinterpretq_u32_u64(vdupq_n_u64(P));
	uint32x4_t OO = vreinterpretq_u32_u64(vdupq_n_u64(O));
	uint32x4_t PL = vsetq_lane_u32(vgetq_lane_u32(PP, 0), PP, 3);	// LLHL
	uint32x4_t OL = vsetq_lane_u32(vgetq_lane_u32(OO, 0), OO, 3);
	uint32x4_t outflankL, outflankH, flippedH4, flippedL;
	uint32x2x2_t flippedLH;
	uint64x1_t flipped;
	const uint64x2_t maskL = { 0x0000000f10101010, 0x2040800008040201 };
	const uint64x2_t maskH = { 0x000000e010101000, 0x0204080080402000 };
	const uint32x4_t msb = vdupq_n_u32(0x80000000);
	const uint32x4_t one = vdupq_n_u32(1);

	outflankL = vandq_u32(vshlq_u32(msb, vnegq_s32(vreinterpretq_s32_u32(
		vclzq_u32(vbicq_u32(vreinterpretq_u32_u64(maskL), OL))))), PL);
	flippedL = vandq_u32(vreinterpretq_u32_u64(maskL), vreinterpretq_u32_s32(
		vnegq_s32(vreinterpretq_s32_u32(vaddq_u32(outflankL, outflankL)))));

	outflankH = vbicq_u32(vreinterpretq_u32_u64(maskH), vtrnq_u32(OO, OO).val[1]);
	outflankH = vandq_u32(vbicq_u32(outflankH, vsubq_u32(outflankH, one)), vtrnq_u32(PP, PP).val[1]);
	flippedH4 = vandq_u32(vreinterpretq_u32_u64(maskH), vqsubq_u32(outflankH, one));

	flippedLH = vtrn_u32(vget_high_u32(flippedL), vorr_u32(vget_low_u32(flippedH4), vget_high_u32(flippedH4)));
	flipped = vreinterpret_u64_u32(vorr_u32(vorr_u32(flippedLH.val[0], flippedLH.val[1]), vget_low_u32(flippedL)));

	return vget_lane_u64(flipped, 0);
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
	uint32x4_t PP = vreinterpretq_u32_u64(vdupq_n_u64(P));
	uint32x4_t OO = vreinterpretq_u32_u64(vdupq_n_u64(O));
	uint32x4_t PL = vsetq_lane_u32(vgetq_lane_u32(PP, 0), PP, 3);	// LLHL
	uint32x4_t OL = vsetq_lane_u32(vgetq_lane_u32(OO, 0), OO, 3);
	uint32x4_t outflankL, outflankH, flippedH4, flippedL;
	uint32x2x2_t flippedLH;
	uint64x1_t flipped;
	const uint64x2_t maskL = { 0x0000001f20202020, 0x4080000010080402 };
	const uint64x2_t maskH = { 0x000000c020202000, 0x0408100000804000 };
	const uint32x4_t msb = vdupq_n_u32(0x80000000);
	const uint32x4_t one = vdupq_n_u32(1);

	outflankL = vandq_u32(vshlq_u32(msb, vnegq_s32(vreinterpretq_s32_u32(
		vclzq_u32(vbicq_u32(vreinterpretq_u32_u64(maskL), OL))))), PL);
	flippedL = vandq_u32(vreinterpretq_u32_u64(maskL), vreinterpretq_u32_s32(
		vnegq_s32(vreinterpretq_s32_u32(vaddq_u32(outflankL, outflankL)))));

	outflankH = vbicq_u32(vreinterpretq_u32_u64(maskH), vtrnq_u32(OO, OO).val[1]);
	outflankH = vandq_u32(vbicq_u32(outflankH, vsubq_u32(outflankH, one)), vtrnq_u32(PP, PP).val[1]);
	flippedH4 = vandq_u32(vreinterpretq_u32_u64(maskH), vqsubq_u32(outflankH, one));

	flippedLH = vtrn_u32(vget_high_u32(flippedL), vorr_u32(vget_low_u32(flippedH4), vget_high_u32(flippedH4)));
	flipped = vreinterpret_u64_u32(vorr_u32(vorr_u32(flippedLH.val[0], flippedLH.val[1]), vget_low_u32(flippedL)));

	return vget_lane_u64(flipped, 0);
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
	uint64x2_t PP = vdupq_n_u64(P);
	uint32x4_t outflankL;
	uint64x2_t flipped, outflankH;
	const uint64x2_t maskL = { 0x0000003f40404040, 0x0000000020100804 };
	const uint64x2_t maskH = { 0x4040400000000000, 0x0810200000000000 };
	const uint32x4_t msb = vdupq_n_u32(0x80000000);
	const uint64x2_t one = vdupq_n_u64(1);

	outflankL = vshlq_u32(msb, vnegq_s32(vclzq_s32(vreinterpretq_s32_u64(not_O_in_mask(maskL, O)))));
	outflankL = vandq_u32(outflankL, vreinterpretq_u32_u64(PP));
	flipped = vandq_u64(maskL, vreinterpretq_u64_s32(vnegq_s32(vreinterpretq_s32_u32(vaddq_u32(outflankL, outflankL)))));

	outflankH = not_O_in_mask(maskH, O);
	outflankH = vandq_u64(vbicq_u64(outflankH, vsubq_u64(outflankH, one)), PP);
	flipped = vbslq_u64(maskH, vqsubq_u64(outflankH, one), flipped);

	return vget_lane_u64(vorr_u64(vget_low_u64(flipped), vget_high_u64(flipped)), 0);
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
	uint64x2_t PP = vdupq_n_u64(P);
	uint32x4_t outflankL;
	uint64x2_t flipped, outflankH;
	const uint64x2_t maskL = { 0x0000007f80808080, 0x0000000040201008 };
	const uint64x2_t maskH = { 0x8080800000000000, 0x1020400000000000 };
	const uint32x4_t msb = vdupq_n_u32(0x80000000);
	const uint64x2_t one = vdupq_n_u64(1);

	outflankL = vshlq_u32(msb, vnegq_s32(vclzq_s32(vreinterpretq_s32_u64(not_O_in_mask(maskL, O)))));
	outflankL = vandq_u32(outflankL, vreinterpretq_u32_u64(PP));
	flipped = vandq_u64(maskL, vreinterpretq_u64_s32(vnegq_s32(vreinterpretq_s32_u32(vaddq_u32(outflankL, outflankL)))));

	outflankH = not_O_in_mask(maskH, O);
	outflankH = vandq_u64(vbicq_u64(outflankH, vsubq_u64(outflankH, one)), PP);
	flipped = vbslq_u64(maskH, vqsubq_u64(outflankH, one), flipped);

	return vget_lane_u64(vorr_u64(vget_low_u64(flipped), vget_high_u64(flipped)), 0);
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
	unsigned int outflank_h;
	unsigned long long flipped, outflank_v, outflank_d7;

	outflank_v = outflank_right(O, 0x0000000101010101) & P;
	flipped  = (outflank_v * -2) & 0x0000000101010101;

	outflank_d7 = outflank_right(O, 0x0000000204081020) & P;
	flipped |= (outflank_d7 * -2) & 0x0000000204081020;

	outflank_h = ((unsigned int) (O >> 16) + 0x02000000) & (unsigned int) (P >> 16);
	flipped |= (((unsigned long long) outflank_h << 16) - outflank_h) & 0x00007e0000000000;

	flipped |= (((P >> 8) & 0x0001000000000000) | ((P >> 9) & 0x0002000000000000)) & O;

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
	unsigned int outflank_h;
	unsigned long long flipped, outflank_v, outflank_d7;

	outflank_v = outflank_right(O, 0x0000000202020202) & P;
	flipped  = (outflank_v * -2) & 0x0000000202020202;

	outflank_d7 = outflank_right(O, 0x0000000408102040) & P;
	flipped |= (outflank_d7 * -2) & 0x0000000408102040;

	outflank_h = ((unsigned int) (O >> 16) + 0x04000000) & (unsigned int) (P >> 16);
	flipped |= (((unsigned long long) outflank_h << 16) - outflank_h) & 0x00007c0000000000;

	flipped |= (((P >> 8) & 0x0002000000000000) | ((P >> 9) & 0x0004000000000000)) & O;

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

	outflank_v = outflank_right(O, 0x0000000404040404) & P;
	flipped  = (outflank_v * -2) & 0x0000000404040404;

	outflank_d7 = outflank_right(O, 0x0000000810204080) & P;
	flipped |= (outflank_d7 * -2) & 0x0000000810204080;

	outflank_h = ((unsigned int) (O >> 16) + 0x08000000) & (unsigned int) (P >> 16);
	flipped |= (((unsigned long long) outflank_h << 16) - outflank_h) & 0x0000780000000000;

	flipped |= ((((P >> 9) | (P << 9)) & 0x0008000200000000) | ((P >> 8) & 0x0004000000000000)
		| ((P >> 7) & 0x0002000000000000) | ((P << 1) & 0x0000020000000000)) & O;

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
	unsigned int outflank_h;
	unsigned long long flipped, outflank_v, outflank_d;

	outflank_v = outflank_right(O, 0x0000000808080808) & P;
	flipped  = (outflank_v * -2) & 0x0000000808080808;

	outflank_h = OUTFLANK_3[(O >> 41) & 0x3f] & rotl8(P >> 40, 3);
	flipped |= (unsigned long long)(unsigned char) FLIPPED_3_H[outflank_h] << 40;

	outflank_d = OUTFLANK_3[(((unsigned int) (O >> 16) & 0x08142240) * 0x01010101) >> 25];
	outflank_d &= ((P & 0x0000081422418000) * 0x0101010101010101) >> 53;	// hgfedc[bahgf]...
	flipped |= FLIPPED_3_H[outflank_d] & 0x0000081422400000;	// A3D6H2

	flipped |= (((P >> 9) & 0x0010000000000000) | ((P >> 8) & 0x0008000000000000) | ((P >> 7) & 0x0004000000000000)) & O;

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
	unsigned int outflank_h;
	unsigned long long flipped, outflank_v, outflank_d;

	outflank_v = outflank_right(O, 0x0000001010101010) & P;
	flipped  = (outflank_v * -2) & 0x0000001010101010;

	outflank_h = OUTFLANK_4[(O >> 41) & 0x3f] & rotl8(P >> 40, 2);
	flipped |= (unsigned long long)(unsigned char) FLIPPED_4_H[outflank_h] << 40;

	outflank_d = OUTFLANK_4[(((unsigned int) (O >> 16) & 0x10284402) * 0x01010101) >> 25];
	outflank_d &= ((P & 0x0000102844820100) * 0x0101010101010101) >> 54;	// hgfed[cbahg]...
	flipped |= FLIPPED_4_H[outflank_d] & 0x0000102844020000;	// A2E6H3

	flipped |= (((P >> 9) & 0x0020000000000000) | ((P >> 8) & 0x0010000000000000) | ((P >> 7) & 0x0008000000000000)) & O;

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

	outflank_v = outflank_right(O, 0x0000002020202020) & P;
	flipped  = (outflank_v * -2) & 0x0000002020202020;

	outflank_d9 = outflank_right(O, 0x0000001008040201) & P;
	flipped |= (outflank_d9 * -2) & 0x0000001008040201;

	outflank_h = outflank_right_H((unsigned int) (O >> 41) << 28) & (unsigned int) (P >> 13);
	flipped |= (unsigned long long) (outflank_h * (unsigned int) -2) << 13;

	flipped |= ((((P >> 7) | (P << 7)) & 0x0010004000000000) | ((P >> 8) & 0x0020000000000000)
		| ((P >> 9) & 0x0040000000000000) | ((P >> 1) & 0x0000400000000000)) & O;

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

	outflank_v = outflank_right(O, 0x0000004040404040) & P;
	flipped  = (outflank_v * -2) & 0x0000004040404040;

	outflank_d9 = outflank_right(O, 0x0000002010080402) & P;
	flipped |= (outflank_d9 * -2) & 0x0000002010080402;

	outflank_h = outflank_right_H((unsigned int) (O >> 41) << 27) & (unsigned int) (P >> 14);
	flipped |= (unsigned long long) (outflank_h * (unsigned int) -2) << 14;

	flipped |= (((P >> 7) & 0x0020000000000000) | ((P >> 8) & 0x0040000000000000)) & O;

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

	outflank_v = outflank_right(O, 0x0000008080808080) & P;
	flipped  = (outflank_v * -2) & 0x0000008080808080;

	outflank_d9 = outflank_right(O, 0x0000004020100804) & P;
	flipped |= (outflank_d9 * -2) & 0x0000004020100804;

	outflank_h = outflank_right_H((unsigned int) (O >> 41) << 26) & (unsigned int) (P >> 15);
	flipped |= (unsigned long long) (outflank_h * (unsigned int) -2) << 15;

	flipped |= (((P >> 7) & 0x0040000000000000) | ((P >> 8) & 0x0080000000000000)) & O;

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
	unsigned long long flipped, outflank_v, outflank_d7;
	unsigned int outflank_h;

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
	unsigned long long flipped, outflank_v, outflank_d7;
	unsigned int outflank_h;

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
	unsigned long long flipped, outflank_v, outflank_d7;
	unsigned int outflank_h;

	outflank_v = outflank_right(O, 0x0000040404040404) & P;
	flipped  = (outflank_v * -2) & 0x0000040404040404;

	outflank_d7 = outflank_right(O, 0x0000081020408000) & P;
	flipped |= (outflank_d7 * -2) & 0x0000081020408000;

	flipped |= (((P << 9) & 0x0000020000000000) | ((P << 1) & 0x0002000000000000)) & O;

	outflank_h = ((unsigned int) (O >> 24) + 0x08000000) & (unsigned int) (P >> 24);
	flipped |= (((unsigned long long) outflank_h << 24) - outflank_h) & 0x0078000000000000;

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
	unsigned int outflank_h;
	unsigned long long flipped, outflank_v, outflank_d7;

	outflank_v = outflank_right(O, 0x0000202020202020) & P;
	flipped  = (outflank_v * -2) & 0x0000202020202020;

	outflank_d7 = outflank_right(O, 0x0000100804020100) & P;
	flipped |= (outflank_d7 * -2) & 0x0000100804020100;

	flipped |= (((P << 7) & 0x0000400000000000) | ((P >> 1) & 0x0040000000000000)) & O;

	outflank_h = outflank_right_H((unsigned int) (O >> 49) << 28) & (unsigned int) (P >> 21);
	flipped |= (unsigned long long) (outflank_h * (unsigned int) -2) << 21;

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
	unsigned long long flipped, outflank_v, outflank_h, outflank_d7;

	outflank_v = outflank_right(O, 0x0004040404040404) & P;
	flipped  = (outflank_v * -2) & 0x0004040404040404;

	outflank_d7 = outflank_right(O, 0x0008102040800000) & P;
	flipped |= (outflank_d7 * -2) & 0x0008102040800000;

	flipped |= (((P << 9) & 0x0002000000000000) | ((P << 1) & 0x0200000000000000)) & O;

	outflank_h = (O + 0x0800000000000000) & P;
	flipped |= (outflank_h - (outflank_h >> 8)) & 0x7800000000000000;

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

	outflank_d = OUTFLANK_3[(((unsigned int) (O >> 32) & 0x08142240) * 0x01010101) >> 25];
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

	outflank_d = OUTFLANK_4[(((unsigned int) (O >> 32) & 0x10284402) * 0x01010101) >> 25];
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
	unsigned int outflank_h;
	unsigned long long flipped, outflank_v, outflank_d7;

	outflank_v = outflank_right(O, 0x0020202020202020) & P;
	flipped  = (outflank_v * -2) & 0x0020202020202020;

	outflank_d7 = outflank_right(O, 0x0010080402010000) & P;
	flipped |= (outflank_d7 * -2) & 0x0010080402010000;

	flipped |= (((P << 7) & 0x0040000000000000) | ((P >> 1) & 0x4000000000000000)) & O;

	outflank_h = outflank_right_H((unsigned int) (O >> 57) << 28) & (unsigned int) (P >> 29);
	flipped |= (unsigned long long) (outflank_h * (unsigned int) -2) << 29;

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
unsigned long long (*flip_neon[])(const unsigned long long, const unsigned long long) = {
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

#endif
