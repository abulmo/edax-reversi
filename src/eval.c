/**
 * @file eval.c
 *
 * Evaluation function.
 *
 * @date 1998 - 2017
 * @author Richard Delorme
 * @version 4.4
 */

#include "eval.h"

#include "bit.h"
#include "board.h"
#include "options.h"
#include "move.h"
#include "util.h"

#include <stdlib.h>
#include <assert.h>

/** coordinate to feature conversion */
typedef struct CoordinateToFeature {
	int n_feature;
	struct {
		int i;
		int x;
	} feature[16];
} CoordinateToFeature;

/** feature to coordinates conversion */
typedef struct FeatureToCoordinate {
	int n_square;
	int x[16];
} FeatureToCoordinate;

/** array to convert features into coordinates */
static const FeatureToCoordinate EVAL_F2X[] = {
	{ 9, {A1, B1, A2, B2, C1, A3, C2, B3, C3}},
	{ 9, {H1, G1, H2, G2, F1, H3, F2, G3, F3}},
	{ 9, {A8, A7, B8, B7, A6, C8, B6, C7, C6}},
	{ 9, {H8, H7, G8, G7, H6, F8, G6, F7, F6}},

	{10, {A5, A4, A3, A2, A1, B2, B1, C1, D1, E1}},
	{10, {H5, H4, H3, H2, H1, G2, G1, F1, E1, D1}},
	{10, {A4, A5, A6, A7, A8, B7, B8, C8, D8, E8}},
	{10, {H4, H5, H6, H7, H8, G7, G8, F8, E8, D8}},

	{10, {B2, A1, B1, C1, D1, E1, F1, G1, H1, G2}},
	{10, {B7, A8, B8, C8, D8, E8, F8, G8, H8, G7}},
	{10, {B2, A1, A2, A3, A4, A5, A6, A7, A8, B7}},
	{10, {G2, H1, H2, H3, H4, H5, H6, H7, H8, G7}},

	{10, {A1, C1, D1, C2, D2, E2, F2, E1, F1, H1}},
	{10, {A8, C8, D8, C7, D7, E7, F7, E8, F8, H8}},
	{10, {A1, A3, A4, B3, B4, B5, B6, A5, A6, A8}},
	{10, {H1, H3, H4, G3, G4, G5, G6, H5, H6, H8}},

	{ 8, {A2, B2, C2, D2, E2, F2, G2, H2}},
	{ 8, {A7, B7, C7, D7, E7, F7, G7, H7}},
	{ 8, {B1, B2, B3, B4, B5, B6, B7, B8}},
	{ 8, {G1, G2, G3, G4, G5, G6, G7, G8}},

	{ 8, {A3, B3, C3, D3, E3, F3, G3, H3}},
	{ 8, {A6, B6, C6, D6, E6, F6, G6, H6}},
	{ 8, {C1, C2, C3, C4, C5, C6, C7, C8}},
	{ 8, {F1, F2, F3, F4, F5, F6, F7, F8}},

	{ 8, {A4, B4, C4, D4, E4, F4, G4, H4}},
	{ 8, {A5, B5, C5, D5, E5, F5, G5, H5}},
	{ 8, {D1, D2, D3, D4, D5, D6, D7, D8}},
	{ 8, {E1, E2, E3, E4, E5, E6, E7, E8}},

	{ 8, {A1, B2, C3, D4, E5, F6, G7, H8}},
	{ 8, {A8, B7, C6, D5, E4, F3, G2, H1}},

	{ 7, {B1, C2, D3, E4, F5, G6, H7}},
	{ 7, {H2, G3, F4, E5, D6, C7, B8}},
	{ 7, {A2, B3, C4, D5, E6, F7, G8}},
	{ 7, {G1, F2, E3, D4, C5, B6, A7}},

	{ 6, {C1, D2, E3, F4, G5, H6}},
	{ 6, {A3, B4, C5, D6, E7, F8}},
	{ 6, {F1, E2, D3, C4, B5, A6}},
	{ 6, {H3, G4, F5, E6, D7, C8}},

	{ 5, {D1, E2, F3, G4, H5}},
	{ 5, {A4, B5, C6, D7, E8}},
	{ 5, {E1, D2, C3, B4, A5}},
	{ 5, {H4, G5, F6, E7, D8}},

	{ 4, {D1, C2, B3, A4}},
	{ 4, {A5, B6, C7, D8}},
	{ 4, {E1, F2, G3, H4}},
	{ 4, {H5, G6, F7, E8}},

	{ 0, {NOMOVE}}
};

