/**
 * @file bit.h
 *
 * Bitwise operations header file.
 *
 * @date 1998 - 2012
 * @author Richard Delorme
 * @version 4.3
 */

#ifndef EDAX_BIT_H
#define EDAX_BIT_H

#include <stdio.h>

struct Random;

/* declaration */
int bit_count(unsigned long long);
int bit_weighted_count(const unsigned long long);
int first_bit(unsigned long long);
int next_bit(unsigned long long*);
int last_bit(unsigned long long);
void bitboard_write(const unsigned long long, FILE*);
unsigned long long transpose(unsigned long long);
unsigned long long vertical_mirror(unsigned long long);
unsigned long long horizontal_mirror(unsigned long long);
unsigned int bswap_int(unsigned int);
unsigned short bswap_short(unsigned short);
int get_rand_bit(unsigned long long, struct Random*);

/** Loop over each bit set. */
#define foreach_bit(i, b) for (i = first_bit(b); b; i = next_bit(&b))

extern const unsigned long long X_TO_BIT[];
/** Return a bitboard with bit x set. */
#define x_to_bit(x) X_TO_BIT[x]

//#define x_to_bit(x) (1ULL << (x)) // 1% slower on Sandy Bridge

#endif

