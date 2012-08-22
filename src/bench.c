/**
 * @file bench.c
 *
 * @date 1998 - 2012
 * @author Richard Delorme
 * @version 4.3
 */

#include "bit.h"
#include "board.h"
#include "move.h"
#include "options.h"
#include "search.h"
#include "util.h"

#include <math.h>

/*
 * @brief return a CPU clock tick.
 *
 * @return a CPU clock tick.
 */
static unsigned long long click()
{
#if defined(USE_GAS_X64)

	unsigned int a, d;
	__asm__ volatile (
		"rdtsc" : "=a" (a), "=d" (d));
	return a | (((unsigned long long)d) << 32);

#elif defined(USE_GAS_X86)
	unsigned long long a;
	__asm__ volatile (
		"rdtsc" : "=A" (a));
	return a;
#elif defined(_WIN32)
	return __rdtsc();
#else
	return cpu_clock();
#endif
}

/*
 * @brief Move generator performance test.
 */
static void bench_move_generator()
{
	const char *b = "OOOOOOOOOXXXXXXOOXXXXXXOOXXXXXXOOXXXXXXOOXXXXXXOOXXXXXXOOOOOOOOO O";
	char m[4];
	Board board;
	Move move;
	int i, x;
	const int N_WARMUP = 1000;
	const int N_REPEAT = 1000000;
	unsigned long long c, overhead;
	double t, t_mean, t_var, t_min, t_max;

	c = -click();
	for (i = 0; i < N_WARMUP; ++i) {
	}
	c += click();

	c = -click();
	for (i = 0; i < N_REPEAT; ++i) {
	}
	c += click();
	overhead = c;

	t_mean = t_var = 0.0;
	t_max = 0;
	t_min = 1e30;

	for (x = A1; x < PASS; ++x) {
		board_set(&board, b);
		board.player &= ~x_to_bit(x);
		board.opponent &= ~x_to_bit(x);
		
		c = -click();
		for (i = 0; i < N_WARMUP; ++i) {
			board_get_move(&board, x, &move);
		}
		c += click();

		c = -click();
		for (i = 0; i < N_REPEAT; ++i) {
			board_get_move(&board, x, &move);
		}
		c += click();

		t = ((double)(c - overhead)) / N_REPEAT;
		t_mean += t;
		t_var += t * t;
		if (t < t_min) t_min = t;
		if (t > t_max) t_max = t;

		if (options.verbosity >= 2) printf("board_get_move: %s %.1f clicks;\n", move_to_string(x, WHITE, m), t);

	}

	t_mean /= x;
	t_var = t_var / x - (t_mean * t_mean);
	
	printf("board_get_move:  %.2f < %.2f +/- %.2f < %.2f\n", t_min, t_mean, sqrt(t_var), t_max);
}

/*
 * @brief Last Move performance test.
 */
static void bench_count_last_flip()
{
	const char *b = "OOOOOOOOOXXXXXXOOXXXXXXOOXXXXXXOOXXXXXXOOXXXXXXOOXXXXXXOOOOOOOOO O";
	char m[4];
	Board board;
	int i, x;
	const int N_WARMUP = 1000;
	const int N_REPEAT = 1000000;
	unsigned long long c, overhead;
	double t, t_mean, t_var, t_min, t_max;

	c = -click();
	for (i = 0; i < N_WARMUP; ++i) {
	}
	c += click();

	c = -click();
	for (i = 0; i < N_REPEAT; ++i) {
	}
	c += click();
	overhead = c;

	t_mean = t_var = 0.0;
	t_max = 0;
	t_min = 1e30;

	for (x = A1; x < PASS; ++x) {
		board_set(&board, b);
		board.player &= ~x_to_bit(x);
		board.opponent &= ~x_to_bit(x);
		
		c = -click();
		for (i = 0; i < N_WARMUP; ++i) {
			count_last_flip[x](board.player);
		}
		c += click();

		c = -click();
		for (i = 0; i < N_REPEAT; ++i) {
			count_last_flip[x](board.player);
		}
		c += click();

		t = ((double)(c - overhead)) / N_REPEAT;
		t_mean += t;
		t_var += t * t;
		if (t < t_min) t_min = t;
		if (t > t_max) t_max = t;

		if (options.verbosity >= 2) printf("count_last_flip: %s %.1f clicks;\n", move_to_string(x, WHITE, m), t);

	}

	t_mean /= x;
	t_var = t_var / x - (t_mean * t_mean);
	
	printf("count_last_flip:  %.2f < %.2f +/- %.2f < %.2f\n", t_min, t_mean, sqrt(t_var), t_max);
}

/*
 * @brief Scoring performance test.
 */
static void bench_board_score_1()
{
	const char *b = "OOOOOOOOOXXXXXXOOXXXXXXOOXXXXXXOOXXXXXXOOXXXXXXOOXXXXXXOOOOOOOOO O";
	char m[4];
	Board board;
	int i, x;
	const int N_WARMUP = 1000;
	const int N_REPEAT = 1000000;
	unsigned long long c, overhead;
	double t, t_mean, t_var, t_min, t_max;


	c = -click();
	for (i = 0; i < N_WARMUP; ++i) {
	}
	c += click();

	c = -click();
	for (i = 0; i < N_REPEAT; ++i) {
	}
	c += click();
	overhead = c;

	t_mean = t_var = 0.0;
	t_max = 0;
	t_min = 1e30;

	for (x = A1; x < PASS; ++x) {
		board_set(&board, b);
		board.player &= ~x_to_bit(x);
		board.opponent &= ~x_to_bit(x);
		
		c = -click();
		for (i = 0; i < N_WARMUP; ++i) {
			board_score_1(&board, SCORE_MAX, x);
		}
		c += click();

		c = -click();
		for (i = 0; i < N_REPEAT; ++i) {
			board_score_1(&board, SCORE_MAX, x);
		}
		c += click();

		t = ((double)(c - overhead)) / N_REPEAT;
		t_mean += t;
		t_var += t * t;
		if (t < t_min) t_min = t;
		if (t > t_max) t_max = t;

		if (options.verbosity >= 2) printf("board_score_1: %s %.1f clicks;\n", move_to_string(x, WHITE, m), t);

	}

	t_mean /= x;
	t_var = t_var / x - (t_mean * t_mean);
	
	printf("board_score_1:  %.2f < %.2f +/- %.2f < %.2f\n", t_min, t_mean, sqrt(t_var), t_max);
}

