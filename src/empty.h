/**
 * @file empty.h
 *
 * List of empty squares.
 *
 * @date 1998 - 2012
 * @author Richard Delorme
 * @version 4.3
 */

#ifndef EDAX_EMPTY_H
#define EDAX_EMPTY_H

/** double linked list of squares */
typedef struct SquareList {
	unsigned long long b;         /*!< bit representation of the square location */
	int x;                        /*!< square location */
	int quadrant;                 /*!< parity quadrant */
	struct SquareList *previous;  /*!< link to previous square */
	struct SquareList *next;      /*!< link to next square */
} SquareList;

/**
 * @brief remove an empty square from the list.
 *
 * @param empty  empty square.
 */
static inline void empty_remove(SquareList *empty)
{
	empty->previous->next = empty->next;
	empty->next->previous = empty->previous;
}

/**
 * @brief restore the list of empty squares
 *
 * @param empty  empty square.
 */
static inline void empty_restore(SquareList *empty)
{
	empty->previous->next = empty;
	empty->next->previous = empty;
}

/** Loop over all empty squares */
#define foreach_empty(empty, list)\
	for ((empty) = (list)->next; (empty)->next; (empty) = (empty)->next)

/** Loop over all empty squares on even quadrants */
#define foreach_even_empty(empty, list, parity)\
	for ((empty) = (list)->next; (empty)->next; (empty) = (empty)->next) if ((parity & empty->quadrant) == 0)

/** Loop over all empty squares on odd quadrants */
#define foreach_odd_empty(empty, list, parity)\
	for ((empty) = (list)->next; (empty)->next; (empty) = (empty)->next) if (parity & empty->quadrant)

#endif

