#include "persistence/economic_sql_item_transfer_transaction.h"

#include "item/economic_accounting_item_reference.h"
#include "item/held_retirement_transport.h"
#include "core/defines.h"
#include "persistence/sql_room_item_payload.h"
#include "player/player_snapshot_codec.h"
#include "player/player_save_pipeline.h"
#include "persistence/critical_command_completion.h"
#include "persistence/critical_command_repository.h"
#include "world/native_quest_recovery_context.h"
#include "persistence/quest_mobile_native_sql.h"
#include "persistence/economic_sql_native_mobile_birth_transaction.h"
#include "economy/native_mobile_birth_accounting.h"
#include "persistence/shop_item_runtime_payload.h"
#include "economy/shop_trade_recovery_manifest.h"
#include "item/craft_recipe_continuation.h"

#include <algorithm>
#include <cerrno>
#include <charconv>
#include <cstring>
#include <memory>
#include <map>
#include <new>
#include <optional>
#include <string>
#include <vector>
#include <tuple>
#include <type_traits>

#ifndef __NO_MYSQL__
namespace
{
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

bool execute(MYSQL *connection, const std::string &sql)
{
	if (mysql_real_query(connection, sql.data(), sql.size()) == 0)
		return true;
	errno = mysql_errno(connection) ? static_cast<int>(mysql_errno(connection)) : EIO;
	return false;
}

unsigned int failure_code()
{
	return errno ? static_cast<unsigned int>(errno) : EILSEQ;
}

bool refine_count(MYSQL *connection, const std::string &table, const std::string &predicate,
		  size_t expected)
{
	if (!execute(connection, "SELECT COUNT(*) FROM " + table + " WHERE " + predicate))
		return false;
	std::unique_ptr<MYSQL_RES, decltype(&mysql_free_result)> rows(
		mysql_store_result(connection), mysql_free_result);
	if (!rows || mysql_num_rows(rows.get()) != 1)
	{
		errno = EILSEQ;
		return false;
	}
	MYSQL_ROW row = mysql_fetch_row(rows.get());
	uint64_t count = 0;
	if (!row || !row[0])
	{
		errno = EILSEQ;
		return false;
	}
	const std::string text(row[0]);
	const auto parsed = std::from_chars(text.data(), text.data() + text.size(), count);
	if (parsed.ec != std::errc{} || parsed.ptr != text.data() + text.size() ||
	    count != expected)
	{
		errno = EILSEQ;
		return false;
	}
	return true;
}

bool refine_financial_rows(MYSQL *connection, const critical_operation_id &operation,
			   const economic_accounting_plan &plan, bool append)
{
	constexpr const char *coins[] = { "copper", "silver", "gold", "platinum" };
	auto row = [&](const char *table,
		       const std::vector<std::pair<std::string, std::string>> &fields)
	{
		std::string columns, values, predicate;
		for (const auto &field : fields)
		{
			columns += (columns.empty() ? "" : ",") + field.first;
			values += (values.empty() ? "" : ",") + field.second;
			predicate += (predicate.empty() ? "" : " AND ") + field.first + "=" +
				     field.second;
		}
		return (!append ||
			execute(connection, "INSERT INTO " + std::string(table) + "(" + columns +
						    ") VALUES(" + values + ")")) &&
		       refine_count(connection, table, predicate, 1);
	};
	for (size_t i = 0; i < plan.accounts.size(); ++i)
	{
		const auto &a = plan.accounts[i];
		std::array<uint8_t, ECONOMIC_ACCOUNT_KEY_BYTES> key{};
		if (economic_account_key_encode(a.key, &key) != economic_accounting_error::ok)
			return false;
		std::vector<std::pair<std::string, std::string>> fields{
			{ "operation_id", id(operation) },
			{ "account_index", std::to_string(i) },
			{ "account_key", hex(key) },
			{ "before_revision", std::to_string(a.before_revision) },
			{ "after_revision", std::to_string(a.after_revision) }
		};
		for (size_t j = 0; j < 4; ++j)
		{
			fields.emplace_back("before_" + std::string(coins[j]),
					    std::to_string(a.before[j]));
			fields.emplace_back("after_" + std::string(coins[j]),
					    std::to_string(a.after[j]));
		}
		if (!row("economic_accounting_account_effect", fields))
			return false;
	}
	for (size_t i = 0; i < plan.postings.size(); ++i)
	{
		const auto &a = plan.postings[i];
		std::vector<std::pair<std::string, std::string>> fields{
			{ "operation_id", id(operation) },
			{ "line_index", std::to_string(i) },
			{ "event_index", std::to_string(a.event_index) },
			{ "account_index", std::to_string(a.account_index) },
			{ "child_index", std::to_string(a.child_index) },
			{ "copper_value", std::to_string(a.copper) }
		};
		for (size_t j = 0; j < 4; ++j)
			fields.emplace_back("delta_" + std::string(coins[j]),
					    std::to_string(a.delta[j]));
		if (!row("economic_accounting_coin_posting", fields))
			return false;
	}
	return refine_count(connection, "economic_accounting_account_effect",
			    "operation_id=" + id(operation), plan.accounts.size()) &&
	       refine_count(connection, "economic_accounting_coin_posting",
			    "operation_id=" + id(operation), plan.postings.size());
}

std::string optional_id(const critical_operation_id &value)
{
	return critical_operation_id_is_zero(value) ? "NULL" : id(value);
}

bool insert_operation(MYSQL *connection, const critical_command &command,
		      const economic_frozen_intent &intent, const economic_accounting_plan *plan,
		      std::span<const uint8_t> encoded_plan, unsigned int result_code)
{
	const auto &metadata = intent.admission.metadata;
	std::array<uint8_t, ECONOMIC_SOURCE_EVENT_BYTES> source_event = {};
	std::string source_event_sql = "NULL";
	if (metadata.source_event)
	{
		if (economic_source_event_encode(*metadata.source_event, &source_event) !=
		    economic_accounting_error::ok)
		{
			errno = EILSEQ;
			return false;
		}
		source_event_sql = hex(source_event);
	}
	economic_digest plan_digest = {};
	std::string plan_digest_sql = "NULL";
	std::string canonical_plan_sql = "NULL";
	const bool applied = result_code == 0 && plan != nullptr;
	if (applied)
	{
		if (economic_plan_digest(*plan, &plan_digest) != economic_accounting_error::ok)
		{
			errno = EILSEQ;
			return false;
		}
		plan_digest_sql = hex(plan_digest);
		canonical_plan_sql = hex(encoded_plan);
	}
	else if (!result_code || plan || !encoded_plan.empty())
	{
		errno = EINVAL;
		return false;
	}
	const size_t account_count = applied ? plan->accounts.size() : 0;
	const size_t posting_count = applied ? plan->postings.size() : 0;
	const size_t child_count = applied ? plan->children.size() : 0;
	const size_t item_event_count = applied ? plan->item_events.size() : 0;
	const size_t before_count = applied ? plan->items_before.size() : 0;
	const size_t after_count = applied ? plan->items_after.size() : 0;
	if (account_count > ECONOMIC_ACCOUNTING_MAX_ACCOUNTS ||
	    posting_count > ECONOMIC_ACCOUNTING_MAX_POSTINGS ||
	    child_count > ECONOMIC_ACCOUNTING_MAX_CHILDREN ||
	    item_event_count > ECONOMIC_ACCOUNTING_MAX_ITEM_EVENTS ||
	    before_count > ECONOMIC_ACCOUNTING_MAX_ITEM_WITNESSES ||
	    after_count > ECONOMIC_ACCOUNTING_MAX_ITEM_WITNESSES)
	{
		errno = E2BIG;
		return false;
	}
	economic_digest intent_digest = {};
	if (economic_intent_digest(intent, &intent_digest) != economic_accounting_error::ok)
	{
		errno = EILSEQ;
		return false;
	}
	const std::string sql =
		"INSERT INTO economic_accounting_operation(operation_id,lineage,epoch,"
		"original_operation_id,accounting_version,writer_id,policy_version,compiler_version,"
		"actor_kind,actor_id,reason,source_event,intent_digest,domain_digest,plan_digest,"
		"canonical_intent,canonical_plan,outcome,result_code,account_count,posting_count,"
		"child_count,item_event_count,before_witness_count,after_witness_count) VALUES(" +
		id(metadata.operation_id) + "," + id(metadata.lineage) + "," + id(metadata.epoch) +
		"," + optional_id(metadata.original_operation_id) + "," +
		std::to_string(metadata.version) + "," + std::to_string(metadata.writer_id) + "," +
		std::to_string(metadata.policy_version) + "," +
		std::to_string(metadata.compiler_version) + "," +
		std::to_string(static_cast<uint8_t>(metadata.actor_kind)) + "," +
		std::to_string(metadata.actor_id) + "," +
		std::to_string(static_cast<uint16_t>(metadata.reason)) + "," + source_event_sql +
		"," + hex(intent_digest) + "," + hex(intent.domain_digest) + "," + plan_digest_sql +
		"," + hex(command.accounting_intent) + "," + canonical_plan_sql + "," +
		(applied ? "1" : "2") + "," + std::to_string(result_code) + "," +
		std::to_string(account_count) + "," + std::to_string(posting_count) + "," +
		std::to_string(child_count) + "," + std::to_string(item_event_count) + "," +
		std::to_string(before_count) + "," + std::to_string(after_count) + ")";
	return execute(connection, sql);
}

bool insert_source_claim(MYSQL *connection, const economic_operation_metadata &metadata)
{
	if (!metadata.source_event)
		return true;
	std::array<uint8_t, ECONOMIC_SOURCE_EVENT_BYTES> encoded = {};
	if (economic_source_event_encode(*metadata.source_event, &encoded) !=
	    economic_accounting_error::ok)
	{
		errno = EILSEQ;
		return false;
	}
	if (execute(connection, "INSERT INTO economic_accounting_source_claim(lineage,source_event,"
				"operation_id,outcome) VALUES(" +
					id(metadata.lineage) + "," + hex(encoded) + "," +
					id(metadata.operation_id) + ",1)"))
		return true;
	// A committed source already issued its item. A new command cannot retry it.
	if (errno == 1062)
		errno = EEXIST;
	return false;
}

bool parse_unsigned(const std::string &text, uint64_t *value)
{
	if (!value || text.empty())
		return false;
	uint64_t candidate = 0;
	const auto parsed = std::from_chars(text.data(), text.data() + text.size(), candidate);
	if (parsed.ec != std::errc{} || parsed.ptr != text.data() + text.size())
		return false;
	*value = candidate;
	return true;
}

bool read_operation(MYSQL *connection, const critical_operation_id &operation_id,
		    std::vector<std::optional<std::string>> *values)
{
	if (!values ||
	    !execute(connection,
		     "SELECT outcome,result_code,intent_digest,domain_digest,plan_digest,"
		     "canonical_intent,canonical_plan,account_count,posting_count,child_count,"
		     "item_event_count,before_witness_count,after_witness_count "
		     "FROM economic_accounting_operation WHERE operation_id=" +
			     id(operation_id)))
		return false;
	std::unique_ptr<MYSQL_RES, decltype(&mysql_free_result)> rows(
		mysql_store_result(connection), mysql_free_result);
	if (!rows || mysql_num_rows(rows.get()) != 1 || mysql_num_fields(rows.get()) != 13)
	{
		errno = mysql_errno(connection) ? static_cast<int>(mysql_errno(connection)) :
						  EILSEQ;
		return false;
	}
	MYSQL_ROW row = mysql_fetch_row(rows.get());
	unsigned long *lengths = mysql_fetch_lengths(rows.get());
	if (!row || !lengths)
	{
		errno = EILSEQ;
		return false;
	}
	try
	{
		values->clear();
		values->reserve(13);
		for (size_t index = 0; index < 13; ++index)
			values->push_back(row[index] ? std::optional<std::string>(std::string(
							       row[index], lengths[index])) :
						       std::nullopt);
	}
	catch (const std::bad_alloc &)
	{
		errno = ENOMEM;
		return false;
	}
	return true;
}

bool read_reference_count(MYSQL *connection, const critical_operation_id &operation_id,
			  uint64_t *count)
{
	if (!count ||
	    !execute(connection, "SELECT COUNT(*) FROM economic_accounting_item_reference WHERE "
				 "operation_id=" +
					 id(operation_id)))
		return false;
	std::unique_ptr<MYSQL_RES, decltype(&mysql_free_result)> rows(
		mysql_store_result(connection), mysql_free_result);
	if (!rows || mysql_num_rows(rows.get()) != 1)
	{
		errno = mysql_errno(connection) ? static_cast<int>(mysql_errno(connection)) :
						  EILSEQ;
		return false;
	}
	MYSQL_ROW row = mysql_fetch_row(rows.get());
	if (!row || !row[0] || !parse_unsigned(row[0], count))
	{
		errno = EILSEQ;
		return false;
	}
	return true;
}

bool read_source_claim_count(MYSQL *connection, const critical_operation_id &operation_id,
			     uint64_t *count)
{
	if (!count ||
	    !execute(connection, "SELECT COUNT(*) FROM economic_accounting_source_claim WHERE "
				 "operation_id=" +
					 id(operation_id)))
		return false;
	std::unique_ptr<MYSQL_RES, decltype(&mysql_free_result)> rows(
		mysql_store_result(connection), mysql_free_result);
	if (!rows || mysql_num_rows(rows.get()) != 1)
	{
		errno = mysql_errno(connection) ? static_cast<int>(mysql_errno(connection)) :
						  EILSEQ;
		return false;
	}
	MYSQL_ROW row = mysql_fetch_row(rows.get());
	if (!row || !row[0] || !parse_unsigned(row[0], count))
	{
		errno = EILSEQ;
		return false;
	}
	return true;
}

bool metadata_matches(const economic_plan_metadata &actual, const economic_plan_metadata &expected)
{
	const bool source_event_matches =
		actual.source_event.has_value() == expected.source_event.has_value() &&
		(!actual.source_event ||
		 (actual.source_event->kind == expected.source_event->kind &&
		  actual.source_event->source.bytes == expected.source_event->source.bytes &&
		  actual.source_event->generation.bytes ==
			  expected.source_event->generation.bytes &&
		  actual.source_event->sequence == expected.source_event->sequence &&
		  actual.source_event->slot == expected.source_event->slot));
	return actual.version == expected.version &&
	       actual.lineage.bytes == expected.lineage.bytes &&
	       actual.epoch.bytes == expected.epoch.bytes &&
	       actual.operation_id.bytes == expected.operation_id.bytes &&
	       actual.original_operation_id.bytes == expected.original_operation_id.bytes &&
	       actual.actor_kind == expected.actor_kind && actual.actor_id == expected.actor_id &&
	       actual.writer_id == expected.writer_id &&
	       actual.policy_version == expected.policy_version &&
	       actual.compiler_version == expected.compiler_version &&
	       actual.reason == expected.reason && source_event_matches &&
	       actual.intent_digest == expected.intent_digest &&
	       actual.domain_digest == expected.domain_digest;
}

bool digest_matches(const economic_digest &expected, const std::optional<std::string> &actual)
{
	return actual && actual->size() == expected.size() &&
	       std::memcmp(expected.data(), actual->data(), expected.size()) == 0;
}

bool native_quest_command_bound(const critical_command &command)
{
	if (command.schema_version != CRITICAL_COMMAND_ACCOUNTING_SCHEMA_VERSION ||
	    command.payload_version != ITEM_TRANSFER_NATIVE_MOBILE_RECOVERY_PAYLOAD_VERSION ||
	    !command.publication_required || !command.accepted_at_usec ||
	    !critical_command_envelope_valid(command))
		return false;
	economic_frozen_intent intent;
	item_transfer_payload payload{};
	if (economic_intent_decode(command.accounting_intent, &intent) !=
		    economic_accounting_error::ok ||
	    economic_intent_verify_binding(command, intent) != economic_accounting_error::ok ||
	    !item_transfer_command_decode_payload(command, &payload) ||
	    !item_transfer_native_mobile_recovery_shape_valid(payload))
		return false;
	auto original = command;
	original.schema_version = CRITICAL_COMMAND_SCHEMA_VERSION;
	original.accounting_intent.clear();
	original.accepted_at_usec = 0;
	original.publication_required = false;
	std::vector<uint8_t> expected;
	const auto &metadata = intent.admission.metadata;
	return item_native_mobile_accounting_intent(
		       original, metadata.lineage, metadata.epoch,
		       payload.native_mobile.final_giver_pid,
		       metadata.source_event ? &*metadata.source_event : nullptr,
		       &expected) == economic_accounting_error::ok &&
	       expected == command.accounting_intent;
}

bool native_quest_plan_matches(const critical_command &command,
			       const item_transfer_custody_delta &delta)
{
	item_transfer_payload payload{};
	std::vector<player_item_snapshot> literal;
	if (!item_transfer_command_decode_payload(command, &payload))
	{
		errno = EILSEQ;
		return false;
	}
	const auto code = player_item_snapshot_list_decode(payload.item_blob.data(),
							   payload.item_blob_size, &literal);
	if (code != player_snapshot_codec_result::ok)
	{
		errno = code == player_snapshot_codec_result::allocation_failure ? ENOMEM : EILSEQ;
		return false;
	}
	if (delta.before.size() != payload.item_count || delta.after.size() != payload.item_count ||
	    delta.events.size() != payload.item_count)
		return false;
	for (size_t i = 0; i < payload.item_count; ++i)
	{
		const auto &entry = payload.items[i];
		const auto snapshot = std::find_if(literal.begin(), literal.end(),
						   [&](const auto &row)
						   { return row.object_uid == entry.item_uid; });
		if (snapshot == literal.end())
			return false;
		uint64_t root = 0, parent = 0;
		if (!item_transfer_target_topology(payload, entry.item_uid, &root, &parent))
			return false;
		const economic_item_position before{
			payload.from_owner,	    entry.root_item_uid,
			entry.parent_item_uid,	    entry.expected_item_revision,
			item_custody_state::active, static_cast<uint16_t>(snapshot->equipment_slot)
		};
		const economic_item_position after{
			payload.to_owner,
			root,
			parent,
			entry.expected_item_revision + 1,
			payload.native_mobile.action == item_native_mobile_action::acceptance ?
				item_custody_state::active :
				item_custody_state::destroyed,
			0
		};
		const auto same =
			[](const economic_item_position &a, const economic_item_position &b)
		{
			return item_owner_identity_equal(a.owner, b.owner) &&
			       a.root_uid == b.root_uid && a.parent_uid == b.parent_uid &&
			       a.revision == b.revision && a.state == b.state &&
			       a.equipment_slot == b.equipment_slot;
		};
		const auto &event = delta.events[i];
		if (delta.before[i].uid != entry.item_uid || delta.after[i].uid != entry.item_uid ||
		    !same(delta.before[i].position, before) ||
		    !same(delta.after[i].position, after) || event.event_index != i ||
		    event.child_index || event.uid != entry.item_uid ||
		    !same(event.before, before) || !same(event.after, after))
			return false;
	}
	return true;
}

bool held_exact_count(MYSQL *connection, const std::string &sql, uint64_t expected)
{
	if (!execute(connection, sql))
		return false;
	std::unique_ptr<MYSQL_RES, decltype(&mysql_free_result)> rows(
		mysql_store_result(connection), mysql_free_result);
	if (!rows || mysql_num_fields(rows.get()) != 1 || mysql_num_rows(rows.get()) != 1)
	{
		errno = mysql_errno(connection) ? static_cast<int>(mysql_errno(connection)) :
						  EILSEQ;
		return false;
	}
	MYSQL_ROW row = mysql_fetch_row(rows.get());
	unsigned long *lengths = mysql_fetch_lengths(rows.get());
	uint64_t count = 0;
	if (!row || !lengths || !row[0] ||
	    !parse_unsigned(std::string(row[0], lengths[0]), &count) || count != expected)
	{
		errno = EILSEQ;
		return false;
	}
	return true;
}

bool held_plan(const critical_command &command, economic_accounting_plan *plan)
{
	item_transfer_payload payload{};
	economic_frozen_intent intent;
	if (!plan || !held_retirement_command_identity(command, &payload) ||
	    economic_intent_decode(command.accounting_intent, &intent) !=
		    economic_accounting_error::ok ||
	    economic_intent_plan_metadata(command, intent, &plan->metadata) !=
		    economic_accounting_error::ok)
		return false;
	const auto &entry = payload.items[0];
	const economic_item_position before{
		payload.from_owner,	    entry.item_uid, 0, entry.expected_item_revision,
		item_custody_state::active, HOLD + 1
	};
	const economic_item_position after{ payload.to_owner,
					    entry.item_uid,
					    0,
					    entry.expected_item_revision + 1,
					    item_custody_state::destroyed,
					    0 };
	plan->items_before = { { entry.item_uid, before } };
	plan->items_after = { { entry.item_uid, after } };
	plan->item_events = { { 0, 0, entry.item_uid, before, after } };
	return economic_plan_normalize(plan) == economic_accounting_error::ok &&
	       economic_plan_validate_structure(*plan) == economic_accounting_error::ok;
}

bool held_root_and_source(MYSQL *connection, const critical_command &command,
			  const economic_frozen_intent &intent, unsigned int result_code)
{
	const auto &m = intent.admission.metadata;
	std::array<uint8_t, ECONOMIC_SOURCE_EVENT_BYTES> source{};
	if (!m.source_event ||
	    economic_source_event_encode(*m.source_event, &source) != economic_accounting_error::ok)
	{
		errno = EILSEQ;
		return false;
	}
	const std::string scope = "operation_id=" + id(command.operation_id);
	const std::string predicate =
		scope + " AND lineage=" + id(m.lineage) + " AND epoch=" + id(m.epoch) +
		" AND original_operation_id <=> " + optional_id(m.original_operation_id) +
		" AND accounting_version=" + std::to_string(m.version) +
		" AND writer_id=" + std::to_string(m.writer_id) +
		" AND policy_version=" + std::to_string(m.policy_version) +
		" AND compiler_version=" + std::to_string(m.compiler_version) +
		" AND actor_kind=" + std::to_string(static_cast<uint8_t>(m.actor_kind)) +
		" AND actor_id=" + std::to_string(m.actor_id) +
		" AND reason=" + std::to_string(static_cast<uint16_t>(m.reason)) +
		" AND source_event=" + hex(source) +
		" AND outcome=" + std::to_string(result_code ? 2 : 1) +
		" AND result_code=" + std::to_string(result_code);
	if (!held_exact_count(
		    connection,
		    "SELECT COUNT(*) FROM economic_accounting_operation WHERE " + predicate, 1))
		return false;
	for (const char *table :
	     { "economic_accounting_account_effect", "economic_accounting_coin_posting" })
		if (!held_exact_count(
			    connection,
			    "SELECT COUNT(*) FROM " + std::string(table) + " WHERE " + scope, 0))
			return false;
	if (!result_code &&
	    !held_exact_count(connection,
			      "SELECT COUNT(*) FROM economic_accounting_source_claim WHERE " +
				      scope + " AND lineage=" + id(m.lineage) +
				      " AND source_event=" + hex(source) + " AND outcome=1",
			      1))
		return false;
	return held_exact_count(connection,
				"SELECT COUNT(*) FROM item_ownership_ledger WHERE " + scope,
				result_code ? 0 : 1);
}

bool held_retained_effect(MYSQL *connection, const critical_command &command,
			  const economic_accounting_plan &plan, std::span<const uint8_t> result)
{
	item_transfer_payload payload{};
	economic_accounting_plan expected;
	std::vector<uint8_t> encoded, expected_encoded;
	std::array<uint8_t, ITEM_TRANSFER_RESULT_BYTES> expected_result{};
	if (!held_retirement_command_identity(command, &payload) ||
	    !held_plan(command, &expected) ||
	    economic_plan_encode(plan, &encoded) != economic_accounting_error::ok ||
	    economic_plan_encode(expected, &expected_encoded) != economic_accounting_error::ok ||
	    encoded != expected_encoded)
	{
		errno = EILSEQ;
		return false;
	}
	const auto &entry = payload.items[0];
	item_transfer_result native{};
	native.root_item_uid = entry.item_uid;
	native.item_count = 1;
	native.from_owner_revision = payload.expected_from_revision + 1;
	native.to_owner_revision = payload.expected_to_revision + 1;
	native.max_item_revision = entry.expected_item_revision + 1;
	if (!item_transfer_command_encode_result(native, &expected_result) ||
	    result.size() != expected_result.size() ||
	    !std::equal(expected_result.begin(), expected_result.end(), result.begin()))
	{
		errno = EILSEQ;
		return false;
	}
	const std::string predicate =
		"operation_id=" + id(command.operation_id) +
		" AND event_index=0 AND item_uid=" + std::to_string(entry.item_uid) +
		" AND root_item_uid=" + std::to_string(entry.item_uid) +
		" AND parent_item_uid IS NULL AND from_owner_type=" +
		std::to_string(static_cast<uint8_t>(payload.from_owner.type)) +
		" AND from_owner_id=" + std::to_string(payload.from_owner.id) +
		" AND from_owner_context_id=0 AND to_owner_type=" +
		std::to_string(static_cast<uint8_t>(payload.to_owner.type)) +
		" AND to_owner_id=0 AND to_owner_context_id=0 AND item_revision=" +
		std::to_string(native.max_item_revision) +
		" AND from_owner_revision=" + std::to_string(native.from_owner_revision) +
		" AND to_owner_revision=" + std::to_string(native.to_owner_revision) +
		" AND reason_type=" + std::to_string(static_cast<uint16_t>(payload.reason)) +
		" AND reason_id=" + std::to_string(payload.reason_id) +
		" AND source_site=" + std::to_string(static_cast<uint16_t>(command.source_site)) +
		" AND from_equipment_slot=" + std::to_string(HOLD + 1) + " AND to_equipment_slot=0";
	return held_exact_count(connection,
				"SELECT COUNT(*) FROM item_ownership_ledger WHERE " + predicate, 1);
}

uint64_t native_publication_save_revision(MYSQL *, uint32_t);

} // namespace
#endif

