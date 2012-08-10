/**
 * @file opening.h
 *
 * Opening Name aliasing.
 *
 * @date 1998 - 2012
 * @author Richard Delorme
 * @version 4.3
 */

#ifndef EDAX_OPENING_NAME
#define EDAX_OPENING_NAME

struct Board;

const char *opening_get_line(const char*);
const char *opening_get_french_name(const struct Board*);
const char *opening_get_english_name(const struct Board*);

#endif

