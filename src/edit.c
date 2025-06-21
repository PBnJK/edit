/* edit
 * The "edit" editor
 */

#include <stdarg.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <ncurses.h>

#include "global.h"

#include "file.h"
#include "line.h"

#include "edit.h"

static void _exit_command_typing(Edit *edit);
static void _render_command(Edit *edit);
static void _clear_command(Edit *edit);
static void _handle_command(Edit *edit);

static void _update_gutter(Edit *edit);
static void _update_cursor_x(Edit *edit);

static void _move_to_start_of_line(Edit *edit);
static void _move_to_end_of_line(Edit *edit);
static void _move_to_end_of_file(Edit *edit);

static void _newline(Edit *edit);

static char *_get_mode_string(Edit *edit);

/* Initializes the editor */
void edit_new(Edit *edit, const char *filename) {
	edit->line = 0;
	edit->idx = 0;

	edit->x = 0;
	edit->y = 0;

	edit->vx = 0;
	edit->vy = 0;

	getmaxyx(stdscr, edit->h, edit->w);

	memset(edit->msg, 0, STATUS_MSG_LEN);
	edit->msg_len = 0;

	edit->running = true;

	edit->mode = EDIT_MODE_NORMAL;

	line_new(&edit->cmd);

	file_new(&edit->file, filename);
	_update_gutter(edit);

	edit->x = edit->gutter;

	edit_render(edit);
	edit_render_status(edit);
}

/* Frees the editor from memory */
void edit_free(Edit *edit) {
	file_free(&edit->file);
}

/* Updates the editor */
void edit_update(Edit *edit) {
	int ch = getch();
	switch( edit->mode ) {
	case EDIT_MODE_NORMAL:
		edit_mode_normal(edit, ch);
		break;
	case EDIT_MODE_INSERT:
		edit_mode_insert(edit, ch);
		break;
	case EDIT_MODE_VISUAL:
		edit_mode_visual(edit, ch);
		break;
	case EDIT_MODE_COMMAND:
		edit_mode_command(edit, ch);
	}

	edit_render_status(edit);
}

/* Handles the NORMAL mode
 *
 * In this mode, you cannot type characters directly, but can instead use
 * shortcuts to perform certain actions
 */
void edit_mode_normal(Edit *edit, int ch) {
	switch( ch ) {
	case ':':
		edit->mode = EDIT_MODE_COMMAND;
		_render_command(edit);
		break;
	case 'i':
		edit->mode = EDIT_MODE_INSERT;
		break;
	case '^':
		_move_to_start_of_line(edit);
		break;
	case '$':
		_move_to_end_of_line(edit);
		break;
	case 'G':
		_move_to_end_of_file(edit);
		break;
	case 'o':
		_move_to_end_of_line(edit);
		_newline(edit);
		edit->mode = EDIT_MODE_INSERT;
		break;
	case 'a':
		edit_move_right(edit);
		edit->mode = EDIT_MODE_INSERT;
		break;
	case 'v':
		edit->mode = EDIT_MODE_VISUAL;
		break;
	case 'h':
	case KEY_LEFT:
	case KEY_BACKSPACE:
		edit_move_left(edit);
		break;
	case 'j':
	case KEY_DOWN:
		edit_move_down(edit);
		break;
	case 'k':
	case KEY_UP:
		edit_move_up(edit);
		break;
	case 'l':
	case KEY_RIGHT:
		edit_move_right(edit);
		break;
	case 'q':
		edit_quit(edit);
		break;
	}
}

/* Handles the INSERT mode
 *
 * This mode functions much like your usual run-of-the-mill IDEs and text
 * editors. You type characters and they show up
 */
void edit_mode_insert(Edit *edit, int ch) {
	switch( ch ) {
	case CTRL('['):
	case CTRL('n'):
		edit->mode = EDIT_MODE_NORMAL;
		break;
	case '\n':
		_newline(edit);
		break;
	case KEY_UP:
		edit_move_up(edit);
		break;
	case KEY_DOWN:
		edit_move_down(edit);
		break;
	case KEY_LEFT:
		edit_move_left(edit);
		break;
	case KEY_RIGHT:
		edit_move_right(edit);
		break;
	case KEY_BACKSPACE:
		edit_delete_char(edit);
		break;
	default:
		edit_insert_char(edit, ch);
	}
}

/* Handles the VISUAL mode
 *
 * This mode allows you to select text and perform commands on the selection
 */
