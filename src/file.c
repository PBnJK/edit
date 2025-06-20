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

static void _create_default_file(File *file);

static void _grow_line_array(File *file);
static void _grow_line_array_to(File *file, size_t new_capacity);

static bool _is_line_array_full(File *file);

/* Creates a new file */
bool file_new(File *file, const char *filename) {
	file->length = 0;
	file->capacity = 4;
	file->lines = malloc(sizeof(Line) * file->capacity);

	return file_load(file, filename);
}

/* Frees a file from memory */
void file_free(File *file) {
	memset(file->name, 0, MAX_FILE_NAME_SIZE);

	for( size_t i = 0; i < file->length; ++i ) {
		line_free(&file->lines[i]);
	}

	free(file->lines);
	file->lines = NULL;

	file->length = 0;
	file->capacity = 0;
}

/* Tries to load a file
 * If it exists, loads its contents
 * Otherwise, "creates" a new empty file
 */
bool file_load(File *file, const char *filename) {
	if( filename == NULL ) {
		_create_default_file(file);
		return true;
	}

	strncpy(file->name, filename, MAX_FILE_NAME_SIZE - 1);

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
		line_insert_str(&line, char_idx, buf);

		if( feof(fp) ) {
			file_insert_line(file, line_idx++, &line);
			break;
		}

		/* If true, we reached a newline, and can finish this line
		 * Otherwise, the entire buffer was filled, and there's more to read
		 */
		if( buf[BUFSIZ - 1] == 0x55 ) {
			char_idx = 0;

			file_insert_line(file, line_idx++, &line);
			line_new(&line);
		} else {
			buf[BUFSIZ - 1] = 0x55;
			char_idx += BUFSIZ - 1;
		}
	}

	if( !feof(fp) ) {
		return false;
	}

	return true;
}

/* Renders the file's contents */
void file_render(File *file, size_t from, int gutter) {
	const size_t maxy = getmaxy(stdscr) - 3;

	for( size_t y = 0; y < maxy && y < file->length - from; ++y ) {
		move(y, 0);

		size_t offset = y + from;
		printw("%-*zu", gutter, offset + 1);
		line_render(&file->lines[offset]);
	}
}

/* Renders a single line from the file */
void file_render_line(File *file, size_t idx, int gutter) {
	move(idx, 0);
	printw("%-*zu", gutter, idx + 1);
	line_render(&file->lines[idx]);
}

/* Inserts a character into a line in the file */
void file_insert_char(File *file, size_t line, size_t idx, char c) {
	line_insert_char(&file->lines[line], idx, c);
}

/* Deletes a character from a line in the file */
void file_delete_char(File *file, size_t line, size_t idx) {
	line_delete_char(&file->lines[line], idx);
}

/* Inserts a string
 * Sames as creating a line and calling @file_insert_line with it
 */
void file_insert_string(File *file, size_t idx, char *str) {
	Line line;
	line_new(&line);

	line_insert_str(&line, 0, str);
	file_insert_line(file, idx, &line);
}

/* Inserts a line break into the file */
void file_break_line(File *file, size_t line, size_t idx) {
	Line new_line;
	line_new(&new_line);

	/* If we're at the beginning of the line, insert the empty line here */
	if( idx == 0 ) {
		file_insert_line(file, line, &new_line);
		return;
	}

	Line *curr_line = file_get_line(file, line);
	if( !curr_line ) {
		fprintf(stderr, "Current line is null!");
		exit(1);
	}

	if( idx > curr_line->length ) {
		fprintf(stderr, "Cursor is beyond the line's limits!");
		exit(1);
	}

	/* If we're at the end of the line, insert the empty line on the next row */
	if( idx == curr_line->length ) {
		file_insert_line(file, line + 1, &new_line);
		return;
	}

	char *buf = line_copy(curr_line, idx, -1, true);
	line_insert_str(&new_line, 0, buf);
	free(buf);

	file_insert_line(file, line + 1, &new_line);
}

/* Adds a new empty line to the file
 * Same as calling @file_insert_line with an empty line
 */
void file_insert_empty_line(File *file, size_t idx) {
	Line line;
	line_new(&line);

	file_insert_line(file, idx, &line);
}

/* Adds a new line to the file */
void file_insert_line(File *file, size_t idx, Line *line) {
	file_shift_lines_down(file, idx);
	line_clone(line, &file->lines[idx], false);

	++file->length;
}

size_t file_move_line_up(File *file, size_t idx) {
	if( idx == 0 ) {
		return 0;
	}

	Line *prev = file_get_line(file, idx - 1);
	const size_t prev_length = prev->length;

	if( prev_length > 0 ) {
		Line *line = file_get_line(file, idx);
		char *text = line_get_c_str(line);

		line_insert_str(prev, prev->length, text);
		line_free(line);

		file_shift_lines_up(file, idx + 1);
	} else {
		line_free(prev);
		file_shift_lines_up(file, idx);
	}

	return prev_length;
}

/* Shifts lines starting at @idx one row up */
void file_shift_lines_up(File *file, size_t idx) {
	if( file->length == 0 ) {
		return;
	}

	size_t i;
	for( i = idx; i < file->length; ++i ) {
		file->lines[i - 1] = file->lines[i];
	}

	/* The previous line has a pointer to text so this isn't a leak */
	line_zero(&file->lines[--file->length]);
}

/* Shifts lines starting at @idx one row down */
void file_shift_lines_down(File *file, size_t idx) {
	if( file->length == 0 ) {
		return;
	}

	if( _is_line_array_full(file) ) {
		_grow_line_array(file);
	}

	long i;
	long s_idx = (long)idx;
	for( i = (long)file->length - 1; i >= s_idx; --i ) {
		file->lines[i + 1] = file->lines[i];
	}

	line_new(&file->lines[idx]);
}

/* Returns the line at @idx */
Line *file_get_line(File *file, size_t idx) {
	if( idx >= file->length ) {
		return NULL;
	}

	return &file->lines[idx];
}

/* Returns the length of line @idx */
long file_get_line_length(File *file, size_t idx) {
	Line *line = file_get_line(file, idx);
	if( line ) {
		return line->length;
	}

	return -1;
}

#ifndef __DATE__
#define __DATE__ "made with love <3"
#endif

static char *DEFAULT_FILE[] = {
	"            .-.   .-.   .-.",
	"            | |   *-* .-* *-.",
	".-----. .---* | .---. *-. .-*",
	"| .-- | | .-. | *-. |   | |",
	"| *---| | *-* | .-* *-. | .-.",
	"*-----* *-----* *-----* .---*",
	"",
	"a file editor by pedrob",
	__DATE__,
};

static void _create_default_file(File *file) {
	const size_t LEN = (sizeof(DEFAULT_FILE) / sizeof(*DEFAULT_FILE));
	for( size_t i = 0; i < LEN; ++i ) {
		file_insert_string(file, i, DEFAULT_FILE[i]);
	}
}

static void _grow_line_array(File *file) {
	const size_t new_capacity = (file->capacity < 4 ? 4 : file->capacity * 2);
	_grow_line_array_to(file, new_capacity);
}

static void _grow_line_array_to(File *file, size_t new_capacity) {
	size_t size = sizeof(*file->lines) * new_capacity;
	Line *new_lines = realloc(file->lines, size);
	if( !new_lines ) {
		fprintf(stderr, "Failed to reallocate %zu bytes for file!\n", size);
		exit(1);
	}

	file->lines = new_lines;
	file->capacity = new_capacity;
}

static bool _is_line_array_full(File *file) {
	return file->length >= file->capacity;
}
