/**
 * @file obftest.h
 *
 * @brief Problem solver.
 *
 * @date 1998 - 2024
 * @author Richard Delorme
 * @version 4.6
 */

#ifndef EDAX_OPDTEST_H
#define EDAX_OPDTEST_H


struct Search;

void obf_test(struct Search*, const char*, const char*);
void script_to_obf(struct Search*, const char*, const char*);
void obf_filter(const char*, const char *);
void obf_speed(struct Search*, const int);

#endif /* EDAX_OPDTEST_H */

