#include "sql/sql_telemetry_connection.h"
#include "sql/mysql_client_compat.h"
#include <cassert>
#include <fcntl.h>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <new>
#include <cstddef>

int RUNNING_PORT = 7777;
void logit(const char *, const char *, ...) {}
MYSQL *sql_open_configured_connection(unsigned long);
static unsigned calls = 0;
static const char *expected_user = "fixture_ingest";
static const char *expected_password = "fixture_ingest_only";
static bool fail_after_connect = false;
static bool fail_next_new = false;
static unsigned int closes = 0;
extern "C" void *__real__Znwm(std::size_t);
extern "C" void *__wrap__Znwm(std::size_t size)
{
	if (fail_next_new)
	{
		fail_next_new = false;
		throw std::bad_alloc();
	}
	return __real__Znwm(size);
}
extern "C" void __real_mysql_close(MYSQL *);
extern "C" void __wrap_mysql_close(MYSQL *connection)
{
	++closes;
	__real_mysql_close(connection);
}
extern "C" MYSQL *__real_mysql_real_connect(MYSQL *, const char *, const char *, const char *,
					    const char *, unsigned int, const char *,
					    unsigned long);
extern "C" MYSQL *__wrap_mysql_real_connect(MYSQL *conn, const char *host, const char *user,
					    const char *password, const char *database,
					    unsigned int port, const char *socket,
					    unsigned long flags)
{
	++calls;
	assert(std::strcmp(user, expected_user) == 0);
	assert(std::strcmp(password, expected_password) == 0);
	assert(std::strcmp(host, "127.0.0.1") == 0);
	assert(std::strcmp(database, "duris_telemetry_test") == 0);
	assert((flags & CLIENT_MULTI_STATEMENTS) == 0);
	// A credential-boundary spy only: verify the real factory selected the
	// expected role, then use the existing disposable root account for I/O.
	// This deliberately does not create users, grant privileges or claim
	// real ingest-role authentication/grant coverage.
	MYSQL *connected =
		__real_mysql_real_connect(conn, host, "root", "", database, port, socket, flags);
	if (connected && fail_after_connect)
	{
		fail_after_connect = false;
		fail_next_new = true;
	}
	return connected;
}
static void env(const char *key, const char *value)
{
	assert(setenv(key, value, 1) == 0);
}
int main()
{
	assert(getenv("TELEMETRY_REPOSITORY_DISPOSABLE") &&
	       std::strcmp(getenv("TELEMETRY_REPOSITORY_DISPOSABLE"), "1") == 0);
	env("ENVIRONMENT", "local");
	env("DB_HOST", "127.0.0.1");
	env("DB_USER", "fixture_game");
	env("DB_PASSWD", "fixture_game_only");
	env("DB_NAME", "duris_telemetry_test");
	env("DB_ALLOWED_TARGETS", "127.0.0.1/duris_telemetry_test");
	unsetenv("DB_SOCKET");
	unsetenv("TELEMETRY_DB_USER");
	unsetenv("TELEMETRY_DB_PASSWD");
	assert(sql_open_telemetry_connection() == nullptr && calls == 0);
	env("TELEMETRY_DB_USER", expected_user);
	assert(sql_open_telemetry_connection() == nullptr && calls == 0);
	env("TELEMETRY_DB_PASSWD", expected_password);
	env("DB_ALLOWED_TARGETS", "127.0.0.1/another_test");
	assert(sql_open_telemetry_connection() == nullptr && calls == 0);
	env("DB_ALLOWED_TARGETS", "127.0.0.1/duris_telemetry_test");
	env("ENVIRONMENT", "production");
	RUNNING_PORT = 7778;
	assert(sql_open_telemetry_connection() == nullptr && calls == 0);
	env("ENVIRONMENT", "local");
	RUNNING_PORT = 7777;
	MYSQL *conn = sql_open_telemetry_connection();
	assert(conn && calls == 1);
	assert(fcntl(sql_mysql_socket_descriptor(conn), F_GETFD) & FD_CLOEXEC);
	unsigned int timeout = 0;
	assert(mysql_get_option(conn, MYSQL_OPT_CONNECT_TIMEOUT, &timeout) == 0 && timeout == 2);
	assert(mysql_get_option(conn, MYSQL_OPT_READ_TIMEOUT, &timeout) == 0 && timeout == 2);
	assert(mysql_get_option(conn, MYSQL_OPT_WRITE_TIMEOUT, &timeout) == 0 && timeout == 2);
	const char *sql =
		"SELECT @@time_zone,@@sql_mode,@@character_set_connection,@@innodb_lock_wait_timeout";
	assert(mysql_real_query(conn, sql, std::strlen(sql)) == 0);
	MYSQL_RES *result = mysql_store_result(conn);
	assert(result);
	MYSQL_ROW row = mysql_fetch_row(result);
	assert(row);
	assert(std::strcmp(row[0], "+00:00") == 0 && std::strstr(row[1], "STRICT_TRANS_TABLES"));
	assert(std::strcmp(row[2], "utf8mb4") == 0 && std::strcmp(row[3], "2") == 0);
	mysql_free_result(result);
	mysql_close(conn);
	expected_user = "fixture_game";
	expected_password = "fixture_game_only";
	conn = sql_open_configured_connection(0);
	assert(conn && calls == 2);
	mysql_close(conn);
	expected_user = "fixture_ingest";
	expected_password = "fixture_ingest_only";
	const auto closes_before = closes;
	fail_after_connect = true;
	conn = sql_open_telemetry_connection();
	assert(conn == nullptr && !fail_next_new && closes == closes_before + 1);
	std::cout
		<< "verified factory: fail-closed credential selection, target/role rejection, "
		   "real UTC/strict/charset/deadline initialization and allocation cleanup PASS (credential I/O spy)\n";
}
