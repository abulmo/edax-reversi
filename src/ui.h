/**
 * @file ui.h
 *
 * @brief User interface header.
 *
 * @date 1998 - 2012
 * @author Richard Delorme
 * @version 4.3
 */


#ifndef EDAX_UI_H
#define EDAX_UI_H

#include "board.h"
#include "book.h"
#include "event.h"
#include "search.h"
#include "move.h"
#include "play.h"

/**
 * UI structure
 */
typedef struct UI {
	Play play[2];              /**< Play Control */
	Book book[1];              /**< Opening book */
	struct GGSClient *ggs;     /**< GGS Client */
	bool is_same_play;         /**< true if play[0] == play[1] */
	int type;                  /**< type of UI */
	int mode;                  /**< computer's color mode TODO: remove me*/
	Event event[1];            /**< event */

	void (*init)(struct UI*);  /**< init function */
	void (*loop)(struct UI*);  /**< main loop function */
	void (*free)(struct UI*);  /**< free resources function */
} UI;

bool ui_switch(UI*, const char*);

void ui_event_init(UI*);
bool ui_event_peek(UI*, char**, char**);
void ui_event_wait(UI*, char**, char**);
bool ui_event_exist(UI*);
void ui_event_free(UI*);

void ui_init_edax(UI*);
void ui_loop_edax(UI*);
void ui_free_edax(UI*);

void ui_init_gtp(UI*);
void ui_loop_gtp(UI*);
void ui_free_gtp(UI*);

void ui_init_nboard(UI*);
void ui_loop_nboard(UI*);
void ui_free_nboard(UI*);

void ui_init_xboard(UI*);
void ui_loop_xboard(UI*);
void ui_free_xboard(UI*);

void ui_init_ggs(UI*);
void ui_loop_ggs(UI*);
void ui_free_ggs(UI*);

void ui_init_cassio(UI*);
void ui_loop_cassio(UI*);
void ui_free_cassio(UI*);

void ui_book_init(UI*);

#endif

