/**
 * @file eval_builder.c
 *
 * Set of tools to build evaluation functions
 * This code is not used.
 * This or similar code has been used to build current evaluation function.
 * It will be reused to build a new evaluation function for Edax version 5.0
 *
 * @date 1998 - 201?
 * @author Richard Delorme
 * @version 5.0
 */
 
#include "const.h"
#include "clock.h"
#include "gamebase.h"
#include "eval.h"
#include "stat.h"
#include <assert.h>
#include <ctype.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>
#include <float.h>

#define EDAX_BUILDER_REGULAR

#define EVAL 0x4556414c
#define LAVE 0x4c415645

/** minimization algorithm */
enum{
	EVAL_STEEPEST_DESCENT, /* steepest_descent (with momentum) */
	EVAL_FLETCHER_REEVES,  /* Fletcher-Reeves' conjugate gradient */
	EVAL_POLAK_RIBIERE     /* Polak-Ribiere's conjugate gradient */
};

/** filter */
enum{
	FILTER_NONE,
	FILTER_SPATIAL,
	FILTER_TEMPORAL
};

/** evaluation function */
enum{
	EVAL_EDAX_v3,
	EVAL_EDAX_v5,
	EVAL_LOGISTELLO,
	EVAL_CORNER3x3,
	EVAL_CORNER3x3_B,
	EVAL_CORNER5x2,
	EVAL_CORNER6x2,
	EVAL_EDGE,
	EVAL_EDGE_X,
	EVAL_EDGE_C,
	EVAL_EDGE_CX,
	EVAL_EDGE_FG,
	EVAL_ABFG,
	EVAL_CC,
	EVAL_AA,
	EVAL_BB,
	EVAL_D8,
	EVAL_D7,
	EVAL_D6,
	EVAL_D5,
	EVAL_D4,
	EVAL_D3
};

/* error function */
enum {
	EVAL_ABS_ERROR,
	EVAL_SQUARED_ERROR,
	EVAL_KALMAN_ERROR, /* KALMAN ??? */
	EVAL_SQUARED_ERROR_WEIGHT
};

/* EvalBuilder struct */
typedef struct EvalBuilder{
	int edax_header,eval_header; /* recognition header */
	int version,release,build;   /* version, release & build numbers*/
	double date;                 /* date of creation */
	int type;                    /* type of function : */
	int n_ply,n_vectors;         /* plys * vectors */
	int n_features;              /* features */
	int n_data;                  /* data */
	int n_games;                 /* games */
	int *vector_size;            /* vectors size */
	int *vector_squares;         /* vectors size in bits */
	int *vector_offset;          /* vectors offset */
	int *vector_times;
	short **data;                /* data[ply][config] */
	short **coeff;               /* coeff[vector][subconfig] -> data */
	int **feature;               /* feature[game][feature] */
	char *score;			     /* score[game] */
} EvalBuilder;

typedef struct EvalOption {
	int min_iter;
	int max_iter;
	double accuracy;
	int round_frequency;
	int zero_frequency;
	int equalize_frequency;
	int unbias_frequency;
	int restart_frequency;
	int minimization_algorithm;
	int error_type;
	double alpha, beta;
} EvalOption;

extern int s12[2][531441],s12[2][531441],s10[2][59049],c10[2][59049],i10[2][59049],c9[2][19683],s8[2][6561],s7[2][2187],s6[2][729],s5[2][243],s4[2][81],s3[2][27];

/* create a new EvalBuilder structure */
EvalBuilder *eval_builder_create(int n_vectors,int *vector_size,int *vector_times,int n_features,int n_games){
	EvalBuilder *eval;
	int i;

	eval=(EvalBuilder*)malloc(sizeof (EvalBuilder));

	eval->edax_header=EDAX;
	eval->eval_header=EVAL;
	eval->version=EDAX_VERSION;
	eval->release=EDAX_RELEASE;
	eval->build=0;
	eval->date=ul_clock_get_date();
	eval->n_ply=61;
	eval->n_vectors=n_vectors;
	eval->n_data=0;
	eval->vector_size=(int *)malloc(n_vectors*sizeof (int));
	assert(eval->vector_size!=NULL);
	eval->vector_squares=(int *)malloc(n_vectors*sizeof (int));
	assert(eval->vector_squares!=NULL);
	eval->vector_offset=(int *)malloc(n_vectors*sizeof (int));
	assert(eval->vector_offset!=NULL);
	eval->vector_times=(int *)malloc(n_vectors*sizeof (int));
	assert(eval->vector_times!=NULL);
	eval->coeff=(short **)malloc(n_vectors*sizeof (short *));
	assert(eval->coeff!=NULL);
	for (i=0;i<n_vectors;i++){
		eval->vector_size[i]=vector_size[i];
		eval->vector_times[i]=vector_times[i];
		eval->vector_offset[i]=(i==0?0:(eval->vector_offset[i-1]+vector_size[i-1]));
		eval->n_data+=eval->vector_size[i];
		switch(vector_size[i]){
		case 1:
			eval->vector_squares[i]=0;
			break;
		case 3:
			eval->vector_squares[i]=1;
			break;
		case 6:
		case 9:
			eval->vector_squares[i]=2;
			break;
		case 27:
		case 18:
			eval->vector_squares[i]=3;
			break;
		case 81:
		case 45:
			eval->vector_squares[i]=4;
			break;
		case 243:
		case 135:
			eval->vector_squares[i]=5;
			break;
		case 729:
		case 378:
			eval->vector_squares[i]=6;
			break;
		case 2187:
		case 1134:
			eval->vector_squares[i]=7;
			break;
		case 6561:
		case 3321:
			eval->vector_squares[i]=8;
			break;
		case 19683:
		case 10206:
			eval->vector_squares[i]=9;
			break;
		case 59049:
		case 29646:
		case 29889:
			eval->vector_squares[i]=10;
			break;
		case 531441:
		case 266814:
		case 266085:
			eval->vector_squares[i]=12;
			break;
		default:
			eval->vector_squares[i]=0;
		}
	}
	eval->data=(short**)malloc(eval->n_ply*sizeof (short*));
	assert(eval->data!=NULL);
	eval->data[0]=(short*)calloc(eval->n_ply*eval->n_data,sizeof (short));
	assert(eval->data[0]!=NULL);
	for (i=1;i<eval->n_ply;i++){
		eval->data[i]=eval->data[i-1]+eval->n_data;
	}
	eval->n_features=n_features;
	eval->n_games=n_games;
	eval->feature=(int**)malloc(n_games*sizeof (int*));
	assert(eval->feature!=NULL);
	eval->feature[0]=(int*)malloc(n_games*eval->n_features*sizeof (int));
	assert(eval->feature[0]!=NULL);
	for (i=1;i<n_games;i++){
		eval->feature[i]=eval->feature[i-1]+eval->n_features;
	}
	eval->score=(char *)malloc(n_games*sizeof (char));
	assert(eval->score!=NULL);

	return eval;
}

/* destroy an EvalBuilder structure */
void eval_builder_destroy(EvalBuilder *eval){
	if (eval!=NULL){
		free(eval->feature[0]);
		free(eval->feature);
		free(eval->data[0]);
		free(eval->data);
		free(eval->vector_size);
		free(eval->vector_squares);
		free(eval->vector_offset);
		free(eval);
	}
}

/* set EvalBuilder items for 'ply' */
void eval_builder_set_ply(EvalBuilder* eval,int ply){
	int i;
	short **x=eval->coeff;

	x[0]=eval->data[ply];
	for (i=1;i<eval->n_vectors;i++){
		x[i]=x[i-1]+eval->vector_size[i-1];
	}
}

/* read an EvalBuilder structure */
void eval_builder_read(EvalBuilder* eval,const char *file){
	FILE *f;
	int r;

	f=fopen(file,"rb");
	if (f==NULL){
		fprintf(stderr,"eval_builder_read : can't open %s\n",file);
		exit(EXIT_FAILURE);
	}

	r = fread(&(eval->edax_header),sizeof (int),1,f);
	r += fread(&(eval->eval_header),sizeof (int),1,f);
	r += fread(&(eval->version),sizeof (int),1,f);
	r += fread(&(eval->release),sizeof (int),1,f);
	r += fread(&(eval->build),sizeof (int),1,f);
	r += fread(&(eval->date),sizeof (double),1,f);
	r += fread(eval->data[0],sizeof (short),eval->n_data*eval->n_ply,f);

	if (r != 6 + eval->n_data*eval->n_ply) {
		fprintf(stderr,"eval_builder_read : can't read %s\n",file);
		exit(EXIT_FAILURE);
	}
}

/* write an EvalBuilder structure */
void eval_builder_write(EvalBuilder* eval,const char *file){
	FILE *f;
	
	f=fopen(file,"wb");
	if (f==NULL){
		fprintf(stderr,"eval_builder_write : can't open %s\n",file);
		exit(EXIT_FAILURE);
	}

	fwrite(&(eval->edax_header),sizeof (int),1,f);
	fwrite(&(eval->eval_header),sizeof (int),1,f);
	fwrite(&(eval->version),sizeof (int),1,f);
	fwrite(&(eval->release),sizeof (int),1,f);
	fwrite(&(eval->build),sizeof (int),1,f);
	fwrite(&(eval->date),sizeof (double),1,f);
	fwrite(eval->data[0],sizeof (short),eval->n_data*eval->n_ply,f);
}

