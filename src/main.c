/* edit
 * Entry-point
 */

#include <stdlib.h>
#include <time.h>

#include <ncurses.h>

#include "global.h"

static void _init(void);

int main(int argc, char *argv[]) {
	UNUSED(argc);
	UNUSED(argv);

	_init();
	return EXIT_SUCCESS;
}

/* Initializes ncurses */
static void _init(void) {
	srand(time(NULL));

	initscr();

	/* Set up ncurses to:
	 * 1. Immediately return characters without waiting for a newline
	 * 2. Pass signals (SIGINT, SIGKILL, etc.) to us
	 * 3. Not echo characters to the screen
	 * 4. Read keypad input
	 */
	cbreak();
	raw();
	noecho();
	keypad(stdscr, true);

	curs_set(0);

	start_color();
	init_pair(COLP_RED, COLOR_RED, COLOR_BLACK);
	init_pair(COLP_GREEN, COLOR_GREEN, COLOR_BLACK);
	init_pair(COLP_YELLOW, COLOR_YELLOW, COLOR_BLACK);
	init_pair(COLP_BLUE, COLOR_BLUE, COLOR_BLACK);
	init_pair(COLP_MAGENTA, COLOR_MAGENTA, COLOR_BLACK);
	init_pair(COLP_CYAN, COLOR_CYAN, COLOR_BLACK);
	init_pair(COLP_BLACK, COLOR_BLACK, COLOR_WHITE);

	refresh();
}
