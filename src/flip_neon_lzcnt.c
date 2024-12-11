/**
 * @file flip_neon_lzcnt.c
 *
 * This module deals with flipping discs.
 *
 * For LSB to MSB directions, isolate LS1B can be used to determine
 * contiguous opponent discs.
 * For MSB to LSB directions, LZCNT is used to isolate MS1B.
 *
 * @date 2024
 * @author Richard Delorme
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
	uint64x2_t	flip, oflank0, mask0;				uint64x2_t	oflank1, mask1;
	int32x4_t	clz0;						int32x4_t	clz1;
	uint32x4_t	msb0;						uint32x4_t	msb1;
	const uint64x2_t one = vdupq_n_u64(1);
	uint64x2_t PP = vdupq_lane_u64(vget_low_u64(OP), 0);
	uint64x2_t OO = vdupq_lane_u64(vget_high_u64(OP), 0);

	mask0 = MASK_LR_v4[pos][2];					mask1 = MASK_LR_v4[pos][3];
	// isolate non-opponent MS1B
	oflank0 = vbicq_u64(mask0, OO);					oflank1 = vbicq_u64(mask1, OO);
	// outflank = (0x8000000000000000ULL >> lzcnt) & P
	clz0 = vclzq_s32(vreinterpretq_s32_u64(oflank0));		clz1 = vclzq_s32(vreinterpretq_s32_u64(oflank1));
	// set loword's MSB if hiword = 0
	msb0 = vreinterpretq_u32_u64(vshrq_n_u64(oflank0, 32));		msb1 = vreinterpretq_u32_u64(vshrq_n_u64(oflank1, 32));
	msb0 = vshlq_n_u32(vceqzq_u32(msb0), 31);			msb1 = vshlq_n_u32(vceqzq_u32(msb1), 31);
	msb0 = vshlq_u32(msb0, vnegq_s32(clz0));			msb1 = vshlq_u32(msb1, vnegq_s32(clz1));
	// 0 if outflank is P, otherwise oflank = msb
	oflank0 = vbicq_u64(vreinterpretq_u64_u32(msb0), PP);		oflank1 = vbicq_u64(vreinterpretq_u64_u32(msb1), PP);
	// set all bits higher than outflank
	oflank0 = vsubq_u64(oflank0, vreinterpretq_u64_u32(msb0));	oflank1 = vsubq_u64(oflank1, vreinterpretq_u64_u32(msb1));
	flip = vandq_u64(vbslq_u64(mask1, oflank1, vandq_u64(mask0, oflank0)), OO);

	mask0 = MASK_LR_v4[pos][0];					mask1 = MASK_LR_v4[pos][1];
	// get outflank with carry-propagation
	oflank0 = vaddq_u64(vornq_u64(OO, mask0), one);			oflank1 = vaddq_u64(vornq_u64(OO, mask1), one);
	oflank0 = vandq_u64(vandq_u64(PP, mask0), oflank0);		oflank1 = vandq_u64(vandq_u64(PP, mask1), oflank1);
	// set all bits lower than oflank, using satulation if oflank = 0
	oflank0 = vqsubq_u64(oflank0, one);				oflank1 = vqsubq_u64(oflank1, one);
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