/* init eval_builder_edax features for a board */
void eval_builder_logistello_get_features(const Board *b, int *X){
	int p=b->player;
	const char *x=b->square;

	X[0]=c9[p][x[A1]*6561+x[B1]*2187+x[A2]*729+x[B2]*243+x[C1]*81+x[A3]*27+x[C2]*9+x[B3]*3+x[C3]];
	X[1]=c9[p][x[H1]*6561+x[G1]*2187+x[H2]*729+x[G2]*243+x[F1]*81+x[H3]*27+x[F2]*9+x[G3]*3+x[F3]];
	X[2]=c9[p][x[A8]*6561+x[A7]*2187+x[B8]*729+x[B7]*243+x[A6]*81+x[C8]*27+x[B6]*9+x[C7]*3+x[C6]];
	X[3]=c9[p][x[H8]*6561+x[H7]*2187+x[G8]*729+x[G7]*243+x[H6]*81+x[F8]*27+x[G6]*9+x[F7]*3+x[F6]];

	X[4]=s10[p][x[B2]*19683+x[A1]*6561+x[B1]*2187+x[C1]*729+x[D1]*243+x[E1]*81+x[F1]*27+x[G1]*9+x[H1]*3+x[G2]]+10206; 
	X[5]=s10[p][x[B7]*19683+x[A8]*6561+x[B8]*2187+x[C8]*729+x[D8]*243+x[E8]*81+x[F8]*27+x[G8]*9+x[H8]*3+x[G7]]+10206;
	X[6]=s10[p][x[B2]*19683+x[A1]*6561+x[A2]*2187+x[A3]*729+x[A4]*243+x[A5]*81+x[A6]*27+x[A7]*9+x[A8]*3+x[B7]]+10206;
	X[7]=s10[p][x[G2]*19683+x[H1]*6561+x[H2]*2187+x[H3]*729+x[H4]*243+x[H5]*81+x[H6]*27+x[H7]*9+x[H8]*3+x[G7]]+10206;

	X[8]=i10[p][x[A1]*19683+x[B1]*6561+x[C1]*2187+x[D1]*729+x[E1]*243+x[A2]*81+x[B2]*27+x[C2]*9+x[D2]*3+x[E2]]+39852;
	X[9]=i10[p][x[H1]*19683+x[G1]*6561+x[F1]*2187+x[E1]*729+x[D1]*243+x[H2]*81+x[G2]*27+x[F2]*9+x[E2]*3+x[D2]]+39852;
	X[10]=i10[p][x[A8]*19683+x[B8]*6561+x[C8]*2187+x[D8]*729+x[E8]*243+x[A2]*81+x[B2]*27+x[C2]*9+x[D2]*3+x[E2]]+39852;
	X[11]=i10[p][x[H8]*19683+x[G8]*6561+x[F8]*2187+x[E8]*729+x[D8]*243+x[H2]*81+x[G2]*27+x[F2]*9+x[E2]*3+x[D2]]+39852;
	X[12]=i10[p][x[A1]*19683+x[A2]*6561+x[A3]*2187+x[A4]*729+x[A5]*243+x[B1]*81+x[B2]*27+x[B3]*9+x[B4]*3+x[B5]]+39852;
	X[13]=i10[p][x[A8]*19683+x[A7]*6561+x[A6]*2187+x[A5]*729+x[A4]*243+x[B8]*81+x[B7]*27+x[B6]*9+x[B5]*3+x[B4]]+39852;
	X[14]=i10[p][x[H1]*19683+x[H2]*6561+x[H3]*2187+x[H4]*729+x[H5]*243+x[G1]*81+x[G2]*27+x[G3]*9+x[G4]*3+x[G5]]+39852;
	X[15]=i10[p][x[H8]*19683+x[H7]*6561+x[H6]*2187+x[H5]*729+x[H4]*243+x[G8]*81+x[G7]*27+x[G6]*9+x[G5]*3+x[G4]]+39852;

	X[16]=s8[p][x[A2]*2187+x[B2]*729+x[C2]*243+x[D2]*81+x[E2]*27+x[F2]*9+x[G2]*3+x[H2]]+98901;
	X[17]=s8[p][x[A7]*2187+x[B7]*729+x[C7]*243+x[D7]*81+x[E7]*27+x[F7]*9+x[G7]*3+x[H7]]+98901;
	X[18]=s8[p][x[B1]*2187+x[B2]*729+x[B3]*243+x[B4]*81+x[B5]*27+x[B6]*9+x[B7]*3+x[B8]]+98901;
	X[19]=s8[p][x[G1]*2187+x[G2]*729+x[G3]*243+x[G4]*81+x[G5]*27+x[G6]*9+x[G7]*3+x[G8]]+98901;

	X[20]=s8[p][x[A3]*2187+x[B3]*729+x[C3]*243+x[D3]*81+x[E3]*27+x[F3]*9+x[G3]*3+x[H3]]+102222;
	X[21]=s8[p][x[A6]*2187+x[B6]*729+x[C6]*243+x[D6]*81+x[E6]*27+x[F6]*9+x[G6]*3+x[H6]]+102222;
	X[22]=s8[p][x[C1]*2187+x[C2]*729+x[C3]*243+x[C4]*81+x[C5]*27+x[C6]*9+x[C7]*3+x[C8]]+102222;
	X[23]=s8[p][x[F1]*2187+x[F2]*729+x[F3]*243+x[F4]*81+x[F5]*27+x[F6]*9+x[F7]*3+x[F8]]+102222;

	X[24]=s8[p][x[A4]*2187+x[B4]*729+x[C4]*243+x[D4]*81+x[E4]*27+x[F4]*9+x[G4]*3+x[H4]]+105543;
	X[25]=s8[p][x[A5]*2187+x[B5]*729+x[C5]*243+x[D5]*81+x[E5]*27+x[F5]*9+x[G5]*3+x[H5]]+105543;
	X[26]=s8[p][x[D1]*2187+x[D2]*729+x[D3]*243+x[D4]*81+x[D5]*27+x[D6]*9+x[D7]*3+x[D8]]+105543;
	X[27]=s8[p][x[E1]*2187+x[E2]*729+x[E3]*243+x[E4]*81+x[E5]*27+x[E6]*9+x[E7]*3+x[E8]]+105543;

	X[28]=s8[p][x[A1]*2187+x[B2]*729+x[C3]*243+x[D4]*81+x[E5]*27+x[F6]*9+x[G7]*3+x[H8]]+108864;
	X[29]=s8[p][x[A8]*2187+x[B7]*729+x[C6]*243+x[D5]*81+x[E4]*27+x[F3]*9+x[G2]*3+x[H1]]+108864;

	X[30]=s7[p][x[B1]*729+x[C2]*243+x[D3]*81+x[E4]*27+x[F5]*9+x[G6]*3+x[H7]]+112185;
	X[31]=s7[p][x[H2]*729+x[G3]*243+x[F4]*81+x[E5]*27+x[D6]*9+x[C7]*3+x[B8]]+112185;
	X[32]=s7[p][x[A2]*729+x[B3]*243+x[C4]*81+x[D5]*27+x[E6]*9+x[F7]*3+x[G8]]+112185;
	X[33]=s7[p][x[G1]*729+x[F2]*243+x[E3]*81+x[D4]*27+x[C5]*9+x[B6]*3+x[A7]]+112185;

	X[34]=s6[p][x[C1]*243+x[D2]*81+x[E3]*27+x[F4]*9+x[G5]*3+x[H6]]+113319;
	X[35]=s6[p][x[A3]*243+x[B4]*81+x[C5]*27+x[D6]*9+x[E7]*3+x[F8]]+113319;
	X[36]=s6[p][x[F1]*243+x[E2]*81+x[D3]*27+x[C4]*9+x[B5]*3+x[A6]]+113319;
	X[37]=s6[p][x[H3]*243+x[G4]*81+x[F5]*27+x[E6]*9+x[D7]*3+x[C8]]+113319;

	X[38]=s5[p][x[D1]*81+x[E2]*27+x[F3]*9+x[G4]*3+x[H5]]+113697;
	X[39]=s5[p][x[A4]*81+x[B5]*27+x[C6]*9+x[D7]*3+x[E8]]+113697;
	X[40]=s5[p][x[E1]*81+x[D2]*27+x[C3]*9+x[B4]*3+x[A5]]+113697;
	X[41]=s5[p][x[H4]*81+x[G5]*27+x[F6]*9+x[E7]*3+x[D8]]+113697;

	X[42]=s4[p][x[D1]*27+x[C2]*9+x[B3]*3+x[A4]]+113832;
	X[43]=s4[p][x[A5]*27+x[B6]*9+x[C7]*3+x[D8]]+113832;
	X[44]=s4[p][x[E1]*27+x[F2]*9+x[G3]*3+x[H4]]+113832;
	X[45]=s4[p][x[H5]*27+x[G6]*9+x[F7]*3+x[E8]]+113832;
	
	X[46]=113877;
}

/* init eval_builder corner 5x2 feature for a board */
void eval_builder_get_corner5x2_features(const Board *b,int *X){
	int p=b->player;
	const char *x=b->square;

	X[0]=i10[p][x[A1]*19683+x[B1]*6561+x[C1]*2187+x[D1]*729+x[E1]*243+x[A2]*81+x[B2]*27+x[C2]*9+x[D2]*3+x[E2]];
	X[1]=i10[p][x[H1]*19683+x[G1]*6561+x[F1]*2187+x[E1]*729+x[D1]*243+x[H2]*81+x[G2]*27+x[F2]*9+x[E2]*3+x[D2]];
	X[2]=i10[p][x[A8]*19683+x[B8]*6561+x[C8]*2187+x[D8]*729+x[E8]*243+x[A2]*81+x[B2]*27+x[C2]*9+x[D2]*3+x[E2]];
	X[3]=i10[p][x[H8]*19683+x[G8]*6561+x[F8]*2187+x[E8]*729+x[D8]*243+x[H2]*81+x[G2]*27+x[F2]*9+x[E2]*3+x[D2]];
	X[4]=i10[p][x[A1]*19683+x[A2]*6561+x[A3]*2187+x[A4]*729+x[A5]*243+x[B1]*81+x[B2]*27+x[B3]*9+x[B4]*3+x[B5]];
	X[5]=i10[p][x[A8]*19683+x[A7]*6561+x[A6]*2187+x[A5]*729+x[A4]*243+x[B8]*81+x[B7]*27+x[B6]*9+x[B5]*3+x[B4]];
	X[6]=i10[p][x[H1]*19683+x[H2]*6561+x[H3]*2187+x[H4]*729+x[H5]*243+x[G1]*81+x[G2]*27+x[G3]*9+x[G4]*3+x[G5]];
	X[7]=i10[p][x[H8]*19683+x[H7]*6561+x[H6]*2187+x[H5]*729+x[H4]*243+x[G8]*81+x[G7]*27+x[G6]*9+x[G5]*3+x[G4]];
	X[8]=59049;
}


/* init eval_builder corner 3x3 feature for a board */
void eval_builder_get_corner3x3_features(const Board *b,int *X){
	int p=b->player;
	const char *x=b->square;

	X[0]=c9[p][x[A1]*6561+x[B1]*2187+x[A2]*729+x[B2]*243+x[C1]*81+x[A3]*27+x[C2]*9+x[B3]*3+x[C3]];
	X[1]=c9[p][x[H1]*6561+x[G1]*2187+x[H2]*729+x[G2]*243+x[F1]*81+x[H3]*27+x[F2]*9+x[G3]*3+x[F3]];
	X[2]=c9[p][x[A8]*6561+x[A7]*2187+x[B8]*729+x[B7]*243+x[A6]*81+x[C8]*27+x[B6]*9+x[C7]*3+x[C6]];
	X[3]=c9[p][x[H8]*6561+x[H7]*2187+x[G8]*729+x[G7]*243+x[H6]*81+x[F8]*27+x[G6]*9+x[F7]*3+x[F6]];
	X[4]=10206;
}

/* init eval_builder edge feature for a board */
void eval_builder_get_edge_features(const Board *b,int *X){
	int p=b->player;
	const char *x=b->square;

	X[0]=s8[p][x[A1]*2187+x[B1]*729+x[C1]*243+x[D1]*81+x[E1]*27+x[F1]*9+x[G1]*3+x[H1]];
	X[1]=s8[p][x[A8]*2187+x[B8]*729+x[C8]*243+x[D8]*81+x[E8]*27+x[F8]*9+x[G8]*3+x[H8]];
	X[2]=s8[p][x[A1]*2187+x[A2]*729+x[A3]*243+x[A4]*81+x[A5]*27+x[A6]*9+x[A7]*3+x[A8]];
	X[3]=s8[p][x[H1]*2187+x[H2]*729+x[H3]*243+x[H4]*81+x[H5]*27+x[H6]*9+x[H7]*3+x[H8]];
	X[4]=3321;
}

/* init eval_builder edge feature for a board */
void eval_builder_get_edge_X_features(const Board *b,int *X){
	int p=b->player;
	const char *x=b->square;

	X[0]=s10[p][x[B2]*19683+x[A1]*6561+x[B1]*2187+x[C1]*729+x[D1]*243+x[E1]*81+x[F1]*27+x[G1]*9+x[H1]*3+x[G2]];
	X[1]=s10[p][x[B7]*19683+x[A8]*6561+x[B8]*2187+x[C8]*729+x[D8]*243+x[E8]*81+x[F8]*27+x[G8]*9+x[H8]*3+x[G7]];
	X[2]=s10[p][x[B2]*19683+x[A1]*6561+x[A2]*2187+x[A3]*729+x[A4]*243+x[A5]*81+x[A6]*27+x[A7]*9+x[A8]*3+x[B7]];
	X[3]=s10[p][x[G2]*19683+x[H1]*6561+x[H2]*2187+x[H3]*729+x[H4]*243+x[H5]*81+x[H6]*27+x[H7]*9+x[H8]*3+x[G7]];
	X[4]=29646;
}

/* init eval_builder edge feature for a board */
void eval_builder_get_edge_C_features(const Board *b,int *X){
	int p=b->player;
	const char *x=b->square;

	X[0]=s10[p][x[A2]*19683+x[A1]*6561+x[B1]*2187+x[C1]*729+x[D1]*243+x[E1]*81+x[F1]*27+x[G1]*9+x[H1]*3+x[H2]];
	X[1]=s10[p][x[A7]*19683+x[A8]*6561+x[B8]*2187+x[C8]*729+x[D8]*243+x[E8]*81+x[F8]*27+x[G8]*9+x[H8]*3+x[H7]];
	X[2]=s10[p][x[B1]*19683+x[A1]*6561+x[A2]*2187+x[A3]*729+x[A4]*243+x[A5]*81+x[A6]*27+x[A7]*9+x[A8]*3+x[B8]];
	X[3]=s10[p][x[G1]*19683+x[H1]*6561+x[H2]*2187+x[H3]*729+x[H4]*243+x[H5]*81+x[H6]*27+x[H7]*9+x[H8]*3+x[G8]];
	X[4]=29646;
}


