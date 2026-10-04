#include "telemetry/telemetry_config_private.h"
#include "telemetry/telemetry_runtime.h"
#include "telemetry/telemetry_battle.h"
#include "telemetry/telemetry_battle_contract.h"
#include "telemetry/telemetry_battle_contribution.h"
#include "telemetry/telemetry_transport_private.h"
#include "core/structs.h"
#include "core/utils.h"
#include "core/prototypes.h"
#include "magic/spells.h"

#include "telemetry/telemetry_session.h"
#include "telemetry_test_runtime.h"
#include <memory>
#include <limits>
#include <atomic>
#include <cassert>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <thread>
#include <vector>
#include <set>

P_room world = nullptr;
P_char character_list = nullptr;
struct zone_data *zone_table = nullptr;
int top_of_zone_table = -1;
int top_of_world = -1;
P_char fixture_pet = nullptr;
P_char fixture_pet_master = nullptr;
std::vector<affected_type> fixture_control_effects;
std::vector<bool> fixture_control_saves;
std::size_t fixture_control_save_index = 0U;
bool fixture_control_eyeless = false, fixture_control_named_immune = false;
int fixture_control_random = 0;

// Game-service seams for the extracted, unchanged blind/Stun helper bodies.
// Affect mutation is isolated here; the running-server journey remains required.
affected_type *affect_to_char(P_char character, affected_type *effect)
{
	assert(!(effect->flags & AFFTYPE_NOAPPLY));
	character->specials.affected_by |= effect->bitvector;
	character->specials.affected_by2 |= effect->bitvector2;
	fixture_control_effects.push_back(*effect);
	return &fixture_control_effects.back();
}
bool has_innate(P_char, int innate)
{
	assert(innate == INNATE_EYELESS);
	return fixture_control_eyeless;
}
bool isname(const char *name, const char *)
{
	assert(std::strcmp(name, "_noblind_") == 0);
	return fixture_control_named_immune;
}
bool NewSaves(P_char, int type, int)
{
	assert(type == SAVING_FEAR && fixture_control_save_index < fixture_control_saves.size());
	return fixture_control_saves[fixture_control_save_index++];
}
int number(int first, int last)
{
	assert(first <= fixture_control_random && fixture_control_random <= last);
	return fixture_control_random;
}
void send_to_char(const char *, P_char) {}
void act(const char *, int, P_char, P_obj, void *, int) {}
void stop_fighting(P_char character)
{
	assert(IS_AFFECTED2(character, AFF2_STUNNED));
	GET_OPPONENT(character) = nullptr;
	telemetry_runtime_game_combat_context(character);
}
void panic_corruption(const char *, const char *, ...)
{
	std::abort();
}
P_char get_linked_char(P_char character, ush_int link)
{
	return character == fixture_pet && link == LNK_PET ? fixture_pet_master : nullptr;
}

