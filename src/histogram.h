/**
 * @file histogram.h
 *
 * Histogram management.
 *
 * @date 1998 - 2024
 * @author Richard Delorme
 * @version 4.6
 */

#ifndef EDAX_HISTOGRAM_H
#define EDAX_HISTOGRAM_H

/* declaration */
void histogram_init(uint64_t h[129][65]);
void histogram_print(uint64_t h[129][65]);
void histogram_stats(uint64_t h[129][65]);
void histogram_to_ppm(const char *file, uint64_t histogram[129][65]);

#endif

