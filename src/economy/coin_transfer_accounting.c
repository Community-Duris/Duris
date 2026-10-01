#include "economy/coin_transfer_accounting.h"
#include "economy/economic_accounting_intent.h"
#include "economy/economic_accounting_plan.h"
#include "item/economic_accounting_item_reference.h"

#include <algorithm>
#include <array>
#include <charconv>
#include <cerrno>
#include <cstring>
#include <memory>
#include <new>
#include <optional>
#include <openssl/sha.h>
#include <string>
#include <utility>
#include <vector>

namespace
{
constexpr uint16_t PLAYER_LOCATOR = 1;
constexpr uint32_t CURRENCY_CHILD_DOMAIN = COIN_TRANSFER_OPERATION_DOMAIN;
struct failure
{
	unsigned int code;
};
void require(bool condition, unsigned int code = EILSEQ)
{
	if (!condition)
		throw failure{ code };
}
void checked(economic_accounting_error error)
{
	require(error == economic_accounting_error::ok,
		error == economic_accounting_error::capacity ? ENOMEM : EINVAL);
}
#ifndef __NO_MYSQL__
std::string hex(std::span<const uint8_t> data)
{
	static constexpr char digits[] = "0123456789abcdef";
	std::string value = "X'";
	value.reserve(data.size() * 2 + 3);
	for (const auto byte : data)
	{
		value += digits[byte >> 4];
		value += digits[byte & 15];
	}
	value += '\'';
	return value;
}
std::string id(const critical_operation_id &value)
{
	return hex(value.bytes);
}
#endif
uint64_t little_u64(std::span<const uint8_t> bytes, size_t offset)
{
	uint64_t value = 0;
	for (size_t byte = 0; byte < 8; ++byte)
		value |= uint64_t(bytes[offset + byte]) << (8 * byte);
	return value;
}
void append_u64(std::vector<uint8_t> *bytes, uint64_t value)
{
	for (size_t byte = 0; byte < 8; ++byte)
		bytes->push_back(static_cast<uint8_t>(value >> (byte * 8)));
}
#ifndef __NO_MYSQL__
void execute(MYSQL *connection, const std::string &sql)
{
	if (mysql_real_query(connection, sql.data(), sql.size()))
		throw failure{ mysql_errno(connection) ? mysql_errno(connection) : EIO };
}
using cells = std::vector<std::optional<std::string>>;
cells read(MYSQL *connection, const std::string &sql, size_t columns)
{
	execute(connection, sql);
	std::unique_ptr<MYSQL_RES, decltype(&mysql_free_result)> result(
		mysql_store_result(connection), mysql_free_result);
	require(bool(result), mysql_errno(connection) ? mysql_errno(connection) : EIO);
	require(mysql_num_rows(result.get()) == 1, ENOENT);
	auto row = mysql_fetch_row(result.get());
	auto lengths = mysql_fetch_lengths(result.get());
	require(row && lengths && mysql_num_fields(result.get()) == columns);
	cells values;
	for (size_t index = 0; index < columns; ++index)
		values.push_back(row[index] ? std::optional<std::string>(
						      std::string(row[index], lengths[index])) :
					      std::nullopt);
	return values;
}
std::optional<cells> read_optional(MYSQL *connection, const std::string &sql, size_t columns)
{
	execute(connection, sql);
	std::unique_ptr<MYSQL_RES, decltype(&mysql_free_result)> result(
		mysql_store_result(connection), mysql_free_result);
	require(bool(result), mysql_errno(connection) ? mysql_errno(connection) : EIO);
	require(mysql_num_fields(result.get()) == columns && mysql_num_rows(result.get()) <= 1,
		EILSEQ);
	if (!mysql_num_rows(result.get()))
		return std::nullopt;
	auto row = mysql_fetch_row(result.get());
	auto lengths = mysql_fetch_lengths(result.get());
	require(row && lengths, EIO);
	cells values;
	for (size_t index = 0; index < columns; ++index)
		values.push_back(row[index] ? std::optional<std::string>(
						      std::string(row[index], lengths[index])) :
					      std::nullopt);
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
void count(MYSQL *connection, const std::string &table, const std::string &where, uint64_t expected)
{
	require(integer<uint64_t>(read(connection,
				       "SELECT COUNT(*) FROM " + table + " WHERE " + where,
				       1)[0]) == expected);
}
std::string predicate(const std::vector<std::pair<std::string, std::string>> &values)
{
	std::string sql;
	for (const auto &[name, value] : values)
	{
		if (!sql.empty())
			sql += " AND ";
		sql += name + " <=> " + value;
	}
	return sql;
}
void insert(MYSQL *connection, const std::string &table,
	    const std::vector<std::pair<std::string, std::string>> &values)
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
	require(mysql_affected_rows(connection) == 1);
}
void insert_source_claim(MYSQL *connection,
			 const std::vector<std::pair<std::string, std::string>> &values)
{
	try
	{
		insert(connection, "economic_accounting_source_claim", values);
	}
	catch (const failure &error)
	{
		if (error.code == 1062)
			throw failure{ EEXIST };
		throw;
	}
}
void coin_fields(std::vector<std::pair<std::string, std::string>> *values,
		 const std::string &prefix, const economic_coin_vector &coins)
{
	static constexpr std::array<const char *, 4> names = { "copper", "silver", "gold",
							       "platinum" };
	for (size_t index = 0; index < names.size(); ++index)
		values->emplace_back(prefix + names[index], std::to_string(coins[index]));
}
std::optional<economic_account_effect>
prior_pile_effect(MYSQL *connection, const critical_operation_id &lineage,
		  const critical_operation_id &epoch, const economic_account_key &account,
		  const critical_operation_id &exclude_operation_id, uint64_t maximum_revision)
{
	std::array<uint8_t, ECONOMIC_ACCOUNT_KEY_BYTES> encoded = {};
	checked(economic_account_key_encode(account, &encoded));
	const auto row = read_optional(
		connection,
		"SELECT e.after_copper,e.after_silver,e.after_gold,e.after_platinum,e.after_revision "
		"FROM economic_accounting_account_effect e "
		"JOIN economic_accounting_operation o ON o.operation_id=e.operation_id "
		"WHERE e.account_key=" +
			hex(encoded) + " AND o.lineage=" + id(lineage) +
			" AND o.epoch=" + id(epoch) + " AND o.outcome=1 AND o.operation_id<>" +
			id(exclude_operation_id) +
			" AND e.after_revision<=" + std::to_string(maximum_revision) +
			" ORDER BY e.after_revision DESC LIMIT 1 FOR UPDATE",
		5);
	if (!row)
		return std::nullopt;
	economic_account_effect effect = {};
	effect.key = account;
	for (size_t index = 0; index < effect.after.size(); ++index)
		effect.after[index] = integer<int64_t>((*row)[index]);
	effect.after_revision = integer<uint64_t>((*row)[4]);
	return effect;
}
economic_item_position current_item_position(MYSQL *connection, uint64_t uid)
{
	const auto row =
		read(connection,
		     "SELECT root_item_uid,parent_item_uid,owner_type,owner_id,owner_context_id,"
		     "item_revision,state FROM item_current_owner WHERE item_uid=" +
			     std::to_string(uid) + " FOR UPDATE",
		     7);
	economic_item_position position = {};
	position.root_uid = integer<uint64_t>(row[0]);
	position.parent_uid = row[1] ? integer<uint64_t>(row[1]) : 0;
	position.owner.type = static_cast<item_owner_type>(integer<uint8_t>(row[2]));
	position.owner.id = integer<uint64_t>(row[3]);
	position.owner.context_id = integer<uint64_t>(row[4]);
	position.revision = integer<uint64_t>(row[5]);
	position.state = static_cast<item_custody_state>(integer<uint8_t>(row[6]));
	return position;
}
void append_item_witness(economic_accounting_plan *plan, uint64_t uid,
			 const economic_item_position &position)
{
	const auto found = std::find_if(plan->items_before.begin(), plan->items_before.end(),
					[uid](const economic_item_snapshot &item)
					{ return item.uid == uid; });
	if (found != plan->items_before.end())
	{
		require(economic_item_position_equal(found->position, position), EILSEQ);
		return;
	}
	plan->items_before.push_back({ uid, position });
	plan->items_after.push_back({ uid, position });
}
void append_coin_pile_ancestors(MYSQL *connection, economic_accounting_plan *plan,
				const item_transfer_payload &pile)
{
	uint64_t uid = pile.items[0].parent_item_uid;
	for (size_t depth = 0; uid && depth < ECONOMIC_ACCOUNTING_MAX_ITEM_WITNESSES; ++depth)
	{
		const auto position = current_item_position(connection, uid);
		append_item_witness(plan, uid, position);
		uid = position.parent_uid;
	}
	require(!uid, ELOOP);
}
#endif
struct identity
{
	economic_frozen_intent intent;
	coin_transfer_payload payload;
	economic_account_key source, destination;
	currency_command_payload source_change, destination_change;
};
economic_account_kind endpoint_account_kind(const coin_transfer_endpoint &endpoint)
{
	if (endpoint.change.type == critical_command_type::account_bank)
		return economic_account_kind::wallet;
	if (endpoint.change.type == critical_command_type::item_transfer)
		return economic_account_kind::pile;
	throw failure{ EACCES };
}
uint64_t endpoint_authority_id(const coin_transfer_endpoint &endpoint)
{
	if (endpoint.change.type == critical_command_type::account_bank)
	{
		currency_command_payload change = {};
		require(currency_command_decode_payload(endpoint.change, &change), EINVAL);
		return change.pid;
	}
	item_transfer_payload change = {};
	require(endpoint.change.type == critical_command_type::item_transfer &&
			item_transfer_command_decode_payload(endpoint.change, &change) &&
			change.item_count == 1 &&
			change.selected_item_uid == change.items[0].item_uid,
		EINVAL);
	return change.selected_item_uid;
}
economic_coin_vector coin_vector(const std::array<int32_t, 4> &values)
{
	return { values[0], values[1], values[2], values[3] };
}
critical_command admission_projection(const critical_command &root)
{
	auto projection = root;
	projection.schema_version = CRITICAL_COMMAND_SCHEMA_VERSION;
	projection.accounting_intent.clear();
	projection.publication_required = false;
	return projection;
}
identity decode(const critical_command &root)
{
	require(root.schema_version == CRITICAL_COMMAND_ACCOUNTING_SCHEMA_VERSION &&
			root.type == critical_command_type::coin_transfer &&
			critical_command_envelope_valid(root),
		EPROTONOSUPPORT);
	identity value;
	require(coin_transfer_command_decode_payload(root, &value.payload), EINVAL);
	const coin_transfer_endpoint *endpoints[] = { &value.payload.source,
						      &value.payload.destination };
	for (size_t index = 0; index < 2; ++index)
	{
		const auto &endpoint = *endpoints[index];
		if (endpoint_account_kind(endpoint) == economic_account_kind::wallet)
		{
			auto *change = index ? &value.destination_change : &value.source_change;
			require(currency_command_decode_payload(endpoint.change, change) &&
					change->reason == currency_reason_type::coin_transfer &&
					change->pid > 0 && change->pid <= INT32_MAX &&
					endpoint.change.keys.size() == 2 &&
					endpoint.change.expected_revisions.size() == 2 &&
					endpoint.change.keys[0].type ==
						critical_entity_type::player &&
					endpoint.change.keys[0].id == change->pid &&
					endpoint.change.expected_revisions[0].revision !=
						UINT64_MAX &&
					endpoint.change.expected_revisions[1].revision !=
						UINT64_MAX,
				EINVAL);
			require(std::all_of(change->bank_delta.amount.begin(),
					    change->bank_delta.amount.end(),
					    [](int64_t amount) { return amount == 0; }),
				EACCES);
		}
	}
	checked(economic_intent_decode(root.accounting_intent, &value.intent));
	const auto &facts = value.intent.admission.facts;
	require(facts.size() == ECONOMIC_COIN_TRANSFER_FACT_BYTES, EINVAL);
	const auto &metadata = value.intent.admission.metadata;
	value.source = { metadata.lineage, endpoint_account_kind(value.payload.source),
			 little_u64(facts, 0), 0 };
	value.destination = { metadata.lineage, endpoint_account_kind(value.payload.destination),
			      little_u64(facts, 8), 0 };
	if (value.source.kind == economic_account_kind::pile)
		require(value.source.authority_id == endpoint_authority_id(value.payload.source),
			EACCES);
	if (value.destination.kind == economic_account_kind::pile)
		require(value.destination.authority_id ==
				endpoint_authority_id(value.payload.destination),
			EACCES);
	std::vector<uint8_t> expected;
	checked(coin_transfer_accounting_intent(admission_projection(root), metadata.epoch,
						value.source, value.destination, &expected));
	require(expected == root.accounting_intent, EACCES);
	return value;
}
void validate_wallet_endpoint(const coin_transfer_endpoint &endpoint, size_t index,
			      const critical_command &destination_after_source,
			      const currency_command_result &wallet,
			      economic_account_effect *effect)
{
	require(effect && endpoint.change.type == critical_command_type::account_bank &&
			endpoint.change.expected_revisions.size() == 2,
		EILSEQ);
	currency_command_payload change = {};
	require(currency_command_decode_payload(endpoint.change, &change), EILSEQ);
	const uint64_t expected_wallet_revision = endpoint.change.expected_revisions[0].revision;
	const uint64_t expected_bank_revision =
		index == 1 ? destination_after_source.expected_revisions[1].revision :
			     endpoint.change.expected_revisions[1].revision;
	require(expected_wallet_revision != UINT64_MAX && expected_bank_revision != UINT64_MAX,
		EILSEQ);
	if (index == 0)
		require(wallet.bank_revision == endpoint.change.expected_revisions[1].revision + 1,
			EILSEQ);
	else
		require(wallet.bank_revision == expected_bank_revision + 1, EILSEQ);
	effect->before = coin_vector(endpoint.before);
	effect->after = coin_vector(endpoint.after);
	require(wallet.wallet_revision == expected_wallet_revision + 1 &&
			wallet.wallet.amount == effect->after,
		EILSEQ);
	for (size_t denomination = 0; denomination < 4; ++denomination)
		require(effect->after[denomination] - effect->before[denomination] ==
				change.wallet_delta.amount[denomination],
			EILSEQ);
	effect->before_revision = expected_wallet_revision;
	effect->after_revision = wallet.wallet_revision;
}
[[maybe_unused]] void validate_payload(const coin_transfer_payload &payload,
				       const economic_account_key &source,
				       const economic_account_key &destination,
				       const currency_command_result *results,
				       economic_accounting_plan *plan)
{
	require(results && plan, EINVAL);
	coin_transfer_result result = {};
	result.wallets[0] = results[0];
	result.wallets[1] = results[1];
	critical_command destination_after_source = {};
	require(coin_transfer_command_destination_after_source(payload, result,
							       &destination_after_source),
		EILSEQ);
	const coin_transfer_endpoint *endpoints[] = { &payload.source, &payload.destination };
	const economic_account_key accounts[] = { source, destination };
	for (size_t index = 0; index < 2; ++index)
	{
		economic_account_effect effect = {};
		effect.key = accounts[index];
		validate_wallet_endpoint(*endpoints[index], index, destination_after_source,
					 results[index], &effect);
		plan->accounts.push_back(effect);
	}
}
#ifndef __NO_MYSQL__
economic_accounting_plan make_plan(MYSQL *connection, const critical_command &root,
				   const identity &value, const coin_transfer_result &result)
{
	require(connection, EINVAL);
	economic_accounting_plan plan;
	checked(economic_intent_plan_metadata(root, value.intent, &plan.metadata));
	const coin_transfer_endpoint *endpoints[] = { &value.payload.source,
						      &value.payload.destination };
	const economic_account_key accounts[] = { value.source, value.destination };
	critical_command destination_after_source = {};
	require(coin_transfer_command_destination_after_source(value.payload, result,
							       &destination_after_source),
		EILSEQ);
	for (size_t index = 0; index < 2; ++index)
	{
		const auto &endpoint = *endpoints[index];
		economic_child_link child;
		child.operation_id = endpoint.change.operation_id;
		child.domain = CURRENCY_CHILD_DOMAIN;
		child.discriminator = index;
		plan.children.push_back(child);
		economic_account_effect effect = {};
		effect.key = accounts[index];
		effect.before = coin_vector(endpoint.before);
		effect.after = coin_vector(endpoint.after);
		if (endpoint_account_kind(endpoint) == economic_account_kind::wallet)
		{
			validate_wallet_endpoint(endpoint, index, destination_after_source,
						 result.wallets[index], &effect);
		}
		else
		{
			item_transfer_payload pile = {};
			require(item_transfer_command_decode_payload(endpoint.change, &pile) &&
					pile.item_count == 1,
				EILSEQ);
			const auto &entry = pile.items[0];
			const bool creation = pile.from_owner.type == item_owner_type::system;
			const auto &pile_result = result.piles[index];
			const uint64_t item_revision = pile_result.max_item_revision;
			require(pile_result.item_count == 1 &&
					pile_result.root_item_uid == entry.item_uid &&
					item_revision &&
					(creation ?
						 item_revision == 1 :
						 entry.expected_item_revision < UINT64_MAX &&
							 item_revision ==
								 entry.expected_item_revision + 1),
				EILSEQ);
			if (creation)
			{
				require(effect.before == economic_coin_vector{} &&
						!prior_pile_effect(connection,
								   plan.metadata.lineage,
								   plan.metadata.epoch, effect.key,
								   plan.metadata.operation_id,
								   item_revision - 1),
					EEXIST);
				effect.before_revision = 0;
			}
			else
			{
				const auto prior = prior_pile_effect(
					connection, plan.metadata.lineage, plan.metadata.epoch,
					effect.key, plan.metadata.operation_id,
					entry.expected_item_revision);
				require(prior.has_value(), ENODATA);
				require(prior->after == effect.before, EILSEQ);
				effect.before_revision = prior->after_revision;
			}
			effect.after_revision = item_revision;
			require(effect.after_revision > effect.before_revision, EILSEQ);

			uint64_t target_root = 0, target_parent = 0;
			require(item_transfer_target_topology(pile, entry.item_uid, &target_root,
							      &target_parent),
				EILSEQ);
			economic_item_position before = {};
			if (!creation)
			{
				before.owner = pile.from_owner;
				before.root_uid = entry.root_item_uid;
				before.parent_uid = entry.parent_item_uid;
				before.revision = entry.expected_item_revision;
				before.state = item_custody_state::active;
			}
			economic_item_position after = {};
			after.owner = pile.to_owner;
			after.root_uid = target_root;
			after.parent_uid = target_parent;
			after.revision = item_revision;
			after.state = pile.to_owner.type == item_owner_type::destruction ?
					      item_custody_state::destroyed :
					      item_custody_state::active;
			plan.items_before.push_back({ entry.item_uid, before });
			plan.items_after.push_back({ entry.item_uid, after });
			append_coin_pile_ancestors(connection, &plan, pile);
			plan.item_events.push_back({ static_cast<uint32_t>(plan.item_events.size()),
						     static_cast<uint16_t>(index + 1),
						     entry.item_uid, before, after });
		}
		plan.accounts.push_back(effect);
		economic_coin_posting posting;
		posting.event_index = index;
		posting.account_index = index;
		posting.child_index = static_cast<uint16_t>(index + 1);
		for (size_t denomination = 0; denomination < 4; ++denomination)
			posting.delta[denomination] =
				effect.after[denomination] - effect.before[denomination];
		checked(economic_coin_value(posting.delta, &posting.copper));
		plan.postings.push_back(posting);
	}
	checked(economic_plan_normalize(&plan));
	return plan;
}
std::vector<std::pair<std::string, std::string>>
operation_fields(const critical_command &root, const economic_frozen_intent &intent,
		 unsigned int result_code, const economic_accounting_plan *plan)
{
	economic_plan_metadata metadata;
	checked(economic_intent_plan_metadata(root, intent, &metadata));
	std::string source_event = "NULL";
	if (metadata.source_event)
	{
		std::array<uint8_t, ECONOMIC_SOURCE_EVENT_BYTES> encoded = {};
		checked(economic_source_event_encode(*metadata.source_event, &encoded));
		source_event = hex(encoded);
	}
	std::vector<uint8_t> encoded;
	if (plan)
		checked(economic_plan_encode(*plan, &encoded));
	economic_digest plan_digest = {};
	if (plan)
		SHA256(encoded.data(), encoded.size(), plan_digest.data());
	return { { "operation_id", id(root.operation_id) },
		 { "lineage", id(metadata.lineage) },
		 { "epoch", id(metadata.epoch) },
		 { "original_operation_id", "NULL" },
		 { "accounting_version", std::to_string(metadata.version) },
		 { "writer_id", std::to_string(metadata.writer_id) },
		 { "policy_version", std::to_string(metadata.policy_version) },
		 { "compiler_version", std::to_string(metadata.compiler_version) },
		 { "actor_kind", std::to_string(static_cast<uint8_t>(metadata.actor_kind)) },
		 { "actor_id", std::to_string(metadata.actor_id) },
		 { "reason", std::to_string(static_cast<uint16_t>(metadata.reason)) },
		 { "source_event", source_event },
		 { "intent_digest", hex(metadata.intent_digest) },
		 { "domain_digest", hex(metadata.domain_digest) },
		 { "plan_digest", plan ? hex(plan_digest) : "NULL" },
		 { "canonical_intent", hex(root.accounting_intent) },
		 { "canonical_plan", plan ? hex(encoded) : "NULL" },
		 { "outcome", result_code ? "2" : "1" },
		 { "result_code", std::to_string(result_code) },
		 { "account_count", plan ? std::to_string(plan->accounts.size()) : "0" },
		 { "posting_count", plan ? std::to_string(plan->postings.size()) : "0" },
		 { "child_count", plan ? std::to_string(plan->children.size()) : "0" },
		 { "item_event_count", plan ? std::to_string(plan->item_events.size()) : "0" },
		 { "before_witness_count", plan ? std::to_string(plan->items_before.size()) : "0" },
		 { "after_witness_count", plan ? std::to_string(plan->items_after.size()) : "0" } };
}
std::vector<std::pair<std::string, std::string>>
effect_fields(const critical_operation_id &root, size_t index,
	      const economic_account_effect &effect)
{
	std::array<uint8_t, ECONOMIC_ACCOUNT_KEY_BYTES> key = {};
	checked(economic_account_key_encode(effect.key, &key));
	std::vector<std::pair<std::string, std::string>> values = { { "operation_id", id(root) },
								    { "account_index",
								      std::to_string(index) },
								    { "account_key", hex(key) } };
	coin_fields(&values, "before_", effect.before);
	coin_fields(&values, "after_", effect.after);
	values.emplace_back("before_revision", std::to_string(effect.before_revision));
	values.emplace_back("after_revision", std::to_string(effect.after_revision));
	return values;
}
std::vector<std::pair<std::string, std::string>>
posting_fields(const critical_operation_id &root, size_t index,
	       const economic_coin_posting &posting)
{
	std::vector<std::pair<std::string, std::string>> values = {
		{ "operation_id", id(root) },
		{ "line_index", std::to_string(index) },
		{ "event_index", std::to_string(posting.event_index) },
		{ "account_index", std::to_string(posting.account_index) },
		{ "child_index", std::to_string(posting.child_index) },
		{ "copper_value", std::to_string(posting.copper) }
	};
	coin_fields(&values, "delta_", posting.delta);
	return values;
}
void verify_plan_rows(MYSQL *connection, const critical_command &root,
		      const economic_accounting_plan *plan, bool append)
{
	const std::string where = "operation_id=" + id(root.operation_id);
	const bool has_source_claim = plan && plan->metadata.source_event.has_value();
	if (has_source_claim)
	{
		std::array<uint8_t, ECONOMIC_SOURCE_EVENT_BYTES> encoded = {};
		checked(economic_source_event_encode(*plan->metadata.source_event, &encoded));
		const auto claim = std::vector<std::pair<std::string, std::string>>{
			{ "lineage", id(plan->metadata.lineage) },
			{ "source_event", hex(encoded) },
			{ "operation_id", id(root.operation_id) },
			{ "outcome", "1" }
		};
		if (append)
			insert_source_claim(connection, claim);
		count(connection, "economic_accounting_source_claim", predicate(claim), 1);
	}
	if (plan)
	{
		for (size_t index = 0; index < plan->children.size(); ++index)
		{
			const auto &child = plan->children[index];
			const auto values = std::vector<std::pair<std::string, std::string>>{
				{ "operation_id", id(root.operation_id) },
				{ "child_index", std::to_string(index + 1) },
				{ "child_operation_id", id(child.operation_id) },
				{ "domain_id", std::to_string(child.domain) },
				{ "discriminator", std::to_string(child.discriminator) },
				{ "parent_index", std::to_string(child.parent_index) },
				{ "relationship", std::to_string(child.relationship) },
				{ "receipt_operation_id", id(child.operation_id) }
			};
			if (append)
				insert(connection, "economic_accounting_child", values);
			count(connection, "economic_accounting_child", predicate(values), 1);
		}
		for (size_t index = 0; index < plan->accounts.size(); ++index)
		{
			const auto values =
				effect_fields(root.operation_id, index, plan->accounts[index]);
			if (append)
				insert(connection, "economic_accounting_account_effect", values);
			count(connection, "economic_accounting_account_effect", predicate(values),
			      1);
		}
		for (size_t index = 0; index < plan->postings.size(); ++index)
		{
			const auto values =
				posting_fields(root.operation_id, index, plan->postings[index]);
			if (append)
				insert(connection, "economic_accounting_coin_posting", values);
			count(connection, "economic_accounting_coin_posting", predicate(values), 1);
		}
	}
	count(connection, "economic_accounting_child", where, plan ? plan->children.size() : 0);
	count(connection, "economic_accounting_account_effect", where,
	      plan ? plan->accounts.size() : 0);
	count(connection, "economic_accounting_coin_posting", where,
	      plan ? plan->postings.size() : 0);
	count(connection, "economic_accounting_item_reference", where,
	      plan ? plan->item_events.size() : 0);
	count(connection, "economic_accounting_source_claim", where, has_source_claim ? 1 : 0);
	count(connection, "item_ownership_ledger", where, 0);
}
void verify_item_reference_rows(MYSQL *connection, const critical_command &root,
				const identity &value, const coin_transfer_result &result,
				const economic_accounting_plan &plan, bool append)
{
	uint16_t line_index = 0;
	for (size_t endpoint_index = 0; endpoint_index < 2; ++endpoint_index)
	{
		const auto &endpoint = endpoint_index ? value.payload.destination :
							value.payload.source;
		if (endpoint.change.type != critical_command_type::item_transfer)
			continue;
		item_transfer_payload pile = {};
		require(item_transfer_command_decode_payload(endpoint.change, &pile) &&
				pile.item_count == 1,
			EILSEQ);
		const auto &entry = pile.items[0];
		const uint64_t after_revision = result.piles[endpoint_index].max_item_revision;
		const uint64_t before_revision = pile.from_owner.type == item_owner_type::system ?
							 0 :
							 entry.expected_item_revision;
		uint64_t target_root = 0, target_parent = 0;
		require(after_revision &&
				item_transfer_target_topology(pile, entry.item_uid, &target_root,
							      &target_parent),
			EILSEQ);
		const auto &child_id = endpoint.change.operation_id;
		count(connection, "item_ownership_ledger", "operation_id=" + id(child_id), 1);
		const auto &ledger = std::vector<std::pair<std::string, std::string>>{
			{ "operation_id", id(child_id) },
			{ "event_index", "0" },
			{ "item_uid", std::to_string(entry.item_uid) },
			{ "root_item_uid", std::to_string(target_root) },
			{ "parent_item_uid",
			  target_parent ? std::to_string(target_parent) : "NULL" },
			{ "from_owner_type",
			  std::to_string(static_cast<uint8_t>(pile.from_owner.type)) },
			{ "from_owner_id", std::to_string(pile.from_owner.id) },
			{ "from_owner_context_id", std::to_string(pile.from_owner.context_id) },
			{ "to_owner_type",
			  std::to_string(static_cast<uint8_t>(pile.to_owner.type)) },
			{ "to_owner_id", std::to_string(pile.to_owner.id) },
			{ "to_owner_context_id", std::to_string(pile.to_owner.context_id) },
			{ "item_revision", std::to_string(after_revision) },
			{ "from_owner_revision",
			  std::to_string(result.piles[endpoint_index].from_owner_revision) },
			{ "to_owner_revision",
			  std::to_string(result.piles[endpoint_index].to_owner_revision) },
			{ "reason_type", std::to_string(static_cast<uint16_t>(pile.reason)) },
			{ "reason_id", std::to_string(pile.reason_id) },
			{ "source_site",
			  std::to_string(static_cast<uint16_t>(endpoint.change.source_site)) }
		};
		count(connection, "item_ownership_ledger", predicate(ledger), 1);

		economic_accounting_item_reference ref = {};
		ref.operation_id = root.operation_id;
		const auto event = std::find_if(plan.item_events.begin(), plan.item_events.end(),
						[&](const economic_item_event &candidate)
						{ return candidate.uid == entry.item_uid; });
		require(event != plan.item_events.end() && event->event_index == line_index,
			EILSEQ);
		ref.line_index = static_cast<uint16_t>(event->event_index);
		ref.event_index = event->event_index;
		ref.child_index = event->child_index;
		ref.item_uid = entry.item_uid;
		ref.before_revision = before_revision;
		ref.after_revision = after_revision;
		ref.legacy_operation_id = child_id;
		ref.legacy_event_index = 0;
		require(economic_accounting_item_reference_validate(ref), EILSEQ);
		if (append)
			require(economic_accounting_item_reference_insert(connection, ref),
				mysql_errno(connection) ? mysql_errno(connection) : EIO);
		const auto reference = std::vector<std::pair<std::string, std::string>>{
			{ "operation_id", id(ref.operation_id) },
			{ "line_index", std::to_string(ref.line_index) },
			{ "event_index", std::to_string(ref.event_index) },
			{ "child_index", std::to_string(ref.child_index) },
			{ "item_uid", std::to_string(ref.item_uid) },
			{ "before_revision", std::to_string(ref.before_revision) },
			{ "after_revision", std::to_string(ref.after_revision) },
			{ "legacy_operation_id", id(ref.legacy_operation_id) },
			{ "legacy_event_index", std::to_string(ref.legacy_event_index) }
		};
		count(connection, "economic_accounting_item_reference", predicate(reference), 1);
		++line_index;
	}
	count(connection, "economic_accounting_item_reference",
	      "operation_id=" + id(root.operation_id), line_index);
}
unsigned int error_code(const failure &error)
{
	return error.code;
}
void lock_native_wallet_before(MYSQL *connection, const coin_transfer_endpoint &endpoint)
{
	currency_command_payload command = {};
	require(currency_command_decode_payload(endpoint.change, &command), EINVAL);
	const auto row = read(
		connection,
		"SELECT copper,silver,gold,platinum,wallet_revision FROM player_data WHERE pid=" +
			std::to_string(command.pid) + " FOR UPDATE",
		5);
	const auto revision = integer<uint64_t>(row[4]);
	if (revision == endpoint.change.expected_revisions[0].revision)
	{
		for (size_t denomination = 0; denomination < 4; ++denomination)
			require(integer<int64_t>(row[denomination]) ==
					endpoint.before[denomination],
				EILSEQ);
	}
}
std::vector<economic_sql_mapping_request> wallet_mapping_requests(const identity &value)
{
	std::vector<economic_sql_mapping_request> requests;
	const coin_transfer_endpoint *endpoints[] = { &value.payload.source,
						      &value.payload.destination };
	const economic_account_key accounts[] = { value.source, value.destination };
	const currency_command_payload changes[] = { value.source_change,
						     value.destination_change };
	for (size_t index = 0; index < 2; ++index)
		if (endpoints[index]->change.type == critical_command_type::account_bank)
			requests.push_back({ accounts[index], PLAYER_LOCATOR, changes[index].pid });
	return requests;
}
void require_pile_account_history(MYSQL *connection, const economic_plan_metadata &metadata,
				  const coin_transfer_endpoint &endpoint,
				  const economic_account_key &account)
{
	item_transfer_payload pile = {};
	require(endpoint.change.type == critical_command_type::item_transfer &&
			item_transfer_command_decode_payload(endpoint.change, &pile) &&
			pile.item_count == 1,
		EINVAL);
	const auto &entry = pile.items[0];
	if (pile.from_owner.type == item_owner_type::system)
	{
		require(endpoint.before == std::array<int32_t, 4>{}, EILSEQ);
		require(!prior_pile_effect(connection, metadata.lineage, metadata.epoch, account,
					   metadata.operation_id, UINT64_MAX),
			EEXIST);
		return;
	}
	const auto prior = prior_pile_effect(connection, metadata.lineage, metadata.epoch, account,
					     metadata.operation_id, entry.expected_item_revision);
	require(prior.has_value(), ENODATA);
}
#endif

} // namespace

