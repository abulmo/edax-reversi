/**
 * @file board_mmx.c
 *
 * MMX translation of some board.c functions for X86-32
 *
 * If both hasMMX and hasSSE2 are undefined, dynamic dispatching code
 * will be generated.  (This setting requires VC or GCC 4.4+)
 *
 * @date 2014 - 2023
 * @author Toshihiko Okuhara
 * @version 4.5
 */

#include "bit.h"
#include "hash.h"
#include "board.h"
#include "move.h"

#ifdef USE_GAS_MMX
  #ifndef hasMMX
	#pragma GCC push_options
	#pragma GCC target ("mmx")
  #endif
	#include <mmintrin.h>
#endif

static const unsigned long long mask_7e = 0x7e7e7e7e7e7e7e7eULL;
#ifndef POPCOUNT
static const unsigned long long mask_55 = 0x5555555555555555ULL;
static const unsigned long long mask_33 = 0x3333333333333333ULL;
static const unsigned long long mask_0F = 0x0f0f0f0f0f0f0f0fULL;
#endif

#ifndef hasMMX
bool	hasMMX = false;
#endif
bool	hasSSE2 = false;

void init_mmx (void)
{
	int	flg1, flg2, cpuid_edx, cpuid_ecx;
#ifdef USE_MSVC_X86
	int	cpuinfo[4];

	__asm {
		pushfd
		pop	eax
		mov	flg2, eax
		btc	eax, 21
		push	eax
		popfd
		pushfd
		pop	flg1
	}

	if (flg1 == flg2)	/* CPUID not supported */
		return;

	__cpuid(cpuinfo, 1);
	cpuid_edx = cpuinfo[3];
	cpuid_ecx = cpuinfo[2];

#else
	__asm__ (
		"pushfl\n\t"
		"popl	%0\n\t"
		"movl	%0, %1\n\t"
		"btc	$21, %0\n\t"	/* flip ID bit in EFLAGS */
		"pushl	%0\n\t"
		"popfl\n\t"
		"pushfl\n\t"
		"popl	%0"
	: "=r" (flg1), "=r" (flg2) );

	if (flg1 == flg2)	/* CPUID not supported */
		return;

	__asm__ (
		"movl	$1, %%eax\n\t"
		"cpuid"
	: "=d" (cpuid_edx), "=c" (cpuid_ecx) :: "%eax", "%ebx" );

#endif

#ifndef hasMMX
	hasMMX  = ((cpuid_edx & 0x00800000u) != 0);
#endif
	hasSSE2 = ((cpuid_edx & 0x04000000u) != 0);
	// hasPOPCNT = ((cpuid_ecx & 0x00800000u) != 0);

#if (MOVE_GENERATOR == MOVE_GENERATOR_32)
	if (hasSSE2)
		init_flip_sse();
#endif
}

/**
 * @brief MMX translation of get_moves
 *
 * x 2 faster bench mobility on 32-bit x86.
 *
 */
#ifdef USE_MSVC_X86

