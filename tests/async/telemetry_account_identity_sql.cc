#include "account/account_load.h"
#include "sql/sql.h"
#include "sql/sql_exclusion_guard.h"
#include "sql/sql_transaction.h"
#include "sql/sql_telemetry_account_identity.h"

#include <openssl/rand.h>

#include <cassert>
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <new>
#include <string>
#include <thread>

MYSQL *DB = nullptr;
static bool transaction_active;
static unsigned begins, commits, rollbacks, queries, random_calls;
static unsigned last_error;
static std::string mode;
static std::uint64_t collision;
static MYSQL *worker_connection;
static bool owned, load_snapshot;
static unsigned owned_commits;

void *operator new(std::size_t bytes)
{
	if (transaction_active && mode == "allocation_failure")
		throw std::bad_alloc{};
	void *allocation = std::malloc(bytes);
	if (!allocation)
		throw std::bad_alloc{};
	return allocation;
}

void operator delete(void *allocation) noexcept
{
	std::free(allocation);
}
void operator delete(void *allocation, std::size_t) noexcept
{
	std::free(allocation);
}

/* Execute the production allocator against the real engine. These boundaries
 * inject only native transaction outcomes and entropy; no allocator SQL or
 * identity decisions are reproduced in the harness. */
bool sql_begin_transaction()
{
	assert(!owned);
	++begins;
	if (transaction_active || mysql_query(DB, "START TRANSACTION"))
		return false;
	transaction_active = true;
	return true;
}

bool sql_commit()
{
	assert(!owned);
	++commits;
	if (mode == "commit_failure")
		return false;
	if (mysql_commit(DB))
		return false;
	if (mode == "lost_commit_reply")
		return false;
	transaction_active = false;
	return true;
}

bool sql_rollback()
{
	assert(!owned);
	++rollbacks;
	transaction_active = false;
	return mysql_rollback(DB) == 0;
}

bool sql_in_transaction()
{
	assert(!owned);
	return transaction_active;
}

bool sql_observed_execute_at(MYSQL *connection, persistence_query_site,
			     persistence_query_context context, const char *statement, size_t bytes,
			     std::uint64_t *)
{
	assert(connection == (owned ? worker_connection : DB));
	assert(context == (owned ? PERSISTENCE_QUERY_CONTEXT_PLAYER_LOAD_WORKER :
				   PERSISTENCE_QUERY_CONTEXT_MAIN));
	const std::string query(statement, bytes);
	++queries;
	if (mode == "token_write_failure" &&
	    query.starts_with("INSERT INTO telemetry_account_token"))
		return mysql_query(connection, "INVALID TELEMETRY FIXTURE STATEMENT") == 0;
	if (owned && query == "COMMIT")
	{
		++owned_commits;
		if (mode == "commit_failure")
			return false;
	}
	const int status = mysql_real_query(connection, statement, bytes);
	if (status)
		last_error = mysql_errno(connection);
	if (owned && status == 0)
	{
		if (query == "START TRANSACTION")
		{
			transaction_active = true;
			if (mode == "expire_after_begin")
				std::this_thread::sleep_for(std::chrono::milliseconds(150));
		}
		if (query == "COMMIT")
		{
			transaction_active = false;
			if (mode == "lost_commit_reply")
				return false;
		}
	}
	return status == 0;
}

char *sql_escape_string(const char *text)
{
	assert(!owned);
	const auto bytes = std::strlen(text);
	char *result = static_cast<char *>(std::malloc(bytes * 2U + 1U));
	assert(result);
	const auto escaped = mysql_real_escape_string(DB, result, text, bytes);
	result[escaped] = '\0';
	return result;
}

extern "C" int RAND_bytes(unsigned char *buffer, int bytes)
{
	++random_calls;
	assert(bytes == sizeof(std::uint64_t));
	if (mode == "entropy_failure")
		return 0;
	if (mode == "zero_entropy")
	{
		std::memset(buffer, 0, bytes);
		return 1;
	}
	if (mode == "collision")
	{
		std::memcpy(buffer, &collision, bytes);
		return 1;
	}
	return RAND_priv_bytes(buffer, bytes);
}

