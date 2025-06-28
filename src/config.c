/* edit
 * Configuration options
 */

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "config.h"

#define FNV_PRIME (0x01000193)
#define FNV_OFFSET_BASIS (0x811c9dc5)

static ConfigOption *_init_option(char *key, uint32_t hash, char *value);
static void _free_option(ConfigOption *opt);
static void _free_option_recursively(ConfigOption *opt);

static uint32_t _hash_string(char *str, size_t len);

static bool _check_match(ConfigOption *opt, char *key, uint32_t hash);

/* Initializes a new config */
void config_init(Config *config) {
	for( size_t i = 0; i < MAX_CONFIGS; ++i ) {
		config->values[i] = NULL;
	}

	config->count = 0;
}

/* Frees a config from memory */
void config_free(Config *config) {
	for( size_t i = 0; i < MAX_CONFIGS; ++i ) {
		_free_option_recursively(config->values[i]);
		config->values[i] = NULL;
	}

	config->count = 0;
}

/* Inserts or updates an option in the configuration
 *
 * If an item with the key @key existed, updates it and returns false
 * Otherwise, returns true
 */
bool config_set(Config *config, char *key, char *value) {
	/* Hashes string and constricts to the maximum value */
	uint32_t hash = _hash_string(key, strlen(key));
	uint32_t idx = hash & CONFIGS_MASK;

	ConfigOption *opt = config->values[idx];

	/* If NULL, this is a new item we're inserting */
	if( opt == NULL ) {
		config->values[idx] = _init_option(key, hash, value);
		++config->count;
		return true;
	}

	while( true ) {
		/* If there's a match, update its value */
		if( _check_match(opt, key, hash) ) {
			opt->value = value;
			return false;
		}

		/* If there's no next item, create it */
		if( opt->next == NULL ) {
			opt->next = _init_option(key, hash, value);
			++config->count;
			return true;
		}

		opt = opt->next;
	}
}

/* Gets the value of an item in the configuration */
char *config_get(Config *config, char *key) {
	if( config->count == 0 ) {
		return NULL;
	}

	/* Hashes string and constricts to the maximum value */
	uint32_t hash = _hash_string(key, strlen(key));
	uint32_t idx = hash & CONFIGS_MASK;

	ConfigOption *opt = config->values[idx];

	/* If NULL, this item does not exist */
	if( opt == NULL ) {
		return NULL;
	}

	while( true ) {
		/* If there's a match, return it */
		if( _check_match(opt, key, hash) ) {
			return opt->value;
		}

		/* If there's no next item, the item does not exist */
		if( opt->next == NULL ) {
			return NULL;
		}

		opt = opt->next;
	}
}

/* Removes an item from the configuration
 *
 * If an item with the key @key existed, removes it and returns true
 * Otherwise, returns false
 */
bool config_remove(Config *config, char *key) {
	/* If the list is empty, exit early */
	if( config->count == 0 ) {
		return false;
	}

	/* Hashes string and constricts to the maximum value */
	uint32_t hash = _hash_string(key, strlen(key));
	uint32_t idx = hash & CONFIGS_MASK;

	ConfigOption *opt = config->values[idx];

	/* If NULL, this item does not exist */
	if( opt == NULL ) {
		return false;
	}

	/* If it's a match, then remove it
	 * This is a special case for if the first item is the match
	 */
	if( _check_match(opt, key, hash) ) {
		_free_option(opt);
		config->values[idx] = NULL;
		--config->count;
		return true;
	}

	while( true ) {
		ConfigOption *prev = opt;
		opt = opt->next;

		/* If there's no next item, the item doesn't exist */
		if( opt->next == NULL ) {
			return false;
		}

		/* If it's a match, remove it */
		if( _check_match(opt, key, hash) ) {
			_free_option(opt);
			prev->next = NULL;
			--config->count;
			return true;
		}
	}
}

/* Returns whether the config has an item with key @key */
bool config_has(Config *config, char *key) {
	if( config->count == 0 ) {
		return false;
	}

	/* Hashes string and constricts to the maximum value */
	uint32_t hash = _hash_string(key, strlen(key));
	uint32_t idx = hash & CONFIGS_MASK;

	ConfigOption *opt = config->values[idx];

	/* If NULL, this item does not exist */
	if( opt == NULL ) {
		return false;
	}

	while( true ) {
		/* If there's a match, the item exists */
		if( _check_match(opt, key, hash) ) {
			return true;
		}

		/* If there's no next item, the item does not exist */
		if( opt->next == NULL ) {
			return false;
		}

		opt = opt->next;
	}
}

/* Creates a new config option entry */
static ConfigOption *_init_option(char *key, uint32_t hash, char *value) {
	ConfigOption *opt = malloc(sizeof(*opt));

	size_t sz = strlen(key);
	opt->key = malloc(sz + 1);
	memcpy(opt->key, key, sz);
	opt->key[sz] = '\0';

	sz = strlen(value);
	opt->value = malloc(sz + 1);
	memcpy(opt->value, value, sz);
	opt->value[sz] = '\0';

	opt->hash = hash;

	return opt;
}

/* Frees a config option from memory */
static void _free_option(ConfigOption *opt) {
	free(opt->key);
	free(opt->value);
}

/* Frees a config option and all of its children from memory */
static void _free_option_recursively(ConfigOption *opt) {
	ConfigOption *curr = opt;
	while( curr ) {
		_free_option(curr);

		curr = curr->next;
		opt->next = NULL;
		opt = curr;
	}
}

/* Hashes a string using the FNV-1a algorithm */
static uint32_t _hash_string(char *str, size_t len) {
	uint32_t hash = FNV_OFFSET_BASIS;
	for( size_t i = 0; i < len; ++i ) {
		hash ^= str[i];
		hash *= FNV_PRIME;
	}

	return hash;
}

/* Checks if the option matches with the key and hash */
static bool _check_match(ConfigOption *opt, char *key, uint32_t hash) {
	return (opt->hash == hash && strcmp(opt->key, key) == 0);
}
