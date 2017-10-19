/**
 * @file stdbool.h
 *
 * @brief Replacement include file for deficient C compiler.
 *
 * @date 1998 - 2017
 * @author Richard Delorme
 * @version 4.4
 */

#ifndef EDAX_STDBOOL_H
#define EDAX_STDBOOL_H

typedef enum {
	false = 0,
	true = 1
} bool;

#endif // EDAX_STDBOOL_H