/** array to convert coordinates into feature */
static const CoordinateToFeature EVAL_X2F[] = {
	{7, {{ 0,    6561}, { 4,     243}, { 8,    6561}, {10,    6561}, {12,   19683}, {14,   19683}, {28,    2187}}},  /* a1 */
	{5, {{ 0,    2187}, { 4,      27}, { 8,    2187}, {18,    2187}, {30,     729}}},                                /* b1 */
	{6, {{ 0,      81}, { 4,       9}, { 8,     729}, {12,    6561}, {22,    2187}, {34,     243}}},                 /* c1 */
	{7, {{ 4,       3}, { 5,       1}, { 8,     243}, {12,    2187}, {26,    2187}, {38,      81}, {42,      27}}},  /* d1 */
	{7, {{ 4,       1}, { 5,       3}, { 8,      81}, {12,       9}, {27,    2187}, {40,      81}, {44,      27}}},  /* e1 */
	{6, {{ 1,      81}, { 5,       9}, { 8,      27}, {12,       3}, {23,    2187}, {36,     243}}},                 /* f1 */
	{5, {{ 1,    2187}, { 5,      27}, { 8,       9}, {19,    2187}, {33,     729}}},                                /* g1 */
	{7, {{ 1,    6561}, { 5,     243}, { 8,       3}, {11,    6561}, {12,       1}, {15,   19683}, {29,       1}}},  /* h1 */
	{5, {{ 0,     729}, { 4,     729}, {10,    2187}, {16,    2187}, {32,     729}}},                                /* a2 */
	{7, {{ 0,     243}, { 4,      81}, { 8,   19683}, {10,   19683}, {16,     729}, {18,     729}, {28,     729}}},  /* b2 */
	{6, {{ 0,       9}, {12,     729}, {16,     243}, {22,     729}, {30,     243}, {42,       9}}},                 /* c2 */
	{5, {{12,     243}, {16,      81}, {26,     729}, {34,      81}, {40,      27}}},                                /* d2 */
	{5, {{12,      81}, {16,      27}, {27,     729}, {36,      81}, {38,      27}}},                                /* e2 */
	{6, {{ 1,       9}, {12,      27}, {16,       9}, {23,     729}, {33,     243}, {44,       9}}},                 /* f2 */
	{7, {{ 1,     243}, { 5,      81}, { 8,       1}, {11,   19683}, {16,       3}, {19,     729}, {29,       3}}},  /* g2 */
	{5, {{ 1,     729}, { 5,     729}, {11,    2187}, {16,       1}, {31,     729}}},                                /* h2 */
	{6, {{ 0,      27}, { 4,    2187}, {10,     729}, {14,    6561}, {20,    2187}, {35,     243}}},                 /* a3 */
	{6, {{ 0,       3}, {14,     729}, {18,     243}, {20,     729}, {32,     243}, {42,       3}}},                 /* b3 */
	{5, {{ 0,       1}, {20,     243}, {22,     243}, {28,     243}, {40,       9}}},                                /* c3 */
	{4, {{20,      81}, {26,     243}, {30,      81}, {36,      27}}},                                               /* d3 */
	{4, {{20,      27}, {27,     243}, {33,      81}, {34,      27}}},                                               /* e3 */
	{5, {{ 1,       1}, {20,       9}, {23,     243}, {29,       9}, {38,       9}}},                                /* f3 */
	{6, {{ 1,       3}, {15,     729}, {19,     243}, {20,       3}, {31,     243}, {44,       3}}},                 /* g3 */
	{6, {{ 1,      27}, { 5,    2187}, {11,     729}, {15,    6561}, {20,       1}, {37,     243}}},                 /* h3 */
	{7, {{ 4,    6561}, { 6,   19683}, {10,     243}, {14,    2187}, {24,    2187}, {39,      81}, {42,       1}}},  /* a4 */
	{5, {{14,     243}, {18,      81}, {24,     729}, {35,      81}, {40,       3}}},                                /* b4 */
	{4, {{22,      81}, {24,     243}, {32,      81}, {36,       9}}},                                               /* c4 */
	{4, {{24,      81}, {26,      81}, {28,      81}, {33,      27}}},                                               /* d4 */
	{4, {{24,      27}, {27,      81}, {29,      27}, {30,      27}}},                                               /* e4 */
	{4, {{23,      81}, {24,       9}, {31,      81}, {34,       9}}},                                               /* f4 */
	{5, {{15,     243}, {19,      81}, {24,       3}, {37,      81}, {38,       3}}},                                /* g4 */
	{7, {{ 5,    6561}, { 7,   19683}, {11,     243}, {15,    2187}, {24,       1}, {41,      81}, {44,       1}}},  /* h4 */
	{7, {{ 4,   19683}, { 6,    6561}, {10,      81}, {14,       9}, {25,    2187}, {40,       1}, {43,      27}}},  /* a5 */
	{5, {{14,      81}, {18,      27}, {25,     729}, {36,       3}, {39,      27}}},                                /* b5 */
	{4, {{22,      27}, {25,     243}, {33,       9}, {35,      27}}},                                               /* c5 */
	{4, {{25,      81}, {26,      27}, {29,      81}, {32,      27}}},                                               /* d5 */
	{4, {{25,      27}, {27,      27}, {28,      27}, {31,      27}}},                                               /* e5 */
	{4, {{23,      27}, {25,       9}, {30,       9}, {37,      27}}},                                               /* f5 */
	{5, {{15,      81}, {19,      27}, {25,       3}, {34,       3}, {41,      27}}},                                /* g5 */
	{7, {{ 5,   19683}, { 7,    6561}, {11,      81}, {15,       9}, {25,       1}, {38,       1}, {45,      27}}},  /* h5 */
	{6, {{ 2,      81}, { 6,    2187}, {10,      27}, {14,       3}, {21,    2187}, {36,       1}}},                 /* a6 */
	{6, {{ 2,       9}, {14,      27}, {18,       9}, {21,     729}, {33,       3}, {43,       9}}},                 /* b6 */
	{5, {{ 2,       1}, {21,     243}, {22,       9}, {29,     243}, {39,       9}}},                                /* c6 */
	{4, {{21,      81}, {26,       9}, {31,       9}, {35,       9}}},                                               /* d6 */
	{4, {{21,      27}, {27,       9}, {32,       9}, {37,       9}}},                                               /* e6 */
	{5, {{ 3,       1}, {21,       9}, {23,       9}, {28,       9}, {41,       9}}},                                /* f6 */
	{6, {{ 3,       9}, {15,      27}, {19,       9}, {21,       3}, {30,       3}, {45,       9}}},                 /* g6 */
	{6, {{ 3,      81}, { 7,    2187}, {11,      27}, {15,       3}, {21,       1}, {34,       1}}},                 /* h6 */
	{5, {{ 2,    2187}, { 6,     729}, {10,       9}, {17,    2187}, {33,       1}}},                                /* a7 */
	{7, {{ 2,     243}, { 6,      81}, { 9,   19683}, {10,       1}, {17,     729}, {18,       3}, {29,     729}}},  /* b7 */
	{6, {{ 2,       3}, {13,     729}, {17,     243}, {22,       3}, {31,       3}, {43,       3}}},                 /* c7 */
	{5, {{13,     243}, {17,      81}, {26,       3}, {37,       3}, {39,       3}}},                                /* d7 */
	{5, {{13,      81}, {17,      27}, {27,       3}, {35,       3}, {41,       3}}},                                /* e7 */
	{6, {{ 3,       3}, {13,      27}, {17,       9}, {23,       3}, {32,       3}, {45,       3}}},                 /* f7 */
	{7, {{ 3,     243}, { 7,      81}, { 9,       1}, {11,       1}, {17,       3}, {19,       3}, {28,       3}}},  /* g7 */
	{5, {{ 3,    2187}, { 7,     729}, {11,       9}, {17,       1}, {30,       1}}},                                /* h7 */
	{7, {{ 2,    6561}, { 6,     243}, { 9,    6561}, {10,       3}, {13,   19683}, {14,       1}, {29,    2187}}},  /* a8 */
	{5, {{ 2,     729}, { 6,      27}, { 9,    2187}, {18,       1}, {31,       1}}},                                /* b8 */
	{6, {{ 2,      27}, { 6,       9}, { 9,     729}, {13,    6561}, {22,       1}, {37,       1}}},                 /* c8 */
	{7, {{ 6,       3}, { 7,       1}, { 9,     243}, {13,    2187}, {26,       1}, {41,       1}, {43,       1}}},  /* d8 */
	{7, {{ 6,       1}, { 7,       3}, { 9,      81}, {13,       9}, {27,       1}, {39,       1}, {45,       1}}},  /* e8 */
	{6, {{ 3,      27}, { 7,       9}, { 9,      27}, {13,       3}, {23,       1}, {35,       1}}},                 /* f8 */
	{5, {{ 3,     729}, { 7,      27}, { 9,       9}, {19,       1}, {32,       1}}},                                /* g8 */
	{7, {{ 3,    6561}, { 7,     243}, { 9,       3}, {11,       3}, {13,       1}, {15,       1}, {28,       1}}},  /* h8 */
	{0, {{ 0, 0}}} // <- PASS
};

