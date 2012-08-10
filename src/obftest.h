/**
 * @file obftest.h
 *
 * @brief Problem solver.
 *
 * @date 1998 - 2012
 * @author Richard Delorme
 * @version 4.3
 */

#ifndef EDAX_OPDTEST_H
#define EDAX_OPDTEST_H


struct Search;

void obf_test(struct Search*, const char*, const char*);
void script_to_obf(struct Search*, const char*, const char*);
void obf_filter(const char*, const char *);

#endif /* EDAX_OPDTEST_H */

