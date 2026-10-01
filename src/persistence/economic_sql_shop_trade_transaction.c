#include "persistence/economic_sql_shop_trade_transaction.h"

#include "item/economic_accounting_item_reference.h"
#include "player/player_snapshot_codec.h"

#include <cerrno>

#ifndef __NO_MYSQL__
#include <algorithm>
#include <charconv>
#include <climits>
#include <cstring>
#include <limits>
#include <memory>
#include <new>
#include <optional>
#include <set>
#include <string>
#include <strings.h>
#include <tuple>
#include <vector>

namespace
{
constexpr uint16_t PLAYER_LOCATOR = 1;
constexpr uint16_t BANK_LOCATOR = 2;
constexpr uint16_t SHOPKEEPER_LOCATOR = 6;
constexpr uint16_t SHOP_ITEM_REASON_BUY = 16;
constexpr uint16_t SHOP_ITEM_REASON_SELL = 17;
constexpr uint16_t SHOP_ITEM_REASON_CREATE = 2;
constexpr uint16_t SHOP_ITEM_REASON_DESTROY = 3;
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

std::string hex(std::span<const uint8_t> bytes)
{
	static constexpr char digits[] = "0123456789abcdef";
	std::string result = "X'";
	result.reserve(bytes.size() * 2 + 3);
	for (uint8_t byte : bytes)
	{
		result += digits[byte >> 4];
		result += digits[byte & 0x0f];
	}
	result += '\'';
	return result;
}

std::string id(const critical_operation_id &value)
{
	return hex(value.bytes);
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

void produced_items(const shop_trade_payload &payload)
{
	if (payload.action != shop_trade_action::buy_produced)
		return;
	std::vector<player_item_snapshot> snapshots;
	require(player_item_snapshot_list_decode(payload.item_blob.data(), payload.item_blob_size,
						 &snapshots) == player_snapshot_codec_result::ok &&
			snapshots.size() == payload.item_count &&
			snapshots.front().object_uid == payload.selected_item_uid,
		EILSEQ);
	std::vector<bool> matched(payload.item_count, false);
	for (size_t index = 0; index < snapshots.size(); ++index)
	{
		const auto &snapshot = snapshots[index];
		const auto found = std::lower_bound(
			payload.items.begin(), payload.items.begin() + payload.item_count,
			snapshot.object_uid, [](const shop_trade_item_entry &entry, uint64_t uid)
			{ return entry.item_uid < uid; });
		const uint64_t parent_uid =
			snapshot.parent_index == PLAYER_SNAPSHOT_NO_PARENT ?
				0 :
				snapshots[static_cast<size_t>(snapshot.parent_index)].object_uid;
		require(found != payload.items.begin() + payload.item_count &&
				found->item_uid == snapshot.object_uid &&
				found->vnum == snapshot.vnum &&
				found->parent_item_uid == parent_uid && !snapshot.equipment_slot,
			EILSEQ);
		const auto position = static_cast<size_t>(found - payload.items.begin());
		require(!matched[position], EILSEQ);
		matched[position] = true;
	}
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
	const auto row = one(connection,
			     "SELECT shop_id,mob_vnum,cash,shop_revision,keeper_roaming "
			     "FROM shopkeepers WHERE id=" +
				     std::to_string(keeper_id) + " FOR UPDATE",
			     5);
	require(number<uint64_t>(row[0]) == payload.shop_id &&
			number<int32_t>(row[1]) == payload.keeper_vnum,
		ESTALE);
	require(row[2].has_value(), ENODATA);
	const auto cash = number<int64_t>(row[2]);
	require(cash >= 0 && cash <= INT_MAX, ERANGE);
	const auto revision = number<uint64_t>(row[3]);
	require(row[4].has_value(), ENODATA);
	const auto roaming = number<uint8_t>(row[4]);
	require(roaming <= 1, ERANGE);
	require(cash == payload.expected_keeper_cash && revision == payload.expected_shop_revision,
		ESTALE);
	require(roaming == payload.keeper_roaming, ESTALE);
	before->shop_id = payload.shop_id;
	before->keeper_vnum = payload.keeper_vnum;
	before->keeper_cash_before = cash;
	before->keeper_roaming = roaming != 0;
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

struct native_domain
{
	const char *table;
	const char *owner_column;
	uint64_t owner_id;
};

struct native_item
{
	uint64_t uid = 0;
	uint64_t row_id = 0;
	uint64_t parent_row_id = 0;
};

uint64_t native_count(MYSQL *connection, const native_domain &domain, uint64_t uid)
{
	const auto row = one(connection,
			     "SELECT COUNT(*) FROM " + std::string(domain.table) + " WHERE " +
				     domain.owner_column + "=" + std::to_string(domain.owner_id) +
				     " AND obj_uid=" + std::to_string(uid) + " FOR UPDATE",
			     1);
	return number<uint64_t>(row[0]);
}

native_item native_row(MYSQL *connection, const native_domain &domain, uint64_t uid, int32_t vnum)
{
	const auto row = one(connection,
			     "SELECT id,COALESCE(container_id,0),vnum,equip_slot FROM " +
				     std::string(domain.table) + " WHERE " + domain.owner_column +
				     "=" + std::to_string(domain.owner_id) +
				     " AND obj_uid=" + std::to_string(uid) + " FOR UPDATE",
			     4, true);
	require(!row.empty(), ESTALE);
	require(number<int32_t>(row[2]) == vnum && number<int32_t>(row[3]) == 0, ESTALE);
	return { uid, number<uint64_t>(row[0]), number<uint64_t>(row[1]) };
}

void insert_operation(MYSQL *connection, const critical_command &command,
		      const economic_frozen_intent &intent, const economic_accounting_plan *plan,
		      const shop_trade_payload &payload, unsigned int result_code)
{
	const auto &meta = intent.admission.metadata;
	const bool applied = plan && result_code == 0;
	std::vector<uint8_t> encoded_plan;
	economic_digest intent_digest = {}, plan_digest = {};
	require((plan != nullptr) == (result_code == 0), EINVAL);
	require(economic_intent_digest(intent, &intent_digest) == economic_accounting_error::ok,
		EILSEQ);
	if (applied)
	{
		require(economic_plan_encode(*plan, &encoded_plan) ==
					economic_accounting_error::ok &&
				economic_plan_digest(*plan, &plan_digest) ==
					economic_accounting_error::ok,
			EILSEQ);
	}
	std::array<uint8_t, ECONOMIC_SOURCE_EVENT_BYTES> source_event = {};
	std::string source_event_sql = "NULL";
	if (meta.source_event)
	{
		require(economic_source_event_encode(*meta.source_event, &source_event) ==
				economic_accounting_error::ok,
			EILSEQ);
		source_event_sql = hex(source_event);
	}
	const std::string sql =
		"INSERT INTO economic_accounting_operation(operation_id,lineage,epoch,"
		"original_operation_id,accounting_version,writer_id,policy_version,compiler_version,"
		"actor_kind,actor_id,reason,source_event,realized_price_copper,intent_digest,domain_digest,plan_digest,"
		"canonical_intent,canonical_plan,outcome,result_code,account_count,posting_count,"
		"child_count,item_event_count,before_witness_count,after_witness_count) VALUES(" +
		id(command.operation_id) + "," + id(meta.lineage) + "," + id(meta.epoch) + "," +
		(critical_operation_id_is_zero(meta.original_operation_id) ?
			 "NULL" :
			 id(meta.original_operation_id)) +
		"," + std::to_string(meta.version) + "," + std::to_string(meta.writer_id) + "," +
		std::to_string(meta.policy_version) + "," + std::to_string(meta.compiler_version) +
		"," + std::to_string(static_cast<uint8_t>(meta.actor_kind)) + "," +
		std::to_string(meta.actor_id) + "," +
		std::to_string(static_cast<uint16_t>(meta.reason)) + "," + source_event_sql + "," +
		(applied && (meta.reason == economic_reason::shop_buy ||
			     meta.reason == economic_reason::shop_sell) ?
			 std::to_string(payload.price) :
			 "NULL") +
		"," + hex(intent_digest) + "," + hex(intent.domain_digest) + "," +
		(applied ? hex(plan_digest) : "NULL") + "," + hex(command.accounting_intent) + "," +
		(applied ? hex(encoded_plan) : "NULL") + "," +
		(applied ? "1,0," : "2," + std::to_string(result_code) + ",") +
		std::to_string(applied ? plan->accounts.size() : 0) + "," +
		std::to_string(applied ? plan->postings.size() : 0) + "," +
		std::to_string(applied ? plan->children.size() : 0) + "," +
		std::to_string(applied ? plan->item_events.size() : 0) + "," +
		std::to_string(applied ? plan->items_before.size() : 0) + "," +
		std::to_string(applied ? plan->items_after.size() : 0) + ")";
	execute(connection, sql);
}

void insert_source_claim(MYSQL *connection, const economic_operation_metadata &meta)
{
	if (!meta.source_event)
		return;
	std::array<uint8_t, ECONOMIC_SOURCE_EVENT_BYTES> encoded = {};
	require(economic_source_event_encode(*meta.source_event, &encoded) ==
			economic_accounting_error::ok,
		EILSEQ);
	execute(connection,
		"INSERT INTO economic_accounting_source_claim(lineage,source_event,operation_id,outcome) "
		"VALUES(" +
			id(meta.lineage) + "," + hex(encoded) + "," + id(meta.operation_id) +
			",1)");
}

void insert_plan_rows(MYSQL *connection, const critical_operation_id &operation,
		      const economic_accounting_plan &plan)
{
	for (size_t index = 0; index < plan.accounts.size(); ++index)
	{
		std::array<uint8_t, ECONOMIC_ACCOUNT_KEY_BYTES> encoded = {};
		const auto &effect = plan.accounts[index];
		require(economic_account_key_encode(effect.key, &encoded) ==
				economic_accounting_error::ok,
			EILSEQ);
		std::string sql =
			"INSERT INTO economic_accounting_account_effect(operation_id,account_index,account_key,"
			"before_copper,before_silver,before_gold,before_platinum,after_copper,after_silver,"
			"after_gold,after_platinum,before_revision,after_revision) VALUES(" +
			id(operation) + "," + std::to_string(index) + "," + hex(encoded);
		for (int64_t amount : effect.before)
			sql += "," + std::to_string(amount);
		for (int64_t amount : effect.after)
			sql += "," + std::to_string(amount);
		sql += "," + std::to_string(effect.before_revision) + "," +
		       std::to_string(effect.after_revision) + ")";
		execute(connection, sql);
	}
	for (size_t index = 0; index < plan.postings.size(); ++index)
	{
		const auto &posting = plan.postings[index];
		std::string sql =
			"INSERT INTO economic_accounting_coin_posting(operation_id,line_index,event_index,"
			"account_index,child_index,delta_copper,delta_silver,delta_gold,delta_platinum,"
			"copper_value) VALUES(" +
			id(operation) + "," + std::to_string(index) + "," +
			std::to_string(posting.event_index) + "," +
			std::to_string(posting.account_index) + "," +
			std::to_string(posting.child_index);
		for (int64_t amount : posting.delta)
			sql += "," + std::to_string(amount);
		sql += "," + std::to_string(posting.copper) + ")";
		execute(connection, sql);
	}
	for (const auto &event : plan.item_events)
	{
		require(event.event_index < UINT16_MAX, E2BIG);
		economic_accounting_item_reference reference = {};
		reference.operation_id = operation;
		reference.line_index = static_cast<uint16_t>(event.event_index);
		reference.event_index = event.event_index;
		reference.child_index = event.child_index;
		reference.item_uid = event.uid;
		reference.before_revision = event.before.revision;
		reference.after_revision = event.after.revision;
		reference.legacy_operation_id = operation;
		reference.legacy_event_index = static_cast<uint16_t>(event.event_index);
		require(economic_accounting_item_reference_insert(connection, reference),
			mysql_errno(connection) ? mysql_errno(connection) : EILSEQ);
	}
}

void physical_items(MYSQL *connection, const shop_trade_payload &payload, uint32_t keeper_id,
		    const shop_trade_accounting_authority &before)
{
	const native_domain player{ "player_items", "pid", payload.player_pid };
	const native_domain shop{ "shopkeeper_items", "shopkeeper_id", keeper_id };
	const bool produced = payload.action == shop_trade_action::buy_produced;
	if (produced)
	{
		for (size_t index = 0; index < payload.item_count; ++index)
		{
			const uint64_t uid = payload.items[index].item_uid;
			require(!native_count(connection, player, uid) &&
					!native_count(connection, shop, uid),
				ESTALE);
		}
		const auto stock =
			native_row(connection, shop, payload.stock_item_uid, payload.stock_vnum);
		require(stock.parent_row_id == 0, ESTALE);
		std::vector<native_item> ancestors;
		uint64_t uid = payload.target_parent_item_uid;
		while (uid)
		{
			const auto found =
				std::find_if(before.items_before.begin(), before.items_before.end(),
					     [uid](const auto &entry) { return entry.uid == uid; });
			require(found != before.items_before.end(), ESTALE);
			const size_t index =
				static_cast<size_t>(found - before.items_before.begin());
			ancestors.push_back(native_row(connection, player, uid,
						       before.item_vnums_before[index]));
			uid = found->position.parent_uid;
		}
		for (size_t index = 0; index < ancestors.size(); ++index)
			require(ancestors[index].parent_row_id ==
					(index + 1 < ancestors.size() ?
						 ancestors[index + 1].row_id :
						 0),
				ESTALE);
		return;
	}
	const bool from_shop = payload.action == shop_trade_action::buy_existing ||
			       payload.action == shop_trade_action::discard_invalid;
	const native_domain &source = from_shop ? shop : player;
	const native_domain &other = from_shop ? player : shop;
	std::vector<native_item> rows;
	rows.reserve(payload.item_count);
	for (size_t index = 0; index < payload.item_count; ++index)
	{
		const auto &entry = payload.items[index];
		rows.push_back(native_row(connection, source, entry.item_uid, entry.vnum));
		require(!native_count(connection, other, entry.item_uid), ESTALE);
	}
	for (size_t index = 0; index < payload.item_count; ++index)
	{
		const auto &entry = payload.items[index];
		uint64_t parent_row_id = 0;
		if (entry.parent_item_uid)
		{
			const auto found =
				std::find_if(rows.begin(), rows.end(), [&](const auto &row)
					     { return row.uid == entry.parent_item_uid; });
			require(found != rows.end(), ESTALE);
			parent_row_id = found->row_id;
		}
		require(rows[index].parent_row_id == parent_row_id, ESTALE);
		const size_t children = std::count_if(
			payload.items.begin(), payload.items.begin() + payload.item_count,
			[&](const auto &child) { return child.parent_item_uid == entry.item_uid; });
		const auto count = one(connection,
				       "SELECT COUNT(*) FROM " + std::string(source.table) +
					       " WHERE container_id=" +
					       std::to_string(rows[index].row_id) + " FOR UPDATE",
				       1);
		require(number<uint64_t>(count[0]) == children, EMSGSIZE);
	}
}

std::vector<size_t> item_order(const shop_trade_payload &payload)
{
	std::vector<size_t> order;
	std::vector<bool> added(payload.item_count, false);
	order.reserve(payload.item_count);
	while (order.size() < payload.item_count)
	{
		bool progress = false;
		for (size_t index = 0; index < payload.item_count; ++index)
		{
			if (added[index])
				continue;
			const auto &entry = payload.items[index];
			if (!entry.parent_item_uid)
			{
				order.push_back(index);
				added[index] = true;
				progress = true;
				continue;
			}
			const auto parent = std::find_if(
				payload.items.begin(), payload.items.begin() + payload.item_count,
				[&](const auto &candidate)
				{ return candidate.item_uid == entry.parent_item_uid; });
			require(parent != payload.items.begin() + payload.item_count, EILSEQ);
			const size_t parent_index =
				static_cast<size_t>(parent - payload.items.begin());
			if (added[parent_index])
			{
				order.push_back(index);
				added[index] = true;
				progress = true;
			}
		}
		require(progress, EILSEQ);
	}
	return order;
}

void move_existing_tree(MYSQL *connection, const shop_trade_payload &payload, uint32_t keeper_id)
{
	const native_domain player{ "player_items", "pid", payload.player_pid };
	const native_domain shop{ "shopkeeper_items", "shopkeeper_id", keeper_id };
	const bool from_shop = payload.action == shop_trade_action::buy_existing ||
			       payload.action == shop_trade_action::discard_invalid;
	const native_domain &source = from_shop ? shop : player;
	const native_domain &destination = from_shop ? player : shop;
	const std::string source_affects = from_shop ? "shopkeeper_item_affects" :
						       "player_item_affects";
	const std::string destination_affects = from_shop ? "player_item_affects" :
							    "shopkeeper_item_affects";
	const std::string source_descriptions = from_shop ? "shopkeeper_item_extra_descr" :
							    "player_item_extra_descr";
	const std::string destination_descriptions = from_shop ? "player_item_extra_descr" :
								 "shopkeeper_item_extra_descr";

	std::vector<native_item> old_rows(payload.item_count);
	for (size_t index = 0; index < payload.item_count; ++index)
		old_rows[index] = native_row(connection, source, payload.items[index].item_uid,
					     payload.items[index].vnum);
	if (payload.action == shop_trade_action::sell_destroy ||
	    payload.action == shop_trade_action::discard_invalid)
	{
		const auto root = std::find_if(old_rows.begin(), old_rows.end(),
					       [&](const auto &entry)
					       { return entry.uid == payload.selected_item_uid; });
		require(root != old_rows.end(), ESTALE);
		execute(connection, "DELETE FROM " + std::string(source.table) + " WHERE " +
					    source.owner_column + "=" +
					    std::to_string(source.owner_id) +
					    " AND id=" + std::to_string(root->row_id) +
					    " AND obj_uid=" + std::to_string(root->uid));
		require(mysql_affected_rows(connection) == 1, ESTALE);
		return;
	}
	const auto order = item_order(payload);
	std::vector<uint64_t> new_row_ids(payload.item_count, 0);
	constexpr const char *shared_columns =
		"vnum,equip_slot,container_id,quantity,weight,cost,timer,extra_flags,wear_flags,"
		"item_type,value0,value1,value2,value3,value4,value5,value6,value7,name,short_descr,"
		"description,action_descr,bitvector1,bitvector2,bitvector3,bitvector4,bitvector5,"
		"obj_uid,item_condition,item_material,item_properties";
	for (size_t index : order)
	{
		const auto &entry = payload.items[index];
		uint64_t parent_row = 0;
		if (entry.parent_item_uid)
		{
			const auto parent = std::find_if(
				payload.items.begin(), payload.items.begin() + payload.item_count,
				[&](const auto &candidate)
				{ return candidate.item_uid == entry.parent_item_uid; });
			require(parent != payload.items.begin() + payload.item_count, EILSEQ);
			parent_row =
				new_row_ids[static_cast<size_t>(parent - payload.items.begin())];
		}
		require(!entry.parent_item_uid || parent_row, EILSEQ);
		const uint64_t source_row = old_rows[index].row_id;
		const std::string sql =
			"INSERT INTO " + std::string(destination.table) + "(" +
			destination.owner_column + "," + shared_columns + ") SELECT " +
			std::to_string(destination.owner_id) + ",vnum,equip_slot," +
			(parent_row ? std::to_string(parent_row) : "NULL") +
			",quantity,weight,cost,timer,extra_flags,wear_flags,item_type,value0,value1,value2,"
			"value3,value4,value5,value6,value7,name,short_descr,description,action_descr,"
			"bitvector1,bitvector2,bitvector3,bitvector4,bitvector5,obj_uid,item_condition,"
			"item_material,item_properties FROM " +
			std::string(source.table) + " WHERE " + source.owner_column + "=" +
			std::to_string(source.owner_id) + " AND id=" + std::to_string(source_row) +
			" AND obj_uid=" + std::to_string(entry.item_uid);
		execute(connection, sql);
		require(mysql_affected_rows(connection) == 1, ESTALE);
		new_row_ids[index] = mysql_insert_id(connection);
		require(new_row_ids[index], EIO);
		execute(connection, "INSERT INTO " + destination_affects +
					    "(item_id,location,modifier) SELECT " +
					    std::to_string(new_row_ids[index]) +
					    ",location,modifier FROM " + source_affects +
					    " WHERE item_id=" + std::to_string(source_row));
		execute(connection, "INSERT INTO " + destination_descriptions +
					    "(item_id,keyword,description) SELECT " +
					    std::to_string(new_row_ids[index]) +
					    ",keyword,description FROM " + source_descriptions +
					    " WHERE item_id=" + std::to_string(source_row));
	}
	const auto root = std::find_if(old_rows.begin(), old_rows.end(), [&](const auto &entry)
				       { return entry.uid == payload.selected_item_uid; });
	require(root != old_rows.end(), ESTALE);
	execute(connection, "DELETE FROM " + std::string(source.table) + " WHERE " +
				    source.owner_column + "=" + std::to_string(source.owner_id) +
				    " AND id=" + std::to_string(root->row_id) +
				    " AND obj_uid=" + std::to_string(root->uid));
	require(mysql_affected_rows(connection) == 1, ESTALE);
}

std::string snapshot_string(MYSQL *connection, const player_item_snapshot &snapshot, uint8_t mask,
			    const std::string &value)
{
	return snapshot.string_mask & mask ? quoted(connection, value.data(), value.size()) :
					     "NULL";
}

void insert_produced_items(MYSQL *connection, const shop_trade_payload &payload)
{
	std::vector<player_item_snapshot> snapshots;
	require(player_item_snapshot_list_decode(payload.item_blob.data(), payload.item_blob_size,
						 &snapshots) == player_snapshot_codec_result::ok,
		EILSEQ);
	std::vector<uint64_t> row_ids(snapshots.size(), 0);
	for (size_t index = 0; index < snapshots.size(); ++index)
	{
		const auto &row = snapshots[index];
		std::string container = "NULL";
		if (row.parent_index >= 0)
		{
			require(static_cast<size_t>(row.parent_index) < index, EILSEQ);
			container = std::to_string(row_ids[static_cast<size_t>(row.parent_index)]);
		}
		else if (payload.target_parent_item_uid)
		{
			const auto parent = item(connection, payload.target_parent_item_uid);
			require(parent.exists, ESTALE);
			container = std::to_string(
				native_row(connection,
					   { "player_items", "pid", payload.player_pid },
					   payload.target_parent_item_uid, parent.vnum)
					.row_id);
		}
		std::string properties;
		require(player_item_properties_encode(row.extra2_flags, row.dynamic_affects,
						      &properties) ==
				player_snapshot_codec_result::ok,
			EILSEQ);
		std::string sql =
			"INSERT INTO player_items(pid,vnum,equip_slot,container_id,quantity,weight,cost,timer,"
			"extra_flags,wear_flags,item_type,value0,value1,value2,value3,value4,value5,value6,"
			"value7,name,short_descr,description,action_descr,bitvector1,bitvector2,bitvector3,"
			"bitvector4,bitvector5,item_material,obj_uid,item_condition,item_properties) VALUES(" +
			std::to_string(payload.player_pid) + "," + std::to_string(row.vnum) + "," +
			std::to_string(row.equipment_slot) + "," + container + ",1," +
			std::to_string(row.weight) + "," + std::to_string(row.cost) + "," +
			std::to_string(row.timers[0]) + "," + std::to_string(row.extra_flags) +
			"," + std::to_string(row.wear_flags) + "," + std::to_string(row.type);
		for (int32_t value : row.values)
			sql += "," + std::to_string(value);
		sql += "," + snapshot_string(connection, row, 1, row.name) + "," +
		       snapshot_string(connection, row, 4, row.short_description) + "," +
		       snapshot_string(connection, row, 2, row.description) + "," +
		       snapshot_string(connection, row, 8, row.action_description);
		for (uint64_t bitvector : row.bitvectors)
			sql += "," + std::to_string(bitvector);
		sql += "," + std::to_string(row.material) + "," + std::to_string(row.object_uid) +
		       "," + std::to_string(row.condition) + "," +
		       quoted(connection, properties.data(), properties.size()) + ")";
		execute(connection, sql);
		row_ids[index] = mysql_insert_id(connection);
		require(row_ids[index], EIO);
		std::set<std::pair<int16_t, int16_t>> affects;
		for (const auto &affect : row.affects)
			if ((affect[0] || affect[1]) &&
			    affects.emplace(affect[0], affect[1]).second)
				execute(connection,
					"INSERT INTO player_item_affects(item_id,location,modifier) VALUES(" +
						std::to_string(row_ids[index]) + "," +
						std::to_string(affect[0]) + "," +
						std::to_string(affect[1]) + ")");
		for (const auto &description : row.extra_descriptions)
		{
			require(!description.keyword.empty(), EILSEQ);
			execute(connection,
				"INSERT INTO player_item_extra_descr(item_id,keyword,description) VALUES(" +
					std::to_string(row_ids[index]) + "," +
					quoted(connection, description.keyword.data(),
					       description.keyword.size()) +
					"," +
					quoted(connection, description.description.data(),
					       description.description.size()) +
					")");
		}
	}
}

uint64_t revision_for_owner(const shop_trade_payload &payload,
			    const shop_trade_accounting_authority &before,
			    const item_owner_identity &owner)
{
	if (item_owner_identity_equal(owner, primary_owner(payload)))
		return before.player_owner_revision_before;
	require(item_owner_identity_equal(owner, counterparty_owner(payload)), EILSEQ);
	return before.counterparty_owner_revision_before;
}

uint16_t item_reason(const shop_trade_payload &payload)
{
	switch (payload.action)
	{
	case shop_trade_action::buy_existing:
		return SHOP_ITEM_REASON_BUY;
	case shop_trade_action::buy_produced:
		return SHOP_ITEM_REASON_CREATE;
	case shop_trade_action::sell_store:
		return SHOP_ITEM_REASON_SELL;
	case shop_trade_action::sell_destroy:
	case shop_trade_action::discard_invalid:
		return SHOP_ITEM_REASON_DESTROY;
	default:
		throw failure{ EINVAL };
	}
}

void apply_item_events(MYSQL *connection, const critical_command &command,
		       const shop_trade_payload &payload,
		       const shop_trade_accounting_authority &before,
		       const shop_trade_result &result, const economic_accounting_plan &plan)
{
	std::vector<const economic_item_event *> ordered;
	ordered.reserve(plan.item_events.size());
	if (payload.action == shop_trade_action::buy_produced)
	{
		for (size_t index : item_order(payload))
		{
			const uint64_t uid = payload.items[index].item_uid;
			const auto event = std::find_if(plan.item_events.begin(),
							plan.item_events.end(),
							[uid](const auto &candidate)
							{ return candidate.uid == uid; });
			require(event != plan.item_events.end(), EILSEQ);
			ordered.push_back(&*event);
		}
	}
	else
		for (const auto &event : plan.item_events)
			ordered.push_back(&event);
	for (const auto *event_ptr : ordered)
	{
		const auto &event = *event_ptr;
		item_owner_identity from = event.before.owner;
		if (payload.action == shop_trade_action::buy_produced)
			from = { item_owner_type::system, 0, 0 };
		const auto &to = event.after.owner;
		if (event.before.state == item_custody_state::absent)
		{
			execute(connection,
				"INSERT INTO item_current_owner(item_uid,root_item_uid,parent_item_uid,"
				"owner_type,owner_id,owner_context_id,item_revision,vnum,state,coin_payload,"
				"equipment_slot) VALUES(" +
					std::to_string(event.uid) + "," +
					std::to_string(event.after.root_uid) + "," +
					(event.after.parent_uid ?
						 std::to_string(event.after.parent_uid) :
						 "NULL") +
					"," + std::to_string(static_cast<uint8_t>(to.type)) + "," +
					std::to_string(to.id) + "," +
					std::to_string(to.context_id) + "," +
					std::to_string(event.after.revision) + "," +
					std::to_string(payload.items[event.event_index].vnum) +
					"," +
					std::to_string(static_cast<uint8_t>(event.after.state)) +
					",NULL,0)");
		}
		else
		{
			execute(connection,
				"UPDATE item_current_owner SET root_item_uid=" +
					std::to_string(event.after.root_uid) + ",parent_item_uid=" +
					(event.after.parent_uid ?
						 std::to_string(event.after.parent_uid) :
						 "NULL") +
					",owner_type=" +
					std::to_string(static_cast<uint8_t>(to.type)) +
					",owner_id=" + std::to_string(to.id) +
					",owner_context_id=" + std::to_string(to.context_id) +
					",item_revision=" + std::to_string(event.after.revision) +
					",state=" +
					std::to_string(static_cast<uint8_t>(event.after.state)) +
					",equipment_slot=0 WHERE item_uid=" +
					std::to_string(event.uid) + " AND item_revision=" +
					std::to_string(event.before.revision) + " AND owner_type=" +
					std::to_string(
						static_cast<uint8_t>(event.before.owner.type)) +
					" AND owner_id=" + std::to_string(event.before.owner.id) +
					" AND owner_context_id=" +
					std::to_string(event.before.owner.context_id));
			require(mysql_affected_rows(connection) == 1, ESTALE);
		}
	}
	for (const auto *event_ptr : ordered)
	{
		const auto &event = *event_ptr;
		item_owner_identity from = event.before.owner;
		if (payload.action == shop_trade_action::buy_produced)
			from = { item_owner_type::system, 0, 0 };
		const auto &to = event.after.owner;
		const uint64_t from_revision = revision_for_owner(payload, before, from);
		const uint64_t to_revision = revision_for_owner(payload, before, to);
		std::string ledger =
			"INSERT INTO item_ownership_ledger(operation_id,event_index,item_uid,root_item_uid,"
			"parent_item_uid,from_owner_type,from_owner_id,from_owner_context_id,to_owner_type,"
			"to_owner_id,to_owner_context_id,item_revision,from_owner_revision,to_owner_revision,"
			"reason_type,reason_id,source_site,from_equipment_slot,to_equipment_slot) VALUES(" +
			id(command.operation_id) + "," + std::to_string(event.event_index) + "," +
			std::to_string(event.uid) + "," + std::to_string(event.after.root_uid) +
			"," +
			(event.after.parent_uid ? std::to_string(event.after.parent_uid) : "NULL") +
			"," + std::to_string(static_cast<uint8_t>(from.type)) + "," +
			std::to_string(from.id) + "," + std::to_string(from.context_id) + "," +
			std::to_string(static_cast<uint8_t>(to.type)) + "," +
			std::to_string(to.id) + "," + std::to_string(to.context_id) + "," +
			std::to_string(event.after.revision) + "," + std::to_string(from_revision) +
			"," + std::to_string(to_revision + 1) + "," +
			std::to_string(item_reason(payload)) + "," +
			std::to_string(payload.shop_id) + "," +
			std::to_string(static_cast<uint16_t>(command.source_site)) + ",0,0)";
		execute(connection, ledger);
		const size_t result_index = static_cast<size_t>(event.event_index);
		require(result_index < result.item_count &&
				result.item_uids[result_index] == event.uid &&
				result.item_revisions[result_index] == event.after.revision,
			EILSEQ);
	}
}

void advance_owner_revisions(MYSQL *connection, const shop_trade_payload &payload,
			     const shop_trade_accounting_authority &before)
{
	const auto primary = primary_owner(payload);
	const auto counterparty = counterparty_owner(payload);
	auto update = [&](const item_owner_identity &owner, uint64_t revision)
	{
		execute(connection,
			"UPDATE item_owner_revision SET revision=revision+1 WHERE owner_type=" +
				std::to_string(static_cast<uint8_t>(owner.type)) +
				" AND owner_id=" + std::to_string(owner.id) +
				" AND owner_context_id=" + std::to_string(owner.context_id) +
				" AND revision=" + std::to_string(revision));
		require(mysql_affected_rows(connection) == 1, ESTALE);
	};
	update(primary, before.player_owner_revision_before);
	update(counterparty, before.counterparty_owner_revision_before);
}

economic_coin_vector canonical_wallet(int64_t copper)
{
	economic_coin_vector result = {};
	constexpr int64_t values[] = { 1, 10, 100, 1000 };
	for (size_t index = result.size(); index-- > 0;)
	{
		result[index] = copper / values[index];
		copper %= values[index];
	}
	return result;
}

void fill_result(const shop_trade_payload &payload, const shop_trade_accounting_authority &before,
		 shop_trade_result *result)
{
	result->action = payload.action;
	result->wallet.amount = before.balances_before.wallet.amount;
	result->bank.amount = before.balances_before.bank.amount;
	result->wallet_revision = before.balances_before.wallet_revision;
	result->bank_revision = before.balances_before.bank_revision;
	result->keeper_cash = before.keeper_cash_before;
	result->keeper_cash_recorded = true;
	result->shop_revision = before.shop_revision_before + 1;
	result->player_owner_revision = before.player_owner_revision_before + 1;
	result->counterparty_owner_revision = before.counterparty_owner_revision_before + 1;
	result->item_count = payload.item_count;
	const bool charged = payload.action != shop_trade_action::discard_invalid;
	if (charged)
	{
		int64_t wallet_copper = 0;
		require(economic_coin_value(before.balances_before.wallet.amount, &wallet_copper) ==
				economic_accounting_error::ok,
			ERANGE);
		const bool buying = payload.action == shop_trade_action::buy_existing ||
				    payload.action == shop_trade_action::buy_produced;
		if (buying)
		{
			if (wallet_copper >= payload.price)
			{
				wallet_copper -= payload.price;
				result->wallet.amount = canonical_wallet(wallet_copper);
			}
			result->keeper_cash += payload.price;
		}
		else
		{
			if (wallet_copper <= INT64_MAX - payload.price)
			{
				wallet_copper += payload.price;
				result->wallet.amount = canonical_wallet(wallet_copper);
			}
			if (result->keeper_cash >= payload.price)
				result->keeper_cash -= payload.price;
			else if (before.keeper_roaming && before.keeper_vnum == 11005)
				result->keeper_cash = before.keeper_cash_before;
		}
		++result->wallet_revision;
		++result->bank_revision;
	}
	for (size_t index = 0; index < payload.item_count; ++index)
	{
		result->item_uids[index] = payload.items[index].item_uid;
		result->item_revisions[index] =
			payload.action == shop_trade_action::buy_produced ?
				1 :
				payload.items[index].expected_item_revision + 1;
	}
}

void apply_native_trade(MYSQL *connection, const shop_trade_payload &payload, uint32_t bank_id,
			uint32_t keeper_id, const shop_trade_accounting_authority &before,
			const shop_trade_result &result)
{
	if (payload.action == shop_trade_action::buy_produced)
		insert_produced_items(connection, payload);
	else
		move_existing_tree(connection, payload, keeper_id);
	if (payload.action != shop_trade_action::discard_invalid)
	{
		execute(connection,
			"UPDATE player_data SET copper=" + std::to_string(result.wallet.amount[0]) +
				",silver=" + std::to_string(result.wallet.amount[1]) +
				",gold=" + std::to_string(result.wallet.amount[2]) +
				",platinum=" + std::to_string(result.wallet.amount[3]) +
				",wallet_revision=" + std::to_string(result.wallet_revision) +
				" WHERE pid=" + std::to_string(payload.player_pid) +
				" AND wallet_revision=" +
				std::to_string(before.balances_before.wallet_revision));
		require(mysql_affected_rows(connection) == 1, ESTALE);
		execute(connection, "UPDATE account_banks SET bank_revision=" +
					    std::to_string(result.bank_revision) + " WHERE id=" +
					    std::to_string(bank_id) + " AND bank_revision=" +
					    std::to_string(before.balances_before.bank_revision));
		require(mysql_affected_rows(connection) == 1, ESTALE);
	}
	execute(connection,
		"UPDATE shopkeepers SET cash=" + std::to_string(result.keeper_cash) +
			",shop_revision=" + std::to_string(result.shop_revision) +
			" WHERE id=" + std::to_string(keeper_id) +
			" AND shop_revision=" + std::to_string(before.shop_revision_before));
	require(mysql_affected_rows(connection) == 1, ESTALE);
}

void count_is(MYSQL *connection, const std::string &table, const std::string &where,
	      uint64_t expected)
{
	const auto row = one(connection, "SELECT COUNT(*) FROM " + table + " WHERE " + where, 1);
	require(number<uint64_t>(row[0]) == expected, EILSEQ);
}

std::string effect_where(const critical_operation_id &operation, size_t index,
			 const economic_account_effect &effect)
{
	std::array<uint8_t, ECONOMIC_ACCOUNT_KEY_BYTES> key = {};
	require(economic_account_key_encode(effect.key, &key) == economic_accounting_error::ok,
		EILSEQ);
	std::string where = "operation_id=" + id(operation) +
			    " AND account_index=" + std::to_string(index) +
			    " AND account_key=" + hex(key);
	static constexpr const char *names[] = { "copper", "silver", "gold", "platinum" };
	for (size_t coin = 0; coin < 4; ++coin)
		where += " AND before_" + std::string(names[coin]) + "=" +
			 std::to_string(effect.before[coin]) + " AND after_" + names[coin] + "=" +
			 std::to_string(effect.after[coin]);
	return where + " AND before_revision=" + std::to_string(effect.before_revision) +
	       " AND after_revision=" + std::to_string(effect.after_revision);
}

std::string posting_where(const critical_operation_id &operation, size_t index,
			  const economic_coin_posting &posting)
{
	std::string where = "operation_id=" + id(operation) +
			    " AND line_index=" + std::to_string(index) +
			    " AND event_index=" + std::to_string(posting.event_index) +
			    " AND account_index=" + std::to_string(posting.account_index) +
			    " AND child_index=" + std::to_string(posting.child_index);
	static constexpr const char *names[] = { "copper", "silver", "gold", "platinum" };
	for (size_t coin = 0; coin < 4; ++coin)
		where += " AND delta_" + std::string(names[coin]) + "=" +
			 std::to_string(posting.delta[coin]);
	return where + " AND copper_value=" + std::to_string(posting.copper);
}

std::string ledger_where(const critical_command &command, const shop_trade_payload &payload,
			 const shop_trade_accounting_authority &authority,
			 const economic_item_event &event)
{
	item_owner_identity from = event.before.owner;
	if (payload.action == shop_trade_action::buy_produced)
		from = { item_owner_type::system, 0, 0 };
	const auto &to = event.after.owner;
	const uint64_t from_revision = revision_for_owner(payload, authority, from);
	const uint64_t to_revision = revision_for_owner(payload, authority, to) + 1;
	const uint16_t reason = item_reason(payload);
	return "operation_id=" + id(command.operation_id) +
	       " AND event_index=" + std::to_string(event.event_index) +
	       " AND item_uid=" + std::to_string(event.uid) +
	       " AND root_item_uid=" + std::to_string(event.after.root_uid) +
	       (event.after.parent_uid ?
			" AND parent_item_uid=" + std::to_string(event.after.parent_uid) :
			" AND parent_item_uid IS NULL") +
	       " AND from_owner_type=" + std::to_string(static_cast<uint8_t>(from.type)) +
	       " AND from_owner_id=" + std::to_string(from.id) +
	       " AND from_owner_context_id=" + std::to_string(from.context_id) +
	       " AND to_owner_type=" + std::to_string(static_cast<uint8_t>(to.type)) +
	       " AND to_owner_id=" + std::to_string(to.id) +
	       " AND to_owner_context_id=" + std::to_string(to.context_id) +
	       " AND item_revision=" + std::to_string(event.after.revision) +
	       " AND from_owner_revision=" + std::to_string(from_revision) +
	       " AND to_owner_revision=" + std::to_string(to_revision) +
	       " AND reason_type=" + std::to_string(reason) +
	       " AND reason_id=" + std::to_string(payload.shop_id) +
	       " AND source_site=" + std::to_string(static_cast<uint16_t>(command.source_site)) +
	       " AND from_equipment_slot=0 AND to_equipment_slot=0";
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
		produced_items(payload);
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
		physical_items(connection, payload, candidate.keeper_id, before);
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

unsigned int
economic_sql_shop_trade_execute_and_record(MYSQL *connection, const critical_command &command,
					   const economic_sql_shop_trade_context &context,
					   shop_trade_result *result, unsigned int *result_code,
					   bool *mutation_applied)
{
#ifdef __NO_MYSQL__
	(void)connection;
	(void)command;
	(void)context;
	(void)result;
	(void)result_code;
	(void)mutation_applied;
	return ENOTSUP;
#else
	if (!connection || !result || !result_code || !mutation_applied ||
	    !(connection->server_status & SERVER_STATUS_IN_TRANS) || !context.session_id ||
	    mysql_thread_id(connection) != context.session_id)
		return EINVAL;
	try
	{
		economic_sql_shop_trade_context active;
		const auto lock_error = economic_sql_shop_trade_lock(connection, command, &active);
		if (lock_error)
			return lock_error;
		if (active.authority.lineage.bytes != context.authority.lineage.bytes ||
		    active.authority.epoch.bytes != context.authority.epoch.bytes ||
		    active.authority.lineage_revision != context.authority.lineage_revision ||
		    active.bank_id != context.bank_id || active.keeper_id != context.keeper_id ||
		    active.authority.mappings.size() != context.authority.mappings.size())
			return ESTALE;
		for (size_t index = 0; index < active.authority.mappings.size(); ++index)
			if (active.authority.mappings[index].revision !=
			    context.authority.mappings[index].revision)
				return ESTALE;

		economic_frozen_intent intent;
		shop_trade_payload payload = {};
		economic_account_key wallet, bank, keeper_account;
		if (shop_trade_accounting_decode(command, &intent, &payload, &wallet, &bank,
						 &keeper_account) !=
			    economic_accounting_error::ok ||
		    intent.admission.metadata.lineage.bytes != active.authority.lineage.bytes ||
		    intent.admission.metadata.epoch.bytes != active.authority.epoch.bytes)
			return EILSEQ;
		shop_trade_result candidate = {};
		fill_result(payload, active.before, &candidate);
		economic_accounting_plan plan;
		const auto status = shop_trade_accounting_plan(command, intent, active.before,
							       candidate, &plan);
		if (status != economic_accounting_error::ok)
		{
			unsigned int denied = EILSEQ;
			switch (status)
			{
			case economic_accounting_error::negative_holding:
				denied = ENOBUFS;
				break;
			case economic_accounting_error::overflow:
				denied = ERANGE;
				break;
			case economic_accounting_error::stale_revision:
				denied = ESTALE;
				break;
			case economic_accounting_error::unauthorized:
				denied = EACCES;
				break;
			case economic_accounting_error::capacity:
				denied = E2BIG;
				break;
			case economic_accounting_error::invalid_identity:
			case economic_accounting_error::invalid_version:
				denied = EINVAL;
				break;
			default:
				break;
			}
			shop_trade_result rejected = {};
			rejected.action = payload.action;
			rejected.wallet.amount = active.before.balances_before.wallet.amount;
			rejected.bank.amount = active.before.balances_before.bank.amount;
			rejected.wallet_revision = active.before.balances_before.wallet_revision;
			rejected.bank_revision = active.before.balances_before.bank_revision;
			insert_operation(connection, command, intent, nullptr, payload, denied);
			*result = std::move(rejected);
			*result_code = denied;
			*mutation_applied = false;
			return 0;
		}

		apply_native_trade(connection, payload, active.bank_id, active.keeper_id,
				   active.before, candidate);
		apply_item_events(connection, command, payload, active.before, candidate, plan);
		advance_owner_revisions(connection, payload, active.before);
		insert_operation(connection, command, intent, &plan, payload, 0);
		insert_source_claim(connection, intent.admission.metadata);
		insert_plan_rows(connection, command.operation_id, plan);
		if (!(connection->server_status & SERVER_STATUS_IN_TRANS) ||
		    mysql_thread_id(connection) != active.session_id)
			return ENOTCONN;
		*result = std::move(candidate);
		*result_code = 0;
		*mutation_applied = true;
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

unsigned int economic_sql_shop_trade_verify_retained(MYSQL *connection,
						     const critical_command &command,
						     unsigned int result_code,
						     const uint8_t *result_payload,
						     size_t result_size)
{
#ifdef __NO_MYSQL__
	(void)connection;
	(void)command;
	(void)result_code;
	(void)result_payload;
	(void)result_size;
	return ENOTSUP;
#else
	if (!connection || !result_payload || result_size != SHOP_TRADE_RESULT_BYTES)
		return EINVAL;
	try
	{
		economic_frozen_intent intent;
		shop_trade_payload payload = {};
		economic_account_key wallet, bank, keeper_account;
		shop_trade_result result = {};
		if (shop_trade_accounting_decode(command, &intent, &payload, &wallet, &bank,
						 &keeper_account) !=
			    economic_accounting_error::ok ||
		    !shop_trade_command_decode_result(result_payload, result_size, &result) ||
		    result.action != payload.action)
			return EILSEQ;
		const auto row = one(
			connection,
			"SELECT lineage,epoch,original_operation_id,accounting_version,writer_id,"
			"policy_version,compiler_version,actor_kind,actor_id,reason,source_event,"
			"intent_digest,domain_digest,plan_digest,canonical_intent,canonical_plan,outcome,"
			"result_code,account_count,posting_count,child_count,item_event_count,"
			"before_witness_count,after_witness_count FROM economic_accounting_operation WHERE "
			"operation_id=" +
				id(command.operation_id),
			24);
		const auto &meta = intent.admission.metadata;
		economic_digest intent_digest = {};
		require(economic_intent_digest(intent, &intent_digest) ==
				economic_accounting_error::ok,
			EILSEQ);
		std::array<uint8_t, ECONOMIC_SOURCE_EVENT_BYTES> source_event = {};
		if (meta.source_event)
			require(economic_source_event_encode(*meta.source_event, &source_event) ==
					economic_accounting_error::ok,
				EILSEQ);
		const bool original_is_null =
			critical_operation_id_is_zero(meta.original_operation_id);
		require(row[0] &&
				*row[0] == std::string(reinterpret_cast<const char *>(
							       meta.lineage.bytes.data()),
						       meta.lineage.bytes.size()) &&
				row[1] &&
				*row[1] == std::string(reinterpret_cast<const char *>(
							       meta.epoch.bytes.data()),
						       meta.epoch.bytes.size()) &&
				row[2].has_value() == !original_is_null &&
				(!row[2] ||
				 *row[2] ==
					 std::string(
						 reinterpret_cast<const char *>(
							 meta.original_operation_id.bytes.data()),
						 meta.original_operation_id.bytes.size())) &&
				number<uint16_t>(row[3]) == meta.version &&
				number<uint32_t>(row[4]) == meta.writer_id &&
				number<uint32_t>(row[5]) == meta.policy_version &&
				number<uint32_t>(row[6]) == meta.compiler_version &&
				number<uint8_t>(row[7]) == static_cast<uint8_t>(meta.actor_kind) &&
				number<uint64_t>(row[8]) == meta.actor_id &&
				number<uint16_t>(row[9]) == static_cast<uint16_t>(meta.reason) &&
				row[10].has_value() == meta.source_event.has_value() &&
				(!meta.source_event ||
				 *row[10] == std::string(reinterpret_cast<const char *>(
								 source_event.data()),
							 source_event.size())) &&
				row[11] &&
				*row[11] == std::string(reinterpret_cast<const char *>(
								intent_digest.data()),
							intent_digest.size()) &&
				row[12] &&
				*row[12] == std::string(reinterpret_cast<const char *>(
								intent.domain_digest.data()),
							intent.domain_digest.size()) &&
				row[14] &&
				*row[14] == std::string(reinterpret_cast<const char *>(
								command.accounting_intent.data()),
							command.accounting_intent.size()) &&
				number<unsigned int>(row[17]) == result_code &&
				number<uint8_t>(row[16]) == (result_code ? 2 : 1),
			EILSEQ);
		const std::string root = "operation_id=" + id(command.operation_id);
		const auto expected_claims = !result_code && meta.source_event ? uint64_t{ 1 } :
										 uint64_t{ 0 };
		count_is(connection, "economic_accounting_source_claim", root, expected_claims);
		if (expected_claims)
			count_is(connection, "economic_accounting_source_claim",
				 "lineage=" + id(meta.lineage) + " AND source_event=" +
					 hex(source_event) + " AND " + root + " AND outcome=1",
				 1);
		if (result_code)
		{
			require(!row[13] && !row[15] && number<uint16_t>(row[18]) == 0 &&
					number<uint16_t>(row[19]) == 0 &&
					number<uint16_t>(row[20]) == 0 &&
					number<uint16_t>(row[21]) == 0 &&
					number<uint16_t>(row[22]) == 0 &&
					number<uint16_t>(row[23]) == 0 &&
					!result.keeper_cash_recorded && !result.shop_revision &&
					!result.player_owner_revision &&
					!result.counterparty_owner_revision && !result.item_count &&
					result.wallet_revision ==
						payload.expected_wallet_revision &&
					result.bank_revision == payload.expected_bank_revision &&
					std::all_of(result.item_uids.begin(),
						    result.item_uids.end(),
						    [](uint64_t value) { return value == 0; }) &&
					std::all_of(result.item_revisions.begin(),
						    result.item_revisions.end(),
						    [](uint64_t value) { return value == 0; }),
				EILSEQ);
			count_is(connection, "economic_accounting_account_effect", root, 0);
			count_is(connection, "economic_accounting_coin_posting", root, 0);
			count_is(connection, "economic_accounting_item_reference", root, 0);
			count_is(connection, "item_ownership_ledger", root, 0);
			return 0;
		}

		require(row[13] && row[15] && row[13]->size() == 32 &&
				row[15]->size() <= ECONOMIC_ACCOUNTING_MAX_PLAN_BYTES,
			EILSEQ);
		economic_accounting_plan retained;
		require(economic_plan_decode(
				std::span<const uint8_t>(
					reinterpret_cast<const uint8_t *>(row[15]->data()),
					row[15]->size()),
				&retained) == economic_accounting_error::ok &&
				economic_plan_validate_structure(retained) ==
					economic_accounting_error::ok,
			EILSEQ);
		economic_digest retained_digest = {};
		require(economic_plan_digest(retained, &retained_digest) ==
					economic_accounting_error::ok &&
				*row[13] == std::string(reinterpret_cast<const char *>(
								retained_digest.data()),
							retained_digest.size()),
			EILSEQ);
		count_is(connection, "economic_accounting_account_effect", root,
			 retained.accounts.size());
		count_is(connection, "economic_accounting_coin_posting", root,
			 retained.postings.size());
		count_is(connection, "economic_accounting_item_reference", root,
			 retained.item_events.size());
		count_is(connection, "item_ownership_ledger", root, retained.item_events.size());
		for (size_t index = 0; index < retained.accounts.size(); ++index)
			count_is(connection, "economic_accounting_account_effect",
				 effect_where(command.operation_id, index,
					      retained.accounts[index]),
				 1);
		for (size_t index = 0; index < retained.postings.size(); ++index)
			count_is(connection, "economic_accounting_coin_posting",
				 posting_where(command.operation_id, index,
					       retained.postings[index]),
				 1);

		shop_trade_accounting_authority authority;
		authority.epoch = meta.epoch;
		authority.wallet_account = wallet;
		authority.bank_account = bank;
		authority.keeper_account = keeper_account;
		authority.shop_id = payload.shop_id;
		authority.keeper_vnum = payload.keeper_vnum;
		authority.keeper_cash_before = payload.expected_keeper_cash;
		authority.keeper_roaming = payload.keeper_roaming != 0;
		authority.shop_revision_before = payload.expected_shop_revision;
		require(result.player_owner_revision && result.counterparty_owner_revision, EILSEQ);
		authority.player_owner_revision_before = result.player_owner_revision - 1;
		authority.counterparty_owner_revision_before =
			result.counterparty_owner_revision - 1;
		const economic_account_effect *wallet_effect = nullptr;
		const economic_account_effect *bank_effect = nullptr;
		const economic_account_effect *keeper_effect = nullptr;
		for (const auto &effect : retained.accounts)
		{
			if (economic_account_key_equal(effect.key, wallet))
				wallet_effect = &effect;
			else if (economic_account_key_equal(effect.key, bank))
				bank_effect = &effect;
			else if (economic_account_key_equal(effect.key, keeper_account))
				keeper_effect = &effect;
		}
		require(wallet_effect && bank_effect && keeper_effect, EILSEQ);
		authority.balances_before.wallet.amount = wallet_effect->before;
		authority.balances_before.bank.amount = bank_effect->before;
		authority.balances_before.wallet_revision = wallet_effect->before_revision;
		authority.balances_before.bank_revision = bank_effect->before_revision;
		require(keeper_effect->before[0] == payload.expected_keeper_cash, EILSEQ);
		authority.items_before = retained.items_before;
		authority.item_vnums_before.reserve(retained.items_before.size());
		for (const auto &snapshot : retained.items_before)
		{
			const auto entry = std::find_if(
				payload.items.begin(), payload.items.begin() + payload.item_count,
				[&](const auto &candidate)
				{ return candidate.item_uid == snapshot.uid; });
			if (entry != payload.items.begin() + payload.item_count)
				authority.item_vnums_before.push_back(
					payload.action == shop_trade_action::buy_produced ?
						0 :
						entry->vnum);
			else if (snapshot.uid == payload.stock_item_uid)
				authority.item_vnums_before.push_back(payload.stock_vnum);
			else
				authority.item_vnums_before.push_back(0);
		}
		economic_accounting_plan expected;
		require(shop_trade_accounting_plan(command, intent, authority, result, &expected) ==
				economic_accounting_error::ok,
			EILSEQ);
		std::vector<uint8_t> expected_plan;
		require(economic_plan_encode(expected, &expected_plan) ==
					economic_accounting_error::ok &&
				expected_plan ==
					std::vector<uint8_t>(row[15]->begin(), row[15]->end()) &&
				number<uint16_t>(row[18]) == retained.accounts.size() &&
				number<uint16_t>(row[19]) == retained.postings.size() &&
				number<uint16_t>(row[20]) == retained.children.size() &&
				number<uint16_t>(row[21]) == retained.item_events.size() &&
				number<uint16_t>(row[22]) == retained.items_before.size() &&
				number<uint16_t>(row[23]) == retained.items_after.size() &&
				result.item_count == retained.item_events.size(),
			EILSEQ);
		for (const auto &event : retained.item_events)
		{
			const size_t index = static_cast<size_t>(event.event_index);
			require(index < result.item_count && result.item_uids[index] == event.uid &&
					result.item_revisions[index] == event.after.revision,
				EILSEQ);
			count_is(connection, "item_ownership_ledger",
				 ledger_where(command, payload, authority, event), 1);
			economic_accounting_item_reference reference = {};
			require(economic_accounting_item_reference_find_by_legacy(
					connection, command.operation_id,
					static_cast<uint16_t>(event.event_index), &reference) &&
					reference.operation_id.bytes ==
						command.operation_id.bytes &&
					reference.line_index == event.event_index &&
					reference.event_index == event.event_index &&
					reference.child_index == event.child_index &&
					reference.item_uid == event.uid &&
					reference.before_revision == event.before.revision &&
					reference.after_revision == event.after.revision,
				EILSEQ);
		}
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
