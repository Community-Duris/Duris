#include "redis/redis_namespace.h"
#include "redis/redis_key_registry.h"

#include <stdio.h>
#include <string.h>

bool redis_namespace_validate(const char *configured, const char *environment, char *output,
			      size_t output_size)
{
	if (!environment || (strcmp(environment, "local") && strcmp(environment, "production")) ||
	    !configured || !*configured || !output || !output_size ||
	    strlen(configured) >= output_size)
		return false;
	char prefix[32];
	const int written = snprintf(prefix, sizeof prefix, "duris:%s:", environment);
	if (written <= 0 || (size_t)written >= sizeof prefix ||
	    strncmp(configured, prefix, (size_t)written))
		return false;
	const char *deployment = configured + written;
	const size_t deployment_size = strlen(deployment);
	if (!deployment_size || deployment_size > 32)
		return false;
	for (size_t index = 0; index < deployment_size; ++index)
	{
		const unsigned char value = static_cast<unsigned char>(deployment[index]);
		if (!(value >= 'a' && value <= 'z') && !(value >= '0' && value <= '9') &&
		    value != '-' && value != '_')
			return false;
	}
	if (deployment[0] == '-' || deployment[0] == '_' ||
	    deployment[deployment_size - 1] == '-' || deployment[deployment_size - 1] == '_')
		return false;
	memcpy(output, configured, strlen(configured) + 1);
	return true;
}

bool redis_namespace_season_key(const char *key_namespace, uint64_t epoch, const char *suffix,
				char *output, size_t output_size)
{
	if (!key_namespace || !*key_namespace || !epoch || !suffix || !*suffix || !output ||
	    !output_size)
		return false;
	char epoch_text[32];
	const int epoch_size =
		snprintf(epoch_text, sizeof epoch_text, "%llu", (unsigned long long)epoch);
	if (epoch_size <= 0 || (size_t)epoch_size >= sizeof epoch_text)
		return false;
	const size_t namespace_size = strlen(key_namespace);
	const size_t infix_size = strlen(REDIS_SEASON_INFIX);
	const size_t suffix_size = strlen(suffix);
	const size_t required =
		namespace_size + infix_size + (size_t)epoch_size + 1 + suffix_size + 1;
	if (required > output_size)
		return false;
	char *cursor = output;
	memcpy(cursor, key_namespace, namespace_size);
	cursor += namespace_size;
	memcpy(cursor, REDIS_SEASON_INFIX, infix_size);
	cursor += infix_size;
	memcpy(cursor, epoch_text, (size_t)epoch_size);
	cursor += epoch_size;
	*cursor++ = ':';
	memcpy(cursor, suffix, suffix_size + 1);
	return true;
}
