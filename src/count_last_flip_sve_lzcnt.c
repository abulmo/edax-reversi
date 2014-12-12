/**
 * @file count_last_flip_sve_lzcnt.c
 *
 * A function is provided to count the number of fipped disc of the last move.
 *
 * Count last flip using the flip_sve_lzcnt way.
 * For optimization purpose, the value returned is twice the number of flipped
 * disc, to facilitate the computation of disc difference.
 *
 * @date 2024
 * @author Toshihiko Okuhara
 * @version 4.5
 * 
 */

#include <arm_sve.h>

/** precomputed count flip array */
extern const unsigned char COUNT_FLIP[8][256];

/**
 * Count last flipped discs when playing on the last empty.
 *
 * @param pos the last empty square.
 * @param P player's disc pattern.
 * @return flipped disc count.
 */

#ifndef __ARM_FEATURE_SVE2
	// equivalent only if no intersection between masks
#define svbsl_u64(op1,op2,op3)	svorr_u64_m(pg, (op2), svand_u64_x(pg, (op3), (op1)))
#define svbsl1n_u64(op1,op2,op3)	svorr_u64_m(pg, (op2), svbic_u64_x(pg, (op3), (op1)))
#endif

int last_flip(int pos, unsigned long long P)
{
	svuint64_t	PP, p_flip, p_oflank, p_eraser, p_cap, mask;
	svbool_t	pg;
	const uint64_t	(*pmask)[8];

	PP = svdup_u64(P);
	pmask = &lrmask[pos];
	pg = svwhilelt_b64(0, 4);

	mask = svld1_u64(pg, *pmask + 4);	// right: clear all bits lower than outflank
	p_oflank = svand_x(pg, mask, PP);
	p_oflank = svand_x(pg, svclz_z(pg, p_oflank), 63);
	p_eraser = svlsr_x(pg, svdup_u64(-1), p_oflank);
	p_flip = svbic_x(pg, mask, p_eraser);

	mask = svld1_u64(pg, *pmask + 0);	// left: look for player LS1B
	p_oflank = svand_x(pg, mask, PP);
		// set all bits lower than oflank, using satulation if oflank = 0
	p_cap = svbic_x(pg, svqsub(p_oflank, 1), p_oflank);
	p_flip = svbsl_u64(p_cap, p_flip, mask);

	if (svcntd() == 2) {	// sve128 only
		mask = svld1_u64(pg, *pmask + 6);	// right: set all bits higher than outflank
		p_oflank = svand_x(pg, mask, PP);
		p_oflank = svand_x(pg, svclz_z(pg, p_oflank), 63);
		p_eraser = svlsr_x(pg, svdup_u64(-1), p_oflank);
		p_flip = svbsl1n_u64(p_eraser, p_flip, mask);

		mask = svld1_u64(pg, *pmask + 2);	// left: look for player LS1B
		p_oflank = svand_x(pg, mask, PP);
			// set all bits lower than oflank, using satulation if oflank = 0
		p_cap = svbic_x(pg, svqsub(p_oflank, 1), p_oflank);
		p_flip = svbsl_u64(p_cap, p_flip, mask);
	}

	return svaddv_u64(pg, svcnt_u64_x(pg, p_flip)) * 2;
}
