/**
 * @file endgame_sse.c
 *
 *
 * SSE / AVX optimized version of endgame.c for the last four empties.
 *
 * Bitboard and empty list are kept in SSE registers.
 *
 * @date 1998 - 2024
 * @author Richard Delorme
 * @author Toshihiko Okuhara
 * @version 4.5
 * 
 */

#include "bit.h"
#include "settings.h"
#include "search.h"
#include <stdint.h>
#include <assert.h>

#define	SWAP64	0x4e	// for _mm_shuffle_epi32
#define	DUPLO	0x44
#define	DUPHI	0xee

#if defined(__AVX__) && (defined(__x86_64__) || defined(_M_X64))
	#define	EXTRACT_O(OP)	_mm_extract_epi64(OP, 1)
#else
	#define	EXTRACT_O(OP)	_mm_cvtsi128_si64(_mm_shuffle_epi32(OP, DUPHI))
#endif

#if defined(__AVX__) || defined(__SSE4_1__)
	static inline int vectorcall TESTZ_FLIP(__m128i X) { return _mm_testz_si128(X, X); }
#elif defined(__x86_64__) || defined(_M_X64)
	#define TESTZ_FLIP(X)	(!_mm_cvtsi128_si64(X))
#else
	static inline int vectorcall TESTZ_FLIP(__m128i X) { return !_mm_cvtsi128_si32(_mm_packs_epi16(X, X)); }
#endif

#if defined(__AVX512VL__) || defined(__AVX10_1__)
	#define	TEST_EPI8_MASK32(X,Y)	_cvtmask32_u32(_mm256_test_epi8_mask((X), (Y)))
	#define	TEST1_EPI8_MASK32(X)	_cvtmask32_u32(_mm256_test_epi8_mask((X), (X)))
	#define	TEST_EPI8_MASK16(X,Y)	_cvtmask16_u32(_mm_test_epi8_mask((X), (Y)))
#else	// AVX2
	#define	TEST_EPI8_MASK32(X,Y)	_mm256_movemask_epi8(_mm256_sub_epi8(_mm256_setzero_si256(), _mm256_and_si256((X),(Y))))
	#define	TEST1_EPI8_MASK32(X)	_mm256_movemask_epi8(_mm256_sub_epi8(_mm256_setzero_si256(), (X)))
	#define	TEST_EPI8_MASK16(X,Y)	_mm_movemask_epi8(_mm_sub_epi8(_mm_setzero_si128(), _mm_and_si128((X),(Y))))
#endif

// in count_last_flip_sse.c
extern const uint8_t COUNT_FLIP[8][256];
extern const V4DI mask_dvhd[64];

/**
 * @brief Compute a board resulting of a move played on a previous board.
 *
 * @param OP board to play the move on.
 * @param x move to play.
 * @param flipped flipped returned from mm_Flip.
 * @return resulting board.
 */
static inline __m128i vectorcall board_flip_next(__m128i OP, int x, __m128i flipped)
{
	OP = _mm_xor_si128(OP, _mm_or_si128(reduce_vflip(flipped), _mm_loadl_epi64((__m128i *) &X_TO_BIT[x])));
	return _mm_shuffle_epi32(OP, SWAP64);
}

/**
 * @brief Get the final score.
 *
 * Get the final score, when 1 empty square remain.
 * The original code has been adapted from Zebra by Gunnar Anderson.
 *
 * @param PO     Board to evaluate. (O ignored)
 * @param alpha  Alpha bound. (beta - 1)
 * @param pos    Last empty square to play.
 * @return       The final score, as a disc difference.
 */
#if LAST_FLIP_COUNTER == COUNT_LAST_FLIP_BMI2
// PEXT count last flip (2.60s on skylake, 2.35 on icelake, 2.16s on Zen4), very slow on Zen1/2
extern const unsigned long long mask_x[64][4];

static inline int vectorcall board_score_sse_1(__m128i OP, const int alpha, const int pos)
{
	uint_fast8_t	n_flips;
	unsigned int	th, tv;
	unsigned long long P = _mm_cvtsi128_si64(OP);
	unsigned long long mP;
	int	score, score2;
	const uint8_t *COUNT_FLIP_X = COUNT_FLIP[pos & 7];
	const uint8_t *COUNT_FLIP_Y = COUNT_FLIP[pos >> 3];

	mP = P & mask_x[pos][3];	// mask out unrelated bits to make dummy 0 bits for outside
	// n_flips  = COUNT_FLIP_X[th = _bextr_u64(mP, pos & 0x38, 8)];
	n_flips  = COUNT_FLIP_X[th = (mP >> (pos & 0x38)) & 0xFF];
	n_flips += COUNT_FLIP_Y[_pext_u64(mP, mask_x[pos][0])];
	n_flips += COUNT_FLIP_Y[_pext_u64(mP, mask_x[pos][1])];
	n_flips += COUNT_FLIP_Y[tv = _pext_u64(mP, mask_x[pos][2])];

	score = 2 * bit_count(P) - SCORE_MAX + 2;	// = (bit_count(P) + 1) - (SCORE_MAX - 1 - bit_count(P))
	score += n_flips;

	if (n_flips == 0) {
		score2 = score - 2;	// empty for opponent
		if (score <= 0)
			score = score2;

		if (score > alpha) {	// lazy cut-off
			mP = ~P & mask_x[pos][3];
			n_flips  = COUNT_FLIP_X[th ^ 0xFF];
			n_flips += COUNT_FLIP_Y[_pext_u64(mP, mask_x[pos][0])];
			n_flips += COUNT_FLIP_Y[_pext_u64(mP, mask_x[pos][1])];
			n_flips += COUNT_FLIP_Y[tv ^ 0xFF];

			if (n_flips != 0)
				score = score2 - n_flips;
		}
	}

	return score;
}

#elif (LAST_FLIP_COUNTER == COUNT_LAST_FLIP_AVX512) && defined(SIMULLASTFLIP512)
// branchless AVX512(512) lastflip (2.71s on skylake, 2.48 on icelake, 2.15s on Zen4)

extern	const V8DI lrmask[66];	// in flip_avx512cd.c

