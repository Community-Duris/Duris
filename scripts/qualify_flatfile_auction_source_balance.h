// Independent remaining claim-source totals and retained beneficiary lifetimes.
// Credit roots, source-set digests and consumer order are separate proof gates.
#ifndef DURIS_QUALIFY_FLATFILE_AUCTION_SOURCE_BALANCE_H
#define DURIS_QUALIFY_FLATFILE_AUCTION_SOURCE_BALANCE_H

#include "qualify_flatfile_auction_money.h"

namespace restore_auction_source_balance
{
using namespace restore_auction_money;
struct result : restore_auction_money::result
{
	bool current_values_verified = false;
	size_t source_findings = 0, consumed_rows = 0, unconsumed_rows = 0;
	size_t compared_claims = 0;
	bool balance_verified() const
	{
		return verified() && sources_present && catalog_present &&
		       compared_claims == native_claims;
	}
};
struct claim
{
	uint64_t pid;
	bool retired, seen = false;
	wide remaining = 0;
};
inline account_key source_account(const restore_native_auction::source_row &row)
{
	bytes encoded(row.lineage.begin(), row.lineage.end());
	put(encoded, 1, 2);
	put(encoded, 5, 2);
	put(encoded, row.mapping, 8);
	put(encoded, 0, 8);
	put(encoded, 0, 4);
	return fixed_account(encoded);
}
inline result audit(const std::filesystem::path &root, audit_budget &budget)
{
	result output;
	std::map<account_key, claim> claims;
	std::map<uint64_t, account_key> active;
	std::vector<restore_current_money::finding> findings;
	auto issue = [&](const char *code, const account_key &account = account_key{},
			 const identity &operation = identity{}, uint64_t pid = 0,
			 const observed_values *observed = nullptr)
	{
		++output.source_findings;
		if (findings.size() < maximum_findings)
		{
			restore_current_money::finding row{ account, {}, operation, code, 5, pid };
			if (observed)
				row.observed = *observed;
			findings.push_back(row);
		}
	};
	observers observe;
	observe.mapping = [&](const account_key &account, const auto &mapping)
	{
		if (number(account, 18, 2) != 5)
			return;
		const auto pid = number(mapping.key, 12, 8);
		need(claims.emplace(account, claim{ pid, mapping.retired }).second);
		if (!mapping.retired && number(account, 28, 8) == 0)
			need(active.emplace(pid, account).second);
	};
	observe.source = [&](const auto &row)
	{
		if (output.consumed_rows + output.unconsumed_rows == maximum_accounts)
			throw account_budget_refused();
		const bool consumed = nonzero(row.consumed);
		if (consumed)
			++output.consumed_rows;
		else
			++output.unconsumed_rows;
		const auto account = source_account(row);
		const auto found = claims.find(account);
		if (found == claims.end())
			issue("claim_source_lifetime_unknown", account, row.operation, row.pid);
		else if (found->second.pid != row.pid)
			issue("claim_source_beneficiary_mismatch", account, row.operation, row.pid);
		else if (!consumed)
		{
			if (found->second.retired)
				issue("retired_claim_source_unconsumed", account, row.operation,
				      row.pid);
			else
				found->second.remaining += row.amount;
		}
	};
	observe.holding = [&](const auto &holding)
	{
		if (holding.kind != 5)
			return;
		const auto found = active.find(holding.id);
		if (found == active.end())
			return; // The current-money reader reports the unmapped native claim.
		auto &state = claims.at(found->second);
		need(!state.seen);
		state.seen = true;
		++output.compared_claims;
		if (state.remaining != holding.amount)
		{
			const observed_values observed{ { state.remaining, 0, 0, 0 },
							{ holding.amount, 0, 0, 0 } };
			issue("claim_source_balance_mismatch", found->second, {}, holding.id,
			      &observed);
		}
	};
	observe.finish = [&](ledger &locked, restore_auction_money::result &values)
	{
		output.current_values_verified = values.verified();
		if (!values.sources_present && (values.initialized || values.catalog_present))
			issue("claim_source_catalog_missing");
		for (const auto &[account, state] : claims)
		{
			audit_checkpoint();
			if (!state.retired && !state.seen && state.remaining)
				issue("claim_sources_without_native_claim", account, {}, state.pid);
		}
		for (const auto &row : findings)
			locked.issue(row.code, row.account, row.operation, 5, row.native_id, {},
				     row.observed ? &*row.observed : nullptr);
		values.finding_count += output.source_findings - findings.size();
	};
	static_cast<restore_auction_money::result &>(output) =
		restore_auction_money::audit(root, budget, observe);
	return output;
}
} // namespace restore_auction_source_balance
#endif
