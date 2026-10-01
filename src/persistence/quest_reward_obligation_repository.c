#include "persistence/quest_reward_obligation_repository.h"
#include "economy/currency_command.h"

#include <array>
#include <cerrno>
#include <charconv>
#include <cstring>
#include <memory>
#include <new>
#include <string>
#include <utility>
#include <map>

#ifndef __NO_MYSQL__
namespace
{
bool receipt_number(const char *text, uint64_t *value)
{
	if (!text || !value)
		return false;
	const auto end = text + strlen(text);
	const auto converted = std::from_chars(text, end, *value);
	return converted.ec == std::errc{} && converted.ptr == end;
}

// These are immutable native event witnesses, not current inventory/balance
// guesses. The caller's player-load transaction also supplies the live projection.
quest_reward_obligation_result
read_economic_receipts(MYSQL *connection, std::vector<quest_reward_obligation_record> *records,
		       unsigned int *database_error, quest_reward_read_metrics *metrics)
try
{
	struct expected_receipt
	{
		size_t record_index;
		size_t reward_index;
		bool seen = false;
	};
	std::map<uint64_t, expected_receipt> items;
	std::map<std::string, expected_receipt> cash;
	std::string item_sources, cash_operations;
	for (size_t record_index = 0; record_index < records->size(); ++record_index)
	{
		const auto &record = (*records)[record_index];
		for (size_t index = 0; index < record.terms.reward_count; ++index)
		{
			const auto &reward = record.terms.rewards[index];
			if (reward.type == 1U)
			{
				const auto source =
					quest_item_reward_source_id(record.terms, index);
				if (!source ||
				    !items.emplace(source, expected_receipt{ record_index, index })
					     .second)
					return quest_reward_obligation_result::corrupt;
				if (!item_sources.empty())
					item_sources += ',';
				item_sources += std::to_string(source);
			}
			else if (reward.type == 3U)
			{
				critical_operation_id child = {};
				char hex[33] = {};
				if (!critical_operation_id_derive(
					    record.offering_operation,
					    QUEST_REWARD_CURRENCY_OPERATION_DOMAIN,
					    static_cast<uint32_t>(index + 1), &child) ||
				    !critical_operation_id_to_hex(child, hex, sizeof(hex)) ||
				    !cash.emplace(hex, expected_receipt{ record_index, index })
					     .second)
					return quest_reward_obligation_result::corrupt;
				if (!cash_operations.empty())
					cash_operations += ',';
				cash_operations += "UNHEX('" + std::string(hex) + "')";
			}
		}
	}
	// A single bounded native SELECT per witness kind replaces up to 4,096
	// per-slot queries. Empty sets still execute so the read has a fixed cost.
	for (bool item_query : { true, false })
	{
		const size_t expected_count = item_query ? items.size() : cash.size();
		const std::string query =
			item_query ?
				"SELECT l.reason_id,i.result_payload,(i.status=1 AND i.result_code=0 AND i.command_type=5 "
				"AND l.from_owner_type=7 AND l.from_owner_id=0 AND l.from_owner_context_id=0 "
				"AND l.to_owner_type=1 AND l.to_owner_context_id=0 AND l.item_revision=1 "
				"AND l.item_uid=l.root_item_uid),l.item_uid,l.from_owner_revision,l.to_owner_revision,"
				"c.vnum,l.to_owner_id "
				"FROM item_ownership_ledger l JOIN critical_operation_inbox i "
				"ON i.operation_id=l.operation_id LEFT JOIN item_current_owner c ON c.item_uid=l.item_uid "
				"WHERE l.reason_type=2 AND l.reason_id IN (" +
					(item_sources.empty() ? "NULL" : item_sources) +
					") AND (l.parent_item_uid IS NULL OR l.parent_item_uid=0) LIMIT " +
					std::to_string(expected_count + 1) :
				"SELECT LOWER(HEX(i.operation_id)),i.result_payload,(i.status=1 AND i.result_code=0 AND i.command_type=3 "
				"AND l.reason_type=5 AND l.source_site=5 AND l.bank_delta_copper=0 AND l.bank_delta_silver=0 "
				"AND l.bank_delta_gold=0 AND l.bank_delta_platinum=0),l.wallet_after_copper,l.wallet_after_silver,"
				"l.wallet_after_gold,l.wallet_after_platinum,l.bank_after_copper,l.bank_after_silver,"
				"l.bank_after_gold,l.bank_after_platinum,l.wallet_revision,l.bank_revision,"
				"l.pid,l.reason_id,l.wallet_delta_copper,l.wallet_delta_silver,l.wallet_delta_gold,l.wallet_delta_platinum "
				"FROM critical_operation_inbox i LEFT JOIN currency_ledger l ON l.operation_id=i.operation_id "
				"WHERE i.operation_id IN (" +
					(cash_operations.empty() ? "NULL" : cash_operations) +
					") LIMIT " + std::to_string(expected_count + 1);
		++metrics->query_count;
		if (mysql_real_query(connection, query.data(), query.size()))
		{
			if (database_error)
				*database_error = mysql_errno(connection);
			return quest_reward_obligation_result::database_error;
		}
		std::unique_ptr<MYSQL_RES, decltype(&mysql_free_result)> rows(
			mysql_store_result(connection), mysql_free_result);
		if (!rows)
		{
			if (database_error)
				*database_error = mysql_errno(connection);
			return quest_reward_obligation_result::database_error;
		}
		MYSQL_ROW row = nullptr;
		while ((row = mysql_fetch_row(rows.get())))
		{
			const auto lengths = mysql_fetch_lengths(rows.get());
			if (!lengths)
				return quest_reward_obligation_result::corrupt;
			++metrics->row_count;
			for (unsigned int column = 0; column < mysql_num_fields(rows.get());
			     ++column)
				metrics->byte_count += row[column] ? lengths[column] : 0;
			if (!row[0] || !row[1] || !row[2] || strcmp(row[2], "1"))
				return quest_reward_obligation_result::corrupt;
			expected_receipt *expected = nullptr;
			if (item_query)
			{
				uint64_t source = 0;
				if (!receipt_number(row[0], &source))
					return quest_reward_obligation_result::corrupt;
				const auto found = items.find(source);
				if (found != items.end())
					expected = &found->second;
			}
			else
			{
				const auto found = cash.find(row[0]);
				if (found != cash.end())
					expected = &found->second;
			}
			if (!expected || expected->seen)
				return quest_reward_obligation_result::corrupt;
			expected->seen = true;
			auto &record = (*records)[expected->record_index];
			const auto &reward = record.terms.rewards[expected->reward_index];
			if (item_query)
			{
				item_transfer_result result = {};
				uint64_t uid = 0, from = 0, to = 0, vnum = 0, pid = 0;
				if (!item_transfer_command_decode_result(
					    reinterpret_cast<const uint8_t *>(row[1]), lengths[1],
					    &result) ||
				    !receipt_number(row[3], &uid) ||
				    !receipt_number(row[4], &from) ||
				    !receipt_number(row[5], &to) ||
				    !receipt_number(row[6], &vnum) ||
				    !receipt_number(row[7], &pid) || vnum != reward.number ||
				    pid != record.terms.player_pid || !uid ||
				    result.root_item_uid != uid || !result.item_count ||
				    result.max_item_revision != 1 ||
				    result.from_owner_revision != from ||
				    result.to_owner_revision != to)
					return quest_reward_obligation_result::corrupt;
			}
			else
			{
				currency_command_result result = {};
				uint64_t pid = 0, reason = 0;
				if (!currency_command_decode_result(
					    reinterpret_cast<const uint8_t *>(row[1]), lengths[1],
					    &result) ||
				    !receipt_number(row[13], &pid) ||
				    pid != record.terms.player_pid ||
				    !receipt_number(row[14], &reason) ||
				    reason != expected->reward_index + 1)
					return quest_reward_obligation_result::corrupt;
				uint64_t value = reward.number;
				for (size_t field = 0; field < 4; ++field)
				{
					uint64_t delta = 0;
					if (!receipt_number(row[field + 15], &delta) ||
					    delta != (field == 3 ? value : value % 10))
						return quest_reward_obligation_result::corrupt;
					value /= 10;
				}
				for (size_t field = 0; field < 10; ++field)
				{
					uint64_t expected = 0;
					if (!receipt_number(row[field + 3], &expected))
						return quest_reward_obligation_result::corrupt;
					const uint64_t actual =
						field < 4 ? static_cast<uint64_t>(
								    result.wallet.amount[field]) :
						field < 8 ? static_cast<uint64_t>(
								    result.bank.amount[field - 4]) :
						field == 8 ? result.wallet_revision :
							     result.bank_revision;
					if (actual != expected)
						return quest_reward_obligation_result::corrupt;
				}
			}
			record.economic_applied_mask |= UINT64_C(1) << expected->reward_index;
		}
	}
	return quest_reward_obligation_result::ok;
}
catch (const std::bad_alloc &)
{
	if (database_error)
		*database_error = ENOMEM;
	return quest_reward_obligation_result::database_error;
}
}
#endif