/** feature size */
static const int EVAL_SIZE[] = {19683, 59049, 59049, 59049, 6561, 6561, 6561, 6561, 2187,729, 243, 81, 1};

/** packed feature size */
static const int EVAL_PACKED_SIZE[] = {10206, 29889, 29646, 29646, 3321, 3321, 3321, 3321, 1134, 378, 135, 45, 1};

/** feature offset */
static const int EVAL_OFFSET[] = {
	     0,      0,      0,      0,
	 19683,  19683,  19683,  19683,
	 78732,  78732,  78732,  78732,
	137781,	137781, 137781, 137781,
	196830,	196830, 196830, 196830,
	203391,	203391, 203391, 203391,
	209952,	209952, 209952, 209952,
	216513,	216513,
	223074,	223074,	223074,	223074,
	225261,	225261,	225261,	225261,
	225990,	225990, 225990,	225990,
	226233,	226233, 226233, 226233,
	226314,
};

static const int EVAL_MAX_VALUE[] = {
	 19682,  19682,  19682,  19682,
	 78731,  78731,  78731,  78731,
	137780,	137780, 137780, 137780,
	196829,	196829, 196829, 196829,
	203390,	203390, 203390, 203390,
	209951,	209951, 209951, 209951,
	216512,	216512,
	223073,	223073,	223073,	223073,
	225260,	225260,	225260,	225260,
	225989,	225989, 225989,	225989,
	226232,	226232, 226232, 226232,
	226313,	226313,	226313,	226313,
	226314,
};

/** feature symetry packing */
static int EVAL_C10[2][59049];
static int EVAL_S10[2][59049];
static int EVAL_C9[2][19683];
static int EVAL_S8[2][6561];
static int EVAL_S7[2][2187];
static int EVAL_S6[2][729];
static int EVAL_S5[2][243];
static int EVAL_S4[2][81];

/** number of (unpacked) weights */
static const int EVAL_N_WEIGHT = 226315;

/** number of plies */
static const int EVAL_N_PLY = 61;

/** number of features */
static const int EVAL_N_FEATURE = 47;

/** eval weight load status */
static int EVAL_LOADED = 0;

/** eval weights */
short ***EVAL_WEIGHT;

/** evaluation function error coefficient parameters */
static double EVAL_A, EVAL_B, EVAL_C, EVAL_a, EVAL_b, EVAL_c;


