#include "persistence/sql_room_coin_payload.h"
#include "persistence/sql_room_item_payload.h"
#include "persistence/critical_outbox.h"
#include "economy/coin_transfer_accounting.h"
#include "economy/economic_accounting_intent.h"
#include "item/economic_accounting_item_reference.h"
#include "player/player_snapshot_codec.h"
#include "core/defines.h"

#include <algorithm>
#include <array>
#include <cerrno>
#include <charconv>
#include <climits>
#include <cstring>
#include <memory>
#include <new>
#include <optional>
#include <openssl/sha.h>
#include <string>
#include <type_traits>
#include <utility>

namespace
{
bool refuse(unsigned int error)
{
	errno = static_cast<int>(error);
	return false;
}
#ifndef __NO_MYSQL__
struct failure
{
	unsigned int code;
};
void require(bool condition, unsigned int error = EILSEQ)
{
	if (!condition)
		throw failure{ error };
}
void checked(economic_accounting_error error)
{
	require(error == economic_accounting_error::ok,
		error == economic_accounting_error::capacity ? ENOMEM : EILSEQ);
}
bool session(MYSQL *connection, unsigned long expected, bool in_transaction)
{
	using flag = std::remove_pointer_t<decltype(MYSQL_BIND{}.is_null)>;
	flag reconnect = false;
	return connection && expected && mysql_thread_id(connection) == expected &&
	       (!in_transaction || ((connection->server_status & SERVER_STATUS_IN_TRANS) &&
				    (connection->server_status & SERVER_STATUS_AUTOCOMMIT))) &&
	       !mysql_get_option(connection, MYSQL_OPT_RECONNECT, &reconnect) && !reconnect;
}
using cell = std::optional<std::string>;
using row = std::vector<cell>;
std::vector<row> rows(MYSQL *connection, const std::string &sql, size_t columns, size_t limit)
{
	const auto query_status = mysql_real_query(connection, sql.data(), sql.size());
	const auto query_error = mysql_errno(connection);
	require(!query_status, query_error ? query_error : EIO);
	std::unique_ptr<MYSQL_RES, decltype(&mysql_free_result)> result(
		mysql_store_result(connection), mysql_free_result);
	require(bool(result), mysql_errno(connection) ? mysql_errno(connection) : EIO);
	require(mysql_num_fields(result.get()) == columns && mysql_num_rows(result.get()) <= limit);
	std::vector<row> values;
	values.reserve(static_cast<size_t>(mysql_num_rows(result.get())));
	while (MYSQL_ROW source = mysql_fetch_row(result.get()))
	{
		const auto *lengths = mysql_fetch_lengths(result.get());
		require(lengths != nullptr, EIO);
		row value;
		value.reserve(columns);
		for (size_t index = 0; index < columns; ++index)
			value.emplace_back(
				source[index] ? cell(std::string(source[index], lengths[index])) :
						std::nullopt);
		values.push_back(std::move(value));
	}
	require(!mysql_errno(connection), mysql_errno(connection));
	return values;
}
row one(MYSQL *connection, const std::string &sql, size_t columns)
{
	auto result = rows(connection, sql, columns, 2);
	require(result.size() == 1);
	return std::move(result[0]);
}
template <typename T> T number(const cell &value)
{
	require(value && !value->empty());
	T result = 0;
	const auto parsed = std::from_chars(value->data(), value->data() + value->size(), result);
	require(parsed.ec == std::errc{} && parsed.ptr == value->data() + value->size());
	return result;
}
std::string hex(std::span<const uint8_t> bytes)
{
	static constexpr char digits[] = "0123456789abcdef";
	std::string value = "X'";
	value.reserve(2 * bytes.size() + 3);
	for (uint8_t byte : bytes)
	{
		value += digits[byte >> 4];
		value += digits[byte & 15];
	}
	return value + '\'';
}
std::string id(const critical_operation_id &value)
{
	return hex(value.bytes);
}
critical_operation_id operation_id(const cell &value)
{
	critical_operation_id result = {};
	require(value && value->size() == result.bytes.size());
	std::copy(value->begin(), value->end(), result.bytes.begin());
	require(!critical_operation_id_is_zero(result));
	return result;
}
std::span<const uint8_t> bytes(const cell &value)
{
	require(value.has_value());
	return { reinterpret_cast<const uint8_t *>(value->data()), value->size() };
}
void count(MYSQL *connection, const std::string &table, const std::string &predicate,
	   uint64_t expected)
{
	require(number<uint64_t>(one(connection,
				     "SELECT COUNT(*) FROM " + table + " WHERE " + predicate,
				     1)[0]) == expected);
}
std::string predicate(const std::vector<std::pair<std::string, std::string>> &fields)
{
	std::string value;
	for (const auto &[name, expected] : fields)
	{
		if (!value.empty())
			value += " AND ";
		value += name + " <=> " + expected;
	}
	return value;
}
uint64_t little_u64(std::span<const uint8_t> value, size_t offset)
{
	require(offset <= value.size() && value.size() - offset >= 8);
	uint64_t result = 0;
	for (size_t index = 0; index < 8; ++index)
		result |= uint64_t(value[offset + index]) << (8 * index);
	return result;
}
// Exact suffix of the existing canonical 40-byte account key, excluding lineage.
// There is no second account catalog or invented command binding here.
std::array<uint8_t, ECONOMIC_ACCOUNT_KEY_BYTES - 16> pile_suffix(uint64_t uid)
{
	std::array<uint8_t, ECONOMIC_ACCOUNT_KEY_BYTES - 16> result = {};
	result[0] = static_cast<uint8_t>(ECONOMIC_ACCOUNTING_VERSION);
	result[1] = static_cast<uint8_t>(ECONOMIC_ACCOUNTING_VERSION >> 8);
	result[2] = static_cast<uint8_t>(economic_account_kind::pile);
	for (size_t index = 0; index < 8; ++index)
		result[4 + index] = static_cast<uint8_t>(uid >> (8 * index));
	return result;
}
std::string typed_root(const std::string &operation, const std::string &inbox)
{
	return "(" + operation +
	       ".writer_id=" + std::to_string(ECONOMIC_WRITER_WALLET_COIN_TRANSFER) + " AND " +
	       operation +
	       ".reason=" + std::to_string(static_cast<unsigned>(economic_reason::coin_transfer)) +
	       " OR " + inbox + ".command_type=" +
	       std::to_string(static_cast<unsigned>(critical_command_type::coin_transfer)) + ")";
}
std::string history(uint64_t uid)
{
	const auto value = std::to_string(uid);
	return "(EXISTS(SELECT 1 FROM item_current_owner own WHERE own.item_uid=" + value +
	       " AND own.coin_payload IS NOT NULL) OR "
	       "EXISTS(SELECT 1 FROM economic_accounting_item_reference ref "
	       "LEFT JOIN economic_accounting_operation op ON op.operation_id=ref.operation_id "
	       "LEFT JOIN critical_operation_inbox inbox ON inbox.operation_id=ref.operation_id "
	       "WHERE ref.item_uid=" +
	       value + " AND " + typed_root("op", "inbox") +
	       ") OR "
	       "EXISTS(SELECT 1 FROM economic_accounting_account_effect effect "
	       "LEFT JOIN economic_accounting_operation op ON op.operation_id=effect.operation_id "
	       "LEFT JOIN critical_operation_inbox inbox ON inbox.operation_id=effect.operation_id "
	       "WHERE OCTET_LENGTH(effect.account_key)=40 AND SUBSTRING(effect.account_key,17)=" +
	       hex(pile_suffix(uid)) + " AND " + typed_root("op", "inbox") +
	       ") OR "
	       "EXISTS(SELECT 1 FROM item_ownership_ledger ledger "
	       "JOIN economic_accounting_child child ON child.child_operation_id=ledger.operation_id "
	       "LEFT JOIN economic_accounting_operation op ON op.operation_id=child.operation_id "
	       "LEFT JOIN critical_operation_inbox inbox ON inbox.operation_id=child.operation_id "
	       "WHERE ledger.item_uid=" +
	       value + " AND child.domain_id=" + std::to_string(COIN_TRANSFER_OPERATION_DOMAIN) +
	       "))";
}
constexpr const char *core_tables[] = { "economic_epoch",
					"economic_lineage_state",
					"economic_account_mapping",
					"economic_accounting_operation",
					"economic_accounting_account_effect",
					"economic_accounting_coin_posting",
					"economic_accounting_child",
					"economic_accounting_item_reference",
					"economic_accounting_source_claim" };
constexpr const char *native_tables[] = { "item_current_owner",	   "item_owner_revision",
					  "item_ownership_ledger", "critical_operation_inbox",
					  "critical_outbox",	   "currency_ledger",
					  "season_reset_state" };
bool available(MYSQL *connection)
{
	std::string names;
	for (const auto *name : core_tables)
		names += (names.empty() ? "'" : ",'") + std::string(name) + "'";
	for (const auto *name : native_tables)
		names += ",'" + std::string(name) + "'";
	auto found = rows(connection,
			  "SELECT table_name,engine FROM information_schema.tables "
			  "WHERE table_schema=DATABASE() AND table_name IN (" +
				  names + ") ORDER BY table_name LIMIT 17",
			  2, 17);
	size_t core = 0;
	for (const auto &entry : found)
	{
		require(entry[0].has_value(), EPROTONOSUPPORT);
		for (const auto *name : core_tables)
			core += *entry[0] == name;
	}
	if (!core)
		return false;
	require(core == std::size(core_tables) &&
			found.size() == std::size(core_tables) + std::size(native_tables),
		EPROTONOSUPPORT);
	for (const auto &entry : found)
		require(entry[1] && *entry[1] == "InnoDB", EPROTONOSUPPORT);
	const auto column = one(connection,
				"SELECT data_type,is_nullable FROM information_schema.columns "
				"WHERE table_schema=DATABASE() AND table_name='item_current_owner' "
				"AND column_name='coin_payload' LIMIT 2",
				2);
	require(column[0] && *column[0] == "mediumblob" && column[1] && *column[1] == "YES",
		EPROTONOSUPPORT);
	return true;
}
struct proof
{
	economic_frozen_intent intent;
	economic_accounting_plan plan;
	std::array<size_t, 2> accounts = {}, children = {};
	size_t pile_index = 0, wallet_index = 0;
};
bool same_source(const std::optional<economic_source_event> &left,
		 const std::optional<economic_source_event> &right)
{
	if (left.has_value() != right.has_value())
		return false;
	if (!left)
		return true;
	std::array<uint8_t, ECONOMIC_SOURCE_EVENT_BYTES> a = {}, b = {};
	checked(economic_source_event_encode(*left, &a));
	checked(economic_source_event_encode(*right, &b));
	return a == b;
}
bool same_metadata(const economic_operation_metadata &a, const economic_operation_metadata &b)
{
	return a.version == b.version && a.lineage.bytes == b.lineage.bytes &&
	       a.epoch.bytes == b.epoch.bytes && a.operation_id.bytes == b.operation_id.bytes &&
	       a.original_operation_id.bytes == b.original_operation_id.bytes &&
	       a.actor_kind == b.actor_kind && a.actor_id == b.actor_id &&
	       a.writer_id == b.writer_id && a.policy_version == b.policy_version &&
	       a.compiler_version == b.compiler_version && a.reason == b.reason &&
	       same_source(a.source_event, b.source_event);
}
void ordinary_plan(proof &value, uint64_t uid)
{
	const auto &plan = value.plan;
	const auto &meta = plan.metadata;
	checked(economic_plan_validate_structure(plan));
	require(meta.writer_id == ECONOMIC_WRITER_WALLET_COIN_TRANSFER &&
		meta.reason == economic_reason::coin_transfer &&
		meta.version == ECONOMIC_ACCOUNTING_VERSION && meta.policy_version == 1 &&
		meta.compiler_version == 1 && meta.actor_kind == economic_actor_kind::domain &&
		critical_operation_id_is_zero(meta.original_operation_id) &&
		value.intent.admission.facts_version == 1 &&
		value.intent.admission.facts.size() == ECONOMIC_COIN_TRANSFER_FACT_BYTES &&
		same_metadata(meta, value.intent.admission.metadata) &&
		meta.domain_digest == value.intent.domain_digest && plan.accounts.size() == 2 &&
		plan.children.size() == 2 && plan.postings.size() == 2 &&
		plan.item_events.size() == 1 && plan.items_before.size() == 1 &&
		plan.items_after.size() == 1);
	economic_digest intent_digest = {};
	checked(economic_intent_digest(value.intent, &intent_digest));
	require(intent_digest == meta.intent_digest);
	std::array<bool, 2> seen = {};
	for (size_t index = 0; index < plan.children.size(); ++index)
	{
		const auto &child = plan.children[index];
		require(child.domain == COIN_TRANSFER_OPERATION_DOMAIN && child.discriminator < 2 &&
			!child.parent_index && child.relationship == 1 &&
			!seen[child.discriminator]);
		seen[child.discriminator] = true;
		value.children[child.discriminator] = index;
	}
	seen = {};
	for (const auto &posting : plan.postings)
	{
		require(posting.event_index < 2 && !seen[posting.event_index] &&
			posting.account_index < 2 &&
			posting.child_index == value.children[posting.event_index] + 1);
		seen[posting.event_index] = true;
		value.accounts[posting.event_index] = posting.account_index;
		require(posting.event_index ? posting.copper > 0 : posting.copper < 0);
	}
	require(value.accounts[0] != value.accounts[1]);
	for (size_t index = 0; index < 2; ++index)
	{
		const auto &effect = plan.accounts[value.accounts[index]];
		require(effect.key.lineage.bytes == meta.lineage.bytes && !effect.key.context_id &&
			effect.key.authority_id ==
				little_u64(value.intent.admission.facts, 8 * index) &&
			(effect.key.kind == economic_account_kind::wallet ||
			 effect.key.kind == economic_account_kind::pile));
		for (size_t denomination = 0; denomination < 4; ++denomination)
			require(effect.before[denomination] >= 0 &&
				effect.before[denomination] <= INT_MAX &&
				effect.after[denomination] >= 0 &&
				effect.after[denomination] <= INT_MAX);
	}
	require(meta.actor_id == plan.accounts[value.accounts[0]].key.authority_id);
	const bool drop = plan.accounts[value.accounts[0]].key.kind ==
			  economic_account_kind::wallet;
	value.wallet_index = drop ? 0 : 1;
	value.pile_index = drop ? 1 : 0;
	const auto &pile = plan.accounts[value.accounts[value.pile_index]];
	const auto &wallet = plan.accounts[value.accounts[value.wallet_index]];
	require(pile.key.kind == economic_account_kind::pile && pile.key.authority_id == uid &&
		wallet.key.kind == economic_account_kind::wallet &&
		wallet.before_revision != UINT64_MAX &&
		wallet.after_revision == wallet.before_revision + 1);
	const auto &event = plan.item_events[0];
	require(event.event_index == 0 &&
		event.child_index == value.children[value.pile_index] + 1 && event.uid == uid &&
		plan.items_before[0].uid == uid && plan.items_after[0].uid == uid &&
		economic_item_position_equal(plan.items_before[0].position, event.before) &&
		economic_item_position_equal(plan.items_after[0].position, event.after) &&
		event.after.owner.type == item_owner_type::room && event.after.owner.id > 0 &&
		event.after.owner.id <= INT_MAX && !event.after.owner.context_id &&
		event.after.root_uid == uid && !event.after.parent_uid &&
		!event.after.equipment_slot && event.after.state == item_custody_state::active &&
		pile.after_revision == event.after.revision &&
		event.before.revision != UINT64_MAX &&
		event.after.revision == event.before.revision + 1 &&
		pile.before_revision <= event.before.revision);
	if (drop)
		require(event.before.state == item_custody_state::absent && !pile.before_revision &&
			pile.before == economic_coin_vector{});
	else
	{
		require(pile.before_revision && event.before.state == item_custody_state::active &&
			event.before.root_uid == uid && !event.before.parent_uid &&
			!event.before.equipment_slot &&
			item_owner_identity_equal(event.before.owner, event.after.owner));
		for (size_t index = 0; index < 4; ++index)
			require(pile.after[index] <= pile.before[index]);
	}
	economic_source_event source = {};
	const auto &debit = plan.accounts[value.accounts[0]];
	const bool has_source =
		coin_transfer_accounting_source_event(debit.key.kind, debit.key.authority_id,
						      debit.before_revision, false, drop, &source);
	require(same_source(meta.source_event,
			    has_source ? std::optional<economic_source_event>(source) :
					 std::nullopt));
}

std::pair<critical_operation_id, economic_account_key> current_head(MYSQL *connection, uint64_t uid,
								    uint64_t revision)
{
	const auto selected = one(
		connection,
		"SELECT effect.operation_id,effect.account_key FROM economic_accounting_account_effect effect "
		"JOIN economic_accounting_operation op ON op.operation_id=effect.operation_id "
		"WHERE effect.account_key=CONCAT(op.lineage," +
			hex(pile_suffix(uid)) +
			") AND "
			"effect.after_revision=" +
			std::to_string(revision) +
			" AND op.outcome=1 AND op.result_code=0 "
			"AND NOT EXISTS(SELECT 1 FROM economic_accounting_account_effect newer "
			"JOIN economic_accounting_operation newer_op ON newer_op.operation_id=newer.operation_id "
			"WHERE newer.account_key=effect.account_key AND newer_op.lineage=op.lineage AND newer_op.epoch=op.epoch "
			"AND newer_op.outcome=1 AND newer.after_revision>effect.after_revision) "
			"ORDER BY effect.operation_id LIMIT 2",
		2);
	economic_account_key account;
	checked(economic_account_key_decode(bytes(selected[1]), &account));
	require(account.kind == economic_account_kind::pile && account.authority_id == uid &&
		!account.context_id);
	return { operation_id(selected[0]), account };
}
proof retained_plan(MYSQL *connection, const critical_operation_id &root, uint64_t uid)
{
	const auto stored = one(connection,
				"SELECT OCTET_LENGTH(canonical_intent),LEFT(canonical_intent," +
					std::to_string(ECONOMIC_ACCOUNTING_MAX_INTENT_BYTES + 1) +
					"),OCTET_LENGTH(canonical_plan),LEFT(canonical_plan," +
					std::to_string(ECONOMIC_ACCOUNTING_MAX_PLAN_BYTES + 1) +
					") FROM economic_accounting_operation WHERE operation_id=" +
					id(root) + " LIMIT 2",
				4);
	require(number<size_t>(stored[0]) <= ECONOMIC_ACCOUNTING_MAX_INTENT_BYTES &&
		number<size_t>(stored[2]) <= ECONOMIC_ACCOUNTING_MAX_PLAN_BYTES &&
		number<size_t>(stored[0]) == bytes(stored[1]).size() &&
		number<size_t>(stored[2]) == bytes(stored[3]).size());
	proof value;
	checked(economic_intent_decode(bytes(stored[1]), &value.intent));
	checked(economic_plan_decode(bytes(stored[3]), &value.plan));
	std::vector<uint8_t> intent, plan;
	checked(economic_intent_encode(value.intent, &intent));
	checked(economic_plan_encode(value.plan, &plan));
	require(std::equal(intent.begin(), intent.end(), bytes(stored[1]).begin(),
			   bytes(stored[1]).end()) &&
		std::equal(plan.begin(), plan.end(), bytes(stored[3]).begin(),
			   bytes(stored[3]).end()) &&
		value.plan.metadata.operation_id.bytes == root.bytes);
	ordinary_plan(value, uid);
	return value;
}
void operation_rows(MYSQL *connection, const proof &value)
{
	const auto &root = value.plan.metadata.operation_id;
	std::vector<uint8_t> intent, plan;
	checked(economic_intent_encode(value.intent, &intent));
	checked(economic_plan_encode(value.plan, &plan));
	const auto &meta = value.plan.metadata;
	economic_digest digest = {};
	SHA256(plan.data(), plan.size(), digest.data());
	std::string source = "NULL";
	if (meta.source_event)
	{
		std::array<uint8_t, ECONOMIC_SOURCE_EVENT_BYTES> encoded = {};
		checked(economic_source_event_encode(*meta.source_event, &encoded));
		source = hex(encoded);
	}
	count(connection, "economic_accounting_operation",
	      predicate(
		      { { "operation_id", id(root) },
			{ "lineage", id(meta.lineage) },
			{ "epoch", id(meta.epoch) },
			{ "original_operation_id", "NULL" },
			{ "accounting_version", std::to_string(meta.version) },
			{ "writer_id", std::to_string(meta.writer_id) },
			{ "policy_version", std::to_string(meta.policy_version) },
			{ "compiler_version", std::to_string(meta.compiler_version) },
			{ "actor_kind", std::to_string(static_cast<unsigned>(meta.actor_kind)) },
			{ "actor_id", std::to_string(meta.actor_id) },
			{ "reason", std::to_string(static_cast<unsigned>(meta.reason)) },
			{ "source_event", source },
			{ "intent_digest", hex(meta.intent_digest) },
			{ "domain_digest", hex(meta.domain_digest) },
			{ "plan_digest", hex(digest) },
			{ "canonical_intent", hex(intent) },
			{ "canonical_plan", hex(plan) },
			{ "outcome", "1" },
			{ "result_code", "0" },
			{ "account_count", std::to_string(value.plan.accounts.size()) },
			{ "posting_count", std::to_string(value.plan.postings.size()) },
			{ "child_count", std::to_string(value.plan.children.size()) },
			{ "item_event_count", std::to_string(value.plan.item_events.size()) },
			{ "before_witness_count", std::to_string(value.plan.items_before.size()) },
			{ "after_witness_count", std::to_string(value.plan.items_after.size()) } }),
	      1);
	count(connection, "economic_accounting_operation", "operation_id=" + id(root), 1);
}
void prior_effect(MYSQL *connection, const proof &value)
{
	const auto &effect = value.plan.accounts[value.accounts[value.pile_index]];
	std::array<uint8_t, ECONOMIC_ACCOUNT_KEY_BYTES> key = {};
	checked(economic_account_key_encode(effect.key, &key));
	const uint64_t maximum = value.plan.item_events[0].before.revision;
	const auto selected = rows(
		connection,
		"SELECT effect.after_copper,effect.after_silver,effect.after_gold,effect.after_platinum,effect.after_revision "
		"FROM economic_accounting_account_effect effect JOIN economic_accounting_operation op "
		"ON op.operation_id=effect.operation_id WHERE effect.account_key=" +
			hex(key) + " AND op.lineage=" + id(value.plan.metadata.lineage) +
			" AND op.epoch=" + id(value.plan.metadata.epoch) +
			" AND op.outcome=1 AND op.operation_id<>" +
			id(value.plan.metadata.operation_id) +
			" AND effect.after_revision<=" + std::to_string(maximum) +
			" ORDER BY effect.after_revision DESC,effect.operation_id LIMIT 2",
		5, 2);
	if (!maximum)
	{
		require(selected.empty());
		return;
	}
	require(!selected.empty() && number<uint64_t>(selected[0][4]) == effect.before_revision &&
		(selected.size() < 2 ||
		 number<uint64_t>(selected[1][4]) != effect.before_revision));
	for (size_t index = 0; index < 4; ++index)
		require(number<int64_t>(selected[0][index]) == effect.before[index]);
}
struct receipt
{
	uint16_t payload_version = 0;
	uint64_t revision = 0;
	std::vector<uint8_t> result;
};
receipt inbox(MYSQL *connection, const critical_operation_id &operation, critical_command_type type,
	      uint32_t schema, size_t maximum)
{
	const auto stored = one(
		connection,
		"SELECT command_type,schema_version,payload_version,status,result_code,failure_stage,durable_revision,"
		"OCTET_LENGTH(result_payload),LEFT(result_payload," +
			std::to_string(maximum + 1) +
			"),command_hash,keys_hash FROM critical_operation_inbox WHERE operation_id=" +
			id(operation) + " LIMIT 2",
		11);
	require(number<uint16_t>(stored[0]) == static_cast<uint16_t>(type) &&
		number<uint32_t>(stored[1]) == schema && number<unsigned>(stored[3]) == 1 &&
		!number<unsigned>(stored[4]) && !number<unsigned>(stored[5]) &&
		number<size_t>(stored[7]) == maximum && bytes(stored[8]).size() == maximum &&
		bytes(stored[9]).size() == SHA256_DIGEST_LENGTH &&
		bytes(stored[10]).size() == SHA256_DIGEST_LENGTH &&
		std::any_of(bytes(stored[9]).begin(), bytes(stored[9]).end(),
			    [](uint8_t byte) { return byte != 0; }) &&
		std::any_of(bytes(stored[10]).begin(), bytes(stored[10]).end(),
			    [](uint8_t byte) { return byte != 0; }));
	receipt result;
	result.payload_version = number<uint16_t>(stored[2]);
	result.revision = number<uint64_t>(stored[6]);
	require(result.revision != 0);
	result.result.assign(bytes(stored[8]).begin(), bytes(stored[8]).end());
	return result;
}
void outbox(MYSQL *connection, const critical_operation_id &operation, uint16_t destination,
	    std::span<const uint8_t> result)
{
	const auto where = "operation_id=" + id(operation);
	count(connection, "critical_outbox", where, 1);
	count(connection, "critical_outbox",
	      predicate({ { "operation_id", id(operation) },
			  { "event_index", "0" },
			  { "destination", std::to_string(destination) },
			  { "event_type", "1" },
			  { "payload_version", "1" },
			  { "payload", hex(result) } }),
	      1);
}
void coin_fields(std::vector<std::pair<std::string, std::string>> &fields,
		 const std::string &prefix, const economic_coin_vector &coins)
{
	static constexpr const char *names[] = { "copper", "silver", "gold", "platinum" };
	for (size_t index = 0; index < coins.size(); ++index)
		fields.emplace_back(prefix + names[index], std::to_string(coins[index]));
}
void wallet_ledger(MYSQL *connection, const proof &value, const currency_command_result &wallet)
{
	const auto &effect = value.plan.accounts[value.accounts[value.wallet_index]];
	const auto &child = value.plan.children[value.children[value.wallet_index]].operation_id;
	const auto native =
		one(connection,
		    "SELECT pid,bank_id,source_site FROM currency_ledger WHERE operation_id=" +
			    id(child) + " LIMIT 2",
		    3);
	const auto pid = number<uint32_t>(native[0]);
	const auto bank = number<uint32_t>(native[1]);
	require(pid && pid <= INT_MAX && bank &&
		number<unsigned>(native[2]) ==
			static_cast<unsigned>(critical_source_site::command));
	// Original retained verification matches historical immutable mapping fields;
	// neither active_native_id nor today's wallet/bank balance/revision is required.
	count(connection, "economic_account_mapping",
	      predicate({ { "mapping_id", std::to_string(effect.key.authority_id) },
			  { "lineage", id(effect.key.lineage) },
			  { "account_kind", "1" },
			  { "context_id", "0" },
			  { "backend_kind", "1" },
			  { "locator_kind", "1" },
			  { "native_id", std::to_string(pid) } }),
	      1);
	std::vector<std::pair<std::string, std::string>> fields = {
		{ "operation_id", id(child) },
		{ "pid", std::to_string(pid) },
		{ "bank_id", std::to_string(bank) },
		{ "wallet_revision", std::to_string(wallet.wallet_revision) },
		{ "bank_revision", std::to_string(wallet.bank_revision) },
		{ "reason_type",
		  std::to_string(static_cast<unsigned>(currency_reason_type::coin_transfer)) },
		{ "reason_id", "0" },
		{ "source_site",
		  std::to_string(static_cast<unsigned>(critical_source_site::command)) }
	};
	economic_coin_vector delta = {};
	checked(economic_coin_delta(effect.before, effect.after, &delta));
	coin_fields(fields, "wallet_delta_", delta);
	coin_fields(fields, "bank_delta_", {});
	coin_fields(fields, "wallet_after_", wallet.wallet.amount);
	coin_fields(fields, "bank_after_", wallet.bank.amount);
	count(connection, "currency_ledger", predicate(fields), 1);
	count(connection, "currency_ledger", "operation_id=" + id(child), 1);
	count(connection, "item_ownership_ledger", "operation_id=" + id(child), 0);
}
void pile_ledger(MYSQL *connection, const proof &value, const item_transfer_result &pile)
{
	const auto &event = value.plan.item_events[0];
	const auto &child = value.plan.children[value.children[value.pile_index]].operation_id;
	const auto &root = value.plan.metadata.operation_id;
	const bool drop = value.pile_index == 1;
	const auto &room = event.after.owner;
	const item_owner_identity from =
		drop ? item_owner_identity{ item_owner_type::system, 0, 0 } : room;
	require(pile.root_item_uid == event.uid && pile.item_count == 1 &&
		pile.max_item_revision == event.after.revision && !pile.corpse_revision &&
		!pile.collector_catalog_changed && pile.from_owner_revision &&
		pile.to_owner_revision &&
		(drop || pile.from_owner_revision == pile.to_owner_revision));
	const auto reason = drop ? item_transfer_reason::creation :
				   item_transfer_reason::player_put;
	std::vector<std::pair<std::string, std::string>> fields = {
		{ "operation_id", id(child) },
		{ "event_index", "0" },
		{ "item_uid", std::to_string(event.uid) },
		{ "root_item_uid", std::to_string(event.after.root_uid) },
		{ "parent_item_uid", "NULL" },
		{ "from_owner_type", std::to_string(static_cast<unsigned>(from.type)) },
		{ "from_owner_id", std::to_string(from.id) },
		{ "from_owner_context_id", "0" },
		{ "to_owner_type", std::to_string(static_cast<unsigned>(room.type)) },
		{ "to_owner_id", std::to_string(room.id) },
		{ "to_owner_context_id", "0" },
		{ "item_revision", std::to_string(event.after.revision) },
		{ "from_owner_revision", std::to_string(pile.from_owner_revision) },
		{ "to_owner_revision", std::to_string(pile.to_owner_revision) },
		{ "reason_type", std::to_string(static_cast<unsigned>(reason)) },
		{ "reason_id", "0" },
		{ "source_site",
		  std::to_string(static_cast<unsigned>(critical_source_site::command)) },
		{ "from_equipment_slot", "0" },
		{ "to_equipment_slot", "0" }
	};
	count(connection, "item_ownership_ledger", predicate(fields), 1);
	count(connection, "item_ownership_ledger", "operation_id=" + id(child), 1);
	count(connection, "currency_ledger", "operation_id=" + id(child), 0);
	economic_accounting_item_reference ref = {};
	ref.operation_id = root;
	ref.line_index = static_cast<uint16_t>(event.event_index);
	ref.event_index = event.event_index;
	ref.child_index = event.child_index;
	ref.item_uid = event.uid;
	ref.before_revision = event.before.revision;
	ref.after_revision = event.after.revision;
	ref.legacy_operation_id = child;
	ref.legacy_event_index = 0;
	require(economic_accounting_item_reference_validate(ref));
	count(connection, "economic_accounting_item_reference",
	      predicate({ { "operation_id", id(root) },
			  { "line_index", std::to_string(ref.line_index) },
			  { "event_index", std::to_string(ref.event_index) },
			  { "child_index", std::to_string(ref.child_index) },
			  { "item_uid", std::to_string(ref.item_uid) },
			  { "before_revision", std::to_string(ref.before_revision) },
			  { "after_revision", std::to_string(ref.after_revision) },
			  { "legacy_operation_id", id(child) },
			  { "legacy_event_index", "0" } }),
	      1);
}
void retained_receipts(MYSQL *connection, const proof &value, sql_room_coin_pile &output)
{
	const auto &root = value.plan.metadata.operation_id;
	const auto retained = inbox(connection, root, critical_command_type::coin_transfer,
				    CRITICAL_COMMAND_ACCOUNTING_SCHEMA_VERSION,
				    COIN_TRANSFER_RESULT_BYTES);
	require(retained.payload_version == COIN_TRANSFER_PAYLOAD_VERSION);
	std::array<uint8_t, CRITICAL_OUTBOX_COIN_RECEIPT_BYTES> audit = {};
	uint64_t revision = 0;
	for (size_t index = 0; index < 2; ++index)
	{
		const auto &child = value.plan.children[value.children[index]].operation_id;
		std::copy(child.bytes.begin(), child.bytes.end(),
			  audit.begin() + index * CRITICAL_COMMAND_ID_BYTES);
		const size_t offset =
			index * (CURRENCY_RESULT_PAYLOAD_BYTES + ITEM_TRANSFER_RESULT_BYTES);
		const auto &effect = value.plan.accounts[value.accounts[index]];
		if (index == value.wallet_index)
		{
			currency_command_result result = {};
			std::array<uint8_t, CURRENCY_RESULT_PAYLOAD_BYTES> canonical = {};
			require(currency_command_decode_result(retained.result.data() + offset,
							       canonical.size(), &result) &&
				currency_command_encode_result(result, &canonical) &&
				std::equal(canonical.begin(), canonical.end(),
					   retained.result.begin() + offset) &&
				std::all_of(retained.result.begin() + offset +
						    CURRENCY_RESULT_PAYLOAD_BYTES,
					    retained.result.begin() + offset +
						    CURRENCY_RESULT_PAYLOAD_BYTES +
						    ITEM_TRANSFER_RESULT_BYTES,
					    [](uint8_t byte) { return byte == 0; }) &&
				result.wallet.amount == effect.after &&
				result.wallet_revision == effect.after_revision &&
				result.bank_revision);
			for (int64_t amount : result.bank.amount)
				require(amount >= 0 && amount <= INT_MAX);
			const auto child_receipt =
				inbox(connection, child, critical_command_type::account_bank,
				      CRITICAL_COMMAND_SCHEMA_VERSION, canonical.size());
			const auto child_revision =
				std::max(result.wallet_revision, result.bank_revision);
			require(child_receipt.payload_version == CURRENCY_COMMAND_PAYLOAD_VERSION &&
				child_receipt.revision == child_revision &&
				std::equal(canonical.begin(), canonical.end(),
					   child_receipt.result.begin(),
					   child_receipt.result.end()));
			revision = std::max(revision, child_revision);
			wallet_ledger(connection, value, result);
			outbox(connection, child, 3, canonical);
		}
		else
		{
			item_transfer_result result = {};
			std::array<uint8_t, ITEM_TRANSFER_RESULT_BYTES> canonical = {};
			require(item_transfer_command_decode_result(
					retained.result.data() + offset +
						CURRENCY_RESULT_PAYLOAD_BYTES,
					canonical.size(), &result) &&
				item_transfer_command_encode_result(result, &canonical) &&
				std::equal(canonical.begin(), canonical.end(),
					   retained.result.begin() + offset +
						   CURRENCY_RESULT_PAYLOAD_BYTES) &&
				std::all_of(retained.result.begin() + offset,
					    retained.result.begin() + offset +
						    CURRENCY_RESULT_PAYLOAD_BYTES,
					    [](uint8_t byte) { return byte == 0; }));
			const auto child_receipt =
				inbox(connection, child, critical_command_type::item_transfer,
				      CRITICAL_COMMAND_SCHEMA_VERSION, canonical.size());
			const auto child_revision =
				std::max({ result.max_item_revision, result.from_owner_revision,
					   result.to_owner_revision });
			require(child_receipt.payload_version >=
					ITEM_TRANSFER_EXACT_PAYLOAD_VERSION &&
				child_receipt.payload_version <= ITEM_TRANSFER_PAYLOAD_VERSION &&
				child_receipt.revision == child_revision &&
				std::equal(canonical.begin(), canonical.end(),
					   child_receipt.result.begin(),
					   child_receipt.result.end()));
			revision = std::max(revision, child_revision);
			pile_ledger(connection, value, result);
			outbox(connection, child, 4, canonical);
			output.retained_pile_result = result;
			output.pile_child_operation_id = child;
		}
	}
	require(retained.revision == revision);
	outbox(connection, root, CRITICAL_OUTBOX_COIN_RECEIPT_DESTINATION, audit);
	count(connection, "currency_ledger", "operation_id=" + id(root), 0);
	output.retained_root_revision = retained.revision;
}

void native_pile(MYSQL *connection, uint64_t uid, uint64_t room, uint64_t expected_revision,
		 const proof &value, sql_room_coin_pile &output)
{
	const auto owner =
		one(connection,
		    "SELECT revision FROM item_owner_revision WHERE owner_type=3 AND owner_id=" +
			    std::to_string(room) + " AND owner_context_id=0 LIMIT 2 FOR UPDATE",
		    1);
	const auto owner_revision = number<uint64_t>(owner[0]);
	require(owner_revision != 0);
	const auto stored = one(
		connection,
		"SELECT item_uid,root_item_uid,COALESCE(parent_item_uid,0),owner_type,owner_id,owner_context_id,"
		"item_revision,vnum,state,equipment_slot,OCTET_LENGTH(coin_payload),LEFT(coin_payload," +
			std::to_string(ITEM_TRANSFER_ITEM_BLOB_MAX_BYTES + 1) +
			") FROM item_current_owner WHERE item_uid=" + std::to_string(uid) +
			" OR root_item_uid=" + std::to_string(uid) + " OR parent_item_uid=" +
			std::to_string(uid) + " ORDER BY item_uid LIMIT 2 FOR UPDATE",
		12);
	item_ownership_runtime_entry identity = {};
	identity.item_uid = number<uint64_t>(stored[0]);
	identity.root_item_uid = number<uint64_t>(stored[1]);
	identity.parent_item_uid = number<uint64_t>(stored[2]);
	identity.owner = { static_cast<item_owner_type>(number<uint8_t>(stored[3])),
			   number<uint64_t>(stored[4]), number<uint64_t>(stored[5]) };
	identity.item_revision = number<uint64_t>(stored[6]);
	identity.owner_revision = owner_revision;
	identity.vnum = number<int32_t>(stored[7]);
	identity.state = static_cast<item_custody_state>(number<uint8_t>(stored[8]));
	require(identity.item_uid == uid && identity.root_item_uid == uid &&
		!identity.parent_item_uid && identity.owner.type == item_owner_type::room &&
		identity.owner.id == room && !identity.owner.context_id &&
		identity.item_revision == expected_revision && identity.vnum > 0 &&
		identity.state == item_custody_state::active && !number<unsigned>(stored[9]) &&
		number<size_t>(stored[10]) > 0 &&
		number<size_t>(stored[10]) <= ITEM_TRANSFER_ITEM_BLOB_MAX_BYTES &&
		bytes(stored[11]).size() == number<size_t>(stored[10]));
	std::vector<player_item_snapshot> decoded;
	require(player_item_snapshot_list_decode(bytes(stored[11]).data(), bytes(stored[11]).size(),
						 &decoded) == player_snapshot_codec_result::ok &&
		decoded.size() == 1);
	const auto &literal = decoded[0];
	require(literal.object_uid == uid && literal.vnum == identity.vnum &&
		literal.type == ITEM_MONEY && literal.parent_index == PLAYER_SNAPSHOT_NO_PARENT &&
		literal.equipment_slot == -1 &&
		literal.string_mask == (STRUNG_KEYS | STRUNG_DESC1 | STRUNG_DESC2 | STRUNG_DESC3) &&
		literal.dynamic_affects.empty() && literal.extra_descriptions.size() <= 1 &&
		!(literal.extra_flags &
		  (ITEM_LIT | ITEM_TRANSIENT | ITEM_ARTIFACT | ITEM_PROCLIB)));
	const auto &effect = value.plan.accounts[value.accounts[value.pile_index]];
	bool nonempty = false;
	for (size_t index = 0; index < 4; ++index)
	{
		require(literal.values[index] >= 0 && literal.values[index] == effect.after[index]);
		nonempty = nonempty || literal.values[index] != 0;
	}
	require(nonempty && effect.after_revision == identity.item_revision);
	const auto &after = value.plan.item_events[0].after;
	require(item_owner_identity_equal(identity.owner, after.owner) &&
		identity.root_item_uid == after.root_uid &&
		identity.parent_item_uid == after.parent_uid &&
		identity.item_revision == after.revision && identity.state == after.state);
	std::vector<uint8_t> canonical;
	require(player_item_snapshot_list_encode(decoded, &canonical) ==
			player_snapshot_codec_result::ok &&
		std::equal(canonical.begin(), canonical.end(), bytes(stored[11]).begin(),
			   bytes(stored[11]).end()));
	output.identity = identity;
	output.item = std::move(decoded[0]);
}
#endif
}