quest_reward_obligation_result
quest_reward_obligation_repository_pending(MYSQL *connection, uint32_t player_pid,
					   std::vector<quest_reward_obligation_record> *obligations,
					   unsigned int *database_error_code,
					   quest_reward_read_metrics *metrics)
{
	quest_reward_read_metrics local_metrics;
	if (!metrics)
		metrics = &local_metrics;
	*metrics = {};
	if (database_error_code)
		*database_error_code = 0;
	if (!connection || !player_pid || !obligations)
		return quest_reward_obligation_result::invalid;
#ifdef __NO_MYSQL__
	if (database_error_code)
		*database_error_code = ENOTSUP;
	return quest_reward_obligation_result::database_error;
#else
	static const char SQL[] =
		"SELECT q.offering_operation_id,q.continuation,i.status,i.result_code,q.xp_applied_mask "
		"FROM quest_reward_obligation q JOIN critical_operation_inbox i "
		"ON i.operation_id=q.offering_operation_id "
		"WHERE q.player_pid=? AND q.acknowledged_at IS NULL "
		"ORDER BY q.created_at,q.offering_operation_id LIMIT 65";
	MYSQL_STMT *statement = mysql_stmt_init(connection);
	if (!statement)
	{
		if (database_error_code)
			*database_error_code = mysql_errno(connection) ? mysql_errno(connection) :
									 ENOMEM;
		return quest_reward_obligation_result::database_error;
	}
	auto fail_database = [&]()
	{
		if (database_error_code)
			*database_error_code = mysql_stmt_errno(statement);
		mysql_stmt_close(statement);
		return quest_reward_obligation_result::database_error;
	};
	if (mysql_stmt_prepare(statement, SQL, sizeof(SQL) - 1))
		return fail_database();
	MYSQL_BIND parameter = {};
	parameter.buffer_type = MYSQL_TYPE_LONG;
	parameter.buffer = &player_pid;
	parameter.is_unsigned = true;
	if (mysql_stmt_bind_param(statement, &parameter))
		return fail_database();
	++metrics->query_count;
	if (mysql_stmt_execute(statement) || mysql_stmt_store_result(statement))
		return fail_database();
	quest_reward_obligation_record record;
	std::array<uint8_t, ITEM_TRANSFER_CONTINUATION_MAX_BYTES> continuation = {};
	unsigned long operation_length = 0, continuation_length = 0;
	uint8_t status = 0;
	uint32_t result_code = 0;
	uint64_t xp_applied_mask = 0;
	MYSQL_BIND results[5] = {};
	results[0].buffer_type = MYSQL_TYPE_BLOB;
	results[0].buffer = record.offering_operation.bytes.data();
	results[0].buffer_length = record.offering_operation.bytes.size();
	results[0].length = &operation_length;
	results[1].buffer_type = MYSQL_TYPE_BLOB;
	results[1].buffer = continuation.data();
	results[1].buffer_length = continuation.size();
	results[1].length = &continuation_length;
	results[2].buffer_type = MYSQL_TYPE_TINY;
	results[2].buffer = &status;
	results[2].is_unsigned = true;
	results[3].buffer_type = MYSQL_TYPE_LONG;
	results[3].buffer = &result_code;
	results[3].is_unsigned = true;
	results[4].buffer_type = MYSQL_TYPE_LONGLONG;
	results[4].buffer = &xp_applied_mask;
	results[4].is_unsigned = true;
	if (mysql_stmt_bind_result(statement, results))
		return fail_database();
	std::vector<quest_reward_obligation_record> selected;
	for (;;)
	{
		const int fetched = mysql_stmt_fetch(statement);
		if (fetched == MYSQL_NO_DATA)
			break;
		if (!fetched || fetched == MYSQL_DATA_TRUNCATED)
		{
			++metrics->row_count;
			metrics->byte_count += operation_length + continuation_length +
					       sizeof(status) + sizeof(result_code) +
					       sizeof(xp_applied_mask);
		}
		if (fetched == MYSQL_DATA_TRUNCATED)
		{
			mysql_stmt_close(statement);
			return quest_reward_obligation_result::corrupt;
		}
		if (fetched)
			return fail_database();
		if (selected.size() == QUEST_REWARD_PENDING_MAX)
		{
			mysql_stmt_close(statement);
			return quest_reward_obligation_result::limit_exceeded;
		}
		quest_reward_continuation terms;
		if (operation_length != record.offering_operation.bytes.size() ||
		    critical_operation_id_is_zero(record.offering_operation) || status != 1 ||
		    result_code ||
		    !quest_reward_continuation_decode(continuation.data(), continuation_length,
						      &terms) ||
		    terms.player_pid != player_pid)
		{
			mysql_stmt_close(statement);
			return quest_reward_obligation_result::corrupt;
		}
		try
		{
			record.continuation.assign(continuation.begin(),
						   continuation.begin() + continuation_length);
			record.terms = terms;
			record.xp_applied_mask = xp_applied_mask;
			selected.push_back(record);
		}
		catch (const std::bad_alloc &)
		{
			mysql_stmt_close(statement);
			if (database_error_code)
				*database_error_code = ENOMEM;
			return quest_reward_obligation_result::database_error;
		}
	}
	mysql_stmt_close(statement);
	const auto receipts =
		read_economic_receipts(connection, &selected, database_error_code, metrics);
	if (receipts != quest_reward_obligation_result::ok)
		return receipts;
	*obligations = std::move(selected);
	return quest_reward_obligation_result::ok;
#endif
}

