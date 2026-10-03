#include "sql/sql_telemetry_account_identity.h"

#ifndef __NO_MYSQL__
#include "sql/sql.h"
#include "sql/sql_transaction.h"

#include <openssl/rand.h>

#include <charconv>
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

struct transaction_owner
{
	bool committed = false;
	~transaction_owner()
	{
		if (!committed)
			(void)sql_rollback();
	}
};

using result_ptr = std::unique_ptr<MYSQL_RES, decltype(&mysql_free_result)>;
using escaped_ptr = std::unique_ptr<char, decltype(&std::free)>;

bool execute(const std::string &statement)
{
	return statement.size() <= 1024U &&
	       sql_observed_execute_at(DB, PERSISTENCE_QUERY_SITE, PERSISTENCE_QUERY_CONTEXT_MAIN,
				       statement.data(), statement.size(), nullptr);
}

bool unsigned_value(const char *text, unsigned long bytes, std::uint64_t *value)
{
	if (!text || !bytes || bytes > 20U)
		return false;
	const auto parsed = std::from_chars(text, text + bytes, *value);
	return parsed.ec == std::errc{} && parsed.ptr == text + bytes;
}

bool read_key(const std::string &statement, std::uint64_t *key)
{
	*key = 0U;
	if (!execute(statement))
		return false;
	result_ptr result(mysql_store_result(DB), mysql_free_result);
	if (!result || mysql_num_fields(result.get()) != 1U || mysql_num_rows(result.get()) > 1U)
		return false;
	if (!mysql_num_rows(result.get()))
		return true;
	const auto row = mysql_fetch_row(result.get());
	const auto lengths = mysql_fetch_lengths(result.get());
	return row && lengths && unsigned_value(row[0], lengths[0], key) && *key != 0U;
}

bool allocate_key(const std::string &prefix, const std::string &suffix, std::uint64_t *key)
{
	for (unsigned attempt = 0U; attempt < ALLOCATION_ATTEMPTS; ++attempt)
	{
		std::uint64_t candidate = 0U;
		if (RAND_bytes(reinterpret_cast<unsigned char *>(&candidate), sizeof(candidate)) !=
		    1)
			return false;
		if (!candidate)
			continue;
		if (execute(prefix + std::to_string(candidate) + suffix))
		{
			*key = candidate;
			return true;
		}
		if (mysql_errno(DB) != DUPLICATE_KEY)
			return false;
	}
	return false;
}

} // namespace
#endif

bool sql_prepare_telemetry_account_token(const char *account_name, std::uint64_t environment_id,
					 std::uint64_t season_id, std::uint64_t *token)
{
	if (!token)
		return false;
	*token = 0U;
#ifdef __NO_MYSQL__
	(void)account_name;
	(void)environment_id;
	(void)season_id;
	return false;
#else
	try
	{
		if (!DB || !account_name || !*account_name || !environment_id || !season_id ||
		    strnlen(account_name, ACCOUNT_NAME_BYTES + 1U) > ACCOUNT_NAME_BYTES ||
		    sql_in_transaction())
			return false;
		escaped_ptr requested(sql_escape_string(account_name), std::free);
		if (!requested || !sql_begin_transaction())
			return false;
		transaction_owner transaction;
		std::string canonical_name;
		{
			/* Lock the authoritative parent first. This serializes aliases, creation,
		 * rename and deletion without publishing a name or guessing a lifetime. */
			if (!execute(
				    std::string(
					    "SELECT account_name,COALESCE(blocked,0) FROM accounts WHERE account_name='") +
				    requested.get() + "' LIMIT 1 FOR UPDATE"))
				return false;
			result_ptr result(mysql_store_result(DB), mysql_free_result);
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
		escaped_ptr canonical(sql_escape_string(canonical_name.c_str()), std::free);
		if (!canonical)
			return false;
		std::uint64_t lifetime = 0U;
		if (!read_key(
			    std::string(
				    "SELECT lifetime_id FROM telemetry_account_lifetime WHERE account_name='") +
				    canonical.get() + "' LIMIT 1",
			    &lifetime))
			return false;
		if (!lifetime &&
		    !allocate_key(
			    "INSERT INTO telemetry_account_lifetime(lifetime_id,account_name) VALUES (",
			    std::string(",'") + canonical.get() + "')", &lifetime))
			return false;
		const std::string scope =
			std::to_string(environment_id) + "," + std::to_string(season_id);
		std::uint64_t prepared = 0U;
		if (!read_key(
			    "SELECT account_token FROM telemetry_account_token WHERE environment_id=" +
				    std::to_string(environment_id) +
				    " AND season_id=" + std::to_string(season_id) +
				    " AND lifetime_id=" + std::to_string(lifetime) + " LIMIT 1",
			    &prepared))
			return false;
		if (!prepared &&
		    !allocate_key(
			    "INSERT INTO telemetry_account_token(account_token,environment_id,season_id,lifetime_id) VALUES (",
			    "," + scope + "," + std::to_string(lifetime) + ")", &prepared))
			return false;
		if (!sql_commit())
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
#endif
}
