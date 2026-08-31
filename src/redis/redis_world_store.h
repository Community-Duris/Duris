#ifndef REDIS_WORLD_STORE_H
#define REDIS_WORLD_STORE_H

#include "redis/redis_command_observability.h"

#include <stddef.h>
#include <stdint.h>
#include <vector>

struct redis_connection_settings;

struct redis_world_store_config
{
	const struct redis_connection_settings *connection;
	const char *key_namespace;
	const char *authentication_secret;
	const char *previous_authentication_secret;
	uint64_t season_epoch;
	uint64_t generation_ttl_seconds;
};

constexpr size_t REDIS_WORLD_GENERATION_CHUNK_BYTES = 1024 * 1024;
constexpr size_t REDIS_WORLD_GENERATION_MANIFEST_BYTES = 120;
constexpr size_t REDIS_WORLD_GENERATION_MAX_CHUNKS = 64;

bool redis_world_store_claim_fence(const struct redis_world_store_config *config,
				   const char *writer_token, uint64_t lease_msec);
bool redis_world_store_renew_fence(const struct redis_world_store_config *config,
				   const char *writer_token, uint64_t lease_msec);
bool redis_world_store_release_fence(const struct redis_world_store_config *config,
				     const char *writer_token);
bool redis_world_store_mark_clean_shutdown(const struct redis_world_store_config *config,
					   const char *writer_token);
uint64_t redis_world_store_consume_clean_shutdown(const struct redis_world_store_config *config);
bool redis_world_store_consume_generation(const struct redis_world_store_config *config,
					  const char *writer_token, uint64_t sequence);
bool redis_world_store_read_generation(const struct redis_world_store_config *config,
				       uint64_t sequence, std::vector<unsigned char> *generation);
bool redis_world_store_publish(const struct redis_world_store_config *config,
			       const char *writer_token, uint64_t lease_msec,
			       const unsigned char *data, size_t size, uint64_t sequence,
			       int64_t timestamp, uint32_t checksum);
bool redis_world_store_publish_observed(const struct redis_world_store_config *config,
					const char *writer_token, uint64_t lease_msec,
					const unsigned char *data, size_t size, uint64_t sequence,
					int64_t timestamp, uint32_t checksum,
					redis_shared_command_outcome *outcome);

#endif
