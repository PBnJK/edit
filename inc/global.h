#ifndef GUARD_EDIT_GLOBAL_H_
#define GUARD_EDIT_GLOBAL_H_

#define MAX(A, B) (((A) > (B)) ? (A) : (B))
#define MIN(A, B) (((A) < (B)) ? (A) : (B))

/* Color pairs */
#define COLP_RED (1)
#define COLP_GREEN (2)
#define COLP_YELLOW (3)
#define COLP_BLUE (4)
#define COLP_MAGENTA (5)
#define COLP_CYAN (6)
#define COLP_BLACK (7)

/* Ctrl + Key */
#define CTRL(K) ((K) & 0x1f)

/* "Mark" a variable as unused */
#define UNUSED(V) ((void)(V))

#endif // !GUARD_EDIT_GLOBAL_H_
