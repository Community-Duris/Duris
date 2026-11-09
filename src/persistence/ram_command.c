#include "persistence/ram_command.h"
#include "persistence/corpse_ram.h"
#include "player/player_restitution_ram.h"

#include "account/session_audit_command.h"
#include "economy/coin_transfer_command.h"
#include "economy/currency_command.h"
#include "economy/economic_state.h"
#include "economy/auction_ram.h"
#include "economy/boon_ram.h"
#include "economy/collector_ram.h"
#include "economy/shop_trade_runtime.h"
#include "persistence/operation_log.h"
#include "item/item_ownership_runtime.h"
#include "world/epic_command.h"
#include "guild/artifact_guild_state.h"
#include "world/zone_touch_command.h"
#include "combat/combat_outcome_command.h"
#include <openssl/sha.h>
#include <map>
#include "core/prototypes.h"
#include "core/utils.h"

#include <algorithm>
#include <cerrno>
#include <cstring>
#include <limits>

extern P_char character_list;

namespace
{
critical_operation_id prepared_id{};
critical_apply_result prepared_receipt{};
bool has_prepared = false;
uint64_t next_combat_event = 1;
bool flatfile_events = false;
std::map<uint64_t, zone_touch_result> stone_claims;
P_char player(uint32_t pid)
{
	if (P_char found = find_player_by_pid(static_cast<int>(pid)))
		return found;
	for (P_char body = character_list; body; body = body->next)
	{
		P_char found = GET_PLYR(body);
		if (found && IS_PC(found) && found->only.pc &&
		    GET_PID(found) == static_cast<int>(pid))
			return found;
	}
	return nullptr;
}

bool ensure_player(uint32_t pid)
{
	economic_progress_state existing;
	if (economic_state_progress(pid, &existing))
		return true;
	P_char ch = player(pid);
	if (!ch || !ch->only.pc)
		return false;
	const char *account = get_account_name_safe(ch);
	if (!account || !strcmp(account, "Unknown"))
		return false;
	return economic_state_seed(
		{ pid,
		  account,
		  GET_NAME(ch),
		  static_cast<uint8_t>(GET_RACEWAR(ch)),
		  { { { GET_COPPER(ch), GET_SILVER(ch), GET_GOLD(ch), GET_PLATINUM(ch) } },
		    { { GET_BALANCE_COPPER(ch), GET_BALANCE_SILVER(ch), GET_BALANCE_GOLD(ch),
			GET_BALANCE_PLATINUM(ch) } },
		    ch->only.pc->wallet_revision,
		    ch->only.pc->bank_revision },
		  { ch->only.pc->epics, ch->only.pc->epic_revision, ch->only.pc->frags,
		    ch->only.pc->oldfrags, ch->only.pc->frag_revision } });
}

template <size_t N>
critical_apply_result outcome(const std::array<uint8_t, N> &bytes, uint64_t revision)
{
	static_assert(N <= CRITICAL_COMPLETION_RESULT_MAX_BYTES);
	critical_apply_result result{ critical_apply_outcome::applied, revision, 0 };
	result.result_size = N;
	std::copy(bytes.begin(), bytes.end(), result.result_payload.begin());
	return result;
}

critical_apply_result rejected(unsigned int error)
{
	return { critical_apply_outcome::terminal_failure, 0, error };
}

unsigned int currency(const critical_command &command, currency_command_result *result)
{
	currency_command_payload payload{};
	if (!currency_command_decode_payload(command, &payload))
		return EINVAL;
	currency_command_result current{};
	if (!ensure_player(payload.pid) ||
	    !economic_state_balances(payload.pid, payload.account_name.data(), payload.racewar,
				     &current))
		return ENOENT;
	return currency_command_prepare_outcome(command, current, result);
}

unsigned int item(const critical_command &command, item_transfer_result *result)
{
	item_transfer_payload payload{};
	if (!item_transfer_command_decode_payload(command, &payload))
		return EINVAL;
	if (auto error = item_ownership_runtime_prepare(payload, result))
		return error;
	if (auto error = corpse_ram_prepare_item(payload, result))
		return error;
	return collector_ram_prepare_item(command, payload, result);
}

critical_apply_result coin(const critical_command &command)
{
	coin_transfer_payload payload{};
	if (!coin_transfer_command_decode_payload(command, &payload))
		return rejected(EINVAL);
	coin_transfer_result result{};
	const coin_transfer_endpoint *endpoints[]{ &payload.source, &payload.destination };
	for (size_t index = 0; index < 2; ++index)
	{
		const auto &endpoint = *endpoints[index];
		unsigned int error = 0;
		if (endpoint.change.type == critical_command_type::account_bank)
		{
			error = currency(endpoint.change, &result.wallets[index]);
			if (!error && !std::equal(endpoint.after.begin(), endpoint.after.end(),
						  result.wallets[index].wallet.amount.begin()))
				error = ESTALE;
		}
		else
		{
			item_transfer_payload transfer;
			error = item_transfer_command_decode_payload(endpoint.change, &transfer) ?
					item_ownership_runtime_prepare(transfer,
								       &result.piles[index]) :
					EINVAL;
		}
		if (error)
			return rejected(error);
	}
	// Both endpoints were validated against the same live state. Advance any
	// shared owner clock twice, matching the atomic materializer's internal order.
	if (payload.source.change.type == payload.destination.change.type)
	{
		if (payload.source.change.type == critical_command_type::account_bank)
		{
			if (critical_entity_key_equal(payload.source.change.keys[1],
						      payload.destination.change.keys[1]))
			{
				if (result.wallets[1].bank_revision == UINT64_MAX)
					return rejected(ERANGE);
				++result.wallets[1].bank_revision;
			}
		}
		else
		{
			item_transfer_payload source{}, destination{};
			item_transfer_command_decode_payload(payload.source.change, &source);
			item_transfer_command_decode_payload(payload.destination.change,
							     &destination);
			auto advance = [&](const item_owner_identity &owner, uint64_t *revision)
			{
				if (item_owner_identity_equal(owner, source.from_owner) ||
				    item_owner_identity_equal(owner, source.to_owner))
				{
					if (*revision == UINT64_MAX)
						return false;
					++*revision;
				}
				return true;
			};
			if (!advance(destination.from_owner,
				     &result.piles[1].from_owner_revision) ||
			    !advance(destination.to_owner, &result.piles[1].to_owner_revision))
				return rejected(ERANGE);
		}
	}
	if (payload.source.change.type == critical_command_type::item_transfer ||
	    payload.destination.change.type == critical_command_type::item_transfer)
		corpse_ram_note_world_change();
	std::array<uint8_t, COIN_TRANSFER_RESULT_BYTES> bytes{};
	if (!coin_transfer_command_encode_result(payload, result, &bytes))
		return rejected(EINVAL);
	uint64_t revision = 0;
	for (const auto &wallet : result.wallets)
		revision = std::max({ revision, wallet.wallet_revision, wallet.bank_revision });
	for (const auto &pile : result.piles)
		revision = std::max({ revision, pile.from_owner_revision, pile.to_owner_revision,
				      pile.max_item_revision });
	return outcome(bytes, revision);
}
} // namespace

