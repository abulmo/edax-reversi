/**
 * @file eval.h
 *
 * Evaluation function's header.
 *
 * @date 1998 - 2024
 * @author Richard Delorme
 * @version 4.6
 */

#ifndef EDAX_EVAL_H
#define EDAX_EVAL_H

#include "simd.h"

/**
 * struct Feature
 * @brief evaluation pattern
 */
typedef union Feature {
	uint16_t v1[48];
	uint64_t v4[12];
#ifdef __ARM_NEON
	int16x8_t v8[6];
#elif defined(__SSE2__)
	__m128i	v8[6];
#endif
#ifdef __AVX2__
	__m256i	v16[3];
#endif
} Feature;

/**
 * struct Eval
 * @brief evaluation function
 */
typedef struct Eval {
	Feature *feature;         /**!< discs' features */
	int player;
	int ply;
} Eval;

struct Board;
struct Move;

/* function declaration */
void eval_init(Eval*);
void eval_free(Eval*);
void eval_open(const char*);
void eval_close(void);
void eval_set(Eval*, const struct Board*);
void eval_update(Eval*, const struct Move*);
void eval_restore(Eval*);
void eval_pass(Eval*);
int eval_accumulate(const Eval*);
double eval_sigma(const int, const int, const int);

#endif

