#include "telemetry/telemetry_combat_summary.h"

#include <cstdio>
#include <cstdint>

namespace
{
#define CHECK(expression)                                                                         \
	do                                                                                        \
	{                                                                                         \
		if (!(expression))                                                                \
		{                                                                                 \
			std::fprintf(stderr, "check failed: %s (%s:%d)\n", #expression, __FILE__, \
				     __LINE__);                                                   \
			return 1;                                                                 \
		}                                                                                 \
	} while (false)

struct sink
{
	telemetry_combat_summary_payload rows[128]{};
	std::size_t count = 0U;
};

bool collect(void *context, const telemetry_combat_summary_payload &row) noexcept
{
	auto &output = *static_cast<sink *>(context);
	if (output.count >= sizeof(output.rows) / sizeof(output.rows[0]))
		return false;
	output.rows[output.count++] = row;
	return true;
}

telemetry_encounter_id encounter(std::uint64_t sequence)
{
	telemetry_encounter_id value{};
	value.producer.boot_id = 11U;
	value.producer.process_id = 22U;
	value.sequence = sequence;
	return value;
}

telemetry_encounter_source source()
{
	telemetry_encounter_source value{};
	value.environment_id = 101U;
	value.season_id = 202U;
	value.config_id = 303U;
	value.classifier_version = 4U;
	value.policy_version = 5U;
	value.zone_vnum = 9;
	value.group_key = 88U;
	return value;
}

telemetry_combat_actor_ref actor(telemetry_id id, telemetry_pid pid, telemetry_subject_id owner,
				 telemetry_combat_actor_kind kind, std::uint16_t power)
{
	telemetry_combat_actor_ref value{};
	value.actor_id = id;
	value.actor_pid = pid;
	value.owner_subject_id = owner;
	value.kind = kind;
	value.power_band = power;
	return value;
}

const telemetry_combat_summary_payload *find_row(const sink &output,
						 telemetry_combat_actor_kind kind, telemetry_id id)
{
	for (std::size_t index = 0U; index < output.count; ++index)
		if (output.rows[index].actor_kind == kind && output.rows[index].actor_id == id)
			return &output.rows[index];
	return nullptr;
}
} // namespace