/**
 * @brief Opponent feature.
 *
 * Compute a feature from the opponent point of view.
 * @param l feature.
 * @param d feature size.
 * @return opponent feature.
 */
static int opponent_feature(int l, int d)
{
	static const int o[] = {1, 0, 2};
	int f = o[l % 3];

	if (d > 1) f += opponent_feature(l / 3, d - 1) * 3;

	return f;
}

/**
 * @brief Load the evaluation function features' weights.
 *
 * The weights are stored in a global variable, because, once loaded from the
 * file, they stay constant during the lifetime of the program. As loading
 * the weights is time & resource consuming, a counter variable check that
 * the weights are effectively loaded only once.
 *
 * @param file File name of the evaluation function data.
 */
void eval_open(const char* file)
{
	unsigned int edax_header, eval_header;
	unsigned int version, release, build;
	double date;
	const int n_w = 114364;
	int *T;
	int ply, i, j, k, l, n;
	int r;
	int offset;
	FILE* f;
	short *w = NULL;

	if (EVAL_LOADED++) return;

	// the following is assumed:
	//	-(unsigned) int are 32 bits
	if (sizeof (int) != 4) fatal_error("int size is not compatible with Edax.\n");
	//	-(unsigned) short are 16 bits
	if (sizeof (short) != 2) fatal_error("short size is not compatible with Edax.\n");

	// create unpacking tables
	T = (int*) malloc(59049 * sizeof (*T));
	if (T == NULL) fatal_error("Cannot allocate temporary table variable.\n");

	for (l = n = 0; l < 6561; l++){ /* 8 squares : 6561 -> 3321 */
		k=((l / 2187) % 3) + ((l / 729) % 3) * 3 + ((l / 243) % 3) * 9 +
		((l / 81) % 3) * 27 + ((l / 27) % 3) * 81 + ((l / 9) % 3) * 243 +
		((l / 3) % 3) * 729 + (l % 3) * 2187;
		if (k < l) T[l] = T[k];
		else T[l] = n++;
		EVAL_S8[0][l] = T[l];
		EVAL_S8[1][opponent_feature(l, 8)] = T[l];
	}
	for (l = n = 0; l < 2187; l++){ /* 7 squares : 2187 -> 1134 */
		k=((l / 729) % 3) + ((l / 243) % 3) * 3 + ((l / 81) % 3) * 9 +
		 ((l / 27) % 3) * 27 + ((l / 9) % 3) * 81 + ((l / 3) % 3) * 243 +
		 (l % 3) * 729;
		if (k < l) T[l] = T[k];
		else T[l] = n++;
		EVAL_S7[0][l] = T[l];
		EVAL_S7[1][opponent_feature(l, 7)] = T[l];
	}
	for (l = n = 0; l < 729; l++){ /* 6 squares : 729 -> 378 */
		k=((l / 243) % 3) + ((l / 81) % 3) * 3 + ((l / 27) % 3) * 9 +
		 ((l / 9) % 3) * 27 + ((l / 3) % 3) * 81 + (l % 3) * 243;
		if (k < l) T[l]=T[k];
		else T[l] = n++;
		EVAL_S6[0][l] = T[l];
		EVAL_S6[1][opponent_feature(l, 6)] = T[l];
	}
	for (l = n = 0; l < 243; l++) { /* 5 squares : 243 -> 135 */
		k=((l / 81) % 3)+((l / 27) % 3) * 3 + ((l / 9) % 3) * 9+
		((l / 3) % 3) * 27 + (l % 3) * 81;
		if (k < l) T[l] = T[k];
		else T[l] = n++;
		EVAL_S5[0][l] = T[l];
		EVAL_S5[1][opponent_feature(l, 5)] = T[l];
	}
	for (l = n = 0; l < 81; l++) { /* 4 squares : 81 -> 45 */
		k=((l / 27) % 3) + ((l / 9) % 3) * 3 + ((l / 3) % 3) * 9 + (l % 3) * 27;
		if (k < l) T[l] = T[k];
		else T[l] = n++;
		EVAL_S4[0][l] = T[l];
		EVAL_S4[1][opponent_feature(l, 4)] = T[l];
	}
	for (l = n = 0;l < 19683; l++) { /* 9 corner squares : 19683 -> 10206 */
		k = ((l / 6561) % 3) * 6561 + ((l / 729) % 3) * 2187 +
		((l / 2187) % 3) * 729 + ((l / 243) % 3) * 243 +((l / 27) % 3) * 81 +
		((l / 81) % 3) * 27 + ((l / 3) % 3) * 9 + ((l / 9) % 3) * 3 + (l % 3);
		if (k < l) T[l] = T[k];
		else T[l] = n++;
		EVAL_C9[0][l] = T[l];
		EVAL_C9[1][opponent_feature(l, 9)] = T[l];
	}
	for (l = n = 0; l < 59049; l++) { /* 10 squares (edge +X ) : 59049 -> 29646 */
		k=((l / 19683) % 3) + ((l / 6561) % 3) * 3+((l / 2187) % 3) * 9 +
		  ((l / 729) % 3) * 27 + ((l / 243) % 3) * 81 + ((l / 81) % 3) * 243 +
		  ((l / 27) % 3) * 729 + ((l / 9) % 3) * 2187 + ((l / 3) % 3) * 6561 +
		  (l % 3) * 19683;
		if (k < l) T[l] = T[k];
		else T[l] = n++;
		EVAL_S10[0][l] = T[l];
		EVAL_S10[1][opponent_feature(l, 10)] = T[l];
	}
	for (l = n = 0; l < 59049; l++) { /* 10 squares (angle + X) : 59049 -> 29889 */
		k=((l / 19683) % 3) + ((l / 6561) % 3) * 3+((l / 2187) % 3) * 9 +
		  ((l / 729) % 3) * 27 + ((l / 243) % 3) * 243 + ((l / 81) % 3) * 81 +
		  ((l / 27) % 3) * 729 + ((l / 9) % 3) * 2187 + ((l / 3) % 3) * 6561 +
		  (l % 3) * 19683;
		if (k < l) T[l] = T[k];
		else T[l] = n++;
		EVAL_C10[0][l] = T[l];
		EVAL_C10[1][opponent_feature(l, 10)] = T[l];
	}
	free(T);

	// allocation
	EVAL_WEIGHT = (short***) malloc(2 * sizeof (*EVAL_WEIGHT));
	if (EVAL_WEIGHT == NULL) fatal_error("Cannot evaluation weights.\n");
	EVAL_WEIGHT[0] = (short**) malloc(2 * EVAL_N_PLY * sizeof (**EVAL_WEIGHT));
	if (EVAL_WEIGHT[0] == NULL) fatal_error("Cannot evaluation weights.\n");
	EVAL_WEIGHT[1] = EVAL_WEIGHT[0] + EVAL_N_PLY;
	EVAL_WEIGHT[0][0] = (short*) malloc(2 * EVAL_N_PLY * EVAL_N_WEIGHT * sizeof (***EVAL_WEIGHT));
	if (EVAL_WEIGHT[0][0] == NULL) fatal_error("Cannot evaluation weights.\n");
	EVAL_WEIGHT[1][0] = EVAL_WEIGHT[0][0] + EVAL_N_PLY * EVAL_N_WEIGHT;
	for (ply = 1; ply < EVAL_N_PLY; ply++) {
		EVAL_WEIGHT[0][ply] = EVAL_WEIGHT[0][ply - 1] + EVAL_N_WEIGHT;
		EVAL_WEIGHT[1][ply] = EVAL_WEIGHT[1][ply - 1] + EVAL_N_WEIGHT;
	}

	// data reading
	w = (short*) malloc(n_w * sizeof (*w)); // a temporary to read packed weights
	f = fopen(file, "rb");
	if (f == NULL) {
		fprintf(stderr, "Cannot open %s", file);
		exit(EXIT_FAILURE);
	}

	// File header
	r = fread(&edax_header, sizeof (int), 1, f);
	r += fread(&eval_header, sizeof (int), 1, f);
	if (r != 2 || (!(edax_header == EDAX || eval_header == EVAL) && !(edax_header == XADE || eval_header == LAVE))) fatal_error("%s is not an Edax evaluation file\n", file);
	r = fread(&version, sizeof (int), 1, f);
	r += fread(&release, sizeof (int), 1, f);
	r += fread(&build, sizeof (int), 1, f);
	r += fread(&date, sizeof (double), 1, f);
	if (r != 4) fatal_error("Cannot read version info from %s\n", file);
	if (edax_header == XADE) {
		version = bswap_int(version);
		release = bswap_int(release);
		build = bswap_int(build);
	}
	// Weights : read & unpacked them
	for (ply = 0; ply < EVAL_N_PLY; ply++) {
		r = fread(w, sizeof (short), n_w, f);
		if (r != n_w) fatal_error("Cannot read evaluation weight from %s\n", file);
		if (edax_header == XADE) for (i = 0; i < n_w; ++i) w[i] = bswap_short(w[i]);
		i = j = offset = 0;
		for (k = 0; k < EVAL_SIZE[i]; k++,j++) {
			EVAL_WEIGHT[0][ply][j] = w[EVAL_C9[0][k] + offset];
			EVAL_WEIGHT[1][ply][j] = w[EVAL_C9[1][k] + offset];
		}
		offset += EVAL_PACKED_SIZE[i];
		i++;
		for (k = 0; k < EVAL_SIZE[i]; k++,j++) {
			EVAL_WEIGHT[0][ply][j] = w[EVAL_C10[0][k] + offset];
			EVAL_WEIGHT[1][ply][j] = w[EVAL_C10[1][k] + offset];
		}
		offset += EVAL_PACKED_SIZE[i];
		i++;
		for (k = 0; k < EVAL_SIZE[i]; k++,j++) {
			EVAL_WEIGHT[0][ply][j] = w[EVAL_S10[0][k] + offset];
			EVAL_WEIGHT[1][ply][j] = w[EVAL_S10[1][k] + offset];
		}
		offset += EVAL_PACKED_SIZE[i];
		i++;
		for (k = 0; k < EVAL_SIZE[i]; k++,j++) {
			EVAL_WEIGHT[0][ply][j] = w[EVAL_S10[0][k] + offset];
			EVAL_WEIGHT[1][ply][j] = w[EVAL_S10[1][k] + offset];
		}
		offset += EVAL_PACKED_SIZE[i];
		i++;
		for (k = 0; k < EVAL_SIZE[i]; k++,j++) {
			EVAL_WEIGHT[0][ply][j] = w[EVAL_S8[0][k] + offset];
			EVAL_WEIGHT[1][ply][j] = w[EVAL_S8[1][k] + offset];
		}
		offset += EVAL_PACKED_SIZE[i];
		i++;
		for (k = 0; k < EVAL_SIZE[i]; k++,j++) {
			EVAL_WEIGHT[0][ply][j] = w[EVAL_S8[0][k] + offset];
			EVAL_WEIGHT[1][ply][j] = w[EVAL_S8[1][k] + offset];
		}
		offset += EVAL_PACKED_SIZE[i];
		i++;
		for (k = 0; k < EVAL_SIZE[i]; k++,j++) {
			EVAL_WEIGHT[0][ply][j] = w[EVAL_S8[0][k] + offset];
			EVAL_WEIGHT[1][ply][j] = w[EVAL_S8[1][k] + offset];
		}
		offset += EVAL_PACKED_SIZE[i];
		i++;
		for (k = 0; k < EVAL_SIZE[i]; k++,j++) {
			EVAL_WEIGHT[0][ply][j] = w[EVAL_S8[0][k] + offset];
			EVAL_WEIGHT[1][ply][j] = w[EVAL_S8[1][k] + offset];
		}
		offset += EVAL_PACKED_SIZE[i];
		i++;
		for (k = 0; k < EVAL_SIZE[i]; k++,j++) {
			EVAL_WEIGHT[0][ply][j] = w[EVAL_S7[0][k] + offset];
			EVAL_WEIGHT[1][ply][j] = w[EVAL_S7[1][k] + offset];
		}
		offset += EVAL_PACKED_SIZE[i];
		i++;
		for (k = 0; k < EVAL_SIZE[i]; k++,j++) {
			EVAL_WEIGHT[0][ply][j] = w[EVAL_S6[0][k] + offset];
			EVAL_WEIGHT[1][ply][j] = w[EVAL_S6[1][k] + offset];
		}
		offset += EVAL_PACKED_SIZE[i];
		i++;
		for (k = 0; k < EVAL_SIZE[i]; k++,j++) {
			EVAL_WEIGHT[0][ply][j] = w[EVAL_S5[0][k] + offset];
			EVAL_WEIGHT[1][ply][j] = w[EVAL_S5[1][k] + offset];
		}
		offset += EVAL_PACKED_SIZE[i];
		i++;
		for (k = 0; k < EVAL_SIZE[i]; k++,j++) {
			EVAL_WEIGHT[0][ply][j] = w[EVAL_S4[0][k] + offset];
			EVAL_WEIGHT[1][ply][j] = w[EVAL_S4[1][k] + offset];
		}
		offset += EVAL_PACKED_SIZE[i];
		i++;
		EVAL_WEIGHT[0][ply][j] = w[offset];
		EVAL_WEIGHT[1][ply][j] = w[offset];
	}

	fclose(f);
	free(w);

	/*if (version == 3 && release == 2 && build == 5)*/ {
		EVAL_A = -0.10026799, EVAL_B = 0.31027733, EVAL_C = -0.57772603;
		EVAL_a = 0.07585621, EVAL_b = 1.16492647, EVAL_c = 5.4171698;
	}

	info("<Evaluation function weights version %u.%u.%u loaded>\n", version, release, build);
}

