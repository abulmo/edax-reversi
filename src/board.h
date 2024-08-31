/**
 * @file board.h
 *
 * Board management header file.
 *
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
 * @date 1998 - 2024
=======
 * @date 1998 - 2021
>>>>>>> 34a2291 (4.5.0: Use CRC32c for board hash)
=======
 * @date 1998 - 2022
>>>>>>> 9e2bbc5 (split get_all_full_lines from get_stability)
=======
 * @date 1998 - 2023
>>>>>>> 8566ed0 (vector call version of board_next & get_moves)
 * @author Richard Delorme
 * @version 4.5
 */

#ifndef EDAX_BOARD_H
#define EDAX_BOARD_H

#include "const.h"
#include "settings.h"
#include "bit.h"

#include <stdio.h>
#include <stdbool.h>

// struct Board: moved to bit.h

struct Move;
struct Random;

/* function declarations */
void board_init(Board*);
int board_set(Board*, const char*);
int board_from_FEN(Board*, const char*);
bool board_lesser(const Board*, const Board*);
<<<<<<< HEAD
void board_horizontal_mirror(const Board *, Board *);
void board_vertical_mirror(const Board *, Board *);
void board_transpose(const Board *, Board *);
=======
bool board_equal(const Board*, const Board*);
>>>>>>> 8a7e354 (Exclude hash init time from count games; other minor size opts)
void board_symetry(const Board*, const int, Board*);
int board_unique(const Board*, Board*);
void board_check(const Board*);
void board_rand(Board*, int, struct Random*);

// Compare two board for equality
#define	board_equal(b1,b2)	((b1)->player == (b2)->player && (b1)->opponent == (b2)->opponent)

int board_count_last_flips(const Board*, const int);
unsigned long long board_get_move_flip(const Board*, const int, struct Move*);
bool board_check_move(const Board*, struct Move*);
void board_swap_players(Board*);
void board_update(Board*, const struct Move*);
void board_restore(Board*, const struct Move*);
void board_pass(Board*);
<<<<<<< HEAD
<<<<<<< HEAD

bool can_move(const unsigned long long, const unsigned long long);
unsigned long long get_moves_6x6(const unsigned long long, const unsigned long long);
bool can_move_6x6(const unsigned long long, const unsigned long long);
int get_mobility(const unsigned long long, const unsigned long long);
#ifdef __AVX2__
	__m128i vectorcall get_moves_and_potential(__m256i, __m256i);
#else
	unsigned long long get_potential_moves(const unsigned long long, const unsigned long long);
#endif

void edge_stability_init(void);
unsigned long long get_stable_edge(const unsigned long long, const unsigned long long);
#ifndef __AVX2__	// public for android dispatch
	void get_full_lines(const unsigned long long, unsigned long long [4]);
  #if !(defined(hasMMX) && !defined(hasSSE2))
	int get_spreaded_stability(unsigned long long, unsigned long long, unsigned long long [4]);
  #endif
#endif
unsigned long long get_all_full_lines(const unsigned long long);
int get_stability(const unsigned long long, const unsigned long long);
int get_stability_fulls(const unsigned long long, const unsigned long long, unsigned long long [5]);
int get_edge_stability(const unsigned long long, const unsigned long long);
int get_corner_stability(const unsigned long long);
=======
unsigned long long board_next(const Board*, const int, Board*);
>>>>>>> 23e04d1 (Backport endgame_sse optimizations into endgame.c)
=======
>>>>>>> 8566ed0 (vector call version of board_next & get_moves)
unsigned long long board_get_hash_code(const Board*);
int board_get_square_color(const Board*, const int);
bool board_is_occupied(const Board*, const int);
void board_print(const Board*, const int, FILE*);
char* board_to_string(const Board*, const int, char *);
void board_print_FEN(const Board*, const int, FILE*);
char* board_to_FEN(const Board*, const int, char*);
bool board_is_pass(const Board*);
bool board_is_game_over(const Board*);
int board_count_empties(const Board *board);
#if defined(USE_GAS_MMX) || defined(USE_MSVC_X86)
	void init_mmx (void);
	unsigned long long get_moves_mmx(const unsigned long long, const unsigned long long);
	unsigned long long get_moves_sse(const unsigned long long, const unsigned long long);

<<<<<<< HEAD
<<<<<<< HEAD
#elif defined(ANDROID) && !defined(__ARM_NEON) && !defined(hasSSE2)
	void init_neon (void);
	unsigned long long get_moves_sse(unsigned long long, unsigned long long);
