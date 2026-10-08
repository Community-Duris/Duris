// Independent native source availability, whole-row consumption and frozen sets.
// This grants no escrow, item, account-origin or whole-release qualification.
#ifndef DURIS_QUALIFY_FLATFILE_AUCTION_SOURCE_CONSUMPTION_H
#define DURIS_QUALIFY_FLATFILE_AUCTION_SOURCE_CONSUMPTION_H

#include "qualify_flatfile_auction_source_credit.h"

namespace restore_auction_source_consumption
{
using namespace restore_auction_source_credit;
struct result : restore_auction_source_credit::result
{
	size_t consumer_roots = 0, compared_consumers = 0, selected_source_rows = 0;
	size_t cashout_roots = 0, verified_digests = 0, consumption_findings = 0;
	bool consumption_verified() const
	{
		return initialized && sources_present && catalog_present && history.valid() &&
		       !consumption_findings && consumer_roots == compared_consumers;
	}
	bool digests_verified() const
	{
		return consumption_verified() && cashout_roots == verified_digests;
	}
	bool attribution_verified() const
	{
		return balance_verified() && credits_verified() && consumption_verified() &&
		       digests_verified();
	}
};
struct step
{
	identity operation = {};
	int64_t before = 0, after = 0;
	uint64_t before_revision = 0, after_revision = 0;
	bool baseline = false;
};
struct consumer
{
	account_key account = {};
	uint32_t pid = 0, count = 0;
	uint64_t amount = 0, revision = 0;
	identity original = {};
	uint32_t slot = 0;
	digest frozen_digest = {}, command_digest = {};
	bytes result;
	bool cashout = false;
};
struct native_receipt : root_receipt
{
	uint64_t code = 0;
};
inline result audit(const std::filesystem::path &path, audit_budget &budget)
{
	result output;
	std::map<account_key, restore_economic_authority::mapping> mappings;
	std::map<account_key, std::vector<step>> timelines;
	std::map<identity, consumer> consumers;
	std::map<identity, native_receipt> receipts;
	std::map<source_key, restore_native_auction::source_row> sources;
	std::map<std::pair<account_key, identity>, std::vector<source_key>> creations;
	std::map<identity, std::vector<source_key>> consumed;
	std::set<source_key> selected;
	size_t steps = 0;
	std::vector<restore_current_money::finding> findings;
	auto issue = [&](const char *code, const identity &operation = {},
			 const account_key &account = {}, uint64_t pid = 0)
	{
		++output.consumption_findings;
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
		if (root.result_code)
			return;
		const bool baseline = !root.witness.empty();
		const auto writer = number(root.intent, 12, 4);
		bool semantics = false;
		if (!baseline && root.type == 7 && root.payload_version == 1 &&
		    (writer == 9 || writer == 13))
		{
			try
			{
				creator proof(root, mappings);
				if (writer == 9)
					(void)proof.run();
				else
					proof.validate_cashout();
				semantics = true;
			}
			catch (const invalid_credit_proof &)
			{
				issue("claim_source_consumer_semantics_invalid", root.operation);
			}
		}
		for (size_t i = 0; i < number(root.plan, 216, 4); ++i)
		{
			audit_checkpoint();
			const auto row = root.plan.subspan(256 + i * 120, 120);
			if (number(row, 18, 2) != 5)
				continue;
			if (steps == maximum_accounts)
				throw account_budget_refused();
			++steps;
			const auto account = fixed_account(row.first(40));
			step value{ root.operation,	 signed_at(row, 40),  signed_at(row, 72),
				    number(row, 104, 8), number(row, 112, 8), baseline };
			if (baseline)
			{
				value.before = value.after;
				value.before_revision = value.after_revision =
					number(root.witness, 192 + i * 112 + 72, 8);
			}
			if (coin_vector(row, 40) != coins{ signed_at(row, 40), 0, 0, 0 } ||
			    coin_vector(row, 72) != coins{ signed_at(row, 72), 0, 0, 0 } ||
			    value.before < 0 || value.after < 0 || value.before > UINT32_MAX ||
			    value.after > UINT32_MAX ||
			    (!baseline && (value.before_revision == UINT64_MAX ||
					   value.after_revision != value.before_revision + 1)))
				issue("claim_source_history_shape_unproven", root.operation,
				      account);
			timelines[account].push_back(value);
			if (baseline || value.after >= value.before)
				continue;
			if (!semantics)
			{
				issue("claim_source_consumer_writer_unproven", root.operation,
				      account);
				continue;
			}
			consumer proof;
			proof.account = account;
			proof.pid = number(root.payload, 1, 4);
			proof.amount = static_cast<uint64_t>(value.before - value.after);
			proof.revision = value.before_revision;
			proof.command_digest = hash(root.command);
			proof.result.assign(root.result.begin(), root.result.begin() + 152);
			proof.cashout = writer == 13;
			if (proof.cashout)
			{
				proof.count = number(root.intent, 256 + 44, 4);
				const auto digest = root.intent.subspan(256 + 48, 32);
				std::copy_n(digest.begin(), 32, proof.frozen_digest.begin());
				proof.original = identity_at(root.intent, 80);
				proof.slot = number(root.intent, 156, 4);
				++output.cashout_roots;
			}
			if (consumers.size() == maximum_accounts)
				throw account_budget_refused();
			if (!consumers.emplace(root.operation, std::move(proof)).second)
				issue("claim_source_consumer_multiple_lifetimes", root.operation,
				      account);
			++output.consumer_roots;
		}
	};
	observe.receipt = [&](const auto &row)
	{
		if (receipts.size() == maximum_accounts)
			throw account_budget_refused();
		need(receipts.emplace(row.operation,
				      native_receipt{ { row.command_digest,
							bytes(row.result.begin(),
							      row.result.begin() + 152) },
						      row.result_code })
			     .second);
	};
	observe.source = [&](const auto &row)
	{
		if (sources.size() == maximum_accounts)
			throw account_budget_refused();
		const source_key key{ row.operation, row.slot };
		need(sources.emplace(key, row).second);
		creations[{ source_account(row), row.operation }].push_back(key);
		if (nonzero(row.consumed))
			consumed[row.consumed].push_back(key);
	};
	observe.finish = [&](ledger &locked, restore_auction_money::result &values)
	{
		for (auto &[account, events] : timelines)
		{
			audit_checkpoint();
			std::sort(events.begin(), events.end(),
				  [](const auto &a, const auto &b)
				  {
					  return std::tuple(a.before_revision, !a.baseline,
							    a.after_revision, a.operation) <
						 std::tuple(b.before_revision, !b.baseline,
							    b.after_revision, b.operation);
				  });
			std::map<source_key, uint64_t> pool;
			wide balance = 0;
			std::optional<uint64_t> revision;
			for (const auto &event : events)
			{
				audit_checkpoint();
				if (revision && event.before_revision != *revision)
					issue("claim_source_revision_discontinuity",
					      event.operation, account);
				if (balance != event.before)
					issue("claim_source_available_balance_mismatch",
					      event.operation, account);
				revision = event.after_revision;
				if (event.baseline)
					continue; // A witnessed observation cannot invent source rows.
				if (event.after > event.before)
				{
					wide added = 0;
					for (const auto &key :
					     creations[{ account, event.operation }])
					{
						const auto &source = sources.at(key);
						need(pool.emplace(key, source.amount).second);
						added += source.amount;
					}
					if (added != wide(event.after) - event.before)
						issue("claim_source_created_total_mismatch",
						      event.operation, account);
					balance += added;
				}
				else if (event.after < event.before)
				{
					const auto found = consumers.find(event.operation);
					if (found == consumers.end() ||
					    found->second.account != account)
						continue;
					const auto &proof = found->second;
					uint64_t remaining = proof.amount;
					std::vector<source_key> prefix;
					for (const auto &[key, amount] : pool)
					{
						audit_checkpoint();
						if (!remaining)
							break;
						if (amount > remaining)
							break; // Native selection never splits or skips a row.
						prefix.push_back(key);
						remaining -= amount;
					}
					if (remaining || prefix.empty())
					{
						issue("claim_source_whole_row_selection_unproven",
						      event.operation, account, proof.pid);
						continue;
					}
					++output.compared_consumers;
					for (const auto &key : prefix)
					{
						const auto &source = sources.at(key);
						selected.insert(key);
						++output.selected_source_rows;
						if (source.consumed != event.operation)
							issue("claim_source_consumer_marker_mismatch",
							      event.operation, account, proof.pid);
						balance -= source.amount;
						pool.erase(key);
					}
				}
			}
		}
		for (const auto &[operation, proof] : consumers)
		{
			audit_checkpoint();
			const auto receipt = receipts.find(operation);
			if (receipt == receipts.end())
				issue("claim_source_consumer_native_receipt_missing", operation,
				      proof.account, proof.pid);
			else if (receipt->second.code ||
				 receipt->second.command_digest != proof.command_digest ||
				 receipt->second.result != proof.result)
				issue("claim_source_consumer_native_receipt_mismatch", operation,
				      proof.account, proof.pid);
			if (!proof.cashout)
				continue;
			const auto &rows = consumed[operation];
			bool valid = rows.size() == proof.count && !rows.empty() &&
				     rows.size() <= 4096;
			bytes encoded;
			constexpr std::string_view tag = "DURIS-PENDING-CLAIM-SOURCES-V1";
			encoded.insert(encoded.end(), tag.begin(), tag.end());
			put(encoded, rows.size(), 4);
			wide total = 0;
			for (const auto &key : rows)
			{
				audit_checkpoint();
				const auto &source = sources.at(key);
				valid = valid && source_account(source) == proof.account &&
					source.pid == proof.pid && selected.contains(key);
				encoded.insert(encoded.end(), key.first.begin(), key.first.end());
				put(encoded, key.second, 2);
				put(encoded, source.pid, 4);
				put(encoded, source.mapping, 8);
				put(encoded, source.amount, 8);
				total += source.amount;
			}
			if (!rows.empty())
				valid = valid && rows.front().first == proof.original &&
					rows.front().second == proof.slot &&
					proof.original != operation;
			if (!valid || total != proof.amount || hash(encoded) != proof.frozen_digest)
				issue("claim_source_frozen_digest_mismatch", operation,
				      proof.account, proof.pid);
			else
				++output.verified_digests;
		}
		for (const auto &[key, source] : sources)
		{
			audit_checkpoint();
			if (nonzero(source.consumed) && !consumers.contains(source.consumed))
				issue("claim_source_consumer_missing_or_unproven", source.consumed,
				      source_account(source), source.pid);
			else if (nonzero(source.consumed) && !selected.contains(key))
				issue("claim_source_consumed_row_not_selected", source.consumed,
				      source_account(source), source.pid);
		}
		for (const auto &row : findings)
			locked.issue(row.code, row.account, row.operation, 5, row.native_id);
		values.finding_count += output.consumption_findings - findings.size();
	};
	static_cast<restore_auction_source_credit::result &>(output) =
		restore_auction_source_credit::audit(path, budget, observe);
	return output;
}
} // namespace restore_auction_source_consumption
#endif
