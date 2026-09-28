#include "persistence/economic_sql_pending_claim_source.h"

#include <cerrno>

#ifndef __NO_MYSQL__
#include <array>
#include <charconv>
#include <climits>
#include <memory>
#include <new>
#include <string>
#include <vector>

namespace
{
std::string hex(std::span<const uint8_t> bytes)
{
	static constexpr char digits[] = "0123456789abcdef";
	std::string result = "X'";
	result.reserve(bytes.size() * 2 + 3);
	for (uint8_t byte : bytes)
	{
		result += digits[byte >> 4];
		result += digits[byte & 15];
	}
	result += '\'';
	return result;
}

bool execute(MYSQL *connection, const std::string &sql)
{
	if (!mysql_real_query(connection, sql.data(), sql.size()))
		return true;
	errno = mysql_errno(connection) ? static_cast<int>(mysql_errno(connection)) : EIO;
	return false;
}

bool row(MYSQL *connection, const std::string &sql, size_t count, std::vector<std::string> *values)
{
	if (!execute(connection, sql))
		return false;
	std::unique_ptr<MYSQL_RES, decltype(&mysql_free_result)> rows(
		mysql_store_result(connection), mysql_free_result);
	if (!rows || mysql_num_rows(rows.get()) != 1 || mysql_num_fields(rows.get()) != count)
	{
		errno = mysql_errno(connection) ? static_cast<int>(mysql_errno(connection)) :
						  EILSEQ;
		return false;
	}
	MYSQL_ROW cells = mysql_fetch_row(rows.get());
	const unsigned long *lengths = mysql_fetch_lengths(rows.get());
	if (!cells || !lengths)
	{
		errno = EILSEQ;
		return false;
	}
	values->clear();
	values->reserve(count);
	for (size_t index = 0; index < count; ++index)
	{
		if (!cells[index])
		{
			errno = EILSEQ;
			return false;
		}
		values->emplace_back(cells[index], lengths[index]);
	}
	return true;
}

bool u64(const std::string &text, uint64_t *number)
{
	const auto parsed = std::from_chars(text.data(), text.data() + text.size(), *number);
	return !text.empty() && parsed.ec == std::errc{} && parsed.ptr == text.data() + text.size();
}

bool i64(const std::string &text, int64_t *number)
{
	const auto parsed = std::from_chars(text.data(), text.data() + text.size(), *number);
	return !text.empty() && parsed.ec == std::errc{} && parsed.ptr == text.data() + text.size();
}
} // namespace
#endif

unsigned int economic_sql_pending_claim_source_stage(MYSQL *connection,
						     const critical_operation_id &source_operation,
						     uint16_t source_slot,
						     const economic_account_key &claim_account,
						     uint32_t beneficiary_pid, uint64_t amount)
{
#ifdef __NO_MYSQL__
	(void)connection;
	(void)source_operation;
	(void)source_slot;
	(void)claim_account;
	(void)beneficiary_pid;
	(void)amount;
	return ENOTSUP;
#else
	if (!connection || !(connection->server_status & SERVER_STATUS_IN_TRANS) ||
	    critical_operation_id_is_zero(source_operation) || !source_slot || !beneficiary_pid ||
	    !amount || amount > UINT_MAX || !economic_account_key_valid(claim_account) ||
	    claim_account.kind != economic_account_kind::pending_claim || claim_account.context_id)
		return EINVAL;
	try
	{
		std::array<uint8_t, ECONOMIC_ACCOUNT_KEY_BYTES> encoded = {};
		if (economic_account_key_encode(claim_account, &encoded) !=
		    economic_accounting_error::ok)
			return EILSEQ;
		std::vector<std::string> values;
		const auto lineage = hex(claim_account.lineage.bytes);
		if (!row(connection,
			 "SELECT native_id FROM economic_account_mapping WHERE mapping_id=" +
				 std::to_string(claim_account.authority_id) +
				 " AND lineage=" + lineage +
				 " AND account_kind=5 AND context_id=0 AND "
				 "backend_kind=1 AND locator_kind=5 AND active_native_id=" +
				 std::to_string(beneficiary_pid) +
				 " AND retiring_operation_id IS NULL FOR UPDATE",
			 1, &values))
			return errno ? static_cast<unsigned int>(errno) : EILSEQ;
		uint64_t native = 0;
		if (!u64(values[0], &native) || native != beneficiary_pid)
			return EILSEQ;
		if (!row(connection,
			 "SELECT o.outcome,e.after_copper-e.before_copper,"
			 "e.after_silver-e.before_silver,e.after_gold-e.before_gold,"
			 "e.after_platinum-e.before_platinum "
			 "FROM economic_accounting_operation o "
			 "JOIN economic_accounting_account_effect e ON e.operation_id=o.operation_id "
			 "WHERE o.operation_id=" +
				 hex(source_operation.bytes) + " AND o.lineage=" + lineage +
				 " AND e.account_key=" + hex(encoded) + " FOR UPDATE",
			 5, &values))
			return errno ? static_cast<unsigned int>(errno) : EILSEQ;
		uint64_t outcome = 0;
		int64_t delta[4] = {};
		if (!u64(values[0], &outcome) || outcome != 1)
			return EILSEQ;
		for (size_t index = 0; index < 4; ++index)
			if (!i64(values[index + 1], &delta[index]))
				return EILSEQ;
		if (delta[0] != static_cast<int64_t>(amount) || delta[1] || delta[2] || delta[3])
			return EILSEQ;
		if (!execute(connection, "INSERT INTO economic_pending_claim_source("
					 "source_operation_id,source_slot,lineage,claim_mapping_id,"
					 "beneficiary_pid,amount) VALUES(" +
						 hex(source_operation.bytes) + "," +
						 std::to_string(source_slot) + "," + lineage + "," +
						 std::to_string(claim_account.authority_id) + "," +
						 std::to_string(beneficiary_pid) + "," +
						 std::to_string(amount) + ")"))
			return errno ? static_cast<unsigned int>(errno) : EIO;
		return mysql_affected_rows(connection) == 1 ? 0 : EILSEQ;
	}
	catch (const std::bad_alloc &)
	{
		return ENOMEM;
	}
#endif
}
