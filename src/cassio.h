/**
 * @file cassio.h
 *
 * A simple protocol to communicate with Cassio.
 *
 * @date 1998 - 2012
 * @author Richard Delorme
 * @version 4.3
 */

#ifndef CASSIO_H
#define CASSIO_H

void* engine_init(void);
void engine_free(void*);
double engine_midgame_search(void*, const char*, const double, const double, const int, const int);
int engine_endgame_search(void*, const char*, const int, const int, const int);
void engine_stop(void*);
void engine_print_results(void*, char*);
void engine_empty_hash(void*);

void engine_loop(void);

#endif /* CASSIO_H */

