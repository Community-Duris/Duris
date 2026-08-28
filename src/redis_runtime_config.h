#ifndef REDIS_RUNTIME_CONFIG_H
#define REDIS_RUNTIME_CONFIG_H

struct redis_connection_settings;

struct redis_runtime_connections
{
	struct redis_connection_settings *world;
	struct redis_connection_settings *presence;
	struct redis_connection_settings *cache;
	struct redis_connection_settings *donation;
	struct redis_connection_settings *maintenance;
	const char *host;
	int port;
	bool unix_socket;
};

bool redis_runtime_connections_configure(bool donation_enabled,
					 struct redis_runtime_connections *connections);
void redis_runtime_connections_destroy(struct redis_runtime_connections *connections);

#endif
