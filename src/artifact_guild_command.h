#ifndef ARTIFACT_GUILD_COMMAND_H
#define ARTIFACT_GUILD_COMMAND_H

#include "critical_command.h"

#include <array>
#include <cstdint>

constexpr uint16_t ARTIFACT_GUILD_PAYLOAD_VERSION = 1;
constexpr size_t ARTIFACT_GUILD_MAX_ARTIFACTS = 15;
constexpr size_t ARTIFACT_GUILD_RESULT_BYTES = 576;
constexpr uint8_t ARTIFACT_DELTA_FEED = 1 << 0;
constexpr uint8_t ARTIFACT_DELTA_BIND = 1 << 1;

struct artifact_guild_delta
{
	int32_t vnum;
	uint8_t flags;
	uint64_t expected_revision;
	int64_t expected_timer;
	int64_t timer;
	int32_t expected_bind_owner_pid;
	int32_t bind_owner_pid;
	int64_t expected_bind_timer;
	int64_t bind_timer;
};

struct artifact_guild_payload
{
	critical_operation_id parent_operation_id;
	uint32_t actor_pid;
	uint32_t guild_id;
	uint64_t expected_guild_revision;
	int64_t prestige_delta;
	int64_t construction_delta;
	uint16_t artifact_count;
	std::array<artifact_guild_delta, ARTIFACT_GUILD_MAX_ARTIFACTS> artifacts;
};

struct artifact_guild_result_entry
{
	int32_t vnum;
	int64_t timer;
	int32_t bind_owner_pid;
	int64_t bind_timer;
	uint64_t revision;
};

struct artifact_guild_result
{
	uint32_t guild_id;
	uint64_t prestige;
	uint64_t construction;
	uint64_t guild_revision;
	uint16_t artifact_count;
	std::array<artifact_guild_result_entry, ARTIFACT_GUILD_MAX_ARTIFACTS> artifacts;
};

bool artifact_guild_command_encode_payload(const artifact_guild_payload &payload,
					   std::vector<uint8_t> *encoded);
bool artifact_guild_command_decode_payload(const critical_command &command,
					   artifact_guild_payload *payload);
bool artifact_guild_command_encode_result(const artifact_guild_result &result,
					  std::array<uint8_t, ARTIFACT_GUILD_RESULT_BYTES> *encoded);
bool artifact_guild_command_decode_result(const uint8_t *encoded, size_t size,
					  artifact_guild_result *result);
bool artifact_guild_command_build(critical_command *command, critical_operation_id operation_id,
				  const artifact_guild_payload &payload);

#endif
