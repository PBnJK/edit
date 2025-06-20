/* edit
 * The "edit" editor
 */

#include <stdlib.h>

#include <ncurses.h>

#include "global.h"

#include "file.h"

#include "edit.h"

static void _newline(Edit *edit);

void edit_new(Edit *edit, const char *filename) {
	edit->line = 0;
	edit->idx = 0;
	edit->x = 0;
	edit->y = 0;

	file_load(&edit->file, filename);
}

void edit_update(Edit *edit) {
	int ch = getch();

	switch( ch ) {
	case '\\': /* TODO: vim-like "INSERT" and "NORMAL" modes, better keybind */
		exit(0); /* TODO: Exit gracefully */
		break;
	case CTRL('s'):
		break; /* TODO: Save */
	case '\n':
		_newline(edit);
		break;
	case KEY_BACKSPACE:
		edit_delete_char(edit);
		break;
	default:
		edit_insert_char(edit, ch);
	}
}

void edit_render(Edit *edit) {
	erase();
	move(0, 0);

	file_render(&edit->file);

	move(edit->y, edit->x);
}

void edit_insert_char(Edit *edit, char c) {
	file_insert_char(&edit->file, edit->line, edit->idx, c);
	++edit->idx;
	++edit->x;

	/* TODO:
	 *   Obviously, it's insane to re-render the whole file on change
	 *   Only re-render the currrent line (and others *AS NEEDED*)
	 */
	file_render(&edit->file);
}

void edit_delete_char(Edit *edit) {
	if( edit->idx == 0 ) {
		/* TODO: Append to the end of the last line */
		return;
	}

	file_delete_char(&edit->file, edit->line, edit->idx);
	--edit->idx;
	--edit->x;

	/* TODO:
	 *   Obviously, it's insane to re-render the whole file on change
	 *   Only re-render the currrent line (and others *AS NEEDED*)
	 */
	file_render(&edit->file);
}

static void _newline(Edit *edit) {
	file_insert_empty_line(&edit->file, ++edit->line);
	++edit->y;

	edit->x = 0;
	edit->idx = 0;
}
