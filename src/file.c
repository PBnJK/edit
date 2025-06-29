/* edit
 * File handling utilities
 */

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

#include "line.h"
#include "prompt.h"
#include "config.h"

#include "file.h"

#define DEFAULT_FILE_NAME "unnamed"

static void _create_default_file(File *file);

static void _render(File *file, size_t from, int gutter, void (*fn)(Line *));
static void _render_line(
	File *file, size_t idx, size_t from, int gutter, void (*fn)(Line *));

static void _grow_line_array(File *file);
static void _grow_line_array_to(File *file, size_t new_capacity);

static bool _is_line_array_full(File *file);

static char *_ask_to_name(void);

/* Creates a new file */
bool file_init(File *file, const char *filename) {
	file->length = 0;
	file->capacity = 4;
	file->lines = malloc(sizeof(Line) * file->capacity);

	config_init(&file->config);

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

	file->dirty = false;
	file->unnamed = false;
}

/* Tries to load a file
 *
 * If it exists, loads its contents
 * Otherwise, "creates" a new empty file
 */
bool file_load(File *file, const char *filename) {
	if( filename == NULL ) {
		memset(file->name, 0, MAX_FILE_NAME_SIZE);
		_create_default_file(file);
		file->unnamed = true;
		file->dirty = false;
		return true;
	}

	file->unnamed = false;

	strncpy(file->name, filename, MAX_FILE_NAME_SIZE - 1);

	/* Get file extension */
	char *dot = strrchr(file->name, '.');
	if( dot && dot[1] ) {
		file_set_extension(file, dot + 1);
	}

	FILE *fp = fopen(filename, "r");
	if( fp ) {
		bool ok = file_load_from_fp(file, fp);
		fclose(fp);
		return ok;
	}

	file_insert_empty_line(file, 0);
	file->dirty = false;

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

	line_init(&line);

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
			line_init(&line);
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

/* Saves the file */
bool file_save(File *file, const char *as) {
	if( as ) {
		strncpy(file->name, as, MAX_FILE_NAME_SIZE - 1);
	} else if( file->unnamed ) {
		file->unnamed = false;

		char *new_name = _ask_to_name();
		if( !new_name ) {
			new_name = "unnamed";
		}

		strncpy(file->name, new_name, strlen(new_name));
	}

	FILE *fp = fopen(file->name, "w");
	if( fp == NULL ) {
		return false;
	}

	/* Saves the lines to a file */
	for( size_t i = 0; i < file->length; ++i ) {
		Line *line = file_get_line(file, i);
		fwrite(line->text, sizeof(*line->text), line->length, fp);
		fputc('\n', fp);
	}

	fclose(fp);

	file->dirty = false;

	return true;
}

/* Renders the file's contents */
void file_render(File *file, size_t from, int gutter) {
	_render(file, from, gutter, line_render);
}

/* Renders a single line from the file */
void file_render_line(File *file, size_t idx, size_t from, int gutter) {
	_render_line(file, idx, from, gutter, line_render);
}

/* Renders the file's contents, with color */
void file_render_color(File *file, size_t from, int gutter) {
	_render(file, from, gutter, line_render_color);
}

/* Renders a single line from the file, with color */
void file_render_line_color(File *file, size_t idx, size_t from, int gutter) {
	_render_line(file, idx, from, gutter, line_render_color);
}

/* Marks a file as "dirty" (modified) */
void file_mark_dirty(File *file) {
	file->dirty = true;
}

/* Returns if a file is "dirty" (modified) */
bool file_is_dirty(File *file) {
	return file->dirty;
}

/* Replaces a character in the file by @ch directly */
char file_replace_char(File *file, size_t line, size_t idx, char ch) {
	file_mark_dirty(file);
	return line_replace_char(&file->lines[line], idx, ch);
}

/* Inserts a character into a line in the file */
void file_insert_char(File *file, size_t line, size_t idx, char ch) {
	file_mark_dirty(file);
	line_insert_char(&file->lines[line], idx, ch);
}

/* Deletes a character from a line in the file */
char file_delete_char(File *file, size_t line, size_t idx) {
	file_mark_dirty(file);
	return line_delete_char(&file->lines[line], idx);
}

/* Inserts a string
 * Sames as creating a line and calling @file_insert_line with it
 */
void file_insert_string(File *file, size_t idx, char *str) {
	Line line;
	line_init(&line);

	line_insert_str(&line, 0, str);
	file_insert_line(file, idx, &line);
}

/* Inserts a line break into the file */
void file_break_line(File *file, size_t line, size_t idx) {
	Line new_line;
	line_init(&new_line);

	file_mark_dirty(file);

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
	line_init(&line);

	file_insert_line(file, idx, &line);
}

/* Adds a new line to the file */
void file_insert_line(File *file, size_t idx, Line *line) {
	file_shift_lines_down(file, idx);
	line_clone(line, &file->lines[idx], false);

	file_mark_dirty(file);

	++file->length;
}

/* Moves a line up, appending to the previous one if necessary */
size_t file_move_line_up(File *file, size_t idx) {
	if( idx == 0 ) {
		return 0;
	}

	Line *prev = file_get_line(file, idx - 1);
	const size_t prev_length = prev->length;

	if( prev_length > 0 ) {
		Line *line = file_get_line(file, idx);
		char *text = line_get_c_str(line, false);

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

	file_mark_dirty(file);

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

	file_mark_dirty(file);

	if( _is_line_array_full(file) ) {
		_grow_line_array(file);
	}

	long i;
	long s_idx = (long)idx;
	for( i = (long)file->length - 1; i >= s_idx; --i ) {
		file->lines[i + 1] = file->lines[i];
	}

	line_init(&file->lines[idx]);
}

/* Sets the extension of the file */
void file_set_extension(File *file, char *lang) {
	file_set_config(file, "ext", lang);
}

/* Sets a config option */
void file_set_config(File *file, char *key, char *value) {
	config_set(&file->config, key, value);
}

/* Gets a config option */
char *file_get_config(File *file, char *key) {
	return config_get(&file->config, key);
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

/* Returns the name of the file */
char *file_get_name(File *file) {
	if( !file->unnamed && *file->name ) {
		return file->name;
	}

	return NULL;
}

/* Returns the display safe name of the file */
char *file_get_display_name(File *file) {
	char *name = file_get_name(file);
	return (name ? name : "(unnamed)");
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

/* Creates the default file */
static void _create_default_file(File *file) {
	const size_t LEN = (sizeof(DEFAULT_FILE) / sizeof(*DEFAULT_FILE));
	for( size_t i = 0; i < LEN; ++i ) {
		file_insert_string(file, i, DEFAULT_FILE[i]);
	}
}

/* Renders the file's contents, calling @fn on each line */
static void _render(File *file, size_t from, int gutter, void (*fn)(Line *)) {
	const size_t maxy = getmaxy(stdscr) - 3;

	for( size_t y = 0; y < maxy && y < file->length - from; ++y ) {
		move(y, 0);

		const size_t offset = y + from;
		printw("%-*zu", gutter, offset + 1);
		fn(&file->lines[offset]);
	}
}

/* Renders a single line in the file using @fn */
static void _render_line(
	File *file, size_t idx, size_t from, int gutter, void (*fn)(Line *)) {
	const size_t offset = idx + from;

	move(idx, 0);
	printw("%-*zu", gutter, offset + 1);
	fn(&file->lines[offset]);
}

/* Grows the array of lines */
static void _grow_line_array(File *file) {
	const size_t new_capacity = (file->capacity < 4 ? 4 : file->capacity * 2);
	_grow_line_array_to(file, new_capacity);
}

/* Grows the array of lines to the given size */
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

/* Checks if the array of lines is full */
static bool _is_line_array_full(File *file) {
	return file->length >= file->capacity;
}

/* Asks the user to give a file name */
static char *_ask_to_name(void) {
	Prompt prompt;
	prompt_init(&prompt, PROMPT_STR,
		"Name the file (default '" DEFAULT_FILE_NAME "'):");

	return prompt_str_get(&prompt);
}
