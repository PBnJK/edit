/* edit
 * The "edit" editor
 */

#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef USE_PDCURSES
#include <curses.h>
#else
#include <ncurses.h>
#endif

#include "global.h"

#include "file.h"
#include "line.h"
#include "cmd.h"
#include "prompt.h"
#include "config.h"

#include "edit.h"

#define SET_CURSOR_BLINK_BLOCK "\x1b[1 q"
#define SET_CURSOR_STEADY_BLOCK "\x1b[2 q"
#define SET_CURSOR_BLINK_UNDERLINE "\x1b[3 q"
#define SET_CURSOR_STEADY_UNDERLINE "\x1b[4 q"
#define SET_CURSOR_BLINK_BAR "\x1b[5 q"
#define SET_CURSOR_STEADY_BAR "\x1b[6 q"

static void _write_raw(const char *str);

static void _insert_char_pair(Edit *edit, CommandStack *stack, char l, char r);

static char _get_number_arg(Edit *edit, size_t initial);
static void _get_char_arg(Edit *edit);

static void _do_cmd_h(Edit *edit);
static void _do_cmd_j(Edit *edit);
static void _do_cmd_k(Edit *edit);
static void _do_cmd_l(Edit *edit);

static void _do_cmd_g(Edit *edit);
static void _do_cmd_G(Edit *edit);

static void _exit_command_typing(Edit *edit);
static void _render_command(Edit *edit);
static void _clear_command(Edit *edit);

static void _handle_command(Edit *edit);
static void _handle_shell_command(Edit *edit, const char *cmd);

static void _handle_complex_command(Edit *edit, const char *cmd);
static char *_match_command(const char *cmd, const char *match, int len);

static void _update_gutter(Edit *edit);
static void _update_cursor_x(Edit *edit);

static void _move_to_start_of_line(Edit *edit);
static void _move_to_end_of_line(Edit *edit);
static void _move_to_idx(Edit *edit, size_t idx);

static void _move_to_start_of_file(Edit *edit);
static void _move_to_end_of_file(Edit *edit);

static void _newline(Edit *edit);

static bool _ask_to_save(Edit *edit);

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

	config_init(&edit->config);

	edit_change_to_normal(edit);

	line_new(&edit->cmd);
	edit->cmd_char = '\0';
	edit->cmd_num = 0;

	edit->vis_start_idx = 0;
	edit->vis_start_line = 0;
	edit->vis_length = 0;

	cmd_init(&edit->undo);
	cmd_init(&edit->redo);

	file_new(&edit->file, filename);
	_update_gutter(edit);
	_update_cursor_x(edit);

	edit_render(edit);
	edit_render_status(edit);
}

/* Frees the editor from memory */
void edit_free(Edit *edit) {
	file_free(&edit->file);
}

/* Reloads the current file */
void edit_reload(Edit *edit) {
	char *name = file_get_name(&edit->file);
	edit_load(edit, name);
}

/* Loads the given file */
void edit_load(Edit *edit, const char *filename) {
	if( file_is_dirty(&edit->file) && !_ask_to_save(edit) ) {
		return;
	}

	file_free(&edit->file);
	file_new(&edit->file, filename);

	char *name = file_get_display_name(&edit->file);
	edit_set_status(edit, "loaded file '%s'", name);

	edit_render(edit);
	edit_render_status(edit);

	edit->vx = 0;
	edit->vy = 0;

	edit->line = 0;
	edit->y = 0;

	edit->idx = 0;
	edit->x = 0;
	_update_cursor_x(edit);
}

/* Saves the current file */
void edit_save(Edit *edit) {
	edit_save_as(edit, NULL);
}

/* Saves the current file with the name @as */
void edit_save_as(Edit *edit, const char *as) {
	file_save(&edit->file, as);

	char *name = file_get_display_name(&edit->file);
	edit_set_status(edit, "saved file as '%s'", name);
}

/* Updates the editor */
void edit_update(Edit *edit) {
	int ch = getch();
	if( ch == KEY_RESIZE || ch == ERR ) {
		edit_refresh(edit);
		return;
	}

	switch( edit->mode ) {
	case EDIT_MODE_NORMAL:
		edit_mode_normal(edit, ch);
		break;
	case EDIT_MODE_INSERT:
		edit_mode_insert(edit, ch);
		break;
	case EDIT_MODE_REPLACE:
		edit_mode_replace(edit, ch);
		break;
	case EDIT_MODE_VISUAL:
		edit_mode_visual(edit, ch);
		break;
	case EDIT_MODE_COMMAND:
		edit_mode_command(edit, ch);
	}

	edit_render_status(edit);
}