static inline int vectorcall board_score_sse_1(__m128i OP, const int alpha, const int pos)
{
	int	score;
	__m512i	op_outflank, op_flip, op_eraser, mask;
	__m256i	o_flip, opop_flip;
	__mmask8 op_pass;
	__m512i	O4P4 = _mm512_xor_si512(_mm512_broadcastq_epi64(OP),
		 _mm512_set_epi64(-1, -1, -1, -1, 0, 0, 0, 0));

		// left: look for player LS1B
	mask = _mm512_broadcast_i64x4(lrmask[pos].v4[0]);
	op_outflank = _mm512_and_si512(O4P4, mask);
		// set below LS1B if P is in lmask
	op_flip = _mm512_maskz_add_epi64(_mm512_test_epi64_mask(op_outflank, op_outflank),
		op_outflank, _mm512_set1_epi64(-1));
	// op_flip = _mm512_and_si512(_mm512_andnot_si512(op_outflank, op_flip), mask);
	op_flip = _mm512_ternarylogic_epi64(op_outflank, op_flip, mask, 0x08);

		// right: clear all bits lower than outflank
	mask = _mm512_broadcast_i64x4(lrmask[pos].v4[1]);
	op_outflank = _mm512_and_si512(O4P4, mask);
	op_eraser = _mm512_srlv_epi64(_mm512_set1_epi64(-1),
		_mm512_maskz_lzcnt_epi64(_mm512_test_epi64_mask(op_outflank, op_outflank), op_outflank));
	// op_flip = _mm512_or_si512(op_flip, _mm512_andnot_si512(op_eraser, mask));
	op_flip = _mm512_ternarylogic_epi64(op_flip, op_eraser, mask, 0xf2);

	o_flip = _mm512_extracti64x4_epi64(op_flip, 1);
	opop_flip = _mm256_or_si256(_mm256_unpacklo_epi64(_mm512_castsi512_si256(op_flip), o_flip),
		_mm256_unpackhi_epi64(_mm512_castsi512_si256(op_flip), o_flip));
	OP = _mm_xor_si128(_mm512_castsi512_si128(O4P4),
		_mm_or_si128(_mm256_castsi256_si128(opop_flip), _mm256_extracti128_si256(opop_flip, 1)));
	op_pass = _mm_cmpeq_epi64_mask(OP, _mm512_castsi512_si128(O4P4));
	OP = _mm_mask_unpackhi_epi64(OP, op_pass, OP, OP);	// use O if p_pass
	score = bit_count(_mm_cvtsi128_si64(OP));
		// last square for P if not P pass or (O pass and score >= 32)
	// score += ((~op_pass & 1) | ((op_pass >> 1) & (score >= 32)));
	score += (~op_pass | ((op_pass >> 1) & (score >> 5))) & 1;
	(void) alpha;	// no lazy cut-off
	return score * 2 - SCORE_MAX;	// = bit_count(P) - (SCORE_MAX - bit_count(P))
}

#elif (LAST_FLIP_COUNTER == COUNT_LAST_FLIP_AVX512) && defined(SIMULLASTFLIP)
// branchless AVX512(256) lastflip (2.61s on skylake, 2.38 on icelake, 2.13s on Zen4)

extern	const V8DI lrmask[66];	// in flip_avx512cd.c

static inline int vectorcall board_score_sse_1(__m128i OP, const int alpha, const int pos)
{
	int	score;
	__m256i	p_flip, o_flip, p_outflank, o_outflank, p_eraser, o_eraser, mask, opop_flip;
	__mmask8 op_pass;
	__m256i	P4 = _mm256_broadcastq_epi64(OP);

		// left: look for player LS1B
	mask = lrmask[pos].v4[0];
	p_outflank = _mm256_and_si256(P4, mask);	o_outflank = _mm256_andnot_si256(P4, mask);
		// set below LS1B if P is in lmask
	p_flip = _mm256_maskz_add_epi64(_mm256_test_epi64_mask(P4, mask), p_outflank, _mm256_set1_epi64x(-1));
		// set below LS1B if O is in lmask
	o_flip = _mm256_maskz_add_epi64(_mm256_test_epi64_mask(o_outflank, o_outflank), o_outflank, _mm256_set1_epi64x(-1));
	// p_flip = _mm256_and_si256(_mm256_andnot_si256(p_outflank, p_flip), mask);
	p_flip = _mm256_ternarylogic_epi64(p_outflank, p_flip, mask, 0x08);
	// o_flip = _mm256_and_si256(_mm256_andnot_si256(o_outflank, o_flip), mask);
	o_flip = _mm256_ternarylogic_epi64(o_outflank, o_flip, mask, 0x08);

		// right: clear all bits lower than outflank
	mask = lrmask[pos].v4[1];
	p_outflank = _mm256_and_si256(P4, mask);	o_outflank = _mm256_andnot_si256(P4, mask);
	p_eraser = _mm256_srlv_epi64(_mm256_set1_epi64x(-1),
		_mm256_maskz_lzcnt_epi64(_mm256_test_epi64_mask(P4, mask), p_outflank));
	o_eraser = _mm256_srlv_epi64(_mm256_set1_epi64x(-1),
		_mm256_maskz_lzcnt_epi64(_mm256_test_epi64_mask(o_outflank, o_outflank), o_outflank));
	// p_flip = _mm256_or_si256(p_flip, _mm256_andnot_si256(p_eraser, mask));
	p_flip = _mm256_ternarylogic_epi64(p_flip, p_eraser, mask, 0xf2);
	// o_flip = _mm256_or_si256(o_flip, _mm256_andnot_si256(o_eraser, mask));
	o_flip = _mm256_ternarylogic_epi64(o_flip, o_eraser, mask, 0xf2);

	opop_flip = _mm256_or_si256(_mm256_unpacklo_epi64(p_flip, o_flip), _mm256_unpackhi_epi64(p_flip, o_flip));
	OP = _mm_xor_si128(_mm256_castsi256_si128(P4),
		_mm_or_si128(_mm256_castsi256_si128(opop_flip), _mm256_extracti128_si256(opop_flip, 1)));
	op_pass = _mm_cmpeq_epi64_mask(OP, _mm256_castsi256_si128(P4));
	OP = _mm_mask_unpackhi_epi64(OP, op_pass, OP, OP);	// use O if p_pass
	score = bit_count(_mm_cvtsi128_si64(OP));
		// last square for P if not P pass or (O pass and score >= 32)
	// score += ((~op_pass & 1) | ((op_pass >> 1) & (score >= 32)));
	score += (~op_pass | ((op_pass >> 1) & (score >> 5))) & 1;
	(void) alpha;	// no lazy cut-off
	return score * 2 - SCORE_MAX;	// = bit_count(P) - (SCORE_MAX - bit_count(P))
}

#elif (LAST_FLIP_COUNTER == COUNT_LAST_FLIP_AVX512) && defined(LASTFLIP_HIGHCUT)
// AVX512(256) NWS lazy high cut-off version (2.63s on skylake, 2.33 on icelake, 2.14s on Zen4)

extern	const V8DI lrmask[66];	// in flip_avx_ppfill.c

