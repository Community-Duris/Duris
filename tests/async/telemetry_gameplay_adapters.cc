#include "telemetry/telemetry_config_private.h"
#include "telemetry/telemetry_runtime.h"
#include "telemetry/telemetry_battle.h"
#include "telemetry/telemetry_battle_contract.h"
#include "telemetry/telemetry_battle_contribution.h"
#include "telemetry/telemetry_battle_build_context.h"
#include "telemetry/telemetry_battle_build_observation.h"
#include "combat/arena.h"
#include "telemetry/telemetry_transport_private.h"
#include "core/structs.h"
#include "core/utils.h"
#include "core/prototypes.h"
#include "magic/spells.h"
#ifdef TELEMETRY_TEST_NATIVE_AFFECTS
#include "combat/spell_wards.h"
#include "combat/racewar_stat_mods.h"
#include "core/mm.h"
#include "core/profile.h"
#include <array>
#endif

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
#include <openssl/crypto.h>

P_room world = nullptr;
P_char character_list = nullptr;
struct zone_data *zone_table = nullptr;
int top_of_zone_table = -1;
int top_of_world = -1;
Skill skills[MAX_AFFECT_TYPES + 1]{};
struct arena_data arena
{
};
thread_local bool fixture_track_crypto = false;
thread_local std::uint64_t fixture_crypto_heap_calls = 0U;
static const std::thread::id fixture_game_thread_id = std::this_thread::get_id();
bool nevent_require_game_thread(const char *)
{
	return std::this_thread::get_id() == fixture_game_thread_id;
}
void *fixture_crypto_malloc(std::size_t bytes, const char *, int)
{
	fixture_crypto_heap_calls += fixture_track_crypto;
	return std::malloc(bytes);
}
void *fixture_crypto_realloc(void *memory, std::size_t bytes, const char *, int)
{
	fixture_crypto_heap_calls += fixture_track_crypto;
	return std::realloc(memory, bytes);
}
void fixture_crypto_free(void *memory, const char *, int)
{
	fixture_crypto_heap_calls += fixture_track_crypto;
	std::free(memory);
}
P_char fixture_pet = nullptr;
P_char fixture_pet_master = nullptr;
std::vector<affected_type> fixture_control_effects;
std::vector<bool> fixture_control_saves;
std::size_t fixture_control_save_index = 0U;
bool fixture_control_eyeless = false, fixture_control_named_immune = false;
bool fixture_control_resistance = false, fixture_control_freedom = false;
unsigned fixture_control_mutations = 0U, fixture_control_stops = 0U;
const char *fixture_control_route = "blind/Stun";
unsigned fixture_control_call = 0U;
int fixture_control_random = 0;

