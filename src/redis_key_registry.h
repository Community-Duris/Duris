#ifndef REDIS_KEY_REGISTRY_H
#define REDIS_KEY_REGISTRY_H

#include <stddef.h>

#define REDIS_STORE(symbol, lifecycle_id, locator, kind) \
	inline constexpr char REDIS_STORE_ID_##symbol[] = lifecycle_id;
#define REDIS_SURFACE(symbol, token, pattern, store, kind, state) \
	inline constexpr char REDIS_##symbol[] = token;
#define REDIS_OWNED_PATTERN(symbol, pattern) inline constexpr char REDIS_OWNED_##symbol[] = pattern;
#include "redis_key_registry.def"
#undef REDIS_OWNED_PATTERN
#undef REDIS_SURFACE
#undef REDIS_STORE

struct redis_key_registry_entry
{
	const char *name;
	const char *token;
	const char *pattern;
	const char *lifecycle_id;
	const char *kind;
	const char *state;
};

extern const struct redis_key_registry_entry redis_key_registry[];
extern const size_t redis_key_registry_count;

#endif
