#include "redis/redis_key_registry.h"

const redis_key_registry_entry redis_key_registry[] = {
#define REDIS_STORE(symbol, lifecycle_id, locator, kind)
#define REDIS_SURFACE(symbol, token, pattern, store, kind, state) \
	{ #symbol, token, pattern, REDIS_STORE_ID_##store, kind, state },
#define REDIS_OWNED_PATTERN(symbol, pattern)
#include "redis/redis_key_registry.def"
#undef REDIS_OWNED_PATTERN
#undef REDIS_SURFACE
#undef REDIS_STORE
};

const size_t redis_key_registry_count = sizeof redis_key_registry / sizeof redis_key_registry[0];