/* init eval_builder edge feature for a board */
void eval_builder_get_edge_CX_features(const Board *b,int *X){
	int p=b->player;
	const char *x=b->square;

	X[0]=s12[p][x[B2]*177147+x[A2]*59049+x[A1]*19683+x[B1]*6561+x[C1]*2187+x[D1]*729+x[E1]*243+x[F1]*81+x[G1]*27+x[H1]*9+x[H2]*3+x[G2]];
	X[1]=s12[p][x[B7]*177147+x[A7]*59049+x[A8]*19683+x[B8]*6561+x[C8]*2187+x[D8]*729+x[E8]*243+x[F8]*81+x[G8]*27+x[H8]*9+x[H7]*3+x[G7]];
	X[2]=s12[p][x[B2]*177147+x[B1]*59049+x[A1]*19683+x[A2]*6561+x[A3]*2187+x[A4]*729+x[A5]*243+x[A6]*81+x[A7]*27+x[A8]*9+x[B8]*3+x[B7]];
	X[3]=s12[p][x[G2]*177147+x[G1]*59049+x[H1]*19683+x[H2]*6561+x[H3]*2187+x[H4]*729+x[H5]*243+x[H6]*81+x[H7]*27+x[H8]*9+x[G8]*3+x[G7]];
	X[4]=266085;
}

/* ABFG */
void eval_builder_get_ABFG_features(const Board *b,int *X){
	int p=b->player;
	const char *x=b->square;
	X[0]=s8[p][x[C1]*2187+x[D1]*729+x[C2]*243+x[D2]*81+x[E2]*27+x[F2]*9+x[E1]*3+x[F1]];
	X[1]=s8[p][x[C8]*2187+x[D8]*729+x[C7]*243+x[D7]*81+x[E7]*27+x[F7]*9+x[E8]*3+x[F8]];
	X[2]=s8[p][x[A3]*2187+x[A4]*729+x[B3]*243+x[B4]*81+x[B5]*27+x[B6]*9+x[A5]*3+x[A6]];
	X[3]=s8[p][x[H3]*2187+x[H4]*729+x[G3]*243+x[G4]*81+x[G5]*27+x[G6]*9+x[H5]*3+x[H6]];
	X[4]=3321;
}

void eval_builder_get_edge_FG_features(const Board *b,int *X){
	int p=b->player;
	const char *x=b->square;
	X[0]=s12[p][x[D2]*177147+x[C2]*59049+x[A1]*19683+x[B1]*6561+x[C1]*2187+x[D1]*729+x[E1]*243+x[F1]*81+x[G1]*27+x[H1]*9+x[F2]*3+x[E2]];
	X[1]=s12[p][x[D7]*177147+x[C7]*59049+x[A8]*19683+x[B8]*6561+x[C8]*2187+x[D8]*729+x[E8]*243+x[F8]*81+x[G8]*27+x[H8]*9+x[F7]*3+x[E7]];
	X[2]=s12[p][x[B4]*177147+x[B3]*59049+x[A1]*19683+x[A2]*6561+x[A3]*2187+x[A4]*729+x[A5]*243+x[A6]*81+x[A7]*27+x[A8]*9+x[B6]*3+x[B5]];
	X[3]=s12[p][x[G4]*177147+x[G3]*59049+x[H1]*19683+x[H2]*6561+x[H3]*2187+x[H4]*729+x[H5]*243+x[H6]*81+x[H7]*27+x[H8]*9+x[G6]*3+x[G5]];
	X[4]=266085;
}

/* CC */
void eval_builder_get_CC_features(const Board *b,int *X){
	int p=b->player;
	const char *x=b->square;
	X[0]=s8[p][x[A2]*2187+x[B2]*729+x[C2]*243+x[D2]*81+x[E2]*27+x[F2]*9+x[G2]*3+x[H2]];
	X[1]=s8[p][x[A7]*2187+x[B7]*729+x[C7]*243+x[D7]*81+x[E7]*27+x[F7]*9+x[G7]*3+x[H7]];
	X[2]=s8[p][x[B1]*2187+x[B2]*729+x[B3]*243+x[B4]*81+x[B5]*27+x[B6]*9+x[B7]*3+x[B8]];
	X[3]=s8[p][x[G1]*2187+x[G2]*729+x[G3]*243+x[G4]*81+x[G5]*27+x[G6]*9+x[G7]*3+x[G8]];
	X[4]=3321;
}

/* AA */
void eval_builder_get_AA_features(const Board *b,int *X){
	int p=b->player;
	const char *x=b->square;
	X[0]=s8[p][x[A3]*2187+x[B3]*729+x[C3]*243+x[D3]*81+x[E3]*27+x[F3]*9+x[G3]*3+x[H3]];
	X[1]=s8[p][x[A6]*2187+x[B6]*729+x[C6]*243+x[D6]*81+x[E6]*27+x[F6]*9+x[G6]*3+x[H6]];
	X[2]=s8[p][x[C1]*2187+x[C2]*729+x[C3]*243+x[C4]*81+x[C5]*27+x[C6]*9+x[C7]*3+x[C8]];
	X[3]=s8[p][x[F1]*2187+x[F2]*729+x[F3]*243+x[F4]*81+x[F5]*27+x[F6]*9+x[F7]*3+x[F8]];
	X[4]=3321;
}

/* BB */
void eval_builder_get_BB_features(const Board *b,int *X){
	int p=b->player;
	const char *x=b->square;
	X[0]=s8[p][x[A4]*2187+x[B4]*729+x[C4]*243+x[D4]*81+x[E4]*27+x[F4]*9+x[G4]*3+x[H4]];
	X[1]=s8[p][x[A5]*2187+x[B5]*729+x[C5]*243+x[D5]*81+x[E5]*27+x[F5]*9+x[G5]*3+x[H5]];
	X[2]=s8[p][x[D1]*2187+x[D2]*729+x[D3]*243+x[D4]*81+x[D5]*27+x[D6]*9+x[D7]*3+x[D8]];
	X[3]=s8[p][x[E1]*2187+x[E2]*729+x[E3]*243+x[E4]*81+x[E5]*27+x[E6]*9+x[E7]*3+x[E8]];
	X[4]=3321;
}

/* d8 */
void eval_builder_get_d8_features(const Board *b,int *X){
	int p=b->player;
	const char *x=b->square;
	X[0]=s8[p][x[A1]*2187+x[B2]*729+x[C3]*243+x[D4]*81+x[E5]*27+x[F6]*9+x[G7]*3+x[H8]];
	X[1]=s8[p][x[A8]*2187+x[B7]*729+x[C6]*243+x[D5]*81+x[E4]*27+x[F3]*9+x[G2]*3+x[H1]];
	X[2]=3321;
}

/* d7 */
void eval_builder_get_d7_features(const Board *b,int *X){
	int p=b->player;
	const char *x=b->square;
	X[0]=s7[p][x[B1]*729+x[C2]*243+x[D3]*81+x[E4]*27+x[F5]*9+x[G6]*3+x[H7]];
	X[1]=s7[p][x[H2]*729+x[G3]*243+x[F4]*81+x[E5]*27+x[D6]*9+x[C7]*3+x[B8]];
	X[2]=s7[p][x[A2]*729+x[B3]*243+x[C4]*81+x[D5]*27+x[E6]*9+x[F7]*3+x[G8]];
	X[3]=s7[p][x[G1]*729+x[F2]*243+x[E3]*81+x[D4]*27+x[C5]*9+x[B6]*3+x[A7]];
	X[4]=1134;
}

/* d6 */
void eval_builder_get_d6_features(const Board *b,int *X){
	int p=b->player;
	const char *x=b->square;
	X[0]=s6[p][x[C1]*243+x[D2]*81+x[E3]*27+x[F4]*9+x[G5]*3+x[H6]];
	X[1]=s6[p][x[A3]*243+x[B4]*81+x[C5]*27+x[D6]*9+x[E7]*3+x[F8]];
	X[2]=s6[p][x[F1]*243+x[E2]*81+x[D3]*27+x[C4]*9+x[B5]*3+x[A6]];
	X[3]=s6[p][x[H3]*243+x[G4]*81+x[F5]*27+x[E6]*9+x[D7]*3+x[C8]];
	X[4]=378;
}

/* d5 */
void eval_builder_get_d5_features(const Board *b,int *X){
	int p=b->player;
	const char *x=b->square;
	X[0]=s5[p][x[D1]*81+x[E2]*27+x[F3]*9+x[G4]*3+x[H5]];
	X[1]=s5[p][x[A4]*81+x[B5]*27+x[C6]*9+x[D7]*3+x[E8]];
	X[2]=s5[p][x[E1]*81+x[D2]*27+x[C3]*9+x[B4]*3+x[A5]];
	X[3]=s5[p][x[H4]*81+x[G5]*27+x[F6]*9+x[E7]*3+x[D8]];
	X[4]=135;
}

/* d4 */
void eval_builder_get_d4_features(const Board *b,int *X){
	int p=b->player;
	const char *x=b->square;
	X[0]=s4[p][x[D1]*27+x[C2]*9+x[B3]*3+x[A4]];
	X[1]=s4[p][x[A5]*27+x[B6]*9+x[C7]*3+x[D8]];
	X[2]=s4[p][x[E1]*27+x[F2]*9+x[G3]*3+x[H4]];
	X[3]=s4[p][x[H5]*27+x[G6]*9+x[F7]*3+x[E8]];
	X[4]=45;
}

/* d3 */
void eval_builder_get_d3_features(const Board *b,int *X){
	int p=b->player;
	const char *x=b->square;
	X[0]=s3[p][x[G2]*9+x[B3]*3+x[A4]];
	X[1]=s3[p][x[B6]*9+x[C7]*3+x[D8]];
	X[2]=s3[p][x[F2]*9+x[G3]*3+x[H4]];
	X[3]=s3[p][x[G6]*9+x[F7]*3+x[E8]];
	X[4]=18;
}

/* create a new EvalBuilder structure for 'edax' EvalBuilder */
/*EvalBuilder *eval_builder_create_edax_v2(int n_games){
	int vector_size[]={10206,3321,3321,3321,3321,3321,3321,1134,378,135,45,1};
	int vector_times[]={4,4,4,4,4,4,2,4,4,4,4,1};
	
	eval_init();
	eval_builder_set_features=eval_builder_edax_v2_get_features;	
	return eval_builder_create(12,vector_size,vector_times,43,n_games);
}
*/


/* create a new EvalBuilder structure for 'edax' EvalBuilder */
/*EvalBuilder *eval_builder_create_edax(int n_games){
	int vector_size[]={10206,29646,3321,3321,3321,3321,3321,1134,378,135,45,1};
	int vector_times[]={4,4,4,4,4,4,2,4,4,4,4,1};
	
	eval_init();
	eval_builder_set_features = eval_edax_get_packed_features;
	return eval_builder_create(12, vector_size, vector_times, 43, n_games);
}
*/
/* create a new EvalBuilder structure for 'edax' EvalBuilder */
#if 0
EvalBuilder *eval_builder_create_edax3b(int n_games){
	int vector_size[]={10206,29646,29646,3321,3321,3321,3321,1134,378,135,45,1};
	int vector_times[]={4,4,4,4,4,4,2,4,4,4,4,1};

	eval_init();
	eval_builder_set_features = eval_edax3b_get_packed_features;
	return eval_builder_create(12, vector_size, vector_times, 43, n_games);
}
#endif

/* create a new EvalBuilder structure for 'edax' EvalBuilder */
EvalBuilder *eval_builder_create_edax3c(int n_games){
	int vector_size[]={10206,29889,29646,29646,3321,3321,3321,3321,1134,378,135,45,1};
	int vector_times[]={4,4,4,4,4,4,4,2,4,4,4,4,1};

	eval_init();
	eval_builder_set_features = eval_edax_v3r1_get_packed_features;
	return eval_builder_create(13, vector_size, vector_times, 47, n_games);
}

#if 1
/* create a new EvalBuilder structure for 'edax' EvalBuilder */

EvalBuilder *eval_builder_create_edax3d(int n_games){
	int vector_size[]={10206,266814,266085,266085,3321,3321,3321,3321,1134,378,135,45,1};
	int vector_times[]={4,4,4,4,4,4,4,2,4,4,4,4,1};

	eval_init();
	eval_builder_set_features = eval_edax_v3r2_get_packed_features;
	return eval_builder_create(13, vector_size, vector_times, 47, n_games);
}
#endif

/* create a new EvalBuilder structure for 'edax' EvalBuilder */
#if 0
EvalBuilder *eval_builder_create_edax3d(int n_games){
	int vector_size[]={10206,266814,266085,3321,3321,3321,3321,1134,378,135,45,1};
	int vector_times[]={4,4,4,4,4,4,2,4,4,4,4,1};

	eval_init();
	eval_builder_set_features = eval_edax_v3r3_get_packed_features;
	return eval_builder_create(12, vector_size, vector_times, 43, n_games);
}
#endif

