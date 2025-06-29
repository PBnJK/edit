/* edit
 * Line editing utilities
 */

#include <stddef.h>
#include <stdlib.h>
#include <string.h>

#ifdef USE_PDCURSES
#include <curses.h>
#else
#include <ncurses.h>
#endif

#include "global.h"

#include "line.h"

static void _grow_string(Line *line);
static void _grow_string_to(Line *line, size_t new_capacity);

static bool _is_string_full(Line *line);

static size_t _next_power_of_two(size_t n);

/* Creates a new empty line */
void line_init(Line *line) {
	size_t size = sizeof(*line->text) * 8;
	line->text = malloc(size);
	if( !line->text ) {
		fprintf(
			stderr, "Failed to allocate %zu bytes for text buffer!\n", size);
		exit(1);
	}

	line->capacity = 8;
	line->length = 0;
	memset(line->text, '\0', sizeof(*line->text) * line->capacity);

	line->color.data = NULL;
	line->color.capacity = 0;
	line->color.size = 0;
}

/* Frees the line from memory */
void line_free(Line *line) {
	free(line->text);
	line->text = NULL;
	line->length = 0;
	line->capacity = 0;
}

/* Zeroes out the line's contents */
void line_zero(Line *line) {
	line->text = NULL;
	line->length = 0;
	line->capacity = 0;
}

/* Erases a line's contents
 * Will resize its buffer
 */
void line_erase(Line *line) {
	memset(line->text, 0, line->capacity);

	line->capacity = 8;
	line->length = 0;
	line->text = realloc(line->text, line->capacity);
	if( line->text == NULL ) {
		fprintf(stderr, "Failed to reallocate %zu bytes text buffer!\n",
			line->capacity);
		exit(1);
	}
}

/* Renders the line's contents */
void line_render(Line *line) {
	clrtoeol();
	addnstr(line->text, line->length);
}

/* Renders the line's contents with color */
void line_render_color(Line *line) {
	clrtoeol();
	UNUSED(line);
}

/* Updates color data */
void line_update_color(Line *line) {
	UNUSED(line);
}

/* Replaces the character at @idx with @ch */
char line_replace_char(Line *line, size_t idx, char ch) {
	if( idx == line->length ) {
		line_insert_char(line, idx, ch);
		return '\0';
	}

	char prev = line->text[idx];
	line->text[idx] = ch;
	return prev;
}

/* Inserts a character at the end of the line */
void line_insert_char_at_end(Line *line, char ch) {
	line_insert_char(line, line->length, ch);
}

/* Deletes the character at the end of the line */
char line_delete_char_at_end(Line *line) {
	return line_delete_char(line, line->length);
}

/* Inserts the character @ch into column @idx */
void line_insert_char(Line *line, size_t idx, char ch) {
	if( idx < line->length ) {
		line_shift_chars_forwards(line, idx, 1);
	} else if( _is_string_full(line) ) {
		_grow_string(line);
		++line->length;
	} else {
		++line->length;
	}

	line->text[idx] = ch;
}

/* Deletes the character at column @idx */
char line_delete_char(Line *line, size_t idx) {
	if( idx == 0 ) {
		return '\0';
	}

	char prev = line->text[idx - 1];
	line_shift_chars_backwards(line, idx, 1);
	return prev;
}

/* Inserts a string @str into the line at index @idx */
void line_insert_str(Line *line, size_t idx, char *str) {
	size_t len = strlen(str);
	if( len > 0 && str[len - 1] == '\n' ) {
		str[--len] = '\0';
	}

	line_shift_chars_forwards(line, idx, len);
	memcpy(line->text + idx, str, len);
}

/* Copies @len characters starting at column @idx inclusive into a new buffer
 * If @len is <= 0, copies up to the end of the line instead
 * If @kill is true, also deletes the characters from the line
 */
char *line_copy(Line *line, size_t idx, long len, bool kill) {
	size_t u_len;
	if( len <= 0 ) {
		u_len = line->length - idx;
	} else {
		u_len = (size_t)len;
	}

	char *buf = malloc(u_len + 1);
	memcpy(buf, line->text + idx, u_len);
	buf[u_len] = '\0';

	if( kill ) {
		line_shift_chars_backwards(line, idx + u_len, u_len);
	}

	return buf;
}

/* Moves characters forwards by a given amount @by
 * Grows the lines array as needed
 */
void line_shift_chars_forwards(Line *line, size_t idx, size_t by) {
	size_t total = line->length + by;
	if( total >= line->capacity ) {
		total = total < 8 ? 8 : _next_power_of_two(total);
		_grow_string_to(line, total);
	}

	long s_idx = (long)idx;
	for( long i = (long)line->length; i >= s_idx; --i ) {
		line->text[i + by] = line->text[i];
		line->text[i] = ' ';
	}

	line->length += by;
}

/* Moves characters back by a given amount @by
 * Assumes that the text will not underrun the buffer
 */
void line_shift_chars_backwards(Line *line, size_t idx, size_t by) {
	size_t i = idx;
	do {
		line->text[i - by] = line->text[i];
		line->text[i] = '\0';
	} while( ++i < line->length );

	line->length -= by;
	if( line->length < (line->capacity >> 2) ) {
		_grow_string_to(line, line->capacity >> 2);
	}
}

/* Copies the contents of line @from into @to
 * If @deep is false, the two lines will share a text pointer
 * Otherwise, the text will be copied into a new buffer
 */
void line_clone(Line *from, Line *to, bool deep) {
	to->length = from->length;
	to->capacity = from->capacity;

	if( !deep ) {
		to->text = from->text;
	} else {
		to->text = malloc(to->capacity);
		if( !to->text ) {
			fprintf(stderr,
				"Failed to allocate %zu byte buffer for line clone!\n",
				to->capacity);
			exit(1);
		}

		strncpy(to->text, from->text, to->capacity);
	}
}

/* Adds a NUL character after the last character
 * May cause a reallocation
 */
void line_null_terminate(Line *line) {
	if( _is_string_full(line) ) {
		_grow_string(line);
	}

	line->text[line->length + 1] = '\0';
}

/* Returns the line text, ensuring it is NUL-terminated */
char *line_get_c_str(Line *line, bool clone) {
	if( line->length == 0 ) {
		return NULL;
	}

	if( clone ) {
		char *c_str = malloc(line->length + 1);
		strncpy(c_str, line->text, line->length);

		c_str[line->length] = '\0';

		return c_str;
	}

	line_null_terminate(line);
	return line->text;
}

/* Grows the string size */
static void _grow_string(Line *line) {
	const size_t new_capacity = (line->capacity < 8 ? 8 : line->capacity * 2);
	_grow_string_to(line, new_capacity);
}

/* Grows the string size to the given size */
static void _grow_string_to(Line *line, size_t new_capacity) {
	size_t size = sizeof(*line->text) * new_capacity;
	line->text = realloc(line->text, size);
	if( !line->text ) {
		fprintf(
			stderr, "Failed to reallocate %zu bytes for text buffer!\n", size);
		exit(1);
	}

	line->capacity = new_capacity;
}

/* Checks if the string is at capacity */
static bool _is_string_full(Line *line) {
	return line->length >= line->capacity - 1;
}

/* Returns the next power of two after @n
 * https://stackoverflow.com/a/12506181
 */
static size_t _next_power_of_two(size_t n) {
	size_t power = 1;
	while( power < n ) {
		power <<= 1;
	}

	return power;
}
