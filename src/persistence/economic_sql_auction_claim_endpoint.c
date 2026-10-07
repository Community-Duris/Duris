#include "persistence/economic_sql_pending_claim_source.h"
#include "persistence/economic_sql_auction_source_claim.h"
#include "economy/auction_accounting.h"
#include "economy/auction_settlement_accounting.h"
#include "economy/currency_command.h"
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

struct endpoint_request
{
	economic_frozen_intent intent;
	auction_command_payload payload{};
	uint32_t auction = 0, seller = 0;
	uint64_t revision = 0, amount = 0, price = 0;
	uint16_t slot = 0;
	bool unused = false, sold = false;
};
bool requested_endpoint(const critical_command &command, uint32_t pid, endpoint_request *request)
{
	endpoint_request candidate;
	auction_bid_accounting_listing bid;
	auction_bid_accounting_accounts keys;
	if (auction_bid_accounting_decode(command, &candidate.intent, &candidate.payload, &bid,
					  &keys) == economic_accounting_error::ok)
	{
		candidate.auction = bid.auction_id;
		candidate.seller = bid.seller_pid;
		candidate.revision = bid.revision;
		candidate.price = static_cast<uint64_t>(bid.buy_price && candidate.payload.value >=
										 bid.buy_price ?
								bid.buy_price :
								candidate.payload.value);
		candidate.sold = bid.buy_price > 0 &&
				 candidate.price >= static_cast<uint64_t>(bid.buy_price);
		if (keys.absent_bidder_pid == pid && pid)
		{
			candidate.unused = true;
		}
		else if (keys.absent_previous_pid == pid && pid)
		{
			candidate.slot = 1;
			candidate.amount = static_cast<uint64_t>(bid.current_price);
		}
		else if (keys.absent_seller_pid == pid && pid)
		{
			candidate.slot = 2;
			candidate.amount =
				candidate.price -
				static_cast<uint64_t>(static_cast<__int128_t>(candidate.price) *
						      candidate.payload.closing_fee_basis_points /
						      10000);
		}
		else
			return false;
	}
	else
	{
		auction_settlement_listing listing;
		auction_settlement_accounts accounts;
		if (auction_settlement_accounting_decode(command, &candidate.intent,
							 &candidate.payload, &listing, &accounts) !=
			    economic_accounting_error::ok ||
		    !pid || accounts.absent_seller_pid != pid)
			return false;
		candidate.auction = listing.auction_id;
		candidate.seller = listing.seller_pid;
		candidate.revision = listing.revision;
		candidate.price = static_cast<uint64_t>(listing.current_price);
		candidate.sold = true;
		candidate.slot = 2;
		candidate.amount =
			candidate.price -
			static_cast<uint64_t>(static_cast<__int128_t>(candidate.price) *
					      candidate.payload.closing_fee_basis_points / 10000);
	}
	*request = std::move(candidate);
	return true;
}
bool no_rows(MYSQL *connection, const std::string &sql)
{
	if (!execute(connection, sql))
		return false;
	std::unique_ptr<MYSQL_RES, decltype(&mysql_free_result)> result(
		mysql_store_result(connection), mysql_free_result);
	if (!result || mysql_num_fields(result.get()) != 1 || mysql_num_rows(result.get()))
	{
		errno = EILSEQ;
		return false;
	}
	return true;
}
bool no_retained_endpoint(MYSQL *connection, const critical_operation_id &lineage, uint32_t pid)
{
	const auto native = std::to_string(pid), scope = hex(lineage.bytes);
	return no_rows(connection,
		       "SELECT mapping_id FROM economic_account_mapping WHERE lineage=" + scope +
			       " AND account_kind=5 AND native_id=" + native + " FOR UPDATE") &&
	       no_rows(connection,
		       "SELECT source_slot FROM economic_pending_claim_source WHERE lineage=" +
			       scope + " AND beneficiary_pid=" + native + " FOR UPDATE");
}
bool admitted_inbox(MYSQL *connection, const critical_command &command,
		    const endpoint_request &request, bool committed)
{
	std::vector<uint8_t> bytes;
	std::array<uint8_t, 32> digest{};
	std::vector<std::string> values;
	if (critical_command_encode(command, &bytes) != critical_command_codec_result::ok)
	{
		errno = EILSEQ;
		return false;
	}
	SHA256(bytes.data(), bytes.size(), digest.data());
	std::vector<uint8_t> keys;
	for (const auto &key : command.keys)
	{
		keys.push_back(static_cast<uint8_t>(key.type));
		for (unsigned shift = 0; shift < 8; ++shift)
			keys.push_back(static_cast<uint8_t>(key.id >> (shift * 8)));
	}
	std::array<uint8_t, 32> keys_digest{};
	SHA256(keys.data(), keys.size(), keys_digest.data());
	if (!row(connection,
		 "SELECT COUNT(*) FROM critical_operation_inbox WHERE operation_id=" +
			 hex(command.operation_id.bytes) + " AND command_hash=" + hex(digest) +
			 " AND keys_hash=" + hex(keys_digest) + " AND payload_version=" +
			 std::to_string(command.payload_version) + " AND command_type=" +
			 std::to_string(static_cast<uint16_t>(command.type)) +
			 " AND schema_version=2 AND status=" +
			 (committed ?
				  "1 AND result_code=0 AND failure_stage=0 AND committed_at IS NOT NULL" :
				  "0"),
		 1, &values))
		return false;
	uint64_t count = 0;
	if (!u64(values[0], &count) || count != 1)
	{
		errno = EILSEQ;
		return false;
	}
	// Fresh pending admission requires today's active epoch. Committed replay
	// must prove the exact retained original epoch; all original inbox, canonical
	// root, receipt, ledger and creating-mapping checks remain mandatory.
	const std::string epoch_proof =
		committed ?
			"economic_epoch WHERE lineage=" +
				hex(request.intent.admission.metadata.lineage.bytes) +
				" AND epoch=" + hex(request.intent.admission.metadata.epoch.bytes) :
			"economic_lineage_state WHERE lineage=" +
				hex(request.intent.admission.metadata.lineage.bytes) +
				" AND active_epoch=" +
				hex(request.intent.admission.metadata.epoch.bytes);
	if (!row(connection, "SELECT COUNT(*) FROM " + epoch_proof + " LOCK IN SHARE MODE", 1,
		 &values))
		return false;
	if (!u64(values[0], &count) || count != 1)
	{
		errno = ESTALE;
		return false;
	}
	return true;
}
bool creator_native(MYSQL *connection, const critical_command &command,
		    const endpoint_request &request)
{
	std::vector<std::string> values;
	if (!row(connection,
		 "SELECT COUNT(*) FROM auction_ledger WHERE operation_id=" +
			 hex(command.operation_id.bytes) +
			 " AND auction_id=" + std::to_string(request.auction) +
			 " AND auction_revision=" + std::to_string(request.revision + 1) +
			 " AND event_type=" + (request.sold ? "3" : "2") +
			 " AND actor_pid=" + std::to_string(request.payload.actor_pid) +
			 " AND counterparty_pid=" + std::to_string(request.seller) +
			 " AND final_price=" + std::to_string(request.price),
		 1, &values))
		return false;
	uint64_t count = 0;
	if (!u64(values[0], &count) || count != 1)
	{
		errno = EILSEQ;
		return false;
	}
	return true;
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

// Frozen wallet/bank identities distinguish actor-led settlement from a timer.
// No missing command header or current actor is reconstructed.
bool original_native_actor(const economic_frozen_intent &intent, uint32_t *actor)
{
	const auto &meta = intent.admission.metadata;
	const auto &facts = intent.admission.facts;
	if (!actor || !meta.actor_id || meta.actor_id > UINT32_MAX)
	{
		errno = EILSEQ;
		return false;
	}
	if (meta.writer_id == ECONOMIC_WRITER_AUCTION_BID)
	{
		*actor = static_cast<uint32_t>(meta.actor_id);
		return true;
	}
	if (meta.writer_id != ECONOMIC_WRITER_AUCTION_SETTLEMENT || facts.size() < 122)
	{
		errno = EILSEQ;
		return false;
	}
	const auto number = [&](size_t offset, size_t width)
	{
		uint64_t value = 0;
		for (size_t i = 0; i < width; ++i)
			value |= static_cast<uint64_t>(facts[offset + i]) << (8 * i);
		return value;
	};
	const bool wallet = number(16, 8) != 0, bank = number(24, 8) != 0;
	if (wallet != bank || (!wallet && meta.actor_id != number(32, 4)))
	{
		errno = EILSEQ;
		return false;
	}
	*actor = wallet ? static_cast<uint32_t>(meta.actor_id) : 0;
	return true;
}
bool original_sale_result(MYSQL *connection, const critical_operation_id &operation,
			  const economic_frozen_intent &intent,
			  const economic_accounting_plan &plan, const std::string &bytes,
			  const std::string &durable_text, const critical_command *original)
{
	auction_command_result result{};
	std::array<uint8_t, AUCTION_RESULT_PAYLOAD_BYTES> canonical{};
	uint64_t durable = 0;
	if (!auction_command_decode_result(reinterpret_cast<const uint8_t *>(bytes.data()),
					   bytes.size(), &result) ||
	    !auction_command_encode_result(result, &canonical) ||
	    bytes.size() != canonical.size() ||
	    !std::equal(canonical.begin(), canonical.end(),
			reinterpret_cast<const uint8_t *>(bytes.data())) ||
	    !u64(durable_text, &durable) ||
	    durable != std::max({ result.auction_revision, result.wallet_revision,
				  result.bank_revision, result.player_owner_revision,
				  result.auction_owner_revision }))
	{
		errno = EILSEQ;
		return false;
	}
	// Native auction publisher contract in critical_command_repository: 5/1/1.
	// Do not compare delivery status: legitimate already-delivered rows remain proof.
	std::vector<std::string> outbox;
	if (!row(connection,
		 "SELECT event_index,destination,event_type,payload_version,payload FROM critical_outbox WHERE operation_id=" +
			 hex(operation.bytes) + " LOCK IN SHARE MODE",
		 5, &outbox))
		return false;
	if (outbox[0] != "0" || outbox[1] != "5" || outbox[2] != "1" || outbox[3] != "1" ||
	    outbox[4] != bytes)
	{
		errno = EILSEQ;
		return false;
	}
	const auto &meta = intent.admission.metadata;
	const auto &facts = intent.admission.facts;
	const auto number = [&](size_t offset, size_t width)
	{
		uint64_t value = 0;
		for (size_t i = 0; i < width; ++i)
			value |= static_cast<uint64_t>(facts[offset + i]) << (i * 8);
		return value;
	};
	uint64_t escrow = 0, revision = 0;
	uint32_t auction = 0, seller = 0, winner = 0, previous = 0;
	const bool bid = meta.writer_id == ECONOMIC_WRITER_AUCTION_BID &&
			 meta.reason == economic_reason::auction_bid;
	if (bid && (facts.size() == 124 || facts.size() == 140))
	{
		escrow = number(16, 8);
		auction = static_cast<uint32_t>(number(48, 4));
		seller = static_cast<uint32_t>(number(52, 4));
		previous = static_cast<uint32_t>(number(56, 4));
		winner = static_cast<uint32_t>(meta.actor_id);
		revision = number(84, 8);
		const bool sold = result.event_type == auction_event_type::sold;
		if (result.action != auction_action::bid ||
		    (!sold && result.event_type != auction_event_type::bid_placed) ||
		    result.status != (sold ? 2U : 1U) || result.final_price <= 0 ||
		    result.final_price > UINT_MAX ||
		    (sold && static_cast<uint64_t>(result.final_price) != number(76, 8)))
		{
			errno = EILSEQ;
			return false;
		}
		const auto previous_price = number(68, 8);
		if (previous_price > UINT_MAX || result.wallet_value_delta == INT64_MIN)
		{
			errno = EILSEQ;
			return false;
		}
		const int64_t to_pay =
			result.final_price -
			(previous == winner ? static_cast<int64_t>(previous_price) : 0);
		if (to_pay < 0 || result.wallet_value_delta > 0 ||
		    -result.wallet_value_delta > to_pay ||
		    result.claim_credit_used != to_pay + result.wallet_value_delta)
		{
			errno = EILSEQ;
			return false;
		}
		if (result.claim_credit_used)
		{
			const auto claim = std::find_if(
				plan.accounts.begin(), plan.accounts.end(),
				[&](const auto &e)
				{
					return e.key.kind == economic_account_kind::pending_claim &&
					       e.key.authority_id == number(24, 8) &&
					       e.key.lineage.bytes == meta.lineage.bytes &&
					       !e.key.context_id;
				});
			if (claim == plan.accounts.end() || claim->before_revision == UINT64_MAX ||
			    claim->after_revision != claim->before_revision + 1 ||
			    claim->before[0] - claim->after[0] != result.claim_credit_used)
			{
				errno = EILSEQ;
				return false;
			}
		}
	}
	else if (!bid && meta.writer_id == ECONOMIC_WRITER_AUCTION_SETTLEMENT &&
		 meta.reason == economic_reason::auction_settle && facts.size() >= 122)
	{
		const auto items = number(120, 2);
		if (!items || items > AUCTION_COMMAND_MAX_ITEMS ||
		    (facts.size() != 122 + items * 27 && facts.size() != 130 + items * 27))
		{
			errno = EILSEQ;
			return false;
		}
		escrow = number(0, 8);
		auction = static_cast<uint32_t>(number(32, 4));
		seller = static_cast<uint32_t>(number(36, 4));
		winner = static_cast<uint32_t>(number(40, 4));
		revision = number(72, 8);
		if (!winner || result.action != auction_action::finalize ||
		    result.event_type != auction_event_type::sold || result.status != 2 ||
		    result.final_price != static_cast<int64_t>(number(56, 8)) ||
		    result.wallet_value_delta || result.claim_credit_used)
		{
			errno = EILSEQ;
			return false;
		}
		// Settlement keeps item custody unchanged. Compare both original EAP
		// witnesses against the available frozen UID/revision/slot facts.
		if (plan.items_before.size() != items || plan.items_after.size() != items ||
		    number(44, 4) != 1 || number(48, 4) != 1 || number(52, 4) != items ||
		    !number(80, 8))
		{
			errno = EILSEQ;
			return false;
		}
		for (size_t index = 0; index < items; ++index)
		{
			const size_t offset = 122 + 27 * index;
			const auto uid = number(offset, 8), item_revision = number(offset + 8, 8);
			if (!uid || !item_revision || item_revision == UINT64_MAX ||
			    number(offset + 16, 2) != index || number(offset + 18, 4) > INT32_MAX ||
			    number(offset + 22, 4) || facts[offset + 26])
			{
				errno = EILSEQ;
				return false;
			}
			for (size_t prior_index = 0; prior_index < index; ++prior_index)
				if (number(122 + 27 * prior_index, 8) == uid)
				{
					errno = EILSEQ;
					return false;
				}
			const auto before =
				std::find_if(plan.items_before.begin(), plan.items_before.end(),
					     [&](const auto &item) { return item.uid == uid; });
			const auto after =
				std::find_if(plan.items_after.begin(), plan.items_after.end(),
					     [&](const auto &item) { return item.uid == uid; });
			if (before == plan.items_before.end() || after == plan.items_after.end() ||
			    before->position.owner.type != item_owner_type::auction ||
			    before->position.owner.id != auction ||
			    before->position.owner.context_id || before->position.root_uid != uid ||
			    before->position.parent_uid ||
			    before->position.revision != item_revision ||
			    before->position.state != item_custody_state::active ||
			    before->position.equipment_slot ||
			    !economic_item_position_equal(before->position, after->position))
			{
				errno = EILSEQ;
				return false;
			}
		}
		// A domain-only timed settlement has no actor balance projection.
		if (!number(16, 8) && !number(24, 8) &&
		    (result.wallet.amount != economic_coin_vector{} ||
		     result.bank.amount != economic_coin_vector{} || result.wallet_revision ||
		     result.bank_revision))
		{
			errno = EILSEQ;
			return false;
		}
	}
	else
	{
		errno = EILSEQ;
		return false;
	}
	if (!auction || !seller || !revision || revision == UINT64_MAX ||
	    result.auction_id != auction || result.seller_pid != seller ||
	    result.winner_pid != winner || result.previous_bidder_pid != previous ||
	    result.auction_revision != revision + 1 || result.item_count ||
	    result.player_owner_revision || result.auction_owner_revision ||
	    std::any_of(result.item_uids.begin(), result.item_uids.end(),
			[](uint64_t v) { return v != 0; }) ||
	    std::any_of(result.item_revisions.begin(), result.item_revisions.end(),
			[](uint64_t v) { return v != 0; }))
	{
		errno = EILSEQ;
		return false;
	}
	const auto effect =
		std::find_if(plan.accounts.begin(), plan.accounts.end(),
			     [&](const auto &e)
			     {
				     return e.key.kind == economic_account_kind::auction_escrow &&
					    e.key.authority_id == escrow &&
					    e.key.lineage.bytes == meta.lineage.bytes &&
					    !e.key.context_id;
			     });
	if (effect == plan.accounts.end() || effect->before_revision != revision ||
	    effect->after_revision != result.auction_revision)
	{
		errno = EILSEQ;
		return false;
	}
	uint32_t native_actor = 0;
	if (!original_native_actor(intent, &native_actor))
		return false;
	const auto actor = "actor_pid=" + std::to_string(native_actor);
	if (!exact_count(
		    connection, "auction_ledger",
		    "operation_id=" + hex(operation.bytes) + " AND event_type=" +
			    std::to_string(static_cast<uint8_t>(result.event_type)) +
			    " AND auction_id=" + std::to_string(auction) +
			    " AND auction_revision=" + std::to_string(result.auction_revision) +
			    " AND " + actor + " AND counterparty_pid=" + std::to_string(seller) +
			    " AND value_delta=" + std::to_string(result.wallet_value_delta) +
			    " AND final_price=" + std::to_string(result.final_price) +
			    " AND item_count=0",
		    1))
		return false;
	if (bid)
	{
		const auto wallet =
			std::find_if(plan.accounts.begin(), plan.accounts.end(),
				     [&](const auto &e)
				     {
					     return e.key.kind == economic_account_kind::wallet &&
						    e.key.authority_id == number(0, 8) &&
						    e.key.lineage.bytes == meta.lineage.bytes &&
						    !e.key.context_id;
				     });
		if (wallet == plan.accounts.end() || wallet->after != result.wallet.amount ||
		    wallet->after_revision != result.wallet_revision ||
		    result.wallet_value_delta > 0)
		{
			errno = EILSEQ;
			return false;
		}
		// The captured bank lifetime is a historical identity, not today's active
		// account. Bind its real native ID even after retirement or epoch cutover.
		std::vector<std::string> bank_cells;
		uint64_t native_bank = 0;
		std::string bank_context;
		if (original)
		{
			auction_command_payload original_payload{};
			if (!auction_command_decode_payload(*original, &original_payload))
			{
				errno = EILSEQ;
				return false;
			}
			bank_context =
				" AND context_id=" + std::to_string(original_payload.racewar);
		}
		if (!row(connection,
			 "SELECT native_id FROM economic_account_mapping WHERE mapping_id=" +
				 std::to_string(number(8, 8)) +
				 " AND lineage=" + hex(meta.lineage.bytes) +
				 " AND account_kind=2 AND backend_kind=1 AND locator_kind=2" +
				 bank_context + " LOCK IN SHARE MODE",
			 1, &bank_cells) ||
		    !u64(bank_cells[0], &native_bank) || !native_bank || native_bank > UINT32_MAX)
		{
			errno = EILSEQ;
			return false;
		}
		auto predicate =
			"operation_id=" + hex(operation.bytes) +
			" AND pid=" + std::to_string(meta.actor_id) +
			" AND wallet_revision=" + std::to_string(result.wallet_revision) +
			" AND bank_revision=" + std::to_string(result.bank_revision) +
			" AND bank_id=" + std::to_string(native_bank) + " AND reason_type=" +
			std::to_string(static_cast<uint16_t>(currency_reason_type::auction_bid)) +
			" AND reason_id=" + std::to_string(auction);
		if (original)
			predicate += " AND source_site=" +
				     std::to_string(static_cast<uint16_t>(original->source_site));
		// Zero-creator audit has no original command bytes. Never reconstruct its
		// source-site header from the stored digest or a current producer choice.
		static constexpr std::array<const char *, 4> denominations = { "copper", "silver",
									       "gold", "platinum" };
		economic_coin_vector delta{};
		if (economic_coin_delta(wallet->before, wallet->after, &delta) !=
		    economic_accounting_error::ok)
		{
			errno = EILSEQ;
			return false;
		}
		int64_t value_delta = 0;
		if (economic_coin_value(delta, &value_delta) != economic_accounting_error::ok ||
		    value_delta != result.wallet_value_delta)
		{
			errno = EILSEQ;
			return false;
		}
		for (size_t i = 0; i < denominations.size(); ++i)
			predicate += " AND wallet_after_" + std::string(denominations[i]) + "=" +
				     std::to_string(result.wallet.amount[i]) + " AND bank_after_" +
				     denominations[i] + "=" +
				     std::to_string(result.bank.amount[i]) + " AND wallet_delta_" +
				     denominations[i] + "=" + std::to_string(delta[i]) +
				     " AND bank_delta_" + denominations[i] + "=0";
		if (!exact_count(connection, "currency_ledger", predicate, 1))
			return false;
	}
	return true;
}

bool endpoint_root(MYSQL *connection, const critical_operation_id &operation,
		   economic_frozen_intent *intent, economic_accounting_plan *plan,
		   const critical_command *original = nullptr)
{
	std::vector<std::string> cells;
	if (!row(connection,
		 "SELECT o.canonical_intent,o.canonical_plan,o.plan_digest,i.result_payload,i.durable_revision FROM economic_accounting_operation o JOIN critical_operation_inbox i ON i.operation_id=o.operation_id WHERE o.operation_id=" +
			 hex(operation.bytes) +
			 " AND o.outcome=1 AND o.result_code=0 AND i.status=1 AND i.result_code=0 AND i.failure_stage=0 AND i.committed_at IS NOT NULL AND i.command_type=" +
			 std::to_string(static_cast<uint16_t>(critical_command_type::auction)) +
			 " AND i.schema_version=2 AND i.payload_version=1 LOCK IN SHARE MODE",
		 5, &cells))
		return false;
	const auto span = [](const std::string &bytes)
	{ return std::span(reinterpret_cast<const uint8_t *>(bytes.data()), bytes.size()); };
	economic_digest digest{};
	economic_plan_metadata expected;
	if (economic_intent_decode(span(cells[0]), intent) != economic_accounting_error::ok ||
	    economic_plan_decode(span(cells[1]), plan) != economic_accounting_error::ok ||
	    economic_plan_digest(*plan, &digest) != economic_accounting_error::ok ||
	    cells[2].size() != digest.size() ||
	    !std::equal(digest.begin(), digest.end(),
			reinterpret_cast<const uint8_t *>(cells[2].data())))
	{
		errno = EILSEQ;
		return false;
	}
	static_cast<economic_operation_metadata &>(expected) = intent->admission.metadata;
	expected.domain_digest = intent->domain_digest;
	if (expected.operation_id.bytes != operation.bytes ||
	    economic_intent_digest(*intent, &expected.intent_digest) !=
		    economic_accounting_error::ok)
	{
		errno = EILSEQ;
		return false;
	}
	auto canonical = *plan;
	canonical.metadata = expected;
	std::vector<uint8_t> bytes;
	if (economic_plan_encode(canonical, &bytes) != economic_accounting_error::ok ||
	    bytes.size() != cells[1].size() ||
	    !std::equal(bytes.begin(), bytes.end(),
			reinterpret_cast<const uint8_t *>(cells[1].data())))
	{
		errno = EILSEQ;
		return false;
	}
	std::array<uint8_t, ECONOMIC_SOURCE_EVENT_BYTES> source{};
	if (!expected.source_event ||
	    economic_source_event_encode(*expected.source_event, &source) !=
		    economic_accounting_error::ok)
	{
		errno = EILSEQ;
		return false;
	}
	std::vector<std::string> count_cells;
	if (!row(connection,
		 "SELECT COUNT(*) FROM economic_accounting_operation WHERE operation_id=" +
			 hex(operation.bytes) + " AND lineage=" + hex(expected.lineage.bytes) +
			 " AND epoch=" + hex(expected.epoch.bytes) +
			 " AND writer_id=" + std::to_string(expected.writer_id) +
			 " AND reason=" + std::to_string(static_cast<uint16_t>(expected.reason)) +
			 " AND actor_kind=" +
			 std::to_string(static_cast<uint8_t>(expected.actor_kind)) +
			 " AND actor_id=" + std::to_string(expected.actor_id) +
			 (critical_operation_id_is_zero(expected.original_operation_id) ?
				  " AND original_operation_id IS NULL" :
				  " AND original_operation_id=" +
					  hex(expected.original_operation_id.bytes)) +
			 " AND intent_digest=" + hex(expected.intent_digest) +
			 " AND domain_digest=" + hex(expected.domain_digest) +
			 " AND accounting_version=" + std::to_string(expected.version) +
			 " AND policy_version=" + std::to_string(expected.policy_version) +
			 " AND compiler_version=" + std::to_string(expected.compiler_version) +
			 " AND source_event=" + hex(source) +
			 " AND account_count=" + std::to_string(plan->accounts.size()) +
			 " AND posting_count=" + std::to_string(plan->postings.size()) +
			 " AND child_count=" + std::to_string(plan->children.size()) +
			 " AND item_event_count=" + std::to_string(plan->item_events.size()) +
			 " AND before_witness_count=" + std::to_string(plan->items_before.size()) +
			 " AND after_witness_count=" + std::to_string(plan->items_after.size()),
		 1, &count_cells))
		return false;
	uint64_t count = 0;
	if (!u64(count_cells[0], &count) || count != 1)
	{
		errno = EILSEQ;
		return false;
	}
	const auto source_claim_error =
		economic_sql_auction_source_claim_verify(connection, *intent, 0);
	if (source_claim_error)
	{
		errno = static_cast<int>(source_claim_error);
		return false;
	}
	return normalized_financial_rows(connection, operation, *plan) &&
	       original_sale_result(connection, operation, *intent, *plan, cells[3], cells[4],
				    original);
}
bool original_endpoint_effect(MYSQL *connection, const critical_operation_id &operation,
			      const economic_account_key &key, uint32_t pid, uint64_t amount,
			      const economic_accounting_plan &plan, bool historical = false)
{
	std::array<uint8_t, ECONOMIC_ACCOUNT_KEY_BYTES> encoded{};
	if (economic_account_key_encode(key, &encoded) != economic_accounting_error::ok)
	{
		errno = EILSEQ;
		return false;
	}
	const auto effect = std::find_if(plan.accounts.begin(), plan.accounts.end(),
					 [&](const auto &value)
					 { return economic_account_key_equal(value.key, key); });
	if (effect == plan.accounts.end() || effect->before != economic_coin_vector{} ||
	    effect->after != economic_coin_vector{ static_cast<int64_t>(amount), 0, 0, 0 } ||
	    effect->before_revision || effect->after_revision != 1)
	{
		errno = EILSEQ;
		return false;
	}
	std::vector<std::string> cells;
	if (!row(connection,
		 "SELECT COUNT(*) FROM economic_account_mapping WHERE mapping_id=" +
			 std::to_string(key.authority_id) +
			 " AND lineage=" + hex(key.lineage.bytes) +
			 " AND account_kind=5 AND context_id=0 AND backend_kind=1 AND locator_kind=5 AND native_id=" +
			 std::to_string(pid) +
			 (historical ?
				  "" :
				  " AND active_native_id=" + std::to_string(pid) +
					  " AND retiring_operation_id IS NULL AND revision=0") +
			 " AND creating_operation_id=" + hex(operation.bytes),
		 1, &cells))
		return false;
	uint64_t count = 0;
	if (!u64(cells[0], &count) || count != 1)
	{
		errno = EILSEQ;
		return false;
	}
	if (!row(connection,
		 "SELECT COUNT(*) FROM economic_accounting_account_effect WHERE operation_id=" +
			 hex(operation.bytes) + " AND account_index=" +
			 std::to_string(static_cast<size_t>(effect - plan.accounts.begin())) +
			 " AND account_key=" + hex(encoded) +
			 " AND before_copper=0 AND before_silver=0 AND before_gold=0 AND before_platinum=0 AND after_copper=" +
			 std::to_string(amount) +
			 " AND after_silver=0 AND after_gold=0 AND after_platinum=0 AND before_revision=0 AND after_revision=1",
		 1, &cells))
		return false;
	if (!u64(cells[0], &count) || count != 1)
	{
		errno = EILSEQ;
		return false;
	}
	return true;
}
} // namespace
#endif

