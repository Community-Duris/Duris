#include "persistence/economic_sql_collector_transaction.h"

#include "economy/collector_accounting.h"
#include "item/economic_accounting_item_reference.h"

#include <cerrno>

#ifndef __NO_MYSQL__
#include <algorithm>
#include <array>
#include <charconv>
#include <climits>
#include <cstring>
#include <memory>
#include <new>
#include <string>
#include <vector>

namespace
{
constexpr uint16_t PLAYER_LOCATOR = 1;
constexpr uint16_t BANK_LOCATOR = 2;

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
	return errno ? static_cast<unsigned int>(errno) : EIO;
}

bool mapping_native_hint(MYSQL *connection, uint64_t mapping, uint32_t *native)
{
	if (!mapping || !native ||
	    !execute(connection,
		     "SELECT native_id FROM economic_account_mapping WHERE mapping_id=" +
			     std::to_string(mapping)))
		return false;
	std::unique_ptr<MYSQL_RES, decltype(&mysql_free_result)> rows(
		mysql_store_result(connection), mysql_free_result);
	if (!rows || mysql_num_rows(rows.get()) != 1 || mysql_num_fields(rows.get()) != 1)
	{
		errno = mysql_errno(connection) ? static_cast<int>(mysql_errno(connection)) :
						  EILSEQ;
		return false;
	}
	MYSQL_ROW row = mysql_fetch_row(rows.get());
	uint64_t value = 0;
	if (!row || !row[0])
	{
		errno = EILSEQ;
		return false;
	}
	const char *end = row[0] + std::strlen(row[0]);
	const auto parsed = std::from_chars(row[0], end, value);
	if (parsed.ec != std::errc{} || parsed.ptr != end || !value || value > UINT32_MAX)
	{
		errno = EILSEQ;
		return false;
	}
	*native = static_cast<uint32_t>(value);
	return true;
}

uint64_t u64(std::span<const uint8_t> bytes)
{
	uint64_t value = 0;
	for (size_t index = 0; index < sizeof(value); ++index)
		value |= static_cast<uint64_t>(bytes[index]) << (index * 8);
	return value;
}

bool purchase(const collector_command_payload &payload)
{
	return payload.action == collector_action::purchase;
}

bool held(const collector_command_payload &payload)
{
	return payload.action == collector_action::expire ||
	       payload.action == collector_action::cancel;
}

bool identity(const critical_command &command, economic_frozen_intent *intent,
	      collector_command_payload *payload)
{
	if (!intent || !payload || !critical_command_envelope_valid(command) ||
	    command.schema_version != CRITICAL_COMMAND_ACCOUNTING_SCHEMA_VERSION ||
	    !collector_command_decode_payload(command, payload))
		return false;
	if (purchase(*payload))
	{
		collector::record listing;
		economic_account_key wallet, bank;
		return collector_purchase_accounting_decode(command, intent, payload, &listing,
							    &wallet,
							    &bank) == economic_accounting_error::ok;
	}
	if (!held(*payload) ||
	    economic_intent_decode(command.accounting_intent, intent) !=
		    economic_accounting_error::ok ||
	    economic_intent_verify_binding(command, *intent) != economic_accounting_error::ok)
		return false;
	const auto &meta = intent->admission.metadata;
	if (intent->admission.facts.size() != collector::encoded_record_bytes)
		return false;
	collector::record listing;
	if (collector::record_decode(intent->admission.facts.data(),
				     collector::encoded_record_bytes,
				     &listing) != collector::codec_result::ok)
		return false;
	critical_command projected = command;
	projected.schema_version = CRITICAL_COMMAND_SCHEMA_VERSION;
	projected.accounting_intent.clear();
	projected.publication_required = false;
	std::vector<uint8_t> expected;
	const auto status = collector_held_accounting_intent(projected, meta.lineage, meta.epoch,
							     listing, &expected);
	return status == economic_accounting_error::ok && expected == command.accounting_intent;
}

