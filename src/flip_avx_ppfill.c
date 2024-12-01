/**
 * @file flip_avx_ppfill.c
 *
 * This module deals with flipping discs.
 *
 * For LSB to MSB directions, isolate LS1B can be used to determine
 * contiguous opponent discs.
 * For MSB to LSB directions, parallel prefix fill is used to isolate
 * MS1B.
 *
 * @date 1998 - 2024
 * @author Toshihiko Okuhara
 * @version 4.6
 */

#include "simd.h"

extern const V8DI MASK_LR[66];

/**
 * Compute flipped discs when playing on square pos.
 *
 * @param pos player's move.
 * @param P player's disc pattern.
 * @param O opponent's disc pattern.
 * @return partially reduced flipped disc pattern.
 */

__m128i vectorcall mm_flip(const __m128i OP, const int pos)
{
	__m256i	PP, OO, flip, outflank, eraser, mask;

	PP = _mm256_broadcastq_epi64(OP);
	OO = _mm256_broadcastq_epi64(_mm_unpackhi_epi64(OP, OP));

	mask = MASK_LR[pos].v4[1];
	// isolate non-opponent MS1B by clearing lower shadow bits
	eraser = _mm256_andnot_si256(OO, mask);

	// clear valid bits only using variable shift
	outflank = _mm256_sllv_epi64(_mm256_and_si256(PP, mask), _mm256_set_epi64x(7, 9, 8, 1));
	eraser = _mm256_or_si256(eraser, _mm256_srlv_epi64(eraser, _mm256_set_epi64x(7, 9, 8, 1)));
	outflank = _mm256_andnot_si256(eraser, outflank);
	eraser = _mm256_srlv_epi64(eraser, _mm256_set_epi64x(14, 18, 16, 2));
	outflank = _mm256_andnot_si256(eraser, outflank);
	outflank = _mm256_andnot_si256(_mm256_srlv_epi64(eraser, _mm256_set_epi64x(14, 18, 16, 2)), outflank);

	// set mask bits higher than outflank
	flip = _mm256_and_si256(mask, _mm256_sub_epi64(_mm256_setzero_si256(), outflank));

	mask = MASK_LR[pos].v4[0];
	// look for non-opponent LS1B
	outflank = _mm256_andnot_si256(OO, mask);
	outflank = _mm256_and_si256(outflank, _mm256_sub_epi64(_mm256_setzero_si256(), outflank));	// LS1B
	outflank = _mm256_and_si256(outflank, PP);
	// set all bits if outflank = 0, otherwise higher bits than outflank
	eraser = _mm256_sub_epi64(_mm256_cmpeq_epi64(outflank, _mm256_setzero_si256()), outflank);
	flip = _mm256_or_si256(flip, _mm256_andnot_si256(eraser, mask));

	return _mm_or_si128(_mm256_castsi256_si128(flip), _mm256_extracti128_si256(flip, 1));
}

uint64_t board_flip(const Board *board, const int x) 
{
	__m128i flip = mm_flip(_mm_loadu_si128((__m128i *) board), x);
	__m128i rflip = _mm_or_si128(flip, _mm_shuffle_epi32(flip, 0x4e));
	return (uint64_t) _mm_cvtsi128_si64(rflip);
}

uint64_t flip(const int x, const uint64_t P, const uint64_t O) {
	__m128i flip = mm_flip(_mm_set_epi64x((O), (P)), x);
	__m128i rflip = _mm_or_si128(flip, _mm_shuffle_epi32(flip, 0x4e));
	return (uint64_t) _mm_cvtsi128_si64(rflip);
}	


