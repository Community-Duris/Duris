// Independent current wallet/bank comparison. No native mutation or recovery.
#ifndef DURIS_QUALIFY_FLATFILE_WALLET_BANK_H
#define DURIS_QUALIFY_FLATFILE_WALLET_BANK_H

#include "qualify_flatfile_economic_money_history.h"
#include "qualify_flatfile_native_domains.h"
#include <charconv>
#include <map>

namespace restore_wallet_bank
{
using namespace restore_economic_money_history;
constexpr size_t maximum_accounts = 32768;
struct account_budget_refused : audit_budget_refused
{
};
struct observed_values
{
	std::array<uint64_t, 4> native = {};
	coins expected = {};
	uint64_t native_revision = 0, expected_revision = 0;
};
struct finding
{
	account_key account = {};
	identity epoch = {}, operation = {};
	const char *code;
	uint16_t native_kind = 0;
	uint64_t native_id = 0;
	std::optional<observed_values> observed = {};
};
struct result
{
	bool initialized = false;
	identity epoch = {};
	size_t active_accounts = 0, retired_epoch_accounts = 0, prior_epoch_accounts = 0;
	size_t native_wallets = 0, native_banks = 0, compared_accounts = 0;
	size_t unanchored_current_accounts = 0, finding_count = 0;
	std::vector<finding> findings;
	restore_economic_money_history::result history;
	bool valid() const { return finding_count == 0; }
	bool verified() const
	{
		return initialized && nonzero(epoch) && valid() && history.valid() &&
		       active_accounts == compared_accounts;
	}
};
struct account_state
{
	bool retired = false, seen = false, has_history = false;
	terminal tail = {};
};
inline account_key fixed_account(std::span<const uint8_t> value)
{
	need(value.size() == 40);
	account_key result;
	std::copy_n(value.begin(), 40, result.begin());
	return result;
}
inline uint64_t decimal(const std::string &value, uint64_t maximum)
{
	uint64_t result = 0;
	auto parsed = std::from_chars(value.data(), value.data() + value.size(), result);
	need(!value.empty() && parsed.ec == std::errc{} &&
	     parsed.ptr == value.data() + value.size() && result <= maximum &&
	     std::to_string(result) == value);
	return result;
}
inline bytes native_key(uint16_t kind, uint64_t context, uint64_t id,
			const std::string &account = {})
{
	bytes key;
	put(key, kind, 2);
	put(key, context, 8);
	put(key, kind, 2);
	if (kind == 1)
		put(key, id, 8);
	else
		key.insert(key.end(), account.begin(), account.end());
	return key;
}
inline result audit(const std::filesystem::path &root, audit_budget &budget)
{
	scoped_audit_budget scope(budget);
	authority_read_lock lock(root);
	lock.no_pending_player_domains();
	result output;
	std::map<account_key, account_state> accounts;
	std::map<bytes, account_key> active_locators;
	std::set<bytes> native_banks;
	std::vector<std::pair<bytes, account_key>> wallet_banks;
	auto issue = [&](const char *code, const account_key &account = {},
			 const identity &operation = {}, uint16_t kind = 0, uint64_t id = 0,
			 const identity &epoch = {}, const observed_values *observed = nullptr)
	{
		++output.finding_count;
		if (output.findings.size() < maximum_findings)
		{
			finding row{ account,	nonzero(epoch) ? epoch : output.epoch,
				     operation, code,
				     kind,	id };
			if (observed)
				row.observed = *observed;
			output.findings.push_back(row);
		}
	};
	const auto evidence = root / "economic-evidence";
	restore_economic_authority::checker authority(root);
	digest control = {};
	if (lock.locked() && std::filesystem::exists(evidence) &&
	    !std::filesystem::is_empty(evidence))
	{
		authority.begin_page();
		output.initialized = true;
		output.epoch = authority.current_epoch();
		control = authority.authority_body();
		authority.for_each_mapping(
			[&](auto encoded, const auto &row)
			{
				const auto kind = number(row.key, 0, 2);
				if (kind != 1 && kind != 2)
					return;
				if (accounts.size() == maximum_accounts)
					throw account_budget_refused();
				const auto key = fixed_account(encoded);
				need(accounts.emplace(key, account_state{ row.retired }).second);
				if (!row.retired)
				{
					++output.active_accounts;
					need(active_locators.emplace(row.key, key).second);
				}
			});
		restore_economic_money_history::checker history;
		(void)restore_economic_records::checker(
			root, [&](const auto &epoch, const auto &operation, auto plan, auto witness)
			{ history.observe(epoch, operation, plan, witness); })
			.run();
		output.history = history.finish(
			[&](const terminal &tail)
			{
				if (number(tail.account, 18, 2) != 1 &&
				    number(tail.account, 18, 2) != 2)
					return;
				if (tail.epoch != output.epoch)
				{
					++output.prior_epoch_accounts;
					return;
				}
				const auto found = accounts.find(tail.account);
				need(found != accounts.end());
				found->second.tail = tail;
				found->second.has_history = true;
				if (found->second.retired)
				{
					++output.retired_epoch_accounts;
					if (tail.balance != coins{})
						issue("retired_account_balance_nonzero",
						      tail.account, tail.operation);
				}
				if (!tail.baseline_anchored)
				{
					++output.unanchored_current_accounts;
					issue("unknown_legacy_origin", tail.account,
					      tail.operation);
				}
			});
		for (const auto &row : output.history.findings)
			issue(row.code, row.account, row.operation, 0, 0, row.epoch);
		output.finding_count +=
			output.history.invalid_accounts - output.history.findings.size();
	}
	const auto domains = root / "domains";
	std::vector<std::string> domain_names;
	if (std::filesystem::exists(domains))
		for (const auto &entry : std::filesystem::directory_iterator(domains))
		{
			audit_directory_entry();
			const auto leaf = entry.path().filename().string();
			if (leaf.starts_with("player-") || leaf.starts_with("bank-"))
				domain_names.push_back(leaf);
		}
	std::sort(domain_names.begin(), domain_names.end());
	for (const auto &leaf : domain_names)
	{
		const bool wallet = leaf.starts_with("player-"), bank = leaf.starts_with("bank-");
		if (!wallet && !bank)
			continue;
		need(lock.locked() && leaf.ends_with(".domain"));
		if (output.native_wallets + output.native_banks == maximum_accounts)
			throw account_budget_refused();
		const auto body = leaf.substr(wallet ? 7 : 5, leaf.size() - (wallet ? 14 : 12));
		bytes key;
		restore_native_domains::holding holding;
		if (wallet)
		{
			const auto pid = decimal(body, INT32_MAX);
			need(pid);
			holding = restore_native_domains::wallet(file_bytes(domains, leaf, 65536),
								 pid);
			key = native_key(1, 0, pid);
			++output.native_wallets;
		}
		else
		{
			const auto separator = body.rfind('-');
			need(separator != std::string::npos && separator > 0 &&
			     separator + 1 < body.size());
			const auto account = body.substr(0, separator);
			const auto context = decimal(body.substr(separator + 1), INT8_MAX);
			holding = restore_native_domains::bank(file_bytes(domains, leaf, 65536),
							       account, context);
			key = native_key(2, context, 0, account);
			need(native_banks.insert(key).second);
			++output.native_banks;
		}
		const auto locator = active_locators.find(key);
		const auto account = locator == active_locators.end() ? account_key{} :
									locator->second;
		if (wallet)
		{
			if (holding.racewar < 0)
				issue("wallet_bank_context_unrepresentable", account, {}, 1,
				      holding.native_id);
			else
				wallet_banks.emplace_back(native_key(2, holding.racewar, 0,
								     holding.account),
							  account);
		}
		if (locator == active_locators.end())
		{
			issue("native_domain_unmapped", {}, {}, wallet ? 1 : 2, holding.native_id);
			continue;
		}
		auto &state = accounts.at(account);
		need(!state.seen);
		state.seen = true;
		if (!state.has_history || !state.tail.valid)
			continue;
		++output.compared_accounts;
		const observed_values observed{ holding.balance, state.tail.balance,
						holding.revision, state.tail.revision };
		bool representable = true;
		wide total = 0;
		constexpr std::array<int, 4> units = { 1, 10, 100, 1000 };
		for (size_t i = 0; i < 4; ++i)
		{
			representable = representable && holding.balance[i] <= INT64_MAX;
			total += wide(holding.balance[i]) * units[i];
		}
		representable = representable && total <= INT64_MAX;
		if (!representable)
			issue("native_balance_outside_economic_range", account,
			      state.tail.operation, 0, 0, {}, &observed);
		else
		{
			bool equal = true;
			for (size_t i = 0; i < 4; ++i)
				equal = equal && static_cast<int64_t>(holding.balance[i]) ==
							 state.tail.balance[i];
			if (!equal)
				issue("native_balance_mismatch", account, state.tail.operation, 0,
				      0, {}, &observed);
		}
		if (holding.revision != state.tail.revision)
			issue("native_revision_mismatch", account, state.tail.operation, 0, 0, {},
			      &observed);
	}
	for (const auto &[bank, account] : wallet_banks)
		if (!native_banks.contains(bank))
			issue("wallet_bank_domain_missing", account);
	for (const auto &[account, state] : accounts)
		if (!state.retired)
		{
			if (!state.seen)
				issue("native_domain_missing", account);
			if (!state.has_history)
				issue("economic_history_missing", account);
		}
	if (output.initialized)
	{
		authority.begin_page();
		need(authority.authority_body() == control &&
		     authority.current_epoch() == output.epoch);
	}
	lock.no_pending_player_domains();
	lock.finish();
	audit_checkpoint();
	return output;
}
} // namespace restore_wallet_bank
#endif