economic_accounting_error
coin_transfer_accounting_intent(const critical_command &root, const critical_operation_id &epoch,
				const economic_account_key &source_account,
				const economic_account_key &destination_account,
				std::vector<uint8_t> *encoded)
{
	using error = economic_accounting_error;
	if (!encoded || critical_operation_id_is_zero(epoch) ||
	    !economic_account_key_valid(source_account) ||
	    !economic_account_key_valid(destination_account) || source_account.context_id ||
	    destination_account.context_id ||
	    source_account.lineage.bytes != destination_account.lineage.bytes ||
	    economic_account_key_equal(source_account, destination_account))
		return error::invalid_identity;
	coin_transfer_payload payload;
	if (!coin_transfer_command_decode_payload(root, &payload))
		return error::unauthorized;
	const coin_transfer_endpoint *endpoints[] = { &payload.source, &payload.destination };
	const economic_account_key *accounts[] = { &source_account, &destination_account };
	for (size_t index = 0; index < 2; ++index)
	{
		const auto &endpoint = *endpoints[index];
		const auto &account = *accounts[index];
		const auto expected_kind =
			endpoint.change.type == critical_command_type::account_bank ?
				economic_account_kind::wallet :
			endpoint.change.type == critical_command_type::item_transfer ?
				economic_account_kind::pile :
				economic_account_kind{};
		if (account.kind != expected_kind ||
		    (account.kind != economic_account_kind::wallet &&
		     account.kind != economic_account_kind::pile))
			return error::unauthorized;
		if (endpoint.change.type == critical_command_type::account_bank)
		{
			currency_command_payload change = {};
			if (!currency_command_decode_payload(endpoint.change, &change) ||
			    endpoint.change.keys.size() != 2 ||
			    endpoint.change.keys[0].type != critical_entity_type::player ||
			    change.pid != endpoint.change.keys[0].id ||
			    change.reason != currency_reason_type::coin_transfer ||
			    endpoint.change.expected_revisions.size() != 2 ||
			    endpoint.change.expected_revisions[0].revision == UINT64_MAX ||
			    endpoint.change.expected_revisions[1].revision == UINT64_MAX ||
			    std::any_of(change.bank_delta.amount.begin(),
					change.bank_delta.amount.end(),
					[](int64_t amount) { return amount != 0; }))
				return error::unauthorized;
		}
		else
		{
			item_transfer_payload pile = {};
			if (!item_transfer_command_decode_payload(endpoint.change, &pile) ||
			    pile.item_count != 1 ||
			    pile.selected_item_uid != account.authority_id ||
			    pile.items[0].item_uid != account.authority_id)
				return error::unauthorized;
		}
	}
	try
	{
		economic_admission_facts facts;
		facts.metadata.lineage = source_account.lineage;
		facts.metadata.epoch = epoch;
		facts.metadata.actor_kind = economic_actor_kind::domain;
		facts.metadata.actor_id = source_account.authority_id;
		facts.metadata.writer_id = ECONOMIC_WRITER_WALLET_COIN_TRANSFER;
		facts.metadata.reason = economic_reason::coin_transfer;
		bool source_retired = false;
		bool destination_created = false;
		for (size_t index = 0; index < 2; ++index)
		{
			const auto *endpoint = endpoints[index];
			if (endpoint->change.type != critical_command_type::item_transfer)
				continue;
			item_transfer_payload pile = {};
			if (!item_transfer_command_decode_payload(endpoint->change, &pile) ||
			    pile.item_count != 1)
				return error::unauthorized;
			if (index == 0 && pile.to_owner.type == item_owner_type::destruction)
				source_retired = true;
			if (index == 1 && pile.from_owner.type == item_owner_type::system)
				destination_created = true;
		}
		economic_source_event source_event = {};
		uint64_t source_revision = 0;
		if (payload.source.change.type == critical_command_type::account_bank)
			source_revision = payload.source.change.expected_revisions[0].revision;
		else
		{
			item_transfer_payload source_pile = {};
			if (!item_transfer_command_decode_payload(payload.source.change,
								  &source_pile) ||
			    source_pile.item_count != 1)
				return error::unauthorized;
			source_revision = source_pile.items[0].expected_item_revision;
		}
		if (coin_transfer_accounting_source_event(
			    source_account.kind, source_account.authority_id, source_revision,
			    source_retired, destination_created, &source_event))
			facts.metadata.source_event = source_event;
		append_u64(&facts.facts, source_account.authority_id);
		append_u64(&facts.facts, destination_account.authority_id);
		return economic_intent_freeze(root, facts, encoded);
	}
	catch (const std::bad_alloc &)
	{
		return error::capacity;
	}
}

