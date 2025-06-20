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

void line_free(Line *line) {
	free(line->text);
	line->length = 0;
	line->capacity = 0;
}

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

void line_render(Line *line) {
	clrtoeol();
	addnstr(line->text, line->length);
}

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

/* Moves characters forwards by a given amount @by
 * Grows the lines array as needed
 */
void line_shift_chars_forwards(Line *line, size_t idx, size_t by) {
	size_t total = line->length + by;
	if( total >= line->capacity ) {
		total = total < 8 ? 8 : _next_power_of_two(total);
		_grow_string_to(line, total);
	}

	for( size_t i = line->length; i >= idx; --i ) {
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
}

void line_copy(Line *from, Line *to, bool deep) {
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
