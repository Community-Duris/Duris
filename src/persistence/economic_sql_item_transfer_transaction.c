#include "persistence/economic_sql_item_transfer_transaction.h"

#include "item/economic_accounting_item_reference.h"

#include <algorithm>
#include <cerrno>
#include <charconv>
#include <cstring>
#include <memory>
#include <new>
#include <optional>
#include <string>
#include <vector>

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
		return 0;
	}
	catch (const std::bad_alloc &)
	{
		return ENOMEM;
	}
#endif
}
