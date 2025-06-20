/* edit
 * Line editing utilities
 */

#include <stddef.h>
#include <stdlib.h>
#include <string.h>

#include <ncurses.h>

#include "line.h"

static void _grow_string(Line *line);
static void _grow_string_to(Line *line, size_t new_capacity);

static bool _is_string_full(Line *line);

static size_t _next_power_of_two(size_t n);

/* Creates a new empty line */
void line_new(Line *line) {
	line->text = malloc(sizeof(*line->text) * 8);
	if( !line->text ) {
		fprintf(stderr, "Failed to allocate text buffer!\n");
		exit(1);
	}

	memset(line->text, ' ', sizeof(*line->text));

	line->capacity = 8;
	line->length = 0;
}

/* Frees the line from memory */
void line_free(Line *line) {
	free(line->text);
	line->length = 0;
	line->capacity = 0;
}

/* Erases a line's contents
 * Will resize its buffer
 */
void line_erase(Line *line) {
	line->text = realloc(line->text, sizeof(*line->text) * 8);
	if( !line->text ) {
		fprintf(stderr, "Failed to reallocate text buffer!\n");
		exit(1);
	}

	memset(line->text, ' ', sizeof(*line->text));

	line->capacity = 8;
	line->length = 0;
}

/* Renders the line's contents */
void line_render(Line *line) {
	clrtoeol();
	addnstr(line->text, line->length);
}

/* Inserts the character @c into column @idx */
void line_insert_char(Line *line, size_t idx, char c) {
	if( idx < line->length ) {
		line_shift_chars_forwards(line, idx, 1);
	} else if( _is_string_full(line) ) {
		_grow_string(line);
		++line->length;
	} else {
		++line->length;
	}

	line->text[idx] = c;
}

/* Deletes the character at column @idx */
void line_delete_char(Line *line, size_t idx) {
	if( idx == 0 ) {
		exit(1); /* TODO: Panic more gracefully */
	}

	line_shift_chars_backwards(line, idx, 1);
}

void line_insert_str(Line *line, size_t idx, const char *str) {
	size_t len = strlen(str);
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

	char *buf = malloc(u_len);
	memcpy(buf, line->text + idx, u_len);

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
	for( size_t i = idx; i < line->length; ++i ) {
		line->text[i - by] = line->text[i];
		line->text[i] = ' ';
	}

	line->length -= by;
	if( line->length > 0 ) {
		_grow_string_to(line, _next_power_of_two(line->length));
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
			fprintf(stderr, "Failed to allocate buffer for line copy!\n");
			exit(1);
		}

		strncpy(to->text, from->text, to->capacity);
	}
}

static void _grow_string(Line *line) {
	const size_t new_capacity = (line->capacity < 8 ? 8 : line->capacity * 2);
	_grow_string_to(line, new_capacity);
}

static void _grow_string_to(Line *line, size_t new_capacity) {
	line->text = realloc(line->text, sizeof(*line->text) * new_capacity);
	if( !line->text ) {
		fprintf(stderr, "Failed to reallocate text buffer!\n");
		exit(1);
	}

	line->capacity = new_capacity;
}

static bool _is_string_full(Line *line) {
	return line->length >= line->capacity - 1;
}

/* https://stackoverflow.com/a/12506181 */
static size_t _next_power_of_two(size_t n) {
	size_t power = 1;
	while( power < n ) {
		power <<= 1;
	}

	return power;
}
