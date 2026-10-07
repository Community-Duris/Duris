// Independent creator attribution, including consumed historical source rows.
// This does not establish source-set digests, consumption order or origins.
#ifndef DURIS_QUALIFY_FLATFILE_AUCTION_SOURCE_CREDIT_H
#define DURIS_QUALIFY_FLATFILE_AUCTION_SOURCE_CREDIT_H

#include "qualify_flatfile_auction_source_balance.h"

namespace restore_auction_source_credit
{
using namespace restore_auction_source_balance;
using source_key = std::pair<identity, uint16_t>;
struct result : restore_auction_source_balance::result
{
	size_t creator_roots = 0, expected_credits = 0, compared_credits = 0, credit_findings = 0;
	bool credits_verified() const
	{
		return initialized && catalog_present && sources_present && !credit_findings &&
		       expected_credits == compared_credits;
	}
};
struct invalid_credit_proof
{
};
inline void expect(bool value)
{
	if (!value)
		throw invalid_credit_proof();
}
inline int64_t signed_at(std::span<const uint8_t> value, size_t offset)
{
	return std::bit_cast<int64_t>(number(value, offset, 8));
}
inline identity identity_at(std::span<const uint8_t> value, size_t offset)
{
	identity result;
	const auto field = value.subspan(offset, 16);
	std::copy_n(field.begin(), 16, result.begin());
	return result;
}
inline account_key account_at(const identity &lineage, uint16_t kind, uint64_t id,
			      uint64_t context = 0)
{
	bytes encoded(lineage.begin(), lineage.end());
	put(encoded, 1, 2);
	put(encoded, kind, 2);
	put(encoded, id, 8);
	put(encoded, context, 8);
	put(encoded, 0, 4);
	return fixed_account(encoded);
}
inline coins canonical(int64_t amount)
{
	coins result = {};
	constexpr std::array<int64_t, 4> units = { 1, 10, 100, 1000 };
	for (size_t i = 4; i-- > 0;)
	{
		result[i] = amount / units[i];
		amount %= units[i];
	}
	return result;
}
struct root_receipt
{
	digest command_digest;
	bytes result;
};
struct credit
{
	account_key account;
	uint32_t pid;
	uint64_t amount;
	bool seen = false;
};
class creator
{
	const restore_economic_records::record_view &root;
	const std::map<account_key, restore_economic_authority::mapping> &mappings;
	std::span<const uint8_t> payload, facts, receipt;
	identity lineage;
	std::map<account_key, std::span<const uint8_t>> accounts;
	std::set<account_key> used;
	std::vector<std::pair<source_key, credit>> credits;
	uint64_t actor, auction, race, fee_basis;
	void fences(std::span<const uint8_t> account_name)
	{
		using fence_key = std::pair<uint64_t, uint64_t>;
		std::set<fence_key> keys{ { 7, auction } };
		std::map<fence_key, uint64_t> revisions;
		if (actor)
		{
			bytes canonical_name;
			for (auto c : account_name)
			{
				if (!c)
					break;
				expect(c >= 0x20);
				canonical_name.push_back(c >= 'A' && c <= 'Z' ? c + ('a' - 'A') :
										c);
			}
			expect(!canonical_name.empty());
			canonical_name.push_back(0);
			canonical_name.push_back(race);
			const auto digest = hash(canonical_name);
			const auto account_id = number(digest, 0, 8);
			const fence_key player{ 1, actor }, bank{ 2, account_id ? account_id : 1 };
			keys.insert(player);
			keys.insert(bank);
			revisions.emplace(player, number(payload, 10, 8));
			revisions.emplace(bank, number(payload, 18, 8));
		}
		for (size_t i = 0; i < number(payload, 74, 2); ++i)
		{
			const fence_key item{ 3, number(payload, 76 + i * 20, 8) };
			expect(keys.insert(item).second);
			revisions.emplace(item, number(payload, 84 + i * 20, 8));
		}
		expect(number(root.command, 40, 4) == keys.size() &&
		       number(root.command, 44, 4) == revisions.size());
		reader in{ root.command.subspan(52) };
		for (const auto &key : keys)
		{
			const auto kind = in.number(1);
			(void)in.take(7); // Already checked by the common authenticated grammar.
			expect(fence_key{ kind, in.number(8) } == key);
		}
		for (const auto &[key, revision] : revisions)
		{
			const auto kind = in.number(1);
			(void)in.take(7);
			expect(fence_key{ kind, in.number(8) } == key && in.number(8) == revision);
		}
	}
	uint64_t n(std::span<const uint8_t> wire, size_t offset, size_t width = 8) const
	{
		return number(wire, offset, width);
	}
	void mapped(const account_key &key, uint16_t kind, uint64_t native,
		    uint64_t context = 0) const
	{
		const auto found = mappings.find(key);
		expect(found != mappings.end() && number(key, 18, 2) == kind &&
		       number(key, 28, 8) == context);
		// Bank names can change after admission; the frozen immutable lifetime
		// and race context remain the identity, not today's native alias.
		if (kind != 2)
			expect(n(found->second.key, 12) == native);
	}
	std::span<const uint8_t> take(uint16_t kind, uint64_t id, uint64_t context = 0)
	{
		const auto key = account_at(lineage, kind, id, context);
		const auto found = accounts.find(key);
		expect(id && found != accounts.end() && used.insert(key).second);
		return found->second;
	}
	void delta(std::span<const uint8_t> row, const coins &before, const coins &after,
		   uint64_t revision)
	{
		expect(revision != UINT64_MAX && coin_vector(row, 40) == before &&
		       coin_vector(row, 72) == after && n(row, 104) == revision &&
		       n(row, 112) == revision + 1);
	}
	void claim(uint64_t frozen_id, uint64_t absent, uint32_t pid, int64_t change, uint16_t slot,
		   bool debit = false)
	{
		account_key key = account_at(lineage, 5, frozen_id);
		if (absent)
		{
			expect(!frozen_id && absent == pid && !debit);
			bool found = false;
			for (const auto &[candidate, row] : accounts)
			{
				(void)row;
				if (number(candidate, 18, 2) != 5)
					continue;
				const auto mapping = mappings.find(candidate);
				if (mapping != mappings.end() &&
				    n(mapping->second.key, 12) == pid &&
				    mapping->second.creating_operation == root.operation)
				{
					expect(!found);
					key = candidate;
					found = true;
				}
			}
			expect(found);
		}
		mapped(key, 5, pid);
		const auto row = take(5, number(key, 20, 8));
		const auto before = signed_at(row, 40);
		expect(before >= 0 && before <= UINT32_MAX && wide(before) + change >= 0 &&
		       wide(before) + change <= UINT32_MAX);
		if (absent)
			expect(!before && !n(row, 104));
		delta(row, { before, 0, 0, 0 }, { before + change, 0, 0, 0 }, n(row, 104));
		if (change > 0)
		{
			expect(change <= INT32_MAX);
			credits.push_back({ { root.operation, slot },
					    { key, pid, static_cast<uint64_t>(change) } });
		}
	}
	void metadata(uint64_t writer, uint64_t reason, uint64_t id, const identity &listing,
		      const identity &source, uint64_t revision, uint64_t slot = 0)
	{
		const auto intent = root.intent;
		expect(n(intent, 12, 4) == writer && n(intent, 16, 4) == 1 &&
		       n(intent, 20, 4) == 1 && n(intent, 24, 2) == reason && intent[26] == 1 &&
		       intent[27] == 1 && n(intent, 96) == id &&
		       identity_at(intent, 80) == listing && n(intent, 112, 2) == 13 &&
		       n(intent, 114, 2) == 1 && identity_at(intent, 116) == source &&
		       identity_at(intent, 132) == listing && n(intent, 148) == revision &&
		       n(intent, 156, 4) == slot);
	}
	void common_listing(uint64_t id, uint64_t seller, uint64_t winner, uint64_t status,
			    uint64_t custody, int64_t price, int64_t buy, uint64_t revision,
			    const identity &listing, const identity &bid)
	{
		expect(id && id == auction && seller && status == 1 && custody == 1 && price >= 0 &&
		       price <= UINT32_MAX && buy >= 0 && revision && revision != UINT64_MAX &&
		       nonzero(listing) &&
		       (winner ? price > 0 && nonzero(bid) && bid != listing : !nonzero(bid)));
	}
	void result_header(uint64_t action, uint64_t event, uint64_t status, uint64_t seller,
			   uint64_t winner, uint64_t previous, int64_t price, int64_t wallet_delta,
			   uint64_t revision, int64_t credit_used)
	{
		expect(receipt.size() == 320 && receipt[0] == action && receipt[1] == event &&
		       n(receipt, 2, 4) == auction && n(receipt, 6, 4) == status &&
		       n(receipt, 10, 4) == seller && n(receipt, 14, 4) == winner &&
		       n(receipt, 18, 4) == previous && signed_at(receipt, 22) == price &&
		       signed_at(receipt, 30) == wallet_delta && n(receipt, 118) == revision + 1 &&
		       !n(receipt, 126) && !n(receipt, 134) && !n(receipt, 142, 2) &&
		       signed_at(receipt, 144) == credit_used);
	}
	void sink(int64_t amount)
	{
		if (amount)
		{
			const auto row = take(8, 25);
			expect(!nonzero(row.subspan(40)));
		}
	}
	void bid()
	{
		expect(facts.size() == 124 || facts.size() == 140);
		std::array<uint64_t, 3> absent = {};
		if (facts.size() == 140)
		{
			expect(same(facts.subspan(124, 4), bytes{ 'A', 'E', 'C', '1' }));
			for (size_t i = 0; i < 3; ++i)
				absent[i] = n(facts, 128 + i * 4, 4);
			expect(absent[0] || absent[1] || absent[2]);
		}
		const auto seller = n(facts, 52, 4), previous = n(facts, 56, 4),
			   revision = n(facts, 84);
		const auto price = signed_at(facts, 68), buy = signed_at(facts, 76);
		const auto listing = identity_at(facts, 92), prior_bid = identity_at(facts, 108);
		common_listing(n(facts, 48, 4), seller, previous, n(facts, 60, 4), n(facts, 64, 4),
			       price, buy, revision, listing, prior_bid);
		expect(actor && seller != actor && signed_at(payload, 26) > 0);
		const auto value = signed_at(payload, 26);
		const auto final = buy && value >= buy ? buy : value;
		const bool sold = buy > 0 && final >= buy, outbid = previous && previous != actor;
		expect(final > 0 && final <= UINT32_MAX &&
		       (previous ? final > price : final >= price));
		metadata(9, 27, actor, listing, previous ? prior_bid : listing, revision);
		mapped(account_at(lineage, 1, n(facts, 0)), 1, actor);
		mapped(account_at(lineage, 2, n(facts, 8), race), 2, 0, race);
		mapped(account_at(lineage, 4, n(facts, 16)), 4, auction);
		// Every frozen endpoint is bound even when it has no plan effect.
		if (absent[0])
			expect(!n(facts, 24) && absent[0] == actor);
		else
			mapped(account_at(lineage, 5, n(facts, 24)), 5, actor);
		expect(outbid || (!n(facts, 32) && !absent[1]));
		expect(sold || (!n(facts, 40) && !absent[2]));
		const auto to_pay = previous == actor ? final - price : final;
		const auto credit_used = signed_at(receipt, 144);
		expect(credit_used >= 0 && credit_used <= to_pay && (!absent[0] || !credit_used));
		if (credit_used)
		{
			const auto key = account_at(lineage, 5, n(facts, 24));
			const auto found = accounts.find(key);
			expect(found != accounts.end() &&
			       credit_used == std::min(to_pay, signed_at(found->second, 40)));
			claim(n(facts, 24), 0, actor, -credit_used, 0, true);
		}
		const auto fee = sold ? static_cast<int64_t>(wide(final) * fee_basis / 10000) : 0;
		if (outbid)
			claim(n(facts, 32), absent[1], previous, price, 1);
		if (sold)
			claim(n(facts, 40), absent[2], seller, final - fee, 2);
		const auto wallet = take(1, n(facts, 0));
		const auto before = coin_vector(wallet, 40);
		const auto money = coin_value(before);
		const auto payment = to_pay - credit_used;
		expect(money >= payment && n(wallet, 104) == n(payload, 10) &&
		       n(receipt, 102) == n(payload, 10) + 1 && n(payload, 18) != UINT64_MAX &&
		       n(receipt, 110) == n(payload, 18) + 1);
		const auto after = canonical(static_cast<int64_t>(money) - payment);
		delta(wallet, before, after, n(payload, 10));
		expect(coin_vector(receipt, 38) == after);
		const auto escrow = take(4, n(facts, 16));
		delta(escrow, { previous ? price : 0, 0, 0, 0 }, { sold ? 0 : final, 0, 0, 0 },
		      revision);
		sink(fee);
		result_header(2, sold ? 3 : 2, sold ? 2 : 1, seller, actor, previous, final,
			      -payment, revision, credit_used);
		expect(!n(root.plan, 228, 4) && !n(root.plan, 232, 4) && !n(root.plan, 236, 4));
	}
	void settlement()
	{
		expect(facts.size() >= 122);
		const auto items = n(facts, 120, 2), size = 122 + items * 27;
		expect(items && items <= 9 && (facts.size() == size || facts.size() == size + 8));
		uint64_t absent = 0;
		if (facts.size() != size)
		{
			expect(same(facts.subspan(size, 4), bytes{ 'A', 'E', 'C', '1' }));
			absent = n(facts, size + 4, 4);
			expect(absent);
		}
		const auto seller = n(facts, 36, 4), winner = n(facts, 40, 4),
			   revision = n(facts, 72);
		const auto price = signed_at(facts, 56), buy = signed_at(facts, 64);
		const auto listing = identity_at(facts, 88), winning_bid = identity_at(facts, 104);
		common_listing(n(facts, 32, 4), seller, winner, n(facts, 44, 4), n(facts, 48, 4),
			       price, buy, revision, listing, winning_bid);
		expect(n(facts, 52, 4) == items && n(facts, 80));
		const bool remove = payload[0] == 6, sale = !remove && winner;
		metadata(11, remove ? 29 : 30, actor ? actor : auction, listing,
			 winner ? winning_bid : listing, revision, remove ? 1 : 0);
		mapped(account_at(lineage, 4, n(facts, 0)), 4, auction);
		if (actor)
		{
			mapped(account_at(lineage, 1, n(facts, 16)), 1, actor);
			mapped(account_at(lineage, 2, n(facts, 24), race), 2, 0, race);
			expect(n(receipt, 102) == n(payload, 10) &&
			       n(receipt, 110) == n(payload, 18));
		}
		else
			expect(!n(facts, 16) && !n(facts, 24));
		const auto held = winner ? price : 0;
		const auto fee = sale ? static_cast<int64_t>(wide(held) * fee_basis / 10000) : 0;
		delta(take(4, n(facts, 0)), { held, 0, 0, 0 }, { sale ? 0 : held, 0, 0, 0 },
		      revision);
		if (sale)
			claim(n(facts, 8), absent, seller, held - fee, 2);
		else
			expect(!n(facts, 8) && !absent);
		sink(fee);
		result_header(payload[0],
			      remove ? 7 :
			      winner ? 3 :
				       4,
			      remove ? 3 : 2, seller, winner, 0, price, 0, revision, 0);
		expect(n(root.plan, 228, 4) == items && n(root.plan, 232, 4) == items &&
		       !n(root.plan, 236, 4));
		const auto offset = 256 + n(root.plan, 216, 4) * 120 + n(root.plan, 220, 4) * 48;
		const auto before = root.plan.subspan(offset, items * 64),
			   after = root.plan.subspan(offset + items * 64, items * 64);
		expect(same(before, after));
		std::set<uint64_t> seen;
		for (size_t i = 0; i < items; ++i)
		{
			const auto frozen = facts.subspan(122 + i * 27, 27);
			const auto uid = n(frozen, 0), item_revision = n(frozen, 8);
			expect(uid && item_revision && item_revision != UINT64_MAX &&
			       n(frozen, 16, 2) == i && n(frozen, 18, 4) <= INT32_MAX &&
			       !n(frozen, 22, 4) && !frozen[26] && seen.insert(uid).second);
			bool found = false;
			for (size_t j = 0; j < items; ++j)
			{
				const auto row = before.subspan(j * 64, 64);
				if (n(row, 0) != uid)
					continue;
				found = true;
				expect(row[8] == 6 && row[9] == 1 && n(row, 16) == auction &&
				       !n(row, 24) && n(row, 32) == uid && !n(row, 40) &&
				       n(row, 48) == item_revision && !n(row, 56, 2));
			}
			expect(found);
		}
	}