int main()
{
	telemetry_combat_summary_state state{};
	telemetry_combat_summary_state_init(&state);
	const auto run = encounter(7U);
	const auto run_source = source();
	const auto player = actor(1001U, 1001, 1001U, telemetry_combat_actor_kind::player, 40U);
	const auto pet =
		actor(9001U, TELEMETRY_UNKNOWN_PID, 1001U, telemetry_combat_actor_kind::pet, 35U);
	const auto npc =
		actor(7001U, TELEMETRY_UNKNOWN_PID, 0U, telemetry_combat_actor_kind::npc, 42U);
	CHECK(telemetry_combat_summary_begin(&state, run, run_source, telemetry_encounter_mode::pve,
					     100U, 1'000U)
		      .outcome == telemetry_combat_summary_outcome::accepted);
	CHECK(telemetry_combat_summary_add_actor(&state, run, player, 100U).outcome ==
	      telemetry_combat_summary_outcome::accepted);
	CHECK(telemetry_combat_summary_add_actor(&state, run, pet, 101U).outcome ==
	      telemetry_combat_summary_outcome::accepted);
	CHECK(telemetry_combat_summary_add_actor(&state, run, npc, 102U).outcome ==
	      telemetry_combat_summary_outcome::accepted);
	(void)telemetry_combat_summary_record_damage(&state, player, npc, 120U, 110U,
						     TELEMETRY_COMBAT_MODIFIER_MELEE);
	(void)telemetry_combat_summary_record_damage(&state, npc, player, 35U, 111U, 0U);
	(void)telemetry_combat_summary_record_healing(&state, player, npc, 100U, 70U, 112U, 0U);
	(void)telemetry_combat_summary_record_control(&state, player, npc, 2U, 113U, 0U);
	(void)telemetry_combat_summary_record_tanking(&state, player, &npc, 120U, 0U);
	(void)telemetry_combat_summary_record_tanking(&state, player, &npc, 160U, 0U);
	(void)telemetry_combat_summary_record_tanking(&state, player, nullptr, 170U, 0U);
	(void)telemetry_combat_summary_cast_attempt(&state, player, 41, 120U, 0U);
	(void)telemetry_combat_summary_cast_abort(&state, player, 125U);
	(void)telemetry_combat_summary_cast_attempt(&state, player, 42, 130U, 0U);
	(void)telemetry_combat_summary_cast_complete(&state, player, 145U);

	sink output{};
	const auto closed = telemetry_combat_summary_close(
		&state, run, telemetry_encounter_outcome::success, 200U, 2'000U, collect, &output);
	CHECK(closed.outcome == telemetry_combat_summary_outcome::accepted);
	CHECK(output.count == 3U);
	const auto *player_row = find_row(output, telemetry_combat_actor_kind::player, 1001U);
	const auto *pet_row = find_row(output, telemetry_combat_actor_kind::pet, 9001U);
	const auto *npc_row = find_row(output, telemetry_combat_actor_kind::npc, 7001U);
	CHECK(player_row != nullptr && pet_row != nullptr && npc_row != nullptr);
	CHECK(player_row->damage_dealt == 120U && player_row->damage_taken == 35U);
	CHECK(player_row->healing_attempted == 100U && player_row->effective_healing == 70U &&
	      player_row->overhealing == 30U);
	CHECK(player_row->control_applications == 2U);
	CHECK(player_row->casting_attempts == 2U && player_row->casting_completions == 1U &&
	      player_row->casting_aborts == 1U && player_row->casting_elapsed_usec == 20U);
	CHECK(player_row->tanking_usec == 50U && player_row->opponent_count == 1U &&
	      player_row->opponent_power_band == 42U);
	CHECK(player_row->unique_player_count == 1U && player_row->participant_count == 3U);
	CHECK(pet_row->owner_subject_id == 1001U && pet_row->unique_player_count == 1U);
	CHECK(npc_row->actor_kind == telemetry_combat_actor_kind::npc &&
	      npc_row->unique_player_count == 1U);
	CHECK(telemetry_combat_summary_payload_is_valid(*player_row));

	telemetry_combat_summary_state tail{};
	telemetry_combat_summary_state_init(&tail);
	const auto tail_run = encounter(9U);
	CHECK(telemetry_combat_summary_begin(&tail, tail_run, run_source,
					     telemetry_encounter_mode::pve, 300U, 3'000U)
		      .outcome == telemetry_combat_summary_outcome::accepted);
	CHECK(telemetry_combat_summary_add_actor(&tail, tail_run, player, 300U).outcome ==
	      telemetry_combat_summary_outcome::accepted);
	(void)telemetry_combat_summary_cast_attempt(&tail, player, 43, 310U, 0U);
	sink tail_output{};
	const auto tail_closed = telemetry_combat_summary_close(
		&tail, tail_run, telemetry_encounter_outcome::withdrawal, 350U, 3'500U, collect,
		&tail_output);
	CHECK(tail_closed.outcome == telemetry_combat_summary_outcome::accepted);
	CHECK(tail_output.count == 1U);
	CHECK(tail_output.rows[0].casting_attempts == 1U &&
	      tail_output.rows[0].casting_completions == 0U &&
	      tail_output.rows[0].casting_aborts == 1U &&
	      tail_output.rows[0].casting_elapsed_usec == 40U);
	CHECK((tail_output.rows[0].quality_flags & TELEMETRY_QUALITY_UNCLOSED_TAIL) != 0U);

	telemetry_combat_summary_state capped{};
	telemetry_combat_summary_state_init(&capped);
	const auto capped_run = encounter(8U);
	CHECK(telemetry_combat_summary_begin(&capped, capped_run, run_source,
					     telemetry_encounter_mode::pve, 10U, 10U)
		      .outcome == telemetry_combat_summary_outcome::accepted);
	for (std::uint64_t index = 0U; index < 65U; ++index)
	{
		const auto participant =
			actor(2'000U + index, static_cast<telemetry_pid>(2'000U + index),
			      2'000U + index, telemetry_combat_actor_kind::player, 20U);
		(void)telemetry_combat_summary_add_actor(&capped, capped_run, participant, 11U);
	}
	sink capped_output{};
	const auto capped_closed = telemetry_combat_summary_close(
		&capped, capped_run, telemetry_encounter_outcome::failure, 20U, 20U, collect,
		&capped_output);
	CHECK(capped_closed.outcome == telemetry_combat_summary_outcome::accepted);
	CHECK(capped_output.count == TELEMETRY_COMBAT_SUMMARY_MAX_ACTORS);
	CHECK(capped_output.rows[0].dropped_participant_count == 1U);
	CHECK((capped_output.rows[0].quality_flags & TELEMETRY_QUALITY_CARDINALITY_OVERFLOW) != 0U);
	CHECK(capped_output.rows[0].unique_player_count ==
	      TELEMETRY_COMBAT_SUMMARY_MAX_UNIQUE_PLAYERS);

	telemetry_combat_summary_state invalid_close{};
	telemetry_combat_summary_state_init(&invalid_close);
	const auto invalid_run = encounter(10U);
	CHECK(telemetry_combat_summary_begin(&invalid_close, invalid_run, run_source,
					     telemetry_encounter_mode::pve, 100U, 100U)
		      .outcome == telemetry_combat_summary_outcome::accepted);
	const auto invalid_flush = telemetry_combat_summary_close_all(
		&invalid_close, telemetry_encounter_outcome::shutdown, 99U, 99U, collect, &output);
	CHECK(invalid_flush.outcome == telemetry_combat_summary_outcome::invalid);
	CHECK(invalid_close.slots[0].occupied == 1U);
	return 0;
}