bool coin_transfer_accounting_source_event(economic_account_kind source_kind, uint64_t source_id,
					   uint64_t source_revision, bool source_retired,
					   bool destination_created,
					   economic_source_event *event) noexcept
{
	if (!event ||
	    (source_kind != economic_account_kind::wallet &&
	     source_kind != economic_account_kind::pile) ||
	    !source_id || source_revision == UINT64_MAX ||
	    (!source_retired && !destination_created))
		return false;
	economic_source_event result = {};
	result.kind = economic_source_kind::lifecycle;
	result.source.bytes[0] = static_cast<uint8_t>(source_kind);
	for (size_t byte = 0; byte < 8; ++byte)
		result.source.bytes[1 + byte] = static_cast<uint8_t>(source_id >> (byte * 8));
	std::memcpy(result.source.bytes.data() + 9, "COINACC", 7);
	for (size_t byte = 0; byte < 8; ++byte)
		result.generation.bytes[byte] = static_cast<uint8_t>(source_revision >> (byte * 8));
	std::memcpy(result.generation.bytes.data() + 8, "COINLIFE", 8);
	result.slot = (source_retired ? 1U : 0U) | (destination_created ? 2U : 0U);
	*event = result;
	return true;
}

bool coin_transfer_accounting_command_supported(const critical_command &root) noexcept
{
	try
	{
		(void)decode(root);
		return true;
	}
	catch (...)
	{
		return false;
	}
}

