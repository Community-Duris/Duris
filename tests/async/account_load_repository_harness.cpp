#include "account/account_load.h"
#include "sql/sql_exclusion_guard.h"
#include "sql/sql_pool.h"
#include "sql/sql_telemetry_account_identity.h"
#include <mysql/mysql.h>

#include <cassert>
#include <chrono>
#include <cstring>
#include <map>
#include <string>
#include <thread>
#include <vector>

// Replace the client query surface, never the repository under test. Transaction
// state and after-images model commit/rollback while all SQL is checked/executed
// by the production loader. There is no network or configured database access.
static MYSQL connection = {};
static int step = 0, fail_at = 0, repairs = 0, committed = 0, rolled_back = 0;
static int lose_owner_after = 0;
static unsigned int error_code = 0, injected_error = 1064;
static bool rollback_fails = false, missing = false, empty = false;
static bool discarded = false;
static int telemetry_mode = 0, telemetry_calls = 0;
static size_t ip_rows = 1, character_rows = 1;
static size_t email_bytes = 0;
static my_ulonglong affected = 1;
static std::string last_query;
struct fixture_rows
{
	std::vector<std::vector<std::string>> values;
	std::vector<char *> pointers;
	size_t next = 0;
};
static std::map<MYSQL_RES *, fixture_rows> results;

bool sql_prepare_telemetry_account_token_on(MYSQL *db, const char *name, uint64_t environment,
					    uint64_t season, uint64_t deadline, uint64_t *token)
{
	assert(db == &connection && !strcmp(name, "Example"));
	assert(environment == 7 && season == 11 && account_load_now_usec() < deadline);
	assert(committed == 4 && !repairs && results.empty() && step == 10);
	assert(connection.server_status == SERVER_STATUS_AUTOCOMMIT);
	++telemetry_calls;
	*token = 99; // A failed helper must not publish even a partially supplied token.
	if (telemetry_mode == 2)
		throw std::bad_alloc{};
	if (telemetry_mode == 3)
		error_code = 2006;
	if (telemetry_mode == 4)
		connection.server_status |= SERVER_STATUS_IN_TRANS;
	return telemetry_mode == 0;
}

