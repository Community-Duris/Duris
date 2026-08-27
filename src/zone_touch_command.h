#ifndef ZONE_TOUCH_COMMAND_H
#define ZONE_TOUCH_COMMAND_H

#include "critical_command.h"

#include <array>

constexpr uint16_t ZONE_TOUCH_PAYLOAD_VERSION = 1;
constexpr size_t ZONE_TOUCH_MAX_PARTICIPANTS = 15;
constexpr size_t ZONE_TOUCH_RESULT_BYTES = 88;

struct zone_touch_payload
{
	uint32_t zone_number;
	uint32_t toucher_pid;
	int32_t boot_time;
	int32_t touched_at;
	uint16_t group_size;
	std::array<uint32_t, ZONE_TOUCH_MAX_PARTICIPANTS> participant_pids;
	int32_t epic_value;
	int16_t alignment_delta;
	uint8_t reset_requested;
};

using zone_touch_result = zone_touch_payload;

bool zone_touch_command_build(critical_command *command, critical_operation_id operation_id,
			      const zone_touch_payload &payload);
bool zone_touch_command_decode_payload(const critical_command &command,
				       zone_touch_payload *payload);
bool zone_touch_command_encode_result(const zone_touch_result &result,
				      std::array<uint8_t, ZONE_TOUCH_RESULT_BYTES> *encoded);
bool zone_touch_command_decode_result(const uint8_t *encoded, size_t size,
				      zone_touch_result *result);

#endif
