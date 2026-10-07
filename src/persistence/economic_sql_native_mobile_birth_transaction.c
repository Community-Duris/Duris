#include "persistence/economic_sql_native_mobile_birth_transaction.h"
#include "persistence/economic_accounting_repository.h"
#include "persistence/quest_mobile_native_sql.h"
#include "persistence/quest_mobile_native_origin_sql.h"
#ifndef __NO_MYSQL__
#include "persistence/economic_sql_source_snapshot.h"
#endif
#include "item/economic_accounting_item_reference.h"
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
struct birth_identity
{
	quest_mobile_native_image image;
	economic_frozen_intent intent;
};
birth_identity decode(const critical_command &command)
{
	birth_identity value;
	checked(native_mobile_birth_command_decode(command, &value.image));
	checked(economic_intent_decode(command.accounting_intent, &value.intent));
	return value;
}
}
bool economic_sql_native_mobile_birth_command_supported(const critical_command &command) noexcept
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
struct economic_sql_native_mobile_birth_transaction::implementation
{
	native_mobile_birth_result result{};
};
economic_sql_native_mobile_birth_transaction::economic_sql_native_mobile_birth_transaction(
	std::unique_ptr<implementation> state)
	: state_(std::move(state))
{
}
economic_sql_native_mobile_birth_transaction::~economic_sql_native_mobile_birth_transaction() =
	default;
