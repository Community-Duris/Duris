#include "economy/zone_reset_item_source_proof.h"
#include "player/player_snapshot_codec.h"

#include <algorithm>
#include <array>
#include <cerrno>
#include <map>
#include <new>
#include <openssl/sha.h>
#include <set>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>

namespace
{
struct proof_failure
{
};
void require(bool value)
{
	if (!value)
		throw proof_failure{};
}
void checked(economic_accounting_error code)
{
	if (code == economic_accounting_error::capacity)
		throw std::bad_alloc{};
	require(code == economic_accounting_error::ok);
}
using cell = std::optional<std::string>;
using fields = std::vector<std::pair<std::string, cell>>;
using pair_counts = std::map<std::pair<std::string, std::string>, size_t>;

std::string bytes(std::span<const uint8_t> value)
{
	if (value.empty())
		return {};
	return { reinterpret_cast<const char *>(value.data()), value.size() };
}
std::string id(const critical_operation_id &value)
{
	return bytes(value.bytes);
}
std::string digest(std::span<const uint8_t> value)
{
	std::array<uint8_t, SHA256_DIGEST_LENGTH> result{};
	require(SHA256(value.data(), value.size(), result.data()));
	return bytes(result);
}
const economic_sql_source_table &table(const std::vector<economic_sql_source_table> &tables,
				       std::string_view name)
{
	const auto found = std::find_if(tables.begin(), tables.end(),
					[&](const auto &value) { return value.name == name; });
	require(found != tables.end());
	return *found;
}
size_t column(const economic_sql_source_table &value, std::string_view name)
{
	const auto found = std::find(value.columns.begin(), value.columns.end(), name);
	require(found != value.columns.end());
	return static_cast<size_t>(found - value.columns.begin());
}
bool matches(const economic_sql_source_table &value, const economic_sql_source_row &row,
	     const fields &expected)
{
	for (const auto &[name, wanted] : expected)
	{
		const auto at = column(value, name);
		if (at >= row.cells.size() || row.cells[at] != wanted)
			return false;
	}
	return true;
}
struct root_rows
{
	const economic_sql_source_table &value;
	std::map<std::string, std::vector<size_t>> by_operation;
	std::string index_column;
	std::map<std::pair<std::string, std::string>, std::vector<size_t>> by_index;
	root_rows(const economic_sql_source_table &source, const char *key,
		  const char *index_key = nullptr)
		: value(source)
		, index_column(index_key ? index_key : "")
	{
		const auto at = column(source, key);
		const auto indexed = index_key ? column(source, index_key) : 0;
		for (size_t index = 0; index < source.rows.size(); ++index)
			if (source.rows[index].cells[at])
			{
				by_operation[*source.rows[index].cells[at]].push_back(index);
				if (index_key && source.rows[index].cells[indexed])
					by_index[{ *source.rows[index].cells[at],
						   *source.rows[index].cells[indexed] }]
						.push_back(index);
			}
	}
	const std::vector<size_t> &rows(const std::string &operation) const
	{
		static const std::vector<size_t> empty;
		const auto found = by_operation.find(operation);
		return found == by_operation.end() ? empty : found->second;
	}
	void exact(const std::string &operation, const fields &expected) const
	{
		if (!index_column.empty())
		{
			const auto selector = std::find_if(expected.begin(), expected.end(),
							   [&](const auto &field)
							   { return field.first == index_column; });
			require(selector != expected.end() && selector->second);
			const auto selected = by_index.find({ operation, *selector->second });
			require(selected != by_index.end() && selected->second.size() == 1);
			require(matches(value, value.rows[selected->second[0]], expected));
			return;
		}
		size_t count = 0;
		for (const auto at : rows(operation))
			if (matches(value, value.rows[at], expected))
				++count;
		require(count == 1);
	}
};
pair_counts global_pairs(const root_rows &source, const char *left, const char *right)
{
	pair_counts counts;
	const auto a = column(source.value, left), b = column(source.value, right);
	for (const auto &row : source.value.rows)
		if (row.cells[a] && row.cells[b])
			++counts[{ *row.cells[a], *row.cells[b] }];
	return counts;
}
void unique_pair(const pair_counts &counts, const std::string &left, const std::string &right)
{
	const auto found = counts.find({ left, right });
	require(found != counts.end() && found->second == 1);
}
void coins(fields &value, const char *prefix, const economic_coin_vector &amount)
{
	static const char *names[] = { "copper", "silver", "gold", "platinum" };
	for (size_t index = 0; index < amount.size(); ++index)
		value.emplace_back(std::string(prefix) + names[index],
				   std::to_string(amount[index]));
}
std::string source(const economic_plan_metadata &metadata)
{
	require(metadata.source_event.has_value());
	std::array<uint8_t, ECONOMIC_SOURCE_EVENT_BYTES> encoded{};
	checked(economic_source_event_encode(*metadata.source_event, &encoded));
	return bytes(encoded);
}
fields operation(const zone_reset_item_source_family &family)
{
	const auto &plan = family.plan;
	const auto &meta = plan.metadata;
	std::vector<uint8_t> canonical;
	checked(economic_plan_encode(plan, &canonical));
	return { { "operation_id", id(family.operation) },
		 { "lineage", id(meta.lineage) },
		 { "epoch", id(meta.epoch) },
		 { "original_operation_id", std::nullopt },
		 { "accounting_version", std::to_string(meta.version) },
		 { "writer_id", std::to_string(meta.writer_id) },
		 { "policy_version", std::to_string(meta.policy_version) },
		 { "compiler_version", std::to_string(meta.compiler_version) },
		 { "actor_kind", std::to_string(static_cast<uint8_t>(meta.actor_kind)) },
		 { "actor_id", std::to_string(meta.actor_id) },
		 { "reason", std::to_string(static_cast<uint16_t>(meta.reason)) },
		 { "source_event", source(meta) },
		 { "intent_digest", bytes(meta.intent_digest) },
		 { "domain_digest", bytes(meta.domain_digest) },
		 { "plan_digest", digest(canonical) },
		 { "canonical_intent", bytes(family.origin.original.accounting_intent) },
		 { "canonical_plan", bytes(canonical) },
		 { "outcome", "1" },
		 { "result_code", "0" },
		 { "realized_price_copper", std::nullopt },
		 { "account_count", std::to_string(plan.accounts.size()) },
		 { "posting_count", std::to_string(plan.postings.size()) },
		 { "child_count", "0" },
		 { "item_event_count", std::to_string(plan.item_events.size()) },
		 { "before_witness_count", std::to_string(plan.items_before.size()) },
		 { "after_witness_count", std::to_string(plan.items_after.size()) } };
}
fields ledger(const zone_reset_item_source_family &family, const economic_item_event &event)
{
	return { { "operation_id", id(family.operation) },
		 { "event_index", std::to_string(event.event_index) },
		 { "item_uid", std::to_string(event.uid) },
		 { "root_item_uid", std::to_string(event.after.root_uid) },
		 { "parent_item_uid", event.after.parent_uid ?
					      cell(std::to_string(event.after.parent_uid)) :
					      std::nullopt },
		 { "from_owner_type", "0" },
		 { "from_owner_id", "0" },
		 { "from_owner_context_id", "0" },
		 { "to_owner_type", "3" },
		 { "to_owner_id", std::to_string(event.after.owner.id) },
		 { "to_owner_context_id", "0" },
		 { "item_revision", "1" },
		 { "from_owner_revision", "0" },
		 { "to_owner_revision", std::to_string(family.result.to_owner_revision) },
		 { "reason_type", "2" },
		 { "reason_id", "0" },
		 { "source_site",
		   std::to_string(static_cast<uint16_t>(family.origin.original.source_site)) },
		 { "from_equipment_slot", "0" },
		 { "to_equipment_slot", "0" } };
}
void authenticate(zone_reset_item_source_family &family, const root_rows &inbox,
		  const root_rows &outbox, const root_rows &operations, const root_rows &effects,
		  const root_rows &postings, const root_rows &claims, const root_rows &children,
		  const root_rows &ledgers, const root_rows &references, const root_rows &payloads,
		  const root_rows &legacy_operations, const pair_counts &ledger_uids,
		  const pair_counts &payload_uids, const pair_counts &reference_events,
		  const std::map<std::pair<std::string, std::string>, size_t> &source_counts)
{
	const auto root = id(family.operation);
	const auto &command = family.origin.original;
	require(critical_operation_id_equal(command.operation_id, family.operation));
	checked(zone_reset_item_command_decode(command, &family.image));
	checked(zone_reset_item_accounting_compile(command, &family.plan));
	require(item_transfer_command_decode_result(family.origin.result.data(),
						    family.origin.result.size(), &family.result));
	std::vector<uint8_t> canonical, keys;
	const auto command_code = critical_command_encode(command, &canonical);
	if (command_code == critical_command_codec_result::overflow)
		throw std::bad_alloc{};
	require(command_code == critical_command_codec_result::ok);
	for (const auto &key : command.keys)
	{
		keys.push_back(static_cast<uint8_t>(key.type));
		for (size_t at = 0; at < 8; ++at)
			keys.push_back(static_cast<uint8_t>(key.id >> (at * 8)));
	}
	require(inbox.rows(root).size() == 1);
	inbox.exact(root, { { "operation_id", root },
			    { "command_hash", digest(canonical) },
			    { "keys_hash", digest(keys) },
			    { "command_type", "22" },
			    { "schema_version", "2" },
			    { "payload_version", "1" },
			    { "status", "1" },
			    { "result_code", "0" },
			    { "failure_stage", "0" },
			    { "durable_revision", std::to_string(family.result.to_owner_revision) },
			    { "result_payload", bytes(family.origin.result) },
			    { "committed_at IS NOT NULL", "1" } });
	require(outbox.rows(root).size() == 1);
	outbox.exact(root, { { "operation_id", root },
			     { "event_index", "0" },
			     { "destination", "4" },
			     { "event_type", "1" },
			     { "payload_version", "1" },
			     { "payload", bytes(family.origin.result) } });
	require(operations.rows(root).size() == 1);
	operations.exact(root, operation(family));
	require(legacy_operations.rows(root).size() == 1);
	legacy_operations.exact(
		root, { { "operation_id", root }, { "outcome", "1" }, { "result_code", "0" } });
	require(effects.rows(root).size() == family.plan.accounts.size());
	for (size_t index = 0; index < family.plan.accounts.size(); ++index)
	{
		const auto &effect = family.plan.accounts[index];
		std::array<uint8_t, ECONOMIC_ACCOUNT_KEY_BYTES> key{};
		checked(economic_account_key_encode(effect.key, &key));
		fields expected{ { "operation_id", root },
				 { "account_index", std::to_string(index) },
				 { "account_key", bytes(key) } };
		coins(expected, "before_", effect.before);
		coins(expected, "after_", effect.after);
		expected.emplace_back("before_revision", std::to_string(effect.before_revision));
		expected.emplace_back("after_revision", std::to_string(effect.after_revision));
		effects.exact(root, expected);
	}
	require(postings.rows(root).size() == family.plan.postings.size());
	for (size_t index = 0; index < family.plan.postings.size(); ++index)
	{
		const auto &posting = family.plan.postings[index];
		fields expected{ { "operation_id", root },
				 { "line_index", std::to_string(index) },
				 { "event_index", std::to_string(posting.event_index) },
				 { "account_index", std::to_string(posting.account_index) },
				 { "child_index", std::to_string(posting.child_index) },
				 { "copper_value", std::to_string(posting.copper) } };
		coins(expected, "delta_", posting.delta);
		postings.exact(root, expected);
	}
	require(children.rows(root).empty());
	require(ledgers.rows(root).size() == family.plan.item_events.size());
	require(references.rows(root).size() == family.plan.item_events.size());
	require(payloads.rows(root).size() == family.image.items.size());
	for (const auto &event : family.plan.item_events)
	{
		unique_pair(ledger_uids, std::to_string(event.uid), "1");
		unique_pair(reference_events, root, std::to_string(event.event_index));
		ledgers.exact(root, ledger(family, event));
		references.exact(root,
				 { { "operation_id", root },
				   { "line_index", std::to_string(event.event_index) },
				   { "event_index", std::to_string(event.event_index) },
				   { "child_index", "0" },
				   { "item_uid", std::to_string(event.uid) },
				   { "before_revision", "0" },
				   { "after_revision", "1" },
				   { "legacy_operation_id", root },
				   { "legacy_event_index", std::to_string(event.event_index) } });
	}
	for (auto item : family.image.items)
	{
		unique_pair(payload_uids, std::to_string(item.object_uid), "1");
		item.parent_index = PLAYER_SNAPSHOT_NO_PARENT;
		std::vector<uint8_t> encoded;
		const auto item_code = player_item_snapshot_list_encode({ item }, &encoded);
		if (item_code == player_snapshot_codec_result::allocation_failure)
			throw std::bad_alloc{};
		require(item_code == player_snapshot_codec_result::ok);
		payloads.exact(root,
			       { { "item_uid", std::to_string(item.object_uid) },
				 { "item_revision", "1" },
				 { "payload_version", "1" },
				 { "operation_id", root },
				 { "season_epoch", std::to_string(family.image.season_epoch) },
				 { "payload", bytes(encoded) } });
	}
	const auto claim_source = source(family.plan.metadata);
	const auto lineage = id(family.plan.metadata.lineage);
	require(claims.rows(root).size() == 1);
	claims.exact(root, { { "lineage", lineage },
			     { "source_event", claim_source },
			     { "operation_id", root },
			     { "outcome", "1" } });
	const auto unique = source_counts.find({ lineage, claim_source });
	require(unique != source_counts.end() && unique->second == 1);
}
}

