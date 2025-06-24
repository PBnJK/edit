#ifndef GUARD_EDIT_PROMPT_H_
#define GUARD_EDIT_PROMPT_H_

#ifdef USE_PDCURSES
#include <curses.h>
#else
#include <ncurses.h>
#endif

#include <stdbool.h>

typedef enum _PromptType {
	PROMPT_YES_NO,
	PROMPT_YES_NO_CANCEL,
	PROMPT_STR,
} PromptType;

typedef enum _PromptOptResult {
	PROMPT_YES,
	PROMPT_NO,
	PROMPT_CANCEL,
} PromptOptResult;

typedef struct _Prompt {
	WINDOW *win;
	int w;
	int h;

	PromptType type;
} Prompt;

void prompt_init(Prompt *prompt, PromptType type, const char *msg, ...);

PromptOptResult prompt_opt_get(Prompt *prompt);
char *prompt_str_get(Prompt *prompt);

void prompt_free(Prompt *prompt);

#endif // !GUARD_EDIT_PROMPT_H_
