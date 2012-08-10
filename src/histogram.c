/**
 * @file histogram.c
 *
 * Hash table's header.
 *
 * @date 1998 - 2012
 * @author Richard Delorme
 * @version 4.3
 */

#include "util.h"

#include <stdio.h>
#include <math.h>
#include <stdlib.h>

#ifndef RCD
#define RCD 0.5
#endif

void histogram_init(unsigned long long h[129][65])
{
	int i, j;

	for (i = 0; i < 129; ++i)
	for (j = 0; j < 65; ++j)
		h[i][j] = 0;
}

void histogram_print(unsigned long long h[129][65]) 
{
	int i, j;
	unsigned long long n, N;
	double x, y, s, s2;
	double mean[65], variance[65], median[65];

	for (j = 0; j < 65; ++j) printf("; %d", j * 2 - 64);
	printf("; total; mean; error; bias; precision");  
	for (i = 0; i < 129; ++i) {
		printf("\n%d", i - 64);
		n = s = s2 = 0;
			x = (i - 64);
		for (j = 0; j < 65; ++j) {
			y = 2 * j - 64;
			printf("; %lld", h[i][j]);
			n += h[i][j];
			s += y * h[i][j];
			s2 += (x - y) * (x - y) * h[i][j];
		}
		s /= n;
		s2 /= (n - 1);
		if (n > 5) printf("; %lld; %.2f; %.2f; %.2f; %.2f", n, s, sqrt(s2), x - s, sqrt(s2 - (x - s) * (x - s)));
		else if (n > 0) printf("; %lld; %.2f; %.2f", n, s, sqrt(s2));
		else printf("; %lld", n);
	}

	printf("\n");
	N = 0;
	printf("total");
	for (j = 0; j < 65; ++j) {
		n = s = s2 = 0;
		y = 2 * j - 64;
		for (i = 0; i < 129; ++i) {
			x = (i - 64);
			n += h[i][j];
			s += x * h[i][j];
			s2 += (x - y) * (x - y) * h[i][j];
		}
		N += n;
		mean[j] = s / n;
		variance[j] = s2 / (n - 1) ;
		s = 0;
		if ((n & 1) == 1) {
			for (i = 0; i < 129; ++i) {
				s += h[i][j];
				if (s >= (n + 1) / 2) {
					median[j] = i - 64;
					break;
				}
			}
		} else {
			for (i = 0; i < 129; ++i) {
				s += h[i][j];
				if (s == n / 2) {
					median[j] = i - 63.5;
					break;
				} else if (s > n / 2) {
					median[j] = i - 64;
					break;
				}
			}
		}
		printf("; %lld", n);
	}
	printf("; %lld\n", N);
	printf("mean"); for (j = 0; j < 65; ++j) printf("; %.2f", mean[j]); printf("\n");
	printf("median"); for (j = 0; j < 65; ++j) printf("; %.2f", median[j]); printf("\n");
	printf("error"); for (j = 0; j < 65; ++j) printf("; %.2f", sqrt(variance[j])); printf("\n");
	printf("bias"); for (j = 0; j < 65; ++j) printf("; %.2f", (2*j - 64) - mean[j]); printf("\n");
	printf("precision"); for (j = 0; j < 65; ++j) {x = (2*j - 64) - mean[j]; printf("; %.2f", sqrt(variance[j] - x * x));} printf("\n");
}

void histogram_stats(unsigned long long h[129][65])
{
	double y[65], x[129];
	double m_x, m_y, s_x, s_y, s_xy;
	double a, b, r;
	double n;
	int i, j;

	for (j = 0; j < 65; ++j) y[j] = j * 2 - 64;
	for (j = 0; j < 129; ++j) x[j] = j - 64;

	m_x = m_y = s_x = s_y = s_xy = 0.0;
	n = 0.0;

	for (i = 0; i < 129; ++i)
	for (j = 0; j < 65; ++j) {
		n += h[i][j];
		m_x += x[i] * h[i][j];
		m_y += y[j] * h[i][j];
		s_x += x[i] * x[i] * h[i][j];
		s_y += y[j] * y[j] * h[i][j];
		s_xy += x[i] * y[j] * h[i][j];
	}

	m_x /= n;
	m_y /= n;

	s_x = sqrt(s_x / n - m_x * m_x);
	s_y = sqrt(s_y / n - m_y * m_y);
	s_xy = s_xy / n - m_x * m_y;

	r = s_xy / (s_x * s_y);


	printf("statistics summary\n");
	printf("n  = %.0f\n", n);
	printf("m_eval  = %.6f; m_score = %.6f\n", m_x, m_y);
	printf("s_eval = %.6f; s_score = %.6f; cov = %.6f\n", s_x, s_y, s_xy);
	a = sqrt(s_xy) / s_x; // regression
	b = m_y - a * m_x;
	printf("score = %.6f * eval + %.6f; r = %.6f; r2 = %.6f (regression)\n", a, b, r, r*r);
	a = s_y / s_x; // correlation
	b = m_y - a * m_x;
	printf("score = %.6f * eval + %.6f;(correlation)\n", a, b);
	a = sqrt(s_xy) / s_y; // correlation
	b = m_x - a * m_y;
	printf("eval = %.6f * score + %.6f;(regression)\n", a, b);
	a = s_x / s_y; // correlation
	b = m_x - a * m_y;
	printf("eval = %.6f * score + %.6f;(correlation)\n", a, b);
	printf("\n");
}

void histogram_to_ppm(const char *file, unsigned long long histogram[129][65])
{
	FILE *f;
	int i, j, i_k, j_k, v;
	unsigned long long max;
	double w;
	unsigned char r[256], g[256], b[256];

	// open the file
	f = fopen(file, "w");
	if (f == NULL) {
		warn("cannot open ppm file %s\n", file);
		return;
	}

	// compute max_value
	max = 0; 
	for (i = 0; i < 129; ++i)
	for (j = 0; j < 65; ++j) {
		if (histogram[i][j] > max) max = histogram[i][j];
	}
	w = 255.0/max;

	// palette
	r[0] = g[0] = b[0] = 255;
	for (i = 1; i < 64; i++) { r[i +   0] = 255 - i * 4; g[i +   0] = 0;           b[i +   0] = 255;}
	for (i = 0; i < 64; i++) { r[i +  64] = 0;           g[i +  64] = i * 4;       b[i +  64] = 255 - i * 4;}
	for (i = 0; i < 64; i++) { r[i + 128] = i * 4;       g[i + 128] = 255;         b[i + 128] = 0;}
	for (i = 0; i < 64; i++) { r[i + 192] = 255;         g[i + 192] = 255 - i * 4; b[i + 192] = 0;}

	// header;
	fprintf(f, "P3\n%d %d\n%d\n", 516, 520, 255);

	// data		
	for (i = 64; i >= 0; --i) {
		for (i_k = 0; i_k < 8; ++i_k) {
			for (j = 0; j < 129; ++j) {
				v = floor(histogram[j][i] * w + RCD); if (v == 0 && histogram[j][i] > 0) v = 1;
				for (j_k = 0; j_k < 4; ++j_k) fprintf(f, "%d %d %d  ", r[v], g[v], b[v]);
			}
			fputc('\n', f);
		}
	}

	// end
	fclose(f);
}

