#ifndef DURIS_COIN_TRANSFER_COMMAND_H
#define DURIS_COIN_TRANSFER_COMMAND_H

#include "persistence/critical_command.h"
#include "economy/currency_command.h"
#include "item/item_transfer_command.h"

#include <array>
#include <cstdint>

constexpr uint16_t COIN_TRANSFER_PAYLOAD_VERSION = 1;
constexpr size_t COIN_TRANSFER_RESULT_BYTES =
	2 * (CURRENCY_RESULT_PAYLOAD_BYTES + ITEM_TRANSFER_RESULT_BYTES);

// Exactly two endpoints: a wallet or a single physical pile. The existing
// currency/item commands carry their revision fences and the pile's full payload.
// They are applied inside ONE coin commit, never submitted independently.
struct coin_transfer_endpoint
{
	std::array<int32_t, 4> before = {};
	std::array<int32_t, 4> after = {};
	critical_command change = {};
};

struct coin_transfer_payload
{
	coin_transfer_endpoint source;
	coin_transfer_endpoint destination;
};

struct coin_transfer_result
{
	std::array<currency_command_result, 2> wallets = {};
	std::array<item_transfer_result, 2> piles = {};
};

bool coin_transfer_command_build(critical_command *command,
				 const critical_operation_id &operation_id,
				 const coin_transfer_payload &payload,
				 critical_source_site source_site,
				 critical_deadline_class deadline_class);
bool coin_transfer_command_decode_payload(const critical_command &command,
					  coin_transfer_payload *payload);
bool coin_transfer_command_encode_result(const coin_transfer_payload &payload,
					 const coin_transfer_result &result,
					 std::array<uint8_t, COIN_TRANSFER_RESULT_BYTES> *encoded);
bool coin_transfer_command_decode_result(const coin_transfer_payload &payload,
					 const uint8_t *encoded, size_t size,
					 coin_transfer_result *result);
// Adjust only a revision advanced by the source in this same transaction.
bool coin_transfer_command_destination_after_source(const coin_transfer_payload &payload,
						    const coin_transfer_result &result,
						    critical_command *destination);

#endif
