/**
 * @file count_last_flip_avx512cd.c
 *
 * A function is provided to count the number of fipped disc of the last move.
 *
 * Count last flip using the flip_avx512cd way.
 * For optimization purpose, the value returned is twice the number of flipped
 * disc, to facilitate the computation of disc difference.
 *
<<<<<<< HEAD
<<<<<<< HEAD
 * @date 2023 - 2024
=======
 * @date 2023
>>>>>>> 52949e1 (Add build options and files for new count_last_flips)
=======
 * @date 2023 - 2024
>>>>>>> ba1be42 (AVX512 last flip with lastflip_highcut)
 * @author Toshihiko Okuhara
 * @version 4.5
 * 
 */

#include "bit.h"

<<<<<<< HEAD
<<<<<<< HEAD
extern	const V8DI lrmask[66];
=======
extern	const V4DI lmask_v4[66], rmask_v4[66];
>>>>>>> 52949e1 (Add build options and files for new count_last_flips)
=======
extern	const V8DI lrmask[66];
>>>>>>> ba1be42 (AVX512 last flip with lastflip_highcut)

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
<<<<<<< HEAD
<<<<<<< HEAD
	__m256i	flip, outflank, eraser, rmask, lmask;
	__m128i	flip2;

		// left: look for player LS1B
	lmask = lrmask[pos].v4[0];
	outflank = _mm256_and_si256(PP, lmask);
		// set below LS1B if P is in lmask
	flip = _mm256_maskz_add_epi64(_mm256_test_epi64_mask(PP, lmask), outflank, _mm256_set1_epi64x(-1));
	// flip = _mm256_and_si256(_mm256_andnot_si256(outflank, flip), lmask);
	flip = _mm256_ternarylogic_epi64(outflank, flip, lmask, 0x08);
<<<<<<< HEAD

		// right: look for player bit with lzcnt
	rmask = lrmask[pos].v4[1];
	eraser = _mm256_srlv_epi64(_mm256_set1_epi64x(-1),
		_mm256_maskz_lzcnt_epi64(_mm256_test_epi64_mask(PP, rmask), _mm256_and_si256(PP, rmask)));
	// flip = _mm256_or_si256(flip, _mm256_andnot_si256(eraser, rmask));
	flip = _mm256_ternarylogic_epi64(flip, eraser, rmask, 0xf2);

	flip2 = _mm_or_si128(_mm256_castsi256_si128(flip), _mm256_extracti128_si256(flip, 1));
	return 2 * bit_count(_mm_cvtsi128_si64(_mm_or_si128(flip2, _mm_unpackhi_epi64(flip2, flip2))));
=======
	__m256i	flip, outflank, rmask, lmask;
=======
	__m256i	flip, outflank, eraser, rmask, lmask;
>>>>>>> ba1be42 (AVX512 last flip with lastflip_highcut)
	__m128i	flip2;

		// left: look for player LS1B
	lmask = lrmask[pos].v4[0];
	outflank = _mm256_and_si256(PP, lmask);
		// set below LS1B if P is in lmask
	// flip = _mm256_andnot_si256(outflank, _mm256_add_epi64(outflank, _mm256_set1_epi64x(-1)));
	// flip = _mm256_maskz_and_epi64(_mm256_test_epi64_mask(PP, lmask), flip, lmask);
	flip = _mm256_maskz_ternarylogic_epi64(_mm256_test_epi64_mask(PP, lmask),
		outflank, _mm256_add_epi64(outflank, _mm256_set1_epi64x(-1)), lmask, 0x08);
=======
>>>>>>> eb84eb8 (Revise avx512 mask usage to ease ternarylogic opt)

		// right: look for player bit with lzcnt
	rmask = lrmask[pos].v4[1];
	eraser = _mm256_srlv_epi64(_mm256_set1_epi64x(-1),
		_mm256_maskz_lzcnt_epi64(_mm256_test_epi64_mask(PP, rmask), _mm256_and_si256(PP, rmask)));
	// flip = _mm256_or_si256(flip, _mm256_andnot_si256(eraser, rmask));
	flip = _mm256_ternarylogic_epi64(flip, eraser, rmask, 0xf2);

	flip2 = _mm_or_si128(_mm256_castsi256_si128(flip), _mm256_extracti128_si256(flip, 1));
<<<<<<< HEAD
	flip2 = _mm_or_si128(flip2, _mm_shuffle_epi32(flip2, 0x4e));
	return 2 * bit_count(_mm_cvtsi128_si64(flip2));
>>>>>>> 52949e1 (Add build options and files for new count_last_flips)
=======
	return 2 * bit_count(_mm_cvtsi128_si64(_mm_or_si128(flip2, _mm_unpackhi_epi64(flip2, flip2))));
>>>>>>> eb84eb8 (Revise avx512 mask usage to ease ternarylogic opt)
}
