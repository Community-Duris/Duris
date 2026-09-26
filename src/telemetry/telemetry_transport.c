#include "telemetry/telemetry_transport_private.h"
#include "telemetry/telemetry_queue_private.h"
#include "telemetry/telemetry_failure.h"

#include <array>
#include <atomic>
#include <chrono>
#include <cerrno>
#include <cstddef>
#include <cstdint>
#include <limits>

namespace
{

using telemetry_queue_private::queue;

constexpr std::uint8_t LIFECYCLE_UNINITIALIZED = 0U;
constexpr std::uint8_t LIFECYCLE_STARTING = 1U;
constexpr std::uint8_t LIFECYCLE_ACTIVE = 2U;
constexpr std::uint8_t LIFECYCLE_STOPPING = 3U;
constexpr std::uint8_t LIFECYCLE_DISABLED = 4U;
constexpr std::uint8_t LIFECYCLE_STOPPED = 5U;
constexpr std::uint64_t ADMISSION_CLOSED = std::uint64_t{ 1U } << 63U;
constexpr std::uint64_t ADMISSION_FINALIZING = std::uint64_t{ 1U } << 62U;
constexpr std::uint64_t ADMISSION_COUNT_MASK = ~(ADMISSION_CLOSED | ADMISSION_FINALIZING);

struct inflight_state
{
	bool active = false;
	bool resolution_required = false;
	bool isolation = false;
	std::size_t count = 0U;
	std::size_t control_count = 0U;
	std::size_t isolation_index = 0U;
	telemetry_monotonic_usec oldest_admitted_at = 0U;
	bool oldest_admitted_at_valid = false;
	std::uint32_t retry_attempts = 0U;
	telemetry_monotonic_usec retry_not_before = 0U;
};

queue QUEUE{};
std::array<telemetry_record, TELEMETRY_BATCH_MAX_RECORDS_PROPOSAL> INFLIGHT_RECORDS{};
inflight_state INFLIGHT{};
telemetry_transport_config SETTINGS{};
telemetry_repository_config REPOSITORY_SETTINGS{};
// Producer-only admission watermark; worker retries are a separate immutable path.
telemetry_record_key LAST_ADMITTED_KEY{};
telemetry_transport_loss_snapshot LOSS{};
bool LOSS_RANGE_UNKNOWN = false;
static_assert(std::atomic<std::uint64_t>::is_always_lock_free);
static_assert(std::atomic<std::uint32_t>::is_always_lock_free);
static_assert(std::atomic<std::uint8_t>::is_always_lock_free);

std::atomic<std::uint8_t> LIFECYCLE{ LIFECYCLE_UNINITIALIZED };
std::atomic<bool> STOP_REQUESTED{ false };
std::atomic<bool> QUIESCED{ false };
std::atomic<bool> RESTART_ALLOWED{ false };
std::atomic<bool> SHUTDOWN_REQUESTED{ false };
std::atomic<std::uint64_t> ADMISSION_STATE{ 0U };
std::atomic<bool> REPOSITORY_READY{ false };
std::atomic<bool> CIRCUIT_OPEN{ false };
/* Set only after init reports an owned writer.  DB-down/disabled paths must
 * not synchronously call a borrowed shutdown callback that has no live writer. */
std::atomic<bool> REPOSITORY_STARTED{ false };
std::uint32_t REPOSITORY_RETRY_ATTEMPTS = 0U;
telemetry_monotonic_usec REPOSITORY_RETRY_NOT_BEFORE = 0U;

struct transport_health_atoms
{
	std::atomic<std::uint8_t> state{ static_cast<std::uint8_t>(
		telemetry_health_state::disabled) };
	std::atomic<std::uint8_t> backend{ 0U };
	std::atomic<std::uint8_t> disabled_reason{ static_cast<std::uint8_t>(
		telemetry_disabled_reason::not_initialized) };
	std::atomic<std::uint8_t> last_failure_class{ static_cast<std::uint8_t>(
		telemetry_failure_class::none) };
	std::atomic<std::uint32_t> last_error_code{ 0U };
	std::atomic<std::uint32_t> queue_capacity{ 0U };
	std::atomic<std::uint64_t> producer_boot_id{ 0U };
	std::atomic<std::uint64_t> producer_process_id{ 0U };
	std::atomic<std::uint64_t> last_admitted_record_seq{ 0U };
	std::atomic<std::uint64_t> last_committed_record_seq{ 0U };
	std::atomic<std::uint64_t> inflight_first_record_seq{ 0U };
	std::atomic<std::uint64_t> inflight_last_record_seq{ 0U };
	std::atomic<std::uint64_t> inflight_record_kind_mask{ 0U };
	std::atomic<std::uint64_t> inflight_retry_not_before{ 0U };
	std::atomic<std::uint64_t> repository_retry_not_before{ 0U };
	std::atomic<std::uint32_t> inflight_retry_attempts{ 0U };
	std::atomic<std::uint32_t> repository_retry_attempts{ 0U };
	std::atomic<std::uint8_t> inflight_active{ 0U };
	std::atomic<std::uint64_t> queue_high_water{ 0U };
	std::atomic<std::uint64_t> admitted_detail{ 0U };
	std::atomic<std::uint64_t> admitted_control{ 0U };
	std::atomic<std::uint64_t> dropped_detail{ 0U };
	std::atomic<std::uint64_t> dropped_control{ 0U };
	std::atomic<std::uint64_t> applied_records{ 0U };
	std::atomic<std::uint64_t> duplicate_records{ 0U };
	std::atomic<std::uint64_t> stale_checkpoint_records{ 0U };
	std::atomic<std::uint64_t> invalid_records{ 0U };
	std::atomic<std::uint64_t> conflict_records{ 0U };
	std::atomic<std::uint64_t> retryable_failures{ 0U };
	std::atomic<std::uint64_t> ambiguous_commits{ 0U };
	std::atomic<std::uint64_t> quarantined_records{ 0U };
	std::atomic<std::uint64_t> circuit_open_count{ 0U };
	std::atomic<std::uint64_t> last_failure_boot_id{ 0U };
	std::atomic<std::uint64_t> last_failure_process_id{ 0U };
	std::atomic<std::uint64_t> last_failure_first_record_seq{ 0U };
	std::atomic<std::uint64_t> last_failure_last_record_seq{ 0U };
	std::atomic<std::uint64_t> last_failure_record_kind_mask{ 0U };
	std::atomic<std::uint32_t> last_failure_retry_attempts{ 0U };
	std::atomic<std::uint64_t> sequence_gap_count{ 0U };
	std::atomic<std::uint64_t> unclosed_tail_count{ 0U };
	std::atomic<std::uint64_t> attributable_duration_usec{ 0U };
	std::atomic<std::uint64_t> unknown_duration_usec{ 0U };
	std::atomic<std::uint64_t> last_success_monotonic_usec{ 0U };
	std::atomic<std::uint64_t> last_failure_monotonic_usec{ 0U };
};

transport_health_atoms HEALTH{};

telemetry_repository_outcome production_repository_init(void *,
							telemetry_repository_config config) noexcept
{
	try
	{
		return telemetry_repository_init(config);
	}
	catch (...)
	{
		return telemetry_repository_outcome::unavailable;
	}
}

telemetry_apply_batch_result production_repository_apply(void *, const telemetry_record *records,
							 std::size_t count) noexcept
{
	try
	{
		return telemetry_repository_apply(records, count);
	}
	catch (...)
	{
		telemetry_apply_batch_result result{};
		result.outcome = telemetry_batch_outcome::unavailable;
		return result;
	}
}

telemetry_repository_outcome production_repository_request_stop(void *) noexcept
{
	try
	{
		return telemetry_repository_request_stop();
	}
	catch (...)
	{
		return telemetry_repository_outcome::stopping;
	}
}

void production_repository_shutdown(void *) noexcept
{
	try
	{
		telemetry_repository_shutdown();
	}
	catch (...)
	{
	}
}

bool production_clock_now(void *, telemetry_monotonic_usec *value) noexcept
{
	if (value == nullptr)
		return false;
	*value = static_cast<telemetry_monotonic_usec>(
		std::chrono::duration_cast<std::chrono::microseconds>(
			std::chrono::steady_clock::now().time_since_epoch())
			.count());
	return true;
}

telemetry_transport_repository_binding DEFAULT_REPOSITORY = {
	production_repository_init,
	production_repository_apply,
	production_repository_request_stop,
	production_repository_shutdown,
	nullptr,
};
telemetry_transport_clock_binding DEFAULT_CLOCK = { production_clock_now, nullptr };
telemetry_transport_repository_binding REPOSITORY = DEFAULT_REPOSITORY;
telemetry_transport_clock_binding CLOCK = DEFAULT_CLOCK;

bool key_equal(const telemetry_record_key &left, const telemetry_record_key &right) noexcept
{
	return left.producer.boot_id == right.producer.boot_id &&
	       left.producer.process_id == right.producer.process_id &&
	       left.record_seq == right.record_seq;
}

bool apply_result_reserved_is_zero(const telemetry_apply_batch_result &result) noexcept
{
	return result.reserved[0] == 0U && result.reserved[1] == 0U;
}

bool record_apply_reserved_is_zero(const telemetry_record_apply_result &result) noexcept
{
	return result.reserved[0] == 0U && result.reserved[1] == 0U;
}

bool record_apply_unused_is_zero(const telemetry_record_apply_result &result) noexcept
{
	return record_apply_reserved_is_zero(result) && result.key.producer.boot_id == 0U &&
	       result.key.producer.process_id == 0U && result.key.record_seq == 0U &&
	       result.outcome == telemetry_apply_outcome::applied &&
	       result.failure_class == telemetry_failure_class::none && result.error_code == 0U;
}

template <typename T> void saturating_add(std::atomic<T> &value, T amount) noexcept
{
	T current = value.load(std::memory_order_relaxed);
	for (;;)
	{
		const T available = std::numeric_limits<T>::max() - current;
		const T desired = amount > available ? std::numeric_limits<T>::max() :
						       current + amount;
		if (value.compare_exchange_strong(current, desired, std::memory_order_relaxed,
						  std::memory_order_relaxed))
			return;
	}
}

telemetry_monotonic_usec saturating_time_add(telemetry_monotonic_usec value,
					     telemetry_duration_usec amount) noexcept
{
	const telemetry_monotonic_usec max = std::numeric_limits<telemetry_monotonic_usec>::max();
	return amount > max - value ? max : value + amount;
}

telemetry_monotonic_usec retry_backoff(std::uint32_t attempts) noexcept
{
	telemetry_monotonic_usec backoff = TELEMETRY_TRANSPORT_RETRY_BACKOFF_INITIAL_USEC;
	for (std::uint32_t index = 1U;
	     index < attempts && backoff < TELEMETRY_TRANSPORT_RETRY_BACKOFF_MAX_USEC; ++index)
	{
		if (backoff > TELEMETRY_TRANSPORT_RETRY_BACKOFF_MAX_USEC / 2U)
			backoff = TELEMETRY_TRANSPORT_RETRY_BACKOFF_MAX_USEC;
		else
			backoff *= 2U;
	}
	return backoff;
}

bool clock_now(telemetry_monotonic_usec &value) noexcept
{
	if (CLOCK.now == nullptr)
		return false;
	return CLOCK.now(CLOCK.context, &value);
}

void set_health_state(telemetry_health_state state) noexcept
{
	HEALTH.state.store(static_cast<std::uint8_t>(state), std::memory_order_release);
}

telemetry_health_state health_state() noexcept
{
	const auto cached =
		static_cast<telemetry_health_state>(HEALTH.state.load(std::memory_order_acquire));
	return STOP_REQUESTED.load(std::memory_order_acquire) &&
			       cached != telemetry_health_state::stopped ?
		       telemetry_health_state::stopping :
		       cached;
}

void update_queue_high_water(std::uint32_t depth_value) noexcept
{
	std::uint64_t current = HEALTH.queue_high_water.load(std::memory_order_relaxed);
	while (current < depth_value &&
	       !HEALTH.queue_high_water.compare_exchange_strong(
		       current, depth_value, std::memory_order_relaxed, std::memory_order_relaxed))
	{
	}
}

void reset_health(const telemetry_transport_config &config) noexcept
{
	HEALTH.state.store(static_cast<std::uint8_t>(telemetry_health_state::starting),
			   std::memory_order_relaxed);
	HEALTH.backend.store(static_cast<std::uint8_t>(config.backend), std::memory_order_relaxed);
	HEALTH.disabled_reason.store(static_cast<std::uint8_t>(telemetry_disabled_reason::none),
				     std::memory_order_relaxed);
	HEALTH.last_failure_class.store(static_cast<std::uint8_t>(telemetry_failure_class::none),
					std::memory_order_relaxed);
	HEALTH.last_error_code.store(0U, std::memory_order_relaxed);
	HEALTH.queue_capacity.store(config.queue_capacity, std::memory_order_relaxed);
	HEALTH.producer_boot_id.store(config.fresh_producer.boot_id, std::memory_order_relaxed);
	HEALTH.producer_process_id.store(config.fresh_producer.process_id,
					 std::memory_order_relaxed);
	HEALTH.last_admitted_record_seq.store(0U, std::memory_order_relaxed);
	HEALTH.last_committed_record_seq.store(0U, std::memory_order_relaxed);
	HEALTH.inflight_first_record_seq.store(0U, std::memory_order_relaxed);
	HEALTH.inflight_last_record_seq.store(0U, std::memory_order_relaxed);
	HEALTH.inflight_record_kind_mask.store(0U, std::memory_order_relaxed);
	HEALTH.inflight_retry_not_before.store(0U, std::memory_order_relaxed);
	HEALTH.repository_retry_not_before.store(0U, std::memory_order_relaxed);
	HEALTH.inflight_retry_attempts.store(0U, std::memory_order_relaxed);
	HEALTH.repository_retry_attempts.store(0U, std::memory_order_relaxed);
	HEALTH.inflight_active.store(0U, std::memory_order_relaxed);
	HEALTH.queue_high_water.store(0U, std::memory_order_relaxed);
	HEALTH.admitted_detail.store(0U, std::memory_order_relaxed);
	HEALTH.admitted_control.store(0U, std::memory_order_relaxed);
	HEALTH.dropped_detail.store(0U, std::memory_order_relaxed);
	HEALTH.dropped_control.store(0U, std::memory_order_relaxed);
	HEALTH.applied_records.store(0U, std::memory_order_relaxed);
	HEALTH.duplicate_records.store(0U, std::memory_order_relaxed);
	HEALTH.stale_checkpoint_records.store(0U, std::memory_order_relaxed);
	HEALTH.invalid_records.store(0U, std::memory_order_relaxed);
	HEALTH.conflict_records.store(0U, std::memory_order_relaxed);
	HEALTH.retryable_failures.store(0U, std::memory_order_relaxed);
	HEALTH.ambiguous_commits.store(0U, std::memory_order_relaxed);
	HEALTH.quarantined_records.store(0U, std::memory_order_relaxed);
	HEALTH.circuit_open_count.store(0U, std::memory_order_relaxed);
	HEALTH.last_failure_boot_id.store(0U, std::memory_order_relaxed);
	HEALTH.last_failure_process_id.store(0U, std::memory_order_relaxed);
	HEALTH.last_failure_first_record_seq.store(0U, std::memory_order_relaxed);
	HEALTH.last_failure_last_record_seq.store(0U, std::memory_order_relaxed);
	HEALTH.last_failure_record_kind_mask.store(0U, std::memory_order_relaxed);
	HEALTH.last_failure_retry_attempts.store(0U, std::memory_order_relaxed);
	HEALTH.sequence_gap_count.store(0U, std::memory_order_relaxed);
	HEALTH.unclosed_tail_count.store(0U, std::memory_order_relaxed);
	HEALTH.attributable_duration_usec.store(0U, std::memory_order_relaxed);
	HEALTH.unknown_duration_usec.store(0U, std::memory_order_relaxed);
	HEALTH.last_success_monotonic_usec.store(0U, std::memory_order_relaxed);
	HEALTH.last_failure_monotonic_usec.store(0U, std::memory_order_relaxed);
}

void clear_inflight_health() noexcept
{
	HEALTH.inflight_active.store(0U, std::memory_order_release);
	HEALTH.inflight_first_record_seq.store(0U, std::memory_order_release);
	HEALTH.inflight_last_record_seq.store(0U, std::memory_order_release);
	HEALTH.inflight_record_kind_mask.store(0U, std::memory_order_release);
	HEALTH.inflight_retry_attempts.store(0U, std::memory_order_release);
	HEALTH.inflight_retry_not_before.store(0U, std::memory_order_release);
}

void publish_inflight_health() noexcept
{
	if (!INFLIGHT.active || INFLIGHT.count == 0U)
	{
		clear_inflight_health();
		return;
	}
	std::uint64_t kinds = 0U;
	for (std::size_t index = 0U; index < INFLIGHT.count; ++index)
		kinds |= std::uint64_t{ 1U }
			 << static_cast<std::uint8_t>(INFLIGHT_RECORDS[index].header.kind);
	HEALTH.inflight_first_record_seq.store(INFLIGHT_RECORDS[0].header.key.record_seq,
					       std::memory_order_release);
	HEALTH.inflight_last_record_seq.store(
		INFLIGHT_RECORDS[INFLIGHT.count - 1U].header.key.record_seq,
		std::memory_order_release);
	HEALTH.inflight_record_kind_mask.store(kinds, std::memory_order_release);
	HEALTH.inflight_retry_attempts.store(INFLIGHT.retry_attempts, std::memory_order_release);
	HEALTH.inflight_retry_not_before.store(INFLIGHT.retry_not_before,
					       std::memory_order_release);
	HEALTH.inflight_active.store(1U, std::memory_order_release);
}

void reset_inflight() noexcept
{
	INFLIGHT = {};
	clear_inflight_health();
}

void add_drop_health(bool control) noexcept
{
	if (control)
		saturating_add(HEALTH.dropped_control, std::uint64_t{ 1U });
	else
		saturating_add(HEALTH.dropped_detail, std::uint64_t{ 1U });
}

void record_failed_admission(const telemetry_record &record, bool control) noexcept
{
	add_drop_health(control);
	auto &count = control ? LOSS.rejected_control : LOSS.rejected_detail;
	if (count != std::numeric_limits<std::uint64_t>::max())
		++count;
	const auto seq = record.header.key.record_seq;
	if (!LOSS_RANGE_UNKNOWN)
	{
		if (LOSS.first_rejected_record_seq == 0U)
			LOSS.first_rejected_record_seq = seq;
		else if (LOSS.last_rejected_record_seq ==
				 std::numeric_limits<std::uint64_t>::max() ||
			 seq != LOSS.last_rejected_record_seq + 1U)
			LOSS_RANGE_UNKNOWN = true;
		LOSS.last_rejected_record_seq = seq;
	}
	if (LOSS_RANGE_UNKNOWN)
		LOSS.first_rejected_record_seq = LOSS.last_rejected_record_seq = 0U;
}

std::size_t pending_count() noexcept
{
	const auto depth = telemetry_queue_private::depth(&QUEUE);
	// A producer may own admission but not yet have published its slot.
	return depth == 0U && (ADMISSION_STATE.load(std::memory_order_seq_cst) &
			       ADMISSION_COUNT_MASK) != 0U ?
		       1U :
		       depth;
}

void fill_pending(telemetry_transport_pulse_result &result) noexcept
{
	const std::size_t pending = pending_count();
	result.pending = pending > std::numeric_limits<std::uint32_t>::max() ?
				 std::numeric_limits<std::uint32_t>::max() :
				 static_cast<std::uint32_t>(pending);
}

void update_admission_result(telemetry_enqueue_result &result) noexcept
{
	result.queue_depth = telemetry_queue_private::depth(&QUEUE);
	result.control_depth = telemetry_queue_private::control_depth(&QUEUE);
	result.dropped_detail_total = HEALTH.dropped_detail.load(std::memory_order_acquire);
	result.dropped_control_total = HEALTH.dropped_control.load(std::memory_order_acquire);
}

void capture_failure_identity(const telemetry_record *records, std::size_t count,
			      telemetry_failure_class failure_class, std::uint32_t error_code,
			      std::uint32_t retry_attempts) noexcept
{
	HEALTH.last_failure_class.store(static_cast<std::uint8_t>(failure_class),
					std::memory_order_release);
	HEALTH.last_error_code.store(error_code, std::memory_order_release);
	HEALTH.last_failure_retry_attempts.store(retry_attempts, std::memory_order_release);
	if (records == nullptr || count == 0U)
	{
		HEALTH.last_failure_boot_id.store(0U, std::memory_order_release);
		HEALTH.last_failure_process_id.store(0U, std::memory_order_release);
		HEALTH.last_failure_first_record_seq.store(0U, std::memory_order_release);
		HEALTH.last_failure_last_record_seq.store(0U, std::memory_order_release);
		HEALTH.last_failure_record_kind_mask.store(0U, std::memory_order_release);
		return;
	}
	HEALTH.last_failure_boot_id.store(records[0].header.key.producer.boot_id,
					  std::memory_order_release);
	HEALTH.last_failure_process_id.store(records[0].header.key.producer.process_id,
					     std::memory_order_release);
	HEALTH.last_failure_first_record_seq.store(records[0].header.key.record_seq,
						   std::memory_order_release);
	HEALTH.last_failure_last_record_seq.store(records[count - 1U].header.key.record_seq,
						  std::memory_order_release);
	std::uint64_t kinds = 0U;
	for (std::size_t index = 0U; index < count; ++index)
		kinds |= std::uint64_t{ 1U }
			 << static_cast<std::uint8_t>(records[index].header.kind);
	HEALTH.last_failure_record_kind_mask.store(kinds, std::memory_order_release);
}

void mark_failure(telemetry_monotonic_usec now, std::uint32_t error_code,
		  telemetry_failure_class failure_class =
			  telemetry_failure_class::transient_internal) noexcept
{
	HEALTH.last_error_code.store(error_code, std::memory_order_release);
	HEALTH.last_failure_class.store(static_cast<std::uint8_t>(failure_class),
					std::memory_order_release);
	HEALTH.last_failure_monotonic_usec.store(now, std::memory_order_release);
	if (!STOP_REQUESTED.load(std::memory_order_acquire))
		set_health_state(telemetry_health_state::degraded);
}

void open_circuit(telemetry_monotonic_usec now, std::uint32_t error_code,
		  telemetry_failure_class failure_class, const telemetry_record *records,
		  std::size_t count, std::uint32_t retry_attempts) noexcept
{
	capture_failure_identity(records, count, failure_class, error_code, retry_attempts);
	HEALTH.last_failure_monotonic_usec.store(now, std::memory_order_release);
	if (!CIRCUIT_OPEN.exchange(true, std::memory_order_acq_rel))
		saturating_add(HEALTH.circuit_open_count, std::uint64_t{ 1U });
	if (!STOP_REQUESTED.load(std::memory_order_acquire))
		set_health_state(telemetry_health_state::circuit_open);
}

bool schedule_inflight_retry(telemetry_monotonic_usec now, std::uint32_t error_code,
			     telemetry_failure_class failure_class, bool ambiguous) noexcept
{
	if (INFLIGHT.retry_attempts < TELEMETRY_TRANSPORT_MAX_RETRY_ATTEMPTS)
		++INFLIGHT.retry_attempts;
	if (ambiguous)
	{
		INFLIGHT.resolution_required = true;
		saturating_add(HEALTH.ambiguous_commits, std::uint64_t{ 1U });
	}
	else
		saturating_add(HEALTH.retryable_failures, std::uint64_t{ 1U });
	if (!ambiguous && INFLIGHT.retry_attempts >= TELEMETRY_TRANSPORT_MAX_RETRY_ATTEMPTS)
	{
		const std::size_t index = INFLIGHT.isolation ? INFLIGHT.isolation_index : 0U;
		open_circuit(now, error_code, failure_class, &INFLIGHT_RECORDS[index],
			     INFLIGHT.isolation ? 1U : INFLIGHT.count, INFLIGHT.retry_attempts);
		INFLIGHT.retry_not_before = std::numeric_limits<telemetry_monotonic_usec>::max();
		publish_inflight_health();
		return false;
	}
	INFLIGHT.retry_not_before =
		saturating_time_add(now, retry_backoff(INFLIGHT.retry_attempts));
	mark_failure(now, error_code, failure_class);
	capture_failure_identity(
		&INFLIGHT_RECORDS[INFLIGHT.isolation ? INFLIGHT.isolation_index : 0U],
		INFLIGHT.isolation ? 1U : INFLIGHT.count, failure_class, error_code,
		INFLIGHT.retry_attempts);
	publish_inflight_health();
	return true;
}

bool schedule_repository_retry(telemetry_monotonic_usec now, std::uint32_t error_code) noexcept
{
	if (REPOSITORY_RETRY_ATTEMPTS < TELEMETRY_TRANSPORT_MAX_RETRY_ATTEMPTS)
		++REPOSITORY_RETRY_ATTEMPTS;
	if (REPOSITORY_RETRY_ATTEMPTS >= TELEMETRY_TRANSPORT_MAX_RETRY_ATTEMPTS)
	{
		open_circuit(now, error_code, telemetry_failure_class::transient_connection,
			     nullptr, 0U, REPOSITORY_RETRY_ATTEMPTS);
		REPOSITORY_RETRY_NOT_BEFORE = std::numeric_limits<telemetry_monotonic_usec>::max();
		HEALTH.repository_retry_attempts.store(REPOSITORY_RETRY_ATTEMPTS,
						       std::memory_order_release);
		HEALTH.repository_retry_not_before.store(REPOSITORY_RETRY_NOT_BEFORE,
							 std::memory_order_release);
		return false;
	}
	REPOSITORY_RETRY_NOT_BEFORE =
		saturating_time_add(now, retry_backoff(REPOSITORY_RETRY_ATTEMPTS));
	mark_failure(now, error_code, telemetry_failure_class::transient_connection);
	HEALTH.repository_retry_attempts.store(REPOSITORY_RETRY_ATTEMPTS,
					       std::memory_order_release);
	HEALTH.repository_retry_not_before.store(REPOSITORY_RETRY_NOT_BEFORE,
						 std::memory_order_release);
	return true;
}

void count_duration(const telemetry_record &record) noexcept
{
	if (record.header.kind == telemetry_record_kind::interval)
	{
		const auto &interval = record.payload.interval;
		const bool unknown = interval.category == telemetry_interval_category::unknown;
		if (unknown)
			saturating_add(HEALTH.unknown_duration_usec, interval.duration_usec);
		else
			saturating_add(HEALTH.attributable_duration_usec, interval.duration_usec);
	}
	else if (record.header.kind == telemetry_record_kind::coverage_gap &&
		 record.payload.gap.duration_usec != 0U)
		saturating_add(HEALTH.unknown_duration_usec, record.payload.gap.duration_usec);
}

bool validate_early_result(const telemetry_apply_batch_result &result) noexcept
{
	if (!apply_result_reserved_is_zero(result) || result.input_count != 0U ||
	    result.result_count != 0U || result.applied_count != 0U ||
	    result.duplicate_count != 0U || result.stale_checkpoint_count != 0U ||
	    result.invalid_count != 0U || result.conflict_count != 0U ||
	    result.quarantined_count != 0U || result.first_record_seq != 0U ||
	    result.last_record_seq != 0U)
		return false;
	for (const auto &item : result.results)
		if (!record_apply_unused_is_zero(item))
			return false;
	return true;
}

bool validate_full_result(const telemetry_apply_batch_result &result,
			  const telemetry_record *records, std::size_t count, bool retry) noexcept
{
	if (!apply_result_reserved_is_zero(result) || result.input_count != count ||
	    result.result_count != count || count == 0U ||
	    result.first_record_seq != records[0].header.key.record_seq ||
	    result.last_record_seq != records[count - 1U].header.key.record_seq)
		return false;
	if (retry && (result.applied_count != 0U || result.duplicate_count != 0U ||
		      result.stale_checkpoint_count != 0U || result.invalid_count != 0U ||
		      result.conflict_count != 0U || result.quarantined_count != 0U))
		return false;
	std::uint32_t applied = 0U;
	std::uint32_t duplicate = 0U;
	std::uint32_t stale = 0U;
	std::uint32_t invalid = 0U;
	std::uint32_t conflict = 0U;
	std::uint32_t quarantined = 0U;
	for (std::size_t index = 0U; index < count; ++index)
	{
		const auto &item = result.results[index];
		if (!record_apply_reserved_is_zero(item) ||
		    !key_equal(item.key, records[index].header.key))
			return false;
		switch (item.outcome)
		{
		case telemetry_apply_outcome::applied:
			++applied;
			break;
		case telemetry_apply_outcome::duplicate_identical:
			++duplicate;
			break;
		case telemetry_apply_outcome::checkpoint_older:
			++stale;
			break;
		case telemetry_apply_outcome::rejected_invalid:
			++invalid;
			break;
		case telemetry_apply_outcome::duplicate_conflict:
			++conflict;
			break;
		case telemetry_apply_outcome::quarantined_invalid:
			++invalid;
			++quarantined;
			if (retry || item.failure_class != telemetry_failure_class::invalid_record)
				return false;
			break;
		case telemetry_apply_outcome::retryable_failure:
			if (!retry)
				return false;
			break;
		case telemetry_apply_outcome::commit_ambiguous:
			if (!retry)
				return false;
			break;
		case telemetry_apply_outcome::permanent_failure:
			if (!retry)
				return false;
			break;
		default:
			return false;
		}
		if (retry)
		{
			const auto expected =
				result.outcome == telemetry_batch_outcome::commit_ambiguous ?
					telemetry_apply_outcome::commit_ambiguous :
				result.outcome == telemetry_batch_outcome::permanent_failure ?
					telemetry_apply_outcome::permanent_failure :
					telemetry_apply_outcome::retryable_failure;
			if (item.outcome != expected || item.failure_class != result.failure_class)
				return false;
		}
	}
	for (std::size_t index = count; index < TELEMETRY_BATCH_MAX_RECORDS_PROPOSAL; ++index)
		if (!record_apply_unused_is_zero(result.results[index]))
			return false;
	if (retry)
		return (result.outcome == telemetry_batch_outcome::commit_ambiguous &&
			result.failure_class == telemetry_failure_class::commit_ambiguous) ||
		       (result.outcome == telemetry_batch_outcome::permanent_failure &&
			telemetry_failure_is_permanent(result.failure_class)) ||
		       (result.outcome == telemetry_batch_outcome::retryable_failure &&
			telemetry_failure_is_retryable(result.failure_class));
	return result.applied_count == applied && result.duplicate_count == duplicate &&
	       result.stale_checkpoint_count == stale && result.invalid_count == invalid &&
	       result.conflict_count == conflict && result.quarantined_count == quarantined &&
	       (result.outcome == telemetry_batch_outcome::committed ?
			(result.invalid_count == 0U && result.conflict_count == 0U) :
			(result.outcome == telemetry_batch_outcome::committed_with_rejections &&
			 (result.invalid_count != 0U || result.conflict_count != 0U ||
			  result.quarantined_count != 0U)));
}

bool ensure_repository(telemetry_monotonic_usec now) noexcept
{
	if (CIRCUIT_OPEN.load(std::memory_order_acquire))
		return false;
	if (REPOSITORY_READY.load(std::memory_order_acquire))
		return true;
	if (now < REPOSITORY_RETRY_NOT_BEFORE)
		return false;
	const telemetry_repository_outcome outcome =
		REPOSITORY.init(REPOSITORY.context, REPOSITORY_SETTINGS);
	if (outcome == telemetry_repository_outcome::ready ||
	    outcome == telemetry_repository_outcome::already_ready)
	{
		REPOSITORY_STARTED.store(true, std::memory_order_release);
		REPOSITORY_READY.store(true, std::memory_order_release);
		REPOSITORY_RETRY_ATTEMPTS = 0U;
		REPOSITORY_RETRY_NOT_BEFORE = 0U;
		HEALTH.repository_retry_attempts.store(0U, std::memory_order_release);
		HEALTH.repository_retry_not_before.store(0U, std::memory_order_release);
		if (!STOP_REQUESTED.load(std::memory_order_acquire))
			set_health_state(telemetry_health_state::healthy);
		return true;
	}
	if (outcome == telemetry_repository_outcome::flatfile_disabled)
	{
		HEALTH.backend.store(
			static_cast<std::uint8_t>(telemetry_storage_backend::flatfile_disabled),
			std::memory_order_release);
		HEALTH.disabled_reason.store(
			static_cast<std::uint8_t>(telemetry_disabled_reason::flatfile_authority),
			std::memory_order_release);
		set_health_state(telemetry_health_state::disabled);
		std::uint8_t active = LIFECYCLE_ACTIVE;
		(void)LIFECYCLE.compare_exchange_strong(active, LIFECYCLE_DISABLED,
							std::memory_order_acq_rel);
		return false;
	}
	if (outcome == telemetry_repository_outcome::stopping ||
	    outcome == telemetry_repository_outcome::closed)
	{
		STOP_REQUESTED.store(true, std::memory_order_release);
		LIFECYCLE.store(LIFECYCLE_STOPPING, std::memory_order_release);
		set_health_state(telemetry_health_state::stopping);
		return false;
	}
	if (outcome == telemetry_repository_outcome::permanent_failure)
	{
		open_circuit(now, 0U, telemetry_failure_class::permanent_repository, nullptr, 0U,
			     REPOSITORY_RETRY_ATTEMPTS);
		return false;
	}
	schedule_repository_retry(now, 0U);
	return false;
}

void update_pulse_counts(const telemetry_apply_batch_result &result,
			 const telemetry_record *records, std::size_t count,
			 telemetry_transport_pulse_result &pulse) noexcept
{
	pulse.examined += static_cast<std::uint32_t>(count);
	pulse.applied += result.applied_count;
	pulse.duplicate_or_stale += result.duplicate_count + result.stale_checkpoint_count;
	pulse.rejected += result.invalid_count + result.conflict_count;
	for (std::size_t index = 0U; index < count; ++index)
	{
		const auto &item = result.results[index];
		const auto outcome = item.outcome;
		if (outcome == telemetry_apply_outcome::applied)
		{
			saturating_add(HEALTH.applied_records, std::uint64_t{ 1U });
			count_duration(records[index]);
		}
		else if (outcome == telemetry_apply_outcome::duplicate_identical)
			saturating_add(HEALTH.duplicate_records, std::uint64_t{ 1U });
		else if (outcome == telemetry_apply_outcome::checkpoint_older)
			saturating_add(HEALTH.stale_checkpoint_records, std::uint64_t{ 1U });
		else if (outcome == telemetry_apply_outcome::rejected_invalid)
			saturating_add(HEALTH.invalid_records, std::uint64_t{ 1U });
		else if (outcome == telemetry_apply_outcome::quarantined_invalid)
		{
			saturating_add(HEALTH.invalid_records, std::uint64_t{ 1U });
			saturating_add(HEALTH.quarantined_records, std::uint64_t{ 1U });
			capture_failure_identity(&records[index], 1U, item.failure_class,
						 item.error_code, 0U);
		}
		else if (outcome == telemetry_apply_outcome::duplicate_conflict)
			saturating_add(HEALTH.conflict_records, std::uint64_t{ 1U });
	}
}

void discard_records_for_disabled() noexcept
{
	const std::size_t count = telemetry_queue_private::physical_depth(&QUEUE);
	if (count == 0U)
		return;
	std::size_t controls = 0U;
	for (std::size_t index = 0U; index < count; ++index)
	{
		telemetry_record record{};
		if (telemetry_queue_private::peek(&QUEUE, index, &record, nullptr, nullptr) &&
		    telemetry_record_kind_is_control(record.header.kind))
			++controls;
	}
	if (telemetry_queue_private::commit(&QUEUE, count, controls))
	{
		saturating_add(HEALTH.dropped_detail, static_cast<std::uint64_t>(count - controls));
		saturating_add(HEALTH.dropped_control, static_cast<std::uint64_t>(controls));
	}
}

void discard_inflight_for_disabled() noexcept
{
	if (!INFLIGHT.active)
		return;
	{
		if (telemetry_queue_private::commit(&QUEUE, INFLIGHT.count, INFLIGHT.control_count))
		{
			saturating_add(HEALTH.dropped_detail,
				       static_cast<std::uint64_t>(INFLIGHT.count -
								  INFLIGHT.control_count));
			saturating_add(HEALTH.dropped_control,
				       static_cast<std::uint64_t>(INFLIGHT.control_count));
		}
	}
	INFLIGHT = {};
	clear_inflight_health();
}

void disable_transport() noexcept
{
	discard_inflight_for_disabled();
	discard_records_for_disabled();
	REPOSITORY_READY.store(false, std::memory_order_release);
	HEALTH.disabled_reason.store(
		static_cast<std::uint8_t>(
			SETTINGS.backend == telemetry_storage_backend::flatfile_disabled ?
				telemetry_disabled_reason::flatfile_authority :
				telemetry_disabled_reason::configured_off),
		std::memory_order_release);
	QUIESCED.store(true, std::memory_order_release);
	set_health_state(telemetry_health_state::disabled);
	std::uint8_t active = LIFECYCLE_ACTIVE;
	(void)LIFECYCLE.compare_exchange_strong(active, LIFECYCLE_DISABLED,
						std::memory_order_acq_rel);
}

bool should_flush(telemetry_monotonic_usec now, bool force) noexcept
{
	if (telemetry_queue_private::physical_depth(&QUEUE) == 0U)
		return false;
	if (force)
		return true;
	const std::size_t physical = telemetry_queue_private::physical_depth(&QUEUE);
	if (physical >= SETTINGS.max_batch_records ||
	    physical >= SETTINGS.max_batch_bytes / sizeof(telemetry_record))
		return true;
	telemetry_monotonic_usec admitted_at = 0U;
	bool admitted_at_valid = false;
	telemetry_record ignored{};
	if (!telemetry_queue_private::peek(&QUEUE, 0U, &ignored, &admitted_at, &admitted_at_valid))
		return false;
	// An unavailable/backwards age clock must not strand a partial batch forever.
	if (!admitted_at_valid || now < admitted_at)
		return true;
	return now - admitted_at >= SETTINGS.flush_oldest_after_usec;
}

bool begin_batch() noexcept
{
	const std::size_t physical = telemetry_queue_private::physical_depth(&QUEUE);
	if (physical == 0U)
		return false;
	const std::size_t byte_limit = SETTINGS.max_batch_bytes / sizeof(telemetry_record);
	const std::size_t limit =
		physical < SETTINGS.max_batch_records ? physical : SETTINGS.max_batch_records;
	const std::size_t count_limit = limit < byte_limit ? limit : byte_limit;
	if (count_limit == 0U)
		return false;
	INFLIGHT = {};
	clear_inflight_health();
	INFLIGHT.active = true;
	INFLIGHT.count = count_limit;
	for (std::size_t index = 0U; index < count_limit; ++index)
	{
		bool admitted_at_valid = false;
		if (!telemetry_queue_private::peek(&QUEUE, index, &INFLIGHT_RECORDS[index],
						   index == 0U ? &INFLIGHT.oldest_admitted_at :
								 nullptr,
						   index == 0U ? &admitted_at_valid : nullptr))
		{
			INFLIGHT = {};
			clear_inflight_health();
			return false;
		}
		if (index == 0U)
			INFLIGHT.oldest_admitted_at_valid = admitted_at_valid;
		if (telemetry_record_kind_is_control(INFLIGHT_RECORDS[index].header.kind))
			++INFLIGHT.control_count;
	}
	publish_inflight_health();
	return true;
}

void handle_success(const telemetry_apply_batch_result &result, const telemetry_record *records,
		    std::size_t attempt_count, telemetry_monotonic_usec now,
		    telemetry_transport_pulse_result &pulse) noexcept
{
	update_pulse_counts(result, records, attempt_count, pulse);
	if (result.quarantined_count != 0U)
		HEALTH.last_failure_monotonic_usec.store(now, std::memory_order_release);
	HEALTH.last_success_monotonic_usec.store(now, std::memory_order_release);
	if (attempt_count != 0U)
		HEALTH.last_committed_record_seq.store(
			records[attempt_count - 1U].header.key.record_seq,
			std::memory_order_release);
	if (!STOP_REQUESTED.load(std::memory_order_acquire))
		set_health_state(result.invalid_count != 0U || result.conflict_count != 0U ||
						 result.quarantined_count != 0U ?
					 telemetry_health_state::degraded :
					 telemetry_health_state::healthy);

	if (INFLIGHT.isolation)
	{
		++INFLIGHT.isolation_index;
		INFLIGHT.retry_attempts = 0U;
		INFLIGHT.retry_not_before = 0U;
		if (INFLIGHT.isolation_index < INFLIGHT.count)
		{
			publish_inflight_health();
			return;
		}
	}
	{
		if (!telemetry_queue_private::commit(&QUEUE, INFLIGHT.count,
						     INFLIGHT.control_count))
		{
			mark_failure(now, static_cast<std::uint32_t>(EPROTO));
			INFLIGHT.retry_not_before = now;
			publish_inflight_health();
			return;
		}
	}
	INFLIGHT = {};
	clear_inflight_health();
}

void handle_corrupt_callback(telemetry_monotonic_usec now,
			     telemetry_transport_pulse_result &) noexcept
{
	INFLIGHT.resolution_required = true;
	schedule_inflight_retry(now, static_cast<std::uint32_t>(EPROTO),
				telemetry_failure_class::transient_internal, false);
}

void handle_disabled_callback(const telemetry_apply_batch_result &result,
			      telemetry_monotonic_usec now,
			      telemetry_transport_pulse_result &pulse) noexcept
{
	if (!validate_early_result(result))
	{
		handle_corrupt_callback(now, pulse);
		pulse.outcome = telemetry_transport_outcome::unavailable;
		fill_pending(pulse);
		return;
	}
	disable_transport();
	pulse.outcome = telemetry_transport_outcome::flatfile_disabled;
	fill_pending(pulse);
}

void handle_retry_result(const telemetry_apply_batch_result &result,
			 telemetry_monotonic_usec now) noexcept
{
	const bool ambiguous = result.outcome == telemetry_batch_outcome::commit_ambiguous;
	const telemetry_failure_class failure_class =
		result.failure_class == telemetry_failure_class::none ?
			telemetry_failure_class::transient_connection :
			result.failure_class;
	if (result.outcome == telemetry_batch_outcome::unavailable)
	{
		REPOSITORY_READY.store(false, std::memory_order_release);
		schedule_repository_retry(now, result.error_code);
	}
	schedule_inflight_retry(now, result.error_code, failure_class, ambiguous);
}

void start_isolation() noexcept
{
	INFLIGHT.isolation = true;
	INFLIGHT.isolation_index = 0U;
	INFLIGHT.retry_attempts = 0U;
	INFLIGHT.retry_not_before = 0U;
	publish_inflight_health();
}

void handle_invalid_batch(telemetry_monotonic_usec now,
			  telemetry_transport_pulse_result &pulse) noexcept
{
	if (INFLIGHT.count > 1U && !INFLIGHT.isolation)
	{
		start_isolation();
		return;
	}
	telemetry_apply_batch_result synthetic{};
	synthetic.outcome = telemetry_batch_outcome::committed_with_rejections;
	synthetic.input_count = 1U;
	synthetic.result_count = 1U;
	synthetic.invalid_count = 1U;
	synthetic.first_record_seq =
		INFLIGHT_RECORDS[INFLIGHT.isolation ? INFLIGHT.isolation_index : 0U]
			.header.key.record_seq;
	synthetic.last_record_seq = synthetic.first_record_seq;
	const std::size_t index = INFLIGHT.isolation ? INFLIGHT.isolation_index : 0U;
	synthetic.results[0].key = INFLIGHT_RECORDS[index].header.key;
	synthetic.results[0].outcome = telemetry_apply_outcome::rejected_invalid;
	handle_success(synthetic, &INFLIGHT_RECORDS[index], 1U, now, pulse);
}

void set_repository_unavailable(telemetry_transport_pulse_result &result) noexcept
{
	result.outcome = LIFECYCLE.load(std::memory_order_acquire) == LIFECYCLE_DISABLED ?
				 telemetry_transport_outcome::flatfile_disabled :
			 health_state() == telemetry_health_state::stopping ?
				 telemetry_transport_outcome::stopping :
				 telemetry_transport_outcome::unavailable;
}

void attempt_inflight(telemetry_monotonic_usec now, telemetry_transport_pulse_result &result,
		      bool *repository_called) noexcept
{
	const std::size_t index = INFLIGHT.isolation ? INFLIGHT.isolation_index : 0U;
	const std::size_t attempt_count = INFLIGHT.isolation ? 1U : INFLIGHT.count;
	const telemetry_record *attempt_records = INFLIGHT.isolation ? &INFLIGHT_RECORDS[index] :
								       INFLIGHT_RECORDS.data();
	if (!ensure_repository(now))
	{
		set_repository_unavailable(result);
		return;
	}
	const telemetry_apply_batch_result apply_result =
		REPOSITORY.apply(REPOSITORY.context, attempt_records, attempt_count);
	if (repository_called != nullptr)
		*repository_called = true;
	if (INFLIGHT.resolution_required &&
	    (apply_result.outcome == telemetry_batch_outcome::invalid_batch ||
	     apply_result.outcome == telemetry_batch_outcome::disabled))
	{
		schedule_inflight_retry(now, static_cast<std::uint32_t>(EPROTO),
					telemetry_failure_class::commit_ambiguous, true);
		result.outcome = telemetry_transport_outcome::unavailable;
		return;
	}
	if (apply_result.outcome == telemetry_batch_outcome::disabled)
	{
		handle_disabled_callback(apply_result, now, result);
		return;
	}
	if (apply_result.outcome == telemetry_batch_outcome::unavailable)
	{
		if (validate_early_result(apply_result))
			handle_retry_result(apply_result, now);
		else
			handle_corrupt_callback(now, result);
		result.outcome = telemetry_transport_outcome::unavailable;
		return;
	}
	if (apply_result.outcome == telemetry_batch_outcome::invalid_batch)
	{
		if (validate_early_result(apply_result))
			handle_invalid_batch(now, result);
		else
			handle_corrupt_callback(now, result);
		return;
	}
	if (apply_result.outcome == telemetry_batch_outcome::retryable_failure ||
	    apply_result.outcome == telemetry_batch_outcome::commit_ambiguous)
	{
		if (validate_full_result(apply_result, attempt_records, attempt_count, true))
			handle_retry_result(apply_result, now);
		else
			handle_corrupt_callback(now, result);
		result.outcome = telemetry_transport_outcome::unavailable;
		return;
	}
	if (apply_result.outcome == telemetry_batch_outcome::permanent_failure)
	{
		if (validate_full_result(apply_result, attempt_records, attempt_count, true))
			open_circuit(now, apply_result.error_code, apply_result.failure_class,
				     attempt_records, attempt_count, INFLIGHT.retry_attempts);
		else
			handle_corrupt_callback(now, result);
		result.outcome = telemetry_transport_outcome::unavailable;
		return;
	}
	if ((apply_result.outcome != telemetry_batch_outcome::committed &&
	     apply_result.outcome != telemetry_batch_outcome::committed_with_rejections) ||
	    !validate_full_result(apply_result, attempt_records, attempt_count, false))
	{
		handle_corrupt_callback(now, result);
		result.outcome = telemetry_transport_outcome::unavailable;
		return;
	}
	handle_success(apply_result, attempt_records, attempt_count, now, result);
}

telemetry_transport_pulse_result pulse_impl(telemetry_monotonic_usec now, bool force,
					    bool *repository_called) noexcept
{
	telemetry_transport_pulse_result result{};
	result.outcome = telemetry_transport_outcome::drained;
	if (repository_called != nullptr)
		*repository_called = false;
	const std::uint8_t lifecycle = LIFECYCLE.load(std::memory_order_acquire);
	if (lifecycle == LIFECYCLE_UNINITIALIZED || lifecycle == LIFECYCLE_STOPPED)
	{
		result.outcome = telemetry_transport_outcome::unavailable;
		fill_pending(result);
		return result;
	}
	if (lifecycle == LIFECYCLE_DISABLED)
	{
		result.outcome = telemetry_transport_outcome::flatfile_disabled;
		fill_pending(result);
		return result;
	}
	if (CIRCUIT_OPEN.load(std::memory_order_acquire))
	{
		result.outcome = telemetry_transport_outcome::unavailable;
		fill_pending(result);
		return result;
	}

	if (STOP_REQUESTED.load(std::memory_order_acquire) && pending_count() == 0U)
	{
		set_health_state(telemetry_health_state::stopped);
		return result;
	}
	if (!ensure_repository(now))
	{
		set_repository_unavailable(result);
		fill_pending(result);
		return result;
	}
	if (INFLIGHT.active)
	{
		if (now < INFLIGHT.retry_not_before)
		{
			result.outcome = telemetry_transport_outcome::unavailable;
			fill_pending(result);
			return result;
		}
		attempt_inflight(now, result, repository_called);
		fill_pending(result);
		return result;
	}

	const bool force_queue = force || STOP_REQUESTED.load(std::memory_order_acquire);
	if (telemetry_queue_private::physical_depth(&QUEUE) != 0U &&
	    should_flush(now, force_queue) && begin_batch())
	{
		attempt_inflight(now, result, repository_called);
		fill_pending(result);
		return result;
	}

	if (STOP_REQUESTED.load(std::memory_order_acquire))
		result.outcome = telemetry_transport_outcome::stopping;
	else if (!REPOSITORY_READY.load(std::memory_order_acquire))
		result.outcome = telemetry_transport_outcome::unavailable;
	fill_pending(result);
	return result;
}

void finalize_shutdown() noexcept
{
	std::uint64_t admission = ADMISSION_STATE.load(std::memory_order_seq_cst);
	for (;;)
	{
		if ((admission & ADMISSION_FINALIZING) != 0U)
			return;
		if ((admission & ADMISSION_CLOSED) == 0U)
		{
			const std::uint64_t closed = admission | ADMISSION_CLOSED;
			if (!ADMISSION_STATE.compare_exchange_strong(admission, closed,
								     std::memory_order_seq_cst,
								     std::memory_order_seq_cst))
				continue;
			admission = closed;
		}
		const std::uint64_t finalizing = admission | ADMISSION_FINALIZING;
		if (!ADMISSION_STATE.compare_exchange_strong(admission, finalizing,
							     std::memory_order_seq_cst,
							     std::memory_order_seq_cst))
			continue;
		admission = finalizing;
		break;
	}
	if ((admission & ADMISSION_COUNT_MASK) != 0U)
	{
		ADMISSION_STATE.fetch_and(~ADMISSION_FINALIZING, std::memory_order_seq_cst);
		return;
	}
	if (LIFECYCLE.load(std::memory_order_acquire) != LIFECYCLE_STOPPED && pending_count() != 0U)
		saturating_add(HEALTH.unclosed_tail_count, std::uint64_t{ 1U });
	// With worker joined and producer admission drained, these counts are stable.
	// Never-attempted records are definite RAM losses; an unresolved batch is
	// only an unknown tail (it may already have committed).
	if (!INFLIGHT.active)
	{
		const auto retained = telemetry_queue_private::depth(&QUEUE);
		const auto controls = telemetry_queue_private::control_depth(&QUEUE);
		if (controls <= retained)
		{
			saturating_add(HEALTH.dropped_detail,
				       static_cast<std::uint64_t>(retained - controls));
			saturating_add(HEALTH.dropped_control,
				       static_cast<std::uint64_t>(controls));
		}
	}
	telemetry_queue_private::reset(&QUEUE);
	reset_inflight();
	REPOSITORY_RETRY_ATTEMPTS = 0U;
	REPOSITORY_RETRY_NOT_BEFORE = 0U;
	HEALTH.repository_retry_attempts.store(0U, std::memory_order_release);
	HEALTH.repository_retry_not_before.store(0U, std::memory_order_release);
	REPOSITORY_READY.store(false, std::memory_order_release);
	QUIESCED.store(true, std::memory_order_release);
	STOP_REQUESTED.store(false, std::memory_order_release);
	RESTART_ALLOWED.store(true, std::memory_order_release);
	SHUTDOWN_REQUESTED.store(false, std::memory_order_release);
	set_health_state(telemetry_health_state::stopped);
	LIFECYCLE.store(LIFECYCLE_STOPPED, std::memory_order_release);
	ADMISSION_STATE.store(0U, std::memory_order_release);
}

struct producer_guard
{
	bool entered = false;

