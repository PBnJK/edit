/* edit
 * The "edit" editor
 */

#include <stddef.h>
#include <string.h>

#include <ncurses.h>

#include "file.h"

#include "edit.h"

static void _fix_cursor_x(Edit *edit);

static void _newline(Edit *edit);

/* Initializes the editor */
void edit_new(Edit *edit, const char *filename) {
	edit->line = 0;
	edit->idx = 0;
	edit->x = 0;
	edit->y = 0;

	getmaxyx(stdscr, edit->h, edit->w);

	memset(edit->msg, 0, STATUS_MSG_LEN);
	edit->msg_len = 0;

	file_new(&edit->file, filename);
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
bool edit_update(Edit *edit) {
	int ch = getch();

	switch( ch ) {
	case '\\':
		edit_quit(edit);
		return false;
	case 'w':
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

	edit_update_status(edit);
	return true;
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
	if( *msg ) {
		memset(edit->msg, 0, STATUS_MSG_LEN);
		edit->msg_len = 0;
		return;
	}

	strncpy(edit->msg, msg, STATUS_MSG_LEN - 1);
	edit->msg_len = strlen(msg);
}

/* Updates the status bar */
void edit_update_status(Edit *edit) {
	const size_t bottom = edit->h - 1;

	move(bottom, 0);
	clrtoeol();

	printw("%zu %zu\n", edit->x, edit->y);

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