unsigned int economic_sql_item_transfer_lock(MYSQL *connection, const critical_command &command,
					     economic_sql_item_transfer_context *context)
{
#ifdef __NO_MYSQL__
	(void)connection;
	(void)command;
	(void)context;
	return ENOTSUP;
#else
	try
	{
		if (!connection || !context ||
		    !(connection->server_status & SERVER_STATUS_IN_TRANS) ||
		    !item_transfer_accounting_command_supported(command))
			return EINVAL;
		using client_flag = std::remove_pointer_t<decltype(MYSQL_BIND{}.is_null)>;
		client_flag reconnect = false;
		if (mysql_get_option(connection, MYSQL_OPT_RECONNECT, &reconnect) || reconnect)
			return EPERM;
		economic_frozen_intent intent;
		if (economic_intent_decode(command.accounting_intent, &intent) !=
		    economic_accounting_error::ok)
			return EILSEQ;
		economic_sql_item_transfer_context candidate;
		if (held_retirement_transport_command(command))
		{
			candidate.held_retirement.emplace();
			player_held_retirement_checkpoint_stage original;
			if (!held_retirement_command_identity(command) ||
			    !player_save_held_retirement_checkpoint_owner::original_held_bodies(
				    command, &candidate.held_retirement->before,
				    &candidate.held_retirement->after, &original))
				return ENODATA;
			candidate.held_retirement->save_revision = original.save_revision;
			if (critical_command_encode(command, &candidate.original_held_command) !=
			    critical_command_codec_result::ok)
				return EILSEQ;
		}
		const auto &metadata = intent.admission.metadata;
		item_transfer_payload payload{};
		craft_recipe_continuation refine;
		if (!item_transfer_command_decode_payload(command, &payload))
			return EILSEQ;
		std::vector<economic_sql_mapping_request> requests;
		if (craft_refine_from_payload(payload, &refine) && refine.refine_ore_count != 1)
			requests.push_back({ { metadata.lineage, economic_account_kind::wallet,
					       refine.refine_cost.wallet_mapping_id, 0 },
					     1,
					     refine.player_pid });
		const auto lock_error = economic_sql_lock_authority(connection, metadata.lineage,
								    metadata.epoch, requests,
								    &candidate.authority);
		if (lock_error)
			return lock_error;
		candidate.session_id = mysql_thread_id(connection);
		if (refine.discipline == craft_recipe_discipline::refine &&
		    !item_transfer_repository_refine_wallet_lock(connection, command, false))
			return failure_code();
		if (candidate.held_retirement)
		{
			uint64_t observed = 0;
			const auto fence_error = economic_sql_held_retirement_lock_saved_revision(
				connection, command, *candidate.held_retirement, &observed);
			if (fence_error)
				return fence_error;
		}
		if (!(connection->server_status & SERVER_STATUS_IN_TRANS) ||
		    mysql_thread_id(connection) != candidate.session_id)
			return ENOTCONN;
		*context = std::move(candidate);
		return 0;
	}
	catch (const std::bad_alloc &)
	{
		return ENOMEM;
	}
#endif
}

unsigned int economic_sql_item_transfer_record(MYSQL *connection, const critical_command &command,
					       const item_transfer_custody_delta *custody_delta,
					       unsigned int result_code, bool mutation_applied,
					       const economic_sql_item_transfer_context &context)
{
#ifdef __NO_MYSQL__
	(void)connection;
	(void)command;
	(void)custody_delta;
	(void)result_code;
	(void)mutation_applied;
	(void)context;
	return ENOTSUP;
#else
	try
	{
		if (!connection || !(connection->server_status & SERVER_STATUS_IN_TRANS) ||
		    !context.session_id || mysql_thread_id(connection) != context.session_id ||
		    !item_transfer_accounting_command_supported(command) ||
		    (result_code == 0) != mutation_applied || (mutation_applied && !custody_delta))
			return EINVAL;
		economic_frozen_intent intent;
		if (economic_intent_decode(command.accounting_intent, &intent) !=
		    economic_accounting_error::ok)
			return EINVAL;
		if (held_retirement_transport_command(command))
		{
			std::vector<uint8_t> exact, captured;
			if (!context.held_retirement ||
			    critical_command_encode(command, &exact) !=
				    critical_command_codec_result::ok ||
			    exact != context.original_held_command ||
			    !held_retirement_recovery_encode(command, *context.held_retirement,
							     &captured) ||
			    (mutation_applied && !context.held_source_custody_locked))
				return EILSEQ;
		}
		if (intent.admission.metadata.lineage.bytes != context.authority.lineage.bytes ||
		    intent.admission.metadata.epoch.bytes != context.authority.epoch.bytes)
			return ESTALE;
		if (!mutation_applied)
		{
			if (!insert_operation(connection, command, intent, nullptr, {},
					      result_code))
				return failure_code();
			return 0;
		}
		economic_accounting_plan plan;
		if (economic_intent_plan_metadata(command, intent, &plan.metadata) !=
		    economic_accounting_error::ok)
			return EILSEQ;
		plan.items_before = custody_delta->before;
		plan.items_after = custody_delta->after;
		plan.item_events = custody_delta->events;
		item_transfer_payload payload{};
		craft_recipe_continuation refine;
		if (!item_transfer_command_decode_payload(command, &payload) ||
		    item_transfer_refine_wallet_accounting_effects(payload, &plan) !=
			    economic_accounting_error::ok)
			return EILSEQ;
		const bool refining = craft_refine_from_payload(payload, &refine);
		if (refining &&
		    !item_transfer_repository_refine_wallet_lock(connection, command, true))
			return failure_code();
		if (held_retirement_transport_command(command))
		{
			economic_accounting_plan expected;
			std::vector<uint8_t> observed_bytes, expected_bytes;
			if (!held_plan(command, &expected) ||
			    economic_plan_normalize(&plan) != economic_accounting_error::ok ||
			    economic_plan_encode(plan, &observed_bytes) !=
				    economic_accounting_error::ok ||
			    economic_plan_encode(expected, &expected_bytes) !=
				    economic_accounting_error::ok ||
			    observed_bytes != expected_bytes)
				return EILSEQ;
		}
		if (economic_plan_normalize(&plan) != economic_accounting_error::ok ||
		    economic_plan_validate_structure(plan) != economic_accounting_error::ok)
			return EILSEQ;
		std::vector<uint8_t> encoded_plan;
		if (economic_plan_encode(plan, &encoded_plan) != economic_accounting_error::ok)
			return EILSEQ;
		if (!insert_operation(connection, command, intent, &plan, encoded_plan, 0))
			return failure_code();
		if (refining &&
		    !refine_financial_rows(connection, command.operation_id, plan, true))
			return failure_code();
		if (!insert_source_claim(connection, intent.admission.metadata))
			return failure_code();
		for (const auto &event : plan.item_events)
		{
			economic_accounting_item_reference reference = {};
			reference.operation_id = command.operation_id;
			reference.line_index = static_cast<uint16_t>(event.event_index);
			reference.event_index = event.event_index;
			reference.child_index = event.child_index;
			reference.item_uid = event.uid;
			reference.before_revision = event.before.revision;
			reference.after_revision = event.after.revision;
			reference.legacy_operation_id = command.operation_id;
			reference.legacy_event_index = static_cast<uint16_t>(event.event_index);
			if (!economic_accounting_item_reference_insert(connection, reference))
				return failure_code();
		}
		return 0;
	}
	catch (const std::bad_alloc &)
	{
		return ENOMEM;
	}
#endif
}

