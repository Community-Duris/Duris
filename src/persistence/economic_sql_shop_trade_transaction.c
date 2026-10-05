#include "persistence/economic_sql_shop_trade_transaction.h"

#include "item/economic_accounting_item_reference.h"
#include "player/player_snapshot_codec.h"
#include "persistence/shop_item_runtime_payload.h"
#include "economy/shop_trade_item_payload.h"
#include "economy/shop_trade_recovery_image.h"

#include <cerrno>

#ifndef __NO_MYSQL__
#include <algorithm>
#include <charconv>
#include <climits>
#include <cstring>
#include <limits>
#include <memory>
#include <map>
#include <new>
#include <optional>
#include <set>
#include <string>
#include <strings.h>
#include <tuple>
#include <type_traits>
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
	const auto player = one(
		connection,
		"SELECT account_name,racewar,copper,silver,gold,platinum,wallet_revision,level,save_revision "
		"FROM player_data WHERE pid=" +
			std::to_string(payload.player_pid) + " FOR UPDATE",
		9);
	require(player[0] && !strcasecmp(player[0]->c_str(), payload.account_name.data()) &&
			number<uint8_t>(player[1]) == payload.racewar,
		ESTALE);
	if (payload.expected_player_level)
		require(number<uint32_t>(player[7]) == payload.expected_player_level &&
				number<uint64_t>(player[8]) ==
					payload.expected_player_save_revision,
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

uint32_t keeper(MYSQL *connection, const shop_trade_payload &payload, uint32_t keeper_id,
		shop_trade_accounting_authority *before)
{
	const bool shared = payload.expected_player_level != 0;
	const auto row = shared ?
				 one(connection,
				     "SELECT id,shop_id,mob_vnum,cash,shop_revision,keeper_roaming "
				     "FROM shopkeepers WHERE shop_id=" +
					     std::to_string(payload.shop_id) + " FOR UPDATE",
				     6) :
				 one(connection,
				     "SELECT shop_id,mob_vnum,cash,shop_revision,keeper_roaming "
				     "FROM shopkeepers WHERE id=" +
					     std::to_string(keeper_id) + " FOR UPDATE",
				     5);
	const size_t offset = shared ? 1 : 0;
	if (shared)
	{
		const auto id = number<uint64_t>(row[0]);
		require(id && id <= UINT32_MAX, ESTALE);
		keeper_id = static_cast<uint32_t>(id);
	}
	require(number<uint64_t>(row[offset]) == payload.shop_id &&
			number<int32_t>(row[offset + 1]) == payload.keeper_vnum,
		ESTALE);
	require(row[offset + 2].has_value(), ENODATA);
	const auto cash = number<int64_t>(row[offset + 2]);
	require(cash >= 0 && cash <= INT_MAX, ERANGE);
	const auto revision = number<uint64_t>(row[offset + 3]);
	require(row[offset + 4].has_value(), ENODATA);
	const auto roaming = number<uint8_t>(row[offset + 4]);
	require(roaming <= 1, ERANGE);
	require(cash == payload.expected_keeper_cash && revision == payload.expected_shop_revision,
		ESTALE);
	require(roaming == payload.keeper_roaming, ESTALE);
	before->shop_id = payload.shop_id;
	before->keeper_vnum = payload.keeper_vnum;
	before->keeper_cash_before = cash;
	before->keeper_roaming = roaming != 0;
	before->shop_revision_before = revision;
	return keeper_id;
}

struct locked_item
{
	economic_item_snapshot snapshot;
	int32_t vnum = 0;
	bool exists = false;
};

// These helpers share the publication reader's bounded, sorted original cut.
// Definitions below are also used by live retained-result publication.
void publication_session(MYSQL *, unsigned long);
void publication_absence(MYSQL *, uint64_t, const char *);
std::map<uint64_t, uint64_t> publication_keeper_routes(MYSQL *, uint32_t, bool);
std::map<uint64_t, locked_item> publication_custody(MYSQL *, const shop_trade_payload &,
						    std::span<const uint64_t>,
						    const std::map<uint64_t, uint64_t> &,
						    bool retained_only = false);
void recovery_keeper_closure(const shop_trade_payload &, const std::map<uint64_t, locked_item> &,
			     bool);
std::vector<uint64_t> recovery_custody_scope(const shop_trade_payload &,
					     const std::map<uint64_t, locked_item> &,
					     const std::map<uint64_t, uint64_t> &);
void recovery_images(MYSQL *, const shop_trade_payload &, uint32_t, unsigned long, bool);

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

player_item_snapshot native_destination_weight_before(MYSQL *connection,
						      const shop_trade_payload &payload,
						      uint64_t *row_id)
{
	require(row_id && payload.native_destination_weight_recorded &&
			payload.action == shop_trade_action::buy_produced &&
			payload.target_parent_item_uid &&
			payload.target_parent_item_uid == payload.target_root_item_uid,
		EILSEQ);
	const auto target = item(connection, payload.target_parent_item_uid);
	require(target.exists && target.snapshot.position.state == item_custody_state::active &&
			item_owner_identity_equal(target.snapshot.position.owner,
						  player_owner(payload)) &&
			!target.snapshot.position.parent_uid &&
			target.snapshot.position.root_uid == payload.target_parent_item_uid &&
			target.snapshot.position.revision ==
				payload.expected_target_parent_revision,
		ESTALE);
	const auto native = native_row(connection, { "player_items", "pid", payload.player_pid },
				       payload.target_parent_item_uid, target.vnum);
	require(!native.parent_row_id, ESTALE);
	player_item_snapshot original{};
	bool present = false;
	require(shop_item_runtime_read(connection, false, native.row_id, &original, &present),
		errno ? errno : EIO);
	require(present && original.object_uid == payload.target_parent_item_uid &&
			original.vnum == target.vnum && original.type == ITEM_CONTAINER &&
			!original.equipment_slot &&
			original.parent_index == PLAYER_SNAPSHOT_NO_PARENT,
		ENODATA);
	require(shop_item_runtime_verify(connection, false, native.row_id, payload.player_pid, 0,
					 original),
		errno ? errno : EIO);
	std::vector<player_item_snapshot> selected;
	require(shop_trade_accounted_after_items(payload, &selected) && !selected.empty() &&
			selected.front().object_uid == payload.selected_item_uid &&
			shop_trade_destination_weight_verify(original, selected.front().weight,
							     payload.destination_weight),
		EILSEQ);
	*row_id = native.row_id;
	return original;
}

void physical_items(MYSQL *connection, const shop_trade_payload &payload, uint32_t keeper_id,
		    const shop_trade_accounting_authority &before)
{
	const native_domain player{ "player_items", "pid", payload.player_pid };
	const native_domain shop{ "shopkeeper_items", "shopkeeper_id", keeper_id };
	const bool produced = payload.action == shop_trade_action::buy_produced;
	if (payload.expected_player_level)
	{
		// Validate the complete original graph for every v6 action, including
		// destruction, whose path has no destination write to validate it later.
		std::vector<player_item_snapshot> validated_after;
		require(shop_trade_accounted_after_items(payload, &validated_after), EILSEQ);
	}
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
		if (payload.expected_player_level)
		{
			// The output blob is the new staged object, not original stock.
			// Verify existing locked native stock independently; do not invent
			// an original stock literal that the command never recorded.
			player_item_snapshot stock_item{};
			bool present = false;
			require(shop_item_runtime_read(connection, true, stock.row_id, &stock_item,
						       &present),
				errno ? errno : EIO);
			require(present && stock_item.object_uid == payload.stock_item_uid &&
					stock_item.vnum == payload.stock_vnum &&
					stock_item.equipment_slot == 0,
				ENODATA);
			require(shop_item_runtime_verify(connection, true, stock.row_id, keeper_id,
							 0, stock_item),
				errno ? errno : EIO);
		}
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
		if (payload.native_destination_weight_recorded && payload.target_parent_item_uid)
		{
			uint64_t target_row = 0;
			(void)native_destination_weight_before(connection, payload, &target_row);
		}
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
	std::vector<player_item_snapshot> source_items;
	if (payload.expected_player_level)
		require(player_item_snapshot_list_decode(payload.item_blob.data(),
							 payload.item_blob_size, &source_items) ==
				player_snapshot_codec_result::ok,
			EILSEQ);
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
		if (payload.expected_player_level)
		{
			const auto snapshot = std::find_if(
				source_items.begin(), source_items.end(), [&](const auto &candidate)
				{ return candidate.object_uid == entry.item_uid; });
			require(snapshot != source_items.end(), EILSEQ);
			require(shop_item_runtime_verify(connection, from_shop, rows[index].row_id,
							 source.owner_id, parent_row_id, *snapshot),
				errno ? errno : EIO);
		}
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
	std::vector<player_item_snapshot> after_items;
	if (payload.expected_player_level)
		require(shop_trade_accounted_after_items(payload, &after_items), EILSEQ);
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
		if (payload.expected_player_level)
		{
			const auto after = std::find_if(
				after_items.begin(), after_items.end(), [&](const auto &candidate)
				{ return candidate.object_uid == entry.item_uid; });
			require(after != after_items.end(), EILSEQ);
			require(shop_item_runtime_write(connection, !from_shop, new_row_ids[index],
							*after),
				errno ? errno : EIO);
		}
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
	if (payload.expected_player_level)
		require(shop_trade_accounted_after_items(payload, &snapshots), EILSEQ);
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
		if (payload.expected_player_level)
		{
			require(shop_item_runtime_write(connection, false, row_ids[index], row),
				errno ? errno : EIO);
			// Newly produced UIDs have no legitimate source/destination overlap.
			// Reuse the exclusive native check before this transaction can commit.
			const uint64_t parent_id =
				container == "NULL" ? 0 : number<uint64_t>(cell{ container });
			require(shop_item_runtime_verify(connection, false, row_ids[index],
							 payload.player_pid, parent_id, row),
				errno ? errno : EIO);
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
	uint64_t target_row = 0;
	player_item_snapshot destination_after{};
	const bool updates_destination = payload.native_destination_weight_recorded &&
					 payload.target_parent_item_uid;
	if (updates_destination)
	{
		destination_after =
			native_destination_weight_before(connection, payload, &target_row);
		destination_after.weight = payload.destination_weight.after;
	}
	if (payload.action == shop_trade_action::buy_produced)
		insert_produced_items(connection, payload);
	else
		move_existing_tree(connection, payload, keeper_id);
	if (updates_destination)
	{
		// Same transaction/locked row: content weight is not a second custody move.
		require(shop_item_runtime_write(connection, false, target_row, destination_after),
			errno ? errno : EIO);
		require(shop_item_runtime_verify(connection, false, target_row, payload.player_pid,
						 0, destination_after),
			errno ? errno : EIO);
	}
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
		const bool shared = intent.admission.facts.size() == 16;
		candidate.bank_id = native_id(connection, bank.authority_id);
		std::vector<economic_sql_mapping_request> mappings = {
			{ wallet, PLAYER_LOCATOR, payload.player_pid },
			{ bank, BANK_LOCATOR, candidate.bank_id }
		};
		if (!shared)
		{
			candidate.keeper_id = native_id(connection, treasury.authority_id);
			mappings.push_back({ treasury, SHOPKEEPER_LOCATOR, candidate.keeper_id });
		}
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
		candidate.keeper_id = keeper(connection, payload, candidate.keeper_id, &before);
		const item_owner_identity primary = primary_owner(payload);
		const item_owner_identity other = counterparty_owner(payload);
		require(!item_owner_identity_equal(primary, other));
		if (payload.recovery_manifest_recorded)
		{
			publication_session(connection, candidate.session_id);
			require(shop_trade_recovery_manifest_shape_valid(payload.recovery_manifest),
				EILSEQ);
			std::vector<item_owner_identity> owners = { primary, other,
								    player_owner(payload),
								    shop_owner(payload) };
			std::sort(owners.begin(), owners.end(),
				  [](const auto &a, const auto &b) {
					  return std::tie(a.type, a.id, a.context_id) <
						 std::tie(b.type, b.id, b.context_id);
				  });
			owners.erase(std::unique(owners.begin(), owners.end(),
						 item_owner_identity_equal),
				     owners.end());
			for (const auto &owner : owners)
			{
				const uint64_t revision = owner_revision(connection, owner);
				if (item_owner_identity_equal(owner, primary))
					before.player_owner_revision_before = revision;
				if (item_owner_identity_equal(owner, other))
					before.counterparty_owner_revision_before = revision;
			}
			const auto routes =
				publication_keeper_routes(connection, candidate.keeper_id, false);
			// The manifest's four persisted lists and routed rows are locked before
			// selected/ancestor helpers touch physical rows. No fabricated body.
			const auto custody = publication_custody(connection, payload, {}, routes);
			recovery_keeper_closure(payload, custody, false);
			candidate.recovery_custody_uids =
				recovery_custody_scope(payload, custody, routes);
		}
		else
		{
			const bool primary_first =
				std::tie(primary.type, primary.id, primary.context_id) <
				std::tie(other.type, other.id, other.context_id);
			const auto first_revision =
				owner_revision(connection, primary_first ? primary : other);
			const auto second_revision =
				owner_revision(connection, primary_first ? other : primary);
			before.player_owner_revision_before = primary_first ? first_revision :
									      second_revision;
			before.counterparty_owner_revision_before =
				primary_first ? second_revision : first_revision;
		}
		items(connection, payload, &before);
		physical_items(connection, payload, candidate.keeper_id, before);
		if (payload.recovery_manifest_recorded)
			recovery_images(connection, payload, candidate.keeper_id,
					candidate.session_id, false);
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
		// A failed complete AFTER image is an original-transaction error. It
		// returns through the parent's rollback path, never a business denial.
		if (payload.recovery_manifest_recorded)
		{
			// Re-read only original locked/gap-locked identities, not a new
			// discovery after physical mutation. The broad BEFORE cut includes
			// references into both phases' keeper forests, including future UIDs.
			const auto custody = publication_custody(
				connection, payload, active.recovery_custody_uids, {}, true);
			recovery_keeper_closure(payload, custody, true);
			recovery_images(connection, payload, active.keeper_id, active.session_id,
					true);
		}
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
		const bool shared = intent.admission.facts.size() == 16;
		if (shared)
		{
			std::array<uint8_t, SHOP_TRADE_RESULT_BYTES> canonical{};
			require(shop_trade_command_encode_result(result, &canonical) &&
					!memcmp(canonical.data(), result_payload, canonical.size()),
				EILSEQ);
		}
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
		if (shared && payload.action == shop_trade_action::discard_invalid)
		{
			// Cleanup has no monetary effects. The authentic original receipt
			// carries unchanged balances; revisions are the frozen original ones.
			require(retained.accounts.empty() && retained.postings.empty() &&
					!wallet_effect && !bank_effect && !keeper_effect &&
					result.wallet_revision ==
						payload.expected_wallet_revision &&
					result.bank_revision == payload.expected_bank_revision,
				EILSEQ);
			authority.balances_before.wallet.amount = result.wallet.amount;
			authority.balances_before.bank.amount = result.bank.amount;
			authority.balances_before.wallet_revision =
				payload.expected_wallet_revision;
			authority.balances_before.bank_revision = payload.expected_bank_revision;
		}
		else
		{
			require(wallet_effect && bank_effect, EILSEQ);
			authority.balances_before.wallet.amount = wallet_effect->before;
			authority.balances_before.bank.amount = bank_effect->before;
			authority.balances_before.wallet_revision = wallet_effect->before_revision;
			authority.balances_before.bank_revision = bank_effect->before_revision;
			if (shared)
			{
				if (payload.price)
					require(keeper_effect &&
							keeper_effect->before ==
								economic_coin_vector{} &&
							keeper_effect->after ==
								economic_coin_vector{} &&
							!keeper_effect->before_revision &&
							!keeper_effect->after_revision,
						EILSEQ);
				else
					require(!keeper_effect, EILSEQ);
			}
			else
			{
				require(keeper_effect, EILSEQ);
				require(keeper_effect->before[0] == payload.expected_keeper_cash,
					EILSEQ);
			}
		}
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

#ifndef __NO_MYSQL__
namespace
{
void publication_session(MYSQL *connection, unsigned long session)
{
	using flag = std::remove_pointer_t<decltype(MYSQL_BIND{}.is_null)>;
	flag reconnect = true;
	require(connection && session && mysql_thread_id(connection) == session &&
			(connection->server_status & SERVER_STATUS_IN_TRANS) &&
			(connection->server_status & SERVER_STATUS_AUTOCOMMIT) &&
			!mysql_get_option(connection, MYSQL_OPT_RECONNECT, &reconnect) &&
			!reconnect,
		ENOTCONN);
}

uint64_t publication_owner_revision(MYSQL *connection, const item_owner_identity &owner)
{
	require(item_owner_identity_valid(owner), EINVAL);
	const auto row = one(connection,
			     "SELECT revision FROM item_owner_revision WHERE owner_type=" +
				     std::to_string(static_cast<uint8_t>(owner.type)) +
				     " AND owner_id=" + std::to_string(owner.id) +
				     " AND owner_context_id=" + std::to_string(owner.context_id) +
				     " FOR UPDATE",
			     1, true);
	// Missing is the actual native optimistic zero; this reader never inserts.
	return row.empty() ? 0 : number<uint64_t>(row[0]);
}

void publication_absence(MYSQL *connection, uint64_t uid, const char *allowed = nullptr)
{
	// Only physical holdings; retained payload/accounting history survives moves.
	for (const char *table :
	     { "player_items", "shopkeeper_items", "player_pet_items", "locker_items",
	       "account_locker_items", "corpse_items", "saved_items", "siege_items" })
	{
		execute(connection, "SELECT id FROM " + std::string(table) + " WHERE obj_uid=" +
					    std::to_string(uid) + " LIMIT 2 FOR UPDATE");
		std::unique_ptr<MYSQL_RES, decltype(&mysql_free_result)> rows(
			mysql_store_result(connection), mysql_free_result);
		require(bool(rows), EIO);
		require(mysql_num_rows(rows.get()) == (allowed && !strcmp(allowed, table) ? 1 : 0),
			ESTALE);
	}
}

bool publication_position_equal(const economic_item_position &a, const economic_item_position &b)
{
	return item_owner_identity_equal(a.owner, b.owner) && a.root_uid == b.root_uid &&
	       a.parent_uid == b.parent_uid && a.revision == b.revision && a.state == b.state &&
	       a.equipment_slot == b.equipment_slot;
}

// Routing only under the already locked keeper row. The first read is
// nonlocking so it does not precede custody locks; the second read locks and
// verifies exact membership without discovering any new custody authority.
std::map<uint64_t, uint64_t> publication_keeper_routes(MYSQL *connection, uint32_t keeper_id,
						       bool locked)
{
	execute(connection, "SELECT id,obj_uid FROM shopkeeper_items WHERE shopkeeper_id=" +
				    std::to_string(keeper_id) + " ORDER BY id LIMIT " +
				    std::to_string(PLAYER_SNAPSHOT_MAX_OBJECTS + 1) +
				    (locked ? " FOR UPDATE" : ""));
	std::unique_ptr<MYSQL_RES, decltype(&mysql_free_result)> rows(
		mysql_store_result(connection), mysql_free_result);
	require(rows && mysql_num_fields(rows.get()) == 2, EIO);
	require(mysql_num_rows(rows.get()) <= PLAYER_SNAPSHOT_MAX_OBJECTS, E2BIG);
	std::map<uint64_t, uint64_t> result;
	std::set<uint64_t> uids;
	MYSQL_ROW row;
	while ((row = mysql_fetch_row(rows.get())))
	{
		require(row[0] && row[1], ESTALE);
		const uint64_t row_id = number<uint64_t>(cell(std::string(row[0]))),
			       uid = number<uint64_t>(cell(std::string(row[1])));
		require(row_id && uid && uid != UINT64_MAX && uids.insert(uid).second &&
				result.emplace(row_id, uid).second,
			ESTALE);
	}
	return result;
}

std::map<uint64_t, locked_item>
publication_custody(MYSQL *connection, const shop_trade_payload &payload,
		    std::span<const uint64_t> retained_player_uids,
		    const std::map<uint64_t, uint64_t> &keeper_routes, bool retained_only)
{
	std::string selected;
	for (size_t index = 0; index < payload.item_count; ++index)
	{
		if (index)
			selected += ',';
		selected += std::to_string(payload.items[index].item_uid);
	}
	std::set<uint64_t> claimed;
	const auto add_uid = [&](uint64_t uid)
	{
		require(uid && uid != UINT64_MAX, EINVAL);
		claimed.insert(uid);
	};
	for (size_t index = 0; index < payload.item_count; ++index)
		add_uid(payload.items[index].item_uid);
	if (payload.stock_item_uid)
		add_uid(payload.stock_item_uid);
	if (payload.target_parent_item_uid)
	{
		add_uid(payload.target_parent_item_uid);
		add_uid(payload.target_root_item_uid);
	}
	for (const auto &[row_id, uid] : keeper_routes)
	{
		(void)row_id;
		add_uid(uid);
	}
	for (uint64_t uid : retained_player_uids)
		add_uid(uid);
	if (payload.recovery_manifest_recorded)
	{
		require(shop_trade_recovery_manifest_shape_valid(payload.recovery_manifest),
			EILSEQ);
		const auto &manifest = payload.recovery_manifest;
		for (const auto *binding : { &manifest.player_before, &manifest.player_after,
					     &manifest.keeper_before, &manifest.keeper_after })
			for (uint64_t uid : binding->ordered_item_uids)
				add_uid(uid);
	}
	std::string explicit_uids;
	for (uint64_t uid : claimed)
	{
		if (!explicit_uids.empty())
			explicit_uids += ',';
		explicit_uids += std::to_string(uid);
	}
	require(!explicit_uids.empty(), EINVAL);
	std::string player_scope =
		" OR (owner_type=" + std::to_string(static_cast<uint8_t>(item_owner_type::player)) +
		" AND owner_id=" + std::to_string(payload.player_pid) +
		" AND coin_payload IS NULL AND state=" +
		std::to_string(static_cast<uint8_t>(item_custody_state::active)) + ")";
	// Explicit identities ignore ownership, context, state and coin payload.
	// Active references to ANY claimed UID must join this original sorted cut.
	player_scope += " OR ((root_item_uid IN(" + explicit_uids + ") OR parent_item_uid IN(" +
			explicit_uids + ")) AND state=" +
			std::to_string(static_cast<uint8_t>(item_custody_state::active)) + ")";
	// Two complete bounded forests. Original selected destruction/production
	// and stock/ancestor references may lie outside those current forests;
	// bound those separately instead of retaining the old selected-only cut.
	const size_t limit = PLAYER_SNAPSHOT_MAX_OBJECTS * 2 + SHOP_TRADE_MAX_ITEMS +
			     PLAYER_SNAPSHOT_MAX_DEPTH + 1;
	require(claimed.size() <= limit, E2BIG);
	// One globally sorted custody cut BEFORE either helper reads physical rows.
	// Include the full player's ordinary scope and all claimed/active root-parent
	// references regardless of coin_payload, matching the whole-player reader.
	// Truly unphysical inline coin holdings are neither omitted physical rows
	// nor mistaken for ordinary persisted inventory.

	execute(connection,
		"SELECT item_uid,root_item_uid,COALESCE(parent_item_uid,0),owner_type,owner_id,"
		"owner_context_id,item_revision,vnum,state,equipment_slot FROM item_current_owner WHERE "
		"item_uid IN(" +
			explicit_uids + ")" +
			(retained_only ?
				 "" :
				 " OR root_item_uid=" + std::to_string(payload.selected_item_uid) +
					 " OR parent_item_uid IN(" + selected +
					 ") OR (owner_type=" +
					 std::to_string(static_cast<uint8_t>(
						 item_owner_type::shopkeeper)) +
					 " AND owner_id=" +
					 std::to_string(item_shopkeeper_owner_id(payload.shop_id)) +
					 ")" +
					 (payload.target_parent_item_uid ?
						  " OR root_item_uid=" +
							  std::to_string(
								  payload.target_root_item_uid) :
						  "") +
					 player_scope) +
			" ORDER BY item_uid LIMIT " + std::to_string(limit + 1) + " FOR UPDATE");
	std::unique_ptr<MYSQL_RES, decltype(&mysql_free_result)> rows(
		mysql_store_result(connection), mysql_free_result);
	require(rows && mysql_num_fields(rows.get()) == 10, EIO);
	require(mysql_num_rows(rows.get()) <= limit, E2BIG);
	std::map<uint64_t, locked_item> result;
	MYSQL_ROW raw;
	while ((raw = mysql_fetch_row(rows.get())))
	{
		std::array<cell, 10> row{};
		for (size_t index = 0; index < row.size(); ++index)
			if (raw[index])
				row[index] = std::string(raw[index]);
		const uint64_t uid = number<uint64_t>(row[0]);
		const auto type = number<uint8_t>(row[3]), state = number<uint8_t>(row[8]);
		const auto slot = number<uint16_t>(row[9]);
		require(uid && uid != UINT64_MAX &&
				type <= static_cast<uint8_t>(item_owner_type::pet) &&
				state <= static_cast<uint8_t>(item_custody_state::quarantined) &&
				slot <= MAX_WEAR,
			ESTALE);
		locked_item entry{};
		entry.exists = true;
		entry.snapshot.uid = uid;
		entry.snapshot.position = { { static_cast<item_owner_type>(type),
					      number<uint64_t>(row[4]), number<uint64_t>(row[5]) },
					    number<uint64_t>(row[1]),
					    number<uint64_t>(row[2]),
					    number<uint64_t>(row[6]),
					    static_cast<item_custody_state>(state),
					    slot };
		entry.vnum = number<int32_t>(row[7]);
		require(item_owner_identity_valid(entry.snapshot.position.owner) &&
				entry.snapshot.position.revision &&
				result.emplace(uid, entry).second,
			ESTALE);
	}
	return result;
}

std::vector<uint64_t> recovery_custody_scope(const shop_trade_payload &payload,
					     const std::map<uint64_t, locked_item> &custody,
					     const std::map<uint64_t, uint64_t> &routes)
{
	std::set<uint64_t> scope;
	for (const auto &[uid, entry] : custody)
	{
		(void)entry;
		scope.insert(uid);
	}
	for (const auto &[row_id, uid] : routes)
	{
		(void)row_id;
		scope.insert(uid);
	}
	const auto &manifest = payload.recovery_manifest;
	for (const auto *binding : { &manifest.player_before, &manifest.player_after,
				     &manifest.keeper_before, &manifest.keeper_after })
		scope.insert(binding->ordered_item_uids.begin(), binding->ordered_item_uids.end());
	for (size_t index = 0; index < payload.item_count; ++index)
		scope.insert(payload.items[index].item_uid);
	for (uint64_t uid : { payload.stock_item_uid, payload.target_parent_item_uid,
			      payload.target_root_item_uid })
		if (uid)
			scope.insert(uid);
	require(scope.size() <= PLAYER_SNAPSHOT_MAX_OBJECTS * 2 + SHOP_TRADE_MAX_ITEMS +
					PLAYER_SNAPSHOT_MAX_DEPTH + 1,
		E2BIG);
	return { scope.begin(), scope.end() };
}

void recovery_keeper_closure(const shop_trade_payload &payload,
			     const std::map<uint64_t, locked_item> &custody, bool after)
{
	const auto &binding = after ? payload.recovery_manifest.keeper_after :
				      payload.recovery_manifest.keeper_before;
	const std::set<uint64_t> keeper_uids(binding.ordered_item_uids.begin(),
					     binding.ordered_item_uids.end());
	const auto owner = shop_owner(payload);
	for (const auto &[uid, entry] : custody)
	{
		const auto &position = entry.snapshot.position;
		if (position.state != item_custody_state::active)
			continue;
		const bool same_keeper = position.owner.type == owner.type &&
					 position.owner.id == owner.id;
		const bool references_keeper = keeper_uids.count(position.root_uid) ||
					       keeper_uids.count(position.parent_uid);
		if (same_keeper || references_keeper)
			require(keeper_uids.count(uid) &&
					item_owner_identity_equal(position.owner, owner),
				ESTALE);
	}
}

// Authenticate complete current values in the original open transaction.
// The pre-mutation cut already holds the union of all persisted manifest UIDs.
// This is NOT historical receipt validation or permission to publish/ACK.
void recovery_images(MYSQL *connection, const shop_trade_payload &payload, uint32_t keeper_id,
		     unsigned long session, bool after)
{
	publication_session(connection, session);
	require(payload.recovery_manifest_recorded &&
			shop_trade_recovery_manifest_shape_valid(payload.recovery_manifest),
		EILSEQ);
	const auto &manifest = payload.recovery_manifest;
	const auto &player_binding = after ? manifest.player_after : manifest.player_before;
	const auto &keeper_binding = after ? manifest.keeper_after : manifest.keeper_before;
	const auto marker = one(
		connection,
		"SELECT shop_revision,runtime_payload_checkpoint_revision FROM shopkeepers WHERE id=" +
			std::to_string(keeper_id) + " FOR UPDATE",
		2);
	require(marker[1].has_value(), ENODATA);
	const auto checkpoint = number<uint64_t>(marker[1]);
	require(checkpoint && checkpoint <= number<uint64_t>(marker[0]), ESTALE);
	// Routes are only a membership check under the retained keeper lock.
	// They cannot grant newly discovered custody after physical reads began.
	std::set<uint64_t> retained_keeper;
	for (const auto *binding : { &manifest.keeper_before, &manifest.keeper_after })
		retained_keeper.insert(binding->ordered_item_uids.begin(),
				       binding->ordered_item_uids.end());
	const auto routes = publication_keeper_routes(connection, keeper_id, false);
	for (const auto &[row_id, uid] : routes)
	{
		(void)row_id;
		require(retained_keeper.count(uid), ESTALE);
	}
	shop_item_runtime_image player_image, keeper_image;
	require(shop_item_runtime_lock_player_image(
			connection, payload.player_pid,
			std::span<const uint64_t>(player_binding.ordered_item_uids), &player_image),
		errno ? errno : EAGAIN);
	require(publication_keeper_routes(connection, keeper_id, true) == routes, ESTALE);
	require(shop_item_runtime_keeper_image(connection, keeper_id, payload.shop_id,
					       payload.keeper_vnum, &keeper_image),
		errno ? errno : EAGAIN);
	for (const auto &[uid, row] : keeper_image)
	{
		(void)row;
		publication_absence(connection, uid, "shopkeeper_items");
	}
	std::vector<player_item_snapshot> player_values, keeper_values;
	require(shop_trade_recovery_image_reconstruct(
			player_binding,
			after ? shop_trade_recovery_forest_role::player_after :
				shop_trade_recovery_forest_role::player_before,
			player_image, &player_values) &&
			shop_trade_recovery_image_reconstruct(
				keeper_binding,
				after ? shop_trade_recovery_forest_role::keeper_after :
					shop_trade_recovery_forest_role::keeper_before,
				keeper_image, &keeper_values),
		ESTALE);
	publication_session(connection, session);
}

shop_item_runtime_row publication_player_row(MYSQL *connection, const shop_trade_payload &payload,
					     const locked_item &entry,
					     const std::map<uint64_t, locked_item> &custody,
					     const player_item_snapshot *expected)
{
	const auto &position = entry.snapshot.position;
	require(position.state == item_custody_state::active &&
			item_owner_identity_equal(position.owner, player_owner(payload)),
		ESTALE);
	const auto row = one(
		connection,
		"SELECT id,pid,vnum,equip_slot,COALESCE(container_id,0) FROM player_items WHERE obj_uid=" +
			std::to_string(entry.snapshot.uid) + " LIMIT 2 FOR UPDATE",
		5);
	const uint64_t row_id = number<uint64_t>(row[0]), parent_id = number<uint64_t>(row[4]);
	require(row_id && number<uint64_t>(row[1]) == payload.player_pid &&
			number<int32_t>(row[2]) == entry.vnum &&
			number<uint16_t>(row[3]) == position.equipment_slot,
		ESTALE);
	if (position.parent_uid)
	{
		const auto parent = custody.find(position.parent_uid);
		require(parent != custody.end() && parent_id &&
				parent->second.snapshot.position.state ==
					item_custody_state::active &&
				item_owner_identity_equal(parent->second.snapshot.position.owner,
							  player_owner(payload)) &&
				parent->second.snapshot.position.root_uid == position.root_uid,
			ESTALE);
		const auto physical_parent =
			one(connection,
			    "SELECT id,pid FROM player_items WHERE obj_uid=" +
				    std::to_string(position.parent_uid) + " LIMIT 2 FOR UPDATE",
			    2);
		require(number<uint64_t>(physical_parent[0]) == parent_id &&
				number<uint64_t>(physical_parent[1]) == payload.player_pid,
			ESTALE);
	}
	else
		require(!parent_id && position.root_uid == entry.snapshot.uid, ESTALE);
	player_item_snapshot current{};
	bool present = false;
	require(shop_item_runtime_read(connection, false, row_id, &current, &present),
		errno ? errno : EAGAIN);
	require(present && current.object_uid == entry.snapshot.uid && current.vnum == entry.vnum &&
			current.equipment_slot == position.equipment_slot,
		ENODATA);
	require(shop_item_runtime_verify(connection, false, row_id, payload.player_pid, parent_id,
					 current),
		errno ? errno : EAGAIN);
	publication_absence(connection, entry.snapshot.uid, "player_items");
	if (expected)
	{
		auto literal = *expected;
		literal.parent_index = PLAYER_SNAPSHOT_NO_PARENT;
		std::vector<uint8_t> actual_bytes, expected_bytes;
		require(player_item_snapshot_list_encode({ current }, &actual_bytes) ==
					player_snapshot_codec_result::ok &&
				player_item_snapshot_list_encode({ literal }, &expected_bytes) ==
					player_snapshot_codec_result::ok,
			EAGAIN);
		require(actual_bytes == expected_bytes, ESTALE);
	}
	return { row_id,
		 parent_id,
		 position.root_uid,
		 position.revision,
		 static_cast<int16_t>(position.equipment_slot),
		 std::move(current),
		 true };
}
} // namespace
#endif

static unsigned int shop_trade_lock_publication_image(
	MYSQL *connection, const critical_command &command, const critical_completion &sealed,
	std::span<const player_item_snapshot> expected_current_player_items, bool cold,
	economic_sql_shop_trade_publication *output) noexcept
{
#ifdef __NO_MYSQL__
	(void)connection;
	(void)command;
	(void)sealed;
	(void)expected_current_player_items;
	(void)cold;
	(void)output;
	return ENOTSUP;
#else
	if (!connection || !output)
		return EINVAL;
	try
	{
		const unsigned long session = mysql_thread_id(connection);
		publication_session(connection, session);
		require(expected_current_player_items.size() <= PLAYER_SNAPSHOT_MAX_OBJECTS, E2BIG);
		const std::vector<player_item_snapshot> expected_player(
			expected_current_player_items.begin(), expected_current_player_items.end());
		if (!cold)
		{
			std::vector<uint8_t> expected_player_bytes;
			const auto player_code = player_item_snapshot_list_encode(
				expected_player, &expected_player_bytes);
			require(player_code == player_snapshot_codec_result::ok,
				player_code == player_snapshot_codec_result::allocation_failure ?
					ENOMEM :
					EINVAL);
			require(expected_player_bytes.size() <= PLAYER_SNAPSHOT_MAX_BYTES, E2BIG);
		}
		economic_frozen_intent intent;
		shop_trade_payload payload{};
		economic_account_key wallet, bank, virtual_keeper;
		shop_trade_result receipt{};
		std::array<uint8_t, SHOP_TRADE_RESULT_BYTES> encoded{};
		require(command.publication_required &&
				command.schema_version ==
					CRITICAL_COMMAND_ACCOUNTING_SCHEMA_VERSION &&
				command.type == critical_command_type::shop_trade &&
				shop_trade_accounting_decode(command, &intent, &payload, &wallet,
							     &bank, &virtual_keeper) ==
					economic_accounting_error::ok &&
				intent.admission.facts.size() == 16,
			EPROTONOSUPPORT);
		if (cold)
			require(command.payload_version == SHOP_TRADE_RECOVERY_MANIFEST_VERSION &&
					payload.recovery_manifest_recorded &&
					shop_trade_recovery_manifest_shape_valid(
						payload.recovery_manifest),
				EPROTONOSUPPORT);
		const bool rejected = sealed.outcome == critical_apply_outcome::terminal_failure;
		require(sealed.operation_id.bytes == command.operation_id.bytes &&
				sealed.disposition == critical_completion_disposition::execution &&
				sealed.failure_stage == critical_failure_stage::none &&
				(rejected ?
					 sealed.error_code != 0 :
					 !sealed.error_code &&
						 (sealed.outcome ==
							  critical_apply_outcome::applied ||
						  sealed.outcome ==
							  critical_apply_outcome::already_applied)) &&
				sealed.result_size == encoded.size() &&
				shop_trade_command_decode_result(sealed.result_payload.data(),
								 sealed.result_size, &receipt) &&
				shop_trade_command_encode_result(receipt, &encoded) &&
				std::equal(encoded.begin(), encoded.end(),
					   sealed.result_payload.begin()) &&
				std::all_of(sealed.result_payload.begin() + encoded.size(),
					    sealed.result_payload.end(),
					    [](uint8_t byte) { return !byte; }) &&
				receipt.action == payload.action &&
				sealed.durable_revision ==
					std::max({ receipt.wallet_revision, receipt.bank_revision,
						   receipt.shop_revision,
						   receipt.player_owner_revision,
						   receipt.counterparty_owner_revision,
						   *std::max_element(
							   receipt.item_revisions.begin(),
							   receipt.item_revisions.end()) }),
			EILSEQ);
		std::vector<player_item_snapshot> after;
		require(shop_trade_accounted_after_items(payload, &after), EAGAIN);
		economic_sql_shop_trade_publication current;
		current.session_id = session;
		current.rejected = rejected;
		current.bank_id = native_id(connection, bank.authority_id);
		const std::array<economic_sql_mapping_request, 2> mappings = {
			economic_sql_mapping_request{ wallet, PLAYER_LOCATOR, payload.player_pid },
			economic_sql_mapping_request{ bank, BANK_LOCATOR, current.bank_id }
		};
		const auto authority_error = economic_sql_lock_authority(
			connection, intent.admission.metadata.lineage,
			intent.admission.metadata.epoch, mappings, &current.authority);
		require(!authority_error, authority_error);
		const auto player = one(
			connection,
			"SELECT account_name,racewar,copper,silver,gold,platinum,wallet_revision,level,save_revision "
			"FROM player_data WHERE pid=" +
				std::to_string(payload.player_pid) + " FOR UPDATE",
			9);
		require(player[0] && !strcasecmp(player[0]->c_str(), payload.account_name.data()) &&
				number<uint8_t>(player[1]) == payload.racewar,
			ESTALE);
		current.player_level = number<uint32_t>(player[7]);
		current.player_save_revision = number<uint64_t>(player[8]);
		require(current.player_level && current.player_level <= UINT8_MAX &&
				current.player_save_revision >=
					payload.expected_player_save_revision,
			ESTALE);
		for (size_t coin = 0; coin < 4; ++coin)
		{
			current.wallet.amount[coin] = number<int64_t>(player[coin + 2]);
			require(current.wallet.amount[coin] >= 0 &&
					current.wallet.amount[coin] <= INT_MAX,
				ERANGE);
		}
		current.wallet_revision = number<uint64_t>(player[6]);
		const auto bank_row = one(
			connection,
			"SELECT id,account_name,racewar,bank_copper,bank_silver,bank_gold,bank_platinum,bank_revision "
			"FROM account_banks WHERE id=" +
				std::to_string(current.bank_id) + " FOR UPDATE",
			8);
		require(number<uint64_t>(bank_row[0]) == current.bank_id && bank_row[1] &&
				!strcasecmp(bank_row[1]->c_str(), payload.account_name.data()) &&
				number<uint8_t>(bank_row[2]) == payload.racewar,
			ESTALE);
		for (size_t coin = 0; coin < 4; ++coin)
		{
			current.bank.amount[coin] = number<int64_t>(bank_row[coin + 3]);
			require(current.bank.amount[coin] >= 0 &&
					current.bank.amount[coin] <= INT_MAX,
				ERANGE);
		}
		current.bank_revision = number<uint64_t>(bank_row[7]);
		// Later legitimate revisions are current values, never overwritten by
		// this historical operation. At the same revision the vectors are exact.
		require(current.wallet_revision >= receipt.wallet_revision &&
				current.bank_revision >= receipt.bank_revision &&
				(current.wallet_revision != receipt.wallet_revision ||
				 current.wallet.amount == receipt.wallet.amount) &&
				(current.bank_revision != receipt.bank_revision ||
				 current.bank.amount == receipt.bank.amount),
			ESTALE);
		const auto keeper_row = one(
			connection,
			"SELECT id,shop_id,mob_vnum,cash,shop_revision,keeper_roaming,runtime_payload_checkpoint_revision "
			"FROM shopkeepers WHERE shop_id=" +
				std::to_string(payload.shop_id) + " FOR UPDATE",
			7);
		const uint64_t keeper_id = number<uint64_t>(keeper_row[0]);
		require(keeper_id && keeper_id <= UINT32_MAX &&
				number<uint32_t>(keeper_row[1]) == payload.shop_id &&
				number<int32_t>(keeper_row[2]) == payload.keeper_vnum,
			ESTALE);
		current.keeper_id = static_cast<uint32_t>(keeper_id);
		current.keeper_cash = number<int64_t>(keeper_row[3]);
		current.shop_revision = number<uint64_t>(keeper_row[4]);
		const auto roaming = number<uint8_t>(keeper_row[5]);
		require(current.keeper_cash >= 0 && current.keeper_cash <= INT_MAX &&
				roaming <= 1 && roaming == payload.keeper_roaming &&
				current.shop_revision >= (rejected ?
								  payload.expected_shop_revision :
								  receipt.shop_revision),
			ESTALE);
		current.keeper_roaming = roaming != 0;
		require(rejected ? (current.shop_revision != payload.expected_shop_revision ||
				    current.keeper_cash == payload.expected_keeper_cash) :
				   (receipt.keeper_cash_recorded &&
				    (current.shop_revision != receipt.shop_revision ||
				     current.keeper_cash == receipt.keeper_cash)),
			ESTALE);
		current.payload_checkpoint_recorded = keeper_row[6].has_value();
		require(current.payload_checkpoint_recorded, ENODATA);
		current.payload_checkpoint_revision = number<uint64_t>(keeper_row[6]);
		require(current.payload_checkpoint_revision &&
				current.payload_checkpoint_revision <= current.shop_revision,
			ESTALE);
		const auto primary = primary_owner(payload), other = counterparty_owner(payload);
		std::vector<item_owner_identity> owners = { primary, other, player_owner(payload),
							    shop_owner(payload) };
		const auto owner_less = [](const auto &a, const auto &b) {
			return std::tie(a.type, a.id, a.context_id) <
			       std::tie(b.type, b.id, b.context_id);
		};
		std::sort(owners.begin(), owners.end(), owner_less);
		owners.erase(std::unique(owners.begin(), owners.end(), item_owner_identity_equal),
			     owners.end());
		for (const auto &owner : owners)
		{
			const uint64_t revision = publication_owner_revision(connection, owner);
			if (item_owner_identity_equal(owner, primary))
				current.player_owner_revision = revision;
			if (item_owner_identity_equal(owner, other))
				current.counterparty_owner_revision = revision;
			if (item_owner_identity_equal(owner, player_owner(payload)))
				current.wallet_owner_revision = revision;
			if (item_owner_identity_equal(owner, shop_owner(payload)))
				current.keeper_owner_revision = revision;
		}
		require(rejected ||
				(current.player_owner_revision >= receipt.player_owner_revision &&
				 current.counterparty_owner_revision >=
					 receipt.counterparty_owner_revision),
			ESTALE);
		const auto keeper_routes =
			publication_keeper_routes(connection, current.keeper_id, false);
		std::vector<uint64_t> expected_player_uids;
		if (cold)
			expected_player_uids = (rejected ? payload.recovery_manifest.player_before :
							   payload.recovery_manifest.player_after)
						       .ordered_item_uids;
		else
		{
			expected_player_uids.reserve(expected_player.size());
			for (const auto &entry : expected_player)
				expected_player_uids.push_back(entry.object_uid);
		}
		const auto custody = publication_custody(connection, payload, expected_player_uids,
							 keeper_routes);
		if (payload.recovery_manifest_recorded)
			recovery_keeper_closure(payload, custody, !rejected);
		for (const auto &[row_id, uid] : keeper_routes)
		{
			(void)row_id;
			const auto entry = custody.find(uid);
			require(entry != custody.end() &&
					entry->second.snapshot.position.state ==
						item_custody_state::active &&
					item_owner_identity_equal(
						entry->second.snapshot.position.owner,
						shop_owner(payload)),
				ESTALE);
		}
		economic_accounting_plan retained_plan;
		if (!rejected)
		{
			const auto plan_row = one(
				connection,
				"SELECT SUBSTRING(canonical_plan,1," +
					std::to_string(ECONOMIC_ACCOUNTING_MAX_PLAN_BYTES + 1) +
					"),OCTET_LENGTH(canonical_plan) FROM economic_accounting_operation WHERE operation_id=" +
					id(command.operation_id),
				2);
			require(plan_row[0] &&
					plan_row[0]->size() <= ECONOMIC_ACCOUNTING_MAX_PLAN_BYTES &&
					number<uint64_t>(plan_row[1]) == plan_row[0]->size(),
				EILSEQ);
			require(economic_plan_decode(
					std::span<const uint8_t>(reinterpret_cast<const uint8_t *>(
									 plan_row[0]->data()),
								 plan_row[0]->size()),
					&retained_plan) == economic_accounting_error::ok,
				EAGAIN);
			require(economic_plan_validate_structure(retained_plan) ==
						economic_accounting_error::ok &&
					receipt.item_count == payload.item_count,
				EILSEQ);
		}
		std::set<uint64_t> selected;
		for (size_t index = 0; index < payload.item_count; ++index)
			require(selected.insert(payload.items[index].item_uid).second, EILSEQ);
		for (const auto &[uid, entry] : custody)
		{
			current.custody.push_back(entry.snapshot);
			current.custody_vnums.push_back(entry.vnum);
			require((!selected.count(entry.snapshot.position.parent_uid) ||
				 selected.count(uid)) &&
					(entry.snapshot.position.root_uid !=
						 payload.selected_item_uid ||
					 selected.count(uid)),
				ESTALE);
		}
		// All active ordinary player custody and exact claimed/root-parent
		// references were locked above in the same sorted cut as keeper custody.
		// The reader rechecks those locks, then proves the complete native forest
		// BEFORE any keeper helper physical query, without acquiring a new owner.
		if (cold)
			require(shop_item_runtime_lock_player_image(
					connection, payload.player_pid,
					std::span<const uint64_t>(expected_player_uids),
					&current.whole_player_items),
				errno ? errno : EAGAIN);
		else
			require(shop_item_runtime_lock_player_image(connection, payload.player_pid,
								    expected_player,
								    &current.whole_player_items),
				errno ? errno : EAGAIN);
		require(publication_keeper_routes(connection, current.keeper_id, true) ==
				keeper_routes,
			ESTALE);
		require(shop_item_runtime_keeper_image(connection, current.keeper_id,
						       payload.shop_id, payload.keeper_vnum,
						       &current.keeper_items),
			errno ? errno : EAGAIN);
		for (const auto &[uid, row] : current.keeper_items)
			publication_absence(connection, uid, "shopkeeper_items");
		if (payload.recovery_manifest_recorded)
		{
			const auto &manifest = payload.recovery_manifest;
			std::vector<player_item_snapshot> player_values, keeper_values;
			require(shop_trade_recovery_image_reconstruct(
					rejected ? manifest.player_before : manifest.player_after,
					rejected ? shop_trade_recovery_forest_role::player_before :
						   shop_trade_recovery_forest_role::player_after,
					current.whole_player_items, &player_values) &&
					shop_trade_recovery_image_reconstruct(
						rejected ? manifest.keeper_before :
							   manifest.keeper_after,
						rejected ?
							shop_trade_recovery_forest_role::
								keeper_before :
							shop_trade_recovery_forest_role::keeper_after,
						current.keeper_items, &keeper_values),
				ESTALE);
		}
		std::vector<player_item_snapshot> before;
		require(player_item_snapshot_list_decode(payload.item_blob.data(),
							 payload.item_blob_size, &before) ==
				player_snapshot_codec_result::ok,
			EAGAIN);
		for (size_t index = 0; index < payload.item_count; ++index)
		{
			const auto &original = payload.items[index];
			const auto found = custody.find(original.item_uid);
			if (rejected && payload.action == shop_trade_action::buy_produced)
			{
				require(found == custody.end(), ESTALE);
				publication_absence(connection, original.item_uid);
				continue;
			}
			require(found != custody.end() && found->second.vnum == original.vnum,
				ESTALE);
			const auto &entry = found->second;
			const auto &position = entry.snapshot.position;
			const player_item_snapshot *literal = nullptr;
			if (rejected)
			{
				const bool from_shop =
					payload.action == shop_trade_action::buy_existing ||
					payload.action == shop_trade_action::discard_invalid;
				require(position.state == item_custody_state::active &&
						item_owner_identity_equal(
							position.owner,
							from_shop ? shop_owner(payload) :
								    player_owner(payload)) &&
						position.root_uid == original.root_item_uid &&
						position.parent_uid == original.parent_item_uid &&
						!position.equipment_slot &&
						position.revision >=
							original.expected_item_revision,
					ESTALE);
				if (position.revision == original.expected_item_revision)
				{
					const auto snapshot = std::find_if(
						before.begin(), before.end(), [&](const auto &value)
						{ return value.object_uid == original.item_uid; });
					require(snapshot != before.end(), EILSEQ);
					literal = &*snapshot;
				}
			}
			else
			{
				const auto event = std::find_if(
					retained_plan.item_events.begin(),
					retained_plan.item_events.end(), [&](const auto &value)
					{ return value.uid == original.item_uid; });
				require(event != retained_plan.item_events.end() &&
						event->event_index == index &&
						receipt.item_uids[index] == original.item_uid &&
						receipt.item_revisions[index] ==
							event->after.revision &&
						publication_position_equal(position, event->after),
					ESTALE);
				const auto snapshot = std::find_if(
					after.begin(), after.end(), [&](const auto &value)
					{ return value.object_uid == original.item_uid; });
				require(snapshot != after.end(), EILSEQ);
				literal = &*snapshot;
			}
			if (position.state == item_custody_state::destroyed)
			{
				publication_absence(connection, original.item_uid);
				continue;
			}
			if (item_owner_identity_equal(position.owner, player_owner(payload)))
				require(current.player_items
						.emplace(original.item_uid,
							 publication_player_row(connection, payload,
										entry, custody,
										literal))
						.second,
					EILSEQ);
			else
			{
				require(item_owner_identity_equal(position.owner,
								  shop_owner(payload)),
					ESTALE);
				const auto physical = current.keeper_items.find(original.item_uid);
				require(physical != current.keeper_items.end() &&
						physical->second.revision == position.revision,
					ESTALE);
				if (literal)
				{
					auto expected = *literal;
					expected.parent_index = PLAYER_SNAPSHOT_NO_PARENT;
					std::vector<uint8_t> actual_bytes, expected_bytes;
					require(player_item_snapshot_list_encode(
							{ physical->second.item }, &actual_bytes) ==
								player_snapshot_codec_result::ok &&
							player_item_snapshot_list_encode(
								{ expected }, &expected_bytes) ==
								player_snapshot_codec_result::ok,
						EAGAIN);
					require(actual_bytes == expected_bytes, ESTALE);
				}
			}
		}
		if (payload.action == shop_trade_action::buy_produced)
		{
			const auto stock = custody.find(payload.stock_item_uid);
			require(stock != custody.end() &&
					stock->second.vnum == payload.stock_vnum &&
					stock->second.snapshot.position.state ==
						item_custody_state::active &&
					item_owner_identity_equal(
						stock->second.snapshot.position.owner,
						shop_owner(payload)) &&
					stock->second.snapshot.position.root_uid ==
						payload.stock_item_uid &&
					!stock->second.snapshot.position.parent_uid &&
					!stock->second.snapshot.position.equipment_slot &&
					stock->second.snapshot.position.revision >=
						payload.expected_stock_item_revision &&
					current.keeper_items.count(payload.stock_item_uid),
				ESTALE);
			if (!rejected)
			{
				const auto witness = std::find_if(
					retained_plan.items_after.begin(),
					retained_plan.items_after.end(), [&](const auto &entry)
					{ return entry.uid == payload.stock_item_uid; });
				require(witness != retained_plan.items_after.end() &&
						publication_position_equal(
							stock->second.snapshot.position,
							witness->position),
					ESTALE);
			}
			std::set<uint64_t> ancestors;
			uint64_t uid = payload.target_parent_item_uid;
			while (uid)
			{
				require(ancestors.size() < PLAYER_SNAPSHOT_MAX_DEPTH &&
						ancestors.insert(uid).second,
					ELOOP);
				const auto ancestor = custody.find(uid);
				require(ancestor != custody.end() &&
						ancestor->second.snapshot.position.state ==
							item_custody_state::active &&
						item_owner_identity_equal(
							ancestor->second.snapshot.position.owner,
							player_owner(payload)) &&
						ancestor->second.snapshot.position.root_uid ==
							payload.target_root_item_uid,
					ESTALE);
				if (!rejected)
				{
					const auto witness =
						std::find_if(retained_plan.items_after.begin(),
							     retained_plan.items_after.end(),
							     [&](const auto &entry)
							     { return entry.uid == uid; });
					require(witness != retained_plan.items_after.end() &&
							publication_position_equal(
								ancestor->second.snapshot.position,
								witness->position),
						ESTALE);
				}
				require(current.player_items
						.emplace(uid,
							 publication_player_row(connection, payload,
										ancestor->second,
										custody, nullptr))
						.second,
					EILSEQ);
				uid = ancestor->second.snapshot.position.parent_uid;
			}
		}
		size_t player_bytes = 0;
		for (const auto &[uid, row] : current.player_items)
		{
			(void)uid;
			std::vector<uint8_t> bytes;
			require(player_item_snapshot_list_encode({ row.item }, &bytes) ==
					player_snapshot_codec_result::ok,
				EAGAIN);
			require(bytes.size() <= PLAYER_SNAPSHOT_MAX_BYTES - player_bytes, E2BIG);
			player_bytes += bytes.size();
		}
		// Each selected physical parent's entire child closure must agree with
		// the exact selected subtree; unrelated destination ancestors may also
		// contain other roots and are not mislabeled a complete player census.
		for (const auto uid : selected)
		{
			const auto &image = current.player_items.count(uid) ? current.player_items :
									      current.keeper_items;
			const auto parent = image.find(uid);
			if (parent == image.end())
				continue; // A rejected production or a destroyed tree has no rows.
			const bool player_domain = &image == &current.player_items;
			execute(connection,
				"SELECT obj_uid FROM " +
					std::string(player_domain ? "player_items" :
								    "shopkeeper_items") +
					" WHERE container_id=" + std::to_string(parent->second.id) +
					" LIMIT " + std::to_string(SHOP_TRADE_MAX_ITEMS + 1) +
					" FOR UPDATE");
			std::unique_ptr<MYSQL_RES, decltype(&mysql_free_result)> children(
				mysql_store_result(connection), mysql_free_result);
			require(children && mysql_num_rows(children.get()) <= SHOP_TRADE_MAX_ITEMS,
				E2BIG);
			size_t count = 0;
			MYSQL_ROW child;
			while ((child = mysql_fetch_row(children.get())))
			{
				require(child[0], ESTALE);
				const uint64_t child_uid =
					number<uint64_t>(cell(std::string(child[0])));
				const auto entry = custody.find(child_uid);
				require(selected.count(child_uid) && entry != custody.end() &&
						entry->second.snapshot.position.parent_uid == uid,
					ESTALE);
				++count;
			}
			const size_t expected = std::count_if(
				custody.begin(), custody.end(),
				[&](const auto &entry) {
					return selected.count(entry.first) &&
					       entry.second.snapshot.position.parent_uid == uid;
				});
			require(count == expected, ESTALE);
		}
		publication_session(connection, session);
		*output = std::move(current);
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
	catch (...)
	{
		return EIO;
	}
#endif
}

unsigned int economic_sql_shop_trade_lock_publication(
	MYSQL *connection, const critical_command &command, const critical_completion &sealed,
	std::span<const player_item_snapshot> expected_current_player_items,
	economic_sql_shop_trade_publication *output) noexcept
{
	return shop_trade_lock_publication_image(connection, command, sealed,
						 expected_current_player_items, false, output);
}

unsigned int
economic_sql_shop_trade_lock_publication(MYSQL *connection, const critical_command &command,
					 const critical_completion &sealed,
					 economic_sql_shop_trade_publication *output) noexcept
{
	return shop_trade_lock_publication_image(
		connection, command, sealed, std::span<const player_item_snapshot>{}, true, output);
}
