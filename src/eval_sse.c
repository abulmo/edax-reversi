<<<<<<< HEAD
<<<<<<< HEAD
=======
>>>>>>> 3e1ed4f (fix cr/lf in repository to lf)
/**
 * @file eval_sse.c
 *
 * SSE/AVX translation of some eval.c functions
 *
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
 * @date 2018 - 2023
 * @author Toshihiko Okuhara
 * @version 4.5
=======
 * @date 2018 - 2020
 * @author Toshihiko Okuhara
 * @version 4.4
>>>>>>> 3e1ed4f (fix cr/lf in repository to lf)
=======
 * @date 2018 - 2022
=======
 * @date 2018 - 2023
>>>>>>> 4087529 (Revise board0 usage; fix unused flips)
 * @author Toshihiko Okuhara
 * @version 4.5
>>>>>>> 534241b (Revise foreach_bit_r and first_bit_32)
 */

#include <assert.h>

<<<<<<< HEAD
<<<<<<< HEAD
#include "bit_intrinsics.h"
=======
#include "bit.h"
>>>>>>> 3e1ed4f (fix cr/lf in repository to lf)
=======
#include "bit_intrinsics.h"
>>>>>>> 343493d (More neon/sse optimizations; neon dispatch added for arm32)
#include "board.h"
#include "move.h"
#include "eval.h"

<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
extern const EVAL_FEATURE_V EVAL_FEATURE[65];
extern const EVAL_FEATURE_V EVAL_FEATURE_all_opponent;

#ifdef __ARM_NEON
<<<<<<< HEAD
=======
#if defined(__ARM_NEON__) || defined(_M_ARM) || defined(_M_ARM64)
>>>>>>> f2da03e (Refine arm builds adding neon support.)
=======
=======
extern const EVAL_FEATURE_V EVAL_FEATURE[65];
extern const EVAL_FEATURE_V EVAL_FEATURE_all_opponent;

>>>>>>> 1e01a49 (Change EVAL_FEATURE to struct for readability; decrease EVAL_N_PLY)
#ifdef __ARM_NEON__
>>>>>>> 343493d (More neon/sse optimizations; neon dispatch added for arm32)
=======
>>>>>>> 520040b (Use DISPATCH_NEON, not hasNeon, for android arm32 build)
#define __m128i		int16x8_t
#define	_mm_add_epi16	vaddq_s16
#define _mm_sub_epi16	vsubq_s16
#define _mm_slli_epi16	vshlq_n_s16
<<<<<<< HEAD
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

