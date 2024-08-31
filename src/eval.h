<<<<<<< HEAD
<<<<<<< HEAD
=======
>>>>>>> 3e1ed4f (fix cr/lf in repository to lf)
/**
 * @file eval.h
 *
 * Evaluation function's header.
 *
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
 * @date 1998 - 2023
=======
 * @date 1998 - 2018
>>>>>>> 4a049b7 (Rewrite eval_open; Free SymetryPacking after init; short int feature)
 * @author Richard Delorme
 * @version 4.5
=======
 * @date 1998 - 2020
 * @author Richard Delorme
 * @version 4.4
>>>>>>> 3e1ed4f (fix cr/lf in repository to lf)
=======
 * @date 1998 - 2022
 * @author Richard Delorme
 * @version 4.5
>>>>>>> fdb3c8a (SWAR vector eval update; more restore in search_restore_midgame)
 */

#ifndef EDAX_EVAL_H
#define EDAX_EVAL_H

<<<<<<< HEAD
<<<<<<< HEAD
#include "bit.h"
=======
#ifdef __SSE2__
	#include <x86intrin.h>
#endif
>>>>>>> 4a049b7 (Rewrite eval_open; Free SymetryPacking after init; short int feature)
=======
#include "bit.h"
>>>>>>> 3e1ed4f (fix cr/lf in repository to lf)

/** number of features */
enum { EVAL_N_FEATURE = 47 };

/**
 * struct Eval
 * @brief evaluation function
 */
<<<<<<< HEAD
<<<<<<< HEAD
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
<<<<<<< HEAD
	EVAL_FEATURE_V feature;                       /**!< discs' features (96) */
	int n_empties;                                /**< number of empty squares (4) */
	unsigned int parity;                          /**< parity (4) */
=======
	union {
		unsigned short us[EVAL_N_FEATURE];         /**!< discs' features */
#ifdef __SSE2__
		__v8hi	v8[6];
#endif
#ifdef __AVX2__
		__v16hi	v16[3];
#endif
	} feature;
	int player;
>>>>>>> 4a049b7 (Rewrite eval_open; Free SymetryPacking after init; short int feature)
=======
typedef struct Eval {
	union {
		unsigned short us[EVAL_N_FEATURE];         /**!< discs' features */
#if defined(hasSSE2) || defined(USE_MSVC_X86)
		__m128i	v8[6];
=======
typedef union {
	unsigned short us[48];
	unsigned long long ull[12];	// SWAR
#ifdef __ARM_NEON__
	int16x8_t v8[6];
#elif defined(hasSSE2) || defined(USE_MSVC_X86)
	__m128i	v8[6];
>>>>>>> f2da03e (Refine arm builds adding neon support.)
#endif
#ifdef __AVX2__
	__m256i	v16[3];
#endif
}
#if defined(__GNUC__) && !defined(hasSSE2)
__attribute__ ((aligned (16)))
#endif
EVAL_FEATURE_V;

typedef struct Eval {
	EVAL_FEATURE_V feature;                       /**!< discs' features */
	int n_empties;                                /**< number of empty squares */
	unsigned int parity;                          /**< parity */
>>>>>>> 3e1ed4f (fix cr/lf in repository to lf)
} Eval;

struct Board;
struct Move;

<<<<<<< HEAD
<<<<<<< HEAD
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
=======
=======
>>>>>>> 3e1ed4f (fix cr/lf in repository to lf)
/** number of (unpacked) weights */
enum { EVAL_N_WEIGHT = 226315 };

/** number of plies */
enum { EVAL_N_PLY = 61 };

extern short (*EVAL_WEIGHT)[EVAL_N_PLY][EVAL_N_WEIGHT];
<<<<<<< HEAD
>>>>>>> 4a049b7 (Rewrite eval_open; Free SymetryPacking after init; short int feature)

<<<<<<< HEAD
/** number of plies */
enum { EVAL_N_PLY = 54 };	// decreased from 60 in 4.5.1

extern Eval_weight (*EVAL_WEIGHT)[EVAL_N_PLY - 2];	// for 2..53
=======

>>>>>>> 3e1ed4f (fix cr/lf in repository to lf)
=======
#ifndef SELECTIVE_EVAL_UPDATE

extern const EVAL_FEATURE_V EVAL_FEATURE[65];
extern const EVAL_FEATURE_V EVAL_FEATURE_all_opponent;

#endif
>>>>>>> f2da03e (Refine arm builds adding neon support.)

/* function declaration */
void eval_open(const char*);
void eval_close(void);
// void eval_init(Eval*);
// void eval_free(Eval*);
void eval_set(Eval*, const struct Board*);
<<<<<<< HEAD
<<<<<<< HEAD
=======
void eval_update(Eval*, const struct Move*);
void eval_update_leaf(Eval*, const Eval*, const struct Move*);
>>>>>>> 3e1ed4f (fix cr/lf in repository to lf)
=======
void eval_update(int, unsigned long long, Eval*);
void eval_update_leaf(int, unsigned long long, Eval*, const Eval*);
>>>>>>> 9b4cd06 (Optimize search_shallow in endgame.c; revise eval_update parameters)
void eval_restore(Eval*, const struct Move*);
void eval_pass(Eval*);
double eval_sigma(const int, const int, const int);

<<<<<<< HEAD
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

=======
/**
 * @file eval.h
 *
 * Evaluation function's header.
 *
 * @date 1998 - 2020
 * @author Richard Delorme
 * @version 4.4
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
typedef struct Eval {
	union {
		unsigned short us[EVAL_N_FEATURE];         /**!< discs' features */
#if defined(hasSSE2) || defined(USE_MSVC_X86)
		__m128i	v8[6];
#endif
#ifdef __AVX2__
		__m256i	v16[3];
#endif
	} feature;
	int n_empties;                                /**< number of empty squares */
	unsigned int parity;                          /**< parity */
} Eval;

struct Board;
struct Move;

/** number of (unpacked) weights */
enum { EVAL_N_WEIGHT = 226315 };

/** number of plies */
enum { EVAL_N_PLY = 61 };

extern short (*EVAL_WEIGHT)[EVAL_N_PLY][EVAL_N_WEIGHT];


/* function declaration */
void eval_open(const char*);
void eval_close(void);
// void eval_init(Eval*);
// void eval_free(Eval*);
void eval_set(Eval*, const struct Board*);
void eval_update(Eval*, const struct Move*);
void eval_update_leaf(Eval*, const Eval*, const struct Move*);
void eval_restore(Eval*, const struct Move*);
void eval_pass(Eval*);
double eval_sigma(const int, const int, const int);

#endif

>>>>>>> 1dc032e (Improve visual c compatibility)
=======
#endif

>>>>>>> 3e1ed4f (fix cr/lf in repository to lf)