unsigned int economic_sql_pending_claim_endpoint_lock_absent(MYSQL *connection,
							     const critical_command &command,
							     uint32_t beneficiary_pid)
{
#ifdef __NO_MYSQL__
	(void)connection;
	(void)command;
	(void)beneficiary_pid;
	return ENOTSUP;
#else
	if (!connection || !(connection->server_status & SERVER_STATUS_IN_TRANS))
		return EINVAL;
	try
	{
		endpoint_request request;
		if (!requested_endpoint(command, beneficiary_pid, &request))
			return EPROTONOSUPPORT;
		if (!admitted_inbox(connection, command, request, false) ||
		    !no_rows(connection, "SELECT pid FROM auction_money_pickups WHERE pid=" +
						 std::to_string(beneficiary_pid) + " FOR UPDATE") ||
		    !no_retained_endpoint(connection, request.intent.admission.metadata.lineage,
					  beneficiary_pid))
			return errno ? static_cast<unsigned int>(errno) : EILSEQ;
		return 0;
	}
	catch (const std::bad_alloc &)
	{
		return ENOMEM;
	}
#endif
}
unsigned int economic_sql_pending_claim_endpoint_create(MYSQL *connection,
							const critical_command &command,
							uint32_t beneficiary_pid,
							economic_account_key *output)
{
#ifdef __NO_MYSQL__
	(void)connection;
	(void)command;
	(void)beneficiary_pid;
	(void)output;
	return ENOTSUP;
#else
	if (!connection || !output || !(connection->server_status & SERVER_STATUS_IN_TRANS))
		return EINVAL;
	try
	{
		endpoint_request request;
		if (!requested_endpoint(command, beneficiary_pid, &request) || request.unused)
			return EPROTONOSUPPORT;
		std::vector<std::string> values;
		if (!admitted_inbox(connection, command, request, false) ||
		    !creator_native(connection, command, request) ||
		    !no_retained_endpoint(connection, request.intent.admission.metadata.lineage,
					  beneficiary_pid) ||
		    !row(connection,
			 "SELECT money,claim_revision FROM auction_money_pickups WHERE pid=" +
				 std::to_string(beneficiary_pid) + " FOR UPDATE",
			 2, &values))
			return errno ? static_cast<unsigned int>(errno) : EILSEQ;
		uint64_t amount = 0, revision = 0;
		if (!u64(values[0], &amount) || amount != request.amount ||
		    !u64(values[1], &revision) || revision != 1)
			return EILSEQ;
		const auto &meta = request.intent.admission.metadata;
		if (!execute(
			    connection,
			    "INSERT INTO economic_account_mapping(lineage,account_kind,context_id,backend_kind,locator_kind,native_id,active_native_id,creating_operation_id) VALUES(" +
				    hex(meta.lineage.bytes) + ",5,0,1,5," +
				    std::to_string(beneficiary_pid) + "," +
				    std::to_string(beneficiary_pid) + "," +
				    hex(command.operation_id.bytes) + ")"))
			return errno ? static_cast<unsigned int>(errno) : EIO;
		const uint64_t mapping = mysql_insert_id(connection);
		if (!mapping)
			return EILSEQ;
		economic_account_key key{ meta.lineage, economic_account_kind::pending_claim,
					  mapping, 0 };
		economic_sql_authority_snapshot verified;
		const economic_sql_mapping_request mapping_request{ key, 5, beneficiary_pid };
		const auto error = economic_sql_lock_authority(connection, meta.lineage, meta.epoch,
							       std::span(&mapping_request, 1),
							       &verified);
		if (error)
			return error;
		*output = key;
		return 0;
	}
	catch (const std::bad_alloc &)
	{
		return ENOMEM;
	}
#endif
}

