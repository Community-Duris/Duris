#include "telemetry/telemetry_health.h"

#include <cassert>
#include <cstdint>
#include <cstring>
#include <iostream>

namespace
{

constexpr telemetry_monotonic_usec SECOND = 1'000'000U;
constexpr telemetry_monotonic_usec MINUTE = 60U * SECOND;

telemetry_health_snapshot healthy_snapshot()
{
	telemetry_health_snapshot health{};
	health.state = telemetry_health_state::healthy;
	health.backend = telemetry_storage_backend::sql;
	health.schema_version = TELEMETRY_SCHEMA_VERSION;
	health.queue_capacity = 100U;
	health.producer = { 41U, 73U };
	health.advisory_lock_state = telemetry_advisory_lock_state::held;
	return health;
}

void test_idle_does_not_stale()
{
	telemetry_health_monitor_state monitor{};
	const auto config = telemetry_health_monitor_default_config(MINUTE, 100U);
	auto health = healthy_snapshot();
	health.admitted_detail = 10U;
	health.applied_records = 10U;
	health.last_success_monotonic_usec = SECOND;
	auto event = telemetry_health_monitor_evaluate(&monitor, &config, health, 20U * MINUTE);
	assert(event.kind == telemetry_health_event_kind::state);
	event = telemetry_health_monitor_evaluate(&monitor, &config, health, 30U * MINUTE);
	assert(event.kind == telemetry_health_event_kind::none);
	assert(monitor.active_severity == telemetry_health_alert_severity::none);
}

void test_warning_critical_rate_limit_and_recovery()
{
	telemetry_health_monitor_state monitor{};
	const auto config = telemetry_health_monitor_default_config(MINUTE, 100U);
	auto health = healthy_snapshot();
	health.admitted_detail = 10U;
	health.applied_records = 10U;
	health.last_success_monotonic_usec = MINUTE;
	const telemetry_monotonic_usec admitted_at = 10U * MINUTE;
	(void)telemetry_health_monitor_evaluate(&monitor, &config, health, admitted_at - SECOND);

	health.admitted_detail = 11U;
	health.last_admitted_record_seq = 11U;
	health.last_committed_record_seq = 10U;
	auto event = telemetry_health_monitor_evaluate(&monitor, &config, health, admitted_at);
	assert(event.kind == telemetry_health_event_kind::none);
	event = telemetry_health_monitor_evaluate(&monitor, &config, health,
						  admitted_at + 2U * MINUTE - 1U);
	assert(event.kind == telemetry_health_event_kind::none);
	event = telemetry_health_monitor_evaluate(&monitor, &config, health,
						  admitted_at + 2U * MINUTE);
	assert(event.kind == telemetry_health_event_kind::alert);
	assert(event.severity == telemetry_health_alert_severity::warning);
	assert((event.reason_mask & TELEMETRY_HEALTH_REASON_STALLED) != 0U);
	assert(event.affected_first_record_seq == 11U);
	assert(event.affected_last_record_seq == 11U);

	event = telemetry_health_monitor_evaluate(&monitor, &config, health,
						  admitted_at + 2U * MINUTE + SECOND);
	assert(event.kind == telemetry_health_event_kind::none);
	event = telemetry_health_monitor_evaluate(&monitor, &config, health,
						  admitted_at + 5U * MINUTE);
	assert(event.kind == telemetry_health_event_kind::alert);
	assert(event.severity == telemetry_health_alert_severity::critical);
	event = telemetry_health_monitor_evaluate(&monitor, &config, health,
						  admitted_at + 5U * MINUTE + SECOND);
	assert(event.kind == telemetry_health_event_kind::none);
	event = telemetry_health_monitor_evaluate(&monitor, &config, health,
						  admitted_at + 10U * MINUTE);
	assert(event.kind == telemetry_health_event_kind::reminder);
	assert(event.severity == telemetry_health_alert_severity::critical);

	health.applied_records = 11U;
	health.last_committed_record_seq = 11U;
	health.last_success_monotonic_usec = admitted_at + 10U * MINUTE + SECOND;
	event = telemetry_health_monitor_evaluate(&monitor, &config, health,
						  admitted_at + 10U * MINUTE + SECOND);
	assert(event.kind == telemetry_health_event_kind::recovery);
	assert(event.alert_duration_usec == 8U * MINUTE + SECOND);
	assert(event.affected_first_record_seq == 11U);
	assert(event.affected_last_record_seq == 11U);
	event = telemetry_health_monitor_evaluate(&monitor, &config, health,
						  admitted_at + 10U * MINUTE + 2U * SECOND);
	assert(event.kind == telemetry_health_event_kind::none);
}

void test_permanent_failure_is_immediate_and_redacted()
{
	telemetry_health_monitor_state monitor{};
	const auto config = telemetry_health_monitor_default_config(MINUTE, 100U);
	auto health = healthy_snapshot();
	(void)telemetry_health_monitor_evaluate(&monitor, &config, health, MINUTE);
	health.state = telemetry_health_state::circuit_open;
	health.last_failure_class = telemetry_failure_class::permanent_schema;
	health.last_error_code = 1146U;
	health.last_failure_monotonic_usec = MINUTE + SECOND;
	health.last_failure_producer = health.producer;
	health.last_failure_first_record_seq = 20U;
	health.last_failure_last_record_seq = 22U;
	health.last_failure_record_kind_mask = std::uint64_t{ 1U } << static_cast<std::uint8_t>(
						       telemetry_record_kind::progression);
	health.inflight_active = 1U;
	health.inflight_first_record_seq = 20U;
	health.inflight_last_record_seq = 22U;
	health.inflight_record_kind_mask = health.last_failure_record_kind_mask;
	health.admitted_detail = 3U;
	health.last_admitted_record_seq = 22U;
	auto event = telemetry_health_monitor_evaluate(&monitor, &config, health, MINUTE + SECOND);
	assert(event.kind == telemetry_health_event_kind::alert);
	assert(event.severity == telemetry_health_alert_severity::critical);
	assert(event.health.last_error_code == 1146U);
	assert((event.reason_mask & TELEMETRY_HEALTH_REASON_CIRCUIT_OPEN) != 0U);
	assert((event.reason_mask & TELEMETRY_HEALTH_REASON_PERMANENT_FAILURE) != 0U);
	char kinds[64]{};
	telemetry_health_record_kind_mask_format(event.health.last_failure_record_kind_mask, kinds,
						 sizeof(kinds));
	assert(std::strcmp(kinds, "progression") == 0);

	event = telemetry_health_monitor_evaluate(&monitor, &config, health, MINUTE + 2U * SECOND);
	assert(event.kind == telemetry_health_event_kind::none);
	health.last_error_code = 1044U;
	event = telemetry_health_monitor_evaluate(&monitor, &config, health, MINUTE + 3U * SECOND);
	assert(event.kind == telemetry_health_event_kind::alert);
	assert(event.health.last_error_code == 1044U);
	health.state = telemetry_health_state::healthy;
	event = telemetry_health_monitor_evaluate(&monitor, &config, health, MINUTE + 4U * SECOND);
	assert(event.kind == telemetry_health_event_kind::alert);
	assert((event.reason_mask & TELEMETRY_HEALTH_REASON_RECOVERY_PENDING) != 0U);
	health.inflight_active = 0U;
	health.applied_records = 3U;
	health.last_committed_record_seq = 22U;
	health.last_success_monotonic_usec = MINUTE + 5U * SECOND;
	event = telemetry_health_monitor_evaluate(&monitor, &config, health, MINUTE + 5U * SECOND);
	assert(event.kind == telemetry_health_event_kind::recovery);
	event = telemetry_health_monitor_evaluate(&monitor, &config, health, MINUTE + 6U * SECOND);
	assert(event.kind == telemetry_health_event_kind::none);
}

void test_pressure_and_control_drop_are_critical()
{
	telemetry_health_monitor_state monitor{};
	const auto config = telemetry_health_monitor_default_config(MINUTE, 100U);
	auto health = healthy_snapshot();
	(void)telemetry_health_monitor_evaluate(&monitor, &config, health, MINUTE);
	health.queue_depth = 80U;
	auto event = telemetry_health_monitor_evaluate(&monitor, &config, health, MINUTE + SECOND);
	assert(event.kind == telemetry_health_event_kind::alert);
	assert(event.severity == telemetry_health_alert_severity::critical);
	assert((event.reason_mask & TELEMETRY_HEALTH_REASON_QUEUE_PRESSURE) != 0U);

	telemetry_health_monitor_reset(&monitor);
	health = healthy_snapshot();
	(void)telemetry_health_monitor_evaluate(&monitor, &config, health, MINUTE);
	health.dropped_control = 1U;
	event = telemetry_health_monitor_evaluate(&monitor, &config, health, MINUTE + SECOND);
	assert(event.kind == telemetry_health_event_kind::alert);
	assert(event.severity == telemetry_health_alert_severity::critical);
	assert((event.reason_mask & TELEMETRY_HEALTH_REASON_CONTROL_DROP) != 0U);
	health.last_success_monotonic_usec = MINUTE + 2U * SECOND;
	event = telemetry_health_monitor_evaluate(&monitor, &config, health, MINUTE + 2U * SECOND);
	assert(event.kind == telemetry_health_event_kind::recovery);
	event = telemetry_health_monitor_evaluate(&monitor, &config, health, MINUTE + 3U * SECOND);
	assert(event.kind == telemetry_health_event_kind::none);
}

void test_connection_state_alone_does_not_claim_recovery()
{
	telemetry_health_monitor_state monitor{};
	const auto config = telemetry_health_monitor_default_config(MINUTE, 100U);
	auto health = healthy_snapshot();
	(void)telemetry_health_monitor_evaluate(&monitor, &config, health, MINUTE);
	health.state = telemetry_health_state::circuit_open;
	health.last_failure_class = telemetry_failure_class::permanent_permission;
	health.last_error_code = 1044U;
	health.last_failure_monotonic_usec = MINUTE + SECOND;
	auto event = telemetry_health_monitor_evaluate(&monitor, &config, health, MINUTE + SECOND);
	assert(event.kind == telemetry_health_event_kind::alert);
	assert(event.severity == telemetry_health_alert_severity::critical);

	health.state = telemetry_health_state::healthy;
	event = telemetry_health_monitor_evaluate(&monitor, &config, health, MINUTE + 2U * SECOND);
	assert(event.kind == telemetry_health_event_kind::alert);
	assert(event.severity == telemetry_health_alert_severity::critical);
	assert(event.reason_mask == TELEMETRY_HEALTH_REASON_RECOVERY_PENDING);
	event = telemetry_health_monitor_evaluate(&monitor, &config, health, MINUTE + 3U * SECOND);
	assert(event.kind == telemetry_health_event_kind::none);

	health.last_success_monotonic_usec = MINUTE + 4U * SECOND;
	event = telemetry_health_monitor_evaluate(&monitor, &config, health, MINUTE + 4U * SECOND);
	assert(event.kind == telemetry_health_event_kind::recovery);
	event = telemetry_health_monitor_evaluate(&monitor, &config, health, MINUTE + 5U * SECOND);
	assert(event.kind == telemetry_health_event_kind::none);
}

} // namespace

int main()
{
	test_idle_does_not_stale();
	test_warning_critical_rate_limit_and_recovery();
	test_permanent_failure_is_immediate_and_redacted();
	test_pressure_and_control_drop_are_critical();
	test_connection_state_alone_does_not_claim_recovery();
	std::cout << "telemetry health warning/critical/rate-limit/recovery passed\n";
	return 0;
}
