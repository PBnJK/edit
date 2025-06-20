#ifndef GUARD_EDIT_EDIT_H_
#define GUARD_EDIT_EDIT_H_

#include "file.h"
#include <stdbool.h>
#include <stddef.h>

typedef struct _Edit {
	File file; /* Current file */
	size_t line; /* Current line */
	size_t idx; /* Current character */
	size_t x, y; /* Current cursor position (global to file) */
} Edit;

void edit_new(Edit *edit, const char *filename);
void edit_free(Edit *edit);

void edit_update(Edit *edit);
void edit_render(Edit *edit);

void edit_insert_char(Edit *edit, char c);
void edit_delete_char(Edit *edit);

void edit_move_up(Edit *edit);
void edit_move_down(Edit *edit);
void edit_move_left(Edit *edit);
void edit_move_right(Edit *edit);

void edit_quit(void);

#endif // !GUARD_EDIT_EDIT_H_