	producer_guard() noexcept
	{
		std::uint64_t admission = ADMISSION_STATE.load(std::memory_order_seq_cst);
		for (;;)
		{
			if ((admission & (ADMISSION_CLOSED | ADMISSION_FINALIZING)) != 0U ||
			    LIFECYCLE.load(std::memory_order_seq_cst) != LIFECYCLE_ACTIVE)
				return;
			if (admission == ADMISSION_COUNT_MASK)
				return;
			if (ADMISSION_STATE.compare_exchange_strong(admission, admission + 1U,
								    std::memory_order_seq_cst,
								    std::memory_order_seq_cst))
				break;
		}
		if (LIFECYCLE.load(std::memory_order_seq_cst) != LIFECYCLE_ACTIVE ||
		    SHUTDOWN_REQUESTED.load(std::memory_order_acquire))
		{
			const std::uint64_t previous =
				ADMISSION_STATE.fetch_sub(1U, std::memory_order_seq_cst);
			if ((previous & ADMISSION_COUNT_MASK) == 1U &&
			    SHUTDOWN_REQUESTED.load(std::memory_order_acquire))
				finalize_shutdown();
			return;
		}
		entered = true;
	}
	~producer_guard()
	{
		if (!entered)
			return;
		const std::uint64_t previous =
			ADMISSION_STATE.fetch_sub(1U, std::memory_order_seq_cst);
		if (entered && (previous & ADMISSION_COUNT_MASK) == 1U &&
		    SHUTDOWN_REQUESTED.load(std::memory_order_acquire))
			finalize_shutdown();
	}
};

} // namespace

