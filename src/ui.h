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

/**
 * Bench result structure
 */
typedef struct BenchResult {
    unsigned long long T;
    unsigned long long n_nodes;
    int positions;
    Lock lock;
} BenchResult;

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
DLL_API void libedax_terminate(void);
DLL_API void edax_init(void);
DLL_API void edax_new(void);
DLL_API void edax_load(const char*);
DLL_API void edax_save(const char*);
DLL_API void edax_undo(void);
DLL_API void edax_redo(void);
DLL_API void edax_mode(const int);
DLL_API void edax_setboard(const char*);
DLL_API void edax_setboard_from_obj(const Board*, const int);
DLL_API void edax_vmirror(void);
DLL_API void edax_hmirror(void);
DLL_API void edax_rotate(const int);
DLL_API void edax_symetry(const int);
DLL_API void edax_play(char*);
DLL_API void edax_force(char*);
DLL_API void edax_bench(BenchResult*, int);
DLL_API void edax_bench_get_result(BenchResult*);
DLL_API void edax_go(void);
DLL_API void edax_hint(const int, HintList*);
DLL_API void edax_get_bookmove(MoveList*);
DLL_API void edax_get_bookmove_with_position(MoveList*, Position*);
DLL_API void edax_hint_prepare(MoveList*);
DLL_API void edax_hint_next(Hint*);
DLL_API void edax_hint_next_no_multipv_depth(Hint*);
DLL_API void edax_stop(void);
DLL_API void edax_version(void);
DLL_API int edax_move(const char*);
DLL_API const char* edax_opening(void);
DLL_API const char* edax_ouverture(void);
DLL_API void edax_book_store(void);
DLL_API void edax_book_on(void);
DLL_API void edax_book_off(void);
DLL_API void edax_book_randomness(const int);
DLL_API void edax_book_depth(const int);
DLL_API void edax_book_new(const int, const int);
DLL_API void edax_book_load(const char*);
DLL_API void edax_book_save(const char*);
DLL_API void edax_book_import(const char*);
DLL_API void edax_book_export(const char*);
DLL_API void edax_book_merge(const char*);
DLL_API void edax_book_fix(void);
DLL_API void edax_book_negamax(void);
DLL_API void edax_book_correct(void);
DLL_API void edax_book_prune(void);
DLL_API void edax_book_subtree(void);
DLL_API void edax_book_show(Position*);
DLL_API void edax_book_info(Book*);
DLL_API void edax_book_verbose(const int);
DLL_API void edax_book_add(const char*);
DLL_API void edax_book_check(const char*);
DLL_API void edax_book_extract(const char*);
DLL_API void edax_book_deviate(int, int);
DLL_API void edax_book_enhance(int, int);
DLL_API void edax_book_fill(int);
DLL_API void edax_book_play(void);
DLL_API void edax_book_deepen(void);
DLL_API void edax_book_feed_hash(void);
DLL_API void edax_book_add_board_pre_process(void);
DLL_API void edax_book_add_board_post_process(void);
DLL_API void edax_book_add_board(const Board*);

DLL_API void edax_base_problem(const char*, const int, const char*);
DLL_API void edax_base_tofen(const char*, const int, const char*);
DLL_API void edax_base_correct(const char*, const int);
DLL_API void edax_base_complete(const char*);
DLL_API void edax_base_convert(const char*, const char*);
DLL_API void edax_base_unique(const char*, const char*);
DLL_API void edax_set_option(const char*, const char*);

DLL_API char* edax_get_moves(char*);
DLL_API int edax_is_game_over(void);
DLL_API int edax_can_move(void);
DLL_API void edax_get_last_move(Move*);
DLL_API void edax_get_board(Board*);
DLL_API int edax_get_current_player(void);
DLL_API int edax_get_disc(const int);
DLL_API int edax_get_mobility_count(const int);

// internal functions
static void libedax_observer(Result*);
void ui_init_libedax(UI*);
void ui_free_libedax(UI*);
void auto_go(void);

void book_cmd_pre_process(UI*);
void book_cmd_post_process(UI*);

#endif

