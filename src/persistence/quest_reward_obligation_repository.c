#include "persistence/quest_reward_obligation_repository.h"

#include <array>
#include <cerrno>
#include <new>
#include <utility>

quest_reward_obligation_result
quest_reward_obligation_repository_pending(MYSQL *connection, uint32_t player_pid,
					   std::vector<quest_reward_obligation_record> *obligations,
					   unsigned int *database_error_code)
{
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
	if (mysql_stmt_bind_param(statement, &parameter) || mysql_stmt_execute(statement) ||
	    mysql_stmt_store_result(statement))
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
		if (fetched == MYSQL_DATA_TRUNCATED)
		{
			mysql_stmt_close(statement);
			return quest_reward_obligation_result::corrupt;
		}
		if (fetched)
			return fail_database();
		if (selected.size() == 64)
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
	*obligations = std::move(selected);
	return quest_reward_obligation_result::ok;
#endif
}

quest_reward_obligation_result quest_reward_xp_entitlement_repository_pending(
	MYSQL *connection, uint32_t recipient_pid,
	std::vector<quest_reward_xp_entitlement_record> *entitlements,
	unsigned int *database_error_code)
{
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
	if (mysql_stmt_bind_param(statement, &parameter) || mysql_stmt_execute(statement) ||
	    mysql_stmt_store_result(statement))
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
		if (fetched == MYSQL_DATA_TRUNCATED)
		{
			mysql_stmt_close(statement);
			return quest_reward_obligation_result::corrupt;
		}
		if (fetched)
			return fail_database();
		if (selected.size() == 64)
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
