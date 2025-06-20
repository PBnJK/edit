/* edit
 * File handling utilities
 */

#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <ncurses.h>

#include "line.h"

#include "file.h"

static void _grow_line_array(File *file);
static void _grow_line_array_to(File *file, size_t new_capacity);

static bool _is_line_array_full(File *file);

bool file_load(File *file, const char *filename) {
	strncpy(file->name, filename, MAX_FILE_NAME_SIZE - 1);

	/* Initialize line array */
	file->capacity = 0;
	file->length = 0;
	_grow_line_array(file);

	FILE *fp = fopen(filename, "r");
	if( fp ) {
		return file_load_from_fp(file, fp);
	}

	file_insert_empty_line(file, 0);

	return true;
}

/* Loads a file incrementally from a file pointer */
bool file_load_from_fp(File *file, FILE *fp) {
	char buf[BUFSIZ];
	size_t line_idx = 0, char_idx = 0;
	Line line;

	/* Watchdog to check if the entire buffer was read */
	memset(buf, 0, BUFSIZ);
	buf[BUFSIZ - 1] = 0x55;

	line_new(&line);

	while( fgets(buf, BUFSIZ, fp) ) {
		if( feof(fp) ) {
			line_insert_str(&line, char_idx, buf);
			break;
		}

		/* If true, we reached a newline, and can finish this line
		 * Otherwise, the entire buffer was filled, and there's more to read
		 */
		if( buf[BUFSIZ - 1] == 0x55 ) {
			line_insert_str(&line, char_idx, buf);
			char_idx = 0;

			file_insert_line(file, line_idx++, &line);
			line_new(&line);
		} else {
			char_idx += BUFSIZ - 1;
		}
	}

	if( !feof(fp) ) {
		return false;
	}

	return true;
}

void file_render(File *file) {
	for( size_t y = 0; y < file->length; ++y ) {
		line_render(&file->lines[y]);
		move(y, 0);
	}
}

void file_insert_char(File *file, size_t line, size_t idx, char c) {
	line_insert_char(&file->lines[line], idx, c);
}

void file_delete_char(File *file, size_t line, size_t idx) {
	line_delete_char(&file->lines[line], idx);
}

void file_insert_empty_line(File *file, size_t idx) {
	Line line;
	line_new(&line);

	file_insert_line(file, idx, &line);
}

void file_insert_line(File *file, size_t idx, Line *line) {
	file_shift_lines_down(file, idx);
	line_copy(line, &file->lines[idx], false);

	++file->length;
}

void file_shift_lines_down(File *file, size_t idx) {
	size_t i;

	if( _is_line_array_full(file) ) {
		_grow_line_array(file);
	}

	for( i = file->length; i >= idx; --i ) {
		file->lines[i + 1] = file->lines[i];

		if( i == 0 ) {
			break;
		}
	}

	line_erase(&file->lines[i]);
}

static void _grow_line_array(File *file) {
	const size_t new_capacity = (file->capacity == 0 ? 4 : file->capacity * 2);
	_grow_line_array_to(file, new_capacity);
}

static void _grow_line_array_to(File *file, size_t new_capacity) {
	file->lines = realloc(file->lines, sizeof(*file->lines) * new_capacity);
	file->capacity = new_capacity;
}

static bool _is_line_array_full(File *file) {
	return file->length >= file->capacity;
}
