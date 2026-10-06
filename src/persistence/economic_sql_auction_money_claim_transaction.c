#include "persistence/economic_sql_auction_money_claim_transaction.h"
#include "persistence/economic_sql_pending_claim_source.h"

#include "economy/auction_money_claim_accounting.h"

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
#include <strings.h>
#include <vector>

namespace
{
constexpr uint16_t PLAYER_LOCATOR = 1;
constexpr uint16_t BANK_LOCATOR = 2;
constexpr uint16_t CLAIM_LOCATOR = 5;
constexpr size_t FACT_BYTES = 80;

std::string hex(std::span<const uint8_t> bytes)
{
	static constexpr char digits[] = "0123456789abcdef";
	std::string output = "X'";
	output.reserve(bytes.size() * 2 + 3);
	for (uint8_t byte : bytes)
	{
		output += digits[byte >> 4];
		output += digits[byte & 15];
	}
	output += '\'';
	return output;
}

std::string id(const critical_operation_id &value)
{
	return hex(value.bytes);
}

bool execute(MYSQL *connection, const std::string &sql)
{
	if (!mysql_real_query(connection, sql.data(), sql.size()))
		return true;
	errno = mysql_errno(connection) ? static_cast<int>(mysql_errno(connection)) : EIO;
	return false;
}

unsigned int failure_code()
{
	return errno ? static_cast<unsigned int>(errno) : EIO;
}

bool row(MYSQL *connection, const std::string &sql, size_t fields, std::vector<std::string> *values)
{
	if (!execute(connection, sql))
		return false;
	std::unique_ptr<MYSQL_RES, decltype(&mysql_free_result)> rows(
		mysql_store_result(connection), mysql_free_result);
	if (!rows || mysql_num_fields(rows.get()) != fields || mysql_num_rows(rows.get()) != 1)
	{
		errno = mysql_errno(connection) ? static_cast<int>(mysql_errno(connection)) :
						  EILSEQ;
		return false;
	}
	MYSQL_ROW cells = mysql_fetch_row(rows.get());
	const unsigned long *lengths = mysql_fetch_lengths(rows.get());
	if (!cells || !lengths)
	{
		errno = EILSEQ;
		return false;
	}
	values->clear();
	values->reserve(fields);
	for (size_t index = 0; index < fields; ++index)
	{
		if (!cells[index])
		{
			errno = EILSEQ;
			return false;
		}
		values->emplace_back(cells[index], lengths[index]);
	}
	return true;
}

bool u64(const std::string &text, uint64_t *value)
{
	if (text.empty())
		return false;
	const auto parsed = std::from_chars(text.data(), text.data() + text.size(), *value);
	return parsed.ec == std::errc{} && parsed.ptr == text.data() + text.size();
}

bool i64(const std::string &text, int64_t *value)
{
	if (text.empty())
		return false;
	const auto parsed = std::from_chars(text.data(), text.data() + text.size(), *value);
	return parsed.ec == std::errc{} && parsed.ptr == text.data() + text.size();
}

uint64_t little_u64(std::span<const uint8_t> bytes, size_t offset)
{
	uint64_t value = 0;
	for (size_t index = 0; index < 8; ++index)
		value |= static_cast<uint64_t>(bytes[offset + index]) << (index * 8);
	return value;
}

uint32_t little_u32(std::span<const uint8_t> bytes, size_t offset)
{
	uint32_t value = 0;
	for (size_t index = 0; index < 4; ++index)
		value |= static_cast<uint32_t>(bytes[offset + index]) << (index * 8);
	return value;
}

bool identity(const critical_command &command, economic_frozen_intent *intent,
	      auction_command_payload *payload, economic_account_key *wallet,
	      economic_account_key *bank, economic_account_key *claim)
{
	if (!intent || !payload || !wallet || !bank || !claim ||
	    command.schema_version != CRITICAL_COMMAND_ACCOUNTING_SCHEMA_VERSION ||
	    !critical_command_envelope_valid(command) ||
	    !auction_command_decode_payload(command, payload) ||
	    payload->action != auction_action::claim_money || !payload->actor_pid ||
	    economic_intent_decode(command.accounting_intent, intent) !=
		    economic_accounting_error::ok ||
	    economic_intent_verify_binding(command, *intent) != economic_accounting_error::ok ||
	    intent->admission.facts.size() != FACT_BYTES)
		return false;
	const auto &meta = intent->admission.metadata;
	const auto facts = std::span<const uint8_t>(intent->admission.facts);
	if (meta.writer_id != ECONOMIC_WRITER_AUCTION_MONEY_CLAIM ||
	    meta.reason != economic_reason::auction_claim ||
	    meta.actor_kind != economic_actor_kind::domain || meta.actor_id != payload->actor_pid ||
	    little_u32(facts, 24) != payload->actor_pid || !little_u64(facts, 28) ||
	    little_u64(facts, 28) > INT_MAX || !little_u32(facts, 44) ||
	    little_u32(facts, 44) > ECONOMIC_AUCTION_CLAIM_MAX_SOURCES || !meta.source_event ||
	    meta.source_event->kind != economic_source_kind::service ||
	    meta.source_event->source.bytes != meta.original_operation_id.bytes ||
	    meta.source_event->generation.bytes != meta.original_operation_id.bytes ||
	    meta.source_event->sequence != little_u64(facts, 36) || !meta.source_event->slot)
		return false;
	*wallet = { meta.lineage, economic_account_kind::wallet, little_u64(facts, 0), 0 };
	*bank = { meta.lineage, economic_account_kind::bank, little_u64(facts, 8),
		  payload->racewar };
	*claim = { meta.lineage, economic_account_kind::pending_claim, little_u64(facts, 16), 0 };
	return economic_account_key_valid(*wallet) && economic_account_key_valid(*bank) &&
	       economic_account_key_valid(*claim);
}

bool bank_hint(MYSQL *connection, uint64_t mapping, uint32_t *native)
{
	std::vector<std::string> values;
	uint64_t number = 0;
	if (!row(connection,
		 "SELECT native_id FROM economic_account_mapping WHERE mapping_id=" +
			 std::to_string(mapping),
		 1, &values) ||
	    !u64(values[0], &number) || !number || number > UINT32_MAX)
	{
		if (!errno)
			errno = EILSEQ;
		return false;
	}
	*native = static_cast<uint32_t>(number);
	return true;
}

bool locked_sources(MYSQL *connection, const economic_account_key &claim,
		    auction_money_claim_state *state)
{
	std::vector<economic_sql_pending_claim_remaining> sources;
	const auto error = economic_sql_pending_claim_source_remaining(
		connection, claim, state->beneficiary_pid, &sources);
	if (error)
	{
		errno = static_cast<int>(error);
		return false;
	}
	state->sources.clear();
	for (const auto &source : sources)
		state->sources.push_back({ source.operation, source.slot, state->beneficiary_pid,
					   claim.authority_id, source.remaining_amount });
	return true;
}

bool locked_before(MYSQL *connection, const auction_command_payload &payload, uint32_t bank_id,
		   auction_money_claim_authority *before)
{
	std::vector<std::string> values;
	if (!row(connection,
		 "SELECT account_name,racewar,copper,silver,gold,platinum,wallet_revision "
		 "FROM player_data WHERE pid=" +
			 std::to_string(payload.actor_pid) + " FOR UPDATE",
		 7, &values))
		return false;
	uint64_t race = 0;
	if (strcasecmp(values[0].c_str(), payload.account_name.data()) || !u64(values[1], &race) ||
	    race != payload.racewar)
	{
		errno = ESTALE;
		return false;
	}
	for (size_t index = 0; index < 4; ++index)
		if (!i64(values[index + 2], &before->balances_before.wallet.amount[index]) ||
		    before->balances_before.wallet.amount[index] < 0)
		{
			errno = EILSEQ;
			return false;
		}
	if (!u64(values[6], &before->balances_before.wallet_revision))
	{
		errno = EILSEQ;
		return false;
	}
	std::string quoted(payload.account_name.data(),
			   strnlen(payload.account_name.data(), payload.account_name.size()));
	std::string escaped(quoted.size() * 2 + 1, '\0');
	escaped.resize(
		mysql_real_escape_string(connection, escaped.data(), quoted.data(), quoted.size()));
	if (!row(connection,
		 "SELECT id,bank_copper,bank_silver,bank_gold,bank_platinum,bank_revision "
		 "FROM account_banks WHERE account_name='" +
			 escaped + "' AND racewar=" + std::to_string(payload.racewar) +
			 " FOR UPDATE",
		 6, &values))
		return false;
	uint64_t native_bank = 0;
	if (!u64(values[0], &native_bank) || native_bank != bank_id)
	{
		errno = ESTALE;
		return false;
	}
	for (size_t index = 0; index < 4; ++index)
		if (!i64(values[index + 1], &before->balances_before.bank.amount[index]) ||
		    before->balances_before.bank.amount[index] < 0)
		{
			errno = EILSEQ;
			return false;
		}
	if (!u64(values[5], &before->balances_before.bank_revision))
	{
		errno = EILSEQ;
		return false;
	}
	if (!row(connection,
		 "SELECT money,claim_revision FROM auction_money_pickups WHERE pid=" +
			 std::to_string(payload.actor_pid) + " FOR UPDATE",
		 2, &values))
		return false;
	uint64_t money = 0;
	if (!u64(values[0], &money) || money > INT_MAX || !money ||
	    !u64(values[1], &before->claim.revision))
	{
		errno = EILSEQ;
		return false;
	}
	before->claim.beneficiary_pid = payload.actor_pid;
	before->claim.money = static_cast<int64_t>(money);
	return locked_sources(connection, before->claim_account, &before->claim);
}

bool locked_after(MYSQL *connection, const auction_command_payload &payload,
		  const auction_money_claim_authority &before, uint32_t bank_id,
		  const auction_command_result &result)
{
	std::vector<std::string> values;
	uint64_t money = 0, revision = 0;
	if (!row(connection,
		 "SELECT money,claim_revision FROM auction_money_pickups WHERE pid=" +
			 std::to_string(payload.actor_pid) + " FOR UPDATE",
		 2, &values) ||
	    !u64(values[0], &money) || money || !u64(values[1], &revision) ||
	    revision != before.claim.revision + 1)
		return false;
	if (!row(connection,
		 "SELECT copper,silver,gold,platinum,wallet_revision FROM player_data WHERE pid=" +
			 std::to_string(payload.actor_pid) + " FOR UPDATE",
		 5, &values))
		return false;
	for (size_t index = 0; index < 4; ++index)
	{
		int64_t amount = 0;
		if (!i64(values[index], &amount) || amount != result.wallet.amount[index])
		{
			errno = EILSEQ;
			return false;
		}
	}
	if (!u64(values[4], &revision) || revision != result.wallet_revision)
	{
		errno = EILSEQ;
		return false;
	}
	if (!row(connection,
		 "SELECT bank_copper,bank_silver,bank_gold,bank_platinum,bank_revision "
		 "FROM account_banks WHERE id=" +
			 std::to_string(bank_id) + " FOR UPDATE",
		 5, &values))
		return false;
	for (size_t index = 0; index < 4; ++index)
	{
		int64_t amount = 0;
		if (!i64(values[index], &amount) || amount != result.bank.amount[index])
		{
			errno = EILSEQ;
			return false;
		}
	}
	if (!u64(values[4], &revision) || revision != result.bank_revision)
	{
		errno = EILSEQ;
		return false;
	}
	return true;
}

bool insert_operation(MYSQL *connection, const critical_command &command,
		      const economic_frozen_intent &intent, const economic_accounting_plan *plan,
		      unsigned int result_code)
{
	const auto &meta = intent.admission.metadata;
	std::vector<uint8_t> encoded_plan;
	economic_digest plan_digest = {}, intent_digest = {};
	std::array<uint8_t, ECONOMIC_SOURCE_EVENT_BYTES> source = {};
	if (!meta.source_event ||
	    economic_source_event_encode(*meta.source_event, &source) !=
		    economic_accounting_error::ok ||
	    economic_intent_digest(intent, &intent_digest) != economic_accounting_error::ok ||
	    (plan &&
	     (economic_plan_encode(*plan, &encoded_plan) != economic_accounting_error::ok ||
	      economic_plan_digest(*plan, &plan_digest) != economic_accounting_error::ok)) ||
	    ((plan != nullptr) != (result_code == 0)))
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
		id(command.operation_id) + "," + id(meta.lineage) + "," + id(meta.epoch) + "," +
		id(meta.original_operation_id) + "," + std::to_string(meta.version) + "," +
		std::to_string(meta.writer_id) + "," + std::to_string(meta.policy_version) + "," +
		std::to_string(meta.compiler_version) + "," +
		std::to_string(static_cast<uint8_t>(meta.actor_kind)) + "," +
		std::to_string(meta.actor_id) + "," +
		std::to_string(static_cast<uint16_t>(meta.reason)) + "," + hex(source) + "," +
		hex(intent_digest) + "," + hex(intent.domain_digest) + "," +
		(plan ? hex(plan_digest) : "NULL") + "," + hex(command.accounting_intent) + "," +
		(plan ? hex(encoded_plan) : "NULL") + "," +
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

unsigned int
economic_sql_auction_money_claim_lock(MYSQL *connection, const critical_command &command,
				      economic_sql_auction_money_claim_context *context)
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
		auction_command_payload payload = {};
		economic_account_key wallet, bank, claim;
		if (!identity(command, &intent, &payload, &wallet, &bank, &claim))
			return EPROTONOSUPPORT;
		economic_sql_auction_money_claim_context candidate;
		if (!bank_hint(connection, bank.authority_id, &candidate.bank_id))
			return failure_code();
		const std::vector<economic_sql_mapping_request> requests = {
			{ wallet, PLAYER_LOCATOR, payload.actor_pid },
			{ bank, BANK_LOCATOR, candidate.bank_id },
			{ claim, CLAIM_LOCATOR, payload.actor_pid }
		};
		const auto error = economic_sql_lock_authority(connection,
							       intent.admission.metadata.lineage,
							       intent.admission.metadata.epoch,
							       requests, &candidate.authority);
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

unsigned int economic_sql_auction_money_claim_execute_and_record(
	MYSQL *connection, const critical_command &command,
	const economic_sql_auction_money_claim_context &context, auction_command_result *result,
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
		economic_sql_auction_money_claim_context active;
		const auto lock_error =
			economic_sql_auction_money_claim_lock(connection, command, &active);
		if (lock_error)
			return lock_error;
		if (active.bank_id != context.bank_id ||
		    active.authority.lineage.bytes != context.authority.lineage.bytes ||
		    active.authority.epoch.bytes != context.authority.epoch.bytes ||
		    active.authority.lineage_revision != context.authority.lineage_revision ||
		    active.authority.mappings.size() != context.authority.mappings.size())
			return ESTALE;
		for (size_t index = 0; index < active.authority.mappings.size(); ++index)
			if (active.authority.mappings[index].request.account.authority_id !=
				    context.authority.mappings[index].request.account.authority_id ||
			    active.authority.mappings[index].revision !=
				    context.authority.mappings[index].revision)
				return ESTALE;
		economic_frozen_intent intent;
		auction_command_payload payload = {};
		economic_account_key wallet, bank, claim;
		if (!identity(command, &intent, &payload, &wallet, &bank, &claim))
			return EILSEQ;
		auction_money_claim_authority before;
		before.epoch = active.authority.epoch;
		before.wallet = wallet;
		before.bank = bank;
		before.claim_account = claim;
		if (!locked_before(connection, payload, active.bank_id, &before))
			return failure_code();
		critical_command projected = command;
		projected.schema_version = CRITICAL_COMMAND_SCHEMA_VERSION;
		projected.accounting_intent.clear();
		projected.publication_required = false;
		std::vector<uint8_t> expected;
		if (auction_money_claim_accounting_intent(projected, before.epoch, wallet, bank,
							  claim, before.claim, &expected) !=
			    economic_accounting_error::ok ||
		    expected != command.accounting_intent)
			return ESTALE;
		if (!auction_repository_execute_accounted(connection, command, result, result_code,
							  mutation_applied))
			return failure_code();
		if ((*result_code == 0) != *mutation_applied)
			return EILSEQ;
		if (!*mutation_applied)
			return insert_operation(connection, command, intent, nullptr,
						*result_code) ?
				       0 :
				       failure_code();
		if (!locked_after(connection, payload, before, active.bank_id, *result))
			return failure_code();
		economic_accounting_plan plan;
		if (auction_money_claim_accounting_plan(command, intent, before, *result, &plan) !=
			    economic_accounting_error::ok ||
		    !plan.children.empty() || !plan.item_events.empty())
			return EILSEQ;
		if (!insert_operation(connection, command, intent, &plan, 0))
			return failure_code();
		if (!execute(connection,
			     "INSERT INTO economic_accounting_source_claim("
			     "lineage,source_event,operation_id,outcome) SELECT lineage,"
			     "source_event,operation_id,outcome FROM economic_accounting_operation "
			     "WHERE operation_id=" +
				     id(command.operation_id)))
			return failure_code();
		for (size_t index = 0; index < plan.accounts.size(); ++index)
			if (!insert_effect(connection, command.operation_id, index,
					   plan.accounts[index]))
				return failure_code();
		for (size_t index = 0; index < plan.postings.size(); ++index)
			if (!insert_posting(connection, command.operation_id, index,
					    plan.postings[index]))
				return failure_code();
		const auto consumed = economic_sql_pending_claim_source_consume(
			connection, command.operation_id, claim, payload.actor_pid,
			static_cast<uint64_t>(before.claim.money),
			static_cast<uint64_t>(before.claim.money));
		if (consumed)
			return consumed;
		return 0;
	}
	catch (const std::bad_alloc &)
	{
		return ENOMEM;
	}
#endif
}