=======
unsigned long long get_moves(const unsigned long long, const unsigned long long);
=======
>>>>>>> 8566ed0 (vector call version of board_next & get_moves)
bool can_move(const unsigned long long, const unsigned long long);
unsigned long long get_moves_6x6(const unsigned long long, const unsigned long long);
bool can_move_6x6(const unsigned long long, const unsigned long long);
int get_mobility(const unsigned long long, const unsigned long long);
int get_weighted_mobility(const unsigned long long, const unsigned long long);
int get_potential_mobility(const unsigned long long, const unsigned long long);
void edge_stability_init(void);
unsigned long long get_stable_edge(const unsigned long long, const unsigned long long);
void get_all_full_lines(const unsigned long long, unsigned long long [5]);
int get_stability(const unsigned long long, const unsigned long long);
int get_stability_fulls(const unsigned long long, const unsigned long long, unsigned long long [5]);
int get_edge_stability(const unsigned long long, const unsigned long long);
int get_corner_stability(const unsigned long long);

#if defined(USE_GAS_MMX) || defined(USE_MSVC_X86)
void init_mmx (void);
unsigned long long get_moves_mmx(const unsigned long long, const unsigned long long);
unsigned long long get_moves_sse(const unsigned long long, const unsigned long long);
int get_potential_mobility_mmx(unsigned long long, unsigned long long);

#elif defined(ANDROID) && !defined(hasNeon) && !defined(hasSSE2)
void init_neon (void);
unsigned long long get_moves_sse(unsigned long long, unsigned long long);
#endif

<<<<<<< HEAD
#if defined(USE_GAS_MMX) && defined(__3dNOW__)
unsigned long long board_get_hash_code_mmx(const unsigned char *p);
#elif defined(USE_GAS_MMX) || defined(USE_MSVC_X86)
unsigned long long board_get_hash_code_sse(const unsigned char *p);
>>>>>>> 1b29848 (fix & optimize 32 bit build; other minor mods)
#endif

=======
>>>>>>> 34a2291 (4.5.0: Use CRC32c for board hash)
extern unsigned char edge_stability[256 * 256];
<<<<<<< HEAD
<<<<<<< HEAD

<<<<<<< HEAD
<<<<<<< HEAD
// a1/a8/h1/h8 are already stable in horizontal line, so omit them in vertical line to ease kindergarten for CPU_64
#if 0 // defined(__BMI2__) && defined(HAS_CPU_64) && !defined(__bdver4__) && !defined(__znver1__) && !defined(__znver2__) // pdep is slow on AMD before Zen3
	#define	unpackA2A7(x)	_pdep_u64((x), 0x0101010101010101)
	#define	unpackH2H7(x)	_pdep_u64((x), 0x8080808080808080)
=======
/* Define function attributes directive when available */
#if defined(__GNUC__) && !defined(__clang__) && defined(__i386__)
	#define	REGPARM	__attribute__((regparm(2)))
>>>>>>> e558fdb (Some cleanups for clang / android build)
#else
	#define	unpackA2A7(x)	((((x) & 0x7e) * 0x0000040810204080) & 0x0001010101010100)
	#define	unpackH2H7(x)	((((x) & 0x7e) * 0x0002040810204000) & 0x0080808080808000)
#endif

<<<<<<< HEAD
#if (LAST_FLIP_COUNTER == COUNT_LAST_FLIP_CARRY) || (LAST_FLIP_COUNTER == COUNT_LAST_FLIP_KINDERGARTEN) || (LAST_FLIP_COUNTER == COUNT_LAST_FLIP_BITSCAN) || (LAST_FLIP_COUNTER == COUNT_LAST_FLIP_32)
	extern int (*count_last_flip[BOARD_SIZE + 1])(const unsigned long long);
=======
#if ((LAST_FLIP_COUNTER == COUNT_LAST_FLIP_PLAIN) || (LAST_FLIP_COUNTER == COUNT_LAST_FLIP_SSE) || (LAST_FLIP_COUNTER == COUNT_LAST_FLIP_BMI2))
	extern int last_flip(int pos, unsigned long long P);
#else
	#if LAST_FLIP_COUNTER == COUNT_LAST_FLIP_32
		extern int (REGPARM *count_last_flip[BOARD_SIZE + 1])(const unsigned long long);
	#else
		extern int (*count_last_flip[BOARD_SIZE + 1])(const unsigned long long);
	#endif