unsigned long long get_moves_mmx(const unsigned long long P_, const unsigned long long O_)
{
	unsigned int movesL, movesH, mO1, flip1, pre1;
	__m64	P, O, M, mO, flip, pre;

	P = _m_punpckldq(_m_from_int(P_), _m_from_int(P_ >> 32));
	O = _m_punpckldq(_m_from_int(O_), _m_from_int(O_ >> 32));	mO1 = (unsigned int) O_ & 0x7e7e7e7e;
		/* shift = +8 */						/* shift = +1 */
	flip = _m_pand(O, _m_psllqi(P, 8));				flip1  = mO1 & ((unsigned int) P_ << 1);
	flip = _m_por(flip, _m_pand(O, _m_psllqi(flip, 8)));		flip1 |= mO1 & (flip1 << 1);
	pre  = _m_pand(O, _m_psllqi(O, 8));				pre1   = mO1 & (mO1 << 1);
	flip = _m_por(flip, _m_pand(pre, _m_psllqi(flip, 16)));		flip1 |= pre1 & (flip1 << 2);
	flip = _m_por(flip, _m_pand(pre, _m_psllqi(flip, 16)));		flip1 |= pre1 & (flip1 << 2);
	M = _m_psllqi(flip, 8);						movesL = flip1 << 1;
		/* shift = -8 */						/* shift = -1 */
	flip = _m_pand(O, _m_psrlqi(P, 8));				flip1  = mO1 & ((unsigned int) P_ >> 1);
	flip = _m_por(flip, _m_pand(O, _m_psrlqi(flip, 8)));		flip1 |= mO1 & (flip1 >> 1);
	pre  = _m_psrlqi(pre, 8);					pre1 >>= 1;
	flip = _m_por(flip, _m_pand(pre, _m_psrlqi(flip, 16)));		flip1 |= pre1 & (flip1 >> 2);
	flip = _m_por(flip, _m_pand(pre, _m_psrlqi(flip, 16)));		flip1 |= pre1 & (flip1 >> 2);
	M = _m_por(M, _m_psrlqi(flip, 8));				movesL |= flip1 >> 1;
		/* shift = +7 */
	mO = _m_pand(O, *(__m64 *) &mask_7e);				mO1 = (unsigned int)(O_ >> 32) & 0x7e7e7e7e;
	flip = _m_pand(mO, _m_psllqi(P, 7));
	flip = _m_por(flip, _m_pand(mO, _m_psllqi(flip, 7)));
	pre  = _m_pand(mO, _m_psllqi(mO, 7));
	flip = _m_por(flip, _m_pand(pre, _m_psllqi(flip, 14)));
	flip = _m_por(flip, _m_pand(pre, _m_psllqi(flip, 14)));
	M = _m_por(M, _m_psllqi(flip, 7));
		/* shift = -7 */						/* shift = +1 */
	flip = _m_pand(mO, _m_psrlqi(P, 7));				flip1  = mO1 & ((unsigned int)(P_ >> 32) << 1);
	flip = _m_por(flip, _m_pand(mO, _m_psrlqi(flip, 7)));		flip1 |= mO1 & (flip1 << 1);
	pre  = _m_psrlqi(pre, 7);					pre1   = mO1 & (mO1 << 1);
	flip = _m_por(flip, _m_pand(pre, _m_psrlqi(flip, 14)));		flip1 |= pre1 & (flip1 << 2);
	flip = _m_por(flip, _m_pand(pre, _m_psrlqi(flip, 14)));		flip1 |= pre1 & (flip1 << 2);
	M = _m_por(M, _m_psrlqi(flip, 7));				movesH = flip1 << 1;
		/* shift = +9 */						/* shift = -1 */
	flip = _m_pand(mO, _m_psllqi(P, 9));				flip1  = mO1 & ((unsigned int)(P_ >> 32) >> 1);
	flip = _m_por(flip, _m_pand(mO, _m_psllqi(flip, 9)));		flip1 |= mO1 & (flip1 >> 1);
	pre  = _m_pand(mO, _m_psllqi(mO, 9));				pre1 >>= 1;
	flip = _m_por(flip, _m_pand(pre, _m_psllqi(flip, 18)));		flip1 |= pre1 & (flip1 >> 2);
	flip = _m_por(flip, _m_pand(pre, _m_psllqi(flip, 18)));		flip1 |= pre1 & (flip1 >> 2);
	M = _m_por(M, _m_psllqi(flip, 9));				movesH |= flip1 >> 1;
		/* shift = -9 */
	flip = _m_pand(mO, _m_psrlqi(P, 9));
	flip = _m_por(flip, _m_pand(mO, _m_psrlqi(flip, 9)));
	pre  = _m_psrlqi(pre, 9);
	flip = _m_por(flip, _m_pand(pre, _m_psrlqi(flip, 18)));
	flip = _m_por(flip, _m_pand(pre, _m_psrlqi(flip, 18)));
	M = _m_por(M, _m_psrlqi(flip, 9));

	movesL |= _m_to_int(M);
	movesH |= _m_to_int(_m_punpckhdq(M, M));
	_mm_empty();
	return (((unsigned long long) movesH << 32) | movesL) & ~(P_|O_);	// mask with empties
}

