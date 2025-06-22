#ifndef GUARD_EDIT_EDIT_H_
#define GUARD_EDIT_EDIT_H_

#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>

#include "cmd.h"
#include "file.h"
#include "line.h"

#define STATUS_MSG_LEN (60)

/* Editor modes */
typedef enum _Mode {
	EDIT_MODE_NORMAL,
	EDIT_MODE_INSERT,
	EDIT_MODE_REPLACE,
	EDIT_MODE_VISUAL,
	EDIT_MODE_COMMAND,
} Mode;

typedef struct _Edit {
	File file; /* Current file */

	size_t line; /* Current line */
	size_t idx; /* Current character */
	size_t x, y; /* Current cursor position in the terminal */
	size_t vx, vy; /* "Viewport" */

	size_t w, h; /* Terminal dimensions */
	size_t gutter; /* Gutter size */

	char msg[STATUS_MSG_LEN]; /* Status message */
	int msg_len; /* Cached status message length */

	bool running;

	Mode mode; /* Current editor mode */

	Line cmd; /* Normal mode command buffer */
	size_t num_arg; /* Numerical argument to command */

	size_t vis_start_line; /* Starting line of selection */
	size_t vis_start_idx; /* Starting index of selection */
	size_t vis_length; /* Length of the selection */

	CommandStack undo; /* Undo stack */
	CommandStack redo; /* Redo stack */
} Edit;

void edit_new(Edit *edit, const char *filename);
void edit_free(Edit *edit);

void edit_load(Edit *edit, const char *filename);
void edit_save(Edit *edit);

void edit_update(Edit *edit);
void edit_refresh(Edit *edit);

void edit_change_to_normal(Edit *edit);
void edit_change_to_insert(Edit *edit);
void edit_change_to_replace(Edit *edit);
void edit_change_to_visual(Edit *edit);
void edit_change_to_command(Edit *edit);

void edit_mode_normal(Edit *edit, int ch);
void edit_mode_insert(Edit *edit, int ch);
void edit_mode_replace(Edit *edit, int ch);
void edit_mode_visual(Edit *edit, int ch);
void edit_mode_command(Edit *edit, int ch);

void edit_rep_ch(Edit *edit, CommandStack *stack, char ch);
void edit_add_ch(Edit *edit, CommandStack *stack, char ch);
void edit_del_ch(Edit *edit, CommandStack *stack, char ch);

void edit_new_line(Edit *edit, CommandStack *stack);
void edit_add_line(Edit *edit, CommandStack *stack, Line l);
void edit_del_line(Edit *edit, CommandStack *stack, Line l);

void edit_undo(Edit *edit);
void edit_redo(Edit *edit);

void edit_perform_cmd(Edit *edit, CommandStack *stack, Command *cmd);

void edit_yank(Edit *edit, char into, bool kill);
void edit_paste(Edit *edit, char from);

void edit_goto(Edit *edit, size_t idx);

void edit_replace_char(Edit *edit, CommandStack *stack, char ch);
void edit_insert_char(Edit *edit, CommandStack *stack, char ch);
void edit_delete_char(Edit *edit, CommandStack *stack);

void edit_move_up(Edit *edit);
void edit_move_down(Edit *edit);
void edit_move_left(Edit *edit);
void edit_move_right(Edit *edit);

void edit_set_status(Edit *edit, const char *fmt, ...);
void edit_render_status(Edit *edit);

void edit_render(Edit *edit);
void edit_render_current_line(Edit *edit);
void edit_render_line(Edit *edit, size_t idx);

void edit_quit(Edit *edit);

void edit_save_file(Edit *edit);

Line *edit_get_current_line(Edit *edit);
Line *edit_get_line(Edit *edit, size_t idx);

long edit_get_current_line_length(Edit *edit);
long edit_get_line_length(Edit *edit, size_t idx);

size_t edit_get_ui_offset(Edit *edit);

#endif // !GUARD_EDIT_EDIT_H_