unsigned int economic_sql_pending_claim_endpoint_readback(MYSQL *connection,
							  const critical_command &command,
							  uint32_t beneficiary_pid,
							  economic_account_key *output)
{
#ifdef __NO_MYSQL__
	(void)connection;
	(void)command;
	(void)beneficiary_pid;
	(void)output;
	return ENOTSUP;
#else
	if (!connection || !output || !(connection->server_status & SERVER_STATUS_IN_TRANS))
		return EINVAL;
	try
	{
		endpoint_request request;
		if (!requested_endpoint(command, beneficiary_pid, &request))
			return EPROTONOSUPPORT;
		economic_frozen_intent retained;
		economic_accounting_plan plan;
		if (!admitted_inbox(connection, command, request, true) ||
		    !creator_native(connection, command, request) ||
		    !endpoint_root(connection, command.operation_id, &retained, &plan, &command))
			return errno ? static_cast<unsigned int>(errno) : EILSEQ;
		std::vector<uint8_t> encoded;
		if (economic_intent_encode(retained, &encoded) != economic_accounting_error::ok ||
		    encoded != command.accounting_intent)
			return EILSEQ;
		if (request.unused)
			return ENODATA;
		std::vector<std::string> cells;
		const auto &meta = request.intent.admission.metadata;
		if (!row(connection,
			 "SELECT mapping_id FROM economic_account_mapping WHERE lineage=" +
				 hex(meta.lineage.bytes) + " AND account_kind=5 AND native_id=" +
				 std::to_string(beneficiary_pid) + " AND creating_operation_id=" +
				 hex(command.operation_id.bytes) + " LOCK IN SHARE MODE",
			 1, &cells))
			return errno ? static_cast<unsigned int>(errno) : EILSEQ;
		uint64_t mapping = 0;
		if (!u64(cells[0], &mapping) || !mapping)
			return EILSEQ;
		economic_account_key key{ meta.lineage, economic_account_kind::pending_claim,
					  mapping, 0 };
		if (!original_endpoint_effect(connection, command.operation_id, key,
					      beneficiary_pid, request.amount, plan))
			return errno ? static_cast<unsigned int>(errno) : EILSEQ;
		const auto source = "source_operation_id=" + hex(command.operation_id.bytes) +
				    " AND beneficiary_pid=" + std::to_string(beneficiary_pid);
		if (!row(connection,
			 "SELECT COUNT(*) FROM economic_pending_claim_source WHERE " + source, 1,
			 &cells))
			return errno ? static_cast<unsigned int>(errno) : EILSEQ;
		uint64_t count = 0;
		if (!u64(cells[0], &count) || count != (request.amount ? 1U : 0U))
			return EILSEQ;
		if (request.amount)
		{
			if (!row(connection,
				 "SELECT COUNT(*) FROM economic_pending_claim_source WHERE " +
					 source +
					 " AND source_slot=" + std::to_string(request.slot) +
					 " AND lineage=" + hex(meta.lineage.bytes) +
					 " AND claim_mapping_id=" + std::to_string(mapping) +
					 " AND amount=" + std::to_string(request.amount),
				 1, &cells))
				return errno ? static_cast<unsigned int>(errno) : EILSEQ;
			if (!u64(cells[0], &count) || count != 1)
				return EILSEQ;
		}
		std::vector<economic_sql_pending_claim_remaining> remaining;
		const auto error = economic_sql_pending_claim_source_remaining(
			connection, key, beneficiary_pid, &remaining);
		if (error)
			return error;
		uint64_t sum = 0;
		for (const auto &entry : remaining)
		{
			if (entry.remaining_amount > UINT_MAX - sum)
				return EILSEQ;
			sum += entry.remaining_amount;
		}
		if (!row(connection,
			 "SELECT money,claim_revision FROM auction_money_pickups WHERE pid=" +
				 std::to_string(beneficiary_pid) + " FOR UPDATE",
			 2, &cells))
			return errno ? static_cast<unsigned int>(errno) : EILSEQ;
		uint64_t native = 0, revision = 0;
		if (!u64(cells[0], &native) || native != sum || !u64(cells[1], &revision) ||
		    !revision)
			return EILSEQ;
		*output = key;
		return 0;
	}
	catch (const std::bad_alloc &)
	{
		return ENOMEM;
	}
#endif
}

