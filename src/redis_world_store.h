#ifndef REDIS_WORLD_STORE_H
#define REDIS_WORLD_STORE_H

#include <stddef.h>
#include <stdint.h>

struct redis_world_store_config
{
	const char *host;
	int port;
	int connect_timeout_msec;
	int command_timeout_msec;
	uint64_t season_epoch;
	uint64_t generation_ttl_seconds;
};

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
bool redis_world_store_publish(const struct redis_world_store_config *config,
			       const char *writer_token, uint64_t lease_msec,
			       const unsigned char *data, size_t size, uint64_t sequence,
			       int64_t timestamp, uint32_t checksum);

#endif
