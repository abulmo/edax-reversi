/**
 * @file endgame_neon.c
 *
 *
 * Arm Neon optimized version of endgame.c for the last four empties.
 *
 * Bitboard and empty list is kept in Neon registers.
 *
 * @date 1998 - 2024
 * @author Richard Delorme
 * @author Toshihiko Okuhara
 * @version 4.5
 * 
 */

#include "bit_intrinsics.h"
#include "settings.h"
#include "search.h"
#include <assert.h>

#define TESTZ_FLIP(X)	(!vgetq_lane_u64((X), 0))

#ifndef HAS_CPU_64
	#define vaddv_u8(x)	vget_lane_u32(vreinterpret_u32_u64(vpaddl_u32(vpaddl_u16(vpaddl_u8(x)))), 0)
#endif

// in count_last_flip_neon.c
extern const unsigned char COUNT_FLIP[8][256];
extern const uint64x2_t mask_dvhd[64][2];

/**
 * @brief Compute a board resulting of a move played on a previous board.
 *
 * @param OP board to play the move on.
 * @param x move to play.
 * @param flipped flipped returned from mm_Flip.
 * @return resulting board.
 */
static inline uint64x2_t board_flip_next(uint64x2_t OP, int x, uint64x2_t flipped)
{
#if !defined(_MSC_VER) && !defined(__clang__)	// MSVC-arm32 does not have vld1q_lane_u64
	// arm64-gcc-13: 8, armv8a-clang-16: 8, msvc-arm64-19: 8, gcc-arm-13: 16, clang-armv7-11: 18
	OP = veorq_u64(OP, vorrq_u64(flipped, vld1q_lane_u64((uint64_t *) &X_TO_BIT[x], flipped, 0)));
	return vextq_u64(OP, OP, 1);
#else	// arm64-gcc-13: 8, armv8a-clang-16: 7, msvc-arm64-19: 7, gcc-arm-13: 21, clang-armv7-11: 15
	OP = veorq_u64(OP, flipped);
	return vcombine_u64(vget_high_u64(OP), vorr_u64(vget_low_u64(OP), vld1_u64((uint64_t *) &X_TO_BIT[x])));
#endif
}

/**
 * @brief Get the final score.
 *
 * Get the final score, when no move can be made.
 *
 * @param P Board.player
 * @param n_empties Number of empty squares remaining on the board.
 * @return The final score, as a disc difference.
 */
static int board_solve_neon(uint64x1_t P, int n_empties)
{
	int score = vaddv_u8(vcnt_u8(vreinterpret_u8_u64(P))) * 2 - SCORE_MAX;	// in case of opponents win
	int diff = score + n_empties;		// = n_discs_p - (64 - n_empties - n_discs_p)

	SEARCH_STATS(++statistics.n_search_solve);

	if (diff == 0)
		score = diff;
	else if (diff > 0)
		score = diff + n_empties;
	return score;
}

/**
 * @brief Get the final score.
 *
 * Get the final score, when 1 empty square remain.
 * The original code has been adapted from Zebra by Gunnar Anderson.
 *
 * @param P      Board.player to evaluate.
 * @param alpha  Alpha bound. (beta - 1)
 * @param pos    Last empty square to play.
 * @return       The final score, as a disc difference.
 */
#if (LAST_FLIP_COUNTER == COUNT_LAST_FLIP_SVE) && defined(SIMULLASTFLIP)

#include "arm_sve.h"

extern	const uint64_t lrmask[66][8];	// in flip_sve_lzcnt.c

  #ifndef __ARM_FEATURE_SVE2
		// equivalent only if no intersection between masks
	#define svbsl_u64(op1,op2,op3)	svorr_u64_m(pg, (op2), svand_u64_x(pg, (op3), (op1)))
	#define svbsl1n_u64(op1,op2,op3)	svorr_u64_m(pg, (op2), svbic_u64_x(pg, (op3), (op1)))
  #endif

