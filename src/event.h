/**
 * @file event.h
 *
 * Event management.
 *
 * @date 1998 - 2012
 * @author Richard Delorme
 * @version 4.3
 */

#ifndef EDAX_EVENT_H
#define EDAX_EVENT_H

#include <stdbool.h>
#include "util.h"

/** Event management data */
typedef struct Event {
	volatile bool loop;         /*!< flag allowing to loop, waiting for events */
	char **ring;                /*!< ring of buffers */
	int size;                   /*!< size of the buffer ring */
	int first;                  /*!< first position in the ring */
	int end;                    /*!< one past the last position in the ring */
	Thread thread;              /*!< thread */
	SpinLock spin;              /*!< spin lock */
	Lock lock;                  /*!< lock */
	Condition cond;             /*!< condition */
} Event;                        /*!< messages */

void event_init(Event*);
void event_free(Event*);
void event_clear_messages(Event*);
void event_add_message(Event*, char*);
char *event_peek_message(Event*);
bool event_exist(Event*);
void event_wait(Event*, char**, char**);
void event_wait_enter(Event*);

#endif /* EDAX_EVENT_H */

