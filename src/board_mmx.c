<<<<<<< HEAD
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
=======
/**
 * @file board_mmx.c
 *
 * MMX translation of some board.c functions
 *
 * If both hasMMX and hasSSE2 are undefined, dynamic dispatching code
 * will be generated.  (This setting requires GCC 4.4+)
 *
 * Parameters are passed as 4 ints instead of 2 longlongs to avoid
 * alignment ajust and to enable store to load forwarding.
 *
 * @date 2014 - 2018
 * @author Toshihiko Okuhara
 * @version 4.4
 */

#include "bit.h"
#include "hash.h"
#include "board.h"
#include "move.h"

#if !defined(hasSSE2) && defined(USE_GAS_MMX)
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

#ifndef hasSSE2

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
#endif	// hasSSE2

#ifdef hasMMX
/**
 * @brief Update a board.
 *
 * Update a board by flipping its discs and updating every other data,
 * according to the 'move' description.
 *
 * @param board the board to modify
 * @param move  A Move structure describing the modification.
 */
#if defined(hasSSE2) && !defined(__3dNOW__)	// Faster on CPU with slow emms

void board_update(Board *board, const Move *move)
{
	__m128i	F = _mm_loadl_epi64((__m128i *) &move->flipped);
	__m128i	OP = _mm_loadu_si128((__m128i *) board);
	OP = _mm_xor_si128(OP, _mm_or_si128(_mm_unpacklo_epi64(F, F), _mm_loadl_epi64((__m128i *) &X_TO_BIT[move->x])));
	_mm_storel_pi((__m64 *) &board->opponent, _mm_castsi128_ps(OP));
	_mm_storeh_pi((__m64 *) &board->player, _mm_castsi128_ps(OP));
	board_check(board);
}

#elif defined(USE_MSVC_X86)

void board_update(Board *board, const Move *move)
{
	__m64	F = *(__m64 *) &move->flipped;
	__m64	P = _m_pxor(*(__m64 *) &board->player, _m_por(F, *(__m64 *) &X_TO_BIT[move->x]));
	__m64	O = _m_pxor(*(__m64 *) &board->opponent, F);
	*(__m64 *) &board->player = O;
	*(__m64 *) &board->opponent = P;
	_mm_empty();
	board_check(board);
}

#else

void board_update(Board *board, const Move *move)
{
	__asm__ (
		"movq	%2, %%mm1\n\t"
		"movq	%3, %%mm0\n\t"
		"por	%%mm1, %%mm0\n\t"
		"pxor	%0, %%mm0\n\t"
		"pxor	%1, %%mm1\n\t"
		"movq	%%mm0, %1\n\t"
		"movq	%%mm1, %0\n\t"
		"emms"
	: "=m" (board->player), "=m" (board->opponent)
	: "m" (move->flipped), "m" (x_to_bit(move->x))
	: "mm0", "mm1");
	board_check(board);
}

#endif

/**
 * @brief Restore a board.
 *
 * Restore a board by un-flipping its discs and restoring every other data,
 * according to the 'move' description, in order to cancel a board_update_move.
 *
 * @param board board to restore.
 * @param move  a Move structure describing the modification.
 */
#if defined(hasSSE2) && !defined(__3dNOW__)

void board_restore(Board *board, const Move *move)
{
	__m128i	F = _mm_loadl_epi64((__m128i *) &move->flipped);
	__m128i	OP = _mm_unpacklo_epi64(_mm_loadl_epi64((__m128i *) &board->opponent), _mm_loadl_epi64((__m128i *) &board->player));
	OP = _mm_xor_si128(OP, _mm_or_si128(_mm_unpacklo_epi64(F, F), _mm_loadl_epi64((__m128i *) &X_TO_BIT[move->x])));
	_mm_storeu_si128((__m128i *) board, OP);
	board_check(board);
}

#elif defined(USE_MSVC_X86)

void board_restore(Board *board, const Move *move)
{
	__m64	F = *(__m64 *) &move->flipped;
	__m64	P = *(__m64 *) &board->opponent;
	__m64	O = *(__m64 *) &board->player;
	*(__m64 *) &board->player = _m_pxor(P, _m_por(F, *(__m64 *) &X_TO_BIT[move->x]));
	*(__m64 *) &board->opponent = _m_pxor(O, F);
	_mm_empty();
	board_check(board);
}

#else

void board_restore(Board *board, const Move *move)
{
	__asm__ (
		"movq	%2, %%mm1\n\t"
		"movq	%3, %%mm0\n\t"
		"por	%%mm1, %%mm0\n\t"
		"pxor	%1, %%mm0\n\t"
		"pxor	%0, %%mm1\n\t"
		"movq	%%mm0, %0\n\t"
		"movq	%%mm1, %1\n\t"
		"emms"
	: "=m" (board->player), "=m" (board->opponent)
	: "m" (move->flipped), "m" (x_to_bit(move->x))
	: "mm0", "mm1");
	board_check(board);
}

