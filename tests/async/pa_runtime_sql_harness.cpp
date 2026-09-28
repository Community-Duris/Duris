#include "persistence/economic_sql_lifecycle_guard.h"
#include "persistence/economic_sql_lifecycle_lock_names.h"
#include "sql/sql.h"
#include "sql/sql_economic_runtime.h"
#include "sql/sql_exclusion_guard.h"
#include "sql/sql_telemetry_connection.h"

#include <mysql/mysql.h>

#include <cerrno>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <memory>
#include <stdexcept>
#include <string>
#include <vector>
#include <type_traits>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

namespace
{
constexpr const char *exclusion_lock_sql = "CONCAT('duris.player.death.restitution.',DATABASE())";
constexpr const char *probe_table = "pa_runtime_sql_rows";
unsigned long opened_control_session = 0;
std::string executable_path;

const char *required(const char *name)
{
	const char *value = std::getenv(name);
	if (!value || !*value)
		throw std::runtime_error(std::string("missing ") + name);
	return value;
}

MYSQL *connect_fixture()
{
	const std::string host = required("DB_HOST");
	const std::string schema = required("DB_NAME");
	if ((host != "127.0.0.1" && host != "host.docker.internal") || std::getenv("DB_SOCKET") ||
	    std::strcmp(required("PA_RUNTIME_SQL_DISPOSABLE_SCHEMA"), "1") ||
	    !schema.starts_with("pa_runtime_sql_test_") ||
	    schema.find_first_not_of(
		    "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789_") !=
		    std::string::npos)
		throw std::runtime_error("explicit disposable loopback schema required");

	char *end = nullptr;
	const unsigned long port = std::strtoul(required("DB_PORT"), &end, 10);
	if (!end || *end || !port || port > 65535)
		throw std::runtime_error("invalid disposable DB port");

	MYSQL *connection = mysql_init(nullptr);
	if (!connection)
		throw std::runtime_error("mysql_init failed");
	unsigned int timeout = 5;
	unsigned int protocol = MYSQL_PROTOCOL_TCP;
	using reconnect_flag = std::remove_pointer_t<decltype(MYSQL_BIND{}.is_null)>;
	reconnect_flag reconnect = false;
	if (mysql_options(connection, MYSQL_OPT_CONNECT_TIMEOUT, &timeout) ||
	    mysql_options(connection, MYSQL_OPT_READ_TIMEOUT, &timeout) ||
	    mysql_options(connection, MYSQL_OPT_WRITE_TIMEOUT, &timeout) ||
	    mysql_options(connection, MYSQL_OPT_PROTOCOL, &protocol) ||
	    mysql_options(connection, MYSQL_OPT_RECONNECT, &reconnect))
	{
		mysql_close(connection);
		throw std::runtime_error("could not configure disposable SQL connection");
	}
	if (!mysql_real_connect(connection, host.c_str(), required("DB_USER"),
				required("DB_PASSWD"), schema.c_str(),
				static_cast<unsigned int>(port), nullptr, 0))
	{
		const unsigned int error = mysql_errno(connection);
		mysql_close(connection);
		throw std::runtime_error("mysql_real_connect failed, error=" +
					 std::to_string(error));
	}
	if (!sql_telemetry_set_cloexec(connection))
	{
		mysql_close(connection);
		throw std::runtime_error("could not set disposable SQL socket close-on-exec");
	}
	return connection;
}

void execute(MYSQL *connection, const std::string &sql)
{
	if (!connection || mysql_real_query(connection, sql.data(), sql.size()))
		throw std::runtime_error("SQL execution failed, error=" +
					 std::to_string(connection ? mysql_errno(connection) : 0));
	MYSQL_RES *result = mysql_store_result(connection);
	if (result)
		mysql_free_result(result);
	if (mysql_next_result(connection) != -1)
		throw std::runtime_error("unexpected multi-result SQL response");
}

std::vector<std::vector<std::string>> rows(MYSQL *connection, const std::string &sql)
{
	if (!connection || mysql_real_query(connection, sql.data(), sql.size()))
		throw std::runtime_error("SQL query failed, error=" +
					 std::to_string(connection ? mysql_errno(connection) : 0));
	MYSQL_RES *result = mysql_store_result(connection);
	if (!result)
		throw std::runtime_error("SQL query returned no result");
	std::vector<std::vector<std::string>> output;
	while (MYSQL_ROW row = mysql_fetch_row(result))
	{
		unsigned long *lengths = mysql_fetch_lengths(result);
		std::vector<std::string> values;
		for (unsigned int index = 0; index < mysql_num_fields(result); ++index)
			values.emplace_back(row[index] ? std::string(row[index], lengths[index]) :
							 "<NULL>");
		output.push_back(std::move(values));
	}
	mysql_free_result(result);
	if (mysql_errno(connection))
		throw std::runtime_error("SQL result fetch failed");
	return output;
}

std::string scalar(MYSQL *connection, const std::string &sql)
{
	const auto result = rows(connection, sql);
	if (result.size() != 1 || result[0].size() != 1)
		throw std::runtime_error("expected one SQL scalar");
	return result[0][0];
}

void require(bool condition, const std::string &message)
{
	if (!condition)
		throw std::runtime_error(message);
}

std::string exclusion_owner_query()
{
	return "SELECT IFNULL(IS_USED_LOCK(" + std::string(exclusion_lock_sql) + "),0)";
}

std::string boot_owner_query()
{
	return "SELECT IFNULL(IS_USED_LOCK('" +
	       std::string(ECONOMIC_SQL_BOOT_MAINTENANCE_LOCK_NAME) + "'),0)";
}

std::string writer_owner_query()
{
	return "SELECT IFNULL(IS_USED_LOCK('duris:economic_sql_currency_writers'),0)";
}

void require_no_locks(MYSQL *observer, const char *scenario)
{
	const std::string main_owner = scalar(observer, exclusion_owner_query());
	const std::string control_owner = scalar(observer, boot_owner_query());
	const std::string writer_owner = scalar(observer, writer_owner_query());
	require(main_owner == "0",
		std::string(scenario) +
			": main-owner exclusion lock leaked, session=" + main_owner);
	require(control_owner == "0",
		std::string(scenario) +
			": runtime/maintenance lock leaked, session=" + control_owner);
	require(writer_owner == "0",
		std::string(scenario) + ": currency-writer lock leaked, session=" + writer_owner);
}

void prepare_probe_table(MYSQL *observer)
{
	execute(observer, "CREATE TABLE IF NOT EXISTS pa_runtime_sql_rows ("
			  "id INT NOT NULL PRIMARY KEY,value INT NOT NULL) ENGINE=InnoDB");
	execute(observer, "DELETE FROM pa_runtime_sql_rows");
	execute(observer, "INSERT INTO pa_runtime_sql_rows(id,value) VALUES(1,0)");
}

bool production_query(const char *sql)
{
	const persistence_query_site site = { "pa_runtime_sql_harness.cpp", "production_query", 1 };
	return sql_trace_exec_at(site, "pa-runtime-sql", sql, std::strlen(sql), false, false);
}

void run_child_mode(const char *mode, const char *expectation)
{
	const pid_t child = fork();
	if (child < 0)
		throw std::runtime_error("fork failed");
	if (!child)
	{
		if (std::strcmp(mode, "--maintenance-probe") == 0)
			execl(executable_path.c_str(), executable_path.c_str(), mode, expectation,
			      static_cast<char *>(nullptr));
		execl(executable_path.c_str(), executable_path.c_str(), mode,
		      static_cast<char *>(nullptr));
		_exit(127);
	}
	int status = 0;
	if (waitpid(child, &status, 0) != child || !WIFEXITED(status) || WEXITSTATUS(status) != 0)
		throw std::runtime_error(std::string("child mode failed: ") + mode);
}

int maintenance_probe(const char *expectation)
{
	MYSQL *connection = nullptr;
	try
	{
		connection = connect_fixture();
		bool acquired = false;
		{
			economic_sql_lifecycle_guard authority;
			const unsigned int status =
				economic_sql_lifecycle_guard::acquire_maintenance(connection,
										  &authority);
			acquired = status == 0 && authority.is_maintenance_authority();
			if (std::strcmp(expectation, "deny") == 0)
				require(status == EBUSY && !authority.is_maintenance_authority(),
					"independent maintenance process was not denied by SQL runtime lock");
			else if (std::strcmp(expectation, "allow") == 0)
				require(acquired,
					"maintenance did not enter after runtime-owner cleanup");
			else
				throw std::runtime_error("invalid maintenance-probe expectation");
		}
		mysql_close(connection);
		connection = nullptr;
		std::printf("PASS maintenance-probe expectation=%s\n", expectation);
		return 0;
	}
	catch (const std::exception &error)
	{
		if (connection)
			mysql_close(connection);
		std::fprintf(stderr, "FAIL maintenance-probe: %s\n", error.what());
		return 1;
	}
}

int healthy_binding_and_cleanup()
{
	MYSQL *observer = nullptr;
	MYSQL *main_owner = nullptr;
	MYSQL *probe = nullptr;
	try
	{
		observer = connect_fixture();
		prepare_probe_table(observer);
		// Starting without a main owner must fail closed and release the
		// dedicated control lock; the same production owner can then retry.
		require(!sql_economic_runtime_start(),
			"runtime owner started before the main exclusion owner");
		require(scalar(observer, boot_owner_query()) == "0",
			"failed initial binding leaked the dedicated control lock");

		main_owner = connect_fixture();
		require(duris_sql_exclusion_guard_acquire(main_owner),
			"main-owner exclusion lock acquisition failed");
		auto &state = duris_sql_exclusion_guard_state_ref();
		const unsigned long main_session = mysql_thread_id(main_owner);
		require(state.connection == main_owner && state.connection_id == main_session &&
				!state.lost,
			"main-owner lock was not recorded by production exclusion state");
		require(sql_economic_runtime_start(),
			"production SQL runtime owner failed to start");
		const unsigned long control_session = opened_control_session;
		require(control_session != 0 && control_session != main_session,
			"runtime control connection was not a distinct SQL session");
		require(state.economic_connection_id == control_session && !state.lost,
			"runtime control session was not bound to main-owner SQL authority");
		require(scalar(observer, exclusion_owner_query()) == std::to_string(main_session),
			"main SQL lock owner does not match its original session");
		require(scalar(observer, boot_owner_query()) == std::to_string(control_session),
			"runtime SQL lock owner does not match its dedicated control session");

		probe = connect_fixture();
		require(duris_sql_exclusion_guard_allows(probe),
			"healthy independent query probe was fenced");
		DB = main_owner;
		require(production_query("UPDATE pa_runtime_sql_rows SET value=7 WHERE id=1"),
			"production SQL query executor rejected a healthy write");
		require(scalar(observer, "SELECT value FROM pa_runtime_sql_rows WHERE id=1") == "7",
			"healthy production SQL query did not persist its row");
		run_child_mode("--maintenance-probe", "deny");

		sql_economic_runtime_shutdown();
		require(scalar(observer, boot_owner_query()) == "0",
			"runtime-owner shutdown did not release its SQL control lock");
		run_child_mode("--maintenance-probe", "allow");
		require(scalar(observer, exclusion_owner_query()) == std::to_string(main_session),
			"healthy runtime shutdown lost the still-live main-owner lock");
		require(scalar(observer, boot_owner_query()) == "0" &&
				scalar(observer, writer_owner_query()) == "0",
			"runtime/maintenance cleanup left a named lock held");

		duris_sql_exclusion_guard_release();
		DB = nullptr;
		mysql_close(main_owner);
		main_owner = nullptr;
		mysql_close(probe);
		probe = nullptr;
		require_no_locks(observer, "healthy-cleanup");
		execute(observer, "DROP TABLE pa_runtime_sql_rows");
		mysql_close(observer);
		observer = nullptr;
		std::puts("PASS healthy-binding maintenance-exclusion runtime-cleanup");
		return 0;
	}
	catch (const std::exception &error)
	{
		DB = nullptr;
		sql_economic_runtime_shutdown();
		duris_sql_exclusion_guard_release();
		if (main_owner)
			mysql_close(main_owner);
		if (probe)
			mysql_close(probe);
		if (observer)
			mysql_close(observer);
		std::fprintf(stderr, "FAIL healthy-binding: %s\n", error.what());
		return 1;
	}
}

int wrong_binding_and_cleanup()
{
	MYSQL *observer = nullptr;
	MYSQL *main_owner = nullptr;
	MYSQL *real_control = nullptr;
	MYSQL *wrong_control = nullptr;
	MYSQL *probe = nullptr;
	try
	{
		observer = connect_fixture();
		main_owner = connect_fixture();
		real_control = connect_fixture();
		wrong_control = connect_fixture();
		probe = connect_fixture();
		require(duris_sql_exclusion_guard_acquire(main_owner),
			"main-owner lock acquisition failed before binding probe");
		{
			economic_sql_lifecycle_guard authority;
			require(!economic_sql_lifecycle_guard::acquire_runtime(real_control,
									       &authority),
				"dedicated runtime lifecycle guard acquisition failed");
			const unsigned long expected = mysql_thread_id(real_control);
			const unsigned long wrong = mysql_thread_id(wrong_control);
			require(expected != wrong && scalar(observer, boot_owner_query()) ==
							     std::to_string(expected),
				"runtime lifecycle lock was not owned by its control session");
			require(!duris_sql_exclusion_guard_bind_economic_runtime(wrong_control),
				"wrong SQL control session was accepted as runtime authority");
			auto &state = duris_sql_exclusion_guard_state_ref();
			require(state.lost && state.economic_connection_id == wrong,
				"wrong binding did not irreversibly latch loss against its session id");
			require(!duris_sql_exclusion_guard_bind_economic_runtime(real_control),
				"a correct session cleared a previously latched wrong binding");
			require(!duris_sql_exclusion_guard_allows(probe),
				"query admission recovered after wrong binding");
		}
		duris_sql_exclusion_guard_release();
		mysql_close(wrong_control);
		wrong_control = nullptr;
		mysql_close(real_control);
		real_control = nullptr;
		mysql_close(main_owner);
		main_owner = nullptr;
		mysql_close(probe);
		probe = nullptr;
		require_no_locks(observer, "wrong-binding-cleanup");
		mysql_close(observer);
		observer = nullptr;
		std::puts("PASS wrong-binding irreversible-loss cleanup");
		return 0;
	}
	catch (const std::exception &error)
	{
		duris_sql_exclusion_guard_release();
		if (wrong_control)
			mysql_close(wrong_control);
		if (real_control)
			mysql_close(real_control);
		if (main_owner)
			mysql_close(main_owner);
		if (probe)
			mysql_close(probe);
		if (observer)
			mysql_close(observer);
		std::fprintf(stderr, "FAIL wrong-binding: %s\n", error.what());
		return 1;
	}
}

int control_session_loss()
{
	MYSQL *observer = nullptr;
	MYSQL *main_owner = nullptr;
	MYSQL *probe = nullptr;
	MYSQL *replacement = nullptr;
	try
	{
		observer = connect_fixture();
		main_owner = connect_fixture();
		require(duris_sql_exclusion_guard_acquire(main_owner),
			"main-owner lock acquisition failed before control-loss probe");
		require(sql_economic_runtime_start(),
			"runtime owner failed before control-loss probe");
		const unsigned long main_session = mysql_thread_id(main_owner);
		const unsigned long control_session = opened_control_session;
		require(control_session && control_session != main_session,
			"runtime control session was not distinct before loss");
		execute(observer, "KILL CONNECTION " + std::to_string(control_session));
		require(scalar(observer, exclusion_owner_query()) == std::to_string(main_session),
			"control loss unexpectedly released the main-owner lock");
		require(scalar(observer, boot_owner_query()) == "0",
			"killed control session still owns runtime lifecycle lock");
		probe = connect_fixture();
		require(!duris_sql_exclusion_guard_allows(probe),
			"control-session loss did not fence an independent SQL probe");
		auto &state = duris_sql_exclusion_guard_state_ref();
		require(state.lost, "control-session loss did not latch irreversibly");

		replacement = connect_fixture();
		require(scalar(replacement,
			       "SELECT GET_LOCK('" +
				       std::string(ECONOMIC_SQL_BOOT_MAINTENANCE_LOCK_NAME) +
				       "',0)") == "1",
			"replacement control session could not prove the lost lock was released");
		require(!duris_sql_exclusion_guard_allows(replacement) && state.lost,
			"replacement session cleared the control-loss latch");
		require(scalar(replacement,
			       "SELECT RELEASE_LOCK('" +
				       std::string(ECONOMIC_SQL_BOOT_MAINTENANCE_LOCK_NAME) +
				       "')") == "1",
			"replacement control session did not release its probe lock");

		sql_economic_runtime_shutdown();
		duris_sql_exclusion_guard_release();
		mysql_close(main_owner);
		main_owner = nullptr;
		mysql_close(probe);
		probe = nullptr;
		mysql_close(replacement);
		replacement = nullptr;
		require_no_locks(observer, "control-loss-cleanup");
		mysql_close(observer);
		observer = nullptr;
		std::puts("PASS control-session-loss irreversible-latch cleanup");
		return 0;
	}
	catch (const std::exception &error)
	{
		DB = nullptr;
		sql_economic_runtime_shutdown();
		duris_sql_exclusion_guard_release();
		if (main_owner)
			mysql_close(main_owner);
		if (probe)
			mysql_close(probe);
		if (replacement)
			mysql_close(replacement);
		if (observer)
			mysql_close(observer);
		std::fprintf(stderr, "FAIL control-session-loss: %s\n", error.what());
		return 1;
	}
}

int main_owner_session_loss()
{
	MYSQL *observer = nullptr;
	MYSQL *main_owner = nullptr;
	MYSQL *probe = nullptr;
	MYSQL *replacement = nullptr;
	try
	{
		observer = connect_fixture();
		main_owner = connect_fixture();
		require(duris_sql_exclusion_guard_acquire(main_owner),
			"main-owner lock acquisition failed before owner-loss probe");
		require(sql_economic_runtime_start(),
			"runtime owner failed before owner-loss probe");
		const unsigned long main_session = mysql_thread_id(main_owner);
		const unsigned long control_session = opened_control_session;
		DB = main_owner;
		require(scalar(observer, boot_owner_query()) == std::to_string(control_session),
			"dedicated control lock missing before main-owner loss");
		execute(observer, "KILL CONNECTION " + std::to_string(main_session));
		require(scalar(observer, exclusion_owner_query()) == "0",
			"killed main session still owns its exclusion lock");
		require(scalar(observer, boot_owner_query()) == std::to_string(control_session),
			"main-owner loss unexpectedly killed the dedicated control session");
		probe = connect_fixture();
		require(!duris_sql_exclusion_guard_allows(probe),
			"main-owner session loss did not fence an independent SQL probe");
		auto &state = duris_sql_exclusion_guard_state_ref();
		require(state.lost, "main-owner session loss did not latch irreversibly");
		require(!production_query("UPDATE pa_runtime_sql_rows SET value=9 WHERE id=1"),
			"production query executor accepted a write after main-owner loss");

		replacement = connect_fixture();
		require(scalar(replacement,
			       std::string("SELECT GET_LOCK(") + exclusion_lock_sql + ",0)") == "1",
			"replacement session could not acquire the released main-owner lock");
		require(!duris_sql_exclusion_guard_allows(replacement) && state.lost,
			"replacement main owner cleared the original process loss latch");
		require(scalar(replacement, std::string("SELECT RELEASE_LOCK(") +
						    exclusion_lock_sql + ")") == "1",
			"replacement main owner did not release its probe lock");

		sql_economic_runtime_shutdown();
		duris_sql_exclusion_guard_release();
		DB = nullptr;
		mysql_close(main_owner);
		main_owner = nullptr;
		mysql_close(probe);
		probe = nullptr;
		mysql_close(replacement);
		replacement = nullptr;
		require_no_locks(observer, "main-owner-loss-cleanup");
		mysql_close(observer);
		observer = nullptr;
		std::puts("PASS main-owner-session-loss irreversible-latch cleanup");
		return 0;
	}
	catch (const std::exception &error)
	{
		DB = nullptr;
		sql_economic_runtime_shutdown();
		duris_sql_exclusion_guard_release();
		if (main_owner)
			mysql_close(main_owner);
		if (probe)
			mysql_close(probe);
		if (replacement)
			mysql_close(replacement);
		if (observer)
			mysql_close(observer);
		std::fprintf(stderr, "FAIL main-owner-session-loss: %s\n", error.what());
		return 1;
	}
}

int query_fence_and_rollback()
{
	MYSQL *observer = nullptr;
	MYSQL *main_owner = nullptr;
	MYSQL *replacement = nullptr;
	try
	{
		observer = connect_fixture();
		prepare_probe_table(observer);
		main_owner = connect_fixture();
		require(duris_sql_exclusion_guard_acquire(main_owner),
			"main-owner lock acquisition failed before query-fence probe");
		require(sql_economic_runtime_start(),
			"runtime owner failed before query-fence probe");
		DB = main_owner;
		require(production_query("START TRANSACTION"),
			"production SQL executor refused a healthy transaction start");
		require(production_query("UPDATE pa_runtime_sql_rows SET value=1 WHERE id=1"),
			"production SQL executor refused a healthy transaction write");
		require(scalar(main_owner, "SELECT value FROM pa_runtime_sql_rows WHERE id=1") ==
				"1",
			"main-owner SQL session did not see its own uncommitted fixture write");
		require(scalar(observer, "SELECT value FROM pa_runtime_sql_rows WHERE id=1") == "0",
			"uncommitted fixture write became visible before owner loss");

		// This is the production loss latch path: release the original lock,
		// then let another real SQL session reacquire it. The process must stay
		// fenced even though the database lock is visible again.
		duris_sql_exclusion_guard_release();
		replacement = connect_fixture();
		require(scalar(replacement,
			       std::string("SELECT GET_LOCK(") + exclusion_lock_sql + ",0)") == "1",
			"replacement owner could not reacquire the released main lock");
		auto &state = duris_sql_exclusion_guard_state_ref();
		require(state.lost && !duris_sql_exclusion_guard_allows(replacement),
			"replacement session cleared the irreversible query-fence latch");

		require(!production_query("COMMIT"),
			"production SQL executor sent COMMIT after authority loss");
		require(!production_query("UPDATE pa_runtime_sql_rows SET value=2 WHERE id=1"),
			"production SQL executor sent an ordinary write after authority loss");
		require(!production_query("ROLLBACK TO SAVEPOINT pa_runtime_sql"),
			"production SQL executor accepted a savepoint rollback after authority loss");
		const char stacked_rollback[] = "ROLLBACK; SELECT 1";
		require(!sql_trace_exec_at({ "pa_runtime_sql_harness.cpp", "stacked", 1 },
					   "pa-runtime-sql", stacked_rollback,
					   sizeof(stacked_rollback) - 1, false, false),
			"production SQL executor accepted stacked rollback SQL after authority loss");
		require(!production_query("ROLLBACK "),
			"production SQL executor accepted a non-literal rollback form");
		require(scalar(observer, "SELECT value FROM pa_runtime_sql_rows WHERE id=1") == "0",
			"fenced COMMIT or UPDATE changed committed fixture rows");
		require(production_query("ROLLBACK"),
			"production SQL executor refused the one literal rollback exception");
		require(scalar(observer, "SELECT value FROM pa_runtime_sql_rows WHERE id=1") == "0",
			"literal rollback failed to preserve the committed fixture row");
		require(scalar(main_owner, "SELECT value FROM pa_runtime_sql_rows WHERE id=1") ==
				"0",
			"literal rollback did not discard the main owner's uncommitted row");
		require(scalar(replacement, std::string("SELECT RELEASE_LOCK(") +
						    exclusion_lock_sql + ")") == "1",
			"replacement lock probe did not clean up");

		sql_economic_runtime_shutdown();
		DB = nullptr;
		mysql_close(main_owner);
		main_owner = nullptr;
		mysql_close(replacement);
		replacement = nullptr;
		require_no_locks(observer, "query-fence-cleanup");
		execute(observer, "DROP TABLE pa_runtime_sql_rows");
		mysql_close(observer);
		observer = nullptr;
		std::puts("PASS query-fence COMMIT-refusal literal-ROLLBACK row-preservation");
		return 0;
	}
	catch (const std::exception &error)
	{
		DB = nullptr;
		sql_economic_runtime_shutdown();
		duris_sql_exclusion_guard_release();
		if (main_owner)
			mysql_close(main_owner);
		if (replacement)
			mysql_close(replacement);
		if (observer)
			mysql_close(observer);
		std::fprintf(stderr, "FAIL query-fence: %s\n", error.what());
		return 1;
	}
}

int run_scenario(const char *scenario)
{
	if (mysql_library_init(0, nullptr, nullptr))
		throw std::runtime_error("mysql_library_init failed");
	if (std::strcmp(scenario, "--healthy") == 0)
		return healthy_binding_and_cleanup();
	if (std::strcmp(scenario, "--wrong-binding") == 0)
		return wrong_binding_and_cleanup();
	if (std::strcmp(scenario, "--control-loss") == 0)
		return control_session_loss();
	if (std::strcmp(scenario, "--main-loss") == 0)
		return main_owner_session_loss();
	if (std::strcmp(scenario, "--query-fence") == 0)
		return query_fence_and_rollback();
	throw std::runtime_error("unknown scenario");
}
} // namespace