bool insert_operation(MYSQL *connection, const critical_command &command,
		      const economic_frozen_intent &intent, const economic_accounting_plan *plan,
		      unsigned int result_code)
{
	const auto &meta = intent.admission.metadata;
	std::vector<uint8_t> encoded_plan;
	economic_digest plan_digest = {}, intent_digest = {};
	if (economic_intent_digest(intent, &intent_digest) != economic_accounting_error::ok)
	{
		errno = EILSEQ;
		return false;
	}
	if (plan && (economic_plan_encode(*plan, &encoded_plan) != economic_accounting_error::ok ||
		     economic_plan_digest(*plan, &plan_digest) != economic_accounting_error::ok))
	{
		errno = EILSEQ;
		return false;
	}
	if ((plan != nullptr) != (result_code == 0) || meta.source_event)
	{
		errno = EINVAL;
		return false;
	}
	const std::string sql =
		"INSERT INTO economic_accounting_operation(operation_id,lineage,epoch,"
		"original_operation_id,accounting_version,writer_id,policy_version,compiler_version,"
		"actor_kind,actor_id,reason,source_event,intent_digest,domain_digest,plan_digest,"
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
		std::to_string(static_cast<uint16_t>(meta.reason)) + ",NULL," + hex(intent_digest) +
		"," + hex(intent.domain_digest) + "," + (plan ? hex(plan_digest) : "NULL") + "," +
		hex(command.accounting_intent) + "," + (plan ? hex(encoded_plan) : "NULL") + "," +
		(plan ? "1,0," : "2," + std::to_string(result_code) + ",") +
		std::to_string(plan ? plan->accounts.size() : 0) + "," +
		std::to_string(plan ? plan->postings.size() : 0) + "," +
		std::to_string(plan ? plan->children.size() : 0) + "," +
		std::to_string(plan ? plan->item_events.size() : 0) + "," +
		std::to_string(plan ? plan->items_before.size() : 0) + "," +
		std::to_string(plan ? plan->items_after.size() : 0) + ")";
	return execute(connection, sql);
}

bool insert_effect(MYSQL *connection, const critical_operation_id &operation, size_t index,
		   const economic_account_effect &effect)
{
	std::array<uint8_t, ECONOMIC_ACCOUNT_KEY_BYTES> key = {};
	if (economic_account_key_encode(effect.key, &key) != economic_accounting_error::ok)
	{
		errno = EILSEQ;
		return false;
	}
	std::string sql = "INSERT INTO economic_accounting_account_effect(operation_id,"
			  "account_index,account_key,before_copper,before_silver,before_gold,"
			  "before_platinum,after_copper,after_silver,after_gold,after_platinum,"
			  "before_revision,after_revision) VALUES(" +
			  id(operation) + "," + std::to_string(index) + "," + hex(key);
	for (int64_t amount : effect.before)
		sql += "," + std::to_string(amount);
	for (int64_t amount : effect.after)
		sql += "," + std::to_string(amount);
	sql += "," + std::to_string(effect.before_revision) + "," +
	       std::to_string(effect.after_revision) + ")";
	return execute(connection, sql);
}

bool insert_posting(MYSQL *connection, const critical_operation_id &operation, size_t index,
		    const economic_coin_posting &posting)
{
	std::string sql = "INSERT INTO economic_accounting_coin_posting(operation_id,"
			  "line_index,event_index,account_index,child_index,delta_copper,"
			  "delta_silver,delta_gold,delta_platinum,copper_value) VALUES(" +
			  id(operation) + "," + std::to_string(index) + "," +
			  std::to_string(posting.event_index) + "," +
			  std::to_string(posting.account_index) + "," +
			  std::to_string(posting.child_index);
	for (int64_t amount : posting.delta)
		sql += "," + std::to_string(amount);
	sql += "," + std::to_string(posting.copper) + ")";
	return execute(connection, sql);
}
} // namespace
#endif

unsigned int economic_sql_collector_lock(MYSQL *connection, const critical_command &command,
					 economic_sql_collector_context *context)
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
		collector_command_payload payload = {};
		if (!identity(command, &intent, &payload))
			return EPROTONOSUPPORT;
		economic_sql_collector_context candidate;
		std::array<economic_sql_mapping_request, 2> requests = {};
		std::span<const economic_sql_mapping_request> needed;
		if (purchase(payload))
		{
			const auto facts = std::span<const uint8_t>(intent.admission.facts);
			candidate.wallet_account = { intent.admission.metadata.lineage,
						     economic_account_kind::wallet,
						     u64(facts.subspan(0, 8)), 0 };
			candidate.bank_account = { intent.admission.metadata.lineage,
						   economic_account_kind::bank,
						   u64(facts.subspan(8, 8)), payload.racewar };
			if (!economic_account_key_valid(candidate.wallet_account) ||
			    !economic_account_key_valid(candidate.bank_account))
				return EINVAL;
			if (!mapping_native_hint(connection, candidate.bank_account.authority_id,
						 &candidate.bank_id))
				return failure_code();
			requests = {
				economic_sql_mapping_request{ candidate.wallet_account,
							      PLAYER_LOCATOR, payload.actor_pid },
				economic_sql_mapping_request{ candidate.bank_account, BANK_LOCATOR,
							      candidate.bank_id }
			};
			needed = requests;
		}
		const auto error = economic_sql_lock_authority(connection,
							       intent.admission.metadata.lineage,
							       intent.admission.metadata.epoch,
							       needed, &candidate.authority);
		if (error)
			return error;
		candidate.session_id = mysql_thread_id(connection);
		if (!(connection->server_status & SERVER_STATUS_IN_TRANS))
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