>>>>>>> feb7fa7 (count_last_flip_bmi2 and transpose_avx2 added)
=======
#if ((LAST_FLIP_COUNTER == COUNT_LAST_FLIP_PLAIN) || (LAST_FLIP_COUNTER == COUNT_LAST_FLIP_SSE) || (LAST_FLIP_COUNTER == COUNT_LAST_FLIP_BMI2))
=======
extern unsigned long long A1_A8[256];
=======
>>>>>>> 9e2bbc5 (split get_all_full_lines from get_stability)

// a1/a8/h1/h8 are already stable in horizontal line, so omit them in vertical line to ease kindergarten for CPU_64
#if 0 // defined(__BMI2__) && !defined(__bdver4__) && !defined(__znver1__) && !defined(__znver2__) // pdep is slow on AMD before Zen3
#define	unpackA2A7(x)	_pdep_u64((x), 0x0101010101010101)
#define	unpackH2H7(x)	_pdep_u64((x), 0x8080808080808080)
#elif defined(HAS_CPU_64)
#define	unpackA2A7(x)	((((x) & 0x7e) * 0x0000040810204080ULL) & 0x0001010101010100ULL)
#define	unpackH2H7(x)	((((x) & 0x7e) * 0x0002040810204000ULL) & 0x0080808080808000ULL)
#else
#define	unpackA2A7(x)	(((unsigned long long)((((x) >> 4) * 0x00204081) & 0x01010101) << 32) | ((((x) & 0x0f) * 0x00204081) & 0x01010101))
#define	unpackH2H7(x)	(((unsigned long long)((((x) >> 4) * 0x10204080) & 0x80808080) << 32) | ((((x) & 0x0f) * 0x10204080) & 0x80808080))
#endif

#if (LAST_FLIP_COUNTER == COUNT_LAST_FLIP_PLAIN) || (LAST_FLIP_COUNTER == COUNT_LAST_FLIP_SSE) || (LAST_FLIP_COUNTER == COUNT_LAST_FLIP_BMI2)
>>>>>>> 343493d (More neon/sse optimizations; neon dispatch added for arm32)
	extern int last_flip(int pos, unsigned long long P);
#else
	extern int (*count_last_flip[BOARD_SIZE + 1])(const unsigned long long);
>>>>>>> 1b29848 (fix & optimize 32 bit build; other minor mods)
	#define	last_flip(x,P)	count_last_flip[x](P)
#else
	extern int last_flip(int pos, unsigned long long P);
#endif

<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
#if (MOVE_GENERATOR == MOVE_GENERATOR_AVX) || (MOVE_GENERATOR == MOVE_GENERATOR_AVX512)
	extern __m128i vectorcall mm_Flip(const __m128i OP, int pos);
<<<<<<< HEAD
	inline __m128i vectorcall reduce_vflip(__m128i flip) { return _mm_or_si128(flip, _mm_shuffle_epi32(flip, 0x4e)); }
	#define	Flip(x,P,O)	((unsigned long long) _mm_cvtsi128_si64(reduce_vflip(mm_Flip(_mm_set_epi64x((O), (P)), (x)))))
	#define	board_flip(board,x)	((unsigned long long) _mm_cvtsi128_si64(reduce_vflip(mm_Flip(_mm_loadu_si128((__m128i *) (board)), (x)))))
	#define	vboard_flip(board,x)	((unsigned long long) _mm_cvtsi128_si64(reduce_vflip(mm_Flip((board).v2, (x)))))

#elif MOVE_GENERATOR == MOVE_GENERATOR_SSE
	extern __m128i (vectorcall *mm_flip[BOARD_SIZE + 2])(const __m128i);
	#define	Flip(x,P,O)	((unsigned long long) _mm_cvtsi128_si64(mm_flip[x](_mm_set_epi64x((O), (P)))))
	#define mm_Flip(OP,x)	mm_flip[x](OP)
	#define reduce_vflip(x)	(x)
	#define	board_flip(board,x)	((unsigned long long) _mm_cvtsi128_si64(mm_flip[x](_mm_loadu_si128((__m128i *) (board)))))
	#define	vboard_flip(board,x)	((unsigned long long) _mm_cvtsi128_si64(mm_flip[x]((board).v2)))

