#include "telemetry/telemetry_transport_private.h"
#include "telemetry/telemetry_queue_private.h"

#include <algorithm>
#include <array>
#include <atomic>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <thread>

namespace
{

enum class case_kind : std::uint8_t
{
	queue_saturation,
	repository_recovery,
	ambiguous_commit,
	slow_write,
	copyover_shutdown,
	churn,
};

struct fault_repository
{
	std::atomic<std::uint32_t> init_failures{ 0U };
	std::atomic<std::uint32_t> init_calls{ 0U };
	std::atomic<std::uint32_t> apply_calls{ 0U };
	std::atomic<std::uint32_t> applied_records{ 0U };
	std::atomic<bool> ambiguous_mode{ false };
	std::atomic<bool> ambiguous_sent{ false };
	std::atomic<bool> slow_block{ false };
	std::atomic<bool> apply_entered{ false };
	std::atomic<bool> release_apply{ false };
	std::atomic<std::uint64_t> first_record_seq{ 0U };
};

struct fault_clock
{
	std::atomic<telemetry_monotonic_usec> now{ 100U };
};

struct case_result
{
	const char *name = "unknown";
	std::uint64_t queue_peak = 0U;
	std::uint64_t queue_depth_before_teardown = 0U;
	std::uint64_t dropped_detail = 0U;
	std::uint64_t dropped_control = 0U;
	std::uint64_t applied_records = 0U;
	std::uint64_t duplicate_records = 0U;
	std::uint64_t ambiguous_commits = 0U;
	std::uint64_t repository_init_calls = 0U;
	std::uint64_t repository_apply_calls = 0U;
	std::uint64_t max_enqueue_ns = 0U;
	std::uint64_t stop_request_ns = 0U;
	std::uint64_t cycles = 0U;
	std::uint64_t rejected_during_quiesce = 0U;
	std::uint64_t queue_reserved_bytes = 0U;
	bool pending_zero = false;
};

[[noreturn]] void fail(const char *message)
{
	std::fprintf(stderr, "telemetry fault 272 harness: %s\n", message);
	std::exit(2);
}

case_kind parse_case(const char *value)
{
	if (std::strcmp(value, "queue_saturation") == 0)
		return case_kind::queue_saturation;
	if (std::strcmp(value, "repository_recovery") == 0)
		return case_kind::repository_recovery;
	if (std::strcmp(value, "ambiguous_commit") == 0)
		return case_kind::ambiguous_commit;
	if (std::strcmp(value, "slow_write") == 0)
		return case_kind::slow_write;
	if (std::strcmp(value, "copyover_shutdown") == 0)
		return case_kind::copyover_shutdown;
	if (std::strcmp(value, "churn") == 0)
		return case_kind::churn;
	fail("unknown case");
}

const char *case_name(case_kind value)
{
	switch (value)
	{
	case case_kind::queue_saturation:
		return "queue_saturation";
	case case_kind::repository_recovery:
		return "repository_recovery";
	case case_kind::ambiguous_commit:
		return "ambiguous_commit";
	case case_kind::slow_write:
		return "slow_write";
	case case_kind::copyover_shutdown:
		return "copyover_shutdown";
	case case_kind::churn:
		return "churn";
	}
	return "unknown";
}

telemetry_repository_outcome repository_init(void *context,
					     telemetry_repository_config config) noexcept
{
	auto &repository = *static_cast<fault_repository *>(context);
	if (!telemetry_repository_config_is_bounded(config))
		return telemetry_repository_outcome::invalid_config;
	repository.init_calls.fetch_add(1U, std::memory_order_relaxed);
	std::uint32_t remaining = repository.init_failures.load(std::memory_order_acquire);
	while (remaining != 0U && !repository.init_failures.compare_exchange_weak(
					  remaining, remaining - 1U, std::memory_order_acq_rel,
					  std::memory_order_acquire))
	{
	}
	if (remaining != 0U)
		return telemetry_repository_outcome::unavailable;
	return telemetry_repository_outcome::ready;
}

telemetry_apply_batch_result apply_result(const telemetry_record *records, std::size_t count,
					  telemetry_batch_outcome outcome,
					  telemetry_apply_outcome record_outcome)
{
	telemetry_apply_batch_result result{};
	result.outcome = outcome;
	if (outcome == telemetry_batch_outcome::commit_ambiguous)
	{
		result.failure_class = telemetry_failure_class::commit_ambiguous;
		result.error_code = 2013U;
	}
	result.input_count = static_cast<std::uint16_t>(count);
	result.result_count = static_cast<std::uint16_t>(count);
	result.first_record_seq = records[0].header.key.record_seq;
	result.last_record_seq = records[count - 1U].header.key.record_seq;
	for (std::size_t index = 0U; index < count; ++index)
	{
		result.results[index].key = records[index].header.key;
		result.results[index].outcome = record_outcome;
		result.results[index].failure_class = result.failure_class;
		result.results[index].error_code = result.error_code;
		if (record_outcome == telemetry_apply_outcome::applied)
			++result.applied_count;
		else if (record_outcome == telemetry_apply_outcome::duplicate_identical)
			++result.duplicate_count;
	}
	return result;
}

telemetry_apply_batch_result repository_apply(void *context, const telemetry_record *records,
					      std::size_t count) noexcept
{
	auto &repository = *static_cast<fault_repository *>(context);
	if (records == nullptr || count == 0U || count > TELEMETRY_BATCH_MAX_RECORDS_PROPOSAL)
		return {};
	repository.apply_calls.fetch_add(1U, std::memory_order_relaxed);
	std::uint64_t first_record = 0U;
	repository.first_record_seq.compare_exchange_strong(first_record,
							    records[0].header.key.record_seq,
							    std::memory_order_acq_rel,
							    std::memory_order_acquire);
	if (repository.slow_block.load(std::memory_order_acquire))
	{
		repository.apply_entered.store(true, std::memory_order_release);
		while (!repository.release_apply.load(std::memory_order_acquire))
			std::this_thread::yield();
	}
	if (repository.ambiguous_mode.load(std::memory_order_acquire) &&
	    !repository.ambiguous_sent.exchange(true, std::memory_order_acq_rel))
	{
		return apply_result(records, count, telemetry_batch_outcome::commit_ambiguous,
				    telemetry_apply_outcome::commit_ambiguous);
	}
	const bool duplicate = repository.ambiguous_mode.load(std::memory_order_acquire) &&
			       repository.ambiguous_sent.load(std::memory_order_acquire);
	if (duplicate)
	{
		const auto result = apply_result(records, count, telemetry_batch_outcome::committed,
						 telemetry_apply_outcome::duplicate_identical);
		return result;
	}
	repository.applied_records.fetch_add(static_cast<std::uint32_t>(count),
					     std::memory_order_relaxed);
	return apply_result(records, count, telemetry_batch_outcome::committed,
			    telemetry_apply_outcome::applied);
}

telemetry_repository_outcome repository_request_stop(void *) noexcept
{
	return telemetry_repository_outcome::stopping;
}

void repository_shutdown(void *) noexcept {}

bool clock_now(void *context, telemetry_monotonic_usec *value) noexcept
{
	if (context == nullptr || value == nullptr)
		return false;
	*value = static_cast<fault_clock *>(context)->now.load(std::memory_order_acquire);
	return true;
}

telemetry_transport_config config(std::uint32_t capacity = 64U, std::uint32_t control_reserve = 8U)
{
	return { telemetry_storage_backend::sql,
		 0U,
		 TELEMETRY_SCHEMA_VERSION,
		 capacity,
		 control_reserve,
		 4U,
		 0U,
		 static_cast<std::uint32_t>(4U * sizeof(telemetry_record)),
		 1U };
}

telemetry_record detail_record(std::uint64_t sequence)
{
	telemetry_record record{};
	record.header = { TELEMETRY_SCHEMA_VERSION,
			  telemetry_record_kind::interval,
			  0U,
			  { { 11U, 22U }, sequence },
			  1'000'000 + static_cast<telemetry_utc_usec>(sequence) };
	auto &interval = record.payload.interval;
	interval.session = { { { 11U, 22U }, sequence }, 44U, 55U, 66U, 77U };
	interval.connection = { { 11U, 22U }, sequence };
	interval.window = { sequence * 10U, sequence * 10U + 1U,
			    1'000'000 + static_cast<std::int64_t>(sequence),
			    1'000'001 + static_cast<std::int64_t>(sequence) };
	interval.duration_usec = 1U;
	interval.category = telemetry_interval_category::connected_active;
	interval.context = telemetry_activity_context::combat;
	interval.context_quality = telemetry_context_quality::observed;
	interval.dimensions = { 1U, 2U, 3U, 4U, 5, 1U };
	interval.config_id = 1U;
	interval.classifier_version = 1U;
	interval.policy_version = 1U;
	interval.quality_flags = TELEMETRY_QUALITY_NONE;
	if (!telemetry_record_is_valid(record))
		fail("detail record is invalid");
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
	if (!telemetry_record_is_valid(record))
		fail("control record is invalid");
	return record;
}

void setup(fault_repository &repository, fault_clock &clock, std::uint32_t capacity = 64U,
	   std::uint32_t reserve = 8U)
{
	const telemetry_transport_repository_binding binding = { repository_init, repository_apply,
								 repository_request_stop,
								 repository_shutdown, &repository };
	const telemetry_transport_clock_binding clock_binding = { clock_now, &clock };
	if (telemetry_transport_bind_for_tests(&binding, &clock_binding) !=
	    telemetry_transport_outcome::started)
		fail("could not bind fault repository");
	if (telemetry_transport_init(config(capacity, reserve)) !=
	    telemetry_transport_outcome::started)
		fail("could not initialize fault transport");
}

void teardown(fault_repository &repository, fault_clock &clock)
{
	repository.slow_block.store(false, std::memory_order_release);
	repository.release_apply.store(true, std::memory_order_release);
	clock.now.store(10'000'000U, std::memory_order_release);
	(void)telemetry_transport_request_stop();
	for (unsigned attempt = 0U; attempt < 32U; ++attempt)
	{
		const auto drained = telemetry_transport_drain_until(20'000'000U);
		if (drained.pending == 0U)
			break;
	}
	telemetry_transport_shutdown();
	telemetry_transport_unbind_for_tests();
}

case_result snapshot(const char *name, const fault_repository &repository,
		     const case_result &partial)
{
	case_result result = partial;
	result.name = name;
	const auto health = telemetry_transport_health_copy();
	result.queue_peak = health.queue_high_water;
	result.queue_depth_before_teardown = health.queue_depth;
	result.dropped_detail = health.dropped_detail;
	result.dropped_control = health.dropped_control;
	result.applied_records = health.applied_records;
	result.duplicate_records = health.duplicate_records;
	result.ambiguous_commits = health.ambiguous_commits;
	result.repository_init_calls = repository.init_calls.load(std::memory_order_acquire);
	result.repository_apply_calls = repository.apply_calls.load(std::memory_order_acquire);
	result.queue_reserved_bytes = sizeof(telemetry_queue_private::queue);
	result.pending_zero = health.queue_depth == 0U;
	return result;
}

case_result run_saturation()
{
	fault_repository repository{};
	fault_clock clock{};
	setup(repository, clock, 16U, 4U);
	case_result result{};
	result.name = case_name(case_kind::queue_saturation);
	for (std::uint64_t sequence = 1U; sequence <= 12U; ++sequence)
	{
		const auto started = std::chrono::steady_clock::now();
		if (telemetry_transport_enqueue(detail_record(sequence)).admission !=
		    telemetry_queue_admission::accepted_detail)
			fail("detail admission failed before saturation");
		result.max_enqueue_ns =
			std::max(result.max_enqueue_ns,
				 static_cast<std::uint64_t>(
					 std::chrono::duration_cast<std::chrono::nanoseconds>(
						 std::chrono::steady_clock::now() - started)
						 .count()));
	}
	for (std::uint64_t sequence = 13U; sequence <= 16U; ++sequence)
	{
		if (telemetry_transport_enqueue(control_record(sequence)).admission !=
		    telemetry_queue_admission::accepted_control_reserve)
			fail("control reserve did not admit a control record");
	}
	if (telemetry_transport_enqueue(detail_record(17U)).admission !=
	    telemetry_queue_admission::rejected_detail_full)
		fail("detail saturation was not reported");
	if (telemetry_transport_enqueue(control_record(18U)).admission !=
	    telemetry_queue_admission::rejected_control_full)
		fail("control saturation was not reported");
	result = snapshot(result.name, repository, result);
	if (result.queue_peak != 16U || result.dropped_detail == 0U || result.dropped_control == 0U)
		fail("saturation counters were not bounded and explicit");
	teardown(repository, clock);
	return result;
}

case_result run_recovery()
{
	fault_repository repository{};
	repository.init_failures.store(3U, std::memory_order_release);
	fault_clock clock{};
	setup(repository, clock);
	for (std::uint64_t sequence = 1U; sequence <= 8U; ++sequence)
		if (telemetry_transport_enqueue(detail_record(sequence)).admission !=
		    telemetry_queue_admission::accepted_detail)
			fail("recovery workload admission failed");
	for (telemetry_monotonic_usec now : { 100U, 2'000U, 4'000U, 8'000U })
	{
		clock.now.store(now, std::memory_order_release);
		(void)telemetry_transport_pulse(now);
	}
	clock.now.store(20'000U, std::memory_order_release);
	(void)telemetry_transport_pulse(20'000U);
	case_result result = snapshot(case_name(case_kind::repository_recovery), repository, {});
	if (result.repository_init_calls < 4U || result.queue_depth_before_teardown != 0U ||
	    result.applied_records != 8U)
		fail("repository recovery did not catch up the retained batch");
	teardown(repository, clock);
	return result;
}

case_result run_ambiguous()
{
	fault_repository repository{};
	repository.ambiguous_mode.store(true, std::memory_order_release);
	fault_clock clock{};
	setup(repository, clock);
	for (std::uint64_t sequence = 1U; sequence <= 3U; ++sequence)
		if (telemetry_transport_enqueue(detail_record(sequence)).admission !=
		    telemetry_queue_admission::accepted_detail)
			fail("ambiguous workload admission failed");
	clock.now.store(1'000U, std::memory_order_release);
	(void)telemetry_transport_pulse(1'000U);
	clock.now.store(3'000U, std::memory_order_release);
	(void)telemetry_transport_pulse(3'000U);
	case_result result = snapshot(case_name(case_kind::ambiguous_commit), repository, {});
	if (result.queue_depth_before_teardown != 0U || result.ambiguous_commits == 0U ||
	    result.duplicate_records != 3U || repository.first_record_seq.load() != 1U)
		fail("ambiguous commit was not reconciled as an immutable retry");
	teardown(repository, clock);
	return result;
}

case_result run_slow_write()
{
	fault_repository repository{};
	repository.slow_block.store(true, std::memory_order_release);
	fault_clock clock{};
	setup(repository, clock);
	for (std::uint64_t sequence = 1U; sequence <= 4U; ++sequence)
		if (telemetry_transport_enqueue(detail_record(sequence)).admission !=
		    telemetry_queue_admission::accepted_detail)
			fail("slow-write seed admission failed");
	std::thread worker([&] { (void)telemetry_transport_pulse(1'000U); });
	const auto worker_deadline = std::chrono::steady_clock::now() + std::chrono::seconds(2);
	while (!repository.apply_entered.load(std::memory_order_acquire) &&
	       std::chrono::steady_clock::now() < worker_deadline)
		std::this_thread::yield();
	if (!repository.apply_entered.load(std::memory_order_acquire))
		fail("slow-write worker did not reach the injected callback");
	case_result result{};
	result.name = case_name(case_kind::slow_write);
	for (std::uint64_t sequence = 5U; sequence <= 24U; ++sequence)
	{
		const auto started = std::chrono::steady_clock::now();
		const auto admission = telemetry_transport_enqueue(detail_record(sequence));
		const auto elapsed = static_cast<std::uint64_t>(
			std::chrono::duration_cast<std::chrono::nanoseconds>(
				std::chrono::steady_clock::now() - started)
				.count());
		result.max_enqueue_ns = std::max(result.max_enqueue_ns, elapsed);
		if (admission.admission != telemetry_queue_admission::accepted_detail)
			fail("producer admission was blocked or dropped during slow write");
	}
	const auto stop_started = std::chrono::steady_clock::now();
	(void)telemetry_transport_request_stop();
	result.stop_request_ns =
		static_cast<std::uint64_t>(std::chrono::duration_cast<std::chrono::nanoseconds>(
						   std::chrono::steady_clock::now() - stop_started)
						   .count());
	repository.release_apply.store(true, std::memory_order_release);
	worker.join();
	repository.slow_block.store(false, std::memory_order_release);
	clock.now.store(20'000U, std::memory_order_release);
	const auto drained = telemetry_transport_drain_until(30'000U);
	result.pending_zero = drained.pending == 0U;
	result = snapshot(result.name, repository, result);
	if (!result.pending_zero || result.max_enqueue_ns > 100'000'000U ||
	    result.stop_request_ns > 100'000'000U)
		fail("slow-write teardown exceeded the local bounded harness guard");
	teardown(repository, clock);
	return result;
}

case_result run_copyover_shutdown()
{
	fault_repository repository{};
	fault_clock clock{};
	setup(repository, clock);
	for (std::uint64_t sequence = 1U; sequence <= 4U; ++sequence)
		if (telemetry_transport_enqueue(detail_record(sequence)).admission !=
		    telemetry_queue_admission::accepted_detail)
			fail("copyover seed admission failed");
	if (telemetry_transport_quiesce_for_tests() != telemetry_transport_outcome::stopping)
		fail("copyover quiesce seam did not stop admission");
	case_result result{};
	result.name = case_name(case_kind::copyover_shutdown);
	if (telemetry_transport_enqueue(detail_record(5U)).admission !=
	    telemetry_queue_admission::rejected_stopping)
		fail("quiesced transport admitted a gameplay record");
	result.rejected_during_quiesce = 1U;
	if (telemetry_transport_resume_for_tests() != telemetry_transport_outcome::started)
		fail("copyover resume seam did not restore admission");
	if (telemetry_transport_enqueue(detail_record(6U)).admission !=
	    telemetry_queue_admission::accepted_detail)
		fail("resumed transport did not admit the next immutable record");
	clock.now.store(2'000U, std::memory_order_release);
	(void)telemetry_transport_pulse(2'000U);
	const auto drained = telemetry_transport_drain_until(10'000U);
	result.pending_zero = drained.pending == 0U;
	result = snapshot(result.name, repository, result);
	if (!result.pending_zero || result.queue_depth_before_teardown != 0U ||
	    result.rejected_during_quiesce != 1U)
		fail("copyover-style quiesce/resume left a queue tail");
	teardown(repository, clock);
	return result;
}

case_result run_churn()
{
	case_result result{};
	result.name = case_name(case_kind::churn);
	for (std::uint32_t cycle = 0U; cycle < 20U; ++cycle)
	{
		fault_repository repository{};
		fault_clock clock{};
		setup(repository, clock, 32U, 4U);
		if (telemetry_transport_enqueue(detail_record(1U)).admission !=
		    telemetry_queue_admission::accepted_detail)
			fail("churn admission failed");
		clock.now.store(1'000U, std::memory_order_release);
		(void)telemetry_transport_pulse(1'000U);
		const auto health = telemetry_transport_health_copy();
		if (health.queue_depth != 0U)
			fail("churn cycle retained a queue tail");
		teardown(repository, clock);
		++result.cycles;
	}
	result.pending_zero = result.cycles == 20U;
	result.queue_reserved_bytes = sizeof(telemetry_queue_private::queue);
	return result;
}

case_result run(case_kind selected)
{
	switch (selected)
	{
	case case_kind::queue_saturation:
		return run_saturation();
	case case_kind::repository_recovery:
		return run_recovery();
	case case_kind::ambiguous_commit:
		return run_ambiguous();
	case case_kind::slow_write:
		return run_slow_write();
	case case_kind::copyover_shutdown:
		return run_copyover_shutdown();
	case case_kind::churn:
		return run_churn();
	}
	fail("unreachable case");
}

} // namespace

telemetry_repository_outcome telemetry_repository_init(telemetry_repository_config)
{
	return telemetry_repository_outcome::unavailable;
}

telemetry_apply_batch_result telemetry_repository_apply(const telemetry_record *, std::size_t)
{
	return {};
}

telemetry_repository_outcome telemetry_repository_request_stop(void)
{
	return telemetry_repository_outcome::stopping;
}

void telemetry_repository_shutdown(void) {}

int main(int argc, char **argv)
{
	if (argc != 2)
		fail("usage: <queue_saturation|repository_recovery|ambiguous_commit|slow_write|copyover_shutdown|churn>");
	const case_kind selected = parse_case(argv[1]);
	const case_result result = run(selected);
	std::printf("{\"schema_version\":1,\"case\":\"%s\",\"queue_peak\":%llu,"
		    "\"queue_depth_before_teardown\":%llu,\"dropped_detail\":%llu,"
		    "\"dropped_control\":%llu,\"applied_records\":%llu,"
		    "\"duplicate_records\":%llu,\"ambiguous_commits\":%llu,"
		    "\"repository_init_calls\":%llu,\"repository_apply_calls\":%llu,"
		    "\"max_enqueue_ns\":%llu,\"stop_request_ns\":%llu,\"cycles\":%llu,"
		    "\"rejected_during_quiesce\":%llu,\"queue_reserved_bytes\":%llu,"
		    "\"pending_zero\":%s}\n",
		    result.name, static_cast<unsigned long long>(result.queue_peak),
		    static_cast<unsigned long long>(result.queue_depth_before_teardown),
		    static_cast<unsigned long long>(result.dropped_detail),
		    static_cast<unsigned long long>(result.dropped_control),
		    static_cast<unsigned long long>(result.applied_records),
		    static_cast<unsigned long long>(result.duplicate_records),
		    static_cast<unsigned long long>(result.ambiguous_commits),
		    static_cast<unsigned long long>(result.repository_init_calls),
		    static_cast<unsigned long long>(result.repository_apply_calls),
		    static_cast<unsigned long long>(result.max_enqueue_ns),
		    static_cast<unsigned long long>(result.stop_request_ns),
		    static_cast<unsigned long long>(result.cycles),
		    static_cast<unsigned long long>(result.rejected_during_quiesce),
		    static_cast<unsigned long long>(result.queue_reserved_bytes),
		    result.pending_zero ? "true" : "false");
	return 0;
}
