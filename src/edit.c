/* edit
 * The "edit" editor
 */

#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <ncurses.h>

#include "global.h"

#include "file.h"
#include "line.h"

#include "edit.h"

static void _handle_command_typing(Edit *edit, int ch);
static void _render_command(Edit *edit);
static void _clear_command(Edit *edit);
static void _handle_command(Edit *edit);

static void _fix_cursor_x(Edit *edit);
static void _newline(Edit *edit);

static char *_get_mode_string(Edit *edit);

/* Initializes the editor */
void edit_new(Edit *edit, const char *filename) {
	edit->line = 0;
	edit->idx = 0;
	edit->x = 0;
	edit->y = 0;

	getmaxyx(stdscr, edit->h, edit->w);

	memset(edit->msg, 0, STATUS_MSG_LEN);
	edit->msg_len = 0;

	edit->running = true;

	edit->mode = EDIT_MODE_NORMAL;

	edit->is_typing_cmd = false;
	line_new(&edit->cmd);

	file_new(&edit->file, filename);
	edit_render(edit);
	edit_render_status(edit);
}

/* Frees the editor from memory */
void edit_free(Edit *edit) {
	file_free(&edit->file);
}

/* Updates the editor
 *
 * TODO:
 * - Vim-like "INSERT" and "NORMAL" modes, better keybinds
 * - Saving
 */
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
	}

	edit_render_status(edit);
}

void edit_mode_normal(Edit *edit, int ch) {
	if( edit->is_typing_cmd ) {
		_handle_command_typing(edit, ch);
		return;
	}

	switch( ch ) {
	case ':':
		edit->is_typing_cmd = true;
		break;
	case 'i':
		edit->mode = EDIT_MODE_INSERT;
		break;
	case KEY_BACKSPACE:
	case 'h':
		edit_move_left(edit);
		break;
	case 'j':
		edit_move_down(edit);
		break;
	case 'k':
		edit_move_up(edit);
		break;
	case 'l':
		edit_move_right(edit);
		break;
	case 'q':
		edit_quit(edit);
		break;
	}
}

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

void edit_mode_visual(Edit *edit, int ch) {
	UNUSED(edit);
	UNUSED(ch);
}

/* Inserts a character under the cursor */
void edit_insert_char(Edit *edit, char c) {
	file_insert_char(&edit->file, edit->line, edit->idx, c);
	++edit->idx;
	++edit->x;

	edit_render_current_line(edit);
}

/* Deletes the character under the cursor */
void edit_delete_char(Edit *edit) {
	if( edit->idx == 0 ) {
		/* TODO: Append to the end of the last line */
		return;
	}

	file_delete_char(&edit->file, edit->line, edit->idx);
	--edit->idx;
	--edit->x;

	edit_render_current_line(edit);
}

/* If possible, moves the cursor up one row */
void edit_move_up(Edit *edit) {
	if( edit->y <= 0 ) {
		edit->y = 0;
		return;
	}

	--edit->y;
	--edit->line;
	_fix_cursor_x(edit);

	move(edit->y, edit->x);
	refresh();
}

/* If possible, moves the cursor down one row */
void edit_move_down(Edit *edit) {
	const size_t lines = edit->file.length;
	if( edit->y >= lines - 1 ) {
		edit->y = lines - 1;
		return;
	}

	++edit->y;
	++edit->line;
	_fix_cursor_x(edit);

	refresh();
}

/* If possible, moves the cursor left one column */
void edit_move_left(Edit *edit) {
	if( edit->x <= 0 ) {
		edit->x = 0;
		return;
	}

	--edit->x;
	--edit->idx;
	_fix_cursor_x(edit);

	refresh();
}

/* If possible, moves the cursor right one column */
void edit_move_right(Edit *edit) {
	if( edit->x >= edit->w - 1 ) {
		edit->x = edit->w - 1;
		return;
	}

	++edit->x;
	++edit->idx;
	_fix_cursor_x(edit);

	refresh();
}

/* Sets the status message */
void edit_set_status(Edit *edit, const char *msg) {
	if( *msg == '\0' ) {
		memset(edit->msg, 0, STATUS_MSG_LEN);
		edit->msg_len = 0;
		return;
	}

	strncpy(edit->msg, msg, STATUS_MSG_LEN - 1);
	edit->msg_len = strlen(msg);

	edit_render_status(edit);
}

/* Renders the status bar */
void edit_render_status(Edit *edit) {
	const size_t bottom = edit->h - 1;

	move(bottom, 0);
	clrtoeol();

	printw("%s | %zu %zu\n", _get_mode_string(edit), edit->x, edit->y);

	if( edit->msg_len ) {
		move(bottom, edit->w - STATUS_MSG_LEN);
		if( edit->msg_len > STATUS_MSG_LEN ) {
			printw("%.37s...", edit->msg);
		} else {
			printw("%*s", edit->msg_len, edit->msg);
		}
	}

	move(edit->y, edit->x);
	refresh();
}

/* Renders the current file */
void edit_render(Edit *edit) {
	file_render(&edit->file);

	move(edit->y, edit->x);
}

/* Renders the current line */
void edit_render_current_line(Edit *edit) {
	edit_render_line(edit, edit->line);
}

/* Renders a line in the file */
void edit_render_line(Edit *edit, size_t idx) {
	file_render_line(&edit->file, idx);
}

/* Quits the editor */
void edit_quit(Edit *edit) {
	edit_free(edit);
	edit->running = false;
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

static void _handle_command_typing(Edit *edit, int ch) {
	/* Run command */
	if( ch == '\n' ) {
		_handle_command(edit);
		line_erase(&edit->cmd);
		_clear_command(edit);
		edit->is_typing_cmd = false;
		return;
	}

	/* Erase character */
	if( ch == KEY_BACKSPACE ) {
		line_delete_char_at_end(&edit->cmd);
		_render_command(edit);
		return;
	}

	/* Ignore unprintable characters */
	if( ch < 33 || ch > 126 ) {
		return;
	}

	/* Append character */
	line_insert_char_at_end(&edit->cmd, ch);
	_render_command(edit);
}

static void _render_command(Edit *edit) {
	const size_t y = edit->h - 2;

	move(y, 0);
	clrtoeol();

	printw("> %.*s ", (int)edit->cmd.length, edit->cmd.text);

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

static void _handle_command(Edit *edit) {
	char *cmd = line_get_c_str(&edit->cmd);
	const int len = strlen(cmd);

	edit_set_status(edit, edit->cmd.text);

	if( strncmp(cmd, "q", len) == 0 ) {
		edit_quit(edit);
		return;
	}
}

static void _fix_cursor_x(Edit *edit) {
	const long s_length = edit_get_current_line_length(edit);
	if( s_length == -1 ) {
		return;
	}

	const size_t length = (size_t)s_length;
	if( edit->idx >= (size_t)length ) {
		edit->idx = length;
		edit->x = edit->idx;
		move(edit->y, edit->x);
	}
}

static void _newline(Edit *edit) {
	file_break_line(&edit->file, edit->line++, edit->idx);
	++edit->y;

	edit->x = 0;
	edit->idx = 0;

	file_render(&edit->file);
}

static char *_get_mode_string(Edit *edit) {
	switch( edit->mode ) {
	case EDIT_MODE_NORMAL:
		return "NORMAL";
	case EDIT_MODE_INSERT:
		return "INSERT";
	case EDIT_MODE_VISUAL:
		return "VISUAL";
	default:
		fprintf(stderr, "Invalid mode no. %d!\n", edit->mode);
		exit(1);
	}
}