#elif MOVE_GENERATOR == MOVE_GENERATOR_NEON
	extern uint64x2_t mm_Flip(uint64x2_t OP, int pos);
	#define	Flip(x,P,O)	vgetq_lane_u64(mm_Flip(vcombine_u64(vcreate_u64(P), vcreate_u64(O)), (x)), 0)
	#define	board_flip(board,x)	vgetq_lane_u64(mm_Flip(vld1q_u64((uint64_t *) (board)), (x)), 0)
	#define	vboard_flip(board,x)	vgetq_lane_u64(mm_Flip((board).v2, (x)), 0)

#elif MOVE_GENERATOR == MOVE_GENERATOR_SVE
	extern uint64_t Flip(int pos, uint64_t P, uint64_t O);
	#define mm_Flip(OP,x)	vdupq_n_u64(Flip((x), vgetq_lane_u64((OP), 0), vgetq_lane_u64((OP), 1)))
	#define	board_flip(board,x)	Flip((x), (board)->player, (board)->opponent)
	#define	vboard_flip(board,x)	Flip((x), vgetq_lane_u64((board).v2, 0), vgetq_lane_u64((board).v2, 1))

#elif MOVE_GENERATOR == MOVE_GENERATOR_32
	extern unsigned long long (*flip[BOARD_SIZE + 2])(unsigned int, unsigned int, unsigned int, unsigned int);
	#define Flip(x,P,O)	flip[x]((unsigned int)(P), (unsigned int)((P) >> 32), (unsigned int)(O), (unsigned int)((O) >> 32))
  #ifdef __BIG_ENDIAN__
	#define	board_flip(board,x)	flip[x]((unsigned int)((board)->player), ((unsigned int *) &(board)->player)[0], (unsigned int)((board)->opponent), ((unsigned int *) &(board)->opponent)[0])
  #else
	#define	board_flip(board,x)	flip[x]((unsigned int)((board)->player), ((unsigned int *) &(board)->player)[1], (unsigned int)((board)->opponent), ((unsigned int *) &(board)->opponent)[1])
  #endif
  #if defined(USE_GAS_MMX) && !defined(hasSSE2)
	extern void init_flip_sse(void);
  #endif

=======
#if (MOVE_GENERATOR == MOVE_GENERATOR_SSE_BSWAP) || (MOVE_GENERATOR == MOVE_GENERATOR_AVX)
	extern unsigned long long Flip(int, const unsigned long long, const unsigned long long);
=======
#if MOVE_GENERATOR == MOVE_GENERATOR_AVX
=======
#if (MOVE_GENERATOR == MOVE_GENERATOR_AVX) || (MOVE_GENERATOR == MOVE_GENERATOR_AVX512)
>>>>>>> 393b667 (Experimental AVX512VL/CD version of move generator)
	extern __m128i vectorcall mm_Flip(const __m128i OP, int pos);
	#define	Flip(x,P,O)	((unsigned long long) _mm_cvtsi128_si64(mm_Flip(_mm_unpacklo_epi64(_mm_cvtsi64_si128(P), _mm_cvtsi64_si128(O)), (x))))
=======
  #ifdef HAS_CPU_64
	#define	Flip(x,P,O)	((unsigned long long) _mm_cvtsi128_si64(mm_Flip(_mm_insert_epi64(_mm_cvtsi64_si128(P), (O), 1), (x))))
  #else
	#define	Flip(x,P,O)	((unsigned long long) _mm_cvtsi128_si64(mm_Flip(_mm_insert_epi32(_mm_insert_epi32(_mm_insert_epi32(\
		_mm_cvtsi32_si128(P), ((P) >> 32), 1), (O), 2), (O >> 32), 3), (x))))
  #endif
>>>>>>> be2ba1c (add AVX get_potential_mobility; revise foreach_bit for CPU32/C99)
	#define	board_flip(board,x)	((unsigned long long) _mm_cvtsi128_si64(mm_Flip(_mm_loadu_si128((__m128i *) (board)), (x))))

#elif MOVE_GENERATOR == MOVE_GENERATOR_SSE
	extern __m128i (vectorcall *mm_flip[BOARD_SIZE + 2])(const __m128i);
	#define	Flip(x,P,O)	((unsigned long long) _mm_cvtsi128_si64(mm_flip[x](_mm_unpacklo_epi64(_mm_cvtsi64_si128(P), _mm_cvtsi64_si128(O)))))
	#define mm_Flip(OP,x)	mm_flip[x](OP)
	#define	board_flip(board,x)	((unsigned long long) _mm_cvtsi128_si64(mm_flip[x](_mm_loadu_si128((__m128i *) (board)))))