unsigned int economic_sql_native_mobile_birth_transaction::prepare(
	MYSQL *, const critical_command &,
	std::unique_ptr<economic_sql_native_mobile_birth_transaction> *)
{
	return ENOTSUP;
}
unsigned int economic_sql_native_mobile_birth_transaction::apply()
{
	return ENOTSUP;
}
unsigned int economic_sql_native_mobile_birth_transaction::finalize()
{
	return ENOTSUP;
}
unsigned int economic_sql_native_mobile_birth_transaction::verify_root_completion()
{
	return ENOTSUP;
}
const native_mobile_birth_result &economic_sql_native_mobile_birth_transaction::result() const
{
	return state_->result;
}
unsigned int economic_sql_native_mobile_birth_transaction::result_code() const
{
	return ENOTSUP;
}
unsigned int economic_sql_native_mobile_birth_verify_retained(MYSQL *, const critical_command &,
							      unsigned int,
							      std::span<const uint8_t>) noexcept
{
	return ENOTSUP;
}
unsigned int economic_sql_native_mobile_birth_observe_origin(MYSQL *,
							     const quest_mobile_native_reference &,
							     native_mobile_wallet_origin *) noexcept
{
	return ENOTSUP;
}
unsigned int economic_sql_native_mobile_birth_lock_publication(
	MYSQL *, const critical_command &, const critical_completion &, quest_mobile_native_image *,
	std::vector<item_ownership_runtime_entry> *) noexcept
{
	return ENOTSUP;
}
unsigned int economic_sql_native_mobile_birth_lock_wallet_lifetimes(
	MYSQL *, const critical_operation_id &,
	std::vector<economic_sql_native_mobile_wallet_lifetime> *) noexcept
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
std::string owner(uint64_t mobile)
{
	return "owner_type=12 AND owner_id=" + std::to_string(mobile);
}
std::vector<uint64_t> uids(const quest_mobile_native_image &image)
{
	std::vector<uint64_t> result;
	result.reserve(image.items.size());
	for (const auto &item : image.items)
		result.push_back(item.object_uid);
	std::sort(result.begin(), result.end());
	return result;
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
std::string custody_scope(uint64_t mobile, std::span<const uint64_t> values)
{
	std::string result = "(" + owner(mobile) + ")";
	if (!values.empty())
	{
		const auto list = uid_list(values);
		result += " OR item_uid IN(" + list + ") OR root_item_uid IN(" + list +
			  ") OR parent_item_uid IN(" + list + ")";
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
void original_absence(MYSQL *connection, const critical_command &command,
		      const birth_identity &identity, std::span<const uint64_t> values,
		      quest_mobile_native_sql_row *native_before)
{
	const auto &meta = identity.intent.admission.metadata;
	empty(connection,
	      "SELECT operation_id FROM economic_accounting_source_claim WHERE lineage=" +
		      id(meta.lineage) + " AND source_event=" + source(meta) + " FOR UPDATE");
	empty(connection,
	      "SELECT mapping_id FROM economic_account_mapping WHERE (backend_kind=1 AND locator_kind=" +
		      std::to_string(ECONOMIC_NATIVE_MOBILE_WALLET_LOCATOR) + " AND native_id=" +
		      std::to_string(identity.image.reference.mobile_instance_id) +
		      ") OR creating_operation_id=" + id(command.operation_id) +
		      " ORDER BY mapping_id FOR UPDATE");
	const auto error = quest_mobile_native_sql_lock(
		connection, identity.image.reference.mobile_instance_id, native_before);
	require(!error, error);
	require(!native_before->present, EEXIST);
	empty(connection, "SELECT revision FROM item_owner_revision WHERE " +
				  owner(identity.image.reference.mobile_instance_id) +
				  " ORDER BY owner_context_id FOR UPDATE");
	empty(connection,
	      "SELECT item_uid FROM item_current_owner WHERE " +
		      custody_scope(identity.image.reference.mobile_instance_id, values) +
		      " ORDER BY item_uid FOR UPDATE");
	for (uint64_t uid : values)
	{
		for (const char *table :
		     { "item_ownership_baseline", "item_ownership_ledger",
		       "economic_accounting_item_reference", "item_ownership_quarantine",
		       "auction_reconciliation_quarantine", "collector_reconciliation_quarantine",
		       "player_death_restitution_item", "player_death_restitution_delivery" })
			empty(connection, "SELECT item_uid FROM " + std::string(table) +
						  " WHERE item_uid=" + std::to_string(uid) +
						  " FOR UPDATE");
	}
	physical_absence(connection, values);
	for (const char *table :
	     { "economic_accounting_operation", "economic_accounting_account_effect",
	       "economic_accounting_coin_posting", "economic_accounting_item_reference",
	       "economic_accounting_source_claim", "economic_accounting_child",
	       "item_ownership_ledger", "critical_outbox" })
		count(connection, table, "operation_id=" + id(command.operation_id), 0);
}
fields mapping(const critical_command &command, const birth_identity &identity, uint64_t lifetime)
{
	return { { "mapping_id", std::to_string(lifetime) },
		 { "lineage", id(identity.intent.admission.metadata.lineage) },
		 { "account_kind", "1" },
		 { "context_id", std::to_string(ECONOMIC_NATIVE_MOBILE_WALLET_CONTEXT) },
		 { "backend_kind", "1" },
		 { "locator_kind", std::to_string(ECONOMIC_NATIVE_MOBILE_WALLET_LOCATOR) },
		 { "native_id", std::to_string(identity.image.reference.mobile_instance_id) },
		 { "creating_operation_id", id(command.operation_id) } };
}
economic_account_key create_mapping(MYSQL *connection, const critical_command &command,
				    const birth_identity &identity)
{
	auto values = mapping(command, identity, 0);
	values.erase(values.begin());
	values.emplace_back("active_native_id",
			    std::to_string(identity.image.reference.mobile_instance_id));
	values.emplace_back("retiring_operation_id", "NULL");
	values.emplace_back("revision", "0");
	insert(connection, "economic_account_mapping", values);
	const auto lifetime = static_cast<uint64_t>(mysql_insert_id(connection));
	require(lifetime != 0, EIO);
	const economic_account_key key = { identity.intent.admission.metadata.lineage,
					   economic_account_kind::wallet, lifetime,
					   ECONOMIC_NATIVE_MOBILE_WALLET_CONTEXT };
	auto expected = mapping(command, identity, lifetime);
	expected.emplace_back("active_native_id",
			      std::to_string(identity.image.reference.mobile_instance_id));
	expected.emplace_back("retiring_operation_id", "NULL");
	expected.emplace_back("revision", "0");
	count(connection, "economic_account_mapping", predicate(expected), 1);
	return key;
}
fields item_row(const economic_item_event &event, int32_t vnum)
{
	const auto &after = event.after;
	return { { "item_uid", std::to_string(event.uid) },
		 { "root_item_uid", std::to_string(after.root_uid) },
		 { "parent_item_uid",
		   after.parent_uid ? std::to_string(after.parent_uid) : "NULL" },
		 { "owner_type", "12" },
		 { "owner_id", std::to_string(after.owner.id) },
		 { "owner_context_id", "0" },
		 { "item_revision", "1" },
		 { "vnum", std::to_string(vnum) },
		 { "state", "1" },
		 { "equipment_slot", std::to_string(after.equipment_slot) },
		 { "coin_payload", "NULL" } };
}
fields ledger(const critical_operation_id &root, critical_source_site source_site,
	      const economic_item_event &event)
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
		 { "to_owner_type", "12" },
		 { "to_owner_id", std::to_string(event.after.owner.id) },
		 { "to_owner_context_id", "0" },
		 { "item_revision", "1" },
		 { "from_owner_revision", "0" },
		 { "to_owner_revision", "1" },
		 { "reason_type",
		   std::to_string(static_cast<uint16_t>(item_transfer_reason::creation)) },
		 { "reason_id", "0" },
		 { "source_site", std::to_string(static_cast<uint16_t>(source_site)) },
		 { "from_equipment_slot", "0" },
		 { "to_equipment_slot", std::to_string(event.after.equipment_slot) } };
}
fields ledger(const critical_command &command, const economic_item_event &event)
{
	return ledger(command.operation_id, command.source_site, event);
}

void verify_current(MYSQL *connection, const critical_command &command,
		    const birth_identity &identity, const economic_account_key &wallet,
		    const economic_accounting_plan &plan, std::span<const uint64_t> values)
{
	auto expected_mapping = mapping(command, identity, wallet.authority_id);
	expected_mapping.emplace_back("active_native_id",
				      std::to_string(identity.image.reference.mobile_instance_id));
	expected_mapping.emplace_back("retiring_operation_id", "NULL");
	expected_mapping.emplace_back("revision", "0");
	count(connection, "economic_account_mapping", predicate(expected_mapping), 1);
	quest_mobile_native_sql_row native;
	const auto error = quest_mobile_native_sql_lock(
		connection, identity.image.reference.mobile_instance_id, &native);
	require(!error, error);
	require(native.present, ESTALE);
	std::vector<uint8_t> actual, expected;
	const auto encode_image =
		[](const quest_mobile_native_image &image, std::vector<uint8_t> *bytes)
	{
		const auto status = quest_mobile_native_image_encode(image, bytes);
		require(status == player_snapshot_codec_result::ok,
			status == player_snapshot_codec_result::allocation_failure ?
				ENOMEM :
				(status == player_snapshot_codec_result::limit_exceeded ? E2BIG :
											  EBADMSG));
	};
	encode_image(native.image, &actual);
	// identity.image comes from complete canonical immutable-command decode.
	// A v2 command wraps image plus recipe; native SQL still stores the original
	// image format. Full command/inbox/hash proofs remain separately required.
	encode_image(identity.image, &expected);
	require(actual == expected, ESTALE);
	count(connection, "item_owner_revision", owner(identity.image.reference.mobile_instance_id),
	      1);
	count(connection, "item_owner_revision",
	      predicate(
		      { { "owner_type", "12" },
			{ "owner_id", std::to_string(identity.image.reference.mobile_instance_id) },
			{ "owner_context_id", "0" },
			{ "revision", "1" } }),
	      1);
	execute(connection,
		"SELECT item_uid FROM item_current_owner WHERE " +
			custody_scope(identity.image.reference.mobile_instance_id, values) +
			" ORDER BY item_uid LIMIT " + std::to_string(values.size() + 1) +
			" FOR UPDATE");
	std::unique_ptr<MYSQL_RES, decltype(&mysql_free_result)> rows(
		mysql_store_result(connection), mysql_free_result);
	require(bool(rows), EIO);
	require(mysql_num_fields(rows.get()) == 1 && mysql_num_rows(rows.get()) == values.size(),
		ESTALE);
	for (uint64_t uid : values)
	{
		auto row = mysql_fetch_row(rows.get());
		auto lengths = mysql_fetch_lengths(rows.get());
		require(row && lengths && row[0]);
		require(integer<uint64_t>(std::string(row[0], lengths[0])) == uid, ESTALE);
	}
	for (size_t index = 0; index < plan.item_events.size(); ++index)
		count(connection, "item_current_owner",
		      predicate(
			      item_row(plan.item_events[index], identity.image.items[index].vnum)),
		      1);
	physical_absence(connection, values);
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
	      critical_source_site source_site, const economic_accounting_plan &plan,
	      std::span<const uint8_t> canonical_intent, bool append)
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
		      predicate(ledger(operation_id, source_site, event)), 1);
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
void evidence(MYSQL *connection, const critical_command &command,
	      const economic_accounting_plan &plan, bool append)
{
	evidence(connection, command.operation_id, command.source_site, plan,
		 command.accounting_intent, append);
}

