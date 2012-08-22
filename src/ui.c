/**
 * @file ui.c
 *
 * @brief User interface.
 *
 * @date 1998 - 2012
 * @author Richard Delorme
 * @version 4.3
 */

#include "options.h"
#include "ui.h"
#include "util.h"
#include <assert.h>

void gtp_preprocess(char *line);

/**
 * @brief Switch between different User Interface
 *
 * @param ui User Interface.
 * @param ui_type A string describing the chosen user interface.
 * @return true if successful.
 */
bool ui_switch(UI *ui, const char *ui_type)
{
	if (strcmp(ui_type, "edax") == 0) {
		ui->type = UI_EDAX;
		ui->init = ui_init_edax;
		ui->free = ui_free_edax;
		ui->loop = ui_loop_edax;
		return true;
	} else if (strcmp(ui_type, "gtp") == 0) {
		ui->type = UI_GTP;
		ui->init = ui_init_gtp;
		ui->free = ui_free_gtp;
		ui->loop = ui_loop_gtp;
		return true;
	} else if (strcmp(ui_type, "nboard") == 0) {
		ui->type = UI_NBOARD;
		ui->init = ui_init_nboard;
		ui->free = ui_free_nboard;
		ui->loop = ui_loop_nboard;
		return true;
	} else if (strcmp(ui_type, "xboard") == 0) {
		ui->type = UI_XBOARD;
		ui->init = ui_init_xboard;
		ui->free = ui_free_xboard;
		ui->loop = ui_loop_xboard;
		return true;
	} else if (strcmp(ui_type, "ggs") == 0) {
		ui->type = UI_GGS;
		ui->init = ui_init_ggs;
		ui->free = ui_free_ggs;
		ui->loop = ui_loop_ggs;
		return true;
	} else if (strcmp(ui_type, "cassio") == 0) {
		ui->type = UI_CASSIO;
		return true;
	}

	return false;
}


/**
 * @brief Get an event
 *
 * Wait for an event from the standard input.
 *
 * @param ui User Interface.
 */
static void ui_read_input(UI *ui)
{
	char *buffer;
	char cmd[8];
	Event *event = ui->event;
	Play *play = ui->play;

	buffer = string_read_line(stdin);

	if (buffer == NULL) {
		if (ferror(stdin)) {
			if (errno == EINTR) clearerr(stdin);
			else fatal_error("stdin is broken\n");
		} else if (feof(stdin)) {
			buffer = string_duplicate("eof");
		}
		event->loop = false;
	} else {
		if (ui->type == UI_GTP) gtp_preprocess(buffer);
		parse_word(buffer, cmd, 5);
		string_to_lowercase(cmd);
		if (strcmp(cmd, "stop") == 0) {
			event_clear_messages(event);
			info("<stop>\n");
			play_stop(play);
			if (ui->type == UI_GGS) play_stop(play + 1);
		} else if (ui->type == UI_NBOARD && strcmp(cmd, "ping") == 0) {
			play_stop(play);
		} else if (ui->type == UI_XBOARD && strcmp(cmd, "?") == 0) {
			play_stop(play);
		} else {
			if (strcmp(cmd, "quit") == 0 || strcmp(cmd, "q") == 0) {
				event_clear_messages(event);
				play_stop(play);
				if (ui->type == UI_GGS) play_stop(play + 1);
				event->loop = false;
			}
		}
	}
	if (buffer) {
		lock(event);
			event_add_message(event, buffer);
			condition_signal(event);
		unlock(event);
	}
}


/**
 * @brief Read event loop.
 *
 * @param v Engine.
 */
static void* ui_read_input_loop(void *v)
{
	UI *ui = (UI*) v;

	while (ui->event->loop && !feof(stdin) && !ferror(stdin)) {
		ui_read_input(ui);
	}

	info("<exit ui_read_input>\n");

	return NULL;
}


/**
 * @brief Wait input.
 *
 * @param ui User interface.
 * @param cmd Command.
 * @param param Command's parameters.
 */
void ui_event_wait(UI *ui, char **cmd, char **param)
{
	event_wait(ui->event, cmd, param);
	if (options.echo && *cmd) printf(" %s %s\n", *cmd, *param ? *param : "");
}

/**
 * @brief Wait input.
 *
 * @param ui User interface.
 * @param cmd Command.
 * @param param Command's parameters.
 */
bool ui_event_peek(UI *ui, char **cmd, char **param)
{
	int n;
	char *message;

	if ((message = event_peek_message(ui->event)) != NULL) {

		free(*cmd); *cmd = NULL;
		free(*param); *param = NULL;

		n = strlen(message);
		*cmd = (char*) malloc(n + 1);
		*param = (char*) malloc(n + 1);
		parse_command(message, *cmd, *param, n);
		free(message);
		return true;
	} else {
		return false;
	}
}

/**
 * @brief ui_event_exist
 *
 * Locked version of event_exist.
 *
 * @param ui User interface.
 * @return true if an event is waiting.
 */
bool ui_event_exist(UI *ui)
{
	bool ok;

	spin_lock(ui->event);
	ok = event_exist(ui->event);
	spin_unlock(ui->event);	

	return ok;
}

/**
 * @brief Create a new Othello User Interface.
 *
 * Allocate event resources & launch user-input event thread.
 *
 */
void ui_event_init(UI *ui)
{
	event_init(ui->event);
	
	thread_create(&ui->event->thread, ui_read_input_loop, ui);
	thread_detach(ui->event->thread);
}

/**
 * @brief Free events.
 *
 * Allocate event resources & launch user-input event thread.
 *
 */
void ui_event_free(UI *ui)
{
	event_free(ui->event);
}