#ifdef __NO_MYSQL__
unsigned int coin_transfer_accounting_lock(MYSQL *, const critical_command &,
					   const coin_transfer_payload &,
					   coin_transfer_accounting_context *)
{
	return ENOTSUP;
}
unsigned int coin_transfer_accounting_record(MYSQL *, const critical_command &,
					     const coin_transfer_result &, unsigned int,
					     const coin_transfer_accounting_context &)
{
	return ENOTSUP;
}
unsigned int coin_transfer_accounting_verify_retained(MYSQL *, const critical_command &,
						      unsigned int, const uint8_t *, size_t)
{
	return ENOTSUP;
}
#else
unsigned int coin_transfer_accounting_lock(MYSQL *connection, const critical_command &root,
					   const coin_transfer_payload &payload,
					   coin_transfer_accounting_context *context)
{
	try
	{
		require(connection && context, EINVAL);
		const auto value = decode(root);
		require(payload.source.change.operation_id.bytes ==
					value.payload.source.change.operation_id.bytes &&
				payload.destination.change.operation_id.bytes ==
					value.payload.destination.change.operation_id.bytes,
			EEXIST);
		const auto &metadata = value.intent.admission.metadata;
		const auto requests = wallet_mapping_requests(value);
		coin_transfer_accounting_context candidate;
		candidate.session_id = mysql_thread_id(connection);
		const auto error = economic_sql_lock_authority(connection, metadata.lineage,
							       metadata.epoch, requests,
							       &candidate.authority);
		require(!error, error);
		require(connection->server_status & SERVER_STATUS_IN_TRANS, ENOTCONN);
		economic_plan_metadata plan_metadata;
		checked(economic_intent_plan_metadata(root, value.intent, &plan_metadata));
		std::vector<const coin_transfer_endpoint *> ordered;
		for (const auto *endpoint : { &value.payload.source, &value.payload.destination })
			if (endpoint->change.type == critical_command_type::account_bank)
				ordered.push_back(endpoint);
		std::sort(ordered.begin(), ordered.end(),
			  [](const auto *left, const auto *right)
			  {
				  currency_command_payload a = {}, b = {};
				  if (!currency_command_decode_payload(left->change, &a) ||
				      !currency_command_decode_payload(right->change, &b))
					  return false;
				  return a.pid < b.pid;
			  });
		for (const auto *endpoint : ordered)
			lock_native_wallet_before(connection, *endpoint);
		const coin_transfer_endpoint *endpoints[] = { &value.payload.source,
							      &value.payload.destination };
		const economic_account_key accounts[] = { value.source, value.destination };
		for (size_t index = 0; index < 2; ++index)
			if (endpoints[index]->change.type == critical_command_type::item_transfer)
				require_pile_account_history(connection, plan_metadata,
							     *endpoints[index], accounts[index]);
		*context = std::move(candidate);
		return 0;
	}
	catch (const failure &error)
	{
		return error_code(error);
	}
	catch (const std::bad_alloc &)
	{
		return ENOMEM;
	}
}

