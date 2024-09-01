/**
 * @file eval_sse.c
 *
 * SSE/AVX translation of some eval.c functions
 *
 * @date 2018 - 2023
 * @author Toshihiko Okuhara
 * @version 4.5
 */

#include <assert.h>

#include "bit_intrinsics.h"
#include "board.h"
#include "move.h"
#include "eval.h"

extern const EVAL_FEATURE_V EVAL_FEATURE[65];
extern const EVAL_FEATURE_V EVAL_FEATURE_all_opponent;

#ifdef __ARM_NEON
#define __m128i		int16x8_t
#define	_mm_add_epi16	vaddq_s16
#define _mm_sub_epi16	vsubq_s16
#define _mm_slli_epi16	vshlq_n_s16
#endif

#if defined(hasSSE2) || defined(__ARM_NEON) || defined(USE_MSVC_X86)

void eval_update_sse(int x, unsigned long long f, Eval *eval_out, const Eval *eval_in)
{
  #ifdef __AVX2__
	__m256i	f0 = eval_in->feature.v16[0];
	__m256i	f1 = eval_in->feature.v16[1];
	__m256i	f2 = eval_in->feature.v16[2];

	if (eval_in->n_empties & 1) {
		f0 = _mm256_sub_epi16(f0, EVAL_FEATURE[x].v16[0]);
		f1 = _mm256_sub_epi16(f1, EVAL_FEATURE[x].v16[1]);
		f2 = _mm256_sub_epi16(f2, EVAL_FEATURE[x].v16[2]);

		foreach_bit (x, f) {
			f0 = _mm256_add_epi16(f0, EVAL_FEATURE[x].v16[0]);
			f1 = _mm256_add_epi16(f1, EVAL_FEATURE[x].v16[1]);
			f2 = _mm256_add_epi16(f2, EVAL_FEATURE[x].v16[2]);
		}

	} else {
		f0 = _mm256_sub_epi16(f0, _mm256_slli_epi16(EVAL_FEATURE[x].v16[0], 1));
		f1 = _mm256_sub_epi16(f1, _mm256_slli_epi16(EVAL_FEATURE[x].v16[1], 1));
		f2 = _mm256_sub_epi16(f2, _mm256_slli_epi16(EVAL_FEATURE[x].v16[2], 1));

		foreach_bit (x, f) {
			f0 = _mm256_sub_epi16(f0, EVAL_FEATURE[x].v16[0]);
			f1 = _mm256_sub_epi16(f1, EVAL_FEATURE[x].v16[1]);
			f2 = _mm256_sub_epi16(f2, EVAL_FEATURE[x].v16[2]);
		}
	}

	eval_out->feature.v16[0] = f0;
	eval_out->feature.v16[1] = f1;
	eval_out->feature.v16[2] = f2;

  #else
	__m128i	f0 = eval_in->feature.v8[0];
	__m128i	f1 = eval_in->feature.v8[1];
	__m128i	f2 = eval_in->feature.v8[2];
	__m128i	f3 = eval_in->feature.v8[3];
	__m128i	f4 = eval_in->feature.v8[4];
	__m128i	f5 = eval_in->feature.v8[5];

	if (eval_in->n_empties & 1) {
		f0 = _mm_sub_epi16(f0, EVAL_FEATURE[x].v8[0]);
		f1 = _mm_sub_epi16(f1, EVAL_FEATURE[x].v8[1]);
		f2 = _mm_sub_epi16(f2, EVAL_FEATURE[x].v8[2]);
		f3 = _mm_sub_epi16(f3, EVAL_FEATURE[x].v8[3]);
		f4 = _mm_sub_epi16(f4, EVAL_FEATURE[x].v8[4]);
		f5 = _mm_sub_epi16(f5, EVAL_FEATURE[x].v8[5]);

		foreach_bit (x, f) {
			f0 = _mm_add_epi16(f0, EVAL_FEATURE[x].v8[0]);
			f1 = _mm_add_epi16(f1, EVAL_FEATURE[x].v8[1]);
			f2 = _mm_add_epi16(f2, EVAL_FEATURE[x].v8[2]);
			f3 = _mm_add_epi16(f3, EVAL_FEATURE[x].v8[3]);
			f4 = _mm_add_epi16(f4, EVAL_FEATURE[x].v8[4]);
			f5 = _mm_add_epi16(f5, EVAL_FEATURE[x].v8[5]);
		}

	} else {
		f0 = _mm_sub_epi16(f0, _mm_slli_epi16(EVAL_FEATURE[x].v8[0], 1));
		f1 = _mm_sub_epi16(f1, _mm_slli_epi16(EVAL_FEATURE[x].v8[1], 1));
		f2 = _mm_sub_epi16(f2, _mm_slli_epi16(EVAL_FEATURE[x].v8[2], 1));
		f3 = _mm_sub_epi16(f3, _mm_slli_epi16(EVAL_FEATURE[x].v8[3], 1));
		f4 = _mm_sub_epi16(f4, _mm_slli_epi16(EVAL_FEATURE[x].v8[4], 1));
		f5 = _mm_sub_epi16(f5, _mm_slli_epi16(EVAL_FEATURE[x].v8[5], 1));

		foreach_bit (x, f) {
			f0 = _mm_sub_epi16(f0, EVAL_FEATURE[x].v8[0]);
			f1 = _mm_sub_epi16(f1, EVAL_FEATURE[x].v8[1]);
			f2 = _mm_sub_epi16(f2, EVAL_FEATURE[x].v8[2]);
			f3 = _mm_sub_epi16(f3, EVAL_FEATURE[x].v8[3]);
			f4 = _mm_sub_epi16(f4, EVAL_FEATURE[x].v8[4]);
			f5 = _mm_sub_epi16(f5, EVAL_FEATURE[x].v8[5]);
		}
	}

	eval_out->feature.v8[0] = f0;
	eval_out->feature.v8[1] = f1;
	eval_out->feature.v8[2] = f2;
	eval_out->feature.v8[3] = f3;
	eval_out->feature.v8[4] = f4;
	eval_out->feature.v8[5] = f5;
  #endif
}

