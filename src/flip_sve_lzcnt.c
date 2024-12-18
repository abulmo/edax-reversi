/**
 * @file flip_sve_lzcnt.c
 *
 * This module deals with flipping discs.
 *
 * For LSB to MSB directions, isolate LS1B can be used to determine
 * contiguous opponent discs.
 * For MSB to LSB directions, CLZ is used to isolate MS1B.
 *
 * @date 2024
 * @author Toshihiko Okuhara
 * @version 4.6
 */

#include "simd.h"
#include "arm_neon.h"

extern const V8DI MASK_LR[66]

/**
 * Compute flipped discs when playing on square pos.
 *
 * @param pos player's move.
 * @param P player's disc pattern.
 * @param O opponent's disc pattern.
 * @return flipped disc pattern.
 */

#ifndef __ARM_FEATURE_SVE2
	// equivalent only if no intersection between masks
	#define svbsl_u64(op1,op2,op3)	svorr_u64_m(pg, (op2), svand_u64_x(pg, (op3), (op1)))
#endif

uint64_t flip(const int pos, const uint64_t P, const uint64_t O)
{
	svuint64_t	PP, OO, flip, oflank, mask, msb;
	svbool_t	pg;
	const uint64_t	(*pmask)[8];
	

	PP = svdup_u64(P);
	OO = svdup_u64(O);
	msb = svdup_u64(0x8000000000000000);
	pmask = &MASK_LR[pos].v1;
	pg = svwhilelt_b64(0, 4);

	mask = svld1_u64(pg, *pmask + 4);
		// right: isolate non-opponent MS1B
	oflank = svbic_u64_x(pg, mask, OO);
		// outflank = (0x8000000000000000ULL >> lzcnt) & P
	oflank = svand_u64_x(pg, svlsr_u64_x(pg, msb, svclz_u64_x(pg, oflank)), PP);
		// set all bits higher than outflank
	oflank = svreinterpret_u64_s64(svneg_s64_x(pg, svreinterpret_s64_u64(oflank)));
	flip = svand_u64_x(pg, mask, oflank);

	mask = svld1_u64(pg, *pmask + 0);
		// left: look for non-opponent LS1B
	oflank = svbic_u64_x(pg, mask, OO);
	oflank = svand_u64_x(pg, svbic_u64_x(pg, oflank, svsub_n_u64_x(pg, oflank, 1)), PP);
		// set all bits lower than oflank, using satulation if oflank = 0
	flip = svbsl_u64(svqsub_n_u64(oflank, 1), flip, mask);

	if (svcntd() == 2) {	// sve128 only
		mask = svld1_u64(pg, *pmask + 6);
			// right: isolate non-opponent MS1B
		oflank = svbic_u64_x(pg, mask, OO);
			// outflank = (0x8000000000000000ULL >> lzcnt) & P
		oflank = svand_u64_x(pg, svlsr_u64_x(pg, msb, svclz_u64_x(pg, oflank)), PP);
			// set all bits higher than outflank
		oflank = svreinterpret_u64_s64(svneg_s64_x(pg, svreinterpret_s64_u64(oflank)));
  		flip = svbsl_u64(oflank, flip, mask);

		mask = svld1_u64(pg, *pmask + 2);
			// left: look for non-opponent LS1B
		oflank = svbic_u64_x(pg, mask, OO);
		oflank = svand_u64_x(pg, svbic_u64_x(pg, oflank, svsub_n_u64_x(pg, oflank, 1)), PP);
			// set all bits lower than oflank, using satulation if oflank = 0
		flip = svbsl_u64(svqsub_n_u64(oflank, 1), flip, mask);
	}

	return svorv_u64(pg, svand_u64_x(pg, flip, OO));
}


uint64_t board_flip(const Board *board, const int x)
{
	return flip(x, board->player, board->opponent);
}