static int board_score_neon_1(uint64x1_t P, int alpha, int pos)
{
	int	score;
	svuint64_t	PP, p_flip, o_flip, p_oflank, o_oflank, p_eraser, o_eraser, p_cap, o_cap, mask, po_flip, po_score;
	svbool_t	pg, po_nopass;
	const uint64_t	(*pmask)[8];

	PP = svdup_u64(vget_lane_u64(P, 0));
	pmask = &lrmask[pos];
	pg = svwhilelt_b64(0, 4);

	mask = svld1_u64(pg, *pmask + 4);	// right: clear all bits lower than outflank
	p_oflank = svand_x(pg, mask, PP);			o_oflank = svbic_x(pg, mask, PP);
	p_oflank = svand_x(pg, svclz_z(pg, p_oflank), 63);	o_oflank = svand_x(pg, svclz_z(pg, o_oflank), 63);
	p_eraser = svlsr_x(pg, svdup_u64(-1), p_oflank);	o_eraser = svlsr_x(pg, svdup_u64(-1), o_oflank);
	p_flip = svbic_x(pg, mask, p_eraser);			o_flip = svbic_x(pg, mask, o_eraser);

	mask = svld1_u64(pg, *pmask + 0);	// left: look for player LS1B
	p_oflank = svand_x(pg, mask, PP);			o_oflank = svbic_x(pg, mask, PP);
		// set all bits lower than oflank, using satulation if oflank = 0
	p_cap = svbic_x(pg, svqsub(p_oflank, 1), p_oflank);	o_cap = svbic_x(pg, svqsub(o_oflank, 1), o_oflank);
	p_flip = svbsl_u64(p_cap, p_flip, mask);		o_flip = svbsl_u64(o_cap, o_flip, mask);

	if (svcntd() == 2) {	// sve128 only
		mask = svld1_u64(pg, *pmask + 6);	// right: set all bits higher than outflank
		p_oflank = svand_x(pg, mask, PP);			o_oflank = svbic_x(pg, mask, PP);
		p_oflank = svand_x(pg, svclz_z(pg, p_oflank), 63);	o_oflank = svand_x(pg, svclz_z(pg, o_oflank), 63);
		p_eraser = svlsr_x(pg, svdup_u64(-1), p_oflank);	o_eraser = svlsr_x(pg, svdup_u64(-1), o_oflank);
		p_flip = svbsl1n_u64(p_eraser, p_flip, mask);		o_flip = svbsl1n_u64(o_eraser, o_flip, mask);

		mask = svld1_u64(pg, *pmask + 2);	// left: look for player LS1B
		p_oflank = svand_x(pg, mask, PP);			o_oflank = svbic_x(pg, mask, PP);
			// set all bits lower than oflank, using satulation if oflank = 0
		p_cap = svbic_x(pg, svqsub(p_oflank, 1), p_oflank);	o_cap = svbic_x(pg, svqsub(o_oflank, 1), o_oflank);
		p_flip = svbsl_u64(p_cap, p_flip, mask);		o_flip = svbsl_u64(o_cap, o_flip, mask);
	}

	po_flip = svtrn1_u64(svdup_u64(svorv_u64(pg, o_flip)), svdup_u64(svorv_u64(pg, p_flip)));
	po_nopass = svcmpne_n_u64(pg, po_flip, 0);
	po_score = svcnt_u64_x(pg, sveor_u64_x(pg, po_flip, PP));
		// last square for O if P pass and (not O pass or score < 32)
	po_score = svsub_n_u64_m(svorr_b_z(svptrue_pat_b64(SV_VL1), svcmplt_n_u64(pg, po_score, 32), po_nopass), po_score, 1);
	score = svlastb_u64(svorr_b_z(pg, po_nopass, svptrue_pat_b64(SV_VL1)), po_score);	// use o_score if p_pass
	(void) alpha;	// no lazy cut-off
	return score * 2 - SCORE_MAX + 2;	// = (bit_count(P) + 1) - (SCORE_MAX - 1 - bit_count(P))
}

