#include "telemetry/telemetry_transport_private.h"

#include <array>
#include <atomic>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <limits>
#include <thread>

namespace
{

const char *case_name = "startup";

#define CHECK(condition)                                                                           \
	do                                                                                         \
	{                                                                                          \
		if (!(condition))                                                                  \
		{                                                                                  \
			std::fprintf(stderr, "FAIL %s:%d: %s\n", case_name, __LINE__, #condition); \
			std::exit(1);                                                              \
		}                                                                                  \
	} while (0)

enum class apply_mode : std::uint8_t
{
	normal,
	transient,
	ambiguous,
	permanent,
	unavailable,
	malformed_success,
	invalid_batch,
	mixed_rejections,
	quarantined_record,
};

struct observed_call
{
	std::size_t count = 0U;
	std::array<telemetry_record, 4U> records{};
};

struct fake_repository
{
	apply_mode mode = apply_mode::normal;
	std::uint32_t failures_left = 0U;
	std::uint32_t init_failures_left = 0U;
	bool init_permanent = false;
	std::uint32_t calls = 0U;
	std::uint32_t init_calls = 0U;
	std::uint32_t invalid_single_seq = 0U;
	std::array<observed_call, 512U> observed{};
};

struct fake_clock
{
	std::atomic<telemetry_monotonic_usec> now{ 100U };
	std::atomic<bool> available{ true };
};

struct fake_io_control
{
	std::atomic<bool> block_apply{ false };
	std::atomic<bool> apply_entered{ false };
	std::atomic<bool> release_apply{ false };
};

fake_clock clock_state;
fake_repository repository_state;
fake_io_control io_control;

telemetry_repository_outcome fake_init(void *context, telemetry_repository_config config) noexcept
{
	fake_repository &repository = *static_cast<fake_repository *>(context);
	++repository.init_calls;
	if (!telemetry_repository_config_is_bounded(config))
		return telemetry_repository_outcome::invalid_config;
	if (repository.init_permanent)
		return telemetry_repository_outcome::permanent_failure;
	if (repository.init_failures_left != 0U)
	{
		--repository.init_failures_left;
		return telemetry_repository_outcome::unavailable;
	}
	return telemetry_repository_outcome::ready;
}

telemetry_apply_batch_result retry_result(const telemetry_record *records, std::size_t count,
					  telemetry_batch_outcome outcome,
					  telemetry_failure_class failure_class,
					  std::uint32_t error_code) noexcept
{
	telemetry_apply_batch_result result{};
	result.outcome = outcome;
	result.failure_class = failure_class;
	result.error_code = error_code;
	result.input_count = static_cast<std::uint16_t>(count);
	result.result_count = static_cast<std::uint16_t>(count);
	if (count != 0U)
	{
		result.first_record_seq = records[0].header.key.record_seq;
		result.last_record_seq = records[count - 1U].header.key.record_seq;
	}
	for (std::size_t index = 0U; index < count; ++index)
	{
		result.results[index].key = records[index].header.key;
		result.results[index].outcome =
			outcome == telemetry_batch_outcome::commit_ambiguous ?
				telemetry_apply_outcome::commit_ambiguous :
			outcome == telemetry_batch_outcome::permanent_failure ?
				telemetry_apply_outcome::permanent_failure :
				telemetry_apply_outcome::retryable_failure;
		result.results[index].failure_class = failure_class;
		result.results[index].error_code = error_code;
	}
	return result;
}

telemetry_apply_batch_result fake_apply(void *context, const telemetry_record *records,
					std::size_t count) noexcept
{
	fake_repository &repository = *static_cast<fake_repository *>(context);
	CHECK(records != nullptr && count > 0U && count <= 4U);
	const std::uint32_t call = repository.calls++;
	if (call < repository.observed.size())
	{
		repository.observed[call].count = count;
		for (std::size_t index = 0U; index < count; ++index)
			repository.observed[call].records[index] = records[index];
	}
	if (io_control.block_apply.load(std::memory_order_acquire))
	{
		io_control.apply_entered.store(true, std::memory_order_release);
		while (!io_control.release_apply.load(std::memory_order_acquire))
			std::this_thread::yield();
	}

	if (repository.mode == apply_mode::transient && repository.failures_left != 0U)
	{
		--repository.failures_left;
		return retry_result(records, count, telemetry_batch_outcome::retryable_failure,
				    telemetry_failure_class::transient_transaction, 1213U);
	}
	if (repository.mode == apply_mode::ambiguous && repository.failures_left != 0U)
	{
		--repository.failures_left;
		return retry_result(records, count, telemetry_batch_outcome::commit_ambiguous,
				    telemetry_failure_class::commit_ambiguous, 2013U);
	}
	if (repository.mode == apply_mode::permanent && repository.failures_left != 0U)
	{
		--repository.failures_left;
		return retry_result(records, count, telemetry_batch_outcome::permanent_failure,
				    telemetry_failure_class::permanent_schema, 1054U);
	}
	if (repository.mode == apply_mode::unavailable && repository.failures_left != 0U)
	{
		--repository.failures_left;
		telemetry_apply_batch_result result{};
		result.outcome = telemetry_batch_outcome::unavailable;
		result.failure_class = telemetry_failure_class::transient_connection;
		result.error_code = 2013U;
		return result;
	}
	if (repository.mode == apply_mode::malformed_success && repository.failures_left != 0U)
	{
		--repository.failures_left;
		telemetry_apply_batch_result result{};
		result.outcome = telemetry_batch_outcome::committed;
		return result;
	}
	if (repository.mode == apply_mode::invalid_batch && count > 1U &&
	    repository.failures_left != 0U)
	{
		--repository.failures_left;
		telemetry_apply_batch_result result{};
		result.outcome = telemetry_batch_outcome::invalid_batch;
		return result;
	}

	telemetry_apply_batch_result result{};
	result.outcome = telemetry_batch_outcome::committed;
	result.input_count = static_cast<std::uint16_t>(count);
	result.result_count = static_cast<std::uint16_t>(count);
	result.first_record_seq = records[0].header.key.record_seq;
	result.last_record_seq = records[count - 1U].header.key.record_seq;
	for (std::size_t index = 0U; index < count; ++index)
	{
		result.results[index].key = records[index].header.key;
		result.results[index].outcome = telemetry_apply_outcome::applied;
		if (repository.mode == apply_mode::mixed_rejections)
		{
			if (index == 1U)
			{
				result.results[index].outcome =
					telemetry_apply_outcome::rejected_invalid;
				++result.invalid_count;
			}
			else if (index == 2U)
			{
				result.results[index].outcome =
					telemetry_apply_outcome::duplicate_conflict;
				++result.conflict_count;
			}
		}
		if (repository.mode == apply_mode::quarantined_record && index == 0U)
		{
			result.results[index].outcome =
				telemetry_apply_outcome::quarantined_invalid;
			result.results[index].failure_class =
				telemetry_failure_class::invalid_record;
			result.results[index].error_code = 1366U;
			result.failure_class = telemetry_failure_class::invalid_record;
			result.error_code = 1366U;
			++result.invalid_count;
			++result.quarantined_count;
		}
		if (result.results[index].outcome == telemetry_apply_outcome::applied)
			++result.applied_count;
	}
	if (result.invalid_count != 0U || result.conflict_count != 0U)
		result.outcome = telemetry_batch_outcome::committed_with_rejections;
	if (repository.mode == apply_mode::invalid_batch && count == 1U &&
	    records[0].header.key.record_seq == repository.invalid_single_seq)
	{
		result = {};
		result.outcome = telemetry_batch_outcome::committed_with_rejections;
		result.input_count = 1U;
		result.result_count = 1U;
		result.first_record_seq = records[0].header.key.record_seq;
		result.last_record_seq = result.first_record_seq;
		result.results[0].key = records[0].header.key;
		result.results[0].outcome = telemetry_apply_outcome::rejected_invalid;
		result.invalid_count = 1U;
	}
	return result;
}

telemetry_repository_outcome fake_request_stop(void *) noexcept
{
	return telemetry_repository_outcome::stopping;
}

void fake_shutdown(void *) noexcept {}

bool fake_now(void *context, telemetry_monotonic_usec *value) noexcept
{
	fake_clock &clock = *static_cast<fake_clock *>(context);
	if (!clock.available.load(std::memory_order_acquire) || value == nullptr)
		return false;
	*value = clock.now.load(std::memory_order_acquire);
	return true;
}

telemetry_transport_repository_binding repository_binding()
{
	return { fake_init, fake_apply, fake_request_stop, fake_shutdown, &repository_state };
}

telemetry_transport_clock_binding clock_binding()
{
	return { fake_now, &clock_state };
}

telemetry_transport_config config(std::uint32_t capacity = 8U, std::uint32_t reserve = 2U,
				  std::uint16_t batch_records = 4U, std::uint64_t age = 10U)
{
	return { telemetry_storage_backend::sql,
		 0U,
		 TELEMETRY_SCHEMA_VERSION,
		 capacity,
		 reserve,
		 batch_records,
		 0U,
		 static_cast<std::uint32_t>(batch_records * sizeof(telemetry_record)),
		 age };
}

telemetry_record detail_record(std::uint64_t sequence)
{
	telemetry_record record{};
	record.header = { TELEMETRY_SCHEMA_VERSION,
			  telemetry_record_kind::interval,
			  0U,
			  { { 11U, 22U }, sequence },
			  1000 };
	auto &interval = record.payload.interval;
	interval.session = { { { 11U, 22U }, 33U }, 44U, 55, 66U, 77U };
	interval.connection = { { 11U, 22U }, 88U };
	interval.window = { sequence, sequence + 1U, 1000 + static_cast<std::int64_t>(sequence),
			    1001 + static_cast<std::int64_t>(sequence) };
	interval.duration_usec = 1U;
	interval.category = telemetry_interval_category::connected_active;
	interval.context = telemetry_activity_context::none;
	interval.context_quality = telemetry_context_quality::observed;
	interval.dimensions = { 1U, 2U, 3U, 4U, 5, 1U };
	interval.config_id = 1U;
	interval.classifier_version = 1U;
	interval.policy_version = 1U;
	interval.quality_flags = TELEMETRY_QUALITY_NONE;
	CHECK(telemetry_record_is_valid(record));
	return record;
}

telemetry_record control_record(std::uint64_t sequence)
{
	telemetry_record record{};
	record.header = { TELEMETRY_SCHEMA_VERSION,
			  telemetry_record_kind::coverage_gap,
			  0U,
			  { { 11U, 22U }, sequence },
			  TELEMETRY_UTC_UNKNOWN };
	record.payload.gap.reason = telemetry_gap_reason::detail_queue_drop;
	record.payload.gap.quality_flags = TELEMETRY_QUALITY_QUEUE_DROP;
	CHECK(telemetry_record_is_valid(record));
	return record;
}

void bind_and_init(const telemetry_transport_config &transport_config = config())
{
	const auto repository = repository_binding();
	const auto clock = clock_binding();
	CHECK(telemetry_transport_bind_for_tests(&repository, &clock) ==
	      telemetry_transport_outcome::started);
	const auto outcome = telemetry_transport_init(transport_config);
	CHECK(outcome == telemetry_transport_outcome::started ||
	      outcome == telemetry_transport_outcome::unavailable);
}

void finish()
{
	(void)telemetry_transport_request_stop();
	clock_state.now.store(1'000'000U, std::memory_order_release);
	std::uint32_t pending = 0U;
	for (unsigned int attempt = 0U; attempt < 128U; ++attempt)
	{
		const auto result = telemetry_transport_drain_until(2'000'000U);
		pending = result.pending;
		if (pending == 0U)
			break;
	}
	CHECK(pending == 0U);
	telemetry_transport_shutdown();
	telemetry_transport_unbind_for_tests();
	repository_state = {};
	clock_state.now.store(100U, std::memory_order_release);
	clock_state.available.store(true, std::memory_order_release);
}

void row_and_age_flush_tests()
{
	case_name = "row and age flush";
	bind_and_init(config(8U, 2U, 2U, 100U));
	CHECK(telemetry_transport_enqueue(detail_record(1U)).admission ==
	      telemetry_queue_admission::accepted_detail);
	const auto admitted = telemetry_transport_health_copy();
	CHECK(admitted.queue_capacity == 8U);
	CHECK(admitted.producer.boot_id == 11U);
	CHECK(admitted.producer.process_id == 22U);
	CHECK(admitted.last_admitted_record_seq == 1U);
	CHECK(admitted.last_committed_record_seq == 0U);
	CHECK(admitted.queue_depth == 1U);
	CHECK(admitted.advisory_lock_state == telemetry_advisory_lock_state::unavailable);
	CHECK(telemetry_transport_pulse(150U).examined == 0U);
	CHECK(telemetry_transport_enqueue(detail_record(2U)).admission ==
	      telemetry_queue_admission::accepted_detail);
	CHECK(telemetry_transport_pulse(150U).examined == 2U);
	CHECK(repository_state.calls == 1U);
	CHECK(repository_state.observed[0].records[0].header.key.record_seq == 1U);
	CHECK(repository_state.observed[0].records[1].header.key.record_seq == 2U);
	const auto committed = telemetry_transport_health_copy();
	CHECK(committed.last_admitted_record_seq == 2U);
	CHECK(committed.last_committed_record_seq == 2U);
	CHECK(committed.inflight_active == 0U);
	CHECK(committed.inflight_first_record_seq == 0U);
	CHECK(committed.inflight_last_record_seq == 0U);
	CHECK(committed.inflight_record_kind_mask == 0U);
	CHECK(committed.advisory_lock_state == telemetry_advisory_lock_state::held);
	finish();

	bind_and_init(config(8U, 2U, 4U, 10U));
	CHECK(telemetry_transport_enqueue(detail_record(3U)).admission ==
	      telemetry_queue_admission::accepted_detail);
	CHECK(telemetry_transport_pulse(105U).examined == 0U);
	CHECK(telemetry_transport_pulse(111U).examined == 1U);
	CHECK(repository_state.calls == 1U);
	finish();

	telemetry_transport_config byte_config = config(8U, 2U, 4U, 10'000U);
	byte_config.max_batch_bytes = static_cast<std::uint32_t>(sizeof(telemetry_record));
	bind_and_init(byte_config);
	CHECK(telemetry_transport_enqueue(detail_record(4U)).admission ==
	      telemetry_queue_admission::accepted_detail);
	CHECK(telemetry_transport_pulse(100U).examined == 1U);
	CHECK(repository_state.calls == 1U);
	finish();
}

void reserve_and_loss_tests()
{
	case_name = "control reserve and explicit gap";
	bind_and_init(config(4U, 1U, 2U, 1U));
	for (std::uint64_t sequence = 1U; sequence <= 3U; ++sequence)
		CHECK(telemetry_transport_enqueue(detail_record(sequence)).admission ==
		      telemetry_queue_admission::accepted_detail);
	CHECK(telemetry_transport_enqueue(control_record(4U)).admission ==
	      telemetry_queue_admission::accepted_control_reserve);
	CHECK(telemetry_transport_enqueue(detail_record(5U)).admission ==
	      telemetry_queue_admission::rejected_detail_full);
	CHECK(telemetry_transport_health_copy().dropped_detail == 1U);
	const auto dropped_control = telemetry_transport_enqueue(control_record(6U));
	CHECK(dropped_control.admission == telemetry_queue_admission::rejected_control_full ||
	      dropped_control.admission == telemetry_queue_admission::rejected_stopping);
	// The transport never recycles a rejected event key for a synthetic gap.
	for (unsigned int attempt = 0U; attempt < 8U; ++attempt)
		(void)telemetry_transport_pulse(100U + attempt);
	CHECK(repository_state.calls == 2U);
	CHECK(telemetry_transport_health_copy().queue_depth == 0U);
	CHECK(telemetry_transport_health_copy().dropped_control == 1U);
	for (std::uint32_t call = 0U; call < repository_state.calls; ++call)
		for (std::size_t index = 0U; index < repository_state.observed[call].count; ++index)
			CHECK(repository_state.observed[call].records[index].header.key.record_seq <=
			      4U);
	// The producer can report the two intentionally abandoned keys using a fresh key.
	auto gap = control_record(7U);
	gap.payload.gap.first_missing_record_seq = 5U;
	gap.payload.gap.last_missing_record_seq = 6U;
	gap.payload.gap.dropped_records = 2U;
	CHECK(telemetry_transport_enqueue(gap).admission ==
	      telemetry_queue_admission::accepted_control_reserve);
	(void)telemetry_transport_pulse(1000U);
	CHECK(repository_state.calls == 3U);
	CHECK(repository_state.observed[2].records[0].header.key.record_seq == 7U);
	CHECK(repository_state.observed[2].records[0].payload.gap.dropped_records == 2U);
	finish();
}

void immutable_retry_and_ambiguous_tests()
{
	case_name = "immutable retry and ambiguous barrier";
	bind_and_init(config(4U, 1U, 2U, 1U));
	repository_state.mode = apply_mode::transient;
	repository_state.failures_left = 1U;
	const auto first = detail_record(1U);
	CHECK(telemetry_transport_enqueue(first).admission ==
	      telemetry_queue_admission::accepted_detail);
	CHECK(telemetry_transport_enqueue(detail_record(2U)).admission ==
	      telemetry_queue_admission::accepted_detail);
	CHECK(telemetry_transport_pulse(101U).outcome == telemetry_transport_outcome::unavailable);
	CHECK(telemetry_transport_health_copy().queue_depth == 2U);
	auto newer = detail_record(3U);
	CHECK(telemetry_transport_enqueue(newer).admission ==
	      telemetry_queue_admission::accepted_detail);
	newer.payload.interval.dimensions.zone_vnum = 999;
	clock_state.now.store(1000U, std::memory_order_release);
	CHECK(telemetry_transport_pulse(1000U).examined == 0U);
	CHECK(repository_state.calls == 1U);
	clock_state.now.store(2000U, std::memory_order_release);
	CHECK(telemetry_transport_pulse(2000U).examined == 2U);
	CHECK(repository_state.calls == 2U);
	const auto recovered = telemetry_transport_health_copy();
	CHECK(recovered.state == telemetry_health_state::healthy);
	CHECK(recovered.last_failure_class == telemetry_failure_class::transient_transaction);
	CHECK(recovered.last_error_code == 1213U);
	CHECK(recovered.last_success_monotonic_usec == 2000U);
	CHECK(repository_state.observed[0].records[0].payload.interval.dimensions.zone_vnum == 5);
	CHECK(repository_state.observed[1].records[0].payload.interval.dimensions.zone_vnum == 5);
	CHECK(repository_state.observed[1].records[1].header.key.record_seq == 2U);
	CHECK(telemetry_transport_pulse(2000U).examined == 1U);
	CHECK(repository_state.observed[2].records[0].header.key.record_seq == 3U);
	finish();

	bind_and_init(config(4U, 1U, 2U, 1U));
	repository_state.mode = apply_mode::ambiguous;
	repository_state.failures_left = 1U;
	CHECK(telemetry_transport_enqueue(detail_record(10U)).admission ==
	      telemetry_queue_admission::accepted_detail);
	CHECK(telemetry_transport_pulse(101U).outcome == telemetry_transport_outcome::unavailable);
	CHECK(telemetry_transport_enqueue(detail_record(11U)).admission ==
	      telemetry_queue_admission::accepted_detail);
	CHECK(repository_state.calls == 1U);
	CHECK(telemetry_transport_pulse(1000U).examined == 0U);
	CHECK(repository_state.calls == 1U);
	clock_state.now.store(2000U, std::memory_order_release);
	CHECK(telemetry_transport_pulse(2000U).examined == 1U);
	CHECK(repository_state.calls == 2U);
	CHECK(repository_state.observed[1].records[0].header.key.record_seq == 10U);
	CHECK(telemetry_transport_pulse(2000U).examined == 1U);
	CHECK(repository_state.observed[2].records[0].header.key.record_seq == 11U);
	finish();
}

void validation_and_isolation_tests()
{
	case_name = "validated callback outcomes and bounded isolation";
	bind_and_init(config(8U, 2U, 4U, 1U));
	repository_state.mode = apply_mode::malformed_success;
	repository_state.failures_left = 1U;
	CHECK(telemetry_transport_enqueue(detail_record(20U)).admission ==
	      telemetry_queue_admission::accepted_detail);
	CHECK(telemetry_transport_pulse(100U).examined == 0U);
	CHECK(telemetry_transport_health_copy().queue_depth == 1U);
	clock_state.now.store(2000U, std::memory_order_release);
	CHECK(telemetry_transport_pulse(2000U).examined == 0U);
	CHECK(repository_state.calls == 1U);
	clock_state.now.store(4000U, std::memory_order_release);
	CHECK(telemetry_transport_pulse(4000U).examined == 1U);
	CHECK(repository_state.calls == 2U);
	finish();

	bind_and_init(config(8U, 2U, 4U, 1U));
	repository_state.mode = apply_mode::mixed_rejections;
	for (std::uint64_t sequence = 30U; sequence < 33U; ++sequence)
		CHECK(telemetry_transport_enqueue(detail_record(sequence)).admission ==
		      telemetry_queue_admission::accepted_detail);
	CHECK(telemetry_transport_pulse(101U).examined == 3U);
	CHECK(repository_state.calls == 1U);
	CHECK(telemetry_transport_health_copy().invalid_records == 1U);
	CHECK(telemetry_transport_health_copy().conflict_records == 1U);
	finish();

	bind_and_init(config(8U, 2U, 4U, 1U));
	repository_state.mode = apply_mode::invalid_batch;
	repository_state.failures_left = 1U;
	repository_state.invalid_single_seq = 41U;
	for (std::uint64_t sequence = 40U; sequence < 43U; ++sequence)
		CHECK(telemetry_transport_enqueue(detail_record(sequence)).admission ==
		      telemetry_queue_admission::accepted_detail);
	CHECK(telemetry_transport_pulse(100U).examined == 0U);
	CHECK(repository_state.calls == 0U);
	CHECK(telemetry_transport_pulse(1000U).examined == 0U);
	CHECK(repository_state.calls == 1U);
	CHECK(telemetry_transport_pulse(1000U).examined == 1U);
	CHECK(telemetry_transport_pulse(1000U).examined == 1U);
	CHECK(telemetry_transport_pulse(1000U).examined == 1U);
	CHECK(repository_state.calls == 4U);
	CHECK(telemetry_transport_health_copy().invalid_records == 1U);
	finish();

	bind_and_init(config(8U, 2U, 4U, 1U));
	repository_state.mode = apply_mode::quarantined_record;
	for (std::uint64_t sequence = 50U; sequence < 53U; ++sequence)
		CHECK(telemetry_transport_enqueue(detail_record(sequence)).admission ==
		      telemetry_queue_admission::accepted_detail);
	CHECK(telemetry_transport_pulse(101U).examined == 3U);
	CHECK(telemetry_transport_health_copy().quarantined_records == 1U);
	CHECK(telemetry_transport_health_copy().invalid_records == 1U);
	CHECK(telemetry_transport_health_copy().queue_depth == 0U);
	CHECK(telemetry_transport_health_copy().last_failure_class ==
	      telemetry_failure_class::invalid_record);
	CHECK(telemetry_transport_health_copy().last_failure_first_record_seq == 50U);
	CHECK(telemetry_transport_health_copy().last_failure_monotonic_usec == 101U);
	finish();
}

void circuit_breaker_tests()
{
	case_name = "transient retry exhaustion opens a bounded circuit";
	bind_and_init(config(4U, 1U, 1U, 1U));
	repository_state.mode = apply_mode::transient;
	repository_state.failures_left = 20U;
	CHECK(telemetry_transport_enqueue(detail_record(80U)).admission ==
	      telemetry_queue_admission::accepted_detail);
	CHECK(telemetry_transport_pulse(101U).outcome == telemetry_transport_outcome::unavailable);
	CHECK(telemetry_transport_enqueue(detail_record(81U)).admission ==
	      telemetry_queue_admission::accepted_detail);
	telemetry_monotonic_usec now = 101U;
	for (std::uint32_t attempt = 2U; attempt <= TELEMETRY_TRANSPORT_MAX_RETRY_ATTEMPTS;
	     ++attempt)
	{
		telemetry_monotonic_usec backoff = TELEMETRY_TRANSPORT_RETRY_BACKOFF_INITIAL_USEC;
		for (std::uint32_t exponent = 1U; exponent < attempt - 1U; ++exponent)
			backoff = backoff > TELEMETRY_TRANSPORT_RETRY_BACKOFF_MAX_USEC / 2U ?
					  TELEMETRY_TRANSPORT_RETRY_BACKOFF_MAX_USEC :
					  backoff * 2U;
		now += backoff;
		CHECK(telemetry_transport_pulse(now).examined == 0U);
	}
	CHECK(repository_state.calls == TELEMETRY_TRANSPORT_MAX_RETRY_ATTEMPTS);
	const auto exhausted = telemetry_transport_health_copy();
	CHECK(exhausted.retryable_failures == TELEMETRY_TRANSPORT_MAX_RETRY_ATTEMPTS);
	CHECK(exhausted.state == telemetry_health_state::circuit_open);
	CHECK(exhausted.circuit_open_count == 1U);
	CHECK(exhausted.last_failure_class == telemetry_failure_class::transient_transaction);
	CHECK(exhausted.last_error_code == 1213U);
	CHECK(exhausted.last_failure_producer.boot_id == 11U);
	CHECK(exhausted.last_failure_producer.process_id == 22U);
	CHECK(exhausted.last_failure_first_record_seq == 80U);
	CHECK(exhausted.last_failure_last_record_seq == 80U);
	CHECK(exhausted.last_failure_retry_attempts == TELEMETRY_TRANSPORT_MAX_RETRY_ATTEMPTS);
	CHECK(exhausted.last_failure_record_kind_mask ==
	      (std::uint64_t{ 1U } << static_cast<std::uint8_t>(telemetry_record_kind::interval)));
	CHECK(telemetry_transport_health_copy().queue_depth == 2U);
	for (std::uint32_t call = 0U; call < repository_state.calls; ++call)
		CHECK(repository_state.observed[call].records[0].header.key.record_seq == 80U);
	CHECK(telemetry_transport_pulse(now + 10'000'000U).outcome ==
	      telemetry_transport_outcome::unavailable);
	CHECK(repository_state.calls == TELEMETRY_TRANSPORT_MAX_RETRY_ATTEMPTS);
	CHECK(telemetry_transport_enqueue(detail_record(82U)).admission ==
	      telemetry_queue_admission::rejected_circuit_open);
	CHECK(telemetry_transport_enqueue(control_record(83U)).admission ==
	      telemetry_queue_admission::accepted_control_reserve);

	/* Explicit lifecycle recovery revalidates the repository before admission. */
	telemetry_transport_shutdown();
	repository_state = {};
	CHECK(telemetry_transport_init(config(4U, 1U, 1U, 1U)) ==
	      telemetry_transport_outcome::started);
	CHECK(telemetry_transport_enqueue(detail_record(1U)).admission ==
	      telemetry_queue_admission::accepted_detail);
	CHECK(telemetry_transport_pulse(200U).examined == 1U);
	finish();

	case_name = "permanent schema failure opens circuit immediately";
	bind_and_init(config(4U, 1U, 2U, 1U));
	repository_state.mode = apply_mode::permanent;
	repository_state.failures_left = 1U;
	CHECK(telemetry_transport_enqueue(detail_record(90U)).admission ==
	      telemetry_queue_admission::accepted_detail);
	CHECK(telemetry_transport_pulse(101U).outcome == telemetry_transport_outcome::unavailable);
	CHECK(repository_state.calls == 1U);
	const auto permanent = telemetry_transport_health_copy();
	CHECK(permanent.state == telemetry_health_state::circuit_open);
	CHECK(permanent.last_failure_class == telemetry_failure_class::permanent_schema);
	CHECK(permanent.last_error_code == 1054U);
	CHECK(permanent.last_failure_first_record_seq == 90U);
	CHECK(permanent.inflight_active == 1U);
	CHECK(permanent.inflight_first_record_seq == 90U);
	CHECK(permanent.inflight_last_record_seq == 90U);
	CHECK(permanent.inflight_record_kind_mask ==
	      (std::uint64_t{ 1U } << static_cast<std::uint8_t>(telemetry_record_kind::interval)));
	CHECK(telemetry_transport_pulse(2'000'000U).outcome ==
	      telemetry_transport_outcome::unavailable);
	CHECK(repository_state.calls == 1U);
	telemetry_transport_shutdown();
	telemetry_transport_unbind_for_tests();
	repository_state = {};

	case_name = "permanent repository initialization failure does not reconnect";
	bind_and_init(config(4U, 1U, 2U, 1U));
	repository_state.init_permanent = true;
	CHECK(telemetry_transport_enqueue(detail_record(100U)).admission ==
	      telemetry_queue_admission::accepted_detail);
	CHECK(telemetry_transport_pulse(101U).outcome == telemetry_transport_outcome::unavailable);
	CHECK(repository_state.init_calls == 1U);
	CHECK(telemetry_transport_health_copy().state == telemetry_health_state::circuit_open);
	CHECK(telemetry_transport_pulse(10'000'000U).outcome ==
	      telemetry_transport_outcome::unavailable);
	CHECK(repository_state.init_calls == 1U);
	telemetry_transport_shutdown();
	telemetry_transport_unbind_for_tests();
	repository_state = {};
}

void controlled_io_teardown_tests()
{
	case_name = "controlled fake I/O teardown";
	bind_and_init(config(4U, 1U, 1U, 1U));
	io_control.block_apply.store(true, std::memory_order_release);
	io_control.apply_entered.store(false, std::memory_order_release);
	io_control.release_apply.store(false, std::memory_order_release);
	CHECK(telemetry_transport_enqueue(detail_record(90U)).admission ==
	      telemetry_queue_admission::accepted_detail);
	std::thread worker([] { (void)telemetry_transport_drain_until(1'000'000U); });
	bool entered = false;
	for (std::uint32_t attempt = 0U; attempt < 1'000'000U; ++attempt)
	{
		if (io_control.apply_entered.load(std::memory_order_acquire))
		{
			entered = true;
			break;
		}
		std::this_thread::yield();
	}
	if (!entered)
	{
		io_control.release_apply.store(true, std::memory_order_release);
		worker.join();
	}
	CHECK(entered);
	CHECK(telemetry_transport_request_stop() == telemetry_transport_outcome::stopping);
	io_control.release_apply.store(true, std::memory_order_release);
	if (entered)
		worker.join();
	io_control.block_apply.store(false, std::memory_order_release);
	CHECK(repository_state.calls == 1U);
	CHECK(telemetry_transport_health_copy().queue_depth == 0U);
	finish();
}

void quiesce_and_reconnect_tests()
{
	case_name = "quiesce resume and reconnect";
	bind_and_init(config(4U, 1U, 2U, 1U));
	CHECK(telemetry_transport_quiesce_for_tests() == telemetry_transport_outcome::stopping);
	CHECK(telemetry_transport_enqueue(detail_record(50U)).admission ==
	      telemetry_queue_admission::rejected_stopping);
	CHECK(telemetry_transport_resume_for_tests() == telemetry_transport_outcome::started);
	CHECK(telemetry_transport_enqueue(detail_record(50U)).admission ==
	      telemetry_queue_admission::accepted_detail);
	finish();

	bind_and_init(config(4U, 1U, 2U, 1U));
	repository_state.mode = apply_mode::unavailable;
	repository_state.failures_left = 1U;
	CHECK(telemetry_transport_enqueue(detail_record(60U)).admission ==
	      telemetry_queue_admission::accepted_detail);
	CHECK(telemetry_transport_pulse(101U).outcome == telemetry_transport_outcome::unavailable);
	CHECK(repository_state.calls == 1U);
	clock_state.now.store(2000U, std::memory_order_release);
	CHECK(telemetry_transport_pulse(2000U).examined == 1U);
	CHECK(repository_state.init_calls >= 2U);
	finish();
}

void lifecycle_race_and_stress_tests()
{
	case_name = "stop before init and producer worker stress";
	const auto repository = repository_binding();
	const auto clock = clock_binding();
	CHECK(telemetry_transport_bind_for_tests(&repository, &clock) ==
	      telemetry_transport_outcome::started);
	CHECK(telemetry_transport_request_stop() == telemetry_transport_outcome::stopping);
	CHECK(telemetry_transport_init(config()) == telemetry_transport_outcome::stopping);
	CHECK(telemetry_transport_enqueue(detail_record(70U)).admission ==
	      telemetry_queue_admission::rejected_stopping);
	telemetry_transport_shutdown();
	telemetry_transport_unbind_for_tests();
	repository_state = {};

	bind_and_init(config(16U, 4U, 4U, 1U));
	std::atomic<bool> producing{ true };
	std::atomic<std::uint64_t> next_sequence{ 100U };
	std::thread worker(
		[&]
		{
			while (producing.load(std::memory_order_acquire))
			{
				clock_state.now.fetch_add(1U, std::memory_order_relaxed);
				(void)telemetry_transport_pulse(
					clock_state.now.load(std::memory_order_relaxed));
			}
			for (unsigned int attempt = 0U; attempt < 256U; ++attempt)
				(void)telemetry_transport_pulse(1'000'000U + attempt);
		});
	for (unsigned int attempt = 0U; attempt < 20'000U; ++attempt)
	{
		const std::uint64_t sequence =
			next_sequence.fetch_add(1U, std::memory_order_relaxed);
		(void)telemetry_transport_enqueue(detail_record(sequence));
	}
	producing.store(false, std::memory_order_release);
	worker.join();
	CHECK(telemetry_transport_health_copy().queue_depth <= 16U);
	finish();
}

} // namespace

/* The transport's production wrappers are intentionally unused by this focused
 * fake binding, but these definitions keep the standalone test independent of
 * SQL and the repository implementation. */
telemetry_repository_outcome telemetry_repository_init(telemetry_repository_config)
{
	return telemetry_repository_outcome::unavailable;
}
telemetry_apply_batch_result telemetry_repository_apply(const telemetry_record *, std::size_t)
{
	telemetry_apply_batch_result result{};
	result.outcome = telemetry_batch_outcome::unavailable;
	return result;
}
telemetry_repository_outcome telemetry_repository_request_stop(void)
{
	return telemetry_repository_outcome::stopping;
}
void telemetry_repository_shutdown(void) {}

int main()
{
	row_and_age_flush_tests();
	reserve_and_loss_tests();
	immutable_retry_and_ambiguous_tests();
	validation_and_isolation_tests();
	circuit_breaker_tests();
	controlled_io_teardown_tests();
	quiesce_and_reconnect_tests();
	lifecycle_race_and_stress_tests();
	std::puts("Telemetry transport fake-repository harness: PASS");
	return 0;
}