unsigned int
economic_sql_collector_execute_and_record(MYSQL *connection, const critical_command &command,
					  const economic_sql_collector_context &context,
					  collector_command_result *result,
					  unsigned int *result_code, bool *mutation_applied)
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
		// A context can outlive the transaction that created it. Renew every
		// authority lock before using its keys or touching native state.
		economic_sql_collector_context active;
		const auto lock_error = economic_sql_collector_lock(connection, command, &active);
		if (lock_error)
			return lock_error;
		if (active.authority.lineage.bytes != context.authority.lineage.bytes ||
		    active.authority.epoch.bytes != context.authority.epoch.bytes ||
		    active.authority.lineage_revision != context.authority.lineage_revision ||
		    active.wallet_account.authority_id != context.wallet_account.authority_id ||
		    active.bank_account.authority_id != context.bank_account.authority_id ||
		    active.bank_id != context.bank_id ||
		    active.authority.mappings.size() != context.authority.mappings.size())
			return ESTALE;
		for (size_t index = 0; index < active.authority.mappings.size(); ++index)
			if (active.authority.mappings[index].revision !=
			    context.authority.mappings[index].revision)
				return ESTALE;
		economic_frozen_intent intent;
		collector_command_payload payload = {};
		if (!identity(command, &intent, &payload) ||
		    intent.admission.metadata.lineage.bytes != active.authority.lineage.bytes ||
		    intent.admission.metadata.epoch.bytes != active.authority.epoch.bytes)
			return EILSEQ;
		collector_repository_locked_before before;
		if (!collector_repository_execute_accounted(connection, command, result,
							    result_code, mutation_applied, &before))
			return failure_code();
		if ((*result_code == 0) != *mutation_applied)
			return EILSEQ;
		if (!*mutation_applied)
			return insert_operation(connection, command, intent, nullptr,
						*result_code) ?
				       0 :
				       failure_code();
		economic_accounting_plan plan;
		economic_accounting_error status = economic_accounting_error::invalid_identity;
		if (purchase(payload))
		{
			if (!before.bank_id || before.bank_id != active.bank_id)
				return ESTALE;
			if (!result->materialized_item_id)
				return EILSEQ;
			collector_purchase_accounting_authority authority;
			authority.epoch = active.authority.epoch;
			authority.wallet_account = active.wallet_account;
			authority.bank_account = active.bank_account;
			authority.balances_before = before.balances;
			authority.listing_before = before.listing;
			authority.item_before = before.item;
			authority.catalog_revision_before = before.catalog_revision;
			authority.from_owner_revision_before = before.from_owner_revision;
			authority.to_owner_revision_before = before.to_owner_revision;
			status = collector_purchase_accounting_plan(command, intent, authority,
								    *result, &plan);
		}
		else
		{
			collector_held_accounting_authority authority;
			authority.lineage = active.authority.lineage;
			authority.epoch = active.authority.epoch;
			authority.listing_before = before.listing;
			authority.item_before = before.item;
			authority.catalog_revision_before = before.catalog_revision;
			authority.from_owner_revision_before = before.from_owner_revision;
			authority.to_owner_revision_before = before.to_owner_revision;
			status = collector_held_accounting_plan(command, intent, authority, *result,
								&plan);
		}
		if (status != economic_accounting_error::ok || plan.item_events.size() != 1 ||
		    !plan.children.empty())
			return EILSEQ;
		if (!insert_operation(connection, command, intent, &plan, 0))
			return failure_code();
		for (size_t index = 0; index < plan.accounts.size(); ++index)
			if (!insert_effect(connection, command.operation_id, index,
					   plan.accounts[index]))
				return failure_code();
		for (size_t index = 0; index < plan.postings.size(); ++index)
			if (!insert_posting(connection, command.operation_id, index,
					    plan.postings[index]))
				return failure_code();
		const auto &event = plan.item_events[0];
		economic_accounting_item_reference reference = {};
		reference.operation_id = command.operation_id;
		reference.line_index = 0;
		reference.event_index = 0;
		reference.child_index = 0;
		reference.item_uid = event.uid;
		reference.before_revision = event.before.revision;
		reference.after_revision = event.after.revision;
		reference.legacy_operation_id = command.operation_id;
		reference.legacy_event_index = 0;
		if (!economic_accounting_item_reference_insert(connection, reference))
			return failure_code();
		return 0;
	}
	catch (const std::bad_alloc &)
	{
		return ENOMEM;
	}
#endif
}
