/**
 * @file histogram.h
 *
 * Histogram management.
 *
 * @date 1998 - 2012
 * @author Richard Delorme
 * @version 4.3
 */

#ifndef EDAX_HISTOGRAM_H
#define EDAX_HISTOGRAM_H

/* declaration */
void histogram_init(unsigned long long h[129][65]);
void histogram_print(unsigned long long h[129][65]);
void histogram_stats(unsigned long long h[129][65]);
void histogram_to_ppm(const char *file, unsigned long long histogram[129][65]);

#endif