/* create a new EvalBuilder structure for 'edax' EvalBuilder */
EvalBuilder *eval_builder_create_logistello(int n_games){
	int vector_size[]={10206,29646,59049,3321,3321,3321,3321,1134,378,135,45,1};
	int vector_times[]={4,4,8,4,4,4,2,4,4,4,4,1};

	eval_init();
	eval_builder_set_features=eval_builder_logistello_get_features;
	return eval_builder_create(12,vector_size,vector_times,47,n_games);
}

/* create a new EvalBuilder structure for a single feature */
EvalBuilder *eval_builder_create_feature(int n_games, int feature){
	int vector_size[2]={3321,1};
	int vector_times[]={4,1};

	eval_init();
	switch(feature){
	case EVAL_CORNER3x3:
		eval_builder_set_features=eval_builder_get_corner3x3_features;
		vector_size[0]=10206;
		return eval_builder_create(2,vector_size,vector_times,5,n_games);
	case EVAL_CORNER5x2:
		eval_builder_set_features=eval_builder_get_corner5x2_features;
		vector_size[0]=59049;
		vector_times[0]=8;
		return eval_builder_create(2,vector_size,vector_times,9,n_games);
	case EVAL_EDGE:
		eval_builder_set_features=eval_builder_get_edge_features;
		return eval_builder_create(2,vector_size,vector_times,5,n_games);
	case EVAL_EDGE_X:
		eval_builder_set_features=eval_builder_get_edge_X_features;
		vector_size[0]=29646;
		return eval_builder_create(2,vector_size,vector_times,5,n_games);
	case EVAL_EDGE_C:
		eval_builder_set_features=eval_builder_get_edge_C_features;
		vector_size[0]=29646;
		return eval_builder_create(2,vector_size,vector_times,5,n_games);
	case EVAL_EDGE_CX:
		eval_builder_set_features=eval_builder_get_edge_CX_features;
		vector_size[0]=266085;
		return eval_builder_create(2,vector_size,vector_times,5,n_games);
	case EVAL_EDGE_FG:
		eval_builder_set_features=eval_builder_get_edge_FG_features;
		vector_size[0]=266085;
		return eval_builder_create(2,vector_size,vector_times,5,n_games);
	case EVAL_ABFG:
		eval_builder_set_features=eval_builder_get_ABFG_features;
		return eval_builder_create(2,vector_size,vector_times,5,n_games);
	case EVAL_CC:
		eval_builder_set_features=eval_builder_get_CC_features;
		return eval_builder_create(2,vector_size,vector_times,5,n_games);
	case EVAL_AA:
		eval_builder_set_features=eval_builder_get_AA_features;
		return eval_builder_create(2,vector_size,vector_times,5,n_games);
	case EVAL_BB:
		eval_builder_set_features=eval_builder_get_BB_features;
		return eval_builder_create(2,vector_size,vector_times,5,n_games);
	case EVAL_D8:
		eval_builder_set_features=eval_builder_get_d8_features;
		vector_times[0]=2;
		return eval_builder_create(2,vector_size,vector_times,3,n_games);
	case EVAL_D7:
		eval_builder_set_features=eval_builder_get_d7_features;
		vector_size[0]=1134;
		return eval_builder_create(2,vector_size,vector_times,5,n_games);
	case EVAL_D6:
		eval_builder_set_features=eval_builder_get_d6_features;
		vector_size[0]=378;
		return eval_builder_create(2,vector_size,vector_times,5,n_games);
	case EVAL_D5:
		eval_builder_set_features=eval_builder_get_d5_features;
		vector_size[0]=135;
		return eval_builder_create(2,vector_size,vector_times,5,n_games);
	case EVAL_D4:
		eval_builder_set_features=eval_builder_get_d4_features;
		vector_size[0]=45;
		return eval_builder_create(2,vector_size,vector_times,5,n_games);
	case EVAL_D3:
		eval_builder_set_features=eval_builder_get_d3_features;
		vector_size[0]=18;
		return eval_builder_create(2,vector_size,vector_times,5,n_games);
	default:
		/* oups... */
		fprintf(stderr,"invalid feature %d !\n",feature);
		exit(EXIT_FAILURE);
	}
}

/* build the features */
void eval_builder_build_features(EvalBuilder *eval,Gamebase *base,int ply){
	int n=base->n_games,i,I;
	Board b;
	Game *g;

	eval_builder_set_ply(eval,ply);
	for (i=I=0;i<n;i++){
		g=base->games+i;
		if (game_get_board(g,ply,&b) && (!board_is_game_over(&b) || ply==60)){
			if (b.player==BLACK) eval->score[I]=g->score*2-64;
			else eval->score[I]=64-2*g->score;
			eval_builder_set_features(&b,eval->feature[I]);
			I++;
		}
	}
	eval->n_games=I;
}

/* equalize */
void eval_builder_equalize(EvalBuilder *eval,double *w){
	int i, j;
	int K = eval->n_data, I = eval->n_vectors - 1;
	double correction;

	for (i = 0; i < I; i++){
		correction = 0.0;
		for (j = 0; j < eval->vector_size[i]; j++)
			correction += w[j];
		correction /= j;
		for (j = 0; j < eval->vector_size[i]; j++)
			w[j] -= correction;
		w[K-1] += correction * eval->vector_times[i] / eval->vector_times[I];
	}
}

/* count the features */
void eval_builder_zero(EvalBuilder *eval, double *w, int *N, int N_min){
	int k;
	const int K=eval->n_data;
	
	for (k = 0; k < K; k++) if (N[k] < N_min) w[k] = 0.0;
}


/* eval */
void eval_builder_eval(EvalBuilder *eval, int ply, double *x, double *y){
	const int I = eval->n_games, J=eval->n_features;
	int i, j;
	short *c = eval->data[ply];
	int *f;
	double score;
	
	for (i=0;i<I;i++){
		f=eval->feature[i];
		for (score = j = 0; j < J; j++) score += c[f[j]];
		score = BOUND(score, -8191, 8191) / 128.0;
		
		x[i] = score;
		y[i] = eval->score[i];
	}
}

/* count non zero coefficients */
int eval_builder_count_features(EvalBuilder *eval,int ply){
	int i, j, k, n;
	int I = eval->n_games, J = eval->n_features, K = eval->n_data;
	int **x = eval->feature;
	int *h = calloc(K, sizeof (int));

	for (i = 0; i < I; i++)
	for (j = 0; j < J; j++) h[x[i][j]]++;
	for (k = n = 0; k < K; k++) if (h[k]>0) n++;

	free(h);

	return n;
}

/* count non zero coefficients */
int eval_builder_count_significant_coefficients(EvalBuilder *eval,int ply){
	int k, n;
	int K = eval->n_data;
	short *c = eval->data[ply];
	
	for (k = n = 0; k < K; k++) if (c[k] != 0) n++;

	return n;
}

/* count the features */
void eval_builder_get_feature_frequency(EvalBuilder *eval, int *N){
	int i, j, k;
	const int I = eval->n_games, J = eval->n_features, K = eval->n_data;
	int **x = eval->feature;

	for (k = 0; k < K; k++) N[k] = 0;
	for (i = 0; i < I; i++)
	for (j = 0; j < J; j++) N[x[i][j]]++;
}

/* get the weight coefficients */
void eval_builder_get_coefficient(EvalBuilder *eval,double *w){
	int k;
	const int K = eval->n_data;
	short *a = eval->coeff[0];

	for (k = 0; k < K; k++) w[k] = a[k] / 128.0;
}

/* get the game scores */
void eval_builder_get_score(EvalBuilder *eval, double *y){
	int i;
	const int I = eval->n_games;

	for (i = 0; i < I; i++) y[i] = eval->score[i];

}

/* set the weight coefficients */
void eval_builder_set_coefficient(EvalBuilder *eval,double *w){
	int k;
	const int K=eval->n_data;
	short *a=eval->coeff[0];

	for (k=0;k<K;k++) a[k]=(short)(128.0*w[k] + 0.5);
}

/* compute error */
double eval_builder_get_abs_error(EvalBuilder *eval, double *w, double *e){
	int i, j, I = eval->n_games, J = eval->n_features;
	double E = 0.0, score;
	int **x = eval->feature;
	char *y = eval->score;
	
	for (i = 0; i < I; i++){
		score = 0.0; for (j = 0; j < J; j++) score += w[x[i][j]];
		e[i] = y[i] - BOUND(score, -64.0, 64.0);
		E += fabs(e[i]);
	}
	E /= I;
	
	return E;
}

/* compute error gradient */
void eval_builder_get_abs_error_gradient(EvalBuilder *eval,double *e,double *g,int *N,int N_min){
	int i,j,k;
	const int I=eval->n_games,J=eval->n_features,K=eval->n_data;
	int **x=eval->feature;
	
	for (k = 0; k < K; k++) g[k]=0.0;
	for (i = 0; i < I; i++){
		if (e[i] < 0.0) for (j = 0; j < J; j++) g[x[i][j]] ++;
		else if (e[i]>0.0) for (j = 0; j < J; j++) g[x[i][j]] --;
	}
	if (N == NULL) for (k = 0; k < K; k++) g[k] *= 1.0 / I;
	else 
		for (k = 0; k < K; k++)
			g[k] *= (N[k] < N_min ? 0.0 : (N[k] < 20 ? 0.05 : 1.0 / N[k]))/J;
}

/* compute error */
double eval_builder_get_squared_error(EvalBuilder *eval, double *w, double *e){
	int i, j, I = eval->n_games, J = eval->n_features;
	double E = 0.0, score;
	int **x = eval->feature;
	char *y = eval->score;
	
	for (i=0;i<I;i++){
		score = 0.0; for (j = 0; j < J; j++) score += w[x[i][j]];
		e[i] = y[i] - BOUND(score, -64.0, 64.0);
		E += e[i] * e[i];
	}
	E /= I;
	
	return E;
}

/* compute error gradient */
void eval_builder_get_squared_error_gradient(EvalBuilder *eval, double *e, double *g, int *N, int N_min){
	int i, j, k;
	const int I = eval->n_games, J = eval->n_features, K = eval->n_data;
	int **x = eval->feature;

	for (k = 0; k < K; k++) g[k]=0.0;
	for (i = 0; i < I; i++)
	for (j = 0; j < J; j++) g[x[i][j]] -= e[i];
	if (N==NULL) for (k=0;k<K;k++) g[k] *= 2.0/I;
	else for (k=0;k<K;k++) g[k] *= (N[k]<N_min?0.0:(N[k]<20?0.1:2.0/N[k]))/J;
}

/* get the error for the weight w[k] + l *d[k] */
double eval_builder_get_dir_squared_error(EvalBuilder *eval, double *w, double *d, double l){
	int i,j,I=eval->n_games,J=eval->n_features;
	double E=0.0, e;
	int **x=eval->feature;
	char *y=eval->score;
	
	for (i=0;i<I;i++){
		e = y[i];
		for (j = 0; j < J; j++) e -= w[x[i][j]] + l * d[x[i][j]];
		E += e * e;
	}
	E /= I;
	
	return E;
}

/* minimize the absolute error along the gradient direction */
double eval_builder_minimize_dir_abs_error(EvalBuilder *eval,double *w, double *d){
	const int I=eval->n_games, J=eval->n_features;
	double *v = (double*)malloc(I*sizeof (double));   /* a vector */
	int *x;
	double a, b, l, s;
	int i, j, n;
	
	for (i=n=0;i<I;i++){
		x = eval->feature[i];
		s = 0.0; for (j = 0; j < J; j++)	s += w[x[j]];
		a = eval->score[i] - BOUND(s, -64.0, 64.0);
		b = 0.0; for (j = 0; j < J; j++)	b += d[x[j]];
		if (b != 0.0) v[n++] = a / b;
	}
	l = sl_median(v, n);
	if (l <= 0.0) l = DBL_EPSILON; /* otherwise the algo is trapped */
	
	free(v);
	
	return l;
}

/* minimize the absolute error along the gradient direction */
double eval_builder_minimize_dir_squared_error(EvalBuilder *eval,double *w, double *d){
	const int I=eval->n_games, J=eval->n_features;
	int *x;
	double a, b, lambda, A, B, s;
	int i, j, n;
	
	A = B = lambda = 0.0;
	for (i = n = 0; i < I; i++){
		x = eval->feature[i];
		s = 0.0; for (j = 0; j < J; j++)	s += w[x[j]];
		a = eval->score[i] - BOUND(s, -64.0, 64.0);
		b = 0.0; for (j = 0; j < J; j++) b += d[x[j]];
		A += a * b;
		B += b * b;
	}
	if (B > 0.0)	lambda = A / B;
	if (lambda <= 0.0) lambda = DBL_EPSILON; /* never 0 */
	
	return lambda;
}