static unsigned int verify_zero_creator(MYSQL *connection, const critical_operation_id &creator,
					const economic_account_key &key, uint32_t beneficiary_pid,
					bool historical)
{
#ifdef __NO_MYSQL__
	(void)connection;
	(void)creator;
	(void)key;
	(void)beneficiary_pid;
	(void)historical;
	return ENOTSUP;
#else
	if (!connection || !(connection->server_status & SERVER_STATUS_IN_TRANS) ||
	    !economic_account_key_valid(key) || key.kind != economic_account_kind::pending_claim ||
	    key.context_id || !beneficiary_pid || critical_operation_id_is_zero(creator))
		return EINVAL;
	try
	{
		economic_frozen_intent intent;
		economic_accounting_plan plan;
		if (!endpoint_root(connection, creator, &intent, &plan))
			return errno ? static_cast<unsigned int>(errno) : EILSEQ;
		const auto &meta = intent.admission.metadata;
		const auto &facts = intent.admission.facts;
		const auto number = [&](size_t offset, size_t width)
		{
			uint64_t value = 0;
			for (size_t index = 0; index < width; ++index)
				value |= static_cast<uint64_t>(facts[offset + index])
					 << (index * 8);
			return value;
		};
		uint32_t auction = 0;
		uint64_t revision = 0, price = 0;
		size_t tag = 0, pid_offset = 0, listing_offset = 0, prior_offset = 0;
		bool winner = false;
		if (meta.writer_id == ECONOMIC_WRITER_AUCTION_BID &&
		    meta.reason == economic_reason::auction_bid && facts.size() == 140)
		{
			tag = 124;
			pid_offset = 136;
			if (number(40, 8))
				return EILSEQ;
			auction = static_cast<uint32_t>(number(48, 4));
			if (number(52, 4) != beneficiary_pid)
				return EILSEQ;
			revision = number(84, 8);
			price = number(76, 8);
			listing_offset = 92;
			prior_offset = 108;
			winner = number(56, 4) != 0;
			if (!price)
				return EILSEQ;
		}
		else if (meta.writer_id == ECONOMIC_WRITER_AUCTION_SETTLEMENT &&
			 meta.reason == economic_reason::auction_settle && facts.size() >= 130)
		{
			const auto items = number(120, 2);
			if (!items || items > AUCTION_COMMAND_MAX_ITEMS ||
			    facts.size() != 130 + items * 27 || number(8, 8))
				return EILSEQ;
			tag = 122 + items * 27;
			pid_offset = tag + 4;
			auction = static_cast<uint32_t>(number(32, 4));
			if (number(36, 4) != beneficiary_pid)
				return EILSEQ;
			revision = number(72, 8);
			price = number(56, 8);
			listing_offset = 88;
			prior_offset = 104;
			winner = number(40, 4) != 0;
			if (!winner)
				return EILSEQ;
		}
		else
			return EILSEQ;
		if (meta.actor_kind != economic_actor_kind::domain ||
		    !std::equal(facts.begin() + listing_offset, facts.begin() + listing_offset + 16,
				meta.original_operation_id.bytes.begin()) ||
		    !meta.source_event ||
		    !std::equal(facts.begin() + (winner ? prior_offset : listing_offset),
				facts.begin() + (winner ? prior_offset : listing_offset) + 16,
				meta.source_event->source.bytes.begin()) ||
		    !plan.children.empty() || !plan.item_events.empty())
			return EILSEQ;
		if (!std::equal(facts.begin() + tag, facts.begin() + tag + 4,
				std::array<uint8_t, 4>{ 'A', 'E', 'C', '1' }.begin()) ||
		    number(pid_offset, 4) != beneficiary_pid || !auction || !revision ||
		    revision == UINT64_MAX || meta.lineage.bytes != key.lineage.bytes ||
		    !meta.source_event ||
		    meta.source_event->kind != economic_source_kind::auction ||
		    meta.source_event->sequence != revision || meta.source_event->slot ||
		    meta.source_event->generation.bytes != meta.original_operation_id.bytes ||
		    !original_endpoint_effect(connection, creator, key, beneficiary_pid, 0, plan,
					      historical))
			return EILSEQ;
		uint32_t native_actor = 0;
		if (!original_native_actor(intent, &native_actor))
			return EILSEQ;
		std::vector<std::string> cells;
		if (!row(connection,
			 "SELECT final_price FROM auction_ledger WHERE operation_id=" +
				 hex(creator.bytes) + " AND auction_id=" + std::to_string(auction) +
				 " AND auction_revision=" + std::to_string(revision + 1) +
				 " AND event_type=3 AND counterparty_pid=" +
				 std::to_string(beneficiary_pid) + " AND " +
				 ("actor_pid=" + std::to_string(native_actor)) +
				 " LOCK IN SHARE MODE",
			 1, &cells))
			return errno ? static_cast<unsigned int>(errno) : EILSEQ;
		uint64_t native_price = 0;
		if (!u64(cells[0], &native_price) || !native_price || native_price > UINT_MAX ||
		    (price && price != native_price))
			return EILSEQ;
		const auto sink = std::find_if(
			plan.accounts.begin(), plan.accounts.end(),
			[&](const auto &effect)
			{
				return effect.key.kind == economic_account_kind::sink &&
				       effect.key.authority_id ==
					       ECONOMIC_AUCTION_CLOSING_FEE_SINK_ID &&
				       effect.key.lineage.bytes == key.lineage.bytes &&
				       !effect.key.context_id;
			});
		if (sink == plan.accounts.end())
			return EILSEQ;
		int64_t sink_total = 0;
		const auto sink_index = static_cast<uint16_t>(sink - plan.accounts.begin());
		for (const auto &posting : plan.postings)
			if (posting.account_index == sink_index)
			{
				if (posting.copper < 0 ||
				    posting.copper >
					    static_cast<int64_t>(native_price) - sink_total)
					return EILSEQ;
				sink_total += posting.copper;
			}
		if (sink_total != static_cast<int64_t>(native_price))
			return EILSEQ;
		if (!row(connection,
			 "SELECT COUNT(*) FROM economic_pending_claim_source WHERE source_operation_id=" +
				 hex(creator.bytes) +
				 " AND beneficiary_pid=" + std::to_string(beneficiary_pid),
			 1, &cells))
			return errno ? static_cast<unsigned int>(errno) : EILSEQ;
		uint64_t count = 0;
		return u64(cells[0], &count) && count == 0 ? 0 : EILSEQ;
	}
	catch (const std::bad_alloc &)
	{
		return ENOMEM;
	}
#endif
}