quest_reward_obligation_result quest_reward_xp_entitlement_repository_pending(
	MYSQL *connection, uint32_t recipient_pid,
	std::vector<quest_reward_xp_entitlement_record> *entitlements,
	unsigned int *database_error_code, quest_reward_read_metrics *metrics)
{
	quest_reward_read_metrics local_metrics;
	if (!metrics)
		metrics = &local_metrics;
	*metrics = {};
	if (database_error_code)
		*database_error_code = 0;
	if (!connection || !recipient_pid || !entitlements)
		return quest_reward_obligation_result::invalid;
#ifdef __NO_MYSQL__
	if (database_error_code)
		*database_error_code = ENOTSUP;
	return quest_reward_obligation_result::database_error;
#else
	static const char SQL[] =
		"SELECT e.offering_operation_id,q.continuation,e.reward_index,e.amount "
		"FROM quest_reward_xp_entitlement e JOIN quest_reward_obligation q "
		"ON q.offering_operation_id=e.offering_operation_id "
		"JOIN critical_operation_inbox i ON i.operation_id=q.offering_operation_id "
		"WHERE e.recipient_pid=? AND e.applied_at IS NULL AND q.acknowledged_at IS NULL "
		"AND i.status=1 AND i.result_code=0 ORDER BY q.created_at,e.reward_index LIMIT 65";
	MYSQL_STMT *statement = mysql_stmt_init(connection);
	if (!statement)
	{
		if (database_error_code)
			*database_error_code = mysql_errno(connection) ? mysql_errno(connection) :
									 ENOMEM;
		return quest_reward_obligation_result::database_error;
	}
	auto fail_database = [&]()
	{
		if (database_error_code)
			*database_error_code = mysql_stmt_errno(statement);
		mysql_stmt_close(statement);
		return quest_reward_obligation_result::database_error;
	};
	if (mysql_stmt_prepare(statement, SQL, sizeof(SQL) - 1))
		return fail_database();
	MYSQL_BIND parameter = {};
	parameter.buffer_type = MYSQL_TYPE_LONG;
	parameter.buffer = &recipient_pid;
	parameter.is_unsigned = true;
	if (mysql_stmt_bind_param(statement, &parameter))
		return fail_database();
	++metrics->query_count;
	if (mysql_stmt_execute(statement) || mysql_stmt_store_result(statement))
		return fail_database();
	quest_reward_xp_entitlement_record record;
	std::array<uint8_t, ITEM_TRANSFER_CONTINUATION_MAX_BYTES> continuation = {};
	unsigned long operation_length = 0, continuation_length = 0;
	uint32_t reward_index = 0, amount = 0;
	MYSQL_BIND results[4] = {};
	results[0].buffer_type = MYSQL_TYPE_BLOB;
	results[0].buffer = record.offering_operation.bytes.data();
	results[0].buffer_length = record.offering_operation.bytes.size();
	results[0].length = &operation_length;
	results[1].buffer_type = MYSQL_TYPE_BLOB;
	results[1].buffer = continuation.data();
	results[1].buffer_length = continuation.size();
	results[1].length = &continuation_length;
	results[2].buffer_type = MYSQL_TYPE_LONG;
	results[2].buffer = &reward_index;
	results[2].is_unsigned = true;
	results[3].buffer_type = MYSQL_TYPE_LONG;
	results[3].buffer = &amount;
	results[3].is_unsigned = true;
	if (mysql_stmt_bind_result(statement, results))
		return fail_database();
	std::vector<quest_reward_xp_entitlement_record> selected;
	for (;;)
	{
		const int fetched = mysql_stmt_fetch(statement);
		if (fetched == MYSQL_NO_DATA)
			break;
		if (!fetched || fetched == MYSQL_DATA_TRUNCATED)
		{
			++metrics->row_count;
			metrics->byte_count += operation_length + continuation_length +
					       sizeof(reward_index) + sizeof(amount);
		}
		if (fetched == MYSQL_DATA_TRUNCATED)
		{
			mysql_stmt_close(statement);
			return quest_reward_obligation_result::corrupt;
		}
		if (fetched)
			return fail_database();
		if (selected.size() == QUEST_REWARD_PENDING_MAX)
		{
			mysql_stmt_close(statement);
			return quest_reward_obligation_result::limit_exceeded;
		}
		quest_reward_continuation terms;
		if (operation_length != record.offering_operation.bytes.size() ||
		    critical_operation_id_is_zero(record.offering_operation) || !amount ||
		    !quest_reward_continuation_decode(continuation.data(), continuation_length,
						      &terms) ||
		    terms.version < 5 || reward_index >= terms.reward_count ||
		    terms.rewards[reward_index].type != 5U)
		{
			mysql_stmt_close(statement);
			return quest_reward_obligation_result::corrupt;
		}
		bool matched = false;
		for (size_t index = 0; index < terms.xp_award_count; ++index)
			if (terms.xp_awards[index].recipient_pid == recipient_pid &&
			    terms.xp_awards[index].reward_index == reward_index &&
			    terms.xp_awards[index].amount == amount)
				matched = true;
		if (!matched)
		{
			mysql_stmt_close(statement);
			return quest_reward_obligation_result::corrupt;
		}
		try
		{
			record.continuation.assign(continuation.begin(),
						   continuation.begin() + continuation_length);
			record.terms = terms;
			record.reward_index = reward_index;
			record.amount = amount;
			selected.push_back(record);
		}
		catch (const std::bad_alloc &)
		{
			mysql_stmt_close(statement);
			if (database_error_code)
				*database_error_code = ENOMEM;
			return quest_reward_obligation_result::database_error;
		}
	}
	mysql_stmt_close(statement);
	*entitlements = std::move(selected);
	return quest_reward_obligation_result::ok;
#endif
}