/**
 * @brief Free global resources allocated to the evaluation function.
 */
void eval_close(void)
{
	free(EVAL_WEIGHT[0][0]);
	free(EVAL_WEIGHT[0]);
	free(EVAL_WEIGHT);
	EVAL_WEIGHT = NULL;
}

/**
 * @brief Initialize a new evaluation function.
 *
 * Allocate space to store the state of the evaluation function.
 *
 * @param eval Evaluation function.
 */
void eval_init(Eval *eval)
{
	eval->feature = (int*) malloc(EVAL_N_FEATURE * sizeof (*eval->feature));
}

/**
 * @brief Free resources used by the evaluation function.
 *
 * @param eval Evaluation function.
 */
void eval_free(Eval *eval)
{
	free(eval->feature);
}

/**
 * @brief Set up evaluation features from a board.
 *
 * @param eval  Evaluation function.
 * @param board Board to setup features from.
 */
void eval_set(Eval *eval, const Board *board)
{
	int i, j, c;

	for (i = 0; i < EVAL_N_FEATURE; ++i) {
		eval->feature[i] = 0;
		for (j = 0; j < EVAL_F2X[i].n_square; j++) {
			c = board_get_square_color(board, EVAL_F2X[i].x[j]);
			eval->feature[i] = eval->feature[i] * 3 + c;
		}
		eval->feature[i] += EVAL_OFFSET[i];
	}
	eval->player = 0;
}