telemetry_transport_outcome
telemetry_transport_bind_for_tests(const telemetry_transport_repository_binding *repository,
				   const telemetry_transport_clock_binding *clock)
{
	const std::uint8_t lifecycle = LIFECYCLE.load(std::memory_order_acquire);
	if (lifecycle == LIFECYCLE_ACTIVE || lifecycle == LIFECYCLE_STARTING)
		return telemetry_transport_outcome::already_started;
	if (lifecycle == LIFECYCLE_STOPPING)
		return telemetry_transport_outcome::stopping;
	if (repository == nullptr || repository->init == nullptr || repository->apply == nullptr ||
	    clock == nullptr || clock->now == nullptr)
		return telemetry_transport_outcome::invalid_config;
	REPOSITORY = *repository;
	CLOCK = *clock;
	return telemetry_transport_outcome::started;
}

void telemetry_transport_unbind_for_tests(void)
{
	const std::uint8_t lifecycle = LIFECYCLE.load(std::memory_order_acquire);
	if (lifecycle == LIFECYCLE_ACTIVE || lifecycle == LIFECYCLE_STARTING ||
	    lifecycle == LIFECYCLE_STOPPING)
		return;
	REPOSITORY = DEFAULT_REPOSITORY;
	CLOCK = DEFAULT_CLOCK;
}