/* Refreshes the window after a resize */
void edit_refresh(Edit *edit) {
	endwin();
	refresh();

	edit->w = COLS;
	edit->h = LINES;

	_update_gutter(edit);
	_update_cursor_x(edit);

	edit_render(edit);
	edit_render_status(edit);

	refresh();
}

/* Change to NORMAL mode */
void edit_change_to_normal(Edit *edit) {
	_write_raw(SET_CURSOR_STEADY_BLOCK);
	edit->mode = EDIT_MODE_NORMAL;
}

/* Change to INSERT mode */
void edit_change_to_insert(Edit *edit) {
	_write_raw(SET_CURSOR_STEADY_BAR);
	edit->mode = EDIT_MODE_INSERT;
}

/* Change to REPLACE mode */
void edit_change_to_replace(Edit *edit) {
	_write_raw(SET_CURSOR_STEADY_UNDERLINE);
	edit->mode = EDIT_MODE_REPLACE;
}

/* Change to VISUAL mode */
void edit_change_to_visual(Edit *edit) {
	edit->mode = EDIT_MODE_VISUAL;
}

/* Change to COMMAND mode */
void edit_change_to_command(Edit *edit) {
	edit->mode = EDIT_MODE_COMMAND;
	_render_command(edit);
}

/* Handles the NORMAL mode
 *
 * In this mode, you cannot type characters directly, but can instead use
 * shortcuts to perform certain actions
 */
void edit_mode_normal(Edit *edit, int ch) {
	if( ch >= '0' && ch <= '9' ) {
		ch = _get_number_arg(edit, ch - '0');
	}

	switch( ch ) {
	case ':': /* Enter COMMAND mode */
		edit_change_to_command(edit);
		break;
	case KEY_IC:
	case 'i': /* Enter INSERT mode */
		edit_change_to_insert(edit);
		break;
	case 'R': /* Enter REPLACE mode */
		edit_change_to_replace(edit);
		break;
	case '^': /* Move to the start of the line */
		_move_to_start_of_line(edit);
		break;
	case '$': /* Move to the end of the line */
		_move_to_end_of_line(edit);
		break;
	case 'g': /* Handle the 'g' command */
		_do_cmd_g(edit);
		break;
	case 'G': /* Handle the 'G' command */
		_do_cmd_G(edit);
		break;
	case 'o': /* Enter INSERT mode on a new line */
		_move_to_end_of_line(edit);
		edit_insert_char(edit, &edit->undo, '\n');
		edit_change_to_insert(edit);
		break;
	case 'a': /* Enter INSERT mode after the current character */
		edit_move_right(edit);
		edit_change_to_insert(edit);
		break;
	case 'v': /* Enter VISUAL mode */
		edit_change_to_visual(edit);
		break;
	case 'u':
	case CTRL('z'): /* Undo */
		edit_undo(edit);
		break;
	case CTRL('R'):
	case CTRL('y'): /* Redo */
		edit_redo(edit);
		break;
	case 'h':
	case KEY_LEFT:
	case KEY_BACKSPACE: /* Move left */
		_do_cmd_h(edit);
		break;
	case 'j':
	case KEY_DOWN: /* Move down */
		_do_cmd_j(edit);
		break;
	case 'k':
	case KEY_UP: /* Move up */
		_do_cmd_k(edit);
		break;
	case 'l':
	case KEY_RIGHT: /* Move right */
		_do_cmd_l(edit);
		break;
	}

	edit->cmd_char = '\0';
	edit->cmd_num = 0;
}

/* Handles the INSERT mode
 *
 * This mode functions much like your usual run-of-the-mill IDEs and text
 * editors. You type characters and they show up
 */
