/**
 * @file hash.c
 *
 * Hash table's with a lock table or lock free ?
 *
 * @date 1998 - 2012
 * @author Richard Delorme
 * @version 4.3
 */

#include "settings.h"

#if USE_LOCK_HASH
	#include "hash-lock.c"
#else
	#include "hash-lock-free.c"
#endif

