#ifndef BOON_SHOP_COMMAND_H
#define BOON_SHOP_COMMAND_H

#include "critical_command.h"

#include <array>
#include <cstdint>

constexpr uint16_t BOON_SHOP_PAYLOAD_VERSION = 1;
constexpr size_t BOON_SHOP_RESULT_BYTES = 32;
constexpr uint8_t BOON_SHOP_BASE_STAT_COUNT = 10;

struct boon_shop_payload
{
	uint32_t pid = 0;
	uint8_t stat_index = 0;
};

struct boon_shop_result
{
	uint32_t pid = 0;
	uint8_t stat_index = 0;
	int16_t stat_value = 0;
	int64_t remaining_stat_points = 0;
	uint64_t stat_revision = 0;
};

bool boon_shop_command_build(critical_command *command, critical_operation_id operation_id,
			     const boon_shop_payload &payload);
bool boon_shop_command_decode_payload(const critical_command &command, boon_shop_payload *payload);
bool boon_shop_command_encode_result(const boon_shop_result &result,
				     std::array<uint8_t, BOON_SHOP_RESULT_BYTES> *encoded);
bool boon_shop_command_decode_result(const uint8_t *encoded, size_t size, boon_shop_result *result);

#endif