void edit_mode_insert(Edit *edit, int ch) {
	switch( ch ) {
	case CTRL('['):
	case CTRL('n'): /* Enter NORMAL mode */
		edit_change_to_normal(edit);
		break;
	case KEY_IC: /* Enter REPLACE mode */
		edit_change_to_replace(edit);
		break;
	case CTRL('Z'):
		edit_undo(edit);
		break;
	case CTRL('R'):
	case CTRL('y'): /* Redo */
		edit_redo(edit);
		break;
	case '\n': /* Break line */
		edit_insert_char(edit, &edit->undo, '\n');
		break;
	case '\t': /* TODO: Handle TAB properly! */
		edit_insert_char(edit, &edit->undo, ' ');
		edit_insert_char(edit, &edit->undo, ' ');
		edit_insert_char(edit, &edit->undo, ' ');
		edit_insert_char(edit, &edit->undo, ' ');
		break;
	case KEY_UP: /* Move up */
		edit_move_up(edit);
		break;
	case KEY_DOWN: /* Move down */
		edit_move_down(edit);
		break;
	case KEY_LEFT: /* Move left */
		edit_move_left(edit);
		break;
	case KEY_RIGHT: /* Move right */
		edit_move_right(edit);
		break;
	case KEY_BACKSPACE: /* Erase character */
		edit_delete_char(edit, &edit->undo);
		break;
	case '"': /* Inserts matching "" */
		_insert_char_pair(edit, &edit->undo, '"', '"');
		break;
	case '(': /* Inserts matching () */
		_insert_char_pair(edit, &edit->undo, '(', ')');
		break;
	case '[': /* Inserts matching [] */
		_insert_char_pair(edit, &edit->undo, '[', ']');
		break;
	case '{': /* Inserts matching {} */
		_insert_char_pair(edit, &edit->undo, '{', '}');
		break;
	default: /* Type character */
		edit_insert_char(edit, &edit->undo, ch);
	}
}

/* Handles the REPLACE mode
 *
 * This mode functions similarly to INSERT, except it replaces characters
 * directly rather than append them "smartly"
 */
void edit_mode_replace(Edit *edit, int ch) {
	switch( ch ) {
	case CTRL('['):
	case CTRL('n'): /* Enter NORMAL mode */
		edit_change_to_normal(edit);
		break;
	case KEY_IC: /* Enter INSERT mode */
		edit_change_to_insert(edit);
		break;
	case CTRL('Z'):
		edit_undo(edit);
		break;
	case CTRL('R'):
	case CTRL('y'): /* Redo */
		edit_redo(edit);
		break;
	case '\n': /* Break line */
		edit_insert_char(edit, &edit->undo, '\n');
		break;
	case '\t': /* TODO: Handle TAB properly! */
		edit_replace_char(edit, &edit->undo, ' ');
		edit_insert_char(edit, &edit->undo, ' ');
		edit_insert_char(edit, &edit->undo, ' ');
		edit_insert_char(edit, &edit->undo, ' ');
		break;
	case KEY_UP: /* Move up */
		edit_move_up(edit);
		break;
	case KEY_DOWN: /* Move down */
		edit_move_down(edit);
		break;
	case KEY_BACKSPACE:
	case KEY_LEFT: /* Move left */
		edit_move_left(edit);
		break;
	case KEY_RIGHT: /* Move right */
		edit_move_right(edit);
		break;
	default: /* Type character */
		edit_replace_char(edit, &edit->undo, ch);
	}
}

/* Handles the VISUAL mode
 * This mode allows you to select text and perform commands on the selection
 */
void edit_mode_visual(Edit *edit, int ch) {
	switch( ch ) {
	case CTRL('['):
	case CTRL('n'): /* Enter NORMAL mode */
		edit_change_to_normal(edit);
		break;
	case 'h':
	case KEY_LEFT: /* Move left */
		edit_move_left(edit);
		break;
	case 'j':
	case KEY_DOWN: /* Move down */
		edit_move_down(edit);
		break;
	case 'k':
	case KEY_UP: /* Move up */
		edit_move_up(edit);
		break;
	case 'l':
	case KEY_RIGHT: /* Move right */
		edit_move_right(edit);
		break;
	}
}

/* Handles the COMMAND mode
 *
 * This mode allows you to run commands that either affect the text or perform
 * meta actions like saving, loading, etc.
 */
