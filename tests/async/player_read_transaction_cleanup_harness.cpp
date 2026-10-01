#include "player/player_death_recovery_query.h"
#include "player/player_load_pipeline.h"
#include "player/player_save_pipeline.h"
#include "item/item_movement_transaction.h"
#include "persistence/persistence_observability.h"
#include "sql/sql_pool.h"

#include <cassert>
#include <cerrno>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <condition_variable>
#include <mutex>
#include <new>
#include <string>
#include <thread>
#include <unordered_map>
#include <unordered_set>
#include <vector>

bool player_save_journal_pid_quarantined(int)
{
	return false;
}
bool item_movement_transaction_pending_spell_effects(uint32_t,
						     std::vector<critical_operation_id> *operations)
{
	operations->clear();
	return true;
}

namespace
{
struct fake_result
{
	std::vector<std::string> row = { "71" };
	char *values[1] = {};
};

std::unordered_map<MYSQL *, std::string> last_sql;
std::unordered_map<MYSQL *, unsigned int> errors;
std::unordered_set<uintptr_t> closed_connections;
std::mutex close_mutex;
MYSQL *fail_start_connection = nullptr;
MYSQL *fail_rollback_connection = nullptr;
MYSQL *fail_commit_connection = nullptr;
MYSQL *throw_store_result_connection = nullptr;
bool fail_list = false;
bool throw_load_repository = false;
bool ambiguous_load_result = false;

void set_error(MYSQL *connection, unsigned int error)
{
	errors[connection] = error;
}

uintptr_t connection_address(MYSQL *connection)
{
	return reinterpret_cast<uintptr_t>(connection);
}

player_death_recovery_query_request list_request()
{
	player_death_recovery_query_request request = {};
	request.kind = player_death_recovery_query_kind::list;
	return request;
}

void check_connection_retired(MYSQL *bad, MYSQL *healthy)
{
	const uintptr_t bad_address = connection_address(bad);
	sql_pool_release(bad);
	assert(closed_connections.count(bad_address) == 1);
	assert(sql_pool_available() == 0);
	sql_pool_release(healthy);
	MYSQL *borrowed = sql_pool_acquire();
	assert(borrowed == healthy);
	assert(connection_address(borrowed) != bad_address);
	sql_pool_release(borrowed);
}

void begin_pool(MYSQL **first, MYSQL **second)
{
	closed_connections.clear();
	assert(sql_pool_init(2) == 0);
	*first = sql_pool_acquire();
	*second = sql_pool_acquire();
	assert(*first && *second && *first != *second);
}

void end_pool()
{
	sql_pool_shutdown();
	fail_start_connection = nullptr;
	fail_rollback_connection = nullptr;
	fail_commit_connection = nullptr;
	throw_store_result_connection = nullptr;
	fail_list = false;
	throw_load_repository = false;
	ambiguous_load_result = false;
}
} // namespace

MYSQL *sql_open_configured_connection(unsigned long)
{
	MYSQL *connection = static_cast<MYSQL *>(std::calloc(1, sizeof(MYSQL)));
	assert(connection);
	connection->server_status = SERVER_STATUS_AUTOCOMMIT;
	return connection;
}

void logit(const char *, const char *, ...) {}

extern "C" void mysql_close(MYSQL *connection)
{
	std::lock_guard<std::mutex> lock(close_mutex);
	closed_connections.insert(connection_address(connection));
	last_sql.erase(connection);
	errors.erase(connection);
	std::free(connection);
}

extern "C" int mysql_real_query(MYSQL *connection, const char *sql, unsigned long length)
{
	last_sql[connection].assign(sql, length);
	set_error(connection, 0);
	if (last_sql[connection] == "SET TRANSACTION ISOLATION LEVEL REPEATABLE READ")
		return 0;
	if (last_sql[connection].find("START TRANSACTION") == 0)
	{
		connection->server_status = SERVER_STATUS_AUTOCOMMIT | SERVER_STATUS_IN_TRANS;
		if (connection == fail_start_connection)
		{
			set_error(connection,
				  2013); // Reply lost after the server started the snapshot.
			return 1;
		}
		return 0;
	}
	if (last_sql[connection] == "ROLLBACK")
	{
		if (connection == fail_rollback_connection)
		{
			set_error(connection, 2013);
			return 1;
		}
		connection->server_status = SERVER_STATUS_AUTOCOMMIT;
		return 0;
	}
	if (last_sql[connection] == "COMMIT")
	{
		connection->server_status = SERVER_STATUS_AUTOCOMMIT;
		if (connection == fail_commit_connection)
		{
			set_error(connection,
				  2013); // Commit may have happened despite the lost reply.
			return 1;
		}
		return 0;
	}
	return 0;
}

