/* edit
 * User prompt
 */

#include "line.h"
#ifdef USE_PDCURSES
#include <curses.h>
#else
#include <ncurses.h>
#endif

#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "global.h"

#include "prompt.h"

static void _init_prompt(Prompt *prompt, const char *fmt, va_list args);

static void _prompt_center_msg(Prompt *prompt, const char *msg);

static PromptOptResult _yes_no(void);
static PromptOptResult _yes_no_cancel(void);

/* Initializes a prompt */
void prompt_init(Prompt *prompt, PromptType type, const char *msg, ...) {
	va_list args;
	va_start(args, msg);

	_init_prompt(prompt, msg, args);

	prompt->type = type;
	switch( type ) {
	case PROMPT_YES_NO:
		_prompt_center_msg(prompt, "(Y)es / (N)o");
		break;
	case PROMPT_YES_NO_CANCEL:
		_prompt_center_msg(prompt, "(Y)es / (N)o / (C)ancel");
		break;
	case PROMPT_STR:
		break;
	}
}

/* Gets an option prompt */
PromptOptResult prompt_opt_get(Prompt *prompt) {
	switch( prompt->type ) {
	case PROMPT_YES_NO:
		return _yes_no();
	case PROMPT_YES_NO_CANCEL:
		return _yes_no_cancel();
	case PROMPT_STR:
		fprintf(stderr, "Use prompt_str_get instead\n");
		exit(1);
	default:
		fprintf(stderr, "Unknown prompt type '%d'!\n", prompt->type);
		exit(1);
	}
}

/* Gets a string prompt */
char *prompt_str_get(Prompt *prompt) {
	Line line;
	line_new(&line);

	int ch;
	while( (ch = getch()) != '\n' ) {
		if( ch == KEY_BACKSPACE ) {
			line_delete_char_at_end(&line);
		} else if( ch >= 32 && ch <= 126 ) {
			line_insert_char_at_end(&line, ch);
		}

		mvwaddnstr(prompt->win, 3, 1, line.text, line.length);
		wrefresh(prompt->win);
	}

	char *c_str = line_get_c_str(&line, true);
	line_free(&line);

	return c_str;
}

/* Deletes a prompt */
void prompt_free(Prompt *prompt) {
	wborder(prompt->win, ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ');
	werase(prompt->win);
	wrefresh(prompt->win);
	delwin(prompt->win);
	prompt->win = NULL;
}

#define MAX_PROMPT_LEN (64)

/* Initializes a prompt */
static void _init_prompt(Prompt *prompt, const char *fmt, va_list args) {
	char msg[MAX_PROMPT_LEN];
	vsnprintf(msg, MAX_PROMPT_LEN, fmt, args);

	int base_width = COLS / 4;
	int msg_len = strlen(msg);
	int min_width = msg_len + 2;

	int w = MAX(base_width, min_width);
	int h = 5;

	WINDOW *win = subwin(stdscr, h, w, LINES - h * 2, COLS - w);
	werase(win);
	mvwaddnstr(win, 1, (w - msg_len) / 2, msg, msg_len);
	box(win, 0, 0);
	wrefresh(win);

	prompt->win = win;
	prompt->w = w;
	prompt->h = h;
}

/* Centers a message inside a prompt */
static void _prompt_center_msg(Prompt *prompt, const char *msg) {
	int msg_len = strlen(msg);

	mvwaddnstr(prompt->win, 3, (prompt->w - msg_len) / 2, msg, msg_len);
	wrefresh(prompt->win);
}

/* Gets a Y/N response */
static PromptOptResult _yes_no(void) {
	while( true ) {
		switch( getch() ) {
		case 'Y':
		case 'y':
			return PROMPT_YES;
		case 'N':
		case 'n':
			return PROMPT_NO;
		}
	}
}

/* Gets a Y/N/C response */
static PromptOptResult _yes_no_cancel(void) {
	while( true ) {
		switch( getch() ) {
		case 'Y':
		case 'y':
			return PROMPT_YES;
		case 'N':
		case 'n':
			return PROMPT_NO;
		case 'C':
		case 'c':
			return PROMPT_CANCEL;
		}
	}
}