void outbox(MYSQL *connection, const critical_operation_id &operation_id,
	    std::span<const uint8_t> payload, bool pending)
{
	fields expected = {
		{ "operation_id", id(operation_id) },
		{ "event_index", "0" },
		{ "destination", std::to_string(NATIVE_MOBILE_BIRTH_OUTBOX_DESTINATION) },
		{ "event_type", std::to_string(NATIVE_MOBILE_BIRTH_OUTBOX_EVENT) },
		{ "payload_version", std::to_string(NATIVE_MOBILE_BIRTH_RESULT_VERSION) },
		{ "payload", hex(payload) }
	};
	if (pending)
	{
		expected.emplace_back("status", "0");
		expected.emplace_back("attempt_count", "0");
		expected.emplace_back("last_error_code", "0");
		expected.emplace_back("delivered_at", "NULL");
		expected.emplace_back("dead_lettered_at", "NULL");
	}
	count(connection, "critical_outbox", "operation_id=" + id(operation_id), 1);
	count(connection, "critical_outbox",
	      predicate(expected) + (pending ? " AND next_attempt_at<=CURRENT_TIMESTAMP(6)" : ""),
	      1);
}
void outbox(MYSQL *connection, const critical_command &command, std::span<const uint8_t> payload,
	    bool pending)
{
	outbox(connection, command.operation_id, payload, pending);
}

}
struct economic_sql_native_mobile_birth_transaction::implementation
{
	MYSQL *connection = nullptr;
	unsigned long session = 0;
	critical_command command;
	birth_identity identity;
	std::vector<uint64_t> item_uids;
	quest_mobile_native_sql_row native_before;
	economic_sql_authority_snapshot initial_authority, mapped_authority;
	economic_account_key wallet{};
	economic_accounting_plan plan;
	native_mobile_birth_result result{};
	std::string marker;
	unsigned int code = EINPROGRESS;
	bool mutation_started = false;
	enum class phase
	{
		prepared,
		applied,
		finalized,
		verified,
		failed
	} phase = phase::prepared;
};
economic_sql_native_mobile_birth_transaction::economic_sql_native_mobile_birth_transaction(
	std::unique_ptr<implementation> state)
	: state_(std::move(state))
{
}
economic_sql_native_mobile_birth_transaction::~economic_sql_native_mobile_birth_transaction() =
	default;
