#include "sql/sql_telemetry_account_identity.h"

#ifndef __NO_MYSQL__
#include "sql/sql.h"
#include "sql/sql_exclusion_guard.h"
#include "sql/sql_transaction.h"

#include <openssl/rand.h>

#include <charconv>
#include <chrono>
#include <cstdlib>
#include <cstring>
#include <memory>
#include <new>
#include <string>

namespace
{

constexpr unsigned ALLOCATION_ATTEMPTS = 4U;
constexpr std::size_t ACCOUNT_NAME_BYTES = 200U;
constexpr unsigned DUPLICATE_KEY = 1062U;

using result_ptr = std::unique_ptr<MYSQL_RES, decltype(&mysql_free_result)>;
using escaped_ptr = std::unique_ptr<char, decltype(&std::free)>;

struct identity_connection
{
	MYSQL *handle;
	bool main;
	std::uint64_t deadline_usec;
};

bool before_deadline(const identity_connection &connection)
{
	const auto now = std::chrono::duration_cast<std::chrono::microseconds>(
				 std::chrono::steady_clock::now().time_since_epoch())
				 .count();
	return !connection.deadline_usec ||
	       static_cast<std::uint64_t>(now) < connection.deadline_usec;
}

bool execute(const identity_connection &connection, const std::string &statement)
{
	return statement.size() <= 1024U && before_deadline(connection) &&
	       (connection.main || duris_sql_exclusion_guard_allows(connection.handle)) &&
	       before_deadline(connection) &&
	       sql_observed_execute_at(connection.handle, PERSISTENCE_QUERY_SITE,
				       connection.main ?
					       PERSISTENCE_QUERY_CONTEXT_MAIN :
					       PERSISTENCE_QUERY_CONTEXT_PLAYER_LOAD_WORKER,
				       statement.data(), statement.size(), nullptr);
}

struct transaction_owner
{
	const identity_connection &connection;
	bool committed = false;
	~transaction_owner()
	{
		if (!committed)
		{
			if (connection.main)
				(void)sql_rollback();
			else
				(void)mysql_rollback(connection.handle);
		}
	}
};

escaped_ptr escape(const identity_connection &connection, const char *text)
{
	if (connection.main)
		return escaped_ptr(sql_escape_string(text), std::free);
	const std::size_t bytes = std::strlen(text);
	escaped_ptr escaped(static_cast<char *>(std::malloc(bytes * 2U + 1U)), std::free);
	if (escaped &&
	    mysql_real_escape_string(connection.handle, escaped.get(), text, bytes) > bytes * 2U)
		escaped.reset();
	return escaped;
}

bool unsigned_value(const char *text, unsigned long bytes, std::uint64_t *value)
{
	if (!text || !bytes || bytes > 20U)
		return false;
	const auto parsed = std::from_chars(text, text + bytes, *value);
	return parsed.ec == std::errc{} && parsed.ptr == text + bytes;
}

bool read_key(const identity_connection &connection, const std::string &statement,
	      std::uint64_t *key)
{
	*key = 0U;
	if (!execute(connection, statement))
		return false;
	result_ptr result(mysql_store_result(connection.handle), mysql_free_result);
	if (!result || mysql_num_fields(result.get()) != 1U || mysql_num_rows(result.get()) > 1U)
		return false;
	if (!mysql_num_rows(result.get()))
		return true;
	const auto row = mysql_fetch_row(result.get());
	const auto lengths = mysql_fetch_lengths(result.get());
	return row && lengths && unsigned_value(row[0], lengths[0], key) && *key != 0U;
}

bool allocate_key(const identity_connection &connection, const std::string &prefix,
		  const std::string &suffix, std::uint64_t *key)
{
	for (unsigned attempt = 0U; attempt < ALLOCATION_ATTEMPTS; ++attempt)
	{
		std::uint64_t candidate = 0U;
		if (RAND_bytes(reinterpret_cast<unsigned char *>(&candidate), sizeof(candidate)) !=
		    1)
			return false;
		if (!candidate)
			continue;
		if (execute(connection, prefix + std::to_string(candidate) + suffix))
		{
			*key = candidate;
			return true;
		}
		if (mysql_errno(connection.handle) != DUPLICATE_KEY)
			return false;
	}
	return false;
}

bool prepare(const identity_connection &connection, const char *account_name,
	     std::uint64_t environment_id, std::uint64_t season_id, std::uint64_t *token)
{
	if (!token)
		return false;
	*token = 0U;
	try
	{
		if (!connection.handle || !account_name || !*account_name || !environment_id ||
		    !season_id ||
		    strnlen(account_name, ACCOUNT_NAME_BYTES + 1U) > ACCOUNT_NAME_BYTES ||
		    (connection.main ?
			     sql_in_transaction() :
			     !connection.deadline_usec ||
				     (connection.handle->server_status & SERVER_STATUS_IN_TRANS) ||
				     !(connection.handle->server_status & SERVER_STATUS_AUTOCOMMIT)))
			return false;
		escaped_ptr requested = escape(connection, account_name);
		if (!requested || !(connection.main ? sql_begin_transaction() :
						      execute(connection, "START TRANSACTION")))
			return false;
		transaction_owner transaction{ connection };
		std::string canonical_name;
		{
			/* Lock the authoritative parent first. This serializes aliases, creation,
		 * rename and deletion without publishing a name or guessing a lifetime. */
			if (!execute(
				    connection,
				    std::string(
					    "SELECT account_name,COALESCE(blocked,0) FROM accounts WHERE account_name='") +
					    requested.get() + "' LIMIT 1 FOR UPDATE"))
				return false;
			result_ptr result(mysql_store_result(connection.handle), mysql_free_result);
			if (!result || mysql_num_fields(result.get()) != 2U ||
			    mysql_num_rows(result.get()) != 1U)
				return false;
			const auto row = mysql_fetch_row(result.get());
			const auto lengths = mysql_fetch_lengths(result.get());
			std::uint64_t blocked = 0U;
			if (!row || !lengths || !row[0] || !lengths[0] ||
			    lengths[0] > ACCOUNT_NAME_BYTES ||
			    std::memchr(row[0], '\0', lengths[0]) ||
			    !unsigned_value(row[1], lengths[1], &blocked) ||
			    blocked == ACCOUNT_BLOCK_DELETION)
				return false;
			canonical_name.assign(row[0], lengths[0]);
		}
		escaped_ptr canonical = escape(connection, canonical_name.c_str());
		if (!canonical)
			return false;
		std::uint64_t lifetime = 0U;
		if (!read_key(
			    connection,
			    std::string(
				    "SELECT lifetime_id FROM telemetry_account_lifetime WHERE account_name='") +
				    canonical.get() + "' LIMIT 1",
			    &lifetime))
			return false;
		if (!lifetime &&
		    !allocate_key(
			    connection,
			    "INSERT INTO telemetry_account_lifetime(lifetime_id,account_name) VALUES (",
			    std::string(",'") + canonical.get() + "')", &lifetime))
			return false;
		const std::string scope =
			std::to_string(environment_id) + "," + std::to_string(season_id);
		std::uint64_t prepared = 0U;
		if (!read_key(
			    connection,
			    "SELECT account_token FROM telemetry_account_token WHERE environment_id=" +
				    std::to_string(environment_id) +
				    " AND season_id=" + std::to_string(season_id) +
				    " AND lifetime_id=" + std::to_string(lifetime) + " LIMIT 1",
			    &prepared))
			return false;
		if (!prepared &&
		    !allocate_key(
			    connection,
			    "INSERT INTO telemetry_account_token(account_token,environment_id,season_id,lifetime_id) VALUES (",
			    "," + scope + "," + std::to_string(lifetime) + ")", &prepared))
			return false;
		if (!(connection.main ? sql_commit() : execute(connection, "COMMIT")))
			return false;
		transaction.committed = true;
		*token = prepared;
		return true;
	}
	catch (const std::bad_alloc &)
	{
		/* Transaction ownership unwinds before this optional preparation fails. */
		return false;
	}
}

} // namespace
#endif

bool sql_prepare_telemetry_account_token(const char *account_name, std::uint64_t environment_id,
					 std::uint64_t season_id, std::uint64_t *token)
{
#ifdef __NO_MYSQL__
	(void)account_name;
	(void)environment_id;
	(void)season_id;
	if (token)
		*token = 0U;
	return false;
#else
	return prepare({ DB, true, 0U }, account_name, environment_id, season_id, token);
#endif
}

bool sql_prepare_telemetry_account_token_on(struct st_mysql *connection, const char *account_name,
					    std::uint64_t environment_id, std::uint64_t season_id,
					    std::uint64_t deadline_usec, std::uint64_t *token)
{
#ifdef __NO_MYSQL__
	(void)connection;
	(void)account_name;
	(void)environment_id;
	(void)season_id;
	(void)deadline_usec;
	if (token)
		*token = 0U;
	return false;
#else
	return prepare({ connection, false, deadline_usec }, account_name, environment_id,
		       season_id, token);
#endif
}