static critical_apply_result prepare_command(const critical_command &command)
{
	if (!critical_command_valid(command) ||
	    command.schema_version != CRITICAL_COMMAND_SCHEMA_VERSION)
		return rejected(EINVAL);
	// These adapters have never had flat-file projectors. Reject before append;
	// a backend limitation must never strand an acknowledged sealed operation.
	if (flatfile_events && (command.type == critical_command_type::zone ||
				command.type == critical_command_type::artifact ||
				command.type == critical_command_type::session_audit ||
				command.type == critical_command_type::player_death_restitution))
		return rejected(ENOTSUP);
	switch (command.type)
	{
	case critical_command_type::player_death_restitution:
		return player_restitution_ram_prepare(command);
	case critical_command_type::corpse_lifecycle:
		return corpse_ram_prepare(command);
	case critical_command_type::collector:
		return collector_ram_prepare(command);
	case critical_command_type::shop_trade:
	{
		shop_trade_payload payload;
		shop_trade_result result;
		if (!shop_trade_command_decode_payload(command, &payload))
			return rejected(EINVAL);
		if (auto error = shop_trade_runtime_prepare(payload, &result))
			return rejected(error);
		item_transfer_payload transfer;
		item_transfer_result moved{};
		if (!shop_trade_item_payload(payload, &transfer))
			return rejected(EINVAL);
		if (auto error = collector_ram_prepare_item(command, transfer, &moved))
			return rejected(error);
		std::array<uint8_t, SHOP_TRADE_RESULT_BYTES> bytes;
		if (!shop_trade_command_encode_result(result, &bytes))
			return rejected(EINVAL);
		uint64_t revision = std::max({ result.shop_revision, result.wallet_revision,
					       result.bank_revision, result.player_owner_revision,
					       result.counterparty_owner_revision });
		for (size_t i = 0; i < result.item_count; ++i)
			revision = std::max(revision, result.item_revisions[i]);
		return outcome(bytes, revision);
	}
	case critical_command_type::boon_reward:
		return boon_ram_prepare(command);
	case critical_command_type::boon_shop:
	{
		boon_shop_payload payload{};
		if (!boon_shop_command_decode_payload(command, &payload))
			return rejected(EINVAL);
		P_char ch = player(payload.pid);
		if (!ch)
			return rejected(ENOENT);
		return boon_ram_prepare(command, ch->base_stats[payload.stat_index]);
	}
	case critical_command_type::auction:
		return auction_ram_prepare_command(command);
	case critical_command_type::artifact:
	{
		artifact_guild_payload payload{};
		artifact_guild_result result{};
		if (!artifact_guild_command_decode_payload(command, &payload))
			return rejected(EINVAL);
		if (auto error = artifact_guild_state_prepare(payload, &result))
			return rejected(error);
		std::array<uint8_t, ARTIFACT_GUILD_RESULT_BYTES> bytes{};
		if (!artifact_guild_command_encode_result(result, &bytes))
			return rejected(EINVAL);
		uint64_t revision = result.guild_revision;
		for (size_t i = 0; i < result.artifact_count; ++i)
			revision = std::max(revision, result.artifacts[i].revision);
		return outcome(bytes, revision);
	}
	case critical_command_type::zone:
	{
		zone_touch_payload payload{};
		if (!zone_touch_command_decode_payload(command, &payload))
			return rejected(EINVAL);
		zone_touch_result result(payload);
		const auto claim = stone_claims.find(payload.stone_uid);
		if (payload.stone_uid && claim != stone_claims.end() && claim->second.stone_uid)
		{
			result = claim->second;
			result.recovered_claim = true;
		}
		else if (payload.stone_uid)
			for (size_t i = 0; i < payload.group_size; ++i)
			{
				economic_progress_state progress;
				if (!ensure_player(payload.participant_pids[i]) ||
				    !economic_state_progress(payload.participant_pids[i],
							     &progress))
					return rejected(ENOENT);
				const auto amount = payload.awards[i].amount;
				if (progress.epic_revision == UINT64_MAX || amount < 0 ||
				    progress.epics > INT64_MAX - amount)
					return rejected(ERANGE);
				result.balances[i] = progress.epics + amount;
				result.revisions[i] = progress.epic_revision + 1;
			}
		std::array<uint8_t, ZONE_TOUCH_RESULT_BYTES> bytes{};
		if (!zone_touch_command_encode_result(result, &bytes))
			return rejected(EINVAL);
		return outcome(bytes, 0);
	}
	case critical_command_type::combat_outcome:
	{
		combat_outcome_payload payload{};
		combat_outcome_result result{};
		if (!combat_outcome_command_decode_payload(command, &payload))
			return rejected(EINVAL);
		result.participant_count = payload.participant_count;
		if (flatfile_events)
		{
			std::array<uint8_t, SHA256_DIGEST_LENGTH> digest{};
			SHA256(command.operation_id.bytes.data(), command.operation_id.bytes.size(),
			       digest.data());
			for (size_t i = 0; i < 8; ++i)
				result.event_id |= static_cast<uint64_t>(digest[i]) << (8 * i);
			if (!result.event_id)
				result.event_id = 1;
		}
		else
		{
			if (!next_combat_event || next_combat_event > UINT32_MAX)
				return rejected(ERANGE);
			result.event_id = next_combat_event;
		}
		uint64_t revision = 0;
		for (size_t i = 0; i < payload.participant_count; ++i)
		{
			const auto &entry = payload.participants[i];
			economic_player_state current;
			if (!ensure_player(entry.pid) || !economic_state_find(entry.pid, &current))
				return rejected(ENOENT);
			const auto &progress = current.progress;
			const auto &wallet = current.balances;
			if (entry.expected_frag_revision != progress.frag_revision ||
			    entry.expected_epic_revision != progress.epic_revision ||
			    entry.expected_wallet_revision != wallet.wallet_revision)
				return rejected(ESTALE);
			if (progress.frag_revision == UINT64_MAX ||
			    progress.epic_revision == UINT64_MAX ||
			    wallet.wallet_revision == UINT64_MAX ||
			    (entry.frag_delta > 0 &&
			     progress.frags > INT64_MAX - entry.frag_delta) ||
			    (entry.frag_delta < 0 &&
			     progress.frags < INT64_MIN - entry.frag_delta) ||
			    entry.epic_delta < 0 || progress.epics > INT64_MAX - entry.epic_delta)
				return rejected(ERANGE);
			auto &next = result.participants[i];
			next = { entry.pid,
				 progress.frags + entry.frag_delta,
				 progress.epics + entry.epic_delta,
				 -1,
				 {},
				 progress.frag_revision + (entry.frag_delta != 0),
				 progress.epic_revision + (entry.epic_delta != 0),
				 wallet.wallet_revision,
				 entry.expected_bank_revision };
			if (entry.wallet_delta_copper)
			{
				if (entry.racewar != current.racewar ||
				    strcasecmp(current.account.c_str(),
					       entry.account_name.data()) ||
				    wallet.bank_revision != entry.expected_bank_revision)
					return rejected(ESTALE);
				if (entry.wallet_delta_copper < 0)
					return rejected(ERANGE);
				int64_t value = wallet.wallet.amount[0] +
						10 * wallet.wallet.amount[1] +
						100 * wallet.wallet.amount[2] +
						1000 * wallet.wallet.amount[3];
				if (value > INT64_MAX - entry.wallet_delta_copper)
					return rejected(ERANGE);
				next.wallet_value = value + entry.wallet_delta_copper;
				if (next.wallet_value / 1000 > INT_MAX)
					return rejected(ERANGE);
				next.bank = wallet.bank;
				++next.wallet_revision;
				size_t shared = 0;
				for (size_t j = 0; j < payload.participant_count; ++j)
					if (payload.participants[j].wallet_delta_copper &&
					    payload.participants[j].racewar == entry.racewar &&
					    !strcasecmp(payload.participants[j].account_name.data(),
							entry.account_name.data()))
						++shared;
				if (wallet.bank_revision > UINT64_MAX - shared)
					return rejected(ERANGE);
				next.bank_revision = wallet.bank_revision + shared;
			}
			revision = std::max({ revision, next.frag_revision, next.epic_revision,
					      next.wallet_revision, next.bank_revision });
		}
		std::array<uint8_t, COMBAT_OUTCOME_RESULT_BYTES> bytes{};
		if (!combat_outcome_command_encode_result(result, &bytes))
			return rejected(EINVAL);
		return outcome(bytes, revision);
	}
	case critical_command_type::account_bank:
	{
		currency_command_result result{};
		if (const auto error = currency(command, &result))
			return rejected(error);
		std::array<uint8_t, CURRENCY_RESULT_PAYLOAD_BYTES> bytes{};
		if (!currency_command_encode_result(result, &bytes))
			return rejected(EINVAL);
		return outcome(bytes, std::max(result.wallet_revision, result.bank_revision));
	}
	case critical_command_type::item_transfer:
	{
		item_transfer_result result{};
		if (const auto error = item(command, &result))
			return rejected(error);
		std::array<uint8_t, ITEM_TRANSFER_RESULT_BYTES> bytes{};
		if (!item_transfer_command_encode_result(result, &bytes))
			return rejected(EINVAL);
		return outcome(bytes,
			       std::max({ result.from_owner_revision, result.to_owner_revision,
					  result.max_item_revision, result.corpse_revision,
					  !flatfile_events && result.collector_catalog_changed ?
						  collector_ram_revision() :
						  0 }));
	}
	case critical_command_type::coin_transfer:
		return coin(command);
	case critical_command_type::epic:
	{
		epic_command_payload payload{};
		if (!epic_command_decode_payload(command, &payload))
			return rejected(EINVAL);
		economic_progress_state progress;
		if (!ensure_player(payload.pid) || !economic_state_progress(payload.pid, &progress))
			return rejected(ENOENT);
		const int64_t balance = progress.epics;
		const uint64_t revision = progress.epic_revision;
		if (command.expected_revisions[0].revision != UINT64_MAX &&
		    command.expected_revisions[0].revision != revision)
			return rejected(ESTALE);
		if (payload.delta < 0 && (payload.flags & EPIC_COMMAND_REQUIRE_FUNDS) &&
		    balance < -payload.delta)
			return rejected(ENOSPC);
		if (revision == UINT64_MAX ||
		    (payload.delta > 0 && balance > INT64_MAX - payload.delta) ||
		    (payload.delta < 0 && balance < INT64_MIN - payload.delta))
			return rejected(ERANGE);
		const epic_command_result result{ balance + payload.delta, revision + 1,
						  payload.delta };
		std::array<uint8_t, EPIC_RESULT_PAYLOAD_BYTES> bytes{};
		if (!epic_command_encode_result(result, &bytes))
			return rejected(EINVAL);
		return outcome(bytes, result.revision);
	}
	case critical_command_type::session_audit:
	{
		session_audit_payload payload{};
		std::array<uint8_t, SESSION_AUDIT_RESULT_BYTES> bytes{};
		if (!session_audit_command_decode_payload(command, &payload) ||
		    !session_audit_command_encode_result(payload, &bytes))
			return rejected(EINVAL);
		return outcome(bytes, 0);
	}
	default:
		return rejected(ENOTSUP);
	}
}

