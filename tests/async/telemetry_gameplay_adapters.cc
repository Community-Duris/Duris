#include "telemetry/telemetry_config_private.h"
#include "telemetry/telemetry_runtime.h"
#include "telemetry/telemetry_transport_private.h"
#include "core/structs.h"

#include "telemetry/telemetry_session.h"
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
P_char get_linked_char(P_char, ush_int)
{
	return nullptr;
}

P_char get_linked_char(P_char, ush_int)
{
	return nullptr;
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
	assert(telemetry_runtime_init(options) == telemetry_runtime_outcome::accepted);

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
	const telemetry_capture_result entered = telemetry_runtime_game_enter(&player, &descriptor);
	assert(entered.outcome == telemetry_runtime_outcome::accepted);
	assert(player.telemetry_session_sequence != 0U);
	assert(descriptor.telemetry_connection_sequence != 0U);
	const std::uint64_t first_connection = descriptor.telemetry_connection_sequence;

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
		telemetry_runtime_game_enter(&player, &descriptor);
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
	assert(telemetry_runtime_init(options) == telemetry_runtime_outcome::accepted);
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
	assert(telemetry_runtime_init(options) == telemetry_runtime_outcome::accepted);
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
	assert(telemetry_runtime_init(options) == telemetry_runtime_outcome::accepted);
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
	telemetry_monotonic_usec now = 0;
	telemetry_utc_usec utc = 0;
	assert(telemetry_runtime_now(&now, &utc));
	assert(telemetry_runtime_shutdown({ now + 5'000'000U, 1U, {} }) ==
	       telemetry_runtime_outcome::accepted);
	assert(telemetry_runtime_final_reap() == telemetry_runtime_outcome::accepted);
	telemetry_transport_unbind_for_tests();
	assert(resumed.outcome == telemetry_runtime_outcome::queue_full);
	assert(!phantom_ids);
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
	assert(telemetry_runtime_init(options) == telemetry_runtime_outcome::accepted);
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
	descriptor_data descriptor{};
	descriptor.connected = CON_PLAYING;
	const auto resumed = telemetry_runtime_game_session_resume(&player, &descriptor, nullptr);
	const bool retained_ids = player.telemetry_session_sequence != 0 &&
				  descriptor.telemetry_connection_sequence != 0;
	release_worker.store(true);
	const auto drain_deadline = std::chrono::steady_clock::now() + std::chrono::seconds(5);
	while (telemetry_transport_health_copy().queue_depth != 0 &&
	       std::chrono::steady_clock::now() < drain_deadline)
		std::this_thread::sleep_for(std::chrono::milliseconds(1));
	// Queue loss must not strand an admitted classifier/session pair.
	const auto evidence = telemetry_runtime_game_evidence(
		&player, &descriptor, telemetry_runtime_evidence_kind::player_action);
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
}

} // namespace

int main()
{
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