unsigned int economic_sql_item_transfer_verify_retained(MYSQL *connection,
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
	try
	{
		if (!connection || !item_transfer_accounting_command_supported(command))
			return EINVAL;
		economic_frozen_intent intent;
		if (economic_intent_decode(command.accounting_intent, &intent) !=
			    economic_accounting_error::ok ||
		    economic_intent_verify_binding(command, intent) !=
			    economic_accounting_error::ok)
			return EILSEQ;
		std::vector<std::optional<std::string>> values;
		if (!read_operation(connection, command.operation_id, &values))
			return failure_code();
		uint64_t outcome = 0, stored_code = 0, account_count = 0, posting_count = 0;
		uint64_t child_count = 0, event_count = 0, before_count = 0, after_count = 0;
		if (!values[0] || !parse_unsigned(*values[0], &outcome) || !values[1] ||
		    !parse_unsigned(*values[1], &stored_code) || !values[2] ||
		    values[2]->size() != 32 || !values[3] || values[3]->size() != 32 ||
		    !values[5] ||
		    *values[5] != std::string(reinterpret_cast<const char *>(
						      command.accounting_intent.data()),
					      command.accounting_intent.size()) ||
		    !values[7] || !parse_unsigned(*values[7], &account_count) || !values[8] ||
		    !parse_unsigned(*values[8], &posting_count) || !values[9] ||
		    !parse_unsigned(*values[9], &child_count) || !values[10] ||
		    !parse_unsigned(*values[10], &event_count) || !values[11] ||
		    !parse_unsigned(*values[11], &before_count) || !values[12] ||
		    !parse_unsigned(*values[12], &after_count) || stored_code != result_code ||
		    outcome != (result_code ? 2u : 1u))
			return EILSEQ;
		economic_digest intent_digest = {};
		const auto intent_digest_status = economic_intent_digest(intent, &intent_digest);
		if (intent_digest_status != economic_accounting_error::ok ||
		    !digest_matches(intent_digest, values[2]) ||
		    !digest_matches(intent.domain_digest, values[3]))
			return EILSEQ;
		uint64_t reference_count = 0, source_claim_count = 0;
		if (!read_reference_count(connection, command.operation_id, &reference_count))
			return failure_code();
		if (!read_source_claim_count(connection, command.operation_id, &source_claim_count))
			return failure_code();
		if (held_retirement_transport_command(command))
		{
			if (!held_retirement_command_identity(command))
				return EILSEQ;
			if (!held_root_and_source(connection, command, intent, result_code))
				return failure_code();
		}
		if (result_code)
		{
			item_transfer_payload failed_payload = {};
			craft_recipe_continuation failed_refine;
			if (!item_transfer_command_decode_payload(command, &failed_payload) ||
			    (craft_refine_from_payload(failed_payload, &failed_refine) &&
			     !refine_financial_rows(connection, command.operation_id, {}, false)))
				return EILSEQ;
			return !values[4] && !values[6] && !account_count && !posting_count &&
					       !child_count && !event_count && !before_count &&
					       !after_count && !reference_count &&
					       !source_claim_count ?
				       0 :
				       EILSEQ;
		}
		if (!result_payload || result_size != ITEM_TRANSFER_RESULT_BYTES || !values[4] ||
		    values[4]->size() != 32 || !values[6] || event_count == 0 ||
		    account_count > 2 || posting_count > 2 || child_count ||
		    event_count != reference_count)
			return EILSEQ;
		if (source_claim_count !=
		    static_cast<uint64_t>(intent.admission.metadata.source_event.has_value()))
			return EILSEQ;
		item_transfer_result retained_result = {};
		item_transfer_payload payload = {};
		if (!item_transfer_command_decode_result(result_payload, result_size,
							 &retained_result) ||
		    !item_transfer_command_decode_payload(command, &payload) ||
		    retained_result.item_count != payload.item_count ||
		    (payload.reason != item_transfer_reason::craft &&
		     retained_result.item_count != event_count))
			return EILSEQ;
		economic_accounting_plan plan;
		if (economic_plan_decode(std::span<const uint8_t>(reinterpret_cast<const uint8_t *>(
									  values[6]->data()),
								  values[6]->size()),
					 &plan) != economic_accounting_error::ok ||
		    economic_plan_validate_structure(plan) != economic_accounting_error::ok ||
		    plan.accounts.size() != account_count ||
		    plan.postings.size() != posting_count ||
		    plan.item_events.size() != event_count ||
		    plan.items_before.size() != before_count ||
		    plan.items_after.size() != after_count ||
		    economic_item_effects_validate(plan.items_before, plan.items_after,
						   plan.item_events, plan.children.size()) !=
			    economic_accounting_error::ok)
			return EILSEQ;
		economic_plan_metadata expected_metadata;
		if (economic_intent_plan_metadata(command, intent, &expected_metadata) !=
			    economic_accounting_error::ok ||
		    !metadata_matches(plan.metadata, expected_metadata))
			return EILSEQ;
		economic_digest expected_plan_digest = {};
		if (economic_plan_digest(plan, &expected_plan_digest) !=
			    economic_accounting_error::ok ||
		    !digest_matches(expected_plan_digest, values[4]))
			return EILSEQ;
		std::vector<uint8_t> canonical_plan;
		if (economic_plan_encode(plan, &canonical_plan) != economic_accounting_error::ok ||
		    std::string(reinterpret_cast<const char *>(canonical_plan.data()),
				canonical_plan.size()) != *values[6])
			return EILSEQ;
		if (payload.reason == item_transfer_reason::craft)
		{
			const auto &inputs = plan.items_before;
			economic_accounting_plan expected;
			expected.metadata = expected_metadata;
			std::vector<uint8_t> encoded_expected;
			if (item_transfer_craft_accounting_effects(payload, inputs, &expected) !=
				    economic_accounting_error::ok ||
			    item_transfer_refine_wallet_accounting_effects(payload, &expected) !=
				    economic_accounting_error::ok ||
			    economic_plan_normalize(&expected) != economic_accounting_error::ok ||
			    economic_plan_encode(expected, &encoded_expected) !=
				    economic_accounting_error::ok ||
			    encoded_expected != canonical_plan)
				return EILSEQ;
		}
		craft_recipe_continuation refine;
		const bool refining = craft_refine_from_payload(payload, &refine);
		if ((!refining && (account_count || posting_count)) ||
		    (refining &&
		     !refine_financial_rows(connection, command.operation_id, plan, false)))
			return EILSEQ;
		if (refining && refine.refine_ore_count != 1 &&
		    !refine_count(
			    connection, "economic_account_mapping",
			    "mapping_id=" + std::to_string(refine.refine_cost.wallet_mapping_id) +
				    " AND lineage=" + id(expected_metadata.lineage) +
				    " AND account_kind=1 AND context_id=0 AND backend_kind=1 AND locator_kind=1 AND native_id=" +
				    std::to_string(refine.player_pid),
			    1))
			return EILSEQ;
		for (size_t index = 0; index < plan.item_events.size(); ++index)
		{
			const auto &event = plan.item_events[index];
			economic_accounting_item_reference reference = {};
			if (event.event_index != index || event.child_index != 0 ||
			    !economic_accounting_item_reference_find_by_legacy(
				    connection, command.operation_id, static_cast<uint16_t>(index),
				    &reference) ||
			    reference.operation_id.bytes != command.operation_id.bytes ||
			    reference.line_index != index || reference.event_index != index ||
			    reference.child_index != event.child_index ||
			    reference.item_uid != event.uid ||
			    reference.before_revision != event.before.revision ||
			    reference.after_revision != event.after.revision)
				return EILSEQ;
		}
		if (held_retirement_transport_command(command) &&
		    !held_retained_effect(connection, command, plan,
					  { result_payload, result_size }))
			return failure_code();
		// A successful ordinary drop also owns immutable full-literal room
		// payload and native ledger proof. Missing history remains retryable;
		// do not fabricate evidence from current custody or player projections.
		if (payload.reason == item_transfer_reason::player_drop &&
		    !sql_room_item_payload_verify_retained(connection, command, retained_result))
			return failure_code();
		return 0;
	}
	catch (const std::bad_alloc &)
	{
		return ENOMEM;
	}
#endif
}

unsigned int economic_sql_native_quest_lock(MYSQL *connection, const critical_command &command,
					    economic_sql_native_quest_context *context)
{
#ifdef __NO_MYSQL__
	(void)connection;
	(void)command;
	(void)context;
	return ENOTSUP;
#else
	try
	{
		if (!connection || !context ||
		    !(connection->server_status & SERVER_STATUS_IN_TRANS) ||
		    !native_quest_command_bound(command))
			return EINVAL;
		using client_flag = std::remove_pointer_t<decltype(MYSQL_BIND{}.is_null)>;
		client_flag reconnect = false;
		if (mysql_get_option(connection, MYSQL_OPT_RECONNECT, &reconnect) || reconnect)
			return EPERM;
		economic_frozen_intent intent;
		if (economic_intent_decode(command.accounting_intent, &intent) !=
		    economic_accounting_error::ok)
			return EILSEQ;
		economic_sql_native_quest_context candidate;
		player_native_quest_checkpoint_stage original;
		if (!player_save_native_quest_checkpoint_owner::original_held_bodies(
			    command, &candidate.original_player_before,
			    &candidate.original_player_after, &original))
			return ENODATA;
		candidate.acknowledged_save_revision = original.save_revision;
		if (critical_command_encode(command, &candidate.original_command) !=
		    critical_command_codec_result::ok)
			return EILSEQ;
		const auto &metadata = intent.admission.metadata;
		const auto lock_error = economic_sql_lock_authority(
			connection, metadata.lineage, metadata.epoch,
			std::span<const economic_sql_mapping_request>{}, &candidate.authority);
		if (lock_error)
			return lock_error;
		candidate.session_id = mysql_thread_id(connection);
		if (!(connection->server_status & SERVER_STATUS_IN_TRANS) ||
		    mysql_thread_id(connection) != candidate.session_id)
			return ENOTCONN;
		*context = std::move(candidate);
		return 0;
	}
	catch (const std::bad_alloc &)
	{
		return ENOMEM;
	}
#endif
}

unsigned int economic_sql_native_quest_record(MYSQL *connection, const critical_command &command,
					      const item_transfer_custody_delta *custody_delta,
					      unsigned int result_code, bool mutation_applied,
					      const economic_sql_native_quest_context &context)
{
#ifdef __NO_MYSQL__
	(void)connection;
	(void)command;
	(void)custody_delta;
	(void)result_code;
	(void)mutation_applied;
	(void)context;
	return ENOTSUP;
#else
	try
	{
		if (!connection || !(connection->server_status & SERVER_STATUS_IN_TRANS) ||
		    !context.session_id || mysql_thread_id(connection) != context.session_id ||
		    !native_quest_command_bound(command) ||
		    (result_code == 0) != mutation_applied || (mutation_applied && !custody_delta))
			return EINVAL;
		std::vector<uint8_t> current_command;
		if (!context.acknowledged_save_revision ||
		    critical_command_encode(command, &current_command) !=
			    critical_command_codec_result::ok ||
		    current_command != context.original_command)
			return EILSEQ;
		economic_frozen_intent intent;
		if (economic_intent_decode(command.accounting_intent, &intent) !=
		    economic_accounting_error::ok)
			return EINVAL;
		if (intent.admission.metadata.lineage.bytes != context.authority.lineage.bytes ||
		    intent.admission.metadata.epoch.bytes != context.authority.epoch.bytes)
			return ESTALE;
		if (!mutation_applied)
		{
			if (!insert_operation(connection, command, intent, nullptr, {},
					      result_code))
				return failure_code();
			return 0;
		}
		errno = EILSEQ;
		if (!native_quest_plan_matches(command, *custody_delta))
			return failure_code();
		economic_accounting_plan plan;
		if (economic_intent_plan_metadata(command, intent, &plan.metadata) !=
		    economic_accounting_error::ok)
			return EILSEQ;
		plan.items_before = custody_delta->before;
		plan.items_after = custody_delta->after;
		plan.item_events = custody_delta->events;
		if (economic_plan_normalize(&plan) != economic_accounting_error::ok ||
		    economic_plan_validate_structure(plan) != economic_accounting_error::ok)
			return EILSEQ;
		std::vector<uint8_t> encoded_plan;
		if (economic_plan_encode(plan, &encoded_plan) != economic_accounting_error::ok)
			return EILSEQ;
		if (!insert_operation(connection, command, intent, &plan, encoded_plan, 0))
			return failure_code();
		if (!insert_source_claim(connection, intent.admission.metadata))
			return failure_code();
		for (const auto &event : plan.item_events)
		{
			economic_accounting_item_reference reference = {};
			reference.operation_id = command.operation_id;
			reference.line_index = static_cast<uint16_t>(event.event_index);
			reference.event_index = event.event_index;
			reference.child_index = event.child_index;
			reference.item_uid = event.uid;
			reference.before_revision = event.before.revision;
			reference.after_revision = event.after.revision;
			reference.legacy_operation_id = command.operation_id;
			reference.legacy_event_index = static_cast<uint16_t>(event.event_index);
			if (!economic_accounting_item_reference_insert(connection, reference))
				return failure_code();
		}
		return 0;
	}
	catch (const std::bad_alloc &)
	{
		return ENOMEM;
	}
#endif
}

