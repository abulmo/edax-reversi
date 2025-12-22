/**
 * @file histogram.h
 *
 * Histogram management.
 *
 * @date 1998 - 2025
 * @author Richard Delorme
 * @version 4.7
 */

#ifndef EDAX_HISTOGRAM_H
#define EDAX_HISTOGRAM_H

#include <stdint.h>

/* declaration */
void histogram_init(uint64_t h[129][65]);
void histogram_print(uint64_t h[129][65]);
void histogram_stats(uint64_t h[129][65]);
void histogram_to_ppm(const char *file, uint64_t histogram[129][65]);

#endif