extern "C" int mysql_query(MYSQL *db, const char *query)
{
	assert(db == &connection);
	last_query = query;
	error_code = 0;
	if (last_query == "ROLLBACK")
	{
		++rolled_back;
		if (rollback_fails)
		{
			error_code = 2006;
			return 1;
		}
		repairs = 0;
		connection.server_status = SERVER_STATUS_AUTOCOMMIT;
		return 0;
	}
	++step;
	if (step == fail_at)
	{
		error_code = injected_error;
		return 1;
	}
	if (last_query == "START TRANSACTION")
		connection.server_status |= SERVER_STATUS_IN_TRANS;
	else if (last_query.starts_with("INSERT INTO"))
	{
		assert(connection.server_status & SERVER_STATUS_IN_TRANS);
		++repairs;
		assert(last_query.find("tombstone.deleted_at IS NOT NULL") != std::string::npos);
		assert(last_query.find("pd.active=1") != std::string::npos);
	}
	else if (last_query == "COMMIT")
	{
		assert(repairs == 4);
		committed += repairs;
		repairs = 0;
		connection.server_status = SERVER_STATUS_AUTOCOMMIT;
	}
	if (step == lose_owner_after)
		duris_sql_exclusion_guard_state_ref().lost = true;
	return 0;
}
extern "C" unsigned long mysql_real_escape_string(MYSQL *, char *out, const char *input,
						  unsigned long length)
{
	memcpy(out, input, length);
	out[length] = '\0';
	return length;
}
extern "C" unsigned int mysql_errno(MYSQL *)
{
	return error_code;
}
extern "C" my_ulonglong mysql_affected_rows(MYSQL *)
{
	return affected;
}
extern "C" MYSQL_RES *mysql_store_result(MYSQL *)
{
	auto *result = new MYSQL_RES{};
	auto &rows = results[result].values;
	if (last_query.find("FROM accounts ") != std::string::npos)
	{
		assert(last_query.ends_with("LIMIT 1 FOR UPDATE"));
		if (!missing)
			rows.push_back({ "Example", std::string(email_bytes, 'e'), "hash", "code",
					 "1", "0", "2", "100", "101", "102", "3", "4", "5", "6" });
	}
	else if (last_query.find("FROM account_ips ") != std::string::npos)
	{
		assert(last_query.ends_with("LIMIT " + std::to_string(ACCOUNT_LOAD_MAX_IPS + 1)));
		assert(repairs == 4);
		for (size_t i = 0; i < ip_rows; ++i)
			rows.push_back({ "host", "127.0.0.1", "9" });
	}
	else
	{
		assert(last_query.find("ac.deleted_at IS NULL LIMIT ") != std::string::npos);
		assert(last_query.ends_with("LIMIT " +
					    std::to_string(ACCOUNT_LOAD_MAX_CHARACTERS + 1)));
		assert(repairs == 4);
		if (!empty)
			for (size_t i = 0; i < character_rows; ++i)
				rows.push_back({ "42", "Hero", "9", "103", "1", "2", "50", "8", "4",
						 "16", "1200", "104" });
	}
	return result;
}
extern "C" MYSQL_ROW mysql_fetch_row(MYSQL_RES *result)
{
	auto &rows = results.at(result);
	if (rows.next == rows.values.size())
		return nullptr;
	rows.pointers.clear();
	for (auto &value : rows.values[rows.next++])
		rows.pointers.push_back(value.data());
	return rows.pointers.data();
}
extern "C" void mysql_free_result(MYSQL_RES *result)
{
	assert(results.erase(result) == 1);
	delete result;
}
extern "C" MYSQL *sql_pool_acquire()
{
	return &connection;
}
extern "C" void sql_pool_release(MYSQL *db)
{
	assert(db == &connection);
}
extern "C" void sql_pool_discard_connection(MYSQL *db)
{
	assert(db == &connection);
	discarded = true;
}
static void reset()
{
	assert(results.empty());
	connection = {};
	connection.server_status = SERVER_STATUS_AUTOCOMMIT;
	step = fail_at = repairs = committed = rolled_back = 0;
	telemetry_mode = telemetry_calls = 0;
	lose_owner_after = 0;
	duris_sql_exclusion_guard_state_ref().lost = false;
	error_code = 0;
	injected_error = 1064;
	rollback_fails = missing = empty = discarded = false;
	ip_rows = character_rows = 1;
	email_bytes = 0;
	affected = 1;
}
static account_load_request request()
{
	return { account_load_next_id(), account_load_now_usec() + ACCOUNT_LOAD_TIMEOUT_USEC,
		 "Example" };
}
int main()
{
	reset();
	auto result = account_load_execute(&connection, request());
	assert(result.outcome == account_load_outcome::loaded && committed == 4 && !rolled_back);
	assert(result.snapshot.name == "Example" && result.snapshot.password == "hash");
	assert(result.snapshot.blocked == 2 && result.snapshot.confirmed == 1);
	assert(result.snapshot.flags[3] == 6 && result.snapshot.ips.at(0).count == 9);
	assert(result.snapshot.characters.at(0).secondary_class == 16);
	assert(result.snapshot.characters.at(0).last_save == 104);
	assert(!telemetry_calls && !result.snapshot.telemetry_account_token &&
	       !result.snapshot.telemetry_environment_id && !result.snapshot.telemetry_season_id);
	for (int optional = 0; optional < 3; ++optional)
	{
		reset();
		telemetry_mode = optional;
		auto scoped = request();
		scoped.telemetry_environment_id = 7;
		scoped.telemetry_season_id = 11;
		result = account_load_execute(&connection, scoped);
		assert(result.outcome == account_load_outcome::loaded && committed == 4 &&
		       !rolled_back && telemetry_calls == 1);
		assert(result.snapshot.password == "hash" &&
		       result.snapshot.characters.size() == 1);
		assert(result.snapshot.telemetry_account_token == (optional == 0 ? 99U : 0U));
		assert(result.snapshot.telemetry_environment_id == (optional == 0 ? 7U : 0U));
		assert(result.snapshot.telemetry_season_id == (optional == 0 ? 11U : 0U));
	}
	// Shared synchronous repair must preserve the original SQL ownership fence.
	reset();
	duris_sql_exclusion_guard_state_ref().lost = true;
	assert(account_load_repair(&connection, "Example") == -1);
	assert(!step && !repairs);
	// Pool admission is not enough: ownership can disappear during any query.
	// No later write or COMMIT may run, but rollback must remain available.
	for (int loss = 0; loss < 10; ++loss)
	{
		reset();
		if (loss == 0)
			duris_sql_exclusion_guard_state_ref().lost = true;
		else
			lose_owner_after = loss;
		result = account_load_execute(&connection, request());
		assert(result.outcome != account_load_outcome::loaded);
		assert(result.snapshot.name.empty() && result.snapshot.characters.empty());
		assert(step == loss && !committed && !repairs);
		assert(rolled_back == (loss >= 2));
		assert(!(connection.server_status & SERVER_STATUS_IN_TRANS));
	}
	// Every required query (including transaction setup and commit) fails closed.
	for (int failure = 1; failure <= 10; ++failure)
	{
		reset();
		fail_at = failure;
		result = account_load_execute(&connection, request());
		assert(result.outcome == (failure >= 4 && failure <= 7 ?
						  account_load_outcome::repair_failed :
						  account_load_outcome::load_failed));
		assert(result.snapshot.name.empty() && result.snapshot.characters.empty());
		assert(!committed && !repairs);
		assert(rolled_back == (failure > 2));
		assert(!(connection.server_status & SERVER_STATUS_IN_TRANS));
	}
	reset();
	missing = true;
	result = account_load_execute(&connection, request());
	assert(result.outcome == account_load_outcome::not_found && step == 3);
	assert(rolled_back == 1 && !committed);
	reset();
	empty = true;
	affected = 0;
	result = account_load_execute(&connection, request());
	assert(result.outcome == account_load_outcome::loaded &&
	       result.snapshot.characters.empty());
	reset();
	empty = true;
	result = account_load_execute(&connection, request());
	assert(result.outcome == account_load_outcome::load_failed && rolled_back && !committed);
	for (int limit = 0; limit < 3; ++limit)
	{
		reset();
		if (limit == 0)
			ip_rows = ACCOUNT_LOAD_MAX_IPS + 1;
		else if (limit == 1)
			character_rows = ACCOUNT_LOAD_MAX_CHARACTERS + 1;
		else
			email_bytes = ACCOUNT_LOAD_MAX_BYTES + 1;
		result = account_load_execute(&connection, request());
		assert(result.outcome == account_load_outcome::limit_exceeded);
		assert(!committed && !repairs && rolled_back == 1 && result.snapshot.name.empty());
	}
	reset();
	auto expired = request();
	expired.deadline_usec = account_load_now_usec() - 1;
	result = account_load_execute(&connection, expired);
	assert(result.outcome == account_load_outcome::timed_out && !step && !rolled_back);

	// Use the default worker executor to verify pool retirement after a transport
	// error and after rollback fails, even when rollback cleared the first errno.
	for (int failure = 0; failure < 2; ++failure)
	{
		reset();
		fail_at = 5;
		if (failure == 0)
			injected_error = 2006;
		else
			rollback_fails = true;
		assert(account_load_worker_init());
		auto *job = account_load_submit(request());
		assert(job);
		const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(3);
		while (!account_load_poll(job, &result))
		{
			assert(std::chrono::steady_clock::now() < deadline);
			std::this_thread::sleep_for(std::chrono::milliseconds(1));
		}
		assert(discarded && result.outcome == account_load_outcome::load_failed &&
		       result.snapshot.name.empty());
		account_load_release(job);
		account_load_worker_shutdown();
	}
	assert(results.empty());
	// Optional telemetry transport/transaction failures retire the worker handle,
	// while preserving the already committed account snapshot for login.
	for (int optional = 3; optional < 5; ++optional)
	{
		reset();
		telemetry_mode = optional;
		auto scoped = request();
		scoped.telemetry_environment_id = 7;
		scoped.telemetry_season_id = 11;
		assert(account_load_worker_init());
		auto *job = account_load_submit(scoped);
		assert(job);
		const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(3);
		while (!account_load_poll(job, &result))
		{
			assert(std::chrono::steady_clock::now() < deadline);
			std::this_thread::sleep_for(std::chrono::milliseconds(1));
		}
		assert(discarded && telemetry_calls == 1 && committed == 4 &&
		       result.outcome == account_load_outcome::loaded);
		assert(result.snapshot.password == "hash" &&
		       result.snapshot.characters.size() == 1);
		assert(!result.snapshot.telemetry_account_token &&
		       !result.snapshot.telemetry_environment_id &&
		       !result.snapshot.telemetry_season_id);
		account_load_release(job);
		account_load_worker_shutdown();
	}
	puts("account load owned snapshot, query failures, rollback, limits and pool retirement passed");
}