#else	// SSE dispatch (Eval may not be aligned)

void eval_update_sse(int x, unsigned long long f, Eval *eval_out, const Eval *eval_in)
{
	__asm__ (
		"movdqu	%0, %%xmm2\n\t"
		"movdqu	%1, %%xmm3\n\t"
		"movdqu	%2, %%xmm4\n\t"
		"movdqu	%3, %%xmm5\n\t"
		"movdqu	%4, %%xmm6\n\t"
		"movdqu	%5, %%xmm7"
	: :  "m" (eval_in->feature.us[0]), "m" (eval_in->feature.us[8]),  "m" (eval_in->feature.us[16]),
	"m" (eval_in->feature.us[24]), "m" (eval_in->feature.us[32]), "m" (eval_in->feature.us[40]));

	if (eval_in->n_empties & 1) {
		__asm__ (
			"psubw	%0, %%xmm2\n\t"
			"psubw	%1, %%xmm3\n\t"
			"psubw	%2, %%xmm4\n\t"
			"psubw	%3, %%xmm5\n\t"
			"psubw	%4, %%xmm6\n\t"
			"psubw	%5, %%xmm7"
		: :  "m" (EVAL_FEATURE[x].us[0]), "m" (EVAL_FEATURE[x].us[8]), "m" (EVAL_FEATURE[x].us[16]),
		"m" (EVAL_FEATURE[x].us[24]), "m" (EVAL_FEATURE[x].us[32]), "m" (EVAL_FEATURE[x].us[40]));

		foreach_bit (x, f) {
			__asm__ (
				"paddw	%0, %%xmm2\n\t"
				"paddw	%1, %%xmm3\n\t"
				"paddw	%2, %%xmm4\n\t"
				"paddw	%3, %%xmm5\n\t"
				"paddw	%4, %%xmm6\n\t"
				"paddw	%5, %%xmm7"
			: :  "m" (EVAL_FEATURE[x].us[0]), "m" (EVAL_FEATURE[x].us[8]), "m" (EVAL_FEATURE[x].us[16]),
			"m" (EVAL_FEATURE[x].us[24]), "m" (EVAL_FEATURE[x].us[32]), "m" (EVAL_FEATURE[x].us[40]));
		}

	} else {
		__asm__ (
			"movdqa	%0, %%xmm0\n\t"		"movdqa	%1, %%xmm1\n\t"
			"psllw	$1, %%xmm0\n\t"		"psllw	$1, %%xmm1\n\t"
			"psubw	%%xmm0, %%xmm2\n\t"	"psubw	%%xmm1, %%xmm3\n\t"
			"movdqa	%2, %%xmm0\n\t"		"movdqa	%3, %%xmm1\n\t"
			"psllw	$1, %%xmm0\n\t"		"psllw	$1, %%xmm1\n\t"
			"psubw	%%xmm0, %%xmm4\n\t"	"psubw	%%xmm1, %%xmm5\n\t"
			"movdqa	%4, %%xmm0\n\t"		"movdqa	%5, %%xmm1\n\t"
			"psllw	$1, %%xmm0\n\t"		"psllw	$1, %%xmm1\n\t"
			"psubw	%%xmm0, %%xmm6\n\t"	"psubw	%%xmm1, %%xmm7"
		: :  "m" (EVAL_FEATURE[x].us[0]), "m" (EVAL_FEATURE[x].us[8]), "m" (EVAL_FEATURE[x].us[16]),
		"m" (EVAL_FEATURE[x].us[24]), "m" (EVAL_FEATURE[x].us[32]), "m" (EVAL_FEATURE[x].us[40]));

		foreach_bit (x, f) {
			__asm__ (
				"psubw	%0, %%xmm2\n\t"
				"psubw	%1, %%xmm3\n\t"
				"psubw	%2, %%xmm4\n\t"
				"psubw	%3, %%xmm5\n\t"
				"psubw	%4, %%xmm6\n\t"
				"psubw	%5, %%xmm7"
			: :  "m" (EVAL_FEATURE[x].us[0]), "m" (EVAL_FEATURE[x].us[8]), "m" (EVAL_FEATURE[x].us[16]),
			"m" (EVAL_FEATURE[x].us[24]), "m" (EVAL_FEATURE[x].us[32]), "m" (EVAL_FEATURE[x].us[40]));
		}
	}

	__asm__ (
		"movdqu	%%xmm2, %0\n\t"
		"movdqu	%%xmm3, %1\n\t"
		"movdqu	%%xmm4, %2\n\t"
		"movdqu	%%xmm5, %3\n\t"
		"movdqu	%%xmm6, %4\n\t"
		"movdqu	%%xmm7, %5"
	: :  "m" (eval_out->feature.us[0]), "m" (eval_out->feature.us[8]), "m" (eval_out->feature.us[16]),
	"m" (eval_out->feature.us[24]), "m" (eval_out->feature.us[32]), "m" (eval_out->feature.us[40]));
}

