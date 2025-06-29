#ifndef GUARD_EDIT_FILE_H_
#define GUARD_EDIT_FILE_H_

#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>

#include "line.h"
#include "config.h"

#define MAX_FILE_NAME_SIZE (256)

typedef struct _File {
	char name[MAX_FILE_NAME_SIZE]; /* File name */

	Line *lines; /* Lines in the file */
	size_t length; /* Number of lines in the line array */
	size_t capacity; /* Maximum capacity of the line array */

	Config config; /* Configuration */

	bool unnamed;
	bool dirty;
} File;

bool file_init(File *file, const char *filename);
void file_free(File *file);

bool file_load(File *file, const char *filename);
bool file_load_from_fp(File *file, FILE *fp);
bool file_save(File *file, const char *as);

void file_render(File *file, size_t from, int gutter);
void file_render_line(File *file, size_t idx, size_t from, int gutter);

void file_render_color(File *file, size_t from, int gutter);
void file_render_line_color(File *file, size_t idx, size_t from, int gutter);

void file_mark_dirty(File *file);
bool file_is_dirty(File *file);

char file_replace_char(File *file, size_t line, size_t idx, char ch);
void file_insert_char(File *file, size_t line, size_t idx, char ch);
char file_delete_char(File *file, size_t line, size_t idx);

void file_insert_string(File *file, size_t idx, char *str);

void file_break_line(File *file, size_t line, size_t idx);

void file_insert_empty_line(File *file, size_t idx);

void file_insert_line(File *file, size_t idx, Line *line);
void file_delete_line(File *file, size_t idx);

size_t file_move_line_up(File *file, size_t idx);

void file_shift_lines_up(File *file, size_t idx);
void file_shift_lines_down(File *file, size_t idx);

void file_set_extension(File *file, char *lang);

void file_set_config(File *file, char *key, char *value);
char *file_get_config(File *file, char *key);

Line *file_get_line(File *file, size_t idx);
long file_get_line_length(File *file, size_t idx);

char *file_get_name(File *file);
char *file_get_display_name(File *file);

#endif // !GUARD_EDIT_FILE_H_
