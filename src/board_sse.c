<<<<<<< HEAD
<<<<<<< HEAD
=======
>>>>>>> 3e1ed4f (fix cr/lf in repository to lf)
/**
 * @file board_sse.c
 *
 * SSE/AVX translation of some board.c functions
 *
<<<<<<< HEAD
<<<<<<< HEAD
 * @date 2014 - 2024
 * @author Toshihiko Okuhara
 * @version 4.5
=======
 * @date 2014 - 2020
 * @author Toshihiko Okuhara
 * @version 4.4
>>>>>>> 3e1ed4f (fix cr/lf in repository to lf)
=======
 * @date 2014 - 2022
 * @author Toshihiko Okuhara
 * @version 4.5
>>>>>>> 9e2bbc5 (split get_all_full_lines from get_stability)
 */

#include "bit.h"
#include "hash.h"
#include "board.h"

<<<<<<< HEAD
<<<<<<< HEAD
#if defined(ANDROID) && !defined(HAS_CPU_64) && !defined(hasSSE2)
=======
#if defined(ANDROID) && !defined(hasNeon) && !defined(hasSSE2)
>>>>>>> 343493d (More neon/sse optimizations; neon dispatch added for arm32)
#include "android/cpu-features.h"

bool	hasSSE2 = false;

void init_neon (void)
{
<<<<<<< HEAD
  #ifdef __arm__
=======
#ifdef __arm__
>>>>>>> 343493d (More neon/sse optimizations; neon dispatch added for arm32)
	if (android_getCpuFeatures() & ANDROID_CPU_ARM_FEATURE_NEON) {
	#if (MOVE_GENERATOR == MOVE_GENERATOR_BITSCAN)
		extern unsigned long long (*flip_neon[66])(const unsigned long long, const unsigned long long);
		memcpy(flip, flip_neon, sizeof(flip_neon));
	#endif
<<<<<<< HEAD
		hasSSE2 = true;	// for eval_update_sse
	}
  #elif defined(__i386__)	// android x86 w/o SSE2 - uncommon and not tested
=======
		hasSSE2 = true;
	}
#else	// android x86 w/o SSE2 - uncommon and not tested
>>>>>>> 343493d (More neon/sse optimizations; neon dispatch added for arm32)
	int	cpuid_edx, cpuid_ecx;
	__asm__ (
		"movl	$1, %%eax\n\t"
		"cpuid"
	: "=d" (cpuid_edx), "=c" (cpuid_ecx) :: "%eax", "%ebx" );
	if ((cpuid_edx & 0x04000000u) != 0)
		hasSSE2 = true;
<<<<<<< HEAD
  #endif
}
#endif

=======
>>>>>>> 3e1ed4f (fix cr/lf in repository to lf)
=======
#endif
}
#endif

>>>>>>> 343493d (More neon/sse optimizations; neon dispatch added for arm32)
/**
 * @brief SSE2 translation of board_symetry
 *
 * @param board input board
<<<<<<< HEAD
=======
 * @param s symetry
>>>>>>> 3e1ed4f (fix cr/lf in repository to lf)
 * @param sym symetric output board
 */
#ifdef hasSSE2

<<<<<<< HEAD
static __m128i vectorcall board_horizontal_mirror_sse(__m128i bb)
{
	const __m128i mask0F0F = _mm_set1_epi16(0x0F0F);
  #if defined(__SSSE3__) || defined(__AVX__)	// pshufb (cf. http://wm.ite.pl/articles/sse-popcount.html)
	const __m128i mbitrev  = _mm_set_epi8(15, 7, 11, 3, 13, 5, 9, 1, 14, 6, 10, 2, 12, 4, 8, 0);
	bb = _mm_or_si128(_mm_shuffle_epi8(mbitrev, _mm_and_si128(_mm_srli_epi64(bb, 4), mask0F0F)),
		_mm_slli_epi64(_mm_shuffle_epi8(mbitrev, _mm_and_si128(bb, mask0F0F)), 4));
  #else
	const __m128i mask5555 = _mm_set1_epi16(0x5555);
	const __m128i mask3333 = _mm_set1_epi16(0x3333);
	bb = _mm_or_si128(_mm_and_si128(_mm_srli_epi64(bb, 1), mask5555), _mm_slli_epi64(_mm_and_si128(bb, mask5555), 1));
	bb = _mm_or_si128(_mm_and_si128(_mm_srli_epi64(bb, 2), mask3333), _mm_slli_epi64(_mm_and_si128(bb, mask3333), 2));
	bb = _mm_or_si128(_mm_and_si128(_mm_srli_epi64(bb, 4), mask0F0F), _mm_slli_epi64(_mm_and_si128(bb, mask0F0F), 4));
  #endif
	return bb;
}

void board_horizontal_mirror(const Board *board, Board *sym)
{
	_mm_storeu_si128((__m128i *) sym, board_horizontal_mirror_sse(_mm_loadu_si128((__m128i *) board)));
}

static __m128i vectorcall board_vertical_mirror_sse(__m128i bb)
{
  #if defined(__SSSE3__) || defined(__AVX__)	// pshufb
	return _mm_shuffle_epi8(bb, _mm_set_epi8(8, 9, 10, 11, 12, 13, 14, 15, 0, 1, 2, 3, 4, 5, 6, 7));
  #else
	bb = _mm_or_si128(_mm_srli_epi16(bb, 8), _mm_slli_epi16(bb, 8));
	return _mm_shufflehi_epi16(_mm_shufflelo_epi16(bb, 0x1b), 0x1b);
  #endif
}

void board_vertical_mirror(const Board *board, Board *sym)
{
  #if defined(__SSSE3__) || defined(__AVX__) || !defined(HAS_CPU_64)
	_mm_storeu_si128((__m128i *) sym, board_vertical_mirror_sse(_mm_loadu_si128((__m128i *) board)));
  #else	// use BSWAP64
	sym->player = vertical_mirror(board->player);
	sym->opponent = vertical_mirror(board->opponent);
  #endif
}

static __m128i vectorcall board_transpose_sse(__m128i bb)
{
	const __m128i mask00AA = _mm_set1_epi16(0x00AA);
	const __m128i maskCCCC = _mm_set1_epi32(0x0000CCCC);
	const __m128i mask00F0 = _mm_set1_epi64x(0x00000000F0F0F0F0);
	__m128i tt = _mm_and_si128(_mm_xor_si128(bb, _mm_srli_epi64(bb, 7)), mask00AA);
	bb = _mm_xor_si128(_mm_xor_si128(bb, tt), _mm_slli_epi64(tt, 7));
	tt = _mm_and_si128(_mm_xor_si128(bb, _mm_srli_epi64(bb, 14)), maskCCCC);
	bb = _mm_xor_si128(_mm_xor_si128(bb, tt), _mm_slli_epi64(tt, 14));
	tt = _mm_and_si128(_mm_xor_si128(bb, _mm_srli_epi64(bb, 28)), mask00F0);
	bb = _mm_xor_si128(_mm_xor_si128(bb, tt), _mm_slli_epi64(tt, 28));
	return bb;
}

void board_transpose(const Board *board, Board *sym)
{
	_mm_storeu_si128((__m128i *) sym, board_transpose_sse(_mm_loadu_si128((__m128i *) board)));
}

void board_symetry(const Board *board, const int s, Board *sym)
{
	__m128i	bb = _mm_loadu_si128((__m128i *) board);
	if (s & 1)
		bb = board_horizontal_mirror_sse(bb);
	if (s & 2)
		bb = board_vertical_mirror_sse(bb);
	if (s & 4)
		bb = board_transpose_sse(bb);

	_mm_storeu_si128((__m128i *) sym, bb);
	board_check(sym);
}

#elif defined(__ARM_NEON) && !defined(DISPATCH_NEON)

static uint64x2_t board_horizontal_mirror_neon(uint64x2_t bb)
{
  #ifdef HAS_CPU_64
	bb = vreinterpretq_u64_u8(vrbitq_u8(vreinterpretq_u8_u64(bb)));
  #else
	bb = vbslq_u64(vdupq_n_u64(0x5555555555555555), vshrq_n_u64(bb, 1), vshlq_n_u64(bb, 1));
	bb = vbslq_u64(vdupq_n_u64(0x3333333333333333), vshrq_n_u64(bb, 2), vshlq_n_u64(bb, 2));
	bb = vreinterpretq_u64_u8(vsliq_n_u8(vshrq_n_u8(vreinterpretq_u8_u64(bb), 4), vreinterpretq_u8_u64(bb), 4));
  #endif
	return bb;
}

void board_horizontal_mirror(const Board *board, Board *sym)
{
	vst1q_u64((uint64_t *) sym, board_horizontal_mirror_neon(vld1q_u64((uint64_t *) board)));
}

static uint64x2_t board_vertical_mirror_neon(uint64x2_t bb)
{
	return vreinterpretq_u64_u8(vrev64q_u8(vreinterpretq_u8_u64(bb)));
}

void board_vertical_mirror(const Board *board, Board *sym)
{
	vst1q_u64((uint64_t *) sym, board_vertical_mirror_neon(vld1q_u64((uint64_t *) board)));
}

static uint64x2_t board_transpose_neon(uint64x2_t bb)
{
	uint64x2_t tt = vandq_u64(veorq_u64(bb, vshrq_n_u64(bb, 7)), vdupq_n_u64(0x00AA00AA00AA00AA));
	bb = veorq_u64(veorq_u64(bb, tt), vshlq_n_u64(tt, 7));
	tt = vandq_u64(veorq_u64(bb, vshrq_n_u64(bb, 14)), vdupq_n_u64(0x0000CCCC0000CCCC));
	bb = veorq_u64(veorq_u64(bb, tt), vshlq_n_u64(tt, 14));
	tt = vandq_u64(veorq_u64(bb, vshrq_n_u64(bb, 28)), vdupq_n_u64(0x00000000F0F0F0F0));
	bb = veorq_u64(veorq_u64(bb, tt), vshlq_n_u64(tt, 28));
	return bb;
}

void board_transpose(const Board *board, Board *sym)
{
	vst1q_u64((uint64_t *) sym, board_transpose_neon(vld1q_u64((uint64_t *) board)));
}

void board_symetry(const Board *board, const int s, Board *sym)
{
	uint64x2_t bb = vld1q_u64((uint64_t *) board);
	if (s & 1)
		bb = board_horizontal_mirror_neon(bb);
	if (s & 2)
		bb = board_vertical_mirror_neon(bb);
	if (s & 4)
		bb = board_transpose_neon(bb);

	vst1q_u64((uint64_t *) sym, bb);
	board_check(sym);
}

#endif // hasSSE2/Neon

#ifdef __AVX2__
/**
 * @brief unique board
 *
 * Compute a board unique from all its possible symertries.
 *
 * @param board input board
 * @param unique output board
 */
static void board_horizontal_mirror_avx(const __m256i *bb, __m256i *sym)
{
	const __m256i mask0F0F = _mm256_set1_epi16(0x0F0F);
	const __m256i mbitrev  = _mm256_set_epi8(	//cf. http://wm.ite.pl/articles/sse-popcount.html
		15, 7, 11, 3, 13, 5, 9, 1, 14, 6, 10, 2, 12, 4, 8, 0,
		15, 7, 11, 3, 13, 5, 9, 1, 14, 6, 10, 2, 12, 4, 8, 0);
	*sym = _mm256_or_si256(_mm256_shuffle_epi8(mbitrev, _mm256_and_si256(_mm256_srli_epi64(*bb, 4), mask0F0F)),
		_mm256_slli_epi64(_mm256_shuffle_epi8(mbitrev, _mm256_and_si256(*bb, mask0F0F)), 4));
}

static void board_vertical_mirror_avx(const __m256i *bb, __m256i *sym)
{
	*sym = _mm256_shuffle_epi8(*bb, _mm256_set_epi8(
		8, 9, 10, 11, 12, 13, 14, 15, 0, 1, 2, 3, 4, 5, 6, 7,
		8, 9, 10, 11, 12, 13, 14, 15, 0, 1, 2, 3, 4, 5, 6, 7));
}

int board_unique(const Board *board, Board *unique)
{
	Board sym[8];
	int i, j, s = 0;
	static const char reorder[8] = { 0, 2, 4, 6, 1, 5, 3, 7 };

	sym[0] = *board;
	board_transpose(board, &sym[1]);	// was sym[4]
	board_horizontal_mirror_avx((__m256i *) &sym[0], (__m256i *) &sym[2]);	// were sym[1] & sym[6]
	board_vertical_mirror_avx((__m256i *) &sym[0], (__m256i *) &sym[4]);	// were sym[2] & sym[5]
	board_vertical_mirror_avx((__m256i *) &sym[2], (__m256i *) &sym[6]);	// were sym[3] & sym[7]

	*unique = *board;
	for (i = 1; i < 8; ++i) {
		j = reorder[i];
		if (board_lesser(&sym[j], unique)) {
			*unique = sym[j];
			s = i;
		}
	}

	board_check(unique);
	return s;
}
#endif

/**
 * @brief Compute a board resulting of a move played on a previous board.
 *
 * @param OP board to play the move on.
=======
void board_symetry(const Board *board, const int s, Board *sym)
{
	__m128i	bb = _mm_loadu_si128((__m128i *) board);
	__m128i	tt;
	const __m128i mask0F0F = _mm_set1_epi16(0x0F0F);
	const __m128i mask00AA = _mm_set1_epi16(0x00AA);
	const __m128i maskCCCC = _mm_set1_epi32(0x0000CCCC);
	const __m128i mask00F0 = _mm_set1_epi64x(0x00000000F0F0F0F0);
#if defined(__SSSE3__) || defined(__AVX__)	// pshufb
	const __m128i mbswapll = _mm_set_epi8(8, 9, 10, 11, 12, 13, 14, 15, 0, 1, 2, 3, 4, 5, 6, 7);
	const __m128i mbitrev  = _mm_set_epi8(15, 7, 11, 3, 13, 5, 9, 1, 14, 6, 10, 2, 12, 4, 8, 0);

	if (s & 1) {	// horizontal_mirror (cf. http://wm.ite.pl/articles/sse-popcount.html)
		bb = _mm_or_si128(_mm_shuffle_epi8(mbitrev, _mm_and_si128(_mm_srli_epi64(bb, 4), mask0F0F)),
			_mm_slli_epi64(_mm_shuffle_epi8(mbitrev, _mm_and_si128(bb, mask0F0F)), 4));
	}

	if (s & 2) {	// vertical_mirror
		bb = _mm_shuffle_epi8(bb, mbswapll);
	}

#else
	const __m128i mask5555 = _mm_set1_epi16(0x5555);
	const __m128i mask3333 = _mm_set1_epi16(0x3333);

	if (s & 1) {	// horizontal_mirror
		bb = _mm_or_si128(_mm_and_si128(_mm_srli_epi64(bb, 1), mask5555), _mm_slli_epi64(_mm_and_si128(bb, mask5555), 1));
		bb = _mm_or_si128(_mm_and_si128(_mm_srli_epi64(bb, 2), mask3333), _mm_slli_epi64(_mm_and_si128(bb, mask3333), 2));
		bb = _mm_or_si128(_mm_and_si128(_mm_srli_epi64(bb, 4), mask0F0F), _mm_slli_epi64(_mm_and_si128(bb, mask0F0F), 4));
	}

	if (s & 2) {	// vertical_mirror
		bb = _mm_or_si128(_mm_srli_epi16(bb, 8), _mm_slli_epi16(bb, 8));
		bb = _mm_shufflehi_epi16(_mm_shufflelo_epi16(bb, 0x1b), 0x1b);
	}
#endif

	if (s & 4) {	// transpose
		tt = _mm_and_si128(_mm_xor_si128(bb, _mm_srli_epi64(bb, 7)), mask00AA);
		bb = _mm_xor_si128(_mm_xor_si128(bb, tt), _mm_slli_epi64(tt, 7));
		tt = _mm_and_si128(_mm_xor_si128(bb, _mm_srli_epi64(bb, 14)), maskCCCC);
		bb = _mm_xor_si128(_mm_xor_si128(bb, tt), _mm_slli_epi64(tt, 14));
		tt = _mm_and_si128(_mm_xor_si128(bb, _mm_srli_epi64(bb, 28)), mask00F0);
		bb = _mm_xor_si128(_mm_xor_si128(bb, tt), _mm_slli_epi64(tt, 28));
	}

#ifdef __clang__
	sym->player = bb[0];
	sym->opponent = bb[1];
#else	// error on clang 3.8
	_mm_storeu_si128((__m128i *) sym, bb);
#endif

	board_check(sym);
}

#elif defined(hasNeon)

void board_symetry(const Board *board, const int s, Board *sym)
{
	uint64x2_t bb = vld1q_u64((uint64_t *) board);
	uint64x2_t tt;

	if (s & 1) {	// horizontal_mirror
#ifdef HAS_CPU_64
		bb = vreinterpretq_u64_u8(vrbitq_u8(vreinterpretq_u8_u64(bb)));
#else
		bb = vbslq_u64(vdupq_n_u64(0x5555555555555555), vshrq_n_u64(bb, 1), vshlq_n_u64(bb, 1));
		bb = vbslq_u64(vdupq_n_u64(0x3333333333333333), vshrq_n_u64(bb, 2), vshlq_n_u64(bb, 2));
		bb = vreinterpretq_u64_u8(vsliq_n_u8(vshrq_n_u8(vreinterpretq_u8_u64(bb), 4), vreinterpretq_u8_u64(bb), 4));
#endif
	}

	if (s & 2) {	// vertical_mirror
		bb = vreinterpretq_u64_u8(vrev64q_u8(vreinterpretq_u8_u64(bb)));
	}

	if (s & 4) {	// transpose
		tt = vandq_u64(veorq_u64(bb, vshrq_n_u64(bb, 7)), vdupq_n_u64(0x00AA00AA00AA00AA));
		bb = veorq_u64(veorq_u64(bb, tt), vshlq_n_u64(tt, 7));
		tt = vandq_u64(veorq_u64(bb, vshrq_n_u64(bb, 14)), vdupq_n_u64(0x0000CCCC0000CCCC));
		bb = veorq_u64(veorq_u64(bb, tt), vshlq_n_u64(tt, 14));
		tt = vandq_u64(veorq_u64(bb, vshrq_n_u64(bb, 28)), vdupq_n_u64(0x00000000F0F0F0F0));
		bb = veorq_u64(veorq_u64(bb, tt), vshlq_n_u64(tt, 28));
	}

	vst1q_u64((uint64_t *) sym, bb);
	board_check(sym);
}

#endif // hasSSE2/Neon

/**
 * @brief Compute a board resulting of a move played on a previous board.
 *
 * @param board board to play the move on.
>>>>>>> 3e1ed4f (fix cr/lf in repository to lf)
 * @param x move to play.
 * @param next resulting board.
 * @return flipped discs.
 */
<<<<<<< HEAD
<<<<<<< HEAD
#if (MOVE_GENERATOR == MOVE_GENERATOR_AVX) || (MOVE_GENERATOR == MOVE_GENERATOR_AVX512) || (MOVE_GENERATOR == MOVE_GENERATOR_SSE)