#else

unsigned long long get_moves_mmx(const unsigned long long P, const unsigned long long O)
{
	unsigned long long moves;
	__asm__ (
		"movl	%1, %%ebx\n\t"		"movd	%1, %%mm4\n\t"		// (movd for store-forwarding)
		"movl	%3, %%edi\n\t"		"movd	%3, %%mm5\n\t"
		"andl	$0x7e7e7e7e, %%edi\n\t"	"punpckldq %2, %%mm4\n\t"
						"punpckldq %4, %%mm5\n\t"
				/* shift=-1 */			/* shift=-8 */
		"movl	%%ebx, %%eax\n\t"	"movq	%%mm4, %%mm0\n\t"
		"shrl	$1, %%eax\n\t"		"psrlq	$8, %%mm0\n\t"
		"andl	%%edi, %%eax\n\t"	"pand	%%mm5, %%mm0\n\t"	// 0 m7&o6 m6&o5 .. m1&o0
		"movl	%%eax, %%edx\n\t"	"movq	%%mm0, %%mm1\n\t"
		"shrl	$1, %%eax\n\t"		"psrlq	$8, %%mm0\n\t"
		"movl	%%edi, %%ecx\n\t"	"movq	%%mm5, %%mm3\n\t"
		"andl	%%edi, %%eax\n\t"	"pand	%%mm5, %%mm0\n\t"	// 0 0 m7&o6&o5 .. m2&o1&o0
		"shrl	$1, %%ecx\n\t"		"psrlq	$8, %%mm3\n\t"
		"orl	%%edx, %%eax\n\t"	"por	%%mm1, %%mm0\n\t"	// 0 m7&o6 (m6&o5)|(m7&o6&o5) .. (m1&o0)
		"andl	%%edi, %%ecx\n\t"	"pand	%%mm5, %%mm3\n\t"	// 0 o7&o6 o6&o5 o5&o4 o4&o3 ..
		"movl	%%eax, %%edx\n\t"	"movq	%%mm0, %%mm2\n\t"
		"shrl	$2, %%eax\n\t"		"psrlq	$16, %%mm0\n\t"
		"andl	%%ecx, %%eax\n\t"	"pand	%%mm3, %%mm0\n\t"	// 0 0 0 m7&o6&o5&o4 (m6&o5&o4&o3)|(m7&o6&o5&o4&o3) ..
		"orl	%%eax, %%edx\n\t"	"por	%%mm0, %%mm2\n\t"
		"shrl	$2, %%eax\n\t"		"psrlq	$16, %%mm0\n\t"
		"andl	%%ecx, %%eax\n\t"	"pand	%%mm3, %%mm0\n\t"	// 0 0 0 0 0 m7&o6&..&o2 (m6&o5&..&o1)|(m7&o6&..&o1) ..
		"orl	%%edx, %%eax\n\t"	"por	%%mm0, %%mm2\n\t"
		"shrl	$1, %%eax\n\t"		"psrlq	$8, %%mm2\n\t"
				/* shift=+1 */			/* shift=+8 */
						"movq	%%mm4, %%mm0\n\t"
		"addl	%%ebx, %%ebx\n\t"	"psllq	$8, %%mm0\n\t"
		"andl	%%edi, %%ebx\n\t"	"pand	%%mm5, %%mm0\n\t"
		"movl	%%ebx, %%edx\n\t"	"movq	%%mm0, %%mm1\n\t"
		"addl	%%ebx, %%ebx\n\t"	"psllq	$8, %%mm0\n\t"
		"andl	%%edi, %%ebx\n\t"	"pand	%%mm5, %%mm0\n\t"
		"orl	%%ebx, %%edx\n\t"	"por	%%mm1, %%mm0\n\t"
		"addl	%%ecx, %%ecx\n\t"	"psllq	$8, %%mm3\n\t"
						"movq	%%mm0, %%mm1\n\t"
		"leal	(,%%edx,4), %%ebx\n\t"	"psllq	$16, %%mm0\n\t"
		"andl	%%ecx, %%ebx\n\t"	"pand	%%mm3, %%mm0\n\t"
		"orl	%%ebx, %%edx\n\t"	"por	%%mm0, %%mm1\n\t"
		"shll	$2, %%ebx\n\t"		"psllq	$16, %%mm0\n\t"
		"andl	%%ecx, %%ebx\n\t"	"pand	%%mm3, %%mm0\n\t"
		"orl	%%edx, %%ebx\n\t"	"por	%%mm1, %%mm0\n\t"
		"addl	%%ebx, %%ebx\n\t"	"psllq	$8, %%mm0\n\t"
		"orl	%%eax, %%ebx\n\t"	"por	%%mm0, %%mm2\n\t"
								/* shift=-7 */
						"pand	%5, %%mm5\n\t"
						"movq	%%mm4, %%mm0\n\t"
						"psrlq	$7, %%mm0\n\t"
						"pand	%%mm5, %%mm0\n\t"
						"movq	%%mm0, %%mm1\n\t"
						"psrlq	$7, %%mm0\n\t"
						"pand	%%mm5, %%mm0\n\t"
						"movq	%%mm5, %%mm3\n\t"
						"por	%%mm1, %%mm0\n\t"
						"psrlq	$7, %%mm3\n\t"
						"movq	%%mm0, %%mm1\n\t"
						"pand	%%mm5, %%mm3\n\t"
						"psrlq	$14, %%mm0\n\t"
						"pand	%%mm3, %%mm0\n\t"
		"movl	%2, %%esi\n\t"		"por	%%mm0, %%mm1\n\t"
		"movl	%4, %%edi\n\t"		"psrlq	$14, %%mm0\n\t"
		"andl	$0x7e7e7e7e,%%edi\n\t"	"pand	%%mm3, %%mm0\n\t"
		"movl	%%edi, %%ecx\n\t"	"por	%%mm1, %%mm0\n\t"
		"shrl	$1, %%ecx\n\t"		"psrlq	$7, %%mm0\n\t"
		"andl	%%edi, %%ecx\n\t"	"por	%%mm0, %%mm2\n\t"
				/* shift=-1 */			/* shift=+7 */
		"movl	%%esi, %%eax\n\t"	"movq	%%mm4, %%mm0\n\t"
		"shrl	$1, %%eax\n\t"		"psllq	$7, %%mm0\n\t"
		"andl	%%edi, %%eax\n\t"	"pand	%%mm5, %%mm0\n\t"
		"movl	%%eax, %%edx\n\t"	"movq	%%mm0, %%mm1\n\t"
		"shrl	$1, %%eax\n\t"		"psllq	$7, %%mm0\n\t"
		"andl	%%edi, %%eax\n\t"	"pand	%%mm5, %%mm0\n\t"
		"orl	%%edx, %%eax\n\t"	"por	%%mm1, %%mm0\n\t"
						"psllq	$7, %%mm3\n\t"
		"movl	%%eax, %%edx\n\t"	"movq	%%mm0, %%mm1\n\t"
		"shrl	$2, %%eax\n\t"		"psllq	$14, %%mm0\n\t"
		"andl	%%ecx, %%eax\n\t"	"pand	%%mm3, %%mm0\n\t"
		"orl	%%eax, %%edx\n\t"	"por	%%mm0, %%mm1\n\t"
		"shrl	$2, %%eax\n\t"		"psllq	$14, %%mm0\n\t"
		"andl	%%ecx, %%eax\n\t"	"pand	%%mm3, %%mm0\n\t"
		"orl	%%edx, %%eax\n\t"	"por	%%mm1, %%mm0\n\t"
		"shrl	$1, %%eax\n\t"		"psllq	$7, %%mm0\n\t"
						"por	%%mm0, %%mm2\n\t"
				/* shift=+1 */			/* shift=-9 */
						"movq	%%mm4, %%mm0\n\t"
		"addl	%%esi, %%esi\n\t"	"psrlq	$9, %%mm0\n\t"
		"andl	%%edi, %%esi\n\t"	"pand	%%mm5, %%mm0\n\t"
		"movl	%%esi, %%edx\n\t"	"movq	%%mm0, %%mm1\n\t"
		"addl	%%esi, %%esi\n\t"	"psrlq	$9, %%mm0\n\t"
		"andl	%%edi, %%esi\n\t"	"pand	%%mm5, %%mm0\n\t"
						"movq	%%mm5, %%mm3\n\t"
		"orl	%%esi, %%edx\n\t"	"por	%%mm1, %%mm0\n\t"
						"psrlq	$9, %%mm3\n\t"
						"movq	%%mm0, %%mm1\n\t"
		"addl	%%ecx, %%ecx\n\t"	"pand	%%mm5, %%mm3\n\t"
		"leal	(,%%edx,4), %%esi\n\t"	"psrlq	$18, %%mm0\n\t"
		"andl	%%ecx, %%esi\n\t"	"pand	%%mm3, %%mm0\n\t"
		"orl	%%esi, %%edx\n\t"	"por	%%mm0, %%mm1\n\t"
		"shll	$2, %%esi\n\t"		"psrlq	$18, %%mm0\n\t"
		"andl	%%ecx, %%esi\n\t"	"pand	%%mm3, %%mm0\n\t"
		"orl	%%edx, %%esi\n\t"	"por	%%mm1, %%mm0\n\t"
		"addl	%%esi, %%esi\n\t"	"psrlq	$9, %%mm0\n\t"
		"orl	%%eax, %%esi\n\t"	"por	%%mm0, %%mm2\n\t"
								/* shift=+9 */
						"movq	%%mm4, %%mm0\n\t"
						"psllq	$9, %%mm0\n\t"
						"pand	%%mm5, %%mm0\n\t"
						"movq	%%mm0, %%mm1\n\t"
						"psllq	$9, %%mm0\n\t"
						"pand	%%mm5, %%mm0\n\t"
						"por	%%mm1, %%mm0\n\t"
						"psllq	$9, %%mm3\n\t"
						"movq	%%mm0, %%mm1\n\t"
						"psllq	$18, %%mm0\n\t"
						"pand	%%mm3, %%mm0\n\t"
		"movl	%1, %%eax\n\t"		"por	%%mm0, %%mm1\n\t"
		"movl	%2, %%edx\n\t"		"psllq	$18, %%mm0\n\t"
		"orl	%3, %%eax\n\t"		"pand	%%mm3, %%mm0\n\t"
		"orl	%4, %%edx\n\t"		"por	%%mm1, %%mm0\n\t"
		"notl	%%eax\n\t"		"psllq	$9, %%mm0\n\t"
		"notl	%%edx\n\t"		"por	%%mm0, %%mm2\n\t"
		/* mm2|(esi:ebx) is the pseudo-feasible moves at this point. */
		/* Let edx:eax be the feasible moves, i.e., mm2 restricted to empty squares. */
		"movd	%%mm2, %%ecx\n\t"	"punpckhdq %%mm2, %%mm2\n\t"
		"orl	%%ecx, %%ebx\n\t"
		"movd	%%mm2, %%ecx\n\t"
		"orl	%%ecx, %%esi\n\t"
		"andl	%%ebx, %%eax\n\t"
		"andl	%%esi, %%edx\n\t"
		"emms"		/* Reset the FP/MMX unit. */
	: "=&A" (moves)
	: "m" (P), "m" (((unsigned int *)&P)[1]), "m" (O), "m" (((unsigned int *)&O)[1]), "m" (mask_7e)
	: "ebx", "ecx", "esi", "edi", "mm0", "mm1", "mm2", "mm3", "mm4", "mm5" );

	return moves;
}
#endif

