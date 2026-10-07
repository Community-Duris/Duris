#include "persistence/economic_sql_auction_listing_transaction.h"
#include "persistence/economic_sql_auction_source_claim.h"

#include "economy/auction_listing_accounting.h"
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
#include <strings.h>
#include <vector>

namespace
{
constexpr uint16_t PLAYER_LOCATOR = 1;
constexpr uint16_t BANK_LOCATOR = 2;
constexpr uint16_t AUCTION_LOCATOR = 4;

std::string hex(std::span<const uint8_t> bytes)
{
	static constexpr char digits[] = "0123456789abcdef";
	std::string result = "X'";
	result.reserve(bytes.size() * 2 + 3);
	for (uint8_t byte : bytes)
	{
		result += digits[byte >> 4];
		result += digits[byte & 15];
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
	if (!mysql_real_query(connection, sql.data(), sql.size()))
		return true;
	errno = mysql_errno(connection) ? static_cast<int>(mysql_errno(connection)) : EIO;
	return false;
}

unsigned int failure_code()
{
	return errno ? static_cast<unsigned int>(errno) : EIO;
}

bool row(MYSQL *connection, const std::string &sql, size_t fields, std::vector<std::string> *values,
	 bool optional = false)
{
	if (!execute(connection, sql))
		return false;
	std::unique_ptr<MYSQL_RES, decltype(&mysql_free_result)> rows(
		mysql_store_result(connection), mysql_free_result);
	if (!rows || mysql_num_fields(rows.get()) != fields || mysql_num_rows(rows.get()) > 1 ||
	    (!optional && mysql_num_rows(rows.get()) != 1))
	{
		errno = mysql_errno(connection) ? static_cast<int>(mysql_errno(connection)) :
						  EILSEQ;
		return false;
	}
	std::vector<std::string> result;
	if (mysql_num_rows(rows.get()))
	{
		MYSQL_ROW cells = mysql_fetch_row(rows.get());
		const unsigned long *lengths = mysql_fetch_lengths(rows.get());
		if (!cells || !lengths)
		{
			errno = EILSEQ;
			return false;
		}
		result.reserve(fields);
		for (size_t index = 0; index < fields; ++index)
		{
			if (!cells[index])
			{
				errno = EILSEQ;
				return false;
			}
			result.emplace_back(cells[index], lengths[index]);
		}
	}
	*values = std::move(result);
	return true;
}

bool u64(const std::string &text, uint64_t *value)
{
	if (text.empty() || !value)
		return false;
	const auto parsed = std::from_chars(text.data(), text.data() + text.size(), *value);
	return parsed.ec == std::errc{} && parsed.ptr == text.data() + text.size();
}

bool i64(const std::string &text, int64_t *value)
{
	if (text.empty() || !value)
		return false;
	const auto parsed = std::from_chars(text.data(), text.data() + text.size(), *value);
	return parsed.ec == std::errc{} && parsed.ptr == text.data() + text.size();
}

uint64_t little_u64(std::span<const uint8_t> bytes, size_t offset)
{
	uint64_t result = 0;
	for (size_t index = 0; index < 8; ++index)
		result |= static_cast<uint64_t>(bytes[offset + index]) << (index * 8);
	return result;
}

std::string quoted(MYSQL *connection, const char *text, size_t size)
{
	std::string escaped(size * 2 + 1, '\0');
	const auto length = mysql_real_escape_string(connection, escaped.data(), text, size);
	escaped.resize(length);
	return "'" + escaped + "'";
}

bool identity(const critical_command &command, economic_frozen_intent *intent,
	      auction_command_payload *payload, economic_account_key *wallet,
	      economic_account_key *bank)
{
	if (!intent || !payload || !wallet || !bank ||
	    command.schema_version != CRITICAL_COMMAND_ACCOUNTING_SCHEMA_VERSION ||
	    !critical_command_envelope_valid(command) ||
	    !auction_command_decode_payload(command, payload) ||
	    economic_intent_decode(command.accounting_intent, intent) !=
		    economic_accounting_error::ok ||
	    economic_intent_verify_binding(command, *intent) != economic_accounting_error::ok ||
	    intent->admission.facts.size() != 16)
		return false;
	const auto facts = std::span<const uint8_t>(intent->admission.facts);
	const auto &lineage = intent->admission.metadata.lineage;
	*wallet = { lineage, economic_account_kind::wallet, little_u64(facts, 0), 0 };
	*bank = { lineage, economic_account_kind::bank, little_u64(facts, 8), payload->racewar };
	critical_command projected = command;
	projected.schema_version = CRITICAL_COMMAND_SCHEMA_VERSION;
	projected.accounting_intent.clear();
	projected.publication_required = false;
	std::vector<uint8_t> expected;
	return auction_listing_accounting_intent(projected, intent->admission.metadata.epoch,
						 *wallet, *bank,
						 &expected) == economic_accounting_error::ok &&
	       expected == command.accounting_intent;
}

bool mapping_native_hint(MYSQL *connection, uint64_t mapping, uint32_t *native)
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

bool owner_revision(MYSQL *connection, uint32_t pid, uint64_t *revision)
{
	const auto owner = std::to_string(pid);
	if (!execute(connection, "INSERT IGNORE INTO item_owner_revision(owner_type,owner_id,"
				 "owner_context_id,revision) VALUES(1," +
					 owner + ",0,0)"))
		return false;
	std::vector<std::string> values;
	if (!row(connection,
		 "SELECT revision FROM item_owner_revision WHERE owner_type=1 AND owner_id=" +
			 owner + " AND owner_context_id=0 FOR UPDATE",
		 1, &values) ||
	    !u64(values[0], revision))
	{
		if (!errno)
			errno = EILSEQ;
		return false;
	}
	return true;
}

bool locked_before(MYSQL *connection, const auction_command_payload &payload, uint32_t bank_id,
		   auction_listing_accounting_authority *before)
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
	const auto account =
		quoted(connection, payload.account_name.data(),
		       strnlen(payload.account_name.data(), payload.account_name.size()));
	if (!row(connection,
		 "SELECT id,bank_copper,bank_silver,bank_gold,bank_platinum,bank_revision "
		 "FROM account_banks WHERE account_name=" +
			 account + " AND racewar=" + std::to_string(payload.racewar) +
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
	if (!u64(values[5], &before->balances_before.bank_revision) ||
	    !owner_revision(connection, payload.actor_pid, &before->player_owner_revision_before))
	{
		if (!errno)
			errno = EILSEQ;
		return false;
	}
	std::array<size_t, AUCTION_COMMAND_MAX_ITEMS> order = {};
	for (size_t index = 0; index < payload.item_count; ++index)
		order[index] = index;
	std::sort(order.begin(), order.begin() + payload.item_count, [&](size_t left, size_t right)
		  { return payload.items[left].item_uid < payload.items[right].item_uid; });
	before->items_before.resize(payload.item_count);
	for (size_t position = 0; position < payload.item_count; ++position)
	{
		const size_t index = order[position];
		const auto &entry = payload.items[index];
		if (!row(connection,
			 "SELECT root_item_uid,COALESCE(parent_item_uid,0),owner_type,"
			 "owner_id,owner_context_id,item_revision,vnum,state "
			 "FROM item_current_owner WHERE item_uid=" +
				 std::to_string(entry.item_uid) + " FOR UPDATE",
			 8, &values))
			return false;
		uint64_t parsed[8] = {};
		int64_t vnum = 0;
		for (size_t field = 0; field < 8; ++field)
			if (field != 6 && !u64(values[field], &parsed[field]))
			{
				errno = EILSEQ;
				return false;
			}
		if (!i64(values[6], &vnum) || vnum != entry.vnum || parsed[2] > UINT8_MAX ||
		    parsed[7] > UINT8_MAX)
		{
			errno = EILSEQ;
			return false;
		}
		auto &snapshot = before->items_before[index];
		snapshot.uid = entry.item_uid;
		snapshot.position = { { static_cast<item_owner_type>(parsed[2]), parsed[3],
					parsed[4] },
				      parsed[0],
				      parsed[1],
				      parsed[5],
				      static_cast<item_custody_state>(parsed[7]) };
		if (!row(connection,
			 "SELECT item_uid FROM item_current_owner WHERE root_item_uid=" +
				 std::to_string(entry.item_uid) + " AND item_uid<>" +
				 std::to_string(entry.item_uid) + " LIMIT 1 FOR UPDATE",
			 1, &values, true))
			return false;
		if (!values.empty())
		{
			errno = EOPNOTSUPP;
			return false;
		}
	}
	return true;
}

bool create_escrow_mapping(MYSQL *connection, const critical_command &command,
			   const economic_frozen_intent &intent, uint32_t auction_id,
			   economic_account_key *escrow)
{
	if (!auction_id ||
	    !execute(connection,
		     "INSERT INTO economic_account_mapping(lineage,account_kind,context_id,"
		     "backend_kind,locator_kind,native_id,active_native_id,creating_operation_id) "
		     "VALUES(" +
			     id(intent.admission.metadata.lineage) + ",4,0,1," +
			     std::to_string(AUCTION_LOCATOR) + "," + std::to_string(auction_id) +
			     "," + std::to_string(auction_id) + "," + id(command.operation_id) +
			     ")"))
		return false;
	const auto mapping = static_cast<uint64_t>(mysql_insert_id(connection));
	if (!mapping)
	{
		errno = EILSEQ;
		return false;
	}
	*escrow = { intent.admission.metadata.lineage, economic_account_kind::auction_escrow,
		    mapping, 0 };
	economic_sql_authority_snapshot verified;
	const economic_sql_mapping_request request = { *escrow, AUCTION_LOCATOR, auction_id };
	const auto status = economic_sql_lock_authority(connection,
							intent.admission.metadata.lineage,
							intent.admission.metadata.epoch,
							std::span(&request, 1), &verified);
	if (status)
	{
		errno = static_cast<int>(status);
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
		id(command.operation_id) + "," + id(meta.lineage) + "," + id(meta.epoch) +
		",NULL," + std::to_string(meta.version) + "," + std::to_string(meta.writer_id) +
		"," + std::to_string(meta.policy_version) + "," +
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

unsigned int economic_sql_auction_listing_lock(MYSQL *connection, const critical_command &command,
					       economic_sql_auction_listing_context *context)
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
		economic_account_key wallet, bank;
		if (!identity(command, &intent, &payload, &wallet, &bank))
			return EPROTONOSUPPORT;
		economic_sql_auction_listing_context candidate;
		if (!mapping_native_hint(connection, bank.authority_id, &candidate.bank_id))
			return failure_code();
		const std::vector<economic_sql_mapping_request> requests = {
			{ wallet, PLAYER_LOCATOR, payload.actor_pid },
			{ bank, BANK_LOCATOR, candidate.bank_id }
		};
		const auto status = economic_sql_lock_authority(connection,
								intent.admission.metadata.lineage,
								intent.admission.metadata.epoch,
								requests, &candidate.authority);
		if (status)
			return status;
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
economic_sql_auction_listing_execute_and_record(MYSQL *connection, const critical_command &command,
						const economic_sql_auction_listing_context &context,
						auction_command_result *result,
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
		economic_sql_auction_listing_context active;
		const auto lock_error =
			economic_sql_auction_listing_lock(connection, command, &active);
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
		economic_account_key wallet, bank;
		if (!identity(command, &intent, &payload, &wallet, &bank))
			return EILSEQ;
		auction_listing_accounting_authority before;
		before.epoch = active.authority.epoch;
		before.wallet = wallet;
		before.bank = bank;
		if (!locked_before(connection, payload, active.bank_id, &before))
			return failure_code();
		if (!auction_repository_execute_accounted(connection, command, result, result_code,
							  mutation_applied))
			return failure_code();
		if ((*result_code == 0) != *mutation_applied)
			return EILSEQ;
		if (!*mutation_applied)
		{
			if (!insert_operation(connection, command, intent, nullptr, *result_code))
				return failure_code();
			return economic_sql_auction_source_claim_verify(connection, intent,
									*result_code);
		}
		if (!create_escrow_mapping(connection, command, intent, result->auction_id,
					   &before.escrow))
			return failure_code();
		economic_accounting_plan plan;
		if (auction_listing_accounting_plan(command, intent, before, *result, &plan) !=
			    economic_accounting_error::ok ||
		    !plan.children.empty() || plan.item_events.size() != payload.item_count)
			return EILSEQ;
		if (!insert_operation(connection, command, intent, &plan, 0))
			return failure_code();
		const auto source_claim_error =
			economic_sql_auction_source_claim_record(connection, command, intent);
		if (source_claim_error)
			return source_claim_error;
		for (size_t index = 0; index < plan.accounts.size(); ++index)
			if (!insert_effect(connection, command.operation_id, index,
					   plan.accounts[index]))
				return failure_code();
		for (size_t index = 0; index < plan.postings.size(); ++index)
			if (!insert_posting(connection, command.operation_id, index,
					    plan.postings[index]))
				return failure_code();
		for (size_t index = 0; index < plan.item_events.size(); ++index)
		{
			const auto &event = plan.item_events[index];
			economic_accounting_item_reference reference = {};
			reference.operation_id = command.operation_id;
			reference.line_index = static_cast<uint16_t>(index);
			reference.event_index = event.event_index;
			reference.child_index = event.child_index;
			reference.item_uid = event.uid;
			reference.before_revision = event.before.revision;
			reference.after_revision = event.after.revision;
			reference.legacy_operation_id = command.operation_id;
			reference.legacy_event_index = static_cast<uint16_t>(index);
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
