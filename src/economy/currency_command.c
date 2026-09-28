#include "economy/currency_command.h"

#include <openssl/sha.h>

#include <algorithm>
#include <array>
#include <cctype>
#include <cerrno>
#include <climits>
#include <cstring>
#include <limits>
#include <string>
#include <utility>

namespace
{
constexpr size_t PID_OFFSET = 0;
constexpr size_t RACEWAR_OFFSET = 4;
constexpr size_t REASON_OFFSET = 6;
constexpr size_t REASON_ID_OFFSET = 8;
constexpr size_t NAME_LENGTH_OFFSET = 16;
constexpr size_t NAME_OFFSET = 17;
constexpr size_t WALLET_OFFSET = 68;
constexpr size_t BANK_OFFSET = 100;

void put_u16(uint8_t *output, uint16_t value)
{
	output[0] = static_cast<uint8_t>(value);
	output[1] = static_cast<uint8_t>(value >> 8);
}

void put_u32(uint8_t *output, uint32_t value)
{
	for (unsigned int byte = 0; byte < 4; ++byte)
		output[byte] = static_cast<uint8_t>(value >> (byte * 8));
}

void put_u64(uint8_t *output, uint64_t value)
{
	for (unsigned int byte = 0; byte < 8; ++byte)
		output[byte] = static_cast<uint8_t>(value >> (byte * 8));
}

uint16_t get_u16(const uint8_t *input)
{
	return static_cast<uint16_t>(input[0]) |
	       static_cast<uint16_t>(static_cast<uint16_t>(input[1]) << 8);
}

uint32_t get_u32(const uint8_t *input)
{
	uint32_t value = 0;
	for (unsigned int byte = 0; byte < 4; ++byte)
		value |= static_cast<uint32_t>(input[byte]) << (byte * 8);
	return value;
}

uint64_t get_u64(const uint8_t *input)
{
	uint64_t value = 0;
	for (unsigned int byte = 0; byte < 8; ++byte)
		value |= static_cast<uint64_t>(input[byte]) << (byte * 8);
	return value;
}

bool valid_reason(currency_reason_type reason)
{
	return reason > currency_reason_type::unknown &&
	       reason <= currency_reason_type::corpse_lifecycle;
}

bool valid_name(const char *name, size_t *length)
{
	if (!name || !length)
		return false;
	*length = strnlen(name, CURRENCY_ACCOUNT_NAME_MAX_BYTES + 1);
	if (!*length || *length > CURRENCY_ACCOUNT_NAME_MAX_BYTES)
		return false;
	for (size_t index = 0; index < *length; ++index)
		if (static_cast<unsigned char>(name[index]) < 0x20)
			return false;
	return true;
}

bool vector_valid(const currency_vector &vector)
{
	return std::all_of(vector.amount.begin(), vector.amount.end(), [](int64_t amount)
			   { return amount != std::numeric_limits<int64_t>::min(); });
}

bool any_delta(const currency_command_payload &payload)
{
	for (size_t index = 0; index < CURRENCY_DENOMINATION_COUNT; ++index)
		if (payload.wallet_delta.amount[index] || payload.bank_delta.amount[index])
			return true;
	return false;
}

void encode_vector(uint8_t *output, const currency_vector &vector)
{
	for (size_t index = 0; index < CURRENCY_DENOMINATION_COUNT; ++index)
		put_u64(output + index * 8, static_cast<uint64_t>(vector.amount[index]));
}

currency_vector decode_vector(const uint8_t *input)
{
	currency_vector vector = {};
	for (size_t index = 0; index < CURRENCY_DENOMINATION_COUNT; ++index)
		vector.amount[index] = static_cast<int64_t>(get_u64(input + index * 8));
	return vector;
}
} // namespace

bool currency_account_key(const char *account_name, uint8_t racewar, critical_entity_key *key)
{
	size_t length = 0;
	if (!key || !valid_name(account_name, &length))
		return false;
	std::string canonical(account_name, length);
	std::transform(canonical.begin(), canonical.end(), canonical.begin(),
		       [](unsigned char ch) { return static_cast<char>(std::tolower(ch)); });
	canonical.push_back('\0');
	canonical.push_back(static_cast<char>(racewar));
	std::array<uint8_t, SHA256_DIGEST_LENGTH> digest = {};
	SHA256(reinterpret_cast<const unsigned char *>(canonical.data()), canonical.size(),
	       digest.data());
	uint64_t identity = get_u64(digest.data());
	if (!identity)
		identity = 1;
	*key = { critical_entity_type::account, identity };
	return true;
}

bool currency_command_is_rebasable_wallet_reward(const currency_command_payload &payload)
{
	if (payload.reason != currency_reason_type::wallet_reward)
		return false;
	bool positive = false;
	for (size_t index = 0; index < CURRENCY_DENOMINATION_COUNT; ++index)
	{
		if (payload.wallet_delta.amount[index] < 0 || payload.bank_delta.amount[index])
			return false;
		positive = positive || payload.wallet_delta.amount[index] > 0;
	}
	return positive;
}

