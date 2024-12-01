/**
 * @file empty.h
 *
 * List of empty squares.
 *
 * @date 1998 - 2024
 * @author Richard Delorme
 * @version 4.6
 */

#ifndef EDAX_EMPTY_H
#define EDAX_EMPTY_H

/** double linked list of squares */
typedef struct SquareList {
	int previous;   /*!< link to previous square */
	int next;       /*!< link to next square */
} SquareList;

/**
 * @brief remove an empty square from the list.
 *
 * @param empty  empty square.
 */
static inline void empty_remove(SquareList *empty, int x)
{
	int next = empty[x].next;
	int prev = empty[x].previous;
	empty[prev].next = next;
	empty[next].previous = prev;
}

/**
 * @brief restore the list of empty squares
 *
 * @param empty  empty square.
 */
static inline void empty_restore(SquareList *empty, int x)
{
	empty[empty[x].previous].next = x;
	empty[empty[x].next].previous = x;
}

/** Loop over all empty squares */
#define foreach_empty(x, empty)\
	for ((x) = (empty)[NOMOVE].next; (x) != NOMOVE; (x) = (empty)[(x)].next)

/** Loop over all empty squares on even quadrants */
#define foreach_even_empty(x, empty, parity)\
	for ((x) = (empty)[NOMOVE].next; (x) != NOMOVE; (x) = (empty)[(x)].next) if (((parity) & QUADRANT_ID[(x)]) == 0)

/** Loop over all empty squares on odd quadrants */
#define foreach_odd_empty(x, empty, parity)\
	for ((x) = (empty)[NOMOVE].next; (x) != NOMOVE; (x) = (empty)[(x)].next) if (((parity) & QUADRANT_ID[(x)]))

#endif