void edit_mode_command(Edit *edit, int ch) {
	switch( ch ) {
	case CTRL('['):
	case CTRL('n'): /* Enter NORMAL mode */
		edit_change_to_normal(edit);
		_exit_command_typing(edit);
		break;
	case '\n': /* Run command */
		_handle_command(edit);
		_exit_command_typing(edit);
		break;
	case KEY_BACKSPACE: /* Erase character */
		line_delete_char_at_end(&edit->cmd);
		_render_command(edit);
		break;
	default: /* Type (printable) characters */
		if( ch >= 32 && ch <= 126 ) {
			line_insert_char_at_end(&edit->cmd, ch);
			_render_command(edit);
		}
	}
}

/* Creates a CMD_REP_CH command */
void edit_rep_ch(Edit *edit, CommandStack *stack, char ch) {
	cmd_rep_ch(stack, edit->line, edit->idx - 1, ch);
}

/* Creates a CMD_ADD_CH command */
void edit_add_ch(Edit *edit, CommandStack *stack, char ch) {
	cmd_add_ch(stack, edit->line, edit->idx, ch);
}

/* Creates a CMD_DEL_CH command */
void edit_del_ch(Edit *edit, CommandStack *stack, char ch) {
	cmd_del_ch(stack, edit->line, edit->idx, ch);
}

/* Creates a CMD_NEW_LINE command */
void edit_new_line(Edit *edit, CommandStack *stack) {
	cmd_new_line(stack, edit->line, edit->idx);
}

/* Undoes the previous action */
void edit_undo(Edit *edit) {
	Command *cmd = cmd_pop(&edit->undo);
	if( cmd == NULL ) {
		edit_set_status(edit, "nothing to undo!");
		return;
	}

	edit_perform_cmd(edit, &edit->redo, cmd);
}

/* Redoes the previous action */
void edit_redo(Edit *edit) {
	Command *cmd = cmd_pop(&edit->redo);
	if( cmd == NULL ) {
		edit_set_status(edit, "nothing to redo!");
		return;
	}

	edit_perform_cmd(edit, &edit->undo, cmd);
}

/* Runs an edit command */
void edit_perform_cmd(Edit *edit, CommandStack *stack, Command *cmd) {
	edit->idx = cmd->idx;
	_update_cursor_x(edit);

	edit_goto(edit, cmd->line);

	switch( cmd->type ) {
	case CMD_REP_CH:
		edit_replace_char(edit, stack, cmd->data.ch);
		break;
	case CMD_ADD_CH:
		edit_insert_char(edit, stack, cmd->data.ch);
		break;
	case CMD_DEL_CH:
		edit_delete_char(edit, stack);
		break;
	case CMD_NEW_LINE:
		edit_insert_char(edit, stack, '\n');
		break;
	default:
		edit_set_status(edit, "not yet implemented!");
	}

	edit_render(edit);
}

/* Jumps to a line */
void edit_goto(Edit *edit, size_t idx) {
	if( idx >= edit->file.length ) {
		idx = edit->file.length - 1;
	}

	if( idx == edit->line ) {
		return;
	}

	edit->line = idx;

	size_t offset = edit->vy + edit->y;
	if( idx < offset ) {
		if( edit->vy > idx ) {
			edit->y = 0;
			edit->vy = idx;
			edit_render(edit);
		} else {
			edit->y = idx;
		}
	} else {
		size_t ui_offset = edit_get_ui_offset(edit);
		if( idx - edit->vy >= ui_offset ) {
			edit->vy = idx - ui_offset + 1;
			edit->y = ui_offset - 1;
			edit_render(edit);
		} else {
			edit->y = idx;
		}
	}

	_update_cursor_x(edit);
}

/* Directly replaces the character under the cursor with @ch */
void edit_replace_char(Edit *edit, CommandStack *stack, char ch) {
	char prev = file_replace_char(&edit->file, edit->line, edit->idx, ch);
	++edit->idx;
	_update_cursor_x(edit);

	edit_render_current_line(edit);

	edit->last_ins_line = edit->line;
	edit->last_ins_idx = edit->idx;

	edit_rep_ch(edit, stack, prev);
}

/* Inserts a character under the cursor */
void edit_insert_char(Edit *edit, CommandStack *stack, char ch) {
	if( ch == '\n' ) {
		_newline(edit);
	} else {
		file_insert_char(&edit->file, edit->line, edit->idx, ch);
		++edit->idx;
		_update_cursor_x(edit);

		edit_render_current_line(edit);
	}

	edit->last_ins_line = edit->line;
	edit->last_ins_idx = edit->idx;

	edit_del_ch(edit, stack, ch);
}

