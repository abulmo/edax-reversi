/**
 * @file flip_sse.c
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
 * If the OUTFLANK search is in LSB to MSB direction, LS1B or carry propagation 
 * can be used to determine contiguous opponent discs.
 * If the OUTFLANK search is in MSB to LSB direction, MS1B using int-float
 * conversion can be used.
 *
 * @date 1998 - 2020
 * @author Richard Delorme
 * @author Toshihiko Okuhara
 * @version 4.4
 */

#include "bit.h"
#include <stdio.h>

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

static const unsigned char OUTFLANK_7[64] = {
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20,
	0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x08, 0x08, 0x08, 0x08, 0x04, 0x04, 0x02, 0x01
};

/** flip array (indexed with rotated outflank, returns inner 6 bits) */
static const unsigned long long FLIPPED_2_V[25] = {	// ...ahgfe
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

static const unsigned long long FLIPPED_7_V[38] = {
	0x0000000000000000, 0x00ffffffffffff00, 0x00ffffffffff0000, 0x0000000000000000,
	0x00ffffffff000000, 0x0000000000000000, 0x0000000000000000, 0x0000000000000000,
	0x00ffffff00000000, 0x0000000000000000, 0x0000000000000000, 0x0000000000000000,
	0x0000000000000000, 0x0000000000000000, 0x0000000000000000, 0x0000000000000000,
	0x00ffff0000000000, 0x0000000000000000, 0x0000000000000000, 0x0000000000000000,
	// static const unsigned long long FLIPPED_5_V[18] = {	// ...dcbah
		0x0000000000000000, 0x00ff000000000000, 0x000000ffffffff00, 0x00ff00ffffffff00,
		0x000000ffffff0000, 0x00ff00ffffff0000, 0x0000000000000000, 0x0000000000000000,
		0x000000ffff000000, 0x00ff00ffff000000, 0x0000000000000000, 0x0000000000000000,
	0x00ff000000000000, 0x0000000000000000, 0x0000000000000000, 0x0000000000000000,
		0x000000ff00000000, 0x00ff00ff00000000
};

static const unsigned long long *FLIPPED_5_V = FLIPPED_7_V + 20;


#define minusone	_mm_set1_epi32(-1)

#define	SWAP64	0x4e	// for _mm_shuffle_epi32
#define	SWAP32	0xb1
#define	DUPLO	0x44
#define	DUPHI	0xee


/*
 * Extract most significant bit set from 4 x u31 (3 inst)
 * valid only for x < 0x7fffff80
 */
static inline __m128i MS1B_epu31(const __m128i x) {
	const __m128 exp_mask = _mm_castsi128_ps(_mm_set1_epi32(0xff800000));
	return _mm_cvtps_epi32(_mm_and_ps(_mm_cvtepi32_ps(x), exp_mask));	// clear mantissa = non msb bits
}

/*
 * Extract most significant bit set from 4 x u32 (7 inst)
 * cf. http://umezawa.dyndns.info/wordpress/?p=3743
 */
static inline __m128i MS1B_epu32(const __m128i x) {
	__m128i y = MS1B_epu31(x);
	return _mm_andnot_si128(_mm_srli_epi32(_mm_srai_epi32(y, 31), 1), y);	// clear except sign if negative
}

/*
 * Extract most significant bit set from 2 x u64 (12 inst)
 */
static inline __m128i MS1B_epu64(const __m128i x) {
	__m128i y = MS1B_epu32(x);
	return _mm_and_si128(y, _mm_cmpeq_epi32(_mm_srli_epi64(y, 32), _mm_setzero_si128()));	// clear low if high != 0
}

/*
 * Extract most significant bit set (6 inst)
 * valid only for x < 0x000fffffffffffffULL
 *
 * https://software.intel.com/en-us/forums/intel-isa-extensions/topic/301988
 * https://stackoverflow.com/questions/41144668/how-to-efficiently-perform-double-int64-conversions-with-sse-avx/41148578#41148578
 */
 static inline __m128i MS1B_epu52(const __m128i x) {
	const __m128d k1e52 = _mm_set1_pd(0x0010000000000000);
	const __m128d exp_mask = _mm_castsi128_pd(_mm_set1_epi64x(0xfff0000000000000));
	__m128d f;
	f = _mm_or_pd(_mm_castsi128_pd(x), k1e52);	// construct double x + 2^52
	f = _mm_sub_pd(f, k1e52);	// extract 2^52 from double -- mantissa will be automatically normalized
	f = _mm_and_pd(f, exp_mask);	// clear mantissa = non msb bits
	f = _mm_add_pd(f, k1e52);	// add 2^52 to push back the msb
	f = _mm_xor_pd(f, k1e52);	// remove exponent
	return _mm_castpd_si128(f);
}

/**
 * Make inverted flip mask if opponent's disc are surrounded by player's.
 *
 * 0xffffffffffffffffULL (-1) if outflank is 0
 * 0x0000000000000000ULL ( 0) if a 1 is in 64 bit
 */
static inline __m128i flipmask (const __m128i outflank) {
	return _mm_cmpeq_epi32(_mm_shuffle_epi32(outflank, SWAP32), outflank);
}

/**
 * Load 2 unsigned long longs into xmm.
 */
static inline __m128i load64x2 (const unsigned long long *x0, const unsigned long long *x1) {
	return _mm_castps_si128(_mm_loadh_pi(_mm_castsi128_ps(_mm_loadl_epi64((__m128i *) x0)), (__m64 *) x1));
}

/**
 * Compute flipped discs when playing on square A1.
 *
 * @param P player's disc pattern.
 * @param O opponent's disc pattern.
 * @return flipped disc pattern.
 */
static __m128i vectorcall flip_A1(const __m128i OP)
{
	__m128i PP = _mm_shuffle_epi32(OP, DUPLO);
	__m128i OO = _mm_shuffle_epi32(OP, DUPHI);
	__m128i	outflank_vd, outflank_h, flipped;
	const __m128i mask = _mm_set_epi64x(0x8040201008040200, 0x0101010101010100);
	const __m128i next_h = _mm_loadl_epi64((__m128i *) &X_TO_BIT[1]);

	outflank_vd = _mm_andnot_si128(OO, mask);
	outflank_vd = _mm_and_si128(_mm_andnot_si128(_mm_add_epi64(outflank_vd, minusone), outflank_vd), PP);
	outflank_vd = _mm_add_epi64(outflank_vd, minusone);
	flipped = _mm_and_si128(mask, _mm_add_epi64(outflank_vd, _mm_srli_epi64(outflank_vd, 63)));

	outflank_h = _mm_and_si128(_mm_add_epi8(OO, next_h), PP);
	flipped = _mm_or_si128(flipped, _mm_subs_epu8(outflank_h, next_h));

	return _mm_or_si128(flipped, _mm_shuffle_epi32(flipped, SWAP64));
}

/**
 * Compute flipped discs when playing on square B1.
 *
 * @param P player's disc pattern.
 * @param O opponent's disc pattern.
 * @return flipped disc pattern.
 */
static __m128i vectorcall flip_B1(const __m128i OP)
{
	__m128i PP = _mm_shuffle_epi32(OP, DUPLO);
	__m128i OO = _mm_shuffle_epi32(OP, DUPHI);
	__m128i	outflank_vd, outflank_h, flipped;
	const __m128i mask = _mm_set_epi64x(0x0080402010080400, 0x0202020202020200);
	const __m128i next_h = _mm_loadl_epi64((__m128i *) &X_TO_BIT[2]);

	outflank_vd = _mm_andnot_si128(OO, mask);
	outflank_vd = _mm_and_si128(_mm_andnot_si128(_mm_add_epi64(outflank_vd, minusone), outflank_vd), PP);
	outflank_vd = _mm_add_epi64(outflank_vd, minusone);
	flipped = _mm_and_si128(mask, _mm_add_epi64(outflank_vd, _mm_srli_epi64(outflank_vd, 63)));

	outflank_h = _mm_and_si128(_mm_add_epi8(OO, next_h), PP);
	flipped = _mm_or_si128(flipped, _mm_subs_epu8(outflank_h, next_h));

	return _mm_or_si128(flipped, _mm_shuffle_epi32(flipped, SWAP64));
}

/**
 * Compute flipped discs when playing on square C1.
 *
 * @param P player's disc pattern.
 * @param O opponent's disc pattern.
 * @return flipped disc pattern.
 */
static __m128i vectorcall flip_C1(const __m128i OP)
{
	__m128i PP = _mm_shuffle_epi32(OP, DUPLO);
	__m128i OO = _mm_shuffle_epi32(OP, DUPHI);
	__m128i	outflank_vd, flipped, flipped_h_b1b2;
	const __m128i mask = _mm_set_epi64x(0x0000804020100800, 0x0404040404040400);

	outflank_vd = _mm_andnot_si128(OO, mask);
	outflank_vd = _mm_and_si128(_mm_andnot_si128(_mm_add_epi64(outflank_vd, minusone), outflank_vd), PP);
	outflank_vd = _mm_add_epi64(outflank_vd, minusone);
	flipped = _mm_and_si128(mask, _mm_add_epi64(outflank_vd, _mm_srli_epi64(outflank_vd, 63)));

	flipped_h_b1b2 = _mm_and_si128(_mm_adds_epu8(OO, _mm_set_epi8(0, 0, 0, 0, 0, -1, 0, -1, 0, 0, 0, 0, 0, 0, 0, 0x08)), PP);
	flipped_h_b1b2 = _mm_srli_epi64(_mm_mullo_epi16(flipped_h_b1b2, _mm_set_epi16(0, 0, 0x0002, 0x0200, 0, 0, 0, 0x00ff)), 8);
	flipped_h_b1b2 = _mm_and_si128(_mm_and_si128(flipped_h_b1b2, OO), _mm_set_epi16(0, 0, 0, 0x0202, 0, 0, 0, 0x0078));
	flipped = _mm_or_si128(flipped, flipped_h_b1b2);

	return _mm_or_si128(flipped, _mm_shuffle_epi32(flipped, SWAP64));
}

/**
 * Compute flipped discs when playing on square D1.
 *
 * @param P player's disc pattern.
 * @param O opponent's disc pattern.
 * @return flipped disc pattern.
 */
static __m128i vectorcall flip_D1(const __m128i OP)
{
	__m128i	outflank_v, flipped, index_d;
	unsigned int outflank_h, outflank_d;
	const __m128i mask = _mm_set_epi64x(0x0000008041221408, 0x0808080808080800);	// A4D1H5

	outflank_v = _mm_andnot_si128(_mm_shuffle_epi32(OP, DUPHI), mask);
	outflank_v = _mm_and_si128(_mm_andnot_si128(_mm_add_epi64(outflank_v, minusone), outflank_v), OP);
	outflank_v = _mm_add_epi64(outflank_v, minusone);
	flipped = _mm_add_epi64(outflank_v, _mm_srli_epi64(outflank_v, 63));

	index_d = _mm_sad_epu8(_mm_and_si128(OP, _mm_shuffle_epi32(mask, DUPHI)), _mm_setzero_si128());
	outflank_d = OUTFLANK_3[(_mm_extract_epi16(index_d, 4) >> 1) & 0x3f] & rotl8(_mm_cvtsi128_si32(index_d), 3);

	flipped = _mm_castps_si128(_mm_loadh_pi(_mm_castsi128_ps(flipped), (__m64 *) &FLIPPED_3_H[outflank_d]));
	flipped = _mm_and_si128(mask, flipped);

	outflank_h = OUTFLANK_3[(_mm_extract_epi16(OP, 4) >> 1) & 0x3f] & rotl8(_mm_cvtsi128_si64(OP), 3);
	flipped = _mm_or_si128(flipped, _mm_srli_epi64(_mm_loadl_epi64((__m128i *) &FLIPPED_3_H[outflank_h]), 56));

	return _mm_or_si128(flipped, _mm_shuffle_epi32(flipped, SWAP64));
}

/**
 * Compute flipped discs when playing on square E1.
 *
 * @param P player's disc pattern.
 * @param O opponent's disc pattern.
 * @return flipped disc pattern.
 */
static __m128i vectorcall flip_E1(const __m128i OP)
{
	__m128i	outflank_v, flipped, index_d;
	unsigned int outflank_h, outflank_d;
	const __m128i mask = _mm_set_epi64x(0x0000000182442810, 0x1010101010101000);	// A5E1H4

	outflank_v = _mm_andnot_si128(_mm_shuffle_epi32(OP, DUPHI), mask);
	outflank_v = _mm_and_si128(_mm_andnot_si128(_mm_add_epi64(outflank_v, minusone), outflank_v), OP);
	outflank_v = _mm_add_epi64(outflank_v, minusone);
	flipped = _mm_add_epi64(outflank_v, _mm_srli_epi64(outflank_v, 63));

	index_d = _mm_sad_epu8(_mm_and_si128(OP, _mm_shuffle_epi32(mask, DUPHI)), _mm_setzero_si128());
	outflank_d = OUTFLANK_4[(_mm_extract_epi16(index_d, 4) >> 1) & 0x3f] & rotl8(_mm_cvtsi128_si32(index_d), 2);

	flipped = _mm_castps_si128(_mm_loadh_pi(_mm_castsi128_ps(flipped), (__m64 *) &FLIPPED_4_H[outflank_d]));
	flipped = _mm_and_si128(mask, flipped);

	outflank_h = OUTFLANK_4[(_mm_extract_epi16(OP, 4) >> 1) & 0x3f] & rotl8(_mm_cvtsi128_si64(OP), 2);
	flipped = _mm_or_si128(flipped, _mm_srli_epi64(_mm_loadl_epi64((__m128i *) &FLIPPED_4_H[outflank_h]), 56));

	return _mm_or_si128(flipped, _mm_shuffle_epi32(flipped, SWAP64));
}

/**
 * Compute flipped discs when playing on square F1.
 *
 * @param P player's disc pattern.
 * @param O opponent's disc pattern.
 * @return flipped disc pattern.
 */
static __m128i vectorcall flip_F1(const __m128i OP)
{
	__m128i PP = _mm_shuffle_epi32(OP, DUPLO);
	__m128i OO = _mm_shuffle_epi32(OP, DUPHI);
	__m128i	outflank_vd, flipped, outflank_h, flipped_h_g1g2;
	const __m128i mask = _mm_set_epi64x(0x0000010204081000, 0x2020202020202000);

	outflank_h = _mm_and_si128(MS1B_epu31(_mm_andnot_si128(OO, _mm_set_epi32(0, 0, 0, 0x0000001f))), PP);

	flipped_h_g1g2 = _mm_unpacklo_epi64(outflank_h, PP);
	flipped_h_g1g2 = _mm_srli_epi64(_mm_mullo_epi16(flipped_h_g1g2, _mm_set_epi16(0, 0, 1, 0x0100, 0, 0, 0, -0x0400)), 9);
	flipped_h_g1g2 = _mm_and_si128(_mm_and_si128(flipped_h_g1g2, OO), _mm_set_epi16(0, 0, 0, 0x4040, 0, 0, 0, 0x001f));

	outflank_vd = _mm_andnot_si128(OO, mask);
	outflank_vd = _mm_and_si128(_mm_andnot_si128(_mm_add_epi64(outflank_vd, minusone), outflank_vd), PP);
	outflank_vd = _mm_add_epi64(outflank_vd, minusone);
	flipped = _mm_or_si128(flipped_h_g1g2, _mm_and_si128(mask, _mm_add_epi64(outflank_vd, _mm_srli_epi64(outflank_vd, 63))));

	return _mm_or_si128(flipped, _mm_shuffle_epi32(flipped, SWAP64));
}

/**
 * Compute flipped discs when playing on square G1.
 *
 * @param P player's disc pattern.
 * @param O opponent's disc pattern.
 * @return flipped disc pattern.
 */
static __m128i vectorcall flip_G1(const __m128i OP)
{
	__m128i PP = _mm_shuffle_epi32(OP, DUPLO);
	__m128i OO = _mm_shuffle_epi32(OP, DUPHI);
	__m128i	outflank_vd, outflank_h, flipped;

	outflank_vd = _mm_andnot_si128(OO, _mm_set_epi64x(0x0001020408102000, 0x4040404040404000));
	outflank_vd = _mm_and_si128(_mm_and_si128(outflank_vd, _mm_sub_epi64(_mm_setzero_si128(), outflank_vd)), PP);
	flipped = _mm_sub_epi64(outflank_vd, _mm_andnot_si128(flipmask(outflank_vd), _mm_set1_epi64x(0x0000000000000100)));

	outflank_h = _mm_and_si128(MS1B_epu31(_mm_andnot_si128(OO, _mm_set_epi32(0, 0, 0, 0x0000003f))), PP);
	flipped = _mm_sub_epi8(flipped, _mm_add_epi8(outflank_h, outflank_h));
	flipped = _mm_and_si128(flipped, _mm_set_epi64x(0x0001020408102000, 0x404040404040403e));

	return _mm_or_si128(flipped, _mm_shuffle_epi32(flipped, SWAP64));
}

/**
 * Compute flipped discs when playing on square H1.
 *
 * @param P player's disc pattern.
 * @param O opponent's disc pattern.
 * @return flipped disc pattern.
 */
static __m128i vectorcall flip_H1(const __m128i OP)
{
	__m128i PP = _mm_shuffle_epi32(OP, DUPLO);
	__m128i OO = _mm_shuffle_epi32(OP, DUPHI);
	__m128i	outflank_vd, outflank_h, flipped;

	outflank_vd = _mm_andnot_si128(OO, _mm_set_epi64x(0x0102040810204000, 0x8080808080808000));
	outflank_vd = _mm_and_si128(_mm_and_si128(outflank_vd, _mm_sub_epi64(_mm_setzero_si128(), outflank_vd)), PP);
	flipped = _mm_sub_epi64(outflank_vd, _mm_andnot_si128(flipmask(outflank_vd), _mm_set1_epi64x(0x0000000000000100)));

	outflank_h = _mm_and_si128(MS1B_epu31(_mm_andnot_si128(OO, _mm_set_epi32(0, 0, 0, 0x0000007f))), PP);
	flipped = _mm_sub_epi8(flipped, _mm_add_epi8(outflank_h, outflank_h));
	flipped = _mm_and_si128(flipped, _mm_set_epi64x(0x0102040810204000, 0x808080808080807e));

	return _mm_or_si128(flipped, _mm_shuffle_epi32(flipped, SWAP64));
}

/**
 * Compute flipped discs when playing on square A2.
 *
 * @param P player's disc pattern.
 * @param O opponent's disc pattern.
 * @return flipped disc pattern.
 */
static __m128i vectorcall flip_A2(const __m128i OP)
{
	__m128i PP = _mm_shuffle_epi32(OP, DUPLO);
	__m128i OO = _mm_shuffle_epi32(OP, DUPHI);
	__m128i	outflank_vd, outflank_h, flipped;
	const __m128i mask = _mm_set_epi64x(0x4020100804020000, 0x0101010101010000);
	const __m128i next_h = _mm_loadl_epi64((__m128i *) &X_TO_BIT[9]);

	outflank_vd = _mm_andnot_si128(OO, mask);
	outflank_vd = _mm_and_si128(_mm_andnot_si128(_mm_add_epi64(outflank_vd, minusone), outflank_vd), PP);
	outflank_vd = _mm_add_epi64(outflank_vd, minusone);
	flipped = _mm_and_si128(mask, _mm_add_epi64(outflank_vd, _mm_srli_epi64(outflank_vd, 63)));

	outflank_h = _mm_and_si128(_mm_add_epi8(OO, next_h), PP);
	flipped = _mm_or_si128(flipped, _mm_subs_epu8(outflank_h, next_h));

	return _mm_or_si128(flipped, _mm_shuffle_epi32(flipped, SWAP64));
}

/**
 * Compute flipped discs when playing on square B2.
 *
 * @param P player's disc pattern.
 * @param O opponent's disc pattern.
 * @return flipped disc pattern.
 */
static __m128i vectorcall flip_B2(const __m128i OP)
{
	__m128i PP = _mm_shuffle_epi32(OP, DUPLO);
	__m128i OO = _mm_shuffle_epi32(OP, DUPHI);
	__m128i	outflank_vd, outflank_h, flipped;
	const __m128i mask = _mm_set_epi64x(0x8040201008040000, 0x0202020202020000);
	const __m128i next_h = _mm_loadl_epi64((__m128i *) &X_TO_BIT[10]);

	outflank_vd = _mm_andnot_si128(OO, mask);
	outflank_vd = _mm_and_si128(_mm_andnot_si128(_mm_add_epi64(outflank_vd, minusone), outflank_vd), PP);
	outflank_vd = _mm_add_epi64(outflank_vd, minusone);
	flipped = _mm_and_si128(mask, _mm_add_epi64(outflank_vd, _mm_srli_epi64(outflank_vd, 63)));

	outflank_h = _mm_and_si128(_mm_add_epi8(OO, next_h), PP);
	flipped = _mm_or_si128(flipped, _mm_subs_epu8(outflank_h, next_h));

	return _mm_or_si128(flipped, _mm_shuffle_epi32(flipped, SWAP64));
}

/**
 * Compute flipped discs when playing on square C2.
 *
 * @param P player's disc pattern.
 * @param O opponent's disc pattern.
 * @return flipped disc pattern.
 */
static __m128i vectorcall flip_C2(const __m128i OP)
{
	__m128i PP = _mm_shuffle_epi32(OP, DUPLO);
	__m128i OO = _mm_shuffle_epi32(OP, DUPHI);
	__m128i	outflank_vd, flipped, flipped_h_b2b3;
	const __m128i mask = _mm_set_epi64x(0x0080402010080000, 0x0404040404040000);

	outflank_vd = _mm_andnot_si128(OO, mask);
	outflank_vd = _mm_and_si128(_mm_andnot_si128(_mm_add_epi64(outflank_vd, minusone), outflank_vd), PP);
	outflank_vd = _mm_add_epi64(outflank_vd, minusone);
	flipped = _mm_and_si128(mask, _mm_add_epi64(outflank_vd, _mm_srli_epi64(outflank_vd, 63)));

	flipped_h_b2b3 = _mm_and_si128(_mm_adds_epu8(OO, _mm_set_epi8(0, 0, 0, 0, -1, 0, -1, 0, 0, 0, 0, 0, 0, 0, 0x08, 0)), PP);
	flipped_h_b2b3 = _mm_mullo_epi16(_mm_srli_epi64(flipped_h_b2b3, 8), _mm_set_epi16(0, 0, 0x0002, 0x0200, 0, 0, 0, 0x00ff));
	flipped_h_b2b3 = _mm_and_si128(_mm_and_si128(flipped_h_b2b3, OO), _mm_set_epi16(0, 0, 0x0002, 0x0200, 0, 0, 0, 0x7800));
	flipped = _mm_or_si128(flipped, flipped_h_b2b3);

	return _mm_or_si128(flipped, _mm_shuffle_epi32(flipped, SWAP64));
}

/**
 * Compute flipped discs when playing on square D2.
 *
 * @param P player's disc pattern.
 * @param O opponent's disc pattern.
 * @return flipped disc pattern.
 */
static __m128i vectorcall flip_D2(const __m128i OP)
{
	__m128i	outflank_v, flipped, index_d;
	unsigned int outflank_h, outflank_d;
	const __m128i mask = _mm_set_epi64x(0x000080412214ff00, 0x0808080808080000);

	outflank_v = _mm_andnot_si128(_mm_shuffle_epi32(OP, DUPHI), mask);
	outflank_v = _mm_and_si128(_mm_andnot_si128(_mm_add_epi64(outflank_v, minusone), outflank_v), OP);
	outflank_v = _mm_add_epi64(outflank_v, minusone);
	flipped = _mm_add_epi64(outflank_v, _mm_srli_epi64(outflank_v, 63));

	index_d = _mm_sad_epu8(_mm_and_si128(OP, _mm_set1_epi64x(0x0000804122140800)), _mm_setzero_si128());	// A5D2H6
	outflank_d = OUTFLANK_3[(_mm_extract_epi16(index_d, 4) >> 1) & 0x3f] & rotl8(_mm_cvtsi128_si32(index_d), 3);

	flipped = _mm_castps_si128(_mm_loadh_pi(_mm_castsi128_ps(flipped), (__m64 *) &FLIPPED_3_H[outflank_d]));

	outflank_h = OUTFLANK_3[(_mm_extract_epi16(OP, 4) >> 9) & 0x3f] & rotl8(_mm_cvtsi128_si64(OP) >> 8, 3);
	flipped = _mm_insert_epi16(flipped, FLIPPED_3_H[outflank_h], 4);
	flipped = _mm_and_si128(flipped, mask);

	return _mm_or_si128(flipped, _mm_shuffle_epi32(flipped, SWAP64));
}

/**
 * Compute flipped discs when playing on square E2.
 *
 * @param P player's disc pattern.
 * @param O opponent's disc pattern.
 * @return flipped disc pattern.
 */
static __m128i vectorcall flip_E2(const __m128i OP)
{
	__m128i	outflank_v, flipped, index_d;
	unsigned int outflank_h, outflank_d;
	const __m128i mask = _mm_set_epi64x(0x000001824428ff00, 0x1010101010100000);

	outflank_v = _mm_andnot_si128(_mm_shuffle_epi32(OP, DUPHI), mask);
	outflank_v = _mm_and_si128(_mm_andnot_si128(_mm_add_epi64(outflank_v, minusone), outflank_v), OP);
	outflank_v = _mm_add_epi64(outflank_v, minusone);
	flipped = _mm_add_epi64(outflank_v, _mm_srli_epi64(outflank_v, 63));

	index_d = _mm_sad_epu8(_mm_and_si128(OP, _mm_set1_epi64x(0x0000018244281000)), _mm_setzero_si128());	// A6E2H5
	outflank_d = OUTFLANK_4[(_mm_extract_epi16(index_d, 4) >> 1) & 0x3f] & rotl8(_mm_cvtsi128_si32(index_d), 2);

	flipped = _mm_castps_si128(_mm_loadh_pi(_mm_castsi128_ps(flipped), (__m64 *) &FLIPPED_4_H[outflank_d]));

	outflank_h = OUTFLANK_4[(_mm_extract_epi16(OP, 4) >> 9) & 0x3f] & rotl8(_mm_cvtsi128_si64(OP) >> 8, 2);
	flipped = _mm_insert_epi16(flipped, FLIPPED_4_H[outflank_h], 4);
	flipped = _mm_and_si128(flipped, mask);

	return _mm_or_si128(flipped, _mm_shuffle_epi32(flipped, SWAP64));
}

/**
 * Compute flipped discs when playing on square F2.
 *
 * @param P player's disc pattern.
 * @param O opponent's disc pattern.
 * @return flipped disc pattern.
 */
static __m128i vectorcall flip_F2(const __m128i OP)
{
	__m128i PP = _mm_shuffle_epi32(OP, DUPLO);
	__m128i OO = _mm_shuffle_epi32(OP, DUPHI);
	__m128i	outflank_vd, flipped, outflank_h, flipped_h_g2g3;
	const __m128i mask = _mm_set_epi64x(0x0001020408100000, 0x2020202020200000);

	outflank_h = _mm_and_si128(MS1B_epu31(_mm_andnot_si128(OO, _mm_set_epi32(0, 0, 0, 0x00001f00))), PP);

	flipped_h_g2g3 = _mm_unpacklo_epi64(outflank_h, _mm_srli_epi64(PP, 9));
	flipped_h_g2g3 = _mm_mullo_epi16(flipped_h_g2g3, _mm_set_epi16(0, 0, 1, 0x0100, 0, 0, 0, -2));
	flipped_h_g2g3 = _mm_and_si128(_mm_and_si128(flipped_h_g2g3, OO), _mm_set_epi16(0, 0, 0x0040, 0x4000, 0, 0, 0, 0x1f00));

	outflank_vd = _mm_andnot_si128(OO, mask);
	outflank_vd = _mm_and_si128(_mm_andnot_si128(_mm_add_epi64(outflank_vd, minusone), outflank_vd), PP);
	outflank_vd = _mm_add_epi64(outflank_vd, minusone);
	flipped = _mm_or_si128(flipped_h_g2g3, _mm_and_si128(mask, _mm_add_epi64(outflank_vd, _mm_srli_epi64(outflank_vd, 63))));

	return _mm_or_si128(flipped, _mm_shuffle_epi32(flipped, SWAP64));
}

/**
 * Compute flipped discs when playing on square G2.
 *
 * @param P player's disc pattern.
 * @param O opponent's disc pattern.
 * @return flipped disc pattern.
 */
static __m128i vectorcall flip_G2(const __m128i OP)
{
	__m128i PP = _mm_shuffle_epi32(OP, DUPLO);
	__m128i OO = _mm_shuffle_epi32(OP, DUPHI);
	__m128i	outflank_vd, outflank_h, flipped;

	outflank_vd = _mm_andnot_si128(OO, _mm_set_epi64x(0x0102040810200000, 0x4040404040400000));
	outflank_vd = _mm_and_si128(_mm_and_si128(outflank_vd, _mm_sub_epi64(_mm_setzero_si128(), outflank_vd)), PP);
	flipped = _mm_sub_epi64(outflank_vd, _mm_andnot_si128(flipmask(outflank_vd), _mm_set1_epi64x(0x0000000000010000)));

	outflank_h = _mm_and_si128(MS1B_epu31(_mm_andnot_si128(OO, _mm_set_epi32(0, 0, 0, 0x00003f00))), PP);
	flipped = _mm_sub_epi8(flipped, _mm_add_epi8(outflank_h, outflank_h));
	flipped = _mm_and_si128(flipped, _mm_set_epi64x(0x0102040810200000, 0x4040404040403e00));

	return _mm_or_si128(flipped, _mm_shuffle_epi32(flipped, SWAP64));
}

/**
 * Compute flipped discs when playing on square H2.
 *
 * @param P player's disc pattern.
 * @param O opponent's disc pattern.
 * @return flipped disc pattern.
 */
static __m128i vectorcall flip_H2(const __m128i OP)
{
	__m128i PP = _mm_shuffle_epi32(OP, DUPLO);
	__m128i OO = _mm_shuffle_epi32(OP, DUPHI);
	__m128i	outflank_vd, outflank_h, flipped;

	outflank_vd = _mm_andnot_si128(OO, _mm_set_epi64x(0x0204081020400000, 0x8080808080800000));
	outflank_vd = _mm_and_si128(_mm_and_si128(outflank_vd, _mm_sub_epi64(_mm_setzero_si128(), outflank_vd)), PP);
	flipped = _mm_sub_epi64(outflank_vd, _mm_andnot_si128(flipmask(outflank_vd), _mm_set1_epi64x(0x0000000000010000)));

	outflank_h = _mm_and_si128(MS1B_epu31(_mm_andnot_si128(OO, _mm_set_epi32(0, 0, 0, 0x00007f00))), PP);
	flipped = _mm_sub_epi8(flipped, _mm_add_epi8(outflank_h, outflank_h));
	flipped = _mm_and_si128(flipped, _mm_set_epi64x(0x0204081020400000, 0x8080808080807e00));

	return _mm_or_si128(flipped, _mm_shuffle_epi32(flipped, SWAP64));
}

/**
 * Compute flipped discs when playing on square A3.
 *
 * @param P player's disc pattern.
 * @param O opponent's disc pattern.
 * @return flipped disc pattern.
 */
static __m128i vectorcall flip_A3(const __m128i OP)
{
	__m128i PP = _mm_shuffle_epi32(OP, DUPLO);
	__m128i OO = _mm_shuffle_epi32(OP, DUPHI);
	__m128i	outflank_vd, outflank_h, flipped, flipped_h_a2b2;
	const __m128i mask = _mm_set_epi64x(0x2010080402000000, 0x0101010101000000);

	outflank_vd = _mm_andnot_si128(OO, mask);
	outflank_vd = _mm_and_si128(_mm_andnot_si128(_mm_add_epi64(outflank_vd, minusone), outflank_vd), PP);
	outflank_vd = _mm_add_epi64(outflank_vd, minusone);
	flipped = _mm_and_si128(mask, _mm_add_epi64(outflank_vd, _mm_srli_epi64(outflank_vd, 63)));

	outflank_h = _mm_and_si128(PP, _mm_adds_epu8(OO, _mm_set_epi8(0, 0, 0, 0, 0, 0, 0, -1, 0, 0, 0, 0, 0, 2, 0, -1)));
	flipped_h_a2b2 = _mm_srli_epi16(_mm_mullo_epi16(outflank_h, _mm_set_epi16(0, 0, 0, 0x2000, 0, 0, 0x003f, 0x4000)), 6);
	flipped_h_a2b2 = _mm_and_si128(flipped_h_a2b2, _mm_set_epi64x(0x0000000000000200, 0x00000000007e0100));
	flipped = _mm_or_si128(flipped, _mm_and_si128(flipped_h_a2b2, OO));

	return _mm_or_si128(flipped, _mm_shuffle_epi32(flipped, SWAP64));
}

/**
 * Compute flipped discs when playing on square B3.
 *
 * @param P player's disc pattern.
 * @param O opponent's disc pattern.
 * @return flipped disc pattern.
 */
static __m128i vectorcall flip_B3(const __m128i OP)
{
	__m128i PP = _mm_shuffle_epi32(OP, DUPLO);
	__m128i OO = _mm_shuffle_epi32(OP, DUPHI);
	__m128i	outflank_vd, outflank_h, flipped, flipped_h_b2c2;
	const __m128i mask = _mm_set_epi64x(0x4020100804000000, 0x0202020202000000);

	outflank_vd = _mm_andnot_si128(OO, mask);
	outflank_vd = _mm_and_si128(_mm_andnot_si128(_mm_add_epi64(outflank_vd, minusone), outflank_vd), PP);
	outflank_vd = _mm_add_epi64(outflank_vd, minusone);
	flipped = _mm_and_si128(mask, _mm_add_epi64(outflank_vd, _mm_srli_epi64(outflank_vd, 63)));

	outflank_h = _mm_and_si128(PP, _mm_adds_epu8(OO, _mm_set_epi8(0, 0, 0, 0, 0, 0, 0, -1, 0, 0, 0, 0, 0, 4, 0, -1)));
	flipped_h_b2c2 = _mm_srli_epi16(_mm_mullo_epi16(outflank_h, _mm_set_epi16(0, 0, 0, 0x1000, 0, 0, 0x001f, 0x2000)), 5);
	flipped_h_b2c2 = _mm_and_si128(flipped_h_b2c2, _mm_set_epi64x(0x0000000000000400, 0x00000000007c0200));
	flipped = _mm_or_si128(flipped, _mm_and_si128(flipped_h_b2c2, OO));

	return _mm_or_si128(flipped, _mm_shuffle_epi32(flipped, SWAP64));
}

/**
 * Compute flipped discs when playing on square C3.
 *
 * @param P player's disc pattern.
 * @param O opponent's disc pattern.
 * @return flipped disc pattern.
 */
static __m128i vectorcall flip_C3(const __m128i OP)
{
	__m128i PP = _mm_shuffle_epi32(OP, DUPLO);
	__m128i OO = _mm_shuffle_epi32(OP, DUPHI);
	__m128i	outflank_vd, outflank_h, flipped, flipped_b4b3b2c2d2;
	const __m128i mask = _mm_set_epi64x(~0x8040201008000000ULL, ~0x0404040404000000ULL);
	const __m128i next_h = _mm_loadl_epi64((__m128i *) &X_TO_BIT[19]);

	outflank_vd = _mm_and_si128(_mm_andnot_si128(mask, _mm_sub_epi64(_mm_or_si128(OO, mask), minusone)), PP);
	outflank_vd = _mm_add_epi64(outflank_vd, minusone);
	flipped = _mm_andnot_si128(mask, _mm_add_epi64(outflank_vd, _mm_srli_epi64(outflank_vd, 63)));

	outflank_h = _mm_and_si128(_mm_add_epi8(OO, next_h), PP);
	flipped = _mm_or_si128(flipped, _mm_subs_epu8(outflank_h, next_h));

	flipped_b4b3b2c2d2 = _mm_and_si128(_mm_shufflelo_epi16(PP, 0x90), _mm_set_epi16(0, 0, 0, 0x0001, 0x0001, 0x0001, 0x0004, 0x0010));	// ...a1a5a3c1e1
	flipped_b4b3b2c2d2 = _mm_madd_epi16(flipped_b4b3b2c2d2, _mm_set_epi16(0, 0, 0, 0x0200, 0x0200, 0x0002, 0x0100, 0x0080));
	flipped = _mm_or_si128(flipped, _mm_and_si128(_mm_shufflelo_epi16(flipped_b4b3b2c2d2, 0xf8), OO));

	return _mm_or_si128(flipped, _mm_shuffle_epi32(flipped, SWAP64));
}

/**
 * Compute flipped discs when playing on square D3.
 *
 * @param P player's disc pattern.
 * @param O opponent's disc pattern.
 * @return flipped disc pattern.
 */
static __m128i vectorcall flip_D3(const __m128i OP)
{
	__m128i	flipped, flipped_h_c2e2, index_d;
	unsigned int outflank_h, outflank_v, outflank_d, index_v;
	const __m128i mask = _mm_set_epi64x(0x0080412214080000, 0x0808080808080808);	// A6D3H7

	index_v = _mm_movemask_epi8(_mm_slli_epi64(OP, 4));
	outflank_v = OUTFLANK_2[(index_v >> 9) & 0x3f] & rotl8(index_v, 4);

	index_d = _mm_sad_epu8(_mm_and_si128(OP, _mm_shuffle_epi32(mask, DUPHI)), _mm_setzero_si128());
	outflank_d = OUTFLANK_3[(_mm_extract_epi16(index_d, 4) >> 1) & 0x3f] & rotl8(_mm_cvtsi128_si32(index_d), 3);

	flipped = _mm_and_si128(load64x2(&FLIPPED_2_V[outflank_v], &FLIPPED_3_H[outflank_d]), mask);

	outflank_h = OUTFLANK_3[(_mm_extract_epi16(OP, 5) >> 1) & 0x3f] & rotl8(_mm_extract_epi16(OP, 1), 3);

	flipped_h_c2e2 = _mm_unpacklo_epi64(_mm_slli_epi64(OP, 9), _mm_slli_epi64(OP, 7));
	flipped_h_c2e2 = _mm_and_si128(flipped_h_c2e2, _mm_shuffle_epi32(OP, DUPHI));
	flipped_h_c2e2 = _mm_insert_epi16(flipped_h_c2e2, FLIPPED_3_H[outflank_h], 1);
	flipped_h_c2e2 = _mm_and_si128(flipped_h_c2e2, _mm_set_epi64x(0x0000000000001000, 0x0000000000ff0400));
	flipped = _mm_or_si128(flipped, flipped_h_c2e2);

	return _mm_or_si128(flipped, _mm_shuffle_epi32(flipped, SWAP64));
}

/**
 * Compute flipped discs when playing on square E3.
 *
 * @param P player's disc pattern.
 * @param O opponent's disc pattern.
 * @return flipped disc pattern.
 */
static __m128i vectorcall flip_E3(const __m128i OP)
{
	__m128i	flipped, flipped_h_d2f2, index_d;
	unsigned int outflank_h, outflank_v, outflank_d, index_v;
	const __m128i mask = _mm_set_epi64x(0x0001824428100000, 0x1010101010101010);	// A7E3H6

	index_v = _mm_movemask_epi8(_mm_slli_epi64(OP, 3));
	outflank_v = OUTFLANK_2[(index_v >> 9) & 0x3f] & rotl8(index_v, 4);

	index_d = _mm_sad_epu8(_mm_and_si128(OP, _mm_shuffle_epi32(mask, DUPHI)), _mm_setzero_si128());
	outflank_d = OUTFLANK_4[(_mm_extract_epi16(index_d, 4) >> 1) & 0x3f] & rotl8(_mm_cvtsi128_si32(index_d), 2);

	flipped = _mm_and_si128(load64x2(&FLIPPED_2_V[outflank_v], &FLIPPED_4_H[outflank_d]), mask);

	outflank_h = OUTFLANK_4[(_mm_extract_epi16(OP, 5) >> 1) & 0x3f] & rotl8(_mm_extract_epi16(OP, 1), 2);

	flipped_h_d2f2 = _mm_unpacklo_epi64(_mm_slli_epi64(OP, 9), _mm_slli_epi64(OP, 7));
	flipped_h_d2f2 = _mm_and_si128(flipped_h_d2f2, _mm_shuffle_epi32(OP, DUPHI));
	flipped_h_d2f2 = _mm_insert_epi16(flipped_h_d2f2, FLIPPED_4_H[outflank_h], 1);
	flipped_h_d2f2 = _mm_and_si128(flipped_h_d2f2, _mm_set_epi64x(0x0000000000002000, 0x0000000000ff0800));
	flipped = _mm_or_si128(flipped, flipped_h_d2f2);

	return _mm_or_si128(flipped, _mm_shuffle_epi32(flipped, SWAP64));
}

/**
 * Compute flipped discs when playing on square F3.
 *
 * @param P player's disc pattern.
 * @param O opponent's disc pattern.
 * @return flipped disc pattern.
 */
static __m128i vectorcall flip_F3(const __m128i OP)
{
	__m128i PP = _mm_shuffle_epi32(OP, DUPLO);
	__m128i OO = _mm_shuffle_epi32(OP, DUPHI);
	__m128i	outflank_h, outflank_vd, flipped, flipped_vd, flipped_g4g3g2f2e2;
	const __m128i mask = _mm_set_epi64x(~0x0102040810000000ULL, ~0x2020202020000000ULL);

	outflank_h = _mm_and_si128(MS1B_epu31(_mm_andnot_si128(OO, _mm_set_epi32(0, 0, 0, 0x001f0000))), PP);
	flipped = _mm_srli_epi16(_mm_mullo_epi16(outflank_h, _mm_set_epi16(0, 0, 0, 0, 0, 0, -0x1000, 0)), 11);

	outflank_vd = _mm_and_si128(_mm_andnot_si128(mask, _mm_sub_epi64(_mm_or_si128(OO, mask), minusone)), PP);
	outflank_vd = _mm_add_epi64(outflank_vd, minusone);
	flipped_vd = _mm_andnot_si128(mask, _mm_add_epi64(outflank_vd, _mm_srli_epi64(outflank_vd, 63)));
	flipped = _mm_or_si128(flipped, flipped_vd);

	flipped_g4g3g2f2e2 = _mm_and_si128(_mm_shufflelo_epi16(PP, 0x90), _mm_set_epi16(0, 0, 0, 0x0080, 0x0080, 0x0080, 0x0020, 0x0008));	// ...h1h5h3f1d1
	flipped_g4g3g2f2e2 = _mm_srli_epi16(_mm_madd_epi16(flipped_g4g3g2f2e2, _mm_set_epi16(0, 0, 0, 0x0100, 0x0100, 0x0001, 0x0200, 0x0400)), 1);
	flipped = _mm_or_si128(flipped, _mm_and_si128(_mm_shufflelo_epi16(flipped_g4g3g2f2e2, 0xf8), OO));

	return _mm_or_si128(flipped, _mm_shuffle_epi32(flipped, SWAP64));
}

/**
 * Compute flipped discs when playing on square G3.
 *
 * @param P player's disc pattern.
 * @param O opponent's disc pattern.
 * @return flipped disc pattern.
 */
static __m128i vectorcall flip_G3(const __m128i OP)
{
	__m128i PP = _mm_shuffle_epi32(OP, DUPLO);
	__m128i OO = _mm_shuffle_epi32(OP, DUPHI);
	__m128i	outflank_vd, outflank_h, flipped, flipped_g2f2;
	const __m128i mask = _mm_set_epi64x(~0x0204081020000000, ~0x4040404040000000);

	outflank_vd = _mm_and_si128(_mm_andnot_si128(mask, _mm_sub_epi64(_mm_or_si128(OO, mask), minusone)), PP);
	flipped = _mm_sub_epi64(outflank_vd, _mm_andnot_si128(flipmask(outflank_vd), _mm_set1_epi64x(0x0000000001000000)));

	outflank_h = _mm_and_si128(MS1B_epu31(_mm_andnot_si128(OO, _mm_set_epi32(0, 0, 0, 0x003f0000))), PP);
	flipped = _mm_sub_epi8(flipped, _mm_add_epi8(outflank_h, outflank_h));

	flipped_g2f2 = _mm_and_si128(_mm_mullo_epi16(PP, _mm_set_epi16(0, 0, 0, 0x0200, 0, 0, 0, 0x0100)), OO);
	flipped = _mm_and_si128(_mm_or_si128(flipped, flipped_g2f2), _mm_set_epi64x(0x0204081020002000, 0x40404040403f4000));

	return _mm_or_si128(flipped, _mm_shuffle_epi32(flipped, SWAP64));
}

/**
 * Compute flipped discs when playing on square H3.
 *
 * @param P player's disc pattern.
 * @param O opponent's disc pattern.
 * @return flipped disc pattern.
 */
static __m128i vectorcall flip_H3(const __m128i OP)
{
	__m128i PP = _mm_shuffle_epi32(OP, DUPLO);
	__m128i OO = _mm_shuffle_epi32(OP, DUPHI);
	__m128i	outflank_vd, outflank_h, flipped, flipped_h2g2;
	const __m128i mask = _mm_set_epi64x(~0x0408102040000000, ~0x8080808080000000);

	outflank_vd = _mm_and_si128(_mm_andnot_si128(mask, _mm_sub_epi64(_mm_or_si128(OO, mask), minusone)), PP);
	flipped = _mm_sub_epi64(outflank_vd, _mm_andnot_si128(flipmask(outflank_vd), _mm_set1_epi64x(0x0000000001000000)));

	outflank_h = _mm_and_si128(MS1B_epu31(_mm_andnot_si128(OO, _mm_set_epi32(0, 0, 0, 0x007f0000))), PP);
	flipped = _mm_sub_epi8(flipped, _mm_add_epi8(outflank_h, outflank_h));

	flipped_h2g2 = _mm_and_si128(_mm_mullo_epi16(PP, _mm_set_epi16(0, 0, 0, 0x0200, 0, 0, 0, 0x0100)), OO);
	flipped = _mm_and_si128(_mm_or_si128(flipped, flipped_h2g2), _mm_set_epi64x(0x0408102040004000, 0x80808080807f8000));

	return _mm_or_si128(flipped, _mm_shuffle_epi32(flipped, SWAP64));
}

/**
 * Compute flipped discs when playing on square A4.
 *
 * @param P player's disc pattern.
 * @param O opponent's disc pattern.
 * @return flipped disc pattern.
 */
static __m128i vectorcall flip_A4(const __m128i OP)
{
	__m128i PP = _mm_shuffle_epi32(OP, DUPLO);
	__m128i OO = _mm_shuffle_epi32(OP, DUPHI);
	__m128i	outflankL, outflankH, flipped;
	const __m128i maskL = _mm_set_epi32(0, 0x00020408, 0, 0x00010101);
	const __m128i maskH = _mm_set_epi32(0x10080402, 0, 0x01010101, 0xfe000000);

	outflankL = _mm_and_si128(MS1B_epu31(_mm_andnot_si128(OO, maskL)), PP);
	flipped = _mm_and_si128(maskL, _mm_mul_epu32(outflankL, _mm_set1_epi32(-2)));

	outflankH = _mm_andnot_si128(OO, maskH);
	outflankH = _mm_and_si128(_mm_andnot_si128(_mm_add_epi32(outflankH, minusone), outflankH), PP);
	outflankH = _mm_add_epi32(outflankH, minusone);
	flipped = _mm_or_si128(flipped, _mm_and_si128(maskH, _mm_add_epi32(outflankH, _mm_srli_epi32(outflankH, 31))));

	return _mm_or_si128(flipped, _mm_shuffle_epi32(flipped, SWAP64));
}

/**
 * Compute flipped discs when playing on square B4.
 *
 * @param P player's disc pattern.
 * @param O opponent's disc pattern.
 * @return flipped disc pattern.
 */
static __m128i vectorcall flip_B4(const __m128i OP)
{
	__m128i PP = _mm_shuffle_epi32(OP, DUPLO);
	__m128i OO = _mm_shuffle_epi32(OP, DUPHI);
	__m128i	outflankL, outflankH, flipped;
	const __m128i maskL = _mm_set_epi32(0, 0x00040810, 0, 0x00020202);
	const __m128i maskH = _mm_set_epi32(0x20100804, 0, 0x02020202, 0xfc000000);

	outflankL = _mm_and_si128(MS1B_epu31(_mm_andnot_si128(OO, maskL)), PP);
	flipped = _mm_and_si128(maskL, _mm_mul_epu32(outflankL, _mm_set1_epi32(-2)));

	outflankH = _mm_andnot_si128(OO, maskH);
	outflankH = _mm_and_si128(_mm_andnot_si128(_mm_add_epi32(outflankH, minusone), outflankH), PP);
	outflankH = _mm_add_epi32(outflankH, minusone);
	flipped = _mm_or_si128(flipped, _mm_and_si128(maskH, _mm_add_epi32(outflankH, _mm_srli_epi32(outflankH, 31))));

	return _mm_or_si128(flipped, _mm_shuffle_epi32(flipped, SWAP64));
}

/**
 * Compute flipped discs when playing on square C4.
 *
 * @param P player's disc pattern.
 * @param O opponent's disc pattern.
 * @return flipped disc pattern.
 */
static __m128i vectorcall flip_C4(const __m128i OP)
{
	__m128i OL, OH, PL, PH, outflankL, outflankH, flippedL, flippedH, flipped;
	const __m128i maskL = _mm_set_epi32(0x00081020, 0x00020100, 0x00040404, 0x03000000);
	const __m128i maskH = _mm_set_epi32(0x00000102, 0x40201008, 0x04040404, 0xf8000000);

	OH = _mm_shuffle_epi32(OP, 0xfe);
	PH = _mm_shuffle_epi32(OP, 0x54);
	outflankH = _mm_andnot_si128(OH, maskH);
	outflankH = _mm_and_si128(_mm_andnot_si128(_mm_add_epi32(outflankH, minusone), outflankH), PH);
	outflankH = _mm_add_epi32(outflankH, minusone);
	flippedH = _mm_and_si128(maskH, _mm_add_epi32(outflankH, _mm_srli_epi32(outflankH, 31)));
	flipped = _mm_or_si128(_mm_move_epi64(flippedH), _mm_shuffle_epi32(flippedH, 0xc8));

	OL = _mm_shuffle_epi32(OP, 0xaa);
	PL = _mm_shuffle_epi32(OP, 0x00);
	outflankL = _mm_and_si128(MS1B_epu31(_mm_andnot_si128(OL, maskL)), PL);
	flippedL = _mm_andnot_si128(_mm_add_epi32(_mm_add_epi32(outflankL, outflankL), minusone), maskL);
	flipped = _mm_or_si128(flipped, _mm_xor_si128(flippedL, _mm_shuffle_epi32(flippedL, 0xf5)));

	return _mm_or_si128(flipped, _mm_shuffle_epi32(flipped, SWAP64));
}

/**
 * Compute flipped discs when playing on square D4.
 *
 * @param P player's disc pattern.
 * @param O opponent's disc pattern.
 * @return flipped disc pattern.
 */
static __m128i vectorcall flip_D4(const __m128i OP)
{
	__m128i OL, OH, PL, PH, outflankL, outflankH, flippedL, flippedH, flipped;
	const __m128i maskL = _mm_set_epi32(0x00102040, 0x00040201, 0x00080808, 0x07000000);
	const __m128i maskH = _mm_set_epi32(0x00010204, 0x80402010, 0x08080808, 0xf0000000);

	OH = _mm_shuffle_epi32(OP, 0xfe);
	PH = _mm_shuffle_epi32(OP, 0x54);
	outflankH = _mm_andnot_si128(OH, maskH);
	outflankH = _mm_and_si128(_mm_andnot_si128(_mm_add_epi32(outflankH, minusone), outflankH), PH);
	outflankH = _mm_add_epi32(outflankH, minusone);
	flippedH = _mm_and_si128(maskH, _mm_add_epi32(outflankH, _mm_srli_epi32(outflankH, 31)));
	flipped = _mm_or_si128(_mm_move_epi64(flippedH), _mm_shuffle_epi32(flippedH, 0xc8));

	OL = _mm_shuffle_epi32(OP, 0xaa);
	PL = _mm_shuffle_epi32(OP, 0x00);
	outflankL = _mm_and_si128(MS1B_epu31(_mm_andnot_si128(OL, maskL)), PL);
	flippedL = _mm_andnot_si128(_mm_add_epi32(_mm_add_epi32(outflankL, outflankL), minusone), maskL);
	flipped = _mm_or_si128(flipped, _mm_xor_si128(flippedL, _mm_shuffle_epi32(flippedL, 0xf5)));

	return _mm_or_si128(flipped, _mm_shuffle_epi32(flipped, SWAP64));
}

/**
 * Compute flipped discs when playing on square E4.
 *
 * @param P player's disc pattern.
 * @param O opponent's disc pattern.
 * @return flipped disc pattern.
 */
static __m128i vectorcall flip_E4(const __m128i OP)
{
	__m128i OL, OH, PL, PH, outflankL, outflankH, flippedL, flippedH, flipped;
	const __m128i maskL = _mm_set_epi32(0x00204080, 0x00080402, 0x00101010, 0x0f000000);
	const __m128i maskH = _mm_set_epi32(0x01020408, 0x00804020, 0x10101010, 0xe0000000);

	OH = _mm_shuffle_epi32(OP, 0xfe);
	PH = _mm_shuffle_epi32(OP, 0x54);
	outflankH = _mm_andnot_si128(OH, maskH);
	outflankH = _mm_and_si128(_mm_andnot_si128(_mm_add_epi32(outflankH, minusone), outflankH), PH);
	outflankH = _mm_add_epi32(outflankH, minusone);
	flippedH = _mm_and_si128(maskH, _mm_add_epi32(outflankH, _mm_srli_epi32(outflankH, 31)));
	flipped = _mm_or_si128(_mm_move_epi64(flippedH), _mm_shuffle_epi32(flippedH, 0xc8));

	OL = _mm_shuffle_epi32(OP, 0xaa);
	PL = _mm_shuffle_epi32(OP, 0x00);
	outflankL = _mm_and_si128(MS1B_epu31(_mm_andnot_si128(OL, maskL)), PL);
	flippedL = _mm_andnot_si128(_mm_add_epi32(_mm_add_epi32(outflankL, outflankL), minusone), maskL);
	flipped = _mm_or_si128(flipped, _mm_xor_si128(flippedL, _mm_shuffle_epi32(flippedL, 0xf5)));

	return _mm_or_si128(flipped, _mm_shuffle_epi32(flipped, SWAP64));
}

/**
 * Compute flipped discs when playing on square F4.
 *
 * @param P player's disc pattern.
 * @param O opponent's disc pattern.
 * @return flipped disc pattern.
 */
static __m128i vectorcall flip_F4(const __m128i OP)
{
	__m128i OL, OH, PL, PH, outflankL, outflankH, flippedL, flippedH, flipped;
	const __m128i maskL = _mm_set_epi32(0x00408000, 0x00100804, 0x00202020, 0x1f000000);
	const __m128i maskH = _mm_set_epi32(0x02040810, 0x00008040, 0x20202020, 0xc0000000);

	OH = _mm_shuffle_epi32(OP, 0xfe);
	PH = _mm_shuffle_epi32(OP, 0x54);
	outflankH = _mm_andnot_si128(OH, maskH);
	outflankH = _mm_and_si128(_mm_andnot_si128(_mm_add_epi32(outflankH, minusone), outflankH), PH);
	outflankH = _mm_add_epi32(outflankH, minusone);
	flippedH = _mm_and_si128(maskH, _mm_add_epi32(outflankH, _mm_srli_epi32(outflankH, 31)));
	flipped = _mm_or_si128(_mm_move_epi64(flippedH), _mm_shuffle_epi32(flippedH, 0xc8));

	OL = _mm_shuffle_epi32(OP, 0xaa);
	PL = _mm_shuffle_epi32(OP, 0x00);
	outflankL = _mm_and_si128(MS1B_epu31(_mm_andnot_si128(OL, maskL)), PL);
	flippedL = _mm_andnot_si128(_mm_add_epi32(_mm_add_epi32(outflankL, outflankL), minusone), maskL);
	flipped = _mm_or_si128(flipped, _mm_xor_si128(flippedL, _mm_shuffle_epi32(flippedL, 0xf5)));

	return _mm_or_si128(flipped, _mm_shuffle_epi32(flipped, SWAP64));
}

/**
 * Compute flipped discs when playing on square G4.
 *
 * @param P player's disc pattern.
 * @param O opponent's disc pattern.
 * @return flipped disc pattern.
 */
static __m128i vectorcall flip_G4(const __m128i OP)
{
	__m128i	OO, PP, OL, PL, outflankH, outflankL, flipped, flippedL;
	const __m128i maskL = _mm_set_epi32(0, 0x3f000000, 0x00201008, 0x00404040);
	const __m128i maskH = _mm_set_epi64x(~0x0408102000000000ULL, ~0x4040404000000000ULL);

	OL = _mm_shuffle_epi32(OP, 0xaa);
	PL = _mm_shuffle_epi32(OP, 0x00);
	outflankL = _mm_and_si128(MS1B_epu31(_mm_andnot_si128(OL, maskL)), PL);
	flippedL = _mm_and_si128(_mm_sub_epi32(_mm_setzero_si128(), _mm_add_epi32(outflankL, outflankL)), maskL);
	flipped = _mm_xor_si128(flippedL, _mm_shuffle_epi32(flippedL, 0xf5));

	OO = _mm_shuffle_epi32(OP, DUPHI);
	PP = _mm_shuffle_epi32(OP, DUPLO);
	outflankH = _mm_and_si128(_mm_andnot_si128(maskH, _mm_sub_epi64(_mm_or_si128(OO, maskH), minusone)), PP);
	flipped = _mm_or_si128(flipped, _mm_andnot_si128(maskH, _mm_sub_epi64(outflankH, _mm_shuffle_epi32(outflankH, SWAP32))));

	return _mm_or_si128(flipped, _mm_shuffle_epi32(flipped, SWAP64));
}

/**
 * Compute flipped discs when playing on square H4.
 *
 * @param P player's disc pattern.
 * @param O opponent's disc pattern.
 * @return flipped disc pattern.
 */
static __m128i vectorcall flip_H4(const __m128i OP)
{
	__m128i	OO, PP, OL, PL, outflankH, outflankL, flipped, flippedL;
	const __m128i maskL = _mm_set_epi32(0, 0x7f000000, 0x00402010, 0x00808080);
	const __m128i maskH = _mm_set_epi64x(~0x0810204000000000ULL, ~0x8080808000000000ULL);

	OL = _mm_shuffle_epi32(OP, 0xaa);
	PL = _mm_shuffle_epi32(OP, 0x00);
	outflankL = _mm_and_si128(MS1B_epu31(_mm_andnot_si128(OL, maskL)), PL);
	flippedL = _mm_and_si128(_mm_sub_epi32(_mm_setzero_si128(), _mm_add_epi32(outflankL, outflankL)), maskL);
	flipped = _mm_xor_si128(flippedL, _mm_shuffle_epi32(flippedL, 0xf5));

	OO = _mm_shuffle_epi32(OP, DUPHI);
	PP = _mm_shuffle_epi32(OP, DUPLO);
	outflankH = _mm_and_si128(_mm_andnot_si128(maskH, _mm_sub_epi64(_mm_or_si128(OO, maskH), minusone)), PP);
	flipped = _mm_or_si128(flipped, _mm_andnot_si128(maskH, _mm_sub_epi64(outflankH, _mm_shuffle_epi32(outflankH, SWAP32))));

	return _mm_or_si128(flipped, _mm_shuffle_epi32(flipped, SWAP64));
}

/**
 * Compute flipped discs when playing on square A5.
 *
 * @param P player's disc pattern.
 * @param O opponent's disc pattern.
 * @return flipped disc pattern.
 */
static __m128i vectorcall flip_A5(const __m128i OP)
{
	__m128i	OO, PP, OH, PH, outflankL, outflankH, flipped, flippedH;
	const __m128i maskL = _mm_set_epi32(0, 0x02040810, 0, 0x01010101);
	const __m128i maskH = _mm_set_epi32(0x08040200, 0, 0x01010100, 0x000000fe);

	OH = _mm_shuffle_epi32(OP, 0xef);
	PH = _mm_shuffle_epi32(OP, 0x45);
	outflankH = _mm_andnot_si128(OH, maskH);
	outflankH = _mm_and_si128(_mm_and_si128(outflankH, _mm_sub_epi32(_mm_setzero_si128(), outflankH)), PH);
	flippedH = _mm_add_epi32(outflankH, minusone);
	flippedH = _mm_and_si128(maskH, _mm_add_epi32(flippedH, _mm_srli_epi32(flippedH, 31)));
	flipped = _mm_xor_si128(flippedH, _mm_shuffle_epi32(flippedH, 0xa0));

	OO = _mm_shuffle_epi32(OP, DUPHI);
	PP = _mm_shuffle_epi32(OP, DUPLO);
	outflankL = _mm_and_si128(MS1B_epu31(_mm_andnot_si128(OO, maskL)), PP);
	flipped = _mm_or_si128(flipped, _mm_and_si128(maskL, _mm_mul_epu32(outflankL, _mm_set1_epi32(-2))));

	return _mm_or_si128(flipped, _mm_shuffle_epi32(flipped, SWAP64));
}

/**
 * Compute flipped discs when playing on square B5.
 *
 * @param P player's disc pattern.
 * @param O opponent's disc pattern.
 * @return flipped disc pattern.
 */
static __m128i vectorcall flip_B5(const __m128i OP)
{
	__m128i	OO, PP, OH, PH, outflankL, outflankH, flipped, flippedH;
	const __m128i maskL = _mm_set_epi32(0, 0x04081020, 0, 0x02020202);
	const __m128i maskH = _mm_set_epi32(0x10080400, 0, 0x02020200, 0x000000fc);

	OH = _mm_shuffle_epi32(OP, 0xef);
	PH = _mm_shuffle_epi32(OP, 0x45);
	outflankH = _mm_andnot_si128(OH, maskH);
	outflankH = _mm_and_si128(_mm_and_si128(outflankH, _mm_sub_epi32(_mm_setzero_si128(), outflankH)), PH);
	flippedH = _mm_add_epi32(outflankH, minusone);
	flippedH = _mm_and_si128(maskH, _mm_add_epi32(flippedH, _mm_srli_epi32(flippedH, 31)));
	flipped = _mm_xor_si128(flippedH, _mm_shuffle_epi32(flippedH, 0xa0));

	OO = _mm_shuffle_epi32(OP, DUPHI);
	PP = _mm_shuffle_epi32(OP, DUPLO);
	outflankL = _mm_and_si128(MS1B_epu31(_mm_andnot_si128(OO, maskL)), PP);
	flipped = _mm_or_si128(flipped, _mm_and_si128(maskL, _mm_mul_epu32(outflankL, _mm_set1_epi32(-2))));

	return _mm_or_si128(flipped, _mm_shuffle_epi32(flipped, SWAP64));
}

/**
 * Compute flipped discs when playing on square C5.
 *
 * @param P player's disc pattern.
 * @param O opponent's disc pattern.
 * @return flipped disc pattern.
 */
static __m128i vectorcall flip_C5(const __m128i OP)
{
	__m128i OL, OH, PL, PH, outflankL, outflankH, flippedL, flippedH, flipped;
	const __m128i maskL = _mm_set_epi32(0x08102040, 0x02010000, 0x00000003, 0x04040404);
	const __m128i maskH = _mm_set_epi32(0x00010200, 0x20100800, 0x000000f8, 0x04040400);

	OL = _mm_shuffle_epi32(OP, 0xae);
	PL = _mm_shuffle_epi32(OP, 0x04);
	outflankL = _mm_and_si128(MS1B_epu31(_mm_andnot_si128(OL, maskL)), PL);
	flippedL = _mm_andnot_si128(_mm_add_epi32(_mm_add_epi32(outflankL, outflankL), minusone), maskL);
	flipped = _mm_or_si128(_mm_move_epi64(flippedL), _mm_shuffle_epi32(flippedL, 0x76));

	OH = _mm_shuffle_epi32(OP, 0xff);
	PH = _mm_shuffle_epi32(OP, 0x55);
	outflankH = _mm_andnot_si128(OH, maskH);
	outflankH = _mm_and_si128(_mm_andnot_si128(_mm_add_epi32(outflankH, minusone), outflankH), PH);
	outflankH = _mm_add_epi32(outflankH, minusone);
	flippedH = _mm_and_si128(maskH, _mm_add_epi32(outflankH, _mm_srli_epi32(outflankH, 31)));
	flipped = _mm_or_si128(flipped, _mm_xor_si128(flippedH, _mm_shuffle_epi32(flippedH, 0xa0)));

	return _mm_or_si128(flipped, _mm_shuffle_epi32(flipped, SWAP64));
}

/**
 * Compute flipped discs when playing on square D5.
 *
 * @param P player's disc pattern.
 * @param O opponent's disc pattern.
 * @return flipped disc pattern.
 */
static __m128i vectorcall flip_D5(const __m128i OP)
{
	__m128i OL, OH, PL, PH, outflankL, outflankH, flippedL, flippedH, flipped;
	const __m128i maskL = _mm_set_epi32(0x10204080, 0x04020100, 0x00000007, 0x08080808);
	const __m128i maskH = _mm_set_epi32(0x01020400, 0x40201000, 0x000000f0, 0x08080800);

	OL = _mm_shuffle_epi32(OP, 0xae);
	PL = _mm_shuffle_epi32(OP, 0x04);
	outflankL = _mm_and_si128(MS1B_epu31(_mm_andnot_si128(OL, maskL)), PL);
	flippedL = _mm_andnot_si128(_mm_add_epi32(_mm_add_epi32(outflankL, outflankL), minusone), maskL);
	flipped = _mm_or_si128(_mm_move_epi64(flippedL), _mm_shuffle_epi32(flippedL, 0x76));

	OH = _mm_shuffle_epi32(OP, 0xff);
	PH = _mm_shuffle_epi32(OP, 0x55);
	outflankH = _mm_andnot_si128(OH, maskH);
	outflankH = _mm_and_si128(_mm_andnot_si128(_mm_add_epi32(outflankH, minusone), outflankH), PH);
	outflankH = _mm_add_epi32(outflankH, minusone);
	flippedH = _mm_and_si128(maskH, _mm_add_epi32(outflankH, _mm_srli_epi32(outflankH, 31)));
	flipped = _mm_or_si128(flipped, _mm_xor_si128(flippedH, _mm_shuffle_epi32(flippedH, 0xa0)));

	return _mm_or_si128(flipped, _mm_shuffle_epi32(flipped, SWAP64));
}

/**
 * Compute flipped discs when playing on square E5.
 *
 * @param P player's disc pattern.
 * @param O opponent's disc pattern.
 * @return flipped disc pattern.
 */
static __m128i vectorcall flip_E5(const __m128i OP)
{
	__m128i OL, OH, PL, PH, outflankL, outflankH, flippedL, flippedH, flipped;
	const __m128i maskL = _mm_set_epi32(0x20408000, 0x08040201, 0x0000000f, 0x10101010);
	const __m128i maskH = _mm_set_epi32(0x02040800, 0x80402000, 0x000000e0, 0x10101000);

	OL = _mm_shuffle_epi32(OP, 0xae);
	PL = _mm_shuffle_epi32(OP, 0x04);
	outflankL = _mm_and_si128(MS1B_epu31(_mm_andnot_si128(OL, maskL)), PL);
	flippedL = _mm_andnot_si128(_mm_add_epi32(_mm_add_epi32(outflankL, outflankL), minusone), maskL);
	flipped = _mm_or_si128(_mm_move_epi64(flippedL), _mm_shuffle_epi32(flippedL, 0x76));

	OH = _mm_shuffle_epi32(OP, 0xff);
	PH = _mm_shuffle_epi32(OP, 0x55);
	outflankH = _mm_andnot_si128(OH, maskH);
	outflankH = _mm_and_si128(_mm_andnot_si128(_mm_add_epi32(outflankH, minusone), outflankH), PH);
	outflankH = _mm_add_epi32(outflankH, minusone);
	flippedH = _mm_and_si128(maskH, _mm_add_epi32(outflankH, _mm_srli_epi32(outflankH, 31)));
	flipped = _mm_or_si128(flipped, _mm_xor_si128(flippedH, _mm_shuffle_epi32(flippedH, 0xa0)));

	return _mm_or_si128(flipped, _mm_shuffle_epi32(flipped, SWAP64));
}

/**
 * Compute flipped discs when playing on square F5.
 *
 * @param P player's disc pattern.
 * @param O opponent's disc pattern.
 * @return flipped disc pattern.
 */
static __m128i vectorcall flip_F5(const __m128i OP)
{
	__m128i OL, OH, PL, PH, outflankL, outflankH, flippedL, flippedH, flipped;
	const __m128i maskL = _mm_set_epi32(0x40800000, 0x10080402, 0x0000001f, 0x20202020);
	const __m128i maskH = _mm_set_epi32(0x04081000, 0x00804000, 0x000000c0, 0x20202000);

	OL = _mm_shuffle_epi32(OP, 0xae);
	PL = _mm_shuffle_epi32(OP, 0x04);
	outflankL = _mm_and_si128(MS1B_epu31(_mm_andnot_si128(OL, maskL)), PL);
	flippedL = _mm_andnot_si128(_mm_add_epi32(_mm_add_epi32(outflankL, outflankL), minusone), maskL);
	flipped = _mm_or_si128(_mm_move_epi64(flippedL), _mm_shuffle_epi32(flippedL, 0x76));

	OH = _mm_shuffle_epi32(OP, 0xff);
	PH = _mm_shuffle_epi32(OP, 0x55);
	outflankH = _mm_andnot_si128(OH, maskH);
	outflankH = _mm_and_si128(_mm_andnot_si128(_mm_add_epi32(outflankH, minusone), outflankH), PH);
	outflankH = _mm_add_epi32(outflankH, minusone);
	flippedH = _mm_and_si128(maskH, _mm_add_epi32(outflankH, _mm_srli_epi32(outflankH, 31)));
	flipped = _mm_or_si128(flipped, _mm_xor_si128(flippedH, _mm_shuffle_epi32(flippedH, 0xa0)));

	return _mm_or_si128(flipped, _mm_shuffle_epi32(flipped, SWAP64));
}

/**
 * Compute flipped discs when playing on square G5.
 *
 * @param P player's disc pattern.
 * @param O opponent's disc pattern.
 * @return flipped disc pattern.
 */
static __m128i vectorcall flip_G5(const __m128i OP)
{
	__m128i PP = _mm_shuffle_epi32(OP, DUPLO);
	__m128i OO = _mm_shuffle_epi32(OP, DUPHI);
	__m128i	outflankH, outflankL, flipped;
	const __m128i maskL = _mm_set_epi32(0, 0x20100804, 0x0000003f, 0x40404040);
	const __m128i maskH = _mm_set_epi64x(~0x0810200000000000ULL, ~0x4040400000000000ULL);

	outflankH = _mm_and_si128(_mm_andnot_si128(maskH, _mm_sub_epi64(_mm_or_si128(OO, maskH), minusone)), PP);
	flipped = _mm_andnot_si128(maskH, _mm_sub_epi64(outflankH, _mm_shuffle_epi32(outflankH, SWAP32)));

	outflankL = _mm_and_si128(MS1B_epu31(_mm_andnot_si128(OO, maskL)), PP);
	flipped = _mm_or_si128(flipped, _mm_and_si128(_mm_sub_epi32(_mm_setzero_si128(), _mm_add_epi32(outflankL, outflankL)), maskL));

	return _mm_or_si128(flipped, _mm_shuffle_epi32(flipped, SWAP64));
}

/**
 * Compute flipped discs when playing on square H5.
 *
 * @param P player's disc pattern.
 * @param O opponent's disc pattern.
 * @return flipped disc pattern.
 */
static __m128i vectorcall flip_H5(const __m128i OP)
{
	__m128i PP = _mm_shuffle_epi32(OP, DUPLO);
	__m128i OO = _mm_shuffle_epi32(OP, DUPHI);
	__m128i	outflankH, outflankL, flipped;
	const __m128i maskL = _mm_set_epi32(0, 0x40201008, 0x0000007f, 0x80808080);
	const __m128i maskH = _mm_set_epi64x(~0x1020400000000000ULL, ~0x8080800000000000ULL);

	outflankH = _mm_and_si128(_mm_andnot_si128(maskH, _mm_sub_epi64(_mm_or_si128(OO, maskH), minusone)), PP);
	flipped = _mm_andnot_si128(maskH, _mm_sub_epi64(outflankH, _mm_shuffle_epi32(outflankH, SWAP32)));

	outflankL = _mm_and_si128(MS1B_epu32(_mm_andnot_si128(OO, maskL)), PP);
	flipped = _mm_or_si128(flipped, _mm_and_si128(_mm_sub_epi32(_mm_setzero_si128(), _mm_add_epi32(outflankL, outflankL)), maskL));

	return _mm_or_si128(flipped, _mm_shuffle_epi32(flipped, SWAP64));
}

/**
 * Compute flipped discs when playing on square A6.
 *
 * @param P player's disc pattern.
 * @param O opponent's disc pattern.
 * @return flipped disc pattern.
 */
static __m128i vectorcall flip_A6(const __m128i OP)
{
	__m128i PP = _mm_shuffle_epi32(OP, DUPLO);
	__m128i OO = _mm_shuffle_epi32(OP, DUPHI);
	__m128i	outflank_vd, outflank_h_a7b7, flipped;
	const __m128i mask1 = _mm_set_epi64x(0x0000000204081020, 0x0000000101010101);
	const __m128i mask2 = _mm_set_epi64x(~0x0402000000000000ULL, ~0x0101fe0000000000ULL);

	outflank_h_a7b7 = _mm_and_si128(_mm_andnot_si128(mask2, _mm_sub_epi16(_mm_or_si128(OO, mask2), minusone)), PP);
	flipped = _mm_andnot_si128(mask2, _mm_mulhi_epu16(outflank_h_a7b7, minusone));

	outflank_vd = _mm_and_si128(MS1B_epu52(_mm_andnot_si128(OO, mask1)), PP);
	outflank_vd = _mm_add_epi64(_mm_add_epi64(outflank_vd, outflank_vd), minusone);
	flipped = _mm_or_si128(flipped, _mm_andnot_si128(outflank_vd, mask1));

	return _mm_or_si128(flipped, _mm_shuffle_epi32(flipped, SWAP64));
}

/**
 * Compute flipped discs when playing on square B6.
 *
 * @param P player's disc pattern.
 * @param O opponent's disc pattern.
 * @return flipped disc pattern.
 */
static __m128i vectorcall flip_B6(const __m128i OP)
{
	__m128i PP = _mm_shuffle_epi32(OP, DUPLO);
	__m128i OO = _mm_shuffle_epi32(OP, DUPHI);
	__m128i	outflank_vd, outflank_h_b7c7, flipped;
	const __m128i mask1 = _mm_set_epi64x(0x0000000408102040, 0x0000000202020202);
	const __m128i mask2 = _mm_set_epi64x(~0x0804000000000000ULL, ~0x0202fc0000000000ULL);

	outflank_h_b7c7 = _mm_and_si128(_mm_andnot_si128(mask2, _mm_sub_epi16(_mm_or_si128(OO, mask2), minusone)), PP);
	flipped = _mm_andnot_si128(mask2, _mm_mulhi_epu16(outflank_h_b7c7, minusone));

	outflank_vd = _mm_and_si128(MS1B_epu52(_mm_andnot_si128(OO, mask1)), PP);
	outflank_vd = _mm_add_epi64(_mm_add_epi64(outflank_vd, outflank_vd), minusone);
	flipped = _mm_or_si128(flipped, _mm_andnot_si128(outflank_vd, mask1));

	return _mm_or_si128(flipped, _mm_shuffle_epi32(flipped, SWAP64));
}

/**
 * Compute flipped discs when playing on square C6.
 *
 * @param P player's disc pattern.
 * @param O opponent's disc pattern.
 * @return flipped disc pattern.
 */
static __m128i vectorcall flip_C6(const __m128i OP)
{
	__m128i PP = _mm_shuffle_epi32(OP, DUPLO);
	__m128i OO = _mm_shuffle_epi32(OP, DUPHI);
	__m128i	outflank_vd, outflank_h, flipped, flipped_b5b6b7c7d7;
	const __m128i mask = _mm_set_epi64x(0x0000000810204080, 0x0000000404040404);
	const __m128i next_h = _mm_loadl_epi64((__m128i *) &X_TO_BIT[43]);

	outflank_vd = _mm_and_si128(MS1B_epu52(_mm_andnot_si128(OO, mask)), PP);
	flipped = _mm_and_si128(_mm_sub_epi64(_mm_setzero_si128(), _mm_add_epi64(outflank_vd, outflank_vd)), mask);

	outflank_h = _mm_and_si128(_mm_add_epi8(OO, next_h), PP);
	flipped = _mm_or_si128(flipped, _mm_subs_epu8(outflank_h, next_h));

	flipped_b5b6b7c7d7 = _mm_and_si128(_mm_shufflehi_epi16(PP, 0xf4), _mm_set_epi64x(0x0400100001000000, 0x0100010000000000));	// c8e8a4.a8a6..
	flipped_b5b6b7c7d7 = _mm_madd_epi16(flipped_b5b6b7c7d7, _mm_set_epi16(0x0100, 0x0080, 0x0200, 0, 0x0200, 2, 0, 0));
	flipped = _mm_or_si128(flipped, _mm_and_si128(_mm_shufflehi_epi16(flipped_b5b6b7c7d7, 0xd0), OO));

	return _mm_or_si128(flipped, _mm_shuffle_epi32(flipped, SWAP64));
}

/**
 * Compute flipped discs when playing on square D6.
 *
 * @param P player's disc pattern.
 * @param O opponent's disc pattern.
 * @return flipped disc pattern.
 */
static __m128i vectorcall flip_D6(const __m128i OP)
{
	__m128i	flipped, flipped_c7e7, index_d;
	unsigned int outflank_h, outflank_v, outflank_d, index_v;
	const __m128i mask = _mm_set_epi64x(0x0000081422418000, 0x0808080808080808);	// A3D6H2

	index_v = _mm_movemask_epi8(_mm_slli_epi64(OP, 4));
	outflank_v = OUTFLANK_5[(index_v >> 9) & 0x3f] & rotl8(index_v, 1);

	index_d = _mm_sad_epu8(_mm_and_si128(OP, _mm_shuffle_epi32(mask, DUPHI)), _mm_setzero_si128());
	outflank_d = OUTFLANK_3[(_mm_extract_epi16(index_d, 4) >> 1) & 0x3f] & rotl8(_mm_cvtsi128_si32(index_d), 3);

	flipped = _mm_and_si128(load64x2(&FLIPPED_5_V[outflank_v], &FLIPPED_3_H[outflank_d]), mask);

	outflank_h = OUTFLANK_3[(_mm_extract_epi16(OP, 6) >> 9) & 0x3f] & rotl8(_mm_extract_epi16(OP, 2) >> 8, 3);

	flipped_c7e7 = _mm_shuffle_epi32(OP, 0xf5);
	flipped_c7e7 = _mm_and_si128(flipped_c7e7, _mm_set_epi32(0x00100000, 0x00040000, 0x20000000, 0x02000000));
	flipped_c7e7 = _mm_min_epi16(flipped_c7e7, _mm_shuffle_epi32(flipped_c7e7, SWAP64));
	flipped = _mm_or_si128(flipped, _mm_unpacklo_epi16(
		_mm_slli_epi64(_mm_loadl_epi64((__m128i *) &FLIPPED_3_H[outflank_h]), 56), flipped_c7e7));

	return _mm_or_si128(flipped, _mm_shuffle_epi32(flipped, SWAP64));
}

/**
 * Compute flipped discs when playing on square E6.
 *
 * @param P player's disc pattern.
 * @param O opponent's disc pattern.
 * @return flipped disc pattern.
 */
static __m128i vectorcall flip_E6(const __m128i OP)
{
	__m128i	flipped, flipped_d7f7, index_d;
	unsigned int outflank_h, outflank_v, outflank_d, index_v;
	const __m128i mask = _mm_set_epi64x(0x0000102844820100, 0x1010101010101010);	// A2E6H3

	index_v = _mm_movemask_epi8(_mm_slli_epi64(OP, 3));
	outflank_v = OUTFLANK_5[(index_v >> 9) & 0x3f] & rotl8(index_v, 1);

	index_d = _mm_sad_epu8(_mm_and_si128(OP, _mm_shuffle_epi32(mask, DUPHI)), _mm_setzero_si128());
	outflank_d = OUTFLANK_4[(_mm_extract_epi16(index_d, 4) >> 1) & 0x3f] & rotl8(_mm_cvtsi128_si32(index_d), 2);

	flipped = _mm_and_si128(load64x2(&FLIPPED_5_V[outflank_v], &FLIPPED_4_H[outflank_d]), mask);

	outflank_h = OUTFLANK_4[(_mm_extract_epi16(OP, 6) >> 9) & 0x3f] & rotl8(_mm_extract_epi16(OP, 2) >> 8, 2);

	flipped_d7f7 = _mm_shuffle_epi32(OP, 0xf5);
	flipped_d7f7 = _mm_and_si128(flipped_d7f7, _mm_set_epi32(0x00200000, 0x00080000, 0x40000000, 0x04000000));
	flipped_d7f7 = _mm_min_epi16(flipped_d7f7, _mm_shuffle_epi32(flipped_d7f7, SWAP64));
	flipped = _mm_or_si128(flipped, _mm_unpacklo_epi16(
		_mm_slli_epi64(_mm_loadl_epi64((__m128i *) &FLIPPED_4_H[outflank_h]), 56), flipped_d7f7));

	return _mm_or_si128(flipped, _mm_shuffle_epi32(flipped, SWAP64));
}

/**
 * Compute flipped discs when playing on square F6.
 *
 * @param P player's disc pattern.
 * @param O opponent's disc pattern.
 * @return flipped disc pattern.
 */
static __m128i vectorcall flip_F6(const __m128i OP)
{
	__m128i PP = _mm_shuffle_epi32(OP, DUPLO);
	__m128i OO = _mm_shuffle_epi32(OP, DUPHI);
	__m128i	outflank, flipped, flipped_g5g6g7f7e7;

	outflank = MS1B_epu52(_mm_andnot_si128(OO, _mm_set_epi64x(0x0000002020202020, 0x0000001008040201)));
	outflank = _mm_or_si128(outflank, MS1B_epu31(_mm_andnot_si128(OO, _mm_set_epi32(0, 0, 0x00001f00, 0))));
	outflank = _mm_and_si128(outflank, PP);

	flipped = _mm_sub_epi64(_mm_loadl_epi64((__m128i *) &X_TO_BIT[39]), _mm_add_epi64(outflank, outflank));
	flipped = _mm_and_si128(flipped, _mm_set_epi64x(0x0000002020202020, 0x00001e1008040201));

	flipped_g5g6g7f7e7 = _mm_and_si128(_mm_shufflehi_epi16(PP, 0xf9), _mm_set_epi64x(0x2000080080008000, 0x8000000000000000));	// f8d8h6h4h8...
	flipped_g5g6g7f7e7 = _mm_madd_epi16(flipped_g5g6g7f7e7, _mm_set_epi16(0x0100, 0x0200, -0x8000, -0x0080, -0x0080, 0, 0, 0));
	flipped = _mm_or_si128(flipped, _mm_and_si128(_mm_shufflehi_epi16(flipped_g5g6g7f7e7, 0xd0), OO));

	return _mm_or_si128(flipped, _mm_shuffle_epi32(flipped, SWAP64));
}

/**
 * Compute flipped discs when playing on square G6.
 *
 * @param P player's disc pattern.
 * @param O opponent's disc pattern.
 * @return flipped disc pattern.
 */
static __m128i vectorcall flip_G6(const __m128i OP)
{
	__m128i PP = _mm_shuffle_epi32(OP, DUPLO);
	__m128i OO = _mm_shuffle_epi32(OP, DUPHI);
	__m128i	outflank, flipped;

	outflank = MS1B_epu52(_mm_andnot_si128(OO, _mm_set_epi64x(0x0000004040404040, 0x0000002010080402)));
	outflank = _mm_or_si128(outflank, MS1B_epu31(_mm_andnot_si128(OO, _mm_set_epi32(0, 0, 0x00003f00, 0))));
	outflank = _mm_and_si128(outflank, PP);

	flipped = _mm_sub_epi64(_mm_set_epi64x(0x0000800000000000, 0x0000808000000000), _mm_add_epi64(outflank, outflank));
	flipped = _mm_or_si128(flipped, _mm_and_si128(_mm_mulhi_epu16(PP, _mm_set_epi16(0x0100, 0, 0, 0, 0x0200, 0, 0, 0)), OO));	// g7f7
	flipped = _mm_and_si128(flipped, _mm_set_epi64x(0x0040004040404040, 0x00203e2010080402));

	return _mm_or_si128(flipped, _mm_shuffle_epi32(flipped, SWAP64));
}

/**
 * Compute flipped discs when playing on square H6.
 *
 * @param P player's disc pattern.
 * @param O opponent's disc pattern.
 * @return flipped disc pattern.
 */
static __m128i vectorcall flip_H6(const __m128i OP)
{
	__m128i PP = _mm_shuffle_epi32(OP, DUPLO);
	__m128i OO = _mm_shuffle_epi32(OP, DUPHI);
	__m128i	outflank, flipped;

	outflank = MS1B_epu52(_mm_andnot_si128(OO, _mm_set_epi64x(0x0000008080808080, 0x0000004020100804)));
	outflank = _mm_or_si128(outflank, MS1B_epu31(_mm_andnot_si128(OO, _mm_set_epi32(0, 0, 0x00007f00, 0))));
	outflank = _mm_and_si128(outflank, PP);

	flipped = _mm_sub_epi64(_mm_set_epi64x(0x0000800000000000, 0x0000808000000000), _mm_add_epi64(outflank, outflank));
	flipped = _mm_or_si128(flipped, _mm_and_si128(_mm_mulhi_epu16(PP, _mm_set_epi16(0x0100, 0, 0, 0, 0x0200, 0, 0, 0)), OO));	// h7g7
	flipped = _mm_and_si128(flipped, _mm_set_epi64x(0x0080008080808080, 0x00407e4020100804));

	return _mm_or_si128(flipped, _mm_shuffle_epi32(flipped, SWAP64));
}

/**
 * Compute flipped discs when playing on square A7.
 *
 * @param P player's disc pattern.
 * @param O opponent's disc pattern.
 * @return flipped disc pattern.
 */
static __m128i vectorcall flip_A7(const __m128i OP)
{
	__m128i PP = _mm_shuffle_epi32(OP, DUPLO);
	__m128i OO = _mm_shuffle_epi32(OP, DUPHI);
	__m128i	outflank_vd, flipped, outflank_h, next_h;
	const __m128i mask = _mm_set_epi64x(0x0000020408102040, 0x0000010101010101);

	outflank_vd = _mm_and_si128(MS1B_epu52(_mm_andnot_si128(OO, mask)), PP);
	flipped = _mm_andnot_si128(_mm_add_epi64(_mm_add_epi64(outflank_vd, outflank_vd), minusone), mask);

	next_h = _mm_loadl_epi64((__m128i *) &X_TO_BIT[49]);
	outflank_h = _mm_and_si128(_mm_add_epi8(OO, next_h), PP);
	flipped = _mm_or_si128(flipped, _mm_subs_epu8(outflank_h, next_h));

	return _mm_or_si128(flipped, _mm_shuffle_epi32(flipped, SWAP64));
}

/**
 * Compute flipped discs when playing on square B7.
 *
 * @param P player's disc pattern.
 * @param O opponent's disc pattern.
 * @return flipped disc pattern.
 */
static __m128i vectorcall flip_B7(const __m128i OP)
{
	__m128i PP = _mm_shuffle_epi32(OP, DUPLO);
	__m128i OO = _mm_shuffle_epi32(OP, DUPHI);
	__m128i	outflank_vd, flipped, outflank_h, next_h;
	const __m128i mask = _mm_set_epi64x(0x0000040810204080, 0x0000020202020202);

	outflank_vd = _mm_and_si128(MS1B_epu52(_mm_andnot_si128(OO, mask)), PP);
	flipped = _mm_andnot_si128(_mm_add_epi64(_mm_add_epi64(outflank_vd, outflank_vd), minusone), mask);

	next_h = _mm_loadl_epi64((__m128i *) &X_TO_BIT[50]);
	outflank_h = _mm_and_si128(_mm_add_epi8(OO, next_h), PP);
	flipped = _mm_or_si128(flipped, _mm_subs_epu8(outflank_h, next_h));

	return _mm_or_si128(flipped, _mm_shuffle_epi32(flipped, SWAP64));
}

/**
 * Compute flipped discs when playing on square C7.
 *
 * @param P player's disc pattern.
 * @param O opponent's disc pattern.
 * @return flipped disc pattern.
 */
static __m128i vectorcall flip_C7(const __m128i OP)
{
	__m128i PP = _mm_shuffle_epi32(OP, DUPLO);
	__m128i OO = _mm_shuffle_epi32(OP, DUPHI);
	__m128i	flipped, outflank_vd, flipped_h_b6b7;
	const __m128i mask = _mm_set_epi64x(0x0000081020408000, 0x0000040404040404);

	flipped_h_b6b7 = _mm_and_si128(_mm_adds_epu8(OO, _mm_set_epi8(0, -1, 0, -1, 0, 0, 0, 0, 0, 0x08, 0, 0, 0, 0, 0, 0)), PP);
	flipped_h_b6b7 = _mm_srli_epi64(_mm_mullo_epi16(flipped_h_b6b7, _mm_set_epi16(0x0020, 0x2000, 0, 0, 0x000f, 0, 0, 0)), 4);
	flipped_h_b6b7 = _mm_and_si128(_mm_and_si128(flipped_h_b6b7, OO), _mm_set_epi16(0x0002, 0x0200, 0, 0, 0x0078, 0, 0, 0));

	outflank_vd = _mm_and_si128(MS1B_epu52(_mm_andnot_si128(OO, mask)), PP);
	flipped = _mm_or_si128(flipped_h_b6b7, _mm_and_si128(_mm_sub_epi64(_mm_setzero_si128(), _mm_add_epi64(outflank_vd, outflank_vd)), mask));

	return _mm_or_si128(flipped, _mm_shuffle_epi32(flipped, SWAP64));
}

/**
 * Compute flipped discs when playing on square D7.
 *
 * @param P player's disc pattern.
 * @param O opponent's disc pattern.
 * @return flipped disc pattern.
 */
static __m128i vectorcall flip_D7(const __m128i OP)
{
	__m128i	flipped, index_d;
	unsigned int outflank_h, outflank_d, outflank_v, index_v;
	const __m128i mask = _mm_set_epi64x(0x0000080808080808, 0x00ff142241800000);

	index_v = _mm_movemask_epi8(_mm_slli_epi64(OP, 4));
	outflank_v = OUTFLANK_7[((index_v >> 9) & 0x1f) + 32] & index_v;

	index_d = _mm_sad_epu8(_mm_and_si128(OP, _mm_set1_epi64x(0x0008142241800000)), _mm_setzero_si128());	// A4D7H3
	outflank_d = OUTFLANK_3[(_mm_extract_epi16(index_d, 4) >> 1) & 0x3f] & rotl8(_mm_cvtsi128_si32(index_d), 3);

	flipped = load64x2(&FLIPPED_3_H[outflank_d], &FLIPPED_7_V[outflank_v]);

	outflank_h = OUTFLANK_3[(_mm_extract_epi16(OP, 7) >> 1) & 0x3f] & rotl8(_mm_extract_epi16(OP, 3), 3);
	flipped = _mm_and_si128(_mm_insert_epi16(flipped, FLIPPED_3_H[outflank_h], 3), mask);

	return _mm_or_si128(flipped, _mm_shuffle_epi32(flipped, SWAP64));
}

/**
 * Compute flipped discs when playing on square E7.
 *
 * @param P player's disc pattern.
 * @param O opponent's disc pattern.
 * @return flipped disc pattern.
 */
static __m128i vectorcall flip_E7(const __m128i OP)
{
	__m128i	flipped, index_d;
	unsigned int outflank_h, outflank_d, outflank_v, index_v;
	const __m128i mask = _mm_set_epi64x(0x0000101010101010, 0x00ff284482010000);

	index_v = _mm_movemask_epi8(_mm_slli_epi64(OP, 3));
	outflank_v = OUTFLANK_7[((index_v >> 9) & 0x1f) + 32] & index_v;

	index_d = _mm_sad_epu8(_mm_and_si128(OP, _mm_set1_epi64x(0x0010284482010000)), _mm_setzero_si128());	// A3E7H4
	outflank_d = OUTFLANK_4[(_mm_extract_epi16(index_d, 4) >> 1) & 0x3f] & rotl8(_mm_cvtsi128_si32(index_d), 2);

	flipped = load64x2(&FLIPPED_4_H[outflank_d], &FLIPPED_7_V[outflank_v]);

	outflank_h = OUTFLANK_4[(_mm_extract_epi16(OP, 7) >> 1) & 0x3f] & rotl8(_mm_extract_epi16(OP, 3), 2);
	flipped = _mm_and_si128(_mm_insert_epi16(flipped, FLIPPED_4_H[outflank_h], 3), mask);

	return _mm_or_si128(flipped, _mm_shuffle_epi32(flipped, SWAP64));
}

/**
 * Compute flipped discs when playing on square F7.
 *
 * @param P player's disc pattern.
 * @param O opponent's disc pattern.
 * @return flipped disc pattern.
 */
static __m128i vectorcall flip_F7(const __m128i OP)
{
	__m128i PP = _mm_shuffle_epi32(OP, DUPLO);
	__m128i OO = _mm_shuffle_epi32(OP, DUPHI);
	__m128i	outflank, flipped, flipped_g6g7;

	outflank = MS1B_epu52(_mm_andnot_si128(OO, _mm_set_epi64x(0x0000202020202020, 0x0000100804020100)));
	outflank = _mm_or_si128(outflank, MS1B_epu31(_mm_andnot_si128(OO, _mm_set_epi32(0, 0, 0x001f0000, 0))));
	outflank = _mm_and_si128(outflank, PP);

	flipped = _mm_sub_epi64(_mm_loadl_epi64((__m128i *) &X_TO_BIT[47]), _mm_add_epi64(outflank, outflank));
	flipped = _mm_and_si128(flipped, _mm_set_epi64x(0x0000202020202020, 0x001e100804020100));

	flipped_g6g7 = _mm_srli_epi64(_mm_and_si128(PP, _mm_set_epi64x(0x0080008000000000, 0)), 17);
	flipped = _mm_or_si128(flipped, _mm_and_si128(_mm_packus_epi16(flipped_g6g7, flipped_g6g7), OO));

	return _mm_or_si128(flipped, _mm_shuffle_epi32(flipped, SWAP64));
}

/**
 * Compute flipped discs when playing on square G7.
 *
 * @param P player's disc pattern.
 * @param O opponent's disc pattern.
 * @return flipped disc pattern.
 */
static __m128i vectorcall flip_G7(const __m128i OP)
{
	__m128i PP = _mm_shuffle_epi32(OP, DUPLO);
	__m128i OO = _mm_shuffle_epi32(OP, DUPHI);
	__m128i outflank, flipped;

	outflank = MS1B_epu52(_mm_andnot_si128(OO, _mm_set_epi64x(0x0000404040404040, 0x0000201008040201)));
	outflank = _mm_or_si128(outflank, MS1B_epu31(_mm_andnot_si128(OO, _mm_set_epi32(0, 0, 0x003f0000, 0))));
	outflank = _mm_and_si128(outflank, PP);

	flipped = _mm_sub_epi64(_mm_loadl_epi64((__m128i *) &X_TO_BIT[47]), _mm_add_epi64(outflank, outflank));
	flipped = _mm_and_si128(flipped, _mm_set_epi64x(0x0000404040404040, 0x003e201008040201));

	return _mm_or_si128(flipped, _mm_shuffle_epi32(flipped, SWAP64));
}

/**
 * Compute flipped discs when playing on square H7.
 *
 * @param P player's disc pattern.
 * @param O opponent's disc pattern.
 * @return flipped disc pattern.
 */
static __m128i vectorcall flip_H7(const __m128i OP)
{
	__m128i PP = _mm_shuffle_epi32(OP, DUPLO);
	__m128i OO = _mm_shuffle_epi32(OP, DUPHI);
	__m128i outflank, flipped;

	outflank = MS1B_epu52(_mm_andnot_si128(OO, _mm_set_epi64x(0x0000808080808080, 0x0000402010080402)));
	outflank = _mm_or_si128(outflank, MS1B_epu31(_mm_andnot_si128(OO, _mm_set_epi32(0, 0, 0x007f0000, 0))));
	outflank = _mm_and_si128(outflank, PP);

	flipped = _mm_sub_epi64(_mm_loadl_epi64((__m128i *) &X_TO_BIT[47]), _mm_add_epi64(outflank, outflank));
	flipped = _mm_and_si128(flipped, _mm_set_epi64x(0x0000808080808080, 0x007e402010080402));

	return _mm_or_si128(flipped, _mm_shuffle_epi32(flipped, SWAP64));
}

/**
 * Compute flipped discs when playing on square A8.
 *
 * @param P player's disc pattern.
 * @param O opponent's disc pattern.
 * @return flipped disc pattern.
 */
static __m128i vectorcall flip_A8(const __m128i OP)
{
	__m128i PP = _mm_shuffle_epi32(OP, DUPLO);
	__m128i OO = _mm_shuffle_epi32(OP, DUPHI);
	__m128i	outflank_vd, flipped, outflank_h, next_h;
	const __m128i mask = _mm_set_epi64x(0x0002040810204080, 0x0001010101010101);

	outflank_vd = _mm_and_si128(MS1B_epu52(_mm_andnot_si128(OO, mask)), PP);
	flipped = _mm_andnot_si128(_mm_add_epi64(_mm_add_epi64(outflank_vd, outflank_vd), minusone), mask);

	next_h = _mm_loadl_epi64((__m128i *) &X_TO_BIT[57]);
	outflank_h = _mm_and_si128(_mm_add_epi8(OO, next_h), PP);
	flipped = _mm_or_si128(flipped, _mm_subs_epu8(outflank_h, next_h));

	return _mm_or_si128(flipped, _mm_shuffle_epi32(flipped, SWAP64));
}

/**
 * Compute flipped discs when playing on square B8.
 *
 * @param P player's disc pattern.
 * @param O opponent's disc pattern.
 * @return flipped disc pattern.
 */
static __m128i vectorcall flip_B8(const __m128i OP)
{
	__m128i PP = _mm_shuffle_epi32(OP, DUPLO);
	__m128i OO = _mm_shuffle_epi32(OP, DUPHI);
	__m128i	outflank_vd, flipped, outflank_h, next_h;
	const __m128i mask = _mm_set_epi64x(0x0004081020408000, 0x0002020202020202);

	outflank_vd = _mm_and_si128(MS1B_epu52(_mm_andnot_si128(OO, mask)), PP);
	flipped = _mm_andnot_si128(_mm_add_epi64(_mm_add_epi64(outflank_vd, outflank_vd), minusone), mask);

	next_h = _mm_loadl_epi64((__m128i *) &X_TO_BIT[58]);
	outflank_h = _mm_and_si128(_mm_add_epi8(OO, next_h), PP);
	flipped = _mm_or_si128(flipped, _mm_subs_epu8(outflank_h, next_h));

	return _mm_or_si128(flipped, _mm_shuffle_epi32(flipped, SWAP64));
}

/**
 * Compute flipped discs when playing on square C8.
 *
 * @param P player's disc pattern.
 * @param O opponent's disc pattern.
 * @return flipped disc pattern.
 */
static __m128i vectorcall flip_C8(const __m128i OP)
{
	__m128i PP = _mm_shuffle_epi32(OP, DUPLO);
	__m128i OO = _mm_shuffle_epi32(OP, DUPHI);
	__m128i	flipped, outflank_vd, flipped_h_b7b8;
	const __m128i mask = _mm_set_epi64x(0x0008102040800000, 0x0004040404040404);

	flipped_h_b7b8 = _mm_and_si128(_mm_adds_epu8(OO, _mm_set_epi8(-1, 0, -1, 0, 0, 0, 0, 0, 0x08, 0, 0, 0, 0, 0, 0, 0)), PP);
	flipped_h_b7b8 = _mm_slli_epi64(_mm_mullo_epi16(_mm_srli_epi64(flipped_h_b7b8, 8), _mm_set_epi16(0x0020, 0x2000, 0, 0, 0x000f, 0, 0, 0)), 4);
	flipped_h_b7b8 = _mm_and_si128(_mm_and_si128(flipped_h_b7b8, OO), _mm_set_epi16(0x0202, 0, 0, 0, 0x7800, 0, 0, 0));

	outflank_vd = _mm_and_si128(MS1B_epu52(_mm_andnot_si128(OO, mask)), PP);
	flipped = _mm_or_si128(flipped_h_b7b8, _mm_and_si128(_mm_sub_epi64(_mm_setzero_si128(), _mm_add_epi64(outflank_vd, outflank_vd)), mask));

	return _mm_or_si128(flipped, _mm_shuffle_epi32(flipped, SWAP64));
}

/**
 * Compute flipped discs when playing on square D8.
 *
 * @param P player's disc pattern.
 * @param O opponent's disc pattern.
 * @return flipped disc pattern.
 */
static __m128i vectorcall flip_D8(const __m128i OP)
{
	__m128i	flipped, index_d;
	unsigned int outflank_h, outflank_d;
	const __m128i mask = _mm_set_epi64x(0x0008080808080808, 0x0814224180000000);	// A5D8H4
#if 1	// TLU x 2 - 42(gcc)/45(VC)
	unsigned int outflank_v, index_v;

	index_v = _mm_movemask_epi8(_mm_slli_epi64(OP, 4));
	outflank_v = OUTFLANK_7[(index_v >> 9) & 0x3f] & index_v;

	index_d = _mm_sad_epu8(_mm_and_si128(OP, _mm_shuffle_epi32(mask, DUPLO)), _mm_setzero_si128());
	outflank_d = OUTFLANK_3[(_mm_extract_epi16(index_d, 4) >> 1) & 0x3f] & rotl8(_mm_cvtsi128_si32(index_d), 3);

	flipped = _mm_and_si128(mask, load64x2(&FLIPPED_3_H[outflank_d], &FLIPPED_7_V[outflank_v]));

#else	// TLU + MS1B - 45(gcc)/52(VC)
	__m128i	outflank_v;

	index_d = _mm_sad_epu8(_mm_and_si128(OP, _mm_shuffle_epi32(mask, DUPLO)), _mm_setzero_si128());
	outflank_d = OUTFLANK_3[(_mm_extract_epi16(index_d, 4) >> 1) & 0x3f] & rotl8(_mm_cvtsi128_si32(index_d), 3);
	flipped = _mm_loadl_epi64((__m128i *) &FLIPPED_3_H[outflank_d]);

	outflank_v = _mm_and_si128(MS1B_epu52(_mm_andnot_si128(OP, mask)), _mm_slli_si128(OP, 8));
	flipped = _mm_and_si128(mask, _mm_sub_epi64(flipped, _mm_add_epi64(outflank_v, outflank_v)));
#endif

	outflank_h = OUTFLANK_3[(_mm_extract_epi16(OP, 7) >> 9) & 0x3f] & rotl8(_mm_extract_epi16(OP, 3) >> 8, 3);
	flipped = _mm_or_si128(flipped, _mm_slli_epi64(_mm_loadl_epi64((__m128i *) &FLIPPED_3_H[outflank_h]), 56));

	return _mm_or_si128(flipped, _mm_shuffle_epi32(flipped, SWAP64));
}

#if 0	// MS1B x 2 - 51(gcc)/52(VC)
static __m128i vectorcall flip_D8(const __m128i OP)
{
	__m128i PP = _mm_shuffle_epi32(OP, DUPLO);
	__m128i OO = _mm_shuffle_epi32(OP, DUPHI);
	__m128i	flipped, outflank, next_h;
	const __m128i mask1 = _mm_set_epi64x(0x0008080808080808, 0x0010204080000000);
	const __m128i mask2 = _mm_set_epi64x(0x0700000000000000, 0x0004020100000000);

	outflank = _mm_and_si128(_mm_slli_epi64(MS1B_epu52(_mm_srli_epi64(_mm_andnot_si128(OO, mask1), 1)), 1), PP);
	flipped = _mm_and_si128(mask1, _mm_sub_epi64(_mm_setzero_si128(), _mm_add_epi64(outflank, outflank)));

	outflank = _mm_and_si128(MS1B_epu31(_mm_andnot_si128(OO, mask2)), PP);
	flipped = _mm_or_si128(flipped, _mm_and_si128(mask2, _mm_sub_epi64(_mm_setzero_si128(), _mm_add_epi64(outflank, outflank))));

	next_h = _mm_loadl_epi64((__m128i *) &X_TO_BIT[60]);
	outflank = _mm_and_si128(_mm_add_epi8(OO, next_h), PP);
	flipped = _mm_or_si128(flipped, _mm_subs_epu8(outflank, next_h));

	return _mm_or_si128(flipped, _mm_shuffle_epi32(flipped, SWAP64));
}
#endif

/**
 * Compute flipped discs when playing on square E8.
 *
 * @param P player's disc pattern.
 * @param O opponent's disc pattern.
 * @return flipped disc pattern.
 */
static __m128i vectorcall flip_E8(const __m128i OP)
{
	__m128i	flipped, index_d;
	unsigned int outflank_h, outflank_d, outflank_v, index_v;
	const __m128i mask = _mm_set_epi64x(0x0010101010101010, 0x1028448201000000);	// A4E8H5

	index_v = _mm_movemask_epi8(_mm_slli_epi64(OP, 3));
	outflank_v = OUTFLANK_7[(index_v >> 9) & 0x3f] & index_v;

	index_d = _mm_sad_epu8(_mm_and_si128(OP, _mm_shuffle_epi32(mask, DUPLO)), _mm_setzero_si128());
	outflank_d = OUTFLANK_4[(_mm_extract_epi16(index_d, 4) >> 1) & 0x3f] & rotl8(_mm_cvtsi128_si32(index_d), 2);

	flipped = _mm_and_si128(mask, load64x2(&FLIPPED_4_H[outflank_d], &FLIPPED_7_V[outflank_v]));

	outflank_h = OUTFLANK_4[(_mm_extract_epi16(OP, 7) >> 9) & 0x3f] & rotl8(_mm_extract_epi16(OP, 3) >> 8, 2);
	flipped = _mm_or_si128(flipped, _mm_slli_epi64(_mm_loadl_epi64((__m128i *) &FLIPPED_4_H[outflank_h]), 56));

	return _mm_or_si128(flipped, _mm_shuffle_epi32(flipped, SWAP64));
}

/**
 * Compute flipped discs when playing on square F8.
 *
 * @param P player's disc pattern.
 * @param O opponent's disc pattern.
 * @return flipped disc pattern.
 */
static __m128i vectorcall flip_F8(const __m128i OP)
{
	__m128i PP = _mm_shuffle_epi32(OP, DUPLO);
	__m128i OO = _mm_shuffle_epi32(OP, DUPHI);
	__m128i	outflank, flipped, flipped_g7g8;

	outflank = _mm_andnot_si128(OO, _mm_set_epi64x(0x0020202020202020, 0x0010080402010000));
	outflank = _mm_slli_epi64(MS1B_epu52(_mm_srli_epi64(outflank, 4)), 4);
	outflank = _mm_or_si128(outflank, MS1B_epu31(_mm_andnot_si128(OO, _mm_set_epi32(0, 0, 0x1f000000, 0))));
	outflank = _mm_and_si128(outflank, PP);

	flipped = _mm_sub_epi64(_mm_loadl_epi64((__m128i *) &X_TO_BIT[55]), _mm_add_epi64(outflank, outflank));
	flipped = _mm_and_si128(flipped, _mm_set_epi64x(0x0020202020202020, 0x1e10080402010000));

	flipped_g7g8 = _mm_srli_epi64(_mm_and_si128(PP, _mm_set_epi64x(0x8000800000000000, 0)), 9);
	flipped = _mm_or_si128(flipped, _mm_and_si128(_mm_packus_epi16(flipped_g7g8, flipped_g7g8), OO));

	return _mm_or_si128(flipped, _mm_shuffle_epi32(flipped, SWAP64));
}

/**
 * Compute flipped discs when playing on square G8.
 *
 * @param P player's disc pattern.
 * @param O opponent's disc pattern.
 * @return flipped disc pattern.
 */
static __m128i vectorcall flip_G8(const __m128i OP)
{
	__m128i PP = _mm_shuffle_epi32(OP, DUPLO);
	__m128i OO = _mm_shuffle_epi32(OP, DUPHI);
	__m128i outflank, flipped;

	outflank = _mm_andnot_si128(OO, _mm_set_epi64x(0x0040404040404040, 0x0020100804020100));
	outflank = _mm_slli_epi64(MS1B_epu52(_mm_srli_epi64(outflank, 4)), 4);
	outflank = _mm_or_si128(outflank, MS1B_epu31(_mm_andnot_si128(OO, _mm_set_epi32(0, 0, 0x3f000000, 0))));
	outflank = _mm_and_si128(outflank, PP);

	flipped = _mm_sub_epi64(_mm_loadl_epi64((__m128i *) &X_TO_BIT[55]), _mm_add_epi64(outflank, outflank));
	flipped = _mm_and_si128(flipped, _mm_set_epi64x(0x0040404040404040, 0x3e20100804020100));

	return _mm_or_si128(flipped, _mm_shuffle_epi32(flipped, SWAP64));
}

/**
 * Compute flipped discs when playing on square H8.
 *
 * @param P player's disc pattern.
 * @param O opponent's disc pattern.
 * @return flipped disc pattern.
 */
static __m128i vectorcall flip_H8(const __m128i OP)
{
	__m128i PP = _mm_shuffle_epi32(OP, DUPLO);
	__m128i OO = _mm_shuffle_epi32(OP, DUPHI);
	__m128i outflank, flipped;
	__m128i mask = _mm_set_epi64x(0x0080808080808080, 0x7e40201008040201);

	// outflank = MS1B_epu64(_mm_andnot_si128(OO, _mm_set_epi64x(0x0080808080808080, 0x0040201008040201)));
	outflank = _mm_andnot_si128(OO, mask);
	outflank = _mm_min_epu8(outflank, _mm_set_epi64x(0x0008080808080808, 0x0004020108040201));	// pack to 52 bits
	outflank = _mm_mullo_epi16(MS1B_epu52(outflank), _mm_set_epi16(16, 16, 16, 16, 16, 16, 1, 1));	// unpack

	outflank = _mm_or_si128(outflank, MS1B_epu31(_mm_andnot_si128(OO, _mm_set_epi32(0, 0, 0x7f000000, 0))));
	outflank = _mm_and_si128(outflank, PP);

	flipped = _mm_sub_epi64(_mm_loadl_epi64((__m128i *) &X_TO_BIT[55]), _mm_add_epi64(outflank, outflank));
	flipped = _mm_and_si128(flipped, mask);

	return _mm_or_si128(flipped, _mm_shuffle_epi32(flipped, SWAP64));
}

/**
 * Compute (zero-) flipped discs when plassing.
 *
 * @param P player's disc pattern.
 * @param O opponent's disc pattern.
 * @return flipped disc pattern.
 */
static __m128i vectorcall flip_pass(const __m128i OP)
{
	(void) OP; // useless code to shut-up compiler warning
	return _mm_setzero_si128();
}


/** Array of functions to compute flipped discs */
__m128i (vectorcall *mm_flip[])(const __m128i) = {
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

