#include "persistence/economic_sql_shop_trade_transaction.h"

#include <cerrno>

#ifndef __NO_MYSQL__
#include <algorithm>
#include <charconv>
#include <climits>
#include <cstring>
#include <memory>
#include <new>
#include <optional>
#include <string>
#include <strings.h>
#include <tuple>
#include <vector>

namespace
{
constexpr uint16_t PLAYER_LOCATOR = 1;
constexpr uint16_t BANK_LOCATOR = 2;
constexpr uint16_t SHOPKEEPER_LOCATOR = 6;
using cell = std::optional<std::string>;

struct failure
{
	unsigned int code;
};

void require(bool condition, unsigned int code = EILSEQ)
{
	if (!condition)
		throw failure{ code };
}

void execute(MYSQL *connection, const std::string &sql)
{
	if (mysql_real_query(connection, sql.data(), sql.size()))
		throw failure{ mysql_errno(connection) ? mysql_errno(connection) : EIO };
}

std::vector<cell> one(MYSQL *connection, const std::string &sql, size_t fields,
		      bool optional = false)
{
	execute(connection, sql);
	std::unique_ptr<MYSQL_RES, decltype(&mysql_free_result)> rows(
		mysql_store_result(connection), mysql_free_result);
	require(bool(rows), mysql_errno(connection) ? mysql_errno(connection) : EIO);
	require(mysql_num_fields(rows.get()) == fields && mysql_num_rows(rows.get()) <= 1);
	if (!mysql_num_rows(rows.get()))
	{
		require(optional, ENOENT);
		return {};
	}
	MYSQL_ROW raw = mysql_fetch_row(rows.get());
	const unsigned long *lengths = mysql_fetch_lengths(rows.get());
	require(raw && lengths);
	std::vector<cell> result;
	result.reserve(fields);
	for (size_t index = 0; index < fields; ++index)
		result.push_back(raw[index] ? cell(std::string(raw[index], lengths[index])) :
					      std::nullopt);
	return result;
}

template <typename T> T number(const cell &value)
{
	require(value && !value->empty());
	T result = 0;
	const auto parsed = std::from_chars(value->data(), value->data() + value->size(), result);
	require(parsed.ec == std::errc{} && parsed.ptr == value->data() + value->size());
	return result;
}

std::string quoted(MYSQL *connection, const char *value, size_t size)
{
	std::string escaped(size * 2 + 1, '\0');
	const auto length = mysql_real_escape_string(connection, escaped.data(), value, size);
	escaped.resize(length);
	return "'" + escaped + "'";
}

uint32_t native_id(MYSQL *connection, uint64_t mapping)
{
	const auto row = one(connection,
			     "SELECT native_id FROM economic_account_mapping WHERE mapping_id=" +
				     std::to_string(mapping),
			     1);
	const auto id = number<uint64_t>(row[0]);
	require(id && id <= UINT32_MAX);
	return static_cast<uint32_t>(id);
}

item_owner_identity player_owner(const shop_trade_payload &payload)
{
	return { item_owner_type::player, payload.player_pid, 0 };
}

item_owner_identity shop_owner(const shop_trade_payload &payload)
{
	return { item_owner_type::shopkeeper, item_shopkeeper_owner_id(payload.shop_id), 0 };
}

item_owner_identity primary_owner(const shop_trade_payload &payload)
{
	return payload.action == shop_trade_action::discard_invalid ? shop_owner(payload) :
								      player_owner(payload);
}

item_owner_identity counterparty_owner(const shop_trade_payload &payload)
{
	if (payload.action == shop_trade_action::buy_produced)
		return { item_owner_type::system, 0, 0 };
	if (payload.action == shop_trade_action::sell_destroy ||
	    payload.action == shop_trade_action::discard_invalid)
		return { item_owner_type::destruction, 0, 0 };
	return shop_owner(payload);
}

uint64_t owner_revision(MYSQL *connection, const item_owner_identity &owner)
{
	require(item_owner_identity_valid(owner), EINVAL);
	const std::string identity =
		"owner_type=" + std::to_string(static_cast<uint8_t>(owner.type)) +
		" AND owner_id=" + std::to_string(owner.id) +
		" AND owner_context_id=" + std::to_string(owner.context_id);
	execute(connection,
		"INSERT IGNORE INTO item_owner_revision(owner_type,owner_id,owner_context_id,revision) "
		"VALUES(" +
			std::to_string(static_cast<uint8_t>(owner.type)) + "," +
			std::to_string(owner.id) + "," + std::to_string(owner.context_id) + ",0)");
	const auto row = one(
		connection,
		"SELECT revision FROM item_owner_revision WHERE " + identity + " FOR UPDATE", 1);
	return number<uint64_t>(row[0]);
}

void balances(MYSQL *connection, const shop_trade_payload &payload, uint32_t bank_id,
	      shop_trade_accounting_authority *before)
{
	const auto player =
		one(connection,
		    "SELECT account_name,racewar,copper,silver,gold,platinum,wallet_revision "
		    "FROM player_data WHERE pid=" +
			    std::to_string(payload.player_pid) + " FOR UPDATE",
		    7);
	require(player[0] && !strcasecmp(player[0]->c_str(), payload.account_name.data()) &&
			number<uint8_t>(player[1]) == payload.racewar,
		ESTALE);
	for (size_t index = 0; index < 4; ++index)
	{
		const auto amount = number<int64_t>(player[index + 2]);
		require(amount >= 0, ERANGE);
		before->balances_before.wallet.amount[index] = amount;
	}
	before->balances_before.wallet_revision = number<uint64_t>(player[6]);
	const auto account =
		quoted(connection, payload.account_name.data(),
		       strnlen(payload.account_name.data(), payload.account_name.size()));
	const auto bank = one(
		connection,
		"SELECT id,bank_copper,bank_silver,bank_gold,bank_platinum,bank_revision "
		"FROM account_banks WHERE account_name=" +
			account + " AND racewar=" + std::to_string(payload.racewar) + " FOR UPDATE",
		6);
	require(number<uint64_t>(bank[0]) == bank_id, ESTALE);
	for (size_t index = 0; index < 4; ++index)
	{
		const auto amount = number<int64_t>(bank[index + 1]);
		require(amount >= 0, ERANGE);
		before->balances_before.bank.amount[index] = amount;
	}
	before->balances_before.bank_revision = number<uint64_t>(bank[5]);
	require(before->balances_before.wallet_revision == payload.expected_wallet_revision &&
			before->balances_before.bank_revision == payload.expected_bank_revision,
		ESTALE);
}

void keeper(MYSQL *connection, const shop_trade_payload &payload, uint32_t keeper_id,
	    shop_trade_accounting_authority *before)
{
	const auto row =
		one(connection,
		    "SELECT shop_id,mob_vnum,cash,shop_revision FROM shopkeepers WHERE id=" +
			    std::to_string(keeper_id) + " FOR UPDATE",
		    4);
	require(number<uint64_t>(row[0]) == payload.shop_id &&
			number<int32_t>(row[1]) == payload.keeper_vnum,
		ESTALE);
	require(row[2].has_value(), ENODATA);
	const auto cash = number<int64_t>(row[2]);
	require(cash >= 0 && cash <= INT_MAX, ERANGE);
	const auto revision = number<uint64_t>(row[3]);
	require(cash == payload.expected_keeper_cash && revision == payload.expected_shop_revision,
		ESTALE);
	before->shop_id = payload.shop_id;
	before->keeper_vnum = payload.keeper_vnum;
	before->keeper_cash_before = cash;
	// Roaming is gameplay configuration carried by the frozen command. A
	// durable SQL configuration witness is still needed before activation.
	before->keeper_roaming = payload.keeper_roaming != 0;
	before->shop_revision_before = revision;
}

struct locked_item
{
	economic_item_snapshot snapshot;
	int32_t vnum = 0;
	bool exists = false;
};

locked_item item(MYSQL *connection, uint64_t uid, bool optional = false)
{
	const auto row = one(connection,
			     "SELECT root_item_uid,COALESCE(parent_item_uid,0),owner_type,owner_id,"
			     "owner_context_id,item_revision,vnum,state,equipment_slot "
			     "FROM item_current_owner WHERE item_uid=" +
				     std::to_string(uid) + " FOR UPDATE",
			     9, optional);
	if (row.empty())
		return { { uid, {} }, 0, false };
	const auto owner_type = number<uint8_t>(row[2]);
	const auto state = number<uint8_t>(row[7]);
	const auto equipment_slot = number<uint16_t>(row[8]);
	require(owner_type <= static_cast<uint8_t>(item_owner_type::pet) &&
			state <= static_cast<uint8_t>(item_custody_state::quarantined) &&
			equipment_slot == 0,
		ESTALE);
	locked_item result;
	result.exists = true;
	result.snapshot.uid = uid;
	result.snapshot.position = { { static_cast<item_owner_type>(owner_type),
				       number<uint64_t>(row[3]), number<uint64_t>(row[4]) },
				     number<uint64_t>(row[0]),
				     number<uint64_t>(row[1]),
				     number<uint64_t>(row[5]),
				     static_cast<item_custody_state>(state),
				     equipment_slot };
	result.vnum = number<int32_t>(row[6]);
	require(item_owner_identity_valid(result.snapshot.position.owner));
	return result;
}

void items(MYSQL *connection, const shop_trade_payload &payload,
	   shop_trade_accounting_authority *before)
{
	std::vector<locked_item> locked;
	locked.reserve(payload.item_count + 4);
	const bool produced = payload.action == shop_trade_action::buy_produced;
	for (size_t index = 0; index < payload.item_count; ++index)
	{
		const auto entry = item(connection, payload.items[index].item_uid, produced);
		require(produced ? !entry.exists :
				   entry.exists && entry.snapshot.position.state ==
							   item_custody_state::active,
			ESTALE);
		locked.push_back(entry);
	}
	if (!produced)
	{
		const auto count =
			one(connection,
			    "SELECT COUNT(*) FROM item_current_owner WHERE root_item_uid=" +
				    std::to_string(payload.selected_item_uid) + " FOR UPDATE",
			    1);
		require(number<uint64_t>(count[0]) == payload.item_count, EMSGSIZE);
	}
	else
	{
		locked.push_back(item(connection, payload.stock_item_uid));
		uint64_t ancestor = payload.target_parent_item_uid;
		while (ancestor)
		{
			require(locked.size() < ECONOMIC_ACCOUNTING_MAX_ITEM_WITNESSES, E2BIG);
			require(std::none_of(locked.begin(), locked.end(), [&](const auto &entry)
					     { return entry.snapshot.uid == ancestor; }));
			const auto parent = item(connection, ancestor);
			locked.push_back(parent);
			ancestor = parent.snapshot.position.parent_uid;
		}
	}
	std::sort(locked.begin(), locked.end(), [](const auto &left, const auto &right)
		  { return left.snapshot.uid < right.snapshot.uid; });
	for (size_t index = 1; index < locked.size(); ++index)
		require(locked[index - 1].snapshot.uid != locked[index].snapshot.uid);
	before->items_before.reserve(locked.size());
	before->item_vnums_before.reserve(locked.size());
	for (const auto &entry : locked)
	{
		before->items_before.push_back(entry.snapshot);
		before->item_vnums_before.push_back(entry.vnum);
	}
}
} // namespace
#endif