/* Deletes the character under the cursor */
void edit_delete_char(Edit *edit, CommandStack *stack) {
	/* If at the beginning of the line, append it to the previous and move the
	 * lines below one row up
	 */
	if( edit->idx == 0 ) {
		edit->idx = file_move_line_up(&edit->file, edit->line);
		_update_gutter(edit);
		edit_move_up(edit);
		edit_render(edit);

		edit_new_line(edit, stack);
	} else {
		char prev = file_delete_char(&edit->file, edit->line, edit->idx);

		--edit->idx;
		_update_cursor_x(edit);

		edit_render_current_line(edit);

		edit_add_ch(edit, stack, prev);
	}

	edit->last_ins_line = edit->line;
	edit->last_ins_idx = edit->idx;
}

/* If possible, moves the cursor up one row */
bool edit_move_up(Edit *edit) {
	if( edit->line <= 0 ) {
		edit->line = 0;
		return false;
	}

	--edit->line;
	if( edit->y == 0 ) {
		if( edit->vy > 0 ) {
			--edit->vy;
			edit_render(edit);
		}
	} else {
		--edit->y;
	}

	_update_cursor_x(edit);
	refresh();

	return true;
}

/* If possible, moves the cursor down one row */
bool edit_move_down(Edit *edit) {
	const size_t lines = edit->file.length;
	if( edit->line >= lines - 1 ) {
		edit->line = lines - 1;
		return false;
	}

	++edit->line;

	size_t ui_offset = edit_get_ui_offset(edit);
	if( ++edit->y >= ui_offset ) {
		++edit->vy;
		edit->y = ui_offset - 1;
		edit_render(edit);
	}

	_update_cursor_x(edit);
	refresh();

	return true;
}

/* If possible, moves the cursor left one column */
bool edit_move_left(Edit *edit) {
	if( edit->idx <= 0 ) {
		edit->idx = 0;
		return false;
	}

	--edit->idx;
	_update_cursor_x(edit);

	refresh();

	return true;
}

/* If possible, moves the cursor right one column */
bool edit_move_right(Edit *edit) {
	if( edit->x >= edit->w - 1 ) {
		edit->x = edit->w - 1;
		return false;
	}

	++edit->idx;
	_update_cursor_x(edit);

	refresh();

	return true;
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

	const char *name = file_get_display_name(&edit->file);
	const char asterisk = file_is_dirty(&edit->file) ? '*' : ' ';
	printw("%s %c", name, asterisk);

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
	file_render_line(&edit->file, idx, edit->vy, edit->gutter);
}

/* Sets a config option */
void edit_set_config(Edit *edit, char *key, char *value) {
	config_set(&edit->config, key, value);
}

/* Gets a config option */
char *edit_get_config(Edit *edit, char *key) {
	return config_get(&edit->config, key);
}