/**
 * @brief Swap player's feature.
 *
 * @param eval  Evaluation function.
 */
static void eval_swap(Eval *eval)
{
	eval->player ^= 1;
}


/**
 * @brief Update the features after a player's move.
 *
 * @param eval  Evaluation function.
 * @param move  Move.
 */
static void eval_update_0(Eval *eval, const Move *move)
{
	const CoordinateToFeature *s = EVAL_X2F + move->x;
	register int x;
	int *feature = eval->feature;
	unsigned long long f = move->flipped;
	
#ifdef DEBUG

	int i, j;
	for (i = 0; i < s->n_feature; ++i) {
		j = s->feature[i].i;
		assert(0 <= j && j < EVAL_N_FEATURE);
		feature[j] -= 2 * s->feature[i].x;
		assert(EVAL_OFFSET[j] <= feature[j] && feature[j] <= EVAL_MAX_VALUE[j]);
	}

	foreach_bit (x, f) {
		s = EVAL_X2F + x;
		for (i = 0; i < s->n_feature; ++i) {
			j = s->feature[i].i;
			assert(0 <= j && j < EVAL_N_FEATURE);
			feature[j] -= s->feature[i].x;
			assert(EVAL_OFFSET[j] <= feature[j] && feature[j] <= EVAL_MAX_VALUE[j]);
		}
	}

#else

	switch (s->n_feature) {
	case 7:	feature[s->feature[6].i] -= 2 * s->feature[6].x;
	case 6:	feature[s->feature[5].i] -= 2 * s->feature[5].x;
	case 5:	feature[s->feature[4].i] -= 2 * s->feature[4].x;
	case 4:	feature[s->feature[3].i] -= 2 * s->feature[3].x;
	case 3:	feature[s->feature[2].i] -= 2 * s->feature[2].x;
	case 2:	feature[s->feature[1].i] -= 2 * s->feature[1].x;
	case 1:	feature[s->feature[0].i] -= 2 * s->feature[0].x;
	case 0: break;
	}
	foreach_bit (x, f) {
		s = EVAL_X2F + x;
		switch (s->n_feature) {
		case 7:	feature[s->feature[6].i] -= s->feature[6].x;
		case 6:	feature[s->feature[5].i] -= s->feature[5].x;
		case 5:	feature[s->feature[4].i] -= s->feature[4].x;
		case 4:	feature[s->feature[3].i] -= s->feature[3].x;
		case 3:	feature[s->feature[2].i] -= s->feature[2].x;
		case 2:	feature[s->feature[1].i] -= s->feature[1].x;
		case 1:	feature[s->feature[0].i] -= s->feature[0].x;
		case 0: break;
		}
	}
	
#endif
	
}