=======
typedef union {
	unsigned short us[48];
#if defined(hasSSE2) || defined(USE_MSVC_X86)
	__m128i	v8[6];
=======
>>>>>>> f2da03e (Refine arm builds adding neon support.)
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
<<<<<<< HEAD
>>>>>>> 3e1ed4f (fix cr/lf in repository to lf)
=======

>>>>>>> 6820748 (Unify eval_update_sse 0 & 1)
	eval_out->feature.v16[0] = f0;
	eval_out->feature.v16[1] = f1;
	eval_out->feature.v16[2] = f2;

<<<<<<< HEAD
<<<<<<< HEAD
  #else
	__m128i	f0 = eval_in->feature.v8[0];
	__m128i	f1 = eval_in->feature.v8[1];
	__m128i	f2 = eval_in->feature.v8[2];
	__m128i	f3 = eval_in->feature.v8[3];
	__m128i	f4 = eval_in->feature.v8[4];
	__m128i	f5 = eval_in->feature.v8[5];
<<<<<<< HEAD

	if (eval_in->n_empties & 1) {
=======
#else
	int	j;
	widest_register	b;
=======
  #else
>>>>>>> 534241b (Revise foreach_bit_r and first_bit_32)
	const __m128i *ef;

<<<<<<< HEAD
#ifdef HAS_CPU_64
	foreach_bit(x, f) {
>>>>>>> 3e1ed4f (fix cr/lf in repository to lf)
=======

	if (eval_in->n_empties & 1) {
>>>>>>> 6820748 (Unify eval_update_sse 0 & 1)
		f0 = _mm_sub_epi16(f0, EVAL_FEATURE[x].v8[0]);
		f1 = _mm_sub_epi16(f1, EVAL_FEATURE[x].v8[1]);
		f2 = _mm_sub_epi16(f2, EVAL_FEATURE[x].v8[2]);
		f3 = _mm_sub_epi16(f3, EVAL_FEATURE[x].v8[3]);
		f4 = _mm_sub_epi16(f4, EVAL_FEATURE[x].v8[4]);
		f5 = _mm_sub_epi16(f5, EVAL_FEATURE[x].v8[5]);
<<<<<<< HEAD
<<<<<<< HEAD

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
<<<<<<< HEAD
			f0 = _mm_sub_epi16(f0, EVAL_FEATURE[x].v8[0]);
			f1 = _mm_sub_epi16(f1, EVAL_FEATURE[x].v8[1]);
			f2 = _mm_sub_epi16(f2, EVAL_FEATURE[x].v8[2]);
			f3 = _mm_sub_epi16(f3, EVAL_FEATURE[x].v8[3]);
			f4 = _mm_sub_epi16(f4, EVAL_FEATURE[x].v8[4]);
			f5 = _mm_sub_epi16(f5, EVAL_FEATURE[x].v8[5]);
		}
	}

=======
	}

#else
	unsigned int	fl = (unsigned int) f;
	unsigned int	fh = (unsigned int) (f >> 32);

	foreach_bit_32(x, fl) {
		f0 = _mm_sub_epi16(f0, EVAL_FEATURE[x].v8[0]);
		f1 = _mm_sub_epi16(f1, EVAL_FEATURE[x].v8[1]);
		f2 = _mm_sub_epi16(f2, EVAL_FEATURE[x].v8[2]);
		f3 = _mm_sub_epi16(f3, EVAL_FEATURE[x].v8[3]);
		f4 = _mm_sub_epi16(f4, EVAL_FEATURE[x].v8[4]);
		f5 = _mm_sub_epi16(f5, EVAL_FEATURE[x].v8[5]);
	}
	foreach_bit_32(x, fh) {
		f0 = _mm_sub_epi16(f0, EVAL_FEATURE[x + 32].v8[0]);
		f1 = _mm_sub_epi16(f1, EVAL_FEATURE[x + 32].v8[1]);
		f2 = _mm_sub_epi16(f2, EVAL_FEATURE[x + 32].v8[2]);
		f3 = _mm_sub_epi16(f3, EVAL_FEATURE[x + 32].v8[3]);
		f4 = _mm_sub_epi16(f4, EVAL_FEATURE[x + 32].v8[4]);
		f5 = _mm_sub_epi16(f5, EVAL_FEATURE[x + 32].v8[5]);
	}
#endif
>>>>>>> 3e1ed4f (fix cr/lf in repository to lf)
=======
	ef = EVAL_FEATURE[x].v8;
	__m128i	f0 = _mm_sub_epi16(eval_in->feature.v8[0], _mm_slli_epi16(ef[0], 1));
	__m128i	f1 = _mm_sub_epi16(eval_in->feature.v8[1], _mm_slli_epi16(ef[1], 1));
	__m128i	f2 = _mm_sub_epi16(eval_in->feature.v8[2], _mm_slli_epi16(ef[2], 1));
	__m128i	f3 = _mm_sub_epi16(eval_in->feature.v8[3], _mm_slli_epi16(ef[3], 1));
	__m128i	f4 = _mm_sub_epi16(eval_in->feature.v8[4], _mm_slli_epi16(ef[4], 1));
	__m128i	f5 = _mm_sub_epi16(eval_in->feature.v8[5], _mm_slli_epi16(ef[5], 1));
=======
>>>>>>> 6820748 (Unify eval_update_sse 0 & 1)

		foreach_bit_r (x, f, j, r) {
			f0 = _mm_add_epi16(f0, EVAL_FEATURE[x].v8[0]);
			f1 = _mm_add_epi16(f1, EVAL_FEATURE[x].v8[1]);
			f2 = _mm_add_epi16(f2, EVAL_FEATURE[x].v8[2]);
			f3 = _mm_add_epi16(f3, EVAL_FEATURE[x].v8[3]);
			f4 = _mm_add_epi16(f4, EVAL_FEATURE[x].v8[4]);
			f5 = _mm_add_epi16(f5, EVAL_FEATURE[x].v8[5]);
		}

<<<<<<< HEAD
>>>>>>> f2da03e (Refine arm builds adding neon support.)
	eval_out->feature.v8[0] = f0;
	eval_out->feature.v8[1] = f1;
	eval_out->feature.v8[2] = f2;
	eval_out->feature.v8[3] = f3;
	eval_out->feature.v8[4] = f4;
	eval_out->feature.v8[5] = f5;
<<<<<<< HEAD
<<<<<<< HEAD
  #endif
=======
#endif
=======
  #endif
>>>>>>> 534241b (Revise foreach_bit_r and first_bit_32)
}
=======
	} else {
		f0 = _mm_sub_epi16(f0, _mm_slli_epi16(EVAL_FEATURE[x].v8[0], 1));
		f1 = _mm_sub_epi16(f1, _mm_slli_epi16(EVAL_FEATURE[x].v8[1], 1));
		f2 = _mm_sub_epi16(f2, _mm_slli_epi16(EVAL_FEATURE[x].v8[2], 1));
		f3 = _mm_sub_epi16(f3, _mm_slli_epi16(EVAL_FEATURE[x].v8[3], 1));
		f4 = _mm_sub_epi16(f4, _mm_slli_epi16(EVAL_FEATURE[x].v8[4], 1));
		f5 = _mm_sub_epi16(f5, _mm_slli_epi16(EVAL_FEATURE[x].v8[5], 1));
>>>>>>> 6820748 (Unify eval_update_sse 0 & 1)

		foreach_bit_r (x, f, j, r) {
=======
>>>>>>> be2ba1c (add AVX get_potential_mobility; revise foreach_bit for CPU32/C99)
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
<<<<<<< HEAD
#endif
>>>>>>> 3e1ed4f (fix cr/lf in repository to lf)
=======
  #endif
>>>>>>> 534241b (Revise foreach_bit_r and first_bit_32)
}

#else	// SSE dispatch (Eval may not be aligned)

<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
void eval_update_sse(int x, unsigned long long f, Eval *eval_out, const Eval *eval_in)
{
<<<<<<< HEAD
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
=======
static void eval_update_sse_0(Eval *eval_out, const Eval *eval_in, const Move *move)
=======
static void eval_update_sse_0(int x, unsigned long long f, Eval *eval_out, const Eval *eval_in)
>>>>>>> 9b4cd06 (Optimize search_shallow in endgame.c; revise eval_update parameters)
=======
void eval_update_sse(int x, unsigned long long f, Eval *eval_out, const Eval *eval_in)
>>>>>>> 6820748 (Unify eval_update_sse 0 & 1)
{
	widest_register	r;
	int	j;

=======
>>>>>>> be2ba1c (add AVX get_potential_mobility; revise foreach_bit for CPU32/C99)
	__asm__ (
		"movdqu	%0, %%xmm2\n\t"
		"movdqu	%1, %%xmm3\n\t"
		"movdqu	%2, %%xmm4\n\t"
		"movdqu	%3, %%xmm5\n\t"
		"movdqu	%4, %%xmm6\n\t"
		"movdqu	%5, %%xmm7"
	: :  "m" (eval_in->feature.us[0]), "m" (eval_in->feature.us[8]),  "m" (eval_in->feature.us[16]),
	"m" (eval_in->feature.us[24]), "m" (eval_in->feature.us[32]), "m" (eval_in->feature.us[40]));

<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
	foreach_bit_32(x, fl) {
>>>>>>> 3e1ed4f (fix cr/lf in repository to lf)
=======
	foreach_bit_r(x, fl, b) {
>>>>>>> f2da03e (Refine arm builds adding neon support.)
=======
	foreach_bit_r (x, f, j, r) {
>>>>>>> 534241b (Revise foreach_bit_r and first_bit_32)
		__asm__ (
			"psubw	%0, %%xmm2\n\t"
			"psubw	%1, %%xmm3\n\t"
			"psubw	%2, %%xmm4\n\t"
			"psubw	%3, %%xmm5\n\t"
			"psubw	%4, %%xmm6\n\t"
<<<<<<< HEAD
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
=======
			"psubw	%5, %%xmm7\n"
		: :  "m" (EVAL_FEATURE[x].us[0]), "m" (EVAL_FEATURE[x].us[8]), "m" (EVAL_FEATURE[x].us[16]),
		"m" (EVAL_FEATURE[x].us[24]), "m" (EVAL_FEATURE[x].us[32]), "m" (EVAL_FEATURE[x].us[40]));
	}
<<<<<<< HEAD
	foreach_bit_r(x, fh, b) {
=======
	if (eval_in->n_empties & 1) {
>>>>>>> 6820748 (Unify eval_update_sse 0 & 1)
		__asm__ (
			"psubw	%0, %%xmm2\n\t"
			"psubw	%1, %%xmm3\n\t"
			"psubw	%2, %%xmm4\n\t"
			"psubw	%3, %%xmm5\n\t"
			"psubw	%4, %%xmm6\n\t"
<<<<<<< HEAD
			"psubw	%5, %%xmm7\n"
		: :  "m" (EVAL_FEATURE[x + 32].us[0]), "m" (EVAL_FEATURE[x + 32].us[8]), "m" (EVAL_FEATURE[x + 32].us[16]),
		"m" (EVAL_FEATURE[x + 32].us[24]), "m" (EVAL_FEATURE[x + 32].us[32]), "m" (EVAL_FEATURE[x + 32].us[40]));
>>>>>>> 3e1ed4f (fix cr/lf in repository to lf)
	}
=======
>>>>>>> 534241b (Revise foreach_bit_r and first_bit_32)

	__asm__ (
		"movdqu	%%xmm2, %0\n\t"
		"movdqu	%%xmm3, %1\n\t"
		"movdqu	%%xmm4, %2\n\t"
		"movdqu	%%xmm5, %3\n\t"
		"movdqu	%%xmm6, %4\n\t"
<<<<<<< HEAD
		"movdqu	%%xmm7, %5"
=======
		"movdqu	%%xmm7, %5\n\t"
	: :  "m" (eval_out->feature.us[0]), "m" (eval_out->feature.us[8]), "m" (eval_out->feature.us[16]),
	"m" (eval_out->feature.us[24]), "m" (eval_out->feature.us[32]), "m" (eval_out->feature.us[40]));
}
=======
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
>>>>>>> 6820748 (Unify eval_update_sse 0 & 1)

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
<<<<<<< HEAD
		"movdqu	%%xmm7, %5\n\t"
>>>>>>> 3e1ed4f (fix cr/lf in repository to lf)
=======
		"movdqu	%%xmm7, %5"
>>>>>>> 6820748 (Unify eval_update_sse 0 & 1)
	: :  "m" (eval_out->feature.us[0]), "m" (eval_out->feature.us[8]), "m" (eval_out->feature.us[16]),
	"m" (eval_out->feature.us[24]), "m" (eval_out->feature.us[32]), "m" (eval_out->feature.us[40]));
}

#endif // hasSSE2

<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
#if defined(hasSSE2) || (defined(__ARM_NEON) && !defined(DISPATCH_NEON))
=======
#ifdef hasSSE2
>>>>>>> 3e1ed4f (fix cr/lf in repository to lf)
=======
#if defined(hasSSE2) || defined(hasNeon)
>>>>>>> f2da03e (Refine arm builds adding neon support.)
=======
#if defined(hasSSE2) || (defined(__ARM_NEON) && !defined(DISPATCH_NEON))
>>>>>>> 520040b (Use DISPATCH_NEON, not hasNeon, for android arm32 build)

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
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
  #ifdef __AVX2__
=======
	static const EVAL_FEATURE_V EVAL_FEATURE_all_opponent = {{
		 9841,  9841,  9841,  9841, 29524, 29524, 29524, 29524, 29524, 29524, 29524, 29524, 29524, 29524, 29524, 29524,
		 3280,  3280,  3280,  3280,  3280,  3280,  3280,  3280,  3280,  3280,  3280,  3280,  3280,  3280,  1093,  1093,
		 1093,  1093,   364,   364,   364,   364,   121,   121,   121,   121,    40,    40,    40,    40,     0,     0
	}};
=======
>>>>>>> f2da03e (Refine arm builds adding neon support.)
#ifdef __AVX2__
>>>>>>> 3e1ed4f (fix cr/lf in repository to lf)
=======
	widest_register	r;
=======
>>>>>>> be2ba1c (add AVX get_potential_mobility; revise foreach_bit for CPU32/C99)
  #ifdef __AVX2__
>>>>>>> 534241b (Revise foreach_bit_r and first_bit_32)
	__m256i	f0 = EVAL_FEATURE_all_opponent.v16[0];
	__m256i	f1 = EVAL_FEATURE_all_opponent.v16[1];
	__m256i	f2 = EVAL_FEATURE_all_opponent.v16[2];

<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
	foreach_bit (x, b) {
=======
	foreach_bit(x, b) {
>>>>>>> 3e1ed4f (fix cr/lf in repository to lf)
=======
	foreach_bit_r (x, b, j, r) {
>>>>>>> 534241b (Revise foreach_bit_r and first_bit_32)
=======
	foreach_bit (x, b) {
>>>>>>> be2ba1c (add AVX get_potential_mobility; revise foreach_bit for CPU32/C99)
		f0 = _mm256_sub_epi16(f0, EVAL_FEATURE[x].v16[0]);
		f1 = _mm256_sub_epi16(f1, EVAL_FEATURE[x].v16[1]);
		f2 = _mm256_sub_epi16(f2, EVAL_FEATURE[x].v16[2]);
	}
<<<<<<< HEAD
<<<<<<< HEAD

	b = ~(board->opponent | board->player);
<<<<<<< HEAD
<<<<<<< HEAD
	foreach_bit (x, b) {
=======
=======

>>>>>>> f2da03e (Refine arm builds adding neon support.)
	b = ~(board->opponent | board->player);
	foreach_bit(x, b) {
>>>>>>> 3e1ed4f (fix cr/lf in repository to lf)
=======
	foreach_bit_r (x, b, j, r) {
>>>>>>> 534241b (Revise foreach_bit_r and first_bit_32)
=======
	foreach_bit (x, b) {
>>>>>>> be2ba1c (add AVX get_potential_mobility; revise foreach_bit for CPU32/C99)
		f0 = _mm256_add_epi16(f0, EVAL_FEATURE[x].v16[0]);
		f1 = _mm256_add_epi16(f1, EVAL_FEATURE[x].v16[1]);
		f2 = _mm256_add_epi16(f2, EVAL_FEATURE[x].v16[2]);
	}
	eval->feature.v16[0] = f0;
	eval->feature.v16[1] = f1;
	eval->feature.v16[2] = f2;

<<<<<<< HEAD
<<<<<<< HEAD
  #else
=======
#else
>>>>>>> 3e1ed4f (fix cr/lf in repository to lf)
=======
  #else
>>>>>>> 534241b (Revise foreach_bit_r and first_bit_32)
	__m128i	f0 = EVAL_FEATURE_all_opponent.v8[0];
	__m128i	f1 = EVAL_FEATURE_all_opponent.v8[1];
	__m128i	f2 = EVAL_FEATURE_all_opponent.v8[2];
	__m128i	f3 = EVAL_FEATURE_all_opponent.v8[3];
	__m128i	f4 = EVAL_FEATURE_all_opponent.v8[4];
	__m128i	f5 = EVAL_FEATURE_all_opponent.v8[5];

<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
	foreach_bit (x, b) {
=======
	foreach_bit(x, b) {
>>>>>>> 3e1ed4f (fix cr/lf in repository to lf)
=======
	foreach_bit_r (x, b, j, r) {
>>>>>>> 534241b (Revise foreach_bit_r and first_bit_32)
=======
	foreach_bit (x, b) {
>>>>>>> be2ba1c (add AVX get_potential_mobility; revise foreach_bit for CPU32/C99)
		f0 = _mm_sub_epi16(f0, EVAL_FEATURE[x].v8[0]);
		f1 = _mm_sub_epi16(f1, EVAL_FEATURE[x].v8[1]);
		f2 = _mm_sub_epi16(f2, EVAL_FEATURE[x].v8[2]);
		f3 = _mm_sub_epi16(f3, EVAL_FEATURE[x].v8[3]);
		f4 = _mm_sub_epi16(f4, EVAL_FEATURE[x].v8[4]);
		f5 = _mm_sub_epi16(f5, EVAL_FEATURE[x].v8[5]);
	}
<<<<<<< HEAD
<<<<<<< HEAD

	b = ~(board->opponent | board->player);
<<<<<<< HEAD
<<<<<<< HEAD
	foreach_bit (x, b) {
=======
=======

>>>>>>> f2da03e (Refine arm builds adding neon support.)
	b = ~(board->opponent | board->player);
	foreach_bit(x, b) {
>>>>>>> 3e1ed4f (fix cr/lf in repository to lf)
=======
	foreach_bit_r (x, b, j, r) {
>>>>>>> 534241b (Revise foreach_bit_r and first_bit_32)
=======
	foreach_bit (x, b) {
>>>>>>> be2ba1c (add AVX get_potential_mobility; revise foreach_bit for CPU32/C99)
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
<<<<<<< HEAD
<<<<<<< HEAD
  #endif
}

<<<<<<< HEAD
#endif // hasSSE2
=======
/**
 * @file eval_sse.c
 *
 * SSE/AVX translation of some eval.c functions
 *
 * @date 2018 - 2020
 * @author Toshihiko Okuhara
 * @version 4.4
 */

#include <assert.h>

#include "bit.h"
#include "board.h"
#include "move.h"
#include "eval.h"

typedef union {
	unsigned short us[48];
#if defined(hasSSE2) || defined(USE_MSVC_X86)
	__m128i	v8[6];
#endif
#ifdef __AVX2__
	__m256i	v16[3];
#endif
}
#if defined(__GNUC__) && !defined(hasSSE2)
__attribute__ ((aligned (16)))
#endif
EVAL_FEATURE_V;

static const EVAL_FEATURE_V EVAL_FEATURE[65] = {
	{{ // a1
		 6561,     0,     0,     0,   243,     0,     0,     0,  6561,     0,  6561,     0, 19683,     0, 19683,     0,
		    0,     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,  2187,     0,     0,     0,
		    0,     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,     0
	}}, {{ // b1
		 2187,     0,     0,     0,    27,     0,     0,     0,  2187,     0,     0,     0,     0,     0,     0,     0,
		    0,     0,  2187,     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,   729,     0,
		    0,     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,     0
	}}, {{ // c1
		   81,     0,     0,     0,     9,     0,     0,     0,   729,     0,     0,     0,  6561,     0,     0,     0,
		    0,     0,     0,     0,     0,     0,  2187,     0,     0,     0,     0,     0,     0,     0,     0,     0,
		    0,     0,   243,     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,     0
	}}, {{ // d1
		    0,     0,     0,     0,     3,     1,     0,     0,   243,     0,     0,     0,  2187,     0,     0,     0,
		    0,     0,     0,     0,     0,     0,     0,     0,     0,     0,  2187,     0,     0,     0,     0,     0,
		    0,     0,     0,     0,     0,     0,    81,     0,     0,     0,    27,     0,     0,     0,     0,     0
	}}, {{ // e1
		    0,     0,     0,     0,     1,     3,     0,     0,    81,     0,     0,     0,     9,     0,     0,     0,
		    0,     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,  2187,     0,     0,     0,     0,
		    0,     0,     0,     0,     0,     0,     0,     0,    81,     0,     0,     0,    27,     0,     0,     0
	}}, {{ // f1
		    0,    81,     0,     0,     0,     9,     0,     0,    27,     0,     0,     0,     3,     0,     0,     0,
		    0,     0,     0,     0,     0,     0,     0,  2187,     0,     0,     0,     0,     0,     0,     0,     0,
		    0,     0,     0,     0,   243,     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,     0
	}}, {{ // g1
		    0,  2187,     0,     0,     0,    27,     0,     0,     9,     0,     0,     0,     0,     0,     0,     0,
		    0,     0,     0,  2187,     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
		    0,   729,     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,     0
	}}, {{ // h1
		    0,  6561,     0,     0,     0,   243,     0,     0,     3,     0,     0,  6561,     1,     0,     0, 19683,
		    0,     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,     1,     0,     0,
		    0,     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,     0
	}}, {{ // a2
		  729,     0,     0,     0,   729,     0,     0,     0,     0,     0,  2187,     0,     0,     0,     0,     0,
		 2187,     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
		  729,     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,     0
	}}, {{ // b2
		  243,     0,     0,     0,    81,     0,     0,     0, 19683,     0, 19683,     0,     0,     0,     0,     0,
		  729,     0,   729,     0,     0,     0,     0,     0,     0,     0,     0,     0,   729,     0,     0,     0,
		    0,     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,     0
	}}, {{ // c2
		    9,     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,   729,     0,     0,     0,
		  243,     0,     0,     0,     0,     0,   729,     0,     0,     0,     0,     0,     0,     0,   243,     0,
		    0,     0,     0,     0,     0,     0,     0,     0,     0,     0,     9,     0,     0,     0,     0,     0
	}}, {{ // d2
		    0,     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,   243,     0,     0,     0,
		   81,     0,     0,     0,     0,     0,     0,     0,     0,     0,   729,     0,     0,     0,     0,     0,
		    0,     0,    81,     0,     0,     0,     0,     0,    27,     0,     0,     0,     0,     0,     0,     0
	}}, {{ // e2
		    0,     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,    81,     0,     0,     0,
		   27,     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,   729,     0,     0,     0,     0,
		    0,     0,     0,     0,    81,     0,    27,     0,     0,     0,     0,     0,     0,     0,     0,     0
	}}, {{ // f2
		    0,     9,     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,    27,     0,     0,     0,
		    9,     0,     0,     0,     0,     0,     0,   729,     0,     0,     0,     0,     0,     0,     0,     0,
		    0,   243,     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,     9,     0,     0,     0
	}}, {{ // g2
		    0,   243,     0,     0,     0,    81,     0,     0,     1,     0,     0, 19683,     0,     0,     0,     0,
		    3,     0,     0,   729,     0,     0,     0,     0,     0,     0,     0,     0,     0,     3,     0,     0,
		    0,     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,     0
	}}, {{ // h2
		    0,   729,     0,     0,     0,   729,     0,     0,     0,     0,     0,  2187,     0,     0,     0,     0,
		    1,     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,   729,
		    0,     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,     0
	}}, {{ // a3
		   27,     0,     0,     0,  2187,     0,     0,     0,     0,     0,   729,     0,     0,     0,  6561,     0,
		    0,     0,     0,     0,  2187,     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
		    0,     0,     0,   243,     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,     0
	}}, {{ // b3
		    3,     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,   729,     0,
		    0,     0,   243,     0,   729,     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
		  243,     0,     0,     0,     0,     0,     0,     0,     0,     0,     3,     0,     0,     0,     0,     0
	}}, {{ // c3
		    1,     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
		    0,     0,     0,     0,   243,     0,   243,     0,     0,     0,     0,     0,   243,     0,     0,     0,
		    0,     0,     0,     0,     0,     0,     0,     0,     9,     0,     0,     0,     0,     0,     0,     0
	}}, {{ // d3
		    0,     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
		    0,     0,     0,     0,    81,     0,     0,     0,     0,     0,   243,     0,     0,     0,    81,     0,
		    0,     0,     0,     0,    27,     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,     0
	}}, {{ // e3
		    0,     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
		    0,     0,     0,     0,    27,     0,     0,     0,     0,     0,     0,   243,     0,     0,     0,     0,
		    0,    81,    27,     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,     0
	}}, {{ // f3
		    0,     1,     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
		    0,     0,     0,     0,     9,     0,     0,   243,     0,     0,     0,     0,     0,     9,     0,     0,
		    0,     0,     0,     0,     0,     0,     9,     0,     0,     0,     0,     0,     0,     0,     0,     0
	}}, {{ // g3
		    0,     3,     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,   729,
		    0,     0,     0,   243,     3,     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,   243,
		    0,     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,     3,     0,     0,     0
	}}, {{ // h3
		    0,    27,     0,     0,     0,  2187,     0,     0,     0,     0,     0,   729,     0,     0,     0,  6561,
		    0,     0,     0,     0,     1,     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
		    0,     0,     0,     0,     0,   243,     0,     0,     0,     0,     0,     0,     0,     0,     0,     0
	}}, {{ // a4
		    0,     0,     0,     0,  6561,     0, 19683,     0,     0,     0,   243,     0,     0,     0,  2187,     0,
		    0,     0,     0,     0,     0,     0,     0,     0,  2187,     0,     0,     0,     0,     0,     0,     0,
		    0,     0,     0,     0,     0,     0,     0,    81,     0,     0,     1,     0,     0,     0,     0,     0
	}}, {{ // b4
		    0,     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,   243,     0,
		    0,     0,    81,     0,     0,     0,     0,     0,   729,     0,     0,     0,     0,     0,     0,     0,
		    0,     0,     0,    81,     0,     0,     0,     0,     3,     0,     0,     0,     0,     0,     0,     0
	}}, {{ // c4
		    0,     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
		    0,     0,     0,     0,     0,     0,    81,     0,   243,     0,     0,     0,     0,     0,     0,     0,
		   81,     0,     0,     0,     9,     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,     0
	}}, {{ // d4
		    0,     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
		    0,     0,     0,     0,     0,     0,     0,     0,    81,     0,    81,     0,    81,     0,     0,     0,
		    0,    27,     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,     0
	}}, {{ // e4
		    0,     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
		    0,     0,     0,     0,     0,     0,     0,     0,    27,     0,     0,    81,     0,    27,    27,     0,
		    0,     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,     0
	}}, {{ // f4
		    0,     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
		    0,     0,     0,     0,     0,     0,     0,    81,     9,     0,     0,     0,     0,     0,     0,    81,
		    0,     0,     9,     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,     0
	}}, {{ // g4
		    0,     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,   243,
		    0,     0,     0,    81,     0,     0,     0,     0,     3,     0,     0,     0,     0,     0,     0,     0,
		    0,     0,     0,     0,     0,    81,     3,     0,     0,     0,     0,     0,     0,     0,     0,     0
	}}, {{ // h4
		    0,     0,     0,     0,     0,  6561,     0, 19683,     0,     0,     0,   243,     0,     0,     0,  2187,
		    0,     0,     0,     0,     0,     0,     0,     0,     1,     0,     0,     0,     0,     0,     0,     0,
		    0,     0,     0,     0,     0,     0,     0,     0,     0,    81,     0,     0,     1,     0,     0,     0
	}}, {{ // a5
		    0,     0,     0,     0, 19683,     0,  6561,     0,     0,     0,    81,     0,     0,     0,     9,     0,
		    0,     0,     0,     0,     0,     0,     0,     0,     0,  2187,     0,     0,     0,     0,     0,     0,
		    0,     0,     0,     0,     0,     0,     0,     0,     1,     0,     0,    27,     0,     0,     0,     0
	}}, {{ // b5
		    0,     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,    81,     0,
		    0,     0,    27,     0,     0,     0,     0,     0,     0,   729,     0,     0,     0,     0,     0,     0,
		    0,     0,     0,     0,     3,     0,     0,    27,     0,     0,     0,     0,     0,     0,     0,     0
	}}, {{ // c5
		    0,     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
		    0,     0,     0,     0,     0,     0,    27,     0,     0,   243,     0,     0,     0,     0,     0,     0,
		    0,     9,     0,    27,     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,     0
	}}, {{ // d5
		    0,     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
		    0,     0,     0,     0,     0,     0,     0,     0,     0,    81,    27,     0,     0,    81,     0,     0,
		   27,     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,     0
	}}, {{ // e5
		    0,     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
		    0,     0,     0,     0,     0,     0,     0,     0,     0,    27,     0,    27,    27,     0,     0,    27,
		    0,     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,     0
	}}, {{ // f5
		    0,     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
		    0,     0,     0,     0,     0,     0,     0,    27,     0,     9,     0,     0,     0,     0,     9,     0,
		    0,     0,     0,     0,     0,    27,     0,     0,     0,     0,     0,     0,     0,     0,     0,     0
	}}, {{ // g5
		    0,     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,    81,
		    0,     0,     0,    27,     0,     0,     0,     0,     0,     3,     0,     0,     0,     0,     0,     0,
		    0,     0,     3,     0,     0,     0,     0,     0,     0,    27,     0,     0,     0,     0,     0,     0
	}}, {{ // h5
		    0,     0,     0,     0,     0, 19683,     0,  6561,     0,     0,     0,    81,     0,     0,     0,     9,
		    0,     0,     0,     0,     0,     0,     0,     0,     0,     1,     0,     0,     0,     0,     0,     0,
		    0,     0,     0,     0,     0,     0,     1,     0,     0,     0,     0,     0,     0,    27,     0,     0
	}}, {{ // a6
		    0,     0,    81,     0,     0,     0,  2187,     0,     0,     0,    27,     0,     0,     0,     3,     0,
		    0,     0,     0,     0,     0,  2187,     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
		    0,     0,     0,     0,     1,     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,     0
	}}, {{ // b6
		    0,     0,     9,     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,    27,     0,
		    0,     0,     9,     0,     0,   729,     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
		    0,     3,     0,     0,     0,     0,     0,     0,     0,     0,     0,     9,     0,     0,     0,     0
	}}, {{ // c6
		    0,     0,     1,     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
		    0,     0,     0,     0,     0,   243,     9,     0,     0,     0,     0,     0,     0,   243,     0,     0,
		    0,     0,     0,     0,     0,     0,     0,     9,     0,     0,     0,     0,     0,     0,     0,     0
	}}, {{ // d6
		    0,     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
		    0,     0,     0,     0,     0,    81,     0,     0,     0,     0,     9,     0,     0,     0,     0,     9,
		    0,     0,     0,     9,     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,     0
	}}, {{ // e6
		    0,     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
		    0,     0,     0,     0,     0,    27,     0,     0,     0,     0,     0,     9,     0,     0,     0,     0,
		    9,     0,     0,     0,     0,     9,     0,     0,     0,     0,     0,     0,     0,     0,     0,     0
	}}, {{ // f6
		    0,     0,     0,     1,     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
		    0,     0,     0,     0,     0,     9,     0,     9,     0,     0,     0,     0,     9,     0,     0,     0,
		    0,     0,     0,     0,     0,     0,     0,     0,     0,     9,     0,     0,     0,     0,     0,     0
	}}, {{ // g6
		    0,     0,     0,     9,     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,    27,
		    0,     0,     0,     9,     0,     3,     0,     0,     0,     0,     0,     0,     0,     0,     3,     0,
		    0,     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,     9,     0,     0
	}}, {{ // h6
		    0,     0,     0,    81,     0,     0,     0,  2187,     0,     0,     0,    27,     0,     0,     0,     3,
		    0,     0,     0,     0,     0,     1,     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
		    0,     0,     1,     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,     0
	}}, {{ // a7
		    0,     0,  2187,     0,     0,     0,   729,     0,     0,     0,     9,     0,     0,     0,     0,     0,
		    0,  2187,     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
		    0,     1,     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,     0
	}}, {{ // b7
		    0,     0,   243,     0,     0,     0,    81,     0,     0, 19683,     1,     0,     0,     0,     0,     0,
		    0,   729,     3,     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,   729,     0,     0,
		    0,     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,     0
	}}, {{ // c7
		    0,     0,     3,     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,   729,     0,     0,
		    0,   243,     0,     0,     0,     0,     3,     0,     0,     0,     0,     0,     0,     0,     0,     3,
		    0,     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,     3,     0,     0,     0,     0
	}}, {{ // d7
		    0,     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,   243,     0,     0,
		    0,    81,     0,     0,     0,     0,     0,     0,     0,     0,     3,     0,     0,     0,     0,     0,
		    0,     0,     0,     0,     0,     3,     0,     3,     0,     0,     0,     0,     0,     0,     0,     0
	}}, {{ // e7
		    0,     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,    81,     0,     0,
		    0,    27,     0,     0,     0,     0,     0,     0,     0,     0,     0,     3,     0,     0,     0,     0,
		    0,     0,     0,     3,     0,     0,     0,     0,     0,     3,     0,     0,     0,     0,     0,     0
	}}, {{ // f7
		    0,     0,     0,     3,     0,     0,     0,     0,     0,     0,     0,     0,     0,    27,     0,     0,
		    0,     9,     0,     0,     0,     0,     0,     3,     0,     0,     0,     0,     0,     0,     0,     0,
		    3,     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,     3,     0,     0
	}}, {{ // g7
		    0,     0,     0,   243,     0,     0,     0,    81,     0,     1,     0,     1,     0,     0,     0,     0,
		    0,     3,     0,     3,     0,     0,     0,     0,     0,     0,     0,     0,     3,     0,     0,     0,
		    0,     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,     0
	}}, {{ // h7
		    0,     0,     0,  2187,     0,     0,     0,   729,     0,     0,     0,     9,     0,     0,     0,     0,
		    0,     1,     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,     1,     0,
		    0,     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,     0
	}}, {{ // a8
		    0,     0,  6561,     0,     0,     0,   243,     0,     0,  6561,     3,     0,     0, 19683,     1,     0,
		    0,     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,  2187,     0,     0,
		    0,     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,     0
	}}, {{ // b8
		    0,     0,   729,     0,     0,     0,    27,     0,     0,  2187,     0,     0,     0,     0,     0,     0,
		    0,     0,     1,     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,     1,
		    0,     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,     0
	}}, {{ // c8
		    0,     0,    27,     0,     0,     0,     9,     0,     0,   729,     0,     0,     0,  6561,     0,     0,
		    0,     0,     0,     0,     0,     0,     1,     0,     0,     0,     0,     0,     0,     0,     0,     0,
		    0,     0,     0,     0,     0,     1,     0,     0,     0,     0,     0,     0,     0,     0,     0,     0
	}}, {{ // d8
		    0,     0,     0,     0,     0,     0,     3,     1,     0,   243,     0,     0,     0,  2187,     0,     0,
		    0,     0,     0,     0,     0,     0,     0,     0,     0,     0,     1,     0,     0,     0,     0,     0,
		    0,     0,     0,     0,     0,     0,     0,     0,     0,     1,     0,     1,     0,     0,     0,     0
	}}, {{ // e8
		    0,     0,     0,     0,     0,     0,     1,     3,     0,    81,     0,     0,     0,     9,     0,     0,
		    0,     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,     1,     0,     0,     0,     0,
		    0,     0,     0,     0,     0,     0,     0,     1,     0,     0,     0,     0,     0,     1,     0,     0
	}}, {{ // f8
		    0,     0,     0,    27,     0,     0,     0,     9,     0,    27,     0,     0,     0,     3,     0,     0,
		    0,     0,     0,     0,     0,     0,     0,     1,     0,     0,     0,     0,     0,     0,     0,     0,
		    0,     0,     0,     1,     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,     0
	}}, {{ // g8
		    0,     0,     0,   729,     0,     0,     0,    27,     0,     9,     0,     0,     0,     0,     0,     0,
		    0,     0,     0,     1,     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
		    1,     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,     0
	}}, {{ // h8
		    0,     0,     0,  6561,     0,     0,     0,   243,     0,     3,     0,     3,     0,     1,     0,     1,
		    0,     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,     1,     0,     0,     0,
		    0,     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,     0
	}}, {{ // PASS
		    0,     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
		    0,     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
		    0,     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,     0
	}}
};

#if defined(hasSSE2) || defined(USE_MSVC_X86)

static void eval_update_sse_0(Eval *eval_out, const Eval *eval_in, const Move *move)
{
	int	x = move->x;
	unsigned long long f = move->flipped;
#ifdef __AVX2__
	__m256i	f0 = _mm256_sub_epi16(eval_in->feature.v16[0], _mm256_slli_epi16(EVAL_FEATURE[x].v16[0], 1));
	__m256i	f1 = _mm256_sub_epi16(eval_in->feature.v16[1], _mm256_slli_epi16(EVAL_FEATURE[x].v16[1], 1));
	__m256i	f2 = _mm256_sub_epi16(eval_in->feature.v16[2], _mm256_slli_epi16(EVAL_FEATURE[x].v16[2], 1));

	foreach_bit(x, f) {
		f0 = _mm256_sub_epi16(f0, EVAL_FEATURE[x].v16[0]);
		f1 = _mm256_sub_epi16(f1, EVAL_FEATURE[x].v16[1]);
		f2 = _mm256_sub_epi16(f2, EVAL_FEATURE[x].v16[2]);
	}
	eval_out->feature.v16[0] = f0;
	eval_out->feature.v16[1] = f1;
	eval_out->feature.v16[2] = f2;

#else
	__m128i	f0 = _mm_sub_epi16(eval_in->feature.v8[0], _mm_slli_epi16(EVAL_FEATURE[x].v8[0], 1));
	__m128i	f1 = _mm_sub_epi16(eval_in->feature.v8[1], _mm_slli_epi16(EVAL_FEATURE[x].v8[1], 1));
	__m128i	f2 = _mm_sub_epi16(eval_in->feature.v8[2], _mm_slli_epi16(EVAL_FEATURE[x].v8[2], 1));
	__m128i	f3 = _mm_sub_epi16(eval_in->feature.v8[3], _mm_slli_epi16(EVAL_FEATURE[x].v8[3], 1));
	__m128i	f4 = _mm_sub_epi16(eval_in->feature.v8[4], _mm_slli_epi16(EVAL_FEATURE[x].v8[4], 1));
	__m128i	f5 = _mm_sub_epi16(eval_in->feature.v8[5], _mm_slli_epi16(EVAL_FEATURE[x].v8[5], 1));

#ifdef HAS_CPU_64
	foreach_bit(x, f) {
		f0 = _mm_sub_epi16(f0, EVAL_FEATURE[x].v8[0]);
		f1 = _mm_sub_epi16(f1, EVAL_FEATURE[x].v8[1]);
		f2 = _mm_sub_epi16(f2, EVAL_FEATURE[x].v8[2]);
		f3 = _mm_sub_epi16(f3, EVAL_FEATURE[x].v8[3]);
		f4 = _mm_sub_epi16(f4, EVAL_FEATURE[x].v8[4]);
		f5 = _mm_sub_epi16(f5, EVAL_FEATURE[x].v8[5]);
	}

#else
	unsigned int	fl = (unsigned int) f;
	unsigned int	fh = (unsigned int) (f >> 32);

	foreach_bit_32(x, fl) {
		f0 = _mm_sub_epi16(f0, EVAL_FEATURE[x].v8[0]);
		f1 = _mm_sub_epi16(f1, EVAL_FEATURE[x].v8[1]);
		f2 = _mm_sub_epi16(f2, EVAL_FEATURE[x].v8[2]);
		f3 = _mm_sub_epi16(f3, EVAL_FEATURE[x].v8[3]);
		f4 = _mm_sub_epi16(f4, EVAL_FEATURE[x].v8[4]);
		f5 = _mm_sub_epi16(f5, EVAL_FEATURE[x].v8[5]);
	}
	foreach_bit_32(x, fh) {
		f0 = _mm_sub_epi16(f0, EVAL_FEATURE[x + 32].v8[0]);
		f1 = _mm_sub_epi16(f1, EVAL_FEATURE[x + 32].v8[1]);
		f2 = _mm_sub_epi16(f2, EVAL_FEATURE[x + 32].v8[2]);
		f3 = _mm_sub_epi16(f3, EVAL_FEATURE[x + 32].v8[3]);
		f4 = _mm_sub_epi16(f4, EVAL_FEATURE[x + 32].v8[4]);
		f5 = _mm_sub_epi16(f5, EVAL_FEATURE[x + 32].v8[5]);
	}
#endif
	eval_out->feature.v8[0] = f0;
	eval_out->feature.v8[1] = f1;
	eval_out->feature.v8[2] = f2;
	eval_out->feature.v8[3] = f3;
	eval_out->feature.v8[4] = f4;
	eval_out->feature.v8[5] = f5;
#endif
}

/**
 * @brief Update the features after a player's move.
 *
 * @param eval  Evaluation function.
 * @param move  Move.
 */
static void eval_update_sse_1(Eval *eval_out, const Eval *eval_in, const Move *move)
{
	int	x = move->x;
	unsigned long long f = move->flipped;
#ifdef __AVX2__
	__m256i	f0 = _mm256_sub_epi16(eval_in->feature.v16[0], EVAL_FEATURE[x].v16[0]);
	__m256i	f1 = _mm256_sub_epi16(eval_in->feature.v16[1], EVAL_FEATURE[x].v16[1]);
	__m256i	f2 = _mm256_sub_epi16(eval_in->feature.v16[2], EVAL_FEATURE[x].v16[2]);

	foreach_bit(x, f) {
		f0 = _mm256_add_epi16(f0, EVAL_FEATURE[x].v16[0]);
		f1 = _mm256_add_epi16(f1, EVAL_FEATURE[x].v16[1]);
		f2 = _mm256_add_epi16(f2, EVAL_FEATURE[x].v16[2]);
	}
	eval_out->feature.v16[0] = f0;
	eval_out->feature.v16[1] = f1;
	eval_out->feature.v16[2] = f2;

#else
	__m128i	f0 = _mm_sub_epi16(eval_in->feature.v8[0], EVAL_FEATURE[x].v8[0]);
	__m128i	f1 = _mm_sub_epi16(eval_in->feature.v8[1], EVAL_FEATURE[x].v8[1]);
	__m128i	f2 = _mm_sub_epi16(eval_in->feature.v8[2], EVAL_FEATURE[x].v8[2]);
	__m128i	f3 = _mm_sub_epi16(eval_in->feature.v8[3], EVAL_FEATURE[x].v8[3]);
	__m128i	f4 = _mm_sub_epi16(eval_in->feature.v8[4], EVAL_FEATURE[x].v8[4]);
	__m128i	f5 = _mm_sub_epi16(eval_in->feature.v8[5], EVAL_FEATURE[x].v8[5]);

#ifdef HAS_CPU_64
	foreach_bit(x, f) {
		f0 = _mm_add_epi16(f0, EVAL_FEATURE[x].v8[0]);
		f1 = _mm_add_epi16(f1, EVAL_FEATURE[x].v8[1]);
		f2 = _mm_add_epi16(f2, EVAL_FEATURE[x].v8[2]);
		f3 = _mm_add_epi16(f3, EVAL_FEATURE[x].v8[3]);
		f4 = _mm_add_epi16(f4, EVAL_FEATURE[x].v8[4]);
		f5 = _mm_add_epi16(f5, EVAL_FEATURE[x].v8[5]);
	}

#else
	unsigned int	fl = (unsigned int) f;
	unsigned int	fh = (unsigned int) (f >> 32);

	foreach_bit_32(x, fl) {
		f0 = _mm_add_epi16(f0, EVAL_FEATURE[x].v8[0]);
		f1 = _mm_add_epi16(f1, EVAL_FEATURE[x].v8[1]);
		f2 = _mm_add_epi16(f2, EVAL_FEATURE[x].v8[2]);
		f3 = _mm_add_epi16(f3, EVAL_FEATURE[x].v8[3]);
		f4 = _mm_add_epi16(f4, EVAL_FEATURE[x].v8[4]);
		f5 = _mm_add_epi16(f5, EVAL_FEATURE[x].v8[5]);
	}
	foreach_bit_32(x, fh) {
		f0 = _mm_add_epi16(f0, EVAL_FEATURE[x + 32].v8[0]);
		f1 = _mm_add_epi16(f1, EVAL_FEATURE[x + 32].v8[1]);
		f2 = _mm_add_epi16(f2, EVAL_FEATURE[x + 32].v8[2]);
		f3 = _mm_add_epi16(f3, EVAL_FEATURE[x + 32].v8[3]);
		f4 = _mm_add_epi16(f4, EVAL_FEATURE[x + 32].v8[4]);
		f5 = _mm_add_epi16(f5, EVAL_FEATURE[x + 32].v8[5]);
	}

#endif
	eval_out->feature.v8[0] = f0;
	eval_out->feature.v8[1] = f1;
	eval_out->feature.v8[2] = f2;
	eval_out->feature.v8[3] = f3;
	eval_out->feature.v8[4] = f4;
	eval_out->feature.v8[5] = f5;
#endif
}

#else	// SSE dispatch (Eval may not be aligned)

static void eval_update_sse_0(Eval *eval_out, const Eval *eval_in, const Move *move)
{
	int	x = move->x;
	unsigned int	fl = (unsigned int) move->flipped;
	unsigned int	fh = (unsigned int) (move->flipped >> 32);

	__asm__ (
		"movdqa	%2, %%xmm0\n\t"		"movdqa	%3, %%xmm1\n\t"
		"movdqu	%0, %%xmm2\n\t"		"movdqu	%1, %%xmm3\n\t"
		"psllw	$1, %%xmm0\n\t"		"psllw	$1, %%xmm1\n\t"
		"psubw	%%xmm0, %%xmm2\n\t"	"psubw	%%xmm1, %%xmm3\n"
	: :  "m" (eval_in->feature.us[0]), "m" (eval_in->feature.us[8]), "m" (EVAL_FEATURE[x].us[0]),  "m" (EVAL_FEATURE[x].us[8]));
	__asm__ (
		"movdqa	%2, %%xmm0\n\t"		"movdqa	%3, %%xmm1\n\t"
		"movdqu	%0, %%xmm4\n\t"		"movdqu	%1, %%xmm5\n\t"
		"psllw	$1, %%xmm0\n\t"		"psllw	$1, %%xmm1\n\t"
		"psubw	%%xmm0, %%xmm4\n\t"	"psubw	%%xmm1, %%xmm5\n"
	: :  "m" (eval_in->feature.us[16]), "m" (eval_in->feature.us[24]), "m" (EVAL_FEATURE[x].us[16]),  "m" (EVAL_FEATURE[x].us[24]));
	__asm__ (
		"movdqa	%2, %%xmm0\n\t"		"movdqa	%3, %%xmm1\n\t"
		"movdqu	%0, %%xmm6\n\t"		"movdqu	%1, %%xmm7\n\t"
		"psllw	$1, %%xmm0\n\t"		"psllw	$1, %%xmm1\n\t"
		"psubw	%%xmm0, %%xmm6\n\t"	"psubw	%%xmm1, %%xmm7\n"
	: :  "m" (eval_in->feature.us[32]), "m" (eval_in->feature.us[40]), "m" (EVAL_FEATURE[x].us[32]),  "m" (EVAL_FEATURE[x].us[40]));

	foreach_bit_32(x, fl) {
		__asm__ (
			"psubw	%0, %%xmm2\n\t"
			"psubw	%1, %%xmm3\n\t"
			"psubw	%2, %%xmm4\n\t"
			"psubw	%3, %%xmm5\n\t"
			"psubw	%4, %%xmm6\n\t"
			"psubw	%5, %%xmm7\n"
		: :  "m" (EVAL_FEATURE[x].us[0]), "m" (EVAL_FEATURE[x].us[8]), "m" (EVAL_FEATURE[x].us[16]),
		"m" (EVAL_FEATURE[x].us[24]), "m" (EVAL_FEATURE[x].us[32]), "m" (EVAL_FEATURE[x].us[40]));
	}
	foreach_bit_32(x, fh) {
		__asm__ (
			"psubw	%0, %%xmm2\n\t"
			"psubw	%1, %%xmm3\n\t"
			"psubw	%2, %%xmm4\n\t"
			"psubw	%3, %%xmm5\n\t"
			"psubw	%4, %%xmm6\n\t"
			"psubw	%5, %%xmm7\n"
		: :  "m" (EVAL_FEATURE[x + 32].us[0]), "m" (EVAL_FEATURE[x + 32].us[8]), "m" (EVAL_FEATURE[x + 32].us[16]),
		"m" (EVAL_FEATURE[x + 32].us[24]), "m" (EVAL_FEATURE[x + 32].us[32]), "m" (EVAL_FEATURE[x + 32].us[40]));
	}

	__asm__ (
		"movdqu	%%xmm2, %0\n\t"
		"movdqu	%%xmm3, %1\n\t"
		"movdqu	%%xmm4, %2\n\t"
		"movdqu	%%xmm5, %3\n\t"
		"movdqu	%%xmm6, %4\n\t"
		"movdqu	%%xmm7, %5\n\t"
	: :  "m" (eval_out->feature.us[0]), "m" (eval_out->feature.us[8]), "m" (eval_out->feature.us[16]),
	"m" (eval_out->feature.us[24]), "m" (eval_out->feature.us[32]), "m" (eval_out->feature.us[40]));
}

static void eval_update_sse_1(Eval *eval_out, const Eval *eval_in, const Move *move)
{
	int	x = move->x;
	unsigned int	fl = (unsigned int) move->flipped;
	unsigned int	fh = (unsigned int) (move->flipped >> 32);

	__asm__ (
		"movdqu	%0, %%xmm2\n\t"		"movdqu	%1, %%xmm3\n\t"
		"psubw	%2, %%xmm2\n\t"		"psubw	%3, %%xmm3\n"
	: :  "m" (eval_in->feature.us[0]), "m" (eval_in->feature.us[8]), "m" (EVAL_FEATURE[x].us[0]),  "m" (EVAL_FEATURE[x].us[8]));
	__asm__ (
		"movdqu	%0, %%xmm4\n\t"		"movdqu	%1, %%xmm5\n\t"
		"psubw	%2, %%xmm4\n\t"		"psubw	%3, %%xmm5\n"
	: :  "m" (eval_in->feature.us[16]), "m" (eval_in->feature.us[24]), "m" (EVAL_FEATURE[x].us[16]),  "m" (EVAL_FEATURE[x].us[24]));
	__asm__ (
		"movdqu	%0, %%xmm6\n\t"		"movdqu	%1, %%xmm7\n\t"
		"psubw	%2, %%xmm6\n\t"		"psubw	%3, %%xmm7\n"
	: :  "m" (eval_in->feature.us[32]), "m" (eval_in->feature.us[40]), "m" (EVAL_FEATURE[x].us[32]),  "m" (EVAL_FEATURE[x].us[40]));

	foreach_bit_32(x, fl) {
		__asm__ (
			"paddw	%0, %%xmm2\n\t"
			"paddw	%1, %%xmm3\n\t"
			"paddw	%2, %%xmm4\n\t"
			"paddw	%3, %%xmm5\n\t"
			"paddw	%4, %%xmm6\n\t"
			"paddw	%5, %%xmm7\n"
		: :  "m" (EVAL_FEATURE[x].us[0]), "m" (EVAL_FEATURE[x].us[8]), "m" (EVAL_FEATURE[x].us[16]),
		"m" (EVAL_FEATURE[x].us[24]), "m" (EVAL_FEATURE[x].us[32]), "m" (EVAL_FEATURE[x].us[40]));
	}
	foreach_bit_32(x, fh) {
		__asm__ (
			"paddw	%0, %%xmm2\n\t"
			"paddw	%1, %%xmm3\n\t"
			"paddw	%2, %%xmm4\n\t"
			"paddw	%3, %%xmm5\n\t"
			"paddw	%4, %%xmm6\n\t"
			"paddw	%5, %%xmm7\n"
		: :  "m" (EVAL_FEATURE[x + 32].us[0]), "m" (EVAL_FEATURE[x + 32].us[8]), "m" (EVAL_FEATURE[x + 32].us[16]),
		"m" (EVAL_FEATURE[x + 32].us[24]), "m" (EVAL_FEATURE[x + 32].us[32]), "m" (EVAL_FEATURE[x + 32].us[40]));
	}

	__asm__ (
		"movdqu	%%xmm2, %0\n\t"
		"movdqu	%%xmm3, %1\n\t"
		"movdqu	%%xmm4, %2\n\t"
		"movdqu	%%xmm5, %3\n\t"
		"movdqu	%%xmm6, %4\n\t"
		"movdqu	%%xmm7, %5\n\t"
	: :  "m" (eval_out->feature.us[0]), "m" (eval_out->feature.us[8]), "m" (eval_out->feature.us[16]),
	"m" (eval_out->feature.us[24]), "m" (eval_out->feature.us[32]), "m" (eval_out->feature.us[40]));
}

<<<<<<< HEAD
static void eval_restore_sse_0(Eval *eval, const Move *move)
{
	int	x = move->x;
	unsigned int	fl = (unsigned int) move->flipped;
	unsigned int	fh = (unsigned int) (move->flipped >> 32);

	__asm__ (
		"movdqa	%2, %%xmm0\n\t"		"movdqa	%3, %%xmm1\n\t"
		"movdqu	%0, %%xmm2\n\t"		"movdqu	%1, %%xmm3\n\t"
		"psllw	$1, %%xmm0\n\t"		"psllw	$1, %%xmm1\n\t"
		"paddw	%%xmm0, %%xmm2\n\t"	"paddw	%%xmm1, %%xmm3\n"
	: :  "m" (eval->feature.us[0]), "m" (eval->feature.us[8]), "m" (EVAL_FEATURE[x].us[0]),  "m" (EVAL_FEATURE[x].us[8]));
	__asm__ (
		"movdqa	%2, %%xmm0\n\t"		"movdqa	%3, %%xmm1\n\t"
		"movdqu	%0, %%xmm4\n\t"		"movdqu	%1, %%xmm5\n\t"
		"psllw	$1, %%xmm0\n\t"		"psllw	$1, %%xmm1\n\t"
		"paddw	%%xmm0, %%xmm4\n\t"	"paddw	%%xmm1, %%xmm5\n"
	: :  "m" (eval->feature.us[16]), "m" (eval->feature.us[24]), "m" (EVAL_FEATURE[x].us[16]),  "m" (EVAL_FEATURE[x].us[24]));
	__asm__ (
		"movdqa	%2, %%xmm0\n\t"		"movdqa	%3, %%xmm1\n\t"
		"movdqu	%0, %%xmm6\n\t"		"movdqu	%1, %%xmm7\n\t"
		"psllw	$1, %%xmm0\n\t"		"psllw	$1, %%xmm1\n\t"
		"paddw	%%xmm0, %%xmm6\n\t"	"paddw	%%xmm1, %%xmm7\n"
	: :  "m" (eval->feature.us[32]), "m" (eval->feature.us[40]), "m" (EVAL_FEATURE[x].us[32]),  "m" (EVAL_FEATURE[x].us[40]));

	foreach_bit_32(x, fl) {
		__asm__ (
			"paddw	%0, %%xmm2\n\t"
			"paddw	%1, %%xmm3\n\t"
			"paddw	%2, %%xmm4\n\t"
			"paddw	%3, %%xmm5\n\t"
			"paddw	%4, %%xmm6\n\t"
			"paddw	%5, %%xmm7\n"
		: :  "m" (EVAL_FEATURE[x].us[0]), "m" (EVAL_FEATURE[x].us[8]), "m" (EVAL_FEATURE[x].us[16]),
		"m" (EVAL_FEATURE[x].us[24]), "m" (EVAL_FEATURE[x].us[32]), "m" (EVAL_FEATURE[x].us[40]));
	}
	foreach_bit_32(x, fh) {
		__asm__ (
			"paddw	%0, %%xmm2\n\t"
			"paddw	%1, %%xmm3\n\t"
			"paddw	%2, %%xmm4\n\t"
			"paddw	%3, %%xmm5\n\t"
			"paddw	%4, %%xmm6\n\t"
			"paddw	%5, %%xmm7\n"
		: :  "m" (EVAL_FEATURE[x + 32].us[0]), "m" (EVAL_FEATURE[x + 32].us[8]), "m" (EVAL_FEATURE[x + 32].us[16]),
		"m" (EVAL_FEATURE[x + 32].us[24]), "m" (EVAL_FEATURE[x + 32].us[32]), "m" (EVAL_FEATURE[x + 32].us[40]));
	}

	__asm__ (
		"movdqu	%%xmm2, %0\n\t"
		"movdqu	%%xmm3, %1\n\t"
		"movdqu	%%xmm4, %2\n\t"
		"movdqu	%%xmm5, %3\n\t"
		"movdqu	%%xmm6, %4\n\t"
		"movdqu	%%xmm7, %5\n\t"
	: :  "m" (eval->feature.us[0]), "m" (eval->feature.us[8]), "m" (eval->feature.us[16]),
	"m" (eval->feature.us[24]), "m" (eval->feature.us[32]), "m" (eval->feature.us[40]));
}

static void eval_restore_sse_1(Eval *eval, const Move *move)
{
	int	x = move->x;
	unsigned int	fl = (unsigned int) move->flipped;
	unsigned int	fh = (unsigned int) (move->flipped >> 32);

	__asm__ (
		"movdqu	%0, %%xmm2\n\t"		"movdqu	%1, %%xmm3\n\t"
		"paddw	%2, %%xmm2\n\t"		"paddw	%3, %%xmm3\n"
	: :  "m" (eval->feature.us[0]), "m" (eval->feature.us[8]), "m" (EVAL_FEATURE[x].us[0]),  "m" (EVAL_FEATURE[x].us[8]));
	__asm__ (
		"movdqu	%0, %%xmm4\n\t"		"movdqu	%1, %%xmm5\n\t"
		"paddw	%2, %%xmm4\n\t"		"paddw	%3, %%xmm5\n"
	: :  "m" (eval->feature.us[16]), "m" (eval->feature.us[24]), "m" (EVAL_FEATURE[x].us[16]),  "m" (EVAL_FEATURE[x].us[24]));
	__asm__ (
		"movdqu	%0, %%xmm6\n\t"		"movdqu	%1, %%xmm7\n\t"
		"paddw	%2, %%xmm6\n\t"		"paddw	%3, %%xmm7\n"
	: :  "m" (eval->feature.us[32]), "m" (eval->feature.us[40]), "m" (EVAL_FEATURE[x].us[32]),  "m" (EVAL_FEATURE[x].us[40]));

	foreach_bit_32(x, fl) {
		__asm__ (
			"psubw	%0, %%xmm2\n\t"
			"psubw	%1, %%xmm3\n\t"
			"psubw	%2, %%xmm4\n\t"
			"psubw	%3, %%xmm5\n\t"
			"psubw	%4, %%xmm6\n\t"
			"psubw	%5, %%xmm7\n"
		: :  "m" (EVAL_FEATURE[x].us[0]), "m" (EVAL_FEATURE[x].us[8]), "m" (EVAL_FEATURE[x].us[16]),
		"m" (EVAL_FEATURE[x].us[24]), "m" (EVAL_FEATURE[x].us[32]), "m" (EVAL_FEATURE[x].us[40]));
	}
	foreach_bit_32(x, fh) {
		__asm__ (
			"psubw	%0, %%xmm2\n\t"
			"psubw	%1, %%xmm3\n\t"
			"psubw	%2, %%xmm4\n\t"
			"psubw	%3, %%xmm5\n\t"
			"psubw	%4, %%xmm6\n\t"
			"psubw	%5, %%xmm7\n"
		: :  "m" (EVAL_FEATURE[x + 32].us[0]), "m" (EVAL_FEATURE[x + 32].us[8]), "m" (EVAL_FEATURE[x + 32].us[16]),
		"m" (EVAL_FEATURE[x + 32].us[24]), "m" (EVAL_FEATURE[x + 32].us[32]), "m" (EVAL_FEATURE[x + 32].us[40]));
	}

	__asm__ (
		"movdqu	%%xmm2, %0\n\t"
		"movdqu	%%xmm3, %1\n\t"
		"movdqu	%%xmm4, %2\n\t"
		"movdqu	%%xmm5, %3\n\t"
		"movdqu	%%xmm6, %4\n\t"
		"movdqu	%%xmm7, %5\n\t"
	: :  "m" (eval->feature.us[0]), "m" (eval->feature.us[8]), "m" (eval->feature.us[16]),
	"m" (eval->feature.us[24]), "m" (eval->feature.us[32]), "m" (eval->feature.us[40]));
}

<<<<<<< HEAD
#endif // __SSE2__
>>>>>>> 1c68bd5 (SSE / AVX optimized eval feature added)
=======
=======
>>>>>>> f1d221c (Replace eval_restore with simple save-restore, as well as parity)
#endif // hasSSE2
<<<<<<< HEAD
>>>>>>> 1dc032e (Improve visual c compatibility)
=======

#ifdef hasSSE2

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
	static const EVAL_FEATURE_V EVAL_FEATURE_all_opponent = {{
		 9841,  9841,  9841,  9841, 29524, 29524, 29524, 29524, 29524, 29524, 29524, 29524, 29524, 29524, 29524, 29524,
		 3280,  3280,  3280,  3280,  3280,  3280,  3280,  3280,  3280,  3280,  3280,  3280,  3280,  3280,  1093,  1093,
		 1093,  1093,   364,   364,   364,   364,   121,   121,   121,   121,    40,    40,    40,    40,     0,     0
	}};
#ifdef __AVX2__
	__m256i	f0 = EVAL_FEATURE_all_opponent.v16[0];
	__m256i	f1 = EVAL_FEATURE_all_opponent.v16[1];
	__m256i	f2 = EVAL_FEATURE_all_opponent.v16[2];

	foreach_bit(x, b) {
		f0 = _mm256_sub_epi16(f0, EVAL_FEATURE[x].v16[0]);
		f1 = _mm256_sub_epi16(f1, EVAL_FEATURE[x].v16[1]);
		f2 = _mm256_sub_epi16(f2, EVAL_FEATURE[x].v16[2]);
	}
	b = ~(board->opponent | board->player);
	foreach_bit(x, b) {
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

	foreach_bit(x, b) {
		f0 = _mm_sub_epi16(f0, EVAL_FEATURE[x].v8[0]);
		f1 = _mm_sub_epi16(f1, EVAL_FEATURE[x].v8[1]);
		f2 = _mm_sub_epi16(f2, EVAL_FEATURE[x].v8[2]);
		f3 = _mm_sub_epi16(f3, EVAL_FEATURE[x].v8[3]);
		f4 = _mm_sub_epi16(f4, EVAL_FEATURE[x].v8[4]);
		f5 = _mm_sub_epi16(f5, EVAL_FEATURE[x].v8[5]);
	}
	b = ~(board->opponent | board->player);
	foreach_bit(x, b) {
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

/**
 * @brief Update the features after a player's move.
 *
 * @param eval  Evaluation function.
 * @param move  Move.
 */
void eval_update(Eval *eval, const Move *move)
{
	assert(move->flipped);
	if (eval->n_empties & 1)
		eval_update_sse_1(eval, eval, move);
	else
		eval_update_sse_0(eval, eval, move);
}

void eval_update_leaf(Eval *eval_out, const Eval *eval_in, const Move *move)
{
	if (eval_in->n_empties & 1)
		eval_update_sse_1(eval_out, eval_in, move);
	else
		eval_update_sse_0(eval_out, eval_in, move);
}

#endif // hasSSE2
>>>>>>> 1b29848 (fix & optimize 32 bit build; other minor mods)
=======
#endif
=======
  #endif
>>>>>>> 534241b (Revise foreach_bit_r and first_bit_32)
}

/**
 * @brief Update the features after a player's move.
 *
 * @param x     Move position.
 * @param f     Flipped bitboard.
 * @param eval  Evaluation function.
 */
void eval_update(int x, unsigned long long f, Eval *eval)
{
	assert(f);
	if (eval->n_empties & 1)
		eval_update_sse_1(x, f, eval, eval);
	else
		eval_update_sse_0(x, f, eval, eval);
}

void eval_update_leaf(int x, unsigned long long f, Eval *eval_out, const Eval *eval_in)
{
	if (eval_in->n_empties & 1)
		eval_update_sse_1(x, f, eval_out, eval_in);
	else
		eval_update_sse_0(x, f, eval_out, eval_in);
}

=======
>>>>>>> 6820748 (Unify eval_update_sse 0 & 1)
#endif // hasSSE2
>>>>>>> 3e1ed4f (fix cr/lf in repository to lf)