unsigned int economic_sql_native_quest_verify_retained(MYSQL *connection,
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
	try
	{
		if (!connection || !native_quest_command_bound(command))
			return EINVAL;
		economic_frozen_intent intent;
		if (economic_intent_decode(command.accounting_intent, &intent) !=
			    economic_accounting_error::ok ||
		    economic_intent_verify_binding(command, intent) !=
			    economic_accounting_error::ok)
			return EILSEQ;
		std::vector<std::optional<std::string>> values;
		if (!read_operation(connection, command.operation_id, &values))
			return failure_code();
		uint64_t outcome = 0, stored_code = 0, account_count = 0, posting_count = 0;
		uint64_t child_count = 0, event_count = 0, before_count = 0, after_count = 0;
		if (!values[0] || !parse_unsigned(*values[0], &outcome) || !values[1] ||
		    !parse_unsigned(*values[1], &stored_code) || !values[2] ||
		    values[2]->size() != 32 || !values[3] || values[3]->size() != 32 ||
		    !values[5] ||
		    *values[5] != std::string(reinterpret_cast<const char *>(
						      command.accounting_intent.data()),
					      command.accounting_intent.size()) ||
		    !values[7] || !parse_unsigned(*values[7], &account_count) || !values[8] ||
		    !parse_unsigned(*values[8], &posting_count) || !values[9] ||
		    !parse_unsigned(*values[9], &child_count) || !values[10] ||
		    !parse_unsigned(*values[10], &event_count) || !values[11] ||
		    !parse_unsigned(*values[11], &before_count) || !values[12] ||
		    !parse_unsigned(*values[12], &after_count) || stored_code != result_code ||
		    outcome != (result_code ? 2u : 1u))
			return EILSEQ;
		economic_digest intent_digest = {};
		const auto intent_digest_status = economic_intent_digest(intent, &intent_digest);
		if (intent_digest_status != economic_accounting_error::ok ||
		    !digest_matches(intent_digest, values[2]) ||
		    !digest_matches(intent.domain_digest, values[3]))
			return EILSEQ;
		uint64_t reference_count = 0, source_claim_count = 0;
		if (!read_reference_count(connection, command.operation_id, &reference_count))
			return failure_code();
		if (!read_source_claim_count(connection, command.operation_id, &source_claim_count))
			return failure_code();
		item_transfer_payload payload{};
		item_transfer_result retained_result{};
		if (!result_payload || result_size != ITEM_TRANSFER_RESULT_BYTES ||
		    !item_transfer_command_decode_payload(command, &payload) ||
		    !item_transfer_command_decode_result(result_payload, result_size,
							 &retained_result) ||
		    retained_result.root_item_uid != item_transfer_result_root(payload) ||
		    retained_result.item_count != payload.item_count ||
		    retained_result.corpse_revision || retained_result.collector_catalog_changed)
			return EILSEQ;
		if (result_code && retained_result.max_item_revision)
			return EILSEQ;
		if (result_code)
			return !values[4] && !values[6] && !account_count && !posting_count &&
					       !child_count && !event_count && !before_count &&
					       !after_count && !reference_count &&
					       !source_claim_count ?
				       0 :
				       EILSEQ;
		if (!result_payload || result_size != ITEM_TRANSFER_RESULT_BYTES || !values[4] ||
		    values[4]->size() != 32 || !values[6] || event_count == 0 || account_count ||
		    posting_count || child_count || event_count != reference_count)
			return EILSEQ;
		if (source_claim_count !=
		    static_cast<uint64_t>(intent.admission.metadata.source_event.has_value()))
			return EILSEQ;
		if (!item_transfer_command_decode_result(result_payload, result_size,
							 &retained_result) ||
		    !item_transfer_command_decode_payload(command, &payload) ||
		    retained_result.item_count != payload.item_count ||
		    (payload.reason != item_transfer_reason::craft &&
		     retained_result.item_count != event_count))
			return EILSEQ;
		if (payload.expected_from_revision == UINT64_MAX ||
		    payload.expected_to_revision == UINT64_MAX)
			return EILSEQ;
		uint64_t maximum_item_revision = 0;
		for (size_t i = 0; i < payload.item_count; ++i)
		{
			if (payload.items[i].expected_item_revision == UINT64_MAX)
				return EILSEQ;
			maximum_item_revision = std::max(
				maximum_item_revision, payload.items[i].expected_item_revision + 1);
		}
		if (retained_result.from_owner_revision != payload.expected_from_revision + 1 ||
		    retained_result.to_owner_revision != payload.expected_to_revision + 1 ||
		    retained_result.max_item_revision != maximum_item_revision)
			return EILSEQ;
		economic_accounting_plan plan;
		if (economic_plan_decode(std::span<const uint8_t>(reinterpret_cast<const uint8_t *>(
									  values[6]->data()),
								  values[6]->size()),
					 &plan) != economic_accounting_error::ok ||
		    economic_plan_validate_structure(plan) != economic_accounting_error::ok ||
		    plan.item_events.size() != event_count ||
		    plan.items_before.size() != before_count ||
		    plan.items_after.size() != after_count ||
		    economic_item_effects_validate(plan.items_before, plan.items_after,
						   plan.item_events, plan.children.size()) !=
			    economic_accounting_error::ok)
			return EILSEQ;
		economic_plan_metadata expected_metadata;
		if (economic_intent_plan_metadata(command, intent, &expected_metadata) !=
			    economic_accounting_error::ok ||
		    !metadata_matches(plan.metadata, expected_metadata))
			return EILSEQ;
		economic_digest expected_plan_digest = {};
		if (economic_plan_digest(plan, &expected_plan_digest) !=
			    economic_accounting_error::ok ||
		    !digest_matches(expected_plan_digest, values[4]))
			return EILSEQ;
		std::vector<uint8_t> canonical_plan;
		if (economic_plan_encode(plan, &canonical_plan) != economic_accounting_error::ok ||
		    std::string(reinterpret_cast<const char *>(canonical_plan.data()),
				canonical_plan.size()) != *values[6])
			return EILSEQ;
		if (payload.reason == item_transfer_reason::craft)
		{
			const auto &inputs = plan.items_before;
			economic_accounting_plan expected;
			expected.metadata = expected_metadata;
			std::vector<uint8_t> encoded_expected;
			if (item_transfer_craft_accounting_effects(payload, inputs, &expected) !=
				    economic_accounting_error::ok ||
			    economic_plan_normalize(&expected) != economic_accounting_error::ok ||
			    economic_plan_encode(expected, &encoded_expected) !=
				    economic_accounting_error::ok ||
			    encoded_expected != canonical_plan)
				return EILSEQ;
		}
		item_transfer_custody_delta original_effects;
		original_effects.before = plan.items_before;
		original_effects.after = plan.items_after;
		original_effects.events = plan.item_events;
		errno = EILSEQ;
		if (!native_quest_plan_matches(command, original_effects))
			return failure_code();
		for (size_t index = 0; index < plan.item_events.size(); ++index)
		{
			const auto &event = plan.item_events[index];
			economic_accounting_item_reference reference = {};
			if (event.event_index != index || event.child_index != 0 ||
			    !economic_accounting_item_reference_find_by_legacy(
				    connection, command.operation_id, static_cast<uint16_t>(index),
				    &reference) ||
			    reference.operation_id.bytes != command.operation_id.bytes ||
			    reference.line_index != index || reference.event_index != index ||
			    reference.child_index != event.child_index ||
			    reference.item_uid != event.uid ||
			    reference.before_revision != event.before.revision ||
			    reference.after_revision != event.after.revision)
				return EILSEQ;
		}
		// A successful ordinary drop also owns immutable full-literal room
		// payload and native ledger proof. Missing history remains retryable;
		// do not fabricate evidence from current custody or player projections.
		if (payload.reason == item_transfer_reason::player_drop &&
		    !sql_room_item_payload_verify_retained(connection, command, retained_result))
			return failure_code();
		return 0;
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
void native_publication_require(bool condition, unsigned int code)
{
	if (!condition)
		throw code;
}

void native_publication_sql(bool success)
{
	if (!success)
		throw failure_code();
}

void native_publication_session(MYSQL *connection, unsigned long session)
{
	using flag = std::remove_pointer_t<decltype(MYSQL_BIND{}.is_null)>;
	flag reconnect = true;
	native_publication_require(
		connection && session && mysql_thread_id(connection) == session &&
			(connection->server_status & SERVER_STATUS_IN_TRANS) &&
			(connection->server_status & SERVER_STATUS_AUTOCOMMIT) &&
			!mysql_get_option(connection, MYSQL_OPT_RECONNECT, &reconnect) &&
			!reconnect,
		ENOTCONN);
}

void native_publication_codec(player_snapshot_codec_result code)
{
	native_publication_require(
		code == player_snapshot_codec_result::ok,
		code == player_snapshot_codec_result::allocation_failure ?
			ENOMEM :
			(code == player_snapshot_codec_result::limit_exceeded ? E2BIG : EILSEQ));
}

std::vector<uint8_t> native_publication_forest(std::span<const player_item_snapshot> items)
{
	native_publication_require(items.size() <= PLAYER_SNAPSHOT_MAX_OBJECTS, E2BIG);
	const std::vector<player_item_snapshot> values(items.begin(), items.end());
	std::vector<uint8_t> encoded;
	native_publication_codec(player_item_snapshot_list_encode(values, &encoded));
	native_publication_require(encoded.size() <= PLAYER_SNAPSHOT_MAX_BYTES, E2BIG);
	return encoded;
}

uint64_t native_publication_save_revision(MYSQL *connection, uint32_t pid)
{
	native_publication_sql(
		execute(connection, "SELECT save_revision FROM player_data WHERE pid=" +
					    std::to_string(pid) + " LIMIT 2 FOR UPDATE"));
	std::unique_ptr<MYSQL_RES, decltype(&mysql_free_result)> rows(
		mysql_store_result(connection), mysql_free_result);
	if (!rows)
		throw mysql_errno(connection) ? mysql_errno(connection) :
						static_cast<unsigned int>(EIO);
	native_publication_require(
		mysql_num_fields(rows.get()) == 1 && mysql_num_rows(rows.get()) == 1, EILSEQ);
	MYSQL_ROW row = mysql_fetch_row(rows.get());
	unsigned long *lengths = mysql_fetch_lengths(rows.get());
	if (!row || !lengths)
		throw mysql_errno(connection) ? mysql_errno(connection) :
						static_cast<unsigned int>(EILSEQ);
	uint64_t revision = 0;
	native_publication_require(row && lengths && row[0] &&
					   parse_unsigned(std::string(row[0], lengths[0]),
							  &revision),
				   EILSEQ);
	return revision;
}

void native_publication_absence(MYSQL *connection, uint64_t uid, bool player)
{
	// Match SHOP's bounded physical-domain proof. Sidecar/ledger history is retained.
	for (const char *table :
	     { "player_items", "shopkeeper_items", "player_pet_items", "locker_items",
	       "account_locker_items", "corpse_items", "saved_items", "siege_items" })
	{
		native_publication_sql(execute(
			connection, "SELECT id FROM " + std::string(table) + " WHERE obj_uid=" +
					    std::to_string(uid) + " LIMIT 2 FOR UPDATE"));
		std::unique_ptr<MYSQL_RES, decltype(&mysql_free_result)> rows(
			mysql_store_result(connection), mysql_free_result);
		if (!rows)
			throw mysql_errno(connection) ? mysql_errno(connection) :
							static_cast<unsigned int>(EIO);
		native_publication_require(mysql_num_fields(rows.get()) == 1, EILSEQ);
		native_publication_require(
			mysql_num_rows(rows.get()) ==
				(player && !std::strcmp(table, "player_items") ? 1u : 0u),
			ESTALE);
	}
}

bool native_publication_position_equal(const economic_item_position &a,
				       const economic_item_position &b)
{
	return item_owner_identity_equal(a.owner, b.owner) && a.root_uid == b.root_uid &&
	       a.parent_uid == b.parent_uid && a.revision == b.revision && a.state == b.state &&
	       a.equipment_slot == b.equipment_slot;
}

void native_publication_add_forest(
	std::span<const player_item_snapshot> items, const item_owner_identity &owner,
	const std::vector<item_native_quest_publication_custody> &cut,
	std::map<uint64_t, item_native_quest_publication_custody> *expected)
{
	std::vector<uint64_t> roots(items.size());
	for (size_t i = 0; i < items.size(); ++i)
	{
		const auto &item = items[i];
		native_publication_require(item.parent_index >= PLAYER_SNAPSHOT_NO_PARENT &&
						   item.parent_index < static_cast<int32_t>(i),
					   EILSEQ);
		const bool root = item.parent_index == PLAYER_SNAPSHOT_NO_PARENT;
		roots[i] = root ? item.object_uid : roots[item.parent_index];
		const auto row = std::lower_bound(cut.begin(), cut.end(), item.object_uid,
						  [](const auto &entry, uint64_t uid)
						  { return entry.snapshot.uid < uid; });
		native_publication_require(row != cut.end() && row->snapshot.uid == item.object_uid,
					   ESTALE);
		// Unselected item revisions are observed under the locked owner/image cut;
		// they are never manufactured from literal player/NPC snapshots.
		const economic_item_position position{ owner,
						       roots[i],
						       root ? 0 :
							      items[item.parent_index].object_uid,
						       row->snapshot.position.revision,
						       item_custody_state::active,
						       static_cast<uint16_t>(item.equipment_slot) };
		native_publication_require(
			native_publication_position_equal(row->snapshot.position, position) &&
				row->vnum == item.vnum && position.revision &&
				position.revision != UINT64_MAX &&
				expected->emplace(item.object_uid,
						  item_native_quest_publication_custody{
							  { item.object_uid, position },
							  item.vnum })
					.second,
			ESTALE);
	}
}
}
#endif

static unsigned int
native_quest_lock_projection(MYSQL *connection, const critical_command &command,
			     const critical_apply_result &receipt_core,
			     std::span<const player_item_snapshot> original_native_before,
			     std::span<const player_item_snapshot> original_player_before,
			     std::span<const player_item_snapshot> original_player_after,
			     uint64_t acknowledged_save_revision,
			     economic_sql_native_quest_publication *output) noexcept
{
#ifdef __NO_MYSQL__
	(void)connection;
	(void)command;
	(void)receipt_core;
	(void)original_native_before;
	(void)original_player_before;
	(void)original_player_after;
	(void)acknowledged_save_revision;
	(void)output;
	return ENOTSUP;
#else
	if (!connection || !output)
		return EINVAL;
	try
	{
		const unsigned long session = mysql_thread_id(connection);
		native_publication_session(connection, session);
		native_publication_require(native_quest_command_bound(command), EPROTONOSUPPORT);
		item_transfer_payload payload{};
		native_publication_require(
			item_transfer_command_decode_payload(command, &payload) &&
				item_transfer_native_mobile_recovery_shape_valid(payload),
			EILSEQ);
		const bool acceptance = payload.native_mobile.action ==
					item_native_mobile_action::acceptance;
		const bool rejected = receipt_core.outcome ==
				      critical_apply_outcome::terminal_failure;
		item_transfer_result receipt{};
		std::array<uint8_t, ITEM_TRANSFER_RESULT_BYTES> encoded{};
		native_publication_require(
			receipt_core.failure_stage == critical_failure_stage::none &&
				(rejected ?
					 receipt_core.error_code != 0 :
					 !receipt_core.error_code &&
						 (receipt_core.outcome ==
							  critical_apply_outcome::applied ||
						  receipt_core.outcome ==
							  critical_apply_outcome::already_applied)) &&
				receipt_core.result_size == encoded.size() &&
				item_transfer_command_decode_result(
					receipt_core.result_payload.data(),
					receipt_core.result_size, &receipt) &&
				item_transfer_command_encode_result(receipt, &encoded) &&
				std::equal(encoded.begin(), encoded.end(),
					   receipt_core.result_payload.begin()) &&
				std::all_of(receipt_core.result_payload.begin() + encoded.size(),
					    receipt_core.result_payload.end(),
					    [](uint8_t byte) { return !byte; }) &&
				receipt.root_item_uid == item_transfer_result_root(payload) &&
				receipt.item_count == payload.item_count &&
				!receipt.corpse_revision && !receipt.collector_catalog_changed &&
				receipt_core.durable_revision ==
					std::max({ receipt.from_owner_revision,
						   receipt.to_owner_revision,
						   receipt.max_item_revision }),
			EILSEQ);
		uint64_t maximum = 0;
		for (size_t i = 0; i < payload.item_count; ++i)
		{
			native_publication_require(
				payload.items[i].expected_item_revision != UINT64_MAX, EILSEQ);
			maximum = std::max(maximum, payload.items[i].expected_item_revision + 1);
		}
		native_publication_require(
			rejected ? !receipt.max_item_revision :
				   payload.expected_from_revision != UINT64_MAX &&
					   payload.expected_to_revision != UINT64_MAX &&
					   receipt.from_owner_revision ==
						   payload.expected_from_revision + 1 &&
					   receipt.to_owner_revision ==
						   payload.expected_to_revision + 1 &&
					   receipt.max_item_revision == maximum,
			EILSEQ);
		native_publication_require(
			acknowledged_save_revision &&
				acknowledged_save_revision ==
					payload.native_recovery.acknowledged_save_revision &&
				payload.native_recovery.player_pid ==
					payload.native_mobile.final_giver_pid,
			EILSEQ);
		const auto player_before_bytes = native_publication_forest(original_player_before);
		const auto player_after_bytes = native_publication_forest(original_player_after);
		(void)native_publication_forest(original_native_before);
		if (acceptance)
		{
			native_publication_require(
				shop_trade_recovery_forest_verify(
					player_before_bytes,
					shop_trade_recovery_forest_role::player_before,
					payload.native_recovery.player_before) &&
					shop_trade_recovery_forest_verify(
						player_after_bytes,
						shop_trade_recovery_forest_role::player_after,
						payload.native_recovery.player_after),
				EILSEQ);
			std::vector<player_item_snapshot> selected, player_after;
			const std::vector<player_item_snapshot> before(
				original_player_before.begin(), original_player_before.end());
			native_publication_codec(player_item_snapshot_extract_subtree(
				before, item_transfer_result_root(payload), &selected,
				&player_after));
			native_publication_require(
				native_publication_forest(player_after) == player_after_bytes &&
					native_publication_forest(selected) ==
						std::vector<uint8_t>(payload.item_blob.begin(),
								     payload.item_blob.begin() +
									     payload.item_blob_size),
				EILSEQ);
		}
		else
			native_publication_require(player_before_bytes == player_after_bytes,
						   EILSEQ);

		// Exact lifetime first, then original save fence, sorted revision owners,
		// one complete ascending custody cut, and only then physical/sidecar rows.
		quest_mobile_native_sql_row native;
		const int native_code = quest_mobile_native_sql_lock(
			connection, payload.native_mobile.reference.mobile_instance_id, &native);
		native_publication_require(!native_code, static_cast<unsigned int>(native_code));
		native_publication_require(native.present && native.original_session == session &&
						   native.image.state ==
							   quest_mobile_lifetime_state::live &&
						   native.image.cash.has_value(),
					   ENODATA);
		auto expected_reference = payload.native_mobile.reference;
		std::vector<player_item_snapshot> native_after;
		if (!rejected)
		{
			native_publication_codec(quest_mobile_native_items_transition(
				original_native_before, payload.native_mobile.reference, payload,
				&native_after));
			native_publication_require(
				expected_reference.mobile_revision != UINT64_MAX &&
					expected_reference.stock_revision != UINT64_MAX,
				ERANGE);
			++expected_reference.mobile_revision;
			++expected_reference.stock_revision;
			native_publication_require(native.image.last_transition_operation.bytes ==
							   command.operation_id.bytes,
						   ESTALE);
		}
		std::array<uint8_t, QUEST_MOBILE_NATIVE_REFERENCE_BYTES> expected_bytes{},
			current_bytes{};
		native_publication_codec(
			quest_mobile_native_reference_encode(expected_reference, &expected_bytes));
		native_publication_codec(quest_mobile_native_reference_encode(
			native.image.reference, &current_bytes));
		const auto expected_native =
			rejected ? original_native_before :
				   std::span<const player_item_snapshot>(native_after);
		native_publication_require(
			expected_bytes == current_bytes &&
				native_publication_forest(native.image.items) ==
					native_publication_forest(expected_native),
			ESTALE);
		native_publication_require(
			native_publication_save_revision(connection,
							 payload.native_recovery.player_pid) ==
				acknowledged_save_revision,
			ESTALE);
		const item_owner_identity player_owner{ item_owner_type::player,
							payload.native_recovery.player_pid, 0 };
		const item_owner_identity native_owner{
			item_owner_type::native_mobile,
			payload.native_mobile.reference.mobile_instance_id, 0
		};
		std::vector<item_owner_identity> owners{ payload.from_owner, payload.to_owner,
							 player_owner };
		std::sort(owners.begin(), owners.end(),
			  [](const auto &a, const auto &b) {
				  return std::tie(a.type, a.id, a.context_id) <
					 std::tie(b.type, b.id, b.context_id);
			  });
		owners.erase(std::unique(owners.begin(), owners.end(), item_owner_identity_equal),
			     owners.end());
		std::vector<uint64_t> revisions(owners.size());
		for (size_t i = 0; i < owners.size(); ++i)
			native_publication_sql(item_transfer_repository_lock_owner(
				connection, owners[i], &revisions[i]));
		const auto revision = [&](const item_owner_identity &owner)
		{
			for (size_t i = 0; i < owners.size(); ++i)
				if (item_owner_identity_equal(owners[i], owner))
					return revisions[i];
			throw static_cast<unsigned int>(EILSEQ);
		};
		const uint64_t from_revision = revision(payload.from_owner);
		const uint64_t to_revision = revision(payload.to_owner);
		native_publication_require(
			revision(native_owner) == expected_reference.stock_revision &&
				from_revision == (rejected ? payload.expected_from_revision :
							     receipt.from_owner_revision) &&
				(acceptance ?
					 to_revision == (rejected ? payload.expected_to_revision :
								    receipt.to_owner_revision) :
					 to_revision >=
						 (rejected ? std::max(payload.expected_to_revision,
								      receipt.to_owner_revision) :
							     receipt.to_owner_revision)),
			ESTALE);
		std::vector<item_native_quest_publication_custody> cut;
		native_publication_sql(item_transfer_repository_lock_native_publication_custody(
			connection, payload, original_native_before, original_player_before,
			original_player_after, &cut));
		const auto expected_player = rejected ? original_player_before :
							original_player_after;
		std::map<uint64_t, item_native_quest_publication_custody> expected;
		native_publication_add_forest(expected_native, native_owner, cut, &expected);
		native_publication_add_forest(expected_player, player_owner, cut, &expected);
		for (size_t i = 0; i < payload.item_count; ++i)
		{
			const auto &entry = payload.items[i];
			uint64_t root = entry.root_item_uid, parent = entry.parent_item_uid;
			if (!rejected)
				native_publication_require(
					item_transfer_target_topology(payload, entry.item_uid,
								      &root, &parent),
					EILSEQ);
			const economic_item_position position{
				rejected ? payload.from_owner : payload.to_owner,
				root,
				parent,
				entry.expected_item_revision + (rejected ? 0 : 1),
				!rejected && !acceptance ? item_custody_state::destroyed :
							   item_custody_state::active,
				0
			};
			const auto row = std::lower_bound(cut.begin(), cut.end(), entry.item_uid,
							  [](const auto &value, uint64_t uid)
							  { return value.snapshot.uid < uid; });
			native_publication_require(
				row != cut.end() && row->snapshot.uid == entry.item_uid &&
					native_publication_position_equal(row->snapshot.position,
									  position) &&
					row->vnum == entry.vnum,
				ESTALE);
			if (!rejected && !acceptance)
				native_publication_require(
					expected.emplace(entry.item_uid,
							 item_native_quest_publication_custody{
								 { entry.item_uid, position },
								 entry.vnum })
						.second,
					EILSEQ);
			else
			{
				const auto found = expected.find(entry.item_uid);
				native_publication_require(
					found != expected.end() &&
						native_publication_position_equal(
							found->second.snapshot.position, position),
					ESTALE);
			}
		}
		native_publication_require(expected.size() == cut.size(), ESTALE);
		// Reject every extra foreign active root/parent claim or malformed owner context.
		for (const auto &row : cut)
		{
			const auto found = expected.find(row.snapshot.uid);
			native_publication_require(found != expected.end() &&
							   found->second.vnum == row.vnum &&
							   native_publication_position_equal(
								   found->second.snapshot.position,
								   row.snapshot.position),
						   ESTALE);
		}
		shop_item_runtime_image player_image;
		native_publication_sql(shop_item_runtime_lock_player_image(
			connection, payload.native_recovery.player_pid, expected_player,
			&player_image));
		for (const auto &row : cut)
			native_publication_absence(
				connection, row.snapshot.uid,
				item_owner_identity_equal(row.snapshot.position.owner,
							  player_owner) &&
					row.snapshot.position.state == item_custody_state::active);
		economic_sql_native_quest_publication candidate;
		candidate.session_id = session;
		candidate.native = std::move(
			native.image); // Actual current cash-v2, no fabricated past image.
		candidate.from_owner_revision = from_revision;
		candidate.to_owner_revision = to_revision;
		candidate.player_owner_revision = revision(player_owner);
		candidate.acknowledged_save_revision = acknowledged_save_revision;
		candidate.custody.reserve(cut.size());
		for (const auto &row : cut)
		{
			const auto &position = row.snapshot.position;
			candidate.custody.push_back({ row.snapshot.uid, position.root_uid,
						      position.parent_uid, position.owner,
						      position.revision, revision(position.owner),
						      row.vnum, position.state });
		}
		native_publication_session(connection, session);
		*output = std::move(candidate);
		return 0;
	}
	catch (unsigned int code)
	{
		return code ? code : EILSEQ;
	}
	catch (const std::bad_alloc &)
	{
		return ENOMEM;
	}
#endif
}

unsigned int economic_sql_native_quest_lock_publication(
	MYSQL *connection, const critical_command &command, const critical_completion &completion,
	std::span<const player_item_snapshot> original_native_before,
	std::span<const player_item_snapshot> original_player_before,
	std::span<const player_item_snapshot> original_player_after,
	uint64_t acknowledged_save_revision, economic_sql_native_quest_publication *output) noexcept
{
#ifndef __NO_MYSQL__
	if (!connection || !output)
		return EINVAL;
	if (completion.operation_id.bytes != command.operation_id.bytes ||
	    completion.disposition != critical_completion_disposition::execution)
		return EILSEQ;
#endif
	const critical_apply_result receipt_core{
		completion.outcome,	  completion.durable_revision, completion.error_code,
		completion.failure_stage, completion.result_size,      completion.result_payload
	};
	return native_quest_lock_projection(connection, command, receipt_core,
					    original_native_before, original_player_before,
					    original_player_after, acknowledged_save_revision,
					    output);
}

unsigned int economic_sql_native_quest_lock_recovered_continuation(
	MYSQL *connection, const critical_native_recovery_envelope &envelope,
	std::span<const player_item_snapshot> original_native_before,
	std::span<const player_item_snapshot> original_player_before,
	std::span<const player_item_snapshot> original_player_after,
	uint64_t acknowledged_save_revision, economic_sql_native_quest_publication *output) noexcept
{
#ifdef __NO_MYSQL__
	(void)connection;
	(void)envelope;
	(void)original_native_before;
	(void)original_player_before;
	(void)original_player_after;
	(void)acknowledged_save_revision;
	(void)output;
	return ENOTSUP;
#else
	if (!connection || !output)
		return EINVAL;
	try
	{
		const unsigned long session = mysql_thread_id(connection);
		native_publication_session(connection, session);
		native_quest_recovery_context context;
		native_publication_require(
			envelope.revision &&
				envelope.phase ==
					critical_native_recovery_phase::continuation_pending &&
				native_quest_command_bound(envelope.command) &&
				native_quest_recovery_context_decode(
					envelope.command, envelope.attachment, &context) ==
					player_snapshot_codec_result::ok &&
				context.publication_stage ==
					native_quest_recovery_publication_stage::physically_proven &&
				context.receipt.present,
			EILSEQ);
		native_publication_require(
			native_publication_forest(original_native_before) ==
					native_publication_forest(context.native_before) &&
				native_publication_forest(original_player_before) ==
					native_publication_forest(context.player_before),
			EILSEQ);
		const auto &stored = context.receipt;
		// Values select the original BEFORE/AFTER cut; they grant no receipt
		// authority. Acquire the original domain locks before historical readback.
		const critical_apply_result retained{ stored.outcome,	  stored.durable_revision,
						      stored.error_code,  stored.failure_stage,
						      stored.result_size, stored.result_payload };
		economic_sql_native_quest_publication candidate;
		native_publication_sql(native_quest_lock_projection(
			connection, envelope.command, retained, original_native_before,
			original_player_before, original_player_after, acknowledged_save_revision,
			&candidate));
		const auto actual = critical_command_repository_verify_native_quest_in_transaction(
			connection, envelope.command);
		const bool stored_success = stored.outcome == critical_apply_outcome::applied ||
					    stored.outcome ==
						    critical_apply_outcome::already_applied;
		const bool actual_success = actual.outcome == critical_apply_outcome::applied ||
					    actual.outcome ==
						    critical_apply_outcome::already_applied;
		native_publication_require(
			(stored_success ? actual_success : actual.outcome == stored.outcome) &&
				actual.durable_revision == stored.durable_revision &&
				actual.error_code == stored.error_code &&
				actual.failure_stage == stored.failure_stage &&
				actual.result_size == stored.result_size &&
				actual.result_payload == stored.result_payload &&
				candidate.session_id == session,
			EILSEQ);
		native_publication_session(connection, session);
		*output = std::move(candidate);
		return 0;
	}
	catch (unsigned int code)
	{
		return code ? code : EILSEQ;
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

#ifndef __NO_MYSQL__
namespace
{
using native_money_fields = std::vector<std::pair<std::string, std::string>>;
bool native_money_command_bound(const critical_command &command, item_transfer_payload *payload,
				economic_frozen_intent *intent)
{
	if (!payload || !intent ||
	    command.schema_version != CRITICAL_COMMAND_ACCOUNTING_SCHEMA_VERSION ||
	    command.payload_version != ITEM_TRANSFER_NATIVE_MOBILE_MONEY_RECOVERY_PAYLOAD_VERSION ||
	    !command.publication_required || !command.accepted_at_usec ||
	    !critical_command_envelope_valid(command) ||
	    !item_transfer_command_decode_payload(command, payload) ||
	    !payload->native_money.present ||
	    !item_transfer_native_mobile_recovery_shape_valid(*payload) ||
	    economic_intent_decode(command.accounting_intent, intent) !=
		    economic_accounting_error::ok ||
	    economic_intent_verify_binding(command, *intent) != economic_accounting_error::ok)
		return false;
	auto original = command;
	original.schema_version = CRITICAL_COMMAND_SCHEMA_VERSION;
	original.accounting_intent.clear();
	original.accepted_at_usec = 0;
	original.publication_required = false;
	std::vector<uint8_t> rebuilt;
	const auto &m = intent->admission.metadata;
	return !m.source_event &&
	       item_native_mobile_accounting_intent(original, m.lineage, m.epoch,
						    payload->native_mobile.final_giver_pid, nullptr,
						    &rebuilt) == economic_accounting_error::ok &&
	       rebuilt == command.accounting_intent;
}
bool native_fee_command_bound(const critical_command &command, item_transfer_payload *payload,
			      economic_frozen_intent *intent)
{
	if (!payload || !intent ||
	    command.schema_version != CRITICAL_COMMAND_ACCOUNTING_SCHEMA_VERSION ||
	    command.payload_version != ITEM_TRANSFER_NATIVE_MOBILE_COST_RECOVERY_PAYLOAD_VERSION ||
	    !command.publication_required || !command.accepted_at_usec ||
	    !critical_command_envelope_valid(command) ||
	    !item_transfer_command_decode_payload(command, payload) ||
	    !payload->native_cost.fee_only ||
	    !item_transfer_native_mobile_recovery_shape_valid(*payload) ||
	    economic_intent_decode(command.accounting_intent, intent) !=
		    economic_accounting_error::ok ||
	    economic_intent_verify_binding(command, *intent) != economic_accounting_error::ok)
		return false;
	auto original = command;
	original.schema_version = CRITICAL_COMMAND_SCHEMA_VERSION;
	original.accounting_intent.clear();
	original.accepted_at_usec = 0;
	original.publication_required = false;
	std::vector<uint8_t> rebuilt;
	const auto &m = intent->admission.metadata;
	return m.source_event && m.reason == economic_reason::quest_cost &&
	       m.source_event->kind == economic_source_kind::quest_action &&
	       m.source_event->source.bytes == command.operation_id.bytes &&
	       m.source_event->generation.bytes ==
		       payload->native_mobile.reference.birth_source.generation.bytes &&
	       m.source_event->sequence == payload->native_mobile.reference.mobile_revision &&
	       m.source_event->slot == payload->native_cost.completion_slot &&
	       item_native_mobile_accounting_intent(original, m.lineage, m.epoch,
						    payload->native_mobile.final_giver_pid,
						    &*m.source_event,
						    &rebuilt) == economic_accounting_error::ok &&
	       rebuilt == command.accounting_intent;
}
bool native_money_session(MYSQL *connection, unsigned long session)
{
	using flag = std::remove_pointer_t<decltype(MYSQL_BIND{}.is_null)>;
	flag reconnect = true;
	return connection && session && mysql_thread_id(connection) == session &&
	       (connection->server_status & SERVER_STATUS_IN_TRANS) &&
	       !mysql_get_option(connection, MYSQL_OPT_RECONNECT, &reconnect) && !reconnect;
}
bool native_money_count(MYSQL *connection, const std::string &table, const std::string &where,
			uint64_t expected)
{
	if (!execute(connection, "SELECT COUNT(*) FROM " + table + " WHERE " + where))
		return false;
	std::unique_ptr<MYSQL_RES, decltype(&mysql_free_result)> rows(
		mysql_store_result(connection), mysql_free_result);
	if (!rows || mysql_num_rows(rows.get()) != 1 || mysql_num_fields(rows.get()) != 1)
	{
		errno = EILSEQ;
		return false;
	}
	auto row = mysql_fetch_row(rows.get());
	uint64_t actual = 0;
	if (!row || !row[0] || !parse_unsigned(row[0], &actual) || actual != expected)
	{
		errno = EILSEQ;
		return false;
	}
	return true;
}
std::string native_money_predicate(const native_money_fields &fields)
{
	std::string result;
	for (const auto &[name, value] : fields)
	{
		if (!result.empty())
			result += " AND ";
		result += name + (value == "NULL" ? " IS NULL" : "=" + value);
	}
	return result;
}
bool native_money_rows(MYSQL *connection, const char *table, const native_money_fields &fields,
		       bool append)
{
	if (append)
	{
		std::string names, data;
		for (const auto &[name, value] : fields)
		{
			if (!names.empty())
			{
				names += ',';
				data += ',';
			}
			names += name;
			data += value;
		}
		if (!execute(connection, std::string("INSERT INTO ") + table + "(" + names +
						 ") VALUES(" + data + ")"))
			return false;
		if (mysql_affected_rows(connection) != 1)
		{
			errno = EILSEQ;
			return false;
		}
	}
	return native_money_count(connection, table, native_money_predicate(fields), 1);
}
void native_money_coins(native_money_fields *fields, const char *prefix,
			const economic_coin_vector &cash)
{
	constexpr std::array<const char *, 4> names{ "copper", "silver", "gold", "platinum" };
	for (size_t i = 0; i < 4; ++i)
		fields->emplace_back(std::string(prefix) + names[i], std::to_string(cash[i]));
}
bool native_money_financial_rows(MYSQL *connection, const critical_operation_id &root,
				 const economic_accounting_plan &plan, bool append)
{
	for (size_t i = 0; i < plan.accounts.size(); ++i)
	{
		const auto &a = plan.accounts[i];
		std::array<uint8_t, ECONOMIC_ACCOUNT_KEY_BYTES> key{};
		if (economic_account_key_encode(a.key, &key) != economic_accounting_error::ok)
		{
			errno = EILSEQ;
			return false;
		}
		native_money_fields row{ { "operation_id", id(root) },
					 { "account_index", std::to_string(i) },
					 { "account_key", hex(key) } };
		native_money_coins(&row, "before_", a.before);
		native_money_coins(&row, "after_", a.after);
		row.emplace_back("before_revision", std::to_string(a.before_revision));
		row.emplace_back("after_revision", std::to_string(a.after_revision));
		if (!native_money_rows(connection, "economic_accounting_account_effect", row,
				       append))
			return false;
	}
	for (size_t i = 0; i < plan.postings.size(); ++i)
	{
		const auto &a = plan.postings[i];
		native_money_fields row{ { "operation_id", id(root) },
					 { "line_index", std::to_string(i) },
					 { "event_index", std::to_string(a.event_index) },
					 { "account_index", std::to_string(a.account_index) },
					 { "child_index", std::to_string(a.child_index) },
					 { "copper_value", std::to_string(a.copper) } };
		native_money_coins(&row, "delta_", a.delta);
		if (!native_money_rows(connection, "economic_accounting_coin_posting", row, append))
			return false;
	}
	const std::string where = "operation_id=" + id(root);
	return native_money_count(connection, "economic_accounting_account_effect", where,
				  plan.accounts.size()) &&
	       native_money_count(connection, "economic_accounting_coin_posting", where,
				  plan.postings.size()) &&
	       native_money_count(connection, "economic_accounting_child", where, 0) &&
	       native_money_count(connection, "economic_accounting_item_reference", where, 0) &&
	       native_money_count(connection, "economic_accounting_source_claim", where, 0) &&
	       native_money_count(connection, "item_ownership_ledger", where, 0);
}
bool native_money_expected_plan(const critical_command &command,
				const item_transfer_payload &payload,
				const economic_frozen_intent &intent,
				economic_accounting_plan *output)
{
	economic_accounting_plan plan;
	if (economic_intent_plan_metadata(command, intent, &plan.metadata) !=
	    economic_accounting_error::ok)
		return false;
	const economic_account_key player{ plan.metadata.lineage, economic_account_kind::wallet,
					   payload.native_money.player_wallet_mapping_id, 0 };
	const economic_account_key native{ plan.metadata.lineage, economic_account_kind::wallet,
					   payload.native_money.mobile_wallet_mapping_id,
					   ECONOMIC_NATIVE_MOBILE_WALLET_CONTEXT };
	if (item_native_mobile_money_accounting_effects(payload, player, native, &plan) !=
		    economic_accounting_error::ok ||
	    economic_plan_normalize(&plan) != economic_accounting_error::ok ||
	    economic_plan_validate_structure(plan) != economic_accounting_error::ok)
		return false;
	*output = std::move(plan);
	return true;
}
bool native_money_operation_matches(MYSQL *connection, const critical_command &command,
				    const economic_accounting_plan &plan, unsigned int result_code)
{
	const auto &m = plan.metadata;
	std::vector<uint8_t> encoded;
	economic_digest digest{};
	if (!result_code &&
	    (economic_plan_encode(plan, &encoded) != economic_accounting_error::ok ||
	     economic_plan_digest(plan, &digest) != economic_accounting_error::ok))
	{
		errno = EILSEQ;
		return false;
	}
	native_money_fields fields{
		{ "operation_id", id(m.operation_id) },
		{ "lineage", id(m.lineage) },
		{ "epoch", id(m.epoch) },
		{ "original_operation_id", optional_id(m.original_operation_id) },
		{ "accounting_version", std::to_string(m.version) },
		{ "writer_id", std::to_string(m.writer_id) },
		{ "policy_version", std::to_string(m.policy_version) },
		{ "compiler_version", std::to_string(m.compiler_version) },
		{ "actor_kind", std::to_string(static_cast<uint8_t>(m.actor_kind)) },
		{ "actor_id", std::to_string(m.actor_id) },
		{ "reason", std::to_string(static_cast<uint16_t>(m.reason)) },
		{ "source_event", "NULL" },
		{ "intent_digest", hex(m.intent_digest) },
		{ "domain_digest", hex(m.domain_digest) },
		{ "plan_digest", result_code ? "NULL" : hex(digest) },
		{ "canonical_intent", hex(command.accounting_intent) },
		{ "canonical_plan", result_code ? "NULL" : hex(encoded) },
		{ "outcome", result_code ? "2" : "1" },
		{ "result_code", std::to_string(result_code) },
		{ "realized_price_copper", "NULL" },
		{ "account_count", std::to_string(result_code ? 0 : plan.accounts.size()) },
		{ "posting_count", std::to_string(result_code ? 0 : plan.postings.size()) },
		{ "child_count", "0" },
		{ "item_event_count", "0" },
		{ "before_witness_count", "0" },
		{ "after_witness_count", "0" }
	};
	return native_money_count(connection, "economic_accounting_operation",
				  native_money_predicate(fields), 1);
}

bool native_money_historical_mapping(MYSQL *connection, const economic_account_key &key,
				     uint16_t locator, uint64_t native_id)
{
	return native_money_count(
		connection, "economic_account_mapping",
		"mapping_id=" + std::to_string(key.authority_id) +
			" AND lineage=" + id(key.lineage) +
			" AND account_kind=" + std::to_string(static_cast<uint16_t>(key.kind)) +
			" AND context_id=" + std::to_string(key.context_id) +
			" AND backend_kind=" + std::to_string(ECONOMIC_MAPPING_BACKEND_SQL) +
			" AND locator_kind=" + std::to_string(locator) +
			" AND native_id=" + std::to_string(native_id),
		1);
}
}
#endif

unsigned int economic_sql_native_money_lock(MYSQL *connection, const critical_command &command,
					    economic_sql_native_money_context *output) noexcept
{
#ifdef __NO_MYSQL__
	(void)connection;
	(void)command;
	(void)output;
	return ENOTSUP;
#else
	try
	{
		if (!connection || !output ||
		    !native_money_session(connection, mysql_thread_id(connection)))
			return ENOTCONN;
		item_transfer_payload payload{};
		economic_frozen_intent intent;
		if (!native_money_command_bound(command, &payload, &intent))
			return EINVAL;
		economic_sql_native_money_context candidate;
		player_native_quest_checkpoint_stage held;
		if (!player_save_native_quest_checkpoint_owner::original_held_bodies(
			    command, &candidate.original.original_player_before,
			    &candidate.original.original_player_after, &held))
			return ENODATA;
		candidate.original.acknowledged_save_revision = held.save_revision;
		if (critical_command_encode(command, &candidate.original.original_command) !=
		    critical_command_codec_result::ok)
			return EILSEQ;
		const auto &m = intent.admission.metadata;
		candidate.player_wallet = { m.lineage, economic_account_kind::wallet,
					    payload.native_money.player_wallet_mapping_id, 0 };
		candidate.native_wallet = { m.lineage, economic_account_kind::wallet,
					    payload.native_money.mobile_wallet_mapping_id,
					    ECONOMIC_NATIVE_MOBILE_WALLET_CONTEXT };
		const std::array<economic_sql_mapping_request, 2> mappings{
			{ { candidate.player_wallet, 1, payload.native_mobile.final_giver_pid },
			  { candidate.native_wallet, ECONOMIC_NATIVE_MOBILE_WALLET_LOCATOR,
			    payload.native_mobile.reference.mobile_instance_id } }
		};
		const auto code = economic_sql_lock_authority(
			connection, m.lineage, m.epoch, mappings, &candidate.original.authority);
		if (code)
			return code;
		native_mobile_wallet_origin origin;
		const auto origin_code = economic_sql_native_mobile_birth_observe_origin(
			connection, payload.native_mobile.reference, &origin);
		if (origin_code)
			return origin_code;
		if (origin.wallet_mapping_id != candidate.native_wallet.authority_id ||
		    origin.lineage.bytes != m.lineage.bytes)
			return EILSEQ;
		candidate.original.session_id = mysql_thread_id(connection);
		if (!native_money_session(connection, candidate.original.session_id))
			return ENOTCONN;
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
#endif
}

unsigned int
economic_sql_native_money_record(MYSQL *connection, const critical_command &command,
				 unsigned int result_code, bool mutation_applied,
				 const economic_sql_native_money_context &context) noexcept
{
#ifdef __NO_MYSQL__
	(void)connection;
	(void)command;
	(void)result_code;
	(void)mutation_applied;
	(void)context;
	return ENOTSUP;
#else
	try
	{
		if (!native_money_session(connection, context.original.session_id) ||
		    (result_code == 0) != mutation_applied)
			return EINVAL;
		item_transfer_payload payload{};
		economic_frozen_intent intent;
		std::vector<uint8_t> encoded;
		if (!native_money_command_bound(command, &payload, &intent) ||
		    critical_command_encode(command, &encoded) !=
			    critical_command_codec_result::ok ||
		    encoded != context.original.original_command ||
		    !context.original.acknowledged_save_revision ||
		    context.original.acknowledged_save_revision !=
			    payload.native_recovery.acknowledged_save_revision ||
		    intent.admission.metadata.lineage.bytes !=
			    context.original.authority.lineage.bytes ||
		    intent.admission.metadata.epoch.bytes !=
			    context.original.authority.epoch.bytes ||
		    context.original.authority.mappings.size() != 2)
			return EILSEQ;
		economic_accounting_plan plan;
		if (!native_money_expected_plan(command, payload, intent, &plan))
			return EILSEQ;
		std::array<uint8_t, ECONOMIC_ACCOUNT_KEY_BYTES> player{}, native{};
		if (economic_account_key_encode(context.player_wallet, &player) !=
			    economic_accounting_error::ok ||
		    economic_account_key_encode(context.native_wallet, &native) !=
			    economic_accounting_error::ok)
			return EILSEQ;
		bool player_locked = false, native_locked = false;
		for (const auto &mapping : context.original.authority.mappings)
		{
			std::array<uint8_t, ECONOMIC_ACCOUNT_KEY_BYTES> key{};
			if (economic_account_key_encode(mapping.request.account, &key) !=
			    economic_accounting_error::ok)
				return EILSEQ;
			player_locked |= key == player && mapping.request.locator_kind == 1 &&
					 mapping.request.native_id ==
						 payload.native_mobile.final_giver_pid;
			native_locked |= key == native &&
					 mapping.request.locator_kind ==
						 ECONOMIC_NATIVE_MOBILE_WALLET_LOCATOR &&
					 mapping.request.native_id ==
						 payload.native_mobile.reference.mobile_instance_id;
		}
		if (!player_locked || !native_locked ||
		    context.player_wallet.authority_id !=
			    payload.native_money.player_wallet_mapping_id ||
		    context.native_wallet.authority_id !=
			    payload.native_money.mobile_wallet_mapping_id ||
		    context.player_wallet.kind != economic_account_kind::wallet ||
		    context.player_wallet.context_id ||
		    context.player_wallet.lineage.bytes !=
			    intent.admission.metadata.lineage.bytes ||
		    context.native_wallet.kind != economic_account_kind::wallet ||
		    context.native_wallet.context_id != ECONOMIC_NATIVE_MOBILE_WALLET_CONTEXT ||
		    context.native_wallet.lineage.bytes != intent.admission.metadata.lineage.bytes)
			return EILSEQ;
		if (!mutation_applied)
			return insert_operation(connection, command, intent, nullptr, {},
						result_code) &&
					       native_money_operation_matches(connection, command,
									      plan, result_code) &&
					       native_money_session(connection,
								    context.original.session_id) ?
				       0 :
				       failure_code();
		if (economic_plan_encode(plan, &encoded) != economic_accounting_error::ok)
			return EILSEQ;
		if (!insert_operation(connection, command, intent, &plan, encoded, 0) ||
		    !native_money_financial_rows(connection, command.operation_id, plan, true) ||
		    !native_money_operation_matches(connection, command, plan, 0))
			return failure_code();
		return native_money_session(connection, context.original.session_id) ? 0 : ENOTCONN;
	}
	catch (const std::bad_alloc &)
	{
		return ENOMEM;
	}
	catch (...)
	{
		return EILSEQ;
	}
#endif
}

unsigned int
economic_sql_native_money_verify_retained(MYSQL *connection, const critical_command &command,
					  unsigned int result_code,
					  std::span<const uint8_t> result_payload) noexcept
{
#ifdef __NO_MYSQL__
	(void)connection;
	(void)command;
	(void)result_code;
	(void)result_payload;
	return ENOTSUP;
#else
	try
	{
		if (!connection)
			return EINVAL;
		const unsigned long session = mysql_thread_id(connection);
		if (!native_money_session(connection, session))
			return ENOTCONN;
		item_transfer_payload payload{};
		economic_frozen_intent intent;
		if (!native_money_command_bound(command, &payload, &intent))
			return EINVAL;
		item_native_mobile_money_result actual{}, expected{};
		if (!item_native_mobile_money_result_decode(result_payload, &actual) ||
		    !item_native_mobile_money_result_build(payload, &expected))
			return EILSEQ;
		if (result_code)
		{
			expected.player_wallet_revision =
				payload.native_money.projection.player_before_revision;
			expected.mobile_cash_revision =
				payload.native_money.projection.mobile_before_revision;
			expected.mobile_revision = payload.native_mobile.reference.mobile_revision;
		}
		if (actual != expected)
			return EILSEQ;
		std::vector<std::optional<std::string>> row;
		if (!read_operation(connection, command.operation_id, &row))
			return failure_code();
		uint64_t outcome = 0, code = 0, accounts = 0, postings = 0, children = 0,
			 events = 0, before = 0, after = 0;
		if (!row[0] || !parse_unsigned(*row[0], &outcome) || !row[1] ||
		    !parse_unsigned(*row[1], &code) || !row[5] ||
		    *row[5] != std::string(reinterpret_cast<const char *>(
						   command.accounting_intent.data()),
					   command.accounting_intent.size()) ||
		    !row[7] || !parse_unsigned(*row[7], &accounts) || !row[8] ||
		    !parse_unsigned(*row[8], &postings) || !row[9] ||
		    !parse_unsigned(*row[9], &children) || !row[10] ||
		    !parse_unsigned(*row[10], &events) || !row[11] ||
		    !parse_unsigned(*row[11], &before) || !row[12] ||
		    !parse_unsigned(*row[12], &after) || code != result_code ||
		    outcome != (result_code ? 2U : 1U) || children || events || before || after)
			return EILSEQ;
		economic_digest intent_hash{};
		if (economic_intent_digest(intent, &intent_hash) != economic_accounting_error::ok ||
		    !digest_matches(intent_hash, row[2]) ||
		    !digest_matches(intent.domain_digest, row[3]))
			return EILSEQ;
		economic_accounting_plan plan;
		if (!native_money_expected_plan(command, payload, intent, &plan))
			return EILSEQ;
		if (!native_money_operation_matches(connection, command, plan, result_code))
			return failure_code();
		const economic_account_key player{ plan.metadata.lineage,
						   economic_account_kind::wallet,
						   payload.native_money.player_wallet_mapping_id,
						   0 };
		const economic_account_key native{ plan.metadata.lineage,
						   economic_account_kind::wallet,
						   payload.native_money.mobile_wallet_mapping_id,
						   ECONOMIC_NATIVE_MOBILE_WALLET_CONTEXT };
		if (!native_money_historical_mapping(connection, player, 1,
						     payload.native_mobile.final_giver_pid) ||
		    !native_money_historical_mapping(
			    connection, native, ECONOMIC_NATIVE_MOBILE_WALLET_LOCATOR,
			    payload.native_mobile.reference.mobile_instance_id))
			return failure_code();
		native_mobile_wallet_origin origin;
		const auto origin_code = economic_sql_native_mobile_birth_observe_origin(
			connection, payload.native_mobile.reference, &origin);
		if (origin_code)
			return origin_code;
		if (origin.wallet_mapping_id != native.authority_id ||
		    origin.lineage.bytes != native.lineage.bytes)
			return EILSEQ;
		if (result_code)
		{
			if (row[4] || row[6] || accounts || postings)
				return EILSEQ;
			plan.accounts.clear();
			plan.postings.clear();
			if (!native_money_financial_rows(connection, command.operation_id, plan,
							 false))
				return failure_code();
			return native_money_session(connection, session) ? 0 : ENOTCONN;
		}
		if (accounts != plan.accounts.size() || postings != plan.postings.size())
			return EILSEQ;
		std::vector<uint8_t> canonical;
		economic_digest plan_hash{};
		if (economic_plan_encode(plan, &canonical) != economic_accounting_error::ok ||
		    economic_plan_digest(plan, &plan_hash) != economic_accounting_error::ok ||
		    !digest_matches(plan_hash, row[4]) || !row[6] ||
		    *row[6] != std::string(reinterpret_cast<const char *>(canonical.data()),
					   canonical.size()))
			return EILSEQ;
		if (!native_money_financial_rows(connection, command.operation_id, plan, false))
			return failure_code();
		return native_money_session(connection, session) ? 0 : ENOTCONN;
	}
	catch (const std::bad_alloc &)
	{
		return ENOMEM;
	}
	catch (...)
	{
		return EILSEQ;
	}
#endif
}

unsigned int economic_sql_native_money_lock_publication(
	MYSQL *connection, const critical_command &command, const critical_completion &completion,
	std::span<const player_item_snapshot> original_native_before,
	std::span<const player_item_snapshot> original_player_before,
	std::span<const player_item_snapshot> original_player_after,
	uint64_t acknowledged_save_revision, economic_sql_native_money_publication *output) noexcept
{
#ifdef __NO_MYSQL__
	(void)connection;
	(void)command;
	(void)completion;
	(void)original_native_before;
	(void)original_player_before;
	(void)original_player_after;
	(void)acknowledged_save_revision;
	(void)output;
	return ENOTSUP;
#else
	if (!connection || !output)
		return EINVAL;
	try
	{
		const unsigned long session = mysql_thread_id(connection);
		native_publication_session(connection, session);
		item_transfer_payload payload{};
		economic_frozen_intent intent;
		native_publication_require(native_money_command_bound(command, &payload, &intent),
					   EPROTONOSUPPORT);
		native_publication_require(
			completion.operation_id.bytes == command.operation_id.bytes &&
				completion.disposition ==
					critical_completion_disposition::execution &&
				completion.failure_stage == critical_failure_stage::none,
			EILSEQ);
		const bool rejected = completion.outcome ==
				      critical_apply_outcome::terminal_failure;
		native_publication_require(
			rejected ? completion.error_code != 0 :
				   !completion.error_code &&
					   (completion.outcome == critical_apply_outcome::applied ||
					    completion.outcome ==
						    critical_apply_outcome::already_applied),
			EILSEQ);
		item_native_mobile_money_result actual{}, expected{};
		std::array<uint8_t, ITEM_TRANSFER_NATIVE_MOBILE_MONEY_RESULT_BYTES> encoded{};
		native_publication_require(
			completion.result_size == encoded.size() &&
				item_native_mobile_money_result_decode(
					{ completion.result_payload.data(),
					  completion.result_size },
					&actual) &&
				item_native_mobile_money_result_encode(actual, &encoded) &&
				std::equal(encoded.begin(), encoded.end(),
					   completion.result_payload.begin()) &&
				std::all_of(completion.result_payload.begin() + encoded.size(),
					    completion.result_payload.end(),
					    [](uint8_t v) { return !v; }) &&
				item_native_mobile_money_result_build(payload, &expected),
			EILSEQ);
		if (rejected)
		{
			expected.player_wallet_revision =
				payload.native_money.projection.player_before_revision;
			expected.mobile_cash_revision =
				payload.native_money.projection.mobile_before_revision;
			expected.mobile_revision = payload.native_mobile.reference.mobile_revision;
		}
		native_publication_require(
			actual == expected && completion.durable_revision ==
						      std::max({ actual.player_wallet_revision,
								 actual.mobile_cash_revision,
								 actual.mobile_revision,
								 actual.player_custody_revision,
								 actual.stock_revision }),
			EILSEQ);
		native_publication_require(
			acknowledged_save_revision &&
				acknowledged_save_revision ==
					payload.native_recovery.acknowledged_save_revision,
			EILSEQ);
		const auto player_before_bytes = native_publication_forest(original_player_before);
		const auto player_after_bytes = native_publication_forest(original_player_after);
		const auto native_before_bytes = native_publication_forest(original_native_before);
		native_publication_require(
			player_before_bytes == player_after_bytes &&
				shop_trade_recovery_forest_verify(
					player_before_bytes,
					shop_trade_recovery_forest_role::player_before,
					payload.native_recovery.player_before) &&
				shop_trade_recovery_forest_verify(
					player_after_bytes,
					shop_trade_recovery_forest_role::player_after,
					payload.native_recovery.player_after),
			EILSEQ);
		// Original inbox/root/outbox authority BEFORE any current participant lock.
		// Shared repository owner supplies this accessor; no standalone permit.
		const auto receipt_code = economic_sql_native_money_verify_receipt_in_transaction(
			connection, command, completion);
		native_publication_require(!receipt_code, receipt_code);
		economic_sql_native_money_publication candidate;
		quest_mobile_native_sql_row native;
		const auto native_code = quest_mobile_native_sql_lock(
			connection, payload.native_mobile.reference.mobile_instance_id, &native);
		native_publication_require(!native_code, static_cast<unsigned int>(native_code));
		native_publication_require(native.present && native.original_session == session &&
						   native.image.state ==
							   quest_mobile_lifetime_state::live &&
						   native.image.cash.has_value(),
					   ENODATA);
		const auto origin_code = economic_sql_native_mobile_birth_observe_origin(
			connection, payload.native_mobile.reference, &candidate.origin);
		native_publication_require(!origin_code, origin_code);
		native_publication_require(
			candidate.origin.wallet_mapping_id ==
					payload.native_money.mobile_wallet_mapping_id &&
				candidate.origin.lineage.bytes ==
					intent.admission.metadata.lineage.bytes,
			EILSEQ);
		auto reference = payload.native_mobile.reference;
		if (!rejected)
		{
			++reference.mobile_revision;
			native_publication_require(native.image.last_transition_operation.bytes ==
							   command.operation_id.bytes,
						   ESTALE);
		}
		std::array<uint8_t, QUEST_MOBILE_NATIVE_REFERENCE_BYTES> ref{}, current{};
		native_publication_codec(quest_mobile_native_reference_encode(reference, &ref));
		native_publication_codec(
			quest_mobile_native_reference_encode(native.image.reference, &current));
		const auto &money = payload.native_money.projection;
		native_publication_require(
			ref == current &&
				native_publication_forest(native.image.items) ==
					native_before_bytes &&
				native.image.cash->revision ==
					(rejected ? money.mobile_before_revision :
						    money.mobile_after_revision) &&
				native.image.cash->denominations.amount ==
					(rejected ? money.mobile_before : money.mobile_after),
			ESTALE);
		native_publication_sql(
			execute(connection,
				"SELECT save_revision,wallet_revision,copper,silver,gold,platinum "
				"FROM player_data WHERE pid=" +
					std::to_string(payload.native_recovery.player_pid) +
					" LIMIT 2 FOR UPDATE"));
		std::unique_ptr<MYSQL_RES, decltype(&mysql_free_result)> rows(
			mysql_store_result(connection), mysql_free_result);
		native_publication_require(rows && mysql_num_fields(rows.get()) == 6 &&
						   mysql_num_rows(rows.get()) == 1,
					   EILSEQ);
		MYSQL_ROW row = mysql_fetch_row(rows.get());
		const auto *lengths = mysql_fetch_lengths(rows.get());
		native_publication_require(row && lengths, EILSEQ);
		uint64_t values[6]{};
		for (size_t i = 0; i < 6; ++i)
			native_publication_require(
				row[i] &&
					parse_unsigned(std::string(row[i], lengths[i]), &values[i]),
				EILSEQ);
		native_publication_require(
			values[0] == acknowledged_save_revision &&
				values[1] == (rejected ? money.player_before_revision :
							 money.player_after_revision),
			ESTALE);
		for (size_t i = 0; i < 4; ++i)
		{
			native_publication_require(values[i + 2] <= INT_MAX, EILSEQ);
			candidate.player_cash.amount[i] = static_cast<int64_t>(values[i + 2]);
		}
		native_publication_require(candidate.player_cash.amount ==
						   (rejected ? money.player_before :
							       money.player_after),
					   ESTALE);
		candidate.player_wallet_revision = values[1];
		const item_owner_identity player{ item_owner_type::player,
						  payload.native_recovery.player_pid, 0 };
		const item_owner_identity native_owner{ item_owner_type::native_mobile,
							reference.mobile_instance_id, 0 };
		std::array<item_owner_identity, 2> owners{ player, native_owner };
		std::sort(owners.begin(), owners.end(),
			  [](const auto &a, const auto &b) {
				  return std::tie(a.type, a.id, a.context_id) <
					 std::tie(b.type, b.id, b.context_id);
			  });
		uint64_t player_revision = 0, native_revision = 0;
		for (const auto &owner : owners)
		{
			uint64_t revision = 0;
			native_publication_sql(
				item_transfer_repository_lock_owner(connection, owner, &revision));
			if (item_owner_identity_equal(owner, player))
				player_revision = revision;
			else
				native_revision = revision;
		}
		native_publication_require(player_revision == payload.expected_from_revision &&
						   native_revision ==
							   payload.expected_to_revision &&
						   native_revision == reference.stock_revision,
					   ESTALE);
		std::vector<item_native_quest_publication_custody> cut;
		native_publication_sql(item_transfer_repository_lock_native_publication_custody(
			connection, payload, original_native_before, original_player_before,
			original_player_after, &cut));
		std::map<uint64_t, item_native_quest_publication_custody> expected_cut;
		native_publication_add_forest(original_native_before, native_owner, cut,
					      &expected_cut);
		native_publication_add_forest(original_player_before, player, cut, &expected_cut);
		native_publication_require(expected_cut.size() == cut.size(), ESTALE);
		for (const auto &entry : cut)
		{
			const auto found = expected_cut.find(entry.snapshot.uid);
			native_publication_require(found != expected_cut.end() &&
							   found->second.vnum == entry.vnum &&
							   native_publication_position_equal(
								   found->second.snapshot.position,
								   entry.snapshot.position),
						   ESTALE);
		}
		shop_item_runtime_image player_image;
		native_publication_sql(shop_item_runtime_lock_player_image(
			connection, payload.native_recovery.player_pid, original_player_before,
			&player_image));
		for (const auto &entry : cut)
			native_publication_absence(
				connection, entry.snapshot.uid,
				item_owner_identity_equal(entry.snapshot.position.owner, player));
		candidate.original.session_id = session;
		candidate.original.native = std::move(native.image);
		candidate.original.from_owner_revision = player_revision;
		candidate.original.to_owner_revision = native_revision;
		candidate.original.player_owner_revision = player_revision;
		candidate.original.acknowledged_save_revision = acknowledged_save_revision;
		for (const auto &entry : cut)
		{
			const auto &p = entry.snapshot.position;
			candidate.original.custody.push_back(
				{ entry.snapshot.uid, p.root_uid, p.parent_uid, p.owner, p.revision,
				  item_owner_identity_equal(p.owner, player) ? player_revision :
									       native_revision,
				  entry.vnum, p.state });
		}
		native_publication_session(connection, session);
		*output = std::move(candidate);
		return 0;
	}
	catch (unsigned int code)
	{
		return code ? code : EILSEQ;
	}
	catch (const std::bad_alloc &)
	{
		return ENOMEM;
	}
	catch (...)
	{
		return EILSEQ;
	}
#endif
}

unsigned int economic_sql_native_fee_lock_publication(
	MYSQL *connection, const critical_command &command, const critical_completion &completion,
	std::span<const player_item_snapshot> original_native_before,
	std::span<const player_item_snapshot> original_player_before,
	std::span<const player_item_snapshot> original_player_after,
	uint64_t acknowledged_save_revision, economic_sql_native_fee_publication *output) noexcept
{
#ifdef __NO_MYSQL__
	(void)connection;
	(void)command;
	(void)completion;
	(void)original_native_before;
	(void)original_player_before;
	(void)original_player_after;
	(void)acknowledged_save_revision;
	(void)output;
	return ENOTSUP;
#else
	if (!connection || !output)
		return EINVAL;
	try
	{
		const unsigned long session = mysql_thread_id(connection);
		native_publication_session(connection, session);
		item_transfer_payload payload{};
		economic_frozen_intent intent;
		native_publication_require(native_fee_command_bound(command, &payload, &intent),
					   EPROTONOSUPPORT);
		native_publication_require(
			completion.operation_id.bytes == command.operation_id.bytes &&
				completion.disposition ==
					critical_completion_disposition::execution &&
				completion.failure_stage == critical_failure_stage::none,
			EILSEQ);
		const bool rejected = completion.outcome ==
				      critical_apply_outcome::terminal_failure;
		native_publication_require(
			rejected ? completion.error_code != 0 :
				   !completion.error_code &&
					   (completion.outcome == critical_apply_outcome::applied ||
					    completion.outcome ==
						    critical_apply_outcome::already_applied),
			EILSEQ);
		item_native_mobile_fee_result actual{}, expected{};
		std::array<uint8_t, ITEM_TRANSFER_NATIVE_MOBILE_FEE_RESULT_BYTES> encoded{};
		native_publication_require(
			completion.result_size == encoded.size() &&
				item_native_mobile_fee_result_decode(
					{ completion.result_payload.data(),
					  completion.result_size },
					&actual) &&
				item_native_mobile_fee_result_encode(actual, &encoded) &&
				std::equal(encoded.begin(), encoded.end(),
					   completion.result_payload.begin()) &&
				std::all_of(completion.result_payload.begin() + encoded.size(),
					    completion.result_payload.end(),
					    [](uint8_t v) { return !v; }) &&
				item_native_mobile_fee_result_build(payload, &expected),
			EILSEQ);
		if (rejected)
		{
			expected.mobile_cash_revision =
				payload.native_cost.projection.before_revision;
			expected.mobile_revision = payload.native_mobile.reference.mobile_revision;
		}
		native_publication_require(
			actual == expected && completion.durable_revision ==
						      std::max({ actual.native_custody_revision,
								 actual.mobile_cash_revision,
								 actual.mobile_revision,
								 actual.player_custody_revision,
								 actual.stock_revision }),
			EILSEQ);
		native_publication_require(
			acknowledged_save_revision &&
				acknowledged_save_revision ==
					payload.native_recovery.acknowledged_save_revision,
			EILSEQ);
		const auto player_before_bytes = native_publication_forest(original_player_before);
		const auto player_after_bytes = native_publication_forest(original_player_after);
		const auto native_before_bytes = native_publication_forest(original_native_before);
		native_publication_require(
			player_before_bytes == player_after_bytes &&
				shop_trade_recovery_forest_verify(
					player_before_bytes,
					shop_trade_recovery_forest_role::player_before,
					payload.native_recovery.player_before) &&
				shop_trade_recovery_forest_verify(
					player_after_bytes,
					shop_trade_recovery_forest_role::player_after,
					payload.native_recovery.player_after),
			EILSEQ);
		// Original inbox/root/outbox authority BEFORE any current participant lock.
		// Shared repository owner supplies this accessor; no standalone permit.
		const auto receipt_code = economic_sql_native_fee_verify_receipt_in_transaction(
			connection, command, completion);
		native_publication_require(!receipt_code, receipt_code);
		economic_sql_native_fee_publication candidate;
		quest_mobile_native_sql_row native;
		const auto native_code = quest_mobile_native_sql_lock(
			connection, payload.native_mobile.reference.mobile_instance_id, &native);
		native_publication_require(!native_code, static_cast<unsigned int>(native_code));
		native_publication_require(native.present && native.original_session == session &&
						   native.image.state ==
							   quest_mobile_lifetime_state::live &&
						   native.image.cash.has_value(),
					   ENODATA);
		const auto origin_code = economic_sql_native_mobile_birth_observe_origin(
			connection, payload.native_mobile.reference, &candidate.origin);
		native_publication_require(!origin_code, origin_code);
		native_publication_require(candidate.origin.wallet_mapping_id ==
							   payload.native_cost.wallet_mapping_id &&
						   candidate.origin.lineage.bytes ==
							   intent.admission.metadata.lineage.bytes,
					   EILSEQ);
		auto reference = payload.native_mobile.reference;
		if (!rejected)
		{
			++reference.mobile_revision;
			native_publication_require(native.image.last_transition_operation.bytes ==
							   command.operation_id.bytes,
						   ESTALE);
		}
		std::array<uint8_t, QUEST_MOBILE_NATIVE_REFERENCE_BYTES> ref{}, current{};
		native_publication_codec(quest_mobile_native_reference_encode(reference, &ref));
		native_publication_codec(
			quest_mobile_native_reference_encode(native.image.reference, &current));
		const auto &fee = payload.native_cost.projection;
		native_publication_require(ref == current &&
						   native_publication_forest(native.image.items) ==
							   native_before_bytes &&
						   native.image.cash->revision ==
							   (rejected ? fee.before_revision :
								       fee.after_revision) &&
						   native.image.cash->denominations.amount ==
							   (rejected ? fee.before : fee.after),
					   ESTALE);
		native_publication_sql(execute(
			connection, "SELECT save_revision FROM player_data WHERE pid=" +
					    std::to_string(payload.native_recovery.player_pid) +
					    " LIMIT 2 FOR UPDATE"));
		std::unique_ptr<MYSQL_RES, decltype(&mysql_free_result)> rows(
			mysql_store_result(connection), mysql_free_result);
		native_publication_require(rows && mysql_num_fields(rows.get()) == 1 &&
						   mysql_num_rows(rows.get()) == 1,
					   EILSEQ);
		MYSQL_ROW row = mysql_fetch_row(rows.get());
		const auto *lengths = mysql_fetch_lengths(rows.get());
		uint64_t save_revision = 0;
		native_publication_require(row && lengths && row[0] &&
						   parse_unsigned(std::string(row[0], lengths[0]),
								  &save_revision),
					   EILSEQ);
		native_publication_require(save_revision == acknowledged_save_revision, ESTALE);
		const item_owner_identity player{ item_owner_type::player,
						  payload.native_recovery.player_pid, 0 };
		const item_owner_identity native_owner{ item_owner_type::native_mobile,
							reference.mobile_instance_id, 0 };
		std::array<item_owner_identity, 2> owners{ player, native_owner };
		std::sort(owners.begin(), owners.end(),
			  [](const auto &a, const auto &b) {
				  return std::tie(a.type, a.id, a.context_id) <
					 std::tie(b.type, b.id, b.context_id);
			  });
		uint64_t player_revision = 0, native_revision = 0;
		for (const auto &owner : owners)
		{
			uint64_t revision = 0;
			native_publication_sql(
				item_transfer_repository_lock_owner(connection, owner, &revision));
			if (item_owner_identity_equal(owner, player))
				player_revision = revision;
			else
				native_revision = revision;
		}
		native_publication_require(player_revision == payload.expected_to_revision &&
						   native_revision ==
							   payload.expected_from_revision &&
						   native_revision == reference.stock_revision,
					   ESTALE);
		std::vector<item_native_quest_publication_custody> cut;
		native_publication_sql(item_transfer_repository_lock_native_publication_custody(
			connection, payload, original_native_before, original_player_before,
			original_player_after, &cut));
		std::map<uint64_t, item_native_quest_publication_custody> expected_cut;
		native_publication_add_forest(original_native_before, native_owner, cut,
					      &expected_cut);
		native_publication_add_forest(original_player_before, player, cut, &expected_cut);
		native_publication_require(expected_cut.size() == cut.size(), ESTALE);
		for (const auto &entry : cut)
		{
			const auto found = expected_cut.find(entry.snapshot.uid);
			native_publication_require(found != expected_cut.end() &&
							   found->second.vnum == entry.vnum &&
							   native_publication_position_equal(
								   found->second.snapshot.position,
								   entry.snapshot.position),
						   ESTALE);
		}
		shop_item_runtime_image player_image;
		native_publication_sql(shop_item_runtime_lock_player_image(
			connection, payload.native_recovery.player_pid, original_player_before,
			&player_image));
		for (const auto &entry : cut)
			native_publication_absence(
				connection, entry.snapshot.uid,
				item_owner_identity_equal(entry.snapshot.position.owner, player));
		candidate.original.session_id = session;
		candidate.original.native = std::move(native.image);
		candidate.original.from_owner_revision = native_revision;
		candidate.original.to_owner_revision = player_revision;
		candidate.original.player_owner_revision = player_revision;
		candidate.original.acknowledged_save_revision = acknowledged_save_revision;
		for (const auto &entry : cut)
		{
			const auto &p = entry.snapshot.position;
			candidate.original.custody.push_back(
				{ entry.snapshot.uid, p.root_uid, p.parent_uid, p.owner, p.revision,
				  item_owner_identity_equal(p.owner, player) ? player_revision :
									       native_revision,
				  entry.vnum, p.state });
		}
		native_publication_session(connection, session);
		*output = std::move(candidate);
		return 0;
	}
	catch (unsigned int code)
	{
		return code ? code : EILSEQ;
	}
	catch (const std::bad_alloc &)
	{
		return ENOMEM;
	}
	catch (...)
	{
		return EILSEQ;
	}
#endif
}

unsigned int
economic_sql_held_retirement_lock_saved_revision(MYSQL *connection, const critical_command &command,
						 const held_retirement_recovery &captured,
						 uint64_t *output) noexcept
{
#ifdef __NO_MYSQL__
	(void)connection;
	(void)command;
	(void)captured;
	(void)output;
	return ENOTSUP;
#else
	if (!connection || !output)
		return EINVAL;
	try
	{
		const unsigned long session = mysql_thread_id(connection);
		native_publication_session(connection, session);
		item_transfer_payload payload{};
		std::vector<uint8_t> original;
		native_publication_require(
			held_retirement_transport_command(command) &&
				held_retirement_command_identity(command, &payload) &&
				held_retirement_recovery_encode(command, captured, &original),
			EILSEQ);
		const uint64_t revision =
			native_publication_save_revision(connection, payload.from_owner.id);
		native_publication_require(revision == captured.save_revision, ESTALE);
		native_publication_session(connection, session);
		*output = revision;
		return 0;
	}
	catch (unsigned int code)
	{
		return code ? code : EILSEQ;
	}
	catch (const std::bad_alloc &)
	{
		return ENOMEM;
	}
#endif
}

unsigned int economic_sql_held_retirement_lock_selected_domains(
	MYSQL *connection, const critical_command &command,
	const held_retirement_recovery &captured, bool original_after) noexcept
{
#ifdef __NO_MYSQL__
	(void)connection;
	(void)command;
	(void)captured;
	(void)original_after;
	return ENOTSUP;
#else
	if (!connection)
		return EINVAL;
	try
	{
		const unsigned long session = mysql_thread_id(connection);
		native_publication_session(connection, session);
		item_transfer_payload payload{};
		std::vector<uint8_t> original;
		native_publication_require(
			held_retirement_transport_command(command) &&
				held_retirement_command_identity(command, &payload) &&
				held_retirement_recovery_encode(command, captured, &original),
			EILSEQ);
		native_publication_absence(connection, payload.selected_item_uid, !original_after);
		native_publication_session(connection, session);
		return 0;
	}
	catch (unsigned int code)
	{
		return code ? code : EILSEQ;
	}
	catch (const std::bad_alloc &)
	{
		return ENOMEM;
	}
#endif
}

unsigned int economic_sql_held_retirement_lock_source_custody(
	MYSQL *connection, const critical_command &command, uint64_t from_revision,
	uint64_t to_revision, economic_sql_item_transfer_context *context) noexcept
{
#ifdef __NO_MYSQL__
	(void)connection;
	(void)command;
	(void)from_revision;
	(void)to_revision;
	(void)context;
	return ENOTSUP;
#else
	if (!connection || !context || !context->held_retirement)
		return EINVAL;
	try
	{
		native_publication_session(connection, context->session_id);
		item_transfer_payload payload{};
		std::vector<uint8_t> encoded, captured;
		native_publication_require(
			held_retirement_transport_command(command) &&
				held_retirement_command_identity(command, &payload) &&
				critical_command_encode(command, &encoded) ==
					critical_command_codec_result::ok &&
				encoded == context->original_held_command &&
				held_retirement_recovery_encode(command, *context->held_retirement,
								&captured) &&
				!context->held_retirement->receipt.present &&
				!context->held_retirement->physical_stage,
			EILSEQ);
		native_publication_require(from_revision == payload.expected_from_revision &&
						   to_revision == payload.expected_to_revision &&
						   from_revision != UINT64_MAX &&
						   to_revision != UINT64_MAX,
					   ESTALE);
		// Caller already ensured and locked these exact owner rows. Renew in the
		// same canonical order, never create a missing destruction owner here.
		std::array<item_owner_identity, 2> owners{ payload.from_owner, payload.to_owner };
		std::sort(owners.begin(), owners.end(),
			  [](const auto &a, const auto &b) {
				  return std::tie(a.type, a.id, a.context_id) <
					 std::tie(b.type, b.id, b.context_id);
			  });
		for (const auto &owner : owners)
		{
			uint64_t observed = 0;
			native_publication_sql(
				item_transfer_repository_lock_owner(connection, owner, &observed));
			native_publication_require(
				observed == (item_owner_identity_equal(owner, payload.from_owner) ?
						     from_revision :
						     to_revision),
				ESTALE);
		}
		std::vector<item_native_quest_publication_custody> cut;
		native_publication_sql(
			item_transfer_repository_lock_held_retirement_publication_custody(
				connection, command, *context->held_retirement, &cut));
		std::map<uint64_t, item_native_quest_publication_custody> expected;
		native_publication_add_forest(context->held_retirement->before, payload.from_owner,
					      cut, &expected);
		native_publication_require(expected.size() == cut.size(), ESTALE);
		const auto selected = expected.find(payload.selected_item_uid);
		native_publication_require(
			selected != expected.end() &&
				selected->second.snapshot.position.revision ==
					payload.items[0].expected_item_revision &&
				selected->second.snapshot.position.equipment_slot == HOLD + 1 &&
				selected->second.vnum == payload.items[0].vnum,
			ESTALE);
		// Every custody row was locked above before any physical read. The exact
		// original post-wear literal forest cannot be replaced by today's graph.
		shop_item_runtime_image image;
		native_publication_sql(shop_item_runtime_lock_player_image(
			connection, payload.from_owner.id, context->held_retirement->before,
			&image));
		for (const auto &row : cut)
			native_publication_absence(connection, row.snapshot.uid, true);
		native_publication_session(connection, context->session_id);
		context->held_source_custody_locked = true;
		return 0;
	}
	catch (unsigned int code)
	{
		return code ? code : EILSEQ;
	}
	catch (const std::bad_alloc &)
	{
		return ENOMEM;
	}
#endif
}

unsigned int economic_sql_held_retirement_source_custody_hook(MYSQL *connection,
							      const critical_command &command,
							      uint64_t from_revision,
							      uint64_t to_revision,
							      void *original_owner_context) noexcept
{
	return economic_sql_held_retirement_lock_source_custody(
		connection, command, from_revision, to_revision,
		static_cast<economic_sql_item_transfer_context *>(original_owner_context));
}

#ifndef __NO_MYSQL__
namespace
{
// Original frozen projection is the only plan input. No current balance or
// receipt-only inference supplies an original charge or requirement outcome.
bool native_fee_expected_plan(const critical_command &command, const item_transfer_payload &payload,
			      const economic_frozen_intent &intent,
			      economic_accounting_plan *output)
{
	economic_accounting_plan plan;
	if (economic_intent_plan_metadata(command, intent, &plan.metadata) !=
	    economic_accounting_error::ok)
		return false;
	const economic_account_key wallet{ plan.metadata.lineage, economic_account_kind::wallet,
					   payload.native_cost.wallet_mapping_id,
					   ECONOMIC_NATIVE_MOBILE_WALLET_CONTEXT };
	if (item_native_mobile_cost_accounting_effects(payload, wallet, &plan) !=
		    economic_accounting_error::ok ||
	    economic_plan_normalize(&plan) != economic_accounting_error::ok ||
	    economic_plan_validate_structure(plan) != economic_accounting_error::ok ||
	    !plan.children.empty() || !plan.item_events.empty() || !plan.items_before.empty() ||
	    !plan.items_after.empty())
		return false;
	*output = std::move(plan);
	return true;
}
bool native_fee_financial_rows(MYSQL *connection, const critical_operation_id &root,
			       const economic_accounting_plan &plan, bool append, bool applied)
{
	for (size_t i = 0; i < plan.accounts.size(); ++i)
	{
		const auto &a = plan.accounts[i];
		std::array<uint8_t, ECONOMIC_ACCOUNT_KEY_BYTES> key{};
		if (economic_account_key_encode(a.key, &key) != economic_accounting_error::ok)
		{
			errno = EILSEQ;
			return false;
		}
		native_money_fields row{ { "operation_id", id(root) },
					 { "account_index", std::to_string(i) },
					 { "account_key", hex(key) } };
		native_money_coins(&row, "before_", a.before);
		native_money_coins(&row, "after_", a.after);
		row.emplace_back("before_revision", std::to_string(a.before_revision));
		row.emplace_back("after_revision", std::to_string(a.after_revision));
		if (!native_money_rows(connection, "economic_accounting_account_effect", row,
				       append))
			return false;
	}
	for (size_t i = 0; i < plan.postings.size(); ++i)
	{
		const auto &a = plan.postings[i];
		native_money_fields row{ { "operation_id", id(root) },
					 { "line_index", std::to_string(i) },
					 { "event_index", std::to_string(a.event_index) },
					 { "account_index", std::to_string(a.account_index) },
					 { "child_index", std::to_string(a.child_index) },
					 { "copper_value", std::to_string(a.copper) } };
		native_money_coins(&row, "delta_", a.delta);
		if (!native_money_rows(connection, "economic_accounting_coin_posting", row, append))
			return false;
	}
	const std::string where = "operation_id=" + id(root);
	return native_money_count(connection, "economic_accounting_account_effect", where,
				  plan.accounts.size()) &&
	       native_money_count(connection, "economic_accounting_coin_posting", where,
				  plan.postings.size()) &&
	       native_money_count(connection, "economic_accounting_child", where, 0) &&
	       native_money_count(connection, "economic_accounting_item_reference", where, 0) &&
	       native_money_count(connection, "economic_accounting_source_claim", where,
				  applied ? 1 : 0) &&
	       native_money_count(connection, "item_ownership_ledger", where, 0);
}
bool native_fee_operation_matches(MYSQL *connection, const critical_command &command,
				  const economic_accounting_plan &plan, unsigned int result_code)
{
	const auto &m = plan.metadata;
	std::array<uint8_t, ECONOMIC_SOURCE_EVENT_BYTES> source{};
	if (!m.source_event ||
	    economic_source_event_encode(*m.source_event, &source) != economic_accounting_error::ok)
	{
		errno = EILSEQ;
		return false;
	}
	std::vector<uint8_t> encoded;
	economic_digest digest{};
	if (!result_code &&
	    (economic_plan_encode(plan, &encoded) != economic_accounting_error::ok ||
	     economic_plan_digest(plan, &digest) != economic_accounting_error::ok))
	{
		errno = EILSEQ;
		return false;
	}
	native_money_fields fields{
		{ "operation_id", id(m.operation_id) },
		{ "lineage", id(m.lineage) },
		{ "epoch", id(m.epoch) },
		{ "original_operation_id", optional_id(m.original_operation_id) },
		{ "accounting_version", std::to_string(m.version) },
		{ "writer_id", std::to_string(m.writer_id) },
		{ "policy_version", std::to_string(m.policy_version) },
		{ "compiler_version", std::to_string(m.compiler_version) },
		{ "actor_kind", std::to_string(static_cast<uint8_t>(m.actor_kind)) },
		{ "actor_id", std::to_string(m.actor_id) },
		{ "reason", std::to_string(static_cast<uint16_t>(m.reason)) },
		{ "source_event", hex(source) },
		{ "intent_digest", hex(m.intent_digest) },
		{ "domain_digest", hex(m.domain_digest) },
		{ "plan_digest", result_code ? "NULL" : hex(digest) },
		{ "canonical_intent", hex(command.accounting_intent) },
		{ "canonical_plan", result_code ? "NULL" : hex(encoded) },
		{ "outcome", result_code ? "2" : "1" },
		{ "result_code", std::to_string(result_code) },
		{ "realized_price_copper", "NULL" },
		{ "account_count", std::to_string(result_code ? 0 : plan.accounts.size()) },
		{ "posting_count", std::to_string(result_code ? 0 : plan.postings.size()) },
		{ "child_count", "0" },
		{ "item_event_count", "0" },
		{ "before_witness_count", "0" },
		{ "after_witness_count", "0" }
	};
	return native_money_count(connection, "economic_accounting_operation",
				  native_money_predicate(fields), 1);
}

bool native_fee_source_matches(MYSQL *connection, const economic_operation_metadata &m,
			       bool applied)
{
	if (!native_money_count(connection, "economic_accounting_source_claim",
				"operation_id=" + id(m.operation_id), applied ? 1 : 0))
		return false;
	if (!applied)
		return true;
	std::array<uint8_t, ECONOMIC_SOURCE_EVENT_BYTES> encoded{};
	if (!m.source_event || economic_source_event_encode(*m.source_event, &encoded) !=
				       economic_accounting_error::ok)
	{
		errno = EILSEQ;
		return false;
	}
	return native_money_count(connection, "economic_accounting_source_claim",
				  "operation_id=" + id(m.operation_id) +
					  " AND lineage=" + id(m.lineage) +
					  " AND source_event=" + hex(encoded) + " AND outcome=1",
				  1);
}
}
#endif

unsigned int economic_sql_native_fee_lock(MYSQL *connection, const critical_command &command,
					  economic_sql_native_fee_context *output) noexcept
{
#ifdef __NO_MYSQL__
	(void)connection;
	(void)command;
	(void)output;
	return ENOTSUP;
#else
	try
	{
		if (!connection || !output)
			return EINVAL;
		const unsigned long session = mysql_thread_id(connection);
		if (!native_money_session(connection, session))
			return ENOTCONN;
		item_transfer_payload payload{};
		economic_frozen_intent intent;
		if (!native_fee_command_bound(command, &payload, &intent))
			return EINVAL;
		economic_sql_native_fee_context candidate;
		player_native_quest_checkpoint_stage held;
		if (!player_save_native_quest_checkpoint_owner::original_held_bodies(
			    command, &candidate.original.original_player_before,
			    &candidate.original.original_player_after, &held))
			return ENODATA;
		candidate.original.acknowledged_save_revision = held.save_revision;
		std::vector<uint8_t> before, after;
		if (!held.save_revision ||
		    held.save_revision != payload.native_recovery.acknowledged_save_revision ||
		    player_item_snapshot_list_encode(candidate.original.original_player_before,
						     &before) != player_snapshot_codec_result::ok ||
		    player_item_snapshot_list_encode(candidate.original.original_player_after,
						     &after) != player_snapshot_codec_result::ok ||
		    before != after ||
		    !shop_trade_recovery_forest_verify(
			    before, shop_trade_recovery_forest_role::player_before,
			    payload.native_recovery.player_before) ||
		    !shop_trade_recovery_forest_verify(
			    after, shop_trade_recovery_forest_role::player_after,
			    payload.native_recovery.player_after) ||
		    critical_command_encode(command, &candidate.original.original_command) !=
			    critical_command_codec_result::ok)
			return EILSEQ;
		const auto &m = intent.admission.metadata;
		candidate.native_wallet = { m.lineage, economic_account_kind::wallet,
					    payload.native_cost.wallet_mapping_id,
					    ECONOMIC_NATIVE_MOBILE_WALLET_CONTEXT };
		const std::array<economic_sql_mapping_request, 1> mappings{
			{ { candidate.native_wallet, ECONOMIC_NATIVE_MOBILE_WALLET_LOCATOR,
			    payload.native_mobile.reference.mobile_instance_id } }
		};
		const auto code = economic_sql_lock_authority(
			connection, m.lineage, m.epoch, mappings, &candidate.original.authority);
		if (code)
			return code;
		native_mobile_wallet_origin origin;
		const auto origin_code = economic_sql_native_mobile_birth_observe_origin(
			connection, payload.native_mobile.reference, &origin);
		if (origin_code)
			return origin_code;
		if (origin.wallet_mapping_id != candidate.native_wallet.authority_id ||
		    origin.lineage.bytes != m.lineage.bytes)
			return EILSEQ;
		candidate.original.session_id = session;
		if (!native_money_session(connection, candidate.original.session_id))
			return ENOTCONN;
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
#endif
}

unsigned int economic_sql_native_fee_record(MYSQL *connection, const critical_command &command,
					    unsigned int result_code, bool mutation_applied,
					    const economic_sql_native_fee_context &context) noexcept
{
#ifdef __NO_MYSQL__
	(void)connection;
	(void)command;
	(void)result_code;
	(void)mutation_applied;
	(void)context;
	return ENOTSUP;
#else
	try
	{
		if (!native_money_session(connection, context.original.session_id) ||
		    (result_code == 0) != mutation_applied)
			return EINVAL;
		item_transfer_payload payload{};
		economic_frozen_intent intent;
		std::vector<uint8_t> encoded;
		if (!native_fee_command_bound(command, &payload, &intent) ||
		    critical_command_encode(command, &encoded) !=
			    critical_command_codec_result::ok ||
		    encoded != context.original.original_command ||
		    !context.original.acknowledged_save_revision ||
		    context.original.acknowledged_save_revision !=
			    payload.native_recovery.acknowledged_save_revision ||
		    intent.admission.metadata.lineage.bytes !=
			    context.original.authority.lineage.bytes ||
		    intent.admission.metadata.epoch.bytes !=
			    context.original.authority.epoch.bytes ||
		    context.original.authority.mappings.size() != 1)
			return EILSEQ;
		const economic_account_key expected{ intent.admission.metadata.lineage,
						     economic_account_kind::wallet,
						     payload.native_cost.wallet_mapping_id,
						     ECONOMIC_NATIVE_MOBILE_WALLET_CONTEXT };
		const auto &locked = context.original.authority.mappings[0].request;
		if (!economic_account_key_equal(context.native_wallet, expected) ||
		    !economic_account_key_equal(locked.account, expected) ||
		    locked.locator_kind != ECONOMIC_NATIVE_MOBILE_WALLET_LOCATOR ||
		    locked.native_id != payload.native_mobile.reference.mobile_instance_id)
			return EILSEQ;
		economic_accounting_plan plan;
		if (!native_fee_expected_plan(command, payload, intent, &plan))
			return EILSEQ;
		if (!mutation_applied)
		{
			if (!insert_operation(connection, command, intent, nullptr, {},
					      result_code) ||
			    !native_fee_operation_matches(connection, command, plan, result_code))
				return failure_code();
			plan.accounts.clear();
			plan.postings.clear();
			if (!native_fee_financial_rows(connection, command.operation_id, plan,
						       false, false) ||
			    !native_fee_source_matches(connection, plan.metadata, false))
				return failure_code();
		}
		else
		{
			if (economic_plan_encode(plan, &encoded) != economic_accounting_error::ok)
				return EILSEQ;
			if (!insert_operation(connection, command, intent, &plan, encoded, 0) ||
			    !insert_source_claim(connection, intent.admission.metadata) ||
			    !native_fee_financial_rows(connection, command.operation_id, plan, true,
						       true) ||
			    !native_fee_source_matches(connection, plan.metadata, true) ||
			    !native_fee_operation_matches(connection, command, plan, 0))
				return failure_code();
		}
		return native_money_session(connection, context.original.session_id) ? 0 : ENOTCONN;
	}
	catch (const std::bad_alloc &)
	{
		return ENOMEM;
	}
	catch (...)
	{
		return EILSEQ;
	}
#endif
}

unsigned int
economic_sql_native_fee_verify_retained(MYSQL *connection, const critical_command &command,
					unsigned int result_code,
					std::span<const uint8_t> result_payload) noexcept
{
#ifdef __NO_MYSQL__
	(void)connection;
	(void)command;
	(void)result_code;
	(void)result_payload;
	return ENOTSUP;
#else
	try
	{
		if (!connection)
			return EINVAL;
		const unsigned long session = mysql_thread_id(connection);
		if (!native_money_session(connection, session))
			return ENOTCONN;
		item_transfer_payload payload{};
		economic_frozen_intent intent;
		if (!native_fee_command_bound(command, &payload, &intent))
			return EINVAL;
		item_native_mobile_fee_result actual{}, expected{};
		std::array<uint8_t, ITEM_TRANSFER_NATIVE_MOBILE_FEE_RESULT_BYTES> canonical{};
		if (!item_native_mobile_fee_result_decode(result_payload, &actual) ||
		    !item_native_mobile_fee_result_encode(actual, &canonical) ||
		    !std::equal(canonical.begin(), canonical.end(), result_payload.begin(),
				result_payload.end()) ||
		    !item_native_mobile_fee_result_build(payload, &expected))
			return EILSEQ;
		if (result_code)
		{
			expected.mobile_cash_revision =
				payload.native_cost.projection.before_revision;
			expected.mobile_revision = payload.native_mobile.reference.mobile_revision;
		}
		if (actual != expected)
			return EILSEQ;
		economic_accounting_plan plan;
		if (!native_fee_expected_plan(command, payload, intent, &plan))
			return EILSEQ;
		if (!native_fee_operation_matches(connection, command, plan, result_code))
			return failure_code();
		const economic_account_key wallet{ plan.metadata.lineage,
						   economic_account_kind::wallet,
						   payload.native_cost.wallet_mapping_id,
						   ECONOMIC_NATIVE_MOBILE_WALLET_CONTEXT };
		if (!native_money_historical_mapping(
			    connection, wallet, ECONOMIC_NATIVE_MOBILE_WALLET_LOCATOR,
			    payload.native_mobile.reference.mobile_instance_id))
			return failure_code();
		native_mobile_wallet_origin origin;
		const auto origin_code = economic_sql_native_mobile_birth_observe_origin(
			connection, payload.native_mobile.reference, &origin);
		if (origin_code)
			return origin_code;
		if (origin.wallet_mapping_id != wallet.authority_id ||
		    origin.lineage.bytes != wallet.lineage.bytes)
			return EILSEQ;
		if (result_code)
		{
			plan.accounts.clear();
			plan.postings.clear();
		}
		if (!native_fee_financial_rows(connection, command.operation_id, plan, false,
					       result_code == 0) ||
		    !native_fee_source_matches(connection, plan.metadata, result_code == 0))
			return failure_code();
		return native_money_session(connection, session) ? 0 : ENOTCONN;
	}
	catch (const std::bad_alloc &)
	{
		return ENOMEM;
	}
	catch (...)
	{
		return EILSEQ;
	}
#endif
}
