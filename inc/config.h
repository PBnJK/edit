#ifndef GUARD_EDIT_CONFIG_H_
#define GUARD_EDIT_CONFIG_H_

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define MAX_CONFIGS (1 << 8)
#define CONFIGS_MASK (MAX_CONFIGS - 1)

typedef struct _ConfigOption {
	char *key; /* Key (name) of this config */
	uint32_t hash; /* Hash of the key */
	char *value; /* Value of this config */

	struct _ConfigOption *next; /* Next item in linked list of configs */
} ConfigOption;

typedef struct _Config {
	size_t count; /* Number of configs */
	ConfigOption *values[MAX_CONFIGS]; /* Configs */
} Config;

void config_init(Config *config);
void config_free(Config *config);

bool config_set(Config *config, char *key, char *value);
char *config_get(Config *config, char *key);

bool config_remove(Config *config, char *key);

bool config_has(Config *config, char *key);

#endif // !GUARD_EDIT_CONFIG_H_