bool currency_command_is_rebasable_bank_reward(const currency_command_payload &payload)
{
	if (payload.reason != currency_reason_type::chaos_starter_reward)
		return false;
	bool positive_bank = false;
	for (size_t index = 0; index < CURRENCY_DENOMINATION_COUNT; ++index)
	{
		if (payload.wallet_delta.amount[index] || payload.bank_delta.amount[index] < 0)
			return false;
		positive_bank = positive_bank || payload.bank_delta.amount[index] > 0;
	}
	return positive_bank;
}

bool currency_command_is_rebasable_reward(const currency_command_payload &payload)
{
	return currency_command_is_rebasable_wallet_reward(payload) ||
	       currency_command_is_rebasable_bank_reward(payload);
}

bool currency_command_encode_payload(const currency_command_payload &payload,
				     std::vector<uint8_t> *encoded)
{
	size_t name_length = 0;
	if (!encoded || !payload.pid || !valid_reason(payload.reason) ||
	    !valid_name(payload.account_name.data(), &name_length) ||
	    !vector_valid(payload.wallet_delta) || !vector_valid(payload.bank_delta) ||
	    (!any_delta(payload) && payload.reason != currency_reason_type::corpse_lifecycle))
		return false;
	encoded->assign(CURRENCY_COMMAND_PAYLOAD_BYTES, 0);
	put_u32(encoded->data() + PID_OFFSET, payload.pid);
	(*encoded)[RACEWAR_OFFSET] = payload.racewar;
	put_u16(encoded->data() + REASON_OFFSET, static_cast<uint16_t>(payload.reason));
	put_u64(encoded->data() + REASON_ID_OFFSET, static_cast<uint64_t>(payload.reason_id));
	(*encoded)[NAME_LENGTH_OFFSET] = static_cast<uint8_t>(name_length);
	memcpy(encoded->data() + NAME_OFFSET, payload.account_name.data(), name_length);
	encode_vector(encoded->data() + WALLET_OFFSET, payload.wallet_delta);
	encode_vector(encoded->data() + BANK_OFFSET, payload.bank_delta);
	return true;
}

bool currency_command_decode_payload(const critical_command &command,
				     currency_command_payload *payload)
{
	if (!payload || command.type != critical_command_type::account_bank ||
	    command.payload_version != CURRENCY_COMMAND_PAYLOAD_VERSION ||
	    command.payload.size() != CURRENCY_COMMAND_PAYLOAD_BYTES)
		return false;
	const size_t name_length = command.payload[NAME_LENGTH_OFFSET];
	if (!name_length || name_length > CURRENCY_ACCOUNT_NAME_MAX_BYTES)
		return false;
	for (size_t index = NAME_OFFSET + name_length; index < WALLET_OFFSET; ++index)
		if (command.payload[index])
			return false;
	for (size_t index = BANK_OFFSET + 32; index < command.payload.size(); ++index)
		if (command.payload[index])
			return false;
	*payload = {};
	payload->pid = get_u32(command.payload.data() + PID_OFFSET);
	payload->racewar = command.payload[RACEWAR_OFFSET];
	payload->reason =
		static_cast<currency_reason_type>(get_u16(command.payload.data() + REASON_OFFSET));
	payload->reason_id =
		static_cast<int64_t>(get_u64(command.payload.data() + REASON_ID_OFFSET));
	memcpy(payload->account_name.data(), command.payload.data() + NAME_OFFSET, name_length);
	payload->wallet_delta = decode_vector(command.payload.data() + WALLET_OFFSET);
	payload->bank_delta = decode_vector(command.payload.data() + BANK_OFFSET);
	size_t checked_length = 0;
	critical_entity_key account_key = {};
	const critical_entity_key player_key = { critical_entity_type::player, payload->pid };
	return payload->pid && valid_reason(payload->reason) &&
	       valid_name(payload->account_name.data(), &checked_length) &&
	       checked_length == name_length && vector_valid(payload->wallet_delta) &&
	       vector_valid(payload->bank_delta) &&
	       (any_delta(*payload) || payload->reason == currency_reason_type::corpse_lifecycle) &&
	       currency_account_key(payload->account_name.data(), payload->racewar, &account_key) &&
	       command.keys.size() == 2 && command.expected_revisions.size() == 2 &&
	       critical_entity_key_equal(command.keys[0], player_key) &&
	       critical_entity_key_equal(command.keys[1], account_key) &&
	       critical_entity_key_equal(command.expected_revisions[0].key, player_key) &&
	       critical_entity_key_equal(command.expected_revisions[1].key, account_key);
}

