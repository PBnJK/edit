#ifndef GUARD_EDIT_SYN_H_
#define GUARD_EDIT_SYN_H_

#include "global.h"

#include "config.h"
#include "line.h"

typedef struct _SynRules {
	Config map;
} SynRules;

typedef struct _Syn {
	SynRules rules[COLP_MAX];
} Syn;

void syn_init(Syn *syn);
void syn_free(Syn *syn);

void syn_read(Syn *syn, const char *filename);

void syn_add_rule(Syn *syn, char *name);

void syn_update(Syn *syn, Line *line);

#endif // !GUARD_EDIT_SYN_H_