extern "C" int mysql_query(MYSQL *connection, const char *sql)
{
	return mysql_real_query(connection, sql, std::strlen(sql));
}

extern "C" MYSQL_RES *mysql_store_result(MYSQL *connection)
{
	if (last_sql[connection].find("SELECT ac.pid FROM account_characters") == std::string::npos)
		return nullptr;
	if (connection == throw_store_result_connection)
	{
		throw_store_result_connection = nullptr;
		throw std::bad_alloc();
	}
	auto *result = new fake_result();
	result->values[0] = result->row[0].data();
	return reinterpret_cast<MYSQL_RES *>(result);
}

extern "C" unsigned int mysql_field_count(MYSQL *)
{
	return 0;
}

extern "C" unsigned int mysql_num_fields(MYSQL_RES *)
{
	return 1;
}

extern "C" my_ulonglong mysql_num_rows(MYSQL_RES *)
{
	return 1;
}

extern "C" MYSQL_ROW mysql_fetch_row(MYSQL_RES *rows)
{
	return reinterpret_cast<fake_result *>(rows)->values;
}

extern "C" void mysql_free_result(MYSQL_RES *rows)
{
	delete reinterpret_cast<fake_result *>(rows);
}

extern "C" unsigned long mysql_real_escape_string(MYSQL *, char *output, const char *input,
						  unsigned long length)
{
	std::memcpy(output, input, length);
	output[length] = '\0';
	return length;
}

extern "C" unsigned int mysql_errno(MYSQL *connection)
{
	return errors[connection];
}

extern "C" const char *mysql_sqlstate(MYSQL *)
{
	return "HY000";
}

bool critical_operation_id_is_zero(const critical_operation_id &id)
{
	for (uint8_t byte : id.bytes)
		if (byte)
			return false;
	return true;
}

player_death_conflict_result player_death_conflict_read(MYSQL *, int32_t,
							const critical_operation_id &,
							player_snapshot *) noexcept
{
	return { player_death_conflict_outcome::not_found, ENOENT, 0 };
}

player_death_conflict_result
player_death_conflict_list(MYSQL *, int32_t, player_revision_t,
			   std::vector<player_death_conflict_case> *output) noexcept
{
	if (fail_list)
		return { player_death_conflict_outcome::failed, EIO, 0 };
	player_death_conflict_case entry = {};
	entry.save_revision = 22;
	entry.source_revision = 21;
	output->push_back(entry);
	return { player_death_conflict_outcome::read, 0, 21 };
}

bool player_load_request_valid(const player_load_request &request, uint64_t now)
{
	return request.schema_version == PLAYER_LOAD_SCHEMA_VERSION && request.request_id &&
	       request.pid > 0 && request.deadline_usec > now;
}

player_load_result player_load_repository_execute(MYSQL *connection,
						  const player_load_request &request)
{
	// Explicit transactions retain the session AUTOCOMMIT flag: exercise the
	// IN_TRANS refusal independently from disabled-autocommit detection.
	connection->server_status = ambiguous_load_result ?
					    SERVER_STATUS_AUTOCOMMIT :
					    (SERVER_STATUS_AUTOCOMMIT | SERVER_STATUS_IN_TRANS);
	player_load_result result = {};
	result.request_id = request.request_id;
	result.pid = request.pid;
	result.outcome = player_load_outcome::applied;
	result.error_code = ambiguous_load_result ? 2013 : 0;
	result.snapshot.schema_version = PLAYER_SNAPSHOT_SCHEMA_VERSION;
	result.snapshot.pid = request.pid;
	ambiguous_load_result = false;
	if (throw_load_repository)
	{
		throw_load_repository = false;
		throw std::bad_alloc();
	}
	return result;
}

// This fixture isolates read cleanup with a healthy journal; the actual
// replay fence is exercised by test_death_journal_pipeline_lifecycle.py.
bool player_save_pipeline_loads_allowed(void)
{
	return true;
}

