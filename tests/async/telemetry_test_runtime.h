#ifndef DURIS_TELEMETRY_TEST_RUNTIME_H
#define DURIS_TELEMETRY_TEST_RUNTIME_H

#include "telemetry/telemetry_runtime.h"

#include <cassert>
#include <chrono>
#include <cstdio>
#include <source_location>
#include <thread>

#ifdef TELEMETRY_TEST_STUB_REPOSITORY
/* Runtime-only fixtures do not link native account persistence. They retain
 * unknown identity; the account identity fixture executes the real adapter. */
bool sql_prepare_telemetry_account_token(const char *, std::uint64_t, std::uint64_t,
					 std::uint64_t *token)
{
	if (token)
		*token = 0U;
	return false;
}
#endif

/* Integration fixtures wait for the actual worker qualification boundary;
 * producer calls never initialize the repository or wait for SQL themselves. */
inline void
telemetry_test_start_runtime(const telemetry_runtime_options &options,
			     std::source_location caller = std::source_location::current())
{
	assert(telemetry_runtime_init(options) == telemetry_runtime_outcome::accepted);
	const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(2);
	while (telemetry_runtime_health_copy().state != telemetry_health_state::healthy &&
	       std::chrono::steady_clock::now() < deadline)
		std::this_thread::sleep_for(std::chrono::milliseconds(1));
	const auto health = telemetry_runtime_health_copy();
	if (health.state != telemetry_health_state::healthy)
		std::fprintf(
			stderr,
			"Writer qualification failed in %s:%u: state=%u backend=%u failure=%u\n",
			caller.function_name(), caller.line(), unsigned(health.state),
			unsigned(health.backend), unsigned(health.last_failure_class));
	assert(health.state == telemetry_health_state::healthy);
	telemetry_monotonic_usec now = 0U;
	telemetry_utc_usec utc = TELEMETRY_UTC_UNKNOWN;
	assert(telemetry_runtime_now(&now, &utc));
	(void)telemetry_runtime_pulse({ now, utc, 0U, 0U });
}

#endif