unsigned int coin_transfer_accounting_record(MYSQL *connection, const critical_command &root,
					     const coin_transfer_result &result,
					     unsigned int result_code,
					     const coin_transfer_accounting_context &context)
{
	try
	{
		require(connection && (connection->server_status & SERVER_STATUS_IN_TRANS) &&
				mysql_thread_id(connection) == context.session_id,
			ENOTCONN);
		const auto value = decode(root);
		const auto &metadata = value.intent.admission.metadata;
		economic_sql_authority_snapshot current;
		const auto requests = wallet_mapping_requests(value);
		const auto lock_error = economic_sql_lock_authority(
			connection, metadata.lineage, metadata.epoch, requests, &current);
		require(!lock_error, lock_error);
		require(current.lineage_revision == context.authority.lineage_revision &&
				current.mappings.size() == context.authority.mappings.size(),
			ESTALE);
		for (size_t index = 0; index < current.mappings.size(); ++index)
			require(current.mappings[index].revision ==
					context.authority.mappings[index].revision,
				ESTALE);
		economic_plan_metadata plan_metadata;
		checked(economic_intent_plan_metadata(root, value.intent, &plan_metadata));
		const coin_transfer_endpoint *endpoints[] = { &value.payload.source,
							      &value.payload.destination };
		const economic_account_key accounts[] = { value.source, value.destination };
		for (size_t index = 0; index < 2; ++index)
			if (endpoints[index]->change.type == critical_command_type::item_transfer)
				require_pile_account_history(connection, plan_metadata,
							     *endpoints[index], accounts[index]);
		const bool business_rejection = result_code == ESTALE || result_code == ENOSPC ||
						result_code == ERANGE;
		require(!result_code || business_rejection, EINVAL);
		std::optional<economic_accounting_plan> plan;
		if (!result_code)
		{
			plan = make_plan(connection, root, value, result);
			const auto effect_error = economic_coin_effects_validate(
				plan->accounts, plan->postings, plan->children.size());
			checked(effect_error);
			checked(economic_child_links_validate(root.operation_id, plan->children));
			checked(economic_item_effects_validate(plan->items_before,
							       plan->items_after, plan->item_events,
							       plan->children.size()));
		}
		const auto operation =
			operation_fields(root, value.intent, result_code, plan ? &*plan : nullptr);
		insert(connection, "economic_accounting_operation", operation);
		count(connection, "economic_accounting_operation", predicate(operation), 1);
		if (plan)
			verify_item_reference_rows(connection, root, value, result, *plan, true);
		verify_plan_rows(connection, root, plan ? &*plan : nullptr, true);
		if (plan)
			verify_item_reference_rows(connection, root, value, result, *plan, false);
		verify_plan_rows(connection, root, plan ? &*plan : nullptr, false);
		require(connection->server_status & SERVER_STATUS_IN_TRANS, ENOTCONN);
		return 0;
	}
	catch (const failure &error)
	{
		return error_code(error);
	}
	catch (const std::bad_alloc &)
	{
		return ENOMEM;
	}
}