namespace
{
struct fake_repository
{
	bool use_native = false;
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
	std::vector<telemetry_record> battles;
	std::vector<telemetry_record> contributions;
};

telemetry_repository_outcome fake_init(void *context, telemetry_repository_config config) noexcept
{
	auto *fake = static_cast<fake_repository *>(context);
	++fake->init_calls;
	if (fake->use_native)
		return telemetry_repository_init(config);
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
	if (fake->use_native)
	{
		result = telemetry_repository_apply(records, count);
		assert(result.outcome == telemetry_batch_outcome::committed &&
		       result.applied_count == count && result.invalid_count == 0U &&
		       result.quarantined_count == 0U);
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
		if (records[index].header.kind == telemetry_record_kind::battle)
		{
			assert(telemetry_record_is_valid(records[index]));
			fake->battles.push_back(records[index]);
		}
		if (records[index].header.kind == telemetry_record_kind::battle_contribution)
		{
			assert(telemetry_record_is_valid(records[index]));
			fake->contributions.push_back(records[index]);
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

telemetry_repository_outcome fake_request_stop(void *context) noexcept
{
	if (static_cast<fake_repository *>(context)->use_native)
		return telemetry_repository_request_stop();
	return telemetry_repository_outcome::stopping;
}

void fake_shutdown(void *context) noexcept
{
	auto *fake = static_cast<fake_repository *>(context);
	++fake->shutdown_calls;
	if (fake->use_native)
		telemetry_repository_shutdown();
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
	telemetry_battle_actor_context battle_context{};
	battle_context.actor.actor_id = 123U;
	assert(!telemetry_runtime_game_battle_actor(&player, &battle_context));
	assert(battle_context.actor.actor_id == 0U);
	group_list group{ &player, nullptr };
	telemetry_runtime_game_group_changed(&group);
	assert(group.telemetry_generation.sequence == 0U &&
	       group.telemetry_generation.revision == 0U);
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
		mob->runtime_id = allocate_character_runtime_id();
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
	       telemetry_runtime_outcome::accepted);
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

void check_native_battle_context()
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
	const auto producer = telemetry_runtime_producer_copy();
	room_data rooms[3]{};
	zone_data zones[2]{};
	rooms[0].zone = rooms[1].zone = 0U;
	rooms[2].zone = 1U;
	zones[0].number = 1701;
	zones[1].number = 1702;
	world = rooms;
	zone_table = zones;
	top_of_world = 2;
	top_of_zone_table = 1;
	char_data player{}, member{}, npc{}, clone{};
	pc_only_data player_pc{}, member_pc{};
	npc_only_data npc_only{}, clone_only{};
	player.only.pc = &player_pc;
	member.only.pc = &member_pc;
	player_pc.pid = 8501;
	member_pc.pid = 8502;
	npc.only.npc = &npc_only;
	clone.only.npc = &clone_only;
	npc.specials.act = clone.specials.act = ACT_ISNPC;
	npc_only.R_num = clone_only.R_num = 77; // Identical prototype; no legacy instance ID.
	npc.runtime_id = allocate_character_runtime_id();
	clone.runtime_id = allocate_character_runtime_id();
	for (auto *actor : { &player, &member, &npc, &clone })
	{
		actor->in_room = 0;
		actor->player.level = 25;
		actor->player.m_class = 3;
		actor->player.race = 4;
		actor->player.racewar = 5;
	}
	telemetry_battle_actor_context player_context{}, member_context{}, npc_context{},
		clone_context{};
	assert(telemetry_runtime_game_battle_actor(&player, &player_context));
	assert(telemetry_battle_actor_context_is_valid(player_context));
	assert(player_context.actor.actor_id == 8501U && player_context.group_key == 8501U &&
	       player_context.group_revision == 0U && player_context.session.session_seq == 0U &&
	       player_context.encounter.sequence == 0U && player.telemetry_session_sequence == 0U);
	assert(player_context.quality_flags & TELEMETRY_QUALITY_CONTEXT_UNKNOWN);
	assert(telemetry_runtime_game_battle_actor(&npc, &npc_context));
	assert(telemetry_runtime_game_battle_actor(&clone, &clone_context));
	assert(telemetry_battle_actor_context_is_valid(npc_context) &&
	       telemetry_battle_actor_context_is_valid(clone_context));
	assert(npc_context.actor.actor_id ==
		       (TELEMETRY_BATTLE_NPC_GENERATION_TAG | npc.runtime_id) &&
	       npc_context.actor.actor_id != clone_context.actor.actor_id);
	assert(npc_context.group_key == 0U && npc_context.dimensions.group_size == 0U &&
	       npc_context.session.session_seq == 0U && npc_context.encounter.sequence == 0U);
	const auto previous_instance = npc_context.actor.actor_id;
	npc.runtime_id =
		allocate_character_runtime_id(); // Same address reused for the same prototype.
	assert(telemetry_runtime_game_battle_actor(&npc, &npc_context));
	assert(npc_context.actor.actor_id != previous_instance &&
	       npc_context.actor.actor_id != clone_context.actor.actor_id);
	const auto instance = npc.runtime_id;
	for (auto unavailable : { std::uint64_t{ 0U }, TELEMETRY_BATTLE_NPC_GENERATION_TAG,
				  std::numeric_limits<std::uint64_t>::max() })
	{
		npc.runtime_id = unavailable;
		assert(!telemetry_runtime_game_battle_actor(&npc, &npc_context));
		assert(npc_context.actor.actor_id == 0U && npc_context.context_version == 0U);
	}
	npc.runtime_id = instance;
	assert(!telemetry_runtime_game_battle_actor(nullptr, &npc_context));
	assert(!telemetry_runtime_game_battle_actor(&npc, nullptr));
	assert(npc_context.actor.actor_id == 0U);

	descriptor_data descriptor{};
	descriptor.connected = CON_PLAYING;
	descriptor.character = &player;
	player.desc = &descriptor;
	assert(telemetry_runtime_game_enter(&player, &descriptor).outcome ==
	       telemetry_runtime_outcome::accepted);
	assert(telemetry_runtime_game_combat_engage(&player, &npc).outcome ==
	       telemetry_runtime_outcome::accepted);
	assert(telemetry_runtime_game_battle_actor(&player, &player_context));
	assert(player_context.session.session_seq == player.telemetry_session_sequence &&
	       player_context.encounter.sequence != 0U &&
	       player_context.encounter.producer.boot_id == producer.boot_id);
	const auto session = player_context.session;
	assert(telemetry_runtime_game_connection_transition(
		       &player, &descriptor, telemetry_connection_transition_kind::detached)
		       .outcome == telemetry_runtime_outcome::accepted);
	assert(telemetry_runtime_game_battle_actor(&player, &player_context));
	assert(player_context.session.session_seq ==
	       session.session_seq); // Linkdead is the same session.
	assert(telemetry_runtime_game_session_exit(&player, nullptr,
						   telemetry_session_end_reason::logout)
		       .outcome == telemetry_runtime_outcome::accepted);
	player.telemetry_session_sequence = session.session_seq;
	player.telemetry_session_producer_boot_id = session.producer.boot_id;
	player.telemetry_session_producer_process_id = session.producer.process_id;
	assert(telemetry_runtime_game_battle_actor(&player, &player_context));
	assert(player_context.session.session_seq ==
	       0U); // A stale native tuple cannot link closed state.
	player.telemetry_session_sequence = player.telemetry_session_producer_boot_id =
		player.telemetry_session_producer_process_id = 0U;
	assert(telemetry_runtime_game_encounter_leave(&player, telemetry_encounter_outcome::flee)
		       .outcome == telemetry_runtime_outcome::accepted);
	assert(telemetry_runtime_game_battle_actor(&player, &player_context));
	assert(player_context.encounter.sequence == 0U);
	auto battle = std::make_unique<telemetry_battle_state>();
	assert(telemetry_battle_state_init(
		battle.get(), producer,
		{ options.config.environment_id, options.config.season_id, options.config.config_id,
		  options.config.classifier_version, options.config.policy_version, -1, 0U },
		1000U));
	std::vector<telemetry_battle_fact> facts;
	facts.reserve(64);
	const auto sink = [](void *context, const telemetry_battle_fact &fact) noexcept
	{
		static_cast<std::vector<telemetry_battle_fact> *>(context)->push_back(fact);
		return true;
	};

	group_list member_node{ &member, nullptr }, group{ &player, &member_node };
	player.group = member.group = &group;
	telemetry_runtime_game_group_changed(&group);
	assert(group.telemetry_generation.revision == 1U);
	const auto generation = group.telemetry_generation;
	assert(telemetry_runtime_game_battle_group_presence(&player, &member, &player_context,
							    &member_context));
	assert(player_context.group_key == generation.sequence &&
	       player_context.group_key == member_context.group_key &&
	       player_context.group_revision == 1U && member_context.group_revision == 1U);
	const auto party_battle =
		telemetry_battle_observe(battle.get(), telemetry_battle_relation::hostile,
					 player_context, clone_context, 100U, 100U, sink, &facts);
	assert(party_battle.outcome == telemetry_battle_outcome::accepted);
	assert(telemetry_battle_observe(battle.get(), telemetry_battle_relation::group_presence,
					player_context, member_context, 150U, 150U, sink, &facts)
		       .outcome == telemetry_battle_outcome::accepted);
	assert(facts.back().actor_count == 3U && facts.back().observed_owner_count == 2U &&
	       facts.back().side_status == telemetry_battle_side_status::qualified_observed_graph);
	assert(telemetry_battle_close(battle.get(), party_battle.battle,
				      telemetry_battle_close_reason::shutdown, 300U, 300U, sink,
				      &facts)
		       .outcome == telemetry_battle_outcome::accepted);
	bool saw_presence_only = false;
	for (const auto &fact : facts)
		if (fact.kind == telemetry_battle_fact_kind::actor_summary &&
		    fact.actor.actor.actor_id == 8502U)
		{
			assert(fact.roles == TELEMETRY_BATTLE_ROLE_GROUP_PRESENCE &&
			       fact.effort.present_usec == 150U &&
			       fact.effort.contributor_usec == 0U &&
			       fact.actor.session.session_seq == 0U);
			saw_presence_only = true;
		}
	assert(saw_presence_only);
	facts.clear();
	member.in_room = 1; // Same zone and roster does not prove actual presence.
	assert(!telemetry_runtime_game_battle_group_presence(&player, &member, &player_context,
							     &member_context));
	assert(player_context.actor.actor_id == 0U && member_context.actor.actor_id == 0U);
	member.in_room = 0;
	group.ch = &member;
	member_node.ch = &player;
	telemetry_runtime_game_group_changed(&group); // Accepted appointment retains the lifetime.
	assert(group.telemetry_generation.sequence == generation.sequence &&
	       group.telemetry_generation.revision == 2U);
	assert(telemetry_runtime_game_battle_group_presence(&player, &member, &player_context,
							    &member_context));
	assert(player_context.group_revision == 2U && member_context.group_revision == 2U);
	group.telemetry_generation.revision = std::numeric_limits<std::uint16_t>::max();
	telemetry_runtime_game_group_changed(&group);
	telemetry_runtime_game_group_changed(&group);
	assert(group.telemetry_generation.revision == 0U &&
	       group.telemetry_generation.sequence == generation.sequence);
	assert(telemetry_runtime_game_battle_actor(&player, &player_context));
	assert(player_context.group_key == 0U && player_context.group_revision == 0U &&
	       (player_context.quality_flags & TELEMETRY_QUALITY_CONTEXT_OVERFLOW));
	assert(telemetry_battle_actor_context_is_valid(player_context));
	assert(!telemetry_runtime_game_battle_group_presence(&player, &member, &player_context,
							     &member_context));
	// A malformed native list supplies no roster relationship, even with cached metadata.
	member_node.next = &group;
	assert(telemetry_runtime_game_battle_actor(&player, &player_context));
	assert(player_context.group_key == 0U &&
	       (player_context.quality_flags & TELEMETRY_QUALITY_CARDINALITY_OVERFLOW));
	member_node.next = nullptr;
	player.group = member.group = nullptr;

	// Native pet ownership changes keep one live actor; owners get no fabricated PC/session presence.
	fixture_pet = &npc;
	fixture_pet_master = &member;
	member_pc.pid = 0;
	assert(!telemetry_runtime_game_battle_actor(&npc, &npc_context));
	assert(npc_context.actor.actor_id ==
	       0U); // Invalid owner cannot masquerade as ordinary PvE.
	member_pc.pid = 8502;
	assert(telemetry_runtime_game_battle_actor(&npc, &npc_context));
	assert(npc_context.actor.kind == telemetry_combat_actor_kind::pet &&
	       npc_context.actor.owner_subject_id == 8502U &&
	       npc_context.session.session_seq == 0U && npc_context.encounter.sequence == 0U &&
	       npc_context.group_key == 0U);
	assert(telemetry_runtime_game_battle_actor(&player, &player_context));
	const auto joined =
		telemetry_battle_observe(battle.get(), telemetry_battle_relation::hostile,
					 player_context, npc_context, 400U, 400U, sink, &facts);
	assert(joined.outcome == telemetry_battle_outcome::accepted &&
	       facts.back().mode == telemetry_encounter_mode::pvp &&
	       facts.back().actor_count == 2U && facts.back().observed_owner_count == 2U);
	const auto pet_id = npc_context.actor.actor_id;
	fixture_pet_master = nullptr;
	assert(telemetry_runtime_game_battle_actor(&npc, &npc_context));
	assert(npc_context.actor.actor_id == pet_id &&
	       npc_context.actor.kind == telemetry_combat_actor_kind::npc &&
	       npc_context.actor.owner_subject_id == 0U);
	assert(telemetry_battle_context(battle.get(), npc_context, 500U, 500U, sink, &facts)
		       .outcome == telemetry_battle_outcome::accepted);
	assert(facts.back().mode == telemetry_encounter_mode::pve &&
	       facts.back().actor_count == 2U && facts.back().observed_owner_count == 1U);
	assert(telemetry_battle_close(battle.get(), joined.battle,
				      telemetry_battle_close_reason::shutdown, 600U, 600U, sink,
				      &facts)
		       .outcome == telemetry_battle_outcome::accepted);
	std::uint64_t pvp = 0U, pve = 0U;
	for (const auto &fact : facts)
		if (fact.kind == telemetry_battle_fact_kind::actor_summary)
		{
			pvp += fact.effort.pvp_usec;
			pve += fact.effort.pve_usec;
		}
	assert(pvp == 200U && pve == 200U);
	fixture_pet = fixture_pet_master = nullptr;

	auto finish = []()
	{
		telemetry_monotonic_usec now = 0U;
		telemetry_utc_usec utc = 0U;
		assert(telemetry_runtime_now(&now, &utc));
		assert(telemetry_runtime_shutdown({ now + 5'000'000U, 1U, {} }) ==
		       telemetry_runtime_outcome::accepted);
		assert(telemetry_runtime_final_reap() == telemetry_runtime_outcome::accepted);
		telemetry_transport_unbind_for_tests();
	};
	finish();
	fake_repository next_fake{};
	const telemetry_transport_repository_binding next_repository = {
		fake_init, fake_apply, fake_request_stop, fake_shutdown, &next_fake
	};
	assert(telemetry_transport_bind_for_tests(&next_repository, &clock) ==
	       telemetry_transport_outcome::started);
	telemetry_test_start_runtime(enabled_options());
	player.group = member.group = &group;
	assert(telemetry_runtime_game_battle_actor(&player, &player_context));
	assert(player_context.group_revision == 1U &&
	       (group.telemetry_generation.producer.boot_id != generation.producer.boot_id ||
		group.telemetry_generation.producer.process_id != generation.producer.process_id));
	finish();
	assert(!telemetry_runtime_game_battle_actor(&player, &player_context));
	assert(player_context.actor.actor_id == 0U);
	world = nullptr;
	zone_table = nullptr;
	top_of_world = top_of_zone_table = -1;
	std::puts(
		"PASS: native live actor reuse, roster revisions/presence, session links and pet ownership cuts");
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
	telemetry_battle_actor_context resumed_context{};
	assert(telemetry_runtime_game_battle_actor(&recovered, &resumed_context));
	assert(telemetry_battle_actor_context_is_valid(resumed_context));
	assert(resumed_context.session.producer.boot_id ==
		       handoff.handoff.session.id.producer.boot_id &&
	       resumed_context.session.producer.process_id ==
		       handoff.handoff.session.id.producer.process_id &&
	       resumed_context.session.session_seq == handoff.handoff.session.id.session_seq &&
	       resumed_context.encounter.sequence == 0U);
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

fake_repository check_native_unavailable_opponent(bool use_native)
{
	fake_repository fake{};
	fake.use_native = use_native;
	const telemetry_transport_repository_binding repository = { fake_init, fake_apply,
								    fake_request_stop,
								    fake_shutdown, &fake };
	const telemetry_transport_clock_binding clock = { fake_clock, nullptr };
	assert(telemetry_transport_bind_for_tests(&repository, &clock) ==
	       telemetry_transport_outcome::started);
	telemetry_test_start_runtime(enabled_options());
	char_data actor{}, target{}, unavailable{};
	pc_only_data actor_pc{}, target_pc{};
	npc_only_data unavailable_npc{};
	actor.only.pc = &actor_pc;
	target.only.pc = &target_pc;
	actor_pc.pid = 8951;
	target_pc.pid = 8952;
	actor.player.level = target.player.level = 25;
	actor.specials.fighting = &target;
	target.specials.fighting = &actor;
	unavailable.only.npc = &unavailable_npc;
	unavailable.specials.act = ACT_ISNPC;
	unavailable.runtime_id = 0U; // The actual target pointer has no usable lifetime.
	telemetry_runtime_game_combat_damage(&actor, &target, 5U, 0U);
	telemetry_runtime_game_combat_cast_attempt(&actor, 1);
	actor.specials.fighting = &unavailable;
	telemetry_monotonic_usec before = 0U, after = 0U;
	telemetry_utc_usec utc = TELEMETRY_UTC_UNKNOWN;
	assert(telemetry_runtime_now(&before, &utc));
	telemetry_runtime_game_combat_context(&actor);
	assert(telemetry_runtime_now(&after, &utc));
	actor.specials.fighting = nullptr;
	telemetry_runtime_game_combat_context(&actor); // No new zero-valued stream.
	assert(telemetry_runtime_shutdown({ after + 5'000'000U, 1U, {} }) ==
	       telemetry_runtime_outcome::accepted);
	assert(telemetry_runtime_final_reap() == telemetry_runtime_outcome::accepted);
	telemetry_transport_unbind_for_tests();
	assert(fake.contributions.size() == 2U);
	unsigned gaps = 0U;
	for (const auto &record : fake.contributions)
	{
		const auto &row = record.payload.battle_contribution;
		assert((row.quality_flags &
			(TELEMETRY_QUALITY_CONTEXT_UNKNOWN | TELEMETRY_QUALITY_QUEUE_DROP)) ==
		       (TELEMETRY_QUALITY_CONTEXT_UNKNOWN | TELEMETRY_QUALITY_QUEUE_DROP));
		if (row.context.actor.actor.actor_id == 8951U)
		{
			assert(row.end_reason == telemetry_battle_contribution_end::source_gap &&
			       row.counters.damage_dealt == 5U &&
			       row.counters.casting_attempts == 1U &&
			       row.counters.casting_unresolved == 1U &&
			       before <= row.cut.observed_usec && row.cut.observed_usec <= after &&
			       row.cut.observed_usec == row.cut.decision_usec);
			++gaps;
		}
		else
			assert(row.context.actor.actor.actor_id == 8952U &&
			       row.counters.damage_taken == 5U);
	}
	assert(gaps == 1U);
	std::puts(
		"PASS: native unavailable opponent seals explicit source-gap coverage without inventing zero engagement");
	return fake;
}

fake_repository check_native_inactivity_contributions(bool use_native)
{
	fake_repository fake{};
	fake.use_native = use_native;
	const telemetry_transport_repository_binding repository = { fake_init, fake_apply,
								    fake_request_stop,
								    fake_shutdown, &fake };
	const telemetry_transport_clock_binding clock = { fake_clock, nullptr };
	assert(telemetry_transport_bind_for_tests(&repository, &clock) ==
	       telemetry_transport_outcome::started);
	telemetry_test_start_runtime(enabled_options());
	room_data rooms[1]{};
	zone_data zones[1]{};
	zones[0].number = 1903;
	const auto previous_world = world;
	const auto previous_zones = zone_table;
	const int previous_top_world = top_of_world, previous_top_zone = top_of_zone_table;
	world = rooms;
	zone_table = zones;
	top_of_world = top_of_zone_table = 0;
	char_data actors[4]{};
	pc_only_data players[4]{};
	for (unsigned index = 0U; index < 4U; ++index)
	{
		actors[index].only.pc = &players[index];
		players[index].pid = 8911 + static_cast<int>(index);
		actors[index].player.level = 25;
		actors[index].player.m_class = 3;
		actors[index].player.race = 4;
		actors[index].player.racewar = 5;
		actors[index].specials.fighting = &actors[index ^ 1U];
	}
	telemetry_runtime_game_combat_damage(&actors[0], &actors[1], 5U, 0U);
	telemetry_monotonic_usec cast_before = 0U, cast_after = 0U, now = 0U;
	telemetry_utc_usec utc = TELEMETRY_UTC_UNKNOWN;
	assert(telemetry_runtime_now(&cast_before, &utc));
	telemetry_runtime_game_combat_cast_attempt(&actors[0], 1);
	assert(telemetry_runtime_now(&cast_after, &utc));
	std::this_thread::sleep_for(std::chrono::milliseconds(1));
	telemetry_runtime_game_combat_damage(&actors[2], &actors[3], 6U, 0U);
	assert(telemetry_runtime_now(&now, &utc));
	const auto decision = now + 30'000'000U;
	/* The public pulse supplies the fixture's future observation clock. The
	 * actual native close callback must preserve both earlier measured prefixes. */
	assert(telemetry_runtime_pulse({ decision, utc + 30'000'000, 0U, 0U }).outcome ==
	       telemetry_runtime_outcome::accepted);
	assert(telemetry_runtime_shutdown({ now + 5'000'000U, 1U, {} }) ==
	       telemetry_runtime_outcome::accepted);
	assert(telemetry_runtime_final_reap() == telemetry_runtime_outcome::accepted);
	telemetry_transport_unbind_for_tests();
	unsigned closed = 0U;
	telemetry_monotonic_usec first_prefix = 0U, second_prefix = 0U;
	for (const auto &record : fake.battles)
		if (record.payload.battle.kind == telemetry_battle_fact_kind::close)
		{
			const auto &row = record.payload.battle;
			assert(row.close_reason == telemetry_battle_close_reason::inactivity &&
			       row.at_monotonic_usec == decision && row.end_censored == 1U);
			if (row.battle.sequence == 1U)
				first_prefix = row.observed_through_monotonic_usec;
			else
				second_prefix = row.observed_through_monotonic_usec;
			++closed;
		}
	assert(closed == 2U && cast_before <= first_prefix && first_prefix <= cast_after &&
	       first_prefix < second_prefix && second_prefix < decision);
	assert(fake.contributions.size() == 4U);
	unsigned unresolved = 0U;
	for (const auto &record : fake.contributions)
	{
		const auto &row = record.payload.battle_contribution;
		const auto prefix = row.context.battle.sequence == 1U ? first_prefix :
									second_prefix;
		assert(row.end_reason == telemetry_battle_contribution_end::battle_ended &&
		       row.cut.observed_usec == prefix && row.cut.decision_usec == decision &&
		       row.counters.engaged_target_usec == prefix - row.start_usec);
		assert(!(row.quality_flags &
			 (TELEMETRY_QUALITY_QUEUE_DROP | TELEMETRY_QUALITY_CLOCK_DISCONTINUITY)));
		unresolved += static_cast<unsigned>(row.counters.casting_unresolved);
	}
	assert(unresolved == 1U);
	world = previous_world;
	zone_table = previous_zones;
	top_of_world = previous_top_world;
	top_of_zone_table = previous_top_zone;
	std::puts(
		"PASS: native inactivity closes two measured prefixes at a later common decision clock");
	return fake;
}

void export_native_battle_capture(const fake_repository &fake)
{
	const char *export_path = std::getenv("TELEMETRY_BATTLE_CAPTURE_EXPORT");
	if (export_path)
	{
		auto *export_file = std::fopen(export_path, "wb");
		assert(export_file);
		for (const auto &record : fake.battles)
		{
			std::fprintf(
				export_file,
				"{\"boot_id\":%llu,\"process_id\":%llu,\"record_seq\":%llu,\"record_kind\":10,\"schema_version\":1,\"occurrence_utc_usec\":%lld",
				static_cast<unsigned long long>(record.header.key.producer.boot_id),
				static_cast<unsigned long long>(
					record.header.key.producer.process_id),
				static_cast<unsigned long long>(record.header.key.record_seq),
				static_cast<long long>(record.header.occurrence_utc_usec));
#define TELEMETRY_BATTLE_FIELD(name, member, width, is_signed)                                  \
	do                                                                                      \
	{                                                                                       \
		if constexpr (is_signed)                                                        \
			std::fprintf(export_file, ",\"" #name "\":%lld",                        \
				     static_cast<long long>(record.payload.battle.member));     \
		else                                                                            \
			std::fprintf(                                                           \
				export_file, ",\"" #name "\":%llu",                             \
				static_cast<unsigned long long>(record.payload.battle.member)); \
	} while (false);
#include "telemetry/telemetry_battle_fields.inc"
#undef TELEMETRY_BATTLE_FIELD
			std::fputs("}\n", export_file);
		}
		for (const auto &record : fake.contributions)
		{
			std::fprintf(
				export_file,
				"{\"boot_id\":%llu,\"process_id\":%llu,\"record_seq\":%llu,\"record_kind\":11,\"schema_version\":1,\"occurrence_utc_usec\":%lld",
				static_cast<unsigned long long>(record.header.key.producer.boot_id),
				static_cast<unsigned long long>(
					record.header.key.producer.process_id),
				static_cast<unsigned long long>(record.header.key.record_seq),
				static_cast<long long>(record.header.occurrence_utc_usec));
#define TELEMETRY_BC_FIELD(name, member, width, is_signed)                                \
	do                                                                                \
	{                                                                                 \
		if constexpr (is_signed)                                                  \
			std::fprintf(export_file, ",\"" #name "\":%lld",                  \
				     static_cast<long long>(                              \
					     record.payload.battle_contribution.member)); \
		else                                                                      \
			std::fprintf(export_file, ",\"" #name "\":%llu",                  \
				     static_cast<unsigned long long>(                     \
					     record.payload.battle_contribution.member)); \
	} while (false);
#include "telemetry/telemetry_battle_contribution_fields.inc"
#undef TELEMETRY_BC_FIELD
			std::fputs("}\n", export_file);
		}
		assert(std::fclose(export_file) == 0);
	}
}

void check_native_shared_battle_capture(bool use_native = false)
{
	fake_repository fake{};
	fake.use_native = use_native;
	const telemetry_transport_repository_binding repository = { fake_init, fake_apply,
								    fake_request_stop,
								    fake_shutdown, &fake };
	const telemetry_transport_clock_binding clock = { fake_clock, nullptr };
	assert(telemetry_transport_bind_for_tests(&repository, &clock) ==
	       telemetry_transport_outcome::started);
	telemetry_test_start_runtime(enabled_options());
	const auto producer = telemetry_runtime_producer_copy();
	const auto initial_config = telemetry_config_snapshot_copy();
	room_data rooms[3]{};
	zone_data zones[2]{};
	rooms[0].zone = rooms[1].zone = 0U;
	rooms[2].zone = 1U;
	zones[0].number = 1901;
	zones[1].number = 1902;
	world = rooms;
	zone_table = zones;
	top_of_world = 2;
	top_of_zone_table = 1;
	char_data attacker{}, defender{}, healer{}, present{}, distant{}, pet{}, first_npc{},
		second_npc{};
	pc_only_data attacker_pc{}, defender_pc{}, healer_pc{}, present_pc{}, distant_pc{};
	npc_only_data pet_data{}, first_data{}, second_data{};
	attacker.only.pc = &attacker_pc;
	defender.only.pc = &defender_pc;
	healer.only.pc = &healer_pc;
	present.only.pc = &present_pc;
	distant.only.pc = &distant_pc;
	attacker_pc.pid = 8801;
	defender_pc.pid = 8802;
	healer_pc.pid = 8803;
	present_pc.pid = 8804;
	distant_pc.pid = 8805;
	pet.only.npc = &pet_data;
	first_npc.only.npc = &first_data;
	second_npc.only.npc = &second_data;
	pet_data.R_num = first_data.R_num = second_data.R_num = 77;
	for (auto *npc : { &pet, &first_npc, &second_npc })
	{
		npc->specials.act = ACT_ISNPC;
		npc->runtime_id = allocate_character_runtime_id();
	}
	for (auto *actor :
	     { &attacker, &defender, &healer, &present, &distant, &pet, &first_npc, &second_npc })
	{
		actor->in_room = 0;
		actor->player.level = 25;
		actor->player.m_class = 3;
		actor->player.race = 4;
		actor->player.racewar = 5;
	}
	distant.in_room = 1; // Same zone alone cannot prove party participation.
	group_list distant_node{ &distant, nullptr };
	group_list present_node{ &present, &distant_node };
	group_list group{ &attacker, &present_node };
	attacker.group = present.group = distant.group = &group;
	descriptor_data attacker_descriptor{}, defender_descriptor{};
	attacker_descriptor.connected = defender_descriptor.connected = CON_PLAYING;
	attacker_descriptor.character = &attacker;
	defender_descriptor.character = &defender;
	attacker.desc = &attacker_descriptor;
	defender.desc = &defender_descriptor;
	assert(telemetry_runtime_game_enter(&attacker, &attacker_descriptor).outcome ==
	       telemetry_runtime_outcome::accepted);
	assert(telemetry_runtime_game_enter(&defender, &defender_descriptor).outcome ==
	       telemetry_runtime_outcome::accepted);
	attacker.specials.fighting = &defender;
	defender.specials.fighting = &attacker;
	assert(telemetry_runtime_game_combat_engage(&attacker, &defender).outcome ==
	       telemetry_runtime_outcome::accepted);
	assert(telemetry_runtime_game_combat_engage(&attacker, &defender).records_emitted == 0U);
	telemetry_runtime_game_combat_damage(&attacker, &defender, 11U, 0U);
	telemetry_runtime_game_combat_cast_attempt(&attacker, 1);
	telemetry_runtime_game_combat_cast_attempt(&attacker, 1); // One actual pending cast.
	telemetry_runtime_game_combat_cast_complete(&attacker);
	telemetry_runtime_game_combat_cast_complete(&attacker); // Terminal retry adds no amount.
	telemetry_monotonic_usec arrival_cut = 0U;
	telemetry_utc_usec arrival_utc = TELEMETRY_UTC_UNKNOWN;
	assert(telemetry_runtime_now(&arrival_cut, &arrival_utc));
	distant.in_room = 0;
	// Arrival context alone cannot join a battle. A repeated actual hostile
	// observation proves the new same-room party presence, without contribution.
	assert(telemetry_runtime_game_battle_context(&distant).records_emitted == 0U);
	assert(telemetry_runtime_game_combat_engage(&attacker, &defender).records_emitted != 0U);
	assert(telemetry_runtime_game_battle_leave(&distant).outcome ==
	       telemetry_runtime_outcome::accepted);
	distant.in_room = 1;
	telemetry_runtime_game_combat_healing(&healer, &attacker, 20U, 0U, 0U);
	telemetry_runtime_game_combat_healing(&healer, &attacker, 20U, 12U, 0U);
	telemetry_runtime_game_combat_healing(&healer, &attacker, 20U, 0U, 0U);
	telemetry_runtime_game_combat_healing(&attacker, &attacker, 5U, 3U, 0U);
	telemetry_runtime_game_combat_cast_attempt(&healer, 1);
	telemetry_runtime_game_combat_cast_abort(&healer);
	fixture_pet = &pet;
	fixture_pet_master = &attacker;
	pet.specials.fighting = &defender;
	assert(telemetry_runtime_game_combat_engage(&pet, &defender).outcome ==
	       telemetry_runtime_outcome::accepted);
	telemetry_runtime_game_combat_damage(&pet, &defender, 7U, 0U);
	telemetry_runtime_game_combat_damage(&attacker, &first_npc, 3U, 0U);
	telemetry_runtime_game_combat_damage(&attacker, &second_npc, 4U, 0U);
	const auto old_instance = TELEMETRY_BATTLE_NPC_GENERATION_TAG | first_npc.runtime_id;
	assert(telemetry_runtime_game_battle_leave(&first_npc).outcome ==
	       telemetry_runtime_outcome::accepted);
	first_npc.runtime_id = allocate_character_runtime_id();
	const auto new_instance = TELEMETRY_BATTLE_NPC_GENERATION_TAG | first_npc.runtime_id;
	telemetry_runtime_game_combat_damage(&attacker, &first_npc, 5U, 0U);
	assert(old_instance != new_instance);
	const auto generation = group.telemetry_generation;
	telemetry_runtime_game_group_changed(&group);
	assert(group.telemetry_generation.sequence == generation.sequence &&
	       group.telemetry_generation.revision == generation.revision + 1U);
	assert(telemetry_runtime_game_battle_leave(&present).outcome ==
	       telemetry_runtime_outcome::accepted);
	present.in_room = 1;
	assert(telemetry_runtime_game_battle_context(&present).outcome ==
	       telemetry_runtime_outcome::accepted);
	assert(telemetry_runtime_game_battle_leave(&attacker).outcome ==
	       telemetry_runtime_outcome::accepted);
	attacker.in_room = 2;
	assert(telemetry_runtime_game_battle_context(&attacker).outcome ==
	       telemetry_runtime_outcome::accepted);
	telemetry_runtime_game_combat_damage(&attacker, &defender, 6U, 0U);
	fixture_pet_master = nullptr;
	assert(telemetry_runtime_game_battle_context(&pet).outcome ==
	       telemetry_runtime_outcome::accepted);
	telemetry_runtime_game_combat_damage(&pet, &defender, 9U, 0U);
	telemetry_runtime_game_combat_cast_attempt(&attacker, 1);
	auto changed = initial_config;
	++changed.revision;
	++changed.policy_version;
	telemetry_monotonic_usec now = 0U;
	telemetry_utc_usec utc = TELEMETRY_UTC_UNKNOWN;
	assert(telemetry_runtime_now(&now, &utc));
	changed.effective_utc_usec = utc;
	assert(telemetry_config_compute_fingerprint(changed, changed.fingerprint,
						    sizeof(changed.fingerprint)));
	changed.config_id = telemetry_config_id_from_fingerprint(changed.fingerprint,
								 sizeof(changed.fingerprint));
	assert(telemetry_config_publish(changed).outcome == telemetry_runtime_outcome::accepted);
	assert(telemetry_runtime_game_battle_context(&attacker).outcome ==
	       telemetry_runtime_outcome::accepted);
	telemetry_runtime_game_combat_cast_complete(&attacker); // Pre-cut cast stays unresolved.
	telemetry_runtime_game_combat_damage(&attacker, &defender, 19U, 0U);
	assert(telemetry_runtime_now(&now, &utc));
	const auto flush_limit = std::chrono::steady_clock::now() + std::chrono::seconds(5);
	auto flushed = telemetry_runtime_flush_for_copyover(now + 250'000U);
	unsigned flush_retries = 0U;
	/* Each production barrier retains its 250 ms cap. A SQL correctness fixture
	 * may retry a failed generation while the worker finishes the same immutable
	 * records; repeated closes must not duplicate contribution amounts. */
	while (use_native && flushed == telemetry_runtime_outcome::queue_full &&
	       std::chrono::steady_clock::now() < flush_limit)
	{
		std::this_thread::sleep_for(std::chrono::milliseconds(1));
		assert(telemetry_runtime_now(&now, &utc));
		flushed = telemetry_runtime_flush_for_copyover(now + 250'000U);
		++flush_retries;
	}
	if (flushed != telemetry_runtime_outcome::accepted)
	{
		const auto health = telemetry_runtime_health_copy();
		std::fprintf(
			stderr,
			"Native copyover flush refused: outcome=%u state=%u queue=%llu admitted=%llu applied=%llu invalid=%llu conflict=%llu failure=%u retries=%u\n",
			unsigned(flushed), unsigned(health.state),
			static_cast<unsigned long long>(health.queue_depth),
			static_cast<unsigned long long>(health.admitted_detail +
							health.admitted_control),
			static_cast<unsigned long long>(health.applied_records),
			static_cast<unsigned long long>(health.invalid_records),
			static_cast<unsigned long long>(health.conflict_records),
			unsigned(health.last_failure_class), flush_retries);
	}
	assert(flushed == telemetry_runtime_outcome::accepted);
	if (use_native)
		std::printf("Native copyover accepted with %u bounded generation retries\n",
			    flush_retries);
	assert(telemetry_runtime_game_combat_engage(&attacker, &defender).outcome ==
	       telemetry_runtime_outcome::accepted);
	healer.specials.fighting = &second_npc;
	telemetry_runtime_game_combat_damage(&healer, &second_npc, 13U, 0U);
	telemetry_runtime_game_combat_damage(&attacker, &healer, 17U, 0U); // Join two battles.
	fixture_pet_master = &attacker;
	telemetry_runtime_game_combat_damage(&pet, &defender, 2U, 0U);
	fixture_pet_master = nullptr; // Teardown still has the retained pet lifetime kind.
	assert(telemetry_runtime_game_battle_leave(&pet).outcome ==
	       telemetry_runtime_outcome::accepted);
	telemetry_runtime_game_combat_cast_attempt(&attacker, 1);
	assert(telemetry_runtime_game_session_exit(&attacker, &attacker_descriptor,
						   telemetry_session_end_reason::logout)
		       .outcome == telemetry_runtime_outcome::accepted);
	assert(telemetry_runtime_now(&now, &utc));
	assert(telemetry_runtime_shutdown({ now + 5'000'000U, 1U, {} }) ==
	       telemetry_runtime_outcome::accepted);
	assert(telemetry_runtime_final_reap() == telemetry_runtime_outcome::accepted);
	telemetry_transport_unbind_for_tests();
	assert(!fake.battles.empty());
	const auto first_battle = fake.battles.front().payload.battle.battle;
	unsigned copyovers = 0U, shutdowns = 0U, source_roles = 0U, presence_summaries = 0U,
		 arrival_summaries = 0U, session_leaves = 0U;
	telemetry_monotonic_usec session_exit_cut = 0U;
	for (const auto &lifecycle : fake.lifecycles)
		if (lifecycle.session.pid == attacker_pc.pid &&
		    lifecycle.end_reason == telemetry_session_end_reason::logout)
			session_exit_cut = lifecycle.at_monotonic_usec;
	assert(session_exit_cut != 0U);
	bool saw_old_instance = false, saw_new_instance = false, saw_pet_owner_loss = false,
	     saw_changed_scope = false;
	for (std::size_t start = 0U; start < fake.battles.size();)
	{
		const auto count = fake.battles[start].payload.battle.fact_count;
		assert(start + count <= fake.battles.size());
		telemetry_battle_fact packet[TELEMETRY_BATTLE_PACKET_MAX_FACTS]{};
		for (std::size_t index = 0U; index < count; ++index)
			packet[index] = fake.battles[start + index].payload.battle;
		assert(telemetry_battle_packet_is_valid(packet, count));
		start += count;
	}
	for (const auto &record : fake.battles)
	{
		const auto &row = record.payload.battle;
		assert(record.header.key.producer.boot_id == producer.boot_id &&
		       record.header.key.producer.process_id == producer.process_id);
		assert(row.inactivity_grace_usec == 30'000'000U);
		if (row.actor.actor.actor_id == 8805U)
		{
			assert(row.at_monotonic_usec >= arrival_cut &&
			       row.roles == TELEMETRY_BATTLE_ROLE_GROUP_PRESENCE &&
			       row.effort.contributor_usec == 0U);
			arrival_summaries += row.kind ==
						     telemetry_battle_fact_kind::actor_summary &&
					     row.active == 0U;
		}
		if (row.kind == telemetry_battle_fact_kind::actor_context && row.active == 0U &&
		    row.actor.actor.actor_id == 8801U &&
		    row.battle.sequence != first_battle.sequence)
		{
			assert(row.at_monotonic_usec == session_exit_cut);
			++session_leaves;
		}
		if (row.battle.sequence == first_battle.sequence)
		{
			saw_old_instance |= row.actor.actor.actor_id == old_instance;
			saw_new_instance |= row.actor.actor.actor_id == new_instance;
			saw_pet_owner_loss |=
				row.actor.actor.actor_id ==
					(TELEMETRY_BATTLE_NPC_GENERATION_TAG | pet.runtime_id) &&
				row.actor.actor.kind == telemetry_combat_actor_kind::npc &&
				row.actor.actor.owner_subject_id == 0U;
			saw_changed_scope |= row.scope.config_id == changed.config_id &&
					     row.scope.policy_version == changed.policy_version;
			assert(row.observed_owner_count <= 5U);
			if (row.actor.actor.actor_id == 8803U)
			{
				assert(row.roles & TELEMETRY_BATTLE_ROLE_SUPPORT);
				++source_roles;
			}
			if (row.kind == telemetry_battle_fact_kind::actor_summary &&
			    row.actor.actor.actor_id == 8804U)
			{
				assert(row.roles == TELEMETRY_BATTLE_ROLE_GROUP_PRESENCE &&
				       row.effort.contributor_usec == 0U && row.active == 0U);
				++presence_summaries;
			}
		}
		if (row.kind == telemetry_battle_fact_kind::close)
		{
			assert(row.end_censored == 1U);
			copyovers += row.close_reason == telemetry_battle_close_reason::copyover;
			shutdowns += row.close_reason == telemetry_battle_close_reason::shutdown;
			if (row.close_reason == telemetry_battle_close_reason::shutdown)
				assert(row.battle.sequence > first_battle.sequence);
		}
	}
	assert(copyovers == 1U && shutdowns == 1U && source_roles != 0U &&
	       presence_summaries == 1U && arrival_summaries == 1U && session_leaves == 1U);
	assert(saw_old_instance && saw_new_instance && saw_pet_owner_loss && saw_changed_scope);
	std::set<telemetry_sequence> contribution_sequences;
	std::uint64_t dealt = 0U, taken = 0U, healing = 0U, effective = 0U, overhealing = 0U,
		      received = 0U, attempts = 0U, completed = 0U, aborted = 0U, unresolved = 0U;
	std::uint64_t attacker_damage = 0U, old_npc_damage = 0U, new_npc_damage = 0U,
		      owned_pet_damage = 0U, unowned_pet_damage = 0U;
	unsigned aliases = 0U, changed_segments = 0U, pet_teardowns = 0U,
		 contribution_session_leaves = 0U, changed_scope_segments = 0U;
	for (const auto &record : fake.battles)
		aliases += record.payload.battle.kind == telemetry_battle_fact_kind::merge_alias;
	for (const auto &record : fake.contributions)
	{
		const auto &row = record.payload.battle_contribution;
		const auto &actor = row.context.actor.actor;
		const auto &counts = row.counters;
		assert(contribution_sequences.insert(row.sequence).second);
		assert(row.context.available_metrics == TELEMETRY_BC_METRICS &&
		       counts.control_applications == 0U && counts.control_received == 0U);
		assert(actor.actor_id != 8804U &&
		       actor.actor_id != 8805U); // Presence adds no metrics.
		assert(row.cut.observed_usec >= row.start_usec &&
		       row.cut.decision_usec >= row.cut.observed_usec);
		assert(counts.engaged_target_usec <= row.cut.observed_usec - row.start_usec);
		assert(!(row.quality_flags &
			 (TELEMETRY_QUALITY_QUEUE_DROP | TELEMETRY_QUALITY_CLOCK_DISCONTINUITY)));
		dealt += counts.damage_dealt;
		taken += counts.damage_taken;
		healing += counts.healing_attempted;
		effective += counts.effective_healing;
		overhealing += counts.overhealing;
		received += counts.healing_received;
		attempts += counts.casting_attempts;
		completed += counts.casting_completions;
		aborted += counts.casting_aborts;
		unresolved += counts.casting_unresolved;
		if (actor.actor_id == 8801U)
		{
			attacker_damage += counts.damage_dealt;
			if (row.end_reason == telemetry_battle_contribution_end::actor_left &&
			    row.context.battle.sequence != first_battle.sequence)
			{
				assert(row.cut.observed_usec == session_exit_cut &&
				       row.cut.decision_usec == session_exit_cut);
				++contribution_session_leaves;
			}
		}
		if (actor.actor_id == old_instance)
			old_npc_damage += counts.damage_taken;
		if (actor.actor_id == new_instance)
			new_npc_damage += counts.damage_taken;
		if (actor.actor_id == (TELEMETRY_BATTLE_NPC_GENERATION_TAG | pet.runtime_id))
		{
			if (actor.kind == telemetry_combat_actor_kind::pet)
			{
				assert(actor.owner_subject_id == 8801U);
				owned_pet_damage += counts.damage_dealt;
				pet_teardowns += row.end_reason ==
						 telemetry_battle_contribution_end::actor_left;
			}
			else
			{
				assert(actor.kind == telemetry_combat_actor_kind::npc &&
				       actor.owner_subject_id == 0U);
				unowned_pet_damage += counts.damage_dealt;
			}
		}
		if (row.context.scope.config_id == changed.config_id)
		{
			assert(row.context.scope.policy_version == changed.policy_version);
			++changed_scope_segments;
		}
		changed_segments += row.end_reason ==
				    telemetry_battle_contribution_end::context_changed;
	}
	assert(!fake.contributions.empty() && dealt == 96U && taken == dealt &&
	       attacker_damage == 65U && old_npc_damage == 3U && new_npc_damage == 5U &&
	       owned_pet_damage == 9U && unowned_pet_damage == 9U);
	assert(healing == 45U && effective == 15U && overhealing == 30U && received == effective);
	assert(attempts == 4U && completed == 1U && aborted == 1U && unresolved == 2U);
	assert(aliases == 1U && changed_segments != 0U && changed_scope_segments != 0U &&
	       pet_teardowns == 1U && contribution_session_leaves == 1U);
	const auto inactivity = check_native_inactivity_contributions(use_native);
	fake.battles.insert(fake.battles.end(), inactivity.battles.begin(),
			    inactivity.battles.end());
	fake.contributions.insert(fake.contributions.end(), inactivity.contributions.begin(),
				  inactivity.contributions.end());
	const auto unavailable_opponent = check_native_unavailable_opponent(use_native);
	fake.battles.insert(fake.battles.end(), unavailable_opponent.battles.begin(),
			    unavailable_opponent.battles.end());
	fake.contributions.insert(fake.contributions.end(),
				  unavailable_opponent.contributions.begin(),
				  unavailable_opponent.contributions.end());
	export_native_battle_capture(fake);
	fixture_pet = fixture_pet_master = nullptr;
	world = nullptr;
	zone_table = nullptr;
	top_of_world = top_of_zone_table = -1;
	std::puts(
		"PASS: native shared battles and disjoint contributions, useful support, exact party presence, generations, alias, scope cuts and censored lifecycle");
}

fake_repository check_native_control_capture(bool use_native = false, bool export_capture = false)
{
	fake_repository fake{};
	fake.use_native = use_native;
	const telemetry_transport_repository_binding repository = { fake_init, fake_apply,
								    fake_request_stop,
								    fake_shutdown, &fake };
	const telemetry_transport_clock_binding clock = { fake_clock, nullptr };
	assert(telemetry_transport_bind_for_tests(&repository, &clock) ==
	       telemetry_transport_outcome::started);
	telemetry_test_start_runtime(enabled_options());
	room_data rooms[1]{};
	zone_data zones[1]{};
	zones[0].number = 1906;
	const auto previous_world = world;
	const auto previous_zones = zone_table;
	const int previous_top_world = top_of_world, previous_top_zone = top_of_zone_table;
	world = rooms;
	zone_table = zones;
	top_of_world = top_of_zone_table = 0;
	char_data attacker{}, target{}, unrelated{}, npc{}, pet{};
	pc_only_data attacker_data{}, target_data{}, unrelated_data{};
	npc_only_data npc_data{}, pet_data{};
	attacker.only.pc = &attacker_data;
	target.only.pc = &target_data;
	unrelated.only.pc = &unrelated_data;
	attacker_data.pid = 8961;
	target_data.pid = 8962;
	unrelated_data.pid = 8963;
	npc.only.npc = &npc_data;
	pet.only.npc = &pet_data;
	npc_data.R_num = pet_data.R_num = -1; // Native lifetime exists; legacy ID is absent.
	for (auto *actor : { &npc, &pet })
	{
		actor->specials.act = ACT_ISNPC;
		actor->runtime_id = allocate_character_runtime_id();
	}
	for (auto *actor : { &attacker, &target, &unrelated, &npc, &pet })
	{
		actor->in_room = 0;
		actor->player.level = 25;
		actor->player.m_class = 3;
		actor->player.race = RACE_HUMAN;
		actor->player.racewar = 5;
		actor->specials.position = STAT_NORMAL;
	}
	fixture_control_effects.clear();
	fixture_control_save_index = 0U;
	fixture_control_random = 0;
	// Rejected blindness and stun must neither mutate an effect nor start a battle.
	target.specials.affected_by5 = AFF5_NOBLIND;
	assert(!blind(&attacker, &target, 10));
	target.specials.affected_by5 = 0U;
	target.specials.affected_by = AFF_BLIND;
	assert(!blind(&attacker, &target, 10));
	target.specials.affected_by = 0U;
	for (auto race : { RACE_PARASITE, RACE_SLIME })
	{
		target.player.race = race;
		assert(!blind(&attacker, &target, 10));
	}
	target.player.race = RACE_HUMAN;
	fixture_control_eyeless = true;
	assert(!blind(&attacker, &target, 10));
	fixture_control_eyeless = false;
	fixture_control_named_immune = true;
	assert(!blind(&attacker, &target, 10));
	fixture_control_named_immune = false;
	target.player.level = MAXLVLMORTAL + 1;
	assert(!blind(&attacker, &target, 10));
	target.player.level = 25;
	target.specials.position = STAT_DEAD;
	assert(!blind(&attacker, &target, 10));
	Stun(&target, &attacker, 8, false);
	target.specials.position = STAT_NORMAL;
	target.specials.act = ACT_ELITE;
	Stun(&target, &attacker, 8, false);
	target.specials.act = 0U;
	target.player.race = RACE_PLANT;
	Stun(&target, &attacker, 8, false);
	target.player.race = RACE_HUMAN;
	target.specials.affected_by2 = AFF2_STUNNED;
	Stun(&target, &attacker, 8, false);
	target.specials.affected_by2 = 0U;
	fixture_control_saves = { true, true };
	Stun(&target, &attacker, 8, true);
	assert(fixture_control_save_index == 2U && fixture_control_effects.empty());
	telemetry_runtime_game_combat_control(&attacker, &unrelated, 0U, 0U);
	telemetry_runtime_game_combat_control(&attacker, &unrelated, 1U,
					      ~TELEMETRY_COMBAT_MODIFIER_KNOWN);
	// Accepted self effects outside a battle remain outside shared participation.
	assert(blind(&unrelated, &unrelated, 10));
	Stun(&unrelated, &unrelated, 8, false);
	assert(fixture_control_effects.size() == 2U);
	// A real accepted blindness observation can establish the shared hostile edge.
	assert(blind(&attacker, &target, 10));
	assert(!blind(&attacker, &target, 10)); // Already active: no second application.
	assert(fixture_control_effects.size() == 3U);
	attacker.specials.fighting = &target;
	target.specials.fighting = &attacker;
	assert(telemetry_runtime_game_combat_engage(&attacker, &target).outcome ==
	       telemetry_runtime_outcome::accepted);
	fixture_control_saves = { false };
	fixture_control_save_index = 0U;
	Stun(&target, &attacker, 8, true);
	assert(fixture_control_save_index == 1U && !IS_FIGHTING(&target) &&
	       fixture_control_effects.back().duration == 8);
	target.specials.affected_by2 = 0U;
	target.specials.fighting = &attacker;
	fixture_control_saves = { true, false };
	fixture_control_save_index = 0U;
	Stun(&target, &attacker, 8, true);
	assert(fixture_control_save_index == 2U && !IS_FIGHTING(&target) &&
	       fixture_control_effects.back().duration == 4);
	target.specials.affected_by2 = 0U;
	target.specials.fighting = &attacker;
	Stun(&target, &attacker, 8, false);
	assert(!IS_FIGHTING(&target) && fixture_control_effects.back().duration == 8);
	target.specials.affected_by = 0U;
	assert(blind(&target, &target, 10)); // Existing battle: one actor, SELF modifier.
	fixture_pet = &pet;
	fixture_pet_master = &attacker;
	target.specials.affected_by2 = 0U;
	Stun(&target, &pet, 8, false);
	assert(blind(&attacker, &npc, 10));
	assert(blind(&npc, &attacker, 10));
	assert(fixture_control_effects.size() == 10U); // Eight captured + two unrelated.
	assert(telemetry_runtime_encounter_close_all(telemetry_encounter_outcome::copyover)
		       .outcome == telemetry_runtime_outcome::accepted);
	telemetry_monotonic_usec now = 0U;
	telemetry_utc_usec utc = TELEMETRY_UTC_UNKNOWN;
	assert(telemetry_runtime_now(&now, &utc));
	assert(telemetry_runtime_shutdown({ now + 5'000'000U, 1U, {} }) ==
	       telemetry_runtime_outcome::accepted);
	assert(telemetry_runtime_final_reap() == telemetry_runtime_outcome::accepted);
	telemetry_transport_unbind_for_tests();
	std::uint64_t applications = 0U, received = 0U;
	unsigned self_segments = 0U, pet_segments = 0U, npc_segments = 0U, starts = 0U;
	for (const auto &record : fake.battles)
	{
		const auto &fact = record.payload.battle;
		assert(fact.actor.actor.actor_id != 8963U);
		starts += fact.kind == telemetry_battle_fact_kind::start;
	}
	for (const auto &record : fake.contributions)
	{
		const auto &row = record.payload.battle_contribution;
		const auto &actor = row.context.actor.actor;
		assert(row.context.available_metrics == TELEMETRY_BC_METRICS &&
		       actor.actor_id != 8963U);
		assert(!(row.quality_flags &
			 (TELEMETRY_QUALITY_QUEUE_DROP | TELEMETRY_QUALITY_CLOCK_DISCONTINUITY)));
		applications += row.counters.control_applications;
		received += row.counters.control_received;
		if (row.modifier_flags & TELEMETRY_COMBAT_MODIFIER_SELF)
		{
			assert(actor.actor_id == 8962U && row.counters.control_applications == 1U);
			++self_segments;
		}
		if (actor.kind == telemetry_combat_actor_kind::pet)
		{
			assert(actor.owner_subject_id == 8961U &&
			       row.counters.control_applications == 1U);
			++pet_segments;
		}
		if (actor.kind == telemetry_combat_actor_kind::npc)
		{
			assert(actor.actor_id ==
				       (TELEMETRY_BATTLE_NPC_GENERATION_TAG | npc.runtime_id) &&
			       row.counters.control_applications == 1U &&
			       row.counters.control_received == 1U);
			++npc_segments;
		}
	}
	assert(starts == 1U && applications == 8U && received == 8U && self_segments == 1U &&
	       pet_segments == 1U && npc_segments == 1U);
	if (export_capture)
		export_native_battle_capture(fake);
	fixture_pet = fixture_pet_master = nullptr;
	world = previous_world;
	zone_table = previous_zones;
	top_of_world = previous_top_world;
	top_of_zone_table = previous_top_zone;
	std::puts(
		"PASS: accepted blind/Stun source helpers, rejection gates, self isolation, pet/NPC lifetime and conserved 8/8 control");
	return fake;
}

} // namespace

int main(int argc, char **argv)
{
	if (argc == 2 && std::strcmp(argv[1], "--native-control-capture") == 0)
	{
		check_native_control_capture(false, true);
		return 0;
	}
#ifdef TELEMETRY_TEST_NATIVE_BATTLE_SQL
	if (argc == 2 && std::strcmp(argv[1], "--native-control-sql") == 0)
	{
		check_native_control_capture(true, true);
		std::puts("accepted native control helpers through SQL writer passed");
		return 0;
	}
	if (argc == 2 && std::strcmp(argv[1], "--native-battle-sql") == 0)
	{
		check_native_shared_battle_capture(true);
		std::puts("live battle runtime through native SQL writer passed");
		return 0;
	}
#endif
	assert(argc == 1);
	(void)argv;
	check_native_battle_context();
	check_native_shared_battle_capture();
	check_native_control_capture();
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