static inline int vectorcall board_score_sse_1(__m128i OP, const int alpha, const int pos)
{
	int score = 2 * bit_count(_mm_cvtsi128_si64(OP)) - SCORE_MAX + 2;	// = (bit_count(P) + 1) - (SCORE_MAX - 1 - bit_count(P))
		// if player can move, final score > this score.
		// if player pass then opponent play, final score < score - 1 (cancel P) - 1 (last O).
		// if both pass, score - 1 (cancel P) - 1 (empty for O) <= final score <= score (empty for P).
	__m256i P4 = _mm256_broadcastq_epi64(OP);
	__m256i lmask = lrmask[pos].v4[0];
	__m256i rmask = lrmask[pos].v4[1];
	__mmask16 lp = _mm256_test_epi64_mask(P4, lmask);	// P exists on mask
	__mmask16 rp = _mm256_test_epi64_mask(P4, rmask);
	__m256i F4, outflank, eraser, lmO, rmO;
	__m128i F2;
	int nflip;

	if (score > alpha) {	// if player can move, high cut-off will occur regardress of n_flips.
		lmO = _mm256_maskz_andnot_epi64(lp, P4, lmask);	// masked O, clear if all O
		rmO = _mm256_maskz_andnot_epi64(rp, P4, rmask);	// (all O = all P = 0 flip)
		if (_mm256_testz_si256(_mm256_or_si256(lmO, rmO), _mm256_set1_epi64x(NEIGHBOUR[pos]))) {
			// nflip = last_flip(pos, ~P);
				// left: set below LS1B if O is in lmask
			F4 = _mm256_maskz_add_epi64(_mm256_test_epi64_mask(lmO, lmO), lmO, _mm256_set1_epi64x(-1));
			// F4 = _mm256_and_si256(_mm256_andnot_si256(lmO, F4), lmask);
			F4 = _mm256_ternarylogic_epi64(lmO, F4, lmask, 0x08);

				// right: clear all bits lower than outflank
			eraser = _mm256_srlv_epi64(_mm256_set1_epi64x(-1),
				_mm256_maskz_lzcnt_epi64(_mm256_test_epi64_mask(rmO, rmO), rmO));
			// F4 = _mm256_or_si256(F4, _mm256_andnot_si256(eraser, rmask));
			F4 = _mm256_ternarylogic_epi64(F4, eraser, rmask, 0xf2);

			F2 = _mm_or_si128(_mm256_castsi256_si128(F4), _mm256_extracti128_si256(F4, 1));
			nflip = -bit_count(_mm_cvtsi128_si64(_mm_or_si128(F2, _mm_unpackhi_epi64(F2, F2))));
				// last square for O if O can move or score <= 0
			score += (nflip - (int)((nflip | (score - 1)) < 0)) * 2;

		} else	score += 2;	// lazy high cut-off, return min flip

	} else {	// if player cannot move, low cut-off will occur whether opponent can move.
			// left: set below LS1B if P is in lmask
		outflank = _mm256_and_si256(P4, lmask);
		F4 = _mm256_maskz_add_epi64(lp, outflank, _mm256_set1_epi64x(-1));
		// F4 = _mm256_and_si256(_mm256_andnot_si256(outflank, F4), lmask);
		F4 = _mm256_ternarylogic_epi64(outflank, F4, lmask, 0x08);

			// right: clear all bits lower than outflank
		outflank = _mm256_and_si256(P4, rmask);
		eraser = _mm256_srlv_epi64(_mm256_set1_epi64x(-1), _mm256_maskz_lzcnt_epi64(rp, outflank));
		// F4 = _mm256_or_si256(F4, _mm256_andnot_si256(eraser, rmask));
		F4 = _mm256_ternarylogic_epi64(F4, eraser, rmask, 0xf2);

		F2 = _mm_or_si128(_mm256_castsi256_si128(F4), _mm256_extracti128_si256(F4, 1));
		nflip = bit_count(_mm_cvtsi128_si64(_mm_or_si128(F2, _mm_unpackhi_epi64(F2, F2))));
		score += nflip * 2;

		// if nflip == 0, score <= alpha so lazy low cut-off
	}

	return score;
}

#elif (LAST_FLIP_COUNTER == COUNT_LAST_FLIP_AVX_PPFILL) && defined(LASTFLIP_HIGHCUT)
// experimental AVX2 lastflip with lazy high cut-off version (a little slower)
extern	const V8DI lrmask[66];	// in flip_avx_ppfill.c

