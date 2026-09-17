#include "telemetry/telemetry_config_private.h"
#include "telemetry/telemetry_config_reload.h"
#include "telemetry/telemetry_runtime.h"
#include "telemetry/telemetry_transport_private.h"
#include "core/structs.h"

#include <cassert>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <condition_variable>
#include <mutex>
#include <thread>
#include <unistd.h>

P_room world = nullptr;
struct zone_data *zone_table = nullptr;
int top_of_zone_table = -1;
int top_of_world = -1;

P_char get_linked_char(P_char, ush_int)
{
	return nullptr;
}

float environment_observe = 0.0F;
float environment_max_payout_factor = 10.0F;
float environment_payout_factor = 1.0F;
float environment_alignment_mod = 0.2F;
float environment_minimum_alignment = 0.15F;

float get_property(const char *key, double default_value)
{
	if (key == nullptr)
		return static_cast<float>(default_value);
	if (std::strcmp(key, "exp.zoneTrophy.observe") == 0)
		return environment_observe;
	if (std::strcmp(key, "epic.touch.maxPayoutFactor") == 0)
		return environment_max_payout_factor;
	if (std::strcmp(key, "epic.touch.PayoutFactor") == 0)
		return environment_payout_factor;
	if (std::strcmp(key, "epic.zone.alignmentMod") == 0)
		return environment_alignment_mod;
	if (std::strcmp(key, "epic.alignment.minPercentage") == 0)
		return environment_minimum_alignment;
	return static_cast<float>(default_value);
}

#ifdef TELEMETRY_TEST_STUB_REPOSITORY
telemetry_repository_outcome telemetry_repository_init(telemetry_repository_config)
{
	return telemetry_repository_outcome::unavailable;
}

telemetry_apply_batch_result telemetry_repository_apply(const telemetry_record *, std::size_t)
{
	return {};
}

telemetry_health_snapshot telemetry_repository_health_copy()
{
	return {};
}

telemetry_repository_outcome telemetry_repository_request_stop()
{
	return telemetry_repository_outcome::stopping;
}

void telemetry_repository_shutdown() {}
#endif

namespace
{
struct blocked_callback
{
	std::mutex mutex;
	std::condition_variable condition;
	bool entered = false;
	bool release = false;

	bool wait_until_entered(std::chrono::milliseconds timeout)
	{
		std::unique_lock<std::mutex> lock(mutex);
		return condition.wait_for(lock, timeout, [this] { return entered; });
	}

	void call()
	{
		std::unique_lock<std::mutex> lock(mutex);
		entered = true;
		condition.notify_all();
		condition.wait(lock, [this] { return release; });
	}

	void unblock()
	{
		{
			std::lock_guard<std::mutex> lock(mutex);
			release = true;
		}
		condition.notify_all();
	}
};

struct finite_callback_releaser
{
	blocked_callback &callback;
	std::thread thread;

	finite_callback_releaser(blocked_callback &value, std::chrono::milliseconds delay)
		: callback(value)
		, thread(
			  [this, delay]
			  {
				  std::this_thread::sleep_for(delay);
				  callback.unblock();
			  })
	{
	}

	void release_and_join()
	{
		callback.unblock();
		if (thread.joinable())
			thread.join();
	}

	~finite_callback_releaser() { release_and_join(); }
};

struct fake_repository
{
	enum class mode : std::uint8_t
	{
		ready,
		unavailable,
	};

	mode init_mode = mode::ready;
	std::uint32_t init_calls = 0U;
	std::uint32_t apply_calls = 0U;
	std::uint32_t applied_records = 0U;
	std::uint32_t request_stop_calls = 0U;
	std::uint32_t shutdown_calls = 0U;
	bool saw_dimensions = false;
	bool saw_unclosed_tail = false;
	bool block_apply = false;
	telemetry_producer_id required_fresh_producer{};
	blocked_callback apply_callback;
	std::atomic<telemetry_monotonic_usec> transport_now{ 0U };
	telemetry_dimensions dimensions{};
};

struct reload_property_values
{
	float observe;
	float max_payout_factor;
	float payout_factor;
	float alignment_mod;
	float minimum_alignment;
};

struct reload_property_catalog
{
	struct entry
	{
		std::uint8_t digest[TELEMETRY_CONFIG_PROPERTY_DIGEST_BYTES];
		std::uint32_t property_version;
		std::uint32_t stable_namespace;
		std::uint32_t stable_version;
	};
	entry entries[8];
	std::size_t count;
};

bool reload_property_read(void *context, const char *key, float *value) noexcept
{
	if (context == nullptr || key == nullptr || value == nullptr)
		return false;
	auto *properties = static_cast<reload_property_values *>(context);
	if (std::strcmp(key, "exp.zoneTrophy.observe") == 0)
		*value = properties->observe;
	else if (std::strcmp(key, "epic.touch.maxPayoutFactor") == 0)
		*value = properties->max_payout_factor;
	else if (std::strcmp(key, "epic.touch.PayoutFactor") == 0)
		*value = properties->payout_factor;
	else if (std::strcmp(key, "epic.zone.alignmentMod") == 0)
		*value = properties->alignment_mod;
	else if (std::strcmp(key, "epic.alignment.minPercentage") == 0)
		*value = properties->minimum_alignment;
	else
		return false;
	return true;
}

void reload_catalog_add(reload_property_catalog &catalog,
			const telemetry_config_property_snapshot &snapshot,
			std::uint32_t property_version)
{
	assert(catalog.count < sizeof(catalog.entries) / sizeof(catalog.entries[0]));
	auto &entry = catalog.entries[catalog.count++];
	std::memcpy(entry.digest, snapshot.effective_digest, sizeof(entry.digest));
	entry.property_version = property_version;
	entry.stable_namespace = 7U;
	entry.stable_version = 3U;
}

bool reload_catalog_resolve(void *context, const telemetry_config_property_snapshot *snapshot,
			    const std::uint8_t *digest,
			    telemetry_config_property_catalog_resolution *resolution) noexcept
{
	if (context == nullptr || snapshot == nullptr || digest == nullptr || resolution == nullptr)
		return false;
	auto *catalog = static_cast<reload_property_catalog *>(context);
	for (std::size_t index = 0U; index < catalog->count; ++index)
	{
		const auto &entry = catalog->entries[index];
		if (std::memcmp(entry.digest, digest, sizeof(entry.digest)) != 0)
			continue;
		*resolution = {};
		resolution->property_version = entry.property_version;
		resolution->stable_namespace = entry.stable_namespace;
		resolution->stable_version = entry.stable_version;
		std::memcpy(resolution->digest, entry.digest, sizeof(resolution->digest));
		return true;
	}
	return false;
}

telemetry_repository_outcome fake_init(void *context, telemetry_repository_config config) noexcept
{
	auto *fake = static_cast<fake_repository *>(context);
	++fake->init_calls;
	fake->required_fresh_producer = config.fresh_producer;
	if (fake->init_mode == fake_repository::mode::unavailable)
		return telemetry_repository_outcome::unavailable;
	return telemetry_repository_outcome::ready;
}

telemetry_apply_batch_result fake_apply(void *context, const telemetry_record *records,
					std::size_t count) noexcept
{
	auto *fake = static_cast<fake_repository *>(context);
	++fake->apply_calls;
	if (fake->block_apply)
		fake->apply_callback.call();
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
		if (records[index].header.kind == telemetry_record_kind::session_lifecycle)
		{
			fake->dimensions = records[index].payload.lifecycle.dimensions;
			fake->saw_dimensions = true;
			fake->saw_unclosed_tail = fake->saw_unclosed_tail ||
						  (records[index].payload.lifecycle.quality_flags &
						   TELEMETRY_QUALITY_UNCLOSED_TAIL) != 0U;
		}
	}
	return result;
}