unsigned int coin_transfer_accounting_verify_retained(MYSQL *connection,
						      const critical_command &root,
						      unsigned int result_code,
						      const uint8_t *result_payload,
						      size_t result_size)
{
	try
	{
		require(connection && (!result_size || result_payload), EINVAL);
		const auto value = decode(root);
		const bool business_rejection = result_code == ESTALE || result_code == ENOSPC ||
						result_code == ERANGE;
		require(!result_code || business_rejection, EILSEQ);
		std::optional<economic_accounting_plan> plan;
		coin_transfer_result result = {};
		if (!result_code)
		{
			require(coin_transfer_command_decode_result(value.payload, result_payload,
								    result_size, &result),
				EILSEQ);
			plan = make_plan(connection, root, value, result);
		}
		else
			require(result_size == 0, EILSEQ);
		const auto operation =
			operation_fields(root, value.intent, result_code, plan ? &*plan : nullptr);
		count(connection, "economic_accounting_operation", predicate(operation), 1);
		const auto where = "operation_id=" + id(root.operation_id);
		if (plan)
			verify_item_reference_rows(connection, root, value, result, *plan, false);
		verify_plan_rows(connection, root, plan ? &*plan : nullptr, false);
		if (plan)
		{
			for (const auto *endpoint :
			     { &value.payload.source, &value.payload.destination })
				if (endpoint->change.type == critical_command_type::account_bank)
					count(connection, "currency_ledger",
					      "operation_id=" + id(endpoint->change.operation_id),
					      1);
			count(connection, "critical_outbox", where, 1);
		}
		else
		{
			count(connection, "currency_ledger", where, 0);
			count(connection, "critical_outbox", where, 0);
			for (const auto *endpoint :
			     { &value.payload.source, &value.payload.destination })
				count(connection,
				      endpoint->change.type == critical_command_type::account_bank ?
					      "currency_ledger" :
					      "item_ownership_ledger",
				      "operation_id=" + id(endpoint->change.operation_id), 0);
		}
		for (size_t index = 0; index < 2; ++index)
		{
			const auto &endpoint = index ? value.payload.destination :
						       value.payload.source;
			if (endpoint.change.type != critical_command_type::account_bank)
				continue;
			const auto &account = index ? value.destination : value.source;
			const auto native_id = index ? value.destination_change.pid :
						       value.source_change.pid;
			count(connection, "economic_account_mapping",
			      predicate({ { "mapping_id", std::to_string(account.authority_id) },
					  { "lineage", id(account.lineage) },
					  { "account_kind", "1" },
					  { "context_id", "0" },
					  { "backend_kind", "1" },
					  { "locator_kind", "1" },
					  { "native_id", std::to_string(native_id) } }),
			      1);
		}
		return 0;
	}
	catch (const failure &error)
	{
		return error_code(error);
	}
	catch (const std::bad_alloc &)
	{
		return ENOMEM;
	}
}
#endif
