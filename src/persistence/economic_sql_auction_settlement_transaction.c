#include "persistence/economic_sql_auction_settlement_transaction.h"
#include "persistence/economic_sql_auction_source_claim.h"

#include "economy/auction_settlement_accounting.h"
#include "persistence/economic_sql_pending_claim_source.h"

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
constexpr uint16_t CLAIM_LOCATOR = 5;

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

bool hex_id(const std::string &encoded, critical_operation_id *value)
{
	return encoded.size() == 32 && critical_operation_id_from_hex(encoded.c_str(), value);
}

std::string quoted(MYSQL *connection, const char *text, size_t length)
{
	std::string escaped(length * 2 + 1, '\0');
	const auto size = mysql_real_escape_string(connection, escaped.data(), text, length);
	escaped.resize(size);
	return "'" + escaped + "'";
}

bool identity(const critical_command &command, economic_frozen_intent *intent,
	      auction_command_payload *payload, auction_settlement_listing *listing,
	      auction_settlement_accounts *accounts)
{
	return auction_settlement_accounting_decode(command, intent, payload, listing, accounts) ==
	       economic_accounting_error::ok;
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

bool claim(MYSQL *connection, uint32_t pid, int64_t *money, uint64_t *revision,
	   bool optional = true)
{
	std::vector<std::string> values;
	if (!row(connection,
		 "SELECT money,claim_revision FROM auction_money_pickups WHERE pid=" +
			 std::to_string(pid) + " FOR UPDATE",
		 2, &values, optional))
		return false;
	if (values.empty())
	{
		*money = 0;
		*revision = 0;
		return true;
	}
	uint64_t amount = 0;
	if (!u64(values[0], &amount) || amount > UINT_MAX || !u64(values[1], revision))
	{
		errno = EILSEQ;
		return false;
	}
	*money = static_cast<int64_t>(amount);
	return true;
}

bool actor_before(MYSQL *connection, const auction_command_payload &payload, uint32_t bank_id,
		  currency_command_result *before)
{
	if (!payload.actor_pid)
		return true;
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
		if (!i64(values[index + 2], &before->wallet.amount[index]) ||
		    before->wallet.amount[index] < 0)
		{
			errno = EILSEQ;
			return false;
		}
	if (!u64(values[6], &before->wallet_revision))
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
		if (!i64(values[index + 1], &before->bank.amount[index]) ||
		    before->bank.amount[index] < 0)
		{
			errno = EILSEQ;
			return false;
		}
	if (!u64(values[5], &before->bank_revision))
	{
		errno = EILSEQ;
		return false;
	}
	return true;
}

bool locked_before(MYSQL *connection, const auction_command_payload &payload, uint32_t bank_id,
		   auction_settlement_authority *before)
{
	if (!actor_before(connection, payload, bank_id, &before->actor_balances_before))
		return false;
	std::vector<std::string> values;
	if (!row(connection,
		 "SELECT seller_pid,winning_bidder_pid,status+0,custody_state,quantity,cur_price,"
		 "buy_price,auction_revision,UNIX_TIMESTAMP(end_time),HEX(listing_operation_id) "
		 "FROM auctions WHERE id=" +
			 std::to_string(payload.auction_id) + " FOR UPDATE",
		 10, &values))
		return false;
	uint64_t parsed[9] = {};
	for (size_t index = 0; index < 9; ++index)
		if (!u64(values[index], &parsed[index]) ||
		    (index < 5 && parsed[index] > UINT32_MAX))
		{
			errno = EILSEQ;
			return false;
		}
	auto &listing = before->listing;
	listing.auction_id = payload.auction_id;
	listing.seller_pid = static_cast<uint32_t>(parsed[0]);
	listing.winner_pid = static_cast<uint32_t>(parsed[1]);
	listing.status = static_cast<uint32_t>(parsed[2]);
	listing.custody_state = static_cast<uint32_t>(parsed[3]);
	listing.quantity = static_cast<uint32_t>(parsed[4]);
	listing.current_price = static_cast<int64_t>(parsed[5]);
	listing.buy_price = static_cast<int64_t>(parsed[6]);
	listing.revision = parsed[7];
	listing.end_time = parsed[8];
	if (parsed[5] > INT64_MAX || parsed[6] > INT64_MAX ||
	    !hex_id(values[9], &listing.listing_operation))
	{
		errno = EILSEQ;
		return false;
	}
	if (listing.winner_pid)
	{
		if (!row(connection,
			 "SELECT HEX(operation_id),actor_pid,final_price FROM auction_ledger "
			 "WHERE auction_id=" +
				 std::to_string(listing.auction_id) + " AND auction_revision=" +
				 std::to_string(listing.revision) + " AND event_type=2 FOR UPDATE",
			 3, &values))
			return false;
		uint64_t bidder = 0, price = 0;
		if (!hex_id(values[0], &listing.winning_bid_operation) ||
		    !u64(values[1], &bidder) || bidder != listing.winner_pid ||
		    !u64(values[2], &price) ||
		    price != static_cast<uint64_t>(listing.current_price))
		{
			errno = EILSEQ;
			return false;
		}
	}
	if (!listing.quantity || listing.quantity > AUCTION_COMMAND_MAX_ITEMS)
	{
		errno = EOPNOTSUPP;
		return false;
	}
	if (!row(connection,
		 "SELECT COUNT(*) FROM auction_item_custody WHERE auction_id=" +
			 std::to_string(listing.auction_id) + " FOR UPDATE",
		 1, &values))
		return false;
	uint64_t count = 0;
	if (!u64(values[0], &count) || count != listing.quantity)
	{
		errno = ESTALE;
		return false;
	}
	listing.item_count = static_cast<uint16_t>(listing.quantity);
	before->items_before.resize(listing.item_count);
	for (size_t index = 0; index < listing.item_count; ++index)
	{
		if (!row(connection,
			 "SELECT item_uid,item_revision,vnum,COALESCE(claim_pid,0),"
			 "claimed_at IS NOT NULL FROM auction_item_custody WHERE auction_id=" +
				 std::to_string(listing.auction_id) +
				 " AND slot=" + std::to_string(index) + " FOR UPDATE",
			 5, &values))
			return false;
		uint64_t item_uid = 0, item_revision = 0, claim_pid = 0, claimed = 0;
		int64_t vnum = 0;
		if (!u64(values[0], &item_uid) || !u64(values[1], &item_revision) ||
		    !i64(values[2], &vnum) || vnum < 0 || vnum > INT32_MAX ||
		    !u64(values[3], &claim_pid) || claim_pid > UINT32_MAX ||
		    !u64(values[4], &claimed) || claimed > 1)
		{
			errno = EILSEQ;
			return false;
		}
		listing.items[index] = { item_uid,
					 item_revision,
					 static_cast<uint16_t>(index),
					 static_cast<int32_t>(vnum),
					 static_cast<uint32_t>(claim_pid),
					 claimed != 0 };
	}
	std::array<size_t, AUCTION_COMMAND_MAX_ITEMS> order = {};
	for (size_t index = 0; index < listing.item_count; ++index)
		order[index] = index;
	std::sort(order.begin(), order.begin() + listing.item_count, [&](size_t left, size_t right)
		  { return listing.items[left].uid < listing.items[right].uid; });
	for (size_t position = 0; position < listing.item_count; ++position)
	{
		const size_t index = order[position];
		const auto uid = listing.items[index].uid;
		if (!row(connection,
			 "SELECT root_item_uid,COALESCE(parent_item_uid,0),owner_type,owner_id,"
			 "owner_context_id,item_revision,vnum,state FROM item_current_owner "
			 "WHERE item_uid=" +
				 std::to_string(uid) + " FOR UPDATE",
			 8, &values))
			return false;
		uint64_t item[8] = {};
		int64_t vnum = 0;
		for (size_t field = 0; field < 8; ++field)
			if (field != 6 && !u64(values[field], &item[field]))
			{
				errno = EILSEQ;
				return false;
			}
		if (!i64(values[6], &vnum) || vnum != listing.items[index].vnum ||
		    item[2] > UINT8_MAX || item[7] > UINT8_MAX)
		{
			errno = EILSEQ;
			return false;
		}
		before->items_before[index] = { uid,
						{ { static_cast<item_owner_type>(item[2]), item[3],
						    item[4] },
						  item[0],
						  item[1],
						  item[5],
						  static_cast<item_custody_state>(item[7]) } };
		if (!row(connection,
			 "SELECT item_uid FROM item_current_owner WHERE root_item_uid=" +
				 std::to_string(uid) + " AND item_uid<>" + std::to_string(uid) +
				 " LIMIT 1 FOR UPDATE",
			 1, &values, true))
			return false;
		if (!values.empty())
		{
			errno = EOPNOTSUPP;
			return false;
		}
	}
	if (payload.action == auction_action::finalize && listing.winner_pid &&
	    !claim(connection, listing.seller_pid, &before->seller_claim_before,
		   &before->seller_claim_revision_before))
		return false;
	return true;
}

bool locked_after(MYSQL *connection, const auction_command_payload &payload,
		  auction_settlement_authority *state, const auction_command_result &result)
{
	std::vector<std::string> values;
	if (!row(connection,
		 "SELECT status+0,auction_revision FROM auctions WHERE id=" +
			 std::to_string(payload.auction_id) + " FOR UPDATE",
		 2, &values))
		return false;
	uint64_t status = 0, revision = 0;
	if (!u64(values[0], &status) || !u64(values[1], &revision) || status != result.status ||
	    revision != result.auction_revision)
	{
		errno = EILSEQ;
		return false;
	}
	const bool sale = payload.action == auction_action::finalize && state->listing.winner_pid;
	if (sale && !claim(connection, state->listing.seller_pid, &state->seller_claim_after,
			   &state->seller_claim_revision_after, false))
		return false;
	const uint32_t claimant = sale ? state->listing.winner_pid : state->listing.seller_pid;
	state->claim_pids_after.clear();
	for (size_t index = 0; index < state->listing.item_count; ++index)
	{
		if (!row(connection,
			 "SELECT COALESCE(claim_pid,0),claimed_at IS NOT NULL,item_revision "
			 "FROM auction_item_custody WHERE auction_id=" +
				 std::to_string(state->listing.auction_id) +
				 " AND slot=" + std::to_string(index) + " FOR UPDATE",
			 3, &values))
			return false;
		uint64_t pid = 0, claimed = 0, item_revision = 0;
		if (!u64(values[0], &pid) || !u64(values[1], &claimed) ||
		    !u64(values[2], &item_revision) || pid != claimant || claimed ||
		    item_revision != state->listing.items[index].revision)
		{
			errno = EILSEQ;
			return false;
		}
		state->claim_pids_after.push_back(static_cast<uint32_t>(pid));
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

unsigned int economic_sql_auction_settlement_lock(MYSQL *connection,
						  const critical_command &command,
						  economic_sql_auction_settlement_context *context)
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
		auction_settlement_listing listing;
		auction_settlement_accounts accounts;
		if (!identity(command, &intent, &payload, &listing, &accounts))
			return EPROTONOSUPPORT;
		economic_sql_auction_settlement_context candidate;
		std::vector<economic_sql_mapping_request> requests = {
			{ accounts.escrow, AUCTION_LOCATOR, listing.auction_id }
		};
		if (economic_account_key_valid(accounts.seller_claim))
			requests.push_back(
				{ accounts.seller_claim, CLAIM_LOCATOR, listing.seller_pid });
		if (payload.actor_pid)
		{
			if (!mapping_native_hint(connection, accounts.actor_bank.authority_id,
						 &candidate.actor_bank_id))
				return failure_code();
			requests.push_back(
				{ accounts.actor_wallet, PLAYER_LOCATOR, payload.actor_pid });
			requests.push_back(
				{ accounts.actor_bank, BANK_LOCATOR, candidate.actor_bank_id });
		}
		const auto error = economic_sql_lock_authority(connection,
							       intent.admission.metadata.lineage,
							       intent.admission.metadata.epoch,
							       requests, &candidate.authority);
		if (error)
			return error;
		// Existing mapped claim lifetimes require their actual native row, including zero.
		// Only an explicit AEC1 absence request may admit a missing endpoint.
		for (const auto &request : requests)
			if (request.account.kind == economic_account_kind::pending_claim)
			{
				std::vector<std::string> cells;
				if (!row(connection,
					 "SELECT pid FROM auction_money_pickups WHERE pid=" +
						 std::to_string(request.native_id) + " FOR UPDATE",
					 1, &cells))
					return failure_code();
				uint64_t native = 0;
				if (!u64(cells[0], &native) || native != request.native_id)
					return EILSEQ;
			}
		for (uint32_t pid : { accounts.absent_seller_pid })
			if (pid)
			{
				const auto absent_error =
					economic_sql_pending_claim_endpoint_lock_absent(
						connection, command, pid);
				if (absent_error)
					return absent_error;
			}
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

unsigned int economic_sql_auction_settlement_execute_and_record(
	MYSQL *connection, const critical_command &command,
	const economic_sql_auction_settlement_context &context, auction_command_result *result,
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
		economic_sql_auction_settlement_context active;
		const auto lock_error =
			economic_sql_auction_settlement_lock(connection, command, &active);
		if (lock_error)
			return lock_error;
		if (active.actor_bank_id != context.actor_bank_id ||
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
		auction_settlement_listing frozen;
		auction_settlement_accounts accounts;
		if (!identity(command, &intent, &payload, &frozen, &accounts))
			return EILSEQ;
		auction_settlement_authority before;
		before.epoch = active.authority.epoch;
		before.accounts = accounts;
		if (!locked_before(connection, payload, active.actor_bank_id, &before))
			return failure_code();
		critical_command projected = command;
		projected.schema_version = CRITICAL_COMMAND_SCHEMA_VERSION;
		projected.accounting_intent.clear();
		projected.publication_required = false;
		std::vector<uint8_t> expected;
		if (auction_settlement_accounting_intent(projected, before.epoch, before.listing,
							 accounts, &expected) !=
			    economic_accounting_error::ok ||
		    expected != command.accounting_intent)
			return ESTALE;
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
		if (!locked_after(connection, payload, &before, *result))
			return failure_code();
		if (accounts.absent_seller_pid)
		{
			const auto error = economic_sql_pending_claim_endpoint_create(
				connection, command, accounts.absent_seller_pid,
				&accounts.seller_claim);
			if (error)
				return error;
			before.accounts = accounts;
		}
		economic_accounting_plan plan;
		if (auction_settlement_accounting_plan(command, intent, before, *result, &plan) !=
			    economic_accounting_error::ok ||
		    !plan.children.empty() || !plan.item_events.empty())
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
		if (payload.action == auction_action::finalize && before.listing.winner_pid)
		{
			const int64_t proceeds =
				before.listing.current_price -
				(static_cast<__int128_t>(before.listing.current_price) *
				 payload.closing_fee_basis_points / 10000);
			if (proceeds > 0)
			{
				const auto staged = economic_sql_pending_claim_source_stage(
					connection, command.operation_id, 2, accounts.seller_claim,
					before.listing.seller_pid, static_cast<uint64_t>(proceeds));
				if (staged)
					return staged;
			}
		}
		const auto escrow_effect = std::find_if(
			plan.accounts.begin(), plan.accounts.end(), [&](const auto &entry)
			{ return economic_account_key_equal(entry.key, accounts.escrow); });
		if (escrow_effect == plan.accounts.end())
			return EILSEQ;
		if (escrow_effect->after == economic_coin_vector{})
		{
			const auto mapping = std::find_if(
				active.authority.mappings.begin(), active.authority.mappings.end(),
				[&](const auto &entry) {
					return economic_account_key_equal(entry.request.account,
									  accounts.escrow);
				});
			if (mapping == active.authority.mappings.end())
				return EILSEQ;
			const auto retired = economic_sql_retire_mapping(connection, *mapping,
									 command.operation_id);
			if (retired)
				return retired;
		}
		return 0;
	}
	catch (const std::bad_alloc &)
	{
		return ENOMEM;
	}
#endif
}