telemetry_repository_outcome fake_request_stop(void *context) noexcept
{
	auto *fake = static_cast<fake_repository *>(context);
	++fake->request_stop_calls;
	return telemetry_repository_outcome::stopping;
}

void fake_shutdown(void *context) noexcept
{
	auto *fake = static_cast<fake_repository *>(context);
	++fake->shutdown_calls;
}

bool fake_clock(void *context, telemetry_monotonic_usec *monotonic_usec) noexcept
{
	if (monotonic_usec == nullptr)
		return false;
	if (context != nullptr)
	{
		auto *fake = static_cast<fake_repository *>(context);
		*monotonic_usec = fake->transport_now.load(std::memory_order_acquire);
		return true;
	}
	*monotonic_usec = static_cast<telemetry_monotonic_usec>(
		std::chrono::duration_cast<std::chrono::microseconds>(
			std::chrono::steady_clock::now().time_since_epoch())
			.count());
	return true;
}

telemetry_session_enter make_enter(const telemetry_producer_id producer,
				   const telemetry_config_snapshot &config)
{
	telemetry_session_enter enter{};
	enter.session = { { producer, 1U }, 42U, 42, 1U, 1U };
	enter.connection = { producer, 1U };
	enter.at_monotonic_usec = 1'000U;
	enter.at_utc_usec = 2'000U;
	enter.dimensions = { 1U, 1U, 1U, 1U, 100, 1U };
	enter.config_id = config.config_id;
	enter.classifier_version = config.classifier_version;
	enter.policy_version = config.policy_version;
	return enter;
}

telemetry_runtime_options make_enabled_options()
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

telemetry_runtime_options make_property_reload_options(reload_property_values &values,
						       reload_property_catalog &catalog)
{
	telemetry_runtime_options options = make_enabled_options();
	telemetry_config_property_capture probe_capture{};
	probe_capture.reader = { reload_property_read, &values };
	probe_capture.mode = telemetry_config_property_capture_mode::require_reader;
	telemetry_config_property_snapshot probe{};
	assert(telemetry_config_property_snapshot_capture(&probe_capture, &probe) ==
	       telemetry_config_build_outcome::built);
	reload_catalog_add(catalog, probe, 1001U);
	options.property_capture.reader = { reload_property_read, &values };
	options.property_capture.mode = telemetry_config_property_capture_mode::require_reader;
	options.property_capture.catalog = { reload_catalog_resolve, &catalog };
	options.property_capture_enabled = 1U;
	return options;
}

void write_reviewed_catalog(const char *path, const telemetry_config_property_snapshot &snapshot,
			    std::uint32_t property_version)
{
	FILE *file = std::fopen(path, "w");
	assert(file != nullptr);
	for (std::size_t index = 0U; index < TELEMETRY_CONFIG_PROPERTY_DIGEST_BYTES; ++index)
		std::fprintf(file, "%02x", snapshot.effective_digest[index]);
	std::fprintf(file, " %u 7 3\n", property_version);
	assert(std::fclose(file) == 0);
}

void clear_telemetry_environment()
{
	for (const char *name :
	     { "TELEMETRY_ENABLED", "TELEMETRY_BACKEND", "TELEMETRY_CONFIG_REVISION",
	       "TELEMETRY_BUILD_VERSION", "TELEMETRY_CONTENT_VERSION", "TELEMETRY_PROPERTY_VERSION",
	       "TELEMETRY_CLASSIFIER_VERSION", "TELEMETRY_POLICY_VERSION", "TELEMETRY_SEASON_ID",
	       "TELEMETRY_ENVIRONMENT_ID", "TELEMETRY_INTERVAL_USEC",
	       "TELEMETRY_CHECKPOINT_INTERVAL_USEC", "TELEMETRY_ACTIVE_WINDOW_USEC",
	       "TELEMETRY_CONTEXT_SEGMENTS_PER_MINUTE", "TELEMETRY_PULSE_SLOT_COUNT",
	       "TELEMETRY_PROPERTY_CATALOG_FILE" })
		unsetenv(name);
}

void check_disabled_default()
{
	const telemetry_runtime_options options = telemetry_runtime_default_options();
	assert(options.config.enabled == 0U);
	const telemetry_runtime_outcome expected =
		options.config.backend == telemetry_storage_backend::flatfile_disabled ?
			telemetry_runtime_outcome::flatfile_disabled :
			telemetry_runtime_outcome::disabled;
	assert(telemetry_runtime_init(options) == expected);
	assert(telemetry_runtime_health_copy().state == telemetry_health_state::disabled);
	assert(telemetry_runtime_shutdown({ 0U, 0U, {} }) == telemetry_runtime_outcome::accepted);
	assert(telemetry_runtime_final_reap() == telemetry_runtime_outcome::accepted);
	assert(telemetry_runtime_final_reap() == telemetry_runtime_outcome::accepted);
}

