#include "sql/sql.h"
#include "sql/sql_transaction.h"
#include "sql/sql_telemetry_account_identity.h"

#include <openssl/rand.h>

#include <cassert>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <new>
#include <string>

MYSQL *DB = nullptr;
static bool transaction_active;
static unsigned begins, commits, rollbacks, queries, random_calls;
static unsigned last_error;
static std::string mode;
static std::uint64_t collision;

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
	++begins;
	if (transaction_active || mysql_query(DB, "START TRANSACTION"))
		return false;
	transaction_active = true;
	return true;
}

bool sql_commit()
{
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
	++rollbacks;
	transaction_active = false;
	return mysql_rollback(DB) == 0;
}

bool sql_in_transaction()
{
	return transaction_active;
}

bool sql_observed_execute_at(MYSQL *connection, persistence_query_site, persistence_query_context,
			     const char *statement, size_t bytes, std::uint64_t *)
{
	++queries;
	if (mode == "token_write_failure" &&
	    std::string(statement, bytes).starts_with("INSERT INTO telemetry_account_token"))
		return mysql_query(connection, "INVALID TELEMETRY FIXTURE STATEMENT") == 0;
	const int status = mysql_real_query(connection, statement, bytes);
	if (status)
		last_error = mysql_errno(connection);
	return status == 0;
}

char *sql_escape_string(const char *text)
{
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
	if (mode == "outer_transaction")
		assert(sql_begin_transaction());
	std::uint64_t token = UINT64_MAX;
	const bool accepted =
		sql_prepare_telemetry_account_token(argv[1], std::strtoull(argv[2], nullptr, 10),
						    std::strtoull(argv[3], nullptr, 10), &token);
	const bool outer_retained = transaction_active;
	if (transaction_active)
		assert(sql_rollback());
	std::printf(
		"{\"accepted\":%s,\"token\":%llu,\"entropy_calls\":%u,\"begins\":%u,"
		"\"commits\":%u,\"rollbacks\":%u,\"queries\":%u,\"outer_retained\":%s,\"last_error\":%u}\n",
		accepted ? "true" : "false", static_cast<unsigned long long>(token), random_calls,
		begins, commits, rollbacks, queries, outer_retained ? "true" : "false", last_error);
	mysql_close(DB);
	DB = nullptr;
}