quest_reward_obligation_result
quest_reward_obligation_repository_acknowledge(MYSQL *connection, uint32_t player_pid,
					       const critical_operation_id &offering_operation,
					       unsigned int *database_error_code)
{
	if (database_error_code)
		*database_error_code = 0;
	if (!connection || !player_pid || critical_operation_id_is_zero(offering_operation))
		return quest_reward_obligation_result::invalid;
#ifdef __NO_MYSQL__
	if (database_error_code)
		*database_error_code = ENOTSUP;
	return quest_reward_obligation_result::database_error;
#else
	static const char UPDATE_SQL[] =
		"UPDATE quest_reward_obligation SET acknowledged_at=CURRENT_TIMESTAMP(6) "
		"WHERE offering_operation_id=? AND player_pid=? AND acknowledged_at IS NULL "
		"AND NOT EXISTS (SELECT 1 FROM quest_reward_xp_entitlement e "
		"WHERE e.offering_operation_id=quest_reward_obligation.offering_operation_id "
		"AND e.applied_at IS NULL)";
	MYSQL_STMT *statement = mysql_stmt_init(connection);
	if (!statement)
	{
		if (database_error_code)
			*database_error_code = mysql_errno(connection) ? mysql_errno(connection) :
									 ENOMEM;
		return quest_reward_obligation_result::database_error;
	}
	if (mysql_stmt_prepare(statement, UPDATE_SQL, sizeof(UPDATE_SQL) - 1))
	{
		if (database_error_code)
			*database_error_code = mysql_stmt_errno(statement);
		mysql_stmt_close(statement);
		return quest_reward_obligation_result::database_error;
	}
	unsigned long operation_length = offering_operation.bytes.size();
	MYSQL_BIND parameters[2] = {};
	parameters[0].buffer_type = MYSQL_TYPE_BLOB;
	parameters[0].buffer = const_cast<uint8_t *>(offering_operation.bytes.data());
	parameters[0].buffer_length = operation_length;
	parameters[0].length = &operation_length;
	parameters[1].buffer_type = MYSQL_TYPE_LONG;
	parameters[1].buffer = &player_pid;
	parameters[1].is_unsigned = true;
	if (mysql_stmt_bind_param(statement, parameters) || mysql_stmt_execute(statement))
	{
		if (database_error_code)
			*database_error_code = mysql_stmt_errno(statement);
		mysql_stmt_close(statement);
		return quest_reward_obligation_result::database_error;
	}
	const my_ulonglong changed = mysql_stmt_affected_rows(statement);
	mysql_stmt_close(statement);
	if (changed == 1)
		return quest_reward_obligation_result::ok;

	static const char CHECK_SQL[] =
		"SELECT q.acknowledged_at IS NOT NULL, EXISTS(SELECT 1 "
		"FROM quest_reward_xp_entitlement e "
		"WHERE e.offering_operation_id=q.offering_operation_id AND e.applied_at IS NULL) "
		"FROM quest_reward_obligation q "
		"WHERE q.offering_operation_id=? AND q.player_pid=?";
	statement = mysql_stmt_init(connection);
	if (!statement)
	{
		if (database_error_code)
			*database_error_code = mysql_errno(connection) ? mysql_errno(connection) :
									 ENOMEM;
		return quest_reward_obligation_result::database_error;
	}
	if (mysql_stmt_prepare(statement, CHECK_SQL, sizeof(CHECK_SQL) - 1))
	{
		if (database_error_code)
			*database_error_code = mysql_stmt_errno(statement);
		mysql_stmt_close(statement);
		return quest_reward_obligation_result::database_error;
	}
	operation_length = offering_operation.bytes.size();
	if (mysql_stmt_bind_param(statement, parameters) || mysql_stmt_execute(statement) ||
	    mysql_stmt_store_result(statement))
	{
		if (database_error_code)
			*database_error_code = mysql_stmt_errno(statement);
		mysql_stmt_close(statement);
		return quest_reward_obligation_result::database_error;
	}
	uint8_t acknowledged = 0, pending_effects = 0;
	MYSQL_BIND result[2] = {};
	result[0].buffer_type = MYSQL_TYPE_TINY;
	result[0].buffer = &acknowledged;
	result[0].is_unsigned = true;
	result[1].buffer_type = MYSQL_TYPE_TINY;
	result[1].buffer = &pending_effects;
	result[1].is_unsigned = true;
	if (mysql_stmt_bind_result(statement, result))
	{
		if (database_error_code)
			*database_error_code = mysql_stmt_errno(statement);
		mysql_stmt_close(statement);
		return quest_reward_obligation_result::database_error;
	}
	const int fetched = mysql_stmt_fetch(statement);
	mysql_stmt_close(statement);
	if (fetched == MYSQL_NO_DATA)
		return quest_reward_obligation_result::not_found;
	if (fetched || acknowledged > 1 || pending_effects > 1)
		return quest_reward_obligation_result::corrupt;
	if (acknowledged)
		return quest_reward_obligation_result::already_acknowledged;
	return pending_effects ? quest_reward_obligation_result::pending_effects :
				 quest_reward_obligation_result::corrupt;
#endif
}
