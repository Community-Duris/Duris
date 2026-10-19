#include "economy/shop_trade_repository.h"
#include "economy/currency_repository.h"
#include "item/item_transfer_repository.h"
#include "economy/collector_repository.h"
#include "persistence/critical_command_repository.h"
#include <algorithm>
#include <cerrno>
#include <charconv>
#include <cstring>
#include <memory>
#include <string>

namespace
{
bool execute(MYSQL *connection, const std::string &sql)
{
	return mysql_real_query(connection, sql.data(), sql.size()) == 0;
}
template <typename T> bool number(const char *value, T *result)
{
	if (!value)
		return false;
	const auto end = value + strlen(value);
	const auto parsed = std::from_chars(value, end, *result);
	return parsed.ec == std::errc{} && parsed.ptr == end;
}
}

bool shop_trade_repository_execute(MYSQL *connection, const critical_command &command,
				   shop_trade_result *result)
{
	shop_trade_payload payload;
	if (!connection || !result ||
	    command.schema_version != CRITICAL_COMMAND_OUTCOME_SCHEMA_VERSION ||
	    !shop_trade_command_decode_payload(command, &payload) ||
	    !shop_trade_command_decode_result(command.outcome_payload.data(),
					      command.outcome_payload.size(), result) ||
	    payload.expected_shop_revision == UINT64_MAX ||
	    result->shop_revision != payload.expected_shop_revision + 1)
	{
		errno = EINVAL;
		return false;
	}
	const std::string shop = std::to_string(payload.shop_id);
	if (payload.action == shop_trade_action::buy_produced)
	{
		// The exemplar stays in stock; nevertheless it is part of the decision's
		// preconditions and must still identify the same durable stock on replay.
		if (!execute(
			    connection,
			    "SELECT item_revision,vnum FROM item_current_owner WHERE item_uid=" +
				    std::to_string(payload.stock_item_uid) +
				    " AND root_item_uid=item_uid AND parent_item_uid IS NULL AND owner_type=" +
				    std::to_string(
					    static_cast<unsigned>(item_owner_type::shopkeeper)) +
				    " AND owner_id=" +
				    std::to_string(item_shopkeeper_owner_id(payload.shop_id)) +
				    " AND owner_context_id=0 AND state=1 FOR UPDATE"))
			return false;
		std::unique_ptr<MYSQL_RES, decltype(&mysql_free_result)> rows(
			mysql_store_result(connection), mysql_free_result);
		MYSQL_ROW row = rows ? mysql_fetch_row(rows.get()) : nullptr;
		uint64_t revision = 0;
		int32_t vnum = 0;
		if (!row || !number(row[0], &revision) || !number(row[1], &vnum) ||
		    revision != payload.expected_stock_item_revision || vnum != payload.stock_vnum)
		{
			errno = ESTALE;
			return false;
		}
	}
	item_transfer_payload transfer;
	collector_item_boundary_repository_plan boundary;
	unsigned int boundary_error = 0;
	if (!shop_trade_item_payload(payload, &transfer) ||
	    !collector_repository_prepare_item_boundary(connection, transfer, &boundary,
							&boundary_error))
		return false;
	if (boundary_error)
	{
		errno = boundary_error;
		return false;
	}
	if (!execute(connection,
		     "UPDATE shop_trade_state SET revision=" +
			     std::to_string(result->shop_revision) + " WHERE shop_id=" + shop +
			     " AND revision=" + std::to_string(payload.expected_shop_revision)))
		return false;
	if (mysql_affected_rows(connection) != 1)
	{
		errno = ESTALE;
		return false;
	}
	if (payload.action != shop_trade_action::discard_invalid)
	{
		if (!execute(connection,
			     "SELECT copper,silver,gold,platinum FROM player_data WHERE pid=" +
				     std::to_string(payload.player_pid) + " FOR UPDATE"))
			return false;
		std::unique_ptr<MYSQL_RES, decltype(&mysql_free_result)> rows(
			mysql_store_result(connection), mysql_free_result);
		MYSQL_ROW row = rows ? mysql_fetch_row(rows.get()) : nullptr;
		currency_command_payload debit{};
		debit.pid = payload.player_pid;
		debit.account_name = payload.account_name;
		debit.racewar = payload.racewar;
		debit.reason = payload.action == shop_trade_action::buy_existing ||
					       payload.action == shop_trade_action::buy_produced ?
				       currency_reason_type::wallet_spend :
				       currency_reason_type::wallet_reward;
		debit.reason_id = payload.shop_id;
		int64_t before_value = 0, after_value = 0, denomination = 1;
		for (size_t i = 0; i < 4; ++i)
		{
			int64_t before = 0;
			if (!row || !number(row[i], &before) || before < 0 || before > INT32_MAX)
			{
				errno = EILSEQ;
				return false;
			}
			debit.wallet_delta.amount[i] = result->wallet.amount[i] - before;
			before_value += before * denomination;
			after_value += result->wallet.amount[i] * denomination;
			denomination *= 10;
		}
		const int64_t delta = debit.reason == currency_reason_type::wallet_spend ?
					      -payload.price :
					      payload.price;
		if (after_value - before_value != delta)
		{
			errno = EILSEQ;
			return false;
		}
		critical_command child;
		currency_command_result paid;
		unsigned int error = 0;
		bool mutated = false;
		if (!currency_command_build(&child, command.operation_id, debit,
					    payload.expected_wallet_revision,
					    payload.expected_bank_revision, command.source_site,
					    command.deadline_class) ||
		    !currency_repository_execute(connection, child, &paid, &error, &mutated))
			return false;
		if (error || !mutated || paid.wallet.amount != result->wallet.amount ||
		    paid.bank.amount != result->bank.amount ||
		    paid.wallet_revision != result->wallet_revision ||
		    paid.bank_revision != result->bank_revision)
		{
			errno = error ? error : EILSEQ;
			return false;
		}
	}
	const bool buying = payload.action == shop_trade_action::buy_existing ||
			    payload.action == shop_trade_action::buy_produced;
	const uint64_t from = buying ? result->counterparty_owner_revision :
				       result->player_owner_revision;
	const uint64_t to = buying ? result->player_owner_revision :
				     result->counterparty_owner_revision;
	if (!from || !to)
	{
		errno = EILSEQ;
		return false;
	}
	transfer.expected_from_revision = from - 1;
	transfer.expected_to_revision = to - 1;
	critical_command item;
	item_transfer_result moved;
	unsigned int error = 0;
	bool mutated = false;
	if (!item_transfer_command_build(&item, command.operation_id, transfer, command.source_site,
					 command.deadline_class) ||
	    !item_transfer_repository_execute(connection, item, &moved, &error, &mutated))
		return false;
	if (error || !mutated || moved.from_owner_revision != from ||
	    moved.to_owner_revision != to || moved.item_count != result->item_count)
	{
		errno = error ? error : EILSEQ;
		return false;
	}
	for (size_t i = 0; i < result->item_count; ++i)
	{
		const auto &entry = payload.items[i];
		if (result->item_uids[i] != entry.item_uid ||
		    result->item_revisions[i] !=
			    (payload.action == shop_trade_action::buy_produced ?
				     1 :
				     entry.expected_item_revision + 1))
		{
			errno = EILSEQ;
			return false;
		}
		// Detach first: deleting a container must not cascade an unrelated child.
		const char *table = buying || payload.action == shop_trade_action::discard_invalid ?
					    "shopkeeper_items" :
					    "player_items";
		if (payload.action != shop_trade_action::buy_produced &&
		    !execute(connection, "UPDATE " + std::string(table) +
						 " SET container_id=NULL WHERE obj_uid=" +
						 std::to_string(entry.item_uid)))
			return false;
	}
	if (!boundary.entries.empty())
	{
		uint64_t revision = 0;
		std::vector<collector_command_result> events;
		if (!collector_repository_apply_item_boundary(connection, command, boundary,
							      &revision, &events))
			return false;
		for (size_t i = 0; i < events.size(); ++i)
		{
			std::array<uint8_t, COLLECTOR_COMMAND_RESULT_BYTES> bytes;
			if (!collector_command_encode_result(events[i], &bytes) ||
			    !critical_command_repository_insert_outbox_event(
				    connection, command.operation_id, static_cast<uint16_t>(i + 1),
				    COLLECTOR_OUTBOX_DESTINATION, COLLECTOR_OUTBOX_EVENT_MUTATED,
				    COLLECTOR_COMMAND_RESULT_VERSION, bytes.data(), bytes.size()))
				return false;
		}
	}
	for (size_t i = 0; i < result->item_count; ++i)
	{
		const char *table = buying || payload.action == shop_trade_action::discard_invalid ?
					    "shopkeeper_items" :
					    "player_items";
		if (payload.action != shop_trade_action::buy_produced &&
		    !execute(connection, "DELETE FROM " + std::string(table) + " WHERE obj_uid=" +
						 std::to_string(payload.items[i].item_uid)))
			return false;
	}
	return true;
}
