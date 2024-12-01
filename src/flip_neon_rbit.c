/**
 * @file flip_neon_rbit.c
 *
 * This module deals with flipping discs.
 *
 * For LSB to MSB directions, carry propagation can be used to determine
 * contiguous opponent discs.
 * For MSB to LSB directions, rev64/rbit is used to use carry propagation.
 *
 * @date 2024
 * @author Toshihiko Okuhara
 * @version 4.6
 */

#include "simd.h"

extern const uint64x2_t MASK_LR_v4[66][4];

/**
 * Compute flipped discs when playing on square pos.
 *
 * @param pos player's move.
 * @param P player's disc pattern.
 * @param O opponent's disc pattern.
 * @return flipped disc pattern.
 */
uint64x2_t mm_flip(uint64x2_t OP, int pos)
{
	uint64x2_t	flip, oflank0, mask0, oflank1, mask1;
	const uint64x2_t one = vdupq_n_u64(1);
	uint64x2_t rOP = vreinterpretq_u64_u8(vrev64q_u8(vrbitq_u8(vreinterpretq_u8_u64(OP))));
	uint64x2_t PP = vdupq_lane_u64(vget_low_u64(OP), 0);	uint64x2_t rPP = vdupq_lane_u64(vget_low_u64(rOP), 0);
	uint64x2_t OO = vdupq_lane_u64(vget_high_u64(OP), 0);	uint64x2_t rOO = vdupq_lane_u64(vget_high_u64(rOP), 0);

	mask0 = MASK_LR_v4[pos][2];				mask1 = MASK_LR_v4[pos][3];
		// get outflank with carry-propagation
	oflank0 = vaddq_u64(vornq_u64(rOO, mask0), one);	oflank1 = vaddq_u64(vornq_u64(rOO, mask1), one);
	oflank0 = vandq_u64(vandq_u64(rPP, mask0), oflank0);	oflank1 = vandq_u64(vandq_u64(rPP, mask1), oflank1);
		// set all bits lower than oflank, using satulation if oflank = 0
	oflank0 = vqsubq_u64(oflank0, one);			oflank1 = vqsubq_u64(oflank1, one);
	flip = vbslq_u64(mask1, oflank1, vandq_u64(mask0, oflank0));
	flip = vreinterpretq_u64_u8(vrev64q_u8(vrbitq_u8(vreinterpretq_u8_u64(flip))));

	mask0 = MASK_LR_v4[pos][0];				mask1 = MASK_LR_v4[pos][1];
		// get outflank with carry-propagation
	oflank0 = vaddq_u64(vornq_u64(OO, mask0), one);		oflank1 = vaddq_u64(vornq_u64(OO, mask1), one);
	oflank0 = vandq_u64(vandq_u64(PP, mask0), oflank0);	oflank1 = vandq_u64(vandq_u64(PP, mask1), oflank1);
		// set all bits lower than oflank, using satulation if oflank = 0
	oflank0 = vqsubq_u64(oflank0, one);			oflank1 = vqsubq_u64(oflank1, one);
	flip = vbslq_u64(mask1, oflank1, vbslq_u64(mask0, oflank0, flip));

	return vorrq_u64(flip, vextq_u64(flip, flip, 1));
}

uint64_t board_flip(const Board *board, const int x) 
{
	uint64x2_t flip = mm_flip(vld1q_u64((uint64_t *) board), x);
	return vgetq_lane_u64(flip, 0);
}

uint64_t flip(const int x, const uint64_t P, const uint64_t O) {
	uint64x2_t OP = vcombine_u64(vcreate_u64(P), vcreate_u64(O));
	uint64x2_t flip = mm_flip(OP, x);
	return vgetq_lane_u64(flip, 0);
}	