bool sql_room_coin_payload_available(MYSQL *connection, bool *output)
{
#ifdef __NO_MYSQL__
	(void)connection;
	(void)output;
	return refuse(ENOTSUP);
#else
	try
	{
		require(connection && output, EINVAL);
		const auto original = mysql_thread_id(connection);
		require(session(connection, original, false), ENOTCONN);
		const bool candidate = available(connection);
		require(session(connection, original, false), ENOTCONN);
		*output = candidate;
		return true;
	}
	catch (const failure &error)
	{
		return refuse(error.code);
	}
	catch (const std::bad_alloc &)
	{
		return refuse(ENOMEM);
	}
#endif
}

bool sql_room_coin_payload_roots(MYSQL *connection, std::vector<uint64_t> *output)
{
#ifdef __NO_MYSQL__
	(void)connection;
	(void)output;
	return refuse(ENOTSUP);
#else
	try
	{
		require(connection && output, EINVAL);
		const auto original = mysql_thread_id(connection);
		require(session(connection, original, true), ENOTCONN);
		require(available(connection), EPROTONOSUPPORT);
		uint64_t season = 0;
		const bool season_locked = sql_room_item_payload_lock_season(connection, &season);
		const int season_error = errno;
		require(season_locked, season_error ? season_error : ESTALE);
		const auto selected = rows(
			connection,
			"SELECT candidate.item_uid FROM item_current_owner candidate WHERE candidate.owner_type=3 AND "
			"candidate.state=1 AND (candidate.coin_payload IS NOT NULL OR "
			"EXISTS(SELECT 1 FROM economic_accounting_item_reference ref "
			"LEFT JOIN economic_accounting_operation op ON op.operation_id=ref.operation_id "
			"LEFT JOIN critical_operation_inbox inbox ON inbox.operation_id=ref.operation_id "
			"WHERE ref.item_uid=candidate.item_uid AND " +
				typed_root("op", "inbox") +
				")) ORDER BY candidate.item_uid LIMIT " +
				std::to_string(SQL_ROOM_COIN_ROOT_MAX + 1),
			1, SQL_ROOM_COIN_ROOT_MAX + 1);
		require(selected.size() <= SQL_ROOM_COIN_ROOT_MAX, E2BIG);
		std::vector<uint64_t> candidate;
		candidate.reserve(selected.size());
		for (const auto &entry : selected)
		{
			const auto uid = number<uint64_t>(entry[0]);
			require(uid && uid != UINT64_MAX &&
				(candidate.empty() || uid > candidate.back()));
			candidate.push_back(uid);
		}
		require(session(connection, original, true), ENOTCONN);
		*output = std::move(candidate);
		return true;
	}
	catch (const failure &error)
	{
		return refuse(error.code);
	}
	catch (const std::bad_alloc &)
	{
		return refuse(ENOMEM);
	}
#endif
}

