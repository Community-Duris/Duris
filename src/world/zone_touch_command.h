#ifndef ZONE_TOUCH_COMMAND_H
#define ZONE_TOUCH_COMMAND_H

#include "persistence/critical_command.h"

#include <array>

constexpr uint16_t ZONE_TOUCH_PAYLOAD_VERSION = 2;
constexpr size_t ZONE_TOUCH_MAX_PARTICIPANTS = 15;
constexpr size_t ZONE_TOUCH_LEGACY_RESULT_BYTES = 88;
constexpr size_t ZONE_TOUCH_RESULT_BYTES = 512;

struct zone_touch_award
{
	int32_t amount = 0;
	int32_t errand = 0;
	uint8_t flags = 0; // blessing=1, outstanding task penalty=2
};

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
	uint64_t stone_uid = 0; // zero identifies legacy metadata-only commands
	int32_t stone_level = 0;
	uint8_t record_zone = 1;
	std::array<zone_touch_award, ZONE_TOUCH_MAX_PARTICIPANTS> awards = {};
};

struct zone_touch_result : zone_touch_payload
{
	zone_touch_result(const zone_touch_payload &payload = {})
		: zone_touch_payload(payload)
	{
	}
	std::array<int64_t, ZONE_TOUCH_MAX_PARTICIPANTS> balances = {};
	std::array<uint64_t, ZONE_TOUCH_MAX_PARTICIPANTS> revisions = {};
	bool recovered_claim = false;
};

bool zone_touch_award_command(const critical_command &parent, size_t index,
			      critical_command *award);

bool zone_touch_command_build(critical_command *command, critical_operation_id operation_id,
			      const zone_touch_payload &payload);
bool zone_touch_command_decode_payload(const critical_command &command,
				       zone_touch_payload *payload);
bool zone_touch_command_encode_result(const zone_touch_result &result,
				      std::array<uint8_t, ZONE_TOUCH_RESULT_BYTES> *encoded);
bool zone_touch_command_decode_result(const uint8_t *encoded, size_t size,
				      zone_touch_result *result);

#endif
