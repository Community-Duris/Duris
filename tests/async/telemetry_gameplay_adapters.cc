#include "telemetry/telemetry_config_private.h"
#include "telemetry/telemetry_runtime.h"
#include "telemetry/telemetry_transport_private.h"
#include "core/structs.h"
#include "core/utils.h"

#include "telemetry/telemetry_session.h"
#include "telemetry_test_runtime.h"
#include <memory>
#include <atomic>
#include <cassert>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <thread>
#include <vector>

P_room world = nullptr;
struct zone_data *zone_table = nullptr;
int top_of_zone_table = -1;
int top_of_world = -1;
P_char fixture_pet = nullptr;
P_char fixture_pet_master = nullptr;
P_char get_linked_char(P_char character, ush_int link)
{
	return character == fixture_pet && link == LNK_PET ? fixture_pet_master : nullptr;
}

namespace
{
struct fake_repository
{
	std::uint32_t init_calls = 0U;
	std::uint32_t apply_calls = 0U;
	std::uint32_t applied_records = 0U;
	std::uint32_t shutdown_calls = 0U;
	bool saw_combat_context = false;
	bool saw_idle_combat = false;
	std::vector<telemetry_interval_payload> intervals; // worker writes; read after join
	std::vector<telemetry_session_lifecycle_payload> lifecycles;
	std::vector<telemetry_ownership_payload> ownership;
	std::vector<telemetry_encounter_payload> encounters;
	std::vector<telemetry_combat_summary_payload> combat;
};

telemetry_repository_outcome fake_init(void *context, telemetry_repository_config) noexcept
{
	auto *fake = static_cast<fake_repository *>(context);
	++fake->init_calls;
	return telemetry_repository_outcome::ready;
}

telemetry_apply_batch_result fake_apply(void *context, const telemetry_record *records,
					std::size_t count) noexcept
{
	auto *fake = static_cast<fake_repository *>(context);
	++fake->apply_calls;
	fake->applied_records += static_cast<std::uint32_t>(count);
	telemetry_apply_batch_result result{};
	result.outcome = telemetry_batch_outcome::committed;
	result.input_count = static_cast<std::uint16_t>(count);
	result.result_count = static_cast<std::uint16_t>(count);
	result.applied_count = static_cast<std::uint16_t>(count);
	if (count != 0U)
	{
		result.first_record_seq = records[0].header.key.record_seq;
		result.last_record_seq = records[count - 1U].header.key.record_seq;
	}
	for (std::size_t index = 0U; index < count; ++index)
	{
		result.results[index].key = records[index].header.key;
		result.results[index].outcome = telemetry_apply_outcome::applied;
		if (records[index].header.kind == telemetry_record_kind::interval)
			fake->intervals.push_back(records[index].payload.interval);
		if (records[index].header.kind == telemetry_record_kind::session_lifecycle)
			fake->lifecycles.push_back(records[index].payload.lifecycle);
		if (records[index].header.kind == telemetry_record_kind::ownership)
		{
			assert(telemetry_record_is_valid(records[index]));
			fake->ownership.push_back(records[index].payload.ownership);
		}
		if (records[index].header.kind == telemetry_record_kind::encounter)
		{
			assert(telemetry_record_is_valid(records[index]));
			fake->encounters.push_back(records[index].payload.encounter);
		}
		if (records[index].header.kind == telemetry_record_kind::combat_summary)
		{
			assert(telemetry_record_is_valid(records[index]));
			fake->combat.push_back(records[index].payload.combat_summary);
		}
		if (records[index].header.kind == telemetry_record_kind::interval &&
		    records[index].payload.interval.context == telemetry_activity_context::combat)
		{
			fake->saw_combat_context = true;
			fake->saw_idle_combat |= records[index].payload.interval.category ==
						 telemetry_interval_category::connected_idle;
		}
	}
	return result;
}

telemetry_repository_outcome fake_request_stop(void *) noexcept
{
	return telemetry_repository_outcome::stopping;
}

void fake_shutdown(void *context) noexcept
{
	auto *fake = static_cast<fake_repository *>(context);
	++fake->shutdown_calls;
}

bool fake_clock(void *, telemetry_monotonic_usec *monotonic_usec) noexcept
{
	if (monotonic_usec == nullptr)
		return false;
	*monotonic_usec = static_cast<telemetry_monotonic_usec>(
		std::chrono::duration_cast<std::chrono::microseconds>(
			std::chrono::steady_clock::now().time_since_epoch())
			.count());
	return true;
}

telemetry_runtime_options enabled_options()
{
	telemetry_runtime_options options = telemetry_runtime_default_options();
	options.config.enabled = 1U;
	options.config.backend = telemetry_storage_backend::sql;
	options.config.interval_usec = 100U;
	options.config.checkpoint_interval_usec = 200U;
	options.config.active_window_usec = 2'000U;
	options.config.context_segments_per_minute = 8U;
	options.config.pulse_slot_count = 1U;
	assert(telemetry_config_compute_fingerprint(options.config, options.config.fingerprint,
						    sizeof(options.config.fingerprint)));
	options.config.config_id = telemetry_config_id_from_fingerprint(
		options.config.fingerprint, sizeof(options.config.fingerprint));
	return options;
}

void check_disabled_game_path()
{
	assert(telemetry_runtime_init(telemetry_runtime_default_options()) ==
	       telemetry_runtime_outcome::flatfile_disabled);

	char_data player{};
	pc_only_data player_pc{};
	player.only.pc = &player_pc;
	player_pc.pid = 701;
	descriptor_data descriptor{};
	descriptor.connected = CON_PLAYING;

	assert(telemetry_runtime_game_enter(&player, &descriptor).outcome !=
	       telemetry_runtime_outcome::accepted);
	assert(telemetry_runtime_game_context(&player, &descriptor).outcome !=
	       telemetry_runtime_outcome::accepted);
	assert(telemetry_runtime_game_evidence(&player, &descriptor,
					       telemetry_runtime_evidence_kind::player_action)
		       .outcome != telemetry_runtime_outcome::accepted);
	assert(telemetry_runtime_game_session_exit(&player, &descriptor,
						   telemetry_session_end_reason::logout)
		       .outcome != telemetry_runtime_outcome::accepted);
	assert(player.telemetry_session_sequence == 0U);
	assert(descriptor.telemetry_connection_sequence == 0U);
	assert(telemetry_runtime_shutdown({ 0U, 0U, {} }) == telemetry_runtime_outcome::accepted);
	assert(telemetry_runtime_final_reap() == telemetry_runtime_outcome::accepted);
}

void check_environment_options()
{
	unsetenv("TELEMETRY_ENABLED");
	unsetenv("TELEMETRY_BACKEND");
	const telemetry_runtime_options disabled = telemetry_runtime_options_from_environment();
	assert(!disabled.config.enabled);
	assert(disabled.config.backend == telemetry_storage_backend::flatfile_disabled);

	setenv("TELEMETRY_ENABLED", "true", 1);
	setenv("TELEMETRY_BACKEND", "flatfile_disabled", 1);
	setenv("TELEMETRY_BUILD_VERSION", "17", 1);
	const telemetry_runtime_options enabled = telemetry_runtime_options_from_environment();
	assert(!enabled.config.enabled);
	assert(enabled.config.backend == telemetry_storage_backend::flatfile_disabled);
	assert(enabled.config.build_version == 17U);
	unsetenv("TELEMETRY_ENABLED");
	unsetenv("TELEMETRY_BACKEND");
	unsetenv("TELEMETRY_BUILD_VERSION");
}

void check_enabled_game_path()
{
	fake_repository fake{};
	const telemetry_transport_repository_binding repository = { fake_init, fake_apply,
								    fake_request_stop,
								    fake_shutdown, &fake };
	const telemetry_transport_clock_binding clock = { fake_clock, nullptr };
	assert(telemetry_transport_bind_for_tests(&repository, &clock) ==
	       telemetry_transport_outcome::started);
	const telemetry_runtime_options options = enabled_options();
	telemetry_test_start_runtime(options);

	room_data rooms[1]{};
	zone_data zones[1]{};
	rooms[0].zone = 0U;
	zones[0].number = 1701;
	world = rooms;
	zone_table = zones;
	top_of_world = 0;
	top_of_zone_table = 0;

	char_data player{};
	pc_only_data player_pc{};
	player.only.pc = &player_pc;
	player_pc.pid = 702;
	player.in_room = 0;
	player.player.level = 11U;
	player.player.m_class = 3U;
	player.player.race = 4U;
	player.player.racewar = 5U;

	char_data member{};
	pc_only_data member_pc{};
	member.only.pc = &member_pc;
	member_pc.pid = 703;
	group_list member_node{ &member, nullptr };
	group_list group{ &player, &member_node };
	player.group = &group;

	descriptor_data descriptor{};
	descriptor.connected = CON_PLAYING;
	descriptor.character = &player;
	const telemetry_capture_result entered = telemetry_runtime_game_enter(&player, &descriptor);
	assert(entered.outcome == telemetry_runtime_outcome::accepted);
	assert(player.telemetry_session_sequence != 0U);
	assert(descriptor.telemetry_connection_sequence != 0U);
	const std::uint64_t first_connection = descriptor.telemetry_connection_sequence;
	const auto repeated_entry = telemetry_runtime_game_enter(&player, &descriptor);
	assert(repeated_entry.outcome == telemetry_runtime_outcome::accepted);
	assert(repeated_entry.records_emitted == 0U && repeated_entry.records_dropped == 0U);
	assert(descriptor.telemetry_connection_sequence == first_connection);

	assert(telemetry_runtime_game_context(&player, &descriptor).outcome ==
	       telemetry_runtime_outcome::accepted);
	for (const telemetry_runtime_evidence_kind kind :
	     { telemetry_runtime_evidence_kind::movement,
	       telemetry_runtime_evidence_kind::interaction,
	       telemetry_runtime_evidence_kind::communication,
	       telemetry_runtime_evidence_kind::combat_participation })
	{
		assert(telemetry_runtime_game_evidence(&player, &descriptor, kind).outcome ==
		       telemetry_runtime_outcome::accepted);
	}

	assert(telemetry_runtime_game_connection_transition(
		       &player, &descriptor, telemetry_connection_transition_kind::detached)
		       .outcome == telemetry_runtime_outcome::accepted);
	assert(descriptor.telemetry_connection_sequence == 0U);
	assert(telemetry_runtime_game_evidence(&player, nullptr,
					       telemetry_runtime_evidence_kind::linkdead)
		       .outcome == telemetry_runtime_outcome::accepted);

	const telemetry_capture_result reconnected =
		telemetry_runtime_game_presence(&player, &descriptor);
	assert(reconnected.outcome == telemetry_runtime_outcome::accepted);
	assert(descriptor.telemetry_connection_sequence != 0U);
	assert(descriptor.telemetry_connection_sequence != first_connection);
	assert(telemetry_runtime_game_context(&player, &descriptor).outcome ==
	       telemetry_runtime_outcome::accepted);

	assert(telemetry_runtime_game_session_exit(&player, &descriptor,
						   telemetry_session_end_reason::logout)
		       .outcome == telemetry_runtime_outcome::accepted);
	assert(player.telemetry_session_sequence == 0U);
	assert(player.telemetry_session_producer_boot_id == 0U);
	assert(player.telemetry_session_producer_process_id == 0U);
	assert(descriptor.telemetry_connection_sequence == 0U);

	telemetry_monotonic_usec now = 0U;
	telemetry_utc_usec ignored_utc = 0U;
	assert(telemetry_runtime_now(&now, &ignored_utc));
	telemetry_shutdown_request shutdown{};
	shutdown.deadline_monotonic_usec = now + 5'000'000U;
	shutdown.final_flush = 1U;
	assert(telemetry_runtime_shutdown(shutdown) == telemetry_runtime_outcome::accepted);
	assert(telemetry_runtime_final_reap() == telemetry_runtime_outcome::accepted);
	assert(fake.init_calls == 1U);
	assert(fake.applied_records != 0U);
	assert(fake.shutdown_calls == 1U);
	telemetry_transport_unbind_for_tests();
	world = nullptr;
	zone_table = nullptr;
	top_of_world = -1;
	top_of_zone_table = -1;
}

void check_group_generation_and_combat_entry()
{
	fake_repository fake{};
	const telemetry_transport_repository_binding repository = { fake_init, fake_apply,
								    fake_request_stop,
								    fake_shutdown, &fake };
	const telemetry_transport_clock_binding clock = { fake_clock, nullptr };
	assert(telemetry_transport_bind_for_tests(&repository, &clock) ==
	       telemetry_transport_outcome::started);
	telemetry_test_start_runtime(enabled_options());
	room_data rooms[1]{};
	zone_data zones[1]{};
	rooms[0].zone = 0U;
	zones[0].number = 1701;
	world = rooms;
	zone_table = zones;
	top_of_world = top_of_zone_table = 0;
	char_data player{}, member{}, target{}, pet{}, npc{}, other_npc{};
	pc_only_data player_pc{}, member_pc{}, target_pc{};
	npc_only_data pet_npc{}, npc_data{}, other_npc_data{};
	player.only.pc = &player_pc;
	member.only.pc = &member_pc;
	target.only.pc = &target_pc;
	player_pc.pid = 8401;
	member_pc.pid = 8402;
	target_pc.pid = 8403;
	for (P_char pc : { &player, &member, &target })
	{
		pc->in_room = 0;
		pc->player.level = 20U;
	}
	pet.only.npc = &pet_npc;
	npc.only.npc = &npc_data;
	other_npc.only.npc = &other_npc_data;
	pet_npc.idnum = 9411;
	npc_data.idnum = 9412;
	other_npc_data.idnum = 9413;
	for (P_char mob : { &pet, &npc, &other_npc })
	{
		mob->specials.act |= ACT_ISNPC;
		mob->in_room = 0;
		mob->player.level = 20U;
	}
	fixture_pet = &pet;
	fixture_pet_master = &member;
	group_list pet_node{ &pet, nullptr };
	group_list member_node{ &member, &pet_node };
	group_list group{ &player, &member_node };
	player.group = member.group = pet.group = &group;
	auto close = []()
	{
		assert(telemetry_runtime_encounter_close_all(
			       telemetry_encounter_outcome::withdrawal)
			       .outcome == telemetry_runtime_outcome::accepted);
	};
	auto finish = []()
	{
		telemetry_monotonic_usec now{};
		telemetry_utc_usec utc{};
		assert(telemetry_runtime_now(&now, &utc));
		assert(telemetry_runtime_shutdown({ now + 5'000'000U, 1U, {} }) ==
		       telemetry_runtime_outcome::accepted);
		assert(telemetry_runtime_final_reap() == telemetry_runtime_outcome::accepted);
		telemetry_transport_unbind_for_tests();
	};
	assert(telemetry_runtime_game_combat_engage(&player, &target).outcome ==
	       telemetry_runtime_outcome::accepted);
	const auto first_generation = group.telemetry_generation;
	assert(telemetry_producer_id_is_valid(first_generation.producer));
	assert(first_generation.sequence == (TELEMETRY_GROUP_GENERATION_TAG | 1U));
	assert(telemetry_runtime_game_combat_engage(&player, &target).records_emitted == 0U);
	// Appointment changes the character at the head, retaining that group's identity.
	group.ch = &member;
	member_node.ch = &player;
	assert(telemetry_runtime_game_combat_engage(&player, &target).records_emitted == 0U);
	assert(group.telemetry_generation.sequence == first_generation.sequence);
	close();
	// A fresh formal group with the same leader cannot reuse the previous lifetime.
	group_list recreated_member{ &member, &pet_node };
	group_list recreated{ &player, &recreated_member };
	player.group = member.group = pet.group = &recreated;
	assert(telemetry_runtime_game_combat_engage(&player, &npc).outcome ==
	       telemetry_runtime_outcome::accepted);
	const auto recreated_generation = recreated.telemetry_generation;
	assert(recreated_generation.sequence == (TELEMETRY_GROUP_GENERATION_TAG | 2U));
	// The attacked PC is observed even when the accepted attack originates from an NPC.
	assert(telemetry_runtime_game_combat_engage(&npc, &target).outcome ==
	       telemetry_runtime_outcome::accepted);
	close();
	assert(telemetry_runtime_game_combat_engage(&player, &pet).outcome ==
	       telemetry_runtime_outcome::accepted);
	telemetry_runtime_game_combat_damage(&player, &pet, 41U, 0U);
	telemetry_runtime_game_combat_damage(&pet, &player, 9U, 0U);
	close();
	assert(telemetry_runtime_game_combat_engage(&pet, &target).outcome ==
	       telemetry_runtime_outcome::accepted);
	close();
	// NPC storage never supplies a participant PID, including grouped player-owned pets.
	for (P_char mob : { &pet, &npc, &other_npc })
	{
		assert(telemetry_runtime_game_encounter_begin(mob, telemetry_encounter_mode::pve)
			       .outcome == telemetry_runtime_outcome::invalid);
		assert(telemetry_runtime_game_encounter_group_sync(mob).outcome ==
		       telemetry_runtime_outcome::invalid);
		assert(telemetry_runtime_game_encounter_observe(mob).outcome ==
		       telemetry_runtime_outcome::invalid);
		assert(telemetry_runtime_game_encounter_leave(mob,
							      telemetry_encounter_outcome::flee)
			       .outcome == telemetry_runtime_outcome::invalid);
		assert(telemetry_runtime_game_encounter_complete(
			       mob, telemetry_encounter_outcome::success, 1U)
			       .outcome == telemetry_runtime_outcome::invalid);
	}
	assert(telemetry_runtime_game_combat_engage(&pet, &npc).outcome ==
	       telemetry_runtime_outcome::invalid);
	assert(telemetry_runtime_game_combat_engage(&npc, &other_npc).outcome ==
	       telemetry_runtime_outcome::invalid);
	assert(telemetry_runtime_game_combat_engage(&player, &player).outcome ==
	       telemetry_runtime_outcome::invalid);
	assert(telemetry_runtime_game_combat_engage(nullptr, &player).outcome ==
	       telemetry_runtime_outcome::invalid);
	recreated.telemetry_generation = { { 11U, 22U }, first_generation.sequence };
	assert(telemetry_runtime_game_encounter_begin(&player, telemetry_encounter_mode::pve)
		       .outcome == telemetry_runtime_outcome::accepted);
	assert(recreated.telemetry_generation.sequence == (TELEMETRY_GROUP_GENERATION_TAG | 3U));
	close();
	recreated.telemetry_generation.sequence = TELEMETRY_GROUP_GENERATION_TAG;
	assert(telemetry_runtime_game_encounter_begin(&player, telemetry_encounter_mode::pve)
		       .outcome == telemetry_runtime_outcome::accepted);
	assert(recreated.telemetry_generation.sequence == (TELEMETRY_GROUP_GENERATION_TAG | 4U));
	close();
	const auto last_generation = recreated.telemetry_generation;
	finish();
	std::vector<telemetry_encounter_payload> starts;
	for (const auto &row : fake.encounters)
	{
		if (row.participant.subject_id != 0U)
			assert(row.participant.pid >= 8401 && row.participant.pid <= 8403);
		if (row.kind == telemetry_encounter_event_kind::start)
			starts.push_back(row);
	}
	assert(starts.size() == 8U); // Repeated engagement and appointment add no start.
	assert(starts[0].source.group_key == first_generation.sequence &&
	       starts[0].mode == telemetry_encounter_mode::pvp);
	assert(starts[1].source.group_key == 8403U &&
	       starts[1].mode == telemetry_encounter_mode::pvp);
	assert(starts[2].source.group_key == recreated_generation.sequence &&
	       starts[2].mode == telemetry_encounter_mode::pve);
	assert(starts[3].source.group_key == 8403U &&
	       starts[3].mode == telemetry_encounter_mode::pve);
	assert(starts[4].mode == telemetry_encounter_mode::pvp &&
	       starts[5].mode == telemetry_encounter_mode::pvp);
	bool player_damage = false, pet_damage = false;
	for (const auto &row : fake.combat)
	{
		if (row.actor_id == 8401U && row.damage_dealt == 41U)
		{
			assert(row.modifier_flags & TELEMETRY_COMBAT_MODIFIER_PVP);
			assert(row.unique_player_count == 2U && row.participant_count == 3U);
			player_damage = true;
		}
		if (row.actor_kind == telemetry_combat_actor_kind::pet && row.damage_dealt == 9U)
		{
			assert(row.actor_pid == TELEMETRY_UNKNOWN_PID &&
			       row.owner_subject_id == 8402U);
			assert(row.modifier_flags & TELEMETRY_COMBAT_MODIFIER_PVP);
			assert(row.modifier_flags & TELEMETRY_COMBAT_MODIFIER_PET);
			assert(row.unique_player_count == 2U && row.participant_count == 3U);
			pet_damage = true;
		}
	}
	assert(player_damage && pet_damage);
	// A fresh observing producer replaces old in-memory metadata, including copyover reuse.
	fake_repository next_fake{};
	const telemetry_transport_repository_binding next_repository = {
		fake_init, fake_apply, fake_request_stop, fake_shutdown, &next_fake
	};
	assert(telemetry_transport_bind_for_tests(&next_repository, &clock) ==
	       telemetry_transport_outcome::started);
	telemetry_test_start_runtime(enabled_options());
	assert(telemetry_runtime_game_encounter_begin(&player, telemetry_encounter_mode::pve)
		       .outcome == telemetry_runtime_outcome::accepted);
	assert(recreated.telemetry_generation.producer.boot_id !=
		       last_generation.producer.boot_id ||
	       recreated.telemetry_generation.producer.process_id !=
		       last_generation.producer.process_id);
	assert(recreated.telemetry_generation.sequence == first_generation.sequence);
	close();
	finish();
	fixture_pet = fixture_pet_master = nullptr;
	world = nullptr;
	zone_table = nullptr;
	top_of_world = top_of_zone_table = -1;
	std::puts(
		"PASS: formal group generations, both combat sides, NPC guards and pet PvP observations");
}

void check_staggered_checkpoint_cut()
{
	fake_repository fake{};
	const telemetry_transport_repository_binding repository = { fake_init, fake_apply,
								    fake_request_stop,
								    fake_shutdown, &fake };
	const telemetry_transport_clock_binding clock = { fake_clock, nullptr };
	assert(telemetry_transport_bind_for_tests(&repository, &clock) ==
	       telemetry_transport_outcome::started);
	auto options = enabled_options();
	options.config.interval_usec = 60'000'000U;
	options.config.checkpoint_interval_usec = 1'000U;
	options.config.pulse_slot_count = 2U;
	assert(telemetry_config_compute_fingerprint(options.config, options.config.fingerprint,
						    sizeof(options.config.fingerprint)));
	options.config.config_id = telemetry_config_id_from_fingerprint(
		options.config.fingerprint, sizeof(options.config.fingerprint));
	telemetry_test_start_runtime(options);
	char_data player{};
	pc_only_data pc{};
	pc.pid = 704;
	player.only.pc = &pc;
	descriptor_data descriptor{};
	descriptor.connected = CON_PLAYING;
	assert(telemetry_runtime_game_enter(&player, &descriptor).outcome ==
	       telemetry_runtime_outcome::accepted);
	std::this_thread::sleep_for(std::chrono::milliseconds(2));
	telemetry_monotonic_usec now = 0U;
	telemetry_utc_usec utc = 0;
	assert(telemetry_runtime_now(&now, &utc));
	// The first allocated activity slot is in bucket zero. A pulse for the
	// other bucket must not advance its session checkpoint past the classifier.
	const auto skipped = telemetry_runtime_pulse({ now, utc, 1U, 0U });
	assert(skipped.sessions_considered == 0U);
	assert(skipped.checkpoints_sealed == 0U);
	assert(telemetry_runtime_game_evidence(&player, &descriptor,
					       telemetry_runtime_evidence_kind::player_action)
		       .outcome == telemetry_runtime_outcome::accepted);
	assert(telemetry_runtime_game_handoff_copy(&player).outcome ==
	       telemetry_runtime_outcome::accepted);
	assert(telemetry_runtime_now(&now, &utc));
	assert(telemetry_runtime_shutdown({ now + 5'000'000U, 1U, {} }) ==
	       telemetry_runtime_outcome::accepted);
	assert(telemetry_runtime_final_reap() == telemetry_runtime_outcome::accepted);
	telemetry_transport_unbind_for_tests();
}

void check_combat_and_afk_context()
{
	fake_repository fake{};
	const telemetry_transport_repository_binding repository = { fake_init, fake_apply,
								    fake_request_stop,
								    fake_shutdown, &fake };
	const telemetry_transport_clock_binding clock = { fake_clock, nullptr };
	assert(telemetry_transport_bind_for_tests(&repository, &clock) ==
	       telemetry_transport_outcome::started);
	telemetry_runtime_options options = enabled_options();
	// The test finishes well inside the active window. AFK, not expiry, must
	// end active time. Tiny real waits only ensure nonzero interval durations.
	options.config.interval_usec = 60'000'000U;
	options.config.checkpoint_interval_usec = 60'000'000U;
	options.config.active_window_usec = 60'000'000U;
	// Leave headroom for repeated handoff flushes; this is not a budget test.
	options.config.context_segments_per_minute = 64U;
	assert(telemetry_config_compute_fingerprint(options.config, options.config.fingerprint,
						    sizeof(options.config.fingerprint)));
	options.config.config_id = telemetry_config_id_from_fingerprint(
		options.config.fingerprint, sizeof(options.config.fingerprint));
	telemetry_test_start_runtime(options);
	char_data player{}, opponent{};
	pc_only_data pc{};
	player.only.pc = &pc;
	pc.pid = 704;
	player.in_room = -1;
	descriptor_data descriptor{};
	descriptor.connected = CON_PLAYING;
	assert(telemetry_runtime_game_enter(&player, &descriptor).outcome ==
	       telemetry_runtime_outcome::accepted);
	player.specials.fighting = &opponent;
	assert(telemetry_runtime_game_context(&player, &descriptor).outcome ==
	       telemetry_runtime_outcome::accepted);
	// A failed copyover leaves this process playing. Repeated snapshot attempts
	// must not advance session counters beyond the activity classifier's cut.
	for (unsigned attempt = 0; attempt < 8; ++attempt)
	{
		std::this_thread::sleep_for(std::chrono::milliseconds(1));
		assert(telemetry_runtime_game_handoff_copy(&player).outcome ==
		       telemetry_runtime_outcome::accepted);
		assert(telemetry_runtime_game_context(&player, &descriptor).outcome ==
		       telemetry_runtime_outcome::accepted);
	}
	// Being attacked/auto-fighting alone describes context, not human activity.
	std::this_thread::sleep_for(std::chrono::milliseconds(1));
	telemetry_monotonic_usec passive_end = 0;
	telemetry_utc_usec observed_utc = 0;
	assert(telemetry_runtime_now(&passive_end, &observed_utc));
	assert(telemetry_runtime_game_evidence(&player, &descriptor,
					       telemetry_runtime_evidence_kind::player_action)
		       .outcome == telemetry_runtime_outcome::accepted);
	std::this_thread::sleep_for(std::chrono::milliseconds(1));
	player.specials.act |= PLR_AFK;
	assert(telemetry_runtime_game_context(&player, &descriptor).outcome ==
	       telemetry_runtime_outcome::accepted);
	telemetry_monotonic_usec afk_cut = 0;
	assert(telemetry_runtime_now(&afk_cut, &observed_utc));
	std::this_thread::sleep_for(std::chrono::milliseconds(1));
	player.specials.fighting = nullptr;
	assert(telemetry_runtime_game_context(&player, &descriptor).outcome ==
	       telemetry_runtime_outcome::accepted);
	assert(telemetry_runtime_game_session_exit(&player, &descriptor,
						   telemetry_session_end_reason::logout)
		       .outcome == telemetry_runtime_outcome::accepted);
	telemetry_monotonic_usec now = 0;
	telemetry_utc_usec utc = 0;
	assert(telemetry_runtime_now(&now, &utc));
	assert(telemetry_runtime_shutdown({ now + 5'000'000U, 1U, {} }) ==
	       telemetry_runtime_outcome::accepted);
	assert(telemetry_runtime_final_reap() == telemetry_runtime_outcome::accepted);
	telemetry_transport_unbind_for_tests();
	bool afk_stopped_activity = true;
	bool idle_increased = false;
	for (const auto &interval : fake.intervals)
	{
		if (interval.category == telemetry_interval_category::connected_active)
		{
			assert(interval.window.start_monotonic_usec >= passive_end);
			if (interval.window.end_monotonic_usec > afk_cut)
				afk_stopped_activity = false;
		}
		if (interval.window.end_monotonic_usec > afk_cut &&
		    interval.category == telemetry_interval_category::connected_idle &&
		    interval.context == telemetry_activity_context::combat)
			idle_increased = true;
	}
	std::fprintf(
		stderr,
		"context evidence: combat=%u idle_combat=%u afk_stops_active=%u idle_grows=%u\n",
		fake.saw_combat_context, fake.saw_idle_combat, afk_stopped_activity,
		idle_increased);
	assert(fake.saw_combat_context && fake.saw_idle_combat);
	assert(afk_stopped_activity && idle_increased);
}
void check_resume_capacity_rollback()
{
	fake_repository fake{};
	const telemetry_transport_repository_binding repository = { fake_init, fake_apply,
								    fake_request_stop,
								    fake_shutdown, &fake };
	const telemetry_transport_clock_binding clock = { fake_clock, nullptr };
	assert(telemetry_transport_bind_for_tests(&repository, &clock) ==
	       telemetry_transport_outcome::started);
	auto options = enabled_options();
	telemetry_test_start_runtime(options);
	constexpr auto count = TELEMETRY_SESSION_STATE_MAX_SLOTS + 1;
	auto players = std::make_unique<char_data[]>(count);
	auto pcs = std::make_unique<pc_only_data[]>(count);
	auto descriptors = std::make_unique<descriptor_data[]>(count);
	for (std::size_t i = 0; i < count; ++i)
	{
		players[i].only.pc = &pcs[i];
		pcs[i].pid = 8000 + i;
		players[i].in_room = -1;
		descriptors[i].connected = CON_PLAYING;
		descriptors[i].character = &players[i];
	}
	for (std::size_t i = 0; i < count - 1; ++i)
	{
		const auto entered = telemetry_runtime_game_enter(&players[i], &descriptors[i]);
		assert(entered.outcome == telemetry_runtime_outcome::accepted);
	}
	auto &player = players[count - 1];
	auto &descriptor = descriptors[count - 1];
	const auto resumed = telemetry_runtime_game_session_resume(&player, &descriptor, nullptr);
	const auto handoff = telemetry_runtime_game_handoff_copy(&player);
	const auto evidence = telemetry_runtime_game_evidence(
		&player, &descriptor, telemetry_runtime_evidence_kind::player_action);
	std::printf("full_slots=%zu resume_outcome=%u session_id=%llu connection_id=%llu "
		    "handoff_outcome=%u evidence_outcome=%u\n",
		    count - 1, static_cast<unsigned>(resumed.outcome),
		    static_cast<unsigned long long>(player.telemetry_session_sequence),
		    static_cast<unsigned long long>(descriptor.telemetry_connection_sequence),
		    static_cast<unsigned>(handoff.outcome),
		    static_cast<unsigned>(evidence.outcome));
	const bool phantom_ids = player.telemetry_session_sequence != 0 ||
				 descriptor.telemetry_connection_sequence != 0;
	// Independently exercise normal login after the refused resume fixture.
	descriptor.telemetry_resume_pending = 0U;
	descriptor.telemetry_pending_handoff = {};
	const auto entered = telemetry_runtime_game_enter(&player, &descriptor);
	const bool enter_phantom_ids = player.telemetry_session_sequence != 0 ||
				       descriptor.telemetry_connection_sequence != 0;
	assert(telemetry_runtime_game_session_exit(&players[0], &descriptors[0],
						   telemetry_session_end_reason::logout)
		       .outcome == telemetry_runtime_outcome::accepted);
	const auto recovered = telemetry_runtime_game_presence(&player, &descriptor);
	const auto recovered_evidence = telemetry_runtime_game_evidence(
		&player, &descriptor, telemetry_runtime_evidence_kind::player_action);
	telemetry_monotonic_usec now = 0;
	telemetry_utc_usec utc = 0;
	assert(telemetry_runtime_now(&now, &utc));
	assert(telemetry_runtime_shutdown({ now + 5'000'000U, 1U, {} }) ==
	       telemetry_runtime_outcome::accepted);
	assert(telemetry_runtime_final_reap() == telemetry_runtime_outcome::accepted);
	telemetry_transport_unbind_for_tests();
	assert(resumed.outcome == telemetry_runtime_outcome::queue_full);
	assert(!phantom_ids);
	assert(entered.outcome == telemetry_runtime_outcome::queue_full);
	assert(!enter_phantom_ids);
	assert(recovered.outcome == telemetry_runtime_outcome::accepted);
	assert(recovered_evidence.outcome == telemetry_runtime_outcome::accepted);
}

std::atomic<bool> startup_entered{ false };
std::atomic<bool> release_startup{ false };
telemetry_repository_outcome delayed_init(void *context,
					  telemetry_repository_config config) noexcept
{
	startup_entered.store(true);
	while (!release_startup.load())
		std::this_thread::sleep_for(std::chrono::milliseconds(1));
	return fake_init(context, config);
}

void check_deferred_startup_presence()
{
	fake_repository fake{};
	const telemetry_transport_repository_binding repository = { delayed_init, fake_apply,
								    fake_request_stop,
								    fake_shutdown, &fake };
	const telemetry_transport_clock_binding clock = { fake_clock, nullptr };
	assert(telemetry_transport_bind_for_tests(&repository, &clock) ==
	       telemetry_transport_outcome::started);
	const auto options = enabled_options();
	assert(telemetry_runtime_init(options) == telemetry_runtime_outcome::accepted);
	const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(2);
	while (!startup_entered.load() && std::chrono::steady_clock::now() < deadline)
		std::this_thread::sleep_for(std::chrono::milliseconds(1));
	assert(startup_entered.load());
	assert(telemetry_runtime_health_copy().state != telemetry_health_state::healthy);
	constexpr std::size_t count = 11;
	auto players = std::make_unique<char_data[]>(count);
	auto pcs = std::make_unique<pc_only_data[]>(count);
	auto descriptors = std::make_unique<descriptor_data[]>(count);
	for (std::size_t i = 0; i < count; ++i)
	{
		players[i].only.pc = &pcs[i];
		pcs[i].pid = 8200 + i;
		players[i].in_room = -1;
		descriptors[i].connected = CON_PLAYING;
		descriptors[i].character = &players[i];
	}
	char_data switched_npc{};
	switched_npc.specials.act = ACT_ISNPC;
	descriptors[5].character = &switched_npc;
	descriptors[5].original = &players[5];
	descriptors[6].connected = CON_RMOTD;
	players[7].telemetry_session_producer_boot_id = 42U; // partial IDs must not be replaced
	for (std::size_t i : { 0U, 1U, 2U, 5U })
	{
		assert(telemetry_runtime_game_enter(&players[i], &descriptors[i]).outcome ==
		       telemetry_runtime_outcome::queue_full);
		assert(players[i].telemetry_session_sequence == 0U);
		assert(descriptors[i].telemetry_connection_sequence == 0U);
	}
	telemetry_session_handoff handoff{};
	handoff.session = { { { 41U, 43U }, 7U },
			    8203U,
			    8203U,
			    options.config.season_id,
			    options.config.environment_id };
	handoff.previous_producer = { 41U, 43U };
	handoff.last_checkpoint_revision = 5U;
	handoff.cumulative = { 100U, 0U, 100U, 0U, 100U, 0U };
	assert(telemetry_runtime_game_session_resume(&players[3], &descriptors[3], &handoff)
		       .outcome == telemetry_runtime_outcome::queue_full);
	assert(telemetry_runtime_game_session_resume(&players[4], &descriptors[4], nullptr)
		       .outcome == telemetry_runtime_outcome::queue_full);
	assert(descriptors[3].telemetry_resume_pending == 2U);
	assert(descriptors[4].telemetry_resume_pending == 1U);
	assert(players[3].telemetry_session_sequence == 0U &&
	       descriptors[3].telemetry_connection_sequence == 0U);
	telemetry_session_handoff cancelled = handoff;
	cancelled.session.pid = 8209;
	cancelled.session.subject_id = 8209U;
	assert(telemetry_runtime_game_session_resume(&players[9], &descriptors[9], &cancelled)
		       .outcome == telemetry_runtime_outcome::queue_full);
	(void)telemetry_runtime_game_connection_transition(
		&players[9], &descriptors[9], telemetry_connection_transition_kind::detached);
	assert(descriptors[9].telemetry_resume_pending == 0U);
	assert(descriptors[9].telemetry_pending_handoff.session.id.session_seq == 0U);
	assert(telemetry_runtime_game_session_resume(&players[10], &descriptors[10], nullptr)
		       .outcome == telemetry_runtime_outcome::queue_full);
	(void)telemetry_runtime_game_session_exit(&players[10], &descriptors[10],
						  telemetry_session_end_reason::logout);
	assert(descriptors[10].telemetry_resume_pending == 0U);
	assert(telemetry_runtime_game_evidence(&players[8], nullptr,
					       telemetry_runtime_evidence_kind::linkdead)
		       .outcome == telemetry_runtime_outcome::invalid);
	release_startup.store(true);
	const auto ready_deadline = std::chrono::steady_clock::now() + std::chrono::seconds(2);
	while (telemetry_runtime_health_copy().state != telemetry_health_state::healthy &&
	       std::chrono::steady_clock::now() < ready_deadline)
		std::this_thread::sleep_for(std::chrono::milliseconds(1));
	assert(telemetry_runtime_health_copy().state == telemetry_health_state::healthy);
	telemetry_monotonic_usec ready_cut = 0U;
	telemetry_utc_usec utc = TELEMETRY_UTC_UNKNOWN;
	assert(telemetry_runtime_now(&ready_cut, &utc));
	assert(telemetry_runtime_game_presence(&players[0], &descriptors[0]).outcome ==
	       telemetry_runtime_outcome::accepted);
	assert(telemetry_runtime_game_evidence(&players[1], &descriptors[1],
					       telemetry_runtime_evidence_kind::player_action)
		       .outcome == telemetry_runtime_outcome::accepted);
	assert(telemetry_runtime_game_context(&players[2], &descriptors[2]).outcome ==
	       telemetry_runtime_outcome::accepted);
	for (std::size_t i : { 3U, 4U, 5U })
		assert(telemetry_runtime_game_presence(&players[i], &descriptors[i]).outcome ==
		       telemetry_runtime_outcome::accepted);
	for (std::size_t i = 0; i < 6; ++i)
	{
		const auto session = players[i].telemetry_session_sequence;
		const auto connection = descriptors[i].telemetry_connection_sequence;
		assert(session != 0U && connection != 0U &&
		       descriptors[i].telemetry_resume_pending == 0U);
		for (unsigned retry = 0; retry < 3; ++retry)
		{
			const auto result =
				telemetry_runtime_game_presence(&players[i], &descriptors[i]);
			assert(result.outcome == telemetry_runtime_outcome::accepted);
			assert(result.records_emitted == 0U && result.records_dropped == 0U);
			assert(players[i].telemetry_session_sequence == session);
			assert(descriptors[i].telemetry_connection_sequence == connection);
		}
	}
	for (std::size_t i : { 6U, 7U })
		assert(telemetry_runtime_game_presence(&players[i], &descriptors[i]).outcome ==
		       telemetry_runtime_outcome::invalid);
	assert(telemetry_runtime_game_presence(&switched_npc, &descriptors[5]).outcome ==
	       telemetry_runtime_outcome::invalid);
	assert(players[7].telemetry_session_producer_boot_id == 42U);
	const auto retained = telemetry_runtime_game_handoff_copy(&players[3]);
	assert(retained.outcome == telemetry_runtime_outcome::accepted);
	assert(retained.handoff.session.id.session_seq == handoff.session.id.session_seq);
	assert(retained.handoff.session.id.producer.boot_id == handoff.session.id.producer.boot_id);
	assert(retained.handoff.cumulative.connected_usec >= handoff.cumulative.connected_usec);
	assert(retained.handoff.last_checkpoint_revision >= handoff.last_checkpoint_revision);
	std::this_thread::sleep_for(std::chrono::milliseconds(2));
	telemetry_monotonic_usec now = 0U;
	assert(telemetry_runtime_now(&now, &utc));
	(void)telemetry_runtime_pulse({ now, utc, 0U, 0U });
	assert(telemetry_runtime_shutdown({ now + 5'000'000U, 1U, {} }) ==
	       telemetry_runtime_outcome::accepted);
	assert(telemetry_runtime_final_reap() == telemetry_runtime_outcome::accepted);
	telemetry_transport_unbind_for_tests();
	unsigned entries[6]{};
	bool saw_presence = false, saw_active = false, saw_absent_quality = false;
	for (const auto &row : fake.lifecycles)
	{
		assert(row.session.pid >= 8200 && row.session.pid < 8206);
		assert(row.at_monotonic_usec >= ready_cut);
		if (row.lifecycle == telemetry_lifecycle_kind::session_entered ||
		    row.lifecycle == telemetry_lifecycle_kind::connection_attached)
			++entries[row.session.pid - 8200U];
		if (row.session.pid == 8204U)
			saw_absent_quality |=
				(row.quality_flags & TELEMETRY_QUALITY_UNCLOSED_TAIL) != 0U;
	}
	for (unsigned observed : entries)
		assert(observed == 1U);
	for (const auto &row : fake.intervals)
	{
		assert(row.session.pid >= 8200 && row.session.pid < 8206);
		assert(row.window.start_monotonic_usec >= ready_cut);
		if (row.session.pid == 8200U)
		{
			assert(row.category != telemetry_interval_category::connected_active);
			saw_presence = true;
		}
		if (row.session.pid == 8201U &&
		    row.category == telemetry_interval_category::connected_active)
			saw_active = true;
	}
	std::fprintf(stderr, "startup evidence: presence=%u active=%u absent_quality=%u\n",
		     saw_presence, saw_active, saw_absent_quality);
	assert(saw_presence && saw_active && saw_absent_quality);
	std::puts(
		"PASS: deferred startup captures presence and preserves copyover without fabricated time");
}

static std::atomic<bool> worker_entered{ false };
static std::atomic<bool> release_worker{ false };
static telemetry_apply_batch_result blocking_apply(void *context, const telemetry_record *records,
						   std::size_t count) noexcept
{
	worker_entered.store(true);
	while (!release_worker.load())
		std::this_thread::sleep_for(std::chrono::milliseconds(1));
	return fake_apply(context, records, count);
}
void check_resume_queue_pressure()
{
	fake_repository fake{};
	const telemetry_transport_repository_binding repository = { fake_init, blocking_apply,
								    fake_request_stop,
								    fake_shutdown, &fake };
	const telemetry_transport_clock_binding clock = { fake_clock, nullptr };
	assert(telemetry_transport_bind_for_tests(&repository, &clock) ==
	       telemetry_transport_outcome::started);
	auto options = enabled_options();
	telemetry_test_start_runtime(options);
	const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(5);
	while (!worker_entered.load() && std::chrono::steady_clock::now() < deadline)
		std::this_thread::sleep_for(std::chrono::milliseconds(1));
	assert(worker_entered.load());
	char_data churn{};
	pc_only_data churn_pc{};
	churn.only.pc = &churn_pc;
	churn_pc.pid = 8100;
	churn.in_room = -1;
	descriptor_data churn_descriptor{};
	churn_descriptor.connected = CON_PLAYING;
	unsigned iterations = 0;
	bool full = false;
	for (; iterations < 8192; ++iterations)
	{
		const auto entered = telemetry_runtime_game_enter(&churn, &churn_descriptor);
		(void)telemetry_runtime_game_session_exit(&churn, &churn_descriptor,
							  telemetry_session_end_reason::logout);
		if (entered.records_dropped != 0)
		{
			full = true;
			break;
		}
	}
	char_data player{};
	pc_only_data pc{};
	player.only.pc = &pc;
	pc.pid = 8101;
	player.in_room = -1;
	acct_chars owner_member{};
	owner_member.pid = pc.pid;
	acct_entry owner_account{};
	owner_account.acct_character_list = &owner_member;
	owner_account.telemetry_account_token = 33U;
	owner_account.telemetry_environment_id = options.config.environment_id;
	owner_account.telemetry_season_id = options.config.season_id;
	descriptor_data descriptor{};
	descriptor.connected = CON_PLAYING;
	descriptor.character = &player;
	descriptor.account = &owner_account;
	const auto resumed = telemetry_runtime_game_session_resume(&player, &descriptor, nullptr);
	const bool retained_ids = player.telemetry_session_sequence != 0 &&
				  descriptor.telemetry_connection_sequence != 0;
	char_data login_player{};
	pc_only_data login_pc{};
	login_player.only.pc = &login_pc;
	login_pc.pid = 8102;
	login_player.in_room = -1;
	acct_chars login_member{};
	login_member.pid = login_pc.pid;
	owner_member.next = &login_member;
	descriptor_data login_descriptor{};
	login_descriptor.connected = CON_PLAYING;
	login_descriptor.character = &login_player;
	login_descriptor.account = &owner_account;
	const auto login = telemetry_runtime_game_enter(&login_player, &login_descriptor);
	const bool login_retained_ids = login_player.telemetry_session_sequence != 0 &&
					login_descriptor.telemetry_connection_sequence != 0;
	release_worker.store(true);
	const auto drain_deadline = std::chrono::steady_clock::now() + std::chrono::seconds(5);
	while (telemetry_transport_health_copy().queue_depth != 0 &&
	       std::chrono::steady_clock::now() < drain_deadline)
		std::this_thread::sleep_for(std::chrono::milliseconds(1));
	telemetry_monotonic_usec recovery_cut{};
	telemetry_utc_usec recovery_utc{};
	assert(telemetry_runtime_now(&recovery_cut, &recovery_utc));
	// Admit newer keys before retrying either retained ownership boundary.
	// The real transport rejects stale keys even when SQL would accept them.
	assert(telemetry_runtime_game_enter(&churn, &churn_descriptor).outcome ==
	       telemetry_runtime_outcome::accepted);
	(void)telemetry_runtime_game_session_exit(&churn, &churn_descriptor,
						  telemetry_session_end_reason::logout);
	// Queue loss must not strand an admitted classifier/session pair.
	const auto evidence = telemetry_runtime_game_evidence(
		&player, &descriptor, telemetry_runtime_evidence_kind::player_action);
	const auto login_evidence = telemetry_runtime_game_evidence(
		&login_player, &login_descriptor, telemetry_runtime_evidence_kind::player_action);
	std::printf(
		"queue_saturated=%u churn=%u resume_outcome=%u dropped=%u retained_ids=%u evidence_outcome=%u\n",
		full, iterations, static_cast<unsigned>(resumed.outcome), resumed.records_dropped,
		retained_ids, static_cast<unsigned>(evidence.outcome));
	telemetry_monotonic_usec now = 0;
	telemetry_utc_usec utc = 0;
	assert(telemetry_runtime_now(&now, &utc));
	assert(telemetry_runtime_shutdown({ now + 5'000'000U, 1U, {} }) ==
	       telemetry_runtime_outcome::accepted);
	assert(telemetry_runtime_final_reap() == telemetry_runtime_outcome::accepted);
	telemetry_transport_unbind_for_tests();
	const bool pass = full && retained_ids &&
			  resumed.outcome == telemetry_runtime_outcome::queue_full &&
			  resumed.records_dropped > 0 &&
			  evidence.outcome == telemetry_runtime_outcome::accepted;
	std::puts(pass ? "PASS: admitted session survives explicit lifecycle queue loss" :
			 "FAIL: admitted queue-loss resume lost session continuity");
	assert(pass);
	assert(login_retained_ids && login.outcome == telemetry_runtime_outcome::queue_full &&
	       login.records_dropped > 0 &&
	       login_evidence.outcome == telemetry_runtime_outcome::accepted);
	assert(fake.ownership.size() == 2U);
	for (const auto &owner : fake.ownership)
	{
		assert(owner.account_token == 33U);
		assert(owner.session.pid == 8101 || owner.session.pid == 8102);
		assert(owner.at_monotonic_usec < recovery_cut);
	}
}

void check_authenticated_ownership_path()
{
	fake_repository fake{};
	const telemetry_transport_repository_binding repository = { fake_init, fake_apply,
								    fake_request_stop,
								    fake_shutdown, &fake };
	const telemetry_transport_clock_binding clock = { fake_clock, nullptr };
	assert(telemetry_transport_bind_for_tests(&repository, &clock) ==
	       telemetry_transport_outcome::started);
	const auto options = enabled_options();
	telemetry_test_start_runtime(options);
	char_data player{};
	pc_only_data pc{};
	player.only.pc = &pc;
	pc.pid = 8300;
	player.in_room = -1;
	acct_chars member{};
	member.pid = pc.pid;
	acct_entry account{};
	account.acct_character_list = &member;
	account.telemetry_account_token = 11U;
	account.telemetry_environment_id = options.config.environment_id;
	account.telemetry_season_id = options.config.season_id;
	descriptor_data descriptor{};
	descriptor.character = &player;
	descriptor.account = &account;
	descriptor.connected = CON_PLAYING;
	assert(telemetry_runtime_game_enter(&player, &descriptor).outcome ==
	       telemetry_runtime_outcome::accepted);
	const auto session = player.telemetry_session_sequence;
	const auto first_connection = descriptor.telemetry_connection_sequence;
	assert(telemetry_runtime_game_presence(&player, &descriptor).records_emitted == 0U);
	auto observe = [&]()
	{
		std::this_thread::sleep_for(std::chrono::milliseconds(1));
		const auto result = telemetry_runtime_game_presence(&player, &descriptor);
		assert(result.outcome == telemetry_runtime_outcome::accepted &&
		       result.records_emitted == 1U);
	};
	account.telemetry_account_token = 12U;
	observe();
	account.telemetry_environment_id = options.config.environment_id + 1U;
	observe();
	account.telemetry_environment_id = options.config.environment_id;
	observe();
	member.pid = pc.pid + 1;
	observe();
	member.pid = pc.pid;
	observe();
	member.next = &member; // A cyclic list must stay bounded and cannot establish ownership.
	observe();
	member.next = nullptr;
	observe();
	account.acct_blocked = ACCOUNT_BLOCK_DELETION;
	observe();
	account.acct_blocked = 0;
	observe();
	assert(telemetry_runtime_game_connection_transition(
		       &player, &descriptor, telemetry_connection_transition_kind::detached)
		       .outcome == telemetry_runtime_outcome::accepted);
	std::this_thread::sleep_for(std::chrono::milliseconds(1));
	assert(telemetry_runtime_game_connection_transition(
		       &player, &descriptor, telemetry_connection_transition_kind::attached)
		       .outcome == telemetry_runtime_outcome::accepted);
	assert(descriptor.telemetry_connection_sequence != first_connection);
	const auto handoff = telemetry_runtime_game_handoff_copy(&player);
	assert(handoff.outcome == telemetry_runtime_outcome::accepted);
	assert(handoff.handoff.ownership.account_token == 12U);
	assert(handoff.handoff.ownership.source == telemetry_ownership_source::reconnect);
	telemetry_monotonic_usec now{};
	telemetry_utc_usec utc{};
	assert(telemetry_runtime_now(&now, &utc));
	assert(telemetry_runtime_shutdown({ now + 5'000'000U, 1U, {} }) ==
	       telemetry_runtime_outcome::accepted);
	assert(telemetry_runtime_final_reap() == telemetry_runtime_outcome::accepted);
	telemetry_transport_unbind_for_tests();
	assert(fake.ownership.size() == 11U);
	const std::uint64_t tokens[] = { 11U, 12U, 0U, 12U, 0U, 12U, 0U, 12U, 0U, 12U, 12U };
	for (std::size_t index = 0U; index < fake.ownership.size(); ++index)
	{
		const auto &row = fake.ownership[index];
		assert(row.session.id.session_seq == session && row.session.pid == pc.pid);
		assert(row.account_token == tokens[index]);
		if (index)
			assert(row.at_monotonic_usec >
			       fake.ownership[index - 1U].at_monotonic_usec);
		const auto expected =
			index == 0U	  ? telemetry_ownership_source::authenticated_login :
			index == 10U	  ? telemetry_ownership_source::reconnect :
			row.account_token ? telemetry_ownership_source::ownership_changed :
					    telemetry_ownership_source::unavailable;
		assert(row.source == expected);
	}
	assert(fake.ownership[6].quality_flags & TELEMETRY_QUALITY_CARDINALITY_OVERFLOW);
	fake_repository next_fake{};
	const telemetry_transport_repository_binding next_repository = {
		fake_init, fake_apply, fake_request_stop, fake_shutdown, &next_fake
	};
	assert(telemetry_transport_bind_for_tests(&next_repository, &clock) ==
	       telemetry_transport_outcome::started);
	const auto next_options = enabled_options();
	telemetry_test_start_runtime(next_options);
	char_data recovered{};
	recovered.only.pc = &pc;
	recovered.in_room = -1;
	descriptor_data next_descriptor{};
	next_descriptor.character = &recovered;
	next_descriptor.account = &account;
	next_descriptor.connected = CON_PLAYING;
	account.telemetry_account_token =
		13U; // Real reloaded authority may differ from the old observation.
	assert(telemetry_runtime_game_session_resume(&recovered, &next_descriptor, &handoff.handoff)
		       .outcome == telemetry_runtime_outcome::accepted);
	assert(telemetry_runtime_now(&now, &utc));
	assert(telemetry_runtime_shutdown({ now + 5'000'000U, 1U, {} }) ==
	       telemetry_runtime_outcome::accepted);
	assert(telemetry_runtime_final_reap() == telemetry_runtime_outcome::accepted);
	telemetry_transport_unbind_for_tests();
	assert(next_fake.ownership.size() == 1U);
	const auto &resumed_owner = next_fake.ownership[0];
	assert(resumed_owner.account_token == 13U &&
	       resumed_owner.source == telemetry_ownership_source::copyover);
	assert(resumed_owner.session.id.producer.boot_id ==
	       handoff.handoff.session.id.producer.boot_id);
	assert(resumed_owner.connection.producer.boot_id !=
		       handoff.handoff.previous_producer.boot_id ||
	       resumed_owner.connection.producer.process_id !=
		       handoff.handoff.previous_producer.process_id);
	for (unsigned missing = 0U; missing < 3U; ++missing)
	{
		fake_repository missing_fake{};
		const telemetry_transport_repository_binding missing_repository = {
			fake_init, fake_apply, fake_request_stop, fake_shutdown, &missing_fake
		};
		assert(telemetry_transport_bind_for_tests(&missing_repository, &clock) ==
		       telemetry_transport_outcome::started);
		telemetry_test_start_runtime(enabled_options());
		char_data missing_player{};
		missing_player.only.pc = &pc;
		missing_player.in_room = -1;
		descriptor_data missing_descriptor{};
		missing_descriptor.character = &missing_player;
		missing_descriptor.account = &account;
		missing_descriptor.connected = CON_PLAYING;
		account.telemetry_account_token = missing == 0U ? 0U : 13U;
		account.telemetry_environment_id =
			options.config.environment_id + (missing == 1U ? 1U : 0U);
		account.acct_blocked = missing == 2U ? ACCOUNT_BLOCK_DELETION : 0;
		assert(telemetry_runtime_game_session_resume(&missing_player, &missing_descriptor,
							     &handoff.handoff)
			       .outcome == telemetry_runtime_outcome::accepted);
		assert(telemetry_runtime_now(&now, &utc));
		assert(telemetry_runtime_shutdown({ now + 5'000'000U, 1U, {} }) ==
		       telemetry_runtime_outcome::accepted);
		assert(telemetry_runtime_final_reap() == telemetry_runtime_outcome::accepted);
		telemetry_transport_unbind_for_tests();
		assert(missing_fake.ownership.size() == 1U);
		assert(missing_fake.ownership[0].account_token == 0U);
		assert(missing_fake.ownership[0].source == telemetry_ownership_source::unavailable);
	}
	std::puts(
		"PASS: authenticated ownership observes scoped caches, transfers, unavailable identity and reconnect");
}

} // namespace

int main()
{
	check_group_generation_and_combat_entry();
	check_authenticated_ownership_path();
	check_deferred_startup_presence();
	check_resume_capacity_rollback();
	check_resume_queue_pressure();
	check_environment_options();
	check_disabled_game_path();
	check_enabled_game_path();
	check_staggered_checkpoint_cut();
	check_combat_and_afk_context();
	std::puts("telemetry gameplay adapter paths passed");
	return 0;
}
