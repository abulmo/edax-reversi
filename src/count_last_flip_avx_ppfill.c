/**
 * @file count_last_flip_avx_ppfill.c
 *
 * A function is provided to count the number of fipped disc of the last move.
 *
 * Count last flip using the flip_avx_ppfill way.
 * For optimization purpose, the value returned is twice the number of flipped
 * disc, to facilitate the computation of disc difference.
 *
 * @date 2023
 * @author Toshihiko Okuhara
 * @version 4.5
 * 
 */

#include "bit.h"

extern	const V4DI lmask_v4[66], rmask_v4[66];

/**
 * Count last flipped discs when playing on the last empty.
 *
 * @param pos the last empty square.
 * @param P player's disc pattern.
 * @return flipped disc count.
 */

int last_flip(int pos, unsigned long long P)
{
	__m256i PP = _mm256_set1_epi64x(P);
	__m256i	flip, outflank, eraser, rmask, lmask;
	__m128i	flip2;

	rmask = rmask_v4[pos].v4;
		// isolate player MS1B by clearing lower shadow bits
	outflank = _mm256_and_si256(PP, rmask);
	eraser = _mm256_srlv_epi64(outflank, _mm256_set_epi64x(7, 9, 8, 1));
		// eraser = player's shadow
	eraser = _mm256_or_si256(eraser, outflank);
	eraser = _mm256_or_si256(eraser, _mm256_srlv_epi64(eraser, _mm256_set_epi64x(14, 18, 16, 2)));
	flip = _mm256_andnot_si256(eraser, rmask);
	flip = _mm256_andnot_si256(_mm256_srlv_epi64(eraser, _mm256_set_epi64x(28, 36, 32, 4)), flip);
		// clear if no player bit, i.e. all opponent
	flip = _mm256_andnot_si256(_mm256_cmpeq_epi64(flip, rmask), flip);

	lmask = lmask_v4[pos].v4;
		// look for player LS1B
	outflank = _mm256_and_si256(PP, lmask);
	outflank = _mm256_and_si256(outflank, _mm256_sub_epi64(_mm256_setzero_si256(), outflank));	// LS1B
		// set all bits if outflank = 0, otherwise higher bits than outflank
	eraser = _mm256_sub_epi64(_mm256_cmpeq_epi64(outflank, _mm256_setzero_si256()), outflank);
	flip = _mm256_or_si256(flip, _mm256_andnot_si256(eraser, lmask));

	flip2 = _mm_or_si128(_mm256_castsi256_si128(flip), _mm256_extracti128_si256(flip, 1));
	flip2 = _mm_or_si128(flip2, _mm_shuffle_epi32(flip2, 0x4e));
	return 2 * bit_count(_mm_cvtsi128_si64(flip2));
}