/* minimize the error */
double eval_builder_minimize_dir_squared_error_using_brent(EvalBuilder *eval,double *w, double *d, double accuracy){
	int k, iter;
	const int K = eval->n_data;
	double e, e_u, e_v, e_w;
	double l, l_a, l_b, l_m, l_u, l_v, l_w;
	double g, f, p, q, r, tolerance;
	const double N_GOLD = .38196601125;
	const int MAX_ITER = 100;
	int can_fail = 1;

	/* check direction */
	for (k = 0; k > K; k++){
		if (d[k] != 0.0) break;
	}
	if (k == K) return 0.0;
	
	/* bracketting lambda (='l') */
	l_a = 0.0; l_b = +10.0;

	/* minimisation using brent algorithm */
	l = l_w = l_v = 0.0;
	e = e_w = e_v = eval_builder_get_dir_squared_error(eval, w, d, l);

brent_start:
	
	f = g = 0.0;
	for (iter = 0; iter<= MAX_ITER; iter++){
		l_m = (l_a + l_b) * 0.5;
		tolerance = accuracy * fabs(l) + 1e-10;
		if (fabs(l - l_m) <= 2.0 * tolerance - 0.5 * (l_b - l_a)){
			if (can_fail){
				can_fail = 0;
				if (l >= (10.0 - 4.0 * tolerance)){
					l_a = l - 2.0 * tolerance; l_b = 100.0;
					l_w = l_v = l;
					e_w = e_v = e;
					goto brent_start;
				} /* l cannot be negative */
			}
			break;
		}
		if (fabs(f) > tolerance){
			r = (l - l_w) * (e - e_v);
			q = (l - l_v) * (e - e_w);
			p = (l - l_v) * q - (l - l_w) * r;
			q = 2.0 * (q - r);
			if (q > 0.0) p = -p; else q = -q;
			r = f;
			f = g;
			if (fabs(p) > fabs( 0.5 *q * r) || p <= q * (l_a - l) || p >= q * (l_b - l)){
				g = (f = (l >= l_m ? l_a - l : l_b - l))* N_GOLD;
			} else {
				g = p / q;
				l_u = l + g;
				if (l_u - l_a < 2.0 * tolerance || l_b - l_u < 2.0 * tolerance){
					g = (l < l_m) ? tolerance : -tolerance;
				}
			}
		} else {
			f = (l >= l_m) ? l_a - l : l_b - l;
			g = f * N_GOLD;
		}
		if (fabs(g) >= tolerance){
			l_u = l + g;
		} else { 
			l_u = l + ((g > 0.0) ? tolerance : - tolerance);
		}
		e_u = eval_builder_get_dir_squared_error(eval, w, d, l_u);
		if (e_u <= e){
			if ( l_u >= l) l_a = l; else l_b = l;
			l_v = l_w; l_w = l; l = l_u;
			e_v = e_w; e_w = e; e = e_u;
		} else {
			if (l_u < l)  l_a = l_u; else l_b = l_u;
			if ( e_u <= e_w || l_w == l){
				l_v = l_w; l_w = l_u;
				e_v = e_w; e_w = e_u;
			} else if (e_u <= e_v || l_v == l || l_v == l_w){
				l_v = l_u;
				e_v = e_u;
			}
		}
	}
	
	return l;
}

/* compute coefficient through conjugate_gradient */
int eval_builder_conjugate_gradient(EvalBuilder *eval,int ply,EvalOption *option){
	int k, iter, i;
	const int I = eval->n_games,K = eval->n_data;
	double r1, r2, err1, err2, v, m;
	double delta, max_delta = 0.0, mean_delta = 0.0; /* weights change */
	double d_gamma, n_gamma, gamma, lambda;
	double *w = (double*)malloc((K)*sizeof (double)); /* weights */
	double *d = (double*)calloc((K),sizeof (double)); /* delta weights = xi*/
	double *g = (double*)malloc((K)*sizeof (double)); /* g_i*/
	double *h = (double*)malloc((K)*sizeof (double)); /* h_i*/
	double *e = (double*)malloc(I*sizeof (double));   /* errors */
	int *N = (int*)malloc((K)*sizeof (int));          /* frequencies */

	eval_builder_get_coefficient(eval, w);
	eval_builder_get_feature_frequency(eval, N);
	
	/* score variance */
	eval_builder_get_score(eval, e);
	if (option->error_type == EVAL_ABS_ERROR){
		m = sl_median(e, I);
		v = 0.0;	
		for (i = 0; i < I; i++) 
			v += fabs(e[i] - m);
		v /= I;
		v *= v;
		err1 = eval_builder_get_abs_error(eval, w, e);
	} else{
		v = sl_variance(e, I);
		err1 = sqrt(eval_builder_get_squared_error(eval, w, e));
	}
	r1 = 1.0 - (err1 * err1) / (v);
	printf("%2d %4d %6.2f %6.3f %8.4f %12.8f %9.5f %9.5f\r", ply, 0, 0.0, 0.0, err1, r1, 0.0, 0.0);
	fflush(stdout);

	for (iter = 1; iter <= option->max_iter; iter++){
		/* compute gradient */
		if (option->error_type == EVAL_ABS_ERROR){
			eval_builder_get_abs_error_gradient(eval, e, d, N, 3);
		}else{
			eval_builder_get_squared_error_gradient(eval, e, d, N, 3);
		}

		/* compute conjugate direction */
		if (iter == 1 || (option->restart_frequency && (iter % option->restart_frequency == 1))){
			gamma = 0.0;
		} else {
			n_gamma = d_gamma = 0.0;
			if (option->minimization_algorithm == EVAL_POLAK_RIBIERE){
				for (k = 0; k < K; k++){
					d_gamma += g[k] * g[k];
					n_gamma += (d[k] + g[k]) * d[k]; /* Polak Ribiere */
				}
			}else if (option->minimization_algorithm == EVAL_FLETCHER_REEVES){
				for (k = 0; k < K; k++){
					d_gamma += g[k] * g[k];
					n_gamma += d[k] * d[k]; /* Fletcher Reeves */
				}
			}
			if (option->minimization_algorithm != EVAL_STEEPEST_DESCENT){
				if (d_gamma < DBL_EPSILON) break;
				gamma = n_gamma / d_gamma;
			}else {
				gamma = 0.0;
			}
		}

		/* minimize along direction */
		if (option->minimization_algorithm == EVAL_STEEPEST_DESCENT){
			for (k = 0; k < K; k++){
				g[k] = -d[k] + option->beta * h[k];
				d[k] = h[k] = option->alpha * g[k];
			}
			lambda = 1.0;
		}else{
			for (k = 0; k < K; k++){
				g[k] = -d[k];
				d[k] = h[k] = g[k] + gamma * h[k];
			}
			if (option->error_type == EVAL_ABS_ERROR){
				lambda = eval_builder_minimize_dir_abs_error(eval, w, d);
			}else{
				lambda = eval_builder_minimize_dir_squared_error(eval, w, d);
			}
		}

		/* update weights */
		mean_delta = 0.0;
		max_delta = 0.0;
		for (k = 0; k < K; k++){
			delta = d[k] * lambda;
			w[k] += delta;
			delta = fabs(delta);
			mean_delta += delta;
			if (max_delta < delta) max_delta = delta;
		}
		mean_delta /= K;

		/* apply various regularisation methods */
		if (option->equalize_frequency && (iter % option->equalize_frequency == 0)){
			eval_builder_equalize(eval, w);
		}
		
		if (option->zero_frequency && (iter % option->zero_frequency == 0)){
			eval_builder_zero(eval, w, N, 3);
		}

		if (option->unbias_frequency && (iter % option->unbias_frequency == 0)){
			if (option->error_type == EVAL_ABS_ERROR){
				eval_builder_get_abs_error(eval, w, e);
				w[K-1] += (m = sl_median(e,I));
			}else{
				eval_builder_get_abs_error(eval, w, e);
				w[K-1] += (m = sl_mean(e,I));
			}
		}
		
		if (option->round_frequency && (iter % option->round_frequency == 0)){
			eval_builder_set_coefficient(eval, w);
			eval_builder_get_coefficient(eval, w);
		}

		/* compute and show error */
		if (option->error_type == EVAL_ABS_ERROR){
			err2 = eval_builder_get_abs_error(eval, w, e);
		}else{
			err2 = sqrt(eval_builder_get_squared_error(eval, w, e));
		}
		r2 = 1.0 - err2 * err2/ v;
		printf("%2d  %4d %6.2f %6.3f %8.4f %12.8f %9.7f %9.7f   %10.8f \r", ply, iter, lambda, gamma, err2, r2, max_delta, mean_delta, fabs(err2-err1));
		fflush(stdout);
		if ((iter > option->min_iter || ply < 2) && (fabs(err2 - err1) <= option->accuracy && fabs(max_delta) < 1000 * option->accuracy && fabs(mean_delta) <= 10 * option->accuracy))
			break;
		err1 = err2;
	}
	putchar('\n');

	/* apply various regularisation methods */
	if (option->equalize_frequency){
		eval_builder_equalize(eval, w);
	}
		
	if (option->zero_frequency){
		eval_builder_zero(eval, w, N, 3);
	}

	if (option->unbias_frequency){
		if (option->error_type == EVAL_ABS_ERROR){
			eval_builder_get_abs_error(eval, w, e);
			w[K-1] += (m = sl_median(e,I));
		}else{
			eval_builder_get_abs_error(eval, w, e);
			w[K-1] += (m = sl_mean(e,I));
		}
	}

	eval_builder_set_coefficient(eval, w);

	free(N);
	free(e);
	free(h);
	free(g);
	free(d);
	free(w);
	
	return iter;
}

/* compute the coefficients */
void eval_builder_build(EvalBuilder *eval,Gamebase *base, EvalOption *option){
	int ply;
	double t=-ul_clock_get_time();

	eval->build++;
	eval->date=ul_clock_get_date();

	printf("Settings:\n");
	printf("accuracy = %g\n", option->accuracy);
	printf("min_iter = %d\n", option->min_iter);
	printf("max_iter = %d\n", option->max_iter);
	printf("round    = %d\n", option->round_frequency);
	printf("unbias   = %d\n", option->unbias_frequency);
	printf("equalize = %d\n", option->equalize_frequency);
	printf("zero     = %d\n", option->zero_frequency);
	printf("restart  = %d\n", option->restart_frequency);
	printf("error    = %d\n", option->error_type);
	printf("algo     = %d\n", option->minimization_algorithm);

	printf("ply iter  lambda gamma  error     r2           max_delta mean_delta err_delta\n");
	for (ply=0;ply<=60;ply++){
		eval_builder_build_features(eval,base,ply);
		eval_builder_conjugate_gradient(eval,ply,option);
	}
	t+=ul_clock_get_time();
	printf("time = ");ul_clock_print_time(t,stdout);putchar('\n');
}

/* filter temporally (between plies) the coefficients */
void eval_builder_temporal_filter(EvalBuilder *eval,Gamebase *base,int max_iter,double accuracy){
	int i,j,k,n,iter;
	int I=eval->n_games,J=eval->n_features,K=eval->n_data,N=eval->n_ply;
	double *a,*a0,*aN,r;
	int **x=eval->feature;
	int **f,*F;
	double c;
	
	eval->build++;
	eval->date=ul_clock_get_date();
	
	/* frequencies */
	printf("computing feature frequencies\n");
	f=(int**)malloc(N*sizeof (int*));
	F=(int*)calloc(K,sizeof (int));
	for (n=0;n<N;n++){
		printf("%5d/%d\r",n,N);fflush(stdout);
		f[n]=(int*)calloc(K,sizeof (int));
		eval_builder_build_features(eval,base,n);
		for (i=0;i<I;i++)
		for (j=0;j<J;j++) f[n][x[i][j]]++;
		for (k=0;k<K;k++) F[k]+=f[n][k];
	}
	
	/* filtering */
	printf("filtering the data\n");
	a=(double*)malloc(N*sizeof (double));
	a0=(double*)malloc(N*sizeof (double));
	aN=(double*)malloc(N*sizeof (double));
	for (k=0;k<K-1;k++){
		for (n=0;n<N;n++) a0[n]=aN[n]=(eval->data[n][k])/128.0;
		if (F[k]==0) continue;
		for (iter=0;iter<max_iter;iter++){
			for (n=0;n<N;n++) a[n]=aN[n];
			r=0.0;
			for (n=1;n<N-1;n++){
				c=sqrt((double)f[n][k]/F[k]);
				aN[n]=c*a0[n]+(1.0-c)*(a[n-1]+a[n+1])*0.5;
				r+=(aN[n]-a[n])*(aN[n]-a[n]);
			}
			if (r<accuracy) break;
		}
		if (k%100==0) printf("%8d/%d\r",k,K);fflush(stdout);
		for (n=0;n<N;n++) eval->data[n][k]=(short)(aN[n]*128.0);
	}
	printf("\n\n");
	free(a);
	free(a0);
	free(aN);
	free(F);
	for (n=0;n<N;n++) free(f[n]);
	free(f);
}

