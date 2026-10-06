#include "persistence/economic_sql_pending_claim_source.h"

#include "economy/economic_baseline_adapter.h"

#include <cerrno>

#ifndef __NO_MYSQL__
#include <array>
#include <charconv>
#include <climits>
#include <memory>
#include <openssl/sha.h>
#include <new>
#include <string>
#include <string_view>
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

// Exact original ESD1/ESR1 auction_money_pickups row framing. Compare the
// retained original PID, amount and revision, never today's pickup balance.
bool original_claim_digest(const economic_baseline_holding &holding, uint32_t pid)
{
	std::vector<uint8_t> definition;
	const auto number = [](std::vector<uint8_t> &bytes, uint64_t value)
	{
		for (size_t byte = 0; byte < 8; ++byte)
			bytes.push_back(static_cast<uint8_t>(value >> (8 * byte)));
	};
	const auto text = [&](std::vector<uint8_t> &bytes, std::string_view value)
	{
		number(bytes, value.size());
		bytes.insert(bytes.end(), value.begin(), value.end());
	};
	text(definition, "ESD1");
	text(definition, "auction_money_pickups");
	text(definition, "pid");
	number(definition, 3);
	for (const auto column : { "pid", "money", "claim_revision" })
		text(definition, column);
	economic_digest schema{};
	SHA256(definition.data(), definition.size(), schema.data());
	std::vector<uint8_t> row;
	text(row, "ESR1");
	row.insert(row.end(), schema.begin(), schema.end());
	for (const auto &cell : { std::to_string(pid), std::to_string(holding.balance[0]),
				  std::to_string(holding.native_revision) })
	{
		number(row, 1);
		text(row, cell);
	}
	economic_digest actual{};
	SHA256(row.data(), row.size(), actual.data());
	return actual == holding.source_digest;
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

unsigned int economic_sql_pending_claim_source_contract(MYSQL *connection)
{
#ifdef __NO_MYSQL__
	(void)connection;
	return ENOTSUP;
#else
	if (!connection)
		return EINVAL;
	try
	{
		for (const auto &sql :
		     { "SELECT claim_origin_version FROM economic_baseline_witness LIMIT 0",
		       "SELECT spending_operation_id,source_operation_id,source_slot,amount "
		       "FROM economic_pending_claim_consumption LIMIT 0" })
		{
			if (!execute(connection, sql))
				return errno ? static_cast<unsigned int>(errno) : EIO;
			std::unique_ptr<MYSQL_RES, decltype(&mysql_free_result)> rows(
				mysql_store_result(connection), mysql_free_result);
			if (!rows || mysql_num_rows(rows.get()) != 0)
				return EILSEQ;
		}
		return 0;
	}
	catch (const std::bad_alloc &)
	{
		return ENOMEM;
	}
#endif
}

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
	const auto contract_error = economic_sql_pending_claim_source_contract(connection);
	if (contract_error)
		return contract_error;
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

unsigned int economic_sql_pending_claim_source_remaining(
	MYSQL *connection, const economic_account_key &claim_account, uint32_t beneficiary_pid,
	std::vector<economic_sql_pending_claim_remaining> *output)
{
#ifdef __NO_MYSQL__
	(void)connection;
	(void)claim_account;
	(void)beneficiary_pid;
	(void)output;
	return ENOTSUP;
#else
	if (!connection || !(connection->server_status & SERVER_STATUS_IN_TRANS) || !output ||
	    !beneficiary_pid || !economic_account_key_valid(claim_account) ||
	    claim_account.kind != economic_account_kind::pending_claim || claim_account.context_id)
		return EINVAL;
	const auto contract_error = economic_sql_pending_claim_source_contract(connection);
	if (contract_error)
		return contract_error;
	try
	{
		std::array<uint8_t, ECONOMIC_ACCOUNT_KEY_BYTES> encoded = {};
		if (economic_account_key_encode(claim_account, &encoded) !=
		    economic_accounting_error::ok)
			return EILSEQ;
		const auto lineage = hex(claim_account.lineage.bytes);
		const auto key = hex(encoded);
		const auto scope = "s.lineage=" + lineage + " AND s.claim_mapping_id=" +
				   std::to_string(claim_account.authority_id) +
				   " AND s.beneficiary_pid=" + std::to_string(beneficiary_pid);
		std::vector<std::string> values;
		if (!row(connection,
			 "SELECT native_id FROM economic_account_mapping WHERE mapping_id=" +
				 std::to_string(claim_account.authority_id) +
				 " AND lineage=" + lineage +
				 " AND account_kind=5 AND context_id=0 AND backend_kind=1 AND locator_kind=5 "
				 "AND active_native_id=" +
				 std::to_string(beneficiary_pid) +
				 " AND retiring_operation_id IS NULL FOR UPDATE",
			 1, &values))
			return errno ? static_cast<unsigned int>(errno) : EILSEQ;
		uint64_t native = 0;
		if (!u64(values[0], &native) || native != beneficiary_pid)
			return EILSEQ;
		// History is retained indefinitely; the existing4096 bound applies to
		// OPEN allocations, not to every fully consumed origin ever recorded.
		// Bounded aggregate result rows authenticate the complete stored scope
		// before selecting its remaining origin set.
		const auto amount_used =
			"(SELECT COALESCE(SUM(c.amount),0) FROM economic_pending_claim_consumption c "
			"WHERE c.source_operation_id=s.source_operation_id AND c.source_slot=s.source_slot)";
		const auto legacy_total =
			"(SELECT COALESCE(SUM(s2.amount),0) FROM economic_pending_claim_source s2 WHERE "
			"s2.lineage=s.lineage AND s2.claim_mapping_id=s.claim_mapping_id "
			"AND s2.beneficiary_pid=s.beneficiary_pid AND s2.claim_operation_id=s.claim_operation_id)";
		if (!row(connection,
			 "SELECT COUNT(*) FROM economic_pending_claim_source s "
			 "LEFT JOIN economic_accounting_operation o ON o.operation_id=s.source_operation_id "
			 "LEFT JOIN economic_accounting_account_effect e ON e.operation_id=o.operation_id AND e.account_key=" +
				 key +
				 " LEFT JOIN economic_accounting_operation l ON l.operation_id=s.claim_operation_id "
				 "LEFT JOIN economic_accounting_account_effect le ON le.operation_id=l.operation_id AND le.account_key=" +
				 key + " WHERE " + scope +
				 " AND (s.amount=0 OR s.amount>4294967295 OR s.source_slot=0 "
				 "OR o.operation_id IS NULL OR o.outcome<>1 OR o.lineage<>" +
				 lineage +
				 " OR e.operation_id IS NULL OR e.after_copper-e.before_copper<>s.amount "
				 "OR e.before_silver<>e.after_silver OR e.before_gold<>e.after_gold OR e.before_platinum<>e.after_platinum "
				 "OR " +
				 amount_used +
				 ">s.amount OR (s.claim_operation_id IS NOT NULL AND (" +
				 amount_used +
				 ">0 OR l.operation_id IS NULL OR l.outcome<>1 OR l.lineage<>" +
				 lineage +
				 " OR le.operation_id IS NULL OR le.before_copper-le.after_copper<>" +
				 legacy_total +
				 " OR le.before_silver<>le.after_silver OR le.before_gold<>le.after_gold "
				 "OR le.before_platinum<>le.after_platinum OR EXISTS (SELECT 1 FROM economic_pending_claim_consumption c "
				 "WHERE c.spending_operation_id=s.claim_operation_id))))",
			 1, &values))
			return errno ? static_cast<unsigned int>(errno) : EILSEQ;
		uint64_t invalid_history = 0;
		if (!u64(values[0], &invalid_history) || invalid_history)
			return EILSEQ;
		const auto current_debit_total =
			"(SELECT COALESCE(SUM(c2.amount),0) FROM economic_pending_claim_consumption c2 "
			"JOIN economic_pending_claim_source s2 ON s2.source_operation_id=c2.source_operation_id "
			"AND s2.source_slot=c2.source_slot WHERE c2.spending_operation_id=c.spending_operation_id "
			"AND s2.lineage=" +
			lineage +
			" AND s2.claim_mapping_id=" + std::to_string(claim_account.authority_id) +
			" AND s2.beneficiary_pid=" + std::to_string(beneficiary_pid) + ")";
		if (!row(connection,
			 "SELECT COUNT(*) FROM economic_pending_claim_consumption c "
			 "JOIN economic_pending_claim_source s ON s.source_operation_id=c.source_operation_id AND s.source_slot=c.source_slot "
			 "LEFT JOIN economic_accounting_operation o ON o.operation_id=c.spending_operation_id "
			 "LEFT JOIN economic_accounting_account_effect e ON e.operation_id=o.operation_id AND e.account_key=" +
				 key + " WHERE " + scope +
				 " AND (o.operation_id IS NULL OR o.outcome<>1 OR o.lineage<>" +
				 lineage +
				 " OR e.operation_id IS NULL OR c.amount=0 OR c.amount>4294967295 "
				 "OR c.spending_operation_id=c.source_operation_id OR e.before_copper-e.after_copper<>" +
				 current_debit_total +
				 " OR e.before_silver<>e.after_silver OR e.before_gold<>e.after_gold "
				 "OR e.before_platinum<>e.after_platinum OR EXISTS (SELECT 1 FROM economic_pending_claim_source s3 "
				 "WHERE s3.lineage=s.lineage AND s3.claim_mapping_id=s.claim_mapping_id AND s3.beneficiary_pid=s.beneficiary_pid "
				 "AND s3.claim_operation_id=c.spending_operation_id))",
			 1, &values))
			return errno ? static_cast<unsigned int>(errno) : EILSEQ;
		if (!u64(values[0], &invalid_history) || invalid_history)
			return EILSEQ;
		// Lock original allocations before examining links. Every writer takes
		// the same mapping lock; no unlocked balance read authorizes spending.
		if (!execute(
			    connection,
			    "SELECT HEX(s.source_operation_id),s.source_slot,s.amount,"
			    "HEX(s.claim_operation_id) FROM economic_pending_claim_source s WHERE " +
				    scope + " AND s.claim_operation_id IS NULL AND " + amount_used +
				    "<s.amount ORDER BY s.source_operation_id,s.source_slot FOR UPDATE"))
			return errno ? static_cast<unsigned int>(errno) : EIO;
		std::unique_ptr<MYSQL_RES, decltype(&mysql_free_result)> rows(
			mysql_store_result(connection), mysql_free_result);
		if (!rows || mysql_num_fields(rows.get()) != 4 || mysql_num_rows(rows.get()) > 4096)
			return EILSEQ;
		struct origin
		{
			economic_sql_pending_claim_remaining value;
			std::string legacy;
		};
		std::vector<origin> origins;
		MYSQL_ROW cells = nullptr;
		while ((cells = mysql_fetch_row(rows.get())))
		{
			const unsigned long *lengths = mysql_fetch_lengths(rows.get());
			if (!lengths || !cells[0] || !cells[1] || !cells[2])
				return EILSEQ;
			origin source;
			uint64_t slot = 0;
			const std::string operation(cells[0], lengths[0]);
			if (operation.size() != 32 ||
			    !critical_operation_id_from_hex(operation.c_str(),
							    &source.value.operation) ||
			    critical_operation_id_is_zero(source.value.operation) ||
			    !u64(std::string(cells[1], lengths[1]), &slot) || !slot ||
			    slot > UINT16_MAX ||
			    !u64(std::string(cells[2], lengths[2]),
				 &source.value.original_amount) ||
			    !source.value.original_amount ||
			    source.value.original_amount > UINT_MAX)
				return EILSEQ;
			source.value.slot = static_cast<uint16_t>(slot);
			if (cells[3])
				source.legacy.assign(cells[3], lengths[3]);
			origins.push_back(std::move(source));
		}
		rows.reset();
		std::vector<economic_sql_pending_claim_remaining> result;
		for (auto &source : origins)
		{
			const auto origin_where =
				"c.source_operation_id=" + hex(source.value.operation.bytes) +
				" AND c.source_slot=" + std::to_string(source.value.slot);
			if (!row(connection,
				 "SELECT o.outcome,e.after_copper-e.before_copper,"
				 "e.after_silver-e.before_silver,e.after_gold-e.before_gold,"
				 "e.after_platinum-e.before_platinum FROM economic_accounting_operation o "
				 "JOIN economic_accounting_account_effect e ON e.operation_id=o.operation_id "
				 "WHERE o.operation_id=" +
					 hex(source.value.operation.bytes) + " AND o.lineage=" +
					 lineage + " AND e.account_key=" + key + " FOR UPDATE",
				 5, &values))
				return errno ? static_cast<unsigned int>(errno) : EILSEQ;
			uint64_t outcome = 0;
			int64_t delta[4] = {};
			if (!u64(values[0], &outcome) || outcome != 1)
				return EILSEQ;
			for (size_t i = 0; i < 4; ++i)
				if (!i64(values[i + 1], &delta[i]))
					return EILSEQ;
			if (delta[0] != static_cast<int64_t>(source.value.original_amount) ||
			    delta[1] || delta[2] || delta[3])
				return EILSEQ;
			if (!row(connection,
				 "SELECT COALESCE(SUM(c.amount),0) FROM economic_pending_claim_consumption c WHERE " +
					 origin_where,
				 1, &values))
				return errno ? static_cast<unsigned int>(errno) : EILSEQ;
			uint64_t consumed = 0;
			if (!u64(values[0], &consumed) || consumed > source.value.original_amount ||
			    (!source.legacy.empty() && consumed))
				return EILSEQ;
			// Each retained consumer must have its exact successful claim debit.
			// Its total across this claim's original sources equals that effect,
			// rather than attributing an unrelated source's amount to this root.
			const auto debit_total =
				"(SELECT COALESCE(SUM(c2.amount),0) FROM economic_pending_claim_consumption c2 "
				"JOIN economic_pending_claim_source s2 ON s2.source_operation_id=c2.source_operation_id "
				"AND s2.source_slot=c2.source_slot WHERE c2.spending_operation_id=c.spending_operation_id "
				"AND s2.lineage=" +
				lineage + " AND s2.claim_mapping_id=" +
				std::to_string(claim_account.authority_id) +
				" AND s2.beneficiary_pid=" + std::to_string(beneficiary_pid) + ")";
			if (!row(connection,
				 "SELECT COUNT(*) FROM economic_pending_claim_consumption c "
				 "LEFT JOIN economic_accounting_operation o ON o.operation_id=c.spending_operation_id "
				 "LEFT JOIN economic_accounting_account_effect e ON e.operation_id=o.operation_id "
				 "AND e.account_key=" +
					 key + " WHERE " + origin_where +
					 " AND (o.operation_id IS NULL OR o.lineage<>" + lineage +
					 " OR o.outcome<>1 OR e.operation_id IS NULL OR c.amount=0 "
					 "OR e.before_copper-e.after_copper<>" +
					 debit_total +
					 " OR e.before_silver<>e.after_silver OR e.before_gold<>e.after_gold "
					 "OR e.before_platinum<>e.after_platinum OR c.spending_operation_id=c.source_operation_id "
					 "OR EXISTS (SELECT 1 FROM economic_pending_claim_source s3 WHERE s3.lineage=" +
					 lineage + " AND s3.claim_mapping_id=" +
					 std::to_string(claim_account.authority_id) +
					 " AND s3.beneficiary_pid=" +
					 std::to_string(beneficiary_pid) +
					 " AND s3.claim_operation_id=c.spending_operation_id))",
				 1, &values))
				return errno ? static_cast<unsigned int>(errno) : EILSEQ;
			uint64_t invalid = 0;
			if (!u64(values[0], &invalid) || invalid)
				return EILSEQ;
			if (!source.legacy.empty())
			{
				critical_operation_id legacy = {};
				if (source.legacy.size() != 32 ||
				    !critical_operation_id_from_hex(source.legacy.c_str(),
								    &legacy) ||
				    critical_operation_id_is_zero(legacy))
					return EILSEQ;
				if (!row(connection,
					 "SELECT o.outcome,e.before_copper-e.after_copper,"
					 "e.before_silver-e.after_silver,e.before_gold-e.after_gold,"
					 "e.before_platinum-e.after_platinum,"
					 "(SELECT SUM(s.amount) FROM economic_pending_claim_source s WHERE " +
						 scope +
						 " AND s.claim_operation_id=" + hex(legacy.bytes) +
						 ") FROM economic_accounting_operation o JOIN economic_accounting_account_effect e "
						 "ON e.operation_id=o.operation_id WHERE o.operation_id=" +
						 hex(legacy.bytes) + " AND o.lineage=" + lineage +
						 " AND e.account_key=" + key + " FOR UPDATE",
					 6, &values))
					return errno ? static_cast<unsigned int>(errno) : EILSEQ;
				uint64_t debit = 0, total = 0;
				if (!u64(values[0], &outcome) || outcome != 1 ||
				    !u64(values[1], &debit) || !debit || !u64(values[5], &total) ||
				    debit != total)
					return EILSEQ;
				for (size_t i = 2; i < 5; ++i)
					if (!i64(values[i], &delta[i - 1]) || delta[i - 1])
						return EILSEQ;
				if (!row(connection,
					 "SELECT COUNT(*) FROM economic_pending_claim_consumption WHERE spending_operation_id=" +
						 hex(legacy.bytes),
					 1, &values) ||
				    !u64(values[0], &invalid) || invalid)
					return EILSEQ;
				consumed = source.value.original_amount;
			}
			source.value.remaining_amount = source.value.original_amount - consumed;
			if (source.value.remaining_amount)
				result.push_back(source.value);
		}
		*output = std::move(result);
		return 0;
	}
	catch (const std::bad_alloc &)
	{
		return ENOMEM;
	}
#endif
}

unsigned int
economic_sql_pending_claim_source_verify_baseline(MYSQL *connection,
						  const critical_operation_id &baseline_operation,
						  const economic_baseline_batch &baseline)
{
#ifdef __NO_MYSQL__
	(void)connection;
	(void)baseline_operation;
	(void)baseline;
	return ENOTSUP;
#else
	if (!connection || !(connection->server_status & SERVER_STATUS_IN_TRANS) ||
	    critical_operation_id_is_zero(baseline_operation) ||
	    critical_operation_id_is_zero(baseline.lineage) ||
	    baseline.holdings.size() > ECONOMIC_BASELINE_MAX_HOLDINGS)
		return EINVAL;
	const auto contract_error = economic_sql_pending_claim_source_contract(connection);
	if (contract_error)
		return contract_error;
	try
	{
		const auto root = "source_operation_id=" + hex(baseline_operation.bytes);
		uint64_t expected_count = 0;
		std::vector<std::string> values;
		for (size_t index = 0; index < baseline.holdings.size(); ++index)
		{
			const auto &holding = baseline.holdings[index];
			if (holding.account.kind != economic_account_kind::pending_claim)
				continue;
			if (!economic_account_key_valid(holding.account) ||
			    holding.account.lineage.bytes != baseline.lineage.bytes ||
			    holding.account.context_id || holding.balance[0] < 0 ||
			    static_cast<uint64_t>(holding.balance[0]) > UINT_MAX ||
			    holding.balance[1] || holding.balance[2] || holding.balance[3] ||
			    index >= UINT16_MAX)
				return EILSEQ;
			if (!row(connection,
				 "SELECT native_id FROM economic_account_mapping WHERE mapping_id=" +
					 std::to_string(holding.account.authority_id) +
					 " AND lineage=" + hex(baseline.lineage.bytes) +
					 " AND account_kind=5 AND context_id=0 AND backend_kind=1 AND locator_kind=5 "
					 "LOCK IN SHARE MODE",
				 1, &values))
				return errno ? static_cast<unsigned int>(errno) : EILSEQ;
			uint64_t native = 0;
			if (!u64(values[0], &native) || !native || native > UINT32_MAX)
				return EILSEQ;
			if (!original_claim_digest(holding, static_cast<uint32_t>(native)))
				return EILSEQ;
			if (holding.balance[0])
			{
				if (!row(connection,
					 "SELECT COUNT(*) FROM economic_pending_claim_source WHERE " +
						 root +
						 " AND source_slot=" + std::to_string(index + 1) +
						 " AND lineage=" + hex(baseline.lineage.bytes) +
						 " AND claim_mapping_id=" +
						 std::to_string(holding.account.authority_id) +
						 " AND beneficiary_pid=" + std::to_string(native) +
						 " AND amount=" + std::to_string(holding.balance[0]),
					 1, &values))
					return errno ? static_cast<unsigned int>(errno) : EILSEQ;
				uint64_t count = 0;
				if (!u64(values[0], &count) || count != 1)
					return EILSEQ;
				++expected_count;
			}
			// Authenticate both whole/partial retained representations even
			// when the current native amount is zero. Missing original source
			// evidence is refused above before an empty remainder can pass.
			std::vector<economic_sql_pending_claim_remaining> remaining;
			const auto error = economic_sql_pending_claim_source_remaining(
				connection, holding.account, static_cast<uint32_t>(native),
				&remaining);
			if (error)
				return error;
		}
		if (!row(connection,
			 "SELECT COUNT(*) FROM economic_pending_claim_source WHERE " + root, 1,
			 &values))
			return errno ? static_cast<unsigned int>(errno) : EILSEQ;
		uint64_t count = 0;
		return u64(values[0], &count) && count == expected_count ? 0 : EILSEQ;
	}
	catch (const std::bad_alloc &)
	{
		return ENOMEM;
	}
#endif
}

unsigned int economic_sql_pending_claim_source_consume(
	MYSQL *connection, const critical_operation_id &spending_operation,
	const economic_account_key &claim_account, uint32_t beneficiary_pid, uint64_t claim_balance,
	uint64_t amount)
{
#ifdef __NO_MYSQL__
	(void)connection;
	(void)spending_operation;
	(void)claim_account;
	(void)beneficiary_pid;
	(void)claim_balance;
	(void)amount;
	return ENOTSUP;
#else
	if (!connection || !(connection->server_status & SERVER_STATUS_IN_TRANS) ||
	    critical_operation_id_is_zero(spending_operation) || !beneficiary_pid || !amount ||
	    claim_balance > UINT_MAX || amount > claim_balance ||
	    !economic_account_key_valid(claim_account) ||
	    claim_account.kind != economic_account_kind::pending_claim || claim_account.context_id)
		return EINVAL;
	try
	{
		std::vector<economic_sql_pending_claim_remaining> sources;
		const auto error = economic_sql_pending_claim_source_remaining(
			connection, claim_account, beneficiary_pid, &sources);
		if (error)
			return error;
		uint64_t source_balance = 0;
		for (const auto &source : sources)
		{
			if (source.remaining_amount > UINT_MAX - source_balance)
				return EILSEQ;
			source_balance += source.remaining_amount;
		}
		if (source_balance != claim_balance)
			return EILSEQ;
		std::array<uint8_t, ECONOMIC_ACCOUNT_KEY_BYTES> encoded = {};
		if (economic_account_key_encode(claim_account, &encoded) !=
		    economic_accounting_error::ok)
			return EILSEQ;
		std::vector<std::string> values;
		if (!row(connection,
			 "SELECT o.outcome,e.before_copper-e.after_copper,"
			 "e.before_silver-e.after_silver,e.before_gold-e.after_gold,"
			 "e.before_platinum-e.after_platinum FROM economic_accounting_operation o "
			 "JOIN economic_accounting_account_effect e ON e.operation_id=o.operation_id "
			 "WHERE o.operation_id=" +
				 hex(spending_operation.bytes) +
				 " AND o.lineage=" + hex(claim_account.lineage.bytes) +
				 " AND e.account_key=" + hex(encoded) + " FOR UPDATE",
			 5, &values))
			return errno ? static_cast<unsigned int>(errno) : EILSEQ;
		uint64_t outcome = 0, debit = 0;
		int64_t delta = 0;
		if (!u64(values[0], &outcome) || outcome != 1 || !u64(values[1], &debit) ||
		    debit != amount)
			return EILSEQ;
		for (size_t i = 2; i < 5; ++i)
			if (!i64(values[i], &delta) || delta)
				return EILSEQ;
		// This owner is append-only. Exact root replay is handled by the native
		// command owner before mutation; a repeated consumer here must refuse.
		if (!row(connection,
			 "SELECT COUNT(*) FROM economic_pending_claim_consumption WHERE spending_operation_id=" +
				 hex(spending_operation.bytes),
			 1, &values))
			return errno ? static_cast<unsigned int>(errno) : EILSEQ;
		uint64_t existing = 0;
		if (!u64(values[0], &existing) || existing)
			return EILSEQ;
		uint64_t remaining = amount;
		for (const auto &source : sources)
		{
			if (!remaining)
				break;
			if (source.operation.bytes == spending_operation.bytes)
				return EILSEQ;
			const uint64_t used = source.remaining_amount < remaining ?
						      source.remaining_amount :
						      remaining;
			if (!execute(connection,
				     "INSERT INTO economic_pending_claim_consumption(spending_operation_id,"
				     "source_operation_id,source_slot,amount) VALUES(" +
					     hex(spending_operation.bytes) + "," +
					     hex(source.operation.bytes) + "," +
					     std::to_string(source.slot) + "," +
					     std::to_string(used) + ")"))
				return errno ? static_cast<unsigned int>(errno) : EIO;
			if (mysql_affected_rows(connection) != 1)
				return EILSEQ;
			remaining -= used;
		}
		return remaining ? EILSEQ : 0;
	}
	catch (const std::bad_alloc &)
	{
		return ENOMEM;
	}
#endif
}