void edit_mode_visual(Edit *edit, int ch) {
	switch( ch ) {
	case CTRL('['):
	case CTRL('n'):
		edit->mode = EDIT_MODE_NORMAL;
		break;
	}
}

/* Handles the COMMAND mode
 *
 * This mode allows you to run commands that either affect the text or perform
 * meta actions like saving, loading, etc.
 */
void edit_mode_command(Edit *edit, int ch) {
	/* Run command */
	if( ch == '\n' ) {
		_handle_command(edit);
		_exit_command_typing(edit);
		return;
	}

	/* Erase character */
	if( ch == KEY_BACKSPACE ) {
		line_delete_char_at_end(&edit->cmd);
		_render_command(edit);

		return;
	}

	/* Ignore unprintable characters */
	if( ch < 32 || ch > 126 ) {
		return;
	}

	/* Append character */
	line_insert_char_at_end(&edit->cmd, ch);
	_render_command(edit);
}

/* Inserts a character under the cursor */
void edit_insert_char(Edit *edit, char c) {
	file_insert_char(&edit->file, edit->line, edit->idx, c);
	++edit->idx;
	_update_cursor_x(edit);

	edit_render_current_line(edit);
}

/* Deletes the character under the cursor */
void edit_delete_char(Edit *edit) {
	/* If at the beginning of the line, append it to the previous and move the
	 * lines below one row up
	 */
	if( edit->idx == 0 ) {
		edit->idx = file_move_line_up(&edit->file, edit->line);
		_update_gutter(edit);
		edit_move_up(edit);
		edit_render(edit);
		return;
	}

	file_delete_char(&edit->file, edit->line, edit->idx);

	--edit->idx;
	_update_cursor_x(edit);

	edit_render_current_line(edit);
}

/* If possible, moves the cursor up one row */
void edit_move_up(Edit *edit) {
	if( edit->line <= 0 ) {
		edit->line = 0;
		return;
	}

	--edit->line;
	if( edit->y == 0 ) {
		edit->vy = edit->line - (edit->h - 4);
		edit->y = edit->h - 3;
		edit_render(edit);
	}

	--edit->y;
	_update_cursor_x(edit);

	refresh();
}

/* If possible, moves the cursor down one row */
void edit_move_down(Edit *edit) {
	const size_t lines = edit->file.length;
	if( edit->line >= lines - 1 ) {
		edit->line = lines - 1;
		return;
	}

	++edit->line;
	if( ++edit->y >= edit->h - 3 ) {
		edit->vy = edit->line;
		edit->y = 0;
		edit_render(edit);
	}

	_update_cursor_x(edit);

	refresh();
}

/* If possible, moves the cursor left one column */
void edit_move_left(Edit *edit) {
	if( edit->idx <= 0 ) {
		edit->idx = 0;
		return;
	}

	--edit->idx;
	_update_cursor_x(edit);

	refresh();
}

/* If possible, moves the cursor right one column */
void edit_move_right(Edit *edit) {
	if( edit->x >= edit->w - 1 ) {
		edit->x = edit->w - 1;
		return;
	}

	++edit->idx;
	_update_cursor_x(edit);

	refresh();
}

/* Sets the status message */
void edit_set_status(Edit *edit, const char *fmt, ...) {
	if( *fmt == '\0' ) {
		memset(edit->msg, 0, STATUS_MSG_LEN);
		edit->msg_len = 0;
		return;
	}

	va_list args;
	va_start(args, fmt);

	int len = vsnprintf(edit->msg, STATUS_MSG_LEN, fmt, args);
	edit->msg_len = MIN(len, STATUS_MSG_LEN);

	va_end(args);

	edit_render_status(edit);
}

/* Renders the status bar */
void edit_render_status(Edit *edit) {
	const size_t bottom = edit->h - 1;

	move(bottom, 0);
	clrtoeol();

	printw("%s > ", _get_mode_string(edit));
	printw("%zu %zu > ", edit->idx + 1, edit->line + 1);
	printw("%s", edit->file.name);

	if( edit->msg_len ) {
		move(bottom, edit->w - STATUS_MSG_LEN);
		if( edit->msg_len == STATUS_MSG_LEN ) {
			printw("%.*s...", edit->msg_len - 3, edit->msg);
		} else {
			printw("%*s", edit->msg_len, edit->msg);
		}
	}

	move(edit->y, edit->x);
	refresh();
}

