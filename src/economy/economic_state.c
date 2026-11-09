#include "economy/economic_state.h"

#include <climits>
#include <map>
#include <strings.h>
#include <utility>

namespace
{
struct bank_state
{
	currency_vector balance = {};
	uint64_t revision = 0;
};
using bank_key = std::pair<std::string, uint8_t>;
struct player_state
{
	economic_player_state player;
	bank_state *bank;
};
std::map<uint32_t, player_state> players;
std::map<bank_key, bank_state> banks;
bool active = false;

bool valid(const currency_vector &value)
{
	for (const auto amount : value.amount)
		if (amount < 0 || amount > INT_MAX)
			return false;
	return true;
}

bool compatible(const currency_vector &current, uint64_t revision, const currency_vector &incoming,
		uint64_t next)
{
	return valid(incoming) && (next != revision || current.amount == incoming.amount);
}

player_state *find(uint32_t pid, const char *account, uint8_t racewar)
{
	const auto it = players.find(pid);
	if (it == players.end() || !account ||
	    strcasecmp(account, it->second.player.account.c_str()) ||
	    it->second.player.racewar != racewar)
		return nullptr;
	return &it->second;
}
} // namespace

bool economic_state_seed(const economic_player_state &player)
try
{
	if (!player.pid || player.account.empty() || player.name.empty() ||
	    !valid(player.balances.wallet) || !valid(player.balances.bank))
		return false;
	if (players.contains(player.pid))
		return economic_state_publish(player.pid, player.account.c_str(), player.racewar,
					      player.balances);
	std::string account = player.account;
	for (auto &c : account)
		if (c >= 'A' && c <= 'Z')
			c += 'a' - 'A';
	auto [bank, inserted] = banks.try_emplace(bank_key{ account, player.racewar },
						  bank_state{ player.balances.bank,
							      player.balances.bank_revision });
	if (!compatible(bank->second.balance, bank->second.revision, player.balances.bank,
			player.balances.bank_revision))
		return false;
	players.emplace(player.pid, player_state{ player, &bank->second });
	if (player.balances.bank_revision > bank->second.revision)
		bank->second = { player.balances.bank, player.balances.bank_revision };
	return true;
}
catch (...)
{
	return false;
}

bool economic_state_find(uint32_t pid, economic_player_state *player)
{
	const auto found = players.find(pid);
	if (!player || found == players.end())
		return false;
	*player = found->second.player;
	player->balances.bank = found->second.bank->balance;
	player->balances.bank_revision = found->second.bank->revision;
	return true;
}

bool economic_state_balances(uint32_t pid, const char *account, uint8_t racewar,
			     currency_command_result *balances)
{
	const auto *state = find(pid, account, racewar);
	if (!state || !balances)
		return false;
	*balances = state->player.balances;
	balances->bank = state->bank->balance;
	balances->bank_revision = state->bank->revision;
	return true;
}

bool economic_state_can_publish(uint32_t pid, const char *account, uint8_t racewar,
				const currency_command_result &balances)
{
	const auto *state = find(pid, account, racewar);
	return state &&
	       compatible(state->player.balances.wallet, state->player.balances.wallet_revision,
			  balances.wallet, balances.wallet_revision) &&
	       compatible(state->bank->balance, state->bank->revision, balances.bank,
			  balances.bank_revision);
}

bool economic_state_publish(uint32_t pid, const char *account, uint8_t racewar,
			    const currency_command_result &balances)
{
	if (!economic_state_can_publish(pid, account, racewar, balances))
		return false;
	auto *state = find(pid, account, racewar);
	if (balances.wallet_revision > state->player.balances.wallet_revision)
	{
		state->player.balances.wallet = balances.wallet;
		state->player.balances.wallet_revision = balances.wallet_revision;
	}
	if (balances.bank_revision > state->bank->revision)
		*state->bank = { balances.bank, balances.bank_revision };
	return true;
}

void economic_state_reset()
{
	active = false;
	players.clear();
	banks.clear();
}

void economic_state_forget(uint32_t pid)
{
	const auto found = players.find(pid);
	if (found == players.end())
		return;
	const auto bank = found->second.bank;
	players.erase(found);
	for (const auto &[other_pid, player] : players)
		if (player.bank == bank)
			return;
	for (auto entry = banks.begin(); entry != banks.end(); ++entry)
		if (&entry->second == bank)
		{
			banks.erase(entry);
			return;
		}
}

bool economic_state_progress(uint32_t pid, economic_progress_state *progress)
{
	const auto found = players.find(pid);
	if (!progress || found == players.end())
		return false;
	*progress = found->second.player.progress;
	return true;
}

bool economic_state_can_publish_progress(uint32_t pid, const economic_progress_state &progress)
{
	const auto found = players.find(pid);
	if (found == players.end())
		return false;
	auto &current = found->second.player.progress;
	if ((progress.epic_revision == current.epic_revision && progress.epics != current.epics) ||
	    (progress.frag_revision == current.frag_revision &&
	     (progress.frags != current.frags || progress.old_frags != current.old_frags)))
		return false;
	return true;
}

bool economic_state_publish_progress(uint32_t pid, const economic_progress_state &progress)
{
	if (!economic_state_can_publish_progress(pid, progress))
		return false;
	auto &current = players.find(pid)->second.player.progress;
	if (progress.epic_revision > current.epic_revision)
	{
		current.epics = progress.epics;
		current.epic_revision = progress.epic_revision;
	}
	if (progress.frag_revision > current.frag_revision)
	{
		current.frags = progress.frags;
		current.old_frags = progress.old_frags;
		current.frag_revision = progress.frag_revision;
	}
	return true;
}

void economic_state_activate()
{
	active = true;
}
bool economic_state_active()
{
	return active;
}
