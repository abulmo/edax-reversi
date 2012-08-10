/**
 * @file eval.h
 *
 * Evaluation function's header.
 *
 * @date 1998 - 2012
 * @author Richard Delorme
 * @version 4.3
 */

#ifndef EDAX_EVAL_H
#define EDAX_EVAL_H

/**
 * struct Eval
 * @brief evaluation function
 */
typedef struct Eval {
	int *feature;         /**!< discs' features */
	int player;
} Eval;

struct Board;
struct Move;

extern short ***EVAL_WEIGHT;


/* function declaration */
void eval_open(const char*);
void eval_close(void);
void eval_init(Eval*);
void eval_free(Eval*);
void eval_set(Eval*, const struct Board*);
void eval_update(Eval*, const struct Move*);
void eval_restore(Eval*, const struct Move*);
void eval_pass(Eval*);
double eval_sigma(const int, const int, const int);

#endif