void check_environment_options()
{
	clear_telemetry_environment();
	setenv("TELEMETRY_ENABLED", "TRUE", 1);
	setenv("TELEMETRY_BACKEND", "sql", 1);
	setenv("TELEMETRY_INTERVAL_USEC", "1000000", 1);
	setenv("TELEMETRY_CHECKPOINT_INTERVAL_USEC", "2000000", 1);
	setenv("TELEMETRY_ACTIVE_WINDOW_USEC", "3000000", 1);
	setenv("TELEMETRY_CONTEXT_SEGMENTS_PER_MINUTE", "8", 1);
	setenv("TELEMETRY_PULSE_SLOT_COUNT", "2", 1);
	setenv("TELEMETRY_PROPERTY_VERSION", "4294967295", 1);

	reload_property_values environment_values{ 0.0F, 10.0F, 1.0F, 0.2F, 0.15F };
	telemetry_config_property_capture probe_capture{};
	probe_capture.reader = { reload_property_read, &environment_values };
	probe_capture.mode = telemetry_config_property_capture_mode::require_reader;
	telemetry_config_property_snapshot probe{};
	assert(telemetry_config_property_snapshot_capture(&probe_capture, &probe) ==
	       telemetry_config_build_outcome::built);
	char catalog_path[128]{};
	std::snprintf(catalog_path, sizeof(catalog_path), "/tmp/duris-telemetry-catalog-%ld.txt",
		      static_cast<long>(::getpid()));
	write_reviewed_catalog(catalog_path, probe, 0xA1B2C3D4U);
	setenv("TELEMETRY_PROPERTY_CATALOG_FILE", catalog_path, 1);
	const telemetry_runtime_options enabled = telemetry_runtime_options_from_environment();
	assert(enabled.config.interval_usec == 1'000'000U);
	assert(enabled.config.checkpoint_interval_usec == 2'000'000U);
	assert(enabled.config.active_window_usec == 3'000'000U);
	assert(enabled.config.context_segments_per_minute == 8U);
	assert(enabled.config.pulse_slot_count == 2U);
	assert(telemetry_config_is_valid(enabled.config));
#ifdef __NO_MYSQL__
	assert(enabled.config.enabled == 0U);
	assert(enabled.config.backend == telemetry_storage_backend::flatfile_disabled);
#else
	assert(enabled.config.enabled == 1U);
	assert(enabled.config.backend == telemetry_storage_backend::sql);
	assert(enabled.config.property_version == 0xA1B2C3D4U);
	assert(enabled.property_capture_enabled == 1U);
#endif

	unsetenv("TELEMETRY_PROPERTY_CATALOG_FILE");
	const telemetry_runtime_options missing_catalog =
		telemetry_runtime_options_from_environment();
	assert(missing_catalog.config.enabled == 0U);
	setenv("TELEMETRY_PROPERTY_CATALOG_FILE", catalog_path, 1);
	setenv("TELEMETRY_INTERVAL_USEC", "not-a-duration", 1);
	const telemetry_runtime_options invalid = telemetry_runtime_options_from_environment();
	assert(invalid.config.enabled == 0U);
	assert(std::remove(catalog_path) == 0);
	clear_telemetry_environment();
}

telemetry_transport_config make_transport_config()
{
	telemetry_transport_config config{};
	config.backend = telemetry_storage_backend::sql;
	config.schema_version = TELEMETRY_SCHEMA_VERSION;
	config.queue_capacity = TELEMETRY_QUEUE_CAPACITY_PROPOSAL;
	config.control_reserve = TELEMETRY_CONTROL_RESERVE_PROPOSAL;
	config.max_batch_records = TELEMETRY_BATCH_MAX_RECORDS_PROPOSAL;
	config.max_batch_bytes = TELEMETRY_BATCH_MAX_BYTES_PROPOSAL;
	config.flush_oldest_after_usec = TELEMETRY_FLUSH_OLDEST_USEC_PROPOSAL;
	return config;
}

void check_transport_shutdown_without_writer()
{
	fake_repository fake{};
	const telemetry_transport_repository_binding repository = { fake_init, fake_apply,
								    fake_request_stop,
								    fake_shutdown, &fake };
	const telemetry_transport_clock_binding clock = { fake_clock, nullptr };
	assert(telemetry_transport_bind_for_tests(&repository, &clock) ==
	       telemetry_transport_outcome::started);
	assert(telemetry_transport_init(make_transport_config()) ==
	       telemetry_transport_outcome::started);
	assert(telemetry_transport_request_stop() == telemetry_transport_outcome::stopping);
	telemetry_transport_repository_shutdown_for_owner();
	telemetry_transport_shutdown();
	assert(fake.init_calls == 0U);
	assert(fake.request_stop_calls == 0U);
	assert(fake.shutdown_calls == 0U);
	telemetry_transport_unbind_for_tests();
}

void check_transport_db_down_shutdown()
{
	fake_repository fake{};
	fake.init_mode = fake_repository::mode::unavailable;
	const telemetry_transport_repository_binding repository = { fake_init, fake_apply,
								    fake_request_stop,
								    fake_shutdown, &fake };
	const telemetry_transport_clock_binding clock = { fake_clock, nullptr };
	assert(telemetry_transport_bind_for_tests(&repository, &clock) ==
	       telemetry_transport_outcome::started);
	assert(telemetry_transport_init(make_transport_config()) ==
	       telemetry_transport_outcome::started);
	assert(telemetry_transport_pulse(1U).outcome == telemetry_transport_outcome::unavailable);
	telemetry_transport_repository_shutdown_for_owner();
	telemetry_transport_shutdown();
	assert(fake.init_calls == 1U);
	assert(fake.request_stop_calls == 0U);
	assert(fake.shutdown_calls == 0U);
	telemetry_transport_unbind_for_tests();
}