telemetry_transport_outcome telemetry_transport_quiesce_for_tests(void)
{
	if (LIFECYCLE.load(std::memory_order_acquire) != LIFECYCLE_ACTIVE)
		return telemetry_transport_outcome::stopping;
	QUIESCED.store(true, std::memory_order_release);
	return telemetry_transport_outcome::stopping;
}

telemetry_transport_outcome telemetry_transport_resume_for_tests(void)
{
	if (LIFECYCLE.load(std::memory_order_acquire) != LIFECYCLE_ACTIVE ||
	    STOP_REQUESTED.load(std::memory_order_acquire))
		return telemetry_transport_outcome::stopping;
	QUIESCED.store(false, std::memory_order_release);
	return telemetry_transport_outcome::started;
}

telemetry_transport_outcome telemetry_transport_init(telemetry_transport_config config)
{
	if (STOP_REQUESTED.load(std::memory_order_acquire))
		return telemetry_transport_outcome::stopping;
	if (!telemetry_transport_config_is_bounded(config))
		return telemetry_transport_outcome::invalid_config;

	std::uint8_t observed = LIFECYCLE.load(std::memory_order_acquire);
	for (;;)
	{
		if (observed == LIFECYCLE_ACTIVE || observed == LIFECYCLE_STARTING)
			return telemetry_transport_outcome::already_started;
		if (observed == LIFECYCLE_STOPPING)
			return telemetry_transport_outcome::stopping;
		if (observed == LIFECYCLE_UNINITIALIZED &&
		    STOP_REQUESTED.load(std::memory_order_acquire))
			return telemetry_transport_outcome::stopping;
		if (observed == LIFECYCLE_STOPPED &&
		    (!RESTART_ALLOWED.load(std::memory_order_acquire) ||
		     STOP_REQUESTED.load(std::memory_order_acquire)))
			return telemetry_transport_outcome::stopping;
		if (observed == LIFECYCLE_UNINITIALIZED || observed == LIFECYCLE_STOPPED)
		{
			if (LIFECYCLE.compare_exchange_strong(observed, LIFECYCLE_STARTING,
							      std::memory_order_acq_rel,
							      std::memory_order_acquire))
				break;
			continue;
		}
		return telemetry_transport_outcome::unavailable;
	}

	SETTINGS = config;
	LAST_ADMITTED_KEY = {};
	LOSS = {};
	LOSS_RANGE_UNKNOWN = false;
	REPOSITORY_SETTINGS = { config.backend,		0U,
				config.schema_version,	config.max_batch_records,
				config.max_batch_bytes, config.fresh_producer };
	telemetry_queue_private::initialize(&QUEUE, config.queue_capacity, config.control_reserve);
	reset_inflight();
	reset_health(config);
	QUIESCED.store(false, std::memory_order_release);
	RESTART_ALLOWED.store(false, std::memory_order_release);
	SHUTDOWN_REQUESTED.store(false, std::memory_order_release);
	REPOSITORY_READY.store(false, std::memory_order_release);
	REPOSITORY_STARTED.store(false, std::memory_order_release);
	CIRCUIT_OPEN.store(false, std::memory_order_release);

	if (config.backend == telemetry_storage_backend::flatfile_disabled)
	{
		HEALTH.disabled_reason.store(
			static_cast<std::uint8_t>(telemetry_disabled_reason::flatfile_authority),
			std::memory_order_release);
		set_health_state(telemetry_health_state::disabled);
		std::uint8_t starting = LIFECYCLE_STARTING;
		if (!LIFECYCLE.compare_exchange_strong(starting, LIFECYCLE_DISABLED,
						       std::memory_order_acq_rel))
		{
			set_health_state(telemetry_health_state::stopping);
			return telemetry_transport_outcome::stopping;
		}
		return telemetry_transport_outcome::flatfile_disabled;
	}

	REPOSITORY_RETRY_ATTEMPTS = 0U;
	REPOSITORY_RETRY_NOT_BEFORE = 0U;
	std::uint8_t starting = LIFECYCLE_STARTING;
	if (!LIFECYCLE.compare_exchange_strong(starting, LIFECYCLE_ACTIVE,
					       std::memory_order_acq_rel))
	{
		set_health_state(telemetry_health_state::stopping);
		return telemetry_transport_outcome::stopping;
	}
	return telemetry_transport_outcome::started;
}