/**
 * @brief Update the features after a player's move.
 *
 * @param eval  Evaluation function.
 * @param move  Move.
 */
static void eval_update_1(Eval *eval, const Move *move)
{
	const CoordinateToFeature *s = EVAL_X2F + move->x;
	register int x;
	int *feature = eval->feature;
	unsigned long long f = move->flipped;

#ifdef DEBUG

	int i, j;
	for (i = 0; i < s->n_feature; ++i) {
		j = s->feature[i].i;
		assert(0 <= j && j < EVAL_N_FEATURE);
		feature[j] -= s->feature[i].x;
		assert(EVAL_OFFSET[j] <= feature[j] && feature[j] <= EVAL_MAX_VALUE[j]);
	}

	foreach_bit (x, f) {
		s = EVAL_X2F + x;
		for (i = 0; i < s->n_feature; ++i) {
			j = s->feature[i].i;
			assert(0 <= j && j < EVAL_N_FEATURE);
			feature[j] += s->feature[i].x;
			assert(EVAL_OFFSET[j] <= feature[j] && feature[j] <= EVAL_MAX_VALUE[j]);
		}
	}

#else

	switch (s->n_feature) {
	case 7:	feature[s->feature[6].i] -= s->feature[6].x;
	case 6:	feature[s->feature[5].i] -= s->feature[5].x;
	case 5:	feature[s->feature[4].i] -= s->feature[4].x;
	case 4:	feature[s->feature[3].i] -= s->feature[3].x;
	case 3:	feature[s->feature[2].i] -= s->feature[2].x;
	case 2:	feature[s->feature[1].i] -= s->feature[1].x;
	case 1:	feature[s->feature[0].i] -= s->feature[0].x;
	case 0: break;
	}
	foreach_bit (x, f) {
		s = EVAL_X2F + x;
		switch (s->n_feature) {
		case 7:	feature[s->feature[6].i] += s->feature[6].x;
		case 6:	feature[s->feature[5].i] += s->feature[5].x;
		case 5:	feature[s->feature[4].i] += s->feature[4].x;
		case 4:	feature[s->feature[3].i] += s->feature[3].x;
		case 3:	feature[s->feature[2].i] += s->feature[2].x;
		case 2:	feature[s->feature[1].i] += s->feature[1].x;
		case 1:	feature[s->feature[0].i] += s->feature[0].x;
		case 0: break;
		}
	}
#endif
	
}

void eval_update(Eval *eval, const Move *move)
{
	static void (*eval_update_f[])(Eval*, const Move*) = {eval_update_0, eval_update_1};
	assert(move->flipped);
	assert(WHITE == eval->player || BLACK == eval->player);
	eval_update_f[eval->player](eval, move);
	eval_swap(eval);
}

/**
 * @brief Restore the features as before a player's move.
 *
 * @param eval  Evaluation function.
 * @param move  Move.
 */