void print_feature(int n,int i){
	int k;
	char x[]="xo.";
	
	for (k=0;k<n;k++){
		putchar(x[i%3]);
		i/=3;
	}
}

/* compress a feature */
void eval_builder_get_feature(int n,int offset,int *feature){
	int *compress[]={NULL,NULL,NULL,s3[0],s4[0],s5[0],s6[0],s7[0],s8[0],c9[0],s10[0],c10[0],s12[0],s12[0]};
	*feature=compress[n][*feature]+offset;
}

/* from one feature get 'n' sub features differing by a single disc */
int MAX_P=3,MAX_L=12,MIN_SQUARE=3;
void eval_builder_get_sub_features(int n,int offset,int feature,int **subfeature,int *P,int *L){
	int k,p,l;
	int x[20];
	int *compress[]={NULL,NULL,NULL,s3[0],s4[0],s5[0],s6[0],s7[0],s8[0],c9[0],s10[0],c10[0],s12[0],s12[0]};

	for (k=0;k<n;k++){
		x[k]=feature%3;
		feature/=3;
	}
	
	*P=3;
	*L=n;
	for (l=0;l<*L;l++)
	for (p=0;p<*P;p++){
		subfeature[p][l]=0;
		for (k=n-1;k>=0;k--){
			subfeature[p][l]*=3;
			subfeature[p][l]+=(k==l? p : x[k]);
		}
		subfeature[p][l]=compress[n][subfeature[p][l]]+offset;
	}
}

/*  filter temporally (between plies) the coefficients */
void eval_builder_spatial_filter(EvalBuilder *eval,Gamebase *base,int max_iter,double accuracy){
	int power_3[]={1,3,9,27,81,243,729,2187,6561,19683,59049,177147,531441};
	int i,j,k,p,n,l,iter;
	int I=eval->n_games,J=eval->n_features,K=eval->n_data,N=eval->n_ply;
	int **x=eval->feature,***X;
	int *f,*L,*P,*todo;
	double W,*w,F,w0,w1,r,a;

	eval->build++;
	eval->date=ul_clock_get_date();

	printf("computing parent features\n");
	L=(int*)calloc(K,sizeof (int));
	P=(int*)calloc(K,sizeof (int));
	todo=(int*)calloc(K,sizeof (int));
	f=(int*)malloc(K*sizeof (int));
	w=(double*)malloc(K*sizeof (double));
	X=(int***)malloc(K*sizeof (int**));
	for (k=0;k<K;k++){
		X[k]=(int**)malloc(MAX_P*sizeof (int*));
		for (p=0;p<MAX_P;p++){
			X[k][p]=(int*)malloc(MAX_L*sizeof (int));
			for (l=0;l<MAX_L;l++) X[k][p][l]=0xffff;
		}
	}

	/* parent features */
	for (i=0;i<eval->n_vectors;i++){
		n=eval->vector_squares[i];
		if (n<MIN_SQUARE) continue;
		for (j=0;j<power_3[n];j++){
			k=j;
			eval_builder_get_feature(n,eval->vector_offset[i],&k);
			eval_builder_get_sub_features(n,eval->vector_offset[i],j,X[k],P+k,L+k);
			todo[k]=1;
		}
	}

	/* filter */
	printf("filtering\n");
	for (n=0;n<N;n++){
		printf("%5d/%d frequencies      \r",n,N);fflush(stdout);
		eval_builder_build_features(eval,base,n);
		for (k=0;k<K;k++) f[k] = 0;
		for (i=0;i<I;i++)
		for (j=0;j<J;j++) f[x[i][j]]++;

		printf("%5d/%d weights          \r",n,N);fflush(stdout);
		for (k=0;k<K;k++) w[k]=eval->data[n][k]/128.0;

		printf("%5d/%d filtering         \r",n,N);fflush(stdout);
		for (iter=0;iter<max_iter;iter++){
			r=0.0;

			for (k=0;k<K;k++){
				if (!todo[k]) continue;
				w0=eval->data[n][k]/128.0;

				w1=0.0;
				for (l=0;l<L[k];l++){
					F=f[k];
					W=w0*f[k];
					for (p=0;p<P[k];p++){
						if (X[k][p][l]!=k){
							F+=f[X[k][p][l]];
							W+=w[X[k][p][l]]*f[X[k][p][l]];
						}
					}
					if (F>0.0) w1+=W/(F*L[k]); else w1+=w0/L[k];
				}
				a=f[k]/100.0;if (a>0.5) a=0.5;
				w1=a*w0+(1.0-a)*w1;
				r+=(w1-w[k])*(w1-w[k]);
				w[k]=w1;
			}
			printf("%5d/%d %3d %12.4f\r",n,N,iter,r);fflush(stdout);
			if (r<accuracy) break;
		}
		for (k=0;k<K;k++) eval->data[n][k]=128.0*w[k];
	}

	for (k=0;k<K;k++){
		for (p=0;p<P[k];p++) free(X[k][p]);
		free(X[k]);
	}
	free(X);
	free(todo);
	free(L);
	free(w);
	free(f);
	free(P);
}

/* equalize */
void eval_builder_equalize_all(EvalBuilder *eval){
	int ply;
	const int K=eval->n_data;
	double *w=(double*)malloc((K)*sizeof (double));	/* weights */

	printf("equalize\n");
	for (ply=0;ply<=60;ply++){
		eval_builder_set_ply(eval,ply);
		eval_builder_get_coefficient(eval,w);
		eval_builder_equalize(eval,w);
		eval_builder_set_coefficient(eval,w);
		printf("%5d/61\r",ply);
		fflush(stdout);
	}

	free(w);
}

/* equalize */
void eval_builder_zero_rare_features(EvalBuilder *eval,Gamebase *base){
	int ply;
	const int K=eval->n_data;
	double *w=(double*)malloc((K)*sizeof (double));	/* weights */
	int *N=(int*)malloc((K)*sizeof (int));			/* frequencies */

	printf("zero rare features\n");
	for (ply=0;ply<=60;ply++){
		eval_builder_set_ply(eval,ply);
		eval_builder_build_features(eval,base,ply);
		eval_builder_get_coefficient(eval,w);
		eval_builder_get_feature_frequency(eval,N);
		eval_builder_zero(eval, w, N, 3);
		eval_builder_set_coefficient(eval,w);
		printf("%5d/61\r",ply);
		fflush(stdout);
	}

	free(N);
	free(w);
}

/* merge */
void eval_builder_merge(EvalBuilder *eval_1, EvalBuilder *eval_2, int split){
	int ply, k;
	const int K=eval_1->n_data;
	double *w1=(double*)malloc((K)*sizeof (double));	/* weights */
	double *w2=(double*)malloc((K)*sizeof (double));	/* weights */

	if (split == 0){
		for (ply=0;ply<=60;ply++){
			eval_builder_set_ply(eval_1,ply);
			eval_builder_set_ply(eval_2,ply);
			eval_builder_get_coefficient(eval_1,w1);
			eval_builder_get_coefficient(eval_2,w2);
			for (k = 0; k < K; k++) w1[k] = (w1[k] + w2[k])*0.5;
			eval_builder_set_coefficient(eval_1,w1);
			printf("%5d/60\r",ply);
			fflush(stdout);
		}
	} else {
		for (ply=split;ply<=60;ply++){
			eval_builder_set_ply(eval_1,ply);		
			eval_builder_set_ply(eval_2,ply);		
			eval_builder_get_coefficient(eval_2,w2);
			eval_builder_set_coefficient(eval_1,w2);
			printf("%5d/60\r",ply);
			fflush(stdout);
		}
	}

	free(w1);
	free(w2);
}

/* remove the bias */
void eval_builder_unbias(EvalBuilder *eval, Gamebase *base, int error_type){
	double *e,*w=NULL,bias;
	int ply,I,K=0;
	
	e=(double *)malloc(base->n_games*sizeof (double));
	
	printf("correcting weight bias\n");
	for (ply=0;ply<=60;ply++){
		eval_builder_build_features(eval,base,ply);
		I=eval->n_games;
		if (w==NULL){
			K=eval->n_data;
			w=(double *)malloc(K*sizeof (double));
		}
		eval_builder_get_coefficient(eval,w);
		if (error_type == EVAL_ABS_ERROR){
			eval_builder_get_abs_error(eval,w,e);
			w[K-1]+=(bias=sl_median(e,I));
		} else {
			eval_builder_get_squared_error(eval,w,e);
			w[K-1]+=(bias=sl_mean(e,I));
		}
		eval_builder_set_coefficient(eval,w);
		printf("%5d/61 parity = %+6.2f (correction = %+6.2f)\r",ply,w[K-1],bias);
		fflush(stdout);
	}	
	free(w);
	free(e);
}

/* print some statisitics */
void eval_builder_stat(EvalBuilder *eval,Gamebase *base){
	double *x,*y,*e;
	int i,ply,n;
	
	x=(double *)malloc(base->n_games*sizeof (double));
	y=(double *)malloc(base->n_games*sizeof (double));
	e=(double *)malloc(base->n_games*sizeof (double));

	printf("n coeffs\teval mean\teval sdev\teval min\teval max\tscore mean\tscore sdev\tscore min\tscore max\ta\tb\tr\terror bias\terror sdev\terror min\terror max\n");
	
	for (ply=0;ply<=60;ply++){
		eval_builder_build_features(eval,base,ply);		
		eval_builder_eval(eval,ply,x,y);
		n=eval->n_games;
		for (i=0;i<n;i++) e[i]=y[i]-x[i];
		printf("%6d\t",eval_builder_count_features(eval,ply));
		printf("%6d\t",eval_builder_count_significant_coefficients(eval,ply));
		printf("%5.2f\t",sl_mean(x,n));
		printf("%5.2f\t",sl_standard_deviation(x,n));
		printf("%5.2f\t",sl_min(x,n));
		printf("%5.2f\t",sl_max(x,n));
		printf("%5.2f\t",sl_mean(y,n));
		printf("%5.2f\t",sl_standard_deviation(y,n));
		printf("%3.0f\t",sl_min(y,n));
		printf("%3.0f\t",sl_max(y,n));
		printf("%7.4f\t",sl_regression_a(x,y,n));
		printf("%7.4f\t",sl_regression_b(x,y,n));
		printf("%7.4f\t",sl_correlation_r(x,y,n));
		printf("%5.2f\t",sl_mean(e,n));
		printf("%5.2f\t",sl_standard_deviation(e,n));
		printf("%5.2f\t",sl_min(e,n));
		printf("%5.2f\n",sl_max(e,n));
		fflush(stdout);
	}	
	free(e);
	free(y);
	free(x);
}