#endif // hasSSE2

#if defined(hasSSE2) || (defined(__ARM_NEON) && !defined(DISPATCH_NEON))

/**
 * @brief Set up evaluation features from a board.
 *
 * @param eval  Evaluation function.
 * @param board Board to setup features from.
 */
void eval_set(Eval *eval, const Board *board)
{
	int x;
	unsigned long long b = (eval->n_empties & 1) ? board->opponent : board->player;
  #ifdef __AVX2__
	__m256i	f0 = EVAL_FEATURE_all_opponent.v16[0];
	__m256i	f1 = EVAL_FEATURE_all_opponent.v16[1];
	__m256i	f2 = EVAL_FEATURE_all_opponent.v16[2];

	foreach_bit (x, b) {
		f0 = _mm256_sub_epi16(f0, EVAL_FEATURE[x].v16[0]);
		f1 = _mm256_sub_epi16(f1, EVAL_FEATURE[x].v16[1]);
		f2 = _mm256_sub_epi16(f2, EVAL_FEATURE[x].v16[2]);
	}

	b = ~(board->opponent | board->player);
	foreach_bit (x, b) {
		f0 = _mm256_add_epi16(f0, EVAL_FEATURE[x].v16[0]);
		f1 = _mm256_add_epi16(f1, EVAL_FEATURE[x].v16[1]);
		f2 = _mm256_add_epi16(f2, EVAL_FEATURE[x].v16[2]);
	}
	eval->feature.v16[0] = f0;
	eval->feature.v16[1] = f1;
	eval->feature.v16[2] = f2;

  #else
	__m128i	f0 = EVAL_FEATURE_all_opponent.v8[0];
	__m128i	f1 = EVAL_FEATURE_all_opponent.v8[1];
	__m128i	f2 = EVAL_FEATURE_all_opponent.v8[2];
	__m128i	f3 = EVAL_FEATURE_all_opponent.v8[3];
	__m128i	f4 = EVAL_FEATURE_all_opponent.v8[4];
	__m128i	f5 = EVAL_FEATURE_all_opponent.v8[5];

	foreach_bit (x, b) {
		f0 = _mm_sub_epi16(f0, EVAL_FEATURE[x].v8[0]);
		f1 = _mm_sub_epi16(f1, EVAL_FEATURE[x].v8[1]);
		f2 = _mm_sub_epi16(f2, EVAL_FEATURE[x].v8[2]);
		f3 = _mm_sub_epi16(f3, EVAL_FEATURE[x].v8[3]);
		f4 = _mm_sub_epi16(f4, EVAL_FEATURE[x].v8[4]);
		f5 = _mm_sub_epi16(f5, EVAL_FEATURE[x].v8[5]);
	}

	b = ~(board->opponent | board->player);
	foreach_bit (x, b) {
		f0 = _mm_add_epi16(f0, EVAL_FEATURE[x].v8[0]);
		f1 = _mm_add_epi16(f1, EVAL_FEATURE[x].v8[1]);
		f2 = _mm_add_epi16(f2, EVAL_FEATURE[x].v8[2]);
		f3 = _mm_add_epi16(f3, EVAL_FEATURE[x].v8[3]);
		f4 = _mm_add_epi16(f4, EVAL_FEATURE[x].v8[4]);
		f5 = _mm_add_epi16(f5, EVAL_FEATURE[x].v8[5]);
	}

	eval->feature.v8[0] = f0;
	eval->feature.v8[1] = f1;
	eval->feature.v8[2] = f2;
	eval->feature.v8[3] = f3;
	eval->feature.v8[4] = f4;
	eval->feature.v8[5] = f5;
  #endif
}

#endif // hasSSE2