static inline int vectorcall board_score_sse_1(__m128i OP, const int alpha, const int pos)
{
	int score = 2 * bit_count(_mm_cvtsi128_si64(OP)) - SCORE_MAX + 2;	// = (bit_count(P) + 1) - (SCORE_MAX - 1 - bit_count(P))
		// if player can move, final score > this score.
		// if player pass then opponent play, final score < score - 1 (cancel P) - 1 (last O).
		// if both pass, score - 1 (cancel P) - 1 (empty for O) <= final score <= score (empty for P).
	__m256i P4 = _mm256_broadcastq_epi64(OP);
	__m256i F4, lmask, rmask, outflank, eraser, lmO, rmO, lp, rp;
	__m128i F2;
	int nflip;

	lmask = lrmask[pos].v4[0];			rmask = lrmask[pos].v4[1];
	lmO = _mm256_andnot_si256(P4, lmask);		rmO = _mm256_andnot_si256(P4, rmask);
	lp = _mm256_cmpeq_epi64(lmO, lmask);		rp = _mm256_cmpeq_epi64(rmO, rmask);	// 0 if P exists on mask

	if (score > alpha) {	// if player can move, high cut-off will occur regardress of n_flips.
		F4 = _mm256_or_si256(_mm256_andnot_si256(lp, lmO), _mm256_andnot_si256(rp, rmO));	// clear if all O
		if (_mm256_testz_si256(F4, _mm256_set1_epi64x(NEIGHBOUR[pos]))) {	// pass (16%)
			// n_flips = last_flip(pos, ~P);
				// right: isolate opponent MS1B by clearing lower shadow bits
			eraser = _mm256_srlv_epi64(rmO, _mm256_set_epi64x(7, 9, 8, 1));
				// eraser = opponent's shadow
			eraser = _mm256_or_si256(eraser, rmO);
			eraser = _mm256_or_si256(eraser, _mm256_srlv_epi64(eraser, _mm256_set_epi64x(14, 18, 16, 2)));
			F4 = _mm256_andnot_si256(eraser, rmask);
			F4 = _mm256_andnot_si256(_mm256_srlv_epi64(eraser, _mm256_set_epi64x(28, 36, 32, 4)), F4);
				// clear if no opponent bit, i.e. all player
			F4 = _mm256_andnot_si256(_mm256_cmpeq_epi64(F4, rmask), F4);

				// left: look for opponent LS1B
			outflank = _mm256_and_si256(lmO, _mm256_sub_epi64(_mm256_setzero_si256(), lmO));	// LS1B
				// set all bits if outflank = 0, otherwise higher bits than outflank
			eraser = _mm256_sub_epi64(_mm256_cmpeq_epi64(outflank, _mm256_setzero_si256()), outflank);
			F4 = _mm256_or_si256(F4, _mm256_andnot_si256(eraser, lmask));

			F2 = _mm_or_si128(_mm256_castsi256_si128(F4), _mm256_extracti128_si256(F4, 1));
			nflip = -bit_count(_mm_cvtsi128_si64(_mm_or_si128(F2, _mm_unpackhi_epi64(F2, F2))));
				// last square for O if O can move or score <= 0
			score += (nflip - (int)((nflip | (score - 1)) < 0)) * 2;

		} else	score += 2;	// lazy high cut-off, return min flip

	} else {	// if player cannot move, low cut-off will occur whether opponent can move.
			// right: isolate player MS1B by clearing lower shadow bits
		outflank = _mm256_and_si256(P4, rmask);
		eraser = _mm256_srlv_epi64(outflank, _mm256_set_epi64x(7, 9, 8, 1));
			// eraser = player's shadow
		eraser = _mm256_or_si256(eraser, outflank);
		eraser = _mm256_or_si256(eraser, _mm256_srlv_epi64(eraser, _mm256_set_epi64x(14, 18, 16, 2)));
		F4 = _mm256_andnot_si256(eraser, rmask);
		F4 = _mm256_andnot_si256(_mm256_srlv_epi64(eraser, _mm256_set_epi64x(28, 36, 32, 4)), F4);
			// clear if no player bit, i.e. all opponent
		F4 = _mm256_andnot_si256(rp, F4);

			// left: set below LS1B if P is in lmask
		outflank = _mm256_and_si256(P4, lmask);
		outflank = _mm256_andnot_si256(outflank, _mm256_add_epi64(outflank, _mm256_set1_epi64x(-1)));
		F4 = _mm256_or_si256(F4, _mm256_andnot_si256(lp, _mm256_and_si256(outflank, lmask)));

		F2 = _mm_or_si128(_mm256_castsi256_si128(F4), _mm256_extracti128_si256(F4, 1));
		nflip = bit_count(_mm_cvtsi128_si64(_mm_or_si128(F2, _mm_unpackhi_epi64(F2, F2))));
		score += nflip * 2;

		// if nflip == 0, score <= alpha so lazy low cut-off
	}

	return score;
}

#elif defined(__AVX2__) && defined(SIMULLASTFLIP)
// experimental branchless AVX2 MOVMSK version (slower on icc, par on msvc)
// https://eukaryote.hateblo.jp/entry/2020/05/10/033228
static inline int vectorcall board_score_sse_1(__m128i OP, const int alpha, const int pos)
{
	int_fast8_t	p_flip, o_flip;
	unsigned int	tP, tO, h;
	unsigned long long P = _mm_cvtsi128_si64(OP);
	int	score, score2;
	const uint8_t *COUNT_FLIP_X = COUNT_FLIP[pos & 7];
	const uint8_t *COUNT_FLIP_Y = COUNT_FLIP[pos >> 3];

	__m256i M = mask_dvhd[pos].v4;
	__m256i PP = _mm256_broadcastq_epi64(OP);

	(void) alpha;	// no lazy cut-off
	h = (P >> (pos & 0x38)) & 0xFF;
	tP = TEST_EPI8_MASK32(PP, M);			tO = tP ^ TEST1_EPI8_MASK32(M);
	p_flip  = COUNT_FLIP_X[h];			o_flip = -COUNT_FLIP_X[h ^ 0xFF];
	p_flip += COUNT_FLIP_Y[tP & 0xFF];		o_flip -= COUNT_FLIP_Y[tO & 0xFF];
	p_flip += COUNT_FLIP_Y[(tP >> 16) & 0xFF];	o_flip -= COUNT_FLIP_Y[(tO >> 16) & 0xFF];
	p_flip += COUNT_FLIP_Y[tP >> 24];		o_flip -= COUNT_FLIP_Y[tO >> 24];

	score = 2 * bit_count(P) - SCORE_MAX + 2;	// = (bit_count(P) + 1) - (SCORE_MAX - 1 - bit_count(P))
	score2 = score + o_flip - (int)((o_flip | (score - 1)) < 0) * 2;	// last square for O if O can move or score <= 0
	score += p_flip;
	return p_flip ? score : score2;	// gcc/icc inserts branch here, since score2 may be wholly skipped.
}

#elif defined(__AVX2__) && defined(LASTFLIP_HIGHCUT)
// AVX2 NWS lazy high cut-off version
// http://www.amy.hi-ho.ne.jp/okuhara/edaxopt.htm#lazycutoff
// lazy high cut-off idea was in Zebra by Gunnar Anderson (http://radagast.se/othello/zebra.html),
// but commented out because mobility check was no faster than counting flips.
// Now using AVX2, mobility check can be faster than counting flips.

extern	const V8DI lrmask[66];	// in flip_avx_ppfill.c