telemetry_enqueue_result telemetry_transport_enqueue(telemetry_record record)
{
	producer_guard guard;
	telemetry_enqueue_result result{};
	result.key = record.header.key;
	const std::uint8_t lifecycle = LIFECYCLE.load(std::memory_order_seq_cst);
	const bool control = telemetry_record_kind_is_control(record.header.kind);
	if (!guard.entered)
	{
		result.admission = STOP_REQUESTED.load(std::memory_order_acquire) ||
						   lifecycle == LIFECYCLE_STOPPING ?
					   telemetry_queue_admission::rejected_stopping :
					   telemetry_queue_admission::rejected_disabled;
		update_admission_result(result);
		return result;
	}
	if (lifecycle == LIFECYCLE_DISABLED)
	{
		result.admission = telemetry_queue_admission::rejected_disabled;
		update_admission_result(result);
		return result;
	}
	if (lifecycle != LIFECYCLE_ACTIVE || STOP_REQUESTED.load(std::memory_order_acquire) ||
	    QUIESCED.load(std::memory_order_acquire))
	{
		const bool stopping = STOP_REQUESTED.load(std::memory_order_acquire) ||
				      QUIESCED.load(std::memory_order_acquire) ||
				      lifecycle == LIFECYCLE_STOPPING;
		result.admission = stopping ? telemetry_queue_admission::rejected_stopping :
					      telemetry_queue_admission::rejected_disabled;
		update_admission_result(result);
		return result;
	}
	if (!telemetry_record_is_valid(record) ||
	    (LAST_ADMITTED_KEY.record_seq != 0U &&
	     (record.header.key.producer.boot_id != LAST_ADMITTED_KEY.producer.boot_id ||
	      record.header.key.producer.process_id != LAST_ADMITTED_KEY.producer.process_id ||
	      record.header.key.record_seq <= LAST_ADMITTED_KEY.record_seq)))
	{
		result.admission = telemetry_queue_admission::rejected_invalid;
		update_admission_result(result);
		return result;
	}
	if (CIRCUIT_OPEN.load(std::memory_order_acquire) && !control)
	{
		record_failed_admission(record, false);
		result.admission = telemetry_queue_admission::rejected_circuit_open;
		update_admission_result(result);
		return result;
	}

	telemetry_monotonic_usec now = 0U;
	const bool now_valid = clock_now(now);
	const telemetry_queue_private::push_result pushed =
		telemetry_queue_private::try_push(&QUEUE, record, now, now_valid, control);
	if (!pushed.accepted)
	{
		record_failed_admission(record, control);
		result.admission = control ? telemetry_queue_admission::rejected_control_full :
					     telemetry_queue_admission::rejected_detail_full;
		update_admission_result(result);
		return result;
	}
	LAST_ADMITTED_KEY = record.header.key;
	HEALTH.producer_boot_id.store(record.header.key.producer.boot_id,
				      std::memory_order_release);
	HEALTH.producer_process_id.store(record.header.key.producer.process_id,
					 std::memory_order_release);
	HEALTH.last_admitted_record_seq.store(record.header.key.record_seq,
					      std::memory_order_release);
	result.admission = control ? telemetry_queue_admission::accepted_control_reserve :
				     telemetry_queue_admission::accepted_detail;
	if (control)
		saturating_add(HEALTH.admitted_control, std::uint64_t{ 1U });
	else
		saturating_add(HEALTH.admitted_detail, std::uint64_t{ 1U });
	update_queue_high_water(pushed.depth);
	update_admission_result(result);
	return result;
}