/* diff */
void eval_builder_diff(EvalBuilder *eval_1, EvalBuilder *eval_2){
	int ply, k, n;
	const int K=eval_1->n_data;
	double *w1=(double*)malloc((K)*sizeof (double));	/* weights */
	double *w2=(double*)malloc((K)*sizeof (double));	/* weights */
	double *d=(double*)malloc((K)*sizeof (double));	/* diff */
	double max_diff, min_diff, abs_diff, dev_diff, avg_diff, eps_diff;
	double t_max_diff, t_min_diff, t_abs_diff, t_dev_diff, t_avg_diff, t_eps_diff;
	int *histo,*t_histo = NULL;

	t_avg_diff = t_abs_diff = t_dev_diff = t_min_diff = t_max_diff = t_eps_diff = 0;;

	printf("ply\tmean\tabsmean\tdeviation\tmin\tmax\tabsmin\n");
	for (ply=0;ply<=60;ply++){
		eval_builder_set_ply(eval_1,ply);		
		eval_builder_set_ply(eval_2,ply);		
		eval_builder_get_coefficient(eval_1,w1);
		eval_builder_get_coefficient(eval_2,w2);
		for (k = n = 0; k < K; k++){
		 	if (w1[k] != 0.0 && w2[k]!= 0.0) d[n++] = (w1[k] - w2[k]);
		}
		max_diff = sl_max(d, n);
		min_diff = sl_min(d, n);
		avg_diff = sl_mean(d, n);
		dev_diff = sl_standard_deviation(d, K);
		for (k = 0; k < n; k++) d[k] = fabs(d[k]);
		abs_diff = sl_mean(d, n);
		eps_diff = sl_min(d, n);
		printf("%3d\t%7.4f\t%7.4f\t%7.4f\t%7.4f\t%7.4f\t%7.4f\n",
			ply,avg_diff,abs_diff,dev_diff,min_diff,max_diff,eps_diff);
		fflush(stdout);
		
		t_avg_diff += avg_diff;
		t_dev_diff += dev_diff;
		t_abs_diff += abs_diff;
		if (ply == 0){
			t_histo = sl_histogram1(d, n, 0.0, 10.0, 100);
			t_max_diff = max_diff;
			t_min_diff = min_diff;
			t_eps_diff = eps_diff;
		}else{
			histo = sl_histogram1(d, n, 0.0, 10.0, 100);
			for (k = 0; k < 100; k++) t_histo[k] += histo[k]; 
			free(histo);
			if (eps_diff < t_eps_diff) t_eps_diff = eps_diff;
			if (max_diff > t_max_diff) t_max_diff = max_diff;
			if (min_diff < t_min_diff) t_min_diff = min_diff;
		}
	}

	t_avg_diff /= ply;
	t_dev_diff /= ply;
	t_abs_diff /= ply;
	printf("------------------------------------------------------------\n");
	printf("total\t%7.4f\t%7.4f\t%7.4f\t%7.4f\t%7.4f\t%7.4f\n",
		t_avg_diff,t_abs_diff,t_dev_diff,t_min_diff,t_max_diff,t_eps_diff);
		
	if (t_histo != NULL){
		printf("\na\tb\tn_diff\n");
		for (k = 0; k < 100; k++){
			printf("%4.1f\t%4.1f\t%8d\n", 0.1 * k, 0.1 * (k+1), t_histo[k]);
		}
		free(t_histo);
	}

	free(w1);
	free(w2);
	free(d);
}

/* diff */
void eval_builder_plot(EvalBuilder *eval,Gamebase *base,const char *plot_file){
	int ply, i;
	const int I=base->n_games;
	double *x=(double*)malloc(I*sizeof (double));	/* score */
	double *y=(double*)malloc(I*sizeof (double));	/* eval */
	char file[256],title[64];
	sl_Plot *plot;
	sl_Point A={-64,-64},B={64,64},O={0,0};
	sl_Point *X= (sl_Point *)malloc(I*sizeof (sl_Point));

	for (ply=0;ply<=60;ply++){
		eval_builder_build_features(eval,base,ply);		
		eval_builder_eval(eval,ply,x,y);
		for (i = 0; i < I; i++) X[i].x = x[i], X[i].y=y[i];
		sprintf(file,"%s-%d.eps",plot_file,ply);
		sprintf(title,"ply %d.eps",ply);

		plot = sl_plot_open(file);
			sl_plot_titles(plot,"eval","score",title);
			sl_plot_axis(plot,&A,&B,&O);
			sl_plot_scatter(plot,X,I);
		sl_plot_close(plot);
		
	}
	
	free(x);
	free(y);
	free(X);
}

/* show weights of a feature*/
void eval_builder_show_feature_weights(EvalBuilder *eval, int type, const char *feature){
	int	i, k, n=eval->vector_squares[type], ply;
	const int K=eval->n_data;
	double *w=(double*)malloc((K)*sizeof (double));	/* weights */
	double sum;
	
	printf("ply\t%s\n",feature);
	if (strcmp(feature, "sum") == 0){
		for (ply = 0; ply <= 60; ply++){
			eval_builder_set_ply(eval,ply);
			eval_builder_get_coefficient(eval,w);
			sum = 0.0;
			for (k = eval->vector_offset[type]; k < eval->vector_offset[type + 1]; k++){
				sum += w[k];
			}
			printf("%3d\t%.4f\n",ply,sum/eval->vector_size[type]);
		 }
	}else{
		if (n == 0 || strcmp(feature,"bias")==0){
			k = K - 1;
		} else {
			for (i = k = 0; i < n; i++){
				switch(tolower(feature[i])){
				case 'o':
					k = k * 3 + 0;
					break;
				case 'x':
					k = k * 3 + 1;
					break;
				default:
					k = k * 3 + 2;		 
					break;
				}
			}
			eval_builder_get_feature(n, eval->vector_offset[type], &k);
		}
		for (ply = 0; ply <= 60; ply++){
			eval_builder_set_ply(eval,ply);		
			eval_builder_get_coefficient(eval,w);		
			printf("%3d\t%.4f\n",ply,w[k]);
		}
	}
	
	
	free(w);
}

/* print version */
void print_version(void){
	printf("eval_builder %d.%d %s\n",EDAX_VERSION,EDAX_RELEASE,__DATE__);
	printf("Copyright (c) 1998-2000 Richard A. Delorme.\n");
	printf("All Rights Reserved.\n\n");
}

/* print usage */
void print_usage(void){
	fprintf(stderr,"usage : eval_builder <command> <option> <parameters>\n");
	fprintf(stderr,"options:\n");
	fprintf(stderr,"  -tol <float>     set tolerated accuracy.\n");
	fprintf(stderr,"  -max_iter <int>  set maximum tolerated iterations.\n");
	fprintf(stderr,"  -min_iter <int>  set minimum tolerated iterations.\n");
	fprintf(stderr,"  -algo <string>   set algorithm for minimisation:\n");
	fprintf(stderr,"    simple         steepest descent (default)\n");
	fprintf(stderr,"    fletcher       Fletcher-reeves conjugate gradient\n");
	fprintf(stderr,"    polak          Polak-ribiere conjugate gradient\n");
	fprintf(stderr,"  -error <string>  set error type for minimisation:\n");
	fprintf(stderr,"    abs            absolute error\n");
	fprintf(stderr,"    squared        squared_error\n");
	fprintf(stderr,"  -eval <string>   evaluation function to compute\n");
	fprintf(stderr,"    edax           edax evaluation function (default)\n");
	fprintf(stderr,"    ajax           ajax evaluation function\n");
	fprintf(stderr,"    logistello     logistello/zebra evaluation function\n");
	fprintf(stderr,"    <feature>      a single feature, like corner3x3\n");
	fprintf(stderr,"  -unbias <int>    unbias the evaluation function\n");
	fprintf(stderr,"  -equalize <int>  equalize the evaluation function weight\n");
	fprintf(stderr,"  -restart <int>   restart frequency\n");
	fprintf(stderr,"  -round <int>     round frequency\n");
	fprintf(stderr,"  -filter <string> filter the evaluation function weight first\n");
	fprintf(stderr,"    spatial        filter from sub-configuration\n");
	fprintf(stderr,"    temporal       filter through all plies\n");
	fprintf(stderr,"  -split <int>     ply to split file before merging them\n");

	fprintf(stderr,"commands:\n");
	fprintf(stderr,"build <option> game_file [eval_file_in] eval_file_out\n");
	fprintf(stderr,"process <option> game_file [eval_file_in] eval_file_out\n");
	fprintf(stderr,"merge <option> eval_file1 eval_file2 eval_file_out\n");
	fprintf(stderr,"stat <option> game_file eval_file\n");
	fprintf(stderr,"diff <option> game_file eval_file\n");
	fprintf(stderr,"plot <option> game_file eval_file plot_file\n");
	fprintf(stderr,"show eval_file feature_type feature\n");
	
	exit(EXIT_FAILURE);
}