/**
 * @brief MMX translation of get_stability()
 *
 * x 1.5 faster bench stability on 32-bit x86.
 *
 */
#ifdef hasMMX
static void get_full_lines(const unsigned long long disc_, unsigned long long full[4])
{
	__m64	disc = *(__m64 *) &disc_;
	__m64	full_l, full_r;
	unsigned int	full_v;
	const __m64	kFF = _m_pcmpeqb(disc, disc);
	static const unsigned long long e7[] = { 0xff01010101010101, 0x80808080808080ff, 0xffff030303030303, 0xc0c0c0c0c0c0ffff, 0xffffffff0f0f0f0f, 0xf0f0f0f0ffffffff };
	static const unsigned long long e9[] = { 0xff80808080808080, 0x01010101010101ff, 0xffffc0c0c0c0c0c0, 0x030303030303ffff, 0x0f0f0f0ff0f0f0f0 };

	// get_full_lines_mmx(full_d7, disc, 7, e7);
	full_l = _m_pand(disc, _m_por(((__m64 *) e7)[0], _m_psrlqi(disc, 7)));
	full_r = _m_pand(disc, _m_por(((__m64 *) e7)[1], _m_psllqi(disc, 7)));
	full_l = _m_pand(full_l, _m_por(((__m64 *) e7)[2], _m_psrlqi(full_l, 14)));
	full_r = _m_pand(full_r, _m_por(((__m64 *) e7)[3], _m_psllqi(full_r, 14)));
	full_l = _m_pand(full_l, _m_por(((__m64 *) e7)[4], _m_psrlqi(full_l, 28)));
	full_r = _m_pand(full_r, _m_por(((__m64 *) e7)[5], _m_psllqi(full_r, 28)));
	((__m64 *) full)[3] = _m_pand(full_l, full_r);

	// get_full_lines_mmx(full_d9, disc, 9, e9);
	full_l = _m_pand(disc, _m_por(((__m64 *) e9)[0], _m_psrlqi(disc, 9)));
	full_r = _m_pand(disc, _m_por(((__m64 *) e9)[1], _m_psllqi(disc, 9)));
	full_l = _m_pand(full_l, _m_por(((__m64 *) e9)[2], _m_psrlqi(full_l, 18)));
	full_r = _m_pand(full_r, _m_por(((__m64 *) e9)[3], _m_psllqi(full_r, 18)));
	((__m64 *) full)[2] = _m_pand(_m_pand(full_l, full_r), _m_por(((__m64 *) e9)[4], _m_por(_m_psrlqi(full_l, 36), _m_psllqi(full_r, 36))));

	// get_full_lines_mmx(full_h, disc, 1, e1);
	((__m64 *) full)[0] = _m_pcmpeqb(kFF, disc);
	_mm_empty();

	// get_full_lines_mmx(full_v, disc, 8, e8);
	full_v = (unsigned int) disc_ & (unsigned int)(disc_ >> 32);
	full_v &= (full_v >> 16) | (full_v << 16);	// ror 16
	full_v &= (full_v >> 8) | (full_v << 24);	// ror 8
	full[1] = full_v | ((unsigned long long) full_v << 32);
}