/* Quits the editor */
void edit_quit(Edit *edit) {
	if( file_is_dirty(&edit->file) && !_ask_to_save(edit) ) {
		return;
	}

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

/* Gets the screen size without the bottom UI */
size_t edit_get_ui_offset(Edit *edit) {
	return edit->h - 3;
}

/* Writes a raw string to standard output */
static void _write_raw(const char *str) {
	fputs(str, stdout);
	fflush(stdout);
}

/* Inserts a matching pair of characters, placing the cursor between them */
static void _insert_char_pair(Edit *edit, CommandStack *stack, char l, char r) {
	edit_insert_char(edit, stack, l);
	edit_insert_char(edit, stack, r);
	edit_move_left(edit);
}

/* Gets a number argument from the user */
static char _get_number_arg(Edit *edit, size_t initial) {
	int ch = getch();
	while( ch >= '0' && ch <= '9' ) {
		initial *= 10;
		initial += ch - '0';
		ch = getch();
	}

	edit->cmd_num = initial;
	return ch;
}

/* Gets a character from the user */
static void _get_char_arg(Edit *edit) {
	edit->cmd_char = getch();
}

/* Handles the "move left" command */
static void _do_cmd_h(Edit *edit) {
	size_t by = (edit->cmd_num ? edit->cmd_num : 1);
	for( size_t i = 0; i < by && edit_move_left(edit); ++i )
		;
}

/* Handles the "move down" command */
static void _do_cmd_j(Edit *edit) {
	size_t by = (edit->cmd_num ? edit->cmd_num : 1);
	for( size_t i = 0; i < by && edit_move_down(edit); ++i )
		;
}

/* Handles the "move up" command */
static void _do_cmd_k(Edit *edit) {
	size_t by = (edit->cmd_num ? edit->cmd_num : 1);
	for( size_t i = 0; i < by && edit_move_up(edit); ++i )
		;
}

/* Handles the "move right" command */
static void _do_cmd_l(Edit *edit) {
	size_t by = (edit->cmd_num ? edit->cmd_num : 1);
	for( size_t i = 0; i < by && edit_move_right(edit); ++i )
		;
}

/* Handles the 'g' command */
static void _do_cmd_g(Edit *edit) {
	_get_char_arg(edit);

	switch( edit->cmd_char ) {
	case 'i': /* Go to last insert */
		edit_goto(edit, edit->last_ins_line);
		_move_to_idx(edit, edit->last_ins_idx);
		edit_change_to_insert(edit);
		break;
	case 'g': /* Go to the start of the file */
		_move_to_start_of_file(edit);
		break;
	}
}

/* Handles the 'G' command */
static void _do_cmd_G(Edit *edit) {
	if( edit->cmd_num ) {
		edit_goto(edit, edit->cmd_num - 1);
	} else {
		_move_to_end_of_file(edit);
	}
}

/* Exits the command typing mode */
static void _exit_command_typing(Edit *edit) {
	line_erase(&edit->cmd);
	_clear_command(edit);

	edit->cmd_num = 0;

	edit_change_to_normal(edit);
}

/* Renders the current command */
static void _render_command(Edit *edit) {
	const size_t y = edit->h - 2;

	move(y, 0);
	clrtoeol();

	printw("cmd> %.*s ", (int)edit->cmd.length, edit->cmd.text);

	move(edit->y, edit->x);
	refresh();
}

/* Clears the command line */
static void _clear_command(Edit *edit) {
	const size_t y = edit->h - 2;

	move(y, 0);
	clrtoeol();

	move(edit->y, edit->x);
	refresh();
}

#define MATCH_SIMPLE_CMD(C) (strncmp(cmd, (C), len) == 0)

/* Handles an entered command */
static void _handle_command(Edit *edit) {
	char *cmd = line_get_c_str(&edit->cmd, false);
	const int len = strlen(cmd);

	if( len == 0 ) {
		return;
	}

	if( *cmd >= '0' && *cmd <= '9' ) {
		size_t n = 0;
		do {
			n *= 10;
			n += *cmd++ - '0';
		} while( *cmd >= '0' && *cmd <= '9' );

		/* If the command hasn't ended yet, assume this is a command with a
		 * numerical argument
		 */
		edit->cmd_num = n;
		if( *cmd ) {
			_handle_complex_command(edit, cmd);
		}

		if( n == 0 ) {
			edit_goto(edit, 0);
		} else {
			edit_goto(edit, n - 1);
		}

		return;
	}

	/* Reload the current file */
	if MATCH_SIMPLE_CMD( "e" ) {
		edit_reload(edit);
		return;
	}

	/* Write to file */
	if MATCH_SIMPLE_CMD( "w" ) {
		edit_save(edit);
		return;
	}

	/* Quit the editor */
	if MATCH_SIMPLE_CMD( "q" ) {
		edit_quit(edit);
		return;
	}

	/* Write to file and quit the editor*/
	if MATCH_SIMPLE_CMD( "wq" ) {
		edit_save(edit);
		edit_quit(edit);
		return;
	}

	if( *cmd == '!' ) {
		_handle_shell_command(edit, cmd + 1);
		return;
	}

	_handle_complex_command(edit, cmd);
}

#undef MATCH_SIMPLE_CMD

/* Executes a shell command
 * TODO: Capture output
 */
static void _handle_shell_command(Edit *edit, const char *cmd) {
	int err = system(cmd);
	edit_set_status(edit, "system call returned %d", err);
}

#define MATCH_CMD(C) ((args = _match_command(cmd, (C), strlen((C)))))

/* Handles more complex commands */
static void _handle_complex_command(Edit *edit, const char *cmd) {
	char *args;

	/* Load or create a file with the given name */
	if MATCH_CMD( "e " ) {
		edit_load(edit, args);
		return;
	}

	/* Writes the current file with a new name */
	if MATCH_CMD( "w " ) {
		edit_save_as(edit, args);
		return;
	}

	/* Sets a config option */
	if( MATCH_CMD("setc ") || MATCH_CMD("setconfig ") ) {
		char *key = args;
		char *value = NULL;

		char *sp = strchr(args, ' ');
		if( sp == NULL || sp[1] == '\0' ) {
			edit_set_status(edit, "config needs a value");
			return;
		}

		*sp = '\0';
		value = sp + 1;

		edit_set_config(edit, key, value);
		edit_set_status(edit, "'%s' = '%s'", key, value);
		return;
	}

	/* Gets (prints) a config option */
	if( MATCH_CMD("getc ") || MATCH_CMD("getconfig ") ) {
		char *value = edit_get_config(edit, args);
		edit_set_status(edit, "'%s' = '%s'", args, value ? value : "(unset)");
		return;
	}

	edit_set_status(edit, "unknown command '%s'", cmd);
}

#undef MATCH_CMD

/* Matches a command string, returning the string after the command */
static char *_match_command(const char *cmd, const char *match, int len) {
	while( len-- && *cmd ) {
		if( *cmd++ != *match++ ) {
			return NULL;
		}
	}

	return (char *)cmd;
}

/* Updates the gutter size */
static void _update_gutter(Edit *edit) {
	size_t line_count = edit->file.length;

	edit->gutter = 1;
	do {
		++edit->gutter;
		line_count /= 10;
	} while( line_count != 0 );
}

/* Validates the cursor position */
static void _update_cursor_x(Edit *edit) {
	long s_length = edit_get_current_line_length(edit);
	if( s_length == -1 ) {
		edit->line = 0;
		edit->y = 0;
		edit->vy = 0;
		s_length = edit_get_current_line_length(edit);
	}

	const size_t length = (size_t)s_length;
	if( edit->idx >= (size_t)length ) {
		edit->idx = length;
	}

	edit->x = edit->idx + edit->gutter;
	move(edit->y, edit->x);
	refresh();
}

/* Moves the cursor to the start of the line */
static void _move_to_start_of_line(Edit *edit) {
	_move_to_idx(edit, 0);
}

/* Moves the cursor to the end of the line */
static void _move_to_end_of_line(Edit *edit) {
	Line *line = edit_get_current_line(edit);
	_move_to_idx(edit, line->length);
}

/* Moves the cursor to a given index in the line */
static void _move_to_idx(Edit *edit, size_t idx) {
	edit->idx = idx;
	_update_cursor_x(edit);
}

/* Moves the cursor to the start of the file */
static void _move_to_start_of_file(Edit *edit) {
	edit_goto(edit, 0);
	_move_to_start_of_line(edit);
}

/* Moves the cursor to the end of the file */
static void _move_to_end_of_file(Edit *edit) {
	edit_goto(edit, edit->file.length - 1);
	_move_to_start_of_line(edit);
}

/* Prompts the user to save the file */
static bool _ask_to_save(Edit *edit) {
	Prompt prompt;
	char *file = file_get_display_name(&edit->file);
	prompt_init(&prompt, PROMPT_YES_NO_CANCEL, "Save changes to '%s'?", file);

	switch( prompt_opt_get(&prompt) ) {
	case PROMPT_YES:
		edit_save(edit);
		break;
	case PROMPT_NO:
		break;
	case PROMPT_CANCEL:
		prompt_free(&prompt);
		return false;
	}

	prompt_free(&prompt);
	return true;
}

/* Adds a newline in the text */
static void _newline(Edit *edit) {
	file_break_line(&edit->file, edit->line++, edit->idx);
	++edit->y;

	_update_gutter(edit);

	edit->idx = 0;
	_update_cursor_x(edit);

	edit_render(edit);
}

/* Returns a string representing the current mode */
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
	case EDIT_MODE_REPLACE:
		return "REPLACE";
	default:
		fprintf(stderr, "Invalid mode no. '%d'!\n", edit->mode);
		exit(1);
	}
}