critical_apply_result ram_command_prepare(const critical_command &command, void *)
{
	has_prepared = false;
	collector_ram_begin();
	corpse_ram_begin();
	auto result = prepare_command(command);
	if (result.outcome == critical_apply_outcome::applied &&
	    !collector_ram_seal(command, result))
		return rejected(ENOMEM);
	prepared_id = command.operation_id;
	prepared_receipt = result;
	has_prepared = result.outcome == critical_apply_outcome::applied;
	return result;
}

critical_submit_result ram_command_submit(const critical_command &command)
{
	if (command.schema_version != CRITICAL_COMMAND_OUTCOME_SCHEMA_VERSION || !has_prepared ||
	    !critical_operation_id_equal(prepared_id, command.operation_id) ||
	    !critical_command_outcome_matches(
		    command, prepared_receipt.durable_revision, prepared_receipt.error_code,
		    prepared_receipt.result_payload.data(), prepared_receipt.result_size))
		return critical_submit_result::invalid;
	if (command.type == critical_command_type::auction && !auction_ram_can_publish(command))
		return critical_submit_result::invalid;
	if (command.type == critical_command_type::player_death_restitution &&
	    !player_restitution_ram_can_publish(command))
		return critical_submit_result::invalid;
	// Decode and preflight before append: accepted publication must not allocate.
	std::array<currency_command_payload, COMBAT_OUTCOME_MAX_PARTICIPANTS> wallets{};
	std::array<currency_command_result, COMBAT_OUTCOME_MAX_PARTICIPANTS> results{};
	std::array<std::pair<uint32_t, economic_progress_state>, COMBAT_OUTCOME_MAX_PARTICIPANTS>
		progress{};
	size_t progress_count = 0;
	zone_touch_result zone{};
	zone_touch_result *stone = nullptr;
	artifact_guild_result artifact{};
	size_t count = 0;
	if (command.type == critical_command_type::account_bank)
	{
		if (!currency_command_decode_payload(command, &wallets[0]) ||
		    !currency_command_decode_result(command.outcome_payload.data(),
						    command.outcome_payload.size(), &results[0]))
			return critical_submit_result::invalid;
		count = 1;
	}
	else if (command.type == critical_command_type::corpse_lifecycle)
	{
		corpse_lifecycle_payload payload;
		corpse_lifecycle_result result;
		if (!corpse_lifecycle_command_decode_payload(command, &payload) ||
		    !corpse_lifecycle_command_decode_result(command.outcome_payload.data(),
							    command.outcome_payload.size(),
							    &result))
			return critical_submit_result::invalid;
		if (result.wallet_revision)
		{
			economic_player_state player;
			if (!economic_state_find(payload.destination_player_pid, &player))
				return critical_submit_result::invalid;
			wallets[0].pid = player.pid;
			std::copy(player.account.begin(), player.account.end(),
				  wallets[0].account_name.begin());
			wallets[0].racewar = player.racewar;
			results[0] = player.balances;
			std::copy(result.wallet.begin(), result.wallet.end(),
				  results[0].wallet.amount.begin());
			results[0].wallet_revision = result.wallet_revision;
			if (result.bank_revision)
				results[0].bank_revision = result.bank_revision;
			count = 1;
		}
	}
	else if (command.type == critical_command_type::collector)
	{
		collector_command_payload payload;
		collector_command_result result;
		if (!collector_command_decode_payload(command, &payload) ||
		    !collector_command_decode_result(command.outcome_payload.data(),
						     command.outcome_payload.size(), &result))
			return critical_submit_result::invalid;
		if (payload.action == collector_action::purchase)
		{
			wallets[0].pid = payload.actor_pid;
			wallets[0].account_name = payload.account_name;
			wallets[0].racewar = payload.racewar;
			results[0] = { result.wallet, result.bank, result.wallet_revision,
				       result.bank_revision };
			count = 1;
		}
	}
	else if (command.type == critical_command_type::shop_trade)
	{
		shop_trade_payload payload;
		shop_trade_result result;
		if (!shop_trade_command_decode_payload(command, &payload) ||
		    !shop_trade_command_decode_result(command.outcome_payload.data(),
						      command.outcome_payload.size(), &result))
			return critical_submit_result::invalid;
		wallets[0].pid = payload.player_pid;
		wallets[0].account_name = payload.account_name;
		wallets[0].racewar = payload.racewar;
		results[0] = { result.wallet, result.bank, result.wallet_revision,
			       result.bank_revision };
		count = 1;
	}
	else if (command.type == critical_command_type::coin_transfer)
	{
		coin_transfer_payload payload{};
		coin_transfer_result result{};
		if (!coin_transfer_command_decode_payload(command, &payload) ||
		    !coin_transfer_command_decode_result(payload, command.outcome_payload.data(),
							 command.outcome_payload.size(), &result))
			return critical_submit_result::invalid;
		const coin_transfer_endpoint *ends[]{ &payload.source, &payload.destination };
		for (size_t i = 0; i < 2; ++i)
			if (ends[i]->change.type == critical_command_type::account_bank)
			{
				if (!currency_command_decode_payload(ends[i]->change,
								     &wallets[count]))
					return critical_submit_result::invalid;
				results[count++] = result.wallets[i];
			}
	}
	else if (command.type == critical_command_type::epic)
	{
		epic_command_payload payload{};
		epic_command_result result{};
		if (!epic_command_decode_payload(command, &payload) ||
		    !epic_command_decode_result(command.outcome_payload.data(),
						command.outcome_payload.size(), &result) ||
		    !economic_state_progress(payload.pid, &progress[0].second))
			return critical_submit_result::invalid;
		progress[0].first = payload.pid;
		progress[0].second.epics = result.balance;
		progress[0].second.epic_revision = result.revision;
		progress_count = 1;
	}
	else if (command.type == critical_command_type::combat_outcome)
	{
		combat_outcome_payload payload{};
		combat_outcome_result result{};
		if (!combat_outcome_command_decode_payload(command, &payload) ||
		    !combat_outcome_command_decode_result(command.outcome_payload.data(),
							  command.outcome_payload.size(), &result))
			return critical_submit_result::invalid;
		for (size_t i = 0; i < result.participant_count; ++i)
		{
			const auto &entry = result.participants[i];
			auto &next = progress[progress_count++];
			next.first = entry.pid;
			if (!economic_state_progress(entry.pid, &next.second))
				return critical_submit_result::invalid;
			if (entry.frag_revision > next.second.frag_revision)
				next.second.old_frags = next.second.frags;
			next.second.frags = entry.frags;
			next.second.frag_revision = entry.frag_revision;
			next.second.epics = entry.epics;
			next.second.epic_revision = entry.epic_revision;
			if (entry.wallet_value >= 0)
			{
				wallets[count].pid = entry.pid;
				wallets[count].account_name = payload.participants[i].account_name;
				wallets[count].racewar = payload.participants[i].racewar;
				const auto value = entry.wallet_value;
				results[count] = { { { value % 10, (value / 10) % 10,
						       (value / 100) % 10, value / 1000 } },
						   entry.bank,
						   entry.wallet_revision,
						   entry.bank_revision };
				++count;
			}
		}
	}
	else if (command.type == critical_command_type::zone)
	{
		if (!zone_touch_command_decode_result(command.outcome_payload.data(),
						      command.outcome_payload.size(), &zone))
			return critical_submit_result::invalid;
		if (zone.stone_uid && !zone.recovered_claim)
		{
			stone = &stone_claims.try_emplace(zone.stone_uid).first->second;
			for (size_t i = 0; i < zone.group_size; ++i)
			{
				auto &next = progress[progress_count++];
				next.first = zone.participant_pids[i];
				if (!economic_state_progress(next.first, &next.second))
					return critical_submit_result::invalid;
				next.second.epics = zone.balances[i];
				next.second.epic_revision = zone.revisions[i];
			}
		}
	}
	else if (command.type == critical_command_type::artifact &&
		 !artifact_guild_command_decode_result(command.outcome_payload.data(),
						       command.outcome_payload.size(), &artifact))
		return critical_submit_result::invalid;
	for (size_t i = 0; i < count; ++i)
		if (!economic_state_can_publish(wallets[i].pid, wallets[i].account_name.data(),
						wallets[i].racewar, results[i]))
			return critical_submit_result::unavailable;
	if (!collector_ram_can_publish(command))
		return critical_submit_result::invalid;
	for (size_t i = 0; i < progress_count; ++i)
		if (!economic_state_can_publish_progress(progress[i].first, progress[i].second))
			return critical_submit_result::invalid;
	const auto submitted = operation_log_submit(command);
	if (submitted != critical_submit_result::accepted)
		return submitted;
	has_prepared = false;
	bool published = collector_ram_publish(command);
	corpse_ram_publish();
	for (size_t i = 0; i < count; ++i)
		published = economic_state_publish(wallets[i].pid, wallets[i].account_name.data(),
						   wallets[i].racewar, results[i]) &&
			    published;
	if (command.type == critical_command_type::boon_reward ||
	    command.type == critical_command_type::boon_shop)
		published = boon_ram_publish(command) && published;
	if (command.type == critical_command_type::auction)
		published = auction_ram_publish(command) && published;
	if (command.type == critical_command_type::player_death_restitution)
		published = player_restitution_ram_publish(command) && published;
	for (size_t i = 0; i < progress_count; ++i)
		published =
			economic_state_publish_progress(progress[i].first, progress[i].second) &&
			published;
	if (stone)
		*stone = zone;
	if (command.type == critical_command_type::artifact)
		artifact_guild_state_publish(artifact);
	if (command.type == critical_command_type::combat_outcome && !flatfile_events)
		++next_combat_event;
	if (!published)
	{
		critical_command_coordinator_quiesce();
		return critical_submit_result::journal_uncertain;
	}
	return submitted;
}

void ram_command_set_combat_sequence(uint64_t next, bool flatfile)
{
	next_combat_event = next;
	flatfile_events = flatfile;
	stone_claims.clear();
}

bool ram_command_seed_stone(const zone_touch_result &result)
{
	return result.stone_uid && stone_claims.emplace(result.stone_uid, result).second;
}