unsigned int
zone_reset_item_source_inspect(const economic_sql_physical_source_snapshot &base,
			       const sql_room_item_source_snapshot &room,
			       const sql_room_creation_source_snapshot &creation,
			       const economic_sql_source_limits &limits,
			       std::vector<zone_reset_item_source_family> *output) noexcept
{
	if (!output)
		return EINVAL;
	const auto code = sql_room_creation_source_validate_sources(base, room, creation, limits);
	if (code)
		return code;
	try
	{
		root_rows origins(table(creation.tables, "zone_reset_item_birth_origin"),
				  "birth_operation"),
			operations(table(creation.tables, "economic_accounting_operation"),
				   "operation_id"),
			effects(table(creation.tables, "economic_accounting_account_effect"),
				"operation_id", "account_index"),
			postings(table(creation.tables, "economic_accounting_coin_posting"),
				 "operation_id", "line_index"),
			claims(table(creation.tables, "economic_accounting_source_claim"),
			       "operation_id"),
			children(table(creation.tables, "economic_accounting_child"),
				 "operation_id"),
			inbox(table(base.source2.tables, "critical_operation_inbox"),
			      "operation_id"),
			outbox(table(base.source2.tables, "critical_outbox"), "operation_id"),
			ledgers(table(room.tables, "item_ownership_ledger"), "operation_id",
				"event_index"),
			references(table(room.tables, "economic_accounting_item_reference"),
				   "operation_id", "line_index"),
			payloads(table(room.tables, "sql_room_item_payload"), "operation_id",
				 "item_uid");
		root_rows legacy_operations(table(room.tables, "economic_accounting_operation"),
					    "operation_id");
		const auto ledger_uids = global_pairs(ledgers, "item_uid", "item_revision");
		const auto payload_uids = global_pairs(payloads, "item_uid", "item_revision");
		const auto reference_events =
			global_pairs(references, "legacy_operation_id", "legacy_event_index");
		std::set<std::string> roots;
		for (const auto &[root, rows] : origins.by_operation)
			if (root.size() == CRITICAL_COMMAND_ID_BYTES)
				roots.insert(root);
		for (const auto &[root, rows] : inbox.by_operation)
			for (const auto at : rows)
				if (root.size() == CRITICAL_COMMAND_ID_BYTES &&
				    matches(inbox.value, inbox.value.rows[at],
					    { { "command_type", "22" } }))
					roots.insert(root);
		for (const auto &[root, rows] : operations.by_operation)
			for (const auto at : rows)
				if (root.size() == CRITICAL_COMMAND_ID_BYTES &&
				    matches(operations.value, operations.value.rows[at],
					    { { "writer_id", "16" } }))
					roots.insert(root);
		// Creation ledgers remain visible even if inbox/operation/origin is lost.
		for (const auto &[root, rows] : ledgers.by_operation)
			for (const auto at : rows)
				if (root.size() == CRITICAL_COMMAND_ID_BYTES &&
				    matches(ledgers.value, ledgers.value.rows[at],
					    { { "from_owner_type", "0" },
					      { "to_owner_type", "3" },
					      { "reason_type", "2" },
					      { "source_site",
						std::to_string(static_cast<uint16_t>(
							critical_source_site::zone_event)) } }))
					roots.insert(root);
		std::map<std::string, size_t> origin_uid_counts;
		const auto origin_uid_column = column(origins.value, "root_item_uid");
		for (const auto &row : origins.value.rows)
			if (row.cells[origin_uid_column])
				++origin_uid_counts[*row.cells[origin_uid_column]];
		std::map<std::pair<std::string, std::string>, size_t> source_counts;
		const auto lineage_at = column(claims.value, "lineage"),
			   source_at = column(claims.value, "source_event");
		for (const auto &row : claims.value.rows)
			if (row.cells[lineage_at] && row.cells[source_at])
				++source_counts[{ *row.cells[lineage_at], *row.cells[source_at] }];
		std::vector<zone_reset_item_source_family> candidate;
		auto retain_malformed_root = [&](const root_rows &source, const fields &tag)
		{
			const auto key = column(source.value, "operation_id");
			for (const auto &row : source.value.rows)
				if (matches(source.value, row, tag) &&
				    (!row.cells[key] ||
				     row.cells[key]->size() != CRITICAL_COMMAND_ID_BYTES))
				{
					zone_reset_item_source_family malformed;
					malformed.flags = ZONE_RESET_SOURCE_BAD_RETAINED_ROOT |
							  ZONE_RESET_SOURCE_MISSING_ORIGIN;
					candidate.push_back(std::move(malformed));
				}
		};
		retain_malformed_root(inbox, { { "command_type", "22" } });
		retain_malformed_root(operations, { { "writer_id", "16" } });
		retain_malformed_root(
			ledgers, { { "from_owner_type", "0" },
				   { "to_owner_type", "3" },
				   { "reason_type", "2" },
				   { "source_site", std::to_string(static_cast<uint16_t>(
							    critical_source_site::zone_event)) } });
		for (size_t at = 0; at < origins.value.rows.size(); ++at)
		{
			const auto &root = origins.value.rows[at]
						   .cells[column(origins.value, "birth_operation")];
			if (!root || root->size() != CRITICAL_COMMAND_ID_BYTES)
			{
				zone_reset_item_source_family malformed;
				malformed.origin_row = at;
				malformed.flags = ZONE_RESET_SOURCE_MALFORMED_ORIGIN;
				candidate.push_back(std::move(malformed));
			}
		}
		for (const auto &root : roots)
		{
			zone_reset_item_source_family family;
			std::copy(root.begin(), root.end(), family.operation.bytes.begin());
			const auto &rows = origins.rows(root);
			if (rows.empty())
				family.flags = ZONE_RESET_SOURCE_MISSING_ORIGIN;
			else if (rows.size() != 1)
				family.flags = ZONE_RESET_SOURCE_AMBIGUOUS_ORIGIN;
			else
			{
				family.origin_row = rows[0];
				const auto &row = origins.value.rows[rows[0]];
				const auto &body =
					row.cells[column(origins.value, "canonical_origin")];
				const auto decoded =
					body ? zone_reset_item_origin_decode(
						       { reinterpret_cast<const uint8_t *>(
								 body->data()),
							 body->size() },
						       &family.origin) :
					       economic_accounting_error::corrupt_evidence;
				if (decoded == economic_accounting_error::capacity)
					throw std::bad_alloc{};
				if (decoded != economic_accounting_error::ok)
					family.flags = ZONE_RESET_SOURCE_MALFORMED_ORIGIN;
				else
				{
					try
					{
						const auto &uid = row.cells[origin_uid_column];
						require(uid && origin_uid_counts.at(*uid) == 1);
						authenticate(family, inbox, outbox, operations,
							     effects, postings, claims, children,
							     ledgers, references, payloads,
							     legacy_operations, ledger_uids,
							     payload_uids, reference_events,
							     source_counts);
						require(matches(
							origins.value, row,
							{ { "root_item_uid",
							    std::to_string(
								    family.result.root_item_uid) },
							  { "room_revision",
							    std::to_string(
								    family.result
									    .to_owner_revision) } }));
						const auto &terminal = row.cells[column(
							origins.value,
							"terminal_publication_context")];
						if (terminal)
						{
							const std::span<const uint8_t> bytes{
								reinterpret_cast<const uint8_t *>(
									terminal->data()),
								terminal->size()
							};
							critical_command original{};
							const auto original_error =
								zone_reset_item_recovery_original_command_decode(
									bytes, &original);
							if (original_error ==
							    economic_accounting_error::capacity)
								throw std::bad_alloc{};
							require(original_error ==
									economic_accounting_error::ok &&
								critical_command_equal(
									original,
									family.origin.original));
							zone_reset_item_recovery_context context;
							const auto context_error =
								zone_reset_item_recovery_decode(
									original, bytes, &context);
							if (context_error ==
							    economic_accounting_error::capacity)
								throw std::bad_alloc{};
							require(context_error ==
									economic_accounting_error::ok &&
								context.receipt_present &&
								context.receipt.result_size ==
									family.origin.result.size() &&
								std::equal(
									family.origin.result.begin(),
									family.origin.result.end(),
									context.receipt
										.result_payload
										.begin()));
							family.terminal = std::move(context);
						}
					}
					catch (const proof_failure &)
					{
						family.flags = ZONE_RESET_SOURCE_BAD_RETAINED_ROOT;
					}
				}
			}
			candidate.push_back(std::move(family));
		}
		static_assert(std::is_nothrow_move_assignable_v<decltype(candidate)>);
		*output = std::move(candidate);
		return 0;
	}
	catch (const std::bad_alloc &)
	{
		return ENOMEM;
	}
	catch (...)
	{
		return EILSEQ;
	}
}