// returns all full in full[4] in addition to stability count
int get_stability_fulls(unsigned long long P, unsigned long long O, unsigned long long full[5])
{
	__m64	P_central, stable, stable_h, stable_v, stable_d7, stable_d9, old_stable, m;
	unsigned int	OL, OH, PL, PH, t, a1a8, h1h8, SL, SH;

	get_full_lines(P | O, full);

	OL = (unsigned int) O;	OH = (unsigned int)(O >> 32);
	PL = (unsigned int) P;	PH = (unsigned int)(P >> 32);
	SL = PL & 0x7f7f7f00;	SH = PH & 0x007f7f7f;
	P_central = _m_punpckldq(_m_from_int(SL), _m_from_int(SH));

	// P_central & allfull
	full[4] = full[0] & full[1] & full[2] & full[3];
	SL &= (unsigned int) full[4];
	SH &= (unsigned int)(full[4] >> 32);

	// compute the exact stable edges (from precomputed tables)
	a1a8 = edge_stability[((((PL & 0x01010101) + ((PH & 0x01010101) << 4)) * 0x01020408) >> 24) * 256
		+ ((((OL & 0x01010101) + ((OH & 0x01010101) << 4)) * 0x01020408) >> 24)];
	h1h8 = edge_stability[((((PH & 0x80808080) + ((PL & 0x80808080) >> 4)) * 0x00204081) >> 24) * 256
		+ ((((OH & 0x80808080) + ((OL & 0x80808080) >> 4)) * 0x00204081) >> 24)];
	SL |= edge_stability[(PL & 0xff) * 256 + (OL & 0xff)]
		| (((a1a8 & 0x0f) * 0x00204081) & 0x01010101)
		| (((h1h8 & 0x0f) * 0x10204080) & 0x80808080);
	SH |= (edge_stability[((PH >> 16) & 0xff00) + (OH >> 24)] << 24)
		| (((a1a8 >> 4) * 0x00204081) & 0x01010101)
		| (((h1h8 >> 4) * 0x10204080) & 0x80808080);
	stable = _m_punpckldq(_m_from_int(SL), _m_from_int(SH));

	// now compute the other stable discs (ie discs touching another stable disc in each flipping direction).
	t = SL | SH;
	if (t) {
		do {
			old_stable = stable;
			stable_h = _m_por(_m_por(_m_psrlqi(stable, 1), _m_psllqi(stable, 1)), ((__m64 *) full)[0]);
			stable_v = _m_por(_m_por(_m_psrlqi(stable, 8), _m_psllqi(stable, 8)), ((__m64 *) full)[1]);
			stable_d7 = _m_por(_m_por(_m_psrlqi(stable, 7), _m_psllqi(stable, 7)), ((__m64 *) full)[3]);
			stable_d9 = _m_por(_m_por(_m_psrlqi(stable, 9), _m_psllqi(stable, 9)), ((__m64 *) full)[2]);
			stable = _m_por(stable, _m_pand(_m_pand(_m_pand(_m_pand(stable_h, stable_v), stable_d7), stable_d9), P_central));
			m = _m_pxor(stable, old_stable);
		} while (_m_to_int(_m_packsswb(m, m)) != 0);

  #ifdef POPCOUNT
		t = bit_count_32(_m_to_int(stable)) + bit_count_32(_m_to_int(_m_psrlqi(stable, 32)));
  #else
		m = _m_psubd(stable, _m_pand(_m_psrlqi(stable, 1), *(__m64 *) &mask_55));
		m = _m_paddd(_m_pand(m, *(__m64 *) &mask_33), _m_pand(_m_psrlqi(m, 2), *(__m64 *) &mask_33));
		m = _m_pand(_m_paddd(m, _m_psrlqi(m, 4)), *(__m64 *) &mask_0F);
		t = ((unsigned int) _m_to_int(_m_paddb(m, _m_psrlqi(m, 32))) * 0x01010101u) >> 24;
  #endif
	}
	_mm_empty();
	return t;
}

// returns stability count only
int get_stability(const unsigned long long P, const unsigned long long O)
{
	unsigned long long full[5];

	return get_stability_fulls(P, O, full);
}
#endif // hasMMX

#if !defined(hasMMX) && defined(USE_GAS_MMX)
	#pragma GCC pop_options
#endif