telemetry_transport_pulse_result
telemetry_transport_pulse(telemetry_monotonic_usec now_monotonic_usec)
{
	return pulse_impl(now_monotonic_usec, false, nullptr);
}

telemetry_transport_drain_result
telemetry_transport_drain_until(telemetry_monotonic_usec deadline_monotonic_usec)
{
	telemetry_transport_drain_result result{};
	result.outcome = telemetry_transport_outcome::drained;
	bool attempted_any = false;
	for (std::uint32_t iteration = 0U; iteration < TELEMETRY_TRANSPORT_MAX_DRAIN_BATCHES;
	     ++iteration)
	{
		telemetry_monotonic_usec now = deadline_monotonic_usec;
		(void)clock_now(now);
		if (now > deadline_monotonic_usec)
		{
			result.outcome = telemetry_transport_outcome::deadline_reached;
			break;
		}
		telemetry_transport_pulse_result pulse_result{};
		bool called = false;
		pulse_result = pulse_impl(now, true, &called);
		attempted_any = attempted_any || called;
		if (pulse_result.outcome == telemetry_transport_outcome::flatfile_disabled)
		{
			result.outcome = pulse_result.outcome;
			break;
		}
		if (pulse_result.outcome == telemetry_transport_outcome::unavailable &&
		    INFLIGHT.active && now < INFLIGHT.retry_not_before)
		{
			result.outcome = telemetry_transport_outcome::deadline_reached;
			break;
		}
		if (pulse_result.pending == 0U)
		{
			result.outcome = telemetry_transport_outcome::drained;
			break;
		}
		if (!called && pulse_result.outcome != telemetry_transport_outcome::drained)
		{
			result.outcome = telemetry_transport_outcome::deadline_reached;
			break;
		}
		if (!called && INFLIGHT.active && now >= INFLIGHT.retry_not_before)
		{
			result.outcome = telemetry_transport_outcome::deadline_reached;
			break;
		}
		if (iteration + 1U == TELEMETRY_TRANSPORT_MAX_DRAIN_BATCHES)
			result.outcome = telemetry_transport_outcome::deadline_reached;
	}
	if (result.outcome == telemetry_transport_outcome::drained &&
	    STOP_REQUESTED.load(std::memory_order_acquire) && pending_count() == 0U)
		set_health_state(telemetry_health_state::stopped);
	result.final_flush_attempted = attempted_any ? 1U : 0U;
	const std::size_t pending = pending_count();
	result.pending = pending > std::numeric_limits<std::uint32_t>::max() ?
				 std::numeric_limits<std::uint32_t>::max() :
				 static_cast<std::uint32_t>(pending);
	if (result.pending != 0U && result.outcome == telemetry_transport_outcome::drained)
		result.outcome = telemetry_transport_outcome::deadline_reached;
	return result;
}

