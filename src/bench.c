/**
 * @file bench.c
 *
 * @date 1998 - 2023
 * @author Richard Delorme
 * @version 4.5
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
static unsigned long long click(void)
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
#elif defined(_WIN32) && (defined(__i386__) || defined(__x86_64__) || defined(_M_IX86) || defined(_M_X64))
	return __rdtsc();
#else
	return cpu_clock();
#endif
}

/*
 * @brief Move generator performance test.
 */
static void bench_move_generator(void)
{
	const char *b = "OOOOOOOOOXXXXXXOOXXXXXXOOXXXXXXOOXXXXXXOOXXXXXXOOXXXXXXOOOOOOOOO O";
	char m[4];
	Board board;
	Move move;
	int i, x;
	volatile int v;
	const int N_WARMUP = 1000;
	const int N_REPEAT = 1000000;
	unsigned long long c, overhead;
	double t, t_mean, t_var, t_min, t_max;

	v = 0;
	c = -click();
	for (i = 0; i < N_WARMUP; ++i) {
		v += i;
	}
	c += click();

	c = -click();
	for (i = 0; i < N_REPEAT; ++i) {
		v += i;
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
			v += board_get_move_flip(&board, x, &move);
		}
		c += click();

		c = -click();
		for (i = 0; i < N_REPEAT; ++i) {
			v += board_get_move_flip(&board, x, &move);
		}
		c += click();

		t = ((double)(c - overhead)) / N_REPEAT;
		t_mean += t;
		t_var += t * t;
		if (t < t_min) t_min = t;
		if (t > t_max) t_max = t;

		if (options.verbosity >= 2) printf("board_get_move_flip: %s %.1f clicks;\n", move_to_string(x, WHITE, m), t);

	}

	t_mean /= x;
	t_var = t_var / x - (t_mean * t_mean);

	printf("board_get_move_flip:  %.2f < %.2f +/- %.2f < %.2f\n", t_min, t_mean, sqrt(t_var), t_max);
}

/*
 * @brief Last Move performance test.
 */
static void bench_count_last_flip(void)
{
	const char *b = "OOOOOOOOOXXXXXXOOXXXXXXOOXXXXXXOOXXXXXXOOXXXXXXOOXXXXXXOOOOOOOOO O";
	char m[4];
	Board board;
	int i, x;
	volatile int v;
	const int N_WARMUP = 1000;
	const int N_REPEAT = 1000000;
	unsigned long long c, overhead;
	double t, t_mean, t_var, t_min, t_max;

	v = 0;

	c = -click();
	for (i = 0; i < N_WARMUP; ++i) {
		v += i;
	}
	c += click();

	c = -click();
	for (i = 0; i < N_REPEAT; ++i) {
		v += i;
	}
	c += click();
	overhead = c;

	t_mean = t_var = 0.0;
	t_max = 0;
	t_min = 1e30;

	for (x = A1; x < PASS; ++x) {
		board_set(&board, b);
		board.player &= ~x_to_bit(x);
		// board.opponent &= ~x_to_bit(x);

		c = -click();
		for (i = 0; i < N_WARMUP; ++i) {
			v += last_flip(x, board.player & ~i);
		}
		c += click();

		c = -click();
		for (i = 0; i < N_REPEAT; ++i) {
			v += last_flip(x, board.player& ~i);
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
static void bench_board_score_1(void)
{
	const char *b = "OOOOOOOOOXXXXXXOOXXXXXXOOXXXXXXOOXXXXXXOOXXXXXXOOXXXXXXOOOOOOOOO O";
	char m[4];
	Board board;
	int i, x;
	volatile int v;
	const int N_WARMUP = 1000;
	const int N_REPEAT = 1000000;
	unsigned long long c, overhead;
	double t, t_mean, t_var, t_min, t_max;

	board_set(&board, b);
	v = 0;

	c = -click();
	for (i = 0; i < N_WARMUP; ++i) {
		v += i;
	}
	c += click();

	c = -click();
	for (i = 0; i < N_REPEAT; ++i) {
		v += i;
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
			v += board_score_1(board.player, SCORE_MAX - 1, x);
		}
		c += click();

		c = -click();
		for (i = 0; i < N_REPEAT; ++i) {
			v += board_score_1(board.player, SCORE_MAX - 1, x);
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
static void bench_mobility(void)
{
	const char *b = "OOOOOOOOOXXXXXXOOXXXXXXOOXXXXXXOOXXXXXXOOXXXXXXOOXXXXXXOOOOOOOOO O";
	char m[4];
	Board board;
	int i, x;
	volatile int v;
	const int N_WARMUP = 1000;
	const int N_REPEAT = 1000000;
	unsigned long long c, overhead;
	double t, t_mean, t_var, t_min, t_max;

	board_set(&board, b);
	v = 0;
	c = -click();
	for (i = 0; i < N_WARMUP; ++i) {
		board.player &= ~i;
		board.opponent &= ~i;
		v += i;
	}
	c += click();

	board_set(&board, b);
	c = -click();
	for (i = 0; i < N_REPEAT; ++i) {
		board.player &= ~i;
		board.opponent &= ~i;
		v += i;
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
			board.player &= ~i;
			board.opponent &= ~i;
			v += get_mobility(board.player, board.opponent);
			v -= get_mobility(board.opponent, board.player);
		}
		c += click();

		board_set(&board, b);
		c = -click();
		for (i = 0; i < N_REPEAT; ++i) {
			board.player &= ~i;
			board.opponent &= ~i;
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
static void bench_stability(void)
{
	const char *b = "OOOOOOOOOXXXXXXOOXXXXXXOOXXXXXXOOXXXXXXOOXXXXXXOOXXXXXXOOOOOOOOO O";
	char m[4];
	Board board;
	int i, x;
	volatile int v;
	const int N_WARMUP = 1000;
	const int N_REPEAT = 1000000;
	unsigned long long c, overhead;
	double t, t_mean, t_var, t_min, t_max;

	board_init(&board);

	v = 0;
	x = A1;
	c = -click();
	for (i = 0; i < N_WARMUP; ++i) {
		board.player &= ~x_to_bit(x);
		board.opponent &= ~x_to_bit(x);
	}
	c += click();

	board_set(&board, b);
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

		board_set(&board, b);
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