#else
static int board_score_neon_1(uint64x1_t P, int alpha, int pos)
{
	int	score = 2 * vaddv_u8(vcnt_u8(vreinterpret_u8_u64(P))) - SCORE_MAX + 2;	// = (bit_count(P) + 1) - (SCORE_MAX - 1 - bit_count(P))
	int	score2;
	unsigned int	n_flips, m;
	const unsigned char *COUNT_FLIP_X = COUNT_FLIP[pos & 7];
	const unsigned char *COUNT_FLIP_Y = COUNT_FLIP[pos >> 3];
	uint64x2_t	PP = vdupq_lane_u64(P, 0);
	uint64x2_t	I0, I1;
	static const unsigned short o_mask[64] = {
		0xff01, 0x7f03, 0x3f07, 0x1f0f, 0x0f1f, 0x073f, 0x037f, 0x01ff,
		0xfe03, 0xff07, 0x7f0f, 0x3f1f, 0x1f3f, 0x0f7f, 0x07ff, 0x03fe,
		0xfc07, 0xfe0f, 0xff1f, 0x7f3f, 0x3f7f, 0x1fff, 0x0ffe, 0x07fc,
		0xf80f, 0xfc1f, 0xfe3f, 0xff7f, 0x7fff, 0x3ffe, 0x1ffc, 0x0ff8,
		0xf01f, 0xf83f, 0xfc7f, 0xfeff, 0xfffe, 0x7ffc, 0x3ff8, 0x1ff0,
		0xe03f, 0xf07f, 0xf8ff, 0xfcfe, 0xfefc, 0xfff8, 0x7ff0, 0x3fe0,
		0xc07f, 0xe0ff, 0xf0fe, 0xf8fc, 0xfcf8, 0xfef0, 0xffe0, 0x7fc0,
		0x80ff, 0xc0fe, 0xe0fc, 0xf0f8, 0xf8f0, 0xfce0, 0xfec0, 0xff80
	};

	// n_flips = last_flip(pos, P);
  #ifdef HAS_CPU_64	// vaddvq
	unsigned int t0, t1;
	const uint64x2_t dmask = { 0x0808040402020101, 0x8080404020201010 };

	PP = vreinterpretq_u64_u8(vzip1q_u8(vreinterpretq_u8_u64(PP), vreinterpretq_u8_u64(PP)));
	I0 = vandq_u64(PP, mask_dvhd[pos][0]);	// 2 dirs interleaved
	t0 = vaddvq_u16(vreinterpretq_u16_u64(I0));
	n_flips  = COUNT_FLIP_X[t0 >> 8];
	n_flips += COUNT_FLIP_X[t0 & 0xFF];
	I1 = vandq_u64(vreinterpretq_u64_u8(vtstq_u8(vreinterpretq_u8_u64(PP), vreinterpretq_u8_u64(mask_dvhd[pos][1]))), dmask);
	t1 = vaddvq_u16(vreinterpretq_u16_u64(I1));
	n_flips += COUNT_FLIP_Y[t1 >> 8];
	n_flips += COUNT_FLIP_Y[t1 & 0xFF];

  #else // Neon kindergarten
	const uint64x2_t dmask = { 0x1020408001020408, 0x1020408001020408 };

	I0 = vpaddlq_u32(vpaddlq_u16(vpaddlq_u8(vreinterpretq_u8_u64(vandq_u64(PP, mask_dvhd[pos][0])))));
	n_flips  = COUNT_FLIP_X[vgetq_lane_u32(vreinterpretq_u32_u64(I0), 2)];
	n_flips += COUNT_FLIP_X[vgetq_lane_u32(vreinterpretq_u32_u64(I0), 0)];
	I1 = vreinterpretq_u64_s8(vnegq_s8(vreinterpretq_s8_u8(vtstq_u8(vreinterpretq_u8_u64(PP), vreinterpretq_u8_u64(mask_dvhd[pos][1])))));
	I1 = vpaddlq_u32(vmulq_u32(vreinterpretq_u32_u64(dmask), vreinterpretq_u32_u64(I1)));
	n_flips += COUNT_FLIP_Y[vgetq_lane_u8(vreinterpretq_u8_u64(I1), 11)];
	n_flips += COUNT_FLIP_Y[vgetq_lane_u8(vreinterpretq_u8_u64(I1), 3)];
  #endif
	score += n_flips;

	if (n_flips == 0) {
		score2 = score - 2;	// empty for opponent
		if (score <= 0)
			score = score2;

		if (score > alpha) {	// lazy cut-off
			// n_flips = last_flip(pos, O);
			m = o_mask[pos];	// valid diagonal bits
  #ifdef HAS_CPU_64
			n_flips  = COUNT_FLIP_X[(t0 >> 8) ^ 0xFF];
			n_flips += COUNT_FLIP_X[(t0 ^ m) & 0xFF];
			n_flips += COUNT_FLIP_Y[(t1 ^ m) >> 8];
			n_flips += COUNT_FLIP_Y[(~t1) & 0xFF];
  #else
			n_flips  = COUNT_FLIP_X[vgetq_lane_u32(vreinterpretq_u32_u64(I0), 2) ^ 0xFF];
			n_flips += COUNT_FLIP_X[vgetq_lane_u32(vreinterpretq_u32_u64(I0), 0) ^ (m & 0xFF)];
			n_flips += COUNT_FLIP_Y[vgetq_lane_u8(vreinterpretq_u8_u64(I1), 11) ^ (m >> 8)];
			n_flips += COUNT_FLIP_Y[vgetq_lane_u8(vreinterpretq_u8_u64(I1), 3) ^ 0xFF];
  #endif
			if (n_flips != 0)
				score = score2 - n_flips;
		}
	}

	return score;
}
#endif