unsigned int economic_sql_shop_trade_lock(MYSQL *connection, const critical_command &command,
					  economic_sql_shop_trade_context *context)
{
#ifdef __NO_MYSQL__
	(void)connection;
	(void)command;
	(void)context;
	return ENOTSUP;
#else
	if (!connection || !context || !(connection->server_status & SERVER_STATUS_IN_TRANS))
		return EINVAL;
	try
	{
		economic_frozen_intent intent;
		shop_trade_payload payload = {};
		economic_account_key wallet, bank, treasury;
		if (shop_trade_accounting_decode(command, &intent, &payload, &wallet, &bank,
						 &treasury) != economic_accounting_error::ok)
			return EPROTONOSUPPORT;
		economic_sql_shop_trade_context candidate;
		candidate.bank_id = native_id(connection, bank.authority_id);
		candidate.keeper_id = native_id(connection, treasury.authority_id);
		const std::vector<economic_sql_mapping_request> mappings = {
			{ wallet, PLAYER_LOCATOR, payload.player_pid },
			{ bank, BANK_LOCATOR, candidate.bank_id },
			{ treasury, SHOPKEEPER_LOCATOR, candidate.keeper_id }
		};
		const auto status = economic_sql_lock_authority(connection,
								intent.admission.metadata.lineage,
								intent.admission.metadata.epoch,
								mappings, &candidate.authority);
		if (status)
			return status;
		candidate.session_id = mysql_thread_id(connection);
		auto &before = candidate.before;
		before.epoch = candidate.authority.epoch;
		before.wallet_account = wallet;
		before.bank_account = bank;
		before.keeper_account = treasury;
		balances(connection, payload, candidate.bank_id, &before);
		keeper(connection, payload, candidate.keeper_id, &before);
		const item_owner_identity primary = primary_owner(payload);
		const item_owner_identity other = counterparty_owner(payload);
		require(!item_owner_identity_equal(primary, other));
		const bool primary_first = std::tie(primary.type, primary.id, primary.context_id) <
					   std::tie(other.type, other.id, other.context_id);
		const auto first_revision =
			owner_revision(connection, primary_first ? primary : other);
		const auto second_revision =
			owner_revision(connection, primary_first ? other : primary);
		before.player_owner_revision_before = primary_first ? first_revision :
								      second_revision;
		before.counterparty_owner_revision_before = primary_first ? second_revision :
									    first_revision;
		items(connection, payload, &before);
		require(connection->server_status & SERVER_STATUS_IN_TRANS, ENOTCONN);
		require(mysql_thread_id(connection) == candidate.session_id, ENOTCONN);
		*context = std::move(candidate);
		return 0;
	}
	catch (const failure &error)
	{
		return error.code ? error.code : EIO;
	}
	catch (const std::bad_alloc &)
	{
		return ENOMEM;
	}
#endif
}