telemetry_transport_outcome telemetry_transport_request_stop(void)
{
	ADMISSION_STATE.fetch_or(ADMISSION_CLOSED, std::memory_order_seq_cst);
	STOP_REQUESTED.store(true, std::memory_order_release);
	QUIESCED.store(true, std::memory_order_release);
	LIFECYCLE.store(LIFECYCLE_STOPPING, std::memory_order_seq_cst);
	set_health_state(telemetry_health_state::stopping);
	return telemetry_transport_outcome::stopping;
}

void telemetry_transport_shutdown(void)
{
	if (LIFECYCLE.load(std::memory_order_seq_cst) == LIFECYCLE_STOPPED)
		return;
	/* Close admission before checking the producer handshake. */
	STOP_REQUESTED.store(true, std::memory_order_seq_cst);
	QUIESCED.store(true, std::memory_order_seq_cst);
	LIFECYCLE.store(LIFECYCLE_STOPPING, std::memory_order_seq_cst);
	set_health_state(telemetry_health_state::stopping);
	SHUTDOWN_REQUESTED.store(true, std::memory_order_release);
	finalize_shutdown();
}

void telemetry_transport_repository_shutdown_for_owner(void)
{
	bool started = true;
	if (!REPOSITORY_STARTED.compare_exchange_strong(started, false, std::memory_order_acq_rel,
							std::memory_order_acquire))
		return;
	if (REPOSITORY.request_stop != nullptr)
		(void)REPOSITORY.request_stop(REPOSITORY.context);
	if (REPOSITORY.shutdown != nullptr)
		REPOSITORY.shutdown(REPOSITORY.context);
}