static inline int vectorcall board_score_sse_1(__m128i OP, const int alpha, const int pos)
{
	int_fast8_t n_flips;
	uint32_t t;
	unsigned long long P = _mm_cvtsi128_si64(OP);
	int score = 2 * bit_count(P) - SCORE_MAX + 2;	// = (bit_count(P) + 1) - (SCORE_MAX - 1 - bit_count(P))
		// if player can move, final score > this score.
		// if player pass then opponent play, final score < score - 1 (cancel P) - 1 (last O).
		// if both pass, score - 1 (cancel P) - 1 (empty for O) <= final score <= score (empty for P).

	if (score > alpha) {	// if player can move, high cut-off will occur regardress of n_flips.
  #if 0 // def __AVX512VL__	// may trigger license base downclocking, wrong fingerprint on MSVC
		__m512i P8 = _mm512_broadcastq_epi64(OP);
		__m512i M = lrmask[pos].v8;
		__m512i mO = _mm512_andnot_epi64(P8, M);
		if (!_mm512_mask_test_epi64_mask(_mm512_test_epi64_mask(P8, M), mO, _mm512_set1_epi64(NEIGHBOUR[pos]))) {	// pass (16%)
			// n_flips = last_flip(pos, ~P);
			t = _cvtmask32_u32(_mm256_cmpneq_epi8_mask(_mm512_castsi512_si256(mO), _mm512_extracti64x4_epi64(mO, 1)));	// eq only if l = r = 0

  #elif defined(__AVX512VL__) || defined(__AVX10_1__)	// 256bit AVX512 (2.61s on skylake, 2.37 on icelake, 2.16s on Zen4)
		__m256i P4 = _mm256_broadcastq_epi64(OP);
		__m256i M = lrmask[pos].v4[0];
		__m256i F = _mm256_maskz_andnot_epi64(_mm256_test_epi64_mask(P4, M), P4, M);	// clear if all O
		M = lrmask[pos].v4[1];
		// F = _mm256_mask_or_epi64(F, _mm256_test_epi64_mask(P4, M), F, _mm256_andnot_si256(P4, M));
		F = _mm256_mask_ternarylogic_epi64(F, _mm256_test_epi64_mask(P4, M), P4, M, 0xF2);
		if (_mm256_testz_si256(F, _mm256_set1_epi64x(NEIGHBOUR[pos]))) {	// pass (16%)
			// n_flips = last_flip(pos, ~P);
			// t = _cvtmask32_u32(_mm256_cmpneq_epi8_mask(_mm256_andnot_si256(P4, lM), _mm256_andnot_si256(P4, rM)));
			t = _cvtmask32_u32(_mm256_test_epi8_mask(F, F));	// all O = all P = 0 flip

  #else	// AVX2
		__m256i P4 = _mm256_broadcastq_epi64(OP);
		__m256i M = lrmask[pos].v4[0];
		__m256i lmO = _mm256_andnot_si256(P4, M);
		__m256i F = _mm256_andnot_si256(_mm256_cmpeq_epi64(lmO, M), lmO);	// clear if all O
		M = lrmask[pos].v4[1];
		__m256i rmO = _mm256_andnot_si256(P4, M);
		F = _mm256_or_si256(F, _mm256_andnot_si256(_mm256_cmpeq_epi64(rmO, M), rmO));
		if (_mm256_testz_si256(F, _mm256_set1_epi64x(NEIGHBOUR[pos]))) {	// pass (16%)
			// n_flips = last_flip(pos, ~P);
			t = ~_mm256_movemask_epi8(_mm256_cmpeq_epi8(lmO, rmO));	// eq only if l = r = 0
  #endif
			const uint8_t *COUNT_FLIP_Y = COUNT_FLIP[pos >> 3];

			n_flips = -COUNT_FLIP[pos & 7][(~P >> (pos & 0x38)) & 0xFF];	// h
			n_flips -= COUNT_FLIP_Y[(t >> 8) & 0xFF];	// v
			n_flips -= COUNT_FLIP_Y[(t >> 16) & 0xFF];	// d
			n_flips -= COUNT_FLIP_Y[t >> 24];	// d
				// last square for O if O can move or score <= 0
			score += n_flips - (int)((n_flips | (score - 1)) < 0) * 2;
		} else	score += 2;	// min flip

	} else {	// if player cannot move, low cut-off will occur whether opponent can move.
		const uint8_t *COUNT_FLIP_Y = COUNT_FLIP[pos >> 3];

		// n_flips = last_flip(pos, P);
		t = TEST_EPI8_MASK32(_mm256_broadcastq_epi64(OP), mask_dvhd[pos].v4);
		n_flips  = COUNT_FLIP[pos & 7][(P >> (pos & 0x38)) & 0xFF];	// h
		n_flips += COUNT_FLIP_Y[t & 0xFF];	// d
		n_flips += COUNT_FLIP_Y[(t >> 16) & 0xFF];	// v
		n_flips += COUNT_FLIP_Y[t >> 24];	// d
		score += n_flips;

		// if n_flips == 0, score <= alpha so lazy low cut-off
	}

	return score;
}

#else	// COUNT_LAST_FLIP_SSE - reasonably fast on all platforms (2.61s on skylake, 2.16s on Zen4)
static inline int vectorcall board_score_sse_1(__m128i OP, const int alpha, const int pos)
{
	uint_fast8_t	n_flips;
	unsigned int	t;
	int	score, score2;
	const uint8_t *COUNT_FLIP_X = COUNT_FLIP[pos & 7];
	const uint8_t *COUNT_FLIP_Y = COUNT_FLIP[pos >> 3];

	// n_flips = last_flip(pos, P);
  #ifdef AVXLASTFLIP	// no gain
	__m256i M = mask_dvhd[pos].v4;
	__m256i P4 = _mm256_broadcastq_epi64(OP);
	unsigned int h = (_mm_cvtsi128_si64(OP) >> (pos & 0x38)) & 0xFF;

	t = TEST_EPI8_MASK32(P4, M);
	n_flips  = COUNT_FLIP_X[h];
	n_flips += COUNT_FLIP_Y[t & 0xFF];
	t >>= 16;

  #else
	__m128i M0 = mask_dvhd[pos].v2[0];
	__m128i M1 = mask_dvhd[pos].v2[1];
	__m128i P2 = _mm_unpacklo_epi64(OP, OP);
	__m128i II = _mm_sad_epu8(_mm_and_si128(P2, M0), _mm_setzero_si128());

	n_flips  = COUNT_FLIP_X[_mm_extract_epi16(II, 4)];
	n_flips += COUNT_FLIP_X[_mm_cvtsi128_si32(II)];
	t = TEST_EPI8_MASK16(P2, M1);
  #endif
	n_flips += COUNT_FLIP_Y[t >> 8];
	n_flips += COUNT_FLIP_Y[t & 0xFF];

	score = 2 * bit_count_si64(OP) - SCORE_MAX + 2;	// = (bit_count(P) + 1) - (SCORE_MAX - 1 - bit_count(P))
	score += n_flips;

	if (n_flips == 0) {
		score2 = score - 2;	// empty for player
		if (score <= 0)
			score = score2;

		if (score > alpha) {	// lazy cut-off
			// n_flips = last_flip(pos, ~P);
  #ifdef AVXLASTFLIP
			t = TEST1_EPI8_MASK32(_mm256_andnot_si256(P4, M));
			n_flips  = COUNT_FLIP_X[h ^ 0xFF];
			n_flips += COUNT_FLIP_Y[t & 0xFF];
			t >>= 16;
  #else
			II = _mm_sad_epu8(_mm_andnot_si128(P2, M0), _mm_setzero_si128());
			n_flips  = COUNT_FLIP_X[_mm_extract_epi16(II, 4)];
			n_flips += COUNT_FLIP_X[_mm_cvtsi128_si32(II)];
			t = _mm_movemask_epi8(_mm_sub_epi8(_mm_setzero_si128(), _mm_andnot_si128(P2, M1)));
  #endif
			n_flips += COUNT_FLIP_Y[t >> 8];
			n_flips += COUNT_FLIP_Y[t & 0xFF];

			if (n_flips != 0)
				score = score2 - n_flips;
		}
	}

	return score;
}
#endif

// from bench.c
int board_score_1(const unsigned long long player, const int alpha, const int x)
{
	return board_score_sse_1(_mm_cvtsi64_si128(player), alpha, x);
}

/**
 * @brief Get the final score.
 *
 * Get the final min score, when 2 empty squares remain.
 *
 * @param OP The board to evaluate.
 * @param alpha Alpha bound.
 * @param n_nodes Node counter.
 * @param empties Packed empty square coordinates.
 * @return The final min score, as a disc difference.
 */
static int vectorcall board_solve_2(__m128i OP, int alpha, volatile unsigned long long *n_nodes, __m128i empties)
{
	__m128i flipped;
	int score, bestscore, nodes;
	int x1 = _mm_extract_epi16(empties, 1);
	int x2 = _mm_extract_epi16(empties, 0);
	unsigned long long opponent;
	// const int beta = alpha + 1;

	SEARCH_STATS(++statistics.n_board_solve_2);

	opponent = EXTRACT_O(OP);
	if ((NEIGHBOUR[x1] & opponent) && !TESTZ_FLIP(flipped = mm_Flip(OP, x1))) {
		bestscore = board_score_sse_1(_mm_xor_si128(_mm_shuffle_epi32(OP, SWAP64), reduce_vflip(flipped)), alpha, x2);

		if ((bestscore > alpha) && (NEIGHBOUR[x2] & opponent) && !TESTZ_FLIP(flipped = mm_Flip(OP, x2))) {
			score = board_score_sse_1(_mm_xor_si128(_mm_shuffle_epi32(OP, SWAP64), reduce_vflip(flipped)), alpha, x1);
			if (score < bestscore)
				bestscore = score;
			nodes = 3;
		} else	nodes = 2;

	} else if ((NEIGHBOUR[x2] & opponent) && !TESTZ_FLIP(flipped = mm_Flip(OP, x2))) {
		bestscore = board_score_sse_1(_mm_xor_si128(_mm_shuffle_epi32(OP, SWAP64), reduce_vflip(flipped)), alpha, x1);
		nodes = 2;

	} else {	// pass - NEIGHBOUR test is almost 100% true
		alpha = ~alpha;	// = -alpha - 1
		if (!TESTZ_FLIP(flipped = mm_Flip(_mm_shuffle_epi32(OP, SWAP64), x1))) {
			bestscore = board_score_sse_1(_mm_xor_si128(OP, reduce_vflip(flipped)), alpha, x2);

			if ((bestscore > alpha) && !TESTZ_FLIP(flipped = mm_Flip(_mm_shuffle_epi32(OP, SWAP64), x2))) {
				score = board_score_sse_1(_mm_xor_si128(OP, reduce_vflip(flipped)), alpha, x1);
				if (score < bestscore)
					bestscore = score;
				nodes = 3;
			} else	nodes = 2;

		} else if (!TESTZ_FLIP(flipped = mm_Flip(_mm_shuffle_epi32(OP, SWAP64), x2))) {
			bestscore = board_score_sse_1(_mm_xor_si128(OP, reduce_vflip(flipped)), alpha, x1);
			nodes = 2;

		} else {	// gameover
			bestscore = board_solve(_mm_cvtsi128_si64(OP), 2);
			nodes = 1;
		}
		bestscore = -bestscore;
	}

	SEARCH_UPDATE_2EMPTIES_NODES(*n_nodes += nodes;)
	assert(SCORE_MIN <= bestscore && bestscore <= SCORE_MAX);
	assert((bestscore & 1) == 0);
	return bestscore;
}

/**
 * @brief Get the final score.
 *
 * Get the final max score, when 3 empty squares remain.
 *
 * @param OP The board to evaluate.
 * @param alpha Alpha bound.
 * @param n_nodes Node counter.
 * @param empties Packed empty square coordinates.
 * @return The final max score, as a disc difference.
 */
static int vectorcall search_solve_3(__m128i OP, int alpha, volatile unsigned long long *n_nodes, __m128i empties)
{
	__m128i flipped;
	int score, bestscore, x, pol;
	unsigned long long opponent;

	SEARCH_STATS(++statistics.n_search_solve_3);
	SEARCH_UPDATE_INTERNAL_NODES(*n_nodes);

#ifdef __AVX__
	empties = _mm_cvtepu8_epi16(empties);
#elif defined(__SSSE3__)
	empties = _mm_unpacklo_epi8((empties), _mm_setzero_si128());
#endif
	bestscore = -SCORE_INF;
	pol = 1;
	do {
		// best move alphabeta search
		opponent = EXTRACT_O(OP);
		x = _mm_extract_epi16(empties, 2);
		if ((NEIGHBOUR[x] & opponent) && !TESTZ_FLIP(flipped = mm_Flip(OP, x))) {
			bestscore = board_solve_2(board_flip_next(OP, x, flipped), alpha, n_nodes, empties);
			if (bestscore > alpha) return bestscore * pol;
		}

		x = _mm_extract_epi16(empties, 1);
		if (/* (NEIGHBOUR[x] & opponent) && */ !TESTZ_FLIP(flipped = mm_Flip(OP, x))) {
			score = board_solve_2(board_flip_next(OP, x, flipped), alpha, n_nodes, _mm_shufflelo_epi16(empties, 0xd8));	// (d3d1)d2d0
			if (score > alpha) return score * pol;
			else if (score > bestscore) bestscore = score;
		}

		x = _mm_extract_epi16(empties, 0);
		if (/* (NEIGHBOUR[x] & opponent) && */ !TESTZ_FLIP(flipped = mm_Flip(OP, x))) {
			score = board_solve_2(board_flip_next(OP, x, flipped), alpha, n_nodes, _mm_shufflelo_epi16(empties, 0xc9));	// (d3d0)d2d1
			if (score > bestscore) bestscore = score;
			return bestscore * pol;
		}

		if (bestscore > -SCORE_INF)
			return bestscore * pol;

		OP = _mm_shuffle_epi32(OP, SWAP64);	// pass
		alpha = ~alpha;	// = -(alpha + 1)
	} while ((pol = -pol) < 0);

	return board_solve(_mm_cvtsi128_si64(OP), 3);	// gameover
}

/**
 * @brief Get the final score.
 *
 * Get the final min score, when 4 empty squares remain.
 *
 * @param search Search position.
 * @param alpha Upper score value.
 * @return The final min score, as a disc difference.
 */

// pick the move for this ply and pass the rest as packed 3 x 8 bit (AVX/SSSE3) or 3 x 16 bit (SSE), in search order.
#if defined(__SSSE3__) || defined(__AVX__)
  #ifdef __AVX__
	#define	EXTRACT_MOVE(X,i)	_mm_extract_epi8((X), (i) * 4 + 3)
  #else
	#define	EXTRACT_MOVE(X,i)	(_mm_extract_epi16((X), (i) * 2 + 1) >> 8)
  #endif
	#define	v3_empties_0(empties,sort3)	(empties)
	#define	v3_empties(empties,i,shuf,sort3)	_mm_srli_si128((empties), (i) * 4)
#else
	#define	EXTRACT_MOVE(X,i)	_mm_extract_epi16((X), 3 - (i))
	static inline __m128i vectorcall v3_empties_0(__m128i empties, int sort3) {
		// parity based move sorting
		// if (sort3 == 3) empties = _mm_shufflelo_epi16(empties, 0xe1); // swap x2, x3
		if (sort3 & 2)	empties = _mm_shufflelo_epi16(empties, 0xc9); // case 1(x3) 2(x1 x2): x3->x1->x2->x3
		if (sort3 & 1)	empties = _mm_shufflelo_epi16(empties, 0xd8); // case 1(x2) 2(x1 x3): swap x1, x2
		return empties;
	}
	#define	v3_empties(empties,i,shuf,sort3)	v3_empties_0(_mm_shufflelo_epi16((empties), (shuf)), (sort3))
#endif

static int search_solve_4(Search *search, int alpha)
{
	__m128i	OP, flipped;
	__m128i	empties_series;	// (AVX) B15:4th, B11:3rd, B7:2nd, B3:1st, lower 3 bytes for 3 empties
				// (SSE) W3:1st, W2:2nd, W1:3rd, W0:4th
	int x1, x2, x3, x4, paritysort, score, bestscore, pol;
	unsigned long long opponent;
	// const int beta = alpha + 1;
	static const unsigned char parity_case[64] = {	/* x4x3x2x1 = */
		/*0000*/  0, /*0001*/  0, /*0010*/  1, /*0011*/  9, /*0100*/  2, /*0101*/ 10, /*0110*/ 11, /*0111*/  3,
		/*0002*/  0, /*0003*/  0, /*0012*/  0, /*0013*/  0, /*0102*/  4, /*0103*/  4, /*0112*/  5, /*0113*/  5,
		/*0020*/  1, /*0021*/  0, /*0030*/  1, /*0031*/  0, /*0120*/  6, /*0121*/  7, /*0130*/  6, /*0131*/  7,
		/*0022*/  9, /*0023*/  0, /*0032*/  0, /*0033*/  9, /*0122*/  8, /*0123*/  0, /*0132*/  0, /*0133*/  8,
		/*0200*/  2, /*0201*/  4, /*0210*/  6, /*0211*/  8, /*0300*/  2, /*0301*/  4, /*0310*/  6, /*0311*/  8,
		/*0202*/ 10, /*0203*/  4, /*0212*/  7, /*0213*/  0, /*0302*/  4, /*0303*/ 10, /*0312*/  0, /*0313*/  7,
		/*0220*/ 11, /*0221*/  5, /*0230*/  6, /*0231*/  0, /*0320*/  6, /*0321*/  0, /*0330*/ 11, /*0331*/  5,
		/*0222*/  3, /*0223*/  5, /*0232*/  7, /*0233*/  8, /*0322*/  8, /*0323*/  7, /*0332*/  5, /*0333*/  3
	};
#if defined(__SSSE3__) || defined(__AVX__)
	union V4SI {
		unsigned int	ui[4];
		__m128i	v4;
	};
	static const union V4SI shuf_mask[] = {	// make search order identical to 4.4.0
		{{ 0x03020100, 0x02030100, 0x01030200, 0x00030201 }},	//  0: 1(x1) 3(x2 x3 x4), 1(x1) 1(x2) 2(x3 x4), 1 1 1 1, 4
		{{ 0x03020100, 0x02030100, 0x01020300, 0x00020301 }},	//  1: 1(x2) 3(x1 x3 x4)
		{{ 0x03010200, 0x02010300, 0x01030200, 0x00010302 }},	//  2: 1(x3) 3(x1 x2 x4)
		{{ 0x03000201, 0x02000301, 0x01000302, 0x00030201 }},	//  3: 1(x4) 3(x1 x2 x3)
		{{ 0x03010200, 0x01030200, 0x02030100, 0x00030201 }},	//  4: 1(x1) 1(x3) 2(x2 x4)
		{{ 0x03000201, 0x00030201, 0x02030100, 0x01030200 }},	//  5: 1(x1) 1(x4) 2(x2 x3)
		{{ 0x02010300, 0x01020300, 0x03020100, 0x00030201 }},	//  6: 1(x2) 1(x3) 2(x1 x4)
		{{ 0x02000301, 0x00020301, 0x03020100, 0x01030200 }},	//  7: 1(x2) 1(x4) 2(x1 x3)
		{{ 0x01000302, 0x00010302, 0x03020100, 0x02030100 }},	//  8: 1(x3) 1(x4) 2(x1 x2)
		{{ 0x03020100, 0x02030100, 0x01000302, 0x00010302 }},	//  9: 2(x1 x2) 2(x3 x4)
		{{ 0x03010200, 0x02000301, 0x01030200, 0x00020301 }},	// 10: 2(x1 x3) 2(x2 x4)
		{{ 0x03000201, 0x02010300, 0x01020300, 0x00030201 }}	// 11: 2(x1 x4) 2(x2 x3)
	};
	enum { sort3 = 0 };	// sort is done on 4 empties
#else
	int sort3;	// for move sorting on 3 empties
	static const short sort3_shuf[] = {
		0x0000,	//  0: 1(x1) 3(x2 x3 x4), 1(x1) 1(x2) 2(x3 x4), 1 1 1 1, 4		x4x1x2x3-x3x1x2x4-x2x1x3x4-x1x2x3x4
		0x1100,	//  1: 1(x2) 3(x1 x3 x4)	x4x2x1x3-x3x2x1x4-x2x1x3x4-x1x2x3x4
		0x2011,	//  2: 1(x3) 3(x1 x2 x4)	x4x3x1x2-x3x1x2x4-x2x3x1x4-x1x3x2x4
		0x0222,	//  3: 1(x4) 3(x1 x2 x3)	x4x1x2x3-x3x4x1x2-x2x4x1x3-x1x4x2x3
		0x3000,	//  4: 1(x1) 1(x3) 2(x2 x4)	x4x1x2x3-x2x1x3x4-x3x1x2x4-x1x3x2x4 <- x4x1x3x2-x2x1x3x4-x3x1x2x4-x1x3x2x4
		0x3300,	//  5: 1(x1) 1(x4) 2(x2 x3)	x3x1x2x4-x2x1x3x4-x4x1x2x3-x1x4x2x3 <- x3x1x4x2-x2x1x4x3-x4x1x2x3-x1x4x2x3
		0x2000,	//  6: 1(x2) 1(x3) 2(x1 x4)	x4x1x2x3-x1x2x3x4-x3x2x1x4-x2x3x1x4 <- x4x2x3x1-x1x2x3x4-x3x2x1x4-x2x3x1x4
		0x2300,	//  7: 1(x2) 1(x4) 2(x1 x3)	x3x1x2x4-x1x2x3x4-x4x2x1x3-x2x4x1x3 <- x3x2x4x1-x1x2x4x3-x4x2x1x3-x2x4x1x3
		0x2200,	//  8: 1(x3) 1(x4) 2(x1 x2)	x2x1x3x4-x1x2x3x4-x4x3x1x2-x3x4x1x2 <- x2x3x4x1-x1x3x4x2-x4x3x1x2-x3x4x1x2
		0x2200,	//  9: 2(x1 x2) 2(x3 x4)	x4x3x1x2-x3x4x1x2-x2x1x3x4-x1x2x3x4
		0x1021,	// 10: 2(x1 x3) 2(x2 x4)	x4x2x1x3-x3x1x2x4-x2x4x1x3-x1x3x2x4
		0x0112	// 11: 2(x1 x4) 2(x2 x3)	x4x1x2x3-x3x2x1x4-x2x3x1x4-x1x4x2x3
	};
#endif

	SEARCH_STATS(++statistics.n_search_solve_4);
	SEARCH_UPDATE_INTERNAL_NODES(search->n_nodes);

	// stability cutoff (try 12%, cut 7%)
	if (search_SC_NWS_4(search, alpha, &score)) return score;

	OP = _mm_loadu_si128((__m128i *) &search->board);
	x1 = search->empties[NOMOVE].next;
	x2 = search->empties[x1].next;
	x3 = search->empties[x2].next;
	x4 = search->empties[x3].next;

	// parity based move sorting.
	// The following hole sizes are possible:
	//    4 - 1 3 - 2 2 - 1 1 2 - 1 1 1 1
	// Only the 1 1 2 case needs move sorting on this ply.
	paritysort = parity_case[((x3 ^ x4) & 0x24) + ((((x2 ^ x4) & 0x24) * 2 + ((x1 ^ x4) & 0x24)) >> 2)];
#if defined(__SSSE3__) || defined(__AVX__)
	empties_series = _mm_cvtsi32_si128((x1 << 24) | (x2 << 16) | (x3 << 8) | x4);
	empties_series = _mm_shuffle_epi8(empties_series, shuf_mask[paritysort].v4);

#else // SSE
	empties_series = _mm_cvtsi32_si128((x3 << 16) | x4);
	empties_series = _mm_insert_epi16(empties_series, x2, 2);
	empties_series = _mm_insert_epi16(empties_series, x1, 3);
	switch (paritysort) {
		case 4: // case 1(x1) 1(x3) 2(x2 x4)
			empties_series = _mm_shufflelo_epi16(empties_series, 0xd8);	// x1x3x2x4
			break;
		case 5: // case 1(x1) 1(x4) 2(x2 x3)
			empties_series = _mm_shufflelo_epi16(empties_series, 0xc9);	// x1x4x2x3
			break;
		case 6:	// case 1(x2) 1(x3) 2(x1 x4)
			empties_series = _mm_shufflelo_epi16(empties_series, 0x9c);	// x2x3x1x4
			break;
		case 7: // case 1(x2) 1(x4) 2(x1 x3)
			empties_series = _mm_shufflelo_epi16(empties_series, 0x8d);	// x2x4x1x3
			break;
		case 8:	// case 1(x3) 1(x4) 2(x1 x2)
			empties_series = _mm_shufflelo_epi16(empties_series, 0x4e);	// x3x4x1x2
			break;
	}
	sort3 = sort3_shuf[paritysort];
#endif

	bestscore = SCORE_INF;	// min stage
	pol = 1;
	do {
		// best move alphabeta search
		opponent = EXTRACT_O(OP);
		x1 = EXTRACT_MOVE(empties_series, 0);
		if ((NEIGHBOUR[x1] & opponent) && !TESTZ_FLIP(flipped = mm_Flip(OP, x1))) {
			bestscore = search_solve_3(board_flip_next(OP, x1, flipped), alpha, &search->n_nodes,
				v3_empties_0(empties_series, sort3));
			if (bestscore <= alpha) return bestscore * pol;
		}

		x2 = EXTRACT_MOVE(empties_series, 1);
		if ((NEIGHBOUR[x2] & opponent) && !TESTZ_FLIP(flipped = mm_Flip(OP, x2))) {
			score = search_solve_3(board_flip_next(OP, x2, flipped), alpha, &search->n_nodes,
				v3_empties(empties_series, 1, 0xb4, sort3 >> 4));	// (SSE) x2x1x3x4
			if (score <= alpha) return score * pol;
			else if (score < bestscore) bestscore = score;
		}

		x3 = EXTRACT_MOVE(empties_series, 2);
		if ((NEIGHBOUR[x3] & opponent) && !TESTZ_FLIP(flipped = mm_Flip(OP, x3))) {
			score = search_solve_3(board_flip_next(OP, x3, flipped), alpha, &search->n_nodes,
				v3_empties(empties_series, 2, 0x78, sort3 >> 8));	// (SSE) x3x1x2x4
			if (score <= alpha) return score * pol;
			else if (score < bestscore) bestscore = score;
		}

		x4 = EXTRACT_MOVE(empties_series, 3);
		if ((NEIGHBOUR[x4] & opponent) && !TESTZ_FLIP(flipped = mm_Flip(OP, x4))) {
			score = search_solve_3(board_flip_next(OP, x4, flipped), alpha, &search->n_nodes,
				v3_empties(empties_series, 3, 0x39, sort3 >> 12));	// (SSE) x4x1x2x3
			if (score <= bestscore) bestscore = score;
			return bestscore * pol;
		}

		if (bestscore < SCORE_INF)
			return bestscore * pol;

		OP = _mm_shuffle_epi32(OP, SWAP64);	// pass
		alpha = ~alpha;	// = -(alpha + 1)
	} while ((pol = -pol) < 0);

	return board_solve(EXTRACT_O(OP), 4);	// gameover
}
