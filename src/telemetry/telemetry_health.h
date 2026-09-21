#ifndef DURIS_TELEMETRY_HEALTH_H
#define DURIS_TELEMETRY_HEALTH_H

#include "telemetry/telemetry_types.h"

#include <cstddef>
#include <cstdint>
#include <type_traits>

inline constexpr telemetry_duration_usec TELEMETRY_HEALTH_CRITICAL_AFTER_USEC = 300'000'000U;
inline constexpr telemetry_duration_usec TELEMETRY_HEALTH_REPEAT_AFTER_USEC = 300'000'000U;
inline constexpr std::uint8_t TELEMETRY_HEALTH_QUEUE_PRESSURE_PERCENT = 80U;

enum class telemetry_health_alert_severity : std::uint8_t
{
	none = 0,
	warning = 1,
	critical = 2,
};

enum class telemetry_health_event_kind : std::uint8_t
{
	none = 0,
	state = 1,
	alert = 2,
	reminder = 3,
	recovery = 4,
};

inline constexpr std::uint32_t TELEMETRY_HEALTH_REASON_WRITER_DEGRADED = 1U << 0U;
inline constexpr std::uint32_t TELEMETRY_HEALTH_REASON_STALLED = 1U << 1U;
inline constexpr std::uint32_t TELEMETRY_HEALTH_REASON_CIRCUIT_OPEN = 1U << 2U;
inline constexpr std::uint32_t TELEMETRY_HEALTH_REASON_QUEUE_PRESSURE = 1U << 3U;
inline constexpr std::uint32_t TELEMETRY_HEALTH_REASON_CONTROL_DROP = 1U << 4U;
inline constexpr std::uint32_t TELEMETRY_HEALTH_REASON_PERMANENT_FAILURE = 1U << 5U;
inline constexpr std::uint32_t TELEMETRY_HEALTH_REASON_RECOVERY_PENDING = 1U << 6U;

struct telemetry_health_monitor_config
{
	telemetry_duration_usec interval_usec;
	telemetry_duration_usec critical_after_usec;
	telemetry_duration_usec repeat_after_usec;
	std::uint32_t queue_capacity;
	std::uint8_t queue_pressure_percent;
	std::uint8_t reserved[3];
};

struct telemetry_health_failure_signature
{
	telemetry_failure_class failure_class;
	std::uint8_t reserved[3];
	std::uint32_t error_code;
	telemetry_producer_id producer;
	telemetry_record_sequence first_record_seq;
	telemetry_record_sequence last_record_seq;
	std::uint64_t record_kind_mask;
};

struct telemetry_health_monitor_state
{
	std::uint8_t initialized;
	std::uint8_t control_drop_pending;
	telemetry_health_alert_severity active_severity;
	telemetry_health_state last_state;
	std::uint32_t active_reason_mask;
	telemetry_monotonic_usec pending_since_usec;
	telemetry_monotonic_usec alert_started_usec;
	telemetry_monotonic_usec last_emitted_usec;
	telemetry_monotonic_usec control_drop_success_marker;
	std::uint64_t last_control_drop_count;
	telemetry_health_failure_signature last_failure;
	telemetry_producer_id affected_producer;
	telemetry_record_sequence affected_first_record_seq;
	telemetry_record_sequence affected_last_record_seq;
};

struct telemetry_health_event
{
	telemetry_health_event_kind kind;
	telemetry_health_alert_severity severity;
	telemetry_health_state previous_state;
	telemetry_health_state current_state;
	std::uint32_t reason_mask;
	std::uint8_t last_commit_age_available;
	std::uint8_t last_failure_age_available;
	std::uint8_t reserved[2];
	telemetry_duration_usec last_commit_age_usec;
	telemetry_duration_usec last_failure_age_usec;
	telemetry_duration_usec alert_duration_usec;
	telemetry_producer_id affected_producer;
	telemetry_record_sequence affected_first_record_seq;
	telemetry_record_sequence affected_last_record_seq;
	telemetry_health_snapshot health;
};

struct telemetry_health_status
{
	telemetry_health_snapshot health;
	telemetry_health_alert_severity active_severity;
	std::uint8_t last_commit_age_available;
	std::uint8_t last_failure_age_available;
	std::uint8_t reserved;
	std::uint32_t active_reason_mask;
	telemetry_duration_usec configured_interval_usec;
	telemetry_duration_usec last_commit_age_usec;
	telemetry_duration_usec last_failure_age_usec;
	telemetry_duration_usec active_alert_duration_usec;
};

telemetry_health_monitor_config
telemetry_health_monitor_default_config(telemetry_duration_usec interval_usec,
					std::uint32_t queue_capacity) noexcept;
void telemetry_health_monitor_reset(telemetry_health_monitor_state *state) noexcept;
telemetry_health_event telemetry_health_monitor_evaluate(
	telemetry_health_monitor_state *state, const telemetry_health_monitor_config *config,
	telemetry_health_snapshot health, telemetry_monotonic_usec now) noexcept;
telemetry_health_status telemetry_health_monitor_status_copy(
	const telemetry_health_monitor_state *state, const telemetry_health_monitor_config *config,
	telemetry_health_snapshot health, telemetry_monotonic_usec now) noexcept;

const char *telemetry_health_state_name(telemetry_health_state state) noexcept;
const char *telemetry_health_backend_name(telemetry_storage_backend backend) noexcept;
const char *telemetry_health_failure_class_name(telemetry_failure_class failure) noexcept;
const char *telemetry_health_advisory_lock_name(telemetry_advisory_lock_state state) noexcept;
const char *telemetry_health_alert_severity_name(telemetry_health_alert_severity severity) noexcept;
const char *telemetry_health_event_kind_name(telemetry_health_event_kind kind) noexcept;
std::size_t telemetry_health_record_kind_mask_format(std::uint64_t mask, char *output,
						     std::size_t capacity) noexcept;
std::size_t telemetry_health_reason_mask_format(std::uint32_t mask, char *output,
						std::size_t capacity) noexcept;

static_assert(std::is_trivially_copyable_v<telemetry_health_monitor_config>);
static_assert(std::is_trivially_copyable_v<telemetry_health_monitor_state>);
static_assert(std::is_trivially_copyable_v<telemetry_health_event>);
static_assert(std::is_trivially_copyable_v<telemetry_health_status>);

#endif