// Game-service seams for the maintained blind/Stun and status-control spell bodies.
// Affect mutation is isolated here; the running-server journey remains required.
#ifndef TELEMETRY_TEST_NATIVE_AFFECTS
affected_type *affect_to_char(P_char character, affected_type *effect)
{
	assert(!(effect->flags & AFFTYPE_NOAPPLY));
	character->specials.affected_by |= effect->bitvector;
	character->specials.affected_by2 |= effect->bitvector2;
	fixture_control_effects.push_back(*effect);
	++fixture_control_mutations;
	return &fixture_control_effects.back();
}
void affect_join(P_char character, affected_type *effect, int average_duration,
		 int average_modifier)
{
	assert(!average_duration && !average_modifier && effect->type == SPELL_SLEEP);
	(void)affect_to_char(character, effect);
}
#endif
bool resists_spell(P_char, P_char)
{
	return fixture_control_resistance;
}
bool check_freedom_of_movement(P_char, bool)
{
	return fixture_control_freedom;
}
bool saves_spell(P_char, int type)
{
	assert((type == SAVING_PARA || type == SAVING_SPELL) &&
	       fixture_control_save_index < fixture_control_saves.size());
	return fixture_control_saves[fixture_control_save_index++];
}
void appear(P_char, bool) {}
bool ac_can_see(P_char, P_char, bool)
{
	return true;
}
void remember(P_char, P_char) {}
void MobStartFight(P_char character, P_char target)
{
	GET_OPPONENT(character) = target;
}
void StopMercifulAttackers(P_char) {}
bool has_innate(P_char, int innate)
{
#ifndef TELEMETRY_TEST_NATIVE_AFFECTS
	assert(innate == INNATE_EYELESS);
#endif
	return innate == INNATE_EYELESS && fixture_control_eyeless;
}
bool isname(const char *name, const char *)
{
	assert(std::strcmp(name, "_noblind_") == 0);
	return fixture_control_named_immune;
}
bool NewSaves(P_char victim, int type, int)
{
	if (fixture_control_save_index >= fixture_control_saves.size())
		std::fprintf(
			stderr,
			"Saving-throw fixture exhausted: route=%s call=%u type=%d race=%d level=%d index=%zu size=%zu\n",
			fixture_control_route, fixture_control_call, type, GET_RACE(victim),
			GET_LEVEL(victim), fixture_control_save_index,
			fixture_control_saves.size());
	assert((type == SAVING_FEAR || type == SAVING_PARA) &&
	       fixture_control_save_index < fixture_control_saves.size());
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
	assert(IS_STUNNED(character) || IS_FIGHTING(character));
	++fixture_control_stops;
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

#ifdef TELEMETRY_TEST_NATIVE_AFFECTS
// Isolate scheduler/memory/UI/stat services, while executing the maintained
// affect aggregation, flag-bank application, rebuild, unlink and expiry bodies.
std::array<affected_type, 64> fixture_affect_pool{};
std::array<bool, 64> fixture_affect_used{};
mm_ds fixture_affect_allocator{};
std::array<nevent_data, 128> fixture_affect_events{};
std::array<event_short_affect_data, 128> fixture_affect_payloads{};
std::size_t fixture_affect_event_count = 0U;
unsigned fixture_affect_scheduled = 0U, fixture_affect_canceled = 0U, fixture_affect_deaths = 0U;
unsigned fixture_affect_wakes = 0U;
std::uint16_t fixture_expected_wake_mask = 0U;
void fixture_damage_control_release(P_char, P_char, int, int);
void fixture_staff_control_bit(P_char, bool, unsigned, bool, bool);
void fixture_staff_control_bank(P_char, int);
unsigned long long ne_event_tick = 100U;
bool do_profile = false;
profile_timer short_affect_liveness_profile{};
P_index obj_index = nullptr;
extern const stat_data stat_factor[LAST_RACE + 1]{};
extern const race_names race_names_table[LAST_RACE + 1]{};
float combat_by_race[LAST_RACE + 1][3]{};
float combat_by_class[CLASS_COUNT + 1][2]{};
float pulse_all = 2.0F, shield_combat_mult = 1.0F, shield_combat_tank_mult = 1.0F;
int damroll_cap = 255, hitroll_cap = 127;

mm_ds *mm_create(const char *, size_t size, size_t next_offset, unsigned)
{
	assert(size == sizeof(affected_type) && next_offset == offsetof(affected_type, next));
	return &fixture_affect_allocator;
}
void *_mm_get(mm_ds *allocator, const char *, int)
{
	assert(allocator == &fixture_affect_allocator);
	for (std::size_t index = 0U; index < fixture_affect_pool.size(); ++index)
		if (!fixture_affect_used[index])
		{
			fixture_affect_used[index] = true;
			fixture_affect_pool[index] = {};
			return &fixture_affect_pool[index];
		}
	std::abort();
}
void mm_release(mm_ds *allocator, void *effect)
{
	assert(allocator == &fixture_affect_allocator);
	for (std::size_t index = 0U; index < fixture_affect_pool.size(); ++index)
		if (effect == &fixture_affect_pool[index])
		{
			assert(fixture_affect_used[index]);
			fixture_affect_used[index] = false;
			return;
		}
	std::abort();
}
nevent_schedule_result add_event(event_func callback, int delay, P_char character, P_char victim,
				 P_obj object, int, const void *payload, int payload_size)
{
	assert(callback && delay >= 0 && character && !victim && !object);
	assert(fixture_affect_event_count < fixture_affect_events.size());
	const auto index = fixture_affect_event_count++;
	auto &event = fixture_affect_events[index];
	event = {};
	event.ch = character;
	event.func = callback;
	event.sequence = index + 1U;
	event.due_tick = ne_event_tick + delay;
	event.next_char_nev = character->nevents;
	character->nevents = &event;
	if (payload_size != 0)
	{
		assert(payload && payload_size == sizeof(event_short_affect_data));
		std::memcpy(&fixture_affect_payloads[index], payload, payload_size);
		event.data = &fixture_affect_payloads[index];
		++fixture_affect_scheduled;
	}
	return { nevent_schedule_status::scheduled, { &event, event.sequence } };
}
P_nevent get_scheduled(P_char character, event_func_type callback)
{
	for (auto *event = character->nevents; event; event = event->next_char_nev)
		if (event->func == callback)
			return event;
	return nullptr;
}
nevent_handle nevent_handle_from_event(P_nevent event)
{
	return { event, event ? event->sequence : 0U };
}
nevent_cancel_result nevent_cancel(nevent_handle handle)
{
	assert(handle.event && handle.sequence == handle.event->sequence);
	handle.event->func = nullptr;
	++fixture_affect_canceled;
	return nevent_cancel_result::canceled;
}
void spell_ward_sync_timers(P_char) {}
void spell_ward_equipment_sync(P_char) {}
void spell_ward_mask_equipment_bits(unsigned long *) {}
void spell_ward_cancel_events(P_char, affected_type *)
{
	std::abort();
}
void spell_ward_expire(P_char, affected_type *)
{
	std::abort();
}
void unlink_char_affect(P_char, affected_type *)
{
	std::abort();
}
void unlink_char_obj_affect(P_char, affected_type *)
{
	std::abort();
}
void gmcp_char_affects(P_char) {}
int char_light(P_char)
{
	return 0;
}
int room_light(int, int)
{
	return 0;
}
void logit(const char *, const char *, ...) {}
void statuslog(int, const char *, ...) {}
void get_epic_stat_affects(P_char) {}
void get_aura_affects(P_char)
{
	std::abort();
}
P_char in_command_aura(P_char)
{
	return nullptr;
}
int add_racewar_stat_mods(P_char, hold_data *)
{
	return 0;
}
int calculate_hitpoints2(P_char character)
{
	return character->points.base_hit;
}
int calculate_mana(P_char)
{
	return 0;
}
int vitality_limit(P_char)
{
	return 100;
}
int two_weapon_check(P_char)
{
	return 0;
}
bool is_wielding_paladin_sword(P_char)
{
	return false;
}
bool innate_two_daggers(P_char)
{
	return false;
}
float get_property(const char *, double fallback)
{
	return fallback;
}
int get_property(const char *, int fallback)
{
	return fallback;
}
void apply_reaver_mods(P_char) {}
int GET_CHAR_SKILL_P(P_char, int)
{
	return 0;
}
int real_room0(int)
{
	return 0;
}
void StartRegen(P_char, regen_resource) {}
void song_broken(char_link_data *);
void set_ward_bits(P_char, const affected_type *, bool);
void do_wake(P_char character, char *, int)
{
	assert(character->telemetry_control_rebuild_depth == 0U);
	assert(telemetry_runtime_game_control_mask(character) == fixture_expected_wake_mask);
	++fixture_affect_wakes;
}
void do_stand(P_char, char *, int) {}
void stop_riding(P_char)
{
	std::abort();
}
void StopAllAttackers(P_char) {}
int NumAttackers(P_char)
{
	return 0;
}
void clear_links(P_char, ush_int type)
{
	assert(type == LNK_FLANKING || type == LNK_CIRCLING);
}
void character_maintenance_changed(P_char) {}
int dice(int count, int size)
{
	assert(count > 0 && size > 0);
	return count;
}
void do_alert(P_char, char *, int) {}
void die(P_char character, P_char)
{
	++fixture_affect_deaths;
	delete character; // ASan verifies the already-finished scope never reads it again.
}
#endif

namespace
{
struct fixture_native_lifetimes
{
	std::vector<P_char> characters;
	explicit fixture_native_lifetimes(std::initializer_list<P_char> values)
		: characters(values)
	{
		for (auto *character : characters)
		{
			if (!character->runtime_id)
				character->runtime_id = allocate_character_runtime_id();
			register_character_runtime_id(character);
		}
	}
	~fixture_native_lifetimes()
	{
		for (auto *character : characters)
			unregister_character_runtime_id(character);
	}
};
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
	std::vector<telemetry_record> builds;
	std::vector<telemetry_record> controls;
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
		if (records[index].header.kind == telemetry_record_kind::battle_build)
		{
			assert(telemetry_record_is_valid(records[index]));
			fake->builds.push_back(records[index]);
		}
		if (records[index].header.kind == telemetry_record_kind::control)
		{
			assert(telemetry_record_is_valid(records[index]));
			fake->controls.push_back(records[index]);
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
	assert(telemetry_runtime_game_battle_actor(&player, &player_context));
	assert(player_context.session.session_seq != 0U &&
	       player_context.encounter.sequence == 0U &&
	       !(player_context.quality_flags & TELEMETRY_QUALITY_CONTEXT_UNKNOWN));
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
	assert(player_context.session.session_seq == 0U &&
	       (player_context.quality_flags & TELEMETRY_QUALITY_CONTEXT_UNKNOWN));
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

void check_native_build_context(bool export_context = false)
{
	char_data player{}, npc{};
	pc_only_data player_pc{};
	npc_only_data npc_only{};
	player.only.pc = &player_pc;
	player_pc.pid = 8571;
	telemetry_battle_build_context context{};
	context.version = 999U;
	assert(!telemetry_runtime_game_battle_build_context(&player, &context));
	assert(context.version == 0U && context.actor.id == 0U && context.available == 0U);
	fake_repository fake{};
	const telemetry_transport_repository_binding repository = { fake_init, fake_apply,
								    fake_request_stop,
								    fake_shutdown, &fake };
	const telemetry_transport_clock_binding clock = { fake_clock, nullptr };
	assert(telemetry_transport_bind_for_tests(&repository, &clock) ==
	       telemetry_transport_outcome::started);
	auto options = enabled_options();
	options.config.build_version = 7U;
	options.config.content_version = 11U;
	assert(telemetry_config_compute_fingerprint(options.config, options.config.fingerprint,
						    sizeof(options.config.fingerprint)));
	options.config.config_id = telemetry_config_id_from_fingerprint(
		options.config.fingerprint, sizeof(options.config.fingerprint));
	telemetry_test_start_runtime(options);
	room_data room{};
	zone_data zone{};
	zone.number = 1703;
	world = &room;
	zone_table = &zone;
	top_of_world = top_of_zone_table = 0;
	player.in_room = 0;
	player.player.level = 56;
	player.player.m_class = 0x20000001U;
	player.player.secondary_class = 0x10000002U;
	player.player.spec = 3U;
	player.player.race = RACE_HUMAN;
	player.player.racewar = 2U;
	for (std::size_t index = 0U; index < TELEMETRY_BATTLE_BUILD_STATS; ++index)
	{
		player.base_stats[index] = 110 + index;
		player.curr_stats[index] = 230 + index;
	}
	player.points.base_hit = 70'000;
	player.points.max_hit = 123'456;
	player.points.hit = -2;
	player.points.base_mana = 70;
	player.points.max_mana = 120;
	player.points.mana = 105;
	player.points.base_vitality = 90;
	player.points.max_vitality = 150;
	player.points.vitality = 20;
	player.points.base_ward = 1;
	player.points.max_ward = 12;
	player.points.ward = 8;
	player.points.base_armor = 100;
	player.points.curr_armor = -15;
	player.points.base_hitroll = 0;
	player.points.hitroll = 21;
	player.points.base_damroll = -2;
	player.points.damroll = 35;
	const std::int8_t saves[5] = { -128, -1, 0, 42, 127 };
	for (std::size_t index = 0U; index < 5U; ++index)
		player.specials.apply_saving_throw[index] = saves[index];
	player.specials.affected_by = AFF_HASTE | AFF_PROT_FIRE;
	obj_data weapon{}, shield{}, armor{};
	obj_affect dynamic{};
	dynamic.next = &dynamic; // Fixed equipment extraction never follows this list.
	weapon.type = ITEM_WEAPON;
	weapon.material = 4;
	weapon.condition = 97;
	weapon.craftsmanship = -7;
	weapon.value[1] = 2;
	weapon.value[2] = 6;
	weapon.bitvector = 0x1234U;
	weapon.affected[0] = { APPLY_AC, -5 };
	weapon.affected[1] = { APPLY_HITROLL, 7 };
	weapon.affected[2] = { APPLY_DAMROLL, 11 };
	weapon.affected[3] = { APPLY_HIT, 13 };
	shield.type = ITEM_SHIELD;
	shield.value[3] = 25;
	shield.bitvector2 = 0x55U;
	shield.affected[0] = { APPLY_MANA, -2 };
	shield.affects = &dynamic;
	armor.type = ITEM_ARMOR;
	armor.bitvector5 = 0x10000000U;
	armor.affected[0] = { APPLY_HITROLL, -3 };
	player.equipment[0] = &weapon;
	player.equipment[1] = &shield;
	player.equipment[MAX_WEAR - 1] = &armor;
	skills[SKILL_TOUGHNESS].name = "fixture toughness";
	skills[SKILL_TOUGHNESS].targets = TAR_SKILL | TAR_EPIC;
	skills[SKILL_EPIC_AGILITY].name = "fixture agility";
	skills[SKILL_EPIC_AGILITY].targets = TAR_SKILL | TAR_EPIC;
	skills[SPELL_BLINDNESS].name = "fixture excluded epic spell";
	skills[SPELL_BLINDNESS].targets = TAR_SPELL | TAR_EPIC;
	player_pc.skills[SKILL_TOUGHNESS].learned = 50;
	player_pc.skills[SPELL_BLINDNESS].learned = 99;
	player_pc.skills[SKILL_MELEE_MASTERY].learned = 100;
	affected_type effects[3]{};
	effects[0].location = APPLY_HITROLL;
	effects[0].modifier = -7;
	effects[0].bitvector = AFF_HASTE;
	effects[0].context = reinterpret_cast<void *>(std::uintptr_t{ 1U });
	effects[0].next = &effects[1];
	effects[1].flags = AFFTYPE_NOAPPLY;
	effects[1].location = APPLY_MANA;
	effects[1].bitvector = AFF_BLIND;
	effects[1].next = &effects[2];
	effects[2].location = APPLY_AC;
	effects[2].bitvector2 = AFF2_PROT_LIGHTNING;
	player.affected = effects;
	auto read = [&](const char_data *character)
	{
		const auto before = fixture_crypto_heap_calls;
		fixture_track_crypto = true;
		const bool accepted =
			telemetry_runtime_game_battle_build_context(character, &context);
		fixture_track_crypto = false;
		assert(fixture_crypto_heap_calls == before);
		return accepted;
	};
	assert(read(&player));
	const auto original = context;
	assert(context.version == 1U && context.actor.id == 8571U &&
	       context.actor.kind == telemetry_combat_actor_kind::player &&
	       context.config_id == options.config.config_id && context.build_version == 7U &&
	       context.content_version == 11U && context.level == 56U &&
	       context.primary_class_mask == 0x20000001U &&
	       context.secondary_class_mask == 0x10000002U && context.specialization == 3U &&
	       context.race == RACE_HUMAN && context.faction == 2U);
	assert(context.available == 1023U &&
	       context.quality == TELEMETRY_BUILD_SUPPORT_ORIGIN_UNKNOWN);
	for (std::size_t index = 0U; index < TELEMETRY_BATTLE_BUILD_STATS; ++index)
		assert(context.base.stats[index] == 110 + static_cast<int>(index) &&
		       context.effective.stats[index] == 230 + static_cast<int>(index));
	assert(context.base.resources[0] == 70'000 && context.effective.resources[0] == 123'456 &&
	       context.current_resources[0] == -2 && context.base.resources[1] == 70 &&
	       context.effective.resources[1] == 120 && context.current_resources[1] == 105 &&
	       context.base.resources[2] == 90 && context.effective.resources[2] == 150 &&
	       context.current_resources[2] == 20 && context.base.resources[3] == 1 &&
	       context.effective.resources[3] == 12 && context.current_resources[3] == 8);
	assert(context.base.combat[0] == 100 && context.effective.combat[0] == -15 &&
	       context.base.combat[1] == 0 && context.effective.combat[1] == 21 &&
	       context.base.combat[2] == -2 && context.effective.combat[2] == 35);
	assert(std::memcmp(context.saving_modifiers, saves, sizeof(saves)) == 0 &&
	       context.effective_flags[0] == (AFF_HASTE | AFF_PROT_FIRE));
	assert(context.equipment.occupied_slots == 3U && context.equipment.melee_weapons == 1U &&
	       context.equipment.shields == 1U && context.equipment.armor == 1U &&
	       context.equipment.items_with_dynamic_affects == 1U);
	const std::int32_t modifiers[5] = { 13, -2, -5, 4, 11 };
	assert(std::memcmp(context.equipment.direct_modifiers, modifiers, sizeof(modifiers)) == 0 &&
	       context.equipment.flags[0] == 0x1234U && context.equipment.flags[1] == 0x55U &&
	       context.equipment.flags[4] == 0x10000000U);
	assert(context.epics.catalog_skills == 2U && context.epics.learned_skills == 1U &&
	       context.listed_affects.observed_nodes == 3U &&
	       context.listed_affects.offensive_modifier_nodes == 1U &&
	       context.listed_affects.armor_modifier_nodes == 1U &&
	       context.listed_affects.resource_modifier_nodes == 0U &&
	       context.listed_affects.unapplied_nodes == 1U &&
	       context.listed_affects.complete == 1U &&
	       context.listed_affects.flags[0] == AFF_HASTE &&
	       context.listed_affects.flags[1] == AFF2_PROT_LIGHTNING);
	assert(context.arena.membership == telemetry_battle_arena_membership::absent &&
	       context.arena.room_is_arena == 0U);
	if (export_context)
	{
		std::printf("BUILD_CONTEXT_JSON {\"snapshot_bytes\":%zu,\"content_version\":11,"
			    "\"crypto_heap_calls\":%llu,\"equipment_digest\":\"",
			    sizeof(context),
			    static_cast<unsigned long long>(fixture_crypto_heap_calls));
		for (auto byte : context.equipment.fixed_feature_digest)
			std::printf("%02x", byte);
		std::printf("\",\"epic_digest\":\"");
		for (auto byte : context.epics.learned_build_digest)
			std::printf("%02x", byte);
		std::puts("\"}");
	}
	const auto same_equipment = [&]()
	{
		return std::memcmp(context.equipment.fixed_feature_digest,
				   original.equipment.fixed_feature_digest, 32U) == 0;
	};
	const auto same_epics = [&]()
	{
		return std::memcmp(context.epics.learned_build_digest,
				   original.epics.learned_build_digest, 32U) == 0;
	};
	weapon.name = const_cast<char *>("renamed fixture");
	weapon.obj_uid = 12345;
	weapon.db_item_id = 77;
	weapon.R_num = 8;
	weapon.cost = 10000;
	player_pc.epics = 98765;
	player_pc.epic_skill_points = 54321;
	assert(read(&player) && same_equipment() && same_epics());
	++player.curr_stats.Str;
	assert(read(&player) && context.effective.stats[0] == 231 && context.base.stats[0] == 110 &&
	       same_equipment() && same_epics());
	--player.curr_stats.Str;
	++weapon.condition;
	assert(read(&player) && !same_equipment() && same_epics());
	--weapon.condition;
	player.equipment[2] = &weapon;
	player.equipment[0] = nullptr;
	assert(read(&player) && !same_equipment());
	player.equipment[0] = &weapon;
	player.equipment[2] = nullptr;
	player.equipment[1] = &weapon;
	assert(read(&player) && !(context.available & TELEMETRY_BUILD_FIXED_EQUIPMENT) &&
	       (context.quality & TELEMETRY_BUILD_EQUIPMENT_INVALID) &&
	       context.equipment.occupied_slots == 0U && context.effective.resources[0] == 123'456);
	player.equipment[1] = &shield;
	obj_data full_equipment[MAX_WEAR]{};
	for (std::size_t slot = 0U; slot < MAX_WEAR; ++slot)
	{
		full_equipment[slot].type = ITEM_WEAPON;
		full_equipment[slot].affected[0] = { APPLY_HITROLL, -128 };
		player.equipment[slot] = &full_equipment[slot];
	}
	assert(read(&player) && (context.available & TELEMETRY_BUILD_FIXED_EQUIPMENT) &&
	       context.equipment.occupied_slots == (MAX_WEAR) &&
	       context.equipment.melee_weapons == (MAX_WEAR) &&
	       context.equipment.direct_modifiers[3] == -128 * (MAX_WEAR));
	for (auto &object : player.equipment)
		object = nullptr;
	player.equipment[0] = &weapon;
	player.equipment[1] = &shield;
	player.equipment[MAX_WEAR - 1] = &armor;
	++player_pc.skills[SKILL_TOUGHNESS].learned;
	assert(read(&player) && same_equipment() && !same_epics());
	player_pc.skills[SKILL_TOUGHNESS].learned = -1;
	assert(read(&player) && !(context.available & TELEMETRY_BUILD_LEARNED_EPICS) &&
	       (context.quality & TELEMETRY_BUILD_EPICS_UNAVAILABLE) &&
	       context.epics.catalog_skills == 0U && context.epics.learned_skills == 0U);
	player_pc.skills[SKILL_TOUGHNESS].learned = 50;
	skills[SKILL_TOUGHNESS].targets = skills[SKILL_EPIC_AGILITY].targets = 0U;
	assert(read(&player) && !(context.available & TELEMETRY_BUILD_LEARNED_EPICS));
	skills[SKILL_TOUGHNESS].targets = skills[SKILL_EPIC_AGILITY].targets = TAR_SKILL | TAR_EPIC;
	for (int id = FIRST_SKILL; id <= LAST_SKILL; ++id)
	{
		skills[id].name = "fixture full epic catalog";
		skills[id].targets = TAR_SKILL | TAR_EPIC;
		player_pc.skills[id].learned = 1;
	}
	assert(read(&player) && (context.available & TELEMETRY_BUILD_LEARNED_EPICS) &&
	       context.epics.catalog_skills == LAST_SKILL - FIRST_SKILL + 1U &&
	       context.epics.learned_skills == context.epics.catalog_skills);
	for (int id = FIRST_SKILL; id <= LAST_SKILL; ++id)
		skills[id] = {};
	skills[SKILL_TOUGHNESS].name = "fixture toughness";
	skills[SKILL_EPIC_AGILITY].name = "fixture agility";
	skills[SKILL_TOUGHNESS].targets = skills[SKILL_EPIC_AGILITY].targets = TAR_SKILL | TAR_EPIC;
	player_pc.skills[SKILL_TOUGHNESS].learned = 50;
	player_pc.skills[SKILL_EPIC_AGILITY].learned = 0;
	effects[0].next = &effects[0];
	assert(read(&player) && context.listed_affects.observed_nodes == 1U &&
	       context.listed_affects.complete == 0U &&
	       (context.quality & TELEMETRY_BUILD_AFFECTS_CYCLIC));
	affected_type long_list[TELEMETRY_BATTLE_BUILD_MAX_AFFECTS + 1U]{};
	for (std::size_t index = 0U; index < TELEMETRY_BATTLE_BUILD_MAX_AFFECTS; ++index)
		long_list[index].next = &long_list[index + 1U];
	player.affected = long_list;
	assert(read(&player) && context.listed_affects.observed_nodes == 64U &&
	       context.listed_affects.complete == 0U &&
	       (context.quality & TELEMETRY_BUILD_AFFECTS_TRUNCATED));
	long_list[TELEMETRY_BATTLE_BUILD_MAX_AFFECTS - 1U].next = nullptr;
	assert(read(&player) && context.listed_affects.observed_nodes == 64U &&
	       context.listed_affects.complete == 1U &&
	       !(context.quality & TELEMETRY_BUILD_AFFECTS_TRUNCATED));
	player.affected = nullptr;
	room.room_flags |= ROOM_ARENA;
	assert(read(&player) && context.arena.room_is_arena == 1U &&
	       context.arena.membership == telemetry_battle_arena_membership::absent);
	arena.flags = FLAG_ENABLED;
	arena.stage = STAGE_MATCH;
	arena.type = TYPE_DEATHMATCH;
	arena.team[1].player[MAX_TEAM - 1].ch = &player;
	arena.team[1].player[MAX_TEAM - 1].flags = PLAYER_IT | PLAYER_DEAD;
	assert(read(&player) &&
	       context.arena.membership == telemetry_battle_arena_membership::member &&
	       context.arena.team == 2U &&
	       context.arena.player_flags == (PLAYER_IT | PLAYER_DEAD) &&
	       context.arena.stage == STAGE_MATCH && context.arena.type == TYPE_DEATHMATCH &&
	       context.arena.enabled == 1U);
	arena.stage = STAGE_OPEN;
	arena.flags = 0;
	assert(read(&player) &&
	       context.arena.membership == telemetry_battle_arena_membership::member &&
	       context.arena.stage == STAGE_OPEN && context.arena.enabled == 0U);
	arena.team[0].player[0].ch = &player;
	assert(read(&player) && !(context.available & TELEMETRY_BUILD_ARENA_ROSTER) &&
	       context.arena.membership == telemetry_battle_arena_membership::ambiguous &&
	       context.arena.team == 0U && context.arena.player_flags == 0 &&
	       (context.quality & TELEMETRY_BUILD_ARENA_INVALID));
	arena.team[0].player[0].ch = nullptr;
	arena.stage = STAGE_AFTERMATH + 1;
	assert(read(&player) && !(context.available & TELEMETRY_BUILD_ARENA_ROSTER) &&
	       context.arena.membership == telemetry_battle_arena_membership::unavailable);
	arena = {};
	player.in_room = -1;
	assert(read(&player) && !(context.available & TELEMETRY_BUILD_ARENA_ROOM) &&
	       (context.quality & TELEMETRY_BUILD_ROOM_UNAVAILABLE));
	player.in_room = 0;
	npc.only.npc = &npc_only;
	npc.specials.act = ACT_ISNPC;
	npc.in_room = 0;
	npc.runtime_id = allocate_character_runtime_id();
	assert(read(&npc) && context.actor.kind == telemetry_combat_actor_kind::npc &&
	       !(context.available & TELEMETRY_BUILD_LEARNED_EPICS) &&
	       (context.actor.id & TELEMETRY_BATTLE_NPC_GENERATION_TAG));
	const auto previous_lifetime = context.actor.id;
	npc.runtime_id = allocate_character_runtime_id();
	assert(read(&npc) && context.actor.id != previous_lifetime);
	fixture_pet = &npc;
	fixture_pet_master = &player;
	assert(read(&npc) && context.actor.kind == telemetry_combat_actor_kind::pet);
	fixture_pet = fixture_pet_master = nullptr;
	auto changed_config = options.config;
	++changed_config.revision;
	++changed_config.build_version;
	++changed_config.content_version;
	assert(telemetry_config_compute_fingerprint(changed_config, changed_config.fingerprint,
						    sizeof(changed_config.fingerprint)));
	changed_config.config_id = telemetry_config_id_from_fingerprint(
		changed_config.fingerprint, sizeof(changed_config.fingerprint));
	assert(telemetry_config_publish(changed_config).outcome ==
	       telemetry_runtime_outcome::accepted);
	assert(read(&player) && context.config_id == changed_config.config_id &&
	       context.build_version == 8U && context.content_version == 12U && !same_equipment() &&
	       !same_epics() && context.base.resources[0] == 70'000);
	player_pc.pid = 0;
	assert(!read(&player) && context.version == 0U && context.available == 0U &&
	       context.actor.id == 0U);
	assert(!read(nullptr));
	assert(!telemetry_runtime_game_battle_build_context(&npc, nullptr));
	telemetry_monotonic_usec now = 0U;
	telemetry_utc_usec utc = 0U;
	assert(telemetry_runtime_now(&now, &utc));
	assert(telemetry_runtime_shutdown({ now + 5'000'000U, 1U, {} }) ==
	       telemetry_runtime_outcome::accepted);
	assert(telemetry_runtime_final_reap() == telemetry_runtime_outcome::accepted);
	telemetry_transport_unbind_for_tests();
	assert(fake.battles.empty() && fake.contributions.empty() && fake.lifecycles.empty() &&
	       fake.encounters.empty() && player.telemetry_session_sequence == 0U);
	skills[SKILL_TOUGHNESS] = skills[SKILL_EPIC_AGILITY] = skills[SPELL_BLINDNESS] = {};
	world = nullptr;
	zone_table = nullptr;
	top_of_world = top_of_zone_table = -1;
	std::puts(
		"PASS: bounded native build snapshots, fixed feature/epic fingerprints, unavailable families, arena distinction and zero crypto heap calls");
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

void export_native_control_capture(const fake_repository &fake)
{
	const char *path = std::getenv("TELEMETRY_CONTROL_CAPTURE_EXPORT");
	if (!path)
		return;
	auto *output = std::fopen(path, "wb");
	assert(output);
	for (const auto &record : fake.controls)
	{
		std::fprintf(
			output,
			"{\"boot_id\":%llu,\"process_id\":%llu,\"record_seq\":%llu,\"record_kind\":13,\"schema_version\":1,\"occurrence_utc_usec\":%lld",
			static_cast<unsigned long long>(record.header.key.producer.boot_id),
			static_cast<unsigned long long>(record.header.key.producer.process_id),
			static_cast<unsigned long long>(record.header.key.record_seq),
			static_cast<long long>(record.header.occurrence_utc_usec));
#define TELEMETRY_CONTROL_FIELD(name, member, width, is_signed)                      \
	if constexpr (is_signed)                                                     \
		std::fprintf(output, ",\"" #name "\":%lld",                          \
			     static_cast<long long>(record.payload.control.member)); \
	else                                                                         \
		std::fprintf(output, ",\"" #name "\":%llu",                          \
			     static_cast<unsigned long long>(record.payload.control.member));
#include "telemetry/telemetry_control_fields.inc"
#undef TELEMETRY_CONTROL_FIELD
		std::fputs("}\n", output);
	}
	assert(std::fclose(output) == 0);
}

void export_native_battle_capture(const fake_repository &fake)
{
	export_native_control_capture(fake);
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
	fixture_native_lifetimes lifetimes{ &attacker, &target, &unrelated, &npc, &pet };
	npc.only.npc = &npc_data;
	pet.only.npc = &pet_data;
	npc_data.R_num = pet_data.R_num = -1; // Native lifetime exists; legacy ID is absent.
	for (auto *actor : { &npc, &pet })
	{
		actor->specials.act = ACT_ISNPC;
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

fake_repository check_native_expanded_control_capture(bool use_native = false,
						      bool export_capture = false)
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
	zones[0].number = 1908;
	rooms[0].sector_type = SECT_FIELD;
	const auto previous_world = world;
	const auto previous_zones = zone_table;
	const int previous_top_world = top_of_world, previous_top_zone = top_of_zone_table;
	world = rooms;
	zone_table = zones;
	top_of_world = top_of_zone_table = 0;
	char_data attacker{}, target{}, rejected{}, npc{}, pet{};
	pc_only_data attacker_data{}, target_data{}, rejected_data{};
	npc_only_data npc_data{}, pet_data{};
	attacker.only.pc = &attacker_data;
	target.only.pc = &target_data;
	rejected.only.pc = &rejected_data;
	attacker_data.pid = 8971;
	target_data.pid = 8972;
	rejected_data.pid = 8973;
	fixture_native_lifetimes lifetimes{ &attacker, &target, &rejected, &npc, &pet };
	npc.only.npc = &npc_data;
	pet.only.npc = &pet_data;
	npc_data.R_num = pet_data.R_num = -1;
	for (auto *actor : { &npc, &pet })
	{
		actor->specials.act = ACT_ISNPC;
	}
	for (auto *actor : { &attacker, &target, &rejected, &npc, &pet })
	{
		actor->in_room = 0;
		actor->player.level = 40;
		actor->player.m_class = CLASS_WARRIOR;
		actor->player.race = RACE_HUMAN;
		actor->player.racewar = 5;
		actor->specials.position = STAT_NORMAL;
	}
	fixture_control_effects.clear();
	fixture_control_mutations = fixture_control_stops = 0U;
	fixture_control_call = 0U;
	fixture_control_resistance = fixture_control_freedom = false;
	fixture_control_random = 0;
	auto save = [](bool succeeds)
	{
		fixture_control_saves = { succeeds };
		fixture_control_save_index = 0U;
	};
	auto clear = [](P_char actor)
	{
		actor->specials.affected_by = actor->specials.affected_by2 = 0U;
		actor->specials.position = STAT_NORMAL;
		GET_OPPONENT(actor) = nullptr;
	};
	auto major = [&](P_char actor, int level = 40)
	{
		fixture_control_route = "major";
		++fixture_control_call;
		spell_major_paralysis(level, &attacker, nullptr, 0, actor, nullptr);
	};
	auto minor = [&](P_char actor, P_char source = nullptr)
	{
		fixture_control_route = "minor";
		++fixture_control_call;
		spell_minor_paralysis(40, source ? source : &attacker, nullptr, 0, actor, nullptr);
	};
	auto slow = [&](P_char actor)
	{
		fixture_control_route = "slow";
		++fixture_control_call;
		spell_slow(40, &attacker, nullptr, 0, actor, nullptr);
	};
	auto sleep = [&](P_char actor, int level = 40, P_char source = nullptr)
	{
		fixture_control_route = "sleep";
		++fixture_control_call;
		spell_sleep(level, source ? source : &attacker, nullptr, 0, actor, nullptr);
	};
	auto silence = [&](P_char actor)
	{
		fixture_control_route = "silence";
		++fixture_control_call;
		spell_silence(40, &attacker, nullptr, 0, actor, nullptr);
	};
	auto entangle = [&](P_char actor)
	{
		fixture_control_route = "entangle";
		++fixture_control_call;
		spell_entangle(40, &attacker, nullptr, 0, actor, nullptr);
	};
	// Use an actor that never contributes later to prove rejections cannot admit
	// hostile/shared presence. Existing game-service retaliation is isolated.
	fixture_control_resistance = true;
	major(&rejected);
	minor(&rejected);
	slow(&rejected);
	sleep(&rejected);
	fixture_control_resistance = false;
	fixture_control_freedom = true;
	major(&rejected);
	minor(&rejected);
	entangle(&rejected);
	fixture_control_freedom = false;
	save(true);
	major(&rejected);
	save(true);
	minor(&rejected);
	save(true);
	slow(&rejected);
	save(true);
	sleep(&rejected);
	save(true);
	entangle(&rejected);
	rejected.specials.affected_by2 = AFF2_SLOW;
	slow(&rejected);
	rejected.specials.affected_by2 = 0U;
	rejected.player.m_class = CLASS_MONK;
	slow(&rejected);
	rejected.player.m_class = CLASS_WARRIOR;
	for (int race : { RACE_DEMON, RACE_SKELETON, RACE_F_ELEMENTAL })
	{
		rejected.player.race = race;
		save(false);
		sleep(&rejected);
	}
	rejected.player.race = RACE_HUMAN;
	rejected.player.level = 56;
	save(false);
	sleep(&rejected);
	rejected.player.level = 40;
	obj_data no_sleep{};
	no_sleep.extra_flags = ITEM_NOSLEEP;
	rejected.equipment[MAX_WEAR - 1] = &no_sleep;
	sleep(&rejected);
	rejected.equipment[MAX_WEAR - 1] = nullptr;
	npc.specials.act |= ACT_IMMUNE_TO_PARA;
	major(&npc);
	minor(&npc);
	save(false);
	entangle(&npc);
	npc.specials.act &= ~ACT_IMMUNE_TO_PARA;
	rooms[0].room_flags = ROOM_INDOORS;
	entangle(&rejected);
	rooms[0].room_flags = 0U;
	rooms[0].sector_type = SECT_INSIDE;
	entangle(&rejected);
	rooms[0].sector_type = SECT_FIELD;
	rejected.specials.position = STAT_DEAD;
	entangle(&rejected);
	silence(&rejected);
	rejected.specials.position = STAT_NORMAL;
	rejected.player.level = MAXLVLMORTAL + 1;
	entangle(&rejected);
	rejected.player.level = 40;
	rejected.specials.affected_by2 = AFF2_MINOR_PARALYSIS;
	entangle(&rejected);
	rejected.specials.affected_by2 = 0U;
	affected_type existing_entangle{};
	existing_entangle.type = SPELL_ENTANGLE;
	rejected.affected = &existing_entangle;
	entangle(&rejected);
	rejected.affected = nullptr;
	// A zero silence percentage, active silence, immunity and resistance reject.
	silence(&rejected);
	rejected.specials.apply_saving_throw[SAVING_SPELL] = 50;
	rejected.specials.affected_by2 = AFF2_SILENCED;
	silence(&rejected);
	rejected.specials.affected_by2 = 0U;
	rejected.specials.act = ACT_ELITE;
	silence(&rejected);
	rejected.specials.act = 0U;
	npc.specials.apply_saving_throw[SAVING_SPELL] = 50;
	npc.player.race = RACE_DEMON;
	silence(&npc);
	npc.player.race = RACE_HUMAN;
	fixture_control_resistance = true;
	silence(&rejected);
	fixture_control_resistance = false;
	attacker.specials.position = STAT_DEAD;
	major(&rejected);
	slow(&rejected);
	sleep(&rejected);
	silence(&rejected);
	attacker.specials.position = STAT_NORMAL;
	assert(fixture_control_effects.empty() && fixture_control_mutations == 0U);
	// Accepted self sleep outside shared participation remains outside a battle.
	save(false);
	sleep(&rejected, 40, &rejected);
	assert(fixture_control_mutations == 1U);
	// Each actual accepted mutation supplies one application and one received
	// count. Refreshes are accepted applications, never extra disabled duration.
	save(false);
	target.specials.fighting = &attacker;
	major(&target);
	assert(!IS_FIGHTING(&target) && IS_AFFECTED2(&target, AFF2_MAJOR_PARALYSIS));
	clear(&target);
	save(false);
	minor(&target);
	assert(IS_AFFECTED2(&target, AFF2_MINOR_PARALYSIS));
	clear(&target);
	save(false);
	slow(&target);
	assert(IS_AFFECTED2(&target, AFF2_SLOW));
	clear(&target);
	save(false);
	target.specials.fighting = &attacker;
	sleep(&target);
	assert(!IS_FIGHTING(&target) && IS_AFFECTED(&target, AFF_SLEEP));
	save(false);
	sleep(&target); // Existing sleep refresh is exactly one accepted application.
	clear(&target);
	for (int saving_modifier : { 50, 40, 25, 10 })
	{
		target.specials.apply_saving_throw[SAVING_SPELL] = saving_modifier;
		silence(&target);
		assert(IS_AFFECTED2(&target, AFF2_SILENCED));
		assert(fixture_control_effects.back().duration == (saving_modifier == 50 ? 10 :
								   saving_modifier == 40 ? 8 :
								   saving_modifier == 25 ? 5 :
											   3) *
									  WAIT_SEC);
		clear(&target);
	}
	save(false);
	target.specials.fighting = &attacker;
	entangle(&target);
	assert(!IS_FIGHTING(&target) && IS_AFFECTED2(&target, AFF2_MINOR_PARALYSIS));
	clear(&target);
	rooms[0].sector_type = SECT_FOREST;
	fixture_control_random = 1;
	save(false);
	entangle(&target);
	assert(IS_AFFECTED(&target, AFF_BOUND));
	clear(&target);
	rooms[0].sector_type = SECT_FIELD;
	fixture_control_random = 0;
	major(&target, -40); // Negative level retains the actual saving-throw bypass.
	clear(&target);
	attacker.player.level = MAXLVLMORTAL + 1;
	fixture_control_resistance = fixture_control_freedom = true;
	major(&target); // Trusted source preserves its existing immunity bypass.
	attacker.player.level = 40;
	fixture_control_resistance = fixture_control_freedom = false;
	clear(&target);
	save(false);
	minor(&target, &target); // Existing battle: one actor, SELF modifier.
	clear(&target);
	sleep(&target, -40); // Negative-level sleep bypasses save/race/level gates.
	save(false);
	slow(&npc);
	fixture_pet = &pet;
	fixture_pet_master = &attacker;
	clear(&target);
	save(false);
	minor(&target, &pet);
	assert(fixture_control_mutations == 17U && fixture_control_stops == 3U);
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
	unsigned starts = 0U, self = 0U, pet_segments = 0U, npc_segments = 0U;
	for (const auto &record : fake.battles)
	{
		assert(record.payload.battle.actor.actor.actor_id != 8973U);
		starts += record.payload.battle.kind == telemetry_battle_fact_kind::start;
	}
	for (const auto &record : fake.contributions)
	{
		const auto &row = record.payload.battle_contribution;
		const auto &actor = row.context.actor.actor;
		assert(actor.actor_id != 8973U &&
		       row.context.available_metrics == TELEMETRY_BC_METRICS);
		assert(!(row.quality_flags &
			 (TELEMETRY_QUALITY_QUEUE_DROP | TELEMETRY_QUALITY_CLOCK_DISCONTINUITY)));
		applications += row.counters.control_applications;
		received += row.counters.control_received;
		if (row.modifier_flags & TELEMETRY_COMBAT_MODIFIER_SELF)
		{
			// Modifier flags are a segment union. Later received applications
			// can share the segment; SELF does not apportion those counters.
			assert(actor.actor_id == 8972U && row.counters.control_applications == 1U &&
			       row.counters.control_received >= 1U);
			++self;
		}
		if (actor.kind == telemetry_combat_actor_kind::pet)
		{
			assert(actor.owner_subject_id == 8971U &&
			       row.counters.control_applications == 1U);
			++pet_segments;
		}
		if (actor.kind == telemetry_combat_actor_kind::npc && row.counters.control_received)
		{
			assert(actor.actor_id ==
				       (TELEMETRY_BATTLE_NPC_GENERATION_TAG | npc.runtime_id) &&
			       row.counters.control_received == 1U);
			++npc_segments;
		}
	}
	assert(starts == 1U && applications == 17U && received == applications && self == 1U &&
	       pet_segments == 1U && npc_segments == 1U);
	unsigned resolutions = 0U, applied = 0U, rejected_outside = 0U, refreshes = 0U,
		 bypassed = 0U, bound = 0U, declared_silence = 0U;
	std::set<telemetry_control_result> rejections;
	for (const auto &record : fake.controls)
	{
		const auto &value = record.payload.control;
		assert(value.duration_coverage ==
		       (value.kind == telemetry_control_kind::state_entry ||
					value.kind == telemetry_control_kind::state_interval ?
				255U :
				0U));
		if (value.kind != telemetry_control_kind::resolution)
		{
			assert(value.source.actor.actor_id == 0U &&
			       value.target.actor.actor_id != 8973U);
			continue;
		}
		++resolutions;
		applied += value.result == telemetry_control_result::applied;
		refreshes += (value.flags & TELEMETRY_CONTROL_SLEEP_REFRESH) != 0U;
		bypassed += (value.flags & TELEMETRY_CONTROL_SAVE_BYPASSED) != 0U;
		if (value.result != telemetry_control_result::applied)
		{
			assert(value.configured_ticks == 0 && !(value.flags & 28U));
			rejections.insert(value.result);
			if (value.target.actor.actor_id == 8973U)
			{
				assert(value.source_association.battle_sequence == 0U &&
				       value.target_association.battle_sequence == 0U);
				++rejected_outside;
			}
		}
		if (value.family == telemetry_control_family::entangle &&
		    value.result == telemetry_control_result::applied && (value.after_mask & 128U))
		{
			assert(value.configured_ticks == 0);
			++bound;
		}
		if (value.family == telemetry_control_family::silence &&
		    value.result == telemetry_control_result::applied)
		{
			assert(value.configured_ticks == 10 * WAIT_SEC ||
			       value.configured_ticks == 8 * WAIT_SEC ||
			       value.configured_ticks == 5 * WAIT_SEC ||
			       value.configured_ticks == 3 * WAIT_SEC);
			++declared_silence;
		}
	}
	assert(resolutions == fixture_control_call && applied == 18U && rejected_outside > 20U &&
	       refreshes == 1U && bypassed == 3U && bound == 1U && declared_silence == 4U);
	for (const auto expected :
	     { telemetry_control_result::saved, telemetry_control_result::resisted,
	       telemetry_control_result::immune, telemetry_control_result::already_present,
	       telemetry_control_result::movement_protection,
	       telemetry_control_result::target_protected,
	       telemetry_control_result::source_ineligible,
	       telemetry_control_result::target_ineligible,
	       telemetry_control_result::location_ineligible,
	       telemetry_control_result::level_ineligible,
	       telemetry_control_result::class_ineligible,
	       telemetry_control_result::percentage_rejected })
		assert(rejections.contains(expected));
	std::printf(
		"PASS: %u native typed resolutions, %u accepted, 12 actual rejection reasons, refresh/bypass/bound declarations and selected-state coverage independent of context quality\n",
		resolutions, applied);
	if (export_capture)
		export_native_battle_capture(fake);
	fixture_pet = fixture_pet_master = nullptr;
	fixture_control_resistance = fixture_control_freedom = false;
	world = previous_world;
	zone_table = previous_zones;
	top_of_world = previous_top_world;
	top_of_zone_table = previous_top_zone;
	std::puts(
		"PASS: accepted paralysis/slow/sleep/silence/entangle bodies, actual rejection and bypass gates, refresh/self/pet/NPC isolation and conserved 17/17 control");
	return fake;
}

void export_native_build_capture(const fake_repository &fake)
{
	const char *path = std::getenv("TELEMETRY_BUILD_CAPTURE_EXPORT");
	if (!path)
		return;
	auto *file = std::fopen(path, "wb");
	assert(file);
	for (const auto &record : fake.builds)
	{
		std::fprintf(
			file,
			"{\"boot_id\":%llu,\"process_id\":%llu,\"record_seq\":%llu,\"record_kind\":12,\"schema_version\":1,\"occurrence_utc_usec\":%lld",
			static_cast<unsigned long long>(record.header.key.producer.boot_id),
			static_cast<unsigned long long>(record.header.key.producer.process_id),
			static_cast<unsigned long long>(record.header.key.record_seq),
			static_cast<long long>(record.header.occurrence_utc_usec));
#define TELEMETRY_BUILD_FIELD(name, member, width, is_signed)                             \
	if constexpr (is_signed)                                                          \
		std::fprintf(file, ",\"" #name "\":%lld",                                 \
			     static_cast<long long>(record.payload.battle_build.member)); \
	else                                                                              \
		std::fprintf(file, ",\"" #name "\":%llu",                                 \
			     static_cast<unsigned long long>(record.payload.battle_build.member));
#define TELEMETRY_BUILD_BYTES(name, member, width)           \
	std::fprintf(file, ",\"" #name "\":\"");             \
	for (auto byte : record.payload.battle_build.member) \
		std::fprintf(file, "%02x", byte);            \
	std::fputc('"', file);
#include "telemetry/telemetry_battle_build_fields.inc"
#undef TELEMETRY_BUILD_FIELD
#undef TELEMETRY_BUILD_BYTES
		std::fputs("}\n", file);
	}
	assert(std::fclose(file) == 0);
}

void check_native_build_capture(bool use_native = false, bool export_capture = false)
{
	fake_repository fake{};
	fake.use_native = use_native;
	const telemetry_transport_repository_binding repository = { fake_init, fake_apply,
								    fake_request_stop,
								    fake_shutdown, &fake };
	const telemetry_transport_clock_binding clock = { fake_clock, nullptr };
	assert(telemetry_transport_bind_for_tests(&repository, &clock) ==
	       telemetry_transport_outcome::started);
	auto options = enabled_options();
	options.config.context_segments_per_minute = 64U;
	options.config.build_version = 7U;
	options.config.content_version = 11U;
	assert(telemetry_config_compute_fingerprint(options.config, options.config.fingerprint,
						    sizeof(options.config.fingerprint)));
	options.config.config_id = telemetry_config_id_from_fingerprint(
		options.config.fingerprint, sizeof(options.config.fingerprint));
	telemetry_test_start_runtime(options);
	const auto config = telemetry_config_snapshot_copy();
	room_data room{};
	zone_data zone{};
	zone.number = 1977;
	world = &room;
	zone_table = &zone;
	top_of_world = top_of_zone_table = 0;
	char_data player{}, target{}, present{};
	pc_only_data player_pc{}, target_pc{}, present_pc{};
	player.only.pc = &player_pc;
	target.only.pc = &target_pc;
	present.only.pc = &present_pc;
	player_pc.pid = 8971;
	target_pc.pid = 8972;
	present_pc.pid = 8973;
	for (auto *actor : { &player, &target, &present })
	{
		actor->runtime_id = allocate_character_runtime_id();
		actor->in_room = 0;
		actor->player.level = 51;
		actor->player.m_class = 3U;
		actor->player.race = RACE_HUMAN;
		actor->player.racewar = 2U;
	}
	player.player.secondary_class = 4U;
	player.player.spec = 3U;
	player.base_stats.Str = 100;
	player.curr_stats.Str = 130;
	player.points.base_hit = 1000;
	player.points.max_hit = 2000;
	player.points.hit = 1800;
	obj_data weapon{};
	weapon.type = ITEM_WEAPON;
	weapon.condition = 90;
	weapon.affected[0] = { APPLY_DAMROLL, 7 };
	player.equipment[0] = &weapon;
	skills[SKILL_TOUGHNESS].name = "fixture epic";
	skills[SKILL_TOUGHNESS].targets = TAR_SKILL | TAR_EPIC;
	player_pc.skills[SKILL_TOUGHNESS].learned = 40;
	group_list last{ &present, nullptr }, group{ &player, &last };
	player.group = present.group = &group;
	character_list = &player;
	player.next = &target;
	target.next = &present;
	assert(telemetry_runtime_game_combat_engage(&player, &target).outcome ==
	       telemetry_runtime_outcome::accepted);
	for (unsigned hit = 0U; hit < 100U; ++hit)
		telemetry_runtime_game_combat_damage(&player, &target, 1U, 0U);
	// First observed change copies the actual new selected values.
	weapon.condition = 91;
	player.curr_stats.Str = 140;
	telemetry_runtime_game_battle_build_changed(&player);
	assert(telemetry_runtime_game_battle_context(&player).records_emitted == 1U);
	// Coalescing an equal mutation still consumes one bounded read, no point.
	telemetry_runtime_game_battle_build_changed(&player);
	assert(telemetry_runtime_game_battle_context(&player).records_emitted == 0U);
	player_pc.skills[SKILL_TOUGHNESS].learned = 41;
	telemetry_runtime_game_battle_build_changed(&player);
	assert(telemetry_runtime_game_battle_context(&player).records_emitted == 1U);
	player.equipment[1] = &weapon;
	telemetry_runtime_game_battle_build_changed(&player);
	assert(telemetry_runtime_game_battle_context(&player).records_emitted == 1U);
	player.equipment[1] = nullptr;
	telemetry_runtime_game_battle_build_changed(&player);
	assert(telemetry_runtime_game_battle_context(&player).records_emitted == 1U);
	// A new qualified config publishes a fresh point under its exact identity.
	auto changed = config;
	++changed.revision;
	++changed.content_version;
	assert(telemetry_config_compute_fingerprint(changed, changed.fingerprint,
						    sizeof(changed.fingerprint)));
	changed.config_id = telemetry_config_id_from_fingerprint(changed.fingerprint,
								 sizeof(changed.fingerprint));
	assert(telemetry_config_publish(changed).outcome == telemetry_runtime_outcome::accepted);
	assert(telemetry_runtime_game_battle_context(&player).outcome ==
	       telemetry_runtime_outcome::accepted);
	// Withdrawal has no retained profile or fictitious config/build/content.
	telemetry_config_reload_observer(telemetry_config_global_state());
	assert(telemetry_runtime_game_battle_context(&player).outcome ==
	       telemetry_runtime_outcome::invalid);
	++changed.revision;
	++changed.policy_version;
	assert(telemetry_config_compute_fingerprint(changed, changed.fingerprint,
						    sizeof(changed.fingerprint)));
	changed.config_id = telemetry_config_id_from_fingerprint(changed.fingerprint,
								 sizeof(changed.fingerprint));
	assert(telemetry_config_publish(changed).outcome == telemetry_runtime_outcome::accepted);
	telemetry_config_reload_request_clear(telemetry_config_global_state());
	assert(telemetry_runtime_game_battle_context(&player).outcome ==
	       telemetry_runtime_outcome::accepted);
	// A live NPC vanishes without its ordinary teardown callback. The cache
	// keeps only an opaque address, so the later world scan cannot read it.
	auto missing = std::make_unique<char_data>();
	npc_only_data npc_only{};
	missing->specials.act = ACT_ISNPC;
	missing->only.npc = &npc_only;
	missing->runtime_id = allocate_character_runtime_id();
	missing->in_room = 0;
	missing->player.level = 15;
	const auto old_npc_id = TELEMETRY_BATTLE_NPC_GENERATION_TAG | missing->runtime_id;
	present.next = missing.get();
	telemetry_runtime_game_combat_damage(&player, missing.get(), 1U, 0U);
	assert(telemetry_runtime_game_battle_leave(missing.get()).outcome ==
	       telemetry_runtime_outcome::accepted);
	missing->runtime_id = allocate_character_runtime_id();
	missing->player.level = 31;
	const auto missing_id = TELEMETRY_BATTLE_NPC_GENERATION_TAG | missing->runtime_id;
	fixture_pet = missing.get();
	fixture_pet_master = &player;
	telemetry_runtime_game_combat_damage(&player, missing.get(), 1U, 0U);
	fixture_pet = fixture_pet_master = nullptr;
	present.next = nullptr;
	missing.reset();
	player.next = &present; // A live PC also becomes temporarily unresolvable.
	telemetry_monotonic_usec now = 0U;
	telemetry_utc_usec utc = TELEMETRY_UTC_UNKNOWN;
	assert(telemetry_runtime_now(&now, &utc));
	assert(telemetry_runtime_pulse({ now + 11'000'000U, utc + 11'000'000, 0U, 0U }).outcome ==
	       telemetry_runtime_outcome::accepted);
	player.next = &target;
	assert(telemetry_runtime_pulse({ now + 12'000'000U, utc + 12'000'000, 0U, 0U }).outcome ==
	       telemetry_runtime_outcome::accepted);
	// Complete the fixture's future clock before shutdown. Build sampling does
	// not keep the battle alive or extend its measured engagement prefix.
	assert(telemetry_runtime_pulse({ now + 50'000'000U, utc + 50'000'000, 0U, 0U }).outcome ==
	       telemetry_runtime_outcome::accepted);
	assert(telemetry_runtime_shutdown({ now + 5'000'000U, 1U, {} }) ==
	       telemetry_runtime_outcome::accepted);
	assert(telemetry_runtime_final_reap() == telemetry_runtime_outcome::accepted);
	std::set<telemetry_sequence> sequences;
	unsigned entries = 0U, mutations = 0U, configs = 0U, withdrawn = 0U, resumed = 0U,
		 periodic = 0U, missing_points = 0U, target_gaps = 0U, lifetime_points = 0U;
	telemetry_battle_build_observation first{}, gear{}, epic{}, partial{};
	for (const auto &record : fake.builds)
	{
		const auto &value = record.payload.battle_build;
		assert(sequences.insert(value.sequence).second);
		assert(std::any_of(fake.battles.begin(), fake.battles.end(),
				   [&](const auto &basis)
				   {
					   const auto &fact = basis.payload.battle;
					   return fact.battle.sequence == value.battle.sequence &&
						  fact.revision == value.association_revision &&
						  fact.fact_sequence ==
							  value.association_fact_sequence &&
						  fact.at_monotonic_usec <= value.at_monotonic_usec;
				   }));
		if (value.boundary == telemetry_battle_build_boundary::actor_entry)
		{
			++entries;
			if (value.actor_id == 8971U)
				first = value;
			if (value.actor_id == old_npc_id)
			{
				assert(value.actor_kind == telemetry_combat_actor_kind::npc &&
				       value.level == 15U);
				++lifetime_points;
			}
			if (value.actor_id == missing_id)
			{
				assert(value.actor_kind == telemetry_combat_actor_kind::pet &&
				       value.level == 31U && value.primary_class_mask == 0U &&
				       !(value.available & TELEMETRY_BUILD_LEARNED_EPICS));
				++lifetime_points;
			}
		}
		if (value.boundary == telemetry_battle_build_boundary::actor_changed)
		{
			++mutations;
			if (mutations == 1U)
				gear = value;
			if (mutations == 2U)
				epic = value;
			if (mutations == 3U)
				partial = value;
		}
		configs += value.boundary == telemetry_battle_build_boundary::configuration_changed;
		resumed += value.boundary == telemetry_battle_build_boundary::source_resumed;
		periodic += value.boundary == telemetry_battle_build_boundary::periodic_sample;
		if (value.boundary == telemetry_battle_build_boundary::configuration_unavailable)
		{
			++withdrawn;
			assert(value.config_id == 0U && value.build_version == 0U &&
			       value.content_version == 0U);
		}
		if (value.actor_id == missing_id &&
		    value.boundary == telemetry_battle_build_boundary::source_unavailable)
			++missing_points;
		if (value.actor_id == 8972U &&
		    value.boundary == telemetry_battle_build_boundary::source_unavailable)
			++target_gaps;
		if (value.status == telemetry_battle_build_status::unavailable)
			assert(telemetry_battle_build_detail::empty_profile(value));
	}
	assert(entries == 5U && mutations == 4U && configs == 1U && withdrawn == 3U &&
	       resumed == 3U && periodic == 2U && missing_points == 1U && target_gaps == 1U &&
	       lifetime_points == 2U);
	assert(first.primary_class_mask == 3U && first.secondary_class_mask == 4U &&
	       first.specialization == 3U && first.base_stats[0] == 100 &&
	       first.effective_stats[0] == 130 && first.current_resources[0] == 1800 &&
	       first.base_resources[0] == 1000 && first.effective_resources[0] == 2000 &&
	       first.equipment_counts[0] == 1U);
	assert(gear.effective_stats[0] == 140 &&
	       std::memcmp(first.equipment_digest, gear.equipment_digest, 32U) != 0 &&
	       std::memcmp(first.epic_digest, gear.epic_digest, 32U) == 0);
	assert(std::memcmp(gear.epic_digest, epic.epic_digest, 32U) != 0);
	assert(!(partial.available & TELEMETRY_BUILD_FIXED_EQUIPMENT) &&
	       partial.effective_stats[0] == 140 &&
	       (partial.context_quality & TELEMETRY_BUILD_EQUIPMENT_INVALID));
	assert(std::count_if(fake.battles.begin(), fake.battles.end(),
			     [](const auto &record) {
				     return record.payload.battle.kind ==
					    telemetry_battle_fact_kind::close;
			     }) == 1);
	if (export_capture)
	{
		export_native_build_capture(fake);
		export_native_battle_capture(fake);
	}
	character_list = nullptr;
	world = nullptr;
	zone_table = nullptr;
	top_of_world = top_of_zone_table = -1;
	skills[SKILL_TOUGHNESS] = {};
	std::printf(
		"PASS: native build capture exact association, gear/epic changes, partial families, config withdrawal/recovery, periodic lifetime safety; records=%zu\n",
		fake.builds.size());
}

#ifdef TELEMETRY_TEST_NATIVE_AFFECTS
void check_native_affect_mutations()
{
	fake_repository fake{};
	const telemetry_transport_repository_binding repository = { fake_init, fake_apply,
								    fake_request_stop,
								    fake_shutdown, &fake };
	const telemetry_transport_clock_binding clock = { fake_clock, nullptr };
	assert(telemetry_transport_bind_for_tests(&repository, &clock) ==
	       telemetry_transport_outcome::started);
	telemetry_test_start_runtime(enabled_options());
	room_data room{};
	zone_data zone{};
	zone.number = 1979;
	world = &room;
	zone_table = &zone;
	top_of_world = top_of_zone_table = 0;
	char_data player{}, target{};
	pc_only_data pc{}, target_pc{};
	pc.pid = 8988;
	target_pc.pid = 8989;
	fixture_native_lifetimes lifetimes{ &player, &target };
	player.only.pc = &pc;
	target.only.pc = &target_pc;
	player.in_room = target.in_room = 0;
	player.next = &target;
	character_list = &player;
	player.player.level = target.player.level = 20;
	player.player.m_class = target.player.m_class = CLASS_WARRIOR;
	target.player.race = RACE_HUMAN;
	player.points.base_hit = target.points.base_hit = 300;
	player.points.max_hit = target.points.max_hit = 300;
	player.points.hit = target.points.hit = 300;
	assert(telemetry_runtime_game_combat_engage(&player, &target).outcome ==
	       telemetry_runtime_outcome::accepted);
	const unsigned long first[] = { AFF_BLIND, 0U, 0U, 0U, 0U, AFF_SLEEP, 0U, AFF_BOUND };
	const unsigned long second[] = {
		0U,	   AFF2_STUNNED, AFF2_MAJOR_PARALYSIS, AFF2_MINOR_PARALYSIS,
		AFF2_SLOW, 0U,		 AFF2_SILENCED,	       0U
	};
	std::vector<std::pair<std::uint16_t, std::uint16_t>> expected;
	for (unsigned family = 0U; family < 8U; ++family)
	{
		affected_type prototype{};
		prototype.type = 100 + family;
		prototype.flags = AFFTYPE_SHORT;
		prototype.duration = 5;
		prototype.bitvector = first[family];
		prototype.bitvector2 = second[family];
		auto *effect = affect_to_char(&target, &prototype);
		assert(effect && target.affected == effect &&
		       telemetry_runtime_game_control_mask(&target) == (1U << family));
		expected.emplace_back(0U, 1U << family);
		auto *expiry = get_scheduled(&target, event_short_affect);
		assert(expiry &&
		       static_cast<event_short_affect_data *>(expiry->data)->af == effect);
		assert(affect_total(&target, FALSE) == FALSE);
		assert(telemetry_runtime_game_control_mask(&target) == (1U << family));
		expiry->func(&target, nullptr, nullptr, expiry->data);
		assert(!target.affected && telemetry_runtime_game_control_mask(&target) == 0U);
		expected.emplace_back(1U << family, 0U);
		// A retired timer payload cannot invent a second removal.
		event_short_affect(&target, nullptr, nullptr, expiry->data);
	}
	// Overlapping affects preserve the flag until the final native owner is removed.
	affected_type prototype{};
	prototype.type = SPELL_BLINDNESS;
	prototype.bitvector = AFF_BLIND;
	auto *first_blind = affect_to_char(&target, &prototype);
	prototype.type = SPELL_SLEEP;
	auto *second_blind = affect_to_char(&target, &prototype);
	expected.emplace_back(0U, 1U);
	affect_remove(&target, first_blind);
	assert(telemetry_runtime_game_control_mask(&target) == 1U);
	affected_type orphan{};
	affect_remove(&target, &orphan); // Refused unlink restores the same effective state.
	assert(telemetry_runtime_game_control_mask(&target) == 1U);
	affect_remove(&target, second_blind);
	expected.emplace_back(1U, 0U);
	// A native remove/reapply refresh and its canceled timer form one final state.
	prototype = {};
	prototype.type = SPELL_SLEEP;
	prototype.bitvector = AFF_SLEEP;
	prototype.flags = AFFTYPE_SHORT;
	prototype.duration = 5;
	(void)affect_to_char(&target, &prototype);
	expected.emplace_back(0U, 32U);
	affect_join(&target, &prototype, FALSE, FALSE);
	assert(target.affected && target.affected->duration == 10 &&
	       telemetry_runtime_game_control_mask(&target) == 32U);
	affect_from_char(&target, SPELL_SLEEP);
	expected.emplace_back(32U, 0U);
	prototype = {};
	prototype.type = SPELL_BLINDNESS;
	prototype.bitvector = AFF_BLIND;
	prototype.flags = AFFTYPE_NOAPPLY;
	(void)affect_to_char(&target, &prototype);
	assert(telemetry_runtime_game_control_mask(&target) == 0U);
	affect_from_char(&target, SPELL_BLINDNESS);
	// Maintained direct-removal callers also clear residual flags without an affect.
	target.specials.affected_by = AFF_BLIND;
	telemetry_runtime_game_control_changed(&target);
	expected.emplace_back(0U, 1U);
	spell_cure_blind(20, &player, nullptr, SPELL_TYPE_SPELL, &target, nullptr);
	assert(telemetry_runtime_game_control_mask(&target) == 0U);
	expected.emplace_back(1U, 0U);
	target.specials.affected_by = AFF_SLEEP;
	telemetry_runtime_game_control_changed(&target);
	expected.emplace_back(0U, 32U);
	affected_type song{};
	song.type = SONG_SLEEP;
	char_link_data link{};
	link.linking = &target;
	link.affect = &song;
	song_broken(&link);
	assert(telemetry_runtime_game_control_mask(&target) == 0U);
	expected.emplace_back(32U, 0U);
	// A rude awakening removes two native sleep sources as one operation.
	// A third source survives; its rebuild must not invent a stop/restart.
	for (bool retain_sleep : { false, true })
	{
		prototype = {};
		prototype.type = SPELL_SLEEP;
		prototype.bitvector = AFF_SLEEP;
		(void)affect_to_char(&target, &prototype);
		prototype.type = SONG_SLEEP;
		(void)affect_to_char(&target, &prototype);
		expected.emplace_back(0U, 32U);
		if (retain_sleep)
		{
			prototype.type = 100;
			(void)affect_to_char(&target, &prototype);
		}
		SET_POS(&target, POS_PRONE + STAT_SLEEPING);
		target.specials.fighting = &player;
		fixture_expected_wake_mask = retain_sleep ? 32U : 0U;
		update_pos(&target);
		assert(!affected_by_spell(&target, SPELL_SLEEP) &&
		       !affected_by_spell(&target, SONG_SLEEP));
		assert(telemetry_runtime_game_control_mask(&target) == fixture_expected_wake_mask);
		target.specials.fighting = nullptr;
		if (retain_sleep)
			affect_from_char(&target, 100);
		expected.emplace_back(32U, 0U);
	}
	// Falling applies native stun, then releases both magical sleep sources.
	// Stun remains a separate status; no transient sleep removal is observable.
	prototype = {};
	prototype.type = SPELL_SLEEP;
	prototype.bitvector = AFF_SLEEP;
	(void)affect_to_char(&target, &prototype);
	prototype.type = SONG_SLEEP;
	(void)affect_to_char(&target, &prototype);
	expected.emplace_back(0U, 32U);
	SET_POS(&target, POS_STANDING + STAT_SLEEPING);
	fixture_expected_wake_mask = 2U;
	update_pos(&target);
	expected.emplace_back(32U, 34U);
	expected.emplace_back(34U, 2U);
	assert(telemetry_runtime_game_control_mask(&target) == 2U);
	affect_from_char(&target, SPELL_PWORD_STUN);
	expected.emplace_back(2U, 0U);
	assert(fixture_affect_wakes == 4U);
	fixture_expected_wake_mask = 0U;
	// The maintained damage-release block completes all status removals before
	// observing the hit. Multiple minor-paralysis owners retain unrelated flags.
	prototype = {};
	prototype.type = SPELL_MINOR_PARALYSIS;
	prototype.bitvector = AFF_SLEEP | AFF_BOUND | AFF_BLIND;
	prototype.bitvector2 = AFF2_MINOR_PARALYSIS;
	(void)affect_to_char(&target, &prototype);
	expected.emplace_back(0U, 169U);
	prototype.type = SPELL_SLEEP;
	(void)affect_to_char(&target, &prototype);
	SET_POS(&target, POS_PRONE + STAT_SLEEPING);
	fixture_damage_control_release(&player, &target, 20, STAT_NORMAL);
	assert(!target.affected && telemetry_runtime_game_control_mask(&target) == 0U);
	expected.emplace_back(169U, 0U);
	prototype = {};
	prototype.type = SPELL_MINOR_PARALYSIS;
	prototype.bitvector2 = AFF2_MINOR_PARALYSIS;
	(void)affect_to_char(&target, &prototype);
	expected.emplace_back(0U, 8U);
	fixture_damage_control_release(&target, &target, 20, STAT_NORMAL);
	assert(target.affected && telemetry_runtime_game_control_mask(&target) == 8U);
	fixture_damage_control_release(&player, &target, 20, STAT_NORMAL);
	expected.emplace_back(8U, 0U);
	// Staff table/offset writes execute their maintained parsers/copy bodies.
	for (unsigned family = 0U; family < 8U; ++family)
	{
		const auto flag = first[family] ? first[family] : second[family];
		unsigned bit = 0U;
		while ((1UL << bit) != flag)
			++bit;
		fixture_staff_control_bit(&target, first[family] == 0U, bit, true, true);
		assert(telemetry_runtime_game_control_mask(&target) == (1U << family));
		expected.emplace_back(0U, 1U << family);
		fixture_staff_control_bit(&target, first[family] == 0U, bit, false, true);
		expected.emplace_back(1U << family, 0U);
	}
	fixture_staff_control_bank(&target, AFF_BLIND | AFF_SLEEP | AFF_BOUND);
	expected.emplace_back(0U, 161U);
	fixture_staff_control_bank(&target, 0);
	expected.emplace_back(161U, 0U);
	char_data outside{};
	fixture_staff_control_bit(&outside, false, 0U, true, true);
	fixture_staff_control_bank(&outside, 0);
	// Execute the actual equipment aggregation and save-style remove/reapply.
	obj_data gear{};
	gear.R_num = -1;
	gear.type = ITEM_ARMOR;
	gear.bitvector = AFF_BLIND | AFF_SLEEP | AFF_BOUND;
	gear.bitvector2 = AFF2_STUNNED | AFF2_MAJOR_PARALYSIS | AFF2_MINOR_PARALYSIS | AFF2_SLOW |
			  AFF2_SILENCED;
	target.equipment[0] = &gear;
	all_affects(&target, TRUE);
	assert(telemetry_runtime_game_control_mask(&target) == 255U);
	expected.emplace_back(0U, 255U);
	{
		telemetry_control_mutation_scope save(&target);
		all_affects(&target, FALSE);
		assert(telemetry_runtime_game_control_mask(&target) == 0U);
		telemetry_runtime_game_control_changed(&target);
		(void)telemetry_runtime_game_battle_context(&target);
		all_affects(&target, TRUE);
		assert(telemetry_runtime_game_control_mask(&target) == 255U);
	}
	{
		telemetry_control_mutation_scope removal(&target);
		all_affects(&target, FALSE);
		target.equipment[0] = nullptr;
		all_affects(&target, TRUE);
	}
	assert(telemetry_runtime_game_control_mask(&target) == 0U);
	expected.emplace_back(255U, 0U);
	// Restored ward banks may contain selected bits beyond the usual spell catalog.
	// The maintained setter preserves an active overlapping cast/equipment source.
	affected_type cast_ward{};
	cast_ward.type = SPELL_GLOBE;
	cast_ward.flags = AFFTYPE_SPELL_WARD;
	cast_ward.ward_active = 1;
	cast_ward.ward_capacity = 1;
	cast_ward.ward_source_type = SPELL_WARD_SOURCE_CAST;
	cast_ward.bitvector = gear.bitvector;
	cast_ward.bitvector2 = gear.bitvector2;
	affected_type equipment_ward = cast_ward;
	equipment_ward.type = SPELL_MINOR_GLOBE;
	equipment_ward.ward_source_type = SPELL_WARD_SOURCE_EQUIPMENT;
	equipment_ward.ward_source_worn = 1;
	cast_ward.next = &equipment_ward;
	target.affected = &cast_ward;
	set_ward_bits(&target, &cast_ward, true);
	expected.emplace_back(0U, 255U);
	set_ward_bits(&target, &equipment_ward, true);
	set_ward_bits(&target, &cast_ward, false);
	assert(telemetry_runtime_game_control_mask(&target) == 255U);
	cast_ward.ward_active = 0;
	set_ward_bits(&target, &equipment_ward, false);
	expected.emplace_back(255U, 0U);
	assert(telemetry_runtime_game_control_mask(&target) == 0U);
	// An inactive partner cannot retain bits or expose a nested temporary rebuild.
	equipment_ward.flags |= AFFTYPE_NOAPPLY;
	{
		telemetry_control_mutation_scope rebuild(&target);
		set_ward_bits(&target, &cast_ward, true);
		set_ward_bits(&target, &cast_ward, false);
	}
	assert(telemetry_runtime_game_control_mask(&target) == 0U);
	set_ward_bits(nullptr, &cast_ward, true);
	set_ward_bits(&target, nullptr, true);
	target.affected = nullptr;
	// This maintained rebuild can destroy its character; the scope finishes first.
	auto *dead = new char_data{};
	npc_only_data npc{};
	dead->only.npc = &npc;
	SET_BIT(dead->specials.act, ACT_ISNPC);
	dead->in_room = 0;
	dead->points.base_hit = dead->points.max_hit = 300;
	dead->points.hit = -11;
	assert(affect_total(dead, TRUE) == TRUE && fixture_affect_deaths == 1U);
	assert(std::none_of(fixture_affect_used.begin(), fixture_affect_used.end(),
			    [](bool used) { return used; }));
	assert(fixture_affect_scheduled == 11U && fixture_affect_canceled == 11U);
	assert(target.telemetry_control_rebuild_depth == 0U);
	assert(telemetry_runtime_game_battle_leave(&target).outcome ==
	       telemetry_runtime_outcome::accepted);
	telemetry_monotonic_usec now = 0U;
	telemetry_utc_usec utc = TELEMETRY_UTC_UNKNOWN;
	assert(telemetry_runtime_now(&now, &utc));
	assert(telemetry_runtime_shutdown({ now + 5'000'000U, 1U, {} }) ==
	       telemetry_runtime_outcome::accepted);
	assert(telemetry_runtime_final_reap() == telemetry_runtime_outcome::accepted);
	std::size_t transitions = 0U;
	for (const auto &record : fake.controls)
	{
		const auto &value = record.payload.control;
		assert(value.duration_coverage ==
		       (value.kind == telemetry_control_kind::state_entry ||
					value.kind == telemetry_control_kind::state_interval ?
				255U :
				0U));
		if (value.target.actor.actor_id == 8989U &&
		    value.boundary == telemetry_control_boundary::state_changed)
		{
			assert(transitions < expected.size());
			if (value.before_mask != expected[transitions].first ||
			    value.after_mask != expected[transitions].second)
				std::fprintf(
					stderr,
					"Native transition %zu: observed %u -> %u, expected %u -> %u\n",
					transitions, value.before_mask, value.after_mask,
					expected[transitions].first, expected[transitions].second);
			assert(value.before_mask == expected[transitions].first &&
			       value.after_mask == expected[transitions].second);
			++transitions;
		}
	}
	assert(transitions == expected.size() && transitions == 58U);
	character_list = nullptr;
	world = nullptr;
	zone_table = nullptr;
	top_of_world = top_of_zone_table = -1;
	std::puts(
		"PASS: maintained affect/ward/wake/damage/staff functions conserve 58 final control transitions across all eight statuses, expiry, overlap, refresh, refused removal, NOAPPLY, equipment/save rebuilding and compound release; 11 timers canceled and teardown is ASan-safe; selected flag coverage is 255 with independent context quality");
}
#endif

void check_native_control_state_changes()
{
	fake_repository fake{};
	const telemetry_transport_repository_binding repository = { fake_init, fake_apply,
								    fake_request_stop,
								    fake_shutdown, &fake };
	const telemetry_transport_clock_binding clock = { fake_clock, nullptr };
	assert(telemetry_transport_bind_for_tests(&repository, &clock) ==
	       telemetry_transport_outcome::started);
	telemetry_test_start_runtime(enabled_options());
	room_data room{};
	zone_data zone{};
	zone.number = 1979;
	world = &room;
	zone_table = &zone;
	top_of_world = top_of_zone_table = 0;
	char_data player{}, target{}, outside{};
	pc_only_data pc{}, target_pc{}, outside_pc{};
	pc.pid = 8985;
	target_pc.pid = 8986;
	outside_pc.pid = 8987;
	fixture_native_lifetimes lifetimes{ &player, &target, &outside };
	player.only.pc = &pc;
	target.only.pc = &target_pc;
	outside.only.pc = &outside_pc;
	player.in_room = target.in_room = outside.in_room = 0;
	assert(telemetry_runtime_game_combat_engage(&player, &target).outcome ==
	       telemetry_runtime_outcome::accepted);
	// A temporary/account-screen load can reuse a PID and default room zero.
	// Neither an unpublished identity nor a borrowed live identity may mutate it.
	char_data temporary{};
	pc_only_data temporary_pc{};
	temporary_pc.pid = target_pc.pid;
	temporary.only.pc = &temporary_pc;
	temporary.in_room = 0;
	temporary.specials.affected_by = AFF_BLIND;
	telemetry_runtime_game_control_changed(&temporary);
	temporary.runtime_id = target.runtime_id;
	telemetry_runtime_game_control_changed(&temporary);
	temporary.runtime_id = allocate_character_runtime_id();
	telemetry_runtime_game_control_changed(&temporary);
	const unsigned long first[] = { AFF_BLIND, 0U, 0U, 0U, 0U, AFF_SLEEP, 0U, AFF_BOUND };
	const unsigned long second[] = {
		0U,	   AFF2_STUNNED, AFF2_MAJOR_PARALYSIS, AFF2_MINOR_PARALYSIS,
		AFF2_SLOW, 0U,		 AFF2_SILENCED,	       0U
	};
	std::vector<std::pair<std::uint16_t, std::uint16_t>> expected;
	fixture_crypto_heap_calls = 0U;
	fixture_track_crypto = true;
	for (unsigned family = 0U; family < 8U; ++family)
	{
		target.specials.affected_by = first[family];
		target.specials.affected_by2 = second[family];
		telemetry_runtime_game_control_changed(&target);
		expected.emplace_back(0U, 1U << family);
		target.specials.affected_by = target.specials.affected_by2 = 0U;
		telemetry_runtime_game_control_changed(&target);
		expected.emplace_back(1U << family, 0U);
	}
	// A save/rebuild with the same final mask cannot create intermediate rows.
	{
		telemetry_control_mutation_scope rebuild(&target);
		target.specials.affected_by = AFF_BLIND;
		telemetry_runtime_game_control_changed(&target);
		{
			telemetry_control_mutation_scope nested(&target);
			target.specials.affected_by2 = AFF2_STUNNED;
			telemetry_runtime_game_battle_build_changed(&target);
			(void)telemetry_runtime_game_battle_context(&target);
		}
		target.specials.affected_by = target.specials.affected_by2 = 0U;
		rebuild.finish();
		rebuild.finish();
	}
	assert(target.telemetry_control_rebuild_depth == 0U);
	// A compound mutation with a different final mask produces exactly one cut.
	{
		telemetry_control_mutation_scope rebuild(&target);
		target.specials.affected_by = AFF_BLIND;
		telemetry_runtime_game_control_changed(&target);
		{
			telemetry_control_mutation_scope nested(&target);
			target.specials.affected_by2 = AFF2_STUNNED;
		}
	}
	expected.emplace_back(0U, 3U);
	{
		telemetry_control_mutation_scope rebuild(&target);
		target.specials.affected_by = 0U;
		telemetry_runtime_game_control_changed(&target);
		target.specials.affected_by2 = 0U;
	}
	expected.emplace_back(3U, 0U);
	outside.specials.affected_by = AFF_SLEEP;
	telemetry_runtime_game_control_changed(&outside);
	telemetry_runtime_game_control_changed(nullptr);
	fixture_track_crypto = false;
	assert(fixture_crypto_heap_calls == 0U);
	assert(telemetry_runtime_game_battle_leave(&target).outcome ==
	       telemetry_runtime_outcome::accepted);
	target.specials.affected_by = AFF_BOUND;
	telemetry_runtime_game_control_changed(&target); // Inactive grace is not re-entry.
	{
		telemetry_control_mutation_scope null_scope(nullptr);
		outside.telemetry_control_rebuild_depth = std::numeric_limits<std::uint32_t>::max();
		telemetry_control_mutation_scope saturated(&outside);
	}
	assert(outside.telemetry_control_rebuild_depth ==
	       std::numeric_limits<std::uint32_t>::max());
	telemetry_monotonic_usec now = 0U;
	telemetry_utc_usec utc = TELEMETRY_UTC_UNKNOWN;
	assert(telemetry_runtime_now(&now, &utc));
	assert(telemetry_runtime_shutdown({ now + 5'000'000U, 1U, {} }) ==
	       telemetry_runtime_outcome::accepted);
	assert(telemetry_runtime_final_reap() == telemetry_runtime_outcome::accepted);
	std::size_t transitions = 0U;
	unsigned target_entries = 0U, target_departures = 0U;
	for (const auto &record : fake.controls)
	{
		const auto &value = record.payload.control;
		assert(value.target.actor.actor_id != 8987U);
		assert(value.duration_coverage ==
			       (value.kind == telemetry_control_kind::state_entry ||
						value.kind ==
							telemetry_control_kind::state_interval ?
					255U :
					0U) &&
		       (value.quality_flags & TELEMETRY_QUALITY_CONTEXT_UNKNOWN));
		if (value.target.actor.actor_id != 8986U)
			continue;
		target_entries += value.kind == telemetry_control_kind::state_entry;
		target_departures += value.boundary == telemetry_control_boundary::actor_left;
		if (value.boundary == telemetry_control_boundary::state_changed)
		{
			assert(transitions < expected.size());
			assert(value.before_mask == expected[transitions].first &&
			       value.after_mask == expected[transitions].second);
			++transitions;
		}
	}
	assert(transitions == expected.size() && target_entries == 1U && target_departures == 1U);
	assert(fake.builds.size() == 2U); // Mutation observation does not read/hash a build.
	character_list = nullptr;
	world = nullptr;
	zone_table = nullptr;
	top_of_world = top_of_zone_table = -1;
	std::puts(
		"PASS: native control-state callbacks conserve 18 final transitions, suppress nested rebuilds, preserve independent context quality and exclude outside/inactive actors without build hashing");
}

void check_native_build_limits()
{
	fake_repository fake{};
	const telemetry_transport_repository_binding repository = { fake_init, fake_apply,
								    fake_request_stop,
								    fake_shutdown, &fake };
	const telemetry_transport_clock_binding clock = { fake_clock, nullptr };
	assert(telemetry_transport_bind_for_tests(&repository, &clock) ==
	       telemetry_transport_outcome::started);
	auto options = enabled_options();
	options.config.context_segments_per_minute = 2U;
	assert(telemetry_config_compute_fingerprint(options.config, options.config.fingerprint,
						    sizeof(options.config.fingerprint)));
	options.config.config_id = telemetry_config_id_from_fingerprint(
		options.config.fingerprint, sizeof(options.config.fingerprint));
	telemetry_test_start_runtime(options);
	room_data room{};
	zone_data zone{};
	zone.number = 1978;
	world = &room;
	zone_table = &zone;
	top_of_world = top_of_zone_table = 0;
	char_data player{}, target{};
	pc_only_data pc{}, target_pc{};
	pc.pid = 8991;
	target_pc.pid = 8992;
	player.only.pc = &pc;
	target.only.pc = &target_pc;
	player.in_room = target.in_room = 0;
	assert(telemetry_runtime_game_combat_engage(&player, &target).outcome ==
	       telemetry_runtime_outcome::accepted);
	++player.curr_stats.Str;
	telemetry_runtime_game_battle_build_changed(&player);
	assert(telemetry_runtime_game_battle_context(&player).records_emitted == 1U);
	++player.curr_stats.Str;
	telemetry_runtime_game_battle_build_changed(&player);
	assert(telemetry_runtime_game_battle_context(&player).records_emitted == 1U);
	for (unsigned attempt = 0U; attempt < 100U; ++attempt)
	{
		++player.curr_stats.Str;
		telemetry_runtime_game_battle_build_changed(&player);
		assert(telemetry_runtime_game_battle_context(&player).records_emitted == 0U);
	}
	telemetry_monotonic_usec now = 0U;
	telemetry_utc_usec utc = TELEMETRY_UTC_UNKNOWN;
	assert(telemetry_runtime_now(&now, &utc));
	assert(telemetry_runtime_shutdown({ now + 5'000'000U, 1U, {} }) ==
	       telemetry_runtime_outcome::accepted);
	assert(telemetry_runtime_final_reap() == telemetry_runtime_outcome::accepted);
	assert(fake.builds.size() == 4U);
	const auto &gap = fake.builds.back().payload.battle_build;
	assert(gap.boundary == telemetry_battle_build_boundary::rate_limit &&
	       telemetry_battle_build_detail::empty_profile(gap));
	world = nullptr;
	zone_table = nullptr;
	top_of_world = top_of_zone_table = -1;
	std::puts(
		"PASS: bounded native actor sampling emits one empty rate-limit gap during mutation churn");
}

void check_native_build_global_limit()
{
	fake_repository fake{};
	const telemetry_transport_repository_binding repository = { fake_init, fake_apply,
								    fake_request_stop,
								    fake_shutdown, &fake };
	const telemetry_transport_clock_binding clock = { fake_clock, nullptr };
	assert(telemetry_transport_bind_for_tests(&repository, &clock) ==
	       telemetry_transport_outcome::started);
	telemetry_test_start_runtime(enabled_options());
	room_data room{};
	zone_data zone{};
	zone.number = 1979;
	world = &room;
	zone_table = &zone;
	top_of_world = top_of_zone_table = 0;
	char_data player{}, targets[20]{};
	pc_only_data pc{};
	npc_only_data npc_data[20]{};
	pc.pid = 8993;
	player.only.pc = &pc;
	player.in_room = 0;
	for (std::size_t index = 0U; index < 20U; ++index)
	{
		auto &target = targets[index];
		target.specials.act = ACT_ISNPC;
		target.only.npc = &npc_data[index];
		target.runtime_id = allocate_character_runtime_id();
		target.in_room = 0;
		// Separate live components avoid the shared collector's 16-actor cap.
		assert(telemetry_runtime_game_battle_leave(&player).outcome ==
		       telemetry_runtime_outcome::accepted);
		assert(telemetry_runtime_game_combat_engage(&player, &target).outcome ==
		       telemetry_runtime_outcome::accepted);
	}
	// Full scans are bounded before entry capture. A later current observation
	// recovers unknown points using newly allocated logical keys.
	std::this_thread::sleep_for(std::chrono::milliseconds(1100));
	for (auto &target : targets)
		assert(telemetry_runtime_game_battle_context(&target).outcome ==
		       telemetry_runtime_outcome::accepted);
	telemetry_monotonic_usec now = 0U;
	telemetry_utc_usec utc = TELEMETRY_UTC_UNKNOWN;
	assert(telemetry_runtime_now(&now, &utc));
	assert(telemetry_runtime_shutdown({ now + 5'000'000U, 1U, {} }) ==
	       telemetry_runtime_outcome::accepted);
	assert(telemetry_runtime_final_reap() == telemetry_runtime_outcome::accepted);
	unsigned initial = 0U, gaps = 0U, resumed = 0U;
	std::set<telemetry_sequence> sequences;
	for (const auto &record : fake.builds)
	{
		const auto &value = record.payload.battle_build;
		assert(sequences.insert(value.sequence).second);
		initial += value.boundary == telemetry_battle_build_boundary::actor_entry;
		gaps += value.boundary == telemetry_battle_build_boundary::rate_limit;
		resumed += value.boundary == telemetry_battle_build_boundary::source_resumed;
	}
	assert(initial == 20U && gaps == 16U && resumed == 8U);
	world = nullptr;
	zone_table = nullptr;
	top_of_world = top_of_zone_table = -1;
	std::puts(
		"PASS: native global read and marker budgets bound burst entries and recover fresh contexts");
}

void check_native_build_queue_loss()
{
	fake_repository fake{};
	worker_entered.store(false);
	release_worker.store(false);
	const telemetry_transport_repository_binding repository = { fake_init, blocking_apply,
								    fake_request_stop,
								    fake_shutdown, &fake };
	const telemetry_transport_clock_binding clock = { fake_clock, nullptr };
	assert(telemetry_transport_bind_for_tests(&repository, &clock) ==
	       telemetry_transport_outcome::started);
	telemetry_test_start_runtime(enabled_options());
	const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(5);
	while (!worker_entered.load() && std::chrono::steady_clock::now() < deadline)
		std::this_thread::sleep_for(std::chrono::milliseconds(1));
	assert(worker_entered.load());
	room_data room{};
	zone_data zone{};
	zone.number = 1980;
	world = &room;
	zone_table = &zone;
	top_of_world = top_of_zone_table = 0;
	char_data player{}, target{}, churn{};
	pc_only_data pc{}, target_pc{}, churn_pc{};
	pc.pid = 8994;
	target_pc.pid = 8995;
	churn_pc.pid = 8996;
	player.only.pc = &pc;
	target.only.pc = &target_pc;
	churn.only.pc = &churn_pc;
	player.in_room = target.in_room = 0;
	churn.in_room = -1;
	telemetry_runtime_game_combat_damage(&player, &target, 5U, 0U);
	descriptor_data descriptor{};
	descriptor.connected = CON_PLAYING;
	bool full = false;
	for (unsigned attempt = 0U; attempt < 8192U; ++attempt)
	{
		const auto entered = telemetry_runtime_game_enter(&churn, &descriptor);
		(void)telemetry_runtime_game_session_exit(&churn, &descriptor,
							  telemetry_session_end_reason::logout);
		if (entered.records_dropped)
		{
			full = true;
			break;
		}
	}
	assert(full);
	++player.curr_stats.Str;
	telemetry_runtime_game_battle_build_changed(&player);
	const auto refused = telemetry_runtime_game_battle_context(&player);
	assert(refused.outcome == telemetry_runtime_outcome::queue_full &&
	       refused.records_dropped == 1U);
	release_worker.store(true);
	const auto drain = std::chrono::steady_clock::now() + std::chrono::seconds(5);
	while (telemetry_transport_health_copy().queue_depth != 0U &&
	       std::chrono::steady_clock::now() < drain)
		std::this_thread::sleep_for(std::chrono::milliseconds(1));
	assert(telemetry_transport_health_copy().queue_depth == 0U);
	assert(telemetry_runtime_game_battle_context(&player).records_emitted == 1U);
	telemetry_monotonic_usec now = 0U;
	telemetry_utc_usec utc = TELEMETRY_UTC_UNKNOWN;
	assert(telemetry_runtime_now(&now, &utc));
	assert(telemetry_runtime_shutdown({ now + 5'000'000U, 1U, {} }) ==
	       telemetry_runtime_outcome::accepted);
	assert(telemetry_runtime_final_reap() == telemetry_runtime_outcome::accepted);
	assert(fake.builds.size() == 3U);
	const auto &recovered = fake.builds.back().payload.battle_build;
	assert(recovered.actor_id == 8994U && recovered.sequence == 4U &&
	       recovered.boundary == telemetry_battle_build_boundary::source_resumed &&
	       (recovered.quality_flags & TELEMETRY_QUALITY_QUEUE_DROP));
	assert(fake.contributions.size() == 2U);
	for (const auto &record : fake.contributions)
		assert(!(record.payload.battle_contribution.quality_flags &
			 TELEMETRY_QUALITY_QUEUE_DROP));
	world = nullptr;
	zone_table = nullptr;
	top_of_world = top_of_zone_table = -1;
	std::puts(
		"PASS: refused build keys are never reused and build loss preserves measured contribution coverage");
}

} // namespace

int main(int argc, char **argv)
{
#ifdef TELEMETRY_TEST_NATIVE_AFFECTS
	if (argc == 2 && std::strcmp(argv[1], "--native-affects") == 0)
	{
		check_native_control_state_changes();
		check_native_affect_mutations();
		return 0;
	}
#endif
	if (argc == 2 && std::strcmp(argv[1], "--native-build-context") == 0)
	{
		assert(CRYPTO_set_mem_functions(fixture_crypto_malloc, fixture_crypto_realloc,
						fixture_crypto_free) == 1);
		check_native_build_context(true);
		return 0;
	}
	if (argc == 2 && std::strcmp(argv[1], "--native-control-capture") == 0)
	{
		check_native_control_capture(false, true);
		return 0;
	}
	if (argc == 2 && std::strcmp(argv[1], "--native-expanded-control-capture") == 0)
	{
		check_native_expanded_control_capture(false, true);
		return 0;
	}
	if (argc == 2 && std::strcmp(argv[1], "--native-build-capture") == 0)
	{
		check_native_build_capture(false, true);
		return 0;
	}
#ifdef TELEMETRY_TEST_NATIVE_BATTLE_SQL
	if (argc == 2 && std::strcmp(argv[1], "--native-expanded-control-sql") == 0)
	{
		check_native_expanded_control_capture(true, true);
		std::puts("expanded accepted control spells through SQL writer passed");
		return 0;
	}
	if (argc == 2 && std::strcmp(argv[1], "--native-build-sql") == 0)
	{
		check_native_build_capture(true, true);
		return 0;
	}
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
	assert(CRYPTO_set_mem_functions(fixture_crypto_malloc, fixture_crypto_realloc,
					fixture_crypto_free) == 1);
	check_native_build_context();
	check_native_build_capture();
	check_native_control_state_changes();
	check_native_build_limits();
	check_native_build_global_limit();
	check_native_battle_context();
	check_native_shared_battle_capture();
	check_native_control_capture();
	check_native_expanded_control_capture();
	check_group_generation_and_combat_entry();
	check_authenticated_ownership_path();
	check_deferred_startup_presence();
	check_resume_capacity_rollback();
	check_resume_queue_pressure();
	check_native_build_queue_loss();
	check_environment_options();
	check_disabled_game_path();
	check_enabled_game_path();
	check_staggered_checkpoint_cut();
	check_combat_and_afk_context();
	std::puts("telemetry gameplay adapter paths passed");
	return 0;
}
