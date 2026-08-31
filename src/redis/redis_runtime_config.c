#include "redis/redis_runtime_config.h"

#include "redis/redis_connection.h"

#include <cerrno>
#include <cstdlib>
#include <cstring>
#include <strings.h>

namespace
{
constexpr int redis_connect_timeout_msec = 250;
constexpr int redis_command_timeout_msec = 100;

struct redis_identity
{
	const char *username;
	const char *password;
};

bool parse_number(const char *value, int minimum, int maximum, int fallback, int *result)
{
	if (!result)
		return false;
	if (!value || !*value)
	{
		*result = fallback;
		return true;
	}
	errno = 0;
	char *end = NULL;
	const long parsed = strtol(value, &end, 10);
	if (errno || !end || *end || parsed < minimum || parsed > maximum)
		return false;
	*result = static_cast<int>(parsed);
	return true;
}

bool host_is_loopback(const char *host)
{
	return host && (!strcasecmp(host, "localhost") || !strcmp(host, "127.0.0.1") ||
			!strcmp(host, "::1"));
}

bool resolve_identity(const char *username_name, const char *password_name, bool production,
		      redis_identity *identity)
{
	if (!username_name || !password_name || !identity)
		return false;
	const char *username = getenv(username_name);
	const char *password = getenv(password_name);
	const bool scoped = (username && *username) || (password && *password);
	if (scoped)
	{
		if (!username || !*username || !password || !*password)
			return false;
	}
	else
	{
		if (production)
			return false;
		username = getenv("REDIS_USERNAME");
		password = getenv("REDIS_PASSWORD");
	}
	identity->username = username;
	identity->password = password;
	return true;
}
} // namespace

void redis_runtime_connections_destroy(redis_runtime_connections *connections)
{
	if (!connections)
		return;
	redis_connection_settings_destroy(connections->world);
	redis_connection_settings_destroy(connections->presence);
	redis_connection_settings_destroy(connections->cache);
	redis_connection_settings_destroy(connections->donation);
	redis_connection_settings_destroy(connections->maintenance);
	*connections = {};
}

bool redis_runtime_connections_configure(bool donation_enabled,
					 redis_runtime_connections *connections)
{
	if (!connections)
		return false;
	const char *unix_socket = getenv("REDIS_SOCKET");
	const bool use_socket = unix_socket && *unix_socket;
	const char *configured_host = getenv("REDIS_HOST");
	const char *configured_port = getenv("REDIS_PORT");
	if (use_socket &&
	    ((configured_host && *configured_host) || (configured_port && *configured_port)))
		return false;
	const char *host = use_socket ? NULL : configured_host;
	if (!use_socket && (!host || !*host))
		host = "127.0.0.1";
	int port = 0;
	if (!use_socket && !parse_number(configured_port, 1, 65535, 6379, &port))
		return false;

	int database = 0;
	if (!parse_number(getenv("REDIS_DB"), 0, 255, 0, &database))
		return false;
	const char *tls_value = getenv("REDIS_TLS");
	const bool tls = tls_value && !strcasecmp(tls_value, "TRUE");
	if (tls_value && *tls_value && strcasecmp(tls_value, "TRUE") &&
	    strcasecmp(tls_value, "FALSE"))
		return false;
	const char *environment = getenv("ENVIRONMENT");
	const bool production = environment && !strcasecmp(environment, "production");
	const bool require_tls = !use_socket && production && !host_is_loopback(host);

	redis_identity world = {};
	redis_identity presence = {};
	redis_identity cache = {};
	redis_identity donation = {};
	redis_identity maintenance = {};
	if (!resolve_identity("REDIS_WORLD_USERNAME", "REDIS_WORLD_PASSWORD", production, &world) ||
	    !resolve_identity("REDIS_PRESENCE_USERNAME", "REDIS_PRESENCE_PASSWORD", production,
			      &presence) ||
	    !resolve_identity("REDIS_CACHE_USERNAME", "REDIS_CACHE_PASSWORD", production, &cache) ||
	    !resolve_identity("REDIS_MAINTENANCE_USERNAME", "REDIS_MAINTENANCE_PASSWORD",
			      production, &maintenance) ||
	    (donation_enabled &&
	     !resolve_identity("REDIS_DONATION_USERNAME", "REDIS_DONATION_PASSWORD", production,
			       &donation)))
		return false;
	if (production)
	{
		const char *usernames[5] = { world.username, presence.username, cache.username,
					     maintenance.username,
					     donation_enabled ? donation.username : NULL };
		const size_t count = donation_enabled ? 5 : 4;
		for (size_t left = 0; left < count; ++left)
			for (size_t right = left + 1; right < count; ++right)
				if (!usernames[left] || !*usernames[left] || !usernames[right] ||
				    !*usernames[right] ||
				    !strcmp(usernames[left], usernames[right]))
					return false;
	}

	redis_connection_options options = {
		host,
		port,
		redis_connect_timeout_msec,
		redis_command_timeout_msec,
		database,
		world.username,
		world.password,
		tls,
		getenv("REDIS_CA_CERT"),
		getenv("REDIS_TLS_SERVER_NAME"),
		require_tls,
		use_socket ? unix_socket : NULL,
	};
	redis_runtime_connections configured = {};
	configured.world = redis_connection_settings_create(&options);
	options.username = presence.username;
	options.password = presence.password;
	configured.presence = redis_connection_settings_create(&options);
	options.username = cache.username;
	options.password = cache.password;
	configured.cache = redis_connection_settings_create(&options);
	if (donation_enabled)
	{
		options.username = donation.username;
		options.password = donation.password;
		configured.donation = redis_connection_settings_create(&options);
	}
	options.username = maintenance.username;
	options.password = maintenance.password;
	configured.maintenance = redis_connection_settings_create(&options);
	if (!configured.world || !configured.presence || !configured.cache ||
	    (donation_enabled && !configured.donation) || !configured.maintenance)
	{
		redis_runtime_connections_destroy(&configured);
		return false;
	}
	configured.host = host;
	configured.port = port;
	configured.unix_socket = use_socket;
	redis_runtime_connections_destroy(connections);
	*connections = configured;
	return true;
}
