#include "persistence/economic_sql_auction_retained.h"
#include "persistence/economic_sql_auction_source_claim.h"
#include "economy/auction_accounting.h"
#include "economy/auction_settlement_accounting.h"
#include "economy/auction_repository.h"
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
	if (payload.action != auction_action::bid)
		return exact_count(db, "currency_ledger", scope, 0);
	auction_bid_accounting_listing listing;
	auction_bid_accounting_accounts keys;
	economic_frozen_intent intent;
	if (auction_bid_accounting_decode(cmd, &intent, &payload, &listing, &keys) !=
	    economic_accounting_error::ok)
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
	auto p = scope + " AND pid=" + std::to_string(payload.actor_pid) +
		 " AND bank_id=" + std::to_string(bank) +
		 " AND wallet_revision=" + std::to_string(r.wallet_revision) +
		 " AND bank_revision=" + std::to_string(r.bank_revision) + " AND reason_type=" +
		 std::to_string(static_cast<unsigned>(currency_reason_type::auction_bid)) +
		 " AND reason_id=" + std::to_string(r.auction_id) +
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
	if (mysql_get_option(db, MYSQL_OPT_RECONNECT, &reconnect) || reconnect)
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
		auto typed = auction_bid_accounting_decode(cmd, &intent, &payload, &bid, &bidkeys);
		if (typed == economic_accounting_error::capacity)
			return ENOMEM;
		if (typed != economic_accounting_error::ok)
			typed = auction_settlement_accounting_decode(cmd, &intent, &payload,
								     &listing, &settlementkeys);
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
		else
			frozen_keys = { settlementkeys.escrow, settlementkeys.seller_claim,
					settlementkeys.actor_wallet, settlementkeys.actor_bank };
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
			" AND original_operation_id=" + hex(meta.original_operation_id.bytes) +
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
			if (!plan.children.empty() || !plan.item_events.empty())
				return errno ? errno : EILSEQ;
			if (!exact_count(db, "economic_accounting_child", scope, 0) ||
			    !exact_count(db, "economic_accounting_item_reference", scope, 0) ||
			    !exact_count(db, "item_ownership_ledger", scope, 0))
				return errno ? errno : EILSEQ;
			economic_accounting_plan regenerated;
			if (!regenerate(db, cmd, intent, plan, r, &regenerated) ||
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
