// Integration-facing regressions added during parent review.
#define main original_config_harness_main
#include "telemetry_config_harness.cc"
#undef main

namespace
{
telemetry_config_snapshot fixture(telemetry_config_revision revision, std::uint32_t policy)
{
	property_values values{ 0.0F, 10.0F, 1.0F, 0.2F, 0.15F, 0U, 0U, 1.0F };
	auto capture = make_capture(values);
	telemetry_config_property_snapshot properties{};
	CHECK(telemetry_config_property_snapshot_capture(&capture, &properties) ==
	      telemetry_config_build_outcome::built);
	reviewed_property_catalog catalog{};
	catalog.reject_version_collision = true;
	catalog_add(catalog, properties);
	capture.catalog = { resolve_reviewed_catalog, &catalog };
	return build_snapshot(make_input(revision, 123456789LL, policy, capture));
}

void review_regressions()
{
	// Effective defaults and explicitly loaded identical values share one catalog key.
	property_values defaults_values{ 0.0F, 10.0F, 1.0F, 0.1F, 0.1F, 0U, 0U, 1.0F };
	auto capture = make_capture(defaults_values);
	telemetry_config_property_snapshot loaded{};
	CHECK(telemetry_config_property_snapshot_capture(&capture, &loaded) ==
	      telemetry_config_build_outcome::built);
	const auto defaults = telemetry_config_property_snapshot_defaults();
	CHECK(std::memcmp(defaults.effective_digest, loaded.effective_digest,
			  sizeof(loaded.effective_digest)) == 0);

	const auto a1 = fixture(1U, 1U);
	const auto b2 = fixture(2U, 2U);
	const auto a3 = fixture(3U, 1U);
	const auto b3 = fixture(3U, 2U);
	finite_sink sink{ true, {}, 0U };
	key_space keys{ { 1U, 2U }, 1U, 0U };
	auto options = state_config(&sink, &keys);
	telemetry_config_state state{};
	CHECK(telemetry_config_state_init(&state, &options) ==
	      telemetry_config_state_outcome::accepted);
	CHECK(telemetry_config_state_publish(&state, a1).admission_accepted == 1U);
	sink.accept = false;
	CHECK(telemetry_config_state_publish(&state, b2).outcome ==
	      telemetry_config_state_outcome::sink_rejected);
	// Builders return a zero snapshot on failed capture. It must fence old retries too.
	CHECK(telemetry_config_state_publish(&state, {}).outcome ==
	      telemetry_config_state_outcome::invalid);
	sink.accept = true;
	(void)telemetry_config_state_publish(&state, b2);
	CHECK(telemetry_config_state_snapshot_copy(&state).config_id == 0U);

	CHECK(telemetry_config_state_init(&state, &options) ==
	      telemetry_config_state_outcome::accepted);
	CHECK(telemetry_config_state_publish(&state, a1).admission_accepted == 1U);
	sink.accept = false;
	CHECK(telemetry_config_state_publish(&state, b2).outcome ==
	      telemetry_config_state_outcome::sink_rejected);
	CHECK(telemetry_config_state_publish(&state, a3).outcome ==
	      telemetry_config_state_outcome::unchanged);
	sink.accept = true;
	// The pending fast path must not let B steal A's already selected revision.
	CHECK(telemetry_config_state_publish(&state, b3).outcome ==
	      telemetry_config_state_outcome::revision_conflict);
	CHECK(telemetry_config_state_snapshot_copy(&state).config_id == 0U);

	// Restart handoff must retain the whole original row, not just its ID.
	CHECK(telemetry_config_state_init(&state, &options) ==
	      telemetry_config_state_outcome::accepted);
	CHECK(telemetry_config_state_seed_identity(&state, a1) ==
	      telemetry_config_state_outcome::accepted);
	CHECK(telemetry_config_state_snapshot_copy(&state).config_id == 0U);
	const auto before = sink.count;
	CHECK(telemetry_config_state_publish(&state, a3).admission_accepted == 1U);
	CHECK(sink.count == before + 1U);
	CHECK(std::memcmp(&sink.records[before].payload.configuration.config, &a1, sizeof(a1)) ==
	      0);
	CHECK(telemetry_config_state_snapshot_copy(&state).revision == a3.revision);
	CHECK(telemetry_config_state_seed_identity(&state, b2) ==
	      telemetry_config_state_outcome::invalid);

	CHECK(telemetry_config_global_init(&options) == telemetry_config_state_outcome::accepted);
	CHECK(telemetry_config_publish(a1).outcome == telemetry_runtime_outcome::accepted);
	auto *global = telemetry_config_global_state();
	CHECK(telemetry_config_state_snapshot_copy(global).config_id ==
	      telemetry_config_snapshot_copy().config_id);
	telemetry_config_reload_observer(global);
	CHECK(telemetry_config_reload_requested(global));
	CHECK(telemetry_config_snapshot_copy().config_id == 0U);
	telemetry_config_global_reset();
}
}

int main()
{
	review_regressions();
	std::puts("telemetry config parent-review regressions passed");
	return original_config_harness_main();
}