bool currency_command_encode_result(const currency_command_result &result,
				    std::array<uint8_t, CURRENCY_RESULT_PAYLOAD_BYTES> *encoded)
{
	if (!encoded || !vector_valid(result.wallet) || !vector_valid(result.bank))
		return false;
	encode_vector(encoded->data(), result.wallet);
	encode_vector(encoded->data() + 32, result.bank);
	put_u64(encoded->data() + 64, result.wallet_revision);
	put_u64(encoded->data() + 72, result.bank_revision);
	return true;
}

bool currency_command_decode_result(const uint8_t *encoded, size_t size,
				    currency_command_result *result)
{
	if (!encoded || size != CURRENCY_RESULT_PAYLOAD_BYTES || !result)
		return false;
	*result = { .wallet = decode_vector(encoded),
		    .bank = decode_vector(encoded + 32),
		    .wallet_revision = get_u64(encoded + 64),
		    .bank_revision = get_u64(encoded + 72) };
	return vector_valid(result->wallet) && vector_valid(result->bank);
}

bool currency_command_build(critical_command *command, critical_operation_id operation_id,
			    const currency_command_payload &payload,
			    uint64_t expected_wallet_revision, uint64_t expected_bank_revision,
			    critical_source_site source_site,
			    critical_deadline_class deadline_class)
{
	if (!command || critical_operation_id_is_zero(operation_id))
		return false;
	std::vector<uint8_t> encoded;
	critical_entity_key account_key = {};
	if (!currency_command_encode_payload(payload, &encoded) ||
	    !currency_account_key(payload.account_name.data(), payload.racewar, &account_key))
		return false;
	const critical_entity_key player_key = { critical_entity_type::player, payload.pid };
	*command = {
		.schema_version = CRITICAL_COMMAND_SCHEMA_VERSION,
		.operation_id = operation_id,
		.type = critical_command_type::account_bank,
		.payload_version = CURRENCY_COMMAND_PAYLOAD_VERSION,
		.source_site = source_site,
		.deadline_class = deadline_class,
		.accepted_at_usec = 0,
		.keys = { player_key, account_key },
		.expected_revisions = { { player_key, expected_wallet_revision },
					{ account_key, expected_bank_revision } },
		.payload = std::move(encoded),
	};
	return true;
}

unsigned int currency_prepare_mutation(const currency_command_payload &payload,
				       const currency_command_result &before,
				       uint64_t expected_wallet_revision,
				       uint64_t expected_bank_revision,
				       currency_revision_policy revision_policy,
				       std::optional<currency_prepared_mutation> *prepared)
{
	if (!prepared ||
	    (revision_policy != currency_revision_policy::sql_legacy &&
	     revision_policy != currency_revision_policy::flatfile_legacy) ||
	    !vector_valid(payload.wallet_delta) || !vector_valid(payload.bank_delta))
		return EINVAL;
	for (size_t index = 0; index < CURRENCY_DENOMINATION_COUNT; ++index)
		if (before.wallet.amount[index] < 0 || before.wallet.amount[index] > INT_MAX ||
		    before.bank.amount[index] < 0 || before.bank.amount[index] > INT_MAX)
			return EILSEQ;
	const bool rebase = revision_policy == currency_revision_policy::sql_legacy &&
			    currency_command_is_rebasable_reward(payload);
	constexpr uint64_t wildcard = std::numeric_limits<uint64_t>::max();
	if (!rebase && ((expected_wallet_revision != wildcard &&
			 expected_wallet_revision != before.wallet_revision) ||
			(expected_bank_revision != wildcard &&
			 expected_bank_revision != before.bank_revision)))
		return ESTALE;
	auto after = before;
	const auto apply = [](int64_t current, int64_t delta, int64_t *next) -> unsigned int
	{
		// The current value is bounded above, so subtraction avoids signed
		// overflow even for an extreme positive delta. INT64_MIN is rejected.
		if (delta < 0 && current < -delta)
			return ENOSPC;
		if (delta > 0 && current > static_cast<int64_t>(INT_MAX) - delta)
			return ERANGE;
		*next = current + delta;
		return 0;
	};
	for (size_t index = 0; index < CURRENCY_DENOMINATION_COUNT; ++index)
	{
		const auto wallet_error = apply(before.wallet.amount[index],
						payload.wallet_delta.amount[index],
						&after.wallet.amount[index]);
		const auto bank_error = apply(before.bank.amount[index],
					      payload.bank_delta.amount[index],
					      &after.bank.amount[index]);
		// SQL historically reports either insufficient holding before either
		// overflow at the same denomination. Flatfile checks wallet then bank.
		if (revision_policy == currency_revision_policy::sql_legacy &&
		    (wallet_error == ENOSPC || bank_error == ENOSPC))
			return ENOSPC;
		if (wallet_error)
			return wallet_error;
		if (bank_error)
			return bank_error;
	}
	if (before.wallet_revision == wildcard || before.bank_revision == wildcard)
		return ERANGE;
	++after.wallet_revision;
	++after.bank_revision;
	*prepared = currency_prepared_mutation(payload, before, after);
	return 0;
}