const native_mobile_birth_result &economic_sql_native_mobile_birth_transaction::result() const
{
	return state_->result;
}
unsigned int economic_sql_native_mobile_birth_transaction::result_code() const
{
	return state_->code;
}
unsigned int economic_sql_native_mobile_birth_transaction::prepare(
	MYSQL *connection, const critical_command &command,
	std::unique_ptr<economic_sql_native_mobile_birth_transaction> *output)
{
	try
	{
		require(connection && output, EINVAL);
		auto state = std::make_unique<implementation>();
		state->connection = connection;
		state->session = mysql_thread_id(connection);
		active(connection, state->session);
		state->identity = decode(command);
		require(state->identity.image.items.size() <= ECONOMIC_ACCOUNTING_MAX_ITEM_EVENTS,
			E2BIG);
		state->command = command;
		state->item_uids = uids(state->identity.image);
		inbox(connection, command, true);
		const auto &meta = state->identity.intent.admission.metadata;
		const auto authority_error = economic_sql_lock_authority(
			connection, meta.lineage, meta.epoch, {}, &state->initial_authority);
		require(!authority_error, authority_error);
		original_absence(connection, command, state->identity, state->item_uids,
				 &state->native_before);
		char operation_hex[CRITICAL_COMMAND_ID_HEX_SIZE]{};
		require(critical_operation_id_to_hex(command.operation_id, operation_hex,
						     sizeof(operation_hex)),
			EINVAL);
		state->marker = std::string("native_mobile_birth_") + operation_hex;
		execute(connection, "SAVEPOINT " + state->marker);
		active(connection, state->session);
		auto result = std::unique_ptr<economic_sql_native_mobile_birth_transaction>(
			new economic_sql_native_mobile_birth_transaction(std::move(state)));
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
unsigned int economic_sql_native_mobile_birth_transaction::apply()
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
		quest_mobile_native_sql_row before;
		original_absence(state.connection, state.command, state.identity, state.item_uids,
				 &before);
		require(before.original_session == state.native_before.original_session &&
				!before.present,
			ESTALE);
		// First DML may already have happened on any later error. Root must
		// rollback/retire the original session; it must never publish a refusal.
		state.mutation_started = true;
		state.wallet = create_mapping(state.connection, state.command, state.identity);
		const economic_sql_mapping_request request = {
			state.wallet, ECONOMIC_NATIVE_MOBILE_WALLET_LOCATOR,
			state.identity.image.reference.mobile_instance_id
		};
		const auto &meta = state.identity.intent.admission.metadata;
		const auto error = economic_sql_lock_authority(state.connection, meta.lineage,
							       meta.epoch, std::span(&request, 1),
							       &state.mapped_authority);
		require(!error, error);
		require(state.mapped_authority.lineage_revision ==
					state.initial_authority.lineage_revision &&
				state.mapped_authority.mappings.size() == 1 &&
				state.mapped_authority.mappings[0].revision == 0,
			ESTALE);
		checked(native_mobile_birth_accounting_compile(state.command, state.wallet,
							       &state.plan));
		checked(native_mobile_birth_result_build(state.command, state.wallet, state.plan,
							 &state.result));
		insert(state.connection, "item_owner_revision",
		       { { "owner_type", "12" },
			 { "owner_id",
			   std::to_string(state.identity.image.reference.mobile_instance_id) },
			 { "owner_context_id", "0" },
			 { "revision", "1" } });
		quest_mobile_native_sql_row after;
		const auto native_error = quest_mobile_native_sql_apply_locked(
			state.connection, state.command.operation_id, before, state.identity.image,
			&after);
		require(!native_error, native_error);
		// Canonical event order is DFS: parent before child, literal equipment,
		// revision1 directly. Never reuse revision0 generic creation/adoption.
		for (size_t index = 0; index < state.plan.item_events.size(); ++index)
		{
			insert(state.connection, "item_current_owner",
			       item_row(state.plan.item_events[index],
					state.identity.image.items[index].vnum));
			insert(state.connection, "item_ownership_ledger",
			       ledger(state.command, state.plan.item_events[index]));
		}
		verify_current(state.connection, state.command, state.identity, state.wallet,
			       state.plan, state.item_uids);
		active(state.connection, state.session);
		state.code = 0;
		state.phase = implementation::phase::applied;
		return 0;
	}
	catch (const failure &error)
	{
		state.code = error.code;
		return state.code;
	}
	catch (const std::bad_alloc &)
	{
		state.code = ENOMEM;
		return state.code;
	}
	catch (...)
	{
		state.code = EINVAL;
		return state.code;
	}
}
unsigned int economic_sql_native_mobile_birth_transaction::finalize()
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
		const economic_sql_mapping_request request = {
			state.wallet, ECONOMIC_NATIVE_MOBILE_WALLET_LOCATOR,
			state.identity.image.reference.mobile_instance_id
		};
		economic_sql_authority_snapshot authority;
		const auto error = economic_sql_lock_authority(state.connection,
							       state.initial_authority.lineage,
							       state.initial_authority.epoch,
							       std::span(&request, 1), &authority);
		require(!error, error);
		require(authority.lineage_revision == state.initial_authority.lineage_revision &&
				authority.mappings.size() == 1 &&
				authority.mappings[0].revision == 0,
			ESTALE);
		verify_current(state.connection, state.command, state.identity, state.wallet,
			       state.plan, state.item_uids);
		// Root operation precedes all economic reference/source-claim FKs.
		evidence(state.connection, state.command, state.plan, true);
		evidence(state.connection, state.command, state.plan, false);
		verify_current(state.connection, state.command, state.identity, state.wallet,
			       state.plan, state.item_uids);
		inbox(state.connection, state.command, true);
		active(state.connection, state.session);
		state.code = 0;
		state.phase = implementation::phase::finalized;
		return 0;
	}
	catch (const failure &error)
	{
		state.code = error.code;
		return state.code;
	}
	catch (const std::bad_alloc &)
	{
		state.code = ENOMEM;
		return state.code;
	}
	catch (...)
	{
		state.code = EINVAL;
		return state.code;
	}
}
unsigned int economic_sql_native_mobile_birth_transaction::verify_root_completion()
{
	auto &state = *state_;
	if (state.phase != implementation::phase::finalized)
		return EPERM;
	state.phase = implementation::phase::failed;
	try
	{
		active(state.connection, state.session);
		execute(state.connection, "RELEASE SAVEPOINT " + state.marker);
		std::array<uint8_t, NATIVE_MOBILE_BIRTH_RESULT_BYTES> payload{};
		require(native_mobile_birth_result_encode(state.result, &payload), EILSEQ);
		const auto retained_error = economic_sql_native_mobile_birth_verify_retained(
			state.connection, state.command, 0, payload);
		require(!retained_error, retained_error);
		const economic_sql_mapping_request request = {
			state.wallet, ECONOMIC_NATIVE_MOBILE_WALLET_LOCATOR,
			state.identity.image.reference.mobile_instance_id
		};
		economic_sql_authority_snapshot authority;
		const auto error = economic_sql_lock_authority(state.connection,
							       state.initial_authority.lineage,
							       state.initial_authority.epoch,
							       std::span(&request, 1), &authority);
		require(!error, error);
		require(authority.lineage_revision == state.initial_authority.lineage_revision &&
				authority.mappings.size() == 1 &&
				authority.mappings[0].revision == 0,
			ESTALE);
		verify_current(state.connection, state.command, state.identity, state.wallet,
			       state.plan, state.item_uids);
		outbox(state.connection, state.command, payload, true);
		active(state.connection, state.session);
		state.code = 0;
		state.phase = implementation::phase::verified;
		return 0;
	}
	catch (const failure &error)
	{
		state.code = error.code;
		return state.code;
	}
	catch (const std::bad_alloc &)
	{
		state.code = ENOMEM;
		return state.code;
	}
	catch (...)
	{
		state.code = EINVAL;
		return state.code;
	}
}
unsigned int
economic_sql_native_mobile_birth_verify_retained(MYSQL *connection, const critical_command &command,
						 unsigned int result_code,
						 std::span<const uint8_t> payload) noexcept
{
	try
	{
		require(connection && !result_code, EINVAL);
		const auto session = mysql_thread_id(connection);
		active(connection, session);
		const auto identity = decode(command);
		native_mobile_birth_result result{};
		require(native_mobile_birth_result_decode(payload, &result), EILSEQ);
		const economic_account_key wallet = { identity.intent.admission.metadata.lineage,
						      economic_account_kind::wallet,
						      result.wallet_mapping_id,
						      ECONOMIC_NATIVE_MOBILE_WALLET_CONTEXT };
		economic_accounting_plan expected;
		checked(native_mobile_birth_accounting_compile(command, wallet, &expected));
		native_mobile_birth_result expected_result{};
		checked(native_mobile_birth_result_build(command, wallet, expected,
							 &expected_result));
		std::array<uint8_t, NATIVE_MOBILE_BIRTH_RESULT_BYTES> expected_payload{};
		require(native_mobile_birth_result_encode(expected_result, &expected_payload),
			EILSEQ);
		require(payload.size() == expected_payload.size() &&
				std::equal(expected_payload.begin(), expected_payload.end(),
					   payload.begin()),
			EILSEQ);
		inbox(connection, command, false);
		count(connection, "critical_operation_inbox",
		      predicate({ { "operation_id", id(command.operation_id) },
				  { "result_code", "0" },
				  { "failure_stage", "0" },
				  { "durable_revision", "1" },
				  { "result_payload", hex(payload) } }) +
			      " AND committed_at IS NOT NULL",
		      1);
		const auto encoded = read(
			connection,
			"SELECT canonical_plan FROM economic_accounting_operation WHERE operation_id=" +
				id(command.operation_id),
			1);
		require(encoded[0].has_value());
		std::vector<uint8_t> expected_bytes;
		checked(economic_plan_encode(expected, &expected_bytes));
		require(encoded[0]->size() == expected_bytes.size() &&
				std::equal(expected_bytes.begin(), expected_bytes.end(),
					   reinterpret_cast<const uint8_t *>(encoded[0]->data())),
			EILSEQ);
		// Stable mapping identity survives retirement. No active epoch, active
		// mapping, current native image or current cash/stock is consulted.
		count(connection, "economic_account_mapping",
		      predicate(mapping(command, identity, wallet.authority_id)), 1);
		count(connection, "economic_account_mapping",
		      "backend_kind=1 AND locator_kind=" +
			      std::to_string(ECONOMIC_NATIVE_MOBILE_WALLET_LOCATOR) +
			      " AND native_id=" +
			      std::to_string(identity.image.reference.mobile_instance_id),
		      1);
		count(connection, "economic_account_mapping",
		      "creating_operation_id=" + id(command.operation_id), 1);
		evidence(connection, command, expected, false);
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
unsigned int
economic_sql_native_mobile_birth_observe_origin(MYSQL *connection,
						const quest_mobile_native_reference &reference,
						native_mobile_wallet_origin *output) noexcept
{
	try
	{
		require(connection && output && quest_mobile_native_reference_valid(reference),
			EINVAL);
		const auto session = mysql_thread_id(connection);
		active(connection, session);
		const auto root = reference.birth_operation;
		const auto stored = read(
			connection,
			"SELECT canonical_plan,canonical_intent FROM economic_accounting_operation WHERE operation_id=" +
				id(root),
			2);
		require(stored[0].has_value() && stored[1].has_value());
		const auto bytes = [](const std::string &value) {
			return std::span<const uint8_t>(
				reinterpret_cast<const uint8_t *>(value.data()), value.size());
		};
		economic_accounting_plan plan;
		economic_frozen_intent intent;
		checked(economic_plan_decode(bytes(*stored[0]), &plan));
		checked(economic_intent_decode(bytes(*stored[1]), &intent));
		std::vector<uint8_t> canonical_plan, canonical_intent;
		checked(economic_plan_encode(plan, &canonical_plan));
		checked(economic_intent_encode(intent, &canonical_intent));
		require(std::equal(canonical_plan.begin(), canonical_plan.end(),
				   bytes(*stored[0]).begin(), bytes(*stored[0]).end()) &&
			std::equal(canonical_intent.begin(), canonical_intent.end(),
				   bytes(*stored[1]).begin(), bytes(*stored[1]).end()));
		const auto &meta = plan.metadata;
		const auto &admission = intent.admission.metadata;
		std::array<uint8_t, ECONOMIC_SOURCE_EVENT_BYTES> plan_source{}, reference_source{},
			intent_source{};
		require(meta.source_event.has_value() && admission.source_event.has_value());
		checked(economic_source_event_encode(*meta.source_event, &plan_source));
		checked(economic_source_event_encode(reference.birth_source, &reference_source));
		checked(economic_source_event_encode(*admission.source_event, &intent_source));
		economic_digest intent_digest{};
		checked(economic_intent_digest(intent, &intent_digest));
		require(meta.operation_id.bytes == root.bytes &&
			critical_operation_id_is_zero(meta.original_operation_id) &&
			meta.actor_kind == economic_actor_kind::domain &&
			meta.actor_id == reference.mobile_instance_id &&
			meta.writer_id == ECONOMIC_WRITER_NATIVE_MOBILE_BIRTH &&
			meta.reason == economic_reason::npc_reward && meta.policy_version == 1 &&
			meta.compiler_version == 1 && plan_source == reference_source &&
			plan_source == intent_source && meta.intent_digest == intent_digest &&
			meta.domain_digest == intent.domain_digest &&
			intent.admission.facts_version == 1 && intent.admission.facts.empty() &&
			meta.version == admission.version &&
			meta.lineage.bytes == admission.lineage.bytes &&
			meta.epoch.bytes == admission.epoch.bytes &&
			meta.operation_id.bytes == admission.operation_id.bytes &&
			meta.original_operation_id.bytes == admission.original_operation_id.bytes &&
			meta.actor_kind == admission.actor_kind &&
			meta.actor_id == admission.actor_id &&
			meta.writer_id == admission.writer_id &&
			meta.policy_version == admission.policy_version &&
			meta.compiler_version == admission.compiler_version &&
			meta.reason == admission.reason);
		const auto receipt = read(
			connection,
			"SELECT result_payload FROM critical_operation_inbox WHERE operation_id=" +
				id(root) +
				" AND status=1 AND result_code=0 AND failure_stage=0 AND durable_revision=1 AND committed_at IS NOT NULL AND command_type=" +
				std::to_string(static_cast<uint16_t>(
					critical_command_type::native_mobile_birth)) +
				" AND schema_version=2 AND payload_version IN (1,2,3)",
			1);
		require(receipt[0].has_value());
		native_mobile_birth_result result;
		require(native_mobile_birth_result_decode(bytes(*receipt[0]), &result) &&
			result.mobile_instance_id == reference.mobile_instance_id &&
			result.plan_digest == hash(canonical_plan));
		const economic_account_key wallet{ meta.lineage, economic_account_kind::wallet,
						   result.wallet_mapping_id,
						   ECONOMIC_NATIVE_MOBILE_WALLET_CONTEXT };
		require(plan.children.empty() && !plan.accounts.empty() &&
			plan.accounts.size() <= 2 &&
			economic_account_key_equal(plan.accounts[0].key, wallet) &&
			plan.accounts[0].before == economic_coin_vector{} &&
			plan.accounts[0].before_revision == 0 &&
			plan.accounts[0].after_revision == result.cash_revision);
		const auto &cash = plan.accounts[0].after;
		const bool nonzero = std::any_of(cash.begin(), cash.end(),
						 [](int64_t amount) { return amount != 0; });
		require(plan.accounts.size() == (nonzero ? 2U : 1U) &&
			plan.postings.size() == (nonzero ? 2U : 0U));
		if (nonzero)
		{
			const economic_account_key issuance{ meta.lineage,
							     economic_account_kind::issuance, 1,
							     0 };
			require(economic_account_key_equal(plan.accounts[1].key, issuance) &&
				plan.accounts[1].before == economic_coin_vector{} &&
				plan.accounts[1].after == economic_coin_vector{} &&
				!plan.accounts[1].before_revision &&
				!plan.accounts[1].after_revision);
			economic_coin_vector opposite{};
			checked(economic_coin_delta(cash, {}, &opposite));
			int64_t value = 0;
			checked(economic_coin_value(cash, &value));
			require(plan.postings[0].event_index == 0 &&
				!plan.postings[0].account_index && !plan.postings[0].child_index &&
				plan.postings[0].delta == cash &&
				plan.postings[0].copper == value &&
				plan.postings[1].event_index == 1 &&
				plan.postings[1].account_index == 1 &&
				!plan.postings[1].child_index &&
				plan.postings[1].delta == opposite &&
				plan.postings[1].copper == -value);
		}
		require(plan.items_before.size() == plan.item_events.size() &&
			plan.items_after.size() == plan.item_events.size());
		for (size_t index = 0; index < plan.item_events.size(); ++index)
		{
			const auto &event = plan.item_events[index];
			require(event.event_index == index && !event.child_index &&
				economic_item_position_equal(event.before,
							     economic_item_position{}) &&
				event.after.owner.type == item_owner_type::native_mobile &&
				event.after.owner.id == reference.mobile_instance_id &&
				!event.after.owner.context_id && event.after.revision == 1 &&
				event.after.state == item_custody_state::active);
		}
		const fields mapping_row{
			{ "mapping_id", std::to_string(result.wallet_mapping_id) },
			{ "lineage", id(meta.lineage) },
			{ "account_kind", "1" },
			{ "context_id", std::to_string(ECONOMIC_NATIVE_MOBILE_WALLET_CONTEXT) },
			{ "backend_kind", "1" },
			{ "locator_kind", std::to_string(ECONOMIC_NATIVE_MOBILE_WALLET_LOCATOR) },
			{ "native_id", std::to_string(reference.mobile_instance_id) },
			{ "creating_operation_id", id(root) }
		};
		count(connection, "economic_account_mapping", predicate(mapping_row), 1);
		count(connection, "economic_account_mapping",
		      "backend_kind=1 AND locator_kind=" +
			      std::to_string(ECONOMIC_NATIVE_MOBILE_WALLET_LOCATOR) +
			      " AND native_id=" + std::to_string(reference.mobile_instance_id),
		      1);
		count(connection, "economic_account_mapping", "creating_operation_id=" + id(root),
		      1);
		// The actual original reset/alchemist factory freezes zone_event. No
		// missing command is reconstructed from these historical observations.
		evidence(connection, root, critical_source_site::zone_event, plan, canonical_intent,
			 false);
		outbox(connection, root, bytes(*receipt[0]), false);
		active(connection, session);
		*output = { root, meta.lineage, meta.epoch, reference.mobile_instance_id,
			    result.wallet_mapping_id };
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
unsigned int economic_sql_native_mobile_birth_lock_publication(
	MYSQL *connection, const critical_command &command, const critical_completion &completion,
	quest_mobile_native_image *image,
	std::vector<item_ownership_runtime_entry> *custody) noexcept
{
	if (!connection || !image || !custody)
		return EINVAL;
	try
	{
		const auto session = mysql_thread_id(connection);
		active(connection, session);
		require(completion.operation_id.bytes == command.operation_id.bytes &&
				completion.disposition ==
					critical_completion_disposition::execution &&
				(completion.outcome == critical_apply_outcome::applied ||
				 completion.outcome == critical_apply_outcome::already_applied) &&
				!completion.error_code &&
				completion.failure_stage == critical_failure_stage::none &&
				completion.durable_revision == 1 &&
				completion.result_size == NATIVE_MOBILE_BIRTH_RESULT_BYTES &&
				std::all_of(completion.result_payload.begin() +
						    NATIVE_MOBILE_BIRTH_RESULT_BYTES,
					    completion.result_payload.end(),
					    [](uint8_t byte) { return !byte; }),
			EILSEQ);
		const std::span<const uint8_t> payload(completion.result_payload.data(),
						       completion.result_size);
		const auto retained = economic_sql_native_mobile_birth_verify_retained(
			connection, command, 0, payload);
		require(!retained, retained);
		auto identity = decode(command);
		native_mobile_birth_result result{};
		require(native_mobile_birth_result_decode(payload, &result), EILSEQ);
		const economic_account_key wallet = { identity.intent.admission.metadata.lineage,
						      economic_account_kind::wallet,
						      result.wallet_mapping_id,
						      ECONOMIC_NATIVE_MOBILE_WALLET_CONTEXT };
		economic_accounting_plan plan;
		checked(native_mobile_birth_accounting_compile(command, wallet, &plan));
		// Preserve original mapping-before-native-before-custody lock order.
		require(read(connection,
			     "SELECT mapping_id FROM economic_account_mapping WHERE " +
				     predicate(mapping(command, identity, wallet.authority_id)) +
				     " FOR UPDATE",
			     1)[0]
				.has_value());
		quest_mobile_native_sql_row native;
		const auto native_error = quest_mobile_native_sql_lock(
			connection, identity.image.reference.mobile_instance_id, &native);
		require(!native_error, native_error);
		require(native.present && native.original_session == session, ESTALE);
		require(read(connection,
			     "SELECT revision FROM item_owner_revision WHERE " +
				     owner(identity.image.reference.mobile_instance_id) +
				     " AND owner_context_id=0 FOR UPDATE",
			     1)[0]
				.has_value());
		const auto values = uids(identity.image);
		verify_current(connection, command, identity, wallet, plan, values);
		std::vector<item_ownership_runtime_entry> verified;
		verified.reserve(plan.item_events.size());
		for (size_t index = 0; index < plan.item_events.size(); ++index)
		{
			const auto &after = plan.item_events[index].after;
			verified.push_back({ plan.item_events[index].uid, after.root_uid,
					     after.parent_uid, after.owner, after.revision,
					     result.item_owner_revision,
					     identity.image.items[index].vnum, after.state });
		}
		active(connection, session);
		*image = std::move(identity.image);
		*custody = std::move(verified);
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

namespace
{
std::vector<cells> native_wallet_mapping_rows(MYSQL *connection,
					      const critical_operation_id &lineage, bool lock)
{
	const economic_sql_source_limits limits{};
	execute(connection,
		"SELECT mapping_id,account_kind,context_id,backend_kind,locator_kind,native_id,"
		"active_native_id,creating_operation_id,retiring_operation_id,revision "
		"FROM economic_account_mapping WHERE lineage=" +
			id(lineage) + " AND (locator_kind=" +
			std::to_string(ECONOMIC_NATIVE_MOBILE_WALLET_LOCATOR) +
			" OR (account_kind=1 AND context_id=" +
			std::to_string(ECONOMIC_NATIVE_MOBILE_WALLET_CONTEXT) +
			")) ORDER BY native_id,mapping_id LIMIT " +
			std::to_string(limits.maximum_rows + 1) + (lock ? " FOR UPDATE" : ""));
	std::unique_ptr<MYSQL_RES, decltype(&mysql_free_result)> result(
		mysql_store_result(connection), mysql_free_result);
	require(bool(result), mysql_errno(connection) ? mysql_errno(connection) : EIO);
	require(mysql_num_fields(result.get()) == 10);
	require(mysql_num_rows(result.get()) <= limits.maximum_rows, E2BIG);
	std::vector<cells> rows;
	rows.reserve(static_cast<size_t>(mysql_num_rows(result.get())));
	uint64_t total_bytes = 0;
	while (auto row = mysql_fetch_row(result.get()))
	{
		const auto lengths = mysql_fetch_lengths(result.get());
		require(lengths != nullptr);
		cells value;
		value.reserve(10);
		for (size_t index = 0; index < 10; ++index)
		{
			const bool operation = index == 7 || index == 8;
			require(lengths[index] <= (operation ? CRITICAL_COMMAND_ID_BYTES : 20),
				E2BIG);
			require(total_bytes <= limits.maximum_cell_bytes - lengths[index], E2BIG);
			total_bytes += lengths[index];
			value.push_back(row[index] ? std::optional<std::string>(std::string(
							     row[index], lengths[index])) :
						     std::nullopt);
		}
		rows.push_back(std::move(value));
	}
	require(!mysql_errno(connection), mysql_errno(connection) ? mysql_errno(connection) : EIO);
	return rows;
}
critical_operation_id native_wallet_operation(const std::optional<std::string> &value)
{
	require(value && value->size() == CRITICAL_COMMAND_ID_BYTES);
	critical_operation_id result{};
	std::copy(value->begin(), value->end(), result.bytes.begin());
	require(!critical_operation_id_is_zero(result));
	return result;
}
critical_native_recovery_envelope native_wallet_origin_peek(MYSQL *connection, uint64_t native_id,
							    const critical_operation_id &creator)
{
	// This is a bounded observation only. The original retained verifier below
	// takes the birth inbox before mapping/native locks; the original 0063
	// reader later locks and re-authenticates these exact origin bytes.
	execute(connection,
		"SELECT birth_operation,publication_revision,OCTET_LENGTH(canonical_origin),"
		"SUBSTRING(canonical_origin,1," +
			std::to_string(CRITICAL_NATIVE_RECOVERY_MAX_ATTACHMENT_BYTES + 1) +
			") FROM quest_mobile_native_birth_origin WHERE mobile_instance_id=" +
			std::to_string(native_id) + " LIMIT 2");
	std::unique_ptr<MYSQL_RES, decltype(&mysql_free_result)> result(
		mysql_store_result(connection), mysql_free_result);
	require(bool(result), mysql_errno(connection) ? mysql_errno(connection) : EIO);
	require(mysql_num_rows(result.get()) == 1 && mysql_num_fields(result.get()) == 4);
	const auto row = mysql_fetch_row(result.get());
	const auto lengths = mysql_fetch_lengths(result.get());
	require(row && lengths && row[0] && row[1] && row[2] && row[3]);
	require(lengths[0] == CRITICAL_COMMAND_ID_BYTES && lengths[1] <= 20 && lengths[2] <= 20);
	require(std::equal(creator.bytes.begin(), creator.bytes.end(),
			   reinterpret_cast<const uint8_t *>(row[0])));
	const auto size = integer<uint64_t>(std::string(row[2], lengths[2]));
	require(size && size <= CRITICAL_NATIVE_RECOVERY_MAX_ATTACHMENT_BYTES && lengths[3] == size,
		E2BIG);
	critical_native_recovery_envelope envelope;
	envelope.phase = critical_native_recovery_phase::continuation_pending;
	envelope.revision = integer<uint64_t>(std::string(row[1], lengths[1]));
	envelope.attachment.assign(reinterpret_cast<const uint8_t *>(row[3]),
				   reinterpret_cast<const uint8_t *>(row[3]) + lengths[3]);
	checked(native_mobile_birth_recovery_original_command_decode(envelope.attachment,
								     &envelope.command));
	require(envelope.command.operation_id.bytes == creator.bytes &&
		native_mobile_birth_recovery_terminal(envelope));
	return envelope;
}
}
unsigned int economic_sql_native_mobile_birth_lock_wallet_lifetimes(
	MYSQL *connection, const critical_operation_id &lineage,
	std::vector<economic_sql_native_mobile_wallet_lifetime> *output) noexcept
{
	try
	{
		require(connection && output && !critical_operation_id_is_zero(lineage), EINVAL);
		const auto session = mysql_thread_id(connection);
		active(connection, session);
		const auto seeds = native_wallet_mapping_rows(connection, lineage, false);
		std::vector<economic_sql_native_mobile_wallet_lifetime> candidate;
		candidate.reserve(seeds.size());
		std::vector<economic_digest> origin_hashes;
		std::vector<uint64_t> origin_revisions;
		origin_hashes.reserve(seeds.size());
		origin_revisions.reserve(seeds.size());
		uint64_t previous_native = 0;
		// Authenticate/lock ALL original birth inboxes before mapping/native locks.
		// Large attachments are discarded per lifetime; only small DTOs survive.
		for (const auto &seed : seeds)
		{
			const auto mapping_id = integer<uint64_t>(seed[0]);
			const auto native_id = integer<uint64_t>(seed[5]);
			const auto creator = native_wallet_operation(seed[7]);
			require(mapping_id && native_id && native_id != UINT64_MAX &&
				native_id > previous_native);
			previous_native = native_id;
			const auto envelope =
				native_wallet_origin_peek(connection, native_id, creator);
			native_mobile_birth_recovery_context recovery;
			checked(native_mobile_birth_recovery_decode(
				envelope.command, envelope.attachment, &recovery));
			const auto retained = economic_sql_native_mobile_birth_verify_retained(
				connection, envelope.command, recovery.receipt.error_code,
				std::span<const uint8_t>(recovery.receipt.result_payload.data(),
							 recovery.receipt.result_size));
			require(!retained, retained);
			quest_mobile_native_image born;
			checked(native_mobile_birth_command_decode(envelope.command, &born));
			require(born.reference.mobile_instance_id == native_id &&
				born.reference.birth_operation.bytes == creator.bytes);
			native_mobile_wallet_origin historical;
			const auto observed = economic_sql_native_mobile_birth_observe_origin(
				connection, born.reference, &historical);
			require(!observed, observed);
			require(historical.lineage.bytes == lineage.bytes &&
				historical.wallet_mapping_id == mapping_id &&
				historical.mobile_instance_id == native_id &&
				historical.birth_operation.bytes == creator.bytes);
			economic_sql_native_mobile_wallet_lifetime value;
			value.account = { lineage, economic_account_kind::wallet, mapping_id,
					  ECONOMIC_NATIVE_MOBILE_WALLET_CONTEXT };
			value.native_id = native_id;
			value.creating_operation_id = creator;
			value.birth_epoch = historical.birth_epoch;
			candidate.push_back(value);
			origin_hashes.push_back(hash(envelope.attachment));
			origin_revisions.push_back(envelope.revision);
			active(connection, session);
		}
		// Under global lifecycle exclusion the complete selected set must remain
		// exact. Damaged native context/locator rows cannot silently disappear.
		const auto locked = native_wallet_mapping_rows(connection, lineage, true);
		require(locked == seeds);
		for (size_t index = 0; index < candidate.size(); ++index)
		{
			auto &value = candidate[index];
			const auto &row = locked[index];
			require(integer<uint16_t>(row[1]) ==
					static_cast<uint16_t>(economic_account_kind::wallet) &&
				integer<uint64_t>(row[2]) ==
					ECONOMIC_NATIVE_MOBILE_WALLET_CONTEXT &&
				integer<uint16_t>(row[3]) == ECONOMIC_MAPPING_BACKEND_SQL &&
				integer<uint16_t>(row[4]) ==
					ECONOMIC_NATIVE_MOBILE_WALLET_LOCATOR &&
				row[6] && integer<uint64_t>(row[6]) == value.native_id && !row[8] &&
				integer<uint64_t>(row[9]) == 0);
			value.active_native_id = value.native_id;
			quest_mobile_native_sql_row current;
			const auto native_error =
				quest_mobile_native_sql_lock(connection, value.native_id, &current);
			require(!native_error, native_error);
			require(current.present && current.original_session == session &&
					current.image.state == quest_mobile_lifetime_state::live &&
					current.image.cash.has_value(),
				ESTALE);
			quest_mobile_native_published_origin published;
			const auto origin_error = quest_mobile_native_origin_sql_lock(
				connection, current.image.reference, &published);
			require(!origin_error, origin_error);
			require(published.present &&
					published.original.command.operation_id.bytes ==
						value.creating_operation_id.bytes &&
					published.original.revision == origin_revisions[index] &&
					hash(published.original.attachment) == origin_hashes[index],
				EILSEQ);
			value.balance = current.image.cash->denominations.amount;
			value.native_revision = current.image.cash->revision;
			value.native_state = current.image.state;
			require(value.native_revision);
			int64_t copper = 0;
			checked(economic_coin_value(value.balance, &copper));
			require(std::all_of(value.balance.begin(), value.balance.end(),
					    [](int64_t amount) { return amount >= 0; }));
			active(connection, session);
		}
		active(connection, session);
		static_assert(std::is_nothrow_move_assignable_v<decltype(candidate)>);
		*output = std::move(candidate);
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
