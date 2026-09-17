// White-box connection-boundary test: compile the actual SQL factory and pool.
// It does not replace mysql_real_connect or the pool with test doubles.
#include "sql/sql.c"
#include <iostream>
#include <stdexcept>

int RUNNING_PORT = 7777;
void logit(const char *, const char *, ...) {}

static void require_guard(bool value, const char *message)
{
	if (!value)
		throw std::runtime_error(message);
}
static std::string guard_query(MYSQL *connection, const std::string &query)
{
	require_guard(mysql_real_query(connection, query.data(), query.size()) == 0,
		      "real SQL query failed");
	MYSQL_RES *result = mysql_store_result(connection);
	if (!result)
		return {};
	MYSQL_ROW row = mysql_fetch_row(result);
	const std::string value = row && row[0] ? row[0] : "NULL";
	mysql_free_result(result);
	return value;
}
int main()
{
	try
	{
		require_guard(std::getenv("DB_NAME") && std::strcmp(std::getenv("DB_NAME"),
								    "duris_issue_331_test") == 0,
			      "requires the isolated issue331 fixture database");
		require_guard(setenv("ENVIRONMENT", "local", 1) == 0, "set environment");
		require_guard(setenv("DB_ALLOWED_TARGETS", "127.0.0.1/duris_issue_331_test", 1) ==
				      0,
			      "set explicit allowed fixture target");
		MYSQL *control = sql_open_configured_connection(CLIENT_MULTI_STATEMENTS);
		MYSQL *owner = sql_open_configured_connection(CLIENT_MULTI_STATEMENTS);
		require_guard(control && owner, "real configured connection factory failed");
		const std::string lock = DURIS_SQL_EXCLUSION_LOCK_EXPRESSION;
		require_guard(guard_query(control, "SELECT GET_LOCK(" + lock + ",0)") == "1",
			      "recovery holder failed");
		require_guard(!duris_sql_exclusion_guard_acquire(owner),
			      "runtime acquired while recovery held the boundary");
		require_guard(guard_query(control, "SELECT RELEASE_LOCK(" + lock + ")") == "1",
			      "recovery release failed");
		require_guard(duris_sql_exclusion_guard_acquire(owner),
			      "runtime failed to acquire");
		require_guard(guard_query(control, "SELECT GET_LOCK(" + lock + ",0)") == "0",
			      "recovery acquired while runtime held the boundary");
		require_guard(sql_pool_init(2) == 0, "real configured pool initialization failed");
		MYSQL *borrowed = sql_pool_acquire();
		require_guard(borrowed != nullptr, "healthy runtime pool lease failed");

		// An observed query with drain_before must not probe on a dirty socket.
		DB = owner;
		require_guard(mysql_query(owner, "SELECT 41; SELECT 42") == 0,
			      "multi-result fixture failed");
		const persistence_query_site site = { __FILE__, "guard_native_test", __LINE__ };
		require_guard(sql_trace_exec_at(site, "guard_native_test", "SELECT 43", 9, true,
						true),
			      "guard broke the existing drain-before query contract");
		require_guard(duris_sql_exclusion_guard_allows(borrowed),
			      "pending-result fixture spuriously latched guard loss");

		const unsigned long owner_id = mysql_thread_id(owner);
		guard_query(control, "KILL CONNECTION " + std::to_string(owner_id));
		require_guard(guard_query(control, "SELECT GET_LOCK(" + lock + ",0)") == "1",
			      "lost runtime session did not release advisory ownership");
		// A previously borrowed raw writer still exists. Recovery must not
		// commit merely because the lifetime owner's connection disappeared.
		require_guard(guard_query(borrowed, "SELECT 1") == "1",
			      "borrowed-session fixture failed");
		require_guard(
			guard_query(
				control,
				"SELECT COUNT(*)=0 FROM information_schema.processlist WHERE ID<>CONNECTION_ID()") ==
				"0",
			"recovery quiescence fence missed an existing raw pool writer");
		int pool_active = 0;
		require_guard(sql_pool_acquire_with_status(&pool_active) == nullptr &&
				      pool_active == 1,
			      "direct pool lease bypassed cross-translation-unit guard loss");
		require_guard(sql_open_configured_connection(0) == nullptr,
			      "runtime factory reopened after losing original ownership");
		require_guard(sql_pool_replace_connection(borrowed) == nullptr,
			      "pool replacement bypassed ownership loss");
		sql_pool_shutdown();
		require_guard(
			guard_query(
				control,
				"SELECT COUNT(*)=0 FROM information_schema.processlist WHERE ID<>CONNECTION_ID()") ==
				"1",
			"recovery remained non-quiescent after pool teardown");
		require_guard(guard_query(control, "SELECT RELEASE_LOCK(" + lock + ")") == "1",
			      "final recovery release failed");
		duris_sql_exclusion_guard_release();
		require_guard(sql_open_configured_connection(0) == nullptr,
			      "guard release reset the runtime to unprotected startup mode");
		DB = nullptr;
		mysql_close(owner);
		mysql_close(control);
		std::cout << "native SQL factory/pool exclusion verified: competing acquisition, "
			     "pending-result safety, raw-writer fence, shared loss latch, "
			     "lease/reconnect refusal, and drained recovery boundary\n";
		return 0;
	}
	catch (const std::exception &error)
	{
		std::cerr << "native guard failed: " << error.what() << '\n';
		return 1;
	}
}
