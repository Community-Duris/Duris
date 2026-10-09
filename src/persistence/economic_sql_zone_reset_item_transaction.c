#include "persistence/economic_sql_zone_reset_item_transaction.h"
#include "persistence/sql_room_item_payload.h"
#include "persistence/zone_reset_item_origin_sql.h"
#include "persistence/economic_accounting_repository.h"
#include "item/economic_accounting_item_reference.h"
#include "player/player_snapshot_codec.h"
#include "core/defines.h"
#include "world/vnum.obj.h"
#include <algorithm>
#include <array>
#include <cerrno>
#include <charconv>
#include <new>
#include <optional>
#include <openssl/sha.h>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>
#include <vector>
namespace
{
struct failure
{
	unsigned int code;
};
void require(bool condition, unsigned int code = EILSEQ)
{
	if (!condition)
		throw failure{ code ? code : EIO };
}
void checked(economic_accounting_error error)
{
	require(error == economic_accounting_error::ok,
		error == economic_accounting_error::capacity ? ENOMEM : EINVAL);
}
#ifndef __NO_MYSQL__
void participant(bool okay)
{
	if (!okay)
	{
		const auto error = errno;
		throw failure{ error > 0 ? static_cast<unsigned int>(error) : EIO };
	}
}
#endif
struct reset_identity
{
	zone_reset_item_image image;
	economic_frozen_intent intent;
};
reset_identity decode(const critical_command &command)
{
	reset_identity value;
	checked(zone_reset_item_command_decode(command, &value.image));
	checked(economic_intent_decode(command.accounting_intent, &value.intent));
	return value;
}
#ifndef __NO_MYSQL__
item_transfer_result expected_result(const zone_reset_item_image &image)
{
	require(!image.items.empty() && image.items.size() <= UINT16_MAX &&
			image.expected_room_revision != UINT64_MAX,
		EINVAL);
	item_transfer_result result{};
	result.root_item_uid = image.items.front().object_uid;
	result.item_count = static_cast<uint16_t>(image.items.size());
	result.to_owner_revision = image.expected_room_revision + 1;
	result.max_item_revision = 1;
	return result;
}
#endif
}
bool economic_sql_zone_reset_item_command_supported(const critical_command &command) noexcept
{
	try
	{
		(void)decode(command);
		return true;
	}
	catch (...)
	{
		return false;
	}
}
#ifdef __NO_MYSQL__
struct economic_sql_zone_reset_item_transaction::implementation
{
	item_transfer_result result{};
};
economic_sql_zone_reset_item_transaction::economic_sql_zone_reset_item_transaction(
	std::unique_ptr<implementation> state)
	: state_(std::move(state))
{
}
economic_sql_zone_reset_item_transaction::~economic_sql_zone_reset_item_transaction() = default;
unsigned int economic_sql_zone_reset_item_transaction::prepare(
	MYSQL *, const critical_command &,
	std::unique_ptr<economic_sql_zone_reset_item_transaction> *)
{
	return ENOTSUP;
}
unsigned int economic_sql_zone_reset_item_transaction::apply()
{
	return ENOTSUP;
}
unsigned int economic_sql_zone_reset_item_transaction::finalize()
{
	return ENOTSUP;
}
unsigned int economic_sql_zone_reset_item_transaction::verify_root_completion()
{
	return ENOTSUP;
}
const item_transfer_result &economic_sql_zone_reset_item_transaction::result() const
{
	return state_->result;
}
unsigned int economic_sql_zone_reset_item_transaction::result_code() const
{
	return ENOTSUP;
}
unsigned int economic_sql_zone_reset_item_verify_retained(MYSQL *, const critical_command &,
							  unsigned int,
							  std::span<const uint8_t>) noexcept
{
	return ENOTSUP;
}
#else
namespace
{
using cells = std::vector<std::optional<std::string>>;
using fields = std::vector<std::pair<std::string, std::string>>;
void active(MYSQL *connection, unsigned long session)
{
	require(connection && session && (connection->server_status & SERVER_STATUS_IN_TRANS) &&
			mysql_thread_id(connection) == session,
		ENOTCONN);
	using flag = std::remove_pointer_t<decltype(MYSQL_BIND{}.is_null)>;
	flag reconnect = false;
	require(!mysql_get_option(connection, MYSQL_OPT_RECONNECT, &reconnect) && !reconnect,
		EPERM);
}
std::string hex(std::span<const uint8_t> bytes)
{
	static constexpr char digits[] = "0123456789abcdef";
	std::string result = "X'";
	result.reserve(bytes.size() * 2 + 3);
	for (uint8_t value : bytes)
	{
		result += digits[value >> 4];
		result += digits[value & 15];
	}
	result += '\'';
	return result;
}
std::string id(const critical_operation_id &value)
{
	return hex(value.bytes);
}
economic_digest hash(std::span<const uint8_t> bytes)
{
	economic_digest result{};
	require(SHA256(bytes.data(), bytes.size(), result.data()) != nullptr, EIO);
	return result;
}
void execute(MYSQL *connection, const std::string &sql)
{
	if (mysql_real_query(connection, sql.data(), sql.size()))
		throw failure{ mysql_errno(connection) ? mysql_errno(connection) : EIO };
}
cells read(MYSQL *connection, const std::string &sql, size_t columns)
{
	execute(connection, sql);
	std::unique_ptr<MYSQL_RES, decltype(&mysql_free_result)> result(
		mysql_store_result(connection), mysql_free_result);
	require(bool(result), mysql_errno(connection) ? mysql_errno(connection) : EIO);
	require(mysql_num_rows(result.get()) == 1 && mysql_num_fields(result.get()) == columns);
	auto row = mysql_fetch_row(result.get());
	auto lengths = mysql_fetch_lengths(result.get());
	require(row && lengths);
	cells values;
	values.reserve(columns);
	for (size_t index = 0; index < columns; ++index)
	{
		require(lengths[index] <= ECONOMIC_ACCOUNTING_MAX_PLAN_BYTES, E2BIG);
		values.push_back(row[index] ? std::optional<std::string>(
						      std::string(row[index], lengths[index])) :
					      std::nullopt);
	}
	return values;
}
template <typename T> T integer(const std::optional<std::string> &cell)
{
	require(cell.has_value());
	T value = 0;
	const auto parsed = std::from_chars(cell->data(), cell->data() + cell->size(), value);
	require(parsed.ec == std::errc{} && parsed.ptr == cell->data() + cell->size());
	return value;
}
void empty(MYSQL *connection, const std::string &sql)
{
	constexpr std::string_view suffix = " FOR UPDATE";
	require(sql.ends_with(suffix), EINVAL);
	execute(connection, sql.substr(0, sql.size() - suffix.size()) + " LIMIT 1 FOR UPDATE");
	std::unique_ptr<MYSQL_RES, decltype(&mysql_free_result)> result(
		mysql_store_result(connection), mysql_free_result);
	require(bool(result), mysql_errno(connection) ? mysql_errno(connection) : EIO);
	require(mysql_num_fields(result.get()) == 1 && mysql_num_rows(result.get()) == 0, EEXIST);
}
std::string predicate(const fields &values)
{
	std::string result;
	for (const auto &[name, value] : values)
	{
		if (!result.empty())
			result += " AND ";
		result += name + " <=> " + value;
	}
	return result;
}
void count(MYSQL *connection, const std::string &table, const std::string &where, uint64_t expected)
{
	require(integer<uint64_t>(read(connection,
				       "SELECT COUNT(*) FROM " + table + " WHERE " + where,
				       1)[0]) == expected);
}
void insert(MYSQL *connection, const std::string &table, const fields &values)
{
	std::string names, data;
	for (const auto &[name, value] : values)
	{
		if (!names.empty())
		{
			names += ',';
			data += ',';
		}
		names += name;
		data += value;
	}
	execute(connection, "INSERT INTO " + table + "(" + names + ") VALUES(" + data + ")");
	require(mysql_affected_rows(connection) == 1, EIO);
}
void coins(fields &values, const std::string &prefix, const economic_coin_vector &amount)
{
	constexpr std::array<const char *, 4> names = { "copper", "silver", "gold", "platinum" };
	for (size_t index = 0; index < names.size(); ++index)
		values.emplace_back(prefix + names[index], std::to_string(amount[index]));
}
std::string source(const economic_operation_metadata &metadata)
{
	require(metadata.source_event.has_value());
	std::array<uint8_t, ECONOMIC_SOURCE_EVENT_BYTES> bytes{};
	checked(economic_source_event_encode(*metadata.source_event, &bytes));
	return hex(bytes);
}
void inbox(MYSQL *connection, const critical_command &command, bool pending)
{
	std::vector<uint8_t> encoded, keys;
	require(critical_command_encode(command, &encoded) == critical_command_codec_result::ok,
		EINVAL);
	keys.reserve(command.keys.size() * 9);
	for (const auto &key : command.keys)
	{
		keys.push_back(static_cast<uint8_t>(key.type));
		for (size_t byte = 0; byte < 8; ++byte)
			keys.push_back(static_cast<uint8_t>(key.id >> (8 * byte)));
	}
	const fields expected = { { "operation_id", id(command.operation_id) },
				  { "command_hash", hex(hash(encoded)) },
				  { "keys_hash", hex(hash(keys)) },
				  { "command_type",
				    std::to_string(static_cast<uint16_t>(command.type)) },
				  { "schema_version", std::to_string(command.schema_version) },
				  { "payload_version", std::to_string(command.payload_version) },
				  { "status", pending ? "0" : "1" },
				  { "failure_stage", "0" } };
	require(read(connection,
		     "SELECT operation_id FROM critical_operation_inbox WHERE " +
			     predicate(expected) + " FOR UPDATE",
		     1)[0]
			.has_value());
}
std::string uid_list(std::span<const uint64_t> values)
{
	std::string result;
	for (uint64_t value : values)
	{
		if (!result.empty())
			result += ',';
		result += std::to_string(value);
	}
	return result;
}
void physical_absence(MYSQL *connection, std::span<const uint64_t> values)
{
	for (uint64_t uid : values)
	{
		const auto literal = std::to_string(uid);
		for (const char *table :
		     { "player_items", "shopkeeper_items", "player_pet_items", "locker_items",
		       "account_locker_items", "corpse_items", "saved_items", "siege_items" })
			empty(connection, "SELECT id FROM " + std::string(table) +
						  " WHERE obj_uid=" + literal + " FOR UPDATE");
		for (const char *table : { "auction_item_custody", "collector_listings",
					   "player_death_restitution_runtime" })
			empty(connection, "SELECT item_uid FROM " + std::string(table) +
						  " WHERE item_uid=" + literal + " FOR UPDATE");
	}
}

std::string owner(const zone_reset_item_image &image)
{
	return "owner_type=3 AND owner_id=" + std::to_string(image.room_vnum) +
	       " AND owner_context_id=0";
}
std::vector<uint64_t> uids(const zone_reset_item_image &image)
{
	std::vector<uint64_t> result;
	result.reserve(image.items.size());
	for (const auto &item : image.items)
		result.push_back(item.object_uid);
	std::sort(result.begin(), result.end());
	return result;
}
void original_absence(MYSQL *connection, const critical_command &command,
		      const reset_identity &identity, std::span<const uint64_t> values)
{
	const auto &meta = identity.intent.admission.metadata;
	empty(connection,
	      "SELECT operation_id FROM economic_accounting_source_claim WHERE lineage=" +
		      id(meta.lineage) + " AND source_event=" + source(meta) + " FOR UPDATE");
	const auto list = uid_list(values);
	empty(connection, "SELECT item_uid FROM item_current_owner WHERE item_uid IN(" + list +
				  ") OR root_item_uid IN(" + list + ") OR parent_item_uid IN(" +
				  list + ") ORDER BY item_uid FOR UPDATE");
	for (uint64_t uid : values)
	{
		for (const char *table :
		     { "item_ownership_baseline", "item_ownership_ledger",
		       "economic_accounting_item_reference", "item_ownership_quarantine",
		       "auction_reconciliation_quarantine", "collector_reconciliation_quarantine",
		       "player_death_restitution_item", "player_death_restitution_delivery",
		       "sql_room_item_payload" })
			empty(connection, "SELECT item_uid FROM " + std::string(table) +
						  " WHERE item_uid=" + std::to_string(uid) +
						  " FOR UPDATE");
	}
	physical_absence(connection, values);
	// Pile identity is the actual new UID, not a wallet/mapping lifetime.
	// Lock its entire indexed history so neither older epochs nor malformed
	// orphan effects can masquerade as an absent account at revision0.
	for (uint64_t uid : values)
		for (const auto &coin : identity.image.coins)
			if (coin.item_uid == uid)
			{
				const economic_account_key key{ meta.lineage,
								economic_account_kind::pile, uid,
								0 };
				std::array<uint8_t, ECONOMIC_ACCOUNT_KEY_BYTES> encoded{};
				checked(economic_account_key_encode(key, &encoded));
				empty(connection,
				      "SELECT operation_id FROM economic_accounting_account_effect WHERE account_key=" +
					      hex(encoded) + " FOR UPDATE");
			}
	for (const char *table :
	     { "economic_accounting_operation", "economic_accounting_account_effect",
	       "economic_accounting_coin_posting", "economic_accounting_item_reference",
	       "economic_accounting_source_claim", "economic_accounting_child",
	       "item_ownership_ledger", "critical_outbox" })
		empty(connection, "SELECT operation_id FROM " + std::string(table) +
					  " WHERE operation_id=" + id(command.operation_id) +
					  " FOR UPDATE");
}
fields item_row(const economic_item_event &event, const player_item_snapshot &item,
		std::span<const uint8_t> literal)
{
	const auto &after = event.after;
	return { { "item_uid", std::to_string(event.uid) },
		 { "root_item_uid", std::to_string(after.root_uid) },
		 { "parent_item_uid",
		   after.parent_uid ? std::to_string(after.parent_uid) : "NULL" },
		 { "owner_type", "3" },
		 { "owner_id", std::to_string(after.owner.id) },
		 { "owner_context_id", "0" },
		 { "item_revision", "1" },
		 { "vnum", std::to_string(item.vnum) },
		 { "state", "1" },
		 { "equipment_slot", "0" },
		 { "coin_payload", item.type == ITEM_MONEY ? hex(literal) : "NULL" } };
}
fields ledger(const critical_operation_id &root, critical_source_site source_site,
	      uint64_t owner_revision, const economic_item_event &event)
{
	return { { "operation_id", id(root) },
		 { "event_index", std::to_string(event.event_index) },
		 { "item_uid", std::to_string(event.uid) },
		 { "root_item_uid", std::to_string(event.after.root_uid) },
		 { "parent_item_uid",
		   event.after.parent_uid ? std::to_string(event.after.parent_uid) : "NULL" },
		 { "from_owner_type", "0" },
		 { "from_owner_id", "0" },
		 { "from_owner_context_id", "0" },
		 { "to_owner_type", "3" },
		 { "to_owner_id", std::to_string(event.after.owner.id) },
		 { "to_owner_context_id", "0" },
		 { "item_revision", "1" },
		 { "from_owner_revision", "0" },
		 { "to_owner_revision", std::to_string(owner_revision) },
		 { "reason_type", "2" },
		 { "reason_id", "0" },
		 { "source_site", std::to_string(static_cast<uint16_t>(source_site)) },
		 { "from_equipment_slot", "0" },
		 { "to_equipment_slot", "0" } };
}

void verify_born_money(MYSQL *connection, const reset_identity &identity,
		       const economic_accounting_plan &plan,
		       const sql_room_item_payload_batch &batch)
{
	require(batch.payloads.size() == identity.image.items.size(), EILSEQ);
	checked(economic_coin_effects_validate(plan.accounts, plan.postings, plan.children.size()));
	size_t money_count = 0;
	for (size_t index = 0; index < identity.image.items.size(); ++index)
	{
		const auto &item = identity.image.items[index];
		if (item.type != ITEM_MONEY)
			continue;
		++money_count;
		require(item.vnum == VOBJ_COINS && item.equipment_slot == 0, EILSEQ);
		auto normalized = item;
		normalized.parent_index = PLAYER_SNAPSHOT_NO_PARENT;
		normalized.equipment_slot = 0;
		std::vector<uint8_t> expected;
		const auto encoded = player_item_snapshot_list_encode({ normalized }, &expected);
		require(encoded == player_snapshot_codec_result::ok,
			encoded == player_snapshot_codec_result::allocation_failure ? ENOMEM :
										      EILSEQ);
		require(expected == batch.payloads[index], EILSEQ);
		const auto found = std::find_if(identity.image.coins.begin(),
						identity.image.coins.end(),
						[&](const zone_reset_coin_output &coin)
						{ return coin.item_uid == item.object_uid; });
		require(found != identity.image.coins.end(), EILSEQ);
		economic_coin_vector actual{};
		for (size_t denomination = 0; denomination < actual.size(); ++denomination)
		{
			require(item.values[denomination] >= 0, EILSEQ);
			actual[denomination] = item.values[denomination];
		}
		require(actual == found->denominations, EILSEQ);
		const economic_account_key key{ plan.metadata.lineage, economic_account_kind::pile,
						item.object_uid, 0 };
		size_t matches = 0;
		for (const auto &effect : plan.accounts)
			if (economic_account_key_equal(effect.key, key))
			{
				++matches;
				require(effect.before == economic_coin_vector{} &&
						effect.after == actual &&
						effect.before_revision == 0 &&
						effect.after_revision == 1,
					EILSEQ);
			}
		require(matches == 1, EILSEQ);
		// Full exact literal comparison authenticates every actual denomination and
		// UID independently of aggregate copper and of the original nested topology.
		count(connection, "item_current_owner",
		      "item_uid=" + std::to_string(item.object_uid) +
			      " AND item_revision=1 AND coin_payload=" + hex(expected),
		      1);
	}
	require(money_count == identity.image.coins.size(), EILSEQ);
	size_t piles = 0, issuance = 0;
	for (const auto &effect : plan.accounts)
	{
		if (effect.key.kind == economic_account_kind::pile)
			++piles;
		else
		{
			require(effect.key.kind == economic_account_kind::issuance &&
					effect.key.authority_id == 1 &&
					effect.key.context_id == 0 &&
					effect.key.lineage.bytes == plan.metadata.lineage.bytes &&
					effect.before == economic_coin_vector{} &&
					effect.after == economic_coin_vector{} &&
					effect.before_revision == 0 && effect.after_revision == 0,
				EILSEQ);
			++issuance;
		}
	}
	const bool nonzero = std::any_of(identity.image.coins.begin(), identity.image.coins.end(),
					 [](const zone_reset_coin_output &coin)
					 {
						 return std::any_of(coin.denominations.begin(),
								    coin.denominations.end(),
								    [](int64_t value)
								    { return value != 0; });
					 });
	require(piles == money_count && issuance == (nonzero ? 1U : 0U), EILSEQ);
}
void verify_current(MYSQL *connection, const critical_command &command,
		    const reset_identity &identity, const economic_accounting_plan &plan,
		    const sql_room_item_payload_batch &batch)
{
	uint64_t season = 0;
	participant(sql_room_item_payload_lock_season(connection, &season));
	require(season == identity.image.season_epoch, ESTALE);
	count(connection, "item_owner_revision", owner(identity.image), 1);
	count(connection, "item_owner_revision",
	      owner(identity.image) +
		      " AND revision=" + std::to_string(identity.image.expected_room_revision + 1),
	      1);
	const auto root = std::to_string(identity.image.items.front().object_uid);
	count(connection, "item_current_owner", "root_item_uid=" + root, plan.item_events.size());
	require(batch.payloads.size() == plan.item_events.size(), EILSEQ);
	for (size_t index = 0; index < plan.item_events.size(); ++index)
	{
		count(connection, "item_current_owner",
		      predicate(item_row(plan.item_events[index], identity.image.items[index],
					 batch.payloads[index])),
		      1);
		count(connection, "item_ownership_ledger",
		      predicate(ledger(command.operation_id, command.source_site,
				       identity.image.expected_room_revision + 1,
				       plan.item_events[index])),
		      1);
		count(connection, "sql_room_item_payload",
		      predicate({ { "item_uid", std::to_string(plan.item_events[index].uid) },
				  { "item_revision", "1" },
				  { "payload_version", "1" },
				  { "operation_id", id(command.operation_id) },
				  { "season_epoch", std::to_string(identity.image.season_epoch) },
				  { "payload", hex(batch.payloads[index]) } }),
		      1);
	}
	count(connection, "sql_room_item_payload", "operation_id=" + id(command.operation_id),
	      plan.item_events.size());
	verify_born_money(connection, identity, plan, batch);
	physical_absence(connection, uids(identity.image));
}
fields operation(const critical_operation_id &root, const economic_accounting_plan &plan,
		 std::span<const uint8_t> canonical_intent)
{
	const auto &meta = plan.metadata;
	std::vector<uint8_t> encoded;
	checked(economic_plan_encode(plan, &encoded));
	return { { "operation_id", id(root) },
		 { "lineage", id(meta.lineage) },
		 { "epoch", id(meta.epoch) },
		 { "original_operation_id", "NULL" },
		 { "accounting_version", std::to_string(meta.version) },
		 { "writer_id", std::to_string(meta.writer_id) },
		 { "policy_version", std::to_string(meta.policy_version) },
		 { "compiler_version", std::to_string(meta.compiler_version) },
		 { "actor_kind", std::to_string(static_cast<uint8_t>(meta.actor_kind)) },
		 { "actor_id", std::to_string(meta.actor_id) },
		 { "reason", std::to_string(static_cast<uint16_t>(meta.reason)) },
		 { "source_event", source(meta) },
		 { "intent_digest", hex(meta.intent_digest) },
		 { "domain_digest", hex(meta.domain_digest) },
		 { "plan_digest", hex(hash(encoded)) },
		 { "canonical_intent", hex(canonical_intent) },
		 { "canonical_plan", hex(encoded) },
		 { "outcome", "1" },
		 { "result_code", "0" },
		 { "realized_price_copper", "NULL" },
		 { "account_count", std::to_string(plan.accounts.size()) },
		 { "posting_count", std::to_string(plan.postings.size()) },
		 { "child_count", "0" },
		 { "item_event_count", std::to_string(plan.item_events.size()) },
		 { "before_witness_count", std::to_string(plan.items_before.size()) },
		 { "after_witness_count", std::to_string(plan.items_after.size()) } };
}
fields effect(const critical_operation_id &root, size_t index, const economic_account_effect &value)
{
	std::array<uint8_t, ECONOMIC_ACCOUNT_KEY_BYTES> encoded{};
	checked(economic_account_key_encode(value.key, &encoded));
	fields result = { { "operation_id", id(root) },
			  { "account_index", std::to_string(index) },
			  { "account_key", hex(encoded) } };
	coins(result, "before_", value.before);
	coins(result, "after_", value.after);
	result.emplace_back("before_revision", std::to_string(value.before_revision));
	result.emplace_back("after_revision", std::to_string(value.after_revision));
	return result;
}
fields posting(const critical_operation_id &root, size_t index, const economic_coin_posting &value)
{
	fields result = { { "operation_id", id(root) },
			  { "line_index", std::to_string(index) },
			  { "event_index", std::to_string(value.event_index) },
			  { "account_index", std::to_string(value.account_index) },
			  { "child_index", std::to_string(value.child_index) },
			  { "copper_value", std::to_string(value.copper) } };
	coins(result, "delta_", value.delta);
	return result;
}
void evidence(MYSQL *connection, const critical_operation_id &operation_id,
	      critical_source_site source_site, uint64_t owner_revision,
	      const economic_accounting_plan &plan, std::span<const uint8_t> canonical_intent,
	      bool append)
{
	const auto root = operation(operation_id, plan, canonical_intent);
	if (append)
		insert(connection, "economic_accounting_operation", root);
	count(connection, "economic_accounting_operation", predicate(root), 1);
	for (size_t index = 0; index < plan.accounts.size(); ++index)
	{
		const auto row = effect(operation_id, index, plan.accounts[index]);
		if (append)
			insert(connection, "economic_accounting_account_effect", row);
		count(connection, "economic_accounting_account_effect", predicate(row), 1);
	}
	for (size_t index = 0; index < plan.postings.size(); ++index)
	{
		const auto row = posting(operation_id, index, plan.postings[index]);
		if (append)
			insert(connection, "economic_accounting_coin_posting", row);
		count(connection, "economic_accounting_coin_posting", predicate(row), 1);
	}
	for (const auto &event : plan.item_events)
	{
		count(connection, "item_ownership_ledger",
		      predicate(ledger(operation_id, source_site, owner_revision, event)), 1);
		economic_accounting_item_reference ref{};
		ref.operation_id = operation_id;
		ref.line_index = static_cast<uint16_t>(event.event_index);
		ref.event_index = event.event_index;
		ref.child_index = 0;
		ref.item_uid = event.uid;
		ref.before_revision = 0;
		ref.after_revision = 1;
		ref.legacy_operation_id = operation_id;
		ref.legacy_event_index = static_cast<uint16_t>(event.event_index);
		if (append && !economic_accounting_item_reference_insert(connection, ref))
		{
			const auto error = errno;
			throw failure{ error > 0 ? static_cast<unsigned int>(error) : EIO };
		}
		count(connection, "economic_accounting_item_reference",
		      predicate(
			      { { "operation_id", id(operation_id) },
				{ "line_index", std::to_string(ref.line_index) },
				{ "event_index", std::to_string(ref.event_index) },
				{ "child_index", "0" },
				{ "item_uid", std::to_string(ref.item_uid) },
				{ "before_revision", "0" },
				{ "after_revision", "1" },
				{ "legacy_operation_id", id(operation_id) },
				{ "legacy_event_index", std::to_string(ref.legacy_event_index) } }),
		      1);
	}
	const fields claim = { { "lineage", id(plan.metadata.lineage) },
			       { "source_event", source(plan.metadata) },
			       { "operation_id", id(operation_id) },
			       { "outcome", "1" } };
	if (append)
		insert(connection, "economic_accounting_source_claim", claim);
	count(connection, "economic_accounting_source_claim", predicate(claim), 1);
	const auto where = "operation_id=" + id(operation_id);
	count(connection, "economic_accounting_account_effect", where, plan.accounts.size());
	count(connection, "economic_accounting_coin_posting", where, plan.postings.size());
	count(connection, "economic_accounting_item_reference", where, plan.item_events.size());
	count(connection, "item_ownership_ledger", where, plan.item_events.size());
	count(connection, "economic_accounting_source_claim", where, 1);
	count(connection, "economic_accounting_child", where, 0);
}
void evidence(MYSQL *connection, const critical_command &command, uint64_t owner_revision,
	      const economic_accounting_plan &plan, bool append)
{
	evidence(connection, command.operation_id, command.source_site, owner_revision, plan,
		 command.accounting_intent, append);
}

void outbox(MYSQL *connection, const critical_command &command, std::span<const uint8_t> payload,
	    bool pending)
{
	fields expected = { { "operation_id", id(command.operation_id) },
			    { "event_index", "0" },
			    { "destination", std::to_string(ZONE_RESET_ITEM_OUTBOX_DESTINATION) },
			    { "event_type", std::to_string(ZONE_RESET_ITEM_OUTBOX_EVENT) },
			    { "payload_version", std::to_string(ZONE_RESET_ITEM_RESULT_VERSION) },
			    { "payload", hex(payload) } };
	if (pending)
	{
		expected.emplace_back("status", "0");
		expected.emplace_back("attempt_count", "0");
		expected.emplace_back("last_error_code", "0");
		expected.emplace_back("delivered_at", "NULL");
		expected.emplace_back("dead_lettered_at", "NULL");
	}
	count(connection, "critical_outbox", "operation_id=" + id(command.operation_id), 1);
	count(connection, "critical_outbox",
	      predicate(expected) + (pending ? " AND next_attempt_at<=CURRENT_TIMESTAMP(6)" : ""),
	      1);
}
}
struct economic_sql_zone_reset_item_transaction::implementation
{
	MYSQL *connection = nullptr;
	unsigned long session = 0;
	critical_command command;
	reset_identity identity;
	std::vector<uint64_t> item_uids;
	economic_sql_authority_snapshot authority;
	economic_accounting_plan plan;
	sql_room_item_payload_batch literal;
	item_transfer_result result{};
	std::string marker;
	unsigned int code = EINPROGRESS;
	enum class phase
	{
		prepared,
		applied,
		finalized,
		verified,
		failed
	} phase = phase::prepared;
};
economic_sql_zone_reset_item_transaction::economic_sql_zone_reset_item_transaction(
	std::unique_ptr<implementation> state)
	: state_(std::move(state))
{
}
economic_sql_zone_reset_item_transaction::~economic_sql_zone_reset_item_transaction() = default;
const item_transfer_result &economic_sql_zone_reset_item_transaction::result() const
{
	return state_->result;
}
unsigned int economic_sql_zone_reset_item_transaction::result_code() const
{
	return state_->code;
}
unsigned int economic_sql_zone_reset_item_transaction::prepare(
	MYSQL *connection, const critical_command &command,
	std::unique_ptr<economic_sql_zone_reset_item_transaction> *output)
{
	try
	{
		require(connection && output, EINVAL);
		auto state = std::make_unique<implementation>();
		state->connection = connection;
		state->session = mysql_thread_id(connection);
		active(connection, state->session);
		state->identity = decode(command);
		state->command = command;
		state->item_uids = uids(state->identity.image);
		checked(zone_reset_item_accounting_compile(command, &state->plan));
		state->result = expected_result(state->identity.image);
		inbox(connection, command, true);
		const auto &meta = state->identity.intent.admission.metadata;
		const auto error = economic_sql_lock_authority(connection, meta.lineage, meta.epoch,
							       {}, &state->authority);
		require(!error, error);
		// Literal participant takes season exclusion before room/item locks.
		// Money shares this root; artifacts remain with their original owner.
		participant(sql_room_item_payload_prepare_creation(
			connection, state->identity.image, &state->literal));
		require(state->literal.session_id == state->session &&
				state->literal.season_epoch == state->identity.image.season_epoch &&
				state->literal.payloads.size() == state->plan.item_events.size(),
			EILSEQ);
		original_absence(connection, command, state->identity, state->item_uids);
		char operation_hex[CRITICAL_COMMAND_ID_HEX_SIZE]{};
		require(critical_operation_id_to_hex(command.operation_id, operation_hex,
						     sizeof(operation_hex)),
			EINVAL);
		state->marker = std::string("zone_reset_item_") + operation_hex;
		execute(connection, "SAVEPOINT " + state->marker);
		active(connection, state->session);
		auto result = std::unique_ptr<economic_sql_zone_reset_item_transaction>(
			new economic_sql_zone_reset_item_transaction(std::move(state)));
		*output = std::move(result);
		return 0;
	}
	catch (const failure &error)
	{
		return error.code;
	}
	catch (const std::bad_alloc &)
	{
		return ENOMEM;
	}
	catch (...)
	{
		return EINVAL;
	}
}
unsigned int economic_sql_zone_reset_item_transaction::apply()
{
	auto &state = *state_;
	if (state.phase != implementation::phase::prepared)
		return EPERM;
	state.phase = implementation::phase::failed;
	try
	{
		active(state.connection, state.session);
		execute(state.connection, "RELEASE SAVEPOINT " + state.marker);
		execute(state.connection, "SAVEPOINT " + state.marker);
		inbox(state.connection, state.command, true);
		sql_room_item_payload_batch observed;
		participant(sql_room_item_payload_prepare_creation(
			state.connection, state.identity.image, &observed));
		require(observed.session_id == state.literal.session_id &&
				observed.season_epoch == state.literal.season_epoch &&
				observed.payloads == state.literal.payloads,
			ESTALE);
		original_absence(state.connection, state.command, state.identity, state.item_uids);
		// First DML: all subsequent errors demand rollback of the original root.
		execute(state.connection,
			"UPDATE item_owner_revision SET revision=revision+1 WHERE " +
				owner(state.identity.image) + " AND revision=" +
				std::to_string(state.identity.image.expected_room_revision));
		require(mysql_affected_rows(state.connection) == 1, ESTALE);
		for (size_t index = 0; index < state.plan.item_events.size(); ++index)
		{
			insert(state.connection, "item_current_owner",
			       item_row(state.plan.item_events[index],
					state.identity.image.items[index],
					state.literal.payloads[index]));
			insert(state.connection, "item_ownership_ledger",
			       ledger(state.command.operation_id, state.command.source_site,
				      state.result.to_owner_revision,
				      state.plan.item_events[index]));
		}
		participant(sql_room_item_payload_record_creation(
			state.connection, state.command, state.identity.image, &state.literal));
		verify_current(state.connection, state.command, state.identity, state.plan,
			       state.literal);
		active(state.connection, state.session);
		state.code = 0;
		state.phase = implementation::phase::applied;
		return 0;
	}
	catch (const failure &error)
	{
		state.code = error.code;
	}
	catch (const std::bad_alloc &)
	{
		state.code = ENOMEM;
	}
	catch (...)
	{
		state.code = EINVAL;
	}
	return state.code;
}
unsigned int economic_sql_zone_reset_item_transaction::finalize()
{
	auto &state = *state_;
	if (state.phase != implementation::phase::applied)
		return EPERM;
	state.phase = implementation::phase::failed;
	try
	{
		active(state.connection, state.session);
		execute(state.connection, "RELEASE SAVEPOINT " + state.marker);
		execute(state.connection, "SAVEPOINT " + state.marker);
		inbox(state.connection, state.command, true);
		const auto &meta = state.identity.intent.admission.metadata;
		economic_sql_authority_snapshot authority;
		const auto error = economic_sql_lock_authority(state.connection, meta.lineage,
							       meta.epoch, {}, &authority);
		require(!error, error);
		require(authority.lineage_revision == state.authority.lineage_revision, ESTALE);
		verify_current(state.connection, state.command, state.identity, state.plan,
			       state.literal);
		evidence(state.connection, state.command, state.result.to_owner_revision,
			 state.plan, true);
		evidence(state.connection, state.command, state.result.to_owner_revision,
			 state.plan, false);
		verify_current(state.connection, state.command, state.identity, state.plan,
			       state.literal);
		inbox(state.connection, state.command, true);
		active(state.connection, state.session);
		state.code = 0;
		state.phase = implementation::phase::finalized;
		return 0;
	}
	catch (const failure &error)
	{
		state.code = error.code;
	}
	catch (const std::bad_alloc &)
	{
		state.code = ENOMEM;
	}
	catch (...)
	{
		state.code = EINVAL;
	}
	return state.code;
}
unsigned int economic_sql_zone_reset_item_transaction::verify_root_completion()
{
	auto &state = *state_;
	if (state.phase != implementation::phase::finalized)
		return EPERM;
	state.phase = implementation::phase::failed;
	try
	{
		active(state.connection, state.session);
		execute(state.connection, "RELEASE SAVEPOINT " + state.marker);
		std::array<uint8_t, ITEM_TRANSFER_RESULT_BYTES> payload{};
		require(item_transfer_command_encode_result(state.result, &payload), EILSEQ);
		const auto retained = economic_sql_zone_reset_item_verify_retained(
			state.connection, state.command, 0, payload);
		require(!retained, retained);
		const auto &meta = state.identity.intent.admission.metadata;
		economic_sql_authority_snapshot authority;
		const auto error = economic_sql_lock_authority(state.connection, meta.lineage,
							       meta.epoch, {}, &authority);
		require(!error, error);
		require(authority.lineage_revision == state.authority.lineage_revision, ESTALE);
		verify_current(state.connection, state.command, state.identity, state.plan,
			       state.literal);
		outbox(state.connection, state.command, payload, true);
		// Retain the authentic command+receipt after outer inbox/outbox completion,
		// atomically before commit. Historical verifier never calls the origin reader.
		const auto origin = zone_reset_item_origin_sql_retain_locked(
			state.connection, state.command, payload);
		require(!origin, static_cast<unsigned int>(origin));
		active(state.connection, state.session);
		state.code = 0;
		state.phase = implementation::phase::verified;
		return 0;
	}
	catch (const failure &error)
	{
		state.code = error.code;
	}
	catch (const std::bad_alloc &)
	{
		state.code = ENOMEM;
	}
	catch (...)
	{
		state.code = EINVAL;
	}
	return state.code;
}
unsigned int economic_sql_zone_reset_item_verify_retained(MYSQL *connection,
							  const critical_command &command,
							  unsigned int result_code,
							  std::span<const uint8_t> payload) noexcept
{
	try
	{
		require(connection && !result_code, EINVAL);
		const auto session = mysql_thread_id(connection);
		active(connection, session);
		const auto identity = decode(command);
		economic_accounting_plan expected;
		checked(zone_reset_item_accounting_compile(command, &expected));
		const auto result = expected_result(identity.image);
		std::array<uint8_t, ITEM_TRANSFER_RESULT_BYTES> expected_payload{};
		require(item_transfer_command_encode_result(result, &expected_payload), EILSEQ);
		require(payload.size() == expected_payload.size() &&
				std::equal(expected_payload.begin(), expected_payload.end(),
					   payload.begin()),
			EILSEQ);
		inbox(connection, command, false);
		count(connection, "critical_operation_inbox",
		      predicate({ { "operation_id", id(command.operation_id) },
				  { "result_code", "0" },
				  { "failure_stage", "0" },
				  { "durable_revision", std::to_string(result.to_owner_revision) },
				  { "result_payload", hex(payload) } }) +
			      " AND committed_at IS NOT NULL",
		      1);
		// Exact canonical plan/intent and every indexed row. No current world state
		// or original command reconstruction from hashes is used.
		evidence(connection, command, result.to_owner_revision, expected, false);
		outbox(connection, command, payload, false);
		active(connection, session);
		return 0;
	}
	catch (const failure &error)
	{
		return error.code;
	}
	catch (const std::bad_alloc &)
	{
		return ENOMEM;
	}
	catch (...)
	{
		return EINVAL;
	}
}
#endif
