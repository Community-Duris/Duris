// Independent current wallet/bank comparison. No native mutation or recovery.
#ifndef DURIS_QUALIFY_FLATFILE_WALLET_BANK_H
#define DURIS_QUALIFY_FLATFILE_WALLET_BANK_H

#include "qualify_flatfile_current_money.h"
#include "qualify_flatfile_native_domains.h"
#include <charconv>

namespace restore_wallet_bank
{
using namespace restore_current_money;
struct result : restore_current_money::result
{
	size_t native_wallets = 0, native_banks = 0;
};
inline uint64_t decimal(const std::string &value, uint64_t maximum)
{
	uint64_t result = 0;
	auto parsed = std::from_chars(value.data(), value.data() + value.size(), result);
	need(!value.empty() && parsed.ec == std::errc{} &&
	     parsed.ptr == value.data() + value.size() && result <= maximum &&
	     std::to_string(result) == value);
	return result;
}
inline result audit(const std::filesystem::path &root, audit_budget &budget)
{
	result output;
	ledger current(root, budget, 1, 2, output);
	std::set<bytes> native_banks;
	std::vector<std::pair<bytes, account_key>> wallet_banks;
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
		need(current.locked() && leaf.ends_with(".domain"));
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
		const auto account = current.resolve(key);
		if (wallet)
		{
			if (holding.racewar < 0)
				current.issue("wallet_bank_context_unrepresentable", account, {}, 1,
					      holding.native_id);
			else
				wallet_banks.emplace_back(native_key(2, holding.racewar, 0,
								     holding.account),
							  account);
		}
		native_coins balance = {};
		std::copy(holding.balance.begin(), holding.balance.end(), balance.begin());
		current.holding(key, balance, holding.revision, wallet ? 1 : 2, holding.native_id);
	}

	for (const auto &[bank, account] : wallet_banks)
		if (!native_banks.contains(bank))
			current.issue("wallet_bank_domain_missing", account);
	current.finish();
	return output;
}
} // namespace restore_wallet_bank
#endif
