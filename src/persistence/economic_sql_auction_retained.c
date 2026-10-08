#include "persistence/economic_sql_auction_retained.h"
#include "persistence/economic_sql_auction_source_claim.h"
#include "economy/auction_accounting.h"
#include "economy/auction_listing_accounting.h"
#include "economy/auction_item_claim_accounting.h"
#include "economy/auction_money_claim_accounting.h"
#include "economy/auction_settlement_accounting.h"
#include "economy/auction_repository.h"
#include "economy/auction_native_command_context.h"
#include <unordered_set>
#include "persistence/economic_sql_baseline_transaction.h"
#include "persistence/economic_sql_pending_claim_source.h"
#include <cerrno>
#ifndef __NO_MYSQL__
#include <openssl/sha.h>
#include <algorithm>
#include <array>
#include <charconv>
#include <climits>
#include <memory>
#include <new>
#include <string>
#include <type_traits>
#include <vector>
namespace
{
bool accounting_ok(economic_accounting_error error)
{
	if (error == economic_accounting_error::ok)
		return true;
	errno = error == economic_accounting_error::capacity ? ENOMEM : EILSEQ;
	return false;
}
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

bool execute(MYSQL *connection, const std::string &sql)
{
	if (!mysql_real_query(connection, sql.data(), sql.size()))
		return true;
	errno = mysql_errno(connection) ? static_cast<int>(mysql_errno(connection)) : EIO;
	return false;
}

bool row(MYSQL *connection, const std::string &sql, size_t count, std::vector<std::string> *values)
{
	if (!execute(connection, sql))
		return false;
	std::unique_ptr<MYSQL_RES, decltype(&mysql_free_result)> rows(
		mysql_store_result(connection), mysql_free_result);
	if (!rows || mysql_num_rows(rows.get()) != 1 || mysql_num_fields(rows.get()) != count)
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
	values->reserve(count);
	for (size_t index = 0; index < count; ++index)
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

bool u64(const std::string &text, uint64_t *number)
{
	const auto parsed = std::from_chars(text.data(), text.data() + text.size(), *number);
	return !text.empty() && parsed.ec == std::errc{} && parsed.ptr == text.data() + text.size();
}

bool exact_count(MYSQL *connection, const std::string &table, const std::string &predicate,
		 uint64_t expected)
{
	std::vector<std::string> values;
	uint64_t actual = 0;
	if (!row(connection, "SELECT COUNT(*) FROM " + table + " WHERE " + predicate, 1, &values))
		return false;
	if (!u64(values[0], &actual) || actual != expected)
	{
		errno = EILSEQ;
		return false;
	}
	return true;
}

// Compare every durable normalized line with the original immutable EAP.
bool normalized_financial_rows(MYSQL *connection, const critical_operation_id &operation,
			       const economic_accounting_plan &plan)
{
	const auto scope = "operation_id=" + hex(operation.bytes);
	if (!exact_count(connection, "economic_accounting_account_effect", scope,
			 plan.accounts.size()) ||
	    !exact_count(connection, "economic_accounting_coin_posting", scope,
			 plan.postings.size()))
		return false;
	static constexpr std::array<const char *, 4> denominations = { "copper", "silver", "gold",
								       "platinum" };
	for (size_t index = 0; index < plan.accounts.size(); ++index)
	{
		const auto &effect = plan.accounts[index];
		std::array<uint8_t, ECONOMIC_ACCOUNT_KEY_BYTES> key{};
		if (economic_account_key_encode(effect.key, &key) != economic_accounting_error::ok)
		{
			errno = EILSEQ;
			return false;
		}
		auto predicate = scope + " AND account_index=" + std::to_string(index) +
				 " AND account_key=" + hex(key);
		for (size_t denomination = 0; denomination < denominations.size(); ++denomination)
			predicate += " AND before_" + std::string(denominations[denomination]) +
				     "=" + std::to_string(effect.before[denomination]) +
				     " AND after_" + denominations[denomination] + "=" +
				     std::to_string(effect.after[denomination]);
		predicate += " AND before_revision=" + std::to_string(effect.before_revision) +
			     " AND after_revision=" + std::to_string(effect.after_revision);
		if (!exact_count(connection, "economic_accounting_account_effect", predicate, 1))
			return false;
	}
	for (size_t index = 0; index < plan.postings.size(); ++index)
	{
		const auto &posting = plan.postings[index];
		auto predicate = scope + " AND line_index=" + std::to_string(index) +
				 " AND event_index=" + std::to_string(posting.event_index) +
				 " AND account_index=" + std::to_string(posting.account_index) +
				 " AND child_index=" + std::to_string(posting.child_index) +
				 " AND copper_value=" + std::to_string(posting.copper);
		for (size_t denomination = 0; denomination < denominations.size(); ++denomination)
			predicate += " AND delta_" + std::string(denominations[denomination]) +
				     "=" + std::to_string(posting.delta[denomination]);
		if (!exact_count(connection, "economic_accounting_coin_posting", predicate, 1))
			return false;
	}
	return true;
}

std::span<const uint8_t> bytes(const std::string &s)
{
	return { reinterpret_cast<const uint8_t *>(s.data()), s.size() };
}
const economic_account_effect *effect(const economic_accounting_plan &p,
				      const economic_account_key &k)
{
	auto i = std::find_if(p.accounts.begin(), p.accounts.end(),
			      [&](const auto &e) { return economic_account_key_equal(e.key, k); });
	return i == p.accounts.end() ? nullptr : &*i;
}
bool mapping(MYSQL *db, const economic_account_key &k, unsigned locator, uint64_t native,
	     uint64_t *resolved = nullptr, const critical_operation_id *creator = nullptr)
{
	if (!economic_account_key_valid(k))
	{
		errno = EILSEQ;
		return false;
	}
	std::string predicate = "mapping_id=" + std::to_string(k.authority_id) +
				" AND lineage=" + hex(k.lineage.bytes) + " AND account_kind=" +
				std::to_string(static_cast<unsigned>(k.kind)) +
				" AND context_id=" + std::to_string(k.context_id) +
				" AND backend_kind=1 AND locator_kind=" + std::to_string(locator);
	if (native)
		predicate += " AND native_id=" + std::to_string(native);
	if (creator)
		predicate += " AND creating_operation_id=" + hex(creator->bytes);
	std::vector<std::string> cells;
	uint64_t value = 0;
	if (!row(db,
		 "SELECT native_id,creating_operation_id FROM economic_account_mapping WHERE " +
			 predicate + " LOCK IN SHARE MODE",
		 2, &cells))
		return false;
	if (!u64(cells[0], &value) || !value || value > UINT32_MAX)
	{
		errno = EILSEQ;
		return false;
	}
	if (resolved)
		*resolved = value;
	// Bind birth to its immutable successful accounting receipt, regardless of retirement.
	const bool bound = exact_count(
		db,
		"economic_account_mapping m JOIN critical_operation_inbox i ON i.operation_id=m.creating_operation_id",
		"m.mapping_id=" + std::to_string(k.authority_id) +
			" AND i.status=1 AND i.result_code=0 AND i.failure_stage=0 AND i.committed_at IS NOT NULL AND (EXISTS (SELECT 1 FROM economic_accounting_operation o WHERE o.operation_id=i.operation_id AND o.outcome=1 AND o.result_code=0 AND o.lineage=m.lineage) OR EXISTS (SELECT 1 FROM economic_sql_lifecycle_installation l WHERE l.operation_id=i.operation_id AND l.lineage=m.lineage AND EXISTS (SELECT 1 FROM economic_baseline_reservation r WHERE r.operation_id=l.baseline_operation_id AND r.lineage=m.lineage AND r.identity_kind=1 AND r.identity_id=m.mapping_id)))",
		1);
	if (!bound)
		return false;
	if (k.kind == economic_account_kind::pending_claim)
	{
		if (cells[1].size() != 16)
		{
			errno = EILSEQ;
			return false;
		}
		critical_operation_id original_creator{};
		std::copy(bytes(cells[1]).begin(), bytes(cells[1]).end(),
			  original_creator.bytes.begin());
		std::vector<std::string> installations;
		uint64_t installed = 0;
		if (!row(db,
			 "SELECT COUNT(*) FROM economic_sql_lifecycle_installation WHERE operation_id=" +
				 hex(original_creator.bytes) +
				 " AND lineage=" + hex(k.lineage.bytes),
			 1, &installations) ||
		    !u64(installations[0], &installed) || installed > 1)
		{
			if (!errno)
				errno = EILSEQ;
			return false;
		}
		if (!installed)
		{
			const auto error =
				economic_sql_pending_claim_endpoint_verify_retained_creator(
					db, original_creator, k, static_cast<uint32_t>(value));
			if (error)
			{
				errno = static_cast<int>(error);
				return false;
			}
		}
	}
	return true;
}
bool resolve_absent(MYSQL *db, const critical_command &cmd, const economic_accounting_plan &plan,
		    uint32_t pid, economic_account_key *key)
{
	if (!pid)
		return true;
	std::vector<std::string> cells;
	uint64_t number = 0;
	if (!row(db,
		 "SELECT mapping_id FROM economic_account_mapping WHERE lineage=" +
			 hex(plan.metadata.lineage.bytes) +
			 " AND account_kind=5 AND context_id=0 AND backend_kind=1 AND locator_kind=5 AND native_id=" +
			 std::to_string(pid) + " AND creating_operation_id=" +
			 hex(cmd.operation_id.bytes) + " LOCK IN SHARE MODE",
		 1, &cells))
		return false;
	if (!u64(cells[0], &number) || !number)
	{
		errno = EILSEQ;
		return false;
	}
	*key = { plan.metadata.lineage, economic_account_kind::pending_claim, number, 0 };
	const auto *e = effect(plan, *key);
	if (!e || e->before != economic_coin_vector{} || e->before_revision)
	{
		errno = EILSEQ;
		return false;
	}
	return mapping(db, *key, 5, pid, nullptr, &cmd.operation_id);
}
bool claim_before(const economic_accounting_plan &p, const economic_account_key &k,
		  auction_bid_accounting_claim *claim, bool required)
{
	const auto *e = effect(p, k);
	if (!e)
	{
		if (required)
		{
			errno = EILSEQ;
			return false;
		}
		*claim = {};
		return true;
	}
	if (e->before[0] < 0 || e->before[1] || e->before[2] || e->before[3])
	{
		errno = EILSEQ;
		return false;
	}
	*claim = { e->before[0], e->before_revision };
	return true;
}
bool source(MYSQL *db, const critical_command &cmd, const economic_account_key &k, uint32_t pid,
	    unsigned slot, int64_t amount)
{
	if (!amount)
		return true;
	return amount > 0 &&
	       exact_count(db, "economic_pending_claim_source",
			   "source_operation_id=" + hex(cmd.operation_id.bytes) +
				   " AND source_slot=" + std::to_string(slot) +
				   " AND lineage=" + hex(k.lineage.bytes) +
				   " AND claim_mapping_id=" + std::to_string(k.authority_id) +
				   " AND beneficiary_pid=" + std::to_string(pid) +
				   " AND amount=" + std::to_string(amount),
			   1);
}
bool origin(MYSQL *db, const critical_operation_id &operation, uint32_t auction, uint32_t event,
	    uint32_t actor, uint64_t revision, int64_t price)
{
	return exact_count(
		db,
		"auction_ledger l JOIN critical_operation_inbox i ON i.operation_id=l.operation_id",
		"l.operation_id=" + hex(operation.bytes) + " AND l.auction_id=" +
			std::to_string(auction) + " AND l.event_type=" + std::to_string(event) +
			" AND l.actor_pid=" + std::to_string(actor) + " AND l.auction_revision=" +
			std::to_string(revision) + " AND l.final_price=" + std::to_string(price) +
			" AND i.status=1 AND i.result_code=0 AND i.failure_stage=0 AND i.committed_at IS NOT NULL",
		1);
}
// Route original baseline receipts before taking participant or mapping locks.
// The nullable timestamp/marker is historical evidence, never a recapture.
bool baseline_births(MYSQL *db, const economic_frozen_intent &intent,
		     const std::vector<economic_account_key> &keys)
{
	std::string identities;
	for (const auto &key : keys)
		if (economic_account_key_valid(key))
		{
			if (!identities.empty())
				identities += ",";
			identities += std::to_string(key.authority_id);
		}
	if (identities.empty())
		return true;
	std::string query =
		"SELECT DISTINCT w.operation_id,w.canonical_witness,COALESCE(w.command_accepted_at_usec,0),COALESCE(w.claim_origin_version,0),w.book_revision FROM economic_baseline_reservation r JOIN economic_baseline_witness w ON w.operation_id=r.operation_id JOIN economic_account_mapping m ON m.mapping_id=r.identity_id AND m.lineage=r.lineage JOIN economic_sql_lifecycle_installation l ON l.operation_id=m.creating_operation_id AND l.baseline_operation_id=r.operation_id WHERE r.lineage=" +
		hex(intent.admission.metadata.lineage.bytes) +
		" AND r.identity_kind=1 AND r.identity_id IN (" + identities +
		") ORDER BY w.operation_id";
	if (!execute(db, query))
		return false;
	std::unique_ptr<MYSQL_RES, decltype(&mysql_free_result)> rows(mysql_store_result(db),
								      mysql_free_result);
	if (!rows || mysql_num_fields(rows.get()) != 5 || mysql_num_rows(rows.get()) > keys.size())
	{
		errno = EILSEQ;
		return false;
	}
	std::vector<std::array<std::string, 5>> stored;
	for (MYSQL_ROW cells; (cells = mysql_fetch_row(rows.get()));)
	{
		const auto *lengths = mysql_fetch_lengths(rows.get());
		if (!lengths)
		{
			errno = EILSEQ;
			return false;
		}
		std::array<std::string, 5> value;
		for (size_t i = 0; i < 5; ++i)
		{
			if (!cells[i])
			{
				errno = EILSEQ;
				return false;
			}
			value[i].assign(cells[i], lengths[i]);
		}
		stored.push_back(std::move(value));
	}
	rows.reset();
	for (const auto &value : stored)
	{
		uint64_t accepted = 0, marker = 0, revision = 0;
		if (value[0].size() != 16 || !u64(value[2], &accepted) || !u64(value[3], &marker) ||
		    marker > 1 || !u64(value[4], &revision) || !revision)
		{
			errno = EILSEQ;
			return false;
		}
		if (marker && !accepted)
		{
			errno = EILSEQ;
			return false;
		}
		critical_operation_id operation{};
		std::copy(bytes(value[0]).begin(), bytes(value[0]).end(), operation.bytes.begin());
		std::optional<economic_prepared_baseline> prepared;
		const auto decoded = economic_baseline_decode(bytes(value[1]), &prepared);
		if (decoded != economic_accounting_error::ok || !prepared)
		{
			errno = decoded == economic_accounting_error::capacity ? ENOMEM : EILSEQ;
			return false;
		}
		if (prepared->witness().lineage.bytes != intent.admission.metadata.lineage.bytes)
		{
			errno = EILSEQ;
			return false;
		}
		if (accepted)
		{
			critical_command original;
			const auto built =
				economic_baseline_command_build(*prepared, accepted, &original);
			if (built != economic_accounting_error::ok ||
			    original.operation_id.bytes != operation.bytes)
			{
				errno = built == economic_accounting_error::capacity ? ENOMEM :
										       EILSEQ;
				return false;
			}
			uint64_t actual = 0;
			const auto error = economic_sql_baseline_verify_retained_in_transaction(
				db, original, &actual);
			if (error || actual != revision)
			{
				errno = error ? static_cast<int>(error) : EILSEQ;
				return false;
			}
		}
		else
		{
			// Historical NULL rows require their actual known canonical evidence.
			// A zero/guessed accepted time is never passed to command_build.
			const auto error =
				economic_sql_baseline_verify_known_retained_in_transaction(
					db, operation, nullptr);
			if (error)
			{
				errno = static_cast<int>(error);
				return false;
			}
		}
	}
	return true;
}
bool original_consumption(MYSQL *db, const critical_command &cmd, const economic_account_key &key,
			  uint32_t pid, uint64_t amount)
{
	const auto scope = "spending_operation_id=" + hex(cmd.operation_id.bytes);
	if (!amount)
		return exact_count(db, "economic_pending_claim_consumption", scope, 0) &&
		       exact_count(db, "economic_pending_claim_source",
				   "claim_operation_id=" + hex(cmd.operation_id.bytes), 0);
	const auto valid = "s.lineage=" + hex(key.lineage.bytes) +
			   " AND s.claim_mapping_id=" + std::to_string(key.authority_id) +
			   " AND s.beneficiary_pid=" + std::to_string(pid);
	std::vector<std::string> value;
	uint64_t consumed = 0, legacy = 0, invalid = 0;
	if (!row(db,
		 "SELECT COALESCE(SUM(c.amount),0),COALESCE(SUM(NOT (" + valid +
			 " AND c.amount>0 AND c.amount<=s.amount AND (s.claim_operation_id IS NULL OR s.claim_operation_id<>c.spending_operation_id))),0) FROM economic_pending_claim_consumption c JOIN economic_pending_claim_source s ON s.source_operation_id=c.source_operation_id AND s.source_slot=c.source_slot WHERE c." +
			 scope,
		 2, &value) ||
	    !u64(value[0], &consumed) || !u64(value[1], &invalid) || invalid)
	{
		if (!errno)
			errno = EILSEQ;
		return false;
	}
	if (!row(db,
		 "SELECT COALESCE(SUM(s.amount),0),COALESCE(SUM(NOT (" + valid +
			 ")),0) FROM economic_pending_claim_source s WHERE s.claim_operation_id=" +
			 hex(cmd.operation_id.bytes),
		 2, &value) ||
	    !u64(value[0], &legacy) || !u64(value[1], &invalid) || invalid || consumed > amount ||
	    legacy != amount - consumed)
	{
		if (!errno)
			errno = EILSEQ;
		return false;
	}
	return true;
}
bool original_listing_selected(MYSQL *, const critical_command &, const economic_frozen_intent *,
			       const auction_item_claim_state &, const auction_command_payload &,
			       const critical_operation_id &, std::vector<player_item_snapshot> *,
			       bool historical = false, bool whole_listing = false);
bool native_retained_known_root(MYSQL *, const economic_frozen_intent &,
				const auction_command_result &);
bool known_terminal_creator(MYSQL *, const critical_operation_id &, economic_frozen_intent *,
			    economic_accounting_plan *, auction_command_result *);
bool native_retained_selected(MYSQL *, const critical_command &, const economic_frozen_intent &,
			      const auction_item_claim_state &, const auction_command_payload &,
			      const economic_accounting_plan &,
			      std::vector<player_item_snapshot> *);
bool native_retained_auction_forest(MYSQL *, const critical_command &,
				    const economic_frozen_intent &,
				    const auction_settlement_listing &,
				    const economic_accounting_plan &);

bool regenerate(MYSQL *db, const critical_command &cmd, const economic_frozen_intent &intent,
		const economic_accounting_plan &plan, const auction_command_result &result,
		economic_accounting_plan *expected)
{
	if (!exact_count(
		    db,
		    "auction_ledger l JOIN critical_operation_inbox i ON i.operation_id=l.operation_id",
		    "l.operation_id=" + hex(intent.admission.metadata.original_operation_id.bytes) +
			    " AND l.auction_id=" + std::to_string(result.auction_id) +
			    " AND l.event_type=1 AND l.actor_pid=" +
			    std::to_string(result.seller_pid) +
			    " AND i.status=1 AND i.result_code=0 AND i.failure_stage=0 AND i.committed_at IS NOT NULL",
		    1))
		return false;
	auction_command_payload payload{};
	economic_frozen_intent decoded;
	auction_bid_accounting_listing bid;
	auction_bid_accounting_accounts keys;
	if (auction_bid_accounting_decode(cmd, &decoded, &payload, &bid, &keys) ==
	    economic_accounting_error::ok)
	{
		if (!mapping(db, keys.wallet, 1, payload.actor_pid) ||
		    !mapping(db, keys.bank, 2, 0) || !mapping(db, keys.escrow, 4, bid.auction_id))
			return false;
		if (!resolve_absent(db, cmd, plan, keys.absent_previous_pid,
				    &keys.previous_claim) ||
		    !resolve_absent(db, cmd, plan, keys.absent_seller_pid, &keys.seller_claim))
			return false;
		for (auto pair : { std::pair{ keys.bidder_claim, payload.actor_pid },
				   std::pair{ keys.previous_claim, bid.winning_bidder_pid },
				   std::pair{ keys.seller_claim, bid.seller_pid } })
			if (economic_account_key_valid(pair.first) &&
			    !mapping(db, pair.first, 5, pair.second))
				return false;
		auction_bid_accounting_authority authority;
		authority.epoch = intent.admission.metadata.epoch;
		authority.listing = bid;
		if (cmd.payload_version == AUCTION_NATIVE_COMMAND_PAYLOAD_VERSION)
		{
			auction_settlement_listing source;
			source.auction_id = bid.auction_id;
			source.seller_pid = bid.seller_pid;
			source.listing_operation = bid.listing_operation;
			if (!native_retained_auction_forest(db, cmd, intent, source, plan))
				return false;
		}
		authority.accounts = keys;
		const auto *wallet = effect(plan, keys.wallet);
		if (!wallet)
		{
			errno = EILSEQ;
			return false;
		}
		authority.balances_before.wallet.amount = wallet->before;
		authority.balances_before.wallet_revision = wallet->before_revision;
		authority.balances_before.bank = result.bank;
		authority.balances_before.bank_revision = payload.expected_bank_revision;
		const bool outbid = bid.winning_bidder_pid &&
				    bid.winning_bidder_pid != payload.actor_pid;
		const bool sold = bid.buy_price > 0 && payload.value >= bid.buy_price;
		if (!claim_before(plan, keys.bidder_claim, &authority.bidder_claim_before,
				  result.claim_credit_used != 0) ||
		    !claim_before(plan, keys.previous_claim, &authority.previous_claim_before,
				  outbid) ||
		    !claim_before(plan, keys.seller_claim, &authority.seller_claim_before, sold))
			return false;
		auto error = auction_bid_accounting_plan(cmd, intent, authority, result, expected);
		if (error != economic_accounting_error::ok)
		{
			errno = error == economic_accounting_error::capacity ? ENOMEM : EILSEQ;
			return false;
		}
		if (!original_consumption(db, cmd, keys.bidder_claim, payload.actor_pid,
					  static_cast<uint64_t>(result.claim_credit_used)))
			return false;
		unsigned count = 0;
		if (outbid)
		{
			if (!source(db, cmd, keys.previous_claim, bid.winning_bidder_pid, 1,
				    bid.current_price))
				return false;
			++count;
		}
		if (sold)
		{
			int64_t amount =
				result.final_price -
				static_cast<int64_t>(static_cast<__int128_t>(result.final_price) *
						     payload.closing_fee_basis_points / 10000);
			if (!source(db, cmd, keys.seller_claim, bid.seller_pid, 2, amount))
				return false;
			count += amount > 0;
		}
		if (!exact_count(db, "economic_pending_claim_source",
				 "source_operation_id=" + hex(cmd.operation_id.bytes), count))
			return false;
		if (bid.winning_bidder_pid &&
		    !origin(db, bid.previous_bid_operation, bid.auction_id, 2,
			    bid.winning_bidder_pid, bid.revision, bid.current_price))
			return false;
	}
	else
	{
		auction_settlement_listing listing;
		auction_settlement_accounts accounts;
		if (auction_settlement_accounting_decode(cmd, &decoded, &payload, &listing,
							 &accounts) !=
		    economic_accounting_error::ok)
		{
			errno = EPROTONOSUPPORT;
			return false;
		}
		if (!mapping(db, accounts.escrow, 4, listing.auction_id) ||
		    !resolve_absent(db, cmd, plan, accounts.absent_seller_pid,
				    &accounts.seller_claim))
			return false;
		if (payload.actor_pid &&
		    (!mapping(db, accounts.actor_wallet, 1, payload.actor_pid) ||
		     !mapping(db, accounts.actor_bank, 2, 0)))
			return false;
		auction_settlement_authority authority;
		authority.epoch = intent.admission.metadata.epoch;
		authority.listing = listing;
		authority.accounts = accounts;
		authority.actor_balances_before.wallet = result.wallet;
		authority.actor_balances_before.bank = result.bank;
		authority.actor_balances_before.wallet_revision = result.wallet_revision;
		authority.actor_balances_before.bank_revision = result.bank_revision;
		const bool sale = payload.action == auction_action::finalize && listing.winner_pid;
		if (sale)
		{
			const auto *e = effect(plan, accounts.seller_claim);
			if (!e || !mapping(db, accounts.seller_claim, 5, listing.seller_pid))
			{
				errno = EILSEQ;
				return false;
			}
			authority.seller_claim_before = e->before[0];
			authority.seller_claim_revision_before = e->before_revision;
			authority.seller_claim_after = e->after[0];
			authority.seller_claim_revision_after = e->after_revision;
			const int64_t amount =
				listing.current_price -
				static_cast<int64_t>(
					static_cast<__int128_t>(listing.current_price) *
					payload.closing_fee_basis_points / 10000);
			if (!source(db, cmd, accounts.seller_claim, listing.seller_pid, 2,
				    amount) ||
			    !exact_count(db, "economic_pending_claim_source",
					 "source_operation_id=" + hex(cmd.operation_id.bytes),
					 amount > 0))
				return false;
		}
		else if (!exact_count(db, "economic_pending_claim_source",
				      "source_operation_id=" + hex(cmd.operation_id.bytes), 0))
			return false;
		for (size_t i = 0; i < listing.item_count; ++i)
		{
			auto &item = listing.items[i];
			authority.items_before.push_back(
				{ item.uid,
				  { { item_owner_type::auction, listing.auction_id, 0 },
				    item.uid,
				    0,
				    item.revision,
				    item_custody_state::active } });
			authority.claim_pids_after.push_back(sale ? listing.winner_pid :
								    listing.seller_pid);
		}
		if (cmd.payload_version == AUCTION_NATIVE_COMMAND_PAYLOAD_VERSION &&
		    !native_retained_auction_forest(db, cmd, intent, listing, plan))
			return false;
		auto error = auction_settlement_accounting_plan(cmd, intent, authority, result,
								expected);
		if (error != economic_accounting_error::ok)
		{
			errno = error == economic_accounting_error::capacity ? ENOMEM : EILSEQ;
			return false;
		}
		if (!original_consumption(db, cmd, {}, 0, 0))
			return false;
		if (listing.winner_pid &&
		    !origin(db, listing.winning_bid_operation, listing.auction_id, 2,
			    listing.winner_pid, listing.revision, listing.current_price))
			return false;
	}
	std::vector<uint8_t> encoded, actual;
	if (!accounting_ok(economic_plan_encode(*expected, &encoded)) ||
	    !accounting_ok(economic_plan_encode(plan, &actual)))
		return false;
	if (encoded != actual)
	{
		errno = EILSEQ;
		return false;
	}
	return true;
}
uint64_t fact_number(std::span<const uint8_t> facts, size_t offset, size_t width)
{
	uint64_t result = 0;
	for (size_t i = 0; i < width; ++i)
		result |= static_cast<uint64_t>(facts[offset + i]) << (8 * i);
	return result;
}

bool retained_rows(MYSQL *db, const std::string &sql, size_t fields, size_t limit,
		   std::vector<std::vector<std::string>> *output)
{
	if (!execute(db, sql))
		return false;
	std::unique_ptr<MYSQL_RES, decltype(&mysql_free_result)> rows(mysql_store_result(db),
								      mysql_free_result);
	if (!rows || mysql_num_fields(rows.get()) != fields || mysql_num_rows(rows.get()) > limit)
	{
		errno = mysql_errno(db) ? static_cast<int>(mysql_errno(db)) : EILSEQ;
		return false;
	}
	std::vector<std::vector<std::string>> result;
	for (MYSQL_ROW cells; (cells = mysql_fetch_row(rows.get()));)
	{
		const auto *lengths = mysql_fetch_lengths(rows.get());
		if (!lengths)
		{
			errno = EILSEQ;
			return false;
		}
		std::vector<std::string> value;
		for (size_t i = 0; i < fields; ++i)
		{
			if (!cells[i])
			{
				errno = EILSEQ;
				return false;
			}
			value.emplace_back(cells[i], lengths[i]);
		}
		result.push_back(std::move(value));
	}
	if (mysql_errno(db))
	{
		errno = static_cast<int>(mysql_errno(db));
		return false;
	}
	*output = std::move(result);
	return true;
}

bool normalized_items(MYSQL *db, const critical_command &cmd, const auction_command_result &r,
		      const economic_accounting_plan &plan)
{
	const auto scope = "operation_id=" + hex(cmd.operation_id.bytes);
	if (!plan.children.empty() || !exact_count(db, "economic_accounting_child", scope, 0) ||
	    !exact_count(db, "economic_accounting_item_reference", scope,
			 plan.item_events.size()) ||
	    !exact_count(db, "item_ownership_ledger", scope, plan.item_events.size()))
		return false;
	for (size_t i = 0; i < plan.item_events.size(); ++i)
	{
		const auto &e = plan.item_events[i];
		const auto reference = scope + " AND line_index=" + std::to_string(i) +
				       " AND event_index=" + std::to_string(e.event_index) +
				       " AND child_index=" + std::to_string(e.child_index) +
				       " AND item_uid=" + std::to_string(e.uid) +
				       " AND before_revision=" + std::to_string(e.before.revision) +
				       " AND after_revision=" + std::to_string(e.after.revision) +
				       " AND legacy_operation_id=" + hex(cmd.operation_id.bytes) +
				       " AND legacy_event_index=" + std::to_string(i);
		const auto from_revision = cmd.type == critical_command_type::auction &&
							   r.action == auction_action::list ?
						   r.player_owner_revision :
						   r.auction_owner_revision;
		const auto to_revision = r.action == auction_action::list ?
						 r.auction_owner_revision :
						 r.player_owner_revision;
		const auto native =
			scope + " AND event_index=" + std::to_string(i) +
			" AND item_uid=" + std::to_string(e.uid) +
			" AND root_item_uid=" + std::to_string(e.after.root_uid) +
			(cmd.payload_version == AUCTION_NATIVE_COMMAND_PAYLOAD_VERSION &&
					 e.after.parent_uid ?
				 " AND parent_item_uid=" + std::to_string(e.after.parent_uid) :
				 " AND parent_item_uid IS NULL") +
			" AND from_owner_type=" +
			std::to_string(static_cast<unsigned>(e.before.owner.type)) +
			" AND from_owner_id=" + std::to_string(e.before.owner.id) +
			" AND from_owner_context_id=" + std::to_string(e.before.owner.context_id) +
			" AND to_owner_type=" +
			std::to_string(static_cast<unsigned>(e.after.owner.type)) +
			" AND to_owner_id=" + std::to_string(e.after.owner.id) +
			" AND to_owner_context_id=" + std::to_string(e.after.owner.context_id) +
			" AND item_revision=" + std::to_string(e.after.revision) +
			" AND from_owner_revision=" + std::to_string(from_revision) +
			" AND to_owner_revision=" + std::to_string(to_revision) +
			" AND reason_type=" +
			std::to_string(static_cast<unsigned>(
				r.action == auction_action::list ?
					item_transfer_reason::auction_list :
					item_transfer_reason::auction_claim)) +
			" AND reason_id=" + std::to_string(r.auction_id) + " AND source_site=" +
			std::to_string(static_cast<unsigned>(cmd.source_site)) +
			" AND from_equipment_slot=0 AND to_equipment_slot=0";
		if (!exact_count(db, "economic_accounting_item_reference", reference, 1) ||
		    !exact_count(db, "item_ownership_ledger", native, 1))
			return false;
	}
	return true;
}

// Known original creator evidence cannot reconstruct an unseen command header.
// It binds the stored EAI/EAP and normalized original vectors to the native receipt.
bool known_creator(MYSQL *db, const critical_operation_id &operation,
		   economic_frozen_intent *intent, economic_accounting_plan *plan,
		   auction_command_result *result)
{
	std::vector<std::string> cells;
	if (!row(db,
		 "SELECT o.canonical_intent,o.canonical_plan,o.plan_digest,i.result_payload,i.durable_revision "
		 "FROM economic_accounting_operation o JOIN critical_operation_inbox i ON i.operation_id=o.operation_id WHERE o.operation_id=" +
			 hex(operation.bytes) +
			 " AND o.outcome=1 AND o.result_code=0 AND i.status=1 AND i.result_code=0 AND i.failure_stage=0 AND i.committed_at IS NOT NULL LOCK IN SHARE MODE",
		 5, &cells))
		return false;
	economic_digest digest{}, intent_digest{};
	if (!accounting_ok(economic_intent_decode(bytes(cells[0]), intent)) ||
	    !accounting_ok(economic_plan_decode(bytes(cells[1]), plan)) ||
	    !accounting_ok(economic_plan_digest(*plan, &digest)) ||
	    !accounting_ok(economic_intent_digest(*intent, &intent_digest)) ||
	    cells[2].size() != digest.size() ||
	    !std::equal(digest.begin(), digest.end(), bytes(cells[2]).begin()) ||
	    plan->metadata.operation_id.bytes != operation.bytes ||
	    plan->metadata.intent_digest != intent_digest ||
	    plan->metadata.domain_digest != intent->domain_digest ||
	    !normalized_financial_rows(db, operation, *plan))
	{
		if (!errno)
			errno = EILSEQ;
		return false;
	}
	const auto &meta = intent->admission.metadata;
	auto bound = *plan;
	static_cast<economic_operation_metadata &>(bound.metadata) = meta;
	bound.metadata.intent_digest = intent_digest;
	bound.metadata.domain_digest = intent->domain_digest;
	std::vector<uint8_t> rebound, canonical_intent;
	if (!accounting_ok(economic_plan_encode(bound, &rebound)) ||
	    rebound.size() != cells[1].size() ||
	    !std::equal(rebound.begin(), rebound.end(), bytes(cells[1]).begin()) ||
	    !accounting_ok(economic_intent_encode(*intent, &canonical_intent)) ||
	    canonical_intent.size() != cells[0].size() ||
	    !std::equal(canonical_intent.begin(), canonical_intent.end(),
			bytes(cells[0]).begin()) ||
	    !exact_count(
		    db, "economic_accounting_operation",
		    "operation_id=" + hex(operation.bytes) + " AND canonical_plan=" + hex(rebound) +
			    " AND plan_digest=" + hex(digest) +
			    " AND account_count=" + std::to_string(plan->accounts.size()) +
			    " AND posting_count=" + std::to_string(plan->postings.size()) +
			    " AND child_count=" + std::to_string(plan->children.size()) +
			    " AND item_event_count=" + std::to_string(plan->item_events.size()) +
			    " AND before_witness_count=" +
			    std::to_string(plan->items_before.size()) +
			    " AND after_witness_count=" + std::to_string(plan->items_after.size()),
		    1))
	{
		if (!errno)
			errno = EILSEQ;
		return false;
	}
	if (meta.operation_id.bytes != operation.bytes)
	{
		errno = EILSEQ;
		return false;
	}
	if (meta.reason == economic_reason::baseline)
		return true; // Full known baseline was routed before mapping locks.
	if (meta.actor_kind != economic_actor_kind::domain ||
	    (meta.writer_id != ECONOMIC_WRITER_AUCTION_BID &&
	     meta.writer_id != ECONOMIC_WRITER_AUCTION_SETTLEMENT))
	{
		errno = EILSEQ;
		return false;
	}
	if (!exact_count(
		    db, "critical_operation_inbox",
		    "operation_id=" + hex(operation.bytes) + " AND schema_version=" +
			    std::to_string(CRITICAL_COMMAND_ACCOUNTING_SCHEMA_VERSION) +
			    " AND command_type=" +
			    std::to_string(static_cast<unsigned>(critical_command_type::auction)) +
			    " AND payload_version IN (1,2)",
		    1))
		return false;
	auto error = economic_sql_auction_source_claim_verify(db, *intent, 0);
	if (error)
	{
		errno = static_cast<int>(error);
		return false;
	}
	std::array<uint8_t, AUCTION_RESULT_PAYLOAD_BYTES> canonical{};
	uint64_t durable = 0;
	if (!auction_command_decode_result(bytes(cells[3]).data(), cells[3].size(), result) ||
	    !auction_command_encode_result(*result, &canonical) ||
	    cells[3].size() != canonical.size() ||
	    !std::equal(canonical.begin(), canonical.end(), bytes(cells[3]).begin()) ||
	    !u64(cells[4], &durable) ||
	    durable != std::max({ result->auction_revision, result->wallet_revision,
				  result->bank_revision, result->player_owner_revision,
				  result->auction_owner_revision }))
	{
		errno = EILSEQ;
		return false;
	}
	std::vector<std::string> version_cells;
	uint64_t retained_version = 0;
	if (!row(db,
		 "SELECT payload_version FROM critical_operation_inbox WHERE operation_id=" +
			 hex(operation.bytes),
		 1, &version_cells) ||
	    !u64(version_cells[0], &retained_version))
		return false;
	if (retained_version == AUCTION_NATIVE_COMMAND_PAYLOAD_VERSION &&
	    !native_retained_known_root(db, *intent, *result))
		return false;
	const auto facts = std::span<const uint8_t>(intent->admission.facts);
	const bool bid = meta.writer_id == ECONOMIC_WRITER_AUCTION_BID;
	if (facts.size() < (bid ? 124U : 122U) || !meta.source_event ||
	    meta.source_event->kind != economic_source_kind::auction ||
	    meta.source_event->generation.bytes != meta.original_operation_id.bytes ||
	    fact_number(facts, bid ? 48 : 32, 4) != result->auction_id ||
	    fact_number(facts, bid ? 52 : 36, 4) != result->seller_pid ||
	    meta.source_event->sequence != fact_number(facts, bid ? 84 : 72, 8) ||
	    meta.source_event->slot != (result->action == auction_action::remove ? 1U : 0U) ||
	    (bid ? result->action != auction_action::bid :
		   (result->action != auction_action::finalize &&
		    result->action != auction_action::remove)))
	{
		errno = EILSEQ;
		return false;
	}
	critical_operation_id frozen_listing{}, previous{};
	std::copy_n(facts.begin() + (bid ? 92 : 88), 16, frozen_listing.bytes.begin());
	std::copy_n(facts.begin() + (bid ? 108 : 104), 16, previous.bytes.begin());
	const auto winner = static_cast<uint32_t>(fact_number(facts, bid ? 56 : 40, 4));
	if (frozen_listing.bytes != meta.original_operation_id.bytes ||
	    meta.source_event->source.bytes != (winner ? previous.bytes : frozen_listing.bytes) ||
	    !exact_count(
		    db,
		    "auction_ledger l JOIN critical_operation_inbox i ON i.operation_id=l.operation_id",
		    "l.operation_id=" + hex(frozen_listing.bytes) +
			    " AND l.auction_id=" + std::to_string(result->auction_id) +
			    " AND l.event_type=1 AND l.actor_pid=" +
			    std::to_string(result->seller_pid) +
			    " AND i.status=1 AND i.result_code=0 AND i.failure_stage=0 AND i.committed_at IS NOT NULL",
		    1) ||
	    (winner &&
	     !origin(db, previous, result->auction_id, 2, winner, meta.source_event->sequence,
		     static_cast<int64_t>(fact_number(facts, bid ? 68 : 56, 8)))) ||
	    !exact_count(db, "economic_accounting_operation",
			 "operation_id=" + hex(operation.bytes) +
				 (bid ? " AND realized_price_copper=" +
						  std::to_string(result->final_price) :
					" AND realized_price_copper IS NULL"),
			 1))
	{
		if (!errno)
			errno = EILSEQ;
		return false;
	}
	uint64_t native_actor = meta.actor_id;
	if (meta.writer_id == ECONOMIC_WRITER_AUCTION_SETTLEMENT)
	{
		if (facts.size() < 122)
		{
			errno = EILSEQ;
			return false;
		}
		const auto actor_wallet = fact_number(facts, 16, 8);
		if (!actor_wallet)
		{
			if (meta.actor_id != result->auction_id)
			{
				errno = EILSEQ;
				return false;
			}
			native_actor = 0;
		}
		else
		{
			std::vector<std::string> actor;
			if (!row(db,
				 "SELECT native_id FROM economic_account_mapping WHERE mapping_id=" +
					 std::to_string(actor_wallet) +
					 " AND lineage=" + hex(meta.lineage.bytes) +
					 " AND account_kind=1 AND context_id=0 AND backend_kind=1 AND locator_kind=1 LOCK IN SHARE MODE",
				 1, &actor) ||
			    !u64(actor[0], &native_actor) || native_actor != meta.actor_id)
			{
				if (!errno)
					errno = EILSEQ;
				return false;
			}
		}
	}
	const auto native = "operation_id=" + hex(operation.bytes);
	return exact_count(db, "auction_ledger", native, 1) &&
	       exact_count(
		       db, "auction_ledger",
		       native + " AND event_type=" +
			       std::to_string(static_cast<unsigned>(result->event_type)) +
			       " AND auction_id=" + std::to_string(result->auction_id) +
			       " AND auction_revision=" + std::to_string(result->auction_revision) +
			       " AND actor_pid=" + std::to_string(native_actor) +
			       " AND counterparty_pid=" + std::to_string(result->seller_pid) +
			       " AND value_delta=" + std::to_string(result->wallet_value_delta) +
			       " AND final_price=" + std::to_string(result->final_price) +
			       " AND item_count=" + std::to_string(result->item_count),
		       1);
}

bool money_source_baselines(MYSQL *db, const critical_command &cmd)
{
	std::vector<std::vector<std::string>> rows;
	if (!retained_rows(
		    db,
		    "SELECT DISTINCT o.operation_id FROM economic_accounting_operation o JOIN economic_pending_claim_source s ON s.source_operation_id=o.operation_id WHERE o.reason=" +
			    std::to_string(static_cast<unsigned>(economic_reason::baseline)) +
			    " AND (s.claim_operation_id=" + hex(cmd.operation_id.bytes) +
			    " OR EXISTS (SELECT 1 FROM economic_pending_claim_consumption c WHERE c.source_operation_id=s.source_operation_id AND c.source_slot=s.source_slot AND c.spending_operation_id=" +
			    hex(cmd.operation_id.bytes) + ")) ORDER BY o.operation_id",
		    1, ECONOMIC_AUCTION_CLAIM_MAX_SOURCES, &rows))
		return false;
	for (const auto &cells : rows)
	{
		critical_operation_id operation{};
		if (cells[0].size() != operation.bytes.size())
		{
			errno = EILSEQ;
			return false;
		}
		std::copy(bytes(cells[0]).begin(), bytes(cells[0]).end(), operation.bytes.begin());
		const auto error = economic_sql_baseline_verify_known_retained_in_transaction(
			db, operation, nullptr);
		if (error)
		{
			errno = static_cast<int>(error);
			return false;
		}
	}
	return true;
}

bool original_money_sources(MYSQL *db, const critical_command &cmd, const economic_account_key &key,
			    uint32_t pid, auction_money_claim_state *state)
{
	std::vector<std::vector<std::string>> rows;
	const auto op = hex(cmd.operation_id.bytes);
	if (!retained_rows(
		    db,
		    "SELECT source_operation_id,source_slot,amount FROM economic_pending_claim_consumption WHERE spending_operation_id=" +
			    op +
			    " UNION ALL SELECT source_operation_id,source_slot,amount FROM economic_pending_claim_source WHERE claim_operation_id=" +
			    op + " ORDER BY source_operation_id,source_slot",
		    3, ECONOMIC_AUCTION_CLAIM_MAX_SOURCES, &rows))
		return false;
	for (const auto &cells : rows)
	{
		auction_money_claim_source source;
		uint64_t slot = 0;
		if (cells[0].size() != source.operation.bytes.size() || !u64(cells[1], &slot) ||
		    !slot || slot > UINT16_MAX || !u64(cells[2], &source.amount) ||
		    !source.amount || source.amount > INT_MAX)
		{
			errno = EILSEQ;
			return false;
		}
		std::copy(bytes(cells[0]).begin(), bytes(cells[0]).end(),
			  source.operation.bytes.begin());
		source.slot = static_cast<uint16_t>(slot);
		source.beneficiary_pid = pid;
		source.claim_mapping_id = key.authority_id;
		std::vector<std::string> original;
		uint64_t amount = 0;
		if (!row(db,
			 "SELECT amount FROM economic_pending_claim_source WHERE source_operation_id=" +
				 hex(source.operation.bytes) + " AND source_slot=" +
				 std::to_string(slot) + " AND lineage=" + hex(key.lineage.bytes) +
				 " AND claim_mapping_id=" + std::to_string(key.authority_id) +
				 " AND beneficiary_pid=" + std::to_string(pid) +
				 " LOCK IN SHARE MODE",
			 1, &original) ||
		    !u64(original[0], &amount) || amount < source.amount)
		{
			if (!errno)
				errno = EILSEQ;
			return false;
		}
		economic_frozen_intent creator;
		economic_accounting_plan creator_plan;
		auction_command_result creator_result{};
		if (!known_creator(db, source.operation, &creator, &creator_plan, &creator_result))
			return false;
		const auto *credit = effect(creator_plan, key);
		if (!credit || credit->after[0] < credit->before[0] ||
		    static_cast<uint64_t>(credit->after[0] - credit->before[0]) != amount ||
		    credit->after[1] != credit->before[1] ||
		    credit->after[2] != credit->before[2] || credit->after[3] != credit->before[3])
		{
			errno = EILSEQ;
			return false;
		}
		if (creator.admission.metadata.reason != economic_reason::baseline &&
		    ((slot == 1 &&
		      (creator.admission.metadata.writer_id != ECONOMIC_WRITER_AUCTION_BID ||
		       creator_result.previous_bidder_pid != pid)) ||
		     (slot == 2 && (creator_result.event_type != auction_event_type::sold ||
				    creator_result.seller_pid != pid ||
				    creator_result.final_price < static_cast<int64_t>(amount))) ||
		     (slot != 1 && slot != 2)))
		{
			errno = EILSEQ;
			return false;
		}
		if (creator.admission.metadata.reason != economic_reason::baseline)
		{
			const auto facts = std::span<const uint8_t>(creator.admission.facts);
			const bool bid = creator.admission.metadata.writer_id ==
					 ECONOMIC_WRITER_AUCTION_BID;
			if ((bid && facts.size() < 124) ||
			    (!bid && (creator.admission.metadata.writer_id !=
					      ECONOMIC_WRITER_AUCTION_SETTLEMENT ||
				      facts.size() < 122)))
			{
				errno = EILSEQ;
				return false;
			}
			if (slot == 1 && (fact_number(facts, 56, 4) != pid ||
					  fact_number(facts, 68, 8) != amount))
			{
				errno = EILSEQ;
				return false;
			}
			if (slot == 2)
			{
				if (fact_number(facts, bid ? 52 : 36, 4) != pid)
				{
					errno = EILSEQ;
					return false;
				}
				__int128_t fee = 0;
				for (const auto &posting : creator_plan.postings)
				{
					const auto &account =
						creator_plan.accounts[posting.account_index].key;
					if (account.kind == economic_account_kind::sink &&
					    account.authority_id ==
						    ECONOMIC_AUCTION_CLOSING_FEE_SINK_ID)
						fee += posting.copper;
				}
				if (fee < 0 || fee + amount != creator_result.final_price)
				{
					errno = EILSEQ;
					return false;
				}
			}
		}
		state->sources.push_back(source);
	}
	return original_consumption(db, cmd, key, pid, static_cast<uint64_t>(state->money));
}

bool regenerate_three(MYSQL *db, const critical_command &cmd, const economic_frozen_intent &intent,
		      const economic_accounting_plan &plan, const auction_command_result &result,
		      economic_accounting_plan *expected)
{
	auction_command_payload payload{};
	economic_frozen_intent decoded;
	economic_account_key wallet, bank, claim_key;
	economic_accounting_error status;
	if (result.action == auction_action::list)
	{
		if (!accounting_ok(auction_listing_accounting_decode(cmd, &decoded, &payload,
								     &wallet, &bank)))
			return false;
		auction_listing_accounting_authority authority;
		authority.epoch = intent.admission.metadata.epoch;
		authority.wallet = wallet;
		authority.bank = bank;
		for (const auto &e : plan.accounts)
			if (e.key.kind == economic_account_kind::auction_escrow)
				authority.escrow = e.key;
		if (!mapping(db, wallet, 1, payload.actor_pid) || !mapping(db, bank, 2, 0) ||
		    !mapping(db, authority.escrow, 4, result.auction_id, nullptr,
			     &cmd.operation_id))
			return false;
		const auto *w = effect(plan, wallet);
		if (!w || !result.player_owner_revision)
		{
			errno = EILSEQ;
			return false;
		}
		authority.balances_before.wallet.amount = w->before;
		authority.balances_before.wallet_revision = w->before_revision;
		authority.balances_before.bank = result.bank;
		authority.balances_before.bank_revision = payload.expected_bank_revision;
		authority.player_owner_revision_before = result.player_owner_revision - 1;
		if (cmd.payload_version == AUCTION_NATIVE_COMMAND_PAYLOAD_VERSION)
		{
			auction_item_claim_state source;
			source.auction_id = result.auction_id;
			source.seller_pid = payload.actor_pid;
			source.item_count = payload.item_count;
			source.listing_operation = cmd.operation_id;
			for (size_t i = 0; i < payload.item_count; ++i)
				source.rows[i] = { payload.items[i].item_uid,
						   payload.items[i].expected_item_revision,
						   static_cast<uint16_t>(i),
						   payload.items[i].vnum,
						   payload.actor_pid,
						   false };
			if (!native_retained_selected(db, cmd, intent, source, payload, plan,
						      &authority.native_selected_literals))
				return false;
			for (const auto &literal : authority.native_selected_literals)
			{
				const auto witness = std::find_if(
					plan.items_before.begin(), plan.items_before.end(),
					[&](const auto &w) { return w.uid == literal.object_uid; });
				if (witness == plan.items_before.end())
				{
					errno = EILSEQ;
					return false;
				}
				authority.items_before.push_back(*witness);
			}
		}
		else
			for (size_t i = 0; i < payload.item_count; ++i)
				authority.items_before.push_back(
					{ payload.items[i].item_uid,
					  { { item_owner_type::player, payload.actor_pid, 0 },
					    payload.items[i].item_uid,
					    0,
					    payload.items[i].expected_item_revision,
					    item_custody_state::active } });
		status = auction_listing_accounting_plan(cmd, intent, authority, result, expected);
		if (!exact_count(db, "economic_account_mapping",
				 "creating_operation_id=" + hex(cmd.operation_id.bytes), 1))
			return false;
	}
	else if (result.action == auction_action::claim_item)
	{
		auction_item_claim_state claim;
		if (!accounting_ok(auction_item_claim_accounting_decode(cmd, &decoded, &payload,
									&claim, &wallet, &bank)) ||
		    !mapping(db, wallet, 1, payload.actor_pid) || !mapping(db, bank, 2, 0))
			return false;
		auction_item_claim_accounting_authority authority;
		authority.epoch = intent.admission.metadata.epoch;
		authority.wallet_account = wallet;
		authority.bank_account = bank;
		authority.claim = claim;
		authority.balances_before.wallet = result.wallet;
		authority.balances_before.bank = result.bank;
		authority.balances_before.wallet_revision = result.wallet_revision;
		authority.balances_before.bank_revision = result.bank_revision;
		if (!result.player_owner_revision || !result.auction_owner_revision)
		{
			errno = EILSEQ;
			return false;
		}
		authority.player_owner_revision_before = result.player_owner_revision - 1;
		authority.auction_owner_revision_before = result.auction_owner_revision - 1;
		if (cmd.payload_version == AUCTION_NATIVE_COMMAND_PAYLOAD_VERSION)
		{
			if (!native_retained_selected(db, cmd, intent, claim, payload, plan,
						      &authority.native_selected_literals))
				return false;
			for (const auto &literal : authority.native_selected_literals)
			{
				const auto witness = std::find_if(
					plan.items_before.begin(), plan.items_before.end(),
					[&](const auto &w) { return w.uid == literal.object_uid; });
				if (witness == plan.items_before.end())
				{
					errno = EILSEQ;
					return false;
				}
				authority.items_before.push_back(*witness);
			}
		}
		else
			for (size_t i = 0; i < payload.item_count; ++i)
				authority.items_before.push_back(
					{ payload.items[i].item_uid,
					  { { item_owner_type::auction, payload.auction_id, 0 },
					    payload.items[i].item_uid,
					    0,
					    payload.items[i].expected_item_revision,
					    item_custody_state::active } });
		if (!exact_count(
			    db,
			    "auction_ledger l JOIN critical_operation_inbox i ON i.operation_id=l.operation_id",
			    "l.operation_id=" + hex(claim.listing_operation.bytes) +
				    " AND l.auction_id=" + std::to_string(claim.auction_id) +
				    " AND l.event_type=1 AND l.actor_pid=" +
				    std::to_string(claim.seller_pid) +
				    " AND i.status=1 AND i.result_code=0 AND i.failure_stage=0 AND i.committed_at IS NOT NULL",
			    1))
			return false;
		std::vector<std::string> receipt;
		if (!row(db,
			 "SELECT i.result_payload FROM critical_operation_inbox i JOIN auction_ledger l ON l.operation_id=i.operation_id WHERE i.operation_id=" +
				 hex(claim.claim_source_operation.bytes) +
				 " AND l.auction_id=" + std::to_string(claim.auction_id) +
				 " AND l.event_type IN (3,4,7) AND l.auction_revision<=" +
				 std::to_string(claim.auction_revision) +
				 " AND i.status=1 AND i.result_code=0 AND i.failure_stage=0 AND i.committed_at IS NOT NULL LOCK IN SHARE MODE",
			 1, &receipt))
			return false;
		auction_command_result terminal{};
		std::array<uint8_t, AUCTION_RESULT_PAYLOAD_BYTES> canonical{};
		if (!auction_command_decode_result(bytes(receipt[0]).data(), receipt[0].size(),
						   &terminal) ||
		    !auction_command_encode_result(terminal, &canonical) ||
		    receipt[0].size() != canonical.size() ||
		    !std::equal(canonical.begin(), canonical.end(), bytes(receipt[0]).begin()) ||
		    terminal.auction_id != claim.auction_id || terminal.status != claim.status ||
		    terminal.seller_pid != claim.seller_pid ||
		    terminal.winner_pid != claim.winner_pid ||
		    (terminal.event_type == auction_event_type::sold ?
			     claim.claimant_pid != terminal.winner_pid :
			     claim.claimant_pid != terminal.seller_pid))
		{
			errno = EILSEQ;
			return false;
		}
		const auto terminal_scope =
			"operation_id=" + hex(claim.claim_source_operation.bytes);
		if (!exact_count(db, "auction_ledger", terminal_scope, 1) ||
		    !exact_count(
			    db, "auction_ledger",
			    terminal_scope + " AND event_type=" +
				    std::to_string(static_cast<unsigned>(terminal.event_type)) +
				    " AND auction_id=" + std::to_string(terminal.auction_id) +
				    " AND auction_revision=" +
				    std::to_string(terminal.auction_revision) +
				    " AND counterparty_pid=" + std::to_string(terminal.seller_pid) +
				    " AND value_delta=" +
				    std::to_string(terminal.wallet_value_delta) +
				    " AND final_price=" + std::to_string(terminal.final_price) +
				    " AND item_count=" + std::to_string(terminal.item_count),
			    1) ||
		    !exact_count(db, "critical_operation_inbox",
				 terminal_scope + " AND durable_revision=" +
					 std::to_string(std::max(
						 { terminal.auction_revision,
						   terminal.wallet_revision, terminal.bank_revision,
						   terminal.player_owner_revision,
						   terminal.auction_owner_revision })),
				 1))
			return false;
		std::vector<std::string> terminal_schema;
		uint64_t schema = 0;
		if (!row(db,
			 "SELECT schema_version FROM critical_operation_inbox WHERE " +
				 terminal_scope,
			 1, &terminal_schema) ||
		    !u64(terminal_schema[0], &schema))
			return false;
		economic_accounting_plan terminal_plan;
		economic_frozen_intent terminal_intent;
		if (schema == CRITICAL_COMMAND_ACCOUNTING_SCHEMA_VERSION)
		{
			auction_command_result observed{};
			if (!(cmd.payload_version == AUCTION_NATIVE_COMMAND_PAYLOAD_VERSION ?
				      known_terminal_creator(db, claim.claim_source_operation,
							     &terminal_intent, &terminal_plan,
							     &observed) :
				      known_creator(db, claim.claim_source_operation,
						    &terminal_intent, &terminal_plan, &observed)) ||
			    terminal_intent.admission.metadata.original_operation_id.bytes !=
				    claim.listing_operation.bytes)
				return false;
		}
		else if (schema != CRITICAL_COMMAND_SCHEMA_VERSION)
		{
			errno = EILSEQ;
			return false;
		}
		for (size_t i = 0; i < claim.item_count; ++i)
		{
			const auto &item = claim.rows[i];
			if (cmd.payload_version != AUCTION_NATIVE_COMMAND_PAYLOAD_VERSION &&
			    !exact_count(
				    db, "item_ownership_ledger",
				    "operation_id=" + hex(claim.listing_operation.bytes) +
					    " AND event_index=" + std::to_string(item.slot) +
					    " AND item_uid=" + std::to_string(item.uid) +
					    " AND root_item_uid=" + std::to_string(item.uid) +
					    " AND parent_item_uid IS NULL AND from_owner_type=" +
					    std::to_string(static_cast<unsigned>(
						    item_owner_type::player)) +
					    " AND from_owner_id=" +
					    std::to_string(claim.seller_pid) +
					    " AND from_owner_context_id=0 AND to_owner_type=" +
					    std::to_string(static_cast<unsigned>(
						    item_owner_type::auction)) +
					    " AND to_owner_id=" + std::to_string(claim.auction_id) +
					    " AND to_owner_context_id=0 AND item_revision=" +
					    std::to_string(item.revision) + " AND reason_type=" +
					    std::to_string(static_cast<unsigned>(
						    item_transfer_reason::auction_list)) +
					    " AND reason_id=" + std::to_string(claim.auction_id),
				    1))
				return false;
			if (schema == CRITICAL_COMMAND_ACCOUNTING_SCHEMA_VERSION &&
			    terminal_intent.admission.metadata.writer_id ==
				    ECONOMIC_WRITER_AUCTION_SETTLEMENT)
			{
				const auto facts =
					std::span<const uint8_t>(terminal_intent.admission.facts);
				if (facts.size() < 122)
				{
					errno = EILSEQ;
					return false;
				}
				const auto count = fact_number(facts, 120, 2);
				if (!count || count > AUCTION_COMMAND_MAX_ITEMS ||
				    facts.size() < 122 + count * 27 ||
				    (cmd.payload_version !=
					     AUCTION_NATIVE_COMMAND_PAYLOAD_VERSION &&
				     terminal_plan.items_before.size() != count))
				{
					errno = EILSEQ;
					return false;
				}
				bool found = false;
				for (size_t j = 0; j < count; ++j)
				{
					const auto offset = 122 + j * 27;
					if (fact_number(facts, offset, 8) != item.uid)
						continue;
					found = fact_number(facts, offset + 8, 8) ==
							item.revision &&
						fact_number(facts, offset + 16, 2) == item.slot &&
						fact_number(facts, offset + 18, 4) ==
							static_cast<uint32_t>(item.vnum) &&
						!fact_number(facts, offset + 22, 4) &&
						!facts[offset + 26];
				}
				if (!found)
				{
					errno = EILSEQ;
					return false;
				}
			}
			if (!terminal_plan.items_before.empty())
			{
				auto unchanged = terminal_plan;
				unchanged.items_after = unchanged.items_before;
				std::vector<uint8_t> actual_terminal, unchanged_terminal;
				if (!accounting_ok(economic_plan_encode(terminal_plan,
									&actual_terminal)) ||
				    !accounting_ok(
					    economic_plan_encode(unchanged, &unchanged_terminal)) ||
				    actual_terminal != unchanged_terminal)
				{
					if (!errno)
						errno = EILSEQ;
					return false;
				}
				const auto witness = std::find_if(
					terminal_plan.items_before.begin(),
					terminal_plan.items_before.end(),
					[&](const auto &w) { return w.uid == item.uid; });
				if (witness == terminal_plan.items_before.end() ||
				    witness->position.owner.type != item_owner_type::auction ||
				    witness->position.owner.id != claim.auction_id ||
				    witness->position.owner.context_id ||
				    witness->position.root_uid != item.uid ||
				    witness->position.parent_uid ||
				    witness->position.revision != item.revision ||
				    witness->position.state != item_custody_state::active)
				{
					errno = EILSEQ;
					return false;
				}
			}
		}
		status = auction_item_claim_accounting_plan(cmd, intent, authority, result,
							    expected);
	}
	else
	{
		if (!accounting_ok(auction_money_claim_accounting_decode(
			    cmd, &decoded, &payload, &wallet, &bank, &claim_key)) ||
		    !mapping(db, wallet, 1, payload.actor_pid) || !mapping(db, bank, 2, 0) ||
		    !mapping(db, claim_key, 5, payload.actor_pid))
			return false;
		auction_money_claim_authority authority;
		authority.epoch = intent.admission.metadata.epoch;
		authority.wallet = wallet;
		authority.bank = bank;
		authority.claim_account = claim_key;
		const auto *w = effect(plan, wallet), *c = effect(plan, claim_key);
		if (!w || !c)
		{
			errno = EILSEQ;
			return false;
		}
		authority.balances_before.wallet.amount = w->before;
		authority.balances_before.wallet_revision = w->before_revision;
		authority.balances_before.bank = result.bank;
		authority.balances_before.bank_revision = payload.expected_bank_revision;
		authority.claim.beneficiary_pid = payload.actor_pid;
		authority.claim.money = c->before[0];
		authority.claim.revision = c->before_revision;
		if (authority.claim.money <= 0 ||
		    !original_money_sources(db, cmd, claim_key, payload.actor_pid,
					    &authority.claim))
			return false;
		status = auction_money_claim_accounting_plan(cmd, intent, authority, result,
							     expected);
	}
	if (!accounting_ok(status))
		return false;
	std::vector<uint8_t> canonical, original;
	if (!accounting_ok(economic_plan_encode(*expected, &canonical)) ||
	    !accounting_ok(economic_plan_encode(plan, &original)) || canonical != original)
	{
		if (!errno)
			errno = EILSEQ;
		return false;
	}
	if (!normalized_items(db, cmd, result, plan) ||
	    !exact_count(db, "economic_pending_claim_source",
			 "source_operation_id=" + hex(cmd.operation_id.bytes), 0))
		return false;
	if (result.action != auction_action::claim_money &&
	    !original_consumption(db, cmd, {}, 0, 0))
		return false;
	if (result.action != auction_action::list &&
	    !exact_count(db, "economic_account_mapping",
			 "creating_operation_id=" + hex(cmd.operation_id.bytes), 0))
		return false;
	return exact_count(db, "economic_account_mapping",
			   "retiring_operation_id=" + hex(cmd.operation_id.bytes), 0);
}

bool native_rows(MYSQL *db, const critical_command &cmd, const auction_command_result &r,
		 const economic_accounting_plan &plan)
{
	auction_command_payload payload{};
	if (!auction_command_decode_payload(cmd, &payload))
	{
		errno = EILSEQ;
		return false;
	}
	auto scope = "operation_id=" + hex(cmd.operation_id.bytes);
	if (!exact_count(db, "auction_ledger", scope, 1) ||
	    !exact_count(db, "auction_ledger",
			 scope + " AND event_type=" +
				 std::to_string(static_cast<unsigned>(r.event_type)) +
				 " AND auction_id=" + std::to_string(r.auction_id) +
				 " AND auction_revision=" + std::to_string(r.auction_revision) +
				 " AND actor_pid=" + std::to_string(payload.actor_pid) +
				 " AND counterparty_pid=" + std::to_string(r.seller_pid) +
				 " AND value_delta=" + std::to_string(r.wallet_value_delta) +
				 " AND final_price=" + std::to_string(r.final_price) +
				 " AND item_count=" + std::to_string(r.item_count),
			 1))
		return false;
	if (payload.action != auction_action::bid && payload.action != auction_action::list &&
	    payload.action != auction_action::claim_money)
		return exact_count(db, "currency_ledger", scope, 0);
	auction_bid_accounting_listing listing;
	auction_bid_accounting_accounts keys;
	economic_frozen_intent intent;
	auto typed = auction_bid_accounting_decode(cmd, &intent, &payload, &listing, &keys);
	economic_account_key claim_key;
	if (payload.action == auction_action::list)
		typed = auction_listing_accounting_decode(cmd, &intent, &payload, &keys.wallet,
							  &keys.bank);
	else if (payload.action == auction_action::claim_money)
		typed = auction_money_claim_accounting_decode(cmd, &intent, &payload, &keys.wallet,
							      &keys.bank, &claim_key);
	if (typed != economic_accounting_error::ok)
	{
		errno = EILSEQ;
		return false;
	}
	uint64_t bank = 0;
	if (!mapping(db, keys.bank, 2, 0, &bank))
		return false;
	const auto *wallet = effect(plan, keys.wallet);
	if (!wallet)
	{
		errno = EILSEQ;
		return false;
	}
	economic_coin_vector delta{};
	if (economic_coin_delta(wallet->before, wallet->after, &delta) !=
	    economic_accounting_error::ok)
	{
		errno = EILSEQ;
		return false;
	}
	auto p =
		scope + " AND pid=" + std::to_string(payload.actor_pid) +
		" AND bank_id=" + std::to_string(bank) +
		" AND wallet_revision=" + std::to_string(r.wallet_revision) +
		" AND bank_revision=" + std::to_string(r.bank_revision) + " AND reason_type=" +
		std::to_string(static_cast<unsigned>(payload.action == auction_action::list ?
							     currency_reason_type::auction_listing :
						     payload.action == auction_action::claim_money ?
							     currency_reason_type::auction_claim :
							     currency_reason_type::auction_bid)) +
		" AND reason_id=" + std::to_string(payload.auction_id) +
		" AND source_site=" + std::to_string(static_cast<unsigned>(cmd.source_site));
	const char *names[] = { "copper", "silver", "gold", "platinum" };
	for (size_t i = 0; i < 4; ++i)
		p += " AND wallet_delta_" + std::string(names[i]) + "=" + std::to_string(delta[i]) +
		     " AND bank_delta_" + names[i] + "=0 AND wallet_after_" + names[i] + "=" +
		     std::to_string(r.wallet.amount[i]) + " AND bank_after_" + names[i] + "=" +
		     std::to_string(r.bank.amount[i]);
	return exact_count(db, "currency_ledger", scope, 1) &&
	       exact_count(db, "currency_ledger", p, 1);
}
bool known_terminal_creator(MYSQL *db, const critical_operation_id &operation,
			    economic_frozen_intent *intent, economic_accounting_plan *plan,
			    auction_command_result *result)
{
	std::vector<std::string> cells;
	if (!row(db,
		 "SELECT o.canonical_intent,o.canonical_plan,o.plan_digest,i.result_payload,i.durable_revision "
		 "FROM economic_accounting_operation o JOIN critical_operation_inbox i ON i.operation_id=o.operation_id WHERE o.operation_id=" +
			 hex(operation.bytes) +
			 " AND o.outcome=1 AND o.result_code=0 AND i.status=1 AND i.result_code=0 AND i.failure_stage=0 AND i.committed_at IS NOT NULL LOCK IN SHARE MODE",
		 5, &cells))
		return false;
	economic_digest digest{}, intent_digest{};
	if (!accounting_ok(economic_intent_decode(bytes(cells[0]), intent)) ||
	    !accounting_ok(economic_plan_decode(bytes(cells[1]), plan)) ||
	    !accounting_ok(economic_plan_digest(*plan, &digest)) ||
	    !accounting_ok(economic_intent_digest(*intent, &intent_digest)) ||
	    cells[2].size() != digest.size() ||
	    !std::equal(digest.begin(), digest.end(), bytes(cells[2]).begin()) ||
	    plan->metadata.operation_id.bytes != operation.bytes ||
	    plan->metadata.intent_digest != intent_digest ||
	    plan->metadata.domain_digest != intent->domain_digest ||
	    !normalized_financial_rows(db, operation, *plan))
	{
		if (!errno)
			errno = EILSEQ;
		return false;
	}
	const auto &meta = intent->admission.metadata;
	auto bound = *plan;
	static_cast<economic_operation_metadata &>(bound.metadata) = meta;
	bound.metadata.intent_digest = intent_digest;
	bound.metadata.domain_digest = intent->domain_digest;
	std::vector<uint8_t> rebound, canonical_intent;
	if (!accounting_ok(economic_plan_encode(bound, &rebound)) ||
	    rebound.size() != cells[1].size() ||
	    !std::equal(rebound.begin(), rebound.end(), bytes(cells[1]).begin()) ||
	    !accounting_ok(economic_intent_encode(*intent, &canonical_intent)) ||
	    canonical_intent.size() != cells[0].size() ||
	    !std::equal(canonical_intent.begin(), canonical_intent.end(),
			bytes(cells[0]).begin()) ||
	    !exact_count(
		    db, "economic_accounting_operation",
		    "operation_id=" + hex(operation.bytes) + " AND canonical_plan=" + hex(rebound) +
			    " AND plan_digest=" + hex(digest) +
			    " AND account_count=" + std::to_string(plan->accounts.size()) +
			    " AND posting_count=" + std::to_string(plan->postings.size()) +
			    " AND child_count=" + std::to_string(plan->children.size()) +
			    " AND item_event_count=" + std::to_string(plan->item_events.size()) +
			    " AND before_witness_count=" +
			    std::to_string(plan->items_before.size()) +
			    " AND after_witness_count=" + std::to_string(plan->items_after.size()),
		    1))
	{
		if (!errno)
			errno = EILSEQ;
		return false;
	}
	if (meta.operation_id.bytes != operation.bytes)
	{
		errno = EILSEQ;
		return false;
	}
	if (meta.reason == economic_reason::baseline)
		return true; // Full known baseline was routed before mapping locks.
	if (meta.actor_kind != economic_actor_kind::domain ||
	    (meta.writer_id != ECONOMIC_WRITER_AUCTION_BID &&
	     meta.writer_id != ECONOMIC_WRITER_AUCTION_SETTLEMENT))
	{
		errno = EILSEQ;
		return false;
	}
	if (!exact_count(
		    db, "critical_operation_inbox",
		    "operation_id=" + hex(operation.bytes) + " AND schema_version=" +
			    std::to_string(CRITICAL_COMMAND_ACCOUNTING_SCHEMA_VERSION) +
			    " AND command_type=" +
			    std::to_string(static_cast<unsigned>(critical_command_type::auction)) +
			    " AND payload_version IN (1,2)",
		    1))
		return false;
	auto error = economic_sql_auction_source_claim_verify(db, *intent, 0);
	if (error)
	{
		errno = static_cast<int>(error);
		return false;
	}
	std::array<uint8_t, AUCTION_RESULT_PAYLOAD_BYTES> canonical{};
	uint64_t durable = 0;
	if (!auction_command_decode_result(bytes(cells[3]).data(), cells[3].size(), result) ||
	    !auction_command_encode_result(*result, &canonical) ||
	    cells[3].size() != canonical.size() ||
	    !std::equal(canonical.begin(), canonical.end(), bytes(cells[3]).begin()) ||
	    !u64(cells[4], &durable) ||
	    durable != std::max({ result->auction_revision, result->wallet_revision,
				  result->bank_revision, result->player_owner_revision,
				  result->auction_owner_revision }))
	{
		errno = EILSEQ;
		return false;
	}
	std::vector<std::string> version_cells;
	uint64_t retained_version = 0;
	if (!row(db,
		 "SELECT payload_version FROM critical_operation_inbox WHERE operation_id=" +
			 hex(operation.bytes),
		 1, &version_cells) ||
	    !u64(version_cells[0], &retained_version))
		return false;
	if (retained_version == AUCTION_NATIVE_COMMAND_PAYLOAD_VERSION &&
	    !native_retained_known_root(db, *intent, *result))
		return false;
	const auto facts = std::span<const uint8_t>(intent->admission.facts);
	const bool bid = meta.writer_id == ECONOMIC_WRITER_AUCTION_BID;
	if (facts.size() < (bid ? 124U : 122U) || !meta.source_event ||
	    meta.source_event->kind != economic_source_kind::auction ||
	    meta.source_event->generation.bytes != meta.original_operation_id.bytes ||
	    fact_number(facts, bid ? 48 : 32, 4) != result->auction_id ||
	    fact_number(facts, bid ? 52 : 36, 4) != result->seller_pid ||
	    meta.source_event->sequence != fact_number(facts, bid ? 84 : 72, 8) ||
	    meta.source_event->slot != (result->action == auction_action::remove ? 1U : 0U) ||
	    (bid ? result->action != auction_action::bid :
		   (result->action != auction_action::finalize &&
		    result->action != auction_action::remove)))
	{
		errno = EILSEQ;
		return false;
	}
	critical_operation_id frozen_listing{}, previous{};
	std::copy_n(facts.begin() + (bid ? 92 : 88), 16, frozen_listing.bytes.begin());
	std::copy_n(facts.begin() + (bid ? 108 : 104), 16, previous.bytes.begin());
	const auto winner = static_cast<uint32_t>(fact_number(facts, bid ? 56 : 40, 4));
	if (frozen_listing.bytes != meta.original_operation_id.bytes ||
	    meta.source_event->source.bytes != (winner ? previous.bytes : frozen_listing.bytes) ||
	    !exact_count(
		    db,
		    "auction_ledger l JOIN critical_operation_inbox i ON i.operation_id=l.operation_id",
		    "l.operation_id=" + hex(frozen_listing.bytes) +
			    " AND l.auction_id=" + std::to_string(result->auction_id) +
			    " AND l.event_type=1 AND l.actor_pid=" +
			    std::to_string(result->seller_pid) +
			    " AND i.status=1 AND i.result_code=0 AND i.failure_stage=0 AND i.committed_at IS NOT NULL",
		    1) ||
	    (winner &&
	     !origin(db, previous, result->auction_id, 2, winner, meta.source_event->sequence,
		     static_cast<int64_t>(fact_number(facts, bid ? 68 : 56, 8)))) ||
	    !exact_count(db, "economic_accounting_operation",
			 "operation_id=" + hex(operation.bytes) +
				 (bid ? " AND realized_price_copper=" +
						  std::to_string(result->final_price) :
					" AND realized_price_copper IS NULL"),
			 1))
	{
		if (!errno)
			errno = EILSEQ;
		return false;
	}
	uint64_t native_actor = meta.actor_id;
	if (meta.writer_id == ECONOMIC_WRITER_AUCTION_SETTLEMENT)
	{
		if (facts.size() < 122)
		{
			errno = EILSEQ;
			return false;
		}
		const auto actor_wallet = fact_number(facts, 16, 8);
		if (!actor_wallet)
		{
			if (meta.actor_id != result->auction_id)
			{
				errno = EILSEQ;
				return false;
			}
			native_actor = 0;
		}
		else
		{
			std::vector<std::string> actor;
			if (!row(db,
				 "SELECT native_id FROM economic_account_mapping WHERE mapping_id=" +
					 std::to_string(actor_wallet) +
					 " AND lineage=" + hex(meta.lineage.bytes) +
					 " AND account_kind=1 AND context_id=0 AND backend_kind=1 AND locator_kind=1 LOCK IN SHARE MODE",
				 1, &actor) ||
			    !u64(actor[0], &native_actor) || native_actor != meta.actor_id)
			{
				if (!errno)
					errno = EILSEQ;
				return false;
			}
		}
	}
	const auto native = "operation_id=" + hex(operation.bytes);
	return exact_count(db, "auction_ledger", native, 1) &&
	       exact_count(
		       db, "auction_ledger",
		       native + " AND event_type=" +
			       std::to_string(static_cast<unsigned>(result->event_type)) +
			       " AND auction_id=" + std::to_string(result->auction_id) +
			       " AND auction_revision=" + std::to_string(result->auction_revision) +
			       " AND actor_pid=" + std::to_string(native_actor) +
			       " AND counterparty_pid=" + std::to_string(result->seller_pid) +
			       " AND value_delta=" + std::to_string(result->wallet_value_delta) +
			       " AND final_price=" + std::to_string(result->final_price) +
			       " AND item_count=" + std::to_string(result->item_count),
		       1);
}

bool listing_root_core(MYSQL *db, const critical_operation_id &operation,
		       economic_frozen_intent *intent, economic_accounting_plan *plan,
		       auction_command_result *result)
{
	std::vector<std::string> cells;
	if (!row(db,
		 "SELECT o.canonical_intent,o.canonical_plan,o.plan_digest,i.result_payload,i.durable_revision "
		 "FROM economic_accounting_operation o JOIN critical_operation_inbox i ON i.operation_id=o.operation_id WHERE o.operation_id=" +
			 hex(operation.bytes) +
			 " AND o.outcome=1 AND o.result_code=0 AND i.status=1 AND i.result_code=0 AND i.failure_stage=0 AND i.committed_at IS NOT NULL LOCK IN SHARE MODE",
		 5, &cells))
		return false;
	economic_digest digest{}, intent_digest{};
	if (!accounting_ok(economic_intent_decode(bytes(cells[0]), intent)) ||
	    !accounting_ok(economic_plan_decode(bytes(cells[1]), plan)) ||
	    !accounting_ok(economic_plan_digest(*plan, &digest)) ||
	    !accounting_ok(economic_intent_digest(*intent, &intent_digest)) ||
	    cells[2].size() != digest.size() ||
	    !std::equal(digest.begin(), digest.end(), bytes(cells[2]).begin()) ||
	    plan->metadata.operation_id.bytes != operation.bytes ||
	    plan->metadata.intent_digest != intent_digest ||
	    plan->metadata.domain_digest != intent->domain_digest ||
	    !normalized_financial_rows(db, operation, *plan))
	{
		if (!errno)
			errno = EILSEQ;
		return false;
	}
	const auto &meta = intent->admission.metadata;
	auto bound = *plan;
	static_cast<economic_operation_metadata &>(bound.metadata) = meta;
	bound.metadata.intent_digest = intent_digest;
	bound.metadata.domain_digest = intent->domain_digest;
	std::vector<uint8_t> rebound, canonical_intent;
	if (!accounting_ok(economic_plan_encode(bound, &rebound)) ||
	    rebound.size() != cells[1].size() ||
	    !std::equal(rebound.begin(), rebound.end(), bytes(cells[1]).begin()) ||
	    !accounting_ok(economic_intent_encode(*intent, &canonical_intent)) ||
	    canonical_intent.size() != cells[0].size() ||
	    !std::equal(canonical_intent.begin(), canonical_intent.end(),
			bytes(cells[0]).begin()) ||
	    !exact_count(
		    db, "economic_accounting_operation",
		    "operation_id=" + hex(operation.bytes) + " AND canonical_plan=" + hex(rebound) +
			    " AND plan_digest=" + hex(digest) +
			    " AND account_count=" + std::to_string(plan->accounts.size()) +
			    " AND posting_count=" + std::to_string(plan->postings.size()) +
			    " AND child_count=" + std::to_string(plan->children.size()) +
			    " AND item_event_count=" + std::to_string(plan->item_events.size()) +
			    " AND before_witness_count=" +
			    std::to_string(plan->items_before.size()) +
			    " AND after_witness_count=" + std::to_string(plan->items_after.size()),
		    1))
	{
		if (!errno)
			errno = EILSEQ;
		return false;
	}
	if (meta.operation_id.bytes != operation.bytes)
	{
		errno = EILSEQ;
		return false;
	}
	std::array<uint8_t, AUCTION_RESULT_PAYLOAD_BYTES> canonical{};
	uint64_t durable = 0;
	if (!auction_command_decode_result(bytes(cells[3]).data(), cells[3].size(), result) ||
	    !auction_command_encode_result(*result, &canonical) ||
	    cells[3].size() != canonical.size() ||
	    !std::equal(canonical.begin(), canonical.end(), bytes(cells[3]).begin()) ||
	    !u64(cells[4], &durable) ||
	    durable != std::max({ result->auction_revision, result->wallet_revision,
				  result->bank_revision, result->player_owner_revision,
				  result->auction_owner_revision }))
	{
		errno = EILSEQ;
		return false;
	}
	return true;
}

struct original_listing_forest_values
{
	std::vector<player_item_snapshot> combined;
	std::vector<uint64_t> listing_revisions;
	std::vector<std::pair<size_t, size_t>> ranges;
	std::vector<std::vector<std::string>> custody;
};

bool original_listing_forest(MYSQL *db, const critical_operation_id *lineage,
			     const critical_operation_id *epoch, uint32_t auction_id,
			     uint32_t seller_pid, const critical_operation_id &listing,
			     original_listing_forest_values *out)
{
	economic_frozen_intent intent;
	economic_accounting_plan plan;
	auction_command_result receipt{};
	if (!listing_root_core(db, listing, &intent, &plan, &receipt))
		return false;
	const auto &m = intent.admission.metadata;
	auction_accounting_native_facts native;
	if (!accounting_ok(auction_listing_accounting_observe_native_facts(intent, &native)))
		return false;
	if (m.writer_id != ECONOMIC_WRITER_AUCTION_LISTING ||
	    m.reason != economic_reason::auction_listing ||
	    m.actor_kind != economic_actor_kind::domain || m.actor_id != seller_pid ||
	    (lineage && m.lineage.bytes != lineage->bytes) ||
	    !critical_operation_id_is_zero(m.original_operation_id) || !m.source_event ||
	    m.source_event->kind != economic_source_kind::auction ||
	    m.source_event->source.bytes != listing.bytes ||
	    m.source_event->generation.bytes != listing.bytes || m.source_event->sequence ||
	    m.source_event->slot || receipt.action != auction_action::list ||
	    receipt.event_type != auction_event_type::listed || receipt.auction_id != auction_id ||
	    receipt.seller_pid != seller_pid || receipt.winner_pid || receipt.previous_bidder_pid ||
	    receipt.final_price || receipt.claim_credit_used || receipt.status != 1 ||
	    receipt.auction_revision != 1 || !receipt.player_owner_revision ||
	    receipt.auction_owner_revision != 1 ||
	    receipt.item_count != native.selected_root_count || receipt.wallet_value_delta > 0)
	{
		errno = EILSEQ;
		return false;
	}
	const auto scope = "operation_id=" + hex(listing.bytes);
	std::array<uint8_t, ECONOMIC_SOURCE_EVENT_BYTES> source_bytes{};
	economic_digest intent_digest{}, plan_digest{};
	if (!accounting_ok(economic_source_event_encode(*m.source_event, &source_bytes)) ||
	    !accounting_ok(economic_intent_digest(intent, &intent_digest)) ||
	    !accounting_ok(economic_plan_digest(plan, &plan_digest)))
		return false;
	const auto root = scope + " AND lineage=" + hex(m.lineage.bytes) +
			  " AND epoch=" + hex(m.epoch.bytes) +
			  " AND original_operation_id IS NULL AND accounting_version=" +
			  std::to_string(m.version) +
			  " AND writer_id=" + std::to_string(m.writer_id) +
			  " AND policy_version=" + std::to_string(m.policy_version) +
			  " AND compiler_version=" + std::to_string(m.compiler_version) +
			  " AND actor_kind=" + std::to_string(static_cast<unsigned>(m.actor_kind)) +
			  " AND actor_id=" + std::to_string(m.actor_id) +
			  " AND reason=" + std::to_string(static_cast<unsigned>(m.reason)) +
			  " AND source_event=" + hex(source_bytes) +
			  " AND intent_digest=" + hex(intent_digest) +
			  " AND domain_digest=" + hex(intent.domain_digest) +
			  " AND outcome=1 AND result_code=0 AND realized_price_copper IS NULL";
	if (!exact_count(db, "economic_accounting_operation", root, 1) ||
	    !exact_count(db, "economic_epoch",
			 "lineage=" + hex(m.lineage.bytes) + " AND epoch=" + hex(m.epoch.bytes),
			 1) ||
	    (epoch &&
	     !exact_count(db, "economic_epoch",
			  "lineage=" + hex(lineage->bytes) + " AND epoch=" + hex(epoch->bytes),
			  1)) ||
	    !exact_count(
		    db, "critical_operation_inbox",
		    scope + " AND schema_version=2 AND command_type=" +
			    std::to_string(static_cast<unsigned>(critical_command_type::auction)) +
			    " AND payload_version=2 AND OCTET_LENGTH(command_hash)=32 AND OCTET_LENGTH(keys_hash)=32",
		    1))
		return false;
	const auto claim_error = economic_sql_auction_source_claim_verify_known_metadata(db, m, 0);
	if (claim_error)
	{
		errno = claim_error;
		return false;
	}
	std::array<uint8_t, AUCTION_RESULT_PAYLOAD_BYTES> result_bytes{};
	if (!auction_command_encode_result(receipt, &result_bytes))
	{
		errno = EILSEQ;
		return false;
	}
	if (!exact_count(db, "critical_outbox", scope, 1) ||
	    !exact_count(
		    db, "critical_outbox",
		    scope + " AND event_index=0 AND destination=5 AND event_type=1 AND payload_version=1 AND payload=" +
			    hex(result_bytes),
		    1) ||
	    !exact_count(db, "auction_ledger", scope, 1) ||
	    !exact_count(db, "auction_ledger",
			 scope + " AND auction_id=" + std::to_string(auction_id) +
				 " AND event_type=1 AND actor_pid=" + std::to_string(seller_pid) +
				 " AND counterparty_pid=" + std::to_string(seller_pid) +
				 " AND value_delta=" + std::to_string(receipt.wallet_value_delta) +
				 " AND final_price=0 AND item_count=" +
				 std::to_string(receipt.item_count) + " AND auction_revision=1",
			 1) ||
	    !exact_count(db, "economic_accounting_child", scope, 0) || !plan.children.empty())
		return false;
	// Actual recorded native site is observable; no unseen command header is inferred.
	std::vector<std::string> currency;
	if (!row(db, "SELECT source_site,bank_id FROM currency_ledger WHERE " + scope, 2,
		 &currency))
		return false;
	uint64_t site = 0, bank_id = 0;
	if (!u64(currency[0], &site) ||
	    site > static_cast<unsigned>(critical_source_site::operator_repair) ||
	    !u64(currency[1], &bank_id) || !bank_id)
	{
		errno = EILSEQ;
		return false;
	}
	const auto facts = std::span<const uint8_t>(intent.admission.facts);
	economic_account_key wallet{ m.lineage, economic_account_kind::wallet,
				     fact_number(facts, 0, 8), 0 };
	std::vector<std::string> bank_mapping;
	if (!row(db,
		 "SELECT context_id FROM economic_account_mapping WHERE mapping_id=" +
			 std::to_string(fact_number(facts, 8, 8)) +
			 " AND lineage=" + hex(m.lineage.bytes) + " AND account_kind=" +
			 std::to_string(static_cast<unsigned>(economic_account_kind::bank)) +
			 " AND locator_kind=2 AND native_id=" + std::to_string(bank_id),
		 1, &bank_mapping))
		return false;
	uint64_t context = 0;
	if (!u64(bank_mapping[0], &context) || context > UINT32_MAX)
	{
		errno = EILSEQ;
		return false;
	}
	economic_account_key bank{ m.lineage, economic_account_kind::bank, fact_number(facts, 8, 8),
				   static_cast<uint32_t>(context) };
	if (!baseline_births(db, intent, { wallet, bank }) || !mapping(db, wallet, 1, seller_pid) ||
	    !mapping(db, bank, 2, bank_id))
		return false;
	const auto *wallet_effect = effect(plan, wallet);
	economic_coin_vector delta{};
	if (!wallet_effect || wallet_effect->after != receipt.wallet.amount ||
	    wallet_effect->after_revision != receipt.wallet_revision ||
	    !accounting_ok(
		    economic_coin_delta(wallet_effect->before, wallet_effect->after, &delta)))
	{
		errno = EILSEQ;
		return false;
	}
	int64_t delta_value = 0;
	if (!accounting_ok(economic_coin_value(delta, &delta_value)) ||
	    delta_value != receipt.wallet_value_delta)
	{
		errno = EILSEQ;
		return false;
	}
	std::string cp =
		scope + " AND pid=" + std::to_string(seller_pid) +
		" AND bank_id=" + std::to_string(bank_id) +
		" AND wallet_revision=" + std::to_string(receipt.wallet_revision) +
		" AND bank_revision=" + std::to_string(receipt.bank_revision) +
		" AND reason_type=" +
		std::to_string(static_cast<unsigned>(currency_reason_type::auction_listing)) +
		" AND reason_id=0 AND source_site=" + std::to_string(site);
	const char *names[] = { "copper", "silver", "gold", "platinum" };
	for (size_t i = 0; i < 4; ++i)
		cp += " AND wallet_delta_" + std::string(names[i]) + "=" +
		      std::to_string(delta[i]) + " AND bank_delta_" + names[i] +
		      "=0 AND wallet_after_" + names[i] + "=" +
		      std::to_string(receipt.wallet.amount[i]) + " AND bank_after_" + names[i] +
		      "=" + std::to_string(receipt.bank.amount[i]);
	if (!exact_count(db, "currency_ledger", scope, 1) ||
	    !exact_count(db, "currency_ledger", cp, 1))
		return false;
	unsigned escrow_count = 0;
	for (const auto &e : plan.accounts)
		if (e.key.kind == economic_account_kind::auction_escrow)
		{
			++escrow_count;
			if (!mapping(db, e.key, 4, auction_id, nullptr, &listing))
				return false;
		}
	if (escrow_count != 1)
	{
		errno = EILSEQ;
		return false;
	}
	std::vector<std::vector<std::string>> custody;
	if (!retained_rows(
		    db,
		    "SELECT slot,item_uid,vnum,obj_blob,COALESCE(claim_pid,0),claimed_at IS NOT NULL,item_revision FROM auction_item_custody WHERE auction_id=" +
			    std::to_string(auction_id) + " ORDER BY slot LOCK IN SHARE MODE",
		    7, AUCTION_COMMAND_MAX_ITEMS, &custody) ||
	    custody.size() != native.selected_root_count)
	{
		errno = errno ? errno : EILSEQ;
		return false;
	}
	std::vector<player_item_snapshot> combined;
	std::vector<uint64_t> listing_revisions;
	std::vector<std::pair<size_t, size_t>> ranges;
	for (size_t slot = 0; slot < custody.size(); ++slot)
	{
		auto &r = custody[slot];
		uint64_t slot_number = 0, uid = 0, vnum = 0;
		if (!u64(r[0], &slot_number) || slot_number != slot || !u64(r[1], &uid) ||
		    !u64(r[2], &vnum))
		{
			errno = EILSEQ;
			return false;
		}
		std::vector<player_item_snapshot> tree;
		std::vector<uint64_t> revisions;
		if (!auction_repository_decode_native_tree_blob(bytes(r[3]), &tree, &revisions) ||
		    tree.empty() || tree[0].object_uid != uid ||
		    tree[0].vnum != static_cast<int32_t>(vnum) || uid != receipt.item_uids[slot] ||
		    revisions[0] != receipt.item_revisions[slot] ||
		    combined.size() > PLAYER_SNAPSHOT_MAX_OBJECTS - tree.size())
		{
			errno = EILSEQ;
			return false;
		}
		size_t start = combined.size();
		for (auto &node : tree)
		{
			if (node.parent_index >= 0)
				node.parent_index += static_cast<int32_t>(start);
			combined.push_back(std::move(node));
		}
		listing_revisions.insert(listing_revisions.end(), revisions.begin(),
					 revisions.end());
		ranges.emplace_back(start, combined.size());
	}
	std::vector<uint8_t> combined_bytes;
	economic_digest digest{};
	if (combined.size() != native.selected_node_count ||
	    player_item_snapshot_list_encode(combined, &combined_bytes) !=
		    player_snapshot_codec_result::ok ||
	    !SHA256(combined_bytes.data(), combined_bytes.size(), digest.data()) ||
	    digest != native.selected_digest || plan.item_events.size() != combined.size() ||
	    plan.items_before.size() != combined.size() ||
	    plan.items_after.size() != combined.size() ||
	    !exact_count(db, "item_ownership_ledger", scope, combined.size()) ||
	    !exact_count(db, "economic_accounting_item_reference", scope, combined.size()))
	{
		errno = errno ? errno : EILSEQ;
		return false;
	}
	std::unordered_set<uint64_t> unique;
	size_t slot = 0;
	for (size_t i = 0; i < combined.size(); ++i)
	{
		if (slot + 1 < ranges.size() && i == ranges[slot + 1].first)
			++slot;
		const auto &node = combined[i];
		uint64_t root_uid = combined[ranges[slot].first].object_uid,
			 parent_uid =
				 node.parent_index < 0 ? 0 : combined[node.parent_index].object_uid;
		if (!unique.insert(node.object_uid).second)
		{
			errno = EILSEQ;
			return false;
		}
		auto ev = std::find_if(plan.item_events.begin(), plan.item_events.end(),
				       [&](const auto &e) { return e.event_index == i; });
		auto before = std::find_if(plan.items_before.begin(), plan.items_before.end(),
					   [&](const auto &w) { return w.uid == node.object_uid; });
		auto after = std::find_if(plan.items_after.begin(), plan.items_after.end(),
					  [&](const auto &w) { return w.uid == node.object_uid; });
		if (ev == plan.item_events.end() || before == plan.items_before.end() ||
		    after == plan.items_after.end() || ev->uid != node.object_uid ||
		    ev->child_index ||
		    !economic_item_position_equal(ev->before, before->position) ||
		    !economic_item_position_equal(ev->after, after->position) ||
		    ev->before.owner.type != item_owner_type::player ||
		    ev->before.owner.id != seller_pid || ev->before.owner.context_id ||
		    ev->after.owner.type != item_owner_type::auction ||
		    ev->after.owner.id != auction_id || ev->after.owner.context_id ||
		    ev->before.root_uid != root_uid || ev->after.root_uid != root_uid ||
		    ev->before.parent_uid != parent_uid || ev->after.parent_uid != parent_uid ||
		    ev->before.equipment_slot || ev->after.equipment_slot ||
		    ev->before.state != item_custody_state::active ||
		    ev->after.state != item_custody_state::active ||
		    ev->before.revision == UINT64_MAX ||
		    ev->before.revision + 1 != listing_revisions[i] ||
		    ev->after.revision != listing_revisions[i])
		{
			errno = EILSEQ;
			return false;
		}
		std::string ref =
			scope + " AND line_index=" + std::to_string(i) +
			" AND event_index=" + std::to_string(i) +
			" AND child_index=0 AND item_uid=" + std::to_string(node.object_uid) +
			" AND before_revision=" + std::to_string(ev->before.revision) +
			" AND after_revision=" + std::to_string(ev->after.revision) +
			" AND legacy_operation_id=" + hex(listing.bytes) +
			" AND legacy_event_index=" + std::to_string(i);
		std::string ledger =
			scope + " AND event_index=" + std::to_string(i) +
			" AND item_uid=" + std::to_string(node.object_uid) +
			" AND root_item_uid=" + std::to_string(root_uid) +
			(parent_uid ? " AND parent_item_uid=" + std::to_string(parent_uid) :
				      " AND parent_item_uid IS NULL") +
			" AND from_owner_type=" +
			std::to_string(static_cast<unsigned>(item_owner_type::player)) +
			" AND from_owner_id=" + std::to_string(seller_pid) +
			" AND from_owner_context_id=0 AND to_owner_type=" +
			std::to_string(static_cast<unsigned>(item_owner_type::auction)) +
			" AND to_owner_id=" + std::to_string(auction_id) +
			" AND to_owner_context_id=0 AND item_revision=" +
			std::to_string(listing_revisions[i]) + " AND from_owner_revision=" +
			std::to_string(receipt.player_owner_revision) +
			" AND to_owner_revision=" + std::to_string(receipt.auction_owner_revision) +
			" AND reason_type=" +
			std::to_string(static_cast<unsigned>(item_transfer_reason::auction_list)) +
			" AND reason_id=" + std::to_string(auction_id) +
			" AND source_site=" + std::to_string(site) +
			" AND from_equipment_slot=0 AND to_equipment_slot=0";
		if (!exact_count(db, "economic_accounting_item_reference", ref, 1) ||
		    !exact_count(db, "item_ownership_ledger", ledger, 1))
			return false;
	}
	for (size_t i = receipt.item_count; i < AUCTION_COMMAND_MAX_ITEMS; ++i)
		if (receipt.item_uids[i] || receipt.item_revisions[i])
		{
			errno = EILSEQ;
			return false;
		}
	*out = { std::move(combined), std::move(listing_revisions), std::move(ranges),
		 std::move(custody) };
	return true;
}

bool original_listing_selected(MYSQL *db, const critical_command &claim_command,
			       const economic_frozen_intent *claim_intent,
			       const auction_item_claim_state &claim,
			       const auction_command_payload &payload,
			       const critical_operation_id &listing,
			       std::vector<player_item_snapshot> *out, bool historical,
			       bool whole_listing)
{
	original_listing_forest_values source;
	if (!original_listing_forest(
		    db, claim_intent ? &claim_intent->admission.metadata.lineage : nullptr,
		    claim_intent ? &claim_intent->admission.metadata.epoch : nullptr,
		    claim.auction_id, claim.seller_pid, listing, &source))
		return false;
	auto &combined = source.combined;
	auto &listing_revisions = source.listing_revisions;
	auto &ranges = source.ranges;
	auto &custody = source.custody;
	if (whole_listing)
	{
		*out = std::move(combined);
		return true;
	}
	std::vector<player_item_snapshot> selected;
	for (size_t wanted = 0; wanted < payload.item_count; ++wanted)
	{
		const auto &frozen = claim.rows[wanted];
		size_t original_slot = frozen.slot;
		if (original_slot >= ranges.size() ||
		    combined[ranges[original_slot].first].object_uid != frozen.uid)
		{
			errno = EILSEQ;
			return false;
		}
		uint64_t current_claimant = 0, claimed = 0, custody_revision = 0;
		if ((!historical && (!u64(custody[original_slot][4], &current_claimant) ||
				     !u64(custody[original_slot][5], &claimed) ||
				     !u64(custody[original_slot][6], &custody_revision) ||
				     current_claimant != payload.actor_pid || claimed ||
				     custody_revision != frozen.revision)) ||
		    (payload.action != auction_action::list &&
		     frozen.revision != listing_revisions[ranges[original_slot].first]))
		{
			errno = EILSEQ;
			return false;
		}
		const auto [start, end] = ranges[original_slot];
		const size_t target = selected.size();
		if (!historical &&
		    !exact_count(db, "item_current_owner",
				 "root_item_uid=" + std::to_string(frozen.uid), end - start))
			return false;
		for (size_t i = start; i < end; ++i)
		{
			const auto &node = combined[i];
			uint64_t parent =
				node.parent_index < 0 ? 0 : combined[node.parent_index].object_uid;
			size_t fences = 0;
			for (const auto &f : claim_command.expected_revisions)
				if (f.key.type == critical_entity_type::item &&
				    f.key.id == node.object_uid)
				{
					++fences;
					if (!(historical &&
					      payload.action == auction_action::list) &&
					    f.revision != listing_revisions[i])
					{
						errno = EILSEQ;
						return false;
					}
				}
			if (claim_command.payload_version ==
				    AUCTION_NATIVE_COMMAND_PAYLOAD_VERSION &&
			    fences != 1)
			{
				errno = EILSEQ;
				return false;
			}
			std::string current =
				"item_uid=" + std::to_string(node.object_uid) +
				" AND root_item_uid=" + std::to_string(frozen.uid) +
				(parent ? " AND parent_item_uid=" + std::to_string(parent) :
					  " AND parent_item_uid IS NULL") +
				" AND owner_type=" +
				std::to_string(static_cast<unsigned>(item_owner_type::auction)) +
				" AND owner_id=" + std::to_string(claim.auction_id) +
				" AND owner_context_id=0 AND item_revision=" +
				std::to_string(listing_revisions[i]) +
				" AND vnum=" + std::to_string(node.vnum) + " AND state=1";
			if (!historical && !exact_count(db, "item_current_owner", current, 1))
				return false;
			auto row = node;
			if (row.parent_index >= 0)
				row.parent_index = static_cast<int32_t>(target) + row.parent_index -
						   static_cast<int32_t>(start);
			selected.push_back(std::move(row));
		}
	}
	auction_native_command_context claim_native;
	std::vector<uint8_t> selected_bytes;
	economic_digest selected_digest{};
	if (claim_command.payload_version == AUCTION_NATIVE_COMMAND_PAYLOAD_VERSION &&
	    (auction_native_command_decode(claim_command, &claim_native) !=
		     economic_accounting_error::ok ||
	     selected.size() != claim_native.selected_node_count ||
	     payload.item_count != claim_native.selected_root_count ||
	     player_item_snapshot_list_encode(selected, &selected_bytes) !=
		     player_snapshot_codec_result::ok ||
	     !SHA256(selected_bytes.data(), selected_bytes.size(), selected_digest.data()) ||
	     selected_digest != claim_native.selected_digest))
	{
		errno = EILSEQ;
		return false;
	}
	*out = std::move(selected);
	return true;
}

bool native_retained_known_root(MYSQL *db, const economic_frozen_intent &intent,
				const auction_command_result &receipt)
{
	const auto &m = intent.admission.metadata;
	std::array<uint8_t, ECONOMIC_SOURCE_EVENT_BYTES> source{};
	economic_digest digest{};
	std::vector<uint8_t> canonical;
	if (!m.source_event ||
	    !accounting_ok(economic_source_event_encode(*m.source_event, &source)) ||
	    !accounting_ok(economic_intent_digest(intent, &digest)) ||
	    !accounting_ok(economic_intent_encode(intent, &canonical)))
		return false;
	const auto scope = "operation_id=" + hex(m.operation_id.bytes);
	const auto root =
		scope + " AND lineage=" + hex(m.lineage.bytes) +
		" AND epoch=" + hex(m.epoch.bytes) +
		(critical_operation_id_is_zero(m.original_operation_id) ?
			 " AND original_operation_id IS NULL" :
			 " AND original_operation_id=" + hex(m.original_operation_id.bytes)) +
		" AND accounting_version=" + std::to_string(m.version) +
		" AND writer_id=" + std::to_string(m.writer_id) +
		" AND policy_version=" + std::to_string(m.policy_version) +
		" AND compiler_version=" + std::to_string(m.compiler_version) +
		" AND actor_kind=" + std::to_string(static_cast<unsigned>(m.actor_kind)) +
		" AND actor_id=" + std::to_string(m.actor_id) +
		" AND reason=" + std::to_string(static_cast<unsigned>(m.reason)) +
		" AND source_event=" + hex(source) + " AND intent_digest=" + hex(digest) +
		" AND domain_digest=" + hex(intent.domain_digest) +
		" AND canonical_intent=" + hex(canonical) + " AND outcome=1 AND result_code=0";
	std::array<uint8_t, AUCTION_RESULT_PAYLOAD_BYTES> result{};
	if (!auction_command_encode_result(receipt, &result) ||
	    !exact_count(db, "economic_accounting_operation", root, 1) ||
	    !exact_count(db, "economic_epoch",
			 "lineage=" + hex(m.lineage.bytes) + " AND epoch=" + hex(m.epoch.bytes),
			 1) ||
	    !exact_count(db, "critical_outbox", scope, 1) ||
	    !exact_count(
		    db, "critical_outbox",
		    scope + " AND event_index=0 AND destination=5 AND event_type=1 AND payload_version=1 AND payload=" +
			    hex(result),
		    1))
	{
		if (!errno)
			errno = EILSEQ;
		return false;
	}
	const auto error = economic_sql_auction_source_claim_verify_known_metadata(db, m, 0);
	if (error)
	{
		errno = error;
		return false;
	}
	return true;
}

bool native_retained_selected(MYSQL *db, const critical_command &cmd,
			      const economic_frozen_intent &intent,
			      const auction_item_claim_state &claim,
			      const auction_command_payload &payload,
			      const economic_accounting_plan &plan,
			      std::vector<player_item_snapshot> *selected)
{
	std::vector<player_item_snapshot> values;
	if (payload.action == auction_action::claim_item)
	{
		economic_frozen_intent terminal_intent;
		economic_accounting_plan terminal_plan;
		auction_command_result terminal{};
		std::vector<std::string> schema;
		uint64_t version = 0;
		if (!row(db,
			 "SELECT schema_version FROM critical_operation_inbox WHERE operation_id=" +
				 hex(claim.claim_source_operation.bytes),
			 1, &schema) ||
		    !u64(schema[0], &version))
			return false;
		if (version == CRITICAL_COMMAND_ACCOUNTING_SCHEMA_VERSION &&
		    (!known_terminal_creator(db, claim.claim_source_operation, &terminal_intent,
					     &terminal_plan, &terminal) ||
		     terminal_intent.admission.metadata.lineage.bytes !=
			     intent.admission.metadata.lineage.bytes ||
		     terminal_intent.admission.metadata.original_operation_id.bytes !=
			     claim.listing_operation.bytes ||
		     !native_retained_known_root(db, terminal_intent, terminal)))
			return false;
	}
	if (!original_listing_selected(db, cmd, &intent, claim, payload, claim.listing_operation,
				       &values, true, false))
		return false;
	economic_frozen_intent source_intent;
	economic_accounting_plan source_plan;
	auction_command_result source_result{};
	if (!listing_root_core(db, claim.listing_operation, &source_intent, &source_plan,
			       &source_result))
		return false;
	if (plan.items_before.size() != values.size() || plan.items_after.size() != values.size() ||
	    plan.item_events.size() != values.size())
	{
		errno = EILSEQ;
		return false;
	}
	std::unordered_set<uint64_t> uids;
	for (size_t i = 0; i < values.size(); ++i)
	{
		const auto uid = values[i].object_uid;
		const auto before = std::find_if(plan.items_before.begin(), plan.items_before.end(),
						 [&](const auto &w) { return w.uid == uid; });
		const auto after = std::find_if(plan.items_after.begin(), plan.items_after.end(),
						[&](const auto &w) { return w.uid == uid; });
		if (!uids.insert(uid).second || before == plan.items_before.end() ||
		    after == plan.items_after.end() || plan.item_events[i].uid != uid ||
		    plan.item_events[i].event_index != i || plan.item_events[i].child_index ||
		    !economic_item_position_equal(plan.item_events[i].before, before->position) ||
		    !economic_item_position_equal(plan.item_events[i].after, after->position))
		{
			errno = EILSEQ;
			return false;
		}
		const auto &original = payload.action == auction_action::list ?
					       source_plan.items_before :
					       source_plan.items_after;
		const auto source = std::find_if(original.begin(), original.end(),
						 [&](const auto &w) { return w.uid == uid; });
		if (source == original.end() ||
		    !economic_item_position_equal(source->position, before->position))
		{
			errno = EILSEQ;
			return false;
		}
		const critical_entity_key key{ critical_entity_type::item, uid };
		if (std::count_if(cmd.keys.begin(), cmd.keys.end(), [&](const auto &k)
				  { return critical_entity_key_equal(k, key); }) != 1 ||
		    std::count_if(cmd.expected_revisions.begin(), cmd.expected_revisions.end(),
				  [&](const auto &f) {
					  return critical_entity_key_equal(f.key, key) &&
						 f.revision == source->position.revision;
				  }) != 1)
		{
			errno = EILSEQ;
			return false;
		}
	}
	*selected = std::move(values);
	return true;
}

// Settlement stages root entitlement without descendant ownership mutations.
// Authenticate original full native listing closure separately; no synthetic
// economic events/witnesses are introduced to fill a descendant vector.
bool native_retained_auction_forest(MYSQL *db, const critical_command &cmd,
				    const economic_frozen_intent &intent,
				    const auction_settlement_listing &listing,
				    const economic_accounting_plan &plan)
{
	std::vector<std::string> version;
	if (!row(db,
		 "SELECT schema_version,payload_version FROM critical_operation_inbox WHERE operation_id=" +
			 hex(listing.listing_operation.bytes) +
			 " AND status=1 AND result_code=0 AND failure_stage=0 AND committed_at IS NOT NULL LOCK IN SHARE MODE",
		 2, &version))
		return false;
	uint64_t schema = 0, payload_version = 0;
	if (!u64(version[0], &schema) || !u64(version[1], &payload_version))
	{
		errno = EILSEQ;
		return false;
	}
	// Existing historical root-only producer evidence is kept on its original
	// path. It never acquires ANF2 or literal authority by trying ACT2 decoding.
	if (payload_version == AUCTION_COMMAND_PAYLOAD_VERSION)
		return schema == CRITICAL_COMMAND_SCHEMA_VERSION ||
		       schema == CRITICAL_COMMAND_ACCOUNTING_SCHEMA_VERSION;
	if (schema != CRITICAL_COMMAND_ACCOUNTING_SCHEMA_VERSION ||
	    payload_version != AUCTION_NATIVE_COMMAND_PAYLOAD_VERSION)
	{
		errno = EILSEQ;
		return false;
	}
	auction_item_claim_state original;
	original.auction_id = listing.auction_id;
	original.seller_pid = listing.seller_pid;
	original.listing_operation = listing.listing_operation;
	auction_command_payload payload{};
	if (!auction_command_decode_payload(cmd, &payload))
	{
		errno = EILSEQ;
		return false;
	}
	std::vector<player_item_snapshot> values;
	if (!original_listing_selected(db, cmd, &intent, original, payload,
				       listing.listing_operation, &values, true, true))
		return false;
	economic_frozen_intent source_intent;
	economic_accounting_plan source_plan;
	auction_command_result source_result{};
	if (!listing_root_core(db, listing.listing_operation, &source_intent, &source_plan,
			       &source_result))
		return false;
	size_t roots = 0;
	for (const auto &literal : values)
	{
		const auto source = std::find_if(source_plan.items_after.begin(),
						 source_plan.items_after.end(), [&](const auto &w)
						 { return w.uid == literal.object_uid; });
		const critical_entity_key key{ critical_entity_type::item, literal.object_uid };
		if (source == source_plan.items_after.end() ||
		    std::count_if(cmd.keys.begin(), cmd.keys.end(), [&](const auto &k)
				  { return critical_entity_key_equal(k, key); }) != 1 ||
		    std::count_if(cmd.expected_revisions.begin(), cmd.expected_revisions.end(),
				  [&](const auto &f) {
					  return critical_entity_key_equal(f.key, key) &&
						 f.revision == source->position.revision;
				  }) != 1)
		{
			errno = EILSEQ;
			return false;
		}
		if (literal.parent_index != PLAYER_SNAPSHOT_NO_PARENT)
			continue;
		if (payload.action != auction_action::bid)
		{
			if (roots >= listing.item_count ||
			    listing.items[roots].uid != literal.object_uid ||
			    listing.items[roots].revision != source->position.revision ||
			    listing.items[roots].slot != roots ||
			    listing.items[roots].vnum != literal.vnum ||
			    listing.items[roots].claimed || listing.items[roots].claim_pid)
			{
				errno = EILSEQ;
				return false;
			}
			const auto witness = std::find_if(plan.items_before.begin(),
							  plan.items_before.end(),
							  [&](const auto &w)
							  { return w.uid == literal.object_uid; });
			if (witness == plan.items_before.end() ||
			    !economic_item_position_equal(witness->position, source->position))
			{
				errno = EILSEQ;
				return false;
			}
		}
		++roots;
	}
	if (payload.action != auction_action::bid &&
	    (roots != listing.item_count || plan.items_before.size() != roots ||
	     plan.items_after.size() != roots))
	{
		errno = EILSEQ;
		return false;
	}
	return true;
}
}
#endif
unsigned int economic_sql_auction_verify_retained(MYSQL *db, const critical_command &cmd,
						  uint32_t code, critical_failure_stage stage,
						  uint64_t durable, std::span<const uint8_t> result)
{
#ifdef __NO_MYSQL__
	(void)db;
	(void)cmd;
	(void)code;
	(void)stage;
	(void)durable;
	(void)result;
	return ENOTSUP;
#else
	if (!db || !(db->server_status & SERVER_STATUS_IN_TRANS))
		return EINVAL;
	using flag = std::remove_pointer_t<decltype(MYSQL_BIND{}.is_null)>;
	flag reconnect = false;
	if (mysql_get_option(db, MYSQL_OPT_RECONNECT, &reconnect))
		return mysql_errno(db) ? mysql_errno(db) : static_cast<unsigned int>(EIO);
	if (reconnect)
		return EPERM;
	auto session = mysql_thread_id(db);
	errno = 0;
	try
	{
		if (cmd.schema_version != 2 || cmd.type != critical_command_type::auction ||
		    stage != critical_failure_stage::none)
			return EPROTONOSUPPORT;
		economic_frozen_intent intent;
		auction_command_payload payload{};
		auction_bid_accounting_listing bid;
		auction_bid_accounting_accounts bidkeys;
		auction_settlement_listing listing;
		auction_settlement_accounts settlementkeys;
		auction_item_claim_state item_claim;
		economic_account_key wallet, bank, claim_key;
		auto typed = auction_bid_accounting_decode(cmd, &intent, &payload, &bid, &bidkeys);
		if (typed == economic_accounting_error::capacity)
			return ENOMEM;
		if (typed != economic_accounting_error::ok)
			typed = auction_settlement_accounting_decode(cmd, &intent, &payload,
								     &listing, &settlementkeys);
		if (typed != economic_accounting_error::ok &&
		    typed != economic_accounting_error::capacity)
			typed = auction_listing_accounting_decode(cmd, &intent, &payload, &wallet,
								  &bank);
		if (typed != economic_accounting_error::ok &&
		    typed != economic_accounting_error::capacity)
			typed = auction_item_claim_accounting_decode(cmd, &intent, &payload,
								     &item_claim, &wallet, &bank);
		if (typed != economic_accounting_error::ok &&
		    typed != economic_accounting_error::capacity)
			typed = auction_money_claim_accounting_decode(cmd, &intent, &payload,
								      &wallet, &bank, &claim_key);
		if (typed == economic_accounting_error::capacity)
			return ENOMEM;
		if (typed != economic_accounting_error::ok)
			return typed == economic_accounting_error::capacity ? ENOMEM :
									      EPROTONOSUPPORT;
		auction_command_result r{};
		std::array<uint8_t, AUCTION_RESULT_PAYLOAD_BYTES> canonical{};
		if (!auction_command_decode_result(result.data(), result.size(), &r) ||
		    !auction_command_encode_result(r, &canonical) ||
		    result.size() != canonical.size() ||
		    !std::equal(canonical.begin(), canonical.end(), result.begin()) ||
		    durable != std::max({ r.auction_revision, r.wallet_revision, r.bank_revision,
					  r.player_owner_revision, r.auction_owner_revision }))
			return errno ? errno : EILSEQ;
		if (r.action != payload.action ||
		    (!code && payload.action != auction_action::bid && r.claim_credit_used))
			return errno ? errno : EILSEQ;
		std::vector<uint8_t> encoded;
		if (critical_command_encode(cmd, &encoded) != critical_command_codec_result::ok)
			return errno ? errno : EILSEQ;
		economic_digest hash{}, keyhash{}, intenthash{};
		SHA256(encoded.data(), encoded.size(), hash.data());
		std::vector<uint8_t> keybytes;
		for (const auto &k : cmd.keys)
		{
			keybytes.push_back(static_cast<uint8_t>(k.type));
			for (unsigned i = 0; i < 8; ++i)
				keybytes.push_back(static_cast<uint8_t>(k.id >> (8 * i)));
		}
		SHA256(keybytes.data(), keybytes.size(), keyhash.data());
		const auto scope = "operation_id=" + hex(cmd.operation_id.bytes);
		if (!exact_count(
			    db, "critical_operation_inbox",
			    scope + " AND command_hash=" + hex(hash) + " AND keys_hash=" +
				    hex(keyhash) + " AND schema_version=2 AND command_type=" +
				    std::to_string(static_cast<unsigned>(cmd.type)) +
				    " AND payload_version=" + std::to_string(cmd.payload_version) +
				    " AND status=1 AND committed_at IS NOT NULL AND result_code=" +
				    std::to_string(code) +
				    " AND failure_stage=0 AND durable_revision=" +
				    std::to_string(durable) + " AND result_payload=" + hex(result),
			    1))
			return errno ? errno : EILSEQ;
		std::vector<economic_account_key> frozen_keys;
		if (payload.action == auction_action::bid)
			frozen_keys = { bidkeys.wallet,		bidkeys.bank,
					bidkeys.escrow,		bidkeys.bidder_claim,
					bidkeys.previous_claim, bidkeys.seller_claim };
		else if (payload.action == auction_action::list ||
			 payload.action == auction_action::claim_item ||
			 payload.action == auction_action::claim_money)
			frozen_keys = { wallet, bank, claim_key };
		else
			frozen_keys = { settlementkeys.escrow, settlementkeys.seller_claim,
					settlementkeys.actor_wallet, settlementkeys.actor_bank };
		if (!code && payload.action == auction_action::claim_money &&
		    !money_source_baselines(db, cmd))
			return errno ? errno : EILSEQ;
		if (!baseline_births(db, intent, frozen_keys))
			return errno ? errno : EILSEQ;
		const auto &meta = intent.admission.metadata;
		if (!exact_count(db, "economic_epoch",
				 "lineage=" + hex(meta.lineage.bytes) +
					 " AND epoch=" + hex(meta.epoch.bytes),
				 1))
			return errno ? errno : ESTALE;
		std::array<uint8_t, ECONOMIC_SOURCE_EVENT_BYTES> src{};
		if (!meta.source_event ||
		    economic_source_event_encode(*meta.source_event, &src) !=
			    economic_accounting_error::ok ||
		    !accounting_ok(economic_intent_digest(intent, &intenthash)))
			return errno ? errno : EILSEQ;
		economic_accounting_plan plan;
		std::vector<std::string> cells;
		economic_digest planhash{};
		std::string root =
			scope + " AND lineage=" + hex(meta.lineage.bytes) +
			" AND epoch=" + hex(meta.epoch.bytes) +
			(critical_operation_id_is_zero(meta.original_operation_id) ?
				 " AND original_operation_id IS NULL" :
				 " AND original_operation_id=" +
					 hex(meta.original_operation_id.bytes)) +
			" AND accounting_version=" + std::to_string(meta.version) +
			" AND writer_id=" + std::to_string(meta.writer_id) +
			" AND policy_version=" + std::to_string(meta.policy_version) +
			" AND compiler_version=" + std::to_string(meta.compiler_version) +
			" AND actor_kind=" +
			std::to_string(static_cast<unsigned>(meta.actor_kind)) +
			" AND actor_id=" + std::to_string(meta.actor_id) +
			" AND reason=" + std::to_string(static_cast<unsigned>(meta.reason)) +
			" AND source_event=" + hex(src) + " AND intent_digest=" + hex(intenthash) +
			" AND domain_digest=" + hex(intent.domain_digest) +
			" AND canonical_intent=" + hex(cmd.accounting_intent) +
			" AND result_code=" + std::to_string(code);
		if (code)
		{
			root += " AND outcome=2 AND canonical_plan IS NULL AND plan_digest IS NULL AND realized_price_copper IS NULL AND account_count=0 AND posting_count=0 AND child_count=0 AND item_event_count=0 AND before_witness_count=0 AND after_witness_count=0";
			if (!exact_count(db, "economic_accounting_operation", root, 1))
				return errno ? errno : EILSEQ;
			const auto source_claim_error =
				economic_sql_auction_source_claim_verify(db, intent, code);
			if (source_claim_error)
				return source_claim_error;
			for (const char *table :
			     { "economic_accounting_account_effect",
			       "economic_accounting_coin_posting", "economic_accounting_child",
			       "economic_accounting_item_reference",
			       "economic_accounting_source_claim", "auction_ledger",
			       "currency_ledger", "item_ownership_ledger", "critical_outbox" })
				if (!exact_count(db, table, scope, 0))
					return errno ? errno : EILSEQ;
			if (!exact_count(db, "economic_pending_claim_consumption",
					 "spending_operation_id=" + hex(cmd.operation_id.bytes), 0))
				return errno ? errno : EILSEQ;
			if (!exact_count(db, "economic_pending_claim_source",
					 "source_operation_id=" + hex(cmd.operation_id.bytes), 0) ||
			    !exact_count(db, "economic_account_mapping",
					 "creating_operation_id=" + hex(cmd.operation_id.bytes) +
						 " OR retiring_operation_id=" +
						 hex(cmd.operation_id.bytes),
					 0))
				return errno ? errno : EILSEQ;
		}
		else
		{
			if (!row(db,
				 "SELECT canonical_plan,plan_digest FROM economic_accounting_operation WHERE " +
					 scope + " LOCK IN SHARE MODE",
				 2, &cells))
				return errno ? errno : EILSEQ;
			if (!accounting_ok(economic_plan_decode(bytes(cells[0]), &plan)) ||
			    !accounting_ok(economic_plan_digest(plan, &planhash)) ||
			    cells[1].size() != planhash.size() ||
			    !std::equal(planhash.begin(), planhash.end(), bytes(cells[1]).begin()))
				return errno ? errno : EILSEQ;
			root += " AND outcome=1 AND canonical_plan=" + hex(bytes(cells[0])) +
				" AND plan_digest=" + hex(planhash) +
				" AND account_count=" + std::to_string(plan.accounts.size()) +
				" AND posting_count=" + std::to_string(plan.postings.size()) +
				" AND child_count=" + std::to_string(plan.children.size()) +
				" AND item_event_count=" + std::to_string(plan.item_events.size()) +
				" AND before_witness_count=" +
				std::to_string(plan.items_before.size()) +
				" AND after_witness_count=" +
				std::to_string(plan.items_after.size()) +
				(payload.action == auction_action::bid ?
					 " AND realized_price_copper=" +
						 std::to_string(r.final_price) :
					 " AND realized_price_copper IS NULL");
			if (!exact_count(db, "economic_accounting_operation", root, 1) ||
			    !normalized_financial_rows(db, cmd.operation_id, plan))
				return errno ? errno : EILSEQ;
			const auto source_claim_error =
				economic_sql_auction_source_claim_verify(db, intent, code);
			if (source_claim_error)
				return source_claim_error;
			const bool three = payload.action == auction_action::list ||
					   payload.action == auction_action::claim_item ||
					   payload.action == auction_action::claim_money;
			if (!plan.children.empty() || (!three && !plan.item_events.empty()))
				return errno ? errno : EILSEQ;
			if (!three &&
			    (!exact_count(db, "economic_accounting_child", scope, 0) ||
			     !exact_count(db, "economic_accounting_item_reference", scope, 0) ||
			     !exact_count(db, "item_ownership_ledger", scope, 0)))
				return errno ? errno : EILSEQ;
			economic_accounting_plan regenerated;
			if (!(three ? regenerate_three(db, cmd, intent, plan, r, &regenerated) :
				      regenerate(db, cmd, intent, plan, r, &regenerated)) ||
			    !native_rows(db, cmd, r, plan))
				return errno ? errno : EILSEQ;
			if (!exact_count(db, "critical_outbox", scope, 1) ||
			    !exact_count(
				    db, "critical_outbox",
				    scope + " AND event_index=0 AND destination=5 AND event_type=1 AND payload_version=1 AND payload=" +
					    hex(result),
				    1))
				return errno ? errno : EILSEQ;
		}
		if (mysql_thread_id(db) != session || !(db->server_status & SERVER_STATUS_IN_TRANS))
			return ENOTCONN;
		return 0;
	}
	catch (const std::bad_alloc &)
	{
		return ENOMEM;
	}
#endif
}

unsigned int auction_repository_read_original_native_selected(
	MYSQL *db, const critical_command &original_claim,
	const critical_operation_id &original_listing_operation,
	std::vector<player_item_snapshot> *selected) noexcept
{
#ifdef __NO_MYSQL__
	(void)db;
	(void)original_claim;
	(void)original_listing_operation;
	(void)selected;
	return ENOTSUP;
#else
	try
	{
		economic_sql_auction_source_claim_detail::session guard{ db };
		auto error = guard.check(true);
		if (error)
			return error;
		std::vector<player_item_snapshot> value;
		auto read = [&]() -> unsigned int
		{
			if (!selected || critical_operation_id_is_zero(original_listing_operation))
				return EINVAL;
			const bool accepted = original_claim.schema_version ==
						      CRITICAL_COMMAND_ACCOUNTING_SCHEMA_VERSION &&
					      original_claim.payload_version ==
						      AUCTION_NATIVE_COMMAND_PAYLOAD_VERSION;
			const bool fresh =
				original_claim.schema_version == CRITICAL_COMMAND_SCHEMA_VERSION &&
				original_claim.payload_version == AUCTION_COMMAND_PAYLOAD_VERSION &&
				!original_claim.accepted_at_usec &&
				!original_claim.publication_required &&
				original_claim.accounting_intent.empty();
			if (!accepted && !fresh)
				return EPROTONOSUPPORT;
			economic_frozen_intent intent;
			auction_command_payload payload{};
			auction_item_claim_state frozen;
			economic_account_key wallet, bank;
			if (accepted)
			{
				auto decoded = auction_item_claim_accounting_decode(
					original_claim, &intent, &payload, &frozen, &wallet, &bank);
				if (decoded != economic_accounting_error::ok)
					return decoded == economic_accounting_error::capacity ?
						       ENOMEM :
						       EILSEQ;
			}
			else if (!auction_command_decode_payload(original_claim, &payload))
				return EILSEQ;
			if (payload.action != auction_action::claim_item || !payload.actor_pid ||
			    !payload.auction_id || !payload.item_count ||
			    payload.item_count > AUCTION_COMMAND_MAX_ITEMS)
				return EINVAL;
			errno = 0;
			std::vector<std::string> current;
			if (!row(db,
				 "SELECT seller_pid,winning_bidder_pid,status+0,custody_state,auction_revision,listing_operation_id FROM auctions WHERE id=" +
					 std::to_string(payload.auction_id) + " FOR UPDATE",
				 6, &current))
				return errno ? errno : EILSEQ;
			auction_item_claim_state claim;
			claim.auction_id = payload.auction_id;
			claim.claimant_pid = payload.actor_pid;
			claim.item_count = payload.item_count;
			uint64_t n[5]{};
			for (size_t i = 0; i < 5; ++i)
				if (!u64(current[i], &n[i]) || (i < 4 && n[i] > UINT32_MAX))
					return EILSEQ;
			claim.seller_pid = n[0];
			claim.winner_pid = n[1];
			claim.status = n[2];
			claim.custody_state = n[3];
			claim.auction_revision = n[4];
			claim.listing_operation = original_listing_operation;
			if (current[5].size() != 16 ||
			    !std::equal(original_listing_operation.bytes.begin(),
					original_listing_operation.bytes.end(),
					bytes(current[5]).begin()) ||
			    !claim.seller_pid || !claim.auction_revision ||
			    claim.custody_state != 1 || (claim.status != 2 && claim.status != 3))
				return EILSEQ;
			if (!row(db,
				 "SELECT operation_id,event_type,auction_revision FROM auction_ledger WHERE auction_id=" +
					 std::to_string(claim.auction_id) +
					 " AND event_type IN (3,4,7) ORDER BY auction_revision DESC LIMIT 1 FOR UPDATE",
				 3, &current))
				return errno ? errno : EILSEQ;
			uint64_t event = 0, terminal_revision = 0;
			if (current[0].size() != 16 || !u64(current[1], &event) ||
			    !u64(current[2], &terminal_revision) ||
			    terminal_revision > claim.auction_revision)
				return EILSEQ;
			std::copy(bytes(current[0]).begin(), bytes(current[0]).end(),
				  claim.claim_source_operation.bytes.begin());
			if ((event == 3 && (claim.status != 2 || !claim.winner_pid ||
					    payload.actor_pid != claim.winner_pid)) ||
			    (event == 4 && (claim.status != 2 || claim.winner_pid ||
					    payload.actor_pid != claim.seller_pid)) ||
			    (event == 7 &&
			     (claim.status != 3 || payload.actor_pid != claim.seller_pid)))
				return EILSEQ;
			economic_frozen_intent terminal_intent;
			economic_accounting_plan terminal_plan;
			auction_command_result terminal_receipt{};
			if (!known_terminal_creator(db, claim.claim_source_operation,
						    &terminal_intent, &terminal_plan,
						    &terminal_receipt))
				return errno ? errno : EILSEQ;
			const auto &terminal_meta = terminal_intent.admission.metadata;
			if (terminal_meta.original_operation_id.bytes !=
				    original_listing_operation.bytes ||
			    terminal_receipt.auction_id != claim.auction_id ||
			    terminal_receipt.auction_revision != terminal_revision ||
			    terminal_receipt.event_type != static_cast<auction_event_type>(event) ||
			    terminal_receipt.status != claim.status ||
			    terminal_receipt.seller_pid != claim.seller_pid ||
			    terminal_receipt.winner_pid != claim.winner_pid)
				return EILSEQ;
			std::array<uint8_t, ECONOMIC_SOURCE_EVENT_BYTES> terminal_source{};
			economic_digest terminal_digest{};
			if (!terminal_meta.source_event ||
			    !accounting_ok(economic_source_event_encode(*terminal_meta.source_event,
									&terminal_source)) ||
			    !accounting_ok(
				    economic_intent_digest(terminal_intent, &terminal_digest)))
				return errno ? errno : EILSEQ;
			const auto terminal_scope =
				"operation_id=" + hex(claim.claim_source_operation.bytes);
			const auto terminal_root =
				terminal_scope +
				" AND lineage=" + hex(terminal_meta.lineage.bytes) +
				" AND epoch=" + hex(terminal_meta.epoch.bytes) +
				" AND original_operation_id=" +
				hex(terminal_meta.original_operation_id.bytes) +
				" AND accounting_version=" + std::to_string(terminal_meta.version) +
				" AND writer_id=" + std::to_string(terminal_meta.writer_id) +
				" AND policy_version=" +
				std::to_string(terminal_meta.policy_version) +
				" AND compiler_version=" +
				std::to_string(terminal_meta.compiler_version) +
				" AND actor_kind=" +
				std::to_string(static_cast<unsigned>(terminal_meta.actor_kind)) +
				" AND actor_id=" + std::to_string(terminal_meta.actor_id) +
				" AND reason=" +
				std::to_string(static_cast<unsigned>(terminal_meta.reason)) +
				" AND source_event=" + hex(terminal_source) +
				" AND intent_digest=" + hex(terminal_digest) +
				" AND domain_digest=" + hex(terminal_intent.domain_digest);
			if (!exact_count(db, "economic_accounting_operation", terminal_root, 1) ||
			    !exact_count(db, "economic_epoch",
					 "lineage=" + hex(terminal_meta.lineage.bytes) +
						 " AND epoch=" + hex(terminal_meta.epoch.bytes),
					 1))
				return errno ? errno : EILSEQ;
			std::array<uint8_t, AUCTION_RESULT_PAYLOAD_BYTES> terminal_bytes{};
			if (!auction_command_encode_result(terminal_receipt, &terminal_bytes))
				return EILSEQ;
			if (!exact_count(db, "critical_outbox", terminal_scope, 1) ||
			    !exact_count(
				    db, "critical_outbox",
				    terminal_scope +
					    " AND event_index=0 AND destination=5 AND event_type=1 AND payload_version=1 AND payload=" +
					    hex(terminal_bytes),
				    1))
				return errno ? errno : EILSEQ;
			std::unordered_set<uint64_t> roots;
			std::array<size_t, AUCTION_COMMAND_MAX_ITEMS> lock_order{};
			for (size_t i = 0; i < payload.item_count; ++i)
			{
				lock_order[i] = i;
				if (!roots.insert(payload.items[i].item_uid).second)
					return EILSEQ;
			}
			std::sort(lock_order.begin(), lock_order.begin() + payload.item_count,
				  [&](size_t a, size_t b) {
					  return payload.items[a].item_uid <
						 payload.items[b].item_uid;
				  });
			for (size_t lock = 0; lock < payload.item_count; ++lock)
			{
				const size_t i = lock_order[lock];
				const auto &request = payload.items[i];
				if (!row(db,
					 "SELECT slot,item_revision,vnum,COALESCE(claim_pid,0),claimed_at IS NOT NULL FROM auction_item_custody WHERE auction_id=" +
						 std::to_string(claim.auction_id) +
						 " AND item_uid=" +
						 std::to_string(request.item_uid) + " FOR UPDATE",
					 5, &current))
					return errno ? errno : EILSEQ;
				for (size_t f = 0; f < 5; ++f)
					if (!u64(current[f], &n[f]))
						return EILSEQ;
				if (n[0] > UINT16_MAX || n[1] != request.expected_item_revision ||
				    n[2] != static_cast<uint64_t>(request.vnum) ||
				    n[3] != payload.actor_pid || n[4])
					return EILSEQ;
				claim.rows[i] = { request.item_uid,
						  n[1],
						  static_cast<uint16_t>(n[0]),
						  request.vnum,
						  payload.actor_pid,
						  false };
				const auto witness = std::find_if(
					terminal_plan.items_before.begin(),
					terminal_plan.items_before.end(),
					[&](const auto &w) { return w.uid == request.item_uid; });
				if (witness == terminal_plan.items_before.end() ||
				    witness->position.owner.type != item_owner_type::auction ||
				    witness->position.owner.id != claim.auction_id ||
				    witness->position.owner.context_id ||
				    witness->position.root_uid != request.item_uid ||
				    witness->position.parent_uid ||
				    witness->position.equipment_slot ||
				    witness->position.revision != request.expected_item_revision ||
				    witness->position.state != item_custody_state::active)
					return EILSEQ;
			}
			if (accepted)
			{
				if (frozen.auction_id != claim.auction_id ||
				    frozen.seller_pid != claim.seller_pid ||
				    frozen.winner_pid != claim.winner_pid ||
				    frozen.claimant_pid != claim.claimant_pid ||
				    frozen.status != claim.status ||
				    frozen.custody_state != claim.custody_state ||
				    frozen.auction_revision != claim.auction_revision ||
				    frozen.listing_operation.bytes !=
					    claim.listing_operation.bytes ||
				    frozen.claim_source_operation.bytes !=
					    claim.claim_source_operation.bytes ||
				    intent.admission.metadata.lineage.bytes !=
					    terminal_meta.lineage.bytes)
					return EILSEQ;
				for (size_t i = 0; i < claim.item_count; ++i)
					if (frozen.rows[i].uid != claim.rows[i].uid ||
					    frozen.rows[i].revision != claim.rows[i].revision ||
					    frozen.rows[i].slot != claim.rows[i].slot ||
					    frozen.rows[i].vnum != claim.rows[i].vnum ||
					    frozen.rows[i].claim_pid != claim.rows[i].claim_pid ||
					    frozen.rows[i].claimed)
						return EILSEQ;
			}
			if (!original_listing_selected(db, original_claim,
						       accepted ? &intent : &terminal_intent, claim,
						       payload, original_listing_operation, &value))
			{
				const auto session_error = guard.check();
				return session_error ?
					       session_error :
					       static_cast<unsigned>(errno ? errno : EILSEQ);
			}
			return 0;
		};
		error = read();
		const auto session_error = guard.check();
		if (session_error)
			return session_error;
		if (error)
			return error;
		selected->swap(value);
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

static unsigned int
auction_read_native_listing_source(MYSQL *db, const critical_command &command,
				   const critical_operation_id *preparation_lineage,
				   const critical_operation_id *preparation_epoch,
				   economic_sql_auction_native_listing_source *output) noexcept
{
#ifdef __NO_MYSQL__
	(void)db;
	(void)command;
	(void)output;
	(void)preparation_lineage;
	(void)preparation_epoch;
	return ENOTSUP;
#else
	try
	{
		economic_sql_auction_source_claim_detail::session guard{ db };
		auto error = guard.check(true);
		if (error)
			return error;
		economic_sql_auction_native_listing_source result;
		auto verify = [&]() -> unsigned int
		{
			if (!output)
				return EINVAL;
			if (command.payload_version != AUCTION_NATIVE_COMMAND_PAYLOAD_VERSION ||
			    (preparation_lineage ?
				     command.schema_version != CRITICAL_COMMAND_SCHEMA_VERSION ||
					     !command.accounting_intent.empty() ||
					     command.publication_required :
				     command.schema_version !=
					     CRITICAL_COMMAND_ACCOUNTING_SCHEMA_VERSION))
				return EPROTONOSUPPORT;
			economic_frozen_intent intent;
			auction_command_payload payload{};
			auction_bid_accounting_listing bid;
			auction_bid_accounting_accounts bid_accounts;
			auction_settlement_listing listing;
			auction_settlement_accounts settlement_accounts;
			if (preparation_lineage)
			{
				if (!preparation_epoch ||
				    critical_operation_id_is_zero(*preparation_lineage) ||
				    critical_operation_id_is_zero(*preparation_epoch))
					return EINVAL;
				auction_native_command_context native;
				if (auction_native_command_decode(command, &native) !=
				    economic_accounting_error::ok)
					return EILSEQ;
				payload = native.payload;
				std::vector<std::string> current;
				if (!row(db,
					 "SELECT seller_pid,listing_operation_id FROM auctions WHERE id=" +
						 std::to_string(payload.auction_id) +
						 " LOCK IN SHARE MODE",
					 2, &current))
					return errno ? errno : EILSEQ;
				uint64_t seller = 0;
				if (!u64(current[0], &seller) || !seller || seller > UINT32_MAX ||
				    current[1].size() != 16)
					return EILSEQ;
				listing.auction_id = payload.auction_id;
				listing.seller_pid = seller;
				std::copy(bytes(current[1]).begin(), bytes(current[1]).end(),
					  listing.listing_operation.bytes.begin());
			}
			else
			{
				if (auction_bid_accounting_decode(command, &intent, &payload, &bid,
								  &bid_accounts) ==
				    economic_accounting_error::ok)
				{
					listing.auction_id = bid.auction_id;
					listing.seller_pid = bid.seller_pid;
					listing.listing_operation = bid.listing_operation;
				}
				else if (auction_settlement_accounting_decode(
						 command, &intent, &payload, &listing,
						 &settlement_accounts) !=
					 economic_accounting_error::ok)
					return EILSEQ;
			}
			const auto &lineage = preparation_lineage ?
						      *preparation_lineage :
						      intent.admission.metadata.lineage;
			const auto &epoch = preparation_epoch ? *preparation_epoch :
								intent.admission.metadata.epoch;
			if ((payload.action != auction_action::bid &&
			     payload.action != auction_action::finalize &&
			     payload.action != auction_action::remove) ||
			    !listing.auction_id || !listing.seller_pid ||
			    critical_operation_id_is_zero(listing.listing_operation))
				return EILSEQ;
			if (!exact_count(db, "economic_epoch",
					 "lineage=" + hex(lineage.bytes) +
						 " AND epoch=" + hex(epoch.bytes),
					 1))
				return errno ? errno : EILSEQ;
			std::vector<std::string> cells;
			if (!row(db,
				 "SELECT schema_version,payload_version,result_payload,durable_revision FROM critical_operation_inbox WHERE operation_id=" +
					 hex(listing.listing_operation.bytes) +
					 " AND command_type=" +
					 std::to_string(static_cast<unsigned>(
						 critical_command_type::auction)) +
					 " AND status=1 AND result_code=0 AND failure_stage=0 AND committed_at IS NOT NULL LOCK IN SHARE MODE",
				 4, &cells))
				return errno ? errno : EILSEQ;
			uint64_t schema = 0, version = 0, durable = 0;
			auction_command_result receipt{};
			std::array<uint8_t, AUCTION_RESULT_PAYLOAD_BYTES> encoded{};
			if (!u64(cells[0], &schema) || !u64(cells[1], &version) ||
			    !u64(cells[3], &durable) ||
			    !auction_command_decode_result(bytes(cells[2]).data(), cells[2].size(),
							   &receipt) ||
			    !auction_command_encode_result(receipt, &encoded) ||
			    cells[2].size() != encoded.size() ||
			    !std::equal(encoded.begin(), encoded.end(), bytes(cells[2]).begin()) ||
			    durable !=
				    std::max({ receipt.auction_revision, receipt.wallet_revision,
					       receipt.bank_revision, receipt.player_owner_revision,
					       receipt.auction_owner_revision }) ||
			    receipt.action != auction_action::list ||
			    receipt.event_type != auction_event_type::listed ||
			    receipt.auction_id != listing.auction_id ||
			    receipt.seller_pid != listing.seller_pid || receipt.status != 1 ||
			    receipt.auction_revision != 1 || !receipt.item_count ||
			    receipt.item_count > AUCTION_COMMAND_MAX_ITEMS)
				return EILSEQ;
			auto &original = result.items;
			result.auction_id = listing.auction_id;
			if (version == AUCTION_NATIVE_COMMAND_PAYLOAD_VERSION)
			{
				if (schema != CRITICAL_COMMAND_ACCOUNTING_SCHEMA_VERSION)
					return EILSEQ;
				original_listing_forest_values forest;
				if (!original_listing_forest(db, &lineage, &epoch,
							     listing.auction_id, listing.seller_pid,
							     listing.listing_operation, &forest))
					return errno ? errno : EILSEQ;
				result.literals = forest.combined;
				size_t root = 0;
				for (size_t i = 0; i < forest.combined.size(); ++i)
				{
					if (root + 1 < forest.ranges.size() &&
					    i == forest.ranges[root + 1].first)
						++root;
					const auto &literal = forest.combined[i];
					const uint64_t parent =
						literal.parent_index < 0 ?
							0 :
							forest.combined[literal.parent_index]
								.object_uid;
					original.push_back(
						{ literal.object_uid,
						  { { item_owner_type::auction, listing.auction_id,
						      0 },
						    forest.combined[forest.ranges[root].first]
							    .object_uid,
						    parent,
						    forest.listing_revisions[i],
						    item_custody_state::active } });
				}
			}
			else if (version == AUCTION_COMMAND_PAYLOAD_VERSION &&
				 (schema == CRITICAL_COMMAND_SCHEMA_VERSION ||
				  schema == CRITICAL_COMMAND_ACCOUNTING_SCHEMA_VERSION))
			{
				// Legacy receipts carry roots only. They grant no ACT2/literal authority.
				// Preserve genuine singleton sources; descendants require an original ANF2 anchor.
				if (schema == CRITICAL_COMMAND_ACCOUNTING_SCHEMA_VERSION)
				{
					economic_frozen_intent recorded_intent;
					economic_accounting_plan recorded_plan;
					auction_command_result recorded_result{};
					if (!listing_root_core(db, listing.listing_operation,
							       &recorded_intent, &recorded_plan,
							       &recorded_result) ||
					    recorded_intent.admission.metadata.lineage.bytes !=
						    lineage.bytes ||
					    recorded_intent.admission.metadata.writer_id !=
						    ECONOMIC_WRITER_AUCTION_LISTING ||
					    recorded_intent.admission.metadata.reason !=
						    economic_reason::auction_listing ||
					    !native_retained_known_root(db, recorded_intent,
									recorded_result))
						return errno ? errno : EILSEQ;
					const auto claim_error =
						economic_sql_auction_source_claim_verify_known_metadata(
							db, recorded_intent.admission.metadata, 0);
					if (claim_error)
						return claim_error;
				}
				if (!origin(db, listing.listing_operation, listing.auction_id, 1,
					    listing.seller_pid, 1, 0))
					return errno ? errno : EILSEQ;
				const auto scope =
					"operation_id=" + hex(listing.listing_operation.bytes);
				if (!exact_count(db, "item_ownership_ledger", scope,
						 receipt.item_count))
					return errno ? errno : EILSEQ;
				std::unordered_set<uint64_t> unique;
				for (size_t i = 0; i < receipt.item_count; ++i)
				{
					const auto uid = receipt.item_uids[i],
						   revision = receipt.item_revisions[i];
					if (!uid || uid == UINT64_MAX || !revision ||
					    revision == UINT64_MAX || !unique.insert(uid).second)
						return EILSEQ;
					if (!exact_count(
						    db, "item_ownership_ledger",
						    scope + " AND event_index=" + std::to_string(i) +
							    " AND item_uid=" + std::to_string(uid) +
							    " AND root_item_uid=" +
							    std::to_string(uid) +
							    " AND parent_item_uid IS NULL AND from_owner_type=1 AND from_owner_id=" +
							    std::to_string(listing.seller_pid) +
							    " AND from_owner_context_id=0 AND to_owner_type=" +
							    std::to_string(static_cast<unsigned>(
								    item_owner_type::auction)) +
							    " AND to_owner_id=" +
							    std::to_string(listing.auction_id) +
							    " AND to_owner_context_id=0 AND item_revision=" +
							    std::to_string(revision) +
							    " AND from_owner_revision=" +
							    std::to_string(
								    receipt.player_owner_revision) +
							    " AND to_owner_revision=" +
							    std::to_string(
								    receipt.auction_owner_revision) +
							    " AND reason_type=" +
							    std::to_string(static_cast<unsigned>(
								    item_transfer_reason::
									    auction_list)) +
							    " AND reason_id=" +
							    std::to_string(listing.auction_id) +
							    " AND from_equipment_slot=0 AND to_equipment_slot=0",
						    1))
						return errno ? errno : EILSEQ;
					original.push_back({ uid,
							     { { item_owner_type::auction,
								 listing.auction_id, 0 },
							       uid,
							       0,
							       revision,
							       item_custody_state::active } });
				}
			}
			else
				return EILSEQ;
			return 0;
		};
		error = verify();
		const auto session_error = guard.check();
		if (session_error)
			return session_error;
		if (error)
			return error;
		*output = std::move(result);
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

unsigned int economic_sql_auction_read_native_listing_source(
	MYSQL *db, const critical_command &command,
	economic_sql_auction_native_listing_source *output) noexcept
{
	return auction_read_native_listing_source(db, command, nullptr, nullptr, output);
}

unsigned int economic_sql_auction_capture_native_listing_source(
	MYSQL *db, const critical_command &command, const critical_operation_id &lineage,
	const critical_operation_id &epoch,
	economic_sql_auction_native_listing_source *output) noexcept
{
	return auction_read_native_listing_source(db, command, &lineage, &epoch, output);
}

unsigned int
economic_sql_auction_verify_known_native_creator(MYSQL *db,
						 const critical_operation_id &operation) noexcept
{
#ifdef __NO_MYSQL__
	(void)db;
	(void)operation;
	return ENOTSUP;
#else
	try
	{
		economic_sql_auction_source_claim_detail::session guard{ db };
		auto error = guard.check(true);
		if (error)
			return error;
		auto verify = [&]() -> unsigned int
		{
			if (critical_operation_id_is_zero(operation))
				return EINVAL;
			if (!exact_count(
				    db, "critical_operation_inbox",
				    "operation_id=" + hex(operation.bytes) +
					    " AND schema_version=2 AND command_type=" +
					    std::to_string(static_cast<unsigned>(
						    critical_command_type::auction)) +
					    " AND payload_version=2 AND status=1 AND result_code=0 AND failure_stage=0 AND committed_at IS NOT NULL AND OCTET_LENGTH(command_hash)=32 AND OCTET_LENGTH(keys_hash)=32",
				    1))
				return errno ? errno : EILSEQ;
			economic_frozen_intent intent;
			economic_accounting_plan plan;
			auction_command_result receipt{};
			if (!known_terminal_creator(db, operation, &intent, &plan, &receipt))
				return errno ? errno : EILSEQ;
			const auto &meta = intent.admission.metadata;
			if (meta.operation_id.bytes != operation.bytes ||
			    critical_operation_id_is_zero(meta.original_operation_id) ||
			    !receipt.auction_id || !receipt.seller_pid)
				return EILSEQ;
			std::vector<std::string> source_version;
			uint64_t schema = 0, version = 0;
			if (!row(db,
				 "SELECT schema_version,payload_version FROM critical_operation_inbox WHERE operation_id=" +
					 hex(meta.original_operation_id.bytes) +
					 " AND status=1 AND result_code=0 AND failure_stage=0 AND committed_at IS NOT NULL LOCK IN SHARE MODE",
				 2, &source_version) ||
			    !u64(source_version[0], &schema) || !u64(source_version[1], &version))
				return errno ? errno : EILSEQ;
			// Preserve the existing historical v1 source route. It supplies no
			// invented ANF2/ACT2 literal authority.
			if (version == AUCTION_COMMAND_PAYLOAD_VERSION)
				return schema == CRITICAL_COMMAND_SCHEMA_VERSION ||
						       schema ==
							       CRITICAL_COMMAND_ACCOUNTING_SCHEMA_VERSION ?
					       0 :
					       EILSEQ;
			if (schema != CRITICAL_COMMAND_ACCOUNTING_SCHEMA_VERSION ||
			    version != AUCTION_NATIVE_COMMAND_PAYLOAD_VERSION)
				return EILSEQ;
			original_listing_forest_values source;
			if (!original_listing_forest(db, &meta.lineage, &meta.epoch,
						     receipt.auction_id, receipt.seller_pid,
						     meta.original_operation_id, &source))
				return errno ? errno : EILSEQ;
			return 0;
		};
		errno = 0;
		error = verify();
		const auto session_error = guard.check();
		return session_error ? session_error : error;
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