/* Renders the current file */
void edit_render(Edit *edit) {
	erase();
	_update_gutter(edit);
	file_render(&edit->file, edit->vy, edit->gutter);

	move(edit->y, edit->x);
}

/* Renders the current line */
void edit_render_current_line(Edit *edit) {
	edit_render_line(edit, edit->line);
}

/* Renders a line in the file */
void edit_render_line(Edit *edit, size_t idx) {
	_update_gutter(edit);
	file_render_line(&edit->file, idx, edit->gutter);
}

/* Quits the editor */
void edit_quit(Edit *edit) {
	edit_free(edit);
	edit->running = false;
}

/* Saves the current file */
void edit_save(Edit *edit) {
	file_save(&edit->file, NULL);
}

/* Returns the line under the cursor */
Line *edit_get_current_line(Edit *edit) {
	return edit_get_line(edit, edit->line);
}

/* Returns the line at row @idx */
Line *edit_get_line(Edit *edit, size_t idx) {
	return file_get_line(&edit->file, idx);
}

/* Returns the length of the current line */
long edit_get_current_line_length(Edit *edit) {
	return edit_get_line_length(edit, edit->line);
}

/* Returns the length of the line at row @idx */
long edit_get_line_length(Edit *edit, size_t idx) {
	return file_get_line_length(&edit->file, idx);
}

static void _exit_command_typing(Edit *edit) {
	line_erase(&edit->cmd);
	_clear_command(edit);
	edit->mode = EDIT_MODE_NORMAL;
}

static void _render_command(Edit *edit) {
	const size_t y = edit->h - 2;

	move(y, 0);
	clrtoeol();

	printw("cmd> %.*s ", (int)edit->cmd.length, edit->cmd.text);

	move(edit->y, edit->x);
	refresh();
}

static void _clear_command(Edit *edit) {
	const size_t y = edit->h - 2;

	move(y, 0);
	clrtoeol();

	move(edit->y, edit->x);
	refresh();
}

#define MATCH_CMD(C) if( strncmp(cmd, (C), len) == 0 )

static void _handle_command(Edit *edit) {
	char *cmd = line_get_c_str(&edit->cmd);
	const int len = strlen(cmd);

	if( len == 0 ) {
		return;
	}

	MATCH_CMD("w") {
		edit_save(edit);
		return;
	}

	MATCH_CMD("q") {
		edit_quit(edit);
		return;
	}

	MATCH_CMD("wq") {
		edit_save(edit);
		edit_quit(edit);
		return;
	}

	edit_set_status(edit, "unknown command '%s'", cmd);
}

static void _update_gutter(Edit *edit) {
	size_t line_count = edit->file.length;

	edit->gutter = 1;
	do {
		++edit->gutter;
		line_count /= 10;
	} while( line_count != 0 );
}

static void _update_cursor_x(Edit *edit) {
	const long s_length = edit_get_current_line_length(edit);
	if( s_length == -1 ) {
		return;
	}

	const size_t length = (size_t)s_length;
	if( edit->idx >= (size_t)length ) {
		edit->idx = length;
	}

	edit->x = edit->idx + edit->gutter;
	move(edit->y, edit->x);
}

static void _move_to_start_of_line(Edit *edit) {
	edit->idx = 0;
	_update_cursor_x(edit);
}

static void _move_to_end_of_line(Edit *edit) {
	Line *line = edit_get_current_line(edit);
	edit->idx = line->length;
	_update_cursor_x(edit);
}

static void _move_to_end_of_file(Edit *edit) {
	edit->line = edit->file.length - 1;
	_move_to_start_of_line(edit);
}

static void _newline(Edit *edit) {
	file_break_line(&edit->file, edit->line++, edit->idx);
	++edit->y;

	_update_gutter(edit);

	edit->idx = 0;
	_update_cursor_x(edit);

	edit_render(edit);
}

static char *_get_mode_string(Edit *edit) {
	switch( edit->mode ) {
	case EDIT_MODE_NORMAL:
		return "NORMAL ";
	case EDIT_MODE_INSERT:
		return "INSERT ";
	case EDIT_MODE_VISUAL:
		return "VISUAL ";
	case EDIT_MODE_COMMAND:
		return "COMMAND";
	default:
		fprintf(stderr, "Invalid mode no. %d!\n", edit->mode);
		exit(1);
	}
}
