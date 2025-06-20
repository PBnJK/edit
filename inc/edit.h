#ifndef GUARD_EDIT_EDIT_H_
#define GUARD_EDIT_EDIT_H_

#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>

#include "file.h"
#include "line.h"

#define STATUS_MSG_LEN (40)

typedef enum _Mode {
	EDIT_MODE_NORMAL,
	EDIT_MODE_INSERT,
	EDIT_MODE_VISUAL,
} Mode;

typedef struct _Edit {
	File file; /* Current file */

	size_t line; /* Current line */
	size_t idx; /* Current character */
	size_t x, y; /* Current cursor position (global to file) */

	size_t w, h; /* Terminal dimensions */

	char msg[STATUS_MSG_LEN]; /* Status message */
	int msg_len; /* Cached status message length */

	bool running;

	Mode mode; /* Current editor mode */

	bool is_typing_cmd; /* If the user is currently typing a command */
	Line cmd; /* Normal mode command buffer */
} Edit;

void edit_new(Edit *edit, const char *filename);
void edit_free(Edit *edit);

void edit_update(Edit *edit);

void edit_mode_normal(Edit *edit, int ch);
void edit_mode_insert(Edit *edit, int ch);
void edit_mode_visual(Edit *edit, int ch);

void edit_insert_char(Edit *edit, char c);
void edit_delete_char(Edit *edit);

void edit_move_up(Edit *edit);
void edit_move_down(Edit *edit);
void edit_move_left(Edit *edit);
void edit_move_right(Edit *edit);

void edit_set_status(Edit *edit, const char *msg);
void edit_render_status(Edit *edit);

Line *edit_get_current_line(Edit *edit);
Line *edit_get_line(Edit *edit, size_t idx);

long edit_get_current_line_length(Edit *edit);
long edit_get_line_length(Edit *edit, size_t idx);

void edit_render(Edit *edit);
void edit_render_current_line(Edit *edit);
void edit_render_line(Edit *edit, size_t idx);

void edit_quit(Edit *edit);

#endif // !GUARD_EDIT_EDIT_H_
