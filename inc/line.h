#ifndef GUARD_EDIT_LINE_H_
#define GUARD_EDIT_LINE_H_

#include <stdbool.h>
#include <stddef.h>

typedef struct _Line {
	char *text; /* Characters in the line */
	size_t length; /* Number of characters in the string */
	size_t capacity; /* Maximum capacity of the string */
} Line;

void line_new(Line *line);
void line_free(Line *line);

void line_erase(Line *line);

void line_render(Line *line);

void line_insert_char_at_end(Line *line, char c);
void line_delete_char_at_end(Line *line);

void line_insert_char(Line *line, size_t idx, char c);
void line_delete_char(Line *line, size_t idx);

void line_insert_str(Line *line, size_t idx, const char *str);
void line_delete_str(Line *line, size_t idx, size_t len);

char *line_copy(Line *line, size_t idx, long len, bool kill);

void line_shift_chars_forwards(Line *line, size_t idx, size_t by);
void line_shift_chars_backwards(Line *line, size_t idx, size_t by);

void line_clone(Line *from, Line *to, bool deep);

void line_null_terminate(Line *line);

char *line_get_c_str(Line *line);

#endif // !GUARD_EDIT_LINE_H_
