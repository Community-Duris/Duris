#include "persistence/ram_command.h"

#include "account/session_audit_command.h"
#include "economy/coin_transfer_command.h"
#include "economy/currency_command.h"
#include "item/item_ownership_runtime.h"
#include "world/epic_command.h"
#include "core/prototypes.h"
#include "core/utils.h"

#include <algorithm>
#include <cerrno>
#include <cstring>
#include <limits>

extern P_char character_list;

namespace
{
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
	P_char character = player(payload.pid);
	if (!character || !character->only.pc)
		return ENOENT;
	const char *account = get_account_name_safe(character);
	if (!account || strcasecmp(account, payload.account_name.data()) ||
	    GET_RACEWAR(character) != payload.racewar)
		return ESTALE;
	const currency_command_result current{
		{ { GET_COPPER(character), GET_SILVER(character), GET_GOLD(character),
		    GET_PLATINUM(character) } },
		{ { GET_BALANCE_COPPER(character), GET_BALANCE_SILVER(character),
		    GET_BALANCE_GOLD(character), GET_BALANCE_PLATINUM(character) } },
		character->only.pc->wallet_revision,
		character->only.pc->bank_revision
	};
	return currency_command_prepare_outcome(command, current, result);
}

unsigned int item(const critical_command &command, item_transfer_result *result)
{
	item_transfer_payload payload{};
	if (!item_transfer_command_decode_payload(command, &payload))
		return EINVAL;
	return item_ownership_runtime_prepare(payload, result);
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
			error = item(endpoint.change, &result.piles[index]);
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

critical_apply_result ram_command_prepare(const critical_command &command, void *)
{
	if (!critical_command_valid(command) ||
	    command.schema_version != CRITICAL_COMMAND_SCHEMA_VERSION)
		return rejected(EINVAL);
	switch (command.type)
	{
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
					  result.max_item_revision }));
	}
	case critical_command_type::coin_transfer:
		return coin(command);
	case critical_command_type::epic:
	{
		epic_command_payload payload{};
		if (!epic_command_decode_payload(command, &payload))
			return rejected(EINVAL);
		P_char character = player(payload.pid);
		if (!character || !character->only.pc)
			return rejected(ENOENT);
		const int64_t balance = character->only.pc->epics;
		const uint64_t revision = character->only.pc->epic_revision;
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
