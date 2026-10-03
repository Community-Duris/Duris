/* Private test connection factory. Production runtime, transport and repository
 * execute unchanged; account identity stays unknown in this capture fixture. */
#include "persistence/persistence_mode.h"
#include "sql/sql_telemetry_connection.h"

#include <cassert>
#include <cstdlib>
#include <cstring>
#include <string_view>

persistence_mode persistence_mode_get(void)
{
	return PERSISTENCE_MODE_MARIADB_PRIMARY;
}

MYSQL *sql_open_telemetry_connection(void)
{
	const auto required = [](const char *name)
	{
		const char *value = std::getenv(name);
		assert(value && *value);
		return value;
	};
	assert(std::strcmp(required("TELEMETRY_REPOSITORY_DISPOSABLE"), "1") == 0);
	const char *host = required("DB_HOST");
	const char *database = required("DB_NAME");
	const char *user = required("TELEMETRY_BATTLE_WRITER_USER");
	assert(std::strcmp(host, "127.0.0.1") == 0);
	assert(std::string_view(database).starts_with("duris_telemetry_test_"));
	assert(std::string_view(user).starts_with("tbr_writer_"));
	char *end = nullptr;
	const auto port = std::strtoul(required("DB_PORT"), &end, 10);
	assert(end && *end == '\0' && port > 0U && port <= 65535U);
	MYSQL *connection = mysql_init(nullptr);
	assert(connection);
	unsigned int timeout = 3U;
	unsigned int protocol = MYSQL_PROTOCOL_TCP;
	assert(mysql_options(connection, MYSQL_OPT_CONNECT_TIMEOUT, &timeout) == 0);
	assert(mysql_options(connection, MYSQL_OPT_READ_TIMEOUT, &timeout) == 0);
	assert(mysql_options(connection, MYSQL_OPT_WRITE_TIMEOUT, &timeout) == 0);
	assert(mysql_options(connection, MYSQL_OPT_PROTOCOL, &protocol) == 0);
	if (!mysql_real_connect(connection, host, user,
				required("TELEMETRY_BATTLE_WRITER_PASSWORD"), database,
				static_cast<unsigned int>(port), nullptr, 0))
	{
		mysql_close(connection);
		return nullptr;
	}
	assert(sql_telemetry_set_cloexec(connection));
	for (const char *statement :
	     { "SET SESSION sql_mode='STRICT_ALL_TABLES,NO_ENGINE_SUBSTITUTION'",
	       "SET SESSION time_zone='+00:00'",
	       "SET SESSION TRANSACTION ISOLATION LEVEL READ COMMITTED" })
		assert(mysql_real_query(connection, statement, std::strlen(statement)) == 0);
	return connection;
}
