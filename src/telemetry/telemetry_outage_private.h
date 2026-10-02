#ifndef DURIS_TELEMETRY_OUTAGE_PRIVATE_H
#define DURIS_TELEMETRY_OUTAGE_PRIVATE_H

#include "telemetry/telemetry_types.h"

#include <array>
#include <cstddef>
#include <cstdint>

/* Worker-owned evidence, not a payload spool. Every counter is a sampled
 * watermark; an unknown tail never implies an exact loss count or crash time. */
inline constexpr std::size_t TELEMETRY_OUTAGE_MAX_PRODUCERS = 256U;
inline constexpr std::size_t TELEMETRY_OUTAGE_DISK_WORDS = 40U;
inline constexpr std::size_t TELEMETRY_OUTAGE_MAX_BYTES =
	64U + TELEMETRY_OUTAGE_MAX_PRODUCERS * TELEMETRY_OUTAGE_DISK_WORDS * 8U;

enum class telemetry_outage_phase : std::uint8_t
{
	running = 1,
	clean_drained = 2,
	abandoned = 3,
	unknown_tail = 4,
};

enum class telemetry_outage_result : std::uint8_t
{
	ready,
	invalid,
	unsafe_storage,
	owned_elsewhere,
	corrupt,
	quota,
	io_failure,
};

struct telemetry_outage_observation
{
	telemetry_producer_id producer{};
	telemetry_id environment_id = 0U;
	telemetry_id season_id = 0U;
	telemetry_monotonic_usec registered_monotonic_usec = 0U;
	telemetry_utc_usec registered_utc_usec = TELEMETRY_UTC_UNKNOWN;
	telemetry_monotonic_usec observed_monotonic_usec = 0U;
	telemetry_utc_usec observed_utc_usec = TELEMETRY_UTC_UNKNOWN;
	telemetry_outage_phase phase = telemetry_outage_phase::running;
	std::uint64_t record_kind_mask = 0U;
	telemetry_health_snapshot health{};
	std::uint32_t inflight_records = 0U;
	std::uint32_t unattempted_records = 0U;
};

struct telemetry_outage_journal
{
	int directory_fd = -1;
	int ownership_fd = -1;
	std::uint64_t generation = 0U;
	std::size_t count = 0U;
	std::size_t current = TELEMETRY_OUTAGE_MAX_PRODUCERS;
	std::uint32_t error_code = 0U;
	std::array<telemetry_outage_observation, TELEMETRY_OUTAGE_MAX_PRODUCERS> observations{};
};

/* All operations belong to the designated worker. A successful open persists
 * registration before transport admission is possible. read acquires the same
 * exclusive lock and is intended for bounded, offline evidence export. */
telemetry_outage_result
telemetry_outage_open(telemetry_outage_journal *journal, const char *directory,
		      const telemetry_outage_observation &registration) noexcept;
telemetry_outage_result telemetry_outage_read(telemetry_outage_journal *journal,
					      const char *directory) noexcept;
telemetry_outage_result
telemetry_outage_checkpoint(telemetry_outage_journal *journal,
			    const telemetry_outage_observation &observation) noexcept;
void telemetry_outage_close(telemetry_outage_journal *journal) noexcept;
telemetry_outage_phase
telemetry_outage_terminal_phase(const telemetry_outage_observation &observation) noexcept;

#endif
