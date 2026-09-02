#ifndef CURRENCY_COMMAND_H
#define CURRENCY_COMMAND_H

#include "persistence/critical_command.h"

#include <array>
#include <cstdint>

constexpr uint16_t CURRENCY_COMMAND_PAYLOAD_VERSION = 1;
constexpr size_t CURRENCY_ACCOUNT_NAME_MAX_BYTES = 50;
constexpr size_t CURRENCY_COMMAND_PAYLOAD_BYTES = 136;
constexpr size_t CURRENCY_RESULT_PAYLOAD_BYTES = 80;
constexpr size_t CURRENCY_DENOMINATION_COUNT = 4;

enum class currency_reason_type : uint16_t
{
	unknown = 0,
	atm_deposit,
	atm_withdraw,
	bank_payment,
	bank_reward,
	wallet_reward,
	wallet_spend,
	refund,
	auction_pickup,
	auction_listing,
	auction_bid,
	auction_claim,
	ship_insurance,
	boon_reward,
	operator_adjustment,
	chaos_starter_reward,
};

struct currency_vector
{
	std::array<int64_t, CURRENCY_DENOMINATION_COUNT> amount;
};

struct currency_command_payload
{
	uint32_t pid;
	uint8_t racewar;
	currency_reason_type reason;
	int64_t reason_id;
	std::array<char, CURRENCY_ACCOUNT_NAME_MAX_BYTES + 1> account_name;
	currency_vector wallet_delta;
	currency_vector bank_delta;
};

struct currency_command_result
{
	currency_vector wallet;
	currency_vector bank;
	uint64_t wallet_revision;
	uint64_t bank_revision;
};

bool currency_account_key(const char *account_name, uint8_t racewar, critical_entity_key *key);
bool currency_command_is_rebasable_wallet_reward(const currency_command_payload &payload);
bool currency_command_is_rebasable_bank_reward(const currency_command_payload &payload);
bool currency_command_is_rebasable_reward(const currency_command_payload &payload);
bool currency_command_encode_payload(const currency_command_payload &payload,
				     std::vector<uint8_t> *encoded);
bool currency_command_decode_payload(const critical_command &command,
				     currency_command_payload *payload);
bool currency_command_encode_result(const currency_command_result &result,
				    std::array<uint8_t, CURRENCY_RESULT_PAYLOAD_BYTES> *encoded);
bool currency_command_decode_result(const uint8_t *encoded, size_t size,
				    currency_command_result *result);
bool currency_command_build(critical_command *command, critical_operation_id operation_id,
			    const currency_command_payload &payload,
			    uint64_t expected_wallet_revision, uint64_t expected_bank_revision,
			    critical_source_site source_site,
			    critical_deadline_class deadline_class);

#endif