// from bench.c
int board_score_1(const unsigned long long player, const int beta, const int x)
{
	return board_score_neon_1(vcreate_u64(player), beta, x);
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
static int board_solve_2(uint64x2_t OP, int alpha, volatile unsigned long long *n_nodes, uint8x8_t empties)
{
	uint64x2_t flipped;
	int score, bestscore, nodes;
	int x1 = vget_lane_u8(empties, 1);
	int x2 = vget_lane_u8(empties, 0);
	unsigned long long opponent;
	// const int beta = alpha + 1;

	SEARCH_STATS(++statistics.n_board_solve_2);

	opponent = vgetq_lane_u64(OP, 1);
	if ((NEIGHBOUR[x1] & opponent) && !TESTZ_FLIP(flipped = mm_Flip(OP, x1))) {
		bestscore = board_score_neon_1(vget_high_u64(veorq_u64(OP, flipped)), alpha, x2);

		if ((bestscore > alpha) && (NEIGHBOUR[x2] & opponent) && !TESTZ_FLIP(flipped = mm_Flip(OP, x2))) {
			score = board_score_neon_1(vget_high_u64(veorq_u64(OP, flipped)), alpha, x1);
			if (score < bestscore)
				bestscore = score;
			nodes = 3;
		} else	nodes = 2;

	} else if ((NEIGHBOUR[x2] & opponent) && !TESTZ_FLIP(flipped = mm_Flip(OP, x2))) {
		bestscore = board_score_neon_1(vget_high_u64(veorq_u64(OP, flipped)), alpha, x1);
		nodes = 2;

	} else {	// pass - NEIGHBOUR test is almost 100% true
		alpha = ~alpha;	// = -alpha - 1
		OP = vextq_u64(OP, OP, 1);
		if (!TESTZ_FLIP(flipped = mm_Flip(OP, x1))) {
			bestscore = board_score_neon_1(vget_high_u64(veorq_u64(OP, flipped)), alpha, x2);

			if ((bestscore > alpha) && !TESTZ_FLIP(flipped = mm_Flip(OP, x2))) {
				score = board_score_neon_1(vget_high_u64(veorq_u64(OP, flipped)), alpha, x1);
				if (score < bestscore)
					bestscore = score;
				nodes = 3;
			} else	nodes = 2;

		} else if (!TESTZ_FLIP(flipped = mm_Flip(OP, x2))) {
			bestscore = board_score_neon_1(vget_high_u64(veorq_u64(OP, flipped)), alpha, x1);
			nodes = 2;

		} else {	// gameover
			bestscore = board_solve_neon(vget_high_u64(OP), 2);
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
static int search_solve_3(uint64x2_t OP, int alpha, volatile unsigned long long *n_nodes, uint8x8_t empties)
{
	uint64x2_t flipped;
	int score, bestscore, x, pol;
	unsigned long long opponent;

	SEARCH_STATS(++statistics.n_search_solve_3);
	SEARCH_UPDATE_INTERNAL_NODES(*n_nodes);

	bestscore = -SCORE_INF;
	pol = 1;
	do {
		// best move alphabeta search
		opponent = vgetq_lane_u64(OP, 1);
		x = vget_lane_u8(empties, 2);
		if ((NEIGHBOUR[x] & opponent) && !TESTZ_FLIP(flipped = mm_Flip(OP, x))) {
			bestscore = board_solve_2(board_flip_next(OP, x, flipped), alpha, n_nodes, empties);
			if (bestscore > alpha) return bestscore * pol;
		}

		x = vget_lane_u8(empties, 1);
		if (/* (NEIGHBOUR[x] & opponent) && */ !TESTZ_FLIP(flipped = mm_Flip(OP, x))) {
			score = board_solve_2(board_flip_next(OP, x, flipped), alpha, n_nodes, vuzp_u8(empties, empties).val[0]);	// d2d0
			if (score > alpha) return score * pol;
			else if (score > bestscore) bestscore = score;
		}

		x = vget_lane_u8(empties, 0);
		if (/* (NEIGHBOUR[x] & opponent) && */ !TESTZ_FLIP(flipped = mm_Flip(OP, x))) {
			score = board_solve_2(board_flip_next(OP, x, flipped), alpha, n_nodes, vext_u8(empties, empties, 1));	// d2d1
			if (score > bestscore) bestscore = score;
			return bestscore * pol;
		}

		if (bestscore > -SCORE_INF)
			return bestscore * pol;

		OP = vextq_u64(OP, OP, 1);	// pass
		alpha = ~alpha;	// = -(alpha + 1)
	} while ((pol = -pol) < 0);

	return board_solve_neon(vget_low_u64(OP), 3);	// gameover
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

static int search_solve_4(Search *search, int alpha)
{
	uint64x2_t	OP, flipped;
	uint8x16_t	empties_series;	// B15:4th, B11:3rd, B7:2nd, B3:1st, lower 3 bytes for 3 empties
	uint8x16_t	shuf;
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
	static const uint64x2_t shuf_mask[] = {
		{ 0x0203010003020100, 0x0003020101030200 },	//  0: 1(x1) 3(x2 x3 x4), 1(x1) 1(x2) 2(x3 x4), 1 1 1 1, 4
		{ 0x0203010003020100, 0x0002030101020300 },	//  1: 1(x2) 3(x1 x3 x4)
		{ 0x0201030003010200, 0x0001030201030200 },	//  2: 1(x3) 3(x1 x2 x4)
		{ 0x0200030103000201, 0x0003020101000302 },	//  3: 1(x4) 3(x1 x2 x3)
		{ 0x0103020003010200, 0x0003020102030100 },	//  4: 1(x1) 1(x3) 2(x2 x4)	x4x1x2x3-x2x1x3x4-x3x1x2x4-x1x3x2x4
		{ 0x0003020103000201, 0x0103020002030100 },	//  5: 1(x1) 1(x4) 2(x2 x3)	x3x1x2x4-x2x1x3x4-x4x1x2x3-x1x4x2x3
		{ 0x0102030002010300, 0x0003020103020100 },	//  6: 1(x2) 1(x3) 2(x1 x4)	x4x1x2x3-x1x2x3x4-x3x2x1x4-x2x3x1x4
		{ 0x0002030102000301, 0x0103020003020100 },	//  7: 1(x2) 1(x4) 2(x1 x3)	x3x1x2x4-x1x2x3x4-x4x2x1x3-x2x4x1x3
		{ 0x0001030201000302, 0x0203010003020100 },	//  8: 1(x3) 1(x4) 2(x1 x2)	x2x1x3x4-x1x2x3x4-x4x3x1x2-x3x4x1x2
		{ 0x0203010003020100, 0x0001030201000302 },	//  9: 2(x1 x2) 2(x3 x4)	x4x3x1x2-x3x4x1x2-x2x1x3x4-x1x2x3x4
		{ 0x0200030103010200, 0x0002030101030200 },	// 10: 2(x1 x3) 2(x2 x4)	x4x2x1x3-x3x1x2x4-x2x4x1x3-x1x3x2x4
		{ 0x0201030003000201, 0x0003020101020300 }	// 11: 2(x1 x4) 2(x2 x3)	x4x1x2x3-x3x2x1x4-x2x3x1x4-x1x4x2x3
	};

	SEARCH_STATS(++statistics.n_search_solve_4);
	SEARCH_UPDATE_INTERNAL_NODES(search->n_nodes);

	// stability cutoff (try 12%, cut 7%)
	if (search_SC_NWS_4(search, alpha, &score)) return score;

	OP = vld1q_u64((uint64_t *) &search->board);
	x1 = search->empties[NOMOVE].next;
	x2 = search->empties[x1].next;
	x3 = search->empties[x2].next;
	x4 = search->empties[x3].next;

	// parity based move sorting.
	// The following hole sizes are possible:
	//    4 - 1 3 - 2 2 - 1 1 2 - 1 1 1 1
	// Only the 1 1 2 case needs move sorting on this ply.
	empties_series = vreinterpretq_u8_u32(vdupq_n_u32((x1 << 24) | (x2 << 16) | (x3 << 8) | x4));
	paritysort = parity_case[((x3 ^ x4) & 0x24) + (((x2 ^ x4) & 0x24) >> 1) + (((x1 ^ x4) & 0x24) >> 2)];
	shuf = vreinterpretq_u8_u64(shuf_mask[paritysort]);
#ifdef HAS_CPU_64
	empties_series = vqtbl1q_u8(empties_series, shuf);
#else
	empties_series = vcombine_u8(vtbl1_u8(vget_low_u8(empties_series), vget_low_u8(shuf)),
		vtbl1_u8(vget_low_u8(empties_series), vget_high_u8(shuf)));
#endif

	bestscore = SCORE_INF;	// min stage
	pol = 1;
	do {
		// best move alphabeta search
		opponent = vgetq_lane_u64(OP, 1);
		x1 = vgetq_lane_u8(empties_series, 3);
		if ((NEIGHBOUR[x1] & opponent) && !TESTZ_FLIP(flipped = mm_Flip(OP, x1))) {
			bestscore = search_solve_3(board_flip_next(OP, x1, flipped), alpha, &search->n_nodes,
				vget_low_u8(empties_series));
			if (bestscore <= alpha) return bestscore * pol;
		}

		x2 = vgetq_lane_u8(empties_series, 7);
		if ((NEIGHBOUR[x2] & opponent) && !TESTZ_FLIP(flipped = mm_Flip(OP, x2))) {
			score = search_solve_3(board_flip_next(OP, x2, flipped), alpha, &search->n_nodes,
				vget_low_u8(vextq_u8(empties_series, empties_series, 4)));
			if (score <= alpha) return score * pol;
			else if (score < bestscore) bestscore = score;
		}

		x3 = vgetq_lane_u8(empties_series, 11);
		if ((NEIGHBOUR[x3] & opponent) && !TESTZ_FLIP(flipped = mm_Flip(OP, x3))) {
			score = search_solve_3(board_flip_next(OP, x3, flipped), alpha, &search->n_nodes,
				vget_high_u8(empties_series));
			if (score <= alpha) return score * pol;
			else if (score < bestscore) bestscore = score;
		}

		x4 = vgetq_lane_u8(empties_series, 15);
		if ((NEIGHBOUR[x4] & opponent) && !TESTZ_FLIP(flipped = mm_Flip(OP, x4))) {
			score = search_solve_3(board_flip_next(OP, x4, flipped), alpha, &search->n_nodes,
				vget_low_u8(vextq_u8(empties_series, empties_series, 12)));
			if (score < bestscore) bestscore = score;
			return bestscore * pol;
		}

		if (bestscore < SCORE_INF)
			return bestscore * pol;

		OP = vextq_u64(OP, OP, 1);	// pass
		alpha = ~alpha;	// = -(alpha + 1)
	} while ((pol = -pol) < 0);

	return board_solve_neon(vget_high_u64(OP), 4);	// gameover
}
