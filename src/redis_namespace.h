#ifndef REDIS_NAMESPACE_H
#define REDIS_NAMESPACE_H

#include <stddef.h>
#include <stdint.h>

bool redis_namespace_validate(const char *configured, const char *environment, char *output,
			      size_t output_size);
bool redis_namespace_season_key(const char *key_namespace, uint64_t epoch, const char *suffix,
				char *output, size_t output_size);

#endif
