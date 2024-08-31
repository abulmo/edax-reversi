<<<<<<< HEAD
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

=======
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
 * @date 1998 - 2018
 * @author Richard Delorme
 * @author Toshihiko Okuhara
 * @version 4.4
 */

#include "bit.h"

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

/** flip array (indexed with outflank, returns inner 6 bits) */
static const unsigned long long FLIPPED_2_H[130] = {
	0x0000000000000000ULL, 0x0202020202020202ULL, 0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL,
	0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL,
	0x0808080808080808ULL, 0x0a0a0a0a0a0a0a0aULL, 0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL,
	0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL,
	0x1818181818181818ULL, 0x1a1a1a1a1a1a1a1aULL, 0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL,
	0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL,
	0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL,
	0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL,
	0x3838383838383838ULL, 0x3a3a3a3a3a3a3a3aULL, 0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL,
	0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL,
	0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL,
	0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL,
	0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL,
	0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL,
	0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL,
	0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL,
	0x7878787878787878ULL, 0x7a7a7a7a7a7a7a7aULL
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

static const unsigned long long FLIPPED_5_H[137] = {
	0x0000000000000000ULL, 0x1e1e1e1e1e1e1e1eULL, 0x1c1c1c1c1c1c1c1cULL, 0x0000000000000000ULL, 0x1818181818181818ULL, 0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL,
	0x1010101010101010ULL, 0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL,
	0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL,
	0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL,
	0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL,
	0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL,
	0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL,
	0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL,
	0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL,
	0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL,
	0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL,
	0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL,
	0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL,
	0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL,
	0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL,
	0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL,
	0x4040404040404040ULL, 0x5e5e5e5e5e5e5e5eULL, 0x5c5c5c5c5c5c5c5cULL, 0x0000000000000000ULL, 0x5858585858585858ULL, 0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL,
	0x5050505050505050ULL
};

static const unsigned long long FLIPPED_2_V[130] = {
	0x0000000000000000ULL, 0x000000000000ff00ULL, 0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL,
	0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL,
	0x00000000ff000000ULL, 0x00000000ff00ff00ULL, 0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL,
	0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL,
	0x000000ffff000000ULL, 0x000000ffff00ff00ULL, 0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL,
	0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL,
	0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL,
	0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL,
	0x0000ffffff000000ULL, 0x0000ffffff00ff00ULL, 0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL,
	0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL,
	0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL,
	0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL,
	0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL,
	0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL,
	0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL,
	0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL,
	0x00ffffffff000000ULL, 0x00ffffffff00ff00ULL
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

static const unsigned long long  FLIPPED_5_V[137] = {
	0x0000000000000000ULL, 0x000000ffffffff00ULL, 0x000000ffffff0000ULL, 0x0000000000000000ULL, 0x000000ffff000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL,
	0x000000ff00000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL,
	0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL,
	0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL,
	0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL,
	0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL,
	0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL,
	0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL,
	0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL,
	0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL,
	0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL,
	0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL,
	0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL,
	0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL,
	0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL,
	0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL,
	0x00ff000000000000ULL, 0x00ff00ffffffff00ULL, 0x00ff00ffffff0000ULL, 0x0000000000000000ULL, 0x00ff00ffff000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL,
	0x00ff00ff00000000ULL
};



static const V2DI	minusone = {{ -1LL, -1LL }};

#define	SWAP64	0x4e	// for _mm_shuffle_epi32
#define	SWAP32	0xb1

#if 0 // defined(_MSC_VER) && defined(_M_X64) && !defined(__AVX2__)
static inline int _lzcnt_u64(unsigned long long n) {
	unsigned long i;
	if (!_BitScanReverse64(&i, n))
		i = -1;
	return 63 - i;
}
#endif

#if (defined(__x86_64__) && defined(__LZCNT__)) || (defined(_M_X64) && defined(__AVX2__))
	// Strictly, (long long) >> 64 is undefined in C, but either 0 bit (no change)
	// or 64 bit (zero out) shift will lead valid result (i.e. flipped == 0).
	#define	outflank_right(O,maskr,masko)	(0x8000000000000000ULL >> _lzcnt_u64(~(O) & (maskr)))
#elif defined(vertical_mirror)	// bswap to use carry propagation backwards
	#define	outflank_right(O,maskr,masko)	(vertical_mirror(vertical_mirror((O) | ~(maskr)) + 1) & (maskr))
#else	// with guardian bit to avoid __builtin_clz(0)
	#define	outflank_right(O,maskr,masko)	(0x8000000000000000ULL >> __builtin_clzll(((O) & (masko)) ^ (maskr)))
#endif


/**
 * Make inverted flip mask if opponent's disc are surrounded by player's.
 *
 * 0xffffffffffffffffULL (-1) if outflank is 0
 * 0x0000000000000000ULL ( 0) if a 1 is in 64 bit
 */
static inline __m128i flipmask (__m128i outflank) {
	return _mm_cmpeq_epi32(_mm_shuffle_epi32(outflank, SWAP32), outflank);
}

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
	__m128i	outflank_v_d;
	static const V2DI mask = {{ ~0x0101010101010100, ~0x8040201008040200 }};

	outflank_v_d = _mm_and_si128(_mm_andnot_si128(mask.v2, _mm_sub_epi64(_mm_or_si128(_mm_set1_epi64x(O), mask.v2), minusone.v2)), _mm_set1_epi64x(P));
	outflank_v_d = _mm_andnot_si128(mask.v2, _mm_sub_epi64(outflank_v_d, _mm_sub_epi64(flipmask(outflank_v_d), minusone.v2)));
	outflank_v_d = _mm_or_si128(outflank_v_d, _mm_shuffle_epi32(outflank_v_d, SWAP64));

	outflank_h = ((O & 0x7e) + 0x02) & P;

	return ((outflank_h - (unsigned int) (outflank_h != 0)) & 0x7e) | _mm_cvtsi128_si64(outflank_v_d);
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
	__m128i	outflank_v_d;
	static const V2DI mask = {{ ~0x0202020202020200, ~0x0080402010080400 }};

	outflank_v_d = _mm_and_si128(_mm_andnot_si128(mask.v2, _mm_sub_epi64(_mm_or_si128(_mm_set1_epi64x(O), mask.v2), minusone.v2)), _mm_set1_epi64x(P));
	outflank_v_d = _mm_andnot_si128(mask.v2, _mm_sub_epi64(outflank_v_d, _mm_sub_epi64(flipmask(outflank_v_d), minusone.v2)));
	outflank_v_d = _mm_or_si128(outflank_v_d, _mm_shuffle_epi32(outflank_v_d, SWAP64));

	outflank_h = ((O & 0x7c) + 0x04) & P;

	return ((outflank_h - (unsigned int) (outflank_h != 0)) & 0x7c) | _mm_cvtsi128_si64(outflank_v_d);
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
	__m128i	outflank_v_d;
	static const V2DI mask = {{ ~0x0404040404040400, ~0x0000804020100800 }};

	outflank_v_d = _mm_and_si128(_mm_andnot_si128(mask.v2, _mm_sub_epi64(_mm_or_si128(_mm_set1_epi64x(O), mask.v2), minusone.v2)), _mm_set1_epi64x(P));
	outflank_v_d = _mm_andnot_si128(mask.v2, _mm_sub_epi64(outflank_v_d, _mm_sub_epi64(flipmask(outflank_v_d), minusone.v2)));
	outflank_v_d = _mm_or_si128(outflank_v_d, _mm_shuffle_epi32(outflank_v_d, SWAP64));

	outflank_h = OUTFLANK_2[((unsigned int) O >> 1) & 0x3f] & P;

	return (unsigned char) FLIPPED_2_H[outflank_h]
		| ((P >> 7) & 0x0000000000000200ULL & O)
		| _mm_cvtsi128_si64(outflank_v_d);
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
	unsigned int outflank_h, outflank_d7;
	__m128i	outflank_v_d;
	static const V2DI mask = {{ ~0x0808080808080800, ~0x0000008040201000 }};

	outflank_v_d = _mm_and_si128(_mm_andnot_si128(mask.v2, _mm_sub_epi64(_mm_or_si128(_mm_set1_epi64x(O), mask.v2), minusone.v2)), _mm_set1_epi64x(P));
	outflank_v_d = _mm_andnot_si128(mask.v2, _mm_sub_epi64(outflank_v_d, _mm_sub_epi64(flipmask(outflank_v_d), minusone.v2)));
	outflank_v_d = _mm_or_si128(outflank_v_d, _mm_shuffle_epi32(outflank_v_d, SWAP64));

	outflank_h = OUTFLANK_3[((unsigned int) O >> 1) & 0x3f] & P;

	outflank_d7 = ((O | ~0x01020400u) + 1) & P & 0x01020000u;

	return (unsigned char) FLIPPED_3_H[outflank_h]
		| ((outflank_d7 - (unsigned int) (outflank_d7 != 0)) & 0x00020400u)
		| _mm_cvtsi128_si64(outflank_v_d);
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
	unsigned int outflank_h, outflank_d9;
	__m128i	outflank_v_d;
	static const V2DI mask = {{ ~0x1010101010101000, ~0x0000000102040800 }};

	outflank_v_d = _mm_and_si128(_mm_andnot_si128(mask.v2, _mm_sub_epi64(_mm_or_si128(_mm_set1_epi64x(O), mask.v2), minusone.v2)), _mm_set1_epi64x(P));
	outflank_v_d = _mm_andnot_si128(mask.v2, _mm_sub_epi64(outflank_v_d, _mm_sub_epi64(flipmask(outflank_v_d), minusone.v2)));
	outflank_v_d = _mm_or_si128(outflank_v_d, _mm_shuffle_epi32(outflank_v_d, SWAP64));

	outflank_h = OUTFLANK_4[((unsigned int) O >> 1) & 0x3f] & P;

	outflank_d9 = ((O | ~0x80402000u) + 1) & P & 0x80400000u;

	return (unsigned char) FLIPPED_4_H[outflank_h]
		| ((outflank_d9 - (unsigned int) (outflank_d9 != 0)) & 0x00402000u)
		|  _mm_cvtsi128_si64(outflank_v_d);
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
	__m128i	outflank_v_d;
	static const V2DI mask = {{ ~0x2020202020202000, ~0x0000010204081000 }};

	outflank_v_d = _mm_and_si128(_mm_andnot_si128(mask.v2, _mm_sub_epi64(_mm_or_si128(_mm_set1_epi64x(O), mask.v2), minusone.v2)), _mm_set1_epi64x(P));
	outflank_v_d = _mm_andnot_si128(mask.v2, _mm_sub_epi64(outflank_v_d, _mm_sub_epi64(flipmask(outflank_v_d), minusone.v2)));
	outflank_v_d = _mm_or_si128(outflank_v_d, _mm_shuffle_epi32(outflank_v_d, SWAP64));

	outflank_h = OUTFLANK_5[((unsigned int) O >> 1) & 0x3f] & P;

	return (unsigned char) FLIPPED_5_H[outflank_h]
		| ((P >> 9) & 0x0000000000004000ULL & O)
		| _mm_cvtsi128_si64(outflank_v_d);
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
	__m128i	outflank_v_d;
	static const V2DI mask = {{ ~0x4040404040404000, ~0x0001020408102000 }};

	outflank_v_d = _mm_and_si128(_mm_andnot_si128(mask.v2, _mm_sub_epi64(_mm_or_si128(_mm_set1_epi64x(O), mask.v2), minusone.v2)), _mm_set1_epi64x(P));
	outflank_v_d = _mm_andnot_si128(mask.v2, _mm_sub_epi64(outflank_v_d, _mm_sub_epi64(flipmask(outflank_v_d), minusone.v2)));
	outflank_v_d = _mm_or_si128(outflank_v_d, _mm_shuffle_epi32(outflank_v_d, SWAP64));

	outflank_h = OUTFLANK_7[O & 0x3e] & (P << 1);

	return (((-outflank_h) & 0x3e) << 0) | _mm_cvtsi128_si64(outflank_v_d);
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
	__m128i	outflank_v_d;
	static const V2DI mask = {{ ~0x8080808080808000, ~0x0102040810204000 }};

	outflank_v_d = _mm_and_si128(_mm_andnot_si128(mask.v2, _mm_sub_epi64(_mm_or_si128(_mm_set1_epi64x(O), mask.v2), minusone.v2)), _mm_set1_epi64x(P));
	outflank_v_d = _mm_andnot_si128(mask.v2, _mm_sub_epi64(outflank_v_d, _mm_sub_epi64(flipmask(outflank_v_d), minusone.v2)));
	outflank_v_d = _mm_or_si128(outflank_v_d, _mm_shuffle_epi32(outflank_v_d, SWAP64));

	outflank_h = OUTFLANK_7[((unsigned int) O >> 1) & 0x3f] & P;

	return (((-outflank_h) & 0x3f) << 1) | _mm_cvtsi128_si64(outflank_v_d);
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
	__m128i	outflank_v_d;
	static const V2DI mask = {{ ~0x0101010101010000, ~0x4020100804020000 }};

	outflank_v_d = _mm_and_si128(_mm_andnot_si128(mask.v2, _mm_sub_epi64(_mm_or_si128(_mm_set1_epi64x(O), mask.v2), minusone.v2)), _mm_set1_epi64x(P));
	outflank_v_d = _mm_andnot_si128(mask.v2, _mm_sub_epi64(outflank_v_d, _mm_sub_epi64(flipmask(outflank_v_d), minusone.v2)));
	outflank_v_d = _mm_or_si128(outflank_v_d, _mm_shuffle_epi32(outflank_v_d, SWAP64));

	outflank_h = ((O & 0x00007e00u) + 0x00000200u) & P;

	return ((outflank_h - (outflank_h >> 8)) & 0x00007e00u) | _mm_cvtsi128_si64(outflank_v_d);
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
	__m128i	outflank_v_d;
	static const V2DI mask = {{ ~0x0202020202020000, ~0x8040201008040000 }};

	outflank_v_d = _mm_and_si128(_mm_andnot_si128(mask.v2, _mm_sub_epi64(_mm_or_si128(_mm_set1_epi64x(O), mask.v2), minusone.v2)), _mm_set1_epi64x(P));
	outflank_v_d = _mm_andnot_si128(mask.v2, _mm_sub_epi64(outflank_v_d, _mm_sub_epi64(flipmask(outflank_v_d), minusone.v2)));
	outflank_v_d = _mm_or_si128(outflank_v_d, _mm_shuffle_epi32(outflank_v_d, SWAP64));

	outflank_h = ((O & 0x00007c00u) + 0x00000400u) & P;

	return ((outflank_h - (outflank_h >> 8)) & 0x00007c00u) | _mm_cvtsi128_si64(outflank_v_d);
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
	__m128i	outflank_v_d;
	static const V2DI mask = {{ ~0x0404040404040000, ~0x0080402010080000 }};

	outflank_v_d = _mm_and_si128(_mm_andnot_si128(mask.v2, _mm_sub_epi64(_mm_or_si128(_mm_set1_epi64x(O), mask.v2), minusone.v2)), _mm_set1_epi64x(P));
	outflank_v_d = _mm_andnot_si128(mask.v2, _mm_sub_epi64(outflank_v_d, _mm_sub_epi64(flipmask(outflank_v_d), minusone.v2)));
	outflank_v_d = _mm_or_si128(outflank_v_d, _mm_shuffle_epi32(outflank_v_d, SWAP64));

	outflank_h = OUTFLANK_2[((unsigned int) O >> 9) & 0x3f] & ((unsigned int) P >> 8);

	return ((unsigned char) FLIPPED_2_H[outflank_h] << 8)
		| ((P >> 7) & 0x0000000000020000ULL & O)
		| _mm_cvtsi128_si64(outflank_v_d);
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
	unsigned long long outflank_d7;
	__m128i	outflank_v_d;
	static const V2DI mask = {{ ~0x0808080808080000, ~0x0000804020100000 }};

	outflank_v_d = _mm_and_si128(_mm_andnot_si128(mask.v2, _mm_sub_epi64(_mm_or_si128(_mm_set1_epi64x(O), mask.v2), minusone.v2)), _mm_set1_epi64x(P));
	outflank_v_d = _mm_andnot_si128(mask.v2, _mm_sub_epi64(outflank_v_d, _mm_sub_epi64(flipmask(outflank_v_d), minusone.v2)));
	outflank_v_d = _mm_or_si128(outflank_v_d, _mm_shuffle_epi32(outflank_v_d, SWAP64));

	outflank_h = OUTFLANK_3[((unsigned int) O >> 9) & 0x3f] & ((unsigned int) P >> 8);

	outflank_d7 = ((O | ~0x0000000102040000ULL) + 1) & P & 0x0000000102000000ULL;

	return ((unsigned int) FLIPPED_3_H[outflank_h] & 0x0000ff00u)
		| ((outflank_d7 - (unsigned int) (outflank_d7 != 0)) & 0x0000000002040000ULL)
		| _mm_cvtsi128_si64(outflank_v_d);
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
	unsigned long long outflank_d9;
	__m128i	outflank_v_d;
	static const V2DI mask = {{ ~0x1010101010100000, ~0x0000010204080000 }};

	outflank_v_d = _mm_and_si128(_mm_andnot_si128(mask.v2, _mm_sub_epi64(_mm_or_si128(_mm_set1_epi64x(O), mask.v2), minusone.v2)), _mm_set1_epi64x(P));
	outflank_v_d = _mm_andnot_si128(mask.v2, _mm_sub_epi64(outflank_v_d, _mm_sub_epi64(flipmask(outflank_v_d), minusone.v2)));
	outflank_v_d = _mm_or_si128(outflank_v_d, _mm_shuffle_epi32(outflank_v_d, SWAP64));

	outflank_h = OUTFLANK_4[((unsigned int) O >> 9) & 0x3f] & ((unsigned int) P >> 8);

	outflank_d9 = ((O | ~0x0000008040200000ULL) + 1) & P & 0x0000008040000000ULL;

	return ((unsigned int) FLIPPED_4_H[outflank_h] & 0x0000ff00u)
		| ((outflank_d9 - (unsigned int) (outflank_d9 != 0)) & 0x0000000040200000ULL)
		| _mm_cvtsi128_si64(outflank_v_d);
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
	__m128i	outflank_v_d;
	static const V2DI mask = {{ ~0x2020202020200000, ~0x0001020408100000 }};

	outflank_v_d = _mm_and_si128(_mm_andnot_si128(mask.v2, _mm_sub_epi64(_mm_or_si128(_mm_set1_epi64x(O), mask.v2), minusone.v2)), _mm_set1_epi64x(P));
	outflank_v_d = _mm_andnot_si128(mask.v2, _mm_sub_epi64(outflank_v_d, _mm_sub_epi64(flipmask(outflank_v_d), minusone.v2)));
	outflank_v_d = _mm_or_si128(outflank_v_d, _mm_shuffle_epi32(outflank_v_d, SWAP64));

	outflank_h = OUTFLANK_5[((unsigned int) O >> 9) & 0x3f] & ((unsigned int) P >> 8);

	return ((unsigned char) FLIPPED_5_H[outflank_h] << 8)
		| ((P >> 9) & 0x0000000000400000ULL & O)
		| _mm_cvtsi128_si64(outflank_v_d);
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
	__m128i	outflank_v_d;
	static const V2DI mask = {{ ~0x4040404040400000, ~0x0102040810200000 }};

	outflank_v_d = _mm_and_si128(_mm_andnot_si128(mask.v2, _mm_sub_epi64(_mm_or_si128(_mm_set1_epi64x(O), mask.v2), minusone.v2)), _mm_set1_epi64x(P));
	outflank_v_d = _mm_andnot_si128(mask.v2, _mm_sub_epi64(outflank_v_d, _mm_sub_epi64(flipmask(outflank_v_d), minusone.v2)));
	outflank_v_d = _mm_or_si128(outflank_v_d, _mm_shuffle_epi32(outflank_v_d, SWAP64));

	outflank_h = OUTFLANK_7[((unsigned int) O >> 8) & 0x3e] & ((unsigned int) P >> 7);

	return (((-outflank_h) & 0x3e) << 8) | _mm_cvtsi128_si64(outflank_v_d);
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
	__m128i	outflank_v_d;
	static const V2DI mask = {{ ~0x8080808080800000, ~0x0204081020400000 }};

	outflank_v_d = _mm_and_si128(_mm_andnot_si128(mask.v2, _mm_sub_epi64(_mm_or_si128(_mm_set1_epi64x(O), mask.v2), minusone.v2)), _mm_set1_epi64x(P));
	outflank_v_d = _mm_andnot_si128(mask.v2, _mm_sub_epi64(outflank_v_d, _mm_sub_epi64(flipmask(outflank_v_d), minusone.v2)));
	outflank_v_d = _mm_or_si128(outflank_v_d, _mm_shuffle_epi32(outflank_v_d, SWAP64));

	outflank_h = OUTFLANK_7[((unsigned int) O >> 9) & 0x3f] & ((unsigned int) P >> 8);

	return (((-outflank_h) & 0x3f) << 9) | _mm_cvtsi128_si64(outflank_v_d);
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
	unsigned int outflank_h;
	__m128i	outflank_v_d;
	static const V2DI mask = {{ ~0x0101010101000000, ~0x2010080402000000 }};

	outflank_v_d = _mm_and_si128(_mm_andnot_si128(mask.v2, _mm_sub_epi64(_mm_or_si128(_mm_set1_epi64x(O), mask.v2), minusone.v2)), _mm_set1_epi64x(P));
	outflank_v_d = _mm_andnot_si128(mask.v2, _mm_sub_epi64(outflank_v_d, _mm_sub_epi64(flipmask(outflank_v_d), minusone.v2)));
	outflank_v_d = _mm_or_si128(outflank_v_d, _mm_shuffle_epi32(outflank_v_d, SWAP64));

	outflank_h = ((O & 0x007e0000u) + 0x00020000u) & P;

	return ((outflank_h - (outflank_h >> 8)) & 0x007e0000u)
		| (O & (((P << 8) & 0x0000000000000100ULL) | ((P << 7) & 0x0000000000000200ULL)))
		| _mm_cvtsi128_si64(outflank_v_d);
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
	unsigned int outflank_h;
	__m128i	outflank_v_d;
	static const V2DI mask = {{ ~0x0202020202000000, ~0x4020100804000000 }};

	outflank_v_d = _mm_and_si128(_mm_andnot_si128(mask.v2, _mm_sub_epi64(_mm_or_si128(_mm_set1_epi64x(O), mask.v2), minusone.v2)), _mm_set1_epi64x(P));
	outflank_v_d = _mm_andnot_si128(mask.v2, _mm_sub_epi64(outflank_v_d, _mm_sub_epi64(flipmask(outflank_v_d), minusone.v2)));
	outflank_v_d = _mm_or_si128(outflank_v_d, _mm_shuffle_epi32(outflank_v_d, SWAP64));

	outflank_h = ((O & 0x007c0000u) + 0x00040000u) & P;

	return ((outflank_h - (outflank_h >> 8)) & 0x007c0000u)
		| (O & (((P << 8) & 0x0000000000000200ULL) | ((P << 7) & 0x0000000000000400ULL)))
		| _mm_cvtsi128_si64(outflank_v_d);
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
	__m128i	outflank_v_d;
	static const V2DI mask = {{ ~0x0404040404000000, ~0x8040201008000000 }};

	outflank_v_d = _mm_and_si128(_mm_andnot_si128(mask.v2, _mm_sub_epi64(_mm_or_si128(_mm_set1_epi64x(O), mask.v2), minusone.v2)), _mm_set1_epi64x(P));
	outflank_v_d = _mm_andnot_si128(mask.v2, _mm_sub_epi64(outflank_v_d, _mm_sub_epi64(flipmask(outflank_v_d), minusone.v2)));
	outflank_v_d = _mm_or_si128(outflank_v_d, _mm_shuffle_epi32(outflank_v_d, SWAP64));

	outflank_h = OUTFLANK_2[((unsigned int) O >> 17) & 0x3f] & ((unsigned int) P >> 16);

	return ((unsigned char) FLIPPED_2_H[outflank_h] << 16)
		| (O & (((P << 8) & 0x0000000000000400ULL)
			| ((P << 9) & 0x0000000000000200ULL)
			| (((P >> 7) | (P << 7)) & 0x0000000002000800ULL)))
		| _mm_cvtsi128_si64(outflank_v_d);
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
	unsigned long long outflank_d7;
	__m128i	outflank_v_d;
	static const V2DI mask = {{ ~0x0808080808000000, ~0x0080402010000000 }};

	outflank_v_d = _mm_and_si128(_mm_andnot_si128(mask.v2, _mm_sub_epi64(_mm_or_si128(_mm_set1_epi64x(O), mask.v2), minusone.v2)), _mm_set1_epi64x(P));
	outflank_v_d = _mm_andnot_si128(mask.v2, _mm_sub_epi64(outflank_v_d, _mm_sub_epi64(flipmask(outflank_v_d), minusone.v2)));
	outflank_v_d = _mm_or_si128(outflank_v_d, _mm_shuffle_epi32(outflank_v_d, SWAP64));

	outflank_h = OUTFLANK_3[((unsigned int) O >> 17) & 0x3f] & ((unsigned int) P >> 16);

	outflank_d7 = ((O | ~0x0000010204000000ULL) + 1) & P & 0x0000010204000000ULL;

	return ((unsigned int) FLIPPED_3_H[outflank_h] & 0x00ff0000u)
		| (O & (((P << 8) & 0x0000000000000800ULL)
			| ((P << 7) & 0x0000000000001000ULL)
			| ((P << 9) & 0x0000000000000400ULL)))
		| ((outflank_d7 - (outflank_d7 >> 24)) & 0x0000010204000000ULL)
		| _mm_cvtsi128_si64(outflank_v_d);
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
	unsigned long long outflank_d9;
	__m128i	outflank_v_d;
	static const V2DI mask = {{ ~0x1010101010000000, ~0x0001020408000000 }};

	outflank_v_d = _mm_and_si128(_mm_andnot_si128(mask.v2, _mm_sub_epi64(_mm_or_si128(_mm_set1_epi64x(O), mask.v2), minusone.v2)), _mm_set1_epi64x(P));
	outflank_v_d = _mm_andnot_si128(mask.v2, _mm_sub_epi64(outflank_v_d, _mm_sub_epi64(flipmask(outflank_v_d), minusone.v2)));
	outflank_v_d = _mm_or_si128(outflank_v_d, _mm_shuffle_epi32(outflank_v_d, SWAP64));

	outflank_h = OUTFLANK_4[((unsigned int) O >> 17) & 0x3f] & ((unsigned int) P >> 16);

	outflank_d9 = ((O | ~0x0000804020000000ULL) + 1) & P & 0x0000804020000000ULL;

	return ((unsigned int) FLIPPED_4_H[outflank_h] & 0x00ff0000u)
		| (O & (((P << 8) & 0x0000000000001000ULL)
			| ((P << 7) & 0x0000000000002000ULL)
			| ((P << 9) & 0x00000000000000800ULL)))
		| ((outflank_d9 - (outflank_d9 >> 24)) & 0x0000804020000000ULL)
		| _mm_cvtsi128_si64(outflank_v_d);
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
	__m128i	outflank_v_d;
	static const V2DI mask = {{ ~0x2020202020000000, ~0x0102040810000000 }};

	outflank_v_d = _mm_and_si128(_mm_andnot_si128(mask.v2, _mm_sub_epi64(_mm_or_si128(_mm_set1_epi64x(O), mask.v2), minusone.v2)), _mm_set1_epi64x(P));
	outflank_v_d = _mm_andnot_si128(mask.v2, _mm_sub_epi64(outflank_v_d, _mm_sub_epi64(flipmask(outflank_v_d), minusone.v2)));
	outflank_v_d = _mm_or_si128(outflank_v_d, _mm_shuffle_epi32(outflank_v_d, SWAP64));

	outflank_h = OUTFLANK_5[((unsigned int) O >> 17) & 0x3f] & ((unsigned int) P >> 16);

	return ((unsigned char) FLIPPED_5_H[outflank_h] << 16)
		| (O & (((P << 8) & 0x0000000000002000ULL)
			| ((P << 7) & 0x0000000000004000ULL)
			| (((P >> 9) | (P << 9)) & 0x0000000040001000ULL)))
		| _mm_cvtsi128_si64(outflank_v_d);
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
	__m128i	outflank_v_d;
	static const V2DI mask = {{ ~0x4040404040000000, ~0x0204081020000000 }};

	outflank_v_d = _mm_and_si128(_mm_andnot_si128(mask.v2, _mm_sub_epi64(_mm_or_si128(_mm_set1_epi64x(O), mask.v2), minusone.v2)), _mm_set1_epi64x(P));
	outflank_v_d = _mm_andnot_si128(mask.v2, _mm_sub_epi64(outflank_v_d, _mm_sub_epi64(flipmask(outflank_v_d), minusone.v2)));
	outflank_v_d = _mm_or_si128(outflank_v_d, _mm_shuffle_epi32(outflank_v_d, SWAP64));

	outflank_h = OUTFLANK_7[((unsigned int) O >> 16) & 0x3e] & ((unsigned int) P >> 15);

	return (((-outflank_h) & 0x3e) << 16)
		| (O & (((P << 8) & 0x0000000000004000ULL) | ((P << 9) & 0x0000000000002000ULL)))
		| _mm_cvtsi128_si64(outflank_v_d);
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
	__m128i	outflank_v_d;
	static const V2DI mask = {{ ~0x8080808080000000, ~0x0408102040000000 }};

	outflank_v_d = _mm_and_si128(_mm_andnot_si128(mask.v2, _mm_sub_epi64(_mm_or_si128(_mm_set1_epi64x(O), mask.v2), minusone.v2)), _mm_set1_epi64x(P));
	outflank_v_d = _mm_andnot_si128(mask.v2, _mm_sub_epi64(outflank_v_d, _mm_sub_epi64(flipmask(outflank_v_d), minusone.v2)));
	outflank_v_d = _mm_or_si128(outflank_v_d, _mm_shuffle_epi32(outflank_v_d, SWAP64));

	outflank_h = OUTFLANK_7[((unsigned int) O >> 17) & 0x3f] & ((unsigned int) P >> 16);

	return (((-outflank_h) & 0x3f) << 17)
		| (O & (((P << 8) & 0x0000000000008000ULL) | ((P << 9) & 0x0000000000004000ULL)))
		| _mm_cvtsi128_si64(outflank_v_d);
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

	outflank_a1a4e8 = OUTFLANK_3[((O & 0x0008040201010100ULL) * 0x0102040808080808ULL) >> 57]
		& (((P & 0x1008040201010101ULL) * 0x0102040808080808ULL) >> 56);
	flipped = FLIPPED_3_V[outflank_a1a4e8] & 0x0008040201010100ULL;

	outflank_a8a4d1 = OUTFLANK_4[((O & 0x0001010101020400ULL) * 0x1010101008040201ULL) >> 57]
		& (((P & 0x0101010101020408ULL) * 0x1010101008040201ULL) >> 56);
	flipped |= vertical_mirror(FLIPPED_4_V[outflank_a8a4d1]) & 0x0001010101020400ULL;

	outflank_h = ((O & 0x7e000000u) + 0x02000000u) & P;
	flipped |= (outflank_h - (outflank_h >> 8)) & 0x7e000000u;

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

	outflank_b1b4f8 = OUTFLANK_3[((O & 0x0010080402020200ULL) * 0x0081020404040404ULL) >> 57]
		& (((P & 0x2010080402020202ULL) * 0x0081020404040404ULL) >> 56);
	flipped = FLIPPED_3_V[outflank_b1b4f8] & 0x0010080402020200ULL;

	outflank_b8b4e1 = OUTFLANK_4[((O & 0x0002020202040800ULL) * 0x1010101008040201ULL) >> 58]
		& ((((P & 0x0202020202040810ULL) >> 1) * 0x1010101008040201ULL) >> 56);
	flipped |= vertical_mirror(FLIPPED_4_V[outflank_b8b4e1]) & 0x0002020202040800ULL;

	outflank_h = ((O & 0x7c000000u) + 0x04000000u) & P;
	flipped |= (outflank_h - (outflank_h >> 8)) & 0x7c000000u;

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

	outflank_c1c4g8 = OUTFLANK_3[((O & 0x0020100804040400ULL) * 0x0040810202020202ULL) >> 57]
		& (((P & 0x4020100804040404ULL) * 0x0040810202020202ULL) >> 56);
	flipped = FLIPPED_3_V[outflank_c1c4g8] & 0x0020100804040400ULL;

	outflank_c8c4f1 = OUTFLANK_4[((O & 0x0004040404081000ULL) * 0x0404040402010080ULL) >> 57]
		& ((((P & 0x0404040404081020ULL) >> 2) * 0x1010101008040201ULL) >> 56);
	flipped |= vertical_mirror(FLIPPED_4_V[outflank_c8c4f1]) & 0x0004040404081000ULL;

	outflank_h = OUTFLANK_2[(O >> 25) & 0x3f] & (P >> 24);
	flipped |= FLIPPED_2_H[outflank_h] & 0x00000000ff000000ULL;

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

	outflank_v = OUTFLANK_3[((O & 0x0008080808080800ULL) * 0x0020408102040810ULL) >> 57]
		& (((P & 0x0808080808080808ULL) * 0x0020408102040810ULL) >> 56);
	flipped = FLIPPED_3_V[outflank_v] & 0x0008080808080800ULL;

	outflank_h = OUTFLANK_3[(O >> 25) & 0x3f] & (P >> 24);
	flipped |= FLIPPED_3_H[outflank_h] & 0x00000000ff000000ULL;

	outflank_d7 = OUTFLANK_3[((O & 0x0000020408102000ULL) * 0x0101010101010101ULL) >> 57]
		& (((P & 0x0001020408102040ULL) * 0x0101010101010101ULL) >> 56);
	flipped |= FLIPPED_3_H[outflank_d7] & 0x0000020408102000ULL;

	outflank_d9 = OUTFLANK_3[((O & 0x0040201008040200ULL) * 0x0101010101010101ULL) >> 57]
		& (((P & 0x8040201008040201ULL) * 0x0101010101010101ULL) >> 56);
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

	outflank_v = OUTFLANK_3[((O & 0x0010101010101000ULL) * 0x0010204081020408ULL) >> 57]
		& (((P & 0x1010101010101010ULL) * 0x0010204081020408ULL) >> 56);
	flipped = FLIPPED_3_V[outflank_v] & 0x0010101010101000ULL;

	outflank_h = OUTFLANK_4[(O >> 25) & 0x3f] & (P >> 24);
	flipped |= FLIPPED_4_H[outflank_h] & 0x00000000ff000000ULL;

	outflank_d7 = OUTFLANK_4[((O & 0x0002040810204000ULL) * 0x0101010101010101ULL) >> 57]
		& (((P & 0x0102040810204080ULL) * 0x0101010101010101ULL) >> 56);
	flipped |= FLIPPED_4_H[outflank_d7] & 0x0002040810204000ULL;

	outflank_d9 = OUTFLANK_4[((O & 0x0000402010080400ULL) * 0x0101010101010101ULL) >> 57]
		& (((P & 0x0080402010080402ULL) * 0x0101010101010101ULL) >> 56);
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
	unsigned int outflank_h, outflank_c1f4f8, outflank_b8f4f1;
	unsigned long long flipped;

	outflank_c1f4f8 = OUTFLANK_3[((O & 0x0020202020100800ULL) * 0x0040404040810204ULL) >> 57]
		& (((P & 0x2020202020100804ULL) * 0x0040404040810204ULL) >> 56);
	flipped = FLIPPED_3_V[outflank_c1f4f8] & 0x0020202020100800ULL;

	outflank_b8f4f1 = OUTFLANK_4[((O & 0x0004081020202000ULL) * 0x0804020101010101ULL) >> 58]
		& ((((P & 0x0204081020202020ULL) >> 1) * 0x0804020101010101ULL) >> 56);
	flipped |= vertical_mirror(FLIPPED_4_V[outflank_b8f4f1]) & 0x0004081020202000ULL;

	outflank_h = OUTFLANK_5[(O >> 25) & 0x3f] & (P >> 24);
	flipped |= FLIPPED_5_H[outflank_h] & 0x00000000ff000000ULL;

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
	unsigned int outflank_h, outflank_d1g4g8, outflank_c8g4g1;
	unsigned long long flipped;

	outflank_d1g4g8 = OUTFLANK_3[((O & 0x0040404040201000ULL) * 0x0020202020408102ULL) >> 57]
		& (((P & 0x4040404040201008ULL) * 0x0020202020408102ULL) >> 56);
	flipped = FLIPPED_3_V[outflank_d1g4g8] & 0x0040404040201000ULL;

	outflank_c8g4g1 = OUTFLANK_4[((O & 0x0008102040404000ULL) * 0x0001008040404040ULL) >> 57]
		& ((((P & 0x0408102040404040ULL) >> 2) * 0x0804020101010101ULL) >> 56);
	flipped |= vertical_mirror(FLIPPED_4_V[outflank_c8g4g1]) & 0x0008102040404000ULL;

	outflank_h = OUTFLANK_7[(O >> 24) & 0x3e] & (P >> 23);
	flipped |= ((-outflank_h) & 0x3e) << 24;

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

	outflank_e1h4h8 = OUTFLANK_3[((O & 0x0080808080402000ULL) * 0x0010101010204081ULL) >> 57]
		& (((P & 0x8080808080402010ULL) * 0x0010101010204081ULL) >> 56);
	flipped = FLIPPED_3_V[outflank_e1h4h8] & 0x0080808080402000ULL;

	outflank_d8h4h1 = OUTFLANK_4[((O & 0x0010204080808000ULL) * 0x0000804020202020ULL) >> 57]
		& ((((P & 0x0810204080808080ULL) >> 3) * 0x0804020101010101ULL) >> 56);
	flipped |= vertical_mirror(FLIPPED_4_V[outflank_d8h4h1]) & 0x0010204080808000ULL;

	outflank_h = OUTFLANK_7[(O >> 25) & 0x3f] & (P >> 24);
	flipped |= ((-outflank_h) & 0x3f) << 25;

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

	outflank_a1a5d8 = OUTFLANK_4[((O & 0x0004020101010100ULL) * 0x0102040810101010ULL) >> 57]
		& (((P & 0x0804020101010101ULL) * 0x0102040810101010ULL) >> 56);
	flipped = FLIPPED_4_V[outflank_a1a5d8] & 0x0004020101010100ULL;

	outflank_a8a5e1 = OUTFLANK_3[((O & 0x0001010102040800ULL) * 0x0808080808040201ULL) >> 57]
		& (((P & 0x0101010102040810ULL) * 0x0808080808040201ULL) >> 56);
	flipped |= vertical_mirror(FLIPPED_3_V[outflank_a8a5e1]) & 0x0001010102040800ULL;

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

	outflank_b1b5e8 = OUTFLANK_4[((O & 0x0008040202020200ULL) * 0x0081020408080808ULL) >> 57]
		& (((P & 0x1008040202020202ULL) * 0x0081020408080808ULL) >> 56);
	flipped = FLIPPED_4_V[outflank_b1b5e8] & 0x0008040202020200ULL;

	outflank_b8b5f1 = OUTFLANK_3[((O & 0x0002020204081000ULL) * 0x0808080808040201ULL) >> 58]
		& ((((P & 0x0202020204081020ULL) >> 1) * 0x0808080808040201ULL) >> 56);
	flipped |= vertical_mirror(FLIPPED_3_V[outflank_b8b5f1]) & 0x0002020204081000ULL;

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

	outflank_c1c5f8 = OUTFLANK_4[((O & 0x0010080404040400ULL) * 0x0040810204040404ULL) >> 57]
		& (((P & 0x2010080404040404ULL) * 0x0040810204040404ULL) >> 56);
	flipped = FLIPPED_4_V[outflank_c1c5f8] & 0x0010080404040400ULL;

	outflank_c8c5g1 = OUTFLANK_3[((O & 0x0004040408102000ULL) * 0x0002020202010080ULL) >> 57]
		& ((((P & 0x0404040408102040ULL) >> 2) * 0x0808080808040201ULL) >> 56);
	flipped |= vertical_mirror(FLIPPED_3_V[outflank_c8c5g1]) & 0x0004040408102000ULL;

	outflank_h = OUTFLANK_2[(O >> 33) & 0x3f] & (P >> 32);
	flipped |= FLIPPED_2_H[outflank_h] & 0x000000ff00000000ULL;

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

	outflank_v = OUTFLANK_4[((O & 0x0008080808080800ULL) * 0x0020408102040810ULL) >> 57]
		& (((P & 0x0808080808080808ULL) * 0x0020408102040810ULL) >> 56);
	flipped = FLIPPED_4_V[outflank_v] & 0x0008080808080800ULL;

	outflank_h = OUTFLANK_3[(O >> 33) & 0x3f] & (P >> 32);
	flipped |= FLIPPED_3_H[outflank_h] & 0x000000ff00000000ULL;

	outflank_d7 = OUTFLANK_3[((O & 0x0002040810204000ULL) * 0x0101010101010101ULL) >> 57]
		& (((P & 0x0102040810204080ULL) * 0x0101010101010101ULL) >> 56);
	flipped |= FLIPPED_3_H[outflank_d7] & 0x0002040810204000ULL;

	outflank_d9 = OUTFLANK_3[((O & 0x0020100804020000ULL) * 0x0101010101010101ULL) >> 57]
		& (((P & 0x4020100804020100ULL) * 0x0101010101010101ULL) >> 56);
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

	outflank_v = OUTFLANK_4[((O & 0x0010101010101000ULL) * 0x0010204081020408ULL) >> 57]
		& (((P & 0x1010101010101010ULL) * 0x0010204081020408ULL) >> 56);
	flipped = FLIPPED_4_V[outflank_v] & 0x0010101010101000ULL;

	outflank_h = OUTFLANK_4[(O >> 33) & 0x3f] & (P >> 32);
	flipped |= FLIPPED_4_H[outflank_h] & 0x000000ff00000000ULL;

	outflank_d7 = OUTFLANK_4[((O & 0x0004081020400000ULL) * 0x0101010101010101ULL) >> 57]
		& (((P & 0x0204081020408000ULL) * 0x0101010101010101ULL) >> 56);
	flipped |= FLIPPED_4_H[outflank_d7] & 0x0004081020400000ULL;

	outflank_d9 = OUTFLANK_4[((O & 0x0040201008040200ULL) * 0x0101010101010101ULL) >> 57]
		& (((P & 0x8040201008040201ULL) * 0x0101010101010101ULL) >> 56);
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

	outflank_b1f5f8 = OUTFLANK_4[((O & 0x0020202010080400ULL) * 0x0080808080810204ULL) >> 57]
		& (((P & 0x2020202010080402ULL) * 0x0080808080810204ULL) >> 56);
	flipped = FLIPPED_4_V[outflank_b1f5f8] & 0x0020202010080400ULL;

	outflank_c8f5f1 = OUTFLANK_3[((O & 0x0008102020202000ULL) * 0x0002010080404040ULL) >> 57]
		& ((((P & 0x0408102020202020ULL) >> 2) * 0x1008040201010101ULL) >> 56);
	flipped |= vertical_mirror(FLIPPED_3_V[outflank_c8f5f1]) & 0x0008102020202000ULL;

	outflank_h = OUTFLANK_5[(O >> 33) & 0x3f] & (P >> 32);
	flipped |= FLIPPED_5_H[outflank_h] & 0x000000ff00000000ULL;

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

	outflank_c1g5g8 = OUTFLANK_4[((O & 0x0040404020100800ULL) * 0x0040404040408102ULL) >> 57]
		& (((P & 0x4040404020100804ULL) * 0x0040404040408102ULL) >> 56);
	flipped = FLIPPED_4_V[outflank_c1g5g8] & 0x0040404020100800ULL;

	outflank_d8g5g1 = OUTFLANK_3[((O & 0x0010204040404000ULL) * 0x0001008040202020ULL) >> 57]
		& ((((P & 0x0810204040404040ULL) >> 3) * 0x1008040201010101ULL) >> 56);
	flipped |= vertical_mirror(FLIPPED_3_V[outflank_d8g5g1]) & 0x0010204040404000ULL;

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

	outflank_d1h5h8 = OUTFLANK_4[((O & 0x0080808040201000ULL) * 0x0020202020204081ULL) >> 57]
		& (((P & 0x8080808040201008ULL) * 0x0020202020204081ULL) >> 56);
	flipped = FLIPPED_4_V[outflank_d1h5h8] & 0x0080808040201000ULL;

	outflank_e8h5h1 = OUTFLANK_3[((O & 0x0020408080808000ULL) * 0x0000804020101010ULL) >> 57]
		& ((((P & 0x1020408080808080ULL) >> 4) * 0x1008040201010101ULL) >> 56);
	flipped |= vertical_mirror(FLIPPED_3_V[outflank_e8h5h1]) & 0x0020408080808000ULL;

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
	unsigned int outflank_a1a6c8, outflank_a8a6f1;
	unsigned long long flipped, outflank_h;

	outflank_a1a6c8 = OUTFLANK_5[((O & 0x0002010101010100ULL) * 0x0102040810202020ULL) >> 57]
		& (((P & 0x0402010101010101ULL) * 0x0102040810202020ULL) >> 56);
	flipped = FLIPPED_5_V[outflank_a1a6c8] & 0x0002010101010100ULL;

	outflank_a8a6f1 = OUTFLANK_2[((O & 0x0001010204081000ULL) * 0x0404040404040201ULL) >> 57]
		& (((P & 0x0101010204081020ULL) * 0x0404040404040201ULL) >> 56);
	flipped |= vertical_mirror(FLIPPED_2_V[outflank_a8a6f1]) & 0x0001010204081000ULL;

	outflank_h = ((O & 0x00007e0000000000ULL) + 0x0000020000000000ULL) & P;
	flipped |= (outflank_h - (outflank_h >> 8)) & 0x00007e0000000000ULL;

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
	unsigned int outflank_a1a6c8, outflank_a8a6f1;
	unsigned long long flipped, outflank_h;

	outflank_a1a6c8 = OUTFLANK_5[((O & 0x0004020202020200ULL) * 0x0081020408101010ULL) >> 57]
		& (((P & 0x0804020202020202ULL) * 0x0081020408101010ULL) >> 56);
	flipped = FLIPPED_5_V[outflank_a1a6c8] & 0x0004020202020200ULL;

	outflank_a8a6f1 = OUTFLANK_2[((O & 0x0002020408102000ULL) * 0x0404040404040201ULL) >> 58]
		& ((((P & 0x0202020408102040ULL) >> 1) * 0x0404040404040201ULL) >> 56);
	flipped |= vertical_mirror(FLIPPED_2_V[outflank_a8a6f1]) & 0x0002020408102000ULL;

	outflank_h = ((O & 0x00007c0000000000ULL) + 0x0000040000000000ULL) & P;
	flipped |= (outflank_h - (outflank_h >> 8)) & 0x00007c0000000000ULL;

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

	outflank_v = OUTFLANK_5[((O & 0x0004040404040400ULL) * 0x0040810204081020ULL) >> 57]
		& (((P & 0x0404040404040404ULL) * 0x0040810204081020ULL) >> 56);
	flipped = FLIPPED_5_V[outflank_v] & 0x0004040404040400ULL;

	outflank_h = OUTFLANK_2[(O >> 41) & 0x3f] & (P >> 40);
	flipped |= FLIPPED_2_H[outflank_h] & 0x0000ff0000000000ULL;

	outflank_d7 = OUTFLANK_2[((O & 0x0002040810204000ULL) * 0x0101010101010101ULL) >> 57]
		& (((P & 0x0102040810204080ULL) * 0x0101010101010101ULL) >> 56);
	flipped |= FLIPPED_2_H[outflank_d7] & 0x0002040810204000ULL;

	flipped |= (((P >> 9) | (P << 9)) & 0x0008000200000000ULL) & O;

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

	outflank_v = OUTFLANK_5[((O & 0x0008080808080800ULL) * 0x0020408102040810ULL) >> 57]
		& (((P & 0x0808080808080808ULL) * 0x0020408102040810ULL) >> 56);
	flipped = FLIPPED_5_V[outflank_v] & 0x0008080808080800ULL;

	outflank_h = OUTFLANK_3[(O >> 41) & 0x3f] & (P >> 40);
	flipped |= FLIPPED_3_H[outflank_h] & 0x0000ff0000000000ULL;

	outflank_d = OUTFLANK_3[((O & 0x0000081422400000ULL) * 0x0101010101010101ULL) >> 57]
		& (((P & 0x0000081422418000ULL) * 0x0101010101010101ULL) >> 56);
	flipped |= FLIPPED_3_H[outflank_d] & 0x0000081422400000ULL;	// A3D6H2

	flipped |= (((P >> 9) & 0x0010000000000000ULL) | ((P >> 7) & 0x0004000000000000ULL)) & O;

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

	outflank_v = OUTFLANK_5[((O & 0x0010101010101000ULL) * 0x0010204081020408ULL) >> 57]
		& (((P & 0x1010101010101010ULL) * 0x0010204081020408ULL) >> 56);
	flipped = FLIPPED_5_V[outflank_v] & 0x0010101010101000ULL;

	outflank_h = OUTFLANK_4[(O >> 41) & 0x3f] & (P >> 40);
	flipped |= FLIPPED_4_H[outflank_h] & 0x0000ff0000000000ULL;

	outflank_d = OUTFLANK_4[((O & 0x0000102844020000ULL) * 0x0101010101010101ULL) >> 57]
		& (((P & 0x0000102844820100ULL) * 0x0101010101010101ULL) >> 56);
	flipped |= FLIPPED_4_H[outflank_d] & 0x0000102844020000ULL;	// A2E6H3

	flipped |= (((P >> 9) & 0x0020000000000000ULL) | ((P >> 7) & 0x0008000000000000ULL)) & O;

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

	outflank_v = OUTFLANK_5[((O & 0x0020202020202000ULL) * 0x0008102040810204ULL) >> 57]
		& (((P & 0x2020202020202020ULL) * 0x0008102040810204ULL) >> 56);
	flipped = FLIPPED_5_V[outflank_v] & 0x0020202020202000ULL;

	outflank_h = OUTFLANK_5[(O >> 41) & 0x3f] & (P >> 40);
	flipped |= FLIPPED_5_H[outflank_h] & 0x0000ff0000000000ULL;

	outflank_d9 = OUTFLANK_5[((O & 0x0040201008040200ULL) * 0x0101010101010101ULL) >> 57]
		& (((P & 0x8040201008040201ULL) * 0x0101010101010101ULL) >> 56);
	flipped |= FLIPPED_5_H[outflank_d9] & 0x0040201008040200ULL;

	flipped |= (((P >> 7) | (P << 7)) & 0x0010004000000000ULL) & O;

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

	outflank_b1g6g8 = OUTFLANK_5[((O & 0x0040402010080400ULL) * 0x0080808080808102ULL) >> 57]
		& (((P & 0x4040402010080402ULL) * 0x0080808080808102ULL) >> 56);
	flipped = FLIPPED_5_V[outflank_b1g6g8] & 0x0040402010080400ULL;

	outflank_e8g6g1 = OUTFLANK_2[((O & 0x0020404040404000ULL) * 0x0001008040201010ULL) >> 57]
		& ((((P & 0x1020404040404040ULL) >> 4) * 0x2010080402010101ULL) >> 56);
	flipped |= vertical_mirror(FLIPPED_2_V[outflank_e8g6g1]) & 0x0020404040404000ULL;

	outflank_h = OUTFLANK_7[(O >> 40) & 0x3e] & (P >> 39);
	flipped |= (unsigned long long) ((-outflank_h) & 0x3e) << 40;

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

	outflank_c1h6h8 = OUTFLANK_5[((O & 0x0080804020100800ULL) * 0x0040404040404081ULL) >> 57]
		& (((P & 0x8080804020100804ULL) * 0x0040404040404081ULL) >> 56);
	flipped = FLIPPED_5_V[outflank_c1h6h8] & 0x0080804020100800ULL;

	outflank_f8h6h1 = OUTFLANK_2[((O & 0x0040808080808000ULL) * 0x0000804020100808ULL) >> 57]
		& ((((P & 0x2040808080808080ULL) >> 5) * 0x2010080402010101ULL) >> 56);
	flipped |= vertical_mirror(FLIPPED_2_V[outflank_f8h6h1]) & 0x0040808080808000ULL;

	outflank_h = OUTFLANK_7[(O >> 41) & 0x3f] & (P >> 40);
	flipped |= (unsigned long long) ((-outflank_h) & 0x3f) << 41;

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

	outflank_v = outflank_right(O, 0x0000010101010101ULL, 0x0000010101010100ULL) & P;
	flipped  = (-outflank_v * 2) & 0x0000010101010101ULL;

	outflank_h = ((O & 0x007e000000000000ULL) + 0x0002000000000000ULL) & P;
	flipped |= (outflank_h - (outflank_h >> 8)) & 0x007e000000000000ULL;

	outflank_d7 = outflank_right(O, 0x0000020408102040ULL, 0x0000020408102000ULL) & P;
	flipped |= (-outflank_d7 * 2) & 0x0000020408102040ULL;

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

	outflank_v = outflank_right(O, 0x0000020202020202ULL, 0x0000020202020200ULL) & P;
	flipped  = (-outflank_v * 2) & 0x0000020202020202ULL;

	outflank_h = ((O & 0x007c000000000000ULL) + 0x0004000000000000ULL) & P;
	flipped |= (outflank_h - (outflank_h >> 8)) & 0x007c000000000000ULL;

	outflank_d7 = outflank_right(O, 0x0000040810204080ULL, 0x0000040810204000ULL) & P;
	flipped |= (-outflank_d7 * 2) & 0x0000040810204080ULL;

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

	outflank_v = outflank_right(O, 0x0000040404040404ULL, 0x0000040404040400ULL) & P;
	flipped  = (-outflank_v * 2) & 0x0000040404040404ULL;

	outflank_h = OUTFLANK_2[(O >> 49) & 0x3f] & (P >> 48);
	flipped |= FLIPPED_2_H[outflank_h] & 0x00ff000000000000ULL;

	outflank_d = OUTFLANK_2[((O & 0x00040a1020400000ULL) * 0x0101010101010101ULL) >> 57]
		& (((P & 0x00040a1120408000ULL) * 0x0101010101010101ULL) >> 56);
	flipped |= FLIPPED_2_H[outflank_d] & 0x00040a1020400000ULL;	// A5C7H2

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

	outflank_v = outflank_right(O, 0x0000080808080808ULL, 0x0000080808080800ULL) & P;
	flipped  = (-outflank_v * 2) & 0x0000080808080808ULL;

	outflank_h = OUTFLANK_3[(O >> 49) & 0x3f] & (P >> 48);
	flipped |= FLIPPED_3_H[outflank_h] & 0x00ff000000000000ULL;

	outflank_d = OUTFLANK_3[((O & 0x0008142240000000ULL) * 0x0101010101010101ULL) >> 57]
		& (((P & 0x0008142241800000ULL) * 0x0101010101010101ULL) >> 56);
	flipped |= FLIPPED_3_H[outflank_d] & 0x0008142240000000ULL;	// A4D7H3

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

	outflank_v = outflank_right(O, 0x0000101010101010ULL, 0x0000101010101000ULL) & P;
	flipped  = (-outflank_v * 2) & 0x0000101010101010ULL;

	outflank_h = OUTFLANK_4[(O >> 49) & 0x3f] & (P >> 48);
	flipped |= FLIPPED_4_H[outflank_h] & 0x00ff000000000000ULL;

	outflank_d = OUTFLANK_4[((O & 0x0010284402000000ULL) * 0x0101010101010101ULL) >> 57]
		& (((P & 0x0010284482010000ULL) * 0x0101010101010101ULL) >> 56);
	flipped |= FLIPPED_4_H[outflank_d] & 0x0010284402000000ULL;	// A3E7H4

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

	outflank_v = outflank_right(O, 0x0000202020202020ULL, 0x0000202020202000ULL) & P;
	flipped  = (-outflank_v * 2) & 0x0000202020202020ULL;

	outflank_h = OUTFLANK_5[(O >> 49) & 0x3f] & (P >> 48);
	flipped |= FLIPPED_5_H[outflank_h] & 0x00ff000000000000ULL;

	outflank_d = OUTFLANK_5[((O & 0x0020500804020000ULL) * 0x0101010101010101ULL) >> 57]
		& (((P & 0x0020508804020100ULL) * 0x0101010101010101ULL) >> 56);
	flipped |= FLIPPED_5_H[outflank_d] & 0x0020500804020000ULL;	// A2F7H5

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

	outflank_v = outflank_right(O, 0x0000404040404040ULL, 0x0000404040404000ULL) & P;
	flipped  = (-outflank_v * 2) & 0x0000404040404040ULL;

	outflank_h = OUTFLANK_7[(O >> 48) & 0x3e] & (P >> 47);
	flipped |= (unsigned long long) ((-outflank_h) & 0x3e) << 48;

	outflank_d9 = outflank_right(O, 0x0000201008040201ULL, 0x0000201008040200ULL) & P;
	flipped |= (-outflank_d9 * 2) & 0x0000201008040201ULL;

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

	outflank_v = outflank_right(O, 0x0000808080808080ULL, 0x0000808080808000ULL) & P;
	flipped  = (-outflank_v * 2) & 0x0000808080808080ULL;

	outflank_h = OUTFLANK_7[(O >> 49) & 0x3f] & (P >> 48);
	flipped |= (unsigned long long) ((-outflank_h) & 0x3f) << 49;

	outflank_d9 = outflank_right(O, 0x0000402010080402ULL, 0x0000402010080400ULL) & P;
	flipped |= (-outflank_d9 * 2) & 0x0000402010080402ULL;

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

	outflank_v = outflank_right(O, 0x0001010101010101ULL, 0x0001010101010100ULL) & P;
	flipped  = (-outflank_v * 2) & 0x0001010101010101ULL;

	outflank_h = ((O & 0x7e00000000000000ULL) + 0x0200000000000000ULL) & P;
	flipped |= (outflank_h - (outflank_h >> 8)) & 0x7e00000000000000ULL;

	outflank_d7 = outflank_right(O, 0x0002040810204080ULL, 0x0002040810204000ULL) & P;
	flipped |= (-outflank_d7 * 2) & 0x0002040810204080ULL;

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

	outflank_v = outflank_right(O, 0x0002020202020202ULL, 0x0002020202020200ULL) & P;
	flipped  = (-outflank_v * 2) & 0x0002020202020202ULL;

	outflank_h = ((O & 0x7c00000000000000ULL) + 0x0400000000000000ULL) & P;
	flipped |= (outflank_h - (outflank_h >> 8)) & 0x7c00000000000000ULL;

	outflank_d7 = outflank_right(O, 0x0004081020408000ULL, 0x0004081020400000ULL) & P;
	flipped |= (-outflank_d7 * 2) & 0x0004081020408000ULL;

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
	unsigned long long flipped, outflank_d, outflank_v;

	outflank_v = outflank_right(O, 0x0004040404040404ULL, 0x0004040404040400ULL) & P;
	flipped  = (-outflank_v * 2) & 0x0004040404040404ULL;

	outflank_h = OUTFLANK_2[(O >> 57) & 0x3f] & (P >> 56);
	flipped |= FLIPPED_2_H[outflank_h] & 0xff00000000000000ULL;

	outflank_d = OUTFLANK_2[((O & 0x040a102040000000ULL) * 0x0101010101010101ULL) >> 57]
		& (((P & 0x040a112040800000ULL) * 0x0101010101010101ULL) >> 56);
	flipped |= FLIPPED_2_H[outflank_d] & 0x040a102040000000ULL;	// A6C8H3

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

	outflank_v = outflank_right(O, 0x0008080808080808ULL, 0x0008080808080800ULL) & P;
	flipped  = (-outflank_v * 2) & 0x0008080808080808ULL;

	outflank_h = OUTFLANK_3[(O >> 57) & 0x3f] & (P >> 56);
	flipped |= FLIPPED_3_H[outflank_h] & 0xff00000000000000ULL;

	outflank_d = OUTFLANK_3[((O & 0x0814224000000000ULL) * 0x0101010101010101ULL) >> 57]
		& (((P & 0x0814224180000000ULL) * 0x0101010101010101ULL) >> 56);
	flipped |= FLIPPED_3_H[outflank_d] & 0x0814224000000000ULL;	// A5D8H4

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

	outflank_v = outflank_right(O, 0x0010101010101010ULL, 0x0010101010101000ULL) & P;
	flipped  = (-outflank_v * 2) & 0x0010101010101010ULL;

	outflank_h = OUTFLANK_4[(O >> 57) & 0x3f] & (P >> 56);
	flipped |= FLIPPED_4_H[outflank_h] & 0xff00000000000000ULL;

	outflank_d = OUTFLANK_4[((O & 0x1028440200000000ULL) * 0x0101010101010101ULL) >> 57]
		& (((P & 0x1028448201000000ULL) * 0x0101010101010101ULL) >> 56);
	flipped |= FLIPPED_4_H[outflank_d] & 0x1028440200000000ULL;	// A4E8H5

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

	outflank_v = outflank_right(O, 0x0020202020202020ULL, 0x0020202020202000ULL) & P;
	flipped  = (-outflank_v * 2) & 0x0020202020202020ULL;

	outflank_h = OUTFLANK_5[(O >> 57) & 0x3f] & (P >> 56);
	flipped |= FLIPPED_5_H[outflank_h] & 0xff00000000000000ULL;

	outflank_d = OUTFLANK_5[((O & 0x2050080402000000ULL) * 0x0101010101010101ULL) >> 57]
		& (((P & 0x2050880402010000ULL) * 0x0101010101010101ULL) >> 56);
	flipped |= FLIPPED_5_H[outflank_d] & 0x2050080402000000ULL;	// A3F8H6

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

	outflank_v = outflank_right(O, 0x0040404040404040ULL, 0x0040404040404000ULL) & P;
	flipped  = (-outflank_v * 2) & 0x0040404040404040ULL;

	outflank_h = OUTFLANK_7[(O >> 56) & 0x3e] & (P >> 55);
	flipped |= (unsigned long long) ((-outflank_h) & 0x3e) << 56;

	outflank_d9 = outflank_right(O, 0x0020100804020100ULL, 0x0020100804020000ULL) & P;
	flipped |= (-outflank_d9 * 2) & 0x0020100804020100ULL;

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

	outflank_v = outflank_right(O, 0x0080808080808080ULL, 0x0080808080808000ULL) & P;
	flipped  = (-outflank_v * 2) & 0x0080808080808080ULL;

	outflank_h = OUTFLANK_7[(O >> 57) & 0x3f] & (P >> 56);
	flipped |= (unsigned long long) ((-outflank_h) & 0x3f) << 57;

	outflank_d9 = outflank_right(O, 0x0040201008040201ULL, 0x0040201008040200ULL) & P;
	flipped |= (-outflank_d9 * 2) & 0x0040201008040201ULL;

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

>>>>>>> b3f048d (copyright changes)
