#ifndef GUARD_EDIT_GLOBAL_H_
#define GUARD_EDIT_GLOBAL_H_

#define MAX(A, B) (((A) > (B)) ? (A) : (B))
#define MIN(A, B) (((A) < (B)) ? (A) : (B))

/* Color pairs */
typedef enum _ColorPair {
	COLP_NONE = 0,
	COLP_RED = 1,
	COLP_GREEN = 2,
	COLP_YELLOW = 3,
	COLP_BLUE = 4,
	COLP_MAGENTA = 5,
	COLP_CYAN = 6,
	COLP_BLACK = 7,
	COLP_MAX = 8,
} ColorPair;

/* Ctrl + Key */
#define CTRL(K) ((K) & 0x1f)

/* "Mark" a variable as unused */
#define UNUSED(V) ((void)(V))

#endif // !GUARD_EDIT_GLOBAL_H_