<<<<<<< HEAD
<<<<<<< HEAD
#elif MOVE_GENERATOR == MOVE_GENERATOR_SSE_BSWAP
	extern unsigned long long flip(int, const unsigned long long, const unsigned long long);
	#define	Flip(x,P,O)	flip((x), (P), (O))
	#define	board_flip(board,x)	flip((x), (board)->player, (board)->opponent)

>>>>>>> 6506166 (More SSE optimizations)
=======
>>>>>>> 569c1f8 (More neon optimizations; split bit_intrinsics.h from bit.h)
=======
#elif MOVE_GENERATOR == MOVE_GENERATOR_NEON
	extern uint64x2_t mm_Flip(uint64x2_t OP, int pos);
	#define	Flip(x,P,O)	vgetq_lane_u64(mm_Flip(vcombine_u64(vcreate_u64(P), vcreate_u64(O)), (x)), 0)
	#define	board_flip(board,x)	vgetq_lane_u64(mm_Flip(vld1q_u64((uint64_t *) (board)), (x)), 0)

>>>>>>> 343493d (More neon/sse optimizations; neon dispatch added for arm32)
#elif MOVE_GENERATOR == MOVE_GENERATOR_32
	extern unsigned long long (*flip[BOARD_SIZE + 2])(unsigned int, unsigned int, unsigned int, unsigned int);
	#define Flip(x,P,O)	flip[x]((unsigned int)(P), (unsigned int)((P) >> 32), (unsigned int)(O), (unsigned int)((O) >> 32))
  #ifdef __BIG_ENDIAN__
	#define	board_flip(board,x)	flip[x]((unsigned int)((board)->player), ((unsigned int *) &(board)->player)[0], (unsigned int)((board)->opponent), ((unsigned int *) &(board)->opponent)[0])
  #else
	#define	board_flip(board,x)	flip[x]((unsigned int)((board)->player), ((unsigned int *) &(board)->player)[1], (unsigned int)((board)->opponent), ((unsigned int *) &(board)->opponent)[1])
  #endif
	#if defined(USE_GAS_MMX) && !defined(hasSSE2)
		extern void init_flip_sse(void);
	#endif
<<<<<<< HEAD
>>>>>>> 1b29848 (fix & optimize 32 bit build; other minor mods)
=======

<<<<<<< HEAD
<<<<<<< HEAD
>>>>>>> 6506166 (More SSE optimizations)
=======
#elif MOVE_GENERATOR == MOVE_GENERATOR_NEON
	extern unsigned long long Flip(int , unsigned long long, unsigned long long);

>>>>>>> f2da03e (Refine arm builds adding neon support.)
#else
  #if MOVE_GENERATOR == MOVE_GENERATOR_SSE_BSWAP
	extern unsigned long long Flip(int, unsigned long long, unsigned long long);
  #else
	extern unsigned long long (*flip[BOARD_SIZE + 2])(const unsigned long long, const unsigned long long);
	#define	Flip(x,P,O)	flip[x]((P), (O))
<<<<<<< HEAD
  #endif

	#define	board_flip(board,x)	Flip((x), (board)->player, (board)->opponent)
#endif

#ifndef vboard_flip
	#define	vboard_flip(vboard,x)	board_flip(&(vboard).board, (x))
#endif

// Use backup copy of search->board in a vector register if available (assume *pboard == vboard on entry)
#ifdef hasSSE2
	#define	vboard_update(pboard,vboard,move)	_mm_storeu_si128((__m128i *) (pboard), _mm_shuffle_epi32(_mm_xor_si128((vboard).v2, _mm_or_si128(_mm_set1_epi64x((move)->flipped), _mm_loadl_epi64((__m128i *) &X_TO_BIT[move->x]))), 0x4e))
#else
	#define	vboard_update(pboard,vboard,move)	board_update((pboard), (move))
#endif

// Pass Board in a vector register to Flip
#if (MOVE_GENERATOR == MOVE_GENERATOR_AVX) || (MOVE_GENERATOR == MOVE_GENERATOR_AVX512) || (MOVE_GENERATOR == MOVE_GENERATOR_SSE)
	unsigned long long vectorcall board_next_sse(__m128i OP, const int x, Board *next);
	#define	board_next(board,x,next)	board_next_sse(_mm_loadu_si128((__m128i *) (board)), (x), (next))
	#define vboard_next(vboard,x,next)	board_next_sse((vboard).v2, (x), (next))
