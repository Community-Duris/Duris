// Shared independent current-money checks for native holding readers.
#ifndef DURIS_QUALIFY_FLATFILE_CURRENT_MONEY_H
#define DURIS_QUALIFY_FLATFILE_CURRENT_MONEY_H

#include "qualify_flatfile_economic_money_history.h"
#include <map>
#include <ostream>

namespace restore_current_money
{
using namespace restore_economic_money_history;
constexpr size_t maximum_accounts = 32768;
struct account_budget_refused : audit_budget_refused
{
};
// Wide values preserve both unsigned native wallets and signed native auctions.
using native_coins = std::array<wide, 4>;
struct observed_values
{
	native_coins native = {};
	coins expected = {};
	uint64_t native_revision = 0, expected_revision = 0;
};
inline void native_decimal(std::ostream &out, wide value)
{
	if (value < 0)
		out << static_cast<int64_t>(value);
	else
		out << static_cast<uint64_t>(value);
}
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
	size_t compared_accounts = 0, unanchored_current_accounts = 0, finding_count = 0;
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
inline bytes native_key(uint16_t kind, uint64_t context, uint64_t id,
			const std::string &account = {})
{
	bytes key;
	put(key, kind, 2);
	put(key, context, 8);
	put(key, kind, 2);
	if (kind != 2)
		put(key, id, 8);
	else
		key.insert(key.end(), account.begin(), account.end());
	return key;
}
class ledger
{
	std::filesystem::path root;
	scoped_audit_budget scope;
	authority_read_lock lock;
	restore_economic_authority::checker authority;
	digest control = {};
	uint16_t first_kind, second_kind;
	result &output;
	std::map<account_key, account_state> accounts;
	std::map<bytes, account_key> active_locators;
	bool selected(uint16_t kind) const { return kind == first_kind || kind == second_kind; }

    public:
	void issue(const char *code, const account_key &account = {},
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
	}
	ledger(const std::filesystem::path &path, audit_budget &budget, uint16_t first,
	       uint16_t second, result &value)
		: root(path)
		, scope(budget)
		, lock(root)
		, authority(root)
		, first_kind(first)
		, second_kind(second)
		, output(value)
	{
		lock.no_pending_player_domains();
		const auto evidence = root / "economic-evidence";
		if (!lock.locked() || !std::filesystem::exists(evidence) ||
		    std::filesystem::is_empty(evidence))
			return;
		authority.begin_page();
		output.initialized = true;
		output.epoch = authority.current_epoch();
		control = authority.authority_body();
		authority.for_each_mapping(
			[&](auto encoded, const auto &row)
			{
				if (!selected(number(row.key, 0, 2)))
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
				if (!selected(number(tail.account, 18, 2)))
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
	bool locked() const { return lock.locked(); }
	account_key resolve(const bytes &key) const
	{
		const auto found = active_locators.find(key);
		return found == active_locators.end() ? account_key{} : found->second;
	}
	void holding(const bytes &key, const native_coins &balance, uint64_t revision,
		     uint16_t kind, uint64_t id)
	{
		const auto found = active_locators.find(key);
		if (found == active_locators.end())
		{
			issue("native_domain_unmapped", {}, {}, kind, id);
			return;
		}
		const auto account = found->second;
		auto &state = accounts.at(account);
		need(!state.seen);
		state.seen = true;
		if (!state.has_history || !state.tail.valid)
			return;
		++output.compared_accounts;
		const observed_values observed{ balance, state.tail.balance, revision,
						state.tail.revision };
		bool representable = true;
		wide total = 0;
		constexpr std::array<int, 4> units = { 1, 10, 100, 1000 };
		for (size_t i = 0; i < 4; ++i)
		{
			need(balance[i] >= INT64_MIN && balance[i] <= wide(UINT64_MAX));
			representable = representable && balance[i] >= 0 && balance[i] <= INT64_MAX;
			total += balance[i] * units[i];
		}
		representable = representable && total <= INT64_MAX;
		if (!representable)
			issue("native_balance_outside_economic_range", account,
			      state.tail.operation, 0, 0, {}, &observed);
		else
		{
			bool equal = true;
			for (size_t i = 0; i < 4; ++i)
				equal = equal && balance[i] == state.tail.balance[i];
			if (!equal)
				issue("native_balance_mismatch", account, state.tail.operation, 0,
				      0, {}, &observed);
		}
		if (revision != state.tail.revision)
			issue("native_revision_mismatch", account, state.tail.operation, 0, 0, {},
			      &observed);
	}
	void finish()
	{
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
	}
};
} // namespace restore_current_money
#endif
