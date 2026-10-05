#include "telemetry/telemetry_battle_result.h"

#include <algorithm>
#include <array>
#include <cassert>
#include <cstdint>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <string_view>

namespace
{
using observation = telemetry_battle_result_observation;
using kind = telemetry_battle_result_kind;
using authority = telemetry_battle_result_authority;
using reason = telemetry_battle_result_reason;

telemetry_control_actor_context actor(std::int32_t pid)
{
	telemetry_control_actor_context value{};
	value.actor = { static_cast<std::uint64_t>(pid),     pid, static_cast<std::uint64_t>(pid),
			telemetry_combat_actor_kind::player, {},  50U };
	value.session = { { 101U, 202U }, static_cast<std::uint64_t>(pid) };
	value.dimensions = { 50U, 1U, 1U, 1U, 7, 1U };
	value.group_key = static_cast<std::uint64_t>(pid);
	value.context_version = TELEMETRY_BATTLE_ACTOR_CONTEXT_VERSION;
	return value;
}

observation basis(std::uint64_t sequence)
{
	observation value{};
	value.producer = { 101U, 202U };
	value.scope = { 11U, 22U, 33U, 4U, 5U, -1, 0U };
	value.sequence = sequence;
	value.target = actor(42);
	value.target_association = { 7U, 2U, 9U };
	value.start_usec = 100U;
	value.at_usec = 101U;
	value.start_utc_usec = 10'100;
	value.at_utc_usec = 10'101;
	value.build_version = 6U;
	value.content_version = 7U;
	value.credited_zone_vnum = value.from_room_vnum = value.to_room_vnum = -1;
	value.definition_version = TELEMETRY_BATTLE_RESULT_DEFINITION_VERSION;
	value.producer_version = TELEMETRY_BATTLE_RESULT_PRODUCER_VERSION;
	return value;
}

void export_value(const observation &value, unsigned index)
{
	assert(telemetry_battle_result_observation_is_valid(value));
	telemetry_record record{};
	record.header = { TELEMETRY_SCHEMA_VERSION,
			  telemetry_record_kind::battle_result,
			  0U,
			  { value.producer, index },
			  value.at_utc_usec };
	record.payload.battle_result = value;
	assert(telemetry_record_is_valid(record));
	assert(!telemetry_record_kind_is_control(record.header.kind));
	static_assert(sizeof(telemetry_record) <= TELEMETRY_RECORD_MAX_BYTES);
	++record.header.key.producer.process_id;
	assert(!telemetry_record_is_valid(record));
	--record.header.key.producer.process_id;
	record.header.occurrence_utc_usec =
		value.at_utc_usec == TELEMETRY_UTC_UNKNOWN ? 0 : TELEMETRY_UTC_UNKNOWN;
	assert(!telemetry_record_is_valid(record));

	std::array<std::uint8_t, TELEMETRY_BATTLE_RESULT_WIRE_BYTES> wire{};
	assert(telemetry_battle_result_observation_encode(value, wire.data(), wire.size()));
	observation decoded{};
	assert(telemetry_battle_result_observation_decode(wire.data(), wire.size(), &decoded));
	assert(telemetry_battle_result_observation_same_key(value, decoded));
	assert(telemetry_battle_result_observation_equal(value, decoded));
	auto different = value;
	++different.sequence;
	assert(!telemetry_battle_result_observation_same_key(value, different));
	assert(!telemetry_battle_result_observation_equal(value, different));
	different = value;
	different.reserved[0] = 1U;
	assert(!telemetry_battle_result_observation_is_valid(different));
	assert(!telemetry_battle_result_observation_equal(value, different));
	decoded = value;
	assert(!telemetry_battle_result_observation_decode(nullptr, wire.size(), &decoded));
	assert(decoded.sequence == 0U && decoded.target.actor.actor_id == 0U);
	assert(std::all_of(std::begin(decoded.operation_id), std::end(decoded.operation_id),
			   [](std::uint8_t byte) { return byte == 0U; }));
	std::cout << "BATTLE_RESULT_JSON {\"case\":" << index << ",\"fields\":{";
	bool first = true;
#define TELEMETRY_RESULT_FIELD(name, member, width, signed_value)     \
	if (!first)                                                   \
		std::cout << ',';                                     \
	first = false;                                                \
	std::cout << '"' << #name << "\":";                           \
	if constexpr (signed_value)                                   \
		std::cout << static_cast<std::int64_t>(value.member); \
	else                                                          \
		std::cout << static_cast<std::uint64_t>(value.member);
#define TELEMETRY_RESULT_BYTES(name, member, width)                            \
	if (!first)                                                            \
		std::cout << ',';                                              \
	first = false;                                                         \
	std::cout << '"' << #name << "\":\"" << std::hex << std::setfill('0'); \
	for (auto byte : value.member)                                         \
		std::cout << std::setw(2) << static_cast<unsigned>(byte);      \
	std::cout << '"' << std::dec;
#include "telemetry/telemetry_battle_result_fields.inc"
#undef TELEMETRY_RESULT_FIELD
#undef TELEMETRY_RESULT_BYTES
	std::cout << "},\"wire\":\"" << std::hex << std::setfill('0');
	for (auto byte : wire)
		std::cout << std::setw(2) << static_cast<unsigned>(byte);
	std::cout << "\"}" << std::dec << '\n';
}

int verify(const char *path)
{
	std::ifstream input(path, std::ios::binary);
	assert(input);
	std::array<std::uint8_t, 512> bytes{};
	std::array<std::uint8_t, 4> header{};
	while (input.read(reinterpret_cast<char *>(header.data()), header.size()))
	{
		const auto length = (static_cast<std::uint32_t>(header[0]) << 24U) |
				    (static_cast<std::uint32_t>(header[1]) << 16U) |
				    (static_cast<std::uint32_t>(header[2]) << 8U) | header[3];
		if (length > bytes.size())
		{
			input.ignore(length);
			std::cout << "0\n";
			continue;
		}
		input.read(reinterpret_cast<char *>(bytes.data()), length);
		assert(input);
		observation value{};
		std::cout << (telemetry_battle_result_observation_decode(bytes.data(), length,
									 &value) ?
				      "1\n" :
				      "0\n");
	}
	return 0;
}
} // namespace