unsigned long long vectorcall board_next_sse(__m128i OP, const int x, Board *next)
{
	__m128i flipped = reduce_vflip(mm_Flip(OP, x));
=======
=======
#if (MOVE_GENERATOR == MOVE_GENERATOR_AVX) || (MOVE_GENERATOR == MOVE_GENERATOR_SSE)

>>>>>>> 343493d (More neon/sse optimizations; neon dispatch added for arm32)
unsigned long long board_next(const Board *board, const int x, Board *next)
{
	__m128i OP = _mm_loadu_si128((__m128i *) board);
	__m128i flipped = mm_Flip(OP, x);
>>>>>>> 3e1ed4f (fix cr/lf in repository to lf)

	OP = _mm_xor_si128(OP, _mm_or_si128(flipped, _mm_loadl_epi64((__m128i *) &X_TO_BIT[x])));
	_mm_storeu_si128((__m128i *) next, _mm_shuffle_epi32(OP, 0x4e));

	return _mm_cvtsi128_si64(flipped);
}

<<<<<<< HEAD
<<<<<<< HEAD
#elif MOVE_GENERATOR == MOVE_GENERATOR_NEON

unsigned long long board_next_neon(uint64x2_t OP, const int x, Board *next)
{
	uint64x2_t flipped = mm_Flip(OP, x);
  #if !defined(_MSC_VER) && !defined(__clang__)	// MSVC-arm32 does not have vld1q_lane_u64
	// arm64-gcc-13: 21, armv8a-clang-16: 23, msvc-arm64-19: 22, gcc-arm-13: 18, clang-armv7-11: 29 // https://godbolt.org/z/cvhns39rK
	OP = veorq_u64(OP, vorrq_u64(flipped, vld1q_lane_u64((uint64_t *) &X_TO_BIT[x], flipped, 0)));
	vst1q_u64((uint64_t *) next, vextq_u64(OP, OP, 1));
  #else	// arm64-gcc-13: 21, armv8a-clang-16: 22, msvc-arm64-19: 21, gcc-arm-13: 23, clang-armv7-11: 27
	OP = veorq_u64(OP, flipped);
	vst1q_u64((uint64_t *) next, vcombine_u64(vget_high_u64(OP), vorr_u64(vget_low_u64(OP), vld1_u64((uint64_t *) &X_TO_BIT[x]))));
  #endif
	return vgetq_lane_u64(flipped, 0);
}
#endif

=======
=======
#elif MOVE_GENERATOR == MOVE_GENERATOR_NEON

unsigned long long board_next(const Board *board, const int x, Board *next)
{
	uint64x2_t OP = vld1q_u64((uint64_t *) board);
	uint64x2_t flipped = mm_Flip(OP, x);
#ifdef HAS_CPU_64	// vld1q_lane_u64
	OP = veorq_u64(OP, vorrq_u64(flipped, vld1q_lane_u64((uint64_t *) &X_TO_BIT[x], flipped, 0)));
	vst1q_u64((uint64_t *) next, vextq_u64(OP, OP, 1));
#else
	OP = veorq_u64(OP, flipped);
	OP = vcombine_u64(vget_high_u64(OP), vorr_u64(vget_low_u64(OP), vld1_u64(&X_TO_BIT[x])));
	vst1q_u64((uint64_t *) next, OP);
#endif
	return vgetq_lane_u64(flipped, 0);
}
#endif

>>>>>>> 343493d (More neon/sse optimizations; neon dispatch added for arm32)
/**
<<<<<<< HEAD
 * @brief Compute a board resulting of an opponent move played on a previous board.
 *
 * Compute the board after passing and playing a move.
 *
 * @param board board to play the move on.
 * @param x opponent move to play.
 * @param next resulting board.
 * @return flipped discs.
 */
#if (MOVE_GENERATOR == MOVE_GENERATOR_AVX) || (MOVE_GENERATOR == MOVE_GENERATOR_SSE)

unsigned long long board_pass_next(const Board *board, const int x, Board *next)
{
	__m128i	PO = _mm_shuffle_epi32(_mm_loadu_si128((__m128i *) board), 0x4e);
	__m128i flipped = mm_Flip(PO, x);

	PO = _mm_xor_si128(PO, _mm_or_si128(flipped, _mm_loadl_epi64((__m128i *) &X_TO_BIT[x])));
	_mm_storeu_si128((__m128i *) next, _mm_shuffle_epi32(PO, 0x4e));

	return _mm_cvtsi128_si64(flipped);
}

#elif MOVE_GENERATOR == MOVE_GENERATOR_NEON

unsigned long long board_pass_next(const Board *board, const int x, Board *next)
{
	uint64x2_t OP = vld1q_u64((uint64_t *) board);
	uint64x2_t PO = vextq_u64(OP, OP, 1);
	uint64x2_t flipped = mm_Flip(PO, x);

#ifdef HAS_CPU_64	// vld1q_lane_u64
	PO = veorq_u64(PO, vorrq_u64(flipped, vld1q_lane_u64((uint64_t *) &X_TO_BIT[x], flipped, 0)));
	vst1q_u64((uint64_t *) next, vextq_u64(PO, PO, 1));
#else
	PO = veorq_u64(OP, flipped);
	vst1_u64(&next->player, vget_high_u64(PO));
	vst1_u64(&next->opponent, vorr_u64(vget_low_u64(PO), vld1_u64(&X_TO_BIT[x])));
#endif
	return vgetq_lane_u64(flipped, 0);
}

#endif

>>>>>>> 3e1ed4f (fix cr/lf in repository to lf)
/**
=======
>>>>>>> 23e04d1 (Backport endgame_sse optimizations into endgame.c)
 * @brief X64 optimized get_moves
 *
 * Diag-7 is converted to diag-9 (v.v.) using vertical mirroring
 * in SSE versions.
 *
 * @param P bitboard with player's discs.
 * @param O bitboard with opponent's discs.
 * @return all legal moves in a 64-bit unsigned integer.
 */
#ifdef __AVX2__	// 4 AVX

<<<<<<< HEAD
  #if defined(_MSC_VER) || defined(__linux__)	// vectorcall and SYSV-ABI passes __m256i in registers
unsigned long long vectorcall get_moves_avx(__m256i PP, __m256i OO)
{
  #else
unsigned long long get_moves(unsigned long long P, unsigned long long O)	// minGW
{
	__m256i	PP = _mm256_broadcastq_epi64(_mm_cvtsi64_si128(P));
	__m256i OO = _mm256_broadcastq_epi64(_mm_cvtsi64_si128(O));
  #endif
	__m256i	MM, flip_l, flip_r, pre_l, pre_r, shift2;
	__m128i	M;
	const __m256i shift1897 = _mm256_set_epi64x(7, 9, 8, 1);
	__m256i	mOO = _mm256_and_si256(OO, _mm256_set_epi64x(0x007E7E7E7E7E7E00, 0x007E7E7E7E7E7E00, 0x00FFFFFFFFFFFF00, 0x7E7E7E7E7E7E7E7E));
	__m128i occupied = _mm_or_si128(_mm256_castsi256_si128(PP), _mm256_castsi256_si128(OO));
=======
unsigned long long get_moves(const unsigned long long P, const unsigned long long O)
{
	__m256i	PP, mOO, MM, flip_l, flip_r, pre_l, pre_r, shift2;
	__m128i	M;
	const __m256i shift1897 = _mm256_set_epi64x(7, 9, 8, 1);
	const __m256i mflipH = _mm256_set_epi64x(0x7e7e7e7e7e7e7e7e, 0x7e7e7e7e7e7e7e7e, -1, 0x7e7e7e7e7e7e7e7e);

	PP = _mm256_broadcastq_epi64(_mm_cvtsi64_si128(P));
	mOO = _mm256_and_si256(_mm256_broadcastq_epi64(_mm_cvtsi64_si128(O)), mflipH);
>>>>>>> 3e1ed4f (fix cr/lf in repository to lf)

	flip_l = _mm256_and_si256(mOO, _mm256_sllv_epi64(PP, shift1897));
	flip_r = _mm256_and_si256(mOO, _mm256_srlv_epi64(PP, shift1897));
	flip_l = _mm256_or_si256(flip_l, _mm256_and_si256(mOO, _mm256_sllv_epi64(flip_l, shift1897)));
	flip_r = _mm256_or_si256(flip_r, _mm256_and_si256(mOO, _mm256_srlv_epi64(flip_r, shift1897)));
	pre_l = _mm256_and_si256(mOO, _mm256_sllv_epi64(mOO, shift1897));
	pre_r = _mm256_srlv_epi64(pre_l, shift1897);
	shift2 = _mm256_add_epi64(shift1897, shift1897);
	flip_l = _mm256_or_si256(flip_l, _mm256_and_si256(pre_l, _mm256_sllv_epi64(flip_l, shift2)));
	flip_r = _mm256_or_si256(flip_r, _mm256_and_si256(pre_r, _mm256_srlv_epi64(flip_r, shift2)));
	flip_l = _mm256_or_si256(flip_l, _mm256_and_si256(pre_l, _mm256_sllv_epi64(flip_l, shift2)));
	flip_r = _mm256_or_si256(flip_r, _mm256_and_si256(pre_r, _mm256_srlv_epi64(flip_r, shift2)));
<<<<<<< HEAD
	MM = _mm256_or_si256(_mm256_sllv_epi64(flip_l, shift1897), _mm256_srlv_epi64(flip_r, shift1897));

	M = _mm_or_si128(_mm256_castsi256_si128(MM), _mm256_extracti128_si256(MM, 1));
	return _mm_cvtsi128_si64(_mm_andnot_si128(occupied, _mm_or_si128(M, _mm_unpackhi_epi64(M, M))));	// mask with empties
=======
	MM = _mm256_sllv_epi64(flip_l, shift1897);
	MM = _mm256_or_si256(MM, _mm256_srlv_epi64(flip_r, shift1897));

	M = _mm_or_si128(_mm256_castsi256_si128(MM), _mm256_extracti128_si256(MM, 1));
	M = _mm_or_si128(M, _mm_unpackhi_epi64(M, M));
	return _mm_cvtsi128_si64(M) & ~(P|O);	// mask with empties
>>>>>>> 3e1ed4f (fix cr/lf in repository to lf)
}

#elif defined(__x86_64__) || defined(_M_X64)	// 2 SSE, 2 CPU

unsigned long long get_moves(const unsigned long long P, const unsigned long long O)
{
	unsigned long long moves, mO, flip1, pre1, flip8, pre8;
	__m128i	PP, mOO, MM, flip, pre;

	mO = O & 0x7e7e7e7e7e7e7e7eULL;
	PP  = _mm_set_epi64x(vertical_mirror(P), P);
	mOO = _mm_set_epi64x(vertical_mirror(mO), mO);
		/* shift=-9:+7 */								/* shift=+1 */			/* shift = +8 */
	flip = _mm_and_si128(mOO, _mm_slli_epi64(PP, 7));				flip1  = mO & (P << 1);		flip8  = O & (P << 8);
	flip = _mm_or_si128(flip, _mm_and_si128(mOO, _mm_slli_epi64(flip, 7)));		flip1 |= mO & (flip1 << 1);	flip8 |= O & (flip8 << 8);
	pre  = _mm_and_si128(mOO, _mm_slli_epi64(mOO, 7));				pre1   = mO & (mO << 1);	pre8   = O & (O << 8);
	flip = _mm_or_si128(flip, _mm_and_si128(pre, _mm_slli_epi64(flip, 14)));	flip1 |= pre1 & (flip1 << 2);	flip8 |= pre8 & (flip8 << 16);
	flip = _mm_or_si128(flip, _mm_and_si128(pre, _mm_slli_epi64(flip, 14)));	flip1 |= pre1 & (flip1 << 2);	flip8 |= pre8 & (flip8 << 16);
	MM = _mm_slli_epi64(flip, 7);							moves = flip1 << 1;		moves |= flip8 << 8;
		/* shift=-7:+9 */								/* shift=-1 */			/* shift = -8 */
	flip = _mm_and_si128(mOO, _mm_slli_epi64(PP, 9));				flip1  = mO & (P >> 1);		flip8  = O & (P >> 8);
	flip = _mm_or_si128(flip, _mm_and_si128(mOO, _mm_slli_epi64(flip, 9)));		flip1 |= mO & (flip1 >> 1);	flip8 |= O & (flip8 >> 8);
	pre = _mm_and_si128(mOO, _mm_slli_epi64(mOO, 9));				pre1 >>= 1;			pre8 >>= 8;
	flip = _mm_or_si128(flip, _mm_and_si128(pre, _mm_slli_epi64(flip, 18)));	flip1 |= pre1 & (flip1 >> 2);	flip8 |= pre8 & (flip8 >> 16);
	flip = _mm_or_si128(flip, _mm_and_si128(pre, _mm_slli_epi64(flip, 18)));	flip1 |= pre1 & (flip1 >> 2);	flip8 |= pre8 & (flip8 >> 16);
	MM = _mm_or_si128(MM, _mm_slli_epi64(flip, 9));					moves |= flip1 >> 1;		moves |= flip8 >> 8;

	moves |= _mm_cvtsi128_si64(MM) | vertical_mirror(_mm_cvtsi128_si64(_mm_unpackhi_epi64(MM, MM)));
	return moves & ~(P|O);	// mask with empties
}

<<<<<<< HEAD
<<<<<<< HEAD
#elif defined(__aarch64__) || defined(_M_ARM64)	// 4 CPU
=======
#elif 0	// 4 CPU
>>>>>>> 3e1ed4f (fix cr/lf in repository to lf)
=======
#elif defined(__aarch64__) || defined(_M_ARM64)	// 4 CPU
>>>>>>> 343493d (More neon/sse optimizations; neon dispatch added for arm32)

unsigned long long get_moves(const unsigned long long P, const unsigned long long O)
{
	unsigned long long moves, mO;
	unsigned long long flip1, flip7, flip9, flip8, pre1, pre7, pre9, pre8;

	mO = O & 0x7e7e7e7e7e7e7e7eULL;
	flip1  = mO & (P << 1);		flip7  = mO & (P << 7);		flip9  = mO & (P << 9);		flip8  = O & (P << 8);
	flip1 |= mO & (flip1 << 1);	flip7 |= mO & (flip7 << 7);	flip9 |= mO & (flip9 << 9);	flip8 |= O & (flip8 << 8);
	pre1 = mO & (mO << 1);		pre7 = mO & (mO << 7);		pre9 = mO & (mO << 9);		pre8 = O & (O << 8);
	flip1 |= pre1 & (flip1 << 2);	flip7 |= pre7 & (flip7 << 14);	flip9 |= pre9 & (flip9 << 18);	flip8 |= pre8 & (flip8 << 16);
	flip1 |= pre1 & (flip1 << 2);	flip7 |= pre7 & (flip7 << 14);	flip9 |= pre9 & (flip9 << 18);	flip8 |= pre8 & (flip8 << 16);
	moves = flip1 << 1;		moves |= flip7 << 7;		moves |= flip9 << 9;		moves |= flip8 << 8;
	flip1  = mO & (P >> 1);		flip7  = mO & (P >> 7);		flip9  = mO & (P >> 9);		flip8  = O & (P >> 8);
	flip1 |= mO & (flip1 >> 1);	flip7 |= mO & (flip7 >> 7);	flip9 |= mO & (flip9 >> 9);	flip8 |= O & (flip8 >> 8);
	pre1 >>= 1;			pre7 >>= 7;			pre9 >>= 9;			pre8 >>= 8;
	flip1 |= pre1 & (flip1 >> 2);	flip7 |= pre7 & (flip7 >> 14);	flip9 |= pre9 & (flip9 >> 18);	flip8 |= pre8 & (flip8 >> 16);
	flip1 |= pre1 & (flip1 >> 2);	flip7 |= pre7 & (flip7 >> 14);	flip9 |= pre9 & (flip9 >> 18);	flip8 |= pre8 & (flip8 >> 16);
	moves |= flip1 >> 1;		moves |= flip7 >> 7;		moves |= flip9 >> 9;		moves |= flip8 >> 8;

	return moves & ~(P|O);	// mask with empties
}

<<<<<<< HEAD
<<<<<<< HEAD
#elif defined(__ARM_NEON)	// 3 Neon, 1 CPU(32)

  #ifndef DISPATCH_NEON
	#define	get_moves_sse	get_moves	// no dispatch
  #endif
=======
#else // __x86_64__
=======
#elif defined(__ARM_NEON__)	// 3 Neon, 1 CPU(32)

#ifdef hasNeon
#define	get_moves_sse	get_moves	// no dispatch
#endif

unsigned long long get_moves_sse(unsigned long long P, unsigned long long O)
{
	unsigned int	mO, movesL, movesH, flip1, pre1;
	uint64x1_t	rP, rO;
	uint64x2_t	PP, OO, MM, flip, pre;

		/* vertical_mirror in PP[1], OO[1] */				mO = (unsigned int) O & 0x7e7e7e7e;
	rP = vreinterpret_u64_u8(vrev64_u8(vcreate_u8(P)));			flip1  = mO & ((unsigned int) P << 1);
	PP = vcombine_u64(vcreate_u64(P), rP);					flip1 |= mO & (flip1 << 1);
										pre1   = mO & (mO << 1);
	rO = vreinterpret_u64_u8(vrev64_u8(vcreate_u8(O)));			flip1 |= pre1 & (flip1 << 2);
	OO = vcombine_u64(vcreate_u64(O), rO);					flip1 |= pre1 & (flip1 << 2);
										movesL = flip1 << 1;

	flip = vandq_u64(OO, vshlq_n_u64(PP, 8));				flip1  = mO & ((unsigned int) P >> 1);
	flip = vorrq_u64(flip, vandq_u64(OO, vshlq_n_u64(flip, 8)));		flip1 |= mO & (flip1 >> 1);
	pre  = vandq_u64(OO, vshlq_n_u64(OO, 8));				pre1 >>= 1;
	flip = vorrq_u64(flip, vandq_u64(pre, vshlq_n_u64(flip, 16)));		flip1 |= pre1 & (flip1 >> 2);
	flip = vorrq_u64(flip, vandq_u64(pre, vshlq_n_u64(flip, 16)));		flip1 |= pre1 & (flip1 >> 2);
	MM = vshlq_n_u64(flip, 8);						movesL |= flip1 >> 1;

	OO = vandq_u64(OO, vdupq_n_u64(0x7e7e7e7e7e7e7e7e));			mO = (unsigned int) (O >> 32) & 0x7e7e7e7e;
	flip = vandq_u64(OO, vshlq_n_u64(PP, 7));				flip1  = mO & ((unsigned int) (P >> 32) << 1);
	flip = vorrq_u64(flip, vandq_u64(OO, vshlq_n_u64(flip, 7)));		flip1 |= mO & (flip1 << 1);
	pre  = vandq_u64(OO, vshlq_n_u64(OO, 7));				pre1   = mO & (mO << 1);
	flip = vorrq_u64(flip, vandq_u64(pre, vshlq_n_u64(flip, 14)));		flip1 |= pre1 & (flip1 << 2);
	flip = vorrq_u64(flip, vandq_u64(pre, vshlq_n_u64(flip, 14)));		flip1 |= pre1 & (flip1 << 2);
	MM = vorrq_u64(MM, vshlq_n_u64(flip, 7));				movesH = flip1 << 1;

	flip = vandq_u64(OO, vshlq_n_u64(PP, 9));				flip1  = mO & ((unsigned int) (P >> 32) >> 1);
	flip = vorrq_u64(flip, vandq_u64(OO, vshlq_n_u64(flip, 9)));		flip1 |= mO & (flip1 >> 1);
	pre  = vandq_u64(OO, vshlq_n_u64(OO, 9));				pre1 >>= 1;
	flip = vorrq_u64(flip, vandq_u64(pre, vshlq_n_u64(flip, 18)));		flip1 |= pre1 & (flip1 >> 2);
	flip = vorrq_u64(flip, vandq_u64(pre, vshlq_n_u64(flip, 18)));		flip1 |= pre1 & (flip1 >> 2);
	MM = vorrq_u64(MM, vshlq_n_u64(flip, 9));				movesH |= flip1 >> 1;

	movesL |= vgetq_lane_u32(vreinterpretq_u32_u64(MM), 0) | bswap_int(vgetq_lane_u32(vreinterpretq_u32_u64(MM), 3));
	movesH |= vgetq_lane_u32(vreinterpretq_u32_u64(MM), 1) | bswap_int(vgetq_lane_u32(vreinterpretq_u32_u64(MM), 2));
	return (movesL | ((unsigned long long) movesH << 32)) & ~(P|O);	// mask with empties
}

#else // AVX/x86_64/arm
>>>>>>> 343493d (More neon/sse optimizations; neon dispatch added for arm32)
/**
 * @brief SSE optimized get_moves for x86 - 3 SSE, 1 CPU(32)
 *
 */
<<<<<<< HEAD
#if defined(hasSSE2) || defined(USE_MSVC_X86)
>>>>>>> 3e1ed4f (fix cr/lf in repository to lf)
=======
#if defined(hasSSE2) || defined(USE_MSVC_X86) || defined(ANDROID)

#ifdef hasSSE2
#define	get_moves_sse	get_moves	// no dispatch
#endif
>>>>>>> 343493d (More neon/sse optimizations; neon dispatch added for arm32)

unsigned long long get_moves_sse(const unsigned long long P, const unsigned long long O)
{
	unsigned int	mO, movesL, movesH, flip1, pre1;
<<<<<<< HEAD
	uint64x1_t	rP, rO;
	uint64x2_t	PP, OO, MM, flip, pre;

		/* vertical_mirror in PP[1], OO[1] */				mO = (unsigned int) O & 0x7e7e7e7e;
	rP = vreinterpret_u64_u8(vrev64_u8(vcreate_u8(P)));			flip1  = mO & ((unsigned int) P << 1);
	PP = vcombine_u64(vcreate_u64(P), rP);					flip1 |= mO & (flip1 << 1);
										pre1   = mO & (mO << 1);
	rO = vreinterpret_u64_u8(vrev64_u8(vcreate_u8(O)));			flip1 |= pre1 & (flip1 << 2);
	OO = vcombine_u64(vcreate_u64(O), rO);					flip1 |= pre1 & (flip1 << 2);
										movesL = flip1 << 1;

	flip = vandq_u64(OO, vshlq_n_u64(PP, 8));				flip1  = mO & ((unsigned int) P >> 1);
	flip = vorrq_u64(flip, vandq_u64(OO, vshlq_n_u64(flip, 8)));		flip1 |= mO & (flip1 >> 1);
	pre  = vandq_u64(OO, vshlq_n_u64(OO, 8));				pre1 >>= 1;
	flip = vorrq_u64(flip, vandq_u64(pre, vshlq_n_u64(flip, 16)));		flip1 |= pre1 & (flip1 >> 2);
	flip = vorrq_u64(flip, vandq_u64(pre, vshlq_n_u64(flip, 16)));		flip1 |= pre1 & (flip1 >> 2);
	MM = vshlq_n_u64(flip, 8);						movesL |= flip1 >> 1;

	OO = vandq_u64(OO, vdupq_n_u64(0x7e7e7e7e7e7e7e7e));			mO = (unsigned int) (O >> 32) & 0x7e7e7e7e;
	flip = vandq_u64(OO, vshlq_n_u64(PP, 7));				flip1  = mO & ((unsigned int) (P >> 32) << 1);
	flip = vorrq_u64(flip, vandq_u64(OO, vshlq_n_u64(flip, 7)));		flip1 |= mO & (flip1 << 1);
	pre  = vandq_u64(OO, vshlq_n_u64(OO, 7));				pre1   = mO & (mO << 1);
	flip = vorrq_u64(flip, vandq_u64(pre, vshlq_n_u64(flip, 14)));		flip1 |= pre1 & (flip1 << 2);
	flip = vorrq_u64(flip, vandq_u64(pre, vshlq_n_u64(flip, 14)));		flip1 |= pre1 & (flip1 << 2);
	MM = vorrq_u64(MM, vshlq_n_u64(flip, 7));				movesH = flip1 << 1;

	flip = vandq_u64(OO, vshlq_n_u64(PP, 9));				flip1  = mO & ((unsigned int) (P >> 32) >> 1);
	flip = vorrq_u64(flip, vandq_u64(OO, vshlq_n_u64(flip, 9)));		flip1 |= mO & (flip1 >> 1);
	pre  = vandq_u64(OO, vshlq_n_u64(OO, 9));				pre1 >>= 1;
	flip = vorrq_u64(flip, vandq_u64(pre, vshlq_n_u64(flip, 18)));		flip1 |= pre1 & (flip1 >> 2);
	flip = vorrq_u64(flip, vandq_u64(pre, vshlq_n_u64(flip, 18)));		flip1 |= pre1 & (flip1 >> 2);
	MM = vorrq_u64(MM, vshlq_n_u64(flip, 9));				movesH |= flip1 >> 1;

	movesL |= vgetq_lane_u32(vreinterpretq_u32_u64(MM), 0) | bswap_int(vgetq_lane_u32(vreinterpretq_u32_u64(MM), 3));
	movesH |= vgetq_lane_u32(vreinterpretq_u32_u64(MM), 1) | bswap_int(vgetq_lane_u32(vreinterpretq_u32_u64(MM), 2));
	return (movesL | ((unsigned long long) movesH << 32)) & ~(P|O);	// mask with empties
}

#else // AVX/x86_64/arm
/**
 * @brief SSE optimized get_moves for x86 - 3 SSE, 1 CPU(32)
 *
 */
  #if defined(hasSSE2) || defined(USE_MSVC_X86) || defined(ANDROID)

    #ifdef hasSSE2
	#define	get_moves_sse	get_moves	// no dispatch
    #endif

unsigned long long get_moves_sse(const unsigned long long P, const unsigned long long O)
{
	unsigned int	mO, movesL, movesH, flip1, pre1;
	__m128i	OP, rOP, PP, OO, MM, flip, pre;
=======
	__m128i	OP, rOP, PP, OO, MM, flip, pre;
<<<<<<< HEAD
	const __m128i mask7e = _mm_set1_epi8(0x7e);
>>>>>>> 3e1ed4f (fix cr/lf in repository to lf)
=======
>>>>>>> 343493d (More neon/sse optimizations; neon dispatch added for arm32)

		// vertical_mirror in PP[1], OO[1]
	OP  = _mm_unpacklo_epi64(_mm_cvtsi64_si128(P), _mm_cvtsi64_si128(O));		mO = (unsigned int) O & 0x7e7e7e7eU;
	rOP = _mm_shufflelo_epi16(OP, 0x1B);						flip1  = mO & ((unsigned int) P << 1);
	rOP = _mm_shufflehi_epi16(rOP, 0x1B);						flip1 |= mO & (flip1 << 1);
<<<<<<< HEAD
	rOP = _mm_or_si128(_mm_srli_epi16(rOP, 8), _mm_slli_epi16(rOP, 8));		pre1   = mO & (mO << 1);
=======
											pre1   = mO & (mO << 1);
	rOP = _mm_or_si128(_mm_srli_epi16(rOP, 8), _mm_slli_epi16(rOP, 8));
>>>>>>> 3e1ed4f (fix cr/lf in repository to lf)
	    										flip1 |= pre1 & (flip1 << 2);
	PP  = _mm_unpacklo_epi64(OP, rOP);						flip1 |= pre1 & (flip1 << 2);
	OO  = _mm_unpackhi_epi64(OP, rOP);						movesL = flip1 << 1;

	flip = _mm_and_si128(OO, _mm_slli_epi64(PP, 8));				flip1  = mO & ((unsigned int) P >> 1);
	flip = _mm_or_si128(flip, _mm_and_si128(OO, _mm_slli_epi64(flip, 8)));		flip1 |= mO & (flip1 >> 1);
	pre = _mm_and_si128(OO, _mm_slli_epi64(OO, 8));					pre1 >>= 1;
	flip = _mm_or_si128(flip, _mm_and_si128(pre, _mm_slli_epi64(flip, 16)));	flip1 |= pre1 & (flip1 >> 2);
	flip = _mm_or_si128(flip, _mm_and_si128(pre, _mm_slli_epi64(flip, 16)));	flip1 |= pre1 & (flip1 >> 2);
	MM = _mm_slli_epi64(flip, 8);							movesL |= flip1 >> 1;

<<<<<<< HEAD
<<<<<<< HEAD
	OO = _mm_and_si128(OO, _mm_set1_epi8(0x7e));					mO = (unsigned int) (O >> 32) & 0x7e7e7e7eU;
=======
	OO = _mm_and_si128(OO, mask7e);							mO = (unsigned int) (O >> 32) & 0x7e7e7e7eU;
>>>>>>> 3e1ed4f (fix cr/lf in repository to lf)
=======
	OO = _mm_and_si128(OO, _mm_set1_epi8(0x7e));					mO = (unsigned int) (O >> 32) & 0x7e7e7e7eU;
>>>>>>> 343493d (More neon/sse optimizations; neon dispatch added for arm32)
	flip = _mm_and_si128(OO, _mm_slli_epi64(PP, 7));				flip1  = mO & ((unsigned int) (P >> 32) << 1);
	flip = _mm_or_si128(flip, _mm_and_si128(OO, _mm_slli_epi64(flip, 7)));		flip1 |= mO & (flip1 << 1);
	pre = _mm_and_si128(OO, _mm_slli_epi64(OO, 7));					pre1   = mO & (mO << 1);
	flip = _mm_or_si128(flip, _mm_and_si128(pre, _mm_slli_epi64(flip, 14)));	flip1 |= pre1 & (flip1 << 2);
	flip = _mm_or_si128(flip, _mm_and_si128(pre, _mm_slli_epi64(flip, 14)));	flip1 |= pre1 & (flip1 << 2);
	MM = _mm_or_si128(MM, _mm_slli_epi64(flip, 7));					movesH = flip1 << 1;

	flip = _mm_and_si128(OO, _mm_slli_epi64(PP, 9));				flip1  = mO & ((unsigned int) (P >> 32) >> 1);
	flip = _mm_or_si128(flip, _mm_and_si128(OO, _mm_slli_epi64(flip, 9)));		flip1 |= mO & (flip1 >> 1);
	pre = _mm_and_si128(OO, _mm_slli_epi64(OO, 9));					pre1 >>= 1;
	flip = _mm_or_si128(flip, _mm_and_si128(pre, _mm_slli_epi64(flip, 18)));	flip1 |= pre1 & (flip1 >> 2);
	flip = _mm_or_si128(flip, _mm_and_si128(pre, _mm_slli_epi64(flip, 18)));	flip1 |= pre1 & (flip1 >> 2);
	MM = _mm_or_si128(MM, _mm_slli_epi64(flip, 9));					movesH |= flip1 >> 1;

	movesL |= _mm_cvtsi128_si32(MM);	MM = _mm_srli_si128(MM, 4);
	movesH |= _mm_cvtsi128_si32(MM);	MM = _mm_srli_si128(MM, 4);
	movesH |= bswap_int(_mm_cvtsi128_si32(MM));
	movesL |= bswap_int(_mm_cvtsi128_si32(_mm_srli_si128(MM, 4)));
	return (movesL | ((unsigned long long) movesH << 32)) & ~(P|O);	// mask with empties
}

<<<<<<< HEAD
  #else // non-VEX asm

unsigned long long get_moves_sse(const unsigned long long P, const unsigned long long O)
=======
#else // non-VEX asm

<<<<<<< HEAD
unsigned long long get_moves_sse(unsigned long long P, unsigned long long O)
>>>>>>> 3e1ed4f (fix cr/lf in repository to lf)
=======
unsigned long long get_moves_sse(const unsigned long long P, const unsigned long long O)
>>>>>>> 21f8809 (Share all full lines between get_stability and Dogaishi hash reduction)
{
	unsigned long long moves;
	static const V2DI mask7e = {{ 0x7e7e7e7e7e7e7e7eULL, 0x7e7e7e7e7e7e7e7eULL }};

	__asm__ (
		"movl	%1, %%ebx\n\t"
		"movl	%3, %%edi\n\t"
		"andl	$0x7e7e7e7e, %%edi\n\t"
				/* shift=-1 */			/* vertical mirror in PP[1], OO[1] */
		"movl	%%ebx, %%eax\n\t"	"movd	%1, %%xmm4\n\t"		// (movd for store-forwarding)
		"shrl	$1, %%eax\n\t"		"movd	%2, %%xmm0\n\t"
		"andl	%%edi, %%eax\n\t"	"movd	%3, %%xmm5\n\t"
		"movl	%%eax, %%edx\n\t"	"movd	%4, %%xmm1\n\t"
		"shrl	$1, %%eax\n\t"		"punpckldq %%xmm0, %%xmm4\n\t"		// P
		"movl	%%edi, %%ecx\n\t"	"punpckldq %%xmm1, %%xmm5\n\t"		// O
		"andl	%%edi, %%eax\n\t"	"punpcklqdq %%xmm5, %%xmm4\n\t"		// OP
		"shrl	$1, %%ecx\n\t"		"pshuflw $0x1b, %%xmm4, %%xmm0\n\t"
		"orl	%%edx, %%eax\n\t"	"pshufhw $0x1b, %%xmm0, %%xmm0\n\t"
		"andl	%%edi, %%ecx\n\t"	"movdqa	%%xmm0, %%xmm1\n\t"
		"movl	%%eax, %%edx\n\t"	"psllw	$8, %%xmm0\n\t"
		"shrl	$2, %%eax\n\t"		"psrlw	$8, %%xmm1\n\t"
		"andl	%%ecx, %%eax\n\t"	"por	%%xmm1, %%xmm0\n\t"		// rOP
		"orl	%%eax, %%edx\n\t"
		"shrl	$2, %%eax\n\t"		"movdqa	%%xmm4, %%xmm5\n\t"
		"andl	%%ecx, %%eax\n\t"	"punpcklqdq %%xmm0, %%xmm4\n\t"		// PP
		"orl	%%edx, %%eax\n\t"	"punpckhqdq %%xmm0, %%xmm5\n\t"		// OO
		"shrl	$1, %%eax\n\t"
				/* shift=+1 */			/* shift=-8:+8 */
						"movdqa	%%xmm4, %%xmm0\n\t"
		"addl	%%ebx, %%ebx\n\t"	"psllq	$8, %%xmm0\n\t"
		"andl	%%edi, %%ebx\n\t"	"pand	%%xmm5, %%xmm0\n\t"	// 0 m7&o6 m6&o5 .. m1&o0
		"movl	%%ebx, %%edx\n\t"	"movdqa	%%xmm0, %%xmm1\n\t"
		"addl	%%ebx, %%ebx\n\t"	"psllq	$8, %%xmm0\n\t"
						"movdqa	%%xmm5, %%xmm3\n\t"
		"andl	%%edi, %%ebx\n\t"	"pand	%%xmm5, %%xmm0\n\t"	// 0 0 m7&o6&o5 .. m2&o1&o0
						"psllq	$8, %%xmm3\n\t"
		"orl	%%ebx, %%edx\n\t"	"por	%%xmm1, %%xmm0\n\t"	// 0 m7&o6 (m6&o5)|(m7&o6&o5) .. (m1&o0)
		"addl	%%ecx, %%ecx\n\t"	"pand	%%xmm5, %%xmm3\n\t"	// 0 o7&o6 o6&o5 o5&o4 o4&o3 ..
						"movdqa	%%xmm0, %%xmm2\n\t"
		"leal	(,%%edx,4), %%ebx\n\t"	"psllq	$16, %%xmm0\n\t"
		"andl	%%ecx, %%ebx\n\t"	"pand	%%xmm3, %%xmm0\n\t"	// 0 0 0 m7&o6&o5&o4 (m6&o5&o4&o3)|(m7&o6&o5&o4&o3) ..
		"orl	%%ebx, %%edx\n\t"	"por	%%xmm0, %%xmm2\n\t"
		"shll	$2, %%ebx\n\t"		"psllq	$16, %%xmm0\n\t"
		"andl	%%ecx, %%ebx\n\t"	"pand	%%xmm3, %%xmm0\n\t"	// 0 0 0 0 0 m7&o6&..&o2 (m6&o5&..&o1)|(m7&o6&..&o1) ..
		"orl	%%edx, %%ebx\n\t"	"por	%%xmm0, %%xmm2\n\t"
		"addl	%%ebx, %%ebx\n\t"	"psllq	$8, %%xmm2\n\t"
		"orl	%%eax, %%ebx\n\t"

		"movl	%2, %%esi\n\t"
		"movl	%4, %%edi\n\t"
				/* shift=-1 */			/* shift=-9:+7 */
		"andl	$0x7e7e7e7e,%%edi\n\t"	"pand	%5, %%xmm5\n\t"
		"movl	%%esi, %%eax\n\t"	"movdqa	%%xmm4, %%xmm0\n\t"
		"shrl	$1, %%eax\n\t"		"psllq	$7, %%xmm0\n\t"
		"andl	%%edi, %%eax\n\t"	"pand	%%xmm5, %%xmm0\n\t"
		"movl	%%eax, %%edx\n\t"	"movdqa	%%xmm0, %%xmm1\n\t"
		"shrl	$1, %%eax\n\t"		"psllq	$7, %%xmm0\n\t"
		"andl	%%edi, %%eax\n\t"	"pand	%%xmm5, %%xmm0\n\t"
		"movl	%%edi, %%ecx\n\t"	"movdqa	%%xmm5, %%xmm3\n\t"
		"orl	%%edx, %%eax\n\t"	"por	%%xmm1, %%xmm0\n\t"
		"shrl	$1, %%ecx\n\t"		"psllq	$7, %%xmm3\n\t"
		"movl	%%eax, %%edx\n\t"	"movdqa	%%xmm0, %%xmm1\n\t"
		"andl	%%edi, %%ecx\n\t"	"pand	%%xmm5, %%xmm3\n\t"
		"shrl	$2, %%eax\n\t"		"psllq	$14, %%xmm0\n\t"
		"andl	%%ecx, %%eax\n\t"	"pand	%%xmm3, %%xmm0\n\t"
		"orl	%%eax, %%edx\n\t"	"por	%%xmm0, %%xmm1\n\t"
		"shrl	$2, %%eax\n\t"		"psllq	$14, %%xmm0\n\t"
		"andl	%%ecx, %%eax\n\t"	"pand	%%xmm3, %%xmm0\n\t"
		"orl	%%edx, %%eax\n\t"	"por	%%xmm1, %%xmm0\n\t"
		"shrl	$1, %%eax\n\t"		"psllq	$7, %%xmm0\n\t"
						"por	%%xmm0, %%xmm2\n\t"
				/* shift=+1 */			/* shift=-7:+9 */
						"movdqa	%%xmm4, %%xmm0\n\t"
		"addl	%%esi, %%esi\n\t"	"psllq	$9, %%xmm0\n\t"
		"andl	%%edi, %%esi\n\t"	"pand	%%xmm5, %%xmm0\n\t"
		"movl	%%esi, %%edx\n\t"	"movdqa	%%xmm0, %%xmm1\n\t"
		"addl	%%esi, %%esi\n\t"	"psllq	$9, %%xmm0\n\t"
		"andl	%%edi, %%esi\n\t"	"pand	%%xmm5, %%xmm0\n\t"
						"movdqa	%%xmm5, %%xmm3\n\t"
		"orl	%%esi, %%edx\n\t"	"por	%%xmm1, %%xmm0\n\t"
						"psllq	$9, %%xmm3\n\t"
						"movdqa	%%xmm0, %%xmm1\n\t"
		"addl	%%ecx, %%ecx\n\t"	"pand	%%xmm5, %%xmm3\n\t"
		"leal	(,%%edx,4), %%esi\n\t"	"psllq	$18, %%xmm0\n\t"
		"andl	%%ecx, %%esi\n\t"	"pand	%%xmm3, %%xmm0\n\t"
		"orl	%%esi, %%edx\n\t"	"por	%%xmm0, %%xmm1\n\t"
		"shll	$2, %%esi\n\t"		"psllq	$18, %%xmm0\n\t"
		"andl	%%ecx, %%esi\n\t"	"pand	%%xmm3, %%xmm0\n\t"
		"orl	%%edx, %%esi\n\t"	"por	%%xmm1, %%xmm0\n\t"
		"addl	%%esi, %%esi\n\t"	"psllq	$9, %%xmm0\n\t"
		"orl	%%eax, %%esi\n\t"	"por	%%xmm0, %%xmm2\n\t"

		"movl	%1, %%eax\n\t"		"movhlps %%xmm2, %%xmm3\n\t"
		"movl	%2, %%edx\n\t"		"movd	%%xmm3, %%edi\n\t"	"movd	%%xmm2, %%ecx\n\t"
						"psrlq	$32, %%xmm3\n\t"	"psrlq	$32, %%xmm2\n\t"
						"bswapl	%%edi\n\t"		"orl	%%ecx, %%ebx\n\t"
		"orl	%3, %%eax\n\t"		"orl	%%edi, %%esi\n\t"
		"orl	%4, %%edx\n\t"		"movd	%%xmm3, %%edi\n\t"	"movd	%%xmm2, %%ecx\n\t"
		"notl	%%eax\n\t"		"bswapl	%%edi\n\t"
		"notl	%%edx\n\t"		"orl	%%edi, %%ebx\n\t"	"orl	%%ecx, %%esi\n\t"
		"andl	%%esi, %%edx\n\t"
		"andl	%%ebx, %%eax"
	: "=&A" (moves)
	: "m" (P), "m" (((unsigned int *)&P)[1]), "m" (O), "m" (((unsigned int *)&O)[1]), "m" (mask7e)
	: "ebx", "ecx", "esi", "edi" );

	return moves;
}

<<<<<<< HEAD
  #endif // hasSSE2
#endif // x86

#if defined(hasSSE2) || (defined(__ARM_NEON) && !defined(DISPATCH_NEON))

/**
 * @brief SSE/neon optimized get_stable_edge
=======
#endif // hasSSE2
#endif // x86

#if defined(hasSSE2) || defined(hasNeon)	// no dispatch

/**
 * @brief SSE optimized get_stable_edge
>>>>>>> 3e1ed4f (fix cr/lf in repository to lf)
 *
 * @param P bitboard with player's discs.
 * @param O bitboard with opponent's discs.
 * @return a bitboard with (some of) player's stable discs.
 *
 */
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
  #if defined(__aarch64__) || defined(_M_ARM64)	// for vaddvq
unsigned long long get_stable_edge(unsigned long long P, unsigned long long O)
{	// compute the exact stable edges (from precomputed tables)
	// const int16x8_t shiftv = { 0, 1, 2, 3, 4, 5, 6, 7 };	// error on MSVC
	const uint64x2_t shiftv = { 0x0003000200010000, 0x0007000600050004 };
	uint8x16_t PO = vzip1q_u8(vreinterpretq_u8_u64(vdupq_n_u64(O)), vreinterpretq_u8_u64(vdupq_n_u64(P)));
	unsigned int a1a8 = edge_stability[vaddvq_u16(vshlq_u16(vreinterpretq_u16_u8(vandq_u8(PO, vdupq_n_u8(1))), vreinterpretq_s16_u64(shiftv)))];
	unsigned int h1h8 = edge_stability[vaddvq_u16(vshlq_u16(vreinterpretq_u16_u8(vshrq_n_u8(PO, 7)), vreinterpretq_s16_u64(shiftv)))];
	return edge_stability[vgetq_lane_u16(vreinterpretq_u16_u8(PO), 0)]
	    |  (unsigned long long) edge_stability[vgetq_lane_u16(vreinterpretq_u16_u8(PO), 7)] << 56
	    |  unpackA2A7(a1a8) | unpackH2H7(h1h8);
}

  #elif defined(__ARM_NEON)	// Neon kindergarten
unsigned long long get_stable_edge(unsigned long long P, unsigned long long O)
{	// compute the exact stable edges (from precomputed tables)
	const uint64x2_t kMul  = { 0x1020408001020408, 0x1020408001020408 };
	uint64x2_t PP = vcombine_u64(vshl_n_u64(vcreate_u64(P), 7), vcreate_u64(P));
	uint64x2_t OO = vcombine_u64(vshl_n_u64(vcreate_u64(O), 7), vcreate_u64(O));
	uint32x4_t QP = vmulq_u32(vreinterpretq_u32_u64(kMul), vreinterpretq_u32_u8(vshrq_n_u8(vreinterpretq_u8_u64(PP), 7)));
	uint32x4_t QO = vmulq_u32(vreinterpretq_u32_u64(kMul), vreinterpretq_u32_u8(vshrq_n_u8(vreinterpretq_u8_u64(OO), 7)));
	uint32x2_t DP = vpadd_u32(vget_low_u32(QP), vget_high_u32(QP));	// P_h1h8 * * * P_a1a8 * * *
	uint32x2_t DO = vpadd_u32(vget_low_u32(QO), vget_high_u32(QO));	// O_h1h8 * * * O_a1a8 * * *
	uint8x8_t DB = vtrn_u8(vreinterpret_u8_u32(DO), vreinterpret_u8_u32(DP)).val[1];	// P_h1h8 O_h1h8 * * P_a1a8 O_a1a8 * *
	unsigned int a1a8 = edge_stability[vget_lane_u16(vreinterpret_u16_u8(DB), 1)];
	unsigned int h1h8 = edge_stability[vget_lane_u16(vreinterpret_u16_u8(DB), 3)];
	uint8x16_t PO = vzipq_u8(vreinterpretq_u8_u64(OO), vreinterpretq_u8_u64(PP)).val[1];
	return edge_stability[vgetq_lane_u16(vreinterpretq_u16_u8(PO), 0)]
	    |  (unsigned long long) edge_stability[vgetq_lane_u16(vreinterpretq_u16_u8(PO), 7)] << 56
	    |  unpackA2A7(a1a8) | unpackH2H7(h1h8);
}

  #elif defined(hasSSE2)
unsigned long long get_stable_edge(const unsigned long long P, const unsigned long long O)
{
	// compute the exact stable edges (from precomputed tables)
	unsigned int a1a8, h1h8;
=======
static unsigned long long get_stable_edge(const unsigned long long P, const unsigned long long O)
=======
#if defined(__aarch64__) || defined(_M_ARM64)
unsigned long long get_stable_edge(unsigned long long P, unsigned long long O)
=======
#if defined(__aarch64__) || defined(_M_ARM64)	// for vaddvq
<<<<<<< HEAD
unsigned long long get_stable_edge_sse(unsigned long long P, unsigned long long O)
>>>>>>> 9e2bbc5 (split get_all_full_lines from get_stability)
=======
unsigned long long get_stable_edge(unsigned long long P, unsigned long long O)
>>>>>>> 21f8809 (Share all full lines between get_stability and Dogaishi hash reduction)
{	// compute the exact stable edges (from precomputed tables)
	// const int16x8_t shiftv = { 0, 1, 2, 3, 4, 5, 6, 7 };	// error on MSVC
	const uint64x2_t shiftv = { 0x0003000200010000, 0x0007000600050004 };
	uint8x16_t PO = vzip1q_u8(vreinterpretq_u8_u64(vdupq_n_u64(O)), vreinterpretq_u8_u64(vdupq_n_u64(P)));
	unsigned int a1a8 = edge_stability[vaddvq_u16(vshlq_u16(vreinterpretq_u16_u8(vandq_u8(PO, vdupq_n_u8(1))), vreinterpretq_s16_u64(shiftv)))];
	unsigned int h1h8 = edge_stability[vaddvq_u16(vshlq_u16(vreinterpretq_u16_u8(vshrq_n_u8(PO, 7)), vreinterpretq_s16_u64(shiftv)))];
	return edge_stability[vgetq_lane_u16(vreinterpretq_u16_u8(PO), 0)]
	    |  (unsigned long long) edge_stability[vgetq_lane_u16(vreinterpretq_u16_u8(PO), 7)] << 56
	    |  unpackA1A8(a1a8) | unpackH1H8(h1h8);
}

#elif defined(__ARM_NEON__) // Neon kindergarten
unsigned long long get_stable_edge(unsigned long long P, unsigned long long O)
{	// compute the exact stable edges (from precomputed tables)
	const uint64x2_t kMul  = { 0x1020408001020408, 0x1020408001020408 };
	uint64x2_t PP = vcombine_u64(vshl_n_u64(vcreate_u64(P), 7), vcreate_u64(P));
	uint64x2_t OO = vcombine_u64(vshl_n_u64(vcreate_u64(O), 7), vcreate_u64(O));
	uint32x4_t QP = vmulq_u32(vreinterpretq_u32_u64(kMul), vreinterpretq_u32_u8(vshrq_n_u8(vreinterpretq_u8_u64(PP), 7)));
	uint32x4_t QO = vmulq_u32(vreinterpretq_u32_u64(kMul), vreinterpretq_u32_u8(vshrq_n_u8(vreinterpretq_u8_u64(OO), 7)));
	uint32x2_t DP = vpadd_u32(vget_low_u32(QP), vget_high_u32(QP));	// P_h1h8 * * * P_a1a8 * * *
	uint32x2_t DO = vpadd_u32(vget_low_u32(QO), vget_high_u32(QO));	// O_h1h8 * * * O_a1a8 * * *
	uint8x8_t DB = vtrn_u8(vreinterpret_u8_u32(DO), vreinterpret_u8_u32(DP)).val[1];	// P_h1h8 O_h1h8 * * P_a1a8 O_a1a8 * *
	unsigned int a1a8 = edge_stability[vget_lane_u16(vreinterpret_u16_u8(DB), 1)];
	unsigned int h1h8 = edge_stability[vget_lane_u16(vreinterpret_u16_u8(DB), 3)];
	uint8x16_t PO = vzipq_u8(vreinterpretq_u8_u64(OO), vreinterpretq_u8_u64(PP)).val[1];
	return edge_stability[vgetq_lane_u16(vreinterpretq_u16_u8(PO), 0)]
	    |  (unsigned long long) edge_stability[vgetq_lane_u16(vreinterpretq_u16_u8(PO), 7)] << 56
	    |  unpackA1A8(a1a8) | unpackH1H8(h1h8);
}

<<<<<<< HEAD
#elif defined(__x86_64__) || defined(_M_X64)
unsigned long long get_stable_edge(const unsigned long long P, const unsigned long long O)
>>>>>>> 343493d (More neon/sse optimizations; neon dispatch added for arm32)
=======
#elif defined(hasSSE2) || defined(USE_MSVC_X86)
<<<<<<< HEAD
unsigned long long get_stable_edge_sse(const unsigned long long P, const unsigned long long O)
>>>>>>> 9e2bbc5 (split get_all_full_lines from get_stability)
=======
unsigned long long get_stable_edge(const unsigned long long P, const unsigned long long O)
>>>>>>> 21f8809 (Share all full lines between get_stability and Dogaishi hash reduction)
{
	// compute the exact stable edges (from precomputed tables)
<<<<<<< HEAD
	unsigned int a1a8po, h1h8po;
>>>>>>> 3e1ed4f (fix cr/lf in repository to lf)
=======
	unsigned int a1a8, h1h8;
>>>>>>> 93110ce (Use computation or optional pdep to unpack A1_A8)
	unsigned long long stable_edge;

	__m128i	P0 = _mm_cvtsi64_si128(P);
	__m128i	O0 = _mm_cvtsi64_si128(O);
	__m128i	PO = _mm_unpacklo_epi8(O0, P0);
	stable_edge = edge_stability[_mm_extract_epi16(PO, 0)]
		| ((unsigned long long) edge_stability[_mm_extract_epi16(PO, 7)] << 56);

	PO = _mm_unpacklo_epi64(O0, P0);
<<<<<<< HEAD
<<<<<<< HEAD
	a1a8 = edge_stability[_mm_movemask_epi8(_mm_slli_epi64(PO, 7))];
	h1h8 = edge_stability[_mm_movemask_epi8(PO)];
	stable_edge |= unpackA2A7(a1a8) | unpackH2H7(h1h8);

	return stable_edge;
}
  #endif

/**
 * @brief SSE/neon optimized get_edge_stability
 *
 * Compute the exact stable edges from precomputed tables.
 *
 * @param P bitboard with player's discs.
 * @param O bitboard with opponent's discs.
 * @return the number of stable discs on the edges.
 *
 */
  #if defined(__aarch64__) || defined(_M_ARM64)	// for vaddvq
int get_edge_stability(const unsigned long long P, const unsigned long long O)
{
	const uint64x2_t shiftv = { 0x0003000200010000, 0x0007000600050004 };
	uint8x16_t PO = vzip1q_u8(vreinterpretq_u8_u64(vdupq_n_u64(O)), vreinterpretq_u8_u64(vdupq_n_u64(P)));
	uint8x8_t packedstable = vcreate_u8((edge_stability[vgetq_lane_u16(vreinterpretq_u16_u8(PO), 0)]
	  | edge_stability[vgetq_lane_u16(vreinterpretq_u16_u8(PO), 7)] << 8) & 0x7e7e);
	packedstable = vset_lane_u8(edge_stability[vaddvq_u16(vshlq_u16(vreinterpretq_u16_u8(vandq_u8(PO, vdupq_n_u8(1))), vreinterpretq_s16_u64(shiftv)))], packedstable, 2);
	packedstable = vset_lane_u8(edge_stability[vaddvq_u16(vshlq_u16(vreinterpretq_u16_u8(vshrq_n_u8(PO, 7)), vreinterpretq_s16_u64(shiftv)))], packedstable, 3);
	return vaddv_u8(vcnt_u8(packedstable));
}

  #elif defined(__ARM_NEON)	// Neon kindergarten
int get_edge_stability(const unsigned long long P, const unsigned long long O)
{
	const uint64x2_t kMul  = { 0x1020408001020408, 0x1020408001020408 };
	uint64x2_t PP = vcombine_u64(vshl_n_u64(vcreate_u64(P), 7), vcreate_u64(P));
	uint64x2_t OO = vcombine_u64(vshl_n_u64(vcreate_u64(O), 7), vcreate_u64(O));
	uint32x4_t QP = vmulq_u32(vreinterpretq_u32_u64(kMul), vreinterpretq_u32_u8(vshrq_n_u8(vreinterpretq_u8_u64(PP), 7)));
	uint32x4_t QO = vmulq_u32(vreinterpretq_u32_u64(kMul), vreinterpretq_u32_u8(vshrq_n_u8(vreinterpretq_u8_u64(OO), 7)));
	uint32x2_t DP = vpadd_u32(vget_low_u32(QP), vget_high_u32(QP));	// P_h1h8 * * * P_a1a8 * * *
	uint32x2_t DO = vpadd_u32(vget_low_u32(QO), vget_high_u32(QO));	// O_h1h8 * * * O_a1a8 * * *
	uint8x8_t DB = vtrn_u8(vreinterpret_u8_u32(DO), vreinterpret_u8_u32(DP)).val[1];	// P_h1h8 O_h1h8 * * P_a1a8 O_a1a8 * *
	uint8x16_t PO = vzipq_u8(vreinterpretq_u8_u64(OO), vreinterpretq_u8_u64(PP)).val[1];
	uint8x8_t packedstable = vcreate_u8((edge_stability[vgetq_lane_u16(vreinterpretq_u16_u8(PO), 0)]
	  | edge_stability[vgetq_lane_u16(vreinterpretq_u16_u8(PO), 7)] << 8) & 0x7e7e);
	packedstable = vset_lane_u8(edge_stability[vget_lane_u16(vreinterpret_u16_u8(DB), 1)], packedstable, 2);
	packedstable = vset_lane_u8(edge_stability[vget_lane_u16(vreinterpret_u16_u8(DB), 3)], packedstable, 3);
	return vget_lane_u32(vpaddl_u16(vpaddl_u8(vcnt_u8(packedstable))), 0);
}

  #elif defined(hasSSE2)
int get_edge_stability(const unsigned long long P, const unsigned long long O)
{
	__m128i	P0 = _mm_cvtsi64_si128(P);
	__m128i	O0 = _mm_cvtsi64_si128(O);
	__m128i	PO = _mm_unpacklo_epi8(O0, P0);
	unsigned int packedstable = edge_stability[_mm_extract_epi16(PO, 0)] | edge_stability[_mm_extract_epi16(PO, 7)] << 8;
	PO = _mm_unpacklo_epi64(O0, P0);
	packedstable |= edge_stability[_mm_movemask_epi8(_mm_slli_epi64(PO, 7))] << 16 | edge_stability[_mm_movemask_epi8(PO)] << 24;
	return bit_count_32(packedstable & 0xffff7e7e);
}
  #endif

/**
 * @brief AVX2/SSE/neon optimized get_full_lines.
=======
	a1a8po = _mm_movemask_epi8(_mm_slli_epi64(PO, 7));
	h1h8po = _mm_movemask_epi8(PO);
#if 0 // def __BMI2__ // pdep is slow on AMD
	stable_edge |= _pdep_u64(edge_stability[a1a8po], 0x0101010101010101)
		| _pdep_u64(edge_stability[h1h8po], 0x8080808080808080);
#else
	stable_edge |= A1_A8[edge_stability[a1a8po]] | (A1_A8[edge_stability[h1h8po]] << 7);
#endif
=======
	a1a8 = edge_stability[_mm_movemask_epi8(_mm_slli_epi64(PO, 7))];
	h1h8 = edge_stability[_mm_movemask_epi8(PO)];
	stable_edge |= unpackA1A8(a1a8) | unpackH1H8(h1h8);

>>>>>>> 93110ce (Use computation or optional pdep to unpack A1_A8)
	return stable_edge;
}
#endif

/**
 * @brief X64 optimized get_stability
>>>>>>> 3e1ed4f (fix cr/lf in repository to lf)
 *
 * SSE pcmpeqb for horizontal get_full_lines.
 * CPU rotate for vertical get_full_lines.
 * Diag-7 is converted to diag-9 using vertical mirroring.
 * 
<<<<<<< HEAD
 * @param disc all discs on the board.
 * @param full all 1 if full line, otherwise all 0.
 */
  #ifdef __AVX2__

static __m256i vectorcall get_full_lines(const unsigned long long disc)
{
	__m128i l81, l79, l8;
	__m256i	v4_disc, lr79;
	const __m128i kff  = _mm_set1_epi8(-1);
    #if 0 // PCMPEQQ
=======
 * @param P bitboard with player's discs.
 * @param O bitboard with opponent's discs.
 * @return the number of stable discs.
 */
#ifdef __AVX2__
  #ifndef HAS_CPU_64
#define _mm_insert_epi64(s2,s,ndx)	_mm_insert_epi32(_mm_insert_epi32((s2), (s), (ndx) * 2), (s) >> 32, (ndx) * 2 + 1)
  #endif

void get_all_full_lines(const unsigned long long disc, unsigned long long full[5])
{
	unsigned long long l8;
	__m128i l81, l79;
	__m256i	v4_disc, lr79;
	const __m128i kff  = _mm_set1_epi64x(0xffffffffffffffff);
#if 0 // PCMPEQQ
>>>>>>> 3e1ed4f (fix cr/lf in repository to lf)
	static const V4DI m791 = {{ 0x0402010000804020, 0x2040800000010204, 0x0804020180402010, 0x1020408001020408 }};	// V8SI
	static const V4DI m792 = {{ 0x0000008040201008, 0x0000000102040810, 0x1008040201000000, 0x0810204080000000 }};
	static const V4DI m793 = {{ 0x0000804020100804, 0x0000010204081020, 0x2010080402010000, 0x0408102040800000 }};
	static const V4DI m794 = {{ 0x0080402010080402, 0x0001020408102040, 0x4020100804020100, 0x0204081020408000 }};
	static const V2DI m795 = {{ 0x8040201008040201, 0x0102040810204080 }};

<<<<<<< HEAD
	l81 = _mm_cvtsi64_si128(disc);				v4_disc = _mm256_broadcastq_epi64(l81);
	l81 = _mm_cmpeq_epi8(kff, l81);				lr79 = _mm256_and_si256(_mm256_cmpeq_epi32(_mm256_and_si256(v4_disc, m791.v4), m791.v4), m791.v4);
								lr79 = _mm256_or_si256(lr79, _mm256_and_si256(_mm256_cmpeq_epi64(_mm256_and_si256(v4_disc, m792.v4), m792.v4), m792.v4));
	l8 = _mm256_castsi256_si128(v4_disc);			lr79 = _mm256_or_si256(lr79, _mm256_and_si256(_mm256_cmpeq_epi64(_mm256_and_si256(v4_disc, m793.v4), m793.v4), m793.v4));
	l8 = _mm_and_si128(l8, _mm_alignr_epi8(l8, l8, 1));	lr79 = _mm256_or_si256(lr79, _mm256_and_si256(_mm256_cmpeq_epi64(_mm256_and_si256(v4_disc, m794.v4), m794.v4), m794.v4));
	l8 = _mm_and_si128(l8, _mm_alignr_epi8(l8, l8, 2));	l79 = _mm_and_si128(_mm_cmpeq_epi64(_mm_and_si128(_mm256_castsi256_si128(v4_disc), m795.v2), m795.v2), m795.v2);
	l8 = _mm_and_si128(l8, _mm_alignr_epi8(l8, l8, 4));	l79 = _mm_or_si128(l79, _mm_or_si128(_mm256_extracti128_si256(lr79, 1), _mm256_castsi256_si128(lr79)));

    #elif 0 // PCMPEQD
=======
	l81 = _mm_cvtsi64_si128(disc);		v4_disc = _mm256_broadcastq_epi64(l81);
	l81 = _mm_cmpeq_epi8(kff, l81);		lr79 = _mm256_and_si256(_mm256_cmpeq_epi32(_mm256_and_si256(v4_disc, m791.v4), m791.v4), m791.v4);
						lr79 = _mm256_or_si256(lr79, _mm256_and_si256(_mm256_cmpeq_epi64(_mm256_and_si256(v4_disc, m792.v4), m792.v4), m792.v4));
	l8 = disc;				lr79 = _mm256_or_si256(lr79, _mm256_and_si256(_mm256_cmpeq_epi64(_mm256_and_si256(v4_disc, m793.v4), m793.v4), m793.v4));
	l8 &= (l8 >> 8) | (l8 << 56);		lr79 = _mm256_or_si256(lr79, _mm256_and_si256(_mm256_cmpeq_epi64(_mm256_and_si256(v4_disc, m794.v4), m794.v4), m794.v4));
	l8 &= (l8 >> 16) | (l8 << 48);		l79 = _mm_and_si128(_mm_cmpeq_epi64(_mm_and_si128(_mm256_castsi256_si128(v4_disc), m795.v2), m795.v2), m795.v2);
	l8 &= (l8 >> 32) | (l8 << 32);		l79 = _mm_or_si128(l79, _mm_or_si128(_mm256_extracti128_si256(lr79, 1), _mm256_castsi256_si128(lr79)));

#elif 0 // PCMPEQD
>>>>>>> 3e1ed4f (fix cr/lf in repository to lf)
	__m256i lm79;
	static const V4DI m790 = {{ 0x80c0e0f0783c1e0f, 0x0103070f1e3c78f0, 0x70381c0e07030100, 0x0e1c3870e0c08000 }};
	static const V4DI m791 = {{ 0x0402010000804020, 0x2040800000010204, 0x0804020180402010, 0x1020408001020408 }};	// V8SI
	static const V4DI m792 = {{ 0x2010884440201088, 0x0408112202040811, 0x2211080411080402, 0x4488102088102040 }};	// V8SI
	static const V4DI m793 = {{ 0x8844221110884422, 0x1122448808112244, 0x0000000044221108, 0x0000000022448810 }};	// V8SI

<<<<<<< HEAD
	l81 = _mm_cvtsi64_si128(disc);				v4_disc = _mm256_broadcastq_epi64(l81);
	l81 = _mm_cmpeq_epi8(kff, l81);				lm79 = _mm256_and_si256(v4_disc, m790.v4);
								lm79 = _mm256_or_si256(lm79, _mm256_shuffle_epi32(lm79, 0xb1));
	l8 = _mm256_castsi256_si128(v4_disc);			lr79 = _mm256_and_si256(_mm256_cmpeq_epi32(_mm256_and_si256(lm79, m792.v4), m792.v4), m792.v4);
	l8 = _mm_and_si128(l8, _mm_alignr_epi8(l8, l8, 1));	lr79 = _mm256_or_si256(lr79, _mm256_and_si256(_mm256_cmpeq_epi32(_mm256_and_si256(lm79, m793.v4), m793.v4), m793.v4));
	l8 = _mm_and_si128(l8, _mm_alignr_epi8(l8, l8, 2));	lr79 = _mm256_and_si256(_mm256_or_si256(lr79, _mm256_shuffle_epi32(lr79, 0xb1)), m790.v4);
	l8 = _mm_and_si128(l8, _mm_alignr_epi8(l8, l8, 4));	lr79 = _mm256_or_si256(lr79, _mm256_and_si256(_mm256_cmpeq_epi32(_mm256_and_si256(v4_disc, m791.v4), m791.v4), m791.v4));
								l79 = _mm_or_si128(_mm256_extracti128_si256(lr79, 1), _mm256_castsi256_si128(lr79));

    #else // Kogge-Stone
=======
	l81 = _mm_cvtsi64_si128(disc);		v4_disc = _mm256_broadcastq_epi64(l81);
	l81 = _mm_cmpeq_epi8(kff, l81);		lm79 = _mm256_and_si256(v4_disc, m790.v4);
						lm79 = _mm256_or_si256(lm79, _mm256_shuffle_epi32(lm79, 0xb1));
	l8 = disc;				lr79 = _mm256_and_si256(_mm256_cmpeq_epi32(_mm256_and_si256(lm79, m792.v4), m792.v4), m792.v4);
	l8 &= (l8 >> 8) | (l8 << 56);		lr79 = _mm256_or_si256(lr79, _mm256_and_si256(_mm256_cmpeq_epi32(_mm256_and_si256(lm79, m793.v4), m793.v4), m793.v4));
	l8 &= (l8 >> 16) | (l8 << 48);		lr79 = _mm256_and_si256(_mm256_or_si256(lr79, _mm256_shuffle_epi32(lr79, 0xb1)), m790.v4);
	l8 &= (l8 >> 32) | (l8 << 32);		lr79 = _mm256_or_si256(lr79, _mm256_and_si256(_mm256_cmpeq_epi32(_mm256_and_si256(v4_disc, m791.v4), m791.v4), m791.v4));
						l79 = _mm_or_si128(_mm256_extracti128_si256(lr79, 1), _mm256_castsi256_si128(lr79));

#else // Kogge-Stone
>>>>>>> 3e1ed4f (fix cr/lf in repository to lf)
	const __m128i mcpyswap = _mm_set_epi8(0, 1, 2, 3, 4, 5, 6, 7, 7, 6, 5, 4, 3, 2, 1, 0);
	const __m128i mbswapll = _mm_set_epi8(8, 9, 10, 11, 12, 13, 14, 15, 0, 1, 2, 3, 4, 5, 6, 7);
	static const V4DI shiftlr[] = {{{ 9, 7, 7, 9 }}, {{ 18, 14, 14, 18 }}, {{ 36, 28, 28, 36 }}};
	static const V4DI e790 = {{ 0xff80808080808080, 0xff01010101010101, 0xff01010101010101, 0xff80808080808080 }};
	static const V4DI e791 = {{ 0xffffc0c0c0c0c0c0, 0xffff030303030303, 0xffff030303030303, 0xffffc0c0c0c0c0c0 }};
	static const V4DI e792 = {{ 0xfffffffff0f0f0f0, 0xffffffff0f0f0f0f, 0xffffffff0f0f0f0f, 0xfffffffff0f0f0f0 }};

<<<<<<< HEAD
	l81 = _mm_cvtsi64_si128(disc);				v4_disc = _mm256_castsi128_si256(_mm_shuffle_epi8(l81, mcpyswap));
	l81 = _mm_cmpeq_epi8(kff, l81);				v4_disc = _mm256_permute4x64_epi64(v4_disc, 0x50);	// disc, disc, rdisc, rdisc
								lr79 = _mm256_and_si256(v4_disc, _mm256_or_si256(e790.v4, _mm256_srlv_epi64(v4_disc, shiftlr[0].v4)));
	l8 = _mm256_castsi256_si128(v4_disc);			lr79 = _mm256_and_si256(lr79, _mm256_or_si256(e791.v4, _mm256_srlv_epi64(lr79, shiftlr[1].v4)));
	l8 = _mm_and_si128(l8, _mm_alignr_epi8(l8, l8, 1));	lr79 = _mm256_and_si256(lr79, _mm256_or_si256(e792.v4, _mm256_srlv_epi64(lr79, shiftlr[2].v4)));
	l8 = _mm_and_si128(l8, _mm_alignr_epi8(l8, l8, 2));	l79 = _mm_shuffle_epi8(_mm256_extracti128_si256(lr79, 1), mbswapll);
	l8 = _mm_and_si128(l8, _mm_alignr_epi8(l8, l8, 4));	l79 = _mm_and_si128(l79, _mm256_castsi256_si128(lr79));
    #endif
	l81 = _mm_unpacklo_epi64(l81, l8);
	return _mm256_insertf128_si256(_mm256_castsi128_si256(l81), l79, 1);
}

  #elif defined(__ARM_NEON)

void get_full_lines(const unsigned long long disc, unsigned long long full[4])
{
	unsigned long long l8;
	uint8x8_t l01;
	uint64x2_t l79, r79;
	const uint64x2_t e790 = vdupq_n_u64(0x007f7f7f7f7f7f7f);
	const uint64x2_t e791 = vdupq_n_u64(0xfefefefefefefe00);
	const uint64x2_t e792 = vdupq_n_u64(0x00003f3f3f3f3f3f);
	const uint64x2_t e793 = vdupq_n_u64(0x0f0f0f0ff0f0f0f0);

	l01 = vcreate_u8(disc);			l79 = r79 = vreinterpretq_u64_u8(vcombine_u8(l01, vrev64_u8(l01)));
	l01 = vceq_u8(l01, vdup_n_u8(0xff));	l79 = vandq_u64(l79, vornq_u64(vshrq_n_u64(l79, 9), e790));
	full[0] = vget_lane_u64(vreinterpret_u64_u8(l01), 0);
						r79 = vandq_u64(r79, vornq_u64(vshlq_n_u64(r79, 9), e791));
	l8 = disc;				l79 = vbicq_u64(l79, vbicq_u64(e792, vshrq_n_u64(l79, 18)));	// De Morgan
	l8 &= (l8 >> 8) | (l8 << 56);		r79 = vbicq_u64(r79, vshlq_n_u64(vbicq_u64(e792, r79), 18));
	l8 &= (l8 >> 16) | (l8 << 48);		l79 = vandq_u64(vandq_u64(l79, r79), vorrq_u64(e793, vsliq_n_u64(vshrq_n_u64(l79, 36), r79, 36)));
	l8 &= (l8 >> 32) | (l8 << 32);		full[2] = vgetq_lane_u64(l79, 0);
	full[1] = l8;				full[3] = vertical_mirror(vgetq_lane_u64(l79, 1));
}

  #else	// 1 CPU, 3 SSE

void get_full_lines(const unsigned long long disc, unsigned long long full[4])
{
	unsigned long long rdisc = vertical_mirror(disc);
	unsigned long long l8;
	__m128i l01, l79, r79;	// full lines
	const __m128i kff  = _mm_set1_epi8(-1);
	const __m128i e790 = _mm_set1_epi64x(0xff80808080808080);
	const __m128i e791 = _mm_set1_epi64x(0x01010101010101ff);
	const __m128i e792 = _mm_set1_epi64x(0x00003f3f3f3f3f3f);
	const __m128i e793 = _mm_set1_epi64x(0x0f0f0f0ff0f0f0f0);

	l01 = l79 = _mm_cvtsi64_si128(disc);	l79 = r79 = _mm_unpacklo_epi64(l79, _mm_cvtsi64_si128(rdisc));
	l01 = _mm_cmpeq_epi8(kff, l01);		l79 = _mm_and_si128(l79, _mm_or_si128(e790, _mm_srli_epi64(l79, 9)));
	_mm_storel_epi64((__m128i*) &full[0], l01);
						r79 = _mm_and_si128(r79, _mm_or_si128(e791, _mm_slli_epi64(r79, 9)));
	l8 = disc;				l79 = _mm_andnot_si128(_mm_andnot_si128(_mm_srli_epi64(l79, 18), e792), l79);	// De Morgan
	l8 &= (l8 >> 8) | (l8 << 56);		r79 = _mm_andnot_si128(_mm_slli_epi64(_mm_andnot_si128(r79, e792), 18), r79);
	l8 &= (l8 >> 16) | (l8 << 48);		l79 = _mm_and_si128(_mm_and_si128(l79, r79), _mm_or_si128(e793, _mm_or_si128(_mm_srli_epi64(l79, 36), _mm_slli_epi64(r79, 36))));
	l8 &= (l8 >> 32) | (l8 << 32);		_mm_storel_epi64((__m128i *) &full[2], l79);
	full[1] = l8;				full[3] = vertical_mirror(_mm_cvtsi128_si64(_mm_unpackhi_epi64(l79, l79)));
}

  #endif
#endif // hasSSE2/__ARM_NEON

#ifdef __AVX2__
/**
 * @brief AVX2 optimized get_stability
 *
 * @param P bitboard with player's discs.
 * @param O bitboard with opponent's discs.
 * @return the number of stable discs.
 */

// compute the other stable discs (ie discs touching another stable disc in each flipping direction).
static int vectorcall get_spreaded_stability(unsigned long long stable, unsigned long long P_central, __m256i v4_full)
{
	__m128i	v2_stable, v2_old_stable, v2_P_central;
	__m256i	v4_stable;
	const __m256i shift1897 = _mm256_set_epi64x(7, 9, 8, 1);
=======
	l81 = _mm_cvtsi64_si128(disc);		v4_disc = _mm256_castsi128_si256(_mm_shuffle_epi8(l81, mcpyswap));
	l81 = _mm_cmpeq_epi8(kff, l81);		lr79 = _mm256_permute4x64_epi64(v4_disc, 0x50);	// disc, disc, rdisc, rdisc
						lr79 = _mm256_and_si256(lr79, _mm256_or_si256(e790.v4, _mm256_srlv_epi64(lr79, shiftlr[0].v4)));
	l8 = disc;				lr79 = _mm256_and_si256(lr79, _mm256_or_si256(e791.v4, _mm256_srlv_epi64(lr79, shiftlr[1].v4)));
	l8 &= (l8 >> 8) | (l8 << 56);		lr79 = _mm256_and_si256(lr79, _mm256_or_si256(e792.v4, _mm256_srlv_epi64(lr79, shiftlr[2].v4)));
	l8 &= (l8 >> 16) | (l8 << 48);		l79 = _mm_shuffle_epi8(_mm256_extracti128_si256(lr79, 1), mbswapll);
	l8 &= (l8 >> 32) | (l8 << 32);		l79 = _mm_and_si128(l79, _mm256_castsi256_si128(lr79));

#endif
	l81 = _mm_insert_epi64(l81, l8, 1);
	_mm256_storeu_si256((__m256i *) full, _mm256_insertf128_si256(_mm256_castsi128_si256(l81), l79, 1));
	l81 = _mm_and_si128(l81, l79);
	full[4] = _mm_cvtsi128_si64(_mm_and_si128(l81, _mm_shuffle_epi32(l81, 0x4e)));
}

int get_stability_fulls_given(const unsigned long long P, const unsigned long long O, const unsigned long long full[5])
{
	unsigned long long stable, P_central;
	__m128i	v2_stable, v2_old_stable, v2_P_central;
	__m256i	v4_stable, v4_full;
	const __m256i shift1897 = _mm256_set_epi64x(7, 9, 8, 1);

<<<<<<< HEAD
<<<<<<< HEAD
	// add full lines
	v2_stable = _mm_and_si128(l81, l79);
	stable |= _mm_cvtsi128_si64(_mm_and_si128(v2_stable, _mm_unpackhi_epi64(v2_stable, v2_stable))) & P_central;
>>>>>>> 3e1ed4f (fix cr/lf in repository to lf)
=======
	// compute the exact stable edges (from precomputed tables) and add full lines
<<<<<<< HEAD
	stable = get_stable_edge_sse(P, O) | (get_all_full_lines(disc, &full) & P_central);
>>>>>>> 9e2bbc5 (split get_all_full_lines from get_stability)
=======
	stable = get_stable_edge_sse(P, O) | (get_all_full_lines(P | O, &full) & P_central);
>>>>>>> 6c3ed52 (Dogaishi hash reduction by Matsuo & Narazaki; edge-precise get_full_line)
=======
	// compute the exact stable edges (from precomputed tables)
	stable = get_stable_edge(P, O);

	// add full lines
	P_central = (P & 0x007e7e7e7e7e7e00);
<<<<<<< HEAD
	stable |= (allfull & P_central);
>>>>>>> 21f8809 (Share all full lines between get_stability and Dogaishi hash reduction)
=======
	stable |= (full[4] & P_central);
>>>>>>> 4303b09 (Returns all full lines in full[4])

	if (stable == 0)
		return 0;

<<<<<<< HEAD
=======
	// now compute the other stable discs (ie discs touching another stable disc in each flipping direction).
<<<<<<< HEAD
	v4_full = _mm256_insertf128_si256(_mm256_castsi128_si256(l81), l79, 1);
>>>>>>> 3e1ed4f (fix cr/lf in repository to lf)
=======
>>>>>>> 9e2bbc5 (split get_all_full_lines from get_stability)
	v2_stable = _mm_cvtsi64_si128(stable);
	v2_P_central = _mm_cvtsi64_si128(P_central);
	v4_full = _mm256_loadu_si256((__m256i *) full);
	do {
		v2_old_stable = v2_stable;
		v4_stable = _mm256_broadcastq_epi64(v2_stable);
		v4_stable = _mm256_or_si256(_mm256_or_si256(_mm256_srlv_epi64(v4_stable, shift1897), _mm256_sllv_epi64(v4_stable, shift1897)), v4_full);
		v2_stable = _mm_and_si128(_mm256_castsi256_si128(v4_stable), _mm256_extracti128_si256(v4_stable, 1));
		v2_stable = _mm_and_si128(v2_stable, _mm_unpackhi_epi64(v2_stable, v2_stable));
		v2_stable = _mm_or_si128(v2_old_stable, _mm_and_si128(v2_stable, v2_P_central));
	} while (!_mm_testc_si128(v2_old_stable, v2_stable));

	return bit_count(_mm_cvtsi128_si64(v2_stable));
}
<<<<<<< HEAD
#elif defined(hasSSE2) && !defined(HAS_CPU_64)
// 32bit SSE optimized get_spreaded_stability
int get_spreaded_stability(unsigned long long stable, unsigned long long P_central, unsigned long long full[4])
{
	__m128i v_stable, stable_vh, stable_d79, old_stable;

	if (stable == 0)	// (2%)
		return 0;

	v_stable = _mm_cvtsi64_si128(stable);
	do {
		old_stable = v_stable;
		stable_vh = _mm_loadu_si128((__m128i *) &full[0]);
		stable_vh = _mm_or_si128(stable_vh, _mm_unpacklo_epi64(_mm_srli_epi64(v_stable, 1), _mm_srli_epi64(v_stable, 8)));
		stable_vh = _mm_or_si128(stable_vh, _mm_unpacklo_epi64(_mm_slli_epi64(v_stable, 1), _mm_slli_epi64(v_stable, 8)));
		stable_d79 = _mm_loadu_si128((__m128i *) &full[2]);
		stable_d79 = _mm_or_si128(stable_d79, _mm_unpacklo_epi64(_mm_srli_epi64(v_stable, 9), _mm_srli_epi64(v_stable, 7)));
		stable_d79 = _mm_or_si128(stable_d79, _mm_unpacklo_epi64(_mm_slli_epi64(v_stable, 9), _mm_slli_epi64(v_stable, 7)));
		v_stable = _mm_and_si128(stable_vh, stable_d79);
		v_stable = _mm_and_si128(v_stable, _mm_unpackhi_epi64(v_stable, v_stable));
		v_stable = _mm_or_si128(old_stable, _mm_and_si128(v_stable, _mm_loadl_epi64((__m128i *) &P_central)));
	} while (_mm_movemask_epi8(_mm_cmpeq_epi8(v_stable, old_stable)) != 0xffff);	// (44%)

	return bit_count_si64(v_stable);
}
#endif

#ifdef __AVX2__
// returns stability count only
int get_stability(const unsigned long long P, const unsigned long long O)
{
	unsigned long long stable = get_stable_edge(P, O);	// compute the exact stable edges
	unsigned long long P_central = P & 0x007e7e7e7e7e7e00;

	__m256i	v4_full = get_full_lines(P | O);	// add full lines
	__m128i v2_full = _mm_and_si128(_mm256_castsi256_si128(v4_full), _mm256_extracti128_si256(v4_full, 1));
	stable |= (P_central & _mm_cvtsi128_si64(_mm_and_si128(v2_full, _mm_unpackhi_epi64(v2_full, v2_full))));

	return get_spreaded_stability(stable, P_central, v4_full);	// compute the other stable discs
}

// returns all full in full[4] in addition to stability count
int get_stability_fulls(const unsigned long long P, const unsigned long long O, unsigned long long full[5])
{
	unsigned long long stable = get_stable_edge(P, O);	// compute the exact stable edges
	unsigned long long P_central = P & 0x007e7e7e7e7e7e00;

	__m256i	v4_full = get_full_lines(P | O);	// add full lines
	__m128i v2_full = _mm_and_si128(_mm256_castsi256_si128(v4_full), _mm256_extracti128_si256(v4_full, 1));
	// _mm256_storeu_si256((__m256i *) full, v4_full);
	full[4] = _mm_cvtsi128_si64(_mm_and_si128(v2_full, _mm_unpackhi_epi64(v2_full, v2_full)));
	stable |= (P_central & full[4]);

	return get_spreaded_stability(stable, P_central, v4_full);	// compute the other stable discs
}

// returns all full lines only
unsigned long long get_all_full_lines(const unsigned long long disc)
{
	__m256i v4_full = get_full_lines(disc);
	__m128i v2_full = _mm_and_si128(_mm256_castsi256_si128(v4_full), _mm256_extracti128_si256(v4_full, 1));
	return _mm_cvtsi128_si64(_mm_and_si128(v2_full, _mm_unpackhi_epi64(v2_full, v2_full)));
}

/**
 * @brief AVX2 optimized get_moves + get_potential_moves.
 *
 * Get the bitboard of empty squares in contact of a player square, as well as real mobility.
 *
 * @param PP broadcasted bitboard with player's discs.
 * @param OO broadcasted bitboard with opponent's discs.
 * @return potential moves in a higner 64-bit, real moves in a lower 64-bit.
 */
__m128i vectorcall get_moves_and_potential(__m256i PP, __m256i OO)
{
	__m256i	MM, potmob, flip_l, flip_r, pre_l, pre_r, shift2;
	const __m256i shift1897 = _mm256_set_epi64x(7, 9, 8, 1);
	__m256i	mOO = _mm256_and_si256(OO, _mm256_set_epi64x(0x007E7E7E7E7E7E00, 0x007E7E7E7E7E7E00, 0x00FFFFFFFFFFFF00, 0x7E7E7E7E7E7E7E7E));
	__m128i occupied = _mm_or_si128(_mm256_castsi256_si128(PP), _mm256_castsi256_si128(OO));

	flip_l = _mm256_and_si256(mOO, _mm256_sllv_epi64(PP, shift1897));
	flip_r = _mm256_and_si256(mOO, _mm256_srlv_epi64(PP, shift1897));
	flip_l = _mm256_or_si256(flip_l, _mm256_and_si256(mOO, _mm256_sllv_epi64(flip_l, shift1897)));
	flip_r = _mm256_or_si256(flip_r, _mm256_and_si256(mOO, _mm256_srlv_epi64(flip_r, shift1897)));
	pre_l = _mm256_sllv_epi64(mOO, shift1897);	pre_r = _mm256_srlv_epi64(mOO, shift1897);
	potmob = _mm256_or_si256(pre_l, pre_r);
	pre_l = _mm256_and_si256(mOO, pre_l);		pre_r = _mm256_and_si256(mOO, pre_r);
	shift2 = _mm256_add_epi64(shift1897, shift1897);
	flip_l = _mm256_or_si256(flip_l, _mm256_and_si256(pre_l, _mm256_sllv_epi64(flip_l, shift2)));
	flip_r = _mm256_or_si256(flip_r, _mm256_and_si256(pre_r, _mm256_srlv_epi64(flip_r, shift2)));
	flip_l = _mm256_or_si256(flip_l, _mm256_and_si256(pre_l, _mm256_sllv_epi64(flip_l, shift2)));
	flip_r = _mm256_or_si256(flip_r, _mm256_and_si256(pre_r, _mm256_srlv_epi64(flip_r, shift2)));
	MM = _mm256_or_si256(_mm256_sllv_epi64(flip_l, shift1897), _mm256_srlv_epi64(flip_r, shift1897));

	MM = _mm256_or_si256(_mm256_unpacklo_epi64(MM, potmob), _mm256_unpackhi_epi64(MM, potmob));
	return _mm_andnot_si128(occupied, _mm_or_si128(_mm256_castsi256_si128(MM), _mm256_extracti128_si256(MM, 1)));	// mask with empties
}

#endif
=======
/**
 * @file board_sse.c
 *
 * SSE/AVX translation of some board.c functions
 *
 * @date 2014 - 2020
 * @author Toshihiko Okuhara
 * @version 4.4
 */

#include "bit.h"
#include "hash.h"
#include "board.h"

/**
 * @brief SSE2 translation of board_symetry
 *
 * @param board input board
 * @param s symetry
 * @param sym symetric output board
 */
#ifdef hasSSE2

void board_symetry(const Board *board, const int s, Board *sym)
{
	__m128i	bb = _mm_loadu_si128((__m128i *) board);
	__m128i	tt;
	const __m128i mask0F0F = _mm_set1_epi16(0x0F0F);
	const __m128i mask00AA = _mm_set1_epi16(0x00AA);
	const __m128i maskCCCC = _mm_set1_epi32(0x0000CCCC);
	const __m128i mask00F0 = _mm_set1_epi64x(0x00000000F0F0F0F0);
#if defined(__SSSE3__) || defined(__AVX__)	// pshufb
	const __m128i mbswapll = _mm_set_epi8(8, 9, 10, 11, 12, 13, 14, 15, 0, 1, 2, 3, 4, 5, 6, 7);
	const __m128i mbitrev  = _mm_set_epi8(15, 7, 11, 3, 13, 5, 9, 1, 14, 6, 10, 2, 12, 4, 8, 0);

	if (s & 1) {	// horizontal_mirror (cf. http://wm.ite.pl/articles/sse-popcount.html)
		bb = _mm_or_si128(_mm_shuffle_epi8(mbitrev, _mm_and_si128(_mm_srli_epi64(bb, 4), mask0F0F)),
			_mm_slli_epi64(_mm_shuffle_epi8(mbitrev, _mm_and_si128(bb, mask0F0F)), 4));
	}

	if (s & 2) {	// vertical_mirror
		bb = _mm_shuffle_epi8(bb, mbswapll);
	}

#else
	const __m128i mask5555 = _mm_set1_epi16(0x5555);
	const __m128i mask3333 = _mm_set1_epi16(0x3333);

	if (s & 1) {	// horizontal_mirror
		bb = _mm_or_si128(_mm_and_si128(_mm_srli_epi64(bb, 1), mask5555), _mm_slli_epi64(_mm_and_si128(bb, mask5555), 1));
		bb = _mm_or_si128(_mm_and_si128(_mm_srli_epi64(bb, 2), mask3333), _mm_slli_epi64(_mm_and_si128(bb, mask3333), 2));
		bb = _mm_or_si128(_mm_and_si128(_mm_srli_epi64(bb, 4), mask0F0F), _mm_slli_epi64(_mm_and_si128(bb, mask0F0F), 4));
	}

	if (s & 2) {	// vertical_mirror
		bb = _mm_or_si128(_mm_srli_epi16(bb, 8), _mm_slli_epi16(bb, 8));
		bb = _mm_shufflehi_epi16(_mm_shufflelo_epi16(bb, 0x1b), 0x1b);
	}
#endif

	if (s & 4) {	// transpose
		tt = _mm_and_si128(_mm_xor_si128(bb, _mm_srli_epi64(bb, 7)), mask00AA);
		bb = _mm_xor_si128(_mm_xor_si128(bb, tt), _mm_slli_epi64(tt, 7));
		tt = _mm_and_si128(_mm_xor_si128(bb, _mm_srli_epi64(bb, 14)), maskCCCC);
		bb = _mm_xor_si128(_mm_xor_si128(bb, tt), _mm_slli_epi64(tt, 14));
		tt = _mm_and_si128(_mm_xor_si128(bb, _mm_srli_epi64(bb, 28)), mask00F0);
		bb = _mm_xor_si128(_mm_xor_si128(bb, tt), _mm_slli_epi64(tt, 28));
	}

#ifdef __clang__
	sym->player = bb[0];
	sym->opponent = bb[1];
#else	// error on clang 3.8
	_mm_storeu_si128((__m128i *) sym, bb);
#endif

	board_check(sym);
}

#if (MOVE_GENERATOR == MOVE_GENERATOR_AVX) || (MOVE_GENERATOR == MOVE_GENERATOR_SSE)
/**
 * @brief Compute a board resulting of a move played on a previous board.
 *
 * @param board board to play the move on.
 * @param x move to play.
 * @param next resulting board.
 * @return flipped discs.
 */
unsigned long long board_next(const Board *board, const int x, Board *next)
{
	__m128i OP = _mm_loadu_si128((__m128i *) board);
	__m128i flipped = mm_Flip(OP, x);

	OP = _mm_xor_si128(OP, _mm_or_si128(flipped, _mm_loadl_epi64((__m128i *) &X_TO_BIT[x])));
	_mm_storeu_si128((__m128i *) next, _mm_shuffle_epi32(OP, 0x4e));

	return _mm_cvtsi128_si64(flipped);
}

/**
 * @brief Compute a board resulting of an opponent move played on a previous board.
 *
 * Compute the board after passing and playing a move.
 *
 * @param board board to play the move on.
 * @param x opponent move to play.
 * @param next resulting board.
 * @return flipped discs.
 */
unsigned long long board_pass_next(const Board *board, const int x, Board *next)
{
	__m128i	PO = _mm_shuffle_epi32(_mm_loadu_si128((__m128i *) board), 0x4e);
	__m128i flipped = mm_Flip(PO, x);

	PO = _mm_xor_si128(PO, _mm_or_si128(flipped, _mm_loadl_epi64((__m128i *) &X_TO_BIT[x])));
	_mm_storeu_si128((__m128i *) next, _mm_shuffle_epi32(PO, 0x4e));

	return _mm_cvtsi128_si64(flipped);
}
#endif

#endif // hasSSE2

/**
 * @brief X64 optimized get_moves
 *
 * Diag-7 is converted to diag-9 (v.v.) using vertical mirroring
 * in SSE versions.
 *
 * @param P bitboard with player's discs.
 * @param O bitboard with opponent's discs.
 * @return all legal moves in a 64-bit unsigned integer.
 */
#ifdef __AVX2__	// 4 AVX

unsigned long long get_moves(const unsigned long long P, const unsigned long long O)
{
	__m256i	PP, mOO, MM, flip_l, flip_r, pre_l, pre_r, shift2;
	__m128i	M;
	const __m256i shift1897 = _mm256_set_epi64x(7, 9, 8, 1);
	const __m256i mflipH = _mm256_set_epi64x(0x7e7e7e7e7e7e7e7e, 0x7e7e7e7e7e7e7e7e, -1, 0x7e7e7e7e7e7e7e7e);

	PP = _mm256_broadcastq_epi64(_mm_cvtsi64_si128(P));
	mOO = _mm256_and_si256(_mm256_broadcastq_epi64(_mm_cvtsi64_si128(O)), mflipH);

	flip_l = _mm256_and_si256(mOO, _mm256_sllv_epi64(PP, shift1897));
	flip_r = _mm256_and_si256(mOO, _mm256_srlv_epi64(PP, shift1897));
	flip_l = _mm256_or_si256(flip_l, _mm256_and_si256(mOO, _mm256_sllv_epi64(flip_l, shift1897)));
	flip_r = _mm256_or_si256(flip_r, _mm256_and_si256(mOO, _mm256_srlv_epi64(flip_r, shift1897)));
	pre_l = _mm256_and_si256(mOO, _mm256_sllv_epi64(mOO, shift1897));
	pre_r = _mm256_srlv_epi64(pre_l, shift1897);
	shift2 = _mm256_add_epi64(shift1897, shift1897);
	flip_l = _mm256_or_si256(flip_l, _mm256_and_si256(pre_l, _mm256_sllv_epi64(flip_l, shift2)));
	flip_r = _mm256_or_si256(flip_r, _mm256_and_si256(pre_r, _mm256_srlv_epi64(flip_r, shift2)));
	flip_l = _mm256_or_si256(flip_l, _mm256_and_si256(pre_l, _mm256_sllv_epi64(flip_l, shift2)));
	flip_r = _mm256_or_si256(flip_r, _mm256_and_si256(pre_r, _mm256_srlv_epi64(flip_r, shift2)));
	MM = _mm256_sllv_epi64(flip_l, shift1897);
	MM = _mm256_or_si256(MM, _mm256_srlv_epi64(flip_r, shift1897));

	M = _mm_or_si128(_mm256_castsi256_si128(MM), _mm256_extracti128_si256(MM, 1));
	M = _mm_or_si128(M, _mm_unpackhi_epi64(M, M));
	return _mm_cvtsi128_si64(M) & ~(P|O);	// mask with empties
}

#elif defined(__x86_64__) || defined(_M_X64)	// 2 SSE, 2 CPU

unsigned long long get_moves(const unsigned long long P, const unsigned long long O)
{
	unsigned long long moves, mO, flip1, pre1, flip8, pre8;
	__m128i	PP, mOO, MM, flip, pre;

	mO = O & 0x7e7e7e7e7e7e7e7eULL;
	PP  = _mm_set_epi64x(vertical_mirror(P), P);
	mOO = _mm_set_epi64x(vertical_mirror(mO), mO);
		/* shift=-9:+7 */								/* shift=+1 */			/* shift = +8 */
	flip = _mm_and_si128(mOO, _mm_slli_epi64(PP, 7));				flip1  = mO & (P << 1);		flip8  = O & (P << 8);
	flip = _mm_or_si128(flip, _mm_and_si128(mOO, _mm_slli_epi64(flip, 7)));		flip1 |= mO & (flip1 << 1);	flip8 |= O & (flip8 << 8);
	pre  = _mm_and_si128(mOO, _mm_slli_epi64(mOO, 7));				pre1   = mO & (mO << 1);	pre8   = O & (O << 8);
	flip = _mm_or_si128(flip, _mm_and_si128(pre, _mm_slli_epi64(flip, 14)));	flip1 |= pre1 & (flip1 << 2);	flip8 |= pre8 & (flip8 << 16);
	flip = _mm_or_si128(flip, _mm_and_si128(pre, _mm_slli_epi64(flip, 14)));	flip1 |= pre1 & (flip1 << 2);	flip8 |= pre8 & (flip8 << 16);
	MM = _mm_slli_epi64(flip, 7);							moves = flip1 << 1;		moves |= flip8 << 8;
		/* shift=-7:+9 */								/* shift=-1 */			/* shift = -8 */
	flip = _mm_and_si128(mOO, _mm_slli_epi64(PP, 9));				flip1  = mO & (P >> 1);		flip8  = O & (P >> 8);
	flip = _mm_or_si128(flip, _mm_and_si128(mOO, _mm_slli_epi64(flip, 9)));		flip1 |= mO & (flip1 >> 1);	flip8 |= O & (flip8 >> 8);
	pre = _mm_and_si128(mOO, _mm_slli_epi64(mOO, 9));				pre1 >>= 1;			pre8 >>= 8;
	flip = _mm_or_si128(flip, _mm_and_si128(pre, _mm_slli_epi64(flip, 18)));	flip1 |= pre1 & (flip1 >> 2);	flip8 |= pre8 & (flip8 >> 16);
	flip = _mm_or_si128(flip, _mm_and_si128(pre, _mm_slli_epi64(flip, 18)));	flip1 |= pre1 & (flip1 >> 2);	flip8 |= pre8 & (flip8 >> 16);
	MM = _mm_or_si128(MM, _mm_slli_epi64(flip, 9));					moves |= flip1 >> 1;		moves |= flip8 >> 8;

	moves |= _mm_cvtsi128_si64(MM) | vertical_mirror(_mm_cvtsi128_si64(_mm_unpackhi_epi64(MM, MM)));
	return moves & ~(P|O);	// mask with empties
}

#elif 0	// 4 CPU

unsigned long long get_moves(const unsigned long long P, const unsigned long long O)
{
	unsigned long long moves, mO;
	unsigned long long flip1, flip7, flip9, flip8, pre1, pre7, pre9, pre8;

	mO = O & 0x7e7e7e7e7e7e7e7eULL;
	flip1  = mO & (P << 1);		flip7  = mO & (P << 7);		flip9  = mO & (P << 9);		flip8  = O & (P << 8);
	flip1 |= mO & (flip1 << 1);	flip7 |= mO & (flip7 << 7);	flip9 |= mO & (flip9 << 9);	flip8 |= O & (flip8 << 8);
	pre1 = mO & (mO << 1);		pre7 = mO & (mO << 7);		pre9 = mO & (mO << 9);		pre8 = O & (O << 8);
	flip1 |= pre1 & (flip1 << 2);	flip7 |= pre7 & (flip7 << 14);	flip9 |= pre9 & (flip9 << 18);	flip8 |= pre8 & (flip8 << 16);
	flip1 |= pre1 & (flip1 << 2);	flip7 |= pre7 & (flip7 << 14);	flip9 |= pre9 & (flip9 << 18);	flip8 |= pre8 & (flip8 << 16);
	moves = flip1 << 1;		moves |= flip7 << 7;		moves |= flip9 << 9;		moves |= flip8 << 8;
	flip1  = mO & (P >> 1);		flip7  = mO & (P >> 7);		flip9  = mO & (P >> 9);		flip8  = O & (P >> 8);
	flip1 |= mO & (flip1 >> 1);	flip7 |= mO & (flip7 >> 7);	flip9 |= mO & (flip9 >> 9);	flip8 |= O & (flip8 >> 8);
	pre1 >>= 1;			pre7 >>= 7;			pre9 >>= 9;			pre8 >>= 8;
	flip1 |= pre1 & (flip1 >> 2);	flip7 |= pre7 & (flip7 >> 14);	flip9 |= pre9 & (flip9 >> 18);	flip8 |= pre8 & (flip8 >> 16);
	flip1 |= pre1 & (flip1 >> 2);	flip7 |= pre7 & (flip7 >> 14);	flip9 |= pre9 & (flip9 >> 18);	flip8 |= pre8 & (flip8 >> 16);
	moves |= flip1 >> 1;		moves |= flip7 >> 7;		moves |= flip9 >> 9;		moves |= flip8 >> 8;

	return moves & ~(P|O);	// mask with empties
}

#else // __x86_64__
/**
 * @brief SSE optimized get_moves for x86 (3 SSE, 1 CPU)
 *
 */
#if defined(hasSSE2) || defined(USE_MSVC_X86)

unsigned long long get_moves_sse(unsigned long long P, unsigned long long O)
{
	unsigned int	mO, movesL, movesH, flip1, pre1;
	__m128i	OP, rOP, PP, OO, MM, flip, pre;
	const __m128i mask7e = _mm_set1_epi8(0x7e);

		// vertical_mirror in PP[1], OO[1]
	OP  = _mm_unpacklo_epi64(_mm_cvtsi64_si128(P), _mm_cvtsi64_si128(O));		mO = (unsigned int) O & 0x7e7e7e7eU;
	rOP = _mm_shufflelo_epi16(OP, 0x1B);						flip1  = mO & ((unsigned int) P << 1);
	rOP = _mm_shufflehi_epi16(rOP, 0x1B);						flip1 |= mO & (flip1 << 1);
											pre1   = mO & (mO << 1);
	rOP = _mm_or_si128(_mm_srli_epi16(rOP, 8), _mm_slli_epi16(rOP, 8));
	    										flip1 |= pre1 & (flip1 << 2);
	PP  = _mm_unpacklo_epi64(OP, rOP);						flip1 |= pre1 & (flip1 << 2);
	OO  = _mm_unpackhi_epi64(OP, rOP);						movesL = flip1 << 1;

	flip = _mm_and_si128(OO, _mm_slli_epi64(PP, 8));				flip1  = mO & ((unsigned int) P >> 1);
	flip = _mm_or_si128(flip, _mm_and_si128(OO, _mm_slli_epi64(flip, 8)));		flip1 |= mO & (flip1 >> 1);
	pre = _mm_and_si128(OO, _mm_slli_epi64(OO, 8));					pre1 >>= 1;
	flip = _mm_or_si128(flip, _mm_and_si128(pre, _mm_slli_epi64(flip, 16)));	flip1 |= pre1 & (flip1 >> 2);
	flip = _mm_or_si128(flip, _mm_and_si128(pre, _mm_slli_epi64(flip, 16)));	flip1 |= pre1 & (flip1 >> 2);
	MM = _mm_slli_epi64(flip, 8);							movesL |= flip1 >> 1;

	OO = _mm_and_si128(OO, mask7e);							mO = (unsigned int) (O >> 32) & 0x7e7e7e7eU;
	flip = _mm_and_si128(OO, _mm_slli_epi64(PP, 7));				flip1  = mO & ((unsigned int) (P >> 32) << 1);
	flip = _mm_or_si128(flip, _mm_and_si128(OO, _mm_slli_epi64(flip, 7)));		flip1 |= mO & (flip1 << 1);
	pre = _mm_and_si128(OO, _mm_slli_epi64(OO, 7));					pre1   = mO & (mO << 1);
	flip = _mm_or_si128(flip, _mm_and_si128(pre, _mm_slli_epi64(flip, 14)));	flip1 |= pre1 & (flip1 << 2);
	flip = _mm_or_si128(flip, _mm_and_si128(pre, _mm_slli_epi64(flip, 14)));	flip1 |= pre1 & (flip1 << 2);
	MM = _mm_or_si128(MM, _mm_slli_epi64(flip, 7));					movesH = flip1 << 1;

	flip = _mm_and_si128(OO, _mm_slli_epi64(PP, 9));				flip1  = mO & ((unsigned int) (P >> 32) >> 1);
	flip = _mm_or_si128(flip, _mm_and_si128(OO, _mm_slli_epi64(flip, 9)));		flip1 |= mO & (flip1 >> 1);
	pre = _mm_and_si128(OO, _mm_slli_epi64(OO, 9));					pre1 >>= 1;
	flip = _mm_or_si128(flip, _mm_and_si128(pre, _mm_slli_epi64(flip, 18)));	flip1 |= pre1 & (flip1 >> 2);
	flip = _mm_or_si128(flip, _mm_and_si128(pre, _mm_slli_epi64(flip, 18)));	flip1 |= pre1 & (flip1 >> 2);
	MM = _mm_or_si128(MM, _mm_slli_epi64(flip, 9));					movesH |= flip1 >> 1;

	movesL |= _mm_cvtsi128_si32(MM);	MM = _mm_srli_si128(MM, 4);
	movesH |= _mm_cvtsi128_si32(MM);	MM = _mm_srli_si128(MM, 4);
	movesH |= bswap_int(_mm_cvtsi128_si32(MM));
	movesL |= bswap_int(_mm_cvtsi128_si32(_mm_srli_si128(MM, 4)));
	return (movesL | ((unsigned long long) movesH << 32)) & ~(P|O);	// mask with empties
}

#else // non-VEX asm

unsigned long long get_moves_sse(unsigned long long P, unsigned long long O)
{
	unsigned long long moves;
	static const V2DI mask7e = {{ 0x7e7e7e7e7e7e7e7eULL, 0x7e7e7e7e7e7e7e7eULL }};

	__asm__ (
		"movl	%1, %%ebx\n\t"
		"movl	%3, %%edi\n\t"
		"andl	$0x7e7e7e7e, %%edi\n\t"
				/* shift=-1 */			/* vertical mirror in PP[1], OO[1] */
		"movl	%%ebx, %%eax\n\t"	"movd	%1, %%xmm4\n\t"		// (movd for store-forwarding)
		"shrl	$1, %%eax\n\t"		"movd	%2, %%xmm0\n\t"
		"andl	%%edi, %%eax\n\t"	"movd	%3, %%xmm5\n\t"
		"movl	%%eax, %%edx\n\t"	"movd	%4, %%xmm1\n\t"
		"shrl	$1, %%eax\n\t"		"punpckldq %%xmm0, %%xmm4\n\t"		// P
		"movl	%%edi, %%ecx\n\t"	"punpckldq %%xmm1, %%xmm5\n\t"		// O
		"andl	%%edi, %%eax\n\t"	"punpcklqdq %%xmm5, %%xmm4\n\t"		// OP
		"shrl	$1, %%ecx\n\t"		"pshuflw $0x1b, %%xmm4, %%xmm0\n\t"
		"orl	%%edx, %%eax\n\t"	"pshufhw $0x1b, %%xmm0, %%xmm0\n\t"
		"andl	%%edi, %%ecx\n\t"	"movdqa	%%xmm0, %%xmm1\n\t"
		"movl	%%eax, %%edx\n\t"	"psllw	$8, %%xmm0\n\t"
		"shrl	$2, %%eax\n\t"		"psrlw	$8, %%xmm1\n\t"
		"andl	%%ecx, %%eax\n\t"	"por	%%xmm1, %%xmm0\n\t"		// rOP
		"orl	%%eax, %%edx\n\t"
		"shrl	$2, %%eax\n\t"		"movdqa	%%xmm4, %%xmm5\n\t"
		"andl	%%ecx, %%eax\n\t"	"punpcklqdq %%xmm0, %%xmm4\n\t"		// PP
		"orl	%%edx, %%eax\n\t"	"punpckhqdq %%xmm0, %%xmm5\n\t"		// OO
		"shrl	$1, %%eax\n\t"
				/* shift=+1 */			/* shift=-8:+8 */
						"movdqa	%%xmm4, %%xmm0\n\t"
		"addl	%%ebx, %%ebx\n\t"	"psllq	$8, %%xmm0\n\t"
		"andl	%%edi, %%ebx\n\t"	"pand	%%xmm5, %%xmm0\n\t"	// 0 m7&o6 m6&o5 .. m1&o0
		"movl	%%ebx, %%edx\n\t"	"movdqa	%%xmm0, %%xmm1\n\t"
		"addl	%%ebx, %%ebx\n\t"	"psllq	$8, %%xmm0\n\t"
						"movdqa	%%xmm5, %%xmm3\n\t"
		"andl	%%edi, %%ebx\n\t"	"pand	%%xmm5, %%xmm0\n\t"	// 0 0 m7&o6&o5 .. m2&o1&o0
						"psllq	$8, %%xmm3\n\t"
		"orl	%%ebx, %%edx\n\t"	"por	%%xmm1, %%xmm0\n\t"	// 0 m7&o6 (m6&o5)|(m7&o6&o5) .. (m1&o0)
		"addl	%%ecx, %%ecx\n\t"	"pand	%%xmm5, %%xmm3\n\t"	// 0 o7&o6 o6&o5 o5&o4 o4&o3 ..
						"movdqa	%%xmm0, %%xmm2\n\t"
		"leal	(,%%edx,4), %%ebx\n\t"	"psllq	$16, %%xmm0\n\t"
		"andl	%%ecx, %%ebx\n\t"	"pand	%%xmm3, %%xmm0\n\t"	// 0 0 0 m7&o6&o5&o4 (m6&o5&o4&o3)|(m7&o6&o5&o4&o3) ..
		"orl	%%ebx, %%edx\n\t"	"por	%%xmm0, %%xmm2\n\t"
		"shll	$2, %%ebx\n\t"		"psllq	$16, %%xmm0\n\t"
		"andl	%%ecx, %%ebx\n\t"	"pand	%%xmm3, %%xmm0\n\t"	// 0 0 0 0 0 m7&o6&..&o2 (m6&o5&..&o1)|(m7&o6&..&o1) ..
		"orl	%%edx, %%ebx\n\t"	"por	%%xmm0, %%xmm2\n\t"
		"addl	%%ebx, %%ebx\n\t"	"psllq	$8, %%xmm2\n\t"
		"orl	%%eax, %%ebx\n\t"

		"movl	%2, %%esi\n\t"
		"movl	%4, %%edi\n\t"
				/* shift=-1 */			/* shift=-9:+7 */
		"andl	$0x7e7e7e7e,%%edi\n\t"	"pand	%5, %%xmm5\n\t"
		"movl	%%esi, %%eax\n\t"	"movdqa	%%xmm4, %%xmm0\n\t"
		"shrl	$1, %%eax\n\t"		"psllq	$7, %%xmm0\n\t"
		"andl	%%edi, %%eax\n\t"	"pand	%%xmm5, %%xmm0\n\t"
		"movl	%%eax, %%edx\n\t"	"movdqa	%%xmm0, %%xmm1\n\t"
		"shrl	$1, %%eax\n\t"		"psllq	$7, %%xmm0\n\t"
		"andl	%%edi, %%eax\n\t"	"pand	%%xmm5, %%xmm0\n\t"
		"movl	%%edi, %%ecx\n\t"	"movdqa	%%xmm5, %%xmm3\n\t"
		"orl	%%edx, %%eax\n\t"	"por	%%xmm1, %%xmm0\n\t"
		"shrl	$1, %%ecx\n\t"		"psllq	$7, %%xmm3\n\t"
		"movl	%%eax, %%edx\n\t"	"movdqa	%%xmm0, %%xmm1\n\t"
		"andl	%%edi, %%ecx\n\t"	"pand	%%xmm5, %%xmm3\n\t"
		"shrl	$2, %%eax\n\t"		"psllq	$14, %%xmm0\n\t"
		"andl	%%ecx, %%eax\n\t"	"pand	%%xmm3, %%xmm0\n\t"
		"orl	%%eax, %%edx\n\t"	"por	%%xmm0, %%xmm1\n\t"
		"shrl	$2, %%eax\n\t"		"psllq	$14, %%xmm0\n\t"
		"andl	%%ecx, %%eax\n\t"	"pand	%%xmm3, %%xmm0\n\t"
		"orl	%%edx, %%eax\n\t"	"por	%%xmm1, %%xmm0\n\t"
		"shrl	$1, %%eax\n\t"		"psllq	$7, %%xmm0\n\t"
						"por	%%xmm0, %%xmm2\n\t"
				/* shift=+1 */			/* shift=-7:+9 */
						"movdqa	%%xmm4, %%xmm0\n\t"
		"addl	%%esi, %%esi\n\t"	"psllq	$9, %%xmm0\n\t"
		"andl	%%edi, %%esi\n\t"	"pand	%%xmm5, %%xmm0\n\t"
		"movl	%%esi, %%edx\n\t"	"movdqa	%%xmm0, %%xmm1\n\t"
		"addl	%%esi, %%esi\n\t"	"psllq	$9, %%xmm0\n\t"
		"andl	%%edi, %%esi\n\t"	"pand	%%xmm5, %%xmm0\n\t"
						"movdqa	%%xmm5, %%xmm3\n\t"
		"orl	%%esi, %%edx\n\t"	"por	%%xmm1, %%xmm0\n\t"
						"psllq	$9, %%xmm3\n\t"
						"movdqa	%%xmm0, %%xmm1\n\t"
		"addl	%%ecx, %%ecx\n\t"	"pand	%%xmm5, %%xmm3\n\t"
		"leal	(,%%edx,4), %%esi\n\t"	"psllq	$18, %%xmm0\n\t"
		"andl	%%ecx, %%esi\n\t"	"pand	%%xmm3, %%xmm0\n\t"
		"orl	%%esi, %%edx\n\t"	"por	%%xmm0, %%xmm1\n\t"
		"shll	$2, %%esi\n\t"		"psllq	$18, %%xmm0\n\t"
		"andl	%%ecx, %%esi\n\t"	"pand	%%xmm3, %%xmm0\n\t"
		"orl	%%edx, %%esi\n\t"	"por	%%xmm1, %%xmm0\n\t"
		"addl	%%esi, %%esi\n\t"	"psllq	$9, %%xmm0\n\t"
		"orl	%%eax, %%esi\n\t"	"por	%%xmm0, %%xmm2\n\t"

		"movl	%1, %%eax\n\t"		"movhlps %%xmm2, %%xmm3\n\t"
		"movl	%2, %%edx\n\t"		"movd	%%xmm3, %%edi\n\t"	"movd	%%xmm2, %%ecx\n\t"
						"psrlq	$32, %%xmm3\n\t"	"psrlq	$32, %%xmm2\n\t"
						"bswapl	%%edi\n\t"		"orl	%%ecx, %%ebx\n\t"
		"orl	%3, %%eax\n\t"		"orl	%%edi, %%esi\n\t"
		"orl	%4, %%edx\n\t"		"movd	%%xmm3, %%edi\n\t"	"movd	%%xmm2, %%ecx\n\t"
		"notl	%%eax\n\t"		"bswapl	%%edi\n\t"
		"notl	%%edx\n\t"		"orl	%%edi, %%ebx\n\t"	"orl	%%ecx, %%esi\n\t"
		"andl	%%esi, %%edx\n\t"
		"andl	%%ebx, %%eax"
	: "=&A" (moves)
	: "m" (P), "m" (((unsigned int *)&P)[1]), "m" (O), "m" (((unsigned int *)&O)[1]), "m" (mask7e)
	: "ebx", "ecx", "esi", "edi" );

	return moves;
}

#endif // hasSSE2
#endif // x86

#if defined(__x86_64__) || defined(_M_X64)
/**
 * @brief SSE optimized get_stable_edge
 *
 * @param P bitboard with player's discs.
 * @param O bitboard with opponent's discs.
 * @return a bitboard with (some of) player's stable discs.
 *
 */
static unsigned long long get_stable_edge(const unsigned long long P, const unsigned long long O)
{
	// compute the exact stable edges (from precomputed tables)
	unsigned int a1a8po, h1h8po;
	unsigned long long stable_edge;

	__m128i	P0 = _mm_cvtsi64_si128(P);
	__m128i	O0 = _mm_cvtsi64_si128(O);
	__m128i	PO = _mm_unpacklo_epi8(O0, P0);
	stable_edge = edge_stability[_mm_extract_epi16(PO, 0)]
		| ((unsigned long long) edge_stability[_mm_extract_epi16(PO, 7)] << 56);

	PO = _mm_unpacklo_epi64(O0, P0);
	a1a8po = _mm_movemask_epi8(_mm_slli_epi64(PO, 7));
	h1h8po = _mm_movemask_epi8(PO);
#if 0 // def __BMI2__ // pdep is slow on AMD
	stable_edge |= _pdep_u64(edge_stability[a1a8po], 0x0101010101010101ULL)
		| _pdep_u64(edge_stability[h1h8po], 0x8080808080808080ULL);
#else
	stable_edge |= A1_A8[edge_stability[a1a8po]] | (A1_A8[edge_stability[h1h8po]] << 7);
#endif
	return stable_edge;
}

/**
 * @brief X64 optimized get_stability
 *
 * SSE pcmpeqb for horizontal get_full_lines.
 * CPU rotate for vertical get_full_lines.
 * Diag-7 is converted to diag-9 using vertical mirroring.
 * 
 * @param P bitboard with player's discs.
 * @param O bitboard with opponent's discs.
 * @return the number of stable discs.
 */
#ifdef __AVX2__

int get_stability(const unsigned long long P, const unsigned long long O)
{
	unsigned long long disc = (P | O);
	unsigned long long P_central = (P & 0x007e7e7e7e7e7e00ULL);
	unsigned long long l8, stable;
	__m128i	l81, l79, v2_stable, v2_old_stable, v2_P_central;
	__m256i	lr79, v4_disc, v4_stable, v4_full;
	const __m128i kff = _mm_set1_epi64x(0xffffffffffffffff);
	const __m256i shift1897 = _mm256_set_epi64x(7, 9, 8, 1);
#if 0 // PCMPEQQ
	static const V4DI m791 = {{ 0x0402010000804020, 0x2040800000010204, 0x0804020180402010, 0x1020408001020408 }};	// V8SI
	static const V4DI m792 = {{ 0x0000008040201008, 0x0000000102040810, 0x1008040201000000, 0x0810204080000000 }};
	static const V4DI m793 = {{ 0x0000804020100804, 0x0000010204081020, 0x2010080402010000, 0x0408102040800000 }};
	static const V4DI m794 = {{ 0x0080402010080402, 0x0001020408102040, 0x4020100804020100, 0x0204081020408000 }};
	static const V2DI m795 = {{ 0x8040201008040201, 0x0102040810204080 }};

	l81 = _mm_cvtsi64_si128(disc);		v4_disc = _mm256_broadcastq_epi64(l81);
	l81 = _mm_cmpeq_epi8(kff, l81);		lr79 = _mm256_and_si256(_mm256_cmpeq_epi32(_mm256_and_si256(v4_disc, m791.v4), m791.v4), m791.v4);
						lr79 = _mm256_or_si256(lr79, _mm256_and_si256(_mm256_cmpeq_epi64(_mm256_and_si256(v4_disc, m792.v4), m792.v4), m792.v4));
	l8 = disc;				lr79 = _mm256_or_si256(lr79, _mm256_and_si256(_mm256_cmpeq_epi64(_mm256_and_si256(v4_disc, m793.v4), m793.v4), m793.v4));
	l8 &= (l8 >> 8) | (l8 << 56);		lr79 = _mm256_or_si256(lr79, _mm256_and_si256(_mm256_cmpeq_epi64(_mm256_and_si256(v4_disc, m794.v4), m794.v4), m794.v4));
	l8 &= (l8 >> 16) | (l8 << 48);		l79 = _mm_and_si128(_mm_cmpeq_epi64(_mm_and_si128(_mm256_castsi256_si128(v4_disc), m795.v2), m795.v2), m795.v2);
	l8 &= (l8 >> 32) | (l8 << 32);		l79 = _mm_or_si128(l79, _mm_or_si128(_mm256_extracti128_si256(lr79, 1), _mm256_castsi256_si128(lr79)));

#elif 0 // PCMPEQD
	__m256i lm79;
	static const V4DI m790 = {{ 0x80c0e0f0783c1e0f, 0x0103070f1e3c78f0, 0x70381c0e07030100, 0x0e1c3870e0c08000 }};
	static const V4DI m791 = {{ 0x0402010000804020, 0x2040800000010204, 0x0804020180402010, 0x1020408001020408 }};	// V8SI
	static const V4DI m792 = {{ 0x2010884440201088, 0x0408112202040811, 0x2211080411080402, 0x4488102088102040 }};	// V8SI
	static const V4DI m793 = {{ 0x8844221110884422, 0x1122448808112244, 0x0000000044221108, 0x0000000022448810 }};	// V8SI

	l81 = _mm_cvtsi64_si128(disc);		v4_disc = _mm256_broadcastq_epi64(l81);
	l81 = _mm_cmpeq_epi8(kff, l81);		lm79 = _mm256_and_si256(v4_disc, m790.v4);
						lm79 = _mm256_or_si256(lm79, _mm256_shuffle_epi32(lm79, 0xb1));
	l8 = disc;				lr79 = _mm256_and_si256(_mm256_cmpeq_epi32(_mm256_and_si256(lm79, m792.v4), m792.v4), m792.v4);
	l8 &= (l8 >> 8) | (l8 << 56);		lr79 = _mm256_or_si256(lr79, _mm256_and_si256(_mm256_cmpeq_epi32(_mm256_and_si256(lm79, m793.v4), m793.v4), m793.v4));
	l8 &= (l8 >> 16) | (l8 << 48);		lr79 = _mm256_and_si256(_mm256_or_si256(lr79, _mm256_shuffle_epi32(lr79, 0xb1)), m790.v4);
	l8 &= (l8 >> 32) | (l8 << 32);		lr79 = _mm256_or_si256(lr79, _mm256_and_si256(_mm256_cmpeq_epi32(_mm256_and_si256(v4_disc, m791.v4), m791.v4), m791.v4));
						l79 = _mm_or_si128(_mm256_extracti128_si256(lr79, 1), _mm256_castsi256_si128(lr79));

#else // Kogge-Stone
	const __m128i mcpyswap = _mm_set_epi8(0, 1, 2, 3, 4, 5, 6, 7, 7, 6, 5, 4, 3, 2, 1, 0);
	const __m128i mbswapll = _mm_set_epi8(8, 9, 10, 11, 12, 13, 14, 15, 0, 1, 2, 3, 4, 5, 6, 7);
	static const V4DI shiftlr[] = {{{ 9, 7, 7, 9 }}, {{ 18, 14, 14, 18 }}, {{ 36, 28, 28, 36 }}};
	static const V4DI e790 = {{ 0xff80808080808080, 0xff01010101010101, 0xff01010101010101, 0xff80808080808080 }};
	static const V4DI e791 = {{ 0xffffc0c0c0c0c0c0, 0xffff030303030303, 0xffff030303030303, 0xffffc0c0c0c0c0c0 }};
	static const V4DI e792 = {{ 0xfffffffff0f0f0f0, 0xffffffff0f0f0f0f, 0xffffffff0f0f0f0f, 0xfffffffff0f0f0f0 }};

	l81 = _mm_cvtsi64_si128(disc);		v4_disc = _mm256_castsi128_si256(_mm_shuffle_epi8(l81, mcpyswap));
	l81 = _mm_cmpeq_epi8(kff, l81);		lr79 = _mm256_permute4x64_epi64(v4_disc, 0x50);	// disc, disc, rdisc, rdisc
						lr79 = _mm256_and_si256(lr79, _mm256_or_si256(e790.v4, _mm256_srlv_epi64(lr79, shiftlr[0].v4)));
	l8 = disc;				lr79 = _mm256_and_si256(lr79, _mm256_or_si256(e791.v4, _mm256_srlv_epi64(lr79, shiftlr[1].v4)));
	l8 &= (l8 >> 8) | (l8 << 56);		lr79 = _mm256_and_si256(lr79, _mm256_or_si256(e792.v4, _mm256_srlv_epi64(lr79, shiftlr[2].v4)));
	l8 &= (l8 >> 16) | (l8 << 48);		l79 = _mm_shuffle_epi8(_mm256_extracti128_si256(lr79, 1), mbswapll);
	l8 &= (l8 >> 32) | (l8 << 32);		l79 = _mm_and_si128(l79, _mm256_castsi256_si128(lr79));

#endif
	l81 = _mm_insert_epi64(l81, l8, 1);

	// compute the exact stable edges (from precomputed tables)
	stable = get_stable_edge(P, O);

	// add full lines
	v2_stable = _mm_and_si128(l81, l79);
	stable |= _mm_cvtsi128_si64(_mm_and_si128(v2_stable, _mm_unpackhi_epi64(v2_stable, v2_stable))) & P_central;

	if (stable == 0)
		return 0;

	// now compute the other stable discs (ie discs touching another stable disc in each flipping direction).
	v4_full = _mm256_insertf128_si256(_mm256_castsi128_si256(l81), l79, 1);
	v2_stable = _mm_cvtsi64_si128(stable);
	v2_P_central = _mm_cvtsi64_si128(P_central);
	do {
		v2_old_stable = v2_stable;
		v4_stable = _mm256_broadcastq_epi64(v2_stable);
		v4_stable = _mm256_or_si256(_mm256_or_si256(_mm256_srlv_epi64(v4_stable, shift1897), _mm256_sllv_epi64(v4_stable, shift1897)), v4_full);
		v2_stable = _mm_and_si128(_mm256_castsi256_si128(v4_stable), _mm256_extracti128_si256(v4_stable, 1));
		v2_stable = _mm_and_si128(v2_stable, _mm_unpackhi_epi64(v2_stable, v2_stable));
		v2_stable = _mm_or_si128(v2_old_stable, _mm_and_si128(v2_stable, v2_P_central));
	} while (!_mm_testc_si128(v2_old_stable, v2_stable));

	return bit_count(_mm_cvtsi128_si64(v2_stable));
}

#else

int get_stability(const unsigned long long P, const unsigned long long O)
{
	unsigned long long disc = (P | O);
	unsigned long long P_central = (P & 0x007e7e7e7e7e7e00ULL);
	unsigned long long l8, full_h, full_v, full_d7, full_d9, stable;
	unsigned long long stable_h, stable_v, stable_d7, stable_d9, old_stable;
#if 1	// 1 CPU, 3 SSE
	__m128i l01, l79, r79;	// full lines
	const __m128i kff  = _mm_set1_epi64x(0xffffffffffffffff);
	const __m128i edge = _mm_set1_epi64x(0xff818181818181ff);
	const __m128i e791 = _mm_set1_epi64x(0xffffc0c0c0c0c0c0);
	const __m128i e792 = _mm_set1_epi64x(0x030303030303ffff);
	const __m128i e793 = _mm_set1_epi64x(0x0f0f0f0ff0f0f0f0);

	l01 = l79 = _mm_cvtsi64_si128(disc);	r79 = _mm_cvtsi64_si128(vertical_mirror(disc));
	l01 = _mm_cmpeq_epi8(kff, l01);		l79 = r79 = _mm_unpacklo_epi64(l79, r79);
	full_h = _mm_cvtsi128_si64(l01);	l79 = _mm_and_si128(l79, _mm_or_si128(edge, _mm_srli_epi64(l79, 9)));
						r79 = _mm_and_si128(r79, _mm_or_si128(edge, _mm_slli_epi64(r79, 9)));
	l8 = disc;				l79 = _mm_and_si128(l79, _mm_or_si128(e791, _mm_srli_epi64(l79, 18)));
	l8 &= (l8 >> 8) | (l8 << 56);		r79 = _mm_and_si128(r79, _mm_or_si128(e792, _mm_slli_epi64(r79, 18)));
	l8 &= (l8 >> 16) | (l8 << 48);		l79 = _mm_and_si128(_mm_and_si128(l79, r79), _mm_or_si128(e793, _mm_or_si128(_mm_srli_epi64(l79, 36), _mm_slli_epi64(r79, 36))));
	l8 &= (l8 >> 32) | (l8 << 32);		full_d9 = _mm_cvtsi128_si64(l79);
	full_v = l8;				full_d7 = vertical_mirror(_mm_cvtsi128_si64(_mm_unpackhi_epi64(l79, l79)));

#else	// 4 CPU
	unsigned long long l1, l7, l9, r7, r9;	// full lines
	static const unsigned long long edge = 0xff818181818181ffULL;
	static const unsigned long long k01 = 0x0101010101010101ULL;
	static const unsigned long long e7[] = { 0xffff030303030303, 0xc0c0c0c0c0c0ffff, 0xffffffff0f0f0f0f, 0xf0f0f0f0ffffffff };
	static const unsigned long long e9[] = { 0xffffc0c0c0c0c0c0, 0x030303030303ffff, 0x0f0f0f0ff0f0f0f0 };

	l1 = l7 = r7 = disc;
	l1 &= l1 >> 1;				l7 &= edge | (l7 >> 7);		r7 &= edge | (r7 << 7);
	l1 &= l1 >> 2;				l7 &= e7[0] | (l7 >> 14);	r7 &= e7[1] | (r7 << 14);
	l1 &= l1 >> 4;				l7 &= e7[2] | (l7 >> 28);	r7 &= e7[3] | (r7 << 28);
	full_h = ((l1 & k01) * 0xff);		full_d7 = l7 & r7;

	l8 = l9 = r9 = disc;
	l8 &= (l8 >> 8) | (l8 << 56);		l9 &= edge | (l9 >> 9);		r9 &= edge | (r9 << 9);
	l8 &= (l8 >> 16) | (l8 << 48);		l9 &= e9[0] | (l9 >> 18);	r9 &= e9[1] | (r9 << 18);
	l8 &= (l8 >> 32) | (l8 << 32);		full_d9 = l9 & r9 & (e9[2] | (l9 >> 36) | (r9 << 36));
	full_v = l8;

#endif
	// compute the exact stable edges (from precomputed tables)
	stable = get_stable_edge(P, O);

	// add full lines
	stable |= (full_h & full_v & full_d7 & full_d9 & P_central);

	if (stable == 0)
		return 0;

	// now compute the other stable discs (ie discs touching another stable disc in each flipping direction).
	do {
		old_stable = stable;
		stable_h = ((stable >> 1) | (stable << 1) | full_h);
		stable_v = ((stable >> 8) | (stable << 8) | full_v);
		stable_d7 = ((stable >> 7) | (stable << 7) | full_d7);
		stable_d9 = ((stable >> 9) | (stable << 9) | full_d9);
		stable |= (stable_h & stable_v & stable_d7 & stable_d9 & P_central);
	} while (stable != old_stable);

	return bit_count(stable);
}

#endif // __AVX2__
#endif // __x86_64__

/**
 * @brief SSE translation of board_get_hash_code.
 *
 * Too many dependencies, effective only on 32bit build.
 * For AMD, MMX version in board_mmx.c is faster.
 *
 * @param p pointer to 16 bytes to hash.
 * @return the hash code of the bitboard
 */
#if (defined(USE_GAS_MMX) && !defined(__3dNOW__)) || defined(USE_MSVC_X86) // || defined(__x86_64__)

unsigned long long board_get_hash_code_sse(const unsigned char *p)
{
	unsigned long long h;
#if defined(hasSSE2) || defined(USE_MSVC_X86)
	__m128	h0, h1, h2, h3;

	h0 = _mm_loadh_pi(_mm_castsi128_ps(_mm_loadl_epi64((__m128i *) &hash_rank[0][p[0]])), (__m64 *) &hash_rank[4][p[4]]);
	h1 = _mm_loadh_pi(_mm_castsi128_ps(_mm_loadl_epi64((__m128i *) &hash_rank[1][p[1]])), (__m64 *) &hash_rank[5][p[5]]);
	h2 = _mm_loadh_pi(_mm_castsi128_ps(_mm_loadl_epi64((__m128i *) &hash_rank[2][p[2]])), (__m64 *) &hash_rank[6][p[6]]);
	h3 = _mm_loadh_pi(_mm_castsi128_ps(_mm_loadl_epi64((__m128i *) &hash_rank[3][p[3]])), (__m64 *) &hash_rank[7][p[7]]);
	h0 = _mm_xor_ps(h0, h2);	h1 = _mm_xor_ps(h1, h3);
	h2 = _mm_loadh_pi(_mm_castsi128_ps(_mm_loadl_epi64((__m128i *) &hash_rank[8][p[8]])), (__m64 *) &hash_rank[10][p[10]]);
	h3 = _mm_loadh_pi(_mm_castsi128_ps(_mm_loadl_epi64((__m128i *) &hash_rank[9][p[9]])), (__m64 *) &hash_rank[11][p[11]]);
	h0 = _mm_xor_ps(h0, h2);	h1 = _mm_xor_ps(h1, h3);
	h2 = _mm_loadh_pi(_mm_castsi128_ps(_mm_loadl_epi64((__m128i *) &hash_rank[12][p[12]])), (__m64 *) &hash_rank[14][p[14]]);
	h3 = _mm_loadh_pi(_mm_castsi128_ps(_mm_loadl_epi64((__m128i *) &hash_rank[13][p[13]])), (__m64 *) &hash_rank[15][p[15]]);
	h0 = _mm_xor_ps(h0, h2);	h1 = _mm_xor_ps(h1, h3);
	h0 = _mm_xor_ps(h0, h1);
	h0 = _mm_xor_ps(h0, _mm_movehl_ps(h1, h0));
	h = _mm_cvtsi128_si64(_mm_castps_si128(h0));

#else
	__asm__ volatile (
		"movq	%0, %%xmm0\n\t"		"movq	%1, %%xmm1"
	: : "m" (hash_rank[0][p[0]]), "m" (hash_rank[1][p[1]]));
	__asm__ volatile (
		"movq	%0, %%xmm2\n\t"		"movq	%1, %%xmm3"
	: : "m" (hash_rank[2][p[2]]), "m" (hash_rank[3][p[3]]));
	__asm__ volatile (
		"movhps	%0, %%xmm0\n\t"		"movhps	%1, %%xmm1"
	: : "m" (hash_rank[4][p[4]]), "m" (hash_rank[5][p[5]]));
	__asm__ volatile (
		"movhps	%0, %%xmm2\n\t"		"movhps	%1, %%xmm3"
	: : "m" (hash_rank[6][p[6]]), "m" (hash_rank[7][p[7]]));
	__asm__ volatile (
		"xorps	%%xmm2, %%xmm0\n\t"	"xorps	%%xmm3, %%xmm1\n\t"
		"movq	%0, %%xmm2\n\t"		"movq	%1, %%xmm3"
	: : "m" (hash_rank[8][p[8]]), "m" (hash_rank[9][p[9]]));
	__asm__ volatile (
		"movhps	%0, %%xmm2\n\t"		"movhps	%1, %%xmm3"
	: : "m" (hash_rank[10][p[10]]), "m" (hash_rank[11][p[11]]));
	__asm__ volatile (
		"xorps	%%xmm2, %%xmm0\n\t"	"xorps	%%xmm3, %%xmm1\n\t"
		"movq	%0, %%xmm2\n\t"		"movq	%1, %%xmm3"
	: : "m" (hash_rank[12][p[12]]), "m" (hash_rank[13][p[13]]));
	__asm__ volatile (
		"movhps	%1, %%xmm2\n\t"		"movhps	%2, %%xmm3\n\t"
		"xorps	%%xmm2, %%xmm0\n\t"	"xorps	%%xmm3, %%xmm1\n\t"
		"xorps	%%xmm1, %%xmm0\n\t"
		"movhlps %%xmm0, %%xmm1\n\t"
		"xorps	%%xmm1, %%xmm0\n\t"
		"movd	%%xmm0, %%eax\n\t"
		"punpckhdq %%xmm0, %%xmm0\n\t"
		"movd	%%xmm0, %%edx"
	: "=A" (h) : "m" (hash_rank[14][p[14]]), "m" (hash_rank[15][p[15]]));
#endif

	return h;
}

#endif // USE_GAS_MMX

#if 0 // def __AVX2__	// experimental - too many instructions

unsigned long long board_get_hash_code_avx2(const unsigned char *p)
{
	__m128i	ix0, ix8, hh;
	__m256i	hhh;
	static const __v16qi rank = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15 };

	ix0 = _mm_loadu_si128((__m128i *) p);
	ix8 = _mm_unpackhi_epi8(ix0, (__m128i) rank);
	ix0 = _mm_unpacklo_epi8(ix0, (__m128i) rank);

	hhh  = _mm256_i32gather_epi64((long long *) hash_rank[0], _mm_blend_epi16(_mm_setzero_si128(), ix0, 0x55), 8);
	hhh ^= _mm256_i32gather_epi64((long long *) hash_rank[0], _mm_blend_epi16(_mm_setzero_si128(), ix8, 0x55), 8);
	hhh ^= _mm256_i32gather_epi64((long long *) hash_rank[0], _mm_srli_epi32(ix0, 16), 8);
	hhh ^= _mm256_i32gather_epi64((long long *) hash_rank[0], _mm_srli_epi32(ix8, 16), 8);

	hh = _mm256_castsi256_si128(hhh) ^ _mm256_extracti128_si256(hhh, 1);
	hh ^= _mm_shuffle_epi32(hh, 0x4e);
	return hh[0];
}

#endif
>>>>>>> 1a7b0ed (flip_bmi2 added; bmi2 version of stability and corner_stability)
=======

#elif defined(hasNeon)
void get_all_full_lines(const unsigned long long disc, unsigned long long full[5])
{
	unsigned long long l8;
	uint8x8_t l01;
	uint64x2_t l79, r79;
	const uint64x2_t e790 = vdupq_n_u64(0x007f7f7f7f7f7f7f);
	const uint64x2_t e791 = vdupq_n_u64(0xfefefefefefefe00);
	const uint64x2_t e792 = vdupq_n_u64(0x00003f3f3f3f3f3f);
	const uint64x2_t e793 = vdupq_n_u64(0x0f0f0f0ff0f0f0f0);

	l01 = vcreate_u8(disc);			l79 = r79 = vreinterpretq_u64_u8(vcombine_u8(l01, vrev64_u8(l01)));
	l01 = vceq_u8(l01, vdup_n_u8(0xff));	l79 = vandq_u64(l79, vornq_u64(vshrq_n_u64(l79, 9), e790));
	full[0] = vget_lane_u64(vreinterpret_u64_u8(l01), 0);
						r79 = vandq_u64(r79, vornq_u64(vshlq_n_u64(r79, 9), e791));
	l8 = disc;				l79 = vbicq_u64(l79, vbicq_u64(e792, vshrq_n_u64(l79, 18)));	// De Morgan
	l8 &= (l8 >> 8) | (l8 << 56);		r79 = vbicq_u64(r79, vshlq_n_u64(vbicq_u64(e792, r79), 18));
	l8 &= (l8 >> 16) | (l8 << 48);		l79 = vandq_u64(vandq_u64(l79, r79), vorrq_u64(e793, vsliq_n_u64(vshrq_n_u64(l79, 36), r79, 36)));
	l8 &= (l8 >> 32) | (l8 << 32);		full[2] = vgetq_lane_u64(l79, 0);
	full[1] = l8;				full[3] = vertical_mirror(vgetq_lane_u64(l79, 1));

	full[4] = full[0] & l8 & full[2] & full[3];
}

#else	// 1 CPU, 3 SSE
void get_all_full_lines(const unsigned long long disc, unsigned long long full[5])
{
	unsigned long long l8;
	__m128i l01, l79, r79;	// full lines
	const __m128i kff  = _mm_set1_epi64x(0xffffffffffffffff);
	const __m128i e790 = _mm_set1_epi64x(0xff80808080808080);
	const __m128i e791 = _mm_set1_epi64x(0x01010101010101ff);
	const __m128i e792 = _mm_set1_epi64x(0x00003f3f3f3f3f3f);
	const __m128i e793 = _mm_set1_epi64x(0x0f0f0f0ff0f0f0f0);

	l01 = l79 = _mm_cvtsi64_si128(disc);	l79 = r79 = _mm_unpacklo_epi64(l79, _mm_cvtsi64_si128(vertical_mirror(disc)));
	l01 = _mm_cmpeq_epi8(kff, l01);		l79 = _mm_and_si128(l79, _mm_or_si128(e790, _mm_srli_epi64(l79, 9)));
	_mm_storel_epi64((__m128i*) &full[0], l01);
						r79 = _mm_and_si128(r79, _mm_or_si128(e791, _mm_slli_epi64(r79, 9)));
	l8 = disc;				l79 = _mm_andnot_si128(_mm_andnot_si128(_mm_srli_epi64(l79, 18), e792), l79);	// De Morgan
	l8 &= (l8 >> 8) | (l8 << 56);		r79 = _mm_andnot_si128(_mm_slli_epi64(_mm_andnot_si128(r79, e792), 18), r79);
	l8 &= (l8 >> 16) | (l8 << 48);		l79 = _mm_and_si128(_mm_and_si128(l79, r79), _mm_or_si128(e793, _mm_or_si128(_mm_srli_epi64(l79, 36), _mm_slli_epi64(r79, 36))));
	l8 &= (l8 >> 32) | (l8 << 32);		_mm_storel_epi64((__m128i *) &full[2], l79);
	full[1] = l8;				full[3] = vertical_mirror(_mm_cvtsi128_si64(_mm_unpackhi_epi64(l79, l79)));

	full[4] = full[0] & l8 & full[2] & full[3];
}

#endif
<<<<<<< HEAD
#endif // HAS_CPU_64/ANDROID
<<<<<<< HEAD

/**
 * @brief SSE translation of board_get_hash_code.
 *
 * Too many dependencies, effective only on 32bit build.
 * For AMD, MMX version in board_mmx.c is faster.
 *
 * @param p pointer to 16 bytes to hash.
 * @return the hash code of the bitboard
 */
#if (defined(USE_GAS_MMX) && !defined(__3dNOW__)) || defined(USE_MSVC_X86) // || defined(__x86_64__)

unsigned long long board_get_hash_code_sse(const unsigned char *p)
{
	unsigned long long h;
#if defined(hasSSE2) || defined(USE_MSVC_X86)
	__m128	h0, h1, h2, h3;

	h0 = _mm_loadh_pi(_mm_castsi128_ps(_mm_loadl_epi64((__m128i *) &hash_rank[0][p[0]])), (__m64 *) &hash_rank[4][p[4]]);
	h1 = _mm_loadh_pi(_mm_castsi128_ps(_mm_loadl_epi64((__m128i *) &hash_rank[1][p[1]])), (__m64 *) &hash_rank[5][p[5]]);
	h2 = _mm_loadh_pi(_mm_castsi128_ps(_mm_loadl_epi64((__m128i *) &hash_rank[2][p[2]])), (__m64 *) &hash_rank[6][p[6]]);
	h3 = _mm_loadh_pi(_mm_castsi128_ps(_mm_loadl_epi64((__m128i *) &hash_rank[3][p[3]])), (__m64 *) &hash_rank[7][p[7]]);
	h0 = _mm_xor_ps(h0, h2);	h1 = _mm_xor_ps(h1, h3);
	h2 = _mm_loadh_pi(_mm_castsi128_ps(_mm_loadl_epi64((__m128i *) &hash_rank[8][p[8]])), (__m64 *) &hash_rank[10][p[10]]);
	h3 = _mm_loadh_pi(_mm_castsi128_ps(_mm_loadl_epi64((__m128i *) &hash_rank[9][p[9]])), (__m64 *) &hash_rank[11][p[11]]);
	h0 = _mm_xor_ps(h0, h2);	h1 = _mm_xor_ps(h1, h3);
	h2 = _mm_loadh_pi(_mm_castsi128_ps(_mm_loadl_epi64((__m128i *) &hash_rank[12][p[12]])), (__m64 *) &hash_rank[14][p[14]]);
	h3 = _mm_loadh_pi(_mm_castsi128_ps(_mm_loadl_epi64((__m128i *) &hash_rank[13][p[13]])), (__m64 *) &hash_rank[15][p[15]]);
	h0 = _mm_xor_ps(h0, h2);	h1 = _mm_xor_ps(h1, h3);
	h0 = _mm_xor_ps(h0, h1);
	h0 = _mm_xor_ps(h0, _mm_movehl_ps(h1, h0));
	h = _mm_cvtsi128_si64(_mm_castps_si128(h0));

#else
	__asm__ volatile (
		"movq	%0, %%xmm0\n\t"		"movq	%1, %%xmm1"
	: : "m" (hash_rank[0][p[0]]), "m" (hash_rank[1][p[1]]));
	__asm__ volatile (
		"movq	%0, %%xmm2\n\t"		"movq	%1, %%xmm3"
	: : "m" (hash_rank[2][p[2]]), "m" (hash_rank[3][p[3]]));
	__asm__ volatile (
		"movhps	%0, %%xmm0\n\t"		"movhps	%1, %%xmm1"
	: : "m" (hash_rank[4][p[4]]), "m" (hash_rank[5][p[5]]));
	__asm__ volatile (
		"movhps	%0, %%xmm2\n\t"		"movhps	%1, %%xmm3"
	: : "m" (hash_rank[6][p[6]]), "m" (hash_rank[7][p[7]]));
	__asm__ volatile (
		"xorps	%%xmm2, %%xmm0\n\t"	"xorps	%%xmm3, %%xmm1\n\t"
		"movq	%0, %%xmm2\n\t"		"movq	%1, %%xmm3"
	: : "m" (hash_rank[8][p[8]]), "m" (hash_rank[9][p[9]]));
	__asm__ volatile (
		"movhps	%0, %%xmm2\n\t"		"movhps	%1, %%xmm3"
	: : "m" (hash_rank[10][p[10]]), "m" (hash_rank[11][p[11]]));
	__asm__ volatile (
		"xorps	%%xmm2, %%xmm0\n\t"	"xorps	%%xmm3, %%xmm1\n\t"
		"movq	%0, %%xmm2\n\t"		"movq	%1, %%xmm3"
	: : "m" (hash_rank[12][p[12]]), "m" (hash_rank[13][p[13]]));
	__asm__ volatile (
		"movhps	%1, %%xmm2\n\t"		"movhps	%2, %%xmm3\n\t"
		"xorps	%%xmm2, %%xmm0\n\t"	"xorps	%%xmm3, %%xmm1\n\t"
		"xorps	%%xmm1, %%xmm0\n\t"
		"movhlps %%xmm0, %%xmm1\n\t"
		"xorps	%%xmm1, %%xmm0\n\t"
		"movd	%%xmm0, %%eax\n\t"
		"punpckhdq %%xmm0, %%xmm0\n\t"
		"movd	%%xmm0, %%edx"
	: "=A" (h) : "m" (hash_rank[14][p[14]]), "m" (hash_rank[15][p[15]]));
#endif

	return h;
}

#endif // USE_GAS_MMX

#if 0 // def __AVX2__	// experimental - too many instructions

unsigned long long board_get_hash_code_avx2(const unsigned char *p)
{
	__m128i	ix0, ix8, hh;
	__m256i	hhh;
	static const __v16qi rank = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15 };

	ix0 = _mm_loadu_si128((__m128i *) p);
	ix8 = _mm_unpackhi_epi8(ix0, (__m128i) rank);
	ix0 = _mm_unpacklo_epi8(ix0, (__m128i) rank);

	hhh  = _mm256_i32gather_epi64((long long *) hash_rank[0], _mm_blend_epi16(_mm_setzero_si128(), ix0, 0x55), 8);
	hhh ^= _mm256_i32gather_epi64((long long *) hash_rank[0], _mm_blend_epi16(_mm_setzero_si128(), ix8, 0x55), 8);
	hhh ^= _mm256_i32gather_epi64((long long *) hash_rank[0], _mm_srli_epi32(ix0, 16), 8);
	hhh ^= _mm256_i32gather_epi64((long long *) hash_rank[0], _mm_srli_epi32(ix8, 16), 8);

	hh = _mm256_castsi256_si128(hhh) ^ _mm256_extracti128_si256(hhh, 1);
	hh ^= _mm_shuffle_epi32(hh, 0x4e);
	return hh[0];
}

#endif
>>>>>>> 3e1ed4f (fix cr/lf in repository to lf)
=======
>>>>>>> 34a2291 (4.5.0: Use CRC32c for board hash)
=======
#endif // hasSSE2/hasNeon
<<<<<<< HEAD
>>>>>>> 21f8809 (Share all full lines between get_stability and Dogaishi hash reduction)
=======

#ifdef __AVX2__
/**
 * @brief AVX2 optimized get_potential_moves.
 *
 * Get the list of empty squares in contact of a player square.
 *
 * @param P bitboard with player's discs.
 * @param O bitboard with opponent's discs.
 * @return all potential moves in a 64-bit unsigned integer.
 */
static unsigned long long get_potential_moves(const unsigned long long P, const unsigned long long O)
{
	const __m256i shift1897 = _mm256_set_epi64x(7, 9, 8, 1);
	__m256i	O4 = _mm256_broadcastq_epi64(_mm_cvtsi64_si128(O));
	__m128i O2;

	O4 = _mm256_and_si256(O4, _mm256_set_epi64x(0x007E7E7E7E7E7E00, 0x007E7E7E7E7E7E00, 0x00FFFFFFFFFFFF00, 0x7E7E7E7E7E7E7E7E));
	O4 = _mm256_or_si256(O4, _mm256_or_si256(_mm256_srlv_epi64(O4, shift1897), _mm256_sllv_epi64(O4, shift1897)));
	O2 = _mm_or_si128(_mm256_castsi256_si128(O4), _mm256_extracti128_si256(O4, 1));
	O2 = _mm_or_si128(O2, _mm_unpackhi_epi64(O2, O2));
	return _mm_cvtsi128_si64(O2) & ~(P|O); // mask with empties
}
#endif
>>>>>>> be2ba1c (add AVX get_potential_mobility; revise foreach_bit for CPU32/C99)