int main()
{
	MYSQL *bad = nullptr;
	MYSQL *healthy = nullptr;

	// A failed START can have reached the server. It must not be returned to a borrower.
	begin_pool(&bad, &healthy);
	fail_start_connection = bad;
	auto start_failure =
		player_death_recovery_query_execute(bad, 71, "owner", "Hero", list_request());
	assert(start_failure.outcome == player_death_recovery_query_outcome::failed);
	assert(start_failure.error_code == 2013);
	assert(bad->server_status & SERVER_STATUS_IN_TRANS);
	check_connection_retired(bad, healthy);
	end_pool();

	// A lease that already carries a transaction is rejected and retired before new SQL runs.
	begin_pool(&bad, &healthy);
	bad->server_status = SERVER_STATUS_IN_TRANS;
	auto preexisting_snapshot =
		player_death_recovery_query_execute(bad, 71, "owner", "Hero", list_request());
	assert(preexisting_snapshot.outcome == player_death_recovery_query_outcome::failed);
	assert(preexisting_snapshot.error_code == EBUSY);
	check_connection_retired(bad, healthy);
	end_pool();

	// An unsuccessful explicit rollback retires its lease; no later borrower sees its snapshot.
	begin_pool(&bad, &healthy);
	fail_list = true;
	fail_rollback_connection = bad;
	auto rollback_failure =
		player_death_recovery_query_execute(bad, 71, "owner", "Hero", list_request());
	assert(rollback_failure.outcome == player_death_recovery_query_outcome::failed);
	assert(rollback_failure.error_code == 2013);
	assert(bad->server_status & SERVER_STATUS_IN_TRANS);
	check_connection_retired(bad, healthy);
	end_pool();

	// A successful rollback leaves the connection reusable and outside a snapshot.
	begin_pool(&bad, &healthy);
	fail_list = true;
	auto rolled_back =
		player_death_recovery_query_execute(bad, 71, "owner", "Hero", list_request());
	assert(rolled_back.outcome == player_death_recovery_query_outcome::failed);
	assert(!(bad->server_status & SERVER_STATUS_IN_TRANS));
	assert(bad->server_status & SERVER_STATUS_AUTOCOMMIT);
	sql_pool_release(bad);
	MYSQL *reused = sql_pool_acquire();
	assert(reused == bad);
	assert(!(reused->server_status & SERVER_STATUS_IN_TRANS));
	sql_pool_release(reused);
	sql_pool_release(healthy);
	end_pool();

	// C++ exceptions unwind through the real transaction destructor. Successful cleanup reuses;
	// failed cleanup retires the connection.
	begin_pool(&bad, &healthy);
	throw_store_result_connection = bad;
	auto exception_rollback =
		player_death_recovery_query_execute(bad, 71, "owner", "Hero", list_request());
	assert(exception_rollback.outcome == player_death_recovery_query_outcome::failed);
	assert(!(bad->server_status & SERVER_STATUS_IN_TRANS));
	sql_pool_release(bad);
	reused = sql_pool_acquire();
	assert(reused == bad);
	sql_pool_release(reused);
	sql_pool_release(healthy);
	end_pool();

	begin_pool(&bad, &healthy);
	throw_store_result_connection = bad;
	fail_rollback_connection = bad;
	auto exception_failed_cleanup =
		player_death_recovery_query_execute(bad, 71, "owner", "Hero", list_request());
	assert(exception_failed_cleanup.outcome == player_death_recovery_query_outcome::failed);
	check_connection_retired(bad, healthy);
	end_pool();

	// A lost COMMIT response may follow a successful commit. Do not publish its buffered rows.
	begin_pool(&bad, &healthy);
	fail_commit_connection = bad;
	auto commit_ambiguity =
		player_death_recovery_query_execute(bad, 71, "owner", "Hero", list_request());
	assert(commit_ambiguity.outcome == player_death_recovery_query_outcome::failed);
	assert(commit_ambiguity.cases.empty());
	assert(commit_ambiguity.error_code == 2013);
	check_connection_retired(bad, healthy);
	end_pool();

	// Cold-load wrapper must reject a plausible result if its repository leaves the snapshot open.
	begin_pool(&bad, &healthy);
	player_load_request load_request = {};
	load_request.request_id = 501;
	load_request.pid = 71;
	load_request.account_name = "owner";
	load_request.deadline_usec =
		persistence_observability_now_usec() + PLAYER_LOAD_TIMEOUT_USEC;
	player_load_result load_result = {};
	const uintptr_t dirty_load_address = connection_address(bad);
	sql_pool_release(bad);
	assert(player_load_pipeline_execute_sync(load_request, &load_result));
	assert(load_result.request_id == load_request.request_id);
	assert(load_result.pid == load_request.pid);
	assert(load_result.outcome == player_load_outcome::retryable_failure);
	assert(load_result.snapshot.schema_version == 0);
	assert(load_result.snapshot.pid == 0);
	assert(load_result.error_code == EIO);
	assert(closed_connections.count(dirty_load_address) == 1);
	assert(sql_pool_available() == 0);
	sql_pool_release(healthy);
	MYSQL *load_reacquire = sql_pool_acquire();
	assert(load_reacquire == healthy);
	assert(connection_address(load_reacquire) != dirty_load_address);
	sql_pool_release(load_reacquire);
	end_pool();

	// Even when the status flags look clean, a lost transaction response invalidates returned data.
	begin_pool(&bad, &healthy);
	load_request.request_id = 503;
	load_request.deadline_usec =
		persistence_observability_now_usec() + PLAYER_LOAD_TIMEOUT_USEC;
	const uintptr_t ambiguous_load_address = connection_address(bad);
	sql_pool_release(bad);
	ambiguous_load_result = true;
	load_result = {};
	assert(player_load_pipeline_execute_sync(load_request, &load_result));
	assert(load_result.outcome == player_load_outcome::retryable_failure);
	assert(load_result.snapshot.schema_version == 0);
	assert(load_result.snapshot.pid == 0);
	assert(load_result.error_code == 2013);
	assert(closed_connections.count(ambiguous_load_address) == 1);
	assert(sql_pool_available() == 0);
	sql_pool_release(healthy);
	load_reacquire = sql_pool_acquire();
	assert(load_reacquire == healthy);
	assert(connection_address(load_reacquire) != ambiguous_load_address);
	sql_pool_release(load_reacquire);
	end_pool();

	// An exception after the repository begins its transaction discards the lease in the catch path.
	begin_pool(&bad, &healthy);
	load_request.request_id = 502;
	load_request.deadline_usec =
		persistence_observability_now_usec() + PLAYER_LOAD_TIMEOUT_USEC;
	const uintptr_t exception_load_address = connection_address(bad);
	sql_pool_release(bad);
	throw_load_repository = true;
	load_result = {};
	assert(player_load_pipeline_execute_sync(load_request, &load_result));
	assert(load_result.outcome == player_load_outcome::retryable_failure);
	assert(load_result.error_code == ENOMEM);
	assert(load_result.snapshot.schema_version == 0);
	assert(closed_connections.count(exception_load_address) == 1);
	assert(sql_pool_available() == 0);
	sql_pool_release(healthy);
	load_reacquire = sql_pool_acquire();
	assert(load_reacquire == healthy);
	assert(connection_address(load_reacquire) != exception_load_address);
	sql_pool_release(load_reacquire);
	end_pool();

	// A discard requested during shutdown must still return the borrowed slot to the closer.
	begin_pool(&bad, &healthy);
	const uintptr_t shutdown_bad_address = connection_address(bad);
	const uintptr_t shutdown_healthy_address = connection_address(healthy);
	std::mutex shutdown_mutex;
	std::condition_variable shutdown_changed;
	bool shutdown_started = false;
	std::thread shutdown_thread(
		[&]
		{
			{
				std::lock_guard<std::mutex> lock(shutdown_mutex);
				shutdown_started = true;
				shutdown_changed.notify_all();
			}
			sql_pool_shutdown();
		});
	{
		std::unique_lock<std::mutex> lock(shutdown_mutex);
		shutdown_changed.wait(lock, [&] { return shutdown_started; });
	}
	sql_pool_discard_connection(bad);
	sql_pool_release(bad);
	sql_pool_release(healthy);
	shutdown_thread.join();
	assert(sql_pool_total() == 0);
	assert(closed_connections.count(shutdown_bad_address) == 1);
	assert(closed_connections.count(shutdown_healthy_address) == 1);
	end_pool();

	return 0;
}