#elif MOVE_GENERATOR == MOVE_GENERATOR_NEON
	unsigned long long board_next_neon(uint64x2_t OP, const int x, Board *next);
	#define	board_next(board,x,next)	board_next_neon(vld1q_u64((uint64_t *) (board)), (x), (next))
	#define vboard_next(vboard,x,next)	board_next_neon((vboard).v2, (x), (next))
#else
	unsigned long long board_next(const Board *board, const int x, Board *next);
	#define	vboard_next(vboard,x,next)	board_next(&(vboard).board, (x), (next))
#endif

// Pass vboard to get_moves if vectorcall available, otherwise board
#if defined(__AVX2__) && (defined(_MSC_VER) || defined(__linux__))
	unsigned long long vectorcall get_moves_avx(__m256i PP, __m256i OO);
	#define	get_moves(P,O)	get_moves_avx(_mm256_set1_epi64x(P), _mm256_set1_epi64x(O))
	#define	board_get_moves(board)	get_moves_avx(_mm256_set1_epi64x((board)->player), _mm256_set1_epi64x((board)->opponent))
	#define	vboard_get_moves(vboard)	get_moves_avx(_mm256_broadcastq_epi64((vboard).v2), _mm256_broadcastq_epi64(_mm_unpackhi_epi64((vboard).v2, (vboard).v2)))
#else
	unsigned long long get_moves(const unsigned long long, const unsigned long long);
	#define	board_get_moves(board)	get_moves((board)->player, (board)->opponent)
	#define	vboard_get_moves(vboard)	get_moves((vboard).board.player, (vboard).board.opponent)
=======
	#define	board_flip(board,x)	flip[x]((board)->player, (board)->opponent)
>>>>>>> 6506166 (More SSE optimizations)
=======
#else
	#if MOVE_GENERATOR == MOVE_GENERATOR_SSE_BSWAP
		extern unsigned long long Flip(int, unsigned long long, unsigned long long);
	#else
		extern unsigned long long (*flip[BOARD_SIZE + 2])(const unsigned long long, const unsigned long long);
		#define	Flip(x,P,O)	flip[x]((P), (O))
	#endif

	#define	board_flip(board,x)	Flip((x), (board)->player, (board)->opponent)
>>>>>>> 569c1f8 (More neon optimizations; split bit_intrinsics.h from bit.h)
#endif

#if (MOVE_GENERATOR == MOVE_GENERATOR_AVX) || (MOVE_GENERATOR == MOVE_GENERATOR_AVX512) || (MOVE_GENERATOR == MOVE_GENERATOR_SSE)
	#define	vBoard	__m128i
	unsigned long long vectorcall vboard_next(__m128i OP, const int x, Board *next);
	#define	board_next(board,x,next)	vboard_next(_mm_loadu_si128((__m128i *) (board)), (x), (next))
	#define	load_vboard(board)	_mm_loadu_si128((__m128i *) &(board))
	#define	store_vboard(dst,board)	_mm_storeu_si128((__m128i *) &(dst), (board))
#elif MOVE_GENERATOR == MOVE_GENERATOR_NEON
	#define	vBoard	uint64x2_t
	unsigned long long vboard_next(uint64x2_t OP, const int x, Board *next);
	#define	board_next(board,x,next)	vboard_next(vld1q_u64((uint64_t *) (board)), (x), (next))
	#define	load_vboard(board)	vld1q_u64((uint64_t *) &(board))
	#define	store_vboard(dst,board)	vst1q_u64((uint64_t *) &(dst), (board))
#else
	#define	vBoard	Board
	unsigned long long board_next(const Board *board, const int x, Board *next);
	#define	vboard_next(board,x,next)	board_next(&(board), (x), (next))
	#define	load_vboard(board)	(board)
	#define	store_vboard(dst,board)	((dst) = (board))
#endif

#if defined(__AVX2__) && (defined(_MSC_VER) || defined(__clang__))
	unsigned long long vectorcall get_moves_avx(__m256i PP, __m256i OO);
	#define	get_moves(P,O)	get_moves_avx(_mm256_broadcastq_epi64(_mm_cvtsi64_si128(P)), _mm256_broadcastq_epi64(_mm_cvtsi64_si128(O)))
	#define	vboard_get_moves(vboard,board)	get_moves_avx(_mm256_broadcastq_epi64(vboard), _mm256_permute4x64_epi64(_mm256_castsi128_si256(vboard), 0x55))
#else
	unsigned long long get_moves(const unsigned long long, const unsigned long long);
	#define	vboard_get_moves(vboard,board)	get_moves((board).player, (board).opponent)
#endif

#endif