#endif
#endif // hasMMX

/**
 * @brief MMX translation of get_moves
 *
 * x 2 faster bench mobility on 32-bit x86.
 *
 */
#ifdef USE_MSVC_X86

unsigned long long get_moves_mmx(unsigned int PL, unsigned int PH, unsigned int OL, unsigned int OH)
{
	unsigned int movesL, movesH, mO1, flip1, pre1;
	__m64	P, O, M, mO, flip, pre;

	P = _m_punpckldq(_m_from_int(PL), _m_from_int(PH));
	O = _m_punpckldq(_m_from_int(OL), _m_from_int(OH));		mO1 = OL & 0x7e7e7e7e;
		/* shift = +8 */						/* shift = +1 */
	flip = _m_pand(O, _m_psllqi(P, 8));				flip1  = mO1 & (PL << 1);
	flip = _m_por(flip, _m_pand(O, _m_psllqi(flip, 8)));		flip1 |= mO1 & (flip1 << 1);
	pre  = _m_pand(O, _m_psllqi(O, 8));				pre1   = mO1 & (mO1 << 1);
	flip = _m_por(flip, _m_pand(pre, _m_psllqi(flip, 16)));		flip1 |= pre1 & (flip1 << 2);
	flip = _m_por(flip, _m_pand(pre, _m_psllqi(flip, 16)));		flip1 |= pre1 & (flip1 << 2);
	M = _m_psllqi(flip, 8);						movesL = flip1 << 1;
		/* shift = -8 */						/* shift = -1 */
	flip = _m_pand(O, _m_psrlqi(P, 8));				flip1  = mO1 & (PL >> 1);
	flip = _m_por(flip, _m_pand(O, _m_psrlqi(flip, 8)));		flip1 |= mO1 & (flip1 >> 1);
	pre  = _m_psrlqi(pre, 8);					pre1 >>= 1;
	flip = _m_por(flip, _m_pand(pre, _m_psrlqi(flip, 16)));		flip1 |= pre1 & (flip1 >> 2);
	flip = _m_por(flip, _m_pand(pre, _m_psrlqi(flip, 16)));		flip1 |= pre1 & (flip1 >> 2);
	M = _m_por(M, _m_psrlqi(flip, 8));				movesL |= flip1 >> 1;
		/* shift = +7 */
	mO = _m_pand(O, *(__m64 *) &mask_7e);				mO1 = OH & 0x7e7e7e7e;
	flip = _m_pand(mO, _m_psllqi(P, 7));
	flip = _m_por(flip, _m_pand(mO, _m_psllqi(flip, 7)));
	pre  = _m_pand(mO, _m_psllqi(mO, 7));
	flip = _m_por(flip, _m_pand(pre, _m_psllqi(flip, 14)));
	flip = _m_por(flip, _m_pand(pre, _m_psllqi(flip, 14)));
	M = _m_por(M, _m_psllqi(flip, 7));
		/* shift = -7 */						/* shift = +1 */
	flip = _m_pand(mO, _m_psrlqi(P, 7));				flip1  = mO1 & (PH << 1);
	flip = _m_por(flip, _m_pand(mO, _m_psrlqi(flip, 7)));		flip1 |= mO1 & (flip1 << 1);
	pre  = _m_psrlqi(pre, 7);					pre1   = mO1 & (mO1 << 1);
	flip = _m_por(flip, _m_pand(pre, _m_psrlqi(flip, 14)));		flip1 |= pre1 & (flip1 << 2);
	flip = _m_por(flip, _m_pand(pre, _m_psrlqi(flip, 14)));		flip1 |= pre1 & (flip1 << 2);
	M = _m_por(M, _m_psrlqi(flip, 7));				movesH = flip1 << 1;
		/* shift = +9 */						/* shift = -1 */
	flip = _m_pand(mO, _m_psllqi(P, 9));				flip1  = mO1 & (PH >> 1);
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

	movesL = (movesL | _m_to_int(M)) & ~(PL|OL);	// mask with empties
	movesH = (movesH | _m_to_int(_m_punpckhdq(M, M))) & ~(PH|OH);
	_mm_empty();
	return ((unsigned long long) movesH << 32) | movesL;
}

#else

