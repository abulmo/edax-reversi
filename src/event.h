/**
 * @file event.h
 *
 * Event management.
 *
  * @date 1998 - 2024
 * @author Richard Delorme
 * @version 4.6
 */

#ifndef EDAX_EVENT_H
#define EDAX_EVENT_H

#include <stdbool.h>
#include "util.h"

/** Event management data */
typedef struct Event {
	bool loop;                  /*!< flag allowing to loop, waiting for events */
	char **ring;                /*!< ring of buffers */
	int size;                   /*!< size of the buffer ring */
	int first;                  /*!< first position in the ring */
	int end;                    /*!< one past the last position in the ring */
	thrd_t thread;              /*!< thread */
	mtx_t mutex;                /*!< mutex */
	mtx_t cond_mutex;           /*!< conditional mutex */
	cnd_t condition;            /*!< condition */
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