int main(int argc, char **argv)
{
	assert(argc == 6);
	mode = argv[4];
	load_snapshot = mode.starts_with("load_");
	owned = load_snapshot || mode.starts_with("worker_");
	if (owned)
		mode.erase(0, load_snapshot ? 5U : 7U);
	collision = std::strtoull(argv[5], nullptr, 10);
	DB = mysql_init(nullptr);
	assert(DB);
	unsigned timeout = 5U;
	mysql_options(DB, MYSQL_OPT_CONNECT_TIMEOUT, &timeout);
	mysql_options(DB, MYSQL_OPT_READ_TIMEOUT, &timeout);
	mysql_options(DB, MYSQL_OPT_WRITE_TIMEOUT, &timeout);
	assert(mysql_real_connect(
		DB, std::getenv("TELEMETRY_REPOSITORY_HOST"),
		std::getenv("TELEMETRY_IDENTITY_USER"), std::getenv("TELEMETRY_IDENTITY_PASSWORD"),
		std::getenv("TELEMETRY_REPOSITORY_DATABASE"),
		std::atoi(std::getenv("TELEMETRY_REPOSITORY_PORT")), nullptr, 0U));
	assert(mysql_set_character_set(DB, "utf8mb4") == 0);
	if (owned)
	{
		worker_connection = mysql_init(nullptr);
		assert(worker_connection && worker_connection != DB);
		mysql_options(worker_connection, MYSQL_OPT_CONNECT_TIMEOUT, &timeout);
		mysql_options(worker_connection, MYSQL_OPT_READ_TIMEOUT, &timeout);
		mysql_options(worker_connection, MYSQL_OPT_WRITE_TIMEOUT, &timeout);
		assert(mysql_real_connect(
			worker_connection, std::getenv("TELEMETRY_REPOSITORY_HOST"),
			load_snapshot ? std::getenv("TELEMETRY_REPOSITORY_USER") :
					std::getenv("TELEMETRY_IDENTITY_USER"),
			load_snapshot ? std::getenv("TELEMETRY_REPOSITORY_PASSWORD") :
					std::getenv("TELEMETRY_IDENTITY_PASSWORD"),
			std::getenv("TELEMETRY_REPOSITORY_DATABASE"),
			std::atoi(std::getenv("TELEMETRY_REPOSITORY_PORT")), nullptr, 0U));
		assert(mysql_set_character_set(worker_connection, "utf8mb4") == 0);
	}
	if (!owned && mode == "outer_transaction")
		assert(sql_begin_transaction());
	if (owned && mode == "outer_transaction")
		assert(mysql_query(worker_connection, "START TRANSACTION") == 0);
	if (owned && mode == "autocommit_off")
		assert(mysql_autocommit(worker_connection, 0) == 0);
	if (owned && mode == "owner_lost")
		duris_sql_exclusion_guard_state_ref().lost = true;
	std::uint64_t token = UINT64_MAX;
	const auto environment = std::strtoull(argv[2], nullptr, 10);
	const auto season = std::strtoull(argv[3], nullptr, 10);
	const auto now = std::chrono::duration_cast<std::chrono::microseconds>(
				 std::chrono::steady_clock::now().time_since_epoch())
				 .count();
	const uint64_t deadline =
		mode == "expired" ?
			now - 1 :
			now + (mode == "expire_after_begin" ? 100000U : ACCOUNT_LOAD_TIMEOUT_USEC);
	bool accepted;
	account_load_result snapshot;
	if (load_snapshot)
	{
		account_load_request request{ 1U, deadline, argv[1], environment, season };
		snapshot = account_load_execute(worker_connection, request);
		token = snapshot.snapshot.telemetry_account_token;
		accepted = token != 0;
		if (snapshot.outcome == account_load_outcome::loaded)
			assert(snapshot.snapshot.name == argv[1] &&
			       snapshot.snapshot.email == "async@example.invalid" &&
			       snapshot.snapshot.password == "synthetic-worker-hash");
		assert(!accepted || (snapshot.snapshot.telemetry_environment_id == environment &&
				     snapshot.snapshot.telemetry_season_id == season));
		assert(accepted || (!snapshot.snapshot.telemetry_environment_id &&
				    !snapshot.snapshot.telemetry_season_id));
	}
	else if (owned)
		accepted = sql_prepare_telemetry_account_token_on(
			worker_connection, argv[1], environment, season, deadline, &token);
	else
		accepted =
			sql_prepare_telemetry_account_token(argv[1], environment, season, &token);
	const bool outer_retained =
		owned ? (worker_connection->server_status & SERVER_STATUS_IN_TRANS) :
			transaction_active;
	if (!owned && transaction_active)
		assert(sql_rollback());
	if (owned)
	{
		assert(!begins && !commits && !rollbacks);
		assert(!(DB->server_status & SERVER_STATUS_IN_TRANS));
		assert(mysql_rollback(worker_connection) == 0);
		mysql_close(worker_connection);
	}
	std::printf(
		"{\"accepted\":%s,\"token\":%llu,\"entropy_calls\":%u,\"begins\":%u,"
		"\"commits\":%u,\"rollbacks\":%u,\"queries\":%u,\"outer_retained\":%s,\"last_error\":%u,"
		"\"owned_commits\":%u,\"loaded\":%s}\n",
		accepted ? "true" : "false", static_cast<unsigned long long>(token), random_calls,
		begins, commits, rollbacks, queries, outer_retained ? "true" : "false", last_error,
		owned_commits,
		load_snapshot && snapshot.outcome == account_load_outcome::loaded ? "true" :
										    "false");
	mysql_close(DB);
	DB = nullptr;
}