void check_enabled_lifecycle()
{
	fake_repository fake{};
	const telemetry_transport_repository_binding repository = { fake_init, fake_apply,
								    fake_request_stop,
								    fake_shutdown, &fake };
	const telemetry_transport_clock_binding clock = { fake_clock, nullptr };
	assert(telemetry_transport_bind_for_tests(&repository, &clock) ==
	       telemetry_transport_outcome::started);

	const telemetry_runtime_options options = make_enabled_options();
	assert(telemetry_runtime_init(options) == telemetry_runtime_outcome::accepted);
	const telemetry_session_enter enter = make_enter(options.producer, options.config);
	const telemetry_capture_result entered = telemetry_runtime_session_enter(enter);
	assert(entered.outcome == telemetry_runtime_outcome::accepted);
	assert(entered.records_emitted >= 1U);

	telemetry_context_update context{};
	context.session = enter.session;
	context.connection = enter.connection;
	context.at_monotonic_usec = 1'100U;
	context.at_utc_usec = 2'100U;
	context.dimensions = enter.dimensions;
	context.category = telemetry_interval_category::connected_active;
	context.context = telemetry_activity_context::combat;
	context.context_quality = telemetry_context_quality::observed;
	context.config_id = options.config.config_id;
	context.classifier_version = options.config.classifier_version;
	context.policy_version = options.config.policy_version;
	const telemetry_capture_result updated = telemetry_runtime_update_context(context);
	assert(updated.outcome == telemetry_runtime_outcome::accepted);

	const telemetry_pulse_result pulse = telemetry_runtime_pulse({ 1'400U, 2'400U, 0U, 0U });
	assert(pulse.outcome == telemetry_runtime_outcome::accepted);

	telemetry_session_exit exit{};
	exit.session = enter.session;
	exit.connection = enter.connection;
	exit.at_monotonic_usec = 1'500U;
	exit.at_utc_usec = 2'500U;
	exit.reason = telemetry_session_end_reason::logout;
	const telemetry_capture_result exited = telemetry_runtime_session_exit(exit);
	assert(exited.outcome == telemetry_runtime_outcome::accepted);

	telemetry_shutdown_request shutdown{};
	telemetry_monotonic_usec now_monotonic = 0U;
	telemetry_utc_usec now_utc = 0;
	assert(telemetry_runtime_now(&now_monotonic, &now_utc));
	shutdown.deadline_monotonic_usec = now_monotonic + 5'000'000U;
	shutdown.final_flush = 1U;
	assert(telemetry_runtime_shutdown(shutdown) == telemetry_runtime_outcome::accepted);
	assert(telemetry_runtime_final_reap() == telemetry_runtime_outcome::accepted);
	assert(fake.init_calls == 1U);
	assert(fake.apply_calls != 0U);
	assert(fake.applied_records != 0U);
	assert(fake.shutdown_calls == 1U);
	telemetry_transport_unbind_for_tests();
}

