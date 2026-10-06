#include "persistence/economic_sql_auction_bid_transaction.h"

#include "economy/auction_accounting.h"
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
#include <span>
#include <string>
#include <strings.h>
#include <vector>

namespace
{
constexpr uint16_t PLAYER_LOCATOR = 1;
constexpr uint16_t BANK_LOCATOR = 2;
constexpr uint16_t AUCTION_LOCATOR = 4;
constexpr uint16_t CLAIM_LOCATOR = 5;
constexpr size_t BID_FACT_BYTES = 124;

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

uint64_t little_u64(std::span<const uint8_t> bytes, size_t offset)
{
	uint64_t value = 0;
	for (size_t index = 0; index < 8; ++index)
		value |= static_cast<uint64_t>(bytes[offset + index]) << (index * 8);
	return value;
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
	      auction_command_payload *payload, auction_bid_accounting_listing *listing,
	      auction_bid_accounting_accounts *accounts)
{
	return auction_bid_accounting_decode(command, intent, payload, listing, accounts) ==
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

bool claim(MYSQL *connection, uint32_t pid, auction_bid_accounting_claim *state)
{
	std::vector<std::string> values;
	if (!row(connection,
		 "SELECT money,claim_revision FROM auction_money_pickups WHERE pid=" +
			 std::to_string(pid) + " FOR UPDATE",
		 2, &values, true))
		return false;
	if (values.empty())
	{
		*state = {};
		return true;
	}
	uint64_t money = 0;
	if (!u64(values[0], &money) || money > UINT_MAX || !u64(values[1], &state->revision))
	{
		errno = EILSEQ;
		return false;
	}
	state->money = static_cast<int64_t>(money);
	return true;
}

bool locked_before(MYSQL *connection, const auction_command_payload &payload, uint32_t bank_id,
		   auction_bid_accounting_authority *before)
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
	if (!u64(values[5], &before->balances_before.bank_revision))
	{
		errno = EILSEQ;
		return false;
	}
	if (!row(connection,
		 "SELECT seller_pid,winning_bidder_pid,status+0,custody_state,cur_price,"
		 "buy_price,auction_revision,HEX(listing_operation_id) FROM auctions WHERE id=" +
			 std::to_string(payload.auction_id) + " FOR UPDATE",
		 8, &values))
		return false;
	uint64_t parsed[7] = {};
	for (size_t index = 0; index < 7; ++index)
		if (!u64(values[index], &parsed[index]))
		{
			errno = EILSEQ;
			return false;
		}
	if (parsed[0] > UINT32_MAX || parsed[1] > UINT32_MAX || parsed[2] > UINT32_MAX ||
	    parsed[3] > UINT32_MAX || parsed[4] > INT64_MAX || parsed[5] > INT64_MAX)
	{
		errno = EILSEQ;
		return false;
	}
	auto &listing = before->listing;
	listing.auction_id = payload.auction_id;
	listing.seller_pid = static_cast<uint32_t>(parsed[0]);
	listing.winning_bidder_pid = static_cast<uint32_t>(parsed[1]);
	listing.status = static_cast<uint32_t>(parsed[2]);
	listing.custody_state = static_cast<uint32_t>(parsed[3]);
	listing.current_price = static_cast<int64_t>(parsed[4]);
	listing.buy_price = static_cast<int64_t>(parsed[5]);
	listing.revision = parsed[6];
	if (!hex_id(values[7], &listing.listing_operation))
	{
		errno = EILSEQ;
		return false;
	}
	if (listing.winning_bidder_pid)
	{
		if (!row(connection,
			 "SELECT HEX(operation_id),actor_pid,final_price FROM auction_ledger "
			 "WHERE auction_id=" +
				 std::to_string(listing.auction_id) + " AND auction_revision=" +
				 std::to_string(listing.revision) + " AND event_type=2 FOR UPDATE",
			 3, &values))
			return false;
		uint64_t bidder = 0, price = 0;
		if (!hex_id(values[0], &listing.previous_bid_operation) ||
		    !u64(values[1], &bidder) || bidder != listing.winning_bidder_pid ||
		    !u64(values[2], &price) ||
		    price != static_cast<uint64_t>(listing.current_price))
		{
			errno = EILSEQ;
			return false;
		}
	}
	const bool outbid = listing.winning_bidder_pid &&
			    listing.winning_bidder_pid != payload.actor_pid;
	const bool sold = listing.buy_price > 0 && payload.value >= listing.buy_price;
	return claim(connection, payload.actor_pid, &before->bidder_claim_before) &&
	       (!outbid ||
		claim(connection, listing.winning_bidder_pid, &before->previous_claim_before)) &&
	       (!sold || claim(connection, listing.seller_pid, &before->seller_claim_before));
}

bool insert_operation(MYSQL *connection, const critical_command &command,
		      const economic_frozen_intent &intent, const economic_accounting_plan *plan,
		      unsigned int result_code)
{
	const auto &meta = intent.admission.metadata;
	std::vector<uint8_t> encoded_plan;
	economic_digest plan_digest = {}, intent_digest = {};
	std::array<uint8_t, ECONOMIC_SOURCE_EVENT_BYTES> source = {};
	std::string realized_price_sql = "NULL";
	if (plan && meta.reason == economic_reason::auction_bid)
	{
		auction_command_payload payload = {};
		const auto facts = std::span<const uint8_t>(intent.admission.facts);
		if ((facts.size() != BID_FACT_BYTES && facts.size() != BID_FACT_BYTES + 16) ||
		    !auction_command_decode_payload(command, &payload) || payload.value <= 0)
		{
			errno = EILSEQ;
			return false;
		}
		uint64_t realized_price = static_cast<uint64_t>(payload.value);
		const auto buy_price = little_u64(facts, 76);
		if (buy_price && realized_price >= buy_price)
			realized_price = buy_price;
		if (!realized_price || realized_price > UINT_MAX)
		{
			errno = EILSEQ;
			return false;
		}
		realized_price_sql = std::to_string(realized_price);
	}
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
		"actor_kind,actor_id,reason,source_event,realized_price_copper,intent_digest,domain_digest,"
		"plan_digest,"
		"canonical_intent,canonical_plan,outcome,result_code,account_count,posting_count,"
		"child_count,item_event_count,before_witness_count,after_witness_count) VALUES(" +
		id(command.operation_id) + "," + id(meta.lineage) + "," + id(meta.epoch) + "," +
		id(meta.original_operation_id) + "," + std::to_string(meta.version) + "," +
		std::to_string(meta.writer_id) + "," + std::to_string(meta.policy_version) + "," +
		std::to_string(meta.compiler_version) + "," +
		std::to_string(static_cast<uint8_t>(meta.actor_kind)) + "," +
		std::to_string(meta.actor_id) + "," +
		std::to_string(static_cast<uint16_t>(meta.reason)) + "," + hex(source) + "," +
		realized_price_sql + "," + hex(intent_digest) + "," + hex(intent.domain_digest) +
		"," + (plan ? hex(plan_digest) : "NULL") + "," + hex(command.accounting_intent) +
		"," + (plan ? hex(encoded_plan) : "NULL") + "," +
		(plan ? "1,0," : "2," + std::to_string(result_code) + ",") +
		std::to_string(plan ? plan->accounts.size() : 0) + "," +
		std::to_string(plan ? plan->postings.size() : 0) + ",0,0,0,0)";
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

unsigned int economic_sql_auction_bid_lock(MYSQL *connection, const critical_command &command,
					   economic_sql_auction_bid_context *context)
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
		auction_bid_accounting_listing listing;
		auction_bid_accounting_accounts accounts;
		if (!identity(command, &intent, &payload, &listing, &accounts))
			return EPROTONOSUPPORT;
		economic_sql_auction_bid_context candidate;
		if (!mapping_native_hint(connection, accounts.bank.authority_id,
					 &candidate.bank_id))
			return failure_code();
		std::vector<economic_sql_mapping_request> requests = {
			{ accounts.wallet, PLAYER_LOCATOR, payload.actor_pid },
			{ accounts.bank, BANK_LOCATOR, candidate.bank_id },
			{ accounts.escrow, AUCTION_LOCATOR, listing.auction_id }
		};
		if (economic_account_key_valid(accounts.bidder_claim))
			requests.push_back(
				{ accounts.bidder_claim, CLAIM_LOCATOR, payload.actor_pid });
		if (economic_account_key_valid(accounts.previous_claim))
			requests.push_back({ accounts.previous_claim, CLAIM_LOCATOR,
					     listing.winning_bidder_pid });
		if (economic_account_key_valid(accounts.seller_claim))
			requests.push_back(
				{ accounts.seller_claim, CLAIM_LOCATOR, listing.seller_pid });
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
		for (uint32_t pid : { accounts.absent_bidder_pid, accounts.absent_previous_pid,
				      accounts.absent_seller_pid })
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

unsigned int
economic_sql_auction_bid_execute_and_record(MYSQL *connection, const critical_command &command,
					    const economic_sql_auction_bid_context &context,
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
		economic_sql_auction_bid_context active;
		const auto lock_error = economic_sql_auction_bid_lock(connection, command, &active);
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
		auction_bid_accounting_listing frozen;
		auction_bid_accounting_accounts accounts;
		if (!identity(command, &intent, &payload, &frozen, &accounts))
			return EILSEQ;
		auction_bid_accounting_authority before;
		before.epoch = active.authority.epoch;
		before.accounts = accounts;
		if (!locked_before(connection, payload, active.bank_id, &before))
			return failure_code();
		critical_command projected = command;
		projected.schema_version = CRITICAL_COMMAND_SCHEMA_VERSION;
		projected.accounting_intent.clear();
		projected.publication_required = false;
		std::vector<uint8_t> expected;
		if (auction_bid_accounting_intent(projected, before.epoch, before.listing, accounts,
						  &expected) != economic_accounting_error::ok ||
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
		if (accounts.absent_previous_pid)
		{
			const auto error = economic_sql_pending_claim_endpoint_create(
				connection, command, accounts.absent_previous_pid,
				&accounts.previous_claim);
			if (error)
				return error;
		}
		if (accounts.absent_seller_pid)
		{
			const auto error = economic_sql_pending_claim_endpoint_create(
				connection, command, accounts.absent_seller_pid,
				&accounts.seller_claim);
			if (error)
				return error;
		}
		before.accounts = accounts;
		economic_accounting_plan plan;
		if (auction_bid_accounting_plan(command, intent, before, *result, &plan) !=
			    economic_accounting_error::ok ||
		    !plan.children.empty() || !plan.item_events.empty())
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
		if (result->claim_credit_used > 0)
		{
			const auto consumed = economic_sql_pending_claim_source_consume(
				connection, command.operation_id, accounts.bidder_claim,
				payload.actor_pid,
				static_cast<uint64_t>(before.bidder_claim_before.money),
				static_cast<uint64_t>(result->claim_credit_used));
			if (consumed)
				return consumed;
		}
		const bool outbid = before.listing.winning_bidder_pid &&
				    before.listing.winning_bidder_pid != payload.actor_pid;
		if (outbid)
		{
			const auto staged = economic_sql_pending_claim_source_stage(
				connection, command.operation_id, 1, accounts.previous_claim,
				before.listing.winning_bidder_pid,
				static_cast<uint64_t>(before.listing.current_price));
			if (staged)
				return staged;
		}
		if (result->event_type == auction_event_type::sold)
		{
			const int64_t proceeds = result->final_price -
						 (static_cast<__int128_t>(result->final_price) *
						  payload.closing_fee_basis_points / 10000);
			if (proceeds > 0)
			{
				const auto staged = economic_sql_pending_claim_source_stage(
					connection, command.operation_id, 2, accounts.seller_claim,
					before.listing.seller_pid, static_cast<uint64_t>(proceeds));
				if (staged)
					return staged;
			}
			const auto effect = std::find_if(
				plan.accounts.begin(), plan.accounts.end(), [&](const auto &entry)
				{ return economic_account_key_equal(entry.key, accounts.escrow); });
			const auto mapping = std::find_if(
				active.authority.mappings.begin(), active.authority.mappings.end(),
				[&](const auto &entry) {
					return economic_account_key_equal(entry.request.account,
									  accounts.escrow);
				});
			if (effect == plan.accounts.end() ||
			    effect->after != economic_coin_vector{} ||
			    mapping == active.authority.mappings.end())
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
