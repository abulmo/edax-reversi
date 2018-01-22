/**
 * @file ui.h
 *
 * @brief User interface header.
 *
 * @date 1998 - 2017
 * @author Richard Delorme
 * @version 4.4
 */


#ifndef EDAX_UI_H
#define EDAX_UI_H

#include "board.h"
#include "book.h"
#include "event.h"
#include "search.h"
#include "move.h"
#include "play.h"

#ifdef DLL_BUILD
#define DLL_API __declspec(dllexport)
#else
#define DLL_API
#endif

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

// add for libedax by lavox. 2018/1/19
// api functions
DLL_API void libedax_initialize(int, char**);
DLL_API void libedax_terminate();
DLL_API void edax_init();
DLL_API void edax_new();
DLL_API void edax_load(const char*);
DLL_API void edax_save(const char*);
DLL_API void edax_undo();
DLL_API void edax_redo();
DLL_API void edax_mode(const int);
DLL_API void edax_setboard(const char*);
DLL_API void edax_setboard_from_obj(const Board*, const int);
DLL_API void edax_vmirror();
DLL_API void edax_hmirror();
DLL_API void edax_rotate(const int);
DLL_API void edax_symetry(const int);
DLL_API void edax_play(char*);
DLL_API void edax_force(char*);
DLL_API void edax_go();
DLL_API void edax_hint(const int, HintList*);
DLL_API void edax_hint_prepare();
DLL_API void edax_hint_next(Hint*);
DLL_API void edax_stop();
DLL_API int edax_move(const char*);
DLL_API const char* edax_opening();
DLL_API const char* edax_ouverture();
DLL_API void edax_book_store();
DLL_API void edax_book_on();
DLL_API void edax_book_off();
DLL_API void edax_book_randomness(const int);
DLL_API void edax_book_depth(const int);
DLL_API void edax_book_new(const int, const int);
DLL_API void edax_book_load(const char*);
DLL_API void edax_book_save(const char*);
DLL_API void edax_book_import(const char*);
DLL_API void edax_book_export(const char*);
DLL_API void edax_book_merge(const char*);
DLL_API void edax_book_fix();
DLL_API void edax_book_negamax();
DLL_API void edax_book_correct();
DLL_API void edax_book_prune();
DLL_API void edax_book_subtree();
DLL_API void edax_book_show(Position*);
DLL_API void edax_book_info(Book*);
DLL_API void edax_book_verbose(const int);
DLL_API void edax_book_add(const char*);
DLL_API void edax_book_check(const char*);
DLL_API void edax_book_extract(const char*);
DLL_API void edax_book_deviate(int, int);
DLL_API void edax_book_enhance(int, int);
DLL_API void edax_book_fill(int);
DLL_API void edax_book_play();
DLL_API void edax_book_deepen();
DLL_API void edax_book_feed_hash();
DLL_API void edax_base_problem(const char*, const int, const char*);
DLL_API void edax_base_tofen(const char*, const int, const char*);
DLL_API void edax_base_correct(const char*, const int);
DLL_API void edax_base_complete(const char*);
DLL_API void edax_base_convert(const char*, const char*);
DLL_API void edax_base_unique(const char*, const char*);
DLL_API void edax_set_option(const char*, const char*);

DLL_API char* edax_get_moves(char*);
DLL_API int edax_is_game_over();
DLL_API int edax_can_move();
DLL_API void edax_get_last_move(Move*);
DLL_API void edax_get_board(Board*);
DLL_API int edax_get_current_player();
DLL_API int edax_get_disc(const int);
DLL_API int edax_get_mobility_count(const int);

// internal functions
static void libedax_observer(Result*);
void ui_init_libedax(UI*);
void ui_free_libedax(UI*);
void auto_go();

void book_cmd_pre_process(UI*);
void book_cmd_post_process(UI*);

#endif