void check_game_context_and_copyover_handoff()
{
	fake_repository fake{};
	const telemetry_transport_repository_binding repository = { fake_init, fake_apply,
								    fake_request_stop,
								    fake_shutdown, &fake };
	const telemetry_transport_clock_binding clock = { fake_clock, nullptr };
	assert(telemetry_transport_bind_for_tests(&repository, &clock) ==
	       telemetry_transport_outcome::started);
	const telemetry_runtime_options options = make_enabled_options();
	assert(telemetry_runtime_init(options) == telemetry_runtime_outcome::accepted);

	room_data rooms[1]{};
	zone_data zones[1]{};
	rooms[0].zone = 0U;
	zones[0].number = 700;
	world = rooms;
	zone_table = zones;
	top_of_world = 0;
	top_of_zone_table = 0;

	char_data player{};
	pc_only_data player_pc{};
	player.only.pc = &player_pc;
	player_pc.pid = 42;
	player.in_room = 0;
	player.player.level = 11U;
	player.player.m_class = 3U;
	player.player.race = 4U;
	player.player.racewar = 5U;

	char_data member{};
	pc_only_data member_pc{};
	member.only.pc = &member_pc;
	member_pc.pid = 43;
	group_list group_member{ &member, nullptr };
	group_list group{ &player, &group_member };
	player.group = &group;

	descriptor_data descriptor{};
	descriptor.connected = CON_PLAYING;
	assert(telemetry_runtime_game_enter(&player, &descriptor).outcome ==
	       telemetry_runtime_outcome::accepted);
	assert(telemetry_runtime_game_context(&player, &descriptor).outcome ==
	       telemetry_runtime_outcome::accepted);
	group_member.next = &group;
	const telemetry_capture_result oversized_context =
		telemetry_runtime_game_context(&player, &descriptor);
	assert(oversized_context.outcome == telemetry_runtime_outcome::accepted);
	assert((oversized_context.quality_flags & TELEMETRY_QUALITY_DIMENSION_UNKNOWN) != 0U);
	group_member.next = nullptr;
	const telemetry_handoff_result handoff = telemetry_runtime_game_handoff_copy(&player);
	assert(handoff.outcome == telemetry_runtime_outcome::accepted);
	assert(handoff.handoff.session.id.session_seq == player.telemetry_session_sequence);
	assert(handoff.handoff.previous_producer.boot_id == options.producer.boot_id);

	telemetry_shutdown_request shutdown{};
	telemetry_monotonic_usec now = 0U;
	telemetry_utc_usec ignored_utc = 0;
	assert(telemetry_runtime_now(&now, &ignored_utc));
	shutdown.deadline_monotonic_usec = now + 5'000'000U;
	shutdown.final_flush = 1U;
	assert(telemetry_runtime_shutdown(shutdown) == telemetry_runtime_outcome::accepted);
	assert(telemetry_runtime_final_reap() == telemetry_runtime_outcome::accepted);
	assert(fake.saw_dimensions);
	assert(fake.dimensions.level_band == 3U);
	assert(fake.dimensions.class_id == 3U);
	assert(fake.dimensions.race_id == 4U);
	assert(fake.dimensions.faction_id == 5U);
	assert(fake.dimensions.zone_vnum == 700);
	assert(fake.dimensions.group_size == 2U);

	telemetry_transport_unbind_for_tests();

	fake_repository resumed_fake{};
	const telemetry_transport_repository_binding resumed_repository = {
		fake_init, fake_apply, fake_request_stop, fake_shutdown, &resumed_fake
	};
	assert(telemetry_transport_bind_for_tests(&resumed_repository, &clock) ==
	       telemetry_transport_outcome::started);
	telemetry_runtime_options resumed_options = make_enabled_options();
	resumed_options.producer.boot_id = options.producer.boot_id + 1U;
	resumed_options.producer.process_id = options.producer.process_id + 1U;
	assert(telemetry_runtime_init(resumed_options) == telemetry_runtime_outcome::accepted);

	char_data resumed_player{};
	pc_only_data resumed_pc{};
	resumed_player.only.pc = &resumed_pc;
	resumed_pc.pid = 42;
	resumed_player.in_room = 0;
	resumed_player.player.level = 11U;
	resumed_player.player.m_class = 3U;
	resumed_player.player.race = 4U;
	resumed_player.player.racewar = 5U;
	descriptor_data resumed_descriptor{};
	resumed_descriptor.connected = CON_PLAYING;
	telemetry_session_handoff invalid_handoff = handoff.handoff;
	invalid_handoff.previous_producer = resumed_options.producer;
	assert(telemetry_runtime_game_session_resume(&resumed_player, &resumed_descriptor,
						     &invalid_handoff)
		       .outcome == telemetry_runtime_outcome::invalid);
	assert(resumed_player.telemetry_session_sequence == 0U);
	assert(resumed_descriptor.telemetry_connection_sequence == 0U);
	const telemetry_capture_result resumed = telemetry_runtime_game_session_resume(
		&resumed_player, &resumed_descriptor, &handoff.handoff);
	assert(resumed.outcome == telemetry_runtime_outcome::accepted);
	assert(resumed_player.telemetry_session_sequence == handoff.handoff.session.id.session_seq);
	assert(resumed_player.telemetry_session_producer_boot_id ==
	       handoff.handoff.session.id.producer.boot_id);
	assert(telemetry_runtime_game_context(&resumed_player, &resumed_descriptor).outcome ==
	       telemetry_runtime_outcome::accepted);
	assert(telemetry_runtime_now(&now, &ignored_utc));
	shutdown.deadline_monotonic_usec = now + 5'000'000U;
	assert(telemetry_runtime_shutdown(shutdown) == telemetry_runtime_outcome::accepted);
	assert(telemetry_runtime_final_reap() == telemetry_runtime_outcome::accepted);
	telemetry_transport_unbind_for_tests();

	fake_repository absent_fake{};
	const telemetry_transport_repository_binding absent_repository = {
		fake_init, fake_apply, fake_request_stop, fake_shutdown, &absent_fake
	};
	assert(telemetry_transport_bind_for_tests(&absent_repository, &clock) ==
	       telemetry_transport_outcome::started);
	telemetry_runtime_options absent_options = make_enabled_options();
	absent_options.producer.boot_id = resumed_options.producer.boot_id + 1U;
	absent_options.producer.process_id = resumed_options.producer.process_id + 1U;
	assert(telemetry_runtime_init(absent_options) == telemetry_runtime_outcome::accepted);
	char_data absent_player{};
	pc_only_data absent_pc{};
	absent_player.only.pc = &absent_pc;
	absent_pc.pid = 44;
	absent_player.in_room = 0;
	descriptor_data absent_descriptor{};
	absent_descriptor.connected = CON_PLAYING;
	// A malformed handoff that passes identity prechecks must also leave the
	// descriptor clean, so copyover may retry once with absent/uncertain state.
	telemetry_session_handoff damaged = handoff.handoff;
	damaged.session.subject_id = 44U;
	damaged.session.pid = 44;
	damaged.cumulative = {};
	damaged.cumulative.active_usec = 1U; // impossible: active exceeds connected
	assert(telemetry_runtime_game_session_resume(&absent_player, &absent_descriptor, &damaged)
		       .outcome == telemetry_runtime_outcome::invalid);
	assert(absent_player.telemetry_session_sequence == 0U);
	assert(absent_descriptor.telemetry_connection_sequence == 0U);
	assert(telemetry_runtime_game_session_resume(&absent_player, &absent_descriptor, nullptr)
		       .outcome == telemetry_runtime_outcome::accepted);
	assert(telemetry_runtime_now(&now, &ignored_utc));
	shutdown.deadline_monotonic_usec = now + 5'000'000U;
	assert(telemetry_runtime_shutdown(shutdown) == telemetry_runtime_outcome::accepted);
	assert(telemetry_runtime_final_reap() == telemetry_runtime_outcome::accepted);
	assert(absent_fake.saw_unclosed_tail);
	telemetry_transport_unbind_for_tests();
	world = nullptr;
	zone_table = nullptr;
	top_of_world = -1;
	top_of_zone_table = -1;
}

telemetry_apply_batch_result reject_copyover_batch(void *context, const telemetry_record *records,
						   std::size_t count) noexcept
{
	auto result = fake_apply(context, records, count);
	result.outcome = telemetry_batch_outcome::committed_with_rejections;
	result.applied_count = 0U;
	result.invalid_count = static_cast<std::uint16_t>(count);
	for (std::size_t index = 0U; index < count; ++index)
		result.results[index].outcome = telemetry_apply_outcome::rejected_invalid;
	return result;
}