static void eval_restore_0(Eval *eval, const Move *move)
{
	const CoordinateToFeature *s = EVAL_X2F + move->x;
	register int x;
	int *feature = eval->feature;
	unsigned long long f = move->flipped;

#ifdef DEBUG

	int i, j;
	for (i = 0; i < s->n_feature; ++i) {
		j = s->feature[i].i;
		assert(0 <= j && j < EVAL_N_FEATURE);
		feature[j] += 2 * s->feature[i].x;
		assert(EVAL_OFFSET[j] <= feature[j] && feature[j] <= EVAL_MAX_VALUE[j]);
	}

	foreach_bit (x, f) {
		s = EVAL_X2F + x;
		for (i = 0; i < s->n_feature; ++i) {
			j = s->feature[i].i;
			assert(0 <= j && j < EVAL_N_FEATURE);
			feature[j] += s->feature[i].x;
			assert(EVAL_OFFSET[j] <= feature[j] && feature[j] <= EVAL_MAX_VALUE[j]);
		}
	}

#else

	switch (s->n_feature) {
	case 7:	feature[s->feature[6].i] += 2 * s->feature[6].x;
	case 6:	feature[s->feature[5].i] += 2 * s->feature[5].x;
	case 5:	feature[s->feature[4].i] += 2 * s->feature[4].x;
	case 4:	feature[s->feature[3].i] += 2 * s->feature[3].x;
	case 3:	feature[s->feature[2].i] += 2 * s->feature[2].x;
	case 2:	feature[s->feature[1].i] += 2 * s->feature[1].x;
	case 1:	feature[s->feature[0].i] += 2 * s->feature[0].x;
	case 0: break;
	}
	foreach_bit (x, f) {
		s = EVAL_X2F + x;
		switch (s->n_feature) {
		case 7:	feature[s->feature[6].i] += s->feature[6].x;
		case 6:	feature[s->feature[5].i] += s->feature[5].x;
		case 5:	feature[s->feature[4].i] += s->feature[4].x;
		case 4:	feature[s->feature[3].i] += s->feature[3].x;
		case 3:	feature[s->feature[2].i] += s->feature[2].x;
		case 2:	feature[s->feature[1].i] += s->feature[1].x;
		case 1:	feature[s->feature[0].i] += s->feature[0].x;
		case 0: break;
		}
	}
	
#endif

}

static void eval_restore_1(Eval *eval, const Move *move)
{
	const CoordinateToFeature *s = EVAL_X2F + move->x;
	register int x;
	int *feature = eval->feature;
	unsigned long long f = move->flipped;

#ifdef DEBUG

	int i, j;
	for (i = 0; i < s->n_feature; ++i) {
		j = s->feature[i].i;
		assert(0 <= j && j < EVAL_N_FEATURE);
		feature[j] += s->feature[i].x;
		assert(EVAL_OFFSET[j] <= feature[j] && feature[j] <= EVAL_MAX_VALUE[j]);
	}

	foreach_bit (x, f) {
		s = EVAL_X2F + x;
		for (i = 0; i < s->n_feature; ++i) {
			j = s->feature[i].i;
			assert(0 <= j && j < EVAL_N_FEATURE);
			feature[j] -= s->feature[i].x;
			assert(EVAL_OFFSET[j] <= feature[j] && feature[j] <= EVAL_MAX_VALUE[j]);
		}
	}

#else

	switch (s->n_feature) {
	case 7:	feature[s->feature[6].i] += s->feature[6].x;
	case 6:	feature[s->feature[5].i] += s->feature[5].x;
	case 5:	feature[s->feature[4].i] += s->feature[4].x;
	case 4:	feature[s->feature[3].i] += s->feature[3].x;
	case 3:	feature[s->feature[2].i] += s->feature[2].x;
	case 2:	feature[s->feature[1].i] += s->feature[1].x;
	case 1:	feature[s->feature[0].i] += s->feature[0].x;
	case 0: break;
	}
	foreach_bit (x, f) {
		s = EVAL_X2F + x;
		switch (s->n_feature) {
		case 7:	feature[s->feature[6].i] -= s->feature[6].x;
		case 6:	feature[s->feature[5].i] -= s->feature[5].x;
		case 5:	feature[s->feature[4].i] -= s->feature[4].x;
		case 4:	feature[s->feature[3].i] -= s->feature[3].x;
		case 3:	feature[s->feature[2].i] -= s->feature[2].x;
		case 2:	feature[s->feature[1].i] -= s->feature[1].x;
		case 1:	feature[s->feature[0].i] -= s->feature[0].x;
		case 0: break;
		}
	}

#endif

}

void eval_restore(Eval *eval, const Move *move)
{
	static void (*eval_restore_f[])(Eval*, const Move*) = {eval_restore_0, eval_restore_1};
	assert(move->flipped);
	eval_swap(eval);
	assert(WHITE == eval->player || BLACK == eval->player);
	eval_restore_f[eval->player](eval, move);
}

/**
 * @brief Update/Restore the features after a passing move.
 *
 * @param eval  Evaluation function.
 */
void eval_pass(Eval *eval)
{
	eval_swap(eval);
}


/**
 * @brief Compute the error-type of the evaluation function according to the
 * depths.
 *
 * A statistical study showed that the accuracy of the alphabeta
 * mostly depends on the depth & the ply of the game. This function is useful
 * to the probcut algorithm. Using a function instead of a table of data makes
 * easier to inter- or extrapolate new values.
 *
 * @param n_empty Number of empty squares on the board.
 * @param depth Depth used in alphabeta.
 * @param probcut_depth A shallow depth used in probcut algorithm.
 */
double eval_sigma(const int n_empty, const int depth, const int probcut_depth)
{
	double sigma;

	sigma = EVAL_A * n_empty + EVAL_B * depth + EVAL_C * probcut_depth;
	sigma = EVAL_a * sigma * sigma + EVAL_b * sigma + EVAL_c;

	return sigma;
}