int main(int argc, char **argv)
{
	if (argc == 3 && std::string_view(argv[1]) == "--verify")
		return verify(argv[2]);
	auto death = basis(1U);
	death.kind = kind::death_observed;
	death.authority = authority::native_death;
	death.source = actor(43);
	death.source_association = { 7U, 2U, 9U };
	export_value(death, 1U);
	auto movement = basis(2U);
	movement.kind = kind::flee_movement;
	movement.authority = authority::accepted_flee;
	movement.flags = TELEMETRY_BATTLE_RESULT_MOVED;
	movement.from_room_vnum = 100;
	movement.to_room_vnum = 101;
	export_value(movement, 2U);
	auto retreat = movement;
	retreat.sequence = 3U;
	retreat.kind = kind::withdrawal;
	retreat.authority = authority::accepted_retreat;
	export_value(retreat, 3U);
	auto disengage = retreat;
	disengage.sequence = 4U;
	disengage.authority = authority::accepted_disengage;
	disengage.flags = 0U;
	disengage.to_room_vnum = disengage.from_room_vnum;
	export_value(disengage, 4U);
	auto escape = movement;
	escape.sequence = 5U;
	escape.parent_sequence = movement.sequence;
	escape.kind = kind::escape_observed;
	escape.authority = authority::bounded_escape_watch;
	escape.proof_window_usec = TELEMETRY_BATTLE_ESCAPE_MIN_USEC;
	escape.start_usec = movement.at_usec;
	escape.start_utc_usec = movement.at_utc_usec;
	escape.at_usec = escape.start_usec + escape.proof_window_usec;
	escape.at_utc_usec = escape.start_utc_usec + escape.proof_window_usec;
	escape.flags = TELEMETRY_BATTLE_RESULT_MOVED | TELEMETRY_BATTLE_RESULT_NO_OPPONENT |
		       TELEMETRY_BATTLE_RESULT_SAME_SESSION |
		       TELEMETRY_BATTLE_RESULT_WATCH_COMPLETE;
	export_value(escape, 5U);
	auto objective = basis(6U);
	objective.kind = kind::objective_requested;
	objective.authority = authority::zone_touch_submit;
	objective.source_object_uid = 901U;
	objective.credited_zone_vnum = 7;
	objective.participant_count = 3U;
	objective.source_payload_version = 2U;
	objective.flags = TELEMETRY_BATTLE_RESULT_RECORD_ZONE |
			  TELEMETRY_BATTLE_RESULT_RESET_REQUESTED;
	for (std::size_t index = 0U; index < sizeof(objective.operation_id); ++index)
		objective.operation_id[index] = static_cast<std::uint8_t>(index + 1U);
	export_value(objective, 6U);
	objective.sequence = 7U;
	objective.kind = kind::objective_committed;
	objective.authority = authority::zone_touch_receipt;
	export_value(objective, 7U);
	auto legacy = objective;
	legacy.sequence = 8U;
	legacy.source_payload_version = 1U;
	legacy.source_object_uid = 0U;
	export_value(legacy, 8U);
	auto uncertain = movement;
	uncertain.sequence = 9U;
	uncertain.parent_sequence = movement.sequence;
	uncertain.kind = kind::unresolved;
	uncertain.authority = authority::bounded_escape_watch;
	uncertain.reason = reason::reengaged;
	uncertain.quality_flags = TELEMETRY_QUALITY_CONTEXT_UNKNOWN;
	export_value(uncertain, 9U);
	auto censored = uncertain;
	censored.sequence = 10U;
	censored.kind = kind::censored;
	censored.authority = authority::lifecycle;
	censored.reason = reason::copyover;
	export_value(censored, 10U);
	auto receipt_unknown = objective;
	receipt_unknown.sequence = 11U;
	receipt_unknown.kind = kind::unresolved;
	receipt_unknown.reason = reason::receipt_unknown;
	receipt_unknown.quality_flags = TELEMETRY_QUALITY_CONTEXT_UNKNOWN;
	export_value(receipt_unknown, 11U);
	auto pet = death;
	pet.sequence = 12U;
	pet.target.actor.actor_id = TELEMETRY_BATTLE_NPC_GENERATION_TAG | 55U;
	pet.target.actor.actor_pid = TELEMETRY_UNKNOWN_PID;
	pet.target.actor.kind = telemetry_combat_actor_kind::pet;
	pet.target.session = {};
	export_value(pet, 12U);
	auto npc = pet;
	npc.sequence = 13U;
	npc.target.actor.kind = telemetry_combat_actor_kind::npc;
	npc.target.actor.owner_subject_id = 0U;
	export_value(npc, 13U);
	auto configuration_unknown = uncertain;
	configuration_unknown.sequence = 14U;
	configuration_unknown.reason = reason::configuration_unknown;
	configuration_unknown.scope.config_id = 0U;
	configuration_unknown.scope.classifier_version = 0U;
	configuration_unknown.scope.policy_version = 0U;
	configuration_unknown.build_version = configuration_unknown.content_version = 0U;
	export_value(configuration_unknown, 14U);
	auto clock = death;
	clock.sequence = 15U;
	clock.start_utc_usec = clock.at_utc_usec + 1;
	clock.quality_flags = TELEMETRY_QUALITY_CLOCK_DISCONTINUITY;
	export_value(clock, 15U);
	auto outside = objective;
	outside.sequence = 16U;
	outside.target_association = {};
	export_value(outside, 16U);
	auto recovered = objective;
	recovered.sequence = 17U;
	recovered.authority = authority::zone_touch_outbox;
	recovered.flags |= TELEMETRY_BATTLE_RESULT_RECOVERED;
	export_value(recovered, 17U);
	std::cout
		<< "Typed battle result values passed: 17 fixtures; payload=" << sizeof(observation)
		<< ", wire=" << TELEMETRY_BATTLE_RESULT_WIRE_BYTES << '\n';
	return 0;
}