unsigned long long get_moves_mmx(unsigned int PL, unsigned int PH, unsigned int OL, unsigned int OH)
{
	unsigned long long moves;
	__asm__ (
		"movl	%1, %%ebx\n\t"		"movd	%1, %%mm4\n\t"
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
	: "m" (PL), "m" (PH), "m" (OL), "m" (OH), "m" (mask_7e)
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
#ifdef USE_MSVC_X86

int get_stability_mmx(unsigned int PL, unsigned int PH, unsigned int OL, unsigned int OH)
{
	__m64	P, O, P_central, disc, full_h, full_v, full_d7, full_d9, full_l, full_r, stable;
	__m64	stable_h, stable_v, stable_d7, stable_d9, old_stable, m;
	unsigned int	t, a1a8po, h1h8po;
	static const unsigned long long MFF = 0xffffffffffffffff;
	static const unsigned long long edge = 0xff818181818181ffULL;
	static const unsigned long long e7[] = { 0xffff8383838383ff, 0xffffffff8f8f8fff, 0xffc1c1c1c1c1ffff, 0xfff1f1f1ffffffff };
	static const unsigned long long e9[] = { 0xffffc1c1c1c1c1ff, 0xff8f8f8ff1f1f1ff, 0xff8383838383ffff };

	P = _m_punpckldq(_m_from_int(PL), _m_from_int(PH));
	O = _m_punpckldq(_m_from_int(OL), _m_from_int(OH));
	disc = _m_por(P, O);
	P_central = _m_pandn(*(__m64 *) &edge, P);

	// get full lines and set intersection of them to stable
	// get_full_lines_mmx(full_h, disc, 1, e1);
	full_h = _m_pcmpeqb(*(__m64 *) &MFF, disc);
	stable = _m_pand(P_central, full_h);

	// get_full_lines_mmx(full_v, disc, 8, e8);
	full_v = _m_pand(_m_punpcklbw(disc, disc), _m_punpckhbw(disc, disc));	//  (d,d,c,c,b,b,a,a) & (h,h,g,g,f,f,e,e)
	full_v = _m_pand(_m_punpcklwd(full_v, full_v), _m_punpckhwd(full_v, full_v));	// (dh,dh,dh,dh,cg,cg,cg,cg) & (bf,bf,bf,bf,ae,ae,ae,ae)
	full_v = _m_pand(_m_punpckldq(full_v, full_v), _m_punpckhdq(full_v, full_v));	// (bdfh*4, bdfh*4) & (aceg*4, aceg*4)
	stable = _m_pand(stable, full_v);

	// get_full_lines_mmx(full_d7, disc, 7, e7);
	full_l = _m_pand(disc, _m_por(*(__m64 *) &edge, _m_psrlqi(disc, 7)));
	full_r = _m_pand(disc, _m_por(*(__m64 *) &edge, _m_psllqi(disc, 7)));
	full_l = _m_pand(full_l, _m_por(*(__m64 *) &e7[0], _m_psrlqi(full_l, 14)));
	full_r = _m_pand(full_r, _m_por(*(__m64 *) &e7[2], _m_psllqi(full_r, 14)));
	full_l = _m_pand(full_l, _m_por(*(__m64 *) &e7[1], _m_psrlqi(full_l, 28)));
	full_r = _m_pand(full_r, _m_por(*(__m64 *) &e7[3], _m_psllqi(full_r, 28)));
	full_d7 = _m_pand(full_l, full_r);
	stable = _m_pand(stable, full_d7);

	// get_full_lines_mmx(full_d9, disc, 9, e9);

	full_l = _m_pand(disc, _m_por(*(__m64 *) &edge, _m_psrlqi(disc, 9)));
	full_r = _m_pand(disc, _m_por(*(__m64 *) &edge, _m_psllqi(disc, 9)));
	full_l = _m_pand(full_l, _m_por(*(__m64 *) &e9[0], _m_psrlqi(full_l, 18)));
	full_r = _m_pand(full_r, _m_por(*(__m64 *) &e9[2], _m_psllqi(full_r, 18)));
	full_d9 = _m_pand(_m_pand(full_l, full_r), _m_por(*(__m64 *) &e9[1], _m_por(_m_psrlqi(full_l, 36), _m_psllqi(full_r, 36))));
	stable = _m_pand(stable, full_d9);

	// compute the exact stable edges (from precomputed tables)
	a1a8po = ((((PL & 0x01010101u) + ((PH & 0x01010101u) << 4)) * 0x01020408u) >> 24) * 256
		+ ((((OL & 0x01010101u) + ((OH & 0x01010101u) << 4)) * 0x01020408u) >> 24);
	h1h8po = ((((PH & 0x80808080u) + ((PL & 0x80808080u) >> 4)) * 0x00204081u) >> 24) * 256
		+ ((((OH & 0x80808080u) + ((OL & 0x80808080u) >> 4)) * 0x00204081u) >> 24);
	stable = _m_por(stable, _m_por(_m_por(*(__m64 *) &A1_A8[edge_stability[a1a8po]],
		_m_psllqi(*(__m64 *) &A1_A8[edge_stability[h1h8po]], 7)),
		_m_punpckldq(_m_from_int(edge_stability[(PL & 0xff) * 256 + (OL & 0xff)]),
		_m_from_int(edge_stability[((PH >> 16) & 0xff00) + (OH >> 24)] << 24))));

	// now compute the other stable discs (ie discs touching another stable disc in each flipping direction).
	if (_m_to_int(_m_packsswb(stable, stable)) == 0) {
		_mm_empty();
		return 0;
	}

	do {
		old_stable = stable;
		stable_h = _m_por(_m_por(_m_psrlqi(stable, 1), _m_psllqi(stable, 1)), full_h);
		stable_v = _m_por(_m_por(_m_psrlqi(stable, 8), _m_psllqi(stable, 8)), full_v);
		stable_d7 = _m_por(_m_por(_m_psrlqi(stable, 7), _m_psllqi(stable, 7)), full_d7);
		stable_d9 = _m_por(_m_por(_m_psrlqi(stable, 9), _m_psllqi(stable, 9)), full_d9);
		stable = _m_por(stable, _m_pand(_m_pand(_m_pand(_m_pand(stable_h, stable_v), stable_d7), stable_d9), P_central));
		m = _m_pxor(stable, old_stable);
	} while (_m_to_int(_m_packsswb(m, m)) != 0);

#ifdef POPCOUNT
	t = __popcnt(_m_to_int(stable)) + __popcnt(_m_to_int(_m_psrlqi(stable, 32)));
#else
	m = _m_psubd(stable, _m_pand(_m_psrlqi(stable, 1), *(__m64 *) &mask_55));
	m = _m_paddd(_m_pand(m, *(__m64 *) &mask_33), _m_pand(_m_psrlqi(m, 2), *(__m64 *) &mask_33));
	m = _m_pand(_m_paddd(m, _m_psrlqi(m, 4)), *(__m64 *) &mask_0F);
	t = ((unsigned int) _m_to_int(_m_paddb(m, _m_psrlqi(m, 32))) * 0x01010101u) >> 24;
#endif
	_mm_empty();
	return t;
}

#elif defined(USE_GAS_MMX) && !(defined(__clang__) && (__clang__major__ < 3))
// LLVM ERROR: Unsupported asm: input constraint with a matching output constraint of incompatible type!

#define	get_full_lines_mmx(result,disc,dir,edge)	__asm__ (\
		"movq	%2, %%mm0\n\t"		"movq	%2, %%mm1\n\t"\
		"psrlq	%3, %%mm0\n\t"		"psllq	%3, %%mm1\n\t"\
		"por	%6, %%mm0\n\t"		"por	%6, %%mm1\n\t"\
		"pand	%2, %%mm0\n\t"		"pand	%2, %%mm1\n\t"\
		"movq	%%mm0, %%mm2\n\t"	"movq	%%mm1, %%mm3\n\t"\
		"psrlq	%4, %%mm0\n\t"		"psllq	%4, %%mm1\n\t"\
		"por	%7, %%mm0\n\t"		"por	%9, %%mm1\n\t"\
		"pand	%%mm2, %%mm0\n\t"	"pand	%%mm3, %%mm1\n\t"\
		"movq	%%mm0, %%mm2\n\t"	"pand	%%mm1, %%mm0\n\t"\
		"psrlq	%5, %%mm2\n\t"		"psllq	%5, %%mm1\n\t"\
		"por	%8, %%mm2\n\t"		"por	%10, %%mm1\n\t"\
		"pand	%%mm2, %%mm0\n\t"	"pand	%%mm1, %%mm0\n\t"\
		"movq	%%mm0, %0\n\t"\
		"pand	%%mm0, %1"\
	: "=m" (result), "+y" (stable)\
	: "y" (disc), "i" (dir), "i" (dir * 2), "i" (dir * 4),\
	  "my" (e0), "m" (edge[0]), "m" (edge[1]), "m" (edge[2]), "m" (edge[3])\
	: "mm0", "mm1", "mm2", "mm3");

int get_stability_mmx(unsigned int PL, unsigned int PH, unsigned int OL, unsigned int OH)
{
	__m64	P, O, P_central, disc, full_h, full_v, full_d7, full_d9, stable;
#ifdef hasSSE2
	__v2di	PO;
#endif
	int	t, a1a8po, h1h8po;
	static const unsigned long long e0 = 0xff818181818181ffULL;
	// static const unsigned long long e1[] = { 0xffc1c1c1c1c1c1ffULL, 0xfff1f1f1f1f1f1ffULL, 0xff838383838383ffULL, 0xff8f8f8f8f8f8fffULL };
	// static const unsigned long long e8[] = { 0xffff8181818181ffULL, 0xffffffff818181ffULL, 0xff8181818181ffffULL, 0xff818181ffffffffULL };
	static const unsigned long long e7[] = { 0xffff8383838383ffULL, 0xffffffff8f8f8fffULL, 0xffc1c1c1c1c1ffffULL, 0xfff1f1f1ffffffffULL };
	static const unsigned long long e9[] = { 0xffffc1c1c1c1c1ffULL, 0xfffffffff1f1f1ffULL, 0xff8383838383ffffULL, 0xff8f8f8fffffffffULL };

	__asm__ (
		"movd	%2, %0\n\t"		"movd	%4, %1\n\t"
		"punpckldq %3, %0\n\t"		"punpckldq %5, %1"
	: "=&y" (P), "=&y" (O) : "m" (PL), "m" (PH), "m" (OL), "m" (OH));
#ifdef hasSSE2
	PO = _mm_unpacklo_epi64(_mm_movpi64_epi64(O), _mm_movpi64_epi64(P));
#endif
	__asm__ (
		"por	%3, %0\n\t"
		"pandn	%3, %1\n\t"
		"movq	%1, %2"
	: "=y" (disc), "=y" (stable), "=m" (P_central)
	: "y" (P), "0" (O), "1" (e0));

	// get full lines and set intersection of them to stable
	// get_full_lines_mmx(full_h, disc, 1, e1);
	__asm__ (
		"pcmpeqb %%mm0, %%mm0\n\t"
		"pcmpeqb %2, %%mm0\n\t"
		"movq	%%mm0, %0\n\t"
		"pand	%%mm0, %1"
	: "=m" (full_h), "+y" (stable) : "y" (disc) : "mm0");
	// get_full_lines_mmx(full_v, disc, 8, e8);
	__asm__ (
		"movq	%2, %%mm0\n\t"		"movq	%2, %%mm1\n\t"
		"punpcklbw %%mm0, %%mm0\n\t"	"punpckhbw %%mm1, %%mm1\n\t"
		"pand	%%mm1, %%mm0\n\t"	// (d,d,c,c,b,b,a,a) & (h,h,g,g,f,f,e,e)
#ifdef hasSSE2
		"pshufw	$177, %%mm0, %%mm1\n\t"
		"pand	%%mm1, %%mm0\n\t"	// (cg,cg,dh,dh,ae,ae,bf,bf) & (dh,dh,cg,cg,bf,bf,ae,ae)
		"pshufw	$78, %%mm0, %%mm1\n\t"
		"pand	%%mm1, %%mm0\n\t"	// (abef*4, cdgh*4) & (cdgh*4, abef*4)
#else
		"movq	%%mm0, %%mm1\n\t"
		"punpcklwd %%mm0, %%mm0\n\t"	"punpckhwd %%mm1, %%mm1\n\t"
		"pand	%%mm1, %%mm0\n\t"	// (dh,dh,dh,dh,cg,cg,cg,cg) & (bf,bf,bf,bf,ae,ae,ae,ae)
		"movq	%%mm0, %%mm1\n\t"
		"punpckldq %%mm0, %%mm0\n\t"	"punpckhdq %%mm1, %%mm1\n\t"
		"pand	%%mm1, %%mm0\n\t"	// (bdfh*4, bdfh*4) & (aceg*4, aceg*4)
#endif
		"movq	%%mm0, %0\n\t"
		"pand	%%mm0, %1"
	: "=m" (full_v), "+y" (stable) : "y" (disc) : "mm0", "mm1");
	get_full_lines_mmx(full_d7, disc, 7, e7);
	get_full_lines_mmx(full_d9, disc, 9, e9);

	// compute the exact stable edges (from precomputed tables)
#ifdef hasSSE2
	a1a8po = _mm_movemask_epi8(_mm_slli_epi64(PO, 7));
	h1h8po = _mm_movemask_epi8(PO);
#else
	a1a8po = ((((PL & 0x01010101u) + ((PH & 0x01010101u) << 4)) * 0x01020408u) >> 24) * 256
		+ ((((OL & 0x01010101u) + ((OH & 0x01010101u) << 4)) * 0x01020408u) >> 24);
	h1h8po = ((((PH & 0x80808080u) + ((PL & 0x80808080u) >> 4)) * 0x00204081u) >> 24) * 256
		+ ((((OH & 0x80808080u) + ((OL & 0x80808080u) >> 4)) * 0x00204081u) >> 24);
#endif
	__asm__(
		"movd	%1, %%mm0\n\t"		"por	%3, %0\n\t"
		"movd	%2, %%mm1\n\t"
		"punpckldq %%mm1, %%mm0\n\t"	"movq	%4, %%mm1\n\t"
		"por	%%mm0, %0\n\t"		"psllq	$7, %%mm1\n\t"
						"por	%%mm1, %0"
	: "+y" (stable)
	: "g" ((int) edge_stability[(PL & 0xff) * 256 + (OL & 0xff)]),
	  "g" (edge_stability[((PH >> 16) & 0xff00) + (OH >> 24)] << 24),
	  "m" (A1_A8[edge_stability[a1a8po]]),
	  "m" (A1_A8[edge_stability[h1h8po]])
	: "mm0", "mm1");

	// now compute the other stable discs (ie discs touching another stable disc in each flipping direction).
	__asm__ (
		"movq	%1, %%mm0\n\t"
		"packsswb %%mm0, %%mm0\n\t"
		"movd	%%mm0, %0\n\t"
	: "=g" (t) : "y" (stable) : "mm0" );

	if (t == 0) {
		__asm__ ( "emms" );
		return 0;
	}

	do {
		__asm__ (
			"movq	%1, %%mm3\n\t"
			"movq	%6, %1\n\t"
			"movq	%%mm3, %%mm0\n\t"	"movq	%%mm3, %%mm1\n\t"
			"psrlq	$1, %%mm0\n\t"		"psllq	$1, %%mm1\n\t"		"movq	%%mm3, %%mm2\n\t"
			"por	%%mm1, %%mm0\n\t"	"movq	%%mm3, %%mm1\n\t"	"psrlq	$7, %%mm2\n\t"
			"por	%2, %%mm0\n\t"		"psllq	$7, %%mm1\n\t"		"por	%%mm1, %%mm2\n\t"
			"pand	%%mm0, %1\n\t"						"por	%4, %%mm2\n\t"
			"movq	%%mm3, %%mm0\n\t"	"movq	%%mm3, %%mm1\n\t"	"pand	%%mm2, %1\n\t"
			"psrlq	$8, %%mm0\n\t"		"psllq	$8, %%mm1\n\t"		"movq	%%mm3, %%mm2\n\t"
			"por	%%mm1, %%mm0\n\t"	"movq	%%mm3, %%mm1\n\t"	"psrlq	$9, %%mm2\n\t"
			"por	%3, %%mm0\n\t"		"psllq	$9, %%mm1\n\t"		"por	%%mm1, %%mm2\n\t"
			"pand	%%mm0, %1\n\t"						"por	%5, %%mm2\n\t"
											"pand	%%mm2, %1\n\t"
			"por	%%mm3, %1\n\t"
			"pxor	%1, %%mm3\n\t"
			"packsswb %%mm3, %%mm3\n\t"
			"movd	%%mm3, %0"
		: "=g" (t), "+y" (stable)
		: "m" (full_h), "m" (full_v), "m" (full_d7), "m" (full_d9), "m" (P_central)
		: "mm0", "mm1", "mm2", "mm3");
	} while (t);

	// bit_count(stable)
#ifdef POPCOUNT
	__asm__ (
		"movd	%1, %0\n\t"
		"psrlq	$32, %1\n\t"
		"movd	%1, %%edx\n\t"
		"popcntl %0, %0\n\t"
		"popcntl %%edx, %%edx\n\t"
		"addl	%%edx, %0\n\t"
		"emms"
	: "=&a" (t) : "y" (stable) : "edx");
#else
	__asm__ (
 		"movq	%1, %%mm0\n\t"
		"psrlq	$1, %1\n\t"
		"pand	%2, %1\n\t"
		"psubd	%1, %%mm0\n\t"

		"movq	%%mm0, %%mm1\n\t"
		"psrlq	$2, %%mm0\n\t"
		"pand	%3, %%mm1\n\t"
		"pand	%3, %%mm0\n\t"
		"paddd	%%mm1, %%mm0\n\t"

		"movq	%%mm0, %%mm1\n\t"
		"psrlq	$4, %%mm0\n\t"
		"paddd	%%mm1, %%mm0\n\t"
		"pand	%4, %%mm0\n\t"
	#ifdef hasSSE2
		"pxor	%%mm1, %%mm1\n\t"
		"psadbw	%%mm1, %%mm0\n\t"
		"movd	%%mm0, %0\n\t"
	#else
		"movq	%%mm0, %%mm1\n\t"
		"psrlq	$32, %%mm0\n\t"
		"paddb	%%mm1, %%mm0\n\t"

		"movd	%%mm0, %0\n\t"
		"imull	$0x01010101, %0, %0\n\t"
		"shrl	$24, %0\n\t"
	#endif
		"emms"
	: "=a" (t) : "y" (stable), "m" (mask_55), "my" (mask_33), "m" (mask_0F) : "mm0", "mm1");
#endif

	return t;
}
#endif // USE_MSVC_X86

/**
 * @brief MMX translation of get_potential_mobility
 *
 * @param P bitboard with player's discs.
 * @param O bitboard with opponent's discs.
 * @return a count of potential moves.
 */
#ifdef USE_MSVC_X86

int get_potential_mobility_mmx(unsigned long long P, unsigned long long O)
{
	__m64	m, mO;
	int	count;
	static const unsigned long long mask_v = 0x00ffffffffffff00ULL;
	// static const unsigned long long mask_d = 0x007e7e7e7e7e7e00ULL;	// = mask_7e & mask_v
#ifdef POPCOUNT
	int	mh, ml;
#else
	static const unsigned long long mask_15 = 0x1555555555555515ULL;
	static const unsigned long long mask_01 = 0x0100000000000001ULL;
#endif

	mO = _m_pand(*(__m64 *) &O, *(__m64 *) &mask_7e);
	m = _m_por(_m_psllqi(mO, 1), _m_psrlqi(mO, 1));
	mO = _m_pand(*(__m64 *) &O, *(__m64 *) &mask_v);
	m = _m_por(m, _m_por(_m_psllqi(mO, 8), _m_psrlqi(mO, 8)));
	mO = _m_pand(mO, *(__m64 *) &mask_7e);
	m = _m_por(m, _m_por(_m_psllqi(mO, 7), _m_psrlqi(mO, 7)));
	m = _m_por(m, _m_por(_m_psllqi(mO, 9), _m_psrlqi(mO, 9)));
	m = _m_pandn(_m_por(*(__m64 *) &O, *(__m64 *) &P), m);

#ifdef POPCOUNT
	ml = _m_to_int(m);
	mh = _m_to_int(_m_psrlqi(m, 32));
	count = __popcnt(ml) + __popcnt(mh) + __popcnt(ml & 0x00000081) + __popcnt(mh + 0x81000000);
#else
	m = _m_paddd(_m_psubd(m, _m_pand(_m_psrlqi(m, 1), *(__m64 *) &mask_15)), _m_pand(m, *(__m64 *) &mask_01));
	m = _m_paddd(_m_pand(m, *(__m64 *) &mask_33), _m_pand(_m_psrlqi(m, 2), *(__m64 *) &mask_33));
	m = _m_pand(_m_paddd(m, _m_psrlqi(m, 4)), *(__m64 *) &mask_0F);
	count = ((unsigned int) _m_to_int(_m_paddb(m, _m_psrlqi(m, 32))) * 0x01010101u) >> 24;
#endif
	_mm_empty();
	return count;
}

#elif defined(USE_GAS_MMX)

int get_potential_mobility_mmx(unsigned long long P, unsigned long long O)
{
	int	count;
	static const unsigned long long mask_v = 0x00ffffffffffff00ULL;
	// static const unsigned long long mask_d = 0x007e7e7e7e7e7e00ULL;	// = mask_7e & mask_v
#ifndef POPCOUNT
	static const unsigned long long mask_15 = 0x1555555555555515ULL;
	static const unsigned long long mask_01 = 0x0100000000000001ULL;
#endif

	__asm__ (
		"movq	%3, %%mm2\n\t"		"movq	%4, %%mm5\n\t"
		"pand	%2, %%mm2\n\t"		"pand	%2, %%mm5\n\t"		"movq	%%mm2, %%mm3\n\t"
		"movq	%%mm2, %%mm4\n\t"	"movq	%%mm5, %%mm6\n\t"	"pand	%%mm5, %%mm3\n\t"
		"psllq	$1, %%mm2\n\t"		"psllq	$8, %%mm5\n\t"
		"psrlq	$1, %%mm4\n\t"		"psrlq	$8, %%mm6\n\t"
		"por	%%mm4, %%mm2\n\t"	"por	%%mm6, %%mm5\n\t"
		"por	%%mm5, %%mm2\n\t"
		"movq	%%mm3, %%mm5\n\t"
		"movq	%%mm3, %%mm4\n\t"	"movq	%%mm5, %%mm6\n\t"
		"psllq	$7, %%mm3\n\t"		"psllq	$9, %%mm5\n\t"
		"psrlq	$7, %%mm4\n\t"		"psrlq	$9, %%mm6\n\t"
		"por	%%mm4, %%mm3\n\t"	"por	%%mm6, %%mm5\n\t"
		"por	%%mm3, %%mm2\n\t"	"por	%%mm5, %%mm2\n\t"
		"por	%1, %2\n\t"
		"pandn	%%mm2, %2\n\t"

#ifdef POPCOUNT
		"movd	%2, %%ecx\n\t"
		"popcntl %%ecx, %0\n\t"		"andl	$0x00000081, %%ecx\n\t"
		"psrlq	$32, %2\n\t"		"popcntl %%ecx, %%ecx\n\t"
		"movd	%2, %%edx\n\t"		"addl	%%ecx, %0\n\t"
		"popcntl %%edx, %%ecx\n\t"	"andl	$0x81000000, %%edx\n\t"
		"addl	%%ecx, %0\n\t"		"popcntl %%edx, %%edx\n\t"
						"addl	%%edx, %0\n\t"
		"emms"
	: "=g" (count) : "y" (P), "y" (O), "m" (mask_7e), "m" (mask_v)
	: "ecx", "edx", "mm2", "mm3", "mm4", "mm5", "mm6");

#else
		"movq	%2, %1\n\t"		"movq	%2, %%mm2\n\t"
		"psrlq	$1, %2\n\t"
		"pand	%5, %2\n\t"		"pand	%6, %%mm2\n\t"
		"psubd	%2, %1\n\t"		"paddd	%%mm2, %1\n\t"

		"movq	%1, %2\n\t"
		"psrlq	$2, %1\n\t"
		"pand	%7, %2\n\t"
		"pand	%7, %1\n\t"
		"paddd	%2, %1\n\t"

		"movq	%1, %2\n\t"
		"psrlq	$4, %1\n\t"
		"paddd	%2, %1\n\t"
		"pand	%8, %1\n\t"
	#ifdef hasSSE2
		"pxor	%2, %2\n\t"
		"psadbw	%2, %1\n\t"
		"movd	%1, %0\n\t"
	#else
		"movq	%1, %2\n\t"
		"psrlq	$32, %1\n\t"
		"paddb	%2, %1\n\t"

		"movd	%1, %0\n\t"
		"imull	$0x01010101, %0, %0\n\t"
		"shrl	$24, %0\n\t"
	#endif
		"emms"
	: "=g" (count)
	: "y" (P), "y" (O), "m" (mask_7e), "m" (mask_v),
	  "m" (mask_15), "m" (mask_01), "m" (mask_33), "m" (mask_0F)
	: "mm2", "mm3", "mm4", "mm5", "mm6");
#endif

	return count;
}
#endif