/*
 * @brief Mobility performance test.
 */
static void bench_mobility()
{
	const char *b = "OOOOOOOOOXXXXXXOOXXXXXXOOXXXXXXOOXXXXXXOOXXXXXXOOXXXXXXOOOOOOOOO O";
	char m[4];
	Board board;
	int i, x, v;
	const int N_WARMUP = 1000;
	const int N_REPEAT = 1000000;
	unsigned long long c, overhead;
	double t, t_mean, t_var, t_min, t_max;

	board_set(&board, b);

	x = A1;
	c = -click();
	for (i = 0; i < N_WARMUP; ++i) {
		board.player &= ~x_to_bit(x);
		board.opponent &= ~x_to_bit(x);
	}
	c += click();

	c = -click();
	for (i = 0; i < N_REPEAT; ++i) {
		board.player &= ~x_to_bit(x);
		board.opponent &= ~x_to_bit(x);
	}
	c += click();
	overhead = 0;

	t_mean = t_var = 0.0;
	t_max = 0;
	t_min = 1e30;

	for (x = A1; x < PASS; ++x) {
		board_set(&board, b);
		
		v = 0;
		c = -click();
		for (i = 0; i < N_WARMUP; ++i) {
			board.player &= ~x_to_bit(x);
			board.opponent &= ~x_to_bit(x);
			v += get_mobility(board.player, board.opponent);
			v -= get_mobility(board.opponent, board.player);
		}
		c += click();

		c = -click();
		for (i = 0; i < N_REPEAT; ++i) {
			board.player &= ~x_to_bit(x);
			board.opponent &= ~x_to_bit(x);
			v += get_mobility(board.player, board.opponent);
			v -= get_mobility(board.opponent, board.player);
		}
		c += click();

		t = ((double)(c - overhead)) / N_REPEAT / 2;
		t_mean += t;
		t_var += t * t;
		if (t < t_min) t_min = t;
		if (t > t_max) t_max = t;

		if (options.verbosity >= 2) printf("v = %d\n", v);
		if (options.verbosity >= 2) printf("mobility: %s %.1f clicks;\n", move_to_string(x, WHITE, m), t);
	}

	t_mean /= x;
	t_var = t_var / x - (t_mean * t_mean);
	
	printf("mobility:  %.2f < %.2f +/- %.2f < %.2f\n", t_min, t_mean, sqrt(t_var), t_max);
}

/*
 * @brief Stability performance test.
 */
static void bench_stability()
{
	const char *b = "OOOOOOOOOXXXXXXOOXXXXXXOOXXXXXXOOXXXXXXOOXXXXXXOOXXXXXXOOOOOOOOO O";
	char m[4];
	Board board;
	int i, x, v;
	const int N_WARMUP = 1000;
	const int N_REPEAT = 1000000;
	unsigned long long c, overhead;
	double t, t_mean, t_var, t_min, t_max;

	board_init(&board);
	
	x = A1;
	c = -click();
	for (i = 0; i < N_WARMUP; ++i) {
		board.player &= ~x_to_bit(x);
		board.opponent &= ~x_to_bit(x);
	}
	c += click();

	c = -click();
	for (i = 0; i < N_REPEAT; ++i) {
		board.player &= ~x_to_bit(x);
		board.opponent &= ~x_to_bit(x);
	}
	c += click();
	overhead = c;

	t_mean = t_var = 0.0;
	t_max = 0;
	t_min = 1e30;

	for (x = A1; x < PASS; ++x) {
		board_set(&board, b);
		
		v = 0;
		c = -click();
		for (i = 0; i < N_WARMUP; ++i) {
			board.player &= ~x_to_bit(x);
			board.opponent &= ~x_to_bit(x);
			v += get_stability(board.player, board.opponent);
		}
		c += click();

		c = -click();
		for (i = 0; i < N_REPEAT; ++i) {
			board.player &= ~x_to_bit(x);
			board.opponent &= ~x_to_bit(x);
			v += get_stability(board.player, board.opponent);
		}
		c += click();

		t = ((double)(c - overhead)) / N_REPEAT;
		t_mean += t;
		t_var += t * t;
		if (t < t_min) t_min = t;
		if (t > t_max) t_max = t;

		if (options.verbosity >= 2) printf("v = %d\n", v);
		if (options.verbosity >= 2) printf("stability: %s %.1f clicks;\n", move_to_string(x, WHITE, m), t);
	}

	t_mean /= x;
	t_var = t_var / x - (t_mean * t_mean);
	
	printf("stability:  %.2f < %.2f +/- %.2f < %.2f\n", t_min, t_mean, sqrt(t_var), t_max);
}

/**
 * @brief perform various performance tests.
 */
void bench(void)
{
	printf("The unit of the results is CPU cycles\n");
	bench_move_generator();
	bench_count_last_flip();
	bench_board_score_1();
	bench_mobility();
	bench_stability();
}