    public:
	creator(const restore_economic_records::record_view &value,
		const std::map<account_key, restore_economic_authority::mapping> &authority)
		: root(value)
		, mappings(authority)
		, payload(root.payload)
		, facts(root.intent.subspan(256))
		, receipt(root.result)
		, lineage(identity_at(root.intent, 32))
		, actor(number(payload, 1, 4))
		, auction(number(payload, 5, 4))
		, race(number(payload, 9, 1))
		, fee_basis(number(payload, 58, 4))
	{
	}
	auto run()
	{
		expect(root.payload_version == 1 && payload.size() >= 90 && receipt.size() == 320 &&
		       (payload[0] == 2 || payload[0] == 3 || payload[0] == 6) &&
		       fee_basis <= 10000);
		reader in{ payload.subspan(74) };
		const auto items = in.number(2);
		expect(items <= 9);
		for (size_t i = 0; i < items; ++i)
		{
			expect(in.number(8) != 0);
			(void)in.number(8);
			expect(in.number(4) <= INT32_MAX);
		}
		std::span<const uint8_t> account_name;
		for (auto limit : { 50, 32, 255, 1024, 8192 })
		{
			const auto length = in.number(2);
			expect(length <= static_cast<uint64_t>(limit));
			const auto text = in.take(length);
			if (limit == 50)
				account_name = text;
		}
		const auto blob = in.number(4);
		expect(blob < 32768);
		(void)in.take(blob);
		in.done();
		fences(account_name);
		for (size_t i = 0; i < number(root.plan, 216, 4); ++i)
		{
			const auto row = root.plan.subspan(256 + i * 120, 120);
			expect(accounts.emplace(fixed_account(row.first(40)), row).second);
		}
		if (payload[0] == 2)
			bid();
		else
			settlement();
		expect(used.size() == accounts.size());
		return credits;
	}
};
inline result audit(const std::filesystem::path &path, audit_budget &budget)
{
	result output;
	std::map<account_key, restore_economic_authority::mapping> mappings;
	std::map<identity, root_receipt> roots, receipts;
	std::map<source_key, credit> expected;
	std::vector<restore_current_money::finding> findings;
	auto issue = [&](const char *code, const identity &operation = {},
			 const account_key &account = {}, uint64_t pid = 0)
	{
		++output.credit_findings;
		if (findings.size() < maximum_findings)
			findings.push_back({ account, {}, operation, code, 5, pid });
	};
	observers observe;
	observe.mapping = [&](const account_key &key, const auto &value)
	{
		const auto kind = number(key, 18, 2);
		if (kind != 1 && kind != 2 && kind != 4 && kind != 5)
			return;
		if (mappings.size() == maximum_accounts)
			throw account_budget_refused();
		need(mappings.emplace(key, value).second);
	};
	observe.record = [&](const auto &root)
	{
		if (root.result_code || !root.witness.empty())
			return;
		const auto writer = number(root.intent, 12, 4);
		if (root.type != 7 || (writer != 9 && writer != 11))
		{
			for (size_t i = 0; i < number(root.plan, 216, 4); ++i)
			{
				const auto row = root.plan.subspan(256 + i * 120, 120);
				if (number(row, 18, 2) == 5 &&
				    coin_value(coin_vector(row, 72)) >
					    coin_value(coin_vector(row, 40)))
					issue("claim_source_credit_writer_unproven", root.operation,
					      fixed_account(row.first(40)));
			}
			return;
		}
		if (roots.size() == maximum_accounts)
			throw account_budget_refused();
		need(roots.emplace(root.operation,
				   root_receipt{ hash(root.command),
						 bytes(root.result.begin(),
						       root.result.begin() +
							       std::min<size_t>(
								       152, root.result.size())) })
			     .second);
		++output.creator_roots;
		try
		{
			const auto credits = creator(root, mappings).run();
			for (const auto &[key, value] : credits)
			{
				if (expected.size() == maximum_accounts)
					throw account_budget_refused();
				need(expected.emplace(key, value).second);
			}
		}
		catch (const invalid_credit_proof &)
		{
			issue("claim_source_creator_semantics_invalid", root.operation);
		}
	};
	observe.receipt = [&](const auto &row)
	{
		if (receipts.size() == maximum_accounts)
			throw account_budget_refused();
		need(receipts.emplace(row.operation,
				      root_receipt{
					      row.command_digest,
					      bytes(row.result.begin(), row.result.begin() + 152) })
			     .second);
		// Publishing the auction event changes this native receipt's flag, not
		// its credit. The result codec explicitly ignores trailing padding.
		if (roots.contains(row.operation) && row.result_code)
			issue("claim_source_creator_native_receipt_invalid", row.operation);
	};
	observe.source = [&](const auto &row)
	{
		const auto account = source_account(row);
		const auto found = expected.find({ row.operation, row.slot });
		if (found == expected.end())
			issue("claim_source_creator_missing_or_unproven", row.operation, account,
			      row.pid);
		else
		{
			found->second.seen = true;
			if (found->second.account != account || found->second.pid != row.pid ||
			    found->second.amount != row.amount)
				issue("claim_source_credit_mismatch", row.operation, account,
				      row.pid);
			else
				++output.compared_credits;
		}
	};
	observe.finish = [&](ledger &locked, restore_auction_money::result &values)
	{
		output.expected_credits = expected.size();
		for (const auto &[operation, root] : roots)
		{
			audit_checkpoint();
			const auto found = receipts.find(operation);
			if (found == receipts.end())
				issue("claim_source_creator_native_receipt_missing", operation);
			else if (root.command_digest != found->second.command_digest ||
				 root.result != found->second.result)
				issue("claim_source_creator_native_receipt_mismatch", operation);
		}
		for (const auto &[key, value] : expected)
		{
			audit_checkpoint();
			if (!value.seen)
				issue("claim_source_credit_row_missing", key.first, value.account,
				      value.pid);
		}
		for (const auto &row : findings)
			locked.issue(row.code, row.account, row.operation, 5, row.native_id);
		values.finding_count += output.credit_findings - findings.size();
	};
	static_cast<restore_auction_source_balance::result &>(output) =
		restore_auction_source_balance::audit(path, budget, observe);
	return output;
}
} // namespace restore_auction_source_credit
#endif