/**
 * @brief MMX translation of board_get_hash_code.
 *
 * @param p pointer to 16 bytes to hash.
 * @return the hash code of the bitboard
 */

#if defined(USE_GAS_MMX) && defined(__3dNOW__)

unsigned long long board_get_hash_code_mmx(const unsigned char *p)
{
	unsigned long long h;

	__asm__ volatile (
		"movq	%0, %%mm0\n\t"		"movq	%1, %%mm1"
	: : "m" (hash_rank[0][p[0]]), "m" (hash_rank[1][p[1]]));
	__asm__ volatile (
		"pxor	%0, %%mm0\n\t"		"pxor	%1, %%mm1"
	: : "m" (hash_rank[2][p[2]]), "m" (hash_rank[3][p[3]]));
	__asm__ volatile (
		"pxor	%0, %%mm0\n\t"		"pxor	%1, %%mm1"
	: : "m" (hash_rank[4][p[4]]), "m" (hash_rank[5][p[5]]));
	__asm__ volatile (
		"pxor	%0, %%mm0\n\t"		"pxor	%1, %%mm1"
	: : "m" (hash_rank[6][p[6]]), "m" (hash_rank[7][p[7]]));
	__asm__ volatile (
		"pxor	%0, %%mm0\n\t"		"pxor	%1, %%mm1"
	: : "m" (hash_rank[8][p[8]]), "m" (hash_rank[9][p[9]]));
	__asm__ volatile (
		"pxor	%0, %%mm0\n\t"		"pxor	%1, %%mm1"
	: : "m" (hash_rank[10][p[10]]), "m" (hash_rank[11][p[11]]));
	__asm__ volatile (
		"pxor	%0, %%mm0\n\t"		"pxor	%1, %%mm1"
	: : "m" (hash_rank[12][p[12]]), "m" (hash_rank[13][p[13]]));
	__asm__ volatile (
		"pxor	%1, %%mm0\n\t"		"pxor	%2, %%mm1\n\t"
		"pxor	%%mm1, %%mm0\n\t"
		"movd	%%mm0, %%eax\n\t"
		"punpckhdq %%mm0, %%mm0\n\t"
		"movd	%%mm0, %%edx\n\t"
		"emms"
	: "=A" (h)
	: "m" (hash_rank[14][p[14]]), "m" (hash_rank[15][p[15]])
	: "mm0", "mm1");

	return h;
}

#endif // __3dNOW

#if !defined(hasMMX) && defined(USE_GAS_MMX)
	#pragma GCC pop_options
#endif
>>>>>>> 1dc032e (Improve visual c compatibility)