bool sql_room_coin_payload_read(MYSQL *connection, uint64_t uid, sql_room_coin_pile *output)
{
#ifdef __NO_MYSQL__
	(void)connection;
	(void)uid;
	(void)output;
	return refuse(ENOTSUP);
#else
	try
	{
		require(connection && output && uid && uid != UINT64_MAX, EINVAL);
		const auto original = mysql_thread_id(connection);
		require(session(connection, original, true), ENOTCONN);
		require(available(connection), EPROTONOSUPPORT);
		sql_room_coin_pile candidate;
		candidate.session_id = original;
		// Nonlocking locator/receipt hints are not authority. A moved/revised row
		// refuses after the owner-first current read; this reader never retries.
		const auto hint = one(
			connection,
			"SELECT owner_id,item_revision FROM item_current_owner WHERE item_uid=" +
				std::to_string(uid) + " AND owner_type=3 AND state=1 LIMIT 2",
			2);
		const auto room = number<uint64_t>(hint[0]), revision = number<uint64_t>(hint[1]);
		require(room && room <= INT_MAX && revision);
		const auto head = current_head(connection, uid, revision);
		const auto value = retained_plan(connection, head.first, uid);
		require(head.second.lineage.bytes == value.plan.metadata.lineage.bytes);
		candidate.lineage = value.plan.metadata.lineage;
		candidate.epoch = value.plan.metadata.epoch;
		candidate.root_operation_id = head.first;
		// This uses retained native current lineage authority, not admission flags.
		// Empty mapping requests deliberately do not lock a historical wallet.
		economic_sql_authority_snapshot authority;
		const auto authority_error = economic_sql_lock_authority(
			connection, candidate.lineage, candidate.epoch, {}, &authority);
		require(!authority_error, authority_error);
		require(session(connection, original, true), ENOTCONN);
		const bool season_locked =
			sql_room_item_payload_lock_season(connection, &candidate.season_epoch);
		const int season_error = errno;
		require(season_locked, season_error ? season_error : ESTALE);
		native_pile(connection, uid, room, revision, value, candidate);
		const auto fresh_head =
			current_head(connection, uid, candidate.identity.item_revision);
		require(fresh_head.first.bytes == head.first.bytes &&
			economic_account_key_equal(fresh_head.second, head.second));
		operation_rows(connection, value);
		prior_effect(connection, value);
		const auto indexed_error =
			coin_transfer_accounting_verify_indexed_plan(connection, value.plan);
		require(!indexed_error, indexed_error);
		retained_receipts(connection, value, candidate);
		// Unrelated items can advance the room counter after this historical coin
		// receipt. Hydrate current locked authority, never infer it from receipt.
		require(candidate.identity.owner_revision >=
			candidate.retained_pile_result.to_owner_revision);
		require(session(connection, original, true), ENOTCONN);
		*output = std::move(candidate);
		return true;
	}
	catch (const failure &error)
	{
		return refuse(error.code);
	}
	catch (const std::bad_alloc &)
	{
		return refuse(ENOMEM);
	}
#endif
}

bool sql_room_coin_payload_present(MYSQL *connection, uint64_t uid, bool *output)
{
#ifdef __NO_MYSQL__
	(void)connection;
	(void)uid;
	(void)output;
	return refuse(ENOTSUP);
#else
	try
	{
		require(connection && output && uid && uid != UINT64_MAX, EINVAL);
		const auto original = mysql_thread_id(connection);
		require(session(connection, original, false), ENOTCONN);
		require(available(connection), EPROTONOSUPPORT);
		const auto selected = one(connection, "SELECT " + history(uid), 1);
		const auto present = number<unsigned>(selected[0]);
		require(present <= 1 && session(connection, original, false), ENOTCONN);
		*output = present != 0;
		return true;
	}
	catch (const failure &error)
	{
		return refuse(error.code);
	}
	catch (const std::bad_alloc &)
	{
		return refuse(ENOMEM);
	}
#endif
}