// Only the configured-connection factory is replaced with an explicitly
// disposable loopback connector; production runtime-owner and SQL guard code
// remain linked and execute against real server sessions.
MYSQL *pa_runtime_sql_test_open_connection(unsigned long client_flags)
{
	if (client_flags != 0)
		return nullptr;
	MYSQL *connection = connect_fixture();
	opened_control_session = mysql_thread_id(connection);
	return connection;
}

// sql.c's production SQL tracing path also logs query failures. Keep this
// harness independent of the game logger while retaining its real executor.
void logit(const char *, const char *, ...) {}

int main(int argc, char **argv)
{
	char executable[4096] = {};
	const ssize_t length = readlink("/proc/self/exe", executable, sizeof(executable) - 1);
	if (length <= 0 || static_cast<size_t>(length) >= sizeof(executable) - 1)
	{
		std::fprintf(stderr, "FAIL could not resolve harness executable\n");
		return 2;
	}
	executable_path.assign(executable, static_cast<size_t>(length));

	if (argc == 3 && std::strcmp(argv[1], "--maintenance-probe") == 0)
	{
		if (mysql_library_init(0, nullptr, nullptr))
			return 2;
		return maintenance_probe(argv[2]);
	}
	if (argc == 2)
	{
		try
		{
			const int status = run_scenario(argv[1]);
			mysql_library_end();
			return status;
		}
		catch (const std::exception &error)
		{
			mysql_library_end();
			std::fprintf(stderr, "FAIL %s: %s\n", argv[1], error.what());
			return 1;
		}
	}
	if (argc != 1)
	{
		std::fprintf(stderr, "usage: pa-runtime-sql [scenario]\n");
		return 2;
	}

	try
	{
		for (const char *scenario : { "--healthy", "--wrong-binding", "--control-loss",
					      "--main-loss", "--query-fence" })
			run_child_mode(scenario, "");
	}
	catch (const std::exception &error)
	{
		std::fprintf(stderr, "FAIL SQL runtime-authority scenarios: %s\n", error.what());
		return 1;
	}
	std::puts("PASS all SQL runtime-authority scenarios");
	return 0;
}