void check_copyover_durability_barrier(bool blocked, bool rejected, bool unavailable)
{
	fake_repository fake{};
	fake.block_apply = blocked;
	if (unavailable)
		fake.init_mode = fake_repository::mode::unavailable;
	const telemetry_transport_repository_binding repository = {
		fake_init, rejected ? reject_copyover_batch : fake_apply, fake_request_stop,
		fake_shutdown, &fake
	};
	const telemetry_transport_clock_binding clock = { fake_clock, nullptr };
	assert(telemetry_transport_bind_for_tests(&repository, &clock) ==
	       telemetry_transport_outcome::started);
	const auto options = make_enabled_options();
	assert(telemetry_runtime_init(options) == telemetry_runtime_outcome::accepted);
	const auto enter = make_enter(options.producer, options.config);
	assert(telemetry_runtime_session_enter(enter).outcome ==
	       telemetry_runtime_outcome::accepted);
	std::thread unblocker;
	if (blocked)
	{
		// Fill one batch and wait for its callback before measuring the deadline;
		// otherwise a heavily loaded host can miss the first 40ms request entirely.
		for (std::uint64_t sequence = 2U; sequence <= TELEMETRY_BATCH_MAX_RECORDS_PROPOSAL;
		     ++sequence)
		{
			auto extra = enter;
			extra.session.id.session_seq = sequence;
			extra.connection.connection_seq = sequence;
			assert(telemetry_runtime_session_enter(extra).outcome ==
			       telemetry_runtime_outcome::accepted);
		}
		assert(fake.apply_callback.wait_until_entered(std::chrono::seconds(5)));
	}
	telemetry_monotonic_usec now = 0U;
	telemetry_utc_usec utc = 0;
	assert(telemetry_runtime_now(&now, &utc));
	const auto start = std::chrono::steady_clock::now();
	const auto first = telemetry_runtime_flush_for_copyover(now + 40'000U);
	const auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
				     std::chrono::steady_clock::now() - start)
				     .count();
	assert(elapsed < 130);
	assert(first == (blocked || rejected || unavailable ?
				 telemetry_runtime_outcome::queue_full :
				 telemetry_runtime_outcome::accepted));
	if (blocked)
	{
		assert(fake.apply_callback.wait_until_entered(std::chrono::seconds(1)));
		// Admitted records are still owned by the blocked repository call.
		const auto health = telemetry_transport_health_copy();
		assert(health.admitted_control > 0U && health.applied_records == 0U);
		unblocker = std::thread(
			[&]
			{
				std::this_thread::sleep_for(std::chrono::milliseconds(150));
				fake.apply_callback.unblock();
			});
	}
	// Simulate failed exec: normal admissions and the same worker remain live.
	assert(telemetry_runtime_record_evidence({ enter.session,
						   enter.connection,
						   1'200U,
						   2'200U,
						   telemetry_runtime_evidence_kind::player_action,
						   {},
						   TELEMETRY_QUALITY_NONE })
		       .outcome == telemetry_runtime_outcome::accepted);
	assert(telemetry_runtime_now(&now, &utc));
	const auto next_start = std::chrono::steady_clock::now();
	// Even an oversized caller deadline is capped by the runtime's 250ms limit.
	const auto next = telemetry_runtime_flush_for_copyover(now + 5'000'000U);
	assert(std::chrono::steady_clock::now() - next_start < std::chrono::milliseconds(350));
	if (unblocker.joinable())
		unblocker.join();
	assert(next == (rejected || unavailable ? telemetry_runtime_outcome::queue_full :
						  telemetry_runtime_outcome::accepted));
	std::printf("copyover barrier: blocked=%u rejected=%u unavailable=%u first=%u "
		    "next=%u elapsed=%lldms\n",
		    blocked, rejected, unavailable, static_cast<unsigned>(first),
		    static_cast<unsigned>(next), static_cast<long long>(elapsed));
	assert(telemetry_runtime_now(&now, &utc));
	assert(telemetry_runtime_shutdown({ now + 100'000U, 0U, {} }) ==
	       telemetry_runtime_outcome::accepted);
	assert(telemetry_runtime_final_reap() == telemetry_runtime_outcome::accepted);
	telemetry_transport_unbind_for_tests();
}

void check_producer_reuse_rejected()
{
	fake_repository fake{};
	const telemetry_transport_repository_binding repository = { fake_init, fake_apply,
								    fake_request_stop,
								    fake_shutdown, &fake };
	const telemetry_transport_clock_binding clock = { fake_clock, nullptr };
	assert(telemetry_transport_bind_for_tests(&repository, &clock) ==
	       telemetry_transport_outcome::started);
	const auto options = make_enabled_options();
	const auto finish = []
	{
		telemetry_monotonic_usec now = 0U;
		telemetry_utc_usec utc = 0;
		assert(telemetry_runtime_now(&now, &utc));
		assert(telemetry_runtime_shutdown({ now + 5'000'000U, 1U, {} }) ==
		       telemetry_runtime_outcome::accepted);
		assert(telemetry_runtime_final_reap() == telemetry_runtime_outcome::accepted);
	};
	assert(telemetry_runtime_init(options) == telemetry_runtime_outcome::accepted);
	finish();
	assert(fake.required_fresh_producer.boot_id == options.producer.boot_id);
	assert(fake.required_fresh_producer.process_id == options.producer.process_id);
	const auto reused = telemetry_runtime_init(options);
	if (reused == telemetry_runtime_outcome::accepted)
		finish(); // clean the failing-first path before checking its result
	const auto fresh = make_enabled_options();
	assert(fresh.producer.boot_id != options.producer.boot_id ||
	       fresh.producer.process_id != options.producer.process_id);
	assert(telemetry_runtime_init(fresh) == telemetry_runtime_outcome::accepted);
	finish();
	telemetry_transport_unbind_for_tests();
	std::printf("runtime producer reuse: outcome=%u fresh=accepted\n",
		    static_cast<unsigned>(reused));
	std::fflush(stdout);
	assert(reused == telemetry_runtime_outcome::invalid);
}

void check_classifier_counter_ownership()
{
	fake_repository fake{};
	const telemetry_transport_repository_binding repository = { fake_init, fake_apply,
								    fake_request_stop,
								    fake_shutdown, &fake };
	const telemetry_transport_clock_binding clock = { fake_clock, nullptr };
	assert(telemetry_transport_bind_for_tests(&repository, &clock) ==
	       telemetry_transport_outcome::started);
	const auto options = make_enabled_options();
	assert(telemetry_runtime_init(options) == telemetry_runtime_outcome::accepted);
	const auto enter = make_enter(options.producer, options.config);
	assert(telemetry_runtime_session_enter(enter).outcome ==
	       telemetry_runtime_outcome::accepted);
	assert(telemetry_runtime_record_evidence({ enter.session,
						   enter.connection,
						   1'000U,
						   2'000U,
						   telemetry_runtime_evidence_kind::player_action,
						   {},
						   TELEMETRY_QUALITY_NONE })
		       .outcome == telemetry_runtime_outcome::accepted);

	// A syntactically valid owner update must not advance the session past the
	// classifier's cut. Previously it poisoned every subsequent classifier delta.
	telemetry_counter_update foreign{};
	foreign.session = enter.session;
	foreign.connection = enter.connection;
	foreign.at_monotonic_usec = 1'500U;
	foreign.connected_delta_usec = 500U;
	foreign.unknown_delta_usec = 500U;
	foreign.resident_delta_usec = 500U;
	assert(telemetry_counter_update_is_valid(foreign));
	const auto rejected = telemetry_runtime_update_counters(foreign);
	const auto first = telemetry_runtime_pulse({ 2'000U, 3'000U, 0U, 0U });
	const auto second = telemetry_runtime_pulse({ 3'000U, 4'000U, 0U, 0U });
	const auto handoff = telemetry_runtime_session_handoff_copy(enter.session);
	std::printf("counter ownership: external=%u pulses=%u/%u handoff=%u active=%llu\n",
		    static_cast<unsigned>(rejected.outcome), static_cast<unsigned>(first.outcome),
		    static_cast<unsigned>(second.outcome), static_cast<unsigned>(handoff.outcome),
		    static_cast<unsigned long long>(handoff.handoff.cumulative.active_usec));
	std::fflush(stdout);
	telemetry_monotonic_usec now = 0U;
	telemetry_utc_usec utc = 0;
	assert(telemetry_runtime_now(&now, &utc));
	assert(telemetry_runtime_shutdown({ now + 5'000'000U, 1U, {} }) ==
	       telemetry_runtime_outcome::accepted);
	assert(telemetry_runtime_final_reap() == telemetry_runtime_outcome::accepted);
	telemetry_transport_unbind_for_tests();
	assert(rejected.outcome == telemetry_runtime_outcome::invalid);
	assert(first.outcome == telemetry_runtime_outcome::accepted);
	assert(second.outcome == telemetry_runtime_outcome::accepted);
	assert(handoff.outcome == telemetry_runtime_outcome::accepted);
	assert(handoff.handoff.cumulative.active_usec == 2'000U);
}

void check_effective_property_reload()
{
	fake_repository fake{};
	const telemetry_transport_repository_binding repository = { fake_init, fake_apply,
								    fake_request_stop,
								    fake_shutdown, &fake };
	const telemetry_transport_clock_binding clock = { fake_clock, nullptr };
	assert(telemetry_transport_bind_for_tests(&repository, &clock) ==
	       telemetry_transport_outcome::started);

	reload_property_values values{ 0.0F, 10.0F, 1.0F, 0.2F, 0.15F };
	reload_property_catalog catalog{};
	const telemetry_runtime_options options = make_property_reload_options(values, catalog);
	assert(telemetry_runtime_init(options) == telemetry_runtime_outcome::accepted);
	const telemetry_config_snapshot initial = telemetry_config_snapshot_copy();
	assert(telemetry_config_is_valid(initial));
	assert(initial.property_version == 1001U);
	const telemetry_session_enter enter = make_enter(options.producer, initial);
	assert(telemetry_runtime_session_enter(enter).outcome ==
	       telemetry_runtime_outcome::accepted);

	/* apply_properties() may notify even when the effective typed values are
	 * unchanged. The admitted identity is reused and capture remains enabled. */
	telemetry_config_reload_notify();
	const telemetry_config_snapshot unchanged = telemetry_config_snapshot_copy();
	assert(telemetry_config_is_valid(unchanged));
	assert(unchanged.property_version == initial.property_version);
	assert(unchanged.config_id == initial.config_id);
	assert(telemetry_runtime_record_evidence({ enter.session,
						   enter.connection,
						   1'100U,
						   2'100U,
						   telemetry_runtime_evidence_kind::player_action,
						   {},
						   TELEMETRY_QUALITY_NONE })
		       .outcome == telemetry_runtime_outcome::accepted);

	/* A changed effective value requires a new reviewed full-digest mapping;
	 * the runtime never increments the public property version itself. */
	values.payout_factor = 1.25F;
	telemetry_config_property_capture changed_probe_capture{};
	changed_probe_capture.reader = { reload_property_read, &values };
	changed_probe_capture.mode = telemetry_config_property_capture_mode::require_reader;
	telemetry_config_property_snapshot changed_probe{};
	assert(telemetry_config_property_snapshot_capture(&changed_probe_capture, &changed_probe) ==
	       telemetry_config_build_outcome::built);
	reload_catalog_add(catalog, changed_probe, 1002U);
	telemetry_config_reload_notify();
	const telemetry_config_snapshot changed = telemetry_config_snapshot_copy();
	assert(telemetry_config_is_valid(changed));
	assert(changed.property_version == 1002U);
	assert(changed.config_id != initial.config_id);
	assert(telemetry_runtime_record_evidence({ enter.session,
						   enter.connection,
						   1'200U,
						   2'200U,
						   telemetry_runtime_evidence_kind::movement,
						   {},
						   TELEMETRY_QUALITY_NONE })
		       .outcome == telemetry_runtime_outcome::accepted);

	/* Unknown digest: visibility is withdrawn, but logical session scope is not
	 * erased. Pulse and handoff stay unavailable instead of using changed's
	 * stale identity. */
	values.alignment_mod = 0.35F;
	telemetry_config_property_snapshot unknown_probe{};
	assert(telemetry_config_property_snapshot_capture(&changed_probe_capture, &unknown_probe) ==
	       telemetry_config_build_outcome::built);
	telemetry_config_reload_notify();
	assert(telemetry_config_snapshot_copy().config_id == 0U);
	assert(telemetry_runtime_pulse({ 3'000U, 4'000U, 0U, 0U }).outcome ==
	       telemetry_runtime_outcome::queue_full);
	assert(telemetry_runtime_session_handoff_copy(enter.session).outcome ==
	       telemetry_runtime_outcome::queue_full);

	/* A reviewed mapping can be admitted on the next post-apply notification;
	 * the same logical session then resumes pulse/handoff continuity. */
	reload_catalog_add(catalog, unknown_probe, 1003U);
	telemetry_config_reload_notify();
	const telemetry_config_snapshot recovered = telemetry_config_snapshot_copy();
	assert(telemetry_config_is_valid(recovered));
	assert(recovered.property_version == 1003U);
	assert(telemetry_runtime_pulse({ 5'000U, 6'000U, 0U, 0U }).outcome ==
	       telemetry_runtime_outcome::accepted);
	const telemetry_handoff_result handoff =
		telemetry_runtime_session_handoff_copy(enter.session);
	assert(handoff.outcome == telemetry_runtime_outcome::accepted);
	assert(handoff.handoff.session.id.producer.boot_id == enter.session.id.producer.boot_id);
	assert(handoff.handoff.session.id.producer.process_id ==
	       enter.session.id.producer.process_id);
	assert(handoff.handoff.session.id.session_seq == enter.session.id.session_seq);
	assert(handoff.handoff.session.season_id == enter.session.season_id);
	assert(handoff.handoff.session.environment_id == enter.session.environment_id);
	telemetry_session_exit exit{};
	exit.session = enter.session;
	exit.connection = enter.connection;
	exit.at_monotonic_usec = 0U;
	exit.at_utc_usec = TELEMETRY_UTC_UNKNOWN;
	assert(telemetry_runtime_now(&exit.at_monotonic_usec, &exit.at_utc_usec));
	exit.reason = telemetry_session_end_reason::logout;
	const telemetry_capture_result exited = telemetry_runtime_session_exit(exit);
	assert(exited.outcome == telemetry_runtime_outcome::accepted);

	telemetry_monotonic_usec now = 0U;
	telemetry_utc_usec ignored_utc = 0U;
	assert(telemetry_runtime_now(&now, &ignored_utc));
	assert(telemetry_runtime_shutdown({ now + 5'000'000U, 1U, {} }) ==
	       telemetry_runtime_outcome::accepted);
	assert(telemetry_runtime_final_reap() == telemetry_runtime_outcome::accepted);
	telemetry_transport_unbind_for_tests();
}

void check_bounded_shutdown_request_and_final_reap()
{
	fake_repository fake{};
	fake.block_apply = true;
	const telemetry_transport_repository_binding repository = { fake_init, fake_apply,
								    fake_request_stop,
								    fake_shutdown, &fake };
	const telemetry_transport_clock_binding clock = { fake_clock, &fake };
	assert(telemetry_transport_bind_for_tests(&repository, &clock) ==
	       telemetry_transport_outcome::started);
	const telemetry_runtime_options options = make_enabled_options();
	assert(telemetry_runtime_init(options) == telemetry_runtime_outcome::accepted);
	const telemetry_session_enter enter = make_enter(options.producer, options.config);
	const telemetry_capture_result entered = telemetry_runtime_session_enter(enter);
	assert(entered.outcome == telemetry_runtime_outcome::accepted);
	fake.transport_now.store(3'000'000U, std::memory_order_release);
	assert(fake.apply_callback.wait_until_entered(std::chrono::seconds(1)));

	finite_callback_releaser releaser(fake.apply_callback, std::chrono::milliseconds(250));
	telemetry_monotonic_usec now = 0U;
	telemetry_utc_usec ignored_utc = 0;
	assert(telemetry_runtime_now(&now, &ignored_utc));
	telemetry_shutdown_request shutdown{};
	shutdown.deadline_monotonic_usec = now + 50'000U;
	shutdown.final_flush = 1U;
	const auto request_start = std::chrono::steady_clock::now();
	const telemetry_runtime_outcome requested = telemetry_runtime_shutdown(shutdown);
	const auto request_elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
		std::chrono::steady_clock::now() - request_start);
	std::printf("blocked apply shutdown request: outcome=%u elapsed=%lldms\n",
		    static_cast<unsigned>(requested),
		    static_cast<long long>(request_elapsed.count()));
	std::fflush(stdout);
	assert(requested == telemetry_runtime_outcome::stopping);
	assert(request_elapsed < std::chrono::milliseconds(200));
	assert(fake.shutdown_calls == 0U);
	assert(telemetry_config_snapshot_copy().config_id == options.config.config_id);
	assert(telemetry_runtime_init(options) != telemetry_runtime_outcome::accepted);

	telemetry_context_update context{};
	context.session = enter.session;
	context.connection = enter.connection;
	context.at_monotonic_usec = 1'100U;
	context.at_utc_usec = 2'100U;
	context.dimensions = enter.dimensions;
	context.category = telemetry_interval_category::connected_idle;
	context.context = telemetry_activity_context::none;
	context.context_quality = telemetry_context_quality::observed;
	context.config_id = options.config.config_id;
	context.classifier_version = options.config.classifier_version;
	context.policy_version = options.config.policy_version;
	assert(telemetry_runtime_update_context(context).outcome ==
	       telemetry_runtime_outcome::stopping);

	releaser.release_and_join();
	assert(telemetry_runtime_final_reap() == telemetry_runtime_outcome::accepted);
	assert(fake.request_stop_calls == 1U);
	assert(fake.shutdown_calls == 1U);
	assert(fake.request_stop_calls == 1U);
	assert(fake.shutdown_calls == 1U);
	telemetry_transport_unbind_for_tests();

	fake_repository resumed_fake{};
	const telemetry_transport_repository_binding resumed_repository = {
		fake_init, fake_apply, fake_request_stop, fake_shutdown, &resumed_fake
	};
	assert(telemetry_transport_bind_for_tests(&resumed_repository, &clock) ==
	       telemetry_transport_outcome::started);
	// A new runtime after final reap requires a new producer incarnation.
	assert(telemetry_runtime_init(make_enabled_options()) ==
	       telemetry_runtime_outcome::accepted);
	assert(telemetry_runtime_now(&now, &ignored_utc));
	shutdown.deadline_monotonic_usec = now + 5'000'000U;
	assert(telemetry_runtime_shutdown(shutdown) == telemetry_runtime_outcome::accepted);
	assert(telemetry_runtime_final_reap() == telemetry_runtime_outcome::accepted);
	assert(resumed_fake.request_stop_calls == 1U);
	assert(resumed_fake.shutdown_calls == 1U);
	telemetry_transport_unbind_for_tests();
}
} // namespace

int main()
{
	check_disabled_default();
	check_environment_options();
	check_transport_shutdown_without_writer();
	check_transport_db_down_shutdown();
	check_enabled_lifecycle();
	check_game_context_and_copyover_handoff();
	check_copyover_durability_barrier(false, false, false);
	check_copyover_durability_barrier(true, false, false);
	check_copyover_durability_barrier(false, true, false);
	check_copyover_durability_barrier(false, false, true);
	check_producer_reuse_rejected();
	check_classifier_counter_ownership();
	check_effective_property_reload();
	check_bounded_shutdown_request_and_final_reap();
	std::puts("telemetry runtime lifecycle and copyover integration passed");
	return 0;
}