/* main */
int main(int argc,char **argv){
	Gamebase *base;
	EvalBuilder *eval_data, *eval_data_1, *eval_data_2;
	EvalOption option= {
		0,
		1000,
		0.0001,
		0,
		0,
		0,
		0,
		50,
		EVAL_STEEPEST_DESCENT,
		EVAL_SQUARED_ERROR,
		1.0,
		0.1
	};

	int filter, eval;
	int split;
	int feature;
	char *file_1, *file_2, *file_3;
	int i;
		
	print_version();
	if (argc < 4) print_usage();
	
	filter = FILTER_NONE;
	eval = EVAL_EDAX;
	file_1 = file_2 = file_3 = NULL;
	split = feature = 0;
	
	for (i = 2; i < argc; i++){
		if (strcmp(argv[i],"-tol")==0){
			option.accuracy = atof(argv[++i]);
		}else if (strcmp(argv[i],"-max_iter")==0){
			option.max_iter = atoi(argv[++i]);
		}else if (strcmp(argv[i],"-min_iter")==0){
			option.min_iter = atoi(argv[++i]);
		}else if (strcmp(argv[i],"-round")==0){
			option.round_frequency = atoi(argv[++i]);
		}else if (strcmp(argv[i],"-equalize")==0){
			option.equalize_frequency = atoi(argv[++i]);
		}else if (strcmp(argv[i],"-zero")==0){
			option.zero_frequency = atoi(argv[++i]);
		}else if (strcmp(argv[i],"-unbias")==0){
			option.unbias_frequency = atoi(argv[++i]);
		}else if (strcmp(argv[i],"-restart")==0){
			option.restart_frequency = atoi(argv[++i]);
		}else if (strcmp(argv[i],"-error")==0){
			option.error_type = atoi(argv[++i]);
		}else if (strcmp(argv[i],"-alpha")==0){
			option.alpha = atoi(argv[++i]);
		}else if (strcmp(argv[i],"-beta")==0){
			option.beta = atoi(argv[++i]);
		}else if (strcmp(argv[i],"-algo")==0){
			if (strcmp(argv[++i],"simple")==0) option.minimization_algorithm = EVAL_STEEPEST_DESCENT;
			else if (strcmp(argv[i],"fletcher")==0) option.minimization_algorithm = EVAL_FLETCHER_REEVES;
			else if (strcmp(argv[i],"polak")==0) option.minimization_algorithm = EVAL_POLAK_RIBIERE;
			else print_usage();
		}else if (strcmp(argv[i],"-error")==0){
			if (strcmp(argv[++i],"abs")==0) option.error_type = EVAL_ABS_ERROR;
			else if (strcmp(argv[i],"squared")==0) option.error_type = EVAL_SQUARED_ERROR;
			else print_usage();
		}else if (strcmp(argv[i],"-eval")==0){
			if (strcmp(argv[++i],"edax")==0) eval = EVAL_EDAX;
			else if (strcmp(argv[i],"edax3b")==0) eval = EVAL_EDAX_3b;
			else if (strcmp(argv[i],"edax3c")==0) eval = EVAL_EDAX_3c;
			else if (strcmp(argv[i],"edax3d")==0) eval = EVAL_EDAX_3d;
			else if (strcmp(argv[i],"ajax")==0) eval = EVAL_AJAX;
			else if (strcmp(argv[i],"logistello")==0) eval = EVAL_LOGISTELLO;
			else if (strcmp(argv[i],"corner3x3")==0) eval = EVAL_CORNER3x3;
			else if (strcmp(argv[i],"corner5x2")==0) eval = EVAL_CORNER5x2;
			else if (strcmp(argv[i],"edge")==0) eval = EVAL_EDGE;
			else if (strcmp(argv[i],"edgeX")==0) eval = EVAL_EDGE_X;
			else if (strcmp(argv[i],"edgeC")==0) eval = EVAL_EDGE_C;
			else if (strcmp(argv[i],"edgeCX")==0) eval = EVAL_EDGE_CX;
			else if (strcmp(argv[i],"edgeFG")==0) eval = EVAL_EDGE_FG;
			else if (strcmp(argv[i],"ABFG")==0) eval = EVAL_ABFG;
			else if (strcmp(argv[i],"CC")==0) eval = EVAL_CC;
			else if (strcmp(argv[i],"BB")==0) eval = EVAL_BB;
			else if (strcmp(argv[i],"AA")==0) eval = EVAL_AA;
			else if (strcmp(argv[i],"D8")==0) eval = EVAL_D8;
			else if (strcmp(argv[i],"D7")==0) eval = EVAL_D7;
			else if (strcmp(argv[i],"D6")==0) eval = EVAL_D6;
			else if (strcmp(argv[i],"D5")==0) eval = EVAL_D5;
			else if (strcmp(argv[i],"D4")==0) eval = EVAL_D4;
			else if (strcmp(argv[i],"D3")==0) eval = EVAL_D3;
			else print_usage();
		}else if (strcmp(argv[i],"-feature")==0){
			if (strcmp(argv[++i],"corner3x3")==0) feature = EVAL_CORNER3x3;
			else if (strcmp(argv[i],"corner5x2")==0) feature = EVAL_CORNER5x2;
			else if (strcmp(argv[i],"edge")==0) feature = EVAL_EDGE;
			else if (strcmp(argv[i],"edgeX")==0) feature = EVAL_EDGE_X;
			else if (strcmp(argv[i],"edgeC")==0) feature = EVAL_EDGE_C;
			else if (strcmp(argv[i],"edgeCX")==0) feature = EVAL_EDGE_CX;
			else if (strcmp(argv[i],"edgeFG")==0) feature = EVAL_EDGE_FG;
			else if (strcmp(argv[i],"ABFG")==0) feature = EVAL_ABFG;
			else if (strcmp(argv[i],"CC")==0) feature = EVAL_CC;
			else if (strcmp(argv[i],"BB")==0) feature = EVAL_BB;
			else if (strcmp(argv[i],"AA")==0) feature = EVAL_AA;
			else if (strcmp(argv[i],"D8")==0) feature = EVAL_D8;
			else if (strcmp(argv[i],"D7")==0) feature = EVAL_D7;
			else if (strcmp(argv[i],"D6")==0) feature = EVAL_D6;
			else if (strcmp(argv[i],"D5")==0) feature = EVAL_D5;
			else if (strcmp(argv[i],"D4")==0) feature = EVAL_D4;
			else if (strcmp(argv[i],"D3")==0) feature = EVAL_D3;
			else print_usage();
		}else if (strcmp(argv[i],"-split")==0){
			split = atoi(argv[++i]);
		}else if (strcmp(argv[i],"-filter")==0){
			if (strcmp(argv[++i],"spatial")==0) filter = FILTER_SPATIAL;
			else if (strcmp(argv[i],"temporal")==0) filter = FILTER_TEMPORAL;
			else print_usage();
		}else if (file_1 == NULL){
			file_1 = argv[i];
		}else if (file_2 == NULL){
			file_2 = argv[i];
		}else if (file_3 == NULL){
			file_3 = argv[i];
		}else print_usage();
	}

	/* build the evaluation function */
	if (strcmp(argv[1],"build")==0){
		if (file_1 == NULL || file_2 == NULL) print_usage();

		base = gamebase_create(0);
		gamebase_import(base, file_1);
		printf("eval_builder : read %d games\n", base->n_games);

		switch(eval){
/*		case EVAL_EDAX:
			eval_data = eval_builder_create_edax(base->n_games);
			break;
		case EVAL_EDAX_3b:
			eval_data = eval_builder_create_edax3b(base->n_games);
			break;
*/		case EVAL_EDAX_3c:
			eval_data = eval_builder_create_edax3c(base->n_games);
			break;
		case EVAL_EDAX_3d:
			eval_data = eval_builder_create_edax3d(base->n_games);
			break;
		case EVAL_AJAX:
			fprintf(stderr,"NOT IMPLEMENTED YET\n");
			exit(EXIT_FAILURE);
		case EVAL_LOGISTELLO:
			eval_data = eval_builder_create_logistello(base->n_games);
			break;
		default:
			eval_data = eval_builder_create_feature(base->n_games, eval);
			break;
		}
		if (file_3 != NULL)
			eval_builder_read(eval_data, file_2);

		eval_builder_build(eval_data, base, &option);

		if (file_3 != NULL)
			eval_builder_write(eval_data, file_3);
		else
			eval_builder_write(eval_data, file_2);
	}

	/* process */
	else if (strcmp(argv[1],"process")==0){
		if (file_1 == NULL || file_2 == NULL) print_usage();

		base = gamebase_create(0);
		gamebase_import(base, file_1);
		printf("eval_builder : read %d games\n", base->n_games);

		switch(eval){
/*		case EVAL_EDAX:
			eval_data = eval_builder_create_edax(base->n_games);
			break;
		case EVAL_EDAX_3b:
			eval_data = eval_builder_create_edax3b(base->n_games);
			break;
*/		case EVAL_EDAX_3c:
			eval_data = eval_builder_create_edax3c(base->n_games);
			break;
		case EVAL_EDAX_3d:
			eval_data = eval_builder_create_edax3d(base->n_games);
			break;
		case EVAL_AJAX:
			fprintf(stderr,"NOT IMPLEMENTED YET\n");
			exit(EXIT_FAILURE);
		case EVAL_LOGISTELLO:
			eval_data = eval_builder_create_logistello(base->n_games);
			break;
		default:
			eval_data = eval_builder_create_feature(base->n_games, eval);
			break;
		}
		if (file_3 != NULL)
			eval_builder_read(eval_data, file_2);

		if (filter == FILTER_SPATIAL)
			eval_builder_spatial_filter(eval_data, base, option.max_iter, option.accuracy);
		if (filter == FILTER_TEMPORAL)
			eval_builder_temporal_filter(eval_data, base, option.max_iter, option.accuracy);
		if (option.equalize_frequency)
			eval_builder_equalize_all(eval_data);
		if (option.zero_frequency)
			eval_builder_zero_rare_features(eval_data, base);
		if (option.unbias_frequency)
			eval_builder_unbias(eval_data, base, option.error_type);

		if (file_3 != NULL)
			eval_builder_write(eval_data, file_3);
		else
			eval_builder_write(eval_data, file_2);
	}

	/* statistics */
	else if (strcmp(argv[1],"stat")==0){
		if (file_1 == NULL || file_2 == NULL) print_usage();

		base = gamebase_create(0);
		gamebase_import(base, file_1);
		printf("eval_builder : read %d games\n", base->n_games);

		switch(eval){
/*		case EVAL_EDAX:
			eval_data = eval_builder_create_edax(base->n_games);
			break;
		case EVAL_EDAX_3b:
			eval_data = eval_builder_create_edax3b(base->n_games);
			break;
*/		case EVAL_EDAX_3c:
			eval_data = eval_builder_create_edax3c(base->n_games);
			break;
		case EVAL_EDAX_3d:
			eval_data = eval_builder_create_edax3d(base->n_games);
			break;
		case EVAL_AJAX:
			fprintf(stderr,"NOT IMPLEMENTED YET\n");
			exit(EXIT_FAILURE);
		case EVAL_LOGISTELLO:
			eval_data = eval_builder_create_logistello(base->n_games);
			break;
		default:
			eval_data = eval_builder_create_feature(base->n_games, eval);
			break;
		}
		eval_builder_read(eval_data, file_2);
		eval_builder_stat(eval_data, base);

	/* merge */
	}else if (strcmp(argv[1],"merge")==0){
		if (file_1 == NULL || file_2 == NULL || file_3 == NULL) print_usage();

		switch(eval){
/*		case EVAL_EDAX:
			eval_data_1 = eval_builder_create_edax(1);
			eval_data_2 = eval_builder_create_edax(1);
			break;
		case EVAL_EDAX_3b:
			eval_data_1 = eval_builder_create_edax3b(1);
			eval_data_2 = eval_builder_create_edax3b(1);
			break;
*/		case EVAL_EDAX_3c:
			eval_data_1 = eval_builder_create_edax3c(1);
			eval_data_2 = eval_builder_create_edax3c(1);
			break;
		case EVAL_EDAX_3d:
			eval_data_1 = eval_builder_create_edax3d(1);
			eval_data_2 = eval_builder_create_edax3d(1);
			break;
		case EVAL_AJAX:
			fprintf(stderr,"NOT IMPLEMENTED YET\n");
			exit(EXIT_FAILURE);
		case EVAL_LOGISTELLO:
			eval_data_1 = eval_builder_create_logistello(1);
			eval_data_2 = eval_builder_create_logistello(1);
			break;
		default:
			eval_data_1 = eval_builder_create_feature(1, eval);
			eval_data_2 = eval_builder_create_feature(1, eval);
			break;
		}
		eval_builder_read(eval_data_1, file_1);
		eval_builder_read(eval_data_2, file_2);

		eval_builder_merge(eval_data_1, eval_data_2, split);

		eval_builder_write(eval_data_1, file_3);

	/* diff */
	}else if (strcmp(argv[1],"diff")==0){
		if (file_1 == NULL || file_2 == NULL) print_usage();

		switch(eval){
/*		case EVAL_EDAX:
			eval_data_1 = eval_builder_create_edax(1);
			eval_data_2 = eval_builder_create_edax(1);
			break;
		case EVAL_EDAX_3b:
			eval_data_1 = eval_builder_create_edax3b(1);
			eval_data_2 = eval_builder_create_edax3b(1);
			break;
*/		case EVAL_EDAX_3c:
			eval_data_1 = eval_builder_create_edax3c(1);
			eval_data_2 = eval_builder_create_edax3c(1);
			break;
		case EVAL_EDAX_3d:
			eval_data_1 = eval_builder_create_edax3d(1);
			eval_data_2 = eval_builder_create_edax3d(1);
			break;
		case EVAL_AJAX:
			fprintf(stderr,"NOT IMPLEMENTED YET\n");
			exit(EXIT_FAILURE);
		case EVAL_LOGISTELLO:
			eval_data_1 = eval_builder_create_logistello(1);
			eval_data_2 = eval_builder_create_logistello(1);
			break;
		default:
			eval_data_1 = eval_builder_create_feature(1, eval);
			eval_data_2 = eval_builder_create_feature(1, eval);
			break;
		}
		eval_builder_read(eval_data_1, file_1);
		eval_builder_read(eval_data_2, file_2);

		eval_builder_diff(eval_data_1, eval_data_2);

	/* plot */
	}else if (strcmp(argv[1],"plot")==0){
		if (file_1 == NULL || file_2 == NULL || file_3 == NULL) print_usage();

		base = gamebase_create(0);
		gamebase_import(base, file_1);
		printf("eval_builder : read %d games\n", base->n_games);

		switch(eval){
/*		case EVAL_EDAX:
			eval_data = eval_builder_create_edax(base->n_games);
			break;
		case EVAL_EDAX_3b:
			eval_data = eval_builder_create_edax3b(base->n_games);
			break;
*/		case EVAL_EDAX_3c:
			eval_data = eval_builder_create_edax3c(base->n_games);
			break;
		case EVAL_EDAX_3d:
			eval_data = eval_builder_create_edax3d(base->n_games);
			break;
		case EVAL_AJAX:
			fprintf(stderr,"NOT IMPLEMENTED YET\n");
			exit(EXIT_FAILURE);
		case EVAL_LOGISTELLO:
			eval_data = eval_builder_create_logistello(base->n_games);
			break;
		default:
			eval_data = eval_builder_create_feature(base->n_games, eval);
			break;
		}
		eval_builder_read(eval_data, file_2);
		eval_builder_plot(eval_data, base, file_3);

	/* plot */
	}else if (strcmp(argv[1],"show")==0){
		if (file_1 == NULL || file_2 == NULL) print_usage();

		switch(eval){
/*		case EVAL_EDAX:
			eval_data = eval_builder_create_edax(1);
			switch(feature){
			case EVAL_CORNER3x3:
				feature = 0;
				break;
			case EVAL_EDGE_X:
				feature = 1;
				break;
			case EVAL_ABFG:
				feature = 2;
				break;
			case EVAL_CC:
				feature = 3;
				break;
			case EVAL_AA:
				feature = 4;
				break;
			case EVAL_BB:
				feature = 5;
				break;
			case EVAL_D8:
				feature = 6;
				break;
			case EVAL_D7:
				feature = 7;
				break;
			case EVAL_D6:
				feature = 8;
				break;
			case EVAL_D5:
				feature = 9;
				break;
			case EVAL_D4:
				feature = 10;
				break;
			default:
				feature = 11;
				break;
			}
			break;
*/		case EVAL_EDAX_3b:
		case EVAL_EDAX_3c:
		case EVAL_EDAX_3d:
		case EVAL_AJAX:
		case EVAL_LOGISTELLO:
		default:
			fprintf(stderr,"NOT IMPLEMENTED YET\n");
			exit(EXIT_FAILURE);
			break;
		}
		if (eval_data != NULL){
			eval_builder_read(eval_data, file_1);
			printf("[%d] : %s\n", feature, file_2);
			eval_builder_show_feature_weights(eval_data, feature, file_2);
		}

	/* print usage */
	}else{
		print_usage();
	}

	return EXIT_SUCCESS;
}
