/* edit
 * Syntax highlighting handling
 */

#include <stddef.h>

#include "global.h"

#include "config.h"

#include "syn.h"

void syn_init(Syn *syn) {
	for( size_t i = 0; i < COLP_MAX; ++i ) {
		SynRules *rule = &syn->rules[i];
		config_init(&rule->map);
	}
}

void syn_free(Syn *syn) {
	for( size_t i = 0; i < COLP_MAX; ++i ) {
		SynRules *rule = &syn->rules[i];
		config_free(&rule->map);
	}
}

void syn_read(Syn *syn, const char *filename) { }

void syn_update(Syn *syn, Line *line) { }