unsigned int economic_sql_pending_claim_endpoint_verify_zero_creator(
	MYSQL *connection, const critical_operation_id &creator, const economic_account_key &key,
	uint32_t beneficiary_pid)
{
	return verify_zero_creator(connection, creator, key, beneficiary_pid, false);
}

unsigned int economic_sql_pending_claim_endpoint_verify_retained_creator(
	MYSQL *connection, const critical_operation_id &creator, const economic_account_key &key,
	uint32_t beneficiary_pid)
{
#ifdef __NO_MYSQL__
	(void)connection;
	(void)creator;
	(void)key;
	(void)beneficiary_pid;
	return ENOTSUP;
#else
	if (!connection || !(connection->server_status & SERVER_STATUS_IN_TRANS) ||
	    !economic_account_key_valid(key) || key.kind != economic_account_kind::pending_claim ||
	    key.context_id || !beneficiary_pid || critical_operation_id_is_zero(creator))
		return EINVAL;
	try
	{
		using flag = std::remove_pointer_t<decltype(MYSQL_BIND{}.is_null)>;
		flag reconnect = false;
		if (mysql_get_option(connection, MYSQL_OPT_RECONNECT, &reconnect) || reconnect)
			return EPERM;
		const auto session = mysql_thread_id(connection);
		const auto same_session = [&]()
		{
			flag current_reconnect = false;
			return mysql_thread_id(connection) == session &&
			       (connection->server_status & SERVER_STATUS_IN_TRANS) &&
			       (connection->server_status & SERVER_STATUS_AUTOCOMMIT) &&
			       !mysql_get_option(connection, MYSQL_OPT_RECONNECT,
						 &current_reconnect) &&
			       !current_reconnect;
		};
		if (!same_session())
			return EPERM;
		// Borrow the original transaction. This proves immutable known creator
		// values; an unseen historical command header is never reconstructed.
		economic_frozen_intent intent;
		economic_accounting_plan plan;
		if (!endpoint_root(connection, creator, &intent, &plan))
			return errno ? static_cast<unsigned>(errno) : EILSEQ;
		const auto &meta = intent.admission.metadata;
		const auto &facts = intent.admission.facts;
		const auto number = [&](size_t offset, size_t width)
		{
			uint64_t value = 0;
			for (size_t i = 0; i < width; ++i)
				value |= static_cast<uint64_t>(facts[offset + i]) << (8 * i);
			return value;
		};
		uint16_t slot = 0;
		uint64_t auction = 0, revision = 0;
		size_t tag = 0, listing = 0, prior = 0;
		bool winner = false;
		if (meta.writer_id == ECONOMIC_WRITER_AUCTION_BID &&
		    meta.reason == economic_reason::auction_bid && facts.size() == 140)
		{
			tag = 124;
			listing = 92;
			prior = 108;
			auction = number(48, 4);
			revision = number(84, 8);
			winner = number(56, 4) != 0;
			if (number(132, 4) == beneficiary_pid && number(56, 4) == beneficiary_pid &&
			    meta.actor_id != beneficiary_pid && !number(32, 8))
				slot = 1;
			else if (number(136, 4) == beneficiary_pid &&
				 number(52, 4) == beneficiary_pid && !number(40, 8))
				slot = 2;
			else
				return EILSEQ;
		}
		else if (meta.writer_id == ECONOMIC_WRITER_AUCTION_SETTLEMENT &&
			 meta.reason == economic_reason::auction_settle && facts.size() >= 130)
		{
			const auto items = number(120, 2);
			if (!items || items > AUCTION_COMMAND_MAX_ITEMS ||
			    facts.size() != 130 + 27 * items)
				return EILSEQ;
			tag = 122 + 27 * items;
			listing = 88;
			prior = 104;
			auction = number(32, 4);
			revision = number(72, 8);
			winner = number(40, 4) != 0;
			if (!winner || number(tag + 4, 4) != beneficiary_pid ||
			    number(36, 4) != beneficiary_pid || number(8, 8))
				return EILSEQ;
			slot = 2;
		}
		else
			return EILSEQ;
		if (!auction || !revision || revision == UINT64_MAX ||
		    meta.lineage.bytes != key.lineage.bytes ||
		    meta.actor_kind != economic_actor_kind::domain || !meta.source_event ||
		    meta.source_event->kind != economic_source_kind::auction ||
		    meta.source_event->sequence != revision || meta.source_event->slot ||
		    meta.source_event->generation.bytes != meta.original_operation_id.bytes ||
		    !std::equal(facts.begin() + listing, facts.begin() + listing + 16,
				meta.original_operation_id.bytes.begin()) ||
		    !std::equal(facts.begin() + (winner ? prior : listing),
				facts.begin() + (winner ? prior : listing) + 16,
				meta.source_event->source.bytes.begin()) ||
		    !std::equal(facts.begin() + tag, facts.begin() + tag + 4,
				std::array<uint8_t, 4>{ 'A', 'E', 'C', '1' }.begin()) ||
		    !plan.children.empty() || !plan.item_events.empty())
			return EILSEQ;
		const auto effect = std::find_if(
			plan.accounts.begin(), plan.accounts.end(), [&](const auto &value)
			{ return economic_account_key_equal(value.key, key); });
		if (effect == plan.accounts.end() || effect->after[0] < 0 ||
		    static_cast<uint64_t>(effect->after[0]) > UINT_MAX || effect->after[1] ||
		    effect->after[2] || effect->after[3])
			return EILSEQ;
		const auto amount = static_cast<uint64_t>(effect->after[0]);
		if (!original_endpoint_effect(connection, creator, key, beneficiary_pid, amount,
					      plan, true))
			return errno ? static_cast<unsigned>(errno) : EILSEQ;
		if (!amount)
		{
			const auto error = slot == 2 ? verify_zero_creator(connection, creator, key,
									   beneficiary_pid, true) :
						       EILSEQ;
			return error ? error : same_session() ? 0 : EIO;
		}
		if (slot == 1 && amount != number(68, 8))
			return EILSEQ;
		std::vector<std::string> cells;
		if (slot == 2)
		{
			if (!row(connection,
				 "SELECT result_payload FROM critical_operation_inbox WHERE operation_id=" +
					 hex(creator.bytes) + " LOCK IN SHARE MODE",
				 1, &cells))
				return errno ? static_cast<unsigned>(errno) : EILSEQ;
			auction_command_result result{};
			if (!auction_command_decode_result(
				    reinterpret_cast<const uint8_t *>(cells[0].data()),
				    cells[0].size(), &result) ||
			    result.event_type != auction_event_type::sold ||
			    result.final_price <= 0 || result.final_price > UINT_MAX)
				return EILSEQ;
			const auto sink = std::find_if(
				plan.accounts.begin(), plan.accounts.end(),
				[&](const auto &value)
				{
					return value.key.kind == economic_account_kind::sink &&
					       value.key.authority_id ==
						       ECONOMIC_AUCTION_CLOSING_FEE_SINK_ID &&
					       value.key.lineage.bytes == key.lineage.bytes &&
					       !value.key.context_id;
				});
			if (sink == plan.accounts.end())
				return EILSEQ;
			const auto sink_index = static_cast<size_t>(sink - plan.accounts.begin());
			const auto credit_index =
				static_cast<size_t>(effect - plan.accounts.begin());
			uint64_t fee = 0, credit = 0;
			for (const auto &posting : plan.postings)
				if (posting.account_index == sink_index ||
				    posting.account_index == credit_index)
				{
					if (posting.copper < 0 ||
					    static_cast<uint64_t>(posting.copper) > UINT_MAX)
						return EILSEQ;
					auto &total = posting.account_index == sink_index ? fee :
											    credit;
					if (static_cast<uint64_t>(posting.copper) >
					    UINT_MAX - total)
						return EILSEQ;
					total += static_cast<uint64_t>(posting.copper);
				}
			if (credit != amount ||
			    fee + amount != static_cast<uint64_t>(result.final_price))
				return EILSEQ;
		}
		if (!row(connection,
			 "SELECT COUNT(*) FROM economic_pending_claim_source WHERE source_operation_id=" +
				 hex(creator.bytes) +
				 " AND beneficiary_pid=" + std::to_string(beneficiary_pid),
			 1, &cells))
			return errno ? static_cast<unsigned>(errno) : EILSEQ;
		uint64_t count = 0;
		if (!u64(cells[0], &count) || count != 1)
			return EILSEQ;
		if (!row(connection,
			 "SELECT COUNT(*) FROM economic_pending_claim_source WHERE source_operation_id=" +
				 hex(creator.bytes) +
				 " AND beneficiary_pid=" + std::to_string(beneficiary_pid) +
				 " AND source_slot=" + std::to_string(slot) +
				 " AND lineage=" + hex(key.lineage.bytes) +
				 " AND claim_mapping_id=" + std::to_string(key.authority_id) +
				 " AND amount=" + std::to_string(amount),
			 1, &cells))
			return errno ? static_cast<unsigned>(errno) : EILSEQ;
		if (!u64(cells[0], &count) || count != 1)
			return EILSEQ;
		return same_session() ? 0 : EIO;
	}
	catch (const std::bad_alloc &)
	{
		return ENOMEM;
	}
#endif
}
