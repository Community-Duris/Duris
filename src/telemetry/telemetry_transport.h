#ifndef DURIS_TELEMETRY_TRANSPORT_H
#define DURIS_TELEMETRY_TRANSPORT_H

#include "telemetry/telemetry_types.h"

#include <cstddef>
#include <cstdint>
#include <type_traits>

/*
 * A transport has one fixed-capacity RAM queue and one writer owner.  Init and
 * shutdown are controlled by the lifetime owner; enqueue is a bounded copy
 * and never waits for SQL, allocation, or a worker.  Pulse/drain are
 * worker-only.  request_stop is nonblocking, and shutdown follows worker join
 * rather than performing an unbounded game-thread join.
 * #265's process-lifetime coordinator owns the actual thread handle, starts one
 * worker after queue init, and joins it OFF the game thread. Transport init does
 * not spawn a hidden thread. The worker initializes the repository and calls
 * pulse/drain; it reports stopped before exiting. The coordinator retains all
 * state until join completes, then shuts down repository and transport. Health
 * stopped is a signal to the coordinator, not permission to free before join.
 */
enum class telemetry_transport_outcome : std::uint8_t
{
	started = 0,
	already_started = 1,
	invalid_config = 2,
	flatfile_disabled = 3,
	unavailable = 4,
	stopping = 5,
	drained = 6,
	deadline_reached = 7,
};

struct telemetry_transport_config
{
	telemetry_storage_backend backend;
	std::uint8_t reserved;
	std::uint16_t schema_version;
	std::uint32_t queue_capacity;
	std::uint32_t control_reserve;
	std::uint16_t max_batch_records;
	std::uint16_t reserved2;
	std::uint32_t max_batch_bytes;
	telemetry_duration_usec flush_oldest_after_usec;
};

/* Returned by enqueue; accepted means retained in bounded RAM only. */
struct telemetry_enqueue_result
{
	telemetry_queue_admission admission;
	std::uint8_t reserved[3];
	telemetry_record_key key;
	std::uint32_t queue_depth;
	std::uint32_t control_depth;
	std::uint64_t dropped_detail_total;
	std::uint64_t dropped_control_total;
};

struct telemetry_transport_pulse_result
{
	telemetry_transport_outcome outcome;
	std::uint8_t reserved[3];
	std::uint32_t examined;
	std::uint32_t applied;
	std::uint32_t duplicate_or_stale;
	std::uint32_t rejected;
	std::uint32_t pending;
};

struct telemetry_transport_drain_result
{
	telemetry_transport_outcome outcome;
	std::uint8_t final_flush_attempted;
	std::uint16_t reserved;
	std::uint32_t pending;
};

/*
 * The capacity is fixed at init and never grows or overwrites a worker-owned
 * record.  The control reserve is finite: lifecycle/checkpoint/gap/configuration
 * records can use it, but saturation still produces an explicit rejection.
 */
constexpr bool
telemetry_transport_config_is_bounded(const telemetry_transport_config &config) noexcept
{
	return config.schema_version == TELEMETRY_SCHEMA_VERSION && config.reserved == 0U &&
	       config.reserved2 == 0U && telemetry_storage_backend_is_valid(config.backend) &&
	       config.queue_capacity > 0U &&
	       config.queue_capacity <= TELEMETRY_QUEUE_CAPACITY_PROPOSAL &&
	       config.control_reserve > 0U && config.control_reserve < config.queue_capacity &&
	       config.control_reserve <= TELEMETRY_CONTROL_RESERVE_PROPOSAL &&
	       config.max_batch_records > 0U &&
	       config.max_batch_records <= TELEMETRY_BATCH_MAX_RECORDS_PROPOSAL &&
	       config.max_batch_bytes >= sizeof(telemetry_record) &&
	       config.max_batch_bytes <= TELEMETRY_BATCH_MAX_BYTES_PROPOSAL &&
	       config.flush_oldest_after_usec > 0U &&
	       config.flush_oldest_after_usec <= TELEMETRY_FLUSH_OLDEST_USEC_MAX_PROPOSAL;
}

telemetry_transport_outcome telemetry_transport_init(telemetry_transport_config config);

/* record is copied before this call returns; the caller retains no ownership. */
telemetry_enqueue_result telemetry_transport_enqueue(telemetry_record record);

/* Pulse/drain perform bounded worker-owned writer work; they never run on the game thread. */
telemetry_transport_pulse_result
telemetry_transport_pulse(telemetry_monotonic_usec now_monotonic_usec);
telemetry_transport_drain_result
telemetry_transport_drain_until(telemetry_monotonic_usec deadline_monotonic_usec);
telemetry_transport_outcome telemetry_transport_request_stop(void);
/* Called by the lifetime owner only after the transport worker has joined. */
void telemetry_transport_shutdown(void);
/* Returns a synchronized cached copy; it performs no SQL or live inspection. */
telemetry_health_snapshot telemetry_transport_health_copy(void);

static_assert(std::is_trivially_copyable_v<telemetry_transport_config>);
static_assert(std::is_standard_layout_v<telemetry_transport_config>);
static_assert(std::is_trivially_copyable_v<telemetry_enqueue_result>);

#endif
