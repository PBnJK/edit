/* edit
 * Entry-point
 */

#if defined(__linux__) || defined(__APPLE__)
#include <signal.h>
#elif defined(_WIN32)
#include <windows.h>
#include <stdio.h>
#endif

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#ifdef USE_PDCURSES
#include <curses.h>
#else
#include <ncurses.h>
#endif

#include "global.h"

#include "edit.h"

static Edit edit;

static bool _register_signal_handlers(void);

static void _init_ncurses(void);
static void _cleanup(void);

int main(int argc, char *argv[]) {
	char *initial = NULL;
	if( argc > 1 ) {
		initial = argv[1];
	}

	_init_ncurses();

	if( !_register_signal_handlers() ) {
		return 1;
	}

	edit_init(&edit, initial);
	while( edit.running ) {
		edit_update(&edit);
	}

	_cleanup();

	return EXIT_SUCCESS;
}

#if defined(__linux__) || defined(__APPLE__)
void _unix_signal_handler(int sig) {
	if( sig == SIGINT || sig == SIGTERM ) {
		edit.running = false;
	}
}
#elif defined(_WIN32)
BOOL WINAPI _win_signal_handler(DWORD sig) {
	if( sig == CTRL_C_EVENT || sig == CTRL_CLOSE_EVENT ) {
		edit.running = false;
		return TRUE;
	}

	return FALSE;
}
#endif

/* Registers handlers for certain signals */
static bool _register_signal_handlers(void) {
#if defined(__linux__) || defined(__APPLE__)
	struct sigaction act = { 0 };
	act.sa_handler = _unix_signal_handler;
	act.sa_flags = 0;
	sigemptyset(&act.sa_mask);

	if( sigaction(SIGINT, &act, NULL) == -1 ) {
		return false;
	}

	if( sigaction(SIGTERM, &act, NULL) == -1 ) {
		return false;
	}

	return true;
#elif defined(_WIN32)
	return SetConsoleCtrlHandler(_win_signal_handler, TRUE) == 0 ? true : false;
#endif
}

/* Initializes ncurses */
static void _init_ncurses(void) {
	srand(time(NULL));

	initscr();

	/* Set up ncurses to:
	 * 1. Immediately return characters without waiting for a newline
	 * 2. Not echo characters to the screen
	 * 3. Read keypad input
	 */
	cbreak();
	noecho();
	keypad(stdscr, true);

	curs_set(1);

	/* Initializes color pairs */
	if( has_colors() ) {
		start_color();
		init_pair(COLP_RED, COLOR_RED, COLOR_BLACK);
		init_pair(COLP_GREEN, COLOR_GREEN, COLOR_BLACK);
		init_pair(COLP_YELLOW, COLOR_YELLOW, COLOR_BLACK);
		init_pair(COLP_BLUE, COLOR_BLUE, COLOR_BLACK);
		init_pair(COLP_MAGENTA, COLOR_MAGENTA, COLOR_BLACK);
		init_pair(COLP_CYAN, COLOR_CYAN, COLOR_BLACK);
		init_pair(COLP_BLACK, COLOR_BLACK, COLOR_WHITE);
	}

	refresh();
}

/* Cleans up the program */
static void _cleanup(void) {
	edit_quit(&edit);
	endwin();
}
