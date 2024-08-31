/**
 * @file eval.h
 *
 * Evaluation function's header.
 *
 * @date 1998 - 2023
 * @author Richard Delorme
 * @version 4.5
 */

#ifndef EDAX_EVAL_H
#define EDAX_EVAL_H

#include "bit.h"

/** number of features */
enum { EVAL_N_FEATURE = 47 };

/**
 * struct Eval
 * @brief evaluation function
 */
typedef union {
	unsigned short us[48];
	unsigned long long ull[12];	// SWAR
#ifdef __ARM_NEON
	int16x8_t v8[6];
#elif defined(hasSSE2) || defined(USE_MSVC_X86)
	__m128i	v8[6];
#endif
#ifdef __AVX2__
	__m256i	v16[3];
#endif
} EVAL_FEATURE_V;

typedef struct Eval {
	EVAL_FEATURE_V feature;                       /**!< discs' features (96) */
	int n_empties;                                /**< number of empty squares (4) */
	unsigned int parity;                          /**< parity (4) */
} Eval;

struct Board;
struct Move;

/** unpacked weights */
// enum { EVAL_N_WEIGHT = 226315 };
typedef struct Eval_weight {
	short	S0;		// also acts as guard for VGATHERDD access
	short	C9[19683];
	short	C10[59049];
	short	S100[59049];
	short	S101[59049];
	short	S8x4[6561*4];
	short	S7654[2187+729+243+81];
} Eval_weight;

/** number of plies */
enum { EVAL_N_PLY = 54 };	// decreased from 60 in 4.5.1

extern Eval_weight (*EVAL_WEIGHT)[EVAL_N_PLY - 2];	// for 2..53

/* function declaration */
void eval_open(const char*);
void eval_close(void);
// void eval_init(Eval*);
// void eval_free(Eval*);
void eval_set(Eval*, const struct Board*);
void eval_restore(Eval*, const struct Move*);
void eval_pass(Eval*);
double eval_sigma(const int, const int, const int);

#if defined(hasSSE2) || defined(__ARM_NEON) || defined(USE_MSVC_X86) || defined(ANDROID)
void eval_update_sse(int, unsigned long long, Eval *, const Eval *);
#endif
#if defined(hasSSE2) || defined(__ARM_NEON)
#define	eval_update(x, f, eval)	eval_update_sse(x, f, eval, eval)
#define	eval_update_leaf(x, f, eval_out, eval_in)	eval_update_sse(x, f, eval_out, eval_in)
#else
void eval_update(int, unsigned long long, Eval*);
void eval_update_leaf(int, unsigned long long, Eval*, const Eval*);
#endif

#endif