telemetry_transport_loss_snapshot telemetry_transport_loss_copy_for_producer(void)
{
	return LOSS;
}

telemetry_health_snapshot telemetry_transport_health_copy(void)
{
	telemetry_health_snapshot result{};
	result.state = health_state();
	result.backend = static_cast<telemetry_storage_backend>(
		HEALTH.backend.load(std::memory_order_acquire));
	result.disabled_reason = static_cast<telemetry_disabled_reason>(
		HEALTH.disabled_reason.load(std::memory_order_acquire));
	result.last_failure_class = static_cast<telemetry_failure_class>(
		HEALTH.last_failure_class.load(std::memory_order_acquire));
	result.schema_version = TELEMETRY_SCHEMA_VERSION;
	result.last_error_code = HEALTH.last_error_code.load(std::memory_order_acquire);
	result.queue_capacity = HEALTH.queue_capacity.load(std::memory_order_acquire);
	result.producer.boot_id = HEALTH.producer_boot_id.load(std::memory_order_acquire);
	result.producer.process_id = HEALTH.producer_process_id.load(std::memory_order_acquire);
	result.last_admitted_record_seq =
		HEALTH.last_admitted_record_seq.load(std::memory_order_acquire);
	result.last_committed_record_seq =
		HEALTH.last_committed_record_seq.load(std::memory_order_acquire);
	result.inflight_first_record_seq =
		HEALTH.inflight_first_record_seq.load(std::memory_order_acquire);
	result.inflight_last_record_seq =
		HEALTH.inflight_last_record_seq.load(std::memory_order_acquire);
	result.inflight_record_kind_mask =
		HEALTH.inflight_record_kind_mask.load(std::memory_order_acquire);
	result.inflight_retry_attempts =
		HEALTH.inflight_retry_attempts.load(std::memory_order_acquire);
	result.repository_retry_attempts =
		HEALTH.repository_retry_attempts.load(std::memory_order_acquire);
	result.inflight_active = HEALTH.inflight_active.load(std::memory_order_acquire);
	result.advisory_lock_state = result.backend ==
						     telemetry_storage_backend::flatfile_disabled ?
					     telemetry_advisory_lock_state::not_applicable :
				     REPOSITORY_READY.load(std::memory_order_acquire) ?
					     telemetry_advisory_lock_state::held :
					     telemetry_advisory_lock_state::unavailable;
	const telemetry_monotonic_usec retry_not_before =
		result.inflight_active != 0U ?
			HEALTH.inflight_retry_not_before.load(std::memory_order_acquire) :
			HEALTH.repository_retry_not_before.load(std::memory_order_acquire);
	telemetry_monotonic_usec now = 0U;
	if (retry_not_before == std::numeric_limits<telemetry_monotonic_usec>::max())
		result.retry_backoff_remaining_usec = retry_not_before;
	else if (retry_not_before != 0U && clock_now(now) && retry_not_before > now)
		result.retry_backoff_remaining_usec = retry_not_before - now;
	result.queue_depth = telemetry_queue_private::depth(&QUEUE);
	result.queue_high_water = HEALTH.queue_high_water.load(std::memory_order_acquire);
	result.admitted_detail = HEALTH.admitted_detail.load(std::memory_order_acquire);
	result.admitted_control = HEALTH.admitted_control.load(std::memory_order_acquire);
	result.dropped_detail = HEALTH.dropped_detail.load(std::memory_order_acquire);
	result.dropped_control = HEALTH.dropped_control.load(std::memory_order_acquire);
	result.applied_records = HEALTH.applied_records.load(std::memory_order_acquire);
	result.duplicate_records = HEALTH.duplicate_records.load(std::memory_order_acquire);
	result.stale_checkpoint_records =
		HEALTH.stale_checkpoint_records.load(std::memory_order_acquire);
	result.invalid_records = HEALTH.invalid_records.load(std::memory_order_acquire);
	result.conflict_records = HEALTH.conflict_records.load(std::memory_order_acquire);
	result.retryable_failures = HEALTH.retryable_failures.load(std::memory_order_acquire);
	result.ambiguous_commits = HEALTH.ambiguous_commits.load(std::memory_order_acquire);
	result.quarantined_records = HEALTH.quarantined_records.load(std::memory_order_acquire);
	result.circuit_open_count = HEALTH.circuit_open_count.load(std::memory_order_acquire);
	result.last_failure_producer.boot_id =
		HEALTH.last_failure_boot_id.load(std::memory_order_acquire);
	result.last_failure_producer.process_id =
		HEALTH.last_failure_process_id.load(std::memory_order_acquire);
	result.last_failure_first_record_seq =
		HEALTH.last_failure_first_record_seq.load(std::memory_order_acquire);
	result.last_failure_last_record_seq =
		HEALTH.last_failure_last_record_seq.load(std::memory_order_acquire);
	result.last_failure_record_kind_mask =
		HEALTH.last_failure_record_kind_mask.load(std::memory_order_acquire);
	result.last_failure_retry_attempts =
		HEALTH.last_failure_retry_attempts.load(std::memory_order_acquire);
	result.sequence_gap_count = HEALTH.sequence_gap_count.load(std::memory_order_acquire);
	result.unclosed_tail_count = HEALTH.unclosed_tail_count.load(std::memory_order_acquire);
	result.attributable_duration_usec =
		HEALTH.attributable_duration_usec.load(std::memory_order_acquire);
	result.unknown_duration_usec = HEALTH.unknown_duration_usec.load(std::memory_order_acquire);
	result.last_success_monotonic_usec =
		HEALTH.last_success_monotonic_usec.load(std::memory_order_acquire);
	result.last_failure_monotonic_usec =
		HEALTH.last_failure_monotonic_usec.load(std::memory_order_acquire);
	return result;
}
